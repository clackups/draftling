#pragma once
/* ----- Seeed Studio reTerminal Sticky -----
 *
 * https://www.seeedstudio.com/sticky/docs/en/device-guide/hardware-overview/
 * -- ESP32-S3R8 (embedded octal 8 MB PSRAM), 32 MB flash, driving a
 * 3.97" 800x480 SSD1677 black/white e-paper panel over SPI, with a
 * GT911 capacitive touch controller on its own I2C bus and a BQ27220
 * fuel gauge on a second, separate I2C bus. Unlike every other
 * SSD1677 board in this repo, the MicroSD card is wired onto the
 * SAME SPI bus as the e-paper panel (shared SCLK/MOSI/MISO, separate
 * CS) rather than a dedicated bus.
 *
 * Pin assignments are triple-sourced (per the FreeInk SDK's own
 * comment for this device): Seeed's official hardware-overview page
 * above, and the FreeInk SDK
 * (https://github.com/Free-Ink/freeink-sdk, MIT licensed)
 * `libs/hardware/BoardConfig/include/BoardConfig.h`'s `STICKY`
 * BoardProfile, which cross-references a V01 schematic and the
 * vendor's own peripheral demo (`pin_config.h`). The two sources
 * agree on every pin used here. No source code was copied from
 * FreeInk -- only these factual pin assignments (the same
 * facts-only treatment already used for the Xteink X4 Pro / Classic
 * and the Elecrow CrowPanel 5.79" board headers).
 *
 * Tested on physical hardware. FreeInk's own comment for this device
 * lists several "pending hardware validation" items (panel mount
 * orientation, MicroSD bus-sharing behavior, PDM mic pins). Where
 * FreeInk's driver made a Sticky-specific choice this port did not
 * take over (e.g. the full-refresh waveform), this port instead
 * reuses whatever the *Waveshare ESP32-S3-ePaper-3.97*
 * backend settled on after real hardware testing, since that board
 * carries the exact same SSD1677-family 800x480 panel class -- see
 * components/display/display_reterminal_sticky.cpp for details.
 *
 * Included by main/app_config.h when
 * CONFIG_DRAFTLING_MODEL_SEEED_RETERMINAL_STICKY is selected.
 */

#define BOARD_NAME      "Seeed reTerminal Sticky"

/* Power-rail latch: PWR_HOLD (GPIO45) and PWR_LOCK (GPIO46) must both
 * be driven HIGH first thing in boot or the board's own power switch
 * drops the rail as soon as the case button is released (FreeInk:
 * "Power-rail latch pins a battery-powered board must drive HIGH
 * early in boot to keep itself on"). Neither pin is RTC-capable
 * (outside the ESP32-S3's GPIO0-21 RTC-IO range), so without an
 * explicit gpio_hold_en() + gpio_deep_sleep_hold_en() the pad would
 * lose its driven level -- and therefore the whole board's power --
 * the instant deep sleep powers down the digital IOMUX domain. main.cpp
 * re-arms the hold at every boot and re-latches it before every deep
 * sleep. Releasing these pins instead of holding them would be a real
 * software power-off; Draftling deliberately never does that (deep
 * sleep is this board's "off" state, matching every other board in
 * this repo) so the capability is unused but documented here for
 * anyone who wants it later. */
#define PWR_HOLD_PIN    45
#define PWR_LOCK_PIN    46

/* Battery charger enable, EN_BAT_CHGn -> BQ25616 /CE, active-LOW.
 * GPIO39 is in the ESP32-S3's JTAG pin group and resets with a weak
 * pull-up, which leaves /CE high (charging disabled) for as long as
 * the firmware is running unless driven LOW explicitly -- FreeInk
 * measured this as the difference between ~0.06 A USB input while
 * awake (charging effectively disabled) and ~0.5 A once deep sleep
 * isolates the pad and lets the line float back to its enabled
 * default. Driven LOW at boot and held (gpio_hold_en) through every
 * deep sleep so the charger stays enabled both awake and asleep. */
#define CHARGE_EN_PIN   39

/* E-paper panel SPI bus (SCLK=13, MOSI=14, MISO=12, CS=15, DC=16,
 * RST=17, BUSY=18) and its power-enable pin (EP_PWR_EN=47) are
 * hard-coded directly inside display_reterminal_sticky.cpp, matching
 * the existing convention used by display_ws_epd397.cpp /
 * display_xteink_epd.cpp: this backend is used by exactly one board,
 * so there is no per-SKU variation to carry through a board header.
 * Unlike every other single-board SSD1677 backend in this repo, that
 * SPI bus is also shared with the MicroSD card below (same physical
 * SCLK/MOSI/MISO net, separate CS) -- see SD_SPI_* below and
 * SD_SPI_HOST_NUM. */

/* GT911 capacitive touch controller on its own I2C bus (I2C_NUM_0),
 * physically separate from the fuel-gauge bus below. I2C_SDA_PIN /
 * I2C_SCL_PIN feed the generic touch bring-up in main.cpp
 * (touchscreen_config_t.sda/scl), so no board-specific main.cpp code
 * is needed for the bus itself -- just the power-enable poke below.
 * TOUCH_POWER_EN_PIN is active-HIGH (opposite polarity from the
 * Xteink X4 Pro's active-low GPIO2); driven HIGH once before
 * touchscreen_init() runs its own RST pulse and dual-address (0x5D /
 * 0x14) probe -- no address-select reset dance is done here, unlike
 * the X4 Pro, since this port has no hardware to verify one is
 * needed and the generic probe already covers both GT911 addresses. */
