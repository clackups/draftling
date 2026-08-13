#include "sdkconfig.h"
#if defined(CONFIG_DRAFTLING_DISPLAY_ILI9341) || defined(CONFIG_DRAFTLING_DISPLAY_ST7796)

/*
 * Shared 4-wire SPI TFT backend for the Freenove FNK0104 family's
 * two SPI panels: the 2.8" ILI9341 (FNK0104A/B) and the 4.0" ST7796
 * (FNK0104S -- Freenove's own board silkscreen and README call it
 * "ST7789", but the vendor TFT_eSPI setup header and init sequence
 * are unambiguously ST7796). Both controllers share the same 4-wire
 * SPI protocol (8-bit command + 8-bit parameter bytes, one CS pulse
 * per command/data-burst) and the same RGB565 pixel format, so one
 * file serves both, selecting the vendor init table with #if
 * CONFIG_DRAFTLING_DISPLAY_ST7796.
 *
 * All four FNK0104 SPI-TFT SKUs (A/B/S) wire the panel to the same
 * GPIOs (MOSI=11, SCLK=12, DC=46, CS=10, BL=45, no discrete RST --
 * TFT_eSPI's setup headers define TFT_RST as -1 for this family, so
 * this backend uses the SWRESET command instead of a GPIO pulse),
 * so the pins are hard-coded here rather than threaded through
 * display_init()'s 6 generic pin slots, matching the existing
 * convention of components/display/display_rgb.cpp hard-coding its
 * per-board Sunton pins internally.
 *
 * See https://github.com/Freenove/Freenove_ESP32_S3_Display for the
 * reference TFT_eSPI setup headers and Arduino sketches this backend
 * is derived from.
 */

#include <cstdio>
#include <cstring>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_heap_caps.h>
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <driver/ledc.h>
#include <esp_lcd_panel_io.h>

#include "display.h"

static const char *TAG = "DisplayILI9341";

/* Pins shared by every FNK0104 SPI-TFT SKU (A/B/S). */
#define FNK_LCD_MOSI_PIN   11
#define FNK_LCD_SCK_PIN    12
#define FNK_LCD_DC_PIN     46
#define FNK_LCD_CS_PIN     10
#define FNK_LCD_BL_PIN     45

#define FNK_SPI_HOST       SPI2_HOST

#if defined(CONFIG_DRAFTLING_DISPLAY_ST7796)
#define FNK_SPI_CLOCK_HZ   (80 * 1000 * 1000)
#else
#define FNK_SPI_CLOCK_HZ   (40 * 1000 * 1000)
#endif

/* LEDC PWM backlight, mirroring the AXS15231B backend's active-HIGH
 * default configuration (Guition JC3248W535 convention: duty MAX =
 * full brightness). */
#define BL_LEDC_TIMER       LEDC_TIMER_3
#define BL_LEDC_MODE        LEDC_LOW_SPEED_MODE
#define BL_LEDC_CHANNEL     LEDC_CHANNEL_1
#define BL_LEDC_DUTY_RES    LEDC_TIMER_8_BIT
#define BL_LEDC_DUTY_MAX    ((1 << 8) - 1)
#define BL_LEDC_FREQ_HZ     50000

static esp_lcd_panel_io_handle_t s_io_handle = NULL;

static uint16_t *s_fb = NULL;
static size_t    s_fb_pixels = 0;
static int s_width  = 0;
static int s_height = 0;

static int s_dirty_x1 = -1, s_dirty_y1 = -1, s_dirty_x2 = -1, s_dirty_y2 = -1;
static int s_clip_x = 0, s_clip_y = 0, s_clip_w = 0, s_clip_h = 0;

/* Byte-swapped scratch buffer for the whole dirty rectangle of a
 * single display_flush() call, sized to the full framebuffer so any
 * rect fits without reallocation. This must NOT be reused row-by-row
 * across separate esp_lcd_panel_io_tx_color() calls: that API queues
 * the transfer asynchronously (io_cfg.trans_queue_depth above 1) and
 * only blocks once the queue is full, so a buffer that is refilled
 * for the next row before the SPI/DMA hardware has actually read the
 * previous row out of it gets corrupted -- observed on real hardware
 * as the top portion of the screen rendering correctly and the rest
 * being shifted by a few rows once the queue saturates. Building the
 * whole rect into one buffer and issuing a single tx_color call (the
 * same pattern used by display_rlcd.cpp and display_st77922.cpp)
 * avoids the race entirely. PSRAM is fine here: the ESP32-S3's GDMA
 * can source SPI DMA transfers directly from PSRAM. */
static uint8_t *s_tx_buf = NULL;

