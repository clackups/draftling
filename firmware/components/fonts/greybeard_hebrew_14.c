/*******************************************************************************
 * Size: 14 px
 * Bpp: 1
 * Opts: --font Greybeard-14px.ttf -r 0x590-0x5FF --size 14 --bpp 1 --format lvgl --no-compress --lv-fallback greybeard_14_he_next --lv-font-name greybeard_hebrew_14 -o /tmp/gen/greybeard_hebrew_14.c
 ******************************************************************************/

#include "lvgl.h"

#ifndef GREYBEARD_HEBREW_14
#define GREYBEARD_HEBREW_14 1
#endif

#if GREYBEARD_HEBREW_14

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+05B0 "ְ" */
    0xa0,

    /* U+05B1 "ֱ" */
    0xd, 0x12,

    /* U+05B2 "ֲ" */
    0xf, 0x2,

    /* U+05B3 "ֳ" */
    0xf, 0x12,

    /* U+05B4 "ִ" */
    0xc0,

    /* U+05B5 "ֵ" */
    0x99,

    /* U+05B6 "ֶ" */
    0x89, 0x0,

    /* U+05B7 "ַ" */
    0xf0,

    /* U+05B8 "ָ" */
    0xe8,

    /* U+05B9 "ֹ" */
    0xc0,

    /* U+05BB "ֻ" */
    0x81, 0x2,

    /* U+05BC "ּ" */
    0xc0,

    /* U+05BD "ֽ" */
    0xe0,

    /* U+05BE "־" */
    0xf0,

    /* U+05BF "ֿ" */
    0xf0,

    /* U+05C0 "׀" */
    0xfe,

    /* U+05C1 "ׁ" */
    0xc0,

    /* U+05C2 "ׂ" */
    0xc0,

    /* U+05C3 "׃" */
    0xf0, 0xf0,

    /* U+05C4 "ׄ" */
    0xc0,

    /* U+05D0 "א" */
    0x94, 0x54, 0xe9, 0x47, 0x20,

    /* U+05D1 "ב" */
    0xe0, 0x84, 0x21, 0xb, 0xe0,

    /* U+05D2 "ג" */
    0x60, 0x84, 0x21, 0x36, 0x20,

    /* U+05D3 "ד" */
    0xf8, 0x84, 0x21, 0x8, 0x40,

    /* U+05D4 "ה" */
    0xfc, 0x10, 0x51, 0x45, 0x14, 0x40,

    /* U+05D5 "ו" */
    0xd5, 0x54,

    /* U+05D6 "ז" */
    0xf9, 0x8, 0x42, 0x10, 0x80,

    /* U+05D7 "ח" */
    0xfd, 0x14, 0x51, 0x45, 0x14, 0x40,

    /* U+05D8 "ט" */
    0x9c, 0x63, 0x18, 0xcb, 0x80,

    /* U+05D9 "י" */
    0xd4,

    /* U+05DA "ך" */
    0xf8, 0x42, 0x10, 0x84, 0x21, 0x8, 0x40,

    /* U+05DB "כ" */
    0xe0, 0x82, 0x10, 0x8b, 0x80,

    /* U+05DC "ל" */
    0x84, 0x3c, 0x10, 0x84, 0x42, 0x20,

    /* U+05DD "ם" */
    0xfd, 0x14, 0x51, 0x45, 0x17, 0xc0,

    /* U+05DE "מ" */
    0x95, 0x53, 0x18, 0xc6, 0xe0,

    /* U+05DF "ן" */
    0xd5, 0x55, 0x50,

    /* U+05E0 "נ" */
    0x31, 0x11, 0x11, 0xf0,

    /* U+05E1 "ס" */
    0xf9, 0x14, 0x51, 0x45, 0x23, 0x0,

    /* U+05E2 "ע" */
    0x9c, 0x52, 0xa3, 0x13, 0x0,

    /* U+05E3 "ף" */
    0xf9, 0x14, 0x59, 0x4, 0x10, 0x41, 0x4, 0x10,

    /* U+05E4 "פ" */
    0xf9, 0x14, 0x59, 0x4, 0x17, 0x80,

    /* U+05E5 "ץ" */
    0xca, 0x54, 0xc4, 0x21, 0x8, 0x42, 0x0,

    /* U+05E6 "צ" */
    0x8c, 0x52, 0x61, 0x7, 0xe0,

    /* U+05E7 "ק" */
    0xf0, 0x43, 0x19, 0x52, 0x90, 0x84, 0x0,

    /* U+05E8 "ר" */
    0xf0, 0x42, 0x10, 0x84, 0x20,

    /* U+05E9 "ש" */
    0xad, 0x6b, 0x5c, 0xcb, 0x80,

    /* U+05EA "ת" */
    0x78, 0x92, 0x49, 0x24, 0x9e, 0x40,

    /* U+05F0 "װ" */
    0xda, 0x52, 0x94, 0xa5, 0x20,

    /* U+05F1 "ױ" */
    0xda, 0x52, 0x10, 0x84, 0x20,

    /* U+05F2 "ײ" */
    0xda, 0x52,

    /* U+05F3 "׳" */
    0x2a, 0x0,

    /* U+05F4 "״" */
    0x25, 0x29, 0x0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 112, .box_w = 1, .box_h = 3, .ofs_x = 3, .ofs_y = -3},
    {.bitmap_index = 1, .adv_w = 112, .box_w = 5, .box_h = 3, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 3, .adv_w = 112, .box_w = 5, .box_h = 3, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 5, .adv_w = 112, .box_w = 5, .box_h = 3, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 7, .adv_w = 112, .box_w = 1, .box_h = 2, .ofs_x = 3, .ofs_y = -3},
    {.bitmap_index = 8, .adv_w = 112, .box_w = 4, .box_h = 2, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 9, .adv_w = 112, .box_w = 5, .box_h = 2, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 11, .adv_w = 112, .box_w = 4, .box_h = 1, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 12, .adv_w = 112, .box_w = 3, .box_h = 2, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 13, .adv_w = 112, .box_w = 1, .box_h = 2, .ofs_x = 2, .ofs_y = 8},
    {.bitmap_index = 14, .adv_w = 112, .box_w = 5, .box_h = 3, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 16, .adv_w = 112, .box_w = 1, .box_h = 2, .ofs_x = 2, .ofs_y = 3},
    {.bitmap_index = 17, .adv_w = 112, .box_w = 1, .box_h = 3, .ofs_x = 3, .ofs_y = -3},
    {.bitmap_index = 18, .adv_w = 112, .box_w = 4, .box_h = 1, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 19, .adv_w = 112, .box_w = 4, .box_h = 1, .ofs_x = 1, .ofs_y = 8},
    {.bitmap_index = 20, .adv_w = 112, .box_w = 1, .box_h = 7, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 21, .adv_w = 112, .box_w = 1, .box_h = 2, .ofs_x = 5, .ofs_y = 8},
    {.bitmap_index = 22, .adv_w = 112, .box_w = 1, .box_h = 2, .ofs_x = 1, .ofs_y = 8},
    {.bitmap_index = 23, .adv_w = 112, .box_w = 2, .box_h = 6, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 25, .adv_w = 112, .box_w = 1, .box_h = 2, .ofs_x = 3, .ofs_y = 8},
    {.bitmap_index = 26, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 31, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 36, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 41, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 46, .adv_w = 112, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 52, .adv_w = 112, .box_w = 2, .box_h = 7, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 54, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 59, .adv_w = 112, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 65, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 70, .adv_w = 112, .box_w = 2, .box_h = 3, .ofs_x = 2, .ofs_y = 4},
    {.bitmap_index = 71, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 78, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 83, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 89, .adv_w = 112, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 95, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 100, .adv_w = 112, .box_w = 2, .box_h = 10, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 103, .adv_w = 112, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 107, .adv_w = 112, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 113, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 118, .adv_w = 112, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 126, .adv_w = 112, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 132, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 139, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 144, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 151, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 156, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 161, .adv_w = 112, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 167, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 172, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 177, .adv_w = 112, .box_w = 5, .box_h = 3, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 179, .adv_w = 112, .box_w = 3, .box_h = 3, .ofs_x = 2, .ofs_y = 4},
    {.bitmap_index = 181, .adv_w = 112, .box_w = 6, .box_h = 3, .ofs_x = 0, .ofs_y = 4}
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

extern lv_font_t greybeard_14_he_next;


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t greybeard_hebrew_14 = {
#else
lv_font_t greybeard_hebrew_14 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 13,          /*The maximum line height required by the font*/
    .base_line = 3,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = &greybeard_14_he_next,
#endif
    .user_data = NULL,
};



#endif /*#if GREYBEARD_HEBREW_14*/

