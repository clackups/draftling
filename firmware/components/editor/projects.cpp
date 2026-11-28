/* projects.cpp -- project folders on the SD card. See projects.h. */
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <strings.h>
#include <esp_log.h>
#include <esp_heap_caps.h>

#include "projects.h"
#include "editor.h"
#include "sd_card.h"

static const char *TAG = "Projects";

<<<<<<< HEAD
=======
#define ROOT_SCAN_MAX PROJECT_ROOT_DOCS_MAX
>>>>>>> a796c65 (option to ignore moving files in root; auto ignore files with clippings in the name for dual-boot)

/* Project and root metadata: hidden files, so neither the file browser
 * nor Git sync ever sees them. */
#define PROJECT_META_NAME ".project.meta"
#define ROOT_META_NAME    ".projects.meta"
#define ROOT_IGNORE_NAME  ".projects.ignore"

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

extern "C" int project_root_doc_names(project_file_name_t *names, int max)
{
<<<<<<< HEAD
    int count = 0;
    sd_card_file_entry_t *e = NULL;
    if (!scan_dir(mount(), &e, &count)) return 0;
    int n = 0;
    for (int i = 0; i < count; i++) {
        if (e[i].is_dir || !editor_path_format(e[i].name, NULL)) continue;
        /* only shown in a pop-up: a long name is cut short */
        if (names && n < max)
            snprintf(names[n], sizeof(project_file_name_t), "%.*s",
                     (int)sizeof(project_file_name_t) - 1, e[i].name);
        n++;
    }
    sd_card_free_list(e);
    return n;
}

=======
    project_file_name_t *all = (project_file_name_t *)heap_caps_malloc(
        ROOT_SCAN_MAX * sizeof(project_file_name_t), MALLOC_CAP_SPIRAM);
    bool *ignored = (bool *)heap_caps_malloc(ROOT_SCAN_MAX * sizeof(bool), MALLOC_CAP_SPIRAM);
    if (!all || !ignored) {
        heap_caps_free(all);
        heap_caps_free(ignored);
        return -1;
    }
    int total = project_root_doc_list(all, ignored, ROOT_SCAN_MAX);
    if (total < 0) {
        heap_caps_free(all);
        heap_caps_free(ignored);
        return -1;
    }
    int n = 0;
    for (int i = 0; i < total && i < ROOT_SCAN_MAX; i++) {
        if (ignored[i]) continue;
        if (names && n < max) strlcpy(names[n], all[i], sizeof(project_file_name_t));
        n++;
    }
    heap_caps_free(all);
    heap_caps_free(ignored);
    return n;
}

extern "C" int project_root_doc_count(void)
{
    return project_root_doc_names(NULL, 0);
}

static bool name_contains_clippings(const char *name)
{
    static const char needle[] = "clippings";
    if (!name) return false;
    for (const char *p = name; *p; p++) {
        const char *a = p;
        const char *b = needle;
        while (*a && *b) {
            char ca = *a >= 'A' && *a <= 'Z' ? (char)(*a + ('a' - 'A')) : *a;
            if (ca != *b) break;
            a++;
            b++;
        }
        if (!*b) return true;
    }
    return false;
}

static bool root_ignore_load(project_file_name_t **names, int *count)
{
    *names = (project_file_name_t *)heap_caps_malloc(
        ROOT_SCAN_MAX * sizeof(project_file_name_t), MALLOC_CAP_SPIRAM);
    *count = 0;
    if (!*names) return false;
    char path[96];
    snprintf(path, sizeof(path), "%s/" ROOT_IGNORE_NAME, mount());
    if (!sd_card_file_exists(path)) return true;

    char *data = NULL;
    size_t len = 0;
    esp_err_t err = sd_card_read_file(path, &data, &len);
    if (err != ESP_OK || !data) {
        ESP_LOGE(TAG, "Cannot read root ignore list: %s", esp_err_to_name(err));
        free(data);
        heap_caps_free(*names);
        *names = NULL;
        return false;
    }

    const char *p = data;
    const char *end = data + len;
    while (p < end && *count < ROOT_SCAN_MAX) {
        const char *nl = (const char *)memchr(p, '\n', (size_t)(end - p));
        size_t n = nl ? (size_t)(nl - p) : (size_t)(end - p);
        if (n && p[n - 1] == '\r') n--;
        if (n > 0 && n < sizeof(project_file_name_t) &&
            !memchr(p, '/', n) && !memchr(p, '\\', n)) {
            memcpy((*names)[*count], p, n);
            (*names)[*count][n] = '\0';
            (*count)++;
        }
        p = nl ? nl + 1 : end;
    }
    free(data);
    return true;
}

static bool root_name_is_ignored(const char *name, const project_file_name_t *ignored,
                                 int ignored_count)
{
    for (int i = 0; i < ignored_count; i++)
        if (strcasecmp(name, ignored[i]) == 0) return true;
    return false;
}

