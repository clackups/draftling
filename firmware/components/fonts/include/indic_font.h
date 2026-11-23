#pragma once

/*
 * Shaped Indic scripts for LVGL (Indic language pack): Devanagari,
 * Bengali and Telugu.
 *
 * These scripts need real shaping -- conjuncts and half forms, the
 * reph, vowel signs drawn before the consonant cluster, marks placed
 * per base glyph -- so a run of their text is shaped by HarfBuzz
 * (components/harfbuzz) with the layout tables of the script's Noto
 * Sans font (deva/deva_layout.ttf, beng/beng_layout.ttf,
 * telu/telu_layout.ttf), and every resulting glyph is turned into a
 * private-use code point that this font draws from pre-rendered 1 bpp
 * bitmaps (tools/gen_indic_font.py, deva/LICENSE.txt, beng/LICENSE.txt,
 * telu/LICENSE.txt). Glyph ids are global: the Devanagari glyphs first,
 * then the Bengali ones, then the Telugu ones (at most 2048 in all).
 *
 *   U+100000 + gid * 32 + d      a spacing glyph; its advance is the
 *                                font's plus (d - 16) * 20 font units
 *                                (HarfBuzz's kerning)
 *   U+FC000 + gid                a mark (no advance) at the pen
 *   U+F8000 | x << 6 | y,        a mark moved by x (8-bit signed, 8 font
 *     then U+FC800 + gid         units) and y (6-bit signed, 10 font
 *                                units): the first code draws the mark --
 *                                LVGL passes it the next code point --
 *                                and the second draws nothing
 *
 * (U+F0000..U+F7FFF are the Tamil font's.) The editor shapes each line
 * it shows (indic_shape()); text shown elsewhere (titles, file names)
 * is drawn unshaped from the nominal glyphs, which keeps it readable.
 *
 * Compiled only with CONFIG_DRAFTLING_HAS_INDIC_SHAPING (Devanagari,
 * Bengali or Telugu layouts).
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INDIC_PUA_OFFSET    0xF8000u
#define INDIC_PUA_MARK      0xFC000u
#define INDIC_PUA_MARK_DONE 0xFC800u
#define INDIC_PUA_SPACING   0x100000u

/* Scripts of indic_script(). */
typedef enum {
    INDIC_NONE = 0,
    INDIC_DEVA,     /* Devanagari */
    INDIC_BENG,     /* Bengali (and Assamese) */
    INDIC_TELU,     /* Telugu */
    INDIC_ANY,      /* the joiners and dandas: part of a run of either */
} indic_script_t;

/* Create the font for the font slot with this line height and
 * monospace cell width in pixels. The returned font is static; its
 * .fallback starts out NULL. Returns NULL when there is no glyph set
 * for that size. */
lv_font_t *indic_font_create(int32_t line_height, int cell_w);

/* Which run of shaped text the code point belongs in: INDIC_DEVA for
 * the Devanagari and Devanagari Extended blocks, INDIC_BENG for the
 * Bengali block, INDIC_TELU for the Telugu block, INDIC_ANY for the zero-width (non-)joiner and the
 * dandas, INDIC_NONE otherwise or when the script is not compiled in.
 * A run is one script plus INDIC_ANY characters. */
indic_script_t indic_script(uint32_t cp);

/* One shaped glyph: the private-use code point to draw and the byte
 * offset (in the text passed to indic_shape()) of the characters it
 * belongs to. Offsets never decrease along the output. */
typedef struct {
    uint32_t code;
    uint32_t cluster;
} indic_glyph_t;

/* Shape the UTF-8 run s[0..len) of script (INDIC_ANY: a run of only
 * joiners / dandas, shaped as the first script compiled in). Writes up
 * to max glyphs to out and returns their number (0 when shaping is
 * unavailable or out is too small). */
size_t indic_shape(indic_script_t script, const char *s, size_t len,
                   indic_glyph_t *out, size_t max);

/* Monospace cells the code point takes at most (an upper bound for
 * width estimates; marks take none), or -1 when it is not one of the
 * shaped scripts. */
int indic_cp_cells(uint32_t cp);

/* True when a cursor between prev and next would sit inside one
 * syllable: before a vowel sign or other mark, or between a virama
 * (or joiner) and the consonant it joins. */
bool indic_mid_cluster(uint32_t prev, uint32_t next);

#ifdef __cplusplus
}
#endif
