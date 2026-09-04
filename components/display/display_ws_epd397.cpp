#include "sdkconfig.h"
#if defined(CONFIG_DRAFTLING_DISPLAY_WS_EPD397)

/*
 * Waveshare ESP32-S3-ePaper-3.97 SPI e-paper backend.
 *
 * https://docs.waveshare.com/ESP32-S3-ePaper-3.97 -- a 3.97" 800x480
 * black/white e-paper panel driven directly over SPI by a single
 * panel controller (unlike the Xteink X4 Pro, this board has no
 * manufacturing-run variance to probe for at boot). Pin numbers and
 * the AXP2101 PMIC register map below are facts read out of
 * Waveshare's official ESP-IDF example
 * (github.com/waveshareteam/ESP32-S3-ePaper-3.97); that repository
 * carries no LICENSE file / license grant, so none of its source was
 * copied -- the register sequence implemented here is this repo's
 * own code, following the standard SSD1677-family differential-
 * refresh protocol already used by
 * components/display/display_xteink_epd.cpp's ssd1677_* functions
 * (dual-RAM 0x24/0x26 writes, 0x22/0x20 update control / master
 * activation, busy-while-HIGH). This board has NOT been tested on
 * physical hardware.
 *
 * Panel: 800x480, no mirror. SPI: SCLK=11, MOSI=12, CS=10, DC=9,
 * RST=46, BUSY=3, no MISO. No front-light (display_set_backlight()
 * is a no-op).
 *
 * Power-sequencing note: this panel's analog supply rail is switched
 * by the on-board AXP2101 PMIC's ALDO3 LDO output rather than a
 * plain GPIO (confirmed from the vendor example's EPD_Power_ON() /
 * EPD_Power_OFF(), which gate the panel via the PMIC before/after
 * every display bring-up). ALDO3 must therefore be enabled over I2C
 * *before* any SPI command reaches the panel, or it simply never
 * responds. display_set_shared_i2c_bus() -- which main.cpp calls
 * before display_init(), see the CONFIG_DRAFTLING_DISPLAY_WS_EPD397
 * arm of the shared-I2C-bus condition there -- is the hook used to do
 * this: it calls battery_axp2101_enable_display_rail() (components/
 * battery/battery.cpp) immediately, ahead of the normal
 * battery_init_axp2101() call later in boot.
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

#include "battery.h"
#include "display.h"
#include "display_margins.h"

static const char *TAG = "DisplayWsEpd397";

/* ---- Panel geometry ---- */
#define PANEL_WIDTH         800
#define PANEL_HEIGHT        480
#define PANEL_WIDTH_BYTES   (PANEL_WIDTH / 8)
#define FRAMEBUFFER_BYTES   (PANEL_WIDTH_BYTES * PANEL_HEIGHT)

/* ---- Pins. This backend is used by exactly one board, so the pins
 * are hard-coded here rather than threaded through a board header --
 * matching display_ili9341.cpp / display_ssd1683.cpp /
 * display_xteink_epd.cpp. ---- */
#define EPD_SCLK_PIN         11
#define EPD_MOSI_PIN         12
#define EPD_CS_PIN           10
#define EPD_DC_PIN           9
#define EPD_RST_PIN          46
#define EPD_BUSY_PIN         3

#define WS_EPD397_SPI_HOST   SPI2_HOST
/* Conservative starting point. The Xteink X4 Pro's SSD1677-family
 * backend found 20 MHz produced a partial/faded update on real
 * hardware (marginal SPI signal integrity on a long RAM-plane burst)
 * and settled on 5 MHz; since this board is untested on real
 * hardware too, start at the same safe value rather than assume this
 * panel/wiring tolerates more. */
#define WS_EPD397_SPI_CLOCK_HZ  (5 * 1000 * 1000)

/* On enclosures whose cover overlaps the panel (user-adjustable via
 * Settings -> Screen margins, zero by default -- see
 * display_margins.h), every incoming coordinate (from LVGL, already
 * rendering at the margin-shrunk DISPLAY_LOGICAL_WIDTH/HEIGHT -- see
 * app_config.h) is offset by the left/top margin before it is
 * written into the physical panel framebuffer. */
#define EPD_MARGIN_LEFT      display_margin_left()
#define EPD_MARGIN_TOP       display_margin_top()

