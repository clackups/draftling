/*******************************************************************************
 * Size: 14 px
 * Bpp: 1
 * Opts: --font Greybeard-14px.ttf -r 0x400-0x4FF,0x20B4 --size 14 --bpp 1 --format lvgl --no-compress --lv-font-name greybeard_cyrillic_14 -o /tmp/gen/greybeard_cyrillic_14.c
 ******************************************************************************/

#include "lvgl.h"

#ifndef GREYBEARD_CYRILLIC_14
#define GREYBEARD_CYRILLIC_14 1
#endif

#if GREYBEARD_CYRILLIC_14

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0400 "Ѐ" */
    0x41, 0x81, 0xf8, 0x43, 0xd0, 0x84, 0x3e,

    /* U+0401 "Ё" */
    0x4a, 0x41, 0xf8, 0x43, 0xd0, 0x84, 0x3e,

    /* U+0402 "Ђ" */
    0xf8, 0x82, 0xe, 0x24, 0x92, 0x49, 0x24, 0x11,
    0x80,

    /* U+0403 "Ѓ" */
    0x13, 0x1, 0xf8, 0x42, 0x10, 0x84, 0x20,

    /* U+0404 "Є" */
    0x32, 0x61, 0xf, 0x42, 0x9, 0x30,

    /* U+0405 "Ѕ" */
    0x74, 0x61, 0x7, 0x4, 0x31, 0x70,

    /* U+0406 "І" */
    0xf9, 0x8, 0x42, 0x10, 0x84, 0xf8,

    /* U+0407 "Ї" */
    0x4a, 0x41, 0xf2, 0x10, 0x84, 0x21, 0x3e,

    /* U+0408 "Ј" */
    0x3c, 0x20, 0x82, 0x8, 0x28, 0xa2, 0x70,

    /* U+0409 "Љ" */
    0x30, 0xe1, 0x42, 0x85, 0xca, 0x54, 0xe9, 0x9c,

    /* U+040A "Њ" */
    0x91, 0x22, 0x44, 0x8f, 0xd2, 0x64, 0xc9, 0x9c,

    /* U+040B "Ћ" */
    0xf8, 0x82, 0xe, 0x24, 0x92, 0x49, 0x24,

    /* U+040C "Ќ" */
    0x13, 0x1, 0x18, 0xca, 0x9c, 0x94, 0x62,

    /* U+040D "Ѝ" */
    0x41, 0x81, 0x18, 0xce, 0xb5, 0xcc, 0x62,

    /* U+040E "Ў" */
    0x28, 0xe0, 0x21, 0x45, 0x22, 0x8c, 0x12, 0x84,
    0x0,

    /* U+040F "Џ" */
    0x8c, 0x63, 0x18, 0xc6, 0x31, 0xf9, 0x8,

    /* U+0410 "А" */
    0x30, 0xc3, 0x12, 0x49, 0xe8, 0x61, 0x84,

    /* U+0411 "Б" */
    0xfc, 0x21, 0xf, 0x46, 0x31, 0xf0,

    /* U+0412 "В" */
    0xf4, 0x63, 0x1f, 0x46, 0x31, 0xf0,

    /* U+0413 "Г" */
    0xfc, 0x21, 0x8, 0x42, 0x10, 0x80,

    /* U+0414 "Д" */
    0x79, 0x24, 0x92, 0x49, 0x24, 0x92, 0xfe, 0x18,
    0x40,

    /* U+0415 "Е" */
    0xfc, 0x21, 0xf, 0x42, 0x10, 0xf8,

    /* U+0416 "Ж" */
    0x93, 0x25, 0x52, 0xa3, 0x8a, 0x95, 0x49, 0x92,

    /* U+0417 "З" */
    0x74, 0x42, 0x13, 0x4, 0x31, 0x70,

    /* U+0418 "И" */
    0x8c, 0x63, 0x3a, 0xd7, 0x31, 0x88,

    /* U+0419 "Й" */
    0x53, 0x81, 0x18, 0xce, 0xb5, 0xcc, 0x62,

    /* U+041A "К" */
    0x8c, 0x65, 0x4e, 0x4a, 0x51, 0x88,

    /* U+041B "Л" */
    0xc, 0x52, 0x49, 0x24, 0x92, 0x49, 0xc4,

    /* U+041C "М" */
    0x86, 0x1c, 0xf3, 0xb6, 0xd8, 0x61, 0x84,

    /* U+041D "Н" */
    0x8c, 0x63, 0x1f, 0xc6, 0x31, 0x88,

    /* U+041E "О" */
    0x74, 0x63, 0x18, 0xc6, 0x31, 0x70,

    /* U+041F "П" */
    0xfc, 0x63, 0x18, 0xc6, 0x31, 0x88,

    /* U+0420 "Р" */
    0xf4, 0x63, 0x1f, 0x42, 0x10, 0x80,

    /* U+0421 "С" */
    0x32, 0x61, 0x8, 0x42, 0x9, 0x30,

    /* U+0422 "Т" */
    0xf9, 0x8, 0x42, 0x10, 0x84, 0x20,

    /* U+0423 "У" */
    0x85, 0x14, 0x8a, 0x30, 0x42, 0x28, 0x40,

    /* U+0424 "Ф" */
    0x23, 0xab, 0x5a, 0xd6, 0xae, 0x20,

    /* U+0425 "Х" */
    0x8c, 0x54, 0xa2, 0x29, 0x51, 0x88,

    /* U+0426 "Ц" */
    0x8a, 0x28, 0xa2, 0x8a, 0x28, 0xa2, 0xfc, 0x10,
    0x40,

    /* U+0427 "Ч" */
    0x8c, 0x63, 0x17, 0x84, 0x21, 0x8,

    /* U+0428 "Ш" */
    0x93, 0x26, 0x4c, 0x99, 0x32, 0x64, 0xc9, 0xfe,

    /* U+0429 "Щ" */
    0x93, 0x26, 0x4c, 0x99, 0x32, 0x64, 0xc9, 0xfe,
    0x4, 0x8,

    /* U+042A "Ъ" */
    0xe0, 0x82, 0x8, 0x38, 0x92, 0x49, 0x38,

    /* U+042B "Ы" */
    0x8c, 0x63, 0x1c, 0xd6, 0xb5, 0xe8,

    /* U+042C "Ь" */
    0x84, 0x21, 0xf, 0x46, 0x31, 0xf0,

    /* U+042D "Э" */
    0x64, 0x82, 0x17, 0x84, 0x32, 0x60,

    /* U+042E "Ю" */
    0x9a, 0x9a, 0x69, 0xe6, 0x9a, 0x69, 0x98,

    /* U+042F "Я" */
    0x7c, 0x63, 0x17, 0x95, 0x29, 0x88,

    /* U+0430 "а" */
    0x70, 0x42, 0xf8, 0xc5, 0xe0,

    /* U+0431 "б" */
    0x9, 0x91, 0xb, 0x66, 0x31, 0x8b, 0x80,

    /* U+0432 "в" */
    0xf4, 0x63, 0xe8, 0xc7, 0xc0,

    /* U+0433 "г" */
    0xfc, 0x21, 0x8, 0x42, 0x0,

    /* U+0434 "д" */
    0x79, 0x24, 0x92, 0x49, 0x2f, 0xe1, 0x84,

    /* U+0435 "е" */
    0x74, 0x63, 0xf8, 0x41, 0xc0,

    /* U+0436 "ж" */
    0x92, 0xa9, 0x51, 0xc5, 0x52, 0x64, 0x80,

    /* U+0437 "з" */
    0x74, 0x42, 0x60, 0xc5, 0xc0,

    /* U+0438 "и" */
    0x8c, 0x67, 0x5a, 0xe6, 0x20,

    /* U+0439 "й" */
    0x53, 0x81, 0x18, 0xce, 0xb5, 0xcc, 0x40,

    /* U+043A "к" */
    0x8c, 0xa9, 0x8a, 0x4a, 0x20,

    /* U+043B "л" */
    0xc, 0x52, 0x49, 0x24, 0x9c, 0x40,

    /* U+043C "м" */
    0x8c, 0x77, 0xba, 0xd6, 0xa0,

    /* U+043D "н" */
    0x8c, 0x63, 0xf8, 0xc6, 0x20,

    /* U+043E "о" */
    0x74, 0x63, 0x18, 0xc5, 0xc0,

    /* U+043F "п" */
    0xfc, 0x63, 0x18, 0xc6, 0x20,

    /* U+0440 "р" */
    0xb6, 0x63, 0x18, 0xe6, 0xd0, 0x80,

    /* U+0441 "с" */
    0x74, 0x61, 0x8, 0x45, 0xc0,

    /* U+0442 "т" */
    0xf9, 0x8, 0x42, 0x10, 0x80,

    /* U+0443 "у" */
    0x8c, 0x62, 0xa5, 0x18, 0x84, 0xc0,

    /* U+0444 "ф" */
    0x21, 0x8, 0xea, 0xd6, 0xb5, 0xab, 0x88, 0x40,

    /* U+0445 "х" */
    0x8c, 0x54, 0x45, 0x46, 0x20,

    /* U+0446 "ц" */
    0x8a, 0x28, 0xa2, 0x8a, 0x2f, 0xc1, 0x4,

    /* U+0447 "ч" */
    0x8c, 0x62, 0xf0, 0x84, 0x20,

    /* U+0448 "ш" */
    0x93, 0x26, 0x4c, 0x99, 0x32, 0x7f, 0x80,

    /* U+0449 "щ" */
    0x93, 0x26, 0x4c, 0x99, 0x32, 0x7f, 0x81, 0x2,

    /* U+044A "ъ" */
    0xe2, 0x82, 0xe, 0x24, 0x93, 0x80,

    /* U+044B "ы" */
    0x8c, 0x63, 0x9a, 0xd7, 0xa0,

    /* U+044C "ь" */
    0x84, 0x21, 0xe8, 0xc7, 0xc0,

    /* U+044D "э" */
    0x64, 0x82, 0xf0, 0xc9, 0x80,

    /* U+044E "ю" */
    0x9a, 0x9a, 0x79, 0xa6, 0x99, 0x80,

    /* U+044F "я" */
    0x7c, 0x62, 0xf2, 0xa6, 0x20,

    /* U+0450 "ѐ" */
    0x42, 0x8, 0x7, 0x46, 0x3f, 0x84, 0x1c,

    /* U+0451 "ё" */
    0x4a, 0x40, 0xe8, 0xc7, 0xf0, 0x83, 0x80,

    /* U+0452 "ђ" */
    0x43, 0xe4, 0x14, 0x69, 0x14, 0x51, 0x45, 0x10,
    0x8c,

    /* U+0453 "ѓ" */
    0x10, 0x88, 0xf, 0xc2, 0x10, 0x84, 0x20,

    /* U+0454 "є" */
    0x32, 0x61, 0xe8, 0x24, 0xc0,

    /* U+0455 "ѕ" */
    0x74, 0x60, 0xe0, 0xc5, 0xc0,

    /* U+0456 "і" */
    0x21, 0x0, 0xc2, 0x10, 0x84, 0x27, 0xc0,

    /* U+0457 "ї" */
    0x94, 0x80, 0xc2, 0x10, 0x84, 0x27, 0xc0,

    /* U+0458 "ј" */
    0x11, 0x7, 0x11, 0x11, 0x11, 0x96,

    /* U+0459 "љ" */
    0x30, 0xa1, 0x42, 0xe5, 0x2a, 0x67, 0x0,

    /* U+045A "њ" */
    0x91, 0x22, 0x47, 0xe9, 0x32, 0x67, 0x0,

    /* U+045B "ћ" */
    0x43, 0xe4, 0x16, 0x65, 0x14, 0x51, 0x45, 0x10,

    /* U+045C "ќ" */
    0x10, 0x88, 0x8, 0xca, 0x98, 0xa4, 0xa2,

    /* U+045D "ѝ" */
    0x42, 0x8, 0x8, 0xc6, 0x75, 0xae, 0x62,

    /* U+045E "ў" */
    0x53, 0x81, 0x18, 0xc5, 0x4a, 0x31, 0x9, 0x80,

    /* U+045F "џ" */
    0x8c, 0x63, 0x18, 0xc7, 0xe4, 0x20,

    /* U+0460 "Ѡ" */
    0x8d, 0x6b, 0x5a, 0xd6, 0xb5, 0x50,

    /* U+0461 "ѡ" */
    0x8d, 0x6b, 0x5a, 0xd5, 0x40,

    /* U+0462 "Ѣ" */
    0x20, 0x8f, 0x88, 0x38, 0x92, 0x49, 0x38,

    /* U+0463 "ѣ" */
    0x20, 0x82, 0x3e, 0x20, 0x83, 0x89, 0x24, 0xe0,

    /* U+0464 "Ѥ" */
    0x9a, 0x9a, 0x28, 0xfa, 0x8a, 0x29, 0x98,

    /* U+0465 "ѥ" */
    0x9a, 0x9a, 0x3e, 0xa2, 0x99, 0x80,

    /* U+0466 "Ѧ" */
    0x10, 0x20, 0x41, 0x42, 0x8f, 0x95, 0x49, 0x92,

    /* U+0467 "ѧ" */
    0x21, 0x14, 0xea, 0xd6, 0xa0,

    /* U+0468 "Ѩ" */
    0x89, 0x12, 0x24, 0xaf, 0x53, 0xaa, 0xd5, 0xaa,

    /* U+0469 "ѩ" */
    0x89, 0x12, 0x57, 0xea, 0xb5, 0x6a, 0x80,

    /* U+046A "Ѫ" */
    0xfe, 0x88, 0xa0, 0x83, 0x8a, 0x95, 0x49, 0x92,

    /* U+046B "ѫ" */
    0xfc, 0x54, 0x47, 0x56, 0xa0,

    /* U+046C "Ѭ" */
    0xbf, 0x46, 0x54, 0x4f, 0x93, 0xaa, 0xd5, 0xaa,

    /* U+046D "ѭ" */
    0xbf, 0x46, 0x57, 0xc9, 0xd5, 0x6a, 0x80,

    /* U+046E "Ѯ" */
    0x49, 0x80, 0xe8, 0x84, 0xc1, 0x8, 0x5d, 0x7,
    0x0,

    /* U+046F "ѯ" */
    0x49, 0x80, 0xe8, 0x84, 0xc1, 0xb, 0xa0, 0xe0,

    /* U+0470 "Ѱ" */
    0xad, 0x6b, 0x5a, 0xb8, 0x84, 0x20,

    /* U+0471 "ѱ" */
    0x21, 0x9, 0x5a, 0xd6, 0xb5, 0xab, 0x88, 0x40,

    /* U+0472 "Ѳ" */
    0x74, 0x63, 0x1f, 0xc6, 0x31, 0x70,

    /* U+0473 "ѳ" */
    0x74, 0x63, 0xf8, 0xc5, 0xc0,

    /* U+0474 "Ѵ" */
    0x86, 0x28, 0x94, 0x51, 0x42, 0x8, 0x20,

    /* U+0475 "ѵ" */
    0x86, 0x24, 0x94, 0x50, 0x82, 0x0,

    /* U+0476 "Ѷ" */
    0x91, 0x20, 0x21, 0x89, 0x25, 0x14, 0x20, 0x82,
    0x0,

    /* U+0477 "ѷ" */
    0x92, 0x44, 0x80, 0x86, 0x24, 0x94, 0x50, 0x82,
    0x0,

    /* U+0478 "Ѹ" */
    0x41, 0x42, 0xad, 0x5a, 0xb5, 0x6b, 0x52, 0x44,
    0x10, 0x20,

    /* U+0479 "ѹ" */
    0x4b, 0x56, 0xad, 0x5a, 0xd4, 0x91, 0x4, 0x8,

    /* U+047A "Ѻ" */
    0x23, 0xab, 0x18, 0xc6, 0x31, 0xab, 0x88,

    /* U+047B "ѻ" */
    0x23, 0xab, 0x18, 0xc6, 0xae, 0x20,

    /* U+047C "Ѽ" */
    0x71, 0x1e, 0xc2, 0xa8, 0x30, 0x6e, 0xc9, 0x93,
    0x25, 0xb0,

    /* U+047D "ѽ" */
    0x71, 0x12, 0xd8, 0x84, 0x50, 0x60, 0xdd, 0x93,
    0x25, 0xb0,

    /* U+047E "Ѿ" */
    0xfd, 0x41, 0x1a, 0xd6, 0xb5, 0xad, 0x54,

    /* U+047F "ѿ" */
    0xfd, 0x41, 0x1a, 0xd6, 0xb5, 0xaa, 0x80,

    /* U+0480 "Ҁ" */
    0x32, 0x61, 0x8, 0x42, 0x8, 0x30, 0x84,

    /* U+0481 "ҁ" */
    0x74, 0x61, 0x8, 0x41, 0xc2, 0x10,

    /* U+0482 "҂" */
    0x4, 0x38, 0x30, 0x51, 0x14, 0x18, 0x38, 0x40,

    /* U+0483 "҃" */
    0xf, 0xe0,

    /* U+0484 "҄" */
    0x69,

    /* U+0485 "҅" */
    0x64,

    /* U+0486 "҆" */
    0x98,

    /* U+0487 "҇" */
    0x71, 0x12, 0x18,

    /* U+048A "Ҋ" */
    0x51, 0xc0, 0x22, 0x8a, 0x6a, 0xaa, 0xca, 0x29,
    0xc2, 0x10,

    /* U+048B "ҋ" */
    0x51, 0xc0, 0x22, 0x8a, 0x6a, 0xaa, 0xca, 0x70,
    0x84,

    /* U+048C "Ҍ" */
    0x41, 0xe, 0x10, 0x79, 0x14, 0x51, 0x78,

    /* U+048D "ҍ" */
    0x43, 0x84, 0x1e, 0x45, 0x17, 0x80,

    /* U+048E "Ҏ" */
    0xf4, 0x6b, 0x2f, 0x46, 0x10, 0x80,

    /* U+048F "ҏ" */
    0xb6, 0x63, 0x1a, 0xcb, 0xd1, 0x80,

    /* U+0490 "Ґ" */
    0x8, 0x7f, 0x8, 0x42, 0x10, 0x84, 0x20,

    /* U+0491 "ґ" */
    0x8, 0x7f, 0x8, 0x42, 0x10, 0x80,

    /* U+0492 "Ғ" */
    0x7d, 0x4, 0x10, 0xf1, 0x4, 0x10, 0x40,

    /* U+0493 "ғ" */
    0x7d, 0x4, 0x3c, 0x41, 0x4, 0x0,

    /* U+0494 "Ҕ" */
    0xfc, 0x21, 0xe, 0x4a, 0x31, 0x88, 0x98,

    /* U+0495 "ҕ" */
    0xfc, 0x21, 0xc9, 0x46, 0x22, 0x60,

    /* U+0496 "Җ" */
    0x93, 0x25, 0x52, 0xa3, 0x8a, 0x95, 0x49, 0x92,
    0x4, 0x8,

    /* U+0497 "җ" */
    0x92, 0xa9, 0x51, 0xc5, 0x52, 0x64, 0x81, 0x2,

    /* U+0498 "Ҙ" */
    0x74, 0x42, 0x13, 0x4, 0x31, 0x71, 0x10,

    /* U+0499 "ҙ" */
    0x74, 0x42, 0x60, 0xc5, 0xc4, 0x40,

    /* U+049A "Қ" */
    0x8a, 0x29, 0x28, 0xe2, 0x49, 0x22, 0x8c, 0x10,
    0x40,

    /* U+049B "қ" */
    0x8a, 0x4a, 0x30, 0xa2, 0x48, 0xc1, 0x4,

    /* U+049C "Ҝ" */
    0x8c, 0x6b, 0x6f, 0x56, 0xb1, 0x88,

    /* U+049D "ҝ" */
    0x8d, 0x6d, 0xea, 0xd6, 0x20,

    /* U+049E "Ҟ" */
    0x47, 0x94, 0x94, 0x71, 0x24, 0x91, 0x44,

    /* U+049F "ҟ" */
    0x43, 0xc4, 0x11, 0x49, 0x46, 0x14, 0x49, 0x10,

    /* U+04A0 "Ҡ" */
    0xe4, 0x92, 0x4a, 0x30, 0xa2, 0x49, 0x24,

    /* U+04A1 "ҡ" */
    0xe4, 0x92, 0x8c, 0x28, 0x92, 0x40,

    /* U+04A2 "Ң" */
    0x8a, 0x28, 0xa2, 0xfa, 0x28, 0xa2, 0x8c, 0x10,
    0x40,

    /* U+04A3 "ң" */
    0x8a, 0x28, 0xbe, 0x8a, 0x28, 0xc1, 0x4,

    /* U+04A4 "Ҥ" */
    0x9e, 0x49, 0x24, 0xf2, 0x49, 0x24, 0x90,

    /* U+04A5 "ҥ" */
    0x9e, 0x49, 0x3c, 0x92, 0x49, 0x0,

    /* U+04A6 "Ҧ" */
    0xf2, 0x49, 0x24, 0x9a, 0x59, 0x65, 0x94, 0x11,
    0x80,

    /* U+04A7 "ҧ" */
    0xf2, 0x49, 0x26, 0x96, 0x59, 0x41, 0x18,

    /* U+04A8 "Ҩ" */
    0x74, 0x65, 0x5a, 0xd6, 0xb2, 0x68,

    /* U+04A9 "ҩ" */
    0x74, 0x65, 0x5a, 0xc9, 0xa0,

    /* U+04AA "Ҫ" */
    0x32, 0x61, 0x8, 0x42, 0x9, 0x30, 0x88,

    /* U+04AB "ҫ" */
    0x74, 0x61, 0x8, 0x45, 0xc4, 0x40,

    /* U+04AC "Ҭ" */
    0xf9, 0x8, 0x42, 0x10, 0x84, 0x30, 0x84,

    /* U+04AD "ҭ" */
    0xf9, 0x8, 0x42, 0x10, 0xc2, 0x10,

    /* U+04AE "Ү" */
    0x8c, 0x62, 0xa5, 0x10, 0x84, 0x20,

    /* U+04AF "ү" */
    0x8c, 0x62, 0xa5, 0x10, 0x84, 0x20,

    /* U+04B0 "Ұ" */
    0x8c, 0x62, 0xa5, 0x13, 0xe4, 0x20,

    /* U+04B1 "ұ" */
    0x8c, 0x62, 0xa5, 0x13, 0xe4, 0x20,

    /* U+04B2 "Ҳ" */
    0x8a, 0x25, 0x14, 0x21, 0x45, 0x22, 0x8c, 0x10,
    0x40,

    /* U+04B3 "ҳ" */
    0x8a, 0x25, 0x8, 0x52, 0x28, 0xc1, 0x4,

    /* U+04B4 "Ҵ" */
    0xe4, 0x89, 0x12, 0x24, 0x48, 0x91, 0x22, 0x7e,
    0x4, 0x8,

    /* U+04B5 "ҵ" */
    0xe4, 0x89, 0x12, 0x24, 0x48, 0x9f, 0x81, 0x2,

    /* U+04B6 "Ҷ" */
    0x8a, 0x28, 0xa2, 0x78, 0x20, 0x82, 0xc, 0x10,
    0x40,

    /* U+04B7 "ҷ" */
    0x8a, 0x28, 0x9e, 0x8, 0x20, 0xc1, 0x4,

    /* U+04B8 "Ҹ" */
    0x8c, 0x6b, 0x57, 0x94, 0xa1, 0x8,

    /* U+04B9 "ҹ" */
    0x8d, 0x6a, 0xf2, 0x94, 0x20,

    /* U+04BA "Һ" */
    0x84, 0x21, 0xf, 0x46, 0x31, 0x88,

    /* U+04BB "һ" */
    0x84, 0x21, 0x6c, 0xc6, 0x31, 0x8c, 0x40,

    /* U+04BC "Ҽ" */
    0x5a, 0x9c, 0x5f, 0x41, 0x4, 0x9, 0x18,

    /* U+04BD "ҽ" */
    0xda, 0x97, 0xd0, 0x40, 0x91, 0x80,

    /* U+04BE "Ҿ" */
    0x5a, 0x9c, 0x5f, 0x41, 0x4, 0x9, 0x18, 0x20,
    0x80,

    /* U+04BF "ҿ" */
    0xda, 0x97, 0xd0, 0x40, 0x91, 0x84, 0x10,

    /* U+04C0 "Ӏ" */
    0xe9, 0x24, 0x92, 0xe0,

    /* U+04C1 "Ӂ" */
    0x28, 0x72, 0xc, 0x95, 0x4a, 0x8e, 0x2a, 0x55,
    0x26, 0x48,

    /* U+04C2 "ӂ" */
    0x28, 0x70, 0x4, 0x95, 0x4a, 0x8e, 0x2a, 0x93,
    0x24,

    /* U+04C3 "Ӄ" */
    0x8c, 0x65, 0x4e, 0x4a, 0x31, 0x88, 0x98,

    /* U+04C4 "ӄ" */
    0x8c, 0xa9, 0xc9, 0x46, 0x22, 0x60,

    /* U+04C5 "Ӆ" */
    0xc, 0x28, 0x91, 0x22, 0x44, 0x89, 0x12, 0xce,
    0x8, 0x20,

    /* U+04C6 "ӆ" */
    0xc, 0x28, 0x91, 0x22, 0x44, 0xb3, 0x82, 0x8,

    /* U+04C7 "Ӈ" */
    0x8c, 0x63, 0x1f, 0xc6, 0x31, 0x88, 0x5c,

    /* U+04C8 "ӈ" */
    0x8c, 0x63, 0xf8, 0xc6, 0x21, 0x70,

    /* U+04C9 "Ӊ" */
    0x8a, 0x28, 0xa2, 0xfa, 0x28, 0xa2, 0x9c, 0x21,
    0x0,

    /* U+04CA "ӊ" */
    0x8a, 0x28, 0xbe, 0x8a, 0x29, 0xc2, 0x10,

    /* U+04CB "Ӌ" */
    0x8c, 0x63, 0x17, 0x84, 0x21, 0x18, 0x84,

    /* U+04CC "ӌ" */
    0x8c, 0x62, 0xf0, 0x84, 0x62, 0x10,

    /* U+04CD "Ӎ" */
    0x85, 0xb, 0x36, 0x6b, 0x56, 0xa1, 0x42, 0x8e,
    0x8, 0x20,

    /* U+04CE "ӎ" */
    0x8a, 0x2d, 0xb6, 0xaa, 0x29, 0xc2, 0x10,

    /* U+04CF "ӏ" */
    0xe9, 0x24, 0x92, 0x5c,

    /* U+04D0 "Ӑ" */
    0x48, 0xc0, 0xc, 0x31, 0x24, 0x9e, 0x86, 0x18,
    0x40,

    /* U+04D1 "ӑ" */
    0x49, 0x80, 0xe0, 0x85, 0xf1, 0x8b, 0xc0,

    /* U+04D2 "Ӓ" */
    0x49, 0x20, 0xc, 0x31, 0x24, 0x9e, 0x86, 0x18,
    0x40,

    /* U+04D3 "ӓ" */
    0x4a, 0x40, 0xe0, 0x85, 0xf1, 0x8b, 0xc0,

    /* U+04D4 "Ӕ" */
    0x1c, 0xc3, 0x14, 0x5d, 0xc9, 0x24, 0x9c,

    /* U+04D5 "ӕ" */
    0x6c, 0x24, 0x4b, 0xf9, 0x12, 0x1b, 0x80,

    /* U+04D6 "Ӗ" */
    0x49, 0x81, 0xf8, 0x43, 0xd0, 0x84, 0x3e,

    /* U+04D7 "ӗ" */
    0x49, 0x80, 0xe8, 0xc7, 0xf0, 0x83, 0x80,

    /* U+04D8 "Ә" */
    0x74, 0x42, 0x1f, 0xc6, 0x31, 0x70,

    /* U+04D9 "ә" */
    0xe0, 0x83, 0xf8, 0xc9, 0x80,

    /* U+04DA "Ӛ" */
    0x4a, 0x40, 0xe8, 0x87, 0xf1, 0x8c, 0x5c,

    /* U+04DB "ӛ" */
    0x94, 0x81, 0xc1, 0x7, 0xf1, 0x93, 0x0,

    /* U+04DC "Ӝ" */
    0x24, 0x4a, 0xc, 0x95, 0x4a, 0x8e, 0x2a, 0x55,
    0x26, 0x48,

    /* U+04DD "ӝ" */
    0x24, 0x48, 0x4, 0x95, 0x4a, 0x8e, 0x2a, 0x93,
    0x24,

    /* U+04DE "Ӟ" */
    0x4a, 0x40, 0xe8, 0x84, 0xc1, 0xc, 0x5c,

    /* U+04DF "ӟ" */
    0x4a, 0x40, 0xe8, 0x84, 0xc1, 0x8b, 0x80,

    /* U+04E0 "Ӡ" */
    0xf8, 0x44, 0x23, 0x4, 0x31, 0x70,

    /* U+04E1 "ӡ" */
    0xf8, 0x44, 0x23, 0x4, 0x31, 0x70,

    /* U+04E2 "Ӣ" */
    0x70, 0x23, 0x18, 0xce, 0xb5, 0xcc, 0x62,

    /* U+04E3 "ӣ" */
    0x70, 0x23, 0x19, 0xd6, 0xb9, 0x88,

    /* U+04E4 "Ӥ" */
    0x4a, 0x41, 0x18, 0xce, 0xb5, 0xcc, 0x62,

    /* U+04E5 "ӥ" */
    0x4a, 0x41, 0x18, 0xce, 0xb5, 0xcc, 0x40,

    /* U+04E6 "Ӧ" */
    0x4a, 0x40, 0xe8, 0xc6, 0x31, 0x8c, 0x5c,

    /* U+04E7 "ӧ" */
    0x4a, 0x40, 0xe8, 0xc6, 0x31, 0x8b, 0x80,

    /* U+04E8 "Ө" */
    0x74, 0x63, 0x1f, 0xc6, 0x31, 0x70,

    /* U+04E9 "ө" */
    0x74, 0x63, 0xf8, 0xc5, 0xc0,

    /* U+04EA "Ӫ" */
    0x4a, 0x40, 0xe8, 0xc7, 0xf1, 0x8c, 0x5c,

    /* U+04EB "ӫ" */
    0x4a, 0x40, 0xe8, 0xc7, 0xf1, 0x8b, 0x80,

    /* U+04EC "Ӭ" */
    0x94, 0x80, 0xc9, 0x5, 0xe1, 0xc, 0x98,

    /* U+04ED "ӭ" */
    0x94, 0x80, 0xc9, 0x5, 0xe1, 0x93, 0x0,

    /* U+04EE "Ӯ" */
    0x78, 0x8, 0x51, 0x48, 0xa3, 0x4, 0x22, 0x84,
    0x0,

    /* U+04EF "ӯ" */
    0x70, 0x23, 0x18, 0xa9, 0x46, 0x21, 0x30,

    /* U+04F0 "Ӱ" */
    0x49, 0x20, 0x21, 0x45, 0x22, 0x8c, 0x12, 0x84,
    0x0,

    /* U+04F1 "ӱ" */
    0x4a, 0x41, 0x18, 0xc5, 0x4a, 0x31, 0x9, 0x80,

    /* U+04F2 "Ӳ" */
    0x25, 0x20, 0x21, 0x45, 0x22, 0x8c, 0x12, 0x84,
    0x0,

    /* U+04F3 "ӳ" */
    0x4a, 0x64, 0x8, 0xc6, 0x2a, 0x51, 0x88, 0x4c,
    0x0,

    /* U+04F4 "Ӵ" */
    0x4a, 0x41, 0x18, 0xc5, 0xe1, 0x8, 0x42,

    /* U+04F5 "ӵ" */
    0x4a, 0x41, 0x18, 0xc5, 0xe1, 0x8, 0x40,

    /* U+04F6 "Ӷ" */
    0xfc, 0x21, 0x8, 0x42, 0x10, 0xc2, 0x10,

    /* U+04F7 "ӷ" */
    0xfc, 0x21, 0x8, 0x43, 0x8, 0x40,

    /* U+04F8 "Ӹ" */
    0x4a, 0x41, 0x18, 0xc7, 0x35, 0xad, 0x7a,

    /* U+04F9 "ӹ" */
    0x4a, 0x41, 0x18, 0xc7, 0x35, 0xaf, 0x40,

    /* U+04FA "Ӻ" */
    0x7d, 0x4, 0x10, 0xf1, 0x4, 0x10, 0x60, 0x46,
    0x0,

    /* U+04FB "ӻ" */
    0x7d, 0x4, 0x3c, 0x41, 0x6, 0x4, 0x60,

    /* U+04FC "Ӽ" */
    0x8c, 0x54, 0xa2, 0x29, 0x51, 0x88, 0x4c,

    /* U+04FD "ӽ" */
    0x8c, 0x54, 0x45, 0x46, 0x21, 0x30,

    /* U+04FE "Ӿ" */
    0x8c, 0x54, 0x4f, 0x91, 0x51, 0x88,

    /* U+04FF "ӿ" */
    0x8a, 0x89, 0xf2, 0x2a, 0x20,

    /* U+20B4 "₴" */
    0x64, 0x85, 0xf2, 0x7d, 0x9, 0x30
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 7, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 14, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 23, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 30, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 36, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 42, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 48, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 55, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 62, .adv_w = 112, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 70, .adv_w = 112, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 78, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 85, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 92, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 99, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 108, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 115, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 122, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 128, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 134, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 140, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 149, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 155, .adv_w = 112, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 163, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 169, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 175, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 182, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 188, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 195, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 202, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 208, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 214, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 220, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 226, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 232, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 238, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 245, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 251, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 257, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 266, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 272, .adv_w = 112, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 280, .adv_w = 112, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 290, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 297, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 303, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 309, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 315, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 322, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 328, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 333, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 340, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 345, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 350, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 357, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 362, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 369, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 374, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 379, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 386, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 391, .adv_w = 112, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 397, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 402, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 407, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 412, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 417, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 423, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 428, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 433, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 439, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 447, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 452, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 459, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 464, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 471, .adv_w = 112, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 479, .adv_w = 112, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 485, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 490, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 495, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 500, .adv_w = 112, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 506, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 511, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 518, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 525, .adv_w = 112, .box_w = 6, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 534, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 541, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 546, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 551, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 558, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 565, .adv_w = 112, .box_w = 4, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 571, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 578, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 585, .adv_w = 112, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 593, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 600, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 607, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 615, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 621, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 627, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 632, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 639, .adv_w = 112, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 647, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 654, .adv_w = 112, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 660, .adv_w = 112, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 668, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 673, .adv_w = 112, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 681, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 688, .adv_w = 112, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 696, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 701, .adv_w = 112, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 709, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 716, .adv_w = 112, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 725, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 733, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 739, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 747, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 753, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 758, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 765, .adv_w = 112, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 771, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 780, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 789, .adv_w = 112, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 799, .adv_w = 112, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 807, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 814, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 820, .adv_w = 112, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 830, .adv_w = 112, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 840, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 847, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 854, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 861, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 867, .adv_w = 112, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 875, .adv_w = 112, .box_w = 5, .box_h = 3, .ofs_x = 1, .ofs_y = 8},
    {.bitmap_index = 877, .adv_w = 112, .box_w = 4, .box_h = 2, .ofs_x = 3, .ofs_y = 8},
    {.bitmap_index = 878, .adv_w = 112, .box_w = 2, .box_h = 3, .ofs_x = 2, .ofs_y = 8},
    {.bitmap_index = 879, .adv_w = 112, .box_w = 2, .box_h = 3, .ofs_x = 3, .ofs_y = 8},
    {.bitmap_index = 880, .adv_w = 112, .box_w = 7, .box_h = 3, .ofs_x = 0, .ofs_y = 8},
    {.bitmap_index = 883, .adv_w = 112, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 893, .adv_w = 112, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 902, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 909, .adv_w = 112, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 915, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 921, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 927, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 934, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 940, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 947, .adv_w = 112, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 953, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 960, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 966, .adv_w = 112, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 976, .adv_w = 112, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 984, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 991, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 997, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1006, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1013, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1019, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1024, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1031, .adv_w = 112, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1039, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1046, .adv_w = 112, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1052, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1061, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1068, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1075, .adv_w = 112, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1081, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1090, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1097, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1103, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1108, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1115, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1121, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1128, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1134, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1140, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1146, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1152, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1158, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1167, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1174, .adv_w = 112, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1184, .adv_w = 112, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1192, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1201, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1208, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1214, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1219, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1225, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1232, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1239, .adv_w = 112, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1245, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1254, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1261, .adv_w = 112, .box_w = 3, .box_h = 9, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 1265, .adv_w = 112, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1275, .adv_w = 112, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1284, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1291, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1297, .adv_w = 112, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1307, .adv_w = 112, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1315, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1322, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1328, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1337, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1344, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1351, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1357, .adv_w = 112, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1367, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1374, .adv_w = 112, .box_w = 3, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 1378, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1387, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1394, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1403, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1410, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1417, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1424, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1431, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1438, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1444, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1449, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1456, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1463, .adv_w = 112, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1473, .adv_w = 112, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1482, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1489, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1496, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1502, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1508, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1515, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1521, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1528, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1535, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1542, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1549, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1555, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1560, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1567, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1574, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1581, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1588, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1597, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1604, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1613, .adv_w = 112, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1621, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1630, .adv_w = 112, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1639, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1646, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1653, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 1660, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1666, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1673, .adv_w = 112, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1680, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1689, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1696, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1703, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1709, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1715, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1720, .adv_w = 112, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0}
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
const lv_font_t greybeard_cyrillic_14 = {
#else
lv_font_t greybeard_cyrillic_14 = {
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
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if GREYBEARD_CYRILLIC_14*/

