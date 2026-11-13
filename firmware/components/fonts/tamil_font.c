/*
 * Tamil cluster-glyph font driver -- see include/tamil_font.h.
 *
 * Blob format (written by tools/gen_tamil_font.py, little endian):
 *   "DTAM", u8 version (2), u8 line_height, u8 base_line, u8 cell_w,
 *   u32 count, count entries of 16 bytes sorted by key:
 *     u32 key (base << 16 | mark, mark 0 = none), u32 bitmap offset,
 *     u8 box_w, u8 box_h, i8 ofs_x, i8 ofs_y, u8 adv_w (pixels),
 *     3 pad bytes;
 *   then the bitmaps, 1 bpp, rows packed MSB-first.
 * One blob per base-font slot; the build embeds the Greybeard or the
 * Hack set (CONFIG_DRAFTLING_DISPLAY_HIDPI).
 */

#include <string.h>
#include "sdkconfig.h"
#include "lvgl.h"
#include "tamil_font.h"
#include "tamil/tamil_cells.h"

/* ---- Embedded blobs (EMBED_FILES in CMakeLists.txt) ---- */

#ifdef CONFIG_DRAFTLING_DISPLAY_HIDPI
extern const uint8_t _binary_tamil_hack_9_bin_start[];
extern const uint8_t _binary_tamil_hack_11_bin_start[];
extern const uint8_t _binary_tamil_hack_14_bin_start[];
extern const uint8_t _binary_tamil_hack_16_bin_start[];
extern const uint8_t _binary_tamil_hack_18_bin_start[];
extern const uint8_t _binary_tamil_hack_22_bin_start[];
extern const uint8_t _binary_tamil_hack_26_bin_start[];
extern const uint8_t _binary_tamil_hack_30_bin_start[];
static const uint8_t *const s_blobs[] = {
    _binary_tamil_hack_9_bin_start,  _binary_tamil_hack_11_bin_start,
    _binary_tamil_hack_14_bin_start, _binary_tamil_hack_16_bin_start,
    _binary_tamil_hack_18_bin_start, _binary_tamil_hack_22_bin_start,
    _binary_tamil_hack_26_bin_start, _binary_tamil_hack_30_bin_start,
};
#else
extern const uint8_t _binary_tamil_greybeard_11_bin_start[];
extern const uint8_t _binary_tamil_greybeard_14_bin_start[];
extern const uint8_t _binary_tamil_greybeard_16_bin_start[];
extern const uint8_t _binary_tamil_greybeard_18_bin_start[];
extern const uint8_t _binary_tamil_greybeard_22_bin_start[];
extern const uint8_t _binary_tamil_greybeard_26_bin_start[];
extern const uint8_t _binary_tamil_greybeard_30_bin_start[];
extern const uint8_t _binary_tamil_greybeard_34_bin_start[];
static const uint8_t *const s_blobs[] = {
    _binary_tamil_greybeard_11_bin_start, _binary_tamil_greybeard_14_bin_start,
    _binary_tamil_greybeard_16_bin_start, _binary_tamil_greybeard_18_bin_start,
    _binary_tamil_greybeard_22_bin_start, _binary_tamil_greybeard_26_bin_start,
    _binary_tamil_greybeard_30_bin_start, _binary_tamil_greybeard_34_bin_start,
};
#endif
#define BLOB_COUNT (sizeof(s_blobs) / sizeof(s_blobs[0]))

#define ENTRY_SIZE 16

typedef struct {
    const uint8_t *entries;
    const uint8_t *bitmaps;
    uint32_t count;
    uint8_t cell_w;
} tamil_blob_t;

static tamil_blob_t s_parsed[BLOB_COUNT];
static lv_font_t s_fonts[BLOB_COUNT];

static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* ---- Character classes ---- */

bool tamil_is_base(uint32_t cp)
{
    if (cp == TAMIL_DOTTED_CIRCLE || cp == TAMIL_LIG_KSSA) return true;
    if (cp < 0x0B95 || cp > 0x0BB9) return false;
    /* Unassigned code points in the consonant range. */
    switch (cp) {
    case 0x0B96: case 0x0B97: case 0x0B98: case 0x0B9B: case 0x0B9D:
    case 0x0BA0: case 0x0BA1: case 0x0BA2: case 0x0BA5: case 0x0BA6:
    case 0x0BA7: case 0x0BAB: case 0x0BAC: case 0x0BAD:
        return false;
    default:
        return true;
    }
}

