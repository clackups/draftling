/*
 * Linux: X11 window through XCB.
 *
 * The frame goes to the window with PutImage (ZPixmap, depth 24 or 32
 * TrueColor, 32 bits per pixel -- every current X server), in bands
 * that fit the server's maximum request length. X11 key codes are
 * Linux key codes plus 8. The server's auto-repeat is made
 * "detectable" through XKB (no fake releases), and repeated presses
 * are dropped by the keyboard layer, since the editor repeats keys
 * itself.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <xcb/xcb.h>
#include <xcb/xkb.h>

#include "esp_log.h"
#include "linux_host.h"
#include "sdkconfig.h"

static const char *TAG = "X11";

static xcb_connection_t *s_conn;
static xcb_screen_t *s_screen;
static xcb_window_t s_win;
static xcb_gcontext_t s_gc;
static xcb_atom_t s_wm_protocols, s_wm_delete;
static uint8_t s_depth;
static int s_w, s_h;
static bool s_btn;
static uint32_t s_max_req;   /* bytes per request */

static xcb_atom_t intern(const char *name)
{
    xcb_intern_atom_reply_t *r =
        xcb_intern_atom_reply(s_conn, xcb_intern_atom(s_conn, 0, (uint16_t)strlen(name), name), NULL);
    xcb_atom_t a = r ? r->atom : XCB_ATOM_NONE;
    free(r);
    return a;
}

/* A 24- or 32-bit TrueColor visual of the root depth with 32 bits per
 * pixel in ZPixmap: then the frame's 0x00RRGGBB words go out as they
 * are (little-endian clients; XCB tells the server the byte order). */
static bool check_format(void)
{
    s_depth = s_screen->root_depth;
    if (s_depth != 24 && s_depth != 32) return false;
    const xcb_setup_t *setup = xcb_get_setup(s_conn);
    xcb_format_iterator_t fi = xcb_setup_pixmap_formats_iterator(setup);
    for (; fi.rem; xcb_format_next(&fi)) {
        if (fi.data->depth == s_depth) return fi.data->bits_per_pixel == 32;
    }
    return false;
}

