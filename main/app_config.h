#pragma once

#include "sdkconfig.h"

/* Display dimensions (derived from Kconfig hardware model selection) */
#define DISPLAY_WIDTH   CONFIG_DRAFTLING_DISPLAY_WIDTH
#define DISPLAY_HEIGHT  CONFIG_DRAFTLING_DISPLAY_HEIGHT
#define DISPLAY_ROTATE  CONFIG_DRAFTLING_DISPLAY_ROTATE_ANGLE

/* Logical pixel size: every logical LVGL pixel is rendered as
 * DISPLAY_SCALE x DISPLAY_SCALE panel pixels by the display backend
 * (nearest-neighbor expansion). The editor and LVGL canvas use
 * DISPLAY_LOGICAL_WIDTH / DISPLAY_LOGICAL_HEIGHT; only the display
 * backend deals in physical panel pixels. */
#define DISPLAY_SCALE          CONFIG_DRAFTLING_DISPLAY_SCALE
#define DISPLAY_LOGICAL_WIDTH  (DISPLAY_WIDTH  / DISPLAY_SCALE)
#define DISPLAY_LOGICAL_HEIGHT (DISPLAY_HEIGHT / DISPLAY_SCALE)

/* SD Card mount point (shared across all hardware models) */
#define SD_MOUNT_POINT  "/sdcard"

#if defined(CONFIG_DRAFTLING_MODEL_WAVESHARE_RLCD42)
/* ----- Waveshare ESP32-S3-RLCD-4.2 ----- */
#define BOARD_NAME      "Waveshare ESP32-S3-RLCD-4.2"

/* RLCD Display - SPI interface */
#define RLCD_MOSI_PIN   12
#define RLCD_SCK_PIN    11
#define RLCD_DC_PIN     5
#define RLCD_CS_PIN     40
#define RLCD_RST_PIN    41
#define RLCD_TE_PIN     6

/* SD Card - SDMMC 1-bit interface */
#define SD_CLK_PIN      38
#define SD_CMD_PIN      21
#define SD_D0_PIN       39

/* I2C Bus */
#define I2C_SDA_PIN     13
#define I2C_SCL_PIN     14

/* Battery voltage ADC (GPIO4, ADC1_CH3, 3:1 divider) */
#define BATT_ADC_PIN    4
#define BATT_EN_PIN     -1
#define BATT_DIVIDER    3

/* Deep-sleep wakeup on GPIO18 (EXT0, active-low) */
#define WAKEUP_GPIO_NUM 18

#elif defined(CONFIG_DRAFTLING_MODEL_LILYGO_T5_EPD_S3_PRO)
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
 * so the touch driver swaps XY to match (same convention as the
 * M5Stack PaperS3 GT911 block above).
 *
 * NOTE: the GT911 lives on the same I2C bus as epdiy's PCA9535 +
 * TPS65185. Both consumers use ESP-IDF driver-NG
 * (driver/i2c_master.h); ESP-IDF allows only one
 * i2c_new_master_bus() per port. main.cpp creates the bus once,
 * publishes it to the display backend via
 * display_set_shared_i2c_bus() ahead of display_init() (epdiy
 * routes through epd_init_with_config() with
 * EpdInitConfig.i2c.bus_handle), and passes the same handle to
 * the touchscreen component via touchscreen_config_t.i2c_bus. This
 * requires the epdiy commit pinned in
 * components/display/idf_component.yml (the published 2.0.0
 * managed component still uses the legacy driver/i2c.h and would
 * abort with a CONFLICT message). With the shared-bus path in
 * place, DRAFTLING_TOUCHSCREEN defaults to `y` on both Pro and
 * Lite. */
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

#elif defined(CONFIG_DRAFTLING_MODEL_LILYGO_T5_EPD_S3_PRO_H752)
/* ----- LilyGO T5 E-Paper S3 Pro original H752 -----
 *
 * Original H752 / v1.0-240810 hardware revision. This board is not
 * pin-compatible with the newer H752-01 Pro / Pro Lite target above:
 * there is no PCA9535/TPS65185 epdiy board path. The panel is driven
 * by LilyGO's older EPD47 shift-register backend using CFG_DATA /
 * CFG_CLK / CFG_STR plus direct CKH/CKV/STH and D0..D7 lines.
 */
#define BOARD_NAME      "LilyGO T5 E-Paper S3 Pro H752"

/* Shared SPI bus: MicroSD and SX1262 LoRa. */
#define SD_SPI_MISO_PIN 8
#define SD_SPI_MOSI_PIN 17
#define SD_SPI_SCK_PIN  18
#define SD_SPI_CS_PIN   16
#define SD_EN_PIN       -1

#define BOARD_LORA_CS_PIN 46
#define BOARD_LORA_RST_PIN 43
#define BOARD_LORA_IRQ_PIN 3
#define BOARD_LORA_BUSY_PIN 44

