/* git_sync.cpp -- Draftling's Git synchronisation front-end.
 *
 * This file wires the native Git client (git_core.h / git_*.c) to the
 * editor. Every project folder on the SD card (/sdcard/<project>) is
 * synced on its own: it keeps a real commit history under
 * <project>/.git, and on every sync it
 *
 *   1. commits the current working tree ("*.md", "*.fountain" and
 *      "*.txt" files) locally,
 *   2. fetches the remote branch over the smart Git HTTP protocol,
 *   3. fast-forwards or rebases the local commits onto the remote tip,
 *      running a diff3 line merge and committing any conflicts as-is
 *      with <<<<<<< / ======= / >>>>>>> markers,
 *   4. checks the merged tree back out onto the SD card (the editor UI
 *      then reloads the open buffer), and
 *   5. pushes the result back to the remote.
 *
 * Configuration lives in <project>/git.cfg:
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
#include <algorithm>
#include <string>
#include <strings.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_heap_caps.h>
#include <esp_netif_sntp.h>
#include <esp_wifi.h>
#include <mbedtls/base64.h>

#include "git_sync.h"
#include "git_core.h"
#include "draftbox.h"
#include "sd_card.h"
#include "wifi_manager.h"

static const char *TAG = "GitSync";

/* ---- configuration ------------------------------------------------- */

typedef struct {
    char repo_url[300];
    char branch[64];
    char token[160];
    char username[64];
    char remote_path[128];     /* remote sub-directory, no leading '/', trailing '/' */
    char author_name[64];
    char author_email[96];
    char local_path[160];      /* project folder, e.g. "/sdcard/MyDrafts" */
    bool configured;
    bool bad_branch;           /* branch= is not a valid branch name */
} git_cfg_t;

/* Configuration of the project being synced. Written only by
 * git_sync_start() while no sync runs; the sync task reads it. Other
 * calls load their own copy (load_config()). */
static git_cfg_t s_cfg;

static git_sync_state_t    s_state    = GIT_SYNC_IDLE;
static git_sync_callback_t s_callback = NULL;
static char s_last_error[128] = "";

static bool s_time_synced = false;

/* ---- sync task stack (SPIRAM) ------------------------------------- */

#define GIT_SYNC_STACK_SIZE (24 * 1024)
static StackType_t *s_stack_buf = NULL;
static StaticTask_t s_task_tcb;
/* One long-lived worker runs every sync: its static stack and TCB are
 * never handed to a second task while the first is still winding
 * down (a self-deleted static task stays on FreeRTOS' termination list
 * until the idle task gets to it). */
static TaskHandle_t s_worker = NULL;
static volatile git_sync_direction_t s_req_dir = GIT_SYNC_BOTH;

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

/* Status messages name the server by its host name ("Pushing to
 * github.com..."): copy the host part of repo_url (no scheme, user
 * info, port or path) into out, or "server" if there is none. */
static void remote_host_of(const char *url, char *out, size_t osz)
{
    const char *p = url;
    const char *sch = strstr(p, "://");
    if (sch) p = sch + 3;
    size_t n = strcspn(p, "/?#");
    const char *at = (const char *)memchr(p, '@', n);
    if (at) { n -= (size_t)(at + 1 - p); p = at + 1; }
    if (n && p[0] == '[') {
        /* IPv6 literal: keep the brackets, drop a port after them */
        const char *rb = (const char *)memchr(p, ']', n);
        if (rb) n = (size_t)(rb + 1 - p);
    } else {
        const char *colon = (const char *)memchr(p, ':', n);
        if (colon) n = (size_t)(colon - p);
    }
    if (n == 0) { strlcpy(out, "server", osz); return; }
    if (n >= osz) n = osz - 1;
    memcpy(out, p, n);
    out[n] = '\0';
}

/* Repository name: the URL path after the host, without the leading
 * '/', a query or fragment, trailing '/' and ".git". */
static void repo_name_of(const char *url, char *out, size_t osz)
{
    const char *p = url;
    const char *sch = strstr(p, "://");
    if (sch) p = sch + 3;
    p += strcspn(p, "/?#");
    while (*p == '/') p++;
    size_t n = strcspn(p, "?#");
    while (n && p[n - 1] == '/') n--;
    if (n >= 4 && strncmp(p + n - 4, ".git", 4) == 0) n -= 4;
    if (n == 0) { strlcpy(out, "(server root)", osz); return; }
    if (n >= osz) n = osz - 1;
    memcpy(out, p, n);
    out[n] = '\0';
}

static void remote_host(char *out, size_t osz)
{
    remote_host_of(s_cfg.repo_url, out, osz);
}

/* ---- best-effort wall-clock ------------------------------------- */

