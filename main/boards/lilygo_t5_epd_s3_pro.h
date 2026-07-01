#pragma once
/* ----- LilyGO T5 E-Paper S3 Pro / Pro Lite -----
 *
 * 4.7" ED047TC1 e-paper panel, 960 x 540 landscape, driven by the
 * vroland/epdiy library's `epd_board_v7` configuration: 8-bit
 * parallel data bus + control lines on direct ESP32-S3 GPIOs, plus
 * a TPS65185 high-voltage power-management IC commanded over I2C via
 * a PCA9535PW I/O expander (I2C addr 0x20). All EPD pin assignments
 * are encapsulated inside epdiy's `epd_board_v7.c` so they are NOT
 * redefined here -- Draftling only needs to know about the
 * peripherals it touches itself (SD card, I2C touch, BOOT button).
 *
 * The "Pro" and "Pro Lite" SKUs share the same H752-01 schematic.
 * The only difference is that the Lite variant depopulates the
 * SX1262 LoRa radio (CS = GPIO 46) and the MIA-M10Q GPS receiver;
 * neither is used by Draftling, so a single Kconfig entry and a
 * single firmware image cover both variants.
 *
 * References:
 *   https://github.com/Xinyuan-LilyGO/T5S3-4.7-e-paper-PRO
 *     docs/pin_define.md, docs/pinmap.md, examples/factory/main/utilities.h
 *   https://github.com/vroland/epdiy
 *     src/board/epd_board_v7.c, README.md "LilyGo Boards" table
 *     (LilyGo T5 S3 E-Paper Pro -> ED047TC1 -> epd_board_v7).
 *
 * Included by main/app_config.h when
 * CONFIG_DRAFTLING_MODEL_LILYGO_T5_EPD_S3_PRO is selected.
 */

#define BOARD_NAME      "LilyGO T5 E-Paper S3 Pro / Pro Lite"

/* On-board MicroSD card on a dedicated SPI bus.
 *
 * Pin assignments per the LilyGO reference:
 *   examples/factory/main/utilities.h
 *     #define BOARD_SD_MOSI 13
 *     #define BOARD_SD_MISO 21
 *     #define BOARD_SD_SCK  14
 *     #define BOARD_SD_CS   12
 *
 * The SPI bus (GPIO 13/21/14) is *shared* with the SX1262 LoRa
 * radio (CS = GPIO 46) on the Pro variant. The Lite variant does
 * not have LoRa installed, so the bus is SD-only there. To keep
 * the same code path on both SKUs and avoid the well-known
 * "intermittent SD init failure with LoRa CS floating" problem
 * (LilyGO T5S3-4.7-e-paper-PRO issue #3), main.cpp drives LoRa CS
 * HIGH before sd_card_init_spi() runs. On the Lite the GPIO is
 * just an unconnected output which is harmless. */
#define SD_SPI_MOSI_PIN 13
#define SD_SPI_MISO_PIN 21
#define SD_SPI_SCK_PIN  14
#define SD_SPI_CS_PIN   12
#define SD_EN_PIN       -1

/* SPI CS of the SX1262 LoRa radio (Pro variant). main.cpp drives
 * this HIGH before SD init to prevent the LoRa chip from
 * intercepting SPI traffic intended for the SD slot. Harmless on
 * the Lite variant where the LoRa silicon is depopulated. */
#define BOARD_LORA_CS_PIN 46

/* MIA-M10Q GPS receiver UART pins (Pro variant). The chip has no
 * dedicated power-enable GPIO on the H752-01 schematic, so software
 * has to ask it to power itself down via UBX-RXM-PMREQ. main.cpp
 * does that at boot (right after SD init) and again before deep
 * sleep, via t5_gps_sleep(). Pin numbers match BOARD_GPS_TXD /
 * BOARD_GPS_RXD in the LilyGO factory firmware:
 *   examples/factory/main/utilities.h
 *     #define BOARD_GPS_TXD 43
 *     #define BOARD_GPS_RXD 44
 * BOARD_GPS_TX_PIN is the ESP32-S3 TX (drives the GPS RX line) and
 * BOARD_GPS_RX_PIN is the ESP32-S3 RX (sampled from GPS TX). The
 * Lite variant has the GPS chip depopulated, so the UART write
 * harmlessly clocks into an open pin. */
