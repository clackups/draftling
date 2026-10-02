#include "sdkconfig.h"
#if defined(CONFIG_DRAFTLING_DISPLAY_RGB)

/*
 * Parallel RGB565 color-LCD driver (ESP32-S3 LCD RGB peripheral).
 *
 * Used by:
 *   - Sunton ESP32-8048S070C (7", 800x480, 16-bit parallel RGB)
 *   - Sunton ESP32-8048S043C (4.3", 800x480, ST7262, 16-bit RGB)
 *   - Waveshare ESP32-S3-Touch-LCD-7 (7", 800x480, ST7262, 16-bit
 *     RGB; LCD reset and backlight sit behind a CH422G I2C
 *     IO-expander instead of direct GPIOs -- see
 *     CONFIG_DRAFTLING_HAS_CH422G below)
 *   - Waveshare ESP32-S3-LCD-3.16 (3.16", 320x820 portrait, ST7701,
 *     16-bit RGB; the controller needs a vendor init sequence over a
 *     bit-banged 3-wire SPI bus before it shows anything -- see
 *     CONFIG_DRAFTLING_RGB_PANEL_ST7701 below)
 *
 * The ESP32-S3 LCD peripheral drives a "dumb" RGB TFT directly:
 * a continuously-scanned-out framebuffer in PSRAM is shifted out
 * pixel-by-pixel on the 16 data lines under the HSYNC / VSYNC / DE /
 * PCLK timing programmed below. esp_lcd_new_rgb_panel() owns all the
 * panel GPIOs; esp_lcd_panel_draw_bitmap() copies a rectangle into
 * the scan-out framebuffer.
 *
 * Architecture
 * ------------
 * This backend keeps its own RGB565 framebuffer in PSRAM (sized to
 * width * height * 2). The LVGL port's flush_cb pushes RGB565
 * rectangles into it via display_push_rgb565() (with SCALE x SCALE
 * nearest-neighbor expansion of each logical pixel), accumulating a
 * dirty bounding box; display_flush() copies just that region to the
 * panel via esp_lcd_panel_draw_bitmap(). A degraded per-pixel
 * fallback (display_set_pixel, interpreting 0/0xFF as black/white) is
 * provided for the splash-screen logo path in editor_ui.cpp.
 *
 * Per-board pin map / timings are selected at build time:
 * CONFIG_DRAFTLING_RGB_BOARD_S043 (4.3" Sunton ESP32-8048S043C) and
 * CONFIG_DRAFTLING_HAS_CH422G (Waveshare ESP32-S3-Touch-LCD-7) each
 * select their own branch; the default (neither set) is the 7"
 * Sunton ESP32-8048S070C. All three boards share the 800x480
 * resolution but differ in their control-pin map, panel timings and
 * data-line order. The data GPIOs are always listed in the order
 * required by the panel so a standard RGB565 pixel lands on the
 * correct color lines.
 *
 * Backlight and reset
 * --------------------
 * The Sunton boards drive backlight via LEDC PWM on a direct GPIO and
 * have no software-controlled LCD reset line. The Waveshare
 * ESP32-S3-Touch-LCD-7 instead routes both through a CH422G I2C
 * IO-expander shared with the GT911 touchscreen bus: the backlight
 * EXIO pin is on/off only (no PWM), and the LCD reset EXIO pin must
 * be pulsed low-then-high before esp_lcd_new_rgb_panel() /
 * esp_lcd_panel_init() run (the RGB peripheral's own
 * esp_lcd_panel_reset() is a no-op for this panel type -- there is no
 * dedicated reset_gpio_num field in esp_lcd_rgb_panel_config_t).
 * main.cpp has already called ch422g_init() on the shared I2C bus by
 * the time display_init() runs (see CONFIG_DRAFTLING_HAS_CH422G in
 * main.cpp), so this file only needs to pulse/set individual EXIO
 * pins via ch422g_set_pin().
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_heap_caps.h>
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_rgb.h>
#include <esp_rom_sys.h>

#include "display.h"
#include "display_margins.h"
#include "io_expander_ch422g.h"

static const char *TAG = "DisplayRGB";

/* ---- Panel pin map + timings (per board) ----
 *
 * Backlight is GPIO2 on both Sunton boards. The control pins, panel
 * timings and data-line order differ between the 7" ESP32-8048S070C
 * (default) and the 4.3" ESP32-8048S043C
 * (CONFIG_DRAFTLING_RGB_BOARD_S043). esp_lcd_new_rgb_panel maps
 * data_gpio_nums[0] to the least-significant bit of the 16-bit RGB565
 * word and [15] to the most-significant; RGB565 packs as
 * R[15:11] G[10:5] B[4:0]. Each board's data array is listed in the
 * order its panel wiring requires so a standard RGB565 pixel lands on
 * the correct color lines. */
