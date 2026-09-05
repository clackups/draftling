/*******************************************************************************
 * Size: 18 px
 * Bpp: 1
 * Opts: --font Greybeard-18px.ttf -r 0x590-0x5FF --size 18 --bpp 1 --format lvgl --no-compress --lv-fallback greybeard_18_he_next --lv-font-name greybeard_hebrew_18 -o /tmp/gen/greybeard_hebrew_18.c
 ******************************************************************************/

#include "lvgl.h"

#ifndef GREYBEARD_HEBREW_18
#define GREYBEARD_HEBREW_18 1
#endif

#if GREYBEARD_HEBREW_18

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+05B0 "ְ" */
    0xcc,

    /* U+05B1 "ֱ" */
    0x8b, 0xa8, 0x23,

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
    0xf8,

    /* U+05B8 "ָ" */
    0xf9, 0x8,

    /* U+05B9 "ֹ" */
    0xc0,

    /* U+05BB "ֻ" */
    0xc0, 0xc0, 0xc0,

    /* U+05BC "ּ" */
    0xc0,

    /* U+05BD "ֽ" */
    0xe0,

    /* U+05BE "־" */
    0xfc,

    /* U+05BF "ֿ" */
    0xf0,

    /* U+05C0 "׀" */
    0xff, 0x80,

    /* U+05C1 "ׁ" */
    0xc0,

    /* U+05C2 "ׂ" */
    0xc0,

    /* U+05C3 "׃" */
    0xf0, 0xf0,

    /* U+05C4 "ׄ" */
    0xc0,

    /* U+05D0 "א" */
    0x85, 0x5, 0x9, 0x25, 0x51, 0x21, 0x41, 0xc2,

    /* U+05D1 "ב" */
    0xf8, 0x8, 0x10, 0x20, 0x40, 0x81, 0x2, 0xfe,

    /* U+05D2 "ג" */
    0x60, 0x84, 0x21, 0x8, 0xa9, 0x88,

    /* U+05D3 "ד" */
    0xfe, 0x8, 0x10, 0x20, 0x40, 0x81, 0x2, 0x4,

    /* U+05D4 "ה" */
    0xfe, 0x4, 0xa, 0x14, 0x28, 0x50, 0xa1, 0x42,

    /* U+05D5 "ו" */
    0xd5, 0x55, 0x40,

    /* U+05D6 "ז" */
    0xf9, 0x8, 0x42, 0x10, 0x84, 0x20,

    /* U+05D7 "ח" */
    0xfe, 0x85, 0xa, 0x14, 0x28, 0x50, 0xa1, 0x42,

    /* U+05D8 "ט" */
    0x85, 0x16, 0xc, 0x18, 0x30, 0x60, 0xc2, 0xf8,

    /* U+05D9 "י" */
    0xd5,

    /* U+05DA "ך" */
    0xfc, 0x10, 0x41, 0x4, 0x10, 0x41, 0x4, 0x10,
    0x41,

    /* U+05DB "כ" */
    0xf0, 0x20, 0x41, 0x4, 0x10, 0x42, 0xf0,

    /* U+05DC "ל" */
    0x82, 0x8, 0x3e, 0x4, 0x10, 0x41, 0x8, 0x42,
    0x8,

    /* U+05DD "ם" */
    0xff, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
    0x7f,

    /* U+05DE "מ" */
    0x99, 0x49, 0xa, 0x18, 0x30, 0x60, 0xc1, 0x9e,

    /* U+05DF "ן" */
    0xd5, 0x55, 0x55,

    /* U+05E0 "נ" */
    0x31, 0x11, 0x11, 0x11, 0xf0,

    /* U+05E1 "ס" */
    0xfc, 0x8a, 0xc, 0x18, 0x30, 0x60, 0xa2, 0x38,

    /* U+05E2 "ע" */
    0xc6, 0x84, 0x89, 0x11, 0x22, 0x83, 0xc, 0xe0,

    /* U+05E3 "ף" */
    0xfc, 0x85, 0xa, 0x16, 0x20, 0x40, 0x81, 0x2,
    0x4, 0x8, 0x10,

    /* U+05E4 "פ" */
    0xfc, 0x85, 0xa, 0x16, 0x20, 0x40, 0x81, 0x7c,

    /* U+05E5 "ץ" */
    0xc5, 0x14, 0x92, 0x51, 0x84, 0x10, 0x41, 0x4,
    0x10,

    /* U+05E6 "צ" */
    0x86, 0x14, 0x52, 0x28, 0x40, 0x81, 0xfc,

    /* U+05E7 "ק" */
    0xf8, 0x10, 0x61, 0x86, 0x29, 0x28, 0xa2, 0x8,
    0x20,

    /* U+05E8 "ר" */
    0xf8, 0x10, 0x41, 0x4, 0x10, 0x41, 0x4,

    /* U+05E9 "ש" */
    0x93, 0x26, 0x4c, 0x99, 0x32, 0x64, 0xd2, 0xf8,

    /* U+05EA "ת" */
    0xfe, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21,
    0xe1,

    /* U+05F0 "װ" */
    0xcd, 0x14, 0x51, 0x45, 0x14, 0x51, 0x44,

    /* U+05F1 "ױ" */
    0xcd, 0x14, 0x51, 0x4, 0x10, 0x41, 0x4,

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
    {.bitmap_index = 0, .adv_w = 144, .box_w = 2, .box_h = 3, .ofs_x = 3, .ofs_y = -4},
    {.bitmap_index = 1, .adv_w = 144, .box_w = 8, .box_h = 3, .ofs_x = 0, .ofs_y = -4},
    {.bitmap_index = 4, .adv_w = 144, .box_w = 7, .box_h = 3, .ofs_x = 1, .ofs_y = -4},
    {.bitmap_index = 7, .adv_w = 144, .box_w = 6, .box_h = 3, .ofs_x = 1, .ofs_y = -4},
    {.bitmap_index = 10, .adv_w = 144, .box_w = 1, .box_h = 2, .ofs_x = 4, .ofs_y = -3},
    {.bitmap_index = 11, .adv_w = 144, .box_w = 4, .box_h = 2, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 12, .adv_w = 144, .box_w = 6, .box_h = 3, .ofs_x = 1, .ofs_y = -4},
    {.bitmap_index = 15, .adv_w = 144, .box_w = 5, .box_h = 1, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 16, .adv_w = 144, .box_w = 5, .box_h = 3, .ofs_x = 2, .ofs_y = -4},
    {.bitmap_index = 18, .adv_w = 144, .box_w = 1, .box_h = 2, .ofs_x = 3, .ofs_y = 10},
    {.bitmap_index = 19, .adv_w = 144, .box_w = 6, .box_h = 3, .ofs_x = 1, .ofs_y = -4},
    {.bitmap_index = 22, .adv_w = 144, .box_w = 1, .box_h = 2, .ofs_x = 2, .ofs_y = 4},
    {.bitmap_index = 23, .adv_w = 144, .box_w = 1, .box_h = 3, .ofs_x = 4, .ofs_y = -4},
    {.bitmap_index = 24, .adv_w = 144, .box_w = 6, .box_h = 1, .ofs_x = 1, .ofs_y = 8},
    {.bitmap_index = 25, .adv_w = 144, .box_w = 4, .box_h = 1, .ofs_x = 2, .ofs_y = 10},
    {.bitmap_index = 26, .adv_w = 144, .box_w = 1, .box_h = 9, .ofs_x = 4, .ofs_y = 0},
    {.bitmap_index = 28, .adv_w = 144, .box_w = 1, .box_h = 2, .ofs_x = 7, .ofs_y = 10},
    {.bitmap_index = 29, .adv_w = 144, .box_w = 1, .box_h = 2, .ofs_x = 1, .ofs_y = 10},
    {.bitmap_index = 30, .adv_w = 144, .box_w = 2, .box_h = 6, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 32, .adv_w = 144, .box_w = 1, .box_h = 2, .ofs_x = 4, .ofs_y = 10},
    {.bitmap_index = 33, .adv_w = 144, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 41, .adv_w = 144, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 49, .adv_w = 144, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 55, .adv_w = 144, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 63, .adv_w = 144, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 71, .adv_w = 144, .box_w = 2, .box_h = 9, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 74, .adv_w = 144, .box_w = 5, .box_h = 9, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 80, .adv_w = 144, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 88, .adv_w = 144, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 96, .adv_w = 144, .box_w = 2, .box_h = 4, .ofs_x = 3, .ofs_y = 5},
    {.bitmap_index = 97, .adv_w = 144, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 106, .adv_w = 144, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 113, .adv_w = 144, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 122, .adv_w = 144, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 131, .adv_w = 144, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 139, .adv_w = 144, .box_w = 2, .box_h = 12, .ofs_x = 3, .ofs_y = -3},
    {.bitmap_index = 142, .adv_w = 144, .box_w = 4, .box_h = 9, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 147, .adv_w = 144, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 155, .adv_w = 144, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 163, .adv_w = 144, .box_w = 7, .box_h = 12, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 174, .adv_w = 144, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 182, .adv_w = 144, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 191, .adv_w = 144, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 198, .adv_w = 144, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 207, .adv_w = 144, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 214, .adv_w = 144, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 222, .adv_w = 144, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 231, .adv_w = 144, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 238, .adv_w = 144, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 245, .adv_w = 144, .box_w = 6, .box_h = 4, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 248, .adv_w = 144, .box_w = 4, .box_h = 4, .ofs_x = 3, .ofs_y = 5},
    {.bitmap_index = 250, .adv_w = 144, .box_w = 7, .box_h = 4, .ofs_x = 1, .ofs_y = 5}
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

extern lv_font_t greybeard_18_he_next;


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t greybeard_hebrew_18 = {
#else
lv_font_t greybeard_hebrew_18 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 16,          /*The maximum line height required by the font*/
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
    .fallback = &greybeard_18_he_next,
#endif
    .user_data = NULL,
};



#endif /*#if GREYBEARD_HEBREW_18*/

