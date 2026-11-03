/* git_net.c -- pkt-line framing and the "smart" Git HTTP transport
 * (gitprotocol-http): info/refs, git-upload-pack, git-receive-pack. */
#include "git_core.h"

#include <stdlib.h>
#include <string.h>

#include <esp_log.h>
#include <esp_heap_caps.h>
#include <esp_http_client.h>
#include <esp_crt_bundle.h>
#include <esp_timer.h>
#include <esp_wifi.h>

#include "wifi_manager.h"

static const char *TAG = "git.net";

#define GIT_AGENT "agent=draftling/1.0"
#define HTTP_TIMEOUT_MS 40000

/* ------------------------------------------------------------------ */
/* pkt-line                                                           */
/* ------------------------------------------------------------------ */

static bool pkt_write(git_buf *b, const char *payload, size_t n)
{
    char hdr[5];
    snprintf(hdr, sizeof(hdr), "%04x", (unsigned)(n + 4));
    if (!git_buf_put(b, hdr, 4)) return false;
    return n ? git_buf_put(b, payload, n) : true;
}

static bool pkt_flush(git_buf *b) { return git_buf_put(b, "0000", 4); }

static int hex4(const char *p)
{
    int v = 0;
    for (int i = 0; i < 4; i++) {
        int c = p[i], d;
        if (c >= '0' && c <= '9') d = c - '0';
        else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') d = c - 'A' + 10;
        else return -1;
        v = (v << 4) | d;
    }
    return v;
}

/* ------------------------------------------------------------------ */
/* shared HTTP helpers                                                */
/* ------------------------------------------------------------------ */

static esp_http_client_handle_t http_begin(const git_remote *r, const char *url,
                                           esp_http_client_method_t method,
                                           esp_http_client_addr_type_t addr_type)
{
    esp_http_client_config_t cfg = {0};
    cfg.url = url;
    cfg.method = method;
    cfg.crt_bundle_attach = esp_crt_bundle_attach;
    cfg.timeout_ms = HTTP_TIMEOUT_MS;
    cfg.buffer_size = 4096;
    cfg.buffer_size_tx = 2048;
    cfg.keep_alive_enable = true;
    cfg.addr_type = addr_type;

    esp_http_client_handle_t c = esp_http_client_init(&cfg);
    if (!c) return NULL;
    esp_http_client_set_header(c, "User-Agent", "git/draftling-1.0");
    /* We do not implement HTTP content decompression -- make sure the
     * server never gzips the packfile response. */
    esp_http_client_set_header(c, "Accept-Encoding", "identity");
    if (r->auth_basic[0])
        esp_http_client_set_header(c, "Authorization", r->auth_basic);
    return c;
}

/* Open a connection to the Git server, preferring its AAAA record
 * when our own WiFi has a global IPv6 address (dual-stack network).
 * esp-tls's getaddrinfo() hint is a hard filter, not a preference --
 * asking for AF_INET6 fails outright if the host has no AAAA record --
 * so on failure this transparently retries with the default (system)
 * resolution order instead of giving up. `extra` (if non-NULL) sets
 * any request-specific headers that must land on *every* attempt,
 * since each retry uses a brand new esp_http_client_handle_t. */
typedef void (*http_extra_headers_fn)(esp_http_client_handle_t c);

static esp_err_t http_open(esp_http_client_handle_t *out, const git_remote *r,
                           const char *url, esp_http_client_method_t method,
                           int write_len, http_extra_headers_fn extra)
{
    bool try_v6 = wifi_manager_has_global_ipv6();
    for (int attempt = 0; attempt < (try_v6 ? 2 : 1); attempt++) {
        esp_http_client_addr_type_t at =
            (attempt == 0 && try_v6) ? HTTP_ADDR_TYPE_INET6 : HTTP_ADDR_TYPE_UNSPEC;
        esp_http_client_handle_t c = http_begin(r, url, method, at);
        if (!c) { *out = NULL; return ESP_FAIL; }
        if (extra) extra(c);
        esp_err_t err = esp_http_client_open(c, write_len);
        if (err == ESP_OK) { *out = c; return ESP_OK; }
        ESP_LOGW(TAG, "%s: connect via %s failed (%s)%s", url,
                 at == HTTP_ADDR_TYPE_INET6 ? "AAAA" : "default resolution",
                 esp_err_to_name(err),
                 (attempt == 0 && try_v6) ? ", retrying via A" : "");
        esp_http_client_cleanup(c);
    }
    *out = NULL;
    return ESP_FAIL;
}

