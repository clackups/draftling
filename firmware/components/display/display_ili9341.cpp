#include "sdkconfig.h"
#if defined(CONFIG_DRAFTLING_DISPLAY_ILI9341) || defined(CONFIG_DRAFTLING_DISPLAY_ST7796) || \
    defined(CONFIG_DRAFTLING_DISPLAY_ST7365) || defined(CONFIG_DRAFTLING_DISPLAY_ST7789)

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
 * The panel wiring (SPI pins, optional hardware RST, backlight,
 * interface-mode straps, SPI clock and backlight PWM frequency) is
 * board-specific and comes from main/boards/<board>.h through
 * display_spi_tft_config(), which main.cpp calls before
 * display_init(). The FNK0104 boards have no discrete RST line
 * (TFT_eSPI's setup headers define TFT_RST as -1 for this family),
 * so this backend sends SWRESET; boards that wire RST get a hardware
 * reset pulse first.
 *
 * See https://github.com/Freenove/Freenove_ESP32_S3_Display for the
 * reference TFT_eSPI setup headers and Arduino sketches this backend
 * is derived from.
 *
 * The same file also drives the Viewe UEED035HV-RX40-L001 3.5"
 * 320x480 transflective display (ST7365 controller, ST7796-compatible
 * command set) attached to a Viewe UEDX24320028E-WB-A board
 * (CONFIG_DRAFTLING_DISPLAY_ST7365): same MIPI-DCS command set
 * (CASET/RASET/RAMWR/MADCTL) and RGB565 pipeline, but a real RST
 * line, two interface-mode straps (IM2 is tied HIGH on the board) and
 * Viewe's own init table. The board's stock 2.8" GC9307 panel uses IM2..0 = 110; this
 * display only answers with IM2..0 = 111 and needs display inversion
 * ON -- the same configuration as Viewe's UEDX32480035E-WB-A
 * (ESP32_Display_Panel board file BOARD_VIEWE_UEDX32480035E_WB_A.h in
 * https://github.com/VIEWESMART/UEDX24320028ESP32-2.8inch-Touch-Display),
 * which the straps and init table here follow. Tested on hardware.
 *
 * It also drives the ST7789 version of the 2.8" 240x320 module used by
 * the MicroJournal Rev 5.1 (CONFIG_DRAFTLING_DISPLAY_ST7789). The init
 * table is TFT_eSPI's ST7789_Init.h (2.5.43), which the upstream
 * Micro Journal firmware (rev_5_st7789: ST7789_DRIVER, TFT_RGB_ORDER =
 * TFT_BGR, TFT_INVERSION_OFF) uses.
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
#include "display_margins.h"

static const char *TAG = "DisplayILI9341";

/* Board wiring, set by display_spi_tft_config() before display_init(). */
static display_spi_tft_config_t s_cfg = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, 40 * 1000 * 1000, 50000
};

#define FNK_SPI_HOST       SPI2_HOST

/* LEDC PWM backlight, mirroring the AXS15231B backend's active-HIGH
 * default configuration (Guition JC3248W535 convention: duty MAX =
 * full brightness). */
#define BL_LEDC_TIMER       LEDC_TIMER_3
#define BL_LEDC_MODE        LEDC_LOW_SPEED_MODE
#define BL_LEDC_CHANNEL     LEDC_CHANNEL_1
#define BL_LEDC_DUTY_RES    LEDC_TIMER_8_BIT
#define BL_LEDC_DUTY_MAX    ((1 << 8) - 1)

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
 * same pattern used by display_rlcd.cpp) avoids the race entirely.
 * PSRAM is fine here: the ESP32-S3's GDMA
 * can source SPI DMA transfers directly from PSRAM -- provided
 * io_cfg.flags.psram_dma_direct is set (see display_init()) and the
 * buffer is DMA/cache-line aligned; see the allocation below. */
static uint8_t *s_tx_buf = NULL;

