/*
 * Online demo: Git sync. Nothing goes on the network.
 *
 * A project counts as configured when its git.cfg names a repo_url, as
 * on the device. "Synchronizing" walks through the usual progress
 * messages and then copies the project's documents to a stand-in for
 * the server, <project>/.git/demo-server/, so the file status checks
 * work afterwards: Delete is allowed for pushed files, Alt+I shows
 * the lines changed since the last sync, and a project with
 * everything pushed can be deleted. Draftbox is not available.
 */

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include <dirent.h>
#include <sys/stat.h>

#include "git_sync.h"
#include "lvgl.h"
#include "sd_card.h"
#include "wifi_manager.h"

namespace {

struct cfg_t {
    bool configured = false;
    std::string repo_url;
    std::string branch = "main";
};

git_sync_state_t g_state = GIT_SYNC_IDLE;
git_sync_callback_t g_cb;
char g_project[128];
char g_last_error[128];
int g_step;
lv_timer_t *g_timer;

bool has_suffix(const std::string &s, const char *suf)
{
    size_t n = strlen(suf);
    return s.size() >= n && s.compare(s.size() - n, n, suf) == 0;
}

bool is_doc(const std::string &name)
{
    return !name.empty() && name[0] != '.' &&
           (has_suffix(name, ".md") || has_suffix(name, ".fountain") ||
            has_suffix(name, ".txt"));
}

cfg_t load_cfg(const char *dir)
{
    cfg_t c;
    std::string path = std::string(dir) + "/git.cfg";
    char *data = nullptr;
    size_t len = 0;
    if (sd_card_read_file(path.c_str(), &data, &len) != ESP_OK || !data) return c;
    std::string s(data, len);
    free(data);
    size_t pos = 0;
    while (pos < s.size()) {
        size_t nl = s.find('\n', pos);
        std::string line = s.substr(pos, nl == std::string::npos ? std::string::npos : nl - pos);
        pos = nl == std::string::npos ? s.size() : nl + 1;
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
        size_t eq = line.find('=');
        if (line.empty() || line[0] == '#' || eq == std::string::npos) continue;
        std::string k = line.substr(0, eq), v = line.substr(eq + 1);
        if (k == "repo_url") c.repo_url = v;
        else if (k == "branch" && !v.empty()) c.branch = v;
    }
    c.configured = !c.repo_url.empty();
    return c;
}

std::string host_of(const std::string &url)
{
    size_t p = url.find("://");
    p = p == std::string::npos ? 0 : p + 3;
    size_t end = url.find_first_of("/?#", p);
    std::string h = url.substr(p, end == std::string::npos ? std::string::npos : end - p);
    size_t at = h.rfind('@');
    if (at != std::string::npos) h = h.substr(at + 1);
    size_t colon = h.find(':');
    if (colon != std::string::npos && h[0] != '[') h = h.substr(0, colon);
    return h.empty() ? "server" : h;
}

std::string repo_of(const std::string &url)
{
    size_t p = url.find("://");
    p = p == std::string::npos ? 0 : p + 3;
    p = url.find_first_of("/?#", p);
    if (p == std::string::npos) return "(server root)";
    while (p < url.size() && url[p] == '/') p++;
    std::string r = url.substr(p, url.find_first_of("?#", p) - p);
    while (!r.empty() && r.back() == '/') r.pop_back();
    if (has_suffix(r, ".git")) r.resize(r.size() - 4);
    return r.empty() ? "(server root)" : r;
}

std::string server_dir(const char *project_dir)
{
    return std::string(project_dir) + "/.git/demo-server";
}

std::vector<std::string> list_docs(const std::string &dir)
{
    std::vector<std::string> v;
    DIR *d = opendir(dir.c_str());
    if (!d) return v;
    struct dirent *e;
    while ((e = readdir(d)) != nullptr) {
        if (e->d_type != DT_DIR && is_doc(e->d_name)) v.push_back(e->d_name);
    }
    closedir(d);
    std::sort(v.begin(), v.end());
    return v;
}

bool read_all(const std::string &path, std::string *out)
{
    FILE *f = fopen(path.c_str(), "rb");
    if (!f) return false;
    out->clear();
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) out->append(buf, n);
    fclose(f);
    return true;
}

