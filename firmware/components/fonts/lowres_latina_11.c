/*******************************************************************************
 * Size: 11 px
 * Bpp: 1
 * Opts: --font Greybeard-11px.ttf -r 0x100-0x17F,0x218-0x21B,0x2C7,0x2D8-0x2DB,0x2DD,0x18F,0x1B7,0x259,0x292,0x1E04-0x1E05,0x1E0C-0x1E0D,0x1E24-0x1E25,0x1E32-0x1E33,0x1E36-0x1E37,0x1E42-0x1E43,0x1E46-0x1E47,0x1E5A-0x1E5B,0x1E62-0x1E63,0x1E6C-0x1E6D,0x1E7E-0x1E7F,0x1E88-0x1E89,0x1E92-0x1E93,0x1E9E,0x391-0x3A1,0x3A3-0x3A9,0x3B1-0x3C9,0x2011,0x2013-0x2014,0x2018-0x201A,0x201C-0x201E,0x2020-0x2021,0x202F-0x2030,0x2032-0x2033,0x2039-0x203A,0x2122,0x2212,0x221A,0x221E,0x2243,0x2260,0x2264-0x2265 --size 11 --bpp 1 --format lvgl --no-compress --lv-fallback lowres_11_la_next --lv-font-name lowres_latina_11 -o ./lowres_latina_11.c
 ******************************************************************************/

#include "lvgl.h"

#ifndef LOWRES_LATINA_11
#define LOWRES_LATINA_11 1
#endif

