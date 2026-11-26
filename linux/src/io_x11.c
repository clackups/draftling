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
 *
 * The desktop clipboard is the CLIPBOARD selection (ICCCM). A copy
 * makes the window its owner, and SelectionRequest events are answered
 * with TARGETS, TIMESTAMP and the text types; texts larger than one
 * request go out incrementally (INCR, carried on by PropertyNotify).
 * A paste converts the selection to UTF8_STRING (STRING if refused)
 * into a property of our window and reads it, INCR included.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
static xcb_timestamp_t s_last_time;   /* of the last input event */

/* ---- clipboard state ---- */
#define CLIP_N_INCR   4                    /* incremental deliveries at once */
#define CLIP_INCR_TTL 30                   /* s before a stalled one is dropped */
#define CLIP_MAX_RX   (256u * 1024 * 1024) /* largest text accepted */

static xcb_atom_t s_a_clipboard, s_a_targets, s_a_timestamp, s_a_utf8, s_a_text,
    s_a_mime_utf8, s_a_mime_plain, s_a_incr, s_a_prop;

static bool s_own;              /* we own CLIPBOARD */
static char *s_clip;            /* its text (UTF-8) */
static size_t s_clip_len;
static xcb_timestamp_t s_own_time;

typedef struct {
    xcb_window_t win;           /* 0: free */
    xcb_atom_t prop, type;
    char *data;
    size_t len, off;
    time_t start;
} clip_incr_t;
static clip_incr_t s_incr[CLIP_N_INCR];

typedef enum { RX_IDLE, RX_WAIT_UTF8, RX_WAIT_STRING, RX_INCR } rx_state_t;
static rx_state_t s_rx_state;
static char *s_rx;
static size_t s_rx_len;

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
        if (linux_io_explicit)
            ESP_LOGW(TAG, "No X display (DISPLAY not set or not reachable)");
        else
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
            XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_PROPERTY_CHANGE |
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

    s_a_clipboard = intern("CLIPBOARD");
    s_a_targets = intern("TARGETS");
    s_a_timestamp = intern("TIMESTAMP");
    s_a_utf8 = utf8;
    s_a_text = intern("TEXT");
    s_a_mime_utf8 = intern("text/plain;charset=utf-8");
    s_a_mime_plain = intern("text/plain");
    s_a_incr = intern("INCR");
    s_a_prop = intern("DRAFTLING_CLIPBOARD");

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

/* ---- clipboard ---------------------------------------------------------------- */

/* Most bytes of text in one ChangeProperty (the request head is 24
 * bytes); larger texts go out incrementally. */
static uint32_t clip_chunk(void)
{
    uint32_t c = s_max_req > 1024 ? s_max_req - 64 : 1024;
    return c > 256 * 1024 ? 256 * 1024 : c;
}

static void clip_free_own(void)
{
    free(s_clip);
    s_clip = NULL;
    s_clip_len = 0;
    s_own = false;
}

static void incr_end(clip_incr_t *t)
{
    if (!t->win) return;
    bool shared = false;   /* another delivery to the same window */
    for (int i = 0; i < CLIP_N_INCR; i++)
        if (&s_incr[i] != t && s_incr[i].win == t->win) shared = true;
    if (t->win != s_win && !shared) {
        uint32_t none = 0;
        xcb_change_window_attributes(s_conn, t->win, XCB_CW_EVENT_MASK, &none);
    }
    free(t->data);
    memset(t, 0, sizeof(*t));
}

static void x11_clip_set(const char *text, size_t len)
{
    if (!s_conn || !s_a_clipboard) return;
    if (!text) {
        if (s_own) xcb_set_selection_owner(s_conn, XCB_NONE, s_a_clipboard, s_last_time);
        clip_free_own();
        xcb_flush(s_conn);
        return;
    }
    char *copy = (char *)malloc(len + 1);
    if (!copy) return;
    memcpy(copy, text, len);
    copy[len] = '\0';
    clip_free_own();
    xcb_set_selection_owner(s_conn, s_win, s_a_clipboard, s_last_time);
    xcb_get_selection_owner_reply_t *r =
        xcb_get_selection_owner_reply(s_conn, xcb_get_selection_owner(s_conn, s_a_clipboard), NULL);
    bool ok = r && r->owner == s_win;
    free(r);
    if (!ok) {
        ESP_LOGW(TAG, "Could not take the clipboard over");
        free(copy);
        return;
    }
    s_clip = copy;
    s_clip_len = len;
    s_own = true;
    s_own_time = s_last_time;
}

static bool is_text_target(xcb_atom_t a)
{
    return a && (a == s_a_utf8 || a == XCB_ATOM_STRING || a == s_a_text ||
                 a == s_a_mime_utf8 || a == s_a_mime_plain);
}

