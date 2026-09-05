/*
 * Greybeard font runtime fallback router.
 *
 * The base Greybeard fonts (greybeard_NN.c) cover only Latin, Latin-1
 * Supplement, the Euro sign and the Numero sign. Coverage for
 * Cyrillic (greybeard_cyrillic_NN.c) and Hebrew (greybeard_hebrew_NN.c)
 * lives in optional subset font files that are compiled in only when
 * the corresponding keyboard layout is enabled in Kconfig. This keeps
 * the firmware small for users who do not need those scripts.
 *
 * Each base font is generated with `--lv-fallback greybeard_NN_ext`,
 * so a glyph lookup that misses the base coverage automatically walks
 * into the runtime-mutable router struct defined below. greybeard_init()
 * configures the .fallback pointer of each router at boot to chain in
 * whichever subset fonts are present:
 *
 *   base   --> greybeard_NN_ext (router)
 *                |
 *                v (if Hebrew enabled)
 *              greybeard_hebrew_NN --> greybeard_NN_he_next (router)
 *                                        |
 *                                        v (if Ukrainian enabled)
 *                                      greybeard_cyrillic_NN
 *
 *   base   --> greybeard_NN_ext (router)
 *                |
 *                v (if Hebrew disabled, Ukrainian enabled)
 *              greybeard_cyrillic_NN
 *
 * If no subset font is enabled the router's fallback stays NULL and
 * LVGL's glyph lookup terminates after the base font with the usual
 * "glyph not found" path.
 *
 * The router itself reports no glyphs of its own -- its get_glyph_dsc
 * always returns false, which causes lv_font_get_glyph_dsc() to walk
 * straight through to the .fallback pointer.
 */

#include "lvgl.h"
#include "sdkconfig.h"
#include "greybeard.h"

/* Always-false glyph descriptor -- forces LVGL to walk to .fallback. */
static bool router_get_glyph_dsc(const lv_font_t *font,
                                 lv_font_glyph_dsc_t *dsc,
                                 uint32_t letter,
                                 uint32_t letter_next)
{
    LV_UNUSED(font);
    LV_UNUSED(dsc);
    LV_UNUSED(letter);
    LV_UNUSED(letter_next);
    return false;
}

/* Never invoked (router never reports a glyph), but LVGL asserts
 * the pointer is non-NULL on some paths. */
static const void *router_get_glyph_bitmap(lv_font_glyph_dsc_t *dsc,
                                           lv_draw_buf_t *draw_buf)
{
    LV_UNUSED(dsc);
    LV_UNUSED(draw_buf);
    return NULL;
}

#define ROUTER(name, lh, bl)                                  \
    lv_font_t name = {                                        \
        .get_glyph_dsc    = router_get_glyph_dsc,             \
        .get_glyph_bitmap = router_get_glyph_bitmap,          \
        .line_height      = (lh),                             \
        .base_line        = (bl),                             \
        .subpx            = LV_FONT_SUBPX_NONE,               \
        .underline_position  = -1,                            \
        .underline_thickness = 1,                             \
        .dsc              = NULL,                             \
        .fallback         = NULL,                             \
        .user_data        = NULL,                             \
    }

/* Routers for the base-font fallback slot (one per size).
 * line_height / base_line match the corresponding base font so that
 * LVGL's row geometry stays consistent if it ever queries the router
 * directly. */
ROUTER(greybeard_11_ext, 11, 2);
ROUTER(greybeard_14_ext, 13, 2);
ROUTER(greybeard_16_ext, 15, 3);
ROUTER(greybeard_18_ext, 17, 3);
ROUTER(greybeard_22_ext, 21, 4);
ROUTER(greybeard_26_ext, 25, 5);

/* Routers chained after the Hebrew font so that Hebrew can hand off
 * to Cyrillic when both layouts are enabled. Defined unconditionally
 * because the Hebrew .c files reference these symbols. */
ROUTER(greybeard_11_he_next, 11, 2);
ROUTER(greybeard_14_he_next, 13, 2);
ROUTER(greybeard_16_he_next, 15, 3);
ROUTER(greybeard_18_he_next, 17, 3);
ROUTER(greybeard_22_he_next, 21, 4);
ROUTER(greybeard_26_he_next, 25, 5);

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

void greybeard_init(void)
{
#if defined(CONFIG_KB_LAYOUT_ENABLE_HE)
    /* Base -> Hebrew (-> Cyrillic if UA also enabled) */
    greybeard_11_ext.fallback = &greybeard_hebrew_11;
    greybeard_14_ext.fallback = &greybeard_hebrew_14;
    greybeard_16_ext.fallback = &greybeard_hebrew_16;
    greybeard_18_ext.fallback = &greybeard_hebrew_18;
    greybeard_22_ext.fallback = &greybeard_hebrew_22;
    greybeard_26_ext.fallback = &greybeard_hebrew_26;
#  ifdef CONFIG_KB_LAYOUT_ENABLE_UA
    greybeard_11_he_next.fallback = &greybeard_cyrillic_11;
    greybeard_14_he_next.fallback = &greybeard_cyrillic_14;
    greybeard_16_he_next.fallback = &greybeard_cyrillic_16;
    greybeard_18_he_next.fallback = &greybeard_cyrillic_18;
    greybeard_22_he_next.fallback = &greybeard_cyrillic_22;
    greybeard_26_he_next.fallback = &greybeard_cyrillic_26;
#  endif
#elif defined(CONFIG_KB_LAYOUT_ENABLE_UA)
    /* Base -> Cyrillic (no Hebrew) */
    greybeard_11_ext.fallback = &greybeard_cyrillic_11;
    greybeard_14_ext.fallback = &greybeard_cyrillic_14;
    greybeard_16_ext.fallback = &greybeard_cyrillic_16;
    greybeard_18_ext.fallback = &greybeard_cyrillic_18;
    greybeard_22_ext.fallback = &greybeard_cyrillic_22;
    greybeard_26_ext.fallback = &greybeard_cyrillic_26;
#endif
}
