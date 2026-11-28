/* projects.cpp -- project folders on the SD card. See projects.h. */
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <strings.h>
#include <string>
#include <vector>
#include <esp_log.h>
#include <esp_heap_caps.h>

#include "projects.h"
#include "editor.h"
#include "sd_card.h"

static const char *TAG = "Projects";


/* Project and root metadata: hidden files, so neither the file browser
 * nor Git sync ever sees them. */
#define PROJECT_META_NAME ".project.meta"
#define ROOT_META_NAME    ".projects.meta"
/* Root documents to leave outside projects; see projects.h. */
#define ROOT_IGNORE_NAME  ".projects.ignore"
/* Kept in the root by reader firmware that shares the card. */
#define ROOT_KEEP_NAME    "My Clippings.txt"

extern "C" bool project_name_char_ok(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '-' || c == '_' || c == ' ';
}

extern "C" const char *project_name_problem(const char *name)
{
    size_t n = name ? strlen(name) : 0;
    if (n == 0) return "Enter a project name";
    if (n > PROJECT_NAME_MAX) return "Project name is too long";
    if (name[0] == ' ') return "Project name must not start with a space";
    if (name[n - 1] == ' ') return "Project name must not end with a space";
    for (size_t i = 0; i < n; i++) {
        if (!project_name_char_ok(name[i]))
            return "Use letters, digits, spaces, - and _ only";
    }
    /* Windows creates this folder on removable drives. */
    if (strcasecmp(name, "System Volume Information") == 0)
        return "This name is reserved";
    return NULL;
}

static const char *mount(void)
{
    const char *mp = sd_card_get_mount_point();
    return mp ? mp : "/sdcard";
}

extern "C" void project_dir(const char *name, char *out, size_t out_sz)
{
    snprintf(out, out_sz, "%s/%s", mount(), name ? name : "");
}

/* Every entry of dir (sd_card_list_dir()); false when it cannot be
 * listed. Release *out with sd_card_free_list(). */
static bool scan_dir(const char *dir, sd_card_file_entry_t **out, int *count)
{
    return sd_card_list_dir(dir, out, count) == ESP_OK;
}

extern "C" project_name_t *project_list(int *out_count)
{
    *out_count = -1;
    int count = 0;
    sd_card_file_entry_t *e = NULL;
    if (!scan_dir(mount(), &e, &count)) return NULL;
    project_name_t *out = count ? (project_name_t *)malloc((size_t)count * sizeof(*out)) : NULL;
    if (count && !out) {
        ESP_LOGE(TAG, "project list: out of memory");
        sd_card_free_list(e);
        return NULL;
    }
    int n = 0;
    /* sd_card_list_dir() sorts case-insensitively by name. */
    for (int i = 0; out && i < count; i++) {
        if (!e[i].is_dir || project_name_problem(e[i].name)) continue;
        /* project_name_problem() limits the length: always fits */
        if (snprintf(out[n], sizeof(project_name_t), "%s", e[i].name)
                < (int)sizeof(project_name_t))
            n++;
    }
    sd_card_free_list(e);
    if (n == 0) { free(out); out = NULL; }
    *out_count = n;
    return out;
}

extern "C" bool project_any(void)
{
    int n = 0;
    project_name_t *all = project_list(&n);
    free(all);
    return n != 0;
}

extern "C" bool project_exists(const char *name)
{
    if (!name || !name[0]) return false;
    char dir[64];
    project_dir(name, dir, sizeof(dir));
    return sd_card_is_dir(dir);
}

extern "C" esp_err_t project_create(const char *name)
{
    if (project_name_problem(name)) return ESP_ERR_INVALID_ARG;
    char dir[64];
    project_dir(name, dir, sizeof(dir));
    if (sd_card_file_exists(dir)) return ESP_ERR_INVALID_STATE;
    esp_err_t err = sd_card_mkdir(dir);
    if (err == ESP_OK) ESP_LOGI(TAG, "Created project %s", name);
    return err;
}