/* Put the text into the requestor's property: in one piece, or
 * announce an INCR transfer that PropertyNotify carries on. */
static bool clip_deliver(xcb_window_t win, xcb_atom_t prop, xcb_atom_t target)
{
    xcb_atom_t type = target == s_a_text ? s_a_utf8 : target;
    char *data;
    size_t len;
    if (target == XCB_ATOM_STRING) {
        data = linux_clip_to_latin1(s_clip, s_clip_len, &len);
    } else {
        data = (char *)malloc(s_clip_len + 1);
        if (data) memcpy(data, s_clip, s_clip_len);
        len = s_clip_len;
    }
    if (!data) return false;
    if (len <= clip_chunk()) {
        xcb_change_property(s_conn, XCB_PROP_MODE_REPLACE, win, prop, type, 8, (uint32_t)len,
                            data);
        free(data);
        return true;
    }
    time_t now = time(NULL);
    clip_incr_t *t = NULL;
    for (int i = 0; i < CLIP_N_INCR; i++) {
        if (s_incr[i].win && now - s_incr[i].start > CLIP_INCR_TTL) incr_end(&s_incr[i]);
        if (!s_incr[i].win && !t) t = &s_incr[i];
    }
    if (!t) {
        free(data);
        return false;
    }
    *t = (clip_incr_t){ .win = win, .prop = prop, .type = type, .data = data, .len = len,
                        .start = now };
    uint32_t mask = XCB_EVENT_MASK_PROPERTY_CHANGE;
    xcb_change_window_attributes(s_conn, win, XCB_CW_EVENT_MASK, &mask);
    uint32_t size = len > UINT32_MAX ? UINT32_MAX : (uint32_t)len;
    xcb_change_property(s_conn, XCB_PROP_MODE_REPLACE, win, prop, s_a_incr, 32, 1, &size);
    return true;
}

static void sel_request(const xcb_selection_request_event_t *r)
{
    /* Obsolete clients leave the property out: use the target. */
    xcb_atom_t prop = r->property ? r->property : r->target;
    bool ok = false;
    if (r->selection == s_a_clipboard && s_own && s_clip) {
        if (r->target == s_a_targets) {
            xcb_atom_t t[8];
            int n = 0;
            t[n++] = s_a_targets;
            if (s_a_timestamp) t[n++] = s_a_timestamp;
            if (s_a_utf8) t[n++] = s_a_utf8;
            t[n++] = XCB_ATOM_STRING;
            if (s_a_text) t[n++] = s_a_text;
            if (s_a_mime_utf8) t[n++] = s_a_mime_utf8;
            if (s_a_mime_plain) t[n++] = s_a_mime_plain;
            xcb_change_property(s_conn, XCB_PROP_MODE_REPLACE, r->requestor, prop,
                                XCB_ATOM_ATOM, 32, (uint32_t)n, t);
            ok = true;
        } else if (r->target == s_a_timestamp && s_a_timestamp) {
            xcb_change_property(s_conn, XCB_PROP_MODE_REPLACE, r->requestor, prop,
                                XCB_ATOM_INTEGER, 32, 1, &s_own_time);
            ok = true;
        } else if (is_text_target(r->target)) {
            ok = clip_deliver(r->requestor, prop, r->target);
        }
    }
    char buf[32];
    memset(buf, 0, sizeof(buf));
    xcb_selection_notify_event_t *n = (xcb_selection_notify_event_t *)buf;
    n->response_type = XCB_SELECTION_NOTIFY;
    n->time = r->time;
    n->requestor = r->requestor;
    n->selection = r->selection;
    n->target = r->target;
    n->property = ok ? prop : XCB_NONE;
    xcb_send_event(s_conn, 0, r->requestor, XCB_EVENT_MASK_NO_EVENT, buf);
}

/* A requestor deleted the property of an INCR delivery: the next
 * chunk, or the empty one that ends it. */
static void incr_step(xcb_window_t win, xcb_atom_t prop)
{
    for (int i = 0; i < CLIP_N_INCR; i++) {
        clip_incr_t *t = &s_incr[i];
        if (t->win != win || t->prop != prop) continue;
        size_t n = t->len - t->off;
        if (n > clip_chunk()) n = clip_chunk();
        xcb_change_property(s_conn, XCB_PROP_MODE_REPLACE, win, prop, t->type, 8, (uint32_t)n,
                            t->data + t->off);
        t->off += n;
        if (n == 0) incr_end(t);
        return;
    }
}

static void rx_reset(void)
{
    free(s_rx);
    s_rx = NULL;
    s_rx_len = 0;
    s_rx_state = RX_IDLE;
}

