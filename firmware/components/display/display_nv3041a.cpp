#include "sdkconfig.h"
#if defined(CONFIG_DRAFTLING_DISPLAY_NV3041A)

/*
 * NV3041A 480x272 color LCD on a 4-lane QSPI bus (Guition JC4827W543C,
 * 4.3" IPS panel).
 *
 * Protocol and init sequence come from Arduino_GFX
 * (Arduino_ESP32QSPI + Arduino_NV3041A), the library the vendor demos
 * use on this board (github.com/profi-max/JC4827W543_4.3inch_ESP32S3_board);
 * ESPHome's qspi_dbi config for the board sends the same table:
 *
 *   - register write: opcode 0x02 (1 lane), 24-bit address
 *     0x00 <cmd> 0x00 (1 lane), parameters on 1 lane;
 *   - pixel write: CASET / RASET / RAMWR as register writes, then the
 *     pixel data in chunks with opcode 0x32 and address 0x003C00
 *     (RAMWRC) on 1 lane, data on 4 lanes, big-endian RGB565;
 *   - SWRESET, then the vendor init table, then -- as
 *     Arduino_NV3041A::begin() does after the table -- MADCTL 0xC0
 *     (MX | MY) and INVON (an IPS panel, ips = true) once more,
 *     outside the vendor register bank (after 0xFF 0x00). The table's
 *     own MADCTL write sits inside the bank (0xFF 0xA5) and did not
 *     set the orientation on hardware; without the trailing writes the
 *     picture was upside down and the colors inverted, and since the
 *     panel has no reset GPIO and stays powered across ESP32 resets,
 *     leftover state from earlier firmware hid this on warm boots.
 *     A test pattern cycling MADCTL 0x00 / 0xC0 x INVOFF / INVON
 *     confirmed 0xC0 + INVON after both a cold power-on and a warm
 *     reset.
 *
 * The panel has no reset GPIO. The panel is 480x272 landscape in its
 * native frame; with this MADCTL the GT911 reports in the same frame
 * without mirroring (checked on hardware).
 *
 * The panel's TE output is wired to GPIO0, which is also the BOOT
 * button and the deep-sleep wake source. TE is never enabled (no
 * TEON), so the panel should leave the line alone; display_init()
 * logs the GPIO0 level as a check.
 *
 * Pins come from the board header through display_nv3041a_config().
 * The QSPI pins are not the SPI2 IOMUX pins, so the ESP-IDF
 * SPICOMMON_BUSFLAG_QUAD bug (see display_st77922.cpp) does not
 * apply, but the flag is left out all the same.
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

static const char *TAG = "DisplayNV3041A";

#define NV_SPI_HOST     SPI2_HOST

#define NV_OP_WRITE_REG 0x02
#define NV_OP_WRITE_PIX 0x32

/* Pixels per QSPI pixel transaction. The chunk buffer is internal
 * DMA-capable RAM, so the DMA never reads PSRAM. */
#define NV_CHUNK_PIXELS 8192

#define NV_MADCTL       0xC0   /* MX | MY */

#define BL_LEDC_TIMER    LEDC_TIMER_3
#define BL_LEDC_MODE     LEDC_LOW_SPEED_MODE
#define BL_LEDC_CHANNEL  LEDC_CHANNEL_1
#define BL_LEDC_DUTY_MAX ((1 << 8) - 1)

struct nv_init_cmd_t {
    uint8_t  cmd;
    uint8_t  len;
    uint16_t delay_ms;
    uint8_t  data[1];
};

/* Vendor init sequence, byte for byte (Arduino_GFX
 * nv3041a_init_operations[]), followed by the MADCTL and INVON writes
 * of Arduino_NV3041A::begin() (see the file header). Do not reorder or
 * "clean up". COLMOD 0x01 = RGB565; SLPOUT and DISPON are sent with a
 * 0x00 parameter, as in the original. */
