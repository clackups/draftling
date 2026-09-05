/*******************************************************************************
 * Size: 16 px
 * Bpp: 1
 * Opts: --font Greybeard-16px.ttf -r 0x400-0x4FF,0x20B4 --size 16 --bpp 1 --format lvgl --no-compress --lv-font-name greybeard_cyrillic_16 -o /tmp/gen/greybeard_cyrillic_16.c
 ******************************************************************************/

#include "lvgl.h"

#ifndef GREYBEARD_CYRILLIC_16
#define GREYBEARD_CYRILLIC_16 1
#endif

#if GREYBEARD_CYRILLIC_16

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0400 "Ѐ" */
    0x40, 0xc0, 0x3f, 0x82, 0x8, 0x3c, 0x82, 0x8,
    0x3f,

    /* U+0401 "Ё" */
    0x49, 0x20, 0x3f, 0x82, 0x8, 0x3c, 0x82, 0x8,
    0x3f,

    /* U+0402 "Ђ" */
    0xfe, 0x10, 0x10, 0x1e, 0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x1, 0x1, 0x6,

    /* U+0403 "Ѓ" */
    0x8, 0xc0, 0x3f, 0x82, 0x8, 0x20, 0x82, 0x8,
    0x20,

    /* U+0404 "Є" */
    0x39, 0x18, 0x20, 0xfa, 0x8, 0x20, 0x44, 0xe0,

    /* U+0405 "Ѕ" */
    0x7a, 0x18, 0x20, 0x60, 0x60, 0x41, 0x85, 0xe0,

    /* U+0406 "І" */
    0xf9, 0x8, 0x42, 0x10, 0x84, 0x27, 0xc0,

    /* U+0407 "Ї" */
    0x4a, 0x41, 0xf2, 0x10, 0x84, 0x21, 0x9, 0xf0,

    /* U+0408 "Ј" */
    0x1c, 0x20, 0x82, 0x8, 0x20, 0xa2, 0x89, 0xc0,

    /* U+0409 "Љ" */
    0x18, 0x28, 0x48, 0x48, 0x4e, 0x49, 0x49, 0x49,
    0x49, 0x8e,

    /* U+040A "Њ" */
    0x91, 0x22, 0x44, 0x8f, 0xd2, 0x64, 0xc9, 0x93,
    0x38,

    /* U+040B "Ћ" */
    0xfe, 0x10, 0x10, 0x1e, 0x11, 0x11, 0x11, 0x11,
    0x11, 0x11,

    /* U+040C "Ќ" */
    0x8, 0xc0, 0x21, 0x86, 0x29, 0x38, 0x92, 0x28,
    0x61,

    /* U+040D "Ѝ" */
    0x40, 0xc0, 0x21, 0x8e, 0x59, 0x69, 0xa7, 0x18,
    0x61,

    /* U+040E "Ў" */
    0x49, 0xe0, 0x21, 0x45, 0x22, 0x8a, 0x10, 0x4a,
    0x18,

    /* U+040F "Џ" */
    0x86, 0x18, 0x61, 0x86, 0x18, 0x61, 0x87, 0xf1,
    0x4, 0x10,

    /* U+0410 "А" */
    0x30, 0xc4, 0x92, 0x86, 0x1f, 0xe1, 0x86, 0x10,

    /* U+0411 "Б" */
    0xfa, 0x8, 0x20, 0xfa, 0x18, 0x61, 0x87, 0xe0,

    /* U+0412 "В" */
    0xfa, 0x18, 0x61, 0xfa, 0x18, 0x61, 0x87, 0xe0,

    /* U+0413 "Г" */
    0xfe, 0x8, 0x20, 0x82, 0x8, 0x20, 0x82, 0x0,

    /* U+0414 "Д" */
    0x7c, 0x89, 0x12, 0x24, 0x48, 0x91, 0x22, 0x45,
    0xfe, 0xc, 0x18, 0x20,

    /* U+0415 "Е" */
    0xfe, 0x8, 0x20, 0xf2, 0x8, 0x20, 0x83, 0xf0,

    /* U+0416 "Ж" */
    0x93, 0x26, 0x4a, 0xa3, 0x8a, 0xa4, 0xc9, 0x93,
    0x24,

    /* U+0417 "З" */
    0x72, 0x20, 0x42, 0x30, 0x20, 0x41, 0x89, 0xc0,

    /* U+0418 "И" */
    0x86, 0x18, 0xe5, 0x96, 0x9a, 0x71, 0x86, 0x10,

    /* U+0419 "Й" */
    0x49, 0xe0, 0x21, 0x8e, 0x59, 0x69, 0xa7, 0x18,
    0x61,

    /* U+041A "К" */
    0x86, 0x18, 0xa4, 0xa3, 0x89, 0x22, 0x86, 0x10,

    /* U+041B "Л" */
    0xe, 0x24, 0x89, 0x12, 0x24, 0x48, 0x91, 0x23,
    0x84,

    /* U+041C "М" */
    0x83, 0x7, 0x1d, 0x5a, 0xb2, 0x64, 0xc1, 0x83,
    0x4,

    /* U+041D "Н" */
    0x86, 0x18, 0x61, 0xfe, 0x18, 0x61, 0x86, 0x10,

    /* U+041E "О" */
    0x7a, 0x18, 0x61, 0x86, 0x18, 0x61, 0x85, 0xe0,

    /* U+041F "П" */
    0xfe, 0x18, 0x61, 0x86, 0x18, 0x61, 0x86, 0x10,

    /* U+0420 "Р" */
    0xfa, 0x18, 0x61, 0x87, 0xe8, 0x20, 0x82, 0x0,

    /* U+0421 "С" */
    0x39, 0x18, 0x20, 0x82, 0x8, 0x20, 0x44, 0xe0,

    /* U+0422 "Т" */
    0xfe, 0x20, 0x40, 0x81, 0x2, 0x4, 0x8, 0x10,
    0x20,

    /* U+0423 "У" */
    0x86, 0x14, 0x52, 0x28, 0xa1, 0x4, 0xa1, 0x80,

    /* U+0424 "Ф" */
    0x10, 0x20, 0xe2, 0xa9, 0x32, 0x64, 0xc9, 0x54,
    0x70, 0x40, 0x80,

    /* U+0425 "Х" */
    0x86, 0x14, 0x92, 0x30, 0xc4, 0x92, 0x86, 0x10,

    /* U+0426 "Ц" */
    0x85, 0xa, 0x14, 0x28, 0x50, 0xa1, 0x42, 0x85,
    0xfc, 0x8, 0x10, 0x20,

    /* U+0427 "Ч" */
    0x86, 0x18, 0x61, 0x85, 0xf0, 0x41, 0x4, 0x10,

    /* U+0428 "Ш" */
    0x93, 0x26, 0x4c, 0x99, 0x32, 0x64, 0xc9, 0x93,
    0xfc,

    /* U+0429 "Щ" */
    0x93, 0x26, 0x4c, 0x99, 0x32, 0x64, 0xc9, 0x93,
    0xfc, 0x8, 0x10, 0x20,

    /* U+042A "Ъ" */
    0xe0, 0x40, 0x81, 0x3, 0xc4, 0x48, 0x91, 0x22,
    0x78,

    /* U+042B "Ы" */
    0x83, 0x6, 0xc, 0x1f, 0x31, 0x62, 0xc5, 0x8b,
    0xe4,

    /* U+042C "Ь" */
    0x82, 0x8, 0x20, 0xfa, 0x18, 0x61, 0x87, 0xe0,

    /* U+042D "Э" */
    0x72, 0x20, 0x41, 0x7c, 0x10, 0x41, 0x89, 0xc0,

    /* U+042E "Ю" */
    0x9d, 0x46, 0x8d, 0x1e, 0x34, 0x68, 0xd1, 0xa3,
    0x38,

    /* U+042F "Я" */
    0x7e, 0x18, 0x61, 0x7c, 0x94, 0x51, 0x86, 0x10,

    /* U+0430 "а" */
    0x78, 0x10, 0x5f, 0x86, 0x37, 0x40,

    /* U+0431 "б" */
    0x8, 0xc4, 0x20, 0xbb, 0x18, 0x61, 0x85, 0xe0,

    /* U+0432 "в" */
    0xfa, 0x18, 0x7e, 0x86, 0x1f, 0x80,

    /* U+0433 "г" */
    0xfe, 0x8, 0x20, 0x82, 0x8, 0x0,

    /* U+0434 "д" */
    0x7c, 0x89, 0x12, 0x24, 0x48, 0xbf, 0xc1, 0x83,
    0x4,

    /* U+0435 "е" */
    0x7a, 0x18, 0x7f, 0x82, 0x7, 0x80,

    /* U+0436 "ж" */
    0x93, 0x25, 0x53, 0xe9, 0x32, 0x64, 0x80,

    /* U+0437 "з" */
    0x7a, 0x10, 0x46, 0x6, 0x17, 0x80,

    /* U+0438 "и" */
    0x86, 0x18, 0xe5, 0xa7, 0x18, 0x40,

    /* U+0439 "й" */
    0x49, 0xe0, 0x21, 0x86, 0x39, 0x69, 0xc6, 0x10,

    /* U+043A "к" */
    0x86, 0x29, 0x38, 0x92, 0x28, 0x40,

    /* U+043B "л" */
    0x1c, 0x94, 0x51, 0x45, 0x18, 0x40,

    /* U+043C "м" */
    0x83, 0x8e, 0xad, 0x59, 0x32, 0x60, 0x80,

    /* U+043D "н" */
    0x86, 0x18, 0x7f, 0x86, 0x18, 0x40,

    /* U+043E "о" */
    0x7a, 0x18, 0x61, 0x86, 0x17, 0x80,

    /* U+043F "п" */
    0xfe, 0x18, 0x61, 0x86, 0x18, 0x40,

    /* U+0440 "р" */
    0xbb, 0x18, 0x61, 0x87, 0x1b, 0xa0, 0x82, 0x0,

    /* U+0441 "с" */
    0x7a, 0x18, 0x20, 0x82, 0x17, 0x80,

    /* U+0442 "т" */
    0xfe, 0x20, 0x40, 0x81, 0x2, 0x4, 0x0,

    /* U+0443 "у" */
    0x86, 0x14, 0x52, 0x28, 0xa1, 0x4, 0xa1, 0x0,

    /* U+0444 "ф" */
    0x10, 0x20, 0x43, 0xe9, 0x32, 0x64, 0xc9, 0x92,
    0xf8, 0x40, 0x81, 0x0,

    /* U+0445 "х" */
    0x86, 0x14, 0x8c, 0x4a, 0x18, 0x40,

    /* U+0446 "ц" */
    0x85, 0xa, 0x14, 0x28, 0x50, 0xbf, 0x81, 0x2,
    0x4,

    /* U+0447 "ч" */
    0x86, 0x18, 0x5f, 0x4, 0x10, 0x40,

    /* U+0448 "ш" */
    0x93, 0x26, 0x4c, 0x99, 0x32, 0x7f, 0x80,

    /* U+0449 "щ" */
    0x93, 0x26, 0x4c, 0x99, 0x32, 0x7f, 0x81, 0x2,
    0x4,

    /* U+044A "ъ" */
    0xe2, 0x82, 0xe, 0x24, 0x93, 0x80,

    /* U+044B "ы" */
    0x86, 0x18, 0x79, 0x96, 0x5e, 0x40,

    /* U+044C "ь" */
    0x84, 0x21, 0xe8, 0xc7, 0xc0,

    /* U+044D "э" */
    0x72, 0x20, 0x4f, 0x6, 0x27, 0x0,

    /* U+044E "ю" */
    0x89, 0x2a, 0x8f, 0x1a, 0x32, 0xa2, 0x0,

    /* U+044F "я" */
    0x7e, 0x18, 0x5f, 0x25, 0x18, 0x40,

    /* U+0450 "ѐ" */
    0x20, 0x81, 0x0, 0x7a, 0x18, 0x7f, 0x82, 0x7,
    0x80,

    /* U+0451 "ё" */
    0x49, 0x20, 0x1e, 0x86, 0x1f, 0xe0, 0x81, 0xe0,

    /* U+0452 "ђ" */
    0x41, 0xf1, 0x2, 0xc6, 0x48, 0x50, 0xa1, 0x42,
    0x88, 0x10, 0x43, 0x0,

    /* U+0453 "ѓ" */
    0x10, 0x42, 0x0, 0xfe, 0x8, 0x20, 0x82, 0x8,
    0x0,

    /* U+0454 "є" */
    0x39, 0x18, 0x3c, 0x81, 0x13, 0x80,

    /* U+0455 "ѕ" */
    0x7a, 0x18, 0x1e, 0x6, 0x17, 0x80,

    /* U+0456 "і" */
    0x21, 0x0, 0xc2, 0x10, 0x84, 0x27, 0xc0,

    /* U+0457 "ї" */
    0x94, 0x80, 0xc2, 0x10, 0x84, 0x27, 0xc0,

    /* U+0458 "ј" */
    0x8, 0x40, 0x70, 0x84, 0x21, 0x8, 0x63, 0x17,
    0x0,

    /* U+0459 "љ" */
    0x18, 0x28, 0x48, 0x4e, 0x49, 0x49, 0x8e,

    /* U+045A "њ" */
    0x91, 0x22, 0x47, 0xe9, 0x32, 0x67, 0x0,

    /* U+045B "ћ" */
    0x41, 0xf1, 0x2, 0xe6, 0x28, 0x50, 0xa1, 0x42,
    0x84,

    /* U+045C "ќ" */
    0x10, 0x42, 0x0, 0x86, 0x29, 0x38, 0x92, 0x28,
    0x40,

    /* U+045D "ѝ" */
    0x20, 0x81, 0x0, 0x86, 0x18, 0xe5, 0xa7, 0x18,
    0x40,

    /* U+045E "ў" */
    0x49, 0xe0, 0x21, 0x85, 0x14, 0x8a, 0x28, 0x41,
    0x28, 0x40,

    /* U+045F "џ" */
    0x86, 0x18, 0x61, 0x86, 0x1f, 0xc4, 0x10, 0x40,

    /* U+0460 "Ѡ" */
    0x83, 0x26, 0x4c, 0x99, 0x32, 0x64, 0xc9, 0x6c,
    0xd8,

    /* U+0461 "ѡ" */
    0x83, 0x26, 0x4c, 0x99, 0x2d, 0x9b, 0x0,

    /* U+0462 "Ѣ" */
    0x20, 0x43, 0xe1, 0x3, 0xc4, 0x48, 0x91, 0x22,
    0x78,

    /* U+0463 "ѣ" */
    0x20, 0x40, 0x87, 0xe2, 0x4, 0xf, 0x11, 0x22,
    0x78,

    /* U+0464 "Ѥ" */
    0x9a, 0x9a, 0x28, 0xfa, 0x8a, 0x28, 0xa6, 0x60,

    /* U+0465 "ѥ" */
    0x9a, 0x9a, 0x3e, 0xa2, 0x99, 0x80,

    /* U+0466 "Ѧ" */
    0x10, 0x20, 0xa1, 0x44, 0x4f, 0x95, 0x49, 0x93,
    0x24,

    /* U+0467 "ѧ" */
    0x10, 0x50, 0xa3, 0xe5, 0x52, 0x64, 0x80,

    /* U+0468 "Ѩ" */
    0x89, 0x12, 0x24, 0xaf, 0x53, 0xaa, 0xd5, 0xab,
    0x54,

    /* U+0469 "ѩ" */
    0x89, 0x12, 0x57, 0xea, 0xb5, 0x6a, 0x80,

    /* U+046A "Ѫ" */
    0xff, 0x5, 0x11, 0x41, 0x7, 0x15, 0x49, 0x93,
    0x24,

    /* U+046B "ѫ" */
    0xff, 0x5, 0x11, 0xc5, 0x52, 0x64, 0x80,

    /* U+046C "Ѭ" */
    0xbf, 0x46, 0x8c, 0xaf, 0xd1, 0x27, 0x55, 0xab,
    0x54,

    /* U+046D "ѭ" */
    0xbf, 0x46, 0x57, 0xc9, 0xd5, 0x6a, 0x80,

    /* U+046E "Ѯ" */
    0x48, 0xc0, 0x1e, 0x84, 0x10, 0x4e, 0x4, 0x10,
    0x5e, 0x81, 0xe0, 0x40,

    /* U+046F "ѯ" */
    0x48, 0xc0, 0x1e, 0x84, 0x11, 0x81, 0x5, 0xe8,
    0x1e, 0x4,

    /* U+0470 "Ѱ" */
    0x93, 0x26, 0x4c, 0x99, 0x32, 0x5f, 0x8, 0x10,
    0x20,

    /* U+0471 "ѱ" */
    0x10, 0x20, 0x44, 0x99, 0x32, 0x64, 0xc9, 0x54,
    0x70, 0x40, 0x81, 0x0,

    /* U+0472 "Ѳ" */
    0x7a, 0x18, 0x61, 0xfe, 0x18, 0x61, 0x85, 0xe0,

    /* U+0473 "ѳ" */
    0x7a, 0x18, 0x7f, 0x86, 0x17, 0x80,

    /* U+0474 "Ѵ" */
    0x83, 0x9, 0x12, 0x44, 0x85, 0xa, 0x18, 0x10,
    0x20,

    /* U+0475 "ѵ" */
    0x83, 0x9, 0x12, 0x42, 0x86, 0x4, 0x0,

    /* U+0476 "Ѷ" */
    0x48, 0x48, 0x4, 0x18, 0x48, 0x92, 0x14, 0x28,
    0x60, 0x40, 0x80,

    /* U+0477 "ѷ" */
    0x48, 0x90, 0x90, 0x8, 0x30, 0x91, 0x24, 0x28,
    0x60, 0x40,

    /* U+0478 "Ѹ" */
    0x41, 0x42, 0x85, 0x5a, 0xb5, 0x6a, 0xd6, 0xa4,
    0x88, 0x10, 0x40, 0x80,

    /* U+0479 "ѹ" */
    0x4b, 0x56, 0xad, 0x5a, 0xd4, 0x91, 0x2, 0x8,
    0x10,

    /* U+047A "Ѻ" */
    0x10, 0xfa, 0x4c, 0x18, 0x30, 0x60, 0xc1, 0x83,
    0x25, 0xf0, 0x80,

    /* U+047B "ѻ" */
    0x10, 0xfa, 0x4c, 0x18, 0x30, 0x64, 0xbe, 0x10,

    /* U+047C "Ѽ" */
    0x71, 0x1c, 0xc2, 0xa8, 0x30, 0x60, 0xdd, 0x93,
    0x26, 0x4b, 0x60,

    /* U+047D "ѽ" */
    0x71, 0x12, 0xd4, 0x90, 0x8, 0xa0, 0xc1, 0xbb,
    0x26, 0x4b, 0x60,

    /* U+047E "Ѿ" */
    0x7c, 0xa8, 0x4, 0x19, 0x32, 0x64, 0xc9, 0x93,
    0x25, 0xb3, 0x60,

    /* U+047F "ѿ" */
    0x7c, 0xa8, 0x0, 0x8, 0x32, 0x64, 0xc9, 0x92,
    0xd9, 0xb0,

    /* U+0480 "Ҁ" */
    0x39, 0x18, 0x20, 0x82, 0x8, 0x20, 0x40, 0xe0,
    0x82, 0x8,

    /* U+0481 "ҁ" */
    0x7a, 0x18, 0x20, 0x82, 0x7, 0x82, 0x8, 0x20,

    /* U+0482 "҂" */
    0x4, 0x91, 0x85, 0x10, 0x8a, 0x18, 0x92, 0x0,

    /* U+0483 "҃" */
    0x1f, 0x80,

    /* U+0484 "҄" */
    0x69,

    /* U+0485 "҅" */
    0xe4,

    /* U+0486 "҆" */
    0xd8,

    /* U+0487 "҇" */
    0x71, 0x12, 0x14, 0x10,

    /* U+048A "Ҋ" */
    0x48, 0xf0, 0x4, 0x28, 0xd2, 0xa5, 0x52, 0xa5,
    0x8a, 0x14, 0x70, 0x20, 0x82, 0x0,

    /* U+048B "ҋ" */
    0x48, 0xf0, 0x4, 0x28, 0x51, 0xa5, 0x52, 0xc5,
    0x1c, 0x8, 0x20, 0x80,

    /* U+048C "Ҍ" */
    0x40, 0x83, 0xe2, 0x7, 0xc8, 0x50, 0xa1, 0x42,
    0xf8,

    /* U+048D "ҍ" */
    0x43, 0xc4, 0x1e, 0x45, 0x17, 0x80,

    /* U+048E "Ҏ" */
    0xfa, 0x18, 0x65, 0x8b, 0xd8, 0x20, 0x82, 0x0,

    /* U+048F "ҏ" */
    0xbb, 0x18, 0x61, 0x97, 0x2b, 0x60, 0x82, 0x0,

    /* U+0490 "Ґ" */
    0x4, 0x1f, 0xe0, 0x82, 0x8, 0x20, 0x82, 0x8,
    0x20,

    /* U+0491 "ґ" */
    0x4, 0x1f, 0xe0, 0x82, 0x8, 0x20, 0x80,

    /* U+0492 "Ғ" */
    0x7e, 0x81, 0x2, 0xf, 0x8, 0x10, 0x20, 0x40,
    0x80,

    /* U+0493 "ғ" */
    0x7e, 0x81, 0x7, 0x84, 0x8, 0x10, 0x0,

    /* U+0494 "Ҕ" */
    0xfe, 0x8, 0x20, 0xf2, 0x28, 0x61, 0x86, 0x10,
    0x42, 0x70,

    /* U+0495 "ҕ" */
    0xfe, 0x8, 0x3c, 0x8a, 0x18, 0x41, 0x9, 0xc0,

    /* U+0496 "Җ" */
    0x93, 0x26, 0x4a, 0xa3, 0x8a, 0xa4, 0xc9, 0x93,
    0x24, 0x8, 0x10, 0x20,

    /* U+0497 "җ" */
    0x93, 0x25, 0x53, 0xe9, 0x32, 0x64, 0x81, 0x2,
    0x4,

    /* U+0498 "Ҙ" */
    0x72, 0x20, 0x42, 0x30, 0x20, 0x41, 0x89, 0xc2,
    0x4, 0x20,

    /* U+0499 "ҙ" */
    0x7a, 0x10, 0x46, 0x6, 0x17, 0x88, 0x10, 0x80,

    /* U+049A "Қ" */
    0x85, 0xa, 0x24, 0x8a, 0x1c, 0x24, 0x44, 0x85,
    0xc, 0x8, 0x10, 0x20,

    /* U+049B "қ" */
    0x86, 0x29, 0x38, 0x92, 0x28, 0xc1, 0x4, 0x10,

    /* U+049C "Ҝ" */
    0x86, 0x1a, 0xaa, 0xf2, 0xaa, 0xa1, 0x86, 0x10,

    /* U+049D "ҝ" */
    0x86, 0x9a, 0xbc, 0xaa, 0x98, 0x40,

    /* U+049E "Ҟ" */
    0x43, 0xe5, 0x12, 0x45, 0xe, 0x12, 0x22, 0x42,
    0x84,

    /* U+049F "ҟ" */
    0x41, 0xe1, 0x2, 0x14, 0x49, 0x1c, 0x24, 0x44,
    0x84,

    /* U+04A0 "Ҡ" */
    0xe2, 0x44, 0x91, 0x22, 0x87, 0x9, 0x12, 0x22,
    0x44,

    /* U+04A1 "ҡ" */
    0xe2, 0x44, 0x91, 0xc2, 0x44, 0x48, 0x80,

    /* U+04A2 "Ң" */
    0x85, 0xa, 0x14, 0x2f, 0xd0, 0xa1, 0x42, 0x85,
    0xc, 0x8, 0x10, 0x20,

    /* U+04A3 "ң" */
    0x85, 0xa, 0x17, 0xe8, 0x50, 0xa1, 0x81, 0x2,
    0x4,

    /* U+04A4 "Ҥ" */
    0x8f, 0x12, 0x24, 0x4f, 0x91, 0x22, 0x44, 0x89,
    0x10,

    /* U+04A5 "ҥ" */
    0x8f, 0x12, 0x27, 0xc8, 0x91, 0x22, 0x0,

    /* U+04A6 "Ҧ" */
    0xf1, 0x22, 0x44, 0x89, 0xd2, 0x64, 0xc9, 0x93,
    0x24, 0x8, 0x10, 0xc0,

    /* U+04A7 "ҧ" */
    0xf1, 0x22, 0x44, 0xe9, 0x32, 0x64, 0x81, 0x2,
    0x18,

    /* U+04A8 "Ҩ" */
    0x79, 0xa, 0x24, 0xa9, 0x52, 0xa5, 0x4a, 0x8c,
    0xf4,

    /* U+04A9 "ҩ" */
    0x79, 0xa, 0x24, 0xa9, 0x51, 0x9e, 0x80,

    /* U+04AA "Ҫ" */
    0x39, 0x18, 0x20, 0x82, 0x8, 0x20, 0x44, 0xe2,
    0x4, 0x20,

    /* U+04AB "ҫ" */
    0x7a, 0x18, 0x20, 0x82, 0x17, 0x88, 0x10, 0x80,

    /* U+04AC "Ҭ" */
    0xfe, 0x20, 0x40, 0x81, 0x2, 0x4, 0x8, 0x10,
    0x30, 0x20, 0x40, 0x80,

    /* U+04AD "ҭ" */
    0xfe, 0x20, 0x40, 0x81, 0x2, 0x6, 0x4, 0x8,
    0x10,

    /* U+04AE "Ү" */
    0x82, 0x89, 0x11, 0x42, 0x82, 0x4, 0x8, 0x10,
    0x20,

    /* U+04AF "ү" */
    0x82, 0x89, 0x11, 0x42, 0x82, 0x4, 0x8, 0x10,
    0x20,

    /* U+04B0 "Ұ" */
    0x82, 0x89, 0x11, 0x42, 0x82, 0x4, 0x3e, 0x10,
    0x20,

    /* U+04B1 "ұ" */
    0x82, 0x89, 0x11, 0x42, 0x82, 0x4, 0x3e, 0x10,
    0x20,

    /* U+04B2 "Ҳ" */
    0x85, 0x9, 0x22, 0x43, 0x6, 0x12, 0x24, 0x85,
    0xc, 0x8, 0x10, 0x20,

    /* U+04B3 "ҳ" */
    0x85, 0x9, 0x21, 0x84, 0x90, 0xa1, 0x81, 0x2,
    0x4,

    /* U+04B4 "Ҵ" */
    0xfa, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
    0x22, 0x3f, 0x1, 0x1, 0x1,

    /* U+04B5 "ҵ" */
    0xfa, 0x22, 0x22, 0x22, 0x22, 0x22, 0x3f, 0x1,
    0x1, 0x1,

    /* U+04B6 "Ҷ" */
    0x85, 0xa, 0x14, 0x28, 0x4f, 0x81, 0x2, 0x4,
    0xc, 0x8, 0x10, 0x20,

    /* U+04B7 "ҷ" */
    0x85, 0xa, 0x13, 0xe0, 0x40, 0x81, 0x81, 0x2,
    0x4,

    /* U+04B8 "Ҹ" */
    0x86, 0x18, 0x65, 0x95, 0xf1, 0x45, 0x4, 0x10,

    /* U+04B9 "ҹ" */
    0x86, 0x59, 0x5f, 0x14, 0x50, 0x40,

    /* U+04BA "Һ" */
    0x82, 0x8, 0x20, 0xfa, 0x18, 0x61, 0x86, 0x10,

    /* U+04BB "һ" */
    0x82, 0x8, 0x2e, 0xc6, 0x18, 0x61, 0x86, 0x10,

    /* U+04BC "Ҽ" */
    0x4d, 0x26, 0x8d, 0x17, 0xe4, 0x8, 0x10, 0x12,
    0x18,

    /* U+04BD "ҽ" */
    0x4d, 0x26, 0x8b, 0xf2, 0x2, 0x43, 0x0,

    /* U+04BE "Ҿ" */
    0x4d, 0x26, 0x8d, 0x17, 0xe4, 0x8, 0x10, 0x12,
    0x18, 0x10, 0x20, 0x40,

    /* U+04BF "ҿ" */
    0x4d, 0x26, 0x8b, 0xf2, 0x2, 0x43, 0x2, 0x4,
    0x8,

    /* U+04C0 "Ӏ" */
    0xe9, 0x24, 0x92, 0x5c,

    /* U+04C1 "Ӂ" */
    0x48, 0xf0, 0x4, 0x99, 0x32, 0x55, 0x1c, 0x55,
    0x26, 0x4c, 0x90,

    /* U+04C2 "ӂ" */
    0x48, 0xf0, 0x4, 0x99, 0x2a, 0x9f, 0x49, 0x93,
    0x24,

    /* U+04C3 "Ӄ" */
    0x86, 0x18, 0xa4, 0xa3, 0x89, 0x22, 0x86, 0x10,
    0x42, 0x70,

    /* U+04C4 "ӄ" */
    0x86, 0x29, 0x3c, 0x8a, 0x18, 0x41, 0x9, 0xc0,

    /* U+04C5 "Ӆ" */
    0xe, 0x12, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
    0x22, 0xc7, 0x1, 0x2, 0x4,

    /* U+04C6 "ӆ" */
    0x1c, 0x49, 0x12, 0x24, 0x48, 0xa3, 0x81, 0x4,
    0x10,

    /* U+04C7 "Ӈ" */
    0x86, 0x18, 0x61, 0xfe, 0x18, 0x61, 0x86, 0x10,
    0x42, 0x70,

    /* U+04C8 "ӈ" */
    0x86, 0x18, 0x7f, 0x86, 0x18, 0x41, 0x9, 0xc0,

    /* U+04C9 "Ӊ" */
    0x85, 0xa, 0x14, 0x2f, 0xd0, 0xa1, 0x42, 0x85,
    0x1c, 0x8, 0x20, 0x80,

    /* U+04CA "ӊ" */
    0x85, 0xa, 0x17, 0xe8, 0x50, 0xa3, 0x81, 0x4,
    0x10,

    /* U+04CB "Ӌ" */
    0x86, 0x18, 0x61, 0x85, 0xf0, 0x41, 0x4, 0x30,
    0x82, 0x8,

    /* U+04CC "ӌ" */
    0x86, 0x18, 0x5f, 0x4, 0x10, 0xc2, 0x8, 0x20,

    /* U+04CD "Ӎ" */
    0x83, 0x7, 0x1d, 0x5a, 0xb2, 0x64, 0xc1, 0x83,
    0xc, 0x8, 0x20, 0x80,

    /* U+04CE "ӎ" */
    0x83, 0x8e, 0xad, 0x59, 0x32, 0x61, 0x81, 0x4,
    0x10,

    /* U+04CF "ӏ" */
    0xe9, 0x24, 0x92, 0x5c,

    /* U+04D0 "Ӑ" */
    0x48, 0xc0, 0xc, 0x31, 0x24, 0xa1, 0xfe, 0x18,
    0x61,

    /* U+04D1 "ӑ" */
    0x48, 0xc0, 0x1e, 0x4, 0x17, 0xe1, 0x8d, 0xd0,

    /* U+04D2 "Ӓ" */
    0x49, 0x20, 0xc, 0x31, 0x24, 0xa1, 0xfe, 0x18,
    0x61,

    /* U+04D3 "ӓ" */
    0x49, 0x20, 0x1e, 0x4, 0x17, 0xe1, 0x8d, 0xd0,

    /* U+04D4 "Ӕ" */
    0x1f, 0x18, 0x28, 0x28, 0x2e, 0x48, 0x78, 0x48,
    0x88, 0x8f,

    /* U+04D5 "ӕ" */
    0xec, 0x24, 0x4b, 0xf9, 0x12, 0x1b, 0x80,

    /* U+04D6 "Ӗ" */
    0x48, 0xc0, 0x3f, 0x82, 0x8, 0x3c, 0x82, 0x8,
    0x3f,

    /* U+04D7 "ӗ" */
    0x48, 0xc0, 0x1e, 0x86, 0x1f, 0xe0, 0x81, 0xe0,

    /* U+04D8 "Ә" */
    0x71, 0x20, 0x41, 0x7, 0xf8, 0x61, 0x48, 0xc0,

    /* U+04D9 "ә" */
    0x70, 0x20, 0x7f, 0x85, 0x23, 0x0,

    /* U+04DA "Ӛ" */
    0x49, 0x20, 0x1c, 0x48, 0x10, 0x7f, 0x86, 0x14,
    0x8c,

    /* U+04DB "ӛ" */
    0x49, 0x20, 0x1c, 0x8, 0x1f, 0xe1, 0x48, 0xc0,

    /* U+04DC "Ӝ" */
    0x24, 0x48, 0x4, 0x99, 0x32, 0x55, 0x1c, 0x55,
    0x26, 0x4c, 0x90,

    /* U+04DD "ӝ" */
    0x24, 0x48, 0x4, 0x99, 0x2a, 0x9f, 0x49, 0x93,
    0x24,

    /* U+04DE "Ӟ" */
    0x49, 0x20, 0x1c, 0x88, 0x10, 0x8c, 0x8, 0x18,
    0x9c,

    /* U+04DF "ӟ" */
    0x49, 0x20, 0x1e, 0x84, 0x11, 0x81, 0x85, 0xe0,

    /* U+04E0 "Ӡ" */
    0xfc, 0x10, 0x84, 0x38, 0x10, 0x41, 0x85, 0xe0,

    /* U+04E1 "ӡ" */
    0xfc, 0x10, 0x84, 0x38, 0x10, 0x41, 0x85, 0xe0,

    /* U+04E2 "Ӣ" */
    0x78, 0x8, 0x61, 0x8e, 0x59, 0x69, 0xa7, 0x18,
    0x61,

    /* U+04E3 "ӣ" */
    0x78, 0x0, 0x21, 0x86, 0x39, 0x69, 0xc6, 0x10,

    /* U+04E4 "Ӥ" */
    0x49, 0x20, 0x21, 0x8e, 0x59, 0x69, 0xa7, 0x18,
    0x61,

    /* U+04E5 "ӥ" */
    0x49, 0x20, 0x21, 0x86, 0x39, 0x69, 0xc6, 0x10,

    /* U+04E6 "Ӧ" */
    0x49, 0x20, 0x1e, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x5e,

    /* U+04E7 "ӧ" */
    0x49, 0x20, 0x1e, 0x86, 0x18, 0x61, 0x85, 0xe0,

    /* U+04E8 "Ө" */
    0x7a, 0x18, 0x61, 0xfe, 0x18, 0x61, 0x85, 0xe0,

    /* U+04E9 "ө" */
    0x7a, 0x18, 0x7f, 0x86, 0x17, 0x80,

    /* U+04EA "Ӫ" */
    0x49, 0x20, 0x1e, 0x86, 0x18, 0x7f, 0x86, 0x18,
    0x5e,

    /* U+04EB "ӫ" */
    0x49, 0x20, 0x1e, 0x86, 0x1f, 0xe1, 0x85, 0xe0,

    /* U+04EC "Ӭ" */
    0x49, 0x20, 0x1c, 0x88, 0x10, 0x5f, 0x4, 0x18,
    0x9c,

    /* U+04ED "ӭ" */
    0x49, 0x20, 0x1c, 0x88, 0x13, 0xc1, 0x89, 0xc0,

    /* U+04EE "Ӯ" */
    0x78, 0x8, 0x61, 0x45, 0x22, 0x8a, 0x10, 0x4a,
    0x18,

    /* U+04EF "ӯ" */
    0x78, 0x0, 0x21, 0x85, 0x14, 0x8a, 0x28, 0x41,
    0x28, 0x40,

    /* U+04F0 "Ӱ" */
    0x49, 0x20, 0x21, 0x45, 0x22, 0x8a, 0x10, 0x4a,
    0x18,

    /* U+04F1 "ӱ" */
    0x49, 0x20, 0x21, 0x85, 0x14, 0x8a, 0x28, 0x41,
    0x28, 0x40,

    /* U+04F2 "Ӳ" */
    0x25, 0x20, 0x21, 0x45, 0x22, 0x8a, 0x10, 0x4a,
    0x18,

    /* U+04F3 "ӳ" */
    0x24, 0x94, 0x80, 0x86, 0x14, 0x52, 0x28, 0xa1,
    0x4, 0xa1, 0x0,

    /* U+04F4 "Ӵ" */
    0x49, 0x20, 0x21, 0x86, 0x18, 0x5f, 0x4, 0x10,
    0x41,

    /* U+04F5 "ӵ" */
    0x49, 0x20, 0x21, 0x86, 0x17, 0xc1, 0x4, 0x10,

    /* U+04F6 "Ӷ" */
    0xfe, 0x8, 0x20, 0x82, 0x8, 0x20, 0x83, 0x4,
    0x10, 0x40,

    /* U+04F7 "ӷ" */
    0xfe, 0x8, 0x20, 0x82, 0xc, 0x10, 0x41, 0x0,

    /* U+04F8 "Ӹ" */
    0x48, 0x90, 0x4, 0x18, 0x30, 0x7c, 0xc5, 0x8b,
    0x16, 0x2f, 0x90,

    /* U+04F9 "ӹ" */
    0x49, 0x20, 0x21, 0x86, 0x1e, 0x65, 0x97, 0x90,

    /* U+04FA "Ӻ" */
    0x7e, 0x81, 0x2, 0xf, 0x8, 0x10, 0x20, 0x40,
    0xe0, 0x20, 0x43, 0x0,

    /* U+04FB "ӻ" */
    0x7e, 0x81, 0x7, 0x84, 0x8, 0x1c, 0x4, 0x8,
    0x60,

    /* U+04FC "Ӽ" */
    0x86, 0x14, 0x92, 0x30, 0xc4, 0x92, 0x86, 0x10,
    0x42, 0x30,

    /* U+04FD "ӽ" */
    0x86, 0x14, 0x8c, 0x4a, 0x18, 0x41, 0x8, 0xc0,

    /* U+04FE "Ӿ" */
    0x86, 0x14, 0x8c, 0xfc, 0xc4, 0x92, 0x86, 0x10,

    /* U+04FF "ӿ" */
    0x85, 0x23, 0x3f, 0x31, 0x28, 0x40,

    /* U+20B4 "₴" */
    0x7a, 0x10, 0xbf, 0x13, 0xf4, 0x20, 0x85, 0xe0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 9, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 18, .adv_w = 128, .box_w = 8, .box_h = 13, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 31, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 40, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 48, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 56, .adv_w = 128, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 63, .adv_w = 128, .box_w = 5, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 71, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 79, .adv_w = 128, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 89, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 98, .adv_w = 128, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 108, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 117, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 126, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 135, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 145, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 153, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 161, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 169, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 177, .adv_w = 128, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 189, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 197, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 206, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 214, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 222, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 231, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 239, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 248, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 257, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 265, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 273, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 281, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 289, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 297, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 306, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 314, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 325, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 333, .adv_w = 128, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 345, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 353, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 362, .adv_w = 128, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 374, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 383, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 392, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 400, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 408, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 417, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 425, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 431, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 439, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 445, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 451, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 460, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 466, .adv_w = 128, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 473, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 479, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 485, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 493, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 499, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 505, .adv_w = 128, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 512, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 518, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 524, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 530, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 538, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 544, .adv_w = 128, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 551, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 559, .adv_w = 128, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 571, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 577, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 586, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 592, .adv_w = 128, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 599, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 608, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 614, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 620, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 625, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 631, .adv_w = 128, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 638, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 644, .adv_w = 128, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 653, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 661, .adv_w = 128, .box_w = 7, .box_h = 13, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 673, .adv_w = 128, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 682, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 688, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 694, .adv_w = 128, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 701, .adv_w = 128, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 708, .adv_w = 128, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 717, .adv_w = 128, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 724, .adv_w = 128, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 731, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 740, .adv_w = 128, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 749, .adv_w = 128, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 758, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 768, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 776, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 785, .adv_w = 128, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 792, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 801, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 810, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 818, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 824, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 833, .adv_w = 128, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 840, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 849, .adv_w = 128, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 856, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 865, .adv_w = 128, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 872, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 881, .adv_w = 128, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 888, .adv_w = 128, .box_w = 6, .box_h = 15, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 900, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 910, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 919, .adv_w = 128, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 931, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 939, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 945, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 954, .adv_w = 128, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 961, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 972, .adv_w = 128, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 982, .adv_w = 128, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 994, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1003, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1014, .adv_w = 128, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1022, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1033, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1044, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1055, .adv_w = 128, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1065, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1075, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1083, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1091, .adv_w = 128, .box_w = 4, .box_h = 3, .ofs_x = 2, .ofs_y = 8},
    {.bitmap_index = 1093, .adv_w = 128, .box_w = 4, .box_h = 2, .ofs_x = 4, .ofs_y = 8},
    {.bitmap_index = 1094, .adv_w = 128, .box_w = 2, .box_h = 3, .ofs_x = 3, .ofs_y = 8},
    {.bitmap_index = 1095, .adv_w = 128, .box_w = 2, .box_h = 3, .ofs_x = 3, .ofs_y = 8},
    {.bitmap_index = 1096, .adv_w = 128, .box_w = 7, .box_h = 4, .ofs_x = 1, .ofs_y = 8},
    {.bitmap_index = 1100, .adv_w = 128, .box_w = 7, .box_h = 15, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1114, .adv_w = 128, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1126, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1135, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1141, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1149, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1157, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 1166, .adv_w = 128, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1173, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1182, .adv_w = 128, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1189, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1199, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1207, .adv_w = 128, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1219, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1228, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1238, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1246, .adv_w = 128, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1258, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1266, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1274, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1280, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1289, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1298, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1307, .adv_w = 128, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1314, .adv_w = 128, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1326, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1335, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1344, .adv_w = 128, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1351, .adv_w = 128, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1363, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1372, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1381, .adv_w = 128, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1388, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1398, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1406, .adv_w = 128, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1418, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1427, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1436, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1445, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1454, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1463, .adv_w = 128, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1475, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1484, .adv_w = 128, .box_w = 8, .box_h = 13, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 1497, .adv_w = 128, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 1507, .adv_w = 128, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1519, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1528, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1536, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1542, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1550, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1558, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1567, .adv_w = 128, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1574, .adv_w = 128, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1586, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1595, .adv_w = 128, .box_w = 3, .box_h = 10, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 1599, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1610, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1619, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1629, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1637, .adv_w = 128, .box_w = 8, .box_h = 13, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 1650, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1659, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1669, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1677, .adv_w = 128, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1689, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1698, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1708, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1716, .adv_w = 128, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1728, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1737, .adv_w = 128, .box_w = 3, .box_h = 10, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 1741, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1750, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1758, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1767, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1775, .adv_w = 128, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1785, .adv_w = 128, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1792, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1801, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1809, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1817, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1823, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1832, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1840, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1851, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1860, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1869, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1877, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1885, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1893, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1902, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1910, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1919, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1927, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1936, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1944, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1952, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1958, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1967, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1975, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1984, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1992, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2001, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2011, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2020, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2030, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2039, .adv_w = 128, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2050, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2059, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2067, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 2077, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2085, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2096, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2104, .adv_w = 128, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2116, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2125, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2135, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2143, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2151, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2157, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0}
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
const lv_font_t greybeard_cyrillic_16 = {
#else
lv_font_t greybeard_cyrillic_16 = {
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
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if GREYBEARD_CYRILLIC_16*/