/* Hex+ASCII dump of the first bytes of a buffer, for protocol debugging. */
static void dump_head(const char *what, const uint8_t *b, size_t n)
{
    if (n > 64) n = 64;
    char hex[64 * 3 + 1];
    char asc[64 + 1];
    size_t h = 0;
    for (size_t i = 0; i < n; i++) {
        h += snprintf(hex + h, sizeof(hex) - h, "%02x ", b[i]);
        asc[i] = (b[i] >= 0x20 && b[i] < 0x7f) ? (char)b[i] : '.';
    }
    asc[n] = '\0';
    ESP_LOGW(TAG, "%s first %u bytes: %s| %s", what, (unsigned)n, hex, asc);
}

/* ------------------------------------------------------------------ */
/* info/refs                                                          */
/* ------------------------------------------------------------------ */

esp_err_t git_http_list_refs(const git_remote *r, const char *service,
                             git_ref_adv *refs, int max, int *out_count)
{
    *out_count = 0;

    char url[420];
    snprintf(url, sizeof(url), "%s/info/refs?service=%s", r->base_url, service);

    esp_http_client_handle_t c;
    esp_err_t err = http_open(&c, r, url, HTTP_METHOD_GET, 0, NULL);
    if (err != ESP_OK) return err;
    esp_http_client_fetch_headers(c);
    int status = esp_http_client_get_status_code(c);
    if (status != 200) {
        ESP_LOGE(TAG, "info/refs HTTP %d", status);
        esp_http_client_close(c);
        esp_http_client_cleanup(c);
        return (status == 401 || status == 403) ? ESP_ERR_INVALID_STATE : ESP_FAIL;
    }

    git_buf body = GIT_BUF_INIT;
    char rb[1024];
    int n;
    while ((n = esp_http_client_read(c, rb, sizeof(rb))) > 0)
        git_buf_put(&body, rb, n);
    esp_http_client_close(c);
    esp_http_client_cleanup(c);

    /* parse pkt-lines */
    size_t i = 0;
    bool seen_service = false;
    while (i + 4 <= body.len) {
        int len = hex4((char *)body.data + i);
        if (len < 0) break;
        if (len == 0) { i += 4; continue; }          /* flush */
        if ((size_t)len < 4 || i + (size_t)len > body.len) break;

        char *line = (char *)body.data + i + 4;
        int llen = len - 4;
        i += (size_t)len;

        if (!seen_service) {
            if (llen >= 9 && memcmp(line, "# service", 9) == 0) { seen_service = true; continue; }
        }
        /* "<40hex> <refname>[\0caps]\n" */
        if (llen < GIT_OID_HEXSZ + 2) continue;
        if (line[GIT_OID_HEXSZ] != ' ') continue;

        git_oid oid;
        memcpy(oid.hex, line, GIT_OID_HEXSZ);
        oid.hex[GIT_OID_HEXSZ] = '\0';

        char name[160];
        int nl = 0;
        for (int k = GIT_OID_HEXSZ + 1; k < llen && line[k] != '\n' &&
             line[k] != '\0' && nl < (int)sizeof(name) - 1; k++)
            name[nl++] = line[k];
        name[nl] = '\0';
        if (nl == 0 || strcmp(name, "capabilities^{}") == 0) continue;

        if (*out_count < max) {
            strlcpy(refs[*out_count].name, name, sizeof(refs[*out_count].name));
            refs[*out_count].oid = oid;
            (*out_count)++;
        }
    }

    git_buf_free(&body);
    return ESP_OK;
}

/* ------------------------------------------------------------------ */
/* git-upload-pack (fetch)                                            */
/* ------------------------------------------------------------------ */

static void fetch_pack_headers(esp_http_client_handle_t c)
{
    esp_http_client_set_header(c, "Content-Type", "application/x-git-upload-pack-request");
    esp_http_client_set_header(c, "Accept", "application/x-git-upload-pack-result");
    esp_http_client_set_header(c, "Git-Protocol", "version=2");
}