static const nv_init_cmd_t nv3041a_init_cmds[] = {
    {0xFF, 1, 0, {0xA5}},
    {0x36, 1, 0, {0xC0}},
    {0x3A, 1, 0, {0x01}},
    {0x41, 1, 0, {0x03}},
    {0x44, 1, 0, {0x15}},
    {0x45, 1, 0, {0x15}},
    {0x7D, 1, 0, {0x03}},
    {0xC1, 1, 0, {0xBB}},
    {0xC2, 1, 0, {0x05}},
    {0xC3, 1, 0, {0x10}},
    {0xC6, 1, 0, {0x3E}},
    {0xC7, 1, 0, {0x25}},
    {0xC8, 1, 0, {0x11}},
    {0x7A, 1, 0, {0x5F}},
    {0x6F, 1, 0, {0x44}},
    {0x78, 1, 0, {0x70}},
    {0xC9, 1, 0, {0x00}},
    {0x67, 1, 0, {0x21}},
    {0x51, 1, 0, {0x0A}},
    {0x52, 1, 0, {0x76}},
    {0x53, 1, 0, {0x0A}},
    {0x54, 1, 0, {0x76}},
    {0x46, 1, 0, {0x0A}},
    {0x47, 1, 0, {0x2A}},
    {0x48, 1, 0, {0x0A}},
    {0x49, 1, 0, {0x1A}},
    {0x56, 1, 0, {0x43}},
    {0x57, 1, 0, {0x42}},
    {0x58, 1, 0, {0x3C}},
    {0x59, 1, 0, {0x64}},
    {0x5A, 1, 0, {0x41}},
    {0x5B, 1, 0, {0x3C}},
    {0x5C, 1, 0, {0x02}},
    {0x5D, 1, 0, {0x3C}},
    {0x5E, 1, 0, {0x1F}},
    {0x60, 1, 0, {0x80}},
    {0x61, 1, 0, {0x3F}},
    {0x62, 1, 0, {0x21}},
    {0x63, 1, 0, {0x07}},
    {0x64, 1, 0, {0xE0}},
    {0x65, 1, 0, {0x02}},
    {0xCA, 1, 0, {0x20}},
    {0xCB, 1, 0, {0x52}},
    {0xCC, 1, 0, {0x10}},
    {0xCD, 1, 0, {0x42}},
    {0xD0, 1, 0, {0x20}},
    {0xD1, 1, 0, {0x52}},
    {0xD2, 1, 0, {0x10}},
    {0xD3, 1, 0, {0x42}},
    {0xD4, 1, 0, {0x0A}},
    {0xD5, 1, 0, {0x32}},
    /* gamma */
    {0x80, 1, 0, {0x00}},
    {0xA0, 1, 0, {0x00}},
    {0x81, 1, 0, {0x07}},
    {0xA1, 1, 0, {0x06}},
    {0x82, 1, 0, {0x02}},
    {0xA2, 1, 0, {0x01}},
    {0x86, 1, 0, {0x11}},
    {0xA6, 1, 0, {0x10}},
    {0x87, 1, 0, {0x27}},
    {0xA7, 1, 0, {0x27}},
    {0x83, 1, 0, {0x37}},
    {0xA3, 1, 0, {0x37}},
    {0x84, 1, 0, {0x35}},
    {0xA4, 1, 0, {0x35}},
    {0x85, 1, 0, {0x3F}},
    {0xA5, 1, 0, {0x3F}},
    {0x88, 1, 0, {0x0B}},
    {0xA8, 1, 0, {0x0B}},
    {0x89, 1, 0, {0x14}},
    {0xA9, 1, 0, {0x14}},
    {0x8A, 1, 0, {0x1A}},
    {0xAA, 1, 0, {0x1A}},
    {0x8B, 1, 0, {0x0A}},
    {0xAB, 1, 0, {0x0A}},
    {0x8C, 1, 0, {0x14}},
    {0xAC, 1, 0, {0x08}},
    {0x8D, 1, 0, {0x17}},
    {0xAD, 1, 0, {0x07}},
    {0x8E, 1, 0, {0x16}},
    {0xAE, 1, 0, {0x06}},
    {0x8F, 1, 0, {0x1B}},
    {0xAF, 1, 0, {0x07}},
    {0x90, 1, 0, {0x04}},
    {0xB0, 1, 0, {0x04}},
    {0x91, 1, 0, {0x0A}},
    {0xB1, 1, 0, {0x0A}},
    {0x92, 1, 0, {0x16}},
    {0xB2, 1, 0, {0x15}},
    {0xFF, 1, 0, {0x00}},
    {0x11, 1, 120, {0x00}},   /* SLPOUT */
    {0x29, 1, 100, {0x00}},   /* DISPON */
    {0x21, 0, 0, {0}},        /* INVON (IPS panel) */
    {0x36, 1, 0, {NV_MADCTL}},/* MADCTL, outside the vendor bank */
};

