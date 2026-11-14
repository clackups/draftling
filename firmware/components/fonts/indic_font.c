/*
 * Font driver and shaper of the shaped Indic scripts (Devanagari,
 * Bengali) -- see include/indic_font.h.
 *
 * Blob format (written by tools/gen_indic_font.py, little endian):
 *   magic ("DDEV" / "DBEN"), u8 version (1), u8 line_height,
 *   u8 base_line, u8 cell_w, u8 pixel size, u8 pad, u16 glyph count,
 *   one 8-byte entry per glyph id: u32 bitmap offset, u8 box_w,
 *   u8 box_h, i8 ofs_x, i8 ofs_y; then the bitmaps, 1 bpp, rows packed
 *   MSB-first.
 * One blob per script and font slot; the build embeds the low-res or
 * the hi-res set (CONFIG_DRAFTLING_DISPLAY_HIDPI). Glyph advances and
 * the nominal glyph of every code point are in deva/deva_tables.h and
 * beng/beng_tables.h.
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "sdkconfig.h"
#include "lvgl.h"
#include "hb.h"
#include "indic_font.h"

/* The shaping cache lives in PSRAM: its many small blocks would
 * otherwise go to internal RAM (see components/harfbuzz/config). */
#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#define cache_alloc(n) \
    heap_caps_malloc_prefer((n), 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT, MALLOC_CAP_8BIT)
#define cache_free(p) heap_caps_free(p)
#else
#define cache_alloc(n) malloc(n)
#define cache_free(p) free(p)
#endif

#ifdef CONFIG_DRAFTLING_HAS_DEVANAGARI
#include "deva/deva_tables.h"
#else
#define DEVA_GLYPHS 0
#endif
#ifdef CONFIG_DRAFTLING_HAS_BENGALI
#include "beng/beng_tables.h"
#else
#define BENG_GLYPHS 0
#endif

/* Global glyph ids: Devanagari first, then Bengali. */
#define BENG_BASE    DEVA_GLYPHS
#define TOTAL_GLYPHS (DEVA_GLYPHS + BENG_GLYPHS)
_Static_assert(TOTAL_GLYPHS <= 2048, "glyph ids do not fit the private-use encoding");

/* ---- Embedded data (EMBED_FILES in CMakeLists.txt) ---- */

#define DECL(n) extern const uint8_t _binary_##n##_bin_start[];
#define REF(n)  _binary_##n##_bin_start,
#ifdef CONFIG_DRAFTLING_DISPLAY_HIDPI
#define DEVA_SLOTS(X) X(deva_hires_9) X(deva_hires_11) X(deva_hires_14) X(deva_hires_16) \
                      X(deva_hires_18) X(deva_hires_22) X(deva_hires_26) X(deva_hires_30)
#define BENG_SLOTS(X) X(beng_hires_9) X(beng_hires_11) X(beng_hires_14) X(beng_hires_16) \
                      X(beng_hires_18) X(beng_hires_22) X(beng_hires_26) X(beng_hires_30)
#else
#define DEVA_SLOTS(X) X(deva_lowres_11) X(deva_lowres_14) X(deva_lowres_16) X(deva_lowres_18) \
                      X(deva_lowres_22) X(deva_lowres_26) X(deva_lowres_30) X(deva_lowres_34)
#define BENG_SLOTS(X) X(beng_lowres_11) X(beng_lowres_14) X(beng_lowres_16) X(beng_lowres_18) \
                      X(beng_lowres_22) X(beng_lowres_26) X(beng_lowres_30) X(beng_lowres_34)
#endif
#define SLOT_COUNT 8

