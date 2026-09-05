/*******************************************************************************
 * Size: 11 px
 * Bpp: 1
 * Opts: --font Greybeard-11px.ttf -r 0x400-0x4FF,0x20B4 --size 11 --bpp 1 --format lvgl --no-compress --lv-font-name greybeard_cyrillic_11 -o /tmp/gen/greybeard_cyrillic_11.c
 ******************************************************************************/

#include "lvgl.h"

#ifndef GREYBEARD_CYRILLIC_11
#define GREYBEARD_CYRILLIC_11 1
#endif

#if GREYBEARD_CYRILLIC_11

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0400 "Ѐ" */
    0x42, 0xf8, 0x8e, 0x88, 0xf0,

    /* U+0401 "Ё" */
    0xd8, 0x3d, 0x8, 0x72, 0x10, 0xf0,

    /* U+0402 "Ђ" */
    0xf8, 0x82, 0xe, 0x24, 0x92, 0x49, 0x4, 0x60,

    /* U+0403 "Ѓ" */
    0x24, 0xf8, 0x88, 0x88, 0x80,

    /* U+0404 "Є" */
    0x69, 0x8e, 0x88, 0x96,

    /* U+0405 "Ѕ" */
    0x69, 0x84, 0x21, 0x96,

    /* U+0406 "І" */
    0xe9, 0x24, 0x97,

    /* U+0407 "Ї" */
    0xd8, 0x1c, 0x42, 0x10, 0x84, 0x70,

    /* U+0408 "Ј" */
    0x31, 0x11, 0x19, 0x96,

    /* U+0409 "Љ" */
    0x31, 0x45, 0x16, 0x55, 0x55, 0x76,

    /* U+040A "Њ" */
    0xa5, 0x29, 0xea, 0xd6, 0xb6,

    /* U+040B "Ћ" */
    0xf8, 0x82, 0xe, 0x24, 0x92, 0x49,

    /* U+040C "Ќ" */
    0x24, 0x99, 0xac, 0xa9, 0x90,

    /* U+040D "Ѝ" */
    0x42, 0x99, 0xbb, 0xdd, 0x90,

    /* U+040E "Ў" */
    0x51, 0x23, 0x15, 0x10, 0x88, 0xc0,

    /* U+040F "Џ" */
    0x99, 0x99, 0x99, 0x9f, 0x22,

    /* U+0410 "А" */
    0x66, 0x99, 0xf9, 0x99,

    /* U+0411 "Б" */
    0xe8, 0x8e, 0x99, 0x9e,

    /* U+0412 "В" */
    0xe9, 0x9e, 0x99, 0x9e,

    /* U+0413 "Г" */
    0xf8, 0x88, 0x88, 0x88,

    /* U+0414 "Д" */
    0x72, 0x94, 0xa5, 0x29, 0x5f, 0x8c, 0x40,

    /* U+0415 "Е" */
    0xf8, 0x8e, 0x88, 0x8f,

    /* U+0416 "Ж" */
    0xad, 0x6a, 0xea, 0xd6, 0xb5,

    /* U+0417 "З" */
    0x69, 0x16, 0x11, 0x96,

    /* U+0418 "И" */
    0x99, 0xbb, 0xdd, 0x99,

    /* U+0419 "Й" */
    0x52, 0x99, 0xbb, 0xdd, 0x90,

    /* U+041A "К" */
    0x99, 0xac, 0xaa, 0x99,

    /* U+041B "Л" */
    0x19, 0x52, 0x94, 0xa5, 0x39,

    /* U+041C "М" */
    0x8e, 0xf7, 0x5a, 0xc6, 0x31,

    /* U+041D "Н" */
    0x99, 0x9f, 0x99, 0x99,

    /* U+041E "О" */
    0x69, 0x99, 0x99, 0x96,

    /* U+041F "П" */
    0xf9, 0x99, 0x99, 0x99,

    /* U+0420 "Р" */
    0xe9, 0x99, 0xe8, 0x88,

    /* U+0421 "С" */
    0x69, 0x88, 0x88, 0x96,

    /* U+0422 "Т" */
    0xf9, 0x8, 0x42, 0x10, 0x84,

    /* U+0423 "У" */
    0x8c, 0x54, 0xa2, 0x11, 0x18,

    /* U+0424 "Ф" */
    0x23, 0xab, 0x5a, 0xd5, 0xc4,

    /* U+0425 "Х" */
    0x99, 0x96, 0x69, 0x99,

    /* U+0426 "Ц" */
    0x94, 0xa5, 0x29, 0x4a, 0x5f, 0x8, 0x40,

    /* U+0427 "Ч" */
    0x99, 0x99, 0x71, 0x11,

    /* U+0428 "Ш" */
    0xad, 0x6b, 0x5a, 0xd6, 0xbf,

    /* U+0429 "Щ" */
    0xad, 0x6b, 0x5a, 0xd6, 0xbf, 0x8, 0x40,

    /* U+042A "Ъ" */
    0xe1, 0x8, 0x62, 0x94, 0xa6,

    /* U+042B "Ы" */
    0x8c, 0x63, 0x9a, 0xd6, 0xb9,

    /* U+042C "Ь" */
    0x88, 0x8e, 0x99, 0x9e,

    /* U+042D "Э" */
    0x69, 0x17, 0x11, 0x96,

    /* U+042E "Ю" */
    0x95, 0x6b, 0xda, 0xd6, 0xb2,

    /* U+042F "Я" */
    0x79, 0x97, 0x55, 0x99,

    /* U+0430 "а" */
    0x61, 0x79, 0x97,

    /* U+0431 "б" */
    0x68, 0x8e, 0x99, 0x96,

    /* U+0432 "в" */
    0xe9, 0xe9, 0x9e,

    /* U+0433 "г" */
    0xf8, 0x88, 0x88,

    /* U+0434 "д" */
    0x72, 0x94, 0xa5, 0x7e, 0x31,

    /* U+0435 "е" */
    0x69, 0xf8, 0x87,

    /* U+0436 "ж" */
    0xad, 0x5d, 0x5a, 0xd4,

    /* U+0437 "з" */
    0x69, 0x21, 0x96,

    /* U+0438 "и" */
    0x99, 0x9b, 0xd9,

    /* U+0439 "й" */
    0x57, 0x9, 0x99, 0xbd, 0x90,

    /* U+043A "к" */
    0x99, 0xae, 0x99,

    /* U+043B "л" */
    0x19, 0x52, 0x94, 0xc4,

    /* U+043C "м" */
    0x8c, 0x77, 0x5a, 0xd4,

    /* U+043D "н" */
    0x99, 0xf9, 0x99,

    /* U+043E "о" */
    0x69, 0x99, 0x96,

    /* U+043F "п" */
    0xf9, 0x99, 0x99,

    /* U+0440 "р" */
    0xe9, 0x99, 0x9e, 0x88,

    /* U+0441 "с" */
    0x69, 0x88, 0x87,

    /* U+0442 "т" */
    0xf9, 0x8, 0x42, 0x10,

    /* U+0443 "у" */
    0x99, 0x95, 0x62, 0x4c,

    /* U+0444 "ф" */
    0x21, 0x1d, 0x5a, 0xd6, 0xae, 0x21, 0x0,

    /* U+0445 "х" */
    0x99, 0x66, 0x99,

    /* U+0446 "ц" */
    0x94, 0xa5, 0x29, 0x7c, 0x21,

    /* U+0447 "ч" */
    0x99, 0x97, 0x11,

    /* U+0448 "ш" */
    0xad, 0x6b, 0x5a, 0xfc,

    /* U+0449 "щ" */
    0xad, 0x6b, 0x5a, 0xfc, 0x21,

    /* U+044A "ъ" */
    0xe5, 0xc, 0x52, 0x98,

    /* U+044B "ы" */
    0x8c, 0x73, 0x5a, 0xe4,

    /* U+044C "ь" */
    0x88, 0xe9, 0x9e,

    /* U+044D "э" */
    0xe1, 0x71, 0x1e,

    /* U+044E "ю" */
    0x95, 0x7b, 0x5a, 0xc8,

    /* U+044F "я" */
    0x79, 0x75, 0x99,

    /* U+0450 "ѐ" */
    0x42, 0x6, 0x9f, 0x88, 0x70,

    /* U+0451 "ё" */
    0x55, 0x6, 0x9f, 0x88, 0x70,

    /* U+0452 "ђ" */
    0x47, 0x90, 0xe4, 0xa5, 0x29, 0x11, 0x0,

    /* U+0453 "ѓ" */
    0x24, 0xf, 0x88, 0x88, 0x80,

    /* U+0454 "є" */
    0x78, 0xe8, 0x87,

    /* U+0455 "ѕ" */
    0x78, 0xc3, 0x1e,

    /* U+0456 "і" */
    0x48, 0x64, 0x92, 0xe0,

    /* U+0457 "ї" */
    0xb4, 0x64, 0x92, 0xe0,

    /* U+0458 "ј" */
    0x11, 0x3, 0x11, 0x11, 0x19, 0x60,

    /* U+0459 "љ" */
    0x31, 0x45, 0x95, 0x57, 0x60,

    /* U+045A "њ" */
    0xa5, 0x3d, 0x5a, 0xd8,

    /* U+045B "ћ" */
    0x47, 0x90, 0xb6, 0xa5, 0x29,

    /* U+045C "ќ" */
    0x24, 0x9, 0x9a, 0xe9, 0x90,

    /* U+045D "ѝ" */
    0x42, 0x9, 0x99, 0xbd, 0x90,

    /* U+045E "ў" */
    0x57, 0x9, 0x99, 0x56, 0x24, 0xc0,

    /* U+045F "џ" */
    0x99, 0x99, 0x9f, 0x22,

    /* U+0460 "Ѡ" */
    0x8d, 0x6b, 0x5a, 0xd6, 0xaa,

    /* U+0461 "ѡ" */
    0x8d, 0x6b, 0x5a, 0xa8,

    /* U+0462 "Ѣ" */
    0x23, 0xe2, 0xe, 0x24, 0x92, 0x4e,

    /* U+0463 "ѣ" */
    0x42, 0x3c, 0x87, 0x25, 0x2e,

    /* U+0464 "Ѥ" */
    0x9d, 0x69, 0xfa, 0x52, 0xb3,

    /* U+0465 "ѥ" */
    0x9d, 0x3f, 0x4a, 0x4c,

    /* U+0466 "Ѧ" */
    0x21, 0x14, 0xa7, 0x56, 0xb5,

    /* U+0467 "ѧ" */
    0x21, 0x14, 0xea, 0xd4,

    /* U+0468 "Ѩ" */
    0x92, 0x49, 0x2a, 0xaa, 0xfd, 0x75,

    /* U+0469 "ѩ" */
    0x92, 0x4e, 0xae, 0xd7, 0x50,

    /* U+046A "Ѫ" */
    0xfc, 0x54, 0xea, 0xd6, 0xb5,

    /* U+046B "ѫ" */
    0xfc, 0x54, 0xea, 0xd4,

    /* U+046C "Ѭ" */
    0xfe, 0xaa, 0xbc, 0x92, 0xed, 0x75,

    /* U+046D "ѭ" */
    0xff, 0x1a, 0xa4, 0xbb, 0x50,

    /* U+046E "Ѯ" */
    0x52, 0x69, 0x16, 0x11, 0x68, 0x60,

    /* U+046F "ѯ" */
    0x52, 0x69, 0x21, 0x16, 0x86,

    /* U+0470 "Ѱ" */
    0xad, 0x6b, 0x57, 0x10, 0x84,

    /* U+0471 "ѱ" */
    0x21, 0x2b, 0x5a, 0xd6, 0xae, 0x21, 0x0,

    /* U+0472 "Ѳ" */
    0x69, 0x9f, 0x99, 0x96,

    /* U+0473 "ѳ" */
    0x69, 0xf9, 0x96,

    /* U+0474 "Ѵ" */
    0x8c, 0x64, 0xa5, 0x30, 0x84,

    /* U+0475 "ѵ" */
    0x8c, 0x94, 0xc2, 0x10,

    /* U+0476 "Ѷ" */
    0x92, 0x63, 0x25, 0x29, 0x84, 0x20,

    /* U+0477 "ѷ" */
    0x92, 0x41, 0x19, 0x29, 0x84, 0x20,

    /* U+0478 "Ѹ" */
    0x42, 0x8b, 0x6d, 0xb6, 0xba, 0x92, 0x10, 0x40,

    /* U+0479 "ѹ" */
    0x56, 0xdb, 0x6b, 0xa9, 0x21, 0x4,

    /* U+047A "Ѻ" */
    0x23, 0xab, 0x18, 0xc6, 0x35, 0x71, 0x0,

    /* U+047B "ѻ" */
    0x23, 0xab, 0x18, 0xd5, 0xc4,

    /* U+047C "Ѽ" */
    0x65, 0xe3, 0x18, 0xd6, 0xb5, 0x50,

    /* U+047D "ѽ" */
    0x65, 0xc1, 0x18, 0xc6, 0xb5, 0x50,

    /* U+047E "Ѿ" */
    0xf9, 0x23, 0x5a, 0xd6, 0xb5, 0x50,

    /* U+047F "ѿ" */
    0xfd, 0x41, 0x1a, 0xd6, 0xb5, 0x50,

    /* U+0480 "Ҁ" */
    0x69, 0x88, 0x88, 0x86, 0x22,

    /* U+0481 "ҁ" */
    0x79, 0x88, 0x86, 0x22,

    /* U+0482 "҂" */
    0x11, 0x63, 0xc6, 0x88,

    /* U+0483 "҃" */
    0x3e, 0x0,

    /* U+0484 "҄" */
    0x69,

    /* U+0485 "҅" */
    0xe4,

    /* U+0486 "҆" */
    0xd8,

    /* U+0487 "҇" */
    0x64, 0xc0,

    /* U+048A "Ҋ" */
    0x51, 0x25, 0x2b, 0x5b, 0x52, 0xb8, 0x88,

    /* U+048B "ҋ" */
    0x53, 0x81, 0x29, 0x5b, 0x52, 0xb8, 0x88,

    /* U+048C "Ҍ" */
    0x47, 0x10, 0xe4, 0xa5, 0x2e,

    /* U+048D "ҍ" */
    0x47, 0x10, 0xe4, 0xb8,

    /* U+048E "Ҏ" */
    0xe9, 0x9b, 0xe9, 0x88,

    /* U+048F "ҏ" */
    0xe9, 0x99, 0xbe, 0x98,

    /* U+0490 "Ґ" */
    0x11, 0xf8, 0x88, 0x88, 0x80,

    /* U+0491 "ґ" */
    0x11, 0xf8, 0x88, 0x88,

    /* U+0492 "Ғ" */
    0x7a, 0x10, 0x8f, 0x21, 0x8,

    /* U+0493 "ғ" */
    0x7a, 0x11, 0xe4, 0x20,

    /* U+0494 "Ҕ" */
    0xf8, 0x88, 0xe9, 0x99, 0x16,

    /* U+0495 "ҕ" */
    0xf8, 0x8e, 0x99, 0x16,

    /* U+0496 "Җ" */
    0xad, 0x6a, 0xea, 0xd6, 0xb5, 0x8, 0x40,

    /* U+0497 "җ" */
    0xad, 0x5d, 0x5a, 0xd4, 0x21,

    /* U+0498 "Ҙ" */
    0x69, 0x16, 0x11, 0x96, 0x24,

    /* U+0499 "ҙ" */
    0x69, 0x21, 0x96, 0x24,

    /* U+049A "Қ" */
    0x94, 0xa9, 0x8a, 0x52, 0x53, 0x8, 0x40,

    /* U+049B "қ" */
    0x94, 0xa9, 0xc9, 0x4c, 0x21,

    /* U+049C "Ҝ" */
    0x8d, 0x6d, 0xcb, 0x56, 0x31,

    /* U+049D "ҝ" */
    0xad, 0xb9, 0x6a, 0xc4,

    /* U+049E "Ҟ" */
    0x4f, 0x54, 0xc5, 0x29, 0x29,

    /* U+049F "ҟ" */
    0x47, 0x90, 0x95, 0x39, 0x29,

    /* U+04A0 "Ҡ" */
    0xe4, 0x92, 0x8c, 0x28, 0xa2, 0x49,

    /* U+04A1 "ҡ" */
    0xe4, 0x92, 0x8e, 0x24, 0x90,

    /* U+04A2 "Ң" */
    0x94, 0xa5, 0xe9, 0x4a, 0x53, 0x8, 0x40,

    /* U+04A3 "ң" */
    0x94, 0xbd, 0x29, 0x4c, 0x21,

    /* U+04A4 "Ҥ" */
    0xbd, 0x29, 0xca, 0x52, 0x94,

    /* U+04A5 "ҥ" */
    0xbd, 0x39, 0x4a, 0x50,

    /* U+04A6 "Ҧ" */
    0xe5, 0x29, 0x6a, 0xd6, 0xb5, 0x9, 0x80,

    /* U+04A7 "ҧ" */
    0xe5, 0x2d, 0x5a, 0xd4, 0x26,

    /* U+04A8 "Ҩ" */
    0x74, 0x65, 0x5a, 0xd6, 0x4f,

    /* U+04A9 "ҩ" */
    0x74, 0x65, 0x59, 0x3c,

    /* U+04AA "Ҫ" */
    0x69, 0x88, 0x88, 0x96, 0x24,

    /* U+04AB "ҫ" */
    0x79, 0x88, 0x87, 0x24,

    /* U+04AC "Ҭ" */
    0xf9, 0x8, 0x42, 0x10, 0x86, 0x10, 0x80,

    /* U+04AD "ҭ" */
    0xf9, 0x8, 0x42, 0x18, 0x42,

    /* U+04AE "Ү" */
    0x8c, 0x54, 0xa2, 0x10, 0x84,

    /* U+04AF "ү" */
    0x8c, 0x54, 0xa2, 0x10, 0x84,

    /* U+04B0 "Ұ" */
    0x8c, 0x54, 0xa2, 0x7c, 0x84,

    /* U+04B1 "ұ" */
    0x8c, 0x54, 0xa2, 0x7c, 0x84,

    /* U+04B2 "Ҳ" */
    0x94, 0xa4, 0xc6, 0x4a, 0x53, 0x8, 0x40,

    /* U+04B3 "ҳ" */
    0x94, 0x98, 0xc9, 0x4c, 0x21,

    /* U+04B4 "Ҵ" */
    0xe9, 0x24, 0x92, 0x49, 0x24, 0x9f, 0x4, 0x10,

    /* U+04B5 "ҵ" */
    0xe9, 0x24, 0x92, 0x49, 0xf0, 0x41,

    /* U+04B6 "Ҷ" */
    0x94, 0xa5, 0x27, 0x8, 0x43, 0x8, 0x40,

    /* U+04B7 "ҷ" */
    0x94, 0xa4, 0xe1, 0xc, 0x21,

    /* U+04B8 "Ҹ" */
    0x8c, 0x6b, 0x57, 0x94, 0xa1,

    /* U+04B9 "ҹ" */
    0x8d, 0x6a, 0xf2, 0x84,

    /* U+04BA "Һ" */
    0x88, 0x8e, 0x99, 0x99,

    /* U+04BB "һ" */
    0x88, 0xbd, 0x99, 0x99,

    /* U+04BC "Ҽ" */
    0xb6, 0x72, 0xf4, 0x21, 0x26,

    /* U+04BD "ҽ" */
    0xb6, 0x7e, 0x84, 0x1c,

    /* U+04BE "Ҿ" */
    0xb6, 0x72, 0xf4, 0x21, 0x26, 0x10, 0x80,

    /* U+04BF "ҿ" */
    0xb6, 0x7e, 0x84, 0x1c, 0x42,

    /* U+04C0 "Ӏ" */
    0xe9, 0x24, 0x97,

    /* U+04C1 "Ӂ" */
    0x51, 0x23, 0x5a, 0xba, 0xb5, 0xa8,

    /* U+04C2 "ӂ" */
    0x53, 0x81, 0x5a, 0xba, 0xb5, 0xa8,

    /* U+04C3 "Ӄ" */
    0x99, 0xac, 0xa9, 0x99, 0x16,

    /* U+04C4 "ӄ" */
    0x99, 0xae, 0x99, 0x16,

    /* U+04C5 "Ӆ" */
    0x18, 0xa4, 0x92, 0x49, 0x24, 0xb7, 0x8, 0x40,

    /* U+04C6 "ӆ" */
    0x18, 0xa4, 0x92, 0x4a, 0x70, 0x84,

    /* U+04C7 "Ӈ" */
    0x99, 0x9f, 0x99, 0x99, 0x16,

    /* U+04C8 "ӈ" */
    0x99, 0xf9, 0x99, 0x16,

    /* U+04C9 "Ӊ" */
    0x94, 0xa5, 0xe9, 0x4a, 0x57, 0x11, 0x0,

    /* U+04CA "ӊ" */
    0x94, 0xbd, 0x29, 0x5c, 0x44,

    /* U+04CB "Ӌ" */
    0x99, 0x99, 0x71, 0x13, 0x22,

    /* U+04CC "ӌ" */
    0x99, 0x97, 0x13, 0x22,

    /* U+04CD "Ӎ" */
    0x8e, 0xf7, 0x5a, 0xc6, 0x33, 0x8, 0x80,

    /* U+04CE "ӎ" */
    0x8e, 0xeb, 0x58, 0xcc, 0x22,

    /* U+04CF "ӏ" */
    0xe9, 0x24, 0x97,

    /* U+04D0 "Ӑ" */
    0x96, 0x66, 0x9f, 0x99, 0x90,

    /* U+04D1 "ӑ" */
    0x96, 0x6, 0x17, 0x99, 0x70,

    /* U+04D2 "Ӓ" */
    0xd8, 0x18, 0xc9, 0x7a, 0x52, 0x90,

    /* U+04D3 "ӓ" */
    0x55, 0x6, 0x17, 0x99, 0x70,

    /* U+04D4 "Ӕ" */
    0x7b, 0x29, 0x7e, 0x52, 0x97,

    /* U+04D5 "ӕ" */
    0xd9, 0x5f, 0x4a, 0x6c,

    /* U+04D6 "Ӗ" */
    0x96, 0xf8, 0x8e, 0x88, 0xf0,

    /* U+04D7 "ӗ" */
    0x96, 0x6, 0x9f, 0x88, 0x70,

    /* U+04D8 "Ә" */
    0x69, 0x11, 0xf9, 0x96,

    /* U+04D9 "ә" */
    0xe1, 0x1f, 0x96,

    /* U+04DA "Ӛ" */
    0xd8, 0x19, 0x21, 0x7a, 0x52, 0x60,

    /* U+04DB "ӛ" */
    0x55, 0xe, 0x11, 0xf9, 0x60,

    /* U+04DC "Ӝ" */
    0xd8, 0x2b, 0x5a, 0xba, 0xb5, 0xa8,

    /* U+04DD "ӝ" */
    0x52, 0x81, 0x5a, 0xba, 0xb5, 0xa8,

    /* U+04DE "Ӟ" */
    0xd8, 0x19, 0x21, 0x30, 0x52, 0x60,

    /* U+04DF "ӟ" */
    0x55, 0x6, 0x92, 0x19, 0x60,

    /* U+04E0 "Ӡ" */
    0xf1, 0x26, 0x11, 0x96,

    /* U+04E1 "ӡ" */
    0xf1, 0x26, 0x11, 0x96,

    /* U+04E2 "Ӣ" */
    0xf0, 0x99, 0xbb, 0xdd, 0x90,

    /* U+04E3 "ӣ" */
    0xf0, 0x99, 0x9b, 0xd9,

    /* U+04E4 "Ӥ" */
    0xd8, 0x25, 0x2b, 0x5b, 0x5a, 0x90,

    /* U+04E5 "ӥ" */
    0x55, 0x9, 0x99, 0xbd, 0x90,

    /* U+04E6 "Ӧ" */
    0xd8, 0x19, 0x29, 0x4a, 0x52, 0x60,

    /* U+04E7 "ӧ" */
    0x55, 0x6, 0x99, 0x99, 0x60,

    /* U+04E8 "Ө" */
    0x69, 0x9f, 0x99, 0x96,

    /* U+04E9 "ө" */
    0x69, 0xf9, 0x96,

    /* U+04EA "Ӫ" */
    0xd8, 0x19, 0x29, 0x7a, 0x52, 0x60,

    /* U+04EB "ӫ" */
    0x55, 0x6, 0x9f, 0x99, 0x60,

    /* U+04EC "Ӭ" */
    0xd8, 0x19, 0x21, 0x38, 0x52, 0x60,

    /* U+04ED "ӭ" */
    0xaa, 0xe, 0x17, 0x11, 0xe0,

    /* U+04EE "Ӯ" */
    0x70, 0x23, 0x15, 0x10, 0x88, 0xc0,

    /* U+04EF "ӯ" */
    0x70, 0x99, 0x95, 0x62, 0x4c,

    /* U+04F0 "Ӱ" */
    0xd8, 0x23, 0x15, 0x10, 0x88, 0xc0,

    /* U+04F1 "ӱ" */
    0x55, 0x9, 0x99, 0x56, 0x24, 0xc0,

    /* U+04F2 "Ӳ" */
    0x4c, 0x81, 0x18, 0xa8, 0x88, 0xc0,

    /* U+04F3 "ӳ" */
    0x4c, 0x81, 0x29, 0x49, 0x4c, 0x22, 0x30,

    /* U+04F4 "Ӵ" */
    0xd8, 0x25, 0x29, 0x38, 0x42, 0x10,

    /* U+04F5 "ӵ" */
    0x55, 0x9, 0x99, 0x71, 0x10,

    /* U+04F6 "Ӷ" */
    0xf8, 0x88, 0x88, 0x8c, 0x44,

    /* U+04F7 "ӷ" */
    0xf8, 0x88, 0x8c, 0x44,

    /* U+04F8 "Ӹ" */
    0xd8, 0x23, 0x1c, 0xd6, 0xb5, 0xc8,

    /* U+04F9 "ӹ" */
    0x52, 0x81, 0x18, 0xe6, 0xb5, 0xc8,

    /* U+04FA "Ӻ" */
    0x7a, 0x10, 0x8f, 0x21, 0xc, 0x13, 0x0,

    /* U+04FB "ӻ" */
    0x7a, 0x11, 0xe4, 0x30, 0x4c,

    /* U+04FC "Ӽ" */
    0x99, 0x96, 0x69, 0x99, 0x16,

    /* U+04FD "ӽ" */
    0x99, 0x66, 0x99, 0x16,

    /* U+04FE "Ӿ" */
    0x99, 0x6f, 0x69, 0x99,

    /* U+04FF "ӿ" */
    0x96, 0xf6, 0x99,

    /* U+20B4 "₴" */
    0x69, 0x1f, 0xf8, 0x96
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 5, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 11, .adv_w = 96, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 19, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 24, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 28, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 32, .adv_w = 96, .box_w = 3, .box_h = 8, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 35, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 41, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 45, .adv_w = 96, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 51, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 56, .adv_w = 96, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 62, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 67, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 72, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 78, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 83, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 87, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 91, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 95, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 99, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 106, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 110, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 115, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 119, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 123, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 128, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 132, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 137, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 142, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 146, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 150, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 154, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 158, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 162, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 167, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 172, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 177, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 181, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 188, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 192, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 197, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 204, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 209, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 214, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 218, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 222, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 227, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 231, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 234, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 238, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 241, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 244, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 249, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 252, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 256, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 259, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 262, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 267, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 270, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 274, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 278, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 281, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 284, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 287, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 291, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 294, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 298, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 302, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 309, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 312, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 317, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 320, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 324, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 329, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 333, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 337, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 340, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 343, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 347, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 350, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 355, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 360, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 367, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 372, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 375, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 378, .adv_w = 96, .box_w = 3, .box_h = 9, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 382, .adv_w = 96, .box_w = 3, .box_h = 9, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 386, .adv_w = 96, .box_w = 4, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 392, .adv_w = 96, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 397, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 401, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 406, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 411, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 416, .adv_w = 96, .box_w = 4, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 422, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 426, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 431, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 435, .adv_w = 96, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 441, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 446, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 451, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 455, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 460, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 464, .adv_w = 96, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 470, .adv_w = 96, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 475, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 480, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 484, .adv_w = 96, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 490, .adv_w = 96, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 495, .adv_w = 96, .box_w = 4, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 501, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 506, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 511, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 518, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 522, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 525, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 530, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 534, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 540, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 546, .adv_w = 96, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 554, .adv_w = 96, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 560, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 567, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 572, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 578, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 584, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 590, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 596, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 601, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 605, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 609, .adv_w = 96, .box_w = 3, .box_h = 3, .ofs_x = 2, .ofs_y = 6},
    {.bitmap_index = 611, .adv_w = 96, .box_w = 4, .box_h = 2, .ofs_x = 2, .ofs_y = 7},
    {.bitmap_index = 612, .adv_w = 96, .box_w = 2, .box_h = 3, .ofs_x = 2, .ofs_y = 6},
    {.bitmap_index = 613, .adv_w = 96, .box_w = 2, .box_h = 3, .ofs_x = 2, .ofs_y = 6},
    {.bitmap_index = 614, .adv_w = 96, .box_w = 5, .box_h = 2, .ofs_x = 1, .ofs_y = 7},
    {.bitmap_index = 616, .adv_w = 96, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 623, .adv_w = 96, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 630, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 635, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 639, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 643, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 647, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 652, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 656, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 661, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 665, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 670, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 674, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 681, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 686, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 691, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 695, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 702, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 707, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 712, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 716, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 721, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 726, .adv_w = 96, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 732, .adv_w = 96, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 737, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 744, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 749, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 754, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 758, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 765, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 770, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 775, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 779, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 784, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 788, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 795, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 800, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 805, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 810, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 815, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 820, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 827, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 832, .adv_w = 96, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 840, .adv_w = 96, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 846, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 853, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 858, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 863, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 867, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 871, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 875, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 880, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 884, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 891, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 896, .adv_w = 96, .box_w = 3, .box_h = 8, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 899, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 905, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 911, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 916, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 920, .adv_w = 96, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 928, .adv_w = 96, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 934, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 939, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 943, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 950, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 955, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 960, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 964, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 971, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 976, .adv_w = 96, .box_w = 3, .box_h = 8, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 979, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 984, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 989, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 995, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1000, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1005, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1009, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1014, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1019, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1023, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1026, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1032, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1037, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1043, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1049, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1055, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1060, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1064, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1068, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1073, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1077, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1083, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1088, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1094, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1099, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1103, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1106, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1112, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1117, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1123, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1128, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1134, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1139, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1145, .adv_w = 96, .box_w = 4, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1151, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1157, .adv_w = 96, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1164, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1170, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1175, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 1180, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1184, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1190, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1196, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1203, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1208, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1213, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1217, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1221, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1224, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 1024, .range_length = 136, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 1162, .range_length = 118, .glyph_id_start = 137,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 8372, .range_length = 1, .glyph_id_start = 255,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
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



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t greybeard_cyrillic_11 = {
#else
lv_font_t greybeard_cyrillic_11 = {
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
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if GREYBEARD_CYRILLIC_11*/