bool tamil_is_mark(uint32_t cp)
{
    return cp == 0x0B82 || (cp >= 0x0BBE && cp <= 0x0BC2) ||
           (cp >= 0x0BC6 && cp <= 0x0BC8) || (cp >= 0x0BCA && cp <= 0x0BCD) ||
           cp == 0x0BD7;
}

/* ---- Ligatures ---- */

/* Decode one 3-byte UTF-8 code point (the Tamil block), or 0. */
static uint32_t dec3(const char *s, size_t len)
{
    const uint8_t *u = (const uint8_t *)s;
    if (len < 3 || (u[0] & 0xF0) != 0xE0 || (u[1] & 0xC0) != 0x80 ||
        (u[2] & 0xC0) != 0x80) return 0;
    return ((uint32_t)(u[0] & 0x0F) << 12) | ((uint32_t)(u[1] & 0x3F) << 6) |
           (u[2] & 0x3F);
}

size_t tamil_ligature(const char *s, size_t len, uint32_t *lig)
{
    uint32_t c0 = dec3(s, len);
    if (c0 != 0x0B95 && c0 != 0x0BB8 && c0 != 0x0BB6) return 0;
    if (dec3(s + 3, len - 3) != 0x0BCD) return 0;
    uint32_t c2 = dec3(s + 6, len - 6);
    if (c0 == 0x0B95 && c2 == 0x0BB7) {
        *lig = TAMIL_LIG_KSSA;
        return 9;
    }
    if (c0 != 0x0B95 && c2 == 0x0BB0 && dec3(s + 9, len - 9) == 0x0BC0) {
        *lig = TAMIL_LIG_SRI;
        return 12;
    }
    return 0;
}

/* ---- Cell counts (tamil_cells.h) ---- */

/* Cells of the glyph for key, or -1 when there is none. */
static int key_cells(uint32_t key)
{
    int32_t lo = 0;
    int32_t hi = (int32_t)(sizeof(TAMIL_CELL_KEYS) / sizeof(TAMIL_CELL_KEYS[0])) - 1;
    while (lo <= hi) {
        int32_t mid = (lo + hi) >> 1;
        uint32_t k = TAMIL_CELL_KEYS[mid];
        if (k == key) return TAMIL_CELL_COUNTS[mid];
        if (k < key) lo = mid + 1;
        else hi = mid - 1;
    }
    return -1;
}

/* Key of the glyph drawn for cp followed by next. */
static uint32_t glyph_key(uint32_t cp, uint32_t next)
{
    if (tamil_is_base(cp) && tamil_is_mark(next)) return (cp << 16) | next;
    return cp << 16;
}

int tamil_cp_cells(uint32_t prev, uint32_t cp, uint32_t next)
{
    if (tamil_is_mark(cp)) {
        if (tamil_is_base(prev)) return 0;
        int n = key_cells((TAMIL_DOTTED_CIRCLE << 16) | cp);
        return n < 0 ? 1 : n;
    }
    if (cp != TAMIL_DOTTED_CIRCLE && cp != TAMIL_LIG_KSSA && cp != TAMIL_LIG_SRI &&
        (cp < 0x0B80 || cp > 0x0BFF)) return -1;
    int n = key_cells(glyph_key(cp, next));
    if (n < 0) n = key_cells(cp << 16);
    return n < 0 ? 1 : n;
}

/* Private-use code points (plane 15) standing for a joined glyph:
 * 0xF0000 | base offset << 8 | mark offset, offsets from U+0B80
 * (0x7F for the dotted circle, 0x7E for the k-ssa ligature). */
#define PUA_BASE      0xF0000u
#define DC_OFFSET     0x7Fu
#define KSSA_OFFSET   0x7Eu

uint32_t tamil_draw_cp(uint32_t cp, uint32_t next)
{
    if (!tamil_is_base(cp) || !tamil_is_mark(next)) return cp;
    uint32_t b = cp == TAMIL_DOTTED_CIRCLE ? DC_OFFSET :
                 cp == TAMIL_LIG_KSSA ? KSSA_OFFSET : cp - 0x0B80;
    return PUA_BASE | (b << 8) | (next - 0x0B80);
}

