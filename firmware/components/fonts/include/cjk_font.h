#pragma once

/*
 * CJK pixel fonts for LVGL (Korean, Japanese and Chinese language packs).
 *
 * Three native pixel sizes are embedded as compact blobs (see
 * cjk/LICENSE.txt and tools/gen_cjk_fonts.py): Fusion Pixel Font 10 px
 * and 12 px and GNU Unifont 16 px. They hold only the full-width glyphs
 * of the language pack's language (kana and JIS kanji, GB 2312 hanzi,
 * or Hangul) plus CJK punctuation and fullwidth forms. A runtime
 * lv_font_t is created for every Greybeard / Hack slot: it reports each
 * glyph two monospace cells wide and draws the native bitmap scaled by
 * a whole number (nearest neighbour, so pixels stay crisp -- the same
 * 2x technique the Hack Hebrew subset uses), centred in the slot's
 * line box.
 *
 * Compiled only with CONFIG_DRAFTLING_HAS_CJK.
 */

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Create the CJK font for a slot. line_height / base_line are the base
 * font's metrics, cell_w its monospace advance in pixels. The returned
 * font is static (never freed); its .fallback starts out NULL and can
 * be chained further. Returns NULL when the pool is exhausted. */
lv_font_t *cjk_font_create(int32_t line_height, int32_t base_line, int cell_w);

/* True for code points drawn two cells wide by the CJK fonts (used by
 * width estimates in the editor). Does not check glyph availability. */
bool cjk_is_wide(uint32_t cp);

#ifdef __cplusplus
}
#endif
