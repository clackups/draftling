/*
 * CJK pixel-font driver -- see include/cjk_font.h.
 *
 * Blob format (written by tools/gen_cjk_fonts.py, little endian):
 *   "DCJK", u8 version, u8 N, u8 ascent, u8 reserved, u32 count,
 *   u16 code points[count] (ascending), then count N x N bitmaps of
 *   ceil(N*N/8) bytes each, 1 bpp, rows packed MSB-first.
 *
 * The language pack picks one font set (cjk/ja, cjk/zh or cjk/ko, see
 * CONFIG_DRAFTLING_CJK_FONT_SET): that language's glyph repertoire in
 * its regional glyph style, one blob per native size.
 */

#include <string.h>
#include "sdkconfig.h"
#include "lvgl.h"
#include "cjk_font.h"

/* ---- Embedded blobs (EMBED_FILES in CMakeLists.txt) ---- */

extern const uint8_t _binary_cjk10_bin_start[];
extern const uint8_t _binary_cjk12_bin_start[];
extern const uint8_t _binary_cjk16_bin_start[];

/* ---- Parsed blob ---- */

typedef struct {
    const uint8_t *cps;     /* u16 LE code points */
    const uint8_t *bitmaps;
    uint32_t count;
    uint16_t bpg;           /* bytes per glyph */
    uint8_t  n;             /* cell size */
} cjk_blob_t;

/* Native sizes: index 0 = 10 px, 1 = 12 px, 2 = 16 px. */
#define NATIVE_COUNT 3
static cjk_blob_t s_base[NATIVE_COUNT];
static bool s_loaded;

static void blob_parse(cjk_blob_t *b, const uint8_t *d)
{
    memset(b, 0, sizeof(*b));
    if (!d || memcmp(d, "DCJK", 4) != 0 || d[4] != 1) return;
    b->n = d[5];
    b->count = (uint32_t)d[8] | ((uint32_t)d[9] << 8) |
               ((uint32_t)d[10] << 16) | ((uint32_t)d[11] << 24);
    b->bpg = (uint16_t)((b->n * b->n + 7) / 8);
    b->cps = d + 12;
    b->bitmaps = b->cps + 2 * b->count;
}

static void load_blobs(void)
{
    if (s_loaded) return;
    blob_parse(&s_base[0], _binary_cjk10_bin_start);
    blob_parse(&s_base[1], _binary_cjk12_bin_start);
    blob_parse(&s_base[2], _binary_cjk16_bin_start);
    s_loaded = true;
}

/* Index of cp in the blob, or -1. */
static int32_t blob_find(const cjk_blob_t *b, uint32_t cp)
{
    int32_t lo = 0, hi = (int32_t)b->count - 1;
    while (lo <= hi) {
        int32_t mid = (lo + hi) >> 1;
        uint32_t v = (uint32_t)b->cps[2 * mid] | ((uint32_t)b->cps[2 * mid + 1] << 8);
        if (v == cp) return mid;
        if (v < cp) lo = mid + 1;
        else hi = mid - 1;
    }
    return -1;
}

/* ---- Per-slot fonts ---- */

/* Native size + integer scale; tried in order until one has the glyph. */
typedef struct {
    uint8_t native;   /* 0..NATIVE_COUNT-1 */
    uint8_t scale;    /* 1.. */
} cjk_choice_t;

#define MAX_CHOICES 6

typedef struct {
    cjk_choice_t choice[MAX_CHOICES];
    uint8_t nchoice;
    int16_t line_height;
    int16_t base_line;
    int16_t adv;      /* two monospace cells */
} cjk_slot_t;

#define SLOT_POOL 16
static cjk_slot_t s_slots[SLOT_POOL];
static lv_font_t  s_fonts[SLOT_POOL];
static int        s_slot_count;

bool cjk_is_wide(uint32_t cp)
{
    return (cp >= 0x2E80 && cp <= 0x2FDF) ||
           (cp >= 0x3000 && cp <= 0x33FF) ||
           (cp >= 0x3400 && cp <= 0x4DBF) ||
           (cp >= 0x4E00 && cp <= 0x9FFF) ||
           (cp >= 0xAC00 && cp <= 0xD7A3) ||
           (cp >= 0xF900 && cp <= 0xFAFF) ||
           (cp >= 0xFE30 && cp <= 0xFE4F) ||
           (cp >= 0xFF01 && cp <= 0xFF60) ||
           (cp >= 0xFFE0 && cp <= 0xFFE6);
}

/* glyph id: bits 28..29 = native size, 24..27 = scale, 0..23 = glyph
 * index. */
#define GID(nat, scale, idx) \
    (((uint32_t)(nat) << 28) | ((uint32_t)(scale) << 24) | (uint32_t)(idx))