extern "C" esp_err_t project_delete(const char *name)
{
    if (project_name_problem(name)) return ESP_ERR_INVALID_ARG;
    char dir[64];
    project_dir(name, dir, sizeof(dir));
    if (!sd_card_is_dir(dir)) return ESP_ERR_NOT_FOUND;
    esp_err_t err = sd_card_remove_tree(dir);
    ESP_LOGI(TAG, "Deleted project %s: %s", name, esp_err_to_name(err));
    return err;
}

extern "C" bool project_of_path(const char *path, char *out, size_t out_sz)
{
    if (!path) return false;
    const char *mp = mount();
    size_t ml = strlen(mp);
    if (strncmp(path, mp, ml) != 0 || path[ml] != '/') return false;
    const char *p = path + ml + 1;
    const char *slash = strchr(p, '/');
    if (!slash || strchr(slash + 1, '/')) return false;
    size_t n = (size_t)(slash - p);
    if (n == 0 || n > PROJECT_NAME_MAX || n >= out_sz) return false;
    memcpy(out, p, n);
    out[n] = '\0';
    return project_name_problem(out) == NULL;
}

typedef std::vector<std::string> name_list_t;

static void ignore_path(char *out, size_t out_sz)
{
    snprintf(out, out_sz, "%s/" ROOT_IGNORE_NAME, mount());
}

/* The ignore list; false when it exists but cannot be read. */
static bool ignore_load(name_list_t &out)
{
    out.clear();
    char path[96];
    ignore_path(path, sizeof(path));
    if (!sd_card_file_exists(path)) return true;
    char *data = NULL;
    size_t len = 0;
    esp_err_t err = sd_card_read_file(path, &data, &len);
    if (err != ESP_OK || !data) {
        ESP_LOGE(TAG, "Cannot read %s: %s", path, esp_err_to_name(err));
        free(data);
        return false;
    }
    const char *p = data, *end = data + len;
    while (p < end) {
        const char *nl = (const char *)memchr(p, '\n', (size_t)(end - p));
        size_t n = nl ? (size_t)(nl - p) : (size_t)(end - p);
        if (n && p[n - 1] == '\r') n--;
        if (n > 0 && n < sizeof(project_file_name_t) && !memchr(p, '/', n))
            out.emplace_back(p, n);
        p = nl ? nl + 1 : end;
    }
    free(data);
    return true;
}

static esp_err_t ignore_save(const name_list_t &names)
{
    char path[96];
    ignore_path(path, sizeof(path));
    if (names.empty())
        return sd_card_file_exists(path) ? sd_card_delete_file(path) : ESP_OK;
    std::string body;
    for (const std::string &n : names) {
        body += n;
        body += '\n';
    }
    return sd_card_write_file(path, body.data(), body.size());
}

static bool name_listed(const name_list_t &names, const char *name)
{
    for (const std::string &n : names)
        if (strcasecmp(n.c_str(), name) == 0) return true;
    return false;
}

/* A root entry the migration is about: a document, not the readers'
 * clippings file. */
static bool root_doc_candidate(const sd_card_file_entry_t &e)
{
    return !e.is_dir && editor_path_format(e.name, NULL) &&
           strcasecmp(e.name, ROOT_KEEP_NAME) != 0;
}

extern "C" int project_root_docs(project_root_doc_t *out, int max, int *total,
                                 int *unignored)
{
    if (total) *total = 0;
    if (unignored) *unignored = 0;
    if (!out) max = 0;
    name_list_t ignored;
    if (!ignore_load(ignored)) return -1;
    int count = 0;
    sd_card_file_entry_t *e = NULL;
    if (!scan_dir(mount(), &e, &count)) return -1;
    int n = 0, all = 0, free_docs = 0;
    /* sd_card_list_dir() sorts case-insensitively by name. */
    for (int i = 0; i < count; i++) {
        if (!root_doc_candidate(e[i])) continue;
        bool ign = name_listed(ignored, e[i].name);
        all++;
        if (!ign) free_docs++;
        if (n < max) {
            snprintf(out[n].name, sizeof(out[n].name), "%s", e[i].name);
            out[n].ignored = ign;
            n++;
        }
    }
    sd_card_free_list(e);
    if (total) *total = all;
    if (unignored) *unignored = free_docs;
    return n;
}

