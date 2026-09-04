#include "sdkconfig.h"
#if defined(CONFIG_DRAFTLING_DISPLAY_XTEINK_EPD)

/*
 * Xteink X4 Pro SPI e-paper backend.
 *
 * Different manufacturing runs of this board ship one of three panel
 * controllers -- SSD1677, or one of two UltraChip parts (UC8179 /
 * UC8279) -- which cannot be told apart from the outside. At
 * display_init() time this backend bit-bangs a small identification
 * probe over the same SPI pins (reading the VER/FLG registers a
 * UC81xx part answers and an SSD1677 does not) and then drives
 * whichever controller is actually present. Only the black/white
 * differential-refresh path is implemented; grayscale / anti-aliasing
 * rendering is out of scope, matching every other e-paper backend in
 * this codebase.
 *
 * Pin assignments, register sequences and waveform timing are ported
 * from the FreeInk SDK (https://github.com/Free-Ink/freeink-sdk, MIT
 * licensed), which reverse-engineered the Xteink X4 Pro OEM firmware
 * down to exact register values (Ssd1677Driver.cpp, Uc8179Driver.cpp,
 * Uc8279X4Driver.cpp, EpdBus.cpp and XteinkDetect.cpp). This board has
 * NOT been tested on physical hardware; see HARDWARE.md.
 *
 * Panel: 800x480, no mirror. SPI: SCLK=12, MOSI=11, CS=13, DC=18,
 * RST=14, BUSY=6, no MISO in normal operation (the boot-time probe
 * temporarily reconfigures MOSI as an input for its half-duplex
 * VER/FLG read). Dual-channel (cool/warm) PWM front-light on GPIO8/9.
 */

#include <algorithm>
#include <cstring>

#include <driver/gpio.h>
#include <driver/ledc.h>
#include <driver/spi_master.h>
#include <esp_heap_caps.h>
#include <esp_lcd_panel_io.h>
#include <esp_log.h>
#include <esp_rom_sys.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "display.h"

static const char *TAG = "DisplayXteink";

/* ---- Panel geometry ---- */
#define PANEL_WIDTH         800
#define PANEL_HEIGHT        480
#define PANEL_WIDTH_BYTES   (PANEL_WIDTH / 8)
#define FRAMEBUFFER_BYTES   (PANEL_WIDTH_BYTES * PANEL_HEIGHT)

/* UC8179/UC8279 address the panel as 800x600 (480 visible rows,
 * padded); UC8279 additionally offsets the visible window by 120
 * gates. One scratch buffer sized for the addressed area covers both
 * controllers' RAM-plane writes. */
#define UC81XX_TRES_HEIGHT     600
#define UC8279_GATE_OFFSET     120
#define UC81XX_SCRATCH_BYTES   (PANEL_WIDTH_BYTES * UC81XX_TRES_HEIGHT)

/* ---- Pins. This backend is used by exactly one board, so the pins
 * are hard-coded here rather than threaded through a board header --
 * matching display_ili9341.cpp / display_h752.cpp. ---- */
#define EPD_SCLK_PIN         12
#define EPD_MOSI_PIN         11
#define EPD_CS_PIN           13
#define EPD_DC_PIN           18
#define EPD_RST_PIN          14
#define EPD_BUSY_PIN         6
#define FRONTLIGHT_COOL_PIN  8
#define FRONTLIGHT_WARM_PIN  9

#define XTEINK_SPI_HOST      SPI2_HOST
/* The OEM firmware clocks this panel at 5 MHz. The SSD1677 is rated
 * for faster serial writes and the FreeInk SDK's own X4 Pro profile
 * runs it at 20 MHz for speed -- but flags that exact tradeoff as
 * "drop back to 5 MHz if artifacts appear". On real hardware, 20 MHz
 * produced a partial/faded differential update over roughly the
 * lower 60% of the panel (rows written later in the bulk RAM-plane
 * transfer, most likely corrupted by marginal SPI signal integrity
 * at that rate), needing several refreshes to converge to full
 * black. 5 MHz matches the OEM and clears it. */
#define XTEINK_SPI_CLOCK_HZ  (5 * 1000 * 1000)

/* The enclosure's cover overlaps the panel unevenly (left 12 px, top
 * 8 px, right/bottom 0), hiding that band from view. Every incoming
 * coordinate (from LVGL, already rendering at the margin-shrunk
 * DISPLAY_LOGICAL_WIDTH/HEIGHT -- see app_config.h) is offset by the
 * left/top margin before it is written into the physical panel
 * framebuffer, so on-screen content never lands under the cover. */
#define EPD_MARGIN_LEFT      CONFIG_DRAFTLING_DISPLAY_MARGIN_LEFT
#define EPD_MARGIN_TOP       CONFIG_DRAFTLING_DISPLAY_MARGIN_TOP

#ifdef CONFIG_DRAFTLING_EPD_FULL_REFRESH_INTERVAL
#define XTEINK_FULL_REFRESH_INTERVAL CONFIG_DRAFTLING_EPD_FULL_REFRESH_INTERVAL
#else
#define XTEINK_FULL_REFRESH_INTERVAL 30
#endif