/* Original H752 has no GPS receiver populated on the board variant
 * this port targets. Keeping the macros at -1 makes the existing
 * T5 radio-sleep helper compile into no-op GPS handling. */
#define BOARD_GPS_TX_PIN -1
#define BOARD_GPS_RX_PIN -1

/* I2C bus carrying GT911, PCF8563 RTC, BQ27220 fuel gauge and
 * BQ25896 charger. There is no PCA9535/TPS65185 on original H752. */
#define I2C_SDA_PIN     6
#define I2C_SCL_PIN     5

/* GT911 capacitive touch controller. Calibrated empirically on the
 * actual panel (corner-tap capture, 2026-06-10): the controller
 * reports in a 540x960 PORTRAIT space rotated 90 degrees from the
 * landscape display. Screen TL/TR/BL/BR map to raw (540,0)/
 * (540,960)/(0,0)/(0,960), so screen_x = raw_y and screen_y =
 * 539 - raw_x: mirror X first, then swap. In Draftling, LVGL works
 * in the logical 480x270 frame when DISPLAY_SCALE=2, so the
 * touchscreen component scales the transformed coordinates down. */
#define TOUCH_I2C_ADDR  0x5D
#define TOUCH_INT_PIN   CONFIG_DRAFTLING_TOUCH_INT_GPIO
#define TOUCH_RST_PIN   CONFIG_DRAFTLING_TOUCH_RST_GPIO
#define TOUCH_NATIVE_W  540
#define TOUCH_NATIVE_H  960
#define TOUCH_SWAP_XY   1
#define TOUCH_MIRROR_X  1
#define TOUCH_MIRROR_Y  0

/* Battery monitor. The BQ27220/BQ25896 I2C chips are present on the
 * original H752. GPIO4 also exposes a battery ADC path in LilyGO's
 * pin map, but Draftling prefers the fuel gauge when available. */
#define BATT_ADC_PIN    -1
#define BATT_EN_PIN     -1
#define BATT_DIVIDER    1

/* Front-light / backlight enable, active HIGH. The H752 display
 * backend drives this via LEDC PWM and the settings menu exposes
 * the normal Backlight percentage cycle. */
#define H752_BL_PIN     40

/* User key. BOOT/GPIO0 is left for bootloader entry; GPIO48 is the
 * only practical user button on this revision. It is also the light
 * sleep wake source used by the H752 standby path. */
#define WAKEUP_GPIO_NUM 48

#elif defined(CONFIG_DRAFTLING_MODEL_M5STACK_PAPERS3)
/* ----- M5Stack PaperS3 -----
 *
 * The PaperS3 drives an ED047TC1 e-paper panel (960 x 540 native
 * landscape) via the ESP32-S3 LCD/I80 parallel peripheral, fed by
 * the vroland/epdiy library through the in-tree
 * components/display/epd_board_papers3.c board definition.
 *
 * That board definition owns GPIOs 6-18 (8-bit data bus + CKH, CKV,
 * STH, SPV, XLE) and GPIO 45/46 (EPD_EN and BST_EN power-rail gates).
 * Pins below are the ones Draftling itself touches outside of the
 * display driver (SD card, wakeup, I2C touch / RTC / IMU, battery
 * monitor). See the M5Stack hardware reference for the full pin
 * list: https://docs.m5stack.com/en/core/papers3
 */
#define BOARD_NAME      "M5Stack PaperS3"

/* Onboard MicroSD on a dedicated SPI host (SPI3 - the EPD parallel
 * bus owned by epdiy claims GPIO 6-18 plus 45/46 for its data /
 * control / power lines, so the SD slot must use a separate set of
 * pins).
 *
 * Pin assignments verified against the M5Stack PaperS3 hardware
 * reference and the FastEPD/Arduino "papers3_screenshot" example:
 *   CS=47, SCK=39, MOSI=38, MISO=40. */
#define SD_SPI_MOSI_PIN 38
#define SD_SPI_MISO_PIN 40
#define SD_SPI_SCK_PIN  39
#define SD_SPI_CS_PIN   47
#define SD_EN_PIN       -1

/* I2C bus (BM8563 RTC, BMI270 IMU and GT911 capacitive touch
 * controller on the PaperS3) */
#define I2C_SDA_PIN     41
#define I2C_SCL_PIN     42

/* GT911 capacitive touch controller.
 *
 * Pin assignments match the M5Stack PaperS3 hardware reference
 * (pin_int=GPIO48, pin_sda=GPIO41, pin_scl=GPIO42, no RST GPIO).
 * The GT911 reset line is not wired to any ESP32-S3 pin on this
 * board -- it is released by the power-rail RC, so the touchscreen
 * driver cannot perform the INT-driven address-select reset
 * sequence and instead probes both possible I2C addresses (0x5D
 * primary, 0x14 backup).
 *
 * Native panel coordinate range as reported by the GT911 is
 * 540 wide x 960 tall (portrait). The epdiy backend rotates the
 * framebuffer to a 960x540 landscape orientation
 * (EPD_ROT_LANDSCAPE), so the touch axes need swapping and the
 * vertical axis needs inverting -- swap_xy=1, mirror_x=1,
 * mirror_y=0. */
