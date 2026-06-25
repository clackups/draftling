#include <cstdio>
#include <cstring>
#include <string>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <nvs.h>
#include "sdkconfig.h"
#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "editor_ui.h"
#include "editor.h"
#include "md_parser.h"
#include "ble_keyboard.h"
#if defined(CONFIG_DRAFTLING_HAS_USB_HOST)
#include "usb_kbd.h"
#endif
#if defined(CONFIG_DRAFTLING_HAS_TAB5_KBD)
#include "tab5_kbd.h"
#endif
#include "kb_layout.h"
#include "wifi_manager.h"
#include "git_sync.h"
#include "sd_card.h"
#include "lvgl_port.h"
#include "display.h"
#include "standby.h"
#include "greybeard.h"
#include "battery.h"
#include "wifi_icon.h"
#include "freertos/task.h"

/*
 * Font aliases.
 *
 * Greybeard is a monospaced bitmap font that includes Latin,
 * Latin-1 Supplement, and Cyrillic glyphs in every size, so a
 * single set of fonts covers all enabled keyboard layouts.
 *
 * The body font is selected at runtime via the base font size
 * user setting (11, 14, or 16 px).  Heading fonts are scaled
 * relative to the body size, using the 26 px font for the
 * largest headings.  Status bars always use FONT_11 regardless
 * of the body font setting.
 */
#define FONT_11 (&greybeard_11)
#define FONT_14 (&greybeard_14)
#define FONT_16 (&greybeard_16)
#define FONT_18 (&greybeard_18)
#define FONT_22 (&greybeard_22)
#define FONT_26 (&greybeard_26)

static const char *TAG = "EditorUI";

/* Layout constants -- account for display rotation and the logical
 * pixel scale factor (CONFIG_DRAFTLING_DISPLAY_SCALE). The editor
 * lives entirely in logical pixels; the display backend scales each
 * logical pixel to SCALE x SCALE physical panel pixels.
 *
 * At 90 or 270 degrees, the logical width and height are swapped. */
#if CONFIG_DRAFTLING_DISPLAY_ROTATE_ANGLE == 90 || CONFIG_DRAFTLING_DISPLAY_ROTATE_ANGLE == 270
#define SCR_W        (CONFIG_DRAFTLING_DISPLAY_HEIGHT / CONFIG_DRAFTLING_DISPLAY_SCALE)
#define SCR_H        (CONFIG_DRAFTLING_DISPLAY_WIDTH  / CONFIG_DRAFTLING_DISPLAY_SCALE)
#else
#define SCR_W        (CONFIG_DRAFTLING_DISPLAY_WIDTH  / CONFIG_DRAFTLING_DISPLAY_SCALE)
#define SCR_H        (CONFIG_DRAFTLING_DISPLAY_HEIGHT / CONFIG_DRAFTLING_DISPLAY_SCALE)
#endif
#define HEADER_H     16
#define STATUS_H     16
#define EDITOR_Y     HEADER_H
#define EDITOR_H     (SCR_H - HEADER_H - STATUS_H)
#define LIST_PANEL_H (SCR_H - 18)  /* height for list panels below header */

/* ---- Base font size setting ----
 * The user can pick 11, 14, or 16 px as the editor body font.
 * Heading fonts are scaled up from the body size. */
#define FONT_SIZE_COUNT 3
static const int FONT_SIZE_OPTIONS[FONT_SIZE_COUNT] = { 11, 14, 16 };
static const char *FONT_SIZE_LABELS[FONT_SIZE_COUNT] = { "11 px", "14 px", "16 px" };

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

/* Forward declaration (defined below) */
static int char_width_for_font(const lv_font_t *font);

#if defined(CONFIG_DRAFTLING_DISPLAY_COLOR)
/* Forward declaration: tear down every screen / overlay and rebuild
 * them under the freshly-selected color theme. Defined alongside
 * the other init helpers near the bottom of this file. */
static void rebuild_screens_for_theme(void);
#endif

/* Return the body font for the current size setting. */
static const lv_font_t *body_font(void)
{
    if (s_font_size == 16) return FONT_16;
    if (s_font_size == 14) return FONT_14;
    return FONT_11;
}

/* Return heading fonts (h1/h2/h3) scaled relative to the body size.
 *   body 11 -> h3 14, h2 16, h1 18
 *   body 14 -> h3 16, h2 18, h1 22
 *   body 16 -> h3 18, h2 22, h1 26
 */
static const lv_font_t *h1_font(void)
{
    if (s_font_size == 16) return FONT_26;
    if (s_font_size == 14) return FONT_22;
    return FONT_18;
}

static const lv_font_t *h2_font(void)
{
    if (s_font_size == 16) return FONT_22;
    if (s_font_size == 14) return FONT_18;
    return FONT_16;
}

static const lv_font_t *h3_font(void)
{
    if (s_font_size == 16) return FONT_18;
    if (s_font_size == 14) return FONT_16;
    return FONT_14;
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
 * is persisted in NVS. All themes use a black background; the
 * foreground choices are light green (default), dark green,
 * amber/orange, and white -- a familiar "monochrome terminal"
 * palette appropriate for a distraction-free Markdown editor.
 */
#if defined(CONFIG_DRAFTLING_DISPLAY_COLOR)

typedef struct {
    const char *name;       /* shown in Settings menu */
    uint32_t    fg_rgb;     /* 0xRRGGBB */
    uint32_t    bg_rgb;
} color_theme_t;

#define COLOR_THEME_COUNT 4
static const color_theme_t COLOR_THEMES[COLOR_THEME_COUNT] = {
    /* "Light green on black" is the default for color LCDs: pure
     * 100 % green at 0x00FF00 evokes a classic CRT terminal and
     * gives the highest legibility on small panels. The remaining
     * foreground hexes match the eye-friendly palette used by the
     * companion clackups/smart-keyboard project
     * (theme_darkgreen_on_black and a slightly-darkened amber) so
     * the two devices look consistent when used side by side. */
    { "Light green on black", 0x00FF00, 0x000000 },
    { "Dark green on black",  0x15631A, 0x000000 },
    { "Orange on black",      0xCC6600, 0x000000 },
    { "White on black",       0xFFFFFF, 0x000000 },
};

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
 * Boards whose display backend exposes a controllable backlight
 * (CONFIG_DRAFTLING_DISPLAY_HAS_BACKLIGHT) let the user pick a
 * brightness percentage from F1 -> Settings. The selection is
 * persisted in NVS and applied at boot (and immediately on change)
 * via display_set_backlight().
 *
 * The available steps are coarse on purpose: a single Enter on the
 * Settings list cycles through them, so finer granularity would be
 * tedious to dial in. */
#if defined(CONFIG_DRAFTLING_DISPLAY_HAS_BACKLIGHT)
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
/* Default to 50 % when available, else the lowest still-allowed
 * step (set per board via CONFIG_DRAFTLING_BACKLIGHT_MIN_PCT). */
#if CONFIG_DRAFTLING_BACKLIGHT_MIN_PCT <= 50
static int s_backlight_pct = 50;
#else
static int s_backlight_pct = CONFIG_DRAFTLING_BACKLIGHT_MIN_PCT;
#endif

static void load_backlight_from_nvs(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS_EDITOR, NVS_READONLY, &h) == ESP_OK) {
        uint8_t v = 0;
        if (nvs_get_u8(h, NVS_KEY_BACKLIGHT, &v) == ESP_OK && v <= 100) {
            s_backlight_pct = v;
        }
        nvs_close(h);
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
    /* Unknown value: prefer 50 % if it is still in the cycle,
     * otherwise the first option. */
    for (int i = 0; i < BACKLIGHT_OPTION_COUNT; i++) {
        if (BACKLIGHT_OPTIONS[i] == 50) return i;
    }
    return 0;
}
#endif  /* CONFIG_DRAFTLING_DISPLAY_HAS_BACKLIGHT */

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

/* Recalculate derived layout values from the current body font. */
static void recalc_layout(void)
{
    const lv_font_t *bf = body_font();
    s_line_h = lv_font_get_line_height(bf);
    s_char_w = char_width_for_font(bf);
}

/* Return the index into FONT_SIZE_OPTIONS for the given pixel size.
 * Falls back to 0 (11 px) when the size is not recognized. */
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
            if (val == 11 || val == 14 || val == 16)
                s_font_size = val;
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

/* Number of menu items */
#define MENU_ITEM_COUNT 8

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
static sd_card_file_entry_t s_browser_entries[64];

/* Settings screen objects */
static lv_obj_t *s_scr_settings  = NULL;
static lv_obj_t *s_settings_list = NULL;
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

/* ---- Exit (Esc) prompt overlay ----
 * Shown when Esc is pressed in the editor with unsaved changes. Offers
 * three choices: save the file, exit without saving, or cancel and
 * keep editing. Navigated with Up/Down + Enter; Esc cancels. */
static lv_obj_t *s_exit_panel    = NULL;
static lv_obj_t *s_exit_hdr_lbl  = NULL;
static lv_obj_t *s_exit_opt_lbl[3] = { NULL, NULL, NULL };
static bool      s_exit_open     = false;
static int       s_exit_sel      = 0;      /* 0=Save 1=Exit-no-save 2=Cancel */

#define EXIT_OPT_SAVE   0
#define EXIT_OPT_DISCARD 1
#define EXIT_OPT_CANCEL 2
#define EXIT_OPT_COUNT  3

static const char *const EXIT_OPT_LABELS[EXIT_OPT_COUNT] = {
    "Save and exit",
    "Exit without saving",
    "Cancel (keep editing)",
};

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

/* Line label pool */
#define MAX_LINE_LABELS 24

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
 * Each pane keeps its own MAX_LINE_LABELS-slot label / selection pools
 * and per-slot render cache. A split layout halves each pane's width
 * but keeps the full editor height, and each pane wraps text to its
 * own (narrower) width.
 *
 * The per-slot render cache lets editor_ui_refresh() skip LVGL
 * mutations whenever a slot's visible text / style / position are
 * unchanged, which keeps the e-paper dirty region small enough for a
 * fast partial refresh instead of a full-screen flash. */
#define EDITOR_MAX_PANES 2

typedef struct {
    lv_obj_t   *cont;      /* per-pane content container */
    lv_obj_t   *cursor;    /* thin vertical cursor bar */
    lv_obj_t   *logo;      /* "draftling" placeholder when the doc is empty */
    lv_obj_t   *line_labels[MAX_LINE_LABELS];
    lv_obj_t   *sel_rects[MAX_LINE_LABELS];
    std::string prev_line_text[MAX_LINE_LABELS];
    int  prev_line_type[MAX_LINE_LABELS];    /* md_line_type_t, -1 if cache empty */
    int  prev_line_y[MAX_LINE_LABELS];       /* y_pos last used, -1 if cache empty */
    int  prev_line_h[MAX_LINE_LABELS];       /* rendered_h last computed */
    bool prev_line_visible[MAX_LINE_LABELS]; /* slot was visible (not hidden) */
    bool prev_line_was_selected[MAX_LINE_LABELS]; /* line intersected the selection */
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
} pane_t;

static pane_t  s_panes[EDITOR_MAX_PANES];
static pane_t *s_rp          = &s_panes[0];  /* current render / active pane */
static int     s_pane_count  = 1;            /* 1 = single, 2 = split */
static int     s_focus       = 0;            /* focused pane index (0..count-1) */

/* Split layout: single pane, or a vertical (left / right) split with
 * the left pane occupying 1/2, 2/3 or 1/3 of the usable width. Driven
 * by the Ctrl+1 / Ctrl+2 / Ctrl+3 shortcuts. */
typedef enum {
    SPLIT_NONE = 0,   /* single pane (full width) */
    SPLIT_HALF,       /* two panes, left = 1/2 */
    SPLIT_LEFT_2_3,   /* two panes, left = 2/3 */
    SPLIT_LEFT_1_3,   /* two panes, left = 1/3 */
} split_mode_t;
static split_mode_t s_split_mode = SPLIT_NONE;

/* True while the editor screen is the active top-level screen, false
 * while the file browser is. This is the authoritative source for
 * key-event routing and refresh gating. It must NOT be derived from
 * editor_get_mode(): the editor "mode" is a per-document property
 * (editor.cpp aliases it onto the active document), and the active
 * document is re-bound to the focused pane on every cursor blink and
 * refresh. Reading the per-document mode to decide which screen we are
 * on therefore breaks in split mode -- the focused pane can carry an
 * EDITING mode while the browser is on screen, leaving keystrokes
 * routed to the (hidden) editor and the browser apparently frozen. */
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

/* Alias the historical per-document globals onto the current pane. */
#define s_cont_edit              (s_rp->cont)
#define s_cursor                 (s_rp->cursor)
#define s_img_logo               (s_rp->logo)
#define s_line_labels            (s_rp->line_labels)
#define s_sel_rects              (s_rp->sel_rects)
#define s_prev_line_text         (s_rp->prev_line_text)
#define s_prev_line_type         (s_rp->prev_line_type)
#define s_prev_line_y            (s_rp->prev_line_y)
#define s_prev_line_h            (s_rp->prev_line_h)
#define s_prev_line_visible      (s_rp->prev_line_visible)
#define s_prev_line_was_selected (s_rp->prev_line_was_selected)
#define s_cursor_on_screen       (s_rp->cursor_on_screen)

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

/* Recompute each pane's content-area rectangle from the current split
 * state. The editor body occupies [EDITOR_Y, EDITOR_Y + EDITOR_H). A
 * single pane fills it entirely; a vertical split places two panes
 * side by side (left / right) with a 1 px vertical divider between
 * them. Both panes keep the full editor height; only their width and
 * x-offset differ, so each pane wraps text to its own narrower width.
 * The left pane's share of the usable width is set by s_split_mode. */
