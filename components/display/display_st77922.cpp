#include "sdkconfig.h"
#if defined(CONFIG_DRAFTLING_DISPLAY_ST77922)

/*
 * ST77922 QSPI color-LCD backend for the Freenove FNK0104N (3.5"
 * 320x480 native panel, rendered landscape at 480x320).
 *
 * The protocol, pin assignments and vendor init table below are
 * ported verbatim from Freenove's own reference driver
 * (Libraries/FNK0104N/TFT_eSPI_v2.5.43.zip -> TFT_eSPI/ST77922.cpp,
 * bundled with https://github.com/Freenove/Freenove_ESP32_S3_Display),
 * which is structurally identical to the AXS15231B protocol already
 * used elsewhere in this component (single-line register writes
 * wrapped in a variable-cmd/addr QSPI preamble, 4-line pixel-write
 * bursts with CS held low across chunks) -- see display_axs15231b.cpp
 * for the general pattern this file follows.
 *
 * Unlike AXS15231B, the ST77922 does not offer a MADCTL row/column
 * swap for this panel's landscape orientation: Freenove's reference
 * driver keeps MADCTL at its portrait value and instead performs a
 * software pixel transpose in Fill_Colors() for rotation 1/3. This
 * file ports that same transpose (rotation "1": logical (lx, ly) ->
 * physical (LCD_NATIVE_WIDTH - 1 - ly, lx)) so CASET/RASET-equivalent
 * addressing always operates in the panel's native portrait frame,
 * while display_clear/display_set_pixel/display_push_rgb565 continue
 * to operate in logical (already-landscape) coordinates like every
 * other backend.
 *
 * All FNK0104N pins are hard-coded here (this is the only board using
 * this backend), matching the existing convention of
 * display_rgb.cpp hard-coding its per-board Sunton pins internally.
 */

#include <cstdio>
#include <cstring>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_heap_caps.h>
#include <driver/gpio.h>
#include <driver/spi_master.h>

#include "display.h"

static const char *TAG = "DisplayST77922";

/* Pins, from Freenove's ST77922.h reference header. */
#define FNK_N_CS_PIN     10
#define FNK_N_BL_PIN     41
#define FNK_N_SCLK_PIN   12
#define FNK_N_D0_PIN     11
#define FNK_N_D1_PIN     13
#define FNK_N_D2_PIN     14
#define FNK_N_D3_PIN     9

#define FNK_N_SPI_HOST       SPI2_HOST
#define FNK_N_SPI_CLOCK_HZ   (80 * 1000 * 1000)

/* Native panel resolution (portrait). The logical (landscape) frame
 * used by display_clear/set_pixel/push_rgb565 is
 * FNK_N_NATIVE_HEIGHT x FNK_N_NATIVE_WIDTH (480 x 320). */
#define FNK_N_NATIVE_WIDTH   320
#define FNK_N_NATIVE_HEIGHT  480

#define QSPI_1W_CMD   0x02
#define QSPI_4W_CMD   0x32
#define WR_RAM_C_CMD  0x3C
#define SET_X_CMD     0x2A
#define SET_Y_CMD     0x2B
#define MADCTL_CMD    0x36

#define TX_LEN  0x4000

struct lcd_init_cmd {
    uint8_t cmd;
    const uint8_t *data;
    uint8_t len;
    uint32_t delay_ms;
};

/* Vendor init table, ported verbatim from Freenove's ST77922.cpp
 * (st77922_lcd_init[]). Do not reorder or "clean up" -- this is a
 * black-box vendor timing/gamma/power recipe for this exact panel. */
