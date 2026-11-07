/*
 * Upgrade progress screen (see ui.h).
 */
#include "ui.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "lvgl.h"
#include "board.h"

static const char *TAG = "ui";

#define DETAIL_LINES  4
#define LOG_LINES     14
#define LOG_LINE_LEN  96

static lv_obj_t *s_phase;
static lv_obj_t *s_detail[DETAIL_LINES];
static lv_obj_t *s_bar;
static lv_obj_t *s_bar_label;
static lv_obj_t *s_log;
static lv_obj_t *s_btn_row;
static lv_obj_t *s_btn_yes;
static lv_obj_t *s_btn_no;
static lv_obj_t *s_btn_yes_label;
static lv_obj_t *s_btn_no_label;
static EventGroupHandle_t s_btn_evt;

static char s_log_lines[LOG_LINES][LOG_LINE_LEN];
static int s_log_count;

#define BTN_YES_BIT (1u << 0)
#define BTN_NO_BIT  (1u << 1)

static void btn_cb(lv_event_t *e)
{
    uintptr_t bit = (uintptr_t)lv_event_get_user_data(e);
    xEventGroupSetBits(s_btn_evt, (EventBits_t)bit);
}

static lv_obj_t *make_button(lv_obj_t *parent, lv_obj_t **label, uintptr_t bit)
{
    lv_obj_t *b = lv_button_create(parent);
    lv_obj_set_size(b, 300, 110);
    lv_obj_add_event_cb(b, btn_cb, LV_EVENT_CLICKED, (void *)bit);
    *label = lv_label_create(b);
    lv_obj_set_style_text_font(*label, &lv_font_montserrat_28, 0);
    lv_obj_center(*label);
    return b;
}

void ui_init(const char *title)
{
    s_btn_evt = xEventGroupCreate();

    board_display_start();

    board_display_lock();
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x101820), 0);
    lv_obj_set_style_text_color(scr, lv_color_hex(0xE8E8E8), 0);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(scr, 30, 0);
    lv_obj_set_style_pad_row(scr, 18, 0);

    lv_obj_t *t = lv_label_create(scr);
    lv_obj_set_style_text_font(t, &lv_font_montserrat_40, 0);
    lv_label_set_text(t, title);

    s_phase = lv_label_create(scr);
    lv_obj_set_width(s_phase, LV_PCT(100));
    lv_label_set_long_mode(s_phase, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(s_phase, &lv_font_montserrat_32, 0);
    lv_label_set_text(s_phase, "");

    for (int i = 0; i < DETAIL_LINES; i++) {
        s_detail[i] = lv_label_create(scr);
        lv_obj_set_width(s_detail[i], LV_PCT(100));
        lv_label_set_long_mode(s_detail[i], LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_font(s_detail[i], &lv_font_montserrat_24, 0);
        lv_label_set_text(s_detail[i], "");
    }

    s_bar = lv_bar_create(scr);
    lv_obj_set_size(s_bar, LV_PCT(100), 50);
    lv_bar_set_range(s_bar, 0, 100);
    s_bar_label = lv_label_create(s_bar);
    lv_obj_set_style_text_font(s_bar_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(s_bar_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(s_bar_label);
    lv_obj_set_hidden(s_bar, true);

    s_btn_row = lv_obj_create(scr);
    lv_obj_set_size(s_btn_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(s_btn_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_btn_row, 0, 0);
    lv_obj_set_flex_flow(s_btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(s_btn_row, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    s_btn_yes = make_button(s_btn_row, &s_btn_yes_label, BTN_YES_BIT);
    s_btn_no = make_button(s_btn_row, &s_btn_no_label, BTN_NO_BIT);
    lv_obj_set_hidden(s_btn_row, true);

    s_log = lv_label_create(scr);
    lv_obj_set_width(s_log, LV_PCT(100));
    lv_obj_set_flex_grow(s_log, 1);
    lv_label_set_long_mode(s_log, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(s_log, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_log, lv_color_hex(0x9FB0C0), 0);
    lv_label_set_text(s_log, "");
    board_display_unlock();
}

static void set_label_v(lv_obj_t *label, const char *fmt, va_list ap)
{
    char buf[192];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    board_display_lock();
    lv_label_set_text(label, buf);
    board_display_unlock();
}

void ui_phase(ui_state_t state, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char buf[192];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    ESP_LOGI(TAG, "== %s", buf);

    uint32_t color = state == UI_STATE_OK ? 0x5CE07A :
                     state == UI_STATE_ERROR ? 0xFF6060 : 0xFFD24A;
    board_display_lock();
    lv_obj_set_style_text_color(s_phase, lv_color_hex(color), 0);
    lv_label_set_text(s_phase, buf);
    board_display_unlock();
}

void ui_detail(int line, const char *fmt, ...)
{
    if (line < 0 || line >= DETAIL_LINES) return;
    va_list ap;
    va_start(ap, fmt);
    set_label_v(s_detail[line], fmt, ap);
    va_end(ap);
}

void ui_progress(int percent)
{
    board_display_lock();
    if (percent < 0) {
        lv_obj_set_hidden(s_bar, true);
    } else {
        if (percent > 100) percent = 100;
        lv_obj_set_hidden(s_bar, false);
        lv_bar_set_value(s_bar, percent, LV_ANIM_OFF);
        lv_label_set_text_fmt(s_bar_label, "%d %%", percent);
    }
    board_display_unlock();
}

void ui_log(const char *fmt, ...)
{
    char buf[LOG_LINE_LEN];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    ESP_LOGI(TAG, "%s", buf);

    if (s_log_count == LOG_LINES) {
        memmove(s_log_lines[0], s_log_lines[1], sizeof(s_log_lines[0]) * (LOG_LINES - 1));
        s_log_count--;
    }
    strlcpy(s_log_lines[s_log_count++], buf, LOG_LINE_LEN);

    static char text[LOG_LINES * LOG_LINE_LEN];
    text[0] = '\0';
    for (int i = 0; i < s_log_count; i++) {
        strlcat(text, s_log_lines[i], sizeof(text));
        if (i + 1 < s_log_count) strlcat(text, "\n", sizeof(text));
    }
    board_display_lock();
    lv_label_set_text(s_log, text);
    board_display_unlock();
}

bool ui_ask(const char *yes, const char *no, uint32_t timeout_ms, bool dflt)
{
    xEventGroupClearBits(s_btn_evt, BTN_YES_BIT | BTN_NO_BIT);
    board_display_lock();
    lv_label_set_text(s_btn_yes_label, yes);
    lv_label_set_text(s_btn_no_label, no);
    lv_obj_set_hidden(s_btn_row, false);
    board_display_unlock();

    EventBits_t bits = xEventGroupWaitBits(
        s_btn_evt, BTN_YES_BIT | BTN_NO_BIT, pdTRUE, pdFALSE,
        timeout_ms ? pdMS_TO_TICKS(timeout_ms) : portMAX_DELAY);

    board_display_lock();
    lv_obj_set_hidden(s_btn_row, true);
    board_display_unlock();

    if (bits & BTN_YES_BIT) return true;
    if (bits & BTN_NO_BIT) return false;
    return dflt;
}
