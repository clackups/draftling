/*******************************************************************************
 * Size: 26 px
 * Bpp: 1
 * Opts: --font Greybeard-22px.ttf -r 0x590-0x5FF --size 26 --bpp 1 --format lvgl --no-compress --lv-fallback greybeard_26_he_next --lv-font-name greybeard_hebrew_26 -o /tmp/gen/greybeard_hebrew_26.c
 ******************************************************************************/

#include "lvgl.h"

#ifndef GREYBEARD_HEBREW_26
#define GREYBEARD_HEBREW_26 1
#endif

#if GREYBEARD_HEBREW_26

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+05B0 "ְ" */
    0xf0, 0xf0,

    /* U+05B1 "ֱ" */
    0x0, 0xf1, 0xbc, 0x60, 0x0, 0x18, 0xc6, 0x30,

    /* U+05B2 "ֲ" */
    0x1, 0x80, 0xc0, 0x1f, 0x8f, 0xd8, 0xc,

    /* U+05B3 "ֳ" */
    0x0, 0xff, 0xbf, 0xe0, 0x60, 0x18, 0xc6, 0x30,

    /* U+05B4 "ִ" */
    0xf0,

    /* U+05B5 "ֵ" */
    0xc7, 0x8c,

    /* U+05B6 "ֶ" */
    0xc0, 0xf0, 0x30, 0x0, 0x30, 0xc, 0x0,

    /* U+05B7 "ַ" */
    0xff, 0xfc,

    /* U+05B8 "ָ" */
    0xff, 0xfc, 0x60, 0xc1, 0x80,

    /* U+05B9 "ֹ" */
    0xf0,

    /* U+05BB "ֻ" */
    0xc0, 0x30, 0x0, 0xc0, 0x33, 0x0, 0xc0,

    /* U+05BC "ּ" */
    0xf0,

    /* U+05BD "ֽ" */
    0xff,

    /* U+05BE "־" */
    0xff, 0xff, 0xfc,

    /* U+05BF "ֿ" */
    0xff, 0xf0,

    /* U+05C0 "׀" */
    0xff, 0xff, 0xff, 0xc0,

    /* U+05C1 "ׁ" */
    0xf0,

    /* U+05C2 "ׂ" */
    0xf0,

    /* U+05C3 "׃" */
    0x4f, 0x40, 0x0, 0x4f, 0x40,

    /* U+05C4 "ׄ" */
    0xf0,

    /* U+05D0 "א" */
    0x83, 0x20, 0xcc, 0x1d, 0x87, 0x79, 0xdf, 0x45,
    0xf3, 0x1c, 0xc3, 0x30, 0x7c, 0x1f, 0x83, 0xe0,
    0x0,

    /* U+05D1 "ב" */
    0xfe, 0x7f, 0x80, 0xc0, 0x60, 0x30, 0x18, 0xc,
    0x6, 0x3, 0x1, 0x80, 0xdf, 0xff, 0xf8,

    /* U+05D2 "ג" */
    0x3c, 0x3e, 0x6, 0x6, 0x6, 0x6, 0x6, 0x1e,
    0x1e, 0x3f, 0x73, 0xf3, 0xc3,

    /* U+05D3 "ד" */
    0xff, 0xff, 0xfc, 0x6, 0x0, 0xc0, 0x18, 0x3,
    0x0, 0x60, 0xc, 0x1, 0x80, 0x30, 0x6, 0x0,
    0xc0, 0x18,

    /* U+05D4 "ה" */
    0xff, 0xff, 0xf0, 0xc, 0x3, 0x0, 0xd8, 0x36,
    0xd, 0x83, 0x60, 0xd8, 0x36, 0xd, 0x83, 0x60,
    0xc0,

    /* U+05D5 "ו" */
    0xff, 0xc6, 0x31, 0x8c, 0x63, 0x18, 0xc6, 0x31,
    0x80,

    /* U+05D6 "ז" */
    0x7f, 0xff, 0x81, 0x0, 0x80, 0xc0, 0x60, 0x30,
    0x18, 0xc, 0x6, 0x3, 0x1, 0x80, 0xc0,

    /* U+05D7 "ח" */
    0xff, 0xff, 0xf6, 0xd, 0x83, 0x60, 0xd8, 0x36,
    0xd, 0x83, 0x60, 0xd8, 0x36, 0xd, 0x83, 0x60,
    0xc0,

    /* U+05D8 "ט" */
    0xc3, 0x63, 0xf1, 0x78, 0x3c, 0x1e, 0xf, 0x7,
    0x83, 0xc1, 0xe0, 0xb0, 0xdf, 0xef, 0xc0,

    /* U+05D9 "י" */
    0xff, 0xc6, 0x31, 0x8c, 0x60,

    /* U+05DA "ך" */
    0xff, 0xff, 0xc0, 0x60, 0x30, 0x18, 0xc, 0x6,
    0x3, 0x1, 0x80, 0xc0, 0x60, 0x30, 0x18, 0xc,
    0x6, 0x3, 0x1, 0x80, 0xc0,

    /* U+05DB "כ" */
    0xfc, 0x7f, 0x80, 0xc0, 0x20, 0x18, 0xc, 0x6,
    0x3, 0x1, 0x80, 0xc0, 0xdf, 0xef, 0xc0,

    /* U+05DC "ל" */
    0x60, 0x60, 0x30, 0x18, 0xc, 0x7, 0xfb, 0xfe,
    0x3, 0x1, 0x80, 0xc0, 0x60, 0x30, 0x30, 0x18,
    0xc, 0x8, 0xc, 0x6, 0x0,

    /* U+05DD "ם" */
    0xff, 0xff, 0xf6, 0xd, 0x83, 0x60, 0xd8, 0x36,
    0xd, 0x83, 0x60, 0xd8, 0x36, 0xd, 0xff, 0x7f,
    0xc0,

    /* U+05DE "מ" */
    0xcf, 0x33, 0xec, 0xdd, 0x43, 0x60, 0xd8, 0x36,
    0xd, 0x83, 0x40, 0xf0, 0x3c, 0xf, 0x3f, 0xcf,
    0xc0,

    /* U+05DF "ן" */
    0xff, 0xc6, 0x31, 0x8c, 0x63, 0x18, 0xc6, 0x31,
    0x8c, 0x63, 0x18, 0xc0,

    /* U+05E0 "נ" */
    0x1e, 0x3c, 0x18, 0x30, 0x60, 0xc1, 0x83, 0x6,
    0xc, 0x1f, 0xff, 0xe0,

    /* U+05E1 "ס" */
    0xff, 0xbf, 0xe3, 0x18, 0x82, 0x60, 0xd8, 0x36,
    0xd, 0x83, 0x60, 0xd8, 0x32, 0x8, 0xfe, 0x1f,
    0x0,

    /* U+05E2 "ע" */
    0xc3, 0xe0, 0xd0, 0x6c, 0x36, 0x18, 0xcc, 0x3e,
    0x1f, 0x6, 0x2, 0x3, 0x1f, 0x8e, 0x0,

    /* U+05E3 "ף" */
    0xff, 0xbf, 0xe6, 0xd, 0x83, 0x60, 0xdf, 0x37,
    0xcc, 0x3, 0x0, 0xc0, 0x30, 0xc, 0x3, 0x0,
    0xc0, 0x30, 0xc, 0x3, 0x0, 0xc0, 0x30,

    /* U+05E4 "פ" */
    0xff, 0xbf, 0xf6, 0xd, 0x83, 0x60, 0xdf, 0x37,
    0xcc, 0x3, 0x0, 0xc0, 0x30, 0xf, 0xfe, 0xff,
    0x80,

    /* U+05E5 "ץ" */
    0xe1, 0xf8, 0x36, 0x9, 0x86, 0x61, 0x98, 0xc6,
    0x61, 0x98, 0x64, 0x1e, 0x7, 0x81, 0x80, 0x60,
    0x18, 0x6, 0x1, 0x80, 0x60, 0x18, 0x0,

    /* U+05E6 "צ" */
    0xc3, 0xe0, 0xd0, 0x44, 0x61, 0xb0, 0x70, 0x30,
    0xc, 0x3, 0x0, 0x80, 0x7f, 0xff, 0xf8,

    /* U+05E7 "ק" */
    0xff, 0xbf, 0xe0, 0xc, 0x3, 0x0, 0xd8, 0x36,
    0x9, 0x86, 0x61, 0x98, 0xc6, 0x31, 0x88, 0x62,
    0x18, 0x6, 0x1, 0x80, 0x60, 0x18, 0x0,

    /* U+05E8 "ר" */
    0xfe, 0x7f, 0x0, 0xc0, 0x30, 0x18, 0xc, 0x6,
    0x3, 0x1, 0x80, 0xc0, 0x60, 0x30, 0x18,

    /* U+05E9 "ש" */
    0xc6, 0x3c, 0x63, 0xc6, 0x3c, 0x63, 0xc6, 0x3c,
    0x63, 0xc6, 0x3c, 0x63, 0xc6, 0x3c, 0x44, 0xcc,
    0xcf, 0xfc, 0xff, 0x0,

    /* U+05EA "ת" */
    0x7f, 0x9f, 0xf3, 0xc, 0xc3, 0x30, 0xcc, 0x33,
    0xc, 0xc3, 0x30, 0xcc, 0x33, 0xf, 0xc3, 0xf0,
    0xc0,

    /* U+05F0 "װ" */
    0xfb, 0xff, 0x7c, 0x61, 0x8c, 0x31, 0x86, 0x30,
    0xc6, 0x18, 0xc3, 0x18, 0x63, 0xc, 0x61, 0x8c,
    0x31, 0x86,

    /* U+05F1 "ױ" */
    0xfb, 0xff, 0x7c, 0x61, 0x8c, 0x31, 0x86, 0x30,
    0xc6, 0x18, 0x3, 0x0, 0x60, 0xc, 0x1, 0x80,
    0x30, 0x6,

    /* U+05F2 "ײ" */
    0xfb, 0xff, 0x7c, 0x61, 0x8c, 0x31, 0x86, 0x30,
    0xc6, 0x18,

    /* U+05F3 "׳" */
    0x6, 0x18, 0x61, 0x4, 0x18, 0x0,

    /* U+05F4 "״" */
    0x6, 0x30, 0xcc, 0x19, 0x82, 0x10, 0x42, 0xc,
    0x60
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 208, .box_w = 2, .box_h = 6, .ofs_x = 6, .ofs_y = -6},
    {.bitmap_index = 2, .adv_w = 208, .box_w = 10, .box_h = 6, .ofs_x = 1, .ofs_y = -6},
    {.bitmap_index = 10, .adv_w = 208, .box_w = 9, .box_h = 6, .ofs_x = 2, .ofs_y = -6},
    {.bitmap_index = 17, .adv_w = 208, .box_w = 10, .box_h = 6, .ofs_x = 1, .ofs_y = -6},
    {.bitmap_index = 25, .adv_w = 208, .box_w = 2, .box_h = 2, .ofs_x = 6, .ofs_y = -4},
    {.bitmap_index = 26, .adv_w = 208, .box_w = 7, .box_h = 2, .ofs_x = 4, .ofs_y = -4},
    {.bitmap_index = 28, .adv_w = 208, .box_w = 10, .box_h = 5, .ofs_x = 2, .ofs_y = -6},
    {.bitmap_index = 35, .adv_w = 208, .box_w = 7, .box_h = 2, .ofs_x = 4, .ofs_y = -5},
    {.bitmap_index = 37, .adv_w = 208, .box_w = 7, .box_h = 5, .ofs_x = 4, .ofs_y = -7},
    {.bitmap_index = 42, .adv_w = 208, .box_w = 2, .box_h = 2, .ofs_x = 5, .ofs_y = 15},
    {.bitmap_index = 43, .adv_w = 208, .box_w = 10, .box_h = 5, .ofs_x = 2, .ofs_y = -6},
    {.bitmap_index = 50, .adv_w = 208, .box_w = 2, .box_h = 2, .ofs_x = 5, .ofs_y = 6},
    {.bitmap_index = 51, .adv_w = 208, .box_w = 2, .box_h = 4, .ofs_x = 6, .ofs_y = -6},
    {.bitmap_index = 52, .adv_w = 208, .box_w = 11, .box_h = 2, .ofs_x = 1, .ofs_y = 11},
    {.bitmap_index = 55, .adv_w = 208, .box_w = 6, .box_h = 2, .ofs_x = 4, .ofs_y = 14},
    {.bitmap_index = 57, .adv_w = 208, .box_w = 2, .box_h = 13, .ofs_x = 6, .ofs_y = 0},
    {.bitmap_index = 61, .adv_w = 208, .box_w = 2, .box_h = 2, .ofs_x = 11, .ofs_y = 15},
    {.bitmap_index = 62, .adv_w = 208, .box_w = 2, .box_h = 2, .ofs_x = 1, .ofs_y = 15},
    {.bitmap_index = 63, .adv_w = 208, .box_w = 4, .box_h = 9, .ofs_x = 5, .ofs_y = 0},
    {.bitmap_index = 68, .adv_w = 208, .box_w = 2, .box_h = 2, .ofs_x = 6, .ofs_y = 15},
    {.bitmap_index = 69, .adv_w = 208, .box_w = 10, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 86, .adv_w = 208, .box_w = 9, .box_h = 13, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 101, .adv_w = 208, .box_w = 8, .box_h = 13, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 114, .adv_w = 208, .box_w = 11, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 132, .adv_w = 208, .box_w = 10, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 149, .adv_w = 208, .box_w = 5, .box_h = 13, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 158, .adv_w = 208, .box_w = 9, .box_h = 13, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 173, .adv_w = 208, .box_w = 10, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 190, .adv_w = 208, .box_w = 9, .box_h = 13, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 205, .adv_w = 208, .box_w = 5, .box_h = 7, .ofs_x = 3, .ofs_y = 6},
    {.bitmap_index = 210, .adv_w = 208, .box_w = 9, .box_h = 18, .ofs_x = 2, .ofs_y = -5},
    {.bitmap_index = 231, .adv_w = 208, .box_w = 9, .box_h = 13, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 246, .adv_w = 208, .box_w = 9, .box_h = 18, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 267, .adv_w = 208, .box_w = 10, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 284, .adv_w = 208, .box_w = 10, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 301, .adv_w = 208, .box_w = 5, .box_h = 18, .ofs_x = 3, .ofs_y = -5},
    {.bitmap_index = 313, .adv_w = 208, .box_w = 7, .box_h = 13, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 325, .adv_w = 208, .box_w = 10, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 342, .adv_w = 208, .box_w = 9, .box_h = 13, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 357, .adv_w = 208, .box_w = 10, .box_h = 18, .ofs_x = 1, .ofs_y = -5},
    {.bitmap_index = 380, .adv_w = 208, .box_w = 10, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 397, .adv_w = 208, .box_w = 10, .box_h = 18, .ofs_x = 1, .ofs_y = -5},
    {.bitmap_index = 420, .adv_w = 208, .box_w = 9, .box_h = 13, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 435, .adv_w = 208, .box_w = 10, .box_h = 18, .ofs_x = 1, .ofs_y = -5},
    {.bitmap_index = 458, .adv_w = 208, .box_w = 9, .box_h = 13, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 473, .adv_w = 208, .box_w = 12, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 493, .adv_w = 208, .box_w = 10, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 510, .adv_w = 208, .box_w = 11, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 528, .adv_w = 208, .box_w = 11, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 546, .adv_w = 208, .box_w = 11, .box_h = 7, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 556, .adv_w = 208, .box_w = 7, .box_h = 6, .ofs_x = 4, .ofs_y = 7},
    {.bitmap_index = 562, .adv_w = 208, .box_w = 12, .box_h = 6, .ofs_x = 1, .ofs_y = 7}
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

extern lv_font_t greybeard_26_he_next;


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t greybeard_hebrew_26 = {
#else
lv_font_t greybeard_hebrew_26 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 25,          /*The maximum line height required by the font*/
    .base_line = 7,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = &greybeard_26_he_next,
#endif
    .user_data = NULL,
};



#endif /*#if GREYBEARD_HEBREW_26*/

