/* git_sync.cpp -- Draftling's Git synchronisation front-end.
 *
 * This file wires the native Git client (git_core.h / git_*.c) to the
 * editor: it keeps a real commit history under <sdcard>/.git, and on
 * every sync it
 *
 *   1. commits the current working tree ("*.md" files) locally,
 *   2. fetches the remote branch over the smart Git HTTP protocol,
 *   3. fast-forwards or rebases the local commits onto the remote tip,
 *      running a diff3 line merge and committing any conflicts as-is
 *      with <<<<<<< / ======= / >>>>>>> markers,
 *   4. checks the merged tree back out onto the SD card (the editor UI
 *      then reloads the open buffer), and
 *   5. pushes the result back to the remote.
 *
 * Configuration lives in /sdcard/git.cfg:
 *
 *   repo_url=https://github.com/user/repo
 *   branch=main
 *   token=ghp_xxx                 (HTTP Basic password)
 *   username=x-access-token       (optional; HTTP Basic user)
 *   path=notes/                   (optional remote sub-directory)
 *   author_name=Jane Doe          (optional)
 *   author_email=jane@example.com (optional)
 */
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_heap_caps.h>
#include <esp_netif_sntp.h>
#include <mbedtls/base64.h>

#include "git_sync.h"
#include "git_core.h"
#include "sd_card.h"
#include "wifi_manager.h"

static const char *TAG = "GitSync";

/* ---- configuration ------------------------------------------------- */

static struct {
    char repo_url[300];
    char branch[64];
    char token[160];
    char username[64];
    char remote_path[128];     /* remote sub-directory, no leading '/', trailing '/' */
    char author_name[64];
    char author_email[96];
    char local_path[128];      /* SD working dir, e.g. "/sdcard" */
    bool configured;
} s_cfg;

static git_sync_state_t    s_state    = GIT_SYNC_IDLE;
static git_sync_callback_t s_callback = NULL;
static char s_last_error[128] = "";
static char s_last_sync[32]   = "";

static bool s_time_synced = false;

/* ---- sync task stack (SPIRAM) ------------------------------------- */

#define GIT_SYNC_STACK_SIZE (24 * 1024)
static StackType_t *s_stack_buf = NULL;
static StaticTask_t s_task_tcb;

/* ---- helpers ----------------------------------------------------- */

static void set_error(const char *msg)
{
    strlcpy(s_last_error, msg, sizeof(s_last_error));
    s_state = GIT_SYNC_ERROR;
    if (s_callback) s_callback(GIT_SYNC_ERROR, msg);
}

static void notify(git_sync_state_t st, const char *msg)
{
    s_state = st;
    if (s_callback) s_callback(st, msg);
}

/* See the doc comment in include/git_sync.h. The native client streams
 * blobs through the SD card, so the transient PSRAM peak during a sync
 * is dominated by a single file plus the LCS merge matrix (~2 MB cap).
 * Keep the historical 2x factor and 512 KB reserve. */
extern "C" size_t git_sync_max_file_size(void)
{
    const size_t MIN_CAP = 64 * 1024;
    const size_t RESERVE = 512 * 1024;
    size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t usable = (free_psram > RESERVE) ? (free_psram - RESERVE) : (free_psram / 2);
    size_t cap = usable / 2;
    if (cap < MIN_CAP) cap = MIN_CAP;
    return cap;
}

/* ---- best-effort wall-clock ------------------------------------- */

static void ensure_time(void)
{
    if (s_time_synced) return;
    time_t now = time(NULL);
    if (now > 1735689600) { s_time_synced = true; return; }   /* already > 2025-01 */

    esp_sntp_config_t cfg = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    if (esp_netif_sntp_init(&cfg) == ESP_OK) {
        if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(6000)) == ESP_OK) {
            s_time_synced = true;
            ESP_LOGI(TAG, "SNTP time acquired");
        }
        esp_netif_sntp_deinit();
    }
}

static void make_ident(char *out, size_t outsz)
{
    time_t now = time(NULL);
    if (now < 1735689600) now = 1735689600;   /* floor at 2025-01-01 */
    const char *name  = s_cfg.author_name[0]  ? s_cfg.author_name  : "Draftling";
    const char *email = s_cfg.author_email[0] ? s_cfg.author_email : "draftling@localhost";
    snprintf(out, outsz, "%s <%s> %ld +0000", name, email, (long)now);
}

