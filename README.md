# Draftling

A distraction-free Markdown text editor for ESP32-S3- and ESP32-P4-based
development boards with reflective LCD, e-paper or color LCD displays.


## Features

- **WYSIWYG Markdown editing** on reflective LCD, e-paper or
  small color LCD displays

- **Bluetooth Low Energy (BLE) keyboard** input with auto-discovery and pairing

- **File browser** to open and manage `.md` files on the SD card
  (entries sorted alphabetically, directories first)

- **Markdown rendering**: headings (H1-H4), bullet and numbered lists,
  blockquotes, code fences, horizontal rules, inline bold / italic /
  code / strikethrough

- **Gap buffer** text engine for efficient editing. The document size
  limit is sized at boot from the PSRAM that is free when the editor
  starts (with a minimum of 64 KB and an upper bound queried from the
  Git sync client, so the editor cannot produce a document larger than
  what Git can push) and surfaced read-only in F1 -> Settings. Typical
  limits range from a few hundred KB to a few MB depending on the
  board.

- **WiFi** client mode with credentials from `/sdcard/wifi.cfg`

- **Git sync** via GitHub REST API (pull and push `.md` files,
  including deletion / rename propagation in both directions; the
  last-synced per-file blob SHAs are kept in a hidden `.git_state`
  file on the SD card so the next sync can do a proper 3-way merge)

- **Per-file metadata sidecars**: when a `.md` file is closed (or
  before the device enters deep sleep), the editor records the current
  cursor position and scroll line in a hidden sidecar file named
  `.<basename>.meta` next to the document (for example `notes.md` ->
  `.notes.md.meta`). The next time the file is opened, the cursor is
  restored to its previous position and the view scrolls so the cursor
  is visible. The `.meta` files are hidden from the file browser (they
  start with a dot) and are ignored by Git sync (which only handles
  `*.md`).

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| F1 | Open main menu (BLE, WiFi, Git, Layout, Settings...) |
| Arrow keys | Move cursor |
| Home / End | Start / end of line |
| PgUp / PgDn | Scroll by page |
| Ctrl+S | Save file |
| Ctrl+O | Open file browser |
| Ctrl+N | New file |
| Ctrl+L | Cycle keyboard layout |
| Win+Space | Cycle keyboard layout (same as Ctrl+L) |
| Ctrl+M | Menu (same as F1) |
| Ctrl+G | Git sync (pull + push) |
| Ctrl+W | Toggle WiFi (connect / disconnect) |
| Ctrl+F | Find |
| Ctrl+H | Find + Replace (Tab switches field, Enter = next match, Ctrl+Enter = replace + next) |
| Ctrl+C / Ctrl+X / Ctrl+V | Copy / Cut / Paste the current selection |
| Ctrl+A | Select all |
| Ctrl+R | Force full e-paper refresh (clears ghosting; e-paper boards only) |
| Ctrl+B | Cycle backlight / front-light brightness (boards with a controllable backlight) |
| Ctrl+Home/End | Start / end of document |
| Ctrl+Left/Right | Word movement |
| Ctrl+1 | Single-pane mode (full-width editor) |
| Ctrl+2 | Split screen into two equal-width panes |
| Ctrl+3 | Split with the left pane at 2/3 width; press again to toggle the left pane to 1/3 |
| Ctrl+Tab | Move keyboard focus to the other pane (when split) |
| Escape | Switch to file browser. With unsaved changes, a dialog offers Save and exit / Exit without saving / Cancel (Up/Down + Enter to choose) |

## Pairing a New Keyboard

If you want to pair a different keyboard (or re-pair after a factory reset
of the keyboard), you need to erase the stored BLE bond first.  There are
three ways to trigger **Forget All Keyboards**:

- **Wakeup / boot button -- 2-second hold**: on every board that has a
  wakeup or boot button (GPIO18 on the Waveshare RLCD-4.2, GPIO0 on most
  others), hold the button for at least 2 seconds.  The device immediately
  drops the current connection, clears all stored pairings, and starts
  scanning for a new keyboard.
- **Side key on LilyGO T5 E-Paper S3 Pro H752 -- 2-second hold**: the side
  key on GPIO48 normally injects F1 (menu) on a short press.  Holding it
  for 2 seconds triggers Forget All instead.