static const uint8_t d_f1[]  = {0x00};
static const uint8_t d_60[]  = {0x00, 0x00, 0x00};
static const uint8_t d_65[]  = {0x80};
static const uint8_t d_79[]  = {0x06};
static const uint8_t d_7b1[] = {0x00, 0x08, 0x08};
static const uint8_t d_80[]  = {0x55, 0x62, 0x2F, 0x17, 0xF0, 0x52, 0x70, 0xD2, 0x52, 0x62, 0xEA};
static const uint8_t d_81[]  = {0x26, 0x52, 0x72, 0x27};
static const uint8_t d_84[]  = {0x92, 0x25};
static const uint8_t d_87[]  = {0x10, 0x10, 0x58, 0x00, 0x02, 0x3A};
static const uint8_t d_88[]  = {0x00, 0x00, 0x2C, 0x10, 0x04, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x06};
static const uint8_t d_89[]  = {0x00, 0x00, 0x00};
static const uint8_t d_8a[]  = {0x13, 0x00, 0x2C, 0x00, 0x00, 0x2C, 0x10, 0x10, 0x00, 0x3E, 0x19};
static const uint8_t d_8b[]  = {0x15, 0xB1, 0xB1, 0x44, 0x96, 0x2C, 0x10, 0x97, 0x8E};
static const uint8_t d_8c[]  = {0x1D, 0xB1, 0xB1, 0x44, 0x96, 0x2C, 0x10, 0x50, 0x0F, 0x01, 0xC5, 0x12, 0x09};
static const uint8_t d_8d[]  = {0x0C};
static const uint8_t d_8e[]  = {0x33, 0x01, 0x0C, 0x13, 0x01, 0x01};
static const uint8_t d_b3[]  = {0x00, 0x30};
static const uint8_t d_71[]  = {0xD0};
static const uint8_t d_66[]  = {0x02, 0x3F};
static const uint8_t d_be[]  = {0x26, 0x00, 0x9D};
static const uint8_t d_70[]  = {0x01, 0xA0, 0x11, 0x40, 0xE0, 0x00, 0x11, 0x69, 0x11, 0x00, 0x00, 0x1A};
static const uint8_t d_90[]  = {0x04, 0x04, 0x55, 0x74, 0x00, 0x40, 0x43, 0x27, 0x27};
static const uint8_t d_91[]  = {0x04, 0x04, 0x55, 0x75, 0x00, 0x40, 0x42, 0x27, 0x27};
static const uint8_t d_92[]  = {0x04, 0x44, 0x55, 0xC0, 0x06, 0x00, 0x07, 0x05, 0x90, 0x27};
static const uint8_t d_93[]  = {0x04, 0x43, 0x11, 0x00, 0x00, 0x00, 0x00, 0x05, 0x90, 0x27};
static const uint8_t d_94[]  = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t d_95[]  = {0x96, 0x16, 0x00, 0x00, 0xFF};
static const uint8_t d_96[]  = {0x44, 0x53, 0x03, 0x12, 0x23, 0x24, 0x06, 0x05, 0x94, 0x27, 0x00, 0x44};
static const uint8_t d_97[]  = {0x44, 0x53, 0x47, 0x56, 0x20, 0x20, 0x02, 0x01, 0x94, 0x27, 0x00, 0x44};
static const uint8_t d_ba[]  = {0x55, 0x94, 0x2D, 0x94, 0x27};
static const uint8_t d_9a[]  = {0x40, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00};
static const uint8_t d_9b[]  = {0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00};
static const uint8_t d_9c[]  = {0x5C, 0x12, 0x00, 0x00, 0x10, 0x12, 0x00, 0x00, 0x10, 0x02, 0x00, 0x00, 0x00};
static const uint8_t d_9d[]  = {0x8A, 0x51, 0x00, 0x00, 0x00, 0x80, 0x1E, 0x01};
static const uint8_t d_9e[]  = {0x51, 0x00, 0x00, 0x00, 0x80, 0x1E, 0x01};
static const uint8_t d_b4[]  = {0x1D, 0x1C, 0x1E, 0x0B, 0x14, 0x02, 0x13, 0x09, 0x1E, 0x00, 0x1E, 0x10};
static const uint8_t d_b5[]  = {0x1D, 0x1C, 0x1E, 0x0A, 0x15, 0x03, 0x11, 0x08, 0x1E, 0x01, 0x1E, 0x12};
static const uint8_t d_b6[]  = {0x77, 0x77, 0x00, 0x0A, 0xFF, 0x0A, 0xFF};
static const uint8_t d_86[]  = {0xCD, 0x04, 0xB1, 0x02, 0x58, 0x12, 0x58, 0x0C, 0x13, 0x01, 0xA5, 0x00, 0xA5, 0xA5};
static const uint8_t d_b7[]  = {0x07, 0x0A, 0x0E, 0x06, 0x05, 0x03, 0x2B, 0x03, 0x03, 0x42, 0x07, 0x10, 0x10, 0x2E, 0x3F, 0x0D};
static const uint8_t d_b8[]  = {0x07, 0x0A, 0x0D, 0x05, 0x05, 0x02, 0x2B, 0x02, 0x03, 0x42, 0x06, 0x10, 0x0F, 0x2E, 0x3F, 0x0D};
static const uint8_t d_b9[]  = {0x23, 0x23};
static const uint8_t d_bf1[] = {0x10, 0x14, 0x14, 0x0B, 0x0B, 0x0B};
static const uint8_t d_f2[]  = {0x00};
static const uint8_t d_73[]  = {0x04, 0xDA, 0x12, 0x54, 0x47};
static const uint8_t d_77[]  = {0x6B, 0x5B, 0xFD, 0xC3, 0xC5};
static const uint8_t d_7a[]  = {0x15, 0x27};
static const uint8_t d_7b2[] = {0x04, 0x57};
static const uint8_t d_7e[]  = {0x01, 0x0E};
static const uint8_t d_bf2[] = {0x36};
static const uint8_t d_e3[]  = {0x40, 0x40};
static const uint8_t d_f0[]  = {0x00};
static const uint8_t d_d0[]  = {0x00};
static const uint8_t d_2a[]  = {0x00, 0x00, 0x01, 0x3F};
static const uint8_t d_2b[]  = {0x00, 0x00, 0x01, 0xDF};
static const uint8_t d_3a[]  = {0x01};
static const uint8_t d_36[]  = {0x00};
static const uint8_t d_35[]  = {0x01};

