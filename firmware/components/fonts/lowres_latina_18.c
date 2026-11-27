/*******************************************************************************
 * Size: 18 px
 * Bpp: 1
 * Opts: --font Greybeard-18px.ttf --autohint-off -r 0x100-0x17F,0x218-0x21B,0x2C7,0x2D8-0x2DB,0x2DD,0x18F,0x1B7,0x259,0x292,0x1E04-0x1E05,0x1E0C-0x1E0D,0x1E24-0x1E25,0x1E32-0x1E33,0x1E36-0x1E37,0x1E42-0x1E43,0x1E46-0x1E47,0x1E5A-0x1E5B,0x1E62-0x1E63,0x1E6C-0x1E6D,0x1E7E-0x1E7F,0x1E88-0x1E89,0x1E92-0x1E93,0x1E9E,0x391-0x3A1,0x3A3-0x3A9,0x3B1-0x3C9,0x2011,0x2013-0x2014,0x2018-0x201A,0x201C-0x201E,0x2020-0x2021,0x202F-0x2030,0x2032-0x2033,0x2039-0x203A,0x2122,0x2212,0x221A,0x221E,0x2243,0x2260,0x2264-0x2265 --size 18 --bpp 1 --format lvgl --no-compress --lv-fallback lowres_18_la_next --lv-font-name lowres_latina_18 -o ./lowres_latina_18.c
 ******************************************************************************/

#include "lvgl.h"

#ifndef LOWRES_LATINA_18
#define LOWRES_LATINA_18 1
#endif

