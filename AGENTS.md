# Copilot Instructions

## Code Style

- Do not use non-ASCII characters anywhere in the repository -- in code,
  comments, string literals, commit messages, or documentation (including
  Markdown files under `docs/` and this `AGENTS.md`). All files must be
  ASCII-only. Use ASCII equivalents instead: `u` (or `micro`) for `mu`,
  `->` for an arrow, `--` for an em-dash, `>=` / `<=` for the inequality
  glyphs, `ohm` for the ohm symbol, `deg` for the degree sign, `sec.` for
  the section sign, etc. The only exception is the auto-generated LVGL
  font sources under `components/fonts/` (e.g. `greybeard_*.c`), whose
  comments reference the Unicode glyphs they encode.
- All board-specific configuration -- pin numbers, panel dimensions,
  wakeup GPIOs, brand strings, etc. -- belongs in `main/Kconfig.projbuild`
  and the per-board header files under `main/boards/`. C and C++ files
  outside `main/` must NOT reference specific board models (no
  `#if defined(CONFIG_DRAFTLING_MODEL_*)`, no hard-coded board names).
  Use the derived feature flags (`CONFIG_DRAFTLING_DISPLAY_EPD`,
  `CONFIG_DRAFTLING_DISPLAY_RLCD`, `CONFIG_DRAFTLING_DISPLAY_HAS_BACKLIGHT`,
  `CONFIG_DRAFTLING_HAS_BATTERY`, `CONFIG_DRAFTLING_SD_SDMMC`,
  `CONFIG_DRAFTLING_WAKEUP_GPIO`, etc.) exposed by `main/Kconfig.projbuild`
  instead. To add a new model, introduce a new `DRAFTLING_MODEL_*` choice
  option, set the matching derived flags / `default` lines for the existing
  feature symbols, create a new per-board header file under `main/boards/`
  defining the board's `BOARD_NAME`, pin numbers and `WAKEUP_GPIO_NUM`, add
  the corresponding `#elif` include directive in `main/app_config.h`, and
  update `main/main.cpp`'s display / SD init switch as needed -- no other
  C / C++ file should require changes.

## Project Overview

Draftling is a distraction-free Markdown text editor for ESP32-S3-based
development boards with reflective LCD displays. It is built with the
ESP-IDF framework (v5.3+) and uses LVGL v9 for the graphical interface.

The user connects a Bluetooth keyboard and edits Markdown files stored on
a MicroSD card. The reflective LCD needs no backlight and works well in
daylight. On request the device connects to WiFi and synchronizes files
with a remote Git repository via the GitHub REST API.

### Supported Hardware

| Board | Display |
|-------|---------|
| Waveshare ESP32-S3-RLCD-4.2 | 4.2-inch reflective LCD, 400x300 |
| M5Stack PaperS3 | 4.7-inch e-paper (ED047TC1), 540x960 |

UC8179-based panels (Seeed Studio reTerminal E1001 and the Waveshare
E-Paper Driver HAT) were previously supported but have been removed:
the controller proved too slow for an interactive Markdown editor
even with fast partial updates and accumulated ghosting too quickly
to be usable.

## Repository Layout

```
CMakeLists.txt              Top-level CMake project file
partitions.csv              Custom partition table (16 MB flash)
sdkconfig.defaults          Common Kconfig defaults for all targets
sdkconfig.defaults.esp32s3  ESP32-S3-specific defaults (PSRAM, BLE, WiFi)
main/                       Application entry point and hardware config
  main.cpp                  app_main(): initializes all subsystems
  app_config.h              Display macros; includes the active board header
  boards/                   Per-board pin definitions (one .h file per board)
  Kconfig.projbuild         Menuconfig: hardware model, display size, rotation
  idf_component.yml         IDF component manifest (depends on lvgl ^9.2)
  CMakeLists.txt            Registers main as an IDF component
components/                 Reusable IDF components
  battery/                  Battery voltage monitor (ADC)
  ble_keyboard/             BLE HID keyboard host (Bluedroid)
  display/                  RLCD SPI display driver and LVGL port
  editor/                   Gap-buffer text editor, Markdown parser, LVGL UI
  fonts/                    Custom LVGL bitmap fonts (Greybeard family)
  git_sync/                 GitHub REST API file synchronization
  kb_layout/                Keyboard layout translation (US/UA/DE/FR)
  power/                    TCA9554-latched battery rail + PWR-button driver
  sd_card/                  SD card (SDMMC 1-bit) file operations
  standby/                  Deep-sleep / standby inactivity timer
  tab5_kbd/                 M5Stack Tab5 attachable keyboard (I2C + INT)
  wifi_manager/             WiFi STA connection manager
  usb_kbd/                  USB HID keyboard host (boot protocol)
```

## Component Details

### main/

The application entry point. `app_main()` in `main.cpp` initializes every
subsystem in order: NVS flash, display hardware, LVGL, battery monitor,
editor UI, SD card, BLE keyboard, WiFi manager, Git sync, and standby
timer. It also registers an auto-save callback that persists the current
document before entering deep sleep.

`app_config.h` defines display-dimension and scale macros from Kconfig
symbols and then delegates all board-specific pin constants to a thin
per-board header included from `main/boards/`. Each file in `main/boards/`
(e.g. `waveshare_rlcd42.h`, `m5stack_papers3.h`) defines exactly one board:
`BOARD_NAME`, SPI pins (MOSI, SCK, DC, CS, RST, TE), SD card pins
(CLK, CMD, D0 for SDMMC, or MOSI/MISO/SCK/CS for SPI), I2C pins, the
battery ADC pin, and the deep-sleep wakeup GPIO. The M5Stack PaperS3
header omits panel data-bus / control-line pins because the `vroland/epdiy`
driver configures them internally from the in-tree PaperS3 board definition
(`components/display/epd_board_papers3.c`).

