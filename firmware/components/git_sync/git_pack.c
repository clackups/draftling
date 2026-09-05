/* git_pack.c -- packfile v2 reader (with OFS_DELTA / REF_DELTA) and a
 * non-delta packfile writer. */
#include "git_core.h"

#include <stdlib.h>
#include <string.h>

#include <esp_log.h>
#include <esp_heap_caps.h>

#include "miniz.h"

static const char *TAG = "git.pack";

/* Largest single object we will materialise in RAM while unpacking. */
#define GIT_PACK_MAX_OBJ (3 * 1024 * 1024)

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

/* Inflate one zlib stream from the current file position, expecting
 * exactly `out_expected` bytes.  Leaves the file positioned right after
 * the compressed data. */
static esp_err_t inflate_stream(FILE *fp, long file_end,
                                uint8_t *out, size_t out_expected)
{
    tinfl_decompressor *d = heap_caps_malloc(sizeof(*d), MALLOC_CAP_SPIRAM);
    if (!d) d = malloc(sizeof(*d));
    if (!d) return ESP_ERR_NO_MEM;
    tinfl_init(d);

    uint8_t inbuf[2048];
    size_t in_have = 0, in_pos = 0, out_pos = 0, total_consumed = 0;
    long start = ftell(fp);
    esp_err_t ret = ESP_FAIL;

    for (;;) {
        if (in_pos == in_have) {
            in_have = fread(inbuf, 1, sizeof(inbuf), fp);
            in_pos = 0;
            if (in_have == 0) { ret = ESP_FAIL; break; }
        }
        size_t in_bytes = in_have - in_pos;
        size_t out_bytes = out_expected - out_pos;
        long consumed_pos = start + (long)total_consumed + (long)in_bytes;
        int more = (consumed_pos < file_end) ? TINFL_FLAG_HAS_MORE_INPUT : 0;

        tinfl_status st = tinfl_decompress(d, inbuf + in_pos, &in_bytes,
                                           out, out + out_pos, &out_bytes,
                                           TINFL_FLAG_PARSE_ZLIB_HEADER |
                                           TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF |
                                           more);
        in_pos += in_bytes;
        total_consumed += in_bytes;
        out_pos += out_bytes;

        if (st == TINFL_STATUS_DONE) {
            ret = (out_pos == out_expected) ? ESP_OK : ESP_FAIL;
            break;
        }
        if (st < TINFL_STATUS_DONE) { ret = ESP_FAIL; break; }
        if (st == TINFL_STATUS_NEEDS_MORE_INPUT && in_pos == in_have && feof(fp)) {
            ret = ESP_FAIL;
            break;
        }
    }

    free(d);
    if (ret == ESP_OK)
        fseek(fp, start + (long)total_consumed, SEEK_SET);
    return ret;
}

/* ------------------------------------------------------------------ */
/* delta application                                                  */
/* ------------------------------------------------------------------ */

static uint64_t delta_varint(const uint8_t **p, const uint8_t *end)
{
    uint64_t v = 0;
    int shift = 0;
    while (*p < end) {
        uint8_t b = *(*p)++;
        v |= (uint64_t)(b & 0x7f) << shift;
        shift += 7;
        if (!(b & 0x80)) break;
    }
    return v;
}

