#include "sdkconfig.h"
#if defined(CONFIG_DRAFTLING_DISPLAY_ST77922)

/*
 * Freenove FNK0104N: 3.5" 320x480 IPS panel with a Sitronix ST77922
 * TDDI controller (display + touch in one chip) on a 4-lane QSPI bus.
 *
 * Earlier attempts at this board (the last one removed in commit
 * 7412216) left the screen black. The cause was not the panel
 * protocol but an ESP-IDF bug triggered by SPICOMMON_BUSFLAG_QUAD on
 * these SPI2 IOMUX pins -- see the comment in display_init(). With
 * that flag dropped, both Espressif's esp_lcd_st77922 component and
 * this backend drive the panel.
 *
 * This backend ports Freenove's own Arduino driver
 * (Libraries/FNK0104N/TFT_eSPI_v2.5.43.zip -> TFT_eSPI/ST77922.cpp in
 * github.com/Freenove/Freenove_ESP32_S3_Display) onto the raw ESP-IDF
 * spi_master API:
 *
 *   - register write: opcode 0x02 (1 lane), 24-bit address
 *     0x00 <cmd> 0x00 (1 lane), parameters on 1 lane;
 *   - pixel write: opcode 0x32 (1 lane), address 0x002C00 (RAMWR) for
 *     the first chunk of a burst and 0x003C00 (RAMWRC) for every later
 *     one, pixel data on 4 lanes, big-endian RGB565 (COLMOD 0x01);
 *   - SWRESET, then the vendor init table (st77922_init_cmds[]). The
 *     panel's RESET pin is wired to the ESP32-S3's CHIP_PU (EN) line,
 *     which a software or USB reset of the chip does not toggle.
 *
 * Freenove's own XiaoZhi firmware for this board
 * (Upload_Xiaozhi_Bin/3.5inch) embeds the identical 63-entry table.
 *
 * Wiring (Freenove 3.5inch_ESP32-S3_Display_Schematic.pdf, which
 * matches the pin defines in the vendor driver): CS=10, SCK=12,
 * SDA0..3=11/13/14/9 (the ESP32-S3 SPI2 IOMUX pins), backlight=41
 * through a BSS138 low-side switch (active high), TE=42 (unused).
 * The panel's interface-mode straps (IM0/IM1 high, IM2 low = QSPI)
 * are fixed on the board.
 *
 * display_init() logs RDDPM (0x0A) after the init table as a health
 * check: 0x9C means booster on, sleep out, normal mode, display on;
 * 0xFF means the panel is not answering.
 *
 * The backend works in the panel's native portrait frame (320x480);
 * the LVGL port's 90-degree base rotation (DRAFTLING_DISPLAY_ROTATE)
 * turns it landscape, the same way as display_rgb.cpp on the
 * Waveshare ESP32-S3-LCD-3.16. ST77922 windows must start and end on
 * 4-pixel column boundaries ("pixel number must be a multiple of 4"
 * in the datasheet), so every flush is widened to that grid.
 */

#include <cstring>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_heap_caps.h>
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <driver/spi_master.h>

#include "display.h"
#include "display_margins.h"

static const char *TAG = "DisplayST77922";

#define ST_CS_PIN       10
#define ST_SCK_PIN      12
#define ST_D0_PIN       11
#define ST_D1_PIN       13
#define ST_D2_PIN       14
#define ST_D3_PIN       9
#define ST_BL_PIN       41

#define ST_SPI_HOST     SPI2_HOST
/* The vendor driver and Freenove's XiaoZhi firmware run the bus at
 * 80 MHz. Writes work there too, but register reads come back shifted
 * by one bit, so stay at 40 MHz (a full frame still takes ~15 ms). */
#define ST_SPI_CLOCK_HZ (40 * 1000 * 1000)

#define ST_OP_WRITE_REG 0x02
#define ST_OP_WRITE_PIX 0x32
#define ST_OP_READ      0x0B

/* Pixels per QSPI pixel transaction (the vendor uses 0x4000). The
 * chunk buffer is internal DMA-capable RAM, so the DMA never reads
 * PSRAM while the bus runs at full speed. */
#define ST_CHUNK_PIXELS 8192

#define BL_LEDC_TIMER    LEDC_TIMER_3
#define BL_LEDC_MODE     LEDC_LOW_SPEED_MODE
#define BL_LEDC_CHANNEL  LEDC_CHANNEL_1
#define BL_LEDC_DUTY_MAX ((1 << 8) - 1)
#define BL_LEDC_FREQ_HZ  20000

