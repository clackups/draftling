/* bl_ledc.h -- LEDC PWM set-up and duty writes shared by the display
 * backends that dim a backlight / front-light with a plain GPIO.
 * Everything runs in LEDC_LOW_SPEED_MODE; each backend keeps its own
 * timer, channel, resolution, frequency and percent-to-duty mapping
 * (polarity, duty floor, full-on value). */
#pragma once

#include <stdint.h>
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <esp_err.h>

/* Route pin to an LEDC channel at initial_duty. Releases a deep-sleep
 * pad hold first: display_deep_sleep_prepare() latches the pin "off",
 * the latch survives the wake reset, and LEDC cannot drive a held pad.
 * Set-up errors abort, as at every other point of display init. */
static inline void bl_ledc_setup(int pin, ledc_timer_t timer, ledc_channel_t channel,
                                 ledc_timer_bit_t resolution, uint32_t freq_hz,
                                 ledc_clk_cfg_t clk, uint32_t initial_duty)
{
    gpio_hold_dis((gpio_num_t)pin);

    ledc_timer_config_t t = {};
    t.speed_mode      = LEDC_LOW_SPEED_MODE;
    t.duty_resolution = resolution;
    t.timer_num       = timer;
    t.freq_hz         = freq_hz;
    t.clk_cfg         = clk;
    ESP_ERROR_CHECK(ledc_timer_config(&t));

    ledc_channel_config_t c = {};
    c.gpio_num   = pin;
    c.speed_mode = LEDC_LOW_SPEED_MODE;
    c.channel    = channel;
    c.timer_sel  = timer;
    c.duty       = initial_duty;
    c.hpoint     = 0;
    ESP_ERROR_CHECK(ledc_channel_config(&c));
}

/* percent (clamped to 0..100) of duty_max. */
static inline uint32_t bl_ledc_percent_duty(int percent, uint32_t duty_max)
{
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    return (uint32_t)((duty_max * (uint32_t)percent) / 100);
}

/* Apply a duty. A failure is logged, not fatal: a missed brightness
 * change must not reboot the device and lose unsaved text. */
static inline void bl_ledc_write(ledc_channel_t channel, uint32_t duty)
{
    ESP_ERROR_CHECK_WITHOUT_ABORT(ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, duty));
    ESP_ERROR_CHECK_WITHOUT_ABORT(ledc_update_duty(LEDC_LOW_SPEED_MODE, channel));
}