#define TOUCH_I2C_ADDR  0x5D
#define TOUCH_INT_PIN   CONFIG_DRAFTLING_TOUCH_INT_GPIO
#define TOUCH_RST_PIN   CONFIG_DRAFTLING_TOUCH_RST_GPIO
#define TOUCH_NATIVE_W  540
#define TOUCH_NATIVE_H  960
#define TOUCH_SWAP_XY   1
#define TOUCH_MIRROR_X  1
#define TOUCH_MIRROR_Y  0

/* Battery voltage monitor.
 *
 * The PaperS3 routes the LiPo cell through a 1:2 resistive divider
 * (R_top == R_bottom, V_pin = V_bat / 2) into ADC1 channel 2 on
 * GPIO3. There is no enable transistor -- the divider is always
 * powered. Pin and divider ratio match the M5Unified Power_Class
 * configuration for board_M5PaperS3 (BAT_ADC = ADC1_GPIO3,
 * adc_ratio = 2.0). The battery component will multiply the
 * measured pin voltage by BATT_DIVIDER (=2) to recover the actual
 * cell voltage.
 *
 * GPIO4 carries the charger CHG_STAT signal on the PaperS3 (active
 * low while charging). It is not wired through the battery API yet;
 * the editor only displays the cell percentage. */
#define BATT_ADC_PIN    3
#define BATT_EN_PIN     -1
#define BATT_DIVIDER    2

/* Deep-sleep wakeup on the BOOT button (GPIO0, active-low).
 *
 * Earlier revisions tried GPIO21 (wrong -- that's the on-board
 * buzzer/speaker pin) and GPIO48 (the GT911 touch-panel INT line).
 * GPIO48 also failed because the GT911 holds its INT line low while
 * uninitialised (the line doubles as I2C-address selection during
 * reset), so the standby manager would see GPIO48 stuck low and
 * fire the GPIO_INTR_LOW_LEVEL wake immediately.
 *
 * GPIO0 is the only digital input button on the PaperS3 besides the
 * hardware power switch, and -- being an ESP32-S3 strapping pin with
 * a board-level pull-up -- is guaranteed high while the device is
 * idle. It is also RTC-capable, so we can use real EXT0 deep sleep
 * (matching the other supported boards) instead of the light-sleep
 * + esp_restart() workaround. */
#define WAKEUP_GPIO_NUM 0

#elif defined(CONFIG_DRAFTLING_MODEL_WAVESHARE_TOUCH_LCD_349)
/* ----- Waveshare ESP32-S3-Touch-LCD-3.49 -----
 *
 * 3.49" IPS color LCD, 172 x 640 native portrait (presented as
 * 640 x 172 landscape via software rotation), AXS15231B controller
 * over QSPI. Pin assignments below are taken verbatim from the
 * official Waveshare reference firmware
 * (Examples/Arduino/09_LVGL_V8_Test/user_config.h at
 * https://github.com/waveshareteam/ESP32-S3-Touch-LCD-3.49); they
 * differ from the placeholder values used during early bring-up.
 *
 * Touch input is an Allystar AXS5106-family capacitive controller
 * sharing the AXS15231B's I2C protocol family. INT and RST are not
 * wired to the ESP32 on this board (Waveshare's own example also
 * sets both to -1).
 */
#define BOARD_NAME          "Waveshare ESP32-S3-Touch-LCD-3.49"

/* AXS15231B QSPI display interface */
#define LCD_QSPI_CS_PIN     9
#define LCD_QSPI_SCK_PIN    10
#define LCD_QSPI_D0_PIN     11
#define LCD_QSPI_D1_PIN     12
#define LCD_QSPI_D2_PIN     13
#define LCD_QSPI_D3_PIN     14
#define LCD_RST_PIN         21
#define LCD_TE_PIN          -1   /* TE not wired on this board */
/* GPIO 8 drives the BL boost-converter enable, and is wired
 * **active LOW**: a logical LOW on the pin turns the backlight ON
 * at full brightness, a logical HIGH turns it OFF. This is the
 * polarity used by Waveshare's own reference firmware
 * (Examples/ESP-IDF/10_LVGL_V9_Test/components/lcd_bl_pwm_bsp/
 * lcd_bl_pwm_bsp.h), where `LCD_PWM_MODE_255` (the brightest level)
 * is defined as `(0xff - 255) = 0` and `LCD_PWM_MODE_0` (off) is
 * `(0xff - 0) = 255`.
 *
 * We mirror that reference exactly: an LEDC channel on GPIO 8
 * driven at duty 0 keeps the boost circuit on at full brightness,
 * and intermediate duties dim the panel proportionally. The pin is
 * pre-configured as an output with the internal pull-up enabled
 * before the LEDC channel is bound so the line never floats during
 * the boot-to-LEDC handoff. `LCD_BL_ACTIVE_LOW = 1` tells the
 * AXS15231B backend to invert the percent->duty mapping (and to
 * drive the pin HIGH for deep-sleep BL cut).
 *
 * Earlier bring-up attempts on this board left the pin high-Z
 * (LCD_BL_PIN = -1) because we had inverted the polarity and the
 * "100 % brightness" duty was actually OFF, producing a "screen
 * dark all session, flashes only at deep-sleep entry" symptom.
 * Verified working against Waveshare's 10_LVGL_V9_Test example
 * with `Backlight_Testing` enabled (the brightness loop cycles
 * 255 -> 175 -> 125 -> 0 raw duties and the panel dims
 * accordingly). */