static void ensure_time(void)
{
    if (s_time_synced) return;
    /* Only ever attempt this once per boot -- set the flag up front,
     * regardless of whether the sync below actually succeeds, so an
     * unreachable NTP server costs one bounded wait per boot rather
     * than one on every single git sync.
     *
     * There used to be a "skip entirely if time(NULL) already reads a
     * plausible post-2025 date" shortcut here. That is not the same
     * thing as "the clock is trustworthy": ESP-IDF's system clock is
     * backed by the RTC timer domain, which keeps ticking through
     * deep sleep, so on every wake it already reads whatever the RTC
     * tracked across the nap -- using the internal RC oscillator on
     * boards with no external 32kHz crystal, which can drift by
     * minutes over a long sleep. That drifted value still looks
     * "plausible" (> 2025-01-01), so the shortcut skipped the actual
     * correction on every wake, meaning a real SNTP sync effectively
     * only ever ran once, on the device's very first-ever boot, and
     * every commit since then used a clock that only ever drifts
     * further. Attempting SNTP unconditionally on every boot corrects
     * that drift each time instead of trusting a carried-over guess. */
    s_time_synced = true;

    esp_sntp_config_t cfg = ESP_NETIF_SNTP_DEFAULT_CONFIG("2.pool.ntp.org");
    if (esp_netif_sntp_init(&cfg) == ESP_OK) {
        if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(6000)) == ESP_OK) {
            ESP_LOGI(TAG, "SNTP time acquired");
        } else {
            ESP_LOGW(TAG, "SNTP sync timed out, using RTC-tracked time");
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

/* A working-tree document: Markdown ("*.md"), a Fountain screenplay
 * ("*.fountain") or plain text ("*.txt"). Hidden files (the editor's ".<name>.meta" sidecars)
 * are never synced. */
static bool has_suffix(const char *name, size_t l, const char *sfx)
{
    size_t n = strlen(sfx);
    return l > n && strcmp(name + l - n, sfx) == 0;
}

static bool is_doc(const char *name)
{
    size_t l = strlen(name);
    return name[0] != '.' &&
           (has_suffix(name, l, ".md") || has_suffix(name, l, ".fountain") ||
            has_suffix(name, l, ".txt"));
}

/* A name the SD card's FAT file system can store: none of
 * \ / : * ? " < > |, no control character, no trailing dot or space. */
static bool fat_name_ok(const char *name)
{
    size_t l = strlen(name);
    if (l == 0 || name[l - 1] == '.' || name[l - 1] == ' ') return false;
    for (const char *p = name; *p; p++) {
        unsigned char c = (unsigned char)*p;
        if (c < 0x20 || strchr("\\/:*?\"<>|", c)) return false;
    }
    return true;
}

/* A tree entry the sync keeps in step with the card: a plain file
 * (not a folder, symlink or submodule) that is a document and whose
 * name the card can store. Every other entry stays in Git untouched:
 * never written to the card, never removed from the tree. */
static bool synced_entry(const git_tree_entry *e)
{
    uint32_t type = e->mode & 0170000;
    return type == 0100000 && is_doc(e->name) && fat_name_ok(e->name);
}

/* "<dir>/<name>" without tripping -Wformat-truncation. */
static void joinp(char *out, size_t osz, const char *dir, const char *name)
{
    strlcpy(out, dir, osz);
    strlcat(out, "/", osz);
    strlcat(out, name, osz);
}

/* Whether the working dir holds any document. */
static esp_err_t worktree_has_docs(bool *out)
{
    *out = false;
    sd_card_file_entry_t *entries = NULL;
    int count = 0;
    esp_err_t ret = sd_card_list_dir(s_cfg.local_path, &entries, &count);
    if (ret != ESP_OK) return ret;
    for (int i = 0; i < count && !*out; i++)
        if (!entries[i].is_dir && is_doc(entries[i].name)) *out = true;
    sd_card_free_list(entries);
    return ESP_OK;
}

/* Build the synced sub-tree from the document files in the working
 * dir. Only documents are synced, so everything else in base (the
 * current sub-tree: folders, images, README files...) is kept as it
 * is; its documents are replaced by the ones on the card. Returns the
 * tree oid (zero-cleared when the tree ends up empty). */
static esp_err_t build_worktree_subtree(const git_oid *base, git_oid *out, int *out_nfiles)
{
    git_oid_clear(out);
    *out_nfiles = 0;

    git_tree t = {};
    esp_err_t ret = git_tree_load(base, &t);    /* zero oid = empty tree */
    if (ret != ESP_OK) return ret;
    for (int i = t.count - 1; i >= 0; i--) {
        const git_tree_entry *e = &t.entries[i];
        if (synced_entry(e)) git_tree_set(&t, e->mode, e->name, NULL);   /* remove */
    }

    sd_card_file_entry_t *entries = NULL;
    int count = 0;
    esp_err_t lr = sd_card_list_dir(s_cfg.local_path, &entries, &count);
    if (lr != ESP_OK) { git_tree_free(&t); return lr; }
    for (int i = 0; i < count; i++) {
        if (entries[i].is_dir || !is_doc(entries[i].name)) continue;

        char path[400];
        joinp(path, sizeof(path), s_cfg.local_path, entries[i].name);
        git_oid blob;
        ret = git_odb_write_file(GIT_OBJ_BLOB, path, &blob);
        if (ret == ESP_ERR_NOT_FOUND) { ret = ESP_OK; continue; }
        if (ret != ESP_OK) break;

        if (!git_tree_set(&t, 0100644, entries[i].name, &blob)) { ret = ESP_ERR_NO_MEM; break; }
        (*out_nfiles)++;
    }
    sd_card_free_list(entries);

    if (ret == ESP_OK && t.count > 0)
        ret = git_tree_write(&t, out);
    git_tree_free(&t);
    return ret;
}

/* Materialise a flat tree's documents into the working dir; delete
 * local document files not present in the tree. Every file is tried;
 * the result is an error if any of them could not be written or
 * deleted, since the working dir then does not match the tree.
 * Entries synced_entry() refuses (a name FAT cannot store, a symlink
 * or submodule named like a document) are counted in *out_skipped,
 * not treated as errors: they could never be written, and failing on
 * them would block the project's sync for good. */
static esp_err_t checkout_subtree(const git_oid *tree_oid, int *out_written,
                                  int *out_skipped)
{
    if (out_written) *out_written = 0;
    if (out_skipped) *out_skipped = 0;

    git_tree t = {};
    esp_err_t ret = git_tree_load(tree_oid, &t);
    if (ret != ESP_OK) return ret;

    for (int i = 0; i < t.count; i++) {
        const git_tree_entry *e = &t.entries[i];
        if ((e->mode & 040000) == 040000) continue;    /* skip sub-dirs */
        /* Only documents are synced. Anything else in the repository
         * (images, README files, a git.cfg) stays in Git: writing it
         * into the project folder could overwrite the device's own
         * git.cfg or files the sync never commits back. */
        if (!is_doc(e->name)) continue;
        if (!synced_entry(e)) {
            ESP_LOGW(TAG, "checkout: skipping %s (not a plain file, or a name "
                          "the SD card cannot store)", e->name);
            if (out_skipped) (*out_skipped)++;
            continue;
        }

        char path[400];
        joinp(path, sizeof(path), s_cfg.local_path, e->name);

        /* Skip the write if the on-disk content already matches. */
        git_oid cur;
        if (git_odb_hash_file(GIT_OBJ_BLOB, path, &cur) == ESP_OK &&
            git_oid_eq(&cur, &e->oid))
            continue;
        git_obj_type ot;
        if (git_odb_read_type(&e->oid, &ot, NULL) != ESP_OK || ot != GIT_OBJ_BLOB ||
            git_odb_read_to_file(&e->oid, NULL, path) != ESP_OK) {
            ESP_LOGE(TAG, "checkout: cannot write %s", path);
            ret = ESP_FAIL;
            continue;
        }
        if (out_written) (*out_written)++;
    }

    /* Delete local document files that are gone from the tree. */
    sd_card_file_entry_t *entries = NULL;
    int count = 0;
    esp_err_t lr = sd_card_list_dir(s_cfg.local_path, &entries, &count);
    if (lr == ESP_OK) {
        for (int i = 0; i < count; i++) {
            if (entries[i].is_dir || !is_doc(entries[i].name)) continue;
            if (git_tree_get(&t, entries[i].name)) continue;
            char path[400];
            joinp(path, sizeof(path), s_cfg.local_path, entries[i].name);
            if (sd_card_delete_file(path) != ESP_OK) {
                ESP_LOGE(TAG, "checkout: cannot delete %s", path);
                ret = ESP_FAIL;
            }
        }
        sd_card_free_list(entries);
    } else {
        ret = lr;
    }

    git_tree_free(&t);
    return ret;
}

/* Turn merge results into a stored flat tree. */
static esp_err_t results_to_tree(const git_merge_path_result *res, int n, git_oid *out)
{
    git_tree t = {};
    for (int i = 0; i < n; i++) {
        if (git_oid_is_zero(&res[i].oid)) continue;      /* deleted */
        if (!git_tree_set(&t, res[i].mode, res[i].name, &res[i].oid)) {
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
    if (!commit_oid || git_oid_is_zero(commit_oid)) { git_oid_clear(out_tree); return ESP_OK; }
    git_commit c;
    esp_err_t ret = git_commit_load(commit_oid, &c);
    if (ret != ESP_OK) return ret;
    *out_tree = c.tree;
    git_commit_free(&c);
    return ESP_OK;
}

/* Subtree oid for a remote path within a commit's tree. */
static esp_err_t subtree_for_commit_at(const char *remote_path,
                                       const git_oid *commit_oid, git_oid *out)
{
    git_oid root;
    esp_err_t ret = commit_tree_oid(commit_oid, &root);
    if (ret != ESP_OK) return ret;
    if (git_oid_is_zero(&root)) { git_oid_clear(out); return ESP_OK; }
    ret = git_tree_subtree_oid(&root, remote_path, out);
    if (ret == ESP_ERR_NOT_FOUND) { git_oid_clear(out); return ESP_OK; }
    return ret;
}

/* Subtree oid for the syncing project's remote path. */
static esp_err_t subtree_for_commit(const git_oid *commit_oid, git_oid *out)
{
    return subtree_for_commit_at(s_cfg.remote_path, commit_oid, out);
}

/* ---- the sync itself ------------------------------------------- */

/* Update a ref; on failure report it (set_error()) and
 * return false. The refs are what the next sync starts from, so a lost
 * update must not pass silently. */
static bool ref_write(const char *ref, const git_oid *oid)
{
    if (git_ref_write(ref, oid) == ESP_OK) return true;
    ESP_LOGE(TAG, "cannot write ref %s", ref);
    set_error("Cannot write to the SD card");
    return false;
}

struct sync_stats {
    bool committed_local;
    bool fast_forwarded;
    bool rebased;
    bool pushed;
    int  nconflicts;
    int  files_out;
    int  files_skipped;
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
    git_repo_clean_tmp();
    if (git_head_set_symbolic(localref) != ESP_OK) {
        set_error("Cannot write to the SD card");
        return ESP_FAIL;
    }

    char host[96];
    remote_host(host, sizeof(host));
    char msg[160];

    /* 1. advertise remote refs */
    snprintf(msg, sizeof(msg), "Contacting %s...", host);
    notify(GIT_SYNC_IN_PROGRESS, msg);
    git_oid remote_sha;
    git_oid_clear(&remote_sha);
    bool remote_has_branch = false;
    esp_err_t ret = git_http_find_ref(&remote, "git-upload-pack", server_ref,
                                      &remote_sha, &remote_has_branch);
    if (ret == ESP_ERR_INVALID_STATE) {
        snprintf(msg, sizeof(msg), "%s rejected the login (check token)", host);
        set_error(msg);
        return ret;
    }
    if (ret != ESP_OK) {
        snprintf(msg, sizeof(msg), "Cannot reach %s", host);
        set_error(msg);
        return ret;
    }

    /* git_ref_read() leaves *out untouched on failure (ref file does
     * not exist yet -- the very first sync of a repo), so local_head
     * must start zeroed: step 5 below reads it unconditionally
     * (subtree_for_commit(&local_head, ...)), regardless of whether
     * have_head is true, and an uninitialized oid there would compare
     * as some garbage non-zero SHA instead of "no commit". */
    git_oid local_head;
    git_oid_clear(&local_head);
    esp_err_t rr = git_ref_read(localref, &local_head);
    if (rr != ESP_OK && rr != ESP_ERR_NOT_FOUND) {
        /* Not "no commit yet": treating it so would start over as a
         * first sync. */
        set_error("Cannot read the local Git history");
        return rr;
    }
    bool have_head = rr == ESP_OK && !git_oid_is_zero(&local_head);

    /* 2. initial clone */
    if (!have_head && remote_has_branch) {
        snprintf(msg, sizeof(msg), "Cloning history from %s...", host);
        notify(GIT_SYNC_IN_PROGRESS, msg);
        char pack[160];
        snprintf(pack, sizeof(pack), "%s/tmp/fetch.pack", git_repo_gitdir());
        ret = git_http_fetch_pack(&remote, &remote_sha, 1, NULL, 0, pack);
        if (ret != ESP_OK) { set_error("Fetch failed"); return ret; }
        FILE *fp = fopen(pack, "rb");
        ret = fp ? git_pack_unpack_file(fp) : ESP_FAIL;
        if (fp) fclose(fp);
        remove(pack);
        if (ret != ESP_OK) { set_error("Cannot unpack fetched history"); return ret; }

        if (!ref_write(remoteref, &remote_sha)) return ESP_FAIL;

        /* Documents already on the card were never committed: checking
         * the server's tree out over them would delete or overwrite
         * them. Then the local branch is left unborn, step 3 commits
         * the card's documents as a commit of their own, and step 4
         * merges that with the server's history like any other local
         * commit (files on one side are kept, a file on both sides is
         * merged with conflict markers). */
        bool have_local_docs = false;
        if (worktree_has_docs(&have_local_docs) != ESP_OK) {
            set_error("Cannot read working tree");
            return ESP_FAIL;
        }
        if (!have_local_docs) {
            if (!ref_write(localref, &remote_sha)) return ESP_FAIL;
            local_head = remote_sha;
            have_head = true;

            git_oid sub;
            ret = subtree_for_commit(&local_head, &sub);
            if (ret == ESP_OK) ret = checkout_subtree(&sub, NULL, &st->files_skipped);
            if (ret != ESP_OK) {
                /* Start over as a first sync next time. */
                git_ref_delete(localref);
                set_error("Cannot write files to the SD card");
                return ret;
            }
        }
    }

    /* 3. local commit from the working tree */
    notify(GIT_SYNC_IN_PROGRESS, "Committing local changes...");
    ensure_time();

    git_oid cur_sub;
    ret = subtree_for_commit(have_head ? &local_head : NULL, &cur_sub);
    if (ret != ESP_OK) { set_error("Cannot read current tree"); return ret; }

    git_oid ours_sub;
    int nfiles = 0;
    ret = build_worktree_subtree(&cur_sub, &ours_sub, &nfiles);
    if (ret != ESP_OK) { set_error("Cannot read working tree"); return ret; }

    if (!git_oid_eq(&ours_sub, &cur_sub)) {
        git_oid base_root;
        ret = commit_tree_oid(have_head ? &local_head : NULL, &base_root);
        if (ret != ESP_OK) { set_error("Cannot read current tree"); return ret; }
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

        if (!ref_write(localref, &newc)) return ESP_FAIL;
        local_head = newc;
        have_head = true;
        st->committed_local = true;
        ESP_LOGI(TAG, "local commit %s", newc.hex);
    }

    /* The commit that matches the files on the card, for rolling back
     * if the integrated tree cannot be written out (step 5). */
    git_oid committed_head = local_head;

    /* 4. integrate remote */
    bool need_integrate = remote_has_branch &&
                          !git_oid_eq(&remote_sha, &local_head) &&
                          !git_is_ancestor(&remote_sha, &local_head);

    if (need_integrate) {
        if (!git_odb_exists(&remote_sha)) {
            snprintf(msg, sizeof(msg), "Fetching commits from %s...", host);
            notify(GIT_SYNC_IN_PROGRESS, msg);
            git_oid haves[16];
            int nh = 0;
            git_oid *chain = NULL;
            int nchain = 0;
            if (git_commit_list_linear(NULL, &local_head, 16, &chain, &nchain) == ESP_OK) {
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
            if (ret != ESP_OK) {
                snprintf(msg, sizeof(msg), "Cannot unpack commits from %s", host);
                set_error(msg);
                return ret;
            }
        }

        git_oid mb;
        ret = git_merge_base(&local_head, &remote_sha, &mb);
        if (ret != ESP_OK) { set_error("Cannot compare with the server's history"); return ret; }

        if (git_oid_eq(&mb, &remote_sha)) {
            /* remote tip is already an ancestor -- we are ahead, nothing
             * to integrate; the push below fast-forwards the remote. */
            ESP_LOGI(TAG, "local ahead of remote, no integrate needed");
        } else if (git_oid_eq(&mb, &local_head)) {
            /* fast-forward */
            local_head = remote_sha;
            if (!ref_write(localref, &local_head)) return ESP_FAIL;
            st->fast_forwarded = true;
            ESP_LOGI(TAG, "fast-forward to %s", remote_sha.hex);
        } else {
            notify(GIT_SYNC_IN_PROGRESS, "Rebasing local commits...");
            git_oid *commits = NULL;
            int ncommits = 0;
            const git_oid *mbp = git_oid_is_zero(&mb) ? NULL : &mb;
            ret = git_commit_list_linear(mbp, &local_head, 0, &commits, &ncommits);
            if (ret != ESP_OK) {
                set_error(ret == ESP_ERR_NO_MEM ? "Out of memory listing local commits"
                                                : "Cannot read the local Git history");
                return ret;
            }

            git_oid newbase = remote_sha;
            char ident[224];
            make_ident(ident, sizeof(ident));

            for (int i = 0; i < ncommits; i++) {
                git_commit cc;
                if (git_commit_load(&commits[i], &cc) != ESP_OK) { ret = ESP_FAIL; break; }
                git_oid cparent = (cc.nparents > 0) ? cc.parents[0] : (git_oid){0};

                git_oid base_sub, ours_sub2, theirs_sub;
                ret = subtree_for_commit(git_oid_is_zero(&cparent) ? NULL : &cparent, &base_sub);
                if (ret == ESP_OK) ret = subtree_for_commit(&newbase, &ours_sub2);
                if (ret == ESP_OK) ret = subtree_for_commit(&commits[i], &theirs_sub);
                if (ret != ESP_OK) { git_commit_free(&cc); break; }

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
                ret = commit_tree_oid(&newbase, &nb_root);
                if (ret != ESP_OK) { git_commit_free(&cc); break; }
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
            if (!ref_write(localref, &local_head)) return ESP_FAIL;
            st->rebased = true;
        }
    } else if (remote_has_branch && git_oid_eq(&remote_sha, &local_head)) {
        if (!ref_write(remoteref, &remote_sha)) return ESP_FAIL;
    }

    /* 5. checkout the integrated tree */
    {
        git_oid sub;
        ret = subtree_for_commit(&local_head, &sub);
        if (ret == ESP_OK) {
            notify(GIT_SYNC_IN_PROGRESS, "Updating working files...");
            ret = checkout_subtree(&sub, &st->files_out, &st->files_skipped);
        }
        if (ret != ESP_OK) {
            /* Otherwise the next sync would commit the old files on
             * top of the integrated history and revert the server's
             * changes. Going back to the commit of the card's files
             * makes it integrate again instead. */
            if (!git_oid_is_zero(&committed_head))
                git_ref_write(localref, &committed_head);
            else
                git_ref_delete(localref);
            set_error("Cannot write files to the SD card");
            return ret;
        }
    }

    /* 6. push */
    bool want_push = (dir != GIT_SYNC_PULL);
    bool have_remote_target = (s_cfg.repo_url[0] != '\0');

    /* What the push would be layered on top of: the remote's current
     * tip, or the zero oid if the branch does not exist there yet (a
     * brand-new / empty remote repo) -- git's smart-HTTP protocol
     * represents "create this ref" with an all-zero old-oid. Comparing
     * against &local_head itself in that case (as this used to) is
     * trivially always "equal" and silently skips every push to an
     * empty remote, no matter what is committed locally. */
    git_oid remote_baseline;
    if (remote_has_branch) remote_baseline = remote_sha;
    else git_oid_clear(&remote_baseline);

    if (want_push && have_remote_target && !git_oid_eq(&local_head, &remote_baseline)) {

        if (remote_has_branch && !git_is_ancestor(&remote_sha, &local_head)) {
            snprintf(msg, sizeof(msg), "Push blocked: %s changed during sync -- retry", host);
            set_error(msg);
            return ESP_FAIL;
        }

        snprintf(msg, sizeof(msg), "Pushing to %s...", host);
        notify(GIT_SYNC_IN_PROGRESS, msg);
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

        /* A connection that breaks while sending (a weak WiFi signal)
         * is retried. If the server did apply a push whose answer was
         * lost, the retry is refused as stale and the next sync finds
         * the server up to date. */
        git_buf report = GIT_BUF_INIT;
        for (int attempt = 1;; attempt++) {
            ret = git_http_push(&remote, server_ref, &remote_baseline, &local_head, pack, &report);
            if (ret != ESP_ERR_TIMEOUT || attempt >= 3) break;
            ESP_LOGW(TAG, "push attempt %d lost the connection, retrying", attempt);
            snprintf(msg, sizeof(msg), "Connection lost, pushing to %s again...", host);
            notify(GIT_SYNC_IN_PROGRESS, msg);
            git_buf_free(&report);
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
        remove(pack);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "push report: %.*s", (int)report.len, (char *)report.data);
            git_buf_free(&report);
            if (ret == ESP_ERR_TIMEOUT)
                snprintf(msg, sizeof(msg), "Connection to %s lost while pushing", host);
            else
                snprintf(msg, sizeof(msg), "%s rejected the push", host);
            set_error(msg);
            return ret;
        }
        git_buf_free(&report);

        /* The push went through; without the record the files only look
         * not pushed (Del refuses them) until the next sync. */
        if (!ref_write(remoteref, &local_head)) return ESP_FAIL;
        st->pushed = true;
        ESP_LOGI(TAG, "pushed %s", local_head.hex);
    }

    return ESP_OK;
}

/* ---- task ------------------------------------------------------- */

static void run_sync(git_sync_direction_t dir)
{
    if (!wifi_manager_is_connected()) {
        set_error("WiFi not connected");
        return;
    }

    /* Modem sleep makes a weak link drop more packets; keep the radio
     * awake for the duration of the sync. */
    wifi_ps_type_t old_ps = WIFI_PS_MIN_MODEM;
    bool ps_saved = esp_wifi_get_ps(&old_ps) == ESP_OK;
    if (ps_saved) esp_wifi_set_ps(WIFI_PS_NONE);

    sync_stats st;
    esp_err_t ret = do_sync(dir, &st);

    if (ps_saved) esp_wifi_set_ps(old_ps);

    git_repo_close();

    if (ret == ESP_OK) {
        char summary[128];
        size_t pos = 0;
        pos += snprintf(summary + pos, sizeof(summary) - pos,
                        st.nconflicts ? "Sync done, %d conflict(s)" : "Sync complete",
                        st.nconflicts);
        const char *parts[6];
        int np = 0;
        char b1[24], b2[24], b3[32];
        if (st.committed_local) parts[np++] = "committed";
        if (st.fast_forwarded)  parts[np++] = "fast-forward";
        if (st.rebased)         parts[np++] = "rebased";
        if (st.files_out) { snprintf(b1, sizeof(b1), "%d file(s) updated", st.files_out); parts[np++] = b1; }
        if (st.files_skipped) {
            snprintf(b3, sizeof(b3), "%d not copied to the card", st.files_skipped);
            parts[np++] = b3;
        }
        if (st.pushed) { snprintf(b2, sizeof(b2), "pushed"); parts[np++] = b2; }
        if (np) {
            pos += snprintf(summary + pos, sizeof(summary) - pos, " (");
            for (int i = 0; i < np && pos < sizeof(summary); i++)
                pos += snprintf(summary + pos, sizeof(summary) - pos, "%s%s",
                                i ? ", " : "", parts[i]);
            snprintf(summary + pos, sizeof(summary) - pos, ")");
        }
        notify(GIT_SYNC_SUCCESS, summary);
    } else if (s_state != GIT_SYNC_ERROR) {
        set_error("Sync failed");
    }
}

static void sync_worker(void *arg)
{
    (void)arg;
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        run_sync(s_req_dir);
    }
}

/* ---- config --------------------------------------------------- */

static void trim(char *s)
{
    size_t n = strlen(s);
    while (n && (s[n - 1] == '\r' || s[n - 1] == '\n' || s[n - 1] == ' ' || s[n - 1] == '\t'))
        s[--n] = '\0';
}

static void parse_config(git_cfg_t *c, const char *data, const char *dir)
{
    memset(c, 0, sizeof(*c));
    strlcpy(c->branch, "main", sizeof(c->branch));
    strlcpy(c->local_path, dir, sizeof(c->local_path));

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

        if (!strcmp(key, "repo_url"))          strlcpy(c->repo_url, val, sizeof(c->repo_url));
        else if (!strcmp(key, "branch"))       strlcpy(c->branch, val, sizeof(c->branch));
        else if (!strcmp(key, "token"))        strlcpy(c->token, val, sizeof(c->token));
        else if (!strcmp(key, "username"))     strlcpy(c->username, val, sizeof(c->username));
        else if (!strcmp(key, "author_name"))  strlcpy(c->author_name, val, sizeof(c->author_name));
        else if (!strcmp(key, "author_email")) strlcpy(c->author_email, val, sizeof(c->author_email));
        else if (!strcmp(key, "path")) {
            strlcpy(c->remote_path, val, sizeof(c->remote_path));
            /* normalise: no leading '/', exactly one trailing '/' */
            char *q = c->remote_path;
            while (*q == '/') memmove(q, q + 1, strlen(q));
            size_t n = strlen(c->remote_path);
            while (n && c->remote_path[n - 1] == '/') c->remote_path[--n] = '\0';
            if (n && n + 1 < sizeof(c->remote_path)) {
                c->remote_path[n] = '/';
                c->remote_path[n + 1] = '\0';
            }
        }
    }

    if (c->branch[0] == '\0') strlcpy(c->branch, "main", sizeof(c->branch));
    /* The branch name becomes a path under .git/refs: refuse one that
     * is not a valid name (and would reach elsewhere). */
    c->bad_branch = !git_branch_name_ok(c->branch);
    c->configured = (c->repo_url[0] != '\0') && !c->bad_branch;
}

/* "<dir>/git.cfg" */
static void cfg_path(const char *dir, char *out, size_t osz)
{
    joinp(out, osz, dir, "git.cfg");
}

/* Load <dir>/git.cfg into *c. Returns c->configured; a missing file
 * leaves an unconfigured config for dir. */
static bool load_config(git_cfg_t *c, const char *dir)
{
    char path[200];
    cfg_path(dir, path, sizeof(path));
    char *data = NULL;
    size_t len = 0;
    if (sd_card_read_file(path, &data, &len) == ESP_OK && data) {
        parse_config(c, data, dir);
        free(data);
    } else {
        parse_config(c, "", dir);
    }
    return c->configured;
}

/* ---- public API --------------------------------------------- */

extern "C" esp_err_t git_sync_init(void)
{
    /* Nothing to load up front: each project folder carries its own
     * git.cfg, read when that project is synced or checked. */
    return ESP_OK;
}

/* ---- Draftbox --------------------------------------------- */

/* Set while git_sync_draftbox_connect() runs (on a worker task), so a
 * sync cannot start with a configuration that is being replaced. */
static volatile bool s_draftbox_busy = false;

/* Keys that describe the remote repository. Connecting to a Draftbox
 * repository replaces all of them with what the server returned:
 * repo_url, token, username and branch (when sent). path is dropped --
 * a sub-directory chosen for the previous repository would not exist
 * on the new one -- so the whole repository is synced. */
static bool is_repo_key(const char *line)
{
    static const char *const keys[] = { "repo_url", "token", "username", "branch", "path" };
    while (*line == ' ' || *line == '\t') line++;
    for (const char *k : keys) {
        size_t n = strlen(k);
        if (strncmp(line, k, n) != 0) continue;
        const char *p = line + n;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '=') return true;
    }
    return false;
}

/* Normalise the user's site entry into a bare "host[:port][/prefix]". */
static bool normalise_site(const char *in, char *out, size_t out_sz)
{
    while (*in == ' ' || *in == '\t') in++;
    if (!strncasecmp(in, "https://", 8)) in += 8;
    else if (!strncasecmp(in, "http://", 7)) in += 7;
    strlcpy(out, in, out_sz);
    size_t n = strlen(out);
    while (n && (out[n - 1] == '/' || out[n - 1] == ' ' || out[n - 1] == '\t'))
        out[--n] = '\0';
    if (n == 0 || out[0] == '/') return false;
    for (const char *p = out; *p; p++) {
        unsigned char c = (unsigned char)*p;
        if (c <= 0x20 || c >= 0x7f || c == '"' || c == '\\' || c == '?' || c == '#' || c == '@')
            return false;
    }
    return true;
}

static esp_err_t draftbox_connect(const char *dir, const char *site, const char *otp,
                                  char *msg, size_t msg_len)
{
    char cfgfile[200];
    cfg_path(dir, cfgfile, sizeof(cfgfile));
    char host[128];
    if (!normalise_site(site ? site : "", host, sizeof(host))) {
        snprintf(msg, msg_len, "invalid site name");
        return ESP_ERR_INVALID_ARG;
    }
    char pw[9];
    size_t n = 0;
    for (const char *p = otp ? otp : ""; *p; p++) {
        if (*p == ' ' || *p == '-') continue;
        if (*p < '0' || *p > '9' || n >= 8) { n = 0; break; }
        pw[n++] = *p;
    }
    if (n != 8) {
        snprintf(msg, msg_len, "password must be 8 digits");
        return ESP_ERR_INVALID_ARG;
    }
    pw[8] = '\0';

    if (!wifi_manager_is_connected()) {
        snprintf(msg, msg_len, "connect WiFi first");
        return ESP_ERR_INVALID_STATE;
    }
    /* The password works only once: make sure the result can be stored
     * before spending it. */
    if (!sd_card_is_ready()) {
        snprintf(msg, msg_len, "no SD card");
        return ESP_ERR_INVALID_STATE;
    }

    draftbox_result_t *res = (draftbox_result_t *)calloc(1, sizeof(*res));
    if (!res) {
        snprintf(msg, msg_len, "out of memory");
        return ESP_ERR_NO_MEM;
    }
    esp_err_t err = draftbox_token_exchange(host, pw, res);
    if (err != ESP_OK) {
        snprintf(msg, msg_len, "%s", res->error);
        free(res);
        return err;
    }

    /* Keep every line of the old git.cfg that does not describe the
     * remote repository (author_name, author_email, comments...). */
    std::string cfg;
    char *old = NULL;
    size_t old_len = 0;
    git_cfg_t *prev = (git_cfg_t *)calloc(1, sizeof(git_cfg_t));
    if (!prev) {
        memset(res->token, 0, sizeof(res->token));
        free(res);
        snprintf(msg, msg_len, "out of memory");
        return ESP_ERR_NO_MEM;
    }
    parse_config(prev, "", dir);
    if (sd_card_read_file(cfgfile, &old, &old_len) == ESP_OK && old) {
        parse_config(prev, old, dir);
        const char *p = old;
        const char *end = old + old_len;
        while (p < end) {
            const char *nl = (const char *)memchr(p, '\n', (size_t)(end - p));
            size_t len = nl ? (size_t)(nl - p) : (size_t)(end - p);
            std::string line(p, len);
            p += len + (nl ? 1 : 0);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty() || is_repo_key(line.c_str())) continue;
            cfg += line;
            cfg += '\n';
        }
    }
    free(old);
    cfg += "repo_url=";
    cfg += res->clone_url;
    cfg += "\ntoken=";
    cfg += res->token;
    cfg += '\n';
    if (res->user[0]) {
        cfg += "username=";
        cfg += res->user;
        cfg += '\n';
    }
    if (res->branch[0]) {
        cfg += "branch=";
        cfg += res->branch;
        cfg += '\n';
    }

    err = sd_card_write_file(cfgfile, cfg.data(), cfg.size());
    if (err != ESP_OK) {
        snprintf(msg, msg_len, "cannot write git.cfg");
    } else {
        /* The remote-tracking ref describes what is on the *previous*
         * server; git_sync_file_status() must not take it as proof that
         * a file is stored on the new one. The next sync recreates it. */
        if (prev->repo_url[0] && !prev->bad_branch &&
            strcmp(prev->repo_url, res->clone_url) != 0) {
            char refpath[400];
            snprintf(refpath, sizeof(refpath), "%s/.git/refs/remotes/origin/%s",
                     dir, prev->branch);
            if (sd_card_file_exists(refpath)) sd_card_delete_file(refpath);
        }
        ESP_LOGI(TAG, "Draftbox: %s now points at %s", cfgfile, res->clone_url);
        snprintf(msg, msg_len, "connected to %.80s%s",
                 res->repository[0] ? res->repository : res->clone_url,
                 res->read_only ? " (read-only)" : "");
    }
    /* Do not leave the secret lying around in freed heap. */
    memset(prev, 0, sizeof(*prev));
    free(prev);
    memset(res->token, 0, sizeof(res->token));
    std::fill(cfg.begin(), cfg.end(), '\0');
    free(res);
    return err;
}

