/*
 * Hi-res font set: runtime fallback router (Hack typeface).
 *
 * Mirrors lowres.c: the base Hack fonts (hires_NN.c) cover only
 * Latin, Latin-1 Supplement, the Euro sign and the Numero sign.
 * Coverage for Cyrillic (hires_cyrillic_NN.c) and Hebrew
 * (hires_hebrew_NN.c) lives in optional subset font files that are
 * compiled in only when the corresponding keyboard layout is enabled
 * in Kconfig. This keeps the firmware small for users who do not need
 * those scripts.
 *
 * Each base font is generated with `--lv-fallback hires_NN_ext`, so a
 * glyph lookup that misses the base coverage automatically walks into
 * the runtime-mutable router struct defined below. hires_init()
 * configures the .fallback pointer of each router at boot to chain in
 * whichever subset fonts are present, in this order:
 *
 *   base --> hires_NN_ext (router)
 *        --> hires_hebrew_NN   --> hires_NN_he_next (router)   [Hebrew]
 *        --> hires_latina_NN   --> hires_NN_la_next (router)   [Latin Extended-A]
 *        --> hires_latinx_NN   --> hires_NN_lx_next (router)   [Vietnamese / CJK]
 *        --> CJK runtime font (cjk_font.c)                  [Korean / Japanese / Chinese]
 *        --> Tamil runtime font (tamil_font.c)              [Tamil]
 *        --> hires_cyrillic_NN                                [Ukrainian]
 *
 * Subsets that are not compiled in are skipped; the Cyrillic subset is
 * generated without a fallback, so it always ends the chain. The
 * Latin-extension subset (Vietnamese letters, curly quotes, dashes)
 * is compiled in with the Vietnamese layout or the CJK fonts; the
 * Latin Extended-A subset (Polish, Czech, Turkish, ... letters) with
 * any layout that needs it (CONFIG_DRAFTLING_HAS_LATIN_EXT_A).
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
#include "hires.h"
#ifdef CONFIG_DRAFTLING_HAS_CJK
#include "cjk_font.h"
#endif
#ifdef CONFIG_DRAFTLING_HAS_TAMIL
#include "tamil_font.h"
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
ROUTER(hires_9_ext,  19, 4);
ROUTER(hires_11_ext, 23, 5);
ROUTER(hires_14_ext, 26, 6);
ROUTER(hires_16_ext, 30, 6);
ROUTER(hires_18_ext, 34, 7);
ROUTER(hires_22_ext, 41, 8);
ROUTER(hires_26_ext, 50, 10);
ROUTER(hires_30_ext, 58, 12);

/* Routers chained after the Hebrew and Latin-extension subsets so they
 * can hand off to the next font in the chain. Defined unconditionally
 * because the subset .c files reference these symbols. */
ROUTER(hires_9_he_next, 19, 4);
ROUTER(hires_11_he_next, 23, 5);
ROUTER(hires_14_he_next, 26, 6);
ROUTER(hires_16_he_next, 30, 6);
ROUTER(hires_18_he_next, 34, 7);
ROUTER(hires_22_he_next, 41, 8);
ROUTER(hires_26_he_next, 50, 10);
ROUTER(hires_30_he_next, 58, 12);

ROUTER(hires_9_lx_next, 19, 4);
ROUTER(hires_11_lx_next, 23, 5);
ROUTER(hires_14_lx_next, 26, 6);
ROUTER(hires_16_lx_next, 30, 6);
ROUTER(hires_18_lx_next, 34, 7);
ROUTER(hires_22_lx_next, 41, 8);
ROUTER(hires_26_lx_next, 50, 10);
ROUTER(hires_30_lx_next, 58, 12);

ROUTER(hires_9_la_next, 19, 4);
ROUTER(hires_11_la_next, 23, 5);
ROUTER(hires_14_la_next, 26, 6);
ROUTER(hires_16_la_next, 30, 6);
ROUTER(hires_18_la_next, 34, 7);
ROUTER(hires_22_la_next, 41, 8);
ROUTER(hires_26_la_next, 50, 10);
ROUTER(hires_30_la_next, 58, 12);

#ifdef CONFIG_KB_LAYOUT_ENABLE_UA
extern const lv_font_t hires_cyrillic_9;
extern const lv_font_t hires_cyrillic_11;
extern const lv_font_t hires_cyrillic_14;
extern const lv_font_t hires_cyrillic_16;
extern const lv_font_t hires_cyrillic_18;
extern const lv_font_t hires_cyrillic_22;
extern const lv_font_t hires_cyrillic_26;
extern const lv_font_t hires_cyrillic_30;
#endif

#ifdef CONFIG_KB_LAYOUT_ENABLE_HE
extern const lv_font_t hires_hebrew_9;
extern const lv_font_t hires_hebrew_11;
extern const lv_font_t hires_hebrew_14;
extern const lv_font_t hires_hebrew_16;
extern const lv_font_t hires_hebrew_18;
extern const lv_font_t hires_hebrew_22;
extern const lv_font_t hires_hebrew_26;
extern const lv_font_t hires_hebrew_30;
#endif

