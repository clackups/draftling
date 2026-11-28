# Draftling: supported hardware

Draftling runs on a number of ESP32-S3 devices and development boards,
one ESP32-P4 device (M5Stack Tab5), one ESP32-C5 board and one
ESP32-C61 device (OnePage Reader). See the
[photo gallery](images/README.md) of some of them.

Every board runs the same firmware source; the board is picked at build
time with a preset (`idf.py --preset <preset> build`, see
[BUILDING.md](BUILDING.md)). Unless a board's section says otherwise,
it has 16 MB of flash and 8 MB of PSRAM, takes a Bluetooth (BLE)
keyboard, syncs over Wi-Fi, stores documents on a MicroSD card, and its
BOOT button wakes it from sleep (hold it 2 s to forget the paired
keyboards).

The same editor also runs on Linux computers, as a native program in a
Wayland or X11 window; it is built with
`make` in `linux/` rather than with a preset (see
[linux/README.md](linux/README.md)), and in a web browser, as a web
application built with `make` in `web_client/` (see
[web_client/README.md](web_client/README.md)).

Features used in the table:

- **Light**: the backlight or front-light can be dimmed from
  F1 -> Settings -> Display or with Ctrl+B.
- **Battery**: the status bar shows the battery level.
- **USB**: "SD card via USB" and "Sync via browser" are available (the
  board's USB port is the ESP32-S3's native USB).

## Summary

