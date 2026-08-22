#pragma once
/* ----- Waveshare ESP32-S3-Touch-LCD-7 -----
 *
 * 7" IPS color LCD, 800 x 480 native landscape, ST7262 panel on a
 * 16-bit parallel RGB565 interface driven by the ESP32-S3 LCD RGB
 * peripheral (esp_lcd_new_rgb_panel), same display backend as the
 * Sunton ESP32-8048S0xx family. ESP32-S3N16R8 module: 16 MB flash,
 * 8 MB octal PSRAM.
 *
 * Unlike the Sunton boards, most control lines are not wired to
 * direct ESP32-S3 GPIOs but sit behind an on-board CH422G I2C
 * IO-expander (components/io_expander/io_expander_ch422g.cpp, gated
 * on CONFIG_DRAFTLING_HAS_CH422G) on the same I2C bus as the GT911
 * capacitive touchscreen:
 *   EXIO1 = touch reset, EXIO2 = backlight enable (on/off only, no
 *   PWM), EXIO3 = LCD reset, EXIO4 = SD card chip-select (active
 *   low; the SD card is the only device on its SPI bus, so main.cpp
 *   asserts this once at boot instead of toggling it per-transfer --
 *   see sd_card_init_spi()'s cs=-1 handling).
 * EXIO5 (USB/CAN transceiver select) and EXIO0 are not used by
 * Draftling (no CAN or RS485 support).
 *
 * The LCD-reset (EXIO3) and backlight (EXIO2) pins are consumed
 * directly by components/display/display_rgb.cpp, matching the
 * existing convention that the RGB backend owns every panel-only
 * GPIO/EXIO itself (see sunton_8048s070.h); only the touch-reset and
 * SD chip-select EXIO pins are defined here for main.cpp to consume.
 *
 * All pin numbers and the CH422G EXIO assignments below are taken
 * from the upstream board file shipped with
 * esp-arduino-libs/ESP32_Display_Panel
 * (src/board/supported/waveshare/BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_7.h,
 * Waveshare's own reference for this board) and cross-checked
 * against the Waveshare wiki (https://docs.waveshare.com/ESP32-S3-Touch-LCD-7).
 *
 * Included by main/app_config.h when
 * CONFIG_DRAFTLING_MODEL_WAVESHARE_TOUCH_LCD_7 is selected.
 */

#define BOARD_NAME      "Waveshare ESP32-S3-Touch-LCD-7"

/* Shared I2C bus carrying the CH422G IO-expander and the GT911
 * capacitive touch controller. main.cpp creates this bus once (see
 * the CONFIG_DRAFTLING_HAS_CH422G block) and hands the same
 * i2c_master_bus_handle_t to ch422g_init() and to the touchscreen
 * component (touchscreen_config_t.i2c_bus), matching the shared-bus
 * pattern already used for the epdiy / MIPI-DSI boards. */
#define I2C_SDA_PIN     8
#define I2C_SCL_PIN     9

/* CH422G EXIO pin assignments consumed by main.cpp (components/io_expander).
 * EXIO2 (backlight) and EXIO3 (LCD reset) are consumed directly by
 * components/display/display_rgb.cpp instead -- see the comment above. */
#define CH422G_TOUCH_RST_PIN 1
#define CH422G_SD_CS_PIN     4

/* On-board MicroSD slot on a dedicated SPI bus (MOSI/MISO/SCK are
 * direct GPIOs; chip-select is CH422G EXIO4, asserted once at boot
 * -- see CH422G_SD_CS_PIN above and main.cpp's SD-init block).
 * SD_SPI_CS_PIN is -1 so sd_card_init_spi() leaves hardware CS
 * control disabled (sdspi_device_config_t.gpio_cs = GPIO_NUM_NC). */
#define SD_SPI_MOSI_PIN 11
#define SD_SPI_MISO_PIN 13
#define SD_SPI_SCK_PIN  12
#define SD_SPI_CS_PIN   -1
#define SD_EN_PIN       -1

/* GT911 capacitive touch controller. INT is a direct GPIO (used both
 * for the LVGL pointer indev and, during boot, as a push-pull output
 * held low while main.cpp pulses the CH422G touch-reset line so the
 * GT911 latches its primary I2C address 0x5D). RST is not a direct
 * GPIO -- see CH422G_TOUCH_RST_PIN above -- so TOUCH_RST_PIN is left
 * at the Kconfig default of -1 and touchscreen.cpp skips its own
 * GPIO reset pulse (main.cpp has already reset the chip by the time
 * touchscreen_init() runs). The panel is natively landscape 800x480,
 * so no axis swap or mirror is needed. */
#define TOUCH_I2C_ADDR  0x5D
#define TOUCH_INT_PIN   CONFIG_DRAFTLING_TOUCH_INT_GPIO
#define TOUCH_RST_PIN   CONFIG_DRAFTLING_TOUCH_RST_GPIO
#define TOUCH_NATIVE_W  800
#define TOUCH_NATIVE_H  480
#define TOUCH_SWAP_XY   0
#define TOUCH_MIRROR_X  0
#define TOUCH_MIRROR_Y  0

/* No on-board battery monitor: this is a USB-powered HMI dev board
 * with a PH2.0 battery connector and charge-status LEDs (PWR/CHG/
 * DONE) but no software-readable fuel gauge or ADC divider. */
#define BATT_ADC_PIN    -1
#define BATT_EN_PIN     -1
#define BATT_DIVIDER    1

/* Deep-sleep wakeup on the BOOT button (GPIO0, active-low strapping
 * pin with a board-level pull-up; RTC-capable so EXT0 wake works). */
#define WAKEUP_GPIO_NUM 0