static display_nv3041a_config_t s_cfg = {
    -1, -1, -1, -1, -1, -1, -1, 32 * 1000 * 1000, 5000,
};

static spi_device_handle_t s_spi = NULL;

/* Native landscape framebuffer (s_width x s_height), PSRAM. */
static uint16_t *s_fb = NULL;
static size_t    s_fb_pixels = 0;
static int s_width  = 0;
static int s_height = 0;

/* Byte-swapped pixel chunk, internal DMA memory. */
static uint16_t *s_chunk = NULL;

static int s_dirty_x1 = -1, s_dirty_y1 = -1, s_dirty_x2 = -1, s_dirty_y2 = -1;
static int s_bl_last_pct = 100;
static bool s_panel_asleep = false;

extern "C" void display_nv3041a_config(const display_nv3041a_config_t *cfg)
{
    s_cfg = *cfg;
}

static void write_reg(uint8_t cmd, const uint8_t *data, size_t len)
{
    spi_transaction_ext_t t = {};
    t.base.flags   = SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR;
    t.base.cmd     = NV_OP_WRITE_REG;
    t.base.addr    = (uint32_t)cmd << 8;
    t.command_bits = 8;
    t.address_bits = 24;
    t.base.length    = len * 8;
    t.base.tx_buffer = len ? data : NULL;
    ESP_ERROR_CHECK(spi_device_polling_transmit(s_spi, &t.base));
}

