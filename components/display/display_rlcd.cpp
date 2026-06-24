#include "sdkconfig.h"
#if defined(CONFIG_DRAFTLING_DISPLAY_RLCD)

#include <cstdio>
#include <cstring>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_heap_caps.h>
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_lcd_panel_io.h>

#include "display.h"

static const char *TAG = "Display";

static esp_lcd_panel_io_handle_t s_io_handle = NULL;
static uint8_t *s_disp_buf = NULL;
static int s_disp_len = 0;
static int s_width = 0;
static int s_height = 0;
static int s_rst_pin = -1;
/* RLCD has no BUSY signal; the parameter is accepted for API
 * compatibility with the e-paper drivers and ignored. */

/* Landscape LUT: for each (x,y) store the byte index and bit mask */
static uint16_t *s_pixel_index_lut = NULL;  /* width * height entries */
static uint8_t  *s_pixel_bit_lut   = NULL;  /* width * height entries */

static void init_landscape_lut(int width, int height)
{
    int h4 = height / 4;
    for (int y = 0; y < height; y++) {
        int inv_y = height - 1 - y;
        int block_y = inv_y / 4;
        int local_y = inv_y % 4;
        for (int x = 0; x < width; x++) {
            int byte_x = x / 2;
            int local_x = x % 2;
            uint32_t index = byte_x * h4 + block_y;
            uint8_t bit = 7 - (local_y * 2 + local_x);
            int lut_idx = x * height + y;
            s_pixel_index_lut[lut_idx] = (uint16_t)index;
            s_pixel_bit_lut[lut_idx]   = (uint8_t)(1 << bit);
        }
    }
}

static void send_command(uint8_t cmd)
{
    esp_lcd_panel_io_tx_param(s_io_handle, cmd, NULL, 0);
}

static void send_data(uint8_t data)
{
    esp_lcd_panel_io_tx_param(s_io_handle, -1, &data, 1);
}

static void send_buffer(const uint8_t *data, int len)
{
    esp_lcd_panel_io_tx_color(s_io_handle, -1, data, len);
}

static void hw_reset(void)
{
    gpio_set_level((gpio_num_t)s_rst_pin, 1);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level((gpio_num_t)s_rst_pin, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level((gpio_num_t)s_rst_pin, 1);
    vTaskDelay(pdMS_TO_TICKS(50));
}

extern "C" void display_init(int mosi, int sck, int dc, int cs, int rst,
                             int busy, int width, int height)
{
    (void)busy;  /* RLCD has no BUSY signal */
    s_width = width;
    s_height = height;
    s_rst_pin = rst;

    /* Init SPI bus */
    spi_bus_config_t bus_cfg = {};
    bus_cfg.miso_io_num   = -1;
    bus_cfg.mosi_io_num   = mosi;
    bus_cfg.sclk_io_num   = sck;
    bus_cfg.quadwp_io_num = -1;
    bus_cfg.quadhd_io_num = -1;
    bus_cfg.max_transfer_sz = width * height;
    ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    /* Create LCD panel IO over SPI */
    esp_lcd_panel_io_spi_config_t io_cfg = {};
    io_cfg.dc_gpio_num      = (gpio_num_t)dc;
    io_cfg.cs_gpio_num      = (gpio_num_t)cs;
    io_cfg.pclk_hz          = 10 * 1000 * 1000;
    io_cfg.lcd_cmd_bits     = 8;
    io_cfg.lcd_param_bits   = 8;
    io_cfg.spi_mode         = 0;
    io_cfg.trans_queue_depth = 10;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI3_HOST, &io_cfg, &s_io_handle));

    /* Configure RST GPIO */
    gpio_config_t gpio_cfg = {};
    gpio_cfg.intr_type    = GPIO_INTR_DISABLE;
    gpio_cfg.mode         = GPIO_MODE_OUTPUT;
    gpio_cfg.pin_bit_mask = (1ULL << rst);
    gpio_cfg.pull_up_en   = GPIO_PULLUP_ENABLE;
    ESP_ERROR_CHECK(gpio_config(&gpio_cfg));

    /* Allocate framebuffer in PSRAM */
    s_disp_len = (width * height) / 8;
    s_disp_buf = (uint8_t *)heap_caps_malloc(s_disp_len, MALLOC_CAP_SPIRAM);
    assert(s_disp_buf);

    /* Allocate and initialise LUTs in PSRAM */
    int total_pixels = width * height;
    s_pixel_index_lut = (uint16_t *)heap_caps_malloc(total_pixels * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    s_pixel_bit_lut   = (uint8_t  *)heap_caps_malloc(total_pixels * sizeof(uint8_t),  MALLOC_CAP_SPIRAM);
    assert(s_pixel_index_lut && s_pixel_bit_lut);
    init_landscape_lut(width, height);

    /* Hardware reset & RLCD init sequence */
    hw_reset();

    send_command(0xD6); send_data(0x17); send_data(0x02);
    send_command(0xD1); send_data(0x01);
    send_command(0xC0); send_data(0x11); send_data(0x04);
    send_command(0xC1); send_data(0x69); send_data(0x69); send_data(0x69); send_data(0x69);
    send_command(0xC2); send_data(0x19); send_data(0x19); send_data(0x19); send_data(0x19);
    send_command(0xC4); send_data(0x4B); send_data(0x4B); send_data(0x4B); send_data(0x4B);
    send_command(0xC5); send_data(0x19); send_data(0x19); send_data(0x19); send_data(0x19);
    send_command(0xD8); send_data(0x80); send_data(0xE9);
    send_command(0xB2); send_data(0x02);
    send_command(0xB3); send_data(0xE5); send_data(0xF6); send_data(0x05); send_data(0x46);
                        send_data(0x77); send_data(0x77); send_data(0x77); send_data(0x77);
                        send_data(0x76); send_data(0x45);
    send_command(0xB4); send_data(0x05); send_data(0x46); send_data(0x77); send_data(0x77);
                        send_data(0x77); send_data(0x77); send_data(0x76); send_data(0x45);
    send_command(0x62); send_data(0x32); send_data(0x03); send_data(0x1F);
    send_command(0xB7); send_data(0x13);
    send_command(0xB0); send_data(0x64);
    send_command(0x11);
    vTaskDelay(pdMS_TO_TICKS(200));
    send_command(0xC9); send_data(0x00);
    send_command(0x36); send_data(0x48);
    send_command(0x3A); send_data(0x11);
    send_command(0xB9); send_data(0x20);
    send_command(0xB8); send_data(0x29);
    send_command(0x21);
    send_command(0x2A); send_data(0x12); send_data(0x2A);
    send_command(0x2B); send_data(0x00); send_data(0xC7);
    send_command(0x35); send_data(0x00);
    send_command(0xD0); send_data(0xFF);
    send_command(0x38);
    send_command(0x29);

    display_clear(0xFF);
    display_flush();

    ESP_LOGI(TAG, "RLCD %dx%d initialized", width, height);
}

