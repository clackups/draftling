#pragma once
/* ----- OnePage Reader (ESP32-C61), board revision V1 2026-08-19 -----
 *
 * Open-hardware e-reader (github.com/MoveCall/onepage-reader):
 * ESP32-C61HR2 (single-core RISC-V, WiFi 6, BLE, 2 MB in-package quad
 * PSRAM), 16 MB PY25Q128 flash, an 800x480 SSD1677 e-paper panel (the
 * Xteink X4 panel, on a 24-pin FPC), a MicroSD slot sharing the
 * panel's SPI bus, a PDM microphone, an LGS4056 Li-ion charger and a
 * USB-C port wired to the chip's USB-Serial/JTAG.
 *
 * Sources: the vendor's MIT-licensed Arduino board support
 * (github.com/MoveCall/onepage-reader-sdk-arduino: InputManager,
 * SDCardManager, EInkDisplay) and its CrossPoint port
 * (github.com/MoveCall/crosspoint-onepage: lib/hal/HalGPIO.h,
 * HalGPIO.cpp, HalPowerManager.cpp). Only pin numbers and the power
 * sequencing facts below were taken from them.
 * Tested on physical hardware.
 *
 * Included by main/app_config.h when
 * CONFIG_DRAFTLING_MODEL_ONEPAGE_C61 is selected.
 */

#define BOARD_NAME          "OnePage Reader"

/* SSD1677 e-paper panel on SPI2 (display_ssd1677_spi.cpp). The bus is
 * shared with the MicroSD card, so MISO is part of it.
 *
 * EPD_RST_PIN (GPIO27) is BOTH the panel's reset line and the enable
 * of the supply rail of the MicroSD card and the microphone (HIGH =
 * powered, external pull-down). It is driven HIGH early in app_main(),
 * pulsed once by display_init() before the card is mounted, and
 * driven LOW and held through deep sleep by the pre-sleep hook, which
 * first parks the SPI and microphone lines LOW so they cannot feed
 * the unpowered card through its protection diodes. The vendor
 * firmware notes that pulsing it while the card is mounted wedges the
 * card until a power cycle.
 *
 * The vendor runs the SD card at 20 MHz and lowered the drive strength
 * of SCLK / MOSI one step to tame ringing on the shared bus; main.cpp
 * does the same. */
#define EPD_SPI_HOST        SPI2_HOST
#define EPD_SCLK_PIN        22
#define EPD_MOSI_PIN        23
#define EPD_MISO_PIN        24
#define EPD_CS_PIN          25
#define EPD_DC_PIN          8
#define EPD_RST_PIN         27
#define EPD_BUSY_PIN        29
#define EPD_SPI_CLOCK_HZ    (20 * 1000 * 1000)
/* RST is also the panel's supply here: hold it LOW long enough to
 * drain the rail and let it settle afterwards (vendor: 120 / 120 ms). */
#define EPD_RST_LOW_MS      120
#define EPD_RST_SETTLE_MS   120

/* MicroSD on the same bus (sd_card_init_spi() reuses the initialized
 * host). The rail is switched by EPD_RST_PIN, so no separate enable. */
#define SD_SPI_HOST_NUM     SPI2_HOST
#define SD_SPI_MISO_PIN     EPD_MISO_PIN
#define SD_SPI_MOSI_PIN     EPD_MOSI_PIN
#define SD_SPI_SCK_PIN      EPD_SCLK_PIN
#define SD_SPI_CS_PIN       26
#define SD_EN_PIN           -1

/* PDM microphone (unused): parked at sleep entry, see above. */
#define PDM_CLK_PIN         7
#define PDM_DIN_PIN         3

/* Battery: cell voltage through a 1:2 divider on GPIO5 (ADC1 channel
 * 3). The vendor pauses charging (CHARGE_EN_PIN LOW) while sampling so
 * the charger's terminal voltage does not inflate the reading; here
 * charging stays enabled and the reading is smoothed instead.
 * USB_DET_PIN is the LM66200 power mux's open-drain status output:
 * LOW while USB powers the board. */
#define BATT_ADC_PIN        5
#define BATT_EN_PIN         -1
#define BATT_DIVIDER        2
#define CHARGE_EN_PIN       10      /* HIGH = charging enabled */
#define USB_DET_PIN         11      /* LOW = USB present */

/* Buttons, all active-low.
 *
 * Power (GPIO2) is the deep-sleep wake source (EXT1; GPIO2 is an LP IO)
 * and F1 on a short press, sleep on a 2 s hold. The side keys (PREV /
 * NEXT) are Up / Down; a 2 s hold of PREV forgets the BLE keyboards.
 *
 * The four front keys share one ADC resistor ladder on GPIO4 (ADC1
 * channel 2, 12 dB attenuation); the voltage windows are the vendor's
 * (InputManager.cpp, measured: Back ~2592 mV, Left ~1956 mV, Right
 * ~1316 mV, Confirm ~0 mV, about 3.1 V with no key). They inject Esc /
 * Left / Right / Enter. */
#define WAKEUP_GPIO_NUM     2
#define BTN_UP_PIN          6
#define BTN_DOWN_PIN        9
#define BTN_LADDER_ADC_PIN  4
#define BTN_LADDER_BACK_MV_MIN     2400
#define BTN_LADDER_BACK_MV_MAX     2800
#define BTN_LADDER_LEFT_MV_MIN     1780
#define BTN_LADDER_LEFT_MV_MAX     2140
#define BTN_LADDER_RIGHT_MV_MIN    1140
#define BTN_LADDER_RIGHT_MV_MAX    1500
#define BTN_LADDER_CONFIRM_MV_MAX  250
