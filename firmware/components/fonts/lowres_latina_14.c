/*******************************************************************************
 * Size: 14 px
 * Bpp: 1
 * Opts: --font Greybeard-14px.ttf -r 0x100-0x17F,0x218-0x21B,0x2C7,0x2D8-0x2DB,0x2DD,0x18F,0x259,0x1E04-0x1E05,0x1E0C-0x1E0D,0x1E24-0x1E25,0x1E32-0x1E33,0x1E36-0x1E37,0x1E42-0x1E43,0x1E46-0x1E47,0x1E5A-0x1E5B,0x1E62-0x1E63,0x1E6C-0x1E6D,0x1E7E-0x1E7F,0x1E88-0x1E89,0x1E92-0x1E93,0x1E9E,0x391-0x3A1,0x3A3-0x3A9,0x3B1-0x3C9,0x2013-0x2014,0x2018-0x2019,0x201C-0x201E,0x2020-0x2021,0x202F-0x2030,0x2032-0x2033,0x2122,0x2212,0x2260,0x2264-0x2265 --size 14 --bpp 1 --format lvgl --no-compress --lv-fallback lowres_14_la_next --lv-font-name lowres_latina_14 -o ./lowres_latina_14.c
 ******************************************************************************/

#include "lvgl.h"

#ifndef LOWRES_LATINA_14
#define LOWRES_LATINA_14 1
#endif

