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
 * whichever subset fonts are present, in this order:
 *
 *   base --> greybeard_NN_ext (router)
 *        --> greybeard_hebrew_NN   --> greybeard_NN_he_next (router)   [Hebrew]
 *        --> greybeard_latinx_NN   --> greybeard_NN_lx_next (router)   [Vietnamese / CJK]
 *        --> CJK runtime font (cjk_font.c)                  [Korean / Japanese / Chinese]
 *        --> greybeard_cyrillic_NN                                [Ukrainian]
 *
 * Subsets that are not compiled in are skipped; the Cyrillic subset is
 * generated without a fallback, so it always ends the chain. The
 * Latin-extension subset (Vietnamese letters, curly quotes, dashes)
 * is compiled in with the Vietnamese layout or the CJK fonts.
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
#ifdef CONFIG_DRAFTLING_HAS_CJK
#include "cjk_font.h"
#endif

#if defined(CONFIG_KB_LAYOUT_ENABLE_VI) || defined(CONFIG_DRAFTLING_HAS_CJK)
#define HAS_LATINX 1
#endif

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
ROUTER(greybeard_30_ext, 30, 6);
ROUTER(greybeard_34_ext, 34, 6);

/* Routers chained after the Hebrew and Latin-extension subsets so they
 * can hand off to the next font in the chain. Defined unconditionally
 * because the subset .c files reference these symbols. */
ROUTER(greybeard_11_he_next, 11, 2);
ROUTER(greybeard_14_he_next, 13, 2);
ROUTER(greybeard_16_he_next, 15, 3);
ROUTER(greybeard_18_he_next, 17, 3);
ROUTER(greybeard_22_he_next, 21, 4);
ROUTER(greybeard_26_he_next, 25, 5);
ROUTER(greybeard_30_he_next, 30, 6);
ROUTER(greybeard_34_he_next, 34, 6);

ROUTER(greybeard_11_lx_next, 11, 2);
ROUTER(greybeard_14_lx_next, 13, 2);
ROUTER(greybeard_16_lx_next, 15, 3);
ROUTER(greybeard_18_lx_next, 17, 3);
ROUTER(greybeard_22_lx_next, 21, 4);
ROUTER(greybeard_26_lx_next, 25, 5);
ROUTER(greybeard_30_lx_next, 30, 6);
ROUTER(greybeard_34_lx_next, 34, 6);

#ifdef CONFIG_KB_LAYOUT_ENABLE_UA
extern const lv_font_t greybeard_cyrillic_11;
extern const lv_font_t greybeard_cyrillic_14;
extern const lv_font_t greybeard_cyrillic_16;
extern const lv_font_t greybeard_cyrillic_18;
extern const lv_font_t greybeard_cyrillic_22;
extern const lv_font_t greybeard_cyrillic_26;
extern const lv_font_t greybeard_cyrillic_30;
extern const lv_font_t greybeard_cyrillic_34;
#endif

#ifdef CONFIG_KB_LAYOUT_ENABLE_HE
extern const lv_font_t greybeard_hebrew_11;
extern const lv_font_t greybeard_hebrew_14;
extern const lv_font_t greybeard_hebrew_16;
extern const lv_font_t greybeard_hebrew_18;
extern const lv_font_t greybeard_hebrew_22;
extern const lv_font_t greybeard_hebrew_26;
extern const lv_font_t greybeard_hebrew_30;
extern const lv_font_t greybeard_hebrew_34;
#endif

#ifdef HAS_LATINX
extern const lv_font_t greybeard_latinx_11;
extern const lv_font_t greybeard_latinx_14;
extern const lv_font_t greybeard_latinx_16;
extern const lv_font_t greybeard_latinx_18;
extern const lv_font_t greybeard_latinx_22;
extern const lv_font_t greybeard_latinx_26;
extern const lv_font_t greybeard_latinx_30;
extern const lv_font_t greybeard_latinx_34;
#endif

typedef struct {
    const lv_font_t *base;
    lv_font_t *ext;
    lv_font_t *he_next;
    lv_font_t *lx_next;
    const lv_font_t *hebrew;
    const lv_font_t *latinx;
    const lv_font_t *cyrillic;
} slot_chain_t;

#ifdef CONFIG_KB_LAYOUT_ENABLE_HE
#define HE(n) &greybeard_hebrew_##n
#else
#define HE(n) NULL
#endif
#ifdef HAS_LATINX
#define LX(n) &greybeard_latinx_##n
#else
#define LX(n) NULL
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_UA
#define CY(n) &greybeard_cyrillic_##n
#else
#define CY(n) NULL
#endif

#define SLOT(n) { &greybeard_##n, &greybeard_##n##_ext, &greybeard_##n##_he_next, &greybeard_##n##_lx_next, HE(n), LX(n), CY(n) }

static const slot_chain_t s_slots[] = {
    SLOT(11),
    SLOT(14),
    SLOT(16),
    SLOT(18),
    SLOT(22),
    SLOT(26),
    SLOT(30),
    SLOT(34),
};

void greybeard_init(void)
{
    for (size_t i = 0; i < sizeof(s_slots) / sizeof(s_slots[0]); i++) {
        const slot_chain_t *c = &s_slots[i];
        /* `tail` is the router (or runtime font) whose .fallback
         * receives the next subset in the chain. */
        lv_font_t *tail = c->ext;
        if (c->hebrew) { tail->fallback = c->hebrew; tail = c->he_next; }
        if (c->latinx) { tail->fallback = c->latinx; tail = c->lx_next; }
#ifdef CONFIG_DRAFTLING_HAS_CJK
        lv_font_t *cjk = cjk_font_create(c->base->line_height,
                                         c->base->base_line,
                                         lv_font_get_glyph_width(c->base, 'M', 0));
        if (cjk) { tail->fallback = cjk; tail = cjk; }
#endif
        /* The Cyrillic subsets are generated without a fallback, so
         * they end the chain. */
        if (c->cyrillic) tail->fallback = c->cyrillic;
    }
}
