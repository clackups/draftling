#include "sdkconfig.h"
#if defined(CONFIG_DRAFTLING_DISPLAY_ST7789)

/*
 * RockBase NM-CYD-C5 ST7789 SPI color LCD driver.
 *
 * Hardware
 * --------
 * 2.8" IPS panel, 320x240 landscape, driven over a standard 4-wire
 * SPI interface (SCK/MISO/MOSI/CS/DC). The physical SPI bus (SCK=6,
 * MISO=2, MOSI=7) is shared with the on-board MicroSD slot (and, on
 * boards that wire it, the XPT2046 resistive touch controller), each
 * on its own CS line -- see main/boards/nm_cyd_c5.h.
 *
 * Strategy
 * --------
 * Unlike the AXS15231B QSPI boards (which need a hand-rolled vendor
 * command sequence), ST7789 is supported directly by the stock
 * ESP-IDF `esp_lcd` component's esp_lcd_new_panel_st7789() API
 * (components/esp_lcd/src/esp_lcd_panel_st7789.c upstream), which
 * implements the full panel init sequence internally. No managed
 * component is needed -- ST7789 has been bundled in-tree since IDF
 * 5.x, unlike the third-party `jbrilha/esp_lcd_st7789` registry
 * package (there is no `espressif/esp_lcd_st7789` package at all).
 * We only need to wire up the generic esp_lcd SPI panel-IO layer and
 * hand the resulting handles to that driver, then push RGB565 tiles
 * via esp_lcd_panel_draw_bitmap() -- no local framebuffer is
 * required (there is no partial-refresh ghosting to manage, unlike
 * e-paper).
 */

#include <cstdio>
#include <cstring>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_heap_caps.h>
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <driver/spi_master.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_st7789.h>

#include "display.h"

static const char *TAG = "DisplayST7789";

static esp_lcd_panel_io_handle_t s_io    = NULL;
static esp_lcd_panel_handle_t    s_panel = NULL;
static int s_width  = 0;
static int s_height = 0;

/* Host-side R/B swap scratch buffer. See the doc comment on
 * rb_swap_buffer() for why this exists in addition to (not instead
 * of) panel_cfg.rgb_ele_order below. Sized lazily to the largest
 * tile ever requested by display_push_rgb565() (in practice the
 * full screen, since lvgl_port pushes the LVGL flush area as one
 * tile). */
static uint16_t *s_rb_swap_buf    = NULL;
static size_t    s_rb_swap_pixels = 0;

/* ---- Backlight (LEDC PWM), active-HIGH ---- */
#define BL_LEDC_TIMER   LEDC_TIMER_1
#define BL_LEDC_MODE    LEDC_LOW_SPEED_MODE
#define BL_LEDC_CHANNEL LEDC_CHANNEL_1
#define BL_LEDC_RES     LEDC_TIMER_10_BIT
#define BL_DUTY_MAX     ((1 << 10) - 1)

static int  s_bl_pin = -1;
static bool s_bl_ready = false;

static void backlight_pwm_init(int bl_pin)
{
    s_bl_pin = bl_pin;
    if (bl_pin < 0) return;

    ledc_timer_config_t t = {};
    t.speed_mode      = BL_LEDC_MODE;
    t.timer_num       = BL_LEDC_TIMER;
    t.duty_resolution = BL_LEDC_RES;
    t.freq_hz         = 5000;
    t.clk_cfg         = LEDC_AUTO_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&t));

    ledc_channel_config_t c = {};
    c.speed_mode = BL_LEDC_MODE;
    c.channel    = BL_LEDC_CHANNEL;
    c.timer_sel  = BL_LEDC_TIMER;
    c.gpio_num   = bl_pin;
    c.duty       = BL_DUTY_MAX; /* start fully on */
    c.hpoint     = 0;
    ESP_ERROR_CHECK(ledc_channel_config(&c));
    s_bl_ready = true;
}

