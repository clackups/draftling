/*******************************************************************************
 * Size: 16 px
 * Bpp: 1
 * Opts: --font Greybeard-16px.ttf -r 0x590-0x5FF --size 16 --bpp 1 --format lvgl --no-compress --lv-fallback greybeard_16_he_next --lv-font-name greybeard_hebrew_16 -o /tmp/gen/greybeard_hebrew_16.c
 ******************************************************************************/

#include "lvgl.h"

#ifndef GREYBEARD_HEBREW_16
#define GREYBEARD_HEBREW_16 1
#endif

#if GREYBEARD_HEBREW_16

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+05B0 "ְ" */
    0xcc,

    /* U+05B1 "ֱ" */
    0x8f, 0x50, 0x98,

    /* U+05B2 "ֲ" */
    0x7, 0xe0, 0x18,

    /* U+05B3 "ֳ" */
    0xed, 0x4, 0xc0,

    /* U+05B4 "ִ" */
    0xc0,

    /* U+05B5 "ֵ" */
    0x99,

    /* U+05B6 "ֶ" */
    0xcc, 0x3, 0x0,

    /* U+05B7 "ַ" */
    0xf0,

    /* U+05B8 "ָ" */
    0xe9, 0x0,

    /* U+05B9 "ֹ" */
    0xc0,

    /* U+05BB "ֻ" */
    0xc0, 0xc0, 0xc0,

    /* U+05BC "ּ" */
    0xc0,

    /* U+05BD "ֽ" */
    0xe0,

    /* U+05BE "־" */
    0xf8,

    /* U+05BF "ֿ" */
    0xf0,

    /* U+05C0 "׀" */
    0xff,

    /* U+05C1 "ׁ" */
    0xc0,

    /* U+05C2 "ׂ" */
    0xc0,

    /* U+05C3 "׃" */
    0xf0, 0xf0,

    /* U+05C4 "ׄ" */
    0xc0,

    /* U+05D0 "א" */
    0x8a, 0x14, 0x9a, 0x92, 0x28, 0x71,

    /* U+05D1 "ב" */
    0xf0, 0x20, 0x82, 0x8, 0x20, 0xbf,

    /* U+05D2 "ג" */
    0x60, 0x84, 0x21, 0x9, 0xb1,

    /* U+05D3 "ד" */
    0xfc, 0x20, 0x82, 0x8, 0x20, 0x82,

    /* U+05D4 "ה" */
    0xfe, 0x4, 0xa, 0x14, 0x28, 0x50, 0xa1,

    /* U+05D5 "ו" */
    0xd5, 0x55,

    /* U+05D6 "ז" */
    0xf9, 0x8, 0x42, 0x10, 0x84,

    /* U+05D7 "ח" */
    0xfe, 0x85, 0xa, 0x14, 0x28, 0x50, 0xa1,

    /* U+05D8 "ט" */
    0x8e, 0x58, 0x61, 0x86, 0x18, 0xbc,

    /* U+05D9 "י" */
    0xd5,

    /* U+05DA "ך" */
    0xfc, 0x10, 0x41, 0x4, 0x10, 0x41, 0x4, 0x10,
    0x40,

    /* U+05DB "כ" */
    0xf0, 0x20, 0x41, 0x4, 0x10, 0xbc,

    /* U+05DC "ל" */
    0x82, 0x8, 0x3e, 0x4, 0x10, 0x41, 0x8, 0x42,
    0x0,

    /* U+05DD "ם" */
    0xfe, 0x85, 0xa, 0x14, 0x28, 0x50, 0xbf,

    /* U+05DE "מ" */
    0x9a, 0xa4, 0x51, 0x86, 0x18, 0x67,

    /* U+05DF "ן" */
    0xd5, 0x55, 0x54,

    /* U+05E0 "נ" */
    0x31, 0x11, 0x11, 0x1f,

    /* U+05E1 "ס" */
    0xfc, 0x49, 0xa, 0x14, 0x28, 0x49, 0xc,

    /* U+05E2 "ע" */
    0xcd, 0x12, 0x49, 0x14, 0x63, 0x30,

    /* U+05E3 "ף" */
    0xfc, 0x85, 0xa, 0x16, 0x20, 0x40, 0x81, 0x2,
    0x4, 0x8,

    /* U+05E4 "פ" */
    0xfc, 0x85, 0xa, 0x16, 0x20, 0x40, 0xbe,

    /* U+05E5 "ץ" */
    0xc5, 0x14, 0x94, 0x61, 0x4, 0x10, 0x41, 0x4,
    0x0,

    /* U+05E6 "צ" */
    0x86, 0x14, 0x8a, 0x10, 0x20, 0x7f,

    /* U+05E7 "ק" */
    0xf8, 0x10, 0x61, 0x8a, 0x4a, 0x28, 0x82, 0x8,
    0x0,

    /* U+05E8 "ר" */
    0xf8, 0x10, 0x41, 0x4, 0x10, 0x41,

    /* U+05E9 "ש" */
    0x93, 0x26, 0x4c, 0x99, 0x32, 0x69, 0x7c,

    /* U+05EA "ת" */
    0x7c, 0x44, 0x89, 0x12, 0x24, 0x48, 0xf1,

    /* U+05F0 "װ" */
    0xcd, 0x14, 0x51, 0x45, 0x14, 0x51,

    /* U+05F1 "ױ" */
    0xcd, 0x14, 0x51, 0x4, 0x10, 0x41,

    /* U+05F2 "ײ" */
    0xcd, 0x14, 0x51,

    /* U+05F3 "׳" */
    0x12, 0x48,

    /* U+05F4 "״" */
    0x12, 0x49, 0x24, 0x80
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 128, .box_w = 2, .box_h = 3, .ofs_x = 3, .ofs_y = -4},
    {.bitmap_index = 1, .adv_w = 128, .box_w = 7, .box_h = 3, .ofs_x = 0, .ofs_y = -4},
    {.bitmap_index = 4, .adv_w = 128, .box_w = 7, .box_h = 3, .ofs_x = 0, .ofs_y = -4},
    {.bitmap_index = 7, .adv_w = 128, .box_w = 6, .box_h = 3, .ofs_x = 1, .ofs_y = -4},
    {.bitmap_index = 10, .adv_w = 128, .box_w = 1, .box_h = 2, .ofs_x = 3, .ofs_y = -3},
    {.bitmap_index = 11, .adv_w = 128, .box_w = 4, .box_h = 2, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 12, .adv_w = 128, .box_w = 6, .box_h = 3, .ofs_x = 1, .ofs_y = -4},
    {.bitmap_index = 15, .adv_w = 128, .box_w = 4, .box_h = 1, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 16, .adv_w = 128, .box_w = 3, .box_h = 3, .ofs_x = 2, .ofs_y = -4},
    {.bitmap_index = 18, .adv_w = 128, .box_w = 1, .box_h = 2, .ofs_x = 2, .ofs_y = 9},
    {.bitmap_index = 19, .adv_w = 128, .box_w = 6, .box_h = 3, .ofs_x = 1, .ofs_y = -4},
    {.bitmap_index = 22, .adv_w = 128, .box_w = 1, .box_h = 2, .ofs_x = 3, .ofs_y = 3},
    {.bitmap_index = 23, .adv_w = 128, .box_w = 1, .box_h = 3, .ofs_x = 3, .ofs_y = -4},
    {.bitmap_index = 24, .adv_w = 128, .box_w = 5, .box_h = 1, .ofs_x = 1, .ofs_y = 7},
    {.bitmap_index = 25, .adv_w = 128, .box_w = 4, .box_h = 1, .ofs_x = 1, .ofs_y = 9},
    {.bitmap_index = 26, .adv_w = 128, .box_w = 1, .box_h = 8, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 27, .adv_w = 128, .box_w = 1, .box_h = 2, .ofs_x = 6, .ofs_y = 9},
    {.bitmap_index = 28, .adv_w = 128, .box_w = 1, .box_h = 2, .ofs_x = 0, .ofs_y = 9},
    {.bitmap_index = 29, .adv_w = 128, .box_w = 2, .box_h = 6, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 31, .adv_w = 128, .box_w = 1, .box_h = 2, .ofs_x = 3, .ofs_y = 9},
    {.bitmap_index = 32, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 38, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 44, .adv_w = 128, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 49, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 55, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 62, .adv_w = 128, .box_w = 2, .box_h = 8, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 64, .adv_w = 128, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 69, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 76, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 82, .adv_w = 128, .box_w = 2, .box_h = 4, .ofs_x = 3, .ofs_y = 4},
    {.bitmap_index = 83, .adv_w = 128, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 92, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 98, .adv_w = 128, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 107, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 114, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 120, .adv_w = 128, .box_w = 2, .box_h = 11, .ofs_x = 3, .ofs_y = -3},
    {.bitmap_index = 123, .adv_w = 128, .box_w = 4, .box_h = 8, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 127, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 134, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 140, .adv_w = 128, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 150, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 157, .adv_w = 128, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 166, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 172, .adv_w = 128, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 181, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 187, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 194, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 201, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 207, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 213, .adv_w = 128, .box_w = 6, .box_h = 4, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 216, .adv_w = 128, .box_w = 4, .box_h = 4, .ofs_x = 2, .ofs_y = 4},
    {.bitmap_index = 218, .adv_w = 128, .box_w = 7, .box_h = 4, .ofs_x = 0, .ofs_y = 4}
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

extern lv_font_t greybeard_16_he_next;


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t greybeard_hebrew_16 = {
#else
lv_font_t greybeard_hebrew_16 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 15,          /*The maximum line height required by the font*/
    .base_line = 4,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = &greybeard_16_he_next,
#endif
    .user_data = NULL,
};



#endif /*#if GREYBEARD_HEBREW_16*/