/* Front-light LEDC: two channels driven identically (Draftling has no
 * warm/cool color-temperature UI), 10 kHz / 10-bit, active-HIGH. */
#define BL_LEDC_TIMER        LEDC_TIMER_0
#define BL_LEDC_MODE         LEDC_LOW_SPEED_MODE
#define BL_LEDC_CHANNEL_COOL LEDC_CHANNEL_0
#define BL_LEDC_CHANNEL_WARM LEDC_CHANNEL_1
#define BL_LEDC_DUTY_RES     LEDC_TIMER_10_BIT
#define BL_LEDC_DUTY_MAX     ((1 << 10) - 1)
#define BL_LEDC_FREQ_HZ      10000

enum xteink_ctrl_t {
    XTEINK_CTRL_SSD1677 = 0,
    XTEINK_CTRL_UC8179,
    XTEINK_CTRL_UC8279,
};

static esp_lcd_panel_io_handle_t s_io = NULL;
static uint8_t  *s_fb = NULL;
static uint8_t  *s_scratch = NULL;   /* UC8179/UC8279 padded/reversed staging buffer */
static uint8_t  *s_white_scratch = NULL; /* constant all-white OLD-plane buffer, see uc8179/uc8279_display_full() */
static bool      s_initialized = false;
static bool      s_bl_inited = false;
static bool      s_needs_initial_full = true;
static bool      s_force_full = true;
static int       s_partial_count = 0;
static int       s_width = PANEL_WIDTH;
static int       s_height = PANEL_HEIGHT;
static xteink_ctrl_t s_ctrl = XTEINK_CTRL_SSD1677;
static bool       s_uc81xx_screen_on = false;

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

/* ---- SPI command/data framing (esp_lcd_panel_io_spi) ----
 * epd_cmd()/epd_data1()/epd_data() map 1:1 onto the FreeInk SDK's
 * EpdBus::cmd()/data() calls. Bulk RAM-plane writes go through
 * esp_lcd_panel_io_tx_color() (same DMA fast path display_ili9341.cpp
 * uses for its RAMWR bursts) rather than tx_param, which is sized for
 * short parameter writes. */
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

/* BUSY polarity differs by controller family (FreeInk SDK EpdBus.h):
 *   SSD1677:          busy while HIGH.
 *   UC8179 / UC8279:  BUSY_N, busy while LOW; production waits one
 *                      RTOS tick before polling, then waits for HIGH
 *                      (no fixed timeout -- issuing the next command
 *                      while still busy can make the UC controller
 *                      discard plane or LUT writes). */
static void epd_wait_busy_active_high(void)
{
    int64_t start = esp_timer_get_time();
    while (gpio_get_level((gpio_num_t)EPD_BUSY_PIN) == 1) {
        vTaskDelay(pdMS_TO_TICKS(1));
        if (esp_timer_get_time() - start > 30 * 1000 * 1000) break;
    }
}