static bool x11_open(int *w, int *h)
{
    int screen_no = 0;
    s_conn = xcb_connect(NULL, &screen_no);
    if (!s_conn || xcb_connection_has_error(s_conn)) {
        ESP_LOGI(TAG, "No X display (DISPLAY not set or not reachable)");
        if (s_conn) xcb_disconnect(s_conn);
        s_conn = NULL;
        return false;
    }
    xcb_screen_iterator_t it = xcb_setup_roots_iterator(xcb_get_setup(s_conn));
    for (int i = 0; i < screen_no && it.rem; i++) xcb_screen_next(&it);
    s_screen = it.data;
    if (!s_screen || !check_format()) {
        ESP_LOGW(TAG, "Needs a 24-bit TrueColor screen");
        xcb_disconnect(s_conn);
        s_conn = NULL;
        return false;
    }
    s_max_req = xcb_get_maximum_request_length(s_conn) * 4u;

    s_w = linux_window_w ? linux_window_w : CONFIG_DRAFTLING_DISPLAY_WIDTH;
    s_h = linux_window_h ? linux_window_h : CONFIG_DRAFTLING_DISPLAY_HEIGHT;
    s_win = xcb_generate_id(s_conn);
    uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t vals[2] = {
        s_screen->black_pixel,
        XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY |
            XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
            XCB_EVENT_MASK_FOCUS_CHANGE |
#if defined(CONFIG_DRAFTLING_TOUCHSCREEN)
            XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
            XCB_EVENT_MASK_POINTER_MOTION |
#endif
            0,
    };
    xcb_create_window(s_conn, XCB_COPY_FROM_PARENT, s_win, s_screen->root, 0, 0,
                      (uint16_t)s_w, (uint16_t)s_h, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      s_screen->root_visual, mask, vals);

    xcb_change_property(s_conn, XCB_PROP_MODE_REPLACE, s_win, XCB_ATOM_WM_NAME,
                        XCB_ATOM_STRING, 8, (uint32_t)strlen(LINUX_WINDOW_TITLE),
                        LINUX_WINDOW_TITLE);
    xcb_atom_t net_name = intern("_NET_WM_NAME"), utf8 = intern("UTF8_STRING");
    if (net_name && utf8)
        xcb_change_property(s_conn, XCB_PROP_MODE_REPLACE, s_win, net_name, utf8, 8,
                            (uint32_t)strlen(LINUX_WINDOW_TITLE), LINUX_WINDOW_TITLE);
    static const char wm_class[] = "draftling\0Draftling";
    xcb_change_property(s_conn, XCB_PROP_MODE_REPLACE, s_win, XCB_ATOM_WM_CLASS,
                        XCB_ATOM_STRING, 8, sizeof(wm_class), wm_class);
    /* WM_NORMAL_HINTS: minimum size 320x240 (PMinSize). */
    uint32_t hints[18] = { 0 };
    hints[0] = 1u << 4;
    hints[5] = 320;
    hints[6] = 240;
    xcb_change_property(s_conn, XCB_PROP_MODE_REPLACE, s_win, XCB_ATOM_WM_NORMAL_HINTS,
                        XCB_ATOM_WM_SIZE_HINTS, 32, 18, hints);
    s_wm_protocols = intern("WM_PROTOCOLS");
    s_wm_delete = intern("WM_DELETE_WINDOW");
    if (s_wm_protocols && s_wm_delete)
        xcb_change_property(s_conn, XCB_PROP_MODE_REPLACE, s_win, s_wm_protocols,
                            XCB_ATOM_ATOM, 32, 1, &s_wm_delete);

    s_gc = xcb_generate_id(s_conn);
    uint32_t gcv = 0;
    xcb_create_gc(s_conn, s_gc, s_win, XCB_GC_GRAPHICS_EXPOSURES, &gcv);

    /* Detectable auto-repeat: held keys send presses only. */
    xcb_xkb_use_extension_reply_t *xr =
        xcb_xkb_use_extension_reply(s_conn, xcb_xkb_use_extension(s_conn, 1, 0), NULL);
    if (xr && xr->supported) {
        uint32_t f = XCB_XKB_PER_CLIENT_FLAG_DETECTABLE_AUTO_REPEAT;
        free(xcb_xkb_per_client_flags_reply(
            s_conn, xcb_xkb_per_client_flags(s_conn, XCB_XKB_ID_USE_CORE_KBD, f, f, 0, 0, 0),
            NULL));
    }
    free(xr);

    xcb_map_window(s_conn, s_win);
    xcb_flush(s_conn);
    *w = s_w;
    *h = s_h;
    return true;
}

/* Core-protocol modifier state (the modifiers held before this event)
 * as LINUX_MOD_*. Mod1 is Alt and Mod4 Super on every common keymap;
 * Right Alt is Mod5 (ISO_Level3_Shift) on AltGr keymaps. */
static unsigned x11_mods(uint16_t state)
{
    unsigned m = 0;
    if (state & XCB_MOD_MASK_SHIFT)   m |= LINUX_MOD_SHIFT;
    if (state & XCB_MOD_MASK_CONTROL) m |= LINUX_MOD_CTRL;
    if (state & XCB_MOD_MASK_1)       m |= LINUX_MOD_ALT;
    if (state & XCB_MOD_MASK_5)       m |= LINUX_MOD_ALTGR;
    if (state & XCB_MOD_MASK_4)       m |= LINUX_MOD_SUPER;
    return m;
}

static int x11_fd(void)
{
    return s_conn ? xcb_get_file_descriptor(s_conn) : -1;
}