/* Last user-requested backlight percent, cached so display_sleep() /
 * display_wake() can restore the brightness after blanking the panel
 * (mirrors the same cache in display_axs15231b.cpp). Initialised to
 * 100 to match backlight_pwm_init()'s initial full-brightness duty. */
static int s_bl_last_pct = 100;

static bool s_panel_asleep = false;

static void send_command(uint8_t cmd)
{
    esp_lcd_panel_io_tx_param(s_io_handle, cmd, NULL, 0);
}

static void send_data(const uint8_t *data, size_t n)
{
    esp_lcd_panel_io_tx_param(s_io_handle, -1, data, n);
}

static void send_cmd_data(uint8_t cmd, const uint8_t *data, size_t n)
{
    send_command(cmd);
    if (n > 0) send_data(data, n);
}

static void backlight_pwm_init(void)
{
    gpio_config_t g = {};
    g.intr_type    = GPIO_INTR_DISABLE;
    g.mode         = GPIO_MODE_OUTPUT;
    g.pin_bit_mask = (1ULL << FNK_LCD_BL_PIN);
    ESP_ERROR_CHECK(gpio_config(&g));
    gpio_set_level((gpio_num_t)FNK_LCD_BL_PIN, 1);

    ledc_timer_config_t t = {};
    t.speed_mode      = BL_LEDC_MODE;
    t.duty_resolution = BL_LEDC_DUTY_RES;
    t.timer_num       = BL_LEDC_TIMER;
    t.freq_hz         = BL_LEDC_FREQ_HZ;
    t.clk_cfg         = LEDC_USE_RC_FAST_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&t));

    ledc_channel_config_t c = {};
    c.gpio_num   = FNK_LCD_BL_PIN;
    c.speed_mode = BL_LEDC_MODE;
    c.channel    = BL_LEDC_CHANNEL;
    c.timer_sel  = BL_LEDC_TIMER;
    c.duty       = BL_LEDC_DUTY_MAX;
    c.hpoint     = 0;
    ESP_ERROR_CHECK(ledc_channel_config(&c));
}