/* esp_lcd_panel_io_spi caps a single SPI transaction at 32 KB
 * (SPI_LL_DMA_MAX_BIT_LEN on ESP32-S3) and splits any larger
 * esp_lcd_panel_io_tx_color() buffer into that many chunks
 * internally. Without io_cfg.flags.psram_dma_direct (the default),
 * each chunk that lives in PSRAM is bounced through a *freshly
 * malloc'd internal-DRAM* buffer for the DMA transfer, because the
 * driver assumes PSRAM source buffers need a DMA-capable-internal
 * copy unless told otherwise. On FNK0104S a full-screen flush is
 * 307200 bytes -> 10 chunks -> up to 10 simultaneous 32 KB internal
 * allocations in flight (trans_queue_depth), which can exhaust
 * internal SRAM once the rest of the firmware (LVGL, BLE, WiFi) has
 * claimed its share; esp_lcd_panel_io_tx_color()'s return value is
 * not checked here, so an ESP_ERR_NO_MEM part-way through a big
 * flush silently drops the rest of the rectangle instead of
 * asserting -- this reproduced as the lower half of the screen (the
 * later rows of a full-height flush) never updating while small
 * writes near the top (e.g. the status-bar battery percentage)
 * still succeeded. Setting psram_dma_direct lets the driver DMA
 * straight out of s_tx_buf instead, provided the buffer (and every
 * 32 KB chunk offset into it) is aligned; DMA_ALIGN_BYTES keeps both
 * the allocation and the chunk size aligned so the fast path is
 * always taken. */
#define DMA_ALIGN_BYTES 64

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

extern "C" void display_spi_tft_config(const display_spi_tft_config_t *cfg)
{
    s_cfg = *cfg;
}

/* Hardware reset pulse on boards that wire the RST line. */
static void panel_hw_reset(void)
{
    if (s_cfg.rst < 0) return;
    gpio_set_level((gpio_num_t)s_cfg.rst, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level((gpio_num_t)s_cfg.rst, 1);
    vTaskDelay(pdMS_TO_TICKS(120));
}

static void backlight_pwm_init(void)
{
    if (s_cfg.bl < 0) return;

    /* display_deep_sleep_prepare() below latches this pin LOW with
     * gpio_hold_en() + gpio_deep_sleep_hold_en() so the backlight
     * stays off through deep sleep. That hold survives the wake
     * reset on ESP32-S3 (deep sleep re-runs app_main() from scratch,
     * it is not a resume), so without releasing it here the pad
     * stays clamped LOW forever: gpio_config() / ledc_channel_config()
     * below reconfigure the peripheral side but the physical pad
     * keeps ignoring it, leaving the backlight (and thus the whole
     * screen) dark after every wake from deep sleep. Matches the
     * same gpio_hold_dis() call in display_epdiy.cpp, display_h752.cpp
     * and display_axs15231b.cpp's backlight init. Safe to call
     * unconditionally on a cold boot too: releasing a pad that was
     * never held is a no-op. */
    gpio_hold_dis((gpio_num_t)s_cfg.bl);

    gpio_config_t g = {};
    g.intr_type    = GPIO_INTR_DISABLE;
    g.mode         = GPIO_MODE_OUTPUT;
    g.pin_bit_mask = (1ULL << s_cfg.bl);
    ESP_ERROR_CHECK(gpio_config(&g));
    gpio_set_level((gpio_num_t)s_cfg.bl, 1);

    ledc_timer_config_t t = {};
    t.speed_mode      = BL_LEDC_MODE;
    t.duty_resolution = BL_LEDC_DUTY_RES;
    t.timer_num       = BL_LEDC_TIMER;
    t.freq_hz         = s_cfg.bl_pwm_hz;
    t.clk_cfg         = LEDC_USE_RC_FAST_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&t));

    ledc_channel_config_t c = {};
    c.gpio_num   = s_cfg.bl;
    c.speed_mode = BL_LEDC_MODE;
    c.channel    = BL_LEDC_CHANNEL;
    c.timer_sel  = BL_LEDC_TIMER;
    c.duty       = BL_LEDC_DUTY_MAX;
    c.hpoint     = 0;
    ESP_ERROR_CHECK(ledc_channel_config(&c));
}

