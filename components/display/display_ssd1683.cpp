#include "sdkconfig.h"
#if defined(CONFIG_DRAFTLING_DISPLAY_SSD1683)

/*
 * Elecrow CrowPanel ESP32-S3 5.79" E-Paper HMI Display driver.
 *
 * Hardware
 * --------
 * 792x272 black/white e-paper panel built from two SSD1683 driver
 * chips wired to the same SPI bus (shared CS/DC/RST/BUSY): one chip
 * ("slave") drives the physical left half (columns 0-399), the other
 * ("master") drives the right half (columns 392-791), with an
 * 8-pixel overlap at the seam for alignment. The two chips are
 * differentiated purely by their command set -- slave commands are
 * 0x9x/0xCx/0xAx, master commands are the SSD1683's normal 0x1x/0x2x/
 * 0x4x set -- so a single physical SPI transaction stream addresses
 * whichever chip the active command belongs to.
 *
 * All panel pins (SCK=12, MOSI=11, DC=46, CS=45, RST=47, BUSY=48) and
 * the panel power-enable pin (GPIO7, "IO7_LCD_3.3_CTL" on the vendor
 * schematic, must be driven HIGH before the panel responds) are
 * hard-coded below rather than carried in the board header: this
 * backend is used by exactly one board, matching the convention
 * already used by display_ili9341.cpp / display_xteink_epd.cpp.
 *
 * The master/slave RAM-window addressing math (including the master's
 * inverted X-address counting and the seam-alignment special case in
 * the partial-refresh byte range) originates from the community
 * ESPHome driver at github.com/samperk1/esphome-crowpanel-579, which
 * reverse-engineered and photograph-verified it against real CrowPanel
 * 5.79" hardware. This backend has since been extensively verified on
 * its own physical hardware too -- see HARDWARE.md and PR #47
 * (github.com/clackups/draftling/pull/47) for that investigation.
 *
 * Framebuffer layout
 * -------------------
 * s_disp_buf is a plain 1-bpp buffer, 99 bytes/row (792/8) x 272
 * rows, MSB-first, bit=1 -> white paper, bit=0 -> black ink (same
 * polarity convention as display_rlcd.cpp). Byte 49 of every row
 * (columns 392-399) is the seam: it is sent to *both* chips, which
 * is how the panel firmware aligns the two halves.
 *
 * Refresh policy
 * --------------
 * Every refresh (partial or full) rewrites the *entire* panel's New
 * and Old RAM on both chips, not just the dirty rectangle -- narrower
 * windowing left the untouched rest of the panel's RAM implicitly
 * relying on stale earlier writes, which on-hardware testing found
 * unreliable (see epd_update_partial()'s own comment). display_flush()
 * uses the fast OTP black/white LUT (waveform 0xFF) for ordinary
 * edits, promoting to the full OTP waveform (0xF7) every
 * CONFIG_DRAFTLING_EPD_FULL_REFRESH_INTERVAL refreshes or on request
 * (display_clear/display_full_refresh/display_request_full_refresh).
 * Both paths run their update twice, back-to-back: a single Master
 * Activation trigger on this panel can leave an incomplete pixel
 * transition regardless of waveform or RAM content, and immediately
 * repeating the same update reliably completes it (see the
 * CROWPANEL_PARTIAL_REFRESH comment near display_flush() for the full
 * story).
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

#include "display.h"

static const char *TAG = "DisplaySSD1683";

/* ---- Panel pins (this backend serves exactly one board) ---- */
#define EPD_SCK_PIN     12
#define EPD_MOSI_PIN    11
#define EPD_DC_PIN      46
#define EPD_CS_PIN      45
#define EPD_RST_PIN     47
#define EPD_BUSY_PIN    48
#define EPD_PWR_PIN     7   /* IO7_LCD_3.3_CTL, active HIGH */

#define EPD_SPI_HOST    SPI2_HOST

/* ---- Panel geometry ---- */
#define PANEL_W             792
#define PANEL_H             272
#define BYTES_PER_ROW       99   /* 792 / 8 */
#define BYTES_PER_HALF_ROW  50   /* slave: bytes 0-49, master: bytes 49-98 */
#define HALF_BUF_LEN        (BYTES_PER_HALF_ROW * PANEL_H)  /* 13600 */
#define FULL_BUF_LEN         (BYTES_PER_ROW * PANEL_H)        /* 26928 */

/* esp_lcd_panel_io_tx_color() bounces a PSRAM source buffer through a
 * freshly malloc'd internal-DRAM copy unless io_cfg.flags.
 * psram_dma_direct is set *and* the buffer is DMA-alignment-friendly
 * -- see display_ili9341.cpp's DMA_ALIGN_BYTES comment for the full
 * story (a documented, previously-fixed bug in this exact codebase):
 * without it, that bounce allocation can silently fail under internal
 * SRAM pressure, and esp_lcd_panel_io_tx_color()'s return value going
 * unchecked means the rest of the buffer just never gets sent while
 * the surrounding command sequence proceeds as if nothing went wrong.
 * This backend never needed to care while every send_buffer() call
 * was a small, narrow-window write; now that every refresh sends the
 * full HALF_BUF_LEN (13600 bytes) to each chip, the same failure mode
 * applies here too. */
