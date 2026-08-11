# Wi-Fi + Bluetooth on the M5Stack Tab5 (ESP-Hosted-MCU)

The ESP32-P4 has no on-chip 2.4 GHz radio. On the M5Stack Tab5 the
P4 is paired with an on-board ESP32-C6 module that provides Wi-Fi
and BLE; the two chips are wired together over SDIO. Draftling
reaches the C6 through
[ESP-Hosted-MCU](https://github.com/espressif/esp-hosted-mcu):

- `wifi_manager` (Git sync) calls stock `esp_wifi_*` APIs, which
  `espressif/esp_wifi_remote` redirects into hosted RPCs.
- `ble_keyboard` (BLE HID host) brings up Bluedroid the usual way
  but attaches a hosted-VHCI HCI driver instead of the native
  `esp_bt_controller` (the latter does not exist on the P4). The
  switch is gated on `CONFIG_BT_CONTROLLER_DISABLED`, which the
  Tab5 sdkconfig sets.

The P4-side wiring is automatic: it is enabled in
`sdkconfig.defaults.esp32p4`, and `main/main.cpp` calls
`esp_hosted_init()` + `esp_hosted_connect_to_slave()` before
`ble_keyboard_init()` / `wifi_manager_connect()`. The host
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
schematic. They are encoded in `sdkconfig.defaults.esp32p4` as the
per-slot `CONFIG_ESP_HOSTED_PRIV_SDIO_PIN_*_SLOT_1` symbols plus
`CONFIG_ESP_HOSTED_SDIO_GPIO_RESET_SLAVE` (slot 1 is the P4 default
and is the SDMMC slot wired to the on-board C6).

> **Pitfall.** In `espressif/esp_hosted` >= 2.x the
> `CONFIG_ESP_HOSTED_SDIO_PIN_CLK / _CMD / _D0..D3` symbols are
> *non-promptable derived ints* whose values are computed from the
> per-slot `_PRIV_SDIO_PIN_*_SLOT_{0,1}` defaults. Setting
> `CONFIG_ESP_HOSTED_SDIO_PIN_CLK=12` in `sdkconfig.defaults` is
> silently dropped by Kconfig, the upstream P4/SLOT_1 defaults
> (CLK 18, CMD 19, D0 14, D1 15, D2 16, D3 17) win, and SDIO
> bring-up fails with `sdmmc_init_ocr: send_op_cond (1) returned
> 0x107` because the upstream D1 default (15) is the same pin the
> Tab5 uses for the C6 reset line. Always override the
> `_PRIV_SDIO_PIN_*_SLOT_1` knobs instead.

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

## One-time C6 slave firmware flashing

The C6 needs the ESP-Hosted slave firmware. The host side is pinned to
`espressif/esp_hosted >=2.12,<3.0.0` in `main/idf_component.yml` (3.0
replaced the Bluetooth Kconfig / API this project's `ble_keyboard.cpp`
relies on -- see the comment above that dependency), so the
slave image must be flashed with esp-hosted-mcu firmware of version
**2.x**. The M5Stack prebuilt image that ships with M5Tab5-UserDemo
(`ESP32C6-WiFi-SDIO-Interface-V1.4.1-96bea3a_0x0.bin`) was built
against an older 1.x slave and its RPC interface is **not** compatible
with `esp_hosted >= 2.x` -- using it will look exactly like "C6 not
responding" (the SDIO bus comes up but the INIT event never
arrives). Build a matching 2.x slave from upstream.

### 1. Build the slave firmware

```bash
git clone --recurse-submodules https://github.com/espressif/esp-hosted-mcu.git
cd esp-hosted-mcu/slave
idf.py set-target esp32c6
idf.py menuconfig   # see knobs below
idf.py build
```

### 2. Put the C6 into download mode

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

### 3. Flash and clean up

Here `/dev/ttyUSB0` corresponds to the USB UART adapter, so the name
could be different (such as /dev/ttyACM0).

```bash
idf.py -p /dev/ttyUSB0 flash
```

Once flashing is done, **remove the G9-to-GND short** and reset
the C6 (cycling Tab5 power is the easiest way). On the next P4
boot the host should reach `ESP-Hosted link up` and the
capabilities line should include `HCI over SDIO / BLE only`.

The C6 firmware persists across P4 reflashes; re-flashing it is
only necessary if you upgrade the host-side `espressif/esp_hosted`
component to a version whose RPC interface is incompatible with
the slave image you currently have on the C6.

## Expected boot log

On a successful link bring-up `main` prints lines similar to:

```
I (xxxx) Draftling: Initializing ESP-Hosted link to ESP32-C6...
I (xxxx) transport: Attempt connection with slave: retry[0]
I (xxxx) transport: Reset slave using GPIO[15]
I (xxxx) sdio_wrapper: SDIO master: Data-Lines: 4-bit Freq(KHz)[40000 KHz]
I (xxxx) sdio_wrapper: GPIOs: CLK[12] CMD[13] D0[11] D1[10] D2[9] D3[8] Slave_Reset[15]
I (xxxx) H_SDIO_DRV: SDIO Host operating in PACKET MODE
I (xxxx) transport: Received INIT event from ESP32 peripheral
I (xxxx) transport: capabilities: 0xd
I (xxxx) transport:      * WLAN
I (xxxx) transport:        - HCI over SDIO
I (xxxx) transport:        - BLE only
I (xxxx) Draftling: ESP-Hosted link up
I (xxxx) Draftling: Initializing Bluetooth keyboard...
I (xxxx) BLEKeyboard: Hosted BT controller enabled (BLE on C6)
I (xxxx) BLEKeyboard: Hosted VHCI attached to Bluedroid
I (xxxx) BLEKeyboard: Bluedroid enabled
```

## Common bring-up failures

- **`Reset slave using GPIO[15]` then a hang or
  `Failed to communicate with slave`** -- the C6 is not running
  slave firmware, or the slave image is incompatible with the
  host-side `espressif/esp_hosted` version. Re-flash the C6 with
  the image listed above.
- **`SDIO read/write error` or `CMD53 timeout`** -- usually a
  signal-integrity issue with the SDIO bus; lower
  `CONFIG_ESP_HOSTED_SDIO_CLOCK_FREQ_KHZ` (e.g. to `20000`) and
  re-test.
- **`esp_hosted_connect_to_slave failed (-1)`** -- the SDIO pin
  map does not match the actual Tab5 wiring. Verify the
  `CONFIG_ESP_HOSTED_PRIV_SDIO_PIN_*_SLOT_1` values (CLK / CMD /
  D0 / D1_4BIT_BUS / D2_4BIT_BUS / D3_4BIT_BUS) against the
  schematic of your specific Tab5 revision; do **not** edit the
  `CONFIG_ESP_HOSTED_SDIO_PIN_*` symbols (see "Pitfall" above).
- **Wi-Fi associates but BLE never connects** -- the C6 image is
  Wi-Fi-only. Re-flash with a build that has
  `HCI over SDIO / BLE only` in its capabilities line (see the
  capabilities log above).

## Deep sleep

The Tab5 standby path is real ESP32-P4 deep sleep
(`esp_deep_sleep_start()`). The GT911 touch INT (GPIO 23) and the
on-board USER button are not wired to LP_IO pins (GPIO 0-15) on
the P4, so no GPIO wake source is available. The chip therefore
wakes only via the hardware **RESET** button: it cold-boots and
the editor restores from autosave on the next run.

Implications:
- Editor state in PSRAM is lost across standby; rely on autosave.
- The ESP32-C6 co-processor is dropped before deep sleep by
  `pre_sleep_tab5_deinit()` in `main/main.cpp`, which calls
  `bsp_feature_enable(BSP_FEATURE_WIFI, false)` to clear
  WLAN_PWR_EN (PI4IOE5V6408 #2, address 0x44, P0). The expander
  latches the level across the P4's deep-sleep / reset
  transitions, so the C6 stays off until the next cold boot
  re-runs `bsp_feature_enable(BSP_FEATURE_WIFI, true)` in
  `app_main()`. The SDIO link to the C6 is torn down implicitly
  when the P4's SDIO peripheral powers off, and
  `esp_hosted_init()` re-establishes it on the next cold boot.
  The same pre-sleep hook also puts the GT911 touch controller
  to sleep (`touchscreen_sleep()`), blanks the MIPI-DSI panel +
  backlight (`display_deep_sleep_prepare()`), and powers down
  the RTC peripherals domain
  (`esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF)`).
- `DRAFTLING_STANDBY_WAKE_ON_TOUCH` must NOT be enabled on Tab5:
  GPIO 23 is not LP_IO and the EXT0 API is not available on the
  P4 at all (`SOC_PM_SUPPORT_EXT0_WAKEUP` is undefined).

If a future Tab5 hardware revision wires a user input to an LP_IO
pin, switch `components/standby/standby.cpp` to
`esp_deep_sleep_enable_gpio_wakeup()` (the P4 equivalent of EXT0)
for that pin.

## USB-A keyboard (preempts BLE)

The Tab5 carries a USB-A host receptacle wired to the P4 USB OTG
peripheral, gated by `BSP_FEATURE_USB` (PI4IOE5V6408 #2 P5) in the
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
see the *Deep sleep* section above), so the charger keeps running
while the MCU sleeps and the battery comes back full after a long
idle. As a result no special "no sleep while charging" path is
needed in `components/standby/standby.cpp`.