#define I2C_SDA_PIN         3
#define I2C_SCL_PIN         2
#define TOUCH_I2C_ADDR      0x5D
#define TOUCH_INT_PIN       CONFIG_DRAFTLING_TOUCH_INT_GPIO
#define TOUCH_RST_PIN       CONFIG_DRAFTLING_TOUCH_RST_GPIO
#define TOUCH_POWER_EN_PIN  42
/* Portrait digitizer mounted under a landscape panel: the GT911
 * reports a 480x800 portrait frame (raw X 0..479 along the panel's
 * short side, raw Y 0..799 along its long side). TOUCH_NATIVE_W/H
 * describe that raw, pre-swap frame -- the same convention as the
 * Xteink X4 Pro's portrait GT911 -- and swapXY + flip both maps it
 * onto the 800x480 panel frame. Sources that agree, all verified on
 * hardware by their authors: the FreeInk SDK Sticky profile
 * ("confirmed by corner + menu bring-up taps"; panel ranges X 0..799,
 * Y 0..479 after the swap), LowFlowIO/sticky-micronotes
 * (panel x = 799 - rawY, y = 479 - rawX, with the same SSD1677 RAM
 * addressing as display_reterminal_sticky.cpp) and ESPHome's
 * swap_xy-only config, whose display frame is this one rotated 180
 * degrees. Not yet confirmed on a unit by this port; if taps land
 * off, CONFIG_DRAFTLING_TOUCH_DEBUG_LOG prints raw and logical
 * coordinates (see components/touchscreen/include/touchscreen.h). */
#define TOUCH_NATIVE_W      480
#define TOUCH_NATIVE_H      800
#define TOUCH_SWAP_XY       1
#define TOUCH_MIRROR_X      1
#define TOUCH_MIRROR_Y      1

/* BQ27220 fuel gauge on its OWN I2C bus (I2C_NUM_1, SDA=GPIO1,
 * SCL=GPIO0), physically separate from the GT911 touch bus above --
 * unlike every other BQ27220 board in this repo (LilyGO T5 Pro),
 * where the gauge shares the display/touch shared bus. main.cpp
 * creates this second bus itself (see the
 * CONFIG_DRAFTLING_MODEL_SEEED_RETERMINAL_STICKY block near the
 * battery-init call) since no other consumer needs it. GPIO0 is an
 * ESP32-S3 strapping pin; the bus is only created well after boot
 * strapping has latched, so this is safe. No I2C charger on this bus
 * (the BQ25616 charger's only host-visible control is the GPIO
 * CHARGE_EN_PIN above), so battery_read_charging() always reports
 * "unknown" for the fuel-gauge backend -- charge state is instead
 * surfaced only in the sense that CHARGE_EN_PIN keeps charging
 * enabled; Draftling has no separate UI signal for it on this board. */
#define GAUGE_I2C_SDA_PIN   1
#define GAUGE_I2C_SCL_PIN   0
#define BATT_ADC_PIN        -1
#define BATT_EN_PIN         -1
#define BATT_DIVIDER        1

/* On-board MicroSD, plain SPI, sharing the e-paper panel's bus
 * (SCLK=13/MOSI=14/MISO=12 above, separate CS=8). SD_EN_PIN=10 is
 * active-high and handled entirely by the generic sd_card_init_spi()
 * power-enable path in components/sd_card/sd_card.cpp -- no
 * board-specific main.cpp code needed. Bus sharing with the display
 * is asserted by both Seeed's hardware-overview page and the FreeInk
 * SDK, but neither source's own authors have exercised SD traffic on
 * real hardware yet (FreeInk: "SD bus-sharing is inferred ... verify
 * CS/transactions don't collide with the panel on hardware"). */
#define SD_SPI_MOSI_PIN     14
#define SD_SPI_MISO_PIN     12
#define SD_SPI_SCK_PIN      13
#define SD_SPI_CS_PIN       8
#define SD_EN_PIN           10
/* The display backend owns this physical bus as SPI2_HOST (see
 * display_reterminal_sticky.cpp); override app_config.h's SPI3_HOST
 * default so the SD driver attaches to the same host instead of
 * trying to stand up a second bus on the same pins. */
#define SD_SPI_HOST_NUM     SPI2_HOST

/* Up / Down are dedicated page-turn buttons (Seeed's own product
 * copy: "next/previous page buttons"), active-low with internal
 * pull-ups -- see reterminal_sticky_btn_init() in main.cpp, which
 * injects Page Up / Page Down exactly like the Xteink X4 Pro's
 * Left/Right buttons.
 *
 * The single Power/AI button (GPIO4) is both WAKEUP_GPIO_NUM and, in
 * FreeInk's own vendor-firmware-derived description, a shared
 * confirm/power control ("click confirms, hold sleeps"). Draftling
 * does not replicate that click-to-confirm semantic (Enter is already
 * reachable via a BLE keyboard or a touch tap, since this board has
 * touch); instead it reuses the existing Xteink X4 Pro convention for
 * a single "Power"-labeled button on a board with no hardware
 * power-off latch in active use: a short press enters deep sleep
 * directly (see the CONFIG_DRAFTLING_MODEL_XTEINK_X4_PRO arm of
 * wakeup_btn_poll_cb() in main.cpp, extended to this board too), and
 * a 2 s hold forgets every stored BLE keyboard bond. */
#define BTN_UP_PIN          5
#define BTN_DOWN_PIN        6
#define WAKEUP_GPIO_NUM     4
