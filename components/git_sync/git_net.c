/* git_net.c -- pkt-line framing and the "smart" Git HTTP transport
 * (gitprotocol-http): info/refs, git-upload-pack, git-receive-pack. */
#include "git_core.h"

#include <stdlib.h>
#include <string.h>

#include <esp_log.h>
#include <esp_heap_caps.h>
#include <esp_http_client.h>
#include <esp_crt_bundle.h>

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
                                           esp_http_client_method_t method)
{
    esp_http_client_config_t cfg = {0};
    cfg.url = url;
    cfg.method = method;
    cfg.crt_bundle_attach = esp_crt_bundle_attach;
    cfg.timeout_ms = HTTP_TIMEOUT_MS;
    cfg.buffer_size = 4096;
    cfg.buffer_size_tx = 2048;
    cfg.keep_alive_enable = true;

    esp_http_client_handle_t c = esp_http_client_init(&cfg);
    if (!c) return NULL;
    esp_http_client_set_header(c, "User-Agent", "git/draftling-1.0");
    if (r->auth_basic[0])
        esp_http_client_set_header(c, "Authorization", r->auth_basic);
    return c;
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

    esp_http_client_handle_t c = http_begin(r, url, HTTP_METHOD_GET);
    if (!c) return ESP_FAIL;

    esp_err_t err = esp_http_client_open(c, 0);
    if (err != ESP_OK) { esp_http_client_cleanup(c); return err; }
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

esp_err_t git_http_fetch_pack(const git_remote *r,
                              const git_oid *wants, int nwants,
                              const git_oid *haves, int nhaves,
                              const char *out_pack_path)
{
    if (nwants <= 0) return ESP_ERR_INVALID_ARG;

    git_buf req = GIT_BUF_INIT;
    for (int i = 0; i < nwants; i++) {
        char line[128];
        int l = (i == 0)
            ? snprintf(line, sizeof(line), "want %s ofs-delta %s\n", wants[i].hex, GIT_AGENT)
            : snprintf(line, sizeof(line), "want %s\n", wants[i].hex);
        pkt_write(&req, line, (size_t)l);
    }
    pkt_flush(&req);
    for (int i = 0; i < nhaves; i++) {
        char line[64];
        int l = snprintf(line, sizeof(line), "have %s\n", haves[i].hex);
        pkt_write(&req, line, (size_t)l);
    }
    pkt_write(&req, "done\n", 5);

    char url[380];
    snprintf(url, sizeof(url), "%s/git-upload-pack", r->base_url);
    esp_http_client_handle_t c = http_begin(r, url, HTTP_METHOD_POST);
    if (!c) { git_buf_free(&req); return ESP_FAIL; }
    esp_http_client_set_header(c, "Content-Type", "application/x-git-upload-pack-request");
    esp_http_client_set_header(c, "Accept", "application/x-git-upload-pack-result");

    esp_err_t ret = esp_http_client_open(c, (int)req.len);
    if (ret != ESP_OK) goto cleanup;
    if (esp_http_client_write(c, (char *)req.data, (int)req.len) != (int)req.len) {
        ret = ESP_FAIL;
        goto cleanup;
    }
    esp_http_client_fetch_headers(c);
    int status = esp_http_client_get_status_code(c);
    if (status != 200) {
        ESP_LOGE(TAG, "upload-pack HTTP %d", status);
        ret = ESP_FAIL;
        goto cleanup;
    }

    FILE *fp = fopen(out_pack_path, "wb");
    if (!fp) { ret = ESP_FAIL; goto cleanup; }

    /* Response = pkt-lines carrying ACK/NAK (and possibly ERR / shallow /
     * comment lines), then, right after a "NAK" / "ACK <id>" line or a
     * flush-pkt, the raw packfile.  We are not in side-band mode, so once
     * the pack starts every remaining byte belongs to it. */
    {
        uint8_t buf[2048];
        uint8_t lenbuf[4];
        int lenbuf_n = 0;         /* bytes of the 4-hex length gathered */
        int pkt_remaining = -1;   /* >=0: payload bytes still to skip */
        uint8_t head[8];          /* first payload bytes, for classifying */
        int head_n = 0;
        bool in_pack = false;
        bool got_pack = false;
        bool err = false;
        ret = ESP_FAIL;

        for (;;) {
            int rd = esp_http_client_read(c, (char *)buf, sizeof(buf));
            if (rd <= 0) break;
            const uint8_t *p = buf;
            size_t avail = (size_t)rd;

            while (!in_pack && avail > 0) {
                if (pkt_remaining < 0) {
                    /* accumulating the 4-byte length prefix */
                    lenbuf[lenbuf_n++] = *p++;
                    avail--;
                    if (lenbuf_n < 4) continue;
                    lenbuf_n = 0;
                    int len = hex4((char *)lenbuf);
                    if (len < 0) { in_pack = true; got_pack = false; break; }
                    if (len == 0) { in_pack = true; break; }        /* flush */
                    if (len <= 4) { pkt_remaining = 0; }            /* empty */
                    else { pkt_remaining = len - 4; head_n = 0; }
                    continue;
                }
                /* consuming this pkt-line's payload */
                size_t take = ((size_t)pkt_remaining < avail) ? (size_t)pkt_remaining : avail;
                for (size_t k = 0; k < take && head_n < (int)sizeof(head); k++)
                    head[head_n++] = p[k];
                p += take;
                avail -= take;
                pkt_remaining -= (int)take;
                if (pkt_remaining > 0) break;    /* line spans reads */
                pkt_remaining = -1;

                if (head_n >= 3 && memcmp(head, "NAK", 3) == 0) in_pack = true;
                else if (head_n >= 3 && memcmp(head, "ACK", 3) == 0) in_pack = true;
                else if (head_n >= 3 && memcmp(head, "ERR", 3) == 0) {
                    ESP_LOGE(TAG, "remote error during fetch");
                    err = true;
                    in_pack = true;
                }
            }

            if (err) break;
            if (in_pack && avail > 0) {
                fwrite(p, 1, avail, fp);
                got_pack = true;
            }
        }

        if (fclose(fp) != 0) got_pack = false;
        ret = (got_pack && !err) ? ESP_OK : ESP_FAIL;
        if (ret != ESP_OK) remove(out_pack_path);
    }

cleanup:
    esp_http_client_close(c);
    esp_http_client_cleanup(c);
    git_buf_free(&req);
    return ret;
}

/* ------------------------------------------------------------------ */
/* git-receive-pack (push)                                            */
/* ------------------------------------------------------------------ */

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
    esp_http_client_handle_t c = http_begin(r, url, HTTP_METHOD_POST);
    if (!c) { fclose(pf); git_buf_free(&cmds); return ESP_FAIL; }
    esp_http_client_set_header(c, "Content-Type", "application/x-git-receive-pack-request");
    esp_http_client_set_header(c, "Accept", "application/x-git-receive-pack-result");

    int total = (int)cmds.len + (int)pack_size;
    esp_err_t ret = esp_http_client_open(c, total);
    if (ret != ESP_OK) goto cleanup;

    if (esp_http_client_write(c, (char *)cmds.data, (int)cmds.len) != (int)cmds.len) {
        ret = ESP_FAIL;
        goto cleanup;
    }
    {
        char chunk[2048];
        size_t rd;
        while ((rd = fread(chunk, 1, sizeof(chunk), pf)) > 0) {
            if (esp_http_client_write(c, chunk, (int)rd) != (int)rd) {
                ret = ESP_FAIL;
                goto cleanup;
            }
        }
    }

    esp_http_client_fetch_headers(c);
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