extern "C" void display_clear(uint8_t color)
{
    memset(s_disp_buf, color, s_disp_len);
}

extern "C" void display_set_pixel(uint16_t x, uint16_t y, uint8_t color)
{
    int lut_idx = x * s_height + y;
    uint16_t idx  = s_pixel_index_lut[lut_idx];
    uint8_t  mask = s_pixel_bit_lut[lut_idx];
    if (color)
        s_disp_buf[idx] |= mask;
    else
        s_disp_buf[idx] &= ~mask;
}

extern "C" void display_flush(void)
{
    send_command(0x2A); send_data(0x12); send_data(0x2A);
    send_command(0x2B); send_data(0x00); send_data(0xC7);
    send_command(0x2C);
    send_buffer(s_disp_buf, s_disp_len);
}

extern "C" void display_full_refresh(void)
{
    /* The RLCD has no ghosting and no separate partial mode, so a full
     * refresh is just a normal flush. */
    display_flush();
}

extern "C" void display_request_full_refresh(void)
{
    /* No partial-refresh state machine on this backend; nothing to
     * latch. The next display_flush() already pushes the full
     * framebuffer. */
}

extern "C" uint8_t *display_get_buffer(void)
{
    return s_disp_buf;
}

extern "C" int display_get_buffer_size(void)
{
    return s_disp_len;
}

extern "C" bool display_push_rgb565(int /*x*/, int /*y*/, int /*w*/, int /*h*/,
                                    const void * /*color_map*/)
{
    /* No fast path on RLCD - lvgl_port.cpp falls back to the
     * per-pixel display_set_pixel conversion. */
    return false;
}

extern "C" void display_set_partial_clip(int /*x*/, int /*y*/,
                                         int /*w*/, int /*h*/)
{
    /* RLCD always pushes the entire framebuffer; no partial-refresh
     * region to clip. No-op. */
}

extern "C" void display_set_backlight(int /*percent*/)
{
    /* The reflective LCD has no backlight. No-op. */
}

extern "C" void display_sleep(void)
{
    /* The reflective LCD has no backlight to switch off, and the
     * panel retains its image without power. The standby manager
     * uses deep sleep on this board anyway, so this is just a
     * defensive no-op. */
}

extern "C" void display_wake(void)
{
    /* No-op (see display_sleep). */
}

extern "C" void display_deep_sleep_prepare(void)
{
    /* The reflective LCD has no backlight, retains its image
     * without power, and the panel goes dark naturally once VDD
     * drops in deep sleep. No-op. */
}

extern "C" void display_set_shared_i2c_bus(void * /*bus_handle*/)
{
    /* RLCD does not use I2C. No-op (declared in display.h for the
     * shared-bus path used by the EPDIY backend). */
}

#endif /* CONFIG_DRAFTLING_DISPLAY_RLCD */