static const lcd_init_cmd s_init_seq[] = {
    {0xF1, d_f1,  sizeof(d_f1),  0},
    {0x60, d_60,  sizeof(d_60),  0},
    {0x65, d_65,  sizeof(d_65),  0},
    {0x79, d_79,  sizeof(d_79),  0},
    {0x7B, d_7b1, sizeof(d_7b1), 0},
    {0x80, d_80,  sizeof(d_80),  0},
    {0x81, d_81,  sizeof(d_81),  0},
    {0x84, d_84,  sizeof(d_84),  0},
    {0x87, d_87,  sizeof(d_87),  0},
    {0x88, d_88,  sizeof(d_88),  0},
    {0x89, d_89,  sizeof(d_89),  0},
    {0x8A, d_8a,  sizeof(d_8a),  0},
    {0x8B, d_8b,  sizeof(d_8b),  0},
    {0x8C, d_8c,  sizeof(d_8c),  0},
    {0x8D, d_8d,  sizeof(d_8d),  0},
    {0x8E, d_8e,  sizeof(d_8e),  0},
    {0xB3, d_b3,  sizeof(d_b3),  0},
    {0xF1, d_f1,  sizeof(d_f1),  0},
    {0x71, d_71,  sizeof(d_71),  0},
    {0x66, d_66,  sizeof(d_66),  0},
    {0xBE, d_be,  sizeof(d_be),  0},
    {0x70, d_70,  sizeof(d_70),  0},
    {0x90, d_90,  sizeof(d_90),  0},
    {0x91, d_91,  sizeof(d_91),  0},
    {0x92, d_92,  sizeof(d_92),  0},
    {0x93, d_93,  sizeof(d_93),  0},
    {0x94, d_94,  sizeof(d_94),  0},
    {0x95, d_95,  sizeof(d_95),  0},
    {0x96, d_96,  sizeof(d_96),  0},
    {0x97, d_97,  sizeof(d_97),  0},
    {0xBA, d_ba,  sizeof(d_ba),  0},
    {0x9A, d_9a,  sizeof(d_9a),  0},
    {0x9B, d_9b,  sizeof(d_9b),  0},
    {0x9C, d_9c,  sizeof(d_9c),  0},
    {0x9D, d_9d,  sizeof(d_9d),  0},
    {0x9E, d_9e,  sizeof(d_9e),  0},
    {0xB4, d_b4,  sizeof(d_b4),  0},
    {0xB5, d_b5,  sizeof(d_b5),  0},
    {0xB6, d_b6,  sizeof(d_b6),  0},
    {0x86, d_86,  sizeof(d_86),  0},
    {0xB7, d_b7,  sizeof(d_b7),  0},
    {0xB8, d_b8,  sizeof(d_b8),  0},
    {0xB9, d_b9,  sizeof(d_b9),  0},
    {0xBF, d_bf1, sizeof(d_bf1), 0},
    {0xF2, d_f2,  sizeof(d_f2),  0},
    {0x73, d_73,  sizeof(d_73),  0},
    {0x77, d_77,  sizeof(d_77),  0},
    {0x7A, d_7a,  sizeof(d_7a),  0},
    {0x7B, d_7b2, sizeof(d_7b2), 0},
    {0x7E, d_7e,  sizeof(d_7e),  0},
    {0xBF, d_bf2, sizeof(d_bf2), 0},
    {0xE3, d_e3,  sizeof(d_e3),  0},
    {0xF0, d_f0,  sizeof(d_f0),  0},
    {0xD0, d_d0,  sizeof(d_d0),  0},
    {0x2A, d_2a,  sizeof(d_2a),  0},
    {0x2B, d_2b,  sizeof(d_2b),  0},
    {0x21, nullptr, 0,           0},
    {0x11, nullptr, 0,           120},
    {0x29, nullptr, 0,           0},
    {0x2C, nullptr, 0,           0},
    {0x3A, d_3a,  sizeof(d_3a),  0},
    {0x36, d_36,  sizeof(d_36),  0},
    {0x35, d_35,  sizeof(d_35),  20},
};

