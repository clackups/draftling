#include "sdkconfig.h"
#if defined(CONFIG_DRAFTLING_DISPLAY_RETERMINAL_STICKY)

/*
 * Seeed Studio reTerminal Sticky SPI e-paper backend.
 *
 * https://www.seeedstudio.com/sticky/docs/en/device-guide/hardware-overview/
 * -- a 3.97" 800x480 black/white e-paper panel driven directly over
 * SPI. Pin numbers come from Seeed's own hardware-overview page and
 * the FreeInk SDK's `STICKY` BoardProfile
 * (https://github.com/Free-Ink/freeink-sdk, MIT licensed), which
 * agree; see main/boards/seeed_reterminal_sticky.h.
 *
 * Panel: 800x480, no mirror. SPI: SCLK=13, MOSI=14, MISO=12, CS=15,
 * DC=16, RST=17, BUSY=18, panel power-enable EP_PWR_EN=47
 * (active-high plain GPIO, no PMIC). The bus is shared with the
 * MicroSD card (CS=8), which main.cpp powers and deselects before
 * display_init() runs: with the card unpowered, the panel never
 * received a usable image. No front-light.
 *
 * Controller: Sticky units ship with either an SSD1677 or an SSD2677
 * (Seeed_GFX2's Driver_Sticky_Auto); sticky_detect_controller() tells
 * them apart by BUSY polarity after reset.
 *
 *   - SSD1677 (tested on physical hardware): the same reset timing,
 *     booster, borders and 0xFC refresh sequences as
 *     display_ws_epd397.cpp (full refresh = RED loaded with the
 *     inverse frame), the standard SSD1677 method in AGENTS.md. It
 *     ghosts much less here than the sticky-micronotes sequences
 *     (full 0xF7, fast 0xFF) used before. An earlier 0xFC attempt
 *     showed nothing, but the panel was blank then anyway because of
 *     the unpowered SD card.
 *   - SSD2677 (untested, no such unit available): ported from
 *     Seeed_GFX2's Driver_SSD2677.cpp; see the section below.
 *
 * SPI clock: a conservative 10 MHz (the vendor peripheral demo's
 * value) because the bus is shared with the SD card.
 */

#include <algorithm>
#include <cstring>

#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_heap_caps.h>
#include <esp_lcd_panel_io.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "display.h"
#include "display_margins.h"

static const char *TAG = "DisplayReterminalSticky";

/* ---- Panel geometry ---- */
#define PANEL_WIDTH         800
#define PANEL_HEIGHT        480
#define PANEL_WIDTH_BYTES   (PANEL_WIDTH / 8)
#define FRAMEBUFFER_BYTES   (PANEL_WIDTH_BYTES * PANEL_HEIGHT)

/* ---- Pins. This backend is used by exactly one board, so the pins
 * are hard-coded here rather than threaded through a board header --
 * matching display_ws_epd397.cpp / display_ili9341.cpp /
 * display_xteink_epd.cpp. MISO is wired (unlike display_ws_epd397.cpp)
 * because the MicroSD card shares this SPI bus -- see
 * main/boards/seeed_reterminal_sticky.h. ---- */
#define EPD_SCLK_PIN         13
#define EPD_MOSI_PIN         14
#define EPD_MISO_PIN         12
#define EPD_CS_PIN           15
#define EPD_DC_PIN           16
#define EPD_RST_PIN          17
#define EPD_BUSY_PIN         18
#define EPD_PWR_EN_PIN       47

#define STICKY_SPI_HOST      SPI2_HOST
/* See the file header comment: conservative 10 MHz, matching the
 * vendor peripheral demo, because this bus is shared with the SD
 * card. Revisit once real hardware confirms the bus tolerates more. */
#define STICKY_SPI_CLOCK_HZ  (10 * 1000 * 1000)

/* On enclosures whose cover overlaps the panel (user-adjustable via
 * Settings -> Screen margins, zero by default -- see
 * display_margins.h), every incoming coordinate (from LVGL, already
 * rendering at the margin-shrunk DISPLAY_LOGICAL_WIDTH/HEIGHT -- see
 * app_config.h) is offset by the left/top margin before it is
 * written into the physical panel framebuffer. */
#define EPD_MARGIN_LEFT      display_margin_left()
#define EPD_MARGIN_TOP       display_margin_top()

#ifdef CONFIG_DRAFTLING_EPD_FULL_REFRESH_INTERVAL
#define STICKY_FULL_REFRESH_INTERVAL CONFIG_DRAFTLING_EPD_FULL_REFRESH_INTERVAL
#else
#define STICKY_FULL_REFRESH_INTERVAL 30
#endif

static esp_lcd_panel_io_handle_t s_io = NULL;
static uint8_t  *s_fb = NULL;
/* SSD1677: inverted frame for a full refresh, see
 * sticky_display(). SSD2677: the previous frame (s_prev). */
