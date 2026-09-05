# Waveshare ESP32-S3-Touch-LCD-7 -- board port notes

_Paths below (`main/...`, `components/...`, `sdkconfig.defaults*`,
`CMakePresets.json`, `build/...`) are relative to `firmware/`._

The Waveshare ESP32-S3-Touch-LCD-7 is a 7" HMI dev board: an
ESP32-S3N16R8 module (16 MB flash, 8 MB octal PSRAM) driving an
800x480 ST7262 IPS panel over a 16-bit parallel RGB565 interface, with
a GT911 capacitive touch controller and a MicroSD slot, both wired
behind an on-board CH422G I2C IO-expander instead of direct GPIOs.
It shares the same `components/display/display_rgb.cpp` backend as
the Sunton ESP32-8048S0xx family (see
[docs/sunton-esp32-8048S070c.md](sunton-esp32-8048S070c.md)), extended
with a CH422G-aware backlight / reset path.

References:

- Waveshare wiki: https://docs.waveshare.com/ESP32-S3-Touch-LCD-7
- Upstream pin map: `esp-arduino-libs/ESP32_Display_Panel`,
  `src/board/supported/waveshare/BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_7.h`
  (Waveshare's own reference for this board; all pin numbers and
  CH422G EXIO assignments below are taken from it and cross-checked
  against the wiki).

The board is keyboard-driven like every other Draftling target (USB /
BLE), with touch as a secondary input, enabled by default -- see
"Touchscreen" below.

## The CH422G IO-expander

Unlike the Sunton boards, most control lines on this board are not
wired to direct ESP32-S3 GPIOs but sit behind an on-board CH422G I2C
IO-expander (`components/io_expander/io_expander_ch422g.cpp`, gated
on `CONFIG_DRAFTLING_HAS_CH422G`) on the same I2C bus as the GT911
touch controller (SDA=GPIO8, SCL=GPIO9):

| EXIO | Function | Driven by |
| ---- | -------- | --------- |
| EXIO1 | Touch reset | main.cpp (CH422G boot-time block) |
| EXIO2 | Backlight enable (on/off only, no PWM) | `display_rgb.cpp` |
| EXIO3 | LCD reset | `display_rgb.cpp` |
| EXIO4 | SD card chip-select (active low) | main.cpp (asserted once at boot) |

EXIO5 (USB/CAN transceiver select) and EXIO0 are not used by
Draftling -- the board has no CAN or RS485 support in this firmware.

The CH422G has no "register address" the way a TCA9554 or PCA9535
does: each internal register is a distinct 7-bit I2C slave address
that accepts exactly one data byte (0x24 = system/mode register,
0x38 = EXIO0-7 output level). `io_expander_ch422g.cpp` implements
just those two registers -- switching all 8 EXIO pins to push-pull
outputs at init, then shadowing the output byte in RAM since the chip
has no read-modify-write.

`main.cpp` calls `ch422g_init()` on the shared I2C bus before
`display_init()` runs (the RGB backend pulses the LCD reset line and
drives the backlight through it as part of its own init), then -- if
`CONFIG_DRAFTLING_TOUCHSCREEN` is enabled -- pulses the GT911
touch-reset line (EXIO1) with the INT pin (GPIO4) held low as a
push-pull output, so the controller latches its primary I2C address
0x5D. This is the same address-select technique used on the M5Stack
Tab5, minus the TOUCH_EN power-cycle: this board's GT911 has its own
dedicated reset line, so a plain reset pulse is enough.

## SD card chip-select via I2C expander

