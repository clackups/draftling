#pragma once
/* ----- RockBase NM-CYD-C5 -----
 *
 * "Cheap Yellow Display" built on ESP32-C5-WROOM-1 (RISC-V, dual-band
 * Wi-Fi 6 + BLE 5 + IEEE 802.15.4), 16MB flash + 8MB PSRAM. 2.8" IPS
 * color LCD, 320x240 landscape, ST7789 controller over SPI.
 *
 * The display, an XPT2046 resistive touch controller and the on-board
 * MicroSD slot all share one physical SPI bus with separate CS lines
 * (per https://github.com/RockBase-iot/NM-CYD-C5 and the pin map in
 * RockBase-iot/ESP32-KillerBee's include/boards/nm_cyd_c5_pins.h):
 *
 *   Device   SCK  MISO  MOSI  CS
 *   Display    6     2     7  23
 *   Touch      6     2     7   1
 *   SD Card    6     2     7  10
 *
 * DC=24, RST=-1 (display RESET is tied to the board RESET, not a
 * dedicated GPIO), BL=25 (LEDC PWM backlight). Draftling does not yet
 * implement an XPT2046 resistive-touch driver (the touchscreen
 * component currently only supports I2C capacitive controllers), so
 * touch is left unused here -- Draftling is keyboard-driven via an
 * external BLE keyboard like every other supported board.
 *
 * This board has no user buttons besides the power switch, so (like
 * the M5Stack Tab5) there is no RTC-capable GPIO wake source: standby
 * enters real deep sleep and the only wake path is the hardware RESET
 * button, with autosave restoring the document on the next boot.
 *
 * Included by main/app_config.h when CONFIG_DRAFTLING_MODEL_NM_CYD_C5
 * is selected.
 */

#define BOARD_NAME          "RockBase NM-CYD-C5"

/* ST7789 SPI display interface. Shares its physical bus (SCK/MISO/MOSI)
 * with the MicroSD card via separate CS lines -- see
 * components/display/display_st7789.cpp and main.cpp's SD-init path. */
#define LCD_SPI_HOST        SPI2_HOST
#define LCD_MOSI_PIN        7
#define LCD_MISO_PIN        2
#define LCD_SCK_PIN         6
#define LCD_CS_PIN          23
#define LCD_DC_PIN          24
#define LCD_RST_PIN         -1
#define LCD_BL_PIN          25

/* MicroSD card, sharing the display's SPI bus (mosi/miso/sck passed
 * as -1 to sd_card_init_spi() so it reuses the bus display_st7789_init()
 * already brought up). */
#define SD_SPI_MOSI_PIN     -1
#define SD_SPI_MISO_PIN     -1
#define SD_SPI_SCK_PIN      -1
#define SD_SPI_CS_PIN       10
#define SD_EN_PIN           -1

/* XPT2046 resistive touch controller CS (GPIO1), on the same shared
 * SPI bus. Not yet wired up by Draftling -- kept here for a future
 * touch driver so the pin does not need to be rediscovered. */
#define TOUCH_SPI_CS_PIN    1

/* No on-board battery monitor (USB-powered dev board). */
#define BATT_ADC_PIN        -1
#define BATT_EN_PIN         -1
#define BATT_DIVIDER        1

/* No user buttons besides the power switch; no RTC-capable GPIO wake
 * source is available (see the file header comment above). This value
 * is set for code paths that reference WAKEUP_GPIO_NUM directly, but
 * no EXT0 arming is actually performed on this board. */
#define WAKEUP_GPIO_NUM     0