static uint8_t  *s_fb_inv = NULL;
static bool      s_initialized = false;
/* SSD1677: copy of the frame being shown. A refresh is started
 * without waiting for it to finish (see sticky_refresh()), so LVGL can
 * keep changing s_fb meanwhile; RED RAM is re-synced from this copy
 * once the panel is idle again. */
static uint8_t  *s_shown = NULL;
static bool      s_refresh_pending = false;
static bool      s_red_stale = false;

/* Sticky units ship with one of two panel controllers, mixed in
 * production (Seeed_GFX2's Driver_Sticky_Auto): an SSD1677, or an
 * SSD2677 -- an UltraChip-style command set with 2-bit pixel data and
 * no RAM windows. Both sit on identical wiring but drive BUSY with
 * opposite polarity (SSD1677 busy HIGH, SSD2677 busy LOW), which is
 * also how sticky_detect_controller() tells them apart. */
typedef enum {
    STICKY_CTRL_SSD1677,
    STICKY_CTRL_SSD2677,
} sticky_ctrl_t;
static sticky_ctrl_t s_ctrl = STICKY_CTRL_SSD1677;

/* SSD2677 only: the frame currently on the glass (the "old" half of
 * each fast-refresh transition pair), the 2bpp transfer buffer, the
 * waveform currently latched (0 = none) and whether the panel's
 * driving voltages are on (fast refreshes leave them on). */
#define SSD2677_TX_BYTES     (FRAMEBUFFER_BYTES * 2)
static uint8_t  *s_prev = NULL;
static uint8_t  *s_tx = NULL;
static uint8_t   s_ssd2677_latched = 0;
static bool      s_ssd2677_powered = false;
static bool      s_needs_initial_full = true;
static bool      s_force_full = true;
static int       s_partial_count = 0;
static int       s_width = PANEL_WIDTH;
static int       s_height = PANEL_HEIGHT;

static int s_dirty_x0 = -1, s_dirty_y0 = -1, s_dirty_x1 = -1, s_dirty_y1 = -1;
static int s_clip_x0 = -1, s_clip_y0 = -1, s_clip_x1 = -1, s_clip_y1 = -1;
/* See display_ws_epd397.cpp's comment on the equivalent field: the
 * sum of pushed-region areas, not the bounding box's area, drives the
 * fast-vs-full waveform choice in display_flush(). */
static long s_dirty_area_sum = 0;

static inline void clear_dirty(void)
{
    s_dirty_x0 = s_dirty_y0 = s_dirty_x1 = s_dirty_y1 = -1;
    s_dirty_area_sum = 0;
}

static inline void mark_dirty_rect(int x, int y, int w, int h)
{
    if (w <= 0 || h <= 0) return;
    int x0 = std::max(0, x);
    int y0 = std::max(0, y);
    int x1 = std::min(s_width,  x + w);
    int y1 = std::min(s_height, y + h);
    if (x1 <= x0 || y1 <= y0) return;
    s_dirty_area_sum += (long)(x1 - x0) * (long)(y1 - y0);
    if (s_dirty_x0 < 0) {
        s_dirty_x0 = x0; s_dirty_y0 = y0;
        s_dirty_x1 = x1 - 1; s_dirty_y1 = y1 - 1;
        return;
    }
    s_dirty_x0 = std::min(s_dirty_x0, x0);
    s_dirty_y0 = std::min(s_dirty_y0, y0);
    s_dirty_x1 = std::max(s_dirty_x1, x1 - 1);
    s_dirty_y1 = std::max(s_dirty_y1, y1 - 1);
}

static inline void set_panel_pixel(int x, int y, bool black)
{
    if ((unsigned)x >= (unsigned)s_width || (unsigned)y >= (unsigned)s_height) return;
    size_t idx = (size_t)y * PANEL_WIDTH_BYTES + (size_t)(x >> 3);
    uint8_t mask = (uint8_t)(0x80U >> (x & 7));
    if (black) s_fb[idx] &= (uint8_t)~mask;
    else       s_fb[idx] |= mask;
}

static void fill_panel_rect(int x, int y, int w, int h, bool black)
{
    if (!s_fb || w <= 0 || h <= 0) return;
    int x0 = std::max(0, x);
    int y0 = std::max(0, y);
    int x1 = std::min(s_width, x + w);
    int y1 = std::min(s_height, y + h);
    if (x1 <= x0 || y1 <= y0) return;

    if (x0 == 0 && x1 == s_width) {
        for (int py = y0; py < y1; ++py) {
            memset(s_fb + (size_t)py * PANEL_WIDTH_BYTES, black ? 0x00 : 0xFF, PANEL_WIDTH_BYTES);
        }
        return;
    }
    for (int py = y0; py < y1; ++py) {
        for (int px = x0; px < x1; ++px) set_panel_pixel(px, py, black);
    }
}

