/*
 * Linux: Wayland window (xdg-shell) with shared-memory buffers.
 *
 * The frame is copied into a free wl_buffer (two are kept; the
 * compositor holds one while it shows it) and the changed rectangle is
 * reported as damage. Keys arrive as Linux key codes (wl_keyboard's
 * key event); the compositor's key map is not used -- the editor has
 * its own layouts. The window asks for server-side decorations
 * (xdg-decoration) where the compositor offers them; elsewhere (GNOME)
 * it has none.
 */


#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <linux/input-event-codes.h>
#include <wayland-client.h>

#include "esp_log.h"
#include "linux_host.h"
#include "sdkconfig.h"
#include "xdg-decoration-client-protocol.h"
#include "xdg-shell-client-protocol.h"

static const char *TAG = "Wayland";

#define N_BUFFERS 2
/* Wheel: one notch is 10 units of wl_pointer.axis. */
#define AXIS_STEP 10.0

typedef struct {
    struct wl_buffer *buf;
    uint32_t *data;
    size_t size;
    int w, h;
    bool busy;
} shm_buf_t;

static struct wl_display *s_dpy;
static struct wl_registry *s_reg;
static struct wl_compositor *s_comp;
static struct wl_shm *s_shm;
static struct wl_seat *s_seat;
static struct xdg_wm_base *s_wm;
static struct zxdg_decoration_manager_v1 *s_deco_mgr;
static struct wl_surface *s_surf;
static struct xdg_surface *s_xsurf;
static struct xdg_toplevel *s_top;
static struct zxdg_toplevel_decoration_v1 *s_deco;
static struct wl_keyboard *s_kbd;
static struct wl_pointer *s_ptr;
static shm_buf_t s_bufs[N_BUFFERS];

static bool s_configured;
static int s_cfg_w, s_cfg_h;   /* from the last toplevel configure */
static int s_w, s_h;           /* window size in use */
static double s_px, s_py;
static bool s_btn;
static double s_axis;
static bool s_want_redraw;     /* a frame was dropped: no free buffer */

/* ---- shared memory ---------------------------------------------------------- */

