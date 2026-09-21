#include "sdkconfig.h"
#if defined(CONFIG_DRAFTLING_DISPLAY_SEEED_E1001)

/*
 * Seeed Studio reTerminal E1001 SPI e-paper backend.
 *
 * 7.5-inch 800x480 monochrome e-paper display driven over SPI by the
 * UltraChip UC8179 controller (Good Display GDEY075T7 panel).
 *
 * Pin assignments:
 *   - SCLK: GPIO7
 *   - MISO: GPIO8  (shared with MicroSD)
 *   - MOSI: GPIO9  (shared with MicroSD)
 *   - CS:   GPIO10 (active low)
 *   - DC:   GPIO11
 *   - RST:  GPIO12 (active low; also gates load switch U10 power to display)
 *   - BUSY: GPIO13 (active low: 0 while busy, 1 when ready)
 *
 * Waveforms and commands match the official Seeed_GxEPD2 driver
 * (GxEPD2_750_GDEY075T7):
 *   - Full refresh: OTP LUT with TSSET 0x5A (~1.2 s)
 *   - Fast partial update: OTP fast waveform with TSSET 0x6E (~450 ms)
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

static const char *TAG = "DisplayE1001";

/* ---- Panel geometry ---- */
#define PANEL_WIDTH         800
#define PANEL_HEIGHT        480
#define PANEL_WIDTH_BYTES   (PANEL_WIDTH / 8)
#define FRAMEBUFFER_BYTES   (PANEL_WIDTH_BYTES * PANEL_HEIGHT)

/* ---- Hardware pins ---- */
#define EPD_SCLK_PIN        7
#define EPD_MISO_PIN        8
#define EPD_MOSI_PIN        9
#define EPD_CS_PIN          10
#define EPD_DC_PIN          11
#define EPD_RST_PIN         12
#define EPD_BUSY_PIN        13

/* MicroSD slot sharing the same SPI bus */
#define SD_SPI_CS_PIN       14
#define SD_EN_PIN           16

#define E1001_SPI_HOST      SPI2_HOST
/* Use 2 MHz clock matching Seeed GxEPD2 demo (SPISettings 2000000) */
#define E1001_SPI_CLOCK_HZ  (2 * 1000 * 1000)

/* Margins (Settings -> Screen margins) */
#define EPD_MARGIN_LEFT     display_margin_left()
#define EPD_MARGIN_TOP      display_margin_top()

#ifdef CONFIG_DRAFTLING_EPD_FULL_REFRESH_INTERVAL
#define E1001_FULL_REFRESH_INTERVAL CONFIG_DRAFTLING_EPD_FULL_REFRESH_INTERVAL
#else
#define E1001_FULL_REFRESH_INTERVAL 30
#endif

static esp_lcd_panel_io_handle_t s_io = NULL;
static uint8_t *s_fb = NULL;
static uint8_t *s_scratch = NULL;
static bool     s_initialized = false;
static bool     s_needs_initial_full = true;
static bool     s_force_full = true;
static bool     s_power_on = false;
static bool     s_using_partial_mode = false;
static int      s_partial_count = 0;
static int      s_width = PANEL_WIDTH;
static int      s_height = PANEL_HEIGHT;

static int s_dirty_x0 = -1, s_dirty_y0 = -1, s_dirty_x1 = -1, s_dirty_y1 = -1;
static int s_clip_x0 = -1, s_clip_y0 = -1, s_clip_x1 = -1, s_clip_y1 = -1;
static long s_dirty_area_sum = 0;

/* ---- Low-level helpers ---- */

static inline void epd_cmd(uint8_t cmd)
{
    esp_err_t err = esp_lcd_panel_io_tx_param(s_io, cmd, NULL, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "epd_cmd(0x%02X) failed: %s", cmd, esp_err_to_name(err));
    }
}

