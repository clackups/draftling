#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Battery monitor.
 *
 * Two backends are exposed:
 *
 *   - ADC + resistive divider (battery_init): used on boards that
 *     wire the LiPo cell through a divider into an ESP32-S3 ADC1 pin
 *     (Waveshare ESP32-S3-RLCD-4.2, M5Stack PaperS3,
 *     Waveshare Touch-LCD-3.49). Voltage maps
 *     to percentage via a fixed 5-step LiPo discharge table:
 *       >= 4.10 V -> 100 %  (full)
 *       >= 3.95 V ->  75 %
 *       >= 3.80 V ->  50 %
 *       >= 3.60 V ->  25 %
 *       <  3.60 V ->   0 %  (empty)
 *     Readings are smoothed with an exponential moving average over
 *     8 samples to reduce ADC noise.
 *
 *   - TI BQ27220 fuel gauge over I2C (battery_init_bq27220): used on
 *     boards that route the cell through a dedicated coulomb counter
 *     (LilyGO T5 E-Paper S3 Pro / Pro Lite). Only the gauge's
 *     Voltage register is used; percentage is derived from that
 *     voltage via the same LiPo discharge LUT the ADC backend uses,
 *     because the gauge's StateOfCharge register requires Data
 *     Memory programming (Design Capacity, chemistry, etc.) and a
 *     learning cycle that the factory firmware never performs.
 */

/*
 * Initialize the ADC for battery voltage measurement.
 * Must be called once before any other battery_* function.
 *
 *   gpio_num     -- ADC1 GPIO connected to the divider output
 *   enable_gpio  -- GPIO that powers the divider, or -1 if always on
 *   divider      -- multiplier from measured pin voltage back to the
 *                   battery voltage. Use 2 for a balanced 1:1 divider
 *                   (R_top == R_bottom, V_pin = V_bat / 2) and 3 for
 *                   a 2:1 divider where R_top is twice R_bottom
 *                   (V_pin = V_bat / 3).
 *
 * Returns 0 on success, non-zero on failure.
 */
int battery_init(int gpio_num, int enable_gpio, int divider);

/*
 * Initialize a BQ27220 fuel-gauge backend on an existing I2C master
 * bus (driver/i2c_master.h). The bus must already have been created
 * by the caller and remains owned by the caller (the battery
 * component only attaches a device handle).
 *
 *   i2c_master_bus -- opaque pointer to an i2c_master_bus_handle_t
 *                     (declared as void * here so the public header
 *                     does not need to pull in driver/i2c_master.h).
 *
 * Returns 0 on success, non-zero on failure. After a successful call,
 * `battery_read_mv()` reports the cell voltage in millivolts from
 * the BQ27220 Voltage register (0x08), and `battery_read_percent()`
 * runs that voltage through the same LiPo discharge lookup table
 * used by the ADC backend. The BQ27220's StateOfCharge register
 * (0x2C) is NOT used: out of the factory the gauge ships with
 * default Data Memory and its Impedance-Track SoC stays pinned
 * around 50 % regardless of actual cell voltage, even after several
 * full discharge/charge cycles. Treating the gauge as a voltmeter
 * sidesteps the required-but-unprogrammed DM setup.
 *
 * Used on boards that route the LiPo cell through a TI BQ27220
 * coulomb counter on I2C (addr 0x55) instead of a GPIO ADC divider,
 * such as the LilyGO T5 E-Paper S3 Pro / Pro Lite.
 */
int battery_init_bq27220(void *i2c_master_bus);

/*
 * Initialize a TI INA226 power-monitor backend on an existing I2C
 * master bus. The INA226 measures battery rail voltage on its BUS
 * input; this driver only reads the bus voltage register and maps
 * it to percentage via a Li-ion discharge curve scaled by `cells`.
 *
 *   i2c_master_bus -- opaque pointer to an i2c_master_bus_handle_t
 *                     (driver/i2c_master.h).
 *   i2c_addr       -- I2C address of the INA226 (0x40..0x4F). The
 *                     M5Stack Tab5 wires the part at 0x41.
 *   cells          -- number of Li-ion cells in series the INA226
 *                     bus rail measures. Use 1 for a single 18650 /
 *                     LiPo cell (3.0-4.2 V) and 2 for an NP-F550-
 *                     style 2S pack (6.0-8.4 V, e.g. the Tab5).
 *
 * Returns 0 on success, non-zero on failure. After a successful
 * call, `battery_read_mv()` reports the per-cell voltage (so the
 * existing 3.0-4.2 V LiPo mapping in mv_to_percent() applies
 * regardless of `cells`) and `battery_read_percent()` runs that
 * voltage through the same lookup table the ADC backend uses.
 *
 * The INA226 has no SoC / coulomb-count state, so percentage is
 * voltage-derived and approximate.
 */