#define RGB_DISP_GPIO       -1

#if defined(CONFIG_DRAFTLING_RGB_PANEL_ST7701)
/* ---- Waveshare ESP32-S3-LCD-3.16 (3.16", ST7701, 320x820) ----
 * Unlike the ST7262 panels below, the ST7701 is a full controller
 * that stays blank until it gets the vendor's register sequence
 * (st7701_init_cmds[] below) over a 3-wire, 9-bit SPI bus. That bus
 * shares its pins with other functions: CS is GPIO0 (the BOOT
 * button), SCK is GPIO2 and SDA is GPIO1 (the MicroSD slot's SDMMC
 * CMD and CLK). The sequence is sent once from display_init(), then
 * the three pins are released for the BOOT button and the SD card --
 * the same "IO multiplex" mode as the vendor's esp_lcd_st7701 setup.
 * The panel ignores the SD traffic because its CS (BOOT, pulled up)
 * stays high.
 *
 * Pins, timings and the init table come from the community ESP-IDF
 * projects for this board (fabian-bxr/ESP32-S3-LCD-3.16,
 * thelastoutpostworkshop/ESP32-S3_3_16_ST7701_movie_player), which
 * agree with each other and carry the manufacturer's demo values.
 * The 16 data lines are listed B0-4, G0-5, R0-4, as in those
 * projects -- data_gpio_nums[0] is the RGB565 LSB. Backlight is
 * LEDC PWM on GPIO6, active low (duty 0 = full brightness). */
#define RGB_BL_GPIO         6
#define RGB_BL_ACTIVE_LOW   1
#define RGB_RST_GPIO        16
#define ST7701_SPI_CS_GPIO  0
#define ST7701_SPI_SCK_GPIO 2
#define ST7701_SPI_SDA_GPIO 1
#define RGB_PCLK_HZ         (16 * 1000 * 1000)   /* 16 MHz */
#define RGB_HSYNC_GPIO      38
#define RGB_VSYNC_GPIO      39
#define RGB_DE_GPIO         40
#define RGB_PCLK_GPIO       41
#define RGB_HSYNC_PULSE     6
#define RGB_HSYNC_BACK      30
#define RGB_HSYNC_FRONT     30
#define RGB_VSYNC_PULSE     40
#define RGB_VSYNC_BACK      20
#define RGB_VSYNC_FRONT     20
#define RGB_PCLK_ACTIVE_NEG 0
#define RGB_PCLK_IDLE_HIGH  0

static const int kDataGpios[16] = {
    21, 5, 45, 48, 47,          /* B0-B4 */
    14, 13, 12, 11, 10, 9,      /* G0-G5 */
    17, 46, 3, 8, 18            /* R0-R4 */
};

#elif defined(CONFIG_DRAFTLING_HAS_CH422G)
/* ---- Waveshare ESP32-S3-Touch-LCD-7 (7", ST7262, CH422G) ----
 * Backlight (EXIO2) and LCD reset (EXIO3) are on the CH422G, not a
 * direct GPIO -- see backlight_ch422g_set() / display_init() below.
 * Timings and data-line map are from the upstream
 * esp-arduino-libs/ESP32_Display_Panel board file
 * (BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_7.h), which lists the 16 data
 * GPIOs in B0-4, G0-5, R0-4 order -- already the order
 * esp_lcd_new_rgb_panel needs (data_gpio_nums[0] = RGB565 LSB), so
 * no B,G,R reordering is needed here (unlike the Sunton 7" board). */
#define RGB_CH422G_BL_EXIO   2
#define RGB_CH422G_RST_EXIO  3
#define RGB_PCLK_HZ         (16 * 1000 * 1000)   /* 16 MHz */
#define RGB_HSYNC_GPIO      46
#define RGB_VSYNC_GPIO      3
#define RGB_DE_GPIO         5
#define RGB_PCLK_GPIO       7
#define RGB_HSYNC_PULSE     4
#define RGB_HSYNC_BACK      8
#define RGB_HSYNC_FRONT     8
#define RGB_VSYNC_PULSE     4
#define RGB_VSYNC_BACK      8
#define RGB_VSYNC_FRONT     8
#define RGB_PCLK_ACTIVE_NEG 1
#define RGB_PCLK_IDLE_HIGH  0

static const int kDataGpios[16] = {
    14, 38, 18, 17, 10,     /* B0-B4 */
    39, 0, 45, 48, 47, 21,  /* G0-G5 */
    1, 2, 42, 41, 40        /* R0-R4 */
};