static inline void epd_data1(uint8_t data)
{
    esp_err_t err = esp_lcd_panel_io_tx_param(s_io, -1, &data, 1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "epd_data1(0x%02X) failed: %s", data, esp_err_to_name(err));
    }
}

static inline void epd_data(const uint8_t *data, size_t len)
{
    esp_err_t err = esp_lcd_panel_io_tx_color(s_io, -1, data, len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "epd_data(len=%u) failed: %s", (unsigned)len, esp_err_to_name(err));
    }
}

static void epd_wait_busy(int timeout_ms)
{
    vTaskDelay(pdMS_TO_TICKS(1));
    int64_t start = esp_timer_get_time();
    int64_t timeout_us = (int64_t)timeout_ms * 1000;
    while (gpio_get_level((gpio_num_t)EPD_BUSY_PIN) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        if (timeout_ms > 0 && (esp_timer_get_time() - start) > timeout_us) {
            ESP_LOGW(TAG, "BUSY timeout after %d ms (raw pin=%d)", timeout_ms,
                     gpio_get_level((gpio_num_t)EPD_BUSY_PIN));
            break;
        }
    }
}

static void epd_reset_pulse(void)
{
    /* Power on load switch U10 and cycle reset */
    gpio_set_level((gpio_num_t)EPD_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level((gpio_num_t)EPD_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "Post-reset BUSY pin level = %d", gpio_get_level((gpio_num_t)EPD_BUSY_PIN));
    epd_wait_busy(1000);
}

static void uc8179_power_on(void)
{
    if (!s_power_on) {
        epd_cmd(0x04); /* POWER_ON */
        epd_wait_busy(5000);
        s_power_on = true;
    }
}

static void uc8179_power_off(void)
{
    if (s_power_on) {
        epd_cmd(0x02); /* POWER_OFF */
        epd_wait_busy(5000);
        s_power_on = false;
    }
}

static void uc8179_set_partial_ram_area(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    uint16_t xe = (x + w - 1) | 0x0007; /* byte boundary inclusive */
    uint16_t ye = y + h - 1;
    x &= 0xFFF8; /* byte boundary */

    epd_cmd(0x90); /* PARTIAL_WINDOW */
    epd_data1((uint8_t)(x / 256));
    epd_data1((uint8_t)(x % 256));
    epd_data1((uint8_t)(xe / 256));
    epd_data1((uint8_t)(xe % 256));
    epd_data1((uint8_t)(y / 256));
    epd_data1((uint8_t)(y % 256));
    epd_data1((uint8_t)(ye / 256));
    epd_data1((uint8_t)(ye % 256));
    epd_data1(0x01);
}

static void uc8179_init_display(void)
{
    epd_cmd(0x00); /* PANEL_SETTING */
    epd_data1(0x1F); /* KW: 3f, KWR: 2F, BWROTP: 0f, BWOTP: 1f */

    epd_cmd(0x01); /* POWER_SETTING (same as OTP) */
    epd_data1(0x07); /* enable internal */
    epd_data1(0x07); /* VGH=20V, VGL=-20V */
    epd_data1(0x3F); /* VDH=15V */
    epd_data1(0x3F); /* VDL=-15V */
    epd_data1(0x09); /* VDHR=4.2V */

    epd_cmd(0x06); /* BOOSTER_SOFT_START */
    epd_data1(0x17);
    epd_data1(0x17);
    epd_data1(0x28);
    epd_data1(0x17);

    epd_cmd(0x61); /* TRES (Resolution Setting: 800 x 480) */
    epd_data1((uint8_t)(PANEL_WIDTH / 256));
    epd_data1((uint8_t)(PANEL_WIDTH % 256));
    epd_data1((uint8_t)(PANEL_HEIGHT / 256));
    epd_data1((uint8_t)(PANEL_HEIGHT % 256));

    epd_cmd(0x15); /* DUSPI */
    epd_data1(0x00); /* disabled */

    epd_cmd(0x50); /* VCOM AND DATA INTERVAL SETTING */
    epd_data1(0x29); /* LUTKW, N2OCP: copy new to old */
    epd_data1(0x07); /* CDI 10 hsync (default) */

    epd_cmd(0x60); /* TCON SETTING */
    epd_data1(0x22);

    epd_cmd(0xE3); /* PWS */
    epd_data1(0x22);
}

static void uc8179_init_full(void)
{
    uc8179_init_display();
    epd_cmd(0x00);   /* PANEL_SETTING */
    epd_data1(0x1F); /* full update LUT from OTP */
    uc8179_power_on();
    s_using_partial_mode = false;
}

static void uc8179_init_part(void)
{
    uc8179_init_display();
    epd_cmd(0xE0);   /* CCSET */
    epd_data1(0x02); /* TSFIX */
    epd_cmd(0xE5);   /* TSSET */
    epd_data1(0x6E); /* 110: fast partial from OTP */
    uc8179_power_on();
    s_using_partial_mode = true;
}

static void uc8179_display_full(const uint8_t *fb)
{
    ESP_LOGI(TAG, "Executing full e-paper refresh (800x480)...");
    uc8179_init_full();

    /* Write DTM2 (0x13): new frame */
    epd_cmd(0x13);
    epd_data(fb, FRAMEBUFFER_BYTES);

    /* If initial refresh, write DTM1 (0x10): previous frame as all 0x00 */
    if (s_needs_initial_full) {
        memset(s_scratch, 0x00, FRAMEBUFFER_BYTES);
        epd_cmd(0x10);
        epd_data(s_scratch, FRAMEBUFFER_BYTES);
    }

    /* Update full matching Seeed GxEPD2 */
    epd_cmd(0xE0);   /* CCSET */
    epd_data1(0x02); /* TSFIX */
    epd_cmd(0xE5);   /* TSSET */
    epd_data1(0x5A); /* 90 */

    epd_cmd(0x12);   /* DISPLAY_REFRESH */
    epd_wait_busy(15000);
    ESP_LOGI(TAG, "Full e-paper refresh finished");

    /* Sync OLD plane (0x10) with the just-shown frame so subsequent
     * partial updates calculate accurate deltas. */
    epd_cmd(0x10);
    epd_data(fb, FRAMEBUFFER_BYTES);
}

static void uc8179_display_fast(const uint8_t *fb, int x, int y, int w, int h)
{
    if (!s_using_partial_mode) {
        uc8179_init_part();
    }

    int x_start = x & ~7;
    int x_end   = (x + w - 1) | 7;
    int cols    = (x_end - x_start + 1) / 8;
    int rows    = h;

    epd_cmd(0x91); /* PARTIAL_IN */
    uc8179_set_partial_ram_area((uint16_t)x_start, (uint16_t)y, (uint16_t)(cols * 8), (uint16_t)rows);

    /* Pack window lines into scratch buffer for single transfer */
    uint8_t *dst = s_scratch;
    for (int r = 0; r < rows; r++) {
        const uint8_t *src_line = fb + (size_t)(y + r) * PANEL_WIDTH_BYTES + (size_t)(x_start / 8);
        memcpy(dst, src_line, (size_t)cols);
        dst += cols;
    }

    epd_cmd(0x13); /* DTM2: new frame */
    epd_data(s_scratch, (size_t)cols * rows);

    epd_cmd(0x12); /* DISPLAY_REFRESH */
    epd_wait_busy(5000);
    epd_cmd(0x92); /* PARTIAL_OUT after refresh */

    /* Sync OLD plane (0x10) with the just-shown frame so subsequent
     * partial updates (like erasing characters with backspace)
     * correctly compute pixel deltas and drive particles back to white. */
    epd_cmd(0x91); /* PARTIAL_IN */
    uc8179_set_partial_ram_area((uint16_t)x_start, (uint16_t)y, (uint16_t)(cols * 8), (uint16_t)rows);
    epd_cmd(0x10); /* DTM1: old frame */
    epd_data(s_scratch, (size_t)cols * rows);
    epd_cmd(0x92); /* PARTIAL_OUT */
}

static void uc8179_deep_sleep(void)
{
    uc8179_power_off();
    epd_cmd(0x07); /* DEEP_SLEEP */
    epd_data1(0xA5);
}

/* ---- Framebuffer dirty tracking ---- */

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
    if (black) {
        s_fb[idx] &= (uint8_t)~mask;
    } else {
        s_fb[idx] |= mask;
    }
}

