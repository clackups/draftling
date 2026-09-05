/*******************************************************************************
 * Size: 11 px
 * Bpp: 1
 * Opts: --font Greybeard-11px.ttf -r 0x590-0x5FF --size 11 --bpp 1 --format lvgl --no-compress --lv-fallback greybeard_11_he_next --lv-font-name greybeard_hebrew_11 -o /tmp/gen/greybeard_hebrew_11.c
 ******************************************************************************/

#include "lvgl.h"

#ifndef GREYBEARD_HEBREW_11
#define GREYBEARD_HEBREW_11 1
#endif

#if GREYBEARD_HEBREW_11

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
    0x80,

    /* U+05B5 "ֵ" */
    0xa0,

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
    0xe0,

    /* U+05C0 "׀" */
    0xfc,

    /* U+05C1 "ׁ" */
    0xc0,

    /* U+05C2 "ׂ" */
    0xc0,

    /* U+05C3 "׃" */
    0xf3, 0xc0,

    /* U+05C4 "ׄ" */
    0xc0,

    /* U+05D0 "א" */
    0xa9, 0x6a, 0x9d,

    /* U+05D1 "ב" */
    0xc2, 0x22, 0x2f,

    /* U+05D2 "ג" */
    0xc2, 0x22, 0x59,

    /* U+05D3 "ד" */
    0xf8, 0x84, 0x21, 0x8,

    /* U+05D4 "ה" */
    0xf8, 0x52, 0x94, 0xa4,

    /* U+05D5 "ו" */
    0xd5, 0x50,

    /* U+05D6 "ז" */
    0xf9, 0x8, 0x42, 0x10,

    /* U+05D7 "ח" */
    0xfa, 0x52, 0x94, 0xa4,

    /* U+05D8 "ט" */
    0x9c, 0x63, 0x19, 0x70,

    /* U+05D9 "י" */
    0xd4,

    /* U+05DA "ך" */
    0xf1, 0x11, 0x11, 0x11,

    /* U+05DB "כ" */
    0xe1, 0x11, 0x1e,

    /* U+05DC "ל" */
    0x88, 0xf1, 0x11, 0x24,

    /* U+05DD "ם" */
    0xfa, 0x52, 0x94, 0xbc,

    /* U+05DE "מ" */
    0xb3, 0x52, 0x98, 0xdc,

    /* U+05DF "ן" */
    0xd5, 0x55,

    /* U+05E0 "נ" */
    0x31, 0x11, 0x1f,

    /* U+05E1 "ס" */
    0xfa, 0x52, 0x94, 0x98,

    /* U+05E2 "ע" */
    0x99, 0x56, 0x2c,

    /* U+05E3 "ף" */
    0xf2, 0x5a, 0x10, 0x84, 0x21,

    /* U+05E4 "פ" */
    0xf2, 0x5a, 0x10, 0xb8,

    /* U+05E5 "ץ" */
    0x99, 0xac, 0x88, 0x88,

    /* U+05E6 "צ" */
    0x99, 0x62, 0x1f,

    /* U+05E7 "ק" */
    0xe1, 0x9a, 0xa8, 0x88,

    /* U+05E8 "ר" */
    0xe1, 0x11, 0x11,

    /* U+05E9 "ש" */
    0xad, 0x6b, 0x99, 0x70,

    /* U+05EA "ת" */
    0xf2, 0x52, 0x94, 0xe4,

    /* U+05F0 "װ" */
    0xda, 0x52, 0x94, 0xa4,

    /* U+05F1 "ױ" */
    0xda, 0x52, 0x10, 0x84,

    /* U+05F2 "ײ" */
    0xda, 0x52,

    /* U+05F3 "׳" */
    0x2a, 0x0,

    /* U+05F4 "״" */
    0x2a, 0xa8
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 96, .box_w = 1, .box_h = 3, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 1, .adv_w = 96, .box_w = 5, .box_h = 3, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 3, .adv_w = 96, .box_w = 5, .box_h = 3, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 5, .adv_w = 96, .box_w = 5, .box_h = 3, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 7, .adv_w = 96, .box_w = 1, .box_h = 1, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 8, .adv_w = 96, .box_w = 3, .box_h = 1, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 9, .adv_w = 96, .box_w = 5, .box_h = 2, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 11, .adv_w = 96, .box_w = 4, .box_h = 1, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 12, .adv_w = 96, .box_w = 3, .box_h = 2, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 13, .adv_w = 96, .box_w = 1, .box_h = 2, .ofs_x = 1, .ofs_y = 7},
    {.bitmap_index = 14, .adv_w = 96, .box_w = 5, .box_h = 3, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 16, .adv_w = 96, .box_w = 1, .box_h = 2, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 17, .adv_w = 96, .box_w = 1, .box_h = 3, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 18, .adv_w = 96, .box_w = 4, .box_h = 1, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 19, .adv_w = 96, .box_w = 3, .box_h = 1, .ofs_x = 1, .ofs_y = 7},
    {.bitmap_index = 20, .adv_w = 96, .box_w = 1, .box_h = 6, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 21, .adv_w = 96, .box_w = 1, .box_h = 2, .ofs_x = 4, .ofs_y = 7},
    {.bitmap_index = 22, .adv_w = 96, .box_w = 1, .box_h = 2, .ofs_x = 0, .ofs_y = 7},
    {.bitmap_index = 23, .adv_w = 96, .box_w = 2, .box_h = 5, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 25, .adv_w = 96, .box_w = 1, .box_h = 2, .ofs_x = 2, .ofs_y = 7},
    {.bitmap_index = 26, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 29, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 32, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 35, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 39, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 43, .adv_w = 96, .box_w = 2, .box_h = 6, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 45, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 49, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 53, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 57, .adv_w = 96, .box_w = 2, .box_h = 3, .ofs_x = 2, .ofs_y = 3},
    {.bitmap_index = 58, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 62, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 65, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 69, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 73, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 77, .adv_w = 96, .box_w = 2, .box_h = 8, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 79, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 82, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 86, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 89, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 94, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 98, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 102, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 105, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 109, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 112, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 116, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 120, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 124, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 128, .adv_w = 96, .box_w = 5, .box_h = 3, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 130, .adv_w = 96, .box_w = 3, .box_h = 3, .ofs_x = 2, .ofs_y = 3},
    {.bitmap_index = 132, .adv_w = 96, .box_w = 5, .box_h = 3, .ofs_x = 0, .ofs_y = 3}
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

extern lv_font_t greybeard_11_he_next;


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t greybeard_hebrew_11 = {
#else
lv_font_t greybeard_hebrew_11 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 11,          /*The maximum line height required by the font*/
    .base_line = 2,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = &greybeard_11_he_next,
#endif
    .user_data = NULL,
};



#endif /*#if GREYBEARD_HEBREW_11*/