esp_err_t git_http_fetch_pack(const git_remote *r,
                              const git_oid *wants, int nwants,
                              const git_oid *haves, int nhaves,
                              const char *out_pack_path)
{
    if (nwants <= 0) return ESP_ERR_INVALID_ARG;

    /* Protocol v2 fetch (gitprotocol-v2). GitHub's git-upload-pack POST
     * endpoint answers in v2 whenever the "Git-Protocol: version=2"
     * header is present; the v2 "packfile" section is always side-band
     * framed, which we demux below. */
    git_buf req = GIT_BUF_INIT;
    pkt_write(&req, "command=fetch\n", 14);
    { char cap[64]; int l = snprintf(cap, sizeof(cap), "%s\n", GIT_AGENT); pkt_write(&req, cap, l); }
    git_buf_put(&req, "0001", 4);                 /* delim-pkt */
    pkt_write(&req, "ofs-delta\n", 10);
    pkt_write(&req, "no-progress\n", 12);
    for (int i = 0; i < nwants; i++) {
        char line[64];
        int l = snprintf(line, sizeof(line), "want %s\n", wants[i].hex);
        pkt_write(&req, line, (size_t)l);
    }
    for (int i = 0; i < nhaves; i++) {
        char line[64];
        int l = snprintf(line, sizeof(line), "have %s\n", haves[i].hex);
        pkt_write(&req, line, (size_t)l);
    }
    pkt_write(&req, "done\n", 5);
    pkt_flush(&req);

    char url[380];
    snprintf(url, sizeof(url), "%s/git-upload-pack", r->base_url);

    /* The response goes to a scratch file on the SD card, then the
     * packfile is demultiplexed out of it: neither is held in RAM. */
    char resp_path[160];
    snprintf(resp_path, sizeof(resp_path), "%s.resp", out_pack_path);
    FILE *rf = NULL, *fp = NULL;
    uint8_t *pkt = NULL;

    esp_http_client_handle_t c;
    esp_err_t ret = http_open(&c, r, url, HTTP_METHOD_POST, (int)req.len, fetch_pack_headers);
    if (ret != ESP_OK) { git_buf_free(&req); return ret; }
    if (esp_http_client_write(c, (char *)req.data, (int)req.len) != (int)req.len) {
        ret = ESP_FAIL;
        goto cleanup;
    }
    esp_http_client_fetch_headers(c);
    int status = esp_http_client_get_status_code(c);
    {
        char *ctype = NULL;
        esp_http_client_get_header(c, "Content-Type", &ctype);
        ESP_LOGI(TAG, "upload-pack HTTP %d, Content-Type: %s", status,
                 ctype ? ctype : "(none)");
    }
    if (status != 200) {
        ESP_LOGE(TAG, "upload-pack HTTP %d", status);
        ret = (status == 401 || status == 403) ? ESP_ERR_INVALID_STATE : ESP_FAIL;
        goto cleanup;
    }

    /* pkt-line payloads are at most 65516 bytes */
    pkt = heap_caps_malloc(65536, MALLOC_CAP_SPIRAM);
    if (!pkt) pkt = malloc(65536);
    if (!pkt) { ret = ESP_ERR_NO_MEM; goto cleanup; }

    rf = fopen(resp_path, "w+b");
    if (!rf) { ret = ESP_FAIL; goto cleanup; }
    {
        size_t total = 0;
        int n;
        while ((n = esp_http_client_read(c, (char *)pkt, 4096)) > 0) {
            if (fwrite(pkt, 1, (size_t)n, rf) != (size_t)n) { ret = ESP_FAIL; goto cleanup; }
            if (total == 0) dump_head("fetch response", pkt, (size_t)n);
            total += (size_t)n;
        }
        if (n < 0 || total == 0) { ret = ESP_FAIL; goto cleanup; }
        ESP_LOGI(TAG, "fetch response: %u bytes", (unsigned)total);
    }
    esp_http_client_close(c);
    esp_http_client_cleanup(c);
    c = NULL;
    rewind(rf);

    fp = fopen(out_pack_path, "wb");
    if (!fp) { ret = ESP_FAIL; goto cleanup; }

    {
        enum { SEC_NONE, SEC_PACKFILE, SEC_SKIP } sec = SEC_NONE;
        bool wrote = false, err = false, seen_ack = false;

        for (;;) {
            long pos = ftell(rf);
            char h[4];
            if (fread(h, 1, 4, rf) != 4) break;
            int plen = hex4(h);
            if (plen < 0) {
                /* not pkt-framed here: v0 raw packfile tail */
                if (seen_ack || sec == SEC_PACKFILE) {
                    fseek(rf, pos, SEEK_SET);
                    size_t k;
                    while ((k = fread(pkt, 1, 4096, rf)) > 0) fwrite(pkt, 1, k, fp);
                    wrote = true;
                }
                break;
            }
            if (plen == 0) { if (sec == SEC_PACKFILE) sec = SEC_NONE; continue; }
            if (plen >= 1 && plen <= 3) continue;  /* delim / keepalive / end */
            if (plen < 4) break;

            const uint8_t *pl = pkt;
            int pll = plen - 4;
            if (fread(pkt, 1, (size_t)pll, rf) != (size_t)pll) break;

            if (sec != SEC_PACKFILE) {
                if (pll >= 8 && memcmp(pl, "packfile", 8) == 0) { sec = SEC_PACKFILE; continue; }
                if ((pll >= 15 && memcmp(pl, "acknowledgments", 15) == 0) ||
                    (pll >= 12 && memcmp(pl, "shallow-info", 12) == 0) ||
                    (pll >= 11 && memcmp(pl, "wanted-refs", 11) == 0) ||
                    (pll >= 13 && memcmp(pl, "packfile-uris", 13) == 0)) { sec = SEC_SKIP; continue; }
            }

            /* side-band framed data (v2 packfile, or a sideband v0 server) */
            if (sec == SEC_PACKFILE || (pll >= 1 && (pl[0] == 1 || pl[0] == 2 || pl[0] == 3))) {
                if (pll < 1) continue;
                uint8_t band = pl[0];
                if (band == 1) {
                    if (fwrite(pl + 1, 1, pll - 1, fp) != (size_t)(pll - 1)) { err = true; break; }
                    wrote = true;
                } else if (band == 3) {
                    ESP_LOGE(TAG, "remote: %.*s", pll - 1, (const char *)pl + 1);
                    err = true;
                    break;
                }
                continue;
            }

            /* v0 negotiation lines */
            if (pll >= 3 && memcmp(pl, "NAK", 3) == 0) { seen_ack = true; continue; }
            if (pll >= 3 && memcmp(pl, "ACK", 3) == 0) { seen_ack = true; continue; }
            if (pll >= 4 && memcmp(pl, "ERR ", 4) == 0) {
                ESP_LOGE(TAG, "remote: %.*s", pll - 4, (const char *)pl + 4);
                err = true;
                break;
            }
            /* anything else: ignore */
        }

        /* last-ditch: locate the packfile signature directly */
        if (!wrote && !err) {
            static const uint8_t sig[7] = { 'P', 'A', 'C', 'K', 0, 0, 0 };
            rewind(rf);
            long base = 0;
            size_t keep = 0, k;
            long found = -1;
            /* chunks overlap by 7 bytes so a split signature is seen */
            while (found < 0 && (k = fread(pkt + keep, 1, 4096, rf)) > 0) {
                size_t n = keep + k;
                for (size_t i = 0; i + 8 <= n; i++) {
                    if (memcmp(pkt + i, sig, 7) == 0 && (pkt[i + 7] == 2 || pkt[i + 7] == 3)) {
                        found = base + (long)i;
                        break;
                    }
                }
                keep = n < 7 ? n : 7;
                memmove(pkt, pkt + n - keep, keep);
                base += (long)(n - keep);
            }
            if (found >= 0) {
                fseek(rf, found, SEEK_SET);
                while ((k = fread(pkt, 1, 4096, rf)) > 0) fwrite(pkt, 1, k, fp);
                wrote = true;
            }
        }

        if (fclose(fp) != 0) wrote = false;
        fp = NULL;
        ret = (wrote && !err) ? ESP_OK : ESP_FAIL;
        if (ret != ESP_OK) {
            remove(out_pack_path);
            if (!err) ESP_LOGE(TAG, "no packfile found in upload-pack response");
        }
    }

cleanup:
    if (fp) fclose(fp);
    if (rf) { fclose(rf); remove(resp_path); }
    if (c) {
        esp_http_client_close(c);
        esp_http_client_cleanup(c);
    }
    free(pkt);
    git_buf_free(&req);
    return ret;
}

