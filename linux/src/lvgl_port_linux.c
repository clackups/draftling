/*
 * Linux: LVGL display and pointer port, and the main loop.
 *
 * LVGL renders RGB565 tiles (LV_DISPLAY_RENDER_MODE_PARTIAL, as on the
 * color-LCD boards); flush_cb converts them into a 32-bit frame of the
 * window's size -- every LVGL pixel as a 2x2 block with the "2x scale"
 * setting -- and the changed rectangle goes to the I/O backend after
 * each pass of the loop.
 *
 * The window can be resized (CONFIG_DRAFTLING_DISPLAY_RESIZABLE). A
 * new size takes effect once it has held for RESIZE_SETTLE_MS (a drag
 * sends many) and once the editor says it can lay its screens out
 * again (editor_ui_can_relayout(): not while a dialog or pop-up is
 * open). Until then the old canvas is shown, clipped or with a border.
 *
 * Locking is the device's: a recursive mutex around everything that
 * touches LVGL. The main thread holds it while it handles input and
 * runs lv_timer_handler(); the editor's worker threads take it through
 * draftling_lvgl_port_lock().
 */

#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "display_scale.h"
#include "editor_ui.h"
#include "esp_log.h"
#include "linux_host.h"
#include "lvgl.h"
#include "lvgl_port.h"
#include "sdkconfig.h"

static const char *TAG = "LinuxPort";

/* Smallest canvas the editor is laid out for; a smaller window shows
 * part of it. */
#define MIN_CANVAS_W 400
#define MIN_CANVAS_H 300
#define RESIZE_SETTLE_MS 120
#define RELAYOUT_RETRY_MS 250
#define LOOP_MAX_WAIT_MS 50
/* Clipboard transfers the loop waits on besides the display. */
#define LOOP_MAX_EXTRA_FDS 8
#define BORDER_COLOR 0x00202020u

static const linux_io_backend_t *s_io;

static lv_display_t *s_disp;
static void *s_buf1, *s_buf2;
static size_t s_buf_bytes;
static int s_scale = 1;

static uint32_t *s_fb;          /* window-sized 0x00RRGGBB frame */
static int s_win_w, s_win_h;    /* frame / window size */
static int s_cv_w, s_cv_h;      /* LVGL canvas (logical) size */
static int s_dx1, s_dy1, s_dx2 = -1, s_dy2 = -1;   /* dirty box, window px */

static int s_want_w, s_want_h;  /* last size the backend reported */
static uint32_t s_want_at;
static bool s_relayout_pending;
static uint32_t s_relayout_try_at;

static int s_ptr_x, s_ptr_y;
static bool s_ptr_down;
static bool s_ptr_swallow;      /* the press that woke the screen */

static bool s_blank;            /* standby: the window shows black */
static uint32_t *s_black;       /* a black frame of the window's size */
static int s_black_w, s_black_h;

/* Dimming in place of a backlight (linux_port_set_dim()): the frame
 * is presented from s_dim_fb, s_fb scaled by s_dim_lut. */
static bool s_dim;
static uint8_t s_dim_lut[256];
static uint32_t *s_dim_fb;
static int s_dim_w, s_dim_h;

static pthread_mutex_t s_lock;
static volatile sig_atomic_t s_quit_signal;
static volatile sig_atomic_t s_shot_signal;
static bool s_quit_requested;

static uint32_t now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)((uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u);
}

static void mark_dirty(int x1, int y1, int x2, int y2)
{
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= s_win_w) x2 = s_win_w - 1;
    if (y2 >= s_win_h) y2 = s_win_h - 1;
    if (x2 < x1 || y2 < y1) return;
    if (s_dx2 < 0) {
        s_dx1 = x1; s_dy1 = y1; s_dx2 = x2; s_dy2 = y2;
        return;
    }
    if (x1 < s_dx1) s_dx1 = x1;
    if (y1 < s_dy1) s_dy1 = y1;
    if (x2 > s_dx2) s_dx2 = x2;
    if (y2 > s_dy2) s_dy2 = y2;
}

void linux_io_damage_all(void)
{
    mark_dirty(0, 0, s_win_w - 1, s_win_h - 1);
}

