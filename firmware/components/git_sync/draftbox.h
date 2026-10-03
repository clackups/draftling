/* draftbox.h -- Draftbox token-exchange client (internal to git_sync). */
#pragma once

#include <stdbool.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char clone_url[300];   /* matches git_sync's repo_url */
    char token[160];       /* matches git_sync's token */
    char user[64];         /* HTTP Basic user; matches git_sync's username */
    char branch[64];       /* empty if the server does not send it */
    char repository[96];   /* "user/repo", for the status message */
    bool read_only;
    char error[64];        /* human-readable reason on failure */
} draftbox_result_t;

/* Exchange a one-time password for an access token on
 * https://<site>/api/v1/token-exchange. Blocks for up to ~20 s per
 * connection attempt. On failure res->error says why. */
esp_err_t draftbox_token_exchange(const char *site, const char *otp,
                                  draftbox_result_t *res);

#ifdef __cplusplus
}
#endif
