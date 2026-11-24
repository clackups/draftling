/*
 * Compatibility shim for the vroland/epdiy managed component,
 * force-included into every C file of that library by
 * components/display/CMakeLists.txt.
 *
 * ESP-IDF v6.1.1 (commit ef6b7b742f, "refactor(lcdcam): move shared
 * core clock setup to drivers") changed two ESP32-S3 LCD_CAM LL
 * functions to take the LCD group id instead of the register block:
 *
 *   lcd_ll_select_clk_src(int group_id, lcd_clock_source_t src)
 *   lcd_ll_set_group_clock_coeff(int group_id, int div_num, ...)
 *
 * epdiy 2.1.3 (and upstream main as of 2026-11) still passes
 * `lcd.hal.dev` / `&LCD_CAM`, which no longer compiles
 * (-Wint-conversion). epdiy only ever uses LCD_CAM group 0
 * (`lcd_hal_init(&lcd.hal, 0)`), so the old first argument is
 * replaced with 0. Drop this file once epdiy handles IDF 6.1.1.
 */
#pragma once

#include "sdkconfig.h"
#include "esp_idf_version.h"

#if CONFIG_IDF_TARGET_ESP32S3 && ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 1, 1)
/* Declare the real functions first, so the macros below do not
 * rewrite their definitions. */
#include "hal/lcd_ll.h"

#define lcd_ll_select_clk_src(dev, src) \
    lcd_ll_select_clk_src(0, (src))
#define lcd_ll_set_group_clock_coeff(dev, div_num, div_a, div_b) \
    lcd_ll_set_group_clock_coeff(0, (div_num), (div_a), (div_b))
#endif
