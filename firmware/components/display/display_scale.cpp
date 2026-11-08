#include "display_scale.h"

#include <nvs.h>
#include <nvs_flash.h>

static const char *NVS_NS     = "dispscale";
static const char *NVS_KEY_X2 = "x2";

/* Frozen for the life of the session -- see display_scale.h. */
static int s_factor = 1;

static bool read_pending(void)
{
    bool v = false;
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) == ESP_OK) {
        uint8_t u = 0;
        if (nvs_get_u8(h, NVS_KEY_X2, &u) == ESP_OK) v = (u != 0);
        nvs_close(h);
    }
    return v;
}

extern "C" void display_scale_init(void)
{
    s_factor = read_pending() ? 2 : 1;
}

extern "C" int display_scale_factor(void)
{
    return s_factor;
}

extern "C" void display_scale_set_2x(bool on)
{
    /* Deliberately does NOT touch s_factor: only the next boot's
     * display_scale_init() picks this up. */
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_u8(h, NVS_KEY_X2, on ? 1 : 0);
        nvs_commit(h);
        nvs_close(h);
    }
}

extern "C" bool display_scale_get_pending_2x(void)
{
    return read_pending();
}
