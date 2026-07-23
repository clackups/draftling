/*******************************************************************************
 * Size: 22 px
 * Bpp: 1
 * Opts: --font /tmp/hacktest/gbttf/Greybeard-11px.ttf -r 0x590-0x5FF --size 22 --bpp 1 --format lvgl --no-compress --lv-fallback hack_11_he_next --lv-font-name hack_hebrew_11 -o /tmp/hacktest/gen/hack_hebrew_11.c
 ******************************************************************************/

#include "lvgl.h"

#ifndef HACK_HEBREW_11
#define HACK_HEBREW_11 1
#endif

#if HACK_HEBREW_11

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+05B0 "ְ" */
    0xf0, 0xf0,

    /* U+05B1 "ֱ" */
    0x0, 0xc0, 0x3c, 0xc3, 0x30, 0x30, 0xcc, 0x30,

    /* U+05B2 "ֲ" */
    0x0, 0xc0, 0x3f, 0xc3, 0xf0, 0x0, 0xc0, 0x30,

    /* U+05B3 "ֳ" */
    0x0, 0xc0, 0x3f, 0xc3, 0xf0, 0x30, 0xcc, 0x30,

    /* U+05B4 "ִ" */
    0xf0,

    /* U+05B5 "ֵ" */
    0xcf, 0x30,

    /* U+05B6 "ֶ" */
    0xc0, 0xf0, 0x30, 0xc0, 0x30,

    /* U+05B7 "ַ" */
    0xff, 0xff,

    /* U+05B8 "ָ" */
    0xff, 0xf3, 0xc,

    /* U+05B9 "ֹ" */
    0xff,

    /* U+05BB "ֻ" */
    0xc0, 0x30, 0x0, 0xc0, 0x30, 0x0, 0xc0, 0x30,

    /* U+05BC "ּ" */
    0xff,

    /* U+05BD "ֽ" */
    0xff, 0xf0,

    /* U+05BE "־" */
    0xff, 0xff,

    /* U+05BF "ֿ" */
    0xff, 0xf0,

    /* U+05C0 "׀" */
    0xff, 0xff, 0xff,

    /* U+05C1 "ׁ" */
    0xff,

    /* U+05C2 "ׂ" */
    0xff,

    /* U+05C3 "׃" */
    0xff, 0xff, 0x0, 0xff, 0xff,

    /* U+05C4 "ׄ" */
    0xff,

    /* U+05D0 "א" */
    0xcc, 0xcc, 0xc3, 0xc3, 0x3c, 0x3c, 0xcc, 0xcc,
    0xc3, 0xc3, 0xf3, 0xf3,

    /* U+05D1 "ב" */
    0xf0, 0xf0, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc,
    0xc, 0xc, 0xff, 0xff,

    /* U+05D2 "ג" */
    0xf0, 0xf0, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc,
    0x33, 0x33, 0xc3, 0xc3,

    /* U+05D3 "ד" */
    0xff, 0xff, 0xf0, 0x30, 0xc, 0x3, 0x0, 0xc0,
    0x30, 0xc, 0x3, 0x0, 0xc0, 0x30, 0xc,

    /* U+05D4 "ה" */
    0xff, 0xff, 0xf0, 0xc, 0x3, 0x30, 0xcc, 0x33,
    0xc, 0xc3, 0x30, 0xcc, 0x33, 0xc, 0xc3,

    /* U+05D5 "ו" */
    0xff, 0x33, 0x33, 0x33, 0x33, 0x33,

    /* U+05D6 "ז" */
    0xff, 0xff, 0xf0, 0xc0, 0x30, 0xc, 0x3, 0x0,
    0xc0, 0x30, 0xc, 0x3, 0x0, 0xc0, 0x30,

    /* U+05D7 "ח" */
    0xff, 0xff, 0xf3, 0xc, 0xc3, 0x30, 0xcc, 0x33,
    0xc, 0xc3, 0x30, 0xcc, 0x33, 0xc, 0xc3,

    /* U+05D8 "ט" */
    0xc3, 0xf0, 0xfc, 0xf, 0x3, 0xc0, 0xf0, 0x3c,
    0xf, 0x3, 0xc3, 0x30, 0xcf, 0xc3, 0xf0,

    /* U+05D9 "י" */
    0xff, 0x33, 0x33,

    /* U+05DA "ך" */
    0xff, 0xff, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
    0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,

    /* U+05DB "כ" */
    0xfc, 0xfc, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
    0x3, 0x3, 0xfc, 0xfc,

    /* U+05DC "ל" */
    0xc0, 0xc0, 0xc0, 0xc0, 0xff, 0xff, 0x3, 0x3,
    0x3, 0x3, 0x3, 0x3, 0xc, 0xc, 0x30, 0x30,

    /* U+05DD "ם" */
    0xff, 0xff, 0xf3, 0xc, 0xc3, 0x30, 0xcc, 0x33,
    0xc, 0xc3, 0x30, 0xcc, 0x33, 0xfc, 0xff,

    /* U+05DE "מ" */
    0xcf, 0x33, 0xc3, 0xcc, 0xf3, 0x30, 0xcc, 0x33,
    0xc, 0xc3, 0xc0, 0xf0, 0x3c, 0xff, 0x3f,

    /* U+05DF "ן" */
    0xff, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33,

    /* U+05E0 "נ" */
    0xf, 0xf, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
    0x3, 0x3, 0xff, 0xff,

    /* U+05E1 "ס" */
    0xff, 0xff, 0xf3, 0xc, 0xc3, 0x30, 0xcc, 0x33,
    0xc, 0xc3, 0x30, 0xcc, 0x30, 0xf0, 0x3c,

    /* U+05E2 "ע" */
    0xc3, 0xc3, 0xc3, 0xc3, 0x33, 0x33, 0x3c, 0x3c,
    0xc, 0xc, 0xf0, 0xf0,

    /* U+05E3 "ף" */
    0xff, 0x3f, 0xc3, 0xc, 0xc3, 0x3c, 0xcf, 0x30,
    0xc, 0x3, 0x0, 0xc0, 0x30, 0xc, 0x3, 0x0,
    0xc0, 0x30, 0xc, 0x3,

    /* U+05E4 "פ" */
    0xff, 0x3f, 0xc3, 0xc, 0xc3, 0x3c, 0xcf, 0x30,
    0xc, 0x3, 0x0, 0xc0, 0x33, 0xf0, 0xfc,

    /* U+05E5 "ץ" */
    0xc3, 0xc3, 0xc3, 0xc3, 0xcc, 0xcc, 0xf0, 0xf0,
    0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,

    /* U+05E6 "צ" */
    0xc3, 0xc3, 0xc3, 0xc3, 0x3c, 0x3c, 0xc, 0xc,
    0x3, 0x3, 0xff, 0xff,

    /* U+05E7 "ק" */
    0xfc, 0xfc, 0x3, 0x3, 0xc3, 0xc3, 0xcc, 0xcc,
    0xcc, 0xcc, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,

    /* U+05E8 "ר" */
    0xfc, 0xfc, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
    0x3, 0x3, 0x3, 0x3,

    /* U+05E9 "ש" */
    0xcc, 0xf3, 0x3c, 0xcf, 0x33, 0xcc, 0xf3, 0x3f,
    0xf, 0xc3, 0xc3, 0x30, 0xcf, 0xc3, 0xf0,

    /* U+05EA "ת" */
    0xff, 0x3f, 0xc3, 0xc, 0xc3, 0x30, 0xcc, 0x33,
    0xc, 0xc3, 0x30, 0xcc, 0x3f, 0xf, 0xc3,

    /* U+05F0 "װ" */
    0xf3, 0xfc, 0xf3, 0xc, 0xc3, 0x30, 0xcc, 0x33,
    0xc, 0xc3, 0x30, 0xcc, 0x33, 0xc, 0xc3,

    /* U+05F1 "ױ" */
    0xf3, 0xfc, 0xf3, 0xc, 0xc3, 0x30, 0xcc, 0x30,
    0xc, 0x3, 0x0, 0xc0, 0x30, 0xc, 0x3,

    /* U+05F2 "ײ" */
    0xf3, 0xfc, 0xf3, 0xc, 0xc3, 0x30, 0xcc, 0x30,

    /* U+05F3 "׳" */
    0xc, 0x33, 0xc, 0xc3, 0x0,

    /* U+05F4 "״" */
    0xc, 0xc3, 0x33, 0x30, 0xcc, 0xcc, 0x33, 0x0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 176, .box_w = 2, .box_h = 6, .ofs_x = 4, .ofs_y = -4},
    {.bitmap_index = 2, .adv_w = 176, .box_w = 10, .box_h = 6, .ofs_x = 0, .ofs_y = -4},
    {.bitmap_index = 10, .adv_w = 176, .box_w = 10, .box_h = 6, .ofs_x = 0, .ofs_y = -4},
    {.bitmap_index = 18, .adv_w = 176, .box_w = 10, .box_h = 6, .ofs_x = 0, .ofs_y = -4},
    {.bitmap_index = 26, .adv_w = 176, .box_w = 2, .box_h = 2, .ofs_x = 4, .ofs_y = -4},
    {.bitmap_index = 27, .adv_w = 176, .box_w = 6, .box_h = 2, .ofs_x = 2, .ofs_y = -4},
    {.bitmap_index = 29, .adv_w = 176, .box_w = 10, .box_h = 4, .ofs_x = 0, .ofs_y = -4},
    {.bitmap_index = 34, .adv_w = 176, .box_w = 8, .box_h = 2, .ofs_x = 2, .ofs_y = -4},
    {.bitmap_index = 36, .adv_w = 176, .box_w = 6, .box_h = 4, .ofs_x = 2, .ofs_y = -4},
    {.bitmap_index = 39, .adv_w = 176, .box_w = 2, .box_h = 4, .ofs_x = 2, .ofs_y = 14},
    {.bitmap_index = 40, .adv_w = 176, .box_w = 10, .box_h = 6, .ofs_x = 0, .ofs_y = -4},
    {.bitmap_index = 48, .adv_w = 176, .box_w = 2, .box_h = 4, .ofs_x = 2, .ofs_y = 4},
    {.bitmap_index = 49, .adv_w = 176, .box_w = 2, .box_h = 6, .ofs_x = 4, .ofs_y = -4},
    {.bitmap_index = 51, .adv_w = 176, .box_w = 8, .box_h = 2, .ofs_x = 2, .ofs_y = 10},
    {.bitmap_index = 53, .adv_w = 176, .box_w = 6, .box_h = 2, .ofs_x = 2, .ofs_y = 14},
    {.bitmap_index = 55, .adv_w = 176, .box_w = 2, .box_h = 12, .ofs_x = 6, .ofs_y = 0},
    {.bitmap_index = 58, .adv_w = 176, .box_w = 2, .box_h = 4, .ofs_x = 8, .ofs_y = 14},
    {.bitmap_index = 59, .adv_w = 176, .box_w = 2, .box_h = 4, .ofs_x = 0, .ofs_y = 14},
    {.bitmap_index = 60, .adv_w = 176, .box_w = 4, .box_h = 10, .ofs_x = 4, .ofs_y = 0},
    {.bitmap_index = 65, .adv_w = 176, .box_w = 2, .box_h = 4, .ofs_x = 4, .ofs_y = 14},
    {.bitmap_index = 66, .adv_w = 176, .box_w = 8, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 78, .adv_w = 176, .box_w = 8, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 90, .adv_w = 176, .box_w = 8, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 102, .adv_w = 176, .box_w = 10, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 117, .adv_w = 176, .box_w = 10, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 132, .adv_w = 176, .box_w = 4, .box_h = 12, .ofs_x = 4, .ofs_y = 0},
    {.bitmap_index = 138, .adv_w = 176, .box_w = 10, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 153, .adv_w = 176, .box_w = 10, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 168, .adv_w = 176, .box_w = 10, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 183, .adv_w = 176, .box_w = 4, .box_h = 6, .ofs_x = 4, .ofs_y = 6},
    {.bitmap_index = 186, .adv_w = 176, .box_w = 8, .box_h = 16, .ofs_x = 2, .ofs_y = -4},
    {.bitmap_index = 202, .adv_w = 176, .box_w = 8, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 214, .adv_w = 176, .box_w = 8, .box_h = 16, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 230, .adv_w = 176, .box_w = 10, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 245, .adv_w = 176, .box_w = 10, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 260, .adv_w = 176, .box_w = 4, .box_h = 16, .ofs_x = 4, .ofs_y = -4},
    {.bitmap_index = 268, .adv_w = 176, .box_w = 8, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 280, .adv_w = 176, .box_w = 10, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 295, .adv_w = 176, .box_w = 8, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 307, .adv_w = 176, .box_w = 10, .box_h = 16, .ofs_x = 0, .ofs_y = -4},
    {.bitmap_index = 327, .adv_w = 176, .box_w = 10, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 342, .adv_w = 176, .box_w = 8, .box_h = 16, .ofs_x = 2, .ofs_y = -4},
    {.bitmap_index = 358, .adv_w = 176, .box_w = 8, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 370, .adv_w = 176, .box_w = 8, .box_h = 16, .ofs_x = 2, .ofs_y = -4},
    {.bitmap_index = 386, .adv_w = 176, .box_w = 8, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 398, .adv_w = 176, .box_w = 10, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 413, .adv_w = 176, .box_w = 10, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 428, .adv_w = 176, .box_w = 10, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 443, .adv_w = 176, .box_w = 10, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 458, .adv_w = 176, .box_w = 10, .box_h = 6, .ofs_x = 0, .ofs_y = 6},
    {.bitmap_index = 466, .adv_w = 176, .box_w = 6, .box_h = 6, .ofs_x = 4, .ofs_y = 6},
    {.bitmap_index = 471, .adv_w = 176, .box_w = 10, .box_h = 6, .ofs_x = 0, .ofs_y = 6}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 1456, .range_length = 10, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 1467, .range_length = 10, .glyph_id_start = 11,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 1488, .range_length = 27, .glyph_id_start = 21,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 1520, .range_length = 5, .glyph_id_start = 48,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 4,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};

extern const lv_font_t hack_11_he_next;


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t hack_hebrew_11 = {
#else
lv_font_t hack_hebrew_11 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 22,          /*The maximum line height required by the font*/
    .base_line = 4,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -2,
    .underline_thickness = 2,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = &hack_11_he_next,
#endif
    .user_data = NULL,
};



#endif /*#if HACK_HEBREW_11*/

