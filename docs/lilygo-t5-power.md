# LilyGO T5 E-Paper S3 Pro / Pro Lite -- deep-sleep power budget

_Paths below (`main/...`, `components/...`) are relative to `firmware/`._

The LilyGO T5 board carries an ESP32-S3, a 1500 mAh NP-F-class
Li-ion pack, an EPDIY-driven e-paper panel (TPS65185 + PCA9535
expander), a GT911 capacitive touch controller, a BQ27220 fuel
gauge, a BQ25896 charger, a PCF8563TS RTC, a MicroSD slot, and
on the *Pro* variant an SX1262 LoRa radio plus a MIA-M10Q GPS.

With Draftling's default deep-sleep path the board drew **~30 mA**
in sleep, draining the pack in roughly two days. The cause was a
handful of peripherals that the ESP-IDF default `esp_deep_sleep_start()`
does not touch -- it powers down the digital domain but leaves the
RTC peripherals up and leaves analog peripherals in whatever state
their drivers left them.

The fix is `pre_sleep_t5_deinit()` in `main/main.cpp`, registered
as the standby pre-sleep callback under
`CONFIG_DRAFTLING_MODEL_LILYGO_T5_EPD_S3_PRO` (which covers both the
Pro and the Pro Lite SKUs). It walks the
following list of peripherals immediately before
`esp_deep_sleep_start()` runs:

| Peripheral             | Sleep state                            | Mechanism                                                                                                  |
| ---------------------- | -------------------------------------- | ---------------------------------------------------------------------------------------------------------- |
| GT911 touch controller | sleep mode (~8 uA)                     | `touchscreen_sleep()` writes `0x05` to register `0x8040` (GT911 DS sec.6).                                    |
| EPDIY (TPS65185 + PCA9535) | TPS65185 OFF (<1 uA)               | `display_deep_sleep_prepare()` -> `epd_poweroff()` drives `WAKEUP` LOW via the PCA9535, then `epd_deinit()`. |
| Front-light LED (GPIO 11) | held LOW across deep sleep         | `display_deep_sleep_prepare()` zeroes LEDC duty, then `gpio_reset_pin` + `gpio_set_level(0)` + `gpio_hold_en` so the pad stays at 0 V; `gpio_deep_sleep_hold_en()` latches the hold. |
| SX1262 LoRa (Pro only) | SetSleep cold start (~600 nA)         | `t5_lora_sleep()` issues the 2-byte SPI sequence `0x84 0x00` over SPI3 (sx126x DS sec.13.1.7). Harmless NACK on the Lite variant where the chip is depopulated. Also issued at boot from `t5_disable_radios_at_boot()` (right after SD init) so the radio never burns its ~600 uA STBY_RC current during the active session. |
| MIA-M10Q GPS (Pro only) | UBX-RXM-PMREQ backup (~15 uA)        | `t5_gps_sleep()` opens UART1 at 38400 baud on GPIO 43/44 and writes the 16-byte UBX-RXM-PMREQ v0 frame (`B5 62 02 41 08 00 00 00 00 00 06 00 00 00 51 4B` -- duration=0, flags=backup\|force; u-blox M10 interface description sec.3.13.6.4), then releases the UART pins. The MIA-M10Q has no power-enable GPIO on the H752-01 schematic, so this is the only software lever for its ~25 mA acquisition draw. Also issued at boot from `t5_disable_radios_at_boot()`. Lite variants have the GPS depopulated; the bytes clock harmlessly into an open pin. |
| MicroSD card           | bus released, card to standby          | `sd_card_deinit()` calls `esp_vfs_fat_sdcard_unmount`, removes the SPI device, then `spi_bus_free(SPI3_HOST)`. |
| Unused RTC-IO pins     | isolated                                | `t5_isolate_unused_gpios()` calls `rtc_gpio_isolate()` on the four genuinely spare RTC-IO pins (GPIO 1-4). Skips BOOT (EXT0 wake), the front-light (held LOW), the four SPI3 / SD pins (released by `sd_card_deinit`), the EPD 8-bit data bus (GPIO 5-10 and 15-18, released by `epd_lcd_deinit` inside `display_deep_sleep_prepare`), and the USB-Serial-JTAG console pins (GPIO 19/20 -- isolating them would silently cut the last lines of log before `esp_deep_sleep_start`). |
| RTC peripherals domain | powered off                             | `esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF)` -- safe because Draftling has no ULP and no RTC SLOW data to retain across wake. |
| BQ25896 charger        | autonomous (intentional)               | left running so a USB plug-in still tops the pack up during sleep. `battery_init_bq25896()` brings the chip out of its default detection-heavy state at boot: it disables the I2C watchdog, clears `ICO_EN` / `HVDCP_EN` / `MAXC_EN` / `AUTO_DPDM_EN` in REG02 (so the Input Current Optimizer does not silently clamp IDPM_LIM to ~150 mA on cable sag and re-plug doesn't snap IINLIM back to 500 mA SDP), clears `EN_ILIM` in REG00 (so the external ILIM resistor cannot clip the register-set limit), raises `IINLIM` to 2 A / `ICHG` to 1024 mA, and forces VINDPM to an absolute 3.9 V via REG0D (so the chip cannot throttle input current to hold VBUS up when a thin USB cable sags -- without this the relative-VINDPM POR default latches at ~4.4 V on a 5 V brick and pulls charging back down to ~0.11 A even after the ICO fix). The boot log reports the latched IINLIM, ICHG, post-ICO IDPM_LIM, VINDPM and REG0B status so a "charging slowly" regression is immediately visible. |
| BQ27220 fuel gauge     | autonomous (intentional)               | low-power coulomb counter, idle current ~7 uA. |
| PCF8563 RTC            | autonomous (intentional)               | idle current ~250 nA. |

The sequence in `pre_sleep_t5_deinit()` matters: the touch
controller and EPDIY must be put to sleep *before* their I2C bus is
torn down by `epd_deinit()`, and the SX1262 must be put to sleep
*before* `sd_card_deinit()` frees SPI3. The MIA-M10Q is on UART1
(independent of SPI3 and the shared I2C bus), so its sleep step can
sit anywhere in the sequence; we run it right after the LoRa step to
keep all radio teardown together.

After this hook, measured sleep current is in the tens of uA --
several orders of magnitude below the original 30 mA -- giving
roughly a month of idle on a fully charged pack.

## What we do **not** touch

- `pre_sleep_autosave()` runs first inside `pre_sleep_t5_deinit()`
  to save the open document, persist cursor/scroll metadata, and
  wipe the panel to white. This is the standard cross-board
  pre-sleep work shared with PaperS3.
- The BQ25896 charger and BQ27220 fuel gauge are left on so the
  battery keeps charging while the MCU sleeps and the SoC has an
  accurate percentage reading on the next wake.
- The PCF8563 RTC is left on so the on-screen clock survives sleep.

## Cross-references

- `components/touchscreen/touchscreen.cpp` -- `touchscreen_sleep()` (GT911 0x8040 cmd 0x05).
- `components/display/display_epdiy.cpp` -- `display_deep_sleep_prepare()` (LEDC zero, GPIO 11 hold, epdiy teardown).
- `components/sd_card/sd_card.cpp` -- `sd_card_deinit()` (SPI bus free).
- `main/main.cpp` -- `pre_sleep_t5_deinit()` (orchestrator), `t5_lora_sleep()`, `t5_gps_sleep()`, `t5_disable_radios_at_boot()`, `t5_isolate_unused_gpios()`.
- `components/standby/standby.cpp` -- `standby_enter_sleep()` calls the registered pre-sleep callback before `esp_deep_sleep_start()`.