#elif defined(CONFIG_DRAFTLING_RGB_BOARD_S043)
#define RGB_BL_GPIO         2
/* ---- Sunton ESP32-8048S043C (4.3", ST7262) ---- */
#define RGB_PCLK_HZ         (12500 * 1000)   /* 12.5 MHz */
#define RGB_HSYNC_GPIO      39
#define RGB_VSYNC_GPIO      41
#define RGB_DE_GPIO         40
#define RGB_PCLK_GPIO       42
#define RGB_HSYNC_PULSE     4
#define RGB_HSYNC_BACK      8
#define RGB_HSYNC_FRONT     8
#define RGB_VSYNC_PULSE     4
#define RGB_VSYNC_BACK      8
#define RGB_VSYNC_FRONT     8
#define RGB_PCLK_ACTIVE_NEG 1   /* ST7262 panel needs active-low PCLK */
#define RGB_PCLK_IDLE_HIGH  0

/* Data lines in R,G,B order (the 4.3" panel's required wiring).
 * R0-R4 -> bits 0..4, G0-G5 -> 5..10, B0-B4 -> 11..15. */
static const int kDataGpios[16] = {
    8, 3, 46, 9, 1,       /* R0-R4 */
    5, 6, 7, 15, 16, 4,   /* G0-G5 */
    45, 48, 47, 21, 14    /* B0-B4 */
};

#else
#define RGB_BL_GPIO         2
/* ---- Sunton ESP32-8048S070C (7", default) ---- */
#define RGB_PCLK_HZ         (12 * 1000 * 1000)   /* 12 MHz */
#define RGB_HSYNC_GPIO      39
#define RGB_VSYNC_GPIO      40
#define RGB_DE_GPIO         41
#define RGB_PCLK_GPIO       42
#define RGB_HSYNC_PULSE     2
#define RGB_HSYNC_BACK      43
#define RGB_HSYNC_FRONT     8
#define RGB_VSYNC_PULSE     2
#define RGB_VSYNC_BACK      12
#define RGB_VSYNC_FRONT     8
#define RGB_PCLK_ACTIVE_NEG 0
#define RGB_PCLK_IDLE_HIGH  1

/* Data lines in B,G,R order. The verified Sunton 7" wiring lists the
 * physical pins in R,G,B order; we place them in B,G,R order here so
 * a standard RGB565 pixel lands on the correct color lines. With the
 * R,G,B order red and blue came out swapped (e.g. "orange on black"
 * rendered as blue on black).
 * B0-B4 -> bits 0..4, G0-G5 -> 5..10, R0-R4 -> 11..15. */
static const int kDataGpios[16] = {
    15, 7, 6, 5, 4,       /* B0-B4 */
    9, 46, 3, 8, 16, 1,   /* G0-G5 */
    14, 21, 47, 48, 45    /* R0-R4 */
};
#endif

#ifndef RGB_BL_ACTIVE_LOW
#define RGB_BL_ACTIVE_LOW   0
#endif

#if !defined(CONFIG_DRAFTLING_HAS_CH422G)
/* ---- Backlight LEDC (direct GPIO) ---- */
#define BL_LEDC_TIMER       LEDC_TIMER_0
#define BL_LEDC_MODE        LEDC_LOW_SPEED_MODE
#define BL_LEDC_CHANNEL     LEDC_CHANNEL_0
#define BL_LEDC_DUTY_RES    LEDC_TIMER_8_BIT
#define BL_LEDC_DUTY_MAX    ((1 << 8) - 1)
#define BL_LEDC_FREQ_HZ     1000
#endif

/* Logical-to-panel pixel scale (Kconfig). Each logical LVGL pixel is
 * rendered as SCALE x SCALE physical panel pixels. */
/* Panels render 1:1; the former DRAFTLING_DISPLAY_SCALE upscale
 * has been removed (high-density boards use a larger font instead). */
#define RGB_SCALE 1

static esp_lcd_panel_handle_t s_panel = NULL;
static int s_width  = 0;
static int s_height = 0;

/* Host-side RGB565 framebuffer (PSRAM). */
static uint16_t *s_fb = NULL;
static size_t    s_fb_pixels = 0;

/* Accumulated dirty bounding box (inclusive); (-1,...) means clean. */
static int s_dirty_x1 = -1;
static int s_dirty_y1 = -1;
static int s_dirty_x2 = -1;
static int s_dirty_y2 = -1;

static int  s_bl_pin = -1;
static int  s_bl_last_pct = 100;