struct st_init_cmd_t {
    uint8_t  cmd;
    uint8_t  len;
    uint16_t delay_ms;
    uint8_t  data[16];
};

/* Vendor init sequence, byte for byte (TFT_eSPI/ST77922.cpp,
 * st77922_lcd_init[]). Do not reorder or "clean up". Ends with
 * INVON, SLPOUT, DISPON, RAMWR, COLMOD = 0x01 (RGB565 on the
 * ST77922), MADCTL = 0 and TEON. */
static const st_init_cmd_t st77922_init_cmds[] = {
    {0xF1, 1, 0, {0x00}},
    {0x60, 3, 0, {0x00, 0x00, 0x00}},
    {0x65, 1, 0, {0x80}},
    {0x79, 1, 0, {0x06}},
    {0x7B, 3, 0, {0x00, 0x08, 0x08}},
    {0x80, 11, 0, {0x55, 0x62, 0x2F, 0x17, 0xF0, 0x52, 0x70, 0xD2, 0x52, 0x62, 0xEA}},
    {0x81, 4, 0, {0x26, 0x52, 0x72, 0x27}},
    {0x84, 2, 0, {0x92, 0x25}},
    {0x87, 6, 0, {0x10, 0x10, 0x58, 0x00, 0x02, 0x3A}},
    {0x88, 15, 0, {0x00, 0x00, 0x2C, 0x10, 0x04, 0x00, 0x00, 0x00, 0x01, 0x01,
                   0x01, 0x01, 0x01, 0x00, 0x06}},
    {0x89, 3, 0, {0x00, 0x00, 0x00}},
    {0x8A, 11, 0, {0x13, 0x00, 0x2C, 0x00, 0x00, 0x2C, 0x10, 0x10, 0x00, 0x3E, 0x19}},
    {0x8B, 9, 0, {0x15, 0xB1, 0xB1, 0x44, 0x96, 0x2C, 0x10, 0x97, 0x8E}},
    {0x8C, 13, 0, {0x1D, 0xB1, 0xB1, 0x44, 0x96, 0x2C, 0x10, 0x50, 0x0F, 0x01,
                   0xC5, 0x12, 0x09}},
    {0x8D, 1, 0, {0x0C}},
    {0x8E, 6, 0, {0x33, 0x01, 0x0C, 0x13, 0x01, 0x01}},
    {0xB3, 2, 0, {0x00, 0x30}},
    {0xF1, 1, 0, {0x00}},
    {0x71, 1, 0, {0xD0}},
    {0x66, 2, 0, {0x02, 0x3F}},
    {0xBE, 3, 0, {0x26, 0x00, 0x9D}},
    {0x70, 12, 0, {0x01, 0xA0, 0x11, 0x40, 0xE0, 0x00, 0x11, 0x69, 0x11, 0x00,
                   0x00, 0x1A}},
    {0x90, 9, 0, {0x04, 0x04, 0x55, 0x74, 0x00, 0x40, 0x43, 0x27, 0x27}},
    {0x91, 9, 0, {0x04, 0x04, 0x55, 0x75, 0x00, 0x40, 0x42, 0x27, 0x27}},
    {0x92, 10, 0, {0x04, 0x44, 0x55, 0xC0, 0x06, 0x00, 0x07, 0x05, 0x90, 0x27}},
    {0x93, 10, 0, {0x04, 0x43, 0x11, 0x00, 0x00, 0x00, 0x00, 0x05, 0x90, 0x27}},
    {0x94, 6, 0, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {0x95, 5, 0, {0x96, 0x16, 0x00, 0x00, 0xFF}},
    {0x96, 12, 0, {0x44, 0x53, 0x03, 0x12, 0x23, 0x24, 0x06, 0x05, 0x94, 0x27,
                   0x00, 0x44}},
    {0x97, 12, 0, {0x44, 0x53, 0x47, 0x56, 0x20, 0x20, 0x02, 0x01, 0x94, 0x27,
                   0x00, 0x44}},
    {0xBA, 5, 0, {0x55, 0x94, 0x2D, 0x94, 0x27}},
    {0x9A, 7, 0, {0x40, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00}},
    {0x9B, 7, 0, {0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00}},
    {0x9C, 13, 0, {0x5C, 0x12, 0x00, 0x00, 0x10, 0x12, 0x00, 0x00, 0x10, 0x02,
                   0x00, 0x00, 0x00}},
    {0x9D, 8, 0, {0x8A, 0x51, 0x00, 0x00, 0x00, 0x80, 0x1E, 0x01}},
    {0x9E, 7, 0, {0x51, 0x00, 0x00, 0x00, 0x80, 0x1E, 0x01}},
    {0xB4, 12, 0, {0x1D, 0x1C, 0x1E, 0x0B, 0x14, 0x02, 0x13, 0x09, 0x1E, 0x00,
                   0x1E, 0x10}},
    {0xB5, 12, 0, {0x1D, 0x1C, 0x1E, 0x0A, 0x15, 0x03, 0x11, 0x08, 0x1E, 0x01,
                   0x1E, 0x12}},
    {0xB6, 7, 0, {0x77, 0x77, 0x00, 0x0A, 0xFF, 0x0A, 0xFF}},
    {0x86, 14, 0, {0xCD, 0x04, 0xB1, 0x02, 0x58, 0x12, 0x58, 0x0C, 0x13, 0x01,
                   0xA5, 0x00, 0xA5, 0xA5}},
    {0xB7, 16, 0, {0x07, 0x0A, 0x0E, 0x06, 0x05, 0x03, 0x2B, 0x03, 0x03, 0x42,
                   0x07, 0x10, 0x10, 0x2E, 0x3F, 0x0D}},
    {0xB8, 16, 0, {0x07, 0x0A, 0x0D, 0x05, 0x05, 0x02, 0x2B, 0x02, 0x03, 0x42,
                   0x06, 0x10, 0x0F, 0x2E, 0x3F, 0x0D}},
    {0xB9, 2, 0, {0x23, 0x23}},
    {0xBF, 6, 0, {0x10, 0x14, 0x14, 0x0B, 0x0B, 0x0B}},
    {0xF2, 1, 0, {0x00}},
    {0x73, 5, 0, {0x04, 0xDA, 0x12, 0x54, 0x47}},
    {0x77, 5, 0, {0x6B, 0x5B, 0xFD, 0xC3, 0xC5}},
    {0x7A, 2, 0, {0x15, 0x27}},
    {0x7B, 2, 0, {0x04, 0x57}},
    {0x7E, 2, 0, {0x01, 0x0E}},
    {0xBF, 1, 0, {0x36}},
    {0xE3, 2, 0, {0x40, 0x40}},
    {0xF0, 1, 0, {0x00}},
    {0xD0, 1, 0, {0x00}},
    {0x2A, 4, 0, {0x00, 0x00, 0x01, 0x3F}},
    {0x2B, 4, 0, {0x00, 0x00, 0x01, 0xDF}},
    {0x21, 0, 0, {0}},                     /* INVON */
    {0x11, 0, 120, {0}},                   /* SLPOUT */
    {0x29, 0, 0, {0}},                     /* DISPON */
    {0x2C, 0, 0, {0}},                     /* RAMWR */
    {0x3A, 1, 0, {0x01}},                  /* COLMOD: RGB565 */
    {0x36, 1, 0, {0x00}},                  /* MADCTL */
    {0x35, 1, 20, {0x01}},                 /* TEON */
};

static spi_device_handle_t s_spi = NULL;

/* Native portrait framebuffer (s_width x s_height), PSRAM. */
static uint16_t *s_fb = NULL;
static size_t    s_fb_pixels = 0;
static int s_width  = 0;
static int s_height = 0;

/* Byte-swapped pixel chunk, internal DMA memory. */
static uint16_t *s_chunk = NULL;

static int s_dirty_x1 = -1, s_dirty_y1 = -1, s_dirty_x2 = -1, s_dirty_y2 = -1;
static int s_bl_last_pct = 100;
static bool s_panel_asleep = false;

static void write_reg(uint8_t cmd, const uint8_t *data, size_t len)
{
    spi_transaction_ext_t t = {};
    t.base.flags   = SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR;
    t.base.cmd     = ST_OP_WRITE_REG;
    t.base.addr    = (uint32_t)cmd << 8;
    t.command_bits = 8;
    t.address_bits = 24;
    t.base.length    = len * 8;
    t.base.tx_buffer = len ? data : NULL;
    ESP_ERROR_CHECK(spi_device_polling_transmit(s_spi, &t.base));
}

/* Read `len` bytes of register `cmd`: opcode 0x0B and the 24-bit
 * address 0x00 <cmd> 0x00, then the reply on a single lane (sampled on
 * SDA1 / GPIO13), with no dummy clocks -- the framing of esp_lcd's
 * rx_param, which reads this panel correctly at 40 MHz. Only used for
 * the RDDPM health check in display_init(). */
static esp_err_t read_reg(uint8_t cmd, uint8_t *out, size_t len)
{
    spi_transaction_ext_t t = {};
    t.base.flags   = SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR;
    t.base.cmd     = ST_OP_READ;
    t.base.addr    = (uint32_t)cmd << 8;
    t.command_bits = 8;
    t.address_bits = 24;
    t.base.rxlength  = len * 8;
    t.base.rx_buffer = out;
    return spi_device_polling_transmit(s_spi, &t.base);
}

static void log_readback(const char *name, uint8_t cmd, size_t len)
{
    uint8_t v[4] = {};
    esp_err_t err = read_reg(cmd, v, len);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "%s (0x%02X) read failed: %s", name, cmd, esp_err_to_name(err));
        return;
    }
    char s[16] = {};
    for (size_t i = 0; i < len; i++) snprintf(s + i * 3, sizeof(s) - i * 3, "%02X ", v[i]);
    ESP_LOGI(TAG, "%s (0x%02X): %s", name, cmd, s);
}

