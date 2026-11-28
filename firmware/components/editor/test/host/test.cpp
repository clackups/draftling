/* Host test of projects.cpp: the documents outside projects, their
 * ignore list and the migration into a project. The SD card is a
 * temporary folder; the sd_card_* functions projects.cpp uses are
 * implemented on it below. See run.sh. */
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <dirent.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

#include "projects.h"
#include "editor.h"
#include "sd_card.h"

static std::string s_root;

/* ---- SD card on a folder ---- */

extern "C" const char *esp_err_to_name(esp_err_t err)
{
    return err == ESP_OK ? "ESP_OK" : "error";
}

extern "C" const char *sd_card_get_mount_point(void) { return s_root.c_str(); }

extern "C" bool sd_card_file_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

extern "C" bool sd_card_is_dir(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

extern "C" esp_err_t sd_card_mkdir(const char *path)
{
    return mkdir(path, 0755) == 0 ? ESP_OK : ESP_FAIL;
}

extern "C" esp_err_t sd_card_rename(const char *src, const char *dst)
{
    return rename(src, dst) == 0 ? ESP_OK : ESP_FAIL;
}

extern "C" esp_err_t sd_card_delete_file(const char *path)
{
    return unlink(path) == 0 ? ESP_OK : ESP_FAIL;
}

extern "C" esp_err_t sd_card_remove_tree(const char *path)
{
    std::string cmd = "rm -rf '" + std::string(path) + "'";
    return system(cmd.c_str()) == 0 ? ESP_OK : ESP_FAIL;
}

extern "C" esp_err_t sd_card_read_file(const char *path, char **out, size_t *len)
{
    FILE *f = fopen(path, "rb");
    if (!f) return ESP_FAIL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    *out = (char *)malloc((size_t)n + 1);
    *len = fread(*out, 1, (size_t)n, f);
    (*out)[*len] = '\0';
    fclose(f);
    return ESP_OK;
}

extern "C" esp_err_t sd_card_write_file(const char *path, const char *data, size_t len)
{
    FILE *f = fopen(path, "wb");
    if (!f) return ESP_FAIL;
    fwrite(data, 1, len, f);
    fclose(f);
    return ESP_OK;
}

extern "C" esp_err_t sd_card_list_dir(const char *path, sd_card_file_entry_t **out, int *count)
{
    *out = NULL;
    *count = 0;
    DIR *d = opendir(path);
    if (!d) return ESP_FAIL;
    std::vector<sd_card_file_entry_t> v;
    while (struct dirent *de = readdir(d)) {
        if (de->d_name[0] == '.') continue;
        sd_card_file_entry_t e = {};
        snprintf(e.name, sizeof(e.name), "%s", de->d_name);
        e.is_dir = sd_card_is_dir((std::string(path) + "/" + de->d_name).c_str());
        v.push_back(e);
    }
    closedir(d);
    std::sort(v.begin(), v.end(), [](const sd_card_file_entry_t &a, const sd_card_file_entry_t &b) {
        if (a.is_dir != b.is_dir) return a.is_dir;
        return strcasecmp(a.name, b.name) < 0;
    });
    if (v.empty()) return ESP_OK;
    *out = (sd_card_file_entry_t *)malloc(v.size() * sizeof(sd_card_file_entry_t));
    memcpy(*out, v.data(), v.size() * sizeof(sd_card_file_entry_t));
    *count = (int)v.size();
    return ESP_OK;
}

extern "C" void sd_card_free_list(sd_card_file_entry_t *list) { free(list); }

extern "C" bool editor_path_format(const char *path, editor_doc_format_t *out)
{
    (void)out;
    const char *dot = strrchr(path, '.');
    return dot && (!strcasecmp(dot, ".md") || !strcasecmp(dot, ".fountain") ||
                   !strcasecmp(dot, ".txt"));
}

/* ---- helpers ---- */

static std::string at(const std::string &rel) { return s_root + "/" + rel; }

static void touch(const std::string &rel)
{
    assert(sd_card_write_file(at(rel).c_str(), "x", 1) == ESP_OK);
}

static std::string read_all(const std::string &rel)
{
    char *d = NULL;
    size_t n = 0;
    if (sd_card_read_file(at(rel).c_str(), &d, &n) != ESP_OK) return "";
    std::string s(d, n);
    free(d);
    return s;
}

static int counts(int *total, int *unignored)
{
    return project_root_docs(NULL, 0, total, unignored);
}

int main(void)
{
    char tmpl[] = "/tmp/projects_test.XXXXXX";
    assert(mkdtemp(tmpl));
    s_root = tmpl;

    touch("notes.md");
    touch("keep.txt");
    touch("My Clippings.txt");          /* readers' file: never listed */
    touch("newspaper-clippings.md");    /* a document like any other */
    touch("image.png");                 /* not a document */
    assert(sd_card_mkdir(at("Work").c_str()) == ESP_OK);

    /* Listing, with and without an output array. */
    int total = -1, unignored = -1;
    assert(counts(&total, &unignored) == 0);
    assert(total == 3 && unignored == 3);
    project_root_doc_t docs[PROJECT_ROOT_DOCS_MAX];
    int n = project_root_docs(docs, PROJECT_ROOT_DOCS_MAX, &total, &unignored);
    assert(n == 3);
    assert(!strcmp(docs[0].name, "keep.txt") && !docs[0].ignored);
    assert(!strcmp(docs[1].name, "newspaper-clippings.md"));
    assert(!strcmp(docs[2].name, "notes.md"));

    /* Ignore one; the match is case-insensitive. */
    assert(project_root_doc_set_ignored("KEEP.TXT", true) == ESP_OK);
    n = project_root_docs(docs, PROJECT_ROOT_DOCS_MAX, &total, &unignored);
    assert(n == 3 && total == 3 && unignored == 2 && docs[0].ignored);
    assert(project_root_doc_set_ignored("a/b.md", true) == ESP_ERR_INVALID_ARG);
    assert(project_root_doc_set_ignored("", true) == ESP_ERR_INVALID_ARG);

    /* Migration leaves the ignored file and the clippings file. */
    int moved = 0;
    assert(project_migrate_root("Work", &moved) == ESP_OK);
    assert(moved == 2);
    assert(sd_card_file_exists(at("Work/notes.md").c_str()));
    assert(sd_card_file_exists(at("Work/newspaper-clippings.md").c_str()));
    assert(sd_card_file_exists(at("keep.txt").c_str()));
    assert(sd_card_file_exists(at("My Clippings.txt").c_str()));
    assert(counts(&total, &unignored) == 0 && total == 1 && unignored == 0);

    /* Names of files that are gone are dropped on the next change. */
    touch("other.md");
    assert(project_root_doc_set_ignored("other.md", true) == ESP_OK);
    assert(sd_card_delete_file(at("keep.txt").c_str()) == ESP_OK);
    touch("third.md");
    assert(project_root_doc_set_ignored("third.md", true) == ESP_OK);
    std::string list = read_all(".projects.ignore");
    assert(list.find("keep.txt") == std::string::npos);
    assert(list.find("other.md\n") != std::string::npos);
    assert(list.find("third.md\n") != std::string::npos);

    /* Un-ignoring every file removes the list. */
    assert(project_root_doc_set_ignored("other.md", false) == ESP_OK);
    assert(project_root_doc_set_ignored("third.md", false) == ESP_OK);
    assert(!sd_card_file_exists(at(".projects.ignore").c_str()));

    /* More documents than the array: only max are stored, the counts
     * cover all of them (AddressSanitizer checks the bounds). */
    for (int i = 0; i < PROJECT_ROOT_DOCS_MAX + 10; i++) {
        char name[32];
        snprintf(name, sizeof(name), "bulk-%03d.txt", i);
        touch(name);
    }
    assert(project_root_doc_set_ignored("bulk-137.txt", true) == ESP_OK);
    project_root_doc_t *few = (project_root_doc_t *)malloc(4 * sizeof(project_root_doc_t));
    n = project_root_docs(few, 4, &total, &unignored);
    assert(n == 4);
    assert(total == PROJECT_ROOT_DOCS_MAX + 12 && unignored == total - 1);
    free(few);
    n = project_root_docs(docs, PROJECT_ROOT_DOCS_MAX, &total, &unignored);
    assert(n == PROJECT_ROOT_DOCS_MAX && total > n);

    /* An ignore list longer than PROJECT_ROOT_DOCS_MAX still works. */
    for (int i = 0; i < PROJECT_ROOT_DOCS_MAX + 10; i++) {
        char name[32];
        snprintf(name, sizeof(name), "bulk-%03d.txt", i);
        assert(project_root_doc_set_ignored(name, true) == ESP_OK);
    }
    assert(counts(&total, &unignored) == 0 && unignored == 2);   /* other, third */

    std::string cmd = "rm -rf '" + s_root + "'";
    if (system(cmd.c_str()) != 0) return 1;
    printf("projects host test: OK\n");
    return 0;
}