static spi_device_handle_t s_spi = NULL;

/* Logical (already-landscape) framebuffer, width x height =
 * FNK_N_NATIVE_HEIGHT x FNK_N_NATIVE_WIDTH (480 x 320). */
static uint16_t *s_fb = NULL;
static size_t    s_fb_pixels = 0;
static int s_width  = 0;   /* logical width  (480) */
static int s_height = 0;   /* logical height (320) */

/* Scratch buffer for the transposed (native-orientation) pixel burst,
 * sized to the full framebuffer so any dirty rectangle fits without
 * reallocation. PSRAM is fine here: the ESP32-S3's GDMA can source
 * SPI DMA transfers directly from PSRAM (unlike the legacy DMA
 * controller used by earlier chips). */
static uint16_t *s_tx_buf = NULL;

static int s_dirty_x1 = -1, s_dirty_y1 = -1, s_dirty_x2 = -1, s_dirty_y2 = -1;
static int s_clip_x = 0, s_clip_y = 0, s_clip_w = 0, s_clip_h = 0;

/* Last user-requested backlight percent, cached so display_sleep() /
 * display_wake() can restore the brightness after blanking the panel
 * (mirrors the same cache in display_axs15231b.cpp). Initialised to
 * 100 to match the panel's initial full-brightness state after
 * display_init(). */
static int s_bl_last_pct = 100;

static bool s_panel_asleep = false;

static void write_reg(uint8_t cmd, const void *data, uint8_t len)
{
    spi_transaction_ext_t tx = {};
    tx.base.flags = SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR;
    tx.base.cmd   = QSPI_1W_CMD;
    tx.base.addr  = ((uint32_t)cmd) << 8;
    tx.command_bits = 8;
    tx.address_bits = 24;
    if (len != 0) {
        tx.base.tx_buffer = data;
        tx.base.length = 8u * len;
    }
    if (FNK_N_CS_PIN >= 0) gpio_set_level((gpio_num_t)FNK_N_CS_PIN, 0);
    ESP_ERROR_CHECK(spi_device_polling_transmit(s_spi, (spi_transaction_t *)&tx));
    if (FNK_N_CS_PIN >= 0) gpio_set_level((gpio_num_t)FNK_N_CS_PIN, 1);
}

static void set_windows(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey)
{
    uint8_t x_data[] = {
        (uint8_t)(sx >> 8), (uint8_t)(sx & 0xFF),
        (uint8_t)((ex - 1) >> 8), (uint8_t)((ex - 1) & 0xFF)
    };
    uint8_t y_data[] = {
        (uint8_t)(sy >> 8), (uint8_t)(sy & 0xFF),
        (uint8_t)((ey - 1) >> 8), (uint8_t)((ey - 1) & 0xFF)
    };
    write_reg(SET_X_CMD, x_data, 4);
    write_reg(SET_Y_CMD, y_data, 4);
}

/* Stream w*h pixels (already in native panel orientation, tightly
 * packed row-major) starting at native window (sx, sy, w, h). */
