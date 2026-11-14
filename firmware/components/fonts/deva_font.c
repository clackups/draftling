/*
 * Devanagari font driver and shaper -- see include/deva_font.h.
 *
 * Blob format (written by tools/gen_deva_font.py, little endian):
 *   "DDEV", u8 version (1), u8 line_height, u8 base_line, u8 cell_w,
 *   u8 pixel size, u8 pad, u16 glyph count, one 8-byte entry per glyph
 *   id: u32 bitmap offset, u8 box_w, u8 box_h, i8 ofs_x, i8 ofs_y;
 *   then the bitmaps, 1 bpp, rows packed MSB-first.
 * One blob per font slot; the build embeds the low-res or the hi-res
 * set (CONFIG_DRAFTLING_DISPLAY_HIDPI). Glyph advances, the mark list
 * and the nominal glyph of every code point are in deva/deva_tables.h.
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "sdkconfig.h"
#include "lvgl.h"
#include "hb.h"
#include "deva_font.h"
#include "deva/deva_tables.h"

/* ---- Embedded data (EMBED_FILES in CMakeLists.txt) ---- */

extern const uint8_t _binary_deva_layout_ttf_start[];
extern const uint8_t _binary_deva_layout_ttf_end[];

#ifdef CONFIG_DRAFTLING_DISPLAY_HIDPI
extern const uint8_t _binary_deva_hires_9_bin_start[];
extern const uint8_t _binary_deva_hires_11_bin_start[];
extern const uint8_t _binary_deva_hires_14_bin_start[];
extern const uint8_t _binary_deva_hires_16_bin_start[];
extern const uint8_t _binary_deva_hires_18_bin_start[];
extern const uint8_t _binary_deva_hires_22_bin_start[];
extern const uint8_t _binary_deva_hires_26_bin_start[];
extern const uint8_t _binary_deva_hires_30_bin_start[];
static const uint8_t *const s_blobs[] = {
    _binary_deva_hires_9_bin_start,  _binary_deva_hires_11_bin_start,
    _binary_deva_hires_14_bin_start, _binary_deva_hires_16_bin_start,
    _binary_deva_hires_18_bin_start, _binary_deva_hires_22_bin_start,
    _binary_deva_hires_26_bin_start, _binary_deva_hires_30_bin_start,
};
#else
extern const uint8_t _binary_deva_lowres_11_bin_start[];
extern const uint8_t _binary_deva_lowres_14_bin_start[];
extern const uint8_t _binary_deva_lowres_16_bin_start[];
extern const uint8_t _binary_deva_lowres_18_bin_start[];
extern const uint8_t _binary_deva_lowres_22_bin_start[];
extern const uint8_t _binary_deva_lowres_26_bin_start[];
extern const uint8_t _binary_deva_lowres_30_bin_start[];
extern const uint8_t _binary_deva_lowres_34_bin_start[];
static const uint8_t *const s_blobs[] = {
    _binary_deva_lowres_11_bin_start, _binary_deva_lowres_14_bin_start,
    _binary_deva_lowres_16_bin_start, _binary_deva_lowres_18_bin_start,
    _binary_deva_lowres_22_bin_start, _binary_deva_lowres_26_bin_start,
    _binary_deva_lowres_30_bin_start, _binary_deva_lowres_34_bin_start,
};
#endif
#define BLOB_COUNT (sizeof(s_blobs) / sizeof(s_blobs[0]))

#define HEADER_SIZE 12
#define ENTRY_SIZE  8

/* Private-use code point fields (see deva_font.h). */
#define SPACING_END     0x100000u
#define ADV_UNIT        10      /* font units per advance step */
#define DX_UNIT         4       /* font units per mark offset step */
#define DY_UNIT         8

typedef struct {
    const uint8_t *entries;
    const uint8_t *bitmaps;
    uint16_t count;
    float scale;            /* pixels per font unit */
} deva_blob_t;

static deva_blob_t s_parsed[BLOB_COUNT];
static lv_font_t s_fonts[BLOB_COUNT];

static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int32_t px(const deva_blob_t *b, int32_t units)
{
    return (int32_t)lroundf((float)units * b->scale);
}

/* ---- Character classes ---- */

bool deva_in_run(uint32_t cp)
{
    return (cp >= 0x0900 && cp <= 0x097F) || (cp >= 0xA8E0 && cp <= 0xA8FF) ||
           cp == 0x200C || cp == 0x200D;
}

