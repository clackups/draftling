#pragma once

/*
 * Tamil fonts for LVGL (Tamil language pack).
 *
 * LVGL does no text shaping, but Tamil needs it: a vowel sign after a
 * consonant is drawn before it (e.g. U+0BC6), around it (U+0BCA) or
 * fused with it (U+0B95 U+0BC1). LVGL passes the next code point to
 * a font, so the Tamil font draws a consonant together with the mark
 * that follows it as one pre-shaped glyph (rendered from Noto Sans
 * Tamil by tools/gen_tamil_font.py, see tamil/LICENSE.txt) and the
 * mark itself as an empty, zero-width glyph. A mark without a
 * consonant before it cannot be drawn that way (the font does not see
 * the previous code point); the editor puts a dotted circle (U+25CC)
 * in front of such a mark on screen, and the font has the dotted
 * circle as a base too.
 *
 * The glyphs keep the font's proportional advance widths, so the
 * syllables of a word join up. tamil_cp_cells() gives an upper bound
 * in monospace cells for width estimates.
 *
 * Two ligatures span more code points than the font can see: k-ssa
 * (U+0B95 U+0BCD U+0BB7) and sri (U+0BB8 or U+0BB6, U+0BCD U+0BB0
 * U+0BC0). tamil_ligature() finds them; the editor shows each as one
 * private-use code point (TAMIL_LIG_*), which the font draws as the
 * ligature (k-ssa also joins a vowel sign after it).
 *
 * Compiled only with CONFIG_DRAFTLING_HAS_TAMIL.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Dotted circle: the base the editor shows a lone vowel sign on. */
#define TAMIL_DOTTED_CIRCLE 0x25CCu

/* Private-use code points the editor shows for the ligatures. */
#define TAMIL_LIG_KSSA 0xE001u
#define TAMIL_LIG_SRI  0xE002u

/* Create the Tamil font for the base font slot with this line height
 * and monospace cell width in pixels (each Greybeard / Hack slot has
 * its own). The returned font is
 * static; its .fallback starts out NULL and can be chained further.
 * Returns NULL when no Tamil glyphs exist for that size. */
lv_font_t *tamil_font_create(int32_t line_height, int cell_w);

/* If the UTF-8 text s[0..len) starts with a ligature sequence, return
 * its length in bytes and set *lig to its TAMIL_LIG_* code point;
 * otherwise return 0. */
size_t tamil_ligature(const char *s, size_t len, uint32_t *lig);

/* True for a consonant (or the dotted circle, or the k-ssa ligature)
 * that joins a following mark into one glyph. */
bool tamil_is_base(uint32_t cp);

/* True for a mark (vowel sign, pulli, ...) that joins the preceding
 * base. */
bool tamil_is_mark(uint32_t cp);

/* Monospace cells cp between prev and next takes at most (0 = none):
 * a base followed by a mark takes the cells of the joined glyph, a
 * mark after a base takes none, a lone mark the cells of the mark on
 * a dotted circle (as the editor shows it). An upper bound for width
 * estimates: the glyphs are proportional. Returns -1 when cp is not a
 * Tamil character (or the dotted circle). */
int tamil_cp_cells(uint32_t prev, uint32_t cp, uint32_t next);

/* The code point to draw for cp followed by next with
 * lv_draw_character(), which has no look-ahead: a private-use code
 * point standing for the joined glyph when cp is a base followed by a
 * mark, else cp itself. */
uint32_t tamil_draw_cp(uint32_t cp, uint32_t next);

#ifdef __cplusplus
}
#endif
