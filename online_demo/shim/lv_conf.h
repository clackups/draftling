/* LVGL configuration of the online demo.
 *
 * On the device LVGL is configured through ESP-IDF's Kconfig (LVGL's
 * own Kconfig defaults plus the CONFIG_LV_* lines of
 * firmware/sdkconfig.defaults); this file sets the same values and
 * leaves the rest at LVGL's defaults. The firmware's PSRAM allocator
 * (lv_mem_psram.c) becomes the C library's malloc here. */
#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_COLOR_DEPTH 16

#define LV_USE_STDLIB_MALLOC    LV_STDLIB_CLIB
#define LV_USE_STDLIB_STRING    LV_STDLIB_CLIB
#define LV_USE_STDLIB_SPRINTF   LV_STDLIB_CLIB

#define LV_USE_OS LV_OS_NONE

/* CONFIG_LV_FONT_MONTSERRAT_14=y */
#define LV_FONT_MONTSERRAT_14 1

/* CONFIG_LV_USE_BIDI=y, CONFIG_LV_BASE_DIR_AUTO=y */
#define LV_USE_BIDI 1
#define LV_BIDI_BASE_DIR_DEF LV_BASE_DIR_AUTO

#define LV_USE_LOG 0

/* No demos, examples or extra libraries. */
#define LV_BUILD_EXAMPLES 0
#define LV_BUILD_DEMOS 0

#endif /* LV_CONF_H */