extern "C" esp_err_t git_sync_draftbox_connect(const char *project_dir,
                                               const char *site, const char *otp,
                                               char *msg, size_t msg_len)
{
    if (!project_dir || !project_dir[0]) {
        snprintf(msg, msg_len, "no project selected");
        return ESP_ERR_INVALID_ARG;
    }
    if (s_state == GIT_SYNC_IN_PROGRESS || s_draftbox_busy) {
        snprintf(msg, msg_len, "sync in progress, try later");
        return ESP_ERR_INVALID_STATE;
    }
    s_draftbox_busy = true;
    esp_err_t err = draftbox_connect(project_dir, site, otp, msg, msg_len);
    s_draftbox_busy = false;
    return err;
}

extern "C" esp_err_t git_sync_start(const char *project_dir, git_sync_direction_t direction)
{
    if (s_state == GIT_SYNC_IN_PROGRESS) { set_error("Sync already in progress"); return ESP_ERR_INVALID_STATE; }
    if (s_draftbox_busy) { set_error("Draftbox connection in progress"); return ESP_ERR_INVALID_STATE; }
    if (!project_dir || !project_dir[0]) { set_error("No project selected"); return ESP_ERR_INVALID_ARG; }
    if (!load_config(&s_cfg, project_dir)) {
        set_error(s_cfg.bad_branch ? "Invalid branch name in git.cfg" : "Not configured");
        return ESP_ERR_INVALID_STATE;
    }
    ESP_LOGI(TAG, "Sync %s: %s branch=%s path=%s", project_dir, s_cfg.repo_url,
             s_cfg.branch, s_cfg.remote_path[0] ? s_cfg.remote_path : "(root)");

    s_state = GIT_SYNC_IN_PROGRESS;

    if (!s_worker) {
        if (!s_stack_buf) {
            s_stack_buf = (StackType_t *)heap_caps_malloc(GIT_SYNC_STACK_SIZE, MALLOC_CAP_SPIRAM);
            if (!s_stack_buf) { set_error("Failed to alloc sync stack"); return ESP_ERR_NO_MEM; }
        }
        s_worker = xTaskCreateStaticPinnedToCore(
            sync_worker, "git_sync", GIT_SYNC_STACK_SIZE / sizeof(StackType_t),
            NULL, 3, s_stack_buf, &s_task_tcb, 0);
        if (!s_worker) { set_error("Failed to start sync task"); return ESP_FAIL; }
    }
    s_req_dir = direction;
    xTaskNotifyGive(s_worker);
    return ESP_OK;
}