The MicroSD slot's MOSI/MISO/SCK are direct GPIOs (11/13/12) on a
dedicated SPI bus, but chip-select is CH422G EXIO4 -- a pin that
cannot be toggled per-transaction the way a hardware CS GPIO can.
Since the SD card is the only device on this SPI bus, `main.cpp`
asserts EXIO4 low once at boot (before `sd_card_init_spi()` runs) and
never touches it again, passing `SD_SPI_CS_PIN = -1`
(`GPIO_NUM_NC` / `SDSPI_SLOT_NO_CS`) to `sd_card_init_spi()`.
ESP-IDF's `sdspi_host` driver always manages CS itself in software
(holding it low across a whole multi-transaction SD command, never
handing CS to the SPI peripheral's own `spics_io_num`), and simply
skips its own GPIO calls when `gpio_cs == GPIO_NUM_NC` -- so a
permanently-low CS driven by the expander is indistinguishable from a
GPIO CS that the driver would have held low for the same duration.

## Display backend

The RGB panel is driven by the same `components/display/display_rgb.cpp`
backend as the Sunton ESP32-8048S0xx family (framebuffer in PSRAM,
dirty-bbox `display_push_rgb565()` / `display_flush()` path -- see
[docs/sunton-esp32-8048S070c.md](sunton-esp32-8048S070c.md) for the
shared architecture). This board selects a third pin/timing branch
via `CONFIG_DRAFTLING_HAS_CH422G`.

### Panel pins and timings

| Signal     | GPIO |
| ---------- | ---- |
| HSYNC      | 46   |
| VSYNC      | 3    |
| DE         | 5    |
| PCLK       | 7    |
| DISP       | -1 (not wired) |

| Timing parameter   | Value |
| ------------------ | ----- |
| PCLK               | 16 MHz |
| HSYNC pulse width  | 4  |
| HSYNC back porch   | 8  |
| HSYNC front porch  | 8  |
| VSYNC pulse width  | 4  |
| VSYNC back porch   | 8  |
| VSYNC front porch  | 8  |
| `pclk_active_neg`  | 1  |
| `pclk_idle_high`   | 0  |

Backlight and LCD reset are CH422G EXIO2 / EXIO3 (see above), not
GPIOs. The backlight EXIO is a plain on/off switch with no PWM
capability, so `CONFIG_DRAFTLING_DISPLAY_BACKLIGHT_BINARY` is set for
this board: the editor omits the "Backlight: NN%" Settings entry and
the Ctrl+B shortcut entirely (a brightness control that can only
toggle fully on or off would be misleading), and the backlight simply
stays on (`display_set_backlight()` is still callable and still
treats any non-zero percent as "on", but nothing in the UI calls it
on this board). Standby's display-off / deep-sleep paths still cut
and restore the backlight through `display_sleep()` / `display_wake()`
/ `display_deep_sleep_prepare()` in `display_rgb.cpp`, independently
of the (unused, on this board) Settings-menu state.

### Color line order

The upstream board file lists the 16 RGB565 data lines already in
B,G,R order (`data_gpio_nums[0]` = RGB565 LSB), so unlike the Sunton
7" board no reordering from the physical R,G,B wiring is needed:

| RGB565 bits | Color group | GPIOs |
| ----------- | ----------- | ----- |
| 0..4        | B0-B4       | 14, 38, 18, 17, 10 |
| 5..10       | G0-G5       | 39, 0, 45, 48, 47, 21 |
| 11..15      | R0-R4       | 1, 2, 42, 41, 40 |

## Pins Draftling touches directly

These are defined in `main/boards/waveshare_touch_lcd_7.h`.

| Function        | GPIO / EXIO |
| --------------- | ----------- |
| I2C SDA (shared: CH422G + GT911) | GPIO8 |
| I2C SCL (shared: CH422G + GT911) | GPIO9 |
| SD MOSI          | GPIO11 |
| SD MISO          | GPIO13 |
| SD SCK           | GPIO12 |
| SD CS            | CH422G EXIO4 |
| Touch I2C addr   | 0x5D (GT911 primary) |
| Touch INT        | GPIO4 |
| Touch RST        | CH422G EXIO1 |
| Deep-sleep wake  | GPIO0 (BOOT button, EXT0) |

No on-board battery monitor is wired: this is a USB-powered HMI dev
board with a PH2.0 battery connector and PWR/CHG/DONE status LEDs but
no software-readable fuel gauge or ADC divider, so
`CONFIG_DRAFTLING_HAS_BATTERY` is unset and the editor status bar
hides the battery indicator.

## Touchscreen

The on-board GT911 is supported (`CONFIG_DRAFTLING_TOUCH_GT911`,
derived from the model choice) and `CONFIG_DRAFTLING_TOUCHSCREEN`
defaults to **on** for this board -- unlike the Sunton RGB boards
(where touch defaults off because their GT911 INT is unpopulated, so
bring-up has to fall back to a dual-address I2C probe), this board
wires both INT (GPIO4) and RST (CH422G EXIO1), giving a deterministic
address-select reset (see "The CH422G IO-expander" above), so touch is
reliable enough to enable out of the box. It can still be turned off
in `menuconfig` if not wanted. The panel is natively landscape
800x480, so no axis swap or mirror is needed
(`TOUCH_SWAP_XY = TOUCH_MIRROR_X = TOUCH_MIRROR_Y = 0`).

If touch still doesn't respond after flashing, make sure your build
actually picked up the current default: a `build/waveshare_touch_lcd_7/
sdkconfig` generated before this default changed will keep whatever
value it already resolved (Kconfig's `sdkconfig`-policy defaults don't
retroactively flip settings already written to an existing sdkconfig).
Delete it and reconfigure, or just toggle **Enable touchscreen input**
on directly:

```
rm -f build/waveshare_touch_lcd_7/sdkconfig
idf.py --preset waveshare_touch_lcd_7 build
```

## Building

```
idf.py --preset waveshare_touch_lcd_7 build
idf.py --preset waveshare_touch_lcd_7 -p /dev/ttyACM0 flash monitor
```

`sdkconfig.defaults.waveshare_touch_lcd_7` selects
`CONFIG_DRAFTLING_MODEL_WAVESHARE_TOUCH_LCD_7`. Touch is on by
default; disable it with `idf.py --preset waveshare_touch_lcd_7
menuconfig` under **DRAFTLING Configuration** if not wanted.

## Cross-references

- `components/display/display_rgb.cpp` -- RGB panel backend (init, push, flush, CH422G backlight / reset).
- `components/io_expander/io_expander_ch422g.cpp` -- CH422G driver (`ch422g_init()`, `ch422g_set_pin()`).
- `main/boards/waveshare_touch_lcd_7.h` -- pin map, CH422G EXIO assignments consumed by main.cpp.
- `main/Kconfig.projbuild` -- model choice, `DRAFTLING_DISPLAY_RGB` / `DRAFTLING_HAS_CH422G` derived flags, width/height (800x480), `DRAFTLING_DISPLAY_HIDPI` (Hack fonts), GT911 / touch-INT defaults.
- `main/main.cpp` -- shared I2C bus + `ch422g_init()` + touch-reset dance + SD CS assert.
- `components/touchscreen/touchscreen.cpp` -- GT911 driver (address probe skipped here since RST is deterministic).
- `sdkconfig.defaults.waveshare_touch_lcd_7` -- board selection for the `waveshare_touch_lcd_7` preset.
