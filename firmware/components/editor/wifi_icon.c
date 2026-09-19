/*
 * SPDX-License-Identifier: MIT
 * Wi-Fi indicator icon for the editor status bar.
 *
 * The Greybeard fonts cover Latin, Latin-1 Supplement and Cyrillic
 * (up to U+04FF plus a few currency / numero glyphs). The "wireless"
 * pictograph U+1F6DC is well outside that range, so the editor
 * cannot display it as a font glyph. This file embeds an 11x7 pixel
 * Wi-Fi symbol (three nested arcs above a source dot) as an LVGL
 * v9 1-bit indexed image.
 *
 * Two descriptors are exposed -- one with a black foreground for the
 * default light theme and one with a white foreground for the
 * inverted (CONFIG_DRAFTLING_EPD_BLACK_BACKGROUND) theme. Both
 * descriptors share the same pixel data and use a transparent
 * palette entry for "off" pixels so the icon composites cleanly
 * over the status-bar background.
 *
 * A second pair of descriptors (wifi6_icon_*) stacks a hand-drawn 5px
 * "6" digit above the same 7px Wi-Fi glyph, separated by a blank row,
 * for the IPv6-dual-stack indicator -- see wifi_manager_has_global_ipv6().
 *
 * Drawn by hand; pure project asset, no external image source.
 */

#include "lvgl.h"
#include "wifi_icon.h"

/* I1 image layout for LVGL v9:
 *   - 8 bytes palette (2 entries, BGRA8888 each):
 *       entry 0 -> color for bit value 0
 *       entry 1 -> color for bit value 1
 *   - then the bitmap, MSB first, ceil(W/8) bytes per row */

#define WIFI_ICON_STRIDE 2

/* Black foreground over transparent background */
static const uint8_t wifi_icon_black_data[8 + WIFI_ICON_STRIDE * WIFI_ICON_H] = {
    /* palette: bit 0 = transparent, bit 1 = opaque black */
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xff,
    /* pixels (11x7, MSB first):
     *   ..#######..
     *   .#.......#.
     *   #..#####..#
     *   ..#.....#..
     *   .#..###..#.
     *   ....#.#....
     *   .....#..... */
    0x3F, 0x80,
    0x40, 0x40,
    0x9F, 0x20,
    0x20, 0x80,
    0x4E, 0x40,
    0x0A, 0x00,
    0x04, 0x00,
};

/* White foreground over transparent background (inverted theme) */
static const uint8_t wifi_icon_white_data[8 + WIFI_ICON_STRIDE * WIFI_ICON_H] = {
    /* palette: bit 0 = transparent, bit 1 = opaque white */
    0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff,
    0x3F, 0x80,
    0x40, 0x40,
    0x9F, 0x20,
    0x20, 0x80,
    0x4E, 0x40,
    0x0A, 0x00,
    0x04, 0x00,
};

const lv_image_dsc_t wifi_icon_black = {
    .header = {
        .w = WIFI_ICON_W,
        .h = WIFI_ICON_H,
        .stride = WIFI_ICON_STRIDE,
        .cf = LV_COLOR_FORMAT_I1,
    },
    .data_size = sizeof(wifi_icon_black_data),
    .data = wifi_icon_black_data,
};

const lv_image_dsc_t wifi_icon_white = {
    .header = {
        .w = WIFI_ICON_W,
        .h = WIFI_ICON_H,
        .stride = WIFI_ICON_STRIDE,
        .cf = LV_COLOR_FORMAT_I1,
    },
    .data_size = sizeof(wifi_icon_white_data),
    .data = wifi_icon_white_data,
};

/* Wi-Fi + "6" dual-stack variant: a 5px "6" digit, one blank row, then
 * the same 7px Wi-Fi glyph as above (identical bytes, copy-pasted so
 * each descriptor's pixel data stays self-contained and independently
 * reviewable).
 *
 * Black foreground over transparent background:
 *   .###.......
 *   #..........
 *   ####.......
 *   #...#......
 *   .###.......
 *   ...........
 *   ..#######..
 *   .#.......#.
 *   #..#####..#
 *   ..#.....#..
 *   .#..###..#.
 *   ....#.#....
 *   .....#..... */
static const uint8_t wifi6_icon_black_data[8 + WIFI_ICON_STRIDE * WIFI6_ICON_H] = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xff,
    0x0E, 0x00,
    0x10, 0x00,
    0x1E, 0x00,
    0x11, 0x00,
    0x0E, 0x00,
    0x00, 0x00,
    0x3F, 0x80,
    0x40, 0x40,
    0x9F, 0x20,
    0x20, 0x80,
    0x4E, 0x40,
    0x0A, 0x00,
    0x04, 0x00,
};

/* White foreground over transparent background (inverted theme) */
static const uint8_t wifi6_icon_white_data[8 + WIFI_ICON_STRIDE * WIFI6_ICON_H] = {
    0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff,
    0x0E, 0x00,
    0x10, 0x00,
    0x1E, 0x00,
    0x11, 0x00,
    0x0E, 0x00,
    0x00, 0x00,
    0x3F, 0x80,
    0x40, 0x40,
    0x9F, 0x20,
    0x20, 0x80,
    0x4E, 0x40,
    0x0A, 0x00,
    0x04, 0x00,
};

const lv_image_dsc_t wifi6_icon_black = {
    .header = {
        .w = WIFI6_ICON_W,
        .h = WIFI6_ICON_H,
        .stride = WIFI_ICON_STRIDE,
        .cf = LV_COLOR_FORMAT_I1,
    },
    .data_size = sizeof(wifi6_icon_black_data),
    .data = wifi6_icon_black_data,
};

const lv_image_dsc_t wifi6_icon_white = {
    .header = {
        .w = WIFI6_ICON_W,
        .h = WIFI6_ICON_H,
        .stride = WIFI_ICON_STRIDE,
        .cf = LV_COLOR_FORMAT_I1,
    },
    .data_size = sizeof(wifi6_icon_white_data),
    .data = wifi6_icon_white_data,
};
