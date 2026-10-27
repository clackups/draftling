/*******************************************************************************
 * Size: 11 px
 * Bpp: 1
 * Opts: --font Greybeard-11px.ttf -r 0x102-0x103,0x110-0x111,0x128-0x129,0x168-0x169,0x1A0-0x1A1,0x1AF-0x1B0,0x1EA0-0x1EF9,0x2013-0x2014,0x2018-0x201D,0x2022,0x2030,0x20AB --size 11 --bpp 1 --format lvgl --no-compress --lv-fallback greybeard_11_lx_next --lv-font-name greybeard_latinx_11 -o ./greybeard_latinx_11.c
 ******************************************************************************/

#include "lvgl.h"

#ifndef GREYBEARD_LATINX_11
#define GREYBEARD_LATINX_11 1
#endif

#if GREYBEARD_LATINX_11

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0102 "Ă" */
    0x96, 0x66, 0x9f, 0x99, 0x90,

    /* U+0103 "ă" */
    0x96, 0x6, 0x17, 0x99, 0x70,

    /* U+0110 "Đ" */
    0x62, 0x93, 0xd4, 0xa5, 0x4c,

    /* U+0111 "đ" */
    0x13, 0xc4, 0xe9, 0x4a, 0x4e,

    /* U+0128 "Ĩ" */
    0x5a, 0x72, 0x22, 0x22, 0x70,

    /* U+0129 "ĩ" */
    0x5a, 0x6, 0x22, 0x22, 0x70,

    /* U+0168 "Ũ" */
    0x5a, 0x9, 0x99, 0x99, 0x60,

    /* U+0169 "ũ" */
    0x5a, 0x9, 0x99, 0x99, 0x70,

    /* U+01A0 "Ơ" */
    0xb, 0x65, 0x29, 0x4a, 0x52, 0x60,

    /* U+01A1 "ơ" */
    0xb, 0x65, 0x29, 0x49, 0x80,

    /* U+01AF "Ư" */
    0xc, 0xe5, 0x29, 0x4a, 0x52, 0x60,

    /* U+01B0 "ư" */
    0xc, 0xe5, 0x29, 0x49, 0xc0,

    /* U+1EA0 "Ạ" */
    0x66, 0x99, 0xf9, 0x99, 0x6,

    /* U+1EA1 "ạ" */
    0x61, 0x79, 0x97, 0x6,

    /* U+1EA2 "Ả" */
    0x62, 0x46, 0x69, 0xf9, 0x90,

    /* U+1EA3 "ả" */
    0x62, 0x46, 0x17, 0x99, 0x70,

    /* U+1EA4 "Ấ" */
    0xa, 0xa8, 0xc6, 0x4b, 0xd2, 0x90,

    /* U+1EA5 "ấ" */
    0xa, 0xa8, 0x6, 0x9, 0xd2, 0x70,

    /* U+1EA6 "Ầ" */
    0x82, 0x8a, 0x63, 0x25, 0xe9, 0x48,

    /* U+1EA7 "ầ" */
    0x82, 0x8a, 0x3, 0x4, 0xe9, 0x38,

    /* U+1EA8 "Ẩ" */
    0x1a, 0x6c, 0xc6, 0x4b, 0xd2, 0x90,

    /* U+1EA9 "ẩ" */
    0x1a, 0x6c, 0x6, 0x9, 0xd2, 0x70,

    /* U+1EAA "Ẫ" */
    0x2a, 0xa8, 0xc6, 0x4b, 0xd2, 0x90,

    /* U+1EAB "ẫ" */
    0x2a, 0x91, 0x46, 0x9, 0xd2, 0x70,

    /* U+1EAC "Ậ" */
    0x25, 0x66, 0x9f, 0x99, 0x90, 0x60,

    /* U+1EAD "ậ" */
    0x25, 0x6, 0x17, 0x99, 0x70, 0x60,

    /* U+1EAE "Ắ" */
    0x11, 0x22, 0xe6, 0x4b, 0xd2, 0x90,

    /* U+1EAF "ắ" */
    0x11, 0x22, 0xe6, 0x9, 0xd2, 0x70,

    /* U+1EB0 "Ằ" */
    0x41, 0x22, 0xe6, 0x4b, 0xd2, 0x90,

    /* U+1EB1 "ằ" */
    0x41, 0x22, 0xe6, 0x9, 0xd2, 0x70,

    /* U+1EB2 "Ẳ" */
    0x30, 0xaa, 0xe6, 0x4b, 0xd2, 0x90,

    /* U+1EB3 "ẳ" */
    0x30, 0xaa, 0xe6, 0x9, 0xd2, 0x70,

    /* U+1EB4 "Ẵ" */
    0x55, 0x22, 0xe6, 0x4b, 0xd2, 0x90,

    /* U+1EB5 "ẵ" */
    0x55, 0x22, 0xe6, 0x9, 0xd2, 0x70,

    /* U+1EB6 "Ặ" */
    0x96, 0x66, 0x9f, 0x99, 0x90, 0x60,

    /* U+1EB7 "ặ" */
    0x96, 0x6, 0x17, 0x99, 0x70, 0x60,

    /* U+1EB8 "Ẹ" */
    0xf8, 0x8e, 0x88, 0x8f, 0x6,

    /* U+1EB9 "ẹ" */
    0x69, 0xf8, 0x87, 0x6,

    /* U+1EBA "Ẻ" */
    0x62, 0x4f, 0x8e, 0x88, 0xf0,

    /* U+1EBB "ẻ" */
    0x62, 0x46, 0x9f, 0x88, 0x70,

    /* U+1EBC "Ẽ" */
    0x5a, 0xf8, 0x8e, 0x88, 0xf0,

    /* U+1EBD "ẽ" */
    0x5a, 0x6, 0x9f, 0x88, 0x70,

    /* U+1EBE "Ế" */
    0xa, 0xa9, 0xe8, 0x72, 0x10, 0xf0,

    /* U+1EBF "ế" */
    0xa, 0xa8, 0x6, 0x4b, 0xd0, 0x70,

    /* U+1EC0 "Ề" */
    0x82, 0x8a, 0xf4, 0x39, 0x8, 0x78,

    /* U+1EC1 "ề" */
    0x82, 0x8a, 0x3, 0x25, 0xe8, 0x38,

    /* U+1EC2 "Ể" */
    0x1a, 0x6d, 0xe8, 0x72, 0x10, 0xf0,

    /* U+1EC3 "ể" */
    0x1a, 0x6c, 0x6, 0x4b, 0xd0, 0x70,

    /* U+1EC4 "Ễ" */
    0x2a, 0xa9, 0xe8, 0x72, 0x10, 0xf0,

    /* U+1EC5 "ễ" */
    0x2a, 0x91, 0x46, 0x4b, 0xd0, 0x70,

    /* U+1EC6 "Ệ" */
    0x25, 0xf8, 0x8e, 0x88, 0xf0, 0x60,

    /* U+1EC7 "ệ" */
    0x25, 0x6, 0x9f, 0x88, 0x70, 0x60,

    /* U+1EC8 "Ỉ" */
    0x65, 0x74, 0x92, 0xe0,

    /* U+1EC9 "ỉ" */
    0x65, 0xc, 0x92, 0xe0,

    /* U+1ECA "Ị" */
    0xe9, 0x24, 0x97, 0x18,

    /* U+1ECB "ị" */
    0x48, 0x64, 0x92, 0xe3, 0x0,

    /* U+1ECC "Ọ" */
    0x69, 0x99, 0x99, 0x96, 0x6,

    /* U+1ECD "ọ" */
    0x69, 0x99, 0x96, 0x6,

    /* U+1ECE "Ỏ" */
    0x62, 0x69, 0x99, 0x99, 0x60,

    /* U+1ECF "ỏ" */
    0x62, 0x46, 0x99, 0x99, 0x60,

    /* U+1ED0 "Ố" */
    0xa, 0xa8, 0xc9, 0x4a, 0x52, 0x60,

    /* U+1ED1 "ố" */
    0xa, 0xa8, 0x6, 0x4a, 0x52, 0x60,

    /* U+1ED2 "Ồ" */
    0x82, 0x8a, 0x64, 0xa5, 0x29, 0x30,

    /* U+1ED3 "ồ" */
    0x82, 0x8a, 0x3, 0x25, 0x29, 0x30,

    /* U+1ED4 "Ổ" */
    0x1a, 0x6c, 0xc9, 0x4a, 0x52, 0x60,

    /* U+1ED5 "ổ" */
    0x1a, 0x6c, 0x6, 0x4a, 0x52, 0x60,

    /* U+1ED6 "Ỗ" */
    0x2a, 0xa8, 0xc9, 0x4a, 0x52, 0x60,

    /* U+1ED7 "ỗ" */
    0x2a, 0x91, 0x46, 0x4a, 0x52, 0x60,

    /* U+1ED8 "Ộ" */
    0x25, 0x69, 0x99, 0x99, 0x60, 0x60,

    /* U+1ED9 "ộ" */
    0x25, 0x6, 0x99, 0x99, 0x60, 0x60,

    /* U+1EDA "Ớ" */
    0x22, 0x5b, 0x29, 0x4a, 0x52, 0x60,

    /* U+1EDB "ớ" */
    0x22, 0x2, 0xd9, 0x4a, 0x52, 0x60,

    /* U+1EDC "Ờ" */
    0x41, 0x5b, 0x29, 0x4a, 0x52, 0x60,

    /* U+1EDD "ờ" */
    0x41, 0x2, 0xd9, 0x4a, 0x52, 0x60,

    /* U+1EDE "Ở" */
    0x61, 0x5b, 0x29, 0x4a, 0x52, 0x60,

    /* U+1EDF "ở" */
    0x61, 0x12, 0xd9, 0x4a, 0x52, 0x60,

    /* U+1EE0 "Ỡ" */
    0x55, 0x5b, 0x29, 0x4a, 0x52, 0x60,

    /* U+1EE1 "ỡ" */
    0x55, 0x2, 0xd9, 0x4a, 0x52, 0x60,

    /* U+1EE2 "Ợ" */
    0xb, 0x65, 0x29, 0x4a, 0x52, 0x60, 0x18,

    /* U+1EE3 "ợ" */
    0xb, 0x65, 0x29, 0x49, 0x80, 0x60,

    /* U+1EE4 "Ụ" */
    0x99, 0x99, 0x99, 0x96, 0x6,

    /* U+1EE5 "ụ" */
    0x99, 0x99, 0x97, 0x6,

    /* U+1EE6 "Ủ" */
    0x62, 0x99, 0x99, 0x99, 0x60,

    /* U+1EE7 "ủ" */
    0x62, 0x49, 0x99, 0x99, 0x70,

    /* U+1EE8 "Ứ" */
    0x22, 0x67, 0x29, 0x4a, 0x52, 0x60,

    /* U+1EE9 "ứ" */
    0x22, 0x3, 0x39, 0x4a, 0x52, 0x70,

    /* U+1EEA "Ừ" */
    0x41, 0x67, 0x29, 0x4a, 0x52, 0x60,

    /* U+1EEB "ừ" */
    0x41, 0x3, 0x39, 0x4a, 0x52, 0x70,

    /* U+1EEC "Ử" */
    0x61, 0x67, 0x29, 0x4a, 0x52, 0x60,

    /* U+1EED "ử" */
    0x61, 0x13, 0x39, 0x4a, 0x52, 0x70,

    /* U+1EEE "Ữ" */
    0x55, 0x67, 0x29, 0x4a, 0x52, 0x60,

    /* U+1EEF "ữ" */
    0x55, 0x3, 0x39, 0x4a, 0x52, 0x70,

    /* U+1EF0 "Ự" */
    0xc, 0xe5, 0x29, 0x4a, 0x52, 0x60, 0x18,

    /* U+1EF1 "ự" */
    0xc, 0xe5, 0x29, 0x49, 0xc0, 0x60,

    /* U+1EF2 "Ỳ" */
    0x41, 0x23, 0x15, 0x10, 0x84, 0x20,

    /* U+1EF3 "ỳ" */
    0x42, 0x9, 0x99, 0x56, 0x24, 0xc0,

    /* U+1EF4 "Ỵ" */
    0x8c, 0x54, 0xa2, 0x10, 0x84, 0x3, 0x0,

    /* U+1EF5 "ỵ" */
    0x99, 0x96, 0x24, 0x99,

    /* U+1EF6 "Ỷ" */
    0x61, 0x23, 0x15, 0x10, 0x84, 0x20,

    /* U+1EF7 "ỷ" */
    0x62, 0x49, 0x99, 0x56, 0x24, 0xc0,

    /* U+1EF8 "Ỹ" */
    0x2a, 0xa3, 0x15, 0x10, 0x84, 0x20,

    /* U+1EF9 "ỹ" */
    0x5a, 0x9, 0x99, 0x56, 0x24, 0xc0,

    /* U+2013 "–" */
    0xf8,

    /* U+2014 "—" */
    0xfc,

    /* U+2018 "‘" */
    0x6c,

    /* U+2019 "’" */
    0xd8,

    /* U+201A "‚" */
    0xd8,

    /* U+201B "‛" */
    0xe4,

    /* U+201C "“" */
    0x4c, 0xb6,

    /* U+201D "”" */
    0xda, 0x64,

    /* U+2022 "•" */
    0x5f, 0xa0,

    /* U+2030 "‰" */
    0x45, 0x10, 0x36, 0x41, 0x55, 0x50,

    /* U+20AB "₫" */
    0x13, 0xc4, 0xe9, 0x4a, 0x4e, 0x7, 0x80
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 5, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 10, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 15, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 20, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 25, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 30, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 35, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 40, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 46, .adv_w = 96, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 51, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 57, .adv_w = 96, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 62, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 67, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 71, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 76, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 81, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 87, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 93, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 99, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 105, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 111, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 117, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 123, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 129, .adv_w = 96, .box_w = 4, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 135, .adv_w = 96, .box_w = 4, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 141, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 147, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 153, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 159, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 165, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 171, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 177, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 183, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 189, .adv_w = 96, .box_w = 4, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 195, .adv_w = 96, .box_w = 4, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 201, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 206, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 210, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 215, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 220, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 225, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 230, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 236, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 242, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 248, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 254, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 260, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 266, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 272, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 278, .adv_w = 96, .box_w = 4, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 284, .adv_w = 96, .box_w = 4, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 290, .adv_w = 96, .box_w = 3, .box_h = 9, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 294, .adv_w = 96, .box_w = 3, .box_h = 9, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 298, .adv_w = 96, .box_w = 3, .box_h = 10, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 302, .adv_w = 96, .box_w = 3, .box_h = 11, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 307, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 312, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 316, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 321, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 326, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 332, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 338, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 344, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 350, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 356, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 362, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 368, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 374, .adv_w = 96, .box_w = 4, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 380, .adv_w = 96, .box_w = 4, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 386, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 392, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 398, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 404, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 410, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 416, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 422, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 428, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 434, .adv_w = 96, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 441, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 447, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 452, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 456, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 461, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 466, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 472, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 478, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 484, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 490, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 496, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 502, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 508, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 514, .adv_w = 96, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 521, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 527, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 533, .adv_w = 96, .box_w = 4, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 539, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 546, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 550, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 556, .adv_w = 96, .box_w = 4, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 562, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 568, .adv_w = 96, .box_w = 4, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 574, .adv_w = 96, .box_w = 5, .box_h = 1, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 575, .adv_w = 96, .box_w = 6, .box_h = 1, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 576, .adv_w = 96, .box_w = 2, .box_h = 3, .ofs_x = 2, .ofs_y = 5},
    {.bitmap_index = 577, .adv_w = 96, .box_w = 2, .box_h = 3, .ofs_x = 2, .ofs_y = 5},
    {.bitmap_index = 578, .adv_w = 96, .box_w = 2, .box_h = 3, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 579, .adv_w = 96, .box_w = 2, .box_h = 3, .ofs_x = 2, .ofs_y = 5},
    {.bitmap_index = 580, .adv_w = 96, .box_w = 5, .box_h = 3, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 582, .adv_w = 96, .box_w = 5, .box_h = 3, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 584, .adv_w = 96, .box_w = 3, .box_h = 4, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 586, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 592, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_0[] = {
    0x0, 0x1, 0xe, 0xf, 0x26, 0x27, 0x66, 0x67,
    0x9e, 0x9f, 0xad, 0xae
};

static const uint16_t unicode_list_2[] = {
    0x0, 0x1, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa,
    0xf, 0x1d, 0x98
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 258, .range_length = 175, .glyph_id_start = 1,
        .unicode_list = unicode_list_0, .glyph_id_ofs_list = NULL, .list_length = 12, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    },
    {
        .range_start = 7840, .range_length = 90, .glyph_id_start = 13,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 8211, .range_length = 153, .glyph_id_start = 103,
        .unicode_list = unicode_list_2, .glyph_id_ofs_list = NULL, .list_length = 11, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
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
    .cmap_num = 3,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};

extern const lv_font_t greybeard_11_lx_next;


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t greybeard_latinx_11 = {
#else
lv_font_t greybeard_latinx_11 = {
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
    .fallback = &greybeard_11_lx_next,
#endif
    .user_data = NULL,
};



#endif /*#if GREYBEARD_LATINX_11*/