extern "C" void display_set_backlight(int percent)
{
    if (s_cfg.bl < 0) return;
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
 * on a real board, try MADCTL 0xE8 (MY|MX|MV|BGR) instead.
 * The Viewe panel's vendor portrait orientation is MADCTL 0x48
 * (MX|BGR), the same as ILI9341 rotation 0, so the same landscape
 * value applies there. */
#define FNK_MADCTL_LANDSCAPE  0x28

/* The ST7789's native scan direction differs from the ILI9341's, so the
 * same landscape picture takes MX | MV | BGR (TFT_eSPI's ST7789
 * rotation 1) instead of MV | BGR. */
#define ST7789_MADCTL_LANDSCAPE  0x68

#if defined(CONFIG_DRAFTLING_DISPLAY_ILI9341)
static void ili9341_init_sequence(void)
{
    panel_hw_reset();
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
#endif

#if defined(CONFIG_DRAFTLING_DISPLAY_ST7789)
static void st7789_init_sequence(void)
{
    panel_hw_reset();
    send_command(0x01); /* SWRESET */
    vTaskDelay(pdMS_TO_TICKS(150));

    static const uint8_t madctl[] = {ST7789_MADCTL_LANDSCAPE};
    static const uint8_t b6[]     = {0x0A, 0x82};
    static const uint8_t ramctrl[] = {0x00, 0xE0};
    static const uint8_t colmod[] = {0x55};
    static const uint8_t porctrl[] = {0x0C, 0x0C, 0x00, 0x33, 0x33};
    static const uint8_t gctrl[]  = {0x35};
    static const uint8_t vcoms[]  = {0x28};
    static const uint8_t lcmctrl[] = {0x0C};
    static const uint8_t vdvvrhen[] = {0x01, 0xFF};
    static const uint8_t vrhs[]   = {0x10};
    static const uint8_t vdvset[] = {0x20};
    static const uint8_t frctr2[] = {0x0F};
    static const uint8_t pwctrl1[] = {0xA4, 0xA1};
    static const uint8_t e0[]     = {0xD0, 0x00, 0x02, 0x07, 0x0A, 0x28, 0x32,
                                     0x44, 0x42, 0x06, 0x0E, 0x12, 0x14, 0x17};
    static const uint8_t e1[]     = {0xD0, 0x00, 0x02, 0x07, 0x0A, 0x28, 0x31,
                                     0x54, 0x47, 0x0E, 0x1C, 0x17, 0x1B, 0x1E};

    send_command(0x11); /* SLPOUT */
    vTaskDelay(pdMS_TO_TICKS(120));
    send_command(0x13); /* NORON */
    send_cmd_data(0x36, madctl, sizeof(madctl));
    send_cmd_data(0xB6, b6, sizeof(b6));
    send_cmd_data(0xB0, ramctrl, sizeof(ramctrl));
    send_cmd_data(0x3A, colmod, sizeof(colmod));
    vTaskDelay(pdMS_TO_TICKS(10));
    send_cmd_data(0xB2, porctrl, sizeof(porctrl));
    send_cmd_data(0xB7, gctrl, sizeof(gctrl));
    send_cmd_data(0xBB, vcoms, sizeof(vcoms));
    send_cmd_data(0xC0, lcmctrl, sizeof(lcmctrl));
    send_cmd_data(0xC2, vdvvrhen, sizeof(vdvvrhen));
    send_cmd_data(0xC3, vrhs, sizeof(vrhs));
    send_cmd_data(0xC4, vdvset, sizeof(vdvset));
    send_cmd_data(0xC6, frctr2, sizeof(frctr2));
    send_cmd_data(0xD0, pwctrl1, sizeof(pwctrl1));
    send_cmd_data(0xE0, e0, sizeof(e0));
    send_cmd_data(0xE1, e1, sizeof(e1));

    /* TFT_eSPI's table ends with INVON, but the upstream firmware builds
     * this panel with TFT_INVERSION_OFF, which TFT_eSPI applies right
     * after the table -- so the net state is inversion off. */
    send_command(0x20); /* INVOFF */

    vTaskDelay(pdMS_TO_TICKS(120));
    send_command(0x29); /* DISPON */
    vTaskDelay(pdMS_TO_TICKS(120));
}
#endif

#if defined(CONFIG_DRAFTLING_DISPLAY_ST7796)
static void st7796_init_sequence(void)
{
    panel_hw_reset();
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
#endif

#if defined(CONFIG_DRAFTLING_DISPLAY_ST7365)
static void st7365_init_sequence(void)
{
    /* Hardware reset. The IM0/IM1 straps were set in display_init()
     * before this runs, so the panel samples 4-wire SPI mode here. */
    panel_hw_reset();

    send_command(0x11); /* SLPOUT */
    vTaskDelay(pdMS_TO_TICKS(120));

    /* Viewe's init table for this panel (BOARD_VIEWE_UEDX32480035E_WB_A),
     * with MADCTL / COLMOD added in place of the ones its driver sends
     * itself. The repeated 0xC5 write is copied as-is from the vendor
     * table (the second value wins). */
    static const uint8_t cmd_lock_en1[]  = {0xC3};
    static const uint8_t cmd_lock_en2[]  = {0x96};
    static const uint8_t madctl[]        = {FNK_MADCTL_LANDSCAPE};
    static const uint8_t colmod[]        = {0x55};
    static const uint8_t b4[]            = {0x01};
    static const uint8_t b7[]            = {0xC6};
    static const uint8_t c0[]            = {0x80, 0x04};
    static const uint8_t c1[]            = {0x13};
    static const uint8_t c5a[]           = {0xA7};
    static const uint8_t c5b[]           = {0x16};
    static const uint8_t e8[]            = {0x40, 0x8A, 0x00, 0x00, 0x29,
                                            0x19, 0xA5, 0x33};
    static const uint8_t e0[]            = {0xF0, 0x19, 0x20, 0x10, 0x11, 0x0A,
                                            0x46, 0x44, 0x57, 0x09, 0x1A, 0x1B,
                                            0x2A, 0x2D};
    static const uint8_t e1[]            = {0xF0, 0x12, 0x1A, 0x0A, 0x0C, 0x18,
                                            0x45, 0x44, 0x56, 0x3F, 0x15, 0x11,
                                            0x24, 0x26};
    static const uint8_t cmd_lock_dis1[] = {0x3C};
    static const uint8_t cmd_lock_dis2[] = {0x69};

    send_cmd_data(0xF0, cmd_lock_en1, sizeof(cmd_lock_en1));
    send_cmd_data(0xF0, cmd_lock_en2, sizeof(cmd_lock_en2));
    send_cmd_data(0x36, madctl, sizeof(madctl));
    send_cmd_data(0x3A, colmod, sizeof(colmod));
    send_cmd_data(0xB4, b4, sizeof(b4));
    send_cmd_data(0xB7, b7, sizeof(b7));
    send_cmd_data(0xC0, c0, sizeof(c0));
    send_cmd_data(0xC1, c1, sizeof(c1));
    send_cmd_data(0xC5, c5a, sizeof(c5a));
    send_cmd_data(0xC5, c5b, sizeof(c5b));
    send_cmd_data(0xE8, e8, sizeof(e8));
    send_cmd_data(0xE0, e0, sizeof(e0));
    send_cmd_data(0xE1, e1, sizeof(e1));
    send_cmd_data(0xF0, cmd_lock_dis1, sizeof(cmd_lock_dis1));
    send_cmd_data(0xF0, cmd_lock_dis2, sizeof(cmd_lock_dis2));

    /* Inversion ON, as in the vendor configuration -- without it the
     * panel shows inverted colours (e.g. green-on-black as pink on
     * white). */
    send_command(0x21); /* INVON */
    send_command(0x29); /* DISPON */
    vTaskDelay(pdMS_TO_TICKS(50));
}
#endif

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

    /* Interface-mode straps (if the board routes them to GPIOs) and
     * RST. On the Viewe ST7365 panel IM2..0 = 111 selects 4-wire
     * 8-bit SPI (it does not respond at all with the stock GC9307
     * panel's setting 110); the straps must be settled before the
     * reset pulse in the init sequence. RST is held high (inactive)
     * until then. */
    {
        uint64_t mask = 0;
        if (s_cfg.im0 >= 0) mask |= 1ULL << s_cfg.im0;
        if (s_cfg.im1 >= 0) mask |= 1ULL << s_cfg.im1;
        if (s_cfg.rst >= 0) mask |= 1ULL << s_cfg.rst;
        if (mask) {
            gpio_config_t g = {};
            g.intr_type    = GPIO_INTR_DISABLE;
            g.mode         = GPIO_MODE_OUTPUT;
            g.pin_bit_mask = mask;
            ESP_ERROR_CHECK(gpio_config(&g));
            if (s_cfg.im0 >= 0) gpio_set_level((gpio_num_t)s_cfg.im0, 1);
            if (s_cfg.im1 >= 0) gpio_set_level((gpio_num_t)s_cfg.im1, 1);
            if (s_cfg.rst >= 0) gpio_set_level((gpio_num_t)s_cfg.rst, 1);
        }
    }

    spi_bus_config_t bus_cfg = {};
    bus_cfg.miso_io_num   = s_cfg.miso;
    bus_cfg.mosi_io_num   = s_cfg.mosi;
    bus_cfg.sclk_io_num   = s_cfg.sck;
    bus_cfg.quadwp_io_num = -1;
    bus_cfg.quadhd_io_num = -1;
    bus_cfg.max_transfer_sz = width * height * 2;
    ESP_ERROR_CHECK(spi_bus_initialize(FNK_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_spi_config_t io_cfg = {};
    io_cfg.dc_gpio_num      = (gpio_num_t)s_cfg.dc;
    io_cfg.cs_gpio_num      = (gpio_num_t)s_cfg.cs;
    io_cfg.pclk_hz          = s_cfg.spi_clock_hz;
    io_cfg.lcd_cmd_bits     = 8;
    io_cfg.lcd_param_bits   = 8;
    io_cfg.spi_mode         = 0;
    io_cfg.trans_queue_depth = 10;
    /* See the DMA_ALIGN_BYTES comment above s_tx_buf's declaration:
     * without this flag, esp_lcd_panel_io_tx_color() bounces every
     * >32 KB chunk of a PSRAM color buffer through a freshly
     * malloc'd internal-DRAM buffer, which can silently fail (and
     * drop the rest of the flush) once internal SRAM is under
     * pressure. */
    io_cfg.flags.psram_dma_direct = 1;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)FNK_SPI_HOST, &io_cfg, &s_io_handle));

    s_fb_pixels = (size_t)width * height;
    s_fb = (uint16_t *)heap_caps_malloc(s_fb_pixels * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    /* Aligned (and sized so every 32 KB DMA chunk offset stays
     * aligned) so the psram_dma_direct fast path above is always
     * taken instead of falling back to the internal-DRAM bounce
     * copy for a misaligned chunk. */
    size_t tx_buf_size = (s_fb_pixels * sizeof(uint16_t) + DMA_ALIGN_BYTES - 1) &
                         ~(size_t)(DMA_ALIGN_BYTES - 1);
    s_tx_buf = (uint8_t *)heap_caps_aligned_alloc(DMA_ALIGN_BYTES, tx_buf_size, MALLOC_CAP_SPIRAM);
    assert(s_fb && s_tx_buf);
    memset(s_fb, 0, s_fb_pixels * sizeof(uint16_t));

    backlight_pwm_init();

#if defined(CONFIG_DRAFTLING_DISPLAY_ST7365)
    st7365_init_sequence();
#elif defined(CONFIG_DRAFTLING_DISPLAY_ST7789)
    st7789_init_sequence();
#elif defined(CONFIG_DRAFTLING_DISPLAY_ST7796)
    st7796_init_sequence();
#else
    ili9341_init_sequence();
#endif

    display_clear(0x00);
    display_full_refresh();

    ESP_LOGI(TAG, "SPI-TFT %dx%d initialized", width, height);
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
    /* Coordinates are *logical* (margin-shrunk) pixels -- offset by
     * the left/top margin into the full physical panel (same
     * convention as display_ws_epd397.cpp / display_xteink_epd.cpp). */
    int px = (int)x + display_margin_left();
    int py = (int)y + display_margin_top();
    if (px < 0 || py < 0 || px >= s_width || py >= s_height) return;
    s_fb[(size_t)py * s_width + px] = color ? 0xFFFF : 0x0000;
    if (s_dirty_x1 < 0) {
        s_dirty_x1 = s_dirty_x2 = px;
        s_dirty_y1 = s_dirty_y2 = py;
    } else {
        if (px < s_dirty_x1) s_dirty_x1 = px;
        if (px > s_dirty_x2) s_dirty_x2 = px;
        if (py < s_dirty_y1) s_dirty_y1 = py;
        if (py > s_dirty_y2) s_dirty_y2 = py;
    }
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
    esp_err_t ret = esp_lcd_panel_io_tx_color(s_io_handle, -1, s_tx_buf, (size_t)w * h * 2);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "tx_color failed (%s), dropped %dx%d flush", esp_err_to_name(ret), w, h);
    }
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
    if (s_cfg.bl >= 0) {
        gpio_hold_en((gpio_num_t)s_cfg.bl);
        gpio_deep_sleep_hold_en();
    }
}

extern "C" void display_set_shared_i2c_bus(void * /*bus_handle*/)
{
    /* This backend does not use I2C. No-op. */
}


const char *display_controller_name(void)
{
#if defined(CONFIG_DRAFTLING_DISPLAY_ST7796)
    return "ST7796";
#elif defined(CONFIG_DRAFTLING_DISPLAY_ST7365)
    return "ST7365";
#elif defined(CONFIG_DRAFTLING_DISPLAY_ST7789)
    return "ST7789";
#else
    return "ILI9341";
#endif
}

#endif /* CONFIG_DRAFTLING_DISPLAY_ILI9341 || _ST7796 || _ST7365 || _ST7789 */
