#include "sdkconfig.h"
#if defined(CONFIG_DRAFTLING_DISPLAY_ST77922)

/*
 * ST77922 QSPI color-LCD backend for the Freenove FNK0104N (3.5"
 * 320x480 native panel, rendered landscape at 480x320).
 *
 * This backend is built on the official `espressif/esp_lcd_st77922`
 * managed component (from espressif/esp-iot-solution). The same
 * component and the same byte-identical vendor init table are also
 * used by the xiaozhi-esp32 board file
 * main/boards/lcdwiki-es3c35p/lcdwiki-es3c35p.cc, which drives a
 * different vendor's board (LCDWiki ES3C35P) built around the same
 * LCD module/controller. Two earlier revisions of this
 * file hand-rolled the QSPI protocol directly against
 * spi_device_polling_transmit(), first porting Freenove's separate
 * Arduino TFT_eSPI vendor driver (Libraries/FNK0104N/
 * TFT_eSPI_v2.5.43.zip -> TFT_eSPI/ST77922.cpp) and then patching the
 * RAMWR/RAMWRC command mismatch found by diffing against
 * esp_lcd_st77922_general.c -- but the panel still showed the
 * previous firmware's leftover image after a fresh flash + reset,
 * because that hand-rolled init path never sent a reset of any kind
 * (no discrete RST pin, no SWRESET command either). The official
 * component's esp_lcd_panel_reset() sends a proper software reset
 * (SWRESET, 0x01, + 120 ms delay) whenever reset_gpio_num < 0, fixing
 * that class of bug outright, so this file now delegates panel
 * bring-up (reset, vendor init table, CASET/RASET/RAMWR framing, CS
 * management) to the component instead of re-implementing it.
 *
 * The vendor init command table below is unchanged from the earlier
 * revisions (still ported from Freenove's driver and byte-identical
 * to the table used by the lcdwiki-es3c35p board file above), just
 * retyped as `st77922_lcd_init_cmd_t` to match the component's
 * `st77922_vendor_config_t::init_cmds` field.
 *
 * Unlike AXS15231B, the ST77922 does not offer a MADCTL row/column
 * swap for this panel's landscape orientation
 * (`panel_st77922_swap_xy()` in the component unconditionally returns
 * ESP_ERR_NOT_SUPPORTED). Freenove's own reference driver keeps
 * MADCTL at its portrait value and instead performs a software pixel
 * transpose in its ST77922::Fill_Colors_Landscape() helper; this file
 * ports that same transpose (logical (lx, ly) -> physical
 * (FNK_N_NATIVE_WIDTH - ly - h, lx), see flush_rect() below) so the
 * draw_bitmap window always addresses the panel's native portrait
 * frame, while
 * display_clear/display_set_pixel/display_push_rgb565 continue to
 * operate in logical (already-landscape) coordinates like every other
 * backend.
 *
 * Like AXS15231B, the ST77922 expects pixel data on the wire in
 * big-endian (MSB-first) RGB565 byte order. Freenove's own reference
 * LVGL integration for this exact board
 * (Tutorial_No_Touch/Sketches/Sketch_11.1_LVGL/display.h) sets
 * `#define LV_COLOR_16_SWAP 1` for FNK0104N_3P5_320x480_ST77922,
 * which makes LVGL byte-swap every pixel before it reaches the flush
 * callback. We do not use LV_COLOR_16_SWAP (LVGL's buffers stay
 * native little-endian, matching every other backend), so the
 * transpose step below must perform the equivalent byte swap itself
 * when copying into s_tx_buf, or every pixel is sent MSB/LSB
 * reversed -- which silently corrupts colours enough that the
 * editor's mostly-dark-on-light text becomes invisible. The official
 * component's own tx_color()/draw_bitmap() do not byte-swap either,
 * so this manual step is still required.
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
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_st77922.h>

#include "display.h"

static const char *TAG = "DisplayST77922";

/* Pins, from Freenove's ST77922.h reference header / xiaozhi-esp32
 * board config.h (main/boards/freenove-esp32s3-display-3.5-lcd/
 * config.h), which agree exactly. */
#define FNK_N_CS_PIN     10
#define FNK_N_BL_PIN     41
#define FNK_N_SCLK_PIN   12
#define FNK_N_D0_PIN     11
#define FNK_N_D1_PIN     13
#define FNK_N_D2_PIN     14
#define FNK_N_D3_PIN     9
#define FNK_N_RST_PIN    (gpio_num_t)-1  /* No discrete RST pin; the component issues
                              * a software reset (SWRESET) instead -- see
                              * esp_lcd_panel_reset() in display_init(). */