#define DMA_ALIGN_BYTES 64
#define HALF_BUF_ALLOC_LEN (((size_t)HALF_BUF_LEN + DMA_ALIGN_BYTES - 1) & \
                            ~(size_t)(DMA_ALIGN_BYTES - 1))

#ifdef CONFIG_DRAFTLING_EPD_FULL_REFRESH_INTERVAL
#define SSD1683_FULL_REFRESH_INTERVAL CONFIG_DRAFTLING_EPD_FULL_REFRESH_INTERVAL
#else
#define SSD1683_FULL_REFRESH_INTERVAL 30
#endif

static esp_lcd_panel_io_handle_t s_io_handle = NULL;
static uint8_t *s_disp_buf   = NULL;  /* FULL_BUF_LEN, PSRAM */
static uint8_t *s_pack_a     = NULL;  /* HALF_BUF_LEN scratch, PSRAM */
static uint8_t *s_pack_b     = NULL;  /* HALF_BUF_LEN scratch, PSRAM */
static bool     s_initialized = false;

/*
 * Dirty rectangles accumulated since the last display_flush(). LVGL's
 * PARTIAL render mode typically produces a couple of disjoint dirty
 * areas per keystroke on the editor screen (the title bar's line/col
 * indicator at the top of the panel, and the edited text line further
 * down) -- see flush_cb() in lvgl_port.cpp. Collapsing those into a
 * single unioned bounding box (the original approach here) meant every
 * keystroke's partial refresh window spanned the vertical gap between
 * them too, which on this panel's 272 px height can be most of the
 * screen. The upstream community driver this backend is ported from
 * (github.com/samperk1/esphome-crowpanel-579) never exercises a partial
 * window anywhere near that large in its own photograph-verified test
 * suite (partial_refresh_test.yaml only tries 200x112 / 391x60 windows)
 * -- there is no evidence the panel's fast waveform is meant to run
 * across that much untouched background, and doing so is a strong
 * candidate for the "whole screen outside the edited line inverts
 * black then white on every keystroke" symptom reported on real
 * hardware. Tracking a handful of separate rectangles and refreshing
 * each individually keeps every partial window tight to what actually
 * changed, at the cost of one extra fixed-duration refresh event per
 * extra disjoint area touched in a render cycle (the SSD1683's
 * waveform runs for the same duration regardless of window size, so N
 * separate windows costs roughly N times the single-window latency).
 */
#define MAX_DIRTY_RECTS 4

struct DirtyRect {
    int  x0, y0, x1, y1;
    bool valid;
};

static DirtyRect s_dirty[MAX_DIRTY_RECTS];
static bool s_force_full    = true;
static int  s_partial_count = 0;

/* Two rectangles are considered part of the same on-screen edit if
 * they overlap or are within 1 px of touching -- pixels belonging to
 * one LVGL invalidated area arrive from flush_cb() spatially
 * contiguous (row by row within that area's bounds), so this reliably
 * keeps one area's pixels in one slot while still starting a new slot
 * for a spatially distant area. */
static inline bool rects_mergeable(int ax0, int ay0, int ax1, int ay1,
                                   int bx0, int by0, int bx1, int by1)
{
    return !(ax1 + 1 < bx0 || bx1 + 1 < ax0 ||
              ay1 + 1 < by0 || by1 + 1 < ay0);
}

static inline void mark_dirty_rect(int x, int y, int w, int h)
{
    if (w <= 0 || h <= 0) return;
    int x0 = x, y0 = y, x1 = x + w - 1, y1 = y + h - 1;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= PANEL_W) x1 = PANEL_W - 1;
    if (y1 >= PANEL_H) y1 = PANEL_H - 1;
    if (x1 < x0 || y1 < y0) return;

    for (int i = 0; i < MAX_DIRTY_RECTS; i++) {
        if (!s_dirty[i].valid) continue;
        if (rects_mergeable(x0, y0, x1, y1, s_dirty[i].x0, s_dirty[i].y0,
                            s_dirty[i].x1, s_dirty[i].y1)) {
            if (x0 < s_dirty[i].x0) s_dirty[i].x0 = x0;
            if (y0 < s_dirty[i].y0) s_dirty[i].y0 = y0;
            if (x1 > s_dirty[i].x1) s_dirty[i].x1 = x1;
            if (y1 > s_dirty[i].y1) s_dirty[i].y1 = y1;
            return;
        }
    }
    for (int i = 0; i < MAX_DIRTY_RECTS; i++) {
        if (!s_dirty[i].valid) {
            s_dirty[i].x0 = x0; s_dirty[i].y0 = y0;
            s_dirty[i].x1 = x1; s_dirty[i].y1 = y1;
            s_dirty[i].valid = true;
            return;
        }
    }
    /* All slots taken by mutually-distant rects: fall back to
     * widening slot 0 rather than dropping the update. Still correct
     * (s_disp_buf already has the right pixels either way), just less
     * tightly windowed for this one edge case. */
    if (x0 < s_dirty[0].x0) s_dirty[0].x0 = x0;
    if (y0 < s_dirty[0].y0) s_dirty[0].y0 = y0;
    if (x1 > s_dirty[0].x1) s_dirty[0].x1 = x1;
    if (y1 > s_dirty[0].y1) s_dirty[0].y1 = y1;
}

