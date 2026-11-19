/*
 * Online demo: the display backend API.
 *
 * Color LCD boards (Freenove FNK0104S): LVGL's frames reach the page
 * through lvgl_port_demo.c directly; these calls only carry the
 * backlight level to the page.
 *
 * E-paper boards (CONFIG_DRAFTLING_DISPLAY_EPD, the Seeed reTerminal
 * Sticky): a simulated panel with the behaviour of
 * firmware/components/display/display_reterminal_sticky.cpp. The LVGL
 * port pushes RGB565 tiles into a 1-bpp frame buffer (the same black /
 * white threshold as epd_fb.h) and calls display_flush() after the
 * last tile; the flush refreshes the panel with the whole frame when
 * the dirty box meets the editor's partial clip. The first refresh
 * after power-on is a full one, every later one a fast one, as on the
 * Sticky. A refresh keeps the panel busy for as long as the SSD1677's
 * waveform takes. A flush while the panel is busy does not start a
 * refresh of its own: on the device the LVGL task waits in
 * sticky_wait_idle() and then sends the frame as it is by then, so
 * everything typed during a refresh appears in the next one. Here the
 * flush only notes that a refresh is owed, and demo_display_poll()
 * (main loop) starts it with the current frame once the panel is
 * free. The page animates each refresh (web/demo.js).
 */

#include <stdlib.h>
#include <string.h>

#include <emscripten.h>

#include "demo_host.h"
#include "display.h"
#include "sdkconfig.h"