#define BOARD_GPS_TX_PIN 43
#define BOARD_GPS_RX_PIN 44

/* I2C bus carrying the GT911 capacitive touch controller, the
 * PCF8563TS RTC, the BQ27220 fuel gauge, the BQ25896 charger, the
 * PCA9535 I/O expander (also used by epdiy for EPD power-rail
 * control) and the TPS65185 EPD power IC. */
#define I2C_SDA_PIN     39
#define I2C_SCL_PIN     40

/* GT911 capacitive touch controller (present on both Pro and Lite
 * per the LilyGO docs/pin_define.md hardware section, even though
 * Draftling defaults to TOUCHSCREEN=y only on the Pro SKU).
 *
 * Native panel coordinate range as reported by the GT911 is
 * 540 wide x 960 tall (portrait); the display backend presents the
 * panel as 960x540 landscape via epd_set_rotation(EPD_ROT_LANDSCAPE),
 * so the touch driver swaps XY to match.
 *
 * NOTE: the GT911 lives on the same I2C bus as epdiy's PCA9535 +
 * TPS65185. Both consumers use ESP-IDF driver-NG
 * (driver/i2c_master.h); ESP-IDF allows only one
 * i2c_new_master_bus() per port. main.cpp creates the bus once,
 * publishes it to the display backend via
 * display_set_shared_i2c_bus() ahead of display_init() (epdiy
 * routes through epd_init_with_config() with
 * EpdInitConfig.i2c.bus_handle), and passes the same handle to
 * the touchscreen component via touchscreen_config_t.i2c_bus. */
#define TOUCH_I2C_ADDR  0x5D
#define TOUCH_INT_PIN   CONFIG_DRAFTLING_TOUCH_INT_GPIO
#define TOUCH_RST_PIN   CONFIG_DRAFTLING_TOUCH_RST_GPIO
#define TOUCH_NATIVE_W  540
#define TOUCH_NATIVE_H  960
#define TOUCH_SWAP_XY   1
#define TOUCH_MIRROR_X  1
#define TOUCH_MIRROR_Y  0

/* Battery monitor.
 *
 * The T5 E-Paper S3 Pro routes the LiPo cell through a BQ25896
 * charger + BQ27220YZFR fuel gauge, both on the shared I2C bus
 * (BQ27220 at 0x55, BQ25896 at 0x6B). There is NO dedicated GPIO
 * ADC pin for the cell voltage, so the ADC-based battery_init()
 * stays a no-op (BATT_ADC_PIN=-1); main.cpp instead calls
 * battery_init_bq27220(shared_i2c_bus) when
 * CONFIG_DRAFTLING_BATTERY_BQ27220 is set (defaulted y for these
 * models). The editor UI shows a voltage-derived battery percentage
 * in the status bar via DRAFTLING_HAS_BATTERY (the BQ27220's own
 * StateOfCharge register is unreliable without Data Memory
 * programming, so the backend only uses its Voltage register). */
#define BATT_ADC_PIN    -1
#define BATT_EN_PIN     -1
#define BATT_DIVIDER    1

/* Front-light (white edge-lit LED panel illumination).
 *
 * The T5 E-Paper S3 Pro / Pro Lite carry a controllable white front-
 * light on GPIO 11 (BOARD_BL_EN in the LilyGO factory firmware --
 * Xinyuan-LilyGO/T5S3-4.7-e-paper-PRO docs/pin_define.md and
 * examples/factory/main/ui_port.cpp). Pin is active HIGH and the
 * actual LEDC PWM init lives inside the EPDIY display backend
 * (components/display/display_epdiy.cpp, gated on
 * CONFIG_DRAFTLING_MODEL_LILYGO_T5_EPD_S3_PRO), so no pin
 * macro needs to be exported here. Unlike the colour-LCD boards,
 * the e-paper panel itself is readable without any front-light, so
 * the editor Settings cycle includes a 0 % step (see
 * DRAFTLING_BACKLIGHT_MIN_PCT in main/Kconfig.projbuild). */

/* Deep-sleep wakeup on the BOOT button (GPIO 0, active-low strapping
 * pin with a board-level pull-up; RTC-capable so EXT0 wake works).
 * Matches BOARD_BOOT_BTN in examples/factory/main/utilities.h. */
#define WAKEUP_GPIO_NUM 0