#ifdef CONFIG_DRAFTLING_HAS_DEVANAGARI
extern const uint8_t _binary_deva_layout_ttf_start[];
extern const uint8_t _binary_deva_layout_ttf_end[];
DEVA_SLOTS(DECL)
static const uint8_t *const s_deva_blobs[SLOT_COUNT] = { DEVA_SLOTS(REF) };
#endif
#ifdef CONFIG_DRAFTLING_HAS_BENGALI
extern const uint8_t _binary_beng_layout_ttf_start[];
extern const uint8_t _binary_beng_layout_ttf_end[];
BENG_SLOTS(DECL)
static const uint8_t *const s_beng_blobs[SLOT_COUNT] = { BENG_SLOTS(REF) };
#endif

#define HEADER_SIZE 12
#define ENTRY_SIZE  8

/* Private-use code point fields (see indic_font.h). */
#define SPACING_END     0x110000u
#define ADV_UNIT        20      /* font units per advance step */
#define DX_UNIT         8       /* font units per mark offset step */
#define DY_UNIT         10

/* ---- Scripts ---- */

typedef struct {
    const uint8_t *layout;          /* HarfBuzz layout font */
    size_t layout_size;
    const uint8_t *const *blobs;    /* one per slot */
    const char *magic;
    const uint16_t *advance;        /* by local glyph id */
    uint16_t glyphs;
    uint16_t upem;
    uint16_t base;                  /* first global glyph id */
    hb_script_t hb_script;
    hb_font_t *hb_font;
    bool hb_failed;
} script_t;

enum { S_DEVA, S_BENG, S_COUNT };

static script_t s_scripts[S_COUNT];
static bool s_scripts_ready;

static void scripts_init(void)
{
    if (s_scripts_ready) return;
    s_scripts_ready = true;
#ifdef CONFIG_DRAFTLING_HAS_DEVANAGARI
    s_scripts[S_DEVA] = (script_t){
        _binary_deva_layout_ttf_start,
        (size_t)(_binary_deva_layout_ttf_end - _binary_deva_layout_ttf_start),
        s_deva_blobs, "DDEV", DEVA_ADVANCE, DEVA_GLYPHS, DEVA_UPEM, 0,
        HB_SCRIPT_DEVANAGARI, NULL, false,
    };
#endif
#ifdef CONFIG_DRAFTLING_HAS_BENGALI
    s_scripts[S_BENG] = (script_t){
        _binary_beng_layout_ttf_start,
        (size_t)(_binary_beng_layout_ttf_end - _binary_beng_layout_ttf_start),
        s_beng_blobs, "DBEN", BENG_ADVANCE, BENG_GLYPHS, BENG_UPEM, BENG_BASE,
        HB_SCRIPT_BENGALI, NULL, false,
    };
#endif
}

/* The script of a global glyph id, or NULL. */
static const script_t *script_of_gid(uint32_t gid, uint32_t *local)
{
    for (int i = 0; i < S_COUNT; i++) {
        const script_t *s = &s_scripts[i];
        if (s->glyphs && gid >= s->base && gid < (uint32_t)s->base + s->glyphs) {
            *local = gid - s->base;
            return s;
        }
    }
    return NULL;
}

/* ---- Blobs: one per script per font slot ---- */

typedef struct {
    const uint8_t *entries;
    const uint8_t *bitmaps;
    uint16_t count;
    float scale;            /* pixels per font unit */
} blob_t;

typedef struct {
    blob_t blob[S_COUNT];   /* count 0 = script not compiled in */
} slot_t;

static slot_t s_slots[SLOT_COUNT];
static lv_font_t s_fonts[SLOT_COUNT];

static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int32_t px(const blob_t *b, int32_t units)
{
    return (int32_t)lroundf((float)units * b->scale);
}

/* ---- Character classes ---- */

indic_script_t indic_script(uint32_t cp)
{
    if (cp == 0x200C || cp == 0x200D || cp == 0x0964 || cp == 0x0965) return INDIC_ANY;
#ifdef CONFIG_DRAFTLING_HAS_DEVANAGARI
    if ((cp >= 0x0900 && cp <= 0x097F) || (cp >= 0xA8E0 && cp <= 0xA8FF)) return INDIC_DEVA;
#endif
#ifdef CONFIG_DRAFTLING_HAS_BENGALI
    if (cp >= 0x0980 && cp <= 0x09FF) return INDIC_BENG;
#endif
    return INDIC_NONE;
}