#if LOWRES_LATINA_14

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

    /* U+018F "Ə" */
    0x74, 0x42, 0x1f, 0xc6, 0x31, 0x70,

    /* U+0218 "Ș" */
    0x74, 0x61, 0x7, 0x4, 0x31, 0x71, 0x10,

    /* U+0219 "ș" */
    0x74, 0x60, 0xe0, 0xc5, 0xc4, 0x40,

    /* U+021A "Ț" */
    0xf9, 0x8, 0x42, 0x10, 0x84, 0x20, 0x88,

    /* U+021B "ț" */
    0x42, 0x3c, 0x84, 0x21, 0x9, 0x31, 0x10,

    /* U+0259 "ə" */
    0xe0, 0x83, 0xf8, 0xc9, 0x80,

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

    /* U+0391 "Α" */
    0x30, 0xc3, 0x12, 0x49, 0xe8, 0x61, 0x84,

    /* U+0392 "Β" */
    0xf4, 0x63, 0x1f, 0x46, 0x31, 0xf0,

    /* U+0393 "Γ" */
    0xfc, 0x21, 0x8, 0x42, 0x10, 0x80,

    /* U+0394 "Δ" */
    0x21, 0x14, 0xa5, 0x46, 0x31, 0xf8,

    /* U+0395 "Ε" */
    0xfc, 0x21, 0xf, 0x42, 0x10, 0xf8,

    /* U+0396 "Ζ" */
    0xf8, 0x44, 0x22, 0x21, 0x10, 0xf8,

    /* U+0397 "Η" */
    0x8c, 0x63, 0x1f, 0xc6, 0x31, 0x88,

    /* U+0398 "Θ" */
    0x74, 0x63, 0x1f, 0xc6, 0x31, 0x70,

    /* U+0399 "Ι" */
    0xf9, 0x8, 0x42, 0x10, 0x84, 0xf8,

    /* U+039A "Κ" */
    0x8c, 0x65, 0x4e, 0x4a, 0x51, 0x88,

    /* U+039B "Λ" */
    0x21, 0x14, 0xa5, 0x46, 0x31, 0x88,

    /* U+039C "Μ" */
    0x86, 0x1c, 0xf3, 0xb6, 0xd8, 0x61, 0x84,

    /* U+039D "Ν" */
    0x8c, 0x73, 0x5a, 0xce, 0x31, 0x88,

    /* U+039E "Ξ" */
    0xfc, 0x40, 0x7, 0x0, 0x11, 0xf8,

    /* U+039F "Ο" */
    0x74, 0x63, 0x18, 0xc6, 0x31, 0x70,

    /* U+03A0 "Π" */
    0xfc, 0x63, 0x18, 0xc6, 0x31, 0x88,

    /* U+03A1 "Ρ" */
    0xf4, 0x63, 0x1f, 0x42, 0x10, 0x80,

    /* U+03A3 "Σ" */
    0xfc, 0x10, 0x41, 0x11, 0x10, 0xf8,

    /* U+03A4 "Τ" */
    0xf9, 0x8, 0x42, 0x10, 0x84, 0x20,

    /* U+03A5 "Υ" */
    0x8c, 0x62, 0xa5, 0x10, 0x84, 0x20,

    /* U+03A6 "Φ" */
    0x23, 0xab, 0x5a, 0xd6, 0xae, 0x20,

    /* U+03A7 "Χ" */
    0x8c, 0x54, 0xa2, 0x29, 0x51, 0x88,

    /* U+03A8 "Ψ" */
    0xad, 0x6b, 0x5a, 0xb8, 0x84, 0x20,

    /* U+03A9 "Ω" */
    0x31, 0x28, 0x61, 0x86, 0x14, 0x8c, 0xfc,

    /* U+03B1 "α" */
    0x4a, 0xa9, 0x24, 0x92, 0x46, 0xc0,

    /* U+03B2 "β" */
    0x64, 0xa5, 0x2e, 0x4a, 0x31, 0x8f, 0xa1, 0x0,

    /* U+03B3 "γ" */
    0x8c, 0x54, 0xa2, 0x10, 0x84, 0x20,

    /* U+03B4 "δ" */
    0x32, 0x10, 0x45, 0x46, 0x31, 0x8b, 0x80,

    /* U+03B5 "ε" */
    0x74, 0x60, 0xc8, 0x45, 0xc0,

    /* U+03B6 "ζ" */
    0xf8, 0x88, 0x44, 0x22, 0x10, 0x83, 0x82, 0x20,

    /* U+03B7 "η" */
    0xb6, 0x63, 0x18, 0xc6, 0x21, 0x8,

    /* U+03B8 "θ" */
    0x22, 0xa3, 0x1f, 0xc6, 0x31, 0x51, 0x0,

    /* U+03B9 "ι" */
    0x88, 0x88, 0x89, 0x60,

    /* U+03BA "κ" */
    0x8c, 0xa9, 0x8a, 0x4a, 0x20,

    /* U+03BB "λ" */
    0x82, 0x10, 0x42, 0x29, 0x51, 0x8c, 0x40,

    /* U+03BC "μ" */
    0x8c, 0x63, 0x18, 0xee, 0xb0, 0x80,

    /* U+03BD "ν" */
    0x8c, 0x62, 0xa5, 0x10, 0x80,

    /* U+03BE "ξ" */
    0xfa, 0x21, 0x7, 0x22, 0x10, 0x83, 0x82, 0x20,

    /* U+03BF "ο" */
    0x74, 0x63, 0x18, 0xc5, 0xc0,

    /* U+03C0 "π" */
    0xfd, 0x24, 0x92, 0x49, 0x24, 0x80,

    /* U+03C1 "ρ" */
    0x74, 0x63, 0x18, 0xe6, 0xd0, 0x80,

    /* U+03C2 "ς" */
    0x31, 0x28, 0x20, 0x81, 0x3, 0x81, 0x8,

    /* U+03C3 "σ" */
    0x3d, 0x48, 0xa2, 0x89, 0x42, 0x0,

    /* U+03C4 "τ" */
    0xf9, 0x8, 0x42, 0x10, 0x60,

    /* U+03C5 "υ" */
    0x8c, 0x63, 0x18, 0xc5, 0xc0,

    /* U+03C6 "φ" */
    0x15, 0x6b, 0x5a, 0xd5, 0xc4, 0x20,

    /* U+03C7 "χ" */
    0x8c, 0x54, 0xa2, 0x29, 0x51, 0x88,

    /* U+03C8 "ψ" */
    0x21, 0x9, 0x5a, 0xd6, 0xb5, 0xab, 0x88, 0x40,

    /* U+03C9 "ω" */
    0x45, 0x6, 0x4c, 0x99, 0x32, 0x5b, 0x0,

    /* U+1E04 "Ḅ" */
    0xf4, 0x63, 0x1f, 0x46, 0x31, 0xf0, 0x8, 0x40,

    /* U+1E05 "ḅ" */
    0x84, 0x21, 0x6c, 0xc6, 0x31, 0xcd, 0x80, 0x42,
    0x0,

    /* U+1E0C "Ḍ" */
    0xe4, 0xa3, 0x18, 0xc6, 0x32, 0xe0, 0x8, 0x40,

    /* U+1E0D "ḍ" */
    0x8, 0x42, 0xd9, 0xc6, 0x31, 0x9b, 0x40, 0x42,
    0x0,

    /* U+1E24 "Ḥ" */
    0x8c, 0x63, 0x1f, 0xc6, 0x31, 0x88, 0x8, 0x40,

    /* U+1E25 "ḥ" */
    0x84, 0x21, 0x6c, 0xc6, 0x31, 0x8c, 0x40, 0x42,
    0x0,

    /* U+1E32 "Ḳ" */
    0x8c, 0x65, 0x4e, 0x4a, 0x51, 0x88, 0x8, 0x40,

    /* U+1E33 "ḳ" */
    0x84, 0x21, 0x19, 0x53, 0x14, 0x94, 0x40, 0x42,
    0x0,

    /* U+1E36 "Ḷ" */
    0x84, 0x21, 0x8, 0x42, 0x10, 0xf8, 0x8, 0x40,

    /* U+1E37 "ḷ" */
    0x61, 0x8, 0x42, 0x10, 0x84, 0x27, 0xc0, 0x42,
    0x0,

    /* U+1E42 "Ṃ" */
    0x86, 0x1c, 0xf3, 0xb6, 0xd8, 0x61, 0x84, 0x1,
    0x4,

    /* U+1E43 "ṃ" */
    0xed, 0x26, 0x4c, 0x99, 0x32, 0x64, 0x80, 0x10,
    0x20,

    /* U+1E46 "Ṇ" */
    0x8c, 0x73, 0x5a, 0xce, 0x31, 0x88, 0x8, 0x40,

    /* U+1E47 "ṇ" */
    0xb6, 0x63, 0x18, 0xc6, 0x20, 0x21, 0x0,

    /* U+1E5A "Ṛ" */
    0xf4, 0x63, 0x1f, 0x52, 0x51, 0x88, 0x8, 0x40,

    /* U+1E5B "ṛ" */
    0xdb, 0x50, 0x84, 0x21, 0x0, 0x42, 0x0,

    /* U+1E62 "Ṣ" */
    0x74, 0x61, 0x7, 0x4, 0x31, 0x70, 0x8, 0x40,

    /* U+1E63 "ṣ" */
    0x74, 0x60, 0xe0, 0xc5, 0xc0, 0x21, 0x0,

    /* U+1E6C "Ṭ" */
    0xf9, 0x8, 0x42, 0x10, 0x84, 0x20, 0x8, 0x40,

    /* U+1E6D "ṭ" */
    0x42, 0x3c, 0x84, 0x21, 0x9, 0x30, 0x8, 0x40,

    /* U+1E7E "Ṿ" */
    0x8c, 0x63, 0x18, 0xa9, 0x44, 0x20, 0x8, 0x40,

    /* U+1E7F "ṿ" */
    0x8c, 0x62, 0xa5, 0x10, 0x80, 0x21, 0x0,

    /* U+1E88 "Ẉ" */
    0x83, 0x6, 0x4c, 0x99, 0x2a, 0x9b, 0x22, 0x44,
    0x0, 0x40, 0x80,

    /* U+1E89 "ẉ" */
    0x83, 0x26, 0x4c, 0x96, 0xc8, 0x91, 0x0, 0x10,
    0x20,

    /* U+1E92 "Ẓ" */
    0xf8, 0x44, 0x22, 0x21, 0x10, 0xf8, 0x8, 0x40,

    /* U+1E93 "ẓ" */
    0xf8, 0x44, 0x44, 0x43, 0xe0, 0x21, 0x0,

    /* U+1E9E "ẞ" */
    0xfc, 0xa9, 0x68, 0xc6, 0x31, 0xb0,

    /* U+2013 "–" */
    0xfc,

    /* U+2014 "—" */
    0xfe,

    /* U+2018 "‘" */
    0x6d,

    /* U+2019 "’" */
    0xb6,

    /* U+201C "“" */
    0x4c, 0xb6, 0x90,

    /* U+201D "”" */
    0x96, 0xd3, 0x20,

    /* U+201E "„" */
    0x96, 0xd3, 0x20,

    /* U+2020 "†" */
    0x10, 0x70, 0x47, 0xf5, 0x42, 0x4, 0x8, 0x10,
    0x20, 0x40, 0x80,

    /* U+2021 "‡" */
    0x10, 0x70, 0x47, 0xf5, 0x42, 0x4, 0x2a, 0xfe,
    0x20, 0xe0, 0x80,

    /* U+202F " " */
    0x0,

    /* U+2030 "‰" */
    0x42, 0x8a, 0x52, 0x31, 0xa, 0x95, 0x54, 0xa0,

    /* U+2032 "′" */
    0x2d, 0x40,

    /* U+2033 "″" */
    0x25, 0xb4, 0xa4,

    /* U+2122 "™" */
    0xf5, 0x75, 0xd5,

    /* U+2212 "−" */
    0xf8,

    /* U+2260 "≠" */
    0x8, 0xbe, 0x42, 0x7d, 0x10,

    /* U+2264 "≤" */
    0x9, 0x91, 0x4, 0x18, 0x20, 0xf8,

    /* U+2265 "≥" */
    0x83, 0x4, 0x11, 0x32, 0x0, 0xf8
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
    {.bitmap_index = 917, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 923, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 930, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 936, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 943, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 950, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 955, .adv_w = 112, .box_w = 5, .box_h = 3, .ofs_x = 1, .ofs_y = 7},
    {.bitmap_index = 957, .adv_w = 112, .box_w = 4, .box_h = 3, .ofs_x = 1, .ofs_y = 7},
    {.bitmap_index = 959, .adv_w = 112, .box_w = 2, .box_h = 2, .ofs_x = 2, .ofs_y = 8},
    {.bitmap_index = 960, .adv_w = 112, .box_w = 4, .box_h = 4, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 962, .adv_w = 112, .box_w = 3, .box_h = 4, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 964, .adv_w = 112, .box_w = 5, .box_h = 3, .ofs_x = 1, .ofs_y = 7},
    {.bitmap_index = 966, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 973, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 979, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 985, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 991, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 997, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1003, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1009, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1015, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1021, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1027, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1033, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1040, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1046, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1052, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1058, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1064, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1070, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1076, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1082, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1088, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1094, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1100, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1106, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1113, .adv_w = 112, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1119, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1127, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1133, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1140, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1145, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1153, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1159, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1166, .adv_w = 112, .box_w = 4, .box_h = 7, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 1170, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1175, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1182, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1188, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1193, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1201, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1206, .adv_w = 112, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1212, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1218, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1225, .adv_w = 112, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1231, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1236, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1241, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1247, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1253, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1261, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1268, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1276, .adv_w = 112, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1285, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1293, .adv_w = 112, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1302, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1310, .adv_w = 112, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1319, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1327, .adv_w = 112, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1336, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1344, .adv_w = 112, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1353, .adv_w = 112, .box_w = 6, .box_h = 12, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 1362, .adv_w = 112, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 1371, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1379, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1386, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1394, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1401, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1409, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1416, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1424, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1432, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1440, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1447, .adv_w = 112, .box_w = 7, .box_h = 12, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 1458, .adv_w = 112, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 1467, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1475, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1482, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1488, .adv_w = 112, .box_w = 6, .box_h = 1, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 1489, .adv_w = 112, .box_w = 7, .box_h = 1, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 1490, .adv_w = 112, .box_w = 2, .box_h = 4, .ofs_x = 3, .ofs_y = 6},
    {.bitmap_index = 1491, .adv_w = 112, .box_w = 2, .box_h = 4, .ofs_x = 3, .ofs_y = 6},
    {.bitmap_index = 1492, .adv_w = 112, .box_w = 5, .box_h = 4, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 1495, .adv_w = 112, .box_w = 5, .box_h = 4, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 1498, .adv_w = 112, .box_w = 5, .box_h = 4, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1501, .adv_w = 112, .box_w = 7, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1512, .adv_w = 112, .box_w = 7, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1523, .adv_w = 112, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1524, .adv_w = 112, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1532, .adv_w = 112, .box_w = 3, .box_h = 4, .ofs_x = 2, .ofs_y = 6},
    {.bitmap_index = 1534, .adv_w = 112, .box_w = 6, .box_h = 4, .ofs_x = 0, .ofs_y = 6},
    {.bitmap_index = 1537, .adv_w = 112, .box_w = 6, .box_h = 4, .ofs_x = 0, .ofs_y = 6},
    {.bitmap_index = 1540, .adv_w = 112, .box_w = 5, .box_h = 1, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 1541, .adv_w = 112, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1546, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1552, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_1[] = {
    0x0, 0x89, 0x8a, 0x8b, 0x8c, 0xca, 0x138, 0x149,
    0x14a, 0x14b, 0x14c, 0x14e
};

static const uint16_t unicode_list_5[] = {
    0x0, 0x1, 0x8, 0x9, 0x20, 0x21, 0x2e, 0x2f,
    0x32, 0x33, 0x3e, 0x3f, 0x42, 0x43, 0x56, 0x57,
    0x5e, 0x5f, 0x68, 0x69, 0x7a, 0x7b, 0x84, 0x85,
    0x8e, 0x8f, 0x9a, 0x20f, 0x210, 0x214, 0x215, 0x218,
    0x219, 0x21a, 0x21c, 0x21d, 0x22b, 0x22c, 0x22e, 0x22f,
    0x31e, 0x40e, 0x45c, 0x460, 0x461
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 256, .range_length = 128, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 399, .range_length = 335, .glyph_id_start = 129,
        .unicode_list = unicode_list_1, .glyph_id_ofs_list = NULL, .list_length = 12, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    },
    {
        .range_start = 913, .range_length = 17, .glyph_id_start = 141,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 931, .range_length = 7, .glyph_id_start = 158,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 945, .range_length = 25, .glyph_id_start = 165,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 7684, .range_length = 1122, .glyph_id_start = 190,
        .unicode_list = unicode_list_5, .glyph_id_ofs_list = NULL, .list_length = 45, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
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
    .cmap_num = 6,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};

extern const lv_font_t lowres_14_la_next;


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t lowres_latina_14 = {
#else
lv_font_t lowres_latina_14 = {
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
    .fallback = &lowres_14_la_next,
#endif
    .user_data = NULL,
};



#endif /*#if LOWRES_LATINA_14*/

