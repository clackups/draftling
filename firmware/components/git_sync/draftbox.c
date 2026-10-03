/* draftbox.c -- Draftbox token-exchange client.
 *
 * Draftbox (github.com/clackups/draftbox) is a Git hosting server whose
 * access tokens can carry a one-time 8-digit password. The password is
 * exchanged for the token over a small JSON API:
 *
 *   POST https://<site>/api/v1/token-exchange   {"password":"12345678"}
 *
 *   200 {"token":"dbx_...","name":"...","access":"write"|"read",
 *        "user":"...","repository":"user/repo",
 *        "cloneUrl":"https://<site>/user/repo.git","branch":"main",
 *        "expiresAt":...}
 *   404 {"error":"invalid_password"}
 *   429 {"error":"rate_limited"}
 *
 * Each password works once. "repository", "cloneUrl" and "branch" are
 * null for a global token (not bound to one repository), which cannot
 * be used for sync. "branch" is newer than the other fields; an older
 * server leaves it out. */
#include "draftbox.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <esp_log.h>
#include <esp_http_client.h>
#include <esp_crt_bundle.h>

#include "wifi_manager.h"

static const char *TAG = "draftbox";

#define DRAFTBOX_TIMEOUT_MS 20000
#define DRAFTBOX_MAX_BODY   4096

/* ---- minimal JSON reader --------------------------------------------
 * The response is one flat object whose values are strings, null or
 * scalars. json_get_string() looks up a top-level key and decodes its
 * string value; nested objects and arrays are skipped. */

static const char *json_ws(const char *p)
{
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    return p;
}

/* Parse a JSON string starting at the opening quote. Decodes into out
 * (truncating at out_sz - 1 bytes; out may be NULL to just skip it).
 * Returns the position after the closing quote, or NULL on a syntax
 * error. */
