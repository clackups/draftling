#include <cstdio>
#include <cstring>
#include <cctype>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <sys/stat.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_app_desc.h>
#include <esp_mac.h>
#include <esp_chip_info.h>
#include <esp_flash.h>
#include <esp_heap_caps.h>
#include <esp_psram.h>
#include "soc/soc_caps.h"
#include <sd_protocol_defs.h>
#include <nvs_flash.h>
#include <nvs.h>
#include "sdkconfig.h"
#include "lvgl.h"
#include "widgets/label/lv_label_private.h"  /* offset / text_size of scrolling menu labels */
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "editor_ui.h"
#include "editor.h"
#include "md_parser.h"
#include "editor_format.h"
#include "projects.h"
#include "epub.h"
#include "ble_keyboard.h"
#if defined(CONFIG_DRAFTLING_HAS_USB_HOST)
#include "usb_kbd.h"
#endif
#if defined(CONFIG_DRAFTLING_HAS_TAB5_KBD)
#include "tab5_kbd.h"
#endif
#if defined(CONFIG_DRAFTLING_HAS_USB_MSC)
#include "usb_msc.h"
#endif
#include "kb_layout.h"
#include "cjk_width.h"
#if defined(CONFIG_KB_LAYOUT_ENABLE_VI) || defined(CONFIG_KB_LAYOUT_ENABLE_KO) || \
    defined(CONFIG_KB_LAYOUT_ENABLE_JA) || defined(CONFIG_KB_LAYOUT_ENABLE_ZH) || \
    defined(CONFIG_KB_LAYOUT_ENABLE_TA99) || defined(CONFIG_KB_LAYOUT_ENABLE_TA_TW) || \
    defined(CONFIG_KB_LAYOUT_ENABLE_TA_TV) || defined(CONFIG_KB_LAYOUT_ENABLE_BN_NJ) || \
    defined(CONFIG_KB_LAYOUT_ENABLE_TE_RTS) || defined(CONFIG_KB_LAYOUT_ENABLE_TE_PO) || \
    defined(CONFIG_KB_LAYOUT_ENABLE_TE_AP)
#define DRAFTLING_HAS_IME 1
#include "ime.h"
#endif
#ifdef CONFIG_DRAFTLING_HAS_TAMIL
#include "tamil_font.h"
#endif
#ifdef CONFIG_DRAFTLING_HAS_INDIC_SHAPING
#include "indic_font.h"
#endif
#include "wifi_manager.h"
#include "git_sync.h"
#if defined(CONFIG_DRAFTLING_HAS_KERMIT)
#include "kermit.h"
#endif
#include "sd_card.h"

/* What the documents are kept on, in user-visible text: a MicroSD card
 * or, with CONFIG_DRAFTLING_STORAGE_FLASH, a partition of the internal
 * flash (mounted at the same path, see sd_card_init_flash()). */
#if defined(CONFIG_DRAFTLING_STORAGE_FLASH)
#define STORAGE_NAME  "internal storage"
#else
#define STORAGE_NAME  "SD card"
#endif

#include "lvgl_port.h"
#include "display.h"
#include "display_margins.h"
#include "display_orientation.h"
#include "display_flip.h"
#include "display_scale.h"
#include "standby.h"
#include "lowres.h"
#include "battery.h"
#include "wifi_icon.h"
#include "freertos/task.h"

/*
 * Font aliases.
 *
 * Two monospaced bitmap font families are available. Standard-density
 * boards use Greybeard (Latin + Latin-1 + Cyrillic + Hebrew). The
 * high-density boards (CONFIG_DRAFTLING_DISPLAY_HIDPI -- the ones that
 * used to upscale the framebuffer 2x) instead render 1:1 with the
 * larger Hack family, whose six sizes mirror the Greybeard slots so
 * the rest of the editor is family-agnostic. The FONT_NN number is
 * the Greybeard slot; on Hack boards it maps to a larger pixel size
 * with an approximately matching row height.
 *
 * The body font is selected at runtime via the base font size user
 * setting (11, 14, 16, 18, or 22 px; high-density boards instead offer
 * 9, 11, 14, 16, or 20 px, since the larger Hack glyphs need fewer
 * body steps to cover the same visual range). Heading fonts are
 * scaled relative to the body size, going as high as slot 34 (only
 * defined on non-HIDPI boards, for the 22 px body size's H1).
 * Status bars always use FONT_11 regardless of the body font setting.
 */
/* The Indic pack renders with copies of the fonts whose lines are
 * about 30 % taller (lowres.c / hires.c, tall_lines.h), so the Tamil,
 * Devanagari, Bengali and Telugu letters can be drawn larger. */
#ifdef CONFIG_DRAFTLING_TALL_LINES
#define SLOT_FONT(f) ((const lv_font_t *)&f##_tall)
#else
#define SLOT_FONT(f) (&f)
#endif
#ifdef CONFIG_DRAFTLING_DISPLAY_HIDPI
#include "hires.h"
#define FONT_9  SLOT_FONT(hires_9)
#define FONT_11 SLOT_FONT(hires_11)
#define FONT_14 SLOT_FONT(hires_14)
#define FONT_16 SLOT_FONT(hires_16)
#define FONT_18 SLOT_FONT(hires_18)
#define FONT_22 SLOT_FONT(hires_22)
#define FONT_26 SLOT_FONT(hires_26)
#define FONT_30 SLOT_FONT(hires_30)
#else
#define FONT_11 SLOT_FONT(lowres_11)
#define FONT_14 SLOT_FONT(lowres_14)
#define FONT_16 SLOT_FONT(lowres_16)
#define FONT_18 SLOT_FONT(lowres_18)
#define FONT_22 SLOT_FONT(lowres_22)
#define FONT_26 SLOT_FONT(lowres_26)
#define FONT_30 SLOT_FONT(lowres_30)
#define FONT_34 SLOT_FONT(lowres_34)
#endif

static const char *TAG = "EditorUI";

/* A web page dims what it shows rather than a backlight. */
#if defined(CONFIG_DRAFTLING_WEB_PAGE)
#define BACKLIGHT_LABEL "Brightness"
#else
#define BACKLIGHT_LABEL "Backlight"
#endif

/* Mac / iOS key names (the web client on such a device): the Ctrl+ and
 * Alt+ of every hint, help row and menu label become Cmd+ and Opt+
 * (the page treats Command as Ctrl, and Option is Alt). Applied where
 * UI text reaches LVGL -- lv_label_set_text() and lv_list_add_button()
 * below -- except the document's own lines, which are set with
 * (lv_label_set_text)() to bypass it. */
#if defined(CONFIG_DRAFTLING_WEB_PAGE)
static bool s_mac_key_names;

extern "C" void editor_ui_set_mac_key_names(bool on)
{
    s_mac_key_names = on;
}

/* Command + these never reach a web page on a Mac (macOS or the browser
 * quits, closes, hides, minimizes, opens a window or tab, switches
 * apps): their hints keep Ctrl+, since Control works as Ctrl too. */
static bool mac_reserved_cmd(const char *k)
{
    static const char *const RESERVED[] = { "Q", "W", "M", "H", "N", "T", "Tab", "Space" };
    for (const char *r : RESERVED) {
        size_t n = strlen(r);
        if (!strncmp(k, r, n) && !isalnum((unsigned char)k[n])) return true;
    }
    return false;
}

static const char *mac_key_names(const char *s)
{
    if (!s_mac_key_names || !s || (!strstr(s, "Ctrl+") && !strstr(s, "Alt+")))
        return s;
    static std::string bufs[4];   /* a few calls' results alive at once */
    static unsigned bi;
    std::string &o = bufs[bi++ & 3];
    o.clear();
    for (const char *p = s; *p;) {
        if (!strncmp(p, "Ctrl+", 5) && !mac_reserved_cmd(p + 5)) {
            o += "Cmd+";
            p += 5;
        } else if (!strncmp(p, "Alt+", 4) && (p == s || !isalpha((unsigned char)p[-1]))) {
            o += "Opt+";
            p += 4;
        } else {
            o += *p++;
        }
    }
    return o.c_str();
}
#define lv_label_set_text(obj, text) lv_label_set_text((obj), mac_key_names(text))
#define lv_list_add_button(list, icon, text) \
    lv_list_add_button((list), (icon), mac_key_names(text))
#else
extern "C" void editor_ui_set_mac_key_names(bool on)
{
    (void)on;
}
#endif

/* Layout constants -- account for display rotation. The editor and
 * LVGL canvas render 1:1 at the panel resolution (no framebuffer
 * upscaling; high-density boards use a larger font instead), minus
 * the user-adjustable screen margins (display_margin_*(), zero by
 * default on every board -- see Settings -> Screen margins and
 * app_config.h's DISPLAY_LOGICAL_WIDTH/HEIGHT).
 *
 * The LVGL logical canvas has the panel's width and height swapped
 * whenever the *effective* rotation is 90 or 270 degrees. That happens
 * when the board's build-time base rotation
 * (CONFIG_DRAFTLING_DISPLAY_ROTATE_ANGLE) is 90/270, OR when the user
 * picked portrait orientation (Settings -> Display orientation, which
 * adds a quarter turn -- 90 or 270 degrees, per
 * CONFIG_DRAFTLING_DISPLAY_PORTRAIT_EXTRA_ROTATE), but not both -- an
 * XOR (both 90 and 270 flip the swap either way, so the exact quarter
 * turn does not matter here). display_orientation_is_portrait() is
 * frozen for the session (see display_orientation.h), so SCR_W / SCR_H
 * stay constant until the next restart, exactly like the margins. */
#if defined(CONFIG_DRAFTLING_DISPLAY_RESIZABLE)
/* A window that can be resized (the Linux application): the LVGL
 * display has the current canvas size (already divided by the 2x
 * scale), which the LVGL port changes before editor_ui_relayout().
 * No margins, orientation or base rotation apply. */
#define SCR_W        ((int)lv_display_get_horizontal_resolution(NULL))
#define SCR_H        ((int)lv_display_get_vertical_resolution(NULL))
#else
#if CONFIG_DRAFTLING_DISPLAY_ROTATE_ANGLE == 90 || CONFIG_DRAFTLING_DISPLAY_ROTATE_ANGLE == 270
static inline bool scr_axes_swapped(void) { return !display_orientation_is_portrait(); }
#else
static inline bool scr_axes_swapped(void) { return display_orientation_is_portrait(); }
#endif
/* The "2x scale" setting (display_scale.h, frozen for the session)
 * halves the LVGL canvas in both directions; the LVGL port draws every
 * pixel as a 2x2 block. */
#define SCR_W        ((scr_axes_swapped() \
    ? (CONFIG_DRAFTLING_DISPLAY_HEIGHT - display_margin_top()  - display_margin_bottom()) \
    : (CONFIG_DRAFTLING_DISPLAY_WIDTH  - display_margin_left() - display_margin_right())) \
    / display_scale_factor())
#define SCR_H        ((scr_axes_swapped() \
    ? (CONFIG_DRAFTLING_DISPLAY_WIDTH  - display_margin_left() - display_margin_right()) \
    : (CONFIG_DRAFTLING_DISPLAY_HEIGHT - display_margin_top()  - display_margin_bottom())) \
    / display_scale_factor())
#endif
/* Split panes top / bottom instead of left / right: on a portrait
 * screen, so the split follows the long axis. */
static inline bool split_top_bottom(void)
{
#if defined(CONFIG_DRAFTLING_DISPLAY_RESIZABLE)
    return SCR_H > SCR_W;
#else
    return display_orientation_is_portrait();
#endif
}
/* Width the editor and file browser screens lay out in: the screen
 * minus the strip on the right edge that holds the Ctrl+T writing
 * timer's progress line while a timer runs (s_ptimer_strip, 0
 * otherwise; see "Writing timer" below). */
static int s_ptimer_strip = 0;
#define CONTENT_W    (SCR_W - s_ptimer_strip)
/* Header / status bar heights. These bars always use FONT_11, so on
 * high-density boards (which render 1:1 with the taller Hack font
 * instead of the old 2x upscale) they must be twice as tall to fit the
 * larger glyphs -- this reproduces the physical bar height users saw
 * when these boards still upscaled a FONT_11 (Greybeard 11 px) bar 2x. */
#ifdef CONFIG_DRAFTLING_DISPLAY_HIDPI
#define BASE_HEADER_H 32
#define BASE_STATUS_H 32
#else
#define BASE_HEADER_H 16
#define BASE_STATUS_H 16
#endif
#if defined(CONFIG_DRAFTLING_DISPLAY_ROUND_CORNERS)
/* Rounded screen corners (the web client in full screen on a phone;
 * Settings -> Display -> "Rounded screen corners"): the bars grow to
 * cover the corners the host reports (editor_ui_set_screen_corners()),
 * so the text area below / above them keeps the full width, and their
 * text moves in from the corners. Set by corners_compute() before
 * build_screens(). */
static int s_hdr_h = BASE_HEADER_H;   /* title bar height */
static int s_sta_h = BASE_STATUS_H;   /* status bar height */
static int s_hdr_in_l, s_hdr_in_r;    /* title bar text: room left at its ends */
static int s_sta_in_l, s_sta_in_r;    /* status bar text: room left at its ends */
static int s_top_clear, s_bot_clear;  /* corner bands kept while the bars are hidden */
#define HEADER_H     s_hdr_h
#define STATUS_H     s_sta_h
#define HDR_IN_L     s_hdr_in_l
#define HDR_IN_R     s_hdr_in_r
#define STA_IN_L     s_sta_in_l
#define STA_IN_R     s_sta_in_r
#define TOP_CLEAR    s_top_clear
#define BOT_CLEAR    s_bot_clear
#else
#define HEADER_H     BASE_HEADER_H
#define STATUS_H     BASE_STATUS_H
#define HDR_IN_L     0
#define HDR_IN_R     0
#define STA_IN_L     0
#define STA_IN_R     0
#define TOP_CLEAR    0
#define BOT_CLEAR    0
#endif
#define LIST_PANEL_H (SCR_H - HEADER_H)  /* height for list panels below header */
/* Top of the title bar's text: the bottom of a taller bar, where the
 * rounded corners take the least room. */
#define HDR_TEXT_Y   (HEADER_H - BASE_HEADER_H)
/* The editor body sits between the bars, or fills the screen while
 * they are hidden (Settings -> Editor -> "Hide status bars while
 * typing", s_bars_hidden), less the bands rounded corners take.
 * bars_relayout() shifts each pane's view by whole pixels
 * (pane_t::top_px / lead_px) so the text stays put. */
#define EDITOR_Y     (s_bars_hidden ? TOP_CLEAR : HEADER_H)
#define EDITOR_H     (s_bars_hidden ? (SCR_H - TOP_CLEAR - BOT_CLEAR) \
                                    : (SCR_H - HEADER_H - STATUS_H))

/* ---- Base font size setting ----
 * The user can pick 11, 14, 16, 18, or 22 px as the editor body font.
 * Heading fonts are scaled up from the body size. High-density
 * (HIDPI) boards render with the larger Hack family and expose two
 * extra steps instead: 20 px, since their panels have the resolution
 * to show a bigger body font comfortably, and 9 px -- smaller than
 * the smallest Greybeard-mirrored slot -- for users who want to fit
 * more text on a large, dense panel. */
#ifdef CONFIG_DRAFTLING_DISPLAY_HIDPI
#define FONT_SIZE_COUNT 5
static const int FONT_SIZE_OPTIONS[FONT_SIZE_COUNT] = { 9, 11, 14, 16, 20 };
static const char *FONT_SIZE_LABELS[FONT_SIZE_COUNT] = { "9 px", "11 px", "14 px", "16 px", "20 px" };
#else
#define FONT_SIZE_COUNT 5
static const int FONT_SIZE_OPTIONS[FONT_SIZE_COUNT] = { 11, 14, 16, 18, 22 };
static const char *FONT_SIZE_LABELS[FONT_SIZE_COUNT] = { "11 px", "14 px", "16 px", "18 px", "22 px" };
#endif

/* NVS namespace/key for font size */
#define NVS_NS_EDITOR   "editor"
#define NVS_KEY_FONTSZ  "fontsz"
#define NVS_KEY_SPLIT   "split"

/* Current body font size in pixels (default 11) */
static int s_font_size = 11;

/* Derived layout values -- recomputed when font size changes */
static int s_line_h       = 11;
static int s_char_w       = 6;

/* Accessor macros that used to be compile-time constants.
 * VISIBLE_LINES is derived from the *current* pane's content height
 * (s_rp->h) so it stays correct whether the editor is showing one
 * full-height pane or two half-height split panes. */
#define LINE_H        s_line_h
#define VISIBLE_LINES (s_line_h > 0 ? (s_rp->h / s_line_h) : 1)
#define CHAR_W        s_char_w

/* How far to jump the viewport when the cursor falls outside it, as a
 * fraction of VISIBLE_LINES. Snapping the cursor to the very edge row
 * (the old behavior: scroll = cur_line, or cur_line - VISIBLE_LINES +
 * 1) means the next line of continued typing or navigation in the
 * same direction immediately falls outside the viewport again,
 * re-triggering a scroll -- and its full-screen re-render -- on
 * every single line. Jumping a majority of the screen height instead
 * leaves headroom so several more lines fit before another scroll is
 * needed, trading a slightly larger jump for far fewer redraws
 * (particularly noticeable on e-paper boards, where every redraw is
 * a visible, non-instant refresh). Used by every scroll-adjustment
 * site in this file, including refresh_active_pane()'s word-wrap
 * correction loop below, so a scroll is never just one line whether
 * it is triggered by ensure_cursor_visible(), editor_ui_move_visual(),
 * or a paragraph wrapping into more rows than expected.
 *
 * Defined near s_rp below (VISIBLE_LINES expands to an s_rp
 * dereference), rather than here alongside VISIBLE_LINES itself. */
#define SCROLL_JUMP_FRACTION 0.6f

/* Forward declaration (defined below) */
static int char_width_for_font(const lv_font_t *font);
static int status_avail_width(void);
static int utf8_cells_in_bytes(const char *text, size_t byte_len);
static size_t utf8_bytes_for_cells(const char *text, size_t byte_len, int cells);
static size_t utf8_byte_of_char(const char *s, size_t len, int n);

#if defined(CONFIG_DRAFTLING_DISPLAY_COLOR)
/* Forward declaration: tear down every screen / overlay and rebuild
 * them under the freshly-selected color theme. Defined alongside
 * the other init helpers near the bottom of this file. */
static void rebuild_screens_for_theme(void);
#endif

/* Return the body font for the current size setting. */
static const lv_font_t *body_font(void)
{
#ifdef CONFIG_DRAFTLING_DISPLAY_HIDPI
    if (s_font_size == 20) return FONT_18;
    if (s_font_size == 9)  return FONT_9;
#else
    if (s_font_size == 22) return FONT_22;
    if (s_font_size == 18) return FONT_18;
#endif
    if (s_font_size == 16) return FONT_16;
    if (s_font_size == 14) return FONT_14;
    return FONT_11;
}

/* Return heading fonts (h1/h2/h3) scaled relative to the body size.
 *   body 9  -> h3 11, h2 14, h1 16 (HIDPI only)
 *   body 11 -> h3 14, h2 16, h1 18
 *   body 14 -> h3 16, h2 18, h1 22
 *   body 16 -> h3 18, h2 22, h1 26
 *   body 18 -> h3 22, h2 26, h1 30 (non-HIDPI only; slot 30 is a clean
 *              2x pixel-double of Greybeard's native 15 px TTF, since
 *              Greybeard has no native 30 px size)
 *   body 20 -> h3 22, h2 26, h1 30 (HIDPI only; slot 30 is Hack-only)
 *   body 22 -> h3 26, h2 30, h1 34 (non-HIDPI only; slot 34 is a clean
 *              2x pixel-double of Greybeard's native 17 px TTF)
 */
static const lv_font_t *h1_font(void)
{
#ifdef CONFIG_DRAFTLING_DISPLAY_HIDPI
    if (s_font_size == 20) return FONT_30;
    if (s_font_size == 9)  return FONT_16;
#else
    if (s_font_size == 22) return FONT_34;
    if (s_font_size == 18) return FONT_30;
#endif
    if (s_font_size == 16) return FONT_26;
    if (s_font_size == 14) return FONT_22;
    return FONT_18;
}

static const lv_font_t *h2_font(void)
{
#ifdef CONFIG_DRAFTLING_DISPLAY_HIDPI
    if (s_font_size == 20) return FONT_26;
    if (s_font_size == 9)  return FONT_14;
#else
    if (s_font_size == 22) return FONT_30;
    if (s_font_size == 18) return FONT_26;
#endif
    if (s_font_size == 16) return FONT_22;
    if (s_font_size == 14) return FONT_18;
    return FONT_16;
}

static const lv_font_t *h3_font(void)
{
#ifdef CONFIG_DRAFTLING_DISPLAY_HIDPI
    if (s_font_size == 20) return FONT_22;
    if (s_font_size == 9)  return FONT_11;
#else
    if (s_font_size == 22) return FONT_26;
    if (s_font_size == 18) return FONT_22;
#endif
    if (s_font_size == 16) return FONT_18;
    if (s_font_size == 14) return FONT_16;
    return FONT_14;
}

/* ---- Modal overlay geometry ----
 * The small editor overlays (save prompt, exit prompt, search box)
 * lay a header line at y=0 and content rows below it. All use
 * FONT_11 for the header, so the header occupies one FONT_11 line height. Deriving
 * these offsets from the font (instead of a hard-coded 20 px that was
 * tuned for the 11 px Greybeard header) keeps the panels tall enough
 * that their border never clips the content on high-density boards,
 * where FONT_11 is the much taller Hack 19 px glyph. */
#define OVERLAY_PAD 6   /* lv_obj_set_style_pad_all() on each overlay */

/* Y offset (inside the padded content area) of the first content row
 * that sits below the overlay's FONT_11 header line. */
static int overlay_row_y(void)
{
    return lv_font_get_line_height(FONT_11) + 4;
}

/* Pitch between successive FONT_11 content rows in an overlay. */
static int overlay_row_pitch(void)
{
    return lv_font_get_line_height(FONT_11) + 2;
}

/* Search overlay row offsets (Find row, Replace row) inside the panel. */
static int search_find_row_y(void) { return overlay_row_y(); }
static int search_repl_row_y(void) { return overlay_row_y() + overlay_row_pitch(); }

/* X offset of the search field value labels: just past the "F:" / "R:"
 * two-character FONT_11 prefix, so the value never overlaps the prefix
 * even with the wider Hack glyphs on high-density boards. */
static int search_field_x(void)
{
    return 2 * char_width_for_font(FONT_11) + 4;
}

/* ---- Color theme ----
 *
 * The editor renders only two colors: a foreground (text, cursor,
 * borders, separator lines) and a background (screens, dialog panels).
 * Inverted highlights (selection rectangles, active list rows) draw
 * the foreground color as their background and vice-versa.
 *
 * On monochrome boards (RLCD, e-paper) the palette is fixed:
 * black-on-white by default, optionally inverted via
 * CONFIG_DRAFTLING_EPD_BLACK_BACKGROUND on the e-paper boards.
 *
 * On color LCDs (CONFIG_DRAFTLING_DISPLAY_COLOR) the user can pick
 * one of several preset themes from F1 -> Settings. The selection
 * is persisted in NVS.
 */
#if defined(CONFIG_DRAFTLING_DISPLAY_COLOR)

typedef struct {
    const char *name;       /* shown in Settings menu */
    uint32_t    fg_rgb;     /* 0xRRGGBB */
    uint32_t    bg_rgb;
} color_theme_t;

#if defined(CONFIG_DRAFTLING_DISPLAY_TRANSFLECTIVE)
/* Transflective panels (CONFIG_DRAFTLING_DISPLAY_TRANSFLECTIVE)
 * reproduce colors differently from transmissive LCDs, so they get
 * their own set of color themes. */
static const color_theme_t COLOR_THEMES[] = {
    { "Green on black",       0x00FF00, 0x000000 },
    { "Yellow on black",      0xFFFF00, 0x000000 },
    { "Cyan on black",        0x00FFFF, 0x000000 },
    { "White on black",       0xFFFFFF, 0x000000 },
    { "Black on white",       0x000000, 0xFFFFFF },
};
#else
static const color_theme_t COLOR_THEMES[] = {
    /* Color themes that look nice to the eye on most LCD panels.  */
    { "Light green on black", 0x00FF00, 0x000000 },
    { "Amber on dark-red",    0xCC6600, 0x400000 },
    { "Amber on black",       0xCC6600, 0x000000 },
    { "White on black",       0xFFFFFF, 0x000000 },
    { "Black on beige",       0x000000, 0xDAA06D },
};
#endif

#define COLOR_THEME_COUNT \
    ((int)(sizeof(COLOR_THEMES) / sizeof(COLOR_THEMES[0])))

#define NVS_KEY_THEME "theme"
static int s_theme_idx = 0;

static void load_theme_from_nvs(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS_EDITOR, NVS_READONLY, &h) == ESP_OK) {
        uint8_t v = 0;
        if (nvs_get_u8(h, NVS_KEY_THEME, &v) == ESP_OK && v < COLOR_THEME_COUNT) {
            s_theme_idx = v;
        }
        nvs_close(h);
    }
}

static void save_theme_to_nvs(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS_EDITOR, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_u8(h, NVS_KEY_THEME, (uint8_t)s_theme_idx);
        nvs_commit(h);
        nvs_close(h);
    }
}

static inline lv_color_t theme_fg(void)
{
    return lv_color_hex(COLOR_THEMES[s_theme_idx].fg_rgb);
}

static inline lv_color_t theme_bg(void)
{
    return lv_color_hex(COLOR_THEMES[s_theme_idx].bg_rgb);
}

#else  /* monochrome */

static inline lv_color_t theme_fg(void)
{
#if defined(CONFIG_DRAFTLING_EPD_BLACK_BACKGROUND)
    return lv_color_white();
#else
    return lv_color_black();
#endif
}

static inline lv_color_t theme_bg(void)
{
#if defined(CONFIG_DRAFTLING_EPD_BLACK_BACKGROUND)
    return lv_color_black();
#else
    return lv_color_white();
#endif
}

#endif  /* CONFIG_DRAFTLING_DISPLAY_COLOR */

/* ---- Backlight setting ----
 *
 * Boards whose display backend exposes a controllable (dimmable)
 * backlight (CONFIG_DRAFTLING_DISPLAY_HAS_BACKLIGHT, and not
 * CONFIG_DRAFTLING_DISPLAY_BACKLIGHT_BINARY -- see below) let the user
 * pick a brightness percentage from F1 -> Settings. The selection is
 * persisted in NVS and applied at boot (and immediately on change)
 * via display_set_backlight().
 *
 * The available steps are coarse on purpose: a single Enter on the
 * Settings list cycles through them, so finer granularity would be
 * tedious to dial in.
 *
 * Boards with CONFIG_DRAFTLING_DISPLAY_BACKLIGHT_BINARY (no PWM --
 * the backlight enable line is a plain on/off switch) skip this
 * entire feature: the Settings entry and the Ctrl+B shortcut are
 * both compiled out (every occurrence of
 * CONFIG_DRAFTLING_DISPLAY_HAS_BACKLIGHT in this file is guarded with
 * `&& !defined(CONFIG_DRAFTLING_DISPLAY_BACKLIGHT_BINARY)`), rather
 * than exposing a control that can only toggle the backlight fully
 * on or off. The backlight itself is left at whatever the display
 * backend's own default is (on) with no user override. */
#if defined(CONFIG_DRAFTLING_DISPLAY_HAS_BACKLIGHT) && !defined(CONFIG_DRAFTLING_DISPLAY_BACKLIGHT_BINARY)
/* The cycle steps. Boards whose lowest steps are unusably dim
 * either (a) raise CONFIG_DRAFTLING_BACKLIGHT_MIN_PCT to exclude
 * them at compile time, or (b) set CONFIG_DRAFTLING_BL_DUTY_FLOOR_PCT
 * so the display backend remaps low percents into a higher PWM
 * duty range (preferred -- keeps the full UI range). */
static const int BACKLIGHT_OPTIONS[] = {
#if CONFIG_DRAFTLING_BACKLIGHT_MIN_PCT <= 0
    /* Only meaningful on reflective / e-paper panels that stay
     * readable without any back/front-light; transmissive LCDs
     * raise CONFIG_DRAFTLING_BACKLIGHT_MIN_PCT above 0 to drop
     * this step. */
    0,
#endif
#if (CONFIG_DRAFTLING_BACKLIGHT_MIN_PCT <= 5) && \
    (defined(CONFIG_DRAFTLING_MODEL_LILYGO_T5_EPD_S3_PRO) || \
     defined(CONFIG_DRAFTLING_MODEL_LILYGO_T5_EPD_S3_PRO_H752))
    /* Extra-dim step for the LilyGO T5 E-Paper S3 Pro / Pro Lite
     * front-light: 10 % is already usable in a dark room but some
     * users want an even lower setting for night reading. The
     * e-paper panel itself stays readable without the front-light,
     * so 5 % is well above the "barely visible" threshold. */
    5,
#endif
#if CONFIG_DRAFTLING_BACKLIGHT_MIN_PCT <= 10
    10,
#endif
#if CONFIG_DRAFTLING_BACKLIGHT_MIN_PCT <= 25
    25,
#endif
#if CONFIG_DRAFTLING_BACKLIGHT_MIN_PCT <= 50
    50,
#endif
#if CONFIG_DRAFTLING_BACKLIGHT_MIN_PCT <= 75
    75,
#endif
    100
};
#define BACKLIGHT_OPTION_COUNT \
    ((int)(sizeof(BACKLIGHT_OPTIONS) / sizeof(BACKLIGHT_OPTIONS[0])))

#define NVS_KEY_BACKLIGHT "backlight"
/* Default CONFIG_DRAFTLING_BACKLIGHT_DEFAULT_PCT (50 % on most
 * boards), but never below the lowest still-allowed step (set per
 * board via CONFIG_DRAFTLING_BACKLIGHT_MIN_PCT). */
#if CONFIG_DRAFTLING_BACKLIGHT_MIN_PCT <= CONFIG_DRAFTLING_BACKLIGHT_DEFAULT_PCT
static int s_backlight_pct = CONFIG_DRAFTLING_BACKLIGHT_DEFAULT_PCT;
#else
static int s_backlight_pct = CONFIG_DRAFTLING_BACKLIGHT_MIN_PCT;
#endif

/* Backends with no level of their own to report (every device). The
 * Linux program's display backend overrides it. */
extern "C" __attribute__((weak)) int display_get_backlight(void)
{
    return -1;
}

static void load_backlight_from_nvs(void)
{
    nvs_handle_t h;
    bool saved = false;
    if (nvs_open(NVS_NS_EDITOR, NVS_READONLY, &h) == ESP_OK) {
        uint8_t v = 0;
        if (nvs_get_u8(h, NVS_KEY_BACKLIGHT, &v) == ESP_OK && v <= 100) {
            s_backlight_pct = v;
            saved = true;
        }
        nvs_close(h);
    }
    /* Never set: keep the level the hardware has now, if it says. */
    if (!saved) {
        int cur = display_get_backlight();
        if (cur >= 0 && cur <= 100) s_backlight_pct = cur;
    }
    /* Clamp a stale NVS value (e.g. one persisted before the min was
     * raised) to the lowest still-allowed option. */
    if (s_backlight_pct < CONFIG_DRAFTLING_BACKLIGHT_MIN_PCT) {
        s_backlight_pct = CONFIG_DRAFTLING_BACKLIGHT_MIN_PCT;
    }
}

static void save_backlight_to_nvs(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS_EDITOR, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_u8(h, NVS_KEY_BACKLIGHT, (uint8_t)s_backlight_pct);
        nvs_commit(h);
        nvs_close(h);
    }
}

static int find_backlight_option(int pct)
{
    for (int i = 0; i < BACKLIGHT_OPTION_COUNT; i++) {
        if (BACKLIGHT_OPTIONS[i] == pct) return i;
    }
    /* A level between the steps (the hardware's own, see
     * load_backlight_from_nvs()): the step below it, so the next
     * step up is the one above it. */
    int best = 0;
    for (int i = 0; i < BACKLIGHT_OPTION_COUNT; i++) {
        if (BACKLIGHT_OPTIONS[i] < pct) best = i;
    }
    return best;
}
#endif  /* CONFIG_DRAFTLING_DISPLAY_HAS_BACKLIGHT */

/* Host desktop clipboard: nothing on a device. The Linux program and
 * the web client override both (linux/src/clipboard_linux.c). */
extern "C" __attribute__((weak)) void editor_host_clipboard_set(const char *text, size_t len)
{
    (void)text;
    (void)len;
}

extern "C" __attribute__((weak)) bool editor_host_clipboard_request(void)
{
    return false;
}

/* A Ctrl+V waiting for the host clipboard's text: keys typed meanwhile
 * stay queued (key_drain_cb()), so they land after the pasted text. */
#define HOST_PASTE_TIMEOUT_MS 3000
static bool     s_host_paste_pending;
static uint32_t s_host_paste_start;

/* Offer what Ctrl+C / Ctrl+X just put on the clipboard to the host
 * (NULL when it went to the SD card file: the host then lets go of its
 * clipboard, so Ctrl+V uses the editor's own). */
static void host_clipboard_export(void)
{
    size_t len = 0;
    const char *text = editor_clipboard_data(&len);
    editor_host_clipboard_set(text, len);
}

#if defined(CONFIG_DRAFTLING_DISPLAY_CAN_ROTATE)
/* ---- Runtime 180-degree display flip ----
 * Toggled from the F1 -> Settings menu and persisted in NVS. Applied
 * on top of the build-time base rotation by the LVGL port (a 180 flip
 * never changes the reported resolution, so no widget rebuild is
 * needed). Useful on boards whose USB-C / SD-card edge can face either
 * way depending on the enclosure (e.g. Sunton ESP32-8048S043C). */
#define NVS_KEY_ROTATE180 "rot180"
static bool s_rotate180 = false;

static void load_rotate180_from_nvs(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS_EDITOR, NVS_READONLY, &h) == ESP_OK) {
        uint8_t v = 0;
        if (nvs_get_u8(h, NVS_KEY_ROTATE180, &v) == ESP_OK) {
            s_rotate180 = (v != 0);
        }
        nvs_close(h);
    }
}

static void save_rotate180_to_nvs(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS_EDITOR, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_u8(h, NVS_KEY_ROTATE180, s_rotate180 ? 1 : 0);
        nvs_commit(h);
        nvs_close(h);
    }
}
#endif  /* CONFIG_DRAFTLING_DISPLAY_CAN_ROTATE */

#if defined(CONFIG_DRAFTLING_DISPLAY_CAN_INVERT)
/* ---- Inverted screen ----
 * Toggled from the F1 -> Settings menu and persisted in NVS. When
 * enabled, active (drawn / ink) pixels show the background and
 * inactive pixels show the text -- a full visual invert, applied by
 * display_set_invert() at flush time without touching the framebuffer
 * or the LVGL widget tree. Currently only offered on the Waveshare
 * ESP32-S3-RLCD-4.2 (see display_rlcd.cpp). */
#define NVS_KEY_INVERT "invert"
static bool s_invert = false;

static void load_invert_from_nvs(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS_EDITOR, NVS_READONLY, &h) == ESP_OK) {
        uint8_t v = 0;
        if (nvs_get_u8(h, NVS_KEY_INVERT, &v) == ESP_OK) {
            s_invert = (v != 0);
        }
        nvs_close(h);
    }
}

static void save_invert_to_nvs(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS_EDITOR, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_u8(h, NVS_KEY_INVERT, s_invert ? 1 : 0);
        nvs_commit(h);
        nvs_close(h);
    }
}
#endif  /* CONFIG_DRAFTLING_DISPLAY_CAN_INVERT */

/* ---- Append-only editing ----
 * Toggled from the F1 -> Settings menu and persisted in NVS. When
 * active, existing text can no longer be deleted or edited: Backspace,
 * Delete and Ctrl+X (cut) are no-ops, and any key that would insert
 * text moves the cursor to the very end of the document first (if it
 * is not already there) so the new text is always appended. Enter
 * (newline) is exempt from the end-of-document jump so a blank line
 * can still be inserted wherever the cursor happens to be. */
#define NVS_KEY_APPEND_ONLY "appendonly"
static bool s_append_only = false;

static void load_append_only_from_nvs(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS_EDITOR, NVS_READONLY, &h) == ESP_OK) {
        uint8_t v = 0;
        if (nvs_get_u8(h, NVS_KEY_APPEND_ONLY, &v) == ESP_OK) {
            s_append_only = (v != 0);
        }
        nvs_close(h);
    }
}

static void save_append_only_to_nvs(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS_EDITOR, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_u8(h, NVS_KEY_APPEND_ONLY, s_append_only ? 1 : 0);
        nvs_commit(h);
        nvs_close(h);
    }
}

/* ---- Name suggestions ----
 * Toggled from F1 -> Settings -> Editor and persisted in NVS; on by
 * default. While the user types the start of a known name (in
 * Fountain: a Character cue), the matching names drop down under the
 * cursor -- see the format's suggest hook and suggest_update(). */
#define NVS_KEY_NAME_SUGGEST "namesugg"
static bool s_name_suggest = true;

static void load_name_suggest_from_nvs(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS_EDITOR, NVS_READONLY, &h) == ESP_OK) {
        uint8_t v = 1;
        if (nvs_get_u8(h, NVS_KEY_NAME_SUGGEST, &v) == ESP_OK) {
            s_name_suggest = (v != 0);
        }
        nvs_close(h);
    }
}

static void save_name_suggest_to_nvs(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS_EDITOR, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_u8(h, NVS_KEY_NAME_SUGGEST, s_name_suggest ? 1 : 0);
        nvs_commit(h);
        nvs_close(h);
    }
}

/* ---- Active language layouts ----
 * Toggled from the F1 -> Settings menu and persisted in NVS. Selects
 * which of the compiled-in keyboard layouts (see kb_layout.h) Ctrl+L /
 * Win+Space rotate through -- a build with several layouts enabled in
 * Kconfig does not force every user to page through all of them just
 * to get back to the one or two they actually use. Defaults to US
 * only (see kb_layout.cpp's default_active_mask()). The mask itself is
 * owned by kb_layout.cpp; this is just the NVS load/save wrapper. */
/* The bit order of the mask follows the compiled-in layouts, so each
 * language pack keeps its own key: flashing a different pack must not
 * reinterpret the other pack's bits. */
#if defined(CONFIG_DRAFTLING_LANG_PACK_VIETNAMESE)
#define NVS_KEY_ACTIVE_LAYOUTS "actlay_vi"
#elif defined(CONFIG_DRAFTLING_LANG_PACK_KOREAN)
#define NVS_KEY_ACTIVE_LAYOUTS "actlay_ko"
#elif defined(CONFIG_DRAFTLING_LANG_PACK_JAPANESE)
#define NVS_KEY_ACTIVE_LAYOUTS "actlay_ja"
#elif defined(CONFIG_DRAFTLING_LANG_PACK_CHINESE)
#define NVS_KEY_ACTIVE_LAYOUTS "actlay_zh"
#elif defined(CONFIG_DRAFTLING_LANG_PACK_INDIC)
#define NVS_KEY_ACTIVE_LAYOUTS "actlay_in"
#elif defined(CONFIG_DRAFTLING_LANG_PACK_ALL)
#define NVS_KEY_ACTIVE_LAYOUTS "actlay_all"
#define NVS_ACTIVE_LAYOUTS_U64 1   /* more than 32 layouts */
#else
#define NVS_KEY_ACTIVE_LAYOUTS "actlayouts"
#endif

static void load_active_layouts_from_nvs(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS_EDITOR, NVS_READONLY, &h) == ESP_OK) {
#if defined(NVS_ACTIVE_LAYOUTS_U64)
        uint64_t v = 0;
        if (nvs_get_u64(h, NVS_KEY_ACTIVE_LAYOUTS, &v) == ESP_OK) {
            kb_layout_set_active_mask(v);
        }
#else
        /* The other packs have at most 32 layouts: the key keeps the
         * 32-bit value it always had. */
        uint32_t v = 0;
        if (nvs_get_u32(h, NVS_KEY_ACTIVE_LAYOUTS, &v) == ESP_OK) {
            kb_layout_set_active_mask(v);
        }
#endif
        nvs_close(h);
    }
}

static void save_active_layouts_to_nvs(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS_EDITOR, NVS_READWRITE, &h) == ESP_OK) {
#if defined(NVS_ACTIVE_LAYOUTS_U64)
        nvs_set_u64(h, NVS_KEY_ACTIVE_LAYOUTS, kb_layout_get_active_mask());
#else
        nvs_set_u32(h, NVS_KEY_ACTIVE_LAYOUTS, (uint32_t)kb_layout_get_active_mask());
#endif
        nvs_commit(h);
        nvs_close(h);
    }
}

/* The layout selected with Ctrl+L / Win+Space / F1 -> Keyboard, kept
 * across restarts and deep sleep. Stored by its short name, not its id:
 * the ids follow the compiled-in layouts and differ between language
 * packs. */
#define NVS_KEY_CUR_LAYOUT "curlayout"

static void load_current_layout_from_nvs(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS_EDITOR, NVS_READONLY, &h) != ESP_OK) return;
    char name[16];
    size_t len = sizeof(name);
    if (nvs_get_str(h, NVS_KEY_CUR_LAYOUT, name, &len) == ESP_OK) {
        for (int i = 0; i < KB_LAYOUT_COUNT; i++) {
            if (strcmp(kb_layout_name((kb_layout_id_t)i), name) == 0) {
                kb_layout_select((kb_layout_id_t)i);
                break;
            }
        }
    }
    nvs_close(h);
}

/* Call after anything that may change the current layout; writes only
 * when it differs from what was saved last. */
static void save_current_layout_to_nvs(void)
{
    static kb_layout_id_t s_saved = KB_LAYOUT_COUNT;
    kb_layout_id_t cur = kb_layout_get();
    if (cur == s_saved) return;
    nvs_handle_t h;
    if (nvs_open(NVS_NS_EDITOR, NVS_READWRITE, &h) == ESP_OK) {
        if (nvs_set_str(h, NVS_KEY_CUR_LAYOUT, kb_layout_name(cur)) == ESP_OK &&
            nvs_commit(h) == ESP_OK)
            s_saved = cur;
        nvs_close(h);
    }
}

/* ---- Screen margins ----
 * Pixels of physical panel hidden under an opaque bezel/enclosure
 * cover on each edge (main/app_config.h's DISPLAY_LOGICAL_WIDTH/
 * HEIGHT and this file's SCR_W/SCR_H both subtract these). Zero by
 * default on every board; adjustable here. The LVGL display
 * resolution is fixed for the life of the session, so a change only
 * takes effect on the next restart -- these four locals track the
 * *pending* (NVS-persisted) value for display/cycling, distinct from
 * display_margin_*()'s frozen, currently-active value (see
 * display_margins.h). */
static int s_margin_left = 0, s_margin_right = 0, s_margin_top = 0, s_margin_bottom = 0;

/* Pending display orientation (Settings -> Display orientation). Like
 * the margins, this is frozen for the session (see
 * display_orientation.h); this local tracks the NVS-persisted value
 * that will apply on the next restart. */
static bool s_pending_portrait = false;

/* Pending "Display upside down" flag (Settings -> Display upside
 * down). Like the orientation above, this is frozen for the session
 * (see display_flip.h); this local tracks the NVS-persisted value
 * that will apply on the next restart. */
static bool s_pending_upside_down = false;

/* Pending "2x scale" flag (Settings -> 2x scale). Frozen for the
 * session like the orientation (see display_scale.h); this local
 * tracks the NVS-persisted value that will apply on the next restart. */
static bool s_pending_scale2x = false;

/* Set whenever a restart-only display setting -- a margin, the
 * display orientation, the upside-down flip or the 2x scale -- is
 * changed in Settings -> Display; drives the "restart now?" prompt
 * shown on the way out of that level (see settings_go_back() /
 * s_restart_confirm_open below), since that is the earliest point a
 * restart can be applied without yanking the user out of the middle of
 * adjusting settings. */
static bool s_restart_needed = false;

/* Full-takeover confirmation shown by settings_go_back() when
 * leaving Settings -> Display with a pending restart-only change --
 * same one-widget-list-reused pattern as the color-theme picker below.
 * sel 0 = restart now, 1 = keep editing and apply later. */
static bool s_restart_confirm_open     = false;
static int  s_restart_confirm_sel      = 0;
static int  s_restart_confirm_sel_prev = -1;

static void load_margins_from_nvs(void)
{
    display_margins_get_pending(&s_margin_left, &s_margin_right,
                                &s_margin_top, &s_margin_bottom);
}

/* Cycle steps for each margin: 0-40 px in 2 px increments. Covers the
 * range seen on real enclosures (the Xteink X4 Pro needed 12/8) with
 * enough headroom for a wider bezel, without an unwieldy number of
 * Enter presses per step. */
#define MARGIN_OPTION_COUNT 21
static const int MARGIN_OPTIONS[MARGIN_OPTION_COUNT] = {
    0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20,
    22, 24, 26, 28, 30, 32, 34, 36, 38, 40
};

static int find_margin_option(int px)
{
    for (int i = 0; i < MARGIN_OPTION_COUNT; i++) {
        if (MARGIN_OPTIONS[i] == px) return i;
    }
    return 0;
}

/* True when the cursor sits at the very end of the active document's
 * text (the only place where insertion is allowed while append-only
 * editing is active). */
static bool cursor_at_doc_end(void)
{
    return editor_get_cursor() >= editor_get_length();
}

/* Recalculate derived layout values from the current body font. */
static void recalc_layout(void)
{
    const lv_font_t *bf = body_font();
    s_line_h = lv_font_get_line_height(bf);
    s_char_w = char_width_for_font(bf);
}

/* Return the index into FONT_SIZE_OPTIONS for the given pixel size.
 * Falls back to index 0 (the smallest offered size) when the size is
 * not recognized. */
static int find_font_size_option(int sz)
{
    for (int i = 0; i < FONT_SIZE_COUNT; i++) {
        if (FONT_SIZE_OPTIONS[i] == sz) return i;
    }
    return 0;
}

static void load_font_size_from_nvs(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS_EDITOR, NVS_READONLY, &h) == ESP_OK) {
        uint8_t val = 0;
        if (nvs_get_u8(h, NVS_KEY_FONTSZ, &val) == ESP_OK) {
            /* Accept the persisted value only if it is one of the
             * sizes actually offered on this board (the option list
             * differs between standard and HIDPI builds). */
            bool valid = false;
            for (int i = 0; i < FONT_SIZE_COUNT; i++) {
                if (FONT_SIZE_OPTIONS[i] == val) { valid = true; break; }
            }
            if (valid) s_font_size = val;
        }
        nvs_close(h);
    }
}

static void save_font_size_to_nvs(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS_EDITOR, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_u8(h, NVS_KEY_FONTSZ, (uint8_t)s_font_size);
        nvs_commit(h);
        nvs_close(h);
    }
}

/* LVGL objects.
 *
 * The per-document editor widgets (content container, cursor bar,
 * "draftling" placeholder, line-label / selection-rectangle pools and
 * their render caches) live in the pane_t pool defined further below
 * and are reached through file-scope aliasing macros (s_cont_edit,
 * s_cursor, s_img_logo, s_line_labels, ...). Everything that is shared
 * across panes (the screen, title / status bars, file browser) stays a
 * real global here. */
static lv_obj_t *s_scr       = NULL;
static lv_obj_t *s_lbl_title = NULL;
#define TITLE_W (CONTENT_W - 4 - HDR_IN_L - HDR_IN_R)   /* title label width, set in build_screens() */
static lv_obj_t *s_lbl_status= NULL;
static lv_obj_t *s_scr_browser = NULL;
static lv_obj_t *s_list_files  = NULL;
static lv_obj_t *s_lbl_br_status = NULL;
/* Divider line between the two editor panes (split mode only). */
static lv_obj_t *s_pane_divider = NULL;

/* One-shot LVGL timer that restores the status bar to its standard
 * text 3 seconds after a transient message (e.g. "File too large")
 * was posted via editor_ui_set_status(). NULL when no message is
 * currently pending auto-clear. Runs inside lv_timer_handler() and
 * therefore does not need explicit draftling_lvgl_port_lock(). */
static lv_timer_t *s_status_clear_timer = NULL;

/* BLE connection prompt screen */
static lv_obj_t *s_scr_ble_prompt  = NULL;
static lv_obj_t *s_ble_prompt_lbl  = NULL;

/* Menu overlay objects */
static lv_obj_t *s_scr_menu     = NULL;
static lv_obj_t *s_menu_list    = NULL;
static lv_obj_t *s_lbl_menu_hdr = NULL;
static int       s_menu_sel     = 0;
/* Previously highlighted menu item, so update_list_highlight() can
 * clear just the old selection on Up/Down navigation instead of
 * re-styling every item (which would invalidate the entire menu list
 * and trigger a full e-paper refresh on the PaperS3 backend). */
static int       s_menu_sel_prev = -1;
static bool      s_menu_open    = false;

/* The F1 menu is a tree of levels (menu_level_t). Each level is a list
 * of rows identified by a menu_item_t id; refresh_menu_items() builds
 * the rows of the current level into s_menu_items[] (rows that do not
 * apply to the board, or to the current state, are left out), so the
 * row index of an item varies -- always go through s_menu_items[]. */
enum menu_level_t {
    MENU_LVL_ROOT = 0,
    MENU_LVL_SYNC,
    MENU_LVL_CONN,
    MENU_LVL_BLE,      /* under Connectivity */
    MENU_LVL_WIFI,     /* under Connectivity */
    MENU_LVL_SYSTEM,
};

enum menu_item_t {
    /* Top level */
    MENU_ITEM_SETTINGS = 0,
    MENU_ITEM_KB_LAYOUT,     /* only with more than one active layout */
    MENU_ITEM_SYNC_MENU,
    MENU_ITEM_CONN_MENU,
    MENU_ITEM_SYSTEM_MENU,
    MENU_ITEM_SLEEP,
    MENU_ITEM_BOOKS,         /* "Open a book..." */
    MENU_ITEM_QUIT,          /* CONFIG_DRAFTLING_HAS_QUIT */
    MENU_ITEM_HELP,
    MENU_ITEM_CLOSE,         /* the top level's counterpart of "Back" */
    /* Sync */
    MENU_ITEM_GIT_SYNC,
    MENU_ITEM_DRAFTBOX,
    MENU_ITEM_KERMIT,        /* CONFIG_DRAFTLING_HAS_KERMIT */
    /* Connectivity */
    MENU_ITEM_BLE_MENU,
    MENU_ITEM_WIFI_MENU,
    /* Connectivity -> Bluetooth */
    MENU_ITEM_BLE_STATUS,
    MENU_ITEM_BLE_SCAN,
    /* Connectivity -> WiFi */
    MENU_ITEM_WIFI_STATUS,
    MENU_ITEM_WIFI_TOGGLE,   /* connect to the configured SSID / disconnect */
    MENU_ITEM_WIFI_NEW,
    /* System & Storage */
    MENU_ITEM_HWINFO,
    MENU_ITEM_USB_MSC,       /* CONFIG_DRAFTLING_HAS_USB_MSC */
    MENU_ITEM_FORMAT_SD,
    /* Every level below the top one ends with it */
    MENU_ITEM_BACK,
};

#define MENU_MAX_ITEMS  12
#define MENU_MAX_DEPTH  4

static menu_level_t s_menu_level = MENU_LVL_ROOT;
static menu_item_t  s_menu_items[MENU_MAX_ITEMS];
static int          s_menu_item_count = 0;
/* Selection of each parent level, restored on the way back up. */
static int          s_menu_sel_stack[MENU_MAX_DEPTH];
static int          s_menu_depth = 0;

/* Header line of the F1 menu screen (s_lbl_menu_hdr) on the top level;
 * sub-levels name their path, and the "Format SD card" and "Sync via
 * browser" pickers replace it with their question while open. */
#define MENU_HDR_TEXT "Menu - Up/Down, Enter to select, Esc to close"

/* "Format SD card" picker: three steps rendered into s_menu_list (like
 * the "SD card via USB" picker), with the question in the menu header.
 * FMT_STEP_WIFI is skipped when no WiFi network is configured. */
typedef enum {
    FMT_STEP_NONE = 0,   /* picker closed */
    FMT_STEP_WIFI,       /* keep or erase the WiFi settings */
    FMT_STEP_CONFIRM1,   /* first confirmation */
    FMT_STEP_CONFIRM2,   /* second confirmation */
} fmt_step_t;
static fmt_step_t s_fmt_step     = FMT_STEP_NONE;
static int        s_fmt_sel      = 0;
static int        s_fmt_sel_prev = -1;
static bool       s_fmt_keep_wifi = false;
/* Credentials read before formatting, written back to wifi.cfg after
 * it when s_fmt_keep_wifi is set. Wiped when the picker closes. */
static char       s_fmt_ssid[33];
static char       s_fmt_pass[65];

#if defined(CONFIG_DRAFTLING_HAS_USB_MSC)
/* "SD card via USB" picker + deferred-apply state. Selecting the item
 * opens a 3-row picker (Off / Read-only / Read-write, mirroring the
 * color-theme picker's s_settings_list reuse, but built into
 * s_menu_list instead since this item lives on the top-level F1 menu
 * rather than in Settings). Picking a row stages it in
 * s_usbmsc_pending_mode; the actual usb_msc_set_mode() call happens
 * once the user leaves the picker itself, Enter or Esc (see
 * close_usbmsc_picker()), not the whole F1 menu. */
static bool          s_usbmsc_picker_open = false;
static int           s_usbmsc_picker_sel  = 0;
static int           s_usbmsc_picker_sel_prev = -1;
static usb_msc_mode_t s_usbmsc_pending_mode = USB_MSC_MODE_OFF;

static const char *usb_msc_mode_name(usb_msc_mode_t mode)
{
    switch (mode) {
    case USB_MSC_MODE_READ_ONLY:  return "Read-only";
    case USB_MSC_MODE_READ_WRITE: return "Read-write";
    default:                      return "Off";
    }
}

/* The F1 menu item is disabled (shown as unavailable, Enter is a
 * no-op) only while it is off AND no SD card is mounted -- there is
 * nothing yet for usb_msc to expose, and usb_msc_set_mode() would
 * just fail with ESP_ERR_INVALID_STATE. Once a session is already
 * running it stays selectable regardless of card state, so a card
 * pulled out mid-session can still be switched back off. */
static bool usbmsc_item_disabled(void)
{
#if defined(CONFIG_DRAFTLING_STORAGE_FLASH)
    /* usb_msc exposes a card's sectors; there is no card. */
    return true;
#endif
    return usb_msc_get_mode() == USB_MSC_MODE_OFF && !sd_card_is_ready();
}
#endif

#if defined(CONFIG_DRAFTLING_HAS_KERMIT)
/* "Sync via browser" picker (Start / Stop), rendered into s_menu_list
 * like the "SD card via USB" one. The choice applies at once -- see
 * kermit_picker_activate(). While running, the Kermit server
 * (components/kermit) owns the serial console and the SD card. */
static bool s_kermit_picker_open     = false;
static int  s_kermit_picker_sel      = 0;
static int  s_kermit_picker_sel_prev = -1;
/* True while the message pop-up is the "Sync via browser" dialog: it
 * stays up for the whole Kermit session and its only action is
 * Finished (Enter or a tap), which stops the server. */
static bool s_msg_kermit             = false;
#endif
/* The pop-up lists documents found outside any project: Enter (or a
 * tap) asks for the name of a new project to move them to, Esc powers
 * the device off (deep sleep); it asks again on the next start. */
static bool s_msg_migrate            = false;

/* ---- Books (EPUB), see editor_path_is_book() ----
 *
 * The book browser (F1 -> "Open a book...", B in the file browser)
 * lists every folder of the card -- projects or not -- and the .epub
 * files, in s_menu_list; the contents list (C while reading) lists the
 * chapters of the book being read. */
static bool                  s_books_open = false;
static char                  s_books_dir[256] = "";   /* below the mount point, "" = root */
static sd_card_file_entry_t *s_books_list = NULL;
static int                   s_books_count = 0;
static int                  *s_books_rows = NULL;     /* entry per row, -1 = ".." */
static int                   s_books_nrows = 0;
static int                   s_books_sel = 0, s_books_sel_prev = 0;
static bool                  s_toc_open = false;
static int                   s_toc_sel = 0, s_toc_sel_prev = 0;
#define TOC_MAX_ROWS   512    /* chapters listed */
#define BOOKS_MAX_ROWS 512    /* entries of a folder listed */

static bool open_path_in_editor(const char *path);

static void books_free(void)
{
    sd_card_free_list(s_books_list);
    s_books_list = NULL;
    s_books_count = 0;
    free(s_books_rows);
    s_books_rows = NULL;
    s_books_nrows = 0;
}
/* The pop-up says the SD card is missing. It cannot be closed: the user
 * inserts a card, then Enter (or a tap) restarts the device. */
static bool s_msg_restart            = false;

/* Why the firmware must keep off the SD card right now -- it is handed
 * to a USB host or to the Kermit server -- or NULL if it may use it. */
static const char *sd_handed_off_msg(void)
{
#if defined(CONFIG_DRAFTLING_HAS_USB_MSC)
    if (usb_msc_get_mode() != USB_MSC_MODE_OFF)
        return "SD card via USB active -- disable it in F1 menu first";
#endif
#if defined(CONFIG_DRAFTLING_HAS_KERMIT)
    if (kermit_server_is_running())
        return "Sync via browser active";
#endif
    return NULL;
}

#if !defined(CONFIG_DRAFTLING_DISPLAY_EPD)
static lv_timer_t *s_blink_timer = NULL;
#endif
static bool s_cursor_visible = true;
/* True when editor_ui_refresh() positioned the cursor inside the
 * visible editor area on the last render. Per-pane (stored in pane_t)
 * and reached through the s_cursor_on_screen alias. This is independent
 * of LV_OBJ_FLAG_HIDDEN, which on LCD backends is also toggled by the
 * blink timer (cursor_blink_cb) and would otherwise make visual
 * Up/Down think the cursor is off-screen mid-blink and fall back
 * to logical-line jumps. */
static int  s_browser_sel    = 0;
/* See s_menu_sel_prev. */
static int  s_browser_sel_prev = -1;
static int  s_browser_count  = 0;
/* Every entry of the folder shown (sd_card_list_dir(); NULL when
 * empty), replaced by each refresh_file_list(). */
static sd_card_file_entry_t *s_browser_entries = NULL;

/* ---- Projects (see projects.h) ----
 * The file browser has two levels: the list of projects (the folders
 * in the SD card root) and the documents of one project.
 * s_cur_project is the project the browser shows ("" = the list of
 * projects); s_last_project is the project the user last worked in,
 * where untitled documents are saved and the browser reopens. */
static project_name_t s_cur_project  = "";
static project_name_t s_last_project = "";
/* The first editor_ui_show_file_browser() opens the last project. */
static bool s_browser_started = false;
/* The boot-time "create a project" / "move the root files" prompt was
 * dismissed with Esc; not asked again until the next boot. */
static bool s_setup_dismissed = false;

/* Settings screen objects */
static lv_obj_t *s_scr_settings  = NULL;
static lv_obj_t *s_settings_list = NULL;
static lv_obj_t *s_lbl_settings_hdr = NULL;
static int       s_settings_sel  = 0;
/* See s_menu_sel_prev above. */
static int       s_settings_sel_prev = -1;
static bool      s_settings_open = false;
static bool      s_factory_reset_confirm = false; /* awaiting second Enter */

#if defined(CONFIG_DRAFTLING_DISPLAY_COLOR)
/* Theme picker overlay (shares s_scr_settings / s_settings_list,
 * shown in place of the regular settings list while open). */
static bool s_theme_picker_open      = false;
static int  s_theme_picker_sel       = 0;
static int  s_theme_picker_sel_prev  = -1;
#endif

/* Active-language-layouts picker overlay (shares s_scr_settings /
 * s_settings_list, shown in place of the regular settings list while
 * open). One checklist row per compiled-in layout plus a "Done" row;
 * Enter toggles the highlighted layout in/out of the active set
 * immediately (no separate confirm step, mirroring Append-only /
 * Rotate / Invert). */
static bool s_layouts_picker_open      = false;
static int  s_layouts_picker_sel       = 0;
static int  s_layouts_picker_sel_prev  = -1;

/* Passkey overlay objects */
static lv_obj_t *s_passkey_panel = NULL;
static lv_obj_t *s_passkey_label = NULL;

/* Save-prompt overlay objects */
static lv_obj_t *s_save_panel    = NULL;
static lv_obj_t *s_save_hdr_lbl  = NULL;
static lv_obj_t *s_save_name_lbl = NULL;
static lv_obj_t *s_save_cur      = NULL;   /* blinking cursor in name field */
static bool      s_save_open     = false;
static char      s_save_buf[128] = "";     /* editable filename (no directory) */
static int       s_save_pos      = 0;      /* cursor position in s_save_buf (byte) */
/* The same overlay asks for other names too (s_save_mode): renaming the
 * file browser's selected file (F2 or Alt+R; s_rename_src then holds
 * its full path), and the project prompts. The panel is moved onto
 * whichever screen is showing. */
typedef enum {
    SAVE_MODE_SAVE_AS,         /* save the document under a name */
    SAVE_MODE_RENAME,          /* rename the selected file */
    SAVE_MODE_PROJECT_NEW,     /* Ctrl+N in the list of projects */
    SAVE_MODE_PROJECT_FIRST,   /* no project on the card yet */
    SAVE_MODE_PROJECT_MIGRATE, /* documents found in the root folder */
    SAVE_MODE_PROJECT_RENAME,  /* rename the selected project */
    SAVE_MODE_XFER_PROJECT,    /* new project for Alt+M / Alt+C */
    SAVE_MODE_XFER_NAME,       /* file name for Alt+M / Alt+C */
} save_mode_t;
static save_mode_t s_save_mode = SAVE_MODE_SAVE_AS;
#define s_save_is_rename (s_save_mode == SAVE_MODE_RENAME)

/* A name typed in another script (Cyrillic, Hebrew, Devanagari, ...)
 * travels badly between file systems and Git hosts, so the name prompt
 * switches to a Latin layout while it is open (kb_layout_force_latin())
 * and gives the previous one back when it closes. The switch is not
 * saved to NVS. */
static bool s_save_layout_forced = false;
static kb_layout_id_t s_save_prev_layout;

static void update_title_bar(void);

static void save_prompt_force_latin(void)
{
    if (s_save_layout_forced) return;
    s_save_prev_layout = kb_layout_force_latin();
    s_save_layout_forced = true;
    update_title_bar();
}

static void save_prompt_restore_layout(void)
{
    if (!s_save_layout_forced) return;
    s_save_layout_forced = false;
    kb_layout_restore(s_save_prev_layout);
    update_title_bar();
}
static char      s_rename_src[512] = "";
/* Return to the in-pane (split) selector rather than the full-screen
 * browser after a project prompt raised from it. */
static bool      s_save_from_inpane = false;

/* ---- Move / copy a file to a project (Alt+M / Alt+C) ----
 * A project picker on the menu screen (s_menu_list), like the Format SD
 * card picker; then, when needed, the name prompt above. */
static bool           s_xfer_open = false;
static bool           s_xfer_copy = false;
static bool           s_xfer_from_inpane = false;
static int            s_xfer_sel = 0;
static int            s_xfer_sel_prev = -1;
static project_name_t s_xfer_src_project = "";
static char           s_xfer_src_name[256] = "";
static project_name_t s_xfer_dst_project = "";
static project_name_t *s_xfer_projects = NULL;  /* malloc'ed, see xfer_set_projects() */
static int            s_xfer_nprojects = 0;  /* rows before "New project..." */
/* The picker moves the files found outside any project (see
 * check_project_setup()) instead of one file. */
static bool           s_xfer_migrate = false;
/* Set while that picker hands over to the browser, so the browser does
 * not raise the "Files outside projects" dialog again in between. */
static bool           s_setup_busy = false;

/* ---- WiFi "New connection" scan/pick flow ----
 * Selecting the F1 -> Connectivity -> WiFi -> "Choose network..." row
 * (MENU_ITEM_WIFI_NEW) triggers a background scan and then reuses s_menu_list to show the
 * results, mirroring the "SD card via USB" mode picker's reuse of the
 * same list widget. Picking an open network connects immediately;
 * picking a secured one raises a small password-entry overlay (its
 * own top-level modal, parented to s_scr_menu, structurally a copy of
 * the save-as prompt's header/value-label/cursor trio) before
 * connecting. A successful connect is written to /sdcard/wifi.cfg so
 * it is picked up automatically by Ctrl+W / boot-time reconnect
 * afterwards -- see wifi_manager_save_to_file(). */
static bool      s_wifi_scan_picker_open     = false;
static bool      s_wifi_scan_in_progress     = false;
static int       s_wifi_scan_picker_sel      = 0;
static int       s_wifi_scan_picker_sel_prev = -1;
static wifi_scan_result_t s_wifi_scan_results[WIFI_SCAN_MAX_RESULTS];
static int       s_wifi_scan_count           = 0;
/* The picker always appends a trailing "Rescan" row after the SSID
 * list (or after the "No networks found" placeholder row when there
 * are no results) -- see wifi_scan_rescan_row(). */

static lv_obj_t *s_wifi_pw_panel    = NULL;
static lv_obj_t *s_wifi_pw_hdr_lbl  = NULL;
static lv_obj_t *s_wifi_pw_name_lbl = NULL;   /* shows the password as typed */
static lv_obj_t *s_wifi_pw_cur      = NULL;
static bool      s_wifi_pw_open     = false;
static char      s_wifi_pw_ssid[33] = "";     /* SSID this password is for */
static char      s_wifi_pw_buf[65]  = "";     /* matches wifi_config_t.sta.password size */
static int       s_wifi_pw_pos      = 0;      /* byte cursor into s_wifi_pw_buf */
/* A network whose saved password (wifi_scan_result_t::known) was
 * refused: the next pick of it asks for the password. */
static char      s_wifi_known_refused[33] = "";

/* ---- "Connect a Draftbox repository" prompt ----
 * F1 menu row MENU_ITEM_DRAFTBOX. A two-step single-line prompt on the
 * menu screen, laid out like the WiFi password overlay. If WiFi is not
 * connected yet, the panel first connects it (DBX_STEP_WIFI, like
 * Ctrl+W) and moves on once wifi_state_cb() reports the result. Then
 * the server choice (DBX_STEP_SERVER: draftbox.art or a custom
 * server), for a custom server its site name (DBX_STEP_SITE, whatever
 * was typed last time), then the 8-digit one-time password. Enter on the
 * password runs git_sync_draftbox_connect() on a background task,
 * which exchanges the password for an access token and writes the
 * clone URL and token into git.cfg; the result goes to the status bar. */
#define DBX_STEP_SITE 0
#define DBX_STEP_OTP  1
#define DBX_STEP_WIFI 2   /* waiting for the WiFi connection */
#define DBX_STEP_SERVER 3 /* draftbox.art or a custom server */
#define DBX_DEFAULT_SITE "draftbox.art"
static lv_obj_t *s_dbx_panel    = NULL;
static lv_obj_t *s_dbx_hdr_lbl  = NULL;
static lv_obj_t *s_dbx_val_lbl  = NULL;
static lv_obj_t *s_dbx_cur      = NULL;
static bool      s_dbx_open     = false;
static int       s_dbx_step     = DBX_STEP_SITE;
static char      s_dbx_site[128] = "";       /* custom server name */
static bool      s_dbx_custom   = false;     /* custom server chosen */
static int       s_dbx_site_pos = 0;
static char      s_dbx_otp[10]  = "";        /* 8 digits; see text_field_key() */
static int       s_dbx_otp_pos  = 0;

/* ---- Message pop-up ----
 * A modal panel with a title, a word-wrapped message and a "close" hint,
 * moved onto the active screen when shown (like the delete prompt).
 * Used for results that are too important for the status bar, e.g. the
 * Draftbox connection. While s_msg_busy is set (an operation is still
 * running) the hint is hidden and every key is ignored; the operation
 * replaces the text with its result and clears the flag. */
static lv_obj_t *s_msg_panel     = NULL;
static lv_obj_t *s_msg_title_lbl = NULL;
static lv_obj_t *s_msg_text_lbl  = NULL;
static lv_obj_t *s_msg_hint_lbl  = NULL;
static bool      s_msg_open      = false;
static bool      s_msg_busy      = false;

/* ---- Search / Replace overlay ----
 * A single panel handles both Ctrl+F (find) and Ctrl+H (find +
 * replace). When `s_search_replace_mode` is true, the panel exposes
 * a second "Replace" field; Tab toggles focus between the two
 * fields, Enter performs Find Next, Ctrl+Enter performs Replace +
 * Find Next, and Esc closes the dialog. */
static lv_obj_t *s_search_panel    = NULL;
static lv_obj_t *s_search_hdr_lbl  = NULL;
static lv_obj_t *s_search_find_hdr = NULL;
static lv_obj_t *s_search_find_lbl = NULL;
static lv_obj_t *s_search_repl_hdr = NULL;
static lv_obj_t *s_search_repl_lbl = NULL;
static lv_obj_t *s_search_cur      = NULL;
static lv_obj_t *s_search_help_lbl = NULL;
static bool      s_search_open     = false;
static bool      s_search_replace_mode = false;
static int       s_search_field    = 0;    /* 0 = find, 1 = replace */
static char      s_search_buf[128] = "";
static int       s_search_pos      = 0;    /* byte cursor into s_search_buf */
static char      s_replace_buf[128] = "";
static int       s_replace_pos     = 0;    /* byte cursor into s_replace_buf */
/* Range of the most recent match in the document; used by Replace
 * to know what to substitute. -1 means "no match selected". */
static int       s_search_match_start = -1;
static int       s_search_match_end   = -1;

/* ---- Exit / sleep prompt overlay ----
 * Shown when the editor has unsaved changes and the user asks to leave
 * it: Esc (-> file browser) or Ctrl+P / F1 "Sleep now" (-> deep sleep).
 * Offers three choices: save, proceed without saving, or cancel and
 * keep editing. Navigated with Up/Down + Enter; Esc cancels.
 *
 * The same LVGL objects serve both destinations; s_exit_purpose picks
 * which one the Save / Discard rows lead to, and the row labels are set
 * to match when the overlay is shown. */
typedef enum {
    EXIT_PURPOSE_BROWSER,   /* Esc: leave the editor for the file browser */
    EXIT_PURPOSE_SLEEP,     /* Ctrl+P / "Sleep now": enter deep sleep */
    EXIT_PURPOSE_QUIT,      /* "Quit Draftling" (CONFIG_DRAFTLING_HAS_QUIT) */
} exit_purpose_t;

static lv_obj_t *s_exit_panel    = NULL;
static lv_obj_t *s_exit_hdr_lbl  = NULL;
static lv_obj_t *s_exit_opt_lbl[3] = { NULL, NULL, NULL };
static bool      s_exit_open     = false;
static int       s_exit_sel      = 0;      /* 0=Save 1=proceed-no-save 2=Cancel */
static exit_purpose_t s_exit_purpose = EXIT_PURPOSE_BROWSER;

#define EXIT_OPT_SAVE   0
#define EXIT_OPT_DISCARD 1
#define EXIT_OPT_CANCEL 2
#define EXIT_OPT_COUNT  3

static const char *const EXIT_OPT_LABELS[EXIT_OPT_COUNT] = {
    "Save and exit",
    "Exit without saving",
    "Cancel (keep editing)",
};

static const char *const SLEEP_OPT_LABELS[EXIT_OPT_COUNT] = {
    "Save and sleep",
    "Sleep without saving",
    "Cancel (keep editing)",
};

#if defined(CONFIG_DRAFTLING_HAS_QUIT)
static const char *const QUIT_OPT_LABELS[EXIT_OPT_COUNT] = {
    "Save and quit",
    "Quit without saving",
    "Cancel (keep editing)",
};
#endif

/* ---- New-document format picker ----
 * Ctrl+N (and N in the file browser) asks which format the new
 * untitled document is written in. The overlay is moved onto the
 * active screen when shown: the editor screen for Ctrl+N in the editor
 * and the split-mode in-pane selector, the browser screen otherwise. */
typedef enum {
    NEWDOC_BROWSER,   /* full-screen file browser: replace the document */
    NEWDOC_INPANE,    /* split-mode in-pane selector: the focused pane */
    NEWDOC_EDITOR,    /* Ctrl+N in the editor: the focused pane */
} newdoc_target_t;

#define NEWFMT_COUNT 3
static lv_obj_t *s_newfmt_panel = NULL;
/* Name-suggestion drop-down (editor screen, see suggest_update()) */
static lv_obj_t *s_sugg_panel = NULL;
static lv_obj_t *s_sugg_lbl[FMT_SUGGEST_MAX];
static void suggest_hide(void);
static lv_obj_t *s_newfmt_opt_lbl[NEWFMT_COUNT] = { NULL, NULL, NULL };
static bool      s_newfmt_open  = false;
static int       s_newfmt_sel   = 0;
static newdoc_target_t s_newfmt_target = NEWDOC_BROWSER;

static const editor_doc_format_t NEWFMT_FORMATS[NEWFMT_COUNT] = {
    EDITOR_DOC_MARKDOWN, EDITOR_DOC_FOUNTAIN, EDITOR_DOC_TEXT,
};
static const char *const NEWFMT_LABELS[NEWFMT_COUNT] = {
    "Markdown (.md)",
    "Fountain screenplay (.fountain)",
    "Plain text (.txt)",
};

/* ---- Delete-file confirmation ----
 * Del or Alt+D in the file browser. Only offered for a file whose
 * current content is already on the Git server (git_sync_file_status()),
 * so a deletion can always be undone from the repository history. The
 * overlay is moved onto the active screen when shown, like the format
 * picker above. */
#define DELCONF_DELETE 0
#define DELCONF_CANCEL 1
#define DELCONF_COUNT  2
static lv_obj_t *s_del_panel   = NULL;
static lv_obj_t *s_del_hdr_lbl = NULL;
static lv_obj_t *s_del_opt_lbl[DELCONF_COUNT] = { NULL, NULL };
static bool      s_del_open    = false;
static int       s_del_sel     = DELCONF_CANCEL;
static char      s_del_path[512] = "";
/* Row to select after the deletion (the neighbouring file). */
static char      s_del_next_name[256] = "";
/* The prompt deletes the selected project (s_del_path is its folder). */
static bool      s_del_is_project = false;

/* ---- Help screen ----
 * F10 (or F1 -> Help) shows a scrollable page with the shortcuts of the
 * current mode -- the editor or the file browser -- and, in the editor,
 * the formatting cheat sheet of the focused document's format (see
 * editor_format_t::help). Up/Down/PgUp/PgDn/Home/End scroll; Esc or
 * Enter return to where the help was opened from. F1 -> "Hardware
 * information" shows its page (show_hwinfo()) on the same screen. */
typedef enum {
    HELP_FROM_EDITOR,
    HELP_FROM_BROWSER,   /* full-screen file browser */
    HELP_FROM_INPANE,    /* split-mode in-pane file selector */
} help_origin_t;

static lv_obj_t *s_scr_help      = NULL;
static lv_obj_t *s_lbl_help_hdr  = NULL;
static lv_obj_t *s_help_cont     = NULL;   /* scrollable body */
static lv_obj_t *s_help_lbl      = NULL;
static bool      s_help_open     = false;
static help_origin_t s_help_origin = HELP_FROM_EDITOR;

/* BOARD_NAME from the board header, handed over by main.cpp
 * (editor_ui_set_board_name()) for the hardware information page. */
static const char *s_board_name  = NULL;

/* Radio co-processor (editor_ui_set_coprocessor()); NULL = none. */
static const char *s_coproc_chip = NULL;
static const char *s_coproc_fw   = NULL;

/* ---- Device battery display ----
 *
 * Boards that have a wired-up battery monitor (BATT_ADC_PIN >= 0)
 * show a percentage in the status bar of both the editor and the
 * file browser. Each board defines this in app_config.h:
 *   - Waveshare RLCD-4.2:   GPIO4, 3:1 divider
 *   - M5Stack PaperS3:      GPIO3, 2:1 divider (no enable) */
#if defined(CONFIG_DRAFTLING_HAS_BATTERY)
#define DRAFTLING_HAS_BATT_INDICATOR 1
static lv_obj_t  *s_lbl_dev_batt    = NULL;  /* editor screen */
static lv_obj_t  *s_lbl_br_dev_batt = NULL;  /* file browser screen */
static lv_obj_t  *s_lbl_ble_dev_batt = NULL; /* BLE prompt screen */
/* On the original H752 the FastEPD panel ghosts/flickers when the
 * battery label is rewritten on a periodic timer, because every
 * rewrite drives an e-paper refresh. On that board only, switch to a
 * "pull" model: no timer; the label is refreshed at natural redraw
 * points via sync_battery_labels(), and only when the value actually
 * changed. Every other board keeps the periodic timer below (cheap on
 * colour LCD; the epdiy e-paper boards are unaffected per testing). */
#if defined(CONFIG_DRAFTLING_MODEL_LILYGO_T5_EPD_S3_PRO_H752)
#define DRAFTLING_BATT_PULL_MODE 1
#else
static lv_timer_t *s_batt_timer     = NULL;
#define BATT_POLL_MS 5000  /* refresh battery every 5 s (so charging
                              * state / plug-in events show up quickly
                              * on backends that expose it, INA226 on
                              * the M5Stack Tab5 in particular) */
#endif
#endif

/* ---- WiFi connectivity icon ----
 * Shown in the right corner of both status bars whenever the WiFi
 * stack reports STATE_CONNECTED. The Greybeard fonts do not cover
 * U+1F6DC, so we render the symbol from a small embedded LVGL
 * image (see wifi_icon.c). */
static lv_obj_t *s_img_wifi    = NULL;  /* editor screen */
static lv_obj_t *s_img_br_wifi = NULL;  /* file browser screen */

/* ---- Hide status bars while typing ----
 * Toggled from F1 -> Settings -> Editor and persisted in NVS; on by
 * default. While on, the editor screen's title bar and status bar
 * (labels, separator rules, battery and WiFi icon) disappear at the
 * first key the editor handles and come back once no key has been
 * pressed for s_bars_idle_s seconds. A status message (Saved, Git
 * progress, input-method candidates, ...) brings them back at once.
 * The text area keeps its size, so nothing reflows when the bars
 * come and go. The bars are made transparent rather than flagged
 * hidden, because other code already toggles LV_OBJ_FLAG_HIDDEN on
 * the status label and the WiFi icon. */
#define NVS_KEY_HIDE_BARS   "hidebars"
#define NVS_KEY_BARS_IDLE   "barsidle"
static bool     s_hide_bars   = true;
static uint16_t s_bars_idle_s = 30;
static const uint16_t BARS_IDLE_OPTIONS[] = { 15, 30, 60, 120 };
#define BARS_IDLE_OPTION_COUNT 4
static bool        s_bars_hidden = false;   /* bars currently hidden */
static lv_timer_t *s_bars_timer  = NULL;    /* idle timer, paused when unused */
static lv_obj_t   *s_editor_hline = NULL;   /* title bar separator rule */
static lv_obj_t   *s_editor_sline = NULL;   /* status bar separator rule */
static lv_obj_t   *s_br_sline = NULL;       /* file browser status bar rule */

static int find_bars_idle_option(uint16_t s)
{
    for (int i = 0; i < BARS_IDLE_OPTION_COUNT; i++)
        if (BARS_IDLE_OPTIONS[i] == s) return i;
    return 1;   /* 30 s */
}

static void bars_relayout(void);
static int sprint_title_words(void);

/* The active layout that shows its input method's text in the status
 * bar (ime_uses_status(): Japanese, Chinese, Taravinyas, Telugu RTS),
 * or KB_LAYOUT_COUNT when none of the active layouts does. */
static kb_layout_id_t status_ime_layout(void)
{
#ifdef DRAFTLING_HAS_IME
    for (int i = 0; i < KB_LAYOUT_COUNT; i++) {
        kb_layout_id_t id = (kb_layout_id_t)i;
        if (kb_layout_is_active(id) && ime_uses_status(kb_layout_ime_of(id))) return id;
    }
#endif
    return KB_LAYOUT_COUNT;
}

/* The bars stay while the title bar shows the writing sprint's word
 * count (Ctrl+T, "Show word count"), and while one of the active
 * layouts types through the status bar (its candidates or the letters
 * being composed must stay visible). */
static bool bars_may_hide(void)
{
    return s_hide_bars && sprint_title_words() < 0 &&
           status_ime_layout() == KB_LAYOUT_COUNT;
}

static void bars_set_hidden(bool hidden)
{
    s_bars_hidden = hidden;
    lv_opa_t opa = hidden ? LV_OPA_TRANSP : LV_OPA_COVER;
    lv_obj_t *objs[] = {
        s_lbl_title, s_lbl_status, s_img_wifi,
#if defined(CONFIG_DRAFTLING_HAS_BATTERY)
        s_lbl_dev_batt,
#endif
    };
    for (size_t i = 0; i < sizeof(objs) / sizeof(objs[0]); i++) {
        if (objs[i]) lv_obj_set_style_opa(objs[i], opa, 0);
    }
    /* The rules are plain (clickable) objects lying over the expanded
     * text area: hide them outright so they do not swallow taps. */
    lv_obj_t *rules[] = { s_editor_hline, s_editor_sline };
    for (size_t i = 0; i < 2; i++) {
        if (!rules[i]) continue;
        if (hidden) lv_obj_add_flag(rules[i], LV_OBJ_FLAG_HIDDEN);
        else        lv_obj_remove_flag(rules[i], LV_OBJ_FLAG_HIDDEN);
    }
    bars_relayout();
}

/* Set by bars_typing_begin() while a key is being handled: the bars
 * are hidden after the key unless the key posted a status message. */
static bool s_bars_hide_pending = false;

/* A keyboard layout switch flashes the bars, so the new layout tag in
 * the title bar can be seen: bars_flash_request() during the key makes
 * bars_typing_end() show them for BARS_FLASH_MS, then hide them again
 * (the idle countdown keeps running meanwhile). */
#define BARS_FLASH_MS 500
static bool        s_bars_flash_req   = false;
static lv_timer_t *s_bars_flash_timer = NULL;   /* paused when unused */

static void bars_flash_request(void)
{
    /* The title bar shows the layout tag only with several active. */
    if (kb_layout_active_count() > 1) s_bars_flash_req = true;
}

static void bars_flash_stop(void)
{
    s_bars_flash_req = false;
    if (s_bars_flash_timer) lv_timer_pause(s_bars_flash_timer);
}

static void bars_flash_timer_cb(lv_timer_t *timer)
{
    lv_timer_pause(timer);
    if (!bars_may_hide() || s_bars_hidden) return;
    bars_set_hidden(true);
#if defined(CONFIG_DRAFTLING_DISPLAY_EPD)
    /* Refresh the whole changed area, not a typing clip. */
    display_set_partial_clip(0, 0, 0, 0);
#endif
}

/* Show the bars and stop the idle timer (status message, idle
 * timeout, setting switched off). */
static void bars_show(void)
{
    s_bars_hide_pending = false;
    bars_flash_stop();
    if (s_bars_timer) lv_timer_pause(s_bars_timer);
    if (s_bars_hidden) bars_set_hidden(false);
}

static void bars_idle_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    bars_show();
}

/* A key reached the editor: schedule hiding the bars for when the key
 * is done (bars_typing_end()). */
static void bars_typing_begin(void)
{
    s_bars_hide_pending = bars_may_hide();
}

/* After the key: hide the bars unless the key posted a status message,
 * and restart the idle countdown. Returns true when the bars changed
 * from shown to hidden. */
static bool bars_typing_end(void)
{
    bool flash = s_bars_flash_req;
    bars_flash_stop();
    if (!s_bars_hide_pending) return false;
    s_bars_hide_pending = false;
    uint32_t ms = (uint32_t)s_bars_idle_s * 1000U;
    if (!s_bars_timer) {
        s_bars_timer = lv_timer_create(bars_idle_timer_cb, ms, NULL);
        if (!s_bars_timer) return false;
    } else {
        lv_timer_set_period(s_bars_timer, ms);
    }
    lv_timer_reset(s_bars_timer);
    lv_timer_resume(s_bars_timer);
    if (flash) {
        if (!s_bars_flash_timer) {
            s_bars_flash_timer = lv_timer_create(bars_flash_timer_cb,
                                                 BARS_FLASH_MS, NULL);
        }
        if (s_bars_flash_timer) {
            lv_timer_reset(s_bars_flash_timer);
            lv_timer_resume(s_bars_flash_timer);
            if (!s_bars_hidden) return false;
            bars_set_hidden(false);
            return true;
        }
    }
    if (s_bars_hidden) return false;
    bars_set_hidden(true);
    return true;
}

static void load_hide_bars_from_nvs(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS_EDITOR, NVS_READONLY, &h) == ESP_OK) {
        uint8_t v = 1;
        if (nvs_get_u8(h, NVS_KEY_HIDE_BARS, &v) == ESP_OK) {
            s_hide_bars = (v != 0);
        }
        uint16_t s = 30;
        if (nvs_get_u16(h, NVS_KEY_BARS_IDLE, &s) == ESP_OK) {
            s_bars_idle_s = BARS_IDLE_OPTIONS[find_bars_idle_option(s)];
        }
        nvs_close(h);
    }
}

static void save_hide_bars_to_nvs(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS_EDITOR, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_u8(h, NVS_KEY_HIDE_BARS, s_hide_bars ? 1 : 0);
        nvs_set_u16(h, NVS_KEY_BARS_IDLE, s_bars_idle_s);
        nvs_commit(h);
        nvs_close(h);
    }
}

#if defined(CONFIG_DRAFTLING_DISPLAY_HOST_ROTATE)
#if !defined(CONFIG_DRAFTLING_DISPLAY_RESIZABLE)
#error "DRAFTLING_DISPLAY_HOST_ROTATE needs DRAFTLING_DISPLAY_RESIZABLE"
#endif
/* ---- Orientation turned by the host ----
 * The web client's page turns its whole screen a quarter turn when the
 * user wants landscape on a portrait screen (or the other way round),
 * and half a turn for Rotate 180; the editor only sees a window of a
 * new size. Settings -> Display, applied at once. */
#define NVS_KEY_HOST_ORIENT "horient"
#define NVS_KEY_HOST_ROT180 "hrot180"
static uint8_t s_host_orient;     /* EDITOR_ORIENT_* */
static bool    s_host_rot180;

static const char *host_orient_name(int o)
{
    switch (o) {
    case EDITOR_ORIENT_LANDSCAPE: return "landscape";
    case EDITOR_ORIENT_PORTRAIT:  return "portrait";
    default:                      return "as the screen";
    }
}

static void load_host_rotation_from_nvs(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS_EDITOR, NVS_READONLY, &h) == ESP_OK) {
        uint8_t v = 0;
        if (nvs_get_u8(h, NVS_KEY_HOST_ORIENT, &v) == ESP_OK && v <= EDITOR_ORIENT_PORTRAIT)
            s_host_orient = v;
        if (nvs_get_u8(h, NVS_KEY_HOST_ROT180, &v) == ESP_OK)
            s_host_rot180 = v != 0;
        nvs_close(h);
    }
}

static void save_host_rotation_to_nvs(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS_EDITOR, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_u8(h, NVS_KEY_HOST_ORIENT, s_host_orient);
        nvs_set_u8(h, NVS_KEY_HOST_ROT180, s_host_rot180 ? 1 : 0);
        nvs_commit(h);
        nvs_close(h);
    }
}

extern "C" __attribute__((weak)) void editor_host_rotation_changed(int orient, bool rotate180)
{
    (void)orient;
    (void)rotate180;
}
#endif  /* CONFIG_DRAFTLING_DISPLAY_HOST_ROTATE */

#if defined(CONFIG_DRAFTLING_DISPLAY_ROUND_CORNERS)
#if !defined(CONFIG_DRAFTLING_DISPLAY_RESIZABLE)
#error "DRAFTLING_DISPLAY_ROUND_CORNERS needs DRAFTLING_DISPLAY_RESIZABLE (editor_ui_can_relayout())"
#endif
/* ---- Rounded screen corners ----
 * A phone's screen has rounded corners, which cut into the title and
 * status bars when the web client runs in full screen. The user picks
 * the corner radius (Settings -> Display -> "Rounded screen corners",
 * in the host's units: CSS pixels in a browser); the host says which
 * corners of the canvas are corners of the screen and how many canvas
 * pixels a unit is (editor_ui_set_screen_corners()). The bars then
 * grow to cover those corners, so every row of the text area keeps the
 * full width, and the bar text moves in from the corners. */
#define NVS_KEY_CORNERS "corners"
static const uint8_t CORNER_OPTIONS[] = { 0, 16, 24, 32, 40, 48, 56, 64 };
#define CORNER_OPTION_COUNT ((int)(sizeof(CORNER_OPTIONS) / sizeof(CORNER_OPTIONS[0])))
/* A text row may lose this many pixels at its ends to a corner: the
 * text area starts where the corner takes no more. */
#define CORNER_SLACK 2
static uint8_t  s_corner_size;            /* the setting, 0 = off */
static unsigned s_corner_mask;            /* EDITOR_CORNER_* from the host */
static int      s_corner_milli = 1000;    /* canvas pixels per unit x 1000 */

static int find_corner_option(int v)
{
    for (int i = 0; i < CORNER_OPTION_COUNT; i++) {
        if (CORNER_OPTIONS[i] == v) return i;
    }
    return 0;
}

static void load_corners_from_nvs(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS_EDITOR, NVS_READONLY, &h) == ESP_OK) {
        uint8_t v = 0;
        if (nvs_get_u8(h, NVS_KEY_CORNERS, &v) == ESP_OK)
            s_corner_size = CORNER_OPTIONS[find_corner_option(v)];
        nvs_close(h);
    }
}

static void save_corners_to_nvs(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS_EDITOR, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_u8(h, NVS_KEY_CORNERS, s_corner_size);
        nvs_commit(h);
        nvs_close(h);
    }
}

/* Pixels a corner of radius r takes from a row y pixels from its edge. */
static int corner_clip(int r, int y)
{
    if (r <= 0 || y >= r) return 0;
    double d = (double)(r - (y < 0 ? 0 : y));
    return (int)ceil((double)r - sqrt((double)r * r - d * d));
}

/* Rows from the edge until a corner of radius r takes at most
 * CORNER_SLACK pixels from a row. */
static int corner_band(int r)
{
    if (r <= CORNER_SLACK) return 0;
    const double sl = CORNER_SLACK;
    return (int)ceil((double)r - sqrt(2.0 * r * sl - sl * sl));
}

/* Bar heights, insets and bands for the current setting and host
 * geometry. */
static void corners_compute(void)
{
    int r = 0;
    if (s_corner_size && s_corner_mask) {
        r = (int)((long)s_corner_size * s_corner_milli / 1000 / display_scale_factor());
    }
    const int r_tl = (s_corner_mask & EDITOR_CORNER_TOP_LEFT)     ? r : 0;
    const int r_tr = (s_corner_mask & EDITOR_CORNER_TOP_RIGHT)    ? r : 0;
    const int r_bl = (s_corner_mask & EDITOR_CORNER_BOTTOM_LEFT)  ? r : 0;
    const int r_br = (s_corner_mask & EDITOR_CORNER_BOTTOM_RIGHT) ? r : 0;

    s_top_clear = std::max(corner_band(r_tl), corner_band(r_tr));
    s_bot_clear = std::max(corner_band(r_bl), corner_band(r_br));
    s_hdr_h = std::max(BASE_HEADER_H, s_top_clear);
    s_sta_h = std::max(BASE_STATUS_H, s_bot_clear);

    /* The title text sits at the bottom of its bar (HDR_TEXT_Y), the
     * status text at the top of its own, two pixels below the rule. */
    const int hdr_text_top = s_hdr_h - BASE_HEADER_H;
    s_hdr_in_l = corner_clip(r_tl, hdr_text_top);
    s_hdr_in_r = corner_clip(r_tr, hdr_text_top);
    const int sta_text_bottom = s_sta_h - 2 - (int)lv_font_get_line_height(FONT_11);
    s_sta_in_l = corner_clip(r_bl, sta_text_bottom);
    s_sta_in_r = corner_clip(r_br, sta_text_bottom);
}

/* Lay the screens out again when the corner geometry changed. False
 * while a dialog is open (nothing changed then; ask again later). */
static bool corners_apply(void)
{
    const int old[] = { s_hdr_h, s_sta_h, s_hdr_in_l, s_hdr_in_r,
                        s_sta_in_l, s_sta_in_r, s_top_clear, s_bot_clear };
    corners_compute();
    const int now[] = { s_hdr_h, s_sta_h, s_hdr_in_l, s_hdr_in_r,
                        s_sta_in_l, s_sta_in_r, s_top_clear, s_bot_clear };
    if (memcmp(old, now, sizeof(old)) == 0 || !s_scr) return true;
    if (!editor_ui_can_relayout()) {
        s_hdr_h = old[0]; s_sta_h = old[1];
        s_hdr_in_l = old[2]; s_hdr_in_r = old[3];
        s_sta_in_l = old[4]; s_sta_in_r = old[5];
        s_top_clear = old[6]; s_bot_clear = old[7];
        return false;
    }
    rebuild_screens_for_theme();
#ifdef DRAFTLING_HAS_IME
    ime_set_status_cells(status_avail_width() / char_width_for_font(FONT_11));
#endif
    return true;
}

/* A header label that has no width of its own: keep it clear of the
 * right-hand corner. */
static void corner_fit_header(lv_obj_t *lbl)
{
    lv_obj_set_width(lbl, SCR_W - 4 - HDR_IN_L - HDR_IN_R);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_CLIP);
}

/* The host's side of the setting: nothing by default (the web client
 * fits its own key bar to the corners, web_client/src/io_web.c). */
extern "C" __attribute__((weak)) void editor_host_corners_changed(int size)
{
    (void)size;
}

extern "C" bool editor_ui_set_screen_corners(unsigned mask, int px_per_unit_milli)
{
    s_corner_mask = mask;
    s_corner_milli = px_per_unit_milli > 0 ? px_per_unit_milli : 1000;
    return corners_apply();
}
#else
#define corner_fit_header(lbl) ((void)(lbl))
#endif /* CONFIG_DRAFTLING_DISPLAY_ROUND_CORNERS */

/* ---- Key-event input queue ----
 * The BLE callback runs on the HID host task and must not block on the
 * LVGL mutex.  Key events are enqueued here from the BLE context (ISR-
 * safe) and drained by an LVGL timer running on the GUI task. */
#define KEY_QUEUE_LEN       CONFIG_DRAFTLING_KEY_QUEUE_LEN
#define KEY_DRAIN_PERIOD_MS CONFIG_DRAFTLING_KEY_DRAIN_PERIOD_MS
static QueueHandle_t s_key_queue = NULL;
static lv_timer_t   *s_key_drain_timer = NULL;

/* Maximum number of auto-generated draft filenames (draft_001..draft_999) */
#define MAX_DRAFT_SEQ 999

/* ---- Key repeat ----
 * When a key is held down the BLE HID report keeps sending the same
 * keycodes, but some keyboards only send a single report.  We implement
 * software key repeat: after an initial delay (KEY_REPEAT_DELAY_MS) we
 * start re-injecting the last key-down event at KEY_REPEAT_RATE_MS
 * intervals.  The repeat is cancelled on key-up. */
#define KEY_REPEAT_DELAY_MS   500
#define KEY_REPEAT_RATE_MS    50
/* Safety net for a key-up that never arrives -- e.g. BLE boot-protocol
 * report diffing (see process_key_array() in ble_keyboard.cpp) cannot
 * always tell "still held" apart from "released and something else
 * changed", so a lost key-up is a real, observed failure mode, not
 * theoretical. Without this, a stuck-held key repeats forever: every
 * KEY_REPEAT_RATE_MS re-injection on an e-paper board also triggers a
 * display refresh, so a stuck key can permanently saturate the system
 * and silently starve all later real keystrokes from ever being
 * processed -- confirmed on real hardware as the editor's content
 * freezing indefinitely (repeated identical redraws in the log, but
 * the text the user kept typing afterward never appears at all). No
 * human intentionally holds a key this long while typing. */
#define KEY_REPEAT_MAX_MS     5000
static kb_event_t s_repeat_ev    = {};      /* last key-down event */
static bool       s_repeat_held  = false;   /* is a key currently held? */
static uint32_t   s_repeat_start = 0;       /* tick when key was pressed */
static bool       s_repeat_firing = false;  /* past initial delay? */
static lv_timer_t *s_repeat_timer = NULL;

/* Standby timeout options in seconds: 0=Off, 300=5min, 600=10min, etc. */
static const uint32_t TIMEOUT_OPTIONS[] = { 0, 300, 600, 900, 1800, 3600 };
static const char *TIMEOUT_LABELS[]     = { "Off", "5 min", "10 min",
                                            "15 min", "30 min", "60 min" };
#define TIMEOUT_OPTION_COUNT  6

/* Line label pool. Sized to fill the editor body on the smaller
 * panels in portrait orientation (Settings -> Display orientation),
 * where the tall side becomes the editor height -- e.g. the Xteink X4
 * Pro is ~19 body rows tall in landscape but ~33 in portrait. The
 * largest panels (Tab5, 7" Sunton) still cap the visible-line count in
 * portrait, the same way they already do in landscape. Each extra slot
 * costs one lazily-created LVGL label per pane plus a small struct
 * entry -- see pane_t. */
#define MAX_LINE_LABELS 40

/* A "line" here is one source paragraph (run between '\n's) which can
 * be arbitrarily long, so the rendered text for a slot must NOT be
 * capped to a fixed size. Earlier revisions used a 1024-byte buffer
 * and silently dropped trailing UTF-8 codepoints once a single
 * paragraph filled it -- e.g. a 569-Cyrillic-char paragraph fits in
 * exactly 1023 bytes and the next typed character (any 2-byte glyph)
 * was discarded by the codepoint trimmer. We now build the per-line
 * display string into a growable std::string and cache it the same
 * way, so the cap is bounded only by available heap. */
/* ---- Editor panes ----
 *
 * The editor body can be shown as one pane (the historical default) or
 * split into two side-by-side (left / right) panes, each bound to its
 * own editor_doc_t. Every per-document widget and render-cache slot
 * lives in a pane_t. The large body of rendering, cursor, selection and
 * touch code below was written against file-scope globals (s_cont_edit,
 * s_cursor, s_line_labels[], s_prev_line_*[], ...). Rather than thread
 * a pane pointer through dozens of helpers, those names are #defined as
 * aliases onto the fields of the "current" pane s_rp -- mirroring the
 * editor_doc_t aliasing trick in editor.cpp. Code that operates on a
 * specific pane binds it first with pane_bind().
 *
 * Each pane keeps its own MAX_LINE_LABELS-slot label pool and
 * per-slot render cache. A split layout halves each pane's width
 * but keeps the full editor height, and each pane wraps text to its
 * own (narrower) width.
 *
 * The per-slot render cache lets editor_ui_refresh() skip LVGL
 * mutations whenever a slot's visible text / style / position are
 * unchanged, which keeps the e-paper dirty region small enough for a
 * fast partial refresh instead of a full-screen flash. */
/* ---- WYSIWYG line views ----
 *
 * Each rendered body line is shown with its Markdown markup applied
 * rather than as raw text: heading, blockquote and code-fence markers,
 * the "**" / "*" / "_" / "~~" / "`" around inline spans and the "---"
 * of a horizontal rule are hidden, and the styled characters are drawn
 * bold / slanted / struck through / boxed. The one exception is the
 * line holding the focused pane's cursor, whose markers stay visible
 * so they can be seen and edited ("live preview", as in Obsidian or
 * Typora); its styling is still applied.
 *
 * The lv_label per line is kept (its letter-position and hit-test APIs
 * drive the cursor, selection and touch code); the styling is painted
 * on top of it from an LV_EVENT_DRAW_MAIN_END handler
 * (line_deco_draw_cb()). There are no bold or italic font assets:
 * bold is a second copy of each glyph one pixel to the right, and
 * italic re-draws each glyph as horizontal bands shifted progressively
 * further right towards the top -- both work for every font family,
 * size and script without adding flash.
 *
 * Because markers can be hidden, the label text no longer maps 1:1 to
 * the raw line. line_view_t::d2r records, for every display character,
 * the raw byte offset it came from; view_raw_to_disp() /
 * view_disp_to_raw() translate cursor, selection and touch positions
 * through it. The DECO_* decoration flags live in editor_format.h,
 * shared with the per-format hooks (md_editor.cpp, fountain_editor.cpp). */

/* A run of identically-decorated characters on one visual row, in
 * the label's content-area coordinates, laid out left to right (RTL
 * text gets one segment per character). */
typedef struct {
    int32_t  x, y, w;
    uint32_t char_idx;    /* display index of the first character */
    uint32_t byte_off;    /* its byte offset in the label text */
    uint32_t count;       /* characters in the run */
    uint8_t  flags;       /* DECO_* */
} deco_seg_t;

typedef struct {
    std::vector<uint32_t>   d2r;    /* display char -> raw byte offset */
    std::vector<uint8_t>    flags;  /* DECO_* per display char */
    std::vector<deco_seg_t> segs;   /* laid out by layout_line_decorations() */
    size_t  raw_len;                /* raw line length in bytes */
    uint8_t bullet_level;           /* list nesting, picks the bullet shape */
    bool    hr;                     /* draw a horizontal rule */
} line_view_t;

#define EDITOR_MAX_PANES 2

typedef struct {
    lv_obj_t   *cont;      /* per-pane content container */
    lv_obj_t   *cursor;    /* thin vertical cursor bar */
    lv_obj_t   *logo;      /* "draftling" placeholder when the doc is empty */
    lv_obj_t   *line_labels[MAX_LINE_LABELS];
    std::string prev_line_text[MAX_LINE_LABELS];
    int  prev_line_type[MAX_LINE_LABELS];    /* md_line_type_t, -1 if cache empty */
    int  prev_line_y[MAX_LINE_LABELS];       /* y_pos last used, -1 if cache empty */
    int  prev_line_h[MAX_LINE_LABELS];       /* rendered_h last computed */
    bool prev_line_visible[MAX_LINE_LABELS]; /* slot was visible (not hidden) */
    bool prev_line_was_selected[MAX_LINE_LABELS]; /* line intersected the selection */
    line_view_t view[MAX_LINE_LABELS];       /* display <-> raw mapping + styling */
    editor_doc_t *doc;     /* bound document (NULL until acquired) */
    int  x;                /* content-area left (screen x) */
    int  w;                /* content-area width */
    int  y;                /* content-area top (screen y) */
    int  h;                /* content-area height */
    bool cursor_on_screen; /* last refresh placed the cursor in view */

    /* Per-pane view state (cursor / scroll / selection). When the same
     * file is open in both panes they share one editor_doc_t (one
     * buffer, one set of cursor/scroll/selection fields). To keep each
     * pane's view independent, pane_bind() stashes the outgoing pane's
     * view here and restores the incoming pane's view into the shared
     * document. has_saved_view is false until the pane first adopts its
     * document's current view (so a freshly opened file keeps the cursor
     * restored from its sidecar metadata). */
    size_t saved_cursor;       /* saved cursor (logical byte offset) */
    int    saved_scroll;       /* saved scroll line */
    int    saved_sel_anchor;   /* saved selection anchor (< 0 = none) */
    bool   has_saved_view;     /* view captured at least once */

    /* Number of logical lines actually rendered on screen by the last
     * refresh_active_pane() call, i.e. how many lines fit in the pane
     * once word-wrap is accounted for. Long/wrapped lines can each
     * consume several visual rows, so this can be well below
     * VISIBLE_LINES (which assumes one visual row per logical line).
     * Page Up/Down use it to size a "page" jump to what is actually on
     * screen instead of overshooting past the end of a short,
     * long-line document. */
    int    visible_line_count;

    /* Pixels of the first rendered line (the scroll line) hidden above
     * the pane's top edge. Scrolling is otherwise by whole logical
     * lines, which cannot follow the cursor through a single paragraph
     * taller than the pane: once that paragraph is the top line there
     * is no further line to scroll to. top_px only applies while the
     * pane still shows the same document from the same scroll line it
     * was set for (top_px_doc / top_px_line), so any line-granular
     * scroll implicitly resets it to 0. See pane_top_px(). */
    int           top_px;
    int           top_px_line;
    editor_doc_t *top_px_doc;
    /* top_px was set by bars_relayout() to keep the text in place when
     * the status bars hid / showed: it need not be a whole number of
     * visual rows, so refresh_active_pane() leaves it alone. */
    bool          top_px_free;

    /* Blank pixels above the scroll line, under the same validity rule
     * as top_px (lead_line / lead_doc). Set by bars_relayout() when the
     * status bars hide with the document's first line at the top: no
     * text above to fill the title bar's place, so the text starts
     * lower instead of moving up. */
    int           lead_px;
    int           lead_line;
    editor_doc_t *lead_doc;

    /* The format's scan state (Markdown code fence, Fountain element)
     * after lines 0..scan_ck_line-1 of scan_ck_doc, so a refresh does
     * not re-parse every line above the screen. Valid while nothing
     * before scan_ck_off (the start of that line) changed -- see
     * editor_change_mark_get() -- and, since a Fountain line's element
     * depends on whether the cursor is on the next line, while the
     * cursor stays after scan_ck_off. */
    editor_doc_t          *scan_ck_doc;
    const editor_format_t *scan_ck_fmt;
    int                    scan_ck_line;
    size_t                 scan_ck_off;
    bool                   scan_ck_in_code;
    ftn_state_t            scan_ck_ftn;

    /* The same state every SCAN_TAB_STEP lines (entry k: line
     * (k + 1) * SCAN_TAB_STEP), so that a scroll up -- PgUp -- starts
     * from the nearest entry above instead of line 0, which took
     * seconds on a long document. Same validity rules as the
     * checkpoint; scan_tab_n entries are valid. */
    struct scan_tab_ent {
        int         line;
        size_t      off;
        bool        in_code;
        ftn_state_t ftn;
    }                     *scan_tab;
    int                    scan_tab_n, scan_tab_cap;
    editor_doc_t          *scan_tab_doc;
    const editor_format_t *scan_tab_fmt;
} pane_t;

#define SCAN_TAB_STEP 128

static_assert(EDITOR_MAX_PANES <= EDITOR_CHANGE_MARKS,
              "one editor change mark per pane");

static pane_t  s_panes[EDITOR_MAX_PANES];
static pane_t *s_rp          = &s_panes[0];  /* current render / active pane */
static int     s_pane_count  = 1;            /* 1 = single, 2 = split */
static int     s_focus       = 0;            /* focused pane index (0..count-1) */

/* See the SCROLL_JUMP_FRACTION comment above; defined here (rather
 * than alongside it) because VISIBLE_LINES dereferences s_rp. */
static inline int scroll_jump_step(void)
{
    int step = (int)(VISIBLE_LINES * SCROLL_JUMP_FRACTION);
    return step > 0 ? step : 1;
}

/* Current pane's in-line pixel offset of the scroll line (see
 * pane_t::top_px); 0 once the scroll line or document has changed. */
static int pane_top_px(void)
{
    if (s_rp->top_px <= 0) return 0;
    if (s_rp->top_px_doc != s_rp->doc ||
        s_rp->top_px_line != editor_get_scroll_line()) return 0;
    return s_rp->top_px;
}

/* Set while a touch drag pans a pane: refresh_active_pane() then leaves
 * top_px where the drag put it instead of pulling the cursor row back
 * into view. */
static bool s_touch_panning = false;

static void pane_set_top_px(int px)
{
    s_rp->top_px      = px > 0 ? px : 0;
    s_rp->top_px_line = editor_get_scroll_line();
    s_rp->top_px_doc  = s_rp->doc;
    s_rp->top_px_free = false;
}

/* Current pane's blank lead above the scroll line (pane_t::lead_px);
 * 0 once the scroll line or document has changed. */
static int pane_lead_px(void)
{
    if (s_rp->lead_px <= 0) return 0;
    if (s_rp->lead_doc != s_rp->doc ||
        s_rp->lead_line != editor_get_scroll_line()) return 0;
    return s_rp->lead_px;
}

/* Set while bars_relayout() renders a pane only to measure a line:
 * refresh_active_pane() then does not scroll to the cursor. */
static bool s_no_autoscroll = false;

/* Split layout: single pane, or a two-pane split along the display's
 * long axis with the first pane occupying 1/2, 2/3 or 1/3 of it.
 * Driven by the Ctrl+1 / Ctrl+2 / Ctrl+3 shortcuts.
 *
 * "First" is the left pane in landscape and the top pane in portrait
 * (Settings -> Display orientation) -- the split always divides the
 * long side, so a portrait screen splits into top/bottom rather than
 * left/right. The SPLIT_LEFT_* names are historical; read them as
 * "first pane = 2/3" etc. */
typedef enum {
    SPLIT_NONE = 0,   /* single pane (full size) */
    SPLIT_HALF,       /* two panes, first = 1/2 */
    SPLIT_LEFT_2_3,   /* two panes, first = 2/3 */
    SPLIT_LEFT_1_3,   /* two panes, first = 1/3 */
} split_mode_t;
static split_mode_t s_split_mode = SPLIT_NONE;

/* True while the editor screen is the active top-level screen, false
 * while the file browser is. This is the authoritative source for
 * key-event routing and refresh gating. (A per-document "mode" used to
 * be read for this; the active document is re-bound to the focused
 * pane on every cursor blink and refresh, so in split mode it could say
 * "editing" while the browser was on screen, routing keys to the
 * hidden editor.) */
static bool s_editor_screen_active = false;

/* Which pane a file-browser "open" / "new" action should load into.
 * Set by editor_ui_show_file_browser(): in single-pane mode this is
 * pane 0 (replace the current document); while split it is the
 * focused pane, so each panel opens a file for itself. */
static int     s_open_target_pane = 0;

/* In-pane file selector (split mode only).
 *
 * In single-pane mode the file browser is the full-screen s_scr_browser
 * (an lv_list named s_list_files). While the editor is split, opening a
 * file should not blank both panes; instead a compact file selector is
 * overlaid on just the focused pane so the other pane keeps showing its
 * document. s_inpane_list / s_inpane_hdr are that overlay (siblings of
 * the pane containers on s_scr, raised to the foreground while open).
 *
 * s_browser_list is the list the shared browser logic (refresh_file_list,
 * browser_activate_item, handle_browser_key) currently operates on: it
 * normally aliases s_list_files and is repointed at s_inpane_list while
 * the in-pane selector is open. */
static lv_obj_t *s_inpane_list = NULL;
static lv_obj_t *s_inpane_hdr  = NULL;
static lv_obj_t *s_browser_list = NULL;
static bool      s_inpane_browser_open = false;

/* Defined further down with the rest of the file-browser code. */
static void show_inpane_browser(void);
static void close_inpane_browser(void);
static editor_doc_t *open_into_pane(int target, const char *path);

/* Alias the historical per-document globals onto the current pane. */
#define s_cont_edit              (s_rp->cont)
#define s_cursor                 (s_rp->cursor)
#define s_img_logo               (s_rp->logo)
#define s_line_labels            (s_rp->line_labels)
#define s_prev_line_text         (s_rp->prev_line_text)
#define s_prev_line_type         (s_rp->prev_line_type)
#define s_prev_line_y            (s_rp->prev_line_y)
#define s_prev_line_h            (s_rp->prev_line_h)
#define s_prev_line_visible      (s_rp->prev_line_visible)
#define s_prev_line_was_selected (s_rp->prev_line_was_selected)
#define s_cursor_on_screen       (s_rp->cursor_on_screen)
#define s_visible_line_count     (s_rp->visible_line_count)

/* Number of logical lines to advance for a "page" jump (PgUp/PgDn,
 * Ctrl+Up/Down): the count of lines actually visible on screen right
 * now (s_visible_line_count), not VISIBLE_LINES -- VISIBLE_LINES is a
 * pixel-height / single-row-height estimate that assumes one visual
 * row per logical line, so on a document made of long, word-wrapped
 * lines it can be several times the number of lines that actually
 * fit, sending Page Down straight past the end of a short document.
 * Falls back to VISIBLE_LINES before the pane has ever rendered
 * (visible_line_count still at its zero-initialized default). */
static inline int page_line_count(void)
{
    int n = s_visible_line_count;
    return n > 0 ? n : VISIBLE_LINES;
}

/* Capture the live document's view (cursor / scroll / selection) into a
 * pane's saved view state. Used to record a pane's view from the active
 * document so a sibling pane that shares the same buffer can restore its
 * own independent view. */
static void pane_capture_view(pane_t *p)
{
    p->saved_cursor     = editor_get_cursor();
    p->saved_scroll     = editor_get_scroll_line();
    p->saved_sel_anchor = editor_get_sel_anchor();
    p->has_saved_view   = true;
}

/* Push a pane's saved view back into the (now active) shared document so
 * the document's cursor / scroll / selection reflect this pane. */
static void pane_restore_view(pane_t *p)
{
    editor_set_cursor(p->saved_cursor);
    editor_set_scroll_line(p->saved_scroll);
    editor_set_sel_anchor(p->saved_sel_anchor);
}

/* Bind pane idx as the current pane: point the aliasing macros at it
 * and make its document the engine's active document.
 *
 * Two panes that open the same file share one editor_doc_t (and thus one
 * cursor / scroll / selection). To keep each pane's view independent,
 * binding swaps view state in and out of the shared document: the
 * outgoing pane's live view is captured, then the incoming pane's saved
 * view is restored. Re-binding the already-current pane is a no-op for
 * view state (the document already holds this pane's live view). */
static void pane_bind(int idx)
{
    if (idx < 0) idx = 0;
    if (idx >= EDITOR_MAX_PANES) idx = EDITOR_MAX_PANES - 1;
    pane_t *target = &s_panes[idx];

    if (target == s_rp) {
        /* Same pane: keep the active document current. Adopt the
         * document's view the first time (e.g. just after a file was
         * opened into this pane, which reset has_saved_view). */
        if (s_rp->doc) {
            editor_set_active(s_rp->doc);
            if (!s_rp->has_saved_view) pane_capture_view(s_rp);
        }
        return;
    }

    /* Switching panes: stash the outgoing pane's live view (only valid
     * while its document is the active one). */
    if (s_rp && s_rp->doc && s_rp->doc == editor_get_active()) {
        pane_capture_view(s_rp);
    }

    s_rp = target;
    if (s_rp->doc) {
        editor_set_active(s_rp->doc);
        if (s_rp->has_saved_view) {
            pane_restore_view(s_rp);
        } else {
            /* First bind: adopt the document's current view (cursor /
             * scroll restored from its sidecar metadata on open). */
            pane_capture_view(s_rp);
        }
    }
}

/* Bind the focused pane (the one that receives keyboard input). */
static void pane_bind_focus(void) { pane_bind(s_focus); }

static void invalidate_render_cache(void)
{
    for (int i = 0; i < MAX_LINE_LABELS; i++) {
        s_prev_line_text[i].clear();
        s_prev_line_type[i]         = -1;
        s_prev_line_y[i]            = -1;
        s_prev_line_h[i]            = 0;
        s_prev_line_visible[i]      = false;
        s_prev_line_was_selected[i] = false;
    }
}

/* Wipe every pane's render cache (used on theme rebuild / font change
 * where all panes' labels were recreated). */
static void invalidate_all_render_caches(void)
{
    pane_t *save = s_rp;
    for (int p = 0; p < EDITOR_MAX_PANES; p++) {
        s_rp = &s_panes[p];
        invalidate_render_cache();
    }
    s_rp = save;
}

#if defined(CONFIG_DRAFTLING_DISPLAY_EPD)
/* Force a clean full-screen repaint on the e-paper backends.
 *
 * On EPD the panel framebuffer persists between flushes and LVGL only
 * redraws widgets whose content actually changed -- fine for incremental
 * edits, but wrong whenever the framebuffer is wiped or the widget layout
 * shifts without lv_scr_load() running a full screen redraw. In split mode
 * the editor and its in-pane file selector share one screen (s_scr), so
 * lv_scr_load(s_scr) is a no-op and no implicit full redraw happens. Two
 * symptoms result:
 *   - opening a file into one pane leaves the *other* pane blank, because
 *     display_clear() cleared its pixels but LVGL never redrew its
 *     (unchanged) labels;
 *   - collapsing a split leaves the old divider / right-pane content as
 *     ghosting, because only the changed regions were partially refreshed.
 * Clearing the framebuffer, wiping every pane's render cache and
 * invalidating the whole screen forces LVGL to repaint every widget into
 * the freshly-cleared framebuffer; display_clear() also promotes the next
 * flush to a flashing full refresh, clearing any accumulated ghosting.
 * Mirrors the font-change repaint in the F1 settings handler. */
static void epd_full_screen_repaint(void)
{
    display_clear(0xFF);
    invalidate_all_render_caches();
    if (s_scr) lv_obj_invalidate(s_scr);
}
#endif

/* First pane's share (numerator/3, or /2 for HALF) of the usable long
 * axis for the current split mode. */
static void split_first_fraction(int total, int *first)
{
    switch (s_split_mode) {
    case SPLIT_LEFT_2_3: *first = (total * 2) / 3; break;
    case SPLIT_LEFT_1_3: *first = total / 3;       break;
    case SPLIT_HALF:
    default:             *first = total / 2;       break;
    }
    if (*first < 1) *first = 1;
    if (*first > total - 1) *first = total - 1;
}

/* Recompute each pane's content-area rectangle from the current split
 * state. The editor body occupies [EDITOR_Y, EDITOR_Y + EDITOR_H). A
 * single pane fills it entirely; a two-pane split divides the display's
 * long axis with a 1 px divider between the panes:
 *
 *   - landscape: left / right panes (vary x + width, full height), so
 *     each pane wraps text to its own narrower width;
 *   - portrait (Settings -> Display orientation): top / bottom panes
 *     (vary y + height, full width), so each pane shows fewer rows.
 *
 * The first pane's share of the long axis is set by s_split_mode. */
static void recalc_pane_geometry(void)
{
    if (s_pane_count <= 1) {
        s_panes[0].x = 0;
        s_panes[0].w = CONTENT_W;
        s_panes[0].y = EDITOR_Y;
        s_panes[0].h = EDITOR_H;
        s_panes[1].x = 0;
        s_panes[1].w = 0;
        s_panes[1].y = EDITOR_Y;
        s_panes[1].h = EDITOR_H;
        return;
    }

    if (split_top_bottom()) {
        /* Top / bottom split: vary y + height, full width. */
        int total = EDITOR_H - 1;     /* 1 px reserved for the divider */
        int top_h;
        split_first_fraction(total, &top_h);
        int bot_h = total - top_h;
        s_panes[0].x = 0;
        s_panes[0].w = CONTENT_W;
        s_panes[0].y = EDITOR_Y;
        s_panes[0].h = top_h;
        s_panes[1].x = 0;
        s_panes[1].w = CONTENT_W;
        s_panes[1].y = EDITOR_Y + top_h + 1;
        s_panes[1].h = bot_h;
        return;
    }

    /* Left / right split: vary x + width, full height. */
    int total = CONTENT_W - 1;            /* 1 px reserved for the divider */
    int left_w;
    split_first_fraction(total, &left_w);
    int right_w = total - left_w;
    s_panes[0].x = 0;
    s_panes[0].w = left_w;
    s_panes[0].y = EDITOR_Y;
    s_panes[0].h = EDITOR_H;
    s_panes[1].x = left_w + 1;
    s_panes[1].w = right_w;
    s_panes[1].y = EDITOR_Y;
    s_panes[1].h = EDITOR_H;
}

/* Apply the current split geometry to the (already created) pane
 * containers and the divider line. Hides inactive panes / the divider
 * in single-pane mode. */
static void layout_panes(void)
{
    recalc_pane_geometry();
    for (int p = 0; p < EDITOR_MAX_PANES; p++) {
        if (!s_panes[p].cont) continue;
        if (p < s_pane_count) {
            lv_obj_set_pos(s_panes[p].cont, s_panes[p].x, s_panes[p].y);
            lv_obj_set_size(s_panes[p].cont, s_panes[p].w, s_panes[p].h);
            lv_obj_remove_flag(s_panes[p].cont, LV_OBJ_FLAG_HIDDEN);
            if (s_panes[p].logo)
                lv_obj_set_width(s_panes[p].logo, s_panes[p].w);
        } else {
            lv_obj_add_flag(s_panes[p].cont, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (s_pane_divider) {
        if (s_pane_count > 1) {
            if (split_top_bottom()) {
                /* Horizontal rule between the top and bottom panes. */
                lv_obj_set_pos(s_pane_divider, 0, s_panes[0].y + s_panes[0].h);
                lv_obj_set_size(s_pane_divider, CONTENT_W, 1);
            } else {
                /* Vertical rule between the left and right panes. */
                lv_obj_set_pos(s_pane_divider, s_panes[0].x + s_panes[0].w, EDITOR_Y);
                lv_obj_set_size(s_pane_divider, 1, EDITOR_H);
            }
            lv_obj_remove_flag(s_pane_divider, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_pane_divider, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

/* Styles */
static lv_style_t s_style_body;
static lv_style_t s_style_h1;
static lv_style_t s_style_h2;
static lv_style_t s_style_h3;
static lv_style_t s_style_code;
static lv_style_t s_style_quote;

static void init_styles(void)
{
    /* base_dir AUTO: LVGL's style default is LTR, which makes
     * LV_TEXT_ALIGN_AUTO always resolve to LEFT and breaks both the
     * paragraph alignment and lv_label_get_letter_pos() cursor
     * geometry for RTL text. AUTO detects the paragraph direction
     * from the first strong character, so a line starting with
     * Hebrew right-aligns (and keeps right alignment when Latin is
     * mixed in), while LTR-starting lines stay left-aligned. */
    lv_style_init(&s_style_body);
    lv_style_set_text_font(&s_style_body, body_font());
    lv_style_set_text_color(&s_style_body, theme_fg());
    lv_style_set_pad_all(&s_style_body, 0);
    lv_style_set_base_dir(&s_style_body, LV_BASE_DIR_AUTO);

    lv_style_init(&s_style_h1);
    lv_style_set_text_font(&s_style_h1, h1_font());
    lv_style_set_text_color(&s_style_h1, theme_fg());
    lv_style_set_base_dir(&s_style_h1, LV_BASE_DIR_AUTO);

    lv_style_init(&s_style_h2);
    lv_style_set_text_font(&s_style_h2, h2_font());
    lv_style_set_text_color(&s_style_h2, theme_fg());
    lv_style_set_base_dir(&s_style_h2, LV_BASE_DIR_AUTO);

    lv_style_init(&s_style_h3);
    lv_style_set_text_font(&s_style_h3, h3_font());
    lv_style_set_text_color(&s_style_h3, theme_fg());
    lv_style_set_base_dir(&s_style_h3, LV_BASE_DIR_AUTO);

    lv_style_init(&s_style_code);
    lv_style_set_text_font(&s_style_code, body_font());
    lv_style_set_text_color(&s_style_code, theme_fg());
    lv_style_set_border_width(&s_style_code, 1);
    lv_style_set_border_color(&s_style_code, theme_fg());
    lv_style_set_pad_left(&s_style_code, 4);
    lv_style_set_base_dir(&s_style_code, LV_BASE_DIR_AUTO);

    lv_style_init(&s_style_quote);
    lv_style_set_text_font(&s_style_quote, body_font());
    lv_style_set_text_color(&s_style_quote, theme_fg());
    lv_style_set_border_side(&s_style_quote, LV_BORDER_SIDE_LEFT);
    lv_style_set_border_width(&s_style_quote, 2);
    lv_style_set_border_color(&s_style_quote, theme_fg());
    lv_style_set_pad_left(&s_style_quote, 8);
    lv_style_set_base_dir(&s_style_quote, LV_BASE_DIR_AUTO);

    recalc_layout();

    /* Styles changed -> all cached label content is now stale. */
    invalidate_render_cache();
}

#if !defined(CONFIG_DRAFTLING_DISPLAY_EPD)
static void cursor_blink_cb(lv_timer_t *timer)
{
    (void)timer;
    /* The in-pane file selector covers the focused pane; suppress the
     * caret blink (which rebinds to the focused pane) while it is up. */
    if (s_inpane_browser_open) return;
    /* Blink only the focused pane's caret, and only while the refresh
     * code actually placed it on-screen (s_cursor_on_screen). */
    pane_bind_focus();
    s_cursor_visible = !s_cursor_visible;
    if (s_cursor && s_cursor_on_screen) {
        if (s_cursor_visible) lv_obj_remove_flag(s_cursor, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(s_cursor, LV_OBJ_FLAG_HIDDEN);
    }
}
#endif

#if defined(CONFIG_DRAFTLING_HAS_BATTERY)
/* Build a battery level string for the status bar. */
static void format_batt_str(char *buf, size_t len)
{
    int pct = battery_read_percent();
    if (pct < 0) {
        snprintf(buf, len, "----");
        return;
    }
    /* Prepend a "+" charge indicator when the battery is actively
     * being charged from an external supply (USB-C / DC jack). The
     * ASCII '+' is used in place of a U+26A1 lightning bolt because
     * the bundled Greybeard fonts do not cover that codepoint (see
     * update_wifi_icons() for the same reasoning). Backends that
     * cannot detect charge state return -1 and the glyph stays off. */
    int chg = battery_read_charging();
    if (chg == 1) {
        snprintf(buf, len, "+%d%%", pct);
    } else {
        snprintf(buf, len, "%d%%", pct);
    }
}

#if defined(DRAFTLING_BATT_PULL_MODE)
/* H752 pull model: cache the formatted string, and only rewrite the
 * labels when it actually changed (so a redraw point that hasn't seen
 * a battery change costs no e-paper update). */
static char s_cached_batt[20] = "";
static bool s_batt_pending = false;

static void update_battery_cache(void)
{
    char batt[20];
    format_batt_str(batt, sizeof(batt));
    if (strcmp(s_cached_batt, batt) != 0) {
        strncpy(s_cached_batt, batt, sizeof(s_cached_batt) - 1);
        s_cached_batt[sizeof(s_cached_batt) - 1] = '\0';
        s_batt_pending = true;
    }
}

static void sync_battery_labels(void)
{
    update_battery_cache();
    if (!s_batt_pending) return;
    s_batt_pending = false;
    if (s_lbl_dev_batt)    lv_label_set_text(s_lbl_dev_batt, s_cached_batt);
    if (s_lbl_br_dev_batt) lv_label_set_text(s_lbl_br_dev_batt, s_cached_batt);
    if (s_lbl_ble_dev_batt) lv_label_set_text(s_lbl_ble_dev_batt, s_cached_batt);
}

/* Paint the current value on every battery label, including ones
 * created (empty) since the last update -- see build_screens(). */
static void battery_labels_repaint(void)
{
    s_cached_batt[0] = '\0';
    sync_battery_labels();
}
#else
/* The string the battery labels currently show. */
static char s_shown_batt[20] = "";

static void batt_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    char batt[20];
    format_batt_str(batt, sizeof(batt));
    /* lv_label_set_text() dirties a label even when the new text is
     * identical to the current one (see update_title_bar()'s
     * s_prev_title comment above) -- on e-paper backends that means
     * this 5 s poll would otherwise trigger a screen refresh every
     * cycle regardless of whether the percentage actually changed.
     * On the Xteink X4 Pro's UC8179 controller a "fast" partial
     * refresh re-triggers a whole-panel activation (no windowed
     * partial on that silicon), so an unnoticed no-op battery-label
     * redraw every 5 s was quietly re-activating the entire panel in
     * the background -- visible as the screen continuing to change
     * for tens of seconds after the user stopped interacting with it.
     * Skip the call entirely when the string has not changed. */
    if (strcmp(s_shown_batt, batt) != 0) {
        if (s_lbl_dev_batt)    lv_label_set_text(s_lbl_dev_batt, batt);
        if (s_lbl_br_dev_batt) lv_label_set_text(s_lbl_br_dev_batt, batt);
        if (s_lbl_ble_dev_batt) lv_label_set_text(s_lbl_ble_dev_batt, batt);
        strncpy(s_shown_batt, batt, sizeof(s_shown_batt) - 1);
        s_shown_batt[sizeof(s_shown_batt) - 1] = '\0';
    }
}

/* Paint the current value on every battery label, including ones
 * created (empty) since the last update. Without this, a label built
 * after the last change -- the BLE-prompt screen's, which build_screens()
 * creates after the poll timer's first reading, or all of them after a
 * color-theme rebuild -- would stay blank until the percentage moved. */
static void battery_labels_repaint(void)
{
    s_shown_batt[0] = '\0';
    batt_timer_cb(NULL);
}
/* No-op on the timer-driven boards: the call sites below are shared,
 * but only the H752 pull model needs an explicit sync. */
static inline void sync_battery_labels(void) {}
#endif
#else  /* !CONFIG_DRAFTLING_HAS_BATTERY */
/* No battery indicator on this board: the shared sync_battery_labels()
 * call sites compile to nothing. */
static inline void sync_battery_labels(void) {}
static inline void battery_labels_repaint(void) {}
#endif

/* Update the WiFi connectivity icons in both status bars: shown when
 * the WiFi stack is connected, hidden otherwise. Once connected, the
 * icon itself switches to the "6" dual-stack badge (wifi6_icon_*)
 * whenever the STA interface also holds a global IPv6 address (see
 * wifi_manager_has_global_ipv6()); the two variants differ in height,
 * so the icon is re-centered vertically in the status bar every time
 * it is (re)picked here rather than only once at creation. */
static void update_wifi_icons(void)
{
    bool connected = wifi_manager_is_connected();
    bool dual_stack = connected && wifi_manager_has_global_ipv6();
    int icon_h = dual_stack ? WIFI6_ICON_H : WIFI_ICON_H;
#if defined(CONFIG_DRAFTLING_EPD_BLACK_BACKGROUND) || \
    defined(CONFIG_DRAFTLING_DISPLAY_COLOR)
    const lv_image_dsc_t *icon = dual_stack ? &wifi6_icon_white : &wifi_icon_white;
#else
    const lv_image_dsc_t *icon = dual_stack ? &wifi6_icon_black : &wifi_icon_black;
#endif
    int icon_y = SCR_H - STATUS_H + (STATUS_H - icon_h) / 2;

    if (s_img_wifi) {
        lv_image_set_src(s_img_wifi, icon);
        lv_obj_set_y(s_img_wifi, icon_y);
        if (connected) lv_obj_remove_flag(s_img_wifi, LV_OBJ_FLAG_HIDDEN);
        else           lv_obj_add_flag(s_img_wifi, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_img_br_wifi) {
        lv_image_set_src(s_img_br_wifi, icon);
        lv_obj_set_y(s_img_br_wifi, icon_y);
        if (connected) lv_obj_remove_flag(s_img_br_wifi, LV_OBJ_FLAG_HIDDEN);
        else           lv_obj_add_flag(s_img_br_wifi, LV_OBJ_FLAG_HIDDEN);
    }
}

/* Writing sprint word count for the title bar (Ctrl+T, "Show word
 * count"): -1 when not shown. Defined with the writing timer. */
static int sprint_title_words(void);

static void update_title_bar(void)
{
    const char *path = editor_get_file_path();
    const char *name;
    switch (editor_get_format()) {
    case EDITOR_DOC_FOUNTAIN: name = "Untitled screenplay"; break;
    case EDITOR_DOC_TEXT:     name = "Untitled text";       break;
    default:                  name = "Untitled";            break;
    }
    if (path) {
        const char *slash = strrchr(path, '/');
        name = slash ? slash + 1 : path;
    }
    /* A book: its title and the chapter. */
    char chapter_tag[24] = "";
    if (editor_is_read_only()) {
        if (editor_book_title()) name = editor_book_title();
        snprintf(chapter_tag, sizeof(chapter_tag), "  Ch %d/%d",
                 editor_book_chapter() + 1, editor_book_chapter_count());
    }
    char buf[384];
    int line, col;
    editor_get_cursor_pos(&line, &col);
    int total_lines = editor_get_line_count();
    /* Only worth telling the user which layout is active when there is
     * more than one in the active set to rotate through (see the
     * "Active language layouts" setting) -- with a single active
     * layout Ctrl+L has nothing to switch to, so the tag would just be
     * noise in the title bar. */
    char layout_tag[24] = "";
    if (kb_layout_active_count() > 1) {
        snprintf(layout_tag, sizeof(layout_tag), "  [%s]",
                 kb_layout_name(kb_layout_get()));
    }
    if (kb_layout_get_caps_lock()) {
        strncat(layout_tag, "  CAPS", sizeof(layout_tag) - strlen(layout_tag) - 1);
    }
    /* Words written since the writing sprint started (Ctrl+T). */
    char words_tag[24] = "";
    int sprint_words = sprint_title_words();
    if (sprint_words >= 0)
        snprintf(words_tag, sizeof(words_tag), "  %d words", sprint_words);
#if defined(CONFIG_DRAFTLING_DISPLAY_EPD)
    /* On e-paper boards the cursor moves on every keystroke, so the
     * column counter is omitted to avoid dirtying the title bar on
     * every edit. The line counter ("L %d/%d") only changes when the
     * cursor moves to a different line, so the s_prev_title cache
     * below collapses no-op redraws back to a single update. */
    char tail[96];
    snprintf(tail, sizeof(tail), "%s%s  L %d/%d%s%s",
             editor_is_modified() ? " *" : "", chapter_tag,
             line + 1, total_lines, layout_tag, words_tag);
#else
    char tail[96];
    snprintf(tail, sizeof(tail), "%s%s  L %d/%d C:%d%s%s",
             editor_is_modified() ? " *" : "", chapter_tag,
             line + 1, total_lines, col + 1, layout_tag, words_tag);
#endif

    /* A long file name would push the counters off the line (the label
     * used to wrap it over the header rule). The title font is
     * monospace, so cut the name to the characters that fit next to
     * the counters and end it with an ellipsis (U+2026, one cell; every
     * base font carries the glyph). */
    int avail = TITLE_W / char_width_for_font(FONT_11);
    int name_max = avail - utf8_cells_in_bytes(tail, strlen(tail));
    size_t name_len = strlen(name);
    if (utf8_cells_in_bytes(name, name_len) > name_max) {
        static const char ellipsis[] = "\xE2\x80\xA6";   /* U+2026 */
        int keep = name_max - 1;
        if (keep < 1) keep = 1;
        size_t keep_bytes = utf8_bytes_for_cells(name, name_len, keep);
        if (keep_bytes == 0) keep_bytes = utf8_byte_of_char(name, name_len, 1);
        snprintf(buf, sizeof(buf), "%.*s%s%s", (int)keep_bytes, name, ellipsis, tail);
    } else {
        snprintf(buf, sizeof(buf), "%s%s", name, tail);
    }

    /* lv_label_set_text() in LVGL v9 unconditionally invalidates the
     * label even when the new text is identical to the current one,
     * which on e-paper backends dirties the entire title-bar strip
     * on every keystroke.  Combined with the dirty region from the
     * edited line that spans the top and bottom of the screen and
     * triggers a full-screen refresh in the e-paper driver's ">75%"
     * huge-area path.  Compare against the last text we pushed and
     * skip the call when unchanged. */
    static char s_prev_title[sizeof(buf)] = { 0 };
    if (strcmp(s_prev_title, buf) != 0) {
        lv_label_set_text(s_lbl_title, buf);
        strncpy(s_prev_title, buf, sizeof(s_prev_title) - 1);
        s_prev_title[sizeof(s_prev_title) - 1] = '\0';
    }
}

static lv_style_t *style_for_type(md_line_type_t type)
{
    switch (type) {
    case MD_LINE_H1:           return &s_style_h1;
    case MD_LINE_H2:           return &s_style_h2;
    case MD_LINE_H3:
    case MD_LINE_H4:           return &s_style_h3;
    case MD_LINE_CODE_FENCE:
    case MD_LINE_CODE_CONTENT: return &s_style_code;
    case MD_LINE_BLOCKQUOTE:   return &s_style_quote;
    default:                   return &s_style_body;
    }
}

/* Return the monospace cell width (in pixels) for a given font slot.
 * The values are the advance widths stored in the generated font data
 * divided by 16 (LVGL stores advances in 1/16-px units). Hack (used on
 * high-density boards) and Greybeard have different per-slot widths. */
static int char_width_for_font(const lv_font_t *font)
{
#ifdef CONFIG_DRAFTLING_DISPLAY_HIDPI
    if (font == FONT_30) return 28;   /* hires_30 (47 px): adv_w 448 / 16 */
    if (font == FONT_26) return 25;   /* hires_26 (41 px): adv_w 400 / 16 */
    if (font == FONT_22) return 21;   /* hires_22 (34 px): adv_w 336 / 16 */
    if (font == FONT_18) return 17;   /* hires_18 (28 px): adv_w 272 / 16 */
    if (font == FONT_16) return 15;   /* hires_16 (25 px): adv_w 240 / 16 */
    if (font == FONT_14) return 13;   /* hires_14 (21 px): adv_w 208 / 16 */
    if (font == FONT_9)  return 10;   /* hires_9  (16 px): adv_w 160 / 16 */
    return 11;                        /* hires_11 (19 px): adv_w 176 / 16 */
#else
    if (font == FONT_34) return 18;   /* adv_w 288 / 16 = 18 */
    if (font == FONT_30) return 16;   /* adv_w 256 / 16 = 16 */
    if (font == FONT_26) return 13;   /* adv_w 208 / 16 = 13 */
    if (font == FONT_22) return 11;   /* adv_w 176 / 16 = 11 */
    if (font == FONT_18) return 9;    /* adv_w 144 / 16 = 9 */
    if (font == FONT_16) return 8;    /* adv_w 128 / 16 = 8 */
    if (font == FONT_14) return 7;    /* adv_w 112 / 16 = 7 */
    return 6;                         /* FONT_11: adv_w  96 / 16 = 6 */
#endif
}

static uint32_t utf8_decode(const char *s, size_t len, size_t *i);

/* Cells of cp between prev and next: two for the wide ones (CJK); a
 * Tamil consonant takes at most the cells of its glyph joined with the
 * vowel sign after it, which takes none (an upper bound: Tamil is
 * proportional, see components/fonts/tamil_font.h). */
static int cp_cells(uint32_t prev, uint32_t cp, uint32_t next)
{
#ifdef CONFIG_DRAFTLING_HAS_TAMIL
    int t = tamil_cp_cells(prev, cp, next);
    if (t >= 0) return t;
#else
    (void)prev;
    (void)next;
#endif
#ifdef CONFIG_DRAFTLING_HAS_INDIC_SHAPING
    int d = indic_cp_cells(cp);
    if (d >= 0) return d;
#endif
    return cjk_cp_is_wide(cp) ? 2 : 1;
}

/* Monospace cells taken by the first byte_len bytes of text: one per
 * character, two for the wide ones (CJK), Tamil by cp_cells(). */
static int utf8_cells_in_bytes(const char *text, size_t byte_len)
{
    if (byte_len == 0) return 0;
    int cells = 0;
    size_t i = 0;
    uint32_t prev = 0;
    uint32_t cp = utf8_decode(text, byte_len, &i);
    for (;;) {
        bool more = i < byte_len;
        uint32_t next = more ? utf8_decode(text, byte_len, &i) : 0;
        cells += cp_cells(prev, cp, next);
        if (!more) break;
        prev = cp;
        cp = next;
    }
    return cells;
}

/* Byte length of the longest prefix of text that fits in `cells`. */
static size_t utf8_bytes_for_cells(const char *text, size_t byte_len, int cells)
{
    size_t i = 0;
    int used = 0;
    uint32_t prev = 0;
    while (i < byte_len) {
        size_t next_i = i;
        uint32_t cp = utf8_decode(text, byte_len, &next_i);
        size_t after = next_i;
        uint32_t next = after < byte_len ? utf8_decode(text, byte_len, &after) : 0;
        int w = cp_cells(prev, cp, next);
        if (used + w > cells) break;
        used += w;
        prev = cp;
        i = next_i;
    }
    return i;
}


/* Decode the UTF-8 codepoint at s[*i] (bounded by len) and advance *i
 * past it. A malformed or truncated sequence decodes as its lead byte
 * and advances by one, so callers always make progress. */
static uint32_t utf8_decode(const char *s, size_t len, size_t *i)
{
    size_t p = *i;
    unsigned char c = (unsigned char)s[p];
    uint32_t cp;
    size_t adv;
    if (c < 0x80) {
        cp = c; adv = 1;
    } else if ((c & 0xE0) == 0xC0 && p + 1 < len) {
        cp = ((uint32_t)(c & 0x1F) << 6) |
             ((unsigned char)s[p + 1] & 0x3F);
        adv = 2;
    } else if ((c & 0xF0) == 0xE0 && p + 2 < len) {
        cp = ((uint32_t)(c & 0x0F) << 12) |
             (((unsigned char)s[p + 1] & 0x3F) << 6) |
             ((unsigned char)s[p + 2] & 0x3F);
        adv = 3;
    } else if ((c & 0xF8) == 0xF0 && p + 3 < len) {
        cp = ((uint32_t)(c & 0x07) << 18) |
             (((unsigned char)s[p + 1] & 0x3F) << 12) |
             (((unsigned char)s[p + 2] & 0x3F) << 6) |
             ((unsigned char)s[p + 3] & 0x3F);
        adv = 4;
    } else {
        cp = c; adv = 1;
    }
    *i = p + adv;
    return cp;
}

/* Bidi class of a codepoint: +1 strong LTR (Latin, Greek, Cyrillic),
 * -1 strong RTL (Hebrew, Arabic and their presentation forms), 0 for
 * direction-neutral characters (spaces, digits, punctuation). */
static int cp_strong_dir(uint32_t cp)
{
    /* Strong LTR */
    if ((cp >= 'A' && cp <= 'Z') || (cp >= 'a' && cp <= 'z')) return 1;
    if (cp >= 0x00C0 && cp <= 0x024F) return 1;  /* Latin Extended */
    if (cp >= 0x0370 && cp <= 0x03FF) return 1;  /* Greek */
    if (cp >= 0x0400 && cp <= 0x04FF) return 1;  /* Cyrillic */
    /* Strong RTL */
    if (cp >= 0x0590 && cp <= 0x05FF) return -1; /* Hebrew */
    if (cp >= 0x0600 && cp <= 0x06FF) return -1; /* Arabic */
    if (cp >= 0x0700 && cp <= 0x074F) return -1; /* Syriac */
    if (cp >= 0x0750 && cp <= 0x077F) return -1; /* Arabic Supplement */
    if (cp >= 0xFB1D && cp <= 0xFEFF) return -1; /* Hebrew + Arabic
                                                    Presentation Forms */
    return 0;
}

/* Scan UTF-8 text for the first STRONG directional codepoint,
 * mirroring LVGL's auto base-direction detection. Returns +1 for
 * strong LTR, -1 for strong RTL, 0 if the text contains only
 * direction-neutral characters (see cp_strong_dir()). */
static int utf8_first_strong_dir(const char *s, size_t len)
{
    size_t i = 0;
    while (i < len) {
        int dir = cp_strong_dir(utf8_decode(s, len, &i));
        if (dir != 0) return dir;
    }
    return 0;
}

/* ---- WYSIWYG line views (see the comment above line_view_t) ---- */

/* Byte offset of the n-th UTF-8 codepoint of s, clamped to len. */
static size_t utf8_byte_of_char(const char *s, size_t len, int n)
{
    size_t i = 0;
    while (n-- > 0 && i < len) (void)utf8_decode(s, len, &i);
    return i;
}

/* Build the display text for one raw line and its view (d2r mapping
 * and per-character styling). reveal keeps every Markdown marker
 * visible (the focused cursor line); otherwise the markers are hidden.
 * The document's format (fmt) decides which markers are hidden and
 * which line-wide styling applies; inline spans are applied here.
 * Tabs are expanded to four spaces, since the bundled fonts have no
 * tab glyph. An all-hidden line (e.g. a rule) displays as a single
 * space so the label keeps one row of height. */
static void build_line_view(const editor_format_t *fmt, const char *lt, size_t ll,
                            const md_line_info_t *mi, bool reveal, long cursor,
                            std::string &text, line_view_t *v)
{
    static std::vector<uint8_t> hide;
    static std::vector<uint8_t> rflags;
    hide.assign(ll, 0);
    rflags.assign(ll, 0);

    size_t content_off = (size_t)(mi->content - lt);
    size_t content_end = content_off + mi->content_len;
    if (content_off > ll) content_off = ll;
    if (content_end > ll) content_end = ll;

    fmt_marks_t marks = { hide.data(), rflags.data(), ll, content_off, content_end,
                          reveal ? cursor : -1, false, 0 };
    fmt->mark_line(mi, reveal, &marks);
    v->hr = marks.hr;
    v->bullet_level = marks.bullet_level;

    for (int k = 0; k < mi->span_count; k++) {
        const md_span_t *sp = &mi->spans[k];
        size_t cs = content_off + sp->start;
        size_t ce = content_off + sp->end;
        size_t m  = sp->mark_len;
        if (cs < m || ce + m > ll) continue;
        uint8_t f = (sp->bold ? DECO_BOLD : 0) | (sp->italic ? DECO_ITALIC : 0) |
                    (sp->code ? DECO_CODE : 0) | (sp->strikethrough ? DECO_STRIKE : 0) |
                    (sp->underline ? DECO_UNDER : 0);
        for (size_t b = cs; b < ce; b++) rflags[b] |= f;
        if (!reveal) {
            std::fill(hide.begin() + (cs - m), hide.begin() + cs, 1);
            std::fill(hide.begin() + ce, hide.begin() + ce + m, 1);
        }
    }

    text.clear();
    v->d2r.clear();
    v->flags.clear();
    size_t b = 0;
#ifdef CONFIG_DRAFTLING_HAS_TAMIL
    uint32_t prev_cp = 0;   /* last code point displayed */
#endif
    while (b < ll) {
        size_t start = b;
        uint32_t cp = utf8_decode(lt, ll, &b);
        if (hide[start]) continue;
#ifdef CONFIG_DRAFTLING_HAS_INDIC_SHAPING
        /* A run of Devanagari, Bengali or Telugu is shaped by HarfBuzz into the
         * private-use code points of its glyphs (indic_font.h), each
         * mapped to the first byte of the characters it shows. A run is
         * one script; the joiners and dandas go with any. */
        indic_script_t sc = indic_script(cp);
        if (sc != INDIC_NONE) {
            size_t end = b;
            while (end < ll && !hide[end]) {
                size_t nx = end;
                indic_script_t s2 = indic_script(utf8_decode(lt, ll, &nx));
                if (s2 == INDIC_NONE) break;
                if (s2 != INDIC_ANY) {
                    if (sc == INDIC_ANY) sc = s2;
                    else if (s2 != sc) break;
                }
                end = nx;
            }
            static std::vector<indic_glyph_t> dg;
            dg.resize((end - start) * 2 + 8);
            size_t n = indic_shape(sc, lt + start, end - start, dg.data(), dg.size());
            for (size_t k = 0; k < n; k++) {
                uint32_t c = dg[k].code;      /* planes 15 / 16: 4 bytes */
                char u[4] = {
                    (char)(0xF0 | (c >> 18)), (char)(0x80 | ((c >> 12) & 0x3F)),
                    (char)(0x80 | ((c >> 6) & 0x3F)), (char)(0x80 | (c & 0x3F)),
                };
                text.append(u, 4);
                size_t r = start + dg[k].cluster;
                v->d2r.push_back((uint32_t)r);
                v->flags.push_back(rflags[r < ll ? r : start]);
            }
            if (n > 0) {
#ifdef CONFIG_DRAFTLING_HAS_TAMIL
                prev_cp = 0;
#endif
                b = end;
                continue;
            }
        }
#endif
#ifdef CONFIG_DRAFTLING_HAS_TAMIL
        /* The k-ssa and sri ligatures span more code points than the
         * font sees: show each as its private-use code point, mapped
         * to the first byte of the sequence. */
        {
            uint32_t lig = 0;
            size_t L = tamil_ligature(lt + start, ll - start, &lig);
            bool shown = L > 0;
            for (size_t k = start; shown && k < start + L; k++) shown = !hide[k];
            if (shown) {
                char u[4];
                u[0] = (char)(0xE0 | (lig >> 12));
                u[1] = (char)(0x80 | ((lig >> 6) & 0x3F));
                u[2] = (char)(0x80 | (lig & 0x3F));
                text.append(u, 3);
                v->d2r.push_back((uint32_t)start);
                v->flags.push_back(rflags[start]);
                prev_cp = lig;
                b = start + L;
                continue;
            }
        }
        /* A Tamil vowel sign is drawn by the consonant before it; one
         * without a consonant gets a dotted circle to sit on (as
         * HarfBuzz does), which maps to the sign itself. */
        if (tamil_is_mark(cp) && !tamil_is_base(prev_cp)) {
            text.append("\xE2\x97\x8C");   /* U+25CC dotted circle */
            v->d2r.push_back((uint32_t)start);
            v->flags.push_back(rflags[start]);
        }
        prev_cp = cp;
#else
        (void)cp;
#endif
        if (lt[start] == '\t') {
            for (int t = 0; t < 4; t++) {
                text.push_back(' ');
                v->d2r.push_back((uint32_t)start);
                v->flags.push_back(rflags[start]);
            }
            continue;
        }
        if (rflags[start] & (DECO_BULLET | DECO_BREAK)) text.push_back(' ');
        else text.append(lt + start, b - start);
        v->d2r.push_back((uint32_t)start);
        v->flags.push_back(rflags[start]);
    }
    v->raw_len = ll;
    if (text.empty()) text = " ";
}

/* Display character index at which raw byte offset rb of the line is
 * shown. Offsets inside a hidden marker land on the next visible
 * character. */
static int view_raw_to_disp(const line_view_t *v, size_t rb)
{
    return (int)(std::lower_bound(v->d2r.begin(), v->d2r.end(), (uint32_t)rb) -
                 v->d2r.begin());
}

/* Raw byte offset for display character index d (past the end -> end
 * of the raw line, i.e. after any hidden closing marker). */
static size_t view_disp_to_raw(const line_view_t *v, int d)
{
    if (d < 0) d = 0;
    if ((size_t)d >= v->d2r.size()) return v->raw_len;
    return v->d2r[(size_t)d];
}

static void push_deco_seg(line_view_t *v, const deco_seg_t &s)
{
    if (!v->segs.empty()) {
        deco_seg_t &p = v->segs.back();
        if (p.flags == s.flags && p.y == s.y &&
            p.x + p.w == s.x && p.char_idx + p.count == s.char_idx) {
            p.w += s.w;
            p.count += s.count;
            return;
        }
    }
    v->segs.push_back(s);
}

/* The code point at byte b of txt (0 at the end), for glyph widths
 * that depend on the next character (Tamil vowel signs). */
static uint32_t cp_at(const char *txt, size_t txt_len, size_t b)
{
    return b < txt_len ? utf8_decode(txt, txt_len, &b) : 0;
}

/* The code point lv_draw_character() must draw for cp followed by
 * next: it has no look-ahead, so a Tamil consonant with a vowel sign
 * is drawn through the private-use code point of the joined glyph. */
static uint32_t deco_draw_cp(uint32_t cp, uint32_t next)
{
#ifdef CONFIG_DRAFTLING_HAS_TAMIL
    return tamil_draw_cp(cp, next);
#else
    (void)next;
    return cp;
#endif
}

/* True when the cursor sits inside a Tamil, Devanagari, Bengali or
 * Telugu syllable -- between a consonant and the vowel sign or other mark
 * drawn with it, inside a conjunct, or inside a k-ssa / sri ligature --
 * where it would be shown at the same spot as after it. Left / Right
 * step over such positions. */
static bool cursor_mid_cluster(void)
{
#if defined(CONFIG_DRAFTLING_HAS_TAMIL) || defined(CONFIG_DRAFTLING_HAS_INDIC_SHAPING)
    char buf[24];
    size_t n = editor_peek_before(buf, 12);
    size_t c = editor_get_cursor();
    size_t total = n;
    for (size_t i = 0; i < 12; i++) {
        char ch = editor_char_at(c + i);
        if (ch == '\0' || ch == '\n') break;
        buf[total++] = ch;
    }
    /* The code points right before and after the cursor. */
    uint32_t prev = 0, next = 0;
    if (n > 0) {
        size_t st = n;
        while (st > 0 && ((unsigned char)buf[st - 1] & 0xC0) == 0x80) st--;
        if (st > 0) st--;
        size_t j = st;
        prev = utf8_decode(buf, n, &j);
    }
    if (total > n) {
        size_t i = n;
        next = utf8_decode(buf, total, &i);
    }
#ifdef CONFIG_DRAFTLING_HAS_INDIC_SHAPING
    if (indic_mid_cluster(prev, next)) return true;
#endif
#ifdef CONFIG_DRAFTLING_HAS_TAMIL
    /* A mark right after the cursor, a base right before it. */
    if (tamil_is_mark(next) && tamil_is_base(prev)) return true;
    /* A ligature that starts before the cursor and ends after it. */
    for (size_t p = 0; p < n; p++) {
        if (((unsigned char)buf[p] & 0xC0) == 0x80) continue;
        uint32_t lig;
        size_t L = tamil_ligature(buf + p, total - p, &lig);
        if (L && p + L > n) return true;
    }
#endif
    return false;
#else
    return false;
#endif
}

/* Lay out the display characters [i0, i1) (first byte b0), which all
 * share flags f, as row segments. For left-to-right text the run is
 * checked against the label's own layout from its end points alone and
 * only split (recursively) where it wraps, so a run costs a few
 * lv_label_get_letter_pos() calls per visual row rather than one per
 * character. Text containing RTL characters is laid out per character:
 * LVGL reports an RTL character's position at its right edge. */
static void layout_deco_range(lv_obj_t *label, const lv_font_t *font,
                              const char *txt, size_t txt_len, bool line_rtl,
                              bool has_rtl, line_view_t *v, uint32_t i0,
                              uint32_t i1, uint32_t b0, uint8_t f)
{
    if (i0 >= i1) return;

    if (has_rtl) {
        size_t b = b0;
        for (uint32_t i = i0; i < i1; i++) {
            size_t cb = b;
            uint32_t cp = utf8_decode(txt, txt_len, &b);
            int dir = cp_strong_dir(cp);
            bool rtl = dir < 0 || (dir == 0 && line_rtl);
            int32_t gw = lv_font_get_glyph_width(font, cp, cp_at(txt, txt_len, b));
            lv_point_t p;
            lv_label_get_letter_pos(label, i, &p);
            deco_seg_t s = { rtl ? p.x - gw : p.x, p.y, gw, i, (uint32_t)cb, 1, f };
            push_deco_seg(v, s);
        }
        return;
    }

    lv_point_t p0;
    lv_label_get_letter_pos(label, i0, &p0);
    int32_t w = 0, last_w = 0;
    size_t b = b0, mid_b = b0;
    uint32_t mid = i0 + (i1 - i0) / 2;
    for (uint32_t i = i0; i < i1; i++) {
        if (i == mid) mid_b = b;
        uint32_t cp = utf8_decode(txt, txt_len, &b);
        last_w = lv_font_get_glyph_width(font, cp, cp_at(txt, txt_len, b));
        w += last_w;
    }
    bool one_row = (i1 - i0 == 1);
    if (!one_row) {
        lv_point_t p1;
        lv_label_get_letter_pos(label, i1 - 1, &p1);
        one_row = (p1.y == p0.y && p1.x + last_w - p0.x == w);
    }
    if (one_row) {
        deco_seg_t s = { p0.x, p0.y, w, i0, b0, i1 - i0, f };
        push_deco_seg(v, s);
        return;
    }
    layout_deco_range(label, font, txt, txt_len, line_rtl, false, v, i0, mid, b0, f);
    layout_deco_range(label, font, txt, txt_len, line_rtl, false, v, mid, i1, (uint32_t)mid_b, f);
}

/* Compute v->segs for a label whose text / width were just set. */
static void layout_line_decorations(lv_obj_t *label, const std::string &text,
                                    line_view_t *v)
{
    v->segs.clear();
    size_t n = v->flags.size();
    bool any = false;
    for (size_t i = 0; i < n && !any; i++) any = v->flags[i] != 0;
    if (!any) return;

    const lv_font_t *font = lv_obj_get_style_text_font(label, LV_PART_MAIN);
    const char *txt = text.c_str();
    size_t len = text.size();
    bool has_rtl = false;
    for (size_t b = 0; b < len && !has_rtl;) has_rtl = cp_strong_dir(utf8_decode(txt, len, &b)) < 0;
    bool line_rtl = utf8_first_strong_dir(txt, len) < 0;

    size_t b = 0;
    uint32_t i = 0;
    while (i < n && b < len) {
        uint8_t f = v->flags[i];
        uint32_t j = i;
        size_t jb = b;
        while (j < n && jb < len && v->flags[j] == f) {
            (void)utf8_decode(txt, len, &jb);
            j++;
        }
        if (f) {
            layout_deco_range(label, font, txt, len, line_rtl, has_rtl, v,
                              i, j, (uint32_t)b, f);
        }
        i = j;
        b = jb;
    }
}

static void deco_fill(lv_layer_t *layer, int32_t x1, int32_t y1, int32_t x2,
                      int32_t y2, lv_color_t color)
{
    if (x2 < x1 || y2 < y1) return;
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.bg_color = color;
    d.bg_opa   = LV_OPA_COVER;
    lv_area_t a = { x1, y1, x2, y2 };
    lv_draw_rect(layer, &d, &a);
}

/* Draw one glyph slanted: it is split into 4-row horizontal bands,
 * each drawn shifted one more pixel right than the band below it,
 * pivoting around the baseline (asc = rows above the baseline).
 * Clipping each band goes through the layer's clip area, which LVGL
 * documents as settable before adding draw tasks. */
static void deco_draw_slanted(lv_layer_t *layer, lv_draw_label_dsc_t *dsc,
                              uint32_t cp, int32_t x, int32_t top,
                              int32_t lh, int32_t asc)
{
    const int32_t band = 4;
    const lv_area_t orig = layer->_clip_area;
    /* Centre the slant on the cell: shift everything left by half of
     * the top band's shift. */
    int32_t centre = (asc / band) / 2;
    int32_t y0 = 0;
    while (y0 < lh) {
        /* Floor division so rows below the baseline shift left. */
        int32_t d = asc - 1 - y0;
        int32_t k = d >= 0 ? d / band : -((-d + band - 1) / band);
        int32_t y1 = asc - 1 - k * band;
        if (y1 > lh - 1) y1 = lh - 1;
        lv_area_t clip = orig;
        if (clip.y1 < top + y0) clip.y1 = top + y0;
        if (clip.y2 > top + y1) clip.y2 = top + y1;
        if (clip.y1 <= clip.y2) {
            layer->_clip_area = clip;
            lv_point_t pt = { x + k - centre, top };
            lv_draw_character(layer, dsc, &pt, cp);
        }
        y0 = y1 + 1;
    }
    layer->_clip_area = orig;
}

/* Offset of a label's content area (where lv_label_get_letter_pos()
 * coordinates start) from its outer top-left corner: padding plus
 * border. Non-zero for the blockquote style (left bar + padding) and
 * the code style (box + left padding); a cursor placed at the raw
 * letter position would otherwise land inside the glyph. */
static lv_point_t label_content_offset(lv_obj_t *label)
{
    lv_area_t oc, cc;
    lv_obj_get_coords(label, &oc);
    lv_obj_get_content_coords(label, &cc);
    lv_point_t p = { cc.x1 - oc.x1, cc.y1 - oc.y1 };
    return p;
}

/* Let the styling spill a few pixels past the label box: a faux-bold
 * or slanted glyph in the last column, and the code-span box drawn
 * just outside its cells. */
static void line_deco_ext_size_cb(lv_event_t *e)
{
    lv_event_set_ext_draw_size(e, 3);
}

/* LV_EVENT_DRAW_MAIN_END handler of every editor line label: paints
 * the view's rule, bullets and inline styling over the plain text the
 * label has just drawn. Colors follow the label's own (a fully
 * selected line is inverted) and its LV_PART_SELECTED style for
 * characters inside a partial selection. */
static void line_deco_draw_cb(lv_event_t *e)
{
    const line_view_t *v = (const line_view_t *)lv_event_get_user_data(e);
    if (!v || (!v->hr && v->segs.empty())) return;
    lv_obj_t *label = lv_event_get_target_obj(e);
    lv_layer_t *layer = lv_event_get_layer(e);

    lv_area_t ca;
    lv_obj_get_content_coords(label, &ca);
    const lv_font_t *font = lv_obj_get_style_text_font(label, LV_PART_MAIN);
    int32_t lh  = lv_font_get_line_height(font);
    int32_t asc = lh - font->base_line;
    int32_t th  = lh >= 24 ? 2 : 1;         /* stroke thickness */
    int32_t bdx = lh >= 40 ? 2 : 1;         /* faux-bold offset */

    lv_color_t fg = lv_obj_get_style_text_color(label, LV_PART_MAIN);
    lv_color_t bg = lv_obj_get_style_bg_opa(label, LV_PART_MAIN) >= LV_OPA_50
                        ? lv_obj_get_style_bg_color(label, LV_PART_MAIN)
                        : theme_bg();
    lv_color_t sel_fg = lv_obj_get_style_text_color(label, LV_PART_SELECTED);
    lv_color_t sel_bg = lv_obj_get_style_bg_color(label, LV_PART_SELECTED);
    uint32_t ss = lv_label_get_text_selection_start(label);
    uint32_t se = lv_label_get_text_selection_end(label);
    bool has_sel = ss != LV_LABEL_TEXT_SELECTION_OFF &&
                   se != LV_LABEL_TEXT_SELECTION_OFF && se > ss;

    if (v->hr) {
        int32_t y = ca.y1 + lh / 2 - th / 2;
        deco_fill(layer, ca.x1, y, ca.x2, y + th - 1, fg);
    }

    const char *txt = lv_label_get_text(label);
    size_t txt_len = strlen(txt);
    lv_draw_label_dsc_t ld;
    lv_draw_label_dsc_init(&ld);
    ld.font = font;
    ld.opa  = LV_OPA_COVER;

    /* x-height centre (roughly), used by strikethrough and bullets. The
     * Indic pack's tall fonts point at their base font (user_data),
     * whose ascent is that of the letters, not of the taller line. */
    int32_t asc_txt = asc;
    if (font->user_data) {
        const lv_font_t *bf = (const lv_font_t *)font->user_data;
        asc_txt = lv_font_get_line_height(bf) - bf->base_line;
    }
    int32_t mid_y = asc - (asc_txt * 3) / 10;

    for (const deco_seg_t &s : v->segs) {
        int32_t top = ca.y1 + s.y;
        uint8_t f = s.flags;

        /* Pass 1: per-character backgrounds / strokes; pass 2: glyphs.
         * Italic cells are blanked in pass 1 so the slanted copies
         * drawn in pass 2 are not overwritten by a neighbour's blank. */
        for (int pass = 0; pass < 2; pass++) {
            size_t b = s.byte_off;
            int32_t cx = ca.x1 + s.x;
            for (uint32_t k = 0; k < s.count && b < txt_len; k++) {
                uint32_t cp = utf8_decode(txt, txt_len, &b);
                uint32_t nx = cp_at(txt, txt_len, b);
                int32_t gw = lv_font_get_glyph_width(font, cp, nx);
                uint32_t dcp = deco_draw_cp(cp, nx);
                int32_t gx = cx;
                cx += gw;
                uint32_t ci = s.char_idx + k;
                bool sel = has_sel && ci >= ss && ci < se;
                lv_color_t cfg = sel ? sel_fg : fg;
                lv_color_t cbg = sel ? sel_bg : bg;

                if (pass == 0) {
                    if (f & DECO_ITALIC) {
                        deco_fill(layer, gx, top, gx + gw - 1, top + lh - 1, cbg);
                    }
                    continue;
                }

                if (f & DECO_BULLET) {
                    int32_t d = gw / 2 + 1;
                    if (d < 3) d = 3;
                    int32_t x1 = gx + (gw - d) / 2;
                    int32_t y1 = top + mid_y - d / 2;
                    lv_draw_rect_dsc_t rd;
                    lv_draw_rect_dsc_init(&rd);
                    int shape = v->bullet_level % 3;
                    rd.radius = shape == 2 ? 0 : LV_RADIUS_CIRCLE;
                    if (shape == 1) {
                        rd.bg_opa       = LV_OPA_TRANSP;
                        rd.border_color = cfg;
                        rd.border_width = th;
                        rd.border_opa   = LV_OPA_COVER;
                    } else {
                        rd.bg_color = cfg;
                        rd.bg_opa   = LV_OPA_COVER;
                    }
                    lv_area_t a = { x1, y1, x1 + d - 1, y1 + d - 1 };
                    lv_draw_rect(layer, &rd, &a);
                    continue;
                }

                if (f & DECO_BREAK) {
                    /* A return arrow: a stem down the right of the cell
                     * at x-height, a bar along the bottom and a solid
                     * arrowhead at its left end. */
                    int32_t a  = gw / 3 < 2 ? 2 : gw / 3;
                    int32_t xl = gx + 1;
                    int32_t xr = gx + gw - 1 - th;
                    int32_t yb = top + asc - a;
                    int32_t yt = top + asc - (asc * 6) / 10;
                    if (yt > yb - a) yt = yb - a;
                    deco_fill(layer, xl, yb, xr + th - 1, yb + th - 1, cfg);
                    deco_fill(layer, xr, yt, xr + th - 1, yb + th - 1, cfg);
                    for (int32_t i = 1; i <= a; i++)
                        deco_fill(layer, xl + i, yb - i, xl + i, yb + th - 1 + i, cfg);
                    continue;
                }

                ld.color = cfg;
                if (f & DECO_ITALIC) {
                    deco_draw_slanted(layer, &ld, dcp, gx, top, lh, asc);
                    if (f & DECO_BOLD) {
                        deco_draw_slanted(layer, &ld, dcp, gx + bdx, top, lh, asc);
                    }
                } else if (f & DECO_BOLD) {
                    lv_point_t pt = { gx + bdx, top };
                    lv_draw_character(layer, &ld, &pt, dcp);
                }
                if (f & DECO_STRIKE) {
                    int32_t y = top + mid_y - th / 2;
                    deco_fill(layer, gx, y, gx + gw - 1, y + th - 1, cfg);
                }
                if (f & DECO_UNDER) {
                    /* One row below the baseline, inside the cell. */
                    int32_t y = top + asc + 1;
                    if (y + th > top + lh) y = top + lh - th;
                    deco_fill(layer, gx, y, gx + gw - 1, y + th - 1, cfg);
                }
            }
        }

        if (f & DECO_CODE) {
            /* Box the run's cells. Over selected characters it blends
             * into the highlight, which is fine. */
            lv_draw_rect_dsc_t rd;
            lv_draw_rect_dsc_init(&rd);
            rd.bg_opa       = LV_OPA_TRANSP;
            rd.border_color = fg;
            rd.border_width = 1;
            rd.border_opa   = LV_OPA_COVER;
            lv_area_t a = { ca.x1 + s.x - 1, top, ca.x1 + s.x + s.w, top + lh - 1 };
            lv_draw_rect(layer, &rd, &a);
        }
    }
}

/* Render the currently-bound pane (s_rp) for its bound document. The
 * caller binds the pane (pane_bind) and decides whether this pane shows
 * the cursor (draw_cursor is true only for the focused pane). The
 * shared title / battery widgets are refreshed once by the
 * editor_ui_refresh() wrapper, not here. */
static void refresh_active_pane(bool draw_cursor)
{
    /* Show logo when no file is loaded and buffer is empty */
    size_t text_len = editor_get_length();
    bool show_logo = (editor_get_file_path() == NULL && text_len == 0);
    if (s_img_logo) {
        if (show_logo) {
            lv_obj_remove_flag(s_img_logo, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_img_logo, LV_OBJ_FLAG_HIDDEN);
        }
    }

    int total  = editor_get_line_count();

    /* Selection range (byte offsets in the document) */
    size_t sel_start = 0, sel_end = 0;
    bool has_sel = editor_selection_active();
    if (has_sel) editor_get_selection_range(&sel_start, &sel_end);

    int cur_line, cur_col;
    editor_get_cursor_pos(&cur_line, &cur_col);

    int cur_y = -1;      /* cursor y position (set when cursor line is rendered) */
    int cur_x = -1;      /* cursor x position */
    int cur_h = LINE_H;  /* cursor height (matches font of cursor line) */
    bool cur_rendered = false;  /* cursor line was laid out this pass */

    /* Render visible lines.  Wrapped lines consume more vertical space
     * than a single LINE_H row, so the cursor may be pushed off-screen
     * even though ensure_cursor_visible() thought it was in range.
     * When that happens, increment scroll and re-render (bounded). */
    for (int _scroll_retry = 0; _scroll_retry < MAX_LINE_LABELS;
         _scroll_retry++) {
        int scroll = editor_get_scroll_line();
        const editor_format_t *fmt = editor_format_active();
        fmt_scan_t scan;
        editor_format_scan_begin(fmt, &scan);

        /* Carry the format's line state (Markdown: code fences;
         * Fountain: the element context) up to the scroll line,
         * starting from the pane's checkpoint when it is still valid */
        int mark = (int)(s_rp - s_panes);
        int first = 0;

        /* The table: drop entries at or after a change, then start from
         * the nearest usable entry above the scroll line. */
        if (s_rp->scan_tab_doc != s_rp->doc || s_rp->scan_tab_fmt != fmt) {
            s_rp->scan_tab_doc = s_rp->doc;
            s_rp->scan_tab_fmt = fmt;
            s_rp->scan_tab_n = 0;
        }
        size_t changed = editor_change_mark_get(mark);
        while (s_rp->scan_tab_n > 0 && s_rp->scan_tab[s_rp->scan_tab_n - 1].off >= changed)
            s_rp->scan_tab_n--;
        for (int k = s_rp->scan_tab_n - 1; k >= 0; k--) {
            const pane_t::scan_tab_ent *e = &s_rp->scan_tab[k];
            if (e->line <= scroll && scan.cursor > e->off) {
                first = e->line;
                scan.line    = first;
                scan.in_code = e->in_code;
                scan.ftn     = e->ftn;
                break;
            }
        }

        if (scroll > 0 && s_rp->scan_ck_doc == s_rp->doc &&
            s_rp->scan_ck_fmt == fmt && s_rp->scan_ck_line > 0 &&
            s_rp->scan_ck_line <= scroll && s_rp->scan_ck_line > first &&
            editor_change_mark_get(mark) > s_rp->scan_ck_off &&
            scan.cursor > s_rp->scan_ck_off) {
            first = s_rp->scan_ck_line;
            scan.line    = first;
            scan.in_code = s_rp->scan_ck_in_code;
            scan.ftn     = s_rp->scan_ck_ftn;
        }
        for (int i = first; i < scroll && i < total; i++) {
            size_t ll, off;
            const char *lt = editor_get_line_off(i, &ll, &off);
            /* Record the next table entry when passing its line. */
            int n = s_rp->scan_tab_n;
            if (i > 0 && i == (n + 1) * SCAN_TAB_STEP && scan.cursor > off) {
                if (n == s_rp->scan_tab_cap) {
                    int cap = n ? n * 2 : 64;
                    void *nt = heap_caps_realloc(s_rp->scan_tab,
                                                 (size_t)cap * sizeof(*s_rp->scan_tab),
                                                 MALLOC_CAP_SPIRAM);
                    if (nt) {
                        s_rp->scan_tab = (pane_t::scan_tab_ent *)nt;
                        s_rp->scan_tab_cap = cap;
                    }
                }
                if (n < s_rp->scan_tab_cap) {
                    s_rp->scan_tab[n] = { i, off, scan.in_code, scan.ftn };
                    s_rp->scan_tab_n = n + 1;
                }
            }
            fmt->parse_line(&scan, lt, ll, NULL);
        }
        s_rp->scan_ck_doc = NULL;
        if (scroll > 0 && scroll < total) {
            size_t ll, off;
            editor_get_line_off(scroll, &ll, &off);
            if (scan.cursor > off) {
                s_rp->scan_ck_doc     = s_rp->doc;
                s_rp->scan_ck_fmt     = fmt;
                s_rp->scan_ck_line    = scroll;
                s_rp->scan_ck_off     = off;
                s_rp->scan_ck_in_code = scan.in_code;
                s_rp->scan_ck_ftn     = scan.ftn;
                editor_change_mark_reset(mark);
            }
        }

        std::string tmp;        /* final rendered text for the current slot */
        line_view_t view_tmp;   /* its display <-> raw mapping + styling */
        /* running y position in editor content area; the scroll line
         * may start above the pane top (see pane_t::top_px) */
        int top_px = pane_top_px();
        int y_pos = pane_lead_px() - top_px;
        int first_h = 0;     /* rendered height of the scroll line */
        int first_pitch = LINE_H; /* its visual row pitch */
        cur_y = -1;
        cur_x = -1;
        cur_h = LINE_H;
        cur_rendered = false;
        bool counted_visible = false; /* first hidden slot -> s_visible_line_count */

        for (int i = 0; i < MAX_LINE_LABELS; i++) {
            int line_idx = scroll + i;
            if (line_idx >= total || y_pos >= s_rp->h) {
                /* Record how many logical lines actually fit above this
                 * slot -- see the pane_t::visible_line_count comment.
                 * Once this branch triggers for a given i it triggers
                 * for every later i too (line_idx and y_pos are both
                 * monotonic), so the first hit gives the count. */
                if (!counted_visible) {
                    s_visible_line_count = i;
                    counted_visible = true;
                }
                /* Only invalidate the slot when its visible state
                 * actually changes; otherwise lv_obj_add_flag()
                 * dirties the previous label rectangle on every
                 * refresh and the union of all those rectangles
                 * fills most of the editor area, defeating the
                 * partial-update path on e-paper backends.
                 *
                 * Gate on the widget's *actual* LVGL visibility rather
                 * than the s_prev_line_* cache: invalidate_render_cache()
                 * (called on a font-size or theme change) resets those
                 * cache flags to false while the labels are still shown
                 * on screen holding the previous layout's text. Keying
                 * off the cache would then skip hiding a genuinely-visible
                 * stale label, so its old (differently-sized) content
                 * survives the next full repaint as garbage -- exactly
                 * the symptom seen after switching to a larger font and
                 * re-opening a file. Checking the live HIDDEN flag hides
                 * whatever is really on screen while still avoiding a
                 * redundant invalidate when the slot is already hidden. */
                if (s_line_labels[i] &&
                    !lv_obj_has_flag(s_line_labels[i], LV_OBJ_FLAG_HIDDEN)) {
                    lv_obj_add_flag(s_line_labels[i], LV_OBJ_FLAG_HIDDEN);
                }
                /* Always clear the cached state for hidden slots so a
                 * later transition back to visible is forced through
                 * the full re-render path with a clean baseline. */
                s_prev_line_visible[i]      = false;
                s_prev_line_was_selected[i] = false;
                continue;
            }

            size_t ll, loff;
            const char *lt = editor_get_line_off(line_idx, &ll, &loff);

            md_line_info_t mi;
            fmt->parse_line(&scan, lt, ll, &mi);

            /* Build the final display string (markers hidden except on
             * the focused cursor line -- see line_view_t) up-front so
             * we can compare against the cached previous content before
             * touching any LVGL state. The std::string grows to fit, so
             * paragraphs of arbitrary length render in full without
             * truncation. */
            bool reveal = draw_cursor && line_idx == cur_line;
            long cur_b = reveal ? (long)utf8_byte_of_char(lt, ll, cur_col) : -1;
            build_line_view(fmt, lt, ll, &mi, reveal, cur_b, tmp, &view_tmp);
            line_view_t *view = &s_rp->view[i];

            /* Determine whether this line intersects the active
             * selection (used both for the highlight branches below
             * and to disable the fast-path cache when selection is
             * involved -- the inversion / overlay rendering needs
             * full re-evaluation). */
            bool line_intersects_sel = false;
            if (has_sel) {
                size_t leoff = loff + ll;
                if (sel_start < leoff && sel_end > loff) {
                    line_intersects_sel = true;
                } else if (leoff < text_len &&
                           editor_char_at(leoff) == '\n' &&
                           sel_start < leoff + 1 && sel_end > loff) {
                    line_intersects_sel = true;
                }
            }

            /* Fast path: when the visible text, markdown style, and y
             * position for this slot all match the previous refresh
             * AND no selection touches this line (now or last time),
             * we can leave the LVGL label untouched -- which means no
             * dirty rectangle for this slot and the e-paper backend
             * issues a tight partial refresh covering only the line
             * that actually changed. */
            bool can_skip = s_line_labels[i] != NULL &&
                            s_prev_line_visible[i] &&
                            !line_intersects_sel &&
                            !s_prev_line_was_selected[i] &&
                            s_prev_line_type[i] == (int)mi.type &&
                            s_prev_line_y[i] == y_pos &&
                            s_prev_line_text[i] == tmp &&
                            view->flags == view_tmp.flags &&
                            view->hr == view_tmp.hr &&
                            view->bullet_level == view_tmp.bullet_level;

            int line_h;
            int rendered_h;
            if (can_skip) {
                /* Reuse cached layout metrics; the label still holds
                 * the same text/font/width so its rendered geometry
                 * is unchanged. lv_label_get_letter_pos() (used for
                 * the cursor below) is non-mutating and works on the
                 * existing label state. The rendering (and so
                 * view->segs) is unchanged too, but the raw line behind
                 * it may differ (e.g. "*x*" vs "_x_"), so the mapping
                 * is always refreshed. */
                view->d2r.swap(view_tmp.d2r);
                view->raw_len = view_tmp.raw_len;
                rendered_h = s_prev_line_h[i];
                const lv_font_t *line_font = lv_obj_get_style_text_font(
                                                s_line_labels[i], LV_PART_MAIN);
                line_h = lv_font_get_line_height(line_font ? line_font : body_font());
            } else {
                /* Create or reuse label */
                if (!s_line_labels[i]) {
                    s_line_labels[i] = lv_label_create(s_cont_edit);
                    lv_obj_set_width(s_line_labels[i], s_rp->w - 4);
                    lv_label_set_long_mode(s_line_labels[i], LV_LABEL_LONG_WRAP);
                    lv_obj_add_event_cb(s_line_labels[i], line_deco_draw_cb,
                                        LV_EVENT_DRAW_MAIN_END, view);
                    lv_obj_add_event_cb(s_line_labels[i], line_deco_ext_size_cb,
                                        LV_EVENT_REFR_EXT_DRAW_SIZE, NULL);
                }
                lv_obj_remove_flag(s_line_labels[i], LV_OBJ_FLAG_HIDDEN);
                lv_obj_remove_style_all(s_line_labels[i]);
                lv_obj_add_style(s_line_labels[i], style_for_type(mi.type), 0);
                /* Re-apply width after style reset (remove_style_all clears it)
                 * so that LV_LABEL_LONG_WRAP can wrap at the correct boundary. */
                lv_obj_set_width(s_line_labels[i], s_rp->w - 4);
                fmt->apply_layout(s_line_labels[i], mi.type, s_rp->w - 4);
                /* Re-apply the LV_PART_SELECTED colors too (remove_style_all
                 * wipes every part, not just LV_PART_MAIN) so the selection
                 * highlight set below renders correctly. This uses LVGL's
                 * native per-character text selection (lv_label_set_text_
                 * selection_start/end) rather than a hand-rolled overlay,
                 * so it draws correctly no matter how many visual rows a
                 * wrapped line's selection spans. */
                lv_obj_set_style_bg_color(s_line_labels[i], theme_fg(),
                                          LV_PART_SELECTED);
                lv_obj_set_style_bg_opa(s_line_labels[i], LV_OPA_COVER,
                                        LV_PART_SELECTED);
                lv_obj_set_style_text_color(s_line_labels[i], theme_bg(),
                                            LV_PART_SELECTED);
                lv_label_set_text_selection_start(s_line_labels[i],
                                                  LV_LABEL_TEXT_SELECTION_OFF);
                lv_label_set_text_selection_end(s_line_labels[i],
                                                LV_LABEL_TEXT_SELECTION_OFF);
                lv_label_set_text_static(s_line_labels[i], "");
                (lv_label_set_text)(s_line_labels[i], tmp.c_str());
                lv_obj_set_pos(s_line_labels[i], 2, y_pos);

                /* Get the font for this line to compute correct line height */
                const lv_font_t *line_font = lv_obj_get_style_text_font(
                                                s_line_labels[i], LV_PART_MAIN);
                line_h = lv_font_get_line_height(line_font ? line_font : body_font());

                /* Determine actual rendered height (may be taller if text wraps) */
                lv_obj_update_layout(s_line_labels[i]);
                rendered_h = lv_obj_get_height(s_line_labels[i]);
                if (rendered_h < line_h) rendered_h = line_h;

                view->d2r.swap(view_tmp.d2r);
                view->flags.swap(view_tmp.flags);
                view->raw_len      = view_tmp.raw_len;
                view->hr           = view_tmp.hr;
                view->bullet_level = view_tmp.bullet_level;
                layout_line_decorations(s_line_labels[i], tmp, view);

                /* Update cache with what we just drew. */
                s_prev_line_text[i]    = tmp;
                s_prev_line_type[i]    = (int)mi.type;
                s_prev_line_y[i]       = y_pos;
                s_prev_line_h[i]       = rendered_h;
                s_prev_line_visible[i] = true;
            }

            /* Selection highlight.  A fully-selected line uses color
             * inversion on the whole label (black bg, white text).  A
             * partial selection -- which may span more than one visual
             * row when the line word-wraps -- uses LVGL's native
             * per-character text selection (lv_label_set_text_selection_
             * start/end, drawn via the LV_PART_SELECTED style applied
             * above) so every wrapped row is highlighted accurately
             * instead of a hand-rolled single-row overlay that silently
             * over-highlighted (and mis-reported to the user as fully
             * selected) any selection spanning more than one row. */
            if (has_sel) {
                size_t line_off = loff;
                size_t line_end_off = line_off + ll;
                bool fully = (sel_start <= line_off &&
                              sel_end >= line_end_off);
                bool partial = !fully &&
                    (sel_start < line_end_off && sel_end > line_off);
                /* Include trailing newline for overlap check */
                if (!fully && !partial && line_end_off < text_len &&
                    editor_char_at(line_end_off) == '\n') {
                    partial = (sel_start < line_end_off + 1 &&
                               sel_end > line_off);
                }
                if (fully) {
                    lv_obj_set_style_bg_color(s_line_labels[i],
                                              theme_fg(), 0);
                    lv_obj_set_style_bg_opa(s_line_labels[i],
                                            LV_OPA_COVER, 0);
                    lv_obj_set_style_text_color(s_line_labels[i],
                                                theme_bg(), 0);
                } else if (partial) {
                    /* Compute selection byte range within this line */
                    size_t raw_s = (sel_start > line_off)
                                       ? sel_start - line_off : 0;
                    size_t raw_e = (sel_end < line_end_off)
                                       ? sel_end - line_off : ll;
                    /* Convert raw byte offsets to display char indices
                     * (hidden Markdown markers have no display char). */
                    int disp_s = view_raw_to_disp(view, raw_s);
                    int disp_e = view_raw_to_disp(view, raw_e);
                    if (disp_e > disp_s) {
                        lv_label_set_text_selection_start(
                            s_line_labels[i], (uint32_t)disp_s);
                        lv_label_set_text_selection_end(
                            s_line_labels[i], (uint32_t)disp_e);
                    }
                }
            }

            /* Remember whether this slot is currently rendering a
             * selected line so the next refresh can decide whether
             * to invoke the full re-render path that clears the
             * inversion / overlay. */
            s_prev_line_was_selected[i] = line_intersects_sel;

            /* If this is the cursor line, compute its pixel position.
             * The cursor column counts UTF-8 characters from the raw
             * line start; map it through the view, since the display
             * text can differ from the raw line (hidden markers in an
             * unfocused pane, expanded tabs). */
            if (line_idx == cur_line) {
                int col_in_display = view_raw_to_disp(
                    view, utf8_byte_of_char(lt, ll, cur_col));

                /* Use LVGL to find the actual pixel position of the cursor
                 * character.  This correctly handles word-level wrapping
                 * where the break point differs from a simple chars_per_row
                 * calculation. */
                lv_point_t lpos;
                lv_label_get_letter_pos(s_line_labels[i],
                                        (uint32_t)col_in_display, &lpos);
                /* The space a row breaks at may hang past the right
                 * edge (see lv_wrap_spaces.c); keep the cursor after it
                 * at the edge instead of off screen. */
                int32_t cw = lv_obj_get_content_width(s_line_labels[i]);
                if (cw > 2 && lpos.x > cw - 2) lpos.x = cw - 2;
                lv_point_t cofs = label_content_offset(s_line_labels[i]);
                cur_x = 2 + cofs.x + lpos.x;
                cur_y = y_pos + cofs.y + lpos.y;
                cur_h = line_h;
                cur_rendered = true;

                /* A line with no strong directional character (empty
                 * or whitespace-only) detects as LTR and parks the
                 * cursor at the left edge. If the user has switched
                 * to an RTL keyboard layout, the first typed
                 * character will right-align the paragraph, so show
                 * the cursor at the right edge it is about to type
                 * from. */
                if (kb_layout_is_rtl() &&
                    utf8_first_strong_dir(tmp.c_str(), tmp.size()) == 0) {
                    cur_x = s_rp->w - 4;
                }
            }

            if (i == 0) {
                first_h = rendered_h;
                first_pitch = line_h + lv_obj_get_style_text_line_space(
                                  s_line_labels[i], LV_PART_MAIN);
                if (first_pitch <= 0) first_pitch = LINE_H;
            }
            y_pos += rendered_h;
        }
        /* Every slot fit on screen without tripping the count above
         * (only possible when the whole MAX_LINE_LABELS-slot pool is
         * filled with lines short enough to never exceed the pane
         * height) -- fall back to the full pool size. */
        if (!counted_visible) s_visible_line_count = MAX_LINE_LABELS;

        /* If the cursor line is in the expected range but wrapped lines
         * pushed it off-screen, scroll and re-render. Jump by
         * scroll_jump_step() rather than a single line, same as every
         * other scroll site in this file (see SCROLL_JUMP_FRACTION) --
         * otherwise a wrapped paragraph near the bottom of the screen
         * made this correction nudge by exactly one line while an
         * unwrapped one jumped by ~60% via ensure_cursor_visible(),
         * which felt like inconsistent, unpredictable scrolling.
         * Clamped to cur_line so the jump cannot scroll past the very
         * line it is trying to bring into view. Use cur_y + cur_h to
         * ensure the full cursor row is visible. */
        if (s_no_autoscroll) break;
        if (scroll < cur_line &&
            (!cur_rendered || cur_y + cur_h > s_rp->h)) {
            int new_scroll = scroll + scroll_jump_step();
            if (new_scroll > cur_line) new_scroll = cur_line;
            editor_set_scroll_line(new_scroll);
            continue;
        }

        /* The cursor is inside the scroll line itself: a paragraph
         * taller than the pane (or one partly hidden above the top
         * edge) has no further whole line to scroll to, so shift the
         * line within the pane instead (pane_t::top_px). The cursor
         * row lands about SCROLL_JUMP_FRACTION of a screen away from
         * the edge it crossed, like the line-granular jumps above,
         * and top_px is kept a whole number of visual rows so no row
         * is cut at the top edge. */
        bool top_free = s_rp->top_px_free && top_px > 0;
        if (cur_line == scroll && cur_rendered && !s_touch_panning &&
            (cur_y < 0 || cur_y + cur_h > s_rp->h)) {
            int step_px = scroll_jump_step() * first_pitch;
            int tgt = (cur_y < 0) ? step_px - cur_h
                                  : s_rp->h - step_px;
            if (tgt < 0) tgt = 0;
            if (tgt > s_rp->h - cur_h) tgt = s_rp->h - cur_h;
            int new_top = top_px + cur_y - tgt;
            new_top = (new_top / first_pitch) * first_pitch;
            if (new_top > first_h - first_pitch) new_top = first_h - first_pitch;
            if (new_top < 0) new_top = 0;
            if (new_top != top_px) {
                pane_set_top_px(new_top);
                continue;
            }
        } else if (top_px > 0 && top_px > first_h - first_pitch &&
                   !(top_free && top_px < first_h)) {
            /* The scroll line shrank (text deleted) below the hidden
             * part: keep at least its last row on screen. */
            pane_set_top_px(first_h - first_pitch);
            continue;
        }
        break;
    }

    /* Update cursor position. Only the focused pane shows a caret; an
     * unfocused pane keeps its cursor hidden. */
    if (draw_cursor && cur_y >= 0 && cur_y + cur_h <= s_rp->h && s_cursor) {
        lv_obj_set_size(s_cursor, 2, cur_h);
        lv_obj_set_pos(s_cursor, cur_x, cur_y);
        lv_obj_remove_flag(s_cursor, LV_OBJ_FLAG_HIDDEN);
        s_cursor_visible = true;
        s_cursor_on_screen = true;
    } else if (s_cursor) {
        lv_obj_add_flag(s_cursor, LV_OBJ_FLAG_HIDDEN);
        s_cursor_on_screen = false;
    }
}

/* Public refresh entry point: render every visible pane for its bound
 * document, then refresh the shared title / battery widgets once. The
 * focused pane is rendered last so the engine's active document is left
 * pointing at it for subsequent key handling. */
extern "C" void editor_ui_refresh(void)
{
    if (!s_editor_screen_active) return;

    for (int p = 0; p < s_pane_count; p++) {
        if (p == s_focus) continue;     /* focused pane drawn last */
        pane_bind(p);
        refresh_active_pane(false);
    }
    pane_bind_focus();
    /* While the in-pane file selector overlays the focused pane, that
     * pane is hidden behind the opaque list -- skip its (wasted) draw
     * but still leave the active doc bound to it. */
    if (!s_inpane_browser_open) {
        refresh_active_pane(true);
    }

    update_title_bar();
    sync_battery_labels();
}

/* Re-render only the focused pane. Used by the mid-navigation scroll
 * steps in editor_ui_move_visual(): a full editor_ui_refresh() also
 * binds and redraws the unfocused pane, which (when both panes share
 * one document) momentarily retargets the active document at the other
 * pane while capturing/restoring its saved view. Keeping the scroll
 * step focused-pane-only means vertical cursor movement never touches
 * the other pane, so the caret always stays within the focused pane
 * until the user switches with Ctrl+Tab. */
static void refresh_focused_pane(void)
{
    if (!s_editor_screen_active) return;
    pane_bind_focus();
    if (!s_inpane_browser_open) {
        refresh_active_pane(true);
    }
}

/* Like refresh_focused_pane() but also updates the shared title and
 * battery widgets. Used as the post-key refresh for pure cursor
 * navigation (arrows, Home/End, Page Up/Down, word/document moves):
 * these never change document content, so re-rendering the unfocused
 * pane is unnecessary. Avoiding the full editor_ui_refresh() keeps the
 * binding (and therefore the visible caret) on the focused pane, so a
 * cursor that reaches the beginning / end of the text cannot end up
 * drawn in the other pane -- focus only ever moves with Ctrl+Tab.
 * Content-changing keys still use the full editor_ui_refresh() so a
 * sibling pane showing the same shared document repaints the edit. */
static void refresh_focused_pane_and_title(void)
{
    if (!s_editor_screen_active) return;
    refresh_focused_pane();
    update_title_bar();
    sync_battery_labels();
}

static void ensure_cursor_visible(void)
{
    int cur_line, cur_col;
    (void)cur_col;
    editor_get_cursor_pos(&cur_line, &cur_col);
    int scroll = editor_get_scroll_line();
    if (cur_line < scroll) {
        int new_scroll = scroll - scroll_jump_step();
        if (new_scroll < 0) new_scroll = 0;
        /* The cursor jumped further than one scroll step (Ctrl+Home,
         * Find, goto-line, ...) -- a partial jump would still leave
         * it off-screen, so land on it exactly instead. */
        if (cur_line < new_scroll) new_scroll = cur_line;
        editor_set_scroll_line(new_scroll);
    } else if (cur_line >= scroll + VISIBLE_LINES) {
        int new_scroll = scroll + scroll_jump_step();
        if (cur_line >= new_scroll + VISIBLE_LINES) {
            new_scroll = cur_line - VISIBLE_LINES + 1;
        }
        editor_set_scroll_line(new_scroll);
    }
}

/* Forward declarations: defined below in the menu / list-rendering
 * section but referenced from the touch event callbacks immediately
 * below, and from refresh_file_list() (so list rows pick up theme
 * colors and the initial selection highlight). */
static void apply_list_selection_styles(lv_obj_t *list, int sel);
static void update_list_highlight(lv_obj_t *list, int sel, int prev_sel);

/* ---- Pixel <-> document-offset helpers (used by both touch input
 * and visual-line cursor navigation).  Always compiled so that the
 * arrow-key handler can ask "which character is at pixel (x, y) of
 * the rendered editor body?" regardless of whether the touchscreen
 * driver is enabled. */

/* Convert a point (in s_cont_edit local coordinates) to a byte
 * offset within the document. Returns true on
 * success and fills *out_off; returns false if the point landed on
 * empty space below the last rendered line. */
static bool ui_point_to_offset(int x, int y, size_t *out_off)
{
    if (!out_off) return false;

    /* Walk the per-slot render cache to find which line contains
     * the point. The cache is kept in lock-step with the visible
     * labels by editor_ui_refresh(), so s_prev_line_y[i] /
     * s_prev_line_h[i] describe the layout the user actually sees. */
    int slot = -1;
    for (int i = 0; i < MAX_LINE_LABELS; i++) {
        /* The first slot may start above the pane top (pane_t::top_px),
         * so a negative y is valid for a visible slot. */
        if (!s_prev_line_visible[i]) continue;
        int y1 = s_prev_line_y[i];
        int y2 = y1 + s_prev_line_h[i];
        if (y >= y1 && y < y2) { slot = i; break; }
    }
    if (slot < 0) {
        /* Point lies below the last visible line -- map to end-of-document. */
        *out_off = editor_get_length();
        return true;
    }

    int line_idx = editor_get_scroll_line() + slot;
    int total_lines = editor_get_line_count();
    if (line_idx >= total_lines) {
        *out_off = editor_get_length();
        return true;
    }

    size_t ll = 0, line_off = 0;
    const char *lt = editor_get_line_off(line_idx, &ll, &line_off);
    if (!lt) return false;

    /* Translate (x_in_container) into (x_in_label). Each label is
     * positioned at x=2 in the container (see build_editor_screen). */
    int x_in_label = x - 2;
    if (x_in_label < 0) x_in_label = 0;
    int y_in_label = y - s_prev_line_y[slot];
    if (y_in_label < 0) y_in_label = 0;

    /* lv_label_get_letter_on returns a codepoint index into the
     * displayed text. LVGL's BIDI reordering (LV_USE_BIDI) is enabled
     * so Hebrew text renders right-to-left; for pure LTR content this
     * lookup is a no-op. */
    int disp_char = 0;
    if (s_line_labels[slot]) {
        /* lv_label_get_letter_on() discounts the label's padding but
         * not its border (the blockquote bar, the code-block box), so
         * take the border off here. */
        lv_point_t cofs = label_content_offset(s_line_labels[slot]);
        lv_point_t p;
        p.x = x_in_label -
              (cofs.x - lv_obj_get_style_pad_left(s_line_labels[slot], LV_PART_MAIN));
        p.y = y_in_label -
              (cofs.y - lv_obj_get_style_pad_top(s_line_labels[slot], LV_PART_MAIN));
        /* bidi=true so taps on RTL/mixed lines map through LVGL's
         * visual->logical reordering instead of the raw byte order. */
        disp_char = (int)lv_label_get_letter_on(s_line_labels[slot], &p, true);
    }
    if (disp_char < 0) disp_char = 0;

    /* Map the display character back to a raw byte offset through the
     * view the slot was rendered with (hidden Markdown markers, drawn
     * bullets and expanded tabs make the display text differ from the
     * raw line). A point past the end of the rendered text lands after
     * the last raw byte. */
    size_t raw = view_disp_to_raw(&s_rp->view[slot], disp_char);
    if (raw > ll) raw = ll;
    *out_off = line_off + raw;
    return true;
}

/* ---- Visual-line cursor movement ----
 *
 * The default editor_move_up()/_down() jump by *logical* lines,
 * which on a long soft-wrapped paragraph skips the whole rendered
 * block in one keystroke. The functions below instead step the
 * cursor to the next/previous *visual* row, matching what most
 * other editors do.
 *
 * s_visual_goal_x remembers the cursor's preferred x pixel across
 * consecutive Up/Down presses so the column is preserved even when
 * crossing shorter rows -- it is reset by handle_editor_key() on
 * any non-vertical key. */
static int s_visual_goal_x = -1;

static void editor_ui_move_visual(int direction)
{
    /* direction: -1 = up, +1 = down */
    /* Use the render-time on-screen flag rather than LV_OBJ_FLAG_HIDDEN:
     * on LCD backends the blink timer toggles the HIDDEN flag every
     * ~500 ms, and reading it here would intermittently make the
     * function think the cursor is off-screen and fall back to
     * logical-line jumps (skipping multiple visual rows in a
     * soft-wrapped paragraph and resetting s_visual_goal_x). */
    if (!s_cursor || !s_cursor_on_screen) {
        if (direction < 0) editor_move_up();
        else               editor_move_down();
        s_visual_goal_x = -1;
        return;
    }

    int cx = lv_obj_get_x(s_cursor);
    int cy = lv_obj_get_y(s_cursor);
    int ch = lv_obj_get_height(s_cursor);
    int gx = (s_visual_goal_x >= 0) ? s_visual_goal_x : cx;
    /* LVGL's lv_label_get_letter_on() uses an inclusive bottom-edge
     * test (pos.y <= line_y + letter_height), so a y exactly on the
     * boundary between visual rows N and N+1 snaps to row N.  For
     * Up we step one pixel above the cursor top (cy - 1), which
     * lands strictly inside the previous row.  For Down we have to
     * step *past* the cursor bottom (cy + ch + 1), not merely onto
     * it -- otherwise lv_label_get_letter_on() would return a
     * letter on the same visual row and the cursor would not move. */
    int target_y = (direction < 0) ? cy - 1 : cy + ch + 1;

    /* Short-circuit at document boundaries so a Down press on the
     * very last visual row of the document does not scroll past
     * end-of-text. We still need this check up front because the
     * scroll fall-through below assumes there is another logical
     * line to bring into view. */
    {
        int cur_line, cur_col;
        editor_get_cursor_pos(&cur_line, &cur_col);
        (void)cur_col;
        int total = editor_get_line_count();
        if (direction > 0 && cur_line >= total - 1) {
            /* Last logical line: see if any visual row remains
             * within the current label below the cursor. If not,
             * stay put. */
            if (target_y >= s_rp->h) {
                /* At-or-past viewport bottom on the last line: only a
                 * label running past the bottom edge (a paragraph
                 * taller than what is left of the pane) still has
                 * rows below. */
                int slot = cur_line - editor_get_scroll_line();
                if (slot < 0 || slot >= MAX_LINE_LABELS ||
                    !s_prev_line_visible[slot] ||
                    s_prev_line_y[slot] + s_prev_line_h[slot] <= s_rp->h) {
                    return;
                }
            }
        }
        if (direction < 0 && cur_line == 0) {
            /* First logical line: if the cursor is already on the
             * first visual row (cy <= 0, nothing of the line hidden
             * above the top edge), there is no row above. */
            if (cy <= 0 && pane_top_px() == 0) return;
        }
    }

    /* If the target visual row is outside the rendered editor area,
     * scroll and re-render so the row above/below becomes addressable
     * via ui_point_to_offset(). Jumping by scroll_jump_step() rather
     * than a single line means holding Up/Down at the edge of the
     * viewport needs a scroll (and its refresh) only once every few
     * rows instead of on every keypress -- see the comment on
     * SCROLL_JUMP_FRACTION. This does not change which visual row the
     * cursor lands on: target_y is still exactly one row above/below
     * the cursor's own (not-yet-moved) rendered position, wherever
     * that ends up after the jump. */
    for (int attempt = 0; attempt < 2; attempt++) {
        if (target_y >= 0 && target_y < s_rp->h) break;
        int sc = editor_get_scroll_line();
        int cur_line, cur_col;
        editor_get_cursor_pos(&cur_line, &cur_col);
        (void)cur_col;
        if (cur_line == sc &&
            ((direction < 0 && pane_top_px() > 0) ||
             (direction > 0 && s_prev_line_visible[0] &&
              s_prev_line_y[0] + s_prev_line_h[0] > s_rp->h))) {
            /* The cursor is inside a paragraph taller than the pane
             * and the target row is part of it: shift the paragraph
             * within the pane (pane_t::top_px) instead of jumping
             * whole lines, which would skip the rest of it. */
            int step_px = scroll_jump_step() * ch;
            int top = pane_top_px() + (direction < 0 ? -step_px : step_px);
            int max_top = s_prev_line_y[0] + pane_top_px() +
                          s_prev_line_h[0] - ch;
            if (top > max_top) top = max_top;
            pane_set_top_px(top);
        } else if (direction < 0) {
            if (sc <= 0) {
                editor_move_up();
                s_visual_goal_x = gx;
                return;
            }
            int new_sc = sc - scroll_jump_step();
            if (new_sc < 0) new_sc = 0;
            editor_set_scroll_line(new_sc);
        } else {
            int total = editor_get_line_count();
            if (sc + 1 >= total) {
                editor_move_down();
                s_visual_goal_x = gx;
                return;
            }
            int new_sc = sc + scroll_jump_step();
            if (new_sc > total - 1) new_sc = total - 1;
            /* Never scroll past the cursor's own line: its remaining
             * rows (a tall paragraph) would be skipped. */
            if (new_sc > cur_line) new_sc = cur_line;
            editor_set_scroll_line(new_sc);
        }
        refresh_focused_pane();
        if (!s_cursor || !s_cursor_on_screen) {
            if (direction < 0) editor_move_up();
            else               editor_move_down();
            s_visual_goal_x = gx;
            return;
        }
        cy = lv_obj_get_y(s_cursor);
        ch = lv_obj_get_height(s_cursor);
        target_y = (direction < 0) ? cy - 1 : cy + ch + 1;
    }

    if (target_y < 0 || target_y >= s_rp->h) {
        if (direction < 0) editor_move_up();
        else               editor_move_down();
        s_visual_goal_x = gx;
        return;
    }

    size_t off;
    if (ui_point_to_offset(gx, target_y, &off)) {
        editor_set_cursor(off);
        s_visual_goal_x = gx;
    } else {
        if (direction < 0) editor_move_up();
        else               editor_move_down();
        s_visual_goal_x = gx;
    }
}

#if defined(CONFIG_DRAFTLING_TOUCHSCREEN)
/* The three activate-on-tap callbacks for the touchable lists.
 * They are defined further down with the rest of their respective
 * list code; forward-declared here so list_touch_attach can store
 * them as function pointers in the static touch contexts. */
static void browser_activate_item(int row);
static void menu_activate_item(int idx);
static void settings_activate_item(int idx);
#endif

/* Defined with the exit/sleep overlay code further down; the F1 menu's
 * "Sleep now" item (menu_activate_item) needs it before that point. */
static void show_sleep_prompt(void);
#if defined(CONFIG_DRAFTLING_HAS_QUIT)
static void show_leave_prompt(exit_purpose_t purpose);
#endif
static void show_help(help_origin_t origin);
static void show_hwinfo(help_origin_t origin);

#if defined(CONFIG_DRAFTLING_HAS_USB_MSC) || defined(CONFIG_DRAFTLING_HAS_KERMIT)
/* Defined with the split-layout code further down (editor_ui_apply_split_mode);
 * commit_usb_msc_pending_mode() and kermit_start() collapse any split
 * before handing the SD card to USB / the Kermit server so no pane can
 * be left silently editing a file it can no longer safely save. */
static void editor_ui_apply_split_mode(split_mode_t mode);
#endif

/* ---- Touch input ----
 *
 * Touch is gated on CONFIG_DRAFTLING_TOUCHSCREEN. When enabled we
 * register LVGL pointer-event handlers on:
 *   - the editor content area (s_cont_edit): tap moves the cursor,
 *     double-tap selects the word at the tap point, and vertical
 *     swipes scroll the document.
 *   - each list-row button in the main menu, settings menu, and
 *     file browser: a tap highlights the row (single click) and a
 *     second tap on the already-highlighted row activates it.
 *     This mirrors the keyboard flow (arrow keys then Enter) so a
 *     touch-only user can reach every command.
 *
 * The keyboard handlers are untouched and continue to work in
 * parallel; touch is purely additive. */

#if defined(CONFIG_DRAFTLING_TOUCHSCREEN)

#include <cctype>
#include <cstdlib>

/* Treat as a "word" character: ASCII alnum / underscore, and any
 * UTF-8 continuation/start byte (>= 0x80). This makes word-select
 * work on Latin, Cyrillic, accented chars, etc. without needing a
 * full Unicode category table. */
static bool touch_is_word_byte(unsigned char c)
{
    if (c >= 0x80) return true;
    if (c == '_') return true;
    if (c >= '0' && c <= '9') return true;
    if (c >= 'a' && c <= 'z') return true;
    if (c >= 'A' && c <= 'Z') return true;
    return false;
}

/* (ui_point_to_offset is defined unconditionally above so
 * visual-line cursor movement can use it too.) */

/* Last tap state for software double-tap detection. LVGL fires
 * LV_EVENT_CLICKED on every release; a click whose press began within
 * TOUCH_DOUBLE_TAP_MS of the previous click (release), close to it,
 * is promoted to a "double tap". Timing from the previous release to
 * this press (not release to release) keeps how long the finger rests
 * on the glass out of the window. */
static uint32_t s_last_tap_ms = 0;
static uint32_t s_press_ms    = 0;
static int      s_last_tap_x  = -1;
static int      s_last_tap_y  = -1;
#define TOUCH_DOUBLE_TAP_MS   400  /* same as the LVGL default short-click streak */
/* Finger jitter tolerance. 12 px is ~1 mm on the high-density panels,
 * tighter than a fingertip repeats (seen on the Seeed reTerminal
 * Sticky's 3.97" 800x480 panel). */
#if defined(CONFIG_DRAFTLING_DISPLAY_HIDPI)
#define TOUCH_DOUBLE_TAP_PX    24
#else
#define TOUCH_DOUBLE_TAP_PX    12
#endif

/* Drag-to-scroll state. Touch panels here typically poll at 30-60 Hz
 * with small per-frame deltas; LVGL's built-in gesture detector needs
 * a fairly high velocity to fire, which makes "slow scroll a long
 * document" feel unresponsive. So we implement drag-scrolling
 * directly off LV_EVENT_PRESSED / _PRESSING / _RELEASED: every time
 * the finger has moved one full line height vertically, scroll the
 * document by one line and re-anchor. The accumulated drag distance
 * also lets us suppress the LV_EVENT_CLICKED that LVGL emits on
 * release after a drag, so a swipe never moves the cursor by
 * accident. */
static bool s_drag_active     = false;
static int  s_drag_start_x    = 0;
static int  s_drag_start_y    = 0;
static int  s_drag_anchor_y   = 0;
static int  s_drag_total_dy   = 0;
#define TOUCH_DRAG_SUPPRESS_PX 8   /* clicks within this radius still count as taps */

/* Convert a tap point (in s_cont_edit local coordinates) to a byte
 * offset within the document. Returns true on
 * success and fills *out_off; returns false if the tap landed on
 * empty space below the last rendered line. */
/* Convert a tap point (in s_cont_edit local coordinates) to a byte
 * offset within the document. Implemented as a
 * thin wrapper around ui_point_to_offset(), which is also reused
 * by keyboard-driven visual-line cursor movement. */
static bool touch_point_to_offset(int x, int y, size_t *out_off)
{
    return ui_point_to_offset(x, y, out_off);
}

/* Expand a single-tap byte offset into the selection range of the
 * word containing it. */
static void touch_select_word_at(size_t off)
{
    size_t text_len = editor_get_length();
    if (text_len == 0) return;
    if (off > text_len) off = text_len;

    /* Walk left until non-word byte (UTF-8-safe: continuation bytes
     * test as word chars so multi-byte codepoints are not split). */
    size_t start = off;
    while (start > 0 && touch_is_word_byte((unsigned char)editor_char_at(start - 1))) {
        start--;
    }
    size_t end = off;
    while (end < text_len && touch_is_word_byte((unsigned char)editor_char_at(end))) {
        end++;
    }
    if (start == end) return;  /* tapped on whitespace */

    editor_clear_selection();
    editor_set_cursor(start);
    editor_set_selection_anchor();
    editor_set_cursor(end);
}

/* Scroll the pane the finger is on by delta_lines WITHOUT moving the
 * keyboard focus or the editing caret. A drag / swipe should only pan
 * the touched pane's view; the caret must stay where the user is
 * typing -- which may be the other pane. The touched pane is bound just
 * long enough to update its scroll line and repaint it, then the
 * focused-pane binding is restored. Binding the focused pane back does
 * not repaint it, so its caret widget keeps its position / blink state.
 * Only the focused pane ever draws a caret (refresh_active_pane's
 * draw_cursor is true only when the touched pane is the focused one), so
 * panning an unfocused pane never makes a caret appear in it. */
static void touch_scroll_pane_by(int pane_idx, int delta_lines)
{
    if (delta_lines == 0) return;
    if (pane_idx < 0 || pane_idx >= s_pane_count) pane_idx = s_focus;

    bool refocus = (pane_idx != s_focus);
    pane_bind(pane_idx);

    int total      = editor_get_line_count();
    int scroll     = editor_get_scroll_line();
    int top        = pane_top_px();
    /* Within a paragraph taller than the pane, pan by rows inside it
     * (pane_t::top_px) before moving on to whole lines. */
    int first_bottom = s_prev_line_visible[0]
                     ? s_prev_line_y[0] + s_prev_line_h[0] : 0;
    if ((delta_lines < 0 && top > 0) ||
        (delta_lines > 0 && first_bottom > s_rp->h)) {
        int new_top = top + delta_lines * LINE_H;
        int max_top = top + first_bottom - LINE_H;
        if (new_top > max_top) new_top = max_top;
        if (new_top < 0) new_top = 0;
        pane_set_top_px(new_top);
        s_touch_panning = true;
        refresh_active_pane(pane_idx == s_focus);
        s_touch_panning = false;
        standby_reset_timer();
    } else {
        int new_scroll = scroll + delta_lines;
        if (new_scroll > total - 1) new_scroll = total - 1;
        if (new_scroll < 0) new_scroll = 0;
        if (new_scroll != scroll) {
            editor_set_scroll_line(new_scroll);
            refresh_active_pane(pane_idx == s_focus);
            standby_reset_timer();
        }
    }

    if (refocus) pane_bind_focus();
}

static void editor_touch_event_cb(lv_event_t *e)
{
    /* Bail out early if any modal overlay is open: the overlay's
     * own keyboard handler owns the input. (A future revision could
     * give overlays their own touch routing; for now we skip them.) */
    if (s_menu_open || s_settings_open || s_save_open || s_search_open ||
        s_exit_open) {
        return;
    }
    if (!s_editor_screen_active) return;
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) suggest_hide();

    /* The tapped pane index was stored in the container's user_data at
     * creation. Focus is NOT moved here: a drag / swipe only pans the
     * touched pane (see touch_scroll_pane_by), leaving the caret in the
     * pane the user is editing. Keyboard focus follows a genuine TAP
     * (the CLICKED branch below), which is the user explicitly placing
     * the caret in that pane. */
    int pane_idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (pane_idx < 0 || pane_idx >= s_pane_count) pane_idx = s_focus;

    lv_event_code_t code = lv_event_get_code(e);

    /* --- Drag-to-scroll ---
     *
     * On PRESSED we record the start point; on every PRESSING we
     * compute the vertical delta from the last anchor and, once it
     * exceeds one line height, scroll the document by that many
     * lines. RELEASED leaves s_drag_total_dy set so the CLICKED
     * branch below can ignore the release if the user actually
     * dragged rather than tapped. */
    if (code == LV_EVENT_PRESSED) {
        lv_indev_t *indev = lv_indev_active();
        if (!indev) return;
        lv_point_t pt;
        lv_indev_get_point(indev, &pt);
        s_press_ms      = lv_tick_get();
        s_drag_active   = true;
        s_drag_start_x  = pt.x;
        s_drag_start_y  = pt.y;
        s_drag_anchor_y = pt.y;
        s_drag_total_dy = 0;
        return;
    }

    if (code == LV_EVENT_PRESSING) {
        if (!s_drag_active) return;
        lv_indev_t *indev = lv_indev_active();
        if (!indev) return;
        lv_point_t pt;
        lv_indev_get_point(indev, &pt);

        int dy = pt.y - s_drag_anchor_y;
        s_drag_total_dy = pt.y - s_drag_start_y;

        int line_h = LINE_H > 0 ? LINE_H : 1;
        if (dy <= -line_h || dy >= line_h) {
            int step = dy / line_h;
            s_drag_anchor_y += step * line_h;

            /* Finger moves down (dy>0, step>0)  -> scroll backward
             *   (reveal lines above, like dragging the page down).
             * Finger moves up   (dy<0, step<0)  -> scroll forward
             *   (reveal lines below, like dragging the page up).
             * Pan only the touched pane; keep the caret / focus put. */
            touch_scroll_pane_by(pane_idx, -step);
        }
        return;
    }

    if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        s_drag_active = false;
        /* s_drag_total_dy is consumed by the CLICKED branch below
         * (LVGL fires CLICKED right after RELEASED on a short press). */
        return;
    }

    if (code == LV_EVENT_CLICKED) {
        /* Suppress the click if the user actually dragged: a swipe
         * should scroll only, never move the caret. */
        if (std::abs(s_drag_total_dy) > TOUCH_DRAG_SUPPRESS_PX) {
            s_drag_total_dy = 0;
            return;
        }
        s_drag_total_dy = 0;

        /* A genuine tap places the caret -> move keyboard focus to the
         * tapped pane and bind it so the pane-local coordinate math and
         * caret update below operate on that pane's document. */
        s_focus = pane_idx;
        pane_bind_focus();

        lv_indev_t *indev = lv_indev_active();
        if (!indev) return;
        lv_point_t pt;
        lv_indev_get_point(indev, &pt);
        /* Translate screen-space point into pane-local coordinates.
         * The pane's content area starts at (s_rp->x, s_rp->y). */
        int lx = pt.x - s_rp->x;
        int ly = pt.y - s_rp->y;
        if (ly < 0) return;

        uint32_t now = lv_tick_get();
        bool is_double = (s_press_ms - s_last_tap_ms) <= TOUCH_DOUBLE_TAP_MS &&
                          (s_last_tap_x >= 0) &&
                          (std::abs((int)(pt.x - s_last_tap_x)) <= TOUCH_DOUBLE_TAP_PX) &&
                          (std::abs((int)(pt.y - s_last_tap_y)) <= TOUCH_DOUBLE_TAP_PX);
        s_last_tap_ms = now;
        s_last_tap_x  = pt.x;
        s_last_tap_y  = pt.y;

        size_t off = 0;
        if (!touch_point_to_offset(lx, ly, &off)) return;

        if (is_double) {
            touch_select_word_at(off);
        } else {
            editor_clear_selection();
            editor_set_cursor(off);
        }
        ensure_cursor_visible();
        editor_ui_refresh();
        standby_reset_timer();
        return;
    }

    if (code == LV_EVENT_GESTURE) {
        lv_indev_t *indev = lv_indev_active();
        if (!indev) return;
        lv_dir_t dir = lv_indev_get_gesture_dir(indev);
        /* Reset the indev's gesture state so it does not fire again
         * on the next refresh (LVGL leaves the last gesture latched
         * until something consumes it). */
        lv_indev_wait_release(indev);

        int step = VISIBLE_LINES > 2 ? VISIBLE_LINES - 1 : 1;
        if (dir == LV_DIR_TOP) {
            /* Finger swiped up -> show content further down. Pan only
             * the touched pane; keep the caret / focus put. */
            touch_scroll_pane_by(pane_idx, +step);
        } else if (dir == LV_DIR_BOTTOM) {
            touch_scroll_pane_by(pane_idx, -step);
        }
        /* Horizontal swipes are ignored for now (no obvious editor
         * action; could be wired to undo/redo later). */
        return;
    }
}

/* Tap-on-row helper for the menu / settings / file-browser lists.
 * Stored as the LV_EVENT_CLICKED callback on every lv_list_add_btn
 * (the row index is stuffed into the button's user_data).
 *
 * Behaviour: tap a row to highlight it (single click) and tap it
 * again to activate (matching the keyboard's "arrow keys to focus
 * + Enter to activate" flow). This avoids accidental activations
 * on imprecise taps and lets the user inspect the highlight before
 * committing. */
typedef void (*list_activate_fn)(int idx);

typedef struct {
    int            *p_sel;       /* selection index storage */
    int            *p_sel_prev;  /* selection-prev storage (NULL if none) */
    lv_obj_t       *list;        /* list widget */
    list_activate_fn activate;   /* fn to call when an already-selected row is tapped */
    int             count;       /* number of items (for clamp) */
} list_touch_ctx_t;

/* One static context per touchable list. The refresh_*() functions
 * fill these in (count, list) and rewire the click callbacks after
 * (re)building rows so user_data is always in sync with the visible
 * row order. */
static list_touch_ctx_t s_menu_touch_ctx;
static list_touch_ctx_t s_settings_touch_ctx;
static list_touch_ctx_t s_browser_touch_ctx;

static void list_touch_event_cb(lv_event_t *e)
{
    list_touch_ctx_t *ctx = (list_touch_ctx_t *)lv_event_get_user_data(e);
    lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
    if (!ctx || !ctx->p_sel || !btn) return;

    /* The row index is the button's position among its siblings
     * (LVGL keeps children in insertion order). Deriving it here
     * avoids stomping the button's user_data slot, which the file
     * browser uses to remember the entry index. */
    int32_t idx32 = lv_obj_get_index(btn);
    int idx = (int)idx32;
    if (idx < 0 || idx >= ctx->count) return;

    if (idx == *ctx->p_sel) {
        /* Tapped the already-highlighted row -- activate it. */
        if (ctx->activate) ctx->activate(idx);
    } else {
        /* First tap on a different row -- just move the highlight. */
        if (ctx->p_sel_prev) *ctx->p_sel_prev = *ctx->p_sel;
        *ctx->p_sel = idx;
        update_list_highlight(ctx->list, *ctx->p_sel,
                              ctx->p_sel_prev ? *ctx->p_sel_prev : -1);
        if (ctx->p_sel_prev) *ctx->p_sel_prev = *ctx->p_sel;
    }
    standby_reset_timer();
}

/* Walk every row of `list` and attach list_touch_event_cb
 * (LV_EVENT_CLICKED) with `ctx` as the user-data pointer. The row
 * index is derived at click time via lv_obj_get_index() so each
 * row's own user_data slot is left alone (the file browser uses
 * that slot to store the underlying s_browser_entries[] index). */
static void list_touch_attach(lv_obj_t *list, list_touch_ctx_t *ctx)
{
    if (!list || !ctx) return;
    ctx->list = list;
    uint32_t n = lv_obj_get_child_count(list);
    ctx->count = (int)n;
    for (uint32_t i = 0; i < n; i++) {
        lv_obj_t *btn = lv_obj_get_child(list, (int32_t)i);
        if (!btn) continue;
        /* Remove any prior callback we registered so a second
         * refresh_*() does not stack duplicates. */
        lv_obj_remove_event_cb(btn, list_touch_event_cb);
        lv_obj_add_event_cb(btn, list_touch_event_cb,
                            LV_EVENT_CLICKED, ctx);
    }
}


/* Tap handler for the BLE-prompt screen's upper-right "Off" button.
 * Drops the device straight into deep sleep -- standby_enter_sleep()
 * runs the pre-sleep autosave + per-board teardown the same way
 * the inactivity timeout and the power-button long-press do, so
 * this is the touch-only equivalent of those paths. */
static void ble_prompt_off_btn_cb(lv_event_t *e)
{
    (void)e;
    standby_enter_sleep();
}

/* Tap handler for the BLE-prompt screen's "Forget KB" button.
 * Clears all stored BLE keyboard pairings and immediately starts
 * scanning for a new keyboard, allowing the user to pair with a
 * different (or freshly reset) keyboard without needing a physical
 * keyboard to navigate the settings menu. */
#if !defined(CONFIG_DRAFTLING_NO_BLUETOOTH)
static void ble_prompt_forget_btn_cb(lv_event_t *e)
{
    (void)e;
    ble_keyboard_forget_all();
}
#endif

#endif /* CONFIG_DRAFTLING_TOUCHSCREEN */

/* ---- File browser ---- */

/* The file browser status bar's "no transient message" text. Normally
 * a fixed hint; while the SD card is handed to a USB host it instead
 * reminds the user which keys still work (see commit_usb_msc_pending_mode()
 * and the guards in browser_activate_item() / handle_browser_key()),
 * since editor_ui_set_status()'s 3 s transient message would otherwise
 * fade back to the normal (now-wrong) hint. Both call sites that used
 * to hard-code "F1:Menu  N:New file" (editor_ui_show_file_browser()
 * and restore_default_status()) go through this instead, so they and
 * the auto-off idle timer (polled in key_drain_cb) all agree. */
/* ---- Project helpers ---- */

static bool browser_at_root(void) { return s_cur_project[0] == '\0'; }

/* Proposed name for a new project: MyDrafts, or MyDrafts2, MyDrafts3,
 * ... when that one exists. */
static void default_project_name(char *out, size_t n)
{
    snprintf(out, n, "%s", PROJECT_DEFAULT_NAME);
    for (int i = 2; i < 100 && project_exists(out); i++)
        snprintf(out, n, "%s%d", PROJECT_DEFAULT_NAME, i);
}

/* The browser's highlighted entry, or NULL (an empty list's
 * placeholder row has no entry). */
static const sd_card_file_entry_t *browser_selected_entry(void)
{
    if (!s_browser_list) return NULL;
    lv_obj_t *btn = lv_obj_get_child(s_browser_list, s_browser_sel);
    if (!btn) return NULL;
    int idx = (int)(intptr_t)lv_obj_get_user_data(btn);
    if (idx < 0 || idx >= s_browser_count) return NULL;
    return &s_browser_entries[idx];
}

/* Folder the browser lists: the card root or the project shown. */
static void browser_dir(char *out, size_t n)
{
    if (browser_at_root()) snprintf(out, n, "%s", sd_card_get_mount_point());
    else project_dir(s_cur_project, out, n);
}

/* The highlighted row is a book (.epub file). */
static const sd_card_file_entry_t *browser_selected_book(void)
{
    const sd_card_file_entry_t *e = browser_selected_entry();
    return (e && !e->is_dir && editor_path_is_book(e->name)) ? e : NULL;
}

/* Project of a document's file, if it lies in one. */
static bool doc_project(const editor_doc_t *d, char *out, size_t n)
{
    const char *p = d ? editor_doc_get_path(d) : NULL;
    return p && project_of_path(p, out, n);
}

/* The project the user works in: in the editor the focused document's
 * project (an untitled one: the last project); in the browser the
 * project shown, or the highlighted one in the list of projects. */
static bool context_project(char *out, size_t n)
{
    out[0] = '\0';
    if (s_editor_screen_active && !s_inpane_browser_open) {
        if (doc_project(s_panes[s_focus].doc, out, n)) return true;
    } else if (!browser_at_root()) {
        snprintf(out, n, "%s", s_cur_project);
        return true;
    } else {
        const sd_card_file_entry_t *e = browser_selected_entry();
        if (e && e->is_dir)
            return snprintf(out, n, "%s", e->name) < (int)n;
        return false;
    }
    if (s_last_project[0] && project_exists(s_last_project)) {
        snprintf(out, n, "%s", s_last_project);
        return true;
    }
    return false;
}

/* Folder where an untitled document is saved. */
static bool new_doc_dir(char *out, size_t n)
{
    project_name_t p;
    if (!browser_at_root() && project_exists(s_cur_project)) snprintf(p, sizeof(p), "%s", s_cur_project);
    else if (s_last_project[0] && project_exists(s_last_project)) snprintf(p, sizeof(p), "%s", s_last_project);
    else return false;
    project_dir(p, out, n);
    return true;
}

static int status_avail_width(void);

/* prefix + the longest of hints (longest first) that fits the status
 * bar; just the prefix if none does. */
static void fit_status_text(char *buf, size_t n, const char *prefix,
                            const char *const *hints, int nhints)
{
    int avail = status_avail_width() / char_width_for_font(FONT_11);
    for (int i = 0; i < nhints; i++) {
        snprintf(buf, n, "%s%s", prefix, hints[i]);
        if (utf8_cells_in_bytes(buf, strlen(buf)) <= avail) return;
    }
    snprintf(buf, n, "%s", prefix);
}

static void browser_status_hint(char *buf, size_t buf_size)
{
#if defined(CONFIG_DRAFTLING_HAS_USB_MSC)
    usb_msc_mode_t mode = usb_msc_get_mode();
    if (mode != USB_MSC_MODE_OFF) {
        snprintf(buf, buf_size, "SD card via USB (%s) -- F1 to disable",
                 mode == USB_MSC_MODE_READ_WRITE ? "read-write" : "read-only");
        return;
    }
#endif
#if defined(CONFIG_DRAFTLING_HAS_KERMIT)
    if (kermit_server_is_running()) {
        snprintf(buf, buf_size, "Sync via browser active");
        return;
    }
#endif
    if (browser_at_root()) {
        static const char *const hints[] = {
            "F1:Menu  N:New project  F2:Rename  Del:Delete  Alt+I:Info",
            "F1:Menu  N:New project  F2:Rename",
            "F1:Menu  N:New project",
            "F1:Menu",
        };
        fit_status_text(buf, buf_size, "Projects  ", hints, 4);
    } else {
        static const char *const hints[] = {
            "F1:Menu  N:New  F2:Rename  Alt+M:Move  Alt+C:Copy  Esc:Projects",
            "F1:Menu  N:New  F2:Rename  Esc:Projects",
            "F1:Menu  N:New  Esc:Projects",
            "F1:Menu",
        };
        char prefix[PROJECT_NAME_MAX + 4];
        snprintf(prefix, sizeof(prefix), "%s  ", s_cur_project);
        fit_status_text(buf, buf_size, prefix, hints, 4);
    }
}

static void refresh_file_list(const char *select_name = NULL)
{
    /* Remember the currently-selected entry's filename (if any) so we
     * can restore the selection after the list is rebuilt. This keeps
     * the highlight where the user left it when returning from the
     * editor, and also preserves selection across a Git-sync refresh
     * that added or removed files. select_name, when given, is
     * selected instead (e.g. a file's new name after a rename). */
    char remembered_name[sizeof(((sd_card_file_entry_t *)0)->name)] = {0};
    if (select_name) {
        strncpy(remembered_name, select_name, sizeof(remembered_name) - 1);
    } else if (s_browser_list && s_browser_sel >= 0 &&
        s_browser_sel < (int)lv_obj_get_child_count(s_browser_list)) {
        lv_obj_t *cur_btn = lv_obj_get_child(s_browser_list, s_browser_sel);
        if (cur_btn) {
            int cur_idx = (int)(intptr_t)lv_obj_get_user_data(cur_btn);
            if (cur_idx >= 0 && cur_idx < s_browser_count) {
                snprintf(remembered_name, sizeof(remembered_name), "%s",
                         s_browser_entries[cur_idx].name);
            }
        }
    }

    /* A project that went away (renamed or deleted elsewhere) drops
     * the browser back to the list of projects. */
    if (!browser_at_root() && !project_exists(s_cur_project)) {
        snprintf(remembered_name, sizeof(remembered_name), "%s", s_cur_project);
        s_cur_project[0] = '\0';
    }

    sd_card_free_list(s_browser_entries);
    s_browser_entries = NULL;
    s_browser_count = 0;
    char dir[64];
    if (browser_at_root()) snprintf(dir, sizeof(dir), "%s", sd_card_get_mount_point());
    else project_dir(s_cur_project, dir, sizeof(dir));
    if (sd_card_list_dir(dir, &s_browser_entries, &s_browser_count) != ESP_OK) {
        s_browser_entries = NULL;
        s_browser_count = 0;
        if (sd_card_is_mounted()) editor_ui_set_status("Cannot read the " STORAGE_NAME " folder");
    }
    if (browser_at_root()) {
        /* The list of projects: the folders in the SD card root with a
         * valid project name (as project_list() picks them), then the
         * books (.epub) in the root. */
        int n = 0;
        for (int i = 0; i < s_browser_count; i++) {
            const sd_card_file_entry_t &e = s_browser_entries[i];
            bool keep = e.is_dir ? !project_name_problem(e.name)
                                 : editor_path_is_book(e.name);
            if (!keep) continue;
            if (!e.is_dir) {
                if (n != i) s_browser_entries[n] = s_browser_entries[i];
                n++;
                continue;
            }
            if (n != i) s_browser_entries[n] = s_browser_entries[i];
            s_browser_entries[n].size = 0;
            n++;
        }
        s_browser_count = n;
    }

    lv_obj_clean(s_browser_list);

    int restored_row = -1;
    int row = 0;
    for (int i = 0; i < s_browser_count; i++) {
        const char *name = s_browser_entries[i].name;
        /* In a project only its documents and books are listed. */
        bool show = browser_at_root() ? true
                  : (!s_browser_entries[i].is_dir &&
                     (editor_path_format(name, NULL) || editor_path_is_book(name)));
        if (show) {
            char label[sizeof(s_browser_entries[0].name) + 8];
            if (s_browser_entries[i].is_dir)
                snprintf(label, sizeof(label), "%.255s/", name);
            else
                snprintf(label, sizeof(label), "  %.255s", name);
            lv_obj_t *btn = lv_list_add_btn(s_browser_list, NULL, label);
            lv_obj_set_user_data(btn, (void *)(intptr_t)i);
            if (remembered_name[0] && strcmp(name, remembered_name) == 0) {
                restored_row = row;
            }
            row++;
        }
    }
    if (row == 0) {
        /* Placeholder row: no entry behind it (user data -1). */
        lv_obj_t *btn = lv_list_add_btn(s_browser_list, NULL,
                                        browser_at_root() ? "(no projects -- N: new project)"
                                                          : "(no files -- N: new file)");
        lv_obj_set_user_data(btn, (void *)(intptr_t)-1);
    }
    if (s_inpane_hdr) {
        char hdr[PROJECT_NAME_MAX + 32];
        if (browser_at_root()) snprintf(hdr, sizeof(hdr), "Open into pane: projects");
        else snprintf(hdr, sizeof(hdr), "Open into pane: %s", s_cur_project);
        lv_label_set_text(s_inpane_hdr, hdr);
    }
    s_browser_sel = (restored_row >= 0) ? restored_row : 0;
    /* Clamp in case the list shrank below the saved selection. */
    int new_count = (int)lv_obj_get_child_count(s_browser_list);
    if (s_browser_sel >= new_count)
        s_browser_sel = (new_count > 0) ? new_count - 1 : 0;
    /* List was rebuilt -- no item carries a highlight yet, so the
     * next update_list_highlight() call should not try to "clear"
     * a stale previous selection. */
    s_browser_sel_prev = -1;
    /* Apply the editor theme colors (and initial highlight) to every
     * row so list buttons render with theme_fg() text on theme_bg(),
     * not the LVGL default greys that would be near-invisible on a
     * black background. Mirror what refresh_menu_items() does. */
    apply_list_selection_styles(s_browser_list, s_browser_sel);
    s_browser_sel_prev = s_browser_sel;

#if defined(CONFIG_DRAFTLING_TOUCHSCREEN)
    /* Attach tap-to-select / tap-again-to-activate handlers to the
     * file browser rows. Each row's user_data already carries the
     * s_browser_entries[] index for the Enter-key path; the touch
     * callback derives the visual row number via lv_obj_get_index()
     * so user_data is preserved. */
    s_browser_touch_ctx.p_sel      = &s_browser_sel;
    s_browser_touch_ctx.p_sel_prev = &s_browser_sel_prev;
    s_browser_touch_ctx.activate   = browser_activate_item;
    list_touch_attach(s_browser_list, &s_browser_touch_ctx);
#endif

    sync_battery_labels();
}

static void restore_default_status(void);
static void set_status_with_timeout(const char *msg, uint32_t timeout_ms);
static void show_msg_popup(const char *title, const char *text, bool busy);
static void show_name_prompt(save_mode_t mode, const char *hdr, const char *initial);

/* Open a project in the browser, with select (or the project's last
 * selected file) highlighted. */
static void browser_enter_project(const char *name, const char *select = NULL)
{
    snprintf(s_cur_project, sizeof(s_cur_project), "%s", name);
    snprintf(s_last_project, sizeof(s_last_project), "%s", name);
    projects_set_last(name);
    char last[256] = "";
    if (!select) {
        project_get_last_file(name, last, sizeof(last));
        select = last;
    }
    s_browser_sel = 0;
    refresh_file_list(select[0] ? select : NULL);
    restore_default_status();
}

/* Remember the highlighted file as the project's last selection. */
static void browser_remember_selection(void)
{
    if (browser_at_root()) return;
    const sd_card_file_entry_t *e = browser_selected_entry();
    if (e && !e->is_dir) project_set_last_file(s_cur_project, e->name);
}

/* From a project back to the list of projects. */
static void browser_go_up(void)
{
    if (browser_at_root()) return;
    browser_remember_selection();
    project_name_t old;
    snprintf(old, sizeof(old), "%s", s_cur_project);
    s_cur_project[0] = '\0';
    refresh_file_list(old);
    restore_default_status();
}

/* Start-up checks of the card layout, when the file browser shows:
 * the card is missing; documents lie in the root folder (from before
 * projects existed, or copied there over USB); no project exists yet
 * (a new or freshly formatted card). */
static void check_project_setup(void)
{
    if (s_save_open || s_msg_open || s_menu_open || s_setup_busy) return;
    if (sd_handed_off_msg()) return;
    if (!sd_card_is_ready()) {
        if (s_setup_dismissed) return;
        s_setup_dismissed = true;
#if defined(CONFIG_DRAFTLING_STORAGE_FLASH)
        show_msg_popup("No internal storage",
                       "The internal storage cannot be mounted.\n"
                       "Restart the device.", false);
#else
        show_msg_popup("No SD card",
                       "The SD card is missing or cannot be read.\n"
                       "Insert a MicroSD card and restart the device.", false);
#endif
        s_msg_restart = true;
        lv_label_set_text(s_msg_hint_lbl, "Enter: restart");
        return;
    }
    if (s_setup_dismissed) return;
    const int SHOW = 6;
    project_file_name_t names[SHOW];
    int ndocs = project_root_doc_names(names, SHOW);
    if (ndocs > 0) {
        std::string text = ndocs == 1 ? "This file does not belong to any project:\n"
                                      : "These files do not belong to any project:\n";
        for (int i = 0; i < ndocs && i < SHOW; i++) {
            text += "  ";
            text += names[i];
            text += '\n';
        }
        if (ndocs > SHOW) {
            char more[48];
            snprintf(more, sizeof(more), "  ...and %d more\n", ndocs - SHOW);
            text += more;
        }
        bool have_projects = project_any();
        if (have_projects)
            text += ndocs == 1 ? "Move it to a project?" : "Move them to a project?";
        else
            text += ndocs == 1 ? "Move it to a new project?" : "Move them to a new project?";
        char cfg[32];
        snprintf(cfg, sizeof(cfg), "%s/git.cfg", sd_card_get_mount_point());
        if (sd_card_file_exists(cfg))
            text += have_projects
                ? " Their Git sync settings (git.cfg) and history go with them, "
                  "unless the project has its own."
                : " Their Git sync settings (git.cfg) and history go with them.";
        show_msg_popup("Files outside projects", text.c_str(), false);
        s_msg_migrate = true;
        lv_label_set_text(s_msg_hint_lbl, have_projects
                              ? "Enter: move to a project  Esc: power off"
                              : "Enter: move to a new project  Esc: power off");
        return;
    }
    if (!project_any()) {
        show_name_prompt(SAVE_MODE_PROJECT_FIRST, "Create a project (Enter):",
                         PROJECT_DEFAULT_NAME);
        set_status_with_timeout("Files are kept in projects. Name the first one", 0);
    }
}

extern "C" void editor_ui_show_file_browser(void)
{
    /* The row to highlight: the file the editor had open, or on the
     * first call the last project's last selected file. */
    char select[256] = "";
    if (!s_browser_started && sd_card_is_ready()) {
        /* First time: reopen the project the user last worked in. */
        s_browser_started = true;
        projects_get_last(s_last_project, sizeof(s_last_project));
        if (s_last_project[0] && project_exists(s_last_project)) {
            snprintf(s_cur_project, sizeof(s_cur_project), "%s", s_last_project);
            project_get_last_file(s_cur_project, select, sizeof(select));
        } else {
            s_last_project[0] = '\0';
        }
    }
    if (s_pane_count > 1) {
        /* Split: overlay a compact file selector on just the focused
         * pane, keeping the other pane's document on screen. The editor
         * screen stays active; the in-pane selector flag (checked first
         * in process_key_event) reroutes keys to the browser handler. */

        /* The in-pane selector is drawn as a child of the editor screen
         * (s_scr). At boot (or after a BLE (re)connect) the split layout
         * is restored before any editor screen has been loaded -- the
         * active LVGL screen is still the BLE prompt. Loading the overlay
         * onto an off-screen canvas leaves the prompt frozen on screen
         * and swallows keystrokes, so make the editor screen active and
         * paint both panes before overlaying the selector. */
        s_editor_screen_active = true;
        if (lv_scr_act() != s_scr) {
            lv_scr_load(s_scr);
            editor_ui_refresh();
        }
        show_inpane_browser();
        check_project_setup();
        return;
    }

    /* Mark the browser as the active screen up front so key routing and
     * refresh gating switch over immediately, regardless of the focused
     * pane's per-document editor mode (see s_editor_screen_active). */
    s_editor_screen_active = false;

    /* Single-pane: opening a file replaces the current document, so
     * close it now (historical behavior; the buffer is reloaded
     * from disk when a file is picked). */
    {
        /* Back in the project of the document being closed; a book in
         * the root folder is shown in the list of projects. A book in
         * any other folder leaves the browser where it was. */
        const char *ap = editor_get_file_path();
        project_name_t dp;
        const char *mp = sd_card_get_mount_point();
        size_t ml = strlen(mp);
        if (ap && project_of_path(ap, dp, sizeof(dp))) {
            snprintf(s_cur_project, sizeof(s_cur_project), "%s", dp);
            snprintf(s_last_project, sizeof(s_last_project), "%s", dp);
            snprintf(select, sizeof(select), "%s", strrchr(ap, '/') + 1);
            project_set_last_file(dp, select);
        } else if (ap && editor_path_is_book(ap) && !strncmp(ap, mp, ml) &&
                   ap[ml] == '/' && !strchr(ap + ml + 1, '/')) {
            s_cur_project[0] = '\0';
            snprintf(select, sizeof(select), "%s", ap + ml + 1);
        }
    }
    editor_close_file();
    s_open_target_pane = 0;
    /* Operate on the full-screen browser list. */
    s_browser_list = s_list_files;
    refresh_file_list(select[0] ? select : NULL);

    /* The Wi-Fi connection state is conveyed by the Wi-Fi icon; the
     * status bar shows the static hint so it doesn't compete with
     * transient messages (e.g. Git-sync progress). */
    if (s_lbl_br_status) {
        char hint[160];
        browser_status_hint(hint, sizeof(hint));
        lv_label_set_text(s_lbl_br_status, hint);
    }

    sync_battery_labels();
#if defined(CONFIG_DRAFTLING_DISPLAY_EPD)
    /* This is the actual boot-time first screen on e-paper (the boot
     * flow shows the file browser before the editor -- see
     * editor_ui_init()). epd_full_screen_repaint() doesn't apply here:
     * it only invalidates s_scr (the editor screen), not s_scr_browser,
     * so it wouldn't force this screen's own widgets to redraw. Same
     * underlying issue as the two lv_scr_load() call sites already
     * fixed for the editor screen (apply_pending_connect_state()'s
     * restore branch, editor_ui_show_editor()): a bare lv_scr_load()
     * does not reliably force every child widget to redraw from a
     * blank/stale framebuffer on this backend. Wipe the framebuffer
     * and invalidate this screen explicitly instead of relying on
     * lv_scr_load() alone. */
    display_clear(0xFF);
    lv_obj_invalidate(s_scr_browser);
#endif
    lv_scr_load(s_scr_browser);
    check_project_setup();
}

extern "C" void editor_ui_show_editor(void)
{
    s_editor_screen_active = true;
    /* The default text names the focused document's project. */
    if (!s_status_clear_timer) restore_default_status();
    sync_battery_labels();
    lv_scr_load(s_scr);
#if defined(CONFIG_DRAFTLING_DISPLAY_EPD)
    /* This is the actual boot-time path into the editor on e-paper --
     * the boot flow shows the file browser first (see editor_ui_init()),
     * so this runs on the very first file/new-file pick of a session,
     * not apply_pending_connect_state()'s "restore" branch (which only
     * applies to a *second* connect after the editor was already
     * showing). Same missing-repaint bug: refresh_active_pane()'s
     * per-line cache and update_title_bar()'s s_prev_title are not
     * reset by a bare lv_scr_load()+editor_ui_refresh(), so a screen
     * that starts blank (as s_scr does before its first real paint)
     * stays blank until something else forces a full redraw. Matches
     * the fix already applied to apply_pending_connect_state(). */
    epd_full_screen_repaint();
#endif
    editor_ui_refresh();
}

/* Available pixel width for the editor's bottom status label.
 * Reserves room on the right for the battery percentage (when the
 * board has a battery) and the small WiFi icon, so the status text
 * never overlaps them and -- importantly -- never wraps to a second
 * line (which would extend past SCR_H and cause LVGL to draw a
 * screen scrollbar). The numbers mirror the layout in build_screens()
 * for s_lbl_dev_batt and s_img_wifi. */
static int status_avail_width(void)
{
    int right_reserved;
#if defined(CONFIG_DRAFTLING_HAS_BATTERY)
    /* Battery occupies SCR_W-80..SCR_W-2; WiFi icon sits at SCR_W-95.
     * Leave a 2 px gap to the wifi icon. */
    right_reserved = 95 + 2;
#else
    /* No battery; only the wifi icon at SCR_W-15. */
    right_reserved = 15 + 2;
#endif
    int avail = CONTENT_W - 2 /* left margin */ - right_reserved - STA_IN_L - STA_IN_R;
    if (avail < 0) avail = 0;
    return avail;
}

/* Hide the editor's status label when its current text is wider than
 * the available area (so only the battery percentage remains visible
 * in the bottom bar); show it otherwise. Greybeard is monospaced, so
 * the rendered width is cells * char_width(FONT_11) (a CJK character
 * takes two cells). */
static void update_status_visibility(void)
{
    if (!s_lbl_status) return;
    const char *text = lv_label_get_text(s_lbl_status);
    if (!text) text = "";
    int cells = utf8_cells_in_bytes(text, strlen(text));
    int text_w = cells * char_width_for_font(FONT_11);
    if (text_w > status_avail_width()) {
        lv_obj_add_flag(s_lbl_status, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_remove_flag(s_lbl_status, LV_OBJ_FLAG_HIDDEN);
    }
}

/* Default status-bar text for the editor screen: the focused
 * document's project, then as many key hints as fit. Kept in one
 * place so the auto-clear timer and the initial label setup agree. */
static void editor_default_status(char *buf, size_t n)
{
    static const char *const hints[] = {
        "F1:Menu Ctrl+S:Save Ctrl+L:Layout Ctrl+G:Git Esc:Files",
        "F1:Menu Ctrl+S:Save Ctrl+G:Git Esc:Files",
        "F1:Menu Esc:Files",
        "F1:Menu",
    };
    project_name_t p = "";
    if (!doc_project(s_panes[s_focus].doc, p, sizeof(p)) && s_last_project[0])
        snprintf(p, sizeof(p), "%s", s_last_project);
    char prefix[PROJECT_NAME_MAX + 4] = "";
    if (p[0]) snprintf(prefix, sizeof(prefix), "%s | ", p);
    fit_status_text(buf, n, prefix, hints, 4);
}

/* Restore the status bar of both screens to its standard, no-message
 * state (the same text the screens show when they are first entered).
 * The WiFi connection state is communicated by the Wi-Fi icon in the
 * title / status bar, so the default text never mentions it -- otherwise
 * it would reappear between transient Git-sync progress messages and
 * confuse the user. */
static void restore_default_status(void)
{
    if (s_lbl_status) {
        char st[160];
        editor_default_status(st, sizeof(st));
        lv_label_set_text(s_lbl_status, st);
    }
    update_status_visibility();
    if (s_lbl_br_status) {
        char hint[160];
        browser_status_hint(hint, sizeof(hint));
        lv_label_set_text(s_lbl_br_status, hint);
    }
}

/* One-shot LVGL timer callback: clear the transient status message. */
static void status_clear_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    /* The timer auto-deletes after firing because we set its repeat
     * count to 1, so just drop our reference. */
    s_status_clear_timer = NULL;
    restore_default_status();
}

/* Cancel any pending auto-clear timer and restore the default status
 * bar text immediately. Safe to call when no timer is pending (no-op
 * in that case). Used by call sites that act as the "user moved on"
 * signal -- e.g. selecting a different file in the browser. */
static void cancel_status_clear_and_restore(void)
{
    if (s_status_clear_timer) {
        lv_timer_delete(s_status_clear_timer);
        s_status_clear_timer = NULL;
        restore_default_status();
    }
}

/* Internal: post a status message with a caller-chosen auto-clear
 * timeout. editor_ui_set_status() is the public wrapper that picks
 * the standard 3-second window. */
static void set_status_with_timeout(const char *msg, uint32_t timeout_ms)
{
    ESP_LOGI(TAG, "Status: %s", msg);
    if (s_lbl_status) lv_label_set_text(s_lbl_status, msg);
    if (s_lbl_br_status) lv_label_set_text(s_lbl_br_status, msg);
    update_status_visibility();
    bars_show();
    if (s_status_clear_timer) {
        lv_timer_delete(s_status_clear_timer);
        s_status_clear_timer = NULL;
    }
    if (timeout_ms > 0) {
        s_status_clear_timer = lv_timer_create(status_clear_timer_cb, timeout_ms, NULL);
        if (s_status_clear_timer) {
            lv_timer_set_repeat_count(s_status_clear_timer, 1);
        }
    }
    sync_battery_labels();
}

extern "C" void editor_ui_set_status(const char *msg)
{
    /* Auto-clear the message after 3 seconds so transient errors
     * (e.g. "File too large") do not linger after the user has moved
     * on to a different file or screen. The timer is one-shot and is
     * recreated on every set_status call, so successive messages each
     * get their own 3-second window. */
    set_status_with_timeout(msg, 3000);
}

extern "C" void editor_ui_show_fatal(const char *msg)
{
    /* Full-screen, centered, word-wrapped variant of
     * editor_ui_set_status() for unrecoverable boot-time errors
     * (e.g. ESP-Hosted link to the on-board ESP32-C6 not coming up).
     *
     * The regular status label is FONT_11 with LV_LABEL_LONG_CLIP and
     * a width constrained by status_avail_width(), so a multi-line
     * instruction message would be silently clipped to its first line.
     * Instead we build a dedicated screen and load it, so the message
     * fills the panel and survives any future editor_ui_set_status()
     * calls. The screen stays on until power-cycle / reset. */
    ESP_LOGE(TAG, "Fatal: %s", msg);
    if (!msg) msg = "";

    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, theme_bg(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(scr, 10, 0);
    lv_obj_set_style_border_width(scr, 0, 0);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(scr);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl, SCR_W - 20);
    lv_obj_set_style_text_font(lbl, FONT_14, 0);
    lv_obj_set_style_text_color(lbl, theme_fg(), 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(lbl, msg);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);

    lv_scr_load(scr);
}

extern "C" void editor_ui_set_board_name(const char *name)
{
    s_board_name = name;
}

extern "C" void editor_ui_set_coprocessor(const char *chip, const char *firmware)
{
    s_coproc_chip = chip;
    s_coproc_fw = firmware;
}

extern "C" void editor_ui_set_ble_prompt_text(const char *text)
{
    /* Update the bottom-half label on the boot "BLE prompt" screen.
     * editor_ui_init() loads this screen with a generic "Initializing..."
     * caption because at that point we do not yet know whether a
     * USB keyboard will enumerate; main.cpp calls this function once
     * it has decided to bring up BLE, so the "Searching for BLE
     * keyboard..." message only appears on the no-USB path. */
    if (!s_ble_prompt_lbl || !text) return;
    /* main.cpp mounts the card before this is called: say so here too,
     * since the file browser (with its "insert the SD card" pop-up)
     * only shows once a keyboard is connected. */
    if (!sd_handed_off_msg() && !sd_card_is_ready()) {
        std::string t(text);
#if defined(CONFIG_DRAFTLING_STORAGE_FLASH)
        t += "\n\nThe internal storage cannot be mounted.\nRestart the device.";
#else
        t += "\n\nNo SD card found.\nInsert a MicroSD card and restart the device.";
#endif
        lv_label_set_text(s_ble_prompt_lbl, t.c_str());
        return;
    }
    lv_label_set_text(s_ble_prompt_lbl, text);
}

/* In the file browsers only the selected row scrolls a name too long
 * for the row; the other rows show its beginning, clipped. Switching
 * the long mode also resets the scroll offset to the start. */
static void list_row_set_scroll(lv_obj_t *list, lv_obj_t *btn, bool scroll)
{
    if (list != s_list_files && list != s_inpane_list) return;
    lv_obj_t *lbl = lv_obj_get_child_by_type(btn, 0, &lv_label_class);
    if (!lbl) return;
    lv_label_long_mode_t mode = scroll ? LV_LABEL_LONG_MODE_SCROLL_CIRCULAR
                                       : LV_LABEL_LONG_MODE_CLIP;
    if (lv_label_get_long_mode(lbl) != mode) lv_label_set_long_mode(lbl, mode);
}

/* Apply the "selected" styling to item `sel` and the "unselected"
 * styling to every other item.  Used after a full list rebuild. */
static void apply_list_selection_styles(lv_obj_t *list, int sel)
{
    uint32_t count = lv_obj_get_child_count(list);
    for (uint32_t i = 0; i < count; i++) {
        lv_obj_t *child = lv_obj_get_child(list, i);
        list_row_set_scroll(list, child, (int)i == sel);
        if ((int)i == sel) {
            lv_obj_set_style_bg_color(child, theme_fg(), 0);
            lv_obj_set_style_bg_opa(child, LV_OPA_COVER, 0);
            lv_obj_set_style_text_color(child, theme_bg(), 0);
        } else {
            lv_obj_set_style_bg_opa(child, LV_OPA_TRANSP, 0);
            lv_obj_set_style_text_color(child, theme_fg(), 0);
        }
    }
    /* After a full rebuild make sure the selected row is on screen.
     * Without this, a list taller than the panel (e.g. on PaperS3 with
     * the larger Hack font) keeps its previous scroll offset
     * and the highlight may land outside the visible area. */
    if (sel >= 0 && (uint32_t)sel < count) {
        lv_obj_scroll_to_view(lv_obj_get_child(list, sel), LV_ANIM_OFF);
    }
}

/* Move the highlight from `prev_sel` to `sel` by restyling only those
 * two items.  This keeps the LVGL dirty rectangle limited to a couple
 * of menu rows instead of the whole list, so e-paper backends can
 * issue a partial refresh of just the affected area on Up/Down
 * navigation. */
static void update_list_highlight(lv_obj_t *list, int sel, int prev_sel)
{
    if (prev_sel == sel) return;
    uint32_t count = lv_obj_get_child_count(list);
    if (prev_sel >= 0 && (uint32_t)prev_sel < count) {
        lv_obj_t *prev = lv_obj_get_child(list, prev_sel);
        lv_obj_set_style_bg_opa(prev, LV_OPA_TRANSP, 0);
        lv_obj_set_style_text_color(prev, theme_fg(), 0);
        list_row_set_scroll(list, prev, false);
    }
    if (sel >= 0 && (uint32_t)sel < count) {
        lv_obj_t *cur = lv_obj_get_child(list, sel);
        list_row_set_scroll(list, cur, true);
        lv_obj_set_style_bg_color(cur, theme_fg(), 0);
        lv_obj_set_style_bg_opa(cur, LV_OPA_COVER, 0);
        lv_obj_set_style_text_color(cur, theme_bg(), 0);
        /* Scroll the list so the highlighted row is always visible.
         * Needed when the list is taller than the panel (e.g. on
         * PaperS3 with the larger Hack font, where only a few
         * items fit on screen).  LV_ANIM_OFF avoids smooth-scroll
         * animation, which would force many extra e-paper refreshes. */
        lv_obj_scroll_to_view(cur, LV_ANIM_OFF);
    }
}

/* F1 menu and Settings letter accelerators: the letter that picks a
 * row on its level (resolved with kb_layout_shortcut_char(), like the
 * file browser's N), 0 for none. menu_mark_hotkeys() paints the first
 * occurrence of the letter (either case) in the row's label bold; every
 * label text before that letter is ASCII, so the index found is also a
 * byte offset. */
static char menu_item_hotkey(menu_item_t id)
{
    switch (id) {
    case MENU_ITEM_SETTINGS:    return 's';  /* "Settings..." */
    case MENU_ITEM_KB_LAYOUT:   return 'k';  /* "Keyboard: ..." */
    case MENU_ITEM_SYNC_MENU:   return 'y';  /* "Sync..." */
    case MENU_ITEM_CONN_MENU:   return 'c';  /* "Connectivity..." */
    case MENU_ITEM_SYSTEM_MENU: return 't';  /* "System & Storage..." */
    case MENU_ITEM_HELP:        return 'h';  /* "Help (F10)" */
    case MENU_ITEM_BOOKS:       return 'b';  /* "Open a book..." */
    case MENU_ITEM_QUIT:        return 'q';  /* "Quit Draftling" */
    case MENU_ITEM_GIT_SYNC:    return 's';  /* "Synchronize: ..." */
    case MENU_ITEM_DRAFTBOX:    return 'c';  /* "Connect a Draftbox repository..." */
    case MENU_ITEM_KERMIT:      return 'b';  /* "Sync via browser..." */
    case MENU_ITEM_BLE_MENU:    return 'b';  /* "Bluetooth..." */
    case MENU_ITEM_WIFI_MENU:   return 'w';  /* "WiFi..." */
    case MENU_ITEM_BLE_SCAN:    return 's';  /* "Scan for keyboard" */
    case MENU_ITEM_WIFI_TOGGLE:
        return wifi_manager_is_connected() ? 'd' : 'c'; /* "Disconnect" / "Connect..." */
    case MENU_ITEM_WIFI_NEW:    return 'n';  /* "Choose network..." */
    case MENU_ITEM_HWINFO:      return 'i';  /* "Hardware information" */
    case MENU_ITEM_USB_MSC:     return 'u';  /* "SD card via USB: ..." */
    case MENU_ITEM_FORMAT_SD:   return 'f';  /* "Format SD card..." */
    default:                    return 0;
    }
}

/* Character index of hotkey letter key (either case) in txt, or -1. */
static int hotkey_pos(const char *txt, char key)
{
    if (!txt || !key) return -1;
    for (int i = 0; txt[i]; i++) {
        if ((unsigned char)txt[i] >= 0x80) return -1;
        if (tolower((unsigned char)txt[i]) == key) return i;
    }
    return -1;
}

/* Faux-bold spills a pixel or two past the label box. */
static void menu_hotkey_ext_size_cb(lv_event_t *e)
{
    lv_event_set_ext_draw_size(e, 2);
}

/* LV_EVENT_DRAW_MAIN_END handler of a menu row label: redraws the
 * accelerator letter one pixel to the right (the editor's faux-bold,
 * see line_deco_draw_cb()). lv_list labels scroll circularly when the
 * text does not fit, so follow the label's scroll offset and its
 * second, wrapped-around copy the same way lv_label draws them. */
static void menu_hotkey_draw_cb(lv_event_t *e)
{
    uint32_t ci = (uint32_t)(uintptr_t)lv_event_get_user_data(e);
    lv_obj_t *label = lv_event_get_target_obj(e);
    lv_layer_t *layer = lv_event_get_layer(e);
    const char *txt = lv_label_get_text(label);
    if (!txt || strlen(txt) <= ci) return;

    const lv_font_t *font = lv_obj_get_style_text_font(label, LV_PART_MAIN);
    int32_t bdx = lv_font_get_line_height(font) >= 40 ? 2 : 1;
    lv_area_t ca;
    lv_obj_get_content_coords(label, &ca);
    lv_point_t pos;
    lv_label_get_letter_pos(label, ci, &pos);

    lv_draw_label_dsc_t ld;
    lv_draw_label_dsc_init(&ld);
    ld.font  = font;
    ld.opa   = LV_OPA_COVER;
    ld.color = lv_obj_get_style_text_color(label, LV_PART_MAIN);

    const lv_label_t *lb = (const lv_label_t *)label;
    int32_t x = ca.x1 + pos.x + lb->offset.x + bdx;
    int32_t y = ca.y1 + pos.y + lb->offset.y;
    int32_t wrap = 0;
    if (lb->long_mode == LV_LABEL_LONG_MODE_SCROLL_CIRCULAR &&
        lb->text_size.x > lv_area_get_width(&ca)) {
        wrap = lb->text_size.x +
               lv_font_get_glyph_width(font, ' ', ' ') * LV_LABEL_WAIT_CHAR_COUNT;
    }

    lv_area_t orig = layer->_clip_area;
    lv_area_t clip = {
        LV_MAX(orig.x1, ca.x1), LV_MAX(orig.y1, ca.y1),
        LV_MIN(orig.x2, ca.x2 + bdx), LV_MIN(orig.y2, ca.y2)
    };
    if (clip.x1 > clip.x2 || clip.y1 > clip.y2) return;
    layer->_clip_area = clip;
    lv_point_t pt = { x, y };
    lv_draw_character(layer, &ld, &pt, (uint8_t)txt[ci]);
    if (wrap) {
        pt.x = x + wrap;
        lv_draw_character(layer, &ld, &pt, (uint8_t)txt[ci]);
    }
    layer->_clip_area = orig;
}

/* Paint row's hotkey letter bold (see menu_item_hotkey()). */
static void list_mark_hotkey(lv_obj_t *list, int row, char key)
{
    lv_obj_t *btn = lv_obj_get_child(list, row);
    lv_obj_t *lbl = btn ? lv_obj_get_child(btn, 0) : NULL;
    if (!lbl) return;
    int pos = hotkey_pos(lv_label_get_text(lbl), key);
    if (pos < 0) return;
    lv_obj_add_event_cb(lbl, menu_hotkey_ext_size_cb,
                        LV_EVENT_REFR_EXT_DRAW_SIZE, NULL);
    lv_obj_add_event_cb(lbl, menu_hotkey_draw_cb, LV_EVENT_DRAW_MAIN_END,
                        (void *)(uintptr_t)pos);
    lv_obj_refresh_ext_draw_size(lbl);
}

static menu_level_t menu_parent_level(menu_level_t lvl)
{
    switch (lvl) {
    case MENU_LVL_BLE:
    case MENU_LVL_WIFI: return MENU_LVL_CONN;
    default:            return MENU_LVL_ROOT;
    }
}

static const char *menu_level_header(menu_level_t lvl)
{
    switch (lvl) {
    case MENU_LVL_SYNC:   return "Menu > Sync - Enter to select, Esc to go back";
    case MENU_LVL_CONN:   return "Menu > Connectivity - Esc to go back";
    case MENU_LVL_BLE:    return "Menu > Connectivity > Bluetooth - Esc: back";
    case MENU_LVL_WIFI:   return "Menu > Connectivity > WiFi - Esc: back";
    case MENU_LVL_SYSTEM: return "Menu > System & Storage - Esc to go back";
    default:              return MENU_HDR_TEXT;
    }
}

/* Append one row of the current level. */
static void menu_add(menu_item_t id, const char *label)
{
    if (s_menu_item_count >= MENU_MAX_ITEMS) return;
    s_menu_items[s_menu_item_count++] = id;
    lv_list_add_btn(s_menu_list, NULL, label);
}

static void menu_activate_item(int row);

/* Rebuild the rows of the current menu level (s_menu_level). */
static void refresh_menu_items(void)
{
    lv_obj_clean(s_menu_list);
    s_menu_item_count = 0;
    if (s_lbl_menu_hdr) lv_label_set_text(s_lbl_menu_hdr, menu_level_header(s_menu_level));

    char buf[80];

    switch (s_menu_level) {
    case MENU_LVL_ROOT:
        menu_add(MENU_ITEM_SETTINGS, "Settings...");
        /* The keyboard layout row only makes sense when there is more
         * than one layout to cycle through (F1 -> Settings -> Keyboard
         * -> Active layouts). */
        if (kb_layout_active_count() > 1) {
            snprintf(buf, sizeof(buf), "Keyboard: %s  (Enter to cycle)",
                     kb_layout_name(kb_layout_get()));
            menu_add(MENU_ITEM_KB_LAYOUT, buf);
        }
        menu_add(MENU_ITEM_SYNC_MENU, "Sync...");
#if !(defined(CONFIG_DRAFTLING_NETWORK_HOST_MANAGED) && defined(CONFIG_DRAFTLING_NO_BLUETOOTH))
        /* Nothing to show without WiFi control and Bluetooth (the
         * Flatpak). */
        menu_add(MENU_ITEM_CONN_MENU, "Connectivity...");
#endif
        menu_add(MENU_ITEM_SYSTEM_MENU, "System & Storage...");
        if (sd_card_is_mounted() && !sd_handed_off_msg())
            menu_add(MENU_ITEM_BOOKS, "Open a book...");
        menu_add(MENU_ITEM_SLEEP, "Sleep now");
#if defined(CONFIG_DRAFTLING_HAS_QUIT)
        menu_add(MENU_ITEM_QUIT, "Quit Draftling");
#endif
        menu_add(MENU_ITEM_HELP, "Help (F10)");
        menu_add(MENU_ITEM_CLOSE, "Close menu (Esc / F1)");
        break;

    case MENU_LVL_SYNC:
        /* Git sync of the project the user works in */
        {
            project_name_t p;
            char dir[64];
            if (context_project(p, sizeof(p))) {
                project_dir(p, dir, sizeof(dir));
                snprintf(buf, sizeof(buf), "Synchronize: %s%s", p,
                         git_sync_is_configured(dir) ? "" : " (not configured)");
            } else {
                snprintf(buf, sizeof(buf), "Synchronize (no project selected)");
            }
            menu_add(MENU_ITEM_GIT_SYNC, buf);
        }
        /* Draftbox one-time password -> <project>/git.cfg */
        menu_add(MENU_ITEM_DRAFTBOX, "Connect a Draftbox repository...");
#if defined(CONFIG_DRAFTLING_HAS_KERMIT)
        /* Sync via browser: Kermit server on the serial console, serving
         * the project the user works in (see kermit_start()). */
        {
            project_name_t p;
            if (context_project(p, sizeof(p)))
                snprintf(buf, sizeof(buf), "Sync via browser: %s...", p);
            else
                snprintf(buf, sizeof(buf), "Sync via browser (no project selected)");
            menu_add(MENU_ITEM_KERMIT, buf);
        }
#endif
        menu_add(MENU_ITEM_BACK, "Back (Esc)");
        break;

    case MENU_LVL_CONN:
#if !defined(CONFIG_DRAFTLING_NETWORK_HOST_MANAGED)
        /* The host's network (the Flatpak): nothing to control. */
        menu_add(MENU_ITEM_WIFI_MENU, "WiFi...");
#endif
#if !defined(CONFIG_DRAFTLING_NO_BLUETOOTH)
        /* Without a BLE keyboard host (keyboards connected natively:
         * USB, the Linux program's own keyboard) there is nothing to
         * show or scan for. */
        menu_add(MENU_ITEM_BLE_MENU, "Bluetooth...");
#endif
        menu_add(MENU_ITEM_BACK, "Back (Esc)");
        break;

    case MENU_LVL_BLE:
        snprintf(buf, sizeof(buf), "Status: %s",
                 ble_keyboard_is_connected()
                     ? ble_keyboard_get_device_name()
                     : "not connected");
        menu_add(MENU_ITEM_BLE_STATUS, buf);
        menu_add(MENU_ITEM_BLE_SCAN, "Scan for keyboard");
        menu_add(MENU_ITEM_BACK, "Back (Esc)");
        break;

    case MENU_LVL_WIFI:
        if (wifi_manager_is_connected()) {
            snprintf(buf, sizeof(buf), "Status: %s (%s)",
                     wifi_manager_get_ssid(), wifi_manager_get_ip());
        } else {
            snprintf(buf, sizeof(buf), "Status: not connected");
        }
        menu_add(MENU_ITEM_WIFI_STATUS, buf);
        /* Connect to the configured (wifi.cfg / NVS) SSID, or disconnect */
        if (wifi_manager_is_connected()) {
            snprintf(buf, sizeof(buf), "Disconnect");
        } else {
            char cfg_ssid[33];
            if (wifi_manager_get_configured_ssid(cfg_ssid, sizeof(cfg_ssid)))
                snprintf(buf, sizeof(buf), "Connect to %s", cfg_ssid);
            else
                snprintf(buf, sizeof(buf), "Connect (not configured)");
        }
        menu_add(MENU_ITEM_WIFI_TOGGLE, buf);
        /* Scan and pick a new network */
        menu_add(MENU_ITEM_WIFI_NEW, "Choose network...");
        menu_add(MENU_ITEM_BACK, "Back (Esc)");
        break;

    case MENU_LVL_SYSTEM:
        menu_add(MENU_ITEM_HWINFO, "Hardware information");
#if defined(CONFIG_DRAFTLING_HAS_USB_MSC)
        /* SD card via USB. Shows the staged choice (s_usbmsc_pending_mode),
         * not necessarily what usb_msc is doing right now -- see the
         * picker opened from menu_activate_item() and applied by
         * commit_usb_msc_pending_mode(). See usbmsc_item_disabled() for
         * when it is disabled instead. */
        if (usbmsc_item_disabled()) {
#if defined(CONFIG_DRAFTLING_STORAGE_FLASH)
            snprintf(buf, sizeof(buf), "SD card via USB: (no SD card, internal storage)");
#else
            snprintf(buf, sizeof(buf), "SD card via USB: (no SD card)");
#endif
        } else {
            snprintf(buf, sizeof(buf), "SD card via USB: %s  (Enter to change)",
                     usb_msc_mode_name(s_usbmsc_pending_mode));
        }
        menu_add(MENU_ITEM_USB_MSC, buf);
#endif
        menu_add(MENU_ITEM_FORMAT_SD,
                 sd_card_is_mounted() ? "Format " STORAGE_NAME "..."
                                      : "Format " STORAGE_NAME "... (no " STORAGE_NAME ")");
        menu_add(MENU_ITEM_BACK, "Back (Esc)");
        break;
    }

    for (int i = 0; i < s_menu_item_count; i++)
        list_mark_hotkey(s_menu_list, i, menu_item_hotkey(s_menu_items[i]));

    if (s_menu_sel >= s_menu_item_count) s_menu_sel = s_menu_item_count - 1;
    if (s_menu_sel < 0) s_menu_sel = 0;

    /* Highlight selection */
    apply_list_selection_styles(s_menu_list, s_menu_sel);
    s_menu_sel_prev = s_menu_sel;

#if defined(CONFIG_DRAFTLING_TOUCHSCREEN)
    /* Attach tap-to-select / tap-again-to-activate handlers to every
     * row. The selection state is owned by the menu itself, so we
     * pass the addresses of s_menu_sel / s_menu_sel_prev. */
    s_menu_touch_ctx.p_sel      = &s_menu_sel;
    s_menu_touch_ctx.p_sel_prev = &s_menu_sel_prev;
    s_menu_touch_ctx.activate   = menu_activate_item;
    list_touch_attach(s_menu_list, &s_menu_touch_ctx);
#endif

    sync_battery_labels();
}

/* Move only the highlight bar without rebuilding the list. */
static void update_menu_highlight(void)
{
    update_list_highlight(s_menu_list, s_menu_sel, s_menu_sel_prev);
    s_menu_sel_prev = s_menu_sel;
}

/* Open a sub-level of the menu, remembering where we were. */
static void menu_enter_level(menu_level_t lvl)
{
    if (s_menu_depth < MENU_MAX_DEPTH) s_menu_sel_stack[s_menu_depth++] = s_menu_sel;
    s_menu_level = lvl;
    s_menu_sel = 0;
    refresh_menu_items();
}

static void close_menu(void);

/* Esc / "Back": up one level, or close the menu on the top level. */
static void menu_go_back(void)
{
    if (s_menu_level == MENU_LVL_ROOT) {
        close_menu();
        return;
    }
    s_menu_level = menu_parent_level(s_menu_level);
    s_menu_sel = (s_menu_depth > 0) ? s_menu_sel_stack[--s_menu_depth] : 0;
    if (s_menu_level == MENU_LVL_ROOT) s_menu_depth = 0;
    refresh_menu_items();
}

static void show_menu(void)
{
    s_menu_open = true;
    s_menu_level = MENU_LVL_ROOT;
    s_menu_depth = 0;
    s_menu_sel = 0;
    /* Defensive reset: every normal exit from the picker / password
     * prompt (Enter or Esc) already clears these, so this is only a
     * safety net against a stale flag surviving some other path back
     * to the top-level menu. */
    s_wifi_scan_picker_open = false;
    s_wifi_pw_open = false;
    if (s_wifi_pw_panel) lv_obj_add_flag(s_wifi_pw_panel, LV_OBJ_FLAG_HIDDEN);
    s_dbx_open = false;
    if (s_dbx_panel) lv_obj_add_flag(s_dbx_panel, LV_OBJ_FLAG_HIDDEN);
    s_fmt_step = FMT_STEP_NONE;
    s_xfer_open = false;
    s_xfer_migrate = false;
    s_books_open = false;
    s_toc_open = false;
    books_free();
#if defined(CONFIG_DRAFTLING_HAS_KERMIT)
    s_kermit_picker_open = false;
#endif
    if (s_lbl_menu_hdr) lv_label_set_text(s_lbl_menu_hdr, MENU_HDR_TEXT);
#if defined(CONFIG_DRAFTLING_HAS_USB_MSC)
    /* Sync the staged choice to whatever usb_msc is actually doing
     * right now -- picks up an auto-off that happened while the menu
     * was closed. (Leaving the picker itself, Enter or Esc, already
     * commits any staged choice -- see close_usbmsc_picker() -- so
     * this should normally be a no-op.) */
    s_usbmsc_picker_open = false;
    s_usbmsc_pending_mode = usb_msc_get_mode();
#endif
    refresh_menu_items();
    sync_battery_labels();
#if defined(CONFIG_DRAFTLING_DISPLAY_EPD)
    /* Same missing-repaint bug already fixed for the editor, file
     * browser, and BLE-prompt-restore screen transitions: a bare
     * lv_scr_load() does not reliably force this screen's widgets to
     * redraw from a stale framebuffer on this backend.
     * epd_full_screen_repaint() doesn't apply here -- it only
     * invalidates s_scr (the editor screen), not s_scr_menu. */
    display_clear(0xFF);
    lv_obj_invalidate(s_scr_menu);
#endif
    lv_scr_load(s_scr_menu);
}

#if defined(CONFIG_DRAFTLING_HAS_USB_MSC)
/* Called by close_usbmsc_picker() right after leaving the "SD card via
 * USB" picker (Enter or Esc): applies s_usbmsc_pending_mode if it
 * differs from what usb_msc is actually doing right now. Esc leaves
 * s_usbmsc_pending_mode unchanged, so this is a no-op in that case.
 *
 * Turning it off restarts the device (see usb_msc.h) -- this function
 * does not return in that case. */
static void commit_usb_msc_pending_mode(void)
{
    usb_msc_mode_t cur = usb_msc_get_mode();
    if (s_usbmsc_pending_mode == cur) return;

    if (s_usbmsc_pending_mode == USB_MSC_MODE_OFF) {
        usb_msc_set_mode(USB_MSC_MODE_OFF);
        /* usb_msc_set_mode(OFF) alone does not give the ESP32-S3's
         * native USB pins back to USB-Serial-JTAG (flashing, and
         * usually the console too, on these single-USB-port boards)
         * -- see usb_msc.h. Restart now rather than leave the board
         * unflashable until a physical reset; a status message here
         * would never actually be seen, since the restart cuts it
         * off before the next LVGL flush. */
        ESP_LOGI(TAG, "Restarting to restore USB-Serial-JTAG after SD-via-USB");
        esp_restart();
        /* does not return */
    }

#if defined(CONFIG_DRAFTLING_HAS_KERMIT)
    if (cur == USB_MSC_MODE_OFF && kermit_server_is_running()) {
        s_usbmsc_pending_mode = cur;
        editor_ui_set_status("Sync via browser active");
        return;
    }
#endif

    if (cur == USB_MSC_MODE_OFF) {
        /* Taking the card away from local editing for the first time:
         * auto-save whatever is open and collapse any split so no
         * pane is left trying to save into the now-USB-owned card,
         * then force the file browser -- close_menu() routes there
         * whenever s_editor_screen_active is false, once the user
         * eventually leaves the F1 menu too. */
        if (editor_is_modified() && editor_get_file_path()) {
            editor_save_file();
        }
        editor_ui_apply_split_mode(SPLIT_NONE);
        s_editor_screen_active = false;
    }

    esp_err_t err = usb_msc_set_mode(s_usbmsc_pending_mode);
    if (err != ESP_OK) {
        s_usbmsc_pending_mode = usb_msc_get_mode();
        editor_ui_set_status("SD card via USB: failed to start (no SD card?)");
        return;
    }

    editor_ui_set_status(s_usbmsc_pending_mode == USB_MSC_MODE_READ_WRITE
                          ? "SD card via USB: Read-write"
                          : "SD card via USB: Read-only");
}
#endif

static void close_menu(void)
{
    s_menu_open = false;
    if (s_editor_screen_active)
        editor_ui_show_editor();
    else
        editor_ui_show_file_browser();
}

/* ---- Settings screen ---- */

static int find_timeout_option(uint32_t sec)
{
    for (int i = 0; i < TIMEOUT_OPTION_COUNT; i++) {
        if (TIMEOUT_OPTIONS[i] == sec) return i;
    }
    return 2; /* default to 10 min */
}

/* ---- Settings menu items ----
 * Settings is a tree of levels like the F1 menu (see menu_level_t).
 * refresh_settings_items() builds the rows of the current level into
 * s_settings_items[], leaving out the rows a board does not have
 * (backlight, color theme, rotate / invert), so always go through
 * s_settings_items[] rather than a fixed row index. */
enum settings_level_t {
    SET_LVL_ROOT = 0,
    SET_LVL_EDITOR,
    SET_LVL_DISPLAY,
    SET_LVL_MARGINS,   /* under Display */
    SET_LVL_KEYBOARD,
    SET_LVL_SYSTEM,
};

enum settings_item_t {
    SET_ITEM_EDITOR_MENU = 0,
    SET_ITEM_DISPLAY_MENU,
    SET_ITEM_KEYBOARD_MENU,
    SET_ITEM_SYSTEM_MENU,
    /* Editor */
    SET_ITEM_FONTSZ,
    SET_ITEM_APPEND_ONLY,
    SET_ITEM_NAME_SUGGEST,
    SET_ITEM_HIDE_BARS,
    SET_ITEM_BARS_IDLE,      /* only while SET_ITEM_HIDE_BARS is on */
    /* Display */
    SET_ITEM_BACKLIGHT,      /* HAS_BACKLIGHT && !BACKLIGHT_BINARY */
    SET_ITEM_THEME,          /* DISPLAY_COLOR */
    SET_ITEM_ORIENTATION,
    SET_ITEM_ROTATE,         /* DISPLAY_CAN_ROTATE: live 180-degree flip */
    SET_ITEM_UPSIDE_DOWN,    /* otherwise: the same turn, after a restart */
    SET_ITEM_INVERT,         /* DISPLAY_CAN_INVERT */
    SET_ITEM_SCALE2X,
    SET_ITEM_CORNERS,        /* DISPLAY_ROUND_CORNERS */
    SET_ITEM_HOST_ORIENT,    /* DISPLAY_HOST_ROTATE: the host turns the screen */
    SET_ITEM_HOST_ROTATE,
    SET_ITEM_MARGINS_MENU,
    /* Display -> Screen margins */
    SET_ITEM_MARGIN_LEFT,
    SET_ITEM_MARGIN_RIGHT,
    SET_ITEM_MARGIN_TOP,
    SET_ITEM_MARGIN_BOTTOM,
    /* Keyboard */
    SET_ITEM_LAYOUTS,
    /* System */
    SET_ITEM_TIMEOUT,
    SET_ITEM_RESET,
    /* Last row of every level */
    SET_ITEM_BACK,
};

#define SETTINGS_MAX_ITEMS 12
#define SETTINGS_MAX_DEPTH 4

static settings_level_t s_settings_level = SET_LVL_ROOT;
static settings_item_t  s_settings_items[SETTINGS_MAX_ITEMS];
static int              s_settings_item_count = 0;
static int              s_settings_sel_stack[SETTINGS_MAX_DEPTH];
static int              s_settings_depth = 0;

static const char *settings_level_header(settings_level_t lvl)
{
    switch (lvl) {
    case SET_LVL_EDITOR:   return "Settings > Editor - Arrows to change, Esc: back";
    case SET_LVL_DISPLAY:  return "Settings > Display - Arrows to change, Esc: back";
    case SET_LVL_MARGINS:  return "Settings > Display > Screen margins - Esc: back";
    case SET_LVL_KEYBOARD: return "Settings > Keyboard - Esc: back";
    case SET_LVL_SYSTEM:   return "Settings > System - Arrows to change, Esc: back";
    default:               return "Settings - Up/Down, Enter to select, Esc to go back";
    }
}

static settings_level_t settings_parent_level(settings_level_t lvl)
{
    return (lvl == SET_LVL_MARGINS) ? SET_LVL_DISPLAY : SET_LVL_ROOT;
}

static void settings_add(settings_item_t id, const char *label)
{
    if (s_settings_item_count >= SETTINGS_MAX_ITEMS) return;
    s_settings_items[s_settings_item_count++] = id;
    lv_list_add_btn(s_settings_list, NULL, label);
}

/* Item id of a settings row, or SET_ITEM_BACK when out of range. */
static settings_item_t settings_item_at(int row)
{
    if (row < 0 || row >= s_settings_item_count) return SET_ITEM_BACK;
    return s_settings_items[row];
}

static void settings_activate_item(int row);

static void refresh_settings_items(void)
{
    lv_obj_clean(s_settings_list);
    s_settings_item_count = 0;
    if (s_lbl_settings_hdr)
        lv_label_set_text(s_lbl_settings_hdr, settings_level_header(s_settings_level));

    char buf[80];

    switch (s_settings_level) {
    case SET_LVL_ROOT:
        settings_add(SET_ITEM_EDITOR_MENU, "Editor...");
        settings_add(SET_ITEM_DISPLAY_MENU, "Display...");
        settings_add(SET_ITEM_KEYBOARD_MENU, "Keyboard...");
        settings_add(SET_ITEM_SYSTEM_MENU, "System...");
        break;

    case SET_LVL_EDITOR:
        /* Base font size */
        {
            int fi = find_font_size_option(s_font_size);
            snprintf(buf, sizeof(buf), "Base font size: %s",
                     FONT_SIZE_LABELS[fi]);
            settings_add(SET_ITEM_FONTSZ, buf);
        }
        /* Append-only editing */
        snprintf(buf, sizeof(buf), "Append-only editing: %s",
                 s_append_only ? "on" : "off");
        settings_add(SET_ITEM_APPEND_ONLY, buf);
        /* Drop-down of known names while typing (Fountain characters) */
        snprintf(buf, sizeof(buf), "Suggest character names: %s",
                 s_name_suggest ? "on" : "off");
        settings_add(SET_ITEM_NAME_SUGGEST, buf);
        /* Hide the title / status bars while typing */
        if (s_hide_bars && status_ime_layout() != KB_LAYOUT_COUNT) {
            /* On, but kept off by a layout that types in the status bar. */
            snprintf(buf, sizeof(buf), "Hide status bars while typing: on (not with %s)",
                     kb_layout_name(status_ime_layout()));
        } else {
            snprintf(buf, sizeof(buf), "Hide status bars while typing: %s",
                     s_hide_bars ? "on" : "off");
        }
        settings_add(SET_ITEM_HIDE_BARS, buf);
        if (s_hide_bars) {
            snprintf(buf, sizeof(buf), "Show status bars after idle: %us",
                     (unsigned)s_bars_idle_s);
            settings_add(SET_ITEM_BARS_IDLE, buf);
        }
        break;

    case SET_LVL_DISPLAY:
#if defined(CONFIG_DRAFTLING_DISPLAY_HAS_BACKLIGHT) && !defined(CONFIG_DRAFTLING_DISPLAY_BACKLIGHT_BINARY)
        /* Backlight brightness (LCD boards with a controllable backlight) */
        snprintf(buf, sizeof(buf), BACKLIGHT_LABEL ": %d%%", s_backlight_pct);
        settings_add(SET_ITEM_BACKLIGHT, buf);
#endif
#if defined(CONFIG_DRAFTLING_DISPLAY_COLOR)
        /* Color theme (color LCDs only) */
        snprintf(buf, sizeof(buf), "Color theme: %s",
                 COLOR_THEMES[s_theme_idx].name);
        settings_add(SET_ITEM_THEME, buf);
#endif
#if !defined(CONFIG_DRAFTLING_DISPLAY_RESIZABLE)
        /* Display orientation -- landscape (default) or portrait (the
         * whole UI turned another 90 degrees). Frozen for the session
         * like the margins, so it only takes effect on a restart. A
         * resizable window has neither (nor margins, below): its shape
         * is the window's. */
        snprintf(buf, sizeof(buf), "Orientation: %s (restart to apply)",
                 s_pending_portrait ? "Portrait" : "Landscape");
        settings_add(SET_ITEM_ORIENTATION, buf);
#endif
#if defined(CONFIG_DRAFTLING_DISPLAY_HOST_ROTATE)
        /* The host turns the window's contents: applied at once. */
        snprintf(buf, sizeof(buf), "Orientation: %s", host_orient_name(s_host_orient));
        settings_add(SET_ITEM_HOST_ORIENT, buf);
        snprintf(buf, sizeof(buf), "Rotate 180: %s", s_host_rot180 ? "on" : "off");
        settings_add(SET_ITEM_HOST_ROTATE, buf);
#elif defined(CONFIG_DRAFTLING_DISPLAY_RESIZABLE)
        /* (no Rotate 180) */
#elif defined(CONFIG_DRAFTLING_DISPLAY_CAN_ROTATE)
        /* Runtime 180-degree display flip, applied at once */
        snprintf(buf, sizeof(buf), "Rotate 180: %s",
                 s_rotate180 ? "on" : "off");
        settings_add(SET_ITEM_ROTATE, buf);
#else
        /* The same 180-degree turn on boards that cannot flip live: it
         * is added to the LVGL rotation, which is frozen for the
         * session (display_flip.h), so it applies on a restart. */
        snprintf(buf, sizeof(buf), "Rotate 180: %s (restart to apply)",
                 s_pending_upside_down ? "on" : "off");
        settings_add(SET_ITEM_UPSIDE_DOWN, buf);
#endif
#if defined(CONFIG_DRAFTLING_DISPLAY_CAN_INVERT)
        snprintf(buf, sizeof(buf), "Invert display: %s",
                 s_invert ? "on" : "off");
        settings_add(SET_ITEM_INVERT, buf);
#endif
        /* 2x scale for the visually impaired: every pixel drawn as a
         * 2x2 block. The LVGL canvas size is fixed for the session, so
         * it only takes effect on a restart. */
        snprintf(buf, sizeof(buf), "2x scale (large view): %s (restart to apply)",
                 s_pending_scale2x ? "on" : "off");
        settings_add(SET_ITEM_SCALE2X, buf);
#if defined(CONFIG_DRAFTLING_DISPLAY_ROUND_CORNERS)
        /* Rounded screen corners (full screen on a phone): applied at
         * once, the bars cover the corners. */
        if (s_corner_size)
            snprintf(buf, sizeof(buf), "Rounded screen corners: %u (full screen)",
                     (unsigned)s_corner_size);
        else
            snprintf(buf, sizeof(buf), "Rounded screen corners: off");
        settings_add(SET_ITEM_CORNERS, buf);
#endif
#if !defined(CONFIG_DRAFTLING_DISPLAY_RESIZABLE)
        settings_add(SET_ITEM_MARGINS_MENU, "Screen margins...");
#endif
        break;

    case SET_LVL_MARGINS:
        /* Pixels of panel hidden under the enclosure on each edge. Zero
         * by default; the LVGL display resolution is fixed for the
         * session, so a change is not visible until a restart --
         * settings_go_back() offers one when the user leaves the
         * Display level with a pending change. */
        snprintf(buf, sizeof(buf), "Left: %dpx (restart to apply)", s_margin_left);
        settings_add(SET_ITEM_MARGIN_LEFT, buf);
        snprintf(buf, sizeof(buf), "Right: %dpx (restart to apply)", s_margin_right);
        settings_add(SET_ITEM_MARGIN_RIGHT, buf);
        snprintf(buf, sizeof(buf), "Top: %dpx (restart to apply)", s_margin_top);
        settings_add(SET_ITEM_MARGIN_TOP, buf);
        snprintf(buf, sizeof(buf), "Bottom: %dpx (restart to apply)", s_margin_bottom);
        settings_add(SET_ITEM_MARGIN_BOTTOM, buf);
        break;

    case SET_LVL_KEYBOARD:
        /* Active language layouts -- comma-separated summary of the
         * layouts Ctrl+L / Win+Space currently rotate through. */
        {
            char names[200] = "";
            for (int i = 0; i < KB_LAYOUT_COUNT; i++) {
                kb_layout_id_t id = (kb_layout_id_t)i;
                if (!kb_layout_is_active(id)) continue;
                if (names[0]) strncat(names, ", ", sizeof(names) - strlen(names) - 1);
                strncat(names, kb_layout_name(id), sizeof(names) - strlen(names) - 1);
            }
            /* A long list would not fit the row: give the count instead. */
            if (strlen(names) > 40) {
                snprintf(names, sizeof(names), "%d layouts", kb_layout_active_count());
            }
            snprintf(buf, sizeof(buf), "Active layouts: %s (Enter to edit)", names);
            settings_add(SET_ITEM_LAYOUTS, buf);
        }
        break;

    case SET_LVL_SYSTEM:
        /* Standby timeout */
        {
            uint32_t cur = standby_get_timeout();
            const char *cur_label = "custom";
            int idx = find_timeout_option(cur);
            if (idx >= 0 && idx < TIMEOUT_OPTION_COUNT)
                cur_label = TIMEOUT_LABELS[idx];
            snprintf(buf, sizeof(buf), "Standby timeout: %s", cur_label);
            settings_add(SET_ITEM_TIMEOUT, buf);
        }
        /* Factory reset */
        settings_add(SET_ITEM_RESET,
                     s_factory_reset_confirm
                         ? "Factory reset -- ENTER again to confirm"
                         : "Factory reset");
        break;
    }

    settings_add(SET_ITEM_BACK, "Back (Esc)");

    if (s_settings_sel >= s_settings_item_count) s_settings_sel = s_settings_item_count - 1;
    if (s_settings_sel < 0) s_settings_sel = 0;

    /* Highlight selection */
    apply_list_selection_styles(s_settings_list, s_settings_sel);
    s_settings_sel_prev = s_settings_sel;

#if defined(CONFIG_DRAFTLING_TOUCHSCREEN)
    s_settings_touch_ctx.p_sel      = &s_settings_sel;
    s_settings_touch_ctx.p_sel_prev = &s_settings_sel_prev;
    s_settings_touch_ctx.activate   = settings_activate_item;
    list_touch_attach(s_settings_list, &s_settings_touch_ctx);
#endif

    sync_battery_labels();
}

/* Move only the highlight bar without rebuilding the list. */
static void update_settings_highlight(void)
{
    update_list_highlight(s_settings_list, s_settings_sel, s_settings_sel_prev);
    s_settings_sel_prev = s_settings_sel;
}

#if defined(CONFIG_DRAFTLING_DISPLAY_COLOR)
/* ---- Color-theme picker ----
 * Renders the available themes into the same s_settings_list widget
 * while s_theme_picker_open is true, so we do not need a second
 * screen. The current selection is highlighted on entry. */
static void refresh_theme_picker_items(void)
{
    lv_obj_clean(s_settings_list);
    char buf[80];
    for (int i = 0; i < COLOR_THEME_COUNT; i++) {
        snprintf(buf, sizeof(buf), "%s%s",
                 (i == s_theme_idx) ? "* " : "  ",
                 COLOR_THEMES[i].name);
        lv_list_add_btn(s_settings_list, NULL, buf);
    }
    lv_list_add_btn(s_settings_list, NULL, "  Cancel (Esc)");
    apply_list_selection_styles(s_settings_list, s_theme_picker_sel);
    s_theme_picker_sel_prev = s_theme_picker_sel;

    sync_battery_labels();
}

static void update_theme_picker_highlight(void)
{
    update_list_highlight(s_settings_list, s_theme_picker_sel,
                          s_theme_picker_sel_prev);
    s_theme_picker_sel_prev = s_theme_picker_sel;
}

/* Cancel the picker and re-render the regular settings list. */
static void close_theme_picker(void)
{
    s_theme_picker_open = false;
    refresh_settings_items();
}
#endif

/* ---- Active-language-layouts picker ----
 * Renders one checklist row per compiled-in layout into the same
 * s_settings_list widget while s_layouts_picker_open is true, plus a
 * trailing "Done" row. Unlike the theme picker, Enter toggles the
 * highlighted row's membership in the active set immediately (applied
 * and persisted on the spot) rather than picking one value and
 * confirming -- there is no "cancel", since each toggle already is
 * the final state. */
/* Row order: US-English first, then the others by their long name, so
 * the layouts of one language (French AZERTY, AFNOR, BEPO) are next to
 * each other. The layout ids, and so the saved active-layout mask, keep
 * their order. */
static kb_layout_id_t s_layouts_order[KB_LAYOUT_COUNT];

static int layouts_order_cmp(const void *a, const void *b)
{
    kb_layout_id_t x = *(const kb_layout_id_t *)a;
    kb_layout_id_t y = *(const kb_layout_id_t *)b;
    if (x == KB_LAYOUT_US) return y == KB_LAYOUT_US ? 0 : -1;
    if (y == KB_LAYOUT_US) return 1;
    int c = strcasecmp(kb_layout_long_name(x), kb_layout_long_name(y));
    return c ? c : (int)x - (int)y;
}

static void layouts_picker_sort(void)
{
    for (int i = 0; i < KB_LAYOUT_COUNT; i++)
        s_layouts_order[i] = (kb_layout_id_t)i;
    qsort(s_layouts_order, KB_LAYOUT_COUNT, sizeof(s_layouts_order[0]),
          layouts_order_cmp);
}

static void layouts_picker_row_text(kb_layout_id_t id, char *buf,
                                    size_t sz)
{
    snprintf(buf, sz, "[%s] %-6s %s",
             kb_layout_is_active(id) ? "x" : " ",
             kb_layout_name(id), kb_layout_long_name(id));
}

static void refresh_layouts_picker_items(void)
{
    lv_obj_clean(s_settings_list);
    layouts_picker_sort();
    char buf[80];
    for (int i = 0; i < KB_LAYOUT_COUNT; i++) {
        layouts_picker_row_text(s_layouts_order[i], buf, sizeof(buf));
        lv_list_add_btn(s_settings_list, NULL, buf);
    }
    lv_list_add_btn(s_settings_list, NULL, "  Done (Esc)");
    apply_list_selection_styles(s_settings_list, s_layouts_picker_sel);
    s_layouts_picker_sel_prev = s_layouts_picker_sel;

    sync_battery_labels();
}

static void update_layouts_picker_highlight(void)
{
    update_list_highlight(s_settings_list, s_layouts_picker_sel,
                          s_layouts_picker_sel_prev);
    s_layouts_picker_sel_prev = s_layouts_picker_sel;
}

/* Re-label one row after a toggle. Rebuilding the whole list would
 * reset its scroll offset, and scrolling the highlighted row back into
 * view then moved the list under the user's eyes. */
static void update_layouts_picker_row(int row)
{
    kb_layout_id_t id = s_layouts_order[row];
    lv_obj_t *btn = lv_obj_get_child(s_settings_list, (int32_t)row);
    lv_obj_t *lbl = btn ? lv_obj_get_child_by_type(btn, 0, &lv_label_class)
                        : NULL;
    if (!lbl) {
        refresh_layouts_picker_items();
        return;
    }
    char buf[80];
    layouts_picker_row_text(id, buf, sizeof(buf));
    lv_label_set_text(lbl, buf);
}

static void close_layouts_picker(void)
{
    s_layouts_picker_open = false;
    refresh_settings_items();
}

/* ---- Restart-to-apply prompt ----
 * Renders into the same s_settings_list widget while
 * s_restart_confirm_open is true -- same reused-list pattern as the
 * color-theme picker above. Shown by settings_go_back() when the user
 * leaves the Settings -> Display level having changed a restart-only
 * display setting there (orientation, rotate 180 on boards that cannot
 * flip live, 2x scale or a screen margin). */
static void refresh_restart_confirm_items(void)
{
    lv_obj_clean(s_settings_list);
    if (s_lbl_settings_hdr)
        lv_label_set_text(s_lbl_settings_hdr,
                          "Settings > Display - restart to apply the changes?");
    lv_list_add_btn(s_settings_list, NULL, "Restart now to apply changes");
    lv_list_add_btn(s_settings_list, NULL, "Keep editing (apply later)");
    apply_list_selection_styles(s_settings_list, s_restart_confirm_sel);
    s_restart_confirm_sel_prev = s_restart_confirm_sel;
}

static void update_restart_confirm_highlight(void)
{
    update_list_highlight(s_settings_list, s_restart_confirm_sel,
                          s_restart_confirm_sel_prev);
    s_restart_confirm_sel_prev = s_restart_confirm_sel;
}

static void close_settings(void);

/* Open a sub-level of Settings, remembering where we were. */
static void settings_enter_level(settings_level_t lvl)
{
    if (s_settings_depth < SETTINGS_MAX_DEPTH)
        s_settings_sel_stack[s_settings_depth++] = s_settings_sel;
    s_settings_level = lvl;
    s_settings_sel = 0;
    s_factory_reset_confirm = false;
    refresh_settings_items();
}

/* Up one level without asking anything. */
static void settings_pop_level(void)
{
    s_settings_level = settings_parent_level(s_settings_level);
    s_settings_sel = (s_settings_depth > 0) ? s_settings_sel_stack[--s_settings_depth] : 0;
    if (s_settings_level == SET_LVL_ROOT) s_settings_depth = 0;
    s_factory_reset_confirm = false;
    refresh_settings_items();
}

/* Esc / "Back": up one level, or back to the F1 menu from the top
 * level. Leaving the Display level after a restart-only setting was
 * changed there first offers to restart -- the LVGL display resolution
 * and rotation are fixed for the session (see display_margins.h /
 * display_orientation.h / display_flip.h / display_scale.h), so a
 * restart is the only way to apply the change without waiting for the
 * next natural reboot / wake. */
static void settings_go_back(void)
{
    if (s_settings_level == SET_LVL_DISPLAY && s_restart_needed) {
        s_restart_confirm_open = true;
        s_restart_confirm_sel = 0;
        refresh_restart_confirm_items();
        return;
    }
    if (s_settings_level == SET_LVL_ROOT) {
        close_settings();
        return;
    }
    settings_pop_level();
}

static void show_settings(void)
{
    s_settings_open = true;
    s_menu_open = false;
    s_settings_level = SET_LVL_ROOT;
    s_settings_depth = 0;
    s_settings_sel = 0;
    s_factory_reset_confirm = false;
    s_restart_needed = false;
    s_restart_confirm_open = false;
#if defined(CONFIG_DRAFTLING_DISPLAY_COLOR)
    s_theme_picker_open = false;
#endif
    s_layouts_picker_open = false;
    refresh_settings_items();
    sync_battery_labels();
#if defined(CONFIG_DRAFTLING_DISPLAY_EPD)
    /* Same missing-repaint bug as show_menu() -- see its comment. */
    display_clear(0xFF);
    lv_obj_invalidate(s_scr_settings);
#endif
    lv_scr_load(s_scr_settings);
}

static void close_settings(void)
{
    s_settings_open = false;
#if defined(CONFIG_DRAFTLING_DISPLAY_COLOR)
    s_theme_picker_open = false;
#endif
    s_layouts_picker_open = false;
    show_menu();
}

/* Cycles a settings row's discrete value forward (dir=+1) or backward
 * (dir=-1). Enter has always meant "next value"; the Left/Right arrow
 * keys reuse this exact logic (dir=-1 for Left, dir=+1 for Right) so
 * a row can be adjusted either direction without repeatedly wrapping
 * all the way around via Enter. Returns false for rows that are not a
 * simple in-place value cycle (Theme and Active layouts open a
 * sub-picker instead; sub-level rows, Factory reset and Back are
 * actions, not values), so callers can no-op the arrow keys on those rows. */
static bool settings_cycle_item(int row, int dir)
{
    const settings_item_t idx = settings_item_at(row);
    if (idx == SET_ITEM_TIMEOUT) {
        uint32_t cur = standby_get_timeout();
        int opt = find_timeout_option(cur);
        opt = (opt + dir + TIMEOUT_OPTION_COUNT) % TIMEOUT_OPTION_COUNT;
        standby_set_timeout(TIMEOUT_OPTIONS[opt]);
        refresh_settings_items();
        return true;
    } else if (idx == SET_ITEM_FONTSZ) {
        int fi = find_font_size_option(s_font_size);
        fi = (fi + dir + FONT_SIZE_COUNT) % FONT_SIZE_COUNT;
        s_font_size = FONT_SIZE_OPTIONS[fi];
        save_font_size_to_nvs();
        init_styles();
        refresh_settings_items();
        /* The new font changes widget geometry on every screen and in
         * every pane. Pixels left over from the previous (differently
         * sized) layout would otherwise survive in the persistent
         * display buffer -- and stale line geometry lingers in each
         * pane's render cache -- and reappear as garbage the next time
         * the editor screen is drawn (for example after re-opening a
         * file). Wipe the framebuffer, drop every pane's cached line
         * geometry so each label is fully re-laid-out, and invalidate
         * the active screen so LVGL repaints everything. This runs on
         * all backends because both the e-paper and the persistent
         * framebuffer LCDs keep stale pixels in regions the new layout
         * no longer covers. display_clear() also promotes the next
         * flush to a flashing full refresh on e-paper, clearing any
         * accumulated ghosting. */
#if defined(CONFIG_DRAFTLING_EPD_BLACK_BACKGROUND)
        display_clear(0x00);
#else
        display_clear(0xFF);
#endif
        /* Also wipe LVGL's own draw buffers. On the reflective-LCD
         * FULL render-mode path LVGL keeps two screen-sized buffers
         * and can flush a stale one (holding the pre-change layout)
         * when the next screen is only partially invalidated -- for
         * example after re-opening a file -- which surfaces as garbage
         * in the untouched regions. display_clear() only wipes the
         * panel framebuffer, not these buffers. */
        draftling_lvgl_port_clear_buffers();
        invalidate_all_render_caches();
        lv_obj_invalidate(lv_scr_act());
        return true;
#if defined(CONFIG_DRAFTLING_DISPLAY_HAS_BACKLIGHT) && !defined(CONFIG_DRAFTLING_DISPLAY_BACKLIGHT_BINARY)
    } else if (idx == SET_ITEM_BACKLIGHT) {
        int bi = find_backlight_option(s_backlight_pct);
        bi = (bi + dir + BACKLIGHT_OPTION_COUNT) % BACKLIGHT_OPTION_COUNT;
        s_backlight_pct = BACKLIGHT_OPTIONS[bi];
        save_backlight_to_nvs();
        display_set_backlight(s_backlight_pct);
        refresh_settings_items();
        return true;
#endif
#if defined(CONFIG_DRAFTLING_DISPLAY_CAN_ROTATE)
    } else if (idx == SET_ITEM_ROTATE) {
        /* Only two states, so either direction just toggles it.
         * Applied immediately so the user sees the new orientation
         * without leaving the menu; persisted in NVS and re-applied
         * at boot. A 180 flip does not change the reported
         * resolution, so the widget tree stays valid -- the LVGL port
         * just repaints the whole panel in the new orientation. */
        s_rotate180 = !s_rotate180;
        save_rotate180_to_nvs();
        draftling_lvgl_port_set_flip180(s_rotate180);
        refresh_settings_items();
        return true;
#endif
#if defined(CONFIG_DRAFTLING_DISPLAY_CAN_INVERT)
    } else if (idx == SET_ITEM_INVERT) {
        /* Only two states, so either direction just toggles it.
         * Applied immediately so the user sees the new polarity
         * without leaving the menu; persisted in NVS and re-applied
         * at boot. display_set_invert() only changes what
         * display_flush() sends to the panel, so no repaint of the
         * LVGL widget tree is needed. */
        s_invert = !s_invert;
        save_invert_to_nvs();
        display_set_invert(s_invert);
        refresh_settings_items();
        return true;
#endif
    } else if (idx == SET_ITEM_APPEND_ONLY) {
        /* Only two states, so either direction just toggles it. No
         * hardware side effects, so just flip the flag, persist it,
         * and refresh the label. */
        s_append_only = !s_append_only;
        save_append_only_to_nvs();
        refresh_settings_items();
        return true;
    } else if (idx == SET_ITEM_NAME_SUGGEST) {
        /* Only two states, so either direction toggles it. */
        s_name_suggest = !s_name_suggest;
        save_name_suggest_to_nvs();
        refresh_settings_items();
        return true;
    } else if (idx == SET_ITEM_HIDE_BARS) {
        /* Only two states, so either direction toggles it. Switching
         * it off brings hidden bars back at once. */
        s_hide_bars = !s_hide_bars;
        save_hide_bars_to_nvs();
        if (!s_hide_bars) bars_show();
        refresh_settings_items();
        return true;
    } else if (idx == SET_ITEM_BARS_IDLE) {
        int oi = find_bars_idle_option(s_bars_idle_s);
        oi = (oi + dir + BARS_IDLE_OPTION_COUNT) % BARS_IDLE_OPTION_COUNT;
        s_bars_idle_s = BARS_IDLE_OPTIONS[oi];
        save_hide_bars_to_nvs();
        refresh_settings_items();
        return true;
    } else if (idx == SET_ITEM_ORIENTATION) {
        /* Landscape <-> portrait. Only two states, so either direction
         * toggles. Persisted to NVS for the next boot; the LVGL
         * rotation and the whole widget tree are fixed for the session
         * (see display_orientation.h), so settings_go_back()
         * offers a restart on the way out. */
        s_pending_portrait = !s_pending_portrait;
        display_orientation_set_portrait(s_pending_portrait);
        s_restart_needed = true;
        refresh_settings_items();
        return true;
#if !defined(CONFIG_DRAFTLING_DISPLAY_CAN_ROTATE)
    } else if (idx == SET_ITEM_UPSIDE_DOWN) {
        /* Only two states, so either direction toggles. Persisted to
         * NVS for the next boot; like the orientation above, the LVGL
         * rotation and the whole widget tree are fixed for the
         * session (see display_flip.h), so settings_go_back()
         * offers a restart on the way out. */
        s_pending_upside_down = !s_pending_upside_down;
        display_flip_set_upside_down(s_pending_upside_down);
        s_restart_needed = true;
        refresh_settings_items();
        return true;
#endif
#if defined(CONFIG_DRAFTLING_DISPLAY_HOST_ROTATE)
    } else if (idx == SET_ITEM_HOST_ORIENT || idx == SET_ITEM_HOST_ROTATE) {
        /* The host turns its screen and reports the new size; the
         * screens are laid out again then (Settings stays open). */
        if (idx == SET_ITEM_HOST_ORIENT)
            s_host_orient = (uint8_t)((s_host_orient + dir + 3) % 3);
        else
            s_host_rot180 = !s_host_rot180;
        save_host_rotation_to_nvs();
        editor_host_rotation_changed(s_host_orient, s_host_rot180);
        refresh_settings_items();
        return true;
#endif
#if defined(CONFIG_DRAFTLING_DISPLAY_ROUND_CORNERS)
    } else if (idx == SET_ITEM_CORNERS) {
        /* Off, then growing radii; applied at once (the screens are
         * rebuilt, Settings stays where it is). */
        int oi = find_corner_option(s_corner_size);
        oi = (oi + dir + CORNER_OPTION_COUNT) % CORNER_OPTION_COUNT;
        s_corner_size = CORNER_OPTIONS[oi];
        save_corners_to_nvs();
        editor_host_corners_changed(s_corner_size);
        refresh_settings_items();
        corners_apply();
        return true;
#endif
    } else if (idx == SET_ITEM_SCALE2X) {
        /* Only two states, so either direction toggles. The LVGL
         * canvas size is fixed for the session (see display_scale.h),
         * so settings_go_back() offers a restart on the way out. */
        s_pending_scale2x = !s_pending_scale2x;
        display_scale_set_2x(s_pending_scale2x);
        s_restart_needed = true;
        refresh_settings_items();
        return true;
    } else if (idx == SET_ITEM_MARGIN_LEFT) {
        int oi = find_margin_option(s_margin_left);
        oi = (oi + dir + MARGIN_OPTION_COUNT) % MARGIN_OPTION_COUNT;
        s_margin_left = MARGIN_OPTIONS[oi];
        display_margins_set(s_margin_left, s_margin_right, s_margin_top, s_margin_bottom);
        s_restart_needed = true;
        refresh_settings_items();
        return true;
    } else if (idx == SET_ITEM_MARGIN_RIGHT) {
        int oi = find_margin_option(s_margin_right);
        oi = (oi + dir + MARGIN_OPTION_COUNT) % MARGIN_OPTION_COUNT;
        s_margin_right = MARGIN_OPTIONS[oi];
        display_margins_set(s_margin_left, s_margin_right, s_margin_top, s_margin_bottom);
        s_restart_needed = true;
        refresh_settings_items();
        return true;
    } else if (idx == SET_ITEM_MARGIN_TOP) {
        int oi = find_margin_option(s_margin_top);
        oi = (oi + dir + MARGIN_OPTION_COUNT) % MARGIN_OPTION_COUNT;
        s_margin_top = MARGIN_OPTIONS[oi];
        display_margins_set(s_margin_left, s_margin_right, s_margin_top, s_margin_bottom);
        s_restart_needed = true;
        refresh_settings_items();
        return true;
    } else if (idx == SET_ITEM_MARGIN_BOTTOM) {
        int oi = find_margin_option(s_margin_bottom);
        oi = (oi + dir + MARGIN_OPTION_COUNT) % MARGIN_OPTION_COUNT;
        s_margin_bottom = MARGIN_OPTIONS[oi];
        display_margins_set(s_margin_left, s_margin_right, s_margin_top, s_margin_bottom);
        s_restart_needed = true;
        refresh_settings_items();
        return true;
    }
    return false;
}

static void settings_activate_item(int row)
{
    const settings_item_t idx = settings_item_at(row);

    /* Moving away from the factory-reset item cancels confirmation */
    if (idx != SET_ITEM_RESET && s_factory_reset_confirm) {
        s_factory_reset_confirm = false;
        refresh_settings_items();
    }

    if (settings_cycle_item(row, 1)) {
        /* Handled -- Enter always means "next value", same as before. */
    } else if (idx == SET_ITEM_EDITOR_MENU) {
        settings_enter_level(SET_LVL_EDITOR);
    } else if (idx == SET_ITEM_DISPLAY_MENU) {
        settings_enter_level(SET_LVL_DISPLAY);
    } else if (idx == SET_ITEM_MARGINS_MENU) {
        settings_enter_level(SET_LVL_MARGINS);
    } else if (idx == SET_ITEM_KEYBOARD_MENU) {
        settings_enter_level(SET_LVL_KEYBOARD);
    } else if (idx == SET_ITEM_SYSTEM_MENU) {
        settings_enter_level(SET_LVL_SYSTEM);
    } else if (idx == SET_ITEM_LAYOUTS) {
        /* Open the active-layouts checklist sub-list. Each row toggles
         * immediately on Enter (applied + persisted on the spot); Esc
         * returns to the settings list. */
        s_layouts_picker_sel = 0;
        s_layouts_picker_open = true;
        refresh_layouts_picker_items();
#if defined(CONFIG_DRAFTLING_DISPLAY_COLOR)
    } else if (idx == SET_ITEM_THEME) {
        /* Open the theme picker sub-list. The user navigates with
         * Up/Down and confirms with Enter; only on Enter is the new
         * theme persisted and the screens rebuilt. Esc returns to
         * the settings list without changes. */
        s_theme_picker_open = true;
        s_theme_picker_sel = s_theme_idx;
        refresh_theme_picker_items();
#endif
    } else if (idx == SET_ITEM_RESET) {
        /* Factory reset -- requires double-press confirmation */
        if (s_factory_reset_confirm) {
            ESP_LOGW(TAG, "Factory reset: erasing NVS and restarting");
            nvs_flash_erase();
            esp_restart();
            /* does not return */
        } else {
            s_factory_reset_confirm = true;
            refresh_settings_items();
        }
    } else if (idx == SET_ITEM_BACK) {
        settings_go_back();
    }
}

static void handle_settings_key(const kb_event_t *ev)
{
    if (s_restart_confirm_open) {
        switch (ev->keycode) {
        case KB_KEY_UP:
        case KB_KEY_DOWN:
            s_restart_confirm_sel = s_restart_confirm_sel == 0 ? 1 : 0;
            update_restart_confirm_highlight();
            break;
        case KB_KEY_ENTER:
            if (s_restart_confirm_sel == 0) {
                ESP_LOGI(TAG, "Restarting to apply new display settings");
                esp_restart();
                /* does not return */
            }
            /* Keep editing: the new values stay saved in NVS and apply
             * on the next restart; do not ask again for them. */
            s_restart_confirm_open = false;
            s_restart_needed = false;
            settings_pop_level();
            break;
        case KB_KEY_ESCAPE:
            /* Same as "keep editing" -- Escape leaving the user stuck
             * re-answering the same prompt would be worse than just
             * deferring the restart, since the new value is already
             * saved to NVS and will apply on the next natural
             * restart/sleep-wake regardless. */
            s_restart_confirm_open = false;
            s_restart_needed = false;
            settings_pop_level();
            break;
        default:
            break;
        }
        return;
    }

#if defined(CONFIG_DRAFTLING_DISPLAY_COLOR)
    if (s_theme_picker_open) {
        /* Total picker rows: COLOR_THEME_COUNT themes + 1 "Cancel" row. */
        const int picker_count = COLOR_THEME_COUNT + 1;
        switch (ev->keycode) {
        case KB_KEY_UP:
            if (s_theme_picker_sel > 0) s_theme_picker_sel--;
            update_theme_picker_highlight();
            break;
        case KB_KEY_DOWN:
            if (s_theme_picker_sel < picker_count - 1) s_theme_picker_sel++;
            update_theme_picker_highlight();
            break;
        case KB_KEY_ENTER:
            if (s_theme_picker_sel >= 0 &&
                s_theme_picker_sel < COLOR_THEME_COUNT) {
                /* Apply the selected theme. The theme drives the bg
                 * color of every screen and the text color of dozens
                 * of widgets configured at create time, so we tear
                 * the screens down and rebuild them in place against
                 * the new palette -- no esp_restart() required. */
                if (s_theme_picker_sel != s_theme_idx) {
                    s_theme_idx = s_theme_picker_sel;
                    save_theme_to_nvs();
                    /* close_theme_picker() restores the regular
                     * settings list state; rebuild_screens_for_theme()
                     * then deletes every screen (including the one
                     * holding s_settings_list) and re-creates them
                     * under the new theme, restoring the user's
                     * current screen at the end. */
                    close_theme_picker();
                    rebuild_screens_for_theme();
                    return;
                }
                /* No change -> just close the picker. */
                close_theme_picker();
            } else {
                /* Cancel row */
                close_theme_picker();
            }
            break;
        case KB_KEY_ESCAPE:
            close_theme_picker();
            break;
        default:
            break;
        }
        return;
    }
#endif

    if (s_layouts_picker_open) {
        /* Total picker rows: KB_LAYOUT_COUNT checklist rows + 1 "Done" row. */
        const int picker_count = KB_LAYOUT_COUNT + 1;
        switch (ev->keycode) {
        case KB_KEY_UP:
            if (s_layouts_picker_sel > 0) s_layouts_picker_sel--;
            update_layouts_picker_highlight();
            break;
        case KB_KEY_DOWN:
            if (s_layouts_picker_sel < picker_count - 1) s_layouts_picker_sel++;
            update_layouts_picker_highlight();
            break;
        case KB_KEY_ENTER:
        case KB_KEY_SPACE:
            /* Space is equivalent to Enter here so a row can be
             * toggled without leaving the home row -- there is no
             * text field on this screen for Space to type into. */
            if (s_layouts_picker_sel >= 0 &&
                s_layouts_picker_sel < KB_LAYOUT_COUNT) {
                kb_layout_id_t id = s_layouts_order[s_layouts_picker_sel];
                /* kb_layout_set_active() silently refuses to remove the
                 * last active layout, so the checklist can never be
                 * emptied out from under the user. */
                if (kb_layout_set_active(id, !kb_layout_is_active(id))) {
                    save_active_layouts_to_nvs();
                    /* Removing the current layout switches to the next. */
                    save_current_layout_to_nvs();
                    /* A layout that types in the status bar keeps the
                     * bars on screen. */
                    if (!bars_may_hide()) bars_show();
                    update_layouts_picker_row(s_layouts_picker_sel);
                }
            } else {
                /* Done row */
                close_layouts_picker();
            }
            break;
        case KB_KEY_ESCAPE:
            close_layouts_picker();
            break;
        default:
            break;
        }
        return;
    }

    switch (ev->keycode) {
    case KB_KEY_UP:
        if (s_settings_sel > 0) s_settings_sel--;
        if (s_factory_reset_confirm) {
            /* Cancelling confirmation changes the item label, so we
             * need a full rebuild; otherwise just move the highlight. */
            s_factory_reset_confirm = false;
            refresh_settings_items();
        } else {
            update_settings_highlight();
        }
        break;
    case KB_KEY_DOWN:
        if (s_settings_sel < s_settings_item_count - 1) s_settings_sel++;
        if (s_factory_reset_confirm) {
            s_factory_reset_confirm = false;
            refresh_settings_items();
        } else {
            update_settings_highlight();
        }
        break;
    case KB_KEY_ENTER:
        settings_activate_item(s_settings_sel);
        break;
    case KB_KEY_LEFT:
        /* Previous value on the highlighted row, without moving the
         * highlight -- a no-op on rows settings_cycle_item() does not
         * own (sub-levels, pickers, Factory reset, Back). */
        settings_cycle_item(s_settings_sel, -1);
        break;
    case KB_KEY_RIGHT:
        settings_cycle_item(s_settings_sel, 1);
        break;
    case KB_KEY_ESCAPE:
        settings_go_back();
        break;
    default:
        break;
    }
}

/* Forward declarations for WiFi background connect (defined below) */
static void wifi_connect_task(void *arg);
static void wifi_connect_async(void);
static void wifi_scan_task(void *arg);
static void wifi_scan_async(void);
static void wifi_connect_picked_task(void *arg);
static void wifi_connect_picked_async(const char *ssid, const char *password);
static void draftbox_connect_async(const char *site, const char *otp);
/* Forward declaration: the password prompt's Esc handler returns to
 * the picker list defined further down. */
static void refresh_wifi_scan_picker_items(void);

/* ---- WiFi password-entry overlay ----
 * Raised by the WiFi scan picker below when the user picks a secured
 * network. A top-level modal in its own right (parented to
 * s_scr_menu, not the picker's s_menu_list), structurally a copy of
 * the save-as prompt's header/value-label/cursor trio. The password is
 * shown in clear text so the user can see what they type. Esc backs out to the SSID list rather than closing the whole
 * menu, so a wrong network pick is cheap to undo. */
static void refresh_wifi_pw_prompt(void)
{
    if (!s_wifi_pw_panel) return;

    lv_label_set_text(s_wifi_pw_name_lbl, s_wifi_pw_buf);

    int cw = char_width_for_font(FONT_11);
    int chars = 0;
    for (int i = 0; i < s_wifi_pw_pos; i++) {
        if ((s_wifi_pw_buf[i] & 0xC0) != 0x80) chars++;
    }
    lv_obj_set_pos(s_wifi_pw_cur, chars * cw, overlay_row_y());
    lv_obj_remove_flag(s_wifi_pw_cur, LV_OBJ_FLAG_HIDDEN);
}

static void show_wifi_password_prompt(const char *ssid)
{
    strncpy(s_wifi_pw_ssid, ssid, sizeof(s_wifi_pw_ssid) - 1);
    s_wifi_pw_ssid[sizeof(s_wifi_pw_ssid) - 1] = '\0';
    s_wifi_pw_buf[0] = '\0';
    s_wifi_pw_pos = 0;

    char hdr[64];
    snprintf(hdr, sizeof(hdr), "Password for %.32s (Enter/Esc):", s_wifi_pw_ssid);
    lv_label_set_text(s_wifi_pw_hdr_lbl, hdr);

    s_wifi_pw_open = true;
    lv_obj_remove_flag(s_wifi_pw_panel, LV_OBJ_FLAG_HIDDEN);
    refresh_wifi_pw_prompt();
}

static void close_wifi_pw_prompt(void)
{
    s_wifi_pw_open = false;
    lv_obj_add_flag(s_wifi_pw_panel, LV_OBJ_FLAG_HIDDEN);
}

static void wifi_pw_prompt_confirm(void)
{
    if (s_wifi_pw_buf[0] == '\0') return; /* empty password -- keep editing */
    char ssid[33];
    strncpy(ssid, s_wifi_pw_ssid, sizeof(ssid) - 1);
    ssid[sizeof(ssid) - 1] = '\0';
    close_wifi_pw_prompt();
    s_wifi_scan_picker_open = false;
    close_menu();
    wifi_connect_picked_async(ssid, s_wifi_pw_buf);
}

/* Line-editing keys shared by every single-line text prompt (WiFi
 * password, Draftbox site / password, save-as / rename name, Find /
 * Replace): cursor movement, Backspace / Delete and printable text in
 * the active keyboard layout. buf is UTF-8, pos a byte offset kept on a
 * character boundary. Enter, Esc (and Tab, where it means something)
 * are the caller's. accept, when given, may refuse a typed text given
 * the current length. Returns true if the text changed. */
typedef bool (*text_accept_fn)(const char *text, size_t cur_len);

static bool text_field_key(char *buf, size_t buf_sz, int *pos, const kb_event_t *ev,
                           text_accept_fn accept = NULL)
{
    switch (ev->keycode) {
    case KB_KEY_LEFT:
        if (*pos > 0) {
            do { (*pos)--; }
            while (*pos > 0 && (buf[*pos] & 0xC0) == 0x80);
        }
        return false;
    case KB_KEY_RIGHT:
        if (*pos < (int)strlen(buf)) {
            (*pos)++;
            while (*pos < (int)strlen(buf) && (buf[*pos] & 0xC0) == 0x80)
                (*pos)++;
        }
        return false;
    case KB_KEY_HOME:
        *pos = 0;
        return false;
    case KB_KEY_END:
        *pos = (int)strlen(buf);
        return false;
    case KB_KEY_BACKSPACE:
        if (*pos > 0) {
            int prev = *pos - 1;
            while (prev > 0 && (buf[prev] & 0xC0) == 0x80) prev--;
            int len = (int)strlen(buf);
            memmove(buf + prev, buf + *pos, (size_t)(len - *pos + 1));
            *pos = prev;
            return true;
        }
        return false;
    case KB_KEY_DELETE: {
        int len = (int)strlen(buf);
        if (*pos < len) {
            int next = *pos + 1;
            while (next < len && (buf[next] & 0xC0) == 0x80) next++;
            memmove(buf + *pos, buf + next, (size_t)(len - next + 1));
            return true;
        }
        return false;
    }
    default: {
        bool ctrl = (ev->modifier & (KB_MOD_LCTRL | KB_MOD_RCTRL)) != 0;
        if (ctrl) return false; /* swallow ctrl combos */
        const char *text = kb_layout_translate(ev->keycode, ev->modifier);
        if (!text || !text[0]) return false;
        if ((unsigned char)text[0] < 0x20) return false; /* reject control chars */
        size_t tlen = strlen(text);
        size_t cur_len = strlen(buf);
        if (accept && !accept(text, cur_len)) return false;
        if (cur_len + tlen >= buf_sz - 1) return false;
        memmove(buf + *pos + tlen, buf + *pos, cur_len - (size_t)*pos + 1);
        memcpy(buf + *pos, text, tlen);
        *pos += (int)tlen;
        return true;
    }
    }
}

static void handle_wifi_pw_prompt_key(const kb_event_t *ev)
{
    switch (ev->keycode) {
    case KB_KEY_ENTER:
        wifi_pw_prompt_confirm();
        return;
    case KB_KEY_ESCAPE:
        /* Back to the SSID list, not out of the whole menu. */
        close_wifi_pw_prompt();
        refresh_wifi_scan_picker_items();
        return;
    default:
        text_field_key(s_wifi_pw_buf, sizeof(s_wifi_pw_buf), &s_wifi_pw_pos, ev);
        break;
    }
    refresh_wifi_pw_prompt();
}

/* ---- Message pop-up (see s_msg_panel) ---- */
static void show_msg_popup(const char *title, const char *text, bool busy)
{
    if (!s_msg_panel) return;
#if defined(CONFIG_DRAFTLING_HAS_KERMIT)
    if (s_msg_kermit) return;   /* the Sync via browser dialog stays up */
#endif
    if (s_msg_restart) return;  /* so does the missing SD card one */
    lv_label_set_text(s_msg_title_lbl, title);
    lv_label_set_text(s_msg_text_lbl, text);
    if (busy) lv_obj_add_flag(s_msg_hint_lbl, LV_OBJ_FLAG_HIDDEN);
    else      lv_obj_remove_flag(s_msg_hint_lbl, LV_OBJ_FLAG_HIDDEN);
    s_msg_busy = busy;
    s_msg_open = true;
    lv_obj_set_parent(s_msg_panel, lv_scr_act());
    lv_obj_remove_flag(s_msg_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_msg_panel);
    lv_obj_update_layout(s_msg_panel);
    lv_obj_align(s_msg_panel, LV_ALIGN_CENTER, 0, 0);
}

static void close_msg_popup(void)
{
    s_msg_open = false;
    s_msg_busy = false;
    if (s_msg_restart || s_msg_migrate) {
        s_msg_restart = false;
        s_msg_migrate = false;
        lv_label_set_text(s_msg_hint_lbl, "Enter / Esc: close");
    }
    if (s_msg_panel) lv_obj_add_flag(s_msg_panel, LV_OBJ_FLAG_HIDDEN);
#if defined(CONFIG_DRAFTLING_HAS_KERMIT)
    if (s_msg_kermit) {
        s_msg_kermit = false;
        lv_label_set_text(s_msg_hint_lbl, "Enter / Esc: close");
    }
#endif
}

#if defined(CONFIG_DRAFTLING_HAS_KERMIT)
static void kermit_finish(void);
#endif

static void show_name_prompt(save_mode_t mode, const char *hdr, const char *initial);
static void default_project_name(char *out, size_t n);

/* "Files outside projects" pop-up: Enter asks for the new project's
 * name; Esc powers off (deep sleep), the files stay where they are. */
static void show_migrate_picker(void);

static void migrate_popup_accept(void)
{
    close_msg_popup();
    if (project_any()) {
        /* Existing projects to choose from, or a new one. */
        show_migrate_picker();
        return;
    }
    char def[PROJECT_NAME_MAX + 1];
    default_project_name(def, sizeof(def));
    show_name_prompt(SAVE_MODE_PROJECT_MIGRATE, "Name of the new project (Enter/Esc):", def);
}

static void migrate_popup_power_off(void)
{
    ESP_LOGI(TAG, "Files outside projects not moved: powering off");
    standby_enter_sleep();
    /* Only returns while sleep is inhibited: keep asking. */
}

static void handle_msg_popup_key(const kb_event_t *ev)
{
#if defined(CONFIG_DRAFTLING_HAS_KERMIT)
    if (s_msg_kermit) {
        if (ev->keycode == KB_KEY_ENTER || ev->keycode == KB_KEY_ESCAPE)
            kermit_finish();
        return;
    }
#endif
    if (s_msg_busy) return;
    if (s_msg_migrate) {
        if (ev->keycode == KB_KEY_ENTER) migrate_popup_accept();
        else if (ev->keycode == KB_KEY_ESCAPE) migrate_popup_power_off();
        return;
    }
    if (s_msg_restart) {
        /* No way out but a restart with a card inserted. */
        if (ev->keycode == KB_KEY_ENTER) {
            ESP_LOGI(TAG, "Restarting to look for the SD card again");
            esp_restart();
        }
        return;
    }
    if (ev->keycode == KB_KEY_ENTER || ev->keycode == KB_KEY_ESCAPE)
        close_msg_popup();
}

#if defined(CONFIG_DRAFTLING_TOUCHSCREEN)
static void msg_popup_touch_cb(lv_event_t *e)
{
    (void)e;
#if defined(CONFIG_DRAFTLING_HAS_KERMIT)
    if (s_msg_kermit) {
        kermit_finish();
        return;
    }
#endif
    if (s_msg_restart) esp_restart();
    if (s_msg_migrate) {
        migrate_popup_accept();
        return;
    }
    if (!s_msg_busy) close_msg_popup();
}
#endif

/* ---- Draftbox prompt (see s_dbx_panel) ---- */

/* Size the panel for `rows` value rows (the server choice needs two)
 * and keep it centred on the screen. */
static void dbx_panel_fit(int rows)
{
    int line_h = lv_font_get_line_height(FONT_11);
    int h = overlay_row_y() + rows * line_h + 2 * OVERLAY_PAD;
    lv_obj_set_height(s_dbx_panel, h);
    lv_obj_set_y(s_dbx_panel, (SCR_H - h) / 2);
}

/* The site the password is exchanged with. */
static const char *dbx_site(void)
{
    return s_dbx_custom ? s_dbx_site : DBX_DEFAULT_SITE;
}

static void refresh_dbx_prompt(void)
{
    if (!s_dbx_panel) return;

    if (s_dbx_step == DBX_STEP_WIFI) {
        char cfg_ssid[33];
        char hdr[80];
        if (wifi_manager_get_configured_ssid(cfg_ssid, sizeof(cfg_ssid)))
            snprintf(hdr, sizeof(hdr), "Connecting WiFi to %s... (Esc: cancel)", cfg_ssid);
        else
            snprintf(hdr, sizeof(hdr), "Connecting WiFi... (Esc: cancel)");
        lv_label_set_text(s_dbx_hdr_lbl, hdr);
        lv_label_set_text(s_dbx_val_lbl, "");
        lv_obj_add_flag(s_dbx_cur, LV_OBJ_FLAG_HIDDEN);
        dbx_panel_fit(1);
        return;
    }

    if (s_dbx_step == DBX_STEP_SERVER) {
        lv_label_set_text(s_dbx_hdr_lbl,
            "Draftbox server (Up/Down, Enter: next, Esc: cancel):");
        lv_label_set_text(s_dbx_val_lbl, s_dbx_custom
            ? "  " DBX_DEFAULT_SITE "\n> connect to a different server..."
            : "> " DBX_DEFAULT_SITE "\n  connect to a different server...");
        lv_obj_add_flag(s_dbx_cur, LV_OBJ_FLAG_HIDDEN);
        dbx_panel_fit(2);
        return;
    }
    dbx_panel_fit(1);

    bool site = (s_dbx_step == DBX_STEP_SITE);
    const char *buf = site ? s_dbx_site : s_dbx_otp;
    int pos = site ? s_dbx_site_pos : s_dbx_otp_pos;

    lv_label_set_text(s_dbx_hdr_lbl, site
        ? "Draftbox server name (Enter: next, Esc: back):"
        : "8-digit one-time password (Enter/Esc):");
    lv_label_set_text(s_dbx_val_lbl, buf);

    int cw = char_width_for_font(FONT_11);
    int chars = 0;
    for (int i = 0; i < pos; i++) {
        if ((buf[i] & 0xC0) != 0x80) chars++;
    }
    lv_obj_set_pos(s_dbx_cur, chars * cw, overlay_row_y());
    lv_obj_remove_flag(s_dbx_cur, LV_OBJ_FLAG_HIDDEN);
}

static void show_dbx_prompt(bool wait_wifi)
{
    s_dbx_step = wait_wifi ? DBX_STEP_WIFI : DBX_STEP_SERVER;
    s_dbx_site_pos = (int)strlen(s_dbx_site);
    memset(s_dbx_otp, 0, sizeof(s_dbx_otp));
    s_dbx_otp_pos = 0;
    s_dbx_open = true;
    lv_obj_remove_flag(s_dbx_panel, LV_OBJ_FLAG_HIDDEN);
    refresh_dbx_prompt();
}

static void close_dbx_prompt(void)
{
    s_dbx_open = false;
    memset(s_dbx_otp, 0, sizeof(s_dbx_otp));
    s_dbx_otp_pos = 0;
    lv_obj_add_flag(s_dbx_panel, LV_OBJ_FLAG_HIDDEN);
}

/* Outcome of the WiFi connection started for the prompt (from
 * wifi_state_cb() or wifi_connect_task(), LVGL lock held). On success
 * go on to the site name; on failure close the prompt and explain why
 * in a pop-up (reason is a full sentence; unused on success). */
static void dbx_wifi_done(bool connected, const char *reason)
{
    if (!s_dbx_open || s_dbx_step != DBX_STEP_WIFI) return;
    if (connected) {
        s_dbx_step = DBX_STEP_SERVER;
        refresh_dbx_prompt();
    } else {
        close_dbx_prompt();
        char text[200];
        snprintf(text, sizeof(text), "Could not connect to WiFi.\n%s",
                 reason ? reason : "");
        show_msg_popup("Draftbox", text, false);
    }
}

static void handle_dbx_prompt_key(const kb_event_t *ev)
{
    if (s_dbx_step == DBX_STEP_WIFI) {
        /* Only Esc while connecting; the connection attempt itself
         * carries on in the background. */
        if (ev->keycode == KB_KEY_ESCAPE) close_dbx_prompt();
        return;
    }
    if (s_dbx_step == DBX_STEP_SERVER) {
        switch (ev->keycode) {
        case KB_KEY_UP:
        case KB_KEY_DOWN:
        case KB_KEY_TAB:
            s_dbx_custom = !s_dbx_custom;
            break;
        case KB_KEY_ENTER:
            s_dbx_step = s_dbx_custom ? DBX_STEP_SITE : DBX_STEP_OTP;
            s_dbx_site_pos = (int)strlen(s_dbx_site);
            break;
        case KB_KEY_ESCAPE:
            close_dbx_prompt();
            return;
        default:
            return;
        }
        refresh_dbx_prompt();
        return;
    }
    switch (ev->keycode) {
    case KB_KEY_ENTER:
        if (s_dbx_step == DBX_STEP_SITE) {
            if (s_dbx_site[0] == '\0') return; /* keep editing */
            s_dbx_step = DBX_STEP_OTP;
        } else {
            if (strlen(s_dbx_otp) != 8) return; /* keep editing */
            char site[sizeof(s_dbx_site)];
            char otp[sizeof(s_dbx_otp)];
            snprintf(site, sizeof(site), "%s", dbx_site());
            snprintf(otp, sizeof(otp), "%s", s_dbx_otp);
            close_dbx_prompt();
            /* The menu stays open underneath; the pop-up shows the
             * progress and then the result. */
            char text[192];
            snprintf(text, sizeof(text), "Connecting to %s...", site);
            show_msg_popup("Draftbox", text, true);
            draftbox_connect_async(site, otp);
            memset(otp, 0, sizeof(otp));
            return;
        }
        break;
    case KB_KEY_ESCAPE:
        if (s_dbx_step == DBX_STEP_OTP) {
            /* Back to the site name (custom server, e.g. to fix a
             * typo) or to the server choice. */
            memset(s_dbx_otp, 0, sizeof(s_dbx_otp));
            s_dbx_otp_pos = 0;
            s_dbx_step = s_dbx_custom ? DBX_STEP_SITE : DBX_STEP_SERVER;
            break;
        }
        /* Site name: back to the server choice. */
        s_dbx_step = DBX_STEP_SERVER;
        break;
    default:
        if (s_dbx_step == DBX_STEP_SITE) {
            text_field_key(s_dbx_site, sizeof(s_dbx_site), &s_dbx_site_pos, ev);
        } else {
            /* Digits only; editing keys pass through. A dead key
             * (translate returned NULL and left an accent pending)
             * must not reach text_field_key(), which would translate
             * the key again and type the accent. */
            const char *text = kb_layout_translate(ev->keycode, ev->modifier);
            if (!text && kb_layout_dead_key_pending()) break;
            bool printable = text && (unsigned char)text[0] >= 0x20;
            if (!printable || (text[0] >= '0' && text[0] <= '9' && !text[1]))
                text_field_key(s_dbx_otp, sizeof(s_dbx_otp), &s_dbx_otp_pos, ev);
        }
        break;
    }
    refresh_dbx_prompt();
}

/* ---- WiFi scan/connect picker ----
 * Renders scan results into the same s_menu_list widget while
 * s_wifi_scan_picker_open is true, mirroring the "SD card via USB"
 * picker's reuse of s_menu_list below. A trailing "Rescan" row always
 * follows the SSID list (or the "No networks found" placeholder). */
static void refresh_wifi_scan_picker_items(void)
{
    lv_obj_clean(s_menu_list);
    char buf[48];
    if (s_wifi_scan_in_progress) {
        lv_list_add_btn(s_menu_list, NULL, "Scanning...");
    } else {
        if (s_wifi_scan_count == 0) {
            lv_list_add_btn(s_menu_list, NULL, "No networks found");
        }
        for (int i = 0; i < s_wifi_scan_count; i++) {
            /* [saved]: the system has the password, picking the network
             * connects without asking (see wifi_scan_picker_activate()). */
            const wifi_scan_result_t *ap = &s_wifi_scan_results[i];
            bool saved = ap->known && strcmp(ap->ssid, s_wifi_known_refused) != 0;
            snprintf(buf, sizeof(buf), "%s%.32s",
                     saved ? "[saved] " : ap->open ? "" : "[locked] ", ap->ssid);
            lv_list_add_btn(s_menu_list, NULL, buf);
        }
        lv_list_add_btn(s_menu_list, NULL, "Rescan");
    }
    apply_list_selection_styles(s_menu_list, s_wifi_scan_picker_sel);
    s_wifi_scan_picker_sel_prev = s_wifi_scan_picker_sel;
    sync_battery_labels();
}

static void update_wifi_scan_picker_highlight(void)
{
    update_list_highlight(s_menu_list, s_wifi_scan_picker_sel,
                          s_wifi_scan_picker_sel_prev);
    s_wifi_scan_picker_sel_prev = s_wifi_scan_picker_sel;
}

/* Leave the picker back to the regular F1 menu list without
 * connecting (Esc). */
static void close_wifi_scan_picker(void)
{
    s_wifi_scan_picker_open = false;
    refresh_menu_items();
}

/* (Re)start a scan: shows "Scanning..." immediately: the background
 * task calls refresh_wifi_scan_picker_items() again once results (or
 * a failure) are in -- see wifi_scan_task(). */
static void start_wifi_scan(void)
{
    s_wifi_scan_in_progress = true;
    s_wifi_scan_picker_sel = 0;
    refresh_wifi_scan_picker_items();
    wifi_scan_async();
}

/* Row index of the trailing "Rescan" entry: right after the last SSID
 * row normally, but when there are no results the picker inserts a
 * "No networks found" placeholder row first (see
 * refresh_wifi_scan_picker_items()), which "Rescan" then follows
 * instead. */
static int wifi_scan_rescan_row(void)
{
    return (s_wifi_scan_count > 0) ? s_wifi_scan_count : 1;
}

/* Enter on a picker row: the trailing row is "Rescan" (see
 * wifi_scan_rescan_row()); an open network connects immediately, a
 * secured one raises the password prompt first. */
static void wifi_scan_picker_activate(int idx)
{
    if (idx == wifi_scan_rescan_row()) {
        start_wifi_scan();
        return;
    }
    if (idx < 0 || idx >= s_wifi_scan_count) return; /* also covers the "No networks found" row */

    const wifi_scan_result_t *ap = &s_wifi_scan_results[idx];
    /* A network the system already has the password for (the Linux
     * program: a NetworkManager profile) connects without asking,
     * unless that password was just refused. */
    bool known = ap->known && strcmp(ap->ssid, s_wifi_known_refused) != 0;
    if (ap->open || known) {
        s_wifi_scan_picker_open = false;
        close_menu();
        wifi_connect_picked_async(ap->ssid, "");
    } else {
        if (strcmp(ap->ssid, s_wifi_known_refused) == 0) s_wifi_known_refused[0] = '\0';
        show_wifi_password_prompt(ap->ssid);
    }
}

#if defined(CONFIG_DRAFTLING_HAS_USB_MSC)
/* ---- "SD card via USB" picker ----
 * Renders the 3 modes into the same s_menu_list widget while
 * s_usbmsc_picker_open is true, mirroring the color-theme picker's
 * reuse of s_settings_list. Picking a row (Enter) stages the choice
 * in s_usbmsc_pending_mode; either way (Enter or Esc), leaving the
 * picker back to the regular F1 menu list runs
 * commit_usb_msc_pending_mode() -- see close_usbmsc_picker(). */
static void refresh_usbmsc_picker_items(void)
{
    lv_obj_clean(s_menu_list);
    char buf[40];
    static const usb_msc_mode_t kModes[3] = {
        USB_MSC_MODE_OFF, USB_MSC_MODE_READ_ONLY, USB_MSC_MODE_READ_WRITE
    };
    for (int i = 0; i < 3; i++) {
        snprintf(buf, sizeof(buf), "%s%s",
                 (kModes[i] == s_usbmsc_pending_mode) ? "* " : "  ",
                 usb_msc_mode_name(kModes[i]));
        lv_list_add_btn(s_menu_list, NULL, buf);
    }
    apply_list_selection_styles(s_menu_list, s_usbmsc_picker_sel);
    s_usbmsc_picker_sel_prev = s_usbmsc_picker_sel;
    sync_battery_labels();
}

static void update_usbmsc_picker_highlight(void)
{
    update_list_highlight(s_menu_list, s_usbmsc_picker_sel,
                          s_usbmsc_picker_sel_prev);
    s_usbmsc_picker_sel_prev = s_usbmsc_picker_sel;
}

/* Leave the picker (Enter or Esc) and re-render the regular F1 menu
 * list. This is the point where the staged choice actually takes
 * effect -- see commit_usb_msc_pending_mode(). Esc leaves
 * s_usbmsc_pending_mode unchanged, so the commit is a no-op then.
 *
 * Turning USB-MSC off restarts the device (see usb_msc.h) -- this
 * function does not return in that case. */
static void close_usbmsc_picker(void)
{
    s_usbmsc_picker_open = false;
    commit_usb_msc_pending_mode();
    refresh_menu_items();
}
#endif

#if defined(CONFIG_DRAFTLING_HAS_KERMIT)
/* ---- "Sync via browser": Kermit server on the serial console ----
 * F1 -> "Sync via browser..." asks Start / Cancel. Start hands the SD
 * card and the console to components/kermit: every open document is
 * saved and closed, the editor is left for the file browser, and the
 * "Sync via browser" dialog (the message pop-up with s_msg_kermit set)
 * tells the user to open the Draftbridge web page. The dialog is modal
 * for the whole session; Finished (Enter / Esc / a tap) stops the
 * server.
 *
 * s_kermit_usb_timer watches the USB connection (kermit_usb_poll_cb()):
 * until a computer has enumerated the server's USB serial port the
 * dialog asks the user to plug the cable in, and when the connection
 * goes away it says so. The device stays awake only while a computer
 * is connected; otherwise the normal inactivity sleep applies. */

#define KERMIT_BRIDGE_TEXT \
    "On a computer connected to this device's USB port, open " \
    "https://bridge.draftling.art/ in Chrome, Edge or Opera and follow " \
    "the instructions there. Select Finished when the synchronization " \
    "is complete."

#define KERMIT_STARTING_TEXT "Starting the file server..."

#define KERMIT_CONNECT_TEXT \
    "Connect this device to a computer with a USB cable."

#define KERMIT_LOST_TEXT \
    "The USB connection to the computer was lost. Connect the cable " \
    "again to continue, or select Finished."

/* Time a computer gets to enumerate the USB serial port after the
 * server starts before the dialog asks for the cable. */
#define KERMIT_USB_GRACE_MS  3000
#define KERMIT_USB_POLL_MS   500

enum kermit_usb_state_t {
    KUSB_STARTING,    /* server just started, waiting for enumeration */
    KUSB_WAITING,     /* never connected: asking for the cable */
    KUSB_CONNECTED,
    KUSB_LOST,        /* was connected, now gone */
};
static kermit_usb_state_t s_kermit_usb_state = KUSB_STARTING;
static lv_timer_t *s_kermit_usb_timer = NULL;
static uint32_t    s_kermit_start_ms  = 0;

static void kermit_set_dialog_text(const char *text)
{
    lv_label_set_text(s_msg_text_lbl, text);
    lv_obj_update_layout(s_msg_panel);
    lv_obj_align(s_msg_panel, LV_ALIGN_CENTER, 0, 0);
}

static void kermit_set_usb_state(kermit_usb_state_t st)
{
    if (st == s_kermit_usb_state) return;
    s_kermit_usb_state = st;
    /* Stay awake only while a computer can talk to the server. */
    standby_set_inhibit(st == KUSB_CONNECTED);
    switch (st) {
    case KUSB_CONNECTED:
        kermit_set_dialog_text(KERMIT_BRIDGE_TEXT);
        lv_label_set_text(s_msg_hint_lbl, "Enter / Esc: Finished");
        break;
    case KUSB_WAITING:
        kermit_set_dialog_text(KERMIT_CONNECT_TEXT);
        lv_label_set_text(s_msg_hint_lbl, "Enter / Esc: Cancel");
        break;
    case KUSB_LOST:
        kermit_set_dialog_text(KERMIT_LOST_TEXT);
        lv_label_set_text(s_msg_hint_lbl, "Enter / Esc: Finished");
        break;
    default:
        break;
    }
}

/* LVGL timer: follow the USB connection while the dialog is up. */
static void kermit_usb_poll_cb(lv_timer_t *t)
{
    (void)t;
    if (!s_msg_kermit) return;
    int c = kermit_server_usb_connected();
    if (c != 0) {
        /* -1: no USB port to watch (UART only) -- assume a client. */
        kermit_set_usb_state(KUSB_CONNECTED);
    } else if (s_kermit_usb_state == KUSB_CONNECTED) {
        kermit_set_usb_state(KUSB_LOST);
    } else if (s_kermit_usb_state == KUSB_STARTING &&
               lv_tick_elaps(s_kermit_start_ms) >= KERMIT_USB_GRACE_MS) {
        kermit_set_usb_state(KUSB_WAITING);
    }
}

/* Runs on the Kermit server task. A short lock timeout: Stop waits for
 * that task while holding the LVGL lock, and a dropped progress line
 * is harmless. */
static void kermit_status_cb(const char *msg, bool files_changed)
{
    (void)files_changed;   /* the file list is refreshed on Finished */
    if (!draftling_lvgl_port_lock(200)) return;
    if (s_msg_kermit && s_kermit_usb_state == KUSB_CONNECTED) {
        char buf[sizeof(KERMIT_BRIDGE_TEXT) + 96];
        snprintf(buf, sizeof(buf), KERMIT_BRIDGE_TEXT "\n\n%s", msg);
        kermit_set_dialog_text(buf);
    }
    draftling_lvgl_port_unlock();
}

static void kermit_untitled_cb(editor_doc_t *doc, void *ctx)
{
    if (editor_doc_is_modified(doc) && !editor_doc_get_path(doc))
        *(bool *)ctx = true;
}

static void kermit_save_cb(editor_doc_t *doc, void *ctx)
{
    if (editor_doc_is_modified(doc) && editor_doc_get_path(doc) &&
        editor_doc_save(doc) != ESP_OK)
        *(bool *)ctx = true;
}

/* Drop a document's file so nothing can write the old content back
 * over a file the client replaces. */
static void kermit_close_cb(editor_doc_t *doc, void *ctx)
{
    (void)ctx;
    if (editor_doc_get_path(doc)) {
        editor_set_active(doc);
        editor_close_file();
    }
}

static void kermit_start(void)
{
    if (kermit_server_is_running()) return;
    const char *busy = sd_handed_off_msg();
    if (busy) {
        editor_ui_set_status(busy);
        return;
    }
    if (!sd_card_is_ready()) {
        editor_ui_set_status("Sync via browser: no " STORAGE_NAME);
        return;
    }
    if (git_sync_get_state() == GIT_SYNC_IN_PROGRESS) {
        editor_ui_set_status("Sync via browser: wait for Git sync to finish");
        return;
    }
    project_name_t kproj;
    if (!context_project(kproj, sizeof(kproj))) {
        editor_ui_set_status("Sync via browser: select a project first");
        return;
    }
    bool untitled = false;
    editor_doc_foreach(kermit_untitled_cb, &untitled);
    if (untitled) {
        editor_ui_set_status("Sync via browser: save or discard the new document first");
        return;
    }
    bool save_failed = false;
    editor_doc_foreach(kermit_save_cb, &save_failed);
    if (save_failed) {
        editor_ui_set_status("Sync via browser: could not save an open document");
        return;
    }
    editor_ui_apply_split_mode(SPLIT_NONE);
    editor_doc_foreach(kermit_close_cb, NULL);
    pane_bind_focus();
    s_editor_screen_active = false;   /* close_menu() goes to the browser */

    /* The client sees the project folder: its documents and git.cfg,
     * the layout Draftbridge expects. */
    static char kdir[64];
    project_dir(kproj, kdir, sizeof(kdir));
    esp_err_t err = kermit_server_start(kdir, kermit_status_cb);
    if (err != ESP_OK) {
        char buf[80];
        snprintf(buf, sizeof(buf), "Sync via browser: failed to start (%s)",
                 esp_err_to_name(err));
        editor_ui_set_status(buf);
        return;
    }
    close_menu();
    char ktitle[PROJECT_NAME_MAX + 24];
    snprintf(ktitle, sizeof(ktitle), "Sync via browser: %s", kproj);
    show_msg_popup(ktitle, KERMIT_STARTING_TEXT, false);
    s_msg_kermit = true;
    lv_label_set_text(s_msg_hint_lbl, "Enter / Esc: Cancel");
    /* Sleep stays allowed until a computer connects. */
    s_kermit_usb_state = KUSB_STARTING;
    s_kermit_start_ms = lv_tick_get();
    if (!s_kermit_usb_timer)
        s_kermit_usb_timer = lv_timer_create(kermit_usb_poll_cb,
                                             KERMIT_USB_POLL_MS, NULL);
}

/* "Finished" in the Sync via browser dialog. */
static void kermit_finish(void)
{
    if (s_kermit_usb_timer) {
        lv_timer_delete(s_kermit_usb_timer);
        s_kermit_usb_timer = NULL;
    }
    close_msg_popup();
    kermit_server_stop();
    standby_set_inhibit(false);
    if (lv_scr_act() == s_scr_browser) refresh_file_list();
    editor_ui_set_status(kermit_server_is_running()
                             ? "Sync via browser: stopping..."
                             : "Sync via browser: finished");
}

#if defined(CONFIG_DRAFTLING_TOUCHSCREEN)
static list_touch_ctx_t s_kermit_touch_ctx;
#endif
static void kermit_picker_activate(int idx);

static void refresh_kermit_picker_items(void)
{
    lv_label_set_text(s_lbl_menu_hdr, "Sync via browser: start the file server?");
    lv_obj_clean(s_menu_list);
    lv_list_add_btn(s_menu_list, NULL, "Start");
    lv_list_add_btn(s_menu_list, NULL, "Cancel");
    apply_list_selection_styles(s_menu_list, s_kermit_picker_sel);
    s_kermit_picker_sel_prev = s_kermit_picker_sel;
#if defined(CONFIG_DRAFTLING_TOUCHSCREEN)
    s_kermit_touch_ctx.p_sel      = &s_kermit_picker_sel;
    s_kermit_touch_ctx.p_sel_prev = &s_kermit_picker_sel_prev;
    s_kermit_touch_ctx.activate   = kermit_picker_activate;
    list_touch_attach(s_menu_list, &s_kermit_touch_ctx);
#endif
    sync_battery_labels();
}

static void update_kermit_picker_highlight(void)
{
    update_list_highlight(s_menu_list, s_kermit_picker_sel,
                          s_kermit_picker_sel_prev);
    s_kermit_picker_sel_prev = s_kermit_picker_sel;
}

static void close_kermit_picker(void)
{
    s_kermit_picker_open = false;
    lv_label_set_text(s_lbl_menu_hdr, MENU_HDR_TEXT);
    refresh_menu_items();
}

/* Row 0 = Start, 1 = Cancel (Enter or a second tap). */
static void kermit_picker_activate(int idx)
{
    close_kermit_picker();
    if (idx == 0) kermit_start();
}
#endif /* CONFIG_DRAFTLING_HAS_KERMIT */

/* ---- Book browser and contents list (see s_books_open) ---- */
#if defined(CONFIG_DRAFTLING_TOUCHSCREEN)
static list_touch_ctx_t s_books_touch_ctx;
static list_touch_ctx_t s_toc_touch_ctx;
#endif
static void books_activate(int idx);
static void toc_activate(int idx);
static void book_show_chapter(int chapter, bool at_end);

/* List s_books_dir: ".." (below the root), the folders, the books. */
static bool books_load(void)
{
    books_free();
    char dir[320];
    snprintf(dir, sizeof(dir), "%s%s%s", sd_card_get_mount_point(),
             s_books_dir[0] ? "/" : "", s_books_dir);
    if (sd_card_list_dir(dir, &s_books_list, &s_books_count) != ESP_OK) {
        s_books_list = NULL;
        s_books_count = 0;
        return false;
    }
    s_books_rows = (int *)malloc((size_t)(s_books_count + 1) * sizeof(int));
    if (!s_books_rows) { books_free(); return false; }
    if (s_books_dir[0]) s_books_rows[s_books_nrows++] = -1;
    /* sd_card_list_dir() puts the folders first. */
    for (int i = 0; i < s_books_count && s_books_nrows < BOOKS_MAX_ROWS; i++) {
        const sd_card_file_entry_t &e = s_books_list[i];
        if (e.is_dir ? !strcasecmp(e.name, "System Volume Information")
                     : !editor_path_is_book(e.name)) continue;
        s_books_rows[s_books_nrows++] = i;
    }
    return true;
}

static void refresh_books_items(void)
{
    char buf[300];
    snprintf(buf, sizeof(buf), "Open a book: /%.200s", s_books_dir);
    lv_label_set_text(s_lbl_menu_hdr, buf);
    lv_obj_clean(s_menu_list);
    for (int r = 0; r < s_books_nrows; r++) {
        int i = s_books_rows[r];
        if (i < 0) snprintf(buf, sizeof(buf), "../  (up)");
        else if (s_books_list[i].is_dir) snprintf(buf, sizeof(buf), "%.255s/", s_books_list[i].name);
        else snprintf(buf, sizeof(buf), "  %.255s", s_books_list[i].name);
        lv_list_add_btn(s_menu_list, NULL, buf);
    }
    if (s_books_nrows == 0) lv_list_add_btn(s_menu_list, NULL, "(no books or folders here)");
    if (s_books_sel >= s_books_nrows) s_books_sel = s_books_nrows ? s_books_nrows - 1 : 0;
    apply_list_selection_styles(s_menu_list, s_books_sel);
    s_books_sel_prev = s_books_sel;
#if defined(CONFIG_DRAFTLING_TOUCHSCREEN)
    s_books_touch_ctx.p_sel      = &s_books_sel;
    s_books_touch_ctx.p_sel_prev = &s_books_sel_prev;
    s_books_touch_ctx.activate   = books_activate;
    list_touch_attach(s_menu_list, &s_books_touch_ctx);
#endif
    sync_battery_labels();
}

/* Into a sub-folder (name) or up (NULL); the folder left is selected
 * on the way up. */
static void books_enter(const char *name)
{
    char left[256] = "";
    if (name) {
        size_t n = strlen(s_books_dir);
        if (n + 1 + strlen(name) >= sizeof(s_books_dir)) {
            editor_ui_set_status("Folder path too long");
            return;
        }
        snprintf(s_books_dir + n, sizeof(s_books_dir) - n, "%s%s", n ? "/" : "", name);
    } else {
        char *slash = strrchr(s_books_dir, '/');
        snprintf(left, sizeof(left), "%s", slash ? slash + 1 : s_books_dir);
        if (slash) *slash = '\0';
        else s_books_dir[0] = '\0';
    }
    if (!books_load()) {
        /* The folder went away: start again from the root. */
        s_books_dir[0] = '\0';
        if (!books_load()) {
            editor_ui_set_status("Cannot read the " STORAGE_NAME);
            return;
        }
    }
    s_books_sel = 0;
    for (int r = 0; left[0] && r < s_books_nrows; r++) {
        int i = s_books_rows[r];
        if (i >= 0 && s_books_list[i].is_dir && !strcmp(s_books_list[i].name, left)) {
            s_books_sel = r;
            break;
        }
    }
    refresh_books_items();
}

static void open_books_picker(void)
{
    if (const char *busy = sd_handed_off_msg()) {
        editor_ui_set_status(busy);
        return;
    }
    /* The book replaces the focused pane's document. */
    if (s_editor_screen_active && editor_doc_is_modified(s_panes[s_focus].doc)) {
        if (s_menu_open) close_menu();
        editor_ui_set_status("Save the document first (Ctrl+S)");
        return;
    }
    if (!s_menu_open) {
        if (s_inpane_browser_open) close_inpane_browser();
        show_menu();
    }
    if (!books_load()) {
        s_books_dir[0] = '\0';
        if (!books_load()) {
            editor_ui_set_status("Cannot read the " STORAGE_NAME);
            return;
        }
    }
    s_books_open = true;
    s_books_sel = 0;
    refresh_books_items();
}

static void close_books_picker(void)
{
    s_books_open = false;
    books_free();
    lv_label_set_text(s_lbl_menu_hdr, MENU_HDR_TEXT);
    refresh_menu_items();
}

static void books_activate(int r)
{
    if (r < 0 || r >= s_books_nrows) return;
    int i = s_books_rows[r];
    if (i < 0) { books_enter(NULL); return; }
    if (s_books_list[i].is_dir) { books_enter(s_books_list[i].name); return; }
    char path[600];   /* mount point + folder (255) + name (255) */
    snprintf(path, sizeof(path), "%s%s%s/%s", sd_card_get_mount_point(),
             s_books_dir[0] ? "/" : "", s_books_dir, s_books_list[i].name);
    s_books_open = false;
    books_free();
    lv_label_set_text(s_lbl_menu_hdr, MENU_HDR_TEXT);
    s_menu_open = false;
    if (s_editor_screen_active) {
        /* The focused document is replaced: keep its position. */
        pane_bind_focus();
        editor_save_meta();
    }
    if (!open_path_in_editor(path)) {
        /* Back where the menu was opened from, with the reason. */
        char why[96];
        snprintf(why, sizeof(why), "%s", editor_book_error());
        close_menu();
        editor_ui_set_status(why);
    }
}

static void handle_books_key(const kb_event_t *ev)
{
    switch (ev->keycode) {
    case KB_KEY_UP:
        if (s_books_sel > 0) s_books_sel--;
        break;
    case KB_KEY_DOWN:
        if (s_books_sel + 1 < s_books_nrows) s_books_sel++;
        break;
    case KB_KEY_ENTER:
        books_activate(s_books_sel);
        return;
    case KB_KEY_BACKSPACE:
        if (s_books_dir[0]) books_enter(NULL);
        return;
    case KB_KEY_ESCAPE:
        close_books_picker();
        return;
    default:
        return;
    }
    update_list_highlight(s_menu_list, s_books_sel, s_books_sel_prev);
    s_books_sel_prev = s_books_sel;
}

static void refresh_toc_items(void)
{
    char buf[160];
    const char *title = editor_book_title();
    snprintf(buf, sizeof(buf), "Contents: %.100s", title ? title : "book");
    lv_label_set_text(s_lbl_menu_hdr, buf);
    lv_obj_clean(s_menu_list);
    int n = editor_book_chapter_count();
    if (n > TOC_MAX_ROWS) n = TOC_MAX_ROWS;
    for (int i = 0; i < n; i++) {
        const char *t = editor_book_chapter_title(i);
        if (t) snprintf(buf, sizeof(buf), "%d. %.120s", i + 1, t);
        else snprintf(buf, sizeof(buf), "%d. (untitled section)", i + 1);
        lv_list_add_btn(s_menu_list, NULL, buf);
    }
    apply_list_selection_styles(s_menu_list, s_toc_sel);
    s_toc_sel_prev = s_toc_sel;
#if defined(CONFIG_DRAFTLING_TOUCHSCREEN)
    s_toc_touch_ctx.p_sel      = &s_toc_sel;
    s_toc_touch_ctx.p_sel_prev = &s_toc_sel_prev;
    s_toc_touch_ctx.activate   = toc_activate;
    list_touch_attach(s_menu_list, &s_toc_touch_ctx);
#endif
    sync_battery_labels();
}

/* C while reading: the chapters, the current one highlighted. */
static void open_toc_picker(void)
{
    int n = editor_book_chapter_count();
    if (n <= 0) return;
    show_menu();
    s_toc_open = true;
    s_toc_sel = editor_book_chapter();
    if (s_toc_sel < 0) s_toc_sel = 0;
    if (s_toc_sel >= TOC_MAX_ROWS) s_toc_sel = TOC_MAX_ROWS - 1;
    refresh_toc_items();
}

static void close_toc_picker(void)
{
    s_toc_open = false;
    lv_label_set_text(s_lbl_menu_hdr, MENU_HDR_TEXT);
    close_menu();   /* back to the book */
}

static void toc_activate(int idx)
{
    close_toc_picker();
    pane_bind_focus();
    if (idx >= 0 && idx < editor_book_chapter_count() && idx != editor_book_chapter())
        book_show_chapter(idx, false);
}

static void handle_toc_key(const kb_event_t *ev)
{
    int last = editor_book_chapter_count();
    if (last > TOC_MAX_ROWS) last = TOC_MAX_ROWS;
    last--;
    switch (ev->keycode) {
    case KB_KEY_UP:
        if (s_toc_sel > 0) s_toc_sel--;
        break;
    case KB_KEY_DOWN:
        if (s_toc_sel < last) s_toc_sel++;
        break;
    case KB_KEY_PAGEUP:
        s_toc_sel = s_toc_sel > 10 ? s_toc_sel - 10 : 0;
        break;
    case KB_KEY_PAGEDOWN:
        s_toc_sel = s_toc_sel + 10 < last ? s_toc_sel + 10 : last;
        break;
    case KB_KEY_ENTER:
        toc_activate(s_toc_sel);
        return;
    case KB_KEY_ESCAPE:
        close_toc_picker();
        return;
    default:
        return;
    }
    update_list_highlight(s_menu_list, s_toc_sel, s_toc_sel_prev);
    s_toc_sel_prev = s_toc_sel;
}

/* ---- "Format SD card" picker (see fmt_step_t) ---- */
#if defined(CONFIG_DRAFTLING_TOUCHSCREEN)
static list_touch_ctx_t s_fmt_touch_ctx;
#endif
static void fmt_picker_activate(int idx);

static void refresh_fmt_picker_items(void)
{
    char buf[96];
    lv_obj_clean(s_menu_list);
    switch (s_fmt_step) {
    case FMT_STEP_WIFI:
        lv_label_set_text(s_lbl_menu_hdr,
                          "Format " STORAGE_NAME ": keep the WiFi settings?");
        snprintf(buf, sizeof(buf), "Keep WiFi settings (%s)", s_fmt_ssid);
        lv_list_add_btn(s_menu_list, NULL, buf);
        lv_list_add_btn(s_menu_list, NULL, "Erase WiFi settings");
        lv_list_add_btn(s_menu_list, NULL, "Cancel");
        break;
    case FMT_STEP_CONFIRM1:
        lv_label_set_text(s_lbl_menu_hdr,
                          "Format " STORAGE_NAME ": ALL files will be erased");
        lv_list_add_btn(s_menu_list, NULL, "Cancel");
        lv_list_add_btn(s_menu_list, NULL, "Erase all files on the " STORAGE_NAME);
        break;
    case FMT_STEP_CONFIRM2:
        lv_label_set_text(s_lbl_menu_hdr,
                          "Really format the " STORAGE_NAME "? Cannot be undone.");
        lv_list_add_btn(s_menu_list, NULL, "Cancel");
#if defined(CONFIG_DRAFTLING_NETWORK_HOST_MANAGED)
        lv_list_add_btn(s_menu_list, NULL, "Yes, format now");
#else
        lv_list_add_btn(s_menu_list, NULL,
                        s_fmt_keep_wifi
                            ? "Yes, format now (keep WiFi settings)"
                            : "Yes, format now (erase WiFi settings)");
#endif
        break;
    default:
        return;
    }
    apply_list_selection_styles(s_menu_list, s_fmt_sel);
    s_fmt_sel_prev = s_fmt_sel;
#if defined(CONFIG_DRAFTLING_TOUCHSCREEN)
    s_fmt_touch_ctx.p_sel      = &s_fmt_sel;
    s_fmt_touch_ctx.p_sel_prev = &s_fmt_sel_prev;
    s_fmt_touch_ctx.activate   = fmt_picker_activate;
    list_touch_attach(s_menu_list, &s_fmt_touch_ctx);
#endif
    sync_battery_labels();
}

static void update_fmt_picker_highlight(void)
{
    update_list_highlight(s_menu_list, s_fmt_sel, s_fmt_sel_prev);
    s_fmt_sel_prev = s_fmt_sel;
}

static void fmt_set_step(fmt_step_t step)
{
    s_fmt_step = step;
    s_fmt_sel = 0;   /* "Keep WiFi" / "Cancel" */
    refresh_fmt_picker_items();
}

/* Back to the regular F1 menu list without formatting. */
static void close_fmt_picker(void)
{
    s_fmt_step = FMT_STEP_NONE;
    memset(s_fmt_pass, 0, sizeof(s_fmt_pass));
    lv_label_set_text(s_lbl_menu_hdr, MENU_HDR_TEXT);
    refresh_menu_items();
}

/* Why the card cannot be formatted right now, or NULL if it can. */
static const char *fmt_blocked_reason(void)
{
    const char *busy = sd_handed_off_msg();
    if (busy) return busy;
    if (!sd_card_is_mounted()) return "Format " STORAGE_NAME ": no " STORAGE_NAME;
    if (git_sync_get_state() == GIT_SYNC_IN_PROGRESS)
        return "Format " STORAGE_NAME ": wait for Git sync to finish";
    return NULL;
}

typedef struct {
    bool keep_wifi;
    char ssid[33];
    char pass[65];
} fmt_args_t;

/* Formats the card off the LVGL task (it can take several seconds on
 * a large card), then restores or erases the WiFi settings and
 * restarts, so nothing keeps pointing at the deleted files (open
 * documents, the Git repository, git.cfg). */
static void fmt_task(void *arg)
{
    fmt_args_t *a = (fmt_args_t *)arg;
    char text[256];

    esp_err_t ret = sd_card_format();
    if (ret == ESP_OK) {
        const char *wifi_msg;
#if defined(CONFIG_DRAFTLING_NETWORK_HOST_MANAGED)
        if (true) {
            wifi_msg = "";   /* no WiFi settings of our own */
        } else
#endif
        if (a->keep_wifi) {
            wifi_msg = (wifi_manager_save_to_file(a->ssid, a->pass) == ESP_OK)
                ? "The WiFi settings were written to the new wifi.cfg."
                : "Could not write wifi.cfg; the WiFi settings stay in "
                  "the device memory.";
        } else {
            wifi_msg = (wifi_manager_forget() == ESP_OK)
                ? "The WiFi settings were erased."
                : "Could not erase the WiFi settings from the device memory.";
        }
        snprintf(text, sizeof(text),
                 "The " STORAGE_NAME " is formatted.\n%s%s"
                 "The device will restart in a few seconds.", wifi_msg,
                 wifi_msg[0] ? "\n" : "");
    } else {
        snprintf(text, sizeof(text),
                 "Formatting failed: %s.\nRestart the device before "
                 "using the " STORAGE_NAME " again.", esp_err_to_name(ret));
    }
    memset(a, 0, sizeof(*a));
    free(a);

    draftling_lvgl_port_lock(-1);
    /* On success the device restarts on its own: show it as busy, so
     * there is no "press Enter" hint and keys are ignored. */
    show_msg_popup("Format " STORAGE_NAME, text, ret == ESP_OK);
    draftling_lvgl_port_unlock();

    if (ret == ESP_OK) {
        vTaskDelay(pdMS_TO_TICKS(3000));
        esp_restart();
    }
    vTaskDelete(NULL);
}

static void fmt_start(void)
{
    const char *why = fmt_blocked_reason();
    if (why) {
        close_fmt_picker();
        editor_ui_set_status(why);
        return;
    }
    fmt_args_t *a = (fmt_args_t *)calloc(1, sizeof(fmt_args_t));
    if (!a) {
        close_fmt_picker();
        editor_ui_set_status("Format " STORAGE_NAME ": out of memory");
        return;
    }
    a->keep_wifi = s_fmt_keep_wifi;
    snprintf(a->ssid, sizeof(a->ssid), "%s", s_fmt_ssid);
    snprintf(a->pass, sizeof(a->pass), "%s", s_fmt_pass);
    close_fmt_picker();

    /* The files are about to disappear: drop unsaved edits so no
     * auto-save writes them back onto the fresh card, and keep
     * standby from sleeping in the middle of the format. */
    editor_discard_all_changes();
    standby_reset_timer();
#if defined(CONFIG_DRAFTLING_STORAGE_FLASH)
    show_msg_popup("Format " STORAGE_NAME,
                   "Formatting the internal storage...\n"
                   "Do not switch the device off.", true);
#else
    show_msg_popup("Format SD card",
                   "Formatting the SD card...\n"
                   "Do not remove the card or switch the device off.", true);
#endif

    if (xTaskCreatePinnedToCore(fmt_task, "sd_format", 6 * 1024, a, 3,
                                NULL, 0) != pdPASS) {
        memset(a, 0, sizeof(*a));
        free(a);
        show_msg_popup("Format " STORAGE_NAME,
                       "Formatting failed: could not start the task.", false);
    }
}

static void fmt_picker_activate(int idx)
{
    switch (s_fmt_step) {
    case FMT_STEP_WIFI:
        if (idx == 2) { close_fmt_picker(); break; }
        s_fmt_keep_wifi = (idx == 0);
        fmt_set_step(FMT_STEP_CONFIRM1);
        break;
    case FMT_STEP_CONFIRM1:
        if (idx == 1) fmt_set_step(FMT_STEP_CONFIRM2);
        else          close_fmt_picker();
        break;
    case FMT_STEP_CONFIRM2:
        if (idx == 1) fmt_start();
        else          close_fmt_picker();
        break;
    default:
        break;
    }
}

/* Entry point from the F1 menu row. */
static void open_fmt_picker(void)
{
    const char *why = fmt_blocked_reason();
    if (why) {
        editor_ui_set_status(why);
        return;
    }
    s_fmt_keep_wifi = false;
#if defined(CONFIG_DRAFTLING_NETWORK_HOST_MANAGED)
    bool have_wifi = false;
#else
    bool have_wifi = wifi_manager_get_configured(s_fmt_ssid, sizeof(s_fmt_ssid),
                                                 s_fmt_pass, sizeof(s_fmt_pass));
#endif
    fmt_set_step(have_wifi ? FMT_STEP_WIFI : FMT_STEP_CONFIRM1);
}

static void save_modified_doc_cb(editor_doc_t *doc, void *ctx);

/* Ctrl+G and F1 -> Git Sync: sync the project the user works in (see
 * context_project()), after saving every open document so the sync
 * task (which reads the files) picks up the latest edits. */
/* Project that Ctrl+G / F1 -> Sync -> Synchronize will sync once the WiFi
 * connection it started is up ("" = none pending). Resolved by
 * sync_wifi_done() from wifi_state_cb() / wifi_connect_task(). */
static project_name_t s_sync_after_wifi = "";

/* Sync project p (see start_project_sync()); with connect_wifi, a
 * missing WiFi connection is brought up first (from wifi.cfg / NVS, as
 * Ctrl+W does) and the sync starts when it is connected. */
static void sync_project(const char *p, bool connect_wifi)
{
    char dir[64];
    project_dir(p, dir, sizeof(dir));
    char sbuf[128];
    if (!git_sync_is_configured(dir)) {
        snprintf(sbuf, sizeof(sbuf), "Git: %.32s not configured (no git.cfg)", p);
        editor_ui_set_status(sbuf);
        return;
    }
    if (!wifi_manager_network_up()) {
        if (!connect_wifi) {
            editor_ui_set_status("Git: connect WiFi first (F1)");
            return;
        }
        snprintf(s_sync_after_wifi, sizeof(s_sync_after_wifi), "%s", p);
        /* An attempt already running reports to wifi_state_cb() too. */
        if (wifi_manager_get_state() != WIFI_STATE_CONNECTING) {
            editor_ui_set_status("WiFi: connecting...");
            wifi_connect_async();
        }
        return;
    }
    editor_doc_foreach(save_modified_doc_cb, NULL);
    pane_bind_focus();
    if (git_sync_start(dir, GIT_SYNC_BOTH) == ESP_OK) {
        snprintf(sbuf, sizeof(sbuf), "Git: syncing %.32s...", p);
        editor_ui_set_status(sbuf);
    } else {
        const char *err = git_sync_get_last_error();
        snprintf(sbuf, sizeof(sbuf), "Git: %s",
                 (err && err[0]) ? err : "failed to start sync");
        editor_ui_set_status(sbuf);
    }
}

/* Outcome of the WiFi connection a pending Synchronize waits for
 * (LVGL lock held). On failure the WiFi status message stays. */
static void sync_wifi_done(bool connected)
{
    if (!s_sync_after_wifi[0]) return;
    project_name_t p;
    snprintf(p, sizeof(p), "%s", s_sync_after_wifi);
    s_sync_after_wifi[0] = '\0';
    if (!connected) return;
    if (const char *busy = sd_handed_off_msg()) {
        editor_ui_set_status(busy);
        return;
    }
    if (!project_exists(p)) return;
    sync_project(p, false);
}

/* Ctrl+G and F1 -> Sync -> Synchronize: sync context_project(),
 * connecting WiFi first when it is down. */
static void start_project_sync(void)
{
    if (const char *busy = sd_handed_off_msg()) {
        editor_ui_set_status(busy);
        return;
    }
    project_name_t p;
    if (!context_project(p, sizeof(p))) {
        editor_ui_set_status("Git: select a project first");
        return;
    }
    sync_project(p, true);
}

static void menu_activate_item(int row)
{
    if (row < 0 || row >= s_menu_item_count) return;
    switch (s_menu_items[row]) {
    case MENU_ITEM_SETTINGS:
        show_settings();
        break;
    case MENU_ITEM_SYNC_MENU:
        menu_enter_level(MENU_LVL_SYNC);
        break;
    case MENU_ITEM_CONN_MENU:
        menu_enter_level(MENU_LVL_CONN);
        break;
    case MENU_ITEM_SYSTEM_MENU:
        menu_enter_level(MENU_LVL_SYSTEM);
        break;
    case MENU_ITEM_BLE_MENU:
        menu_enter_level(MENU_LVL_BLE);
        break;
    case MENU_ITEM_WIFI_MENU:
        menu_enter_level(MENU_LVL_WIFI);
        break;
    case MENU_ITEM_BACK:
        menu_go_back();
        break;
    case MENU_ITEM_CLOSE:
        close_menu();
        break;
    case MENU_ITEM_BLE_STATUS:  /* no action */
    case MENU_ITEM_WIFI_STATUS:
        break;
    case MENU_ITEM_BLE_SCAN:
        /* BLE may be idle (disabled) when another keyboard owns input
         * -- e.g. a USB or Tab5 keyboard was present at boot. Re-enable
         * it first so an on-demand scan can actually start. No-op when
         * BLE is already active. */
        ble_keyboard_enable();
        ble_keyboard_start_scan();
        editor_ui_set_status("BLE: scanning...");
        close_menu();
        break;
    case MENU_ITEM_WIFI_TOGGLE:
        if (wifi_manager_is_connected()) {
            wifi_manager_disconnect();
            editor_ui_set_status("WiFi: disconnected");
            close_menu();
        } else {
            /* Same fast reconnect-from-wifi.cfg/NVS as Ctrl+W. */
            editor_ui_set_status("WiFi: connecting...");
            close_menu();
            wifi_connect_async();
        }
        break;
    case MENU_ITEM_WIFI_NEW:
        /* Opens the scan/pick picker in s_menu_list -- see the "WiFi
         * scan/connect picker" block above. Also usable while
         * connected: picking a network switches to it. */
        s_wifi_scan_picker_open = true;
        start_wifi_scan();
        break;
    case MENU_ITEM_GIT_SYNC:
        /* Git sync reads and writes the SD card directly (its own
         * .git/ history plus the working tree), same as any other
         * file I/O -- refuse it while the card belongs to a USB host
         * or the Kermit server, same as handle_browser_key()'s guard.
         * This item is only reachable via the F1 menu, which stays
         * reachable so the user can switch those back off; the menu
         * is not itself gated the way the file browser is. */
        if (const char *busy = sd_handed_off_msg()) {
            editor_ui_set_status(busy);
            break;
        }
        /* Close the menu when the sync (or the WiFi connection it
         * needs first) can start; otherwise stay with the reason in
         * the status bar. */
        if (git_sync_get_state() != GIT_SYNC_IN_PROGRESS) {
            project_name_t p;
            char dir[64];
            if (context_project(p, sizeof(p))) {
                project_dir(p, dir, sizeof(dir));
                if (git_sync_is_configured(dir)) close_menu();
            }
        }
        start_project_sync();
        break;
    case MENU_ITEM_DRAFTBOX:
        /* Writes git.cfg on the SD card -- same guard as Git Sync. */
        if (const char *busy = sd_handed_off_msg()) {
            editor_ui_set_status(busy);
            break;
        }
        if (wifi_manager_network_up()) {
            show_dbx_prompt(false);
        } else {
            /* Connect first, the same way as Ctrl+W, unless an attempt
             * is already running -- then just wait for its outcome. */
            show_dbx_prompt(true);
            if (wifi_manager_get_state() != WIFI_STATE_CONNECTING)
                wifi_connect_async();
        }
        break;
    case MENU_ITEM_KB_LAYOUT:
        kb_layout_next();
        save_current_layout_to_nvs();
        refresh_menu_items();
        break;
#if defined(CONFIG_DRAFTLING_HAS_USB_MSC)
    case MENU_ITEM_USB_MSC:
        if (usbmsc_item_disabled()) {
#if defined(CONFIG_DRAFTLING_STORAGE_FLASH)
            editor_ui_set_status("SD card via USB: not with internal storage");
#else
            editor_ui_set_status("SD card via USB: insert an SD card first");
#endif
            break;
        }
        /* Open the picker sub-list. Up/Down to navigate, Enter to
         * stage a choice, Esc to return without changing it -- either
         * way, leaving the picker applies it (see
         * close_usbmsc_picker() / commit_usb_msc_pending_mode()). */
        s_usbmsc_picker_open = true;
        s_usbmsc_picker_sel = (s_usbmsc_pending_mode == USB_MSC_MODE_READ_ONLY) ? 1
                            : (s_usbmsc_pending_mode == USB_MSC_MODE_READ_WRITE) ? 2
                            : 0;
        refresh_usbmsc_picker_items();
        break;
#endif
#if defined(CONFIG_DRAFTLING_HAS_KERMIT)
    case MENU_ITEM_KERMIT:
        s_kermit_picker_open = true;
        s_kermit_picker_sel = 0;
        refresh_kermit_picker_items();
        break;
#endif
    case MENU_ITEM_FORMAT_SD:
        open_fmt_picker();
        break;
    case MENU_ITEM_BOOKS:
        open_books_picker();
        break;
    case MENU_ITEM_HWINFO:
        s_menu_open = false;
        show_hwinfo(s_editor_screen_active ? HELP_FROM_EDITOR : HELP_FROM_BROWSER);
        break;
    case MENU_ITEM_HELP:
        /* The help describes the screen the menu was opened from. */
        s_menu_open = false;
        show_help(s_editor_screen_active ? HELP_FROM_EDITOR : HELP_FROM_BROWSER);
        break;
    case MENU_ITEM_SLEEP:
        /* Leave the menu first so, if we prompt about unsaved changes,
         * the dialog is drawn on the editor screen it lives on. With no
         * unsaved changes (or if the menu was opened from the file
         * browser) go straight to deep sleep. */
        close_menu();
        if (s_editor_screen_active && editor_is_modified()) {
            show_sleep_prompt();
        } else {
            standby_enter_sleep();
        }
        break;
#if defined(CONFIG_DRAFTLING_HAS_QUIT)
    case MENU_ITEM_QUIT:
        /* As "Sleep now": ask about unsaved changes in the editor;
         * otherwise quit (the pre-sleep hook saves every titled
         * document). */
        close_menu();
        if (s_editor_screen_active && editor_is_modified()) {
            show_leave_prompt(EXIT_PURPOSE_QUIT);
        } else {
            standby_quit();
        }
        break;
#endif
    default:
        break;
    }
}

static void handle_xfer_picker_key(const kb_event_t *ev);

static void handle_menu_key(const kb_event_t *ev)
{
    if (s_xfer_open) {
        handle_xfer_picker_key(ev);
        return;
    }
    if (s_wifi_scan_picker_open) {
        /* Up/Down/Enter/Esc only -- ignore keys while a scan is still
         * in flight (s_wifi_scan_in_progress) so Enter can't activate
         * the placeholder "Scanning..." row. */
        int last_row = wifi_scan_rescan_row();
        switch (ev->keycode) {
        case KB_KEY_UP:
            if (!s_wifi_scan_in_progress && s_wifi_scan_picker_sel > 0)
                s_wifi_scan_picker_sel--;
            update_wifi_scan_picker_highlight();
            break;
        case KB_KEY_DOWN:
            if (!s_wifi_scan_in_progress && s_wifi_scan_picker_sel < last_row)
                s_wifi_scan_picker_sel++;
            update_wifi_scan_picker_highlight();
            break;
        case KB_KEY_ENTER:
            if (!s_wifi_scan_in_progress)
                wifi_scan_picker_activate(s_wifi_scan_picker_sel);
            break;
        case KB_KEY_ESCAPE:
            close_wifi_scan_picker();
            break;
        default:
            break;
        }
        return;
    }

    if (s_books_open) { handle_books_key(ev); return; }
    if (s_toc_open) { handle_toc_key(ev); return; }

    if (s_fmt_step != FMT_STEP_NONE) {
        int last_row = (s_fmt_step == FMT_STEP_WIFI) ? 2 : 1;
        switch (ev->keycode) {
        case KB_KEY_UP:
            if (s_fmt_sel > 0) s_fmt_sel--;
            update_fmt_picker_highlight();
            break;
        case KB_KEY_DOWN:
            if (s_fmt_sel < last_row) s_fmt_sel++;
            update_fmt_picker_highlight();
            break;
        case KB_KEY_ENTER:
            fmt_picker_activate(s_fmt_sel);
            break;
        case KB_KEY_ESCAPE:
            close_fmt_picker();
            break;
        default:
            break;
        }
        return;
    }

#if defined(CONFIG_DRAFTLING_HAS_USB_MSC)
    if (s_usbmsc_picker_open) {
        switch (ev->keycode) {
        case KB_KEY_UP:
            if (s_usbmsc_picker_sel > 0) s_usbmsc_picker_sel--;
            update_usbmsc_picker_highlight();
            break;
        case KB_KEY_DOWN:
            if (s_usbmsc_picker_sel < 2) s_usbmsc_picker_sel++;
            update_usbmsc_picker_highlight();
            break;
        case KB_KEY_ENTER:
            s_usbmsc_pending_mode = (s_usbmsc_picker_sel == 1) ? USB_MSC_MODE_READ_ONLY
                                   : (s_usbmsc_picker_sel == 2) ? USB_MSC_MODE_READ_WRITE
                                   : USB_MSC_MODE_OFF;
            close_usbmsc_picker();
            break;
        case KB_KEY_ESCAPE:
            close_usbmsc_picker();
            break;
        default:
            break;
        }
        return;
    }
#endif

#if defined(CONFIG_DRAFTLING_HAS_KERMIT)
    if (s_kermit_picker_open) {
        switch (ev->keycode) {
        case KB_KEY_UP:
            if (s_kermit_picker_sel > 0) s_kermit_picker_sel--;
            update_kermit_picker_highlight();
            break;
        case KB_KEY_DOWN:
            if (s_kermit_picker_sel < 1) s_kermit_picker_sel++;
            update_kermit_picker_highlight();
            break;
        case KB_KEY_ENTER:
            kermit_picker_activate(s_kermit_picker_sel);
            break;
        case KB_KEY_ESCAPE:
            close_kermit_picker();
            break;
        default:
            break;
        }
        return;
    }
#endif

    switch (ev->keycode) {
    case KB_KEY_UP:
        if (s_menu_sel > 0) s_menu_sel--;
        update_menu_highlight();
        break;
    case KB_KEY_DOWN:
        if (s_menu_sel < s_menu_item_count - 1) s_menu_sel++;
        update_menu_highlight();
        break;
    case KB_KEY_ENTER:
        menu_activate_item(s_menu_sel);
        break;
    case KB_KEY_ESCAPE:
        menu_go_back();
        break;
    case KB_KEY_F1:
        close_menu();
        break;
    default:
        /* Letter accelerators of the current level (drawn bold). K
         * only moves the highlight to the keyboard row, where Enter
         * cycles the layout; the others activate their row. */
        if (ev->modifier & (KB_MOD_LCTRL | KB_MOD_RCTRL | KB_MOD_LALT |
                            KB_MOD_RALT | KB_MOD_LGUI | KB_MOD_RGUI))
            break;
        {
            char ch = kb_layout_shortcut_char(ev->keycode);
            if (!ch) break;
            for (int i = 0; i < s_menu_item_count; i++) {
                if (menu_item_hotkey(s_menu_items[i]) != ch) continue;
                s_menu_sel = i;
                update_menu_highlight();
                if (s_menu_items[i] != MENU_ITEM_KB_LAYOUT) menu_activate_item(i);
                break;
            }
        }
        break;
    }
}

/* ---- Help screen ---- */

/* Append one "key  description" row to the help text. The description
 * is word-wrapped by hand to the text width (cols characters) with a
 * hanging indent at keycol, so it stays in its column; a key too long
 * for the key column gets a line of its own. */
static void help_row(std::string &out, int cols, int keycol,
                     const char *key, const char *desc)
{
#if defined(CONFIG_DRAFTLING_WEB_PAGE)
    key = mac_key_names(key);   /* before the columns are measured */
    desc = mac_key_names(desc);
#endif
    int klen = (int)strlen(key);
    out += key;
    if (klen >= keycol) {
        out += '\n';
        out.append(keycol, ' ');
    } else {
        out.append(keycol - klen, ' ');
    }
    int dw = cols - keycol;
    if (dw < 8) dw = 8;
    int col = 0;
    const char *p = desc;
    while (*p) {
        const char *w = p;
        while (*w && *w != ' ') w++;
        int wl = (int)(w - p);
        if (col > 0 && col + 1 + wl > dw) {
            out += '\n';
            out.append(keycol, ' ');
            col = 0;
        } else if (col > 0) {
            out += ' ';
            col++;
        }
        out.append(p, wl);
        col += wl;
        while (*w == ' ') w++;
        p = w;
    }
    out += '\n';
}

static void help_heading(std::string &out, const char *title)
{
    if (!out.empty()) out += '\n';
    out += title;
    out += '\n';
    out.append(strlen(title), '-');
    out += '\n';
}

/* Shortcuts that work both in the editor and in the file browser. */
static void help_common_rows(std::string &out, int cols, int kc)
{
    help_row(out, cols, kc, "F1, Ctrl+M", "Menu");
    help_row(out, cols, kc, "F10", "This help");
    help_row(out, cols, kc, "Ctrl+1", "Single pane");
    help_row(out, cols, kc, "Ctrl+2", "Split into two equal panes");
    help_row(out, cols, kc, "Ctrl+3", "Split with a wide first pane; again to make it narrow");
    help_row(out, cols, kc, "Ctrl+G", "Git sync of the current project");
#if !defined(CONFIG_DRAFTLING_NETWORK_HOST_MANAGED)
    help_row(out, cols, kc, "Ctrl+W", "WiFi connect / disconnect");
#endif
    help_row(out, cols, kc, "Ctrl+P", "Sleep");
#if defined(CONFIG_DRAFTLING_DISPLAY_HAS_BACKLIGHT) && !defined(CONFIG_DRAFTLING_DISPLAY_BACKLIGHT_BINARY)
#if defined(CONFIG_DRAFTLING_WEB_PAGE)
    help_row(out, cols, kc, "Ctrl+B", "Brightness");
#else
    help_row(out, cols, kc, "Ctrl+B", "Backlight brightness");
#endif
#endif
#if defined(CONFIG_DRAFTLING_DISPLAY_EPD)
    help_row(out, cols, kc, "Ctrl+R", "Full screen refresh (clears ghosting)");
#endif
    help_row(out, cols, kc, "Ctrl+Q", "Same as Esc, for keyboards without one");
    help_row(out, cols, kc, "Ctrl+T", "Writing sprint: timer (15, 25, 50 min) or word goal, shown as a line on the right edge; optional word count in the top bar");
}

static void build_help_text(std::string &out, help_origin_t origin, int cols)
{
    int kc = cols / 3;
    if (kc < 11) kc = 11;
    if (kc > 16) kc = 16;

    pane_bind_focus();
    if (origin == HELP_FROM_EDITOR && editor_is_read_only()) {
        help_heading(out, "Reading a book");
        help_row(out, cols, kc, "PgDn, Space", "Next page; at the end of a chapter the next chapter");
        help_row(out, cols, kc, "PgUp, Shift+Space", "Previous page; at the start of a chapter the end of the previous one");
        help_row(out, cols, kc, "N, Ctrl+PgDn", "Next chapter");
        help_row(out, cols, kc, "P, Ctrl+PgUp", "Previous chapter");
        help_row(out, cols, kc, "C", "Contents: pick a chapter");
        help_row(out, cols, kc, "Ctrl+F", "Find in the chapter");
        help_row(out, cols, kc, "Shift+arrows, Ctrl+C", "Select and copy text");
        help_row(out, cols, kc, "Alt+I", "Book information");
        help_row(out, cols, kc, "Esc", "Close the book (the position is kept)");
        help_common_rows(out, cols, kc);
    } else if (origin == HELP_FROM_EDITOR) {
        help_heading(out, "Editor");
        help_row(out, cols, kc, "Esc", "Back to the file browser (asks to save changes)");
        help_row(out, cols, kc, "Ctrl+S", "Save / save as");
        help_row(out, cols, kc, "Ctrl+O", "Open another file");
        help_row(out, cols, kc, "Ctrl+N", "New file");
        help_row(out, cols, kc, "Ctrl+F", "Find");
        help_row(out, cols, kc, "Ctrl+H", "Find and replace (Tab: switch field, Ctrl+Enter: replace)");
        help_row(out, cols, kc, "Shift+arrows", "Select text");
        help_row(out, cols, kc, "Ctrl+A", "Select all");
        help_row(out, cols, kc, "Ctrl+C / X / V", "Copy / cut / paste");
        help_row(out, cols, kc, "Ctrl+Left/Right", "Previous / next word");
        help_row(out, cols, kc, "Home / End", "Start / end of line");
        help_row(out, cols, kc, "Ctrl+Home/End", "Start / end of document");
        help_row(out, cols, kc, "PgUp / PgDn", "Page up / down");
        help_row(out, cols, kc, "Ctrl+Up/Down", "Page up / down, same as PgUp / PgDn");
        help_row(out, cols, kc, "Ctrl+Tab", "Switch pane (when split)");
        help_row(out, cols, kc, "Ctrl+L", "Next keyboard layout (also Win+Space)");
        help_row(out, cols, kc, "Alt+I", "File and project information: words, size, Git status");
        help_common_rows(out, cols, kc);

        pane_bind_focus();
        const editor_format_t *fmt = editor_format_active();
        help_heading(out, fmt->help_title);
        for (const fmt_help_row_t *r = fmt->help; r->syntax; r++)
            help_row(out, cols, kc, r->syntax, r->meaning);
    } else {
        help_heading(out, "Projects");
        help_row(out, cols, kc, "Up / Down", "Select a project");
        help_row(out, cols, kc, "Enter", "Open the project");
        help_row(out, cols, kc, "N, Ctrl+N", "New project");
        help_row(out, cols, kc, "F2, Alt+R", "Rename the project");
        help_row(out, cols, kc, "Del, Alt+D", "Delete the project (only when synchronized with Git)");
        help_row(out, cols, kc, "Alt+I", "Project information: files, size, Git status");
        help_row(out, cols, kc, "Ctrl+G", "Git sync of the selected project");
        help_row(out, cols, kc, "B", "Open a book (.epub) from any folder");
        if (origin == HELP_FROM_INPANE)
            help_row(out, cols, kc, "Ctrl+Tab", "Back to the editor, in the other pane");

        help_heading(out, "Files of a project");
        help_row(out, cols, kc, "Up / Down", "Select a file");
        help_row(out, cols, kc, "Enter", "Open the file");
        help_row(out, cols, kc, "Esc, Backspace", "Back to the list of projects");
        help_row(out, cols, kc, "N, Ctrl+N", "New file: Markdown, Fountain or plain text");
        help_row(out, cols, kc, "F2, Alt+R", "Rename the file");
        help_row(out, cols, kc, "Del, Alt+D", "Delete the file (only once it is pushed with Git sync)");
        help_row(out, cols, kc, "Alt+M", "Move the file to another project");
        help_row(out, cols, kc, "Alt+C", "Copy the file to a project (also this one, under a new name)");
        help_row(out, cols, kc, "Alt+I", "File and project information: words, size, Git status");
        help_row(out, cols, kc, "Ctrl+G", "Git sync of this project");
        help_row(out, cols, kc, "B", "Open a book (.epub) from any folder");
        if (origin == HELP_FROM_INPANE)
            help_row(out, cols, kc, "Ctrl+Tab", "Switch to the other pane");
        help_common_rows(out, cols, kc);
    }

#if defined(CONFIG_DRAFTLING_WEB_PAGE)
    /* Keys the web client's page handles itself (web_client/web/client.js). */
    help_heading(out, "Web browser");
    help_row(out, cols, kc, "F9", "Files: download a document, a project or everything (ZIP); upload documents into a project. Saves the open documents first");
    help_row(out, cols, kc, "F11", "Full screen; in Chrome and Edge, hold Esc to leave it");
    help_row(out, cols, kc, "Ctrl+C / X / V", "Copy / cut / paste with the system clipboard");
    if (s_mac_key_names) {
        help_row(out, cols, kc, "Cmd, Opt", "Command and Option, for Ctrl and Alt; Control works as Ctrl too");
        help_row(out, cols, kc, "Ctrl+Q/W/M/H/N/T", "macOS and the browser keep these with Command: press Control");
    } else {
        help_row(out, cols, kc, "Command", "Works as Ctrl on a Mac");
        help_row(out, cols, kc, "Ctrl+W/N/T/Tab", "Kept by the browser outside full screen: use full screen (or N in the file browser for a new file)");
    }
    help_row(out, cols, kc, "Storage", "Documents stay in this browser; Git sync or F9 keeps copies elsewhere");
#endif
}

/* Text width of the help page in characters: the body width minus its
 * 4 px side padding and room for the scrollbar, over the monospace
 * cell. */
static int help_cols(void)
{
    int cols = (SCR_W - 16) / char_width_for_font(FONT_14);
    return cols < 20 ? 20 : cols;
}

/* Decimal size ("31.9 GB", "512 MB"), as SD cards are labelled. */
static void format_size(char *buf, size_t len, uint64_t bytes)
{
    uint64_t mb = bytes / 1000000ULL;
    if (mb >= 1000) {
        uint64_t tenths = (bytes + 50000000ULL) / 100000000ULL;
        snprintf(buf, len, "%u.%u GB", (unsigned)(tenths / 10),
                 (unsigned)(tenths % 10));
    } else {
        snprintf(buf, len, "%u MB", (unsigned)mb);
    }
}

/* Binary size ("512 KB", "8 MB", "1.5 MB"), as memory is measured. */
static void format_mem(char *buf, size_t len, uint64_t bytes)
{
    if (bytes >= (1ULL << 20)) {
        unsigned tenths = (unsigned)((bytes * 10 + (1ULL << 19)) >> 20);
        if (tenths % 10) snprintf(buf, len, "%u.%u MB", tenths / 10, tenths % 10);
        else             snprintf(buf, len, "%u MB", tenths / 10);
    } else {
        snprintf(buf, len, "%u KB", (unsigned)(bytes >> 10));
    }
}

/* "<free> free of <total>" row for a heap with the given caps. */
static void hwinfo_heap_row(std::string &out, int cols, int kc,
                            const char *label, uint32_t caps)
{
    char tot[16], fr[16], buf[48];
    format_mem(tot, sizeof(tot), heap_caps_get_total_size(caps));
    format_mem(fr, sizeof(fr), heap_caps_get_free_size(caps));
    snprintf(buf, sizeof(buf), "%s free of %s", fr, tot);
    help_row(out, cols, kc, label, buf);
}

static void format_mac(char *buf, size_t len, const uint8_t *m)
{
    snprintf(buf, len, "%02X:%02X:%02X:%02X:%02X:%02X",
             m[0], m[1], m[2], m[3], m[4], m[5]);
}

static void hwinfo_mac_row(std::string &out, int cols, int kc,
                           const char *label, esp_mac_type_t type)
{
    uint8_t m[6];
    if (esp_read_mac(m, type) != ESP_OK) return;
    char buf[24];
    format_mac(buf, sizeof(buf), m);
    help_row(out, cols, kc, label, buf);
}

static void build_hwinfo_text(std::string &out, int cols)
{
    const int kc = 15;
    char buf[96];

    help_heading(out, "Board");
    help_row(out, cols, kc, "Model", s_board_name ? s_board_name : "unknown");
    esp_chip_info_t chip;
    esp_chip_info(&chip);
    snprintf(buf, sizeof(buf), "%s rev %d.%d, %d core%s", CONFIG_IDF_TARGET,
             chip.revision / 100, chip.revision % 100, chip.cores,
             chip.cores == 1 ? "" : "s");
    help_row(out, cols, kc, "Chip", buf);

    if (s_coproc_chip) {
        help_heading(out, "Radio co-processor");
        help_row(out, cols, kc, "Chip", s_coproc_chip);
        help_row(out, cols, kc, "Firmware", s_coproc_fw ? s_coproc_fw : "unknown");
    }

    help_heading(out, "Memory");
    hwinfo_heap_row(out, cols, kc, "RAM", MALLOC_CAP_INTERNAL);
    size_t psram = esp_psram_is_initialized() ? esp_psram_get_size() : 0;
    if (psram) {
        format_mem(buf, sizeof(buf), psram);
        help_row(out, cols, kc, "PSRAM", buf);
        hwinfo_heap_row(out, cols, kc, "PSRAM heap", MALLOC_CAP_SPIRAM);
    } else {
        help_row(out, cols, kc, "PSRAM", "none");
    }
    /* The editor's gap buffer, sized by editor_init() from the PSRAM
     * free at boot (with flash storage also capped by the volume size):
     * the largest document that can be opened. */
    format_mem(buf, sizeof(buf), editor_get_max_doc_size());
    help_row(out, cols, kc, "Max file size", buf);
    uint32_t flash = 0;
    if (esp_flash_get_physical_size(esp_flash_default_chip, &flash) == ESP_OK ||
        esp_flash_get_size(esp_flash_default_chip, &flash) == ESP_OK) {
        format_mem(buf, sizeof(buf), flash);
        help_row(out, cols, kc, "Flash", buf);
    }

    help_heading(out, "Screen");
    help_row(out, cols, kc, "Controller", display_controller_name());
#if defined(CONFIG_DRAFTLING_TOUCHSCREEN)
#if defined(CONFIG_DRAFTLING_DISPLAY_MIPI_DSI)
    const char *touch = "GT911 / ST7123";
#elif defined(CONFIG_DRAFTLING_TOUCH_GT911)
    const char *touch = "GT911";
#elif defined(CONFIG_DRAFTLING_TOUCH_FT6336U)
    const char *touch = "FT6336-compatible";
#elif defined(CONFIG_DRAFTLING_TOUCH_AXS5106L)
    const char *touch = "AXS5106L";
#elif defined(CONFIG_DRAFTLING_TOUCH_ST77922)
    const char *touch = "ST77922 (built-in)";
#else
    const char *touch = "yes";
#endif
    help_row(out, cols, kc, "Touch", touch);
#else
    help_row(out, cols, kc, "Touch", "none");
#endif
#if defined(CONFIG_DRAFTLING_DISPLAY_RESIZABLE)
    snprintf(buf, sizeof(buf), "%d x %d", (int)SCR_W * display_scale_factor(),
             (int)SCR_H * display_scale_factor());
#else
    snprintf(buf, sizeof(buf), "%d x %d", CONFIG_DRAFTLING_DISPLAY_WIDTH,
             CONFIG_DRAFTLING_DISPLAY_HEIGHT);
#endif
    help_row(out, cols, kc, "Resolution", buf);
    snprintf(buf, sizeof(buf), "%d x %d%s", (int)SCR_W, (int)SCR_H,
             display_scale_factor() > 1 ? " (2x scale)" : "");
    help_row(out, cols, kc, "Text area", buf);
#ifdef CONFIG_DRAFTLING_DISPLAY_HIDPI
    help_row(out, cols, kc, "Fonts", "high definition (Hack)");
#else
    help_row(out, cols, kc, "Fonts", "low definition (Greybeard)");
#endif

#if defined(CONFIG_DRAFTLING_STORAGE_FLASH)
    help_heading(out, "Storage");
    if (!sd_card_is_internal_flash()) {
        help_row(out, cols, kc, "Status", "not mounted");
    } else {
        help_row(out, cols, kc, "Type", "internal flash (FAT, wear levelling)");
        format_size(buf, sizeof(buf), sd_card_flash_partition_size());
        help_row(out, cols, kc, "Partition", buf);
        uint64_t fs_total = 0, fs_free = 0;
        if (sd_card_get_fs_space(&fs_total, &fs_free) == ESP_OK) {
            char tot[24], fr[24];
            format_size(tot, sizeof(tot), fs_total);
            format_size(fr, sizeof(fr), fs_free);
            snprintf(buf, sizeof(buf), "%s of %s", fr, tot);
            help_row(out, cols, kc, "Free", buf);
        }
    }
#else
    help_heading(out, "SD card");
    sdmmc_card_t *card = sd_card_get_handle();
    if (!card) {
        help_row(out, cols, kc, "Status", "not inserted");
    } else {
        uint64_t cap = (uint64_t)card->csd.capacity * card->csd.sector_size;
        const char *type;
        if (card->is_mmc)                        type = "MMC";
        else if (card->is_sdio)                  type = "SDIO";
        else if (!(card->ocr & SD_OCR_SDHC_CAP)) type = "SDSC";
        else if (cap > (32ULL << 30))            type = "SDXC";
        else                                     type = "SDHC";
        char name[sizeof(card->cid.name) + 1];
        memcpy(name, card->cid.name, sizeof(card->cid.name));
        name[sizeof(card->cid.name)] = '\0';
        if (name[0]) snprintf(buf, sizeof(buf), "%s (%s)", type, name);
        else         snprintf(buf, sizeof(buf), "%s", type);
        help_row(out, cols, kc, "Type", buf);
        format_size(buf, sizeof(buf), cap);
        help_row(out, cols, kc, "Size", buf);
        uint64_t fs_total = 0, fs_free = 0;
        if (sd_card_get_fs_space(&fs_total, &fs_free) == ESP_OK) {
            char tot[24], fr[24];
            format_size(tot, sizeof(tot), fs_total);
            format_size(fr, sizeof(fr), fs_free);
            snprintf(buf, sizeof(buf), "%s of %s", fr, tot);
            help_row(out, cols, kc, "Free", buf);
        }
#if defined(CONFIG_DRAFTLING_SD_SDMMC)
        const char *bus = "SDMMC 1-bit";
#else
        const char *bus = "SPI";
#endif
        /* No bus clock: not a card (the Linux application's data
         * folder), so no interface row. */
        if (card->real_freq_khz > 0) {
            snprintf(buf, sizeof(buf), "%s, %d MHz", bus, card->real_freq_khz / 1000);
            help_row(out, cols, kc, "Interface", buf);
        }
    }
#endif

    help_heading(out, "Keyboard");
    bool any_kbd = false;
    if (ble_keyboard_is_connected()) {
        const char *n = ble_keyboard_get_device_name();
        snprintf(buf, sizeof(buf), "%s", (n && n[0]) ? n : "connected");
        help_row(out, cols, kc, "Bluetooth", buf);
        uint8_t kb_mac[6];
        if (ble_keyboard_get_device_addr(kb_mac)) {
            format_mac(buf, sizeof(buf), kb_mac);
            help_row(out, cols, kc, "Keyboard MAC", buf);
        }
        /* -1 when the keyboard has no Battery Service (or has not
         * reported yet) -- the row is left out then. */
        int kb_batt = ble_keyboard_get_battery_level();
        if (kb_batt >= 0 && kb_batt <= 100) {
            snprintf(buf, sizeof(buf), "%d%%", kb_batt);
            help_row(out, cols, kc, "Battery", buf);
        }
        any_kbd = true;
    }
#if defined(CONFIG_DRAFTLING_HAS_USB_HOST)
    if (usb_kbd_is_connected()) {
        help_row(out, cols, kc, "USB", "HID keyboard");
        any_kbd = true;
    }
#endif
#if defined(CONFIG_DRAFTLING_HAS_TAB5_KBD)
    if (tab5_kbd_is_present()) {
        help_row(out, cols, kc, "Built-in", "Tab5 keyboard");
        any_kbd = true;
    }
#endif
    if (!any_kbd) help_row(out, cols, kc, "Status", "none connected");

    help_heading(out, "USB device");
#if defined(CONFIG_DRAFTLING_HAS_USB_MSC)
    help_row(out, cols, kc, "SD via USB", usb_msc_mode_name(usb_msc_get_mode()));
#else
    help_row(out, cols, kc, "SD via USB", "not available");
#endif
#if defined(CONFIG_DRAFTLING_HAS_KERMIT)
    help_row(out, cols, kc, "Browser sync",
             kermit_server_is_running() ? "Running" : "Stopped");
#else
    help_row(out, cols, kc, "Browser sync", "not available");
#endif

    help_heading(out, "MAC address");
#if SOC_WIFI_SUPPORTED || SOC_BT_SUPPORTED
#if SOC_WIFI_SUPPORTED
    hwinfo_mac_row(out, cols, kc, "WiFi", ESP_MAC_WIFI_STA);
#endif
#if SOC_BT_SUPPORTED
    hwinfo_mac_row(out, cols, kc, "Bluetooth", ESP_MAC_BT);
#endif
#else
    hwinfo_mac_row(out, cols, kc, "Base", ESP_MAC_BASE);
#endif
}

/* Show s_scr_help with the given header and body text. */
static void show_help_page(help_origin_t origin, const char *hdr,
                           const std::string &text)
{
    if (origin == HELP_FROM_INPANE) close_inpane_browser();
    s_help_origin = origin;
    s_help_open = true;

    lv_label_set_text(s_lbl_help_hdr, hdr);
    lv_label_set_text(s_help_lbl, text.c_str());
    lv_obj_scroll_to_y(s_help_cont, 0, LV_ANIM_OFF);

#if defined(CONFIG_DRAFTLING_DISPLAY_EPD)
    /* Same missing-repaint issue as show_menu(). */
    display_clear(0xFF);
    lv_obj_invalidate(s_scr_help);
#endif
    lv_scr_load(s_scr_help);
}

static void show_help(help_origin_t origin)
{
    if (!s_scr_help) return;
    std::string text;
    build_help_text(text, origin, help_cols());
    show_help_page(origin,
                   "Help - Up/Down/PgUp/PgDn to scroll, Esc or Enter to close",
                   text);
}

static void show_hwinfo(help_origin_t origin)
{
    if (!s_scr_help) return;
    std::string text;
    build_hwinfo_text(text, help_cols());
    show_help_page(origin,
                   "Hardware information - Esc or Enter to close", text);
}

/* ---- File information (Alt+I) ----
 * Shown on the help screen: counts for the editor's focused document
 * (its buffer, unsaved changes included) or for the file selected in
 * the file browser (read from the SD card), plus its Git sync status
 * against the last copy known to be on the server. */

/* A word is a run of non-space characters with at least one letter or
 * digit (any non-ASCII byte counts as a letter), so Markdown markers
 * like "#", "-" or "---" are not counted. A paragraph is a block of
 * non-blank lines between blank lines that holds at least one word. */
/* Word / paragraph / line / character counts, fed piece by piece
 * (a file in the browser is read in chunks, never whole). */
typedef struct {
    unsigned words, paras, lines, chars;
    size_t   size;
    bool     in_word, word_ok, line_blank, para_counted;
    char     last;
} text_counts_t;

static void counts_init(text_counts_t *c)
{
    memset(c, 0, sizeof(*c));
    c->line_blank = true;
}

static void counts_byte(text_counts_t *c, unsigned char ch)
{
    bool space = ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' ||
                 ch == '\f' || ch == '\v';
    if (space) {
        if (c->in_word && c->word_ok) {
            c->words++;
            if (!c->para_counted) { c->paras++; c->para_counted = true; }
        }
        c->in_word = c->word_ok = false;
    } else {
        c->in_word = true;
        if (ch >= 0x80 || isalnum(ch)) c->word_ok = true;
        c->line_blank = false;
    }
    if (ch == '\n') {
        if (c->line_blank) c->para_counted = false;
        c->line_blank = true;
    }
}

static void counts_feed(text_counts_t *c, const char *t, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        unsigned char ch = (unsigned char)t[i];
        if ((ch & 0xC0) != 0x80) c->chars++;
        if (ch == '\n') c->lines++;
        counts_byte(c, ch);
    }
    if (len) c->last = t[len - 1];
    c->size += len;
}

/* A word needs a letter or digit; a paragraph is a blank-line-separated
 * block with a word; the last line counts even without a newline. */
static void counts_finish(text_counts_t *c)
{
    counts_byte(c, '\n');                  /* ends the last word */
    if (c->size && c->last != '\n') c->lines++;
}

/* project is the document's project and name its bare file name, or
 * NULL when the document is not saved in a project; data/len is the
 * content to compare. */
static void fileinfo_git_row(std::string &out, int cols, int kc, const char *project,
                             const char *name, const char *data, size_t len)
{
    char buf[96];
    const char *msg;
    if (!project || !name) {
        help_row(out, cols, kc, "Status", "not saved in a project");
        return;
    }
    char dir[64];
    project_dir(project, dir, sizeof(dir));
    int added = -1, deleted = -1;
    switch (git_sync_file_diff(dir, name, data, len, &added, &deleted)) {
    case GIT_SYNC_FILE_PUSHED:         msg = "same as on the server"; break;
    case GIT_SYNC_FILE_NOT_CONFIGURED: msg = "not configured"; break;
    case GIT_SYNC_FILE_BUSY:           msg = "sync in progress"; break;
    case GIT_SYNC_FILE_NOT_PUSHED:     msg = "not on the server yet"; break;
    case GIT_SYNC_FILE_MODIFIED:       msg = "changed since last sync"; break;
    default:                           msg = "cannot read the repository"; break;
    }
    help_row(out, cols, kc, "Status", msg);
    if (added >= 0 && deleted >= 0) {
        snprintf(buf, sizeof(buf), "%d added, %d deleted", added, deleted);
        help_row(out, cols, kc, "Lines", buf);
    }
}

static void format_bytes(char *buf, size_t n, uint64_t bytes)
{
    if (bytes < 10000) snprintf(buf, n, "%u bytes", (unsigned)bytes);
    else if (bytes < 10000000ULL) snprintf(buf, n, "%u KB", (unsigned)((bytes + 512) / 1024));
    else snprintf(buf, n, "%u MB", (unsigned)(bytes / (1024 * 1024)));
}

/* Project name, number of documents, total size and Git status. */
static void build_projectinfo_text(std::string &out, int cols, const char *name)
{
    const int kc = 15;
    char buf[96], dir[64];
    project_dir(name, dir, sizeof(dir));
    int files = 0;
    uint64_t bytes = 0;
    project_stats(name, &files, &bytes);

    help_heading(out, "Project");
    help_row(out, cols, kc, "Name", name);
    snprintf(buf, sizeof(buf), "%d", files);
    help_row(out, cols, kc, "Files", buf);
    format_bytes(buf, sizeof(buf), bytes);
    help_row(out, cols, kc, "Total size", buf);

    help_heading(out, "Project sync");
    char host[96], repo[160], branch[64];
    if (!git_sync_get_remote(dir, host, sizeof(host), repo, sizeof(repo),
                             branch, sizeof(branch))) {
        help_row(out, cols, kc, "Status", "not configured (no git.cfg in the project)");
        return;
    }
    help_row(out, cols, kc, "Server", host);
    help_row(out, cols, kc, "Repository", repo);
    help_row(out, cols, kc, "Branch", branch);
    git_sync_project_status_t st;
    git_sync_project_status(dir, &st);
    if (st.busy) {
        help_row(out, cols, kc, "Status", "sync in progress");
    } else if (st.error) {
        help_row(out, cols, kc, "Status", "cannot read the repository");
    } else if (st.never_pushed) {
        help_row(out, cols, kc, "Status", "never synchronized");
    } else if (st.modified == 0 && st.not_pushed == 0 && st.deleted_locally == 0) {
        help_row(out, cols, kc, "Status", "synchronized");
    } else {
        help_row(out, cols, kc, "Status", "not synchronized");
        if (st.modified) {
            snprintf(buf, sizeof(buf), "%d file(s)", st.modified);
            help_row(out, cols, kc, "Changed", buf);
        }
        if (st.not_pushed) {
            snprintf(buf, sizeof(buf), "%d file(s) not on the server", st.not_pushed);
            help_row(out, cols, kc, "New", buf);
        }
        if (st.deleted_locally) {
            snprintf(buf, sizeof(buf), "%d file(s) deleted here", st.deleted_locally);
            help_row(out, cols, kc, "Deleted", buf);
        }
    }
}

/* data/len is the content to compare with the server, or NULL to let
 * Git sync read the saved file itself. */
/* Book information (Alt+I on a book): title, file, size, chapters and,
 * when reading (chapter >= 0), the current chapter. */
static void build_bookinfo_text(std::string &out, int cols, const char *path, int chapter)
{
    int kc = 11;
    const char *slash = strrchr(path, '/');
    const char *why = NULL;
    epub_book_t *b = epub_open(path, &why);
    help_heading(out, "Book");
    help_row(out, cols, kc, "Title", (b && epub_title(b)) ? epub_title(b) : "(none)");
    help_row(out, cols, kc, "File", slash ? slash + 1 : path);
    const char *mp = sd_card_get_mount_point();
    size_t ml = strlen(mp);
    std::string folder = slash ? std::string(path, (size_t)(slash - path)) : std::string();
    if (!strncmp(folder.c_str(), mp, ml)) folder.erase(0, ml);
    help_row(out, cols, kc, "Folder", folder.empty() ? "/" : folder.c_str());
    struct stat st;
    char buf[64];
    if (stat(path, &st) == 0) {
        format_size(buf, sizeof(buf), (uint64_t)st.st_size);
        help_row(out, cols, kc, "Size", buf);
    }
    if (!b) {
        help_row(out, cols, kc, "Status", why ? why : "Cannot be read");
        return;
    }
    snprintf(buf, sizeof(buf), "%d", epub_chapter_count(b));
    help_row(out, cols, kc, "Chapters", buf);
    if (chapter >= 0) {
        const char *t = epub_chapter_title(b, chapter);
        snprintf(buf, sizeof(buf), "%d of %d", chapter + 1, epub_chapter_count(b));
        std::string cur = buf;
        if (t) { cur += ": "; cur += t; }
        help_row(out, cols, kc, "Reading", cur.c_str());
    }
    epub_close(b);
}

static void build_fileinfo_text(std::string &out, int cols, const char *name,
                                const char *fmt_name, bool unsaved,
                                const text_counts_t *tc,
                                const char *data, size_t len,
                                const char *project, const char *git_name)
{
    const int kc = 15;
    char buf[64];
    unsigned words = tc->words, paras = tc->paras, lines = tc->lines, chars = tc->chars;

    help_heading(out, "File");
    help_row(out, cols, kc, "Name", name);
    help_row(out, cols, kc, "Format", fmt_name);
    snprintf(buf, sizeof(buf), "%u bytes", (unsigned)tc->size);
    help_row(out, cols, kc, "Size", buf);
    if (unsaved) help_row(out, cols, kc, "Unsaved", "yes (counts include the changes)");

    help_heading(out, "Text");
    snprintf(buf, sizeof(buf), "%u", words);
    help_row(out, cols, kc, "Words", buf);
    snprintf(buf, sizeof(buf), "%u", paras);
    help_row(out, cols, kc, "Paragraphs", buf);
    snprintf(buf, sizeof(buf), "%u", lines);
    help_row(out, cols, kc, "Lines", buf);
    snprintf(buf, sizeof(buf), "%u", chars);
    help_row(out, cols, kc, "Characters", buf);

    help_heading(out, "Git sync");
    fileinfo_git_row(out, cols, kc, project, git_name, data, len);

    if (project) build_projectinfo_text(out, cols, project);
}

static const char *fileinfo_format_name(editor_doc_format_t f)
{
    switch (f) {
    case EDITOR_DOC_FOUNTAIN: return "Fountain screenplay";
    case EDITOR_DOC_TEXT:     return "Plain text";
    default:                  return "Markdown";
    }
}

static void show_fileinfo_page(help_origin_t origin, const std::string &text)
{
    show_help_page(origin, "File information - Esc or Enter to close", text);
}

/* Alt+I in the editor: the focused pane's document. */
static void show_editor_fileinfo(void)
{
    if (!s_scr_help) return;
    pane_bind_focus();
    const char *path = editor_get_file_path();
    if (editor_is_read_only() && path) {
        std::string text;
        build_bookinfo_text(text, help_cols(), path, editor_book_chapter());
        show_help_page(HELP_FROM_EDITOR, "Book information - Esc or Enter to close", text);
        return;
    }
    const char *name = "(untitled)";
    const char *git_name = NULL;
    project_name_t proj = "";
    if (path) {
        const char *slash = strrchr(path, '/');
        name = slash ? slash + 1 : path;
        if (project_of_path(path, proj, sizeof(proj))) git_name = name;
    }
    size_t len = 0;
    const char *data = editor_text_begin(&len);
    text_counts_t tc;
    counts_init(&tc);
    counts_feed(&tc, data, len);
    counts_finish(&tc);
    std::string text;
    build_fileinfo_text(text, help_cols(), name,
                        fileinfo_format_name(editor_get_format()),
                        editor_is_modified(), &tc, data ? data : "", len,
                        git_name ? proj : NULL, git_name);
    editor_text_end();
    show_fileinfo_page(HELP_FROM_EDITOR, text);
}

/* Alt+I in either file browser: the selected file, read from the SD
 * card, with its project; in the list of projects the selected
 * project. */
static void show_browser_fileinfo(void)
{
    if (!s_scr_help || !s_browser_list) return;
    help_origin_t origin = s_inpane_browser_open ? HELP_FROM_INPANE : HELP_FROM_BROWSER;
    const sd_card_file_entry_t *e = browser_selected_entry();
    if (const sd_card_file_entry_t *bk = browser_selected_book()) {
        char bdir[64], path[320];
        browser_dir(bdir, sizeof(bdir));
        snprintf(path, sizeof(path), "%s/%s", bdir, bk->name);
        std::string text;
        build_bookinfo_text(text, help_cols(), path, -1);
        show_help_page(origin, "Book information - Esc or Enter to close", text);
        return;
    }
    if (browser_at_root()) {
        if (!e) return;
        std::string text;
        build_projectinfo_text(text, help_cols(), e->name);
        show_help_page(origin, "Project information - Esc or Enter to close", text);
        return;
    }
    if (!e || e->is_dir) {
        std::string text;
        build_projectinfo_text(text, help_cols(), s_cur_project);
        show_help_page(origin, "Project information - Esc or Enter to close", text);
        return;
    }
    const char *name = e->name;
    char dir[64];
    project_dir(s_cur_project, dir, sizeof(dir));
    char path[320];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    /* Counted in chunks: the file is never loaded whole. Git sync
     * reads it again itself for the server comparison. */
    FILE *f = fopen(path, "rb");
    char *chunk = (char *)malloc(4096);
    if (!f || !chunk) {
        if (f) fclose(f);
        free(chunk);
        editor_ui_set_status("Cannot read the file");
        return;
    }
    text_counts_t tc;
    counts_init(&tc);
    size_t n;
    while ((n = fread(chunk, 1, 4096, f)) > 0) counts_feed(&tc, chunk, n);
    bool rd_err = ferror(f);
    fclose(f);
    free(chunk);
    if (rd_err) {
        editor_ui_set_status("Cannot read the file");
        return;
    }
    counts_finish(&tc);
    editor_doc_format_t fmt = EDITOR_DOC_MARKDOWN;
    editor_path_format(name, &fmt);
    std::string text;
    build_fileinfo_text(text, help_cols(), name, fileinfo_format_name(fmt),
                        false, &tc, NULL, 0, s_cur_project, name);
    show_fileinfo_page(origin, text);
}

static void close_help(void)
{
    s_help_open = false;
    switch (s_help_origin) {
    case HELP_FROM_EDITOR:
        editor_ui_show_editor();
        break;
    case HELP_FROM_INPANE:
        editor_ui_show_editor();
        show_inpane_browser();
        break;
    default:
        editor_ui_show_file_browser();
        break;
    }
}

static void handle_help_key(const kb_event_t *ev)
{
    int32_t line = lv_font_get_line_height(FONT_14);
    int32_t page = LIST_PANEL_H - line;
    if (page < line) page = line;

    /* Ctrl+Up / Ctrl+Down page, as in the editor. */
    bool ctrl = (ev->modifier & (KB_MOD_LCTRL | KB_MOD_RCTRL)) != 0;
    if (ctrl && ev->keycode == KB_KEY_UP) {
        lv_obj_scroll_by_bounded(s_help_cont, 0, page, LV_ANIM_OFF);
        return;
    }
    if (ctrl && ev->keycode == KB_KEY_DOWN) {
        lv_obj_scroll_by_bounded(s_help_cont, 0, -page, LV_ANIM_OFF);
        return;
    }

    switch (ev->keycode) {
    case KB_KEY_UP:
        lv_obj_scroll_by_bounded(s_help_cont, 0, line, LV_ANIM_OFF);
        break;
    case KB_KEY_DOWN:
        lv_obj_scroll_by_bounded(s_help_cont, 0, -line, LV_ANIM_OFF);
        break;
    case KB_KEY_PAGEUP:
        lv_obj_scroll_by_bounded(s_help_cont, 0, page, LV_ANIM_OFF);
        break;
    case KB_KEY_PAGEDOWN:
    case KB_KEY_SPACE:
        lv_obj_scroll_by_bounded(s_help_cont, 0, -page, LV_ANIM_OFF);
        break;
    case KB_KEY_HOME:
        lv_obj_scroll_to_y(s_help_cont, 0, LV_ANIM_OFF);
        break;
    case KB_KEY_END:
        lv_obj_scroll_by_bounded(s_help_cont, 0, -LV_COORD_MAX / 2, LV_ANIM_OFF);
        break;
    case KB_KEY_ENTER:
    case KB_KEY_ESCAPE:
    case KB_KEY_F10:
        close_help();
        break;
    default:
        break;
    }
}

#if defined(CONFIG_DRAFTLING_TOUCHSCREEN)
/* A tap on the help page closes it; a drag scrolls it (LVGL does not
 * report a click at the end of a scroll). */
static void help_touch_cb(lv_event_t *e)
{
    (void)e;
    if (s_help_open) close_help();
}
#endif

/* ---- Save-prompt overlay ---- */

/* Compute a default filename for a new (untitled) document, with the
 * extension of its format (".md", ".fountain" or ".txt"). The draft
 * numbers are shared by all extensions, so a new screenplay after
 * draft_001.md becomes draft_002.fountain, not draft_001.fountain.
 * Returns the bare filename (no directory prefix). */
static bool generate_default_name(char *buf, size_t buf_size)
{
    static const char *const exts[] = { "md", "fountain", "txt" };
    char mp[64];
    if (!new_doc_dir(mp, sizeof(mp))) return false;
    const char *ext = editor_format_ext(editor_get_format());
    char path[256];
    for (int seq = 1; seq <= MAX_DRAFT_SEQ; seq++) {
        bool taken = false;
        for (const char *e : exts) {
            snprintf(path, sizeof(path), "%s/draft_%03d.%s", mp, seq, e);
            if (sd_card_file_exists(path)) taken = true;
        }
        if (!taken) {
            snprintf(buf, buf_size, "draft_%03d.%s", seq, ext);
            return true;
        }
    }
    return false;
}

static void refresh_save_prompt(void)
{
    if (!s_save_panel) return;
    lv_label_set_text(s_save_name_lbl, s_save_buf);

    /* Position the thin cursor bar after the character at s_save_pos.
     * We measure the pixel width of the text up to the cursor position
     * using the monospace character width. */
    int cw = char_width_for_font(FONT_11);
    /* Count UTF-8 characters up to s_save_pos bytes */
    int chars = 0;
    for (int i = 0; i < s_save_pos; i++) {
        if ((s_save_buf[i] & 0xC0) != 0x80) chars++;
    }
    int cx = chars * cw;
    lv_obj_set_pos(s_save_cur, cx, overlay_row_y());
    lv_obj_remove_flag(s_save_cur, LV_OBJ_FLAG_HIDDEN);
}

static void show_save_prompt(void)
{
    /* Pre-fill with existing filename (bare name, no directory).
     * s_rename_src holds the folder the file is saved in: the file's
     * own, or for an untitled document its project's. */
    const char *path = editor_get_file_path();
    if (path) {
        const char *slash = strrchr(path, '/');
        const char *name = slash ? slash + 1 : path;
        size_t dl = slash ? (size_t)(slash - path) : 0;
        if (dl >= sizeof(s_rename_src)) dl = sizeof(s_rename_src) - 1;
        memcpy(s_rename_src, path, dl);
        s_rename_src[dl] = '\0';
        strncpy(s_save_buf, name, sizeof(s_save_buf) - 1);
        s_save_buf[sizeof(s_save_buf) - 1] = '\0';
    } else {
        if (!new_doc_dir(s_rename_src, sizeof(s_rename_src))) {
            editor_ui_set_status("Save failed: no project -- create one first");
            return;
        }
        if (!generate_default_name(s_save_buf, sizeof(s_save_buf))) {
            editor_ui_set_status("Save failed: too many drafts");
            return;
        }
    }
    s_save_pos = (int)strlen(s_save_buf);
    s_save_open = true;
    save_prompt_force_latin();
    s_save_mode = SAVE_MODE_SAVE_AS;
    lv_label_set_text(s_save_hdr_lbl, "Save as (Enter/Esc):");
    lv_obj_set_parent(s_save_panel, s_scr);
    lv_obj_remove_flag(s_save_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_save_panel);
    refresh_save_prompt();
}

/* F2 / Alt+R in the file browser: ask for a new name for the selected file. */
/* The single-line name prompt (the save-as overlay) in another mode. */
static void show_name_prompt(save_mode_t mode, const char *hdr, const char *initial)
{
    if (!s_save_panel) return;
    snprintf(s_save_buf, sizeof(s_save_buf), "%s", initial ? initial : "");
    s_save_pos = (int)strlen(s_save_buf);
    s_save_open = true;
    save_prompt_force_latin();
    s_save_mode = mode;
    s_save_from_inpane = s_inpane_browser_open;
    lv_label_set_text(s_save_hdr_lbl, hdr);
    lv_obj_set_parent(s_save_panel, lv_scr_act());
    lv_obj_remove_flag(s_save_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_save_panel);
    refresh_save_prompt();
}

static bool save_mode_is_project_name(save_mode_t m)
{
    return m == SAVE_MODE_PROJECT_NEW || m == SAVE_MODE_PROJECT_FIRST ||
           m == SAVE_MODE_PROJECT_MIGRATE || m == SAVE_MODE_PROJECT_RENAME ||
           m == SAVE_MODE_XFER_PROJECT;
}

/* F2 / Alt+R in the file browser: ask for a new name for the selected
 * file, or in the list of projects for the selected project. */
static void show_rename_prompt(void)
{
    if (!s_save_panel || !s_browser_list) return;
    const sd_card_file_entry_t *e = browser_selected_entry();
    if (!e) return;
    if (const sd_card_file_entry_t *bk = browser_selected_book()) {
        char bdir[64];
        browser_dir(bdir, sizeof(bdir));
        snprintf(s_rename_src, sizeof(s_rename_src), "%s/%s", bdir, bk->name);
        show_name_prompt(SAVE_MODE_RENAME, "Rename to (Enter/Esc):", bk->name);
        return;
    }
    if (browser_at_root()) {
        snprintf(s_rename_src, sizeof(s_rename_src), "%s", e->name);
        show_name_prompt(SAVE_MODE_PROJECT_RENAME, "Rename project to (Enter/Esc):", e->name);
        return;
    }
    if (e->is_dir) return;
    char dir[64];
    project_dir(s_cur_project, dir, sizeof(dir));
    snprintf(s_rename_src, sizeof(s_rename_src), "%s/%s", dir, e->name);
    show_name_prompt(SAVE_MODE_RENAME, "Rename to (Enter/Esc):", e->name);
}

static void close_save_prompt(void)
{
    s_save_open = false;
    lv_obj_add_flag(s_save_panel, LV_OBJ_FLAG_HIDDEN);
    save_prompt_restore_layout();
}

static void rename_prompt_confirm(void)
{
    const char *old_name = strrchr(s_rename_src, '/');
    old_name = old_name ? old_name + 1 : s_rename_src;
    if (strcmp(s_save_buf, old_name) == 0) {
        close_save_prompt();
        restore_default_status();
        return;
    }
    /* Any other extension would hide the file from the browser. */
    if (editor_path_is_book(old_name)) {
        if (!editor_path_is_book(s_save_buf)) {
            editor_ui_set_status("Name must end in .epub");
            return;
        }
    } else if (!editor_path_format(s_save_buf, NULL)) {
        editor_ui_set_status("Name must end in .md, .fountain or .txt");
        return;
    }
    if (strchr(s_save_buf, '/')) {
        editor_ui_set_status("Name must not contain /");
        return;
    }
    char path[512];
    snprintf(path, sizeof(path), "%.*s/%s", (int)(old_name - 1 - s_rename_src),
             s_rename_src, s_save_buf);
    esp_err_t err = editor_rename_file(s_rename_src, path);
    if (err == ESP_ERR_INVALID_STATE) {
        editor_ui_set_status("Rename failed: name already exists");
        return;
    }
    close_save_prompt();
    if (err != ESP_OK) {
        editor_ui_set_status("Rename failed!");
        return;
    }
    refresh_file_list(s_save_buf);
    char msg[80];
    snprintf(msg, sizeof(msg), "Renamed to %.40s", s_save_buf);
    editor_ui_set_status(msg);
}

/* ---- Project prompts, move / copy (see save_mode_t) ---- */

static void save_open_doc_cb(editor_doc_t *doc, void *ctx)
{
    const char *path = (const char *)ctx;
    const char *p = editor_doc_get_path(doc);
    if (p && strcmp(p, path) == 0 && editor_doc_is_modified(doc)) editor_doc_save(doc);
}

/* Back to the browser Alt+M / Alt+C was pressed in, same project. */
static void xfer_return(void)
{
    if (s_xfer_from_inpane) {
        editor_ui_show_editor();
        show_inpane_browser();
        check_project_setup();   /* editor_ui_show_file_browser() does it otherwise */
    } else {
        editor_ui_show_file_browser();
    }
}

/* Move or copy the picked file into dst_project as dst_name. Returns
 * false (with a status message) if that name is taken or it fails. */
static bool xfer_do(const char *dst_project, const char *dst_name)
{
    char sdir[64], ddir[64], src[400], dst[400];
    project_dir(s_xfer_src_project, sdir, sizeof(sdir));
    project_dir(dst_project, ddir, sizeof(ddir));
    snprintf(src, sizeof(src), "%s/%s", sdir, s_xfer_src_name);
    snprintf(dst, sizeof(dst), "%s/%s", ddir, dst_name);
    if (sd_card_file_exists(dst)) {
        editor_ui_set_status("A file with this name exists there");
        return false;
    }
    /* Unsaved edits of an open copy go along. */
    editor_doc_foreach(save_open_doc_cb, src);
    esp_err_t err = s_xfer_copy ? sd_card_copy_file(src, dst)
                                : editor_rename_file(src, dst);
    char msg[128];
    if (err != ESP_OK) {
        snprintf(msg, sizeof(msg), "%s failed", s_xfer_copy ? "Copy" : "Move");
        editor_ui_set_status(msg);
        return false;
    }
    bool same = strcmp(dst_project, s_xfer_src_project) == 0;
    if (same)
        snprintf(msg, sizeof(msg), "Copied as %.60s", dst_name);
    else
        snprintf(msg, sizeof(msg), "%s %.40s to %.32s", s_xfer_copy ? "Copied" : "Moved",
                 s_xfer_src_name, dst_project);
    /* Navigation stays in the original project. */
    refresh_file_list(s_xfer_copy ? (same ? dst_name : s_xfer_src_name) : NULL);
    editor_ui_set_status(msg);
    return true;
}

#if defined(CONFIG_DRAFTLING_TOUCHSCREEN)
static list_touch_ctx_t s_xfer_touch_ctx;
#endif
static void xfer_picker_activate(int idx);

static void refresh_xfer_picker_items(void)
{
    char buf[96];
    if (s_xfer_migrate)
        snprintf(buf, sizeof(buf), "Move the files outside projects to:");
    else
        snprintf(buf, sizeof(buf), "%s %.40s to project:", s_xfer_copy ? "Copy" : "Move",
                 s_xfer_src_name);
    lv_label_set_text(s_lbl_menu_hdr, buf);
    lv_obj_clean(s_menu_list);
    for (int i = 0; i < s_xfer_nprojects; i++) {
        if (strcmp(s_xfer_projects[i], s_xfer_src_project) == 0)
            snprintf(buf, sizeof(buf), "%.32s (this project, new name)", s_xfer_projects[i]);
        else
            snprintf(buf, sizeof(buf), "%.32s", s_xfer_projects[i]);
        lv_list_add_btn(s_menu_list, NULL, buf);
    }
    lv_list_add_btn(s_menu_list, NULL, "New project...");
    apply_list_selection_styles(s_menu_list, s_xfer_sel);
    s_xfer_sel_prev = s_xfer_sel;
#if defined(CONFIG_DRAFTLING_TOUCHSCREEN)
    s_xfer_touch_ctx.p_sel      = &s_xfer_sel;
    s_xfer_touch_ctx.p_sel_prev = &s_xfer_sel_prev;
    s_xfer_touch_ctx.activate   = xfer_picker_activate;
    list_touch_attach(s_menu_list, &s_xfer_touch_ctx);
#endif
    sync_battery_labels();
}

static void close_xfer_picker(void)
{
    s_xfer_open = false;
    s_menu_open = false;
    lv_label_set_text(s_lbl_menu_hdr, MENU_HDR_TEXT);
    xfer_return();
}

/* Move the files found outside any project into the project (created
 * if needed) and open it. */
static void migrate_into_project(const char *name)
{
    char msg[128];
    int moved = 0;
    esp_err_t err = project_migrate_root(name, &moved);
    if (!project_exists(name)) {
        editor_ui_set_status("Cannot create the project folder");
        return;
    }
    browser_enter_project(name);
    if (err != ESP_OK)
        snprintf(msg, sizeof(msg), "Moved %d file(s) to %.32s, some could not be moved",
                 moved, name);
    else
        snprintf(msg, sizeof(msg), "Moved %d file(s) to project %.32s", moved, name);
    set_status_with_timeout(msg, 6000);
}

/* Fill the picker rows with every project on the card except exclude;
 * first (when given) leads the list. */
static void xfer_set_projects(const char *first, const char *exclude)
{
    int n = 0;
    project_name_t *all = project_list(&n);
    if (n < 0) {
        editor_ui_set_status("Cannot read the list of projects");
        n = 0;
    }
    free(s_xfer_projects);
    s_xfer_projects = (project_name_t *)malloc((size_t)(n + 1) * sizeof(project_name_t));
    s_xfer_nprojects = 0;
    if (s_xfer_projects) {
        if (first) snprintf(s_xfer_projects[s_xfer_nprojects++], sizeof(project_name_t), "%s", first);
        for (int i = 0; i < n; i++) {
            if (exclude && strcmp(all[i], exclude) == 0) continue;
            snprintf(s_xfer_projects[s_xfer_nprojects++], sizeof(project_name_t), "%s", all[i]);
        }
    }
    free(all);
}

/* "Files outside projects" dialog with projects on the card: pick one
 * of them, or "New project...". Esc returns to the dialog. */
static void show_migrate_picker(void)
{
    xfer_set_projects(NULL, NULL);
    s_xfer_copy = false;
    s_xfer_src_project[0] = '\0';
    s_xfer_from_inpane = s_inpane_browser_open;
    if (s_inpane_browser_open) close_inpane_browser();
    show_menu();   /* clears the picker flags */
    s_xfer_migrate = true;
    s_xfer_open = true;
    s_xfer_sel = 0;
    refresh_xfer_picker_items();
}

/* Alt+M (move) / Alt+C (copy) on a file in a project. */
static void show_xfer_picker(bool copy)
{
    if (browser_at_root()) {
        editor_ui_set_status("Open a project and select a file");
        return;
    }
    const sd_card_file_entry_t *e = browser_selected_entry();
    if (!e || e->is_dir) {
        editor_ui_set_status("Select a file first");
        return;
    }
    s_xfer_copy = copy;
    s_xfer_from_inpane = s_inpane_browser_open;
    snprintf(s_xfer_src_project, sizeof(s_xfer_src_project), "%s", s_cur_project);
    snprintf(s_xfer_src_name, sizeof(s_xfer_src_name), "%s", e->name);
    browser_remember_selection();

    xfer_set_projects(copy ? s_cur_project : NULL, s_cur_project);

    if (s_inpane_browser_open) close_inpane_browser();
    show_menu();
    s_xfer_open = true;
    s_xfer_sel = 0;
    refresh_xfer_picker_items();
}

static void xfer_picker_activate(int idx)
{
    bool is_new = (idx >= s_xfer_nprojects);
    project_name_t dst = "";
    if (!is_new) snprintf(dst, sizeof(dst), "%s", s_xfer_projects[idx]);
    if (s_xfer_migrate) {
        s_xfer_migrate = false;
        s_setup_busy = true;
        close_xfer_picker();
        s_setup_busy = false;
        if (is_new) {
            char def[PROJECT_NAME_MAX + 1];
            default_project_name(def, sizeof(def));
            show_name_prompt(SAVE_MODE_PROJECT_MIGRATE,
                             "Name of the new project (Enter/Esc):", def);
        } else {
            migrate_into_project(dst);
        }
        return;
    }
    close_xfer_picker();   /* prompts go onto the browser's screen */
    const char *verb = s_xfer_copy ? "Copy" : "Move";
    char hdr[64];
    if (is_new) {
        snprintf(hdr, sizeof(hdr), "%s to new project (Enter/Esc):", verb);
        char def[PROJECT_NAME_MAX + 1];
        default_project_name(def, sizeof(def));
        show_name_prompt(SAVE_MODE_XFER_PROJECT, hdr, def);
        return;
    }
    snprintf(s_xfer_dst_project, sizeof(s_xfer_dst_project), "%s", dst);
    char ddir[64], free_name[256];
    project_dir(dst, ddir, sizeof(ddir));
    if (!project_free_file_name(ddir, s_xfer_src_name, free_name, sizeof(free_name))) {
        editor_ui_set_status("No free file name there");
        return;
    }
    if (strcmp(dst, s_xfer_src_project) == 0 || strcmp(free_name, s_xfer_src_name) != 0) {
        /* Same project, or the name is taken there: ask for a name. */
        snprintf(hdr, sizeof(hdr), "%s as (Enter/Esc):", verb);
        show_name_prompt(SAVE_MODE_XFER_NAME, hdr, free_name);
        return;
    }
    xfer_do(dst, s_xfer_src_name);
}

static void handle_xfer_picker_key(const kb_event_t *ev)
{
    int last_row = s_xfer_nprojects;   /* "New project..." */
    switch (ev->keycode) {
    case KB_KEY_UP:
        if (s_xfer_sel > 0) s_xfer_sel--;
        break;
    case KB_KEY_DOWN:
        if (s_xfer_sel < last_row) s_xfer_sel++;
        break;
    case KB_KEY_ENTER:
        xfer_picker_activate(s_xfer_sel);
        return;
    case KB_KEY_ESCAPE:
        /* From the files-outside-projects picker this lands back on its
         * dialog (check_project_setup() runs as the browser shows). */
        s_xfer_migrate = false;
        close_xfer_picker();
        restore_default_status();
        return;
    default:
        return;
    }
    update_list_highlight(s_menu_list, s_xfer_sel, s_xfer_sel_prev);
    s_xfer_sel_prev = s_xfer_sel;
}

/* Back to the browser a project prompt was raised from. */
static void name_prompt_return(void)
{
    if (s_save_from_inpane && s_pane_count > 1 && !s_inpane_browser_open) {
        editor_ui_show_editor();
        show_inpane_browser();
    }
}

/* Esc in the name prompt. */
static void save_prompt_cancel(void)
{
    save_mode_t mode = s_save_mode;
    close_save_prompt();
    if (mode == SAVE_MODE_PROJECT_MIGRATE) {
        /* Back to the "Files outside projects" pop-up: move or power off. */
        check_project_setup();
        return;
    }
    if (mode == SAVE_MODE_PROJECT_FIRST) {
        /* Not asked again before the next boot. */
        s_setup_dismissed = true;
        editor_ui_set_status("No project yet -- N: new project");
        return;
    }
    restore_default_status();
}

static bool project_name_ok_or_status(const char *name)
{
    const char *why = project_name_problem(name);
    if (why) {
        editor_ui_set_status(why);
        return false;
    }
    return true;
}

static void project_prompt_confirm(void)
{
    char name[sizeof(s_save_buf)];
    snprintf(name, sizeof(name), "%s", s_save_buf);
    char msg[128];

    switch (s_save_mode) {
    case SAVE_MODE_PROJECT_NEW:
    case SAVE_MODE_PROJECT_FIRST: {
        if (!project_name_ok_or_status(name)) return;
        if (project_exists(name)) {
            editor_ui_set_status("A project with this name exists");
            return;
        }
        if (project_create(name) != ESP_OK) {
            editor_ui_set_status("Cannot create the project folder");
            return;
        }
        close_save_prompt();
        browser_enter_project(name);
        name_prompt_return();
        snprintf(msg, sizeof(msg), "Created project %.32s -- N: new file", name);
        editor_ui_set_status(msg);
        return;
    }
    case SAVE_MODE_PROJECT_MIGRATE: {
        if (!project_name_ok_or_status(name)) return;
        if (project_exists(name)) {
            editor_ui_set_status("A project with this name exists -- choose a new name");
            return;
        }
        close_save_prompt();
        migrate_into_project(name);
        return;
    }
    case SAVE_MODE_PROJECT_RENAME: {
        const char *old = s_rename_src;
        if (strcmp(name, old) == 0) {
            close_save_prompt();
            restore_default_status();
            return;
        }
        if (!project_name_ok_or_status(name)) return;
        if (strcasecmp(name, old) != 0 && project_exists(name)) {
            editor_ui_set_status("A project with this name exists");
            return;
        }
        if (git_sync_get_state() == GIT_SYNC_IN_PROGRESS) {
            editor_ui_set_status("Rename: wait for Git sync to finish");
            return;
        }
        char od[64], nd[64];
        project_dir(old, od, sizeof(od));
        project_dir(name, nd, sizeof(nd));
        esp_err_t err = editor_rename_dir(od, nd);
        if (err != ESP_OK) {
            editor_ui_set_status(err == ESP_ERR_INVALID_STATE
                                     ? "A file or folder with this name exists"
                                     : "Rename failed!");
            return;
        }
        close_save_prompt();
        if (strcmp(s_last_project, old) == 0 &&
            snprintf(s_last_project, sizeof(s_last_project), "%s", name)
                < (int)sizeof(s_last_project))
            projects_set_last(name);
        refresh_file_list(name);
        name_prompt_return();
        snprintf(msg, sizeof(msg), "Renamed project to %.32s", name);
        editor_ui_set_status(msg);
        return;
    }
    case SAVE_MODE_XFER_PROJECT: {
        if (!project_name_ok_or_status(name)) return;
        if (project_exists(name)) {
            editor_ui_set_status("This project exists -- pick it from the list");
            return;
        }
        if (project_create(name) != ESP_OK) {
            editor_ui_set_status("Cannot create the project folder");
            return;
        }
        close_save_prompt();
        name_prompt_return();
        xfer_do(name, s_xfer_src_name);
        return;
    }
    case SAVE_MODE_XFER_NAME: {
        if (strchr(name, '/')) {
            editor_ui_set_status("Name must not contain /");
            return;
        }
        if (!editor_path_format(name, NULL)) {
            editor_ui_set_status("Name must end in .md, .fountain or .txt");
            return;
        }
        char ddir[64], dst[400];
        project_dir(s_xfer_dst_project, ddir, sizeof(ddir));
        snprintf(dst, sizeof(dst), "%s/%s", ddir, name);
        if (sd_card_file_exists(dst)) {
            editor_ui_set_status("A file with this name exists there");
            return;
        }
        close_save_prompt();
        name_prompt_return();
        xfer_do(s_xfer_dst_project, name);
        return;
    }
    default:
        close_save_prompt();
        return;
    }
}

static void save_prompt_confirm(void)
{
    if (s_save_buf[0] == '\0') {
        /* Empty name -- ignore */
        return;
    }
    if (s_save_is_rename) {
        rename_prompt_confirm();
        return;
    }
    if (s_save_mode != SAVE_MODE_SAVE_AS) {
        project_prompt_confirm();
        return;
    }
    if (!sd_card_is_ready() || !s_rename_src[0]) {
        close_save_prompt();
        editor_ui_set_status("Save failed: " STORAGE_NAME " not ready");
        return;
    }
    if (strchr(s_save_buf, '/')) {
        editor_ui_set_status("Name must not contain /");
        return;
    }
    char path[512];
    if (snprintf(path, sizeof(path), "%s/%s", s_rename_src, s_save_buf) >= (int)sizeof(path)) {
        editor_ui_set_status("Name too long");
        return;
    }
    esp_err_t err = editor_save_file_as(path);
    close_save_prompt();
    if (err == ESP_OK) {
        project_name_t pr;
        if (project_of_path(path, pr, sizeof(pr))) {
            snprintf(s_last_project, sizeof(s_last_project), "%s", pr);
            project_set_last_file(pr, s_save_buf);
        }
        char msg[80];
        snprintf(msg, sizeof(msg), "Saved as %.40s", s_save_buf);
        editor_ui_set_status(msg);
    } else {
        editor_ui_set_status("Save failed!");
    }
    editor_ui_refresh();
}

/* What may be typed into the save-as / rename / project name field. */
static bool save_name_accept(const char *text, size_t cur_len)
{
    /* Project names: ASCII letters, digits, '-', '_', ' '. */
    if (save_mode_is_project_name(s_save_mode))
        return strlen(text) == 1 && project_name_char_ok(text[0]) &&
               cur_len + 1 <= PROJECT_NAME_MAX;
    /* File names: printable ASCII only, without the characters that
     * Windows / FAT, macOS or a shell refuse or treat specially, so the
     * file moves to any computer and Git host unchanged. Accented and
     * other non-ASCII letters are left out for the same reason. */
    if (strlen(text) != 1) return false;
    unsigned char c = (unsigned char)text[0];
    if (c < 0x20 || c > 0x7E) return false;
    return strchr("/\\:*?\"<>|&'$", c) == NULL;
}

static void handle_save_prompt_key(const kb_event_t *ev)
{
    switch (ev->keycode) {
    case KB_KEY_ENTER:
        save_prompt_confirm();
        return;
    case KB_KEY_ESCAPE:
        save_prompt_cancel();
        return;
    default:
        text_field_key(s_save_buf, sizeof(s_save_buf), &s_save_pos, ev, save_name_accept);
        break;
    }
    refresh_save_prompt();
}

/* ---- New-document format picker logic ---- */

static void refresh_newfmt_prompt(void)
{
    if (!s_newfmt_panel) return;
    for (int i = 0; i < NEWFMT_COUNT; i++) {
        if (!s_newfmt_opt_lbl[i]) continue;
        bool sel = (i == s_newfmt_sel);
        lv_obj_set_style_bg_color(s_newfmt_opt_lbl[i],
                                  sel ? theme_fg() : theme_bg(), 0);
        lv_obj_set_style_bg_opa(s_newfmt_opt_lbl[i], LV_OPA_COVER, 0);
        lv_obj_set_style_text_color(s_newfmt_opt_lbl[i],
                                    sel ? theme_bg() : theme_fg(), 0);
    }
}

static void show_newfmt_prompt(newdoc_target_t target)
{
    if (!s_newfmt_panel) return;
    s_newfmt_target = target;
    s_newfmt_open = true;
    s_newfmt_sel = 0;
    lv_obj_set_parent(s_newfmt_panel, lv_scr_act());
    lv_obj_remove_flag(s_newfmt_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_newfmt_panel);
    refresh_newfmt_prompt();
}

static void close_newfmt_prompt(void)
{
    s_newfmt_open = false;
    if (s_newfmt_panel) lv_obj_add_flag(s_newfmt_panel, LV_OBJ_FLAG_HIDDEN);
}

/* Start a new untitled document in the picked format where the picker
 * was raised from. */
static void newfmt_prompt_activate(void)
{
    editor_doc_format_t f = NEWFMT_FORMATS[s_newfmt_sel];
    newdoc_target_t target = s_newfmt_target;
    close_newfmt_prompt();

    switch (target) {
    case NEWDOC_BROWSER:
        editor_new_file();
        editor_set_format(f);
        editor_ui_show_editor();
        break;
    case NEWDOC_INPANE:
        /* A new document in the focused pane (not a global replace --
         * that is the single-pane behavior). */
        if (!open_into_pane(s_focus, NULL)) {
            editor_ui_set_status("New failed");
            return;
        }
        editor_set_format(f);
        s_open_target_pane = s_focus;
        close_inpane_browser();
        ensure_cursor_visible();
        editor_ui_show_editor();
        break;
    case NEWDOC_EDITOR:
        /* In single-pane mode this is the historical "replace current
         * doc" behavior; while split it only affects the focused pane. */
        pane_bind_focus();
        if (s_pane_count <= 1) {
            editor_new_file();
            s_panes[0].doc = editor_get_active();
            s_panes[0].has_saved_view = false;   /* adopt the new doc's view */
        } else {
            open_into_pane(s_focus, NULL);
        }
        editor_set_format(f);
        ensure_cursor_visible();
        editor_ui_refresh();
        break;
    }
}

static void handle_newfmt_prompt_key(const kb_event_t *ev)
{
    switch (ev->keycode) {
    case KB_KEY_UP:
        s_newfmt_sel = (s_newfmt_sel + NEWFMT_COUNT - 1) % NEWFMT_COUNT;
        refresh_newfmt_prompt();
        return;
    case KB_KEY_DOWN:
        s_newfmt_sel = (s_newfmt_sel + 1) % NEWFMT_COUNT;
        refresh_newfmt_prompt();
        return;
    case KB_KEY_ENTER:
        newfmt_prompt_activate();
        return;
    case KB_KEY_ESCAPE:
        close_newfmt_prompt();
        return;
    default:
        break;
    }
    /* M / F / T pick a format directly. */
    char ch = kb_layout_shortcut_char(ev->keycode);
    int pick = (ch == 'm') ? 0 : (ch == 'f') ? 1 : (ch == 't') ? 2 : -1;
    if (pick >= 0) {
        s_newfmt_sel = pick;
        newfmt_prompt_activate();
    }
}

/* ---- Writing timer and word goal (Ctrl+T) ----
 *
 * Ctrl+T in the editor or either file browser asks for an interval
 * (15 / 25 / 50 minutes) or a word goal (a number of words the user
 * types in, 1000 on a fresh install). The last choice -- the interval,
 * the goal figure and which of the two -- is remembered in NVS and
 * preselected next time. While a timer or a goal runs, a line on the
 * right edge of the screen grows from the top and reaches the bottom
 * when the time is up or the goal is reached; the editor and file
 * browser screens shrink by its width (s_ptimer_strip, CONTENT_W) so
 * the line never covers them. The line lives on lv_layer_top(), so it
 * shows on every screen. At the end the line blinks for
 * PTIMER_BLINK_MS, then the strip is given back. Idle sleep (the
 * inactivity and no-keyboard timers) is held off meanwhile; Ctrl+P and
 * the power buttons still put the device to sleep, which ends it.
 *
 * Word goal progress is the net number of words added to the open
 * documents since the goal was set (wg_scan()): each document's word
 * count (the Alt+I definition, counts_feed()) is taken when the goal
 * starts or the document is first seen, and the difference to its
 * current count is the progress; a document that is closed or
 * replaced as a whole (editor_doc_epoch() changes: open, reload after
 * a sync) keeps the words it contributed. Documents are recounted from
 * the tick timer once typing pauses (WGOAL_PAUSE_MS), and every
 * WGOAL_BUSY_MS during continuous typing on documents up to
 * WGOAL_BUSY_MAX_LEN bytes. The same count runs during a timer, and
 * with the "Show word count" checkbox (s_pt_show_words, NVS
 * ptwcount) the title bar shows it after the layout tag for as long
 * as the sprint lasts (sprint_title_words()); the bars are then not
 * hidden while typing (bars_may_hide()). */
#define NVS_KEY_PTIMER   "ptimer"
#define NVS_KEY_PTKIND   "ptkind"
#define NVS_KEY_WGOAL    "wgoal"
#define NVS_KEY_PTWCOUNT "ptwcount"
#define PTIMER_OPT_COUNT 3
static const uint8_t PTIMER_OPTIONS[PTIMER_OPT_COUNT] = { 15, 25, 50 };
#define PTIMER_ROW_WORDS  PTIMER_OPT_COUNT        /* "Word goal" row */
#define PTIMER_ROW_WCOUNT (PTIMER_OPT_COUNT + 1)  /* "Show word count" */
#define PTIMER_ROW_STOP   (PTIMER_OPT_COUNT + 2)  /* "Stop ..." row */
#define PTIMER_ROW_MAX    (PTIMER_OPT_COUNT + 3)
#define PTIMER_BLINK_MS  5000
#if defined(CONFIG_DRAFTLING_DISPLAY_EPD)
/* Every change of the line is a panel refresh: grow it in steps of
 * 30 s (2 % of a word goal) and blink slowly. */
#define PTIMER_STEP_MS   30000
#define WGOAL_STEPS      50
#define PTIMER_BLINK_PERIOD_MS 1000
#else
#define PTIMER_STEP_MS   0
#define WGOAL_STEPS      0
#define PTIMER_BLINK_PERIOD_MS 500
#endif
#define PTIMER_TICK_MS   1000
#define WGOAL_DEFAULT    1000
#define WGOAL_MAX        999999
#define WGOAL_DIGITS     6
#define WGOAL_PAUSE_MS   2000
#define WGOAL_BUSY_MS    15000
#define WGOAL_BUSY_MAX_LEN (512u * 1024u)
#define WGOAL_TRACK_MAX  4

typedef enum { PTIMER_IDLE, PTIMER_RUNNING, PTIMER_BLINKING } ptimer_state_t;
typedef enum { PT_KIND_TIMER = 0, PT_KIND_WORDS = 1 } pt_kind_t;

static ptimer_state_t s_pt_state    = PTIMER_IDLE;
static pt_kind_t      s_pt_kind     = PT_KIND_TIMER;  /* what runs */
static pt_kind_t      s_pt_last_kind = PT_KIND_TIMER; /* last picked */
static uint8_t        s_pt_minutes  = 25;     /* last interval picked */
static uint32_t       s_pt_start_ms = 0;      /* lv_tick_get() at start */
static uint32_t       s_pt_total_ms = 0;
static uint32_t       s_pt_blink_start_ms = 0;
static lv_timer_t    *s_pt_timer    = NULL;   /* tick / blink timer */
static lv_obj_t      *s_pt_bar      = NULL;   /* the line, on lv_layer_top() */
static int            s_pt_bar_h    = -1;     /* height last drawn */
static bool           s_pt_bar_on   = true;   /* blink phase */
static bool           s_pt_show_words = false; /* "Show word count" */
static int            s_pt_title_words = -1;   /* count in the title bar */

/* Word goal */
static uint32_t s_wg_goal   = WGOAL_DEFAULT;  /* last goal picked */
static uint32_t s_wg_target = 0;              /* goal of the running one */
static int64_t  s_wg_frozen = 0;   /* net words of documents no longer open */
typedef struct {
    const editor_doc_t *doc;       /* NULL = free entry */
    uint32_t epoch;
    uint32_t gen_counted, gen_seen;
    uint32_t seen_ms, count_ms;
    uint32_t base, last;           /* words at start / at the last count */
} wg_track_t;
static wg_track_t s_wg_track[WGOAL_TRACK_MAX];

/* Picker overlay (moved onto the active screen when shown). */
static lv_obj_t *s_pt_panel = NULL;
static lv_obj_t *s_pt_hdr_lbl = NULL;
static lv_obj_t *s_pt_opt_lbl[PTIMER_ROW_MAX] = { NULL };
static bool      s_pt_open = false;
static int       s_pt_sel  = 1;
static int       s_pt_rows = PTIMER_OPT_COUNT + 1;
static char      s_wg_buf[WGOAL_DIGITS + 1] = "";   /* goal being typed */
static bool      s_wg_fresh = true;  /* the next digit replaces the figure */

static void load_ptimer_from_nvs(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS_EDITOR, NVS_READONLY, &h) == ESP_OK) {
        uint8_t v = 0;
        if (nvs_get_u8(h, NVS_KEY_PTIMER, &v) == ESP_OK) {
            for (int i = 0; i < PTIMER_OPT_COUNT; i++)
                if (PTIMER_OPTIONS[i] == v) s_pt_minutes = v;
        }
        if (nvs_get_u8(h, NVS_KEY_PTKIND, &v) == ESP_OK)
            s_pt_last_kind = (v == PT_KIND_WORDS) ? PT_KIND_WORDS : PT_KIND_TIMER;
        uint32_t g = 0;
        if (nvs_get_u32(h, NVS_KEY_WGOAL, &g) == ESP_OK && g >= 1 && g <= WGOAL_MAX)
            s_wg_goal = g;
        if (nvs_get_u8(h, NVS_KEY_PTWCOUNT, &v) == ESP_OK)
            s_pt_show_words = (v != 0);
        nvs_close(h);
    }
}

static void save_ptimer_to_nvs(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS_EDITOR, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_u8(h, NVS_KEY_PTIMER, s_pt_minutes);
        nvs_set_u8(h, NVS_KEY_PTKIND, (uint8_t)s_pt_last_kind);
        nvs_set_u32(h, NVS_KEY_WGOAL, s_wg_goal);
        nvs_set_u8(h, NVS_KEY_PTWCOUNT, s_pt_show_words ? 1 : 0);
        nvs_commit(h);
        nvs_close(h);
    }
}

/* Width of the line: 2 px, 4 px on high-density panels, in panel
 * pixels (the 2x scale setting doubles every LVGL pixel). */
static int ptimer_strip_width(void)
{
#if defined(CONFIG_DRAFTLING_DISPLAY_HIDPI)
    int w = 4;
#else
    int w = 2;
#endif
    w /= display_scale_factor();
    return w < 1 ? 1 : w;
}

/* On e-paper a key handled in the same LVGL cycle may have set a
 * one-shot partial-refresh clip for its own edit (ClipGuard), which
 * would drop the line's change from the panel refresh. Refresh now,
 * with the whole changed area. */
static void ptimer_flush_now(void)
{
#if defined(CONFIG_DRAFTLING_DISPLAY_EPD)
    display_set_partial_clip(0, 0, 0, 0);
    lv_refr_now(NULL);
#endif
}

/* Re-apply CONTENT_W to the editor and file browser screens after the
 * strip appeared or went away, and re-wrap the editor text. */
static void ptimer_relayout(void)
{
    if (!s_scr) return;
    /* Editor screen bars */
    if (s_lbl_title)    lv_obj_set_width(s_lbl_title, TITLE_W);
    if (s_editor_hline) lv_obj_set_width(s_editor_hline, CONTENT_W);
    if (s_editor_sline) lv_obj_set_width(s_editor_sline, CONTENT_W);
    if (s_lbl_status) {
        lv_obj_set_width(s_lbl_status, status_avail_width());
        update_status_visibility();
    }
#if defined(CONFIG_DRAFTLING_HAS_BATTERY)
    const int wifi_off = 95;
#else
    const int wifi_off = 15;
#endif
#if defined(DRAFTLING_HAS_BATT_INDICATOR)
    if (s_lbl_dev_batt)    lv_obj_set_x(s_lbl_dev_batt, CONTENT_W - 80 - STA_IN_R);
    if (s_lbl_br_dev_batt) lv_obj_set_x(s_lbl_br_dev_batt, CONTENT_W - 80 - STA_IN_R);
#endif
    if (s_img_wifi)    lv_obj_set_x(s_img_wifi, CONTENT_W - wifi_off - STA_IN_R);
    if (s_img_br_wifi) lv_obj_set_x(s_img_br_wifi, CONTENT_W - wifi_off - STA_IN_R);
    /* File browser screen */
    if (s_list_files)    lv_obj_set_width(s_list_files, CONTENT_W);
    if (s_br_sline)      lv_obj_set_width(s_br_sline, CONTENT_W);
    if (s_lbl_br_status) lv_obj_set_width(s_lbl_br_status, CONTENT_W - 4 - STA_IN_L - STA_IN_R);

    /* Editor panes: new widths, text re-wrapped. */
    suggest_hide();
    layout_panes();
    if (s_inpane_browser_open && s_inpane_list && s_inpane_hdr) {
        const pane_t *fp = &s_panes[s_focus];
        lv_obj_set_width(s_inpane_hdr, (fp->w > 4) ? fp->w - 4 : fp->w);
        lv_obj_set_width(s_inpane_list, fp->w);
    }
    invalidate_all_render_caches();
    pane_bind_focus();
    ensure_cursor_visible();
    editor_ui_refresh();
    update_title_bar();
}

/* ---- Word goal progress ---- */

static uint32_t wg_count_words(const editor_doc_t *d)
{
    const char *a, *b;
    size_t al, bl;
    editor_doc_text_parts(d, &a, &al, &b, &bl);
    text_counts_t tc;
    counts_init(&tc);
    counts_feed(&tc, a, al);
    counts_feed(&tc, b, bl);
    counts_finish(&tc);
    return tc.words;
}

typedef struct {
    bool seen[WGOAL_TRACK_MAX];
    bool force;
} wg_scan_ctx_t;

static void wg_scan_doc(editor_doc_t *d, void *vctx)
{
    wg_scan_ctx_t *ctx = (wg_scan_ctx_t *)vctx;
    uint32_t epoch = editor_doc_epoch(d);
    uint32_t gen = editor_doc_generation(d);
    uint32_t now = lv_tick_get();
    for (int i = 0; i < WGOAL_TRACK_MAX; i++) {
        wg_track_t *t = &s_wg_track[i];
        if (t->doc != d || t->epoch != epoch) continue;
        ctx->seen[i] = true;
        if (gen == t->gen_counted) return;
        bool count = ctx->force;
        if (!count) {
            if (gen != t->gen_seen) {          /* still typing */
                t->gen_seen = gen;
                t->seen_ms = now;
            } else if ((uint32_t)(now - t->seen_ms) >= WGOAL_PAUSE_MS) {
                count = true;                  /* typing paused */
            }
            if (!count && (uint32_t)(now - t->count_ms) >= WGOAL_BUSY_MS) {
                const char *a, *b;
                size_t al, bl;
                count = editor_doc_text_parts(d, &a, &al, &b, &bl) <= WGOAL_BUSY_MAX_LEN;
            }
        }
        if (count) {
            t->last = wg_count_words(d);
            t->gen_counted = t->gen_seen = gen;
            t->count_ms = t->seen_ms = now;
        }
        return;
    }
    /* Not tracked yet (opened, or replaced as a whole, since the goal
     * started): its current words are the baseline. */
    for (int i = 0; i < WGOAL_TRACK_MAX; i++) {
        wg_track_t *t = &s_wg_track[i];
        if (t->doc || ctx->seen[i]) continue;
        t->doc = d;
        t->epoch = epoch;
        t->base = t->last = wg_count_words(d);
        t->gen_counted = t->gen_seen = gen;
        t->count_ms = t->seen_ms = now;
        ctx->seen[i] = true;
        return;
    }
}

/* Bring the per-document counts up to date (force: recount every
 * changed document now) and drop the documents no longer open,
 * keeping their words in s_wg_frozen. */
static void wg_scan(bool force)
{
    wg_scan_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.force = force;
    editor_doc_foreach(wg_scan_doc, &ctx);
    for (int i = 0; i < WGOAL_TRACK_MAX; i++) {
        wg_track_t *t = &s_wg_track[i];
        if (!t->doc || ctx.seen[i]) continue;
        s_wg_frozen += (int64_t)t->last - (int64_t)t->base;
        t->doc = NULL;
    }
}

static uint32_t wg_progress(void)
{
    int64_t p = s_wg_frozen;
    for (int i = 0; i < WGOAL_TRACK_MAX; i++)
        if (s_wg_track[i].doc)
            p += (int64_t)s_wg_track[i].last - (int64_t)s_wg_track[i].base;
    return p > 0 ? (uint32_t)p : 0;
}

static void wg_reset(void)
{
    memset(s_wg_track, 0, sizeof(s_wg_track));
    s_wg_frozen = 0;
}

static int sprint_title_words(void)
{
    if (!s_pt_show_words || s_pt_state == PTIMER_IDLE) return -1;
    uint32_t p = wg_progress();
    return p > 999999u ? 999999 : (int)p;
}

/* Put the current sprint word count (or none) into the title bar. */
static void ptimer_update_title(void)
{
    int w = sprint_title_words();
    if (w == s_pt_title_words) return;
    s_pt_title_words = w;
    if (!s_lbl_title) return;
    pane_bind_focus();
    update_title_bar();
    ptimer_flush_now();
}

/* Position and size the line for the current state. */
static void ptimer_update_bar(void)
{
    if (!s_pt_bar) return;
    if (s_pt_state == PTIMER_IDLE) {
        lv_obj_add_flag(s_pt_bar, LV_OBJ_FLAG_HIDDEN);
        s_pt_bar_h = -1;
        return;
    }
    int h = SCR_H;
    if (s_pt_state == PTIMER_RUNNING && s_pt_kind == PT_KIND_TIMER) {
        uint32_t el = lv_tick_elaps(s_pt_start_ms);
        if (el > s_pt_total_ms) el = s_pt_total_ms;
#if PTIMER_STEP_MS > 0
        if (el < s_pt_total_ms) el -= el % PTIMER_STEP_MS;
#endif
        h = (int)((int64_t)SCR_H * el / s_pt_total_ms);
    } else if (s_pt_state == PTIMER_RUNNING) {
        uint32_t p = wg_progress();
        if (p > s_wg_target) p = s_wg_target;
#if WGOAL_STEPS > 0
        if (p < s_wg_target)
            p = (uint32_t)((uint64_t)p * WGOAL_STEPS / s_wg_target *
                           s_wg_target / WGOAL_STEPS);
#endif
        h = (int)((int64_t)SCR_H * p / s_wg_target);
    }
    bool show = (s_pt_state == PTIMER_RUNNING) ? true : s_pt_bar_on;
    bool hidden = lv_obj_has_flag(s_pt_bar, LV_OBJ_FLAG_HIDDEN);
    if (h == s_pt_bar_h && show == !hidden) return;
    s_pt_bar_h = h;
    int sw = ptimer_strip_width();
    lv_obj_set_pos(s_pt_bar, SCR_W - sw, 0);
    lv_obj_set_size(s_pt_bar, sw, h > 0 ? h : 1);
    if (show && h > 0) lv_obj_remove_flag(s_pt_bar, LV_OBJ_FLAG_HIDDEN);
    else               lv_obj_add_flag(s_pt_bar, LV_OBJ_FLAG_HIDDEN);
    ptimer_flush_now();
}

/* (Re)create the line on the top layer in the current theme; called
 * by build_screens(), also after a color-theme change. */
static void ptimer_build_bar(void)
{
    if (s_pt_bar) lv_obj_delete(s_pt_bar);
    s_pt_bar = lv_obj_create(lv_layer_top());
    lv_obj_set_style_bg_color(s_pt_bar, theme_fg(), 0);
    lv_obj_set_style_bg_opa(s_pt_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_pt_bar, 0, 0);
    lv_obj_set_style_radius(s_pt_bar, 0, 0);
    lv_obj_set_style_pad_all(s_pt_bar, 0, 0);
    lv_obj_remove_flag(s_pt_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(s_pt_bar, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(s_pt_bar, LV_OBJ_FLAG_HIDDEN);
    s_pt_bar_h = -1;
    ptimer_update_bar();
}

/* End the timer or word goal (after blinking, or "Stop ..."): hide
 * the line, give the strip back to the editor and file browser, allow
 * idle sleep again. */
static void ptimer_stop(void)
{
    if (s_pt_state == PTIMER_IDLE) return;
    s_pt_state = PTIMER_IDLE;
    if (s_pt_timer) {
        lv_timer_delete(s_pt_timer);
        s_pt_timer = NULL;
    }
    wg_reset();
    standby_set_idle_inhibit(false);
    if (s_pt_bar) lv_obj_add_flag(s_pt_bar, LV_OBJ_FLAG_HIDDEN);
    s_pt_bar_h = -1;
    s_ptimer_strip = 0;
    ptimer_relayout();
    s_pt_title_words = -1;
    ptimer_flush_now();
}

static void ptimer_begin_blink(void)
{
    s_pt_state = PTIMER_BLINKING;
    s_pt_blink_start_ms = lv_tick_get();
    s_pt_bar_on = true;
    lv_timer_set_period(s_pt_timer, PTIMER_BLINK_PERIOD_MS);
}

static void ptimer_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (s_pt_state == PTIMER_RUNNING) {
        wg_scan(false);
        ptimer_update_title();
        if (s_pt_kind == PT_KIND_TIMER) {
            if (lv_tick_elaps(s_pt_start_ms) >= s_pt_total_ms)
                ptimer_begin_blink();
        } else {
            uint32_t p = wg_progress();
            if (p >= s_wg_target) {
                char msg[48];
                snprintf(msg, sizeof(msg), "Word goal reached: %u words",
                         (unsigned)p);
                editor_ui_set_status(msg);
                ptimer_begin_blink();
            }
        }
        ptimer_update_bar();
        return;
    }
    if (s_pt_state == PTIMER_BLINKING) {
        if (lv_tick_elaps(s_pt_blink_start_ms) >= PTIMER_BLINK_MS) {
            ptimer_stop();
            return;
        }
        s_pt_bar_on = !s_pt_bar_on;
        ptimer_update_bar();
    }
}

/* Start (or restart) a timer of value minutes, or a word goal of value
 * words. Replaces whichever of the two was running. */
static void ptimer_start(pt_kind_t kind, uint32_t value)
{
    bool had_strip = (s_ptimer_strip > 0);
    s_pt_state    = PTIMER_RUNNING;
    s_pt_kind     = kind;
    s_pt_start_ms = lv_tick_get();
    s_pt_bar_on   = true;
    s_pt_bar_h    = -1;
    wg_reset();
    wg_scan(true);                /* the baselines */
    if (kind == PT_KIND_TIMER) s_pt_total_ms = value * 60u * 1000u;
    else                       s_wg_target = value;
    if (s_pt_timer) lv_timer_set_period(s_pt_timer, PTIMER_TICK_MS);
    else            s_pt_timer = lv_timer_create(ptimer_timer_cb, PTIMER_TICK_MS, NULL);
    standby_set_idle_inhibit(true);
    if (!had_strip) {
        s_ptimer_strip = ptimer_strip_width();
        ptimer_relayout();
    }
    char msg[48];
    if (kind == PT_KIND_TIMER)
        snprintf(msg, sizeof(msg), "Timer: %u minutes", (unsigned)value);
    else
        snprintf(msg, sizeof(msg), "Word goal: %u words", (unsigned)value);
    editor_ui_set_status(msg);
    s_pt_title_words = -2;        /* force the title update */
    ptimer_update_title();
    if (s_pt_show_words) bars_show();
    ptimer_update_bar();
}

static void refresh_ptimer_prompt(void)
{
    if (!s_pt_panel) return;
    if (s_pt_opt_lbl[PTIMER_ROW_WORDS]) {
        bool editing = (s_pt_sel == PTIMER_ROW_WORDS);
        lv_label_set_text_fmt(s_pt_opt_lbl[PTIMER_ROW_WORDS],
                              editing ? "Word goal: %s_ words"
                                      : "Word goal: %s words",
                              s_wg_buf);
    }
    if (s_pt_opt_lbl[PTIMER_ROW_WCOUNT])
        lv_label_set_text(s_pt_opt_lbl[PTIMER_ROW_WCOUNT],
                          s_pt_show_words ? "[x] Show word count"
                                          : "[ ] Show word count");
    for (int i = 0; i < PTIMER_ROW_MAX; i++) {
        if (!s_pt_opt_lbl[i]) continue;
        if (i >= s_pt_rows) {
            lv_obj_add_flag(s_pt_opt_lbl[i], LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        lv_obj_remove_flag(s_pt_opt_lbl[i], LV_OBJ_FLAG_HIDDEN);
        bool sel = (i == s_pt_sel);
        lv_obj_set_style_bg_color(s_pt_opt_lbl[i], sel ? theme_fg() : theme_bg(), 0);
        lv_obj_set_style_bg_opa(s_pt_opt_lbl[i], LV_OPA_COVER, 0);
        lv_obj_set_style_text_color(s_pt_opt_lbl[i], sel ? theme_bg() : theme_fg(), 0);
    }
}

static void show_ptimer_prompt(void)
{
    if (!s_pt_panel) return;
    s_pt_open = true;
    s_pt_rows = (s_pt_state == PTIMER_IDLE) ? PTIMER_ROW_STOP : PTIMER_ROW_MAX;
    s_pt_sel = 1;
    for (int i = 0; i < PTIMER_OPT_COUNT; i++)
        if (PTIMER_OPTIONS[i] == s_pt_minutes) s_pt_sel = i;
    if (s_pt_last_kind == PT_KIND_WORDS) s_pt_sel = PTIMER_ROW_WORDS;
    snprintf(s_wg_buf, sizeof(s_wg_buf), "%u", (unsigned)s_wg_goal);
    s_wg_fresh = true;

    char hdr[64];
    if (s_pt_state == PTIMER_RUNNING && s_pt_kind == PT_KIND_TIMER) {
        uint32_t el = lv_tick_elaps(s_pt_start_ms);
        uint32_t left = (el < s_pt_total_ms) ? s_pt_total_ms - el : 0;
        snprintf(hdr, sizeof(hdr), "Timer: %u min left (Enter/Esc):",
                 (unsigned)((left + 59999) / 60000));
    } else if (s_pt_state == PTIMER_RUNNING) {
        wg_scan(true);
        snprintf(hdr, sizeof(hdr), "Words: %u of %u (Enter/Esc):",
                 (unsigned)wg_progress(), (unsigned)s_wg_target);
    } else {
        snprintf(hdr, sizeof(hdr), "Start a writing sprint (Enter/Esc):");
    }
    lv_label_set_text(s_pt_hdr_lbl, hdr);
    if (s_pt_opt_lbl[PTIMER_ROW_STOP])
        lv_label_set_text(s_pt_opt_lbl[PTIMER_ROW_STOP],
                          s_pt_kind == PT_KIND_WORDS ? "Stop the word goal"
                                                     : "Stop the timer");

    int panel_h = overlay_row_y() + s_pt_rows * overlay_row_pitch() + 2 * OVERLAY_PAD;
    lv_obj_set_size(s_pt_panel, CONTENT_W - 20, panel_h);
    lv_obj_set_pos(s_pt_panel, 10, (SCR_H - panel_h) / 2);
    lv_obj_set_parent(s_pt_panel, lv_scr_act());
    lv_obj_remove_flag(s_pt_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_pt_panel);
    refresh_ptimer_prompt();
}

static void close_ptimer_prompt(void)
{
    s_pt_open = false;
    if (s_pt_panel) lv_obj_add_flag(s_pt_panel, LV_OBJ_FLAG_HIDDEN);
}

static void ptimer_prompt_activate(void)
{
    int sel = s_pt_sel;
    if (sel == PTIMER_ROW_WCOUNT) {
        /* A checkbox: toggle it and keep the prompt open. */
        s_pt_show_words = !s_pt_show_words;
        save_ptimer_to_nvs();
        refresh_ptimer_prompt();
        ptimer_update_title();
        if (s_pt_show_words && s_pt_state != PTIMER_IDLE) bars_show();
        return;
    }
    if (sel == PTIMER_ROW_WORDS) {
        unsigned long v = strtoul(s_wg_buf, NULL, 10);
        if (v < 1 || v > WGOAL_MAX) {
            editor_ui_set_status("Word goal: type a number of words");
            return;                    /* keep the prompt open */
        }
        close_ptimer_prompt();
        s_wg_goal = (uint32_t)v;
        s_pt_last_kind = PT_KIND_WORDS;
        save_ptimer_to_nvs();
        ptimer_start(PT_KIND_WORDS, s_wg_goal);
        return;
    }
    close_ptimer_prompt();
    if (sel == PTIMER_ROW_STOP) {
        bool words = (s_pt_kind == PT_KIND_WORDS);
        ptimer_stop();
        editor_ui_set_status(words ? "Word goal stopped" : "Timer stopped");
        return;
    }
    s_pt_minutes = PTIMER_OPTIONS[sel];
    s_pt_last_kind = PT_KIND_TIMER;
    save_ptimer_to_nvs();
    ptimer_start(PT_KIND_TIMER, s_pt_minutes);
}

/* The digit a key types, from its HID usage (layout independent: the
 * number row and the keypad), or 0. */
static char ptimer_key_digit(const kb_event_t *ev)
{
    if (ev->modifier & (KB_MOD_LCTRL | KB_MOD_RCTRL | KB_MOD_LALT |
                        KB_MOD_RALT | KB_MOD_LGUI | KB_MOD_RGUI))
        return 0;
    uint8_t k = ev->keycode;
    if (k >= 0x1E && k <= 0x26) return (char)('1' + (k - 0x1E));
    if (k == 0x27) return '0';
    if (k >= 0x59 && k <= 0x61) return (char)('1' + (k - 0x59));
    if (k == 0x62) return '0';
    return 0;
}

static void handle_ptimer_prompt_key(const kb_event_t *ev)
{
    char digit = ptimer_key_digit(ev);
    if (digit) {
        /* Typing a number edits the word goal, from any row. */
        if (s_pt_sel != PTIMER_ROW_WORDS) {
            s_pt_sel = PTIMER_ROW_WORDS;
            s_wg_fresh = true;
        }
        if (s_wg_fresh) s_wg_buf[0] = '\0';
        s_wg_fresh = false;
        size_t n = strlen(s_wg_buf);
        if (n == 1 && s_wg_buf[0] == '0') n = 0;   /* no leading zero */
        if (n < WGOAL_DIGITS) {
            s_wg_buf[n] = digit;
            s_wg_buf[n + 1] = '\0';
        }
        refresh_ptimer_prompt();
        return;
    }
    switch (ev->keycode) {
    case KB_KEY_BACKSPACE:
        if (s_pt_sel == PTIMER_ROW_WORDS) {
            size_t n = strlen(s_wg_buf);
            if (n) s_wg_buf[n - 1] = '\0';
            s_wg_fresh = false;
            refresh_ptimer_prompt();
        }
        return;
    case KB_KEY_UP:
        s_pt_sel = (s_pt_sel + s_pt_rows - 1) % s_pt_rows;
        s_wg_fresh = true;
        refresh_ptimer_prompt();
        return;
    case KB_KEY_DOWN:
        s_pt_sel = (s_pt_sel + 1) % s_pt_rows;
        s_wg_fresh = true;
        refresh_ptimer_prompt();
        return;
    case KB_KEY_ENTER:
        ptimer_prompt_activate();
        return;
    case KB_KEY_SPACE:
        if (s_pt_sel == PTIMER_ROW_WCOUNT) ptimer_prompt_activate();
        return;
    case KB_KEY_ESCAPE:
        close_ptimer_prompt();
        return;
    default:
        break;
    }
}

/* ---- Delete-file confirmation logic ---- */

static void refresh_delete_prompt(void)
{
    if (!s_del_panel) return;
    for (int i = 0; i < DELCONF_COUNT; i++) {
        if (!s_del_opt_lbl[i]) continue;
        bool sel = (i == s_del_sel);
        lv_obj_set_style_bg_color(s_del_opt_lbl[i], sel ? theme_fg() : theme_bg(), 0);
        lv_obj_set_style_bg_opa(s_del_opt_lbl[i], LV_OPA_COVER, 0);
        lv_obj_set_style_text_color(s_del_opt_lbl[i], sel ? theme_bg() : theme_fg(), 0);
    }
}

static void close_delete_prompt(void)
{
    s_del_open = false;
    if (s_del_panel) lv_obj_add_flag(s_del_panel, LV_OBJ_FLAG_HIDDEN);
}

/* Remember the row after (or before) the selected one, so the
 * highlight stays in place after a deletion. */
static void remember_neighbour_row(void)
{
    s_del_next_name[0] = '\0';
    int rows = (int)lv_obj_get_child_count(s_browser_list);
    int next = (s_browser_sel + 1 < rows) ? s_browser_sel + 1 : s_browser_sel - 1;
    lv_obj_t *nbtn = (next >= 0) ? lv_obj_get_child(s_browser_list, next) : NULL;
    if (nbtn) {
        int nidx = (int)(intptr_t)lv_obj_get_user_data(nbtn);
        if (nidx >= 0 && nidx < s_browser_count) {
            strncpy(s_del_next_name, s_browser_entries[nidx].name, sizeof(s_del_next_name) - 1);
            s_del_next_name[sizeof(s_del_next_name) - 1] = '\0';
        }
    }
}

static void open_delete_panel(const char *hdr)
{
    lv_label_set_text(s_del_hdr_lbl, hdr);
    s_del_open = true;
    s_del_sel = DELCONF_CANCEL;
    lv_obj_set_parent(s_del_panel, lv_scr_act());
    lv_obj_remove_flag(s_del_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_del_panel);
    refresh_delete_prompt();
}

/* Del / Alt+D in the list of projects: a project may be deleted only
 * when everything in it is on the Git server (or it has no files). */
static void show_project_delete_prompt(const char *name)
{
    char dir[64];
    project_dir(name, dir, sizeof(dir));
    if (editor_dir_has_open_doc(dir)) {
        editor_ui_set_status("A file of this project is open -- close it first");
        return;
    }
    git_sync_project_status_t st;
    git_sync_project_status(dir, &st);
    if (st.busy) {
        editor_ui_set_status("Delete: wait for Git sync to finish");
        return;
    }
    if (st.files > 0 || st.configured) {
        if (!st.configured) {
            editor_ui_set_status("Project not synced with Git (no git.cfg) -- not deleted");
            return;
        }
        if (st.error) {
            editor_ui_set_status("Delete: cannot check Git status");
            return;
        }
        if (st.files > 0 && (st.never_pushed || st.not_pushed || st.modified)) {
            editor_ui_set_status("Project not synchronized -- sync first (Ctrl+G)");
            return;
        }
    }
    s_del_is_project = true;
    snprintf(s_del_path, sizeof(s_del_path), "%s", dir);
    remember_neighbour_row();
    char hdr[96];
    if (st.files > 0)
        snprintf(hdr, sizeof(hdr), "Delete project %.32s? (kept in Git)", name);
    else
        snprintf(hdr, sizeof(hdr), "Delete project %.32s?", name);
    open_delete_panel(hdr);
}

/* Del / Alt+D in the file browser: confirm deleting the selected file,
 * provided Git sync already holds its current content. */
static void show_delete_prompt(void)
{
    if (!s_del_panel || !s_browser_list) return;
    const sd_card_file_entry_t *e = browser_selected_entry();
    if (!e) return;
    if (const sd_card_file_entry_t *bk = browser_selected_book()) {
        /* Books are never in Git: confirm and delete. */
        char bdir[64];
        browser_dir(bdir, sizeof(bdir));
        s_del_is_project = false;
        snprintf(s_del_path, sizeof(s_del_path), "%s/%s", bdir, bk->name);
        remember_neighbour_row();
        char hdr[96];
        snprintf(hdr, sizeof(hdr), "Delete the book %.40s? (cannot be undone)", bk->name);
        open_delete_panel(hdr);
        return;
    }
    if (browser_at_root()) {
        show_project_delete_prompt(e->name);
        return;
    }
    if (e->is_dir) return;
    const char *name = e->name;
    char dir[64];
    project_dir(s_cur_project, dir, sizeof(dir));

    switch (git_sync_file_status(dir, name)) {
    case GIT_SYNC_FILE_PUSHED:
        break;
    case GIT_SYNC_FILE_NOT_CONFIGURED:
        editor_ui_set_status("Delete needs Git sync (git.cfg in the project)");
        return;
    case GIT_SYNC_FILE_BUSY:
        editor_ui_set_status("Delete: wait for Git sync to finish");
        return;
    case GIT_SYNC_FILE_NOT_PUSHED:
        editor_ui_set_status("Not on the Git server yet -- sync first (Ctrl+G)");
        return;
    case GIT_SYNC_FILE_MODIFIED:
        editor_ui_set_status("Changed since last push -- sync first (Ctrl+G)");
        return;
    default:
        editor_ui_set_status("Delete: cannot check Git status");
        return;
    }

    s_del_is_project = false;
    snprintf(s_del_path, sizeof(s_del_path), "%s/%s", dir, name);
    remember_neighbour_row();
    char hdr[96];
    snprintf(hdr, sizeof(hdr), "Delete %.40s? (kept in Git history)", name);
    open_delete_panel(hdr);
}

static void delete_prompt_activate(void)
{
    close_delete_prompt();
    if (s_del_sel != DELCONF_DELETE) {
        restore_default_status();
        return;
    }
    const char *slash = strrchr(s_del_path, '/');
    const char *name = slash ? slash + 1 : s_del_path;
    char msg[80];
    if (s_del_is_project) {
        project_name_t pname;
        if (snprintf(pname, sizeof(pname), "%s", name) >= (int)sizeof(pname)) {
            editor_ui_set_status("Project name too long");
            return;
        }
        if (editor_dir_has_open_doc(s_del_path)) {
            editor_ui_set_status("A file of this project is open -- close it first");
            return;
        }
        esp_err_t err = project_delete(pname);
        if (strcmp(s_last_project, pname) == 0) {
            s_last_project[0] = '\0';
            projects_set_last("");
        }
        refresh_file_list(s_del_next_name[0] ? s_del_next_name : NULL);
        if (err != ESP_OK) {
            editor_ui_set_status("Delete failed -- some files may remain");
            return;
        }
        snprintf(msg, sizeof(msg), "Deleted project %.32s", pname);
        editor_ui_set_status(msg);
        if (!project_any())
            show_name_prompt(SAVE_MODE_PROJECT_FIRST, "Create a project (Enter):",
                             PROJECT_DEFAULT_NAME);
        return;
    }
    esp_err_t err = editor_delete_file(s_del_path);
    if (err == ESP_ERR_INVALID_STATE) {
        editor_ui_set_status("File is open in a pane -- close it first");
        return;
    }
    if (err != ESP_OK) {
        editor_ui_set_status("Delete failed!");
        return;
    }
    snprintf(msg, sizeof(msg), "Deleted %.40s", name);
    refresh_file_list(s_del_next_name[0] ? s_del_next_name : NULL);
    editor_ui_set_status(msg);
}

static void handle_delete_prompt_key(const kb_event_t *ev)
{
    switch (ev->keycode) {
    case KB_KEY_UP:
    case KB_KEY_DOWN:
        s_del_sel = (s_del_sel == DELCONF_DELETE) ? DELCONF_CANCEL : DELCONF_DELETE;
        refresh_delete_prompt();
        return;
    case KB_KEY_ENTER:
        delete_prompt_activate();
        return;
    case KB_KEY_ESCAPE:
        s_del_sel = DELCONF_CANCEL;
        delete_prompt_activate();
        return;
    default:
        return;
    }
}

/* ---- Exit / sleep prompt overlay ----
 * Asks whether to save, proceed without saving, or cancel when the user
 * tries to leave the editor (Esc -> browser, Ctrl+P -> sleep) with
 * unsaved changes. */

static void refresh_exit_prompt(void)
{
    if (!s_exit_panel) return;
    for (int i = 0; i < EXIT_OPT_COUNT; i++) {
        if (!s_exit_opt_lbl[i]) continue;
        bool sel = (i == s_exit_sel);
        /* Invert colors on the selected option so the highlight is
         * visible on the 1-bpp reflective / e-paper panels. */
        lv_obj_set_style_bg_color(s_exit_opt_lbl[i],
                                  sel ? theme_fg() : theme_bg(), 0);
        lv_obj_set_style_bg_opa(s_exit_opt_lbl[i], LV_OPA_COVER, 0);
        lv_obj_set_style_text_color(s_exit_opt_lbl[i],
                                    sel ? theme_bg() : theme_fg(), 0);
    }
}

static void close_exit_prompt(void)
{
    s_exit_open = false;
    if (s_exit_panel) lv_obj_add_flag(s_exit_panel, LV_OBJ_FLAG_HIDDEN);
}

/* Point the overlay's rows at `purpose`, relabel them, and show it. */
static void show_leave_prompt(exit_purpose_t purpose)
{
    if (!s_exit_panel) return;
    s_exit_purpose = purpose;
    const char *const *labels =
        (purpose == EXIT_PURPOSE_SLEEP) ? SLEEP_OPT_LABELS : EXIT_OPT_LABELS;
    const char *hdr = (purpose == EXIT_PURPOSE_SLEEP)
                          ? "Unsaved changes -- sleep anyway?"
                          : "Unsaved changes (Up/Down + Enter):";
#if defined(CONFIG_DRAFTLING_HAS_QUIT)
    if (purpose == EXIT_PURPOSE_QUIT) {
        labels = QUIT_OPT_LABELS;
        hdr = "Unsaved changes -- quit anyway?";
    }
#endif
    for (int i = 0; i < EXIT_OPT_COUNT; i++) {
        if (s_exit_opt_lbl[i]) lv_label_set_text(s_exit_opt_lbl[i], labels[i]);
    }
    if (s_exit_hdr_lbl) lv_label_set_text(s_exit_hdr_lbl, hdr);
    s_exit_open = true;
    s_exit_sel  = EXIT_OPT_SAVE;
    lv_obj_remove_flag(s_exit_panel, LV_OBJ_FLAG_HIDDEN);
    refresh_exit_prompt();
}

static void show_exit_prompt(void)  { show_leave_prompt(EXIT_PURPOSE_BROWSER); }
static void show_sleep_prompt(void) { show_leave_prompt(EXIT_PURPOSE_SLEEP); }

/* Finish a leave-prompt action once the document is no longer dirty
 * (saved, or reloaded from disk by the discard path). */
static void leave_prompt_proceed(exit_purpose_t purpose)
{
#if defined(CONFIG_DRAFTLING_HAS_QUIT)
    if (purpose == EXIT_PURPOSE_QUIT) {
        standby_quit();
        return;
    }
#endif
    if (purpose == EXIT_PURPOSE_SLEEP) {
        standby_enter_sleep();
        /* Only reached on CONFIG_DRAFTLING_STANDBY_DISPLAY_OFF boards --
         * a real deep sleep never returns. The discard path may have
         * reloaded the document from disk while the display was off, so
         * wipe every pane's render cache and repaint from scratch. */
        invalidate_all_render_caches();
        editor_ui_refresh();
    } else {
        editor_ui_show_file_browser();
    }
}

static void exit_prompt_activate(void)
{
    exit_purpose_t purpose = s_exit_purpose;

    switch (s_exit_sel) {
    case EXIT_OPT_SAVE:
        close_exit_prompt();
        if (editor_get_file_path()) {
            /* Known filename -- save in place, then leave. */
            if (editor_save_file() == ESP_OK) {
                leave_prompt_proceed(purpose);
            } else {
                editor_ui_set_status("Save failed!");
            }
        } else {
            /* Untitled document -- ask for a filename first. The user
             * stays in the editor; once saved, repeating the action
             * (Esc / Ctrl+P) goes straight through. */
            show_save_prompt();
        }
        return;
    case EXIT_OPT_DISCARD:
        close_exit_prompt();
        if (purpose == EXIT_PURPOSE_SLEEP || purpose == EXIT_PURPOSE_QUIT) {
            /* Really drop the edits: reload every modified document from
             * disk so the standby pre-sleep auto-save has nothing to
             * write. (The Esc/browser path keeps the in-memory edits --
             * the document is still open, just not visible.) */
            editor_discard_all_changes();
        }
        leave_prompt_proceed(purpose);
        return;
    case EXIT_OPT_CANCEL:
    default:
        close_exit_prompt();
        editor_ui_set_status(
            "F1:Menu Ctrl+S:Save Ctrl+L:Layout Ctrl+G:Git Esc:Files");
        return;
    }
}

#if defined(CONFIG_DRAFTLING_TOUCHSCREEN)
/* A tap on an option row chooses it, like Up/Down + Enter. */
static void exit_opt_touch_cb(lv_event_t *e)
{
    if (!s_exit_open) return;
    s_exit_sel = (int)(intptr_t)lv_event_get_user_data(e);
    refresh_exit_prompt();
    exit_prompt_activate();
}
#endif

static void handle_exit_prompt_key(const kb_event_t *ev)
{
    switch (ev->keycode) {
    case KB_KEY_UP:
        s_exit_sel = (s_exit_sel + EXIT_OPT_COUNT - 1) % EXIT_OPT_COUNT;
        refresh_exit_prompt();
        return;
    case KB_KEY_DOWN:
        s_exit_sel = (s_exit_sel + 1) % EXIT_OPT_COUNT;
        refresh_exit_prompt();
        return;
    case KB_KEY_ENTER:
        exit_prompt_activate();
        return;
    case KB_KEY_ESCAPE:
        /* Esc inside the dialog cancels and keeps editing. */
        s_exit_sel = EXIT_OPT_CANCEL;
        exit_prompt_activate();
        return;
    default:
        break;
    }
}

/* ---- Search / Replace overlay logic ---- */

/* Render both fields and place the cursor bar in the active one. */
static void refresh_search_prompt(void)
{
    if (!s_search_panel) return;
    lv_label_set_text(s_search_find_lbl, s_search_buf);
    if (s_search_replace_mode) {
        lv_label_set_text(s_search_repl_lbl, s_replace_buf);
    }

    int cw = char_width_for_font(FONT_11);
    const char *active_buf = (s_search_field == 0) ? s_search_buf : s_replace_buf;
    int active_pos = (s_search_field == 0) ? s_search_pos : s_replace_pos;
    int chars = 0;
    for (int i = 0; i < active_pos; i++) {
        if ((active_buf[i] & 0xC0) != 0x80) chars++;
    }
    /* Field text labels are positioned just past the "F:" / "R:"
     * prefix inside the panel. */
    int cx = search_field_x() + chars * cw;
    /* Cursor sits at the right edge of the active field's text. */
    int cy = (s_search_field == 0) ? search_find_row_y() : search_repl_row_y();
    lv_obj_set_pos(s_search_cur, cx, cy);
    lv_obj_remove_flag(s_search_cur, LV_OBJ_FLAG_HIDDEN);

    /* Hide / show replace row based on the mode. */
    if (s_search_replace_mode) {
        lv_obj_remove_flag(s_search_repl_hdr, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(s_search_repl_lbl, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s_search_repl_hdr, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_search_repl_lbl, LV_OBJ_FLAG_HIDDEN);
    }

    lv_label_set_text(s_search_hdr_lbl,
                      s_search_replace_mode ? "Find / Replace:" : "Find:");
    lv_label_set_text(s_search_help_lbl,
                      s_search_replace_mode
                          ? "Enter:Next Ctrl+Enter:Replace Tab:Field Esc:Close"
                          : "Enter:Next  Esc:Close");
}

static void show_search_prompt(bool replace_mode)
{
    s_search_replace_mode = replace_mode;
    s_search_open = true;
    s_search_field = 0;
    /* Keep the previous query buffers so re-opening the dialog
     * remembers the user's last input. Clear stale match state
     * so Replace cannot operate on a stale range. */
    s_search_match_start = -1;
    s_search_match_end = -1;
    s_search_pos = (int)strlen(s_search_buf);
    s_replace_pos = (int)strlen(s_replace_buf);
    lv_obj_remove_flag(s_search_panel, LV_OBJ_FLAG_HIDDEN);
    refresh_search_prompt();
}

static void close_search_prompt(void)
{
    s_search_open = false;
    if (s_search_panel) lv_obj_add_flag(s_search_panel, LV_OBJ_FLAG_HIDDEN);
    /* Drop any selection from the most recent match so the editor
     * goes back to a normal cursor view. */
    editor_clear_selection();
    s_search_match_start = -1;
    s_search_match_end = -1;
    editor_ui_set_status(
        "F1:Menu Ctrl+S:Save Ctrl+L:Layout Ctrl+G:Git Esc:Files");
}

/* Find the next occurrence of s_search_buf starting just after the
 * current match (or from the cursor when there is none). On match,
 * move the cursor and select the matched range so the user can see
 * what was found. */
static void search_find_next(void)
{
    if (s_search_buf[0] == '\0') return;
    size_t from;
    if (s_search_match_start >= 0) {
        /* Advance past the previous match so consecutive Enter
         * presses cycle through results. */
        from = (size_t)s_search_match_start + 1;
    } else {
        from = editor_get_cursor();
    }
    int pos = editor_find(s_search_buf, from);
    if (pos < 0 && from > 0) {
        /* Wrap around to the start of the document. */
        pos = editor_find(s_search_buf, 0);
        if (pos >= 0) editor_ui_set_status("Search: wrapped to top");
    }
    if (pos < 0) {
        s_search_match_start = -1;
        s_search_match_end = -1;
        editor_ui_set_status("Search: not found");
        return;
    }
    size_t mlen = strlen(s_search_buf);
    s_search_match_start = pos;
    s_search_match_end = pos + (int)mlen;
    /* Position the cursor at the end of the match and anchor a
     * selection back to the start so the editor highlights the
     * matched text. */
    editor_clear_selection();
    editor_set_cursor((size_t)s_search_match_start);
    editor_set_selection_anchor();
    editor_set_cursor((size_t)s_search_match_end);
    ensure_cursor_visible();
    editor_ui_refresh();
    /* Re-show the dialog (the refresh above repaints the editor
     * underneath). */
    if (s_search_panel) lv_obj_move_foreground(s_search_panel);
    refresh_search_prompt();
}

static void search_replace_current(void)
{
    if (!s_search_replace_mode) return;
    if (s_append_only) {
        editor_ui_set_status("Append-only: replace disabled");
        return;
    }
    if (s_search_match_start < 0) {
        /* No active match -- treat the first Ctrl+Enter as Find Next. */
        search_find_next();
        return;
    }
    esp_err_t err = editor_replace_range((size_t)s_search_match_start,
                                         (size_t)s_search_match_end,
                                         s_replace_buf);
    if (err != ESP_OK) {
        editor_ui_set_status("Replace failed: buffer full");
        return;
    }
    /* Position the cursor right after the replacement and look
     * for the next match from there. */
    size_t after = (size_t)s_search_match_start + strlen(s_replace_buf);
    editor_clear_selection();
    editor_set_cursor(after);
    /* Reset match state so search_find_next() searches forward from
     * the cursor (we just consumed the previous match). */
    s_search_match_start = -1;
    s_search_match_end = -1;
    ensure_cursor_visible();
    editor_ui_refresh();
    if (s_search_panel) lv_obj_move_foreground(s_search_panel);
    /* Auto-advance to the next occurrence so the user can repeat
     * Ctrl+Enter through the document. */
    search_find_next();
}

/* Forward declaration for the editor_get_cursor helper used above
 * is provided by editor.h, which is already included at the top. */

static void handle_search_prompt_key(const kb_event_t *ev)
{
    bool ctrl  = (ev->modifier & (KB_MOD_LCTRL | KB_MOD_RCTRL)) != 0;

    /* Pointer to the active field's buffer + length cursor */
    char  *abuf = (s_search_field == 0) ? s_search_buf : s_replace_buf;
    int   *apos = (s_search_field == 0) ? &s_search_pos : &s_replace_pos;
    size_t bufsz = (s_search_field == 0) ? sizeof(s_search_buf) : sizeof(s_replace_buf);

    switch (ev->keycode) {
    case KB_KEY_ENTER:
        if (ctrl && s_search_replace_mode) {
            search_replace_current();
        } else {
            search_find_next();
        }
        return;
    case KB_KEY_ESCAPE:
        close_search_prompt();
        editor_ui_refresh();
        return;
    case KB_KEY_TAB:
        if (s_search_replace_mode) {
            s_search_field = (s_search_field == 0) ? 1 : 0;
        }
        break;
    default:
        /* Editing the find query invalidates the previous match. */
        if (text_field_key(abuf, bufsz, apos, ev) && s_search_field == 0) {
            s_search_match_start = -1;
            s_search_match_end = -1;
        }
        break;
    }
    refresh_search_prompt();
}

/* ---- Keyboard handler (registered as BLE callback) ---- */

#if defined(CONFIG_DRAFTLING_DISPLAY_EPD)
/* Information captured before a key event is processed, used by
 * try_partial_clip_for_typing() to decide whether the next e-paper
 * refresh can be narrowed to just the area around the typed
 * character. */
struct typing_pre_state_t {
    bool        valid;        /* false -> skip the fast path */
    int         line;         /* cursor line before the edit */
    int         col;          /* cursor column before the edit (UTF-8 chars) */
    int         cur_x;        /* absolute screen x of cursor before */
    int         cur_y;        /* absolute screen y of cursor before */
    int         cur_h;        /* cursor height before */
    bool        modified;     /* editor_is_modified() before */
    int         line_chars;   /* monospace cells of the line before */
    int         char_w;       /* monospace cell width at body font */
};

/* True for a line whose width the cell count does not give exactly:
 * Tamil, Devanagari, Bengali and Telugu, whose glyphs are proportional. The
 * typing fast path is not used on such lines (the whole line is
 * refreshed). */
static bool line_has_proportional_text(const char *lt, size_t ll)
{
#ifdef CONFIG_DRAFTLING_TALL_LINES
    /* Devanagari U+0900..U+097F is E0 A4 xx / E0 A5 xx in UTF-8,
     * Bengali U+0980..U+09FF E0 A6 xx / E0 A7 xx, Tamil U+0B80..U+0BFF
     * E0 AE xx / E0 AF xx, Telugu U+0C00..U+0C7F E0 B0 xx / E0 B1 xx. */
    for (size_t i = 0; i + 1 < ll; i++) {
        unsigned char c1 = (unsigned char)lt[i + 1];
        if ((unsigned char)lt[i] == 0xE0 &&
            ((c1 >= 0xA4 && c1 <= 0xA7) || (c1 >= 0xAE && c1 <= 0xB1)))
            return true;
    }
#else
    (void)lt;
    (void)ll;
#endif
    return false;
}

/* Capture pre-edit state for the fast partial-clip path. Fills *out
 * with valid=false when conditions disqualify the edit (selection
 * active, line is not a plain paragraph, etc.); subsequent
 * try_partial_clip_for_typing() calls then short-circuit. */
static void capture_typing_pre_state(typing_pre_state_t *out)
{
    out->valid = false;
    if (s_cursor == NULL) return;
    if (lv_obj_has_flag(s_cursor, LV_OBJ_FLAG_HIDDEN)) return;
    if (editor_selection_active()) return;

    int line, col;
    editor_get_cursor_pos(&line, &col);
    (void)col;
    int total = editor_get_line_count();
    if (line < 0 || line >= total) return;

    size_t ll = 0;
    const char *lt = editor_get_line(line, &ll);
    if (!lt) return;

    /* Single-line classification (no surrounding code-fence context).
     * We only fast-path lines whose visible glyphs are exactly the
     * raw text -- i.e. plain paragraphs and empty lines. Any line
     * inside a fenced code block, a heading/bullet/quote/etc. has a
     * styled rendering whose pixel layout we can't safely predict
     * here. So has a paragraph with inline spans: typing a closing
     * "*" restyles characters left of the cursor, outside the clip
     * (try_partial_clip_for_typing() re-checks the post-edit line).
     * The document's format decides (Fountain never qualifies: a
     * line's element depends on its neighbours). */
    if (!editor_format_active()->line_is_plain(lt, ll)) return;
    if (line_has_proportional_text(lt, ll)) return;

    int chars = utf8_cells_in_bytes(lt, ll);

    out->valid       = true;
    out->line        = line;
    out->col         = col;
    out->cur_x       = s_rp->x + lv_obj_get_x(s_cursor);
    out->cur_y       = s_rp->y + lv_obj_get_y(s_cursor);
    out->cur_h       = lv_obj_get_height(s_cursor);
    out->modified    = editor_is_modified();
    out->line_chars  = chars;
    out->char_w      = CHAR_W;
}

/* Compute and apply a one-shot panel-refresh clip rectangle for an
 * edit that affects only one body line and the cursor.  Returns true
 * if the clip was set, false if the post-edit state disqualifies the
 * fast path (caller should leave the dirty bbox un-clipped, which
 * yields today's full-line refresh behaviour). */
static bool try_partial_clip_for_typing(const typing_pre_state_t *pre)
{
    if (!pre->valid) return false;
    /* The fast path assumes only the focused pane's cursor line is
     * dirty, so it clips the panel refresh to that one line. While
     * split, a content edit also repaints the sibling pane (it may
     * share the same document and show the edited line at a different
     * position), dirtying pixels outside the clip. A clipped flush
     * would consume the whole dirty bbox but drive only the cursor
     * line, discarding the sibling pane's update and leaving stale
     * "overwritten" text behind. Disable the clip while split so the
     * full dirty bbox (covering both panes) is always refreshed. */
    if (s_pane_count > 1) return false;
    if (s_cursor == NULL) return false;
    if (lv_obj_has_flag(s_cursor, LV_OBJ_FLAG_HIDDEN)) return false;
    if (editor_selection_active()) return false;
    if (editor_is_modified() != pre->modified) {
        /* Modified-flag flip -> title-bar text changed -> the title
         * strip is also dirty and must be refreshed alongside the
         * line. Fall back to the full bbox. */
        return false;
    }

    int line, col;
    editor_get_cursor_pos(&line, &col);
    (void)col;
    if (line != pre->line) return false; /* edit crossed a line boundary */

    int total = editor_get_line_count();
    if (line < 0 || line >= total) return false;

    size_t ll = 0;
    const char *lt = editor_get_line(line, &ll);
    if (!lt) return false;

    if (!editor_format_active()->line_is_plain(lt, ll)) return false;
    if (line_has_proportional_text(lt, ll)) return false;

    int post_chars = utf8_cells_in_bytes(lt, ll);
    int char_w     = pre->char_w;
    int pre_w      = pre->line_chars * char_w;
    int post_w     = post_chars * char_w;
    int max_w      = pre_w > post_w ? pre_w : post_w;
    /* Bail if the line text wraps -- rendered_h would exceed line_h
     * and the dirty area covers multiple visual rows. */
    if (2 + max_w + 4 > s_rp->w) return false;

    int post_cur_x = s_rp->x + lv_obj_get_x(s_cursor);
    int post_cur_y = s_rp->y + lv_obj_get_y(s_cursor);
    int post_cur_h = lv_obj_get_height(s_cursor);
    /* Cursor moved to a different visual row -> the row geometry
     * changed (wrap or scroll), refresh the whole dirty bbox. */
    if (post_cur_y != pre->cur_y || post_cur_h != pre->cur_h) return false;

    /* X-extent of the changed pixels: from the leftmost-touched
     * column (min of pre/post cursor x) to the rightmost edge of the
     * line content (max of pre/post text width), plus the cursor's
     * own 2 px width on the right side. */
    int x_min = pre->cur_x < post_cur_x ? pre->cur_x : post_cur_x;
    int x_text_end = s_rp->x + 2 + max_w;
    int x_cur_end  = (pre->cur_x > post_cur_x ? pre->cur_x : post_cur_x) + 2;
    int x_max = x_text_end > x_cur_end ? x_text_end : x_cur_end;

    /* Pad by a few pixels on each side so antialiased / overhanging
     * glyph outlines (Greybeard is bitmapped, so this is mostly
     * paranoia) and the cursor bar's own 2 px width are always
     * included. */
    x_min -= 4;
    x_max += 4;
    if (x_min < s_rp->x) x_min = s_rp->x;
    if (x_max > s_rp->x + s_rp->w) x_max = s_rp->x + s_rp->w;
    if (x_max <= x_min) return false;

    /* x_min/cur_y/... are LVGL logical coordinates (pre-rotation);
     * display_set_partial_clip() intersects against a dirty bounding
     * box accumulated in physical panel coordinates, so map through
     * the same transform flush_cb() applies to every pushed tile.
     * Skipping this is invisible in the default landscape orientation
     * (logical == physical there) but silently drops every partial
     * refresh -- typing updates the off-screen framebuffer but never
     * reaches the panel -- as soon as a rotation (portrait, or a
     * non-zero base/flip rotation) is in effect. */
    int px, py, pw, ph;
    draftling_lvgl_port_map_to_physical(x_min, pre->cur_y,
                                        x_max - x_min, pre->cur_h,
                                        &px, &py, &pw, &ph);
    display_set_partial_clip(px, py, pw, ph);
    return true;
}
#endif /* CONFIG_DRAFTLING_DISPLAY_EPD */

#if defined(CONFIG_DRAFTLING_DISPLAY_EPD)
/* RAII guard that runs the partial-clip post-processing on every
 * return path of handle_editor_key (including the F1 / Ctrl+? / Esc
 * early returns). Without this, a typing event in one drain batch
 * could leave a narrow clip set when a subsequent navigation event
 * runs early-return -- the next flush would then refresh only the
 * old typing region and clip away everything else. */
struct ClipGuard {
    typing_pre_state_t pre_state;
    bool eligible;
    ClipGuard() : eligible(false) {
        capture_typing_pre_state(&pre_state);
    }
    ~ClipGuard() {
        bool clipped = false;
        if (eligible) clipped = try_partial_clip_for_typing(&pre_state);
        if (!clipped) display_set_partial_clip(0, 0, 0, 0);
    }
};
#endif /* CONFIG_DRAFTLING_DISPLAY_EPD */

/* Return true when the line containing the cursor is rendered
 * right-to-left.  We mirror LVGL's auto base-direction detection
 * (CONFIG_LV_BIDI_BASE_DIR_DEF_AUTO): scan the raw line UTF-8 and
 * return based on the first STRONG directional codepoint -- strong
 * RTL (Hebrew, Arabic, Hebrew/Arabic presentation forms) makes the
 * paragraph RTL; strong LTR (Latin, Greek, Cyrillic) makes it LTR;
 * neutrals (punctuation, spaces, digits, markdown markers) are
 * skipped.  Used to flip the Left / Right arrow keys on RTL lines
 * so that the visual right arrow moves the cursor to the visual
 * right (which is logically backward in an RTL paragraph). */
static bool cursor_line_is_rtl(void)
{
    int line, col;
    editor_get_cursor_pos(&line, &col);
    (void)col;
    size_t ll = 0;
    const char *lt = editor_get_line(line, &ll);
    if (!lt) return false;
    return utf8_first_strong_dir(lt, ll) < 0;
}

/* ---- Split-screen control ----
 *
 * The editor shows one pane by default. The split always divides the
 * display's long axis -- left/right in landscape, top/bottom in
 * portrait (Settings -> Display orientation) -- and is driven entirely
 * by three shortcuts:
 *   Ctrl+1  single pane
 *   Ctrl+2  two equal panes (first = 1/2)
 *   Ctrl+3  two panes with the first = 2/3; pressing Ctrl+3 again
 *           flips the first pane to 1/3 (and back).
 * "First" is the left pane in landscape, the top pane in portrait.
 * Enabling a split for the first time acquires a fresh, empty untitled
 * document into pane 1; collapsing back to a single pane keeps pane 1's
 * document open in the background so re-splitting restores it. Each
 * pane independently selects a file (the file browser targets the
 * focused pane, so each panel opens a file for itself); opening the
 * same path in both panes shares one
 * refcounted buffer so the two editors view / edit the same file. */
static void save_split_to_nvs(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS_EDITOR, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_u8(h, NVS_KEY_SPLIT, (uint8_t)s_split_mode);
        nvs_commit(h);
        nvs_close(h);
    }
}

static void load_split_from_nvs(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS_EDITOR, NVS_READONLY, &h) == ESP_OK) {
        uint8_t v = 0;
        if (nvs_get_u8(h, NVS_KEY_SPLIT, &v) == ESP_OK &&
            v <= (uint8_t)SPLIT_LEFT_1_3) {
            s_split_mode = (split_mode_t)v;
        }
        nvs_close(h);
    }
}

static void editor_ui_apply_split_mode(split_mode_t mode)
{
    bool want_split = (mode != SPLIT_NONE);

    if (want_split && !s_panes[1].doc) {
        editor_doc_t *d = editor_doc_acquire(NULL); /* empty untitled */
        if (!d) {
            editor_ui_set_status("Cannot split: no free document");
            pane_bind_focus();
            return;
        }
        s_panes[1].doc = d;
        s_panes[1].has_saved_view = false;   /* adopt the new doc's view */
    }

    s_split_mode = mode;
    s_pane_count = want_split ? 2 : 1;
    if (!want_split) s_focus = 0;
    if (s_focus >= s_pane_count) s_focus = s_pane_count - 1;

    save_split_to_nvs();

    layout_panes();
    invalidate_all_render_caches();
    pane_bind_focus();
    ensure_cursor_visible();
#if defined(CONFIG_DRAFTLING_DISPLAY_EPD)
    /* Changing the split layout moves / hides the divider and one pane.
     * On e-paper, force a clean full-screen repaint so the collapsed
     * pane's old borders / divider and any partially-refreshed regions
     * do not survive as ghosting (and a freshly-enabled pane is drawn in
     * full). No-op until the editor screen is on screen (e.g. the boot
     * restore below runs before the first lv_scr_load). */
    if (s_editor_screen_active) epd_full_screen_repaint();
#endif
    editor_ui_refresh();
}

/* Rendered height of line `line` of the bound pane's document: render
 * the pane from that line (without scrolling to the cursor) and read
 * the first slot. The caller restores the view and renders again. */
static int bars_measure_line(int line)
{
    editor_set_scroll_line(line);
    s_rp->top_px = 0;
    s_rp->lead_px = 0;
    s_no_autoscroll = true;
    refresh_active_pane(false);
    s_no_autoscroll = false;
    return s_prev_line_h[0] > 0 ? s_prev_line_h[0] : LINE_H;
}

/* Move the bound pane's text down by delta pixels relative to the pane
 * (up when negative), by whole pixels: lines above the scroll line
 * come in when it moves down, rows go out above the top when it moves
 * up. heights[] are the rendered heights of the lines from the scroll
 * line on, as last drawn. */
static void bars_shift_view(int delta, const int *heights, int n)
{
    int scroll = editor_get_scroll_line();
    int off = pane_lead_px() - pane_top_px() + delta;  /* origin of the scroll line */
    int lead = 0, top = 0;
    if (off > 0) {
        while (off > 0 && scroll > 0) {
            scroll--;
            off -= bars_measure_line(scroll);
        }
        if (off > 0) lead = off;
        else         top = -off;
    } else {
        top = -off;
        for (int i = 0; i < n && heights[i] > 0 && top >= heights[i]; i++) {
            top -= heights[i];
            scroll++;
        }
    }
    editor_set_scroll_line(scroll);
    s_rp->top_px      = top;
    s_rp->top_px_line = scroll;
    s_rp->top_px_doc  = s_rp->doc;
    s_rp->top_px_free = true;
    s_rp->lead_px     = lead;
    s_rp->lead_line   = scroll;
    s_rp->lead_doc    = s_rp->doc;
}

/* The bars were hidden or shown: resize the panes to the new editor
 * body (EDITOR_Y / EDITOR_H), shift each pane's view so the text stays
 * where it is on screen, and redraw. When the body shrinks, a cursor
 * now below the last visible row is scrolled up to it
 * (refresh_active_pane() adds more if wrapped lines still push it off
 * the pane). */
static void bars_relayout(void)
{
    if (!s_scr) return;
    int old_y[EDITOR_MAX_PANES];
    int heights[EDITOR_MAX_PANES][MAX_LINE_LABELS];
    for (int p = 0; p < s_pane_count; p++) {
        old_y[p] = s_panes[p].y;
        for (int i = 0; i < MAX_LINE_LABELS; i++)
            heights[p][i] = s_panes[p].prev_line_visible[i]
                          ? s_panes[p].prev_line_h[i] : 0;
    }
    layout_panes();
    for (int p = 0; p < s_pane_count; p++) {
        pane_bind(p);
        int delta = old_y[p] - s_panes[p].y;
        if (delta != 0 && s_editor_screen_active)
            bars_shift_view(delta, heights[p], MAX_LINE_LABELS);
        invalidate_render_cache();
        int cur_line, cur_col;
        editor_get_cursor_pos(&cur_line, &cur_col);
        (void)cur_col;
        int scroll = editor_get_scroll_line();
        if (cur_line >= scroll + VISIBLE_LINES) {
            editor_set_scroll_line(cur_line - VISIBLE_LINES + 1);
        }
    }
    pane_bind_focus();
    editor_ui_refresh();
}

/* Ctrl+3: enter / cycle the asymmetric split. First press (from single
 * or equal split) makes the first pane 2/3; a subsequent Ctrl+3 toggles
 * the first pane between 2/3 and 1/3. (First pane = left in landscape,
 * top in portrait.) */
static void editor_ui_cycle_wide_split(void)
{
    split_mode_t next = (s_split_mode == SPLIT_LEFT_2_3)
                            ? SPLIT_LEFT_1_3 : SPLIT_LEFT_2_3;
    editor_ui_apply_split_mode(next);
}

/* Ctrl+Tab: move keyboard focus to the other pane (only meaningful
 * while split). */
static void editor_ui_focus_other_pane(void)
{
    if (s_pane_count < 2) return;
    s_focus = (s_focus + 1) % s_pane_count;
    pane_bind_focus();
    ensure_cursor_visible();
    editor_ui_refresh();
}

/* Open `path` (NULL / empty = a new untitled document) into pane
 * `target` through the document pool. A path already open in the other
 * pane shares the same refcounted buffer. Releases whatever document
 * the target pane held previously. Returns the acquired document, or
 * NULL on failure. On failure the engine is left with a valid active
 * document; the target pane keeps its previous document when a slot was
 * available, or is emptied if its previous document had to be released
 * first to make room (pool-exhausted retry path). */
static editor_doc_t *open_into_pane(int target, const char *path)
{
    if (target < 0 || target >= EDITOR_MAX_PANES) return NULL;
    editor_doc_t *prev = s_panes[target].doc;
    editor_doc_t *d = editor_doc_acquire(path);
    if (!d && prev) {
        /* The document pool is exhausted. With EDITOR_MAX_DOCS == 2 this
         * happens whenever both panes already hold distinct documents
         * and the user asks to open a third path into one of them: the
         * acquire above needs a free slot before this pane's previous
         * document is released. Release the target pane's previous
         * document first to free its slot, then retry the acquire. The
         * other pane's document is untouched. (If the requested path is
         * already open in the other pane, the first acquire would have
         * succeeded via the shared-buffer fast path and we never get
         * here.) */
        s_panes[target].doc = NULL;
        editor_doc_release(prev);
        prev = NULL;
        d = editor_doc_acquire(path);
        if (!d) {
            /* Open still failed (e.g. file too large / read error).
             * The target pane is now empty. editor_doc_acquire() already
             * restored a valid active document (pick_any_active) on the
             * open-failure path, so the engine is never left without
             * one. */
            return NULL;
        }
        s_panes[target].doc = d;
        s_panes[target].has_saved_view = false;   /* adopt the new doc's view */
        editor_set_active(d);
        return d;
    }
    if (!d) {
        if (prev) editor_set_active(prev);
        return NULL;
    }
    if (d == prev) {
        /* Re-acquired the document this pane already holds: acquire
         * bumped its refcount, so drop the extra reference to keep the
         * count balanced. The pane keeps its existing view. */
        editor_doc_release(d);
    } else if (prev) {
        editor_doc_release(prev);
    }
    if (d != prev) s_panes[target].has_saved_view = false;  /* adopt new view */
    s_panes[target].doc = d;
    editor_set_active(d);
    return d;
}

/* editor_doc_foreach callback: save a document's body if it has unsaved
 * changes and a path. Used by the Ctrl+G "save before git sync" path so
 * every open pane's edits reach disk before the sync task runs. */
static void save_modified_doc_cb(editor_doc_t *doc, void *ctx)
{
    (void)ctx;
    if (editor_doc_is_modified(doc) && editor_doc_get_path(doc)) {
        editor_doc_save(doc);
    }
}

#ifdef DRAFTLING_HAS_IME
/* ---- Input methods (Vietnamese / Korean / Japanese / Chinese packs) ----
 * The ime component composes text right before the cursor through
 * these two callbacks; candidates go to the status bar. */
static size_t ime_host_peek(char *buf, size_t max)
{
    return editor_peek_before(buf, max);
}

static bool ime_host_replace(size_t del, const char *text)
{
    return editor_replace_before(del, text);
}

static const ime_host_t s_ime_host = { ime_host_peek, ime_host_replace };
static bool s_ime_status_shown = false;

/* Mirror ime_status() into the status bar (it stays until the
 * composition ends, then the default text comes back). */
static void ime_sync_status(void)
{
    const char *st = ime_status();
    if (st) {
        set_status_with_timeout(st, 0);
        s_ime_status_shown = true;
    } else if (s_ime_status_shown) {
        s_ime_status_shown = false;
        restore_default_status();
    }
}

/* End any composition before a key the input method does not see. */
static void ime_finish(void)
{
    if (!ime_composing()) return;
    pane_bind_focus();
    ime_commit();
    ime_sync_status();
}
#endif

/* ---- Name-suggestion drop-down ----
 *
 * After a key that changed the focused document, the format's suggest
 * hook (Fountain: known Character names starting with the cue being
 * typed) is asked for completions, which drop down under the cursor.
 * While the list is shown Up / Down pick a name, Enter or Tab replaces
 * the typed text with it, and Esc closes the list until the cursor
 * goes to another line; any other key goes to the editor as usual and
 * the list follows the new text (or closes). */
static fmt_suggest_t s_sugg;
static bool   s_sugg_open = false;
static int    s_sugg_sel = 0;
static size_t s_sugg_dismissed = SIZE_MAX;   /* s_sugg.start of an Esc'd list */
static bool   s_sugg_kept = false;   /* the last editor key left the list open */

static void suggest_hide(void)
{
    if (!s_sugg_open) return;
    s_sugg_open = false;
    if (s_sugg_panel) lv_obj_add_flag(s_sugg_panel, LV_OBJ_FLAG_HIDDEN);
}

static void suggest_paint_selection(void)
{
    for (int i = 0; i < s_sugg.count; i++) {
        bool sel = (i == s_sugg_sel);
        lv_obj_set_style_bg_color(s_sugg_lbl[i], sel ? theme_fg() : theme_bg(), 0);
        lv_obj_set_style_text_color(s_sugg_lbl[i], sel ? theme_bg() : theme_fg(), 0);
    }
}

/* Lay the list out under (or, without room, above) the typed text in
 * the focused pane. Returns false when the cursor is not on screen. */
static bool suggest_place(void)
{
    pane_bind_focus();
    if (!s_sugg_panel || !s_cursor || !s_cursor_on_screen) return false;
    const lv_font_t *font = body_font();
    int cw = char_width_for_font(font);
    int lh = lv_font_get_line_height(font);
    const int pad = 2, border = 1;

    int cells = 0;
    for (int i = 0; i < FMT_SUGGEST_MAX; i++) {
        if (i < s_sugg.count) {
            int c = utf8_cells_in_bytes(s_sugg.names[i], strlen(s_sugg.names[i]));
            if (c > cells) cells = c;
            lv_label_set_text(s_sugg_lbl[i], s_sugg.names[i]);
            lv_obj_set_style_text_font(s_sugg_lbl[i], font, 0);
            lv_obj_set_pos(s_sugg_lbl[i], 0, i * lh);
            lv_obj_set_height(s_sugg_lbl[i], lh);
            lv_obj_remove_flag(s_sugg_lbl[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_sugg_lbl[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
    int inner_w = (cells + 1) * cw;
    for (int i = 0; i < s_sugg.count; i++) lv_obj_set_width(s_sugg_lbl[i], inner_w);
    int w = inner_w + 2 * (pad + border);
    int h = s_sugg.count * lh + 2 * (pad + border);
    if (w > CONTENT_W) w = CONTENT_W;

    /* The typed text ends at the cursor; the list starts under its
     * first character. */
    size_t cur = editor_get_cursor();
    size_t tl;
    const char *typed = NULL;
    {
        int line, col;
        editor_get_cursor_pos(&line, &col);
        size_t ls;
        typed = editor_get_line_off(line, &tl, &ls);
        if (s_sugg.start >= ls && cur >= s_sugg.start && cur - ls <= tl) {
            typed += s_sugg.start - ls;
            tl = cur - s_sugg.start;
        } else {
            tl = 0;
        }
    }
    int cx = s_rp->x + lv_obj_get_x(s_cursor);
    int cy = s_rp->y + lv_obj_get_y(s_cursor);
    int ch = lv_obj_get_height(s_cursor);
    int x = cx - (tl ? utf8_cells_in_bytes(typed, tl) * cw : 0) - pad - border;
    if (x + w > CONTENT_W) x = CONTENT_W - w;
    if (x < 0) x = 0;
    int y = cy + ch + 1;
    if (y + h > s_rp->y + s_rp->h && cy - h - 1 >= 0) y = cy - h - 1;

    lv_obj_set_size(s_sugg_panel, w, h);
    lv_obj_set_pos(s_sugg_panel, x, y);
    return true;
}

/* Re-evaluate the list after an editor key; typed says whether the key
 * changed the text. Returns true when the list was shown, hidden or
 * changed (the screen needs more than the typing fast path). */
static bool suggest_update(bool typed)
{
    bool was_open = s_sugg_open;
    const editor_format_t *f = editor_format_active();
    fmt_suggest_t sg;
    sg.count = 0;
    bool found = typed && s_name_suggest && f->suggest && f->suggest(&sg);
    if (!found || sg.start == s_sugg_dismissed) {
        /* An Esc'd list stays closed while its text is being typed. */
        if (!found && typed) s_sugg_dismissed = SIZE_MAX;
        suggest_hide();
        s_sugg_kept = false;
        return was_open;
    }
    s_sugg_dismissed = SIZE_MAX;
    /* Keep the highlighted name when it is still offered. */
    int sel = 0;
    if (was_open && s_sugg_sel < s_sugg.count) {
        for (int i = 0; i < sg.count; i++) {
            if (strcmp(sg.names[i], s_sugg.names[s_sugg_sel]) == 0) { sel = i; break; }
        }
    }
    s_sugg = sg;
    s_sugg_sel = sel;
    if (!suggest_place()) {
        suggest_hide();
        s_sugg_kept = false;
        return was_open;
    }
    suggest_paint_selection();
    lv_obj_remove_flag(s_sugg_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_sugg_panel);
    s_sugg_open = true;
    s_sugg_kept = true;
    return true;
}

/* Replace the typed text with the highlighted name. */
static void suggest_accept(void)
{
    const char *name = s_sugg.names[s_sugg_sel];
    size_t nl = strlen(name);
    suggest_hide();
    bool ok;
    if (s_append_only) {
        /* Nothing may be replaced: add the rest of the name. */
        ok = nl > s_sugg.len && editor_insert_text(name + s_sugg.len, nl - s_sugg.len);
    } else {
        ok = editor_replace_range(s_sugg.start, s_sugg.start + s_sugg.len, name) == ESP_OK;
    }
    if (!ok) editor_ui_set_status("Buffer full -- increase editor size in menuconfig");
    ensure_cursor_visible();
    editor_ui_refresh();
}

/* Keys the open list takes; returns false for every other key. */
static bool suggest_handle_key(const kb_event_t *ev)
{
    if (!s_sugg_open || ev->modifier != 0) return false;
    switch (ev->keycode) {
    case KB_KEY_UP:
        s_sugg_sel = (s_sugg_sel + s_sugg.count - 1) % s_sugg.count;
        suggest_paint_selection();
        s_sugg_kept = true;
        return true;
    case KB_KEY_DOWN:
        s_sugg_sel = (s_sugg_sel + 1) % s_sugg.count;
        suggest_paint_selection();
        s_sugg_kept = true;
        return true;
    case KB_KEY_ENTER:
    case KB_KEY_TAB:
        suggest_accept();
        return true;
    case KB_KEY_ESCAPE:
        s_sugg_dismissed = s_sugg.start;
        suggest_hide();
        return true;
    default:
        return false;
    }
}

static void handle_editor_key_main(const kb_event_t *ev);

/* Show another chapter of the book in the focused pane. */
static void book_show_chapter(int chapter, bool at_end)
{
    pane_bind_focus();
    editor_doc_t *doc = editor_get_active();
    if (editor_book_goto(chapter, at_end) != ESP_OK) {
        editor_ui_set_status(editor_book_error());
        return;
    }
    /* Every pane on this book adopts the new text's view. */
    for (int i = 0; i < s_pane_count; i++)
        if (s_panes[i].doc == doc && i != s_focus) s_panes[i].has_saved_view = false;
    pane_capture_view(s_rp);
#if defined(CONFIG_DRAFTLING_DISPLAY_EPD)
    if (s_pane_count > 1) epd_full_screen_repaint();
    else display_clear(0xFF);
#endif
    ensure_cursor_visible();
    editor_ui_refresh();
    char msg[160];
    const char *t = editor_book_chapter_title(chapter);
    snprintf(msg, sizeof(msg), "Chapter %d of %d%s%.100s", chapter + 1,
             editor_book_chapter_count(), t ? ": " : "", t ? t : "");
    set_status_with_timeout(msg, 4000);
}

/* Keys of the book reader (the focused document is a book). Returns
 * true when the key was handled here; false lets the editor's own
 * handling run (cursor movement, find, copy, panes, ...), with *ev
 * possibly rewritten (Space = Page Down). */
static bool book_editor_key(const kb_event_t *&ev, kb_event_t *tmp, bool ctrl)
{
    bool alt = (ev->modifier & (KB_MOD_LALT | KB_MOD_RALT)) != 0;
    bool gui = (ev->modifier & (KB_MOD_LGUI | KB_MOD_RGUI)) != 0;
    bool shift = (ev->modifier & (KB_MOD_LSHIFT | KB_MOD_RSHIFT)) != 0;
    bool plain = !ctrl && !alt && !gui;
    char ch = kb_layout_shortcut_char(ev->keycode);
    int cur = editor_book_chapter(), n = editor_book_chapter_count();

    if (plain && ev->keycode == KB_KEY_SPACE) {
        *tmp = *ev;
        tmp->keycode = shift ? KB_KEY_PAGEUP : KB_KEY_PAGEDOWN;
        tmp->modifier = 0;
        tmp->character = 0;
        ev = tmp;
    }
    bool next = (plain && !shift && ch == 'n') || (ctrl && ev->keycode == KB_KEY_PAGEDOWN);
    bool prev = (plain && !shift && ch == 'p') || (ctrl && ev->keycode == KB_KEY_PAGEUP);
    /* Page Down at the end of a chapter goes on to the next one, Page
     * Up at its start back to the end of the previous one. */
    if (ev->keycode == KB_KEY_PAGEDOWN && !ev->modifier &&
        editor_get_cursor() >= editor_get_length() && cur + 1 < n) next = true;
    if (ev->keycode == KB_KEY_PAGEUP && !ev->modifier && editor_get_cursor() == 0 && cur > 0) {
        book_show_chapter(cur - 1, true);
        return true;
    }
    if (next) {
        if (cur + 1 < n) book_show_chapter(cur + 1, false);
        else editor_ui_set_status("This is the last chapter");
        return true;
    }
    if (prev) {
        if (cur > 0) book_show_chapter(cur - 1, false);
        else editor_ui_set_status("This is the first chapter");
        return true;
    }
    if (plain && !shift && ch == 'c') {
        open_toc_picker();
        return true;
    }

    /* Everything that would change the text. */
    static const char *const ro = "Reading a book: N / P chapter, C contents, Esc close";
    switch (ev->keycode) {
    case KB_KEY_UP: case KB_KEY_DOWN: case KB_KEY_LEFT: case KB_KEY_RIGHT:
    case KB_KEY_HOME: case KB_KEY_END: case KB_KEY_PAGEUP: case KB_KEY_PAGEDOWN:
    case KB_KEY_ESCAPE: case KB_KEYCODE_CAPS_LOCK:
        return false;
    case KB_KEY_TAB:
        if (ctrl) return false;   /* Ctrl+Tab: the other pane */
        break;
    default:
        if (ev->keycode >= KB_KEY_F1 && ev->keycode <= KB_KEY_F12) return false;
        if (ctrl && ev->keycode != KB_KEY_BACKSPACE && ev->keycode != KB_KEY_DELETE &&
            ev->keycode != KB_KEY_ENTER && !strchr("vxhs", ch ? ch : '!')) return false;
        break;
    }
    editor_ui_set_status(ro);
    return true;
}

static void handle_editor_key(const kb_event_t *ev)
{
    s_sugg_kept = false;
    /* A status message the key posts keeps the bars shown
     * (set_status_with_timeout() -> bars_show()). */
    bars_typing_begin();
    handle_editor_key_main(ev);
    if (!s_sugg_kept) suggest_hide();
    bool bars_changed = bars_typing_end();
#if defined(CONFIG_DRAFTLING_DISPLAY_EPD)
    /* The typing fast path clips the next refresh to the cursor line,
     * which would leave the bars on the glass. */
    if (bars_changed) display_set_partial_clip(0, 0, 0, 0);
#else
    (void)bars_changed;
#endif
}

static void handle_editor_key_main(const kb_event_t *ev)
{
    bool ctrl  = (ev->modifier & (KB_MOD_LCTRL | KB_MOD_RCTRL)) != 0;
    bool shift = (ev->modifier & (KB_MOD_LSHIFT | KB_MOD_RSHIFT)) != 0;

    /* All editing acts on the focused pane's document; make sure the
     * engine's active doc and the widget aliases point at it. */
    pane_bind_focus();

    /* Set by the content-modifying key cases below. Pure cursor
     * navigation leaves this false so the post-key refresh repaints
     * only the focused pane (keeping the caret inside it); edits set it
     * true to force a full refresh that also repaints a sibling pane
     * sharing the same document. */
    bool content_changed = false;

#if defined(CONFIG_DRAFTLING_DISPLAY_EPD)
    /* Snapshot before-edit state for the partial-refresh fast path,
     * and ensure the clip is updated (set or cleared) on every
     * return path. */
    ClipGuard clip_guard;
    bool &fast_path_eligible = clip_guard.eligible;
#endif

    /* Forget the "preferred column" used by visual Up/Down navigation
     * the moment the user presses anything else -- otherwise typing,
     * Home/End, etc. would leave a stale goal_x that the next Up/Down
     * would snap back to. */
    if (ev->keycode != KB_KEY_UP && ev->keycode != KB_KEY_DOWN) {
        s_visual_goal_x = -1;
    }

    /* The name-suggestion list, when shown, takes Up / Down / Enter /
     * Tab / Esc. */
    if (suggest_handle_key(ev)) return;

#ifdef DRAFTLING_HAS_IME
    /* Shortcuts, function keys and Alt / Win combinations are not for
     * the input method: finish its composition first. */
    if (ctrl || (ev->modifier & (KB_MOD_LALT | KB_MOD_LGUI | KB_MOD_RGUI)) ||
        (ev->keycode >= KB_KEY_F1 && ev->keycode <= KB_KEY_F12)) {
        ime_finish();
    }
#endif

    /* F1 opens the menu */
    if (ev->keycode == KB_KEY_F1) {
        show_menu();
        return;
    }
    if (ev->keycode == KB_KEY_F10) {
        show_help(HELP_FROM_EDITOR);
        return;
    }
    /* Right Alt is AltGr on layouts that have one (AltGr + i types
     * i-acute on the UK layout), so only Left Alt is Alt there. */
    uint8_t alt_mask = kb_layout_has_altgr() ? KB_MOD_LALT
                                             : (KB_MOD_LALT | KB_MOD_RALT);
    if ((ev->modifier & alt_mask) && !ctrl &&
        kb_layout_shortcut_char(ev->keycode) == 'i') {
        /* Alt+I: file information for the focused document. */
        show_editor_fileinfo();
        return;
    }

    /* Defense-in-depth, not the primary guard: commit_usb_msc_pending_mode()
     * and kermit_start() force the file browser (not the editor)
     * whenever SD-via-USB / the Kermit server turns on, and
     * handle_browser_key() then refuses to open/create a file while it
     * stays on, so this screen should be unreachable for as long as
     * the card is handed off. Refuse every key but F1 here too anyway,
     * in case some other path (a future feature, a bug) ever lands on
     * the editor screen while it is active -- the card must not be
     * written to while a host may also be writing to it. */
    if (const char *busy = sd_handed_off_msg()) {
        editor_ui_set_status(busy);
        return;
    }

    /* A book is read, not edited. */
    kb_event_t book_ev;
    if (editor_is_read_only()) {
        if (book_editor_key(ev, &book_ev, ctrl)) return;
        ctrl  = (ev->modifier & (KB_MOD_LCTRL | KB_MOD_RCTRL)) != 0;
        shift = (ev->modifier & (KB_MOD_LSHIFT | KB_MOD_RSHIFT)) != 0;
    }

    /* Win+Space cycles the keyboard layout, mirroring Ctrl+L. HID
     * keycode 0x2C is Space; the GUI ("Win"/"Cmd") modifier is
     * KB_MOD_LGUI / KB_MOD_RGUI. */
    if ((ev->modifier & (KB_MOD_LGUI | KB_MOD_RGUI)) &&
        ev->keycode == KB_KEY_SPACE) {
        kb_layout_next();
        save_current_layout_to_nvs();
        bars_flash_request();
        ensure_cursor_visible();
        editor_ui_refresh();
        return;
    }

    /* Ctrl shortcuts */
    if (ctrl) {
        /* kb_layout_shortcut_char() resolves the shortcut letter by the
         * national layout for Latin layouts (so e.g. Ctrl+Z lands on
         * the key printed Z on a German keyboard) and by the US
         * physical key position for non-Latin layouts (Ukrainian,
         * Hebrew, ...), which have no national Latin letter to remap
         * to -- see kb_layout.h. */
        char ch = kb_layout_shortcut_char(ev->keycode);

        /* Ctrl+1 / Ctrl+2 / Ctrl+3 control the split layout. HID
         * keycodes: 1 = 0x1E, 2 = 0x1F, 3 = 0x20. Handled by keycode
         * (not layout translation) so they work in any layout. */
        if (ev->keycode == 0x1E) {            /* Ctrl+1: single pane */
            editor_ui_apply_split_mode(SPLIT_NONE);
            return;
        }
        if (ev->keycode == 0x1F) {            /* Ctrl+2: equal split */
            editor_ui_apply_split_mode(SPLIT_HALF);
            return;
        }
        if (ev->keycode == 0x20) {            /* Ctrl+3: 2/3 <-> 1/3 */
            editor_ui_cycle_wide_split();
            return;
        }
        if (ev->keycode == KB_KEY_TAB) {      /* Ctrl+Tab: switch pane */
            editor_ui_focus_other_pane();
            return;
        }
        if (ev->keycode == KB_KEY_DOWN) {     /* Ctrl+Down: same as PgDn */
            /* Keycode matches plain Down, which the goal_x reset above
             * skips -- clear it here so a following bare Up/Down starts
             * from the post-jump cursor column rather than a stale one. */
            s_visual_goal_x = -1;
            if (shift) editor_set_selection_anchor();
            else editor_clear_selection();
            editor_move_page_down(page_line_count());
            ensure_cursor_visible();
            if (s_pane_count > 1) refresh_focused_pane_and_title();
            else                  editor_ui_refresh();
            return;
        }
        if (ev->keycode == KB_KEY_UP) {       /* Ctrl+Up: same as PgUp */
            s_visual_goal_x = -1;             /* see Ctrl+Down comment above */
            if (shift) editor_set_selection_anchor();
            else editor_clear_selection();
            editor_move_page_up(page_line_count());
            ensure_cursor_visible();
            if (s_pane_count > 1) refresh_focused_pane_and_title();
            else                  editor_ui_refresh();
            return;
        }

        switch (ch) {
        case 's':
            show_save_prompt();
            break;
        case 'n':
            /* New empty document in the focused pane, in the format
             * picked from the overlay (see newfmt_prompt_activate()). */
            show_newfmt_prompt(NEWDOC_EDITOR);
            return;
        case 't':
            /* Ctrl+T: start / stop the writing timer or word goal. */
            show_ptimer_prompt();
            return;
        case 'o':
            /* Opening another file leaves the current document behind.
             * If it has unsaved changes, prompt to save / discard /
             * cancel first (same dialog as Esc, whose save and discard
             * paths both proceed to the file browser); otherwise open
             * the browser straight away. */
            if (editor_is_modified()) {
                show_exit_prompt();
            } else {
                editor_ui_show_file_browser();
            }
            return;
        case 'c':
            if (editor_copy()) {
                host_clipboard_export();
                editor_ui_set_status("Copied");
            } else if (editor_selection_active())
                editor_ui_set_status("Copy failed: no memory or SD card");
            ensure_cursor_visible();
            editor_ui_refresh();
            return;
        case 'x':
            if (s_append_only) {
                editor_ui_set_status("Append-only: deletion disabled");
                return;
            }
            if (editor_cut()) {
                host_clipboard_export();
                editor_ui_set_status("Cut");
            } else if (editor_selection_active())
                editor_ui_set_status("Cut failed: no memory or SD card");
            ensure_cursor_visible();
            editor_ui_refresh();
            return;
        case 'v':
            if (s_append_only && !cursor_at_doc_end()) {
                editor_clear_selection();
                editor_move_doc_end();
            }
            if (s_host_paste_pending) return;
            if (editor_host_clipboard_request()) {
                /* The text comes later: editor_ui_host_clipboard_text(). */
                s_host_paste_pending = true;
                s_host_paste_start = lv_tick_get();
                return;
            }
            if (!editor_paste())
                editor_ui_set_status("Buffer full -- paste truncated");
            ensure_cursor_visible();
            editor_ui_refresh();
            return;
        case 'a':
            editor_select_all();
            ensure_cursor_visible();
            editor_ui_refresh();
            return;
        case 'f':
            show_search_prompt(false);
            return;
        case 'h':
            show_search_prompt(true);
            return;
        case 'g':
            /* Sync the focused document's project. */
            start_project_sync();
            break;
        case 'w':
#if defined(CONFIG_DRAFTLING_NETWORK_HOST_MANAGED)
            break;   /* the host's network */
#endif
            if (!wifi_manager_is_connected()) {
                editor_ui_set_status("WiFi: connecting...");
                wifi_connect_async();
            } else {
                wifi_manager_disconnect();
                editor_ui_set_status("WiFi: disconnecting...");
            }
            break;
        case 'l':
            /* Ctrl+L: cycle keyboard layout */
            kb_layout_next();
            save_current_layout_to_nvs();
            bars_flash_request();
            break;
        case 'm':
            /* Ctrl+M: menu, equivalent to F1 */
            show_menu();
            return;
        case 'p':
            /* Ctrl+P: enter deep sleep. With unsaved changes in the
             * active document, ask first (save / discard / cancel);
             * otherwise sleep straight away. */
            if (editor_is_modified()) {
                show_sleep_prompt();
            } else {
                standby_enter_sleep();
            }
            return;
#if defined(CONFIG_DRAFTLING_DISPLAY_HAS_BACKLIGHT) && !defined(CONFIG_DRAFTLING_DISPLAY_BACKLIGHT_BINARY)
        case 'b': {
            /* Ctrl+B: cycle to the next backlight / front-light
             * brightness step (same cycle as F1 -> Settings ->
             * Backlight). Applied immediately and persisted in NVS
             * so the value survives reboot / deep sleep. */
            int bi = find_backlight_option(s_backlight_pct);
            bi = (bi + 1) % BACKLIGHT_OPTION_COUNT;
            s_backlight_pct = BACKLIGHT_OPTIONS[bi];
            save_backlight_to_nvs();
            display_set_backlight(s_backlight_pct);
            char sbuf[32];
            snprintf(sbuf, sizeof(sbuf), BACKLIGHT_LABEL ": %d%%", s_backlight_pct);
            editor_ui_set_status(sbuf);
            return;
        }
#endif
#if defined(CONFIG_DRAFTLING_DISPLAY_EPD)
        case 'r':
            /* Ctrl+R: force a full e-paper refresh to clear ghosting
             * and any stale "overwritten" pixels left behind by partial
             * refreshes. Re-displaying the existing framebuffer is not
             * enough -- the garbage lives in the framebuffer itself, so
             * wipe it to the background colour and invalidate the active
             * screen to make LVGL repaint every widget from scratch
             * (display_clear() also flags the next flush as a full
             * refresh). Mirrors the font-size-change path above. */
#if defined(CONFIG_DRAFTLING_EPD_BLACK_BACKGROUND)
            display_clear(0x00);
#else
            display_clear(0xFF);
#endif
            lv_obj_invalidate(lv_scr_act());
            return;
#endif
        default: break;
        }
        /* Ctrl+arrow / Ctrl+Home / Ctrl+End for word/doc movement.
         * On RTL paragraphs the Left / Right arrows are swapped so
         * that the visual right arrow walks the cursor to the visual
         * right (which is the logical previous word in an RTL line). */
        int word_kc = ev->keycode;
        if ((word_kc == KB_KEY_LEFT || word_kc == KB_KEY_RIGHT) &&
            cursor_line_is_rtl()) {
            word_kc = (word_kc == KB_KEY_LEFT) ? KB_KEY_RIGHT : KB_KEY_LEFT;
        }
        if (word_kc == KB_KEY_LEFT) {
            if (shift) editor_set_selection_anchor();
            else editor_clear_selection();
            editor_move_word_left();
        }
        if (word_kc == KB_KEY_RIGHT) {
            if (shift) editor_set_selection_anchor();
            else editor_clear_selection();
            editor_move_word_right();
        }
        if (ev->keycode == KB_KEY_HOME) {
            if (shift) editor_set_selection_anchor();
            else editor_clear_selection();
            editor_move_doc_start();
        }
        if (ev->keycode == KB_KEY_END) {
            if (shift) editor_set_selection_anchor();
            else editor_clear_selection();
            editor_move_doc_end();
        }
        ensure_cursor_visible();
        /* Ctrl word / document moves are pure navigation: repaint only
         * the focused pane while split so the caret stays in it. */
        if (s_pane_count > 1) refresh_focused_pane_and_title();
        else                  editor_ui_refresh();
        return;
    }

#ifdef DRAFTLING_HAS_IME
    /* Input methods: Vietnamese Telex, Korean Hangul, Japanese kana /
     * kanji, Chinese pinyin. Typed keys and, while a composition is
     * open, the editing keys (Space, Enter, Backspace, arrows, Esc)
     * go to the ime component first. */
    {
        kb_ime_t kind = kb_layout_ime();
        bool composing = ime_composing();
        const char *text = (kind != KB_IME_NONE)
                               ? kb_layout_translate(ev->keycode, ev->modifier)
                               : NULL;
        if (kind != KB_IME_NONE && (text || composing)) {
            if (text && !composing) {
                /* Same preparation as plain typing below. */
                if (s_append_only) {
                    editor_clear_selection();
                    if (!cursor_at_doc_end()) editor_move_doc_end();
                } else {
                    editor_delete_selection();
                }
            }
            bool used = ime_handle_key(kind, ev->keycode, ev->modifier, text);
            ime_sync_status();
            if (used) {
                ensure_cursor_visible();
                editor_ui_refresh();
                return;
            }
        } else if (composing) {
            ime_finish();
        }
    }
#endif

    /* Special keys */
    /* Flip Left / Right semantics on RTL paragraphs so the visual
     * right arrow always advances the cursor to the visual right.
     * In an RTL line that means moving logically backward, and vice
     * versa. The other cases (Up / Down / Home / End / etc.) are
     * unaffected because Home / End already map to logical line
     * boundaries which coincide with the correct visual edges in
     * either direction. */
    int nav_kc = ev->keycode;
    if ((nav_kc == KB_KEY_LEFT || nav_kc == KB_KEY_RIGHT) &&
        cursor_line_is_rtl()) {
        nav_kc = (nav_kc == KB_KEY_LEFT) ? KB_KEY_RIGHT : KB_KEY_LEFT;
    }
    switch (nav_kc) {
    case KB_KEY_LEFT:
        if (shift) { editor_set_selection_anchor(); editor_move_left(); }
        else if (editor_selection_active()) {
            size_t s, e; editor_get_selection_range(&s, &e);
            editor_clear_selection(); editor_set_cursor(s);
        } else { editor_move_left(); }
        for (int k = 0; k < 4 && cursor_mid_cluster(); k++) editor_move_left();
        break;
    case KB_KEY_RIGHT:
        if (shift) { editor_set_selection_anchor(); editor_move_right(); }
        else if (editor_selection_active()) {
            size_t s, e; editor_get_selection_range(&s, &e);
            editor_clear_selection(); editor_set_cursor(e);
        } else { editor_move_right(); }
        for (int k = 0; k < 4 && cursor_mid_cluster(); k++) editor_move_right();
        break;
    case KB_KEY_UP:
        if (shift) editor_set_selection_anchor();
        else editor_clear_selection();
        editor_ui_move_visual(-1);
        break;
    case KB_KEY_DOWN:
        if (shift) editor_set_selection_anchor();
        else editor_clear_selection();
        editor_ui_move_visual(+1);
        break;
    case KB_KEY_HOME:
        if (shift) editor_set_selection_anchor();
        else editor_clear_selection();
        editor_move_home();
        break;
    case KB_KEY_END:
        if (shift) editor_set_selection_anchor();
        else editor_clear_selection();
        editor_move_end();
        break;
    case KB_KEY_PAGEUP:
        if (shift) editor_set_selection_anchor();
        else editor_clear_selection();
        editor_move_page_up(page_line_count());
        break;
    case KB_KEY_PAGEDOWN:
        if (shift) editor_set_selection_anchor();
        else editor_clear_selection();
        editor_move_page_down(page_line_count());
        break;
    case KB_KEY_BACKSPACE:
        if (s_append_only) {
            editor_ui_set_status("Append-only: deletion disabled");
            break;
        }
        content_changed = true;
        if (!editor_delete_selection()) {
            editor_delete_back();
#if defined(CONFIG_DRAFTLING_DISPLAY_EPD)
            fast_path_eligible = true;
#endif
        }
        break;
    case KB_KEY_DELETE:
        if (s_append_only) {
            editor_ui_set_status("Append-only: deletion disabled");
            break;
        }
        content_changed = true;
        if (!editor_delete_selection()) {
            editor_delete_forward();
#if defined(CONFIG_DRAFTLING_DISPLAY_EPD)
            fast_path_eligible = true;
#endif
        }
        break;
    case KB_KEY_ENTER:
        content_changed = true;
        if (editor_format_active()->handle_enter(shift)) break;
        /* Newlines are the one insertion allowed anywhere while
         * append-only editing is active, so no end-of-document jump
         * is needed here -- but deletion is still disallowed, so a
         * selection is cleared rather than replaced. */
        if (s_append_only) editor_clear_selection();
        else editor_delete_selection();
        if (!editor_insert_newline())
            editor_ui_set_status("Buffer full -- increase editor size in menuconfig");
        break;
    case KB_KEY_TAB:
        content_changed = true;
        if (editor_format_active()->handle_tab(s_append_only)) break;
        if (s_append_only) {
            editor_clear_selection();
            if (!cursor_at_doc_end()) editor_move_doc_end();
        } else {
            editor_delete_selection();
        }
        if (!editor_insert_text("    ", 4))
            editor_ui_set_status("Buffer full -- increase editor size in menuconfig");
        break;
    case KB_KEY_ESCAPE:
        if (editor_is_modified()) {
            /* Ask whether to save, discard, or keep editing. */
            show_exit_prompt();
            return;
        }
        /* No unsaved changes -- leave the editor immediately. */
        editor_ui_show_file_browser();
        return;
    default: {
        /* Use keyboard layout to translate keycode to UTF-8 */
        const char *text = kb_layout_translate(ev->keycode, ev->modifier);
        if (text) {
            content_changed = true;
            bool had_sel = editor_selection_active();
            if (s_append_only) {
                /* Regular character insertion is only allowed at the
                 * very end of the document while append-only editing
                 * is active. If the cursor is elsewhere, jump it to
                 * the end (dropping any selection, since deletion is
                 * disallowed) before inserting. */
                editor_clear_selection();
                if (!cursor_at_doc_end()) editor_move_doc_end();
            } else {
                editor_delete_selection();
            }
            if (!editor_insert_text(text, strlen(text)))
                editor_ui_set_status("Buffer full -- increase editor size in menuconfig");
#if defined(CONFIG_DRAFTLING_DISPLAY_EPD)
            /* The fast-path clip only handles edits that started
             * without a selection -- replacing a selection
             * potentially repaints multiple lines. */
            if (!had_sel) fast_path_eligible = true;
#else
            (void)had_sel;
#endif
        }
        break;
    }
    }

    ensure_cursor_visible();
    if (content_changed || s_pane_count <= 1) {
        /* Content edits (and single-pane mode) take the full refresh so
         * a sibling pane sharing the same document repaints too. */
        editor_ui_refresh();
    } else {
        /* Pure cursor navigation while split: repaint only the focused
         * pane so the caret can never end up drawn in the other pane
         * when the cursor reaches the beginning / end of the text. */
        refresh_focused_pane_and_title();
    }
    /* Show, follow or close the name-suggestion list. The typing fast
     * path would clip the list's area out of the refresh. */
    if (suggest_update(content_changed)) {
#if defined(CONFIG_DRAFTLING_DISPLAY_EPD)
        fast_path_eligible = false;
#endif
    }
    /* Partial-refresh clip is set/cleared by ClipGuard's destructor
     * on the M5Stack PaperS3 backend. */
}

/* Overlay a compact file selector on the focused pane (split mode).
 * The overlay widgets live on s_scr as siblings of the pane containers;
 * they are positioned over the focused pane's rectangle and raised to
 * the foreground so they cover that pane's text while the other pane
 * keeps rendering its document. */
static void show_inpane_browser(void)
{
    if (!s_inpane_list || !s_inpane_hdr) return;

    /* Track the live split geometry so the overlay matches the focused
     * pane even after a Ctrl+2 / Ctrl+3 width change. */
    recalc_pane_geometry();
    int px = s_panes[s_focus].x;
    int pw = s_panes[s_focus].w;
    int py = s_panes[s_focus].y;
    int ph = s_panes[s_focus].h;
    int hdr_h = lv_font_get_line_height(FONT_11) + 2;
    if (hdr_h > ph) hdr_h = 0;

    lv_obj_set_pos(s_inpane_hdr, px + 2, py);
    lv_obj_set_width(s_inpane_hdr, (pw > 4) ? pw - 4 : pw);
    lv_obj_set_pos(s_inpane_list, px, py + hdr_h);
    lv_obj_set_size(s_inpane_list, pw, ph - hdr_h);

    /* Target the focused pane and operate the shared browser logic on
     * the in-pane list. */
    s_open_target_pane = s_focus;
    s_browser_list = s_inpane_list;
    s_browser_sel = 0;
    s_browser_sel_prev = -1;
    refresh_file_list();

    lv_obj_remove_flag(s_inpane_hdr, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(s_inpane_list, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_inpane_hdr);
    lv_obj_move_foreground(s_inpane_list);

    /* Clear the covered pane's editor widgets so none of its text shows
     * through. The overlay list only paints rows it has; any area below
     * the last entry (and the caret / logo, which are raised above the
     * list) would otherwise leave the previous document's lines visible
     * behind the selector. Hide the focused pane's line labels, cursor
     * and logo explicitly. */
    {
        pane_t *save = s_rp;
        s_rp = &s_panes[s_focus];
        for (int i = 0; i < MAX_LINE_LABELS; i++) {
            if (s_line_labels[i]) lv_obj_add_flag(s_line_labels[i], LV_OBJ_FLAG_HIDDEN);
        }
        if (s_cursor)   lv_obj_add_flag(s_cursor, LV_OBJ_FLAG_HIDDEN);
        if (s_img_logo) lv_obj_add_flag(s_img_logo, LV_OBJ_FLAG_HIDDEN);
        s_cursor_on_screen = false;
        s_rp = save;
    }

    s_inpane_browser_open = true;
    /* The editor screen stays active (the other pane keeps rendering);
     * keystrokes are rerouted by the s_inpane_browser_open check in
     * process_key_event. */
    editor_ui_set_status(browser_at_root()
                             ? "Open into pane: Enter  N:new project  Ctrl+Tab:other pane"
                             : "Open into pane: Enter  N:new  Esc:projects  Ctrl+Tab:other pane");
}

/* Dismiss the in-pane file selector and restore the full-screen list
 * as the active browser list. Safe to call when no overlay is open.
 * Does NOT repaint the editor; the caller decides whether to load a
 * document and then refresh. */
static void close_inpane_browser(void)
{
    if (!s_inpane_browser_open) return;
    s_inpane_browser_open = false;
    if (s_inpane_list) lv_obj_add_flag(s_inpane_list, LV_OBJ_FLAG_HIDDEN);
    if (s_inpane_hdr)  lv_obj_add_flag(s_inpane_hdr, LV_OBJ_FLAG_HIDDEN);
    s_browser_list = s_list_files;

    /* The focused pane was skipped while the overlay covered it (and
     * may now hold a freshly-opened, possibly shorter document). Hide
     * every line label and wipe its render cache so the next
     * editor_ui_refresh() repaints the pane from scratch with no
     * leftover lines from the previously shown file. Hiding the
     * widgets is required in addition to invalidating the cache: the
     * refresh path only hides a stale label when prev_line_visible[]
     * still marks it visible, and invalidate_render_cache() clears that
     * flag -- so without an explicit hide here a shorter new file would
     * leave the previous file's trailing lines on screen. */
    pane_t *save = s_rp;
    s_rp = &s_panes[s_focus];
    for (int i = 0; i < MAX_LINE_LABELS; i++) {
        if (s_line_labels[i]) lv_obj_add_flag(s_line_labels[i], LV_OBJ_FLAG_HIDDEN);
    }
    invalidate_render_cache();
    s_rp = save;
}

/* Open the file (or descend into the directory) selected by the
 * given row index in the file browser. Shared between the Enter-key
 * handler and the touchscreen tap-to-activate path. */
static void browser_activate_item(int row)
{
    /* Touch-tap path for opening a file -- mirrors the keyboard guard
     * in handle_browser_key() so a touchscreen board cannot open a
     * file while the SD card is handed off either. */
    if (const char *busy = sd_handed_off_msg()) {
        editor_ui_set_status(busy);
        return;
    }
    lv_obj_t *btn = lv_obj_get_child(s_browser_list, row);
    if (!btn) return;
    int idx = (int)(intptr_t)lv_obj_get_user_data(btn);
    if (idx < 0 || idx >= s_browser_count) return;
    if (s_browser_entries[idx].is_dir) {
        /* The list of projects: open the project. */
        if (browser_at_root()) {
            project_name_t name;
            if (snprintf(name, sizeof(name), "%s", s_browser_entries[idx].name)
                    < (int)sizeof(name))
                browser_enter_project(name);
        }
        return;
    }
    const char *name = s_browser_entries[idx].name;
    bool book = editor_path_is_book(name);
    /* The list of projects also lists the books in the root folder. */
    if (browser_at_root() && !book) return;

    char dir[64];
    if (browser_at_root()) snprintf(dir, sizeof(dir), "%s", sd_card_get_mount_point());
    else project_dir(s_cur_project, dir, sizeof(dir));
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    if (!browser_at_root()) {
        project_set_last_file(s_cur_project, name);
        snprintf(s_last_project, sizeof(s_last_project), "%s", s_cur_project);
    }
    open_path_in_editor(path);
}

/* Open path (a document or a book) in the editor: single pane, or the
 * target / focused pane of a split. False (with a status message) if
 * it cannot be opened; the browser stays. */
static bool open_path_in_editor(const char *path)
{
    bool book = editor_path_is_book(path);
    if (s_pane_count <= 1) {
        editor_init();
        esp_err_t oerr = editor_open_file(path);
        if (oerr == ESP_ERR_NO_MEM) {
            /* File too large for the editor buffer. Stay on the file
             * browser and tell the user how big the limit is. The limit
             * is sized at boot from available PSRAM (see editor_init),
             * and is also surfaced read-only in F1 -> Settings. */
            char msg[96];
            snprintf(msg, sizeof(msg),
                     "File too large (limit %u KB)",
                     (unsigned)(editor_get_max_doc_size() / 1024));
            editor_ui_set_status(msg);
            return false;
        }
        if (oerr != ESP_OK) {
            editor_ui_set_status(book ? editor_book_error() : "Open failed");
            return false;
        }
        /* Keep pane 0's document handle current (editor_open_file may
         * have acted on the active doc). */
        s_panes[0].doc = editor_get_active();
        s_panes[0].has_saved_view = false;   /* adopt the freshly opened view */
    } else {
        /* Split: load the file into the target (focused) pane through
         * the document pool, sharing the buffer if the same path is
         * already open in the other pane. */
        int tp = (s_open_target_pane >= 0 && s_open_target_pane < s_pane_count)
                     ? s_open_target_pane : s_focus;
        editor_doc_t *d = open_into_pane(tp, path);
        if (!d) {
            editor_ui_set_status(book ? editor_book_error() : "Open failed");
            return false;
        }
        s_focus = tp;
        pane_bind_focus();
    }
#if defined(CONFIG_DRAFTLING_DISPLAY_EPD)
    /* Wipe the framebuffer before drawing the freshly-opened file
     * so no pixels from the file browser (or any previously open
     * document) survive in regions the new content does not cover.
     * display_clear() also flags the next flush as a full refresh,
     * clearing any accumulated e-paper ghosting. Done after the
     * open succeeds so a failed open leaves the browser intact.
     *
     * In split mode the editor screen never changes (the file selector
     * is an in-pane overlay on s_scr), so the upcoming lv_scr_load() in
     * editor_ui_show_editor() is a no-op and would not repaint the
     * *other* pane that display_clear() just blanked. Force a full
     * screen repaint so both panes are redrawn. */
    if (s_pane_count > 1) {
        epd_full_screen_repaint();
    } else {
        display_clear(0xFF);
    }
#endif
    /* editor_open_file restores the cursor / scroll line from the
     * metadata sidecar; make sure the cursor is on screen before
     * the first refresh in case the saved scroll line is out of date. */
    ensure_cursor_visible();
    /* If the in-pane (split) selector was driving this open, dismiss
     * the overlay and restore the full-screen list as the active
     * browser list before repainting the editor. No-op in single-pane
     * mode. */
    close_inpane_browser();
    editor_ui_show_editor();
    return true;
}

static void handle_browser_key(const kb_event_t *ev)
{
    bool ctrl = (ev->modifier & (KB_MOD_LCTRL | KB_MOD_RCTRL)) != 0;

    /* F1 opens the menu from browser too */
    if (ev->keycode == KB_KEY_F1) {
        show_menu();
        return;
    }
    if (ev->keycode == KB_KEY_F10) {
        show_help(HELP_FROM_BROWSER);
        return;
    }

    /* While the SD card is handed to a USB host or the Kermit server,
     * refuse anything that would touch it from the firmware side --
     * opening/creating a file, Git sync, split changes, forced sleep,
     * and so on. Only F1 (to reach the menu and switch it back off)
     * and plain navigation stay live; see commit_usb_msc_pending_mode()
     * and kermit_start(). */
    if (const char *busy = sd_handed_off_msg()) {
        if (ev->keycode != KB_KEY_UP && ev->keycode != KB_KEY_DOWN) {
            editor_ui_set_status(busy);
            return;
        }
    }

    /* Ctrl+G / Ctrl+W / Ctrl+M / Ctrl+P and the Ctrl+1/2/3 split
     * shortcuts work from the file browser as well. See the editor's
     * Ctrl-shortcut block for why kb_layout_shortcut_char() is used
     * instead of a raw layout translation. */
    if (ctrl) {
        char ck = kb_layout_shortcut_char(ev->keycode);

        /* Ctrl+1 / Ctrl+2 / Ctrl+3 control the split layout (HID
         * keycodes 1 = 0x1E, 2 = 0x1F, 3 = 0x20). The panes are on the
         * editor screen, so enabling a split here switches straight to
         * the editor so the change is visible immediately (Ctrl+O in a
         * pane then picks its file). Ctrl+1 with nothing to collapse
         * just stays on the browser. */
        if (ev->keycode == 0x1E) {            /* Ctrl+1: single pane */
            /* The full-screen browser only shows in single-pane mode
             * (a split editor shows the in-pane selector on Esc), so
             * this just confirms the mode + persists it. */
            editor_ui_apply_split_mode(SPLIT_NONE);
            editor_ui_set_status("Split: single pane");
            return;
        }
        if (ev->keycode == 0x1F) {            /* Ctrl+2: equal split */
            editor_ui_apply_split_mode(SPLIT_HALF);
            if (s_pane_count > 1) editor_ui_show_editor();
            return;
        }
        if (ev->keycode == 0x20) {            /* Ctrl+3: 2/3 <-> 1/3 */
            editor_ui_cycle_wide_split();
            if (s_pane_count > 1) editor_ui_show_editor();
            return;
        }

        if (ck == 'p') {
            /* Ctrl+P: deep sleep. No editor screen to prompt on here;
             * the standby pre-sleep hook auto-saves any open document. */
            standby_enter_sleep();
            return;
        }
        if (ck == 'g') {
            start_project_sync();
            return;
        }
        if (ck == 'm') {
            /* Ctrl+M: menu, equivalent to F1 */
            show_menu();
            return;
        }
        if (ck == 't') {
            /* Ctrl+T: start / stop the writing timer or word goal. */
            show_ptimer_prompt();
            return;
        }
        if (ck == 'w') {
#if defined(CONFIG_DRAFTLING_NETWORK_HOST_MANAGED)
            return;   /* the host's network */
#endif
            if (!wifi_manager_is_connected()) {
                editor_ui_set_status("WiFi: connecting...");
                wifi_connect_async();
            } else {
                wifi_manager_disconnect();
                editor_ui_set_status("WiFi: disconnecting...");
            }
            return;
        }
#if defined(CONFIG_DRAFTLING_DISPLAY_HAS_BACKLIGHT) && !defined(CONFIG_DRAFTLING_DISPLAY_BACKLIGHT_BINARY)
        if (ck == 'b') {
            /* Ctrl+B: cycle to the next backlight / front-light
             * brightness step (same behaviour as in the editor). */
            int bi = find_backlight_option(s_backlight_pct);
            bi = (bi + 1) % BACKLIGHT_OPTION_COUNT;
            s_backlight_pct = BACKLIGHT_OPTIONS[bi];
            save_backlight_to_nvs();
            display_set_backlight(s_backlight_pct);
            char sbuf[32];
            snprintf(sbuf, sizeof(sbuf), BACKLIGHT_LABEL ": %d%%", s_backlight_pct);
            editor_ui_set_status(sbuf);
            return;
        }
#endif
#if defined(CONFIG_DRAFTLING_DISPLAY_EPD)
        if (ck == 'r') {
            /* Ctrl+R: force a full e-paper refresh to clear ghosting
             * and any stale "overwritten" pixels left behind by partial
             * refreshes. Wipe the framebuffer to the background colour
             * and invalidate the active screen so LVGL repaints every
             * widget from scratch (display_clear() also flags the next
             * flush as a full refresh). */
#if defined(CONFIG_DRAFTLING_EPD_BLACK_BACKGROUND)
            display_clear(0x00);
#else
            display_clear(0xFF);
#endif
            lv_obj_invalidate(lv_scr_act());
            return;
        }
#endif
    }

    /* Resolve the "N: New file" accelerator the same way Ctrl
     * shortcuts are resolved (see kb_layout_shortcut_char()), not via
     * a raw layout translation -- otherwise the N key would produce a
     * non-Latin character under Ukrainian/Hebrew and this shortcut
     * would silently stop working. N and Ctrl+N both ask for the new
     * document's format first. */
    char ch = kb_layout_shortcut_char(ev->keycode);
    bool alt = (ev->modifier & (KB_MOD_LALT | KB_MOD_RALT)) != 0;

    if (ch == 'n' && !alt) {
        if (browser_at_root()) {
            /* N / Ctrl+N in the list of projects: a new project. */
            char def[PROJECT_NAME_MAX + 1];
            default_project_name(def, sizeof(def));
            show_name_prompt(SAVE_MODE_PROJECT_NEW, "New project (Enter/Esc):", def);
            return;
        }
        show_newfmt_prompt(s_inpane_browser_open ? NEWDOC_INPANE
                                                 : NEWDOC_BROWSER);
        return;
    }
    if ((ch == 'm' || ch == 'c') && alt && !ctrl) {
        /* Alt+M / Alt+C: move / copy the selected file to a project. */
        show_xfer_picker(ch == 'c');
        return;
    }
    if (ch == 'b' && !alt && !ctrl) {
        /* B: the book browser, every folder of the card. */
        open_books_picker();
        return;
    }
    if (ev->keycode == KB_KEY_BACKSPACE ||
        ev->keycode == KB_KEY_ESCAPE) {
        /* Up from a project to the list of projects. */
        browser_go_up();
        return;
    }
    if ((ev->keycode == KB_KEY_DELETE && !ctrl && !alt) || (ch == 'd' && alt && !ctrl)) {
        /* Del / Alt+D: delete the selected file, only if it is already
         * committed and pushed to the Git server (asks to confirm). */
        show_delete_prompt();
        return;
    }
    if (ch == 'i' && alt && !ctrl) {
        /* Alt+I: file information for the selected file. */
        show_browser_fileinfo();
        return;
    }
    if (ev->keycode == KB_KEY_F2 || (ch == 'r' && alt && !ctrl)) {
        /* F2 / Alt+R: rename the selected file (and its metadata
         * sidecar). */
        show_rename_prompt();
        return;
    }

    uint32_t child_count = lv_obj_get_child_count(s_browser_list);
    if (child_count == 0) return;

    switch (ev->keycode) {
    case KB_KEY_UP:
        if (s_browser_sel > 0) s_browser_sel--;
        /* Selecting a different file is one of the two conditions
         * that clears a transient status message (e.g. the
         * "File too large" error left over from a previous open
         * attempt). Cancel the auto-clear timer too so the standard
         * text we just restored is not overwritten 3 seconds later. */
        cancel_status_clear_and_restore();
        break;
    case KB_KEY_DOWN:
        if (s_browser_sel < (int)child_count - 1) s_browser_sel++;
        cancel_status_clear_and_restore();
        break;
    case KB_KEY_ENTER: {
        browser_activate_item(s_browser_sel);
        return;
    }
    default:
        break;
    }

    /* Highlight selected item -- restyle only the items whose state
     * changed (previous and current selection) so the LVGL dirty
     * region stays small enough for a partial e-paper refresh. */
    update_list_highlight(s_browser_list, s_browser_sel, s_browser_sel_prev);
    s_browser_sel_prev = s_browser_sel;
}

/* Key handler for the in-pane (split-mode) file selector. Esc goes up
 * to the list of projects (no effect there); Ctrl+Tab cancels the pick
 * (the pane keeps its current document) and focuses the other pane; F1
 * cancels and opens the menu; N creates a new untitled document in the focused pane;
 * every other key (Up/Down/Enter, Ctrl+G/W/B) is delegated to the
 * shared browser handler, which now operates on the in-pane list and
 * loads the picked file into the focused pane via browser_activate_item
 * (split branch) -> open_into_pane(). */
static void handle_inpane_browser_key(const kb_event_t *ev)
{
    bool ctrl = (ev->modifier & (KB_MOD_LCTRL | KB_MOD_RCTRL)) != 0;

    /* Esc goes up to the list of projects there and does nothing in
     * it, as in the full-screen browser (handle_browser_key()), so the
     * pane can open a file of another project. Ctrl+Tab leaves the
     * selector for the other pane. */
    if (ev->keycode == KB_KEY_F1) {
        close_inpane_browser();
        show_menu();
        return;
    }
    if (ev->keycode == KB_KEY_F10) {
        show_help(HELP_FROM_INPANE);
        return;
    }

    /* Defense-in-depth, not the primary guard: split mode cannot be
     * entered while the SD card is handed off (commit_usb_msc_pending_mode()
     * and kermit_start() collapse it, and handle_browser_key() blocks
     * Ctrl+2/3 from re-entering it), so this overlay should be
     * unreachable in that state to begin with. See handle_editor_key()'s
     * matching guard. */
    if (const char *busy = sd_handed_off_msg()) {
        editor_ui_set_status(busy);
        return;
    }

    if (ctrl && ev->keycode == KB_KEY_TAB) {
        /* Ctrl+Tab leaves the in-pane file selector and moves keyboard
         * focus to the other (editor) pane. Close the overlay first so
         * the pane it covered repaints as an editor, then switch focus
         * and return to editing. */
        close_inpane_browser();
        editor_ui_focus_other_pane();
        editor_ui_show_editor();
        return;
    }

    /* Ctrl+1 / Ctrl+2 / Ctrl+3 change the split layout. This overlay
     * lives on the split editor screen, so close it and re-apply the
     * layout (editor_ui_apply_split_mode() repaints and returns key
     * routing to handle_editor_key). Ctrl+1 in particular is how the
     * user collapses a split they entered by mistake, and it must work
     * from here -- a split editor shows this overlay, not the
     * full-screen browser, on Esc. */
    if (ctrl && (ev->keycode == 0x1E || ev->keycode == 0x1F ||
                 ev->keycode == 0x20)) {
        close_inpane_browser();
        if      (ev->keycode == 0x1E) editor_ui_apply_split_mode(SPLIT_NONE);
        else if (ev->keycode == 0x1F) editor_ui_apply_split_mode(SPLIT_HALF);
        else                          editor_ui_cycle_wide_split();
        return;
    }

    handle_browser_key(ev);
}

/* Process a single key event (must be called with LVGL lock held). */
static void process_key_event(const kb_event_t *ev)
{
    /* Normalize Keypad Enter to regular Enter so all handlers
     * only need to check for KB_KEY_ENTER. */
    kb_event_t norm = *ev;
    if (norm.keycode == KB_KEY_KP_ENTER) {
        norm.keycode = KB_KEY_ENTER;
    }

    /* Ctrl+Q is a substitute for Escape on keyboards that have no
     * dedicated ESC key (e.g. some compact BLE keyboards). Translate
     * it into a synthetic Escape event so every screen handler that
     * already keys off KB_KEY_ESCAPE picks it up unchanged. The letter
     * is resolved like the other Ctrl shortcuts (see
     * kb_layout_shortcut_char()), so on AZERTY it is the key printed Q
     * and Ctrl+A there still selects all. */
    {
        bool ctrl = (norm.modifier & (KB_MOD_LCTRL | KB_MOD_RCTRL)) != 0;
        if (ctrl && kb_layout_shortcut_char(norm.keycode) == 'q') {
            norm.keycode  = KB_KEY_ESCAPE;
            norm.modifier = 0;
            norm.character = 0;
        }
    }

    /* Caps Lock toggles, as on a PC; kb_layout_translate() applies
     * it. The LEDs of the keyboards are set to match. */
    if (norm.keycode == KB_KEYCODE_CAPS_LOCK) {
        bool on = !kb_layout_get_caps_lock();
        kb_layout_set_caps_lock(on);
        uint8_t leds = on ? KB_LED_CAPS_LOCK : 0;
        ble_keyboard_set_leds(leds);
#if defined(CONFIG_DRAFTLING_HAS_USB_HOST)
        usb_kbd_set_leds(leds);
#endif
        /* Other screens show the title when the editor is next drawn. */
        if (s_scr && lv_screen_active() == s_scr) update_title_bar();
        return;
    }

    const kb_event_t *e = &norm;

    if (s_msg_open) {
        /* Modal over whatever screen it was raised on. */
        handle_msg_popup_key(e);
    } else if (s_save_open) {
        handle_save_prompt_key(e);
    } else if (s_newfmt_open) {
        handle_newfmt_prompt_key(e);
    } else if (s_pt_open) {
        handle_ptimer_prompt_key(e);
    } else if (s_del_open) {
        handle_delete_prompt_key(e);
    } else if (s_exit_open) {
        handle_exit_prompt_key(e);
    } else if (s_search_open) {
        handle_search_prompt_key(e);
    } else if (s_settings_open) {
        handle_settings_key(e);
    } else if (s_wifi_pw_open) {
        /* Raised from within the WiFi scan picker (s_menu_open is
         * still true underneath), so it must take priority over
         * handle_menu_key. */
        handle_wifi_pw_prompt_key(e);
    } else if (s_dbx_open) {
        /* Raised from the F1 menu, like the WiFi password prompt. */
        handle_dbx_prompt_key(e);
    } else if (s_help_open) {
        handle_help_key(e);
    } else if (s_menu_open) {
        handle_menu_key(e);
    } else if (s_inpane_browser_open) {
        /* Split-mode in-pane file selector overlays the editor screen
         * (s_editor_screen_active stays true), so it must be checked
         * before handle_editor_key. */
        handle_inpane_browser_key(e);
    } else if (s_editor_screen_active) {
        handle_editor_key(e);
    } else {
        handle_browser_key(e);
    }
}

/* Forward declaration -- defined later in the BLE callback section. */
static void apply_pending_connect_state(void);
static void apply_pending_ble_status_text(void);

/* LVGL timer callback: drains the key-event queue in a batch.
 * This runs inside lv_timer_handler() which already holds the LVGL
 * mutex, so we must NOT call draftling_lvgl_port_lock() here. */
#if defined(CONFIG_DRAFTLING_HAS_USB_MSC)
/* usb_msc's idle auto-off (see usb_msc.h / CONFIG_DRAFTLING_USB_MSC_IDLE_TIMEOUT_SEC)
 * fires from an esp_timer callback with no way to safely touch LVGL,
 * so key_drain_cb polls for it here instead -- the same reason
 * apply_pending_connect_state() exists for BLE connect/disconnect.
 * User-driven mode changes (commit_usb_msc_pending_mode(), called
 * from close_usbmsc_picker()) already refresh the UI themselves; this
 * only reacts to a change nobody in the LVGL task caused. */
static usb_msc_mode_t s_last_polled_usb_msc_mode = USB_MSC_MODE_OFF;

static void poll_usb_msc_auto_off(void)
{
    usb_msc_mode_t mode = usb_msc_get_mode();
    if (mode == s_last_polled_usb_msc_mode) return;
    bool auto_reverted = (s_last_polled_usb_msc_mode != USB_MSC_MODE_OFF &&
                          mode == USB_MSC_MODE_OFF);
    s_last_polled_usb_msc_mode = mode;
    if (!auto_reverted) return;

    /* The picker (if open) and the F1 menu row both show the staged
     * s_usbmsc_pending_mode, not usb_msc_get_mode() directly -- drop
     * the now-stale staged choice along with the real one so neither
     * offers to "change" into a mode that is already active. */
    s_usbmsc_pending_mode = USB_MSC_MODE_OFF;
    s_usbmsc_picker_open = false;

    editor_ui_set_status("SD card via USB: disabled (no host connected)");
    /* Pick up whatever a host may have changed on the card, and drop
     * the now-stale "via USB" hint from the menu if it is open. */
    if (!s_editor_screen_active) refresh_file_list();
    if (s_menu_open) refresh_menu_items();
}
#endif

static void apply_pending_passkey(void);
static void apply_pending_sleep_request(void);

static void key_drain_cb(lv_timer_t *timer)
{
    (void)timer;
    kb_event_t ev;

    /* Drain any pending BLE connect/disconnect transition first.
     * The BLE host task only sets the flag (it cannot safely take
     * the LVGL mutex - on the e-paper boards display_flush() can
     * hold the mutex for several seconds during a panel refresh,
     * which would time out the BLE callback and leave the user
     * stuck on the "Reconnecting..." prompt screen even after
     * the keyboard has connected). See apply_pending_connect_state().
     * Progress text first, so a connect / disconnect arriving in the
     * same tick has the last word on the prompt label. */
    apply_pending_ble_status_text();
    apply_pending_connect_state();
    apply_pending_passkey();
    apply_pending_sleep_request();
#if defined(CONFIG_DRAFTLING_HAS_USB_MSC)
    poll_usb_msc_auto_off();
#endif

    if (s_host_paste_pending) {
        if (lv_tick_elaps(s_host_paste_start) < HOST_PASTE_TIMEOUT_MS) return;
        s_host_paste_pending = false;
        editor_ui_set_status("Paste failed: the clipboard did not answer");
    }

    int drained = 0;
    while (xQueueReceive(s_key_queue, &ev, 0) == pdTRUE) {
        process_key_event(&ev);
        drained++;
    }

#if defined(CONFIG_DRAFTLING_DISPLAY_EPD)
    /* ClipGuard (see handle_editor_key()) sets a one-shot partial-
     * refresh clip sized for a SINGLE keystroke's changed pixels, in
     * its destructor. When keystrokes queue up faster than this timer
     * can drain them -- e.g. while the previous tick's flush_cb() /
     * display_flush() call was blocked for hundreds of ms driving an
     * EPD partial refresh, which is easily longer than a fast typist's
     * inter-key interval -- the loop above processes more than one key
     * per tick, and each key's ClipGuard destructor overwrites the clip
     * in turn: only the LAST key's tiny window survives to the flush
     * that follows. The dirty bounding box has correctly accumulated
     * every key's pixels by then, but the stale narrow clip discards
     * everything outside that last window, and the backend's
     * clear_dirty() then drops that region for good -- the earlier
     * keystrokes' glyphs never reach the panel, showing up as
     * persistent blank gaps under fast typing. Falling back to the
     * full accumulated dirty bbox whenever more than one key was
     * drained this tick keeps the narrow per-key fast path for the
     * common (unhurried) case while staying correct in bursts. */
    if (drained > 1) {
        display_set_partial_clip(0, 0, 0, 0);
    }
#endif
}

extern "C" void editor_ui_host_clipboard_text(const char *text, size_t len)
{
    if (!s_host_paste_pending) return;   /* timed out already */
    s_host_paste_pending = false;
    if (!text) {
        editor_ui_set_status("The clipboard holds no text");
        return;
    }
    if (!s_scr || lv_screen_active() != s_scr) return;
    pane_bind_focus();
    if (!editor_paste_text(text, len))
        editor_ui_set_status("Buffer full -- paste truncated");
    ensure_cursor_visible();
    editor_ui_refresh();
}

/* BLE callback: enqueue the event and return immediately.
 * This function runs on the HID host task and must not touch LVGL. */
extern "C" void editor_ui_handle_key(const void *event)
{
    const kb_event_t *ev = (const kb_event_t *)event;

    if (ev->pressed) {
        /* Reset standby inactivity timer on key-down */
        standby_reset_timer();

        /* Caps Lock toggles once per press: no key repeat. */
        if (ev->keycode == KB_KEYCODE_CAPS_LOCK) {
            if (s_key_queue) xQueueSend(s_key_queue, ev, 0);
            return;
        }

        /* Start key-repeat tracking */
        s_repeat_ev     = *ev;
        s_repeat_held   = true;
        s_repeat_start  = xTaskGetTickCount();
        s_repeat_firing = false;

        if (s_key_queue) {
            xQueueSend(s_key_queue, ev, 0);
        }
    } else {
        /* Key released -- cancel repeat for the released key */
        if (s_repeat_held && ev->keycode == s_repeat_ev.keycode) {
            s_repeat_held  = false;
            s_repeat_firing = false;
        }
    }
}

/* ---- Key repeat timer ---- */

/* Called periodically by an LVGL timer.  If a key is held longer than
 * KEY_REPEAT_DELAY_MS, inject the stored key event at the repeat rate. */
static void key_repeat_cb(lv_timer_t *timer)
{
    (void)timer;
    if (!s_repeat_held || s_host_paste_pending) return;

    uint32_t now = xTaskGetTickCount();
    uint32_t elapsed = (now - s_repeat_start) * portTICK_PERIOD_MS;

    /* See KEY_REPEAT_MAX_MS's comment: treat an implausibly long hold
     * as a missed key-up rather than genuine input, and stop
     * repeating instead of re-injecting this key forever. */
    if (elapsed >= KEY_REPEAT_MAX_MS) {
        s_repeat_held   = false;
        s_repeat_firing = false;
        return;
    }

    if (!s_repeat_firing) {
        if (elapsed >= KEY_REPEAT_DELAY_MS) {
            s_repeat_firing = true;
            /* Inject the first repeat event */
            process_key_event(&s_repeat_ev);
        }
    } else {
        /* Already past the initial delay -- inject at the repeat rate.
         * The timer fires every KEY_REPEAT_RATE_MS so one event per
         * invocation is sufficient. */
        process_key_event(&s_repeat_ev);
    }
}

/* ---- Passkey display callback ---- */

/* Latest passkey event from the BLE host task, applied by the LVGL
 * task in key_drain_cb() (like s_pending_conn_state below). The BLE
 * task must not wait for the LVGL mutex: an e-paper refresh can hold
 * it longer than any sensible timeout, and a dropped passkey left the
 * user with no code to type, so pairing failed. */
static portMUX_TYPE s_passkey_mux = portMUX_INITIALIZER_UNLOCKED;
static uint32_t     s_passkey_pending = 0;
static bool         s_passkey_has_pending = false;

static void passkey_display_cb(uint32_t passkey)
{
    taskENTER_CRITICAL(&s_passkey_mux);
    s_passkey_pending = passkey;
    s_passkey_has_pending = true;
    taskEXIT_CRITICAL(&s_passkey_mux);
}

/* Run on the LVGL task (from key_drain_cb), mutex already held. */
/* Set by editor_ui_request_sleep() (button poller task), handled in
 * the UI task by apply_pending_sleep_request(). */
static volatile bool s_sleep_requested = false;

extern "C" void editor_ui_request_sleep(void)
{
    standby_reset_timer();
    s_sleep_requested = true;
}

/* A Power-button sleep request, treated like Ctrl+P / "Sleep now":
 * with unsaved changes in the editor, ask first. The prompt can only
 * be raised from the editor itself (or over the F1 menu, which is
 * closed first, as "Sleep now" does); while another dialog is open the
 * press only explains why nothing happened, and while an operation is
 * running (busy pop-up, e.g. formatting the card) it is ignored. */
static void apply_pending_sleep_request(void)
{
    if (!s_sleep_requested) return;
    s_sleep_requested = false;

    if (s_msg_open && s_msg_busy) return;
    if (!s_editor_screen_active || !editor_is_modified()) {
        standby_enter_sleep();
        return;
    }
    if (s_exit_open) return;    /* already asking */
    /* Only the plain menu list is closed; a picker shown inside the
     * menu counts as an open dialog. */
    bool menu_picker = s_wifi_scan_picker_open || s_fmt_step != FMT_STEP_NONE ||
                       s_books_open || s_toc_open;
#if defined(CONFIG_DRAFTLING_HAS_USB_MSC)
    menu_picker = menu_picker || s_usbmsc_picker_open;
#endif
    if (s_menu_open && !s_settings_open && !s_wifi_pw_open &&
        !s_dbx_open && !s_help_open && !menu_picker) {
        close_menu();
    }
    if (s_msg_open || s_save_open || s_newfmt_open || s_del_open ||
        s_search_open || s_settings_open || s_wifi_pw_open || s_dbx_open ||
        s_help_open || s_menu_open || s_inpane_browser_open) {
        if (!s_msg_open) {
            show_msg_popup("Unsaved changes",
                           "Close this dialog, then press Power again "
                           "to save or discard the changes.", false);
        }
        return;
    }
    show_sleep_prompt();
}

static void apply_pending_passkey(void)
{
    taskENTER_CRITICAL(&s_passkey_mux);
    bool has = s_passkey_has_pending;
    uint32_t passkey = s_passkey_pending;
    s_passkey_has_pending = false;
    taskEXIT_CRITICAL(&s_passkey_mux);
    if (!has || !s_passkey_panel) return;

    if (passkey == BLE_PASSKEY_DISMISS) {
        lv_obj_add_flag(s_passkey_panel, LV_OBJ_FLAG_HIDDEN);
        /* The dismiss is typically followed almost immediately by a
         * connect-state transition (BLE auth complete -> HIDH open
         * -> ble_connect_status_cb), which queues a second screen
         * change. On the epdiy backend, two large GL16 partials
         * issued back-to-back right after the BLE stack hammered
         * the bus can wedge the `epd_prep` feeders and trip the
         * task watchdog. Latch the next flush to a single GC16
         * full refresh so both redraws coalesce. */
        display_request_full_refresh();
    } else {
        /* Pairing normally runs while the BLE prompt screen (or the
         * file browser) is shown, so move the overlay onto whichever
         * screen is active, like the other pop-ups. */
        char buf[96];
        snprintf(buf, sizeof(buf),
                 "Keyboard pairing code:\n%06lu\n"
                 "Type it on the keyboard, then press Enter",
                 (unsigned long)passkey);
        lv_label_set_text(s_passkey_label, buf);
        lv_obj_set_parent(s_passkey_panel, lv_scr_act());
        lv_obj_center(s_passkey_panel);
        lv_obj_move_foreground(s_passkey_panel);
        lv_obj_remove_flag(s_passkey_panel, LV_OBJ_FLAG_HIDDEN);
    }
}

/* ---- BLE connection status callback ---- */

/* Pending connect/disconnect state set by the BLE host task and
 * consumed by the LVGL task. Values: -1 = no pending change,
 * 0 = pending disconnect, 1 = pending connect. Declared volatile
 * since it crosses task boundaries; we accept the tiny race where
 * a back-to-back connect/disconnect could overwrite each other --
 * the consumer always observes the latest state, which matches
 * what `ble_keyboard_is_connected()` would report. */
static volatile int8_t s_pending_conn_state = -1;

/* Latest BLE progress message waiting for the LVGL task (see
 * ble_status_text_cb()); intermediate ones are superseded. */
static portMUX_TYPE s_ble_status_mux = portMUX_INITIALIZER_UNLOCKED;
static char         s_ble_status_pending[128];
static bool         s_ble_status_has_pending = false;

/* Run on the LVGL task (from key_drain_cb), mutex already held. */
static void apply_pending_ble_status_text(void)
{
    char text[sizeof(s_ble_status_pending)];
    taskENTER_CRITICAL(&s_ble_status_mux);
    bool has = s_ble_status_has_pending;
    if (has) {
        memcpy(text, s_ble_status_pending, sizeof(text));
        s_ble_status_has_pending = false;
    }
    taskEXIT_CRITICAL(&s_ble_status_mux);
    if (!has || !s_ble_prompt_lbl) return;
    /* Only update when the BLE prompt screen is active */
    if (lv_scr_act() == s_scr_ble_prompt) {
        lv_label_set_text(s_ble_prompt_lbl, text);
    }
}

/* Run on the LVGL task (from key_drain_cb). At this point the LVGL
 * mutex is already held by the LVGL task, so we must NOT call
 * draftling_lvgl_port_lock() here. */
static void apply_pending_connect_state(void)
{
    int8_t pending = s_pending_conn_state;
    if (pending < 0) return;
    s_pending_conn_state = -1;

    bool connected = (pending != 0);

    /* Connect/disconnect changes the BLE prompt label and often
     * swaps the active screen (prompt <-> editor / file browser),
     * which on the epdiy backend would otherwise issue back-to-back
     * GL16 partials in close succession with the post-auth passkey
     * dismiss above. Coalesce all of these into a single GC16 full
     * refresh -- the user is already seeing a connection-state
     * change, the flash is acceptable, and it avoids wedging
     * epdiy's `epd_prep` feeders. */
    display_request_full_refresh();

    if (connected) {
        /* Flush any stale key events that accumulated while
         * the keyboard was disconnected / reconnecting. */
        if (s_key_queue) {
            xQueueReset(s_key_queue);
        }

        /* Keyboard just connected -- if we are on the BLE prompt screen,
         * return to whatever the user was doing before disconnect. */
        if (lv_scr_act() == s_scr_ble_prompt) {
            /* Drop any stale touch indev state (in-progress press,
             * wait-until-release, act_obj pinned to the prompt's
             * Off button) before swapping screens, so the editor
             * does not inherit a sticky press that would absorb
             * the first tap. */
            lv_indev_reset(NULL, NULL);
            if (s_editor_screen_active) {
                /* Restore the editor -- file contents are still in
                 * the gap buffer; no need to close/reopen. */
                lv_scr_load(s_scr);
#if defined(CONFIG_DRAFTLING_DISPLAY_EPD)
                /* On e-paper, refresh_active_pane()'s per-line cache
                 * (s_prev_line_text[] etc) still holds whatever text
                 * was last rendered before the BLE prompt screen took
                 * over -- since the document did not change while
                 * disconnected, that cache still matches, so
                 * editor_ui_refresh() alone would skip re-calling
                 * lv_label_set_text() for every line ("can_skip"),
                 * leaving the panel blank until something else forces
                 * a real redraw. Matches the same class of bug
                 * epd_full_screen_repaint() already exists to fix for
                 * the split-pane and font-change transitions -- use
                 * it here too rather than the bare
                 * display_request_full_refresh() above, which only
                 * flags the next flush as full and does not touch
                 * this cache. */
                epd_full_screen_repaint();
#endif
                editor_ui_refresh();
            } else {
                editor_ui_show_file_browser();
            }
        }
        if (s_ble_prompt_lbl) {
            lv_label_set_text(s_ble_prompt_lbl,
                "Keyboard connected!");
        }
    } else {
        /* A BLE disconnect arrived, but if another local keyboard is
         * still usable (the Tab5 attachable keyboard, or a USB
         * keyboard) the user is NOT actually keyboard-less -- on the
         * Tab5 the BLE host fires this callback simply because it gets
         * disabled at boot once a wired keyboard is detected. Do not
         * yank the user onto the "Keyboard disconnected /
         * Reconnecting..." prompt screen in that case; leave the
         * editor / file browser as-is. */
        bool other_kbd = false;
#if defined(CONFIG_DRAFTLING_HAS_TAB5_KBD)
        if (tab5_kbd_is_present()) other_kbd = true;
#endif
#if defined(CONFIG_DRAFTLING_HAS_USB_HOST)
        if (usb_kbd_is_connected()) other_kbd = true;
#endif
        if (other_kbd) {
            return;
        }

        /* Keyboard disconnected -- close any open overlays so the
         * UI is in a clean state when we reconnect.  Do NOT call
         * editor_close_file() -- preserve the user's work. */
        s_menu_open = false;
        s_help_open = false;
        s_settings_open = false;
        s_restart_confirm_open = false;
#if defined(CONFIG_DRAFTLING_DISPLAY_COLOR)
        s_theme_picker_open = false;
#endif
        s_layouts_picker_open = false;
        s_save_open = false;
        if (s_save_panel) lv_obj_add_flag(s_save_panel, LV_OBJ_FLAG_HIDDEN);
        save_prompt_restore_layout();
        close_newfmt_prompt();
        close_delete_prompt();
        s_exit_open = false;
        if (s_exit_panel) lv_obj_add_flag(s_exit_panel, LV_OBJ_FLAG_HIDDEN);
        s_search_open = false;
        if (s_search_panel) lv_obj_add_flag(s_search_panel, LV_OBJ_FLAG_HIDDEN);
        /* Cancel any in-progress key repeat so stale keys do not
         * keep firing after reconnection. */
        s_repeat_held = false;
        s_repeat_firing = false;
        if (s_ble_prompt_lbl) {
            lv_label_set_text(s_ble_prompt_lbl,
                "Keyboard disconnected.\nReconnecting...");
        }
        /* Drop any stale touch indev state (in-progress press /
         * gesture wait-until-release / act_obj pinned to a
         * now-hidden editor widget) before swapping screens.
         * Without this, a stray touch frame around the
         * disconnect can leave the LVGL indev with
         * wait_until_release set, which silently absorbs taps
         * on the BLE prompt's "Off" button. */
        lv_indev_reset(NULL, NULL);
#if defined(CONFIG_DRAFTLING_DISPLAY_EPD)
        /* Same missing-repaint bug as show_menu()/show_settings() --
         * see show_menu()'s comment. */
        display_clear(0xFF);
        lv_obj_invalidate(s_scr_ble_prompt);
#endif
        lv_scr_load(s_scr_ble_prompt);
    }
}

/* Runs in the BLE/Bluedroid host task. We deliberately do NOT take
 * the LVGL mutex here: on the e-paper boards, display_flush() can
 * hold the mutex for several seconds during a panel refresh, which
 * would exceed any reasonable callback-side timeout and leave the
 * user stuck on the "Reconnecting..." prompt screen even after the
 * keyboard is connected. Instead, set a flag that the LVGL task
 * (key_drain_cb) drains on its next tick, where the mutex is
 * already held. */
static void ble_connect_status_cb(bool connected)
{
    ESP_LOGI("EditorUI", "BLE connect status: %s",
             connected ? "CONNECTED" : "DISCONNECTED");
    s_pending_conn_state = connected ? 1 : 0;
}

/* BLE status text callback -- hand the BLE prompt label a connection
 * progress message from the BLE keyboard component. Runs in the
 * BLE/Bluedroid host task, so like ble_connect_status_cb() it only
 * stores the text: waiting for the LVGL mutex here either stalls the
 * BLE host for the length of an e-paper refresh or, with a short
 * timeout, silently drops the message and leaves a stale one on
 * screen. apply_pending_ble_status_text() shows the latest one. */
static void ble_status_text_cb(const char *text)
{
    if (!text) return;
    taskENTER_CRITICAL(&s_ble_status_mux);
    snprintf(s_ble_status_pending, sizeof(s_ble_status_pending), "%s", text);
    s_ble_status_has_pending = true;
    taskEXIT_CRITICAL(&s_ble_status_mux);
}

/* ---- WiFi connect task ----
 * wifi_manager_connect() blocks for up to 30 seconds while waiting for
 * the connection.  Running it on a separate task keeps the LVGL render
 * loop responsive so the user sees status updates in real time. */
/* The WiFi worker tasks below wait for the LVGL lock without a time
 * limit (like the Draftbox worker): nothing that holds the lock waits
 * for them, and a dropped result would leave the scan picker on
 * "Scanning..." or the Draftbox prompt on its WiFi step for good --
 * e-paper flushes hold the lock longer than a short timeout. */
/* The WiFi worker tasks below have 4 KB stacks; like wifi_state_cb(),
 * they hand their results to the LVGL task with lv_async_call() instead
 * of touching the UI themselves (status-bar text goes through the font
 * chain and, with the Indic pack, HarfBuzz shaping). */

/* UI side of a wifi_connect_task() failure (LVGL task, lock held). */
static void wifi_connect_fail_apply(void *arg)
{
    bool not_found = (arg != NULL);
    editor_ui_set_status(not_found ? "WiFi: no credentials found"
                                   : "WiFi: busy, try again");
    dbx_wifi_done(false, not_found
        ? "No WiFi network is configured. Use F1 -> "
          "Connectivity -> WiFi -> \"Choose network...\" first."
        : "WiFi is busy, try again.");
    sync_wifi_done(false);
}

static void wifi_connect_task(void *arg)
{
    (void)arg;
    esp_err_t ret = wifi_manager_connect();
    if (ret == ESP_ERR_NOT_FOUND || ret == ESP_ERR_INVALID_STATE) {
        /* No credentials, or another connect/scan is running -- the
         * wifi_state callback will not fire, so report it here. */
        if (draftling_lvgl_port_lock(-1)) {
            lv_async_call(wifi_connect_fail_apply,
                          ret == ESP_ERR_NOT_FOUND ? (void *)1 : NULL);
            draftling_lvgl_port_unlock();
        }
    }
    vTaskDelete(NULL);
}

/* Start WiFi connection on a background task. */
static void wifi_connect_async(void)
{
    BaseType_t rc = xTaskCreatePinnedToCore(wifi_connect_task, "wifi_conn",
                                            4 * 1024, NULL, 3, NULL, 0);
    if (rc != pdPASS) {
        editor_ui_set_status("WiFi: failed to start task");
        dbx_wifi_done(false, "Failed to start the WiFi task.");
        sync_wifi_done(false);
    }
}

/* ---- WiFi scan task ----
 * wifi_manager_scan() blocks for a few seconds; run it off the LVGL
 * task like wifi_connect_task() above. */
typedef struct {
    esp_err_t ret;
    int count;
    wifi_scan_result_t results[WIFI_SCAN_MAX_RESULTS];
} wifi_scan_outcome_t;

/* UI side of a finished scan (LVGL task, lock held). arg is a
 * wifi_scan_outcome_t to free, or NULL when it could not be allocated
 * (reported as a failed scan). */
static void wifi_scan_apply(void *arg)
{
    wifi_scan_outcome_t *o = (wifi_scan_outcome_t *)arg;
    esp_err_t ret = o ? o->ret : ESP_ERR_NO_MEM;
    /* Only touch the UI if the picker is still what's on screen --
     * the user may have backed out (Esc) while the scan was in
     * flight. */
    if (s_wifi_scan_picker_open) {
        s_wifi_scan_in_progress = false;
        s_wifi_scan_count = (ret == ESP_OK) ? o->count : 0;
        if (ret == ESP_OK && o->count > 0) {
            memcpy(s_wifi_scan_results, o->results,
                   sizeof(o->results[0]) * (size_t)o->count);
        }
        s_wifi_scan_picker_sel = 0;
        refresh_wifi_scan_picker_items();
        if (ret != ESP_OK) {
            editor_ui_set_status("WiFi: scan failed");
        }
    }
    free(o);
}

static void wifi_scan_task(void *arg)
{
    (void)arg;
    wifi_scan_outcome_t *o =
        (wifi_scan_outcome_t *)calloc(1, sizeof(wifi_scan_outcome_t));
    if (o) {
        o->ret = wifi_manager_scan(o->results, WIFI_SCAN_MAX_RESULTS, &o->count);
    }

    if (draftling_lvgl_port_lock(-1)) {
        if (lv_async_call(wifi_scan_apply, o) != LV_RESULT_OK) free(o);
        draftling_lvgl_port_unlock();
    } else {
        free(o);
    }
    vTaskDelete(NULL);
}

/* Start (or restart) a scan on a background task -- see
 * start_wifi_scan() in the picker block above. */
static void wifi_scan_async(void)
{
    BaseType_t rc = xTaskCreatePinnedToCore(wifi_scan_task, "wifi_scan",
                                            4 * 1024, NULL, 3, NULL, 0);
    if (rc != pdPASS) {
        s_wifi_scan_in_progress = false;
        editor_ui_set_status("WiFi: failed to start scan task");
        if (s_wifi_scan_picker_open) refresh_wifi_scan_picker_items();
    }
}

/* ---- WiFi connect-to-picked-network task ----
 * Mirrors wifi_connect_task() above, but connects to the SSID/password
 * the user picked in the scan picker instead of reading wifi.cfg/NVS,
 * and persists them to /sdcard/wifi.cfg on success (in the same format
 * wifi_manager_connect() reads back) so Ctrl+W and the next boot's
 * auto-reconnect pick up the new network too. */
typedef struct {
    char ssid[33];
    char password[65];
} wifi_connect_args_t;

/* UI side of a busy wifi_connect_picked_task() (LVGL task, lock held). */
static void wifi_picked_busy_apply(void *)
{
    editor_ui_set_status("WiFi: busy, try again");
}

/* Runs on the LVGL task (lv_async_call); arg is a strdup()ed SSID. */
static void wifi_known_refused_apply(void *arg)
{
    char *ssid = (char *)arg;
    snprintf(s_wifi_known_refused, sizeof(s_wifi_known_refused), "%s", ssid);
    free(ssid);
}

static void wifi_connect_picked_task(void *arg)
{
    wifi_connect_args_t *args = (wifi_connect_args_t *)arg;
    esp_err_t ret = wifi_manager_connect_to(args->ssid, args->password, true);
    if (ret == ESP_OK) {
        wifi_manager_save_to_file(args->ssid, args->password);
    } else if (ret != ESP_ERR_INVALID_STATE && !args->password[0] &&
               wifi_manager_last_failure_was_auth()) {
        /* A saved password (or none) was refused: ask next time. */
        char *ssid = strdup(args->ssid);
        if (ssid && draftling_lvgl_port_lock(-1)) {
            lv_async_call(wifi_known_refused_apply, ssid);
            draftling_lvgl_port_unlock();
        } else {
            free(ssid);
        }
    } else if (ret == ESP_ERR_INVALID_STATE) {
        if (draftling_lvgl_port_lock(-1)) {
            lv_async_call(wifi_picked_busy_apply, NULL);
            draftling_lvgl_port_unlock();
        }
    }
    free(args);
    vTaskDelete(NULL);
}

static void wifi_connect_picked_async(const char *ssid, const char *password)
{
    wifi_connect_args_t *args = (wifi_connect_args_t *)malloc(sizeof(wifi_connect_args_t));
    if (!args) {
        editor_ui_set_status("WiFi: out of memory");
        return;
    }
    strncpy(args->ssid, ssid, sizeof(args->ssid) - 1);
    args->ssid[sizeof(args->ssid) - 1] = '\0';
    strncpy(args->password, password, sizeof(args->password) - 1);
    args->password[sizeof(args->password) - 1] = '\0';

    BaseType_t rc = xTaskCreatePinnedToCore(wifi_connect_picked_task, "wifi_conn2",
                                            4 * 1024, args, 3, NULL, 0);
    if (rc != pdPASS) {
        free(args);
        editor_ui_set_status("WiFi: failed to start task");
    }
}

/* ---- Draftbox connect task ----
 * git_sync_draftbox_connect() makes an HTTPS request (TLS handshake
 * included), so it runs off the LVGL task like the WiFi tasks above.
 * The result -- success or the reason for failure -- goes to the
 * status bar. */
typedef struct {
    char site[128];
    char otp[10];
    char dir[64];                 /* project folder */
    project_name_t project;
} draftbox_connect_args_t;

static void draftbox_connect_task(void *arg)
{
    draftbox_connect_args_t *args = (draftbox_connect_args_t *)arg;
    char msg[128];
    esp_err_t ret = git_sync_draftbox_connect(args->dir, args->site, args->otp,
                                              msg, sizeof(msg));
    project_name_t project;
    snprintf(project, sizeof(project), "%s", args->project);
    memset(args, 0, sizeof(*args));
    free(args);

    char text[256];
    if (ret == ESP_OK) {
        if (msg[0] >= 'a' && msg[0] <= 'z') msg[0] = (char)(msg[0] - 'a' + 'A');
        snprintf(text, sizeof(text),
                 "%s.\nThe repository URL and token are saved in the git.cfg "
                 "of project %s.", msg, project);
    } else {
        snprintf(text, sizeof(text),
                 "Could not connect: %s.\ngit.cfg was not changed.", msg);
    }
    /* Wait for the lock without a timeout: the pop-up ignores keys
     * until this result replaces its "Connecting..." text. */
    draftling_lvgl_port_lock(-1);
    show_msg_popup("Draftbox", text, false);
    /* "Git Sync (not configured)" may have just become configured. */
    if (ret == ESP_OK && s_menu_open) refresh_menu_items();
    draftling_lvgl_port_unlock();
    vTaskDelete(NULL);
}

static void draftbox_connect_async(const char *site, const char *otp)
{
    draftbox_connect_args_t *args =
        (draftbox_connect_args_t *)calloc(1, sizeof(draftbox_connect_args_t));
    if (!args) {
        show_msg_popup("Draftbox", "Could not connect: out of memory.", false);
        return;
    }
    snprintf(args->site, sizeof(args->site), "%s", site);
    snprintf(args->otp, sizeof(args->otp), "%s", otp);
    if (!context_project(args->project, sizeof(args->project))) {
        free(args);
        show_msg_popup("Draftbox", "Could not connect: select a project first.", false);
        return;
    }
    project_dir(args->project, args->dir, sizeof(args->dir));

    /* esp_http_client + mbedTLS handshake need more stack than the
     * WiFi tasks. */
    BaseType_t rc = xTaskCreatePinnedToCore(draftbox_connect_task, "draftbox",
                                            8 * 1024, args, 3, NULL, 0);
    if (rc != pdPASS) {
        memset(args, 0, sizeof(*args));
        free(args);
        show_msg_popup("Draftbox", "Could not connect: failed to start task.", false);
    }
}

/* ---- WiFi state callback ----
 * The WiFi manager calls wifi_state_cb() from the default event loop
 * task ("sys_evt"), whose stack is only ESP_SYSTEM_EVENT_TASK_STACK_SIZE
 * (2304 bytes). The handling below sets the status bar (text fitting
 * and glyph lookups through the whole font chain, plus HarfBuzz shaping
 * with the Indic pack), saves documents and starts a Git sync, which
 * overflowed that stack (Seeed reTerminal Sticky, Indic pack, on
 * "WiFi: connected"). So the callback only queues the state, and
 * wifi_state_apply() runs on the LVGL task (8-12 KB stack, LVGL lock
 * held). A FIFO keeps the order of states that arrive in quick
 * succession (lv_async_call() timers created in one cycle would not
 * run in creation order). */
#define WIFI_STATE_Q_LEN 8
static wifi_state_t s_wifi_state_q[WIFI_STATE_Q_LEN];
static int  s_wifi_state_q_head = 0;   /* next to apply */
static int  s_wifi_state_q_cnt  = 0;
static bool s_wifi_state_q_armed = false;

static void wifi_state_apply(wifi_state_t state);

/* lv_async_call() handler (LVGL task, lock held). */
static void wifi_state_drain(void *)
{
    s_wifi_state_q_armed = false;
    while (s_wifi_state_q_cnt > 0) {
        wifi_state_t st = s_wifi_state_q[s_wifi_state_q_head];
        s_wifi_state_q_head = (s_wifi_state_q_head + 1) % WIFI_STATE_Q_LEN;
        s_wifi_state_q_cnt--;
        wifi_state_apply(st);
    }
}

static void wifi_state_cb(wifi_state_t state)
{
    /* Generous timeout: an e-paper full refresh can hold the LVGL lock
     * for over a second, and a dropped CONNECTING / ERROR update would
     * leave the user with no idea what the device is doing. */
    if (!draftling_lvgl_port_lock(2000)) return;
    if (s_wifi_state_q_cnt == WIFI_STATE_Q_LEN) {
        /* Full: drop the oldest; the newest state is what matters. */
        s_wifi_state_q_head = (s_wifi_state_q_head + 1) % WIFI_STATE_Q_LEN;
        s_wifi_state_q_cnt--;
    }
    s_wifi_state_q[(s_wifi_state_q_head + s_wifi_state_q_cnt) % WIFI_STATE_Q_LEN] = state;
    s_wifi_state_q_cnt++;
    if (!s_wifi_state_q_armed &&
        lv_async_call(wifi_state_drain, NULL) == LV_RESULT_OK) {
        s_wifi_state_q_armed = true;
    }
    draftling_lvgl_port_unlock();
}

/* The UI side of a WiFi state change (LVGL task, LVGL lock held). */
static void wifi_state_apply(wifi_state_t state)
{
    /* While a Git sync is running, do not overwrite the on-screen
     * progress messages with WiFi state notifications -- the user
     * would otherwise see "WiFi: SSID (IP)" flicker in between
     * "Git: Pulling i/N" messages. Query the authoritative git_sync
     * state (set synchronously inside git_sync_start before the sync
     * task spawns) instead of a UI-side flag, so events that arrive
     * before the first GIT_SYNC_IN_PROGRESS callback are also
     * suppressed. The WiFi icon is still refreshed below so the
     * title bar stays accurate. */
    if (git_sync_get_state() != GIT_SYNC_IN_PROGRESS) {
        switch (state) {
        case WIFI_STATE_CONNECTED:
        {
            char buf[80];
            snprintf(buf, sizeof(buf), "WiFi: connected to %s (%s)",
                     wifi_manager_get_ssid(), wifi_manager_get_ip());
            editor_ui_set_status(buf);
            break;
        }
        case WIFI_STATE_CONNECTING:
        {
            /* Mirror the underlying "WiFi: connecting to <SSID>" log
             * message in the on-screen status bar so users see which
             * SSID we are negotiating with. */
            char buf[80];
            const char *ssid = wifi_manager_get_ssid();
            if (ssid && ssid[0]) {
                snprintf(buf, sizeof(buf), "WiFi: connecting to %s...", ssid);
            } else {
                snprintf(buf, sizeof(buf), "WiFi: connecting...");
            }
            /* No auto-clear: an attempt can take up to 30 s, and the
             * message must stay until CONNECTED / ERROR replaces it. */
            set_status_with_timeout(buf, 0);
            break;
        }
        case WIFI_STATE_ERROR:
        {
            char buf[80];
            const char *ssid = wifi_manager_get_ssid();
            bool auth = wifi_manager_last_failure_was_auth();
            if (ssid && ssid[0]) {
                snprintf(buf, sizeof(buf), auth ? "WiFi: wrong password for %s"
                                                : "WiFi: failed to connect to %s", ssid);
            } else if (auth) {
                snprintf(buf, sizeof(buf), "WiFi: wrong password");
            } else {
                snprintf(buf, sizeof(buf), "WiFi: connection failed");
            }
            set_status_with_timeout(buf, 5000);
            break;
        }
        case WIFI_STATE_DISCONNECTED:
            editor_ui_set_status("WiFi: disconnected");
            break;
        default:
            break;
        }
    }

    /* A Draftbox prompt waiting for this connection (independent of the
     * Git-sync status suppression above). DISCONNECTED is not a failure
     * here: a new attempt drops the old association first. */
    if (state == WIFI_STATE_CONNECTED) {
        dbx_wifi_done(true, NULL);
        sync_wifi_done(true);
    } else if (state == WIFI_STATE_ERROR) {
        sync_wifi_done(false);
        char reason[96];
        const char *ssid = wifi_manager_get_ssid();
        bool auth = wifi_manager_last_failure_was_auth();
        snprintf(reason, sizeof(reason), "%s%s%s.",
                 auth ? "Wrong password" : "Connection failed",
                 (ssid && ssid[0]) ? " for " : "",
                 (ssid && ssid[0]) ? ssid : "");
        dbx_wifi_done(false, reason);
    }

    update_wifi_icons();
}

/* ---- Git sync callback ----
 * Called from the git_sync task when the sync state changes.
 * Must take the LVGL lock before touching any UI objects. */
static void git_sync_cb(git_sync_state_t state, const char *message)
{
    /* A dropped progress message is harmless (the next one replaces
     * it), but the final SUCCESS / ERROR must never be dropped: the
     * progress messages have no auto-clear, so the last one would stay
     * on screen forever. On a slow e-paper panel (Seeed reTerminal
     * E1001: ~450 ms partial refresh) the flush of "Committing local
     * changes..." still holds the lock when a no-op sync finishes, so
     * wait for it unconditionally. This runs on the sync task (or with
     * the recursive lock already held), so blocking is safe. */
    bool final = (state == GIT_SYNC_SUCCESS || state == GIT_SYNC_ERROR);
    if (!draftling_lvgl_port_lock(final ? -1 : 200)) return;

    switch (state) {
    case GIT_SYNC_IN_PROGRESS:
    {
        char buf[80];
        snprintf(buf, sizeof(buf), "Git: %s",
                 message ? message : "syncing...");
        /* No auto-clear timeout: per-file progress messages must stay
         * visible until the next progress callback overwrites them or
         * the sync finishes. Otherwise the 3 s timer would revert the
         * status to the default hint mid-sync (e.g. between two slow
         * uploads), which looks like the sync stopped. The terminal
         * SUCCESS / ERROR branches install a 10 s timer so the final
         * summary still auto-clears. */
        set_status_with_timeout(buf, 0);
        break;
    }
    case GIT_SYNC_SUCCESS:
    {
        char buf[128];
        snprintf(buf, sizeof(buf), "Git: %s",
                 message ? message : "sync complete");
        /* If a file is currently open in the editor, reload it from disk
         * so the user sees any changes that were pulled from the remote. */
        const char *sdir = git_sync_get_project_dir();
        size_t sdl = strlen(sdir);
        const char *apath = editor_get_file_path();
        if (s_editor_screen_active && apath && sdl &&
            strncmp(apath, sdir, sdl) == 0 && apath[sdl] == '/') {
            const char *path = apath;
            esp_err_t rerr = editor_open_file(path);
            if (rerr == ESP_ERR_NO_MEM) {
                char msg[96];
                snprintf(msg, sizeof(msg),
                         "Reload failed: file too large (limit %u KB)",
                         (unsigned)(editor_get_max_doc_size() / 1024));
                editor_ui_set_status(msg);
            }
            editor_ui_refresh();
        }
        /* If the file browser is the active screen (sync was triggered
         * from there), rebuild its file list so newly pulled files
         * appear and deleted ones disappear. */
        if (lv_scr_act() == s_scr_browser) {
            refresh_file_list();
        }
        /* Show the final summary for 10 s -- long enough for the user
         * to read it without lingering forever. Set last so the reload
         * path above (which may call editor_ui_set_status on error)
         * does not race with the longer timeout. */
        set_status_with_timeout(buf, 10000);
        break;
    }
    case GIT_SYNC_ERROR:
    {
        char buf[128];
        snprintf(buf, sizeof(buf), "Git: %s",
                 message ? message : "error");
        set_status_with_timeout(buf, 10000);
        break;
    }
    default:
        break;
    }

    draftling_lvgl_port_unlock();
}

/* ---- Initialization ---- */

/* Build (or rebuild) every LVGL screen, overlay and timer that
 * displays the current theme. Called from editor_ui_init() at boot
 * and again from rebuild_screens_for_theme() after a theme change.
 *
 * Pre-condition: init_styles() has already been called so theme_fg()
 * / theme_bg() reflect the desired palette, and any previously-built
 * screens / overlays / timers have been deleted with their pointers
 * NULLed. */
static void build_screens(void)
{
    /* ---- Editor screen ---- */
    s_scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_scr, theme_bg(), 0);

    /* Title bar */
    s_lbl_title = lv_label_create(s_scr);
    lv_obj_set_pos(s_lbl_title, 2 + HDR_IN_L, HDR_TEXT_Y);
    lv_obj_set_width(s_lbl_title, TITLE_W);
    lv_obj_set_style_text_font(s_lbl_title, FONT_11, 0);
    lv_obj_set_style_text_color(s_lbl_title, theme_fg(), 0);
    /* One line only: update_title_bar() shortens long file names, and
     * clipping keeps anything it misjudges from wrapping over the
     * header rule. */
    lv_label_set_long_mode(s_lbl_title, LV_LABEL_LONG_CLIP);
    lv_label_set_text(s_lbl_title, "Draftling");

    /* Header separator line */
    lv_obj_t *hline = lv_obj_create(s_scr);
    s_editor_hline = hline;
    lv_obj_set_size(hline, CONTENT_W, 1);
    lv_obj_set_pos(hline, 0, HEADER_H - 1);
    lv_obj_set_style_bg_color(hline, theme_fg(), 0);
    lv_obj_set_style_bg_opa(hline, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(hline, 0, 0);
    lv_obj_set_style_radius(hline, 0, 0);
    lv_obj_set_style_pad_all(hline, 0, 0);

    /* Editor content area -- one container per pane. Both panes are
     * created up-front; layout_panes() positions/sizes them and hides
     * the second one while the editor is in single-pane mode. Each
     * pane's per-document widgets (logo, selection overlays, cursor)
     * are created into its own container by binding the pane first. */
    for (int p = 0; p < EDITOR_MAX_PANES; p++) {
        pane_bind(p);

        s_cont_edit = lv_obj_create(s_scr);
        lv_obj_set_pos(s_cont_edit, 0, EDITOR_Y);
        lv_obj_set_size(s_cont_edit, CONTENT_W, EDITOR_H);
        lv_obj_set_style_bg_opa(s_cont_edit, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(s_cont_edit, 0, 0);
        lv_obj_set_style_pad_all(s_cont_edit, 0, 0);
        lv_obj_set_style_radius(s_cont_edit, 0, 0);
        lv_obj_remove_flag(s_cont_edit, LV_OBJ_FLAG_SCROLLABLE);

#if defined(CONFIG_DRAFTLING_TOUCHSCREEN)
        /* Route LVGL pointer events (tap, double-tap, drag-to-scroll,
         * swipe gesture) on the editor area to editor_touch_event_cb.
         * The container is CLICKABLE so it receives press / pressing /
         * released / clicked / gesture events from the indev. Using
         * LV_EVENT_ALL is simpler than registering each subtype and
         * lets editor_touch_event_cb filter by code. The pane index is
         * stored in user_data so the callback can focus the tapped
         * pane. */
        lv_obj_add_flag(s_cont_edit, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(s_cont_edit, editor_touch_event_cb,
                            LV_EVENT_ALL, (void *)(intptr_t)p);
#endif

        /* "draftling" text label shown when no file is open */
        s_img_logo = lv_label_create(s_cont_edit);
        lv_obj_set_width(s_img_logo, CONTENT_W);
        lv_obj_set_style_text_font(s_img_logo, FONT_18, 0);
        lv_obj_set_style_text_color(s_img_logo, lv_color_make(0x80, 0x80, 0x80), 0);
        lv_obj_set_style_text_align(s_img_logo, LV_TEXT_ALIGN_CENTER, 0);
        lv_label_set_text(s_img_logo, "draftling");
        lv_obj_set_pos(s_img_logo, 0,
                       (EDITOR_H - lv_font_get_line_height(FONT_18)) / 2);
        lv_obj_add_flag(s_img_logo, LV_OBJ_FLAG_HIDDEN);

        /* Cursor (thin vertical bar) */
        s_cursor = lv_obj_create(s_cont_edit);
        lv_obj_set_size(s_cursor, 2, LINE_H);
        lv_obj_set_style_bg_color(s_cursor, theme_fg(), 0);
        lv_obj_set_style_bg_opa(s_cursor, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(s_cursor, 0, 0);
        lv_obj_set_style_radius(s_cursor, 0, 0);
        lv_obj_set_style_pad_all(s_cursor, 0, 0);
    }
    pane_bind_focus();

    /* Divider drawn between the two panes when split -- vertical in
     * landscape, horizontal in portrait. layout_panes() sizes and
     * positions it for the current orientation; these are just
     * placeholder values. */
    s_pane_divider = lv_obj_create(s_scr);
    lv_obj_set_size(s_pane_divider, 1, EDITOR_H);
    lv_obj_set_pos(s_pane_divider, CONTENT_W / 2, EDITOR_Y);
    lv_obj_set_style_bg_color(s_pane_divider, theme_fg(), 0);
    lv_obj_set_style_bg_opa(s_pane_divider, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_pane_divider, 0, 0);
    lv_obj_set_style_radius(s_pane_divider, 0, 0);
    lv_obj_set_style_pad_all(s_pane_divider, 0, 0);
    lv_obj_add_flag(s_pane_divider, LV_OBJ_FLAG_HIDDEN);

    /* Apply the current split geometry (positions/sizes both panes and
     * the divider; hides pane 1 + the divider in single-pane mode). */
    layout_panes();

    /* Status bar */
    lv_obj_t *sline = lv_obj_create(s_scr);
    s_editor_sline = sline;
    lv_obj_set_size(sline, CONTENT_W, 1);
    lv_obj_set_pos(sline, 0, SCR_H - STATUS_H);
    lv_obj_set_style_bg_color(sline, theme_fg(), 0);
    lv_obj_set_style_bg_opa(sline, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sline, 0, 0);
    lv_obj_set_style_radius(sline, 0, 0);
    lv_obj_set_style_pad_all(sline, 0, 0);

    s_lbl_status = lv_label_create(s_scr);
    lv_obj_set_pos(s_lbl_status, 2 + STA_IN_L, SCR_H - STATUS_H + 2);
    /* Width is constrained below (see status_avail_width()) so the
     * label never reaches the battery / wifi icon on the right and
     * never wraps to a second line (which would extend past SCR_H
     * and trigger LVGL's screen scrollbar on narrow color LCDs). */
    lv_label_set_long_mode(s_lbl_status, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(s_lbl_status, status_avail_width());
    lv_obj_set_style_text_font(s_lbl_status, FONT_11, 0);
    lv_obj_set_style_text_color(s_lbl_status, theme_fg(), 0);
    lv_label_set_text(s_lbl_status, "F1:Menu Esc:Files");
    update_status_visibility();

#if defined(CONFIG_DRAFTLING_HAS_BATTERY)
    /* Device battery label (right-aligned in editor status bar) */
    s_lbl_dev_batt = lv_label_create(s_scr);
    lv_obj_set_style_text_font(s_lbl_dev_batt, FONT_11, 0);
    lv_obj_set_style_text_color(s_lbl_dev_batt, theme_fg(), 0);
    lv_obj_set_style_text_align(s_lbl_dev_batt, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_pos(s_lbl_dev_batt, CONTENT_W - 80 - STA_IN_R, SCR_H - STATUS_H + 2);
    lv_obj_set_width(s_lbl_dev_batt, 78);
    lv_label_set_text(s_lbl_dev_batt, "");
#define WIFI_ICON_RIGHT_OFFSET 95   /* leave room for battery on its left */
#else
#define WIFI_ICON_RIGHT_OFFSET 15
#endif

    /* WiFi connectivity icon (right corner of editor status bar).
     * Source image and vertical centering (which varies with icon
     * height) are picked by update_wifi_icons(), called once both
     * status bars exist; only the fixed horizontal position is set
     * here. */
    s_img_wifi = lv_image_create(s_scr);
    lv_obj_set_x(s_img_wifi, CONTENT_W - WIFI_ICON_RIGHT_OFFSET - STA_IN_R);
    lv_obj_add_flag(s_img_wifi, LV_OBJ_FLAG_HIDDEN);
#undef WIFI_ICON_RIGHT_OFFSET

    /* Cursor blink timer.
     *
     * On e-paper backends a 500 ms blink causes a full panel refresh
     * twice per second, which is both visually distracting and bad
     * for the panel. Keep the cursor solid (always visible) on EPD
     * targets; only the reflective LCD blinks. */
#if defined(CONFIG_DRAFTLING_DISPLAY_EPD)
    s_cursor_visible = true;
    if (s_cursor) lv_obj_remove_flag(s_cursor, LV_OBJ_FLAG_HIDDEN);
#else
    s_blink_timer = lv_timer_create(cursor_blink_cb, 500, NULL);
#endif

    /* ---- File browser screen ---- */
    s_scr_browser = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_scr_browser, theme_bg(), 0);

    lv_obj_t *br_title = lv_label_create(s_scr_browser);
    lv_obj_set_pos(br_title, 2 + HDR_IN_L, HDR_TEXT_Y);
    corner_fit_header(br_title);
    lv_obj_set_style_text_font(br_title, FONT_11, 0);
    lv_obj_set_style_text_color(br_title, theme_fg(), 0);
    lv_label_set_text(br_title, "File Browser - Up/Down, Enter to open, N for new");

    s_list_files = lv_list_create(s_scr_browser);
    lv_obj_set_pos(s_list_files, 0, HEADER_H);
    lv_obj_set_size(s_list_files, CONTENT_W, LIST_PANEL_H - STATUS_H);
    lv_obj_set_style_border_width(s_list_files, 0, 0);
    lv_obj_set_style_radius(s_list_files, 0, 0);
    lv_obj_set_style_pad_all(s_list_files, 0, 0);
    lv_obj_set_style_text_font(s_list_files, FONT_14, 0);
    /* lv_list defaults to LVGL theme background (white). Force it to
     * the editor theme background so the list panel matches the
     * surrounding screen when CONFIG_DRAFTLING_EPD_BLACK_BACKGROUND
     * is enabled (otherwise the list paints a white rectangle on top
     * of the black screen and white-on-white text is invisible). */
    lv_obj_set_style_bg_color(s_list_files, theme_bg(), 0);

    /* The shared browser logic operates on s_browser_list; in the
     * default (single-pane) case that is the full-screen list. */
    s_browser_list = s_list_files;

    /* In-pane file selector overlay (split mode). Parented to the main
     * editor screen as siblings of the pane containers so they can be
     * positioned over the focused pane and raised to the foreground;
     * created hidden and positioned on demand by show_inpane_browser(). */
    s_inpane_hdr = lv_label_create(s_scr);
    lv_obj_set_style_text_font(s_inpane_hdr, FONT_11, 0);
    lv_obj_set_style_text_color(s_inpane_hdr, theme_fg(), 0);
    lv_obj_set_style_bg_color(s_inpane_hdr, theme_bg(), 0);
    lv_obj_set_style_bg_opa(s_inpane_hdr, LV_OPA_COVER, 0);
    lv_label_set_text(s_inpane_hdr, "Open into pane");
    lv_obj_add_flag(s_inpane_hdr, LV_OBJ_FLAG_HIDDEN);

    s_inpane_list = lv_list_create(s_scr);
    lv_obj_set_style_border_width(s_inpane_list, 0, 0);
    lv_obj_set_style_radius(s_inpane_list, 0, 0);
    lv_obj_set_style_pad_all(s_inpane_list, 0, 0);
    lv_obj_set_style_text_font(s_inpane_list, FONT_14, 0);
    lv_obj_set_style_bg_color(s_inpane_list, theme_bg(), 0);
    lv_obj_set_style_bg_opa(s_inpane_list, LV_OPA_COVER, 0);
    lv_obj_add_flag(s_inpane_list, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *br_sline = lv_obj_create(s_scr_browser);
    s_br_sline = br_sline;
    lv_obj_set_size(br_sline, CONTENT_W, 1);
    lv_obj_set_pos(br_sline, 0, SCR_H - STATUS_H);
    lv_obj_set_style_bg_color(br_sline, theme_fg(), 0);
    lv_obj_set_style_bg_opa(br_sline, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(br_sline, 0, 0);
    lv_obj_set_style_radius(br_sline, 0, 0);
    lv_obj_set_style_pad_all(br_sline, 0, 0);

    s_lbl_br_status = lv_label_create(s_scr_browser);
    lv_obj_set_pos(s_lbl_br_status, 2 + STA_IN_L, SCR_H - STATUS_H + 2);
    lv_obj_set_width(s_lbl_br_status, CONTENT_W - 4 - STA_IN_L - STA_IN_R);
    lv_obj_set_style_text_font(s_lbl_br_status, FONT_11, 0);
    lv_obj_set_style_text_color(s_lbl_br_status, theme_fg(), 0);
    lv_label_set_text(s_lbl_br_status, "F1:Menu  N:New  F2:Rename");

#if defined(CONFIG_DRAFTLING_HAS_BATTERY)
    /* Device battery label (right-aligned in browser status bar) */
    s_lbl_br_dev_batt = lv_label_create(s_scr_browser);
    lv_obj_set_style_text_font(s_lbl_br_dev_batt, FONT_11, 0);
    lv_obj_set_style_text_color(s_lbl_br_dev_batt, theme_fg(), 0);
    lv_obj_set_style_text_align(s_lbl_br_dev_batt, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_pos(s_lbl_br_dev_batt, CONTENT_W - 80 - STA_IN_R, SCR_H - STATUS_H + 2);
    lv_obj_set_width(s_lbl_br_dev_batt, 78);
    lv_label_set_text(s_lbl_br_dev_batt, "");

#if !defined(DRAFTLING_BATT_PULL_MODE)
    /* Battery poll timer. The first reading is painted at the end of
     * build_screens(), once every battery label exists. The H752 has
     * no periodic timer (see the declarations above); its updates
     * ride redraw points. */
    s_batt_timer = lv_timer_create(batt_timer_cb, BATT_POLL_MS, NULL);
#endif
#define WIFI_ICON_RIGHT_OFFSET 95
#else
#define WIFI_ICON_RIGHT_OFFSET 15
#endif

    /* WiFi connectivity icon (right corner of browser status bar).
     * See the matching editor-screen icon above: update_wifi_icons()
     * picks the source image and vertical position. */
    s_img_br_wifi = lv_image_create(s_scr_browser);
    lv_obj_set_x(s_img_br_wifi, CONTENT_W - WIFI_ICON_RIGHT_OFFSET - STA_IN_R);
    lv_obj_add_flag(s_img_br_wifi, LV_OBJ_FLAG_HIDDEN);
#undef WIFI_ICON_RIGHT_OFFSET

    /* Reflect any pre-existing WiFi state (in case the manager
     * fired its callback before the UI was ready). */
    update_wifi_icons();

    /* Register keyboard callback (BLE + USB feed the same handler;
     * only one is wired at a time, chosen by main.cpp based on which
     * keyboard was present at boot). */
    ble_keyboard_set_callback((kb_event_callback_t)editor_ui_handle_key);
#if defined(CONFIG_DRAFTLING_HAS_USB_HOST)
    usb_kbd_set_callback((kb_event_callback_t)editor_ui_handle_key);
#endif
#if defined(CONFIG_DRAFTLING_HAS_TAB5_KBD)
    /* Tab5 attachable keyboard feeds the same handler. It coexists
     * with USB / BLE rather than replacing them: on the Tab5 the
     * built-in keyboard and a USB keyboard are both accepted. */
    tab5_kbd_set_callback((kb_event_callback_t)editor_ui_handle_key);
#endif

    /* ---- BLE connection prompt screen ---- */
    s_scr_ble_prompt = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_scr_ble_prompt, theme_bg(), 0);
    /* The default theme on lv_obj_create() puts a few pixels of
     * padding and a SCROLLABLE flag on the screen. Both interfere
     * with the absolutely-positioned children below: the padding
     * shifts every lv_obj_set_pos() by ~12px (so the upper-right
     * Off button drifts partly off the right edge of the screen
     * and its visible portion no longer matches its hit area),
     * and SCROLLABLE turns any small touch jitter into a scroll
     * gesture that swallows the Off button's CLICKED event. Pin
     * the screen to a 0-padding, non-scrollable surface so the
     * children behave the same way they do on the editor /
     * browser screens, where the same off-the-shelf positions
     * have always worked. */
    lv_obj_set_style_pad_all(s_scr_ble_prompt, 0, 0);
    lv_obj_set_style_border_width(s_scr_ble_prompt, 0, 0);
    lv_obj_set_scrollbar_mode(s_scr_ble_prompt, LV_SCROLLBAR_MODE_OFF);
    lv_obj_remove_flag(s_scr_ble_prompt, LV_OBJ_FLAG_SCROLLABLE);

    /* "draftling" title centered near the top */
    {
        lv_obj_t *title = lv_label_create(s_scr_ble_prompt);
        lv_obj_set_width(title, SCR_W);
        lv_obj_set_style_text_font(title, FONT_18, 0);
        lv_obj_set_style_text_color(title, theme_fg(), 0);
        lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
        lv_label_set_text(title, "draftling");
        int fh = lv_font_get_line_height(FONT_18);
        lv_obj_set_pos(title, 0, SCR_H / 4 - fh / 2);
    }

    /* Prompt label below center */
    s_ble_prompt_lbl = lv_label_create(s_scr_ble_prompt);
    lv_obj_set_width(s_ble_prompt_lbl, SCR_W - 20);
    lv_obj_set_style_text_font(s_ble_prompt_lbl, FONT_14, 0);
    lv_obj_set_style_text_color(s_ble_prompt_lbl, theme_fg(), 0);
    lv_obj_set_style_text_align(s_ble_prompt_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(s_ble_prompt_lbl, "Initializing...");
    lv_obj_set_pos(s_ble_prompt_lbl, 10, SCR_H / 2 + 10);

    /* Bottom status-bar separator line (matches editor / browser
     * screens), with a battery percentage label on the right when
     * the board has a battery monitor. Lets the user see remaining
     * charge while the firmware is still scanning for a BLE keyboard
     * or after the keyboard has disconnected. */
    {
        lv_obj_t *ble_sline = lv_obj_create(s_scr_ble_prompt);
        lv_obj_set_size(ble_sline, SCR_W, 1);
        lv_obj_set_pos(ble_sline, 0, SCR_H - STATUS_H);
        lv_obj_set_style_bg_color(ble_sline, theme_fg(), 0);
        lv_obj_set_style_bg_opa(ble_sline, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(ble_sline, 0, 0);
        lv_obj_set_style_radius(ble_sline, 0, 0);
        lv_obj_set_style_pad_all(ble_sline, 0, 0);
    }

#if defined(CONFIG_DRAFTLING_HAS_BATTERY)
    /* Device battery label (right-aligned in BLE prompt status bar).
     * Painted with the first reading by battery_labels_repaint() at
     * the end of build_screens(), then kept current by the battery
     * poll timer. */
    s_lbl_ble_dev_batt = lv_label_create(s_scr_ble_prompt);
    lv_obj_set_style_text_font(s_lbl_ble_dev_batt, FONT_11, 0);
    lv_obj_set_style_text_color(s_lbl_ble_dev_batt, theme_fg(), 0);
    lv_obj_set_style_text_align(s_lbl_ble_dev_batt, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_pos(s_lbl_ble_dev_batt, SCR_W - 80 - STA_IN_R, SCR_H - STATUS_H + 2);
    lv_obj_set_width(s_lbl_ble_dev_batt, 78);
    lv_label_set_text(s_lbl_ble_dev_batt, "");
#endif

#if defined(CONFIG_DRAFTLING_TOUCHSCREEN)
    /* Power-off button in the upper-right corner. Tapping it sends
     * the device into deep sleep immediately. Only useful on touch
     * builds because there is no keyboard available yet on this
     * screen (we are scanning for it). standby_enter_sleep() runs
     * the registered pre-sleep callback (autosave + per-board
     * peripheral teardown) before esp_deep_sleep_start(). */
    /* Size both buttons from the label font's metrics so the labels
     * fit their frames on every font family (Hack on HIDPI boards is
     * about twice the size of Greybeard). The buttons are finger-sized:
     * twice the line height tall, two cells of padding on each side
     * and a two-cell gap between them, the padding shrunk only when
     * both would not fit the screen width (narrow portrait panels). */
    const lv_font_t *ble_btn_font = FONT_16;
    const int ble_btn_cw  = char_width_for_font(ble_btn_font);
    const int ble_btn_lh  = lv_font_get_line_height(ble_btn_font);
    const int ble_btn_h   = 2 * ble_btn_lh + 4;
    const int ble_btn_gap = 2 * ble_btn_cw;
    int ble_btn_pad = 2 * ble_btn_cw;
    int ble_btn_room = (int)SCR_W - 8 - ble_btn_gap - 12 * ble_btn_cw;
    if (4 * ble_btn_pad > ble_btn_room) ble_btn_pad = ble_btn_room > 8 ? ble_btn_room / 4 : 2;
    const int ble_off_w  = 3 * ble_btn_cw + 2 * ble_btn_pad;  /* "Off"       */
#if !defined(CONFIG_DRAFTLING_NO_BLUETOOTH)
    const int ble_fgt_w  = 9 * ble_btn_cw + 2 * ble_btn_pad;  /* "Forget KB" */
#endif
    {
        lv_obj_t *off_btn = lv_button_create(s_scr_ble_prompt);
        lv_obj_set_size(off_btn, ble_off_w, ble_btn_h);
        /* Anchor to the top-right corner with a small inset so the
         * button stays fully on-screen and its hit area matches the
         * pixels the user sees, independent of the parent screen's
         * padding (which the default LVGL theme may bump non-zero
         * on a future LVGL release). */
        lv_obj_align(off_btn, LV_ALIGN_TOP_RIGHT, -4, 4);
        lv_obj_set_style_bg_color(off_btn, theme_bg(), 0);
        lv_obj_set_style_bg_opa(off_btn, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(off_btn, theme_fg(), 0);
        lv_obj_set_style_border_width(off_btn, 1, 0);
        lv_obj_set_style_border_opa(off_btn, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(off_btn, 4, 0);
        lv_obj_set_style_pad_all(off_btn, 0, 0);
        lv_obj_add_event_cb(off_btn, ble_prompt_off_btn_cb,
                            LV_EVENT_CLICKED, NULL);

        lv_obj_t *off_lbl = lv_label_create(off_btn);
        lv_obj_set_style_text_font(off_lbl, ble_btn_font, 0);
        lv_obj_set_style_text_color(off_lbl, theme_fg(), 0);
        lv_label_set_text(off_lbl, "Off");
        lv_obj_center(off_lbl);
    }

    /* "Forget KB" button to the left of the "Off" button. Clears all
     * stored keyboard pairings and starts a fresh scan, allowing the
     * user to pair a different keyboard without needing a physical
     * keyboard to navigate the settings menu. Not without Bluetooth
     * (e.g. a USB keyboard host build): there is nothing to forget. */
#if !defined(CONFIG_DRAFTLING_NO_BLUETOOTH)
    {
        lv_obj_t *fgt_btn = lv_button_create(s_scr_ble_prompt);
        lv_obj_set_size(fgt_btn, ble_fgt_w, ble_btn_h);
        /* Place to the left of the "Off" button: Off right edge is 4px
         * from screen right; its left edge is (4 + ble_off_w) from
         * right, plus the gap. */
        lv_obj_align(fgt_btn, LV_ALIGN_TOP_RIGHT, -(ble_off_w + 4 + ble_btn_gap), 4);
        lv_obj_set_style_bg_color(fgt_btn, theme_bg(), 0);
        lv_obj_set_style_bg_opa(fgt_btn, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(fgt_btn, theme_fg(), 0);
        lv_obj_set_style_border_width(fgt_btn, 1, 0);
        lv_obj_set_style_border_opa(fgt_btn, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(fgt_btn, 4, 0);
        lv_obj_set_style_pad_all(fgt_btn, 0, 0);
        lv_obj_add_event_cb(fgt_btn, ble_prompt_forget_btn_cb,
                            LV_EVENT_CLICKED, NULL);

        lv_obj_t *fgt_lbl = lv_label_create(fgt_btn);
        lv_obj_set_style_text_font(fgt_lbl, ble_btn_font, 0);
        lv_obj_set_style_text_color(fgt_lbl, theme_fg(), 0);
        lv_label_set_text(fgt_lbl, "Forget KB");
        lv_obj_center(fgt_lbl);
    }
#endif /* !CONFIG_DRAFTLING_NO_BLUETOOTH */
#endif

    /* Register BLE status callbacks */
    ble_keyboard_set_connect_callback(ble_connect_status_cb);
    ble_keyboard_set_status_text_callback(ble_status_text_cb);
#if defined(CONFIG_DRAFTLING_HAS_USB_HOST)
    /* USB keyboard hot-plug shares the same connect-state plumbing
     * so a USB hot-plug arriving after a BLE disconnect dismisses
     * the "Keyboard disconnected / Reconnecting..." prompt screen
     * and restores the editor or file browser. */
    usb_kbd_set_connect_callback(ble_connect_status_cb);
#endif

    /* ---- Menu screen ---- */
    s_scr_menu = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_scr_menu, theme_bg(), 0);

    s_lbl_menu_hdr = lv_label_create(s_scr_menu);
    lv_obj_set_pos(s_lbl_menu_hdr, 2 + HDR_IN_L, HDR_TEXT_Y);
    corner_fit_header(s_lbl_menu_hdr);
    lv_obj_set_style_text_font(s_lbl_menu_hdr, FONT_11, 0);
    lv_obj_set_style_text_color(s_lbl_menu_hdr, theme_fg(), 0);
    lv_label_set_text(s_lbl_menu_hdr, MENU_HDR_TEXT);

    s_menu_list = lv_list_create(s_scr_menu);
    lv_obj_set_pos(s_menu_list, 0, HEADER_H);
    lv_obj_set_size(s_menu_list, SCR_W, LIST_PANEL_H - STATUS_H);
    lv_obj_set_style_border_width(s_menu_list, 0, 0);
    lv_obj_set_style_radius(s_menu_list, 0, 0);
    lv_obj_set_style_pad_all(s_menu_list, 0, 0);
    lv_obj_set_style_text_font(s_menu_list, FONT_14, 0);
    /* See note on s_list_files above: override the LVGL default white
     * list background so the menu blends with the screen in
     * CONFIG_DRAFTLING_EPD_BLACK_BACKGROUND mode. */
    lv_obj_set_style_bg_color(s_menu_list, theme_bg(), 0);

    /* Bottom row: firmware version, mirroring the file browser's
     * bottom status label above. esp_app_get_description()->version
     * is embedded in the app image from CMakeLists.txt's PROJECT_VER
     * at build time. */
    lv_obj_t *menu_version_lbl = lv_label_create(s_scr_menu);
    lv_obj_set_pos(menu_version_lbl, 2 + STA_IN_L, SCR_H - STATUS_H + 2);
    lv_obj_set_width(menu_version_lbl, SCR_W - 4 - STA_IN_L - STA_IN_R);
    lv_obj_set_style_text_font(menu_version_lbl, FONT_11, 0);
    lv_obj_set_style_text_color(menu_version_lbl, theme_fg(), 0);
    {
        char ver_buf[48];
        snprintf(ver_buf, sizeof(ver_buf), "Draftling v%s",
                 esp_app_get_description()->version);
        lv_label_set_text(menu_version_lbl, ver_buf);
    }

    /* ---- Help screen (F10) ---- */
    s_scr_help = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_scr_help, theme_bg(), 0);

    s_lbl_help_hdr = lv_label_create(s_scr_help);
    lv_obj_set_pos(s_lbl_help_hdr, 2 + HDR_IN_L, HDR_TEXT_Y);
    lv_obj_set_width(s_lbl_help_hdr, SCR_W - 4 - HDR_IN_L - HDR_IN_R);
    lv_label_set_long_mode(s_lbl_help_hdr, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(s_lbl_help_hdr, FONT_11, 0);
    lv_obj_set_style_text_color(s_lbl_help_hdr, theme_fg(), 0);
    lv_label_set_text(s_lbl_help_hdr,
                      "Help - Up/Down/PgUp/PgDn to scroll, Esc or Enter to close");

    s_help_cont = lv_obj_create(s_scr_help);
    lv_obj_set_pos(s_help_cont, 0, HEADER_H);
    lv_obj_set_size(s_help_cont, SCR_W, LIST_PANEL_H);
    lv_obj_set_style_border_width(s_help_cont, 0, 0);
    lv_obj_set_style_radius(s_help_cont, 0, 0);
    lv_obj_set_style_pad_all(s_help_cont, 0, 0);
    lv_obj_set_style_pad_left(s_help_cont, 4, 0);
    lv_obj_set_style_pad_right(s_help_cont, 4, 0);
    lv_obj_set_style_bg_color(s_help_cont, theme_bg(), 0);
    lv_obj_set_scroll_dir(s_help_cont, LV_DIR_VER);
#if defined(CONFIG_DRAFTLING_TOUCHSCREEN)
    lv_obj_add_event_cb(s_help_cont, help_touch_cb, LV_EVENT_CLICKED, NULL);
#endif

    s_help_lbl = lv_label_create(s_help_cont);
    lv_obj_set_width(s_help_lbl, LV_PCT(100));
    lv_label_set_long_mode(s_help_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(s_help_lbl, FONT_14, 0);
    lv_obj_set_style_text_color(s_help_lbl, theme_fg(), 0);

    /* ---- WiFi password-entry overlay (shown on the menu screen) ----
     * Structurally a copy of the save-as prompt below (header / value
     * label / cursor bar), but parented to s_scr_menu since that is
     * where the WiFi scan/connect flow lives, not the editor screen. */
    {
        int wpw_row_y   = overlay_row_y();
        int wpw_line_h  = lv_font_get_line_height(FONT_11);
        int wpw_panel_h = wpw_row_y + wpw_line_h + 2 * OVERLAY_PAD;
        s_wifi_pw_panel = lv_obj_create(s_scr_menu);
        lv_obj_set_size(s_wifi_pw_panel, SCR_W - 20, wpw_panel_h);
        lv_obj_set_pos(s_wifi_pw_panel, 10, (SCR_H - wpw_panel_h) / 2);
        lv_obj_set_style_bg_color(s_wifi_pw_panel, theme_bg(), 0);
        lv_obj_set_style_bg_opa(s_wifi_pw_panel, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(s_wifi_pw_panel, theme_fg(), 0);
        lv_obj_set_style_border_width(s_wifi_pw_panel, 2, 0);
        lv_obj_set_style_radius(s_wifi_pw_panel, 4, 0);
        lv_obj_set_style_pad_all(s_wifi_pw_panel, OVERLAY_PAD, 0);
        lv_obj_remove_flag(s_wifi_pw_panel, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(s_wifi_pw_panel, LV_OBJ_FLAG_HIDDEN);

        s_wifi_pw_hdr_lbl = lv_label_create(s_wifi_pw_panel);
        lv_obj_set_style_text_font(s_wifi_pw_hdr_lbl, FONT_11, 0);
        lv_obj_set_style_text_color(s_wifi_pw_hdr_lbl, theme_fg(), 0);
        lv_label_set_long_mode(s_wifi_pw_hdr_lbl, LV_LABEL_LONG_CLIP);
        lv_obj_set_width(s_wifi_pw_hdr_lbl, SCR_W - 20 - 12);
        lv_label_set_text(s_wifi_pw_hdr_lbl, "Password (Enter/Esc):");
        lv_obj_set_pos(s_wifi_pw_hdr_lbl, 0, 0);

        s_wifi_pw_name_lbl = lv_label_create(s_wifi_pw_panel);
        lv_obj_set_style_text_font(s_wifi_pw_name_lbl, FONT_11, 0);
        lv_obj_set_style_text_color(s_wifi_pw_name_lbl, theme_fg(), 0);
        lv_obj_set_width(s_wifi_pw_name_lbl, SCR_W - 20 - 12);
        lv_label_set_text(s_wifi_pw_name_lbl, "");
        lv_obj_set_pos(s_wifi_pw_name_lbl, 0, wpw_row_y);

        s_wifi_pw_cur = lv_obj_create(s_wifi_pw_panel);
        lv_obj_set_size(s_wifi_pw_cur, 2, wpw_line_h);
        lv_obj_set_style_bg_color(s_wifi_pw_cur, theme_fg(), 0);
        lv_obj_set_style_bg_opa(s_wifi_pw_cur, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(s_wifi_pw_cur, 0, 0);
        lv_obj_set_style_radius(s_wifi_pw_cur, 0, 0);
        lv_obj_set_style_pad_all(s_wifi_pw_cur, 0, 0);
        lv_obj_add_flag(s_wifi_pw_cur, LV_OBJ_FLAG_HIDDEN);
    }

    /* ---- Draftbox site / password overlay (menu screen) ----
     * Same layout as the WiFi password overlay above. */
    {
        int dbx_row_y   = overlay_row_y();
        int dbx_line_h  = lv_font_get_line_height(FONT_11);
        int dbx_panel_h = dbx_row_y + dbx_line_h + 2 * OVERLAY_PAD;
        s_dbx_panel = lv_obj_create(s_scr_menu);
        lv_obj_set_size(s_dbx_panel, SCR_W - 20, dbx_panel_h);
        lv_obj_set_pos(s_dbx_panel, 10, (SCR_H - dbx_panel_h) / 2);
        lv_obj_set_style_bg_color(s_dbx_panel, theme_bg(), 0);
        lv_obj_set_style_bg_opa(s_dbx_panel, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(s_dbx_panel, theme_fg(), 0);
        lv_obj_set_style_border_width(s_dbx_panel, 2, 0);
        lv_obj_set_style_radius(s_dbx_panel, 4, 0);
        lv_obj_set_style_pad_all(s_dbx_panel, OVERLAY_PAD, 0);
        lv_obj_remove_flag(s_dbx_panel, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(s_dbx_panel, LV_OBJ_FLAG_HIDDEN);

        s_dbx_hdr_lbl = lv_label_create(s_dbx_panel);
        lv_obj_set_style_text_font(s_dbx_hdr_lbl, FONT_11, 0);
        lv_obj_set_style_text_color(s_dbx_hdr_lbl, theme_fg(), 0);
        lv_label_set_long_mode(s_dbx_hdr_lbl, LV_LABEL_LONG_CLIP);
        lv_obj_set_width(s_dbx_hdr_lbl, SCR_W - 20 - 12);
        lv_label_set_text(s_dbx_hdr_lbl, "");
        lv_obj_set_pos(s_dbx_hdr_lbl, 0, 0);

        s_dbx_val_lbl = lv_label_create(s_dbx_panel);
        lv_obj_set_style_text_font(s_dbx_val_lbl, FONT_11, 0);
        lv_obj_set_style_text_color(s_dbx_val_lbl, theme_fg(), 0);
        lv_obj_set_width(s_dbx_val_lbl, SCR_W - 20 - 12);
        lv_label_set_text(s_dbx_val_lbl, "");
        lv_obj_set_pos(s_dbx_val_lbl, 0, dbx_row_y);

        s_dbx_cur = lv_obj_create(s_dbx_panel);
        lv_obj_set_size(s_dbx_cur, 2, dbx_line_h);
        lv_obj_set_style_bg_color(s_dbx_cur, theme_fg(), 0);
        lv_obj_set_style_bg_opa(s_dbx_cur, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(s_dbx_cur, 0, 0);
        lv_obj_set_style_radius(s_dbx_cur, 0, 0);
        lv_obj_set_style_pad_all(s_dbx_cur, 0, 0);
        lv_obj_add_flag(s_dbx_cur, LV_OBJ_FLAG_HIDDEN);
    }

    /* ---- Settings screen ---- */
    s_scr_settings = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_scr_settings, theme_bg(), 0);

    s_lbl_settings_hdr = lv_label_create(s_scr_settings);
    lv_obj_set_pos(s_lbl_settings_hdr, 2 + HDR_IN_L, HDR_TEXT_Y);
    corner_fit_header(s_lbl_settings_hdr);
    lv_obj_set_style_text_font(s_lbl_settings_hdr, FONT_11, 0);
    lv_obj_set_style_text_color(s_lbl_settings_hdr, theme_fg(), 0);
    lv_label_set_text(s_lbl_settings_hdr, settings_level_header(SET_LVL_ROOT));

    s_settings_list = lv_list_create(s_scr_settings);
    lv_obj_set_pos(s_settings_list, 0, HEADER_H);
    lv_obj_set_size(s_settings_list, SCR_W, LIST_PANEL_H);
    lv_obj_set_style_border_width(s_settings_list, 0, 0);
    lv_obj_set_style_radius(s_settings_list, 0, 0);
    lv_obj_set_style_pad_all(s_settings_list, 0, 0);
    lv_obj_set_style_text_font(s_settings_list, FONT_14, 0);
    /* See note on s_list_files above. */
    lv_obj_set_style_bg_color(s_settings_list, theme_bg(), 0);

    /* ---- Passkey overlay (moved onto the active screen when shown) ---- */
    s_passkey_panel = lv_obj_create(s_scr);
    lv_obj_set_size(s_passkey_panel, SCR_W - 20, LV_SIZE_CONTENT);
    lv_obj_center(s_passkey_panel);
    lv_obj_set_style_bg_color(s_passkey_panel, theme_bg(), 0);
    lv_obj_set_style_bg_opa(s_passkey_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(s_passkey_panel, theme_fg(), 0);
    lv_obj_set_style_border_width(s_passkey_panel, 2, 0);
    lv_obj_set_style_radius(s_passkey_panel, 4, 0);
    lv_obj_set_style_pad_all(s_passkey_panel, 6, 0);
    lv_obj_remove_flag(s_passkey_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_passkey_panel, LV_OBJ_FLAG_HIDDEN);

    s_passkey_label = lv_label_create(s_passkey_panel);
    lv_obj_set_style_text_font(s_passkey_label, FONT_16, 0);
    lv_obj_set_style_text_color(s_passkey_label, theme_fg(), 0);
    lv_obj_set_style_text_align(s_passkey_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_passkey_label, LV_PCT(100));
    lv_label_set_text(s_passkey_label, "");

    /* Register passkey display callback */
    ble_keyboard_set_passkey_callback(passkey_display_cb);

    /* ---- Save-prompt overlay (shown on the editor screen) ---- */
    int save_row_y  = overlay_row_y();
    int save_line_h = lv_font_get_line_height(FONT_11);
    int save_panel_h = save_row_y + save_line_h + 2 * OVERLAY_PAD;
    s_save_panel = lv_obj_create(s_scr);
    lv_obj_set_size(s_save_panel, SCR_W - 20, save_panel_h);
    lv_obj_set_pos(s_save_panel, 10, (SCR_H - save_panel_h) / 2);
    lv_obj_set_style_bg_color(s_save_panel, theme_bg(), 0);
    lv_obj_set_style_bg_opa(s_save_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(s_save_panel, theme_fg(), 0);
    lv_obj_set_style_border_width(s_save_panel, 2, 0);
    lv_obj_set_style_radius(s_save_panel, 4, 0);
    lv_obj_set_style_pad_all(s_save_panel, OVERLAY_PAD, 0);
    lv_obj_remove_flag(s_save_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_save_panel, LV_OBJ_FLAG_HIDDEN);

    s_save_hdr_lbl = lv_label_create(s_save_panel);
    lv_obj_set_style_text_font(s_save_hdr_lbl, FONT_11, 0);
    lv_obj_set_style_text_color(s_save_hdr_lbl, theme_fg(), 0);
    lv_label_set_text(s_save_hdr_lbl, "Save as (Enter/Esc):");
    lv_obj_set_pos(s_save_hdr_lbl, 0, 0);

    s_save_name_lbl = lv_label_create(s_save_panel);
    lv_obj_set_style_text_font(s_save_name_lbl, FONT_11, 0);
    lv_obj_set_style_text_color(s_save_name_lbl, theme_fg(), 0);
    lv_obj_set_width(s_save_name_lbl, SCR_W - 20 - 12);
    lv_label_set_text(s_save_name_lbl, "");
    lv_obj_set_pos(s_save_name_lbl, 0, save_row_y);

    /* Thin cursor bar inside the save prompt name field. The name field
     * is FONT_11, so the caret matches that line height (not the body
     * font's LINE_H, which may differ from FONT_11). */
    s_save_cur = lv_obj_create(s_save_panel);
    lv_obj_set_size(s_save_cur, 2, save_line_h);
    lv_obj_set_style_bg_color(s_save_cur, theme_fg(), 0);
    lv_obj_set_style_bg_opa(s_save_cur, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_save_cur, 0, 0);
    lv_obj_set_style_radius(s_save_cur, 0, 0);
    lv_obj_set_style_pad_all(s_save_cur, 0, 0);
    lv_obj_add_flag(s_save_cur, LV_OBJ_FLAG_HIDDEN);

    /* ---- Exit (Esc) prompt overlay (shown on the editor screen) ----
     * Header line plus three selectable option rows. */
    {
        int rows = EXIT_OPT_COUNT;
        int exit_row_y = overlay_row_y();
        int panel_h = exit_row_y + rows * (LINE_H + 2) + 2 * OVERLAY_PAD;
        s_exit_panel = lv_obj_create(s_scr);
        lv_obj_set_size(s_exit_panel, SCR_W - 20, panel_h);
        lv_obj_set_pos(s_exit_panel, 10, (SCR_H - panel_h) / 2);
        lv_obj_set_style_bg_color(s_exit_panel, theme_bg(), 0);
        lv_obj_set_style_bg_opa(s_exit_panel, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(s_exit_panel, theme_fg(), 0);
        lv_obj_set_style_border_width(s_exit_panel, 2, 0);
        lv_obj_set_style_radius(s_exit_panel, 4, 0);
        lv_obj_set_style_pad_all(s_exit_panel, 6, 0);
        lv_obj_remove_flag(s_exit_panel, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(s_exit_panel, LV_OBJ_FLAG_HIDDEN);

        s_exit_hdr_lbl = lv_label_create(s_exit_panel);
        lv_obj_set_style_text_font(s_exit_hdr_lbl, FONT_11, 0);
        lv_obj_set_style_text_color(s_exit_hdr_lbl, theme_fg(), 0);
        lv_label_set_text(s_exit_hdr_lbl,
                          "Unsaved changes (Up/Down + Enter):");
        lv_obj_set_pos(s_exit_hdr_lbl, 0, 0);

        for (int i = 0; i < EXIT_OPT_COUNT; i++) {
            s_exit_opt_lbl[i] = lv_label_create(s_exit_panel);
            lv_obj_set_style_text_font(s_exit_opt_lbl[i], FONT_11, 0);
            lv_obj_set_style_text_color(s_exit_opt_lbl[i], theme_fg(), 0);
            lv_obj_set_width(s_exit_opt_lbl[i], SCR_W - 20 - 12);
            lv_obj_set_style_pad_hor(s_exit_opt_lbl[i], 2, 0);
            lv_label_set_text(s_exit_opt_lbl[i], EXIT_OPT_LABELS[i]);
            lv_obj_set_pos(s_exit_opt_lbl[i], 0, exit_row_y + i * (LINE_H + 2));
#if defined(CONFIG_DRAFTLING_TOUCHSCREEN)
            lv_obj_add_flag(s_exit_opt_lbl[i], LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(s_exit_opt_lbl[i], exit_opt_touch_cb,
                                LV_EVENT_CLICKED, (void *)(intptr_t)i);
#endif
        }
    }

    /* ---- New-document format picker (moved onto the active screen
     * when shown) ---- */
    {
        int row_y = overlay_row_y();
        int panel_h = row_y + NEWFMT_COUNT * (LINE_H + 2) + 2 * OVERLAY_PAD;
        s_newfmt_panel = lv_obj_create(s_scr);
        lv_obj_set_size(s_newfmt_panel, SCR_W - 20, panel_h);
        lv_obj_set_pos(s_newfmt_panel, 10, (SCR_H - panel_h) / 2);
        lv_obj_set_style_bg_color(s_newfmt_panel, theme_bg(), 0);
        lv_obj_set_style_bg_opa(s_newfmt_panel, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(s_newfmt_panel, theme_fg(), 0);
        lv_obj_set_style_border_width(s_newfmt_panel, 2, 0);
        lv_obj_set_style_radius(s_newfmt_panel, 4, 0);
        lv_obj_set_style_pad_all(s_newfmt_panel, OVERLAY_PAD, 0);
        lv_obj_remove_flag(s_newfmt_panel, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(s_newfmt_panel, LV_OBJ_FLAG_HIDDEN);

        lv_obj_t *hdr = lv_label_create(s_newfmt_panel);
        lv_obj_set_style_text_font(hdr, FONT_11, 0);
        lv_obj_set_style_text_color(hdr, theme_fg(), 0);
        lv_label_set_text(hdr, "New file format (Enter/Esc):");
        lv_obj_set_pos(hdr, 0, 0);

        for (int i = 0; i < NEWFMT_COUNT; i++) {
            s_newfmt_opt_lbl[i] = lv_label_create(s_newfmt_panel);
            lv_obj_set_style_text_font(s_newfmt_opt_lbl[i], FONT_11, 0);
            lv_obj_set_style_text_color(s_newfmt_opt_lbl[i], theme_fg(), 0);
            lv_obj_set_width(s_newfmt_opt_lbl[i], SCR_W - 20 - 12);
            lv_obj_set_style_pad_hor(s_newfmt_opt_lbl[i], 2, 0);
            lv_label_set_text(s_newfmt_opt_lbl[i], NEWFMT_LABELS[i]);
            lv_obj_set_pos(s_newfmt_opt_lbl[i], 0, row_y + i * (LINE_H + 2));
        }
    }

    /* ---- Writing-timer interval picker (Ctrl+T; sized and moved onto
     * the active screen by show_ptimer_prompt()) ---- */
    {
        s_pt_open = false;
        s_pt_panel = lv_obj_create(s_scr);
        lv_obj_set_style_bg_color(s_pt_panel, theme_bg(), 0);
        lv_obj_set_style_bg_opa(s_pt_panel, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(s_pt_panel, theme_fg(), 0);
        lv_obj_set_style_border_width(s_pt_panel, 2, 0);
        lv_obj_set_style_radius(s_pt_panel, 4, 0);
        lv_obj_set_style_pad_all(s_pt_panel, OVERLAY_PAD, 0);
        lv_obj_remove_flag(s_pt_panel, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(s_pt_panel, LV_OBJ_FLAG_HIDDEN);

        s_pt_hdr_lbl = lv_label_create(s_pt_panel);
        lv_obj_set_style_text_font(s_pt_hdr_lbl, FONT_11, 0);
        lv_obj_set_style_text_color(s_pt_hdr_lbl, theme_fg(), 0);
        lv_obj_set_pos(s_pt_hdr_lbl, 0, 0);

        for (int i = 0; i < PTIMER_ROW_MAX; i++) {
            s_pt_opt_lbl[i] = lv_label_create(s_pt_panel);
            lv_obj_set_style_text_font(s_pt_opt_lbl[i], FONT_11, 0);
            lv_obj_set_style_text_color(s_pt_opt_lbl[i], theme_fg(), 0);
            lv_obj_set_width(s_pt_opt_lbl[i], SCR_W - 20 - 12 - 8);
            lv_obj_set_style_pad_hor(s_pt_opt_lbl[i], 2, 0);
            /* The word-goal and stop rows are labelled when shown. */
            if (i < PTIMER_OPT_COUNT)
                lv_label_set_text_fmt(s_pt_opt_lbl[i], "%u minutes",
                                      (unsigned)PTIMER_OPTIONS[i]);
            else
                lv_label_set_text(s_pt_opt_lbl[i], "");
            lv_obj_set_pos(s_pt_opt_lbl[i], 0,
                           overlay_row_y() + i * overlay_row_pitch());
        }
    }

    /* ---- Name-suggestion drop-down (editor screen; laid out by
     * suggest_place()) ---- */
    {
        s_sugg_open = false;
        s_sugg_panel = lv_obj_create(s_scr);
        lv_obj_set_style_bg_color(s_sugg_panel, theme_bg(), 0);
        lv_obj_set_style_bg_opa(s_sugg_panel, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(s_sugg_panel, theme_fg(), 0);
        lv_obj_set_style_border_width(s_sugg_panel, 1, 0);
        lv_obj_set_style_radius(s_sugg_panel, 0, 0);
        lv_obj_set_style_pad_all(s_sugg_panel, 2, 0);
        lv_obj_remove_flag(s_sugg_panel, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(s_sugg_panel, LV_OBJ_FLAG_HIDDEN);
        for (int i = 0; i < FMT_SUGGEST_MAX; i++) {
            s_sugg_lbl[i] = lv_label_create(s_sugg_panel);
            lv_label_set_long_mode(s_sugg_lbl[i], LV_LABEL_LONG_CLIP);
            lv_obj_set_style_bg_opa(s_sugg_lbl[i], LV_OPA_COVER, 0);
            lv_obj_set_style_bg_color(s_sugg_lbl[i], theme_bg(), 0);
            lv_obj_set_style_text_color(s_sugg_lbl[i], theme_fg(), 0);
            lv_label_set_text(s_sugg_lbl[i], "");
        }
    }

    /* ---- Message pop-up (moved onto the active screen when shown) ---- */
    {
        s_msg_panel = lv_obj_create(s_scr);
        lv_obj_set_width(s_msg_panel, SCR_W - 20);
        lv_obj_set_height(s_msg_panel, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_color(s_msg_panel, theme_bg(), 0);
        lv_obj_set_style_bg_opa(s_msg_panel, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(s_msg_panel, theme_fg(), 0);
        lv_obj_set_style_border_width(s_msg_panel, 2, 0);
        lv_obj_set_style_radius(s_msg_panel, 4, 0);
        lv_obj_set_style_pad_all(s_msg_panel, OVERLAY_PAD, 0);
        lv_obj_set_style_pad_row(s_msg_panel, OVERLAY_PAD, 0);
        lv_obj_set_flex_flow(s_msg_panel, LV_FLEX_FLOW_COLUMN);
        lv_obj_remove_flag(s_msg_panel, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(s_msg_panel, LV_OBJ_FLAG_HIDDEN);
#if defined(CONFIG_DRAFTLING_TOUCHSCREEN)
        lv_obj_add_event_cb(s_msg_panel, msg_popup_touch_cb, LV_EVENT_CLICKED, NULL);
#endif

        s_msg_title_lbl = lv_label_create(s_msg_panel);
        lv_obj_set_style_text_font(s_msg_title_lbl, FONT_14, 0);
        lv_obj_set_style_text_color(s_msg_title_lbl, theme_fg(), 0);
        lv_obj_set_width(s_msg_title_lbl, LV_PCT(100));
        lv_label_set_text(s_msg_title_lbl, "");

        s_msg_text_lbl = lv_label_create(s_msg_panel);
        lv_obj_set_style_text_font(s_msg_text_lbl, FONT_14, 0);
        lv_obj_set_style_text_color(s_msg_text_lbl, theme_fg(), 0);
        lv_obj_set_width(s_msg_text_lbl, LV_PCT(100));
        lv_label_set_long_mode(s_msg_text_lbl, LV_LABEL_LONG_WRAP);
        lv_label_set_text(s_msg_text_lbl, "");

        s_msg_hint_lbl = lv_label_create(s_msg_panel);
        lv_obj_set_style_text_font(s_msg_hint_lbl, FONT_11, 0);
        lv_obj_set_style_text_color(s_msg_hint_lbl, theme_fg(), 0);
        lv_obj_set_width(s_msg_hint_lbl, LV_PCT(100));
        lv_label_set_text(s_msg_hint_lbl, "Enter / Esc: close");
    }

    /* ---- Delete-file confirmation (moved onto the active screen when
     * shown) ---- */
    {
        static const char *const labels[DELCONF_COUNT] = { "Delete", "Cancel" };
        int row_y = overlay_row_y();
        int panel_h = row_y + DELCONF_COUNT * (LINE_H + 2) + 2 * OVERLAY_PAD;
        s_del_panel = lv_obj_create(s_scr);
        lv_obj_set_size(s_del_panel, SCR_W - 20, panel_h);
        lv_obj_set_pos(s_del_panel, 10, (SCR_H - panel_h) / 2);
        lv_obj_set_style_bg_color(s_del_panel, theme_bg(), 0);
        lv_obj_set_style_bg_opa(s_del_panel, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(s_del_panel, theme_fg(), 0);
        lv_obj_set_style_border_width(s_del_panel, 2, 0);
        lv_obj_set_style_radius(s_del_panel, 4, 0);
        lv_obj_set_style_pad_all(s_del_panel, OVERLAY_PAD, 0);
        lv_obj_remove_flag(s_del_panel, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(s_del_panel, LV_OBJ_FLAG_HIDDEN);

        s_del_hdr_lbl = lv_label_create(s_del_panel);
        lv_obj_set_style_text_font(s_del_hdr_lbl, FONT_11, 0);
        lv_obj_set_style_text_color(s_del_hdr_lbl, theme_fg(), 0);
        lv_obj_set_width(s_del_hdr_lbl, SCR_W - 20 - 12);
        lv_label_set_long_mode(s_del_hdr_lbl, LV_LABEL_LONG_CLIP);
        lv_label_set_text(s_del_hdr_lbl, "");
        lv_obj_set_pos(s_del_hdr_lbl, 0, 0);

        for (int i = 0; i < DELCONF_COUNT; i++) {
            s_del_opt_lbl[i] = lv_label_create(s_del_panel);
            lv_obj_set_style_text_font(s_del_opt_lbl[i], FONT_11, 0);
            lv_obj_set_style_text_color(s_del_opt_lbl[i], theme_fg(), 0);
            lv_obj_set_width(s_del_opt_lbl[i], SCR_W - 20 - 12);
            lv_obj_set_style_pad_hor(s_del_opt_lbl[i], 2, 0);
            lv_label_set_text(s_del_opt_lbl[i], labels[i]);
            lv_obj_set_pos(s_del_opt_lbl[i], 0, row_y + i * (LINE_H + 2));
        }
    }

    /* ---- Search / Replace overlay (shown on the editor screen) ---- */
    int srch_find_y = search_find_row_y();
    int srch_repl_y = search_repl_row_y();
    int srch_help_y = srch_repl_y + overlay_row_pitch();
    int srch_field_x = search_field_x();
    int srch_line_h  = lv_font_get_line_height(FONT_11);
    int srch_panel_h = srch_help_y + srch_line_h + 2 * OVERLAY_PAD;
    s_search_panel = lv_obj_create(s_scr);
    lv_obj_set_size(s_search_panel, SCR_W - 20, srch_panel_h);
    lv_obj_set_pos(s_search_panel, 10, (SCR_H - srch_panel_h) / 2);
    lv_obj_set_style_bg_color(s_search_panel, theme_bg(), 0);
    lv_obj_set_style_bg_opa(s_search_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(s_search_panel, theme_fg(), 0);
    lv_obj_set_style_border_width(s_search_panel, 2, 0);
    lv_obj_set_style_radius(s_search_panel, 4, 0);
    lv_obj_set_style_pad_all(s_search_panel, OVERLAY_PAD, 0);
    lv_obj_remove_flag(s_search_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_search_panel, LV_OBJ_FLAG_HIDDEN);

    s_search_hdr_lbl = lv_label_create(s_search_panel);
    lv_obj_set_style_text_font(s_search_hdr_lbl, FONT_11, 0);
    lv_obj_set_style_text_color(s_search_hdr_lbl, theme_fg(), 0);
    lv_label_set_text(s_search_hdr_lbl, "Find:");
    lv_obj_set_pos(s_search_hdr_lbl, 0, 0);

    /* Find row */
    s_search_find_hdr = lv_label_create(s_search_panel);
    lv_obj_set_style_text_font(s_search_find_hdr, FONT_11, 0);
    lv_obj_set_style_text_color(s_search_find_hdr, theme_fg(), 0);
    lv_label_set_text(s_search_find_hdr, "F:");
    lv_obj_set_pos(s_search_find_hdr, 0, srch_find_y);

    s_search_find_lbl = lv_label_create(s_search_panel);
    lv_obj_set_style_text_font(s_search_find_lbl, FONT_11, 0);
    lv_obj_set_style_text_color(s_search_find_lbl, theme_fg(), 0);
    lv_obj_set_width(s_search_find_lbl, SCR_W - 20 - 12 - srch_field_x);
    lv_label_set_text(s_search_find_lbl, "");
    lv_obj_set_pos(s_search_find_lbl, srch_field_x, srch_find_y);

    /* Replace row (only shown in replace mode) */
    s_search_repl_hdr = lv_label_create(s_search_panel);
    lv_obj_set_style_text_font(s_search_repl_hdr, FONT_11, 0);
    lv_obj_set_style_text_color(s_search_repl_hdr, theme_fg(), 0);
    lv_label_set_text(s_search_repl_hdr, "R:");
    lv_obj_set_pos(s_search_repl_hdr, 0, srch_repl_y);
    lv_obj_add_flag(s_search_repl_hdr, LV_OBJ_FLAG_HIDDEN);

    s_search_repl_lbl = lv_label_create(s_search_panel);
    lv_obj_set_style_text_font(s_search_repl_lbl, FONT_11, 0);
    lv_obj_set_style_text_color(s_search_repl_lbl, theme_fg(), 0);
    lv_obj_set_width(s_search_repl_lbl, SCR_W - 20 - 12 - srch_field_x);
    lv_label_set_text(s_search_repl_lbl, "");
    lv_obj_set_pos(s_search_repl_lbl, srch_field_x, srch_repl_y);
    lv_obj_add_flag(s_search_repl_lbl, LV_OBJ_FLAG_HIDDEN);

    /* Help line at the bottom of the panel */
    s_search_help_lbl = lv_label_create(s_search_panel);
    lv_obj_set_style_text_font(s_search_help_lbl, FONT_11, 0);
    lv_obj_set_style_text_color(s_search_help_lbl, theme_fg(), 0);
    lv_obj_set_width(s_search_help_lbl, SCR_W - 20 - 12);
    lv_label_set_text(s_search_help_lbl, "Enter:Next  Esc:Close");
    lv_obj_set_pos(s_search_help_lbl, 0, srch_help_y);

    /* Cursor bar shared between the find / replace fields. The find /
     * replace fields are FONT_11, so the caret matches that line
     * height. The refresh function repositions it to whichever row
     * owns focus. */
    s_search_cur = lv_obj_create(s_search_panel);
    lv_obj_set_size(s_search_cur, 2, srch_line_h);
    lv_obj_set_style_bg_color(s_search_cur, theme_fg(), 0);
    lv_obj_set_style_bg_opa(s_search_cur, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_search_cur, 0, 0);
    lv_obj_set_style_radius(s_search_cur, 0, 0);
    lv_obj_set_style_pad_all(s_search_cur, 0, 0);
    lv_obj_add_flag(s_search_cur, LV_OBJ_FLAG_HIDDEN);

    /* ---- Key repeat timer ---- */
    s_repeat_timer = lv_timer_create(key_repeat_cb, KEY_REPEAT_RATE_MS, NULL);

    /* Register WiFi and Git sync status callbacks */
    wifi_manager_set_callback(wifi_state_cb);
    git_sync_set_callback(git_sync_cb);

    /* Every battery label exists now: show the current level on all of
     * them, including the BLE prompt screen loaded below. */
    battery_labels_repaint();

    /* Writing-timer line on the top layer, in the current theme. */
    ptimer_build_bar();

    /* Start on BLE prompt screen (transitions to file browser on connect) */
    lv_scr_load(s_scr_ble_prompt);
}

/* Tear down every screen, overlay and screen-bound timer that was
 * created by build_screens() so the next build_screens() call can
 * recreate them in a fresh theme. Runs on the LVGL task with the
 * port lock held (caller is a key-event handler). Only used by the
 * color-theme picker, which is gated on CONFIG_DRAFTLING_DISPLAY_COLOR. */
#if defined(CONFIG_DRAFTLING_DISPLAY_COLOR)
static void teardown_screens(void)
{
    /* Stop and forget the screen-bound timers first. */
#if !defined(CONFIG_DRAFTLING_DISPLAY_EPD)
    if (s_blink_timer)     { lv_timer_delete(s_blink_timer);     s_blink_timer     = NULL; }
#endif
#if defined(DRAFTLING_HAS_BATT_INDICATOR) && !defined(DRAFTLING_BATT_PULL_MODE)
    if (s_batt_timer)      { lv_timer_delete(s_batt_timer);      s_batt_timer      = NULL; }
#endif
    if (s_repeat_timer)    { lv_timer_delete(s_repeat_timer);    s_repeat_timer    = NULL; }
    if (s_status_clear_timer) {
        lv_timer_delete(s_status_clear_timer);
        s_status_clear_timer = NULL;
    }

    /* Delete every top-level screen we created. Children (status
     * bars, line labels, the passkey / save / search overlays
     * parented to s_scr, the wifi icons, etc.) are deleted
     * automatically with their parent. */
    if (s_scr_browser)    { lv_obj_delete(s_scr_browser);    s_scr_browser    = NULL; }
    if (s_scr_menu)       { lv_obj_delete(s_scr_menu);       s_scr_menu       = NULL; }
    if (s_scr_help)       { lv_obj_delete(s_scr_help);       s_scr_help       = NULL; }
    if (s_scr_settings)   { lv_obj_delete(s_scr_settings);   s_scr_settings   = NULL; }
    if (s_scr_ble_prompt) { lv_obj_delete(s_scr_ble_prompt); s_scr_ble_prompt = NULL; }
    if (s_scr)            { lv_obj_delete(s_scr);            s_scr            = NULL; }

    /* NULL every child-widget pointer so any stale reference
     * crashes deterministically instead of touching freed memory.
     * The per-pane widgets (container, cursor, logo, line label
     * pool) are cleared for every pane. */
    s_lbl_title = s_lbl_status = NULL;
    s_editor_hline = s_editor_sline = NULL;
    /* The rebuilt bars start out shown. */
    s_bars_hidden = false;
    if (s_bars_timer) lv_timer_pause(s_bars_timer);
    bars_flash_stop();
    s_img_wifi = s_img_br_wifi = NULL;
    s_pane_divider = NULL;
    {
        pane_t *save_rp = s_rp;
        for (int p = 0; p < EDITOR_MAX_PANES; p++) {
            s_rp = &s_panes[p];
            s_cont_edit = s_cursor = s_img_logo = NULL;
            for (int i = 0; i < MAX_LINE_LABELS; i++) {
                s_line_labels[i] = NULL;
            }
        }
        s_rp = save_rp;
    }
    s_list_files = s_lbl_br_status = NULL;
    s_inpane_list = s_inpane_hdr = NULL;
    s_browser_list = NULL;
    s_inpane_browser_open = false;
#if defined(DRAFTLING_HAS_BATT_INDICATOR)
    s_lbl_dev_batt = s_lbl_br_dev_batt = NULL;
    s_lbl_ble_dev_batt = NULL;
#endif
    s_menu_list = s_lbl_menu_hdr = NULL;
    s_lbl_help_hdr = s_help_cont = s_help_lbl = NULL;
    s_settings_list = s_lbl_settings_hdr = NULL;
    s_ble_prompt_lbl = NULL;
    s_passkey_panel = s_passkey_label = NULL;
    s_save_panel = s_save_hdr_lbl = s_save_name_lbl = s_save_cur = NULL;
    s_wifi_pw_panel = s_wifi_pw_hdr_lbl = s_wifi_pw_name_lbl = s_wifi_pw_cur = NULL;
    s_dbx_panel = s_dbx_hdr_lbl = s_dbx_val_lbl = s_dbx_cur = NULL;
    s_msg_panel = s_msg_title_lbl = s_msg_text_lbl = s_msg_hint_lbl = NULL;
    s_books_open = false;
    s_toc_open = false;
    books_free();
    s_exit_panel = s_exit_hdr_lbl = NULL;
    s_exit_opt_lbl[0] = s_exit_opt_lbl[1] = s_exit_opt_lbl[2] = NULL;
    s_newfmt_panel = NULL;
    s_newfmt_opt_lbl[0] = s_newfmt_opt_lbl[1] = s_newfmt_opt_lbl[2] = NULL;
    s_pt_panel = s_pt_hdr_lbl = NULL;
    for (int i = 0; i < PTIMER_ROW_MAX; i++) s_pt_opt_lbl[i] = NULL;
    s_br_sline = NULL;
    s_del_panel = s_del_hdr_lbl = NULL;
    s_del_opt_lbl[0] = s_del_opt_lbl[1] = NULL;
    s_search_panel = s_search_hdr_lbl = NULL;
    s_search_find_hdr = s_search_find_lbl = NULL;
    s_search_repl_hdr = s_search_repl_lbl = NULL;
    s_search_cur = s_search_help_lbl = NULL;

    /* Reset every "open" / transient flag so a fresh build starts
     * from a clean state. The persistent document, NVS-backed font
     * size / theme / backlight / standby timeout are NOT touched. */
    s_menu_open               = false;
    s_help_open               = false;
    s_settings_open           = false;
#if defined(CONFIG_DRAFTLING_DISPLAY_COLOR)
    s_theme_picker_open       = false;
#endif
    s_layouts_picker_open     = false;
    s_factory_reset_confirm   = false;
    s_restart_needed         = false;
    s_restart_confirm_open     = false;
    s_save_open               = false;
    s_wifi_scan_picker_open   = false;
    s_wifi_scan_in_progress   = false;
    s_wifi_pw_open            = false;
    s_dbx_open                = false;
    s_msg_open                = false;
    s_msg_busy                = false;
#if defined(CONFIG_DRAFTLING_HAS_KERMIT)
    s_msg_kermit              = false;
#endif
    s_exit_open               = false;
    s_newfmt_open             = false;
    s_pt_open                 = false;
    s_del_open                = false;
    s_search_open             = false;
    s_search_replace_mode     = false;

    /* The editor render cache references the deleted line labels;
     * wipe every pane's cache so the next refresh re-creates fresh
     * slots. */
    invalidate_all_render_caches();
}
#endif /* CONFIG_DRAFTLING_DISPLAY_COLOR */

/* Apply a freshly-selected color theme without rebooting. Recreates
 * every LVGL screen and widget under the new palette and restores
 * the screen the user was on. */
#if defined(CONFIG_DRAFTLING_DISPLAY_COLOR)
static void rebuild_screens_for_theme(void)
{
    /* Remember which screen the user is on so we can restore it. */
    lv_obj_t *prev_active = lv_scr_act();
    bool was_browser     = (prev_active == s_scr_browser);
    bool was_menu        = (prev_active == s_scr_menu);
    bool was_settings    = (prev_active == s_scr_settings);
    bool was_ble_prompt  = (prev_active == s_scr_ble_prompt);
    bool was_editor      = (prev_active == s_scr);
    settings_level_t set_level = s_settings_level;
    int set_depth = s_settings_depth;
    int set_stack[SETTINGS_MAX_DEPTH];
    memcpy(set_stack, s_settings_sel_stack, sizeof(set_stack));
    int set_sel = s_settings_sel;
    bool restart_needed = s_restart_needed;
    /* If lv_scr_act returned something we did not recognise (should
     * not happen) fall back to the editor screen. */
    if (!was_browser && !was_menu && !was_settings &&
        !was_ble_prompt && !was_editor) {
        was_editor = true;
    }

    teardown_screens();
    init_styles();
    build_screens();

    /* Restore the previously-active screen and re-paint its content. */
    if (was_browser) {
        refresh_file_list();
        lv_scr_load(s_scr_browser);
    } else if (was_menu) {
        show_menu();        /* refreshes items + loads s_scr_menu */
    } else if (was_settings) {
        /* The theme is picked in Settings -> Display: come back to
         * that level, and keep a pending restart-only change so leaving
         * the level still offers the restart. */
        show_settings();    /* refreshes items + loads s_scr_settings */
        s_settings_level = set_level;
        s_settings_depth = set_depth;
        memcpy(s_settings_sel_stack, set_stack, sizeof(set_stack));
        s_settings_sel = set_sel;
        s_restart_needed = restart_needed;
        refresh_settings_items();
    } else if (was_ble_prompt) {
        lv_scr_load(s_scr_ble_prompt);
    } else {
        lv_scr_load(s_scr);
        editor_ui_refresh();
    }
    update_title_bar();
    update_wifi_icons();
}
#endif

#if defined(CONFIG_DRAFTLING_DISPLAY_RESIZABLE)
/* The window was resized: the LVGL port may change the canvas size
 * only while nothing transient is on screen, since the relayout
 * rebuilds every screen like a theme change (rebuild_screens_for_theme(),
 * which closes overlays, pickers and pop-ups). Until then the port
 * keeps the old size and asks again. */
extern "C" bool editor_ui_can_relayout(void)
{
    if (!s_scr) return false;   /* before editor_ui_init() */
    if (s_restart_confirm_open || s_fmt_step != FMT_STEP_NONE ||
        s_books_open || s_toc_open || s_factory_reset_confirm || s_theme_picker_open ||
        s_layouts_picker_open || s_save_open || s_xfer_open ||
        s_setup_busy || s_wifi_scan_picker_open ||
        s_wifi_scan_in_progress || s_wifi_pw_open || s_dbx_open ||
        s_msg_open || s_search_open || s_exit_open || s_newfmt_open ||
        s_del_open || s_help_open || s_pt_open || s_sugg_open ||
        s_inpane_browser_open || s_drag_active) {
        return false;
    }
#if defined(CONFIG_DRAFTLING_HAS_USB_MSC)
    if (s_usbmsc_picker_open) return false;
#endif
#if defined(CONFIG_DRAFTLING_HAS_KERMIT)
    if (s_kermit_picker_open) return false;
#endif
    if (s_passkey_panel && !lv_obj_has_flag(s_passkey_panel, LV_OBJ_FLAG_HIDDEN))
        return false;
    return true;
}

/* Files changed behind the editor's back (the web client's Files
 * panel): see editor_ui.h. */
extern "C" void editor_ui_files_changed(const char *path)
{
    if (path) editor_reload_path(path);
    if (s_editor_screen_active) editor_ui_refresh();
    if (lv_scr_act() == s_scr_browser) refresh_file_list();
}

/* Lay every screen out again for the LVGL display's new size (SCR_W /
 * SCR_H read it). Call with the LVGL lock held, after
 * lv_display_set_resolution(), and only when editor_ui_can_relayout()
 * said yes. The documents, the browsed project and the settings stay;
 * the F1 menu comes back at its top level. */
extern "C" void editor_ui_relayout(void)
{
    if (!s_scr) return;
    rebuild_screens_for_theme();
#ifdef DRAFTLING_HAS_IME
    ime_set_status_cells(status_avail_width() / char_width_for_font(FONT_11));
#endif
}
#endif

extern "C" void editor_ui_init(void)
{
    /* Chain optional font subsets (Cyrillic / Hebrew / Vietnamese /
     * CJK) into the base font's fallback slot before any text is
     * rendered. High-density boards use the Hack family; all others
     * use Greybeard. */
#ifdef CONFIG_DRAFTLING_DISPLAY_HIDPI
    hires_init();
#else
    lowres_init();
#endif
#ifdef DRAFTLING_HAS_IME
    ime_init(&s_ime_host, ime_builtin_ja_dict(), ime_builtin_zh_dict());
    ime_set_status_cells(status_avail_width() / char_width_for_font(FONT_11));
#endif

    load_font_size_from_nvs();
#if defined(CONFIG_DRAFTLING_DISPLAY_COLOR)
    load_theme_from_nvs();
#endif
#if defined(CONFIG_DRAFTLING_DISPLAY_HAS_BACKLIGHT) && !defined(CONFIG_DRAFTLING_DISPLAY_BACKLIGHT_BINARY)
    load_backlight_from_nvs();
    display_set_backlight(s_backlight_pct);
#endif
    load_append_only_from_nvs();
#if defined(CONFIG_DRAFTLING_DISPLAY_ROUND_CORNERS)
    load_corners_from_nvs();
    corners_compute();
    editor_host_corners_changed(s_corner_size);
#endif
#if defined(CONFIG_DRAFTLING_DISPLAY_HOST_ROTATE)
    load_host_rotation_from_nvs();
    editor_host_rotation_changed(s_host_orient, s_host_rot180);
#endif
    load_ptimer_from_nvs();
    load_name_suggest_from_nvs();
    load_hide_bars_from_nvs();
    load_active_layouts_from_nvs();
    load_current_layout_from_nvs();
    load_margins_from_nvs();
    s_pending_portrait = display_orientation_get_pending_portrait();
    s_pending_upside_down = display_flip_get_pending_upside_down();
    s_pending_scale2x = display_scale_get_pending_2x();
    init_styles();

    /* Create key-event queue (must exist before BLE callback is set) */
    s_key_queue = xQueueCreate(KEY_QUEUE_LEN, sizeof(kb_event_t));
    assert(s_key_queue);

    /* Editor init */
    editor_init();

    /* Bind the engine's primary document to pane 0 so the pane pool
     * owns it from the start (split mode acquires a second document
     * into pane 1 on demand). */
    s_panes[0].doc = editor_get_active();

    /* Key-event drain timer -- runs every 20 ms (50 Hz) to process
     * queued keyboard events in a batch. This decouples BLE input
     * from the LVGL render cycle so fast typing never drops keys.
     * Created here (not in build_screens) so a theme rebuild does
     * not destroy the timer that owns the rebuild's own call stack. */
    s_key_drain_timer = lv_timer_create(key_drain_cb, KEY_DRAIN_PERIOD_MS, NULL);

    build_screens();

#if defined(CONFIG_DRAFTLING_DISPLAY_CAN_ROTATE)
    /* Restore the persisted 180-degree display flip and apply it on
     * top of the build-time base rotation now that the screens exist
     * (the flip repaints the active screen in the new orientation). */
    load_rotate180_from_nvs();
    if (s_rotate180) {
        draftling_lvgl_port_set_flip180(true);
    }
#endif

#if defined(CONFIG_DRAFTLING_DISPLAY_CAN_INVERT)
    /* Restore the persisted inverted-screen option and apply it now
     * that the panel has been initialized (display_set_invert() sends
     * a flush of the current framebuffer under the restored polarity). */
    load_invert_from_nvs();
    if (s_invert) {
        display_set_invert(true);
    }
#endif

    /* Restore the split layout persisted across reboot / deep sleep.
     * The documents themselves are not auto-reopened (the boot flow
     * shows the file browser), but the saved split mode is materialized
     * so the editor screen comes up in the same layout the user left. */
    load_split_from_nvs();
    if (s_split_mode != SPLIT_NONE) {
        split_mode_t m = s_split_mode;
        s_split_mode = SPLIT_NONE;     /* force apply to run the enable path */
        editor_ui_apply_split_mode(m);
    }

    ESP_LOGI(TAG, "Editor UI initialized");
}