#define LCD_BL_PIN          8
#define LCD_BL_ACTIVE_LOW   1

/* On-board MicroSD card slot on a dedicated SPI bus. Pin assignments
 * match the official Waveshare ESP32-S3-Touch-LCD-3.49 pin map
 * (https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-3.49): SD_CS=38,
 * SD_MOSI=39, SD_MISO=40, SD_SCLK=41. The earlier placeholder values
 * on GPIO 4-7 were both wrong (the board really does have an on-board
 * SD slot) and conflicted with the on-board BAT_ADC on IO4. */
#define SD_SPI_MOSI_PIN     39
#define SD_SPI_MISO_PIN     40
#define SD_SPI_SCK_PIN      41
#define SD_SPI_CS_PIN       38
#define SD_EN_PIN           -1

/* I2C bus carrying the AXS15231-family touch controller. Pins match
 * Touch_SDA_NUM / Touch_SCL_NUM in Waveshare's user_config.h. */
#define I2C_SDA_PIN         17
#define I2C_SCL_PIN         18

/* AXS5106-family touch controller. INT and RST are not wired on
 * this board (both -1 in Waveshare's reference). The controller
 * reports coordinates in the panel's landscape orientation
 * (640 wide x 172 tall) but with both axes flipped relative to
 * the LCD scan-out (per Waveshare's USER_DISP_ROT_90 touch handler:
 * data->point.x = LCD_H_RES - pointX, data->point.y = LCD_V_RES -
 * pointY), so mirror both X and Y. */
#define TOUCH_I2C_ADDR      0x3B
#define TOUCH_INT_PIN       CONFIG_DRAFTLING_TOUCH_INT_GPIO
#define TOUCH_RST_PIN       CONFIG_DRAFTLING_TOUCH_RST_GPIO
#define TOUCH_NATIVE_W      640
#define TOUCH_NATIVE_H      172
#define TOUCH_SWAP_XY       0
#define TOUCH_MIRROR_X      1
#define TOUCH_MIRROR_Y      1

/* On-board battery monitor.
 *
 * The Touch-LCD-3.49 wires the LiPo cell through a ~3:1 resistive
 * divider into GPIO 4 (ADC1_CH3). Matches the Waveshare BAT_ADC pin
 * map in the official wiki and the `01_ADC_Test/adc_bsp.c` reference
 * firmware, which recovers the cell voltage with
 * `*value = 0.001 * vol * 3` -- i.e. BATT_DIVIDER = 3, not the 2
 * we initially guessed (a 2 multiplier produced a ~18 % reading on
 * a fully charged cell, since the table maps ~2.8 V to ~0 %). */
#define BATT_ADC_PIN        4
#define BATT_EN_PIN         -1
#define BATT_DIVIDER        3

/* Power management.
 *
 * The Touch-LCD-3.49 has a hardware power latch that keeps the
 * battery rail alive after the user releases the PWR button: a
 * TCA9554 I2C IO-expander pin 6 must be driven HIGH at boot to keep
 * the latch closed, and driving it LOW cuts the battery rail and
 * fully powers the board off. The PWR button itself is wired to
 * GPIO 16 (active LOW) -- a momentary press on the unpowered board
 * applies VBAT just long enough for the firmware to latch IO6 HIGH;
 * holding the same button while powered triggers the firmware's
 * long-press handler which auto-saves and cuts the latch.
 *
 * The TCA9554 lives on a *second* I2C bus on GPIO 47 (SDA) / GPIO 48
 * (SCL), shared with the on-board BM8563 RTC and BMI270 IMU; the
 * touch-controller I2C bus on GPIO 17 / 18 above is separate and is
 * owned by the touchscreen component.
 *
 * Pin assignments and the TCA9554 latch bit match the Waveshare
 * reference firmware (Examples/ESP-IDF/07_BATT_PWR_Test/components/
 * user_app/user_app.cpp and i2c_bsp/i2c_bsp.c). */
#define PWR_BUTTON_GPIO     16
#define PWR_I2C_SDA_PIN     47
#define PWR_I2C_SCL_PIN     48
#define PWR_TCA9554_ADDR    0x20  /* A2=A1=A0=0 */
#define PWR_TCA9554_LATCH_BIT 6