static void recalc_pane_geometry(void)
{
    if (s_pane_count <= 1) {
        s_panes[0].x = 0;
        s_panes[0].w = SCR_W;
        s_panes[0].y = EDITOR_Y;
        s_panes[0].h = EDITOR_H;
        s_panes[1].x = 0;
        s_panes[1].w = 0;
        s_panes[1].y = EDITOR_Y;
        s_panes[1].h = EDITOR_H;
        return;
    }
    int total = SCR_W - 1;             /* 1 px reserved for the divider */
    int left_w;
    switch (s_split_mode) {
    case SPLIT_LEFT_2_3: left_w = (total * 2) / 3; break;
    case SPLIT_LEFT_1_3: left_w = total / 3;       break;
    case SPLIT_HALF:
    default:             left_w = total / 2;       break;
    }
    if (left_w < 1) left_w = 1;
    if (left_w > total - 1) left_w = total - 1;
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
            lv_obj_set_pos(s_pane_divider, s_panes[0].x + s_panes[0].w, EDITOR_Y);
            lv_obj_set_size(s_pane_divider, 1, EDITOR_H);
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
#else
static void batt_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    char batt[20];
    format_batt_str(batt, sizeof(batt));
    if (s_lbl_dev_batt)    lv_label_set_text(s_lbl_dev_batt, batt);
    if (s_lbl_br_dev_batt) lv_label_set_text(s_lbl_br_dev_batt, batt);
    if (s_lbl_ble_dev_batt) lv_label_set_text(s_lbl_ble_dev_batt, batt);
}
/* No-op on the timer-driven boards: the call sites below are shared,
 * but only the H752 pull model needs an explicit sync. */
static inline void sync_battery_labels(void) {}
#endif
#else  /* !CONFIG_DRAFTLING_HAS_BATTERY */
/* No battery indicator on this board: the shared sync_battery_labels()
 * call sites compile to nothing. */
static inline void sync_battery_labels(void) {}
#endif

/* Update the WiFi connectivity icons in both status bars: shown when
 * the WiFi stack is connected, hidden otherwise. */