static void send_init_table(void)
{
    for (size_t i = 0; i < sizeof(st77922_init_cmds) / sizeof(st77922_init_cmds[0]); i++) {
        const st_init_cmd_t *c = &st77922_init_cmds[i];
        write_reg(c->cmd, c->data, c->len);
        if (c->delay_ms) vTaskDelay(pdMS_TO_TICKS(c->delay_ms));
    }
}

static void set_window(int x1, int y1, int x2, int y2)
{
    const uint8_t xa[4] = { (uint8_t)(x1 >> 8), (uint8_t)x1,
                            (uint8_t)(x2 >> 8), (uint8_t)x2 };
    const uint8_t ya[4] = { (uint8_t)(y1 >> 8), (uint8_t)y1,
                            (uint8_t)(y2 >> 8), (uint8_t)y2 };
    write_reg(0x2A, xa, 4);   /* CASET */
    write_reg(0x2B, ya, 4);   /* RASET */
}

/* Send a window's worth of pixels produced row by row by `fill`
 * (writes `n` big-endian pixels for linear pixel index `start`). */
typedef void (*pixel_fill_fn)(uint16_t *dst, size_t start, size_t n, void *ctx);

static void write_pixels(int x1, int y1, int x2, int y2, pixel_fill_fn fill, void *ctx)
{
    set_window(x1, y1, x2, y2);
    size_t total = (size_t)(x2 - x1 + 1) * (size_t)(y2 - y1 + 1);
    size_t done = 0;
    while (done < total) {
        size_t n = total - done;
        if (n > ST_CHUNK_PIXELS) n = ST_CHUNK_PIXELS;
        fill(s_chunk, done, n, ctx);
        spi_transaction_ext_t t = {};
        t.base.flags   = SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR |
                         SPI_TRANS_MODE_QIO;
        t.base.cmd     = ST_OP_WRITE_PIX;
        t.base.addr    = (done == 0 ? 0x2Cu : 0x3Cu) << 8;   /* RAMWR, then RAMWRC */
        t.command_bits = 8;
        t.address_bits = 24;
        t.base.length    = n * 16;
        t.base.tx_buffer = s_chunk;
        ESP_ERROR_CHECK(spi_device_polling_transmit(s_spi, &t.base));
        done += n;
    }
}