#ifdef CONFIG_DRAFTLING_HAS_LATIN_EXT_A
extern const lv_font_t hires_latina_9;
extern const lv_font_t hires_latina_11;
extern const lv_font_t hires_latina_14;
extern const lv_font_t hires_latina_16;
extern const lv_font_t hires_latina_18;
extern const lv_font_t hires_latina_22;
extern const lv_font_t hires_latina_26;
extern const lv_font_t hires_latina_30;
#endif

#ifdef HAS_LATINX
extern const lv_font_t hires_latinx_9;
extern const lv_font_t hires_latinx_11;
extern const lv_font_t hires_latinx_14;
extern const lv_font_t hires_latinx_16;
extern const lv_font_t hires_latinx_18;
extern const lv_font_t hires_latinx_22;
extern const lv_font_t hires_latinx_26;
extern const lv_font_t hires_latinx_30;
#endif

#ifdef CONFIG_DRAFTLING_HAS_TAMIL
/* Copies of the base fonts with taller lines (TAMIL_LINE_EXTRA), made
 * by hires_init(): the Tamil pack renders with them so that the
 * Tamil letters, whose loops and hooks need much more room above and
 * below their body than Latin letters, can be drawn larger. */
lv_font_t hires_9_tall;
lv_font_t hires_11_tall;
lv_font_t hires_14_tall;
lv_font_t hires_16_tall;
lv_font_t hires_18_tall;
lv_font_t hires_22_tall;
lv_font_t hires_26_tall;
lv_font_t hires_30_tall;
#define TALL(n) &hires_##n##_tall
#else
#define TALL(n) NULL
#endif

typedef struct {
    const lv_font_t *base;
    lv_font_t *ext;
    lv_font_t *he_next;
    lv_font_t *la_next;
    lv_font_t *lx_next;
    const lv_font_t *hebrew;
    const lv_font_t *latina;
    const lv_font_t *latinx;
    const lv_font_t *cyrillic;
    lv_font_t *tall;            /* Tamil pack: the base with taller lines */
} slot_chain_t;

#ifdef CONFIG_KB_LAYOUT_ENABLE_HE
#define HE(n) &hires_hebrew_##n
#else
#define HE(n) NULL
#endif
#ifdef CONFIG_DRAFTLING_HAS_LATIN_EXT_A
#define LA(n) &hires_latina_##n
#else
#define LA(n) NULL
#endif
#ifdef HAS_LATINX
#define LX(n) &hires_latinx_##n
#else
#define LX(n) NULL
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_UA
#define CY(n) &hires_cyrillic_##n
#else
#define CY(n) NULL
#endif

#define SLOT(n) { &hires_##n, &hires_##n##_ext, &hires_##n##_he_next, &hires_##n##_la_next, \
                  &hires_##n##_lx_next, HE(n), LA(n), LX(n), CY(n), TALL(n) }

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

void hires_init(void)
{
    for (size_t i = 0; i < sizeof(s_slots) / sizeof(s_slots[0]); i++) {
        const slot_chain_t *c = &s_slots[i];
        /* `tail` is the router (or runtime font) whose .fallback
         * receives the next subset in the chain. */
        lv_font_t *tail = c->ext;
        if (c->hebrew) { tail->fallback = c->hebrew; tail = c->he_next; }
        if (c->latina) { tail->fallback = c->latina; tail = c->la_next; }
        if (c->latinx) { tail->fallback = c->latinx; tail = c->lx_next; }
#ifdef CONFIG_DRAFTLING_HAS_CJK
        lv_font_t *cjk = cjk_font_create(c->base->line_height,
                                         c->base->base_line,
                                         lv_font_get_glyph_width(c->base, 'M', 0));
        if (cjk) { tail->fallback = cjk; tail = cjk; }
#endif
#ifdef CONFIG_DRAFTLING_HAS_TAMIL
        /* The tall copy: the extra height goes partly below the
         * baseline, partly above the Latin letters. */
        int32_t lh = c->base->line_height;
        *c->tall = *c->base;
        c->tall->line_height = lh + TAMIL_LINE_EXTRA(lh);
        c->tall->base_line = c->base->base_line + TAMIL_BASE_EXTRA(lh);
        c->tall->user_data = (void *)c->base;   /* for the editor's decorations */
        lv_font_t *ta = tamil_font_create(c->tall->line_height,
                                          lv_font_get_glyph_width(c->base, 'M', 0));
        if (ta) { tail->fallback = ta; tail = ta; }
#endif
        /* The Cyrillic subsets are generated without a fallback, so
         * they end the chain. */
        if (c->cyrillic) tail->fallback = c->cyrillic;
    }
}
