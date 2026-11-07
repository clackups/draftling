# Wi-Fi + Bluetooth on the M5Stack Tab5 (ESP-Hosted 3.1)

_Paths below (`main/...`, `components/...`, `sdkconfig.defaults*`) are
relative to `firmware/`._

The ESP32-P4 has no on-chip 2.4 GHz radio. On the M5Stack Tab5 the
P4 is paired with an on-board ESP32-C6 module that provides Wi-Fi
and BLE; the two chips are wired together over SDIO. Draftling
reaches the C6 through
[ESP-Hosted](https://github.com/espressif/esp-hosted-mcu) 3.1
(`espressif/esp_hosted ~3.1.0`); the C6 must run esp-hosted 3.1 too
(see "C6 firmware" below):

- `wifi_manager` (Git sync) calls stock `esp_wifi_*` APIs, which
  ESP-IDF's esp_wifi remote layer redirects into hosted RPCs.
- `ble_keyboard` (BLE HID host) brings up Bluedroid the usual way,
  but first calls ESP-Hosted's `esp_hosted_bt_host_stack_setup()`,
  which starts the BLE controller on the C6 and binds Bluedroid to
  the hosted HCI, instead of the native `esp_bt_controller` (which
  does not exist on the P4). The switch is gated on
  `CONFIG_BT_CONTROLLER_DISABLED`, which the Tab5 sdkconfig sets.

The P4-side wiring is automatic: it is enabled in
`sdkconfig.defaults.esp32p4`, and `main/main.cpp` calls
`esp_hosted_init()` + `esp_hosted_connect_to_slave()` before
`ble_keyboard_init()` / `wifi_manager_connect()`
(`CONFIG_ESP_HOSTED_AUTO_CALL_INIT_BEFORE_APP_MAIN` is off: the C6 has
no power until `main.cpp` enables its rail, see below). It then asks
the C6 for its firmware version and stops with an on-screen message if
it is older than 3.1. The host
component is pulled in by `main/idf_component.yml` under a
`target == esp32p4` rule, so ESP32-S3 builds are unaffected.

## ESP32-P4 chip revision (menuconfig)

The default `sdkconfig.defaults.esp32p4` targets recent ESP32-P4
silicon (chip revision v3.0 and newer). Early Tab5 units shipped
with **older P4 dies** -- the reference unit this project was
validated on is an ESP32-P4 **revision v1.3**, in a Tab5
manufactured in 2024. Such boards will not boot a firmware built
for the default minimum revision and `idf.py flash` refuses to
write the image to the chip.

### Typical symptom

`idf.py -p /dev/ttyACM0 flash` aborts after the chip-detect step
with an error similar to:

```
A fatal error occurred: This chip is ESP32-P4 v1.3 (revision v1.3),
which does not meet the minimum chip revision specified in the
application (v3.0). Detected chip revision is incompatible with
the application binary.
Use --force flag to flash anyway.
```

(`esptool` reports the actual detected revision -- v1.3 in the
example above -- against the minimum the linked image was built
for.) The application binary itself encodes the minimum required
chip revision in its image header, so simply re-running the
command does not help; the firmware has to be rebuilt with a lower
minimum revision before it will flash and boot.

### Fix: lower the minimum revision in menuconfig

For older P4 silicon, rebuild with the minimum chip revision
relaxed to match your hardware:

```bash
idf.py --preset m5stack_tab5 menuconfig
```

then navigate to

```
Component config  --->
  Hardware Settings  --->
    Chip revision  --->
      (X) Minimum Supported ESP32-P4 Revision
          -> v0.0  (or whichever revision matches your die)
      Maximum Supported ESP32-P4 Revision  --->  v3.99
```

Save (`S`), exit (`Q`), then `idf.py --preset m5stack_tab5 build flash
monitor`. With the minimum lowered to `v0.0` the same image flashes
and boots on
both the early v1.x dies (such as the v1.3 reference unit) and
current v3.x silicon. The relevant Kconfig symbols are
`CONFIG_ESP32P4_REV_MIN_*` and `CONFIG_ESP32P4_REV_MAX_*`; you can
also set them directly in a local `sdkconfig` override if you
prefer not to use the curses UI.

> Bypassing the check with `esptool ... --force` is **not** a
> substitute: the bootloader re-validates the image header against
> the chip revision at every boot and will refuse to launch an
> application whose minimum revision is higher than the silicon
> it is running on.

## Tab5 P4<->C6 pin map

| Function | P4 GPIO |
|----------|--------:|
| SDIO CLK | 12 |
| SDIO CMD | 13 |
| SDIO D0  | 11 |
| SDIO D1  | 10 |
| SDIO D2  |  9 |
| SDIO D3  |  8 |
| C6 RESET (active LOW) | 15 |

These match the M5Tab5-UserDemo reference firmware and the Tab5
schematic. ESP-Hosted 3.1 has a Tab5 profile for them,
`CONFIG_ESP32P4_TAB5_C6_BOARD` (set in `sdkconfig.defaults.esp32p4`):
it locks the `CONFIG_ESP_HOSTED_HOST_SDIO_PIN_*` values and the reset
line (`CONFIG_ESP_HOSTED_HOST_RESET_GPIO=15`) to this wiring. Slot 1
is the SDMMC slot wired to the C6.

## Powering the on-board C6

On the Tab5 the C6's 3V3 rail is **not** connected to the always-on
supply. It is gated by the **WLAN_PWR_EN** line on the second
PI4IOE5V6408 I/O expander (I2C address `0x44`, pin `P0`) sitting on
the system I2C bus (SDA `GPIO31`, SCL `GPIO32`). Until WLAN_PWR_EN
is driven HIGH the C6 has no VDD; the host's slave-reset pulse on
`GPIO15` then toggles a dead chip and `esp_hosted_init()` times out
waiting for `ESP_HOSTED_EVENT_TRANSPORT_UP`.

`main/main.cpp` therefore enables the rail before initialising
ESP-Hosted by calling the BSP helper

```c
bsp_feature_enable(BSP_FEATURE_WIFI, true);
```

(from `espressif/m5stack_tab5`, declared in `bsp/m5stack_tab5.h`).
The BSP transparently brings up the I/O expander on first use, so
this call is the only thing needed; the underlying
`bsp_i2c_init()` is already invoked earlier from the display
bring-up. A `vTaskDelay(200 ms)` after the enable lets the C6
LDO settle and the ROM bootloader reach a known state before the
host pulses `Slave_Reset`.

## C6 firmware: esp-hosted 3.1

The C6 needs esp-hosted **3.1** firmware with Wi-Fi and the BLE
controller. Tab5 units come with M5Stack's
`ESP32C6-WiFi-SDIO-Interface-V1.4.1` image (esp-hosted-mcu 1.4.1), and
earlier versions of this document had users flash a 2.x build; neither
works with this Draftling.

### Upgrade from the P4 (recommended)

`misc/p4_upgrade_cp/` (repository root, preset `tab5`) is a small
firmware for the P4 that installs esp-hosted 3.1 on the C6 over the SDIO link, whatever
esp-hosted version the C6 runs (0.0.x, 1.x, 2.x, 3.x, or an
ESP-Hosted-FG build), with progress on the Tab5 screen. No jumper
wires or USB-serial adapter are needed:

```bash
. $IDF_PATH/export.sh
misc/p4_upgrade_cp/build.sh m5stack_tab5
cd misc/p4_upgrade_cp/p4
idf.py --preset m5stack_tab5 -p /dev/ttyACM0 flash monitor
```

When the screen says "Done", flash Draftling again. The C6 firmware
stays across P4 reflashes. See `misc/p4_upgrade_cp/README.md` for
how the tool detects the C6's protocol.

### Flash the C6 over its serial port (fallback)

If the C6 does not answer the upgrade tool at all, flash the image
built in `misc/p4_upgrade_cp/cp/` (preset `tab5`) directly.

The C6 has no boot button on the Tab5. To enter the ROM
download mode you must hold its **GPIO 9** strapping pin LOW
while it is reset (this is the standard ESP32-C6
`BOOT_MODE` strap: GPIO 9 LOW at reset -> serial download mode,
HIGH -> run flash). On the Tab5 the C6's GPIO 9 is exposed on
the secondary header next to the C6 module. Procedure:

1. Power the Tab5 with the P4 firmware halted (or simply
   keep the device unplugged until step 3).
2. Short **C6 G9 to GND** with a jumper wire.
3. Power the Tab5 on (or press the P4 RESET button). The C6
   boots and immediately enters its UART/USB-Serial-JTAG
   download stub instead of running the previously-flashed
   image.
4. Connect a USB-serial adapter (3V3) to the C6's TX / RX pads,
   or use the C6's USB-Serial-JTAG port if your board revision
   wires it out. Verify `esptool.py --chip esp32c6 chip_id`
   returns `Chip is ESP32-C6` before continuing.

If the C6 never enters download mode, double-check that
GPIO 9 is actually being held LOW for the entire duration of
the reset pulse -- a momentary touch is not enough; the strap
is sampled at the instant CHIP_PU goes high.

Then flash (`/dev/ttyUSB0` is the USB-serial adapter; the name may
differ):

```bash
cd misc/p4_upgrade_cp/cp
idf.py --preset m5stack_tab5 -p /dev/ttyUSB0 flash
```

Once flashing is done, **remove the G9-to-GND short** and power the
Tab5 off and on.

## Expected boot log

On a successful link bring-up Draftling prints these lines (ESP-Hosted
adds its own transport lines in between: reset on GPIO15, SDIO card
init, the C6's INIT event and the firmware versions of both sides):

```
I (xxxx) Draftling: Initializing ESP-Hosted link to ESP32-C6...
I (xxxx) Draftling: ESP-Hosted link up
I (xxxx) Draftling: ESP32-C6 runs esp-hosted 3.1.0
I (xxxx) BLEKeyboard: Hosted BLE controller up, Bluedroid bound to hosted HCI
I (xxxx) BLEKeyboard: Bluedroid enabled
```

## Common bring-up failures

- **"ESP32-C6 firmware too old" on screen** -- the C6 runs esp-hosted
  older than 3.1. Run the upgrade tool (see "C6 firmware" above).
- **"ESP32-C6 not responding" on screen** -- the SDIO link did not come
  up within 5 s: the C6 is not running esp-hosted firmware, or is
  wedged. Power the Tab5 off and on; if it persists, run the upgrade
  tool, and as a last resort flash the C6 over its serial port.
- **`SDIO read/write error` or `CMD53 timeout`** -- usually a
  signal-integrity issue with the SDIO bus; lower
  `CONFIG_ESP_HOSTED_HOST_SDIO_CLK_KHZ` (e.g. to `20000`) and re-test.
- **Wi-Fi associates but BLE never connects** -- the C6 image has no
  BLE controller (the init event's capabilities lack "HCI over SDIO").
  Install the image from `misc/p4_upgrade_cp/cp/` (preset `tab5`),
  which has both.

## Sleep and power-off

On battery, going to sleep switches the Tab5 off completely; press
the side power button to turn it on again. On USB power it goes into
ESP32-P4 deep sleep instead, and the **RESET** button wakes it. The
GT911 touch INT (GPIO 23) and the on-board USER button are not wired
to LP_IO pins (GPIO 0-15) on the P4, so there is no GPIO wake
source. Either way the chip cold-boots and the editor restores from
autosave.

`pre_sleep_tab5_deinit()` in `main/main.cpp`, with the BSP's I2C bus
still up:

| Part | Action |
|------|--------|
| Bluetooth / ESP-Hosted | `ble_keyboard_shutdown()` (BLE stays off even when a USB keyboard disappears as its 5 V goes), then `esp_hosted_deinit()` closes the C6's data path and stops the SDIO tasks, so nothing talks to the C6 once its supply is cut |
| Touch controller (0x43 P5) | held in reset (`BSP_FEATURE_TOUCH`); the BSP's touch drivers have no sleep command |
| MIPI-DSI panel | display off + SLPIN (DCS 0x10 through the panel IO; the BSP's panel drivers do not implement `disp_sleep()`) |
| Backlight (GPIO 22) | LEDC stopped with the output LOW, pad held; `display_init()` releases the hold (single-pad hold in deep sleep needs P4 rev >= 3.0) |
| INA226 battery monitor | power-down mode (`battery_ina226_prepare_sleep()`); `battery_init_ina226()` restores continuous mode |
| ESP32-C6 (WLAN_PWR_EN, 0x44 P0) | off (`BSP_FEATURE_WIFI`) |
| USB-A 5 V boost (USB5V_EN, 0x44 P3) | off (`BSP_FEATURE_USB`) |
| Speaker amplifier (SPK_EN, 0x43 P1) | off (`BSP_FEATURE_SPEAKER`); already switched off at boot, since Draftling plays no sound |
| Camera (0x43 P6) | off (`BSP_FEATURE_CAMERA`) |
| External 5 V (EXT5V_EN, 0x43 P2) | off |
| RTC peripherals domain | powered down |
| Charger (CHG_EN, 0x44 P7) | left on, so the battery charges while the Tab5 is off |

Then it toggles PWROFF_PULSE (PI4IOE5V6408 #2 at 0x44, P4) ten times,
50 ms apart -- the sequence of M5Unified's `Power_Class::powerOff()`.
On battery the power circuit cuts the supply during it; on USB the
board stays powered, the hook returns and `standby_enter_sleep()`
enters deep sleep. The expanders keep their outputs through P4 deep
sleep, and `app_main()` turns the rails back on at the next boot.
Editor state in PSRAM is lost either way; documents are saved first.

- `DRAFTLING_STANDBY_WAKE_ON_TOUCH` must NOT be enabled on Tab5:
  GPIO 23 is not LP_IO and the EXT0 API is not available on the
  P4 at all (`SOC_PM_SUPPORT_EXT0_WAKEUP` is undefined).

If a future Tab5 hardware revision wires a user input to an LP_IO
pin, switch `components/standby/standby.cpp` to
`esp_deep_sleep_enable_gpio_wakeup()` (the P4 equivalent of EXT0)
for that pin.

## USB-A keyboard (preempts BLE)

The Tab5 carries a USB-A host receptacle wired to the P4 USB OTG
peripheral, gated by `BSP_FEATURE_USB` (PI4IOE5V6408 #2 P3) in the
`espressif/m5stack_tab5` BSP. Draftling brings the host stack up
just before the keyboard subsystem (`components/usb_kbd`,
`espressif/usb_host_hid`) and waits
`CONFIG_DRAFTLING_USB_KBD_PROBE_MS` (default 1500 ms) for HID
enumeration.

- If a USB HID keyboard is enumerated in that window, `main.cpp`
  skips `ble_keyboard_init()` entirely. The C6 BT controller is
  never brought up, saving power and avoiding the BLE pairing
  prompt.
- If no USB keyboard is present, the build falls back to the BLE
  HID keyboard path described above.

The decision to bring up BLE is one-shot at boot: if no USB
keyboard was detected at boot we take the BLE path and stay there
even if a USB keyboard is plugged in later. Hot-plugging a USB
keyboard *after* BLE has already started, however, immediately
disables the BLE keyboard interface -- `usb_kbd.cpp` calls
`ble_keyboard_disable()` as soon as the wired keyboard enumerates,
which stops scanning, closes any connected HID device, drops all
event/status callbacks and prevents further BLE messages from
reaching the editor. Going the other way (unplugging the USB
keyboard) does not start BLE pairing -- power-cycle to fall back
to BLE.

## Battery indicator

The Tab5 reads pack voltage from an INA226 power monitor at I2C
address `0x41` on the system bus (`bsp_i2c_get_handle()`). The
backend (`components/battery`, gated on
`CONFIG_DRAFTLING_BATTERY_INA226`) reads the bus-voltage register
(0x02, 1.25 mV/LSB), divides by the configured cell count (2 for
the stock NP-F550 pack), and feeds the result through the standard
4.2-3.6 V LiPo lookup table to derive percentage.

The same backend also exposes `battery_read_charging()` which
returns 1 when current is flowing into the pack. This is read
straight from the INA226 shunt-voltage register (0x01, signed
2.5 uV/LSB) -- positive shunt voltage means current is flowing in
the direction the on-board IP2326 charger drives during charging,
so a non-trivial positive reading indicates the pack is being
charged. The status bar prepends a `+` glyph to the battery
percentage whenever charging is detected (`editor_ui.cpp` /
`batt_timer_cb`, polled every 5 s).

## Charging

The Tab5 carries an autonomous IP2326 Li-ion charger between the
USB-C input and the battery pack. The charger's `CHG_EN` input is
wired to **bit 7 of the second PI4IOE5V6408 I/O expander** at I2C
address `0x44` (output register `OUT_SET` = `0x05`). The
espressif/m5stack_tab5 BSP does *not* expose this pin in
`bsp_feature_enable()`, and M5Stack's reference
`bsp_io_expander_pi4ioe_init()` deliberately boots the expander
with `CHG_EN = 0` -- which is the underlying reason the battery
does not charge out-of-the-box when USB-C is plugged in.

Draftling's `main.cpp` enables charging at boot, right after
`bsp_i2c_init()`, by writing the PI4IOE5V6408 directly through the
shared system bus: it sets bit 7 of `IO_DIR` (`0x03`) to make the
pin an output, **clears bit 7 of `OUT_H_IM` (`0x07`)** to take the
pin out of the high-impedance state the part comes up in, and
finally sets bit 7 of `OUT_SET` (`0x05`) to drive it HIGH. The
`OUT_H_IM` step is the part that is easy to miss: both the silicon
and the upstream `esp_io_expander_pi4ioe5v6408` driver leave every
output high-Z on reset (`OUT_H_IM = 0xFF`), and `bsp_feature_enable()`
only clears that bit for pins it knows about (e.g. `WIFI_EN`). Without
the `OUT_H_IM` clear the `OUT_SET` bit latches but the pin keeps
floating, the IP2326 sees no enable and charging silently never
starts -- which is what produced the original "USB plugged in but
the percentage never moves and the charging icon never appears"
symptom on Tab5. The expander latches the resulting level and retains it across P4
deep sleep (same as `WLAN_PWR_EN` on bit 0 of the same expander --
see the *Sleep and power-off* section above), so the charger keeps running
while the MCU sleeps and the battery comes back full after a long
idle. As a result no special "no sleep while charging" path is
needed in `components/standby/standby.cpp`.
