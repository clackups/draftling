/*******************************************************************************
 * Size: 16 px
 * Bpp: 1
 * Opts: --font Greybeard-16px.ttf -r 0x100-0x17F,0x218-0x21B,0x2C7,0x2D8-0x2DB,0x2DD,0x201D-0x201E --size 16 --bpp 1 --format lvgl --no-compress --lv-fallback lowres_16_la_next --lv-font-name lowres_latina_16 -o lowres_latina_16.c
 ******************************************************************************/

#include "lvgl.h"

#ifndef LOWRES_LATINA_16
#define LOWRES_LATINA_16 1
#endif

#if LOWRES_LATINA_16

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0100 "Ā" */
    0x78, 0x3, 0xc, 0x49, 0x28, 0x61, 0xfe, 0x18,
    0x61,

    /* U+0101 "ā" */
    0x78, 0x0, 0x1e, 0x4, 0x17, 0xe1, 0x8d, 0xd0,

    /* U+0102 "Ă" */
    0x48, 0xc0, 0xc, 0x31, 0x24, 0xa1, 0xfe, 0x18,
    0x61,

    /* U+0103 "ă" */
    0x48, 0xc0, 0x1e, 0x4, 0x17, 0xe1, 0x8d, 0xd0,

    /* U+0104 "Ą" */
    0x30, 0xc4, 0x92, 0x86, 0x1f, 0xe1, 0x86, 0x10,
    0x84, 0xc,

    /* U+0105 "ą" */
    0x78, 0x10, 0x5f, 0x86, 0x37, 0x42, 0x10, 0x30,

    /* U+0106 "Ć" */
    0x8, 0xc0, 0xe, 0x46, 0x8, 0x20, 0x82, 0x4,
    0x4e,

    /* U+0107 "ć" */
    0x10, 0x42, 0x0, 0x7a, 0x18, 0x20, 0x82, 0x17,
    0x80,

    /* U+0108 "Ĉ" */
    0x31, 0x20, 0xe, 0x46, 0x8, 0x20, 0x82, 0x4,
    0x4e,

    /* U+0109 "ĉ" */
    0x31, 0x20, 0x1e, 0x86, 0x8, 0x20, 0x85, 0xe0,

    /* U+010A "Ċ" */
    0x10, 0x40, 0xe, 0x46, 0x8, 0x20, 0x82, 0x4,
    0x4e,

    /* U+010B "ċ" */
    0x10, 0x40, 0x1e, 0x86, 0x8, 0x20, 0x85, 0xe0,

    /* U+010C "Č" */
    0x24, 0x60, 0xe, 0x46, 0x8, 0x20, 0x82, 0x4,
    0x4e,

    /* U+010D "č" */
    0x48, 0xc0, 0x1e, 0x86, 0x8, 0x20, 0x85, 0xe0,

    /* U+010E "Ď" */
    0x91, 0x80, 0x3c, 0x8a, 0x18, 0x61, 0x86, 0x18,
    0xbc,

    /* U+010F "ď" */
    0x6, 0x14, 0x28, 0x66, 0x93, 0x22, 0x44, 0x89,
    0x31, 0xa0,

    /* U+0110 "Đ" */
    0x78, 0x89, 0xa, 0x1f, 0x28, 0x50, 0xa1, 0x44,
    0xf0,

    /* U+0111 "đ" */
    0x4, 0x3c, 0x13, 0xa8, 0xd0, 0xa1, 0x42, 0x8c,
    0xe8,

    /* U+0112 "Ē" */
    0x78, 0xf, 0xe0, 0x82, 0xf, 0x20, 0x82, 0x8,
    0x3f,

    /* U+0113 "ē" */
    0x78, 0x0, 0x1e, 0x86, 0x1f, 0xe0, 0x81, 0xe0,

    /* U+0114 "Ĕ" */
    0x48, 0xc0, 0x3f, 0x82, 0x8, 0x3c, 0x82, 0x8,
    0x3f,

    /* U+0115 "ĕ" */
    0x48, 0xc0, 0x1e, 0x86, 0x1f, 0xe0, 0x81, 0xe0,

    /* U+0116 "Ė" */
    0x20, 0x80, 0x3f, 0x82, 0x8, 0x3c, 0x82, 0x8,
    0x3f,

    /* U+0117 "ė" */
    0x10, 0x40, 0x1e, 0x86, 0x1f, 0xe0, 0x81, 0xe0,

    /* U+0118 "Ę" */
    0xfe, 0x8, 0x20, 0xf2, 0x8, 0x20, 0x83, 0xf0,
    0x84, 0xc,

    /* U+0119 "ę" */
    0x7a, 0x18, 0x7f, 0x82, 0x7, 0x84, 0x20, 0x60,

    /* U+011A "Ě" */
    0x48, 0xc0, 0x3f, 0x82, 0x8, 0x3c, 0x82, 0x8,
    0x3f,

    /* U+011B "ě" */
    0x48, 0xc0, 0x1e, 0x86, 0x1f, 0xe0, 0x81, 0xe0,

    /* U+011C "Ĝ" */
    0x31, 0x20, 0xe, 0x46, 0x8, 0x23, 0x86, 0x14,
    0x4f,

    /* U+011D "ĝ" */
    0x31, 0x20, 0x1d, 0x8a, 0x28, 0x9c, 0x81, 0xe8,
    0x61, 0x78,

    /* U+011E "Ğ" */
    0x24, 0x60, 0xe, 0x46, 0x8, 0x23, 0x86, 0x14,
    0x4f,

    /* U+011F "ğ" */
    0x48, 0xc0, 0x1d, 0x8a, 0x28, 0x9c, 0x81, 0xe8,
    0x61, 0x78,

    /* U+0120 "Ġ" */
    0x10, 0x40, 0xe, 0x46, 0x8, 0x23, 0x86, 0x14,
    0x4f,

    /* U+0121 "ġ" */
    0x20, 0x80, 0x1d, 0x8a, 0x28, 0x9c, 0x81, 0xe8,
    0x61, 0x78,

    /* U+0122 "Ģ" */
    0x39, 0x18, 0x20, 0x82, 0x38, 0x61, 0x44, 0xf2,
    0x4, 0x20,

    /* U+0123 "ģ" */
    0x10, 0x81, 0x0, 0x76, 0x28, 0xa2, 0x72, 0x7,
    0xa1, 0x85, 0xe0,

    /* U+0124 "Ĥ" */
    0x31, 0x20, 0x21, 0x86, 0x18, 0x7f, 0x86, 0x18,
    0x61,

    /* U+0125 "ĥ" */
    0x18, 0x98, 0x20, 0x82, 0xec, 0x61, 0x86, 0x18,
    0x61,

    /* U+0126 "Ħ" */
    0x86, 0x18, 0x7f, 0x87, 0xf8, 0x61, 0x86, 0x10,

    /* U+0127 "ħ" */
    0x41, 0xe1, 0x2, 0xe6, 0x28, 0x50, 0xa1, 0x42,
    0x84,

    /* U+0128 "Ĩ" */
    0x4d, 0x81, 0xf2, 0x10, 0x84, 0x21, 0x9, 0xf0,

    /* U+0129 "ĩ" */
    0x4d, 0x80, 0xc2, 0x10, 0x84, 0x27, 0xc0,

    /* U+012A "Ī" */
    0x78, 0x3e, 0x42, 0x10, 0x84, 0x21, 0x9, 0xf0,

    /* U+012B "ī" */
    0xf0, 0x0, 0xc2, 0x10, 0x84, 0x27, 0xc0,

    /* U+012C "Ĭ" */
    0x8b, 0x81, 0xf2, 0x10, 0x84, 0x21, 0x9, 0xf0,

    /* U+012D "ĭ" */
    0x93, 0x0, 0xc2, 0x10, 0x84, 0x27, 0xc0,

    /* U+012E "Į" */
    0xf9, 0x8, 0x42, 0x10, 0x84, 0x27, 0xc4, 0x41,
    0x80,

    /* U+012F "į" */
    0x21, 0x0, 0xc2, 0x10, 0x84, 0x27, 0xc4, 0x41,
    0x80,

    /* U+0130 "İ" */
    0x21, 0x1, 0xf2, 0x10, 0x84, 0x21, 0x9, 0xf0,

    /* U+0131 "ı" */
    0x61, 0x8, 0x42, 0x13, 0xe0,

    /* U+0132 "Ĳ" */
    0xe7, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x52,
    0x52, 0xec,

    /* U+0133 "ĳ" */
    0x45, 0x10, 0x33, 0x45, 0x14, 0x51, 0x47, 0x90,
    0x41, 0x18,

    /* U+0134 "Ĵ" */
    0x18, 0x90, 0x7, 0x8, 0x20, 0x82, 0xa, 0x28,
    0x9c,

    /* U+0135 "ĵ" */
    0x18, 0x90, 0xe, 0x8, 0x20, 0x82, 0x8, 0x28,
    0xa2, 0x70,

    /* U+0136 "Ķ" */
    0x86, 0x18, 0xa4, 0xa3, 0x89, 0x22, 0x86, 0x12,
    0x4, 0x20,

    /* U+0137 "ķ" */
    0x82, 0x8, 0x21, 0x8a, 0x4e, 0x24, 0x8a, 0x12,
    0x4, 0x20,

    /* U+0138 "ĸ" */
    0x86, 0x29, 0x38, 0x92, 0x28, 0x40,

    /* U+0139 "Ĺ" */
    0x8, 0xc0, 0x20, 0x82, 0x8, 0x20, 0x82, 0x8,
    0x3f,

    /* U+013A "ĺ" */
    0x13, 0x0, 0xc2, 0x10, 0x84, 0x21, 0x9, 0xf0,

    /* U+013B "Ļ" */
    0x82, 0x8, 0x20, 0x82, 0x8, 0x20, 0x83, 0xf2,
    0x4, 0x20,

    /* U+013C "ļ" */
    0x61, 0x8, 0x42, 0x10, 0x84, 0x27, 0xc8, 0x22,
    0x0,

    /* U+013D "Ľ" */
    0xe, 0x18, 0x62, 0x82, 0x8, 0x20, 0x82, 0xf,
    0xc0,

    /* U+013E "ľ" */
    0xd, 0x92, 0x4a, 0x20, 0x82, 0x8, 0x20, 0x8f,
    0x80,

    /* U+013F "Ŀ" */
    0x82, 0x8, 0x20, 0x86, 0x18, 0x20, 0x83, 0xf0,

    /* U+0140 "ŀ" */
    0x60, 0x40, 0x81, 0x2, 0x24, 0x48, 0x10, 0x21,
    0xf0,

    /* U+0141 "Ł" */
    0x40, 0x81, 0x2, 0x86, 0x18, 0x10, 0x20, 0x40,
    0xfc,

    /* U+0142 "ł" */
    0x61, 0x8, 0x43, 0x30, 0x84, 0x27, 0xc0,

    /* U+0143 "Ń" */
    0x8, 0xc0, 0x21, 0xc6, 0x9a, 0x65, 0x96, 0x38,
    0x61,

    /* U+0144 "ń" */
    0x10, 0x42, 0x0, 0xbb, 0x18, 0x61, 0x86, 0x18,
    0x40,

    /* U+0145 "Ņ" */
    0x86, 0x1c, 0x69, 0xa6, 0x59, 0x63, 0x86, 0x12,
    0x4, 0x20,

    /* U+0146 "ņ" */
    0xbb, 0x18, 0x61, 0x86, 0x18, 0x48, 0x10, 0x80,

    /* U+0147 "Ň" */
    0x48, 0xc0, 0x21, 0xc6, 0x9a, 0x65, 0x96, 0x38,
    0x61,

    /* U+0148 "ň" */
    0x48, 0xc0, 0x2e, 0xc6, 0x18, 0x61, 0x86, 0x10,

    /* U+0149 "ŉ" */
    0xc0, 0x40, 0x40, 0x80, 0x2e, 0x31, 0x21, 0x21,
    0x21, 0x21, 0x21,

    /* U+014A "Ŋ" */
    0xbb, 0x18, 0x61, 0x86, 0x18, 0x61, 0x96, 0x20,

    /* U+014B "ŋ" */
    0xbb, 0x18, 0x61, 0x86, 0x18, 0x41, 0x24, 0x60,

    /* U+014C "Ō" */
    0x78, 0x7, 0xa1, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x5e,

    /* U+014D "ō" */
    0x78, 0x0, 0x1e, 0x86, 0x18, 0x61, 0x85, 0xe0,

    /* U+014E "Ŏ" */
    0x48, 0xc0, 0x1e, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x5e,

    /* U+014F "ŏ" */
    0x48, 0xc0, 0x1e, 0x86, 0x18, 0x61, 0x85, 0xe0,

    /* U+0150 "Ő" */
    0x25, 0x20, 0x1e, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x5e,

    /* U+0151 "ő" */
    0x24, 0x94, 0x80, 0x7a, 0x18, 0x61, 0x86, 0x17,
    0x80,

    /* U+0152 "Œ" */
    0x7f, 0x22, 0x44, 0x89, 0xd2, 0x24, 0x48, 0x90,
    0xfc,

    /* U+0153 "œ" */
    0x6d, 0x26, 0x4c, 0xf9, 0x12, 0x1b, 0x80,

    /* U+0154 "Ŕ" */
    0x8, 0xc0, 0x3e, 0x86, 0x18, 0x7e, 0x92, 0x28,
    0x61,

    /* U+0155 "ŕ" */
    0x10, 0x42, 0x0, 0xdd, 0x94, 0x10, 0x41, 0x4,
    0x0,

    /* U+0156 "Ŗ" */
    0xfa, 0x18, 0x61, 0xfa, 0x48, 0xa2, 0x86, 0x12,
    0x4, 0x20,

    /* U+0157 "ŗ" */
    0xdd, 0x94, 0x10, 0x41, 0x4, 0x8, 0x10, 0x80,

    /* U+0158 "Ř" */
    0x48, 0xc0, 0x3e, 0x86, 0x18, 0x7e, 0x92, 0x28,
    0x61,

    /* U+0159 "ř" */
    0x48, 0xc0, 0x37, 0x65, 0x4, 0x10, 0x41, 0x0,

    /* U+015A "Ś" */
    0x8, 0xc0, 0x1e, 0x86, 0x4, 0xc, 0x8, 0x18,
    0x5e,

    /* U+015B "ś" */
    0x10, 0x42, 0x0, 0x7a, 0x18, 0x1e, 0x6, 0x17,
    0x80,

    /* U+015C "Ŝ" */
    0x31, 0x20, 0x1e, 0x86, 0x4, 0xc, 0x8, 0x18,
    0x5e,

    /* U+015D "ŝ" */
    0x31, 0x20, 0x1e, 0x86, 0x7, 0x81, 0x85, 0xe0,

    /* U+015E "Ş" */
    0x7a, 0x18, 0x20, 0x60, 0x60, 0x41, 0x85, 0xe2,
    0x4, 0x60,

    /* U+015F "ş" */
    0x7a, 0x18, 0x1e, 0x6, 0x17, 0x88, 0x11, 0x80,

    /* U+0160 "Š" */
    0x48, 0xc0, 0x1e, 0x86, 0x4, 0xc, 0x8, 0x18,
    0x5e,

    /* U+0161 "š" */
    0x48, 0xc0, 0x1e, 0x86, 0x7, 0x81, 0x85, 0xe0,

    /* U+0162 "Ţ" */
    0xfe, 0x20, 0x40, 0x81, 0x2, 0x4, 0x8, 0x10,
    0x20, 0x60, 0x43, 0x0,

    /* U+0163 "ţ" */
    0x20, 0x8f, 0x88, 0x20, 0x82, 0x9, 0x18, 0x40,
    0x8c,

    /* U+0164 "Ť" */
    0x48, 0x60, 0x7, 0xf1, 0x2, 0x4, 0x8, 0x10,
    0x20, 0x40, 0x80,

    /* U+0165 "ť" */
    0x6, 0x4, 0x89, 0x2f, 0x84, 0x8, 0x10, 0x20,
    0x48, 0x60,

    /* U+0166 "Ŧ" */
    0xfe, 0x20, 0x40, 0x81, 0xf, 0x84, 0x8, 0x10,
    0x20,

    /* U+0167 "ŧ" */
    0x20, 0x8f, 0x88, 0xf8, 0x82, 0x9, 0x18,

    /* U+0168 "Ũ" */
    0x66, 0x60, 0x21, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x5e,

    /* U+0169 "ũ" */
    0x66, 0x60, 0x21, 0x86, 0x18, 0x61, 0x8d, 0xd0,

    /* U+016A "Ū" */
    0x78, 0x8, 0x61, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x5e,

    /* U+016B "ū" */
    0x78, 0x0, 0x21, 0x86, 0x18, 0x61, 0x8d, 0xd0,

    /* U+016C "Ŭ" */
    0x48, 0xc0, 0x21, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x5e,

    /* U+016D "ŭ" */
    0x48, 0xc0, 0x21, 0x86, 0x18, 0x61, 0x8d, 0xd0,

    /* U+016E "Ů" */
    0x31, 0x23, 0x21, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x5e,

    /* U+016F "ů" */
    0x31, 0x23, 0x0, 0x86, 0x18, 0x61, 0x86, 0x37,
    0x40,

    /* U+0170 "Ű" */
    0x25, 0x20, 0x21, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x5e,

    /* U+0171 "ű" */
    0x24, 0x94, 0x80, 0x86, 0x18, 0x61, 0x86, 0x37,
    0x40,

    /* U+0172 "Ų" */
    0x86, 0x18, 0x61, 0x86, 0x18, 0x61, 0x85, 0xe0,
    0x84, 0xc,

    /* U+0173 "ų" */
    0x86, 0x18, 0x61, 0x86, 0x37, 0x42, 0x10, 0x30,

    /* U+0174 "Ŵ" */
    0x18, 0x48, 0x4, 0x18, 0x32, 0x64, 0xd5, 0xaa,
    0x89, 0x12, 0x20,

    /* U+0175 "ŵ" */
    0x18, 0x48, 0x4, 0x19, 0x32, 0x64, 0xb6, 0x44,
    0x88,

    /* U+0176 "Ŷ" */
    0x18, 0x48, 0x4, 0x14, 0x48, 0x8a, 0x14, 0x10,
    0x20, 0x40, 0x80,

    /* U+0177 "ŷ" */
    0x31, 0x20, 0x21, 0x85, 0x14, 0x8a, 0x28, 0x41,
    0x28, 0x40,

    /* U+0178 "Ÿ" */
    0x24, 0x48, 0x4, 0x14, 0x48, 0x8a, 0x14, 0x10,
    0x20, 0x40, 0x80,

    /* U+0179 "Ź" */
    0x8, 0xc0, 0x3f, 0x8, 0x21, 0x4, 0x20, 0x84,
    0x3f,

    /* U+017A "ź" */
    0x10, 0x42, 0x0, 0xfc, 0x21, 0x8, 0x42, 0xf,
    0xc0,

    /* U+017B "Ż" */
    0x20, 0x80, 0x3f, 0x8, 0x21, 0x4, 0x20, 0x84,
    0x3f,

    /* U+017C "ż" */
    0x20, 0x80, 0x3f, 0x8, 0x42, 0x10, 0x83, 0xf0,

    /* U+017D "Ž" */
    0x48, 0xc0, 0x3f, 0x8, 0x21, 0x4, 0x20, 0x84,
    0x3f,

    /* U+017E "ž" */
    0x48, 0xc0, 0x3f, 0x8, 0x42, 0x10, 0x83, 0xf0,

    /* U+017F "ſ" */
    0x79, 0x88, 0x88, 0x88, 0x88,

    /* U+0218 "Ș" */
    0x7a, 0x18, 0x20, 0x60, 0x60, 0x41, 0x85, 0xe2,
    0x4, 0x20,

    /* U+0219 "ș" */
    0x7a, 0x18, 0x1e, 0x6, 0x17, 0x88, 0x10, 0x80,

    /* U+021A "Ț" */
    0xfe, 0x20, 0x40, 0x81, 0x2, 0x4, 0x8, 0x10,
    0x20, 0x20, 0x41, 0x0,

    /* U+021B "ț" */
    0x20, 0x8f, 0x88, 0x20, 0x82, 0x9, 0x18, 0x40,
    0x84,

    /* U+02C7 "ˇ" */
    0x8a, 0x88,

    /* U+02D8 "˘" */
    0x99, 0x60,

    /* U+02D9 "˙" */
    0xf0,

    /* U+02DA "˚" */
    0x69, 0x96,

    /* U+02DB "˛" */
    0x5a, 0x30,

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
    {.bitmap_index = 0, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 9, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 17, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 26, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 34, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 44, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 52, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 61, .adv_w = 128, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 70, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 79, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 87, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 96, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 104, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 113, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 121, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 130, .adv_w = 128, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 140, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 149, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 158, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 167, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 175, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 184, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 192, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 201, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 209, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 219, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 227, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 236, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 244, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 253, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 263, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 272, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 282, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 291, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 301, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 311, .adv_w = 128, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 322, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 331, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 340, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 348, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 357, .adv_w = 128, .box_w = 5, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 365, .adv_w = 128, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 372, .adv_w = 128, .box_w = 5, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 380, .adv_w = 128, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 387, .adv_w = 128, .box_w = 5, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 395, .adv_w = 128, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 402, .adv_w = 128, .box_w = 5, .box_h = 13, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 411, .adv_w = 128, .box_w = 5, .box_h = 13, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 420, .adv_w = 128, .box_w = 5, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 428, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 433, .adv_w = 128, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 443, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 453, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 462, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 472, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 482, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 492, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 498, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 507, .adv_w = 128, .box_w = 5, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 515, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 525, .adv_w = 128, .box_w = 5, .box_h = 13, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 534, .adv_w = 128, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 543, .adv_w = 128, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 552, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 560, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 569, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 578, .adv_w = 128, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 585, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 594, .adv_w = 128, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 603, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 613, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 621, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 630, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 638, .adv_w = 128, .box_w = 8, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 649, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 657, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 665, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 674, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 682, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 691, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 699, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 708, .adv_w = 128, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 717, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 726, .adv_w = 128, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 733, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 742, .adv_w = 128, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 751, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 761, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 769, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 778, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 786, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 795, .adv_w = 128, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 804, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 813, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 821, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 831, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 839, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 848, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 856, .adv_w = 128, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 868, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 877, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 888, .adv_w = 128, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 898, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 907, .adv_w = 128, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 914, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 923, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 931, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 940, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 948, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 957, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 965, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 974, .adv_w = 128, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 983, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 992, .adv_w = 128, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1001, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1011, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1019, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1030, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1039, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1050, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1060, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1071, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1080, .adv_w = 128, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1089, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1098, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1106, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1115, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1123, .adv_w = 128, .box_w = 4, .box_h = 10, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 1128, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1138, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1146, .adv_w = 128, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1158, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1167, .adv_w = 128, .box_w = 5, .box_h = 3, .ofs_x = 2, .ofs_y = 8},
    {.bitmap_index = 1169, .adv_w = 128, .box_w = 4, .box_h = 3, .ofs_x = 2, .ofs_y = 8},
    {.bitmap_index = 1171, .adv_w = 128, .box_w = 2, .box_h = 2, .ofs_x = 3, .ofs_y = 9},
    {.bitmap_index = 1172, .adv_w = 128, .box_w = 4, .box_h = 4, .ofs_x = 2, .ofs_y = 7},
    {.bitmap_index = 1174, .adv_w = 128, .box_w = 3, .box_h = 4, .ofs_x = 4, .ofs_y = -3},
    {.bitmap_index = 1176, .adv_w = 128, .box_w = 5, .box_h = 3, .ofs_x = 2, .ofs_y = 8},
    {.bitmap_index = 1178, .adv_w = 128, .box_w = 5, .box_h = 4, .ofs_x = 2, .ofs_y = 7},
    {.bitmap_index = 1181, .adv_w = 128, .box_w = 5, .box_h = 4, .ofs_x = 1, .ofs_y = -2}
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

extern const lv_font_t lowres_16_la_next;


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t lowres_latina_16 = {
#else
lv_font_t lowres_latina_16 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 15,          /*The maximum line height required by the font*/
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
    .fallback = &lowres_16_la_next,
#endif
    .user_data = NULL,
};



#endif /*#if LOWRES_LATINA_16*/