### components/battery/

Two backends:

* **ADC + resistive divider** (`battery_init(gpio, en, divider)`):
  reads the cell voltage through a configurable ADC pin and applies
  exponential moving average smoothing over 8 samples. Voltage maps
  to percentage as >=4.10 V = 100 %, >=3.95 V = 75 %, >=3.80 V = 50 %,
  >=3.60 V = 25 %, below 3.60 V = 0 %. The M5Stack PaperS3 reads its
  cell through ADC1 on GPIO3 with a 1:2 divider, matching the
  M5Unified Power_Class configuration for that board. When
  `BATT_ADC_PIN < 0`, `battery_init()` is a no-op.
* **TI BQ27220 fuel gauge** over I2C (`battery_init_bq27220(bus)`):
  used on the LilyGO T5 E-Paper S3 Pro / Pro Lite, where the cell is
  routed through a BQ25896 charger + BQ27220YZFR coulomb counter at
  0x55 on the I2C bus shared with epdiy (TPS65185 / PCA9535) and
  GT911. main.cpp creates the bus and passes its handle in. Voltage
  GT911. main.cpp creates the bus and passes its handle in. Voltage
  comes from the Voltage register (0x08, mV), and percentage is
  derived from that voltage via the same LiPo discharge LUT used by
  the ADC backend -- the gauge's StateOfCharge register (0x2C) is
  ignored because the factory ships it with default Data Memory and
  its Impedance-Track SoC stays pinned around 50 % even after
  several full discharge/charge cycles. Charge state is
  derived from the Flags register (0x06): bit 0 (`DSG`) is 0 while
  charging or full and 1 while discharging.
* **TI INA226 power monitor** over I2C
  (`battery_init_ina226(bus, addr, cells)`): used on the M5Stack
  Tab5. Bus voltage (register 0x02) is divided by the cell count
  to derive per-cell mV. Charge state comes from the signed
  shunt-voltage register (0x01): positive shunt voltage means
  current is flowing into the pack (the Tab5 wires the shunt so
  the IP2326 charger pushes positive current into the cell). See
  `docs/tab5-esp-hosted.md` for the CHG_EN enable path.

When no backend has been initialized, `battery_read_percent()`
returns -1 and the editor UI hides the battery indicator.
The INA226 backend also returns -1 when the per-cell BUS voltage
drops below ~2.8 V, which is interpreted as "no battery attached"
(otherwise the floating cell rail would be misread as ~14-18 % via
the Li-ion discharge curve).
Similarly `battery_read_charging()` returns -1 on backends that
cannot detect charge state (the GPIO-ADC backend), so the UI auto-
hides the `+` charging glyph on those boards.

Public API: `battery_init()`, `battery_init_bq27220()`,
`battery_init_ina226()`, `battery_read_mv()`,
`battery_read_percent()`, `battery_read_charging()`.

### components/ble_keyboard/

BLE HID keyboard host built on ESP-IDF Bluedroid. Handles device scanning,
pairing with passkey authentication, connection/disconnection callbacks,
and keyboard event dispatching. Each key event carries the HID keycode,
ASCII character, modifier flags, and pressed/released state.

Public API: `ble_keyboard_init()`, `ble_keyboard_start_scan()`,
`ble_keyboard_is_connected()`, `ble_keyboard_set_callback()`,
`ble_keyboard_forget_all()`, and several other callback registration
functions.

`ble_keyboard_forget_all()` erases every stored keyboard bond (both from
the Bluedroid stack and from NVS), disconnects any currently-connected
keyboard, resets the reconnection state machine, and immediately starts a
fresh scan. After the call the device behaves as if it has never paired
with any keyboard. It can be triggered in three ways: a 2-second hold of
the wakeup / boot button (all boards that define `WAKEUP_GPIO_NUM`), a
2-second hold of the side key on the LilyGO T5 E-Paper S3 Pro H752
(GPIO48), or the "Forget KB" touch button on the BLE-prompt screen
(boards with `CONFIG_DRAFTLING_TOUCHSCREEN`).

### components/display/

Per-board display backends behind a single C API:

- **display_rlcd.cpp** -- Waveshare ESP32-S3-RLCD-4.2 reflective LCD
  over SPI.
- **display_epdiy.cpp** -- E-paper backend for the M5Stack PaperS3
  ED047TC1 panel and the LilyGO T5 E-Paper S3 Pro / Pro Lite
  ED047TC2 panel, both driven by the `vroland/epdiy` managed
  component. epdiy keeps a 4-bpp grayscale framebuffer (one
  nibble per panel pixel, ~253 KB at 960x540) in PSRAM; the LVGL
  port pushes RGB565 pixels straight into it through the optional
  `display_push_rgb565()` fast path, with each LVGL pixel scaled
  into a SCALE x SCALE block of panel pixels
  (`CONFIG_DRAFTLING_DISPLAY_SCALE`, default 2). The backend
  accumulates a dirty bounding box across pushes and, on
  `display_flush()`, powers on the EPD rail and runs an
  `epd_hl_update_area()` partial refresh over the bbox using the
  single-pulse `EPD_MODE_FAST` waveform (one visible flash,
  ~80-150 ms); every `CONFIG_DRAFTLING_EPD_FULL_REFRESH_INTERVAL`
  partials (or whenever the dirty area covers most of the screen,
  or after `display_clear()` / `display_full_refresh()`) the next
  refresh is promoted to a full-screen `MODE_GC16` flashing
  update to clear ghosting. The LilyGO T5 path selects epdiy's
  built-in `epd_board_v7`; the PaperS3 path selects the in-tree
  `epd_board_papers3` defined in `epd_board_papers3.c`, which
  drives EPD_EN (GPIO 45) and BST_EN (GPIO 46) directly (no
  TPS65185 / PCA9555 expander) and uses the LCD peripheral for
  STH/CKH/STV/CKV/XLE/D0-D7. Grayscale UI is still TODO.
