/*******************************************************************************
 * Size: 18 px
 * Bpp: 1
 * Opts: --font Greybeard-11px.ttf -r 0x590-0x5FF --size 18 --bpp 1 --format lvgl --no-compress --lv-fallback hack_9_he_next --lv-font-name hack_hebrew_9 -o hack_hebrew_9.c
 ******************************************************************************/

#include "lvgl.h"

#ifndef HACK_HEBREW_9
#define HACK_HEBREW_9 1
#endif

#if HACK_HEBREW_9

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+05B0 "ְ" */
    0xc0, 0xc0,

    /* U+05B1 "ֱ" */
    0x3, 0x0, 0xd8, 0x0, 0x23,

    /* U+05B2 "ֲ" */
    0x3, 0x0, 0xf8, 0x0, 0x3,

    /* U+05B3 "ֳ" */
    0x3, 0x0, 0xf8, 0x60, 0x63,

    /* U+05B4 "ִ" */
    0xc0,

    /* U+05B5 "ֵ" */
    0xd8,

    /* U+05B6 "ֶ" */
    0xc3, 0x0, 0x18,

    /* U+05B7 "ַ" */
    0xfe,

    /* U+05B8 "ָ" */
    0xfb, 0x18,

    /* U+05B9 "ֹ" */
    0xfc,

    /* U+05BB "ֻ" */
    0xc0, 0x0, 0x18, 0x0, 0x3,

    /* U+05BC "ּ" */
    0xfc,

    /* U+05BD "ֽ" */
    0xff, 0xc0,

    /* U+05BE "־" */
    0xfe,

    /* U+05BF "ֿ" */
    0xf8,

    /* U+05C0 "׀" */
    0xff, 0xff, 0xf0,

    /* U+05C1 "ׁ" */
    0xfc,

    /* U+05C2 "ׂ" */
    0xfc,

    /* U+05C3 "׃" */
    0xff, 0x81, 0xff,

    /* U+05C4 "ׄ" */
    0xfc,

    /* U+05D0 "א" */
    0xd9, 0x83, 0x18, 0x3, 0x9b, 0x36, 0x63, 0xc7,
    0xcc,

    /* U+05D1 "ב" */
    0xe0, 0x30, 0x60, 0xc1, 0x83, 0x6, 0xc, 0x19,
    0xfc,

    /* U+05D2 "ג" */
    0xe0, 0x0, 0x60, 0xc1, 0x83, 0x6, 0x13, 0x7,
    0x8c,

    /* U+05D3 "ד" */
    0xff, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6,
    0x6, 0x6,

    /* U+05D4 "ה" */
    0xff, 0x80, 0xc0, 0x60, 0x33, 0x19, 0x8c, 0xc6,
    0x63, 0x31, 0x98, 0xc0,

    /* U+05D5 "ו" */
    0xf3, 0x33, 0x33, 0x33, 0x33,

    /* U+05D6 "ז" */
    0xff, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
    0x18, 0x18,

    /* U+05D7 "ח" */
    0xff, 0x98, 0xcc, 0x66, 0x33, 0x19, 0x8c, 0xc6,
    0x63, 0x31, 0x98, 0xc0,

    /* U+05D8 "ט" */
    0xc7, 0xc1, 0xc1, 0xc1, 0xc1, 0xc1, 0xc1, 0xc6,
    0xc0, 0xf8,

    /* U+05D9 "י" */
    0xf3, 0x33, 0x30,

    /* U+05DA "ך" */
    0xfe, 0xc, 0x18, 0x30, 0x60, 0xc1, 0x83, 0x6,
    0xc, 0x18, 0x30, 0x60,

    /* U+05DB "כ" */
    0xf8, 0x0, 0x18, 0x30, 0x60, 0xc1, 0x83, 0x1,
    0xf0,

    /* U+05DC "ל" */
    0xc1, 0x83, 0x7, 0xf0, 0x60, 0xc1, 0x83, 0x6,
    0xc, 0x60, 0x2, 0x0,

    /* U+05DD "ם" */
    0xff, 0x98, 0xcc, 0x66, 0x33, 0x19, 0x8c, 0xc6,
    0x63, 0x31, 0x9f, 0xc0,

    /* U+05DE "מ" */
    0xdc, 0x18, 0x3b, 0x3b, 0x23, 0x23, 0x23, 0xc3,
    0xc3, 0xdf,

    /* U+05DF "ן" */
    0xf3, 0x33, 0x33, 0x33, 0x33, 0x33, 0x30,

    /* U+05E0 "נ" */
    0x1e, 0xc, 0x18, 0x30, 0x60, 0xc1, 0x83, 0x7,
    0xfc,

    /* U+05E1 "ס" */
    0xff, 0x98, 0xcc, 0x66, 0x33, 0x19, 0x8c, 0xc6,
    0x63, 0x31, 0x87, 0x0,

    /* U+05E2 "ע" */
    0xc7, 0x8f, 0x18, 0x32, 0x67, 0xe, 0xc, 0x1,
    0xc0,

    /* U+05E3 "ף" */
    0xfe, 0x60, 0x61, 0x61, 0x79, 0x1, 0x1, 0x1,
    0x1, 0x1, 0x1, 0x1, 0x1,

    /* U+05E4 "פ" */
    0xfc, 0x60, 0x63, 0x63, 0x7b, 0x3, 0x3, 0x3,
    0x0, 0x7c,

    /* U+05E5 "ץ" */
    0xc7, 0x8f, 0x1e, 0xd, 0x9c, 0x30, 0x60, 0xc1,
    0x83, 0x6, 0xc, 0x0,

    /* U+05E6 "צ" */
    0xc7, 0x8f, 0x18, 0x3, 0x83, 0x6, 0x3, 0x7,
    0xfc,

    /* U+05E7 "ק" */
    0xf8, 0x0, 0x1e, 0x3c, 0x7b, 0x36, 0x6c, 0xc1,
    0x83, 0x6, 0xc, 0x0,

    /* U+05E8 "ר" */
    0xf8, 0x0, 0x18, 0x30, 0x60, 0xc1, 0x83, 0x6,
    0xc,

    /* U+05E9 "ש" */
    0xd9, 0xd9, 0xd9, 0xd9, 0xd9, 0xe1, 0xc0, 0xc6,
    0xc0, 0xf8,

    /* U+05EA "ת" */
    0xfe, 0xc1, 0x8b, 0x16, 0x2c, 0x58, 0xb1, 0x63,
    0xc4,

    /* U+05F0 "װ" */
    0xf3, 0xcc, 0x33, 0xc, 0xc3, 0x30, 0xcc, 0x33,
    0xc, 0xc3, 0x30, 0xcc, 0x30,

    /* U+05F1 "ױ" */
    0xf3, 0xcc, 0x33, 0xc, 0xc3, 0x30, 0xc0, 0x30,
    0xc, 0x3, 0x0, 0xc0, 0x30,

    /* U+05F2 "ײ" */
    0xf3, 0xcc, 0x33, 0xc, 0xc3, 0x30, 0xc0,

    /* U+05F3 "׳" */
    0xc, 0x80, 0x30,

    /* U+05F4 "״" */
    0x19, 0x26, 0x0, 0xd8
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 160, .box_w = 2, .box_h = 5, .ofs_x = 3, .ofs_y = -3},
    {.bitmap_index = 2, .adv_w = 160, .box_w = 8, .box_h = 5, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 7, .adv_w = 160, .box_w = 8, .box_h = 5, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 12, .adv_w = 160, .box_w = 8, .box_h = 5, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 17, .adv_w = 160, .box_w = 2, .box_h = 1, .ofs_x = 3, .ofs_y = -3},
    {.bitmap_index = 18, .adv_w = 160, .box_w = 5, .box_h = 1, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 19, .adv_w = 160, .box_w = 8, .box_h = 3, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 22, .adv_w = 160, .box_w = 7, .box_h = 1, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 23, .adv_w = 160, .box_w = 5, .box_h = 3, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 25, .adv_w = 160, .box_w = 2, .box_h = 3, .ofs_x = 2, .ofs_y = 11},
    {.bitmap_index = 26, .adv_w = 160, .box_w = 8, .box_h = 5, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 31, .adv_w = 160, .box_w = 2, .box_h = 3, .ofs_x = 2, .ofs_y = 3},
    {.bitmap_index = 32, .adv_w = 160, .box_w = 2, .box_h = 5, .ofs_x = 3, .ofs_y = -3},
    {.bitmap_index = 34, .adv_w = 160, .box_w = 7, .box_h = 1, .ofs_x = 2, .ofs_y = 9},
    {.bitmap_index = 35, .adv_w = 160, .box_w = 5, .box_h = 1, .ofs_x = 2, .ofs_y = 12},
    {.bitmap_index = 36, .adv_w = 160, .box_w = 2, .box_h = 10, .ofs_x = 5, .ofs_y = 0},
    {.bitmap_index = 39, .adv_w = 160, .box_w = 2, .box_h = 3, .ofs_x = 7, .ofs_y = 11},
    {.bitmap_index = 40, .adv_w = 160, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 11},
    {.bitmap_index = 41, .adv_w = 160, .box_w = 3, .box_h = 8, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 44, .adv_w = 160, .box_w = 2, .box_h = 3, .ofs_x = 3, .ofs_y = 11},
    {.bitmap_index = 45, .adv_w = 160, .box_w = 7, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 54, .adv_w = 160, .box_w = 7, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 63, .adv_w = 160, .box_w = 7, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 72, .adv_w = 160, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 82, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 94, .adv_w = 160, .box_w = 4, .box_h = 10, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 99, .adv_w = 160, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 109, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 121, .adv_w = 160, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 131, .adv_w = 160, .box_w = 4, .box_h = 5, .ofs_x = 3, .ofs_y = 5},
    {.bitmap_index = 134, .adv_w = 160, .box_w = 7, .box_h = 13, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 146, .adv_w = 160, .box_w = 7, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 155, .adv_w = 160, .box_w = 7, .box_h = 13, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 167, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 179, .adv_w = 160, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 189, .adv_w = 160, .box_w = 4, .box_h = 13, .ofs_x = 3, .ofs_y = -3},
    {.bitmap_index = 196, .adv_w = 160, .box_w = 7, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 205, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 217, .adv_w = 160, .box_w = 7, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 226, .adv_w = 160, .box_w = 8, .box_h = 13, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 239, .adv_w = 160, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 249, .adv_w = 160, .box_w = 7, .box_h = 13, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 261, .adv_w = 160, .box_w = 7, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 270, .adv_w = 160, .box_w = 7, .box_h = 13, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 282, .adv_w = 160, .box_w = 7, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 291, .adv_w = 160, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 301, .adv_w = 160, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 310, .adv_w = 160, .box_w = 10, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 323, .adv_w = 160, .box_w = 10, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 336, .adv_w = 160, .box_w = 10, .box_h = 5, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 343, .adv_w = 160, .box_w = 6, .box_h = 4, .ofs_x = 3, .ofs_y = 6},
    {.bitmap_index = 346, .adv_w = 160, .box_w = 8, .box_h = 4, .ofs_x = 0, .ofs_y = 6}
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

extern const lv_font_t hack_9_he_next;


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t hack_hebrew_9 = {
#else
lv_font_t hack_hebrew_9 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 17,          /*The maximum line height required by the font*/
    .base_line = 3,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -2,
    .underline_thickness = 2,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = &hack_9_he_next,
#endif
    .user_data = NULL,
};



#endif /*#if HACK_HEBREW_9*/

