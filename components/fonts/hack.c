/*
 * Hack font runtime fallback router.
 *
 * Mirrors greybeard.c: the base Hack fonts (hack_NN.c) cover only
 * Latin, Latin-1 Supplement, the Euro sign and the Numero sign.
 * Coverage for Cyrillic (hack_cyrillic_NN.c) and Hebrew
 * (hack_hebrew_NN.c) lives in optional subset font files that are
 * compiled in only when the corresponding keyboard layout is enabled
 * in Kconfig. This keeps the firmware small for users who do not need
 * those scripts.
 *
 * Each base font is generated with `--lv-fallback hack_NN_ext`, so a
 * glyph lookup that misses the base coverage automatically walks into
 * the runtime-mutable router struct defined below. hack_init()
 * configures the .fallback pointer of each router at boot to chain in
 * whichever subset fonts are present:
 *
 *   base   --> hack_NN_ext (router)
 *                |
 *                v (if Hebrew enabled)
 *              hack_hebrew_NN --> hack_NN_he_next (router)
 *                                   |
 *                                   v (if Ukrainian enabled)
 *                                 hack_cyrillic_NN
 *
 *   base   --> hack_NN_ext (router)
 *                |
 *                v (if Hebrew disabled, Ukrainian enabled)
 *              hack_cyrillic_NN
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
#include "hack.h"

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
ROUTER(hack_11_ext, 23, 5);
ROUTER(hack_14_ext, 26, 6);
ROUTER(hack_16_ext, 30, 6);
ROUTER(hack_18_ext, 34, 7);
ROUTER(hack_22_ext, 41, 8);
ROUTER(hack_26_ext, 50, 10);
ROUTER(hack_30_ext, 58, 12);

/* Routers chained after the Hebrew font so that Hebrew can hand off
 * to Cyrillic when both layouts are enabled. Defined unconditionally
 * because the Hebrew .c files reference these symbols. */
ROUTER(hack_11_he_next, 23, 5);
ROUTER(hack_14_he_next, 26, 6);
ROUTER(hack_16_he_next, 30, 6);
ROUTER(hack_18_he_next, 34, 7);
ROUTER(hack_22_he_next, 41, 8);
ROUTER(hack_26_he_next, 50, 10);
ROUTER(hack_30_he_next, 58, 12);

void hack_init(void)
{
#if defined(CONFIG_KB_LAYOUT_ENABLE_HE)
    /* Base -> Hebrew (-> Cyrillic if UA also enabled) */
    hack_11_ext.fallback = &hack_hebrew_11;
    hack_14_ext.fallback = &hack_hebrew_14;
    hack_16_ext.fallback = &hack_hebrew_16;
    hack_18_ext.fallback = &hack_hebrew_18;
    hack_22_ext.fallback = &hack_hebrew_22;
    hack_26_ext.fallback = &hack_hebrew_26;
    hack_30_ext.fallback = &hack_hebrew_30;
#  ifdef CONFIG_KB_LAYOUT_ENABLE_UA
    hack_11_he_next.fallback = &hack_cyrillic_11;
    hack_14_he_next.fallback = &hack_cyrillic_14;
    hack_16_he_next.fallback = &hack_cyrillic_16;
    hack_18_he_next.fallback = &hack_cyrillic_18;
    hack_22_he_next.fallback = &hack_cyrillic_22;
    hack_26_he_next.fallback = &hack_cyrillic_26;
    hack_30_he_next.fallback = &hack_cyrillic_30;
#  endif
#elif defined(CONFIG_KB_LAYOUT_ENABLE_UA)
    /* Base -> Cyrillic (no Hebrew) */
    hack_11_ext.fallback = &hack_cyrillic_11;
    hack_14_ext.fallback = &hack_cyrillic_14;
    hack_16_ext.fallback = &hack_cyrillic_16;
    hack_18_ext.fallback = &hack_cyrillic_18;
    hack_22_ext.fallback = &hack_cyrillic_22;
    hack_26_ext.fallback = &hack_cyrillic_26;
    hack_30_ext.fallback = &hack_cyrillic_30;
#endif
}
