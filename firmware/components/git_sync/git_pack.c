/* git_pack.c -- packfile v2 reader (with OFS_DELTA / REF_DELTA) and a
 * non-delta packfile writer. */
#include "git_core.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <esp_log.h>
#include <esp_heap_caps.h>


static const char *TAG = "git.pack";

/* A delta's base object is held in RAM up to this size, else in a
 * scratch file on the SD card (copy instructions seek into it). */
#define DELTA_BASE_RAM_MAX (256 * 1024)

#define IO_CHUNK 4096

/* ------------------------------------------------------------------ */
/* offset -> oid map (OFS_DELTA base lookup)                          */
/* ------------------------------------------------------------------ */

typedef struct { long off; git_oid oid; git_obj_type type; } off_ent;
typedef struct { off_ent *v; int n, cap; } off_map;

static bool off_map_add(off_map *m, long off, const git_oid *oid, git_obj_type t)
{
    if (m->n >= m->cap) {
        int nc = m->cap ? m->cap * 2 : 128;
        off_ent *nv = heap_caps_realloc(m->v, (size_t)nc * sizeof(*nv), MALLOC_CAP_SPIRAM);
        if (!nv) return false;
        m->v = nv;
        m->cap = nc;
    }
    m->v[m->n].off = off;
    m->v[m->n].oid = *oid;
    m->v[m->n].type = t;
    m->n++;
    return true;
}

static const off_ent *off_map_get(const off_map *m, long off)
{
    for (int i = 0; i < m->n; i++)
        if (m->v[i].off == off) return &m->v[i];
    return NULL;
}

/* ------------------------------------------------------------------ */
/* low-level file reads                                               */
/* ------------------------------------------------------------------ */

static int rd_u8(FILE *fp) { int c = fgetc(fp); return c; }

static esp_err_t rd_exact(FILE *fp, void *buf, size_t n)
{
    return fread(buf, 1, n, fp) == n ? ESP_OK : ESP_FAIL;
}

static uint32_t rd_be32(FILE *fp)
{
    uint8_t b[4];
    if (rd_exact(fp, b, 4) != ESP_OK) return 0;
    return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) |
           ((uint32_t)b[2] << 8) | b[3];
}

/* ------------------------------------------------------------------ */
/* buffered reads from a zlib stream (delta instructions)             */
/* ------------------------------------------------------------------ */

typedef struct {
    git_zreader *z;
    uint8_t buf[1024];
    size_t  pos, have;
} zbuf;

static int zb_getc(zbuf *b)
{
    if (b->pos == b->have) {
        long r = git_zreader_read(b->z, b->buf, sizeof(b->buf));
        if (r <= 0) return -1;
        b->have = (size_t)r;
        b->pos = 0;
    }
    return b->buf[b->pos++];
}

/* Copy n bytes of the stream to the object writer. */
static esp_err_t zb_copy(zbuf *b, size_t n, git_odb_writer *w)
{
    while (n) {
        if (b->pos == b->have) {
            long r = git_zreader_read(b->z, b->buf, sizeof(b->buf));
            if (r <= 0) return ESP_FAIL;
            b->have = (size_t)r;
            b->pos = 0;
        }
        size_t c = b->have - b->pos;
        if (c > n) c = n;
        esp_err_t ret = git_odb_writer_write(w, b->buf + b->pos, c);
        if (ret != ESP_OK) return ret;
        b->pos += c;
        n -= c;
    }
    return ESP_OK;
}

/* The delta stream is fully consumed: no buffered bytes left over and
 * the zlib stream ends. */
static bool zb_finish(zbuf *b)
{
    return b->pos == b->have && git_zreader_finish(b->z);
}

static bool zb_varint(zbuf *b, uint64_t *out)
{
    uint64_t v = 0;
    int shift = 0;
    for (;;) {
        int c = zb_getc(b);
        if (c < 0 || shift > 56) return false;
        v |= (uint64_t)(c & 0x7f) << shift;
        shift += 7;
        if (!(c & 0x80)) break;
    }
    *out = v;
    return true;
}

/* ------------------------------------------------------------------ */
/* delta base: in RAM when small, else a scratch file                 */
/* ------------------------------------------------------------------ */