static void iso_now(char *out, size_t outsz)
{
    time_t now = time(NULL);
    struct tm tm;
    gmtime_r(&now, &tm);
    strftime(out, outsz, "%Y-%m-%dT%H:%M:%SZ", &tm);
}

/* ---- working tree <-> flat tree -------------------------------- */

#define WT_MAX_FILES 128

static bool is_md(const char *name)
{
    size_t l = strlen(name);
    return l > 3 && name[0] != '.' && strcmp(name + l - 3, ".md") == 0;
}

/* "<dir>/<name>" without tripping -Wformat-truncation. */
static void joinp(char *out, size_t osz, const char *dir, const char *name)
{
    strlcpy(out, dir, osz);
    strlcat(out, "/", osz);
    strlcat(out, name, osz);
}

/* Build a flat tree object from the "*.md" files in the working dir.
 * Returns the tree oid (zero-cleared when there are no files). */
static esp_err_t build_worktree_subtree(git_oid *out, int *out_nfiles)
{
    git_oid_clear(out);
    *out_nfiles = 0;

    sd_card_file_entry_t *entries = (sd_card_file_entry_t *)heap_caps_malloc(
        WT_MAX_FILES * sizeof(sd_card_file_entry_t), MALLOC_CAP_SPIRAM);
    if (!entries) return ESP_ERR_NO_MEM;

    int count = sd_card_list_dir(s_cfg.local_path, entries, WT_MAX_FILES);
    if (count < 0) { heap_caps_free(entries); return ESP_FAIL; }

    git_tree t = {};
    esp_err_t ret = ESP_OK;
    for (int i = 0; i < count; i++) {
        if (entries[i].is_dir || !is_md(entries[i].name)) continue;

        char path[400];
        joinp(path, sizeof(path), s_cfg.local_path, entries[i].name);
        char *data = NULL;
        size_t len = 0;
        if (sd_card_read_file(path, &data, &len) != ESP_OK) continue;

        git_oid blob;
        ret = git_odb_write(GIT_OBJ_BLOB, data, len, &blob);
        free(data);
        if (ret != ESP_OK) break;

        if (!git_tree_set(&t, 0100644, entries[i].name, &blob)) { ret = ESP_ERR_NO_MEM; break; }
        (*out_nfiles)++;
    }
    heap_caps_free(entries);

    if (ret == ESP_OK && t.count > 0)
        ret = git_tree_write(&t, out);
    git_tree_free(&t);
    return ret;
}

/* Materialise a flat tree's blobs into the working dir; delete local
 * "*.md" files not present in the tree. */
static esp_err_t checkout_subtree(const git_oid *tree_oid, int *out_written)
{
    if (out_written) *out_written = 0;

    git_tree t = {};
    esp_err_t ret = git_tree_load(tree_oid, &t);
    if (ret != ESP_OK) return ret;

    for (int i = 0; i < t.count; i++) {
        const git_tree_entry *e = &t.entries[i];
        if ((e->mode & 040000) == 040000) continue;    /* skip sub-dirs */

        git_obj_type ot;
        git_buf blob = GIT_BUF_INIT;
        if (git_odb_read(&e->oid, &ot, &blob) != ESP_OK || ot != GIT_OBJ_BLOB) {
            git_buf_free(&blob);
            continue;
        }
        char path[400];
        joinp(path, sizeof(path), s_cfg.local_path, e->name);

        /* Skip the write if the on-disk content already matches. */
        char *cur = NULL;
        size_t curlen = 0;
        bool same = false;
        if (sd_card_read_file(path, &cur, &curlen) == ESP_OK) {
            same = (curlen == blob.len) && (memcmp(cur, blob.data, curlen) == 0);
            free(cur);
        }
        if (!same) {
            sd_card_write_file(path, (const char *)blob.data, blob.len);
            if (out_written) (*out_written)++;
        }
        git_buf_free(&blob);
    }

    /* Delete local *.md files that are gone from the tree. */
    sd_card_file_entry_t *entries = (sd_card_file_entry_t *)heap_caps_malloc(
        WT_MAX_FILES * sizeof(sd_card_file_entry_t), MALLOC_CAP_SPIRAM);
    if (entries) {
        int count = sd_card_list_dir(s_cfg.local_path, entries, WT_MAX_FILES);
        for (int i = 0; i < count; i++) {
            if (entries[i].is_dir || !is_md(entries[i].name)) continue;
            if (git_tree_get(&t, entries[i].name)) continue;
            char path[400];
            joinp(path, sizeof(path), s_cfg.local_path, entries[i].name);
            sd_card_delete_file(path);
        }
        heap_caps_free(entries);
    }

    git_tree_free(&t);
    return ESP_OK;
}