- **lvgl_port.cpp** -- creates the LVGL display object, sets up a
  flush callback that first tries `display_push_rgb565()` (used by
  the epdiy and AXS15231B backends) and otherwise converts LVGL's
  RGB565 output to the backend's 1-bpp pixel format via
  `display_set_pixel()`, and runs the LVGL tick/task timer. Thread
  safety is provided by a mutex exposed as `lvgl_port_lock()` /
  `lvgl_port_unlock()`.

The component's `idf_component.yml` declares the `vroland/epdiy`
dependency required by both e-paper backends; the source files
themselves are gated by `CONFIG_DRAFTLING_DISPLAY_EPDIY` so
non-e-paper builds do not link epdiy into the final image.

Public API: `display_init()`, `display_clear()`, `display_set_pixel()`,
`display_flush()`, `display_full_refresh()`, `display_push_rgb565()`,
`display_set_partial_clip()`, `display_set_backlight()`,
`display_get_buffer()`,
`display_get_buffer_size()`, `lvgl_port_init()`,
`lvgl_port_lock()`, `lvgl_port_unlock()`.

### components/editor/

The largest component. Contains:

- **editor.cpp** -- gap-buffer text engine with a document size limit
  sized dynamically at `editor_init()` from the PSRAM that is free
  when the editor starts (the gap buffer and flat cache are each
  allocated at that size, so the editor's SPIRAM cost is ~2x the
  limit; the value is clamped to a minimum of 64 KB and an upper
  bound returned by `git_sync_max_file_size()` -- so the editor never
  produces a document larger than what git_sync can push -- and a
  ~512 KB headroom is reserved inside git_sync_max_file_size() for
  BLE / WiFi / Git sync / LVGL widget growth). Exposes `editor_get_max_doc_size()` for the UI,
  which surfaces the value read-only in F1 -> Settings. Provides
  cursor movement, selection, clipboard, insert/delete, and file I/O.
- **editor_ui.cpp** -- LVGL-based user interface: title bar with battery
  and layout indicators, scrollable text area with Markdown rendering,
  status bar, file browser dialog, and settings menu (F1).
- **md_parser.cpp** -- single-pass Markdown line parser. Recognizes
  headings H1-H4, bullet and numbered lists, blockquotes, code fences,
  horizontal rules, and inline bold/italic/code/strikethrough spans.
- **draftling_logo.c** -- embedded LVGL image for the splash screen.

The F1 menu opens an in-line **Settings** list with: standby
timeout, base font size, **Backlight** (NN%, only on boards with
`CONFIG_DRAFTLING_DISPLAY_HAS_BACKLIGHT` -- the value is persisted
in NVS under the `editor` namespace and applied at boot via
`display_set_backlight()`; default 50%), color theme (only on
`CONFIG_DRAFTLING_DISPLAY_COLOR`), sleep-now, factory reset and
back. Picking a new color theme does NOT reboot the device:
`rebuild_screens_for_theme()` deletes every screen / overlay /
screen-bound timer, re-runs `init_styles()` under the new palette,
calls `build_screens()` again and restores the screen the user was
on. The persistent state (open document, NVS-backed font size /
theme / backlight / standby timeout) survives the rebuild.

Public API: `editor_init()`, `editor_open_file()`, `editor_save_file()`,
`editor_ui_init()`, `editor_ui_handle_key()`, `editor_find()`,
`editor_replace_range()`, `md_parse_line()`, and many
cursor/selection/clipboard helpers.

The editor stores text as **UTF-8** internally. `editor_open_file()`
inspects the leading bytes of each file for a Unicode BOM and
transcodes on the fly so that files saved from desktop editors render
correctly:

| BOM bytes          | Encoding   | Handling                          |
|--------------------|------------|-----------------------------------|
| `EF BB BF`         | UTF-8      | BOM stripped, rest stored as-is   |
| `FF FE`            | UTF-16 LE  | Transcoded to UTF-8 via `transcode_utf16_to_utf8()` |
| `FE FF`            | UTF-16 BE  | Transcoded to UTF-8 via `transcode_utf16_to_utf8()` |
| (none)             | UTF-8      | Stored as-is                      |

Without this conversion a UTF-16 file (Windows Notepad still defaults
to "Unicode" = UTF-16 LE for many scripts) would arrive as alternating
text bytes and `0x00`/`0x05` filler, and Hebrew / Cyrillic / CJK runs
would render as random Latin-1 glyphs. The transcoder decodes UTF-16
surrogate pairs into single supplementary-plane codepoints, replaces
unpaired surrogates with U+FFFD, and silently drops the CR (U+000D)
half of Windows CRLF line endings.

Editor shortcuts include `Ctrl+F` (Find) and `Ctrl+H` (Find +
Replace). Both open a modal overlay; in Find+Replace mode, `Tab`
switches between the Find and Replace fields, `Enter` jumps to the
next match (wrapping at end-of-document), and `Ctrl+Enter` replaces
the current match and advances to the next.

The keyboard layout is cycled with `Ctrl+L` or, equivalently, with
`Win+Space` (the GUI modifier plus HID keycode 0x2C); both call
`kb_layout_next()` from `handle_editor_key()`.