static bool is_consonant(uint32_t cp)
{
    return (cp >= 0x0915 && cp <= 0x0939) || (cp >= 0x0958 && cp <= 0x095F) ||
           (cp >= 0x0978 && cp <= 0x097F) ||
           (cp >= 0x0995 && cp <= 0x09B9) || (cp >= 0x09DC && cp <= 0x09DF) ||
           cp == 0x09F0 || cp == 0x09F1;
}

/* Combining characters: signs, vowel signs, nukta, virama, stress and
 * Vedic marks, the Bengali au length mark and sandhi mark; the
 * avagraha (U+093D, U+09BD) and khanda ta (U+09CE) are letters. */
static bool is_mark(uint32_t cp)
{
    return (cp >= 0x0900 && cp <= 0x0903) || (cp >= 0x093A && cp <= 0x094F && cp != 0x093D) ||
           (cp >= 0x0951 && cp <= 0x0957) || cp == 0x0962 || cp == 0x0963 ||
           (cp >= 0xA8E0 && cp <= 0xA8F1) || cp == 0xA8FF ||
           (cp >= 0x0981 && cp <= 0x0983) || cp == 0x09BC ||
           (cp >= 0x09BE && cp <= 0x09C4) || cp == 0x09C7 || cp == 0x09C8 ||
           (cp >= 0x09CB && cp <= 0x09CD) || cp == 0x09D7 || cp == 0x09E2 ||
           cp == 0x09E3 || cp == 0x09FE;
}

bool indic_mid_cluster(uint32_t prev, uint32_t next)
{
    if (next == 0x200C || next == 0x200D) return indic_script(prev) != INDIC_NONE;
    if (is_mark(next)) return indic_script(prev) != INDIC_NONE;
    if (is_consonant(next) && (prev == 0x094D || prev == 0x09CD || prev == 0x200D)) return true;
    return false;
}

int indic_cp_cells(uint32_t cp)
{
    if (cp == 0x200C || cp == 0x200D) return 0;
#ifdef CONFIG_DRAFTLING_HAS_DEVANAGARI
    if (cp >= 0x0900 && cp <= 0x097F) return DEVA_CELLS[cp - 0x0900];
    if (cp >= 0xA8E0 && cp <= 0xA8FF) return 1;
#else
    if (cp == 0x0964 || cp == 0x0965) return 1;
#endif
#ifdef CONFIG_DRAFTLING_HAS_BENGALI
    if (cp >= 0x0980 && cp <= 0x09FF) return BENG_CELLS[cp - 0x0980];
#endif
    return -1;
}

/* ---- Shaping (HarfBuzz) ---- */

static hb_buffer_t *s_hb_buf;

static script_t *hb_ready(indic_script_t which)
{
    scripts_init();
    script_t *s = NULL;
    switch (which) {
    case INDIC_DEVA: s = &s_scripts[S_DEVA]; break;
    case INDIC_BENG: s = &s_scripts[S_BENG]; break;
    case INDIC_ANY:
        s = s_scripts[S_DEVA].glyphs ? &s_scripts[S_DEVA] : &s_scripts[S_BENG];
        break;
    default: return NULL;
    }
    if (!s->glyphs || s->hb_failed) return NULL;
    if (!s_hb_buf) {
        s_hb_buf = hb_buffer_create();
        if (!hb_buffer_allocation_successful(s_hb_buf)) {
            hb_buffer_destroy(s_hb_buf);
            s_hb_buf = NULL;
            return NULL;
        }
    }
    if (s->hb_font) return s;
    hb_blob_t *blob = hb_blob_create((const char *)s->layout, (unsigned int)s->layout_size,
                                     HB_MEMORY_MODE_READONLY, NULL, NULL);
    hb_face_t *face = hb_face_create(blob, 0);
    hb_blob_destroy(blob);
    unsigned int glyphs = hb_face_get_glyph_count(face);
    s->hb_font = hb_font_create(face);
    hb_face_destroy(face);
    if (glyphs != s->glyphs) {
        hb_font_destroy(s->hb_font);
        s->hb_font = NULL;
        s->hb_failed = true;
        return NULL;
    }
    return s;
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
    indic_glyph_t *glyphs;
    uint16_t len;
    uint16_t count;
    uint8_t script;
} cache_entry_t;