int battery_init_ina226(void *i2c_master_bus, int i2c_addr, int cells);

/*
 * Initialize a CellWise CW2017 fuel-gauge backend on an existing I2C
 * master bus, at the fixed 7-bit address 0x63. Used on the Xteink X4
 * Pro. The bus must already have been created by the caller and
 * remains owned by the caller.
 *
 *   i2c_master_bus -- opaque pointer to an i2c_master_bus_handle_t.
 *
 * Unlike BQ27220, the CW2017 reports State-of-Charge (register 0x04)
 * as a direct integer percentage -- no voltage-to-percent LUT needed
 * -- but only once a matching 80-byte "BATINFO" battery profile is
 * resident; it reports 0 % otherwise. This routine verifies (or, on
 * a first boot / after a full-flash, uploads) that profile before
 * returning. Register map, reset sequence and the profile bytes were
 * recovered from the Xteink X4 Pro OEM firmware by the FreeInk SDK
 * (github.com/Free-Ink/freeink-sdk, MIT licensed).
 *
 * There is no charger IC on the X4 Pro's I2C bus and the CW2017 has
 * no current register, so battery_read_charging() always returns -1
 * (unknown) for this backend.
 *
 * Returns 0 on success, non-zero on failure.
 */
int battery_init_cw2017(void *i2c_master_bus);

/*
 * Initialize the TI BQ25896 single-cell Li-ion charger on an existing
 * I2C master bus (driver/i2c_master.h). This is a charger, not a
 * monitor: it does not change what battery_read_mv() /
 * battery_read_percent() return. Calling it is independent of the
 * monitor backend selection (the LilyGO T5 E-Paper S3 Pro uses the
 * BQ27220 fuel gauge for SoC and the BQ25896 for charging on the
 * same I2C bus).
 *
 *   i2c_master_bus -- opaque pointer to an i2c_master_bus_handle_t.
 *
 * The default BQ25896 power-on configuration is conservative:
 * IINLIM defaults to 500 mA (USB SDP), AUTO_DPDM is enabled (so a
 * fresh D+/D- detection on every plug-in keeps resetting IINLIM
 * back to 500 mA when no host is present), and the I2C watchdog
 * times out after 40 s and reverts any host-written settings. The
 * net effect on boards like the LilyGO T5 E-Paper S3 Pro is that
 * the cell only ever charges at ~500 mA regardless of what the
 * adapter can deliver.
 *
 * This routine fixes that by:
 *   - disabling the source-detection state machine in REG02
 *     (ICO_EN, HVDCP_EN, MAXC_EN, AUTO_DPDM_EN all cleared) so the
 *     Input Current Optimizer cannot silently clamp the actual input
 *     limit (IDPM_LIM) below IINLIM after detecting VBUS sag, no
 *     QC/MaxCharge handshakes are attempted, and re-plug does not
 *     snap IINLIM back to the USB-SDP 500 mA default,
 *   - raising IINLIM to 2000 mA and clearing EN_ILIM (REG00 bit 6)
 *     so the register value is the sole input-current ceiling -- the
 *     on-board ILIM resistor on the LilyGO T5 is sized for ~500 mA
 *     and would otherwise clip the 2 A request,
 *   - raising the fast-charge current ICHG to 1024 mA (0.5C for a
 *     ~2 Ah cell),
 *   - forcing VINDPM to an absolute 3.9 V (REG0D bit 7 = 1) so the
 *     chip cannot throttle input current to hold VBUS up when a
 *     thin USB cable sags -- the relative-VINDPM POR default
 *     latches at ~4.4 V on a 5 V brick and otherwise crawls in
 *     at ~0.11 A regardless of IINLIM, and
 *   - disabling the I2C watchdog so the above settings persist.
 *
 * Returns 0 on success, non-zero if the chip is not present or any
 * I2C write fails.
 */