#define FNK_N_SPI_HOST       SPI2_HOST
/* 20 MHz QSPI. Freenove's own confirmed-working xiaozhi-esp32 board
 * file (DISPLAY_SPI_SCLK_HZ in
 * freenove-esp32s3-display-3.5-lcd/config.h) uses 40 MHz, and this
 * driver matched that for a while, but ESP-IDF v6's SPI driver reads
 * the PSRAM-resident s_tx_buf color buffer over DMA (psram_dma_direct)
 * fast enough to underrun its own FIFO at 40 MHz: the whole-panel
 * flush issued right after LVGL init reliably aborted with
 * "DMA TX underflow detected" / "recycle spi transactions failed" out
 * of panel_st77922_draw_bitmap(), taking down the app via
 * ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(...)). The same failure
 * mode (and fix) is documented for another PSRAM-backed QSPI panel on
 * ESP-IDF v6 in the sh8601-based esp32_s3_touch_amoled_2_06 board
 * patch notes, which also had to drop from the vendor's 40 MHz default
 * to 20 MHz. 20 MHz is still far more bandwidth than this panel's
 * partial-rect UI updates need. */
#define FNK_N_SPI_CLOCK_HZ   (20 * 1000 * 1000)

/* Native panel resolution (portrait). The logical (landscape) frame
 * used by display_clear/set_pixel/push_rgb565 is
 * FNK_N_NATIVE_HEIGHT x FNK_N_NATIVE_WIDTH (480 x 320). */
#define FNK_N_NATIVE_WIDTH   320
#define FNK_N_NATIVE_HEIGHT  480

/* TODO(temporary diagnostic, remove once root cause confirmed): see the
 * "still, all pixels remain black" bug report. Number of post-boot
 * flush_rect() calls to log (logical + mapped physical rectangle) -- see
 * the usage in flush_rect() below. The #warning below is intentional: it
 * flags this diagnostic in every build log so it is not accidentally
 * left in once the bug is resolved. */
#warning "Temporary flush_rect() diagnostic logging is enabled (display_st77922.cpp); remove once the FNK0104N blank-screen bug is resolved"
#define FLUSH_RECT_LOG_LIMIT 12

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

static const st77922_lcd_init_cmd_t s_init_seq[] = {
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

static esp_lcd_panel_io_handle_t s_io    = NULL;
static esp_lcd_panel_handle_t    s_panel = NULL;

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

/* Alignment for s_tx_buf so the panel IO's psram_dma_direct fast path
 * (see display_init() below) is always taken for the whole buffer
 * instead of falling back to an internal-DRAM bounce copy for a
 * misaligned chunk -- same rationale as display_ili9341.cpp's
 * DMA_ALIGN_BYTES. */
#define DMA_ALIGN_BYTES 64

static int s_dirty_x1 = -1, s_dirty_y1 = -1, s_dirty_x2 = -1, s_dirty_y2 = -1;
static int s_clip_x = 0, s_clip_y = 0, s_clip_w = 0, s_clip_h = 0;

/* Last user-requested backlight percent, cached so display_sleep() /
 * display_wake() can restore the brightness after blanking the panel
 * (mirrors the same cache in display_axs15231b.cpp). Initialised to
 * 100 to match the panel's initial full-brightness state after
 * display_init(). */
static int s_bl_last_pct = 100;

static bool s_panel_asleep = false;

/* Send a raw command (with optional parameter bytes) directly to the
 * panel IO, bypassing the panel object -- used for SLPIN/SLPOUT,
 * which esp_lcd_panel_t has no generic entry point for (only
 * disp_on_off() for DISPOFF/DISPON is exposed).
 *
 * Encodes the command exactly like the espressif/esp_lcd_st77922
 * component's own internal tx_param() does when
 * flags.use_qspi_interface is set: the panel IO was created with
 * lcd_cmd_bits = 32 (see ST77922_PANEL_IO_QSPI_CONFIG), so the 32-bit
 * "command" esp_lcd_panel_io_tx_param() receives packs the QSPI
 * write-command opcode (0x02, matching LCD_OPCODE_WRITE_CMD in the
 * component's private st77922_interface.h) into the top byte and the
 * actual 1-byte MIPI DCS command into bits [15:8]. */
static void tx_param_qspi(uint8_t cmd, const void *data, size_t len)
{
    uint32_t lcd_cmd = ((uint32_t)cmd << 8) | (0x02u << 24);
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(s_io, (int)lcd_cmd, data, len));
}

