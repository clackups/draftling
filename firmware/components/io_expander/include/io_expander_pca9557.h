/*
 * NXP PCA9557 8-bit I2C IO-expander driver (IO0..IO7).
 *
 * Used where a touch controller's reset / INT lines sit behind a
 * PCA9557 on the shared I2C bus (currently the Elecrow CrowPanel ESP32
 * HMI 5.0" v3.0, address 0x18). Compiled in only when
 * CONFIG_DRAFTLING_HAS_PCA9557 is set; on every other board the
 * functions are no-op stubs so callers do not need conditional
 * compilation.
 *
 * Registers (one byte each, bit n = IOn):
 *   0x00 input port, 0x01 output port, 0x02 polarity inversion,
 *   0x03 configuration (1 = input, the reset state).
 * IO0 is an open-drain output; IO1..IO7 are push-pull.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <esp_err.h>
#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Attach to the PCA9557 at `addr` on an already created driver-NG I2C
 * master bus (i2c_master_bus_handle_t, passed as void*). Returns
 * ESP_ERR_NOT_FOUND when nothing answers at that address (e.g. a board
 * revision without the chip); later calls then return
 * ESP_ERR_INVALID_STATE. Idempotent.
 *
 * No-op (returns ESP_OK) on boards without CONFIG_DRAFTLING_HAS_PCA9557.
 */
esp_err_t pca9557_init(void *i2c_bus, uint8_t addr);

/* True once pca9557_init() has found the chip. */
bool pca9557_is_present(void);

/* Make IO<pin> (0..7) an output driven high (level = true) or low. */
esp_err_t pca9557_set_pin(int pin, bool level);

/* Return IO<pin> to an input (high impedance). */
esp_err_t pca9557_release_pin(int pin);

#ifdef __cplusplus
}
#endif