static inline uint32_t rgb565_to_xrgb(uint16_t c)
{
    uint32_t r = (c >> 11) & 0x1F, g = (c >> 5) & 0x3F, b = c & 0x1F;
    r = (r << 3) | (r >> 2);
    g = (g << 2) | (g >> 4);
    b = (b << 3) | (b >> 2);
    return (r << 16) | (g << 8) | b;
}

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px)
{
    const uint16_t *src = (const uint16_t *)px;
    int w = lv_area_get_width(area);
    for (int y = area->y1; y <= area->y2; y++, src += w) {
        for (int sy = 0; sy < s_scale; sy++) {
            int wy = y * s_scale + sy;
            if (wy >= s_win_h) break;
            uint32_t *row = s_fb + (size_t)wy * s_win_w;
            for (int i = 0; i < w; i++) {
                uint32_t c = rgb565_to_xrgb(src[i]);
                int wx = (area->x1 + i) * s_scale;
                for (int sx = 0; sx < s_scale && wx + sx < s_win_w; sx++) row[wx + sx] = c;
            }
        }
    }
    mark_dirty(area->x1 * s_scale, area->y1 * s_scale,
               (area->x2 + 1) * s_scale - 1, (area->y2 + 1) * s_scale - 1);
    lv_display_flush_ready(disp);
}

static uint32_t tick_cb(void)
{
    return now_ms();
}

static void pointer_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    data->point.x = s_ptr_x;
    data->point.y = s_ptr_y;
    data->state = s_ptr_down ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

void linux_io_pointer(int x, int y, bool pressed)
{
    /* Any pointer activity counts as input; the press that turns the
     * screen on is not passed on, up to its release. */
    if (linux_standby_wake() && pressed) s_ptr_swallow = true;
    if (s_ptr_swallow) {
        if (!pressed) s_ptr_swallow = false;
        pressed = false;
    }
    x /= s_scale;
    y /= s_scale;
    s_ptr_x = x < 0 ? 0 : (x >= s_cv_w ? s_cv_w - 1 : x);
    s_ptr_y = y < 0 ? 0 : (y >= s_cv_h ? s_cv_h - 1 : y);
    s_ptr_down = pressed;
}

void linux_io_resize(int w, int h)
{
    if (w <= 0 || h <= 0) return;
    if (w == s_want_w && h == s_want_h) return;
    s_want_w = w;
    s_want_h = h;
    s_want_at = now_ms();
}

void linux_port_set_blank(bool blank)
{
    if (blank == s_blank) return;
    s_blank = blank;
    if (!blank) {
        free(s_black);
        s_black = NULL;
        s_black_w = s_black_h = 0;
    }
    /* The next pass shows the black frame, or the picture again. */
    linux_io_damage_all();
}

void linux_port_set_dim(int percent)
{
    if (s_io && s_io->set_dim && s_io->set_dim(percent)) return;
    if (percent >= 100) {
        if (!s_dim) return;
        s_dim = false;
        free(s_dim_fb);
        s_dim_fb = NULL;
        s_dim_w = s_dim_h = 0;
    } else {
        /* Scaled from 20 % up, so the lowest steps stay readable:
         * 10 % -> 28 %, 25 % -> 40 %, 50 % -> 60 %, 75 % -> 80 %. */
        int f = 20 + (percent < 0 ? 0 : percent) * 80 / 100;
        for (int i = 0; i < 256; i++) s_dim_lut[i] = (uint8_t)((i * f + 50) / 100);
        s_dim = true;
    }
    linux_io_damage_all();
}

/* Show the rectangle of the frame, dimmed if so set. */
static void present_frame(int x, int y, int w, int h)
{
    if (!s_dim) {
        s_io->present(s_fb, s_win_w, s_win_h, x, y, w, h);
        return;
    }
    if (s_dim_w != s_win_w || s_dim_h != s_win_h) {
        free(s_dim_fb);
        s_dim_fb = (uint32_t *)malloc((size_t)s_win_w * s_win_h * 4);
        if (!s_dim_fb) {
            s_dim_w = s_dim_h = 0;
            s_io->present(s_fb, s_win_w, s_win_h, x, y, w, h);
            return;
        }
        s_dim_w = s_win_w;
        s_dim_h = s_win_h;
        /* The backend may show more than the rectangle: fill it all. */
        x = 0; y = 0; w = s_win_w; h = s_win_h;
    }
    for (int yy = y; yy < y + h; yy++) {
        const uint32_t *src = s_fb + (size_t)yy * s_win_w;
        uint32_t *dst = s_dim_fb + (size_t)yy * s_win_w;
        for (int xx = x; xx < x + w; xx++) {
            uint32_t c = src[xx];
            dst[xx] = ((uint32_t)s_dim_lut[(c >> 16) & 0xFF] << 16) |
                      ((uint32_t)s_dim_lut[(c >> 8) & 0xFF] << 8) |
                      s_dim_lut[c & 0xFF];
        }
    }
    s_io->present(s_dim_fb, s_win_w, s_win_h, x, y, w, h);
}

