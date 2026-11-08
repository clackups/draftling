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
    bool err;           /* an update failed: the digest is wrong */
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
    if (psa_hash_update(&c->op, data, n) != PSA_SUCCESS) c->err = true;
#else
    if (mbedtls_sha1_update(&c->ctx, data, n) != 0) c->err = true;
#endif
}

bool git_sha1_final(git_sha1_ctx *c, uint8_t out[GIT_OID_RAWSZ])
{
    if (!c) { memset(out, 0, GIT_OID_RAWSZ); return false; }
    bool ok = !c->err;
#if ESP_IDF_VERSION_MAJOR >= 6
    size_t olen = 0;
    if (psa_hash_finish(&c->op, out, GIT_OID_RAWSZ, &olen) != PSA_SUCCESS ||
        olen != GIT_OID_RAWSZ) {
        psa_hash_abort(&c->op);
        ok = false;
    }
#else
    if (mbedtls_sha1_finish(&c->ctx, out) != 0) ok = false;
    mbedtls_sha1_free(&c->ctx);
#endif
    free(c);
    if (!ok) {
        ESP_LOGE("git.sha1", "SHA-1 computation failed");
        memset(out, 0, GIT_OID_RAWSZ);
    }
    return ok;
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
/* streaming zlib via ROM miniz                                       */
/* ------------------------------------------------------------------ */

/* Everything is streamed through fixed-size buffers, so the memory a
 * Git operation needs does not depend on the size of the objects. */

#define ZR_INBUF 4096

struct git_zreader {
    tinfl_decompressor d;
    uint8_t  win[TINFL_LZ_DICT_SIZE];   /* wrapping output window */
    uint8_t  in[ZR_INBUF];
    FILE    *fp;
    long     start;                     /* file offset of the stream */
    size_t   in_pos, in_have;
    size_t   consumed;                  /* compressed bytes used */
    size_t   win_pos;                   /* next output position */
    size_t   pend_off, pend_len;        /* output not delivered yet */
    bool     eof, done, error;
};

git_zreader *git_zreader_new(void)
{
    git_zreader *z = heap_caps_malloc(sizeof(*z), MALLOC_CAP_SPIRAM);
    if (!z) z = malloc(sizeof(*z));
    return z;
}

void git_zreader_free(git_zreader *z)
{
    free(z);
}

void git_zreader_start(git_zreader *z, FILE *fp)
{
    tinfl_init(&z->d);
    z->fp = fp;
    z->start = ftell(fp);
    z->in_pos = z->in_have = 0;
    z->consumed = 0;
    z->win_pos = 0;
    z->pend_off = z->pend_len = 0;
    z->eof = z->done = z->error = false;
}

long git_zreader_read(git_zreader *z, void *buf, size_t n)
{
    uint8_t *out = (uint8_t *)buf;
    size_t total = 0;
    while (total < n) {
        if (z->pend_len) {
            size_t c = z->pend_len < n - total ? z->pend_len : n - total;
            memcpy(out + total, z->win + z->pend_off, c);
            z->pend_off += c;
            z->pend_len -= c;
            total += c;
            continue;
        }
        if (z->done) break;
        if (z->error) return -1;
        if (z->in_pos == z->in_have && !z->eof) {
            z->in_have = fread(z->in, 1, sizeof(z->in), z->fp);
            z->in_pos = 0;
            if (z->in_have == 0) z->eof = true;
        }
        size_t in_bytes = z->in_have - z->in_pos;
        size_t out_bytes = TINFL_LZ_DICT_SIZE - z->win_pos;
        int flags = TINFL_FLAG_PARSE_ZLIB_HEADER |
                    (z->eof ? 0 : TINFL_FLAG_HAS_MORE_INPUT);
        tinfl_status st = tinfl_decompress(&z->d, z->in + z->in_pos, &in_bytes,
                                           z->win, z->win + z->win_pos,
                                           &out_bytes, flags);
        z->in_pos += in_bytes;
        z->consumed += in_bytes;
        z->pend_off = z->win_pos;
        z->pend_len = out_bytes;
        z->win_pos = (z->win_pos + out_bytes) & (TINFL_LZ_DICT_SIZE - 1);
        if (st == TINFL_STATUS_DONE) z->done = true;
        else if (st < TINFL_STATUS_DONE) z->error = true;
        else if (st == TINFL_STATUS_NEEDS_MORE_INPUT && z->eof) z->error = true;
    }
    return (long)total;
}

bool git_zreader_finish(git_zreader *z)
{
    uint8_t extra;
    if (git_zreader_read(z, &extra, 1) != 0) return false;   /* trailing data */
    if (!z->done) return false;
    /* tinfl's bit buffer may have read whole bytes past the end of the
     * stream (newer miniz versions hand them back themselves, leaving
     * m_num_bits below 8); the stream ends byte-aligned after the
     * Adler-32, so those bytes belong to the next object. */
    size_t extra_bytes = z->d.m_num_bits >> 3;
    if (extra_bytes > z->consumed) return false;
    fseek(z->fp, z->start + (long)(z->consumed - extra_bytes), SEEK_SET);
    return true;
}

#define ZW_OUTBUF 4096

struct git_zwriter {
    tdefl_compressor c;
    uint8_t  out[ZW_OUTBUF];
    FILE    *fp;
    git_sha1_ctx *sha;                  /* hashes the compressed bytes */
    bool     error;
};

git_zwriter *git_zwriter_new(void)
{
    git_zwriter *w = heap_caps_malloc(sizeof(*w), MALLOC_CAP_SPIRAM);
    if (!w) w = malloc(sizeof(*w));
    return w;
}

void git_zwriter_free(git_zwriter *w)
{
    free(w);
}

esp_err_t git_zwriter_start(git_zwriter *w, FILE *fp, git_sha1_ctx *sha)
{
    w->fp = fp;
    w->sha = sha;
    w->error = false;
    /* 128 probes, zlib header. */
    if (tdefl_init(&w->c, NULL, NULL, TDEFL_WRITE_ZLIB_HEADER | 128) != TDEFL_STATUS_OKAY)
        return ESP_FAIL;
    return ESP_OK;
}

static esp_err_t zw_run(git_zwriter *w, const uint8_t *p, size_t n, tdefl_flush flush)
{
    if (w->error) return ESP_FAIL;
    for (;;) {
        size_t in_bytes = n;
        size_t out_bytes = sizeof(w->out);
        tdefl_status st = tdefl_compress(&w->c, p, &in_bytes, w->out, &out_bytes, flush);
        p += in_bytes;
        n -= in_bytes;
        if (out_bytes) {
            if (fwrite(w->out, 1, out_bytes, w->fp) != out_bytes) { w->error = true; return ESP_FAIL; }
            if (w->sha) git_sha1_update(w->sha, w->out, out_bytes);
        }
        if (st == TDEFL_STATUS_DONE) return ESP_OK;
        if (st != TDEFL_STATUS_OKAY) { w->error = true; return ESP_FAIL; }
        if (flush == TDEFL_NO_FLUSH && n == 0 && out_bytes < sizeof(w->out)) return ESP_OK;
    }
}

esp_err_t git_zwriter_write(git_zwriter *w, const void *p, size_t n)
{
    if (!n) return w->error ? ESP_FAIL : ESP_OK;
    return zw_run(w, (const uint8_t *)p, n, TDEFL_NO_FLUSH);
}

esp_err_t git_zwriter_finish(git_zwriter *w)
{
    return zw_run(w, NULL, 0, TDEFL_FINISH);
}

/* ------------------------------------------------------------------ */
/* byte sources                                                       */
/* ------------------------------------------------------------------ */

static long mem_read(void *ctx, void *buf, size_t n)
{
    git_mem_src *m = (git_mem_src *)ctx;
    size_t left = m->len - m->pos;
    if (n > left) n = left;
    memcpy(buf, m->p + m->pos, n);
    m->pos += n;
    return (long)n;
}

static long file_read(void *ctx, void *buf, size_t n)
{
    size_t r = fread(buf, 1, n, (FILE *)ctx);
    if (r == 0 && ferror((FILE *)ctx)) return -1;
    return (long)r;
}

void git_reader_mem(git_reader *r, git_mem_src *m, const void *p, size_t len)
{
    m->p = (const uint8_t *)p;
    m->len = len;
    m->pos = 0;
    r->read = mem_read;
    r->ctx = m;
}

void git_reader_file(git_reader *r, FILE *fp)
{
    r->read = file_read;
    r->ctx = fp;
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