typedef struct {
    git_oid      oid;           /* zero = nothing loaded */
    git_obj_type type;
    size_t       size;
    git_buf      mem;           /* small bases */
    FILE        *fp;            /* large bases */
    char         path[320];
} delta_base;

static void base_reset(delta_base *b)
{
    git_buf_free(&b->mem);
    if (b->fp) { fclose(b->fp); b->fp = NULL; remove(b->path); }
    git_oid_clear(&b->oid);
}

static esp_err_t base_load(delta_base *b, const git_oid *oid)
{
    if (!git_oid_is_zero(&b->oid) && git_oid_eq(&b->oid, oid)) return ESP_OK;
    base_reset(b);

    git_odb_stream s;
    esp_err_t ret = git_odb_stream_open(&s, oid);
    if (ret != ESP_OK) return ret;
    b->type = s.type;
    b->size = s.size;
    if (s.size <= DELTA_BASE_RAM_MAX) {
        if (!git_buf_reserve(&b->mem, s.size + 1)) ret = ESP_ERR_NO_MEM;
        else if (s.size && git_odb_stream_read(&s, b->mem.data, s.size) != (long)s.size) ret = ESP_FAIL;
        else b->mem.len = s.size;
    } else {
        git_odb_tmp_path(b->path, sizeof(b->path), "delta-base");
        b->fp = fopen(b->path, "w+b");
        uint8_t *buf = malloc(IO_CHUNK);
        if (!b->fp || !buf) ret = ESP_FAIL;
        while (ret == ESP_OK && s.left) {
            long r = git_odb_stream_read(&s, buf, IO_CHUNK);
            if (r <= 0 || fwrite(buf, 1, (size_t)r, b->fp) != (size_t)r) ret = ESP_FAIL;
        }
        free(buf);
    }
    git_odb_stream_close(&s);
    if (ret != ESP_OK) { base_reset(b); return ret; }
    b->oid = *oid;
    return ESP_OK;
}

static esp_err_t base_copy(delta_base *b, size_t off, size_t n, git_odb_writer *w,
                           uint8_t *scratch)
{
    if (!b->fp) return git_odb_writer_write(w, b->mem.data + off, n);
    if (fseek(b->fp, (long)off, SEEK_SET) != 0) return ESP_FAIL;
    while (n) {
        size_t c = n < IO_CHUNK ? n : IO_CHUNK;
        if (fread(scratch, 1, c, b->fp) != c) return ESP_FAIL;
        esp_err_t ret = git_odb_writer_write(w, scratch, c);
        if (ret != ESP_OK) return ret;
        n -= c;
    }
    return ESP_OK;
}

/* Apply the delta read from zb to base, storing the result. */
static esp_err_t apply_delta(delta_base *base, zbuf *zb, uint8_t *scratch, git_oid *out)
{
    uint64_t src, dst;
    if (!zb_varint(zb, &src) || !zb_varint(zb, &dst)) return ESP_FAIL;
    if (src != base->size) return ESP_FAIL;

    git_odb_writer w;
    esp_err_t ret = git_odb_writer_open(&w, base->type, (size_t)dst);
    if (ret != ESP_OK) return ret;

    for (;;) {
        int op = zb_getc(zb);
        if (op < 0) break;                     /* end of the delta */
        if (op & 0x80) {
            uint32_t cp_off = 0, cp_size = 0;
            for (int i = 0; i < 4; i++) {
                if (op & (1 << i)) {
                    int c = zb_getc(zb);
                    if (c < 0) { ret = ESP_FAIL; break; }
                    cp_off |= (uint32_t)c << (8 * i);
                }
            }
            for (int i = 0; ret == ESP_OK && i < 3; i++) {
                if (op & (0x10 << i)) {
                    int c = zb_getc(zb);
                    if (c < 0) { ret = ESP_FAIL; break; }
                    cp_size |= (uint32_t)c << (8 * i);
                }
            }
            if (ret != ESP_OK) break;
            if (cp_size == 0) cp_size = 0x10000;
            if ((uint64_t)cp_off + cp_size > base->size) { ret = ESP_FAIL; break; }
            ret = base_copy(base, cp_off, cp_size, &w, scratch);
        } else if (op) {
            ret = zb_copy(zb, (size_t)op, &w);
        } else {
            ret = ESP_FAIL;                    /* op == 0 is reserved */
        }
        if (ret != ESP_OK) break;
    }
    if (ret == ESP_OK && !zb_finish(zb)) ret = ESP_FAIL;
    if (ret != ESP_OK) { git_odb_writer_abort(&w); return ret; }
    return git_odb_writer_finish(&w, out);     /* fails if the size is off */
}

