/*******************************************************************************
 * Size: 14 px
 * Bpp: 1
 * Opts: --font Greybeard-14px.ttf -r 0x100-0x17F,0x218-0x21B,0x2C7,0x2D8-0x2DB,0x2DD,0x201D-0x201E --size 14 --bpp 1 --format lvgl --no-compress --lv-fallback greybeard_14_la_next --lv-font-name greybeard_latina_14 -o greybeard_latina_14.c
 ******************************************************************************/

#include "lvgl.h"

#ifndef GREYBEARD_LATINA_14
#define GREYBEARD_LATINA_14 1
#endif

#if GREYBEARD_LATINA_14

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0100 "Ā" */
    0x78, 0x3, 0xc, 0x31, 0x24, 0x9e, 0x86, 0x18,
    0x40,

    /* U+0101 "ā" */
    0x70, 0x1c, 0x10, 0xbe, 0x31, 0x78,

    /* U+0102 "Ă" */
    0x48, 0xc0, 0xc, 0x31, 0x24, 0x9e, 0x86, 0x18,
    0x40,

    /* U+0103 "ă" */
    0x49, 0x80, 0xe0, 0x85, 0xf1, 0x8b, 0xc0,

    /* U+0104 "Ą" */
    0x30, 0x60, 0xc2, 0x44, 0x8f, 0x21, 0x42, 0x84,
    0x10, 0x18,

    /* U+0105 "ą" */
    0x70, 0x20, 0x9e, 0x8a, 0x27, 0x84, 0xc,

    /* U+0106 "Ć" */
    0x9, 0x80, 0x64, 0xc2, 0x10, 0x82, 0x4c,

    /* U+0107 "ć" */
    0x10, 0x88, 0x7, 0x46, 0x10, 0x84, 0x5c,

    /* U+0108 "Ĉ" */
    0x32, 0x40, 0x64, 0xc2, 0x10, 0x82, 0x4c,

    /* U+0109 "ĉ" */
    0x32, 0x40, 0xe8, 0xc2, 0x10, 0x8b, 0x80,

    /* U+010A "Ċ" */
    0x21, 0x0, 0x64, 0xc2, 0x10, 0x82, 0x4c,

    /* U+010B "ċ" */
    0x21, 0x0, 0xe8, 0xc2, 0x10, 0x8b, 0x80,

    /* U+010C "Č" */
    0x49, 0x80, 0x64, 0xc2, 0x10, 0x82, 0x4c,

    /* U+010D "č" */
    0x49, 0x80, 0xe8, 0xc2, 0x10, 0x8b, 0x80,

    /* U+010E "Ď" */
    0x93, 0x1, 0xc9, 0x46, 0x31, 0x8c, 0xb8,

    /* U+010F "ď" */
    0x6, 0x14, 0x28, 0x66, 0x93, 0x22, 0x44, 0x89,
    0x31, 0xa0,

    /* U+0110 "Đ" */
    0x71, 0x24, 0x51, 0xe5, 0x14, 0x52, 0x70,

    /* U+0111 "đ" */
    0x8, 0xf0, 0x9a, 0x9a, 0x28, 0xa2, 0x99, 0xa0,

    /* U+0112 "Ē" */
    0x70, 0x3f, 0x8, 0x43, 0xd0, 0x84, 0x3e,

    /* U+0113 "ē" */
    0x70, 0x1d, 0x18, 0xfe, 0x10, 0x70,

    /* U+0114 "Ĕ" */
    0x49, 0x81, 0xf8, 0x43, 0xd0, 0x84, 0x3e,

    /* U+0115 "ĕ" */
    0x49, 0x80, 0xe8, 0xc7, 0xf0, 0x83, 0x80,

    /* U+0116 "Ė" */
    0x21, 0x1, 0xf8, 0x43, 0xd0, 0x84, 0x3e,

    /* U+0117 "ė" */
    0x21, 0x0, 0xe8, 0xc7, 0xf0, 0x83, 0x80,

    /* U+0118 "Ę" */
    0xfc, 0x21, 0xf, 0x42, 0x10, 0xf9, 0x6,

    /* U+0119 "ę" */
    0x74, 0x63, 0xf8, 0x41, 0xc4, 0x18,

    /* U+011A "Ě" */
    0x49, 0x81, 0xf8, 0x43, 0xd0, 0x84, 0x3e,

    /* U+011B "ě" */
    0x49, 0x80, 0xe8, 0xc7, 0xf0, 0x83, 0x80,

    /* U+011C "Ĝ" */
    0x32, 0x40, 0xe8, 0xc2, 0x17, 0x8c, 0x5e,

    /* U+011D "ĝ" */
    0x32, 0x40, 0xf8, 0xc6, 0x33, 0x68, 0x62, 0xe0,

    /* U+011E "Ğ" */
    0x49, 0x80, 0xe8, 0xc2, 0x17, 0x8c, 0x5e,

    /* U+011F "ğ" */
    0x49, 0x80, 0xf8, 0xc6, 0x33, 0x68, 0x62, 0xe0,

    /* U+0120 "Ġ" */
    0x21, 0x0, 0xe8, 0xc2, 0x17, 0x8c, 0x5e,

    /* U+0121 "ġ" */
    0x21, 0x0, 0xf8, 0xc6, 0x33, 0x68, 0x62, 0xe0,

    /* U+0122 "Ģ" */
    0x74, 0x61, 0xb, 0xc6, 0x31, 0x79, 0x10,

    /* U+0123 "ģ" */
    0x11, 0x8, 0xf8, 0xc6, 0x33, 0x68, 0x62, 0xe0,

    /* U+0124 "Ĥ" */
    0x32, 0x41, 0x18, 0xc7, 0xf1, 0x8c, 0x62,

    /* U+0125 "ĥ" */
    0x32, 0x61, 0xb, 0x66, 0x31, 0x8c, 0x62,

    /* U+0126 "Ħ" */
    0x8c, 0x7f, 0x1f, 0xc6, 0x31, 0x88,

    /* U+0127 "ħ" */
    0x43, 0xc4, 0x16, 0x65, 0x14, 0x51, 0x45, 0x10,

    /* U+0128 "Ĩ" */
    0x4d, 0x81, 0xf2, 0x10, 0x84, 0x21, 0x3e,

    /* U+0129 "ĩ" */
    0x4d, 0x80, 0xc2, 0x10, 0x84, 0x27, 0xc0,

    /* U+012A "Ī" */
    0x70, 0x3e, 0x42, 0x10, 0x84, 0x21, 0x3e,

    /* U+012B "ī" */
    0x70, 0x18, 0x42, 0x10, 0x84, 0xf8,

    /* U+012C "Ĭ" */
    0x49, 0x81, 0xf2, 0x10, 0x84, 0x21, 0x3e,

    /* U+012D "ĭ" */
    0x93, 0x0, 0xc2, 0x10, 0x84, 0x27, 0xc0,

    /* U+012E "Į" */
    0xf9, 0x8, 0x42, 0x10, 0x84, 0xf9, 0x6,

    /* U+012F "į" */
    0x21, 0x0, 0xc2, 0x10, 0x84, 0x27, 0xc8, 0x30,

    /* U+0130 "İ" */
    0x21, 0x1, 0xf2, 0x10, 0x84, 0x21, 0x3e,

    /* U+0131 "ı" */
    0x61, 0x8, 0x42, 0x13, 0xe0,

    /* U+0132 "Ĳ" */
    0xed, 0x14, 0x51, 0x45, 0x14, 0x55, 0xd8,

    /* U+0133 "ĳ" */
    0x45, 0x10, 0x33, 0x45, 0x14, 0x51, 0x47, 0x90,
    0x46,

    /* U+0134 "Ĵ" */
    0x18, 0x90, 0xf, 0x8, 0x20, 0x82, 0x8a, 0x27,
    0x0,

    /* U+0135 "ĵ" */
    0x32, 0x40, 0xe1, 0x8, 0x42, 0x10, 0xa4, 0xc0,

    /* U+0136 "Ķ" */
    0x8c, 0x65, 0x4e, 0x4a, 0x51, 0x89, 0x10,

    /* U+0137 "ķ" */
    0x84, 0x21, 0x19, 0x53, 0x14, 0x94, 0x48, 0x80,

    /* U+0138 "ĸ" */
    0x8c, 0xa9, 0x8a, 0x4a, 0x20,

    /* U+0139 "Ĺ" */
    0x13, 0x1, 0x8, 0x42, 0x10, 0x84, 0x3e,

    /* U+013A "ĺ" */
    0x36, 0x18, 0x42, 0x10, 0x84, 0x21, 0x3e,

    /* U+013B "Ļ" */
    0x84, 0x21, 0x8, 0x42, 0x10, 0xf9, 0x10,

    /* U+013C "ļ" */
    0x61, 0x8, 0x42, 0x10, 0x84, 0x27, 0xc8, 0x80,

    /* U+013D "Ľ" */
    0x1c, 0x63, 0x28, 0x42, 0x10, 0x87, 0xc0,

    /* U+013E "ľ" */
    0xd, 0x92, 0x4a, 0x20, 0x82, 0x8, 0x20, 0x8f,
    0x80,

    /* U+013F "Ŀ" */
    0x84, 0x21, 0x18, 0xc2, 0x10, 0xf8,

    /* U+0140 "ŀ" */
    0x60, 0x40, 0x81, 0x2, 0x24, 0x48, 0x10, 0x21,
    0xf0,

    /* U+0141 "Ł" */
    0x41, 0x4, 0x14, 0x63, 0x4, 0x10, 0x7c,

    /* U+0142 "ł" */
    0x61, 0x8, 0x43, 0x30, 0x84, 0x27, 0xc0,

    /* U+0143 "Ń" */
    0x13, 0x1, 0x18, 0xe6, 0xb5, 0x9c, 0x62,

    /* U+0144 "ń" */
    0x10, 0x88, 0xb, 0x66, 0x31, 0x8c, 0x62,

    /* U+0145 "Ņ" */
    0x8c, 0x73, 0x5a, 0xce, 0x31, 0x89, 0x10,

    /* U+0146 "ņ" */
    0xb6, 0x63, 0x18, 0xc6, 0x24, 0x40,

    /* U+0147 "Ň" */
    0x49, 0x81, 0x18, 0xe6, 0xb5, 0x9c, 0x62,

    /* U+0148 "ň" */
    0x49, 0x81, 0x6c, 0xc6, 0x31, 0x8c, 0x40,

    /* U+0149 "ŉ" */
    0xc1, 0x8, 0x0, 0x59, 0x94, 0x51, 0x45, 0x14,
    0x40,

    /* U+014A "Ŋ" */
    0xb6, 0x63, 0x18, 0xc6, 0x35, 0x90,

    /* U+014B "ŋ" */
    0xb6, 0x63, 0x18, 0xc6, 0x21, 0x30,

    /* U+014C "Ō" */
    0x70, 0x1d, 0x18, 0xc6, 0x31, 0x8c, 0x5c,

    /* U+014D "ō" */
    0x70, 0x1d, 0x18, 0xc6, 0x31, 0x70,

    /* U+014E "Ŏ" */
    0x49, 0x80, 0xe8, 0xc6, 0x31, 0x8c, 0x5c,

    /* U+014F "ŏ" */
    0x49, 0x80, 0xe8, 0xc6, 0x31, 0x8b, 0x80,

    /* U+0150 "Ő" */
    0x25, 0x20, 0x1c, 0x8a, 0x28, 0xa2, 0x8a, 0x27,
    0x0,

    /* U+0151 "ő" */
    0x4a, 0x64, 0x7, 0x46, 0x31, 0x8c, 0x5c,

    /* U+0152 "Œ" */
    0x7e, 0x49, 0x24, 0x9e, 0x49, 0x24, 0x7c,

    /* U+0153 "œ" */
    0x6d, 0x26, 0x4c, 0xf9, 0x12, 0x1b, 0x80,

    /* U+0154 "Ŕ" */
    0x13, 0x1, 0xe8, 0xc7, 0xd4, 0x94, 0x62,

    /* U+0155 "ŕ" */
    0x10, 0x88, 0xd, 0xb5, 0x8, 0x42, 0x10,

    /* U+0156 "Ŗ" */
    0xf4, 0x63, 0x1f, 0x52, 0x51, 0x89, 0x10,

    /* U+0157 "ŗ" */
    0xdb, 0x50, 0x84, 0x21, 0x4, 0x40,

    /* U+0158 "Ř" */
    0x49, 0x81, 0xe8, 0xc7, 0xd4, 0x94, 0x62,

    /* U+0159 "ř" */
    0x49, 0x81, 0xb6, 0xa1, 0x8, 0x42, 0x0,

    /* U+015A "Ś" */
    0x9, 0x80, 0xe8, 0xc1, 0xc1, 0xc, 0x5c,

    /* U+015B "ś" */
    0x10, 0x88, 0x7, 0x46, 0xe, 0xc, 0x5c,

    /* U+015C "Ŝ" */
    0x32, 0x40, 0xe8, 0xc1, 0xc1, 0xc, 0x5c,

    /* U+015D "ŝ" */
    0x32, 0x40, 0xe8, 0xc1, 0xc1, 0x8b, 0x80,

    /* U+015E "Ş" */
    0x74, 0x61, 0x7, 0x4, 0x31, 0x71, 0x10,

    /* U+015F "ş" */
    0x74, 0x60, 0xe0, 0xc5, 0xc4, 0x40,

    /* U+0160 "Š" */
    0x49, 0x80, 0xe8, 0xc1, 0xc1, 0xc, 0x5c,

    /* U+0161 "š" */
    0x49, 0x80, 0xe8, 0xc1, 0xc1, 0x8b, 0x80,

    /* U+0162 "Ţ" */
    0xf9, 0x8, 0x42, 0x10, 0x84, 0x20, 0x88,

    /* U+0163 "ţ" */
    0x42, 0x3c, 0x84, 0x21, 0x9, 0x31, 0x10,

    /* U+0164 "Ť" */
    0x49, 0x81, 0xf2, 0x10, 0x84, 0x21, 0x8,

    /* U+0165 "ť" */
    0xc, 0x14, 0x52, 0xf1, 0x4, 0x10, 0x41, 0x23,
    0x0,

    /* U+0166 "Ŧ" */
    0xf9, 0x8, 0x47, 0x10, 0x84, 0x20,

    /* U+0167 "ŧ" */
    0x42, 0x3c, 0x8f, 0x21, 0x9, 0x30,

    /* U+0168 "Ũ" */
    0x4d, 0x81, 0x18, 0xc6, 0x31, 0x8c, 0x5c,

    /* U+0169 "ũ" */
    0x4d, 0x81, 0x18, 0xc6, 0x31, 0x9b, 0x40,

    /* U+016A "Ū" */
    0x70, 0x23, 0x18, 0xc6, 0x31, 0x8c, 0x5c,

    /* U+016B "ū" */
    0x70, 0x23, 0x18, 0xc6, 0x33, 0x68,

    /* U+016C "Ŭ" */
    0x49, 0x81, 0x18, 0xc6, 0x31, 0x8c, 0x5c,

    /* U+016D "ŭ" */
    0x49, 0x81, 0x18, 0xc6, 0x31, 0x9b, 0x40,

    /* U+016E "Ů" */
    0x22, 0x89, 0x18, 0xc6, 0x31, 0x8c, 0x5c,

    /* U+016F "ů" */
    0x22, 0x88, 0x8, 0xc6, 0x31, 0x8c, 0xda,

    /* U+0170 "Ű" */
    0x25, 0x20, 0x22, 0x8a, 0x28, 0xa2, 0x8a, 0x27,
    0x0,

    /* U+0171 "ű" */
    0x4a, 0x64, 0x8, 0xc6, 0x31, 0x8c, 0xda,

    /* U+0172 "Ų" */
    0x8c, 0x63, 0x18, 0xc6, 0x31, 0x71, 0x6,

    /* U+0173 "ų" */
    0x8a, 0x28, 0xa2, 0x8a, 0x66, 0x84, 0xc,

    /* U+0174 "Ŵ" */
    0x18, 0x48, 0x4, 0x18, 0x32, 0x64, 0xaa, 0x6c,
    0x89, 0x10,

    /* U+0175 "ŵ" */
    0x18, 0x48, 0x4, 0x19, 0x32, 0x64, 0xb6, 0x44,
    0x88,

    /* U+0176 "Ŷ" */
    0x32, 0x41, 0x18, 0xa9, 0x44, 0x21, 0x8,

    /* U+0177 "ŷ" */
    0x32, 0x41, 0x18, 0xc5, 0x4a, 0x31, 0x9, 0x80,

    /* U+0178 "Ÿ" */
    0x4a, 0x41, 0x18, 0xa9, 0x44, 0x21, 0x8,

    /* U+0179 "Ź" */
    0x9, 0x81, 0xf0, 0x88, 0x84, 0x44, 0x3e,

    /* U+017A "ź" */
    0x10, 0x88, 0xf, 0x84, 0x44, 0x44, 0x3e,

    /* U+017B "Ż" */
    0x21, 0x1, 0xf0, 0x88, 0x84, 0x44, 0x3e,

    /* U+017C "ż" */
    0x21, 0x1, 0xf0, 0x88, 0x88, 0x87, 0xc0,

    /* U+017D "Ž" */
    0x49, 0x81, 0xf0, 0x88, 0x84, 0x44, 0x3e,

    /* U+017E "ž" */
    0x49, 0x81, 0xf0, 0x88, 0x88, 0x87, 0xc0,

    /* U+017F "ſ" */
    0x69, 0x88, 0x88, 0x88, 0x88,

    /* U+0218 "Ș" */
    0x74, 0x61, 0x7, 0x4, 0x31, 0x71, 0x10,

    /* U+0219 "ș" */
    0x74, 0x60, 0xe0, 0xc5, 0xc4, 0x40,

    /* U+021A "Ț" */
    0xf9, 0x8, 0x42, 0x10, 0x84, 0x20, 0x88,

    /* U+021B "ț" */
    0x42, 0x3c, 0x84, 0x21, 0x9, 0x31, 0x10,

    /* U+02C7 "ˇ" */
    0x8a, 0x88,

    /* U+02D8 "˘" */
    0x99, 0x60,

    /* U+02D9 "˙" */
    0xf0,

    /* U+02DA "˚" */
    0x69, 0x96,

    /* U+02DB "˛" */
    0x52, 0xa0,

    /* U+02DD "˝" */
    0x4a, 0x64,

    /* U+201D "”" */
    0x96, 0xd3, 0x20,

    /* U+201E "„" */
    0x96, 0xd3, 0x20
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 9, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 15, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 24, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 31, .adv_w = 112, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 41, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 48, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 55, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 62, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 69, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 76, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 83, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 90, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 97, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 104, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 111, .adv_w = 112, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 121, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 128, .adv_w = 112, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 136, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 143, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 149, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 156, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 163, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 170, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 177, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 184, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 190, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 197, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 204, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 211, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 219, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 226, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 234, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 241, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 249, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 256, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 264, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 271, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 278, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 284, .adv_w = 112, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 292, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 299, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 306, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 313, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 319, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 326, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 333, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 340, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 348, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 355, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 360, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 367, .adv_w = 112, .box_w = 6, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 376, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 385, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 393, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 400, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 408, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 413, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 420, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 427, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 434, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 442, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 449, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 458, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 464, .adv_w = 112, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 473, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 480, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 487, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 494, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 501, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 508, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 514, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 521, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 528, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 537, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 543, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 549, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 556, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 562, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 569, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 576, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 585, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 592, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 599, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 606, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 613, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 620, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 627, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 633, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 640, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 647, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 654, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 661, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 668, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 675, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 682, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 688, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 695, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 702, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 709, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 716, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 723, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 732, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 738, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 744, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 751, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 758, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 765, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 771, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 778, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 785, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 792, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 799, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 808, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 815, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 822, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 829, .adv_w = 112, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 839, .adv_w = 112, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 848, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 855, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 863, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 870, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 877, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 884, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 891, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 898, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 905, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 912, .adv_w = 112, .box_w = 4, .box_h = 10, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 917, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 924, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 930, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 937, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 944, .adv_w = 112, .box_w = 5, .box_h = 3, .ofs_x = 1, .ofs_y = 7},
    {.bitmap_index = 946, .adv_w = 112, .box_w = 4, .box_h = 3, .ofs_x = 1, .ofs_y = 7},
    {.bitmap_index = 948, .adv_w = 112, .box_w = 2, .box_h = 2, .ofs_x = 2, .ofs_y = 8},
    {.bitmap_index = 949, .adv_w = 112, .box_w = 4, .box_h = 4, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 951, .adv_w = 112, .box_w = 3, .box_h = 4, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 953, .adv_w = 112, .box_w = 5, .box_h = 3, .ofs_x = 1, .ofs_y = 7},
    {.bitmap_index = 955, .adv_w = 112, .box_w = 5, .box_h = 4, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 958, .adv_w = 112, .box_w = 5, .box_h = 4, .ofs_x = 1, .ofs_y = -2}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_1[] = {
    0x0, 0x1, 0x2, 0x3, 0xaf, 0xc0, 0xc1, 0xc2,
    0xc3, 0xc5, 0x1e05, 0x1e06
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 256, .range_length = 128, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 536, .range_length = 7687, .glyph_id_start = 129,
        .unicode_list = unicode_list_1, .glyph_id_ofs_list = NULL, .list_length = 12, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
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
    .cmap_num = 2,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};

extern const lv_font_t greybeard_14_la_next;


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t greybeard_latina_14 = {
#else
lv_font_t greybeard_latina_14 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 13,          /*The maximum line height required by the font*/
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
    .fallback = &greybeard_14_la_next,
#endif
    .user_data = NULL,
};



#endif /*#if GREYBEARD_LATINA_14*/