#if defined(CONFIG_DRAFTLING_HAS_CH422G)
/* Waveshare ESP32-S3-Touch-LCD-7: the backlight EXIO is a plain
 * on/off switch (ESP_PANEL_BACKLIGHT_TYPE_SWITCH_EXPANDER in the
 * upstream reference), not a PWM-capable pin, so any non-zero
 * percent turns it fully on -- matching every other CH422G-based
 * Waveshare board port. */
static void backlight_ch422g_set(int percent)
{
    ch422g_set_pin(RGB_CH422G_BL_EXIO, percent > 0);
}
#else
/* LEDC duty for a brightness percentage, honouring the pin's
 * polarity (RGB_BL_ACTIVE_LOW: duty 0 = full brightness). */
static uint32_t backlight_duty(int percent)
{
    uint32_t duty = (uint32_t)((BL_LEDC_DUTY_MAX * percent) / 100);
    return RGB_BL_ACTIVE_LOW ? BL_LEDC_DUTY_MAX - duty : duty;
}

static void backlight_pwm_init(int bl_pin)
{
    if (bl_pin < 0) return;

    /* display_deep_sleep_prepare() may have latched the pin at its
     * "off" level for deep sleep; release it so LEDC can drive it. */
    gpio_hold_dis((gpio_num_t)bl_pin);

    ledc_timer_config_t t = {};
    t.speed_mode      = BL_LEDC_MODE;
    t.duty_resolution = BL_LEDC_DUTY_RES;
    t.timer_num       = BL_LEDC_TIMER;
    t.freq_hz         = BL_LEDC_FREQ_HZ;
    t.clk_cfg         = LEDC_AUTO_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&t));

    ledc_channel_config_t c = {};
    c.gpio_num   = bl_pin;
    c.speed_mode = BL_LEDC_MODE;
    c.channel    = BL_LEDC_CHANNEL;
    c.timer_sel  = BL_LEDC_TIMER;
    c.intr_type  = LEDC_INTR_DISABLE;
    c.duty       = backlight_duty(100);  /* full brightness at boot */
    c.hpoint     = 0;
    ESP_ERROR_CHECK(ledc_channel_config(&c));
}
#endif

extern "C" void display_set_backlight(int percent)
{
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    if (s_bl_pin < 0) return;
    s_bl_last_pct = percent;
#if defined(CONFIG_DRAFTLING_HAS_CH422G)
    backlight_ch422g_set(percent);
#else
    ESP_ERROR_CHECK(ledc_set_duty(BL_LEDC_MODE, BL_LEDC_CHANNEL,
                                  backlight_duty(percent)));
    ESP_ERROR_CHECK(ledc_update_duty(BL_LEDC_MODE, BL_LEDC_CHANNEL));
#endif
}

#if defined(CONFIG_DRAFTLING_RGB_PANEL_ST7701)
/* ---- ST7701 3-wire SPI init ----
 *
 * Each 9-bit word is a D/C bit (0 = command, 1 = parameter) followed
 * by the byte MSB first, sampled on the rising SCK edge (SPI mode 0).
 * The bus only carries a few hundred words once at boot, so it is
 * bit-banged rather than set up on an SPI peripheral. */
struct st7701_cmd_t {
    uint8_t  cmd;
    uint8_t  len;
    uint16_t delay_ms;
    uint8_t  data[16];
};

/* Manufacturer init sequence for this 320x820 panel (Waveshare demo,
 * via the projects named at the pin map above). 0xFF selects the
 * command bank (BK0 = 0x10, BK1 = 0x11, BK3 = 0x13, 0x00 = normal
 * command set). Ends with COLMOD = RGB565, MADCTL = 0, TE on and
 * DISPON. */