static void rx_finish(bool ok)
{
    if (ok) linux_io_clipboard_text(s_rx ? s_rx : "", s_rx_len);
    else linux_io_clipboard_text(NULL, 0);
    rx_reset();
}

static bool rx_append(const void *p, size_t n)
{
    if (n == 0) return true;
    if (s_rx_len + n > CLIP_MAX_RX) return false;
    char *b = (char *)realloc(s_rx, s_rx_len + n + 1);
    if (!b) return false;
    memcpy(b + s_rx_len, p, n);
    s_rx = b;
    s_rx_len += n;
    return true;
}

/* Read (and delete) our property. Returns its type, XCB_NONE on error. */
static xcb_atom_t rx_read_prop(bool *ok)
{
    *ok = false;
    xcb_get_property_reply_t *r = xcb_get_property_reply(
        s_conn,
        xcb_get_property(s_conn, 1, s_win, s_a_prop, XCB_GET_PROPERTY_TYPE_ANY, 0,
                         CLIP_MAX_RX / 4),
        NULL);
    if (!r) return XCB_NONE;
    xcb_atom_t type = r->type;
    if (type != s_a_incr) {
        int n = xcb_get_property_value_length(r);
        *ok = n >= 0 && rx_append(xcb_get_property_value(r), (size_t)n);
    } else {
        *ok = true;
    }
    free(r);
    return type;
}

static void sel_notify(const xcb_selection_notify_event_t *n)
{
    if (n->requestor != s_win || n->selection != s_a_clipboard) return;
    if (s_rx_state != RX_WAIT_UTF8 && s_rx_state != RX_WAIT_STRING) return;
    if (n->property == XCB_NONE) {
        if (s_rx_state == RX_WAIT_UTF8) {
            s_rx_state = RX_WAIT_STRING;
            xcb_convert_selection(s_conn, s_win, s_a_clipboard, XCB_ATOM_STRING, s_a_prop,
                                  s_last_time);
            return;
        }
        rx_finish(false);
        return;
    }
    bool ok;
    xcb_atom_t type = rx_read_prop(&ok);
    if (!ok) rx_finish(false);
    else if (type == s_a_incr) s_rx_state = RX_INCR;   /* the delete started it */
    else rx_finish(true);
}

static void rx_incr_step(void)
{
    bool ok;
    size_t before = s_rx_len;
    rx_read_prop(&ok);
    if (!ok) rx_finish(false);
    else if (s_rx_len == before) rx_finish(true);   /* the empty chunk: done */
}

static bool x11_clip_request(void)
{
    if (!s_conn || !s_a_clipboard || !s_a_utf8 || !s_a_prop || s_own) return false;
    xcb_get_selection_owner_reply_t *r =
        xcb_get_selection_owner_reply(s_conn, xcb_get_selection_owner(s_conn, s_a_clipboard), NULL);
    xcb_window_t owner = r ? r->owner : XCB_NONE;
    free(r);
    if (owner == XCB_NONE || owner == s_win) return false;
    rx_reset();
    xcb_delete_property(s_conn, s_win, s_a_prop);
    s_rx_state = RX_WAIT_UTF8;
    xcb_convert_selection(s_conn, s_win, s_a_clipboard, s_a_utf8, s_a_prop, s_last_time);
    xcb_flush(s_conn);
    return true;
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
            s_last_time = k->time;
            linux_io_sync_modifiers(x11_mods(k->state));
            linux_io_key((int)k->detail - 8, true);
            break;
        }
        case XCB_KEY_RELEASE: {
            xcb_key_release_event_t *k = (xcb_key_release_event_t *)ev;
            s_last_time = k->time;
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
            s_last_time = b->time;
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
        case XCB_SELECTION_REQUEST:
            sel_request((xcb_selection_request_event_t *)ev);
            break;
        case XCB_SELECTION_NOTIFY:
            sel_notify((xcb_selection_notify_event_t *)ev);
            break;
        case XCB_SELECTION_CLEAR: {
            xcb_selection_clear_event_t *c = (xcb_selection_clear_event_t *)ev;
            if (c->selection == s_a_clipboard) clip_free_own();
            break;
        }
        case XCB_PROPERTY_NOTIFY: {
            xcb_property_notify_event_t *pn = (xcb_property_notify_event_t *)ev;
            if (pn->state == XCB_PROPERTY_DELETE) incr_step(pn->window, pn->atom);
            else if (pn->window == s_win && pn->atom == s_a_prop && s_rx_state == RX_INCR)
                rx_incr_step();
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
    for (int i = 0; i < CLIP_N_INCR; i++) incr_end(&s_incr[i]);
    clip_free_own();
    rx_reset();
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
    .clip_set = x11_clip_set,
    .clip_request = x11_clip_request,
};