#ifdef CONFIG_DRAFTLING_EPD_FULL_REFRESH_INTERVAL
#define WS_EPD397_FULL_REFRESH_INTERVAL CONFIG_DRAFTLING_EPD_FULL_REFRESH_INTERVAL
#else
#define WS_EPD397_FULL_REFRESH_INTERVAL 30
#endif

static esp_lcd_panel_io_handle_t s_io = NULL;
static uint8_t  *s_fb = NULL;
static bool      s_initialized = false;
static bool      s_needs_initial_full = true;
static bool      s_force_full = true;
static int       s_partial_count = 0;
static int       s_width = PANEL_WIDTH;
static int       s_height = PANEL_HEIGHT;
static void     *s_shared_i2c_bus = NULL;

static int s_dirty_x0 = -1, s_dirty_y0 = -1, s_dirty_x1 = -1, s_dirty_y1 = -1;
static int s_clip_x0 = -1, s_clip_y0 = -1, s_clip_x1 = -1, s_clip_y1 = -1;

static inline void clear_dirty(void)
{
    s_dirty_x0 = s_dirty_y0 = s_dirty_x1 = s_dirty_y1 = -1;
}

static inline void mark_dirty_rect(int x, int y, int w, int h)
{
    if (w <= 0 || h <= 0) return;
    int x0 = std::max(0, x);
    int y0 = std::max(0, y);
    int x1 = std::min(s_width,  x + w);
    int y1 = std::min(s_height, y + h);
    if (x1 <= x0 || y1 <= y0) return;
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
 * pattern as display_xteink_epd.cpp's epd_cmd()/epd_data1()/epd_data(). */
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

/* Busy while HIGH (SSD1677-family convention). */
static void epd_wait_busy(void)
{
    int64_t start = esp_timer_get_time();
    while (gpio_get_level((gpio_num_t)EPD_BUSY_PIN) == 1) {
        vTaskDelay(pdMS_TO_TICKS(1));
        if (esp_timer_get_time() - start > 30 * 1000 * 1000) break;
    }
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
 * SSD1677-family register sequence -- same protocol as
 * display_xteink_epd.cpp's ssd1677_* functions (dual-RAM BW=0x24 /
 * RED=0x26 differential refresh, no mirror), reimplemented here for
 * this board's own pins. See the file header comment for licensing
 * rationale.
 * ============================================================ */

static void ws_epd397_set_ram_area_full(void)
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

static void ws_epd397_ctrl_init(void)
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

    ws_epd397_set_ram_area_full();

    epd_cmd(0x46); epd_data1(0xF7); epd_wait_busy(); /* AUTO_WRITE_BW_RAM */
    epd_cmd(0x47); epd_data1(0xF7); epd_wait_busy(); /* AUTO_WRITE_RED_RAM */
}

static void ws_epd397_refresh(uint8_t ctrl1, uint8_t seq)
{
    epd_cmd(0x21); epd_data1(ctrl1);  /* DISPLAY_UPDATE_CTRL1 */
    epd_cmd(0x3C); epd_data1(0xC0);   /* BORDER_WAVEFORM, all modes */
    epd_cmd(0x22); epd_data1(seq);    /* DISPLAY_UPDATE_CTRL2 */
    epd_cmd(0x20);                    /* MASTER_ACTIVATION */
    epd_wait_busy();
}

static void ws_epd397_display_full(const uint8_t *fb)
{
    ws_epd397_set_ram_area_full();
    epd_cmd(0x24); epd_data(fb, FRAMEBUFFER_BYTES); /* WRITE_RAM_BW */
    epd_cmd(0x26); epd_data(fb, FRAMEBUFFER_BYTES); /* WRITE_RAM_RED */
    ws_epd397_refresh(0x40 /* CTRL1_BYPASS_RED */, 0xF7);
}

static void ws_epd397_display_fast(const uint8_t *fb)
{
    ws_epd397_set_ram_area_full();
    epd_cmd(0x24); epd_data(fb, FRAMEBUFFER_BYTES); /* WRITE_RAM_BW = new frame */
    /* Single-buffer mode: RED already holds the previous frame from
     * the last refresh's post-sync below, so no pre-write here. */
    ws_epd397_refresh(0x00 /* CTRL1_NORMAL */, 0xFC);

    /* Re-sync both planes to the just-shown frame so the next fast
     * refresh diffs against a clean baseline. */
    ws_epd397_set_ram_area_full();
    epd_cmd(0x24); epd_data(fb, FRAMEBUFFER_BYTES);
    epd_cmd(0x26); epd_data(fb, FRAMEBUFFER_BYTES);
}

