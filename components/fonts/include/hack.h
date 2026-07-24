#pragma once

/*
 * Hack fonts for LVGL.
 * Generated from Hack v3.003 (Regular) via lv_font_conv, with the
 * Hebrew subset rendered from the Greybeard TTFs (Hack has no Hebrew
 * coverage). See components/fonts/AGENTS notes and AGENTS.md for the
 * exact command lines.
 *
 * Hack is used on the high-density boards that previously rendered
 * Greybeard through a 2x nearest-neighbor upscale (the boards that
 * used to set DRAFTLING_DISPLAY_SCALE = 2). Instead of scaling the
 * framebuffer, those boards now render 1:1 and pick a larger native
 * font whose row height approximately matches what the user saw with
 * Greybeard at 2x. The six sizes below mirror the six Greybeard
 * "slots" (11/14/16/18/22/26) so the editor can swap font families
 * without changing its slot logic; the number in each symbol is the
 * Greybeard slot it replaces, NOT the Hack pixel size.
 *
 * A seventh slot (30) is Hack-only: it has no Greybeard counterpart
 * and exists so the HIDPI-only 20 px base font size can render an H1
 * heading larger than its H2 (which uses slot 26). Its number follows
 * the Greybeard slot progression (11/14/16/18/22/26/30) even though no
 * Greybeard 30 px font is generated.
 *
 * Slot -> Hack pixel size -> line_height -> monospace cell width:
 *   hack_11: 19 px, line_height 23, cell 11
 *   hack_14: 21 px, line_height 26, cell 13
 *   hack_16: 25 px, line_height 30, cell 15
 *   hack_18: 28 px, line_height 34, cell 17
 *   hack_22: 34 px, line_height 41, cell 21
 *   hack_26: 41 px, line_height 50, cell 25
 *   hack_30: 47 px, line_height 58, cell 28
 *
 * The base fonts cover:
 *   0x0020-0x007F (Basic Latin)
 *   0x00A0-0x00FF (Latin-1 Supplement)
 *   0x20AC (Euro sign)
 *   0x2116 (Numero sign)
 *
 * Optional subset fonts add coverage for additional scripts and are
 * compiled into the firmware only when the corresponding keyboard
 * layout is enabled in Kconfig:
 *
 *   hack_cyrillic_NN -- Cyrillic block (0x0400-0x04FF) + the Hryvnia
 *                       sign (0x20B4), gated on CONFIG_KB_LAYOUT_ENABLE_UA.
 *   hack_hebrew_NN   -- Hebrew block (0x0590-0x05FF), rendered from the
 *                       Greybeard TTFs pixel-doubled to match the Hack
 *                       row height, gated on CONFIG_KB_LAYOUT_ENABLE_HE.
 *
 * The base font's lv_font_t.fallback pointer is chained at runtime by
 * hack_init() so the right subset font(s) participate in the LVGL
 * glyph lookup. Callers should invoke hack_init() once during UI
 * startup (after lv_init(), before any text is rendered).
 *
 * Hack license: MIT (https://github.com/source-foundry/Hack)
 * Greybeard license: MIT (https://github.com/flowchartsman/greybeard)
 */

#include "lvgl.h"
#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_font_t hack_11;
extern const lv_font_t hack_14;
extern const lv_font_t hack_16;
extern const lv_font_t hack_18;
extern const lv_font_t hack_22;
extern const lv_font_t hack_26;
extern const lv_font_t hack_30;

#ifdef CONFIG_KB_LAYOUT_ENABLE_UA
extern const lv_font_t hack_cyrillic_11;
extern const lv_font_t hack_cyrillic_14;
extern const lv_font_t hack_cyrillic_16;
extern const lv_font_t hack_cyrillic_18;
extern const lv_font_t hack_cyrillic_22;
extern const lv_font_t hack_cyrillic_26;
extern const lv_font_t hack_cyrillic_30;
#endif

#ifdef CONFIG_KB_LAYOUT_ENABLE_HE
extern const lv_font_t hack_hebrew_11;
extern const lv_font_t hack_hebrew_14;
extern const lv_font_t hack_hebrew_16;
extern const lv_font_t hack_hebrew_18;
extern const lv_font_t hack_hebrew_22;
extern const lv_font_t hack_hebrew_26;
extern const lv_font_t hack_hebrew_30;
#endif

/* Wire up the runtime fallback chain so the base fonts pick up
 * Cyrillic and/or Hebrew coverage when those layouts are enabled.
 * Safe to call once after lv_init(). */
void hack_init(void);

#ifdef __cplusplus
}
#endif