static cache_entry_t s_cache[CACHE_SLOTS];

static uint32_t text_hash(const char *s, size_t len, uint32_t script)
{
    uint32_t h = 2166136261u ^ script;     /* FNV-1a */
    for (size_t i = 0; i < len; i++) h = (h ^ (uint8_t)s[i]) * 16777619u;
    return h;
}

static size_t shape_uncached(indic_script_t script, const char *s, size_t len,
                             indic_glyph_t *out, size_t max);

size_t indic_shape(indic_script_t script, const char *s, size_t len,
                   indic_glyph_t *out, size_t max)
{
    if (len == 0) return 0;
    cache_entry_t *e = NULL;
    if (len <= CACHE_MAX_LEN) {
        e = &s_cache[text_hash(s, len, (uint32_t)script) % CACHE_SLOTS];
        if (e->text && e->len == len && e->script == (uint8_t)script &&
            memcmp(e->text, s, len) == 0) {
            if (e->count > max) return 0;
            memcpy(out, e->glyphs, e->count * sizeof(indic_glyph_t));
            return e->count;
        }
    }
    size_t n = shape_uncached(script, s, len, out, max);
    if (e && n > 0) {
        char *t = (char *)cache_alloc(len);
        indic_glyph_t *g = (indic_glyph_t *)cache_alloc(n * sizeof(indic_glyph_t));
        if (t && g) {
            cache_free(e->text);
            cache_free(e->glyphs);
            memcpy(t, s, len);
            memcpy(g, out, n * sizeof(indic_glyph_t));
            e->text = t;
            e->glyphs = g;
            e->len = (uint16_t)len;
            e->count = (uint16_t)n;
            e->script = (uint8_t)script;
        } else {
            cache_free(t);
            cache_free(g);
        }
    }
    return n;
}

static size_t shape_uncached(indic_script_t script, const char *s, size_t len,
                             indic_glyph_t *out, size_t max)
{
    const script_t *sc = hb_ready(script);
    if (!sc) return 0;
    hb_buffer_clear_contents(s_hb_buf);
    hb_buffer_add_utf8(s_hb_buf, s, (int)len, 0, (int)len);
    hb_buffer_set_direction(s_hb_buf, HB_DIRECTION_LTR);
    hb_buffer_set_script(s_hb_buf, sc->hb_script);
    hb_buffer_guess_segment_properties(s_hb_buf);
    hb_shape(sc->hb_font, s_hb_buf, NULL, 0);

    unsigned int n = 0;
    hb_glyph_info_t *info = hb_buffer_get_glyph_infos(s_hb_buf, &n);
    hb_glyph_position_t *pos = hb_buffer_get_glyph_positions(s_hb_buf, &n);
    size_t k = 0;
    for (unsigned int i = 0; i < n; i++) {
        uint32_t lid = info[i].codepoint;
        if (lid >= sc->glyphs) continue;
        uint32_t gid = sc->base + lid;
        if (pos[i].x_advance == 0) {
            /* A glyph without ink and advance (the joiners): nothing. */
            if (sc->advance[lid] != 0) continue;
            int32_t dx = clamp_steps(pos[i].x_offset, DX_UNIT, -128, 127);
            int32_t dy = clamp_steps(pos[i].y_offset, DY_UNIT, -32, 31);
            if (dx == 0 && dy == 0) {
                if (k >= max) return 0;
                out[k].code = INDIC_PUA_MARK + gid;
                out[k].cluster = info[i].cluster;
                k++;
                continue;
            }
            if (k + 2 > max) return 0;
            out[k].code = INDIC_PUA_OFFSET | (((uint32_t)dx & 0xFF) << 6) | ((uint32_t)dy & 0x3F);
            out[k].cluster = info[i].cluster;
            out[k + 1].code = INDIC_PUA_MARK_DONE + gid;
            out[k + 1].cluster = info[i].cluster;
            k += 2;
            continue;
        }
        int32_t d = clamp_steps(pos[i].x_advance - (int32_t)sc->advance[lid],
                                ADV_UNIT, -16, 15);
        if (k >= max) return 0;
        out[k].code = INDIC_PUA_SPACING + (gid << 5) + (uint32_t)(d + 16);
        out[k].cluster = info[i].cluster;
        k++;
    }
    return k;
}