int battery_init_bq25896(void *i2c_master_bus);

/*
 * Read the current battery voltage in millivolts.
 * Returns 0 if the sensor has not been initialized.
 */
int battery_read_mv(void);

/*
 * Read the battery charge percentage (0-100).
 * Returns -1 if the sensor has not been initialized.
 */
int battery_read_percent(void);

/*
 * Report whether the battery is currently being charged from an
 * external source (USB-C / DC jack).
 *
 * Returns:
 *    1 -- charging (current flowing into the cell)
 *    0 -- discharging or idle (running on battery only / full)
 *   -1 -- unknown (backend cannot detect charge state, or not initialized)
 *
 * Backend support matrix:
 *   - INA226   (M5Stack Tab5): reads the signed shunt-voltage register
 *               and reports the polarity. Sign convention is positive =
 *               into the cell on this board.
 *   - BQ27220  (LilyGO T5 Pro / Pro Lite): reads the Flags register
 *               (0x06); the DSG bit (bit 0) is 0 while charging or
 *               full, 1 while discharging.
 *   - ADC backend / no backend: always returns -1.
 *
 * Callers (e.g. the editor status bar) should hide the charging
 * indicator when this function returns -1.
 */
int battery_read_charging(void);

/*
 * Read the instantaneous cell current in milliamps, signed.
 *
 *   > 0 -- current flowing INTO the cell (charging)
 *   < 0 -- current flowing OUT of the cell (discharging / system load)
 *   = 0 -- no measurable current
 *
 * Returns INT32_MIN if the active backend cannot report current
 * (only the BQ27220 fuel gauge does today; ADC, INA226 and "no
 * backend" all return INT32_MIN).
 *
 * Source on BQ27220: register 0x0C (Current), 16-bit signed two's
 * complement, LSB = 1 mA (BQ27220 TRM section 4.1, "Current").
 */
int battery_read_current_ma(void);

/*
 * BQ25896 sleep prep: when the charger reports VBUS is absent (i.e.
 * the device is running on battery), drive EN_HIZ (REG00 bit 7) so
 * the chip drops to its battery-only quiescent draw (~50 uA per the
 * BQ25896 datasheet) instead of the ~1.5 mA it pulls with EN_HIZ=0
 * even when no USB cable is plugged in. No-op when VBUS is present
 * (we want the charger to keep working off USB) or when the chip
 * was not initialised. Cold-boot from deep sleep re-runs
 * battery_init_bq25896(), which clears EN_HIZ back to 0.
 */
void battery_bq25896_prepare_sleep(void);

/*
 * Re-write the small set of BQ25896 registers we care about
 * (REG02 with ICO_EN=0 + HVDCP/MAXC/AUTO_DPDM=0, REG07 watchdog
 * disabled). Cheap (two I2C transactions) and idempotent: if the
 * chip rebooted or a noise event reverted the registers to their
 * 500 mA USB-SDP defaults, this snaps them back. Safe to call from
 * a periodic timer. No-op when the chip was not initialised.
 */
void battery_bq25896_reassert_config(void);

/*
 * Log the BQ25896 charger status as a single INFO line. Reads the
 * full diagnostic register set (REG00, REG0B, REG0C, REG0E, REG0F,
 * REG10, REG11, REG12, REG13) so a slow-charge symptom can be
 * localised to a specific status bit (THERM_STAT, VDPM_STAT,
 * NTC_FAULT, IDPM_STAT, etc.) without a bench meter. No-op when
 * the chip was not initialised.
 */
void battery_bq25896_dump_status(void);

/*
 * Returns 1 if the BQ25896 has been initialised AND currently sees
 * a valid VBUS (REG11 bit 7 VBUS_GD set), 0 if it is initialised
 * but VBUS is absent, -1 if the chip is not initialised or the
 * read failed. Used by the standby pre-sleep hook to decide
 * whether to HIZ the chip and by the debug logger to decide
 * whether to emit periodic status dumps.
 */
int battery_bq25896_vbus_present(void);

#ifdef __cplusplus
}
#endif