struct fb_rect_ctx { int x1, y1, w; };

static void fill_from_fb(uint16_t *dst, size_t start, size_t n, void *vctx)
{
    const fb_rect_ctx *c = (const fb_rect_ctx *)vctx;
    for (size_t i = 0; i < n; i++) {
        size_t idx = start + i;
        int row = (int)(idx / (size_t)c->w);
        int col = (int)(idx % (size_t)c->w);
        uint16_t px = s_fb[(size_t)(c->y1 + row) * s_width + c->x1 + col];
        dst[i] = (uint16_t)((px << 8) | (px >> 8));
    }
}

static void backlight_pwm_init(void)
{
    /* Released here in case display_deep_sleep_prepare() latched the
     * pin low for deep sleep (see display_ili9341.cpp). */
    gpio_hold_dis((gpio_num_t)ST_BL_PIN);

    ledc_timer_config_t t = {};
    t.speed_mode      = BL_LEDC_MODE;
    t.duty_resolution = LEDC_TIMER_8_BIT;
    t.timer_num       = BL_LEDC_TIMER;
    t.freq_hz         = BL_LEDC_FREQ_HZ;
    t.clk_cfg         = LEDC_USE_RC_FAST_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&t));

    ledc_channel_config_t c = {};
    c.gpio_num   = ST_BL_PIN;
    c.speed_mode = BL_LEDC_MODE;
    c.channel    = BL_LEDC_CHANNEL;
    c.timer_sel  = BL_LEDC_TIMER;
    c.duty       = 0;   /* off until the panel is initialised */
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