The editor UI supports a vertical (left/right) **split screen** built
on the multi-document engine. `editor_ui.cpp` wraps the per-pane UI
state (container, cursor, logo, line-label / selection-rect arrays,
render caches, bound `editor_doc_t`, and pane geometry x / w / y / h)
in a `pane_t` struct held in a fixed `s_panes[EDITOR_MAX_PANES]`
(EDITOR_MAX_PANES = 2) pool, mirroring the `editor_doc_t` aliasing
pattern in `editor.cpp`: macros alias the historical widget globals
(`s_cont_edit`, `s_cursor`, ...) to `s_rp->...` where `s_rp` is the
currently rendered pane. `pane_bind()` / `pane_bind_focus()` point
`s_rp` and the engine's active document at a pane before drawing or
handling a key. `editor_ui_refresh()` loops the active panes
(unfocused first, focused last) calling `refresh_active_pane()` for
each, then updates the title / battery once. The split layout is
driven by `split_mode_t` (SPLIT_NONE, SPLIT_HALF, SPLIT_LEFT_2_3,
SPLIT_LEFT_1_3); `recalc_pane_geometry()` keeps both panes full-height
and only varies x / width (1 px divider via `s_pane_divider`).
Shortcuts: `Ctrl+1` single pane (`SPLIT_NONE`), `Ctrl+2` equal split
(`SPLIT_HALF`), `Ctrl+3` left-2/3 then toggling to left-1/3
(`editor_ui_cycle_wide_split()`), `Ctrl+Tab` move focus between panes
(`editor_ui_focus_other_pane()`). Digit HID keycodes (1 = 0x1E,
2 = 0x1F, 3 = 0x20) and `KB_KEY_TAB` are matched before the a..z Ctrl
switch in `handle_editor_key()`. While split, the file browser
(`Ctrl+O`) targets the focused pane via `open_into_pane()`, so each
panel opens a file for itself (focus a pane, then `Ctrl+O`); opening
the same path in both panes shares one refcounted buffer (two views of
one document). Enabling a split acquires a fresh untitled document
into pane 1; collapsing keeps it open in the background. The split
mode persists in NVS (`editor` namespace, key `split`) via
`save_split_to_nvs()` / `load_split_from_nvs()` and is restored at the
end of `editor_ui_init()` (only the layout is restored, not the open
documents, matching the existing boot-to-file-browser behavior).
Auto-save, the standby pre-sleep callback, and `Ctrl+G` git-sync all
iterate every open document via `editor_doc_foreach()`.

`Esc` in the editor leaves to the file browser. When the document has
unsaved changes it instead opens a small modal overlay (`s_exit_panel`,
handled by `handle_exit_prompt_key()`) offering three choices --
"Save and exit", "Exit without saving" and "Cancel (keep editing)" --
navigated with `Up`/`Down` and confirmed with `Enter`; `Esc` inside
the dialog cancels. For an untitled document, "Save and exit" first
opens the save-as prompt so the user can name the file.

The title bar shows `L %d/%d` (current line / total lines) on every
build; on non-EPD targets the column counter is appended as well.

The bottom status bar of both the editor and the file browser
displays a small Wi-Fi icon (`components/editor/wifi_icon.c`) in the
right corner whenever `wifi_manager_is_connected()` is true. The base
Greybeard fonts cover U+0020-U+00FF plus a few currency / numero
glyphs, which excludes the U+1F6DC "wireless" pictograph, so the
icon is rendered from a small embedded LVGL `LV_COLOR_FORMAT_I1`
image instead of as a font glyph. Two pre-baked descriptors are
exposed (black-on-transparent for the default theme and
white-on-transparent for `CONFIG_DRAFTLING_EPD_BLACK_BACKGROUND`).

### components/fonts/

Custom LVGL bitmap fonts generated from the Greybeard typeface. See the
dedicated section below for the full creation process.

### components/git_sync/

Synchronizes Markdown files between the SD card and a remote GitHub
repository using the GitHub REST API over HTTPS. Reads configuration
(repo URL, branch, token, path prefix) from `/sdcard/git.cfg`. Supports
pull, push, or bidirectional sync with state callbacks and error tracking.

The last-synced per-file blob SHAs are persisted in a hidden
`.git_state` file under the SD mount point. Every sync does a 3-way
comparison (saved vs local vs remote) so that:

- a file modified only on one side is propagated to the other;
- a file deleted on the remote is also deleted locally (provided the
  local copy still matches the last-synced SHA); otherwise the local
  edits "win" and the file is recreated on the remote next push;
- a file deleted locally is also deleted on the remote (via the GitHub
  Contents DELETE API, conditional on the saved SHA still being current);
  otherwise the remote-modified copy "wins" and is re-downloaded next
  pull;
- a file renamed on either side is handled correctly because Git models
  renames as a delete + add of two distinct paths, which the rules
  above cover automatically (no duplicate copies left behind).

Public API: `git_sync_init()`, `git_sync_start()`,
`git_sync_get_state()`, `git_sync_is_configured()`.

### components/kb_layout/

Translates HID keycodes and modifier flags into UTF-8 character strings
for the active keyboard layout. Supports US-English (QWERTY), Ukrainian
(Cyrillic), German (QWERTZ), and French (AZERTY). Each layout can be
independently enabled or disabled at build time via Kconfig (see
`components/kb_layout/Kconfig.projbuild`). The active layout is cycled
at runtime with `kb_layout_next()` (bound to `Ctrl+L` and `Win+Space`
in the editor).

Public API: `kb_layout_translate()`, `kb_layout_set()`, `kb_layout_get()`,
`kb_layout_name()`, `kb_layout_next()`.

### components/sd_card/

Initializes the MicroSD card over SDMMC 1-bit interface and mounts it as
a FAT filesystem at `/sdcard`. Provides standard file operations (read,
write, append, delete, rename, existence check, size query) and directory
operations (mkdir, list).

