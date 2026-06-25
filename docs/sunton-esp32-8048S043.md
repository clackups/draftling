# Sunton ESP32-8048S043C -- board port notes

The Sunton ESP32-8048S043C is a 4.3" HMI dev board: an ESP32-S3 (16 MB
flash, 8 MB octal PSRAM) driving an 800x480 IPS color panel (ST7262)
over a 16-bit parallel RGB565 interface, with a GT911 capacitive touch
controller on I2C, a MicroSD slot on SPI, and a PWM backlight on
GPIO 2. It is the smaller sibling of the 7" ESP32-8048S070C and shares
the same display backend (`components/display/display_rgb.cpp`) and the
same ESP32-S3 LCD RGB peripheral path (`esp_lcd_new_rgb_panel`): a
framebuffer in PSRAM is continuously scanned out under the
HSYNC / VSYNC / DE / PCLK timing the controller generates.

The board is keyboard-driven like every other Draftling target (USB /
BLE). Touch is enabled by the `sdkconfig.defaults.sunton043` overlay as
a secondary input device (cursor / scroll / tap-to-select).

References:

- rzeldent / platformio-espressif32-sunton board reference:
  https://github.com/rzeldent/platformio-espressif32-sunton

## Relationship to the 7" ESP32-8048S070C

Both boards select the same `DRAFTLING_DISPLAY_RGB` backend and the
same 800x480 resolution and 2x logical scale (400x240 logical canvas).
They differ only in the panel wiring, which the backend selects at
build time on `CONFIG_DRAFTLING_RGB_BOARD_S043`:

| Parameter          | S070C (7")     | S043C (4.3")        |
| ------------------ | -------------- | ------------------- |
| Panel controller   | RGB TN         | ST7262 IPS          |
| PCLK               | 12 MHz         | 12.5 MHz            |
| HSYNC GPIO         | 39             | 39                  |
| VSYNC GPIO         | 40             | 41                  |
| DE GPIO            | 41             | 40                  |
| PCLK GPIO          | 42             | 42                  |
| HSYNC pw/bp/fp     | 2 / 43 / 8     | 4 / 8 / 8           |
| VSYNC pw/bp/fp     | 2 / 12 / 8     | 4 / 8 / 8           |
| `pclk_active_neg`  | 0              | 1                   |
| `pclk_idle_high`   | 1              | 0                   |
| Data-line order    | B,G,R          | R,G,B               |
| Backlight          | 2 (LEDC PWM)   | 2 (LEDC PWM)        |

Note that VSYNC and DE are swapped between the two boards, and the
data-line order is reversed. On the 7" panel the physical lines are
wired R,G,B but listed B,G,R in `data_gpio_nums[]` (esp_lcd maps
`[0]` to the RGB565 LSB), to avoid an R/B swap. The 4.3" panel needs
the opposite (R,G,B) array order to render a standard RGB565 pixel
with red and blue in the right place.

### Panel pins and timings (S043C)

All panel data / control / backlight GPIOs are owned by the RGB
backend and baked into its `CONFIG_DRAFTLING_RGB_BOARD_S043` block;
they are **not** redefined in `main/app_config.h`.

| Signal     | GPIO |
| ---------- | ---- |
| HSYNC      | 39   |
| VSYNC      | 41   |
| DE         | 40   |
| PCLK       | 42   |
| DISP       | -1 (not wired) |
| Backlight  | 2 (LEDC PWM) |

| RGB565 bits | Color group | GPIOs |
| ----------- | ----------- | ----- |
| 0..4        | R0-R4       | 8, 3, 46, 9, 1 |
| 5..10       | G0-G5       | 5, 6, 7, 15, 16, 4 |
| 11..15      | B0-B4       | 45, 48, 47, 21, 14 |

## Pins Draftling touches directly

These are defined in the `CONFIG_DRAFTLING_MODEL_SUNTON_8048S043`
block of `main/app_config.h`. They are identical to the 7" board's
standard Sunton ESP32-8048S0xx pinout.

| Function        | GPIO |
| --------------- | ---- |
| SD CS           | 10 |
| SD MOSI         | 11 |
| SD MISO         | 13 |
| SD SCK          | 12 |
| Touch I2C SDA   | 19 |
| Touch I2C SCL   | 20 |
| Touch RST       | 38 |
| Touch INT       | -1 (not wired) |
| Deep-sleep wake | 0 (BOOT button, EXT0) |

The SD card is mounted via `sd_card_init_spi()` on `SPI3_HOST` (the
RGB peripheral does not use a SPI bus, so there is no contention).

## Touchscreen