static bool is_consonant(uint32_t cp)
{
    return (cp >= 0x0915 && cp <= 0x0939) || (cp >= 0x0958 && cp <= 0x095F) ||
           (cp >= 0x0978 && cp <= 0x097F);
}

/* Combining characters: signs, vowel signs, nukta, virama, stress
 * and Vedic marks; the avagraha (U+093D) is a letter. */
static bool is_mark(uint32_t cp)
{
    return (cp >= 0x0900 && cp <= 0x0903) || (cp >= 0x093A && cp <= 0x094F && cp != 0x093D) ||
           (cp >= 0x0951 && cp <= 0x0957) || cp == 0x0962 || cp == 0x0963 ||
           (cp >= 0xA8E0 && cp <= 0xA8F1) || cp == 0xA8FF;
}

bool deva_mid_cluster(uint32_t prev, uint32_t next)
{
    if (next == 0x200C || next == 0x200D) return deva_in_run(prev) && prev != 0;
    if (is_mark(next)) return deva_in_run(prev) && prev != 0;
    if (is_consonant(next) && (prev == 0x094D || prev == 0x200D)) return true;
    return false;
}

int deva_cp_cells(uint32_t cp)
{
    if (cp >= 0x0900 && cp <= 0x097F) return DEVA_CELLS[cp - 0x0900];
    if (cp == 0x200C || cp == 0x200D) return 0;
    if (cp >= 0xA8E0 && cp <= 0xA8FF) return 1;
    return -1;
}

/* ---- Shaping (HarfBuzz) ---- */

static hb_font_t *s_hb_font;
static hb_buffer_t *s_hb_buf;
static bool s_hb_failed;

static bool hb_ready(void)
{
    if (s_hb_font) return true;
    if (s_hb_failed) return false;
    size_t size = (size_t)(_binary_deva_layout_ttf_end - _binary_deva_layout_ttf_start);
    hb_blob_t *blob = hb_blob_create((const char *)_binary_deva_layout_ttf_start,
                                     (unsigned int)size, HB_MEMORY_MODE_READONLY,
                                     NULL, NULL);
    hb_face_t *face = hb_face_create(blob, 0);
    hb_blob_destroy(blob);
    unsigned int glyphs = hb_face_get_glyph_count(face);
    s_hb_font = hb_font_create(face);
    hb_face_destroy(face);
    s_hb_buf = hb_buffer_create();
    if (!hb_buffer_allocation_successful(s_hb_buf) || glyphs != DEVA_GLYPHS) {
        hb_buffer_destroy(s_hb_buf);
        hb_font_destroy(s_hb_font);
        s_hb_buf = NULL;
        s_hb_font = NULL;
        s_hb_failed = true;
        return false;
    }
    return true;
}

static int32_t clamp_steps(int32_t v, int32_t unit, int32_t lo, int32_t hi)
{
    int32_t s = (v >= 0) ? (v + unit / 2) / unit : -((-v + unit / 2) / unit);
    return s < lo ? lo : (s > hi ? hi : s);
}

/* ---- Shaping cache ----
 * The editor rebuilds the view of every visible line on each refresh;
 * shaping a line takes a few milliseconds, so the results of recent
 * runs are kept (direct-mapped by a hash of the text, compared in
 * full). Only runs up to CACHE_MAX_LEN bytes are cached. */
#define CACHE_SLOTS   64
#define CACHE_MAX_LEN 512

typedef struct {
    char *text;
    deva_glyph_t *glyphs;
    uint16_t len;
    uint16_t count;
} cache_entry_t;

static cache_entry_t s_cache[CACHE_SLOTS];

static uint32_t text_hash(const char *s, size_t len)
{
    uint32_t h = 2166136261u;     /* FNV-1a */
    for (size_t i = 0; i < len; i++) h = (h ^ (uint8_t)s[i]) * 16777619u;
    return h;
}

static size_t shape_uncached(const char *s, size_t len, deva_glyph_t *out, size_t max);