static void fill_native_rect(uint16_t sx, uint16_t sy, uint16_t w, uint16_t h,
                             const uint16_t *tx_buf)
{
    size_t total = (size_t)w * h;
    set_windows(sx, sy, sx + w, sy + h);

    if (FNK_N_CS_PIN >= 0) gpio_set_level((gpio_num_t)FNK_N_CS_PIN, 0);
    bool first = true;
    while (total > 0) {
        size_t chunk = (total > TX_LEN) ? TX_LEN : total;
        spi_transaction_ext_t tx = {};
        if (first) {
            tx.base.flags = SPI_TRANS_MODE_QIO | SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR;
            tx.base.cmd  = QSPI_4W_CMD;
            tx.base.addr = ((uint32_t)WR_RAM_C_CMD) << 8;
            tx.command_bits = 8;
            tx.address_bits = 24;
            first = false;
        } else {
            tx.base.flags = SPI_TRANS_MODE_QIO | SPI_TRANS_VARIABLE_CMD |
                             SPI_TRANS_VARIABLE_ADDR | SPI_TRANS_VARIABLE_DUMMY;
            tx.command_bits = 0;
            tx.address_bits = 0;
            tx.dummy_bits = 0;
        }
        tx.base.tx_buffer = tx_buf;
        tx.base.length = chunk * 16;
        ESP_ERROR_CHECK(spi_device_polling_transmit(s_spi, (spi_transaction_t *)&tx));
        total  -= chunk;
        tx_buf += chunk;
    }
    if (FNK_N_CS_PIN >= 0) gpio_set_level((gpio_num_t)FNK_N_CS_PIN, 1);
}

/* Transpose a logical (already-landscape) rectangle of the
 * framebuffer into s_tx_buf in the panel's native portrait
 * coordinate space, following Freenove's Fill_Colors() rotation==1
 * mapping: physical(x, y) = (FNK_N_NATIVE_WIDTH - 1 - ly, lx). */
static void flush_rect(int lx, int ly, int w, int h)
{
    int phys_sx = FNK_N_NATIVE_WIDTH - ly - h;
    int phys_sy = lx;
    int phys_w  = h;
    int phys_h  = w;

    for (int row = 0; row < h; row++) {
        const uint16_t *src = s_fb + (size_t)(ly + row) * s_width + lx;
        for (int col = 0; col < w; col++) {
            s_tx_buf[col * phys_w + (phys_w - 1 - row)] = src[col];
        }
    }

    fill_native_rect((uint16_t)phys_sx, (uint16_t)phys_sy,
                      (uint16_t)phys_w, (uint16_t)phys_h, s_tx_buf);
}

