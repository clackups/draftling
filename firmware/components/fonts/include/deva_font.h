#pragma once

/*
 * Devanagari for LVGL (Indic language pack).
 *
 * Devanagari needs real shaping -- conjuncts and half forms, the reph,
 * the i-matra drawn before the consonant cluster, marks placed per
 * base glyph -- so a run of Devanagari text is shaped by HarfBuzz
 * (components/harfbuzz) with the layout tables of Noto Sans Devanagari
 * (deva/deva_layout.ttf), and every resulting glyph is turned into a
 * private-use code point that this font draws from pre-rendered 1 bpp
 * bitmaps (tools/gen_deva_font.py, deva/LICENSE.txt):
 *
 *   U+F8000 + gid * 32 + d       a spacing glyph; its advance is the
 *                                font's plus (d - 16) * 10 font units
 *                                (HarfBuzz's kerning)
 *   U+108000 + gid               a mark (no advance) at the pen
 *   U+100000 | x << 7 | y,       a mark moved by x (8-bit signed, 4 font
 *     then U+10C000 + gid        units) and y (7-bit signed, 8 font units):
 *                                the first code draws the mark -- LVGL
 *                                passes it the next code point -- and the
 *                                second draws nothing
 *
 * The editor shapes each line it shows (deva_shape()); text shown
 * elsewhere (titles, file names) is drawn unshaped from the nominal
 * glyphs, which keeps it readable.
 *
 * Compiled only with CONFIG_DRAFTLING_HAS_DEVANAGARI.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEVA_PUA_SPACING   0xF8000u
#define DEVA_PUA_OFFSET    0x100000u
#define DEVA_PUA_MARK      0x108000u
#define DEVA_PUA_MARK_DONE 0x10C000u

/* Create the Devanagari font for the font slot with this line height
 * and monospace cell width in pixels. The returned font is static; its
 * .fallback starts out NULL. Returns NULL when there is no Devanagari
 * glyph set for that size. */
lv_font_t *deva_font_create(int32_t line_height, int cell_w);

/* True for a code point that belongs in a Devanagari run handed to
 * deva_shape(): the Devanagari and Devanagari Extended blocks and the
 * zero-width (non-)joiner. */
bool deva_in_run(uint32_t cp);

/* One shaped glyph: the private-use code point to draw and the byte
 * offset (in the text passed to deva_shape()) of the characters it
 * belongs to. Offsets never decrease along the output. */
typedef struct {
    uint32_t code;
    uint32_t cluster;
} deva_glyph_t;

/* Shape the UTF-8 run s[0..len) (deva_in_run() characters). Writes up
 * to max glyphs to out and returns their number (0 when shaping is
 * unavailable or out is too small). */
size_t deva_shape(const char *s, size_t len, deva_glyph_t *out, size_t max);

/* Monospace cells the code point takes at most (an upper bound for
 * width estimates; marks take none), or -1 when it is not Devanagari. */
int deva_cp_cells(uint32_t cp);

/* True when a cursor between prev and next would sit inside one
 * syllable: before a vowel sign or other mark, or between a virama
 * (or joiner) and the consonant it joins. */
bool deva_mid_cluster(uint32_t prev, uint32_t next);

#ifdef __cplusplus
}
#endif
