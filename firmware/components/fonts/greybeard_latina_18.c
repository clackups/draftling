/*******************************************************************************
 * Size: 18 px
 * Bpp: 1
 * Opts: --font Greybeard-18px.ttf --autohint-off -r 0x100-0x17F,0x218-0x21B,0x2C7,0x2D8-0x2DB,0x2DD,0x201D-0x201E --size 18 --bpp 1 --format lvgl --no-compress --lv-fallback greybeard_18_la_next --lv-font-name greybeard_latina_18 -o greybeard_latina_18.c
 ******************************************************************************/

#include "lvgl.h"

#ifndef GREYBEARD_LATINA_18
#define GREYBEARD_LATINA_18 1
#endif

#if GREYBEARD_LATINA_18

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
    {.bitmap_index = 1296, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1307, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1316, .adv_w = 144, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1329, .adv_w = 144, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1339, .adv_w = 144, .box_w = 5, .box_h = 3, .ofs_x = 2, .ofs_y = 9},
    {.bitmap_index = 1341, .adv_w = 144, .box_w = 5, .box_h = 3, .ofs_x = 2, .ofs_y = 9},
    {.bitmap_index = 1343, .adv_w = 144, .box_w = 2, .box_h = 2, .ofs_x = 3, .ofs_y = 10},
    {.bitmap_index = 1344, .adv_w = 144, .box_w = 4, .box_h = 4, .ofs_x = 2, .ofs_y = 8},
    {.bitmap_index = 1346, .adv_w = 144, .box_w = 3, .box_h = 4, .ofs_x = 4, .ofs_y = -3},
    {.bitmap_index = 1348, .adv_w = 144, .box_w = 5, .box_h = 3, .ofs_x = 2, .ofs_y = 9},
    {.bitmap_index = 1350, .adv_w = 144, .box_w = 5, .box_h = 4, .ofs_x = 2, .ofs_y = 8},
    {.bitmap_index = 1353, .adv_w = 144, .box_w = 5, .box_h = 4, .ofs_x = 2, .ofs_y = -2}
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

extern const lv_font_t greybeard_18_la_next;


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t greybeard_latina_18 = {
#else
lv_font_t greybeard_latina_18 = {
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
    .fallback = &greybeard_18_la_next,
#endif
    .user_data = NULL,
};



#endif /*#if GREYBEARD_LATINA_18*/