/* Deep-sleep wakeup on BOOT (GPIO0, active-low strapping pin).
 * Used only on USB power: on battery, the inactivity timer cuts the
 * power latch (via the `power` component) instead of entering deep
 * sleep, so the wake source is then the PWR button on GPIO 16
 * (which re-applies VBAT and cold-boots the firmware). */
#define WAKEUP_GPIO_NUM     0

#elif defined(CONFIG_DRAFTLING_MODEL_JC3248W535)
/* ----- Guition JC3248W535 -----
 *
 * 3.5" IPS color LCD, 480 x 320, AXS15231B controller over QSPI.
 * Pin assignments below come from the JC3248W535 reference schematic
 * widely shared in the LovyanGFX / LVGL community for this board;
 * verify against the actual silk-screen of your unit if init fails.
 *
 * Touch input (AXS5106L capacitive controller, I2C address 0x3B)
 * is used as a secondary input device on this board (the JC3248W535
 * has no user buttons besides the power switch). The touch INT line
 * doubles as the EXT0 deep-sleep wake source. The touch pins are
 * configurable via menuconfig under "DRAFTLING Configuration ->
 * Touchscreen INT/RST GPIO" so a non-stock wiring can be supported
 * without code changes.
 */
#define BOARD_NAME          "Guition JC3248W535"

/* AXS15231B QSPI display interface */
#define LCD_QSPI_CS_PIN     45
#define LCD_QSPI_SCK_PIN    47
#define LCD_QSPI_D0_PIN     21
#define LCD_QSPI_D1_PIN     48
#define LCD_QSPI_D2_PIN     40
#define LCD_QSPI_D3_PIN     39
/* The AXS15231B has no dedicated external reset pin on this
 * board (the Arduino reference driver passes GFX_NOT_DEFINED);
 * the controller is reset by its vendor unlock/lock SPI sequence.
 * GPIO4 is in fact the touch I2C SDA line -- driving it as a
 * push-pull RST output during display init would hold touch SDA
 * low and prevent the I2C bus from ever coming up. */
#define LCD_RST_PIN         -1
#define LCD_TE_PIN          38
#define LCD_BL_PIN          1

/* SD card on a dedicated SPI bus */
#define SD_SPI_MOSI_PIN     11
#define SD_SPI_MISO_PIN     13
#define SD_SPI_SCK_PIN      12
#define SD_SPI_CS_PIN       10
#define SD_EN_PIN           -1

/* I2C bus carrying the AXS15231B integrated touch controller.
 * Per the JC3248W535 board pinout (Tactility, Yoradio, edgerun
 * and multiple community references): SDA=GPIO4, SCL=GPIO8. */
#define I2C_SDA_PIN         4
#define I2C_SCL_PIN         8

/* AXS5106L touch controller. INT defaults to GPIO3 (RTC-capable,
 * required for wake-on-touch). The reset line is tied to the LCD
 * reset on this board so we leave the dedicated touch RST at -1.
 *
 * The controller reports coordinates in the panel's native portrait
 * orientation (320 wide x 480 tall). The LCD backend software-
 * rotates to 480x320 landscape, so we swap XY and mirror X on the
 * touch side to match. */
#define TOUCH_I2C_ADDR      0x3B
#define TOUCH_INT_PIN       CONFIG_DRAFTLING_TOUCH_INT_GPIO
#define TOUCH_RST_PIN       CONFIG_DRAFTLING_TOUCH_RST_GPIO
#define TOUCH_NATIVE_W      320
#define TOUCH_NATIVE_H      480
#define TOUCH_SWAP_XY       1
#define TOUCH_MIRROR_X      1
#define TOUCH_MIRROR_Y      0

/* No on-board battery monitor on this dev board */
#define BATT_ADC_PIN        -1
#define BATT_EN_PIN         -1
#define BATT_DIVIDER        1

/* Deep-sleep wakeup. The JC3248W535 has no BOOT button on an
 * RTC-capable GPIO, so the only way to wake the device is via the
 * touch INT line -- enabled by setting both DRAFTLING_TOUCHSCREEN
 * and DRAFTLING_STANDBY_WAKE_ON_TOUCH (the per-board default).
 * WAKEUP_GPIO_NUM below is just the fallback; the standby code
 * overrides it with CONFIG_DRAFTLING_TOUCH_INT_GPIO when
 * wake-on-touch is enabled. */
#define WAKEUP_GPIO_NUM     0

