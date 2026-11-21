/*
 * Linux: standby (CONFIG_DRAFTLING_STANDBY_DISPLAY_OFF).
 *
 * As on a device whose standby keeps running: the inactivity timeout
 * (F1 -> Settings -> System -> Standby timeout, the same NVS setting),
 * Ctrl+P and F1 -> "Sleep now" run the pre-sleep hook (save every
 * document) and turn the screen off -- here the window goes black
 * (display_sleep()). The next key, click, wheel turn or pointer
 * movement over the window turns it on again (display_wake()); that
 * input only wakes, it is not passed on. The editor keeps its state.
 *
 * F1 -> "Quit Draftling" (CONFIG_DRAFTLING_HAS_QUIT) ends the program
 * through standby_quit(), as do closing the window and SIGTERM.
 */

#include <stdbool.h>
#include <stddef.h>

#include "display.h"
#include "esp_log.h"
#include "linux_host.h"
#include "lvgl.h"
#include "nvs.h"
#include "standby.h"

static const char *TAG = "Standby";

#define NVS_NAMESPACE "standby"
#define NVS_KEY_TOUT  "timeout"
#define CHECK_MS      1000

static uint32_t s_timeout_sec = STANDBY_DEFAULT_TIMEOUT_SEC;
static uint32_t s_last_activity;
static bool s_inhibit, s_idle_inhibit;
static bool s_asleep;

/* main_linux.cpp's linux_quit() calls the hook. */
standby_pre_sleep_cb_t linux_pre_sleep_cb;

static void check_cb(lv_timer_t *t)
{
    (void)t;
    if (s_asleep || s_timeout_sec == 0 || s_inhibit || s_idle_inhibit) return;
    if (lv_tick_elaps(s_last_activity) >= s_timeout_sec * 1000u) {
        ESP_LOGI(TAG, "Inactivity timeout reached: turning the screen off");
        standby_enter_sleep();
    }
}

void standby_init(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &h) == ESP_OK) {
        uint32_t val = 0;
        if (nvs_get_u32(h, NVS_KEY_TOUT, &val) == ESP_OK) s_timeout_sec = val;
        nvs_close(h);
    }
    s_last_activity = lv_tick_get();
    lv_timer_create(check_cb, CHECK_MS, NULL);
}

void standby_reset_timer(void)
{
    s_last_activity = lv_tick_get();
}

uint32_t standby_get_timeout(void)
{
    return s_timeout_sec;
}

void standby_set_timeout(uint32_t seconds)
{
    s_timeout_sec = seconds;
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_u32(h, NVS_KEY_TOUT, seconds);
        nvs_commit(h);
        nvs_close(h);
    }
    standby_reset_timer();
}

void standby_set_pre_sleep_cb(standby_pre_sleep_cb_t cb)
{
    linux_pre_sleep_cb = cb;
}

void standby_set_inhibit(bool inhibit)
{
    s_inhibit = inhibit;
    if (!inhibit) standby_reset_timer();
}

void standby_set_idle_inhibit(bool inhibit)
{
    s_idle_inhibit = inhibit;
    if (!inhibit) standby_reset_timer();
}

/* Save, then turn the screen off; returns at once (the program keeps
 * running, as with display-off standby on a device). */
void standby_enter_sleep(void)
{
    if (s_inhibit || s_asleep) return;
    if (linux_pre_sleep_cb) linux_pre_sleep_cb();
    s_asleep = true;
    display_sleep();
}

void standby_quit(void)
{
    linux_quit(0);
}

bool linux_standby_wake(void)
{
    standby_reset_timer();
    if (!s_asleep) return false;
    ESP_LOGI(TAG, "Input: turning the screen on");
    s_asleep = false;
    display_wake();
    return true;
}
