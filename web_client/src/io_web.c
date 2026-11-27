/*
 * Web client: the I/O backend of the Linux program's LVGL port
 * (linux/src/lvgl_port_linux.c) for a canvas in the browser page.
 *
 * The program runs in Web Workers (Emscripten pthreads, the LVGL loop
 * on the "main" pthread, PROXY_TO_PTHREAD); the page's own thread
 * owns the DOM. So:
 *
 * - Input: the page (web/client.js) turns DOM events into calls of
 *   web_input(), which only appends them to a lock-free queue (one
 *   writer, the page's thread; one reader, the LVGL loop) and wakes
 *   the loop. dispatch() hands them to the Linux port's hooks:
 *   linux_io_key() (KeyboardEvent.code mapped to Linux key codes by
 *   the page, so keyboard_linux.c turns them into HID usages as for a
 *   Wayland / X11 window), linux_io_pointer(), linux_io_wheel(),
 *   linux_io_resize().
 * - Output: present() converts the rectangle to RGBA and hands a copy
 *   to the page (Module.webPresent), synchronously, on the page's
 *   thread.
 * - Clipboard: Ctrl+C / Ctrl+X give the text to the page, which puts
 *   it on the system clipboard (Module.webClipSet); Ctrl+V asks the
 *   page (Module.webClipRequest), whose answer -- the text of the
 *   browser's paste event, or of navigator.clipboard.readText() --
 *   comes back through the queue (WEB_EV_PASTE) to
 *   linux_io_clipboard_text() (clipboard_linux.c).
 */

#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#include <emscripten.h>
#include <emscripten/threading.h>

#include "editor.h"
#include "editor_ui.h"
#include "esp_log.h"
#include "linux_host.h"
#include "web_host.h"

static const char *TAG = "WebIO";

/* ---- the input queue ------------------------------------------------------ */

typedef struct {
    int type, a, b, c;
} web_event_t;

#define QUEUE_LEN 1024   /* a power of two */

static web_event_t s_queue[QUEUE_LEN];
static _Atomic uint32_t s_head;   /* written by the page's thread */
static _Atomic uint32_t s_tail;   /* written by the LVGL loop */

/* The page's thread: append an event and wake the loop. A full queue
 * (the loop stuck for a long time) drops the event. */
EMSCRIPTEN_KEEPALIVE void web_input(int type, int a, int b, int c)
{
    uint32_t h = atomic_load_explicit(&s_head, memory_order_relaxed);
    uint32_t t = atomic_load_explicit(&s_tail, memory_order_acquire);
    if (h - t >= QUEUE_LEN) {
        if (type == WEB_EV_PASTE && a) free((void *)(intptr_t)a);
        return;
    }
    s_queue[h & (QUEUE_LEN - 1)] = (web_event_t){ type, a, b, c };
    atomic_store_explicit(&s_head, h + 1, memory_order_release);
    emscripten_futex_wake((volatile void *)&s_head, 1);
}

/* ---- state the page reads or sets ------------------------------------------ */

static _Atomic int s_unsaved;
static volatile double s_quota, s_usage;

/* True while a document has changes not saved yet: the page asks the
 * browser to confirm leaving (beforeunload). */
EMSCRIPTEN_KEEPALIVE int web_unsaved(void)
{
    return atomic_load(&s_unsaved);
}

/* navigator.storage.estimate(): the "card"'s size and use. */
EMSCRIPTEN_KEEPALIVE void web_storage(double quota, double usage)
{
    s_quota = quota;
    s_usage = usage;
}

void web_storage_get(double *quota, double *usage)
{
    *quota = s_quota;
    *usage = s_usage;
}

static void any_unsaved_cb(editor_doc_t *doc, void *ctx)
{
    if (editor_doc_is_modified(doc)) *(bool *)ctx = true;
}

/* ---- clipboard ---------------------------------------------------------- */

static bool s_paste_wanted;
/* The last copy went to the card (.clipboard), not to the page: Ctrl+V
 * then uses the editor's own clipboard. */
static bool s_copy_on_card;

static void web_clip_set(const char *text, size_t len)
{
    s_copy_on_card = text == NULL;
    if (!text) return;
    MAIN_THREAD_EM_ASM({
        if (Module.webClipSet) Module.webClipSet(UTF8ToString($0, $1));
    }, text, (int)len);
}

static bool web_clip_request(void)
{
    if (s_copy_on_card) return false;
    s_paste_wanted = true;
    MAIN_THREAD_ASYNC_EM_ASM({
        if (Module.webClipRequest) Module.webClipRequest();
        else Module._web_input(6, 0, 0, 0);
    });
    return true;
}

static void got_paste(char *text, int len)
{
    if (s_paste_wanted) {
        s_paste_wanted = false;
        linux_io_clipboard_text(text, text ? (size_t)len : 0);
    }
    free(text);
}

/* ---- rounded screen corners ------------------------------------------- */

/* The page reports which corners of the canvas are rounded corners of
 * the screen (full screen on a phone); the editor fits its bars to
 * them (Settings -> Display -> "Rounded screen corners"). While a
 * dialog keeps it from laying the screens out again, try again on
 * every pass of the loop. */
static bool s_corners_pending;
static unsigned s_corners_mask;
static int s_corners_milli = 1000;

static void corners_try(void)
{
    s_corners_pending = !editor_ui_set_screen_corners(s_corners_mask, s_corners_milli);
}