/* ---- Font driver ---- */

/* The blob and local glyph id of a global glyph id in a slot. */
static const blob_t *resolve(const slot_t *sl, uint32_t gid, uint32_t *local,
                             const script_t **sc)
{
    const script_t *s = script_of_gid(gid, local);
    if (!s) return NULL;
    const blob_t *b = &sl->blob[s - s_scripts];
    if (*local >= b->count) return NULL;
    if (sc) *sc = s;
    return b;
}

static void set_empty(lv_font_glyph_dsc_t *dsc)
{
    dsc->adv_w = 0;
    dsc->box_w = 0;
    dsc->box_h = 0;
    dsc->ofs_x = 0;
    dsc->ofs_y = 0;
    dsc->stride = 0;
    dsc->format = LV_FONT_GLYPH_FORMAT_A1;
    dsc->is_placeholder = 0;
    dsc->gid.index = 0;
}

/* Fill dsc for global glyph gid: advance adv_units font units plus
 * adv_steps advance steps (spacing glyphs only), moved by dx / dy font
 * units. */
static bool set_box(const slot_t *sl, lv_font_glyph_dsc_t *dsc, uint32_t gid,
                    bool spacing, int32_t adv_steps, int32_t dx, int32_t dy)
{
    uint32_t local;
    const script_t *sc;
    const blob_t *b = resolve(sl, gid, &local, &sc);
    if (!b) return false;
    const uint8_t *e = b->entries + (size_t)local * ENTRY_SIZE;
    int32_t adv = spacing ? px(b, (int32_t)sc->advance[local] + adv_steps * ADV_UNIT) : 0;
    dsc->adv_w = (uint16_t)(adv > 0 ? adv : 0);
    dsc->box_w = e[4];
    dsc->box_h = e[5];
    dsc->ofs_x = (int16_t)((int8_t)e[6] + px(b, dx));
    dsc->ofs_y = (int16_t)((int8_t)e[7] + px(b, dy));
    dsc->stride = 0;
    dsc->format = LV_FONT_GLYPH_FORMAT_A1;
    dsc->is_placeholder = 0;
    dsc->gid.index = gid;
    return true;
}

/* Global glyph id of the nominal glyph of cp (unshaped text), or 0. */
static uint32_t nominal_gid(uint32_t cp)
{
#ifdef CONFIG_DRAFTLING_HAS_DEVANAGARI
    if (cp >= 0x0900 && cp <= 0x097F) return DEVA_CMAP[cp - 0x0900];
#endif
#ifdef CONFIG_DRAFTLING_HAS_BENGALI
    if (cp >= 0x0980 && cp <= 0x09FF) {
        uint32_t g = BENG_CMAP[cp - 0x0980];
        return g ? BENG_BASE + g : 0;
    }
#ifndef CONFIG_DRAFTLING_HAS_DEVANAGARI
    if (cp == 0x0964 || cp == 0x0965) return BENG_BASE + BENG_DANDA[cp - 0x0964];
#endif
#endif
    (void)cp;
    return 0;
}