#if LOWRES_LATINA_11

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0100 "Ā" */
    0xf0, 0x66, 0x9f, 0x99, 0x90,

    /* U+0101 "ā" */
    0x70, 0x61, 0x79, 0x97,

    /* U+0102 "Ă" */
    0x96, 0x66, 0x9f, 0x99, 0x90,

    /* U+0103 "ă" */
    0x96, 0x6, 0x17, 0x99, 0x70,

    /* U+0104 "Ą" */
    0x66, 0x99, 0xf9, 0x99, 0x21,

    /* U+0105 "ą" */
    0x61, 0x79, 0x97, 0x21,

    /* U+0106 "Ć" */
    0x24, 0x69, 0x88, 0x89, 0x60,

    /* U+0107 "ć" */
    0x12, 0x6, 0x98, 0x88, 0x70,

    /* U+0108 "Ĉ" */
    0x25, 0x69, 0x88, 0x89, 0x60,

    /* U+0109 "ĉ" */
    0x25, 0x6, 0x98, 0x88, 0x70,

    /* U+010A "Ċ" */
    0x60, 0x69, 0x88, 0x89, 0x60,

    /* U+010B "ċ" */
    0x22, 0x6, 0x98, 0x88, 0x70,

    /* U+010C "Č" */
    0x52, 0x69, 0x88, 0x89, 0x60,

    /* U+010D "č" */
    0x52, 0x6, 0x98, 0x88, 0x70,

    /* U+010E "Ď" */
    0xa4, 0xca, 0x99, 0x9a, 0xc0,

    /* U+010F "ď" */
    0x19, 0x4c, 0xca, 0x52, 0x94, 0x60,

    /* U+0110 "Đ" */
    0x62, 0x93, 0xd4, 0xa5, 0x4c,

    /* U+0111 "đ" */
    0x13, 0xc4, 0xe9, 0x4a, 0x4e,

    /* U+0112 "Ē" */
    0xf0, 0xf8, 0x8e, 0x88, 0xf0,

    /* U+0113 "ē" */
    0x70, 0x69, 0xf8, 0x87,

    /* U+0114 "Ĕ" */
    0x96, 0xf8, 0x8e, 0x88, 0xf0,

    /* U+0115 "ĕ" */
    0x96, 0x6, 0x9f, 0x88, 0x70,

    /* U+0116 "Ė" */
    0x60, 0xf8, 0x8e, 0x88, 0xf0,

    /* U+0117 "ė" */
    0x22, 0x6, 0x9f, 0x88, 0x70,

    /* U+0118 "Ę" */
    0xf8, 0x8e, 0x88, 0x8f, 0x21,

    /* U+0119 "ę" */
    0x69, 0xf8, 0x87, 0x21,

    /* U+011A "Ě" */
    0x52, 0xf8, 0x8e, 0x88, 0xf0,

    /* U+011B "ě" */
    0x52, 0x6, 0x9f, 0x88, 0x70,

    /* U+011C "Ĝ" */
    0x25, 0x69, 0x8b, 0x99, 0x70,

    /* U+011D "ĝ" */
    0x25, 0x7, 0x99, 0x9f, 0x19, 0xe0,

    /* U+011E "Ğ" */
    0x96, 0x69, 0x8b, 0x99, 0x70,

    /* U+011F "ğ" */
    0x96, 0x7, 0x99, 0x9f, 0x19, 0xe0,

    /* U+0120 "Ġ" */
    0x60, 0x69, 0x8b, 0x99, 0x70,

    /* U+0121 "ġ" */
    0x22, 0x7, 0x99, 0x9f, 0x19, 0xe0,

    /* U+0122 "Ģ" */
    0x69, 0x88, 0xb9, 0x97, 0x24,

    /* U+0123 "ģ" */
    0x24, 0x79, 0x99, 0xf1, 0x9e,

    /* U+0124 "Ĥ" */
    0x25, 0x9, 0x9f, 0x99, 0x90,

    /* U+0125 "ĥ" */
    0x4a, 0x88, 0xbd, 0x99, 0x90,

    /* U+0126 "Ħ" */
    0x99, 0xf9, 0xf9, 0x99,

    /* U+0127 "ħ" */
    0x47, 0x90, 0xb6, 0xa5, 0x29,

    /* U+0128 "Ĩ" */
    0x5a, 0x72, 0x22, 0x22, 0x70,

    /* U+0129 "ĩ" */
    0x5a, 0x6, 0x22, 0x22, 0x70,

    /* U+012A "Ī" */
    0xf0, 0x72, 0x22, 0x22, 0x70,

    /* U+012B "ī" */
    0xe3, 0x24, 0x97,

    /* U+012C "Ĭ" */
    0x96, 0x72, 0x22, 0x22, 0x70,

    /* U+012D "ĭ" */
    0x96, 0x6, 0x22, 0x22, 0x70,

    /* U+012E "Į" */
    0xe9, 0x24, 0x97, 0x44,

    /* U+012F "į" */
    0x48, 0x64, 0x92, 0xe8, 0x80,

    /* U+0130 "İ" */
    0x63, 0xa4, 0x92, 0xe0,

    /* U+0131 "ı" */
    0xc9, 0x25, 0xc0,

    /* U+0132 "Ĳ" */
    0x9c, 0x63, 0x18, 0xd6, 0xb2,

    /* U+0133 "ĳ" */
    0x4a, 0x41, 0xb4, 0xa5, 0x29, 0xe8, 0x4c,

    /* U+0134 "Ĵ" */
    0x25, 0x31, 0x11, 0x99, 0x60,

    /* U+0135 "ĵ" */
    0x11, 0x40, 0x61, 0x8, 0x42, 0x14, 0x98,

    /* U+0136 "Ķ" */
    0x99, 0xac, 0xaa, 0x99, 0x48,

    /* U+0137 "ķ" */
    0x88, 0x9a, 0xca, 0x99, 0x48,

    /* U+0138 "ĸ" */
    0x99, 0xae, 0x99,

    /* U+0139 "Ĺ" */
    0x12, 0x88, 0x88, 0x88, 0xf0,

    /* U+013A "ĺ" */
    0x53, 0x24, 0x92, 0xe0,

    /* U+013B "Ļ" */
    0x88, 0x88, 0x88, 0x8f, 0x24,

    /* U+013C "ļ" */
    0xc9, 0x24, 0x97, 0x50,

    /* U+013D "Ľ" */
    0x39, 0xa8, 0x88, 0x88, 0xf0,

    /* U+013E "ľ" */
    0x1e, 0x54, 0x84, 0x21, 0x8, 0xe0,

    /* U+013F "Ŀ" */
    0x84, 0x21, 0x18, 0xc2, 0x1e,

    /* U+0140 "ŀ" */
    0xc2, 0x10, 0x94, 0xa1, 0x1c,

    /* U+0141 "Ł" */
    0x42, 0x14, 0xcc, 0x21, 0xf,

    /* U+0142 "ł" */
    0xc9, 0x3c, 0x97,

    /* U+0143 "Ń" */
    0x24, 0x9d, 0xdb, 0xb9, 0x90,

    /* U+0144 "ń" */
    0x24, 0xb, 0xd9, 0x99, 0x90,

    /* U+0145 "Ņ" */
    0x99, 0xdd, 0xbb, 0x99, 0x48,

    /* U+0146 "ņ" */
    0xbd, 0x99, 0x99, 0x48,

    /* U+0147 "Ň" */
    0xa4, 0x9d, 0xdb, 0xb9, 0x90,

    /* U+0148 "ň" */
    0x52, 0xb, 0xd9, 0x99, 0x90,

    /* U+0149 "ŉ" */
    0xc2, 0x20, 0xb6, 0xa5, 0x29, 0x48,

    /* U+014A "Ŋ" */
    0xad, 0x99, 0x99, 0xbb,

    /* U+014B "ŋ" */
    0xbd, 0x99, 0x99, 0x16,

    /* U+014C "Ō" */
    0xf0, 0x69, 0x99, 0x99, 0x60,

    /* U+014D "ō" */
    0x70, 0x69, 0x99, 0x96,

    /* U+014E "Ŏ" */
    0x96, 0x69, 0x99, 0x99, 0x60,

    /* U+014F "ŏ" */
    0x96, 0x6, 0x99, 0x99, 0x60,

    /* U+0150 "Ő" */
    0x4c, 0x99, 0x29, 0x4a, 0x52, 0x60,

    /* U+0151 "ő" */
    0x4c, 0x80, 0xc9, 0x4a, 0x52, 0x60,

    /* U+0152 "Œ" */
    0x7d, 0x29, 0x7a, 0x52, 0x8f,

    /* U+0153 "œ" */
    0x7d, 0x6f, 0x4a, 0x3c,

    /* U+0154 "Ŕ" */
    0x24, 0xe9, 0x9e, 0xa9, 0x90,

    /* U+0155 "ŕ" */
    0x24, 0xf, 0x54, 0x44, 0x40,

    /* U+0156 "Ŗ" */
    0xe9, 0x9e, 0xaa, 0x99, 0x48,

    /* U+0157 "ŗ" */
    0xf5, 0x44, 0x44, 0x24,

    /* U+0158 "Ř" */
    0xa4, 0xe9, 0x9e, 0xa9, 0x90,

    /* U+0159 "ř" */
    0x52, 0xf, 0x54, 0x44, 0x40,

    /* U+015A "Ś" */
    0x24, 0x69, 0x86, 0x19, 0x60,

    /* U+015B "ś" */
    0x12, 0x7, 0x8c, 0x31, 0xe0,

    /* U+015C "Ŝ" */
    0x25, 0x69, 0x86, 0x19, 0x60,

    /* U+015D "ŝ" */
    0x25, 0x7, 0x8c, 0x31, 0xe0,

    /* U+015E "Ş" */
    0x69, 0x84, 0x21, 0x96, 0x24,

    /* U+015F "ş" */
    0x78, 0xc3, 0x1e, 0x24,

    /* U+0160 "Š" */
    0x52, 0x69, 0x86, 0x19, 0x60,

    /* U+0161 "š" */
    0x52, 0x7, 0x8c, 0x31, 0xe0,

    /* U+0162 "Ţ" */
    0xf9, 0x8, 0x42, 0x10, 0x84, 0x11, 0x0,

    /* U+0163 "ţ" */
    0x44, 0xf4, 0x44, 0x43, 0x24,

    /* U+0164 "Ť" */
    0x51, 0x3e, 0x42, 0x10, 0x84, 0x20,

    /* U+0165 "ť" */
    0x1a, 0x55, 0xe4, 0x21, 0x8, 0x30,

    /* U+0166 "Ŧ" */
    0xf9, 0x8, 0xe2, 0x10, 0x84,

    /* U+0167 "ŧ" */
    0x44, 0xf4, 0xf4, 0x43,

    /* U+0168 "Ũ" */
    0x5a, 0x9, 0x99, 0x99, 0x60,

    /* U+0169 "ũ" */
    0x5a, 0x9, 0x99, 0x99, 0x70,

    /* U+016A "Ū" */
    0xf0, 0x99, 0x99, 0x99, 0x60,

    /* U+016B "ū" */
    0x70, 0x99, 0x99, 0x97,

    /* U+016C "Ŭ" */
    0x96, 0x9, 0x99, 0x99, 0x60,

    /* U+016D "ŭ" */
    0x96, 0x9, 0x99, 0x99, 0x70,

    /* U+016E "Ů" */
    0x25, 0x29, 0x99, 0x99, 0x60,

    /* U+016F "ů" */
    0x25, 0x29, 0x99, 0x99, 0x70,

    /* U+0170 "Ű" */
    0x4c, 0x81, 0x29, 0x4a, 0x52, 0x60,

    /* U+0171 "ű" */
    0x4c, 0x81, 0x29, 0x4a, 0x52, 0x70,

    /* U+0172 "Ų" */
    0x99, 0x99, 0x99, 0x96, 0x21,

    /* U+0173 "ų" */
    0x99, 0x99, 0x97, 0x21,

    /* U+0174 "Ŵ" */
    0x22, 0xa3, 0x1a, 0xd7, 0xea, 0x50,

    /* U+0175 "ŵ" */
    0x22, 0x81, 0x18, 0xd6, 0xae, 0x50,

    /* U+0176 "Ŷ" */
    0x22, 0xa3, 0x15, 0x10, 0x84, 0x20,

    /* U+0177 "ŷ" */
    0x25, 0x9, 0x99, 0x56, 0x24, 0xc0,

    /* U+0178 "Ÿ" */
    0xd8, 0x23, 0x15, 0x10, 0x84, 0x20,

    /* U+0179 "Ź" */
    0x24, 0xf1, 0x24, 0x48, 0xf0,

    /* U+017A "ź" */
    0x24, 0xf, 0x12, 0x48, 0xf0,

    /* U+017B "Ż" */
    0x60, 0xf1, 0x24, 0x48, 0xf0,

    /* U+017C "ż" */
    0x22, 0xf, 0x12, 0x48, 0xf0,

    /* U+017D "Ž" */
    0x52, 0xf1, 0x24, 0x48, 0xf0,

    /* U+017E "ž" */
    0x52, 0xf, 0x12, 0x48, 0xf0,

    /* U+017F "ſ" */
    0x76, 0x49, 0x24,

    /* U+018F "Ə" */
    0x69, 0x11, 0xf9, 0x96,

    /* U+01B7 "Ʒ" */
    0xf1, 0x26, 0x11, 0x96,

    /* U+0218 "Ș" */
    0x69, 0x84, 0x21, 0x96, 0x24,

    /* U+0219 "ș" */
    0x78, 0xc3, 0x1e, 0x24,

    /* U+021A "Ț" */
    0xf9, 0x8, 0x42, 0x10, 0x84, 0x11, 0x0,

    /* U+021B "ț" */
    0x44, 0xf4, 0x44, 0x43, 0x24,

    /* U+0259 "ə" */
    0xe1, 0x1f, 0x96,

    /* U+0292 "ʒ" */
    0xf1, 0x26, 0x11, 0x96,

    /* U+02C7 "ˇ" */
    0x96,

    /* U+02D8 "˘" */
    0x99, 0x60,

    /* U+02D9 "˙" */
    0xf0,

    /* U+02DA "˚" */
    0x69, 0x96,

    /* U+02DB "˛" */
    0x52, 0xa0,

    /* U+02DD "˝" */
    0x2a, 0xa8,

    /* U+0391 "Α" */
    0x66, 0x99, 0xf9, 0x99,

    /* U+0392 "Β" */
    0xe9, 0x9e, 0x99, 0x9e,

    /* U+0393 "Γ" */
    0xf8, 0x88, 0x88, 0x88,

    /* U+0394 "Δ" */
    0x21, 0x14, 0xa8, 0xc6, 0x3f,

    /* U+0395 "Ε" */
    0xf8, 0x8e, 0x88, 0x8f,

    /* U+0396 "Ζ" */
    0xf1, 0x22, 0x44, 0x8f,

    /* U+0397 "Η" */
    0x99, 0x9f, 0x99, 0x99,

    /* U+0398 "Θ" */
    0x74, 0x63, 0xf8, 0xc6, 0x2e,

    /* U+0399 "Ι" */
    0xe9, 0x24, 0x97,

    /* U+039A "Κ" */
    0x99, 0xac, 0xaa, 0x99,

    /* U+039B "Λ" */
    0x21, 0x14, 0xa8, 0xc6, 0x31,

    /* U+039C "Μ" */
    0x8e, 0xf7, 0x5a, 0xc6, 0x31,

    /* U+039D "Ν" */
    0x99, 0xdd, 0xbb, 0x99,

    /* U+039E "Ξ" */
    0xf9, 0x6, 0x0, 0x9f,

    /* U+039F "Ο" */
    0x69, 0x99, 0x99, 0x96,

    /* U+03A0 "Π" */
    0xf9, 0x99, 0x99, 0x99,

    /* U+03A1 "Ρ" */
    0xe9, 0x99, 0xe8, 0x88,

    /* U+03A3 "Σ" */
    0xf8, 0x42, 0x24, 0x8f,

    /* U+03A4 "Τ" */
    0xf9, 0x8, 0x42, 0x10, 0x84,

    /* U+03A5 "Υ" */
    0x8c, 0x54, 0xa2, 0x10, 0x84,

    /* U+03A6 "Φ" */
    0x23, 0xab, 0x5a, 0xd5, 0xc4,

    /* U+03A7 "Χ" */
    0x99, 0x96, 0x69, 0x99,

    /* U+03A8 "Ψ" */
    0xad, 0x6b, 0x57, 0x10, 0x84,

    /* U+03A9 "Ω" */
    0x74, 0x63, 0x18, 0xa9, 0x5b,

    /* U+03B1 "α" */
    0x4d, 0x65, 0x29, 0x34,

    /* U+03B2 "β" */
    0x69, 0x9e, 0x99, 0x9e, 0x88,

    /* U+03B3 "γ" */
    0x99, 0x52, 0x22, 0x22,

    /* U+03B4 "δ" */
    0x78, 0x46, 0x99, 0x96,

    /* U+03B5 "ε" */
    0x78, 0xe8, 0x87,

    /* U+03B6 "ζ" */
    0xf2, 0x44, 0x88, 0x87, 0x12,

    /* U+03B7 "η" */
    0xbd, 0x99, 0x99, 0x11,

    /* U+03B8 "θ" */
    0x69, 0x9f, 0x99, 0x96,

    /* U+03B9 "ι" */
    0x92, 0x48, 0xc0,

    /* U+03BA "κ" */
    0x99, 0xae, 0x99,

    /* U+03BB "λ" */
    0x44, 0x22, 0x69, 0x99,

    /* U+03BC "μ" */
    0x99, 0x99, 0xbd, 0x88,

    /* U+03BD "ν" */
    0x99, 0x95, 0x64,

    /* U+03BE "ξ" */
    0xf4, 0x86, 0x48, 0x87, 0x12,

    /* U+03BF "ο" */
    0x69, 0x99, 0x96,

    /* U+03C0 "π" */
    0xf9, 0x99, 0x99,

    /* U+03C1 "ρ" */
    0x69, 0x99, 0x9e, 0x88,

    /* U+03C2 "ς" */
    0x78, 0x88, 0x87, 0x12,

    /* U+03C3 "σ" */
    0x7d, 0x25, 0x29, 0x30,

    /* U+03C4 "τ" */
    0xf4, 0x44, 0x43,

    /* U+03C5 "υ" */
    0x99, 0x99, 0x96,

    /* U+03C6 "φ" */
    0x15, 0x6b, 0x5a, 0xb8, 0x84,

    /* U+03C7 "χ" */
    0x99, 0x96, 0x69, 0x99,

    /* U+03C8 "ψ" */
    0x21, 0x2b, 0x5a, 0xd6, 0xae, 0x21, 0x0,

    /* U+03C9 "ω" */
    0x9c, 0x6b, 0x5a, 0xa8,

    /* U+1E04 "Ḅ" */
    0xe9, 0x9e, 0x99, 0x9e, 0x6,

    /* U+1E05 "ḅ" */
    0x88, 0xe9, 0x99, 0x9e, 0x6,

    /* U+1E0C "Ḍ" */
    0xca, 0x99, 0x99, 0xac, 0xc,

    /* U+1E0D "ḍ" */
    0x11, 0x79, 0x99, 0x97, 0x6,

    /* U+1E24 "Ḥ" */
    0x99, 0x9f, 0x99, 0x99, 0x6,

    /* U+1E25 "ḥ" */
    0x88, 0xbd, 0x99, 0x99, 0x6,

    /* U+1E32 "Ḳ" */
    0x99, 0xac, 0xaa, 0x99, 0x6,

    /* U+1E33 "ḳ" */
    0x88, 0x9a, 0xca, 0x99, 0x6,

    /* U+1E36 "Ḷ" */
    0x88, 0x88, 0x88, 0x8f, 0x6,

    /* U+1E37 "ḷ" */
    0xc9, 0x24, 0x97, 0x18,

    /* U+1E42 "Ṃ" */
    0x8e, 0xf7, 0x5a, 0xc6, 0x31, 0x3, 0x0,

    /* U+1E43 "ṃ" */
    0xfd, 0x6b, 0x5a, 0xd4, 0xc,

    /* U+1E46 "Ṇ" */
    0x99, 0xdd, 0xbb, 0x99, 0x6,

    /* U+1E47 "ṇ" */
    0xbd, 0x99, 0x99, 0x6,

    /* U+1E5A "Ṛ" */
    0xe9, 0x9e, 0xaa, 0x99, 0x6,

    /* U+1E5B "ṛ" */
    0xf5, 0x44, 0x44, 0x6,

    /* U+1E62 "Ṣ" */
    0x69, 0x84, 0x21, 0x96, 0x6,

    /* U+1E63 "ṣ" */
    0x78, 0xc3, 0x1e, 0x6,

    /* U+1E6C "Ṭ" */
    0xf9, 0x8, 0x42, 0x10, 0x84, 0x3, 0x0,

    /* U+1E6D "ṭ" */
    0x44, 0xf4, 0x44, 0x43, 0x6,

    /* U+1E7E "Ṿ" */
    0x8c, 0x63, 0x15, 0x28, 0x84, 0x3, 0x0,

    /* U+1E7F "ṿ" */
    0x8c, 0x54, 0xa2, 0x10, 0xc,

    /* U+1E88 "Ẉ" */
    0x8c, 0x63, 0x5a, 0xfd, 0x4a, 0x3, 0x0,

    /* U+1E89 "ẉ" */
    0x8c, 0x6b, 0x57, 0x28, 0xc,

    /* U+1E92 "Ẓ" */
    0xf1, 0x22, 0x44, 0x8f, 0x6,

    /* U+1E93 "ẓ" */
    0xf1, 0x24, 0x8f, 0x6,

    /* U+1E9E "ẞ" */
    0xf4, 0xa9, 0x49, 0x46, 0x36,

    /* U+2011 "‑" */
    0xf0,

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

    /* U+201C "“" */
    0x4c, 0xb6,

    /* U+201D "”" */
    0xda, 0x64,

    /* U+201E "„" */
    0xda, 0x64,

    /* U+2020 "†" */
    0x21, 0x3e, 0x42, 0x10, 0x84, 0x21, 0x0,

    /* U+2021 "‡" */
    0x21, 0x3e, 0x42, 0x10, 0x9f, 0x21, 0x0,

    /* U+202F " " */
    0x0,

    /* U+2030 "‰" */
    0x45, 0x10, 0x36, 0x41, 0x55, 0x50,

    /* U+2032 "′" */
    0x58,

    /* U+2033 "″" */
    0x55, 0xa0,

    /* U+2039 "‹" */
    0x69,

    /* U+203A "›" */
    0x96,

    /* U+2122 "™" */
    0xfa, 0xd6,

    /* U+2212 "−" */
    0xf0,

    /* U+221A "√" */
    0x8, 0x42, 0x2d, 0x29, 0x8c, 0x42, 0x0,

    /* U+221E "∞" */
    0x55, 0x6a, 0xa0,

    /* U+2243 "≃" */
    0x4d, 0x64, 0xf, 0x80,

    /* U+2260 "≠" */
    0x8, 0xbe, 0x4f, 0xa2, 0x0,

    /* U+2264 "≤" */
    0x16, 0x86, 0x10, 0xf0,

    /* U+2265 "≥" */
    0x86, 0x16, 0x80, 0xf0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 5, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 9, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 14, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 19, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 24, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 28, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 33, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 38, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 43, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 48, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 53, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 58, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 63, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 68, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 73, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 79, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 84, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 89, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 94, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 98, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 103, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 108, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 113, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 118, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 123, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 127, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 132, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 137, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 142, .adv_w = 96, .box_w = 4, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 148, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 153, .adv_w = 96, .box_w = 4, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 159, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 164, .adv_w = 96, .box_w = 4, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 170, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 175, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 180, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 185, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 190, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 194, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 199, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 204, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 209, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 214, .adv_w = 96, .box_w = 3, .box_h = 8, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 217, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 222, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 227, .adv_w = 96, .box_w = 3, .box_h = 10, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 231, .adv_w = 96, .box_w = 3, .box_h = 11, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 236, .adv_w = 96, .box_w = 3, .box_h = 9, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 240, .adv_w = 96, .box_w = 3, .box_h = 6, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 243, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 248, .adv_w = 96, .box_w = 5, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 255, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 260, .adv_w = 96, .box_w = 5, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 267, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 272, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 277, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 280, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 285, .adv_w = 96, .box_w = 3, .box_h = 9, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 289, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 294, .adv_w = 96, .box_w = 3, .box_h = 10, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 298, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 303, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 309, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 314, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 319, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 324, .adv_w = 96, .box_w = 3, .box_h = 8, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 327, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 332, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 337, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 342, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 346, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 351, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 356, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 362, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 366, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 370, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 375, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 379, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 384, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 389, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 395, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 401, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 406, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 410, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 415, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 420, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 425, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 429, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 434, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 439, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 444, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 449, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 454, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 459, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 464, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 468, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 473, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 478, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 485, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 490, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 496, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 502, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 507, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 511, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 516, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 521, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 526, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 530, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 535, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 540, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 545, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 550, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 556, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 562, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 567, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 571, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 577, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 583, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 589, .adv_w = 96, .box_w = 4, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 595, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 601, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 606, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 611, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 616, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 621, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 626, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 631, .adv_w = 96, .box_w = 3, .box_h = 8, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 634, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 638, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 642, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 647, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 651, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 658, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 663, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 666, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 670, .adv_w = 96, .box_w = 4, .box_h = 2, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 671, .adv_w = 96, .box_w = 4, .box_h = 3, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 673, .adv_w = 96, .box_w = 2, .box_h = 2, .ofs_x = 2, .ofs_y = 6},
    {.bitmap_index = 674, .adv_w = 96, .box_w = 4, .box_h = 4, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 676, .adv_w = 96, .box_w = 3, .box_h = 4, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 678, .adv_w = 96, .box_w = 5, .box_h = 3, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 680, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 684, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 688, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 692, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 697, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 701, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 705, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 709, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 714, .adv_w = 96, .box_w = 3, .box_h = 8, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 717, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 721, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 726, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 731, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 735, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 739, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 743, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 747, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 751, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 755, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 760, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 765, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 770, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 774, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 779, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 784, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 788, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 793, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 797, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 801, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 804, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 809, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 813, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 817, .adv_w = 96, .box_w = 3, .box_h = 6, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 820, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 823, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 827, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 831, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 834, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 839, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 842, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 845, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 849, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 853, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 857, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 860, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 863, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 868, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 872, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 879, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 883, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 888, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 893, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 898, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 903, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 908, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 913, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 918, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 923, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 928, .adv_w = 96, .box_w = 3, .box_h = 10, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 932, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 939, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 944, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 949, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 953, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 958, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 962, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 967, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 971, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 978, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 983, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 990, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 995, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1002, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1007, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1012, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1016, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1021, .adv_w = 96, .box_w = 4, .box_h = 1, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 1022, .adv_w = 96, .box_w = 5, .box_h = 1, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 1023, .adv_w = 96, .box_w = 6, .box_h = 1, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 1024, .adv_w = 96, .box_w = 2, .box_h = 3, .ofs_x = 2, .ofs_y = 5},
    {.bitmap_index = 1025, .adv_w = 96, .box_w = 2, .box_h = 3, .ofs_x = 2, .ofs_y = 5},
    {.bitmap_index = 1026, .adv_w = 96, .box_w = 2, .box_h = 3, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 1027, .adv_w = 96, .box_w = 5, .box_h = 3, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 1029, .adv_w = 96, .box_w = 5, .box_h = 3, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 1031, .adv_w = 96, .box_w = 5, .box_h = 3, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1033, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1040, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1047, .adv_w = 96, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1048, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1054, .adv_w = 96, .box_w = 2, .box_h = 3, .ofs_x = 2, .ofs_y = 5},
    {.bitmap_index = 1055, .adv_w = 96, .box_w = 4, .box_h = 3, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 1057, .adv_w = 96, .box_w = 2, .box_h = 4, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 1058, .adv_w = 96, .box_w = 2, .box_h = 4, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 1059, .adv_w = 96, .box_w = 5, .box_h = 3, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 1061, .adv_w = 96, .box_w = 4, .box_h = 1, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 1062, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1069, .adv_w = 96, .box_w = 5, .box_h = 4, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1072, .adv_w = 96, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 1076, .adv_w = 96, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1081, .adv_w = 96, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1085, .adv_w = 96, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0}
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

extern const lv_font_t lowres_11_la_next;


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t lowres_latina_11 = {
#else
lv_font_t lowres_latina_11 = {
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
    .fallback = &lowres_11_la_next,
#endif
    .user_data = NULL,
};



#endif /*#if LOWRES_LATINA_11*/