#elif defined(CONFIG_DRAFTLING_MODEL_M5STACK_TAB5)
/* ----- M5Stack Tab5 -----
 *
 * 5" IPS color LCD, 1280 x 720 landscape, driven over MIPI-DSI
 * (2 data lanes, DPI continuous scanout) by the ESP32-P4. Two
 * hardware revisions exist in the wild:
 *   v1: ILI9881C panel + GT911 capacitive touch (default in BSP)
 *   v2: ST7123 panel + ST7123 touch (auto-detected by I2C probe)
 *
 * All panel + power-rail bring-up is handled by the upstream
 * espressif/m5stack_tab5 BSP managed component, which is pulled in
 * by components/display/idf_component.yml for the esp32p4 target.
 * That includes:
 *   - PI4IOE5V6408 I/O expander (I2C addr 0x43 / 0x44) that drives
 *     LCD_EN (IO4) and TOUCH_EN (IO5) power rails;
 *   - MIPI-DSI PHY LDO power-up;
 *   - the ILI9881C / ST7123 init data blocks;
 *   - LEDC PWM backlight on GPIO22.
 *
 * Pins listed below are the ones Draftling itself touches outside
 * of the BSP (SD card, shared I2C bus carrying the GT911 touch).
 * See https://docs.m5stack.com/en/core/Tab5 for the full pinout.
 */
#define BOARD_NAME          "M5Stack Tab5"

/* On-board MicroSD slot, wired to ESP32-P4 SDMMC pins. The Draftling
 * SD layer uses a generic SPI bus (sd_card_init_spi) on every board
 * except the Waveshare RLCD-4.2; map the SDMMC pins (per BSP
 * m5stack_tab5.h: BSP_SD_SPI_CLK=43, BSP_SD_SPI_MISO=39,
 * BSP_SD_SPI_MOSI=44, BSP_SD_SPI_CS=42) to the SPI alias so the
 * existing SPI3-host code path works without change. */
#define SD_SPI_MOSI_PIN     44
#define SD_SPI_MISO_PIN     39
#define SD_SPI_SCK_PIN      43
#define SD_SPI_CS_PIN       42
#define SD_EN_PIN           -1

/* I2C bus shared by the GT911 touch controller, PI4IOE5V6408
 * I/O expander, BMI270 IMU, audio codecs and other on-board
 * peripherals (BSP_I2C_SDA=31, BSP_I2C_SCL=32). The MIPI-DSI
 * display backend creates this bus via bsp_i2c_init() and the
 * touchscreen component shares it via touchscreen_config_t.i2c_bus
 * (same pattern as the LilyGO T5 E-Paper S3 Pro / Pro Lite). */
#define I2C_SDA_PIN         31
#define I2C_SCL_PIN         32

/* M5Stack Tab5 touch controller. Two hardware variants exist:
 *
 *   - v1: Goodix GT911 capacitive touch at I2C 0x14 (backup
 *         address). RST is not wired to any ESP32-P4 GPIO, so
 *         the GT911 internal POR is triggered by power-cycling
 *         BSP_TOUCH_EN through the first PI4IOE5V6408. main.cpp
 *         pre-drives GPIO23 LOW (overriding the 3V3 pull-up on
 *         INT) before that toggle so the GT911 latches its
 *         backup address rather than the unused 0x5D.
 *
 *   - v2: Sitronix ST7123 integrated panel+touch at I2C 0x55.
 *         No INT-latch dance is needed.
 *
 * Rather than re-implement the per-variant bring-up, the
 * Draftling touchscreen component delegates to the upstream
 * espressif/m5stack_tab5 BSP's bsp_touch_new() on this board.
 * The BSP probes 0x55 first (ST7123, board v2) and falls back
 * to 0x14 backup (GT911, board v1), instantiates the matching
 * esp_lcd_touch_* driver, and returns a generic
 * esp_lcd_touch_handle_t. touchscreen.cpp polls through
 * esp_lcd_touch_read_data + esp_lcd_touch_get_coordinates so
 * the same source handles both variants transparently. The
 * pre-display TOUCH_EN power-cycle in main.cpp still runs (it
 * is the only thing that fixes the v1 GT911 address latch).
 *
 * TOUCH_I2C_ADDR / TOUCH_RST_PIN below are kept as nominal
 * (v1 GT911) values for the legacy touchscreen_config_t struct;
 * the BSP path ignores them and uses BSP_LCD_TOUCH_INT for INT
 * and GPIO_NUM_NC for RST. Native panel coordinate range is
 * 720 x 1280 portrait (matches BSP_LCD_H_RES / BSP_LCD_V_RES).
 *
 * draftling_lvgl_port_init() is called with the user-configured
 * DISPLAY_ROTATE (default 270 for landscape) and LVGL v9.2 then
 * rotates indev points itself inside indev_pointer_proc(). So
 * touchscreen.cpp must hand LVGL coords in the *pre-rotation*
 * portrait frame -- main.cpp passes user_rotate_deg=0 for that
 * reason. Empirically (Tab5 v1, GT911 via BSP), the controller's
 * native axes are inverted on both axes relative to the panel's
 * portrait pixel axes: a tap at landscape upper-left produces
 * raw_x near 0 (small end of the 0..720 short-axis range) and
 * raw_y near 1280 (large end of the long-axis range), which
 * corresponds to panel-native portrait corner (0, 1280-ish) --
 * the opposite portrait corner from what LVGL's rotation-270
 * indev transform expects for landscape (0, 0). Mirroring both
 * X and Y in native_to_logical() flips raw onto panel-native
 * portrait so LVGL's own indev rotation lands the cursor under
 * the finger. */
