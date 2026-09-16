#include "display_flip.h"

#include <nvs.h>
#include <nvs_flash.h>

static const char *NVS_NS      = "dispflip";
static const char *NVS_KEY_UPS = "updown";

/* Frozen for the life of the session -- see the "why frozen" comment
 * in display_flip.h. Set once by display_flip_init(). */
static bool s_upside_down = false;

static bool read_pending(void)
{
    bool v = false;
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) == ESP_OK) {
        uint8_t u = 0;
        if (nvs_get_u8(h, NVS_KEY_UPS, &u) == ESP_OK) v = (u != 0);
        nvs_close(h);
    }
    return v;
}

extern "C" void display_flip_init(void)
{
    s_upside_down = read_pending();
}

extern "C" bool display_flip_is_upside_down(void)
{
    return s_upside_down;
}

extern "C" void display_flip_set_upside_down(bool upside_down)
{
    /* Deliberately does NOT touch s_upside_down -- only the NEXT
     * boot's display_flip_init() picks this up (see display_flip.h). */
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_u8(h, NVS_KEY_UPS, upside_down ? 1 : 0);
        nvs_commit(h);
        nvs_close(h);
    }
}

extern "C" bool display_flip_get_pending_upside_down(void)
{
    return read_pending();
}