static void ws_epd397_deep_sleep(void)
{
    epd_cmd(0x3C); epd_data1(0x80);
    epd_cmd(0x22); epd_data1(0x03);
    epd_cmd(0x20);
    vTaskDelay(pdMS_TO_TICKS(200));
    epd_wait_busy();

    epd_cmd(0x10); epd_data1(0x03); /* DEEP_SLEEP mode 2 */
}

/* ---- display.h public API ---- */

extern "C" void display_set_shared_i2c_bus(void *bus_handle)
{
    s_shared_i2c_bus = bus_handle;
    if (!bus_handle) return;

    /* Bring up the AXP2101's ALDO3 rail (the panel's analog supply)
     * NOW, before display_init() sends anything over SPI -- see the
     * file header comment. This does not select AXP2101 as the
     * active battery-monitor backend; battery_init_axp2101() (called
     * later in main.cpp's normal boot sequence) does that, reusing
     * the same cached I2C device handle. */
    if (battery_axp2101_enable_display_rail(bus_handle) != 0) {
        ESP_LOGE(TAG, "Failed to enable AXP2101 ALDO3 (e-paper panel "
                      "rail) -- panel will not respond");
    }
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

    s_width = PANEL_WIDTH;
    s_height = PANEL_HEIGHT;
    if (width != s_width || height != s_height) {
        ESP_LOGW(TAG, "Configured size %dx%d differs from panel %dx%d",
                 width, height, s_width, s_height);
    }

    spi_bus_config_t bus_cfg = {};
    bus_cfg.mosi_io_num   = EPD_MOSI_PIN;
    bus_cfg.miso_io_num   = -1;
    bus_cfg.sclk_io_num   = EPD_SCLK_PIN;
    bus_cfg.quadwp_io_num = -1;
    bus_cfg.quadhd_io_num = -1;
    bus_cfg.max_transfer_sz = FRAMEBUFFER_BYTES;
    ESP_ERROR_CHECK(spi_bus_initialize(WS_EPD397_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_spi_config_t io_cfg = {};
    io_cfg.dc_gpio_num       = (gpio_num_t)EPD_DC_PIN;
    io_cfg.cs_gpio_num       = (gpio_num_t)EPD_CS_PIN;
    io_cfg.pclk_hz           = WS_EPD397_SPI_CLOCK_HZ;
    io_cfg.lcd_cmd_bits      = 8;
    io_cfg.lcd_param_bits    = 8;
    io_cfg.spi_mode          = 0;
    io_cfg.trans_queue_depth = 4;
    io_cfg.flags.psram_dma_direct = 1;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)WS_EPD397_SPI_HOST, &io_cfg, &s_io));

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

    epd_reset_pulse();
    ws_epd397_ctrl_init();

    s_needs_initial_full = true;
    s_force_full = true;
    s_partial_count = 0;
    clear_dirty();
    s_initialized = true;

    ESP_LOGI(TAG, "Waveshare ESP32-S3-ePaper-3.97 e-paper initialized (%dx%d)",
             s_width, s_height);
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
    s_clip_x0 = std::max(0, ox);
    s_clip_y0 = std::max(0, oy);
    s_clip_x1 = std::min(s_width  - 1, ox + w - 1);
    s_clip_y1 = std::min(s_height - 1, oy + h - 1);
}

extern "C" void display_flush(void)
{
    if (!s_initialized || !s_fb) return;
    if (s_dirty_x0 < 0) return;

    int x0 = s_dirty_x0, y0 = s_dirty_y0, x1 = s_dirty_x1, y1 = s_dirty_y1;
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

    long dirty_area = (long)(x1 - x0 + 1) * (y1 - y0 + 1);
    bool huge = dirty_area * 4 > (long)s_width * s_height * 3;
    bool do_full = s_needs_initial_full || s_force_full || huge ||
                   s_partial_count >= WS_EPD397_FULL_REFRESH_INTERVAL;

    if (do_full) {
        ws_epd397_display_full(s_fb);
        s_partial_count = 0;
        s_needs_initial_full = false;
    } else {
        ws_epd397_display_fast(s_fb);
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
    if (s_initialized) ws_epd397_deep_sleep();
    s_initialized = false;
}

#endif /* CONFIG_DRAFTLING_DISPLAY_WS_EPD397 */
