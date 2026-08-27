/* git_util.c -- oids, growable buffers, SHA-1 and zlib (ROM miniz). */
#include "git_core.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <esp_log.h>
#include <esp_heap_caps.h>
#include <esp_idf_version.h>

#if ESP_IDF_VERSION_MAJOR >= 6
#include <psa/crypto.h>
#else
#include <mbedtls/sha1.h>
#endif

#include "miniz.h"

static const char *TAG = "git.util";

/* ------------------------------------------------------------------ */
/* oids                                                               */
/* ------------------------------------------------------------------ */

void git_oid_clear(git_oid *o) { memset(o->hex, 0, sizeof(o->hex)); }

bool git_oid_is_zero(const git_oid *o)
{
    if (o->hex[0] == '\0') return true;
    for (int i = 0; i < GIT_OID_HEXSZ; i++)
        if (o->hex[i] != '0') return false;
    return true;
}

bool git_oid_eq(const git_oid *a, const git_oid *b)
{
    return strncmp(a->hex, b->hex, GIT_OID_HEXSZ) == 0;
}

static int hexval(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

void git_oid_from_raw(git_oid *o, const uint8_t raw[GIT_OID_RAWSZ])
{
    static const char *hx = "0123456789abcdef";
    for (int i = 0; i < GIT_OID_RAWSZ; i++) {
        o->hex[i * 2]     = hx[raw[i] >> 4];
        o->hex[i * 2 + 1] = hx[raw[i] & 0xf];
    }
    o->hex[GIT_OID_HEXSZ] = '\0';
}

bool git_oid_to_raw(uint8_t raw[GIT_OID_RAWSZ], const git_oid *o)
{
    for (int i = 0; i < GIT_OID_RAWSZ; i++) {
        int hi = hexval(o->hex[i * 2]);
        int lo = hexval(o->hex[i * 2 + 1]);
        if (hi < 0 || lo < 0) return false;
        raw[i] = (uint8_t)((hi << 4) | lo);
    }
    return true;
}

/* ------------------------------------------------------------------ */
/* growable buffer                                                    */
/* ------------------------------------------------------------------ */

void git_buf_free(git_buf *b)
{
    if (b->data) heap_caps_free(b->data);
    b->data = NULL;
    b->len = b->cap = 0;
}

bool git_buf_reserve(git_buf *b, size_t need)
{
    if (need <= b->cap) return true;
    size_t nc = b->cap ? b->cap : 256;
    while (nc < need) nc += nc / 2 + 64;
    uint8_t *nd = heap_caps_realloc(b->data, nc, MALLOC_CAP_SPIRAM);
    if (!nd) nd = heap_caps_realloc(b->data, nc, MALLOC_CAP_DEFAULT);
    if (!nd) return false;
    b->data = nd;
    b->cap = nc;
    return true;
}

bool git_buf_put(git_buf *b, const void *p, size_t n)
{
    if (!git_buf_reserve(b, b->len + n + 1)) return false;
    if (n) memcpy(b->data + b->len, p, n);
    b->len += n;
    b->data[b->len] = 0;
    return true;
}

bool git_buf_putc(git_buf *b, uint8_t c) { return git_buf_put(b, &c, 1); }
bool git_buf_puts(git_buf *b, const char *s) { return git_buf_put(b, s, strlen(s)); }

bool git_buf_printf(git_buf *b, const char *fmt, ...)
{
    char stackbuf[256];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(stackbuf, sizeof(stackbuf), fmt, ap);
    va_end(ap);
    if (n < 0) return false;
    if ((size_t)n < sizeof(stackbuf)) return git_buf_put(b, stackbuf, (size_t)n);

    char *big = malloc((size_t)n + 1);
    if (!big) return false;
    va_start(ap, fmt);
    vsnprintf(big, (size_t)n + 1, fmt, ap);
    va_end(ap);
    bool ok = git_buf_put(b, big, (size_t)n);
    free(big);
    return ok;
}

/* ------------------------------------------------------------------ */
/* SHA-1                                                              */
/* ------------------------------------------------------------------ */

struct git_sha1_ctx {
#if ESP_IDF_VERSION_MAJOR >= 6
    psa_hash_operation_t op;
#else
    mbedtls_sha1_context ctx;
#endif
};

git_sha1_ctx *git_sha1_new(void)
{
    git_sha1_ctx *c = calloc(1, sizeof(*c));
    if (!c) return NULL;
#if ESP_IDF_VERSION_MAJOR >= 6
    c->op = psa_hash_operation_init();
    if (psa_hash_setup(&c->op, PSA_ALG_SHA_1) != PSA_SUCCESS) {
        free(c);
        return NULL;
    }
#else
    mbedtls_sha1_init(&c->ctx);
    mbedtls_sha1_starts(&c->ctx);
#endif
    return c;
}

void git_sha1_update(git_sha1_ctx *c, const void *data, size_t n)
{
    if (!c || !n) return;
#if ESP_IDF_VERSION_MAJOR >= 6
    psa_hash_update(&c->op, data, n);
#else
    mbedtls_sha1_update(&c->ctx, data, n);
#endif
}

void git_sha1_final(git_sha1_ctx *c, uint8_t out[GIT_OID_RAWSZ])
{
    if (!c) { memset(out, 0, GIT_OID_RAWSZ); return; }
#if ESP_IDF_VERSION_MAJOR >= 6
    size_t olen = 0;
    psa_hash_finish(&c->op, out, GIT_OID_RAWSZ, &olen);
#else
    mbedtls_sha1_finish(&c->ctx, out);
    mbedtls_sha1_free(&c->ctx);
#endif
    free(c);
}

void git_sha1_hex(const void *data, size_t n, git_oid *out)
{
    git_sha1_ctx *c = git_sha1_new();
    uint8_t raw[GIT_OID_RAWSZ];
    git_sha1_update(c, data, n);
    git_sha1_final(c, raw);
    git_oid_from_raw(out, raw);
}

/* ------------------------------------------------------------------ */
/* zlib via ROM miniz                                                 */
/* ------------------------------------------------------------------ */

esp_err_t git_zlib_inflate(const uint8_t *in, size_t in_avail,
                           uint8_t *out, size_t out_expected,
                           size_t *in_consumed)
{
    tinfl_decompressor *d = heap_caps_malloc(sizeof(*d), MALLOC_CAP_SPIRAM);
    if (!d) d = malloc(sizeof(*d));
    if (!d) return ESP_ERR_NO_MEM;
    tinfl_init(d);

    size_t in_bytes = in_avail;
    size_t out_bytes = out_expected;
    tinfl_status st = tinfl_decompress(d, in, &in_bytes,
                                       out, out, &out_bytes,
                                       TINFL_FLAG_PARSE_ZLIB_HEADER |
                                       TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
    free(d);

    if (st != TINFL_STATUS_DONE || out_bytes != out_expected) {
        ESP_LOGE(TAG, "inflate failed st=%d out=%u/%u", (int)st,
                 (unsigned)out_bytes, (unsigned)out_expected);
        return ESP_FAIL;
    }
    if (in_consumed) *in_consumed = in_bytes;
    return ESP_OK;
}

esp_err_t git_zlib_inflate_buf(const uint8_t *in, size_t in_avail, git_buf *out)
{
    tinfl_decompressor *d = heap_caps_malloc(sizeof(*d), MALLOC_CAP_SPIRAM);
    if (!d) d = malloc(sizeof(*d));
    if (!d) return ESP_ERR_NO_MEM;
    tinfl_init(d);

    /* miniz needs a 32 KiB wrapping dictionary window as the output. */
    uint8_t *win = heap_caps_malloc(TINFL_LZ_DICT_SIZE, MALLOC_CAP_SPIRAM);
    if (!win) { free(d); return ESP_ERR_NO_MEM; }

    esp_err_t ret = ESP_OK;
    size_t in_pos = 0, out_pos = 0;
    for (;;) {
        size_t in_bytes = in_avail - in_pos;
        size_t out_bytes = TINFL_LZ_DICT_SIZE - out_pos;
        tinfl_status st = tinfl_decompress(d, in + in_pos, &in_bytes,
                                           win, win + out_pos, &out_bytes,
                                           TINFL_FLAG_PARSE_ZLIB_HEADER);
        in_pos += in_bytes;
        if (out_bytes && !git_buf_put(out, win + out_pos, out_bytes)) {
            ret = ESP_ERR_NO_MEM;
            break;
        }
        out_pos = (out_pos + out_bytes) & (TINFL_LZ_DICT_SIZE - 1);
        if (st == TINFL_STATUS_DONE) break;
        if (st < TINFL_STATUS_DONE) { ret = ESP_FAIL; break; }
        if (st == TINFL_STATUS_NEEDS_MORE_INPUT && in_pos >= in_avail) {
            ret = ESP_FAIL;
            break;
        }
    }

    free(win);
    free(d);
    return ret;
}

esp_err_t git_zlib_deflate(const uint8_t *in, size_t in_len, git_buf *out)
{
    tdefl_compressor *c = heap_caps_malloc(sizeof(*c), MALLOC_CAP_SPIRAM);
    if (!c) c = malloc(sizeof(*c));
    if (!c) return ESP_ERR_NO_MEM;

    /* 1150 probes ~= default compression, TDEFL_WRITE_ZLIB_HEADER. */
    if (tdefl_init(c, NULL, NULL, TDEFL_WRITE_ZLIB_HEADER | 128) != TDEFL_STATUS_OKAY) {
        free(c);
        return ESP_FAIL;
    }

    uint8_t chunk[2048];
    size_t in_pos = 0;
    esp_err_t ret = ESP_OK;
    for (;;) {
        size_t in_bytes = in_len - in_pos;
        size_t out_bytes = sizeof(chunk);
        tdefl_status st = tdefl_compress(c, in + in_pos, &in_bytes,
                                         chunk, &out_bytes, TDEFL_FINISH);
        in_pos += in_bytes;
        if (out_bytes && !git_buf_put(out, chunk, out_bytes)) {
            ret = ESP_ERR_NO_MEM;
            break;
        }
        if (st == TDEFL_STATUS_DONE) break;
        if (st != TDEFL_STATUS_OKAY) { ret = ESP_FAIL; break; }
    }

    free(c);
    return ret;
}

/* ------------------------------------------------------------------ */
/* object type names                                                  */
/* ------------------------------------------------------------------ */

const char *git_obj_type_name(git_obj_type t)
{
    switch (t) {
    case GIT_OBJ_COMMIT: return "commit";
    case GIT_OBJ_TREE:   return "tree";
    case GIT_OBJ_BLOB:   return "blob";
    case GIT_OBJ_TAG:    return "tag";
    default:             return "bad";
    }
}

git_obj_type git_obj_type_from_name(const char *s)
{
    if (!strcmp(s, "commit")) return GIT_OBJ_COMMIT;
    if (!strcmp(s, "tree"))   return GIT_OBJ_TREE;
    if (!strcmp(s, "blob"))   return GIT_OBJ_BLOB;
    if (!strcmp(s, "tag"))    return GIT_OBJ_TAG;
    return GIT_OBJ_BAD;
}
