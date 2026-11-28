#pragma once
/* Projects: the SD card holds one folder per project
 * (/sdcard/<project>), each with its own git.cfg, Git history (.git)
 * and documents. The root folder holds only device-wide files
 * (wifi.cfg) and no documents. */

#include <stddef.h>
#include <stdint.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PROJECT_NAME_MAX      32
#define PROJECT_DEFAULT_NAME  "MyDrafts"

typedef char project_name_t[PROJECT_NAME_MAX + 1];

/* A project name may contain ASCII letters, digits, '-', '_' and
 * spaces, must not start (or end, FAT drops trailing spaces) with a
 * space and is at most PROJECT_NAME_MAX characters long. Returns NULL
 * for a valid name, otherwise a short reason for the status bar. */
const char *project_name_problem(const char *name);

/* True if c may appear in a project name. */
bool project_name_char_ok(char c);

/* "/sdcard/<name>" */
void project_dir(const char *name, char *out, size_t out_sz);

/* Every project folder on the card, sorted by name, in an allocated
 * array (free() it; NULL when there are none). *count is the number
 * of names, or -1 when the card cannot be read or the list does not
 * fit in memory -- not the same as "no projects". */
project_name_t *project_list(int *count);

/* True if the card holds at least one project. Also true when that
 * cannot be determined, so an error never sends the user into the
 * first-project set-up. */
bool project_any(void);

bool project_exists(const char *name);

/* Create an empty project folder. ESP_ERR_INVALID_STATE if a file or
 * folder of that name exists. */
esp_err_t project_create(const char *name);

/* Delete a project folder with everything in it. */
esp_err_t project_delete(const char *name);

/* If path is a file directly inside a project folder, store the
 * project name in out and return true. */
bool project_of_path(const char *path, char *out, size_t out_sz);

/* Documents (*.md, *.fountain, *.txt) in the root folder, outside any
 * project. The user can mark some to stay there ("ignored"); the marks
 * are kept in the hidden root file .projects.ignore, one name per line,
 * matched case-insensitively. "My Clippings.txt", which reader firmware
 * sharing the card (Crosspoint, Crossink) keeps in the root, is never
 * listed and never moved. */
#define PROJECT_ROOT_DOCS_MAX 128
typedef char project_file_name_t[256];   /* a whole FAT name */
typedef struct {
    project_file_name_t name;
    bool                ignored;
} project_root_doc_t;

/* Stores up to max root documents (sorted) in out, which may be NULL
 * when max is 0, and returns how many were stored (<= max), or -1 when
 * the root or the ignore list cannot be read. *total and *unignored
 * (either may be NULL) receive the counts over all of them, also those
 * past max. */
int project_root_docs(project_root_doc_t *out, int max, int *total, int *unignored);

/* Mark a root document to stay in the root (ignored) or to be moved
 * again. Names of files no longer in the root are dropped from the
 * list on the way. */
esp_err_t project_root_doc_set_ignored(const char *name, bool ignored);

/* Move the root folder's documents (with their metadata sidecars),
 * git.cfg and .git history into the project, creating it if needed.
 * Ignored documents and "My Clippings.txt" stay in the root. A
 * document whose name is taken in the project gets a "-2", "-3", ...
 * suffix. *moved receives the number of documents moved. */
esp_err_t project_migrate_root(const char *name, int *moved);

/* A file name like name, but free in dir: name itself, or "base-2.ext",
 * "base-3.ext", ... Returns false if none is free. */
bool project_free_file_name(const char *dir, const char *name, char *out, size_t out_sz);

/* Number of documents and their total size. */
void project_stats(const char *name, int *files, uint64_t *bytes);

/* The project's last selected document ("" if none). */
void project_get_last_file(const char *name, char *out, size_t out_sz);
void project_set_last_file(const char *name, const char *file);

/* The last opened project ("" if none). */
void projects_get_last(char *out, size_t out_sz);
void projects_set_last(const char *name);

#ifdef __cplusplus
}
#endif