extern "C" esp_err_t project_root_doc_set_ignored(const char *name, bool ignored)
{
    if (!name || !name[0] || strchr(name, '/') ||
        strlen(name) >= sizeof(project_file_name_t))
        return ESP_ERR_INVALID_ARG;
    name_list_t list;
    if (!ignore_load(list)) return ESP_FAIL;
    /* Drop the entry for this name, and those of files that are gone. */
    name_list_t keep;
    char path[400];
    for (const std::string &n : list) {
        if (strcasecmp(n.c_str(), name) == 0) continue;
        snprintf(path, sizeof(path), "%s/%s", mount(), n.c_str());
        if (sd_card_file_exists(path) && !sd_card_is_dir(path)) keep.push_back(n);
    }
    if (ignored) keep.emplace_back(name);
    return ignore_save(keep);
}

extern "C" bool project_free_file_name(const char *dir, const char *name,
                                       char *out, size_t out_sz)
{
    char path[400];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    if (!sd_card_file_exists(path)) {
        snprintf(out, out_sz, "%s", name);
        return strlen(name) < out_sz;
    }
    const char *dot = strrchr(name, '.');
    size_t base = dot ? (size_t)(dot - name) : strlen(name);
    const char *ext = dot ? dot : "";
    for (int i = 2; i < 100; i++) {
        snprintf(out, out_sz, "%.*s-%d%s", (int)base, name, i, ext);
        snprintf(path, sizeof(path), "%s/%s", dir, out);
        if (!sd_card_file_exists(path)) return true;
    }
    return false;
}

/* ".<name>.meta" next to a document (see editor.cpp's meta_path_for()). */
static void sidecar_path(const char *dir, const char *name, char *out, size_t out_sz)
{
    snprintf(out, out_sz, "%s/.%s.meta", dir, name);
}

extern "C" esp_err_t project_migrate_root(const char *name, int *moved)
{
    *moved = 0;
    if (project_name_problem(name)) return ESP_ERR_INVALID_ARG;
    name_list_t ignored;
    if (!ignore_load(ignored)) return ESP_FAIL;
    char dir[64];
    project_dir(name, dir, sizeof(dir));
    if (!sd_card_is_dir(dir)) {
        esp_err_t err = project_create(name);
        if (err != ESP_OK) return err;
    }

    int count = 0;
    sd_card_file_entry_t *e = NULL;
    if (!scan_dir(mount(), &e, &count)) return ESP_FAIL;
    esp_err_t ret = ESP_OK;
    for (int i = 0; i < count; i++) {
        if (!root_doc_candidate(e[i]) || name_listed(ignored, e[i].name)) continue;
        char dst_name[256];
        if (!project_free_file_name(dir, e[i].name, dst_name, sizeof(dst_name))) {
            ret = ESP_FAIL;
            continue;
        }
        char src[400], dst[400];
        snprintf(src, sizeof(src), "%s/%s", mount(), e[i].name);
        snprintf(dst, sizeof(dst), "%s/%s", dir, dst_name);
        if (sd_card_rename(src, dst) != ESP_OK) {
            ESP_LOGW(TAG, "Cannot move %s", src);
            ret = ESP_FAIL;
            continue;
        }
        (*moved)++;
        char ms[400], md[400];
        sidecar_path(mount(), e[i].name, ms, sizeof(ms));
        sidecar_path(dir, dst_name, md, sizeof(md));
        if (sd_card_file_exists(ms) && !sd_card_file_exists(md)) sd_card_rename(ms, md);
    }
    sd_card_free_list(e);

    /* The Git configuration and history go with the documents, unless
     * the project already has its own. */
    char src[96], dst[96];
    snprintf(src, sizeof(src), "%s/git.cfg", mount());
    snprintf(dst, sizeof(dst), "%s/git.cfg", dir);
    if (sd_card_file_exists(src) && !sd_card_file_exists(dst)) {
        if (sd_card_rename(src, dst) != ESP_OK) ret = ESP_FAIL;
    }
    snprintf(src, sizeof(src), "%s/.git", mount());
    snprintf(dst, sizeof(dst), "%s/.git", dir);
    if (sd_card_is_dir(src) && !sd_card_file_exists(dst)) {
        if (sd_card_rename(src, dst) != ESP_OK) ret = ESP_FAIL;
    }
    ESP_LOGI(TAG, "Moved %d document(s) into %s", *moved, name);
    return ret;
}