/* Turn merge results into a stored flat tree. */
static esp_err_t results_to_tree(const git_merge_path_result *res, int n, git_oid *out)
{
    git_tree t = {};
    for (int i = 0; i < n; i++) {
        if (git_oid_is_zero(&res[i].oid)) continue;      /* deleted */
        if (!git_tree_set(&t, 0100644, res[i].name, &res[i].oid)) {
            git_tree_free(&t);
            return ESP_ERR_NO_MEM;
        }
    }
    esp_err_t ret;
    if (t.count > 0) ret = git_tree_write(&t, out);
    else { git_oid_clear(out); ret = ESP_OK; }
    git_tree_free(&t);
    return ret;
}

static esp_err_t commit_tree_oid(const git_oid *commit_oid, git_oid *out_tree)
{
    if (git_oid_is_zero(commit_oid)) { git_oid_clear(out_tree); return ESP_OK; }
    git_commit c;
    esp_err_t ret = git_commit_load(commit_oid, &c);
    if (ret != ESP_OK) return ret;
    *out_tree = c.tree;
    git_commit_free(&c);
    return ESP_OK;
}

/* Subtree oid for the configured remote path within a commit's tree. */
static esp_err_t subtree_for_commit(const git_oid *commit_oid, git_oid *out)
{
    git_oid root;
    esp_err_t ret = commit_tree_oid(commit_oid, &root);
    if (ret != ESP_OK) return ret;
    if (git_oid_is_zero(&root)) { git_oid_clear(out); return ESP_OK; }
    ret = git_tree_subtree_oid(&root, s_cfg.remote_path, out);
    if (ret == ESP_ERR_NOT_FOUND) { git_oid_clear(out); return ESP_OK; }
    return ret;
}

/* ---- the sync itself ------------------------------------------- */

struct sync_stats {
    bool committed_local;
    bool fast_forwarded;
    bool rebased;
    bool pushed;
    int  nconflicts;
    int  files_out;
};

