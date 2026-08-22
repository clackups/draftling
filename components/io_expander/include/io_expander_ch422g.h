/*
 * WCH CH422G I2C IO-expander driver (EXIO0..EXIO7).
 *
 * Used on boards where the LCD reset, backlight enable, touch reset
 * and/or SD card chip-select lines are not wired to direct ESP32-S3
 * GPIOs but instead sit behind a CH422G expander on the shared I2C
 * bus (currently the Waveshare ESP32-S3-Touch-LCD-7). Compiled in
 * only when CONFIG_DRAFTLING_HAS_CH422G is set; on every other board
 * ch422g_init() / ch422g_set_pin() are no-op stubs so callers do not
 * need conditional compilation.
 *
 * The CH422G has no single "register address" the way a TCA9554 or
 * PCA9535 does: each internal register is addressed as a distinct
 * 7-bit I2C slave address that accepts exactly one data byte. This
 * driver only implements the two registers Draftling needs:
 *   0x24 (WR_SET) -- system/mode register: bit0 = 1 selects "all 8
 *                    EXIO pins are outputs" (POR default is all-input)
 *   0x38 (WR_IO)  -- output level for EXIO0..EXIO7, one bit per pin
 * The CH422G has no read-modify-write, so the output byte is shadowed
 * in RAM and re-sent in full on every ch422g_set_pin() call.
 */
#pragma once

#include <stdbool.h>
#include <esp_err.h>
#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Bring up the CH422G on an already-created driver-NG I2C master bus
 * (i2c_master_bus_handle_t, passed as void* so this header does not
 * need to drag in driver/i2c_master.h) and switch all 8 EXIO pins to
 * push-pull outputs, driven HIGH (the chip's power-on-reset output
 * level). Idempotent: a second call is a no-op.
 *
 * No-op (returns ESP_OK) on boards without CONFIG_DRAFTLING_HAS_CH422G.
 */
esp_err_t ch422g_init(void *i2c_bus);

/*
 * Drive one EXIO pin (0..7) high (level = true) or low (level =
 * false). Safe to call from any task; each call re-sends the full
 * shadowed output byte. No-op if ch422g_init() has not been called
 * or failed.
 */
void ch422g_set_pin(int pin, bool level);

#ifdef __cplusplus
}
#endif