static esp_err_t root_ignore_save(const project_file_name_t *names, int count)
{
    char path[96];
    snprintf(path, sizeof(path), "%s/" ROOT_IGNORE_NAME, mount());
    if (count == 0) {
        if (!sd_card_file_exists(path)) return ESP_OK;
        return sd_card_delete_file(path);
    }
    size_t cap = (size_t)count * sizeof(project_file_name_t);
    char *body = (char *)heap_caps_malloc(cap, MALLOC_CAP_SPIRAM);
    if (!body) return ESP_ERR_NO_MEM;
    size_t used = 0;
    for (int i = 0; i < count; i++) {
        size_t n = strlen(names[i]);
        if (used + n + 1 > cap) {
            heap_caps_free(body);
            return ESP_ERR_INVALID_SIZE;
        }
        memcpy(body + used, names[i], n);
        used += n;
        body[used++] = '\n';
    }
    esp_err_t err = sd_card_write_file(path, body, used);
    heap_caps_free(body);
    return err;
}

static int root_doc_scan(project_file_name_t *names, bool *ignored_out, int max,
                         bool include_ignored)
{
    project_file_name_t *ignored_names = NULL;
    int ignored_count = 0;
    if (!root_ignore_load(&ignored_names, &ignored_count)) return -1;

    int entry_count = 0;
    sd_card_file_entry_t *entries = scan_dir(mount(), &entry_count);
    if (!entries) {
        heap_caps_free(ignored_names);
        return -1;
    }
    int n = 0;
    for (int i = 0; i < entry_count; i++) {
        if (entries[i].is_dir || !editor_path_format(entries[i].name, NULL) ||
            name_contains_clippings(entries[i].name)) continue;
        bool ignored = root_name_is_ignored(entries[i].name, ignored_names, ignored_count);
        if (!include_ignored && ignored) continue;
        if (names && n < max)
            strlcpy(names[n], entries[i].name, sizeof(project_file_name_t));
        if (ignored_out && n < max) ignored_out[n] = ignored;
        n++;
    }
    heap_caps_free(entries);
    heap_caps_free(ignored_names);
    return n;
}

extern "C" int project_root_doc_list(project_file_name_t *names, bool *ignored, int max)
{
    return root_doc_scan(names, ignored, max, true);
}

extern "C" esp_err_t project_root_doc_set_ignored(const char *name, bool ignored)
{
    if (!name || !name[0] || strchr(name, '/') || strchr(name, '\\') ||
        strlen(name) >= sizeof(project_file_name_t) ||
        !editor_path_format(name, NULL) || name_contains_clippings(name))
        return ESP_ERR_INVALID_ARG;

    char file_path[400];
    snprintf(file_path, sizeof(file_path), "%s/%s", mount(), name);
    if (!sd_card_file_exists(file_path) || sd_card_is_dir(file_path))
        return ESP_ERR_NOT_FOUND;

    project_file_name_t *names = NULL;
    int count = 0;
    if (!root_ignore_load(&names, &count)) return ESP_FAIL;
    bool found = false;
    for (int i = 0; i < count; i++) {
        if (strcasecmp(names[i], name) == 0) {
            found = true;
            if (!ignored) {
                memmove(&names[i], &names[i + 1],
                        (size_t)(count - i - 1) * sizeof(project_file_name_t));
                count--;
            }
            break;
        }
    }
    if (ignored && !found) {
        if (count >= ROOT_SCAN_MAX) {
            heap_caps_free(names);
            return ESP_ERR_NO_MEM;
        }
        strlcpy(names[count++], name, sizeof(project_file_name_t));
    }
    esp_err_t err = root_ignore_save(names, count);
    heap_caps_free(names);
    return err;
}

>>>>>>> a796c65 (option to ignore moving files in root; auto ignore files with clippings in the name for dual-boot)
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
    if (!moved) return ESP_ERR_INVALID_ARG;
    *moved = 0;
    if (project_name_problem(name)) return ESP_ERR_INVALID_ARG;
    project_file_name_t *ignored_names = NULL;
    int ignored_count = 0;
    if (!root_ignore_load(&ignored_names, &ignored_count)) return ESP_FAIL;

    char dir[64];
    project_dir(name, dir, sizeof(dir));
    if (!sd_card_is_dir(dir)) {
        esp_err_t err = project_create(name);
        if (err != ESP_OK) {
            heap_caps_free(ignored_names);
            return err;
        }
    }

    int count = 0;
<<<<<<< HEAD
    sd_card_file_entry_t *e = NULL;
    if (!scan_dir(mount(), &e, &count)) return ESP_FAIL;
=======
    sd_card_file_entry_t *e = scan_dir(mount(), &count);
    if (!e) {
        heap_caps_free(ignored_names);
        return ESP_ERR_NO_MEM;
    }
>>>>>>> a796c65 (option to ignore moving files in root; auto ignore files with clippings in the name for dual-boot)
    esp_err_t ret = ESP_OK;
    for (int i = 0; i < count; i++) {
        if (e[i].is_dir || !editor_path_format(e[i].name, NULL) ||
            name_contains_clippings(e[i].name) ||
            root_name_is_ignored(e[i].name, ignored_names, ignored_count)) continue;
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
<<<<<<< HEAD
    sd_card_free_list(e);
=======
    heap_caps_free(e);
    heap_caps_free(ignored_names);
>>>>>>> a796c65 (option to ignore moving files in root; auto ignore files with clippings in the name for dual-boot)

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