extern "C" void display_set_backlight(int percent)
{
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    s_bl_last_pct = percent;
    uint32_t duty = (uint32_t)((BL_LEDC_DUTY_MAX * percent) / 100);
    ESP_ERROR_CHECK(ledc_set_duty(BL_LEDC_MODE, BL_LEDC_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(BL_LEDC_MODE, BL_LEDC_CHANNEL));
}

/* MADCTL value for landscape orientation (rotation "1" in the
 * Adafruit_ILI9341 / TFT_eSPI convention: MV | BGR). Both panels
 * ship natively portrait; setting the MV (row/column swap) bit
 * makes the controller present a landscape width x height
 * addressing window directly, so CASET/RASET below can be issued in
 * logical (already-landscape) coordinates with no host-side pixel
 * transpose needed. This orientation could not be verified against
 * physical hardware; if the image appears upside-down or mirrored
 * on a real board, try MADCTL 0xE8 (MY|MX|MV|BGR) instead. */
#define FNK_MADCTL_LANDSCAPE  0x28

static void ili9341_init_sequence(void)
{
    send_command(0x01); /* SWRESET */
    vTaskDelay(pdMS_TO_TICKS(150));

    static const uint8_t ef[]  = {0x03, 0x80, 0x02};
    static const uint8_t cf[]  = {0x00, 0xC1, 0x30};
    static const uint8_t ed[]  = {0x64, 0x03, 0x12, 0x81};
    static const uint8_t e8[]  = {0x85, 0x00, 0x78};
    static const uint8_t cb[]  = {0x39, 0x2C, 0x00, 0x34, 0x02};
    static const uint8_t f7[]  = {0x20};
    static const uint8_t ea[]  = {0x00, 0x00};
    static const uint8_t c0[]  = {0x23};
    static const uint8_t c1[]  = {0x10};
    static const uint8_t c5[]  = {0x3E, 0x28};
    static const uint8_t c7[]  = {0x86};
    static const uint8_t madctl[] = {FNK_MADCTL_LANDSCAPE};
    static const uint8_t colmod[] = {0x55};
    static const uint8_t b1[]  = {0x00, 0x18};
    static const uint8_t b6[]  = {0x08, 0x82, 0x27};
    static const uint8_t f2[]  = {0x00};
    static const uint8_t gamma_sel[] = {0x01};
    static const uint8_t e0[]  = {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E,
                                  0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00};
    static const uint8_t e1[]  = {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31,
                                  0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F};

    send_cmd_data(0xEF, ef, sizeof(ef));
    send_cmd_data(0xCF, cf, sizeof(cf));
    send_cmd_data(0xED, ed, sizeof(ed));
    send_cmd_data(0xE8, e8, sizeof(e8));
    send_cmd_data(0xCB, cb, sizeof(cb));
    send_cmd_data(0xF7, f7, sizeof(f7));
    send_cmd_data(0xEA, ea, sizeof(ea));
    send_cmd_data(0xC0, c0, sizeof(c0));
    send_cmd_data(0xC1, c1, sizeof(c1));
    send_cmd_data(0xC5, c5, sizeof(c5));
    send_cmd_data(0xC7, c7, sizeof(c7));
    send_cmd_data(0x36, madctl, sizeof(madctl));
    send_cmd_data(0x3A, colmod, sizeof(colmod));
    send_cmd_data(0xB1, b1, sizeof(b1));
    send_cmd_data(0xB6, b6, sizeof(b6));
    send_cmd_data(0xF2, f2, sizeof(f2));
    send_cmd_data(0x26, gamma_sel, sizeof(gamma_sel));
    send_cmd_data(0xE0, e0, sizeof(e0));
    send_cmd_data(0xE1, e1, sizeof(e1));

    /* Freenove's TFT_eSPI setup header for this panel defines
     * TFT_INVERSION_ON -- send INVON (0x21) so colors match the
     * vendor reference firmware. */
    send_command(0x21);

    send_command(0x11); /* SLPOUT */
    vTaskDelay(pdMS_TO_TICKS(150));
    send_command(0x29); /* DISPON */
}

static void st7796_init_sequence(void)
{
    send_command(0x01); /* SWRESET */
    vTaskDelay(pdMS_TO_TICKS(150));
    send_command(0x11); /* SLPOUT */
    vTaskDelay(pdMS_TO_TICKS(150));

    static const uint8_t cmd_lock_en1[]  = {0xC3};
    static const uint8_t cmd_lock_en2[]  = {0x96};
    static const uint8_t madctl[]        = {FNK_MADCTL_LANDSCAPE};
    static const uint8_t colmod[]        = {0x55};
    static const uint8_t b4[]            = {0x01};
    static const uint8_t b6[]            = {0x80, 0x02, 0x3B};
    static const uint8_t e8[]            = {0x40, 0x8A, 0x00, 0x00, 0x29,
                                            0x19, 0xA5, 0x33};
    static const uint8_t c1[]            = {0x06};
    static const uint8_t c2[]            = {0xA7};
    static const uint8_t c5[]            = {0x18};
    static const uint8_t e0[]            = {0xF0, 0x09, 0x0B, 0x06, 0x04, 0x15,
                                            0x2F, 0x54, 0x42, 0x3C, 0x17, 0x14,
                                            0x18, 0x1B};
    static const uint8_t e1[]            = {0xE0, 0x09, 0x0B, 0x06, 0x04, 0x03,
                                            0x2B, 0x43, 0x42, 0x3B, 0x16, 0x14,
                                            0x17, 0x1B};
    static const uint8_t cmd_lock_dis1[] = {0x3C};
    static const uint8_t cmd_lock_dis2[] = {0x69};

    send_cmd_data(0xF0, cmd_lock_en1, sizeof(cmd_lock_en1));
    send_cmd_data(0xF0, cmd_lock_en2, sizeof(cmd_lock_en2));
    send_cmd_data(0x36, madctl, sizeof(madctl));
    send_cmd_data(0x3A, colmod, sizeof(colmod));
    send_cmd_data(0xB4, b4, sizeof(b4));
    send_cmd_data(0xB6, b6, sizeof(b6));
    send_cmd_data(0xE8, e8, sizeof(e8));
    send_cmd_data(0xC1, c1, sizeof(c1));
    send_cmd_data(0xC2, c2, sizeof(c2));
    send_cmd_data(0xC5, c5, sizeof(c5));
    send_cmd_data(0xE0, e0, sizeof(e0));
    send_cmd_data(0xE1, e1, sizeof(e1));
    send_cmd_data(0xF0, cmd_lock_dis1, sizeof(cmd_lock_dis1));
    send_cmd_data(0xF0, cmd_lock_dis2, sizeof(cmd_lock_dis2));

    /* Freenove's TFT_eSPI setup header for this panel defines
     * TFT_INVERSION_ON -- send INVON (0x21) so colors match the
     * vendor reference firmware. */
    send_command(0x21);

    send_command(0x29); /* DISPON */
}

static void set_addr_window(int x, int y, int w, int h)
{
    int x2 = x + w - 1;
    int y2 = y + h - 1;
    uint8_t caset[] = {
        (uint8_t)((x  >> 8) & 0xFF), (uint8_t)(x  & 0xFF),
        (uint8_t)((x2 >> 8) & 0xFF), (uint8_t)(x2 & 0xFF)
    };
    uint8_t raset[] = {
        (uint8_t)((y  >> 8) & 0xFF), (uint8_t)(y  & 0xFF),
        (uint8_t)((y2 >> 8) & 0xFF), (uint8_t)(y2 & 0xFF)
    };
    send_cmd_data(0x2A, caset, sizeof(caset));
    send_cmd_data(0x2B, raset, sizeof(raset));
}

extern "C" void display_init(int /*pin_a*/, int /*pin_b*/, int /*pin_c*/,
                             int /*pin_d*/, int /*pin_e*/, int /*pin_f*/,
                             int width, int height)
{
    s_width  = width;
    s_height = height;

    spi_bus_config_t bus_cfg = {};
    bus_cfg.miso_io_num   = -1;
    bus_cfg.mosi_io_num   = FNK_LCD_MOSI_PIN;
    bus_cfg.sclk_io_num   = FNK_LCD_SCK_PIN;
    bus_cfg.quadwp_io_num = -1;
    bus_cfg.quadhd_io_num = -1;
    bus_cfg.max_transfer_sz = width * height * 2;
    ESP_ERROR_CHECK(spi_bus_initialize(FNK_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_spi_config_t io_cfg = {};
    io_cfg.dc_gpio_num      = (gpio_num_t)FNK_LCD_DC_PIN;
    io_cfg.cs_gpio_num      = (gpio_num_t)FNK_LCD_CS_PIN;
    io_cfg.pclk_hz          = FNK_SPI_CLOCK_HZ;
    io_cfg.lcd_cmd_bits     = 8;
    io_cfg.lcd_param_bits   = 8;
    io_cfg.spi_mode         = 0;
    io_cfg.trans_queue_depth = 10;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)FNK_SPI_HOST, &io_cfg, &s_io_handle));

    s_fb_pixels = (size_t)width * height;
    s_fb = (uint16_t *)heap_caps_malloc(s_fb_pixels * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    s_tx_buf = (uint8_t *)heap_caps_malloc(s_fb_pixels * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    assert(s_fb && s_tx_buf);
    memset(s_fb, 0, s_fb_pixels * sizeof(uint16_t));

    backlight_pwm_init();

#if defined(CONFIG_DRAFTLING_DISPLAY_ST7796)
    st7796_init_sequence();
#else
    ili9341_init_sequence();
#endif

    display_clear(0x00);
    display_full_refresh();

    ESP_LOGI(TAG, "FNK0104 SPI-TFT %dx%d initialized", width, height);
}

extern "C" void display_clear(uint8_t color)
{
    memset(s_fb, color ? 0xFF : 0x00, s_fb_pixels * sizeof(uint16_t));
    s_dirty_x1 = 0;
    s_dirty_y1 = 0;
    s_dirty_x2 = s_width  - 1;
    s_dirty_y2 = s_height - 1;
}

extern "C" void display_set_pixel(uint16_t x, uint16_t y, uint8_t color)
{
    if (x >= s_width || y >= s_height) return;
    s_fb[(size_t)y * s_width + x] = color ? 0xFFFF : 0x0000;
    if (s_dirty_x1 < 0) {
        s_dirty_x1 = s_dirty_x2 = x;
        s_dirty_y1 = s_dirty_y2 = y;
    } else {
        if (x < s_dirty_x1) s_dirty_x1 = x;
        if (x > s_dirty_x2) s_dirty_x2 = x;
        if (y < s_dirty_y1) s_dirty_y1 = y;
        if (y > s_dirty_y2) s_dirty_y2 = y;
    }
}

extern "C" bool display_push_rgb565(int x, int y, int w, int h, const void *color_map)
{
    if (w <= 0 || h <= 0) return true;
    int x2 = x + w - 1;
    int y2 = y + h - 1;
    if (x2 >= s_width)  x2 = s_width  - 1;
    if (y2 >= s_height) y2 = s_height - 1;
    if (x < 0 || y < 0 || x2 < x || y2 < y) return true;

    const uint16_t *src = (const uint16_t *)color_map;
    for (int row = 0; row < (y2 - y + 1); row++) {
        uint16_t *dst = s_fb + (size_t)(y + row) * s_width + x;
        memcpy(dst, src + (size_t)row * w, (size_t)(x2 - x + 1) * sizeof(uint16_t));
    }

    if (s_dirty_x1 < 0) {
        s_dirty_x1 = x;  s_dirty_y1 = y;
        s_dirty_x2 = x2; s_dirty_y2 = y2;
    } else {
        if (x  < s_dirty_x1) s_dirty_x1 = x;
        if (x2 > s_dirty_x2) s_dirty_x2 = x2;
        if (y  < s_dirty_y1) s_dirty_y1 = y;
        if (y2 > s_dirty_y2) s_dirty_y2 = y2;
    }
    return true;
}

extern "C" void display_set_partial_clip(int x, int y, int w, int h)
{
    if (w <= 0 || h <= 0) {
        s_clip_w = s_clip_h = 0;
        return;
    }
    s_clip_x = x; s_clip_y = y; s_clip_w = w; s_clip_h = h;
}

extern "C" void display_flush(void)
{
    if (s_dirty_x1 < 0) return;
    if (s_panel_asleep) {
        s_dirty_x1 = s_dirty_y1 = s_dirty_x2 = s_dirty_y2 = -1;
        s_clip_w = s_clip_h = 0;
        return;
    }

    int x1 = s_dirty_x1, y1 = s_dirty_y1;
    int x2 = s_dirty_x2, y2 = s_dirty_y2;

    if (s_clip_w > 0 && s_clip_h > 0) {
        int cx2 = s_clip_x + s_clip_w - 1;
        int cy2 = s_clip_y + s_clip_h - 1;
        if (s_clip_x > x1) x1 = s_clip_x;
        if (s_clip_y > y1) y1 = s_clip_y;
        if (cx2      < x2) x2 = cx2;
        if (cy2      < y2) y2 = cy2;
        s_clip_w = s_clip_h = 0;
    }
    s_dirty_x1 = s_dirty_y1 = s_dirty_x2 = s_dirty_y2 = -1;

    if (x1 > x2 || y1 > y2) return;
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= s_width)  x2 = s_width  - 1;
    if (y2 >= s_height) y2 = s_height - 1;

    int w = x2 - x1 + 1;
    int h = y2 - y1 + 1;

    set_addr_window(x1, y1, w, h);
    send_command(0x2C); /* RAMWR */

    uint8_t *dst = s_tx_buf;
    for (int row = 0; row < h; row++) {
        const uint16_t *src = s_fb + (size_t)(y1 + row) * s_width + x1;
        for (int col = 0; col < w; col++) {
            uint16_t px = src[col];
            *dst++ = (uint8_t)(px >> 8);
            *dst++ = (uint8_t)(px & 0xFF);
        }
    }
    esp_lcd_panel_io_tx_color(s_io_handle, -1, s_tx_buf, (size_t)w * h * 2);
}

extern "C" void display_full_refresh(void)
{
    s_dirty_x1 = 0;
    s_dirty_y1 = 0;
    s_dirty_x2 = s_width  - 1;
    s_dirty_y2 = s_height - 1;
    display_flush();
}

extern "C" void display_request_full_refresh(void)
{
    /* Color LCD, no partial-refresh state machine to latch. */
}

extern "C" uint8_t *display_get_buffer(void)
{
    return (uint8_t *)s_fb;
}

extern "C" int display_get_buffer_size(void)
{
    return (int)(s_fb_pixels * sizeof(uint16_t));
}

extern "C" void display_sleep(void)
{
    if (s_panel_asleep) return;
    s_panel_asleep = true;
    /* Capture the user's brightness BEFORE display_set_backlight(0)
     * overwrites s_bl_last_pct, so display_wake() can restore it
     * accurately. */
    int saved_pct = s_bl_last_pct;
    display_set_backlight(0);
    s_bl_last_pct = saved_pct;
    send_command(0x28); /* DISPOFF */
    send_command(0x10); /* SLPIN */
}

extern "C" void display_wake(void)
{
    if (!s_panel_asleep) return;
    send_command(0x11); /* SLPOUT */
    vTaskDelay(pdMS_TO_TICKS(120));
    send_command(0x29); /* DISPON */
    display_set_backlight(s_bl_last_pct);
    s_panel_asleep = false;
    display_request_full_refresh();
}

extern "C" void display_deep_sleep_prepare(void)
{
    display_set_backlight(0);
    gpio_hold_en((gpio_num_t)FNK_LCD_BL_PIN);
    gpio_deep_sleep_hold_en();
}

extern "C" void display_set_shared_i2c_bus(void * /*bus_handle*/)
{
    /* This backend does not use I2C. No-op. */
}

#endif /* CONFIG_DRAFTLING_DISPLAY_ILI9341 || CONFIG_DRAFTLING_DISPLAY_ST7796 */