static esp_err_t do_sync(git_sync_direction_t dir, sync_stats *st)
{
    memset(st, 0, sizeof(*st));

    char localref[96];
    char remoteref[96];
    snprintf(localref, sizeof(localref), "refs/heads/%s", s_cfg.branch);
    snprintf(remoteref, sizeof(remoteref), "refs/remotes/origin/%s", s_cfg.branch);
    char server_ref[96];
    snprintf(server_ref, sizeof(server_ref), "refs/heads/%s", s_cfg.branch);

    /* remote descriptor */
    git_remote remote;
    memset(&remote, 0, sizeof(remote));
    {
        char url[400];
        strlcpy(url, s_cfg.repo_url, sizeof(url));
        size_t n = strlen(url);
        while (n > 1 && url[n - 1] == '/') url[--n] = '\0';
        if (n < 4 || strcmp(url + n - 4, ".git") != 0)
            strlcat(url, ".git", sizeof(url));
        strlcpy(remote.base_url, url, sizeof(remote.base_url));

        if (s_cfg.token[0]) {
            const char *user = s_cfg.username[0] ? s_cfg.username : "x-access-token";
            char raw[240];
            int rl = snprintf(raw, sizeof(raw), "%s:%s", user, s_cfg.token);
            if (rl > (int)sizeof(raw) - 1) rl = sizeof(raw) - 1;
            unsigned char b64[340];
            size_t bl = 0;
            if (mbedtls_base64_encode(b64, sizeof(b64) - 1, &bl,
                                      (const unsigned char *)raw, (size_t)rl) == 0) {
                b64[bl] = '\0';
                memcpy(remote.auth_basic, "Basic ", 6);
                strlcpy(remote.auth_basic + 6, (char *)b64, sizeof(remote.auth_basic) - 6);
            }
        }
    }

    if (git_repo_open(s_cfg.local_path) != ESP_OK) {
        set_error("Cannot create .git on SD card");
        return ESP_FAIL;
    }
    git_head_set_symbolic(localref);

    /* 1. advertise remote refs */
    notify(GIT_SYNC_IN_PROGRESS, "Contacting remote...");
    git_ref_adv adv[32];
    int nadv = 0;
    esp_err_t ret = git_http_list_refs(&remote, "git-upload-pack", adv, 32, &nadv);
    if (ret == ESP_ERR_INVALID_STATE) { set_error("Auth rejected (check token)"); return ret; }
    if (ret != ESP_OK) { set_error("Cannot reach remote repository"); return ret; }

    git_oid remote_sha;
    bool remote_has_branch = false;
    for (int i = 0; i < nadv; i++) {
        if (strcmp(adv[i].name, server_ref) == 0) {
            remote_sha = adv[i].oid;
            remote_has_branch = true;
            break;
        }
    }

    git_oid local_head;
    bool have_head = (git_ref_read(localref, &local_head) == ESP_OK) &&
                     !git_oid_is_zero(&local_head);

    /* 2. initial clone */
    if (!have_head && remote_has_branch) {
        notify(GIT_SYNC_IN_PROGRESS, "Cloning history...");
        char pack[160];
        snprintf(pack, sizeof(pack), "%s/tmp/fetch.pack", git_repo_gitdir());
        ret = git_http_fetch_pack(&remote, &remote_sha, 1, NULL, 0, pack);
        if (ret != ESP_OK) { set_error("Fetch failed"); return ret; }
        FILE *fp = fopen(pack, "rb");
        ret = fp ? git_pack_unpack_file(fp) : ESP_FAIL;
        if (fp) fclose(fp);
        remove(pack);
        if (ret != ESP_OK) { set_error("Cannot unpack fetched history"); return ret; }

        git_ref_write(localref, &remote_sha);
        git_ref_write(remoteref, &remote_sha);
        local_head = remote_sha;
        have_head = true;

        git_oid sub;
        if (subtree_for_commit(&local_head, &sub) == ESP_OK)
            checkout_subtree(&sub, NULL);
    }

    /* 3. local commit from the working tree */
    notify(GIT_SYNC_IN_PROGRESS, "Committing local changes...");
    ensure_time();

    git_oid ours_sub;
    int nfiles = 0;
    ret = build_worktree_subtree(&ours_sub, &nfiles);
    if (ret != ESP_OK) { set_error("Cannot read working tree"); return ret; }

    git_oid cur_sub;
    ret = subtree_for_commit(have_head ? &local_head : NULL, &cur_sub);
    if (ret != ESP_OK) { set_error("Cannot read current tree"); return ret; }

    if (!git_oid_eq(&ours_sub, &cur_sub)) {
        git_oid base_root;
        commit_tree_oid(have_head ? &local_head : NULL, &base_root);
        git_oid new_root;
        ret = git_tree_splice_subtree(git_oid_is_zero(&base_root) ? NULL : &base_root,
                                      s_cfg.remote_path, &ours_sub, &new_root);
        if (ret != ESP_OK) { set_error("Cannot build commit tree"); return ret; }

        char ident[224], iso[32], msg[128];
        make_ident(ident, sizeof(ident));
        iso_now(iso, sizeof(iso));
        snprintf(msg, sizeof(msg), "Draftling sync: %d file(s) at %s", nfiles, iso);

        git_oid parents[1];
        int np = 0;
        if (have_head) { parents[0] = local_head; np = 1; }

        git_oid newc;
        ret = git_commit_create(&new_root, parents, np, ident, msg, &newc);
        if (ret != ESP_OK) { set_error("Cannot create local commit"); return ret; }

        git_ref_write(localref, &newc);
        local_head = newc;
        have_head = true;
        st->committed_local = true;
        ESP_LOGI(TAG, "local commit %s", newc.hex);
    }

    /* 4. integrate remote */
    bool need_integrate = remote_has_branch &&
                          !git_oid_eq(&remote_sha, &local_head) &&
                          !git_is_ancestor(&remote_sha, &local_head);

    if (need_integrate) {
        if (!git_odb_exists(&remote_sha)) {
            notify(GIT_SYNC_IN_PROGRESS, "Fetching remote commits...");
            git_oid haves[16];
            int nh = 0;
            git_oid *chain = NULL;
            int nchain = 0;
            if (git_commit_list_linear(NULL, &local_head, &chain, &nchain) == ESP_OK) {
                for (int i = nchain - 1; i >= 0 && nh < 16; i--) haves[nh++] = chain[i];
                free(chain);
            }
            char pack[160];
            snprintf(pack, sizeof(pack), "%s/tmp/fetch.pack", git_repo_gitdir());
            ret = git_http_fetch_pack(&remote, &remote_sha, 1, haves, nh, pack);
            if (ret != ESP_OK) { set_error("Fetch failed"); return ret; }
            FILE *fp = fopen(pack, "rb");
            ret = fp ? git_pack_unpack_file(fp) : ESP_FAIL;
            if (fp) fclose(fp);
            remove(pack);
            if (ret != ESP_OK) { set_error("Cannot unpack remote commits"); return ret; }
        }

        git_oid mb;
        git_merge_base(&local_head, &remote_sha, &mb);

        if (git_oid_eq(&mb, &remote_sha)) {
            /* remote tip is already an ancestor -- we are ahead, nothing
             * to integrate; the push below fast-forwards the remote. */
            ESP_LOGI(TAG, "local ahead of remote, no integrate needed");
        } else if (git_oid_eq(&mb, &local_head)) {
            /* fast-forward */
            local_head = remote_sha;
            git_ref_write(localref, &local_head);
            st->fast_forwarded = true;
            ESP_LOGI(TAG, "fast-forward to %s", remote_sha.hex);
        } else {
            notify(GIT_SYNC_IN_PROGRESS, "Rebasing local commits...");
            git_oid *commits = NULL;
            int ncommits = 0;
            const git_oid *mbp = git_oid_is_zero(&mb) ? NULL : &mb;
            ret = git_commit_list_linear(mbp, &local_head, &commits, &ncommits);
            if (ret != ESP_OK) { set_error("History not linear -- cannot rebase"); return ret; }

            git_oid newbase = remote_sha;
            char ident[224];
            make_ident(ident, sizeof(ident));

            for (int i = 0; i < ncommits; i++) {
                git_commit cc;
                if (git_commit_load(&commits[i], &cc) != ESP_OK) { ret = ESP_FAIL; break; }
                git_oid cparent = (cc.nparents > 0) ? cc.parents[0] : (git_oid){0};

                git_oid base_sub, ours_sub2, theirs_sub;
                subtree_for_commit(git_oid_is_zero(&cparent) ? NULL : &cparent, &base_sub);
                subtree_for_commit(&newbase, &ours_sub2);
                subtree_for_commit(&commits[i], &theirs_sub);

                git_merge_path_result *res = NULL;
                int nres = 0, nconf = 0;
                ret = git_merge_trees_flat(&base_sub, &ours_sub2, &theirs_sub,
                                           "ours (remote)", "theirs (local)",
                                           &res, &nres, &nconf);
                if (ret != ESP_OK) { git_commit_free(&cc); break; }

                git_oid merged_sub;
                ret = results_to_tree(res, nres, &merged_sub);
                free(res);
                if (ret != ESP_OK) { git_commit_free(&cc); break; }

                git_oid nb_root;
                commit_tree_oid(&newbase, &nb_root);
                git_oid new_root;
                ret = git_tree_splice_subtree(git_oid_is_zero(&nb_root) ? NULL : &nb_root,
                                              s_cfg.remote_path, &merged_sub, &new_root);
                if (ret != ESP_OK) { git_commit_free(&cc); break; }

                char msg[256];
                if (nconf)
                    snprintf(msg, sizeof(msg), "%s\n\n[draftling] %d conflict(s) committed with markers",
                             cc.message ? cc.message : "Draftling sync", nconf);
                else
                    snprintf(msg, sizeof(msg), "%s", cc.message ? cc.message : "Draftling sync");
                git_commit_free(&cc);

                git_oid parents[1] = { newbase };
                git_oid newc;
                ret = git_commit_create(&new_root, parents, 1, ident, msg, &newc);
                if (ret != ESP_OK) break;
                newbase = newc;
                st->nconflicts += nconf;
            }
            free(commits);
            if (ret != ESP_OK) { set_error("Rebase failed"); return ret; }

            local_head = newbase;
            git_ref_write(localref, &local_head);
            st->rebased = true;
        }
    } else if (remote_has_branch && git_oid_eq(&remote_sha, &local_head)) {
        git_ref_write(remoteref, &remote_sha);
    }

    /* 5. checkout the integrated tree */
    {
        git_oid sub;
        if (subtree_for_commit(&local_head, &sub) == ESP_OK) {
            notify(GIT_SYNC_IN_PROGRESS, "Updating working files...");
            checkout_subtree(&sub, &st->files_out);
        }
    }

    /* 6. push */
    bool want_push = (dir != GIT_SYNC_PULL);
    bool have_remote_target = (s_cfg.repo_url[0] != '\0');

    if (want_push && have_remote_target && !git_oid_eq(
            &local_head, remote_has_branch ? &remote_sha : &local_head)) {

        if (remote_has_branch && !git_is_ancestor(&remote_sha, &local_head)) {
            set_error("Push blocked: remote advanced during sync -- retry");
            return ESP_FAIL;
        }

        notify(GIT_SYNC_IN_PROGRESS, "Pushing to remote...");
        git_oidset set = {};
        git_oid haves[1];
        int nh = 0;
        if (remote_has_branch) haves[nh++] = remote_sha;

        ret = git_reachable_objects(&local_head, 1, haves, nh, &set);
        if (ret != ESP_OK) { git_oidset_free(&set); set_error("Cannot enumerate objects"); return ret; }

        char pack[160];
        snprintf(pack, sizeof(pack), "%s/tmp/push.pack", git_repo_gitdir());
        ret = git_pack_write_file(pack, &set);
        git_oidset_free(&set);
        if (ret != ESP_OK) { remove(pack); set_error("Cannot build packfile"); return ret; }

        git_oid old_oid;
        git_oid_clear(&old_oid);
        if (remote_has_branch) old_oid = remote_sha;

        git_buf report = GIT_BUF_INIT;
        ret = git_http_push(&remote, server_ref, &old_oid, &local_head, pack, &report);
        remove(pack);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "push report: %.*s", (int)report.len, (char *)report.data);
            git_buf_free(&report);
            set_error("Remote rejected the push");
            return ret;
        }
        git_buf_free(&report);

        git_ref_write(remoteref, &local_head);
        st->pushed = true;
        ESP_LOGI(TAG, "pushed %s", local_head.hex);
    }

    return ESP_OK;
}