/* Lines added / deleted between two texts, counted as multisets of
 * lines (good enough for the file information page). */
void line_counts(const std::string &a, const std::string &b, int *added, int *deleted)
{
    std::map<std::string, int> m;
    auto each = [](const std::string &s, auto fn) {
        size_t pos = 0;
        while (pos < s.size()) {
            size_t nl = s.find('\n', pos);
            fn(s.substr(pos, nl == std::string::npos ? std::string::npos : nl - pos));
            pos = nl == std::string::npos ? s.size() : nl + 1;
        }
    };
    each(a, [&](const std::string &l) { m[l]--; });
    each(b, [&](const std::string &l) { m[l]++; });
    int ad = 0, de = 0;
    for (const auto &kv : m) {
        if (kv.second > 0) ad += kv.second;
        else de -= kv.second;
    }
    if (added) *added = ad;
    if (deleted) *deleted = de;
}

git_sync_file_status_t content_status(const char *project_dir, const char *name,
                                      const std::string &data,
                                      int *added, int *deleted)
{
    if (added) *added = 0;
    if (deleted) *deleted = 0;
    if (!load_cfg(project_dir).configured) return GIT_SYNC_FILE_NOT_CONFIGURED;
    if (g_state == GIT_SYNC_IN_PROGRESS) return GIT_SYNC_FILE_BUSY;
    std::string old;
    if (!read_all(server_dir(project_dir) + "/" + name, &old)) {
        line_counts("", data, added, deleted);
        return GIT_SYNC_FILE_NOT_PUSHED;
    }
    if (old == data) return GIT_SYNC_FILE_PUSHED;
    line_counts(old, data, added, deleted);
    return GIT_SYNC_FILE_MODIFIED;
}

void notify(git_sync_state_t st, const char *msg)
{
    g_state = st;
    if (st == GIT_SYNC_ERROR) snprintf(g_last_error, sizeof(g_last_error), "%s", msg);
    if (g_cb) g_cb(st, msg);
}

/* Copy the project's documents to the stand-in server; returns the
 * number of documents that changed there. */
int push_all(const char *project_dir)
{
    std::string srv = server_dir(project_dir);
    sd_card_mkdir(srv.c_str());
    int changed = 0;
    std::vector<std::string> local = list_docs(project_dir);
    for (const auto &n : local) {
        std::string data, old;
        if (!read_all(std::string(project_dir) + "/" + n, &data)) continue;
        if (read_all(srv + "/" + n, &old) && old == data) continue;
        sd_card_write_file((srv + "/" + n).c_str(), data.data(), data.size());
        changed++;
    }
    for (const auto &n : list_docs(srv)) {
        if (std::find(local.begin(), local.end(), n) == local.end()) {
            remove((srv + "/" + n).c_str());
            changed++;
        }
    }
    return changed;
}

void step_cb(lv_timer_t *t)
{
    cfg_t c = load_cfg(g_project);
    std::string host = host_of(c.repo_url);
    char msg[160];
    switch (g_step++) {
    case 0:
        snprintf(msg, sizeof(msg), "Contacting %s...", host.c_str());
        notify(GIT_SYNC_IN_PROGRESS, msg);
        break;
    case 1:
        notify(GIT_SYNC_IN_PROGRESS, "Committing local changes...");
        break;
    case 2:
        snprintf(msg, sizeof(msg), "Pushing to %s...", host.c_str());
        notify(GIT_SYNC_IN_PROGRESS, msg);
        break;
    default: {
        lv_timer_delete(t);
        g_timer = nullptr;
        int changed = push_all(g_project);
        notify(GIT_SYNC_SUCCESS, changed ? "Sync complete (committed, pushed; demo)"
                                         : "Sync complete (demo)");
        break;
    }
    }
}

}  // namespace

