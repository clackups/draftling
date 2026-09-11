#pragma once
/* ----- Waveshare ESP32-S3-ePaper-3.97 -----
 *
 * https://docs.waveshare.com/ESP32-S3-ePaper-3.97 -- ESP32-S3R8
 * (embedded octal 8 MB PSRAM), external 16 MB flash, driving a 3.97"
 * 800x480 e-paper panel over plain SPI through a single panel
 * controller (no manufacturing-run variance to detect at boot, unlike
 * the Xteink X4 Pro). No touch controller; four discrete active-low
 * tactile buttons (Up/Function/Down plus BOOT) stand in for it.
 *
 * Pin numbers and the AXP2101 PMIC register map used by this board
 * were read out of Waveshare's official ESP-IDF example
 * (github.com/waveshareteam/ESP32-S3-ePaper-3.97). That repository
 * carries no LICENSE file / license grant, so no source from it was
 * copied anywhere in this port -- only these factual pin/register
 * assignments were used, the same way the Elecrow CrowPanel 5.79"
 * board header treats its vendor's Eagle schematic as facts-only.
 * This board has NOT been tested on physical hardware.
 *
 * E-paper panel SPI bus (SCLK=11, MOSI=12, CS=10, DC=9, RST=46,
 * BUSY=3) is hard-coded directly inside
 * components/display/display_ws_epd397.cpp, matching the existing
 * convention used by display_ili9341.cpp / display_xteink_epd.cpp /
 * display_ssd1683.cpp: this backend is used by exactly one board, so
 * there is no per-SKU variation to carry through a board header.
 *
 * Included by main/app_config.h when
 * CONFIG_DRAFTLING_MODEL_WAVESHARE_EPAPER_397 is selected.
 */

#define BOARD_NAME      "Waveshare ESP32-S3-ePaper-3.97"

/* Shared I2C bus: AXP2101 PMIC only (also present on this bus per the
 * vendor SDK, but unused by Draftling: PCF85063 RTC @0x51, SHTC3
 * temp/humidity @0x70, QMI8658 IMU @0x6A/0x6B, ES8311 audio codec
 * @0x18). The AXP2101's I2C address (0x34) is hard-coded inside
 * components/battery/battery.cpp, matching the CW2017's fixed-address
 * convention. */
#define I2C_SDA_PIN     41
#define I2C_SCL_PIN     42

/* AXP2101 PMIC: no charger IC elsewhere on this bus, and it also
 * switches the e-paper panel's own supply rail (ALDO3) -- see
 * components/display/display_ws_epd397.cpp's file header comment and
 * battery_axp2101_enable_display_rail() in components/battery/battery.cpp.
 * No ADC-divider fallback on this board. */
#define BATT_ADC_PIN    -1
#define BATT_EN_PIN     -1
#define BATT_DIVIDER    1

/* On-board MicroSD, SDMMC. The vendor wires a full 4-bit bus
 * (D1=GPIO7, D2=GPIO8, D3=GPIO18 in addition to the pins below), but
 * components/sd_card/sd_card.cpp only ever runs 1-bit mode
 * (CLK/CMD/D0), matching every other SDMMC board in this repo -- the
 * extra D1-D3 lines are simply left unconfigured. No SD power-enable
 * line is documented for this board. */
#define SD_CLK_PIN      16
#define SD_CMD_PIN      17
#define SD_D0_PIN       15

/* Four discrete active-low tactile buttons (plain GPIO inputs, not a
 * quadrature rotary encoder -- confirmed from the vendor SDK, which
 * reads them through a generic multi-button library despite
 * Waveshare's marketing copy calling one of them a "rotary button").
 * Up/Function/Down are wired into editor navigation (arrow keys /
 * Enter) by ws_epaper397_nav_init() in main.cpp, mirroring the
 * Elecrow CrowPanel 5.79"'s Back/dial-switch convention. BOOT
 * (WAKEUP_GPIO_NUM) is the standard deep-sleep wake / BLE-forget
 * button every other board already gets via wakeup_btn_init() --
 * no special handling needed here. All four are RTC-capable GPIOs. */
#define BTN_UP_PIN       4
#define BTN_FUNCTION_PIN 5
#define BTN_DOWN_PIN     6
#define WAKEUP_GPIO_NUM  0