Public API: `sd_card_init()`, `sd_card_read_file()`,
`sd_card_write_file()`, `sd_card_list_dir()`, `sd_card_file_exists()`,
and others.

### components/standby/

Monitors user inactivity and enters ESP32 deep sleep after a configurable
timeout (default 600 seconds / 10 minutes). The timeout is persisted in
NVS so it survives reboots. A pre-sleep callback allows the editor to
auto-save before power-down. The wake source is an EXT0 trigger on
the per-board RTC-capable GPIO selected by `CONFIG_DRAFTLING_WAKEUP_GPIO`
in `main/Kconfig.projbuild` (defaults: GPIO18 on Waveshare RLCD-4.2,
GPIO0 / BOOT on every other board). The standby code itself is
board-agnostic and never tests `DRAFTLING_MODEL_*` directly. On the
M5Stack PaperS3, GPIO0 is used because earlier revisions tried
GPIO21 (the on-board buzzer pin -- floated low under some
speaker-driver states and woke the device instantly) and GPIO48
(the GT911 touch INT) with a light-sleep + `esp_restart()`
workaround; both woke the device immediately, the latter because
the e-paper backend initialises only the panel (not the touch
controller), so the GT911 is left uninitialised and holds INT low.

On targets that have no EXT0 support (the ESP32-P4 / M5Stack Tab5,
where `SOC_PM_SUPPORT_EXT0_WAKEUP` is undefined), the EXT0 arming
is compiled out entirely. The Tab5 has no user button or touch INT
on an LP_IO pin (GPIO 0-15) either, so the only wake path is the
hardware RESET button: the chip cold-boots and the editor restores
from autosave on the next run. See `docs/tab5-esp-hosted.md`.

On the LilyGO T5 E-Paper S3 Pro / Pro Lite, the standby pre-sleep
callback is replaced with `pre_sleep_t5_deinit()` (`main/main.cpp`),
which chains the standard `pre_sleep_autosave` and then walks every
peripheral that the ESP-IDF default deep-sleep path would leave
powered: GT911 (`touchscreen_sleep()`), SX1262 LoRa (`SetSleep`
opcode over SPI3 on the Pro variant), MicroSD (`sd_card_deinit()`
releases SPI3), the front-light LED on GPIO 11 (driven LOW + RTC IO
hold), unused RTC-IO pins (`rtc_gpio_isolate()`), and finally
`esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF)`.
Without this hook the board pulled ~30 mA in sleep, draining the
1500 mAh pack in two days; with the hook the figure drops to the
tens of uA. Full peripheral table in `docs/lilygo-t5-power.md`.

Before arming EXT0, `standby_enter_sleep()` enables the chip's
internal RTC pull-up on the wake GPIO and disables any pull-down.
The supported boards (RLCD-4.2 button on GPIO18 and PaperS3 BOOT on
GPIO0 / strapping pin) already have external pull-ups, so this is
harmless: the two pull-ups simply parallel. The internal pull-up is
always enabled so the behaviour is consistent across boards.

In addition to the inactivity timer, `standby_init()` also arms a
"no keyboard connected" countdown of `CONFIG_DRAFTLING_NO_KEYBOARD_SLEEP_SEC`
seconds (default 180, 0 = disabled). When the timer fires it polls
`ble_keyboard_is_connected()` and only enters deep sleep if no
Bluetooth keyboard has paired by then. This conserves battery when
the device is powered on accidentally or no paired keyboard is in
range. The countdown is only armed on boards with
`CONFIG_DRAFTLING_HAS_BATTERY` -- the USB-only Guition JC3248W535
skips it, since there is nothing to conserve and unexpectedly
blanking the display during bring-up is more disruptive than helpful.

Public API: `standby_init()`, `standby_reset_timer()`,
`standby_set_timeout()`, `standby_set_pre_sleep_cb()`,
`standby_enter_sleep()`.

### components/power/

Hardware power-latch + PWR-button driver. Compiled in only on
boards with `CONFIG_DRAFTLING_HAS_POWER_LATCH` (currently just the
Waveshare ESP32-S3-Touch-LCD-3.49); on every other board the
public functions are no-op stubs.

The Touch-LCD-3.49 keeps the battery rail alive through a TCA9554
I2C IO-expander pin (IO6 by default): the pin must be driven HIGH
at boot or the rail collapses as soon as the user releases the
boot-time PWR button press, and driving it LOW cuts the rail and
fully powers the board off. The dedicated PWR button on GPIO 16
is monitored by a 50 ms-period `esp_timer`; a hold of
`POWER_LONG_PRESS_MS` (1500 ms) or longer triggers the pre-off
callback (typically the editor auto-save) and then `power_off()`.
A short press is intentionally ignored -- short presses are how
the user *powers on* the board from a fully-off state.

`standby_enter_sleep()` calls `power_off()` before falling back
to `esp_deep_sleep_start()`, so on battery the inactivity timer
cleanly powers the board down (and the LCD goes truly dark
because its supply is downstream of the latch) while on USB the
latch has no effect and the chip just deep-sleeps.

The TCA9554 is reached over a dedicated I2C bus (SDA/SCL pins +
address + latch bit all carried by `power_config_t` so this
component stays board-agnostic). The bus is opened with the
ESP-IDF v5.x `i2c_master_*` API; we issue only two register writes
(Output and Configuration) so this component does not pull in a
heavy `esp_io_expander_tca9554` dependency.

Public API: `power_init()`, `power_set_long_press_cb()`, `power_off()`.

### components/tab5_kbd/

