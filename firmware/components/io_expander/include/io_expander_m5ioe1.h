/*
 * M5Stack M5IOE1 I2C IO-expander driver (PYG1..PYG14).
 *
 * Used on boards where the e-paper panel supply and reset, the touch
 * controller's reset and supply and the MicroSD supply sit behind an
 * M5IOE1 on the shared I2C bus (currently the M5Stack PaperMono).
 * Compiled in only when CONFIG_DRAFTLING_HAS_M5IOE1 is set; on every
 * other board the functions are no-op stubs so callers do not need
 * conditional compilation.
 *
 * Only the digital-output subset of the chip is implemented. Each
 * GPIO setting is a 16-bit little-endian register pair (low byte =
 * PYG1..PYG8, high byte = PYG9..PYG14):
 *   0x03/0x04 GPIO_MODE  1 = output
 *   0x05/0x06 GPIO_OUT   output level
 *   0x09/0x0A GPIO_PU, 0x0B/0x0C GPIO_PD  pull-up / pull-down
 *   0x13/0x14 GPIO_DRV   1 = open-drain, 0 = push-pull
 *   0x23      I2C_CFG    [3:0] idle-sleep timeout, 0 = never sleep
 * (register map from M5Stack's MIT-licensed M5IOE1 library).
 */
#pragma once

#include <stdbool.h>
#include <esp_err.h>
#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Attach to the M5IOE1 at its fixed address (0x4F) on an already
 * created driver-NG I2C master bus (i2c_master_bus_handle_t, passed as
 * void*) and disable its I2C idle sleep. Idempotent.
 *
 * No-op (returns ESP_OK) on boards without CONFIG_DRAFTLING_HAS_M5IOE1.
 */
esp_err_t m5ioe1_init(void *i2c_bus);

/*
 * Configure PYG<pin> (1..14, the numbering used by M5Stack's pin maps)
 * as a push-pull output without pulls and drive it high (level = true)
 * or low. Returns ESP_ERR_INVALID_STATE before m5ioe1_init().
 */
esp_err_t m5ioe1_set_pin(int pin, bool level);

/*
 * Return PYG<pin> to a high-impedance input (the chip's reset state),
 * e.g. to stop powering a peripheral before deep sleep.
 */
esp_err_t m5ioe1_release_pin(int pin);

#ifdef __cplusplus
}
#endif
