# Draftling: building the firmware

### Configuring enabled keyboard layouts

The set of compiled-in keyboard layouts follows the language pack (see
"Language packs" below) and can be fine-tuned via `idf.py menuconfig`
under **DRAFTLING Language Pack and Keyboard Layouts**, where each
layout can be enabled or disabled on its own. Disabling unused layouts
saves flash space.

## Building

Requires [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/stable/)
v6.0.2 or later.

All firmware sources and build files live under [`firmware/`](firmware),
which ships a [`CMakePresets.json`](firmware/CMakePresets.json) with one
configuration preset per supported board. Each preset sets the correct
IDF target and hardware model via its own `sdkconfig.defaults.<board>`
file, and places its build output under `build/<board>` with the
generated `sdkconfig` inside that same directory, so multiple boards
can be configured side by side without clobbering each other's build
state. Run every `idf.py` command from inside `firmware/`:

```bash
cd firmware
idf.py --preset waveshare_rlcd42 build
idf.py --preset waveshare_rlcd42 -p /dev/ttyACM0 flash
```

Available preset names: `waveshare_rlcd42`, `m5stack_papers3`,
`lilygo_t5_epd_s3_pro`, `lilygo_t5_epd_s3_pro_h752`,
`waveshare_touch_lcd_349`, `m5stack_tab5`, `jc3248w535`,
`sunton_8048s070`, `sunton_8048s043`, `elecrow_crowpanel_50`,
`viewe_uedx80480043`, `waveshare_touch_lcd_7`,
`freenove_fnk0104a`, `freenove_fnk0104b`, `freenove_fnk0104s`,
`microjournal_rev5`,
`xteink_x4_pro`, `xteink_x4_classic`, `elecrow_crowpanel_579`,
`waveshare_epaper_397`, `seeed_reterminal_e1001`,
`seeed_reterminal_sticky`, `m5stack_papermono`, `nicemcu_c5_28ips`.

To avoid repeating `--preset` on every command, set the `IDF_PRESET`
environment variable instead:

```bash
export IDF_PRESET=waveshare_rlcd42
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

If you pull new changes that touch `sdkconfig.defaults` or any of the
per-board `sdkconfig.defaults.<board>` files, delete the preset's
generated `sdkconfig` so the defaults are re-applied:

```bash
rm -f firmware/build/waveshare_rlcd42/sdkconfig
cd firmware
idf.py --preset waveshare_rlcd42 build
```

## Language packs

A language pack is the set of keyboard layouts, input methods and
fonts built into one firmware image. The board preset picks the
hardware; the language pack is chosen separately, with the
`--lang-pack` option or the `DRAFTLING_LANG_PACK` environment
variable:

| Pack | Layouts | Image size (RLCD-4.2) |
|------|---------|-----------------------|
| `default` | US-English, Ukrainian, German, French, Hebrew | 2.3 MB |
| `vietnamese` | US-English, Vietnamese (Telex) | 2.3 MB |
| `korean` | US-English, Korean (2-set Hangul) | 3.1 MB |
| `japanese` | US-English, Japanese (romaji -> kana -> kanji) | 4.2 MB |
| `chinese` | US-English, Simplified Chinese (pinyin) | 3.9 MB |

Every pack fits every board, including the 8 MB-flash ones.

```bash
cd firmware
idf.py --preset waveshare_rlcd42 --lang-pack japanese build
idf.py --preset waveshare_rlcd42 --lang-pack japanese -p /dev/ttyACM0 flash

# the same, with environment variables
export IDF_PRESET=waveshare_rlcd42 DRAFTLING_LANG_PACK=japanese
idf.py build
```

`--lang-pack` must be given (or the variable set) for every command
on a non-default pack, including `flash` and `monitor`, because it
selects the build directory: the `default` pack builds in
`build/<board>` as before, any other pack in `build/<pack>/<board>`
(e.g. `build/japanese/waveshare_rlcd42`), with its own `sdkconfig`.
So images for different regions can be built side by side and never
share a build tree. The option is added to `idf.py` by
[`firmware/idf_ext.py`](firmware/idf_ext.py); it also appends
`sdkconfig.defaults.lang_<pack>` to the preset's defaults. If an
`sdkconfig` made for one pack is used to build another (for example
with a hand-picked `-B` directory), the build stops with a "Language
pack mismatch" error; delete that `sdkconfig` and build again.

The Korean, Japanese and Chinese packs draw their characters with
pixel fonts (Fusion Pixel Font 10 / 12 px and GNU Unifont 16 px, all
SIL OFL 1.1) in that language's glyph style, scaled by whole numbers to
the editor's font sizes. The generated fonts and dictionaries are
committed; the scripts that make them and their sources are listed in
[`AGENTS.md`](AGENTS.md) ("Language packs").

## Menuconfig Options

Run `idf.py menuconfig` (supplying the preset name either on the
command line or iin a variable) to open the interactive configuration
UI. Draftling adds three custom menus described below (**DRAFTLING
Configuration**, **DRAFTLING Language Pack and Keyboard Layouts** and
**DRAFTLING Editor**). All other options (Bluetooth, LVGL fonts, etc.) use the
ESP-IDF defaults from `sdkconfig.defaults` and normally do not need to
be changed.

### DRAFTLING Configuration

Found at the top-level **DRAFTLING Configuration** menu.

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| **Hardware Model** | choice | M5Stack PaperS3 | Select the target board. Display resolution, driver, pin map, touch controller and the deep-sleep wake source are derived automatically. See the [Supported hardware](#supported-hardware) section for all thirteen options. |
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


## Project Structure

All paths below are relative to `firmware/`:

```
main/               Application entry point, pin definitions, Kconfig
  boards/           Per-board pin definitions (one .h file per board)
components/
  battery/           Battery monitor: ADC + smoothing, or BQ27220
                     fuel gauge over I2C (T5 E-Paper S3 Pro / Lite)
  ble_keyboard/      BLE HID keyboard host (Bluedroid)
  display/           Display backends (RLCD SPI, epdiy e-paper for
                     PaperS3 + LilyGO T5, AXS15231B QSPI,
                     MIPI-DSI for Tab5, RGB565 parallel for Sunton and
                     Waveshare Touch-LCD-7,
                     ILI9341/ST7796 SPI for Freenove FNK0104,
                     ST7789 over PARLIO for NiceMCU C5) and
                     LVGL v9 port
  editor/            Gap-buffer editor, Markdown parser, LVGL UI, menu
  fonts/             Custom LVGL fonts (Latin, Latin-1 Supplement, Cyrillic)
  git_sync/          Native Git client (smart HTTP) with local
                     commit history on the SD card
  io_expander/       CH422G I2C IO-expander driver (Waveshare Touch-LCD-7)
  kb_layout/         Keyboard layout translation (US/UA/DE/FR)
  sd_card/           SD card (SDMMC or SPI) file operations
  standby/           Deep-sleep / standby timer manager
  touchscreen/       I2C touch driver (AXS5106L, GT911, FT6336U) + LVGL pointer indev
  wifi_manager/      WiFi STA connection manager
```