/* ---- task ------------------------------------------------------- */

static void sync_task(void *arg)
{
    git_sync_direction_t dir = (git_sync_direction_t)(intptr_t)arg;

    if (!wifi_manager_is_connected()) {
        set_error("WiFi not connected");
        vTaskDelete(NULL);
        return;
    }

    sync_stats st;
    esp_err_t ret = do_sync(dir, &st);

    git_repo_close();

    if (ret == ESP_OK) {
        char summary[128];
        size_t pos = 0;
        pos += snprintf(summary + pos, sizeof(summary) - pos,
                        st.nconflicts ? "Sync done, %d conflict(s)" : "Sync complete",
                        st.nconflicts);
        const char *parts[5];
        int np = 0;
        char b1[24], b2[24];
        if (st.committed_local) parts[np++] = "committed";
        if (st.fast_forwarded)  parts[np++] = "fast-forward";
        if (st.rebased)         parts[np++] = "rebased";
        if (st.files_out) { snprintf(b1, sizeof(b1), "%d file(s) updated", st.files_out); parts[np++] = b1; }
        if (st.pushed) { snprintf(b2, sizeof(b2), "pushed"); parts[np++] = b2; }
        if (np) {
            pos += snprintf(summary + pos, sizeof(summary) - pos, " (");
            for (int i = 0; i < np && pos < sizeof(summary); i++)
                pos += snprintf(summary + pos, sizeof(summary) - pos, "%s%s",
                                i ? ", " : "", parts[i]);
            snprintf(summary + pos, sizeof(summary) - pos, ")");
        }
        strlcpy(s_last_sync, "OK", sizeof(s_last_sync));
        notify(GIT_SYNC_SUCCESS, summary);
    } else if (s_state != GIT_SYNC_ERROR) {
        set_error("Sync failed");
    }

    vTaskDelete(NULL);
}

