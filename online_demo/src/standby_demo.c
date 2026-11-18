/*
 * Online demo: standby. The inactivity timeout works as on the device
 * (same NVS setting): the pre-sleep hook saves the open documents and
 * the page shows the device asleep; waking it reloads the page, as the
 * board restarts on wake. The no-keyboard timer is not needed: the
 * browser keyboard is always there.
 */

#include <stdbool.h>

#include "demo_host.h"
#include "esp_log.h"
#include "lvgl.h"
#include "nvs.h"
#include "standby.h"

static const char *TAG = "StandbyDemo";

#define NVS_NAMESPACE "standby"
#define NVS_KEY_TOUT  "timeout"

static uint32_t s_timeout_sec = STANDBY_DEFAULT_TIMEOUT_SEC;
static uint32_t s_last_activity;
static bool s_inhibit, s_idle_inhibit;
static standby_pre_sleep_cb_t s_pre_sleep_cb;

static void check_cb(lv_timer_t *t)
{
    (void)t;
    if (s_timeout_sec == 0 || s_inhibit || s_idle_inhibit) return;
    if (lv_tick_elaps(s_last_activity) >= s_timeout_sec * 1000u) {
        ESP_LOGI(TAG, "Inactivity timeout reached -- entering deep sleep");
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
    lv_timer_create(check_cb, 1000, NULL);
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
    s_pre_sleep_cb = cb;
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

void standby_enter_sleep(void)
{
    if (s_inhibit) return;
    if (s_pre_sleep_cb) s_pre_sleep_cb();
    demo_host_sleep();
    demo_halt();
}