extern "C" void display_set_backlight(int percent)
{
    if (!s_bl_ready || s_bl_pin < 0) return;
    if (percent < 0)   percent = 0;
    if (percent > 100) percent = 100;
    uint32_t duty = (uint32_t)((percent * BL_DUTY_MAX) / 100);
    ESP_ERROR_CHECK(ledc_set_duty(BL_LEDC_MODE, BL_LEDC_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(BL_LEDC_MODE, BL_LEDC_CHANNEL));
}

extern "C" void display_init(int /*pin_a*/, int /*pin_b*/, int /*pin_c*/,
                             int /*pin_d*/, int /*pin_e*/, int /*pin_f*/,
                             int /*width*/, int /*height*/)
{
    /* The ST7789 backend needs more pins (spi_host, dc, bl, ...) than
     * display_init()'s 6 generic pin slots hold, so it has its own
     * struct-based init (display_st7789_init). Calling the generic
     * entry point is a build-configuration error. */
    ESP_LOGE(TAG, "display_init() called on ST7789 backend; use "
                  "display_st7789_init() instead");
}

extern "C" void display_st7789_init(const display_st7789_config_t *cfg)
{
    s_width  = cfg->width;
    s_height = cfg->height;

    spi_bus_config_t bus_cfg = {};
    bus_cfg.mosi_io_num     = cfg->mosi;
    bus_cfg.miso_io_num     = cfg->miso;
    bus_cfg.sclk_io_num     = cfg->sck;
    bus_cfg.quadwp_io_num   = -1;
    bus_cfg.quadhd_io_num   = -1;
    bus_cfg.max_transfer_sz = cfg->width * cfg->height * sizeof(uint16_t);
    ESP_ERROR_CHECK(spi_bus_initialize((spi_host_device_t)cfg->spi_host,
                                       &bus_cfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_spi_config_t io_cfg = {};
    io_cfg.dc_gpio_num       = (gpio_num_t)cfg->dc;
    io_cfg.cs_gpio_num       = (gpio_num_t)cfg->cs;
    io_cfg.pclk_hz           = 40 * 1000 * 1000;
    io_cfg.lcd_cmd_bits      = 8;
    io_cfg.lcd_param_bits    = 8;
    io_cfg.spi_mode          = 0;
    io_cfg.trans_queue_depth = 10;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)cfg->spi_host,
                                             &io_cfg, &s_io));

    esp_lcd_panel_dev_config_t panel_cfg = {};
    panel_cfg.reset_gpio_num = (gpio_num_t)cfg->rst;
    /* CYD-family ST7789 panels wire the color filter as BGR, not the
     * esp_lcd default RGB (confirmed against multiple independent
     * ESP32-2432S028R "Cheap Yellow Display" reference configs, e.g.
     * TFT_eSPI's TFT_RGB_ORDER). Symmetric colors (black/white/gray/
     * green, i.e. R==B) are unaffected either way, but asymmetric
     * theme colors (e.g. the Orange theme) come out with red and
     * blue swapped without compensating for this somewhere.
     *
     * panel_cfg.rgb_ele_order (LCD_RGB_ELEMENT_ORDER_BGR) is left at
     * its default (RGB) here on purpose: setting it to BGR was tried
     * first, asking esp_lcd_new_panel_st7789() to program the
     * controller's MADCTL BGR bit for us, but this did not correct
     * the swap on real hardware (the Orange-on-black theme still
     * rendered blue-on-black) -- either this IDF release's ST7789
     * driver does not wire that field through to MADCTL, or the
     * controller ignores/does not implement that bit. Instead we
     * swap the R and B fields of every pixel on the host side in
     * display_push_rgb565() (see rb_swap_buffer()), the same
     * "compensate in software, don't rely on an unverified vendor
     * flag" strategy already used by the sibling AXS15231B backend
     * for its byte-endian issue. */
    /* esp_lcd_panel_dev_config_t.data_endian defaults to
     * LCD_RGB_DATA_ENDIAN_BIG (matching the ST7789's own RAMCTRL
     * reset default), but display_push_rgb565() hands LVGL's
     * lv_color_t buffer to esp_lcd_panel_draw_bitmap() unswapped --
     * and lv_color_t is native little-endian on this target. Without
     * this line every 16-bit pixel is byte-swapped on the wire,
     * which combined with invert_colors above reproduced exactly the
     * reported "cyan/light-green text on white background" bug
     * (black 0x0000 is byte-swap-symmetric, so it only inverts to
     * white; green 0x07E0 swaps to 0xE007 and then inverts to
     * 0x1FF8, i.e. R=9% G=100% B=77% -- cyan-ish light green). Set
     * to LITTLE so the panel is told to expect the data exactly as
     * LVGL already produces it, matching the sibling AXS15231B
     * backend's *result* without needing that backend's manual
     * host-side byte-swap. */
    panel_cfg.data_endian    = LCD_RGB_DATA_ENDIAN_LITTLE;
    panel_cfg.bits_per_pixel = 16;
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(s_io, &panel_cfg, &s_panel));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(s_panel, cfg->invert_colors));
    /* The 320x240 landscape panel is scanned out natively in
     * 240 (W) x 320 (H) portrait order. Landscape rotation on this
     * panel family needs both swap_xy (MADCTL MV) *and* mirror_x
     * (MADCTL MX) -- confirmed against TFT_eSPI's ST7789_Rotation.h
     * rotation-1 MADCTL value (TFT_MAD_MX | TFT_MAD_MV). swap_xy
     * alone produces a diagonal transpose that reads as a mirror
     * image (matches the reported "had to use a physical mirror to
     * read it" bug). */
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(s_panel, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(s_panel, true, false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel, true));

    backlight_pwm_init(cfg->bl);

    ESP_LOGI(TAG, "ST7789 %dx%d initialized", s_width, s_height);
}