static git_sync_file_status_t file_diff_cfg(git_cfg_t *c, const char *project_dir,
                                            const char *name,
                                            const char *data, size_t len,
                                            int *added, int *deleted);

/* Lines added / deleted from the server's blob to the new content (in
 * memory, or the file at path when data is NULL). Both are streamed:
 * only a hash per line is kept. */
static void line_counts(const git_oid *old_oid, const char *data, size_t len,
                        const char *path, int *added, int *deleted)
{
    git_odb_stream s;
    if (git_odb_stream_open(&s, old_oid) != ESP_OK) return;
    if (s.type == GIT_OBJ_BLOB) {
        git_reader ra, rb;
        git_reader_odb(&ra, &s);
        git_mem_src m;
        FILE *f = NULL;
        if (data) git_reader_mem(&rb, &m, data, len);
        else if ((f = fopen(path, "rb")) != NULL) git_reader_file(&rb, f);
        int a, d;
        if ((data || f) && git_diff_line_counts(&ra, &rb, &a, &d) == ESP_OK) {
            *added = a;
            *deleted = d;
        }
        if (f) fclose(f);
    }
    git_odb_stream_close(&s);
}

extern "C" git_sync_file_status_t git_sync_file_diff(const char *project_dir,
                                                    const char *name,
                                                    const char *data, size_t len,
                                                    int *added, int *deleted)
{
    if (added) *added = -1;
    if (deleted) *deleted = -1;
    if (!project_dir || !project_dir[0]) return GIT_SYNC_FILE_NOT_CONFIGURED;
    /* The sync task owns the repository while it runs, and it can
     * rewrite both the file and the refs. Both this check and
     * git_sync_start() run on the UI task, so the state cannot change
     * under us. */
    if (s_state == GIT_SYNC_IN_PROGRESS) return GIT_SYNC_FILE_BUSY;
    git_cfg_t *c = (git_cfg_t *)calloc(1, sizeof(git_cfg_t));
    if (!c) return GIT_SYNC_FILE_ERROR;
    git_sync_file_status_t result = file_diff_cfg(c, project_dir, name, data, len,
                                                  added, deleted);
    memset(c, 0, sizeof(*c));
    free(c);
    return result;
}

