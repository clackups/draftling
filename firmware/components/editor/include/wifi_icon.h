#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WIFI_ICON_W  11
#define WIFI_ICON_H  7

/* Two pre-baked palette variants of the same 11x7 Wi-Fi icon. The
 * editor UI picks one or the other based on the theme. */
extern const lv_image_dsc_t wifi_icon_black;
extern const lv_image_dsc_t wifi_icon_white;

#define WIFI6_ICON_W 11
#define WIFI6_ICON_H 13

/* Same Wi-Fi glyph with a small "6" badge above it, shown instead of
 * wifi_icon_{black,white} when the STA interface has a global IPv6
 * address (see wifi_manager_has_global_ipv6()). */
extern const lv_image_dsc_t wifi6_icon_black;
extern const lv_image_dsc_t wifi6_icon_white;

#ifdef __cplusplus
}
#endif