/* Transpose a logical (already-landscape) rectangle of the
 * framebuffer into s_tx_buf in the panel's native portrait
 * coordinate space, following Freenove's own ST77922::
 * Fill_Colors_Landscape() helper (ST77922.cpp in the FNK0104N vendor
 * driver bundle) -- the function that actually implements landscape
 * rendering on top of the native-portrait (rotation == 0) panel, as
 * opposed to Fill_Colors()'s generic rotation==1/3 branch (used only
 * when Set_Rotation(1) has been called, which also hard-refuses any
 * partial-rect write that is not the full frame -- not applicable
 * here since this backend never changes MADCTL away from 0).
 * Fill_Colors_Landscape() computes native_sx as
 * `LCD_WIDTH - (sy + h)` (LCD_WIDTH == FNK_N_NATIVE_WIDTH, sy/h are
 * the landscape-space y-origin/height), i.e. physical(x, y) =
 * (FNK_N_NATIVE_WIDTH - ly - h, lx) -- a genuine 90-degree-rotation
 * mapping where the landscape y-axis inverts onto the native x-axis
 * (column) while the landscape x-axis maps straight onto the native
 * y-axis (row), matching the per-pixel transpose below
 * (`col * h + (h - 1 - row)`, identical to Fill_Colors_Landscape's
 * `col * h + (h - row - 1)`).
 *
 * Two earlier revisions of this file got the window origin wrong:
 * one used physical(x, y) = (FNK_N_NATIVE_WIDTH - 1 - ly, lx), which
 * only matches the true mapping for a full-frame flush (ly == 0,
 * h == FNK_N_NATIVE_WIDTH); a later revision removed the complement
 * entirely (physical(x, y) = (ly, lx)) after comparing against
 * Fill_Colors()'s unrelated rotation==1 branch (from
 * community/community-mirrored copies of the vendor driver, not the
 * landscape-specific helper) -- both of those also only agree with
 * the correct mapping for a full-frame flush (ly == 0, h ==
 * FNK_N_NATIVE_WIDTH), which is why the initial post-boot
 * full-screen clear appeared to work while every subsequent
 * partial-rect flush (i.e. every actual editor UI update -- text,
 * cursor, title bar) landed at the wrong CASET column and never
 * became visible, leaving the panel looking permanently blank after
 * the first frame. */
static void flush_rect(int lx, int ly, int w, int h)
{
    int phys_sx = FNK_N_NATIVE_WIDTH - ly - h;
    int phys_sy = lx;
    int phys_w  = h;
    int phys_h  = w;

    /* TODO(temporary diagnostic, remove once root cause confirmed): see
     * the "still, all pixels remain black" bug report. Logs the logical
     * and mapped physical rectangle for the first
     * FLUSH_RECT_LOG_LIMIT post-boot flush_rect() calls, to confirm
     * whether display_push_rgb565() / display_flush() are even reaching
     * this function with sane, non-degenerate rectangles once the editor
     * UI starts drawing (step 3 of the diagnostic plan). */
    static int s_flush_log_count = 0;
    if (s_flush_log_count < FLUSH_RECT_LOG_LIMIT) {
        ESP_LOGI(TAG,
                "flush_rect #%d: logical(x=%d,y=%d,w=%d,h=%d) -> "
                "physical(x=%d,y=%d,w=%d,h=%d)",
                s_flush_log_count, lx, ly, w, h, phys_sx, phys_sy, phys_w, phys_h);
        s_flush_log_count++;
    }

    for (int row = 0; row < h; row++) {
        const uint16_t *src = s_fb + (size_t)(ly + row) * s_width + lx;
        for (int col = 0; col < w; col++) {
            /* Byte-swap each pixel: the panel wants big-endian RGB565
             * on the wire (see the LV_COLOR_16_SWAP note above), but
             * s_fb holds native little-endian uint16_t values. */
            uint16_t px = src[col];
            s_tx_buf[col * phys_w + (phys_w - 1 - row)] =
                (uint16_t)((px << 8) | (px >> 8));
        }
    }

    /* esp_lcd_panel_draw_bitmap() takes an exclusive end coordinate
     * and issues CASET/RASET + RAMWR internally (via
     * panel_st77922_draw_bitmap() in the espressif/esp_lcd_st77922
     * component), including re-sending the QSPI cmd/addr header for
     * every DMA-sized sub-transaction of a large burst. */
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(s_panel, phys_sx, phys_sy,
                                              phys_sx + phys_w, phys_sy + phys_h,
                                              s_tx_buf));
}