static bool indic_get_glyph_dsc(const lv_font_t *font, lv_font_glyph_dsc_t *dsc,
                                uint32_t letter, uint32_t letter_next)
{
    const slot_t *sl = (const slot_t *)font->dsc;

    if (letter >= INDIC_PUA_OFFSET && letter < INDIC_PUA_MARK) {
        /* An offset: draws the mark that follows it, moved. */
        if (letter_next < INDIC_PUA_MARK_DONE ||
            letter_next >= INDIC_PUA_MARK_DONE + TOTAL_GLYPHS) {
            set_empty(dsc);
            return true;
        }
        int32_t dx = (int32_t)((letter >> 6) & 0xFF);
        int32_t dy = (int32_t)(letter & 0x3F);
        if (dx & 0x80) dx -= 0x100;
        if (dy & 0x20) dy -= 0x40;
        return set_box(sl, dsc, letter_next - INDIC_PUA_MARK_DONE, false, 0,
                       dx * DX_UNIT, dy * DY_UNIT);
    }
    if (letter >= INDIC_PUA_MARK && letter < INDIC_PUA_MARK + TOTAL_GLYPHS) {
        /* A mark in place. */
        return set_box(sl, dsc, letter - INDIC_PUA_MARK, false, 0, 0, 0);
    }
    if (letter >= INDIC_PUA_MARK_DONE && letter < INDIC_PUA_MARK_DONE + TOTAL_GLYPHS) {
        /* A mark the offset before it has drawn: nothing. */
        set_empty(dsc);
        return true;
    }
    if (letter >= INDIC_PUA_SPACING && letter < SPACING_END) {
        uint32_t gid = (letter - INDIC_PUA_SPACING) >> 5;
        if (gid >= TOTAL_GLYPHS) return false;
        return set_box(sl, dsc, gid, true, (int32_t)(letter & 31) - 16, 0, 0);
    }
    /* Unshaped text (titles, file names): the nominal glyphs. */
    uint32_t gid = nominal_gid(letter);
    if (gid == 0) return false;
    return set_box(sl, dsc, gid, true, 0, 0, 0);
}

static const void *indic_get_glyph_bitmap(lv_font_glyph_dsc_t *g, lv_draw_buf_t *draw_buf)
{
    if (!draw_buf || !g->resolved_font) return NULL;
    const slot_t *sl = (const slot_t *)g->resolved_font->dsc;
    uint32_t local;
    const blob_t *b = resolve(sl, g->gid.index, &local, NULL);
    if (!b) return NULL;
    const uint8_t *e = b->entries + (size_t)local * ENTRY_SIZE;
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

lv_font_t *indic_font_create(int32_t line_height, int cell_w)
{
    scripts_init();
    for (size_t i = 0; i < SLOT_COUNT; i++) {
        slot_t *sl = &s_slots[i];
        const uint8_t *hdr = NULL;
        for (int k = 0; k < S_COUNT; k++) {
            const script_t *s = &s_scripts[k];
            if (!s->glyphs) continue;
            const uint8_t *d = s->blobs[i];
            if (memcmp(d, s->magic, 4) != 0 || d[4] != 1) continue;
            if (d[5] != line_height || d[7] != cell_w) continue;
            blob_t *b = &sl->blob[k];
            b->count = (uint16_t)(d[10] | (d[11] << 8));
            b->scale = (float)d[8] / (float)s->upem;
            b->entries = d + HEADER_SIZE;
            b->bitmaps = b->entries + (size_t)b->count * ENTRY_SIZE;
            hdr = d;
        }
        if (!hdr) continue;

        lv_font_t *f = &s_fonts[i];
        memset(f, 0, sizeof(*f));
        f->get_glyph_dsc = indic_get_glyph_dsc;
        f->get_glyph_bitmap = indic_get_glyph_bitmap;
        f->line_height = hdr[5];
        f->base_line = hdr[6];
        f->subpx = LV_FONT_SUBPX_NONE;
        f->underline_position = -1;
        f->underline_thickness = 1;
        f->dsc = sl;
        return f;
    }
    return NULL;
}