/* ------------------------------------------------------------------ */
/* unpack                                                             */
/* ------------------------------------------------------------------ */

esp_err_t git_pack_unpack_file(FILE *fp)
{
    uint8_t magic[4];
    if (rd_exact(fp, magic, 4) != ESP_OK || memcmp(magic, "PACK", 4) != 0) {
        ESP_LOGE(TAG, "not a packfile");
        return ESP_FAIL;
    }
    uint32_t version = rd_be32(fp);
    uint32_t count = rd_be32(fp);
    if (version != 2 && version != 3) {
        ESP_LOGE(TAG, "unsupported pack version %u", version);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "unpacking %u objects", count);

    off_map omap = {0};
    delta_base base = {0};
    zbuf *zb = calloc(1, sizeof(*zb));
    uint8_t *scratch = malloc(IO_CHUNK);
    git_zreader *z = git_zreader_new();
    esp_err_t ret = (zb && scratch && z) ? ESP_OK : ESP_ERR_NO_MEM;
    if (zb) zb->z = z;

    for (uint32_t i = 0; i < count && ret == ESP_OK; i++) {
        long obj_off = ftell(fp);

        int c = rd_u8(fp);
        if (c < 0) { ret = ESP_FAIL; break; }
        git_obj_type type = (git_obj_type)((c >> 4) & 7);
        uint64_t size = (uint64_t)(c & 0x0f);
        int shift = 4;
        while (c & 0x80) {
            c = rd_u8(fp);
            /* past 60 bits the shift would overflow (undefined) */
            if (c < 0 || shift > 60) { ret = ESP_FAIL; break; }
            size |= (uint64_t)(c & 0x7f) << shift;
            shift += 7;
        }
        if (ret != ESP_OK) break;
        if (size > SIZE_MAX) { ret = ESP_FAIL; break; }

        git_oid oid;
        if (type == GIT_OBJ_OFS_DELTA || type == GIT_OBJ_REF_DELTA) {
            git_oid base_oid;
            if (type == GIT_OBJ_OFS_DELTA) {
                int b = rd_u8(fp);
                if (b < 0) { ret = ESP_FAIL; break; }
                uint64_t off = (uint64_t)(b & 0x7f);
                while (b & 0x80) {
                    b = rd_u8(fp);
                    if (b < 0 || off > ((uint64_t)LONG_MAX >> 7)) { ret = ESP_FAIL; break; }
                    off = ((off + 1) << 7) | (uint64_t)(b & 0x7f);
                }
                if (ret != ESP_OK) break;
                const off_ent *be = off_map_get(&omap, obj_off - (long)off);
                if (!be) {
                    ESP_LOGE(TAG, "ofs-delta base %ld missing", obj_off - (long)off);
                    ret = ESP_FAIL;
                    break;
                }
                base_oid = be->oid;
            } else {
                uint8_t raw[GIT_OID_RAWSZ];
                if (rd_exact(fp, raw, GIT_OID_RAWSZ) != ESP_OK) { ret = ESP_FAIL; break; }
                git_oid_from_raw(&base_oid, raw);
            }
            ret = base_load(&base, &base_oid);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "delta base %s %s", base_oid.hex,
                         ret == ESP_ERR_NOT_FOUND ? "missing" : "cannot be loaded");
                break;
            }
            git_zreader_start(z, fp);
            zb->pos = zb->have = 0;
            ret = apply_delta(&base, zb, scratch, &oid);
            if (ret == ESP_OK && !off_map_add(&omap, obj_off, &oid, base.type))
                ret = ESP_ERR_NO_MEM;
        } else if (type == GIT_OBJ_COMMIT || type == GIT_OBJ_TREE ||
                   type == GIT_OBJ_BLOB || type == GIT_OBJ_TAG) {
            git_odb_writer w;
            ret = git_odb_writer_open(&w, type, (size_t)size);
            if (ret != ESP_OK) break;
            git_zreader_start(z, fp);
            uint64_t left = size;
            while (ret == ESP_OK && left) {
                size_t want = left < IO_CHUNK ? (size_t)left : IO_CHUNK;
                if (git_zreader_read(z, scratch, want) != (long)want) ret = ESP_FAIL;
                else ret = git_odb_writer_write(&w, scratch, want);
                left -= want;
            }
            if (ret == ESP_OK && !git_zreader_finish(z)) ret = ESP_FAIL;
            if (ret == ESP_OK) ret = git_odb_writer_finish(&w, &oid);
            else git_odb_writer_abort(&w);
            if (ret == ESP_OK && !off_map_add(&omap, obj_off, &oid, type))
                ret = ESP_ERR_NO_MEM;
        } else {
            ESP_LOGE(TAG, "unknown pack object type %d", type);
            ret = ESP_FAIL;
        }
    }

    base_reset(&base);
    git_zreader_free(z);
    free(scratch);
    free(zb);
    if (omap.v) heap_caps_free(omap.v);
    return ret;
}