extern "C" void display_init(int /*pin_a*/, int /*pin_b*/, int /*pin_c*/,
                             int /*pin_d*/, int /*pin_e*/, int /*pin_f*/,
                             int width, int height)
{
    s_width  = width;
    s_height = height;

    gpio_config_t bl_cfg = {};
    bl_cfg.intr_type    = GPIO_INTR_DISABLE;
    bl_cfg.mode         = GPIO_MODE_OUTPUT;
    bl_cfg.pin_bit_mask = (1ULL << FNK_N_BL_PIN);
    ESP_ERROR_CHECK(gpio_config(&bl_cfg));
    gpio_set_level((gpio_num_t)FNK_N_BL_PIN, 0);

    s_fb_pixels = (size_t)width * height;

    spi_bus_config_t bus_cfg = {};
    bus_cfg.data0_io_num = FNK_N_D0_PIN;
    bus_cfg.data1_io_num = FNK_N_D1_PIN;
    bus_cfg.sclk_io_num  = FNK_N_SCLK_PIN;
    bus_cfg.data2_io_num = FNK_N_D2_PIN;
    bus_cfg.data3_io_num = FNK_N_D3_PIN;
    /* Explicitly mark the octal-mode data4..data7 lines as unused.
     * spi_bus_config_t's data4_io_num..data7_io_num fields are plain
     * ints (not unioned with anything we set above), so the
     * zero-initialised `bus_cfg = {}` above leaves them at 0 (GPIO0)
     * rather than -1. ESP-IDF's spicommon_bus_initialize_io() only
     * skips these fields when `!(flags & SPICOMMON_BUSFLAG_OCTAL)`,
     * but that test is a bitwise AND against a compound flag
     * (OCTAL == QUAD | IO4_IO7), so it is still true whenever QUAD's
     * bits are set -- i.e. it never actually skips them for a
     * quad-mode bus like this one. The iomux-pin reservation loop
     * then walks into data4_io_num..data7_io_num and tries to
     * reserve GPIO0 four times, logging "GPIO 0 is conflict with
     * others and be overwritten" (matches the exact warning seen in
     * the field on this board). Setting them to -1 makes
     * GPIO_IS_VALID_GPIO() reject them so they are skipped, same as
     * the AXS15231B/ILI9341 backends' bus configs implicitly get via
     * their smaller struct-literal initialisers. */
    bus_cfg.data4_io_num = -1;
    bus_cfg.data5_io_num = -1;
    bus_cfg.data6_io_num = -1;
    bus_cfg.data7_io_num = -1;
    bus_cfg.max_transfer_sz = (int)(s_fb_pixels * sizeof(uint16_t));
    bus_cfg.flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_IOMUX_PINS | SPICOMMON_BUSFLAG_QUAD;
    ESP_ERROR_CHECK(spi_bus_initialize(FNK_N_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    /* Field-for-field equivalent of the component's own
     * ST77922_PANEL_IO_QSPI_CONFIG() macro, expanded by hand instead
     * of invoked directly: the macro's braced-init-list assigns the
     * plain-int literal `-1` to the `gpio_num_t dc_gpio_num` field and
     * leaves `cs_ena_pretrans` / `cs_ena_posttrans` out of the list
     * entirely, which is legal C99 designated-initializer usage but
     * fails to compile as C++ aggregate initialization in this .cpp
     * file (-fpermissive int->enum conversion error, plus
     * -Werror=missing-field-initializers). Zero-initialising the
     * struct first and then assigning every field the macro sets
     * keeps the exact same configuration while staying valid C++. */
    esp_lcd_panel_io_spi_config_t io_cfg = {};
    io_cfg.cs_gpio_num = (gpio_num_t)FNK_N_CS_PIN;
    io_cfg.dc_gpio_num = (gpio_num_t)-1;
    io_cfg.spi_mode = 0;
    io_cfg.pclk_hz = FNK_N_SPI_CLOCK_HZ;
    io_cfg.trans_queue_depth = 10;
    io_cfg.on_color_trans_done = nullptr;
    io_cfg.user_ctx = nullptr;
    io_cfg.lcd_cmd_bits = 32;
    io_cfg.lcd_param_bits = 8;
    io_cfg.flags.quad_mode = true;
    /* See the DMA_ALIGN_BYTES comment above s_tx_buf's declaration:
     * without this flag, esp_lcd_panel_io_tx_color() bounces every
     * >32 KB chunk of a PSRAM color buffer through a freshly malloc'd
     * internal-DRAM buffer, which can silently fail (and drop the
     * rest of the flush) once internal SRAM is under pressure. */
    io_cfg.flags.psram_dma_direct = 1;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)FNK_N_SPI_HOST, &io_cfg, &s_io));

    s_fb = (uint16_t *)heap_caps_malloc(s_fb_pixels * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    size_t tx_buf_size = (s_fb_pixels * sizeof(uint16_t) + DMA_ALIGN_BYTES - 1) &
                         ~(size_t)(DMA_ALIGN_BYTES - 1);
    s_tx_buf = (uint16_t *)heap_caps_aligned_alloc(DMA_ALIGN_BYTES, tx_buf_size, MALLOC_CAP_SPIRAM);
    assert(s_fb && s_tx_buf);
    memset(s_fb, 0, s_fb_pixels * sizeof(uint16_t));

    st77922_vendor_config_t vendor_cfg = {};
    vendor_cfg.init_cmds = s_init_seq;
    vendor_cfg.init_cmds_size = sizeof(s_init_seq) / sizeof(s_init_seq[0]);
    vendor_cfg.flags.use_qspi_interface = 1;

    esp_lcd_panel_dev_config_t panel_cfg = {};
    panel_cfg.reset_gpio_num = FNK_N_RST_PIN;
    /* Matches the confirmed-working xiaozhi-esp32 lcdwiki-es3c35p
     * board's DISPLAY_RGB_ORDER (LCD_RGB_ELEMENT_ORDER_BGR). This has
     * no effect on the panel's actual MADCTL register in practice --
     * the component sends its own MADCTL derived from this field
     * *before* running init_cmds, but our vendor table's own 0x36
     * entry (d_36 below) is sent afterwards and wins (hence the
     * "36h command has been used and will be overwritten" warning
     * logged at boot) -- but matching the reference removes any doubt
     * and keeps this field meaningful if the vendor table ever drops
     * its own MADCTL entry. */
    panel_cfg.rgb_ele_order  = LCD_RGB_ELEMENT_ORDER_BGR;
    panel_cfg.bits_per_pixel = 16;
    panel_cfg.vendor_config  = &vendor_cfg;
    ESP_ERROR_CHECK(esp_lcd_new_panel_st77922(s_io, &panel_cfg, &s_panel));

    /* With FNK_N_RST_PIN < 0, esp_lcd_panel_reset() sends a proper
     * software reset (SWRESET, 0x01) + 120 ms delay instead of toggling
     * a GPIO. A prior revision of this file never sent any reset at
     * all (see the removed comment this replaced), which meant that
     * after reflashing new firmware the panel's GRAM could retain
     * whatever a *previous* firmware had last written to it -- the
     * user-reported symptom of the display still showing the picture
     * from Freenove's own xiaozhi-esp32 firmware after flashing and
     * resetting Draftling. */
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));

    /* The confirmed-working xiaozhi-esp32 lcdwiki-es3c35p board
     * explicitly calls these three functions right after
     * esp_lcd_panel_init() to pin down the panel's final display
     * state. Our vendor init table (s_init_seq above) already sends
     * equivalent DCS commands (0x21 INVON, 0x29 DISPON) *inside* the
     * init sequence, but esp_lcd_panel_invert_color() /
     * esp_lcd_panel_mirror() / esp_lcd_panel_disp_on_off() are the
     * *last* word issued to the panel -- calling them here overrides
     * whatever the table left behind and guarantees our panel ends up
     * in exactly the same electrical state as the reference,
     * regardless of any future edits to s_init_seq. */
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(s_panel, false));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(s_panel, false, false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel, true));

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
     * rounder). The panel's window (CASET/RASET, programmed
     * internally by esp_lcd_panel_draw_bitmap()) is silently ignored
     * (or renders garbage) when the requested rectangle is not
     * aligned to a 4-pixel boundary, so without this the initial
     * full-screen
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
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel, false)); /* DISPOFF */
    tx_param_qspi(0x10, nullptr, 0);                            /* SLPIN */
}

extern "C" void display_wake(void)
{
    if (!s_panel_asleep) return;
    tx_param_qspi(0x11, nullptr, 0);                            /* SLPOUT */
    vTaskDelay(pdMS_TO_TICKS(120));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel, true));  /* DISPON */
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