static git_sync_file_status_t file_diff_cfg(git_cfg_t *c, const char *project_dir,
                                            const char *name,
                                            const char *data, size_t len,
                                            int *added, int *deleted)
{
    if (!load_config(c, project_dir)) return GIT_SYNC_FILE_NOT_CONFIGURED;
    if (!name || !*name || strchr(name, '/') || !is_doc(name)) return GIT_SYNC_FILE_NOT_PUSHED;

    /* Look for the remote-tracking ref first: git_repo_open() would
     * create an empty repository on a card that was never synced. */
    char remoteref[96];
    snprintf(remoteref, sizeof(remoteref), "refs/remotes/origin/%s", c->branch);
    char refpath[400];
    snprintf(refpath, sizeof(refpath), "%s/.git/%s", c->local_path, remoteref);
    if (!sd_card_file_exists(refpath)) return GIT_SYNC_FILE_NOT_PUSHED;

    char path[400];
    joinp(path, sizeof(path), c->local_path, name);
    git_oid work_blob;
    if ((data ? git_odb_hash(GIT_OBJ_BLOB, data, len, &work_blob)
              : git_odb_hash_file(GIT_OBJ_BLOB, path, &work_blob)) != ESP_OK)
        return GIT_SYNC_FILE_ERROR;

    git_sync_file_status_t result = GIT_SYNC_FILE_ERROR;
    if (git_repo_open(c->local_path) != ESP_OK) return result;
    git_oid pushed, sub;
    if (git_ref_read(remoteref, &pushed) != ESP_OK) {
        result = GIT_SYNC_FILE_NOT_PUSHED;
    } else if (subtree_for_commit_at(c->remote_path, &pushed, &sub) == ESP_OK) {
        if (git_oid_is_zero(&sub)) {
            result = GIT_SYNC_FILE_NOT_PUSHED;
        } else {
            git_tree t = {};
            if (git_tree_load(&sub, &t) == ESP_OK) {
                const git_tree_entry *e = git_tree_get(&t, name);
                if (!e)                              result = GIT_SYNC_FILE_NOT_PUSHED;
                else if (git_oid_eq(&e->oid, &work_blob)) result = GIT_SYNC_FILE_PUSHED;
                else                                 result = GIT_SYNC_FILE_MODIFIED;
                if (result == GIT_SYNC_FILE_MODIFIED && added && deleted)
                    line_counts(&e->oid, data, len, path, added, deleted);
                git_tree_free(&t);
            }
        }
    }
    git_repo_close();
    ESP_LOGI(TAG, "file status %s: %d", name, (int)result);
    return result;
}

