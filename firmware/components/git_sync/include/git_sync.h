#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <esp_err.h>
#include <stdbool.h>
#include <stddef.h>

/* The Git client keeps object data on the SD card (scratch files in
 * <project>/.git/tmp) and streams it through fixed-size buffers, so a
 * sync has no file-size limit tied to free PSRAM. Only the line-level
 * merge holds data in RAM: a hash and an offset per line of the part
 * the three versions do not share, plus a Myers diff in linear space.
 * Should even that not fit, the file becomes a whole-file conflict. */

typedef enum {
    GIT_SYNC_IDLE,
    GIT_SYNC_IN_PROGRESS,
    GIT_SYNC_SUCCESS,
    GIT_SYNC_ERROR,
} git_sync_state_t;

typedef enum {
    GIT_SYNC_PULL,
    GIT_SYNC_PUSH,
    GIT_SYNC_BOTH,
} git_sync_direction_t;

typedef void (*git_sync_callback_t)(git_sync_state_t state, const char *message);

/* Every project folder on the SD card is synced on its own. The
 * project_dir arguments below are the folder's full path (for example
 * "/sdcard/MyDrafts"): its configuration is <project_dir>/git.cfg, its
 * history <project_dir>/.git, and its working tree the *.md /
 * *.fountain / *.txt files directly inside it. */

esp_err_t git_sync_init(void);
/* Sync one project. Only one sync runs at a time. */
esp_err_t git_sync_start(const char *project_dir, git_sync_direction_t direction);
git_sync_state_t git_sync_get_state(void);
void git_sync_set_callback(git_sync_callback_t callback);
/* True if <project_dir>/git.cfg names a repository. */
bool git_sync_is_configured(const char *project_dir);
/* Host name of the project's repository, the repository's name (the
 * repo_url path without ".git", e.g. "user/notes") and the branch;
 * false if the project is not configured. */
bool git_sync_get_remote(const char *project_dir, char *host, size_t host_sz,
                         char *repo, size_t repo_sz,
                         char *branch, size_t branch_sz);
/* Folder of the project the running (or last) sync works on. */
const char *git_sync_get_project_dir(void);
const char *git_sync_get_last_error(void);

/* Whether a file in the sync directory is safely stored on the Git
 * server, i.e. could be deleted locally without losing anything.
 * GIT_SYNC_FILE_PUSHED means its current content on the SD card is
 * byte-identical to the version in the last commit known to be on the
 * server (refs/remotes/origin/<branch>, updated by a successful push
 * or by fetching the remote tip). The check is local only; it never
 * touches the network. */
typedef enum {
    GIT_SYNC_FILE_PUSHED,        /* identical to the server's copy */
    GIT_SYNC_FILE_NOT_CONFIGURED,/* no git.cfg / repo_url */
    GIT_SYNC_FILE_BUSY,          /* a sync is running */
    GIT_SYNC_FILE_NOT_PUSHED,    /* not in the last pushed commit (new, or never synced) */
    GIT_SYNC_FILE_MODIFIED,      /* changed since the last push */
    GIT_SYNC_FILE_ERROR,         /* unreadable file or repository */
} git_sync_file_status_t;

/* name is a bare file name in the project folder. */
git_sync_file_status_t git_sync_file_status(const char *project_dir, const char *name);

/* Like git_sync_file_status(), but checks the given content (e.g. an
 * editor buffer with unsaved changes) instead of the file on the SD
 * card when data is non-NULL. On GIT_SYNC_FILE_MODIFIED, *added and
 * *deleted receive the number of lines added and deleted relative to
 * the server's copy; otherwise (or if the diff fails) they are -1. */
git_sync_file_status_t git_sync_file_diff(const char *project_dir, const char *name,
                                          const char *data, size_t len,
                                          int *added, int *deleted);

/* Whole-project version of git_sync_file_status(), local only: one
 * pass over the project's documents against the last commit known to
 * be on the server. */
typedef struct {
    bool configured;     /* git.cfg names a repository */
    bool busy;           /* a sync is running; counts not filled in */
    bool error;          /* the repository or a file could not be read */
    bool never_pushed;   /* nothing known to be on the server yet */
    int  files;          /* documents in the project folder */
    int  pushed;         /* identical to the server's copy */
    int  modified;       /* changed since the last push */
    int  not_pushed;     /* not on the server yet */
    int  deleted_locally;/* on the server, deleted here (next sync deletes them there) */
} git_sync_project_status_t;

void git_sync_project_status(const char *project_dir, git_sync_project_status_t *st);

/* Connect to a Draftbox repository (github.com/clackups/draftbox):
 * exchange the 8-digit one-time password for an access token via
 * https://<site>/api/v1/token-exchange, then write the returned clone
 * URL and token into <project_dir>/git.cfg.
 *
 * site is a host name such as "draftbox.art" (a leading "https://" and
 * trailing '/' are accepted and stripped). Blocks while the HTTPS
 * request runs, so call it off the UI task; WiFi must be connected.
 *
 * On success msg holds a short report ("connected to user/repo"). On
 * any failure git.cfg is left untouched and msg says why. repo_url,
 * token, username and branch are set from the server's cloneUrl,
 * token, user and branch (username / branch only when the server sends
 * them); path is removed; author_name / author_email and any other
 * lines are kept. */
esp_err_t git_sync_draftbox_connect(const char *project_dir,
                                    const char *site, const char *otp,
                                    char *msg, size_t msg_len);

#ifdef __cplusplus
}
#endif