static int shm_file(size_t size)
{
    int fd;
#if defined(__linux__)
    fd = memfd_create("draftling", MFD_CLOEXEC);
#else
    char name[64];
    snprintf(name, sizeof(name), "/draftling-%d-%p", (int)getpid(), (void *)&fd);
    fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (fd >= 0) shm_unlink(name);
#endif
    if (fd < 0) return -1;
    if (ftruncate(fd, (off_t)size) != 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static void buffer_release(void *data, struct wl_buffer *b)
{
    (void)b;
    shm_buf_t *sb = (shm_buf_t *)data;
    sb->busy = false;
    if (s_want_redraw) {
        s_want_redraw = false;
        linux_io_damage_all();
    }
}

static const struct wl_buffer_listener s_buffer_listener = { buffer_release };

static void buffer_free(shm_buf_t *b)
{
    if (b->buf) wl_buffer_destroy(b->buf);
    if (b->data) munmap(b->data, b->size);
    memset(b, 0, sizeof(*b));
}

static bool buffer_alloc(shm_buf_t *b, int w, int h)
{
    buffer_free(b);
    size_t stride = (size_t)w * 4, size = stride * (size_t)h;
    int fd = shm_file(size);
    if (fd < 0) {
        ESP_LOGE(TAG, "Cannot create shared memory: %s", strerror(errno));
        return false;
    }
    void *p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) {
        close(fd);
        return false;
    }
    struct wl_shm_pool *pool = wl_shm_create_pool(s_shm, fd, (int32_t)size);
    b->buf = wl_shm_pool_create_buffer(pool, 0, w, h, (int32_t)stride, WL_SHM_FORMAT_XRGB8888);
    wl_shm_pool_destroy(pool);
    close(fd);
    b->data = (uint32_t *)p;
    b->size = size;
    b->w = w;
    b->h = h;
    b->busy = false;
    wl_buffer_add_listener(b->buf, &s_buffer_listener, b);
    return true;
}

/* ---- xdg-shell ---------------------------------------------------------------- */

static void wm_ping(void *data, struct xdg_wm_base *wm, uint32_t serial)
{
    (void)data;
    xdg_wm_base_pong(wm, serial);
}

static const struct xdg_wm_base_listener s_wm_listener = { wm_ping };

static void xsurf_configure(void *data, struct xdg_surface *xs, uint32_t serial)
{
    (void)data;
    xdg_surface_ack_configure(xs, serial);
    if (s_cfg_w > 0 && s_cfg_h > 0 && (s_cfg_w != s_w || s_cfg_h != s_h)) {
        s_w = s_cfg_w;
        s_h = s_cfg_h;
        if (s_configured) linux_io_resize(s_w, s_h);
    }
    if (s_configured) linux_io_damage_all();
    s_configured = true;
}

static const struct xdg_surface_listener s_xsurf_listener = { xsurf_configure };

static void top_configure(void *data, struct xdg_toplevel *t, int32_t w, int32_t h,
                          struct wl_array *states)
{
    (void)data; (void)t; (void)states;
    s_cfg_w = w;
    s_cfg_h = h;
}

static void top_close(void *data, struct xdg_toplevel *t)
{
    (void)data; (void)t;
    linux_io_close();
}

static void top_configure_bounds(void *data, struct xdg_toplevel *t, int32_t w, int32_t h)
{
    (void)data; (void)t; (void)w; (void)h;
}

static void top_wm_capabilities(void *data, struct xdg_toplevel *t, struct wl_array *caps)
{
    (void)data; (void)t; (void)caps;
}

static const struct xdg_toplevel_listener s_top_listener = {
    .configure = top_configure,
    .close = top_close,
    .configure_bounds = top_configure_bounds,
    .wm_capabilities = top_wm_capabilities,
};

/* ---- keyboard --------------------------------------------------------------- */

static void kb_keymap(void *data, struct wl_keyboard *k, uint32_t fmt, int32_t fd, uint32_t size)
{
    (void)data; (void)k; (void)fmt; (void)size;
    close(fd);
}

static void kb_enter(void *data, struct wl_keyboard *k, uint32_t serial,
                     struct wl_surface *s, struct wl_array *keys)
{
    (void)data; (void)k; (void)serial; (void)s; (void)keys;
    linux_io_release_all_keys();
}

static void kb_leave(void *data, struct wl_keyboard *k, uint32_t serial, struct wl_surface *s)
{
    (void)data; (void)k; (void)serial; (void)s;
    linux_io_release_all_keys();
}

static void kb_key(void *data, struct wl_keyboard *k, uint32_t serial, uint32_t time,
                   uint32_t key, uint32_t state)
{
    (void)data; (void)k; (void)serial; (void)time;
    linux_io_key((int)key, state == WL_KEYBOARD_KEY_STATE_PRESSED);
}

/* The depressed modifiers, by the modifier indices of the standard
 * xkb keymaps compositors use (Shift 0, Lock 1, Control 2, Mod1 3,
 * Mod2 4, Mod3 5, Mod4 6, Mod5 7); the compositor's key map itself is
 * not parsed. */
static void kb_modifiers(void *data, struct wl_keyboard *k, uint32_t serial, uint32_t dep,
                         uint32_t lat, uint32_t lock, uint32_t group)
{
    (void)data; (void)k; (void)serial; (void)lat; (void)lock; (void)group;
    unsigned m = 0;
    if (dep & (1u << 0)) m |= LINUX_MOD_SHIFT;
    if (dep & (1u << 2)) m |= LINUX_MOD_CTRL;
    if (dep & (1u << 3)) m |= LINUX_MOD_ALT;
    if (dep & (1u << 7)) m |= LINUX_MOD_ALTGR;
    if (dep & (1u << 6)) m |= LINUX_MOD_SUPER;
    linux_io_sync_modifiers(m);
}

static void kb_repeat(void *data, struct wl_keyboard *k, int32_t rate, int32_t delay)
{
    (void)data; (void)k; (void)rate; (void)delay;
}

static const struct wl_keyboard_listener s_kb_listener = {
    kb_keymap, kb_enter, kb_leave, kb_key, kb_modifiers, kb_repeat,
};

/* ---- pointer ------------------------------------------------------------------ */

static void ptr_enter(void *data, struct wl_pointer *p, uint32_t serial, struct wl_surface *s,
                      wl_fixed_t x, wl_fixed_t y)
{
    (void)data; (void)p; (void)serial; (void)s;
    s_px = wl_fixed_to_double(x);
    s_py = wl_fixed_to_double(y);
    linux_io_pointer((int)s_px, (int)s_py, s_btn);
}

static void ptr_leave(void *data, struct wl_pointer *p, uint32_t serial, struct wl_surface *s)
{
    (void)data; (void)p; (void)serial; (void)s;
    s_btn = false;
    linux_io_pointer((int)s_px, (int)s_py, false);
}

static void ptr_motion(void *data, struct wl_pointer *p, uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
    (void)data; (void)p; (void)time;
    s_px = wl_fixed_to_double(x);
    s_py = wl_fixed_to_double(y);
    linux_io_pointer((int)s_px, (int)s_py, s_btn);
}

static void ptr_button(void *data, struct wl_pointer *p, uint32_t serial, uint32_t time,
                       uint32_t button, uint32_t state)
{
    (void)data; (void)p; (void)serial; (void)time;
    if (button != BTN_LEFT) return;
    s_btn = state == WL_POINTER_BUTTON_STATE_PRESSED;
    linux_io_pointer((int)s_px, (int)s_py, s_btn);
}

static void ptr_axis(void *data, struct wl_pointer *p, uint32_t time, uint32_t axis, wl_fixed_t v)
{
    (void)data; (void)p; (void)time;
    if (axis != WL_POINTER_AXIS_VERTICAL_SCROLL) return;
    s_axis += wl_fixed_to_double(v);
    int steps = (int)(s_axis / AXIS_STEP);
    if (steps) {
        s_axis -= steps * AXIS_STEP;
        linux_io_wheel(steps);
    }
}

static void ptr_frame(void *data, struct wl_pointer *p) { (void)data; (void)p; }
static void ptr_axis_source(void *data, struct wl_pointer *p, uint32_t s) { (void)data; (void)p; (void)s; }
static void ptr_axis_stop(void *data, struct wl_pointer *p, uint32_t t, uint32_t a)
{
    (void)data; (void)p; (void)t; (void)a;
    s_axis = 0;
}
static void ptr_axis_discrete(void *data, struct wl_pointer *p, uint32_t a, int32_t d)
{
    (void)data; (void)p; (void)a; (void)d;
}

static const struct wl_pointer_listener s_ptr_listener = {
    .enter = ptr_enter,
    .leave = ptr_leave,
    .motion = ptr_motion,
    .button = ptr_button,
    .axis = ptr_axis,
    .frame = ptr_frame,
    .axis_source = ptr_axis_source,
    .axis_stop = ptr_axis_stop,
    .axis_discrete = ptr_axis_discrete,
};

/* ---- seat / registry ------------------------------------------------------- */

static void seat_caps(void *data, struct wl_seat *seat, uint32_t caps)
{
    (void)data;
    if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !s_kbd) {
        s_kbd = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(s_kbd, &s_kb_listener, NULL);
    } else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && s_kbd) {
        wl_keyboard_destroy(s_kbd);
        s_kbd = NULL;
        linux_io_release_all_keys();
    }
