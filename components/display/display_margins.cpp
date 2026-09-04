#include "display_margins.h"

#include <nvs.h>
#include <nvs_flash.h>

static const char *NVS_NS         = "dispmargin";
static const char *NVS_KEY_LEFT   = "left";
static const char *NVS_KEY_RIGHT  = "right";
static const char *NVS_KEY_TOP    = "top";
static const char *NVS_KEY_BOTTOM = "bottom";

/* Frozen for the life of the session -- see the "why frozen" comment
 * in display_margins.h. Set once by display_margins_init(). */
static int s_left = 0, s_right = 0, s_top = 0, s_bottom = 0;

static int read_u8(nvs_handle_t h, const char *key)
{
    uint8_t v = 0;
    if (nvs_get_u8(h, key, &v) == ESP_OK) return v;
    return 0;
}

extern "C" void display_margins_init(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) == ESP_OK) {
        s_left   = read_u8(h, NVS_KEY_LEFT);
        s_right  = read_u8(h, NVS_KEY_RIGHT);
        s_top    = read_u8(h, NVS_KEY_TOP);
        s_bottom = read_u8(h, NVS_KEY_BOTTOM);
        nvs_close(h);
    }
}

extern "C" int display_margin_left(void)   { return s_left; }
extern "C" int display_margin_right(void)  { return s_right; }
extern "C" int display_margin_top(void)    { return s_top; }
extern "C" int display_margin_bottom(void) { return s_bottom; }

extern "C" void display_margins_set(int left, int right, int top, int bottom)
{
    /* Deliberately does NOT touch s_left/s_right/s_top/s_bottom --
     * see the file comment in display_margins.h. Only the NEXT boot's
     * display_margins_init() picks this up. */
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_u8(h, NVS_KEY_LEFT,   (uint8_t)left);
        nvs_set_u8(h, NVS_KEY_RIGHT,  (uint8_t)right);
        nvs_set_u8(h, NVS_KEY_TOP,    (uint8_t)top);
        nvs_set_u8(h, NVS_KEY_BOTTOM, (uint8_t)bottom);
        nvs_commit(h);
        nvs_close(h);
    }
}

extern "C" void display_margins_get_pending(int *left, int *right, int *top, int *bottom)
{
    int l = 0, r = 0, t = 0, b = 0;
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) == ESP_OK) {
        l = read_u8(h, NVS_KEY_LEFT);
        r = read_u8(h, NVS_KEY_RIGHT);
        t = read_u8(h, NVS_KEY_TOP);
        b = read_u8(h, NVS_KEY_BOTTOM);
        nvs_close(h);
    }
    if (left)   *left   = l;
    if (right)  *right  = r;
    if (top)    *top    = t;
    if (bottom) *bottom = b;
}