static void epd_wait_busy_uc_idle_high(void)
{
    vTaskDelay(pdMS_TO_TICKS(1));
    while (gpio_get_level((gpio_num_t)EPD_BUSY_PIN) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

static void epd_reset_pulse(uint16_t extra_settle_ms)
{
    gpio_set_level((gpio_num_t)EPD_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level((gpio_num_t)EPD_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level((gpio_num_t)EPD_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    if (extra_settle_ms) vTaskDelay(pdMS_TO_TICKS(extra_settle_ms));
}

/* ============================================================
 * Boot-time controller probe (ported, trimmed, from the FreeInk
 * SDK's XteinkDetect.cpp). Bit-banged half-duplex read over the same
 * SCLK/MOSI/CS/DC/RST pins the panel SPI bus will use, run BEFORE
 * spi_bus_initialize()/esp_lcd_new_panel_io_spi() claim them.
 *
 * An SSD1677 does not answer command 0x70 (VER) / 0x71 (FLG) at all,
 * so the half-duplex line floats to a uniform pattern through the
 * pull-up. A UC81xx part drives a real, non-uniform VER + a FLG byte
 * with BUSY_N (bit0) set (idle). VER byte 2 (LUT_VER) then tells
 * UC8179 (0x01 or unrecognized) from UC8279 (0x02/0x68/0x69) apart.
 * ============================================================ */

static void probe_write_byte(uint8_t b)
{
    for (int i = 0; i < 8; i++) {
        gpio_set_level((gpio_num_t)EPD_MOSI_PIN, (b & 0x80) ? 1 : 0);
        esp_rom_delay_us(1);
        gpio_set_level((gpio_num_t)EPD_SCLK_PIN, 1);
        esp_rom_delay_us(1);
        gpio_set_level((gpio_num_t)EPD_SCLK_PIN, 0);
        b <<= 1;
    }
}

static uint8_t probe_read_byte(void)
{
    uint8_t b = 0;
    for (int i = 0; i < 8; i++) {
        esp_rom_delay_us(1);
        b = (uint8_t)((b << 1) | (gpio_get_level((gpio_num_t)EPD_MOSI_PIN) ? 1 : 0));
        gpio_set_level((gpio_num_t)EPD_SCLK_PIN, 1);
        esp_rom_delay_us(1);
        gpio_set_level((gpio_num_t)EPD_SCLK_PIN, 0);
    }
    return b;
}

static void probe_cmd_read(uint8_t cmd, uint8_t *out, int len)
{
    gpio_set_direction((gpio_num_t)EPD_MOSI_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)EPD_DC_PIN, 0);
    gpio_set_level((gpio_num_t)EPD_CS_PIN, 0);
    esp_rom_delay_us(1);
    probe_write_byte(cmd);
    gpio_set_level((gpio_num_t)EPD_DC_PIN, 1);
    gpio_set_direction((gpio_num_t)EPD_MOSI_PIN, GPIO_MODE_INPUT);
    esp_rom_delay_us(1);
    for (int i = 0; i < len; i++) out[i] = probe_read_byte();
    gpio_set_level((gpio_num_t)EPD_CS_PIN, 1);
    gpio_set_direction((gpio_num_t)EPD_MOSI_PIN, GPIO_MODE_OUTPUT);
}

static bool probe_ver_is_floating(const uint8_t ver[5])
{
    for (int i = 1; i < 5; i++) if (ver[i] != ver[0]) return false;
    return true;
}

static bool probe_match_uc81xx(const uint8_t ver[5], uint8_t flg)
{
    if (flg == 0x00 || flg == 0xFF) return false;
    if ((flg & 0x01) != 0x01) return false;
    return !probe_ver_is_floating(ver);
}

static bool probe_run_pass(uint8_t ver[5], uint8_t *flg, uint8_t rst_low_ms)
{
    gpio_set_direction((gpio_num_t)EPD_CS_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)EPD_CS_PIN, 1);
    gpio_set_direction((gpio_num_t)EPD_SCLK_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)EPD_SCLK_PIN, 0);
    gpio_set_direction((gpio_num_t)EPD_DC_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)EPD_DC_PIN, 0);
    gpio_set_direction((gpio_num_t)EPD_MOSI_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)EPD_BUSY_PIN, GPIO_MODE_INPUT);

    gpio_set_direction((gpio_num_t)EPD_RST_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)EPD_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(2));
    gpio_set_level((gpio_num_t)EPD_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(rst_low_ms));
    gpio_set_level((gpio_num_t)EPD_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(30));

    uint8_t flg_byte = 0;
    probe_cmd_read(0x71, &flg_byte, 1);
    probe_cmd_read(0x70, ver, 5);
    if (flg) *flg = flg_byte;
    return probe_match_uc81xx(ver, flg_byte);
}

static xteink_ctrl_t probe_controller(void)
{
    uint8_t ver1[5] = {0}, ver2[5] = {0}, flg1 = 0;
    bool pass1 = probe_run_pass(ver1, &flg1, 1);
    vTaskDelay(pdMS_TO_TICKS(2));
    bool pass2 = probe_run_pass(ver2, NULL, pass1 ? 50 : 1);
    bool ver_agree = memcmp(ver1, ver2, sizeof(ver1)) == 0;
    bool confirmed = pass1 && pass2 && ver_agree;

    /* Release every probe pin to plain input so the SPI peripheral
     * (and the RST/BUSY gpio_config calls in display_init()) can
     * claim them cleanly afterward. */
    gpio_set_direction((gpio_num_t)EPD_SCLK_PIN, GPIO_MODE_INPUT);
    gpio_set_direction((gpio_num_t)EPD_MOSI_PIN, GPIO_MODE_INPUT);
    gpio_set_direction((gpio_num_t)EPD_CS_PIN,   GPIO_MODE_INPUT);
    gpio_set_direction((gpio_num_t)EPD_DC_PIN,   GPIO_MODE_INPUT);
    gpio_set_direction((gpio_num_t)EPD_RST_PIN,  GPIO_MODE_INPUT);

    if (!confirmed) {
        ESP_LOGI(TAG, "Display controller probe: SSD1677 (default)");
        return XTEINK_CTRL_SSD1677;
    }

    uint8_t lut_ver = ver2[2];
    if (lut_ver == 0x02 || lut_ver == 0x68 || lut_ver == 0x69) {
        ESP_LOGI(TAG, "Display controller probe: UC8279 (LUT_VER=0x%02X)", lut_ver);
        return XTEINK_CTRL_UC8279;
    }
    ESP_LOGI(TAG, "Display controller probe: UC8179 (LUT_VER=0x%02X)", lut_ver);
    return XTEINK_CTRL_UC8179;
}

/* ============================================================
 * SSD1677 (ported from Ssd1677Driver.cpp, ssd1677DefaultConfig() --
 * the Xteink X4 / X4 Pro defaults). Dual-RAM (BW=0x24, RED=0x26)
 * differential refresh, no mirror. Only the absolute (Full, 0xF7) and
 * differential (Fast, 0xFC) waveforms are used; the SDK's one-shot
 * "Half" first-paint optimization is skipped for simplicity -- the
 * very first flush after boot just uses Full. ============================================================ */