static inline bool rgb565_is_black(uint16_t v)
{
    return ((v >> 5) & 0x3F) < 32;
}

/* ---- SPI command/data framing (esp_lcd_panel_io_spi) -- same
 * pattern as display_ws_epd397.cpp's epd_cmd()/epd_data1()/epd_data(). */
static inline void epd_cmd(uint8_t c)
{
    esp_lcd_panel_io_tx_param(s_io, c, NULL, 0);
}

static inline void epd_data1(uint8_t d)
{
    esp_lcd_panel_io_tx_param(s_io, -1, &d, 1);
}

static inline void epd_data(const uint8_t *d, size_t n)
{
    esp_lcd_panel_io_tx_color(s_io, -1, d, n);
}

/* Busy while HIGH on the SSD1677, while LOW on the SSD2677. */
/* Returns false (and logs) when BUSY did not clear within timeout_ms. */
static bool epd_wait_busy_ms(int timeout_ms)
{
    int busy_level = (s_ctrl == STICKY_CTRL_SSD2677) ? 0 : 1;
    int64_t start = esp_timer_get_time();
    while (gpio_get_level((gpio_num_t)EPD_BUSY_PIN) == busy_level) {
        vTaskDelay(pdMS_TO_TICKS(1));
        if (esp_timer_get_time() - start > (int64_t)timeout_ms * 1000) {
            ESP_LOGW(TAG, "BUSY still active after %d ms", timeout_ms);
            return false;
        }
    }
    return true;
}

static void epd_wait_busy(void)
{
    (void)epd_wait_busy_ms(30 * 1000);
}

