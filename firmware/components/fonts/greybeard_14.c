/*******************************************************************************
 * Size: 14 px
 * Bpp: 1
 * Opts: --font Greybeard-14px.ttf -r 0x20-0x7F,0xA0-0xFF,0x20AC,0x2116 --size 14 --bpp 1 --format lvgl --no-compress --lv-fallback greybeard_14_ext --lv-font-name greybeard_14 -o /tmp/gen/greybeard_14.c
 ******************************************************************************/

#include "lvgl.h"

#ifndef GREYBEARD_14
#define GREYBEARD_14 1
#endif

#if GREYBEARD_14

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
    0x52, 0xbe, 0xa5, 0x7d, 0x4a,

    /* U+0024 "$" */
    0x23, 0xab, 0x46, 0x18, 0xb5, 0x71, 0x0,

    /* U+0025 "%" */
    0x4d, 0x6c, 0xa2, 0x29, 0xb5, 0x90,

    /* U+0026 "&" */
    0x21, 0x45, 0x8, 0x62, 0x59, 0x62, 0x74,

    /* U+0027 "'" */
    0xf0,

    /* U+0028 "(" */
    0x29, 0x49, 0x24, 0x89, 0x10,

    /* U+0029 ")" */
    0x89, 0x12, 0x49, 0x29, 0x40,

    /* U+002A "*" */
    0x51, 0x3e, 0x45, 0x0,

    /* U+002B "+" */
    0x21, 0x3e, 0x42, 0x0,

    /* U+002C "," */
    0x6d, 0x40,

    /* U+002D "-" */
    0xf8,

    /* U+002E "." */
    0xf0,

    /* U+002F "/" */
    0x8, 0x44, 0x21, 0x10, 0x88, 0x42, 0x21, 0x0,

    /* U+0030 "0" */
    0x22, 0xa3, 0x3a, 0xe6, 0x2a, 0x20,

    /* U+0031 "1" */
    0x2e, 0x92, 0x49, 0x20,

    /* U+0032 "2" */
    0x74, 0x62, 0x11, 0x11, 0x10, 0xf8,

    /* U+0033 "3" */
    0x74, 0x42, 0x23, 0x4, 0x31, 0x70,

    /* U+0034 "4" */
    0x11, 0x8c, 0xa5, 0x4b, 0xe2, 0x10,

    /* U+0035 "5" */
    0xfc, 0x21, 0xe8, 0x84, 0x31, 0x70,

    /* U+0036 "6" */
    0x32, 0x21, 0xf, 0x46, 0x31, 0x70,

    /* U+0037 "7" */
    0xfc, 0x42, 0x21, 0x10, 0x88, 0x40,

    /* U+0038 "8" */
    0x74, 0x63, 0x17, 0x46, 0x31, 0x70,

    /* U+0039 "9" */
    0x74, 0x63, 0x17, 0x84, 0x22, 0x60,

    /* U+003A ":" */
    0xf0, 0xf0,

    /* U+003B ";" */
    0x6c, 0x6, 0xd4,

    /* U+003C "<" */
    0x8, 0x88, 0x88, 0x20, 0x82, 0x8,

    /* U+003D "=" */
    0xf8, 0x1, 0xf0,

    /* U+003E ">" */
    0x82, 0x8, 0x20, 0x88, 0x88, 0x80,

    /* U+003F "?" */
    0x74, 0x62, 0x11, 0x10, 0x80, 0x21, 0x0,

    /* U+0040 "@" */
    0x32, 0x67, 0x5a, 0xd6, 0x68, 0x38,

    /* U+0041 "A" */
    0x30, 0xc3, 0x12, 0x49, 0xe8, 0x61, 0x84,

    /* U+0042 "B" */
    0xf4, 0x63, 0x1f, 0x46, 0x31, 0xf0,

    /* U+0043 "C" */
    0x32, 0x61, 0x8, 0x42, 0x9, 0x30,

    /* U+0044 "D" */
    0xe4, 0xa3, 0x18, 0xc6, 0x32, 0xe0,

    /* U+0045 "E" */
    0xfc, 0x21, 0xf, 0x42, 0x10, 0xf8,

    /* U+0046 "F" */
    0xfc, 0x21, 0xf, 0x42, 0x10, 0x80,

    /* U+0047 "G" */
    0x74, 0x61, 0xb, 0xc6, 0x31, 0x78,

    /* U+0048 "H" */
    0x8c, 0x63, 0x1f, 0xc6, 0x31, 0x88,

    /* U+0049 "I" */
    0xf9, 0x8, 0x42, 0x10, 0x84, 0xf8,

    /* U+004A "J" */
    0x3c, 0x20, 0x82, 0x8, 0x28, 0xa2, 0x70,

    /* U+004B "K" */
    0x8c, 0x65, 0x4e, 0x4a, 0x51, 0x88,

    /* U+004C "L" */
    0x84, 0x21, 0x8, 0x42, 0x10, 0xf8,

    /* U+004D "M" */
    0x86, 0x1c, 0xf3, 0xb6, 0xd8, 0x61, 0x84,

    /* U+004E "N" */
    0x8c, 0x73, 0x5a, 0xce, 0x31, 0x88,

    /* U+004F "O" */
    0x74, 0x63, 0x18, 0xc6, 0x31, 0x70,

    /* U+0050 "P" */
    0xf4, 0x63, 0x1f, 0x42, 0x10, 0x80,

    /* U+0051 "Q" */
    0x72, 0x28, 0xa2, 0x8a, 0x28, 0xaa, 0x70, 0x30,

    /* U+0052 "R" */
    0xf4, 0x63, 0x1f, 0x52, 0x51, 0x88,

    /* U+0053 "S" */
    0x74, 0x61, 0x7, 0x4, 0x31, 0x70,

    /* U+0054 "T" */
    0xf9, 0x8, 0x42, 0x10, 0x84, 0x20,

    /* U+0055 "U" */
    0x8c, 0x63, 0x18, 0xc6, 0x31, 0x70,

    /* U+0056 "V" */
    0x8c, 0x63, 0x18, 0xa9, 0x44, 0x20,

    /* U+0057 "W" */
    0x83, 0x6, 0x4c, 0x99, 0x2a, 0x9b, 0x22, 0x44,

    /* U+0058 "X" */
    0x8c, 0x54, 0xa2, 0x29, 0x51, 0x88,

    /* U+0059 "Y" */
    0x8c, 0x62, 0xa5, 0x10, 0x84, 0x20,

    /* U+005A "Z" */
    0xf8, 0x44, 0x22, 0x21, 0x10, 0xf8,

    /* U+005B "[" */
    0xf8, 0x88, 0x88, 0x88, 0x88, 0x8f,

    /* U+005C "\\" */
    0x84, 0x10, 0x84, 0x10, 0x82, 0x10, 0x82, 0x10,

    /* U+005D "]" */
    0xf1, 0x11, 0x11, 0x11, 0x11, 0x1f,

    /* U+005E "^" */
    0x31, 0x28, 0x40,

    /* U+005F "_" */
    0xf8,

    /* U+0060 "`" */
    0x99, 0x10,

    /* U+0061 "a" */
    0x70, 0x42, 0xf8, 0xc5, 0xe0,

    /* U+0062 "b" */
    0x84, 0x21, 0x6c, 0xc6, 0x31, 0xcd, 0x80,

    /* U+0063 "c" */
    0x74, 0x61, 0x8, 0x45, 0xc0,

    /* U+0064 "d" */
    0x8, 0x42, 0xd9, 0xc6, 0x31, 0x9b, 0x40,

    /* U+0065 "e" */
    0x74, 0x63, 0xf8, 0x41, 0xc0,

    /* U+0066 "f" */
    0x19, 0x8, 0x4f, 0x90, 0x84, 0x21, 0x0,

    /* U+0067 "g" */
    0x7c, 0x63, 0x19, 0xb4, 0x31, 0x70,

    /* U+0068 "h" */
    0x84, 0x21, 0x6c, 0xc6, 0x31, 0x8c, 0x40,

    /* U+0069 "i" */
    0x21, 0x0, 0xc2, 0x10, 0x84, 0x27, 0xc0,

    /* U+006A "j" */
    0x11, 0x7, 0x11, 0x11, 0x11, 0x96,

    /* U+006B "k" */
    0x84, 0x21, 0x19, 0x53, 0x14, 0x94, 0x40,

    /* U+006C "l" */
    0x61, 0x8, 0x42, 0x10, 0x84, 0x27, 0xc0,

    /* U+006D "m" */
    0xed, 0x26, 0x4c, 0x99, 0x32, 0x64, 0x80,

    /* U+006E "n" */
    0xb6, 0x63, 0x18, 0xc6, 0x20,

    /* U+006F "o" */
    0x74, 0x63, 0x18, 0xc5, 0xc0,

    /* U+0070 "p" */
    0xb6, 0x63, 0x18, 0xe6, 0xd0, 0x80,

    /* U+0071 "q" */
    0x6c, 0xe3, 0x18, 0xcd, 0xa1, 0x8,

    /* U+0072 "r" */
    0xdb, 0x50, 0x84, 0x21, 0x0,

    /* U+0073 "s" */
    0x74, 0x60, 0xe0, 0xc5, 0xc0,

    /* U+0074 "t" */
    0x42, 0x3c, 0x84, 0x21, 0x9, 0x30,

    /* U+0075 "u" */
    0x8c, 0x63, 0x18, 0xcd, 0xa0,

    /* U+0076 "v" */
    0x8c, 0x62, 0xa5, 0x10, 0x80,

    /* U+0077 "w" */
    0x83, 0x26, 0x4c, 0x96, 0xc8, 0x91, 0x0,

    /* U+0078 "x" */
    0x8c, 0x54, 0x45, 0x46, 0x20,

    /* U+0079 "y" */
    0x8c, 0x62, 0xa5, 0x18, 0x84, 0xc0,

    /* U+007A "z" */
    0xf8, 0x44, 0x44, 0x43, 0xe0,

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
    0xb, 0xa7, 0x2a, 0x53, 0x2e, 0x80,

    /* U+00A3 "£" */
    0x18, 0x92, 0x8, 0x78, 0x82, 0x11, 0xfc,

    /* U+00A4 "¤" */
    0x85, 0xe4, 0x92, 0x7a, 0x10,

    /* U+00A5 "¥" */
    0x8c, 0x62, 0xaf, 0x93, 0xe4, 0x20,

    /* U+00A6 "¦" */
    0xf9, 0xf0,

    /* U+00A7 "§" */
    0x69, 0x84, 0xa9, 0x52, 0x19, 0x60,

    /* U+00A8 "¨" */
    0x99,

    /* U+00A9 "©" */
    0x38, 0x8a, 0x6d, 0x1a, 0x34, 0x68, 0xcd, 0x44,
    0x70,

    /* U+00AA "ª" */
    0x61, 0x79, 0x70, 0xf0,

    /* U+00AB "«" */
    0x4c, 0xa4, 0x90,

    /* U+00AC "¬" */
    0xf8, 0x42,

    /* U+00AD "­" */
    0xe0,

    /* U+00AE "®" */
    0x38, 0x8a, 0xcd, 0x5a, 0xb6, 0x6a, 0xd5, 0x44,
    0x70,

    /* U+00AF "¯" */
    0xf8,

    /* U+00B0 "°" */
    0x69, 0x96,

    /* U+00B1 "±" */
    0x21, 0x3e, 0x42, 0x3, 0xe0,

    /* U+00B2 "²" */
    0x69, 0x12, 0x48, 0xf0,

    /* U+00B3 "³" */
    0x69, 0x12, 0x19, 0x60,

    /* U+00B4 "´" */
    0x25, 0x40,

    /* U+00B5 "µ" */
    0x8c, 0x63, 0x18, 0xee, 0xb0, 0x80,

    /* U+00B6 "¶" */
    0x7d, 0x6b, 0x56, 0x94, 0xa5, 0x28,

    /* U+00B7 "·" */
    0xf0,

    /* U+00B8 "¸" */
    0x22, 0x1e,

    /* U+00B9 "¹" */
    0x2e, 0x92, 0x48,

    /* U+00BA "º" */
    0x69, 0x99, 0x60, 0xf0,

    /* U+00BB "»" */
    0x92, 0x53, 0x20,

    /* U+00BC "¼" */
    0x43, 0x4, 0x11, 0x49, 0x42, 0x96, 0xa8, 0xa3,
    0xc2,

    /* U+00BD "½" */
    0x43, 0x4, 0x11, 0x49, 0x43, 0x99, 0x84, 0x62,
    0xf,

    /* U+00BE "¾" */
    0x62, 0x42, 0x5, 0x99, 0xc2, 0x96, 0xa8, 0xa3,
    0xc2,

    /* U+00BF "¿" */
    0x21, 0x0, 0x42, 0x22, 0x11, 0x8b, 0x80,

    /* U+00C0 "À" */
    0x40, 0xc0, 0xc, 0x31, 0x24, 0x9e, 0x86, 0x18,
    0x40,

    /* U+00C1 "Á" */
    0x8, 0xc0, 0xc, 0x31, 0x24, 0x9e, 0x86, 0x18,
    0x40,

    /* U+00C2 "Â" */
    0x31, 0x20, 0xc, 0x31, 0x24, 0x9e, 0x86, 0x18,
    0x40,

    /* U+00C3 "Ã" */
    0x25, 0x60, 0xc, 0x31, 0x24, 0x9e, 0x86, 0x18,
    0x40,

    /* U+00C4 "Ä" */
    0x49, 0x20, 0xc, 0x31, 0x24, 0x9e, 0x86, 0x18,
    0x40,

    /* U+00C5 "Å" */
    0x31, 0x24, 0x8c, 0x31, 0x24, 0x9e, 0x86, 0x18,
    0x40,

    /* U+00C6 "Æ" */
    0x1c, 0xc3, 0x14, 0x5d, 0xc9, 0x24, 0x9c,

    /* U+00C7 "Ç" */
    0x32, 0x61, 0x8, 0x42, 0x9, 0x30, 0x88,

    /* U+00C8 "È" */
    0x41, 0x81, 0xf8, 0x43, 0xd0, 0x84, 0x3e,

    /* U+00C9 "É" */
    0x13, 0x1, 0xf8, 0x43, 0xd0, 0x84, 0x3e,

    /* U+00CA "Ê" */
    0x32, 0x41, 0xf8, 0x43, 0xd0, 0x84, 0x3e,

    /* U+00CB "Ë" */
    0x4a, 0x41, 0xf8, 0x43, 0xd0, 0x84, 0x3e,

    /* U+00CC "Ì" */
    0x41, 0x81, 0xf2, 0x10, 0x84, 0x21, 0x3e,

    /* U+00CD "Í" */
    0x13, 0x1, 0xf2, 0x10, 0x84, 0x21, 0x3e,

    /* U+00CE "Î" */
    0x32, 0x41, 0xf2, 0x10, 0x84, 0x21, 0x3e,

    /* U+00CF "Ï" */
    0x4a, 0x41, 0xf2, 0x10, 0x84, 0x21, 0x3e,

    /* U+00D0 "Ð" */
    0x71, 0x24, 0x51, 0xe5, 0x14, 0x52, 0x70,

    /* U+00D1 "Ñ" */
    0x4d, 0x81, 0x18, 0xe6, 0xb5, 0x9c, 0x62,

    /* U+00D2 "Ò" */
    0x41, 0x80, 0xe8, 0xc6, 0x31, 0x8c, 0x5c,

    /* U+00D3 "Ó" */
    0x13, 0x0, 0xe8, 0xc6, 0x31, 0x8c, 0x5c,

    /* U+00D4 "Ô" */
    0x32, 0x40, 0xe8, 0xc6, 0x31, 0x8c, 0x5c,

    /* U+00D5 "Õ" */
    0x4d, 0x80, 0xe8, 0xc6, 0x31, 0x8c, 0x5c,

    /* U+00D6 "Ö" */
    0x4a, 0x40, 0xe8, 0xc6, 0x31, 0x8c, 0x5c,

    /* U+00D7 "×" */
    0x8a, 0x88, 0xa8, 0x80,

    /* U+00D8 "Ø" */
    0x3a, 0x89, 0x32, 0x65, 0x4c, 0x99, 0x22, 0xb8,

    /* U+00D9 "Ù" */
    0x41, 0x81, 0x18, 0xc6, 0x31, 0x8c, 0x5c,

    /* U+00DA "Ú" */
    0x13, 0x1, 0x18, 0xc6, 0x31, 0x8c, 0x5c,

    /* U+00DB "Û" */
    0x32, 0x41, 0x18, 0xc6, 0x31, 0x8c, 0x5c,

    /* U+00DC "Ü" */
    0x4a, 0x41, 0x18, 0xc6, 0x31, 0x8c, 0x5c,

    /* U+00DD "Ý" */
    0x13, 0x1, 0x18, 0xa9, 0x44, 0x21, 0x8,

    /* U+00DE "Þ" */
    0x84, 0x3d, 0x18, 0xc7, 0xd0, 0x80,

    /* U+00DF "ß" */
    0x64, 0xa5, 0x4b, 0x46, 0x31, 0x8d, 0x80,

    /* U+00E0 "à" */
    0x21, 0x4, 0x7, 0x4, 0x2f, 0x8c, 0x5e,

    /* U+00E1 "á" */
    0x10, 0x88, 0x7, 0x4, 0x2f, 0x8c, 0x5e,

    /* U+00E2 "â" */
    0x32, 0x40, 0xe0, 0x85, 0xf1, 0x8b, 0xc0,

    /* U+00E3 "ã" */
    0x4d, 0x80, 0xe0, 0x85, 0xf1, 0x8b, 0xc0,

    /* U+00E4 "ä" */
    0x4a, 0x40, 0xe0, 0x85, 0xf1, 0x8b, 0xc0,

    /* U+00E5 "å" */
    0x22, 0x88, 0x7, 0x4, 0x2f, 0x8c, 0x5e,

    /* U+00E6 "æ" */
    0x6c, 0x24, 0x4b, 0xf9, 0x12, 0x1b, 0x80,

    /* U+00E7 "ç" */
    0x74, 0x61, 0x8, 0x45, 0xc4, 0x40,

    /* U+00E8 "è" */
    0x42, 0x8, 0x7, 0x46, 0x3f, 0x84, 0x1c,

    /* U+00E9 "é" */
    0x10, 0x88, 0x7, 0x46, 0x3f, 0x84, 0x1c,

    /* U+00EA "ê" */
    0x32, 0x40, 0xe8, 0xc7, 0xf0, 0x83, 0x80,

    /* U+00EB "ë" */
    0x4a, 0x40, 0xe8, 0xc7, 0xf0, 0x83, 0x80,

    /* U+00EC "ì" */
    0x42, 0x8, 0x6, 0x10, 0x84, 0x21, 0x3e,

    /* U+00ED "í" */
    0x10, 0x88, 0x6, 0x10, 0x84, 0x21, 0x3e,

    /* U+00EE "î" */
    0x64, 0x80, 0xc2, 0x10, 0x84, 0x27, 0xc0,

    /* U+00EF "ï" */
    0x94, 0x80, 0xc2, 0x10, 0x84, 0x27, 0xc0,

    /* U+00F0 "ð" */
    0x93, 0x28, 0x65, 0x46, 0x31, 0x8b, 0x80,

    /* U+00F1 "ñ" */
    0x4d, 0x81, 0x6c, 0xc6, 0x31, 0x8c, 0x40,

    /* U+00F2 "ò" */
    0x42, 0x8, 0x7, 0x46, 0x31, 0x8c, 0x5c,

    /* U+00F3 "ó" */
    0x10, 0x88, 0x7, 0x46, 0x31, 0x8c, 0x5c,

    /* U+00F4 "ô" */
    0x32, 0x40, 0xe8, 0xc6, 0x31, 0x8b, 0x80,

    /* U+00F5 "õ" */
    0x4d, 0x80, 0xe8, 0xc6, 0x31, 0x8b, 0x80,

    /* U+00F6 "ö" */
    0x4a, 0x40, 0xe8, 0xc6, 0x31, 0x8b, 0x80,

    /* U+00F7 "÷" */
    0x30, 0xc0, 0x3f, 0x0, 0xc3, 0x0,

    /* U+00F8 "ø" */
    0x3a, 0x89, 0x32, 0xa6, 0x48, 0xae, 0x0,

    /* U+00F9 "ù" */
    0x42, 0x8, 0x8, 0xc6, 0x31, 0x8c, 0xda,

    /* U+00FA "ú" */
    0x10, 0x88, 0x8, 0xc6, 0x31, 0x8c, 0xda,

    /* U+00FB "û" */
    0x32, 0x41, 0x18, 0xc6, 0x31, 0x9b, 0x40,

    /* U+00FC "ü" */
    0x4a, 0x41, 0x18, 0xc6, 0x31, 0x9b, 0x40,

    /* U+00FD "ý" */
    0x10, 0x88, 0x8, 0xc6, 0x2a, 0x51, 0x88, 0x4c,
    0x0,

    /* U+00FE "þ" */
    0x84, 0x21, 0x6c, 0xc6, 0x31, 0xcd, 0xa1, 0x0,

    /* U+00FF "ÿ" */
    0x4a, 0x41, 0x18, 0xc5, 0x4a, 0x31, 0x9, 0x80,

    /* U+20AC "€" */
    0x18, 0x94, 0x3e, 0x43, 0xc4, 0x9, 0x18,

    /* U+2116 "№" */
    0x95, 0x37, 0x6e, 0xab, 0x17, 0xec, 0x48, 0x90
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 112, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 112, .box_w = 1, .box_h = 10, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 3, .adv_w = 112, .box_w = 4, .box_h = 3, .ofs_x = 1, .ofs_y = 7},
    {.bitmap_index = 5, .adv_w = 112, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 10, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 17, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 23, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 30, .adv_w = 112, .box_w = 1, .box_h = 4, .ofs_x = 3, .ofs_y = 6},
    {.bitmap_index = 31, .adv_w = 112, .box_w = 3, .box_h = 12, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 36, .adv_w = 112, .box_w = 3, .box_h = 12, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 41, .adv_w = 112, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 45, .adv_w = 112, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 49, .adv_w = 112, .box_w = 3, .box_h = 4, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 51, .adv_w = 112, .box_w = 5, .box_h = 1, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 52, .adv_w = 112, .box_w = 2, .box_h = 2, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 53, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 61, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 67, .adv_w = 112, .box_w = 3, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 71, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 77, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 83, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 89, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 95, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 101, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 107, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 113, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 119, .adv_w = 112, .box_w = 2, .box_h = 6, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 121, .adv_w = 112, .box_w = 3, .box_h = 8, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 124, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 130, .adv_w = 112, .box_w = 5, .box_h = 4, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 133, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 139, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 146, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 152, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 159, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 165, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 171, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 177, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 183, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 189, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 195, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 201, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 207, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 214, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 220, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 226, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 233, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 239, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 245, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 251, .adv_w = 112, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 259, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 265, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 271, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 277, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 283, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 289, .adv_w = 112, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 297, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 303, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 309, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 315, .adv_w = 112, .box_w = 4, .box_h = 12, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 321, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 329, .adv_w = 112, .box_w = 4, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 335, .adv_w = 112, .box_w = 6, .box_h = 3, .ofs_x = 0, .ofs_y = 6},
    {.bitmap_index = 338, .adv_w = 112, .box_w = 5, .box_h = 1, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 339, .adv_w = 112, .box_w = 3, .box_h = 4, .ofs_x = 2, .ofs_y = 6},
    {.bitmap_index = 341, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 346, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 353, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 358, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 365, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 370, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 377, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 383, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 390, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 397, .adv_w = 112, .box_w = 4, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 403, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 410, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 417, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 424, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 429, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 434, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 440, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 446, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 451, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 456, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 462, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 467, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 472, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 479, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 484, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 490, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 495, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 503, .adv_w = 112, .box_w = 1, .box_h = 12, .ofs_x = 3, .ofs_y = -1},
    {.bitmap_index = 505, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 513, .adv_w = 112, .box_w = 6, .box_h = 3, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 516, .adv_w = 112, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 517, .adv_w = 112, .box_w = 1, .box_h = 10, .ofs_x = 3, .ofs_y = -2},
    {.bitmap_index = 519, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 525, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 532, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 537, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 543, .adv_w = 112, .box_w = 1, .box_h = 12, .ofs_x = 3, .ofs_y = -1},
    {.bitmap_index = 545, .adv_w = 112, .box_w = 4, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 551, .adv_w = 112, .box_w = 4, .box_h = 2, .ofs_x = 1, .ofs_y = 8},
    {.bitmap_index = 552, .adv_w = 112, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 561, .adv_w = 112, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 565, .adv_w = 112, .box_w = 5, .box_h = 4, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 568, .adv_w = 112, .box_w = 5, .box_h = 3, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 570, .adv_w = 112, .box_w = 3, .box_h = 1, .ofs_x = 2, .ofs_y = 4},
    {.bitmap_index = 571, .adv_w = 112, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 580, .adv_w = 112, .box_w = 5, .box_h = 1, .ofs_x = 1, .ofs_y = 9},
    {.bitmap_index = 581, .adv_w = 112, .box_w = 4, .box_h = 4, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 583, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 588, .adv_w = 112, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 592, .adv_w = 112, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 596, .adv_w = 112, .box_w = 3, .box_h = 4, .ofs_x = 2, .ofs_y = 6},
    {.bitmap_index = 598, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 604, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 610, .adv_w = 112, .box_w = 2, .box_h = 2, .ofs_x = 2, .ofs_y = 3},
    {.bitmap_index = 611, .adv_w = 112, .box_w = 4, .box_h = 4, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 613, .adv_w = 112, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 616, .adv_w = 112, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 620, .adv_w = 112, .box_w = 5, .box_h = 4, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 623, .adv_w = 112, .box_w = 6, .box_h = 12, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 632, .adv_w = 112, .box_w = 6, .box_h = 12, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 641, .adv_w = 112, .box_w = 6, .box_h = 12, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 650, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 657, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 666, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 675, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 684, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 693, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 702, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 711, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 718, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 725, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 732, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 739, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 746, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 753, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 760, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 767, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 774, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 781, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 788, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 795, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 802, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 809, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 816, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 823, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 830, .adv_w = 112, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 834, .adv_w = 112, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 842, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 849, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 856, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 863, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 870, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 877, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 883, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 890, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 897, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 904, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 911, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 918, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 925, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 932, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 939, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 945, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 952, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 959, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 966, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 973, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 980, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 987, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 994, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1001, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1008, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1015, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1022, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1029, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1036, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1043, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1050, .adv_w = 112, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 1056, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1063, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1070, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1077, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1084, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1091, .adv_w = 112, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1100, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1108, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1116, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1123, .adv_w = 112, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0}
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

extern lv_font_t greybeard_14_ext;


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t greybeard_14 = {
#else
lv_font_t greybeard_14 = {
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
    .fallback = &greybeard_14_ext,
#endif
    .user_data = NULL,
};



#endif /*#if GREYBEARD_14*/