#if LOWRES_LATINA_18

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0100 "Ā" */
    0x7c, 0x0, 0x0, 0x81, 0x2, 0xa, 0x14, 0x44,
    0x89, 0xf4, 0x18, 0x30, 0x40,

    /* U+0101 "ā" */
    0x78, 0x0, 0x1e, 0x4, 0x17, 0xe1, 0x86, 0x37,
    0x40,

    /* U+0102 "Ă" */
    0x44, 0x70, 0x0, 0x81, 0x2, 0xa, 0x14, 0x44,
    0x89, 0xf4, 0x18, 0x30, 0x40,

    /* U+0103 "ă" */
    0x44, 0xe0, 0x1e, 0x4, 0x17, 0xe1, 0x86, 0x37,
    0x40,

    /* U+0104 "Ą" */
    0x10, 0x20, 0x41, 0x42, 0x88, 0x91, 0x3e, 0x83,
    0x6, 0x8, 0x20, 0x80, 0xc0,

    /* U+0105 "ą" */
    0x78, 0x10, 0x5f, 0x86, 0x18, 0xdd, 0x8, 0x40,
    0xc0,

    /* U+0106 "Ć" */
    0x8, 0xc0, 0xe, 0x46, 0x8, 0x20, 0x82, 0x8,
    0x20, 0x44, 0xe0,

    /* U+0107 "ć" */
    0x8, 0x21, 0x0, 0x39, 0x18, 0x20, 0x82, 0x4,
    0x4e,

    /* U+0108 "Ĉ" */
    0x18, 0x90, 0xe, 0x46, 0x8, 0x20, 0x82, 0x8,
    0x20, 0x44, 0xe0,

    /* U+0109 "ĉ" */
    0x18, 0x90, 0xe, 0x46, 0x8, 0x20, 0x81, 0x13,
    0x80,

    /* U+010A "Ċ" */
    0x10, 0x40, 0xe, 0x46, 0x8, 0x20, 0x82, 0x8,
    0x20, 0x44, 0xe0,

    /* U+010B "ċ" */
    0x10, 0x40, 0x0, 0x39, 0x18, 0x20, 0x82, 0x4,
    0x4e,

    /* U+010C "Č" */
    0x24, 0x60, 0xe, 0x46, 0x8, 0x20, 0x82, 0x8,
    0x20, 0x44, 0xe0,

    /* U+010D "č" */
    0x24, 0x60, 0xe, 0x46, 0x8, 0x20, 0x81, 0x13,
    0x80,

    /* U+010E "Ď" */
    0x91, 0x80, 0x3c, 0x8a, 0x18, 0x61, 0x86, 0x18,
    0x61, 0x8b, 0xc0,

    /* U+010F "ď" */
    0x3, 0x5, 0x5, 0x6, 0x74, 0x8c, 0x84, 0x84,
    0x84, 0x84, 0x8c, 0x74,

    /* U+0110 "Đ" */
    0x78, 0x89, 0xa, 0x14, 0x3e, 0x50, 0xa1, 0x42,
    0x89, 0xe0,

    /* U+0111 "đ" */
    0x4, 0x3c, 0x13, 0xa8, 0xd0, 0xa1, 0x42, 0x85,
    0x19, 0xd0,

    /* U+0112 "Ē" */
    0x78, 0x0, 0x3f, 0x82, 0x8, 0x20, 0xf2, 0x8,
    0x20, 0x83, 0xf0,

    /* U+0113 "ē" */
    0x3c, 0x0, 0xe, 0x46, 0x1f, 0xe0, 0x81, 0x13,
    0x80,

    /* U+0114 "Ĕ" */
    0x44, 0xe0, 0x3f, 0x82, 0x8, 0x20, 0xf2, 0x8,
    0x20, 0x83, 0xf0,

    /* U+0115 "ĕ" */
    0x44, 0xe0, 0xe, 0x46, 0x1f, 0xe0, 0x81, 0x13,
    0x80,

    /* U+0116 "Ė" */
    0x20, 0x80, 0x3f, 0x82, 0x8, 0x20, 0xf2, 0x8,
    0x20, 0x83, 0xf0,

    /* U+0117 "ė" */
    0x10, 0x40, 0x0, 0x39, 0x18, 0x7f, 0x82, 0x4,
    0x4e,

    /* U+0118 "Ę" */
    0xfe, 0x8, 0x20, 0x83, 0xc8, 0x20, 0x82, 0xf,
    0xc2, 0x10, 0x30,

    /* U+0119 "ę" */
    0x39, 0x18, 0x7f, 0x82, 0x4, 0x4e, 0x10, 0x81,
    0x80,

    /* U+011A "Ě" */
    0x48, 0xc0, 0x3f, 0x82, 0x8, 0x20, 0xf2, 0x8,
    0x20, 0x83, 0xf0,

    /* U+011B "ě" */
    0x24, 0x60, 0xe, 0x46, 0x1f, 0xe0, 0x81, 0x13,
    0x80,

    /* U+011C "Ĝ" */
    0x18, 0x90, 0xe, 0x46, 0x8, 0x20, 0x9e, 0x18,
    0x61, 0x44, 0xf0,

    /* U+011D "ĝ" */
    0x31, 0x20, 0x1d, 0x8a, 0x28, 0xa2, 0x72, 0x7,
    0xa1, 0x85, 0xe0,

    /* U+011E "Ğ" */
    0x24, 0x60, 0xe, 0x46, 0x8, 0x20, 0x9e, 0x18,
    0x61, 0x44, 0xf0,

    /* U+011F "ğ" */
    0x48, 0xc0, 0x1d, 0x8a, 0x28, 0xa2, 0x72, 0x7,
    0xa1, 0x85, 0xe0,

    /* U+0120 "Ġ" */
    0x10, 0x40, 0xe, 0x46, 0x8, 0x20, 0x9e, 0x18,
    0x61, 0x44, 0xf0,

    /* U+0121 "ġ" */
    0x20, 0x80, 0x0, 0x76, 0x28, 0xa2, 0x89, 0xc8,
    0x1e, 0x86, 0x17, 0x80,

    /* U+0122 "Ģ" */
    0x39, 0x18, 0x20, 0x82, 0x78, 0x61, 0x85, 0x13,
    0xc8, 0x10, 0x80,

    /* U+0123 "ģ" */
    0x10, 0x81, 0x0, 0x76, 0x28, 0xa2, 0x89, 0xc8,
    0x1e, 0x86, 0x17, 0x80,

    /* U+0124 "Ĥ" */
    0x31, 0x20, 0x21, 0x86, 0x18, 0x61, 0xfe, 0x18,
    0x61, 0x86, 0x10,

    /* U+0125 "ĥ" */
    0x31, 0x20, 0x20, 0x82, 0xb, 0xb1, 0x86, 0x18,
    0x61, 0x86, 0x10,

    /* U+0126 "Ħ" */
    0x42, 0x42, 0x42, 0xff, 0x42, 0x7e, 0x42, 0x42,
    0x42, 0x42, 0x42,

    /* U+0127 "ħ" */
    0x41, 0xf1, 0x2, 0xe6, 0x28, 0x50, 0xa1, 0x42,
    0x85, 0x8,

    /* U+0128 "Ĩ" */
    0x4d, 0x81, 0xf2, 0x10, 0x84, 0x21, 0x8, 0x42,
    0x7c,

    /* U+0129 "ĩ" */
    0x4d, 0x80, 0xc2, 0x10, 0x84, 0x21, 0x3e,

    /* U+012A "Ī" */
    0xf8, 0x1, 0xf2, 0x10, 0x84, 0x21, 0x8, 0x42,
    0x7c,

    /* U+012B "ī" */
    0xf0, 0x0, 0xc2, 0x10, 0x84, 0x21, 0x3e,

    /* U+012C "Ĭ" */
    0x8b, 0x81, 0xf2, 0x10, 0x84, 0x21, 0x8, 0x42,
    0x7c,

    /* U+012D "ĭ" */
    0x93, 0x0, 0xc2, 0x10, 0x84, 0x21, 0x3e,

    /* U+012E "Į" */
    0xf9, 0x8, 0x42, 0x10, 0x84, 0x21, 0x3e, 0x22,
    0xc,

    /* U+012F "į" */
    0x21, 0x0, 0x6, 0x10, 0x84, 0x21, 0x9, 0xf1,
    0x10, 0x60,

    /* U+0130 "İ" */
    0x21, 0x1, 0xf2, 0x10, 0x84, 0x21, 0x8, 0x42,
    0x7c,

    /* U+0131 "ı" */
    0x61, 0x8, 0x42, 0x10, 0x9f,

    /* U+0132 "Ĳ" */
    0xe7, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
    0x52, 0x52, 0xec,

    /* U+0133 "ĳ" */
    0x42, 0x84, 0x0, 0xc, 0x68, 0x50, 0xa1, 0x42,
    0x85, 0xf, 0x10, 0x20, 0x43, 0x0,

    /* U+0134 "Ĵ" */
    0x18, 0x90, 0x7, 0x8, 0x20, 0x82, 0x8, 0x20,
    0xa2, 0x89, 0xc0,

    /* U+0135 "ĵ" */
    0x18, 0x90, 0xe, 0x8, 0x20, 0x82, 0x8, 0x20,
    0xa2, 0x89, 0xc0,

    /* U+0136 "Ķ" */
    0x86, 0x18, 0xa2, 0x92, 0x8d, 0x22, 0x8a, 0x18,
    0x48, 0x10, 0x80,

    /* U+0137 "ķ" */
    0x82, 0x8, 0x21, 0x8a, 0x4a, 0x38, 0x92, 0x28,
    0x48, 0x10, 0x80,

    /* U+0138 "ĸ" */
    0x86, 0x29, 0x28, 0xe2, 0x48, 0xa1,

    /* U+0139 "Ĺ" */
    0x11, 0x80, 0x20, 0x82, 0x8, 0x20, 0x82, 0x8,
    0x20, 0x83, 0xf0,

    /* U+013A "ĺ" */
    0x13, 0x0, 0xc2, 0x10, 0x84, 0x21, 0x8, 0x42,
    0x7c,

    /* U+013B "Ļ" */
    0x82, 0x8, 0x20, 0x82, 0x8, 0x20, 0x82, 0xf,
    0xc8, 0x10, 0x80,

    /* U+013C "ļ" */
    0x61, 0x8, 0x42, 0x10, 0x84, 0x21, 0x3e, 0x41,
    0x10,

    /* U+013D "Ľ" */
    0xe, 0x18, 0x62, 0x82, 0x8, 0x20, 0x82, 0x8,
    0x3f,

    /* U+013E "ľ" */
    0x6, 0xc4, 0x89, 0x22, 0x4, 0x8, 0x10, 0x20,
    0x40, 0x87, 0xc0,

    /* U+013F "Ŀ" */
    0x82, 0x8, 0x20, 0x86, 0x18, 0x20, 0x82, 0xf,
    0xc0,

    /* U+0140 "ŀ" */
    0x60, 0x20, 0x20, 0x20, 0x21, 0x21, 0x20, 0x20,
    0x20, 0x20, 0xf8,

    /* U+0141 "Ł" */
    0x40, 0x81, 0x2, 0x5, 0xc, 0x30, 0x20, 0x40,
    0x81, 0xf8,

    /* U+0142 "ł" */
    0x61, 0x8, 0x53, 0x32, 0x84, 0x21, 0x3e,

    /* U+0143 "Ń" */
    0x8, 0xc0, 0x21, 0x87, 0x1c, 0x69, 0x96, 0x58,
    0xe3, 0x86, 0x10,

    /* U+0144 "ń" */
    0x10, 0x42, 0x0, 0xbb, 0x18, 0x61, 0x86, 0x18,
    0x61,

    /* U+0145 "Ņ" */
    0x86, 0x1c, 0x71, 0xa6, 0x59, 0x63, 0x8e, 0x18,
    0x48, 0x10, 0x80,

    /* U+0146 "ņ" */
    0xbb, 0x18, 0x61, 0x86, 0x18, 0x61, 0x20, 0x42,
    0x0,

    /* U+0147 "Ň" */
    0x48, 0xc0, 0x21, 0x87, 0x1c, 0x69, 0x96, 0x58,
    0xe3, 0x86, 0x10,

    /* U+0148 "ň" */
    0x48, 0xc0, 0x2e, 0xc6, 0x18, 0x61, 0x86, 0x18,
    0x40,

    /* U+0149 "ŉ" */
    0xc0, 0x40, 0x40, 0x80, 0x2e, 0x31, 0x21, 0x21,
    0x21, 0x21, 0x21, 0x21,

    /* U+014A "Ŋ" */
    0xbb, 0x18, 0x61, 0x86, 0x18, 0x61, 0x86, 0x59,
    0x80,

    /* U+014B "ŋ" */
    0xbb, 0x18, 0x61, 0x86, 0x18, 0x61, 0x4, 0x91,
    0x80,

    /* U+014C "Ō" */
    0x78, 0x0, 0x1e, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x61, 0x85, 0xe0,

    /* U+014D "ō" */
    0x78, 0x0, 0x1e, 0x86, 0x18, 0x61, 0x86, 0x17,
    0x80,

    /* U+014E "Ŏ" */
    0x48, 0xc0, 0x1e, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x61, 0x85, 0xe0,

    /* U+014F "ŏ" */
    0x48, 0xc0, 0x1e, 0x86, 0x18, 0x61, 0x86, 0x17,
    0x80,

    /* U+0150 "Ő" */
    0x24, 0x94, 0x80, 0x7a, 0x18, 0x61, 0x86, 0x18,
    0x61, 0x85, 0xe0,

    /* U+0151 "ő" */
    0x24, 0x94, 0x80, 0x7a, 0x18, 0x61, 0x86, 0x18,
    0x5e,

    /* U+0152 "Œ" */
    0x7f, 0x88, 0x88, 0x88, 0x88, 0x8e, 0x88, 0x88,
    0x88, 0x88, 0x7f,

    /* U+0153 "œ" */
    0x6d, 0x26, 0x4c, 0xf9, 0x12, 0x24, 0xb6,

    /* U+0154 "Ŕ" */
    0x8, 0xc0, 0x3e, 0x86, 0x18, 0x61, 0xfa, 0x48,
    0xa2, 0x86, 0x10,

    /* U+0155 "ŕ" */
    0x10, 0x42, 0x0, 0xdd, 0x94, 0x10, 0x41, 0x4,
    0x10,

    /* U+0156 "Ŗ" */
    0xfa, 0x18, 0x61, 0x87, 0xe9, 0x22, 0x8a, 0x18,
    0x48, 0x10, 0x80,

    /* U+0157 "ŗ" */
    0xdd, 0x94, 0x10, 0x41, 0x4, 0x10, 0x20, 0x42,
    0x0,

    /* U+0158 "Ř" */
    0x48, 0xc0, 0x3e, 0x86, 0x18, 0x61, 0xfa, 0x48,
    0xa2, 0x86, 0x10,

    /* U+0159 "ř" */
    0x48, 0xc0, 0x37, 0x65, 0x4, 0x10, 0x41, 0x4,
    0x0,

    /* U+015A "Ś" */
    0x8, 0xc0, 0x1e, 0x86, 0x8, 0x10, 0x30, 0x20,
    0x41, 0x85, 0xe0,

    /* U+015B "ś" */
    0x10, 0x42, 0x0, 0x7a, 0x18, 0x18, 0x18, 0x18,
    0x5e,

    /* U+015C "Ŝ" */
    0x31, 0x20, 0x1e, 0x86, 0x8, 0x10, 0x30, 0x20,
    0x41, 0x85, 0xe0,

    /* U+015D "ŝ" */
    0x31, 0x20, 0x1e, 0x86, 0x6, 0x6, 0x6, 0x17,
    0x80,

    /* U+015E "Ş" */
    0x7a, 0x18, 0x20, 0x40, 0xc0, 0x81, 0x6, 0x17,
    0x88, 0x11, 0x80,

    /* U+015F "ş" */
    0x7a, 0x18, 0x18, 0x18, 0x18, 0x5e, 0x20, 0x46,
    0x0,

    /* U+0160 "Š" */
    0x48, 0xc0, 0x1e, 0x86, 0x8, 0x10, 0x30, 0x20,
    0x41, 0x85, 0xe0,

    /* U+0161 "š" */
    0x48, 0xc0, 0x1e, 0x86, 0x6, 0x6, 0x6, 0x17,
    0x80,

    /* U+0162 "Ţ" */
    0xfe, 0x20, 0x40, 0x81, 0x2, 0x4, 0x8, 0x10,
    0x20, 0x40, 0xc0, 0x86, 0x0,

    /* U+0163 "ţ" */
    0x20, 0x8f, 0x88, 0x20, 0x82, 0x8, 0x24, 0x61,
    0x2, 0x30,

    /* U+0164 "Ť" */
    0x24, 0x30, 0x7, 0xf1, 0x2, 0x4, 0x8, 0x10,
    0x20, 0x40, 0x81, 0x2, 0x0,

    /* U+0165 "ť" */
    0x6, 0x4, 0x89, 0x2f, 0x84, 0x8, 0x10, 0x20,
    0x40, 0x90, 0xc0,

    /* U+0166 "Ŧ" */
    0xfe, 0x20, 0x40, 0x81, 0xf, 0x84, 0x8, 0x10,
    0x20, 0x40,

    /* U+0167 "ŧ" */
    0x20, 0x8f, 0x88, 0xf8, 0x82, 0x8, 0x24, 0x60,

    /* U+0168 "Ũ" */
    0x25, 0x60, 0x21, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x61, 0x85, 0xe0,

    /* U+0169 "ũ" */
    0x25, 0x60, 0x21, 0x86, 0x18, 0x61, 0x86, 0x37,
    0x40,

    /* U+016A "Ū" */
    0x78, 0x0, 0x21, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x61, 0x85, 0xe0,

    /* U+016B "ū" */
    0x78, 0x0, 0x21, 0x86, 0x18, 0x61, 0x86, 0x37,
    0x40,

    /* U+016C "Ŭ" */
    0x48, 0xc0, 0x21, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x61, 0x85, 0xe0,

    /* U+016D "ŭ" */
    0x48, 0xc0, 0x21, 0x86, 0x18, 0x61, 0x86, 0x37,
    0x40,

    /* U+016E "Ů" */
    0x31, 0x24, 0x8c, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x61, 0x85, 0xe0,

    /* U+016F "ů" */
    0x31, 0x24, 0x8c, 0x2, 0x18, 0x61, 0x86, 0x18,
    0x63, 0x74,

    /* U+0170 "Ű" */
    0x24, 0x94, 0x80, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x61, 0x85, 0xe0,

    /* U+0171 "ű" */
    0x24, 0x94, 0x80, 0x86, 0x18, 0x61, 0x86, 0x18,
    0xdd,

    /* U+0172 "Ų" */
    0x86, 0x18, 0x61, 0x86, 0x18, 0x61, 0x86, 0x17,
    0x82, 0x10, 0x30,

    /* U+0173 "ų" */
    0x86, 0x18, 0x61, 0x86, 0x18, 0xdd, 0x8, 0x40,
    0xc0,

    /* U+0174 "Ŵ" */
    0x18, 0x48, 0x4, 0x18, 0x30, 0x64, 0xc9, 0x93,
    0x56, 0xaa, 0x24, 0x48, 0x80,

    /* U+0175 "ŵ" */
    0x18, 0x48, 0x4, 0x18, 0x32, 0x64, 0xc9, 0x6c,
    0x89, 0x10,

    /* U+0176 "Ŷ" */
    0x18, 0x48, 0x4, 0x18, 0x28, 0x91, 0x14, 0x28,
    0x20, 0x40, 0x81, 0x2, 0x0,

    /* U+0177 "ŷ" */
    0x31, 0x20, 0x21, 0x86, 0x14, 0x92, 0x28, 0xc1,
    0x8, 0xa1, 0x0,

    /* U+0178 "Ÿ" */
    0x24, 0x48, 0x4, 0x18, 0x28, 0x91, 0x14, 0x28,
    0x20, 0x40, 0x81, 0x2, 0x0,

    /* U+0179 "Ź" */
    0x8, 0xc0, 0x3f, 0x4, 0x20, 0x84, 0x20, 0x84,
    0x20, 0x83, 0xf0,

    /* U+017A "ź" */
    0x10, 0x42, 0x0, 0xfc, 0x10, 0x84, 0x21, 0x8,
    0x3f,

    /* U+017B "Ż" */
    0x20, 0x80, 0x3f, 0x4, 0x20, 0x84, 0x20, 0x84,
    0x20, 0x83, 0xf0,

    /* U+017C "ż" */
    0x20, 0x80, 0x0, 0xfc, 0x10, 0x84, 0x21, 0x8,
    0x3f,

    /* U+017D "Ž" */
    0x48, 0xc0, 0x3f, 0x4, 0x20, 0x84, 0x20, 0x84,
    0x20, 0x83, 0xf0,

    /* U+017E "ž" */
    0x48, 0xc0, 0x3f, 0x4, 0x21, 0x8, 0x42, 0xf,
    0xc0,

    /* U+017F "ſ" */
    0x3a, 0x21, 0x8, 0x42, 0x10, 0x84, 0x20,

    /* U+018F "Ə" */
    0x71, 0x20, 0x41, 0x7, 0xf8, 0x61, 0x85, 0x23,
    0x0,

    /* U+01B7 "Ʒ" */
    0xfc, 0x10, 0x82, 0x10, 0xe0, 0x41, 0x6, 0x17,
    0x80,

    /* U+0218 "Ș" */
    0x7a, 0x18, 0x20, 0x40, 0xc0, 0x81, 0x6, 0x17,
    0x88, 0x10, 0x80,

    /* U+0219 "ș" */
    0x7a, 0x18, 0x18, 0x18, 0x18, 0x5e, 0x20, 0x42,
    0x0,

    /* U+021A "Ț" */
    0xfe, 0x20, 0x40, 0x81, 0x2, 0x4, 0x8, 0x10,
    0x20, 0x40, 0xc0, 0x82, 0x0,

    /* U+021B "ț" */
    0x20, 0x8f, 0x88, 0x20, 0x82, 0x8, 0x24, 0x61,
    0x2, 0x10,

    /* U+0259 "ə" */
    0x72, 0x20, 0x41, 0xfe, 0x18, 0x9c,

    /* U+0292 "ʒ" */
    0xfc, 0x10, 0x82, 0x10, 0xe0, 0x41, 0x6, 0x17,
    0x80,

    /* U+02C7 "ˇ" */
    0x8a, 0x88,

    /* U+02D8 "˘" */
    0x8c, 0x5c,

    /* U+02D9 "˙" */
    0xf0,

    /* U+02DA "˚" */
    0x69, 0x96,

    /* U+02DB "˛" */
    0x52, 0xa0,

    /* U+02DD "˝" */
    0x4a, 0x64,

    /* U+0391 "Α" */
    0x10, 0x20, 0x41, 0x42, 0x88, 0x91, 0x3e, 0x83,
    0x6, 0x8,

    /* U+0392 "Β" */
    0xfa, 0x18, 0x61, 0x8b, 0xe8, 0x61, 0x86, 0x1f,
    0x80,

    /* U+0393 "Γ" */
    0xfe, 0x8, 0x20, 0x82, 0x8, 0x20, 0x82, 0x8,
    0x0,

    /* U+0394 "Δ" */
    0x10, 0x20, 0x41, 0x42, 0x88, 0x91, 0x22, 0x83,
    0x7, 0xf8,

    /* U+0395 "Ε" */
    0xfe, 0x8, 0x20, 0x83, 0xc8, 0x20, 0x82, 0xf,
    0xc0,

    /* U+0396 "Ζ" */
    0xfc, 0x10, 0x82, 0x10, 0x82, 0x10, 0x82, 0xf,
    0xc0,

    /* U+0397 "Η" */
    0x86, 0x18, 0x61, 0x87, 0xf8, 0x61, 0x86, 0x18,
    0x40,

    /* U+0398 "Θ" */
    0x7a, 0x18, 0x61, 0x87, 0xf8, 0x61, 0x86, 0x17,
    0x80,

    /* U+0399 "Ι" */
    0xf9, 0x8, 0x42, 0x10, 0x84, 0x21, 0x3e,

    /* U+039A "Κ" */
    0x86, 0x18, 0xa2, 0x92, 0x8d, 0x22, 0x8a, 0x18,
    0x40,

    /* U+039B "Λ" */
    0x10, 0x20, 0x41, 0x42, 0x88, 0x91, 0x22, 0x83,
    0x6, 0x8,

    /* U+039C "Μ" */
    0x83, 0x7, 0x1e, 0x3a, 0xb5, 0x64, 0xc9, 0x83,
    0x6, 0x8,

    /* U+039D "Ν" */
    0x86, 0x1c, 0x71, 0xa6, 0x59, 0x63, 0x8e, 0x18,
    0x40,

    /* U+039E "Ξ" */
    0xfe, 0x10, 0x0, 0x1, 0xe0, 0x0, 0x2, 0x1f,
    0xc0,

    /* U+039F "Ο" */
    0x7a, 0x18, 0x61, 0x86, 0x18, 0x61, 0x86, 0x17,
    0x80,

    /* U+03A0 "Π" */
    0xfe, 0x18, 0x61, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x40,

    /* U+03A1 "Ρ" */
    0xfa, 0x18, 0x61, 0x87, 0xe8, 0x20, 0x82, 0x8,
    0x0,

    /* U+03A3 "Σ" */
    0xfe, 0x4, 0x8, 0x10, 0x21, 0x8, 0x42, 0xf,
    0xc0,

    /* U+03A4 "Τ" */
    0xfe, 0x20, 0x40, 0x81, 0x2, 0x4, 0x8, 0x10,
    0x20, 0x40,

    /* U+03A5 "Υ" */
    0x83, 0x5, 0x12, 0x22, 0x85, 0x4, 0x8, 0x10,
    0x20, 0x40,

    /* U+03A6 "Φ" */
    0x10, 0x71, 0x54, 0x99, 0x32, 0x64, 0xc9, 0x54,
    0x70, 0x40,

    /* U+03A7 "Χ" */
    0x86, 0x14, 0x92, 0x30, 0xc3, 0x12, 0x4a, 0x18,
    0x40,

    /* U+03A8 "Ψ" */
    0x93, 0x26, 0x4c, 0x99, 0x32, 0x5f, 0x8, 0x10,
    0x20, 0x40,

    /* U+03A9 "Ω" */
    0x38, 0x8a, 0xc, 0x18, 0x30, 0x60, 0xa2, 0x28,
    0x53, 0xb8,

    /* U+03B1 "α" */
    0x32, 0x96, 0x14, 0x28, 0x50, 0x92, 0x99,

    /* U+03B2 "β" */
    0x72, 0x28, 0xa2, 0xb2, 0x28, 0x61, 0x87, 0x1b,
    0xa0, 0x82, 0x0,

    /* U+03B3 "γ" */
    0x86, 0x14, 0x51, 0x28, 0xa1, 0x4, 0x10, 0x41,
    0x0,

    /* U+03B4 "δ" */
    0x39, 0x2, 0xc, 0x4a, 0x18, 0x61, 0x85, 0x23,
    0x0,

    /* U+03B5 "ε" */
    0x7a, 0x18, 0x18, 0x82, 0x8, 0x5e,

    /* U+03B6 "ζ" */
    0xfc, 0x21, 0x4, 0x21, 0x4, 0x20, 0x82, 0x7,
    0x81, 0x8,

    /* U+03B7 "η" */
    0xbb, 0x18, 0x61, 0x86, 0x18, 0x61, 0x4, 0x10,
    0x40,

    /* U+03B8 "θ" */
    0x31, 0x28, 0x61, 0x87, 0xf8, 0x61, 0x85, 0x23,
    0x0,

    /* U+03B9 "ι" */
    0x88, 0x88, 0x88, 0x96,

    /* U+03BA "κ" */
    0x86, 0x29, 0x38, 0xa2, 0x48, 0xa1,

    /* U+03BB "λ" */
    0x41, 0x2, 0x8, 0x10, 0xc2, 0x92, 0x4a, 0x18,
    0x40,

    /* U+03BC "μ" */
    0x85, 0xa, 0x14, 0x28, 0x50, 0xb3, 0x59, 0x81,
    0x2, 0x0,

    /* U+03BD "ν" */
    0x86, 0x14, 0x51, 0x28, 0xa3, 0x4,

    /* U+03BE "ξ" */
    0xfd, 0x8, 0x20, 0x81, 0xe4, 0x20, 0x82, 0x7,
    0x81, 0x8,

    /* U+03BF "ο" */
    0x7a, 0x18, 0x61, 0x86, 0x18, 0x5e,

    /* U+03C0 "π" */
    0xfe, 0x89, 0x12, 0x24, 0x48, 0x91, 0x22,

    /* U+03C1 "ρ" */
    0x7a, 0x18, 0x61, 0x86, 0x1c, 0x6e, 0x82, 0x8,
    0x0,

    /* U+03C2 "ς" */
    0x39, 0x18, 0x20, 0x82, 0x4, 0xe, 0x4, 0x20,

    /* U+03C3 "σ" */
    0x3f, 0x48, 0x84, 0x84, 0x84, 0x84, 0x48, 0x30,

    /* U+03C4 "τ" */
    0xfe, 0x20, 0x40, 0x81, 0x2, 0x4, 0x6,

    /* U+03C5 "υ" */
    0x86, 0x18, 0x61, 0x86, 0x18, 0x5e,

    /* U+03C6 "φ" */
    0xd, 0x26, 0x4c, 0x99, 0x32, 0x55, 0x1c, 0x10,
    0x20, 0x40,

    /* U+03C7 "χ" */
    0x86, 0x14, 0x92, 0x30, 0xc3, 0x12, 0x4a, 0x18,
    0x40,

    /* U+03C8 "ψ" */
    0x10, 0x20, 0x44, 0x99, 0x32, 0x64, 0xc9, 0x92,
    0xa8, 0xe0, 0x81, 0x2, 0x0,

    /* U+03C9 "ω" */
    0x45, 0x6, 0xc, 0x99, 0x32, 0x64, 0xb6,

    /* U+1E04 "Ḅ" */
    0xfa, 0x18, 0x61, 0x8b, 0xe8, 0x61, 0x86, 0x1f,
    0x80, 0x20, 0x80,

    /* U+1E05 "ḅ" */
    0x82, 0x8, 0x2e, 0xc6, 0x18, 0x61, 0x87, 0x1b,
    0x80, 0x20, 0x80,

    /* U+1E0C "Ḍ" */
    0xf2, 0x28, 0x61, 0x86, 0x18, 0x61, 0x86, 0x2f,
    0x0, 0x20, 0x80,

    /* U+1E0D "ḍ" */
    0x4, 0x10, 0x5d, 0x8e, 0x18, 0x61, 0x86, 0x37,
    0x40, 0x10, 0x40,

    /* U+1E24 "Ḥ" */
    0x86, 0x18, 0x61, 0x87, 0xf8, 0x61, 0x86, 0x18,
    0x40, 0x20, 0x80,

    /* U+1E25 "ḥ" */
    0x82, 0x8, 0x2e, 0xc6, 0x18, 0x61, 0x86, 0x18,
    0x40, 0x20, 0x80,

    /* U+1E32 "Ḳ" */
    0x86, 0x18, 0xa2, 0x92, 0x8d, 0x22, 0x8a, 0x18,
    0x40, 0x20, 0x80,

    /* U+1E33 "ḳ" */
    0x82, 0x8, 0x21, 0x8a, 0x4a, 0x38, 0x92, 0x28,
    0x40, 0x20, 0x80,

    /* U+1E36 "Ḷ" */
    0x82, 0x8, 0x20, 0x82, 0x8, 0x20, 0x82, 0xf,
    0xc0, 0x20, 0x80,

    /* U+1E37 "ḷ" */
    0x61, 0x8, 0x42, 0x10, 0x84, 0x21, 0x3e, 0x2,
    0x10,

    /* U+1E42 "Ṃ" */
    0x83, 0x7, 0x1e, 0x3a, 0xb5, 0x64, 0xc9, 0x83,
    0x6, 0x8, 0x1, 0x2, 0x0,

    /* U+1E43 "ṃ" */
    0xed, 0x26, 0x4c, 0x99, 0x32, 0x64, 0xc9, 0x0,
    0x20, 0x40,

    /* U+1E46 "Ṇ" */
    0x86, 0x1c, 0x71, 0xa6, 0x59, 0x63, 0x8e, 0x18,
    0x40, 0x20, 0x80,

    /* U+1E47 "ṇ" */
    0xbb, 0x18, 0x61, 0x86, 0x18, 0x61, 0x0, 0x82,
    0x0,

    /* U+1E5A "Ṛ" */
    0xfa, 0x18, 0x61, 0x87, 0xe9, 0x22, 0x8a, 0x18,
    0x40, 0x20, 0x80,

    /* U+1E5B "ṛ" */
    0xdd, 0x94, 0x10, 0x41, 0x4, 0x10, 0x1, 0x4,
    0x0,

    /* U+1E62 "Ṣ" */
    0x7a, 0x18, 0x20, 0x40, 0xc0, 0x81, 0x6, 0x17,
    0x80, 0x20, 0x80,

    /* U+1E63 "ṣ" */
    0x7a, 0x18, 0x18, 0x18, 0x18, 0x5e, 0x0, 0x82,
    0x0,

    /* U+1E6C "Ṭ" */
    0xfe, 0x20, 0x40, 0x81, 0x2, 0x4, 0x8, 0x10,
    0x20, 0x40, 0x1, 0x2, 0x0,

    /* U+1E6D "ṭ" */
    0x20, 0x8f, 0x88, 0x20, 0x82, 0x8, 0x24, 0x60,
    0x4, 0x10,

    /* U+1E7E "Ṿ" */
    0x83, 0x6, 0xa, 0x24, 0x48, 0x8a, 0x14, 0x10,
    0x20, 0x40, 0x1, 0x2, 0x0,

    /* U+1E7F "ṿ" */
    0x83, 0x5, 0x12, 0x22, 0x85, 0x4, 0x8, 0x0,
    0x20, 0x40,

    /* U+1E88 "Ẉ" */
    0x83, 0x6, 0xc, 0x99, 0x32, 0x6a, 0xd5, 0x44,
    0x89, 0x10, 0x1, 0x2, 0x0,

    /* U+1E89 "ẉ" */
    0x83, 0x6, 0x4c, 0x99, 0x2d, 0x91, 0x22, 0x0,
    0x20, 0x40,

    /* U+1E92 "Ẓ" */
    0xfc, 0x10, 0x82, 0x10, 0x82, 0x10, 0x82, 0xf,
    0xc0, 0x20, 0x80,

    /* U+1E93 "ẓ" */
    0xfc, 0x10, 0x84, 0x21, 0x8, 0x3f, 0x0, 0x82,
    0x0,

    /* U+1E9E "ẞ" */
    0xfd, 0xa, 0x24, 0x49, 0x90, 0xa0, 0xc1, 0x83,
    0x6, 0x70,

    /* U+2011 "‑" */
    0xfc,

    /* U+2013 "–" */
    0xff,

    /* U+2014 "—" */
    0xff, 0x80,

    /* U+2018 "‘" */
    0x6d,

    /* U+2019 "’" */
    0xb6,

    /* U+201A "‚" */
    0xb6,

    /* U+201C "“" */
    0x4c, 0xb6, 0x90,

    /* U+201D "”" */
    0x96, 0xd3, 0x20,

    /* U+201E "„" */
    0x96, 0xd3, 0x20,

    /* U+2020 "†" */
    0x10, 0x70, 0x47, 0xf5, 0x42, 0x4, 0x8, 0x10,
    0x20, 0x40, 0x81, 0x0,

    /* U+2021 "‡" */
    0x10, 0x70, 0x47, 0xf5, 0x42, 0x4, 0x8, 0x55,
    0xfc, 0x41, 0xc1, 0x0,

    /* U+202F " " */
    0x0,

    /* U+2030 "‰" */
    0x40, 0xa0, 0xa3, 0xa4, 0x58, 0x20, 0xd2, 0x2d,
    0x2d, 0x2d, 0x12,

    /* U+2032 "′" */
    0x2d, 0x40,

    /* U+2033 "″" */
    0x25, 0xb4, 0xa4,

    /* U+2039 "‹" */
    0x2a, 0x22,

    /* U+203A "›" */
    0x88, 0xa8,

    /* U+2122 "™" */
    0xf1, 0x5b, 0x5b, 0x55, 0x51, 0x51,

    /* U+2212 "−" */
    0xfe,

    /* U+221A "√" */
    0x3, 0x2, 0x2, 0x2, 0x4, 0x4, 0xe4, 0x28,
    0x28, 0x18, 0x10, 0x10, 0x10,

    /* U+221E "∞" */
    0x66, 0x99, 0x91, 0x89, 0x99, 0x66,

    /* U+2243 "≃" */
    0x66, 0xd9, 0x80, 0x3, 0xf0,

    /* U+2260 "≠" */
    0x4, 0x2f, 0xc4, 0x23, 0xf4, 0x20,

    /* U+2264 "≤" */
    0x4, 0x66, 0x20, 0x81, 0x81, 0x81, 0x3, 0xf0,

    /* U+2265 "≥" */
    0x81, 0x81, 0x81, 0x4, 0x66, 0x20, 0x3, 0xf0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 144, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 13, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 22, .adv_w = 144, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 35, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 44, .adv_w = 144, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 57, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 66, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 77, .adv_w = 144, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 86, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 97, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 106, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 117, .adv_w = 144, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 126, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 137, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 146, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 157, .adv_w = 144, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 169, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 179, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 189, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 200, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 209, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 220, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 229, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 240, .adv_w = 144, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 249, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 260, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 269, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 280, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 289, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 300, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 311, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 322, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 333, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 344, .adv_w = 144, .box_w = 6, .box_h = 15, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 356, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 367, .adv_w = 144, .box_w = 6, .box_h = 15, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 379, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 390, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 401, .adv_w = 144, .box_w = 8, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 412, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 422, .adv_w = 144, .box_w = 5, .box_h = 14, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 431, .adv_w = 144, .box_w = 5, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 438, .adv_w = 144, .box_w = 5, .box_h = 14, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 447, .adv_w = 144, .box_w = 5, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 454, .adv_w = 144, .box_w = 5, .box_h = 14, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 463, .adv_w = 144, .box_w = 5, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 470, .adv_w = 144, .box_w = 5, .box_h = 14, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 479, .adv_w = 144, .box_w = 5, .box_h = 15, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 489, .adv_w = 144, .box_w = 5, .box_h = 14, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 498, .adv_w = 144, .box_w = 5, .box_h = 8, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 503, .adv_w = 144, .box_w = 8, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 514, .adv_w = 144, .box_w = 7, .box_h = 15, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 528, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 539, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 550, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 561, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 572, .adv_w = 144, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 578, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 589, .adv_w = 144, .box_w = 5, .box_h = 14, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 598, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 609, .adv_w = 144, .box_w = 5, .box_h = 14, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 618, .adv_w = 144, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 627, .adv_w = 144, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 638, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 647, .adv_w = 144, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 658, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 668, .adv_w = 144, .box_w = 5, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 675, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 686, .adv_w = 144, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 695, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 706, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 715, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 726, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 735, .adv_w = 144, .box_w = 8, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 747, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 756, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 765, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 776, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 785, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 796, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 805, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 816, .adv_w = 144, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 825, .adv_w = 144, .box_w = 8, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 836, .adv_w = 144, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 843, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 854, .adv_w = 144, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 863, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 874, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 883, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 894, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 903, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 914, .adv_w = 144, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 923, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 934, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 943, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 954, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 963, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 974, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 983, .adv_w = 144, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 996, .adv_w = 144, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1006, .adv_w = 144, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1019, .adv_w = 144, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1030, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1040, .adv_w = 144, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1048, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1059, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1068, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1079, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1088, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1099, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1108, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1119, .adv_w = 144, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1129, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1140, .adv_w = 144, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1149, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1160, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1169, .adv_w = 144, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1182, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1192, .adv_w = 144, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1205, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1216, .adv_w = 144, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1229, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1240, .adv_w = 144, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1249, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1260, .adv_w = 144, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1269, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1280, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1289, .adv_w = 144, .box_w = 5, .box_h = 11, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 1296, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1305, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1314, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1325, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1334, .adv_w = 144, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1347, .adv_w = 144, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1357, .adv_w = 144, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1363, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1372, .adv_w = 144, .box_w = 5, .box_h = 3, .ofs_x = 2, .ofs_y = 9},
    {.bitmap_index = 1374, .adv_w = 144, .box_w = 5, .box_h = 3, .ofs_x = 2, .ofs_y = 9},
    {.bitmap_index = 1376, .adv_w = 144, .box_w = 2, .box_h = 2, .ofs_x = 3, .ofs_y = 10},
    {.bitmap_index = 1377, .adv_w = 144, .box_w = 4, .box_h = 4, .ofs_x = 2, .ofs_y = 8},
    {.bitmap_index = 1379, .adv_w = 144, .box_w = 3, .box_h = 4, .ofs_x = 4, .ofs_y = -3},
    {.bitmap_index = 1381, .adv_w = 144, .box_w = 5, .box_h = 3, .ofs_x = 2, .ofs_y = 9},
    {.bitmap_index = 1383, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1393, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1402, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 1411, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1421, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1430, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1439, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1448, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1457, .adv_w = 144, .box_w = 5, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 1464, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1473, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1483, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1493, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1502, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1511, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1520, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1529, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1538, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1547, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1557, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1567, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1577, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1586, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1596, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1606, .adv_w = 144, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1613, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1624, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1633, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1642, .adv_w = 144, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1648, .adv_w = 144, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1658, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1667, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1676, .adv_w = 144, .box_w = 4, .box_h = 8, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 1680, .adv_w = 144, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1686, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1695, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1705, .adv_w = 144, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1711, .adv_w = 144, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1721, .adv_w = 144, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1727, .adv_w = 144, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1734, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1743, .adv_w = 144, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1751, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1759, .adv_w = 144, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1766, .adv_w = 144, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1772, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1782, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1791, .adv_w = 144, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1804, .adv_w = 144, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1811, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1822, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1833, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1844, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1855, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1866, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1877, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1888, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1899, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1910, .adv_w = 144, .box_w = 5, .box_h = 14, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 1919, .adv_w = 144, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1932, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1942, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1953, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1962, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1973, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1982, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1993, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2002, .adv_w = 144, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2015, .adv_w = 144, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2025, .adv_w = 144, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2038, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2048, .adv_w = 144, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2061, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2071, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2082, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2091, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2101, .adv_w = 144, .box_w = 6, .box_h = 1, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 2102, .adv_w = 144, .box_w = 8, .box_h = 1, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 2103, .adv_w = 144, .box_w = 9, .box_h = 1, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 2105, .adv_w = 144, .box_w = 2, .box_h = 4, .ofs_x = 3, .ofs_y = 8},
    {.bitmap_index = 2106, .adv_w = 144, .box_w = 2, .box_h = 4, .ofs_x = 3, .ofs_y = 8},
    {.bitmap_index = 2107, .adv_w = 144, .box_w = 2, .box_h = 4, .ofs_x = 3, .ofs_y = -2},
    {.bitmap_index = 2108, .adv_w = 144, .box_w = 5, .box_h = 4, .ofs_x = 2, .ofs_y = 8},
    {.bitmap_index = 2111, .adv_w = 144, .box_w = 5, .box_h = 4, .ofs_x = 2, .ofs_y = 8},
    {.bitmap_index = 2114, .adv_w = 144, .box_w = 5, .box_h = 4, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 2117, .adv_w = 144, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 2129, .adv_w = 144, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 2141, .adv_w = 144, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2142, .adv_w = 144, .box_w = 8, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2153, .adv_w = 144, .box_w = 3, .box_h = 4, .ofs_x = 3, .ofs_y = 8},
    {.bitmap_index = 2155, .adv_w = 144, .box_w = 6, .box_h = 4, .ofs_x = 1, .ofs_y = 8},
    {.bitmap_index = 2158, .adv_w = 144, .box_w = 3, .box_h = 5, .ofs_x = 3, .ofs_y = 2},
    {.bitmap_index = 2160, .adv_w = 144, .box_w = 3, .box_h = 5, .ofs_x = 3, .ofs_y = 2},
    {.bitmap_index = 2162, .adv_w = 144, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = 6},
    {.bitmap_index = 2168, .adv_w = 144, .box_w = 7, .box_h = 1, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 2169, .adv_w = 144, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 2182, .adv_w = 144, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 2188, .adv_w = 144, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 2193, .adv_w = 144, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2199, .adv_w = 144, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2207, .adv_w = 144, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_1[] = {
    0x0, 0x28, 0x89, 0x8a, 0x8b, 0x8c, 0xca, 0x103,
    0x138, 0x149, 0x14a, 0x14b, 0x14c, 0x14e
};

static const uint16_t unicode_list_5[] = {
    0x0, 0x1, 0x8, 0x9, 0x20, 0x21, 0x2e, 0x2f,
    0x32, 0x33, 0x3e, 0x3f, 0x42, 0x43, 0x56, 0x57,
    0x5e, 0x5f, 0x68, 0x69, 0x7a, 0x7b, 0x84, 0x85,
    0x8e, 0x8f, 0x9a, 0x20d, 0x20f, 0x210, 0x214, 0x215,
    0x216, 0x218, 0x219, 0x21a, 0x21c, 0x21d, 0x22b, 0x22c,
    0x22e, 0x22f, 0x235, 0x236, 0x31e, 0x40e, 0x416, 0x41a,
    0x43f, 0x45c, 0x460, 0x461
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
        .unicode_list = unicode_list_1, .glyph_id_ofs_list = NULL, .list_length = 14, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    },
    {
        .range_start = 913, .range_length = 17, .glyph_id_start = 143,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 931, .range_length = 7, .glyph_id_start = 160,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 945, .range_length = 25, .glyph_id_start = 167,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 7684, .range_length = 1122, .glyph_id_start = 192,
        .unicode_list = unicode_list_5, .glyph_id_ofs_list = NULL, .list_length = 52, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
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

extern const lv_font_t lowres_18_la_next;


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t lowres_latina_18 = {
#else
lv_font_t lowres_latina_18 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 17,          /*The maximum line height required by the font*/
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
    .fallback = &lowres_18_la_next,
#endif
    .user_data = NULL,
};



#endif /*#if LOWRES_LATINA_18*/