#define TOUCH_I2C_ADDR      0x14
#define TOUCH_INT_PIN       CONFIG_DRAFTLING_TOUCH_INT_GPIO
#define TOUCH_RST_PIN       CONFIG_DRAFTLING_TOUCH_RST_GPIO
#define TOUCH_NATIVE_W      720
#define TOUCH_NATIVE_H      1280
#define TOUCH_SWAP_XY       0
#define TOUCH_MIRROR_X      1
#define TOUCH_MIRROR_Y      1

/* No board-managed battery ADC: Tab5 carries a 2S NP-F550 Li-ion
 * pack monitored by an INA226 power monitor at I2C 0x41 (on the
 * system I2C bus, SDA=31 / SCL=32 above). main.cpp calls
 * battery_init_ina226(bsp_i2c_get_handle(), 0x41, 2) under
 * CONFIG_DRAFTLING_BATTERY_INA226 (defaulted y for this board), and
 * the editor status bar picks the percentage up via
 * DRAFTLING_HAS_BATTERY. The ADC-based battery_init() stays a
 * no-op (BATT_ADC_PIN=-1). */
#define BATT_ADC_PIN        -1
#define BATT_EN_PIN         -1
#define BATT_DIVIDER        1

/* Standby on Tab5 uses real deep sleep (esp_deep_sleep_start). The
 * touch INT pin (GPIO 23) is not an LP_IO on the ESP32-P4 and no
 * user button is wired to a LP_IO either, so there is no usable
 * GPIO wake source: the only wake path is the hardware RESET
 * button (cold boot, autosave restores the editor state).
 * WAKEUP_GPIO_NUM is set anyway for code paths that reference it
 * directly but is NOT armed as an EXT0/LP_IO wake source on the
 * P4 (see components/standby/standby.cpp). */
#define WAKEUP_GPIO_NUM     0

#elif defined(CONFIG_DRAFTLING_MODEL_SUNTON_8048S070)
/* ----- Sunton ESP32-8048S070C -----
 *
 * 7" TN color LCD, 800 x 480 native landscape, 16-bit parallel
 * RGB565 interface driven by the ESP32-S3 LCD RGB peripheral
 * (esp_lcd_new_rgb_panel). All panel data / control GPIOs are owned
 * by the RGB display backend (components/display/display_rgb.cpp)
 * and are NOT redefined here -- they are baked into the backend:
 *
 *   hsync=39, vsync=40, de=41, pclk=42, disp=-1, backlight=GPIO2,
 *   data[16] in B,G,R order (RGB565 LSB-first) =
 *     {15,7,6,5,4, 9,46,3,8,16,1, 14,21,47,48,45}
 *   (the physical wiring is R,G,B; esp_lcd maps data_gpio_nums[0]
 *    to the RGB565 LSB, so blue must come first or R/B swap).
 *   timings: pclk 12 MHz, hsync pw=2 bp=43 fp=8,
 *            vsync pw=2 bp=12 fp=8, pclk_idle_high=1.
 *
 * Pins below are the ones Draftling touches outside the display
 * backend (SD card, I2C touch, wakeup button). References:
 *   https://www.haraldkreuzer.net/aktuelles/erste-schritte-mit-dem-
 *     sunton-esp32-s3-7-zoll-display-lovyangfx-und-lvgl
 *   https://www.makerfabs.com/sunton-esp32-s3-7-inch-tn-display-with-touch.html
 */
#define BOARD_NAME      "Sunton ESP32-8048S070C"

/* On-board MicroSD slot on a dedicated SPI bus (not the RGB bus).
 * Standard Sunton ESP32-8048S0xx pinout: CS=10, MOSI=11, MISO=13,
 * SCK=12. */
#define SD_SPI_MOSI_PIN 11
#define SD_SPI_MISO_PIN 13
#define SD_SPI_SCK_PIN  12
#define SD_SPI_CS_PIN   10
#define SD_EN_PIN       -1

/* I2C bus carrying the GT911 capacitive touch controller. Pins per
 * the LovyanGFX Sunton reference: SDA=19, SCL=20, RST=38, INT=NC. */
#define I2C_SDA_PIN     19
#define I2C_SCL_PIN     20

/* GT911 capacitive touch controller. Touch is optional on this
 * board (Draftling is keyboard-driven); these are kept for a future
 * touch enable. The panel is natively landscape 800x480 so no axis
 * swap is needed. INT is not wired on the Sunton reference. */
#define TOUCH_I2C_ADDR  0x5D
#define TOUCH_INT_PIN   CONFIG_DRAFTLING_TOUCH_INT_GPIO
#define TOUCH_RST_PIN   CONFIG_DRAFTLING_TOUCH_RST_GPIO
#define TOUCH_NATIVE_W  800
#define TOUCH_NATIVE_H  480
#define TOUCH_SWAP_XY   0
#define TOUCH_MIRROR_X  0
#define TOUCH_MIRROR_Y  0

