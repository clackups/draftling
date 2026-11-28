#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <strings.h>
#include <string>
#include <vector>

#include "projects.h"
#include "editor.h"
#include "sd_card.h"

struct fs_entry_t {
    std::string path;
    std::string data;
    bool is_dir;
};

static std::vector<fs_entry_t> s_entries;

static std::string parent_path(const std::string &path)
{
    size_t slash = path.find_last_of('/');
    return slash == 0 ? "/" : path.substr(0, slash);
}

static fs_entry_t *find_entry(const char *path)
{
    for (fs_entry_t &entry : s_entries)
        if (entry.path == path) return &entry;
    return NULL;
}

static void add_dir(const char *path)
{
    s_entries.push_back({path, "", true});
}

static void add_file(const char *path)
{
    s_entries.push_back({path, "data", false});
}

const char *sd_card_get_mount_point(void)
{
    return "/sdcard";
}

int sd_card_list_dir(const char *path, sd_card_file_entry_t *out, int max)
{
    std::vector<sd_card_file_entry_t> children;
    for (const fs_entry_t &entry : s_entries) {
        if (parent_path(entry.path) != path) continue;
        sd_card_file_entry_t child = {};
        const char *name = entry.path.c_str() + strlen(path) + 1;
        snprintf(child.name, sizeof(child.name), "%s", name);
        child.is_dir = entry.is_dir;
        child.size = entry.data.size();
        children.push_back(child);
    }
    std::sort(children.begin(), children.end(),
              [](const sd_card_file_entry_t &a, const sd_card_file_entry_t &b) {
        return strcasecmp(a.name, b.name) < 0;
    });
    for (int i = 0; i < (int)children.size() && i < max; i++) out[i] = children[i];
    return (int)children.size();
}

bool sd_card_file_exists(const char *path)
{
    return find_entry(path) != NULL;
}

bool sd_card_is_dir(const char *path)
{
    fs_entry_t *entry = find_entry(path);
    return entry && entry->is_dir;
}

esp_err_t sd_card_mkdir(const char *path)
{
    if (sd_card_file_exists(path)) return ESP_ERR_INVALID_STATE;
    add_dir(path);
    return ESP_OK;
}

esp_err_t sd_card_remove_tree(const char *path)
{
    std::string prefix = std::string(path) + "/";
    s_entries.erase(std::remove_if(s_entries.begin(), s_entries.end(),
        [&](const fs_entry_t &entry) {
            return entry.path == path || entry.path.compare(0, prefix.size(), prefix) == 0;
        }), s_entries.end());
    return ESP_OK;
}

esp_err_t sd_card_rename(const char *src, const char *dst)
{
    if (!sd_card_file_exists(src)) return ESP_ERR_NOT_FOUND;
    if (sd_card_file_exists(dst)) return ESP_ERR_INVALID_STATE;
    fs_entry_t *entry = find_entry(src);
    if (!entry || !entry->is_dir) {
        entry->path = dst;
        return ESP_OK;
    }
    std::string old_path(src);
    std::string new_path(dst);
    std::string prefix = old_path + "/";
    for (fs_entry_t &child : s_entries) {
        if (child.path == old_path) child.path = new_path;
        else if (child.path.compare(0, prefix.size(), prefix) == 0)
            child.path = new_path + child.path.substr(old_path.size());
    }
    return ESP_OK;
}

esp_err_t sd_card_read_file(const char *path, char **out, size_t *len)
{
    fs_entry_t *entry = find_entry(path);
    if (!entry || entry->is_dir) return ESP_ERR_NOT_FOUND;
    *out = (char *)malloc(entry->data.size() + 1);
    if (!*out) return ESP_ERR_NO_MEM;
    memcpy(*out, entry->data.data(), entry->data.size());
    (*out)[entry->data.size()] = '\0';
    *len = entry->data.size();
    return ESP_OK;
}

esp_err_t sd_card_write_file(const char *path, const char *data, size_t len)
{
    fs_entry_t *entry = find_entry(path);
    if (entry && entry->is_dir) return ESP_ERR_INVALID_STATE;
    if (!entry) {
        add_file(path);
        entry = find_entry(path);
    }
    entry->data.assign(data, len);
    return ESP_OK;
}