static const st7701_cmd_t st7701_init_cmds[] = {
    {0xFF, 5, 0, {0x77, 0x01, 0x00, 0x00, 0x13}},
    {0xEF, 1, 0, {0x08}},
    {0xFF, 5, 0, {0x77, 0x01, 0x00, 0x00, 0x10}},
    {0xC0, 2, 0, {0xE5, 0x02}},
    {0xC1, 2, 0, {0x15, 0x0A}},
    {0xC2, 2, 0, {0x07, 0x02}},
    {0xCC, 1, 0, {0x10}},
    {0xB0, 16, 0, {0x00, 0x08, 0x51, 0x0D, 0xCE, 0x06, 0x00, 0x08,
                   0x08, 0x24, 0x05, 0xD0, 0x0F, 0x6F, 0x36, 0x1F}},
    {0xB1, 16, 0, {0x00, 0x10, 0x4F, 0x0C, 0x11, 0x05, 0x00, 0x07,
                   0x07, 0x18, 0x02, 0xD3, 0x11, 0x6E, 0x34, 0x1F}},
    {0xFF, 5, 0, {0x77, 0x01, 0x00, 0x00, 0x11}},
    {0xB0, 1, 0, {0x4D}},
    {0xB1, 1, 0, {0x37}},
    {0xB2, 1, 0, {0x87}},
    {0xB3, 1, 0, {0x80}},
    {0xB5, 1, 0, {0x4A}},
    {0xB7, 1, 0, {0x85}},
    {0xB8, 1, 0, {0x21}},
    {0xB9, 2, 0, {0x00, 0x13}},
    {0xC0, 1, 0, {0x09}},
    {0xC1, 1, 0, {0x78}},
    {0xC2, 1, 0, {0x78}},
    {0xD0, 1, 0, {0x88}},
    {0xE0, 3, 100, {0x80, 0x00, 0x02}},
    {0xE1, 11, 0, {0x0F, 0xA0, 0x00, 0x00, 0x10, 0xA0, 0x00, 0x00,
                   0x00, 0x60, 0x60}},
    {0xE2, 13, 0, {0x30, 0x30, 0x60, 0x60, 0x45, 0xA0, 0x00, 0x00,
                   0x46, 0xA0, 0x00, 0x00, 0x00}},
    {0xE3, 4, 0, {0x00, 0x00, 0x33, 0x33}},
    {0xE4, 2, 0, {0x44, 0x44}},
    {0xE5, 16, 0, {0x0F, 0x4A, 0xA0, 0xA0, 0x11, 0x4A, 0xA0, 0xA0,
                   0x13, 0x4A, 0xA0, 0xA0, 0x15, 0x4A, 0xA0, 0xA0}},
    {0xE6, 4, 0, {0x00, 0x00, 0x33, 0x33}},
    {0xE7, 2, 0, {0x44, 0x44}},
    {0xE8, 16, 0, {0x10, 0x4A, 0xA0, 0xA0, 0x12, 0x4A, 0xA0, 0xA0,
                   0x14, 0x4A, 0xA0, 0xA0, 0x16, 0x4A, 0xA0, 0xA0}},
    {0xEB, 7, 0, {0x02, 0x00, 0x4E, 0x4E, 0xEE, 0x44, 0x00}},
    {0xED, 16, 0, {0xFF, 0xFF, 0x04, 0x56, 0x72, 0xFF, 0xFF, 0xFF,
                   0xFF, 0xFF, 0xFF, 0x27, 0x65, 0x40, 0xFF, 0xFF}},
    {0xEF, 6, 0, {0x08, 0x08, 0x08, 0x40, 0x3F, 0x64}},
    {0xFF, 5, 0, {0x77, 0x01, 0x00, 0x00, 0x13}},
    {0xE8, 2, 0, {0x00, 0x0E}},
    {0xFF, 5, 0, {0x77, 0x01, 0x00, 0x00, 0x00}},
    {0x11, 0, 120, {0}},                        /* SLPOUT */
    {0xFF, 5, 0, {0x77, 0x01, 0x00, 0x00, 0x13}},
    {0xE8, 2, 10, {0x00, 0x0C}},
    {0xE8, 2, 0, {0x00, 0x00}},
    {0xFF, 5, 0, {0x77, 0x01, 0x00, 0x00, 0x00}},
    {0x3A, 1, 0, {0x55}},                       /* COLMOD: RGB565 */
    {0x36, 1, 0, {0x00}},                       /* MADCTL */
    {0x35, 1, 0, {0x00}},                       /* TEON */
    {0x29, 0, 20, {0}},                         /* DISPON */
};

static void st7701_write9(bool is_data, uint8_t byte)
{
    uint16_t word = (uint16_t)((is_data ? 0x100 : 0) | byte);
    for (int bit = 8; bit >= 0; bit--) {
        gpio_set_level((gpio_num_t)ST7701_SPI_SCK_GPIO, 0);
        gpio_set_level((gpio_num_t)ST7701_SPI_SDA_GPIO, (word >> bit) & 1);
        esp_rom_delay_us(1);
        gpio_set_level((gpio_num_t)ST7701_SPI_SCK_GPIO, 1);
        esp_rom_delay_us(1);
    }
}