/* Standby: show black over the whole window instead of the frame. */
static void present_black(void)
{
    if (s_black_w != s_win_w || s_black_h != s_win_h) {
        free(s_black);
        s_black = (uint32_t *)calloc((size_t)s_win_w * s_win_h, 4);
        if (!s_black) return;
        s_black_w = s_win_w;
        s_black_h = s_win_h;
    }
    s_io->present(s_black, s_win_w, s_win_h, 0, 0, s_win_w, s_win_h);
}

void linux_io_close(void)
{
    s_quit_requested = true;
}

static void canvas_for_window(int ww, int wh, int *cw, int *ch)
{
    *cw = ww / s_scale;
    *ch = wh / s_scale;
    if (*cw < MIN_CANVAS_W) *cw = MIN_CANVAS_W;
    if (*ch < MIN_CANVAS_H) *ch = MIN_CANVAS_H;
}

/* Draw buffers for a canvas: a quarter of the screen each, as on the
 * color-LCD boards. */
static void alloc_draw_buffers(int cw, int ch)
{
    /* A multiple of LV_DRAW_BUF_ALIGN: LVGL fits as many rows as the
     * size holds, but its reshape of the buffer to those rows asks for
     * the size rounded up to the alignment and fails at an odd width;
     * the rows were then drawn with the previous area's stride (a
     * sheared, repeated picture in a portrait window). */
    size_t bytes = LV_ROUND_UP((size_t)cw * (ch / 4 + 1) * 2, LV_DRAW_BUF_ALIGN);
    if (bytes > s_buf_bytes) {
        free(s_buf1);
        free(s_buf2);
        s_buf1 = malloc(bytes);
        s_buf2 = malloc(bytes);
        if (!s_buf1 || !s_buf2) {
            ESP_LOGE(TAG, "Out of memory for the draw buffers");
            abort();
        }
        s_buf_bytes = bytes;
    }
    lv_display_set_buffers(s_disp, s_buf1, s_buf2, s_buf_bytes,
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
}

/* Fill the parts of the frame outside the canvas. */
static void paint_border(void)
{
    int cw = s_cv_w * s_scale, ch = s_cv_h * s_scale;
    for (int y = 0; y < s_win_h; y++) {
        uint32_t *row = s_fb + (size_t)y * s_win_w;
        int from = y < ch ? cw : 0;
        for (int x = from; x < s_win_w; x++) row[x] = BORDER_COLOR;
    }
}

/* A new frame size: keep what is drawn, then the editor redraws. */
static void set_frame_size(int w, int h)
{
    uint32_t *nfb = (uint32_t *)malloc((size_t)w * h * 4);
    if (!nfb) {
        ESP_LOGE(TAG, "Out of memory for a %dx%d frame", w, h);
        return;
    }
    for (int y = 0; y < h; y++) {
        uint32_t *row = nfb + (size_t)y * w;
        for (int x = 0; x < w; x++) {
            row[x] = (s_fb && y < s_win_h && x < s_win_w)
                         ? s_fb[(size_t)y * s_win_w + x] : BORDER_COLOR;
        }
    }
    free(s_fb);
    s_fb = nfb;
    s_win_w = w;
    s_win_h = h;
    paint_border();
    linux_io_damage_all();
}

static void try_relayout(void)
{
    int cw, ch;
    canvas_for_window(s_win_w, s_win_h, &cw, &ch);
    if (cw == s_cv_w && ch == s_cv_h) {
        s_relayout_pending = false;
        return;
    }
    if (!editor_ui_can_relayout()) {
        s_relayout_pending = true;
        s_relayout_try_at = now_ms() + RELAYOUT_RETRY_MS;
        return;
    }
    ESP_LOGI(TAG, "Canvas %dx%d -> %dx%d", s_cv_w, s_cv_h, cw, ch);
    s_cv_w = cw;
    s_cv_h = ch;
    alloc_draw_buffers(cw, ch);
    lv_display_set_resolution(s_disp, cw, ch);
    editor_ui_relayout();
    lv_obj_invalidate(lv_screen_active());
    s_relayout_pending = false;
    paint_border();
    linux_io_damage_all();
}

static void handle_resize(void)
{
    uint32_t t = now_ms();
    if ((s_want_w != s_win_w || s_want_h != s_win_h) &&
        (int32_t)(t - s_want_at) >= RESIZE_SETTLE_MS) {
        set_frame_size(s_want_w, s_want_h);
        try_relayout();
    } else if (s_relayout_pending && (int32_t)(t - s_relayout_try_at) >= 0) {
        try_relayout();
    }
}

void draftling_lvgl_port_init(int width, int height, int rotate_deg, int scale)
{
    (void)rotate_deg;   /* a window is always upright */
    pthread_mutexattr_t a;
    pthread_mutexattr_init(&a);
    pthread_mutexattr_settype(&a, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&s_lock, &a);
    pthread_mutexattr_destroy(&a);
    pthread_mutex_lock(&s_lock);   /* the main thread holds it, see run */

    lv_init();
    lv_tick_set_cb(tick_cb);

    s_scale = scale > 1 ? scale : 1;
    set_frame_size(width, height);
    s_want_w = width;
    s_want_h = height;
    canvas_for_window(width, height, &s_cv_w, &s_cv_h);

    s_disp = lv_display_create(s_cv_w, s_cv_h);
    lv_display_set_color_format(s_disp, LV_COLOR_FORMAT_RGB565);
    alloc_draw_buffers(s_cv_w, s_cv_h);
    lv_display_set_flush_cb(s_disp, flush_cb);

#if defined(CONFIG_DRAFTLING_TOUCHSCREEN)
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, pointer_read_cb);
#else
    (void)pointer_read_cb;
#endif
}