static void project_status_cfg(git_cfg_t *c, const char *project_dir,
                               git_sync_project_status_t *st)
{
    if (!load_config(c, project_dir)) return;
    st->configured = true;

    sd_card_file_entry_t *entries = NULL;
    int count = 0;
    if (sd_card_list_dir(project_dir, &entries, &count) != ESP_OK) { st->error = true; return; }

    char remoteref[96];
    snprintf(remoteref, sizeof(remoteref), "refs/remotes/origin/%s", c->branch);
    char refpath[400];
    snprintf(refpath, sizeof(refpath), "%s/.git/%s", project_dir, remoteref);

    git_tree t = {};
    bool have_tree = false;
    if (sd_card_file_exists(refpath) && git_repo_open(project_dir) == ESP_OK) {
        git_oid pushed, sub;
        if (git_ref_read(remoteref, &pushed) == ESP_OK &&
            subtree_for_commit_at(c->remote_path, &pushed, &sub) == ESP_OK) {
            if (git_oid_is_zero(&sub) || git_tree_load(&sub, &t) == ESP_OK)
                have_tree = true;
            else
                st->error = true;
        } else {
            st->error = true;
        }
    }
    st->never_pushed = !have_tree && !st->error;

    int seen_on_server = 0;
    for (int i = 0; i < count; i++) {
        if (entries[i].is_dir || !is_doc(entries[i].name)) continue;
        st->files++;
        const git_tree_entry *e = have_tree ? git_tree_get(&t, entries[i].name) : NULL;
        if (!e) { st->not_pushed++; continue; }
        seen_on_server++;
        char path[400];
        joinp(path, sizeof(path), project_dir, entries[i].name);
        git_oid blob;
        if (git_odb_hash_file(GIT_OBJ_BLOB, path, &blob) != ESP_OK) { st->error = true; continue; }
        if (git_oid_eq(&e->oid, &blob)) st->pushed++;
        else st->modified++;
    }
    if (have_tree) {
        int server_docs = 0;
        for (int i = 0; i < t.count; i++) {
            if ((t.entries[i].mode & 040000) != 040000 && is_doc(t.entries[i].name))
                server_docs++;
        }
        st->deleted_locally = server_docs - seen_on_server;
        if (st->deleted_locally < 0) st->deleted_locally = 0;
    }
    git_tree_free(&t);
    git_repo_close();
    sd_card_free_list(entries);
}