- **"Forget KB" button on the BLE-prompt screen** (boards with a
  touchscreen): when the device is waiting for a keyboard to connect, the
  BLE-prompt screen shows a small **Forget KB** button to the left of the
  **Off** button.  Tapping it has the same effect as the 2-second hold.

After triggering any of these actions the device starts scanning
immediately; connect your new keyboard and it will pair automatically.

## Split-screen Editing

The editor can show two documents side by side. **Ctrl+2** divides the
screen into two equal-width vertical panes; **Ctrl+3** makes the left
pane wider (2/3 of the width) and a second **Ctrl+3** flips the left
pane to 1/3. **Ctrl+1** returns to a single full-width pane.

Each pane opens a file for itself: while split, **Ctrl+O** opens the
file browser for the focused pane, so the picked file loads into that
pane while the other pane keeps its document. **Ctrl+Tab** moves
keyboard focus between the two panes; the focused pane shows the active
cursor and receives all editing keys.

Opening the same file in both panes shares a single in-memory copy of
the document (the panes are two views of the same buffer), so you can
edit or read two parts of one file at once and edits in one pane are
reflected in the other. The current split layout is remembered across
reboot and deep sleep.

## Touch Operations

On boards with a touchscreen, touch input works alongside the
Bluetooth keyboard -- you can use either, or both. All gestures are
summarized below.

### In the editor

| Gesture | Action |
|---------|--------|
| Single tap | Move the cursor to the tapped position |
| Double tap | Select the word at the tapped position |
| Drag up / down | Scroll the document line by line, following the finger (one line per line-height of travel) |
| Swipe up / down (fast flick) | Scroll by roughly one screen |

A drag that moves more than a few pixels never moves the caret -- the
tap-to-cursor action only fires for short, stationary taps.

On the e-paper PaperS3 the gestures are the same, but the slower
refresh rate (~80-150 ms per partial update, several seconds for a
full refresh) means the visible response to a drag is less smooth
than on the color-LCD JC3248W535.

### In menus and the file browser

| Gesture | Action |
|---------|--------|
| Tap a row | Highlight that row (same as moving with arrow keys) |
| Tap the highlighted row again | Activate it (same as pressing Enter) |

This two-step "highlight then activate" flow mirrors the keyboard
"arrows + Enter" interaction and avoids accidental activations on
imprecise taps.


## Supported hardware

Draftling currently runs on seven development boards (six ESP32-S3 and
one ESP32-P4). All of them share the same firmware image; the target
board is picked at build time with `idf.py menuconfig` -> **DRAFTLING
Configuration > Hardware Model**. Display resolution, driver, pin map,
touch controller and the deep-sleep wake source are derived
automatically from that choice.