Driver for the M5Stack Tab5 attachable keyboard, a detachable QWERTY
keyboard that talks to the ESP32-P4 over a dedicated I2C bus
(7-bit address 0x6D, see `TAB5_KBD_I2C_ADDR`; SDA/SCL set by
`CONFIG_DRAFTLING_TAB5_KBD_I2C_SDA_GPIO` /
`CONFIG_DRAFTLING_TAB5_KBD_I2C_SCL_GPIO`, default GPIO 0 / 1) plus a
dedicated interrupt line on GPIO 50 (`CONFIG_DRAFTLING_TAB5_KBD_INT_GPIO`).
Compiled in only when `CONFIG_DRAFTLING_HAS_TAB5_KBD` is set (Tab5
default); main / editor pull it in via
`idf_component_optional_requires()`. main.cpp creates the dedicated
keyboard I2C master bus (auto-selected port via `i2c_port = -1`, since
the BSP system bus already holds one controller) and hands its handle
to `tab5_kbd_init()`.

`tab5_kbd_init(bus, int_gpio)` probes the keyboard once by reading its
version register (0xFE). If the keyboard is not attached the probe
fails, the I2C device handle is removed, and the component stays
permanently idle this boot -- no further traffic is issued on its
dedicated bus. If the probe
succeeds the keyboard is switched to HID mode (register 0x10 = 1),
the event queue / interrupt latch are cleared, RGB custom mode is
selected (register 0x11 = 1), both indicator LEDs are lit green for
one second and then turned off (a one-shot `esp_timer`), and a
negative-edge GPIO interrupt on the INT line drives a worker task.

On each interrupt the task reads the interrupt-status register
(0x01); if the HID-event bit (0x02) is set it drains the queued
events (count from register 0x02), reading the 2-byte HID report
(modifier + keycode) from register 0x30 for each, then clears the
status register to release the INT line. Because the Tab5 HID report
carries a single keycode slot (keycode 0 means "no key"), each report
is diffed against the previous one to synthesise the same
`kb_event_t` press / release stream the BLE and USB keyboards emit;
the editor key handler is shared verbatim. The Tab5 keyboard coexists
with USB and BLE rather than replacing them: on the Tab5 the built-in
keyboard and a USB keyboard both feed the editor, and when neither a
USB nor BLE keyboard is connected the user can still start a BLE scan
on demand from the F1 menu ("BLE: Start scan", which re-enables BLE
if it was left idle at boot).

Only the registers Draftling needs are implemented here; the full
vendor library (RGB binding modes, string mode, I2C-address
re-flashing, Arduino backend) from
`m5stack/M5Tab5-Keyboard-UserDemo` is intentionally not vendored.

Public API: `tab5_kbd_init()`, `tab5_kbd_is_present()`,
`tab5_kbd_set_callback()`.

### components/wifi_manager/

Manages WiFi in station mode. Reads credentials from NVS or from
`/sdcard/wifi.cfg`. Provides connect, disconnect, and state query
functions with an event callback for connection state changes (idle,
connecting, connected, disconnected, error). Required by `git_sync` for
network access.

Public API: `wifi_manager_init()`, `wifi_manager_connect()`,
`wifi_manager_disconnect()`, `wifi_manager_is_connected()`,
`wifi_manager_get_ip()`, `wifi_manager_get_ssid()`.

## Font Creation Process

