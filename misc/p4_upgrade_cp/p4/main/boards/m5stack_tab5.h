/*
 * M5Stack Tab5: ESP32-P4 host, ESP32-C6 co-processor on SDIO slot 1
 * (wiring from M5Tab5-UserDemo and esp-hosted's
 * ESP32P4_TAB5_C6_BOARD profile). Display, touch and the I/O expander
 * that switches the C6 supply come from the espressif/m5stack_tab5 BSP.
 */
#pragma once

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bsp/m5stack_tab5.h"

#define BOARD_NAME          "Tab5"
#define BOARD_CP_NAME       "ESP32-C6"
#define BOARD_CP_SHORT      "C6"
#define BOARD_CP_CHIP_ID    0x000D   /* esp_chip_id_t of the image */

#define BOARD_CP_SDMMC_SLOT 1
#define BOARD_CP_PIN_CLK    12
#define BOARD_CP_PIN_CMD    13
#define BOARD_CP_PIN_D0     11
#define BOARD_CP_PIN_D1     10
#define BOARD_CP_PIN_D2     9
#define BOARD_CP_PIN_D3     8
#define BOARD_CP_PIN_RESET  15   /* C6 EN: LOW = reset */

/* Switches the C6 supply on (WLAN_PWR_EN, I/O expander 0x44 P0). */
static inline esp_err_t board_cp_power_on(void)
{
    esp_err_t err = bsp_feature_enable(BSP_FEATURE_WIFI, true);
    if (err != ESP_OK) {
        ESP_LOGE("board", "WLAN_PWR_EN on failed: %s", esp_err_to_name(err));
        return err;
    }
    /* Let the C6 LDO settle before the first reset pulse. */
    vTaskDelay(pdMS_TO_TICKS(200));
    return ESP_OK;
}

/* Starts the display with LVGL and turns the backlight on. */
static inline void board_display_start(void)
{
    bsp_display_start();
    bsp_display_backlight_on();
}

static inline void board_display_lock(void)   { bsp_display_lock(0); }
static inline void board_display_unlock(void) { bsp_display_unlock(); }