extern "C" {

esp_err_t git_sync_init(void)
{
    return ESP_OK;
}

esp_err_t git_sync_start(const char *project_dir, git_sync_direction_t direction)
{
    (void)direction;
    if (g_state == GIT_SYNC_IN_PROGRESS) return ESP_ERR_INVALID_STATE;
    if (!project_dir || !load_cfg(project_dir).configured) {
        notify(GIT_SYNC_ERROR, "Git sync is not configured (git.cfg)");
        return ESP_ERR_INVALID_STATE;
    }
    if (!wifi_manager_is_connected()) {
        notify(GIT_SYNC_ERROR, "WiFi is not connected");
        return ESP_ERR_INVALID_STATE;
    }
    snprintf(g_project, sizeof(g_project), "%s", project_dir);
    g_state = GIT_SYNC_IN_PROGRESS;
    g_step = 0;
    g_timer = lv_timer_create(step_cb, 700, nullptr);
    return ESP_OK;
}

git_sync_state_t git_sync_get_state(void) { return g_state; }
void git_sync_set_callback(git_sync_callback_t cb) { g_cb = cb; }

bool git_sync_is_configured(const char *project_dir)
{
    return project_dir && project_dir[0] && load_cfg(project_dir).configured;
}

bool git_sync_get_remote(const char *project_dir, char *host, size_t host_sz,
                         char *repo, size_t repo_sz, char *branch, size_t branch_sz)
{
    if (host_sz) host[0] = '\0';
    if (repo_sz) repo[0] = '\0';
    if (branch_sz) branch[0] = '\0';
    if (!project_dir || !project_dir[0]) return false;
    cfg_t c = load_cfg(project_dir);
    if (!c.configured) return false;
    snprintf(host, host_sz, "%s", host_of(c.repo_url).c_str());
    snprintf(repo, repo_sz, "%s", repo_of(c.repo_url).c_str());
    snprintf(branch, branch_sz, "%s", c.branch.c_str());
    return true;
}

const char *git_sync_get_project_dir(void) { return g_project; }
const char *git_sync_get_last_error(void) { return g_last_error; }

git_sync_file_status_t git_sync_file_status(const char *project_dir, const char *name)
{
    std::string data;
    if (!read_all(std::string(project_dir) + "/" + name, &data)) {
        if (!load_cfg(project_dir).configured) return GIT_SYNC_FILE_NOT_CONFIGURED;
        return GIT_SYNC_FILE_ERROR;
    }
    return content_status(project_dir, name, data, nullptr, nullptr);
}

git_sync_file_status_t git_sync_file_diff(const char *project_dir, const char *name,
                                          const char *data, size_t len,
                                          int *added, int *deleted)
{
    return content_status(project_dir, name, std::string(data ? data : "", data ? len : 0),
                          added, deleted);
}

void git_sync_project_status(const char *project_dir, git_sync_project_status_t *st)
{
    memset(st, 0, sizeof(*st));
    st->configured = load_cfg(project_dir).configured;
    std::vector<std::string> local = list_docs(project_dir);
    st->files = (int)local.size();
    if (!st->configured) return;
    if (g_state == GIT_SYNC_IN_PROGRESS) {
        st->busy = true;
        return;
    }
    std::string srv = server_dir(project_dir);
    std::vector<std::string> remote = list_docs(srv);
    st->never_pushed = !sd_card_is_dir(srv.c_str());
    for (const auto &n : local) {
        switch (git_sync_file_status(project_dir, n.c_str())) {
        case GIT_SYNC_FILE_PUSHED:     st->pushed++; break;
        case GIT_SYNC_FILE_MODIFIED:   st->modified++; break;
        case GIT_SYNC_FILE_NOT_PUSHED: st->not_pushed++; break;
        default:                       st->error = true; break;
        }
    }
    for (const auto &n : remote) {
        if (std::find(local.begin(), local.end(), n) == local.end()) st->deleted_locally++;
    }
}

esp_err_t git_sync_draftbox_connect(const char *project_dir, const char *site,
                                    const char *otp, char *msg, size_t msg_len)
{
    (void)project_dir; (void)site; (void)otp;
    snprintf(msg, msg_len, "Draftbox is not available in the online demo");
    return ESP_ERR_NOT_SUPPORTED;
}

}  // extern "C"