static bool cjk_get_glyph_dsc(const lv_font_t *font, lv_font_glyph_dsc_t *dsc,
                              uint32_t letter, uint32_t letter_next)
{
    LV_UNUSED(letter_next);
    if (!cjk_is_wide(letter)) return false;
    const cjk_slot_t *slot = (const cjk_slot_t *)font->dsc;

    for (int i = 0; i < slot->nchoice; i++) {
        uint8_t nat = slot->choice[i].native;
        uint8_t k = slot->choice[i].scale;
        int32_t idx = blob_find(&s_base[nat], letter);
        if (idx < 0) continue;

        int32_t sz = (int32_t)s_base[nat].n * k;
        dsc->adv_w = (uint16_t)slot->adv;
        dsc->box_w = (uint16_t)sz;
        dsc->box_h = (uint16_t)sz;
        dsc->ofs_x = (int16_t)((slot->adv - sz) / 2);
        /* Centre the box vertically in the base font's line box. */
        int32_t gap = slot->line_height - sz;
        dsc->ofs_y = (int16_t)(-slot->base_line + (gap >= 0 ? gap / 2 : -((-gap + 1) / 2)));
        dsc->stride = 0;
        dsc->format = LV_FONT_GLYPH_FORMAT_A1;
        dsc->is_placeholder = 0;
        dsc->gid.index = GID(nat, k, idx);
        return true;
    }
    return false;
}

static const void *cjk_get_glyph_bitmap(lv_font_glyph_dsc_t *g, lv_draw_buf_t *draw_buf)
{
    if (!draw_buf) return NULL;   /* no static (raw) bitmaps */
    uint32_t gid = g->gid.index;
    uint8_t nat = (gid >> 28) & 0x3;
    uint8_t k = (gid >> 24) & 0xF;
    uint32_t idx = gid & 0xFFFFFF;
    if (nat >= NATIVE_COUNT || k == 0) return NULL;

    const cjk_blob_t *b = &s_base[nat];
    if (idx >= b->count) return NULL;
    const uint8_t *src = b->bitmaps + (size_t)idx * b->bpg;
    int n = b->n;
    int sz = n * k;
    if (g->box_w != sz || g->box_h != sz) return NULL;

    uint32_t stride = draw_buf->header.stride;
    uint8_t *out = draw_buf->data;
    for (int y = 0; y < sz; y++) {
        int sy = y / k;
        uint8_t *row = out + (size_t)y * stride;
        for (int x = 0; x < sz; x++) {
            int bit = sy * n + x / k;
            row[x] = (src[bit >> 3] & (0x80 >> (bit & 7))) ? 0xFF : 0x00;
        }
    }
    lv_draw_buf_flush_cache(draw_buf, NULL);
    return draw_buf;
}

/* Fill slot->choice: the largest integer-scaled native size that fits
 * the slot's limit first, then smaller ones (for glyphs missing from
 * the first choice), then oversize ones as a last resort, so a missing
 * glyph is drawn too big rather than not at all. The 10 px font is
 * only used unscaled: doubling it looks coarse and it has the
 * smallest repertoire. */
static void plan_choices(cjk_slot_t *slot, int limit)
{
    typedef struct { int size; int nat; int k; } cand_t;
    cand_t c[NATIVE_COUNT * 6];
    int nc = 0;
    for (int nat = 0; nat < NATIVE_COUNT; nat++) {
        if (s_base[nat].count == 0) continue;
        for (int k = 1; k <= 6; k++) {
            if (nat == 0 && k > 1) break;
            c[nc].size = s_base[nat].n * k;
            c[nc].nat = nat;
            c[nc].k = k;
            nc++;
        }
    }
    /* Order: sizes that fit, largest first (ties: larger native size);
     * then sizes that do not fit, smallest first. */
    for (int i = 0; i < nc; i++) {
        for (int j = i + 1; j < nc; j++) {
            bool fi = c[i].size <= limit, fj = c[j].size <= limit;
            bool swap;
            if (fi != fj) swap = fj;
            else if (fi) swap = c[j].size > c[i].size ||
                               (c[j].size == c[i].size && c[j].nat > c[i].nat);
            else swap = c[j].size < c[i].size;
            if (swap) { cand_t t = c[i]; c[i] = c[j]; c[j] = t; }
        }
    }
    /* Keep at most one entry per native size, so a lookup tries each
     * blob once. */
    slot->nchoice = 0;
    bool used[NATIVE_COUNT] = { false };
    for (int i = 0; i < nc && slot->nchoice < MAX_CHOICES; i++) {
        if (used[c[i].nat]) continue;
        used[c[i].nat] = true;
        slot->choice[slot->nchoice].native = (uint8_t)c[i].nat;
        slot->choice[slot->nchoice].scale = (uint8_t)c[i].k;
        slot->nchoice++;
    }
}

lv_font_t *cjk_font_create(int32_t line_height, int32_t base_line, int cell_w)
{
    load_blobs();
    if (s_slot_count >= SLOT_POOL) return NULL;
    cjk_slot_t *slot = &s_slots[s_slot_count];
    lv_font_t *f = &s_fonts[s_slot_count];
    s_slot_count++;

    slot->line_height = (int16_t)line_height;
    slot->base_line = (int16_t)base_line;
    slot->adv = (int16_t)(2 * cell_w);
    int limit = slot->adv < line_height ? slot->adv : (int)line_height;
    plan_choices(slot, limit);

    memset(f, 0, sizeof(*f));
    f->get_glyph_dsc = cjk_get_glyph_dsc;
    f->get_glyph_bitmap = cjk_get_glyph_bitmap;
    f->line_height = line_height;
    f->base_line = base_line;
    f->subpx = LV_FONT_SUBPX_NONE;
    f->underline_position = -1;
    f->underline_thickness = 1;
    f->dsc = slot;
    return f;
}