The on-board GT911 is supported (`CONFIG_DRAFTLING_TOUCH_GT911`,
derived from the model choice) and is enabled by the
`sdkconfig.defaults.sunton043` overlay. The GT911 INT line is not
wired on the Sunton reference, so the touchscreen component polls the
controller's status register on every LVGL tick. RST is on GPIO 38;
the driver pulses it at init and probes both possible GT911 addresses
(0x5D primary, 0x14 backup). The panel is natively landscape 800x480,
so no axis swap or mirror is needed
(`TOUCH_SWAP_XY = TOUCH_MIRROR_X = TOUCH_MIRROR_Y = 0`).

## Keyboard layouts

This author of this port works with a German BLE keyboard, so the
`sdkconfig.defaults.sunton043` overlay enables only the US (QWERTY)
and DE (QWERTZ) layouts (cycle with `Ctrl+L` or `Win+Space`) and
disables the Ukrainian, French and Hebrew layouts that the common
Kconfig would otherwise pull in.

The author's keyboard has no dedicated ESC key, so `Ctrl+X` is wired as an
Escape substitute. `process_key_event()` in
`components/editor/editor_ui.cpp` translates `Ctrl+X` (HID usage 0x1B
with either Control modifier) into a synthetic `KB_KEY_ESCAPE` event,
so every screen handler that already keys off Escape (leaving the
editor, closing the file browser / settings / menus / dialogs) reacts
identically. This substitution is global, so it works on every board.

## Display rotation

The USB-C connector and MicroSD slot are on the opposite edge of the
board compared to the 7" ESP32-8048S070C, so depending on how the
board is mounted the image may need to be flipped end-for-end. The F1
-> Settings menu has a runtime **Rotate 180** toggle (on / off) for
this. It is offered on any board with `DRAFTLING_DISPLAY_CAN_ROTATE`
(derived: on for the parallel-RGB boards).

The toggle is applied on top of the build-time base rotation
(`DRAFTLING_DISPLAY_ROTATE`, default 0 here) by the LVGL port
(`draftling_lvgl_port_set_flip180()` in
`components/display/lvgl_port.cpp`). A 180-degree flip does not change
the reported panel resolution, so it takes effect immediately without
rebuilding the LVGL widget tree -- the port just updates
`lv_display_set_rotation`, recomputes the effective rotation used by
the software-rotate path in `flush_cb`, and repaints the whole panel.
The setting is persisted in NVS (`editor` namespace, key `rot180`) and
re-applied at boot in `editor_ui_init()`.

## Flash mode

The board's flash is wired for **DIO** (verified on this hardware in
the parallel breezydemo port). The common ESP32-S3 default is QIO,
so `sdkconfig.defaults.sunton043` overrides
`CONFIG_ESPTOOLPY_FLASHMODE_DIO=y` to ensure the bootloader comes up
reliably. PSRAM is octal at 80 MHz (the common ESP32-S3 default,
which matches this board).

## Building

The model and its board-specific overrides live in
`sdkconfig.defaults.sunton043`. Select the board by passing all three
defaults files:

```
SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.esp32s3;sdkconfig.defaults.sunton043" \
    idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

`sdkconfig.defaults.sunton043` selects
`CONFIG_DRAFTLING_MODEL_SUNTON_8048S043`, forces DIO flash mode,
enables the GT911 touchscreen, and sets the US + DE keyboard layouts.

## Cross-references

- `components/display/display_rgb.cpp` -- RGB panel backend; per-board pin / timing / data-order block gated on `CONFIG_DRAFTLING_RGB_BOARD_S043`.
- `main/app_config.h` -- `CONFIG_DRAFTLING_MODEL_SUNTON_8048S043` block (SD / touch / wakeup pins).
- `main/Kconfig.projbuild` -- model choice, `DRAFTLING_DISPLAY_RGB` + `DRAFTLING_RGB_BOARD_S043` derived flags, width/height (800x480), `DISPLAY_SCALE` (2), GT911 / touch-RST defaults.
- `main/main.cpp` -- `display_init()` branch under `CONFIG_DRAFTLING_DISPLAY_RGB` (shared with the 7" board).
- `components/editor/editor_ui.cpp` -- `process_key_event()` Ctrl+X -> Escape substitution; F1 -> Settings "Rotate 180" item (NVS key `rot180`, boot-apply in `editor_ui_init()`).
- `components/display/lvgl_port.cpp` -- `draftling_lvgl_port_set_flip180()` / `draftling_lvgl_port_get_flip180()` runtime 180-degree flip.
- `sdkconfig.defaults.sunton043` -- board selection + DIO flash + touch enable + US/DE keyboard layouts.
- `docs/sunton-esp32-8048S070c.md` -- the 7" sibling board.