static void ssd1677_set_ram_area_full(void)
{
    /* Data-entry: X increment, Y decrement (no mirror). Gates are
     * physically reversed on this panel, so the Y window is computed
     * from the bottom. */
    int y = 0, h = PANEL_HEIGHT, w = PANEL_WIDTH;
    int y_win = PANEL_HEIGHT - y - h; /* = 0 for the full-frame window */

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

static void ssd1677_init(void)
{
    static const uint8_t booster[5] = { 0xAE, 0xC7, 0xC3, 0xC0, 0x80 };

    epd_cmd(0x12); /* SOFT_RESET */
    vTaskDelay(pdMS_TO_TICKS(10));
    epd_wait_busy_active_high();

    epd_cmd(0x18); epd_data1(0x80); /* TEMP_SENSOR_CONTROL: internal */

    epd_cmd(0x0C); /* BOOSTER_SOFT_START */
    for (uint8_t b : booster) epd_data1(b);

    epd_cmd(0x01); /* DRIVER_OUTPUT_CONTROL: height - 1, scan byte */
    epd_data1((uint8_t)((PANEL_HEIGHT - 1) & 0xFF));
    epd_data1((uint8_t)(((PANEL_HEIGHT - 1) >> 8) & 0xFF));
    epd_data1(0x02);

    epd_cmd(0x3C); epd_data1(0x80); /* BORDER_WAVEFORM init value */

    ssd1677_set_ram_area_full();

    epd_cmd(0x46); epd_data1(0xF7); epd_wait_busy_active_high(); /* AUTO_WRITE_BW_RAM */
    epd_cmd(0x47); epd_data1(0xF7); epd_wait_busy_active_high(); /* AUTO_WRITE_RED_RAM */
}

static void ssd1677_refresh(uint8_t ctrl1, uint8_t seq)
{
    epd_cmd(0x21); epd_data1(ctrl1);            /* DISPLAY_UPDATE_CTRL1 */
    epd_cmd(0x3C); epd_data1(0xC0);              /* BORDER_WAVEFORM, all modes */
    epd_cmd(0x22); epd_data1(seq);               /* DISPLAY_UPDATE_CTRL2 */
    epd_cmd(0x20);                               /* MASTER_ACTIVATION */
    epd_wait_busy_active_high();
}

static void ssd1677_display_full(const uint8_t *fb)
{
    ssd1677_set_ram_area_full();
    epd_cmd(0x24); epd_data(fb, FRAMEBUFFER_BYTES); /* WRITE_RAM_BW */
    epd_cmd(0x26); epd_data(fb, FRAMEBUFFER_BYTES); /* WRITE_RAM_RED */
    ssd1677_refresh(0x40 /* CTRL1_BYPASS_RED */, 0xF7);
}

static void ssd1677_display_fast(const uint8_t *fb)
{
    ssd1677_set_ram_area_full();
    epd_cmd(0x24); epd_data(fb, FRAMEBUFFER_BYTES); /* WRITE_RAM_BW = new frame */
    /* Single-buffer mode: RED already holds the previous frame from
     * the last refresh's post-sync below, so no pre-write here. */
    ssd1677_refresh(0x00 /* CTRL1_NORMAL */, 0xFC);

    /* Re-sync both planes to the just-shown frame so the next fast
     * refresh diffs against a clean baseline (Ssd1677Driver's
     * prev==nullptr post-sync path). */
    ssd1677_set_ram_area_full();
    epd_cmd(0x24); epd_data(fb, FRAMEBUFFER_BYTES);
    epd_cmd(0x26); epd_data(fb, FRAMEBUFFER_BYTES);
}

static void ssd1677_deep_sleep(void)
{
    /* powerOffController(): unconditional, harmless even if the panel
     * already self-powered-down after its last refresh. */
    epd_cmd(0x3C); epd_data1(0x80);
    epd_cmd(0x22); epd_data1(0x03);
    epd_cmd(0x20);
    vTaskDelay(pdMS_TO_TICKS(200));
    epd_wait_busy_active_high();

    epd_cmd(0x10); epd_data1(0x03); /* DEEP_SLEEP mode 2 */
}

/* ============================================================
 * UC8179 (ported from Uc8179Driver.cpp, uc8179DefaultConfig()).
 * KW-mode DTM1(0x10)=OLD / DTM2(0x13)=NEW differential refresh via
 * PARTIAL_IN(0x91)/PARTIAL_OUT(0x92), OTP waveforms (no custom LUT).
 * Addressed as 800x600 (480 visible rows, padded, row order
 * reversed). ============================================================ */

static void uc8179_stream_plane(uint8_t ram_cmd, const uint8_t *fb)
{
    uint8_t *dst = s_scratch;
    for (int y = PANEL_HEIGHT - 1; y >= 0; y--) {
        memcpy(dst, fb + (size_t)y * PANEL_WIDTH_BYTES, PANEL_WIDTH_BYTES);
        dst += PANEL_WIDTH_BYTES;
    }
    memset(dst, 0xFF, (size_t)(UC81XX_TRES_HEIGHT - PANEL_HEIGHT) * PANEL_WIDTH_BYTES);
    epd_cmd(ram_cmd);
    epd_data(s_scratch, UC81XX_SCRATCH_BYTES);
}

static void uc8179_power_on_if_needed(void)
{
    if (s_uc81xx_screen_on) return;
    epd_cmd(0x04); /* POWER_ON */
    epd_wait_busy_uc_idle_high();
    s_uc81xx_screen_on = true;
}

static void uc8179_init(void)
{
    static const uint8_t btst[4] = { 0x25, 0x25, 0x3C, 0x25 };

    epd_cmd(0x00); epd_data1(0x3F); epd_data1(0x0A); /* PSR */

    epd_cmd(0x61); /* RESOLUTION: 800 x 600 (addressed) */
    epd_data1(0x03); epd_data1(0x20);
    epd_data1(0x02); epd_data1(0x58);

    epd_cmd(0x65); epd_data1(0); epd_data1(0); epd_data1(0); epd_data1(0); /* GATE_SOURCE_START */

    epd_cmd(0x03); epd_data1(0x20); /* PFS */
    epd_cmd(0x06); for (uint8_t b : btst) epd_data1(b); /* BOOSTER_SOFT_START */
    epd_cmd(0xE1); epd_data1(0x02); /* GATE_SCAN */
    epd_cmd(0xE3); epd_data1(0x22); /* POWER_SAVE */

    s_uc81xx_screen_on = false;
}

static void uc8179_display_full(const uint8_t *fb)
{
    uc8179_stream_plane(0x13, fb); /* NEW = fb */

    /* OLD = white (absolute GC waveform). Streamed from a SEPARATE
     * constant buffer, not by memset()-ing and reusing s_scratch: the
     * epd_data() call in uc8179_stream_plane() above queues its SPI/
     * DMA transfer asynchronously (esp_lcd_panel_io_tx_color(), with
     * trans_queue_depth > 1) and can return before the hardware has
     * actually finished reading s_scratch. Overwriting that same
     * buffer here, with no synchronization in between, raced the DMA
     * read of the just-queued NEW-plane transfer and corrupted it on
     * real hardware -- reproduced as roughly the lower 2/3 of the
     * panel (the majority of the 60000-byte transfer, streamed after
     * a fast CPU-side memset had already overwritten it to white)
     * never showing the actual document content. */
    epd_cmd(0x10); epd_data(s_white_scratch, UC81XX_SCRATCH_BYTES);

    epd_cmd(0x50); epd_data1(0x29); epd_data1(0x07); /* VCOM_DATA_INTERVAL, active */
    epd_cmd(0xE0); epd_data1(0x02);                  /* CCSET */
    epd_cmd(0xE5); epd_data1(0x1E);                  /* TSSET, full */
    epd_cmd(0x00); epd_data1(0x1F); epd_data1(0x0A); /* PSR: REG cleared -> OTP */

    uc8179_power_on_if_needed();
    epd_cmd(0x12); /* DISPLAY_REFRESH */
    epd_wait_busy_uc_idle_high();

    epd_cmd(0x50); epd_data1(0xA9); epd_data1(0x07); /* restore idle CDI */
    uc8179_stream_plane(0x10, fb); /* sync OLD <- just-shown frame */
}

static void uc8179_display_fast(const uint8_t *fb)
{
    uc8179_stream_plane(0x13, fb); /* NEW = fb; OLD already holds the previous frame */

    epd_cmd(0x50); epd_data1(0x29); epd_data1(0x07);
    epd_cmd(0xE0); epd_data1(0x02);
    epd_cmd(0xE5); epd_data1(0x5A); /* TSSET, fast -- the frame-rate lever */
    epd_cmd(0x00); epd_data1(0x1F); epd_data1(0x0A);
    epd_cmd(0x03); epd_data1(0x20); /* PFS re-assert, fast-only */
    epd_cmd(0xE1); epd_data1(0x02); /* gate-scan re-assert, fast-only */

    uc8179_power_on_if_needed();
    epd_cmd(0x91); /* PARTIAL_IN, whole-panel */
    epd_cmd(0x12); /* DISPLAY_REFRESH */
    epd_wait_busy_uc_idle_high();
    epd_cmd(0x92); /* PARTIAL_OUT */

    epd_cmd(0x50); epd_data1(0xA9); epd_data1(0x07);
    uc8179_stream_plane(0x10, fb); /* sync OLD <- just-shown frame */
}

static void uc8179_deep_sleep(void)
{
    if (s_uc81xx_screen_on) {
        epd_cmd(0x02); /* POWER_OFF */
        epd_wait_busy_uc_idle_high();
        s_uc81xx_screen_on = false;
    }
    epd_cmd(0x07); epd_data1(0xA5); /* DEEP_SLEEP */
}

/* ============================================================
 * UC8279 (ported from Uc8279X4Driver.cpp, uc8279X4DefaultConfig() --
 * the X4 Pro variant, NOT the X3's UC8279d). Same KW/DTM/PARTIAL_IN
 * paradigm as UC8179, but its own init (PSR 0x37/0x4D, PLL 0x0E,
 * 1-byte CDI) and a 120-gate scan offset on the 600-gate scan; rows
 * are streamed forward (not reversed) with padding before AND after
 * the visible window. Field-validated in the FreeInk SDK on real
 * hardware (2026-08-19, LUT_VER=0x02 unit).
 * ============================================================ */

static void uc8279_stream_plane(uint8_t ram_cmd, const uint8_t *fb)
{
    uint8_t *dst = s_scratch;
    memset(dst, 0xFF, (size_t)UC8279_GATE_OFFSET * PANEL_WIDTH_BYTES);
    dst += (size_t)UC8279_GATE_OFFSET * PANEL_WIDTH_BYTES;
    memcpy(dst, fb, FRAMEBUFFER_BYTES);
    dst += FRAMEBUFFER_BYTES;
    memset(dst, 0xFF, (size_t)(UC81XX_TRES_HEIGHT - UC8279_GATE_OFFSET - PANEL_HEIGHT) * PANEL_WIDTH_BYTES);
    epd_cmd(ram_cmd);
    epd_data(s_scratch, UC81XX_SCRATCH_BYTES);
}

static void uc8279_power_on_if_needed(void)
{
    if (s_uc81xx_screen_on) return;
    epd_cmd(0x04); /* POWER_ON */
    epd_wait_busy_uc_idle_high();
    s_uc81xx_screen_on = true;
}

static void uc8279_init(void)
{
    epd_cmd(0x00); epd_data1(0x37); epd_data1(0x4D); /* PSR */

    epd_cmd(0x61); /* RESOLUTION: 800 x 600 (addressed) */
    epd_data1(0x03); epd_data1(0x20);
    epd_data1(0x02); epd_data1(0x58);

    epd_cmd(0x65); epd_data1(0); epd_data1(0); epd_data1(0); epd_data1(0); /* GATE_SOURCE_START */

    epd_cmd(0x03); epd_data1(0x20); /* PFS */
    epd_cmd(0x30); epd_data1(0x0E); /* PLL */
    epd_cmd(0xE1); epd_data1(0x02); /* GATE_SCAN */

    s_uc81xx_screen_on = false;
}

static void uc8279_display_full(const uint8_t *fb)
{
    uc8279_stream_plane(0x13, fb); /* NEW = fb */

    /* OLD = white, from the separate constant buffer -- see the race
     * comment in uc8179_display_full(); the same hazard applies here. */
    epd_cmd(0x10); epd_data(s_white_scratch, UC81XX_SCRATCH_BYTES);

    epd_cmd(0x50); epd_data1(0x97);                  /* VCOM_DATA_INTERVAL, full (1 byte only) */
    epd_cmd(0xE0); epd_data1(0x02);                  /* CCSET */
    epd_cmd(0xE5); epd_data1(0x1E);                  /* TSSET, full */

    uc8279_power_on_if_needed();

    /* PSR must be rewritten AFTER POWER_ON to latch (POWER_ON reloads
     * MTP defaults) and right before the refresh command. */
    epd_cmd(0x00); epd_data1(0x17); epd_data1(0x4D); /* REG cleared -> OTP */
    epd_cmd(0x12); /* DISPLAY_REFRESH */
    epd_wait_busy_uc_idle_high();

    uc8279_stream_plane(0x10, fb); /* sync OLD <- just-shown frame */
}

static void uc8279_display_fast(const uint8_t *fb)
{
    uc8279_stream_plane(0x13, fb); /* NEW = fb; OLD already holds the previous frame */

    epd_cmd(0x50); epd_data1(0xD7);                  /* VCOM_DATA_INTERVAL, fast */
    epd_cmd(0xE0); epd_data1(0x02);
    epd_cmd(0xE5); epd_data1(0x5A);                  /* TSSET, fast */
    epd_cmd(0x03); epd_data1(0x20);                  /* PFS re-assert, fast-only */
    epd_cmd(0xE1); epd_data1(0x02);                  /* gate-scan re-assert, fast-only */

    uc8279_power_on_if_needed();

    const int x_end = PANEL_WIDTH - 1;
    const int y_start = UC8279_GATE_OFFSET;
    const int y_end = UC8279_GATE_OFFSET + PANEL_HEIGHT - 1;
    epd_cmd(0x91); /* PARTIAL_IN */
    epd_cmd(0x90); /* PARTIAL_WINDOW: full visible window, gate-offset addressed */
    epd_data1(0x00); epd_data1(0x00);
    epd_data1((uint8_t)((x_end >> 8) & 0xFF)); epd_data1((uint8_t)(x_end & 0xFF));
    epd_data1((uint8_t)((y_start >> 8) & 0xFF)); epd_data1((uint8_t)(y_start & 0xFF));
    epd_data1((uint8_t)((y_end >> 8) & 0xFF)); epd_data1((uint8_t)(y_end & 0xFF));
    epd_data1(0x01);

    epd_cmd(0x00); epd_data1(0x17); epd_data1(0x4D); /* PSR rewrite after PON + window */
    epd_cmd(0x12); /* DISPLAY_REFRESH */
    epd_wait_busy_uc_idle_high();
    epd_cmd(0x92); /* PARTIAL_OUT */

    uc8279_stream_plane(0x10, fb); /* sync OLD <- just-shown frame */
}

static void uc8279_deep_sleep(void)
{
    if (s_uc81xx_screen_on) {
        epd_cmd(0x02); /* POWER_OFF */
        epd_wait_busy_uc_idle_high();
        s_uc81xx_screen_on = false;
    }
    epd_cmd(0x07); epd_data1(0xA5); /* DEEP_SLEEP */
}

/* ---- Controller dispatch ---- */

static void ctrl_init(void)
{
    switch (s_ctrl) {
    case XTEINK_CTRL_SSD1677: ssd1677_init(); break;
    case XTEINK_CTRL_UC8179:  uc8179_init();  break;
    case XTEINK_CTRL_UC8279:  uc8279_init();  break;
    }
}

static void ctrl_display_full(const uint8_t *fb)
{
    switch (s_ctrl) {
    case XTEINK_CTRL_SSD1677: ssd1677_display_full(fb); break;
    case XTEINK_CTRL_UC8179:  uc8179_display_full(fb);  break;
    case XTEINK_CTRL_UC8279:  uc8279_display_full(fb);  break;
    }
}

static void ctrl_display_fast(const uint8_t *fb)
{
    switch (s_ctrl) {
    case XTEINK_CTRL_SSD1677: ssd1677_display_fast(fb); break;
    case XTEINK_CTRL_UC8179:  uc8179_display_fast(fb);  break;
    case XTEINK_CTRL_UC8279:  uc8279_display_fast(fb);  break;
    }
}

static void ctrl_deep_sleep(void)
{
    switch (s_ctrl) {
    case XTEINK_CTRL_SSD1677: ssd1677_deep_sleep(); break;
    case XTEINK_CTRL_UC8179:  uc8179_deep_sleep();  break;
    case XTEINK_CTRL_UC8279:  uc8279_deep_sleep();  break;
    }
}

/* ---- Front-light (dual-channel PWM) ---- */

static void backlight_pwm_init(void)
{
    if (s_bl_inited) return;

    gpio_hold_dis((gpio_num_t)FRONTLIGHT_COOL_PIN);
    gpio_hold_dis((gpio_num_t)FRONTLIGHT_WARM_PIN);

    ledc_timer_config_t t = {};
    t.speed_mode      = BL_LEDC_MODE;
    t.duty_resolution = BL_LEDC_DUTY_RES;
    t.timer_num       = BL_LEDC_TIMER;
    t.freq_hz         = BL_LEDC_FREQ_HZ;
    t.clk_cfg         = LEDC_AUTO_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&t));

    ledc_channel_config_t cc = {};
    cc.gpio_num   = FRONTLIGHT_COOL_PIN;
    cc.speed_mode = BL_LEDC_MODE;
    cc.channel    = BL_LEDC_CHANNEL_COOL;
    cc.timer_sel  = BL_LEDC_TIMER;
    cc.intr_type  = LEDC_INTR_DISABLE;
    cc.duty       = 0;
    cc.hpoint     = 0;
    ESP_ERROR_CHECK(ledc_channel_config(&cc));

    ledc_channel_config_t cw = cc;
    cw.gpio_num = FRONTLIGHT_WARM_PIN;
    cw.channel  = BL_LEDC_CHANNEL_WARM;
    ESP_ERROR_CHECK(ledc_channel_config(&cw));

    s_bl_inited = true;
}