extern "C" void display_clear(uint8_t color)
{
    if (!s_panel) return;
    uint16_t fill = color ? 0xFFFF : 0x0000;
    uint16_t *row = (uint16_t *)heap_caps_malloc((size_t)s_width * sizeof(uint16_t),
                                                 MALLOC_CAP_SPIRAM);
    if (!row) return;
    for (int x = 0; x < s_width; x++) row[x] = fill;
    for (int y = 0; y < s_height; y++) {
        esp_lcd_panel_draw_bitmap(s_panel, 0, y, s_width, y + 1, row);
    }
    heap_caps_free(row);
}

extern "C" void display_set_pixel(uint16_t /*x*/, uint16_t /*y*/, uint8_t /*color*/)
{
    /* Color backend: per-pixel setting is unused (lvgl_port always
     * pushes RGB565 tiles via display_push_rgb565). */
}

/* Swap the R and B 5-bit fields of every RGB565 pixel in [src, src+n)
 * into a lazily-(re)sized scratch buffer, returning it (or NULL on
 * allocation failure, in which case the caller should fall back to
 * sending src unmodified rather than drop the tile).
 *
 * This compensates in software for the CYD-family panel's BGR
 * subpixel wiring (see the panel_cfg.rgb_ele_order comment in
 * display_st7789_init()): asking the controller to do this via its
 * MADCTL BGR bit did not work on real hardware, so every tile is
 * pre-swapped here instead, independent of whatever the controller
 * does with its own color-order setting. RGB565 packs
 * R[15:11] G[10:5] B[4:0]; swapping only exchanges the R and B
 * fields, leaving G (and therefore symmetric colors where R==B, like
 * black/white/gray/green) unchanged either way. */
static const uint16_t *rb_swap_buffer(const uint16_t *src, size_t n)
{
    if (n > s_rb_swap_pixels) {
        uint16_t *buf = (uint16_t *)heap_caps_realloc(
            s_rb_swap_buf, n * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
        if (!buf) {
            ESP_LOGE(TAG, "rb_swap_buffer: alloc of %u px failed",
                     (unsigned)n);
            return NULL;
        }
        s_rb_swap_buf    = buf;
        s_rb_swap_pixels = n;
    }
    for (size_t i = 0; i < n; i++) {
        uint16_t px = src[i];
        uint16_t r  = (px >> 11) & 0x1F;
        uint16_t g  = (px >> 5)  & 0x3F;
        uint16_t b  = px & 0x1F;
        s_rb_swap_buf[i] = (uint16_t)((b << 11) | (g << 5) | r);
    }
    return s_rb_swap_buf;
}

extern "C" bool display_push_rgb565(int x, int y, int w, int h,
                                    const void *color_map)
{
    if (!s_panel)         return true;
    if (w <= 0 || h <= 0) return true;

    int x2 = x + w;
    int y2 = y + h;
    if (x2 > s_width)  x2 = s_width;
    if (y2 > s_height) y2 = s_height;
    if (x2 <= x || y2 <= y) return true;

    const uint16_t *swapped = rb_swap_buffer(
        (const uint16_t *)color_map, (size_t)w * (size_t)h);
    esp_lcd_panel_draw_bitmap(s_panel, x, y, x2, y2,
                              swapped ? swapped : color_map);
    return true;
}

extern "C" void display_set_partial_clip(int /*x*/, int /*y*/,
                                         int /*w*/, int /*h*/)
{
    /* No dirty-bbox refresh state machine on this backend; every
     * push_rgb565 tile is written immediately. No-op. */
}

extern "C" void display_flush(void)
{
    /* No-op: esp_lcd_panel_draw_bitmap() in display_push_rgb565()
     * already streamed the change to the panel over SPI. */
}

extern "C" void display_full_refresh(void)
{
    /* No-op (no e-paper waveforms to clear). */
}

extern "C" void display_request_full_refresh(void)
{
    /* No-op: nothing to latch. */
}

extern "C" uint8_t *display_get_buffer(void)
{
    /* No local framebuffer is kept; every write goes straight to the
     * panel via esp_lcd_panel_draw_bitmap(). */
    return NULL;
}

extern "C" int display_get_buffer_size(void)
{
    return 0;
}

extern "C" void display_sleep(void)
{
    display_set_backlight(0);
    if (s_panel) esp_lcd_panel_disp_on_off(s_panel, false);
}

extern "C" void display_wake(void)
{
    if (s_panel) esp_lcd_panel_disp_on_off(s_panel, true);
}

extern "C" void display_deep_sleep_prepare(void)
{
    display_set_backlight(0);
    if (s_panel) esp_lcd_panel_disp_on_off(s_panel, false);
}

extern "C" void display_set_shared_i2c_bus(void * /*bus_handle*/)
{
    /* ST7789 does not use I2C. No-op. */
}

#endif /* CONFIG_DRAFTLING_DISPLAY_ST7789 */