extern "C" void git_sync_project_status(const char *project_dir,
                                        git_sync_project_status_t *st)
{
    memset(st, 0, sizeof(*st));
    if (!project_dir || !project_dir[0]) return;
    if (s_state == GIT_SYNC_IN_PROGRESS) {
        st->busy = true;
        st->configured = git_sync_is_configured(project_dir);
        return;
    }
    git_cfg_t *c = (git_cfg_t *)calloc(1, sizeof(git_cfg_t));
    if (!c) { st->error = true; return; }
    project_status_cfg(c, project_dir, st);
    memset(c, 0, sizeof(*c));
    free(c);
}

extern "C" git_sync_file_status_t git_sync_file_status(const char *project_dir,
                                                      const char *name)
{
    return git_sync_file_diff(project_dir, name, NULL, 0, NULL, NULL);
}

extern "C" git_sync_state_t git_sync_get_state(void) { return s_state; }
extern "C" void git_sync_set_callback(git_sync_callback_t cb) { s_callback = cb; }
extern "C" bool git_sync_is_configured(const char *project_dir)
{
    if (!project_dir || !project_dir[0]) return false;
    git_cfg_t *c = (git_cfg_t *)calloc(1, sizeof(git_cfg_t));
    if (!c) return false;
    bool ok = load_config(c, project_dir);
    memset(c, 0, sizeof(*c));
    free(c);
    return ok;
}

extern "C" bool git_sync_get_remote(const char *project_dir, char *host, size_t host_sz,
                                    char *repo, size_t repo_sz,
                                    char *branch, size_t branch_sz)
{
    if (host_sz) host[0] = '\0';
    if (repo_sz) repo[0] = '\0';
    if (branch_sz) branch[0] = '\0';
    if (!project_dir || !project_dir[0]) return false;
    git_cfg_t *c = (git_cfg_t *)calloc(1, sizeof(git_cfg_t));
    if (!c) return false;
    bool ok = load_config(c, project_dir);
    if (ok) {
        remote_host_of(c->repo_url, host, host_sz);
        repo_name_of(c->repo_url, repo, repo_sz);
        strlcpy(branch, c->branch, branch_sz);
    }
    memset(c, 0, sizeof(*c));
    free(c);
    return ok;
}

extern "C" const char *git_sync_get_project_dir(void) { return s_cfg.local_path; }
extern "C" const char *git_sync_get_last_error(void) { return s_last_error; }