#if defined(CONFIG_DRAFTLING_TOUCHSCREEN)
    if ((caps & WL_SEAT_CAPABILITY_POINTER) && !s_ptr) {
        s_ptr = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(s_ptr, &s_ptr_listener, NULL);
    } else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && s_ptr) {
        wl_pointer_destroy(s_ptr);
        s_ptr = NULL;
    }
#else
    (void)s_ptr_listener;
#endif
}

static void seat_name(void *data, struct wl_seat *seat, const char *name)
{
    (void)data; (void)seat; (void)name;
}

static const struct wl_seat_listener s_seat_listener = { seat_caps, seat_name };

static uint32_t min_u(uint32_t a, uint32_t b) { return a < b ? a : b; }

static void reg_global(void *data, struct wl_registry *r, uint32_t name, const char *iface,
                       uint32_t version)
{
    (void)data;
    if (strcmp(iface, wl_compositor_interface.name) == 0) {
        s_comp = wl_registry_bind(r, name, &wl_compositor_interface, min_u(version, 4));
    } else if (strcmp(iface, wl_shm_interface.name) == 0) {
        s_shm = wl_registry_bind(r, name, &wl_shm_interface, 1);
    } else if (strcmp(iface, wl_seat_interface.name) == 0 && !s_seat) {
        /* Version 5 has every pointer event the listener handles. */
        s_seat = wl_registry_bind(r, name, &wl_seat_interface, min_u(version, 5));
        wl_seat_add_listener(s_seat, &s_seat_listener, NULL);
    } else if (strcmp(iface, xdg_wm_base_interface.name) == 0) {
        s_wm = wl_registry_bind(r, name, &xdg_wm_base_interface, min_u(version, 5));
        xdg_wm_base_add_listener(s_wm, &s_wm_listener, NULL);
    } else if (strcmp(iface, zxdg_decoration_manager_v1_interface.name) == 0) {
        s_deco_mgr = wl_registry_bind(r, name, &zxdg_decoration_manager_v1_interface, 1);
    }
}

static void reg_remove(void *data, struct wl_registry *r, uint32_t name)
{
    (void)data; (void)r; (void)name;
}

static const struct wl_registry_listener s_reg_listener = { reg_global, reg_remove };

/* ---- backend -------------------------------------------------------------------- */

static void wl_close_all(void);