extern "C" void display_init(int /*pin_a*/, int /*pin_b*/, int /*pin_c*/,
                             int /*pin_d*/, int /*pin_e*/, int /*pin_f*/,
                             int width, int height)
{
    s_width  = width;
    s_height = height;
    s_fb_pixels = (size_t)width * height;

    backlight_pwm_init();

    spi_bus_config_t bus = {};
    bus.data0_io_num = ST_D0_PIN;
    bus.data1_io_num = ST_D1_PIN;
    bus.sclk_io_num  = ST_SCK_PIN;
    bus.data2_io_num = ST_D2_PIN;
    bus.data3_io_num = ST_D3_PIN;
    bus.data4_io_num = -1;
    bus.data5_io_num = -1;
    bus.data6_io_num = -1;
    bus.data7_io_num = -1;
    bus.max_transfer_sz = ST_CHUNK_PIXELS * 2 + 16;
    /* Do NOT pass SPICOMMON_BUSFLAG_QUAD (or _OCTAL/_IOMUX_PINS):
     * ESP-IDF's spicommon_bus_initialize_io() tests
     * `flags & SPICOMMON_BUSFLAG_OCTAL`, which is true for QUAD too
     * (OCTAL = QUAD | IO4_IO7). When the pins are the SPI2 IOMUX pins,
     * as here, it then calls bus_iomux_pins_set_oct() and moves the
     * clock and data pins to the octal IOMUX function, so nothing
     * reaches the panel: every read returns 0xFF and the screen stays
     * black. This is what broke both earlier attempts at this board.
     * Quad transfers work without the flag; it only adds pin checks. */
    bus.flags = SPICOMMON_BUSFLAG_MASTER;
    ESP_ERROR_CHECK(spi_bus_initialize(ST_SPI_HOST, &bus, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t dev = {};
    dev.mode           = 0;
    dev.clock_speed_hz = ST_SPI_CLOCK_HZ;
    dev.spics_io_num   = ST_CS_PIN;
    dev.flags          = SPI_DEVICE_HALFDUPLEX;
    dev.queue_size     = 4;
    ESP_ERROR_CHECK(spi_bus_add_device(ST_SPI_HOST, &dev, &s_spi));

    s_fb = (uint16_t *)heap_caps_malloc(s_fb_pixels * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    s_chunk = (uint16_t *)heap_caps_malloc(ST_CHUNK_PIXELS * sizeof(uint16_t),
                                           MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    assert(s_fb && s_chunk);
    memset(s_fb, 0, s_fb_pixels * sizeof(uint16_t));

    /* SWRESET first: the panel's RESET pin follows CHIP_PU, which a
     * software or USB reset of the ESP32-S3 does not toggle, so the
     * panel may still hold the previous firmware's state. */
    write_reg(0x01, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(120));
    send_init_table();

    log_readback("RDDPM", 0x0A, 1);

    display_clear(0x00);
    display_flush();
    display_set_backlight(s_bl_last_pct);

    ESP_LOGI(TAG, "ST77922 %dx%d initialized (QSPI %d MHz)", width, height,
             ST_SPI_CLOCK_HZ / 1000000);
}

extern "C" void display_clear(uint8_t color)
{
    memset(s_fb, color ? 0xFF : 0x00, s_fb_pixels * sizeof(uint16_t));
    s_dirty_x1 = 0;
    s_dirty_y1 = 0;
    s_dirty_x2 = s_width  - 1;
    s_dirty_y2 = s_height - 1;
}

static void mark_dirty(int x1, int y1, int x2, int y2)
{
    if (s_dirty_x1 < 0) {
        s_dirty_x1 = x1; s_dirty_y1 = y1;
        s_dirty_x2 = x2; s_dirty_y2 = y2;
    } else {
        if (x1 < s_dirty_x1) s_dirty_x1 = x1;
        if (y1 < s_dirty_y1) s_dirty_y1 = y1;
        if (x2 > s_dirty_x2) s_dirty_x2 = x2;
        if (y2 > s_dirty_y2) s_dirty_y2 = y2;
    }
}

extern "C" void display_set_pixel(uint16_t x, uint16_t y, uint8_t color)
{
    int px = (int)x + display_margin_left();
    int py = (int)y + display_margin_top();
    if (px < 0 || py < 0 || px >= s_width || py >= s_height) return;
    s_fb[(size_t)py * s_width + px] = color ? 0xFFFF : 0x0000;
    mark_dirty(px, py, px, py);
}

extern "C" bool display_push_rgb565(int x, int y, int w, int h, const void *color_map)
{
    if (w <= 0 || h <= 0) return true;
    x += display_margin_left();
    y += display_margin_top();
    int x2 = x + w - 1;
    int y2 = y + h - 1;
    if (x2 >= s_width)  x2 = s_width  - 1;
    if (y2 >= s_height) y2 = s_height - 1;
    if (x < 0 || y < 0 || x2 < x || y2 < y) return true;

    const uint16_t *src = (const uint16_t *)color_map;
    for (int row = 0; row < (y2 - y + 1); row++) {
        memcpy(s_fb + (size_t)(y + row) * s_width + x, src + (size_t)row * w,
               (size_t)(x2 - x + 1) * sizeof(uint16_t));
    }
    mark_dirty(x, y, x2, y2);
    return true;
}

extern "C" void display_set_partial_clip(int /*x*/, int /*y*/, int /*w*/, int /*h*/)
{
    /* No-op: always flush the whole dirty rectangle. */
}

extern "C" void display_flush(void)
{
    if (s_dirty_x1 < 0) return;
    int x1 = s_dirty_x1, y1 = s_dirty_y1, x2 = s_dirty_x2, y2 = s_dirty_y2;
    s_dirty_x1 = s_dirty_y1 = s_dirty_x2 = s_dirty_y2 = -1;
    if (s_panel_asleep) return;

    /* Widen to the 4-column grid the ST77922 requires. */
    x1 &= ~3;
    x2 |= 3;
    if (x2 >= s_width) x2 = s_width - 1;

    fb_rect_ctx ctx = { x1, y1, x2 - x1 + 1 };
    write_pixels(x1, y1, x2, y2, fill_from_fb, &ctx);
}

extern "C" void display_full_refresh(void)
{
    mark_dirty(0, 0, s_width - 1, s_height - 1);
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
    int saved_pct = s_bl_last_pct;
    display_set_backlight(0);
    s_bl_last_pct = saved_pct;
    write_reg(0x28, NULL, 0);   /* DISPOFF */
    write_reg(0x10, NULL, 0);   /* SLPIN */
}

extern "C" void display_wake(void)
{
    if (!s_panel_asleep) return;
    write_reg(0x11, NULL, 0);   /* SLPOUT */
    vTaskDelay(pdMS_TO_TICKS(120));
    write_reg(0x29, NULL, 0);   /* DISPON */
    s_panel_asleep = false;
    display_set_backlight(s_bl_last_pct);
    display_full_refresh();
}

extern "C" void display_deep_sleep_prepare(void)
{
    display_set_backlight(0);
    write_reg(0x28, NULL, 0);   /* DISPOFF */
    write_reg(0x10, NULL, 0);   /* SLPIN; the init table's SLPOUT undoes it */
    /* LEDC stops in deep sleep: hold the backlight switch off. */
    gpio_set_direction((gpio_num_t)ST_BL_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)ST_BL_PIN, 0);
    gpio_hold_en((gpio_num_t)ST_BL_PIN);
    gpio_deep_sleep_hold_en();
}

extern "C" void display_set_shared_i2c_bus(void * /*bus_handle*/)
{
    /* This backend does not use I2C. */
}

#endif /* CONFIG_DRAFTLING_DISPLAY_ST77922 */