void display_init(int a, int b, int c, int d, int e, int f, int w, int h)
{
    (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; (void)w; (void)h;
}
void display_spi_tft_config(const display_spi_tft_config_t *cfg) { (void)cfg; }
void display_nv3041a_config(const display_nv3041a_config_t *cfg) { (void)cfg; }
void display_ssd1677_spi_config(const display_ssd1677_spi_config_t *cfg) { (void)cfg; }
void display_axs15231b_init(const display_axs15231b_config_t *cfg) { (void)cfg; }
void display_sleep(void) { }
void display_wake(void) { }
void display_set_invert(bool on) { (void)on; }
void display_set_shared_i2c_bus(void *bus) { (void)bus; }

#if !defined(CONFIG_DRAFTLING_DISPLAY_EPD)

/* ---- color LCD ----------------------------------------------------------- */

void display_clear(uint8_t color) { (void)color; }
void display_set_pixel(uint16_t x, uint16_t y, uint8_t color) { (void)x; (void)y; (void)color; }
void display_flush(void) { }
void display_full_refresh(void) { }
void display_request_full_refresh(void) { }
bool display_push_rgb565(int x, int y, int w, int h, const void *map)
{
    (void)x; (void)y; (void)w; (void)h; (void)map;
    return false;
}
void display_set_partial_clip(int x, int y, int w, int h) { (void)x; (void)y; (void)w; (void)h; }
void display_deep_sleep_prepare(void) { demo_host_set_backlight(0); }
void display_set_backlight(int percent) { demo_host_set_backlight(percent); }
const char *display_controller_name(void) { return "ST7796 (simulated)"; }
void demo_display_poll(void) { }
void demo_display_epd_init(int w, int h) { (void)w; (void)h; }

#else

/* ---- e-paper (SSD1677, reTerminal Sticky) --------------------------------- */

/* SSD1677 waveform times. The Sticky backend measures ~0.4 s for the
 * fast (differential 0xFC) refresh; the full one, run once after
 * power-on, re-drives every pixel and takes several times longer. The
 * frame transfer to both RAM planes over SPI comes on top. */
#define EPD_FAST_MS   400
#define EPD_FULL_MS   1500
#define EPD_SPI_MS    30

static uint8_t *s_fb;      /* one byte per pixel: 0 = black, 1 = white */
static uint8_t *s_shown;   /* the frame handed to the panel last */
static int s_w, s_h;
static int s_x0 = -1, s_y0, s_x1, s_y1;                  /* dirty box */
static int s_cx0 = -1, s_cy0 = -1, s_cx1 = -1, s_cy1 = -1; /* clip */
static bool s_needs_full = true;
static bool s_asleep;
static double s_busy_until;
static bool s_pending;     /* a flush came while the panel was busy */

static inline int imin(int a, int b) { return a < b ? a : b; }
static inline int imax(int a, int b) { return a > b ? a : b; }

void demo_display_epd_init(int w, int h)
{
    s_w = w;
    s_h = h;
    s_fb = (uint8_t *)malloc((size_t)w * h);
    s_shown = (uint8_t *)malloc((size_t)w * h);
    memset(s_fb, 1, (size_t)w * h);
    memset(s_shown, 1, (size_t)w * h);
}

static bool panel_busy(void)
{
    return emscripten_get_now() < s_busy_until;
}

static void mark_dirty(int x, int y, int w, int h)
{
    int x0 = imax(x, 0), y0 = imax(y, 0);
    int x1 = imin(x + w, s_w) - 1, y1 = imin(y + h, s_h) - 1;
    if (x1 < x0 || y1 < y0) return;
    if (s_x0 < 0) {
        s_x0 = x0; s_y0 = y0; s_x1 = x1; s_y1 = y1;
        return;
    }
    s_x0 = imin(s_x0, x0); s_y0 = imin(s_y0, y0);
    s_x1 = imax(s_x1, x1); s_y1 = imax(s_y1, y1);
}

void display_clear(uint8_t color)
{
    if (!s_fb) return;
    memset(s_fb, color ? 1 : 0, (size_t)s_w * s_h);
    mark_dirty(0, 0, s_w, s_h);
}

void display_set_pixel(uint16_t x, uint16_t y, uint8_t color)
{
    if (!s_fb || x >= s_w || y >= s_h) return;
    s_fb[(size_t)y * s_w + x] = color ? 1 : 0;
    mark_dirty(x, y, 1, 1);
}

bool display_push_rgb565(int x, int y, int w, int h, const void *map)
{
    if (!s_fb || !map || w <= 0 || h <= 0) return false;
    const uint16_t *src = (const uint16_t *)map;
    for (int sy = 0; sy < h; sy++) {
        int py = y + sy;
        for (int sx = 0; sx < w; sx++) {
            int px = x + sx;
            uint16_t v = src[(size_t)sy * w + sx];
            if ((unsigned)px < (unsigned)s_w && (unsigned)py < (unsigned)s_h) {
                /* epd_rgb565_is_black(): green channel below half. */
                s_fb[(size_t)py * s_w + px] = ((v >> 5) & 0x3F) < 32 ? 0 : 1;
            }
        }
    }
    mark_dirty(x, y, w, h);
    return true;
}

void display_set_partial_clip(int x, int y, int w, int h)
{
    if (w <= 0 || h <= 0) {
        s_cx0 = s_cy0 = s_cx1 = s_cy1 = -1;
        return;
    }
    int cx0 = imax(0, x), cy0 = imax(0, y);
    int cx1 = imin(s_w - 1, x + w - 1), cy1 = imin(s_h - 1, y + h - 1);
    /* Keep the union of the keys handled before the next flush. */
    if (s_cx0 >= 0) {
        cx0 = imin(cx0, s_cx0); cy0 = imin(cy0, s_cy0);
        cx1 = imax(cx1, s_cx1); cy1 = imax(cy1, s_cy1);
    }
    s_cx0 = cx0; s_cy0 = cy0; s_cx1 = cx1; s_cy1 = cy1;
}

static void panel_refresh(bool full)
{
    /* A refresh still running holds the next one back, as
     * sticky_wait_idle() does on the device; the frame is taken when
     * the panel is free (demo_display_poll()). */
    if (panel_busy()) {
        s_pending = true;
        return;
    }
    s_pending = false;
    int dur = (full ? EPD_FULL_MS : EPD_FAST_MS) + EPD_SPI_MS;
    s_busy_until = emscripten_get_now() + dur;
    memcpy(s_shown, s_fb, (size_t)s_w * s_h);
    demo_host_epd_refresh(s_shown, s_w, s_h, full, 0, dur);
}

void demo_display_poll(void)
{
    if (s_pending && !s_asleep && !panel_busy()) panel_refresh(false);
}

void display_flush(void)
{
    if (!s_fb || s_asleep || s_x0 < 0) return;
    int x0 = s_x0, y0 = s_y0, x1 = s_x1, y1 = s_y1;
    s_x0 = -1;
    if (s_cx0 >= 0 && s_cx1 >= s_cx0 && s_cy1 >= s_cy0) {
        x0 = imax(x0, s_cx0); y0 = imax(y0, s_cy0);
        x1 = imin(x1, s_cx1); y1 = imin(y1, s_cy1);
    }
    s_cx0 = s_cy0 = s_cx1 = s_cy1 = -1;
    if (x1 < x0 || y1 < y0) return;
    panel_refresh(s_needs_full);
    s_needs_full = false;
}

void display_full_refresh(void)
{
    s_cx0 = s_cy0 = s_cx1 = s_cy1 = -1;
    mark_dirty(0, 0, s_w, s_h);
    display_flush();
}

/* The Sticky never does a full refresh after the first one. */
void display_request_full_refresh(void) { }

/* E-paper keeps its picture without power. */
void display_deep_sleep_prepare(void) { s_asleep = true; }

/* No front-light on this board. */
void display_set_backlight(int percent) { (void)percent; }

const char *display_controller_name(void) { return "SSD1677 (simulated)"; }

#endif