extern "C" void display_init(int /*pin_a*/, int /*pin_b*/, int /*pin_c*/,
                             int /*pin_d*/, int /*pin_e*/, int /*pin_f*/,
                             int width, int height)
{
    s_width  = width;
    s_height = height;

    gpio_config_t cs_cfg = {};
    cs_cfg.intr_type    = GPIO_INTR_DISABLE;
    cs_cfg.mode         = GPIO_MODE_OUTPUT;
    cs_cfg.pin_bit_mask = (1ULL << FNK_N_CS_PIN);
    ESP_ERROR_CHECK(gpio_config(&cs_cfg));
    gpio_set_level((gpio_num_t)FNK_N_CS_PIN, 1);

    gpio_config_t bl_cfg = {};
    bl_cfg.intr_type    = GPIO_INTR_DISABLE;
    bl_cfg.mode         = GPIO_MODE_OUTPUT;
    bl_cfg.pin_bit_mask = (1ULL << FNK_N_BL_PIN);
    ESP_ERROR_CHECK(gpio_config(&bl_cfg));
    gpio_set_level((gpio_num_t)FNK_N_BL_PIN, 0);

    spi_bus_config_t bus_cfg = {};
    bus_cfg.data0_io_num = FNK_N_D0_PIN;
    bus_cfg.data1_io_num = FNK_N_D1_PIN;
    bus_cfg.sclk_io_num  = FNK_N_SCLK_PIN;
    bus_cfg.data2_io_num = FNK_N_D2_PIN;
    bus_cfg.data3_io_num = FNK_N_D3_PIN;
    bus_cfg.max_transfer_sz = (TX_LEN * 2) + 8;
    bus_cfg.flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_IOMUX_PINS | SPICOMMON_BUSFLAG_QUAD;
    ESP_ERROR_CHECK(spi_bus_initialize(FNK_N_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t dev_cfg = {};
    dev_cfg.command_bits   = 0;
    dev_cfg.address_bits   = 0;
    dev_cfg.mode           = 0;
    dev_cfg.clock_speed_hz = FNK_N_SPI_CLOCK_HZ;
    dev_cfg.spics_io_num   = -1; /* CS is driven manually, see write_reg() / fill_native_rect() */
    dev_cfg.flags          = SPI_DEVICE_HALFDUPLEX;
    dev_cfg.queue_size     = 17;
    ESP_ERROR_CHECK(spi_bus_add_device(FNK_N_SPI_HOST, &dev_cfg, &s_spi));

    s_fb_pixels = (size_t)width * height;
    s_fb = (uint16_t *)heap_caps_malloc(s_fb_pixels * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    s_tx_buf = (uint16_t *)heap_caps_malloc(s_fb_pixels * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    assert(s_fb && s_tx_buf);
    memset(s_fb, 0, s_fb_pixels * sizeof(uint16_t));

    for (size_t i = 0; i < sizeof(s_init_seq) / sizeof(s_init_seq[0]); i++) {
        write_reg(s_init_seq[i].cmd, s_init_seq[i].data, s_init_seq[i].len);
        if (s_init_seq[i].delay_ms) vTaskDelay(pdMS_TO_TICKS(s_init_seq[i].delay_ms));
    }
    /* MADCTL stays at the reference driver's rotation==1 value (row/
     * column order untouched); the landscape orientation is achieved
     * entirely by the software transpose in flush_rect(). */
    static const uint8_t madctl_rot1[] = {0x00};
    write_reg(MADCTL_CMD, madctl_rot1, sizeof(madctl_rot1));

    gpio_set_level((gpio_num_t)FNK_N_BL_PIN, 1);

    display_clear(0x00);
    display_full_refresh();

    ESP_LOGI(TAG, "FNK0104N ST77922 %dx%d initialized", width, height);
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

    /* Round the flush window out to 4-pixel boundaries. Freenove's own
     * reference LVGL integration (Tutorial_No_Touch/Sketches/
     * Sketch_11.1_LVGL/display.cpp, my_rounder_cb) does exactly this
     * -- "area->x1 &= ~0x3; area->y1 &= ~0x3; area->x2 |= 0x3;
     * area->y2 |= 0x3;" -- and it is only wired up for the ST77922
     * board (the ILI9341/ST7796 branch of the same file needs no
     * rounder). The panel's SET_X/SET_Y window is silently ignored
     * (or renders garbage) when the requested window is not aligned
     * to a 4-pixel boundary, so without this the initial full-screen
     * display_full_refresh() (already 480x320, both multiples of 4)
     * looks fine but every subsequent small, arbitrarily-positioned
     * LVGL partial redraw -- the actual editor UI text/title bar --
     * never reaches the panel, leaving it stuck on a blank backlit
     * screen after the first frame. Expanding is safe: the source
     * pixels for the extra rows/columns are already present in
     * s_fb from earlier writes. */
    x1 &= ~0x3;
    y1 &= ~0x3;
    x2 |= 0x3;
    y2 |= 0x3;
    if (x2 >= s_width)  x2 = s_width  - 1;
    if (y2 >= s_height) y2 = s_height - 1;

    flush_rect(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
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

extern "C" void display_set_backlight(int percent)
{
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    s_bl_last_pct = percent;
    /* No PWM dimming in Freenove's reference driver for this panel;
     * treat as a simple on/off enable. */
    gpio_set_level((gpio_num_t)FNK_N_BL_PIN, percent > 0 ? 1 : 0);
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
    write_reg(0x28, nullptr, 0); /* DISPOFF */
    write_reg(0x10, nullptr, 0); /* SLPIN */
}

extern "C" void display_wake(void)
{
    if (!s_panel_asleep) return;
    write_reg(0x11, nullptr, 0); /* SLPOUT */
    vTaskDelay(pdMS_TO_TICKS(120));
    write_reg(0x29, nullptr, 0); /* DISPON */
    display_set_backlight(s_bl_last_pct);
    s_panel_asleep = false;
    display_request_full_refresh();
}

extern "C" void display_deep_sleep_prepare(void)
{
    display_set_backlight(0);
    gpio_hold_en((gpio_num_t)FNK_N_BL_PIN);
    gpio_deep_sleep_hold_en();
}

extern "C" void display_set_shared_i2c_bus(void * /*bus_handle*/)
{
    /* The FNK0104N's touch controller sits on its own dedicated I2C
     * bus (see touchscreen.cpp), not shared with this backend. No-op. */
}

#endif /* CONFIG_DRAFTLING_DISPLAY_ST77922 */
