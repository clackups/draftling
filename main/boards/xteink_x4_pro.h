#pragma once
/* ----- Xteink X4 Pro -----
 *
 * 4.26" 800x480 e-paper panel driven over plain SPI. Different
 * manufacturing runs ship one of three panel controllers -- SSD1677,
 * or one of two UltraChip parts (UC8179 / UC8279) -- which cannot be
 * told apart from the outside; the display backend
 * (components/display/display_xteink_epd.cpp) probes the controller
 * over the bus at boot and drives whichever one is present.
 *
 * Pin assignments, register sequences and waveform timing come from
 * the FreeInk SDK (https://github.com/Free-Ink/freeink-sdk, MIT
 * licensed), which reverse-engineered the OEM firmware's exact
 * register values. This board has NOT been tested on physical
 * hardware (same caveat as the M5Stack Tab5 entry in HARDWARE.md).
 *
 * Included by main/app_config.h when
 * CONFIG_DRAFTLING_MODEL_XTEINK_X4_PRO is selected.
 */

#define BOARD_NAME      "Xteink X4 Pro"

/* Master peripheral-rail latch. Must be driven HIGH before any
 * SPI/display/SD bring-up -- without it, the panel rail and the SD
 * slot both stay unpowered. Driven directly in main.cpp (no I2C
 * expander involved, unlike the components/power TCA9554 latch used
 * by the Waveshare Touch-LCD-3.49). */
#define XTEINK_POWER_LATCH_PIN 1

/* E-paper panel SPI bus (SCLK=12, MOSI=11, CS=13, DC=18, RST=14,
 * BUSY=6) and the dual-channel front-light PWM pins (cool=8, warm=9)
 * are hard-coded directly inside components/display/display_xteink_epd.cpp,
 * matching the existing convention of display_ili9341.cpp /
 * display_h752.cpp: this backend is used by exactly one board, so
 * there is no per-SKU variation to carry through a board header. */

/* Shared I2C bus: GT911 capacitive touch and CW2017 fuel gauge. */
#define I2C_SDA_PIN     39
#define I2C_SCL_PIN     38

/* GT911 capacitive touch controller. A dedicated power-enable line
 * (active-low) must be driven low before touch bring-up; there is no
 * touchscreen_config_t field for this, so main.cpp drives it directly
 * as a one-off GPIO poke (same style as the LilyGO LoRa-CS drive).
 *
 * Native orientation (swap_xy=1, mirror_y=1) is carried over from the
 * FreeInk SDK's GT911 config for this board (swapXY=true, flipY=true)
 * as a best-effort starting point -- like every other new touch
 * board, it may need a dial-in pass with
 * CONFIG_DRAFTLING_TOUCH_DEBUG_LOG on real hardware. */
#define TOUCH_I2C_ADDR      0x5D
#define TOUCH_INT_PIN       CONFIG_DRAFTLING_TOUCH_INT_GPIO
#define TOUCH_RST_PIN       CONFIG_DRAFTLING_TOUCH_RST_GPIO
#define TOUCH_POWER_EN_PIN  2
#define TOUCH_NATIVE_W      480
#define TOUCH_NATIVE_H      800
#define TOUCH_SWAP_XY       1
#define TOUCH_MIRROR_X      0
#define TOUCH_MIRROR_Y      1

/* CW2017 fuel gauge, on the shared I2C bus above. No charger IC on
 * this bus, so charging state is reported as unknown. */
#define CW2017_I2C_ADDR 0x63
#define BATT_ADC_PIN    -1
#define BATT_EN_PIN     -1
#define BATT_DIVIDER    1

/* On-board MicroSD, SDMMC 1-bit mode. Power-enable line (active-low)
 * driven directly in main.cpp before sd_card_init(), same style as
 * TOUCH_POWER_EN_PIN above. */
#define SD_CLK_PIN      41
#define SD_CMD_PIN      42
#define SD_D0_PIN       40
#define SD_POWER_EN_PIN 5

/* Left/Right buttons (active-low) inject Page Up / Page Down into the
 * editor -- see xteink_x4_pro_btn_init() in main.cpp. Power (active-low)
 * is the standard deep-sleep wake / BLE-forget button, same as BOOT on
 * every other board. All three are RTC-capable GPIOs. */
#define BTN_LEFT_PIN    0
#define BTN_RIGHT_PIN   7
#define WAKEUP_GPIO_NUM 3