bool draftling_lvgl_port_lock(int timeout_ms)
{
    if (timeout_ms < 0) return pthread_mutex_lock(&s_lock) == 0;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (long)(timeout_ms % 1000) * 1000000L;
    if (ts.tv_nsec >= 1000000000L) {
        ts.tv_sec++;
        ts.tv_nsec -= 1000000000L;
    }
    return pthread_mutex_timedlock(&s_lock, &ts) == 0;
}

void draftling_lvgl_port_unlock(void)
{
    pthread_mutex_unlock(&s_lock);
}

void draftling_lvgl_port_set_flip180(bool flip) { (void)flip; }

void draftling_lvgl_port_clear_buffers(void) { }

void draftling_lvgl_port_map_to_physical(int x, int y, int w, int h,
                                         int *px, int *py, int *pw, int *ph)
{
    *px = x * s_scale;
    *py = y * s_scale;
    *pw = w * s_scale;
    *ph = h * s_scale;
}

/* ---- backends ------------------------------------------------------------ */

static const linux_io_backend_t *const s_backends[] = {
#if defined(LINUX_IO_WAYLAND)
    &linux_io_wayland,
#endif
#if defined(LINUX_IO_X11)
    &linux_io_x11,
#endif
#if defined(LINUX_IO_WEB)
    &linux_io_web,
#endif
};
#define N_BACKENDS (sizeof(s_backends) / sizeof(s_backends[0]))

static int s_open_w, s_open_h;

bool linux_io_explicit;

bool linux_port_open_io(const char *name)
{
    if (N_BACKENDS == 0) {
        fprintf(stderr, "draftling: built without any I/O backend\n");
        return false;
    }
    for (size_t i = 0; i < N_BACKENDS; i++) {
        const linux_io_backend_t *b = s_backends[i];
        if (name && strcmp(name, b->name) != 0) continue;
        int w = 0, h = 0;
        linux_io_explicit = name != NULL;
        bool ok = b->open(&w, &h);
        linux_io_explicit = false;
        if (ok) {
            s_io = b;
            linux_io_repeats_keys = b->repeats_keys;
            s_open_w = w;
            s_open_h = h;
            ESP_LOGI(TAG, "Using %s, %dx%d", b->name, w, h);
            return true;
        }
        if (name) {
            fprintf(stderr, "draftling: cannot use %s (see the message above)\n", name);
#if defined(CONFIG_DRAFTLING_LINUX_FLATPAK)
            /* --socket=fallback-x11: no X display while there is a
             * Wayland one, even with --socket=x11 added. */
            if (strcmp(name, "x11") == 0)
                fprintf(stderr, "draftling: the Flatpak reaches X11 only without Wayland; "
                        "to use it anyway: flatpak run --nosocket=fallback-x11 "
                        "--socket=x11 art.draftling.Draftling --io x11\n");
#endif
            return false;
        }
    }
    if (name) {
        fprintf(stderr, "draftling: unknown I/O backend '%s'; this build has:", name);
        for (size_t i = 0; i < N_BACKENDS; i++) fprintf(stderr, " %s", s_backends[i]->name);
        fprintf(stderr, "\n");
    } else {
        fprintf(stderr, "draftling: no display found (tried:");
        for (size_t i = 0; i < N_BACKENDS; i++) fprintf(stderr, " %s", s_backends[i]->name);
        fprintf(stderr, "); run with -v for details\n");
    }
    return false;
}

