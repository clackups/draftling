/*******************************************************************************
 * Size: 26 px
 * Bpp: 1
 * Opts: --font Greybeard-22px.ttf --autohint-off -r 0x590-0x5FF --size 26 --bpp 1 --format lvgl --no-compress --lv-fallback greybeard_26_he_next --lv-font-name greybeard_hebrew_26 -o greybeard_hebrew_26.c
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
    0x0, 0xfc, 0xdf, 0x98, 0x73, 0x1, 0x8e, 0x31,
    0xc0,

    /* U+05B2 "ֲ" */
    0x1, 0xc0, 0x7f, 0xc3, 0xf0, 0xfd, 0xc0, 0x70,

    /* U+05B3 "ֳ" */
    0x0, 0xff, 0xdf, 0xf8, 0x7f, 0x1, 0x8e, 0x31,
    0xc0,

    /* U+05B4 "ִ" */
    0xfc,

    /* U+05B5 "ֵ" */
    0xcf, 0x9f, 0x38,

    /* U+05B6 "ֶ" */
    0xe1, 0xf8, 0x7e, 0x1c, 0x30, 0xc, 0x0,

    /* U+05B7 "ַ" */
    0xff, 0xff, 0xf8,

    /* U+05B8 "ָ" */
    0xff, 0xff, 0xf9, 0x83, 0x0,

    /* U+05B9 "ֹ" */
    0xfc,

    /* U+05BB "ֻ" */
    0xe0, 0x3b, 0xe, 0xc0, 0x37, 0x1, 0xc0,

    /* U+05BC "ּ" */
    0xf0,

    /* U+05BD "ֽ" */
    0xff, 0xc0,

    /* U+05BE "־" */
    0xff, 0xff, 0xfc,

    /* U+05BF "ֿ" */
    0xff, 0xfe,

    /* U+05C0 "׀" */
    0xff, 0xff, 0xff, 0xc0,

    /* U+05C1 "ׁ" */
    0xfc,

    /* U+05C2 "ׂ" */
    0xff, 0x80,

    /* U+05C3 "׃" */
    0x5d, 0x0, 0x97, 0x40,

    /* U+05C4 "ׄ" */
    0xfc,

    /* U+05D0 "א" */
    0x83, 0x1c, 0x7b, 0xc7, 0xf8, 0xf7, 0x9e, 0xfa,
    0x1b, 0xc7, 0x38, 0xe3, 0xdc, 0x3f, 0x87, 0xf8,
    0x7f, 0x2,

    /* U+05D1 "ב" */
    0xfe, 0x3f, 0xe0, 0x38, 0xe, 0x3, 0x80, 0xe0,
    0x38, 0xe, 0x3, 0x80, 0xe0, 0x3b, 0xff, 0xff,
    0xc0,

    /* U+05D2 "ג" */
    0x1c, 0xf, 0x1, 0x80, 0xc0, 0x60, 0x30, 0x18,
    0x1c, 0x1f, 0x9d, 0xce, 0xfe, 0x7e, 0x38,

    /* U+05D3 "ד" */
    0xff, 0xff, 0xfc, 0x7, 0x0, 0xe0, 0x1c, 0x3,
    0x80, 0x70, 0xe, 0x1, 0xc0, 0x38, 0x7, 0x0,
    0xe0, 0x1c,

    /* U+05D4 "ה" */
    0xff, 0xff, 0xfc, 0x3, 0x80, 0x70, 0xe, 0xe1,
    0xdc, 0x3b, 0x87, 0x70, 0xee, 0x1d, 0xc3, 0xb8,
    0x77, 0xe,

    /* U+05D5 "ו" */
    0xff, 0x33, 0x33, 0x33, 0x33, 0x33, 0x30,

    /* U+05D6 "ז" */
    0x3f, 0xff, 0xe0, 0x40, 0x10, 0xc, 0x3, 0x0,
    0xc0, 0x30, 0xc, 0x3, 0x0, 0xc0, 0x30, 0xc,
    0x0,

    /* U+05D7 "ח" */
    0xff, 0xff, 0xfd, 0xc3, 0xb8, 0x77, 0xe, 0xe1,
    0xdc, 0x3b, 0x87, 0x70, 0xee, 0x1d, 0xc3, 0xb8,
    0x77, 0xe,

    /* U+05D8 "ט" */
    0xe3, 0xb9, 0xfe, 0x5f, 0x97, 0xe1, 0xf8, 0x7e,
    0x1f, 0x87, 0xe1, 0xf8, 0xee, 0x3b, 0xfe, 0xfc,
    0x0,

    /* U+05D9 "י" */
    0xff, 0x33, 0x33, 0x30,

    /* U+05DA "ך" */
    0xff, 0xff, 0xf0, 0x1c, 0x7, 0x1, 0xc0, 0x70,
    0x1c, 0x7, 0x1, 0xc0, 0x70, 0x1c, 0x7, 0x1,
    0xc0, 0x70, 0x1c, 0x7, 0x1, 0xc0, 0x70,

    /* U+05DB "כ" */
    0xfc, 0x3f, 0xe0, 0x38, 0xe, 0x1, 0xc0, 0x70,
    0x1c, 0x7, 0x1, 0xc0, 0xe0, 0x3b, 0xfe, 0xfc,
    0x0,

    /* U+05DC "ל" */
    0x30, 0x38, 0xe, 0x3, 0x80, 0xe0, 0x3f, 0xef,
    0xfc, 0x7, 0x1, 0xc0, 0x70, 0x1c, 0x7, 0x3,
    0x80, 0xe0, 0x60, 0x18, 0xc, 0x3, 0x0,

    /* U+05DD "ם" */
    0xff, 0xff, 0xfd, 0xc3, 0xb8, 0x77, 0xe, 0xe1,
    0xdc, 0x3b, 0x87, 0x70, 0xee, 0x1d, 0xc3, 0xbf,
    0xf7, 0xfe,

    /* U+05DE "מ" */
    0xe7, 0x1d, 0xfb, 0xb7, 0xf6, 0xf7, 0x8e, 0xe1,
    0xdc, 0x3b, 0x87, 0xe0, 0xfc, 0x1f, 0x83, 0xf3,
    0xfe, 0x7e,

    /* U+05DF "ן" */
    0xff, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33,
    0x33,

    /* U+05E0 "נ" */
    0x3e, 0x7c, 0x38, 0x70, 0xe1, 0xc3, 0x87, 0xe,
    0x1c, 0x3f, 0xff, 0xe0,

    /* U+05E1 "ס" */
    0xff, 0xdf, 0xf8, 0x67, 0xc, 0xe7, 0xe, 0xe1,
    0xdc, 0x3b, 0x87, 0x70, 0xe2, 0x18, 0x43, 0xf,
    0xe0, 0xf0,

    /* U+05E2 "ע" */
    0xe3, 0xf8, 0x73, 0x1c, 0xc7, 0x31, 0xc6, 0x70,
    0xf8, 0x3e, 0x6, 0x3, 0x0, 0xc3, 0xf0, 0xf0,
    0x0,

    /* U+05E3 "ף" */
    0xff, 0xdf, 0xfd, 0xc3, 0xb8, 0x77, 0xe, 0xf9,
    0xdf, 0x38, 0x7, 0x0, 0xe0, 0x1c, 0x3, 0x80,
    0x70, 0xe, 0x1, 0xc0, 0x38, 0x7, 0x0, 0xe0,
    0x1c,

    /* U+05E4 "פ" */
    0xff, 0xdf, 0xfd, 0xc3, 0xb8, 0x77, 0xe, 0xf9,
    0xdf, 0x38, 0x7, 0x0, 0xe0, 0x1c, 0x3, 0xff,
    0xff, 0xfc,

    /* U+05E5 "ץ" */
    0xf1, 0xfe, 0x1d, 0xc7, 0x38, 0xe7, 0x1c, 0xe6,
    0x1d, 0x83, 0xb0, 0x7c, 0xf, 0x1, 0xe0, 0x38,
    0x7, 0x0, 0xe0, 0x1c, 0x3, 0x80, 0x70, 0xe,
    0x0,

    /* U+05E6 "צ" */
    0xe3, 0xf8, 0x73, 0x38, 0xce, 0x1b, 0x83, 0x80,
    0xc0, 0x18, 0x3, 0x80, 0x70, 0x1f, 0xff, 0xff,
    0xc0,

    /* U+05E7 "ק" */
    0xff, 0xdf, 0xfc, 0x3, 0x80, 0x70, 0xe, 0xe1,
    0xdc, 0x73, 0x8e, 0x73, 0xe, 0x61, 0xcc, 0x39,
    0x7, 0x20, 0xe0, 0x1c, 0x3, 0x80, 0x70, 0xe,
    0x0,

    /* U+05E8 "ר" */
    0xfe, 0x3f, 0xe0, 0x3c, 0xf, 0x1, 0xc0, 0x70,
    0x1c, 0x7, 0x1, 0xc0, 0x70, 0x1c, 0x7, 0x1,
    0xc0,

    /* U+05E9 "ש" */
    0xe6, 0x3e, 0x63, 0xe6, 0x3e, 0x63, 0xe6, 0x3e,
    0x63, 0xe6, 0x3e, 0x63, 0xe6, 0x3e, 0xce, 0xec,
    0xef, 0xfc, 0xff, 0x0,

    /* U+05EA "ת" */
    0x7f, 0xcf, 0xfc, 0x63, 0x8c, 0x71, 0x8e, 0x31,
    0xc6, 0x38, 0xc7, 0x18, 0xe3, 0x1c, 0x63, 0xfc,
    0x7f, 0x8e,

    /* U+05F0 "װ" */
    0xfb, 0xff, 0x7c, 0x63, 0x8c, 0x71, 0x8e, 0x31,
    0xc6, 0x38, 0xc7, 0x18, 0xe3, 0x1c, 0x63, 0x8c,
    0x71, 0x8e,

    /* U+05F1 "ױ" */
    0xfb, 0xff, 0x7c, 0x63, 0x8c, 0x71, 0x8e, 0x31,
    0xc6, 0x38, 0x7, 0x0, 0xe0, 0x1c, 0x3, 0x80,
    0x70, 0xe,

    /* U+05F2 "ײ" */
    0xfb, 0xff, 0x7c, 0x63, 0x8c, 0x71, 0x8e, 0x31,
    0xc6, 0x38,

    /* U+05F3 "׳" */
    0xe, 0x30, 0xc1, 0x86, 0x18, 0x0,

    /* U+05F4 "״" */
    0x6, 0x30, 0xce, 0x19, 0xc1, 0x9c, 0x73, 0xe,
    0x60
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 208, .box_w = 2, .box_h = 6, .ofs_x = 6, .ofs_y = -6},
    {.bitmap_index = 2, .adv_w = 208, .box_w = 11, .box_h = 6, .ofs_x = 1, .ofs_y = -6},
    {.bitmap_index = 11, .adv_w = 208, .box_w = 10, .box_h = 6, .ofs_x = 2, .ofs_y = -6},
    {.bitmap_index = 19, .adv_w = 208, .box_w = 11, .box_h = 6, .ofs_x = 1, .ofs_y = -6},
    {.bitmap_index = 28, .adv_w = 208, .box_w = 2, .box_h = 3, .ofs_x = 6, .ofs_y = -4},
    {.bitmap_index = 29, .adv_w = 208, .box_w = 7, .box_h = 3, .ofs_x = 4, .ofs_y = -4},
    {.bitmap_index = 32, .adv_w = 208, .box_w = 10, .box_h = 5, .ofs_x = 2, .ofs_y = -6},
    {.bitmap_index = 39, .adv_w = 208, .box_w = 7, .box_h = 3, .ofs_x = 4, .ofs_y = -5},
    {.bitmap_index = 42, .adv_w = 208, .box_w = 7, .box_h = 5, .ofs_x = 4, .ofs_y = -6},
    {.bitmap_index = 47, .adv_w = 208, .box_w = 2, .box_h = 3, .ofs_x = 5, .ofs_y = 15},
    {.bitmap_index = 48, .adv_w = 208, .box_w = 10, .box_h = 5, .ofs_x = 2, .ofs_y = -6},
    {.bitmap_index = 55, .adv_w = 208, .box_w = 2, .box_h = 2, .ofs_x = 5, .ofs_y = 6},
    {.bitmap_index = 56, .adv_w = 208, .box_w = 2, .box_h = 5, .ofs_x = 6, .ofs_y = -6},
    {.bitmap_index = 58, .adv_w = 208, .box_w = 11, .box_h = 2, .ofs_x = 1, .ofs_y = 11},
    {.bitmap_index = 61, .adv_w = 208, .box_w = 5, .box_h = 3, .ofs_x = 4, .ofs_y = 14},
    {.bitmap_index = 63, .adv_w = 208, .box_w = 2, .box_h = 13, .ofs_x = 6, .ofs_y = 0},
    {.bitmap_index = 67, .adv_w = 208, .box_w = 2, .box_h = 3, .ofs_x = 11, .ofs_y = 15},
    {.bitmap_index = 68, .adv_w = 208, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 15},
    {.bitmap_index = 70, .adv_w = 208, .box_w = 3, .box_h = 9, .ofs_x = 5, .ofs_y = 0},
    {.bitmap_index = 74, .adv_w = 208, .box_w = 2, .box_h = 3, .ofs_x = 6, .ofs_y = 15},
    {.bitmap_index = 75, .adv_w = 208, .box_w = 11, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 93, .adv_w = 208, .box_w = 10, .box_h = 13, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 110, .adv_w = 208, .box_w = 9, .box_h = 13, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 125, .adv_w = 208, .box_w = 11, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 143, .adv_w = 208, .box_w = 11, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 161, .adv_w = 208, .box_w = 4, .box_h = 13, .ofs_x = 4, .ofs_y = 0},
    {.bitmap_index = 168, .adv_w = 208, .box_w = 10, .box_h = 13, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 185, .adv_w = 208, .box_w = 11, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 203, .adv_w = 208, .box_w = 10, .box_h = 13, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 220, .adv_w = 208, .box_w = 4, .box_h = 7, .ofs_x = 4, .ofs_y = 6},
    {.bitmap_index = 224, .adv_w = 208, .box_w = 10, .box_h = 18, .ofs_x = 2, .ofs_y = -5},
    {.bitmap_index = 247, .adv_w = 208, .box_w = 10, .box_h = 13, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 264, .adv_w = 208, .box_w = 10, .box_h = 18, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 287, .adv_w = 208, .box_w = 11, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 305, .adv_w = 208, .box_w = 11, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 323, .adv_w = 208, .box_w = 4, .box_h = 18, .ofs_x = 4, .ofs_y = -5},
    {.bitmap_index = 332, .adv_w = 208, .box_w = 7, .box_h = 13, .ofs_x = 4, .ofs_y = 0},
    {.bitmap_index = 344, .adv_w = 208, .box_w = 11, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 362, .adv_w = 208, .box_w = 10, .box_h = 13, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 379, .adv_w = 208, .box_w = 11, .box_h = 18, .ofs_x = 1, .ofs_y = -5},
    {.bitmap_index = 404, .adv_w = 208, .box_w = 11, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 422, .adv_w = 208, .box_w = 11, .box_h = 18, .ofs_x = 1, .ofs_y = -5},
    {.bitmap_index = 447, .adv_w = 208, .box_w = 10, .box_h = 13, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 464, .adv_w = 208, .box_w = 11, .box_h = 18, .ofs_x = 1, .ofs_y = -5},
    {.bitmap_index = 489, .adv_w = 208, .box_w = 10, .box_h = 13, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 506, .adv_w = 208, .box_w = 12, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 526, .adv_w = 208, .box_w = 11, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 544, .adv_w = 208, .box_w = 11, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 562, .adv_w = 208, .box_w = 11, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 580, .adv_w = 208, .box_w = 11, .box_h = 7, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 590, .adv_w = 208, .box_w = 7, .box_h = 6, .ofs_x = 4, .ofs_y = 7},
    {.bitmap_index = 596, .adv_w = 208, .box_w = 12, .box_h = 6, .ofs_x = 1, .ofs_y = 7}
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

extern const lv_font_t greybeard_26_he_next;


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
    .line_height = 24,          /*The maximum line height required by the font*/
    .base_line = 6,             /*Baseline measured from the bottom of the line*/
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

