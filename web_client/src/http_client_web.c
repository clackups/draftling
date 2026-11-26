/*
 * Web client: esp_http_client for the Git client (git_net.c) and
 * Draftbox (draftbox.c), over the browser's own network connection.
 *
 * The callers run on worker threads (the Git sync and Draftbox tasks
 * are pthreads, i.e. Web Workers), where a synchronous XMLHttpRequest
 * is allowed, so the blocking API maps onto one request per handle:
 * open() and write() collect the request body, fetch_headers() sends
 * the request and waits for the whole response, read() hands out its
 * body. The browser does TLS, proxies and HTTP/2, and decompresses
 * the body itself.
 *
 * The page is on another origin than the Git server, so the server
 * must allow the request with CORS headers -- Draftbox does, for its
 * Git and JSON API paths (Authorization, Content-Type and Git-Protocol
 * request headers). Headers a page may not set (User-Agent,
 * Accept-Encoding, ...) are left to the browser. A request the browser
 * refuses (no network, CORS, certificate) fails like a refused
 * connection: fetch_headers() returns -1.
 */

#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <emscripten.h>

#include "esp_http_client.h"
#include "esp_log.h"

static const char *TAG = "HTTP";

#define MAX_HEADERS 32
#define DEFAULT_TIMEOUT_MS 30000

typedef struct {
    char *key, *value;
} hdr_t;

struct linux_http_client {
    char *url;
    esp_http_client_method_t method;
    int timeout_ms;
    hdr_t req[MAX_HEADERS];
    int n_req;
    hdr_t resp[MAX_HEADERS];
    int n_resp;
    bool opened, sent;
    /* request body */
    uint8_t *body;
    size_t body_len, body_cap;
    /* response */
    int status;
    uint8_t *data;
    size_t data_len, data_pos;
};

EM_JS_DEPS(http_client_web, "$UTF8ToString,$stringToNewUTF8,$lengthBytesUTF8");

/* Send the request synchronously. Returns the status (0 when the
 * browser refused or failed it), the response headers as one malloc()ed
 * "name: value\r\n..." string and the body as a malloc()ed buffer. */
EM_JS(int, web_xhr, (const char *method, const char *url, const char *headers,
                     const uint8_t *body, int body_len, int timeout_ms,
                     char **resp_headers, uint8_t **resp_body, int *resp_len), {
    var forbidden = /^(user-agent|accept-encoding|host|content-length|connection|keep-alive|transfer-encoding)$/i;
    try {
        var xhr = new XMLHttpRequest();
        xhr.open(UTF8ToString(method), UTF8ToString(url), false);
        xhr.responseType = 'arraybuffer';
        if (timeout_ms > 0) xhr.timeout = timeout_ms;
        var lines = UTF8ToString(headers).split('\n');
        for (var i = 0; i < lines.length; i++) {
            var colon = lines[i].indexOf(':');
            if (colon <= 0) continue;
            var k = lines[i].slice(0, colon);
            if (forbidden.test(k)) continue;
            xhr.setRequestHeader(k, lines[i].slice(colon + 1).trim());
        }
        /* slice(): a copy outside the shared memory, which send()
         * does not take. */
        xhr.send(body_len > 0 ? HEAPU8.slice(body, body + body_len) : null);
        var data = new Uint8Array(xhr.response || new ArrayBuffer(0));
        var p = 0;
        if (data.length) {
            p = _malloc(data.length);
            if (!p) return 0;
            HEAPU8.set(data, p);
        }
        HEAPU32[resp_body >> 2] = p;
        HEAP32[resp_len >> 2] = data.length;
        HEAPU32[resp_headers >> 2] = stringToNewUTF8(xhr.getAllResponseHeaders() || "");
        return xhr.status;
    } catch (e) {
        console.warn('Draftling: ' + UTF8ToString(method) + ' ' + UTF8ToString(url) +
                     ' failed: ' + e);
        return 0;
    }
});

esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t *cfg)
{
    if (!cfg || !cfg->url) return NULL;
    if (strncasecmp(cfg->url, "https://", 8) != 0 && strncasecmp(cfg->url, "http://", 7) != 0) {
        ESP_LOGE(TAG, "Bad URL: %s", cfg->url);
        return NULL;
    }
    struct linux_http_client *c = calloc(1, sizeof(*c));
    if (!c) return NULL;
    c->url = strdup(cfg->url);
    c->method = cfg->method;
    c->timeout_ms = cfg->timeout_ms > 0 ? cfg->timeout_ms : DEFAULT_TIMEOUT_MS;
    if (!c->url) {
        free(c);
        return NULL;
    }
    return c;
}

static void set_hdr(hdr_t *list, int *n, const char *k, const char *v)
{
    for (int i = 0; i < *n; i++) {
        if (strcasecmp(list[i].key, k) == 0) {
            free(list[i].value);
            list[i].value = strdup(v);
            return;
        }
    }
    if (*n >= MAX_HEADERS) return;
    list[*n].key = strdup(k);
    list[*n].value = strdup(v);
    (*n)++;
}