const char *linux_port_io_name(void)
{
    return s_io ? s_io->name : "none";
}

const linux_io_backend_t *linux_port_io(void)
{
    return s_io;
}

/* The size the backend opened with (main_linux.cpp passes it to
 * draftling_lvgl_port_init()). */
void linux_port_initial_size(int *w, int *h)
{
    *w = s_open_w;
    *h = s_open_h;
}

void linux_port_close_io(void)
{
    if (s_io && s_io->close) s_io->close();
    s_io = NULL;
}

static void on_signal(int sig)
{
    if (sig == SIGUSR1) s_shot_signal = 1;
    else s_quit_signal = 1;
}

/* SIGUSR1: the frame as it is now, to <settings folder>/screenshot.ppm
 * (a debugging aid; see README.md). */
static void write_screenshot(void)
{
    char path[sizeof(linux_config_dir) + 32], tmp[sizeof(path) + 8];
    snprintf(path, sizeof(path), "%s/screenshot.ppm", linux_config_dir);
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    FILE *f = fopen(tmp, "wb");
    if (!f) return;
    fprintf(f, "P6\n%d %d\n255\n", s_win_w, s_win_h);
    for (size_t i = 0; i < (size_t)s_win_w * s_win_h; i++) {
        uint8_t px[3] = { (uint8_t)(s_fb[i] >> 16), (uint8_t)(s_fb[i] >> 8), (uint8_t)s_fb[i] };
        fwrite(px, 1, 3, f);
    }
    if (fclose(f) == 0) rename(tmp, path);
    ESP_LOGI(TAG, "Screenshot written to %s", path);
}

void linux_port_run(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    signal(SIGPIPE, SIG_IGN);

    /* The lock is held from draftling_lvgl_port_init() on. */
    for (;;) {
        if (s_quit_signal || s_quit_requested) {
            ESP_LOGI(TAG, "Leaving: saving the open documents");
            linux_quit(0);   /* runs the pre-sleep hook under the lock */
        }
        if (s_shot_signal) {
            s_shot_signal = 0;
            write_screenshot();
        }
        if (s_io->dispatch) s_io->dispatch();
        handle_resize();
        uint32_t wait = lv_timer_handler();
        if (s_dx2 >= 0) {
            /* While the screen is off, LVGL keeps drawing into the
             * frame; it is shown when the screen comes back on. */
            if (s_blank) present_black();
            else present_frame(s_dx1, s_dy1, s_dx2 - s_dx1 + 1, s_dy2 - s_dy1 + 1);
            s_dx2 = s_dy2 = -1;
        }
        if (s_io->flush) s_io->flush();
        pthread_mutex_unlock(&s_lock);

        if (wait > LOOP_MAX_WAIT_MS) wait = LOOP_MAX_WAIT_MS;
        if (wait < 1) wait = 1;
        if (s_want_w != s_win_w || s_want_h != s_win_h || s_relayout_pending) {
            if (wait > 20) wait = 20;
        }
        if (s_io->wait) {
            s_io->wait((int)wait);
            pthread_mutex_lock(&s_lock);
            continue;
        }
        struct pollfd pfd[1 + LOOP_MAX_EXTRA_FDS];
        int nfd = 0;
        int dfd = s_io->fd ? s_io->fd() : -1;
        if (dfd >= 0) pfd[nfd++] = (struct pollfd){ .fd = dfd, .events = POLLIN };
        if (s_io->poll_fds) nfd += s_io->poll_fds(pfd + nfd, LOOP_MAX_EXTRA_FDS);
        if (nfd > 0) {
            poll(pfd, (nfds_t)nfd, (int)wait);
        } else {
            struct timespec ts = { 0, (long)wait * 1000000L };
            nanosleep(&ts, NULL);
        }
        pthread_mutex_lock(&s_lock);
    }
}

int linux_display_scale(void)
{
    return s_scale;
}