/* The setting changed (0 = off): the page fits its key bar to it. */
void editor_host_corners_changed(int size)
{
    MAIN_THREAD_ASYNC_EM_ASM({
        if (Module.webCorners) Module.webCorners($0);
    }, size);
}

/* Settings -> Display -> Orientation / Rotate 180: the page turns its
 * screen and reports the new size (WEB_EV_RESIZE). */
void editor_host_rotation_changed(int orient, bool rotate180)
{
    MAIN_THREAD_ASYNC_EM_ASM({
        if (Module.webRotation) Module.webRotation($0, $1);
    }, orient, rotate180 ? 1 : 0);
}

/* ---- the backend ------------------------------------------------------- */

static uint8_t *s_rgba;
static size_t s_rgba_bytes;

static bool web_open(int *w, int *h)
{
    *w = MAIN_THREAD_EM_ASM_INT({ return Module.webSize ? Module.webSize()[0] : 0; });
    *h = MAIN_THREAD_EM_ASM_INT({ return Module.webSize ? Module.webSize()[1] : 0; });
    if (*w <= 0 || *h <= 0) {
        *w = CONFIG_DRAFTLING_DISPLAY_WIDTH;
        *h = CONFIG_DRAFTLING_DISPLAY_HEIGHT;
    }
    linux_io_repeats_keys = false;   /* the page drops the browser's repeats */
    return true;
}

static void web_dispatch(void)
{
    uint32_t t = atomic_load_explicit(&s_tail, memory_order_relaxed);
    uint32_t h = atomic_load_explicit(&s_head, memory_order_acquire);
    for (; t != h; t++) {
        web_event_t e = s_queue[t & (QUEUE_LEN - 1)];
        switch (e.type) {
        case WEB_EV_KEY:
            linux_io_sync_modifiers((unsigned)e.c);
            linux_io_key(e.a, e.b != 0);
            break;
        case WEB_EV_POINTER:
            linux_io_pointer(e.a, e.b, e.c != 0);
            break;
        case WEB_EV_WHEEL:
            linux_io_wheel(e.a);
            break;
        case WEB_EV_RESIZE:
            linux_io_resize(e.a, e.b);
            break;
        case WEB_EV_RELEASE_ALL:
            linux_io_release_all_keys();
            break;
        case WEB_EV_PASTE:
            got_paste((char *)(intptr_t)e.a, e.b);
            break;
        case WEB_EV_SAVE:
            if (linux_pre_sleep_cb) linux_pre_sleep_cb();
            break;
        case WEB_EV_DAMAGE:
            linux_io_damage_all();
            break;
        case WEB_EV_FILES:
            web_files_run((void *)(intptr_t)e.a);
            break;
        case WEB_EV_CORNERS:
            s_corners_mask = (unsigned)e.a;
            s_corners_milli = e.b;
            s_corners_pending = true;
            break;
        default:
            break;
        }
    }
    atomic_store_explicit(&s_tail, t, memory_order_release);
    if (s_corners_pending) corners_try();

    bool unsaved = false;
    editor_doc_foreach(any_unsaved_cb, &unsaved);
    atomic_store(&s_unsaved, unsaved ? 1 : 0);
}

static void web_wait(int timeout_ms)
{
    uint32_t h = atomic_load(&s_head);
    if (h != atomic_load(&s_tail)) return;
    emscripten_futex_wait((volatile void *)&s_head, h, (double)timeout_ms);
}

static void web_present(const uint32_t *fb, int fb_w, int fb_h, int x, int y, int w, int h)
{
    size_t need = (size_t)w * h * 4;
    if (need > s_rgba_bytes) {
        free(s_rgba);
        s_rgba = (uint8_t *)malloc(need);
        if (!s_rgba) {
            s_rgba_bytes = 0;
            ESP_LOGE(TAG, "Out of memory for a %dx%d update", w, h);
            return;
        }
        s_rgba_bytes = need;
    }
    uint8_t *o = s_rgba;
    for (int yy = y; yy < y + h; yy++) {
        const uint32_t *row = fb + (size_t)yy * fb_w + x;
        for (int xx = 0; xx < w; xx++) {
            uint32_t c = row[xx];
            *o++ = (uint8_t)(c >> 16);
            *o++ = (uint8_t)(c >> 8);
            *o++ = (uint8_t)c;
            *o++ = 0xFF;
        }
    }
    MAIN_THREAD_EM_ASM({
        if (Module.webPresent)
            Module.webPresent($0, $1, $2, $3, $4, $5, HEAPU8.slice($6, $6 + $4 * $5 * 4));
    }, fb_w, fb_h, x, y, w, h, s_rgba);
}

/* Brightness (Settings -> Display): the page dims everything it shows
 * with a CSS filter. */
static bool web_set_dim(int percent)
{
    MAIN_THREAD_ASYNC_EM_ASM({
        if (Module.webBrightness) Module.webBrightness($0);
    }, percent);
    return true;
}

const linux_io_backend_t linux_io_web = {
    .name = "web",
    .open = web_open,
    .fd = NULL,
    .dispatch = web_dispatch,
    .present = web_present,
    .flush = NULL,
    .close = NULL,
    .repeats_keys = false,
    .clip_set = web_clip_set,
    .clip_request = web_clip_request,
    .poll_fds = NULL,
    .wait = web_wait,
    .set_dim = web_set_dim,
};
