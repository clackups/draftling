/*******************************************************************************
 * Size: 18 px
 * Bpp: 1
 * Opts: --font Greybeard-18px.ttf -r 0x20-0x7F,0xA0-0xFF,0x20AC,0x2116 --size 18 --bpp 1 --format lvgl --no-compress --lv-fallback greybeard_18_ext --lv-font-name greybeard_18 -o /tmp/gen/greybeard_18.c
 ******************************************************************************/

#include "lvgl.h"

#ifndef GREYBEARD_18
#define GREYBEARD_18 1
#endif

#if GREYBEARD_18

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0xff, 0x20,

    /* U+0022 "\"" */
    0x99, 0x90,

    /* U+0023 "#" */
    0x49, 0x2f, 0xd2, 0x4b, 0xf4, 0x92,

    /* U+0024 "$" */
    0x21, 0x1d, 0x5a, 0x51, 0x86, 0x29, 0x6a, 0xe2,
    0x10,

    /* U+0025 "%" */
    0x43, 0x42, 0x92, 0x40, 0x2, 0x9, 0x5, 0x4a,
    0x16, 0x10,

    /* U+0026 "&" */
    0x30, 0x91, 0x22, 0x43, 0x4, 0x14, 0xca, 0x89,
    0x19, 0xc8,

    /* U+0027 "'" */
    0xf0,

    /* U+0028 "(" */
    0x12, 0x40, 0x88, 0x88, 0x84, 0x2, 0x10,

    /* U+0029 ")" */
    0x84, 0x20, 0x11, 0x11, 0x12, 0x4, 0x80,

    /* U+002A "*" */
    0x48, 0xcf, 0xcc, 0x48,

    /* U+002B "+" */
    0x10, 0x20, 0x47, 0xf1, 0x2, 0x4, 0x0,

    /* U+002C "," */
    0x6a, 0x0,

    /* U+002D "-" */
    0xfe,

    /* U+002E "." */
    0xc0,

    /* U+002F "/" */
    0x2, 0x0, 0x10, 0x0, 0x80, 0x4, 0x0, 0x20,
    0x0, 0x2, 0x8, 0x0,

    /* U+0030 "0" */
    0x31, 0x28, 0x63, 0x96, 0x9c, 0x61, 0x85, 0x23,
    0x0,

    /* U+0031 "1" */
    0x13, 0x59, 0x11, 0x11, 0x11, 0x10,

    /* U+0032 "2" */
    0x7a, 0x10, 0x41, 0x4, 0x21, 0x8, 0x42, 0xf,
    0xc0,

    /* U+0033 "3" */
    0x7a, 0x10, 0x41, 0x8, 0xe0, 0x41, 0x6, 0x17,
    0x80,

    /* U+0034 "4" */
    0x8, 0x60, 0x8a, 0x48, 0x28, 0xbf, 0x8, 0x20,
    0x80,

    /* U+0035 "5" */
    0xfa, 0x8, 0x2c, 0xca, 0x10, 0x41, 0x6, 0x27,
    0x0,

    /* U+0036 "6" */
    0x39, 0x8, 0x20, 0xbb, 0x18, 0x61, 0x86, 0x17,
    0x80,

    /* U+0037 "7" */
    0xfe, 0x10, 0x42, 0x0, 0x41, 0x4, 0x20, 0x82,
    0x0,

    /* U+0038 "8" */
    0x7a, 0x18, 0x61, 0x49, 0xe8, 0x61, 0x86, 0x17,
    0x80,

    /* U+0039 "9" */
    0x7a, 0x18, 0x61, 0x86, 0x37, 0x41, 0x4, 0x27,
    0x0,

    /* U+003A ":" */
    0xc0, 0x30,

    /* U+003B ";" */
    0x60, 0x0, 0xd4,

    /* U+003C "<" */
    0x8, 0x88, 0x88, 0x1, 0x4, 0x10, 0x40,

    /* U+003D "=" */
    0xfc, 0x0, 0x3f,

    /* U+003E ">" */
    0x82, 0x8, 0x20, 0x80, 0x44, 0x44, 0x0,

    /* U+003F "?" */
    0x7d, 0x4, 0x8, 0x10, 0x41, 0x4, 0x0, 0x0,
    0x0, 0x40,

    /* U+0040 "@" */
    0x38, 0x8a, 0x4d, 0x5a, 0xb5, 0x6a, 0xca, 0x40,
    0x7c,

    /* U+0041 "A" */
    0x10, 0x20, 0x41, 0x44, 0x48, 0x91, 0x3e, 0x83,
    0x6, 0x8,

    /* U+0042 "B" */
    0xfa, 0x18, 0x61, 0x8b, 0xe8, 0x61, 0x86, 0x1f,
    0x80,

    /* U+0043 "C" */
    0x39, 0x18, 0x20, 0x82, 0x8, 0x20, 0x81, 0x13,
    0x80,

    /* U+0044 "D" */
    0xf2, 0x28, 0x61, 0x86, 0x18, 0x61, 0x86, 0x2f,
    0x0,

    /* U+0045 "E" */
    0xfe, 0x8, 0x20, 0x83, 0xc8, 0x20, 0x82, 0xf,
    0xc0,

    /* U+0046 "F" */
    0xfe, 0x8, 0x20, 0x83, 0xc8, 0x20, 0x82, 0x8,
    0x0,

    /* U+0047 "G" */
    0x39, 0x18, 0x20, 0x82, 0x78, 0x61, 0x85, 0x13,
    0xc0,

    /* U+0048 "H" */
    0x86, 0x18, 0x61, 0x87, 0xf8, 0x61, 0x86, 0x18,
    0x40,

    /* U+0049 "I" */
    0xf9, 0x8, 0x42, 0x10, 0x84, 0x21, 0x3e,

    /* U+004A "J" */
    0x1c, 0x20, 0x82, 0x8, 0x20, 0x82, 0x88, 0x7,
    0x0,

    /* U+004B "K" */
    0x86, 0x8, 0xa0, 0x92, 0x8d, 0x22, 0x82, 0x8,
    0x40,

    /* U+004C "L" */
    0x82, 0x8, 0x20, 0x82, 0x8, 0x20, 0x82, 0xf,
    0xc0,

    /* U+004D "M" */
    0x83, 0x7, 0x1c, 0x1a, 0xb0, 0x64, 0xc1, 0x83,
    0x6, 0x8,

    /* U+004E "N" */
    0x86, 0x1c, 0x61, 0xa6, 0x58, 0x63, 0x86, 0x18,
    0x40,

    /* U+004F "O" */
    0x7a, 0x18, 0x61, 0x86, 0x18, 0x61, 0x86, 0x17,
    0x80,

    /* U+0050 "P" */
    0xfa, 0x18, 0x61, 0x87, 0xe8, 0x20, 0x82, 0x8,
    0x0,

    /* U+0051 "Q" */
    0x79, 0xa, 0x14, 0x28, 0x50, 0xa1, 0x42, 0xa5,
    0xa9, 0xe0, 0x0, 0x60,

    /* U+0052 "R" */
    0xfa, 0x18, 0x61, 0x87, 0xe9, 0x22, 0x82, 0x8,
    0x40,

    /* U+0053 "S" */
    0x7a, 0x18, 0x20, 0x40, 0xc0, 0x81, 0x6, 0x17,
    0x80,

    /* U+0054 "T" */
    0xfe, 0x20, 0x40, 0x81, 0x2, 0x4, 0x8, 0x10,
    0x20, 0x40,

    /* U+0055 "U" */
    0x86, 0x18, 0x61, 0x86, 0x18, 0x61, 0x86, 0x17,
    0x80,

    /* U+0056 "V" */
    0x83, 0x6, 0xa, 0x24, 0x48, 0x8a, 0x0, 0x10,
    0x20, 0x40,

    /* U+0057 "W" */
    0x83, 0x6, 0xc, 0x99, 0x32, 0x60, 0xd5, 0x44,
    0x89, 0x10,

    /* U+0058 "X" */
    0x84, 0x4, 0x80, 0x30, 0xc3, 0x12, 0x0, 0x8,
    0x40,

    /* U+0059 "Y" */
    0x82, 0x1, 0x10, 0x2, 0x80, 0x4, 0x8, 0x10,
    0x20, 0x40,

    /* U+005A "Z" */
    0xfc, 0x10, 0x80, 0x10, 0x80, 0x10, 0x82, 0xf,
    0xc0,

    /* U+005B "[" */
    0xf8, 0x88, 0x88, 0x88, 0x88, 0x88, 0xf0,

    /* U+005C "\\" */
    0x80, 0x1, 0x1, 0x0, 0x2, 0x0, 0x4, 0x0,
    0x8, 0x0, 0x10,

    /* U+005D "]" */
    0xf1, 0x11, 0x11, 0x11, 0x11, 0x11, 0xf0,

    /* U+005E "^" */
    0x10, 0x51, 0x14, 0x10,

    /* U+005F "_" */
    0xfe,

    /* U+0060 "`" */
    0x88, 0x10,

    /* U+0061 "a" */
    0x78, 0x10, 0x5f, 0x86, 0x18, 0xdd,

    /* U+0062 "b" */
    0x82, 0x8, 0x2e, 0xc6, 0x18, 0x61, 0x87, 0x1b,
    0x80,

    /* U+0063 "c" */
    0x39, 0x18, 0x20, 0x82, 0x4, 0x4e,

    /* U+0064 "d" */
    0x4, 0x10, 0x5d, 0x8e, 0x18, 0x61, 0x86, 0x37,
    0x40,

    /* U+0065 "e" */
    0x39, 0x18, 0x7f, 0x82, 0x4, 0x4e,

    /* U+0066 "f" */
    0x1c, 0x82, 0x8, 0xfc, 0x82, 0x8, 0x20, 0x82,
    0x0,

    /* U+0067 "g" */
    0x76, 0x28, 0xa2, 0x89, 0xc8, 0x1e, 0x84, 0x7,
    0x80,

    /* U+0068 "h" */
    0x82, 0x8, 0x2e, 0xc6, 0x18, 0x61, 0x86, 0x18,
    0x40,

    /* U+0069 "i" */
    0x20, 0x0, 0x6, 0x10, 0x84, 0x21, 0x9, 0xf0,

    /* U+006A "j" */
    0x8, 0x0, 0x3, 0x84, 0x21, 0x8, 0x42, 0x18,
    0x81, 0xc0,

    /* U+006B "k" */
    0x82, 0x8, 0x21, 0x8a, 0x4a, 0x38, 0x92, 0x28,
    0x40,

    /* U+006C "l" */
    0x61, 0x8, 0x42, 0x10, 0x84, 0x21, 0x3e,

    /* U+006D "m" */
    0xed, 0x26, 0x4c, 0x99, 0x32, 0x64, 0xc9,

    /* U+006E "n" */
    0xbb, 0x18, 0x61, 0x86, 0x18, 0x61,

    /* U+006F "o" */
    0x7a, 0x18, 0x61, 0x86, 0x18, 0x5e,

    /* U+0070 "p" */
    0xbb, 0x18, 0x61, 0x86, 0x1c, 0x6e, 0x82, 0x8,
    0x0,

    /* U+0071 "q" */
    0x76, 0x38, 0x61, 0x86, 0x18, 0xdd, 0x4, 0x10,
    0x40,

    /* U+0072 "r" */
    0xdd, 0x94, 0x10, 0x41, 0x4, 0x10,

    /* U+0073 "s" */
    0x7a, 0x10, 0x18, 0x18, 0x8, 0x5e,

    /* U+0074 "t" */
    0x20, 0x8f, 0x88, 0x20, 0x82, 0x8, 0x24, 0x60,

    /* U+0075 "u" */
    0x86, 0x18, 0x61, 0x86, 0x18, 0xdd,

    /* U+0076 "v" */
    0x82, 0x1, 0x10, 0x2, 0x80, 0x0, 0x8,

    /* U+0077 "w" */
    0x83, 0x6, 0x4c, 0x99, 0x2d, 0x91, 0x22,

    /* U+0078 "x" */
    0x84, 0x4, 0x8c, 0x1, 0x20, 0x21,

    /* U+0079 "y" */
    0x86, 0x18, 0x52, 0x8, 0xa2, 0x4, 0x2, 0x84,
    0x0,

    /* U+007A "z" */
    0xfc, 0x10, 0x84, 0x21, 0x8, 0x3f,

    /* U+007B "{" */
    0x19, 0x8, 0x42, 0x23, 0x8, 0x21, 0x8, 0x41,
    0x80,

    /* U+007C "|" */
    0xff, 0xf8,

    /* U+007D "}" */
    0xc1, 0x8, 0x42, 0x8, 0x62, 0x21, 0x8, 0x4c,
    0x0,

    /* U+007E "~" */
    0x66, 0x49, 0x80,

    /* U+00A0 " " */
    0x0,

    /* U+00A1 "¡" */
    0x9f, 0xe0,

    /* U+00A2 "¢" */
    0x4, 0xe4, 0xe4, 0x82, 0x80, 0x11, 0x7a, 0x0,

    /* U+00A3 "£" */
    0x39, 0x14, 0x10, 0x43, 0xc4, 0x10, 0x42, 0x1f,
    0xc0,

    /* U+00A4 "¤" */
    0x85, 0xe4, 0x92, 0x7a, 0x10,

    /* U+00A5 "¥" */
    0x82, 0x1, 0x10, 0x2, 0x9f, 0xc4, 0x7f, 0x10,
    0x20, 0x40,

    /* U+00A6 "¦" */
    0xf8, 0xf8,

    /* U+00A7 "§" */
    0x69, 0x4, 0x69, 0x99, 0x96, 0x20, 0x96,

    /* U+00A8 "¨" */
    0x90,

    /* U+00A9 "©" */
    0x3c, 0x42, 0x99, 0xa5, 0xa1, 0xa1, 0xa5, 0x99,
    0x42, 0x3c,

    /* U+00AA "ª" */
    0x70, 0x5f, 0x17, 0x83, 0xe0,

    /* U+00AB "«" */
    0x25, 0x29, 0x12, 0x24,

    /* U+00AC "¬" */
    0xfc, 0x10, 0x41,

    /* U+00AD "­" */
    0xf8,

    /* U+00AE "®" */
    0x3c, 0x42, 0xb9, 0xa5, 0xa1, 0xb9, 0xa5, 0x0,
    0x42, 0x3c,

    /* U+00AF "¯" */
    0xfc,

    /* U+00B0 "°" */
    0x69, 0x6,

    /* U+00B1 "±" */
    0x10, 0x20, 0x47, 0xf1, 0x2, 0x4, 0x0, 0xfe,

    /* U+00B2 "²" */
    0x74, 0x42, 0x22, 0x23, 0xe0,

    /* U+00B3 "³" */
    0x74, 0x42, 0x60, 0xc5, 0xc0,

    /* U+00B4 "´" */
    0x21, 0x40,

    /* U+00B5 "µ" */
    0x85, 0xa, 0x14, 0x28, 0x50, 0xb3, 0x59, 0x81,
    0x2, 0x0,

    /* U+00B6 "¶" */
    0x7e, 0x59, 0x65, 0x74, 0x51, 0x45, 0x14, 0x51,
    0x40,

    /* U+00B7 "·" */
    0xc0,

    /* U+00B8 "¸" */
    0x20, 0x1e,

    /* U+00B9 "¹" */
    0x2e, 0x92, 0x48,

    /* U+00BA "º" */
    0x74, 0x63, 0x17, 0x3, 0xe0,

    /* U+00BB "»" */
    0x91, 0x22, 0x52, 0x90,

    /* U+00BC "¼" */
    0x40, 0x83, 0xa, 0x4, 0x49, 0x0, 0x8, 0x0,
    0x59, 0x50, 0xa9, 0xe0, 0x80,

    /* U+00BD "½" */
    0x40, 0x83, 0xa, 0x4, 0x49, 0x0, 0x8, 0x2c,
    0x65, 0xc, 0x61, 0x3, 0xc0,

    /* U+00BE "¾" */
    0x61, 0x24, 0x88, 0x9, 0x4d, 0x4, 0x0, 0x2c,
    0xa8, 0x54, 0xf0, 0x40,

    /* U+00BF "¿" */
    0x10, 0x0, 0x0, 0x80, 0x4, 0x30, 0x40, 0x82,
    0x1, 0xf0,

    /* U+00C0 "À" */
    0x40, 0x60, 0x0, 0x81, 0x2, 0xa, 0x22, 0x44,
    0x89, 0xf4, 0x18, 0x30, 0x40,

    /* U+00C1 "Á" */
    0x4, 0x30, 0x0, 0x81, 0x2, 0xa, 0x22, 0x44,
    0x89, 0xf4, 0x18, 0x30, 0x40,

    /* U+00C2 "Â" */
    0x18, 0x48, 0x0, 0x81, 0x2, 0xa, 0x22, 0x44,
    0x89, 0xf4, 0x18, 0x30, 0x40,

    /* U+00C3 "Ã" */
    0x24, 0xb0, 0x0, 0x81, 0x2, 0xa, 0x22, 0x44,
    0x89, 0xf4, 0x18, 0x30, 0x40,

    /* U+00C4 "Ä" */
    0x24, 0x0, 0x0, 0x81, 0x2, 0xa, 0x22, 0x44,
    0x89, 0xf4, 0x18, 0x30, 0x40,

    /* U+00C5 "Å" */
    0x38, 0x88, 0x1, 0xc1, 0x2, 0xa, 0x22, 0x44,
    0x89, 0xf4, 0x18, 0x30, 0x40,

    /* U+00C6 "Æ" */
    0x1f, 0x18, 0x28, 0x28, 0x28, 0x4e, 0x48, 0x78,
    0x88, 0x88, 0x8f,

    /* U+00C7 "Ç" */
    0x39, 0x18, 0x20, 0x82, 0x8, 0x20, 0x81, 0x13,
    0x84, 0x8, 0xc0,

    /* U+00C8 "È" */
    0x40, 0xc0, 0x3f, 0x82, 0x8, 0x20, 0xf2, 0x8,
    0x20, 0x83, 0xf0,

    /* U+00C9 "É" */
    0x8, 0xc0, 0x3f, 0x82, 0x8, 0x20, 0xf2, 0x8,
    0x20, 0x83, 0xf0,

    /* U+00CA "Ê" */
    0x31, 0x20, 0x3f, 0x82, 0x8, 0x20, 0xf2, 0x8,
    0x20, 0x83, 0xf0,

    /* U+00CB "Ë" */
    0x48, 0x0, 0x3f, 0x82, 0x8, 0x20, 0xf2, 0x8,
    0x20, 0x83, 0xf0,

    /* U+00CC "Ì" */
    0x41, 0x81, 0xf2, 0x10, 0x84, 0x21, 0x8, 0x42,
    0x7c,

    /* U+00CD "Í" */
    0x13, 0x1, 0xf2, 0x10, 0x84, 0x21, 0x8, 0x42,
    0x7c,

    /* U+00CE "Î" */
    0x32, 0x41, 0xf2, 0x10, 0x84, 0x21, 0x8, 0x42,
    0x7c,

    /* U+00CF "Ï" */
    0x48, 0x1, 0xf2, 0x10, 0x84, 0x21, 0x8, 0x42,
    0x7c,

    /* U+00D0 "Ð" */
    0x78, 0x89, 0xa, 0x14, 0x3e, 0x50, 0xa1, 0x42,
    0x89, 0xe0,

    /* U+00D1 "Ñ" */
    0x25, 0x60, 0x21, 0x87, 0x18, 0x69, 0x96, 0x18,
    0xe1, 0x86, 0x10,

    /* U+00D2 "Ò" */
    0x40, 0xc0, 0x1e, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x61, 0x85, 0xe0,

    /* U+00D3 "Ó" */
    0x8, 0xc0, 0x1e, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x61, 0x85, 0xe0,

    /* U+00D4 "Ô" */
    0x31, 0x20, 0x1e, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x61, 0x85, 0xe0,

    /* U+00D5 "Õ" */
    0x25, 0x60, 0x1e, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x61, 0x85, 0xe0,

    /* U+00D6 "Ö" */
    0x48, 0x0, 0x1e, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x61, 0x85, 0xe0,

    /* U+00D7 "×" */
    0x85, 0x23, 0x0, 0x4a, 0x10,

    /* U+00D8 "Ø" */
    0x5, 0xe8, 0xe3, 0x96, 0x59, 0x69, 0xc7, 0x1c,
    0x5e, 0x80,

    /* U+00D9 "Ù" */
    0x40, 0xc0, 0x21, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x61, 0x85, 0xe0,

    /* U+00DA "Ú" */
    0x8, 0xc0, 0x21, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x61, 0x85, 0xe0,

    /* U+00DB "Û" */
    0x31, 0x20, 0x21, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x61, 0x85, 0xe0,

    /* U+00DC "Ü" */
    0x48, 0x0, 0x21, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x61, 0x85, 0xe0,

    /* U+00DD "Ý" */
    0x8, 0x60, 0x4, 0x10, 0x8, 0x80, 0x14, 0x0,
    0x20, 0x40, 0x81, 0x2, 0x0,

    /* U+00DE "Þ" */
    0x82, 0x8, 0x3e, 0x86, 0x18, 0x7e, 0x82, 0x8,
    0x0,

    /* U+00DF "ß" */
    0x72, 0x28, 0xa2, 0x92, 0x68, 0x61, 0x86, 0x99,
    0x80,

    /* U+00E0 "à" */
    0x20, 0x1, 0x0, 0x78, 0x10, 0x5f, 0x86, 0x18,
    0xdd,

    /* U+00E1 "á" */
    0x10, 0x2, 0x0, 0x78, 0x10, 0x5f, 0x86, 0x18,
    0xdd,

    /* U+00E2 "â" */
    0x31, 0x20, 0x1e, 0x4, 0x17, 0xe1, 0x86, 0x37,
    0x40,

    /* U+00E3 "ã" */
    0x25, 0x60, 0x1e, 0x4, 0x17, 0xe1, 0x86, 0x37,
    0x40,

    /* U+00E4 "ä" */
    0x48, 0x0, 0x1e, 0x4, 0x17, 0xe1, 0x86, 0x37,
    0x40,

    /* U+00E5 "å" */
    0x31, 0x20, 0xc, 0x1, 0xe0, 0x41, 0x7e, 0x18,
    0x63, 0x74,

    /* U+00E6 "æ" */
    0x6d, 0x24, 0x4b, 0xf9, 0x12, 0x24, 0xf6,

    /* U+00E7 "ç" */
    0x39, 0x18, 0x20, 0x82, 0x4, 0x4e, 0x10, 0x23,
    0x0,

    /* U+00E8 "è" */
    0x20, 0x1, 0x0, 0x39, 0x18, 0x7f, 0x82, 0x4,
    0x4e,

    /* U+00E9 "é" */
    0x10, 0x2, 0x0, 0x39, 0x18, 0x7f, 0x82, 0x4,
    0x4e,

    /* U+00EA "ê" */
    0x18, 0x90, 0xe, 0x46, 0x1f, 0xe0, 0x81, 0x13,
    0x80,

    /* U+00EB "ë" */
    0x48, 0x0, 0xe, 0x46, 0x1f, 0xe0, 0x81, 0x13,
    0x80,

    /* U+00EC "ì" */
    0x40, 0x8, 0x6, 0x10, 0x84, 0x21, 0x9, 0xf0,

    /* U+00ED "í" */
    0x10, 0x8, 0x6, 0x10, 0x84, 0x21, 0x9, 0xf0,

    /* U+00EE "î" */
    0x64, 0x80, 0xc2, 0x10, 0x84, 0x21, 0x3e,

    /* U+00EF "ï" */
    0x90, 0x0, 0xc2, 0x10, 0x84, 0x21, 0x3e,

    /* U+00F0 "ð" */
    0x91, 0x89, 0xe, 0x4a, 0x18, 0x61, 0x85, 0x23,
    0x0,

    /* U+00F1 "ñ" */
    0x4a, 0xc0, 0x2e, 0xc6, 0x18, 0x61, 0x86, 0x18,
    0x40,

    /* U+00F2 "ò" */
    0x20, 0x1, 0x0, 0x7a, 0x18, 0x61, 0x86, 0x18,
    0x5e,

    /* U+00F3 "ó" */
    0x10, 0x2, 0x0, 0x7a, 0x18, 0x61, 0x86, 0x18,
    0x5e,

    /* U+00F4 "ô" */
    0x31, 0x20, 0x1e, 0x86, 0x18, 0x61, 0x86, 0x17,
    0x80,

    /* U+00F5 "õ" */
    0x25, 0x60, 0x1e, 0x86, 0x18, 0x61, 0x86, 0x17,
    0x80,

    /* U+00F6 "ö" */
    0x48, 0x0, 0x1e, 0x86, 0x18, 0x61, 0x86, 0x17,
    0x80,

    /* U+00F7 "÷" */
    0x10, 0x70, 0x40, 0xf, 0xe0, 0x4, 0x1c, 0x10,

    /* U+00F8 "ø" */
    0x5, 0xe8, 0xe5, 0x86, 0x98, 0x71, 0x7a, 0x0,

    /* U+00F9 "ù" */
    0x20, 0x1, 0x0, 0x86, 0x18, 0x61, 0x86, 0x18,
    0xdd,

    /* U+00FA "ú" */
    0x10, 0x2, 0x0, 0x86, 0x18, 0x61, 0x86, 0x18,
    0xdd,

    /* U+00FB "û" */
    0x31, 0x20, 0x21, 0x86, 0x18, 0x61, 0x86, 0x37,
    0x40,

    /* U+00FC "ü" */
    0x48, 0x0, 0x21, 0x86, 0x18, 0x61, 0x86, 0x37,
    0x40,

    /* U+00FD "ý" */
    0x10, 0x2, 0x0, 0x86, 0x18, 0x52, 0x8, 0xa2,
    0x4, 0x2, 0x84, 0x0,

    /* U+00FE "þ" */
    0x82, 0x8, 0x2e, 0xc6, 0x18, 0x61, 0x87, 0x1b,
    0xa0, 0x82, 0x0,

    /* U+00FF "ÿ" */
    0x48, 0x0, 0x21, 0x86, 0x14, 0x82, 0x28, 0x81,
    0x0, 0xa1, 0x0,

    /* U+20AC "€" */
    0x1c, 0x45, 0x2, 0xf, 0xc8, 0x3e, 0x20, 0x40,
    0x44, 0x70,

    /* U+2116 "№" */
    0x92, 0x95, 0x95, 0xd5, 0xd2, 0xd0, 0xb7, 0xb0,
    0xb0, 0x90, 0x90
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 144, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 144, .box_w = 1, .box_h = 11, .ofs_x = 4, .ofs_y = 0},
    {.bitmap_index = 3, .adv_w = 144, .box_w = 4, .box_h = 3, .ofs_x = 2, .ofs_y = 8},
    {.bitmap_index = 5, .adv_w = 144, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 11, .adv_w = 144, .box_w = 5, .box_h = 14, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 20, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 30, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 40, .adv_w = 144, .box_w = 1, .box_h = 4, .ofs_x = 4, .ofs_y = 8},
    {.bitmap_index = 41, .adv_w = 144, .box_w = 4, .box_h = 13, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 48, .adv_w = 144, .box_w = 4, .box_h = 13, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 55, .adv_w = 144, .box_w = 6, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 59, .adv_w = 144, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 66, .adv_w = 144, .box_w = 3, .box_h = 3, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 68, .adv_w = 144, .box_w = 7, .box_h = 1, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 69, .adv_w = 144, .box_w = 2, .box_h = 1, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 70, .adv_w = 144, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 82, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 91, .adv_w = 144, .box_w = 4, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 97, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 106, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 115, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 124, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 133, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 142, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 151, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 160, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 169, .adv_w = 144, .box_w = 2, .box_h = 6, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 171, .adv_w = 144, .box_w = 3, .box_h = 8, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 174, .adv_w = 144, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 181, .adv_w = 144, .box_w = 6, .box_h = 4, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 184, .adv_w = 144, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 191, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 201, .adv_w = 144, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 210, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 220, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 229, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 238, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 247, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 256, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 265, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 274, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 283, .adv_w = 144, .box_w = 5, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 290, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 299, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 308, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 317, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 327, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 336, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 345, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 354, .adv_w = 144, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 366, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 375, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 384, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 394, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 403, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 413, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 423, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 432, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 442, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 451, .adv_w = 144, .box_w = 4, .box_h = 13, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 458, .adv_w = 144, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 469, .adv_w = 144, .box_w = 4, .box_h = 13, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 476, .adv_w = 144, .box_w = 7, .box_h = 4, .ofs_x = 1, .ofs_y = 8},
    {.bitmap_index = 480, .adv_w = 144, .box_w = 7, .box_h = 1, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 481, .adv_w = 144, .box_w = 3, .box_h = 4, .ofs_x = 3, .ofs_y = 8},
    {.bitmap_index = 483, .adv_w = 144, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 489, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 498, .adv_w = 144, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 504, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 513, .adv_w = 144, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 519, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 528, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 537, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 546, .adv_w = 144, .box_w = 5, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 554, .adv_w = 144, .box_w = 5, .box_h = 15, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 564, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 573, .adv_w = 144, .box_w = 5, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 580, .adv_w = 144, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 587, .adv_w = 144, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 593, .adv_w = 144, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 599, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 608, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 617, .adv_w = 144, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 623, .adv_w = 144, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 629, .adv_w = 144, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 637, .adv_w = 144, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 643, .adv_w = 144, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 650, .adv_w = 144, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 657, .adv_w = 144, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 663, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 672, .adv_w = 144, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 678, .adv_w = 144, .box_w = 5, .box_h = 13, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 687, .adv_w = 144, .box_w = 1, .box_h = 13, .ofs_x = 4, .ofs_y = -1},
    {.bitmap_index = 689, .adv_w = 144, .box_w = 5, .box_h = 13, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 698, .adv_w = 144, .box_w = 6, .box_h = 3, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 701, .adv_w = 144, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 702, .adv_w = 144, .box_w = 1, .box_h = 11, .ofs_x = 4, .ofs_y = -3},
    {.bitmap_index = 704, .adv_w = 144, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 712, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 721, .adv_w = 144, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 726, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 736, .adv_w = 144, .box_w = 1, .box_h = 13, .ofs_x = 4, .ofs_y = -1},
    {.bitmap_index = 738, .adv_w = 144, .box_w = 4, .box_h = 14, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 745, .adv_w = 144, .box_w = 4, .box_h = 1, .ofs_x = 2, .ofs_y = 11},
    {.bitmap_index = 746, .adv_w = 144, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 756, .adv_w = 144, .box_w = 5, .box_h = 7, .ofs_x = 2, .ofs_y = 4},
    {.bitmap_index = 761, .adv_w = 144, .box_w = 6, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 765, .adv_w = 144, .box_w = 6, .box_h = 4, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 768, .adv_w = 144, .box_w = 5, .box_h = 1, .ofs_x = 2, .ofs_y = 4},
    {.bitmap_index = 769, .adv_w = 144, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 779, .adv_w = 144, .box_w = 6, .box_h = 1, .ofs_x = 1, .ofs_y = 10},
    {.bitmap_index = 780, .adv_w = 144, .box_w = 4, .box_h = 4, .ofs_x = 2, .ofs_y = 7},
    {.bitmap_index = 782, .adv_w = 144, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 790, .adv_w = 144, .box_w = 5, .box_h = 7, .ofs_x = 2, .ofs_y = 5},
    {.bitmap_index = 795, .adv_w = 144, .box_w = 5, .box_h = 7, .ofs_x = 2, .ofs_y = 5},
    {.bitmap_index = 800, .adv_w = 144, .box_w = 3, .box_h = 4, .ofs_x = 3, .ofs_y = 8},
    {.bitmap_index = 802, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 812, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 821, .adv_w = 144, .box_w = 2, .box_h = 1, .ofs_x = 3, .ofs_y = 5},
    {.bitmap_index = 822, .adv_w = 144, .box_w = 4, .box_h = 4, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 824, .adv_w = 144, .box_w = 3, .box_h = 7, .ofs_x = 2, .ofs_y = 5},
    {.bitmap_index = 827, .adv_w = 144, .box_w = 5, .box_h = 7, .ofs_x = 2, .ofs_y = 4},
    {.bitmap_index = 832, .adv_w = 144, .box_w = 6, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 836, .adv_w = 144, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 849, .adv_w = 144, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 862, .adv_w = 144, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 874, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 884, .adv_w = 144, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 897, .adv_w = 144, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 910, .adv_w = 144, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 923, .adv_w = 144, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 936, .adv_w = 144, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 949, .adv_w = 144, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 962, .adv_w = 144, .box_w = 8, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 973, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 984, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 995, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1006, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1017, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1028, .adv_w = 144, .box_w = 5, .box_h = 14, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 1037, .adv_w = 144, .box_w = 5, .box_h = 14, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 1046, .adv_w = 144, .box_w = 5, .box_h = 14, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 1055, .adv_w = 144, .box_w = 5, .box_h = 14, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 1064, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1074, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1085, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1096, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1107, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1118, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1129, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1140, .adv_w = 144, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 1145, .adv_w = 144, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1155, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1166, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1177, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1188, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1199, .adv_w = 144, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1212, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1221, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1230, .adv_w = 144, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1239, .adv_w = 144, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1248, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1257, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1266, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1275, .adv_w = 144, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1285, .adv_w = 144, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1292, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1301, .adv_w = 144, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1310, .adv_w = 144, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1319, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1328, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1337, .adv_w = 144, .box_w = 5, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 1345, .adv_w = 144, .box_w = 5, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 1353, .adv_w = 144, .box_w = 5, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 1360, .adv_w = 144, .box_w = 5, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 1367, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1376, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1385, .adv_w = 144, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1394, .adv_w = 144, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1403, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1412, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1421, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1430, .adv_w = 144, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1438, .adv_w = 144, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1446, .adv_w = 144, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1455, .adv_w = 144, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1464, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1473, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1482, .adv_w = 144, .box_w = 6, .box_h = 15, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1494, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1505, .adv_w = 144, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1516, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1526, .adv_w = 144, .box_w = 8, .box_h = 11, .ofs_x = 0, .ofs_y = 0}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_2[] = {
    0x0, 0x6a
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 95, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 160, .range_length = 96, .glyph_id_start = 96,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 8364, .range_length = 107, .glyph_id_start = 192,
        .unicode_list = unicode_list_2, .glyph_id_ofs_list = NULL, .list_length = 2, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
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

extern lv_font_t greybeard_18_ext;


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t greybeard_18 = {
#else
lv_font_t greybeard_18 = {
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
    .fallback = &greybeard_18_ext,
#endif
    .user_data = NULL,
};



#endif /*#if GREYBEARD_18*/