static void fill_panel_rect(int x, int y, int w, int h, bool black)
{
    if (!s_fb || w <= 0 || h <= 0) return;
    int x0 = std::max(0, x);
    int y0 = std::max(0, y);
    int x1 = std::min(s_width, x + w);
    int y1 = std::min(s_height, y + h);
    if (x1 <= x0 || y1 <= y0) return;

    if (w >= s_width && x0 == 0 && (x1 - x0) == s_width) {
        size_t start_idx = (size_t)y0 * PANEL_WIDTH_BYTES;
        size_t count = (size_t)(y1 - y0) * PANEL_WIDTH_BYTES;
        memset(s_fb + start_idx, black ? 0x00 : 0xFF, count);
        return;
    }
    for (int py = y0; py < y1; ++py) {
        for (int px = x0; px < x1; ++px) {
            set_panel_pixel(px, py, black);
        }
    }
}

static inline bool rgb565_is_black(uint16_t v)
{
    return ((v >> 5) & 0x3F) < 32;
}

/* ---- Public API ---- */

extern "C" void display_init(int /*pin_a*/, int /*pin_b*/, int /*pin_c*/,
                             int /*pin_d*/, int /*pin_e*/, int /*pin_f*/,
                             int width, int height)
{
    if (s_initialized) return;

    if (width > 0 && height > 0 && (width != s_width || height != s_height)) {
        ESP_LOGW(TAG, "Requested geometry %dx%d does not match physical panel %dx%d",
                 width, height, s_width, s_height);
    }

    /* Allocate buffers with 64-byte alignment for DMA compatibility */
    s_fb = (uint8_t *)heap_caps_aligned_alloc(64, FRAMEBUFFER_BYTES,
                                              MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    s_scratch = (uint8_t *)heap_caps_aligned_alloc(64, FRAMEBUFFER_BYTES,
                                                  MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    assert(s_fb && s_scratch);
    memset(s_fb, 0xFF, FRAMEBUFFER_BYTES); /* default white */

    /* First, safely initialize SD card pins:
     * MicroSD slot shares SCK (GPIO7) and MOSI (GPIO9) with the display.
     * SD_EN (GPIO16) gates U13 power to the slot. If left off/floating,
     * an inserted card clamps the SPI bus lines via its ESD diodes.
     * SD_CS (GPIO14) must be driven HIGH to deselect the card. */
    gpio_config_t sd_cfg = {};
    sd_cfg.intr_type    = GPIO_INTR_DISABLE;
    sd_cfg.mode         = GPIO_MODE_OUTPUT;
    sd_cfg.pin_bit_mask = (1ULL << SD_SPI_CS_PIN) | (1ULL << SD_EN_PIN);
    gpio_config(&sd_cfg);
    gpio_set_level((gpio_num_t)SD_SPI_CS_PIN, 1);
    gpio_set_level((gpio_num_t)SD_EN_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(50));

    /* Initialize control output pins: CS HIGH, DC HIGH, RST HIGH */
    gpio_config_t out_cfg = {};
    out_cfg.intr_type    = GPIO_INTR_DISABLE;
    out_cfg.mode         = GPIO_MODE_OUTPUT;
    out_cfg.pin_bit_mask = (1ULL << EPD_CS_PIN) | (1ULL << EPD_DC_PIN) | (1ULL << EPD_RST_PIN);
    gpio_config(&out_cfg);
    gpio_set_level((gpio_num_t)EPD_CS_PIN, 1);
    gpio_set_level((gpio_num_t)EPD_DC_PIN, 1);
    gpio_set_level((gpio_num_t)EPD_RST_PIN, 1);

    /* Configure BUSY input pin with pull-up */
    gpio_config_t busy_cfg = {};
    busy_cfg.intr_type    = GPIO_INTR_DISABLE;
    busy_cfg.mode         = GPIO_MODE_INPUT;
    busy_cfg.pull_up_en   = GPIO_PULLUP_ENABLE;
    busy_cfg.pin_bit_mask = (1ULL << EPD_BUSY_PIN);
    gpio_config(&busy_cfg);

    /* Initialize SPI bus (SPI2_HOST) BEFORE reset so bus signals are stable.
     * EPD and MicroSD slot share this bus (MOSI=9, SCLK=7, MISO=8). */
    spi_bus_config_t bus_cfg = {};
    bus_cfg.mosi_io_num     = EPD_MOSI_PIN;
    bus_cfg.miso_io_num     = EPD_MISO_PIN; /* Shared with MicroSD */
    bus_cfg.sclk_io_num     = EPD_SCLK_PIN;
    bus_cfg.quadwp_io_num   = -1;
    bus_cfg.quadhd_io_num   = -1;
    bus_cfg.max_transfer_sz = FRAMEBUFFER_BYTES + 64;
    esp_err_t ret = spi_bus_initialize(E1001_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(ret);
    }
    gpio_set_pull_mode((gpio_num_t)EPD_MISO_PIN, GPIO_PULLUP_ONLY);

    esp_lcd_panel_io_spi_config_t io_cfg = {};
    io_cfg.dc_gpio_num       = (gpio_num_t)EPD_DC_PIN;
    io_cfg.cs_gpio_num       = (gpio_num_t)EPD_CS_PIN;
    io_cfg.pclk_hz           = E1001_SPI_CLOCK_HZ;
    io_cfg.lcd_cmd_bits      = 8;
    io_cfg.lcd_param_bits    = 8;
    io_cfg.spi_mode          = 0;
    io_cfg.trans_queue_depth = 4;
    io_cfg.flags.psram_dma_direct = 1;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)E1001_SPI_HOST, &io_cfg, &s_io));

    /* Now pulse reset to energize panel through U10 and initialize UC8179 */
    epd_reset_pulse();

    uc8179_init_full();

    s_needs_initial_full = true;
    s_force_full = true;
    s_partial_count = 0;
    clear_dirty();
    s_initialized = true;

    ESP_LOGI(TAG, "Seeed reTerminal E1001 e-paper initialized (%dx%d, UC8179 at 2 MHz)",
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

    bool huge = dirty_area_sum * 4 > (long)s_width * s_height * 3;
    bool do_full = s_needs_initial_full || s_force_full || huge ||
                   s_partial_count >= E1001_FULL_REFRESH_INTERVAL;

    if (do_full) {
        uc8179_display_full(s_fb);
        s_partial_count = 0;
        s_needs_initial_full = false;
    } else {
        int w = x1 - x0 + 1;
        int h = y1 - y0 + 1;
        uc8179_display_fast(s_fb, x0, y0, w, h);
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
    /* E-paper retains its image without power */
}

extern "C" void display_wake(void)
{
}

extern "C" void display_set_backlight(int /*percent*/)
{
    /* No backlight / front-light on this board */
}

extern "C" void display_deep_sleep_prepare(void)
{
    if (s_initialized) {
        uc8179_deep_sleep();
    }
    s_initialized = false;
}

#endif /* CONFIG_DRAFTLING_DISPLAY_SEEED_E1001 */