static void update_wifi_icons(void)
{
    bool connected = wifi_manager_is_connected();
    if (s_img_wifi) {
        if (connected) lv_obj_remove_flag(s_img_wifi, LV_OBJ_FLAG_HIDDEN);
        else           lv_obj_add_flag(s_img_wifi, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_img_br_wifi) {
        if (connected) lv_obj_remove_flag(s_img_br_wifi, LV_OBJ_FLAG_HIDDEN);
        else           lv_obj_add_flag(s_img_br_wifi, LV_OBJ_FLAG_HIDDEN);
    }
}

static void update_title_bar(void)
{
    const char *path = editor_get_file_path();
    const char *name = "Untitled";
    if (path) {
        const char *slash = strrchr(path, '/');
        name = slash ? slash + 1 : path;
    }
    char buf[128];
    int line, col;
    editor_get_cursor_pos(&line, &col);
    int total_lines = editor_get_line_count();
#if defined(CONFIG_DRAFTLING_DISPLAY_EPD)
    /* On e-paper boards the cursor moves on every keystroke, so the
     * column counter is omitted to avoid dirtying the title bar on
     * every edit. The line counter ("L %d/%d") only changes when the
     * cursor moves to a different line, so the s_prev_title cache
     * below collapses no-op redraws back to a single update. */
    snprintf(buf, sizeof(buf), "%s%s  L %d/%d  [%s]",
             name, editor_is_modified() ? " *" : "",
             line + 1, total_lines,
             kb_layout_name(kb_layout_get()));
#else
    snprintf(buf, sizeof(buf), "%s%s  L %d/%d C:%d  [%s]",
             name, editor_is_modified() ? " *" : "",
             line + 1, total_lines, col + 1,
             kb_layout_name(kb_layout_get()));
#endif

    /* lv_label_set_text() in LVGL v9 unconditionally invalidates the
     * label even when the new text is identical to the current one,
     * which on e-paper backends dirties the entire title-bar strip
     * on every keystroke.  Combined with the dirty region from the
     * edited line that spans the top and bottom of the screen and
     * triggers a full-screen refresh in the e-paper driver's ">75%"
     * huge-area path.  Compare against the last text we pushed and
     * skip the call when unchanged. */
    static char s_prev_title[128] = { 0 };
    if (strcmp(s_prev_title, buf) != 0) {
        lv_label_set_text(s_lbl_title, buf);
        /* snprintf truncates to at most sizeof(buf)-1 chars, which
         * also fits in s_prev_title[128]. */
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

/* Return the monospace cell width (in pixels) for a given Greybeard
 * font size.  The values are the advance widths stored in the generated
 * font data, divided by 16 (LVGL stores advances in 1/16-px units). */
static int char_width_for_font(const lv_font_t *font)
{
    if (font == FONT_26) return 13;   /* adv_w 208 / 16 = 13 */
    if (font == FONT_22) return 11;   /* adv_w 176 / 16 = 11 */
    if (font == FONT_18) return 9;    /* adv_w 144 / 16 = 9 */
    if (font == FONT_16) return 8;    /* adv_w 128 / 16 = 8 */
    if (font == FONT_14) return 7;    /* adv_w 112 / 16 = 7 */
    return 6;                         /* FONT_11: adv_w  96 / 16 = 6 */
}

/* Count UTF-8 characters in the first byte_len bytes of text. */
static int utf8_chars_in_bytes(const char *text, size_t byte_len)
{
    int count = 0;
    for (size_t i = 0; i < byte_len; i++) {
        if ((text[i] & 0xC0) != 0x80) count++;
    }
    return count;
}

/* Return the byte offset of the n-th UTF-8 character in text. */
static size_t utf8_char_offset(const char *text, int n)
{
    size_t off = 0;
    int ch = 0;
    while (text[off] && ch < n) {
        unsigned char c = (unsigned char)text[off];
        if (c < 0x80) off += 1;
        else if ((c & 0xE0) == 0xC0) off += 2;
        else if ((c & 0xF0) == 0xE0) off += 3;
        else off += 4;
        ch++;
    }
    return off;
}


/* Scan UTF-8 text for the first STRONG directional codepoint,
 * mirroring LVGL's auto base-direction detection. Returns +1 for
 * strong LTR (Latin, Greek, Cyrillic), -1 for strong RTL (Hebrew,
 * Arabic and their presentation forms), 0 if the text contains only
 * direction-neutral characters (spaces, digits, punctuation). */
static int utf8_first_strong_dir(const char *s, size_t len)
{
    size_t i = 0;
    while (i < len) {
        unsigned char c = (unsigned char)s[i];
        uint32_t cp;
        size_t adv;
        if (c < 0x80) {
            cp = c; adv = 1;
        } else if ((c & 0xE0) == 0xC0 && i + 1 < len) {
            cp = ((uint32_t)(c & 0x1F) << 6) |
                 ((unsigned char)s[i + 1] & 0x3F);
            adv = 2;
        } else if ((c & 0xF0) == 0xE0 && i + 2 < len) {
            cp = ((uint32_t)(c & 0x0F) << 12) |
                 (((unsigned char)s[i + 1] & 0x3F) << 6) |
                 ((unsigned char)s[i + 2] & 0x3F);
            adv = 3;
        } else if ((c & 0xF8) == 0xF0 && i + 3 < len) {
            cp = ((uint32_t)(c & 0x07) << 18) |
                 (((unsigned char)s[i + 1] & 0x3F) << 12) |
                 (((unsigned char)s[i + 2] & 0x3F) << 6) |
                 ((unsigned char)s[i + 3] & 0x3F);
            adv = 4;
        } else {
            cp = c; adv = 1;
        }
        i += adv;
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
    }
    return 0;
}

/* Render the currently-bound pane (s_rp) for its bound document. The
 * caller binds the pane (pane_bind) and decides whether this pane shows
 * the cursor (draw_cursor is true only for the focused pane). The
 * shared title / battery widgets are refreshed once by the
 * editor_ui_refresh() wrapper, not here. */
static void refresh_active_pane(bool draw_cursor)
{
    /* Show logo when no file is loaded and buffer is empty */
    size_t text_len = 0;
    const char *flat_text = editor_get_text(&text_len);
    bool show_logo = (editor_get_file_path() == NULL && text_len == 0);
    if (s_img_logo) {
        if (show_logo) {
            lv_obj_remove_flag(s_img_logo, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_img_logo, LV_OBJ_FLAG_HIDDEN);
        }
    }

    int total  = editor_get_line_count();

    /* Selection range (byte offsets in flat text) */
    size_t sel_start = 0, sel_end = 0;
    bool has_sel = editor_selection_active();
    if (has_sel) editor_get_selection_range(&sel_start, &sel_end);

    int cur_line, cur_col;
    editor_get_cursor_pos(&cur_line, &cur_col);

    int cur_y = -1;      /* cursor y position (set when cursor line is rendered) */
    int cur_x = -1;      /* cursor x position */
    int cur_h = LINE_H;  /* cursor height (matches font of cursor line) */

    /* Render visible lines.  Wrapped lines consume more vertical space
     * than a single LINE_H row, so the cursor may be pushed off-screen
     * even though ensure_cursor_visible() thought it was in range.
     * When that happens, increment scroll and re-render (bounded). */
    for (int _scroll_retry = 0; _scroll_retry < MAX_LINE_LABELS;
         _scroll_retry++) {
        int scroll = editor_get_scroll_line();
        bool in_code = false;

        /* Track code fence state up to scroll line */
        for (int i = 0; i < scroll && i < total; i++) {
            size_t ll;
            const char *lt = editor_get_line(i, &ll);
            if (md_is_code_fence(lt, ll)) in_code = !in_code;
        }

        std::string line_buf;   /* reusable scratch for bullet/HR prefixing */
        std::string tmp;        /* final rendered text for the current slot */
        int y_pos = 0;       /* running y position in editor content area */
        cur_y = -1;
        cur_x = -1;
        cur_h = LINE_H;

        for (int i = 0; i < MAX_LINE_LABELS; i++) {
            int line_idx = scroll + i;
            if (line_idx >= total || y_pos >= s_rp->h) {
                /* Only invalidate the slot when its visible state
                 * actually changes; otherwise lv_obj_add_flag()
                 * dirties the previous label rectangle on every
                 * refresh and the union of all those rectangles
                 * fills most of the editor area, defeating the
                 * partial-update path on e-paper backends. */
                if (s_line_labels[i] && s_prev_line_visible[i]) {
                    lv_obj_add_flag(s_line_labels[i], LV_OBJ_FLAG_HIDDEN);
                }
                if (s_sel_rects[i] && s_prev_line_was_selected[i]) {
                    lv_obj_add_flag(s_sel_rects[i], LV_OBJ_FLAG_HIDDEN);
                }
                /* Always clear the cached state for hidden slots so a
                 * later transition back to visible is forced through
                 * the full re-render path with a clean baseline. */
                s_prev_line_visible[i]      = false;
                s_prev_line_was_selected[i] = false;
                continue;
            }

            size_t ll;
            const char *lt = editor_get_line(line_idx, &ll);

            md_line_info_t mi;
            md_parse_line(lt, ll, &mi, in_code);
            if (mi.type == MD_LINE_CODE_FENCE) in_code = !in_code;

            /* Prepare display text */
            const char *disp_text = mi.content;
            size_t disp_len = mi.content_len;
            if (mi.type == MD_LINE_BULLET) {
                int prefix = mi.indent_level * 2;
                line_buf.assign((size_t)prefix, ' ');
                line_buf.append("* ");
                line_buf.append(disp_text, disp_len);
                disp_text = line_buf.data();
                disp_len  = line_buf.size();
            } else if (mi.type == MD_LINE_HR) {
                line_buf.assign(40, '-');
                disp_text = line_buf.data();
                disp_len  = line_buf.size();
            } else if (mi.type == MD_LINE_EMPTY) {
                disp_text = " ";
                disp_len = 1;
            }

            /* Build the final display string up-front so we can compare
             * against the cached previous content before touching any
             * LVGL state. The std::string grows to fit, so paragraphs
             * of arbitrary length render in full without truncation. */
            tmp.assign(disp_text, disp_len);

            /* Determine whether this line intersects the active
             * selection (used both for the highlight branches below
             * and to disable the fast-path cache when selection is
             * involved -- the inversion / overlay rendering needs
             * full re-evaluation). */
            bool line_intersects_sel = false;
            if (has_sel) {
                size_t loff = (size_t)(lt - flat_text);
                size_t leoff = loff + ll;
                if (sel_start < leoff && sel_end > loff) {
                    line_intersects_sel = true;
                } else if (leoff < text_len &&
                           flat_text[leoff] == '\n' &&
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
                            s_prev_line_text[i] == tmp;

            int line_h;
            int rendered_h;
            if (can_skip) {
                /* Reuse cached layout metrics; the label still holds
                 * the same text/font/width so its rendered geometry
                 * is unchanged. lv_label_get_letter_pos() (used for
                 * the cursor below) is non-mutating and works on the
                 * existing label state. */
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
                }
                lv_obj_remove_flag(s_line_labels[i], LV_OBJ_FLAG_HIDDEN);
                lv_obj_remove_style_all(s_line_labels[i]);
                lv_obj_add_style(s_line_labels[i], style_for_type(mi.type), 0);
                /* Re-apply width after style reset (remove_style_all clears it)
                 * so that LV_LABEL_LONG_WRAP can wrap at the correct boundary. */
                lv_obj_set_width(s_line_labels[i], s_rp->w - 4);
                lv_label_set_text_static(s_line_labels[i], "");
                lv_label_set_text(s_line_labels[i], tmp.c_str());
                lv_obj_set_pos(s_line_labels[i], 2, y_pos);

                /* Get the font for this line to compute correct line height */
                const lv_font_t *line_font = lv_obj_get_style_text_font(
                                                s_line_labels[i], LV_PART_MAIN);
                line_h = lv_font_get_line_height(line_font ? line_font : body_font());

                /* Determine actual rendered height (may be taller if text wraps) */
                lv_obj_update_layout(s_line_labels[i]);
                rendered_h = lv_obj_get_height(s_line_labels[i]);
                if (rendered_h < line_h) rendered_h = line_h;

                /* Update cache with what we just drew. */
                s_prev_line_text[i]    = tmp;
                s_prev_line_type[i]    = (int)mi.type;
                s_prev_line_y[i]       = y_pos;
                s_prev_line_h[i]       = rendered_h;
                s_prev_line_visible[i] = true;
            }

            /* Selection highlight.  Fully-selected lines and multi-row
             * partial selections use color inversion on the label
             * itself (black bg, white text).  Single-row partial
             * selections use an overlay label (s_sel_rects) that
             * renders only the selected substring in white on black,
             * positioned exactly over those characters. */
            if (has_sel) {
                size_t line_off = (size_t)(lt - flat_text);
                size_t line_end_off = line_off + ll;
                bool fully = (sel_start <= line_off &&
                              sel_end >= line_end_off);
                bool partial = !fully &&
                    (sel_start < line_end_off && sel_end > line_off);
                /* Include trailing newline for overlap check */
                if (!fully && !partial && line_end_off < text_len &&
                    flat_text[line_end_off] == '\n') {
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
                    if (s_sel_rects[i])
                        lv_obj_add_flag(s_sel_rects[i], LV_OBJ_FLAG_HIDDEN);
                } else if (partial && s_sel_rects[i]) {
                    /* Compute selection byte range within this line */
                    size_t raw_s = (sel_start > line_off)
                                       ? sel_start - line_off : 0;
                    size_t raw_e = (sel_end < line_end_off)
                                       ? sel_end - line_off : ll;
                    /* Convert raw byte offsets to display char indices.
                     * md_parse_line may strip a prefix (e.g. "# ") or
                     * a bullet formatter may prepend characters. */
                    size_t prefix_bytes = (size_t)(mi.content - lt);
                    int disp_s, disp_e;
                    if (raw_s <= prefix_bytes) disp_s = 0;
                    else disp_s = utf8_chars_in_bytes(
                                      mi.content, raw_s - prefix_bytes);
                    if (raw_e <= prefix_bytes) disp_e = 0;
                    else disp_e = utf8_chars_in_bytes(
                                      mi.content, raw_e - prefix_bytes);
                    if (mi.type == MD_LINE_BULLET) {
                        int bp = mi.indent_level * 2 + 2;
                        disp_s += bp;
                        disp_e += bp;
                    }
                    if (disp_e > disp_s) {
                        lv_point_t sp, ep;
                        lv_label_get_letter_pos(s_line_labels[i],
                                                (uint32_t)disp_s, &sp);
                        lv_label_get_letter_pos(s_line_labels[i],
                                                (uint32_t)disp_e, &ep);
                        if (sp.y == ep.y) {
                            /* Single visual row: overlay label with
                             * only the selected substring in white
                             * on black background. */
                            size_t byte_s = utf8_char_offset(tmp.c_str(), disp_s);
                            size_t byte_e = utf8_char_offset(tmp.c_str(), disp_e);
                            if (byte_s > tmp.size()) byte_s = tmp.size();
                            if (byte_e > tmp.size()) byte_e = tmp.size();
                            if (byte_e < byte_s)     byte_e = byte_s;
                            std::string sel_buf = tmp.substr(byte_s, byte_e - byte_s);

                            const lv_font_t *sf =
                                lv_obj_get_style_text_font(
                                    s_line_labels[i], LV_PART_MAIN);
                            lv_obj_set_style_text_font(
                                s_sel_rects[i],
                                sf ? sf : body_font(), 0);
                            lv_label_set_text(s_sel_rects[i], sel_buf.c_str());
                            lv_obj_set_pos(s_sel_rects[i],
                                           2 + sp.x, y_pos + sp.y);
                            lv_obj_move_foreground(s_sel_rects[i]);
                            lv_obj_remove_flag(s_sel_rects[i],
                                               LV_OBJ_FLAG_HIDDEN);
                        } else {
                            /* Multi-row partial: fall back to
                             * full-line inversion on the label. */
                            lv_obj_set_style_bg_color(s_line_labels[i],
                                                      theme_fg(), 0);
                            lv_obj_set_style_bg_opa(s_line_labels[i],
                                                    LV_OPA_COVER, 0);
                            lv_obj_set_style_text_color(s_line_labels[i],
                                                        theme_bg(), 0);
                            lv_obj_add_flag(s_sel_rects[i],
                                            LV_OBJ_FLAG_HIDDEN);
                        }
                    } else {
                        lv_obj_add_flag(s_sel_rects[i],
                                        LV_OBJ_FLAG_HIDDEN);
                    }
                } else {
                    if (s_sel_rects[i])
                        lv_obj_add_flag(s_sel_rects[i], LV_OBJ_FLAG_HIDDEN);
                }
            } else {
                if (s_sel_rects[i])
                    lv_obj_add_flag(s_sel_rects[i], LV_OBJ_FLAG_HIDDEN);
            }

            /* Remember whether this slot is currently rendering a
             * selected line so the next refresh can decide whether
             * to invoke the full re-render path that clears the
             * inversion / overlay. */
            s_prev_line_was_selected[i] = line_intersects_sel;

            /* If this is the cursor line, compute its pixel position.
             * For headings, md_parse_line strips the "# " prefix from content,
             * so the cursor column (which counts from the raw line start) needs
             * to be adjusted. */
            if (line_idx == cur_line) {
                int col_in_display = cur_col;
                /* content pointer offset from raw line start, in UTF-8 chars */
                if (mi.content > lt) {
                    int prefix_chars = 0;
                    for (const char *pp = lt; pp < mi.content; pp++) {
                        if ((*pp & 0xC0) != 0x80) prefix_chars++;
                    }
                    col_in_display -= prefix_chars;
                    if (col_in_display < 0) col_in_display = 0;
                }

                /* Use LVGL to find the actual pixel position of the cursor
                 * character.  This correctly handles word-level wrapping
                 * where the break point differs from a simple chars_per_row
                 * calculation. */
                lv_point_t lpos;
                lv_label_get_letter_pos(s_line_labels[i],
                                        (uint32_t)col_in_display, &lpos);
                cur_x = 2 + lpos.x;
                cur_y = y_pos + lpos.y;
                cur_h = line_h;

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

            y_pos += rendered_h;
        }

        /* If the cursor line is in the expected range but wrapped lines
         * pushed it off-screen, increment scroll and re-render.
         * Use cur_y + cur_h to ensure the full cursor row is visible. */
        if (cur_line >= scroll && scroll < cur_line &&
            (cur_y < 0 || cur_y + cur_h > s_rp->h)) {
            editor_set_scroll_line(scroll + 1);
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
        editor_set_scroll_line(cur_line);
    } else if (cur_line >= scroll + VISIBLE_LINES) {
        editor_set_scroll_line(cur_line - VISIBLE_LINES + 1);
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

/* Walk N UTF-8 codepoints into a buffer and return the resulting
 * byte offset, clamped to len. The display layer already laid the
 * codepoints out in monospaced glyphs, so this is the inverse of
 * what lv_label_get_letter_on() reports for our needs. */
static int ui_utf8_skip_cp(const char *s, int len, int n_cp)
{
    int i = 0;
    while (n_cp > 0 && i < len) {
        unsigned char c = (unsigned char)s[i];
        int adv = 1;
        if      ((c & 0x80) == 0x00) adv = 1;
        else if ((c & 0xE0) == 0xC0) adv = 2;
        else if ((c & 0xF0) == 0xE0) adv = 3;
        else if ((c & 0xF8) == 0xF0) adv = 4;
        if (i + adv > len) break;
        i += adv;
        n_cp--;
    }
    return i;
}

/* Convert a point (in s_cont_edit local coordinates) to a byte
 * offset within the editor's flat text buffer. Returns true on
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
        if (!s_prev_line_visible[i]) continue;
        if (s_prev_line_y[i] < 0)    continue;
        int y1 = s_prev_line_y[i];
        int y2 = y1 + s_prev_line_h[i];
        if (y >= y1 && y < y2) { slot = i; break; }
    }
    if (slot < 0) {
        /* Point lies below the last visible line -- map to end-of-document. */
        size_t total = 0;
        (void)editor_get_text(&total);
        *out_off = total;
        return true;
    }

    int line_idx = editor_get_scroll_line() + slot;
    int total_lines = editor_get_line_count();
    if (line_idx >= total_lines) {
        size_t total = 0;
        editor_get_text(&total);
        *out_off = total;
        return true;
    }

    size_t ll = 0;
    const char *lt = editor_get_line(line_idx, &ll);
    if (!lt) return false;

    size_t flat_len = 0;
    const char *flat = editor_get_text(&flat_len);
    size_t line_off = (size_t)(lt - flat);

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
        lv_point_t p;
        p.x = x_in_label;
        p.y = y_in_label;
        /* bidi=true so taps on RTL/mixed lines map through LVGL's
         * visual->logical reordering instead of the raw byte order. */
        disp_char = (int)lv_label_get_letter_on(s_line_labels[slot], &p, true);
    }
    if (disp_char < 0) disp_char = 0;

    /* Re-parse the line so we know the markdown-prefix lengths and
     * can map the display character index back to a raw byte offset
     * within the original line. */
    md_line_info_t mi;
    md_parse_line(lt, ll, &mi, false /* in_code irrelevant for column math */);

    int disp_prefix_cp = 0;   /* codepoints rendered before mi.content */
    int raw_prefix_b   = 0;   /* bytes skipped in the raw line before mi.content */

    switch (mi.type) {
    case MD_LINE_BULLET: {
        /* Display string is "<2*indent spaces>* <bullet body>".
         * The bullet body is the same as mi.content. The raw line
         * is "<spaces>* <body>", which has the same number of
         * leading display characters as our synthesized prefix --
         * so disp_prefix_cp and raw_prefix_b describe equivalent
         * spans and cancel out for ASCII bullet prefixes. */
        disp_prefix_cp = mi.indent_level * 2 + 2;
        raw_prefix_b   = (int)(mi.content - lt);
        break;
    }
    case MD_LINE_HR:
    case MD_LINE_EMPTY:
        /* Display content is synthetic; map every point to the start
         * of the raw line so Enter/typing inserts there. */
        *out_off = line_off;
        return true;
    case MD_LINE_CODE_FENCE:
    case MD_LINE_H1: case MD_LINE_H2: case MD_LINE_H3: case MD_LINE_H4:
    case MD_LINE_BLOCKQUOTE:
    case MD_LINE_NUMBERED:
    case MD_LINE_PARAGRAPH:
    case MD_LINE_CODE_CONTENT:
    default:
        /* Display is mi.content unchanged; the raw line skipped
         * (mi.content - lt) bytes for the markdown marker. */
        disp_prefix_cp = 0;
        raw_prefix_b   = (int)(mi.content - lt);
        if (raw_prefix_b < 0) raw_prefix_b = 0;
        break;
    }

    /* Clamp the display column to the rendered text length so a
     * point past end-of-line places the cursor right after the last char. */
    int content_cp = disp_char - disp_prefix_cp;
    if (content_cp < 0) content_cp = 0;

    /* Walk content_cp codepoints into mi.content to get the raw
     * byte offset inside the content span, then add the raw prefix. */
    int byte_in_content = ui_utf8_skip_cp(mi.content,
                                          (int)mi.content_len, content_cp);
    *out_off = line_off + (size_t)raw_prefix_b + (size_t)byte_in_content;
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
                /* Conservative: at-or-past viewport bottom on the
                 * last line -> nothing more to move to. */
                return;
            }
        }
        if (direction < 0 && cur_line == 0) {
            /* First logical line: if the cursor is already on the
             * first visual row (cy <= 0), there is no row above. */
            if (cy <= 0) return;
        }
    }

    /* If the target visual row is outside the rendered editor area,
     * scroll one logical line and re-render so the row above/below
     * becomes addressable via ui_point_to_offset(). */
    for (int attempt = 0; attempt < 2; attempt++) {
        if (target_y >= 0 && target_y < s_rp->h) break;
        int sc = editor_get_scroll_line();
        if (direction < 0) {
            if (sc <= 0) {
                editor_move_up();
                s_visual_goal_x = gx;
                return;
            }
            editor_set_scroll_line(sc - 1);
        } else {
            int total = editor_get_line_count();
            if (sc + 1 >= total) {
                editor_move_down();
                s_visual_goal_x = gx;
                return;
            }
            editor_set_scroll_line(sc + 1);
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

/* Walk N UTF-8 codepoints into a buffer and return the resulting
 * byte offset, clamped to len. The display layer already laid the
 * codepoints out in monospaced glyphs, so this is the inverse of
 * what lv_label_get_letter_on() reports for our needs. */
/* (ui_utf8_skip_cp / ui_point_to_offset are defined unconditionally
 * above so visual-line cursor movement can use them too.) */

/* Last tap state for software double-tap detection. LVGL fires
 * LV_EVENT_CLICKED on every release; we compare against the
 * previous one and promote consecutive close-in-time clicks to a
 * "double tap". */
static uint32_t s_last_tap_ms = 0;
static int      s_last_tap_x  = -1;
static int      s_last_tap_y  = -1;
#define TOUCH_DOUBLE_TAP_MS   400  /* same as the LVGL default short-click streak */
#define TOUCH_DOUBLE_TAP_PX    12  /* finger jitter tolerance */

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
 * offset within the editor's flat text buffer. Returns true on
 * success and fills *out_off; returns false if the tap landed on
 * empty space below the last rendered line. */
/* Convert a tap point (in s_cont_edit local coordinates) to a byte
 * offset within the editor's flat text buffer. Implemented as a
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
    size_t flat_len = 0;
    const char *flat = editor_get_text(&flat_len);
    if (!flat || flat_len == 0) return;
    if (off > flat_len) off = flat_len;

    /* Walk left until non-word byte (UTF-8-safe: continuation bytes
     * test as word chars so multi-byte codepoints are not split). */
    size_t start = off;
    while (start > 0 && touch_is_word_byte((unsigned char)flat[start - 1])) {
        start--;
    }
    size_t end = off;
    while (end < flat_len && touch_is_word_byte((unsigned char)flat[end])) {
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
    int new_scroll = scroll + delta_lines;
    if (new_scroll > total - 1) new_scroll = total - 1;
    if (new_scroll < 0) new_scroll = 0;
    if (new_scroll != scroll) {
        editor_set_scroll_line(new_scroll);
        refresh_active_pane(pane_idx == s_focus);
        standby_reset_timer();
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
        bool is_double = (now - s_last_tap_ms) <= TOUCH_DOUBLE_TAP_MS &&
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

#endif /* CONFIG_DRAFTLING_TOUCHSCREEN */

/* ---- File browser ---- */

static void refresh_file_list(void)
{
    /* Remember the currently-selected entry's filename (if any) so we
     * can restore the selection after the list is rebuilt. This keeps
     * the highlight where the user left it when returning from the
     * editor, and also preserves selection across a Git-sync refresh
     * that added or removed files. */
    char remembered_name[sizeof(s_browser_entries[0].name)] = {0};
    if (s_browser_list && s_browser_sel >= 0 &&
        s_browser_sel < (int)lv_obj_get_child_count(s_browser_list)) {
        lv_obj_t *cur_btn = lv_obj_get_child(s_browser_list, s_browser_sel);
        if (cur_btn) {
            int cur_idx = (int)(intptr_t)lv_obj_get_user_data(cur_btn);
            if (cur_idx >= 0 && cur_idx < s_browser_count) {
                strncpy(remembered_name, s_browser_entries[cur_idx].name,
                        sizeof(remembered_name) - 1);
            }
        }
    }

    const char *mp = sd_card_get_mount_point();
    s_browser_count = sd_card_list_dir(mp, s_browser_entries, 64);
    if (s_browser_count < 0) s_browser_count = 0;

    /* Filter to show only .md files and directories */
    lv_obj_clean(s_browser_list);

    int restored_row = -1;
    int row = 0;
    for (int i = 0; i < s_browser_count; i++) {
        const char *name = s_browser_entries[i].name;
        bool show = s_browser_entries[i].is_dir;
        if (!show) {
            size_t nlen = strlen(name);
            show = (nlen > 3 && strcmp(name + nlen - 3, ".md") == 0);
        }
        if (show) {
            char label[sizeof(s_browser_entries[0].name) + 8];
            if (s_browser_entries[i].is_dir)
                snprintf(label, sizeof(label), "[DIR] %.255s", name);
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

extern "C" void editor_ui_show_file_browser(void)
{
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
        return;
    }

    /* Mark the browser as the active screen up front so key routing and
     * refresh gating switch over immediately, regardless of the focused
     * pane's per-document editor mode (see s_editor_screen_active). */
    s_editor_screen_active = false;

    /* Single-pane: opening a file replaces the current document, so
     * close it now (historical behavior; the buffer is reloaded
     * from disk when a file is picked). */
    editor_close_file();
    s_open_target_pane = 0;
    /* Operate on the full-screen browser list. */
    s_browser_list = s_list_files;
    refresh_file_list();

    /* The Wi-Fi connection state is conveyed by the Wi-Fi icon; the
     * status bar shows the static hint so it doesn't compete with
     * transient messages (e.g. Git-sync progress). */
    if (s_lbl_br_status) {
        lv_label_set_text(s_lbl_br_status, "F1:Menu  N:New file");
    }

    sync_battery_labels();
    lv_scr_load(s_scr_browser);
}

extern "C" void editor_ui_show_editor(void)
{
    s_editor_screen_active = true;
    editor_set_mode(EDITOR_MODE_EDITING);
    sync_battery_labels();
    lv_scr_load(s_scr);
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
    int avail = SCR_W - 2 /* left margin */ - right_reserved;
    if (avail < 0) avail = 0;
    return avail;
}

/* Hide the editor's status label when its current text is wider than
 * the available area (so only the battery percentage remains visible
 * in the bottom bar); show it otherwise. Greybeard is monospaced, so
 * the rendered width is utf8_chars * char_width(FONT_11). */
static void update_status_visibility(void)
{
    if (!s_lbl_status) return;
    const char *text = lv_label_get_text(s_lbl_status);
    if (!text) text = "";
    int chars = utf8_chars_in_bytes(text, strlen(text));
    int text_w = chars * char_width_for_font(FONT_11);
    if (text_w > status_avail_width()) {
        lv_obj_add_flag(s_lbl_status, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_remove_flag(s_lbl_status, LV_OBJ_FLAG_HIDDEN);
    }
}

/* Default status-bar text for the editor screen. Kept in one place
 * so the auto-clear timer and the initial label setup agree. */
#define EDITOR_DEFAULT_STATUS \
    "F1:Menu Ctrl+S:Save Ctrl+L:Layout Ctrl+G:Git Esc:Files"

/* Restore the status bar of both screens to its standard, no-message
 * state (the same text the screens show when they are first entered).
 * The WiFi connection state is communicated by the Wi-Fi icon in the
 * title / status bar, so the default text never mentions it -- otherwise
 * it would reappear between transient Git-sync progress messages and
 * confuse the user. */
static void restore_default_status(void)
{
    if (s_lbl_status) {
        lv_label_set_text(s_lbl_status, EDITOR_DEFAULT_STATUS);
    }
    update_status_visibility();
    if (s_lbl_br_status) {
        lv_label_set_text(s_lbl_br_status, "F1:Menu  N:New file");
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

extern "C" void editor_ui_set_ble_prompt_text(const char *text)
{
    /* Update the bottom-half label on the boot "BLE prompt" screen.
     * editor_ui_init() loads this screen with a generic "Initializing..."
     * caption because at that point we do not yet know whether a
     * USB keyboard will enumerate; main.cpp calls this function once
     * it has decided to bring up BLE, so the "Searching for BLE
     * keyboard..." message only appears on the no-USB path. */
    if (!s_ble_prompt_lbl || !text) return;
    lv_label_set_text(s_ble_prompt_lbl, text);
}

/* Apply the "selected" styling to item `sel` and the "unselected"
 * styling to every other item.  Used after a full list rebuild. */
static void apply_list_selection_styles(lv_obj_t *list, int sel)
{
    uint32_t count = lv_obj_get_child_count(list);
    for (uint32_t i = 0; i < count; i++) {
        lv_obj_t *child = lv_obj_get_child(list, i);
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
     * Without this, a list taller than the panel (e.g. on PaperS3 at
     * DRAFTLING_DISPLAY_SCALE >= 2) keeps its previous scroll offset
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
    }
    if (sel >= 0 && (uint32_t)sel < count) {
        lv_obj_t *cur = lv_obj_get_child(list, sel);
        lv_obj_set_style_bg_color(cur, theme_fg(), 0);
        lv_obj_set_style_bg_opa(cur, LV_OPA_COVER, 0);
        lv_obj_set_style_text_color(cur, theme_bg(), 0);
        /* Scroll the list so the highlighted row is always visible.
         * Needed when the list is taller than the panel (e.g. on
         * PaperS3 with DRAFTLING_DISPLAY_SCALE = 3, where only a few
         * items fit on screen).  LV_ANIM_OFF avoids smooth-scroll
         * animation, which would force many extra e-paper refreshes. */
        lv_obj_scroll_to_view(cur, LV_ANIM_OFF);
    }
}

static void refresh_menu_items(void)
{
    lv_obj_clean(s_menu_list);

    char buf[80];

    /* 0: BLE status */
    snprintf(buf, sizeof(buf), "BLE: %s",
             ble_keyboard_is_connected()
                 ? ble_keyboard_get_device_name()
                 : "not connected");
    lv_list_add_btn(s_menu_list, NULL, buf);

    /* 1: BLE scan */
    lv_list_add_btn(s_menu_list, NULL, "BLE: Start scan");

    /* 2: WiFi status / connect */
    if (wifi_manager_is_connected()) {
        snprintf(buf, sizeof(buf), "WiFi: %s (%s)",
                 wifi_manager_get_ssid(), wifi_manager_get_ip());
    } else {
        snprintf(buf, sizeof(buf), "WiFi: Connect");
    }
    lv_list_add_btn(s_menu_list, NULL, buf);

    /* 3: WiFi disconnect */
    lv_list_add_btn(s_menu_list, NULL, "WiFi: Disconnect");

    /* 4: Git sync */
    snprintf(buf, sizeof(buf), "Git Sync%s",
             git_sync_is_configured() ? "" : " (not configured)");
    lv_list_add_btn(s_menu_list, NULL, buf);

    /* 5: Keyboard layout */
    snprintf(buf, sizeof(buf), "Keyboard: %s  (Enter to cycle)",
             kb_layout_name(kb_layout_get()));
    lv_list_add_btn(s_menu_list, NULL, buf);

    /* 6: Settings */
    lv_list_add_btn(s_menu_list, NULL, "Settings...");

    /* 7: Close menu */
    lv_list_add_btn(s_menu_list, NULL, "Close menu (Esc / F1)");

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

static void show_menu(void)
{
    s_menu_open = true;
    s_menu_sel = 0;
    refresh_menu_items();
    sync_battery_labels();
    lv_scr_load(s_scr_menu);
}

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
 * Indices vary slightly between monochrome and color builds; on
 * color LCDs an extra "Color theme" item is inserted after the font
 * size. Use the SETTINGS_IDX_* constants instead of bare integers
 * everywhere downstream so the two layouts stay in lock-step.
 *
 * Backlight is gated separately on CONFIG_DRAFTLING_DISPLAY_HAS_BACKLIGHT
 * so reflective-LCD / e-paper builds (no controllable backlight) skip
 * the entry entirely. */
#define SETTINGS_IDX_TIMEOUT  0
#define SETTINGS_IDX_FONTSZ   1
#define SETTINGS_IDX_MAXFILE  2
#if defined(CONFIG_DRAFTLING_DISPLAY_HAS_BACKLIGHT)
#define SETTINGS_IDX_BACKLIGHT 3
#define _SETTINGS_NEXT_AFTER_BACKLIGHT 4
#else
#define SETTINGS_IDX_BACKLIGHT (-1)
#define _SETTINGS_NEXT_AFTER_BACKLIGHT 3
#endif
#if defined(CONFIG_DRAFTLING_DISPLAY_COLOR)
#define SETTINGS_IDX_THEME    (_SETTINGS_NEXT_AFTER_BACKLIGHT + 0)
#define _SETTINGS_NEXT_AFTER_THEME (_SETTINGS_NEXT_AFTER_BACKLIGHT + 1)
#else
#define SETTINGS_IDX_THEME    (-1)
#define _SETTINGS_NEXT_AFTER_THEME (_SETTINGS_NEXT_AFTER_BACKLIGHT + 0)
#endif
#if defined(CONFIG_DRAFTLING_DISPLAY_CAN_ROTATE)
#define SETTINGS_IDX_ROTATE   (_SETTINGS_NEXT_AFTER_THEME + 0)
#define _SETTINGS_NEXT_AFTER_ROTATE (_SETTINGS_NEXT_AFTER_THEME + 1)
#else
#define SETTINGS_IDX_ROTATE   (-1)
#define _SETTINGS_NEXT_AFTER_ROTATE (_SETTINGS_NEXT_AFTER_THEME + 0)
#endif
#define SETTINGS_IDX_SLEEP    (_SETTINGS_NEXT_AFTER_ROTATE + 0)
#define SETTINGS_IDX_RESET    (_SETTINGS_NEXT_AFTER_ROTATE + 1)
#define SETTINGS_IDX_BACK     (_SETTINGS_NEXT_AFTER_ROTATE + 2)
#define SETTINGS_ITEM_COUNT   (_SETTINGS_NEXT_AFTER_ROTATE + 3)

static void refresh_settings_items(void)
{
    lv_obj_clean(s_settings_list);

    char buf[80];
    uint32_t cur = standby_get_timeout();
    const char *cur_label = "custom";
    int idx = find_timeout_option(cur);
    if (idx >= 0 && idx < TIMEOUT_OPTION_COUNT)
        cur_label = TIMEOUT_LABELS[idx];

    /* Standby timeout */
    snprintf(buf, sizeof(buf), "Standby timeout: %s", cur_label);
    lv_list_add_btn(s_settings_list, NULL, buf);

    /* Base font size */
    {
        int fi = find_font_size_option(s_font_size);
        snprintf(buf, sizeof(buf), "Base font size: %s",
                 FONT_SIZE_LABELS[fi]);
        lv_list_add_btn(s_settings_list, NULL, buf);
    }

    /* Max file size (read-only).
     * Sized dynamically by editor_init() from the PSRAM that was free
     * at boot; surface it here so the user knows the hard upper limit
     * for files they can open. */
    snprintf(buf, sizeof(buf), "Max file size: %u KB (read-only)",
             (unsigned)(editor_get_max_doc_size() / 1024));
    lv_list_add_btn(s_settings_list, NULL, buf);

#if defined(CONFIG_DRAFTLING_DISPLAY_HAS_BACKLIGHT)
    /* Backlight brightness (LCD boards with a controllable backlight) */
    snprintf(buf, sizeof(buf), "Backlight: %d%%", s_backlight_pct);
    lv_list_add_btn(s_settings_list, NULL, buf);
#endif

#if defined(CONFIG_DRAFTLING_DISPLAY_COLOR)
    /* Color theme (color LCDs only) */
    snprintf(buf, sizeof(buf), "Color theme: %s",
             COLOR_THEMES[s_theme_idx].name);
    lv_list_add_btn(s_settings_list, NULL, buf);
#endif

#if defined(CONFIG_DRAFTLING_DISPLAY_CAN_ROTATE)
    /* Runtime 180-degree display flip */
    snprintf(buf, sizeof(buf), "Rotate 180: %s",
             s_rotate180 ? "on" : "off");
    lv_list_add_btn(s_settings_list, NULL, buf);
#endif

    /* Sleep now */
    lv_list_add_btn(s_settings_list, NULL, "Sleep now");

    /* Factory reset */
    if (s_factory_reset_confirm) {
        lv_list_add_btn(s_settings_list, NULL,
                        "Factory reset -- ENTER again to confirm");
    } else {
        lv_list_add_btn(s_settings_list, NULL, "Factory reset");
    }

    /* Back */
    lv_list_add_btn(s_settings_list, NULL, "Back (Esc)");

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

static void show_settings(void)
{
    s_settings_open = true;
    s_menu_open = false;
    s_settings_sel = 0;
    s_factory_reset_confirm = false;
#if defined(CONFIG_DRAFTLING_DISPLAY_COLOR)
    s_theme_picker_open = false;
#endif
    refresh_settings_items();
    sync_battery_labels();
    lv_scr_load(s_scr_settings);
}

static void close_settings(void)
{
    s_settings_open = false;
#if defined(CONFIG_DRAFTLING_DISPLAY_COLOR)
    s_theme_picker_open = false;
#endif
    show_menu();
}

static void settings_activate_item(int idx)
{
    /* Moving away from the factory-reset item cancels confirmation */
    if (idx != SETTINGS_IDX_RESET && s_factory_reset_confirm) {
        s_factory_reset_confirm = false;
        refresh_settings_items();
    }

    if (idx == SETTINGS_IDX_TIMEOUT) {
        /* Cycle to next timeout option */
        uint32_t cur = standby_get_timeout();
        int opt = find_timeout_option(cur);
        opt = (opt + 1) % TIMEOUT_OPTION_COUNT;
        standby_set_timeout(TIMEOUT_OPTIONS[opt]);
        refresh_settings_items();
    } else if (idx == SETTINGS_IDX_FONTSZ) {
        /* Cycle to next font size option */
        int fi = find_font_size_option(s_font_size);
        fi = (fi + 1) % FONT_SIZE_COUNT;
        s_font_size = FONT_SIZE_OPTIONS[fi];
        save_font_size_to_nvs();
        init_styles();
        refresh_settings_items();
#if defined(CONFIG_DRAFTLING_DISPLAY_EPD)
        /* The new font changes widget geometry; on e-paper any
         * pixels left from the previous layout that the new layout
         * does not cover would otherwise stay on screen as garbage
         * until the next full refresh. Wipe the framebuffer and
         * invalidate the active screen so LVGL repaints everything;
         * display_clear() also flags the next flush as a full
         * refresh, clearing any accumulated ghosting. */
        display_clear(0xFF);
        lv_obj_invalidate(lv_scr_act());
#endif
    } else if (idx == SETTINGS_IDX_MAXFILE) {
        /* Read-only display of the dynamically-sized editor buffer.
         * Enter is a no-op; the value is fixed at editor_init() time. */
#if defined(CONFIG_DRAFTLING_DISPLAY_HAS_BACKLIGHT)
    } else if (idx == SETTINGS_IDX_BACKLIGHT) {
        /* Cycle to next backlight brightness step. Apply immediately
         * so the user sees the change without leaving the menu. */
        int bi = find_backlight_option(s_backlight_pct);
        bi = (bi + 1) % BACKLIGHT_OPTION_COUNT;
        s_backlight_pct = BACKLIGHT_OPTIONS[bi];
        save_backlight_to_nvs();
        display_set_backlight(s_backlight_pct);
        refresh_settings_items();
#endif
#if defined(CONFIG_DRAFTLING_DISPLAY_COLOR)
    } else if (idx == SETTINGS_IDX_THEME) {
        /* Open the theme picker sub-list. The user navigates with
         * Up/Down and confirms with Enter; only on Enter is the new
         * theme persisted and the screens rebuilt. Esc returns to
         * the settings list without changes. */
        s_theme_picker_open = true;
        s_theme_picker_sel = s_theme_idx;
        refresh_theme_picker_items();
#endif
#if defined(CONFIG_DRAFTLING_DISPLAY_CAN_ROTATE)
    } else if (idx == SETTINGS_IDX_ROTATE) {
        /* Toggle the runtime 180-degree display flip. Applied
         * immediately so the user sees the new orientation without
         * leaving the menu; persisted in NVS and re-applied at boot.
         * A 180 flip does not change the reported resolution, so the
         * widget tree stays valid -- the LVGL port just repaints the
         * whole panel in the new orientation. */
        s_rotate180 = !s_rotate180;
        save_rotate180_to_nvs();
        draftling_lvgl_port_set_flip180(s_rotate180);
        refresh_settings_items();
#endif
    } else if (idx == SETTINGS_IDX_SLEEP) {
        /* Sleep now -- standby_enter_sleep() runs the registered
         * pre-sleep callback (autosave + per-board peripheral
         * teardown) before esp_deep_sleep_start(), so the menu
         * path takes exactly the same teardown sequence as the
         * inactivity timeout and the no-keyboard timeout. */
        standby_enter_sleep();
    } else if (idx == SETTINGS_IDX_RESET) {
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
    } else if (idx == SETTINGS_IDX_BACK) {
        close_settings();
    }
}

static void handle_settings_key(const kb_event_t *ev)
{
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
        if (s_settings_sel < SETTINGS_ITEM_COUNT - 1) s_settings_sel++;
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
    case KB_KEY_ESCAPE:
        close_settings();
        break;
    default:
        break;
    }
}

/* Forward declarations for WiFi background connect (defined below) */
static void wifi_connect_task(void *arg);
static void wifi_connect_async(void);

static void menu_activate_item(int idx)
{
    switch (idx) {
    case 0: /* BLE status -- no action */
        break;
    case 1: /* BLE scan */
        /* BLE may be idle (disabled) when another keyboard owns input
         * -- e.g. a USB or Tab5 keyboard was present at boot. Re-enable
         * it first so an on-demand scan can actually start. No-op when
         * BLE is already active. */
        ble_keyboard_enable();
        ble_keyboard_start_scan();
        editor_ui_set_status("BLE: scanning...");
        close_menu();
        break;
    case 2: /* WiFi connect */
        if (!wifi_manager_is_connected()) {
            editor_ui_set_status("WiFi: connecting...");
            close_menu();
            wifi_connect_async();
        }
        break;
    case 3: /* WiFi disconnect */
        wifi_manager_disconnect();
        editor_ui_set_status("WiFi: disconnected");
        close_menu();
        break;
    case 4: /* Git sync */
        /* Auto-save unsaved edits so the sync task pushes the latest content. */
        if (editor_is_modified() && editor_get_file_path()) {
            editor_save_file();
        }
        if (git_sync_is_configured() && wifi_manager_is_connected()) {
            close_menu();
            if (git_sync_start(GIT_SYNC_BOTH) == ESP_OK) {
                editor_ui_set_status("Git: syncing...");
            } else {
                char sbuf[128];
                const char *err = git_sync_get_last_error();
                snprintf(sbuf, sizeof(sbuf), "Git: %s",
                         (err && err[0]) ? err : "failed to start sync");
                editor_ui_set_status(sbuf);
            }
        } else if (!wifi_manager_is_connected()) {
            editor_ui_set_status("Git: connect WiFi first");
        } else {
            editor_ui_set_status("Git: not configured");
        }
        break;
    case 5: /* Keyboard layout cycle */
        kb_layout_next();
        refresh_menu_items();
        break;
    case 6: /* Settings */
        show_settings();
        break;
    case 7: /* Close menu */
        close_menu();
        break;
    default:
        break;
    }
}

static void handle_menu_key(const kb_event_t *ev)
{
    switch (ev->keycode) {
    case KB_KEY_UP:
        if (s_menu_sel > 0) s_menu_sel--;
        update_menu_highlight();
        break;
    case KB_KEY_DOWN:
        if (s_menu_sel < MENU_ITEM_COUNT - 1) s_menu_sel++;
        update_menu_highlight();
        break;
    case KB_KEY_ENTER:
        menu_activate_item(s_menu_sel);
        break;
    case KB_KEY_ESCAPE:
    case KB_KEY_F1:
        close_menu();
        break;
    default:
        break;
    }
}

/* ---- Save-prompt overlay ---- */

/* Compute a default filename for a new (untitled) document.
 * Returns the bare filename (no directory prefix). */
static bool generate_default_name(char *buf, size_t buf_size)
{
    const char *mp = sd_card_get_mount_point();
    if (!mp) return false;
    char path[256];
    for (int seq = 1; seq <= MAX_DRAFT_SEQ; seq++) {
        snprintf(path, sizeof(path), "%s/draft_%03d.md", mp, seq);
        if (!sd_card_file_exists(path)) {
            snprintf(buf, buf_size, "draft_%03d.md", seq);
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
    lv_obj_set_pos(s_save_cur, cx, 20);
    lv_obj_remove_flag(s_save_cur, LV_OBJ_FLAG_HIDDEN);
}

static void show_save_prompt(void)
{
    /* Pre-fill with existing filename (bare name, no directory) */
    const char *path = editor_get_file_path();
    if (path) {
        const char *slash = strrchr(path, '/');
        const char *name = slash ? slash + 1 : path;
        strncpy(s_save_buf, name, sizeof(s_save_buf) - 1);
        s_save_buf[sizeof(s_save_buf) - 1] = '\0';
    } else {
        if (!generate_default_name(s_save_buf, sizeof(s_save_buf))) {
            editor_ui_set_status("Save failed: too many drafts");
            return;
        }
    }
    s_save_pos = (int)strlen(s_save_buf);
    s_save_open = true;
    lv_obj_remove_flag(s_save_panel, LV_OBJ_FLAG_HIDDEN);
    refresh_save_prompt();
}

static void close_save_prompt(void)
{
    s_save_open = false;
    lv_obj_add_flag(s_save_panel, LV_OBJ_FLAG_HIDDEN);
}

static void save_prompt_confirm(void)
{
    if (s_save_buf[0] == '\0') {
        /* Empty name -- ignore */
        return;
    }
    const char *mp = sd_card_get_mount_point();
    if (!mp) {
        close_save_prompt();
        editor_ui_set_status("Save failed: SD card not ready");
        return;
    }
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", mp, s_save_buf);
    esp_err_t err = editor_save_file_as(path);
    close_save_prompt();
    if (err == ESP_OK) {
        char msg[80];
        snprintf(msg, sizeof(msg), "Saved as %.40s", s_save_buf);
        editor_ui_set_status(msg);
    } else {
        editor_ui_set_status("Save failed!");
    }
    editor_ui_refresh();
}

static void handle_save_prompt_key(const kb_event_t *ev)
{
    switch (ev->keycode) {
    case KB_KEY_ENTER:
        save_prompt_confirm();
        return;
    case KB_KEY_ESCAPE:
        close_save_prompt();
        editor_ui_set_status(
            "F1:Menu Ctrl+S:Save Ctrl+L:Layout Ctrl+G:Git Esc:Files");
        return;
    case KB_KEY_LEFT:
        if (s_save_pos > 0) {
            /* Move back one UTF-8 character */
            do { s_save_pos--; }
            while (s_save_pos > 0 &&
                   (s_save_buf[s_save_pos] & 0xC0) == 0x80);
        }
        break;
    case KB_KEY_RIGHT:
        if (s_save_pos < (int)strlen(s_save_buf)) {
            /* Move forward one UTF-8 character */
            s_save_pos++;
            while (s_save_pos < (int)strlen(s_save_buf) &&
                   (s_save_buf[s_save_pos] & 0xC0) == 0x80)
                s_save_pos++;
        }
        break;
    case KB_KEY_HOME:
        s_save_pos = 0;
        break;
    case KB_KEY_END:
        s_save_pos = (int)strlen(s_save_buf);
        break;
    case KB_KEY_BACKSPACE:
        if (s_save_pos > 0) {
            /* Delete the previous UTF-8 character */
            int prev = s_save_pos - 1;
            while (prev > 0 && (s_save_buf[prev] & 0xC0) == 0x80)
                prev--;
            int len = (int)strlen(s_save_buf);
            memmove(s_save_buf + prev, s_save_buf + s_save_pos,
                    (size_t)(len - s_save_pos + 1));
            s_save_pos = prev;
        }
        break;
    case KB_KEY_DELETE: {
        int len = (int)strlen(s_save_buf);
        if (s_save_pos < len) {
            /* Find end of current UTF-8 character */
            int next = s_save_pos + 1;
            while (next < len && (s_save_buf[next] & 0xC0) == 0x80)
                next++;
            memmove(s_save_buf + s_save_pos, s_save_buf + next,
                    (size_t)(len - next + 1));
        }
        break;
    }
    default: {
        /* Insert typed character (use layout translation) */
        bool ctrl = (ev->modifier & (KB_MOD_LCTRL | KB_MOD_RCTRL)) != 0;
        if (ctrl) break; /* ignore ctrl combos in filename */
        const char *text = kb_layout_translate(ev->keycode, ev->modifier);
        if (text && text[0]) {
            size_t tlen = strlen(text);
            size_t cur_len = strlen(s_save_buf);
            /* Reject path separators and control chars */
            if (text[0] == '/' || text[0] == '\\' || text[0] < 0x20) break;
            if (cur_len + tlen < sizeof(s_save_buf) - 1) {
                memmove(s_save_buf + s_save_pos + tlen,
                        s_save_buf + s_save_pos,
                        cur_len - (size_t)s_save_pos + 1);
                memcpy(s_save_buf + s_save_pos, text, tlen);
                s_save_pos += (int)tlen;
            }
        }
        break;
    }
    }
    refresh_save_prompt();
}

/* ---- Exit (Esc) prompt overlay ----
 * Asks whether to save, exit without saving, or cancel when Esc is
 * pressed in the editor with unsaved changes. */

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

static void show_exit_prompt(void)
{
    if (!s_exit_panel) return;
    s_exit_open = true;
    s_exit_sel  = EXIT_OPT_SAVE;
    lv_obj_remove_flag(s_exit_panel, LV_OBJ_FLAG_HIDDEN);
    refresh_exit_prompt();
}

static void exit_prompt_activate(void)
{
    switch (s_exit_sel) {
    case EXIT_OPT_SAVE:
        close_exit_prompt();
        if (editor_get_file_path()) {
            /* Known filename -- save in place and leave the editor. */
            if (editor_save_file() == ESP_OK) {
                editor_ui_show_file_browser();
            } else {
                editor_ui_set_status("Save failed!");
            }
        } else {
            /* Untitled document -- ask for a filename first. The user
             * stays in the editor; once saved a later Esc exits
             * cleanly because the document is no longer modified. */
            show_save_prompt();
        }
        return;
    case EXIT_OPT_DISCARD:
        close_exit_prompt();
        editor_ui_show_file_browser();
        return;
    case EXIT_OPT_CANCEL:
    default:
        close_exit_prompt();
        editor_ui_set_status(
            "F1:Menu Ctrl+S:Save Ctrl+L:Layout Ctrl+G:Git Esc:Files");
        return;
    }
}

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
    /* Field text labels are positioned at x=14 inside the panel
     * (the "F:" / "R:" prefix occupies the first 14 px). */
    int cx = 14 + chars * cw;
    /* Cursor sits at the right edge of the active field's text.
     * Field "Find:" line is at y=20, "Replace:" line at y=36
     * inside the panel. */
    int cy = (s_search_field == 0) ? 20 : 36;
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
    case KB_KEY_LEFT:
        if (*apos > 0) {
            do { (*apos)--; }
            while (*apos > 0 && (abuf[*apos] & 0xC0) == 0x80);
        }
        break;
    case KB_KEY_RIGHT:
        if (*apos < (int)strlen(abuf)) {
            (*apos)++;
            while (*apos < (int)strlen(abuf) &&
                   (abuf[*apos] & 0xC0) == 0x80)
                (*apos)++;
        }
        break;
    case KB_KEY_HOME:
        *apos = 0;
        break;
    case KB_KEY_END:
        *apos = (int)strlen(abuf);
        break;
    case KB_KEY_BACKSPACE:
        if (*apos > 0) {
            int prev = *apos - 1;
            while (prev > 0 && (abuf[prev] & 0xC0) == 0x80) prev--;
            int len = (int)strlen(abuf);
            memmove(abuf + prev, abuf + *apos,
                    (size_t)(len - *apos + 1));
            *apos = prev;
            /* Editing the find query invalidates the previous match. */
            if (s_search_field == 0) {
                s_search_match_start = -1;
                s_search_match_end = -1;
            }
        }
        break;
    case KB_KEY_DELETE: {
        int len = (int)strlen(abuf);
        if (*apos < len) {
            int next = *apos + 1;
            while (next < len && (abuf[next] & 0xC0) == 0x80) next++;
            memmove(abuf + *apos, abuf + next,
                    (size_t)(len - next + 1));
            if (s_search_field == 0) {
                s_search_match_start = -1;
                s_search_match_end = -1;
            }
        }
        break;
    }
    default: {
        if (ctrl) break; /* swallow other ctrl combos */
        const char *text = kb_layout_translate(ev->keycode, ev->modifier);
        if (text && text[0]) {
            /* Reject control characters so the buffers stay
             * printable; embedded newlines are not supported in the
             * search dialog. */
            if ((unsigned char)text[0] < 0x20) break;
            size_t tlen = strlen(text);
            size_t cur_len = strlen(abuf);
            if (cur_len + tlen < bufsz - 1) {
                memmove(abuf + *apos + tlen,
                        abuf + *apos,
                        cur_len - (size_t)*apos + 1);
                memcpy(abuf + *apos, text, tlen);
                *apos += (int)tlen;
                if (s_search_field == 0) {
                    s_search_match_start = -1;
                    s_search_match_end = -1;
                }
            }
        }
        break;
    }
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
    int         line_chars;   /* UTF-8 char count of the line before */
    int         char_w;       /* monospace cell width at body font */
};

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
     * here. */
    md_line_info_t mi;
    md_parse_line(lt, ll, &mi, false);
    if (mi.type != MD_LINE_PARAGRAPH && mi.type != MD_LINE_EMPTY) return;

    int chars = utf8_chars_in_bytes(lt, ll);

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

    md_line_info_t mi;
    md_parse_line(lt, ll, &mi, false);
    if (mi.type != MD_LINE_PARAGRAPH && mi.type != MD_LINE_EMPTY) return false;

    int post_chars = utf8_chars_in_bytes(lt, ll);
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

    display_set_partial_clip(x_min, pre->cur_y, x_max - x_min, pre->cur_h);
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
 * The editor shows one pane by default. The split is driven entirely
 * by three shortcuts:
 *   Ctrl+1  single pane (full width)
 *   Ctrl+2  two equal-width panes (left = 1/2)
 *   Ctrl+3  two panes with the left = 2/3; pressing Ctrl+3 again
 *           flips the left pane to 1/3 (and back).
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

/* Ctrl+3: enter / cycle the asymmetric split. First press (from single
 * or equal split) makes the left pane 2/3; a subsequent Ctrl+3 toggles
 * the left pane between 2/3 and 1/3. */
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

static void handle_editor_key(const kb_event_t *ev)
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

    /* F1 opens the menu */
    if (ev->keycode == KB_KEY_F1) {
        show_menu();
        return;
    }

    /* Win+Space cycles the keyboard layout, mirroring Ctrl+L. HID
     * keycode 0x2C is Space; the GUI ("Win"/"Cmd") modifier is
     * KB_MOD_LGUI / KB_MOD_RGUI. */
    if ((ev->modifier & (KB_MOD_LGUI | KB_MOD_RGUI)) &&
        ev->keycode == KB_KEY_SPACE) {
        kb_layout_next();
        ensure_cursor_visible();
        editor_ui_refresh();
        return;
    }

    /* Ctrl shortcuts */
    if (ctrl) {
        /* Interpret Ctrl shortcuts by physical key position (HID keycode),
         * NOT by the current layout translation.  This ensures that
         * Ctrl+S, Ctrl+L, etc. work regardless of the active layout.
         * HID keycodes 0x04..0x1D map to a..z. */
        char ch = 0;
        if (ev->keycode >= 0x04 && ev->keycode <= 0x1D) {
            ch = 'a' + (ev->keycode - 0x04);
        }

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

        switch (ch) {
        case 's':
            show_save_prompt();
            break;
        case 'n':
            /* New empty document in the focused pane. In single-pane
             * mode this is the historical "replace current doc"
             * behavior; while split it only affects the focused pane. */
            if (s_pane_count <= 1) {
                editor_new_file();
                s_panes[0].doc = editor_get_active();
                s_panes[0].has_saved_view = false;   /* adopt the new doc's view */
            } else {
                open_into_pane(s_focus, NULL);
            }
            break;
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
            if (editor_copy())
                editor_ui_set_status("Copied");
            ensure_cursor_visible();
            editor_ui_refresh();
            return;
        case 'x':
            if (editor_cut())
                editor_ui_set_status("Cut");
            ensure_cursor_visible();
            editor_ui_refresh();
            return;
        case 'v':
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
            /* Auto-save every open document (both panes when split) so
             * the sync task picks up the latest edits (it reads from
             * disk). editor_doc_save() leaves the active doc untouched,
             * but rebind focus defensively. */
            editor_doc_foreach(save_modified_doc_cb, NULL);
            pane_bind_focus();
            if (git_sync_is_configured() && wifi_manager_is_connected()) {
                if (git_sync_start(GIT_SYNC_BOTH) == ESP_OK) {
                    editor_ui_set_status("Git: syncing...");
                } else {
                    char sbuf[128];
                    const char *err = git_sync_get_last_error();
                    snprintf(sbuf, sizeof(sbuf), "Git: %s",
                             (err && err[0]) ? err : "failed to start sync");
                    editor_ui_set_status(sbuf);
                }
            } else if (!wifi_manager_is_connected()) {
                editor_ui_set_status("Git: connect WiFi first (F1)");
            } else {
                editor_ui_set_status("Git: not configured");
            }
            break;
        case 'w':
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
            break;
        case 'm':
            /* Ctrl+M: menu, equivalent to F1 */
            show_menu();
            return;
#if defined(CONFIG_DRAFTLING_DISPLAY_HAS_BACKLIGHT)
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
            snprintf(sbuf, sizeof(sbuf), "Backlight: %d%%", s_backlight_pct);
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
        break;
    case KB_KEY_RIGHT:
        if (shift) { editor_set_selection_anchor(); editor_move_right(); }
        else if (editor_selection_active()) {
            size_t s, e; editor_get_selection_range(&s, &e);
            editor_clear_selection(); editor_set_cursor(e);
        } else { editor_move_right(); }
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
        editor_move_page_up(VISIBLE_LINES);
        break;
    case KB_KEY_PAGEDOWN:
        if (shift) editor_set_selection_anchor();
        else editor_clear_selection();
        editor_move_page_down(VISIBLE_LINES);
        break;
    case KB_KEY_BACKSPACE:
        content_changed = true;
        if (!editor_delete_selection()) {
            editor_delete_back();
#if defined(CONFIG_DRAFTLING_DISPLAY_EPD)
            fast_path_eligible = true;
#endif
        }
        break;
    case KB_KEY_DELETE:
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
        editor_delete_selection();
        if (!editor_insert_newline())
            editor_ui_set_status("Buffer full -- increase editor size in menuconfig");
        break;
    case KB_KEY_TAB:
        content_changed = true;
        editor_delete_selection();
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
            editor_delete_selection();
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
     * the last entry (and the caret / selection rects / logo, which are
     * raised above the list) would otherwise leave the previous
     * document's lines visible behind the selector. Hide the focused
     * pane's line labels, selection rects, cursor and logo explicitly. */
    {
        pane_t *save = s_rp;
        s_rp = &s_panes[s_focus];
        for (int i = 0; i < MAX_LINE_LABELS; i++) {
            if (s_line_labels[i]) lv_obj_add_flag(s_line_labels[i], LV_OBJ_FLAG_HIDDEN);
            if (s_sel_rects[i])   lv_obj_add_flag(s_sel_rects[i], LV_OBJ_FLAG_HIDDEN);
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
    editor_ui_set_status("Open into pane - Up/Down Enter  N:new  Esc:cancel");
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
     * every line label / selection rect and wipe its render cache so
     * the next editor_ui_refresh() repaints the pane from scratch with
     * no leftover lines from the previously shown file. Hiding the
     * widgets is required in addition to invalidating the cache: the
     * refresh path only hides a stale label when prev_line_visible[]
     * still marks it visible, and invalidate_render_cache() clears that
     * flag -- so without an explicit hide here a shorter new file would
     * leave the previous file's trailing lines on screen. */
    pane_t *save = s_rp;
    s_rp = &s_panes[s_focus];
    for (int i = 0; i < MAX_LINE_LABELS; i++) {
        if (s_line_labels[i]) lv_obj_add_flag(s_line_labels[i], LV_OBJ_FLAG_HIDDEN);
        if (s_sel_rects[i])   lv_obj_add_flag(s_sel_rects[i], LV_OBJ_FLAG_HIDDEN);
    }
    invalidate_render_cache();
    s_rp = save;
}

/* Open the file (or descend into the directory) selected by the
 * given row index in the file browser. Shared between the Enter-key
 * handler and the touchscreen tap-to-activate path. */
static void browser_activate_item(int row)
{
    lv_obj_t *btn = lv_obj_get_child(s_browser_list, row);
    if (!btn) return;
    int idx = (int)(intptr_t)lv_obj_get_user_data(btn);
    if (idx < 0 || idx >= s_browser_count) return;
    if (s_browser_entries[idx].is_dir) {
        /* Directory navigation is not implemented yet. */
        return;
    }

    char path[512];
    snprintf(path, sizeof(path), "%s/%s",
             sd_card_get_mount_point(), s_browser_entries[idx].name);

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
            return;
        }
        if (oerr != ESP_OK) {
            editor_ui_set_status("Open failed");
            return;
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
            editor_ui_set_status("Open failed");
            return;
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
}

static void handle_browser_key(const kb_event_t *ev)
{
    bool ctrl = (ev->modifier & (KB_MOD_LCTRL | KB_MOD_RCTRL)) != 0;

    /* F1 opens the menu from browser too */
    if (ev->keycode == KB_KEY_F1) {
        show_menu();
        return;
    }

    /* Ctrl+G / Ctrl+W work from the file browser as well */
    if (ctrl) {
        char ck = 0;
        if (ev->keycode >= 0x04 && ev->keycode <= 0x1D) {
            ck = 'a' + (ev->keycode - 0x04);
        }
        if (ck == 'g') {
            if (git_sync_is_configured() && wifi_manager_is_connected()) {
                if (git_sync_start(GIT_SYNC_BOTH) == ESP_OK) {
                    editor_ui_set_status("Git: syncing...");
                } else {
                    char sbuf[128];
                    const char *err = git_sync_get_last_error();
                    snprintf(sbuf, sizeof(sbuf), "Git: %s",
                             (err && err[0]) ? err : "failed to start sync");
                    editor_ui_set_status(sbuf);
                }
            } else if (!wifi_manager_is_connected()) {
                editor_ui_set_status("Git: connect WiFi first (F1)");
            } else {
                editor_ui_set_status("Git: not configured");
            }
            return;
        }
        if (ck == 'm') {
            /* Ctrl+M: menu, equivalent to F1 */
            show_menu();
            return;
        }
        if (ck == 'w') {
            if (!wifi_manager_is_connected()) {
                editor_ui_set_status("WiFi: connecting...");
                wifi_connect_async();
            } else {
                wifi_manager_disconnect();
                editor_ui_set_status("WiFi: disconnecting...");
            }
            return;
        }
#if defined(CONFIG_DRAFTLING_DISPLAY_HAS_BACKLIGHT)
        if (ck == 'b') {
            /* Ctrl+B: cycle to the next backlight / front-light
             * brightness step (same behaviour as in the editor). */
            int bi = find_backlight_option(s_backlight_pct);
            bi = (bi + 1) % BACKLIGHT_OPTION_COUNT;
            s_backlight_pct = BACKLIGHT_OPTIONS[bi];
            save_backlight_to_nvs();
            display_set_backlight(s_backlight_pct);
            char sbuf[32];
            snprintf(sbuf, sizeof(sbuf), "Backlight: %d%%", s_backlight_pct);
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

    /* Translate keycode to character for letter-key checks.
     * ev->character is always 0 because ble_keyboard does not fill
     * it -- all translation goes through kb_layout. */
    const char *br_t = kb_layout_translate(ev->keycode, ev->modifier);
    char ch = (br_t && br_t[0] && !br_t[1]) ? br_t[0] : 0;

    uint32_t child_count = lv_obj_get_child_count(s_browser_list);
    if (child_count == 0) {
        if (ch == 'n' || ch == 'N') {
            editor_new_file();
            editor_ui_show_editor();
        }
        return;
    }

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
        if (ch == 'n' || ch == 'N') {
            editor_new_file();
            editor_ui_show_editor();
            return;
        }
        break;
    }

    /* Highlight selected item -- restyle only the items whose state
     * changed (previous and current selection) so the LVGL dirty
     * region stays small enough for a partial e-paper refresh. */
    update_list_highlight(s_browser_list, s_browser_sel, s_browser_sel_prev);
    s_browser_sel_prev = s_browser_sel;
}

/* Key handler for the in-pane (split-mode) file selector. Esc cancels
 * the pick (the pane keeps its current document); F1 cancels and opens
 * the menu; N creates a new untitled document in the focused pane;
 * every other key (Up/Down/Enter, Ctrl+G/W/B) is delegated to the
 * shared browser handler, which now operates on the in-pane list and
 * loads the picked file into the focused pane via browser_activate_item
 * (split branch) -> open_into_pane(). */
static void handle_inpane_browser_key(const kb_event_t *ev)
{
    bool ctrl = (ev->modifier & (KB_MOD_LCTRL | KB_MOD_RCTRL)) != 0;

    if (ev->keycode == KB_KEY_ESCAPE) {
        close_inpane_browser();
        editor_ui_show_editor();
        return;
    }
    if (ev->keycode == KB_KEY_F1) {
        close_inpane_browser();
        show_menu();
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

    if (!ctrl) {
        const char *t = kb_layout_translate(ev->keycode, ev->modifier);
        char ch = (t && t[0] && !t[1]) ? t[0] : 0;
        if (ch == 'n' || ch == 'N') {
            /* New untitled document in the focused pane (not a global
             * replace -- that is the single-pane behavior). */
            if (!open_into_pane(s_focus, NULL)) {
                editor_ui_set_status("New failed");
                return;
            }
            s_open_target_pane = s_focus;
            close_inpane_browser();
            ensure_cursor_visible();
            editor_ui_show_editor();
            return;
        }
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

    /* Ctrl+X is a substitute for Escape on keyboards that have no
     * dedicated ESC key (e.g. some compact BLE keyboards). Translate
     * it into a synthetic Escape event so every screen handler that
     * already keys off KB_KEY_ESCAPE picks it up unchanged. The 'x'
     * HID usage id is 0x1B; match it with either Control modifier. */
    {
        bool ctrl = (norm.modifier & (KB_MOD_LCTRL | KB_MOD_RCTRL)) != 0;
        if (ctrl && norm.keycode == 0x1B) {
            norm.keycode  = KB_KEY_ESCAPE;
            norm.modifier = 0;
            norm.character = 0;
        }
    }

    const kb_event_t *e = &norm;

    if (s_save_open) {
        handle_save_prompt_key(e);
    } else if (s_exit_open) {
        handle_exit_prompt_key(e);
    } else if (s_search_open) {
        handle_search_prompt_key(e);
    } else if (s_settings_open) {
        handle_settings_key(e);
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

/* LVGL timer callback: drains the key-event queue in a batch.
 * This runs inside lv_timer_handler() which already holds the LVGL
 * mutex, so we must NOT call draftling_lvgl_port_lock() here. */
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
     * the keyboard has connected). See apply_pending_connect_state(). */
    apply_pending_connect_state();

    while (xQueueReceive(s_key_queue, &ev, 0) == pdTRUE) {
        process_key_event(&ev);
    }
}

/* BLE callback: enqueue the event and return immediately.
 * This function runs on the HID host task and must not touch LVGL. */
extern "C" void editor_ui_handle_key(const void *event)
{
    const kb_event_t *ev = (const kb_event_t *)event;

    if (ev->pressed) {
        /* Reset standby inactivity timer on key-down */
        standby_reset_timer();

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
    if (!s_repeat_held) return;

    uint32_t now = xTaskGetTickCount();
    uint32_t elapsed = (now - s_repeat_start) * portTICK_PERIOD_MS;

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

static void passkey_display_cb(uint32_t passkey)
{
    if (!draftling_lvgl_port_lock(100)) return;

    if (passkey == BLE_PASSKEY_DISMISS) {
        /* Hide the passkey overlay */
        if (s_passkey_panel) {
            lv_obj_add_flag(s_passkey_panel, LV_OBJ_FLAG_HIDDEN);
        }
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
        /* Show the passkey overlay with the 6-digit code */
        if (s_passkey_panel) {
            char buf[48];
            snprintf(buf, sizeof(buf), "Enter on keyboard:\n%06lu",
                     (unsigned long)passkey);
            lv_label_set_text(s_passkey_label, buf);
            lv_obj_remove_flag(s_passkey_panel, LV_OBJ_FLAG_HIDDEN);
        }
    }

    draftling_lvgl_port_unlock();
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
        s_settings_open = false;
#if defined(CONFIG_DRAFTLING_DISPLAY_COLOR)
        s_theme_picker_open = false;
#endif
        s_save_open = false;
        if (s_save_panel) lv_obj_add_flag(s_save_panel, LV_OBJ_FLAG_HIDDEN);
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

/* BLE status text callback -- update the BLE prompt label with
 * connection progress messages from the BLE keyboard component. */
static void ble_status_text_cb(const char *text)
{
    if (!s_ble_prompt_lbl) return;
    if (!draftling_lvgl_port_lock(200)) return;

    /* Only update when the BLE prompt screen is active */
    if (lv_scr_act() == s_scr_ble_prompt) {
        lv_label_set_text(s_ble_prompt_lbl, text);
    }

    draftling_lvgl_port_unlock();
}

/* ---- WiFi connect task ----
 * wifi_manager_connect() blocks for up to 30 seconds while waiting for
 * the connection.  Running it on a separate task keeps the LVGL render
 * loop responsive so the user sees status updates in real time. */
static void wifi_connect_task(void *arg)
{
    (void)arg;
    esp_err_t ret = wifi_manager_connect();
    if (ret == ESP_ERR_NOT_FOUND) {
        /* No credentials -- the wifi_state callback will not fire,
         * so update the status bar directly. */
        if (draftling_lvgl_port_lock(200)) {
            editor_ui_set_status("WiFi: no credentials found");
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
    }
}

/* ---- WiFi state callback ----
 * Called from the WiFi manager (event handler context) when the
 * connection state changes.  Must take the LVGL lock before touching
 * any UI objects. */
static void wifi_state_cb(wifi_state_t state)
{
    if (!draftling_lvgl_port_lock(200)) return;

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
            snprintf(buf, sizeof(buf), "WiFi: %s (%s)",
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
                snprintf(buf, sizeof(buf), "WiFi: connecting to %s", ssid);
            } else {
                snprintf(buf, sizeof(buf), "WiFi: connecting...");
            }
            editor_ui_set_status(buf);
            break;
        }
        case WIFI_STATE_ERROR:
            editor_ui_set_status("WiFi: connection failed");
            break;
        case WIFI_STATE_DISCONNECTED:
            editor_ui_set_status("WiFi: disconnected");
            break;
        default:
            break;
        }
    }

    update_wifi_icons();
    draftling_lvgl_port_unlock();
}

/* ---- Git sync callback ----
 * Called from the git_sync task when the sync state changes.
 * Must take the LVGL lock before touching any UI objects. */
static void git_sync_cb(git_sync_state_t state, const char *message)
{
    if (!draftling_lvgl_port_lock(200)) return;

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
        if (s_editor_screen_active && editor_get_file_path()) {
            const char *path = editor_get_file_path();
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
    lv_obj_set_pos(s_lbl_title, 2, 0);
    lv_obj_set_width(s_lbl_title, SCR_W - 4);
    lv_obj_set_style_text_font(s_lbl_title, FONT_11, 0);
    lv_obj_set_style_text_color(s_lbl_title, theme_fg(), 0);
    lv_label_set_text(s_lbl_title, "Draftling");

    /* Header separator line */
    lv_obj_t *hline = lv_obj_create(s_scr);
    lv_obj_set_size(hline, SCR_W, 1);
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
        lv_obj_set_size(s_cont_edit, SCR_W, EDITOR_H);
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
        lv_obj_set_width(s_img_logo, SCR_W);
        lv_obj_set_style_text_font(s_img_logo, FONT_18, 0);
        lv_obj_set_style_text_color(s_img_logo, lv_color_make(0x80, 0x80, 0x80), 0);
        lv_obj_set_style_text_align(s_img_logo, LV_TEXT_ALIGN_CENTER, 0);
        lv_label_set_text(s_img_logo, "draftling");
        lv_obj_set_pos(s_img_logo, 0,
                       (EDITOR_H - lv_font_get_line_height(FONT_18)) / 2);
        lv_obj_add_flag(s_img_logo, LV_OBJ_FLAG_HIDDEN);

        /* Selection overlay labels (created before cursor and line
         * labels so their initial z-order is behind text;
         * partial-selection code calls lv_obj_move_foreground() to
         * bring them on top). These display the selected text in white
         * on a black background for proper inversion on the monochrome
         * e-paper display. */
        for (int i = 0; i < MAX_LINE_LABELS; i++) {
            s_sel_rects[i] = lv_label_create(s_cont_edit);
            lv_obj_set_style_bg_color(s_sel_rects[i],
                                      theme_fg(), 0);
            lv_obj_set_style_bg_opa(s_sel_rects[i], LV_OPA_COVER, 0);
            lv_obj_set_style_text_color(s_sel_rects[i],
                                        theme_bg(), 0);
            lv_obj_set_style_border_width(s_sel_rects[i], 0, 0);
            lv_obj_set_style_radius(s_sel_rects[i], 0, 0);
            lv_obj_set_style_pad_all(s_sel_rects[i], 0, 0);
            lv_label_set_text(s_sel_rects[i], "");
            lv_obj_add_flag(s_sel_rects[i], LV_OBJ_FLAG_HIDDEN);
        }

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

    /* Vertical divider drawn between the two panes when split. */
    s_pane_divider = lv_obj_create(s_scr);
    lv_obj_set_size(s_pane_divider, 1, EDITOR_H);
    lv_obj_set_pos(s_pane_divider, SCR_W / 2, EDITOR_Y);
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
    lv_obj_set_size(sline, SCR_W, 1);
    lv_obj_set_pos(sline, 0, SCR_H - STATUS_H);
    lv_obj_set_style_bg_color(sline, theme_fg(), 0);
    lv_obj_set_style_bg_opa(sline, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sline, 0, 0);
    lv_obj_set_style_radius(sline, 0, 0);
    lv_obj_set_style_pad_all(sline, 0, 0);

    s_lbl_status = lv_label_create(s_scr);
    lv_obj_set_pos(s_lbl_status, 2, SCR_H - STATUS_H + 2);
    /* Width is constrained below (see status_avail_width()) so the
     * label never reaches the battery / wifi icon on the right and
     * never wraps to a second line (which would extend past SCR_H
     * and trigger LVGL's screen scrollbar on narrow color LCDs). */
    lv_label_set_long_mode(s_lbl_status, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(s_lbl_status, status_avail_width());
    lv_obj_set_style_text_font(s_lbl_status, FONT_11, 0);
    lv_obj_set_style_text_color(s_lbl_status, theme_fg(), 0);
    lv_label_set_text(s_lbl_status, EDITOR_DEFAULT_STATUS);
    update_status_visibility();

#if defined(CONFIG_DRAFTLING_HAS_BATTERY)
    /* Device battery label (right-aligned in editor status bar) */
    s_lbl_dev_batt = lv_label_create(s_scr);
    lv_obj_set_style_text_font(s_lbl_dev_batt, FONT_11, 0);
    lv_obj_set_style_text_color(s_lbl_dev_batt, theme_fg(), 0);
    lv_obj_set_style_text_align(s_lbl_dev_batt, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_pos(s_lbl_dev_batt, SCR_W - 80, SCR_H - STATUS_H + 2);
    lv_obj_set_width(s_lbl_dev_batt, 78);
    lv_label_set_text(s_lbl_dev_batt, "");
#define WIFI_ICON_RIGHT_OFFSET 95   /* leave room for battery on its left */
#else
#define WIFI_ICON_RIGHT_OFFSET 15
#endif

    /* WiFi connectivity icon (right corner of editor status bar).
     * Pick the foreground variant that contrasts with the active
     * background: the EPD inverted theme and every color-LCD theme
     * use a dark background, so the white-on-transparent variant
     * applies. Other (default mono) builds use the black variant. */
#if defined(CONFIG_DRAFTLING_EPD_BLACK_BACKGROUND) || \
    defined(CONFIG_DRAFTLING_DISPLAY_COLOR)
    s_img_wifi = lv_image_create(s_scr);
    lv_image_set_src(s_img_wifi, &wifi_icon_white);
#else
    s_img_wifi = lv_image_create(s_scr);
    lv_image_set_src(s_img_wifi, &wifi_icon_black);
#endif
    lv_obj_set_pos(s_img_wifi,
                   SCR_W - WIFI_ICON_RIGHT_OFFSET,
                   SCR_H - STATUS_H + (STATUS_H - 7) / 2);
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
    lv_obj_set_pos(br_title, 2, 0);
    lv_obj_set_style_text_font(br_title, FONT_11, 0);
    lv_obj_set_style_text_color(br_title, theme_fg(), 0);
    lv_label_set_text(br_title, "File Browser - Up/Down, Enter to open, N for new");

    s_list_files = lv_list_create(s_scr_browser);
    lv_obj_set_pos(s_list_files, 0, 18);
    lv_obj_set_size(s_list_files, SCR_W, LIST_PANEL_H - STATUS_H);
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
    lv_obj_set_size(br_sline, SCR_W, 1);
    lv_obj_set_pos(br_sline, 0, SCR_H - STATUS_H);
    lv_obj_set_style_bg_color(br_sline, theme_fg(), 0);
    lv_obj_set_style_bg_opa(br_sline, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(br_sline, 0, 0);
    lv_obj_set_style_radius(br_sline, 0, 0);
    lv_obj_set_style_pad_all(br_sline, 0, 0);

    s_lbl_br_status = lv_label_create(s_scr_browser);
    lv_obj_set_pos(s_lbl_br_status, 2, SCR_H - STATUS_H + 2);
    lv_obj_set_width(s_lbl_br_status, SCR_W - 4);
    lv_obj_set_style_text_font(s_lbl_br_status, FONT_11, 0);
    lv_obj_set_style_text_color(s_lbl_br_status, theme_fg(), 0);
    lv_label_set_text(s_lbl_br_status, "F1:Menu  N:New  Ctrl+G:Git  Ctrl+W:WiFi");

#if defined(CONFIG_DRAFTLING_HAS_BATTERY)
    /* Device battery label (right-aligned in browser status bar) */
    s_lbl_br_dev_batt = lv_label_create(s_scr_browser);
    lv_obj_set_style_text_font(s_lbl_br_dev_batt, FONT_11, 0);
    lv_obj_set_style_text_color(s_lbl_br_dev_batt, theme_fg(), 0);
    lv_obj_set_style_text_align(s_lbl_br_dev_batt, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_pos(s_lbl_br_dev_batt, SCR_W - 80, SCR_H - STATUS_H + 2);
    lv_obj_set_width(s_lbl_br_dev_batt, 78);
    lv_label_set_text(s_lbl_br_dev_batt, "");

#if defined(DRAFTLING_BATT_PULL_MODE)
    /* H752: no periodic timer (see the declarations above); just paint
     * the initial value. Subsequent updates ride redraw points. */
    sync_battery_labels();
#else
    /* Battery poll timer + first reading */
    s_batt_timer = lv_timer_create(batt_timer_cb, BATT_POLL_MS, NULL);
    batt_timer_cb(NULL);  /* show initial value immediately */
#endif
#define WIFI_ICON_RIGHT_OFFSET 95
#else
#define WIFI_ICON_RIGHT_OFFSET 15
#endif

    /* WiFi connectivity icon (right corner of browser status bar) */
#if defined(CONFIG_DRAFTLING_EPD_BLACK_BACKGROUND) || \
    defined(CONFIG_DRAFTLING_DISPLAY_COLOR)
    s_img_br_wifi = lv_image_create(s_scr_browser);
    lv_image_set_src(s_img_br_wifi, &wifi_icon_white);
#else
    s_img_br_wifi = lv_image_create(s_scr_browser);
    lv_image_set_src(s_img_br_wifi, &wifi_icon_black);
#endif
    lv_obj_set_pos(s_img_br_wifi,
                   SCR_W - WIFI_ICON_RIGHT_OFFSET,
                   SCR_H - STATUS_H + (STATUS_H - 7) / 2);
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
     * Populated by batt_timer_cb() which is created further down in
     * the file-browser init block. */
    s_lbl_ble_dev_batt = lv_label_create(s_scr_ble_prompt);
    lv_obj_set_style_text_font(s_lbl_ble_dev_batt, FONT_11, 0);
    lv_obj_set_style_text_color(s_lbl_ble_dev_batt, theme_fg(), 0);
    lv_obj_set_style_text_align(s_lbl_ble_dev_batt, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_pos(s_lbl_ble_dev_batt, SCR_W - 80, SCR_H - STATUS_H + 2);
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
    {
        lv_obj_t *off_btn = lv_button_create(s_scr_ble_prompt);
        lv_obj_set_size(off_btn, 40, 24);
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
        lv_obj_set_style_text_font(off_lbl, FONT_11, 0);
        lv_obj_set_style_text_color(off_lbl, theme_fg(), 0);
        lv_label_set_text(off_lbl, "Off");
        lv_obj_center(off_lbl);
    }
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
    lv_obj_set_pos(s_lbl_menu_hdr, 2, 0);
    lv_obj_set_style_text_font(s_lbl_menu_hdr, FONT_11, 0);
    lv_obj_set_style_text_color(s_lbl_menu_hdr, theme_fg(), 0);
    lv_label_set_text(s_lbl_menu_hdr,
                      "Menu - Up/Down, Enter to select, Esc to close");

    s_menu_list = lv_list_create(s_scr_menu);
    lv_obj_set_pos(s_menu_list, 0, 18);
    lv_obj_set_size(s_menu_list, SCR_W, LIST_PANEL_H);
    lv_obj_set_style_border_width(s_menu_list, 0, 0);
    lv_obj_set_style_radius(s_menu_list, 0, 0);
    lv_obj_set_style_pad_all(s_menu_list, 0, 0);
    lv_obj_set_style_text_font(s_menu_list, FONT_14, 0);
    /* See note on s_list_files above: override the LVGL default white
     * list background so the menu blends with the screen in
     * CONFIG_DRAFTLING_EPD_BLACK_BACKGROUND mode. */
    lv_obj_set_style_bg_color(s_menu_list, theme_bg(), 0);

    /* ---- Settings screen ---- */
    s_scr_settings = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_scr_settings, theme_bg(), 0);

    lv_obj_t *set_hdr = lv_label_create(s_scr_settings);
    lv_obj_set_pos(set_hdr, 2, 0);
    lv_obj_set_style_text_font(set_hdr, FONT_11, 0);
    lv_obj_set_style_text_color(set_hdr, theme_fg(), 0);
    lv_label_set_text(set_hdr,
                      "Settings - Up/Down, Enter to change, Esc to go back");

    s_settings_list = lv_list_create(s_scr_settings);
    lv_obj_set_pos(s_settings_list, 0, 18);
    lv_obj_set_size(s_settings_list, SCR_W, LIST_PANEL_H);
    lv_obj_set_style_border_width(s_settings_list, 0, 0);
    lv_obj_set_style_radius(s_settings_list, 0, 0);
    lv_obj_set_style_pad_all(s_settings_list, 0, 0);
    lv_obj_set_style_text_font(s_settings_list, FONT_14, 0);
    /* See note on s_list_files above. */
    lv_obj_set_style_bg_color(s_settings_list, theme_bg(), 0);

    /* ---- Passkey overlay (shown on the editor screen) ---- */
    s_passkey_panel = lv_obj_create(s_scr);
    lv_obj_set_size(s_passkey_panel, SCR_W - 20, 60);
    lv_obj_set_pos(s_passkey_panel,
                   10, (SCR_H - 60) / 2);
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
    lv_obj_set_width(s_passkey_label, SCR_W - 20 - 12);
    lv_obj_center(s_passkey_label);
    lv_label_set_text(s_passkey_label, "");

    /* Register passkey display callback */
    ble_keyboard_set_passkey_callback(passkey_display_cb);

    /* ---- Save-prompt overlay (shown on the editor screen) ---- */
    s_save_panel = lv_obj_create(s_scr);
    lv_obj_set_size(s_save_panel, SCR_W - 20, 46);
    lv_obj_set_pos(s_save_panel, 10, (SCR_H - 46) / 2);
    lv_obj_set_style_bg_color(s_save_panel, theme_bg(), 0);
    lv_obj_set_style_bg_opa(s_save_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(s_save_panel, theme_fg(), 0);
    lv_obj_set_style_border_width(s_save_panel, 2, 0);
    lv_obj_set_style_radius(s_save_panel, 4, 0);
    lv_obj_set_style_pad_all(s_save_panel, 6, 0);
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
    lv_obj_set_pos(s_save_name_lbl, 0, 20);

    /* Thin cursor bar inside the save prompt name field */
    s_save_cur = lv_obj_create(s_save_panel);
    lv_obj_set_size(s_save_cur, 2, LINE_H);
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
        int panel_h = 20 + rows * (LINE_H + 2) + 12;
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
            lv_obj_set_pos(s_exit_opt_lbl[i], 0, 20 + i * (LINE_H + 2));
        }
    }

    /* ---- Search / Replace overlay (shown on the editor screen) ---- */
    s_search_panel = lv_obj_create(s_scr);
    lv_obj_set_size(s_search_panel, SCR_W - 20, 76);
    lv_obj_set_pos(s_search_panel, 10, (SCR_H - 76) / 2);
    lv_obj_set_style_bg_color(s_search_panel, theme_bg(), 0);
    lv_obj_set_style_bg_opa(s_search_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(s_search_panel, theme_fg(), 0);
    lv_obj_set_style_border_width(s_search_panel, 2, 0);
    lv_obj_set_style_radius(s_search_panel, 4, 0);
    lv_obj_set_style_pad_all(s_search_panel, 6, 0);
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
    lv_obj_set_pos(s_search_find_hdr, 0, 20);

    s_search_find_lbl = lv_label_create(s_search_panel);
    lv_obj_set_style_text_font(s_search_find_lbl, FONT_11, 0);
    lv_obj_set_style_text_color(s_search_find_lbl, theme_fg(), 0);
    lv_obj_set_width(s_search_find_lbl, SCR_W - 20 - 12 - 14);
    lv_label_set_text(s_search_find_lbl, "");
    lv_obj_set_pos(s_search_find_lbl, 14, 20);

    /* Replace row (only shown in replace mode) */
    s_search_repl_hdr = lv_label_create(s_search_panel);
    lv_obj_set_style_text_font(s_search_repl_hdr, FONT_11, 0);
    lv_obj_set_style_text_color(s_search_repl_hdr, theme_fg(), 0);
    lv_label_set_text(s_search_repl_hdr, "R:");
    lv_obj_set_pos(s_search_repl_hdr, 0, 36);
    lv_obj_add_flag(s_search_repl_hdr, LV_OBJ_FLAG_HIDDEN);

    s_search_repl_lbl = lv_label_create(s_search_panel);
    lv_obj_set_style_text_font(s_search_repl_lbl, FONT_11, 0);
    lv_obj_set_style_text_color(s_search_repl_lbl, theme_fg(), 0);
    lv_obj_set_width(s_search_repl_lbl, SCR_W - 20 - 12 - 14);
    lv_label_set_text(s_search_repl_lbl, "");
    lv_obj_set_pos(s_search_repl_lbl, 14, 36);
    lv_obj_add_flag(s_search_repl_lbl, LV_OBJ_FLAG_HIDDEN);

    /* Help line at the bottom of the panel */
    s_search_help_lbl = lv_label_create(s_search_panel);
    lv_obj_set_style_text_font(s_search_help_lbl, FONT_11, 0);
    lv_obj_set_style_text_color(s_search_help_lbl, theme_fg(), 0);
    lv_obj_set_width(s_search_help_lbl, SCR_W - 20 - 12);
    lv_label_set_text(s_search_help_lbl, "Enter:Next  Esc:Close");
    lv_obj_set_pos(s_search_help_lbl, 0, 54);

    /* Cursor bar shared between the find / replace fields. The
     * refresh function repositions it to whichever row owns focus. */
    s_search_cur = lv_obj_create(s_search_panel);
    lv_obj_set_size(s_search_cur, 2, LINE_H);
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

    sync_battery_labels();

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
     * bars, line labels, selection rects, the passkey / save /
     * search overlays parented to s_scr, the wifi icons, etc.) are
     * deleted automatically with their parent. */
    if (s_scr_browser)    { lv_obj_delete(s_scr_browser);    s_scr_browser    = NULL; }
    if (s_scr_menu)       { lv_obj_delete(s_scr_menu);       s_scr_menu       = NULL; }
    if (s_scr_settings)   { lv_obj_delete(s_scr_settings);   s_scr_settings   = NULL; }
    if (s_scr_ble_prompt) { lv_obj_delete(s_scr_ble_prompt); s_scr_ble_prompt = NULL; }
    if (s_scr)            { lv_obj_delete(s_scr);            s_scr            = NULL; }

    /* NULL every child-widget pointer so any stale reference
     * crashes deterministically instead of touching freed memory.
     * The per-pane widgets (container, cursor, logo, line / selection
     * label pools) are cleared for every pane. */
    s_lbl_title = s_lbl_status = NULL;
    s_img_wifi = s_img_br_wifi = NULL;
    s_pane_divider = NULL;
    {
        pane_t *save_rp = s_rp;
        for (int p = 0; p < EDITOR_MAX_PANES; p++) {
            s_rp = &s_panes[p];
            s_cont_edit = s_cursor = s_img_logo = NULL;
            for (int i = 0; i < MAX_LINE_LABELS; i++) {
                s_line_labels[i] = NULL;
                s_sel_rects[i]   = NULL;
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
    s_settings_list = NULL;
    s_ble_prompt_lbl = NULL;
    s_passkey_panel = s_passkey_label = NULL;
    s_save_panel = s_save_hdr_lbl = s_save_name_lbl = s_save_cur = NULL;
    s_exit_panel = s_exit_hdr_lbl = NULL;
    s_exit_opt_lbl[0] = s_exit_opt_lbl[1] = s_exit_opt_lbl[2] = NULL;
    s_search_panel = s_search_hdr_lbl = NULL;
    s_search_find_hdr = s_search_find_lbl = NULL;
    s_search_repl_hdr = s_search_repl_lbl = NULL;
    s_search_cur = s_search_help_lbl = NULL;

    /* Reset every "open" / transient flag so a fresh build starts
     * from a clean state. The persistent document, NVS-backed font
     * size / theme / backlight / standby timeout are NOT touched. */
    s_menu_open               = false;
    s_settings_open           = false;
#if defined(CONFIG_DRAFTLING_DISPLAY_COLOR)
    s_theme_picker_open       = false;
#endif
    s_factory_reset_confirm   = false;
    s_save_open               = false;
    s_exit_open               = false;
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
        show_settings();    /* refreshes items + loads s_scr_settings */
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

extern "C" void editor_ui_init(void)
{
    /* Chain optional Greybeard font subsets (Cyrillic / Hebrew) into
     * the base font's fallback slot before any text is rendered. */
    greybeard_init();

    load_font_size_from_nvs();
#if defined(CONFIG_DRAFTLING_DISPLAY_COLOR)
    load_theme_from_nvs();
#endif
#if defined(CONFIG_DRAFTLING_DISPLAY_HAS_BACKLIGHT)
    load_backlight_from_nvs();
    display_set_backlight(s_backlight_pct);
#endif
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