size_t deva_shape(const char *s, size_t len, deva_glyph_t *out, size_t max)
{
    if (len == 0) return 0;
    cache_entry_t *e = NULL;
    if (len <= CACHE_MAX_LEN) {
        e = &s_cache[text_hash(s, len) % CACHE_SLOTS];
        if (e->text && e->len == len && memcmp(e->text, s, len) == 0) {
            if (e->count > max) return 0;
            memcpy(out, e->glyphs, e->count * sizeof(deva_glyph_t));
            return e->count;
        }
    }
    size_t n = shape_uncached(s, len, out, max);
    if (e && n > 0) {
        char *t = (char *)malloc(len);
        deva_glyph_t *g = (deva_glyph_t *)malloc(n * sizeof(deva_glyph_t));
        if (t && g) {
            free(e->text);
            free(e->glyphs);
            memcpy(t, s, len);
            memcpy(g, out, n * sizeof(deva_glyph_t));
            e->text = t;
            e->glyphs = g;
            e->len = (uint16_t)len;
            e->count = (uint16_t)n;
        } else {
            free(t);
            free(g);
        }
    }
    return n;
}

static size_t shape_uncached(const char *s, size_t len, deva_glyph_t *out, size_t max)
{
    if (!hb_ready()) return 0;
    hb_buffer_clear_contents(s_hb_buf);
    hb_buffer_add_utf8(s_hb_buf, s, (int)len, 0, (int)len);
    hb_buffer_set_direction(s_hb_buf, HB_DIRECTION_LTR);
    hb_buffer_set_script(s_hb_buf, HB_SCRIPT_DEVANAGARI);
    hb_buffer_guess_segment_properties(s_hb_buf);
    hb_shape(s_hb_font, s_hb_buf, NULL, 0);

    unsigned int n = 0;
    hb_glyph_info_t *info = hb_buffer_get_glyph_infos(s_hb_buf, &n);
    hb_glyph_position_t *pos = hb_buffer_get_glyph_positions(s_hb_buf, &n);
    size_t k = 0;
    for (unsigned int i = 0; i < n; i++) {
        uint32_t gid = info[i].codepoint;
        if (gid >= DEVA_GLYPHS) continue;
        if (pos[i].x_advance == 0) {
            /* A glyph without ink and advance (the joiners): nothing. */
            if (DEVA_ADVANCE[gid] != 0) continue;
            int32_t dx = clamp_steps(pos[i].x_offset, DX_UNIT, -128, 127);
            int32_t dy = clamp_steps(pos[i].y_offset, DY_UNIT, -64, 63);
            if (dx == 0 && dy == 0) {
                if (k >= max) return 0;
                out[k].code = DEVA_PUA_MARK + gid;
                out[k].cluster = info[i].cluster;
                k++;
                continue;
            }
            if (k + 2 > max) return 0;
            out[k].code = DEVA_PUA_OFFSET | (((uint32_t)dx & 0xFF) << 7) | ((uint32_t)dy & 0x7F);
            out[k].cluster = info[i].cluster;
            out[k + 1].code = DEVA_PUA_MARK_DONE + gid;
            out[k + 1].cluster = info[i].cluster;
            k += 2;
            continue;
        }
        int32_t d = clamp_steps(pos[i].x_advance - (int32_t)DEVA_ADVANCE[gid],
                                ADV_UNIT, -16, 15);
        if (k >= max) return 0;
        out[k].code = DEVA_PUA_SPACING + (gid << 5) + (uint32_t)(d + 16);
        out[k].cluster = info[i].cluster;
        k++;
    }
    return k;
}

/* ---- Font driver ---- */

static bool set_box(const deva_blob_t *b, lv_font_glyph_dsc_t *dsc, uint32_t gid,
                    int32_t adv, int32_t dx, int32_t dy)
{
    if (gid >= b->count) return false;
    const uint8_t *e = b->entries + (size_t)gid * ENTRY_SIZE;
    dsc->adv_w = (uint16_t)(adv > 0 ? adv : 0);
    dsc->box_w = e[4];
    dsc->box_h = e[5];
    dsc->ofs_x = (int16_t)((int8_t)e[6] + dx);
    dsc->ofs_y = (int16_t)((int8_t)e[7] + dy);
    dsc->stride = 0;
    dsc->format = LV_FONT_GLYPH_FORMAT_A1;
    dsc->is_placeholder = 0;
    dsc->gid.index = gid;
    return true;
}

