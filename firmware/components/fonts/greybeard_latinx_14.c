/*******************************************************************************
 * Size: 14 px
 * Bpp: 1
 * Opts: --font Greybeard-14px.ttf -r 0x102-0x103,0x110-0x111,0x128-0x129,0x168-0x169,0x1A0-0x1A1,0x1AF-0x1B0,0x1EA0-0x1EF9,0x2013-0x2014,0x2018-0x201D,0x2022,0x2030,0x20AB --size 14 --bpp 1 --format lvgl --no-compress --lv-fallback greybeard_14_lx_next --lv-font-name greybeard_latinx_14 -o ./greybeard_latinx_14.c
 ******************************************************************************/

#include "lvgl.h"

#ifndef GREYBEARD_LATINX_14
#define GREYBEARD_LATINX_14 1
#endif

#if GREYBEARD_LATINX_14

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0102 "Ă" */
    0x48, 0xc0, 0xc, 0x31, 0x24, 0x9e, 0x86, 0x18,
    0x40,

    /* U+0103 "ă" */
    0x49, 0x80, 0xe0, 0x85, 0xf1, 0x8b, 0xc0,

    /* U+0110 "Đ" */
    0x71, 0x24, 0x51, 0xe5, 0x14, 0x52, 0x70,

    /* U+0111 "đ" */
    0x8, 0xf0, 0x9a, 0x9a, 0x28, 0xa2, 0x99, 0xa0,

    /* U+0128 "Ĩ" */
    0x4d, 0x81, 0xf2, 0x10, 0x84, 0x21, 0x3e,

    /* U+0129 "ĩ" */
    0x4d, 0x80, 0xc2, 0x10, 0x84, 0x27, 0xc0,

    /* U+0168 "Ũ" */
    0x4d, 0x81, 0x18, 0xc6, 0x31, 0x8c, 0x5c,

    /* U+0169 "ũ" */
    0x4d, 0x81, 0x18, 0xc6, 0x31, 0x9b, 0x40,

    /* U+01A0 "Ơ" */
    0xc, 0x17, 0xa2, 0x8a, 0x28, 0xa2, 0x8a, 0x27,
    0x0,

    /* U+01A1 "ơ" */
    0xc, 0x17, 0xa2, 0x8a, 0x28, 0xa2, 0x70,

    /* U+01AF "Ư" */
    0xc, 0x18, 0xa2, 0x8a, 0x28, 0xa2, 0x8a, 0x27,
    0x0,

    /* U+01B0 "ư" */
    0xc, 0x18, 0xa2, 0x8a, 0x28, 0xa6, 0x68,

    /* U+1EA0 "Ạ" */
    0x30, 0xc3, 0x12, 0x49, 0xe8, 0x61, 0x84, 0x1,
    0x4,

    /* U+1EA1 "ạ" */
    0x70, 0x42, 0xf8, 0xc5, 0xe0, 0x21, 0x0,

    /* U+1EA2 "Ả" */
    0x30, 0x21, 0xc, 0x31, 0x24, 0x9e, 0x86, 0x18,
    0x40,

    /* U+1EA3 "ả" */
    0x60, 0x88, 0x7, 0x4, 0x2f, 0x8c, 0x5e,

    /* U+1EA4 "Ấ" */
    0x22, 0xa8, 0x1, 0x83, 0x9, 0x12, 0x3c, 0x85,
    0xa, 0x10,

    /* U+1EA5 "ấ" */
    0x5, 0x2a, 0x0, 0x70, 0x20, 0x9e, 0x8a, 0x27,
    0x80,

    /* U+1EA6 "Ầ" */
    0x89, 0x50, 0xc, 0x31, 0x24, 0x9e, 0x86, 0x18,
    0x40,

    /* U+1EA7 "ầ" */
    0x81, 0x21, 0x40, 0x38, 0x10, 0x4f, 0x45, 0x13,
    0xc0,

    /* U+1EA8 "Ẩ" */
    0xc, 0x45, 0x50, 0x3, 0x6, 0x12, 0x24, 0x79,
    0xa, 0x10,

    /* U+1EA9 "ẩ" */
    0x19, 0x1a, 0x80, 0x70, 0x20, 0x9e, 0x8a, 0x27,
    0x80,

    /* U+1EAA "Ẫ" */
    0x66, 0x62, 0x80, 0x30, 0xc4, 0x92, 0x7a, 0x18,
    0x40,

    /* U+1EAB "ẫ" */
    0x66, 0x62, 0x80, 0x38, 0x10, 0x4f, 0x45, 0x13,
    0xc0,

    /* U+1EAC "Ậ" */
    0x31, 0x20, 0xc, 0x31, 0x24, 0x9e, 0x86, 0x18,
    0x40, 0x10, 0x40,

    /* U+1EAD "ậ" */
    0x32, 0x40, 0xe0, 0x85, 0xf1, 0x8b, 0xc0, 0x42,
    0x0,

    /* U+1EAE "Ắ" */
    0x8, 0x44, 0x4e, 0x30, 0xc4, 0x92, 0x7a, 0x18,
    0x40,

    /* U+1EAF "ắ" */
    0x11, 0x22, 0xe0, 0x38, 0x2f, 0x8c, 0x5e,

    /* U+1EB0 "Ằ" */
    0x20, 0x44, 0x4e, 0x30, 0xc4, 0x92, 0x7a, 0x18,
    0x40,

    /* U+1EB1 "ằ" */
    0x41, 0x22, 0xe0, 0x38, 0x2f, 0x8c, 0x5e,

    /* U+1EB2 "Ẳ" */
    0x30, 0x25, 0x4e, 0x30, 0xc4, 0x92, 0x7a, 0x18,
    0x40,

    /* U+1EB3 "ẳ" */
    0x60, 0xaa, 0xe0, 0x38, 0x2f, 0x8c, 0x5e,

    /* U+1EB4 "Ẵ" */
    0x29, 0x48, 0x5e, 0x30, 0xc4, 0x92, 0x7a, 0x18,
    0x40,

    /* U+1EB5 "ẵ" */
    0x55, 0x22, 0xe0, 0x38, 0x2f, 0x8c, 0x5e,

    /* U+1EB6 "Ặ" */
    0x48, 0xc0, 0xc, 0x31, 0x24, 0x9e, 0x86, 0x18,
    0x40, 0x10, 0x40,

    /* U+1EB7 "ặ" */
    0x49, 0x80, 0xe0, 0x85, 0xf1, 0x8b, 0xc0, 0x42,
    0x0,

    /* U+1EB8 "Ẹ" */
    0xfc, 0x21, 0xf, 0x42, 0x10, 0xf8, 0x8, 0x40,

    /* U+1EB9 "ẹ" */
    0x74, 0x63, 0xf8, 0x41, 0xc0, 0x21, 0x0,

    /* U+1EBA "Ẻ" */
    0x60, 0x89, 0xf8, 0x43, 0xd0, 0x84, 0x3e,

    /* U+1EBB "ẻ" */
    0x60, 0x88, 0x7, 0x46, 0x3f, 0x84, 0x1c,

    /* U+1EBC "Ẽ" */
    0x4d, 0x81, 0xf8, 0x43, 0xd0, 0x84, 0x3e,

    /* U+1EBD "ẽ" */
    0x4d, 0x80, 0xe8, 0xc7, 0xf0, 0x83, 0x80,

    /* U+1EBE "Ế" */
    0x46, 0xa0, 0x3e, 0x82, 0xf, 0x20, 0x82, 0xf,
    0x80,

    /* U+1EBF "ế" */
    0x5, 0x2a, 0x0, 0x72, 0x28, 0xbe, 0x82, 0x7,
    0x0,

    /* U+1EC0 "Ề" */
    0x89, 0x50, 0x1f, 0x41, 0x7, 0x90, 0x41, 0x7,
    0xc0,

    /* U+1EC1 "ề" */
    0x81, 0x21, 0x40, 0x39, 0x14, 0x5f, 0x41, 0x3,
    0x80,

    /* U+1EC2 "Ể" */
    0x19, 0x1a, 0xbe, 0x82, 0xf, 0x20, 0x82, 0xf,
    0x80,

    /* U+1EC3 "ể" */
    0x19, 0x1a, 0x80, 0x72, 0x28, 0xbe, 0x82, 0x7,
    0x0,

    /* U+1EC4 "Ễ" */
    0x66, 0x62, 0x9f, 0x41, 0x7, 0x90, 0x41, 0x7,
    0xc0,

    /* U+1EC5 "ễ" */
    0x66, 0x62, 0x80, 0x39, 0x14, 0x5f, 0x41, 0x3,
    0x80,

    /* U+1EC6 "Ệ" */
    0x32, 0x41, 0xf8, 0x43, 0xd0, 0x84, 0x3e, 0x2,
    0x10,

    /* U+1EC7 "ệ" */
    0x32, 0x40, 0xe8, 0xc7, 0xf0, 0x83, 0x80, 0x42,
    0x0,

    /* U+1EC8 "Ỉ" */
    0x60, 0x89, 0xf2, 0x10, 0x84, 0x21, 0x3e,

    /* U+1EC9 "ỉ" */
    0x60, 0x88, 0x6, 0x10, 0x84, 0x21, 0x3e,

    /* U+1ECA "Ị" */
    0xf9, 0x8, 0x42, 0x10, 0x84, 0xf8, 0x8, 0x40,

    /* U+1ECB "ị" */
    0x21, 0x0, 0xc2, 0x10, 0x84, 0x27, 0xc0, 0x42,
    0x0,

    /* U+1ECC "Ọ" */
    0x74, 0x63, 0x18, 0xc6, 0x31, 0x70, 0x8, 0x40,

    /* U+1ECD "ọ" */
    0x74, 0x63, 0x18, 0xc5, 0xc0, 0x21, 0x0,

    /* U+1ECE "Ỏ" */
    0x60, 0x88, 0xe8, 0xc6, 0x31, 0x8c, 0x5c,

    /* U+1ECF "ỏ" */
    0x60, 0x88, 0x7, 0x46, 0x31, 0x8c, 0x5c,

    /* U+1ED0 "Ố" */
    0x46, 0xa0, 0x1c, 0x8a, 0x28, 0xa2, 0x8a, 0x27,
    0x0,

    /* U+1ED1 "ố" */
    0x5, 0x2a, 0x0, 0x72, 0x28, 0xa2, 0x8a, 0x27,
    0x0,

    /* U+1ED2 "Ồ" */
    0x89, 0x50, 0xe, 0x45, 0x14, 0x51, 0x45, 0x13,
    0x80,

    /* U+1ED3 "ồ" */
    0x81, 0x21, 0x40, 0x39, 0x14, 0x51, 0x45, 0x13,
    0x80,

    /* U+1ED4 "Ổ" */
    0x19, 0x1a, 0x9c, 0x8a, 0x28, 0xa2, 0x8a, 0x27,
    0x0,

    /* U+1ED5 "ổ" */
    0x19, 0x1a, 0x80, 0x72, 0x28, 0xa2, 0x8a, 0x27,
    0x0,

    /* U+1ED6 "Ỗ" */
    0x66, 0x62, 0x8e, 0x45, 0x14, 0x51, 0x45, 0x13,
    0x80,

    /* U+1ED7 "ỗ" */
    0x66, 0x62, 0x80, 0x39, 0x14, 0x51, 0x45, 0x13,
    0x80,

    /* U+1ED8 "Ộ" */
    0x32, 0x40, 0xe8, 0xc6, 0x31, 0x8c, 0x5c, 0x2,
    0x10,

    /* U+1ED9 "ộ" */
    0x32, 0x40, 0xe8, 0xc6, 0x31, 0x8b, 0x80, 0x42,
    0x0,

    /* U+1EDA "Ớ" */
    0x21, 0x30, 0x5e, 0x8a, 0x28, 0xa2, 0x8a, 0x27,
    0x0,

    /* U+1EDB "ớ" */
    0x20, 0x84, 0xc1, 0x7a, 0x28, 0xa2, 0x8a, 0x27,
    0x0,

    /* U+1EDC "Ờ" */
    0x40, 0xb0, 0x5e, 0x8a, 0x28, 0xa2, 0x8a, 0x27,
    0x0,

    /* U+1EDD "ờ" */
    0x41, 0x2, 0xc1, 0x7a, 0x28, 0xa2, 0x8a, 0x27,
    0x0,

    /* U+1EDE "Ở" */
    0x60, 0x52, 0x5e, 0x8a, 0x28, 0xa2, 0x8a, 0x27,
    0x0,

    /* U+1EDF "ở" */
    0x60, 0x42, 0x41, 0x7a, 0x28, 0xa2, 0x8a, 0x27,
    0x0,

    /* U+1EE0 "Ỡ" */
    0x52, 0x90, 0x5e, 0x8a, 0x28, 0xa2, 0x8a, 0x27,
    0x0,

    /* U+1EE1 "ỡ" */
    0x52, 0x90, 0x5e, 0x8a, 0x28, 0xa2, 0x89, 0xc0,

    /* U+1EE2 "Ợ" */
    0xc, 0x17, 0xa2, 0x8a, 0x28, 0xa2, 0x8a, 0x27,
    0x0, 0x20, 0x80,

    /* U+1EE3 "ợ" */
    0xc, 0x17, 0xa2, 0x8a, 0x28, 0xa2, 0x70, 0x2,
    0x8,

    /* U+1EE4 "Ụ" */
    0x8c, 0x63, 0x18, 0xc6, 0x31, 0x70, 0x8, 0x40,

    /* U+1EE5 "ụ" */
    0x8c, 0x63, 0x18, 0xcd, 0xa0, 0x21, 0x0,

    /* U+1EE6 "Ủ" */
    0x60, 0x89, 0x18, 0xc6, 0x31, 0x8c, 0x5c,

    /* U+1EE7 "ủ" */
    0x60, 0x88, 0x8, 0xc6, 0x31, 0x8c, 0xda,

    /* U+1EE8 "Ứ" */
    0x21, 0x30, 0x62, 0x8a, 0x28, 0xa2, 0x8a, 0x27,
    0x0,

    /* U+1EE9 "ứ" */
    0x20, 0x84, 0xc1, 0x8a, 0x28, 0xa2, 0x8a, 0x66,
    0x80,

    /* U+1EEA "Ừ" */
    0x40, 0xb0, 0x62, 0x8a, 0x28, 0xa2, 0x8a, 0x27,
    0x0,

    /* U+1EEB "ừ" */
    0x41, 0x2, 0xc1, 0x8a, 0x28, 0xa2, 0x8a, 0x66,
    0x80,

    /* U+1EEC "Ử" */
    0x60, 0x52, 0x62, 0x8a, 0x28, 0xa2, 0x8a, 0x27,
    0x0,

    /* U+1EED "ử" */
    0x60, 0x42, 0x41, 0x8a, 0x28, 0xa2, 0x8a, 0x66,
    0x80,

    /* U+1EEE "Ữ" */
    0x52, 0x90, 0x62, 0x8a, 0x28, 0xa2, 0x8a, 0x27,
    0x0,

    /* U+1EEF "ữ" */
    0x52, 0x90, 0x62, 0x8a, 0x28, 0xa2, 0x99, 0xa0,

    /* U+1EF0 "Ự" */
    0xc, 0x18, 0xa2, 0x8a, 0x28, 0xa2, 0x8a, 0x27,
    0x0, 0x20, 0x80,

    /* U+1EF1 "ự" */
    0xc, 0x18, 0xa2, 0x8a, 0x28, 0xa6, 0x68, 0x2,
    0x8,

    /* U+1EF2 "Ỳ" */
    0x41, 0x81, 0x18, 0xa9, 0x44, 0x21, 0x8,

    /* U+1EF3 "ỳ" */
    0x42, 0x8, 0x8, 0xc6, 0x2a, 0x51, 0x88, 0x4c,
    0x0,

    /* U+1EF4 "Ỵ" */
    0x8c, 0x62, 0xa5, 0x10, 0x84, 0x20, 0x8, 0x40,

    /* U+1EF5 "ỵ" */
    0x8c, 0x62, 0xa5, 0x18, 0x84, 0xc8, 0x40,

    /* U+1EF6 "Ỷ" */
    0x60, 0x89, 0x18, 0xa9, 0x44, 0x21, 0x8,

    /* U+1EF7 "ỷ" */
    0x60, 0x88, 0x8, 0xc6, 0x2a, 0x51, 0x88, 0x4c,
    0x0,

    /* U+1EF8 "Ỹ" */
    0x4d, 0x81, 0x18, 0xa9, 0x44, 0x21, 0x8,

    /* U+1EF9 "ỹ" */
    0x4d, 0x81, 0x18, 0xc5, 0x4a, 0x31, 0x9, 0x80,

    /* U+2013 "–" */
    0xfc,

    /* U+2014 "—" */
    0xfe,

    /* U+2018 "‘" */
    0x6d,

    /* U+2019 "’" */
    0xb6,

    /* U+201A "‚" */
    0xb6,

    /* U+201B "‛" */
    0x79,

    /* U+201C "“" */
    0x4c, 0xb6, 0x90,

    /* U+201D "”" */
    0x96, 0xd3, 0x20,

    /* U+2022 "•" */
    0x77, 0xff, 0xf7, 0x0,

    /* U+2030 "‰" */
    0x42, 0x8a, 0x52, 0x31, 0xa, 0x95, 0x54, 0xa0,

    /* U+20AB "₫" */
    0x8, 0xf0, 0x9a, 0x9a, 0x28, 0xa2, 0x99, 0xa0,
    0x3e
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 9, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 16, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 23, .adv_w = 112, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 31, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 38, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 45, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 52, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 59, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 68, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 75, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 84, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 91, .adv_w = 112, .box_w = 6, .box_h = 12, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 100, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 107, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 116, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 123, .adv_w = 112, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 133, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 142, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 151, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 160, .adv_w = 112, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 170, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 179, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 188, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 197, .adv_w = 112, .box_w = 6, .box_h = 14, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 208, .adv_w = 112, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 217, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 226, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 233, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 242, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 249, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 258, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 265, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 274, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 281, .adv_w = 112, .box_w = 6, .box_h = 14, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 292, .adv_w = 112, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 301, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 309, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 316, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 323, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 330, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 337, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 344, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 353, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 362, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 371, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 380, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 389, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 398, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 407, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 416, .adv_w = 112, .box_w = 5, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 425, .adv_w = 112, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 434, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 441, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 448, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 456, .adv_w = 112, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 465, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 473, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 480, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 487, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 494, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 503, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 512, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 521, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 530, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 539, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 548, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 557, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 566, .adv_w = 112, .box_w = 5, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 575, .adv_w = 112, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 584, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 593, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 602, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 611, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 620, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 629, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 638, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 647, .adv_w = 112, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 655, .adv_w = 112, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 666, .adv_w = 112, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 675, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 683, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 690, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 697, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 704, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 713, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 722, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 731, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 740, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 749, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 758, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 767, .adv_w = 112, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 775, .adv_w = 112, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 786, .adv_w = 112, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 795, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 802, .adv_w = 112, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 811, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 819, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 826, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 833, .adv_w = 112, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 842, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 849, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 857, .adv_w = 112, .box_w = 6, .box_h = 1, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 858, .adv_w = 112, .box_w = 7, .box_h = 1, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 859, .adv_w = 112, .box_w = 2, .box_h = 4, .ofs_x = 3, .ofs_y = 6},
    {.bitmap_index = 860, .adv_w = 112, .box_w = 2, .box_h = 4, .ofs_x = 3, .ofs_y = 6},
    {.bitmap_index = 861, .adv_w = 112, .box_w = 2, .box_h = 4, .ofs_x = 3, .ofs_y = -2},
    {.bitmap_index = 862, .adv_w = 112, .box_w = 2, .box_h = 4, .ofs_x = 3, .ofs_y = 6},
    {.bitmap_index = 863, .adv_w = 112, .box_w = 5, .box_h = 4, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 866, .adv_w = 112, .box_w = 5, .box_h = 4, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 869, .adv_w = 112, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 873, .adv_w = 112, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 881, .adv_w = 112, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -2}
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

extern const lv_font_t greybeard_14_lx_next;


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t greybeard_latinx_14 = {
#else
lv_font_t greybeard_latinx_14 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 14,          /*The maximum line height required by the font*/
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
    .fallback = &greybeard_14_lx_next,
#endif
    .user_data = NULL,
};



#endif /*#if GREYBEARD_LATINX_14*/

