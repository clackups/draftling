/*******************************************************************************
 * Size: 16 px
 * Bpp: 1
 * Opts: --font Greybeard-16px.ttf -r 0x20-0x7F,0xA0-0xFF,0x20AC,0x2116 --size 16 --bpp 1 --format lvgl --no-compress --lv-fallback greybeard_16_ext --lv-font-name greybeard_16 -o /tmp/gen/greybeard_16.c
 ******************************************************************************/

#include "lvgl.h"

#ifndef GREYBEARD_16
#define GREYBEARD_16 1
#endif

#if GREYBEARD_16

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0xfe, 0xc0,

    /* U+0022 "\"" */
    0x99, 0x90,

    /* U+0023 "#" */
    0x49, 0x2f, 0xd2, 0x4b, 0xf4, 0x92,

    /* U+0024 "$" */
    0x23, 0xab, 0x4a, 0x38, 0xa5, 0xab, 0x88,

    /* U+0025 "%" */
    0x46, 0x9a, 0x94, 0x10, 0x82, 0x95, 0x96, 0x20,

    /* U+0026 "&" */
    0x30, 0x91, 0x22, 0x82, 0xa, 0x65, 0x44, 0x88,
    0xec,

    /* U+0027 "'" */
    0xf0,

    /* U+0028 "(" */
    0x29, 0x49, 0x24, 0x89, 0x10,

    /* U+0029 ")" */
    0x89, 0x12, 0x49, 0x29, 0x40,

    /* U+002A "*" */
    0x48, 0xcf, 0xcc, 0x48,

    /* U+002B "+" */
    0x10, 0x20, 0x47, 0xf1, 0x2, 0x4, 0x0,

    /* U+002C "," */
    0x6d, 0x40,

    /* U+002D "-" */
    0xfc,

    /* U+002E "." */
    0xf0,

    /* U+002F "/" */
    0x4, 0x10, 0x82, 0x10, 0x42, 0x8, 0x41, 0x8,
    0x20,

    /* U+0030 "0" */
    0x31, 0x28, 0x63, 0x96, 0x9c, 0x61, 0x48, 0xc0,

    /* U+0031 "1" */
    0x2e, 0x92, 0x49, 0x24,

    /* U+0032 "2" */
    0x7a, 0x18, 0x41, 0x8, 0x42, 0x10, 0x83, 0xf0,

    /* U+0033 "3" */
    0x7a, 0x10, 0x42, 0x38, 0x10, 0x41, 0x85, 0xe0,

    /* U+0034 "4" */
    0x8, 0x62, 0x8a, 0x4a, 0x2f, 0xc2, 0x8, 0x20,

    /* U+0035 "5" */
    0xfa, 0x8, 0x3c, 0x88, 0x10, 0x41, 0x89, 0xc0,

    /* U+0036 "6" */
    0x39, 0x8, 0x20, 0xbb, 0x18, 0x61, 0x85, 0xe0,

    /* U+0037 "7" */
    0xfc, 0x10, 0x42, 0x8, 0x41, 0x8, 0x20, 0x80,

    /* U+0038 "8" */
    0x7a, 0x18, 0x61, 0x7a, 0x18, 0x61, 0x85, 0xe0,

    /* U+0039 "9" */
    0x7a, 0x18, 0x61, 0x8d, 0xd0, 0x41, 0x9, 0xc0,

    /* U+003A ":" */
    0xf0, 0xf0,

    /* U+003B ";" */
    0x6c, 0x6, 0xd4,

    /* U+003C "<" */
    0x8, 0x88, 0x88, 0x41, 0x4, 0x10, 0x40,

    /* U+003D "=" */
    0xfc, 0x0, 0x3f,

    /* U+003E ">" */
    0x82, 0x8, 0x20, 0x84, 0x44, 0x44, 0x0,

    /* U+003F "?" */
    0x7a, 0x18, 0x41, 0x8, 0x42, 0x0, 0x20, 0x80,

    /* U+0040 "@" */
    0x39, 0x19, 0x6b, 0xa6, 0x9a, 0x67, 0x40, 0xf0,

    /* U+0041 "A" */
    0x30, 0xc4, 0x92, 0x86, 0x1f, 0xe1, 0x86, 0x10,

    /* U+0042 "B" */
    0xfa, 0x18, 0x61, 0xfa, 0x18, 0x61, 0x87, 0xe0,

    /* U+0043 "C" */
    0x39, 0x18, 0x20, 0x82, 0x8, 0x20, 0x44, 0xe0,

    /* U+0044 "D" */
    0xf2, 0x28, 0x61, 0x86, 0x18, 0x61, 0x8b, 0xc0,

    /* U+0045 "E" */
    0xfe, 0x8, 0x20, 0xf2, 0x8, 0x20, 0x83, 0xf0,

    /* U+0046 "F" */
    0xfe, 0x8, 0x20, 0xf2, 0x8, 0x20, 0x82, 0x0,

    /* U+0047 "G" */
    0x39, 0x18, 0x20, 0x82, 0x38, 0x61, 0x44, 0xf0,

    /* U+0048 "H" */
    0x86, 0x18, 0x61, 0xfe, 0x18, 0x61, 0x86, 0x10,

    /* U+0049 "I" */
    0xf9, 0x8, 0x42, 0x10, 0x84, 0x27, 0xc0,

    /* U+004A "J" */
    0x1c, 0x20, 0x82, 0x8, 0x20, 0xa2, 0x89, 0xc0,

    /* U+004B "K" */
    0x86, 0x18, 0xa4, 0xa3, 0x89, 0x22, 0x86, 0x10,

    /* U+004C "L" */
    0x82, 0x8, 0x20, 0x82, 0x8, 0x20, 0x83, 0xf0,

    /* U+004D "M" */
    0x83, 0x7, 0x1d, 0x5a, 0xb2, 0x64, 0xc1, 0x83,
    0x4,

    /* U+004E "N" */
    0x86, 0x1c, 0x69, 0xa6, 0x59, 0x63, 0x86, 0x10,

    /* U+004F "O" */
    0x7a, 0x18, 0x61, 0x86, 0x18, 0x61, 0x85, 0xe0,

    /* U+0050 "P" */
    0xfa, 0x18, 0x61, 0x87, 0xe8, 0x20, 0x82, 0x0,

    /* U+0051 "Q" */
    0x79, 0xa, 0x14, 0x28, 0x50, 0xa1, 0x4a, 0x8c,
    0xf0, 0x18,

    /* U+0052 "R" */
    0xfa, 0x18, 0x61, 0xfa, 0x48, 0xa2, 0x86, 0x10,

    /* U+0053 "S" */
    0x7a, 0x18, 0x20, 0x60, 0x60, 0x41, 0x85, 0xe0,

    /* U+0054 "T" */
    0xfe, 0x20, 0x40, 0x81, 0x2, 0x4, 0x8, 0x10,
    0x20,

    /* U+0055 "U" */
    0x86, 0x18, 0x61, 0x86, 0x18, 0x61, 0x85, 0xe0,

    /* U+0056 "V" */
    0x83, 0x6, 0x12, 0x24, 0x49, 0xa, 0x14, 0x30,
    0x20,

    /* U+0057 "W" */
    0x83, 0x6, 0x4c, 0x99, 0x35, 0x6a, 0xa2, 0x44,
    0x88,

    /* U+0058 "X" */
    0x86, 0x14, 0x92, 0x30, 0xc4, 0x92, 0x86, 0x10,

    /* U+0059 "Y" */
    0x82, 0x89, 0x11, 0x42, 0x82, 0x4, 0x8, 0x10,
    0x20,

    /* U+005A "Z" */
    0xfc, 0x10, 0x84, 0x10, 0x84, 0x10, 0x83, 0xf0,

    /* U+005B "[" */
    0xf8, 0x88, 0x88, 0x88, 0x88, 0x8f,

    /* U+005C "\\" */
    0x82, 0x4, 0x10, 0x20, 0x81, 0x4, 0x8, 0x20,
    0x41,

    /* U+005D "]" */
    0xf1, 0x11, 0x11, 0x11, 0x11, 0x1f,

    /* U+005E "^" */
    0x10, 0x51, 0x14, 0x10,

    /* U+005F "_" */
    0xfc,

    /* U+0060 "`" */
    0x99, 0x10,

    /* U+0061 "a" */
    0x78, 0x10, 0x5f, 0x86, 0x37, 0x40,

    /* U+0062 "b" */
    0x82, 0x8, 0x2e, 0xc6, 0x18, 0x61, 0xc6, 0xe0,

    /* U+0063 "c" */
    0x7a, 0x18, 0x20, 0x82, 0x17, 0x80,

    /* U+0064 "d" */
    0x4, 0x10, 0x5d, 0x8e, 0x18, 0x61, 0x8d, 0xd0,

    /* U+0065 "e" */
    0x7a, 0x18, 0x7f, 0x82, 0x7, 0x80,

    /* U+0066 "f" */
    0x1c, 0x92, 0x8, 0xf8, 0x82, 0x8, 0x20, 0x80,

    /* U+0067 "g" */
    0x76, 0x28, 0xa2, 0x72, 0x7, 0xa1, 0x85, 0xe0,

    /* U+0068 "h" */
    0x82, 0x8, 0x2e, 0xc6, 0x18, 0x61, 0x86, 0x10,

    /* U+0069 "i" */
    0x21, 0x0, 0xc2, 0x10, 0x84, 0x27, 0xc0,

    /* U+006A "j" */
    0x8, 0x40, 0x70, 0x84, 0x21, 0x8, 0x63, 0x17,
    0x0,

    /* U+006B "k" */
    0x82, 0x8, 0x21, 0x8a, 0x4e, 0x24, 0x8a, 0x10,

    /* U+006C "l" */
    0x61, 0x8, 0x42, 0x10, 0x84, 0x27, 0xc0,

    /* U+006D "m" */
    0xed, 0x26, 0x4c, 0x99, 0x32, 0x64, 0x80,

    /* U+006E "n" */
    0xbb, 0x18, 0x61, 0x86, 0x18, 0x40,

    /* U+006F "o" */
    0x7a, 0x18, 0x61, 0x86, 0x17, 0x80,

    /* U+0070 "p" */
    0xbb, 0x18, 0x61, 0x87, 0x1b, 0xa0, 0x82, 0x0,

    /* U+0071 "q" */
    0x76, 0x38, 0x61, 0x86, 0x37, 0x41, 0x4, 0x10,

    /* U+0072 "r" */
    0xdd, 0x94, 0x10, 0x41, 0x4, 0x0,

    /* U+0073 "s" */
    0x7a, 0x18, 0x1e, 0x6, 0x17, 0x80,

    /* U+0074 "t" */
    0x20, 0x8f, 0x88, 0x20, 0x82, 0x9, 0x18,

    /* U+0075 "u" */
    0x86, 0x18, 0x61, 0x86, 0x37, 0x40,

    /* U+0076 "v" */
    0x86, 0x18, 0x52, 0x48, 0xc3, 0x0,

    /* U+0077 "w" */
    0x83, 0x26, 0x4c, 0x96, 0xc8, 0x91, 0x0,

    /* U+0078 "x" */
    0x86, 0x14, 0x8c, 0x4a, 0x18, 0x40,

    /* U+0079 "y" */
    0x86, 0x14, 0x52, 0x28, 0xa1, 0x4, 0xa1, 0x0,

    /* U+007A "z" */
    0xfc, 0x21, 0x8, 0x42, 0xf, 0xc0,

    /* U+007B "{" */
    0x19, 0x8, 0x42, 0x63, 0x4, 0x21, 0x8, 0x30,

    /* U+007C "|" */
    0xff, 0xf0,

    /* U+007D "}" */
    0xc1, 0x8, 0x42, 0xc, 0x64, 0x21, 0x9, 0x80,

    /* U+007E "~" */
    0x66, 0xd9, 0x80,

    /* U+00A0 " " */
    0x0,

    /* U+00A1 "¡" */
    0xdf, 0xc0,

    /* U+00A2 "¢" */
    0x5, 0xe8, 0xe4, 0x92, 0x8a, 0x5e, 0x40,

    /* U+00A3 "£" */
    0x39, 0x14, 0x50, 0x43, 0xc4, 0x10, 0x87, 0xf0,

    /* U+00A4 "¤" */
    0x85, 0xe4, 0x92, 0x7a, 0x10,

    /* U+00A5 "¥" */
    0x82, 0x89, 0x11, 0x47, 0xc2, 0x1f, 0x8, 0x10,
    0x20,

    /* U+00A6 "¦" */
    0xf9, 0xf0,

    /* U+00A7 "§" */
    0x69, 0x84, 0x69, 0x99, 0x62, 0x19, 0x60,

    /* U+00A8 "¨" */
    0x99,

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
    0xf0,

    /* U+00AE "®" */
    0x3c, 0x42, 0xb9, 0xa5, 0xa5, 0xb9, 0xa9, 0xa5,
    0x42, 0x3c,

    /* U+00AF "¯" */
    0xfc,

    /* U+00B0 "°" */
    0x69, 0x96,

    /* U+00B1 "±" */
    0x10, 0x20, 0x47, 0xf1, 0x2, 0x4, 0x0, 0xfe,

    /* U+00B2 "²" */
    0x74, 0x42, 0x22, 0x23, 0xe0,

    /* U+00B3 "³" */
    0x74, 0x42, 0x60, 0xc5, 0xc0,

    /* U+00B4 "´" */
    0x25, 0x40,

    /* U+00B5 "µ" */
    0x86, 0x18, 0x61, 0x87, 0x3b, 0x60, 0x82, 0x0,

    /* U+00B6 "¶" */
    0x7e, 0x59, 0x65, 0x74, 0x51, 0x45, 0x14, 0x50,

    /* U+00B7 "·" */
    0xf0,

    /* U+00B8 "¸" */
    0x4c, 0xe0,

    /* U+00B9 "¹" */
    0x2e, 0x92, 0x48,

    /* U+00BA "º" */
    0x74, 0x63, 0x17, 0x3, 0xe0,

    /* U+00BB "»" */
    0x91, 0x22, 0x52, 0x90,

    /* U+00BC "¼" */
    0x41, 0x85, 0x12, 0x24, 0x8a, 0x5, 0x16, 0x54,
    0xaa, 0x78, 0x20,

    /* U+00BD "½" */
    0x41, 0x85, 0x12, 0x24, 0x8a, 0x7, 0x19, 0x42,
    0x9a, 0x40, 0xf0,

    /* U+00BE "¾" */
    0x61, 0x24, 0x90, 0xa9, 0x8d, 0x5, 0x16, 0x54,
    0xaa, 0x78, 0x20,

    /* U+00BF "¿" */
    0x10, 0x40, 0x4, 0x21, 0x8, 0x21, 0x85, 0xe0,

    /* U+00C0 "À" */
    0x40, 0xc0, 0xc, 0x31, 0x24, 0xa1, 0xfe, 0x18,
    0x61,

    /* U+00C1 "Á" */
    0x8, 0xc0, 0xc, 0x31, 0x24, 0xa1, 0xfe, 0x18,
    0x61,

    /* U+00C2 "Â" */
    0x31, 0x20, 0xc, 0x31, 0x24, 0xa1, 0xfe, 0x18,
    0x61,

    /* U+00C3 "Ã" */
    0x25, 0x60, 0xc, 0x31, 0x24, 0xa1, 0xfe, 0x18,
    0x61,

    /* U+00C4 "Ä" */
    0x49, 0x20, 0xc, 0x31, 0x24, 0xa1, 0xfe, 0x18,
    0x61,

    /* U+00C5 "Å" */
    0x31, 0x24, 0x8c, 0x31, 0x24, 0xa1, 0xfe, 0x18,
    0x61,

    /* U+00C6 "Æ" */
    0x1f, 0x18, 0x28, 0x28, 0x2e, 0x48, 0x78, 0x48,
    0x88, 0x8f,

    /* U+00C7 "Ç" */
    0x39, 0x18, 0x20, 0x82, 0x8, 0x20, 0x44, 0xe1,
    0x2, 0x30,

    /* U+00C8 "È" */
    0x40, 0xc0, 0x3f, 0x82, 0x8, 0x3c, 0x82, 0x8,
    0x3f,

    /* U+00C9 "É" */
    0x8, 0xc0, 0x3f, 0x82, 0x8, 0x3c, 0x82, 0x8,
    0x3f,

    /* U+00CA "Ê" */
    0x31, 0x20, 0x3f, 0x82, 0x8, 0x3c, 0x82, 0x8,
    0x3f,

    /* U+00CB "Ë" */
    0x49, 0x20, 0x3f, 0x82, 0x8, 0x3c, 0x82, 0x8,
    0x3f,

    /* U+00CC "Ì" */
    0x41, 0x81, 0xf2, 0x10, 0x84, 0x21, 0x9, 0xf0,

    /* U+00CD "Í" */
    0x13, 0x1, 0xf2, 0x10, 0x84, 0x21, 0x9, 0xf0,

    /* U+00CE "Î" */
    0x32, 0x41, 0xf2, 0x10, 0x84, 0x21, 0x9, 0xf0,

    /* U+00CF "Ï" */
    0x4a, 0x41, 0xf2, 0x10, 0x84, 0x21, 0x9, 0xf0,

    /* U+00D0 "Ð" */
    0x78, 0x89, 0xa, 0x1f, 0x28, 0x50, 0xa1, 0x44,
    0xf0,

    /* U+00D1 "Ñ" */
    0x66, 0x60, 0x21, 0xc6, 0x9a, 0x65, 0x96, 0x38,
    0x61,

    /* U+00D2 "Ò" */
    0x40, 0xc0, 0x1e, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x5e,

    /* U+00D3 "Ó" */
    0x8, 0xc0, 0x1e, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x5e,

    /* U+00D4 "Ô" */
    0x31, 0x20, 0x1e, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x5e,

    /* U+00D5 "Õ" */
    0x25, 0x60, 0x1e, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x5e,

    /* U+00D6 "Ö" */
    0x49, 0x20, 0x1e, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x5e,

    /* U+00D7 "×" */
    0x85, 0x23, 0xc, 0x4a, 0x10,

    /* U+00D8 "Ø" */
    0x5, 0xd4, 0xa3, 0x96, 0x5a, 0x69, 0xc5, 0x2b,
    0xa0,

    /* U+00D9 "Ù" */
    0x40, 0xc0, 0x21, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x5e,

    /* U+00DA "Ú" */
    0x8, 0xc0, 0x21, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x5e,

    /* U+00DB "Û" */
    0x31, 0x20, 0x21, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x5e,

    /* U+00DC "Ü" */
    0x49, 0x20, 0x21, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x5e,

    /* U+00DD "Ý" */
    0x4, 0x30, 0x4, 0x14, 0x48, 0x8a, 0x14, 0x10,
    0x20, 0x40, 0x80,

    /* U+00DE "Þ" */
    0x82, 0xf, 0xa1, 0x86, 0x18, 0x7e, 0x82, 0x0,

    /* U+00DF "ß" */
    0x72, 0x28, 0xa4, 0x9a, 0x18, 0x61, 0xa6, 0x60,

    /* U+00E0 "à" */
    0x20, 0x81, 0x0, 0x78, 0x10, 0x5f, 0x86, 0x37,
    0x40,

    /* U+00E1 "á" */
    0x10, 0x42, 0x0, 0x78, 0x10, 0x5f, 0x86, 0x37,
    0x40,

    /* U+00E2 "â" */
    0x31, 0x20, 0x1e, 0x4, 0x17, 0xe1, 0x8d, 0xd0,

    /* U+00E3 "ã" */
    0x25, 0x60, 0x1e, 0x4, 0x17, 0xe1, 0x8d, 0xd0,

    /* U+00E4 "ä" */
    0x49, 0x20, 0x1e, 0x4, 0x17, 0xe1, 0x8d, 0xd0,

    /* U+00E5 "å" */
    0x31, 0x23, 0x0, 0x78, 0x10, 0x5f, 0x86, 0x37,
    0x40,

    /* U+00E6 "æ" */
    0xec, 0x24, 0x4b, 0xf9, 0x12, 0x1b, 0x80,

    /* U+00E7 "ç" */
    0x7a, 0x18, 0x20, 0x82, 0x17, 0x88, 0x11, 0x80,

    /* U+00E8 "è" */
    0x20, 0x81, 0x0, 0x7a, 0x18, 0x7f, 0x82, 0x7,
    0x80,

    /* U+00E9 "é" */
    0x10, 0x42, 0x0, 0x7a, 0x18, 0x7f, 0x82, 0x7,
    0x80,

    /* U+00EA "ê" */
    0x31, 0x20, 0x1e, 0x86, 0x1f, 0xe0, 0x81, 0xe0,

    /* U+00EB "ë" */
    0x49, 0x20, 0x1e, 0x86, 0x1f, 0xe0, 0x81, 0xe0,

    /* U+00EC "ì" */
    0x42, 0x8, 0x6, 0x10, 0x84, 0x21, 0x3e,

    /* U+00ED "í" */
    0x21, 0x10, 0x6, 0x10, 0x84, 0x21, 0x3e,

    /* U+00EE "î" */
    0x64, 0x80, 0xc2, 0x10, 0x84, 0x27, 0xc0,

    /* U+00EF "ï" */
    0x94, 0x80, 0xc2, 0x10, 0x84, 0x27, 0xc0,

    /* U+00F0 "ð" */
    0x91, 0x89, 0xe, 0x46, 0x18, 0x61, 0x85, 0xe0,

    /* U+00F1 "ñ" */
    0x66, 0x60, 0x2e, 0xc6, 0x18, 0x61, 0x86, 0x10,

    /* U+00F2 "ò" */
    0x20, 0x81, 0x0, 0x7a, 0x18, 0x61, 0x86, 0x17,
    0x80,

    /* U+00F3 "ó" */
    0x10, 0x42, 0x0, 0x7a, 0x18, 0x61, 0x86, 0x17,
    0x80,

    /* U+00F4 "ô" */
    0x31, 0x20, 0x1e, 0x86, 0x18, 0x61, 0x85, 0xe0,

    /* U+00F5 "õ" */
    0x25, 0x60, 0x1e, 0x86, 0x18, 0x61, 0x85, 0xe0,

    /* U+00F6 "ö" */
    0x49, 0x20, 0x1e, 0x86, 0x18, 0x61, 0x85, 0xe0,

    /* U+00F7 "÷" */
    0x10, 0x70, 0x40, 0xf, 0xe0, 0x4, 0x1c, 0x10,

    /* U+00F8 "ø" */
    0x5, 0xe8, 0xe5, 0xa6, 0x9c, 0x5e, 0x80,

    /* U+00F9 "ù" */
    0x20, 0x81, 0x0, 0x86, 0x18, 0x61, 0x86, 0x37,
    0x40,

    /* U+00FA "ú" */
    0x10, 0x42, 0x0, 0x86, 0x18, 0x61, 0x86, 0x37,
    0x40,

    /* U+00FB "û" */
    0x31, 0x20, 0x21, 0x86, 0x18, 0x61, 0x8d, 0xd0,

    /* U+00FC "ü" */
    0x49, 0x20, 0x21, 0x86, 0x18, 0x61, 0x8d, 0xd0,

    /* U+00FD "ý" */
    0x10, 0x42, 0x0, 0x86, 0x14, 0x52, 0x28, 0xa1,
    0x4, 0xa1, 0x0,

    /* U+00FE "þ" */
    0x82, 0x8, 0x2e, 0xc6, 0x18, 0x61, 0xc6, 0xe8,
    0x20, 0x80,

    /* U+00FF "ÿ" */
    0x49, 0x20, 0x21, 0x85, 0x14, 0x8a, 0x28, 0x41,
    0x28, 0x40,

    /* U+20AC "€" */
    0x1c, 0x45, 0x7, 0xc4, 0x1e, 0x10, 0x11, 0x1c,

    /* U+2116 "№" */
    0x95, 0x37, 0x6e, 0xdd, 0x56, 0x2f, 0xd8, 0x91,
    0x20
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 128, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 128, .box_w = 1, .box_h = 10, .ofs_x = 4, .ofs_y = 0},
    {.bitmap_index = 3, .adv_w = 128, .box_w = 4, .box_h = 3, .ofs_x = 2, .ofs_y = 7},
    {.bitmap_index = 5, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 11, .adv_w = 128, .box_w = 5, .box_h = 11, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 18, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 26, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 35, .adv_w = 128, .box_w = 1, .box_h = 4, .ofs_x = 4, .ofs_y = 7},
    {.bitmap_index = 36, .adv_w = 128, .box_w = 3, .box_h = 12, .ofs_x = 3, .ofs_y = -1},
    {.bitmap_index = 41, .adv_w = 128, .box_w = 3, .box_h = 12, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 46, .adv_w = 128, .box_w = 6, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 50, .adv_w = 128, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 57, .adv_w = 128, .box_w = 3, .box_h = 4, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 59, .adv_w = 128, .box_w = 6, .box_h = 1, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 60, .adv_w = 128, .box_w = 2, .box_h = 2, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 61, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 70, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 78, .adv_w = 128, .box_w = 3, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 82, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 90, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 98, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 106, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 114, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 122, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 130, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 138, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 146, .adv_w = 128, .box_w = 2, .box_h = 6, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 148, .adv_w = 128, .box_w = 3, .box_h = 8, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 151, .adv_w = 128, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 158, .adv_w = 128, .box_w = 6, .box_h = 4, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 161, .adv_w = 128, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 168, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 176, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 184, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 192, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 200, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 208, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 216, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 224, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 232, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 240, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 248, .adv_w = 128, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 255, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 263, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 271, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 279, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 288, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 296, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 304, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 312, .adv_w = 128, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 322, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 330, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 338, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 347, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 355, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 364, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 373, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 381, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 390, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 398, .adv_w = 128, .box_w = 4, .box_h = 12, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 404, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 413, .adv_w = 128, .box_w = 4, .box_h = 12, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 419, .adv_w = 128, .box_w = 7, .box_h = 4, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 423, .adv_w = 128, .box_w = 6, .box_h = 1, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 424, .adv_w = 128, .box_w = 3, .box_h = 4, .ofs_x = 3, .ofs_y = 7},
    {.bitmap_index = 426, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 432, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 440, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 446, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 454, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 460, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 468, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 476, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 484, .adv_w = 128, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 491, .adv_w = 128, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 500, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 508, .adv_w = 128, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 515, .adv_w = 128, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 522, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 528, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 534, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 542, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 550, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 556, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 562, .adv_w = 128, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 569, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 575, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 581, .adv_w = 128, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 588, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 594, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 602, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 608, .adv_w = 128, .box_w = 5, .box_h = 12, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 616, .adv_w = 128, .box_w = 1, .box_h = 12, .ofs_x = 4, .ofs_y = -1},
    {.bitmap_index = 618, .adv_w = 128, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 626, .adv_w = 128, .box_w = 6, .box_h = 3, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 629, .adv_w = 128, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 630, .adv_w = 128, .box_w = 1, .box_h = 10, .ofs_x = 4, .ofs_y = -3},
    {.bitmap_index = 632, .adv_w = 128, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 639, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 647, .adv_w = 128, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 652, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 661, .adv_w = 128, .box_w = 1, .box_h = 12, .ofs_x = 4, .ofs_y = -1},
    {.bitmap_index = 663, .adv_w = 128, .box_w = 4, .box_h = 13, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 670, .adv_w = 128, .box_w = 4, .box_h = 2, .ofs_x = 2, .ofs_y = 9},
    {.bitmap_index = 671, .adv_w = 128, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 681, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 2, .ofs_y = 3},
    {.bitmap_index = 686, .adv_w = 128, .box_w = 6, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 690, .adv_w = 128, .box_w = 6, .box_h = 4, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 693, .adv_w = 128, .box_w = 4, .box_h = 1, .ofs_x = 2, .ofs_y = 4},
    {.bitmap_index = 694, .adv_w = 128, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 704, .adv_w = 128, .box_w = 6, .box_h = 1, .ofs_x = 1, .ofs_y = 9},
    {.bitmap_index = 705, .adv_w = 128, .box_w = 4, .box_h = 4, .ofs_x = 2, .ofs_y = 6},
    {.bitmap_index = 707, .adv_w = 128, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 715, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 2, .ofs_y = 4},
    {.bitmap_index = 720, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 2, .ofs_y = 4},
    {.bitmap_index = 725, .adv_w = 128, .box_w = 3, .box_h = 4, .ofs_x = 3, .ofs_y = 7},
    {.bitmap_index = 727, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 735, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 743, .adv_w = 128, .box_w = 2, .box_h = 2, .ofs_x = 3, .ofs_y = 4},
    {.bitmap_index = 744, .adv_w = 128, .box_w = 3, .box_h = 4, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 746, .adv_w = 128, .box_w = 3, .box_h = 7, .ofs_x = 2, .ofs_y = 4},
    {.bitmap_index = 749, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 2, .ofs_y = 3},
    {.bitmap_index = 754, .adv_w = 128, .box_w = 6, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 758, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 769, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 780, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 791, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 799, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 808, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 817, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 826, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 835, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 844, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 853, .adv_w = 128, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 863, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 873, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 882, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 891, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 900, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 909, .adv_w = 128, .box_w = 5, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 917, .adv_w = 128, .box_w = 5, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 925, .adv_w = 128, .box_w = 5, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 933, .adv_w = 128, .box_w = 5, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 941, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 950, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 959, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 968, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 977, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 986, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 995, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1004, .adv_w = 128, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 1009, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1018, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1027, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1036, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1045, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1054, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1065, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1073, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1081, .adv_w = 128, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1090, .adv_w = 128, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1099, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1107, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1115, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1123, .adv_w = 128, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1132, .adv_w = 128, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1139, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1147, .adv_w = 128, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1156, .adv_w = 128, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1165, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1173, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1181, .adv_w = 128, .box_w = 5, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 1188, .adv_w = 128, .box_w = 5, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 1195, .adv_w = 128, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 1202, .adv_w = 128, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 1209, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1217, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1225, .adv_w = 128, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1234, .adv_w = 128, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1243, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1251, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1259, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1267, .adv_w = 128, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1275, .adv_w = 128, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1282, .adv_w = 128, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1291, .adv_w = 128, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1300, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1308, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1316, .adv_w = 128, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1327, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1337, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1347, .adv_w = 128, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1355, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0}
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

extern lv_font_t greybeard_16_ext;


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t greybeard_16 = {
#else
lv_font_t greybeard_16 = {
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
    .fallback = &greybeard_16_ext,
#endif
    .user_data = NULL,
};



#endif /*#if GREYBEARD_16*/