static void st7701_send_init(void)
{
    gpio_config_t io = {};
    io.pin_bit_mask = (1ULL << ST7701_SPI_CS_GPIO) |
                      (1ULL << ST7701_SPI_SCK_GPIO) |
                      (1ULL << ST7701_SPI_SDA_GPIO) |
                      (1ULL << RGB_RST_GPIO);
    io.mode = GPIO_MODE_OUTPUT;
    ESP_ERROR_CHECK(gpio_config(&io));
    gpio_set_level((gpio_num_t)ST7701_SPI_CS_GPIO, 1);
    gpio_set_level((gpio_num_t)ST7701_SPI_SCK_GPIO, 0);

    /* Hardware reset (the RGB panel API has no reset line). */
    gpio_set_level((gpio_num_t)RGB_RST_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level((gpio_num_t)RGB_RST_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(120));

    for (size_t i = 0; i < sizeof(st7701_init_cmds) / sizeof(st7701_init_cmds[0]); i++) {
        const st7701_cmd_t *c = &st7701_init_cmds[i];
        gpio_set_level((gpio_num_t)ST7701_SPI_CS_GPIO, 0);
        st7701_write9(false, c->cmd);
        for (int k = 0; k < c->len; k++) st7701_write9(true, c->data[k]);
        gpio_set_level((gpio_num_t)ST7701_SPI_CS_GPIO, 1);
        if (c->delay_ms) vTaskDelay(pdMS_TO_TICKS(c->delay_ms));
    }

    /* Hand CS (BOOT button), SCK and SDA (SD card CMD / CLK) back.
     * gpio_reset_pin() leaves each as an input with its pull-up on,
     * which keeps the panel's CS deasserted. */
    gpio_reset_pin((gpio_num_t)ST7701_SPI_CS_GPIO);
    gpio_reset_pin((gpio_num_t)ST7701_SPI_SCK_GPIO);
    gpio_reset_pin((gpio_num_t)ST7701_SPI_SDA_GPIO);
}
#endif

/* ---------------- Public API ---------------- */

extern "C" void display_init(int /*pin_a*/, int /*pin_b*/, int /*pin_c*/,
                             int /*pin_d*/, int /*pin_e*/, int /*pin_f*/,
                             int width, int height)
{
    s_width  = width;
    s_height = height;

#if defined(CONFIG_DRAFTLING_HAS_CH422G)
    /* Pulse the LCD reset line through the CH422G before touching the
     * RGB peripheral. main.cpp has already called ch422g_init() on
     * the shared I2C bus by the time this runs. esp_lcd_panel_reset()
     * a few lines below is a no-op for the RGB panel type (there is
     * no reset_gpio_num field in esp_lcd_rgb_panel_config_t), so this
     * is the only reset the ST7262 bridge gets. */
    ch422g_set_pin(RGB_CH422G_RST_EXIO, false);
    vTaskDelay(pdMS_TO_TICKS(10));
    ch422g_set_pin(RGB_CH422G_RST_EXIO, true);
    vTaskDelay(pdMS_TO_TICKS(100));
    s_bl_pin = RGB_CH422G_BL_EXIO;   /* marks "backlight available" below; not a real GPIO */
#else
    s_bl_pin = RGB_BL_GPIO;
    backlight_pwm_init(s_bl_pin);
#endif
#if defined(CONFIG_DRAFTLING_RGB_PANEL_ST7701)
    st7701_send_init();
#endif

    esp_lcd_rgb_panel_config_t cfg = {};
    cfg.clk_src   = LCD_CLK_SRC_DEFAULT;
    cfg.timings.pclk_hz           = RGB_PCLK_HZ;
    cfg.timings.h_res             = s_width;
    cfg.timings.v_res             = s_height;
    cfg.timings.hsync_pulse_width = RGB_HSYNC_PULSE;
    cfg.timings.hsync_back_porch  = RGB_HSYNC_BACK;
    cfg.timings.hsync_front_porch = RGB_HSYNC_FRONT;
    cfg.timings.vsync_pulse_width = RGB_VSYNC_PULSE;
    cfg.timings.vsync_back_porch  = RGB_VSYNC_BACK;
    cfg.timings.vsync_front_porch = RGB_VSYNC_FRONT;
    cfg.timings.flags.pclk_active_neg = RGB_PCLK_ACTIVE_NEG;
    cfg.timings.flags.pclk_idle_high  = RGB_PCLK_IDLE_HIGH;
    cfg.data_width   = 16;
    /* esp_lcd_rgb_panel_config_t dropped the old bits_per_pixel field;
     * pixel depth is now derived from in_color_format (out_color_format
     * defaults to in_color_format when left at 0). */
    cfg.in_color_format = LCD_COLOR_FMT_RGB565;
    cfg.num_fbs      = 1;
    /* Bounce buffer of up to 16 lines. esp_lcd_new_rgb_panel() requires
     * the frame buffer to be a whole multiple of it, so use the largest
     * line count that divides the panel height (16 on the 480-line
     * panels, 10 on the 820-line ST7701 panel). */
    int bounce_lines = 16;
    while (s_height % bounce_lines != 0) bounce_lines--;
    cfg.bounce_buffer_size_px = s_width * bounce_lines;
    cfg.flags.fb_in_psram = 1;
    cfg.hsync_gpio_num = (gpio_num_t)RGB_HSYNC_GPIO;
    cfg.vsync_gpio_num = (gpio_num_t)RGB_VSYNC_GPIO;
    cfg.de_gpio_num    = (gpio_num_t)RGB_DE_GPIO;
    cfg.pclk_gpio_num  = (gpio_num_t)RGB_PCLK_GPIO;
    cfg.disp_gpio_num  = (gpio_num_t)RGB_DISP_GPIO;
    for (int i = 0; i < 16; i++) cfg.data_gpio_nums[i] = (gpio_num_t)kDataGpios[i];

    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&cfg, &s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));

    /* Host framebuffer (RGB565) in PSRAM. */
    s_fb_pixels = (size_t)s_width * s_height;
    s_fb = (uint16_t *)heap_caps_malloc(s_fb_pixels * sizeof(uint16_t),
                                        MALLOC_CAP_SPIRAM);
    assert(s_fb);
    memset(s_fb, 0, s_fb_pixels * sizeof(uint16_t));

    display_clear(0x00);
    display_full_refresh();

    ESP_LOGI(TAG, "RGB panel %dx%d initialized", s_width, s_height);
}

