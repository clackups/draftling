#pragma once
/* ----- Xteink X4 Classic (also marketed "X4 v2") -----
 *
 * ESP32-S3 e-reader, the buttons-only sibling of the Xteink X4 Pro
 * (boards/xteink_x4_pro.h). It shares the X4 Pro's ESP32-S3 module,
 * the 4.26" 800x480 e-paper glass and the whole
 * components/display/display_xteink_epd.cpp controller stack (the
 * panel still ships with one of SSD1677 / UC8179 / UC8279 depending
 * on the production run, auto-detected at boot), plus the CW2017 I2C
 * fuel gauge and the SDMMC 1-bit MicroSD slot.
 *
 * What it does NOT have, compared with the X4 Pro:
 *   - no GT911 capacitive touch  (CONFIG_DRAFTLING_TOUCHSCREEN off)
 *   - no front-light             (CONFIG_DRAFTLING_DISPLAY_HAS_BACKLIGHT off)
 * The GPIOs the X4 Pro spends on touch power-enable (GPIO2) and the
 * warm/cool front-light PWM (GPIO8/GPIO9) are wired to four extra
 * front buttons here instead, and the panel's DC/RST/BUSY lines land
 * on different GPIOs than the X4 Pro (SCLK/MOSI/CS are the same) --
 * see components/display/display_xteink_epd.cpp.
 *
 * Pin assignments come from the FreeInk SDK
 * (https://github.com/Free-Ink/freeink-sdk, MIT licensed),
 * docs/xteink-x4c-support.md, which reverse-engineered the OEM
 * firmware. This board has NOT been tested on physical hardware; see
 * HARDWARE.md.
 *
 * Included by main/app_config.h when
 * CONFIG_DRAFTLING_MODEL_XTEINK_X4_CLASSIC is selected.
 */

#define BOARD_NAME      "Xteink X4 Classic"

/* Master peripheral-rail latch, driven HIGH once at boot (same as the
 * X4 Pro's XTEINK_POWER_LATCH_PIN -- GPIO1 is the panel/SD/peripheral
 * power rail; without it the e-paper and SD slot stay unpowered).
 * Not a battery/MCU power switch: there is no hardware latch that can
 * fully power this board off from software, so a Power-button press
 * enters deep sleep (see the generic wakeup handling in main.cpp). */
#define XTEINK_POWER_LATCH_PIN 1

/* E-paper panel SPI bus (SCLK=12, MOSI=11, CS=13) and its control
 * lines (DC=14, RST=10, BUSY=18 -- different GPIOs from the X4 Pro)
 * are hard-coded inside components/display/display_xteink_epd.cpp,
 * matching the convention of display_ili9341.cpp / display_h752.cpp:
 * that backend is used by exactly the X4 Pro and X4 Classic, and it
 * picks the pin set at build time on the model symbol. */

/* Shared I2C bus: CW2017 fuel gauge (0x63). The OEM also wires a
 * BM8563 RTC (0x51) and a QMI8658 IMU (0x6B) here; Draftling uses
 * neither. */
#define I2C_SDA_PIN     39
#define I2C_SCL_PIN     38

/* CW2017 fuel gauge, on the shared I2C bus above. No charger IC on
 * this bus, so charging state is reported as unknown. */
#define CW2017_I2C_ADDR 0x63
#define BATT_ADC_PIN    -1
#define BATT_EN_PIN     -1
#define BATT_DIVIDER    1

/* On-board MicroSD, SDMMC 1-bit mode. Power-enable line (active-low)
 * on GPIO6 -- note this differs from the X4 Pro (GPIO5, which is a
 * button here). main.cpp power-cycles it (HIGH ~80 ms, then LOW and
 * held LOW while mounted) before sd_card_init(). */
#define SD_CLK_PIN      41
#define SD_CMD_PIN      42
#define SD_D0_PIN       40
#define SD_POWER_EN_PIN 6

/* Eight discrete buttons, all active-low on RTC-capable GPIOs, read
 * with the ESP32-S3 internal pull-ups. Draftling drives the editor
 * entirely from these (no touch, and no keyboard until BLE pairs) --
 * see xteink_x4_classic_*_init() in main.cpp:
 *
 *   Power  (GPIO3)  deep-sleep wake source; short press = F1
 *                   (open/close Settings), 2 s hold = forget BLE
 *                   keyboards -- same convention as the CrowPanel
 *                   5.79" Menu button.
 *   Left   (GPIO0)  side key   -> Up arrow      (GPIO0 is a boot
 *                                strap pin; fine as long as it is
 *                                not held during reset)
 *   Right  (GPIO7)  side key   -> Down arrow
 *   B.Left (GPIO5)  bottom key -> Left arrow
 *   B.Right(GPIO2)  bottom key -> Right arrow
 *   Confirm(GPIO8)  bottom key -> Enter
 *   Back   (GPIO9)  bottom key -> Esc
 *
 * (GPIO4 is a further discrete input on the OEM board that the SDK
 * does not use; Draftling ignores it too.) */
#define BTN_LEFT_PIN         0
#define BTN_RIGHT_PIN        7
#define BTN_BOTTOM_LEFT_PIN  5
#define BTN_BOTTOM_RIGHT_PIN 2
#define BTN_CONFIRM_PIN      8
#define BTN_BACK_PIN         9
#define WAKEUP_GPIO_NUM      3
