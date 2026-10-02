#pragma once
/* ----- Waveshare ESP32-S3-LCD-3.16 -----
 *
 * 3.16" IPS color LCD, 320x820 native portrait, ST7701 controller on a
 * 16-bit parallel RGB565 interface driven by the ESP32-S3 LCD RGB
 * peripheral (components/display/display_rgb.cpp, ST7701 branch,
 * gated on CONFIG_DRAFTLING_RGB_PANEL_ST7701). The 270-degree base
 * rotation renders the editor landscape at 820x320. ESP32-S3R8:
 * 16 MB flash, 8 MB octal PSRAM. No touch controller. A slide switch
 * cuts the power; there is no software power latch.
 *
 * Board: https://www.waveshare.com/wiki/ESP32-S3-LCD-3.16
 *
 * Waveshare's demo package was not downloadable at the time of
 * writing, so the pins below come from community projects for this
 * board, which agree with each other:
 *   github.com/fabian-bxr/ESP32-S3-LCD-3.16 (ESP-IDF + LVGL 9)
 *   github.com/thelastoutpostworkshop/ESP32-S3_3_16_ST7701_movie_player
 *   github.com/qianlans/komari-esp32-dashboard (battery ADC, landscape
 *     mapping)
 *
 * The panel pins (RGB data / sync, RST=16, backlight=6) and the
 * ST7701's 3-wire SPI init bus (CS=0, SCK=2, SDA=1) are hard-coded in
 * display_rgb.cpp, matching that backend's convention that it owns
 * every panel GPIO; this header carries only the SD card, battery
 * and wakeup pins.
 *
 * Pin sharing: the 3-wire SPI bus is only used once, inside
 * display_init(), and then released. Its SCK and SDA lines are the
 * MicroSD slot's CMD and CLK, and its CS line is the BOOT button, so
 * display_init() must run before the SD card is mounted and before
 * the BOOT-button poller is set up -- which is main.cpp's normal
 * order. Holding BOOT down while the SD card is busy also selects
 * the panel's command interface, so it may see stray SD traffic.
 *
 * On-board QMI8658 IMU and PCF85063 RTC on I2C (SDA=15, SCL=7) are
 * not used by Draftling. Neither interrupt line reaches the ESP32
 * (the IMU's INT1 resistor to GPIO0 is not fitted, the RTC's INT is
 * unconnected), so main.cpp only quiets both chips once at boot --
 * see ws_lcd316_quiet_i2c_peripherals().
 *
 * Included by main/app_config.h when
 * CONFIG_DRAFTLING_MODEL_WAVESHARE_LCD_316 is selected.
 */

#define BOARD_NAME          "Waveshare ESP32-S3-LCD-3.16"

/* MicroSD on the on-chip SDMMC peripheral, 1-bit mode. */
#define SD_CLK_PIN          1
#define SD_CMD_PIN          2
#define SD_D0_PIN           42

/* I2C bus carrying only the unused QMI8658 IMU and PCF85063 RTC.
 * SA0 of the QMI8658 is tied to GND, which selects 0x6B (0x6A is the
 * address with SA0 high / floating). */
#define IMU_RTC_I2C_SDA_PIN 15
#define IMU_RTC_I2C_SCL_PIN 7
#define QMI8658_I2C_ADDR    0x6B
#define PCF85063_I2C_ADDR   0x51

/* LiPo cell through a 1:2 resistive divider on GPIO4 (ADC1). */
#define BATT_ADC_PIN        4
#define BATT_EN_PIN         -1
#define BATT_DIVIDER        2

/* BOOT button on GPIO0 (active low, pulled up; RTC-capable, so EXT0
 * wake works) is the deep-sleep wakeup source. */
#define WAKEUP_GPIO_NUM     0