extern "C" void display_clear(uint8_t color)
{
    /* 1-bpp legacy API: 0xFF = white, 0x00 = black. */
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
    int px = ((int)x + display_margin_left()) * RGB_SCALE;
    int py = ((int)y + display_margin_top())  * RGB_SCALE;
    if (px >= s_width || py >= s_height) return;
    uint16_t v = (color == 0) ? 0x0000 : 0xFFFF;
    int x_end = px + RGB_SCALE; if (x_end > s_width)  x_end = s_width;
    int y_end = py + RGB_SCALE; if (y_end > s_height) y_end = s_height;
    for (int yy = py; yy < y_end; yy++) {
        uint16_t *row = s_fb + (size_t)yy * s_width + px;
        for (int xx = px; xx < x_end; xx++) *row++ = v;
    }
    int x2 = x_end - 1;
    int y2 = y_end - 1;
    if (s_dirty_x1 < 0) {
        s_dirty_x1 = px;  s_dirty_y1 = py;
        s_dirty_x2 = x2;  s_dirty_y2 = y2;
    } else {
        if (px < s_dirty_x1) s_dirty_x1 = px;
        if (x2 > s_dirty_x2) s_dirty_x2 = x2;
        if (py < s_dirty_y1) s_dirty_y1 = py;
        if (y2 > s_dirty_y2) s_dirty_y2 = y2;
    }
}

extern "C" bool display_push_rgb565(int x, int y, int w, int h,
                                    const void *color_map)
{
    if (w <= 0 || h <= 0) return true;
    int px = (x + display_margin_left()) * RGB_SCALE;
    int py = (y + display_margin_top())  * RGB_SCALE;
    int pw = w * RGB_SCALE;
    int ph = h * RGB_SCALE;
    if (px < 0 || py < 0) return true;
    int x2 = px + pw - 1;
    int y2 = py + ph - 1;
    if (x2 >= s_width)  x2 = s_width  - 1;
    if (y2 >= s_height) y2 = s_height - 1;
    int eff_w = x2 - px + 1;
    int eff_h = y2 - py + 1;
    if (eff_w <= 0 || eff_h <= 0) return true;

    const uint16_t *src = (const uint16_t *)color_map;
    if (RGB_SCALE == 1) {
        for (int row = 0; row < eff_h; row++) {
            uint16_t *dst = s_fb + (size_t)(py + row) * s_width + px;
            memcpy(dst, src, eff_w * sizeof(uint16_t));
            src += w;
        }
    } else {
        for (int sy = 0; sy < h; sy++) {
            int dy0 = py + sy * RGB_SCALE;
            if (dy0 >= s_height) break;
            uint16_t *drow0 = s_fb + (size_t)dy0 * s_width + px;
            uint16_t *p = drow0;
            int written = 0;
            for (int sx = 0; sx < w && written < eff_w; sx++) {
                uint16_t v = src[(size_t)sy * w + sx];
                for (int k = 0; k < RGB_SCALE && written < eff_w; k++) {
                    *p++ = v;
                    written++;
                }
            }
            for (int k = 1; k < RGB_SCALE; k++) {
                int dy = dy0 + k;
                if (dy >= s_height) break;
                memcpy(s_fb + (size_t)dy * s_width + px, drow0,
                       (size_t)eff_w * sizeof(uint16_t));
            }
        }
    }

    if (s_dirty_x1 < 0) {
        s_dirty_x1 = px;  s_dirty_y1 = py;
        s_dirty_x2 = x2;  s_dirty_y2 = y2;
    } else {
        if (px < s_dirty_x1) s_dirty_x1 = px;
        if (x2 > s_dirty_x2) s_dirty_x2 = x2;
        if (py < s_dirty_y1) s_dirty_y1 = py;
        if (y2 > s_dirty_y2) s_dirty_y2 = y2;
    }
    return true;
}