static void epd_reset_pulse(void)
{
    gpio_set_level((gpio_num_t)EPD_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level((gpio_num_t)EPD_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level((gpio_num_t)EPD_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
}

/* ============================================================
 * SSD1677 register sequence (dual-RAM BW=0x24 / RED=0x26 differential
 * refresh) -- same as display_ws_epd397.cpp.
 * ============================================================ */

static void sticky_set_ram_area_full(void)
{
    /* Data-entry: X increment, Y decrement (no mirror). Gates are
     * addressed from the bottom, so the Y window is computed from
     * the bottom -- y_win is 0 for the full-frame window. */
    int y = 0, h = PANEL_HEIGHT, w = PANEL_WIDTH;
    int y_win = PANEL_HEIGHT - y - h;

    epd_cmd(0x11); epd_data1(0x01); /* DATA_ENTRY_MODE */

    epd_cmd(0x44); /* SET_RAM_X_RANGE */
    epd_data1(0x00); epd_data1(0x00);
    epd_data1((uint8_t)(((w - 1)) & 0xFF)); epd_data1((uint8_t)(((w - 1) >> 8) & 0xFF));

    epd_cmd(0x45); /* SET_RAM_Y_RANGE */
    epd_data1((uint8_t)((y_win + h - 1) & 0xFF)); epd_data1((uint8_t)(((y_win + h - 1) >> 8) & 0xFF));
    epd_data1((uint8_t)(y_win & 0xFF)); epd_data1((uint8_t)((y_win >> 8) & 0xFF));

    epd_cmd(0x4E); epd_data1(0x00); epd_data1(0x00); /* SET_RAM_X_COUNTER */

    epd_cmd(0x4F); /* SET_RAM_Y_COUNTER */
    epd_data1((uint8_t)((y_win + h - 1) & 0xFF)); epd_data1((uint8_t)(((y_win + h - 1) >> 8) & 0xFF));
}

static void sticky_ctrl_init(void)
{
    static const uint8_t booster[5] = { 0xAE, 0xC7, 0xC3, 0xC0, 0x80 };

    epd_cmd(0x12); /* SOFT_RESET */
    vTaskDelay(pdMS_TO_TICKS(10));
    epd_wait_busy();

    epd_cmd(0x18); epd_data1(0x80); /* TEMP_SENSOR_CONTROL: internal */

    epd_cmd(0x0C); /* BOOSTER_SOFT_START */
    for (uint8_t b : booster) epd_data1(b);

    epd_cmd(0x01); /* DRIVER_OUTPUT_CONTROL: height - 1, scan byte */
    epd_data1((uint8_t)((PANEL_HEIGHT - 1) & 0xFF));
    epd_data1((uint8_t)(((PANEL_HEIGHT - 1) >> 8) & 0xFF));
    epd_data1(0x02);

    epd_cmd(0x3C); epd_data1(0x80); /* BORDER_WAVEFORM init value */

    sticky_set_ram_area_full();

    epd_cmd(0x46); epd_data1(0xF7); epd_wait_busy(); /* AUTO_WRITE_BW_RAM */
    epd_cmd(0x47); epd_data1(0xF7); epd_wait_busy(); /* AUTO_WRITE_RED_RAM */
}

/* Both full and fast refreshes use CTRL1 normal, border 0xC0 and the
 * 0xFC differential sequence, as display_ws_epd397.cpp does; a full
 * refresh differs only in loading RED with the inverse of the new
 * frame (see sticky_display()). */
/* Starts the refresh and returns without waiting for it: the
 * waveform takes ~0.4 s, and blocking here would block the LVGL task
 * (this runs from its flush callback), so touch input would not be
 * polled and the second tap of a double-tap would be lost.
 * sticky_wait_idle() waits before the next command. */
static void sticky_refresh(void)
{
    epd_cmd(0x21); epd_data1(0x00); /* DISPLAY_UPDATE_CTRL1 */
    epd_cmd(0x3C); epd_data1(0xC0); /* BORDER_WAVEFORM, all modes */
    epd_cmd(0x22); epd_data1(0xFC); /* DISPLAY_UPDATE_CTRL2 */
    epd_cmd(0x20);                  /* MASTER_ACTIVATION */
    vTaskDelay(pdMS_TO_TICKS(2));   /* let BUSY rise */
    s_refresh_pending = true;
    s_red_stale = true;
}

/* Waits for a refresh started by sticky_refresh() to finish, then
 * re-syncs both planes to the frame now shown so the next fast
 * refresh diffs against a clean baseline (display_ws_epd397.cpp's
 * post-sync). */
static void sticky_wait_idle(void)
{
    if (s_refresh_pending) {
        epd_wait_busy();
        s_refresh_pending = false;
    }
    if (s_red_stale) {
        sticky_set_ram_area_full();
        epd_cmd(0x24); epd_data(s_shown, FRAMEBUFFER_BYTES);
        epd_cmd(0x26); epd_data(s_shown, FRAMEBUFFER_BYTES);
        s_red_stale = false;
    }
}

static void sticky_display(const uint8_t *fb, bool full)
{
    sticky_wait_idle();

    /* sticky_wait_idle() may have queued writes from s_shown; this
     * sends commands, which waits for the queued transfers to finish,
     * so s_shown is free to overwrite below. */
    sticky_set_ram_area_full();

    /* Snapshot the frame: the SPI transfers below are queued, and LVGL
     * may change fb again once this returns. The last command sent
     * here (in sticky_refresh()) drains the queue, so s_shown and
     * s_fb_inv are no longer in use by then. */
    memcpy(s_shown, fb, FRAMEBUFFER_BYTES);

    epd_cmd(0x24); epd_data(s_shown, FRAMEBUFFER_BYTES); /* WRITE_RAM_BW = new frame */
    if (full) {
        /* RED = inverse of the new frame, so every pixel reads as
         * changed and is re-driven by the differential waveform. */
        for (size_t i = 0; i < FRAMEBUFFER_BYTES; ++i) {
            s_fb_inv[i] = (uint8_t)~s_shown[i];
        }
        epd_cmd(0x26); epd_data(s_fb_inv, FRAMEBUFFER_BYTES); /* WRITE_RAM_RED */
    }
    /* Fast: RED already holds the previous frame (sticky_wait_idle()). */
    sticky_refresh();
}

/* ============================================================
 * SSD2677 register sequence, ported from Seeed_GFX2's
 * Driver_SSD2677.cpp (aligned with the Sticky product firmware's
 * seeed_epaper/driver/ssd2677.c). Pixels are sent through DTM1 (0x10)
 * as 2-bit codes, 0b00 = black, 0b11 = white. A full refresh sends the
 * new frame; a fast refresh sends (old << 1 | new) transition pairs
 * under the partial waveform, so unchanged pixels are not driven. The
 * controller has no window registers: every refresh sends the whole
 * frame. Each row is sent mirrored, as Seeed's SSD2677 config does.
 * ============================================================ */

/* Seeed falls back to 25 C whenever the panel's temperature cannot be
 * read back (MISO is shared with the SD card and not wired to the
 * panel SDO on every unit); this driver never reads it. */
#define SSD2677_WAVEFORM_FULL     0xEE  /* 21-30 C full-refresh bucket */
#define SSD2677_WAVEFORM_PARTIAL  0x19  /* 21-30 C partial bucket */

static inline uint8_t reverse_bits8(uint8_t b)
{
    b = (uint8_t)(((b & 0xF0) >> 4) | ((b & 0x0F) << 4));
    b = (uint8_t)(((b & 0xCC) >> 2) | ((b & 0x33) << 2));
    b = (uint8_t)(((b & 0xAA) >> 1) | ((b & 0x55) << 1));
    return b;
}

/* Expand 8 mono pixels (bit 1 = white, MSB leftmost) into two bytes of
 * 2-bit pairs; with `prev` set, each pair is (old << 1 | new). */
static inline void ssd2677_pack(uint8_t prev, uint8_t cur, bool diff,
                                uint8_t *o0, uint8_t *o1)
{
    uint16_t out = 0;
    for (int bit = 0; bit < 8; ++bit) {
        uint8_t n = (uint8_t)((cur >> (7 - bit)) & 1);
        uint8_t pair = diff ? (uint8_t)((((prev >> (7 - bit)) & 1) << 1) | n)
                            : (uint8_t)(n ? 0x03 : 0x00);
        out |= (uint16_t)pair << (14 - bit * 2);
    }
    *o0 = (uint8_t)(out >> 8);
    *o1 = (uint8_t)(out & 0xFF);
}

static void ssd2677_send_frame(const uint8_t *fb, const uint8_t *prev)
{
    uint8_t *o = s_tx;
    for (int row = 0; row < PANEL_HEIGHT; ++row) {
        const uint8_t *cur_row  = fb + (size_t)row * PANEL_WIDTH_BYTES;
        const uint8_t *prev_row = prev ? prev + (size_t)row * PANEL_WIDTH_BYTES : NULL;
        for (int i = PANEL_WIDTH_BYTES - 1; i >= 0; --i) {
            uint8_t c = reverse_bits8(cur_row[i]);
            uint8_t p = prev_row ? reverse_bits8(prev_row[i]) : 0;
            ssd2677_pack(p, c, prev_row != NULL, &o[0], &o[1]);
            o += 2;
        }
    }
    epd_cmd(0x10); /* DTM1 */
    epd_data(s_tx, SSD2677_TX_BYTES);
}

static void ssd2677_latch_waveform(uint8_t waveform)
{
    if (s_ssd2677_latched == waveform) return;
    epd_cmd(0xE0); epd_data1(0x12);
    epd_cmd(0xE6); epd_data1(waveform);
    epd_cmd(0xA5);
    epd_wait_busy();
    vTaskDelay(pdMS_TO_TICKS(10));
    s_ssd2677_latched = waveform;
}

static void ssd2677_power_on(void)
{
    if (s_ssd2677_powered) return;
    epd_cmd(0x04); /* PON */
    epd_wait_busy();
    s_ssd2677_powered = true;
}

static void ssd2677_power_off(void)
{
    if (!s_ssd2677_powered) return;
    epd_cmd(0x02); epd_data1(0x00); /* POF */
    epd_wait_busy();
    s_ssd2677_powered = false;
}

static void ssd2677_ctrl_init(void)
{
    static const uint8_t timing[8] = { 0x76, 0x76, 0x76, 0x5A, 0x9D, 0x8A, 0x76, 0x62 };

    epd_wait_busy();
    epd_cmd(0x00); epd_data1(0x2F); epd_data1(0x0E); /* PSR */
    epd_wait_busy();
    epd_cmd(0x06); /* booster */
    epd_data1(0x0F); epd_data1(0x8B); epd_data1(0x93); epd_data1(0xC1);
    epd_cmd(0xE7); epd_data1(0xC1);
    epd_cmd(0x30); epd_data1(0x08); /* PLL */
    epd_cmd(0x50); epd_data1(0x77); /* CDI */
    epd_cmd(0x62); /* timing */
    for (uint8_t b : timing) epd_data1(b);
    epd_cmd(0x61); /* RES = 800x680 (680 gate lines scanned, 480 visible) */
    epd_data1(0x03); epd_data1(0x20); epd_data1(0x02); epd_data1(0xA8);
    epd_cmd(0xE0); epd_data1(0x10);
    epd_cmd(0x65); /* GSST */
    epd_data1(0x00); epd_data1(0x00); epd_data1(0x00); epd_data1(0x00);
    epd_cmd(0xE9); epd_data1(0x01);
    epd_wait_busy();

    s_ssd2677_latched = 0;
    s_ssd2677_powered = false;
}

static void ssd2677_display_full(const uint8_t *fb)
{
    ssd2677_latch_waveform(SSD2677_WAVEFORM_FULL);
    ssd2677_send_frame(fb, NULL);
    ssd2677_power_on();
    epd_cmd(0x12); epd_data1(0x00); /* DRF */
    epd_wait_busy();
    ssd2677_power_off();
    memcpy(s_prev, fb, FRAMEBUFFER_BYTES);
}

static void ssd2677_display_fast(const uint8_t *fb)
{
    ssd2677_latch_waveform(SSD2677_WAVEFORM_PARTIAL);
    ssd2677_send_frame(fb, s_prev);
    /* The firmware keeps the panel powered across consecutive partial
     * refreshes; the next full refresh or deep sleep powers it off. */
    ssd2677_power_on();
    epd_cmd(0x12); epd_data1(0x00); /* DRF */
    epd_wait_busy();
    memcpy(s_prev, fb, FRAMEBUFFER_BYTES);
}

static void ssd2677_deep_sleep(void)
{
    ssd2677_power_off();
    epd_cmd(0x07); epd_data1(0xA5); /* DSLP */
}

/* Seeed_GFX2 Driver_Sticky_Auto::probeByBusyPolarity(): after a reset
 * each controller is busy in its own polarity and then settles at its
 * ready level (SSD1677 ready LOW, SSD2677 ready HIGH), so the first
 * BUSY transition -- or the level once it has been stable for 200 ms
 * -- names the chip. (Seeed's first-stage SPI read of command 0x70 is
 * skipped: MISO is not wired to the panel SDO on every unit.) */
static sticky_ctrl_t sticky_detect_controller(void)
{
    gpio_set_level((gpio_num_t)EPD_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(1));
    gpio_set_level((gpio_num_t)EPD_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level((gpio_num_t)EPD_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(10));

    int level = gpio_get_level((gpio_num_t)EPD_BUSY_PIN);
    int stable = 1;
    int64_t start = esp_timer_get_time();
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(2));
        int now = gpio_get_level((gpio_num_t)EPD_BUSY_PIN);
        if (now != level) { level = now; break; }
        if (++stable >= 100) break;
        if (esp_timer_get_time() - start >= 1500 * 1000) break;
    }
    return level ? STICKY_CTRL_SSD2677 : STICKY_CTRL_SSD1677;
}

/* The 0xFC sequence leaves analog and clock on, so power them off
 * with a 0x03 activation before DEEP_SLEEP, as display_ws_epd397.cpp
 * does. The BUSY waits are bounded: a 30 s hang here once left the
 * wake pin LOW when standby armed the wake source. */
static void sticky_deep_sleep(void)
{
    if (s_refresh_pending) {
        (void)epd_wait_busy_ms(3000);
        s_refresh_pending = false;
    }
    epd_cmd(0x3C); epd_data1(0x80);
    epd_cmd(0x22); epd_data1(0x03);
    epd_cmd(0x20);
    vTaskDelay(pdMS_TO_TICKS(200));
    (void)epd_wait_busy_ms(3000);

    epd_cmd(0x10); epd_data1(0x03); /* DEEP_SLEEP mode 2 */
}

/* ---- display.h public API ---- */

extern "C" void display_set_shared_i2c_bus(void *)
{
    /* No PMIC on this board's panel rail -- EPD_PWR_EN_PIN is a plain
     * GPIO, driven in display_init() below. */
}

extern "C" void display_init(int, int, int, int, int, int, int width, int height)
{
    if (s_initialized) return;

    s_fb = (uint8_t *)heap_caps_malloc(FRAMEBUFFER_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_fb) s_fb = (uint8_t *)heap_caps_malloc(FRAMEBUFFER_BYTES, MALLOC_CAP_8BIT);
    if (!s_fb) {
        ESP_LOGE(TAG, "Framebuffer allocation failed");
        return;
    }
    memset(s_fb, 0xFF, FRAMEBUFFER_BYTES);

    /* Full-refresh scratch for the SSD1677, previous frame for the
     * SSD2677 -- one buffer serves whichever controller is fitted. */
    s_fb_inv = (uint8_t *)heap_caps_malloc(FRAMEBUFFER_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_fb_inv) s_fb_inv = (uint8_t *)heap_caps_malloc(FRAMEBUFFER_BYTES, MALLOC_CAP_8BIT);
    if (!s_fb_inv) {
        ESP_LOGE(TAG, "Full-refresh scratch buffer allocation failed");
        return;
    }
    s_prev = s_fb_inv;

    s_shown = (uint8_t *)heap_caps_malloc(FRAMEBUFFER_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_shown) s_shown = (uint8_t *)heap_caps_malloc(FRAMEBUFFER_BYTES, MALLOC_CAP_8BIT);
    if (!s_shown) {
        ESP_LOGE(TAG, "Shown-frame buffer allocation failed");
        return;
    }

    s_width = PANEL_WIDTH;
    s_height = PANEL_HEIGHT;
    if (width != s_width || height != s_height) {
        ESP_LOGW(TAG, "Configured size %dx%d differs from panel %dx%d",
                 width, height, s_width, s_height);
    }

    /* Panel power-enable, active-high. Drive it before any SPI
     * traffic -- there is no PMIC sequencing dependency on this board
     * (unlike the Waveshare ESP32-S3-ePaper-3.97's AXP2101 ALDO3
     * rail), just a plain GPIO. */
    gpio_config_t pwr_cfg = {};
    pwr_cfg.intr_type    = GPIO_INTR_DISABLE;
    pwr_cfg.mode         = GPIO_MODE_OUTPUT;
    pwr_cfg.pin_bit_mask = (1ULL << EPD_PWR_EN_PIN);
    gpio_config(&pwr_cfg);
    /* A deep-sleep pad hold on this pin survives the wake reset and
     * would swallow the HIGH write below, leaving the panel unpowered. */
    gpio_hold_dis((gpio_num_t)EPD_PWR_EN_PIN);
    gpio_set_level((gpio_num_t)EPD_PWR_EN_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(10));

    /* MISO is wired (unlike display_ws_epd397.cpp) because the
     * MicroSD card shares this bus; sd_card_init_spi() later finds
     * the bus already initialized and just adds its own device with
     * its own CS. */
    spi_bus_config_t bus_cfg = {};
    bus_cfg.mosi_io_num   = EPD_MOSI_PIN;
    bus_cfg.miso_io_num   = EPD_MISO_PIN;
    bus_cfg.sclk_io_num   = EPD_SCLK_PIN;
    bus_cfg.quadwp_io_num = -1;
    bus_cfg.quadhd_io_num = -1;
    bus_cfg.max_transfer_sz = FRAMEBUFFER_BYTES;
    ESP_ERROR_CHECK(spi_bus_initialize(STICKY_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_spi_config_t io_cfg = {};
    io_cfg.dc_gpio_num       = (gpio_num_t)EPD_DC_PIN;
    io_cfg.cs_gpio_num       = (gpio_num_t)EPD_CS_PIN;
    io_cfg.pclk_hz           = STICKY_SPI_CLOCK_HZ;
    io_cfg.lcd_cmd_bits      = 8;
    io_cfg.lcd_param_bits    = 8;
    io_cfg.spi_mode          = 0;
    io_cfg.trans_queue_depth = 4;
    io_cfg.flags.psram_dma_direct = 1;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)STICKY_SPI_HOST, &io_cfg, &s_io));

    gpio_config_t rst_cfg = {};
    rst_cfg.intr_type    = GPIO_INTR_DISABLE;
    rst_cfg.mode         = GPIO_MODE_OUTPUT;
    rst_cfg.pin_bit_mask = (1ULL << EPD_RST_PIN);
    gpio_config(&rst_cfg);

    gpio_config_t busy_cfg = {};
    busy_cfg.intr_type    = GPIO_INTR_DISABLE;
    busy_cfg.mode         = GPIO_MODE_INPUT;
    busy_cfg.pin_bit_mask = (1ULL << EPD_BUSY_PIN);
    gpio_config(&busy_cfg);

    s_ctrl = sticky_detect_controller();
    if (s_ctrl == STICKY_CTRL_SSD2677) {
        s_tx = (uint8_t *)heap_caps_malloc(SSD2677_TX_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!s_tx) {
            ESP_LOGE(TAG, "SSD2677 transfer buffer allocation failed");
            return;
        }
        /* Seeed's SSD2677 reset timing: low 20 ms, then 50 ms settle. */
        gpio_set_level((gpio_num_t)EPD_RST_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(20));
        gpio_set_level((gpio_num_t)EPD_RST_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(50));
        ssd2677_ctrl_init();
        memset(s_prev, 0xFF, FRAMEBUFFER_BYTES);
    } else {
        epd_reset_pulse();
        sticky_ctrl_init();
    }

    s_needs_initial_full = true;
    s_force_full = true;
    s_partial_count = 0;
    clear_dirty();
    s_initialized = true;

    ESP_LOGI(TAG, "Seeed reTerminal Sticky e-paper initialized (%dx%d, %s)",
             s_width, s_height,
             s_ctrl == STICKY_CTRL_SSD2677 ? "SSD2677" : "SSD1677");
}

extern "C" void display_clear(uint8_t color)
{
    if (!s_initialized || !s_fb) return;
    memset(s_fb, color ? 0xFF : 0x00, FRAMEBUFFER_BYTES);
    mark_dirty_rect(0, 0, s_width, s_height);
    s_force_full = true;
}

extern "C" void display_set_pixel(uint16_t x, uint16_t y, uint8_t color)
{
    if (!s_initialized || !s_fb) return;
    int px = (int)x + EPD_MARGIN_LEFT;
    int py = (int)y + EPD_MARGIN_TOP;
    fill_panel_rect(px, py, 1, 1, color == 0);
    mark_dirty_rect(px, py, 1, 1);
}

extern "C" bool display_push_rgb565(int x, int y, int w, int h, const void *color_map)
{
    if (!s_initialized || !s_fb || !color_map || w <= 0 || h <= 0) return false;
    int ox = x + EPD_MARGIN_LEFT;
    int oy = y + EPD_MARGIN_TOP;
    const uint16_t *src = (const uint16_t *)color_map;
    for (int sy = 0; sy < h; ++sy) {
        for (int sx = 0; sx < w; ++sx) {
            bool black = rgb565_is_black(src[(size_t)sy * w + sx]);
            set_panel_pixel(ox + sx, oy + sy, black);
        }
    }
    mark_dirty_rect(ox, oy, w, h);
    return true;
}

extern "C" void display_set_partial_clip(int x, int y, int w, int h)
{
    if (w <= 0 || h <= 0) {
        s_clip_x0 = s_clip_y0 = s_clip_x1 = s_clip_y1 = -1;
        return;
    }
    int ox = x + EPD_MARGIN_LEFT;
    int oy = y + EPD_MARGIN_TOP;
    int cx0 = std::max(0, ox);
    int cy0 = std::max(0, oy);
    int cx1 = std::min(s_width  - 1, ox + w - 1);
    int cy1 = std::min(s_height - 1, oy + h - 1);
    /* Several keys can be handled before the next flush, each setting
     * a clip for its own edit. Keep the union, or the last key's clip
     * drops the earlier edits (two quick Backspaces left part of the
     * first erased letter on the panel). */
    if (s_clip_x0 >= 0) {
        cx0 = std::min(cx0, s_clip_x0);
        cy0 = std::min(cy0, s_clip_y0);
        cx1 = std::max(cx1, s_clip_x1);
        cy1 = std::max(cy1, s_clip_y1);
    }
    s_clip_x0 = cx0;
    s_clip_y0 = cy0;
    s_clip_x1 = cx1;
    s_clip_y1 = cy1;
}

extern "C" void display_flush(void)
{
    if (!s_initialized || !s_fb) return;
    if (s_dirty_x0 < 0) return;

    int x0 = s_dirty_x0, y0 = s_dirty_y0, x1 = s_dirty_x1, y1 = s_dirty_y1;
    long dirty_area_sum = s_dirty_area_sum;
    clear_dirty();

    if (s_clip_x0 >= 0 && s_clip_y0 >= 0 && s_clip_x1 >= s_clip_x0 && s_clip_y1 >= s_clip_y0) {
        x0 = std::max(x0, s_clip_x0);
        y0 = std::max(y0, s_clip_y0);
        x1 = std::min(x1, s_clip_x1);
        y1 = std::min(y1, s_clip_y1);
        s_clip_x0 = s_clip_y0 = s_clip_x1 = s_clip_y1 = -1;
    }

    if (x1 < x0 || y1 < y0) {
        s_force_full = false;
        s_clip_x0 = s_clip_y0 = s_clip_x1 = s_clip_y1 = -1;
        return;
    }

    /* "huge" drives the fast-vs-full waveform choice below, so it
     * should reflect how much content actually changed -- use the sum
     * of the individual pushed regions (dirty_area_sum), not the area
     * of the bounding box that merely encloses them (x0..y1). See the
     * comment on s_dirty_area_sum. */
    bool huge = dirty_area_sum * 4 > (long)s_width * s_height * 3;
    bool do_full = s_needs_initial_full || s_force_full || huge ||
                   s_partial_count >= STICKY_FULL_REFRESH_INTERVAL;

    if (do_full) {
        if (s_ctrl == STICKY_CTRL_SSD2677) ssd2677_display_full(s_fb);
        else                               sticky_display(s_fb, true);
        s_partial_count = 0;
        s_needs_initial_full = false;
    } else {
        if (s_ctrl == STICKY_CTRL_SSD2677) ssd2677_display_fast(s_fb);
        else                               sticky_display(s_fb, false);
        s_partial_count++;
    }

    s_clip_x0 = s_clip_y0 = s_clip_x1 = s_clip_y1 = -1;
    s_force_full = false;
}

extern "C" void display_full_refresh(void)
{
    if (!s_initialized) return;
    s_force_full = true;
    s_clip_x0 = s_clip_y0 = s_clip_x1 = s_clip_y1 = -1;
    mark_dirty_rect(0, 0, s_width, s_height);
    display_flush();
}

extern "C" void display_request_full_refresh(void)
{
    if (!s_initialized) return;
    s_force_full = true;
}

extern "C" uint8_t *display_get_buffer(void)
{
    return s_fb;
}

extern "C" int display_get_buffer_size(void)
{
    return FRAMEBUFFER_BYTES;
}

extern "C" void display_sleep(void)
{
    /* E-paper retains its image without power; nothing to do. */
}

extern "C" void display_wake(void)
{
}

extern "C" void display_set_backlight(int /*percent*/)
{
    /* No front-light on this board. */
}

extern "C" void display_deep_sleep_prepare(void)
{
    if (s_initialized) {
        if (s_ctrl == STICKY_CTRL_SSD2677) ssd2677_deep_sleep();
        else                               sticky_deep_sleep();
    }
    s_initialized = false;
}

#endif /* CONFIG_DRAFTLING_DISPLAY_RETERMINAL_STICKY */