| Board | Preset | Display | Touch | Light | Battery | USB |
| --- | --- | --- | --- | --- | --- | --- |
| [Waveshare ESP32-S3-RLCD-4.2](#waveshare-esp32-s3-rlcd-42) | `waveshare_rlcd42` | 4.2" 400x300 reflective LCD | - | - | yes | yes |
| [LilyGO T5 E-Paper S3 Pro / Pro Lite](#lilygo-t5-e-paper-s3-pro-and-pro-lite) | `lilygo_t5_epd_s3_pro` | 4.7" 960x540 e-paper | yes | front-light | yes | yes |
| [LilyGO T5 E-Paper S3 Pro (H752)](#lilygo-t5-e-paper-s3-pro-h752) | `lilygo_t5_epd_s3_pro_h752` | 4.7" 960x540 e-paper | yes | front-light | yes | yes |
| [LilyGO T5 4.7" E-Paper (ESP32-S3)](#lilygo-t5-47-e-paper-esp32-s3) | `lilygo_t5_47_s3` | 4.7" 960x540 e-paper | - | - | - | yes |
| [MicroJournal Rev.7](#microjournal-rev7) | `microjournal_rev7` | 4.7" 960x540 e-paper | - | - | - | - |
| [M5Stack PaperS3](#m5stack-papers3) | `m5stack_papers3` | 4.7" 960x540 e-paper | yes | - | yes | yes |
| [M5Stack PaperMono / PaperMono-Lite](#m5stack-papermono--papermono-lite) | `m5stack_papermono` | 3.97" 800x480 e-paper | yes | front-light | yes | yes |
| [Xteink X4 Pro](#xteink-x4-pro) | `xteink_x4_pro` | 4.26" 800x480 e-paper | yes | front-light | yes | yes |
| [Xteink X4 Classic (X4 v2)](#xteink-x4-classic-x4-v2) | `xteink_x4_classic` | 4.26" 800x480 e-paper | - | - | yes | yes |
| [Waveshare ESP32-S3-ePaper-3.97](#waveshare-esp32-s3-epaper-397) | `waveshare_epaper_397` | 3.97" 800x480 e-paper | - | - | yes | yes |
| [Seeed reTerminal Sticky](#seeed-reterminal-sticky) | `seeed_reterminal_sticky` | 3.97" 800x480 e-paper | yes | - | yes | - |
| [Seeed Studio reTerminal E1001](#seeed-studio-reterminal-e1001) | `seeed_reterminal_e1001` | 7.5" 800x480 e-paper | - | - | yes | - |
| [Seeed Studio TRMNL 7.5" (OG) DIY Kit](#seeed-studio-trmnl-75-og-diy-kit) | `seeed_trmnl_75_og_diy` | 7.5" 800x480 e-paper | - | - | yes | - |
| [Elecrow CrowPanel 5.79" E-Paper](#elecrow-crowpanel-esp32-s3-579-e-paper-hmi-display) | `elecrow_crowpanel_579` | 5.79" 792x272 e-paper | - | - | - | - |
| [M5Stack Tab5](#m5stack-tab5) | `m5stack_tab5` | 5" 1280x720 IPS | yes | yes | yes | - |
| [Waveshare ESP32-S3-Touch-LCD-3.49](#waveshare-esp32-s3-touch-lcd-349) | `waveshare_touch_lcd_349` | 3.49" 640x172 IPS | yes | yes | yes | yes |
| [Waveshare ESP32-S3-Touch-LCD-7](#waveshare-esp32-s3-touch-lcd-7) | `waveshare_touch_lcd_7` | 7" 800x480 IPS | yes | on / off only | - | yes |
| [Waveshare ESP32-S3-LCD-3.16](#waveshare-esp32-s3-lcd-316) | `waveshare_lcd_316` | 3.16" 820x320 IPS | - | yes | yes | yes |
| [Guition JC3248W535](#guition-jc3248w535) | `guition_jc3248w535` | 3.5" 480x320 IPS | yes | yes | - | - |
| [Guition JC4827W543C](#guition-jc4827w543c) | `guition_jc4827w543c` | 4.3" 480x272 IPS | yes | yes | - | - |
| [Sunton ESP32-8048S043C](#sunton-esp32-8048s043c-and-esp32-8048s070c) | `sunton_8048s043` | 4.3" 800x480 IPS | yes | yes | - | - |
| [Sunton ESP32-8048S070C](#sunton-esp32-8048s043c-and-esp32-8048s070c) | `sunton_8048s070` | 7" 800x480 TN | optional | yes | - | - |
| [Elecrow CrowPanel ESP32 HMI 5.0"](#elecrow-crowpanel-esp32-hmi-50) | `elecrow_crowpanel_50` | 5" 800x480 TN | yes | yes | - | - |
| [VIEWE UEDX80480043E-WB-A / -WB-B](#viewe-uedx80480043e-wb-a---wb-b) | `viewe_uedx80480043` | 4.3" 800x480 IPS | yes | yes | - | - |
| [Freenove FNK0104A](#freenove-fnk0104a-fnk0104b-fnk0104s-fnk0104n) | `freenove_fnk0104a` | 2.8" 320x240 LCD | - | yes | yes | yes |
| [Freenove FNK0104B](#freenove-fnk0104a-fnk0104b-fnk0104s-fnk0104n) | `freenove_fnk0104b` | 2.8" 320x240 LCD | yes | yes | yes | yes |
| [Freenove FNK0104S](#freenove-fnk0104a-fnk0104b-fnk0104s-fnk0104n) | `freenove_fnk0104s` | 4.0" 480x320 LCD | yes | yes | yes | yes |
| [Freenove FNK0104N](#freenove-fnk0104a-fnk0104b-fnk0104s-fnk0104n) | `freenove_fnk0104n` | 3.5" 480x320 IPS | yes | yes | yes | yes |
| [Viewe UEDX24320028E-WB-A + UEED035HV](#viewe-uedx24320028e-wb-a-with-ueed035hv-rx40-l001-display) | `viewe_uedx24320028_ueed035hv` | 3.5" 480x320 transflective | yes | yes | - | yes |
| [MicroJournal Rev 5.0 / 5.1](#microjournal-rev-50--51) | `microjournal_rev5` | 2.8" 320x240 LCD | - | - | - | - |
| [NiceMCU-C5E-DEV_2.8IPS](#nicemcu-c5e-dev_28ips) | `nicemcu_c5_28ips` | 2.8" 320x240 IPS | yes | yes | - | - |
| [OnePage Reader](#onepage-reader) | `onepage_c61` | 4.26" 800x480 e-paper | - | - | yes | - |


## Waveshare ESP32-S3-RLCD-4.2

[Waveshare ESP32-S3-RLCD-4.2](https://www.waveshare.com/wiki/ESP32-S3-RLCD-4.2)

- 4.2" 400x300 reflective LCD: no backlight, readable in daylight. Fast
  and responsive, but the contrast is low, so it needs good lighting.
- No touch.
- Battery level.
- The button on GPIO18 (not BOOT) wakes the device and forgets the
  paired keyboards on a 2 s hold.
- SD card via USB and Sync via browser.
- The screen is fragile and the board needs an enclosure: a [simple
  3D-printed enclosure](3D_Prints/Waveshare_ESP32-S3-RLCD-4.2/), or
  [Whale Writer](https://github.com/shmimel/whale-writer), a foldable
  enclosure with an attached keyboard by Simon Shimel.


## LilyGO T5 E-Paper S3 Pro and Pro Lite

[LilyGO T5 E-Paper S3 Pro](https://github.com/Xinyuan-LilyGO/T5S3-4.7-e-paper-PRO)

So far the most usable option.

- 4.7" 960x540 high-contrast e-paper (ED047TC1). Slower than the
  reflective LCD, but acceptable.
- GT911 touch.
- White front-light.
- Battery level and charging state (BQ27220 fuel gauge).
- SD card via USB and Sync via browser.
- The Pro adds a LoRa radio, GPS, an IR LED, a vibration motor and an
  18650 holder, none of which Draftling uses; the Lite is the same
  board without them. One firmware serves both.
- Low standby current: the device switches off every peripheral before
  sleep (see [docs/lilygo-t5-power.md](docs/lilygo-t5-power.md)).


## LilyGO T5 E-Paper S3 Pro (H752)

[LilyGO T5 E-Paper S3 Pro (H752)](https://github.com/Xinyuan-LilyGO/T5S3-4.7-e-paper-PRO)
is the original revision (v1.0-240810) of the T5 E-Paper S3 Pro.

- Same 4.7" 960x540 e-paper panel and GT911 touch as the current Pro.
- Front-light.
- Battery level.
- Side key: menu (F1) on a short press, wakes the device, forgets the
  paired keyboards on a 2 s hold. The touch key below the panel is
  Esc.
- SD card via USB and Sync via browser.


## LilyGO T5 4.7" E-Paper (ESP32-S3)

[LilyGO T5 4.7" E-Paper](https://lilygo.cc/products/t5-4-7-inch-e-paper-v2-3),
ESP32-S3 version (V2.3). Not the T5 E-Paper S3 Pro above: an older,
different board.

- 4.7" 960x540 e-paper panel.
- No touch (the touch panel of the touch version is not used yet), no
  front-light, no battery level.
- MicroSD card slot on the back of the board.
- The user button (IO21) wakes the device and forgets the paired
  keyboards on a 2 s hold. The BOOT button is not used: its line also
  drives the display.
- SD card via USB and Sync via browser over the USB-C port, or a USB
  keyboard instead (see [USB keyboard host](#usb-keyboard-host) and the
  MicroJournal Rev.7 below).


## MicroJournal Rev.7

[Micro Journal Rev.7](https://github.com/unkyulee/micro-journal/tree/main/rev.7/7.0)
("Kindred Gift"), an e-paper writerdeck built around the LilyGO T5
4.7" E-Paper (ESP32-S3) above, with a built-in keyboard (a Raspberry
Pi Pico running QMK, connected to the T5's USB-C port). The
`microjournal_rev7` preset is the T5 firmware with the USB-C port as a
USB keyboard host.

- The keyboard works as on any computer: the Fn key with the number
  row gives F1 - F12 (F1 opens the menu, F10 the help).
- The knobs send Page Up / Page Down. Their clicks (F5 and Pause in the
  stock keymap) do nothing in Draftling yet; they can be remapped with
  Vial.
- Documents are stored on a MicroSD card in the T5's slot, which can be
  reached in the assembled device. (The stock Rev.7 firmware keeps them
  in the internal flash instead and needs no card.)
- Switch the power off at the back after the device goes to sleep
  (Ctrl+P or the inactivity timeout); there is no wake button.
- USB keyboard only: Bluetooth is left out to save battery.
- No SD card via USB or Sync via browser: the USB port belongs to the
  keyboard.


## M5Stack PaperS3

[M5Stack PaperS3](https://docs.m5stack.com/en/core/papers3)

**Discontinued by M5Stack**; prefer a LilyGO T5 E-Paper S3 Pro for a
similar device.

- 4.7" 960x540 high-contrast e-paper (ED047TC1), compact enclosure
  with magnets on the back.
- GT911 touch.
- No front-light.
- Battery level.
- BOOT wakes the device; a tap can wake it too (optional,
  `DRAFTLING_STANDBY_WAKE_ON_TOUCH`).
- SD card via USB and Sync via browser.


## M5Stack PaperMono / PaperMono-Lite

[M5Stack PaperMono-Lite](https://docs.m5stack.com/en/core/PaperMono-Lite) and
[PaperMono](https://docs.m5stack.com/en/core/PaperMono).

- 3.97" 800x480 e-paper (SSD1677). The device is natively portrait;
  Draftling starts in landscape (F1 -> Settings -> Display ->
  Orientation switches it).
- FT6336G touch.
- Front-light.
- Battery level (1150 mAh); the charging mark shows on USB power.
- Button A: Page Up; wakes the device; hold 2 s to forget the paired
  keyboards. Button B: Page Down; hold 2 s to go to sleep.
- SD card via USB and Sync via browser.
- The full PaperMono's NFC reader and LoRa module are not used; one
  firmware serves both models.
- Sources: M5Stack's pin maps and the MIT-licensed
  [M5GFX](https://github.com/m5stack/M5GFX),
  [M5PM1](https://github.com/m5stack/M5PM1),
  [M5IOE1](https://github.com/m5stack/M5IOE1) and
  [M5PaperMono-PowerDemo](https://github.com/m5stack/M5PaperMono-PowerDemo).


## Xteink X4 Pro

Xteink X4 Pro e-reader.

- 4.26" 800x480 e-paper. Units carry one of three panel controllers
  (SSD1677, UC8179 or UC8279); the firmware detects which at boot.
- GT911 touch.
- Front-light.
- Battery level (CW2017 fuel gauge); the charging state is not
  available.
- Buttons: Left / Right = Page Up / Page Down. Power wakes the device,
  puts it to sleep on a short press (asking first about unsaved
  changes) and forgets the paired keyboards on a 2 s hold.
- SD card via USB and Sync via browser.
- The enclosure's cover hides a band at the edge of the panel (about
  12 px on the left and 8 px on top on the unit it was tuned on). Set
  F1 -> Settings -> Display -> Screen margins to match your unit.
- Dual-OTA partition table, so the stock Xteink or the Crosspoint
  firmware can be installed again later with their own updaters.
- Source: the [FreeInk SDK](https://github.com/Free-Ink/freeink-sdk)
  (MIT), which reverse-engineered the OEM firmware.


## Xteink X4 Classic (X4 v2)

Xteink X4 Classic, also sold as "X4 v2": the buttons-only sibling of
the X4 Pro.

- Same 4.26" 800x480 e-paper with controller detection as the X4 Pro.
- No touch, no front-light.
- Battery level (CW2017); the charging state is not available.
- Eight buttons drive the editor without a keyboard: Power = F1 menu
  (wakes the device, forgets the paired keyboards on a 2 s hold), side
  keys = Up / Down, bottom keys = Left / Right / Enter / Esc.
- SD card via USB and Sync via browser.
- Screen margins and the dual-OTA partition table as on the X4 Pro.
- Source: the FreeInk SDK (`docs/xteink-x4c-support.md`).


## Waveshare ESP32-S3-ePaper-3.97

[Waveshare ESP32-S3-ePaper-3.97](https://docs.waveshare.com/ESP32-S3-ePaper-3.97).

- 3.97" 800x480 e-paper (SSD1677-compatible).
- No touch, no front-light.
- Battery level and charging state (AXP2101 power-management chip).
- Buttons: Up / Down = arrow keys, Function = Enter (hold 2 s to go to
  sleep; it also wakes the device). BOOT wakes the device and forgets
  the paired keyboards on a 2 s hold.
- SD card via USB and Sync via browser.
- The RTC, temperature / humidity sensor, motion sensor and audio codec
  are not used.
- Source: pin numbers and the AXP2101 register map from Waveshare's
  [ESP-IDF example](https://github.com/waveshareteam/ESP32-S3-ePaper-3.97)
  (facts only; that repository has no license).


## Seeed reTerminal Sticky

[Seeed Studio reTerminal Sticky](https://www.seeedstudio.com/sticky/docs/en/device-guide/hardware-overview/):
a battery-powered, magnetically mounted e-paper badge, 32 MB flash.

- 3.97" 800x480 e-paper. Units carry an SSD1677 or an SSD2677
  controller, detected at boot.
- GT911 touch.
- No front-light.
- Battery level and charging state (BQ27220 fuel gauge).
- Buttons: Up / Down = Page Up / Page Down. Power wakes the device,
  puts it to sleep on a short press (asking first about unsaved
  changes) and forgets the paired keyboards on a 2 s hold. On battery,
  sleep switches the device off completely; on USB power it sleeps.
- No SD card via USB or Sync via browser: the USB-C port goes through a
  USB-to-UART bridge.
- Dual-OTA partition table, so the Crosspoint firmware can be installed
  later.
- The temperature / humidity sensor, motion sensor, RTC, microphone and
  buzzer are not used.
- Sources: Seeed's hardware overview and the
  [FreeInk SDK](https://github.com/Free-Ink/freeink-sdk) (MIT).


## Seeed Studio reTerminal E1001

[Seeed Studio reTerminal E1001](https://wiki.seeedstudio.com/reTerminal_E1001/):
a 7.5" e-paper terminal, 32 MB flash.

- 7.5" 800x480 e-paper (GDEY075T7, UC8179): fast partial refresh about
  450 ms, full refresh about 1.2 s.
- No touch, no front-light.
- Battery level.
- KEY0 wakes the device and forgets the paired keyboards on a 2 s hold;
  KEY1 and KEY2 are not used.
- No SD card via USB or Sync via browser.
- The RTC, temperature sensor, LED and buzzer are not used.


## Seeed Studio TRMNL 7.5" (OG) DIY Kit

The same 7.5" e-paper and buttons as the reTerminal E1001, 16 MB
flash, no MicroSD slot. The `seeed_trmnl_75_og_diy` preset is the
E1001 build with the documents kept in the internal flash (see
"Boards without a MicroSD slot" below): 8 MB of document storage.

- Everything else as on the reTerminal E1001 above.
- F1 -> System & Storage -> "Format internal storage..." erases the
  documents; there is no SD card via USB.


## Boards without a MicroSD slot

Any board can keep its documents in a partition of the ESP32's own
flash instead of on a MicroSD card: add
`sdkconfig.defaults.storage_flash_4mb`, `_8mb` or `_16mb` (matching
the board's flash size) to the end of the preset's
`SDKCONFIG_DEFAULTS` in `firmware/CMakePresets.json`. It turns on
`CONFIG_DRAFTLING_STORAGE_FLASH` and selects the partition table:

| Flash | Table | App slot | Document storage |
| --- | --- | --- | --- |
| 4 MB | `partitions_storage_4mb.csv` | 2.94 MB | 1 MB |
| 8 MB | `partitions_storage_8mb.csv` | 4.94 MB | 3 MB |
| 16 MB | `partitions_storage_16mb.csv` | 7.94 MB | 8 MB |

The storage is formatted on first boot. The build refuses a storage
partition smaller than 1 MB. On a 4 MB flash only the default and
Vietnamese language packs fit next to the storage.


## Elecrow CrowPanel ESP32-S3 5.79" E-Paper HMI Display

[Elecrow CrowPanel ESP32-S3 5.79" E-Paper HMI
Display](https://www.elecrow.com/crowpanel-esp32-5-79-e-paper-hmi-display-with-272-792-resolution-black-white-color-driven-by-spi-interface.html):
8 MB flash.

- 5.79" 792x272 e-paper (two SSD1683 controllers, one per half).
- No touch, no front-light.
- No battery level (the battery connector is not measured).
- Buttons drive the menus without a keyboard: Menu = F1 (wakes the
  device, forgets the paired keyboards on a 2 s hold), Back = Esc,
  dial switch = Up / Down / Enter.
- No SD card via USB or Sync via browser.
- Sources: the vendor's
  [Arduino examples](https://github.com/Elecrow-RD/CrowPanel-ESP32-5.79-E-paper-HMI-Display-with-272-792)
  and schematic, and the community
  [ESPHome driver](https://github.com/samperk1/esphome-crowpanel-579).


## M5Stack Tab5

[M5Stack Tab5](https://docs.m5stack.com/en/core/Tab5): an ESP32-P4
tablet, 32 MB PSRAM. Much faster than the ESP32-S3 devices.

- 5" 1280x720 IPS color LCD (ILI9881C or ST7123, detected
  automatically).
- GT911 touch.
- Backlight.
- Battery level and charging state; detachable battery (compatible
  batteries with a built-in USB charger are available).
- Keyboards: BLE, USB, and M5Stack's keyboard attachment for the Tab5.
- Wi-Fi and Bluetooth go through the on-board ESP32-C6, which must run
  esp-hosted 3.1 or newer: see
  [docs/tab5-esp-hosted.md](docs/tab5-esp-hosted.md) for the upgrade
  tool.
- On battery, sleep turns the Tab5 off completely; the side power button
  turns it on. On USB power it sleeps, and RESET wakes it.
- No SD card via USB or Sync via browser.


## Waveshare ESP32-S3-Touch-LCD-3.49

[Waveshare ESP32-S3-Touch-LCD-3.49](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-3.49)

- 3.49" 640x172 IPS color LCD (AXS15231B).
- AXS5106 touch.
- Backlight.
- Battery level.
- PWR button: a short press turns the device on, a long press turns it
  off. On battery, sleep turns it off completely.
- SD card via USB and Sync via browser.


## Waveshare ESP32-S3-Touch-LCD-7

[Waveshare ESP32-S3-Touch-LCD-7](docs/waveshare-esp32-s3-touch-lcd-7.md):
an industrial panel board, rich in peripherals like RS-485. A base for
a desktop writer; too bulky for a portable one.

- 7" 800x480 IPS color LCD.
- GT911 touch.
- Backlight on / off only (no dimming).
- No battery level.
- SD card via USB and Sync via browser.
- Some SD cards fail to read on this board.


## Waveshare ESP32-S3-LCD-3.16

[Waveshare ESP32-S3-LCD-3.16](https://www.waveshare.com/wiki/ESP32-S3-LCD-3.16).

- Long, narrow 3.16" IPS color LCD (ST7701), shown landscape at
  820x320.
- No touch.
- Backlight, 25 % to 100 %.
- Battery level; the charging state is not available. Power slide
  switch.
- SD card via USB and Sync via browser.
- The motion sensor and RTC are not used (they are put into their
  power-saving modes at boot).


## Guition JC3248W535

Guition JC3248W535.

- 3.5" 480x320 IPS color LCD (AXS15231B).
- AXS5106L touch. There are no user buttons: a tap wakes the device.
- Backlight.
- No battery level. USB powered.
- No SD card via USB or Sync via browser.


## Guition JC4827W543C

[Guition
JC4827W543C](https://www.atomic14.com/esp32/boards/guition-jc4827w543/):
4 MB flash. The resistive-touch (JC4827W543R) and no-touch
(JC4827W543N) variants are not supported.

- 4.3" 480x272 IPS color LCD (NV3041A).
- GT911 touch.
- Backlight.
- No battery level: the board has a battery connector and a charger,
  but the battery is not measured.
- No SD card via USB or Sync via browser.
- 4 MB flash: the Japanese, Chinese and Indic language packs do not fit.
- Source: the vendor IO table in
  [profi-max/JC4827W543_4.3inch_ESP32S3_board](https://github.com/profi-max/JC4827W543_4.3inch_ESP32S3_board).


## Sunton ESP32-8048S043C and ESP32-8048S070C

Sunton ESP32-S3 HMI development boards, 4.3" and 7"; see
[docs/sunton-esp32-8048s0xx.md](docs/sunton-esp32-8048s0xx.md) for
the pins and panel timings.

- 4.3" 800x480 IPS (S043C) or 7" 800x480 TN (S070C) color LCD.
- GT911 touch: on by default on the S043C, optional on the S070C.
- Backlight.
- No battery level.
- No SD card via USB or Sync via browser.


## Elecrow CrowPanel ESP32 HMI 5.0"

[Elecrow CrowPanel ESP32 HMI
5.0"](https://www.elecrow.com/wiki/esp32-display-502727-intelligent-touch-screen-wi-fi26ble-800480-hmi-display.html)
(DIS07050H, [GitHub](https://github.com/Elecrow-RD/CrowPanel-5.0-HMI-ESP32-Display-800x480)):
4 MB flash.

- 5" 800x480 TN color LCD.
- GT911 touch.
- Backlight (dimmed at 15 kHz; change "Backlight PWM frequency (Hz)"
  in menuconfig if your unit whines or flickers).
- No battery level: the board has a charger, but the battery is not
  measured.
- BOOT does nothing: the panel holds that pin. Press RESET to wake the
  device; forget the paired keyboards with the "Forget KB" button on
  the keyboard prompt.
- No SD card via USB or Sync via browser.
- 4 MB flash: the Japanese, Chinese and Indic language packs do not fit.
- Standby current about 48 mA: not suited to portable designs.


## VIEWE UEDX80480043E-WB-A / -WB-B

[VIEWE UEDX80480043E-WB](https://github.com/VIEWESMART/UEDX80480043ESP32-4.3inch-Touch-Display).
One firmware for both revisions.

- 4.3" 800x480 color LCD (ST7262).
- GT911 touch.
- Backlight (dimmed at 15 kHz; change "Backlight PWM frequency (Hz)"
  in menuconfig if your unit whines or flickers).
- No battery input: USB powered, for desktop designs.
- No SD card via USB or Sync via browser.
- Standby current about 30 mA.


## Freenove FNK0104A, FNK0104B, FNK0104S, FNK0104N

[Freenove ESP32-S3 Display](https://github.com/Freenove/Freenove_ESP32_S3_Display)
kits: small, inexpensive color-LCD boards.

- FNK0104A: 2.8" 320x240 LCD (ILI9341), no touch.
- FNK0104B: the same panel with FT6336U touch.
- FNK0104S: 4.0" 480x320 LCD (ST7796; Freenove's documentation calls
  it ST7789) with FT6336U touch.
- FNK0104N: 3.5" 480x320 IPS LCD (ST77922) with touch built into the
  display controller.
- All: backlight, battery level, SD card via USB and Sync via browser.


## Viewe UEDX24320028E-WB-A with UEED035HV-RX40-L001 display

The [UEDX24320028E-WB-A
board](https://github.com/VIEWESMART/UEDX24320028ESP32-2.8inch-Touch-Display)
with the
[UEED035HV-RX40-L001](https://viewedisplay.com/product/3-5-inch-320x480-sunlight-readable-transflective-with-super-low-power-tft-lcd-module/)
display attached in place of its stock 2.8" panel (the board with the
stock panel is not supported).

- 3.5" 480x320 transflective color LCD (ST7365): readable in daylight
  with the backlight off.
- CHSC6540-compatible touch.
- Backlight, down to 0 % (off).
- No battery level.
- SD card via USB and Sync via browser.


## MicroJournal Rev 5.0 / 5.1

[Micro Journal](https://github.com/unkyulee/micro-journal) revisions
5.0 and 5.1, an ESP32-S3 writerdeck. One firmware for both.

- 2.8" 320x240 color LCD: ILI9341 on Rev 5.0, ST7789 on Rev 5.1,
  detected at boot (F1 -> System & Storage -> Hardware information
  shows which).
- No touch, no backlight control, no battery level.
- USB keyboard only: Bluetooth is disabled. Keyboards behind a USB hub
  work too.
- No wake button: after sleep, press reset or switch the power off and
  on.
- No SD card via USB or Sync via browser.


## NiceMCU-C5E-DEV_2.8IPS

[NiceMCU-C5E-DEV_2.8IPS](https://github.com/NiceMCU-01/NiceMCU-C5-2.8IPS)
(also sold as NiceMCU-C5-2.8IPS): the only ESP32-C5 board, dual-band
2.4 / 5 GHz Wi-Fi 6.

Not recommended for new builds: it is only slightly cheaper than the
Freenove boards, has no battery input, and the ESP32-C5 has no USB
device support.

- 2.8" 320x240 IPS color LCD (ST7789P3). If the picture is upside
  down, turn on F1 -> Settings -> Display -> Rotate 180.
- Touch (CST816D or FT6336G).
- Backlight.
- No battery input.
- BOOT: hold 2 s to forget the paired keyboards. The ESP32-C5 cannot
  wake from it: press RESET to wake the device.
- Use the USB-C port with the USB-to-UART chip; the native-USB port
  does not work while Draftling runs. No SD card via USB or Sync via
  browser.
- Source: the vendor's example sketch and schematic.


## OnePage Reader

[OnePage Reader](https://github.com/MoveCall/onepage-reader): an
open-hardware e-reader with an ESP32-C61 (Wi-Fi 6, BLE) in a sealed
body.

- 4.26" 800x480 e-paper (SSD1677, the Xteink X4 panel).
- No touch, no front-light.
- Battery level, and the charging indicator while on USB power.
- Buttons: Power wakes the device and puts it to sleep on a 2 s hold
  (asking first about unsaved changes); holding the Up (PREV) side key
  for 2 s forgets the paired keyboards. While a keyboard is connected,
  short presses also work: Power = F1 menu, side keys = Up / Down,
  front keys = Esc / Left / Right / Enter.
- No SD card via USB or Sync via browser: the ESP32-C61 has no USB
  device support (the USB-C port is its serial / debug port).
- The microphone is not used.
- Sources: the vendor's
  [Arduino board support](https://github.com/MoveCall/onepage-reader-sdk-arduino)
  and [CrossPoint port](https://github.com/MoveCall/crosspoint-onepage)
  (MIT).


## USB keyboard host

Any ESP32-S3 board can take a USB keyboard on the chip's native USB
port (GPIO19 / GPIO20) instead of using it as a USB device: turn on
"Use the native USB port as a USB host for a keyboard" in
`idf.py --preset <board> menuconfig` (DRAFTLING Configuration,
`CONFIG_DRAFTLING_USB_HOST_NATIVE`). Keyboards behind a USB hub work
too. The MicroJournal Rev 5 and Rev.7 presets have it on.

- The board's USB connector must be wired to the native USB pins.
  On boards whose USB port goes through a USB-to-UART chip (no "USB" in
  the table above) the pins have to be wired to a USB socket by hand.
  The option is not offered on the boards that use GPIO19 / GPIO20 for
  touch (Sunton, Elecrow CrowPanel 5.0", VIEWE UEDX80480043E-WB, Seeed
  reTerminal E1001).
- Most boards do not supply 5 V on their USB port, and the cable
  between the board and the keyboard (or hub) must carry 5 V on VBUS
  anyway: a keyboard takes its power from it, and a self-powered hub
  only connects to the board when it sees 5 V on its host cable (its
  keyboard port may have power while the board still sees nothing).
  Powering the board through its VCC / 5 V pin is not enough: that
  supply does not reach the USB port's VBUS. Use a USB OTG splitter
  with a power input, which powers the board as well, or wire the same
  5 V to VBUS of the OTG cable.
- SD card via USB and Sync via browser are not available then.
  Flashing over USB needs download mode (hold BOOT while pressing
  reset).
- Bluetooth is left out of such a firmware to save battery, so only
  the USB keyboard works. (Turning Bluetooth off in Component config ->
  Bluetooth as well only makes the firmware smaller.)


## Other hardware

Code contributions for supporting other hardware are very much welcome.

The RockBase NM-CYD-C5, a cheap remake of the Cheap Yellow Display
with an ESP32-C5, was also tried, but its screen is too poor for
comfortable work, so the [pull
request](https://github.com/clackups/draftling/pull/37) stays unmerged.