- **[Waveshare
  ESP32-S3-RLCD-4.2](https://www.waveshare.com/wiki/ESP32-S3-RLCD-4.2)**
  -- 4.2" reflective LCD (400x300, SPI). No touch. Battery monitored
  on GPIO4 (3:1 divider). GPIO18 button wakes from deep
  sleep. On-board MicroSD on the SDMMC 1-bit peripheral.

- **[LilyGO T5 E-Paper S3
  Pro](https://github.com/Xinyuan-LilyGO/T5S3-4.7-e-paper-PRO)** --
  4.7" e-paper ED047TC1 (960x540) driven over a parallel bus by
  `vroland/epdiy` (`epd_board_v7`). GT911 capacitive touch on I2C.
  BQ27220 fuel gauge on I2C (0x55). BOOT (GPIO0) wakes from deep
  sleep. On-board MicroSD on SPI3 (shared with the SX1262 LoRa CS).

- **[LilyGO T5 E-Paper S3 Pro
  Lite](https://github.com/Xinyuan-LilyGO/T5S3-4.7-e-paper-PRO)** --
  Same as the Pro variant minus the SX1262 LoRa radio and MIA-M10Q
  GPS; on-board MicroSD lives alone on SPI3.

- **[LilyGO T5 E-Paper S3 Pro
  (H752)](https://github.com/Xinyuan-LilyGO/T5S3-4.7-e-paper-PRO)** --
  Original pre-"H752-01" revision (v1.0-240810) of the LilyGO T5
  E-Paper S3 Pro. Same 4.7" ED047TC1 panel (960x540) and GT911 touch
  as the current Pro / Pro Lite, but without the PCA9535 IO expander
  and TPS65185 PMIC, so the panel is driven by the vendored FastEPD
  library (`components/fastepd`) instead of `vroland/epdiy`. The side
  key on GPIO48 acts as a Menu key (injects F1); GPIO48 is not an RTC
  IO, so standby uses light sleep + `gpio_wakeup` + `esp_restart`
  rather than EXT0 deep sleep. The capacitive touch key below the
  panel acts as Back (injects Esc).

- **[M5Stack PaperS3](https://docs.m5stack.com/en/core/papers3)** --
  4.7" e-paper ED047TC1 (540x960) driven over a parallel I80 bus by
  the `vroland/epdiy` managed component with a PaperS3-specific
  board definition (`components/display/epd_board_papers3.c`).
  GT911 touch on I2C. Battery on GPIO3 ADC (1:2).
  BOOT (GPIO0) wakes from deep sleep (optionally any touch with
  `CONFIG_DRAFTLING_STANDBY_WAKE_ON_TOUCH`). On-board MicroSD on
  SPI3. **This board is officially discontinued by M5Stack.**

- **[Waveshare
  ESP32-S3-Touch-LCD-3.49](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-3.49)**
  -- 3.49" IPS color LCD (640x172, AXS15231B over
  QSPI). AXS5106-family capacitive touch on I2C (0x3B). BOOT (GPIO0)
  wakes from deep sleep. External SD on a separate SPI bus.

- **[M5Stack Tab5](https://docs.m5stack.com/en/core/Tab5)** --
  ESP32-P4 tablet with a 5" 1280x720 IPS color LCD (ILI9881C or
  ST7123, MIPI-DSI 2-lane, auto-detected by the upstream
  `espressif/m5stack_tab5` BSP). GT911 capacitive touch on I2C (backup
  address 0x14). Li-ion battery + PMIC + USB-C charging.  Wi-Fi /
  Bluetooth are routed through an on-board ESP32-C6 co-processor via
  ESP-Hosted-MCU over SDIO; both `wifi_manager` (Git sync) and
  `ble_keyboard` (BLE HID host) work the same as on the ESP32-S3
  boards. The C6 must be flashed once with the matching ESP-Hosted
  slave firmware -- see
  [docs/tab5-esp-hosted.md](docs/tab5-esp-hosted.md)` for the
  procedure. Touch and the on-board MicroSD slot work; this board has
  been added without on-hardware testing and will likely need bring-up
  tweaks.

- **Guition JC3248W535** --
  3.5" IPS color LCD (480x320, AXS15231B over QSPI). AXS5106L
  capacitive touch on I2C. No user buttons -- the touch INT line is
  the deep-sleep wake source. External SD on a separate SPI bus.

- **Sunton [ESP32-8048S043C](docs/sunton-esp32-8048S043.md) and
  [ESP32-8048S070C](docs/sunton-esp32-8048S070c.md)** -- ESP32-S3 HMI
  development boards with 16-bit parallel RGB565 interface and a GT911
  capacitive touch controller.

All ESP32-S3 boards use at least 8 MB of PSRAM and 16 MB of flash, BLE
for the HID keyboard and 802.11 b/g/n Wi-Fi for Git sync. The Tab5
board uses 32 MB HEX-mode PSRAM on the ESP32-P4 and reaches the same
BLE / Wi-Fi functionality through its on-board ESP32-C6 co-processor
via ESP-Hosted-MCU (see above and `docs/tab5-esp-hosted.md`).

A few [demo
videos](https://youtube.com/playlist?list=PLbRMZQ9npKJRDrk0BhtI4gXMBIHM0c_v_)
are available on my YouTube channel.

### Choosing a board

See the [photo gallery](images/README.md) of some of supported
hardware.

The **Waveshare ESP32-S3-RLCD-4.2** provides a pretty smooth and
responsive user interaction. But the screen is very fragile (the
screen broke during my tests), and the device needs an enclosure. A
[3D printed enclosure model is
available](3D_Prints/Waveshare_ESP32-S3-RLCD-4.2/).  The contrast is
very low, so it needs good lighting for comfortable work.

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

The **M5Stack PaperS3** is compact, packed in a good enclosure with
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

The **Waveshare ESP32-S3-Touch-LCD-3.49** drives a 640x172 landscape
AXS15231B color LCD (natively 172x640 portrait, software-rotated to
landscape) and an AXS5106-family capacitive touch controller at I2C
address 0x3B. The BOOT button on GPIO0 is the deep-sleep wake source.
The on-board PWR / home button on GPIO16 is reserved for the power
latch (short press to power on, long press to power off via the
TCA9554 IO6 latch) and is not used to wake the MCU from deep sleep.

The **M5Stack Tab5** is a much faster device than those using
ESP32-S3, and it has a detachable battery. You can buy compatible
batteries with a built-in fast USB charger. The device supports a USB
keyboard as an alternative too BLE. Also, M5 produces a specialized
keyboard ayttachment for Tab5, which will be supported by FDraftling
in future releases.

The **Guition JC3248W535** is a color-LCD board with no user buttons,
so touch is the only local input besides the BLE keyboard: deep-sleep
wake is armed on the touch INT line and any tap wakes the device.
Touch also drives the editor and the menu lists; see
[Touch Operations](#touch-operations) below.

UC8179-based e-paper displays (such as those used by the Seeed Studio
reTerminal E1001 and the Waveshare E-Paper Driver HAT) were previously
supported but proved too slow for an interactive Markdown editor: even
with fast partial updates, the panel cannot keep up with typing and
quickly accumulates ghosting artefacts. Support for UC8179 has
therefore been removed from the codebase.

On color LCD boards (Touch-LCD-3.49, JC3248W535) the
editor offers a runtime-selectable color theme (F1 -> Settings ->
Color theme): light green on black (default), dark green on black,
amber/orange on black, or white on black.



### Wake from sleep

On boards without user buttons (such as the JC3248W535) the touch
controller's INT line is wired to the deep-sleep wake source, so any
tap on the screen wakes the device.

The M5Stack PaperS3 has a BOOT button on GPIO0 which is the default
deep-sleep wake source; the GT911 touchscreen is used as a regular
input device but not as a wake source by default. Set
`CONFIG_DRAFTLING_STANDBY_WAKE_ON_TOUCH=y` in menuconfig to wake on
touch instead.

## Keyboard Layouts

The editor supports five keyboard layouts that can be switched with
**Ctrl+L** (or **Win+Space**) or through the **F1 menu**:

| Code | Layout |
|------|--------|
| US | US-English (QWERTY) |
| UA | Ukrainian (Cyrillic) |
| DE | German (QWERTZ with umlauts) |
| FR | French (AZERTY with accents) |
| HE | Hebrew (Israeli standard) |

The current layout is shown in the title bar.

### Configuring enabled layouts

The set of compiled-in keyboard layouts is configurable via
`idf.py menuconfig` under **DRAFTLING Keyboard Layouts**. Each layout
can be independently enabled or disabled. By default **US-English** and
**Ukrainian** are enabled. Disabling unused layouts saves flash space.

## Building

Requires [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/stable/)
v5.3 or later (v5.3 - v5.5 confirmed working).

> **Note:** the e-paper boards (M5Stack PaperS3 and LilyGO T5 E-Paper
> S3 Pro / Pro Lite) currently require **ESP-IDF 5.5.x**. The
> `vroland/epdiy` driver is not yet compatible with ESP-IDF 6.x;
> please stay on the 5.5.x release line for these devices until the
> driver becomes compatible.

Keep in mind that M5Stack Tab5 uses a different MCU, so the tatget
should be set to 'esp32s4'.

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

If you update `sdkconfig.defaults` or pull new changes, delete the generated
`sdkconfig` so the defaults are re-applied:

```bash
rm -f sdkconfig
idf.py set-target esp32s3
idf.py build
```

## Menuconfig Options

Run `idf.py menuconfig` to open the interactive configuration UI.
Draftling adds three custom menus described below (**DRAFTLING
Configuration**, **DRAFTLING Keyboard Layouts** and **DRAFTLING
Editor**). All other options (Bluetooth, LVGL fonts, etc.) use the
ESP-IDF defaults from `sdkconfig.defaults` and normally do not need
to be changed.

### DRAFTLING Configuration

Found at the top-level **DRAFTLING Configuration** menu.

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| **Hardware Model** | choice | M5Stack PaperS3 | Select the target board. Display resolution, driver, pin map, touch controller and the deep-sleep wake source are derived automatically. See the [Supported hardware](#supported-hardware) section for all six options. |
| **Display rotation angle** | choice | 0 degrees | Rotate the display by 0, 90, 180, or 270 degrees. |
| **E-paper full-refresh interval** | int | 30 | E-paper boards only: number of partial refreshes between full refreshes. |
| **Enable touchscreen input** | bool | y on PaperS3 and JC3248W535, n otherwise | Enable the I2C touch driver and LVGL pointer input device. |
| **Standby: wake from deep sleep on touchscreen tap** | bool | y on JC3248W535, n otherwise | Arm EXT0 on the touch INT line so any tap wakes the device. |

Backlight brightness on color-LCD boards is not a menuconfig option:
it is set at runtime from F1 -> Settings -> Backlight (or the
Ctrl+B shortcut) and persisted in NVS (default 50%). The backend
drives the BL GPIO with an LEDC PWM signal (~5 kHz, 10-bit). 0% =
off, 100% = full brightness.

The LilyGO T5 E-Paper S3 Pro / Pro Lite carry a controllable white
front-light (GPIO 11) which uses the same Settings entry / Ctrl+B
shortcut. Because the e-paper panel is reflective and remains
readable without any illumination, the cycle on these boards also
includes a 0 % step that fully turns the front-light off.

**Note about e-paper boards (M5Stack PaperS3, LilyGO T5 E-Paper S3
Pro / Pro Lite):** the epdiy-based driver uses the single-pulse
`EPD_MODE_FAST` waveform for partial refreshes (one visible flash,
~80-150 ms per update). A full refresh (3-5 s) is performed
automatically every `DRAFTLING_EPD_FULL_REFRESH_INTERVAL` partials
(default 30) to clear residual ghosting; tune the interval in
`idf.py menuconfig`.

### DRAFTLING Keyboard Layouts

Found at the top-level **DRAFTLING Keyboard Layouts** menu. Each layout
can be independently enabled or disabled. Disabling unused layouts saves
flash space.

| Option | Default | Description |
|--------|---------|-------------|
| US-English (QWERTY) | **y** | Standard US keyboard layout |
| Ukrainian (Cyrillic) | **y** | Ukrainian Cyrillic layout |
| German (QWERTZ) | n | German layout with umlauts |
| French (AZERTY) | n | French layout with accents |

## Configuration Files

Place these on the SD card root:

### WiFi (`wifi.cfg`)

```
MySSID
MyPassword
```

### Git Sync (`git.cfg`)

```
repo_url=https://github.com/user/repo
branch=main
token=ghp_xxxxxxxxxxxx
path=docs/
```

The `token` is a GitHub Personal Access Token with `repo` scope.
**Keep this file private.**

## Project Structure

```
main/               Application entry point, pin definitions, Kconfig
  boards/           Per-board pin definitions (one .h file per board)
components/
  battery/           Battery monitor: ADC + smoothing, or BQ27220
                     fuel gauge over I2C (T5 E-Paper S3 Pro / Lite)
  ble_keyboard/      BLE HID keyboard host (Bluedroid)
  display/           Display backends (RLCD SPI, epdiy e-paper for
                     PaperS3 + LilyGO T5, AXS15231B QSPI,
                     MIPI-DSI for Tab5) and LVGL v9 port
  editor/            Gap-buffer editor, Markdown parser, LVGL UI, menu
  fonts/             Custom LVGL fonts (Latin, Latin-1 Supplement, Cyrillic)
  git_sync/          GitHub REST API file synchronization
  kb_layout/         Keyboard layout translation (US/UA/DE/FR)
  sd_card/           SD card (SDMMC or SPI) file operations
  standby/           Deep-sleep / standby timer manager
  touchscreen/       I2C touch driver (AXS5106L, GT911) + LVGL pointer indev
  wifi_manager/      WiFi STA connection manager
```

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for
details.

Copyright (c) 2026 clackups@gmail.com

Fediverse: [@clackups@social.noleron.com](https://social.noleron.com/@clackups)