extern "C" void display_set_backlight(int percent)
{
    if (!s_bl_inited) return;
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    uint32_t duty = (uint32_t)((BL_LEDC_DUTY_MAX * percent) / 100);
    ESP_ERROR_CHECK(ledc_set_duty(BL_LEDC_MODE, BL_LEDC_CHANNEL_COOL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(BL_LEDC_MODE, BL_LEDC_CHANNEL_COOL));
    ESP_ERROR_CHECK(ledc_set_duty(BL_LEDC_MODE, BL_LEDC_CHANNEL_WARM, duty));
    ESP_ERROR_CHECK(ledc_update_duty(BL_LEDC_MODE, BL_LEDC_CHANNEL_WARM));
}

/* ---- display.h public API ---- */

extern "C" void display_set_shared_i2c_bus(void * /*bus_handle*/)
{
    /* This backend does not use I2C; the touchscreen and CW2017
     * battery backends get the shared bus directly from main.cpp. */
}

extern "C" void display_init(int, int, int, int, int, int, int width, int height)
{
    if (s_initialized) return;

    s_fb = (uint8_t *)heap_caps_malloc(FRAMEBUFFER_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_fb) s_fb = (uint8_t *)heap_caps_malloc(FRAMEBUFFER_BYTES, MALLOC_CAP_8BIT);
    s_scratch = (uint8_t *)heap_caps_malloc(UC81XX_SCRATCH_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_scratch) s_scratch = (uint8_t *)heap_caps_malloc(UC81XX_SCRATCH_BYTES, MALLOC_CAP_8BIT);
    /* Separate, constant all-white buffer for the OLD-plane write in
     * uc8179/uc8279_display_full() -- see the race-condition comment
     * there. Never mutated after this point. */
    s_white_scratch = (uint8_t *)heap_caps_malloc(UC81XX_SCRATCH_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_white_scratch) s_white_scratch = (uint8_t *)heap_caps_malloc(UC81XX_SCRATCH_BYTES, MALLOC_CAP_8BIT);
    if (!s_fb || !s_scratch || !s_white_scratch) {
        ESP_LOGE(TAG, "Framebuffer/scratch allocation failed");
        return;
    }
    memset(s_fb, 0xFF, FRAMEBUFFER_BYTES);
    memset(s_white_scratch, 0xFF, UC81XX_SCRATCH_BYTES);

    s_width = PANEL_WIDTH;
    s_height = PANEL_HEIGHT;
    if (width != s_width || height != s_height) {
        ESP_LOGW(TAG, "Configured size %dx%d differs from panel %dx%d",
                 width, height, s_width, s_height);
    }

    /* Identify the panel controller before the SPI peripheral claims
     * the pins -- see the probe_* functions above. */
    s_ctrl = probe_controller();

    spi_bus_config_t bus_cfg = {};
    bus_cfg.mosi_io_num   = EPD_MOSI_PIN;
    bus_cfg.miso_io_num   = -1;
    bus_cfg.sclk_io_num   = EPD_SCLK_PIN;
    bus_cfg.quadwp_io_num = -1;
    bus_cfg.quadhd_io_num = -1;
    bus_cfg.max_transfer_sz = UC81XX_SCRATCH_BYTES;
    ESP_ERROR_CHECK(spi_bus_initialize(XTEINK_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_spi_config_t io_cfg = {};
    io_cfg.dc_gpio_num       = (gpio_num_t)EPD_DC_PIN;
    io_cfg.cs_gpio_num       = (gpio_num_t)EPD_CS_PIN;
    io_cfg.pclk_hz           = XTEINK_SPI_CLOCK_HZ;
    io_cfg.lcd_cmd_bits      = 8;
    io_cfg.lcd_param_bits    = 8;
    io_cfg.spi_mode          = 0;
    io_cfg.trans_queue_depth = 4;
    io_cfg.flags.psram_dma_direct = 1;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)XTEINK_SPI_HOST, &io_cfg, &s_io));

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

    /* UC8179/UC8279 begin() with a 50 ms extra settle after reset;
     * SSD1677 uses none (its own SOFT_RESET command handles the
     * logical reset with its own delay). */
    epd_reset_pulse((s_ctrl == XTEINK_CTRL_SSD1677) ? 0 : 50);
    ctrl_init();

    backlight_pwm_init();

    s_needs_initial_full = true;
    s_force_full = true;
    s_partial_count = 0;
    clear_dirty();
    s_initialized = true;

    ESP_LOGI(TAG, "Xteink X4 Pro e-paper initialized (%dx%d, controller=%d)",
             s_width, s_height, (int)s_ctrl);
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
                   s_partial_count >= XTEINK_FULL_REFRESH_INTERVAL;

    if (do_full) {
        ctrl_display_full(s_fb);
        s_partial_count = 0;
        s_needs_initial_full = false;
    } else {
        ctrl_display_fast(s_fb);
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

extern "C" void display_deep_sleep_prepare(void)
{
    if (s_bl_inited) {
        ledc_set_duty(BL_LEDC_MODE, BL_LEDC_CHANNEL_COOL, 0);
        ledc_update_duty(BL_LEDC_MODE, BL_LEDC_CHANNEL_COOL);
        ledc_set_duty(BL_LEDC_MODE, BL_LEDC_CHANNEL_WARM, 0);
        ledc_update_duty(BL_LEDC_MODE, BL_LEDC_CHANNEL_WARM);
    }
    for (int pin : { FRONTLIGHT_COOL_PIN, FRONTLIGHT_WARM_PIN }) {
        gpio_hold_dis((gpio_num_t)pin);
        gpio_reset_pin((gpio_num_t)pin);
        gpio_set_direction((gpio_num_t)pin, GPIO_MODE_OUTPUT);
        gpio_set_level((gpio_num_t)pin, 0);
        gpio_hold_en((gpio_num_t)pin);
    }

    if (s_initialized) ctrl_deep_sleep();
    s_initialized = false;
}

#endif /* CONFIG_DRAFTLING_DISPLAY_XTEINK_EPD */