extern "C" void display_set_partial_clip(int /*x*/, int /*y*/,
                                         int /*w*/, int /*h*/)
{
    /* No-op: the RGB backend always pushes the full dirty bbox. */
}

extern "C" void display_flush(void)
{
    if (s_dirty_x1 < 0) return;

    int x1 = s_dirty_x1, y1 = s_dirty_y1;
    int x2 = s_dirty_x2, y2 = s_dirty_y2;
    s_dirty_x1 = s_dirty_y1 = s_dirty_x2 = s_dirty_y2 = -1;

    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= s_width)  x2 = s_width  - 1;
    if (y2 >= s_height) y2 = s_height - 1;
    if (x1 > x2 || y1 > y2) return;

    /* esp_lcd_panel_draw_bitmap reads a tightly-packed (w x h)
     * rectangle. Our framebuffer rows are s_width wide, so stream
     * the rectangle row-by-row through a small scratch buffer. */
    int w = x2 - x1 + 1;
    int h = y2 - y1 + 1;

    if (x1 == 0 && w == s_width) {
        /* Full-width rectangle: rows are already contiguous in the
         * framebuffer, so one draw_bitmap call covers it. */
        esp_lcd_panel_draw_bitmap(s_panel, x1, y1, x2 + 1, y2 + 1,
                                  s_fb + (size_t)y1 * s_width);
        return;
    }

    /* Partial-width rectangle: copy into a contiguous scratch buffer. */
    uint16_t *scratch = (uint16_t *)heap_caps_malloc(
        (size_t)w * h * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (!scratch) {
        /* Fall back to a full-frame push if the scratch alloc fails. */
        esp_lcd_panel_draw_bitmap(s_panel, 0, 0, s_width, s_height, s_fb);
        return;
    }
    for (int row = 0; row < h; row++) {
        memcpy(scratch + (size_t)row * w,
               s_fb + (size_t)(y1 + row) * s_width + x1,
               (size_t)w * sizeof(uint16_t));
    }
    esp_lcd_panel_draw_bitmap(s_panel, x1, y1, x2 + 1, y2 + 1, scratch);
    heap_caps_free(scratch);
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
    s_dirty_x1 = 0;
    s_dirty_y1 = 0;
    s_dirty_x2 = s_width  - 1;
    s_dirty_y2 = s_height - 1;
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
    if (s_bl_pin >= 0) display_set_backlight(0);
}

extern "C" void display_wake(void)
{
    if (s_bl_pin >= 0) display_set_backlight(s_bl_last_pct);
    display_full_refresh();
}

extern "C" void display_deep_sleep_prepare(void)
{
    if (s_bl_pin < 0) return;
#if defined(CONFIG_DRAFTLING_HAS_CH422G)
    backlight_ch422g_set(0);
#elif RGB_BL_ACTIVE_LOW
    /* LEDC is powered down in deep sleep, and an undriven active-low
     * backlight pin would light the panel. Park it high and latch it
     * there (backlight_pwm_init() releases the hold on the next boot). */
    ledc_stop(BL_LEDC_MODE, BL_LEDC_CHANNEL, 1);
    gpio_set_direction((gpio_num_t)s_bl_pin, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)s_bl_pin, 1);
    gpio_hold_en((gpio_num_t)s_bl_pin);
    gpio_deep_sleep_hold_en();
#else
    ledc_set_duty(BL_LEDC_MODE, BL_LEDC_CHANNEL, 0);
    ledc_update_duty(BL_LEDC_MODE, BL_LEDC_CHANNEL);
#endif
}

extern "C" void display_set_shared_i2c_bus(void * /*bus_handle*/)
{
    /* No-op: the RGB backend does not use I2C. */
}

#endif /* CONFIG_DRAFTLING_DISPLAY_RGB */