static const char *json_string(const char *p, char *out, size_t out_sz, bool *truncated)
{
    if (*p != '"') return NULL;
    p++;
    size_t n = 0;
    while (*p && *p != '"') {
        char buf[4];
        size_t blen = 1;
        if (*p == '\\') {
            p++;
            switch (*p) {
            case '"':  buf[0] = '"';  break;
            case '\\': buf[0] = '\\'; break;
            case '/':  buf[0] = '/';  break;
            case 'b':  buf[0] = '\b'; break;
            case 'f':  buf[0] = '\f'; break;
            case 'n':  buf[0] = '\n'; break;
            case 'r':  buf[0] = '\r'; break;
            case 't':  buf[0] = '\t'; break;
            case 'u': {
                unsigned cp = 0;
                for (int i = 1; i <= 4; i++) {
                    char c = p[i];
                    if (!isxdigit((unsigned char)c)) return NULL;
                    cp = (cp << 4) | (unsigned)(isdigit((unsigned char)c) ? c - '0'
                                                : (tolower((unsigned char)c) - 'a' + 10));
                }
                p += 4;
                /* Surrogates are not expected in this API; replace them. */
                if (cp >= 0xD800 && cp <= 0xDFFF) cp = '?';
                if (cp < 0x80) {
                    buf[0] = (char)cp;
                } else if (cp < 0x800) {
                    buf[0] = (char)(0xC0 | (cp >> 6));
                    buf[1] = (char)(0x80 | (cp & 0x3F));
                    blen = 2;
                } else {
                    buf[0] = (char)(0xE0 | (cp >> 12));
                    buf[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
                    buf[2] = (char)(0x80 | (cp & 0x3F));
                    blen = 3;
                }
                break;
            }
            default:
                return NULL;
            }
            p++;
        } else {
            buf[0] = *p++;
        }
        if (out) {
            if (n + blen < out_sz) {
                memcpy(out + n, buf, blen);
                n += blen;
            } else if (truncated) {
                *truncated = true;
            }
        }
    }
    if (*p != '"') return NULL;
    if (out && out_sz) out[n] = '\0';
    return p + 1;
}

/* Skip any JSON value. Returns the position after it, or NULL. */
static const char *json_skip(const char *p)
{
    p = json_ws(p);
    if (*p == '"') return json_string(p, NULL, 0, NULL);
    if (*p == '{' || *p == '[') {
        int depth = 0;
        while (*p) {
            if (*p == '"') {
                p = json_string(p, NULL, 0, NULL);
                if (!p) return NULL;
                continue;
            }
            if (*p == '{' || *p == '[') depth++;
            else if (*p == '}' || *p == ']') {
                if (--depth == 0) return p + 1;
            }
            p++;
        }
        return NULL;
    }
    /* number, true, false, null */
    const char *start = p;
    while (*p && *p != ',' && *p != '}' && *p != ']' &&
           *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n')
        p++;
    return (p == start) ? NULL : p;
}

/* Find a top-level key. Returns a pointer to its value, or NULL when
 * the key is missing or the object is malformed. */
static const char *json_find(const char *json, const char *key)
{
    const char *p = json_ws(json);
    if (*p != '{') return NULL;
    p++;
    char k[32];
    for (;;) {
        p = json_ws(p);
        if (*p == '}') return NULL;
        bool ktrunc = false;
        p = json_string(p, k, sizeof(k), &ktrunc);
        if (!p) return NULL;
        p = json_ws(p);
        if (*p != ':') return NULL;
        p = json_ws(p + 1);
        if (!ktrunc && strcmp(k, key) == 0) return p;
        p = json_skip(p);
        if (!p) return NULL;
        p = json_ws(p);
        if (*p == ',') { p++; continue; }
        return NULL;
    }
}

/* Copy the string value of a top-level key. Returns false when the key
 * is missing, its value is not a string (e.g. null) or does not fit
 * into out. */
static bool json_get_string(const char *json, const char *key, char *out, size_t out_sz)
{
    const char *p = json_find(json, key);
    if (!p || *p != '"') return false;
    bool vtrunc = false;
    return json_string(p, out, out_sz, &vtrunc) != NULL && !vtrunc;
}

/* Whether a top-level key is present with the value null. */
static bool json_is_null(const char *json, const char *key)
{
    const char *p = json_find(json, key);
    return p && strncmp(p, "null", 4) == 0;
}

/* The values end up as key=value lines in git.cfg: a line break or
 * other control character would inject extra keys. */
static bool cfg_safe(const char *s)
{
    for (; *s; s++)
        if ((unsigned char)*s < 0x20 || *s == 0x7f) return false;
    return true;
}

/* ---- HTTP ------------------------------------------------------------ */

/* POST the request body and read the response into a NUL-terminated
 * heap buffer. Prefers the server's AAAA record on a dual-stack
 * network and falls back to default resolution, like git_net.c. */
static esp_err_t post_json(const char *url, const char *body,
                           int *status_out, char **resp_out)
{
    *resp_out = NULL;
    bool try_v6 = wifi_manager_has_global_ipv6();
    esp_http_client_handle_t c = NULL;
    for (int attempt = 0; attempt < (try_v6 ? 2 : 1); attempt++) {
        esp_http_client_config_t cfg = {0};
        cfg.url = url;
        cfg.method = HTTP_METHOD_POST;
        cfg.crt_bundle_attach = esp_crt_bundle_attach;
        cfg.timeout_ms = DRAFTBOX_TIMEOUT_MS;
        cfg.addr_type = (attempt == 0 && try_v6) ? HTTP_ADDR_TYPE_INET6
                                                 : HTTP_ADDR_TYPE_UNSPEC;
        c = esp_http_client_init(&cfg);
        if (!c) return ESP_ERR_NO_MEM;
        esp_http_client_set_header(c, "User-Agent", "draftling/1.0");
        esp_http_client_set_header(c, "Content-Type", "application/json");
        esp_http_client_set_header(c, "Accept", "application/json");
        esp_http_client_set_header(c, "Accept-Encoding", "identity");
        esp_err_t err = esp_http_client_open(c, (int)strlen(body));
        if (err == ESP_OK) break;
        ESP_LOGW(TAG, "%s: connect failed (%s)", url, esp_err_to_name(err));
        esp_http_client_cleanup(c);
        c = NULL;
    }
    if (!c) return ESP_FAIL;

    esp_err_t ret = ESP_OK;
    char *resp = NULL;
    int total = 0, n;
    int len = (int)strlen(body);
    if (esp_http_client_write(c, body, len) != len) {
        ret = ESP_FAIL;
        goto out;
    }
    if (esp_http_client_fetch_headers(c) < 0) {
        ret = ESP_FAIL;
        goto out;
    }
    *status_out = esp_http_client_get_status_code(c);

    resp = (char *)malloc(DRAFTBOX_MAX_BODY + 1);
    if (!resp) { ret = ESP_ERR_NO_MEM; goto out; }
    while (total < DRAFTBOX_MAX_BODY &&
           (n = esp_http_client_read(c, resp + total, DRAFTBOX_MAX_BODY - total)) > 0)
        total += n;
    resp[total] = '\0';
    *resp_out = resp;

out:
    esp_http_client_close(c);
    esp_http_client_cleanup(c);
    return ret;
}

/* ---- public ---------------------------------------------------------- */

esp_err_t draftbox_token_exchange(const char *site, const char *otp,
                                  draftbox_result_t *res)
{
    memset(res, 0, sizeof(*res));

    char url[160];
    if (snprintf(url, sizeof(url), "https://%s/api/v1/token-exchange", site)
            >= (int)sizeof(url)) {
        snprintf(res->error, sizeof(res->error), "site name too long");
        return ESP_ERR_INVALID_ARG;
    }
    char body[64];
    snprintf(body, sizeof(body), "{\"password\":\"%s\"}", otp);

    int status = 0;
    char *resp = NULL;
    esp_err_t err = post_json(url, body, &status, &resp);
    if (err != ESP_OK) {
        snprintf(res->error, sizeof(res->error), "cannot reach %.48s", site);
        free(resp);
        return err;
    }
    ESP_LOGI(TAG, "token-exchange: HTTP %d", status);

    esp_err_t ret = ESP_FAIL;
    char code[32] = "";
    if (status == 200) {
        if (!json_get_string(resp, "token", res->token, sizeof(res->token)) ||
            !res->token[0]) {
            snprintf(res->error, sizeof(res->error), "invalid server response");
        } else if (json_is_null(resp, "cloneUrl") || json_is_null(resp, "branch")) {
            /* A global token: it is not bound to one repository, so
             * there is nothing to sync with. */
            snprintf(res->error, sizeof(res->error),
                     "global token, use a repository-specific one");
        } else if (!json_get_string(resp, "cloneUrl", res->clone_url,
                                    sizeof(res->clone_url)) ||
                   !res->clone_url[0]) {
            snprintf(res->error, sizeof(res->error), "invalid server response");
        } else {
            /* A null or missing user leaves it empty, and git.cfg then
             * gets no username= line. A missing branch (a server that
             * predates the field) likewise gets no branch= line, so the
             * default main applies. */
            if (!json_get_string(resp, "user", res->user, sizeof(res->user)))
                res->user[0] = '\0';
            if (!json_get_string(resp, "branch", res->branch, sizeof(res->branch)))
                res->branch[0] = '\0';
            if (!json_get_string(resp, "repository", res->repository,
                                 sizeof(res->repository)))
                res->repository[0] = '\0';
            char access[16] = "";
            json_get_string(resp, "access", access, sizeof(access));
            res->read_only = (strcmp(access, "read") == 0);
            if (cfg_safe(res->token) && cfg_safe(res->clone_url) &&
                cfg_safe(res->user) && cfg_safe(res->branch)) {
                ret = ESP_OK;
            } else {
                snprintf(res->error, sizeof(res->error), "invalid server response");
            }
        }
    } else if (status == 404 && resp &&
               json_get_string(resp, "error", code, sizeof(code)) &&
               strcmp(code, "invalid_password") == 0) {
        snprintf(res->error, sizeof(res->error), "wrong or expired password");
        ret = ESP_ERR_NOT_FOUND;
    } else if (status == 429) {
        snprintf(res->error, sizeof(res->error), "too many attempts, try later");
    } else {
        snprintf(res->error, sizeof(res->error), "server error (HTTP %d)", status);
    }
    free(resp);
    if (ret != ESP_OK) memset(res->token, 0, sizeof(res->token));
    return ret;
}