static void send_init_table(void)
{
    for (size_t i = 0; i < sizeof(nv3041a_init_cmds) / sizeof(nv3041a_init_cmds[0]); i++) {
        const nv_init_cmd_t *c = &nv3041a_init_cmds[i];
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
    write_reg(0x2C, NULL, 0); /* RAMWR; the pixel chunks continue it */
}

static void write_fb_rect(int x1, int y1, int x2, int y2)
{
    set_window(x1, y1, x2, y2);
    const int w = x2 - x1 + 1;
    const size_t total = (size_t)w * (size_t)(y2 - y1 + 1);
    size_t done = 0;
    while (done < total) {
        size_t n = total - done;
        if (n > NV_CHUNK_PIXELS) n = NV_CHUNK_PIXELS;
        for (size_t i = 0; i < n; i++) {
            size_t idx = done + i;
            int row = (int)(idx / (size_t)w);
            int col = (int)(idx % (size_t)w);
            uint16_t px = s_fb[(size_t)(y1 + row) * s_width + x1 + col];
            s_chunk[i] = (uint16_t)((px << 8) | (px >> 8));
        }
        spi_transaction_ext_t t = {};
        t.base.flags   = SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR |
                         SPI_TRANS_MODE_QIO;
        t.base.cmd     = NV_OP_WRITE_PIX;
        t.base.addr    = 0x3Cu << 8;   /* RAMWRC */
        t.command_bits = 8;
        t.address_bits = 24;
        t.base.length    = n * 16;
        t.base.tx_buffer = s_chunk;
        ESP_ERROR_CHECK(spi_device_polling_transmit(s_spi, &t.base));
        done += n;
    }
}

static void backlight_pwm_init(void)
{
    if (s_cfg.bl < 0) return;
    /* Released here in case display_deep_sleep_prepare() latched the
     * pin low for deep sleep. */
    gpio_hold_dis((gpio_num_t)s_cfg.bl);

    ledc_timer_config_t t = {};
    t.speed_mode      = BL_LEDC_MODE;
    t.duty_resolution = LEDC_TIMER_8_BIT;
    t.timer_num       = BL_LEDC_TIMER;
    t.freq_hz         = (uint32_t)s_cfg.bl_pwm_hz;
    t.clk_cfg         = LEDC_AUTO_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&t));

    ledc_channel_config_t c = {};
    c.gpio_num   = s_cfg.bl;
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
    if (s_cfg.bl < 0) return;
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
    bus.data0_io_num = s_cfg.d0;
    bus.data1_io_num = s_cfg.d1;
    bus.sclk_io_num  = s_cfg.sck;
    bus.data2_io_num = s_cfg.d2;
    bus.data3_io_num = s_cfg.d3;
    bus.data4_io_num = -1;
    bus.data5_io_num = -1;
    bus.data6_io_num = -1;
    bus.data7_io_num = -1;
    bus.max_transfer_sz = NV_CHUNK_PIXELS * 2 + 16;
    /* No SPICOMMON_BUSFLAG_QUAD: see display_st77922.cpp. */
    bus.flags = SPICOMMON_BUSFLAG_MASTER;
    ESP_ERROR_CHECK(spi_bus_initialize(NV_SPI_HOST, &bus, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t dev = {};
    dev.mode           = 0;
    dev.clock_speed_hz = s_cfg.spi_clock_hz;
    dev.spics_io_num   = s_cfg.cs;
    dev.flags          = SPI_DEVICE_HALFDUPLEX;
    dev.queue_size     = 4;
    ESP_ERROR_CHECK(spi_bus_add_device(NV_SPI_HOST, &dev, &s_spi));

    s_fb = (uint16_t *)heap_caps_malloc(s_fb_pixels * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    s_chunk = (uint16_t *)heap_caps_malloc(NV_CHUNK_PIXELS * sizeof(uint16_t),
                                           MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    assert(s_fb && s_chunk);
    memset(s_fb, 0, s_fb_pixels * sizeof(uint16_t));

    /* No reset GPIO: SWRESET, also clearing whatever a previous
     * firmware (or the SLPIN before deep sleep) left behind. */
    write_reg(0x01, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(120));
    send_init_table();

    display_clear(0x00);
    display_flush();
    display_set_backlight(s_bl_last_pct);

    /* GPIO0 is the BOOT button and also the panel's TE output. It
     * must read 1 (released) here, or the BOOT button cannot wake the
     * board from deep sleep. */
    ESP_LOGI(TAG, "NV3041A %dx%d initialized (QSPI %d MHz), GPIO0 (BOOT/TE) = %d",
             width, height, s_cfg.spi_clock_hz / 1000000,
             gpio_get_level(GPIO_NUM_0));
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
    write_fb_rect(x1, y1, x2, y2);
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
    vTaskDelay(pdMS_TO_TICKS(120));
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
    if (!s_panel_asleep) {
        write_reg(0x28, NULL, 0);   /* DISPOFF */
        write_reg(0x10, NULL, 0);   /* SLPIN; SWRESET + init undo it */
        vTaskDelay(pdMS_TO_TICKS(120));
        s_panel_asleep = true;
    }
    if (s_cfg.bl >= 0) {
        /* LEDC stops in deep sleep: hold the backlight switch off. */
        ledc_stop(BL_LEDC_MODE, BL_LEDC_CHANNEL, 0);
        gpio_set_direction((gpio_num_t)s_cfg.bl, GPIO_MODE_OUTPUT);
        gpio_set_level((gpio_num_t)s_cfg.bl, 0);
        gpio_hold_en((gpio_num_t)s_cfg.bl);
    }
    /* The QSPI lines are left alone: CS is GPIO45, a strapping pin
     * (VDD_SPI voltage), which must not be held at a level across the
     * wake reset. The panel ignores the bus in SLPIN. */
    gpio_deep_sleep_hold_en();
}

extern "C" void display_set_shared_i2c_bus(void * /*bus_handle*/)
{
    /* This backend does not use I2C. */
}

const char *display_controller_name(void)
{
    return "NV3041A (QSPI)";
}

#endif /* CONFIG_DRAFTLING_DISPLAY_NV3041A */