static void x11_dispatch(void)
{
    if (!s_conn) return;
    xcb_generic_event_t *ev;
    while ((ev = xcb_poll_for_event(s_conn)) != NULL) {
        switch (ev->response_type & 0x7f) {
        case XCB_EXPOSE:
            linux_io_damage_all();
            break;
        case XCB_CONFIGURE_NOTIFY: {
            xcb_configure_notify_event_t *c = (xcb_configure_notify_event_t *)ev;
            if (c->window == s_win && (c->width != s_w || c->height != s_h)) {
                s_w = c->width;
                s_h = c->height;
                linux_io_resize(s_w, s_h);
            }
            break;
        }
        case XCB_KEY_PRESS: {
            xcb_key_press_event_t *k = (xcb_key_press_event_t *)ev;
            linux_io_sync_modifiers(x11_mods(k->state));
            linux_io_key((int)k->detail - 8, true);
            break;
        }
        case XCB_KEY_RELEASE: {
            xcb_key_release_event_t *k = (xcb_key_release_event_t *)ev;
            linux_io_key((int)k->detail - 8, false);
            break;
        }
        case XCB_FOCUS_IN:
        case XCB_FOCUS_OUT:
            /* Keys held across a focus change: their releases went
             * elsewhere. */
            linux_io_release_all_keys();
            break;
        case XCB_BUTTON_PRESS:
        case XCB_BUTTON_RELEASE: {
            xcb_button_press_event_t *b = (xcb_button_press_event_t *)ev;
            bool press = (ev->response_type & 0x7f) == XCB_BUTTON_PRESS;
            if (b->detail == 1) {
                s_btn = press;
                linux_io_pointer(b->event_x, b->event_y, s_btn);
            } else if (press && (b->detail == 4 || b->detail == 5)) {
                linux_io_wheel(b->detail == 5 ? 1 : -1);
            }
            break;
        }
        case XCB_MOTION_NOTIFY: {
            xcb_motion_notify_event_t *m = (xcb_motion_notify_event_t *)ev;
            linux_io_pointer(m->event_x, m->event_y, s_btn);
            break;
        }
        case XCB_CLIENT_MESSAGE: {
            xcb_client_message_event_t *m = (xcb_client_message_event_t *)ev;
            if (m->type == s_wm_protocols && m->data.data32[0] == s_wm_delete) linux_io_close();
            break;
        }
        default:
            break;
        }
        free(ev);
    }
    if (xcb_connection_has_error(s_conn)) {
        ESP_LOGE(TAG, "Connection to the X server lost");
        linux_io_close();
    }
}

static void x11_present(const uint32_t *fb, int fb_w, int fb_h, int x, int y, int w, int h)
{
    (void)fb_h;
    if (!s_conn || w <= 0 || h <= 0) return;
    /* Rows per request: the request head is 24 bytes. */
    uint32_t row_bytes = (uint32_t)w * 4;
    int rows = (int)((s_max_req - 64) / row_bytes);
    if (rows < 1) rows = 1;
    uint32_t *band = (uint32_t *)malloc((size_t)row_bytes * (size_t)(rows < h ? rows : h));
    if (!band) return;
    for (int y0 = 0; y0 < h; y0 += rows) {
        int n = h - y0 < rows ? h - y0 : rows;
        for (int r = 0; r < n; r++)
            memcpy(band + (size_t)r * w, fb + (size_t)(y + y0 + r) * fb_w + x, row_bytes);
        xcb_put_image(s_conn, XCB_IMAGE_FORMAT_Z_PIXMAP, s_win, s_gc, (uint16_t)w, (uint16_t)n,
                      (int16_t)x, (int16_t)(y + y0), 0, s_depth, row_bytes * (uint32_t)n,
                      (const uint8_t *)band);
    }
    free(band);
}

static void x11_flush(void)
{
    if (s_conn) xcb_flush(s_conn);
}

static void x11_close(void)
{
    if (!s_conn) return;
    xcb_free_gc(s_conn, s_gc);
    xcb_destroy_window(s_conn, s_win);
    xcb_flush(s_conn);
    xcb_disconnect(s_conn);
    s_conn = NULL;
}

const linux_io_backend_t linux_io_x11 = {
    .name = "x11",
    .open = x11_open,
    .fd = x11_fd,
    .dispatch = x11_dispatch,
    .present = x11_present,
    .flush = x11_flush,
    .close = x11_close,
    .repeats_keys = true,
};