/* ---- config --------------------------------------------------- */

static void trim(char *s)
{
    size_t n = strlen(s);
    while (n && (s[n - 1] == '\r' || s[n - 1] == '\n' || s[n - 1] == ' ' || s[n - 1] == '\t'))
        s[--n] = '\0';
}

static void parse_config(const char *data)
{
    memset(&s_cfg, 0, sizeof(s_cfg));
    strlcpy(s_cfg.branch, "main", sizeof(s_cfg.branch));
    strlcpy(s_cfg.local_path, sd_card_get_mount_point(), sizeof(s_cfg.local_path));

    const char *p = data;
    while (*p) {
        const char *nl = strchr(p, '\n');
        size_t linelen = nl ? (size_t)(nl - p) : strlen(p);
        char line[400];
        if (linelen >= sizeof(line)) linelen = sizeof(line) - 1;
        memcpy(line, p, linelen);
        line[linelen] = '\0';
        p = nl ? nl + 1 : p + strlen(p);

        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = line;
        char *val = eq + 1;
        trim(key);
        trim(val);

        if (!strcmp(key, "repo_url"))          strlcpy(s_cfg.repo_url, val, sizeof(s_cfg.repo_url));
        else if (!strcmp(key, "branch"))       strlcpy(s_cfg.branch, val, sizeof(s_cfg.branch));
        else if (!strcmp(key, "token"))        strlcpy(s_cfg.token, val, sizeof(s_cfg.token));
        else if (!strcmp(key, "username"))     strlcpy(s_cfg.username, val, sizeof(s_cfg.username));
        else if (!strcmp(key, "author_name"))  strlcpy(s_cfg.author_name, val, sizeof(s_cfg.author_name));
        else if (!strcmp(key, "author_email")) strlcpy(s_cfg.author_email, val, sizeof(s_cfg.author_email));
        else if (!strcmp(key, "path")) {
            strlcpy(s_cfg.remote_path, val, sizeof(s_cfg.remote_path));
            /* normalise: no leading '/', exactly one trailing '/' */
            char *q = s_cfg.remote_path;
            while (*q == '/') memmove(q, q + 1, strlen(q));
            size_t n = strlen(s_cfg.remote_path);
            while (n && s_cfg.remote_path[n - 1] == '/') s_cfg.remote_path[--n] = '\0';
            if (n && n + 1 < sizeof(s_cfg.remote_path)) {
                s_cfg.remote_path[n] = '/';
                s_cfg.remote_path[n + 1] = '\0';
            }
        }
    }

    s_cfg.configured = (s_cfg.repo_url[0] != '\0');
    if (s_cfg.branch[0] == '\0') strlcpy(s_cfg.branch, "main", sizeof(s_cfg.branch));
}