esp_err_t sd_card_delete_file(const char *path)
{
    fs_entry_t *entry = find_entry(path);
    if (!entry || entry->is_dir) return ESP_ERR_NOT_FOUND;
    s_entries.erase(std::remove_if(s_entries.begin(), s_entries.end(),
        [&](const fs_entry_t &candidate) { return candidate.path == path; }), s_entries.end());
    return ESP_OK;
}

const char *esp_err_to_name(esp_err_t err)
{
    return err == ESP_OK ? "ESP_OK" : "ESP_FAIL";
}

bool editor_path_format(const char *path, editor_doc_format_t *out)
{
    (void)out;
    const char *dot = strrchr(path, '.');
    return dot && (strcasecmp(dot, ".md") == 0 ||
                   strcasecmp(dot, ".fountain") == 0 ||
                   strcasecmp(dot, ".txt") == 0);
}

int main(void)
{
    add_dir("/sdcard");
    add_file("/sdcard/notes.md");
    add_file("/sdcard/keep.txt");
    add_file("/sdcard/Clippings.txt");
    add_file("/sdcard/archive-CLIPPINGS-2026.md");
    std::string long_name = std::string(100, 'n') + ".txt";
    add_file(("/sdcard/" + long_name).c_str());

    project_file_name_t names[PROJECT_ROOT_DOCS_MAX];
    bool ignored[PROJECT_ROOT_DOCS_MAX];
    int count = project_root_doc_list(names, ignored, PROJECT_ROOT_DOCS_MAX);
    assert(count == 3);
    assert(strcmp(names[0], "keep.txt") == 0 && !ignored[0]);
    assert(strcmp(names[1], long_name.c_str()) == 0 && !ignored[1]);
    assert(strcmp(names[2], "notes.md") == 0 && !ignored[2]);
    assert(project_root_doc_count() == 3);

    assert(project_root_doc_set_ignored(long_name.c_str(), true) == ESP_OK);
    assert(sd_card_file_exists("/sdcard/.projects.ignore"));
    count = project_root_doc_list(names, ignored, PROJECT_ROOT_DOCS_MAX);
    assert(count == 3);
    assert(strcmp(names[1], long_name.c_str()) == 0 && ignored[1]);
    assert(project_root_doc_count() == 2);
    std::string uppercase_long_name = long_name;
    for (char &c : uppercase_long_name)
        if (c >= 'a' && c <= 'z') c = (char)(c - ('a' - 'A'));
    assert(sd_card_write_file("/sdcard/.projects.ignore",
                              (uppercase_long_name + "\n").c_str(),
                              uppercase_long_name.size() + 1) == ESP_OK);
    count = project_root_doc_list(names, ignored, PROJECT_ROOT_DOCS_MAX);
    assert(count == 3 && ignored[1]);
    assert(project_root_doc_set_ignored("Clippings.txt", true) == ESP_ERR_INVALID_ARG);

    int moved = 0;
    assert(project_migrate_root("Work", &moved) == ESP_OK);
    assert(moved == 2);
    assert(sd_card_file_exists("/sdcard/Work/notes.md"));
    assert(sd_card_file_exists("/sdcard/Work/keep.txt"));
    assert(sd_card_file_exists(("/sdcard/" + long_name).c_str()));
    assert(sd_card_file_exists("/sdcard/Clippings.txt"));
    assert(sd_card_file_exists("/sdcard/archive-CLIPPINGS-2026.md"));

    assert(project_root_doc_set_ignored(long_name.c_str(), false) == ESP_OK);
    assert(!sd_card_file_exists("/sdcard/.projects.ignore"));
    assert(project_migrate_root("Work", &moved) == ESP_OK);
    assert(moved == 1);
    assert(sd_card_file_exists(("/sdcard/Work/" + long_name).c_str()));
    assert(sd_card_file_exists("/sdcard/Clippings.txt"));
    assert(sd_card_file_exists("/sdcard/archive-CLIPPINGS-2026.md"));
    return 0;
}
