#pragma once
/* ----- M5Stack PaperMono / PaperMono-Lite -----
 *
 * https://docs.m5stack.com/en/core/PaperMono-Lite -- ESP32-S3R8
 * (embedded octal 8 MB PSRAM), 16 MB flash, 3.97" 800x480 e-paper
 * (SSD1677) with front-light and FT6336G capacitive touch. The full
 * PaperMono adds an ST25R3916 NFC reader and a Stamp LoRa-1262
 * module; Draftling uses neither, so one build serves both boards
 * (the NFC chip idles on the shared I2C bus, the LoRa module stays
 * unpowered behind the M5PM1's LoRa_EN output, which is never
 * enabled).
 *
 * EXPERIMENTAL: added without on-hardware testing. Pin numbers come
 * from M5Stack's pin map for both boards; the power-up sequence and
 * the M5PM1 / M5IOE1 register maps come from M5Stack's MIT-licensed
 * M5GFX (board autodetect for board_M5PaperMono), M5PM1, M5IOE1 and
 * M5PaperMono-PowerDemo sources.
 *
 * E-paper panel SPI bus (SCLK=15, MOSI=14, CS=16, DC=17, BUSY=18) is
 * hard-coded inside components/display/display_m5_papermono.cpp,
 * matching the other single-board e-paper backends. The panel has no
 * GPIO reset line: RST and its 3.3 V supply sit on the M5IOE1
 * expander (EPD_RST_IOE_PIN / EPD_EN_IOE_PIN below), driven by
 * main.cpp before display_init().
 *
 * Included by main/app_config.h when
 * CONFIG_DRAFTLING_MODEL_M5STACK_PAPERMONO is selected.
 */

#define BOARD_NAME      "M5Stack PaperMono"

/* Shared system I2C bus: M5PM1 power management @0x6E, M5IOE1 IO
 * expander @0x4F, FT6336G touch @0x38, plus devices Draftling leaves
 * alone (RX8130CE RTC @0x32, BMI270 IMU @0x68, IP2315 charger @0x75,
 * and on the full PaperMono the ST25R3916 NFC reader @0x50). The
 * M5PM1 and M5IOE1 addresses are fixed inside their drivers
 * (components/battery/battery.cpp, components/io_expander/). */
#define I2C_SDA_PIN     47
#define I2C_SCL_PIN     48

/* M5IOE1 expander pins, numbered as M5Stack's pin map does (PYG1 ..
 * PYG14; the driver maps pin n to register bit n - 1). */
#define EPD_EN_IOE_PIN      3   /* e-paper 3.3 V supply enable */
#define EPD_RST_IOE_PIN     5   /* e-paper RST, active low */
#define TOUCH_RST_IOE_PIN   6   /* FT6336G RST, active low */
#define TOUCH_EN_IOE_PIN    13  /* FT6336G VDD enable */
#define SD_EN_IOE_PIN       14  /* MicroSD supply enable */

/* Battery voltage comes from the M5PM1 (DRAFTLING_BATTERY_M5PM1); no
 * ADC divider on a GPIO. */
#define BATT_ADC_PIN    -1
#define BATT_EN_PIN     -1
#define BATT_DIVIDER    1

/* On-board MicroSD, SDMMC. The board wires a full 4-bit bus (D1=10,
 * D2=9, D3=8), but components/sd_card/ runs 1-bit mode only; D1-D3
 * stay unconfigured. The card is powered through SD_EN_IOE_PIN. */
#define SD_CLK_PIN      13
#define SD_CMD_PIN      12
#define SD_D0_PIN       11

/* FT6336G capacitive touch (driven by the FT6336U backend; same
 * register protocol). Its reset and VDD enable are on the M5IOE1
 * (TOUCH_RST_IOE_PIN / TOUCH_EN_IOE_PIN), so there is no GPIO reset.
 *
 * The digitizer reports a 480x800 portrait frame: M5Stack's own demos
 * compare raw touch points directly against their portrait (480x800)
 * UI. M5GFX shows that portrait frame on this panel with
 * offset_rotation 3, which puts portrait (x, y) at panel-RAM
 * (X = y, Y = 479 - x). In the touchscreen component's terms: raw
 * frame 480x800, mirror X (x -> 479 - x) and then swap axes. */
#define TOUCH_I2C_ADDR      0x38
#define TOUCH_INT_PIN       CONFIG_DRAFTLING_TOUCH_INT_GPIO
#define TOUCH_RST_PIN       -1
#define TOUCH_NATIVE_W      480
#define TOUCH_NATIVE_H      800
#define TOUCH_SWAP_XY       1
#define TOUCH_MIRROR_X      1
#define TOUCH_MIRROR_Y      0

/* Two active-low user buttons with external pull-ups, both
 * RTC-capable. Button A is the deep-sleep wake source and, via the
 * generic wakeup_btn_init() poller, forgets all BLE keyboards on a
 * 2 s hold. papermono_btn_init() in main.cpp turns short presses into
 * Page Up (A) / Page Down (B); a 2 s hold of B enters deep sleep
 * (CONFIG_DRAFTLING_SLEEP_BUTTON_GPIO). The power button is handled
 * by the M5PM1 itself (power on, reset, power off). */
#define BTN_A_PIN        2
#define BTN_B_PIN        3
#define WAKEUP_GPIO_NUM  2
