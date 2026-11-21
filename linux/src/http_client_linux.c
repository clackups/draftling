/*
 * Linux: esp_http_client for the Git client (git_net.c) and Draftbox
 * (draftbox.c), on the host's sockets and OpenSSL.
 *
 * One request per handle, as the callers use it: open() connects
 * (IPv6 only when the caller asks for HTTP_ADDR_TYPE_INET6, as on the
 * device, which then retries with default resolution) and sends the
 * request head with the body's length; write() sends the body;
 * fetch_headers() reads the status line and headers; read() returns
 * the body (Content-Length, chunked, or until the server closes).
 * Redirects are not followed, as with esp_http_client's streaming
 * calls. TLS checks the server certificate against the system's CA
 * certificates and the host name.
 */

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>

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
    bool tls;
    char host[256];
    int port;
    char *path;
    esp_http_client_method_t method;
    esp_http_client_addr_type_t addr_type;
    int timeout_ms;
    hdr_t req[MAX_HEADERS];
    int n_req;
    hdr_t resp[MAX_HEADERS];
    int n_resp;
    int fd;
    SSL *ssl;
    int status;
    /* response body framing */
    bool chunked;
    long long remaining;   /* Content-Length left, or chunk left; -1 = to EOF */
    bool body_done;
    /* read buffer */
    char rbuf[16384];
    size_t rpos, rlen;
};

static SSL_CTX *s_ctx;

static SSL_CTX *tls_ctx(void)
{
    if (s_ctx) return s_ctx;
    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) return NULL;
    SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
    /* OpenSSL's built-in default, then the usual bundle locations
     * (Debian / Ubuntu, Fedora / RHEL, openSUSE, the BSDs, Alpine). */
    SSL_CTX_set_default_verify_paths(ctx);
    static const char *const bundles[] = {
        "/etc/ssl/certs/ca-certificates.crt",
        "/etc/pki/tls/certs/ca-bundle.crt",
        "/etc/ssl/ca-bundle.pem",
        "/etc/ssl/cert.pem",
        "/usr/local/share/certs/ca-root-nss.crt",
        "/usr/local/etc/ssl/cert.pem",
    };
    for (size_t i = 0; i < sizeof(bundles) / sizeof(bundles[0]); i++) {
        if (access(bundles[i], R_OK) == 0) SSL_CTX_load_verify_locations(ctx, bundles[i], NULL);
    }
    if (access("/etc/ssl/certs", R_OK) == 0)
        SSL_CTX_load_verify_locations(ctx, NULL, "/etc/ssl/certs");
    ERR_clear_error();
    s_ctx = ctx;
    return ctx;
}

static void log_tls_error(const char *what)
{
    unsigned long e = ERR_get_error();
    char buf[256] = "unknown error";
    if (e) ERR_error_string_n(e, buf, sizeof(buf));
    ESP_LOGW(TAG, "%s: %s", what, buf);
    ERR_clear_error();
}

static bool parse_url(struct linux_http_client *c, const char *url)
{
    const char *p;
    if (strncasecmp(url, "https://", 8) == 0) {
        c->tls = true;
        c->port = 443;
        p = url + 8;
    } else if (strncasecmp(url, "http://", 7) == 0) {
        c->tls = false;
        c->port = 80;
        p = url + 7;
    } else {
        return false;
    }
    const char *end = p + strcspn(p, "/?#");
    const char *at = memchr(p, '@', (size_t)(end - p));
    if (at) p = at + 1;   /* user info: the callers send Authorization */
    const char *hs = p, *he;
    if (*p == '[') {
        he = memchr(p, ']', (size_t)(end - p));
        if (!he) return false;
        hs = p + 1;
        p = he + 1;
    } else {
        he = p;
        while (he < end && *he != ':') he++;
        p = he;
    }
    size_t hl = (size_t)(he - hs);
    if (hl == 0 || hl >= sizeof(c->host)) return false;
    memcpy(c->host, hs, hl);
    c->host[hl] = '\0';
    if (p < end && *p == ':') {
        c->port = atoi(p + 1);
        if (c->port <= 0 || c->port > 65535) return false;
    }
    c->path = strdup(*end == '/' ? end : "/");
    if (*end == '?') {   /* "host?query" */
        free(c->path);
        size_t n = strlen(end) + 2;
        c->path = malloc(n);
        if (c->path) snprintf(c->path, n, "/%s", end);
    }
    if (c->path) c->path[strcspn(c->path, "#")] = '\0';
    return c->path != NULL;
}

esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t *cfg)
{
    if (!cfg || !cfg->url) return NULL;
    struct linux_http_client *c = calloc(1, sizeof(*c));
    if (!c) return NULL;
    c->fd = -1;
    c->url = strdup(cfg->url);
    c->method = cfg->method;
    c->addr_type = cfg->addr_type;
    c->timeout_ms = cfg->timeout_ms > 0 ? cfg->timeout_ms : DEFAULT_TIMEOUT_MS;
    if (!c->url || !parse_url(c, cfg->url)) {
        ESP_LOGE(TAG, "Bad URL: %s", cfg->url);
        esp_http_client_cleanup(c);
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

/* ---- transport ------------------------------------------------------------- */

static bool wait_fd(int fd, short ev, int timeout_ms)
{
    struct pollfd p = { .fd = fd, .events = ev };
    for (;;) {
        int r = poll(&p, 1, timeout_ms);
        if (r < 0 && errno == EINTR) continue;
        return r > 0;
    }
}

static int connect_host(struct linux_http_client *c)
{
    struct addrinfo hints, *res = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = c->addr_type == HTTP_ADDR_TYPE_INET6 ? AF_INET6
                    : c->addr_type == HTTP_ADDR_TYPE_INET  ? AF_INET : AF_UNSPEC;
    char port[8];
    snprintf(port, sizeof(port), "%d", c->port);
    int gr = getaddrinfo(c->host, port, &hints, &res);
    if (gr != 0) {
        ESP_LOGW(TAG, "%s: %s", c->host, gai_strerror(gr));
        return -1;
    }
    int fd = -1;
    for (struct addrinfo *a = res; a; a = a->ai_next) {
        fd = socket(a->ai_family, a->ai_socktype | SOCK_CLOEXEC, a->ai_protocol);
        if (fd < 0) continue;
        int fl = fcntl(fd, F_GETFL);
        fcntl(fd, F_SETFL, fl | O_NONBLOCK);
        int r = connect(fd, a->ai_addr, a->ai_addrlen);
        if (r != 0 && errno == EINPROGRESS && wait_fd(fd, POLLOUT, c->timeout_ms)) {
            int err = 0;
            socklen_t el = sizeof(err);
            getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &el);
            r = err ? -1 : 0;
        }
        if (r == 0) {
            fcntl(fd, F_SETFL, fl & ~O_NONBLOCK);
            struct timeval tv = { c->timeout_ms / 1000, (c->timeout_ms % 1000) * 1000 };
            setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
            setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
            int one = 1;
            setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
            break;
        }
        close(fd);
        fd = -1;
    }
    freeaddrinfo(res);
    if (fd < 0) ESP_LOGW(TAG, "%s:%d: cannot connect", c->host, c->port);
    return fd;
}

static int raw_write(struct linux_http_client *c, const char *buf, size_t len)
{
    size_t done = 0;
    while (done < len) {
        int n;
        if (c->ssl) {
            n = SSL_write(c->ssl, buf + done, (int)(len - done));
            if (n <= 0) { log_tls_error("TLS write"); return -1; }
        } else {
            n = (int)send(c->fd, buf + done, len - done, MSG_NOSIGNAL);
            if (n < 0 && errno == EINTR) continue;
            if (n <= 0) return -1;
        }
        done += (size_t)n;
    }
    return (int)done;
}

/* Fill the read buffer; returns bytes now available, 0 at EOF, -1 on error. */
static int fill(struct linux_http_client *c)
{
    if (c->rpos < c->rlen) return (int)(c->rlen - c->rpos);
    c->rpos = c->rlen = 0;
    int n;
    for (;;) {
        if (c->ssl) {
            n = SSL_read(c->ssl, c->rbuf, (int)sizeof(c->rbuf));
            if (n <= 0) {
                int e = SSL_get_error(c->ssl, n);
                if (e == SSL_ERROR_ZERO_RETURN) return 0;
                /* A server closing without close_notify ends the body
                 * too (HTTP/1.0 style). */
                if (e == SSL_ERROR_SYSCALL && ERR_peek_error() == 0 && n == 0) return 0;
                log_tls_error("TLS read");
                return -1;
            }
        } else {
            n = (int)recv(c->fd, c->rbuf, sizeof(c->rbuf), 0);
            if (n < 0 && errno == EINTR) continue;
            if (n < 0) return -1;
        }
        break;
    }
    c->rlen = (size_t)n;
    return n;
}

/* One line (CRLF or LF ended, without it) into out; false on EOF / error. */
static bool read_line(struct linux_http_client *c, char *out, size_t sz)
{
    size_t n = 0;
    for (;;) {
        int a = fill(c);
        if (a <= 0) return false;
        char ch = c->rbuf[c->rpos++];
        if (ch == '\n') break;
        if (n + 1 < sz) out[n++] = ch;
    }
    if (n && out[n - 1] == '\r') n--;
    out[n] = '\0';
    return true;
}

esp_err_t esp_http_client_open(esp_http_client_handle_t c, int write_len)
{
    if (!c) return ESP_ERR_INVALID_ARG;
    c->fd = connect_host(c);
    if (c->fd < 0) return ESP_FAIL;
    if (c->tls) {
        SSL_CTX *ctx = tls_ctx();
        c->ssl = ctx ? SSL_new(ctx) : NULL;
        if (!c->ssl) { log_tls_error("TLS"); return ESP_FAIL; }
        SSL_set_fd(c->ssl, c->fd);
        SSL_set_tlsext_host_name(c->ssl, c->host);
        SSL_set1_host(c->ssl, c->host);
        if (SSL_connect(c->ssl) != 1) {
            long vr = SSL_get_verify_result(c->ssl);
            if (vr != X509_V_OK)
                ESP_LOGW(TAG, "%s: certificate: %s", c->host, X509_verify_cert_error_string(vr));
            log_tls_error("TLS handshake");
            return ESP_FAIL;
        }
    }
    size_t cap = 1024 + strlen(c->path) + strlen(c->host);
    for (int i = 0; i < c->n_req; i++) cap += strlen(c->req[i].key) + strlen(c->req[i].value) + 4;
    char *head = malloc(cap);
    if (!head) return ESP_ERR_NO_MEM;
    size_t n = 0;
    bool default_port = c->port == (c->tls ? 443 : 80);
    bool v6lit = strchr(c->host, ':') != NULL;
    n += (size_t)snprintf(head + n, cap - n, "%s %s HTTP/1.1\r\nHost: %s%s%s",
                          c->method == HTTP_METHOD_POST ? "POST" : "GET", c->path,
                          v6lit ? "[" : "", c->host, v6lit ? "]" : "");
    if (!default_port) n += (size_t)snprintf(head + n, cap - n, ":%d", c->port);
    n += (size_t)snprintf(head + n, cap - n, "\r\n");
    for (int i = 0; i < c->n_req; i++) {
        if (strcasecmp(c->req[i].key, "Host") == 0 ||
            strcasecmp(c->req[i].key, "Content-Length") == 0 ||
            strcasecmp(c->req[i].key, "Connection") == 0) continue;
        n += (size_t)snprintf(head + n, cap - n, "%s: %s\r\n", c->req[i].key, c->req[i].value);
    }
    if (c->method == HTTP_METHOD_POST || write_len > 0)
        n += (size_t)snprintf(head + n, cap - n, "Content-Length: %d\r\n", write_len > 0 ? write_len : 0);
    n += (size_t)snprintf(head + n, cap - n, "Connection: close\r\n\r\n");
    int w = raw_write(c, head, n);
    free(head);
    return w == (int)n ? ESP_OK : ESP_FAIL;
}

int esp_http_client_write(esp_http_client_handle_t c, const char *buf, int len)
{
    if (!c || c->fd < 0 || len < 0) return -1;
    return raw_write(c, buf, (size_t)len);
}

int64_t esp_http_client_fetch_headers(esp_http_client_handle_t c)
{
    if (!c || c->fd < 0) return -1;
    char line[8192];
    /* Skip interim 1xx responses. */
    for (;;) {
        if (!read_line(c, line, sizeof(line))) return -1;
        int major, minor, status;
        if (sscanf(line, "HTTP/%d.%d %d", &major, &minor, &status) != 3) return -1;
        c->status = status;
        for (;;) {
            if (!read_line(c, line, sizeof(line))) return -1;
            if (!line[0]) break;
            char *colon = strchr(line, ':');
            if (!colon) continue;
            *colon = '\0';
            char *v = colon + 1;
            while (*v == ' ' || *v == '\t') v++;
            if (status >= 200) set_hdr(c->resp, &c->n_resp, line, v);
        }
        if (status >= 200) break;
    }
    char *te = NULL, *cl = NULL;
    esp_http_client_get_header(c, "Transfer-Encoding", &te);
    esp_http_client_get_header(c, "Content-Length", &cl);
    c->chunked = te && strcasestr(te, "chunked");
    if (c->chunked) {
        c->remaining = 0;   /* read the first chunk size on demand */
        return 0;
    }
    if (cl) {
        c->remaining = strtoll(cl, NULL, 10);
        if (c->remaining == 0) c->body_done = true;
        return c->remaining;
    }
    c->remaining = -1;
    return 0;
}

int esp_http_client_get_status_code(esp_http_client_handle_t c)
{
    return c ? c->status : -1;
}

int esp_http_client_read(esp_http_client_handle_t c, char *buf, int len)
{
    if (!c || c->fd < 0 || len <= 0) return -1;
    if (c->body_done) return 0;
    if (c->chunked && c->remaining == 0) {
        char line[128];
        if (!read_line(c, line, sizeof(line))) return -1;
        long long sz = strtoll(line, NULL, 16);
        if (sz <= 0) {
            /* Last chunk: skip the trailer. */
            while (read_line(c, line, sizeof(line)) && line[0]) { }
            c->body_done = true;
            return 0;
        }
        c->remaining = sz;
    }
    int a = fill(c);
    if (a < 0) return -1;
    if (a == 0) {
        if (c->remaining > 0) return -1;   /* cut short */
        c->body_done = true;
        return 0;
    }
    size_t n = (size_t)a < (size_t)len ? (size_t)a : (size_t)len;
    if (c->remaining >= 0 && (long long)n > c->remaining) n = (size_t)c->remaining;
    memcpy(buf, c->rbuf + c->rpos, n);
    c->rpos += n;
    if (c->remaining >= 0) {
        c->remaining -= (long long)n;
        if (c->remaining == 0) {
            if (c->chunked) {
                char crlf[8];
                if (!read_line(c, crlf, sizeof(crlf))) return -1;
            } else {
                c->body_done = true;
            }
        }
    }
    return (int)n;
}

esp_err_t esp_http_client_close(esp_http_client_handle_t c)
{
    if (!c) return ESP_ERR_INVALID_ARG;
    if (c->ssl) {
        SSL_shutdown(c->ssl);
        SSL_free(c->ssl);
        c->ssl = NULL;
    }
    if (c->fd >= 0) {
        close(c->fd);
        c->fd = -1;
    }
    return ESP_OK;
}

esp_err_t esp_http_client_cleanup(esp_http_client_handle_t c)
{
    if (!c) return ESP_ERR_INVALID_ARG;
    esp_http_client_close(c);
    for (int i = 0; i < c->n_req; i++) { free(c->req[i].key); free(c->req[i].value); }
    for (int i = 0; i < c->n_resp; i++) { free(c->resp[i].key); free(c->resp[i].value); }
    free(c->path);
    free(c->url);
    free(c);
    return ESP_OK;
}
