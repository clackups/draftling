#pragma once

/*
 * Greybeard fonts for LVGL.
 * Generated from Greybeard TTF v1.0.0 via lv_font_conv.
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
 *   greybeard_cyrillic_NN -- Cyrillic block (0x0400-0x04FF) + the
 *                            Hryvnia sign (0x20B4), gated on
 *                            CONFIG_KB_LAYOUT_ENABLE_UA.
 *   greybeard_hebrew_NN   -- Hebrew block (0x0590-0x05FF), gated on
 *                            CONFIG_KB_LAYOUT_ENABLE_HE.
 *
 * The base font's lv_font_t.fallback pointer is chained at runtime
 * by greybeard_init() so the right subset font(s) participate in the
 * LVGL glyph lookup. Callers should invoke greybeard_init() once
 * during UI startup (after lv_init(), before any text is rendered).
 *
 * Greybeard is a monospaced bitmap font (vector port of UW ttyp0).
 * Each TTF file is designed for a single native pixel size, so
 * every size below renders pixel-perfect at 1 bpp with no scaling.
 *
 * Native sizes and metrics (advance widths in 1/16-px units):
 *   11 px: adv_w  96 -> char width  6, line_height 11
 *   14 px: adv_w 112 -> char width  7, line_height 13
 *   16 px: adv_w 128 -> char width  8, line_height 15
 *   18 px: adv_w 144 -> char width  9, line_height 17
 *   22 px: adv_w 176 -> char width 11, line_height 21
 *   26 px: adv_w 208 -> char width 13, line_height 25  (scaled from 22 px TTF)
 *
 * License: MIT
 * https://github.com/flowchartsman/greybeard
 */

#include "lvgl.h"
#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_font_t greybeard_11;
extern const lv_font_t greybeard_14;
extern const lv_font_t greybeard_16;
extern const lv_font_t greybeard_18;
extern const lv_font_t greybeard_22;
extern const lv_font_t greybeard_26;

#ifdef CONFIG_KB_LAYOUT_ENABLE_UA
extern const lv_font_t greybeard_cyrillic_11;
extern const lv_font_t greybeard_cyrillic_14;
extern const lv_font_t greybeard_cyrillic_16;
extern const lv_font_t greybeard_cyrillic_18;
extern const lv_font_t greybeard_cyrillic_22;
extern const lv_font_t greybeard_cyrillic_26;
#endif

#ifdef CONFIG_KB_LAYOUT_ENABLE_HE
extern const lv_font_t greybeard_hebrew_11;
extern const lv_font_t greybeard_hebrew_14;
extern const lv_font_t greybeard_hebrew_16;
extern const lv_font_t greybeard_hebrew_18;
extern const lv_font_t greybeard_hebrew_22;
extern const lv_font_t greybeard_hebrew_26;
#endif

/* Wire up the runtime fallback chain so the base fonts pick up
 * Cyrillic and/or Hebrew coverage when those layouts are enabled.
 * Safe to call once after lv_init(). */
void greybeard_init(void);

#ifdef __cplusplus
}
#endif
