#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <esp_err.h>
#include <stdbool.h>
#include <stddef.h>

/* Maximum size (in bytes) of a single file that git_sync can currently
 * handle during a sync.
 *
 * Not a fixed constant: derived at call time from the SPIRAM that is
 * free *right now*. The native Git client streams objects through the SD
 * card, so the transient PSRAM peak during a sync is dominated by a
 * single file's content plus the diff3 LCS matrix; headroom is reserved
 * for the TLS buffers and other concurrent allocations.
 *
 * The editor uses this same function at editor_init() time to clamp
 * its own per-buffer ceiling, so the editor never produces a document
 * larger than what a sync can process. Git is the tighter constraint
 * and therefore the determining factor for the maximum file size. */
size_t git_sync_max_file_size(void);

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

esp_err_t git_sync_init(void);
esp_err_t git_sync_start(git_sync_direction_t direction);
git_sync_state_t git_sync_get_state(void);
void git_sync_set_callback(git_sync_callback_t callback);
bool git_sync_is_configured(void);
const char *git_sync_get_last_error(void);
const char *git_sync_get_last_sync_time(void);

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

/* name is a bare file name in the sync directory (the SD card root). */
git_sync_file_status_t git_sync_file_status(const char *name);

#ifdef __cplusplus
}
#endif