/* ------------------------------------------------------------------ */
/* git-receive-pack (push)                                            */
/* ------------------------------------------------------------------ */

static void push_headers(esp_http_client_handle_t c)
{
    esp_http_client_set_header(c, "Content-Type", "application/x-git-receive-pack-request");
    esp_http_client_set_header(c, "Accept", "application/x-git-receive-pack-result");
}

esp_err_t git_http_push(const git_remote *r, const char *refname,
                        const git_oid *old_oid, const git_oid *new_oid,
                        const char *pack_path, git_buf *report)
{
    const char *old_hex = git_oid_is_zero(old_oid)
        ? "0000000000000000000000000000000000000000" : old_oid->hex;

    /* command list: "<old> <new> <ref>\0<capabilities>\n" (the NUL and
     * capability list must be inside a single pkt-line). */
    git_buf cmds = GIT_BUF_INIT;
    char line[256];
    int l = snprintf(line, sizeof(line), "%s %s %s", old_hex, new_oid->hex, refname);
    line[l++] = '\0';
    l += snprintf(line + l, sizeof(line) - l, "report-status %s\n", GIT_AGENT);
    pkt_write(&cmds, line, (size_t)l);
    pkt_flush(&cmds);

    FILE *pf = fopen(pack_path, "rb");
    if (!pf) { git_buf_free(&cmds); return ESP_FAIL; }
    fseek(pf, 0, SEEK_END);
    long pack_size = ftell(pf);
    fseek(pf, 0, SEEK_SET);

    char url[380];
    snprintf(url, sizeof(url), "%s/git-receive-pack", r->base_url);

    int total = (int)cmds.len + (int)pack_size;
    esp_http_client_handle_t c;
    esp_err_t ret = http_open(&c, r, url, HTTP_METHOD_POST, total, push_headers);
    if (ret != ESP_OK) { fclose(pf); git_buf_free(&cmds); return ret; }

    ESP_LOGI(TAG, "push: sending %d bytes (pack %ld)", total, pack_size);
    if (esp_http_client_write(c, (char *)cmds.data, (int)cmds.len) != (int)cmds.len) {
        ret = ESP_ERR_TIMEOUT;
        goto cleanup;
    }
    {
        /* Connection trouble while sending is reported as
         * ESP_ERR_TIMEOUT (not a rejection), with how far it got. */
        char chunk[2048];
        size_t rd;
        long sent = 0, next_log = 64 * 1024;
        int64_t t0 = esp_timer_get_time();
        while ((rd = fread(chunk, 1, sizeof(chunk), pf)) > 0) {
            if (esp_http_client_write(c, chunk, (int)rd) != (int)rd) {
                wifi_ap_record_t ap;
                int rssi = esp_wifi_sta_get_ap_info(&ap) == ESP_OK ? ap.rssi : 0;
                ESP_LOGE(TAG, "push: connection lost after %ld of %ld pack bytes, "
                         "%lld ms, RSSI %d dBm, free PSRAM %u, internal %u",
                         sent, pack_size, (esp_timer_get_time() - t0) / 1000, rssi,
                         (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
                         (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
                ret = ESP_ERR_TIMEOUT;
                goto cleanup;
            }
            sent += (long)rd;
            if (sent >= next_log) {
                ESP_LOGI(TAG, "push: %ld / %ld bytes, %lld ms", sent, pack_size,
                         (esp_timer_get_time() - t0) / 1000);
                next_log += 64 * 1024;
            }
        }
        ESP_LOGI(TAG, "push: all %ld bytes sent in %lld ms, waiting for the server",
                 sent, (esp_timer_get_time() - t0) / 1000);
    }

    if (esp_http_client_fetch_headers(c) < 0) {
        ESP_LOGE(TAG, "push: no answer from the server");
        ret = ESP_ERR_TIMEOUT;
        goto cleanup;
    }
    int status = esp_http_client_get_status_code(c);
    if (status != 200) {
        ESP_LOGE(TAG, "receive-pack HTTP %d", status);
        ret = ESP_FAIL;
        goto cleanup;
    }

    {
        git_buf raw = GIT_BUF_INIT;
        char rb[512];
        int n;
        while ((n = esp_http_client_read(c, rb, sizeof(rb))) > 0)
            git_buf_put(&raw, rb, n);

        /* unwrap pkt-lines into `report` */
        size_t i = 0;
        bool unpack_ok = false, ref_ok = false;
        while (i + 4 <= raw.len) {
            int len = hex4((char *)raw.data + i);
            if (len < 0) break;
            if (len == 0) { i += 4; continue; }
            if (i + (size_t)len > raw.len) break;
            const char *line2 = (char *)raw.data + i + 4;
            int llen = len - 4;
            /* strip a leading side-band byte 0x01 if present */
            if (llen > 0 && (uint8_t)line2[0] == 0x01) { line2++; llen--; }
            git_buf_put(report, line2, llen);
            if (llen >= 9 && memcmp(line2, "unpack ok", 9) == 0) unpack_ok = true;
            if (llen >= 3 && memcmp(line2, "ok ", 3) == 0) ref_ok = true;
            i += (size_t)len;
        }
        git_buf_free(&raw);
        ret = (unpack_ok && ref_ok) ? ESP_OK : ESP_FAIL;
    }

cleanup:
    esp_http_client_close(c);
    esp_http_client_cleanup(c);
    fclose(pf);
    git_buf_free(&cmds);
    return ret;
}