static esp_err_t apply_delta(const uint8_t *base, size_t base_len,
                             const uint8_t *delta, size_t delta_len,
                             git_buf *out)
{
    const uint8_t *p = delta, *end = delta + delta_len;
    uint64_t src = delta_varint(&p, end);
    uint64_t dst = delta_varint(&p, end);
    if (src != base_len) return ESP_FAIL;
    if (dst > GIT_PACK_MAX_OBJ) return ESP_FAIL;
    if (!git_buf_reserve(out, dst + 1)) return ESP_ERR_NO_MEM;

    while (p < end) {
        uint8_t op = *p++;
        if (op & 0x80) {
            /* up to 7 following bytes select copy offset/size */
            int nb = ((op & 1) != 0) + ((op & 2) != 0) + ((op & 4) != 0) +
                     ((op & 8) != 0) + ((op & 16) != 0) + ((op & 32) != 0) +
                     ((op & 64) != 0);
            if (p + nb > end) return ESP_FAIL;
            uint32_t cp_off = 0, cp_size = 0;
            if (op & 0x01) cp_off |= (uint32_t)(*p++);
            if (op & 0x02) cp_off |= (uint32_t)(*p++) << 8;
            if (op & 0x04) cp_off |= (uint32_t)(*p++) << 16;
            if (op & 0x08) cp_off |= (uint32_t)(*p++) << 24;
            if (op & 0x10) cp_size |= (uint32_t)(*p++);
            if (op & 0x20) cp_size |= (uint32_t)(*p++) << 8;
            if (op & 0x40) cp_size |= (uint32_t)(*p++) << 16;
            if (cp_size == 0) cp_size = 0x10000;
            if ((uint64_t)cp_off + cp_size > base_len) return ESP_FAIL;
            if (!git_buf_put(out, base + cp_off, cp_size)) return ESP_ERR_NO_MEM;
        } else if (op) {
            if (p + op > end) return ESP_FAIL;
            if (!git_buf_put(out, p, op)) return ESP_ERR_NO_MEM;
            p += op;
        } else {
            return ESP_FAIL;   /* op == 0 is reserved */
        }
    }
    return out->len == dst ? ESP_OK : ESP_FAIL;
}

/* ------------------------------------------------------------------ */
/* unpack                                                             */
/* ------------------------------------------------------------------ */

esp_err_t git_pack_unpack_file(FILE *fp)
{
    fseek(fp, 0, SEEK_END);
    long file_end = ftell(fp);
    fseek(fp, 0, SEEK_SET);

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
    esp_err_t ret = ESP_OK;

    for (uint32_t i = 0; i < count && ret == ESP_OK; i++) {
        long obj_off = ftell(fp);

        int c = rd_u8(fp);
        if (c < 0) { ret = ESP_FAIL; break; }
        git_obj_type type = (git_obj_type)((c >> 4) & 7);
        uint64_t size = (uint64_t)(c & 0x0f);
        int shift = 4;
        while (c & 0x80) {
            c = rd_u8(fp);
            if (c < 0) { ret = ESP_FAIL; break; }
            size |= (uint64_t)(c & 0x7f) << shift;
            shift += 7;
        }
        if (ret != ESP_OK) break;
        if (size > GIT_PACK_MAX_OBJ) {
            ESP_LOGE(TAG, "object too large: %llu", (unsigned long long)size);
            ret = ESP_FAIL;
            break;
        }

        git_obj_type base_type = GIT_OBJ_BAD;
        git_buf base = GIT_BUF_INIT;

        if (type == GIT_OBJ_OFS_DELTA) {
            int b = rd_u8(fp);
            uint64_t off = (uint64_t)(b & 0x7f);
            while (b & 0x80) {
                b = rd_u8(fp);
                off = ((off + 1) << 7) | (uint64_t)(b & 0x7f);
            }
            long base_off = obj_off - (long)off;
            const off_ent *be = off_map_get(&omap, base_off);
            if (!be) { ESP_LOGE(TAG, "ofs-delta base %ld missing", base_off); ret = ESP_FAIL; break; }
            base_type = be->type;
            git_obj_type t2;
            ret = git_odb_read(&be->oid, &t2, &base);
        } else if (type == GIT_OBJ_REF_DELTA) {
            uint8_t raw[GIT_OID_RAWSZ];
            if (rd_exact(fp, raw, GIT_OID_RAWSZ) != ESP_OK) { ret = ESP_FAIL; break; }
            git_oid base_oid;
            git_oid_from_raw(&base_oid, raw);
            ret = git_odb_read(&base_oid, &base_type, &base);
            if (ret != ESP_OK)
                ESP_LOGE(TAG, "ref-delta base %s missing", base_oid.hex);
        }

        if (ret != ESP_OK) { git_buf_free(&base); break; }

        /* inflate this object's zlib payload (`size` bytes) */
        uint8_t *raw = heap_caps_malloc(size ? size : 1, MALLOC_CAP_SPIRAM);
        if (!raw) raw = malloc(size ? size : 1);
        if (!raw) { git_buf_free(&base); ret = ESP_ERR_NO_MEM; break; }

        ret = inflate_stream(fp, file_end, raw, size);
        if (ret != ESP_OK) { free(raw); git_buf_free(&base); break; }

        git_oid oid;
        if (type == GIT_OBJ_OFS_DELTA || type == GIT_OBJ_REF_DELTA) {
            git_buf result = GIT_BUF_INIT;
            ret = apply_delta(base.data, base.len, raw, size, &result);
            git_buf_free(&base);
            free(raw);
            if (ret != ESP_OK) { git_buf_free(&result); break; }
            ret = git_odb_write(base_type, result.data, result.len, &oid);
            if (ret == ESP_OK) off_map_add(&omap, obj_off, &oid, base_type);
            git_buf_free(&result);
        } else if (type == GIT_OBJ_COMMIT || type == GIT_OBJ_TREE ||
                   type == GIT_OBJ_BLOB || type == GIT_OBJ_TAG) {
            ret = git_odb_write(type, raw, size, &oid);
            free(raw);
            if (ret == ESP_OK) off_map_add(&omap, obj_off, &oid, type);
        } else {
            free(raw);
            ESP_LOGE(TAG, "unknown pack object type %d", type);
            ret = ESP_FAIL;
        }
    }

    if (omap.v) heap_caps_free(omap.v);
    return ret;
}