/* ---- public API --------------------------------------------- */

extern "C" esp_err_t git_sync_init(void)
{
    char *data = NULL;
    size_t len = 0;
    if (sd_card_read_file("/sdcard/git.cfg", &data, &len) == ESP_OK) {
        parse_config(data);
        free(data);
        if (s_cfg.configured)
            ESP_LOGI(TAG, "Git configured: %s branch=%s path=%s",
                     s_cfg.repo_url, s_cfg.branch,
                     s_cfg.remote_path[0] ? s_cfg.remote_path : "(root)");
    } else {
        ESP_LOGI(TAG, "No /sdcard/git.cfg -- sync disabled");
    }
    return ESP_OK;
}

extern "C" esp_err_t git_sync_start(git_sync_direction_t direction)
{
    if (!s_cfg.configured) { set_error("Not configured"); return ESP_ERR_INVALID_STATE; }
    if (s_state == GIT_SYNC_IN_PROGRESS) { set_error("Sync already in progress"); return ESP_ERR_INVALID_STATE; }

    s_state = GIT_SYNC_IN_PROGRESS;

    if (!s_stack_buf) {
        s_stack_buf = (StackType_t *)heap_caps_malloc(GIT_SYNC_STACK_SIZE, MALLOC_CAP_SPIRAM);
        if (!s_stack_buf) { set_error("Failed to alloc sync stack"); return ESP_ERR_NO_MEM; }
    }

    TaskHandle_t h = xTaskCreateStaticPinnedToCore(
        sync_task, "git_sync", GIT_SYNC_STACK_SIZE / sizeof(StackType_t),
        (void *)(intptr_t)direction, 3, s_stack_buf, &s_task_tcb, 0);
    if (!h) { set_error("Failed to start sync task"); return ESP_FAIL; }
    return ESP_OK;
}

extern "C" git_sync_state_t git_sync_get_state(void) { return s_state; }
extern "C" void git_sync_set_callback(git_sync_callback_t cb) { s_callback = cb; }
extern "C" bool git_sync_is_configured(void) { return s_cfg.configured; }
extern "C" const char *git_sync_get_last_error(void) { return s_last_error; }
extern "C" const char *git_sync_get_last_sync_time(void) { return s_last_sync; }