The `components/fonts/` directory contains custom LVGL bitmap fonts
generated from the **Greybeard** typeface, a monospaced bitmap font that
is a vector port of UW ttyp0 (MIT-licensed, source:
https://github.com/flowchartsman/greybeard).

### Source Files

Greybeard ships as a set of TTF files, each designed for a single native
pixel size. Because the outlines trace exact pixel boundaries, rendering
at the native size with 1-bit-per-pixel (no antialiasing) produces
pixel-perfect glyphs with no scaling artifacts.

### Generation Tool

The fonts were converted to LVGL C source files using **lv_font_conv**,
the official LVGL font conversion utility. The exact command line is
recorded in the header comment of each generated `.c` file. For example,
the 14 px font was generated with:

```
lv_font_conv \
  --font Greybeard-14px.ttf \
  -r 0x20-0x7F,0xA0-0xFF,0x400-0x4FF,0x20AC,0x20B4,0x2116 \
  --size 14 \
  --bpp 1 \
  --format lvgl \
  --no-compress \
  --lv-font-name greybeard_14 \
  -o greybeard_14.c
```

### Post-generation Fix-up

`lv_font_conv` emits a boilerplate include block at the top of every
generated `.c` file:

```c
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif
```

Under ESP-IDF the LVGL component is registered as `lvgl__lvgl` and the
public header is exposed simply as `lvgl.h`; the `lvgl/lvgl.h` fallback
path does not exist and breaks the build with
`fatal error: lvgl/lvgl.h: No such file or directory`. After regenerating
any font, replace that whole `#ifdef … #endif` block with a single line:

```c
#include "lvgl.h"
```

This matches the include style used elsewhere in the component
(`components/fonts/greybeard.c`, `components/fonts/include/greybeard.h`).

### Unicode Ranges

The base `greybeard_NN.c` files cover the always-on core ranges:

| Range | Description |
|-------|-------------|
| U+0020 - U+007F | Basic Latin (ASCII printable characters) |
| U+00A0 - U+00FF | Latin-1 Supplement (accented Latin characters, symbols) |
| U+20AC | Euro sign |
| U+2116 | Numero sign |

Additional script coverage is split into separate subset font files
that are compiled into the firmware only when the corresponding
keyboard layout is enabled in Kconfig. This keeps the firmware small
for builds that do not need a given script.

| File pattern | Range | Gated on |
|--------------|-------|----------|
| `greybeard_cyrillic_NN.c` | U+0400 - U+04FF (Cyrillic) + U+20B4 (Hryvnia) | `CONFIG_KB_LAYOUT_ENABLE_UA` |
| `greybeard_hebrew_NN.c`   | U+0590 - U+05FF (Hebrew block) | `CONFIG_KB_LAYOUT_ENABLE_HE` |

The base font is generated with `--lv-fallback greybeard_NN_ext` and
the Hebrew subset with `--lv-fallback greybeard_NN_he_next`. Both
fallback symbols are tiny runtime-mutable "router" `lv_font_t` structs
defined in `components/fonts/greybeard.c`; `greybeard_init()` chains
their `.fallback` pointers at boot so that a missed glyph lookup in
the base font walks into Hebrew and/or Cyrillic as appropriate.
Hebrew layouts also require `CONFIG_LV_USE_BIDI=y` so LVGL renders
RTL strings in the correct visual order.

### Sizes and Metrics

Six sizes are provided. All except the 26 px variant are rendered at
their native TTF pixel size. The 26 px font is scaled from the 22 px
TTF source.

| File | Pixel Size | Char Width | Line Height | Notes |
|------|-----------|------------|-------------|-------|
| greybeard_11.c | 11 | 6 | 11 | Smallest, used for compact UI elements |
| greybeard_14.c | 14 | 7 | 13 | Default body text |
| greybeard_16.c | 16 | 8 | 15 | |
| greybeard_18.c | 18 | 9 | 17 | |
| greybeard_22.c | 22 | 11 | 21 | Headings |
| greybeard_26.c | 26 | 13 | 25 | Largest heading (scaled from 22 px TTF) |

All fonts are declared in `components/fonts/include/greybeard.h` as
`extern const lv_font_t greybeard_NN` and compiled as an IDF component
that depends on `lvgl__lvgl`.

### Rendering Settings

- **Bits per pixel (bpp):** 1 (pure black and white, no antialiasing).
  This matches the reflective LCD which has no grayscale capability.
- **Compression:** disabled (`--no-compress`) for faster glyph lookup on
  the microcontroller.
- **Format:** `lvgl` (native LVGL font structure).

## Hardware Definitions in Kconfig.projbuild

There are two Kconfig.projbuild files that expose project-specific
menuconfig options.

### main/Kconfig.projbuild -- Hardware Configuration

This file defines the **DRAFTLING Configuration** menu with the following
options:

#### Hardware Model (DRAFTLING_HARDWARE_MODEL)

A `choice` that selects the target board. Both options are
ESP32-S3-only (`depends on IDF_TARGET_ESP32S3`):

- **DRAFTLING_MODEL_WAVESHARE_RLCD42** -- Waveshare ESP32-S3-RLCD-4.2
  with a 400x300 reflective LCD and GPIO18 deep-sleep wakeup button.
  *Requires ESP32-S3.*
- **DRAFTLING_MODEL_M5STACK_PAPERS3** -- M5Stack PaperS3 with a
  4.7" 540x960 ED047TC1 e-paper driven by the `vroland/epdiy`
  library (with the in-tree `epd_board_papers3` board definition),
  on-board MicroSD on SPI3, BOOT button on GPIO0 used as
  the EXT0 deep-sleep wake source (the only RTC-capable user-input
  GPIO on the board). *Requires ESP32-S3.*

The hardware-model selection drives two non-prompted `int` symbols
consumed in `main/app_config.h` as `DISPLAY_WIDTH` / `DISPLAY_HEIGHT`:

- **DRAFTLING_DISPLAY_WIDTH** -- 400 (RLCD), 960 (PaperS3).
- **DRAFTLING_DISPLAY_HEIGHT** -- 300 (RLCD), 540 (PaperS3).

Both symbols are non-prompted (no menuconfig entry); to support a
new board with a different resolution, add a model `config` block
inside the `DRAFTLING_HARDWARE_MODEL` choice and extend the
per-model `default` lines on these symbols.

#### E-paper full-refresh interval (DRAFTLING_EPD_FULL_REFRESH_INTERVAL)

`int` used by e-paper backends only (gated on
`DRAFTLING_DISPLAY_EPD`). Number of partial refreshes between
full refreshes; default 30.

#### Derived feature flags (no menuconfig prompt)

The hardware-model choice also drives a set of hidden `bool` /
`int` symbols that carry the user's board pick to every component
without anyone needing to test individual `DRAFTLING_MODEL_*` ids
in C / C++ code:

| Symbol | Purpose | Set by |
|--------|---------|--------|
| DRAFTLING_DISPLAY_RLCD            | Selects `display_rlcd.cpp`        | RLCD-4.2 |
| DRAFTLING_DISPLAY_EPD             | Gates EPD-only options (BLACK_BACKGROUND, full-refresh interval) and the editor's no-blink cursor / 120 ms flush debounce | PaperS3, LilyGO T5 E-Paper S3 Pro / Pro Lite |
| DRAFTLING_DISPLAY_EPDIY           | Selects `display_epdiy.cpp` (with `epd_board_v7` for LilyGO T5 or the in-tree `epd_board_papers3` for PaperS3) and pulls in the `vroland/epdiy` managed component | PaperS3, LilyGO T5 E-Paper S3 Pro / Pro Lite |
| DRAFTLING_EPDIY_BOARD_PAPERS3     | Switches `display_epdiy.cpp` to the PaperS3 board definition (no VCOM, no shared I2C) | PaperS3 |
| DRAFTLING_DISPLAY_AXS15231B       | Selects `display_axs15231b.cpp`   | Touch-LCD-3.49, JC3248W535 |
| DRAFTLING_DISPLAY_MIPI_DSI        | Selects `display_mipi_dsi.cpp` (delegates to `espressif/m5stack_tab5` BSP) | M5Stack Tab5 |
| DRAFTLING_DISPLAY_COLOR           | Enables the color-theme picker; PARTIAL render mode in `lvgl_port.cpp` | AXS15231B boards, Tab5 |
| DRAFTLING_DISPLAY_HAS_BACKLIGHT   | Adds the "Backlight: NN%" entry to F1 -> Settings, enables the Ctrl+B cycle shortcut, and calls `display_set_backlight()` at boot from NVS | AXS15231B boards, Tab5, LilyGO T5 E-Paper S3 Pro / Pro Lite |
| DRAFTLING_HAS_BATTERY             | Creates the battery-percentage status-bar label and its poll timer | RLCD-4.2, PaperS3, Touch-LCD-3.49, T5 E-Paper S3 Pro / Pro Lite |
| DRAFTLING_BATTERY_BQ27220         | Selects the BQ27220 fuel-gauge backend (`battery_init_bq27220(shared_i2c_bus)`) instead of the GPIO ADC backend | T5 E-Paper S3 Pro / Pro Lite |
| DRAFTLING_HAS_POWER_LATCH         | Enables the `power` component: TCA9554-latched battery rail + PWR-button long-press = power off; standby cuts the latch before falling back to deep sleep | Touch-LCD-3.49 |
| DRAFTLING_SD_SDMMC                | Routes SD init through the on-chip SDMMC peripheral (1-bit) instead of generic SPI | RLCD-4.2 |
| DRAFTLING_WAKEUP_GPIO             | RTC-capable EXT0 wake-up GPIO; consumed by `components/standby/standby.cpp` | per-model defaults |

Components MUST key off these derived symbols; they MUST NOT
test `DRAFTLING_MODEL_*` directly. Adding a new model means
adding new `default` lines to each of the symbols above (plus
the width / height / rotate-default symbols), creating a new per-board
header file under `main/boards/` with the board's `BOARD_NAME`, pin
numbers and `WAKEUP_GPIO_NUM`, adding the corresponding `#elif` include
directive in `main/app_config.h`, and updating the matching display / SD
init branch in `main/main.cpp`.

#### Display Rotation (DRAFTLING_DISPLAY_ROTATE)

A `choice` that sets the display rotation angle. Options are 0, 90, 180,
and 270 degrees. The default is 0 (no rotation). The selected angle is
exposed as the hidden `int` symbol **DRAFTLING_DISPLAY_ROTATE_ANGLE**,
consumed in `app_config.h` as `DISPLAY_ROTATE`.

#### Display scale factor (DRAFTLING_DISPLAY_SCALE)

Non-prompted derived `int` (range 1-8). Logical pixel size: every
LVGL pixel is rendered as SCALE x SCALE physical panel pixels via
nearest-neighbor expansion in the display backend. The editor and
LVGL canvas operate in *logical* coordinates (panel size divided by
SCALE); only the display backend deals in physical panel pixels.
Defaults: 2 for the M5Stack PaperS3 (so the high-density 540x960
panel renders Greybeard text at a comfortably readable size), 1 for
every other board. Because the symbol is non-prompted, the per-model
default re-applies automatically whenever `DRAFTLING_HARDWARE_MODEL`
changes -- no need to delete `sdkconfig`. To override, set the value
in `sdkconfig.defaults`.

Currently the `display_epdiy` (PaperS3 + LilyGO T5 e-paper),
`display_axs15231b` (Touch-LCD-3.49 / JC3248W535) and
`display_mipi_dsi` (M5Stack Tab5) backends implement the up-scaling;
on the RLCD backend a value > 1
has no visible effect because the LVGL framebuffer already matches
the panel size.

### components/kb_layout/Kconfig.projbuild -- Keyboard Layouts

This file defines the **DRAFTLING Keyboard Layouts** menu. Each layout
is an independent `bool` option that can be enabled or disabled:

| Symbol | Layout | Default |
|--------|--------|---------|
| KB_LAYOUT_ENABLE_US | US-English (QWERTY) | y (enabled) |
| KB_LAYOUT_ENABLE_UA | Ukrainian (Cyrillic) | y (enabled) |
| KB_LAYOUT_ENABLE_DE | German (QWERTZ) | n (disabled) |
| KB_LAYOUT_ENABLE_FR | French (AZERTY) | n (disabled) |

Disabling unused layouts saves flash space because the translation
tables for disabled layouts are excluded from the build. The `kb_layout`
component reads these symbols at compile time to conditionally compile
only the enabled layout tables.

## Building

Requires ESP-IDF v5.3 or later (v5.3 - v5.5 confirmed working).

PSRAM is required on every supported board. The editor gap buffer
(sized dynamically at startup from the PSRAM that is free when
`editor_init()` runs -- typically a few hundred KB up to a few MB
depending on the board), the display
framebuffers, the LVGL widget heap (`CONFIG_LV_USE_CUSTOM_MALLOC` routes
through PSRAM), the Git-sync HTTPS response buffers + task stack, and
the Bluedroid host environment (`CONFIG_BT_BLE_DYNAMIC_ENV_MEMORY` plus
`CONFIG_BT_ALLOCATION_FROM_SPIRAM_FIRST`) all assume `MALLOC_CAP_SPIRAM`
is available. The top-level `CMakeLists.txt` aborts the configure step
with a `FATAL_ERROR` if `CONFIG_SPIRAM` is not set. Targets without
on-chip PSRAM support (e.g. ESP32-S2, bare ESP32-C3 modules without
PSRAM) are not supported.

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

If you update `sdkconfig.defaults` or pull new changes, delete the
generated `sdkconfig` so the defaults are re-applied:

```bash
rm -f sdkconfig
idf.py set-target esp32s3
idf.py build
```