static bool wl_open(int *w, int *h)
{
    s_dpy = wl_display_connect(NULL);
    if (!s_dpy) {
        if (linux_io_explicit)
            ESP_LOGW(TAG, "No Wayland display (WAYLAND_DISPLAY not set or not reachable)");
        else
            ESP_LOGI(TAG, "No Wayland display (WAYLAND_DISPLAY not set or not reachable)");
        return false;
    }
    s_reg = wl_display_get_registry(s_dpy);
    wl_registry_add_listener(s_reg, &s_reg_listener, NULL);
    wl_display_roundtrip(s_dpy);
    if (!s_comp || !s_shm || !s_wm) {
        ESP_LOGW(TAG, "The compositor lacks wl_compositor, wl_shm or xdg_wm_base");
        wl_close_all();
        return false;
    }
    wl_display_roundtrip(s_dpy);   /* seat capabilities */

    s_w = linux_window_w ? linux_window_w : CONFIG_DRAFTLING_DISPLAY_WIDTH;
    s_h = linux_window_h ? linux_window_h : CONFIG_DRAFTLING_DISPLAY_HEIGHT;

    s_surf = wl_compositor_create_surface(s_comp);
    s_xsurf = xdg_wm_base_get_xdg_surface(s_wm, s_surf);
    xdg_surface_add_listener(s_xsurf, &s_xsurf_listener, NULL);
    s_top = xdg_surface_get_toplevel(s_xsurf);
    xdg_toplevel_add_listener(s_top, &s_top_listener, NULL);
    xdg_toplevel_set_title(s_top, LINUX_WINDOW_TITLE);
    xdg_toplevel_set_app_id(s_top, LINUX_APP_ID);
    xdg_toplevel_set_min_size(s_top, 320, 240);
    if (s_deco_mgr) {
        s_deco = zxdg_decoration_manager_v1_get_toplevel_decoration(s_deco_mgr, s_top);
        zxdg_toplevel_decoration_v1_set_mode(s_deco, ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
    }
    wl_surface_commit(s_surf);
    while (!s_configured) {
        if (wl_display_dispatch(s_dpy) < 0) {
            ESP_LOGW(TAG, "Lost the compositor before the window was shown");
            wl_close_all();
            return false;
        }
    }
    *w = s_w;
    *h = s_h;
    return true;
}

static int wl_fd(void)
{
    return s_dpy ? wl_display_get_fd(s_dpy) : -1;
}

static void wl_dispatch(void)
{
    if (!s_dpy) return;
    while (wl_display_prepare_read(s_dpy) != 0) wl_display_dispatch_pending(s_dpy);
    wl_display_flush(s_dpy);
    struct pollfd p = { .fd = wl_display_get_fd(s_dpy), .events = POLLIN };
    if (poll(&p, 1, 0) > 0) wl_display_read_events(s_dpy);
    else wl_display_cancel_read(s_dpy);
    wl_display_dispatch_pending(s_dpy);
    if (wl_display_get_error(s_dpy)) {
        ESP_LOGE(TAG, "Connection to the compositor lost");
        linux_io_close();
    }
}

static void wl_present(const uint32_t *fb, int fb_w, int fb_h, int x, int y, int w, int h)
{
    shm_buf_t *b = NULL;
    for (int i = 0; i < N_BUFFERS; i++) {
        if (!s_bufs[i].busy) { b = &s_bufs[i]; break; }
    }
    if (!b) {
        s_want_redraw = true;   /* shown when the compositor frees one */
        return;
    }
    if (b->w != fb_w || b->h != fb_h) {
        if (!buffer_alloc(b, fb_w, fb_h)) return;
    }
    memcpy(b->data, fb, (size_t)fb_w * fb_h * 4);
    wl_surface_attach(s_surf, b->buf, 0, 0);
    wl_surface_damage_buffer(s_surf, x, y, w, h);
    wl_surface_commit(s_surf);
    b->busy = true;
}

static void wl_flush_out(void)
{
    if (s_dpy) wl_display_flush(s_dpy);
}

static void wl_close_all(void)
{
    if (!s_dpy) return;
    for (int i = 0; i < N_BUFFERS; i++) buffer_free(&s_bufs[i]);
    if (s_kbd) wl_keyboard_destroy(s_kbd);
    if (s_ptr) wl_pointer_destroy(s_ptr);
    if (s_deco) zxdg_toplevel_decoration_v1_destroy(s_deco);
    if (s_top) xdg_toplevel_destroy(s_top);
    if (s_xsurf) xdg_surface_destroy(s_xsurf);
    if (s_surf) wl_surface_destroy(s_surf);
    wl_display_flush(s_dpy);
    wl_display_disconnect(s_dpy);
    s_dpy = NULL;
    s_kbd = NULL; s_ptr = NULL; s_deco = NULL; s_top = NULL; s_xsurf = NULL; s_surf = NULL;
    s_comp = NULL; s_shm = NULL; s_seat = NULL; s_wm = NULL; s_deco_mgr = NULL;
}

const linux_io_backend_t linux_io_wayland = {
    .name = "wayland",
    .open = wl_open,
    .fd = wl_fd,
    .dispatch = wl_dispatch,
    .present = wl_present,
    .flush = wl_flush_out,
    .close = wl_close_all,
};
