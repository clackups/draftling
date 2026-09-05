/*
 * Hardware power-latch + PWR-button driver.
 *
 * Implemented for boards with CONFIG_DRAFTLING_HAS_POWER_LATCH (the
 * Waveshare ESP32-S3-Touch-LCD-3.49 is currently the only one). On
 * these boards a TCA9554 I2C IO-expander pin acts as a latch that
 * keeps the battery rail alive after the user releases the dedicated
 * PWR button; driving the latch pin LOW cuts the rail and fully
 * powers the board off. A momentary PWR press on the unpowered
 * board applies VBAT just long enough for this code to run at boot
 * and re-close the latch.
 *
 * Pin assignments and the latch bit live in the per-board block of
 * main/app_config.h and are passed in via power_config_t so this
 * component stays board-agnostic.
 *
 * The board is closed-on at power_init() time. The same call also
 * starts a 50 ms-period polling timer on the PWR button GPIO; a
 * hold of POWER_LONG_PRESS_MS or longer fires the pre-off callback
 * (typically the editor auto-save) and then calls power_off().
 *
 * power_off() cuts the latch and then waits a short while. On
 * battery the board has already lost power by then; on USB it
 * returns so the caller can fall through to a regular deep sleep.
 */
#pragma once

#include <stdint.h>
#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Hold time in milliseconds that constitutes a "long press" of the
 * PWR button. Matches the Waveshare reference firmware (their
 * multi_button library uses a 1500 ms long-press threshold by
 * default). */
#define POWER_LONG_PRESS_MS 1500

/* Callback signature for the PWR long-press handler. Invoked once
 * from the polling timer when the user holds the PWR button for
 * POWER_LONG_PRESS_MS or longer. Typically wired to
 * standby_enter_sleep() so the editor runs its usual pre-sleep
 * auto-save, cuts the backlight, cuts the power latch, and enters
 * deep sleep -- the same shutdown sequence used on inactivity
 * timeout. */
typedef void (*power_long_press_cb_t)(void);

/* Per-board power-latch configuration. Populate in main.cpp from
 * the matching app_config.h defines and pass to power_init(). */
typedef struct {
    int     pwr_button_gpio;   /* PWR button input pin (active LOW) */
    int     i2c_sda;           /* TCA9554 I2C bus SDA pin */
    int     i2c_scl;           /* TCA9554 I2C bus SCL pin */
    uint8_t tca9554_addr;      /* TCA9554 7-bit I2C address (e.g. 0x20) */
    uint8_t latch_bit;         /* TCA9554 pin (0..7) that latches VBAT */
} power_config_t;

/*
 * Initialize the power latch and the PWR button.
 *
 * Closes the TCA9554 latch (drives latch_bit HIGH) so the board
 * stays powered after the user releases the boot-time PWR press,
 * then starts the PWR-button polling timer. Returns 0 on success,
 * negative on I2C/GPIO/timer failure.
 *
 * No-op (and returns 0) on boards without CONFIG_DRAFTLING_HAS_POWER_LATCH.
 *
 * Must be called early in app_main(), before any other code can
 * draw enough current to brown out the board.
 */
int power_init(const power_config_t *cfg);

/*
 * Register a callback invoked from the PWR-button polling timer
 * when the user holds the PWR button for at least
 * POWER_LONG_PRESS_MS. Pass NULL to clear. Only one callback is
 * supported. The callback is responsible for executing the full
 * shutdown sequence (auto-save, display blank, latch cut, deep
 * sleep) -- typically by calling standby_enter_sleep().
 *
 * No-op on boards without CONFIG_DRAFTLING_HAS_POWER_LATCH.
 */
void power_set_long_press_cb(power_long_press_cb_t cb);

/*
 * Cut the power latch (drive latch_bit LOW). On battery this powers
 * the board down immediately and the function never returns. On USB
 * the latch has no effect on the rail; the function returns so the
 * caller can fall through to a regular ESP32 deep-sleep cycle.
 *
 * Does NOT invoke any user callback -- call this directly from your
 * shutdown sequence, after the pre-sleep auto-save and the
 * backlight-cut have already run.
 *
 * No-op (returns immediately) on boards without
 * CONFIG_DRAFTLING_HAS_POWER_LATCH or when the TCA9554 I2C init
 * failed (the firmware logs a warning and runs without the latch).
 */
void power_off(void);

#ifdef __cplusplus
}
#endif