/* ---- Font driver ---- */

static int32_t blob_find(const tamil_blob_t *b, uint32_t key)
{
    int32_t lo = 0, hi = (int32_t)b->count - 1;
    while (lo <= hi) {
        int32_t mid = (lo + hi) >> 1;
        uint32_t k = rd32(b->entries + (size_t)mid * ENTRY_SIZE);
        if (k == key) return mid;
        if (k < key) lo = mid + 1;
        else hi = mid - 1;
    }
    return -1;
}

static bool tamil_get_glyph_dsc(const lv_font_t *font, lv_font_glyph_dsc_t *dsc,
                                uint32_t letter, uint32_t letter_next)
{
    const tamil_blob_t *b = (const tamil_blob_t *)font->dsc;

    /* A mark is drawn by the base before it. */
    if (tamil_is_mark(letter)) {
        dsc->adv_w = 0;
        dsc->box_w = 0;
        dsc->box_h = 0;
        dsc->ofs_x = 0;
        dsc->ofs_y = 0;
        dsc->stride = 0;
        dsc->format = LV_FONT_GLYPH_FORMAT_A1;
        dsc->is_placeholder = 0;
        dsc->gid.index = 0;
        return true;
    }

    uint32_t key;
    if (letter >= PUA_BASE && letter < PUA_BASE + 0x10000) {
        uint32_t bo = (letter >> 8) & 0xFF;
        uint32_t base = bo == DC_OFFSET ? TAMIL_DOTTED_CIRCLE :
                        bo == KSSA_OFFSET ? TAMIL_LIG_KSSA : 0x0B80 + bo;
        key = (base << 16) | (0x0B80 + (letter & 0xFF));
    } else if (letter == TAMIL_DOTTED_CIRCLE || letter == TAMIL_LIG_KSSA ||
               letter == TAMIL_LIG_SRI || (letter >= 0x0B80 && letter <= 0x0BFF)) {
        key = glyph_key(letter, letter_next);
    } else {
        return false;
    }

    int32_t idx = blob_find(b, key);
    if (idx < 0 && (key & 0xFFFF)) idx = blob_find(b, key & 0xFFFF0000u);
    if (idx < 0) return false;

    const uint8_t *e = b->entries + (size_t)idx * ENTRY_SIZE;
    dsc->adv_w = e[12];
    dsc->box_w = e[8];
    dsc->box_h = e[9];
    dsc->ofs_x = (int8_t)e[10];
    dsc->ofs_y = (int8_t)e[11];
    dsc->stride = 0;
    dsc->format = LV_FONT_GLYPH_FORMAT_A1;
    dsc->is_placeholder = 0;
    dsc->gid.index = (uint32_t)idx;
    return true;
}

static const void *tamil_get_glyph_bitmap(lv_font_glyph_dsc_t *g, lv_draw_buf_t *draw_buf)
{
    if (!draw_buf || !g->resolved_font) return NULL;
    const tamil_blob_t *b = (const tamil_blob_t *)g->resolved_font->dsc;
    uint32_t idx = g->gid.index;
    if (idx >= b->count) return NULL;
    const uint8_t *e = b->entries + (size_t)idx * ENTRY_SIZE;
    int w = e[8], h = e[9];
    if (g->box_w != w || g->box_h != h) return NULL;
    const uint8_t *src = b->bitmaps + rd32(e + 4);

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

lv_font_t *tamil_font_create(int32_t line_height, int cell_w)
{
    for (size_t i = 0; i < BLOB_COUNT; i++) {
        const uint8_t *d = s_blobs[i];
        if (memcmp(d, "DTAM", 4) != 0 || d[4] != 2) continue;
        if (d[5] != line_height || d[7] != cell_w) continue;

        tamil_blob_t *b = &s_parsed[i];
        b->count = rd32(d + 8);
        b->cell_w = d[7];
        b->entries = d + 12;
        b->bitmaps = b->entries + (size_t)b->count * ENTRY_SIZE;

        lv_font_t *f = &s_fonts[i];
        memset(f, 0, sizeof(*f));
        f->get_glyph_dsc = tamil_get_glyph_dsc;
        f->get_glyph_bitmap = tamil_get_glyph_bitmap;
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