static inline void clear_dirty(void)
{
    for (int i = 0; i < MAX_DIRTY_RECTS; i++) s_dirty[i].valid = false;
}

static inline bool any_dirty(void)
{
    for (int i = 0; i < MAX_DIRTY_RECTS; i++) if (s_dirty[i].valid) return true;
    return false;
}

static void send_command(uint8_t cmd)
{
    esp_lcd_panel_io_tx_param(s_io_handle, cmd, NULL, 0);
}

static void send_data(uint8_t data)
{
    esp_lcd_panel_io_tx_param(s_io_handle, -1, &data, 1);
}

static void send_buffer(const uint8_t *data, size_t len)
{
    esp_lcd_panel_io_tx_color(s_io_handle, -1, data, len);
}

static void send_ram(uint8_t cmd, const uint8_t *data, size_t len)
{
    send_command(cmd);
    send_buffer(data, len);
}

static void hw_reset(void)
{
    gpio_set_level((gpio_num_t)EPD_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level((gpio_num_t)EPD_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level((gpio_num_t)EPD_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
}

static void wait_busy(void)
{
    /* BUSY is active HIGH on the SSD1683. Poll at 10 ms with a 10 s
     * ceiling (a full refresh takes ~2-3 s; 10 s leaves generous
     * headroom without hanging forever on a wiring fault). */
    for (int i = 0; i < 1000; i++) {
        if (gpio_get_level((gpio_num_t)EPD_BUSY_PIN) == 0) return;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    ESP_LOGE(TAG, "BUSY timeout");
}

/* Extra settle time after a "Master Activation" (0x20) refresh
 * completes (BUSY low), before this backend will issue the next
 * command sequence. Editing text triggers a display refresh on
 * essentially every keystroke, so refreshes on this panel are
 * routinely issued back-to-back the moment BUSY clears -- far more
 * rapidly than the single, human-paced button presses the upstream
 * ESPHome driver this backend is ported from was exercised with.
 * Field testing on real hardware showed the whole panel alternating
 * between an old, fully-settled frame and the current one on every
 * keystroke under that rapid-fire pattern; the SSD1683 datasheet's
 * own documented Wait-BUSY-Low synchronization point may not fully
 * cover the panel's analog rail settle time once a *new* refresh
 * starts immediately after -- BUSY going low ends the chip's own
 * internal processing, but the panel's pixels may still be
 * physically mid-transition, and starting the next refresh's RAM
 * write + trigger right then would cut that transition short,
 * leaving pixels part-way reverted toward what they looked like
 * before this refresh started. This mirrors the debounce/settle
 * handling display_epdiy.cpp already needs for the same "rapid
 * typing hammers an e-paper backend" scenario. Applied only after
 * an actual "Master Activation" refresh (not the shorter waits used
 * during LUT loading in init_display()). 100ms was not enough to
 * eliminate an earlier, different-looking symptom (the whole panel
 * alternating between two full frames); bumped to 400ms. A separate,
 * more serious "recently-written text goes missing" symptom traced
 * to a much longer settle time (2500ms) made no difference either --
 * see the CROWPANEL_PARTIAL_REFRESH comment near display_flush() for
 * what actually fixed that one. */
#define EPD_REFRESH_SETTLE_MS 400

static void wait_busy_after_refresh(void)
{
    wait_busy();
    vTaskDelay(pdMS_TO_TICKS(EPD_REFRESH_SETTLE_MS));
}

/*
 * Point one chip's RAM window at [bs, be] (inclusive column-byte
 * indices, chip-local) x [y0, y1] (inclusive panel rows) and leave
 * the address counter at the start of that window. Called both for
 * the full-panel window (init / full refresh) and for a clipped
 * sub-window (partial refresh) -- the two are the same operation at
 * different extents.
 *
 * The master's X addressing counts down from the high byte (entry
 * mode 0x02: Y increment, X decrement), so its byte range [bs, be]
 * (bs = left/low column, be = right/high column, both in the shared
 * 0-98 buffer-byte index space) maps to chip-local X addresses
 * [98-bs, 98-be] -- see the "Master: X addressing" note in the
 * upstream ESPHome driver this is ported from.
 */
static void set_window_slave(int bs, int be, int y0, int y1)
{
    send_command(0x91); send_data(0x03);
    send_command(0xC4); send_data((uint8_t)(bs & 0x3F)); send_data((uint8_t)(be & 0x3F));
    send_command(0xC5); send_data((uint8_t)(y0 & 0xFF)); send_data((uint8_t)((y0 >> 8) & 0x01));
                        send_data((uint8_t)(y1 & 0xFF)); send_data((uint8_t)((y1 >> 8) & 0x01));
    send_command(0xCE); send_data((uint8_t)(bs & 0x3F));
    send_command(0xCF); send_data((uint8_t)(y0 & 0xFF)); send_data((uint8_t)((y0 >> 8) & 0x01));
}

static void set_window_master(int bs, int be, int y0, int y1)
{
    int xs = 98 - bs;
    int xe = 98 - be;
    send_command(0x11); send_data(0x02);
    send_command(0x44); send_data((uint8_t)(xs & 0x3F)); send_data((uint8_t)(xe & 0x3F));
    send_command(0x45); send_data((uint8_t)(y0 & 0xFF)); send_data((uint8_t)((y0 >> 8) & 0x01));
                        send_data((uint8_t)(y1 & 0xFF)); send_data((uint8_t)((y1 >> 8) & 0x01));
    send_command(0x4E); send_data((uint8_t)(xs & 0x3F));
    send_command(0x4F); send_data((uint8_t)(y0 & 0xFF)); send_data((uint8_t)((y0 >> 8) & 0x01));
}

/* Pack the [bs, be] x [y0, y1] sub-rectangle of s_disp_buf (row
 * stride BYTES_PER_ROW) into a contiguous scratch buffer, ready for
 * one send_buffer() burst. */
static void pack_window(uint8_t *dst, int bs, int be, int y0, int y1)
{
    int w = be - bs + 1;
    for (int y = y0; y <= y1; y++) {
        memcpy(dst + (size_t)(y - y0) * (size_t)w,
               s_disp_buf + (size_t)y * BYTES_PER_ROW + bs, (size_t)w);
    }
}

/*
 * Configure both chips for "Cascade application" (SSD1683 datasheet
 * "Display Update Control 1", command 0x21, second data byte bit 4)
 * before every Master Activation. Datasheet section 6.12 documents
 * this as required for exactly this board's wiring -- two SSD1683
 * chips sharing one CL synchronization pin, one "master" and one
 * "slave" -- but this backend never sent it at all until now. An
 * earlier attempt at adding it (git log ca0df1d, reverted after a
 * severe hardware regression: "nothing renders, screen fades to
 * white") sent 0x21 only to the master's command range and never to
 * the slave's remapped range. Every other command this driver sends
 * to the slave chip is the master's command plus 0x80 (0x11->0x91,
 * 0x44->0xC4, 0x24->0xA4, 0x26->0xA6 -- see set_window_slave() above,
 * verified against samperk1's hardware-tested driver), so 0x21's
 * slave equivalent is 0xA1. Leaving the master in cascade mode while
 * the slave stayed at its power-on-reset default of "single chip
 * application" is a far more likely explanation for that regression
 * (the two chips' activation state machines disagreeing about how to
 * hand off to each other) than the cascade command itself being
 * wrong -- especially since that failure was identical across four
 * separate follow-up attempts that each isolated a different other
 * variable and left this one unchanged. This sends it to both chips.
 */
static void set_cascade_mode(void)
{
    send_command(0x21); send_data(0x00); send_data(0x10);
    send_command(0xA1); send_data(0x00); send_data(0x10);
}

/*
 * GxEPD2's reference driver for this exact panel
 * (github.com/ZinggJM/GxEPD2, src/gdey/GxEPD2_579_GDEY0579T93.cpp --
 * a far more widely used and hardware-tested library than the
 * single-author ESPHome driver this backend was originally ported
 * from) re-addresses *both* chips to their full local RAM range
 * immediately before every Master Activation trigger, full or
 * partial, regardless of how narrow the preceding data write's own
 * window was (see its refresh(int16_t,int16_t,int16_t,int16_t) and
 * refresh(bool)). This backend never did that -- it fires the
 * trigger using whatever window the preceding data write left
 * active. If the SSD1683's internal refresh/waveform engine needs
 * the RAM address-range registers to cover the full panel at trigger
 * time to behave correctly, regardless of how little data was
 * actually written, triggering with a narrow window still active (as
 * this backend always has) could plausibly leave the untouched
 * remainder of the panel in an undefined state -- a candidate
 * explanation for the "shows a stale prior frame" symptom that five
 * other independent Old/New-RAM-focused fix attempts did not
 * resolve. This exact change was tried once before (git log
 * ca0df1d), but bundled together with the master-only cascade-mode
 * bug that same commit introduced, and reverted along with it before
 * ever being isolated and re-tested against the since-fixed cascade
 * configuration.
 *
 * The master's re-address here uses entry mode 0x03 (X increment),
 * not the 0x02 (X decrement + inversion) set_window_master() uses
 * for actual data writes -- matching GxEPD2's
 * _setPartialRamAreaMaster() default mode exactly. This call carries
 * no data of its own (see its call sites), so the direction bit does
 * not affect what ends up in RAM; using GxEPD2's exact mode keeps
 * this faithful to a hardware-verified reference rather than
 * reusing our own data-write-specific convention for a different
 * purpose.
 */
static void set_full_window_for_trigger(void)
{
    set_window_slave(0, 49, 0, PANEL_H - 1);

    send_command(0x11); send_data(0x03);
    send_command(0x44); send_data(0x00); send_data(0x31);
    send_command(0x45); send_data(0x00); send_data(0x00);
                        send_data((uint8_t)((PANEL_H - 1) & 0xFF));
                        send_data((uint8_t)(((PANEL_H - 1) >> 8) & 0x01));
    send_command(0x4E); send_data(0x00);
    send_command(0x4F); send_data(0x00); send_data(0x00);
}

static void epd_update_full(void)
{
    set_window_slave(0, 49, 0, PANEL_H - 1);
    pack_window(s_pack_a, 0, 49, 0, PANEL_H - 1);
    send_ram(0xA4, s_pack_a, HALF_BUF_LEN);

    set_window_master(49, 98, 0, PANEL_H - 1);
    pack_window(s_pack_b, 49, 98, 0, PANEL_H - 1);
    send_ram(0x24, s_pack_b, HALF_BUF_LEN);

    set_full_window_for_trigger();
    set_cascade_mode();
    send_command(0x22); send_data(0xF7);  /* full OTP waveform */
    send_command(0x20);
    wait_busy_after_refresh();

    /* Keep Old RAM in sync with what is now actually on the panel
     * (s_pack_a/s_pack_b already hold exactly that content from the
     * packing above -- nothing overwrote them across the trigger) so
     * that any *partial* refresh which does not happen to touch a
     * given chip still has an accurate Old-RAM baseline there. Both
     * chips' Master Activation is a shared broadcast trigger (there
     * is no way to fire only one), so a partial refresh whose dirty
     * rect only touches one chip's columns still re-triggers the
     * *other* chip's own refresh using whatever is currently in its
     * RAM -- if that chip's Old RAM were left stale (mismatched
     * against its own unchanged, correct New RAM), every such
     * unrelated keystroke would still make it redraw as if content
     * had changed there. This was the previous approach here
     * (forcing Old RAM to all-black after every full refresh), on the
     * theory that the 0xF7 GC16 waveform's own documented auto-copy
     * of New->Old after refresh needed correcting to avoid a stuck-
     * black W->W transition on a later *repeat* full refresh -- but
     * forcing it to black is exactly the mismatch described above,
     * every single time until the next full refresh. A live editor's
     * content is essentially never byte-identical across two full
     * refreshes 30 keystrokes apart, so the repeat-content stuck-
     * black risk this used to guard against is far less costly than
     * paying it on every partial refresh in between. */
    set_window_slave(0, 49, 0, PANEL_H - 1);
    send_ram(0xA6, s_pack_a, HALF_BUF_LEN);

    set_window_master(49, 98, 0, PANEL_H - 1);
    send_ram(0x26, s_pack_b, HALF_BUF_LEN);
}

/*
 * Rewrite the *entire* panel's New and Old RAM, on both chips, before
 * every partial refresh -- not just the dirty rectangle. Narrower
 * windowing (this function's previous approach, and GxEPD2's own
 * writeImage()/writeImageAgain(), which only touch the dirty area)
 * still left every partial refresh's trigger implicitly relying on
 * whatever the *rest* of the panel's RAM already held from earlier
 * writes. On-hardware testing kept finding cases where that RAM
 * didn't actually hold what software believed it did, however
 * carefully each individual write and sync was ordered. Writing the
 * complete, authoritative s_disp_buf content to New RAM (so New RAM
 * can never be anything other than exactly correct) and then to Old
 * RAM (so Old RAM always reflects exactly what New RAM held one
 * refresh ago, for every pixel, not just the ones a given keystroke
 * happened to touch) removes any way for a region's RAM to drift out
 * of sync with reality between refreshes, at the cost of the same
 * SPI transfer size as a full refresh -- only the waveform selector
 * (0xFF fast vs 0xF7 full, still on the dirty-triggered vs periodic
 * cadence display_flush() already applies) still distinguishes a
 * "partial" refresh from a full one. */
static void epd_update_partial(void)
{
    set_window_slave(0, 49, 0, PANEL_H - 1);
    pack_window(s_pack_a, 0, 49, 0, PANEL_H - 1);
    send_ram(0xA4, s_pack_a, HALF_BUF_LEN);

    set_window_master(49, 98, 0, PANEL_H - 1);
    pack_window(s_pack_b, 49, 98, 0, PANEL_H - 1);
    send_ram(0x24, s_pack_b, HALF_BUF_LEN);

    /* Border waveform override (VCOM), re-asserted before every
     * partial trigger -- matches GxEPD2's _Update_Part() exactly.
     * init_display()'s own border-waveform command carries over for
     * full refreshes (GxEPD2's _Update_Full() does not set this
     * either). */
    send_command(0x3C); send_data(0x80);

    set_full_window_for_trigger();
    set_cascade_mode();
    send_command(0x22); send_data(0xFF);  /* fast B/W OTP LUT */
    send_command(0x20);
    wait_busy_after_refresh();

    /* s_pack_a/s_pack_b still hold exactly what was just written as
     * New RAM (nothing overwrote them across the trigger) -- reuse
     * them to keep Old RAM in sync with the whole panel too. */
    set_window_slave(0, 49, 0, PANEL_H - 1);
    send_ram(0xA6, s_pack_a, HALF_BUF_LEN);

    set_window_master(49, 98, 0, PANEL_H - 1);
    send_ram(0x26, s_pack_b, HALF_BUF_LEN);
}

static void init_display(void)
{
    hw_reset();
    wait_busy();

    send_command(0x12);  /* SW reset */
    wait_busy();

    send_command(0x18); send_data(0x80);  /* internal temperature sensor */
    send_command(0x22); send_data(0xB1);  /* enable clock, CP, load temp */
    send_command(0x20);
    wait_busy();

    send_command(0x1A); send_data(0x64); send_data(0x00);  /* temperature value */

    send_command(0x22); send_data(0x91);  /* enable clock, load LUT from OTP */
    send_command(0x20);
    wait_busy();

    send_command(0x3C); send_data(0x03);  /* border waveform */
    wait_busy();

    /* Bring both chips' New and Old RAM to all-black, so the
     * clean-to-white full refresh below fires a real B->W transition
     * across the whole panel instead of relying on undefined POR
     * RAM contents. */
    set_window_master(49, 98, 0, PANEL_H - 1);
    memset(s_pack_b, 0x00, HALF_BUF_LEN);
    send_ram(0x24, s_pack_b, HALF_BUF_LEN);
    set_window_master(49, 98, 0, PANEL_H - 1);
    send_ram(0x26, s_pack_b, HALF_BUF_LEN);

    set_window_slave(0, 49, 0, PANEL_H - 1);
    memset(s_pack_a, 0x00, HALF_BUF_LEN);
    send_ram(0xA4, s_pack_a, HALF_BUF_LEN);
    set_window_slave(0, 49, 0, PANEL_H - 1);
    send_ram(0xA6, s_pack_a, HALF_BUF_LEN);

    set_cascade_mode();
    send_command(0x22); send_data(0xF7);
    send_command(0x20);
    wait_busy();
}

/* ---- public API ---- */

extern "C" void display_init(int /*mosi*/, int /*sck*/, int /*dc*/,
                             int /*cs*/, int /*rst*/, int /*busy*/,
                             int width, int height)
{
    if (s_initialized) return;

    if (width != PANEL_W || height != PANEL_H) {
        ESP_LOGW(TAG, "Configured %dx%d does not match panel size %dx%d; "
                      "using panel size", width, height, PANEL_W, PANEL_H);
    }

    /* Panel power rail, driven once at boot and left alone --
     * display_deep_sleep_prepare() does not touch it (see that
     * function). gpio_hold_dis() is a defensive no-op unless a pad
     * hold survived from a firmware version that used to cut this
     * pin before sleep. */
    gpio_hold_dis((gpio_num_t)EPD_PWR_PIN);
    gpio_config_t pwr_cfg = {};
    pwr_cfg.intr_type    = GPIO_INTR_DISABLE;
    pwr_cfg.mode         = GPIO_MODE_OUTPUT;
    pwr_cfg.pin_bit_mask = (1ULL << EPD_PWR_PIN);
    ESP_ERROR_CHECK(gpio_config(&pwr_cfg));
    gpio_set_level((gpio_num_t)EPD_PWR_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(10));

    gpio_config_t rst_cfg = {};
    rst_cfg.intr_type    = GPIO_INTR_DISABLE;
    rst_cfg.mode         = GPIO_MODE_OUTPUT;
    rst_cfg.pin_bit_mask = (1ULL << EPD_RST_PIN);
    ESP_ERROR_CHECK(gpio_config(&rst_cfg));

    gpio_config_t busy_cfg = {};
    busy_cfg.intr_type    = GPIO_INTR_DISABLE;
    busy_cfg.mode         = GPIO_MODE_INPUT;
    busy_cfg.pin_bit_mask = (1ULL << EPD_BUSY_PIN);
    busy_cfg.pull_down_en = GPIO_PULLDOWN_ENABLE;
    ESP_ERROR_CHECK(gpio_config(&busy_cfg));

    spi_bus_config_t bus_cfg = {};
    bus_cfg.mosi_io_num     = EPD_MOSI_PIN;
    bus_cfg.miso_io_num     = -1;
    bus_cfg.sclk_io_num     = EPD_SCK_PIN;
    bus_cfg.quadwp_io_num   = -1;
    bus_cfg.quadhd_io_num   = -1;
    bus_cfg.max_transfer_sz = HALF_BUF_LEN;
    ESP_ERROR_CHECK(spi_bus_initialize(EPD_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_spi_config_t io_cfg = {};
    io_cfg.dc_gpio_num       = (gpio_num_t)EPD_DC_PIN;
    io_cfg.cs_gpio_num       = (gpio_num_t)EPD_CS_PIN;
    /* SSD1683 datasheet Rev 1.0: "maximum SPI write speed 20MHz"
     * (section 6.2) and fSCL write-mode max 20MHz (section 8, AC
     * characteristics). Every refresh now clocks a full HALF_BUF_LEN
     * (13600 bytes) to each chip twice per keystroke (see
     * display_flush()), so this dominates per-keystroke latency --
     * was 2MHz (no documented reason found for that choice), 10x
     * below the chip's rated maximum. If this causes visible glitches
     * (more likely with longer/marginal wiring), drop back down. */
    io_cfg.pclk_hz           = 20 * 1000 * 1000;
    io_cfg.lcd_cmd_bits      = 8;
    io_cfg.lcd_param_bits    = 8;
    io_cfg.spi_mode          = 0;
    io_cfg.trans_queue_depth = 10;
    /* See the DMA_ALIGN_BYTES comment above -- without this flag, a
     * PSRAM source buffer gets bounced through a freshly malloc'd
     * internal-DRAM copy that can silently fail (and drop the rest of
     * the transfer) once internal SRAM is under pressure. */
    io_cfg.flags.psram_dma_direct = 1;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)EPD_SPI_HOST,
                                             &io_cfg, &s_io_handle));

    s_disp_buf = (uint8_t *)heap_caps_malloc(FULL_BUF_LEN, MALLOC_CAP_SPIRAM);
    /* Aligned (and sized so the whole buffer stays aligned) so the
     * psram_dma_direct fast path above is always taken instead of
     * falling back to the internal-DRAM bounce copy. */
    s_pack_a = (uint8_t *)heap_caps_aligned_alloc(DMA_ALIGN_BYTES, HALF_BUF_ALLOC_LEN,
                                                  MALLOC_CAP_SPIRAM);
    s_pack_b = (uint8_t *)heap_caps_aligned_alloc(DMA_ALIGN_BYTES, HALF_BUF_ALLOC_LEN,
                                                  MALLOC_CAP_SPIRAM);
    assert(s_disp_buf && s_pack_a && s_pack_b);
    memset(s_disp_buf, 0xFF, FULL_BUF_LEN);  /* 0xFF = white paper */

    init_display();

    clear_dirty();
    s_force_full    = true;
    s_partial_count = 0;
    s_initialized   = true;

    ESP_LOGI(TAG, "SSD1683 x2 %dx%d initialized, full refresh every %d partials",
             PANEL_W, PANEL_H, SSD1683_FULL_REFRESH_INTERVAL);
}

extern "C" void display_clear(uint8_t color)
{
    if (!s_initialized) return;
    memset(s_disp_buf, color, FULL_BUF_LEN);
    mark_dirty_rect(0, 0, PANEL_W, PANEL_H);
    s_force_full = true;
}

extern "C" void display_set_pixel(uint16_t x, uint16_t y, uint8_t color)
{
    if (!s_initialized) return;
    if (x >= PANEL_W || y >= PANEL_H) return;
    size_t idx  = (size_t)y * BYTES_PER_ROW + (x >> 3);
    uint8_t mask = (uint8_t)(0x80 >> (x & 7));
    if (color) s_disp_buf[idx] |= mask;   /* bit=1: white paper */
    else       s_disp_buf[idx] &= (uint8_t)~mask;  /* bit=0: black ink */
    mark_dirty_rect(x, y, 1, 1);
}

extern "C" bool display_push_rgb565(int /*x*/, int /*y*/, int /*w*/, int /*h*/,
                                    const void * /*color_map*/)
{
    /* No fast path -- lvgl_port.cpp falls back to the per-pixel
     * display_set_pixel conversion (same as display_rlcd.cpp). */
    return false;
}

/*
 * A single Master Activation trigger on this panel can leave an
 * incomplete pixel transition -- independent of which waveform is
 * used and regardless of RAM content, confirmed on real hardware (see
 * PR #47 discussion, github.com/clackups/draftling/pull/47).
 * Immediately repeating the exact same update a second time reliably
 * completes it, matching what pressing Ctrl+R already did manually --
 * display_flush() below runs both the full and partial refresh paths
 * twice for this reason. Re-triggering *without* rewriting RAM does
 * not work (it actively erases the just-drawn content instead); the
 * second call must be a full, independent run of the update.
 */
#define CROWPANEL_PARTIAL_REFRESH 1

extern "C" void display_flush(void)
{
    if (!s_initialized) return;
    if (!any_dirty()) return;

#if CROWPANEL_PARTIAL_REFRESH
    /* Unlike display_epdiy.cpp (whose "huge dirty area -> promote to
     * full" rule exists to dodge a specific async-feeder-task wedge
     * bug in that backend's LCD-peripheral render path), this backend
     * is a plain synchronous SPI driver with no equivalent failure
     * mode, so that promotion isn't ported here: with the editor's
     * title bar sitting at the top of a 272 px panel -- the shortest
     * of any supported board, by a wide margin -- a *single* dirty
     * rectangle this large would cover a much larger fraction of this
     * panel's height than the same absolute pixel span would on any
     * taller board. The upstream community driver this backend is
     * ported from exercises partial refreshes at least this large
     * (e.g. a 200x112 px window) successfully, so there is no
     * evidence the panel itself needs a single rectangle's area
     * capped -- see mark_dirty_rect()'s block comment for why the
     * title bar and the edited line are kept as separate rectangles
     * instead of one that also spans the untouched gap between them.
     * Only an explicit request (display_clear/display_full_refresh/
     * display_request_full_refresh) or the periodic
     * CONFIG_DRAFTLING_EPD_FULL_REFRESH_INTERVAL ghost-clearing pass
     * promotes to full here. */
    bool do_full = s_force_full ||
                   s_partial_count >= SSD1683_FULL_REFRESH_INTERVAL;
#else
    bool do_full = true;
#endif

    if (do_full) {
        /* Called twice -- see the CROWPANEL_PARTIAL_REFRESH comment
         * above. */
        epd_update_full();
        epd_update_full();
        s_partial_count = 0;
        s_force_full    = false;
    } else {
        /* epd_update_partial() rewrites the entire panel's RAM on
         * every call (see its own comment), so calling it once per
         * s_dirty[] entry here (left over from when it only wrote a
         * narrow window) would repeat the same ~1s+ full-buffer
         * transfer up to MAX_DIRTY_RECTS times for a single keystroke,
         * for no benefit -- call it twice total instead, matching
         * epd_update_full()'s fix above. */
        epd_update_partial();
        epd_update_partial();
        s_partial_count++;
    }

    clear_dirty();
}

extern "C" void display_full_refresh(void)
{
    if (!s_initialized) return;
    mark_dirty_rect(0, 0, PANEL_W, PANEL_H);
    s_force_full = true;
    display_flush();
}

extern "C" void display_request_full_refresh(void)
{
    if (!s_initialized) return;
    s_force_full = true;
}

extern "C" void display_set_partial_clip(int /*x*/, int /*y*/,
                                         int /*w*/, int /*h*/)
{
    /* No-op on this backend; the dirty bbox already drives the
     * refresh region. */
}

extern "C" uint8_t *display_get_buffer(void)
{
    return s_disp_buf;
}

extern "C" int display_get_buffer_size(void)
{
    return FULL_BUF_LEN;
}

extern "C" void display_set_backlight(int /*percent*/)
{
    /* No backlight on this panel. No-op. */
}

extern "C" void display_sleep(void)
{
    /* E-paper retains its image without power; the standby manager
     * uses deep sleep on this board rather than
     * CONFIG_DRAFTLING_STANDBY_DISPLAY_OFF. No-op, matching every
     * other e-paper backend in this codebase. */
}

extern "C" void display_wake(void)
{
    /* No-op (see display_sleep). */
}

extern "C" void display_deep_sleep_prepare(void)
{
    /* display.h's documented contract for this call is a no-op on
     * e-paper backends ("the panel retains its image without
     * power"), matching display_rlcd.cpp / display_epdiy.cpp.
     *
     * An earlier version of this backend cut the panel's 3.3 V rail
     * (EPD_PWR_PIN) here, on the theory that the SSD1683 is bistable
     * and a power cut right after a completed refresh is harmless.
     * On real hardware this left the panel showing its pre-sleep
     * content instead of the white frame pre_sleep_autosave() had
     * just pushed and waited for BUSY to confirm -- cutting power
     * without first issuing the controller's own deep-sleep command
     * (0x10, unverified opcode for the slave half of this dual-chip
     * panel) evidently does not commit the last waveform cleanly on
     * this panel. Leaving the rail powered avoids that regression;
     * the panel's own standby current when idle is low enough that
     * this is not worth the risk without a verified safe power-down
     * sequence. */
}

extern "C" void display_set_shared_i2c_bus(void * /*bus_handle*/)
{
    /* This backend does not use I2C. No-op. */
}

#endif /* CONFIG_DRAFTLING_DISPLAY_SSD1683 */