/* ------------------------------------------------------------------ */
/* write                                                              */
/* ------------------------------------------------------------------ */

static void put_be32(FILE *f, git_sha1_ctx *sha, uint32_t v)
{
    uint8_t b[4] = { v >> 24, v >> 16, v >> 8, v };
    fwrite(b, 1, 4, f);
    git_sha1_update(sha, b, 4);
}

static void put_bytes(FILE *f, git_sha1_ctx *sha, const void *p, size_t n)
{
    fwrite(p, 1, n, f);
    git_sha1_update(sha, p, n);
}

esp_err_t git_pack_write_file(const char *path, const git_oidset *set)
{
    FILE *f = fopen(path, "wb");
    if (!f) return ESP_FAIL;

    git_sha1_ctx *sha = git_sha1_new();
    if (!sha) { fclose(f); return ESP_ERR_NO_MEM; }

    put_bytes(f, sha, "PACK", 4);
    put_be32(f, sha, 2);
    put_be32(f, sha, (uint32_t)set->n);

    esp_err_t ret = ESP_OK;
    for (int i = 0; i < set->n && ret == ESP_OK; i++) {
        git_obj_type type;
        git_buf body = GIT_BUF_INIT;
        ret = git_odb_read(&set->v[i], &type, &body);
        if (ret != ESP_OK) { git_buf_free(&body); break; }

        /* variable-length object header */
        uint8_t hdr[16];
        int hn = 0;
        uint64_t sz = body.len;
        hdr[hn++] = (uint8_t)((type << 4) | (sz & 0x0f)) | (sz >> 4 ? 0x80 : 0);
        sz >>= 4;
        while (sz) {
            hdr[hn++] = (uint8_t)(sz & 0x7f) | (sz >> 7 ? 0x80 : 0);
            sz >>= 7;
        }
        put_bytes(f, sha, hdr, hn);

        git_buf z = GIT_BUF_INIT;
        ret = git_zlib_deflate(body.data, body.len, &z);
        git_buf_free(&body);
        if (ret != ESP_OK) { git_buf_free(&z); break; }
        put_bytes(f, sha, z.data, z.len);
        git_buf_free(&z);
    }

    uint8_t trailer[GIT_OID_RAWSZ];
    git_sha1_final(sha, trailer);
    fwrite(trailer, 1, sizeof(trailer), f);

    if (fclose(f) != 0) ret = ESP_FAIL;
    if (ret != ESP_OK) remove(path);
    return ret;
}