extern "C" void project_stats(const char *name, int *files, uint64_t *bytes)
{
    *files = 0;
    *bytes = 0;
    char dir[64];
    project_dir(name, dir, sizeof(dir));
    int count = 0;
    sd_card_file_entry_t *e = NULL;
    if (!scan_dir(dir, &e, &count)) return;
    for (int i = 0; i < count; i++) {
        if (e[i].is_dir || !editor_path_format(e[i].name, NULL)) continue;
        (*files)++;
        *bytes += e[i].size;
    }
    sd_card_free_list(e);
}

/* "last=<value>" metadata files. */
static void meta_read(const char *path, char *out, size_t out_sz)
{
    out[0] = '\0';
    char *data = NULL;
    size_t len = 0;
    if (!sd_card_file_exists(path) || sd_card_read_file(path, &data, &len) != ESP_OK || !data)
        return;
    const char *p = data;
    while (p && *p) {
        const char *nl = strchr(p, '\n');
        size_t l = nl ? (size_t)(nl - p) : strlen(p);
        if (l > 5 && strncmp(p, "last=", 5) == 0) {
            size_t v = l - 5;
            while (v && (p[5 + v - 1] == '\r')) v--;
            if (v >= out_sz) v = out_sz - 1;
            memcpy(out, p + 5, v);
            out[v] = '\0';
            break;
        }
        p = nl ? nl + 1 : NULL;
    }
    free(data);
}

static void meta_write(const char *path, const char *value)
{
    char cur[256];
    meta_read(path, cur, sizeof(cur));
    if (strcmp(cur, value ? value : "") == 0) return;   /* spare the card */
    char body[300];
    int n = snprintf(body, sizeof(body), "last=%s\n", value ? value : "");
    if (n > 0 && n < (int)sizeof(body)) sd_card_write_file(path, body, (size_t)n);
}

extern "C" void project_get_last_file(const char *name, char *out, size_t out_sz)
{
    char path[96];
    snprintf(path, sizeof(path), "%s/%s/" PROJECT_META_NAME, mount(), name);
    meta_read(path, out, out_sz);
    if (strchr(out, '/')) out[0] = '\0';
}

extern "C" void project_set_last_file(const char *name, const char *file)
{
    if (!name || !name[0] || !project_exists(name)) return;
    char path[96];
    snprintf(path, sizeof(path), "%s/%s/" PROJECT_META_NAME, mount(), name);
    meta_write(path, file);
}

extern "C" void projects_get_last(char *out, size_t out_sz)
{
    char path[64];
    snprintf(path, sizeof(path), "%s/" ROOT_META_NAME, mount());
    meta_read(path, out, out_sz);
    if (project_name_problem(out)) out[0] = '\0';
}

extern "C" void projects_set_last(const char *name)
{
    char path[64];
    snprintf(path, sizeof(path), "%s/" ROOT_META_NAME, mount());
    meta_write(path, name);
}