static bool deva_get_glyph_dsc(const lv_font_t *font, lv_font_glyph_dsc_t *dsc,
                               uint32_t letter, uint32_t letter_next)
{
    const deva_blob_t *b = (const deva_blob_t *)font->dsc;

    if (letter >= DEVA_PUA_OFFSET && letter < DEVA_PUA_MARK) {
        /* An offset: draws the mark that follows it, moved. */
        if (letter_next < DEVA_PUA_MARK_DONE || letter_next >= DEVA_PUA_MARK_DONE + DEVA_GLYPHS) {
            set_box(b, dsc, 0, 0, 0, 0);
            dsc->box_w = 0;
            dsc->box_h = 0;
            return true;
        }
        int32_t dx = (int32_t)((letter >> 7) & 0xFF);
        int32_t dy = (int32_t)(letter & 0x7F);
        if (dx & 0x80) dx -= 0x100;
        if (dy & 0x40) dy -= 0x80;
        return set_box(b, dsc, letter_next - DEVA_PUA_MARK_DONE, 0,
                       px(b, dx * DX_UNIT), px(b, dy * DY_UNIT));
    }
    if (letter >= DEVA_PUA_MARK && letter < DEVA_PUA_MARK + DEVA_GLYPHS) {
        /* A mark in place. */
        return set_box(b, dsc, letter - DEVA_PUA_MARK, 0, 0, 0);
    }
    if (letter >= DEVA_PUA_MARK_DONE && letter < DEVA_PUA_MARK_DONE + DEVA_GLYPHS) {
        /* A mark the offset before it has drawn: nothing. */
        set_box(b, dsc, 0, 0, 0, 0);
        dsc->box_w = 0;
        dsc->box_h = 0;
        return true;
    }
    if (letter >= DEVA_PUA_SPACING && letter < SPACING_END) {
        uint32_t gid = (letter - DEVA_PUA_SPACING) >> 5;
        if (gid >= DEVA_GLYPHS) return false;
        int32_t d = (int32_t)(letter & 31) - 16;
        return set_box(b, dsc, gid, px(b, (int32_t)DEVA_ADVANCE[gid] + d * ADV_UNIT), 0, 0);
    }
    /* Unshaped text (titles, file names): the nominal glyphs. */
    if (letter >= 0x0900 && letter <= 0x097F) {
        uint32_t gid = DEVA_CMAP[letter - 0x0900];
        if (gid == 0) return false;
        return set_box(b, dsc, gid, px(b, DEVA_ADVANCE[gid]), 0, 0);
    }
    return false;
}

static const void *deva_get_glyph_bitmap(lv_font_glyph_dsc_t *g, lv_draw_buf_t *draw_buf)
{
    if (!draw_buf || !g->resolved_font) return NULL;
    const deva_blob_t *b = (const deva_blob_t *)g->resolved_font->dsc;
    uint32_t gid = g->gid.index;
    if (gid >= b->count) return NULL;
    const uint8_t *e = b->entries + (size_t)gid * ENTRY_SIZE;
    int w = e[4], h = e[5];
    if (g->box_w != w || g->box_h != h) return NULL;
    const uint8_t *src = b->bitmaps + rd32(e);

    uint32_t stride = draw_buf->header.stride;
    uint8_t *out = draw_buf->data;
    for (int y = 0; y < h; y++) {
        uint8_t *row = out + (size_t)y * stride;
        for (int x = 0; x < w; x++) {
            int bit = y * w + x;
            row[x] = (src[bit >> 3] & (0x80 >> (bit & 7))) ? 0xFF : 0x00;
        }
    }
    lv_draw_buf_flush_cache(draw_buf, NULL);
    return draw_buf;
}

lv_font_t *deva_font_create(int32_t line_height, int cell_w)
{
    for (size_t i = 0; i < BLOB_COUNT; i++) {
        const uint8_t *d = s_blobs[i];
        if (memcmp(d, "DDEV", 4) != 0 || d[4] != 1) continue;
        if (d[5] != line_height || d[7] != cell_w) continue;

        deva_blob_t *b = &s_parsed[i];
        b->count = (uint16_t)(d[10] | (d[11] << 8));
        b->scale = (float)d[8] / (float)DEVA_UPEM;
        b->entries = d + HEADER_SIZE;
        b->bitmaps = b->entries + (size_t)b->count * ENTRY_SIZE;

        lv_font_t *f = &s_fonts[i];
        memset(f, 0, sizeof(*f));
        f->get_glyph_dsc = deva_get_glyph_dsc;
        f->get_glyph_bitmap = deva_get_glyph_bitmap;
        f->line_height = d[5];
        f->base_line = d[6];
        f->subpx = LV_FONT_SUBPX_NONE;
        f->underline_position = -1;
        f->underline_thickness = 1;
        f->dsc = b;
        return f;
    }
    return NULL;
}
