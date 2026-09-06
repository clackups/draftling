#include "display_orientation.h"

#include <nvs.h>
#include <nvs_flash.h>

static const char *NVS_NS       = "disporient";
static const char *NVS_KEY_PORT = "portrait";

/* Frozen for the life of the session -- see the "why frozen" comment
 * in display_orientation.h. Set once by display_orientation_init(). */
static bool s_portrait = false;

static bool read_pending(void)
{
    bool v = false;
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) == ESP_OK) {
        uint8_t u = 0;
        if (nvs_get_u8(h, NVS_KEY_PORT, &u) == ESP_OK) v = (u != 0);
        nvs_close(h);
    }
    return v;
}

extern "C" void display_orientation_init(void)
{
    s_portrait = read_pending();
}

extern "C" bool display_orientation_is_portrait(void)
{
    return s_portrait;
}

extern "C" void display_orientation_set_portrait(bool portrait)
{
    /* Deliberately does NOT touch s_portrait -- only the NEXT boot's
     * display_orientation_init() picks this up (see display_orientation.h). */
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_u8(h, NVS_KEY_PORT, portrait ? 1 : 0);
        nvs_commit(h);
        nvs_close(h);
    }
}

extern "C" bool display_orientation_get_pending_portrait(void)
{
    return read_pending();
}
