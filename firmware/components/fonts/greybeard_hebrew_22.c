/*******************************************************************************
 * Size: 22 px
 * Bpp: 1
 * Opts: --font Greybeard-22px.ttf -r 0x590-0x5FF --size 22 --bpp 1 --format lvgl --no-compress --lv-fallback greybeard_22_he_next --lv-font-name greybeard_hebrew_22 -o /tmp/gen/greybeard_hebrew_22.c
 ******************************************************************************/

#include "lvgl.h"

#ifndef GREYBEARD_HEBREW_22
#define GREYBEARD_HEBREW_22 1
#endif

#if GREYBEARD_HEBREW_22

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+05B0 "ְ" */
    0xf3, 0xc0,

    /* U+05B1 "ֱ" */
    0x1, 0xe6, 0xf3, 0x6, 0x33, 0x18,

    /* U+05B2 "ֲ" */
    0x3, 0x3, 0xf8, 0xfb, 0x3,

    /* U+05B3 "ֳ" */
    0x1, 0xfe, 0xff, 0x6, 0x33, 0x18,

    /* U+05B4 "ִ" */
    0xf0,

    /* U+05B5 "ֵ" */
    0xcf, 0x30,

    /* U+05B6 "ֶ" */
    0xc3, 0xc3, 0x18, 0x18,

    /* U+05B7 "ַ" */
    0xff, 0xf0,

    /* U+05B8 "ָ" */
    0xff, 0xf3, 0xc,

    /* U+05B9 "ֹ" */
    0xf0,

    /* U+05BB "ֻ" */
    0xc0, 0xd8, 0x1b, 0x3,

    /* U+05BC "ּ" */
    0xf0,

    /* U+05BD "ֽ" */
    0xff,

    /* U+05BE "־" */
    0xff, 0xff, 0xc0,

    /* U+05BF "ֿ" */
    0xff, 0xc0,

    /* U+05C0 "׀" */
    0xff, 0xff, 0xfc,

    /* U+05C1 "ׁ" */
    0xf0,

    /* U+05C2 "ׂ" */
    0xf0,

    /* U+05C3 "׃" */
    0x5d, 0x0, 0xba,

    /* U+05C4 "ׄ" */
    0xf0,

    /* U+05D0 "א" */
    0x86, 0x63, 0xb8, 0xee, 0x77, 0xa2, 0xf3, 0x39,
    0x8e, 0xc3, 0xf0, 0xf8, 0x20,

    /* U+05D1 "ב" */
    0xfc, 0xfe, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6,
    0x6, 0xff, 0xff,

    /* U+05D2 "ג" */
    0x38, 0x78, 0x30, 0x60, 0xc1, 0x87, 0x1f, 0x77,
    0xcf, 0x18,

    /* U+05D3 "ד" */
    0xff, 0xff, 0xc0, 0xc0, 0x60, 0x30, 0x18, 0xc,
    0x6, 0x3, 0x1, 0x80, 0xc0,

    /* U+05D4 "ה" */
    0xff, 0xff, 0xc0, 0x60, 0x36, 0x1b, 0xd, 0x86,
    0xc3, 0x61, 0xb0, 0xd8, 0x60,

    /* U+05D5 "ו" */
    0xff, 0x33, 0x33, 0x33, 0x33, 0x30,

    /* U+05D6 "ז" */
    0x7f, 0xfe, 0x8, 0x18, 0x18, 0x18, 0x18, 0x18,
    0x18, 0x18, 0x18,

    /* U+05D7 "ח" */
    0xff, 0xff, 0xd8, 0x6c, 0x36, 0x1b, 0xd, 0x86,
    0xc3, 0x61, 0xb0, 0xd8, 0x60,

    /* U+05D8 "ט" */
    0xc6, 0xcf, 0xcb, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3,
    0xc6, 0xfe, 0xf8,

    /* U+05D9 "י" */
    0xff, 0x33, 0x33,

    /* U+05DA "ך" */
    0xff, 0xff, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
    0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,

    /* U+05DB "כ" */
    0xf8, 0xfe, 0x6, 0x3, 0x3, 0x3, 0x3, 0x3,
    0x6, 0xfe, 0xf8,

    /* U+05DC "ל" */
    0x60, 0xc0, 0xc0, 0xc0, 0xfe, 0xff, 0x3, 0x3,
    0x3, 0x3, 0x6, 0x6, 0xc, 0x18, 0x18,

    /* U+05DD "ם" */
    0xff, 0xff, 0xd8, 0x6c, 0x36, 0x1b, 0xd, 0x86,
    0xc3, 0x61, 0xbf, 0xdf, 0xe0,

    /* U+05DE "מ" */
    0xce, 0x6f, 0xb6, 0xee, 0x36, 0x1b, 0xd, 0x87,
    0x83, 0xc1, 0xe7, 0xf3, 0xe0,

    /* U+05DF "ן" */
    0xff, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x30,

    /* U+05E0 "נ" */
    0x3c, 0xf0, 0xc3, 0xc, 0x30, 0xc3, 0xf, 0xff,
    0xc0,

    /* U+05E1 "ס" */
    0xff, 0x7f, 0x8c, 0xcc, 0x36, 0x1b, 0xd, 0x86,
    0xc3, 0x21, 0x1f, 0x87, 0x80,

    /* U+05E2 "ע" */
    0xc7, 0xc3, 0x63, 0x63, 0x33, 0x1e, 0x1e, 0xc,
    0x18, 0xf8, 0xe0,

    /* U+05E3 "ף" */
    0xff, 0x7f, 0xd8, 0x6c, 0x37, 0x9b, 0xcc, 0x6,
    0x3, 0x1, 0x80, 0xc0, 0x60, 0x30, 0x18, 0xc,
    0x6,

    /* U+05E4 "פ" */
    0xff, 0x7f, 0xd8, 0x6c, 0x37, 0x9b, 0xcc, 0x6,
    0x3, 0x1, 0xff, 0xff, 0xc0,

    /* U+05E5 "ץ" */
    0xe3, 0xf0, 0xd8, 0xcc, 0x66, 0x63, 0x61, 0xb0,
    0xf0, 0x70, 0x30, 0x18, 0xc, 0x6, 0x3, 0x1,
    0x80,

    /* U+05E6 "צ" */
    0xc7, 0xc3, 0x66, 0x36, 0x1c, 0x18, 0xc, 0x6,
    0x3, 0xff, 0xff,

    /* U+05E7 "ק" */
    0xff, 0x7f, 0xc0, 0x60, 0x36, 0x1b, 0x19, 0x8c,
    0xcc, 0x66, 0x32, 0x19, 0xc, 0x6, 0x3, 0x1,
    0x80,

    /* U+05E8 "ר" */
    0xfc, 0xfe, 0x7, 0x3, 0x3, 0x3, 0x3, 0x3,
    0x3, 0x3, 0x3,

    /* U+05E9 "ש" */
    0xcc, 0xf3, 0x3c, 0xcf, 0x33, 0xcc, 0xf3, 0x3c,
    0xcf, 0x33, 0xd9, 0xbf, 0xcf, 0xe0,

    /* U+05EA "ת" */
    0x7f, 0x3f, 0xcc, 0x66, 0x33, 0x19, 0x8c, 0xc6,
    0x63, 0x31, 0xf8, 0xfc, 0x60,

    /* U+05F0 "װ" */
    0xf7, 0xfb, 0xcc, 0x66, 0x33, 0x19, 0x8c, 0xc6,
    0x63, 0x31, 0x98, 0xcc, 0x60,

    /* U+05F1 "ױ" */
    0xf7, 0xfb, 0xcc, 0x66, 0x33, 0x19, 0x8c, 0x6,
    0x3, 0x1, 0x80, 0xc0, 0x60,

    /* U+05F2 "ײ" */
    0xf7, 0xfb, 0xcc, 0x66, 0x33, 0x19, 0x8c,

    /* U+05F3 "׳" */
    0xc, 0x63, 0x18, 0xc0,

    /* U+05F4 "״" */
    0xc, 0xc6, 0x63, 0x31, 0x98, 0xcc, 0x0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 176, .box_w = 2, .box_h = 5, .ofs_x = 5, .ofs_y = -5},
    {.bitmap_index = 2, .adv_w = 176, .box_w = 9, .box_h = 5, .ofs_x = 1, .ofs_y = -5},
    {.bitmap_index = 8, .adv_w = 176, .box_w = 8, .box_h = 5, .ofs_x = 2, .ofs_y = -5},
    {.bitmap_index = 13, .adv_w = 176, .box_w = 9, .box_h = 5, .ofs_x = 1, .ofs_y = -5},
    {.bitmap_index = 19, .adv_w = 176, .box_w = 2, .box_h = 2, .ofs_x = 5, .ofs_y = -3},
    {.bitmap_index = 20, .adv_w = 176, .box_w = 6, .box_h = 2, .ofs_x = 3, .ofs_y = -3},
    {.bitmap_index = 22, .adv_w = 176, .box_w = 8, .box_h = 4, .ofs_x = 2, .ofs_y = -5},
    {.bitmap_index = 26, .adv_w = 176, .box_w = 6, .box_h = 2, .ofs_x = 3, .ofs_y = -4},
    {.bitmap_index = 28, .adv_w = 176, .box_w = 6, .box_h = 4, .ofs_x = 3, .ofs_y = -5},
    {.bitmap_index = 31, .adv_w = 176, .box_w = 2, .box_h = 2, .ofs_x = 4, .ofs_y = 13},
    {.bitmap_index = 32, .adv_w = 176, .box_w = 8, .box_h = 4, .ofs_x = 2, .ofs_y = -5},
    {.bitmap_index = 36, .adv_w = 176, .box_w = 2, .box_h = 2, .ofs_x = 4, .ofs_y = 5},
    {.bitmap_index = 37, .adv_w = 176, .box_w = 2, .box_h = 4, .ofs_x = 5, .ofs_y = -5},
    {.bitmap_index = 38, .adv_w = 176, .box_w = 9, .box_h = 2, .ofs_x = 1, .ofs_y = 9},
    {.bitmap_index = 41, .adv_w = 176, .box_w = 5, .box_h = 2, .ofs_x = 3, .ofs_y = 12},
    {.bitmap_index = 43, .adv_w = 176, .box_w = 2, .box_h = 11, .ofs_x = 5, .ofs_y = 0},
    {.bitmap_index = 46, .adv_w = 176, .box_w = 2, .box_h = 2, .ofs_x = 9, .ofs_y = 13},
    {.bitmap_index = 47, .adv_w = 176, .box_w = 2, .box_h = 2, .ofs_x = 1, .ofs_y = 13},
    {.bitmap_index = 48, .adv_w = 176, .box_w = 3, .box_h = 8, .ofs_x = 4, .ofs_y = 0},
    {.bitmap_index = 51, .adv_w = 176, .box_w = 2, .box_h = 2, .ofs_x = 5, .ofs_y = 13},
    {.bitmap_index = 52, .adv_w = 176, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 65, .adv_w = 176, .box_w = 8, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 76, .adv_w = 176, .box_w = 7, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 86, .adv_w = 176, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 99, .adv_w = 176, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 112, .adv_w = 176, .box_w = 4, .box_h = 11, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 118, .adv_w = 176, .box_w = 8, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 129, .adv_w = 176, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 142, .adv_w = 176, .box_w = 8, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 153, .adv_w = 176, .box_w = 4, .box_h = 6, .ofs_x = 3, .ofs_y = 5},
    {.bitmap_index = 156, .adv_w = 176, .box_w = 8, .box_h = 15, .ofs_x = 2, .ofs_y = -4},
    {.bitmap_index = 171, .adv_w = 176, .box_w = 8, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 182, .adv_w = 176, .box_w = 8, .box_h = 15, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 197, .adv_w = 176, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 210, .adv_w = 176, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 223, .adv_w = 176, .box_w = 4, .box_h = 15, .ofs_x = 3, .ofs_y = -4},
    {.bitmap_index = 231, .adv_w = 176, .box_w = 6, .box_h = 11, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 240, .adv_w = 176, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 253, .adv_w = 176, .box_w = 8, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 264, .adv_w = 176, .box_w = 9, .box_h = 15, .ofs_x = 1, .ofs_y = -4},
    {.bitmap_index = 281, .adv_w = 176, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 294, .adv_w = 176, .box_w = 9, .box_h = 15, .ofs_x = 1, .ofs_y = -4},
    {.bitmap_index = 311, .adv_w = 176, .box_w = 8, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 322, .adv_w = 176, .box_w = 9, .box_h = 15, .ofs_x = 1, .ofs_y = -4},
    {.bitmap_index = 339, .adv_w = 176, .box_w = 8, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 350, .adv_w = 176, .box_w = 10, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 364, .adv_w = 176, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 377, .adv_w = 176, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 390, .adv_w = 176, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 403, .adv_w = 176, .box_w = 9, .box_h = 6, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 410, .adv_w = 176, .box_w = 6, .box_h = 5, .ofs_x = 3, .ofs_y = 6},
    {.bitmap_index = 414, .adv_w = 176, .box_w = 10, .box_h = 5, .ofs_x = 1, .ofs_y = 6}
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

extern lv_font_t greybeard_22_he_next;


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t greybeard_hebrew_22 = {
#else
lv_font_t greybeard_hebrew_22 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 20,          /*The maximum line height required by the font*/
    .base_line = 5,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = &greybeard_22_he_next,
#endif
    .user_data = NULL,
};



#endif /*#if GREYBEARD_HEBREW_22*/