esp_err_t esp_http_client_set_header(esp_http_client_handle_t c, const char *key, const char *value)
{
    if (!c || !key || !value) return ESP_ERR_INVALID_ARG;
    set_hdr(c->req, &c->n_req, key, value);
    return ESP_OK;
}

esp_err_t esp_http_client_get_header(esp_http_client_handle_t c, const char *key, char **value)
{
    if (!c || !key || !value) return ESP_ERR_INVALID_ARG;
    *value = NULL;
    for (int i = 0; i < c->n_resp; i++) {
        if (strcasecmp(c->resp[i].key, key) == 0) {
            *value = c->resp[i].value;
            break;
        }
    }
    return ESP_OK;
}

esp_err_t esp_http_client_open(esp_http_client_handle_t c, int write_len)
{
    if (!c) return ESP_ERR_INVALID_ARG;
    if (write_len > 0) {
        c->body = malloc((size_t)write_len);
        if (!c->body) return ESP_ERR_NO_MEM;
        c->body_cap = (size_t)write_len;
    }
    c->opened = true;
    return ESP_OK;
}

int esp_http_client_write(esp_http_client_handle_t c, const char *buf, int len)
{
    if (!c || !c->opened || c->sent || len < 0) return -1;
    if (c->body_len + (size_t)len > c->body_cap) {
        size_t cap = c->body_cap ? c->body_cap : 1024;
        while (cap < c->body_len + (size_t)len) cap *= 2;
        uint8_t *nb = realloc(c->body, cap);
        if (!nb) return -1;
        c->body = nb;
        c->body_cap = cap;
    }
    memcpy(c->body + c->body_len, buf, (size_t)len);
    c->body_len += (size_t)len;
    return len;
}

static void parse_headers(struct linux_http_client *c, char *all)
{
    for (char *line = all; line && *line;) {
        char *end = strstr(line, "\r\n");
        if (end) *end = '\0';
        char *colon = strchr(line, ':');
        if (colon) {
            *colon = '\0';
            char *v = colon + 1;
            while (*v == ' ' || *v == '\t') v++;
            set_hdr(c->resp, &c->n_resp, line, v);
        }
        line = end ? end + 2 : NULL;
    }
}

int64_t esp_http_client_fetch_headers(esp_http_client_handle_t c)
{
    if (!c || !c->opened) return -1;
    if (c->sent) return (int64_t)c->data_len;
    c->sent = true;

    size_t cap = 1;
    for (int i = 0; i < c->n_req; i++) cap += strlen(c->req[i].key) + strlen(c->req[i].value) + 3;
    char *hdrs = malloc(cap);
    if (!hdrs) return -1;
    size_t n = 0;
    hdrs[0] = '\0';
    for (int i = 0; i < c->n_req; i++)
        n += (size_t)snprintf(hdrs + n, cap - n, "%s: %s\n", c->req[i].key, c->req[i].value);

    char *resp_headers = NULL;
    uint8_t *resp_body = NULL;
    int resp_len = 0;
    const char *method = c->method == HTTP_METHOD_POST ? "POST" : "GET";
    int status = web_xhr(method, c->url, hdrs, c->body, (int)c->body_len, c->timeout_ms,
                         &resp_headers, &resp_body, &resp_len);
    free(hdrs);
    if (status <= 0) {
        free(resp_headers);
        free(resp_body);
        ESP_LOGW(TAG, "%s %s: no response (network, or not allowed by the server's CORS headers)",
                 method, c->url);
        return -1;
    }
    c->status = status;
    c->data = resp_body;
    c->data_len = (size_t)resp_len;
    if (resp_headers) {
        parse_headers(c, resp_headers);
        free(resp_headers);
    }
    return (int64_t)c->data_len;
}

int esp_http_client_get_status_code(esp_http_client_handle_t c)
{
    return c ? c->status : -1;
}

int esp_http_client_read(esp_http_client_handle_t c, char *buf, int len)
{
    if (!c || !c->sent || c->status <= 0 || len < 0) return -1;
    size_t left = c->data_len - c->data_pos;
    size_t n = left < (size_t)len ? left : (size_t)len;
    memcpy(buf, c->data + c->data_pos, n);
    c->data_pos += n;
    return (int)n;
}

esp_err_t esp_http_client_close(esp_http_client_handle_t c)
{
    if (!c) return ESP_ERR_INVALID_ARG;
    return ESP_OK;
}

esp_err_t esp_http_client_cleanup(esp_http_client_handle_t c)
{
    if (!c) return ESP_ERR_INVALID_ARG;
    for (int i = 0; i < c->n_req; i++) { free(c->req[i].key); free(c->req[i].value); }
    for (int i = 0; i < c->n_resp; i++) { free(c->resp[i].key); free(c->resp[i].value); }
    free(c->body);
    free(c->data);
    free(c->url);
    free(c);
    return ESP_OK;
}