/* ------------------------------------------------------------------ */
/* write                                                              */
/* ------------------------------------------------------------------ */

static void put_bytes(FILE *f, git_sha1_ctx *sha, const void *p, size_t n)
{
    fwrite(p, 1, n, f);
    git_sha1_update(sha, p, n);
}

static void put_be32(FILE *f, git_sha1_ctx *sha, uint32_t v)
{
    uint8_t b[4] = { v >> 24, v >> 16, v >> 8, v };
    put_bytes(f, sha, b, 4);
}

esp_err_t git_pack_write_file(const char *path, const git_oidset *set)
{
    FILE *f = fopen(path, "wb");
    if (!f) return ESP_FAIL;

    git_sha1_ctx *sha = git_sha1_new();
    git_zwriter *zw = git_zwriter_new();
    uint8_t *buf = malloc(IO_CHUNK);
    esp_err_t ret = (sha && zw && buf) ? ESP_OK : ESP_ERR_NO_MEM;

    if (ret == ESP_OK) {
        put_bytes(f, sha, "PACK", 4);
        put_be32(f, sha, 2);
        put_be32(f, sha, (uint32_t)set->n);
    }

    for (int i = 0; i < set->n && ret == ESP_OK; i++) {
        git_odb_stream s;
        ret = git_odb_stream_open(&s, &set->v[i]);
        if (ret != ESP_OK) break;

        /* variable-length object header */
        uint8_t hdr[16];
        int hn = 0;
        uint64_t sz = s.size;
        hdr[hn++] = (uint8_t)((s.type << 4) | (sz & 0x0f)) | (sz >> 4 ? 0x80 : 0);
        sz >>= 4;
        while (sz) {
            hdr[hn++] = (uint8_t)(sz & 0x7f) | (sz >> 7 ? 0x80 : 0);
            sz >>= 7;
        }
        put_bytes(f, sha, hdr, hn);

        ret = git_zwriter_start(zw, f, sha);
        while (ret == ESP_OK && s.left) {
            long r = git_odb_stream_read(&s, buf, IO_CHUNK);
            ret = (r > 0) ? git_zwriter_write(zw, buf, (size_t)r) : ESP_FAIL;
        }
        if (ret == ESP_OK) ret = git_zwriter_finish(zw);
        git_odb_stream_close(&s);
    }

    uint8_t trailer[GIT_OID_RAWSZ];
    git_sha1_final(sha, trailer);
    if (ret == ESP_OK && fwrite(trailer, 1, sizeof(trailer), f) != sizeof(trailer))
        ret = ESP_FAIL;
    /* put_bytes() does not check its writes: a short one leaves the
     * stream's error flag set. */
    if (ret == ESP_OK && ferror(f)) ret = ESP_FAIL;

    git_zwriter_free(zw);
    free(buf);
    if (fclose(f) != 0) ret = ESP_FAIL;
    if (ret != ESP_OK) remove(path);
    return ret;
}
