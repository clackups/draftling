/*
 * Linux: standby. On the device the inactivity timeout and "Sleep
 * now" (Ctrl+P, F1 -> Sleep now) put it into deep sleep after saving
 * every document. On a computer the program is not put to sleep on
 * its own: the inactivity timeout is kept as a setting but never
 * fires, and "Sleep now" saves and quits (linux_quit(), which runs
 * the pre-sleep hook).
 */

#include <stdbool.h>
#include <stddef.h>

#include "linux_host.h"
#include "nvs.h"
#include "standby.h"

#define NVS_NAMESPACE "standby"
#define NVS_KEY_TOUT  "timeout"

static uint32_t s_timeout_sec = STANDBY_DEFAULT_TIMEOUT_SEC;
static bool s_inhibit;

void standby_init(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &h) == ESP_OK) {
        uint32_t val = 0;
        if (nvs_get_u32(h, NVS_KEY_TOUT, &val) == ESP_OK) s_timeout_sec = val;
        nvs_close(h);
    }
}

void standby_reset_timer(void) { }

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
}

/* main_linux.cpp's linux_quit() calls the hook. */
standby_pre_sleep_cb_t linux_pre_sleep_cb;

void standby_set_pre_sleep_cb(standby_pre_sleep_cb_t cb)
{
    linux_pre_sleep_cb = cb;
}

void standby_set_inhibit(bool inhibit)
{
    s_inhibit = inhibit;
}

void standby_set_idle_inhibit(bool inhibit)
{
    (void)inhibit;
}

void standby_enter_sleep(void)
{
    if (s_inhibit) return;
    linux_quit(0);
}
