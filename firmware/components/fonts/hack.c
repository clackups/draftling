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
 * whichever subset fonts are present, in this order:
 *
 *   base --> hack_NN_ext (router)
 *        --> hack_hebrew_NN   --> hack_NN_he_next (router)   [Hebrew]
 *        --> hack_latinx_NN   --> hack_NN_lx_next (router)   [Vietnamese / CJK]
 *        --> CJK runtime font (cjk_font.c)                  [Korean / Japanese / Chinese]
 *        --> hack_cyrillic_NN                                [Ukrainian]
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
#include "hack.h"
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
ROUTER(hack_9_ext,  19, 4);
ROUTER(hack_11_ext, 23, 5);
ROUTER(hack_14_ext, 26, 6);
ROUTER(hack_16_ext, 30, 6);
ROUTER(hack_18_ext, 34, 7);
ROUTER(hack_22_ext, 41, 8);
ROUTER(hack_26_ext, 50, 10);
ROUTER(hack_30_ext, 58, 12);

/* Routers chained after the Hebrew and Latin-extension subsets so they
 * can hand off to the next font in the chain. Defined unconditionally
 * because the subset .c files reference these symbols. */
ROUTER(hack_9_he_next, 19, 4);
ROUTER(hack_11_he_next, 23, 5);
ROUTER(hack_14_he_next, 26, 6);
ROUTER(hack_16_he_next, 30, 6);
ROUTER(hack_18_he_next, 34, 7);
ROUTER(hack_22_he_next, 41, 8);
ROUTER(hack_26_he_next, 50, 10);
ROUTER(hack_30_he_next, 58, 12);

ROUTER(hack_9_lx_next, 19, 4);
ROUTER(hack_11_lx_next, 23, 5);
ROUTER(hack_14_lx_next, 26, 6);
ROUTER(hack_16_lx_next, 30, 6);
ROUTER(hack_18_lx_next, 34, 7);
ROUTER(hack_22_lx_next, 41, 8);
ROUTER(hack_26_lx_next, 50, 10);
ROUTER(hack_30_lx_next, 58, 12);

#ifdef CONFIG_KB_LAYOUT_ENABLE_UA
extern const lv_font_t hack_cyrillic_9;
extern const lv_font_t hack_cyrillic_11;
extern const lv_font_t hack_cyrillic_14;
extern const lv_font_t hack_cyrillic_16;
extern const lv_font_t hack_cyrillic_18;
extern const lv_font_t hack_cyrillic_22;
extern const lv_font_t hack_cyrillic_26;
extern const lv_font_t hack_cyrillic_30;
#endif

#ifdef CONFIG_KB_LAYOUT_ENABLE_HE
extern const lv_font_t hack_hebrew_9;
extern const lv_font_t hack_hebrew_11;
extern const lv_font_t hack_hebrew_14;
extern const lv_font_t hack_hebrew_16;
extern const lv_font_t hack_hebrew_18;
extern const lv_font_t hack_hebrew_22;
extern const lv_font_t hack_hebrew_26;
extern const lv_font_t hack_hebrew_30;
#endif

#ifdef HAS_LATINX
extern const lv_font_t hack_latinx_9;
extern const lv_font_t hack_latinx_11;
extern const lv_font_t hack_latinx_14;
extern const lv_font_t hack_latinx_16;
extern const lv_font_t hack_latinx_18;
extern const lv_font_t hack_latinx_22;
extern const lv_font_t hack_latinx_26;
extern const lv_font_t hack_latinx_30;
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
#define HE(n) &hack_hebrew_##n
#else
#define HE(n) NULL
#endif
#ifdef HAS_LATINX
#define LX(n) &hack_latinx_##n
#else
#define LX(n) NULL
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_UA
#define CY(n) &hack_cyrillic_##n
#else
#define CY(n) NULL
#endif

#define SLOT(n) { &hack_##n, &hack_##n##_ext, &hack_##n##_he_next, &hack_##n##_lx_next, HE(n), LX(n), CY(n) }

static const slot_chain_t s_slots[] = {
    SLOT(9),
    SLOT(11),
    SLOT(14),
    SLOT(16),
    SLOT(18),
    SLOT(22),
    SLOT(26),
    SLOT(30),
};

void hack_init(void)
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