/* No on-board battery monitor on this dev board. */
#define BATT_ADC_PIN    -1
#define BATT_EN_PIN     -1
#define BATT_DIVIDER    1

/* Deep-sleep wakeup on the BOOT button (GPIO0, active-low strapping
 * pin with a board-level pull-up; RTC-capable so EXT0 wake works). */
#define WAKEUP_GPIO_NUM 0

#elif defined(CONFIG_DRAFTLING_MODEL_SUNTON_8048S043)
/* ----- Sunton ESP32-8048S043C -----
 *
 * 4.3" IPS color LCD, 800 x 480 native landscape, ST7262 panel on a
 * 16-bit parallel RGB565 interface driven by the ESP32-S3 LCD RGB
 * peripheral (esp_lcd_new_rgb_panel). All panel data / control GPIOs
 * are owned by the RGB display backend (components/display/
 * display_rgb.cpp) and are NOT redefined here -- they are baked into
 * the backend's S043 block (gated on CONFIG_DRAFTLING_RGB_BOARD_S043):
 *
 *   hsync=39, vsync=41, de=40, pclk=42, disp=-1, backlight=GPIO2,
 *   data[16] in R,G,B order (RGB565 LSB-first) =
 *     {8,3,46,9,1, 5,6,7,15,16,4, 45,48,47,21,14}
 *   timings: pclk 12.5 MHz, hsync pw=4 bp=8 fp=8,
 *            vsync pw=4 bp=8 fp=8, pclk_active_neg=1.
 *
 * The control-pin map (VSYNC/DE swapped), panel timings and data-line
 * order all differ from the 7" ESP32-8048S070C; everything Draftling
 * touches itself (SD card, I2C touch, wakeup button) is the same as
 * the 7" board's standard Sunton ESP32-8048S0xx pinout. Verified
 * against the rzeldent/platformio-espressif32-sunton board reference.
 */
#define BOARD_NAME      "Sunton ESP32-8048S043C"

/* On-board MicroSD slot on a dedicated SPI bus (not the RGB bus).
 * Standard Sunton ESP32-8048S0xx pinout: CS=10, MOSI=11, MISO=13,
 * SCK=12. */
#define SD_SPI_MOSI_PIN 11
#define SD_SPI_MISO_PIN 13
#define SD_SPI_SCK_PIN  12
#define SD_SPI_CS_PIN   10
#define SD_EN_PIN       -1

/* I2C bus carrying the GT911 capacitive touch controller. Pins per
 * the Sunton reference: SDA=19, SCL=20, RST=38, INT=NC. */
#define I2C_SDA_PIN     19
#define I2C_SCL_PIN     20

/* GT911 capacitive touch controller. Touch is optional on this
 * board (Draftling is keyboard-driven); these are kept for a future
 * touch enable. The panel is natively landscape 800x480 so no axis
 * swap is needed. INT is not wired on the Sunton reference. */
#define TOUCH_I2C_ADDR  0x5D
#define TOUCH_INT_PIN   CONFIG_DRAFTLING_TOUCH_INT_GPIO
#define TOUCH_RST_PIN   CONFIG_DRAFTLING_TOUCH_RST_GPIO
#define TOUCH_NATIVE_W  800
#define TOUCH_NATIVE_H  480
#define TOUCH_SWAP_XY   0
#define TOUCH_MIRROR_X  0
#define TOUCH_MIRROR_Y  0

/* No on-board battery monitor on this dev board. */
#define BATT_ADC_PIN    -1
#define BATT_EN_PIN     -1
#define BATT_DIVIDER    1

/* Deep-sleep wakeup on the BOOT button (GPIO0, active-low strapping
 * pin with a board-level pull-up; RTC-capable so EXT0 wake works). */
#define WAKEUP_GPIO_NUM 0

#else
#error "No hardware model selected. Run idf.py menuconfig and choose a model."
#endif

/* Default: no deep-sleep-only BL cut pin. Boards whose BL cannot
 * be cut via the normal LCD_BL_PIN path (because the BL boost
 * circuit does not tolerate the ESP32 driving the BL enable pin
 * during normal operation) may override this to drive a separate
 * pin LOW only at deep-sleep entry. No current board uses this
 * fallback; kept available for future boards that need it. */
#ifndef LCD_BL_DEEP_SLEEP_CUT_PIN
#define LCD_BL_DEEP_SLEEP_CUT_PIN   -1
#endif

/* Default: BL pin is active HIGH (LEDC duty MAX = full brightness,
 * duty 0 = off). Boards whose BL boost circuit is active LOW
 * (LEDC duty 0 = full brightness, duty MAX = off) override this
 * to 1 -- see the Waveshare ESP32-S3-Touch-LCD-3.49 block above. */
#ifndef LCD_BL_ACTIVE_LOW
#define LCD_BL_ACTIVE_LOW   0
#endif
