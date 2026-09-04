# Draftling: supported hardware

Draftling currently runs on a number of ESP32-S3 devices and
development boards, and and one ESP32-P4 device).

See the [photo gallery](images/README.md) of some of supported
hardware.

All ESP32-S3 boards use at least 8 MB of PSRAM and 16 MB of flash, BLE
for the HID keyboard and 802.11 b/g/n Wi-Fi for Git sync. The Tab5
board uses 32 MB HEX-mode PSRAM on the ESP32-P4 and reaches the same
BLE / Wi-Fi functionality through its on-board ESP32-C6 co-processor
via ESP-Hosted-MCU (see above and `docs/tab5-esp-hosted.md`).

All of them share the same firmware source; the target board is picked
at build time by selecting a preset. Display resolution, driver, pin
map, touch controller and the deep-sleep wake source are derived
automatically from that choice.


## Waveshare ESP32-S3-RLCD-4.2

[Waveshare
ESP32-S3-RLCD-4.2](https://www.waveshare.com/wiki/ESP32-S3-RLCD-4.2)
has a 4.2" 400x300 reflective LCD display. No touch. Battery monitored
on GPIO4 (3:1 divider). GPIO18 button wakes from deep sleep. On-board
MicroSD on the SDMMC 1-bit peripheral.

The device provides a pretty smooth and responsive user
interaction. But the screen is very fragile (the screen broke during
my tests), and the device needs an enclosure. A [simplistic 3D-printed
enclosure is available](3D_Prints/Waveshare_ESP32-S3-RLCD-4.2/). Also,
[Simon Shimel has designed Whale Writer, a foldable enclosure with an
attached keyboard](https://github.com/shmimel/whale-writer). The
contrast is very low, so it needs good lighting for comfortable work.


## LilyGO T5 E-Paper S3 Pro and Lite

[LilyGO T5 E-Paper S3
Pro](https://github.com/Xinyuan-LilyGO/T5S3-4.7-e-paper-PRO) -- 4.7"
e-paper ED047TC1 (960x540) driven over a parallel bus by
`vroland/epdiy` (`epd_board_v7`). GT911 capacitive touch on I2C.
BQ27220 fuel gauge on I2C (0x55). BOOT (GPIO0) wakes from deep
sleep. On-board MicroSD on SPI3 (shared with the SX1262 LoRa CS).

The **LilyGO T5 E-Paper S3 Pro** and **Pro Lite** are so far the most
usable option: they come with a high-contrast 4.7" ED047TC1 e-paper
panel with controllable white front-light, GT911 capacitive touch and
a MicroSD slot. Reaction is significantly slower than with RLCD, but
still acceptable. They are driven through the open-source
[`vroland/epdiy`](https://github.com/vroland/epdiy) library
(`epd_board_v7` configuration with a TPS65185 PMIC). The Pro variant
adds a SX1262 LoRa radio, an L76K GPS, an IR LED, a vibration motor
and an external 18650 holder; from Draftling's perspective the Lite is
functionally a Pro with those modules depopulated. Touch is enabled by
default on both SKUs (the on-board I2C bus is shared between epdiy and
the touchscreen component via the pinned post-2.0.0 epdiy commit's
`epd_init_with_config()` entry point). Battery state of charge comes
from the on-board BQ27220 fuel gauge over I2C (the
`components/battery/` `battery_init_bq27220()` backend, gated on
`CONFIG_DRAFTLING_BATTERY_BQ27220`); the editor status bar shows the
percentage. The on-board MicroSD slot shares its SPI bus with the LoRa
radio on the Pro; Draftling drives the LoRa chip-select HIGH at boot
so it does not interfere with SD traffic.

LilyGO T5 E-Paper S3 Pro
Lite](https://github.com/Xinyuan-LilyGO/T5S3-4.7-e-paper-PRO) is the
same as the Pro variant minus the SX1262 LoRa radio and MIA-M10Q GPS;
on-board MicroSD lives alone on SPI3.

## LilyGO T5 E-Paper S3 Pro(H752)

[LilyGO T5 E-Paper S3 Pro
(H752)](https://github.com/Xinyuan-LilyGO/T5S3-4.7-e-paper-PRO) is the
original pre-"H752-01" revision (v1.0-240810) of the LilyGO T5 E-Paper
S3 Pro. Same 4.7" ED047TC1 panel (960x540) and GT911 touch as the
current Pro / Pro Lite, but without the PCA9535 IO expander and
TPS65185 PMIC, so the panel is driven by the vendored FastEPD library
(`components/fastepd`) instead of `vroland/epdiy`. The side key on
GPIO48 acts as a Menu key (injects F1); GPIO48 is not an RTC IO, so
standby uses light sleep + `gpio_wakeup` + `esp_restart` rather than
EXT0 deep sleep. The capacitive touch key below the panel acts as Back
(injects Esc).


## M5Stack PaperS3

[M5Stack PaperS3](https://docs.m5stack.com/en/core/papers3) is a 4.7"
e-paper ED047TC1 (540x960) driven over a parallel I80 bus by the
`vroland/epdiy` managed component with a PaperS3-specific board
definition (`components/display/epd_board_papers3.c`).  GT911 touch on
I2C. Battery on GPIO3 ADC (1:2).  BOOT (GPIO0) wakes from deep sleep
(optionally any touch with
`CONFIG_DRAFTLING_STANDBY_WAKE_ON_TOUCH`). On-board MicroSD on
SPI3. **This board is officially discontinued by M5Stack.**

The M5Stack PaperS3 is compact, packed in a good enclosure with
magnets on the back, and offers the same high-contrast 4.7" e-paper
panel as the LilyGO T5 boards. The epdiy backend uses the
single-pulse `EPD_MODE_FAST` waveform for partial refreshes (one
visible flash, ~80-150 ms per update); a full refresh (3-5 s) is
performed every `DRAFTLING_EPD_FULL_REFRESH_INTERVAL` partials
(default 30) to clear residual ghosting. **Note:** the PaperS3 has
been officially discontinued by M5Stack, so it is no longer
recommended for new builds -- prefer one of the LilyGO T5 E-Paper
S3 Pro variants for a similar e-paper experience on a board that
is still in production.



## Waveshare ESP32-S3-Touch-LCD-3.49

[Waveshare
ESP32-S3-Touch-LCD-3.49](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-3.49)
has a 3.49" IPS color LCD (640x172, AXS15231B over
QSPI). AXS5106-family capacitive touch on I2C (0x3B). BOOT (GPIO0)
wakes from deep sleep. External SD on a separate SPI bus.

The Waveshare ESP32-S3-Touch-LCD-3.49 drives a 640x172 landscape
AXS15231B color LCD (natively 172x640 portrait, software-rotated to
landscape) and an AXS5106-family capacitive touch controller at I2C
address 0x3B. The BOOT button on GPIO0 is the deep-sleep wake source.
The on-board PWR / home button on GPIO16 is reserved for the power
latch (short press to power on, long press to power off via the
TCA9554 IO6 latch) and is not used to wake the MCU from deep sleep.



## M5Stack Tab5

[M5Stack Tab5](https://docs.m5stack.com/en/core/Tab5) -- ESP32-P4
tablet with a 5" 1280x720 IPS color LCD (ILI9881C or ST7123, MIPI-DSI
2-lane, auto-detected by the upstream `espressif/m5stack_tab5`
BSP). GT911 capacitive touch on I2C (backup address 0x14). Li-ion
battery + PMIC + USB-C charging.  Wi-Fi / Bluetooth are routed through
an on-board ESP32-C6 co-processor via ESP-Hosted-MCU over SDIO; both
`wifi_manager` (Git sync) and `ble_keyboard` (BLE HID host) work the
same as on the ESP32-S3 boards. The C6 must be flashed once with the
matching ESP-Hosted slave firmware -- see
[docs/tab5-esp-hosted.md](docs/tab5-esp-hosted.md)` for the
procedure. Touch and the on-board MicroSD slot work; this board has
been added without on-hardware testing and will likely need bring-up
tweaks.

The M5Stack Tab5 is a much faster device than those using ESP32-S3,
and it has a detachable battery. You can buy compatible batteries with
a built-in fast USB charger. The device supports a USB keyboard as an
alternative too BLE. Also, M5 produces a specialized keyboard
ayttachment for Tab5, which is automatically supported by Draftling
firmware.


## Guition JC3248W535

Guition JC3248W535 is an ESP32-S3 device with a 3.5" IPS color LCD
(480x320, AXS15231B over QSPI). AXS5106L capacitive touch on I2C. No
user buttons -- the touch INT line is the deep-sleep wake
source. External SD on a separate SPI bus.

The Guition JC3248W535 is a color-LCD board with no user buttons,
so touch is the only local input besides the BLE keyboard: deep-sleep
wake is armed on the touch INT line and any tap wakes the device.


## Sunton ESP32-8048S043C and ESP32-8048S070C

Sunton [ESP32-8048S043C](docs/sunton-esp32-8048S043.md) and
[ESP32-8048S070C](docs/sunton-esp32-8048S070c.md) -- ESP32-S3 HMI
development boards with 16-bit parallel RGB565 interface and a GT911
capacitive touch controller.


## Waveshare ESP32-S3-Touch-LCD-7

[Waveshare
ESP32-S3-Touch-LCD-7](docs/waveshare-esp32-s3-touch-lcd-7.md) is an
ESP32-S3 board with 800x480 LCD touchscreen. Backlight brightness is
not tunable and some SD cards fail to read on this device. The board
is designed for industrial panels, rich in peripherals like RS-485. It
could be a base for a desktop word processor, while being too bulky
for a portable design.

The board (16 MB flash, 8 MB PSRAM) has the same 800x480 16-bit
parallel RGB565 interface and GT911 capacitive touch controller as the
Sunton boards, but with the LCD reset, backlight, touch reset and SD
card chip-select lines routed through an on-board CH422G I2C
IO-expander instead of direct GPIOs. Unlike the Sunton boards, GT911
INT and RST are both wired (RST via the CH422G), giving a
deterministic address-select reset, so touch is enabled by default
(toggle it off in `menuconfig` if not wanted). BOOT (GPIO0) wakes from
deep sleep. No on-board battery monitor. See the linked doc for the
full CH422G EXIO pin map.

 

## Freenove FNK0104A, FNK0104B, FNK0104S

[Freenove
FNK0104A](https://github.com/Freenove/Freenove_ESP32_S3_Display) --
2.8" ILI9341 color LCD (240x320 native panel, rendered landscape at
320x240) over standard 4-wire SPI. No touch controller. Battery on
GPIO9 ADC (1:2 divider). On-board MicroSD on the SDMMC 1-bit
peripheral. BOOT (GPIO0) wakes from deep sleep.

[Freenove
FNK0104B](https://github.com/Freenove/Freenove_ESP32_S3_Display) --
Same 2.8" ILI9341 panel and SPI wiring as the FNK0104A, plus an
on-board FT6336U capacitive touch controller on I2C (address 0x38).

[Freenove
FNK0104S](https://github.com/Freenove/Freenove_ESP32_S3_Display) --
4.0" ST7796 color LCD (320x480 native panel, rendered landscape at
480x320) over standard 4-wire SPI, with the same FT6336U capacitive
touch controller as the FNK0104B. Battery on GPIO9 ADC (1:2 divider).
On-board MicroSD on the SDMMC 1-bit peripheral. BOOT (GPIO0) wakes
from deep sleep. Freenove's own board silkscreen and README call this
panel "ST7789", but the vendor init sequence is unambiguously ST7796.

The Freenove FNK0104A/B/S boards are small, inexpensive color-LCD
kits (2.8" for the A/B, 4.0" for the S) that ship with a MicroSD slot
and a battery ADC input out of the box. The FNK0104A has no touch
controller and is keyboard-only; the FNK0104B and FNK0104S add an
FT6336U capacitive touch controller, so touch works alongside the BLE
keyboard.


## Xteink X4 Pro

Xteink X4 Pro -- ESP32-S3 e-reader with a 4.26" 800x480 e-paper panel
driven over plain SPI. Different manufacturing runs ship one of three
panel controllers -- SSD1677, or one of two UltraChip parts (UC8179 /
UC8279) -- which cannot be told apart from the outside; the display
backend (`components/display/display_xteink_epd.cpp`) probes the
controller over the bus at boot and drives whichever one is actually
present, so a single firmware image works on any run. GT911
capacitive touch and a CW2017 I2C fuel gauge share one I2C bus.
Dual-channel (cool/warm) PWM front-light, both channels driven
identically since Draftling has no warm/cool color-temperature UI.
Three buttons: Left and Right scroll the editor a screen at a time
(Page Up / Page Down); Power is the deep-sleep wake source, and a
short press also puts the device to sleep on demand (there is no
hardware latch on this board that can cut power to the ESP32 itself,
so deep sleep is the closest equivalent to "off" -- see
`wakeup_btn_poll_cb()` in `main/main.cpp`), while a 2 s hold forgets
BLE keyboards, matching the convention on every other board. On-board
MicroSD on SDMMC 1-bit.

The enclosure's cover overlaps the panel unevenly -- 12 px on the
left edge, 8 px on top, 0 on the right and bottom -- hiding that band
from view; `DRAFTLING_DISPLAY_MARGIN_LEFT/RIGHT/TOP/BOTTOM` (Kconfig,
zero on every other board) shrink the editor/LVGL canvas by that much
and the display backend offsets every write into the physical
framebuffer to compensate, so on-screen content is never drawn where
the cover would hide it.

**This board has been tested on physical hardware after an initial
blind port.** The register sequences, timing and the
controller-detection probe are ported from the [FreeInk
SDK](https://github.com/Free-Ink/freeink-sdk) (MIT licensed), which
reverse-engineered the Xteink X4 Pro OEM firmware down to exact
register values and validated the non-grayscale paths used here on
real units. Grayscale / anti-aliasing rendering is not implemented,
matching every other e-paper board. `display_xteink_epd.cpp` now runs
the panel's SPI bus at the OEM's own 5 MHz rather than the FreeInk
SDK reference's speed-optimized 20 MHz default.

On-hardware testing (UC8179 controller variant) found a full refresh
reproducibly painted only the top third or so of the panel, leaving
the rest blank -- traced to a genuine race condition in
`uc8179_display_full()` / `uc8279_display_full()`: the OLD-plane
write reused the same scratch buffer the NEW-plane write had just
handed to `esp_lcd_panel_io_tx_color()`, whose SPI/DMA transfer is
queued asynchronously and can still be in flight when the very next
line `memset()`s that same buffer, corrupting the in-flight transfer.
Both functions now stream the OLD-plane's constant white fill from a
separate, never-mutated buffer instead of reusing the NEW-plane's
scratch buffer. The GT911 touch orientation (`TOUCH_SWAP_XY` /
`TOUCH_MIRROR_X` / `TOUCH_MIRROR_Y` in `main/boards/xteink_x4_pro.h`)
is still a best-effort starting point and may need a dial-in pass
with `CONFIG_DRAFTLING_TOUCH_DEBUG_LOG`.

## Elecrow CrowPanel ESP32-S3 5.79" E-Paper HMI Display

[Elecrow CrowPanel ESP32-S3 5.79" E-Paper HMI
Display](https://www.elecrow.com/crowpanel-esp32-5-79-e-paper-hmi-display-with-272-792-resolution-black-white-color-driven-by-spi-interface.html)
-- ESP32-S3-WROOM-1-N8R8 driving a 792x272 black/white e-paper panel
built from two SSD1683 controllers (one per half), over plain SPI.
No touchscreen. On-board MicroSD on its own SPI bus. Five buttons:
Menu, Back, a 3-way dial switch (Up/Down/OK), plus RESET and BOOT.

The Menu button (GPIO2) is the deep-sleep wake source and, during
normal operation, doubles as an F1 key (open/close the Settings
menu) on a short press / "forget all BLE keyboards" on a 2 s hold --
the same convention as the LilyGO T5 E-Paper S3 Pro H752's side key.
The Back button and the dial switch round out full menu navigation
without a keyboard: Back injects Esc, the dial's Up/Down positions
inject the arrow keys, and pressing the dial (OK) injects Enter. No
on-board battery ADC or fuel gauge was found in the vendor's Eagle
schematic or Arduino examples (the "BAT" net is a bare JST connector
with no divider wired to any GPIO), so the battery indicator is not
available on this board.

Pin assignments come from the vendor's [Arduino
examples](https://github.com/Elecrow-RD/CrowPanel-ESP32-5.79-E-paper-HMI-Display-with-272-792)
and Eagle schematic, cross-checked against the community ESPHome
driver at
[github.com/samperk1/esphome-crowpanel-579](https://github.com/samperk1/esphome-crowpanel-579),
which reverse-engineered and photograph-verified the dual-SSD1683
command sequences this backend ports
(`components/display/display_ssd1683.cpp`).

### E-paper refresh reliability

The board has been extensively tested on real hardware (see [PR
#47](https://github.com/clackups/draftling/pull/47)). Boot, buttons,
dial, SD card, BLE keyboard input, deep sleep, and both partial and
full e-paper refresh all work correctly.

**Root cause.** A single Master Activation (`0x20`) trigger on this
panel can leave an incomplete pixel transition -- confirmed, by
systematically ruling out every other explanation on real hardware, to
be independent of which waveform is used, of RAM content, and of
typing speed. Immediately repeating the *exact same* update (a full
RAM rewrite followed by another trigger, not just a bare re-trigger on
unchanged RAM -- that was separately confirmed to actively erase the
just-drawn content instead of fixing it) reliably completes the
transition, matching what manually pressing Ctrl+R always did.
`display_flush()` in `display_ssd1683.cpp` runs both the full and
partial refresh paths twice, back-to-back, for this reason.

Every refresh (partial or full) also rewrites the panel's *entire* New
and Old RAM on both chips, not just the dirty rectangle -- an earlier,
narrower-windowed implementation left the untouched rest of the panel
implicitly relying on stale earlier writes, which testing found
unreliable independent of the double-trigger issue above.

**What was ruled out**, each independently confirmed on hardware,
before the actual cause was found: a union dirty-bbox spanning most of
the panel height; missing dual-chip cascade configuration (SSD1683
datasheet section 6.12, register 0x21 "Display Update Control 1");
Old RAM not resynced after a partial refresh; insufficient
post-refresh settle time (tested from 100ms up to 2500ms); the
waveform/LUT itself (tested the factory fast waveform, a from-scratch
custom LUT built from the datasheet's own waveform-setting tables, and
a hardware-verified custom LUT for a different SSD1683 panel -- all
three showed the identical symptom, ruling out the waveform entirely);
and a bare second trigger without rewriting RAM (actively harmful --
erases just-drawn content rather than fixing it, which is what pointed
toward "repeat the *whole* update" instead).



## Other hardware

Code contributions for supporting other types of hardware are very much welcome.

I also tried porting the RockBase NM-CYD-C5 board, which is a cheap
remake of the Cheap Yellow Board, utilizing a modern ESP32-C5 MCU. But
the screen quality is too poor for any comfortable work, so the
corresoindig [pull
request](https://github.com/clackups/draftling/pull/37) stays
unmerged.

UC8179-based e-paper displays (such as those used by the Seeed Studio
reTerminal E1001 and the Waveshare E-Paper Driver HAT) were previously
supported, but proved too slow for an interactive Markdown editor:
even with fast partial updates, the panel cannot keep up with typing
and quickly accumulates ghosting artefacts. Support for UC8179 has
therefore been removed from the codebase, except for the specific
waveform/timing combination used by the Xteink X4 Pro's UC8179 /
UC8279 panel controller variants (see above), which use a different,
faster partial-refresh waveform than the panels that prompted the
earlier removal.
