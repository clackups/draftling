/*
 * GPIO key matrix scanner and layer / modifier engine; see matrix_kbd.h.
 *
 * A task scans the matrix every SCAN_PERIOD_MS: each column is driven
 * low in turn (open-drain, so an idle column floats instead of fighting
 * a pressed key's diode) and the rows are read with their internal
 * pull-ups. A key's new state is accepted once two scans in a row agree
 * (debounce). The action of a key is resolved when it is pressed and
 * kept until it is released, so a release always matches its press even
 * when the layer or Shift changed in between.
 */

#include "matrix_kbd.h"

#include <string.h>

#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"

static const char *TAG = "matrix_kbd";

#define SCAN_PERIOD_MS  5
#define SETTLE_US       5
#define MAX_KEYS        (MATRIX_KBD_MAX_ROWS * MATRIX_KBD_MAX_COLS)

#define KIND(a)  ((uint16_t)(a) & 0xFF00)
#define ARG(a)   ((uint8_t)((a) & 0xFF))
#define SHIFT_MODS (KB_MOD_LSHIFT | KB_MOD_RSHIFT)

static const matrix_kbd_config_t *s_cfg;
static kb_event_callback_t s_cb;
static TaskHandle_t s_task;
static volatile bool s_stop;
static volatile bool s_stopped;
static bool s_present;

static bool     s_stable[MAX_KEYS];   /* debounced state */
static bool     s_last_raw[MAX_KEYS]; /* previous scan */
static uint16_t s_action[MAX_KEYS];   /* action resolved at press */
static uint8_t  s_key_mods[MAX_KEYS]; /* modifiers sent with the press */
static uint8_t  s_osm_pending;        /* one-shot modifiers armed */

static int nkeys(void) { return s_cfg->nrows * s_cfg->ncols; }

/* Modifiers / layer of the keys held now. */
static void held_state(uint8_t *mods, uint8_t *osm_held, uint8_t *layer)
{
    *mods = 0;
    *osm_held = 0;
    *layer = 0;
    for (int k = 0; k < nkeys(); k++) {
        if (!s_stable[k]) continue;
        uint16_t a = s_action[k];
        switch (KIND(a)) {
        case 0x0300: *mods |= ARG(a); break;
        case 0x0400: *osm_held |= ARG(a); break;
        case 0x0500: if (ARG(a) > *layer) *layer = ARG(a); break;
        default: break;
        }
    }
}

static uint16_t resolve(int k, uint8_t mods, uint8_t layer)
{
    const int n = nkeys();
    if ((mods & SHIFT_MODS) && s_cfg->shift_layer &&
        s_cfg->shift_layer[k] != MKBD_TRANS) {
        return s_cfg->shift_layer[k];
    }
    if (layer >= s_cfg->nlayers) layer = 0;
    uint16_t a = s_cfg->layers[layer * n + k];
    if (a == MKBD_TRANS) a = s_cfg->layers[k];
    return a;
}

static void emit(uint8_t keycode, uint8_t mods, bool pressed)
{
    kb_event_callback_t cb = s_cb;
    if (!cb) return;
    kb_event_t ev = {};
    ev.modifier = mods;
    ev.keycode = keycode;
    ev.character = 0;   /* the editor maps keycodes through the layout */
    ev.pressed = pressed;
    cb(&ev);
}

static void key_changed(int k, bool pressed)
{
    uint8_t mods, osm_held, layer;
    if (pressed) {
        held_state(&mods, &osm_held, &layer);
        uint16_t a = resolve(k, mods | osm_held, layer);
        s_action[k] = a;
        s_stable[k] = true;
        switch (KIND(a)) {
        case 0x0100:
        case 0x0200: {
            uint8_t m = mods | osm_held | s_osm_pending;
            if (KIND(a) == 0x0200) m &= ~SHIFT_MODS;
            s_osm_pending = 0;
            s_key_mods[k] = m;
            emit(ARG(a), m, true);
            break;
        }
        case 0x0400:
            /* Tapping it again before the next key cancels it. */
            s_osm_pending ^= ARG(a);
            break;
        default:
            break;
        }
    } else {
        uint16_t a = s_action[k];
        s_stable[k] = false;
        s_action[k] = MKBD_NONE;
        if (KIND(a) == 0x0100 || KIND(a) == 0x0200) {
            emit(ARG(a), s_key_mods[k], false);
        }
    }
}

static void scan(bool raw[])
{
    for (int c = 0; c < s_cfg->ncols; c++) {
        gpio_num_t col = (gpio_num_t)s_cfg->col_pins[c];
        gpio_set_level(col, 0);
        esp_rom_delay_us(SETTLE_US);
        for (int r = 0; r < s_cfg->nrows; r++) {
            raw[r * s_cfg->ncols + c] =
                gpio_get_level((gpio_num_t)s_cfg->row_pins[r]) == 0;
        }
        gpio_set_level(col, 1);
    }
}

static void scan_task(void *arg)
{
    (void)arg;
    bool raw[MAX_KEYS];
    while (!s_stop) {
        scan(raw);
        for (int k = 0; k < nkeys(); k++) {
            if (raw[k] == s_last_raw[k] && raw[k] != s_stable[k]) {
                key_changed(k, raw[k]);
            }
            s_last_raw[k] = raw[k];
        }
        vTaskDelay(pdMS_TO_TICKS(SCAN_PERIOD_MS));
    }
    s_stopped = true;
    vTaskDelete(NULL);
}

extern "C" int matrix_kbd_init(const matrix_kbd_config_t *cfg)
{
    if (s_present) return 0;
    if (!cfg || !cfg->row_pins || !cfg->col_pins || !cfg->layers ||
        cfg->nrows == 0 || cfg->ncols == 0 || cfg->nlayers == 0 ||
        cfg->nrows > MATRIX_KBD_MAX_ROWS || cfg->ncols > MATRIX_KBD_MAX_COLS) {
        ESP_LOGE(TAG, "bad config");
        return -1;
    }
    s_cfg = cfg;

    /* matrix_kbd_prepare_sleep() held the columns low and left the rows
     * on the RTC mux; take both back. */
    gpio_deep_sleep_hold_dis();
    uint64_t row_mask = 0, col_mask = 0;
    for (int r = 0; r < cfg->nrows; r++) {
        gpio_num_t g = (gpio_num_t)cfg->row_pins[r];
        if (rtc_gpio_is_valid_gpio(g)) rtc_gpio_deinit(g);
        row_mask |= 1ULL << g;
    }
    for (int c = 0; c < cfg->ncols; c++) {
        gpio_num_t g = (gpio_num_t)cfg->col_pins[c];
        gpio_hold_dis(g);
        col_mask |= 1ULL << g;
    }

    gpio_config_t io = {};
    io.pin_bit_mask = row_mask;
    io.mode = GPIO_MODE_INPUT;
    io.pull_up_en = GPIO_PULLUP_ENABLE;
    io.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io.intr_type = GPIO_INTR_DISABLE;
    if (gpio_config(&io) != ESP_OK) return -1;

    /* gpio_config() also takes GPIO19 / GPIO20 (the ESP32-S3's native
     * USB pins) away from the USB PHY, which the MicroJournal Rev.6
     * matrix uses as columns. */
    io.pin_bit_mask = col_mask;
    io.mode = GPIO_MODE_OUTPUT_OD;
    io.pull_up_en = GPIO_PULLUP_DISABLE;
    if (gpio_config(&io) != ESP_OK) return -1;
    for (int c = 0; c < cfg->ncols; c++) {
        gpio_set_level((gpio_num_t)cfg->col_pins[c], 1);
    }

    memset(s_stable, 0, sizeof(s_stable));
    memset(s_last_raw, 0, sizeof(s_last_raw));
    for (int k = 0; k < MAX_KEYS; k++) s_action[k] = MKBD_NONE;
    s_osm_pending = 0;
    s_stop = false;
    s_stopped = false;
    if (xTaskCreate(scan_task, "matrix_kbd", 4096, NULL, 5, &s_task) != pdTRUE) {
        ESP_LOGE(TAG, "scan task create failed");
        return -1;
    }
    s_present = true;
    ESP_LOGI(TAG, "%dx%d key matrix ready", cfg->nrows, cfg->ncols);
    return 0;
}

extern "C" bool matrix_kbd_is_present(void)
{
    return s_present;
}

extern "C" void matrix_kbd_set_callback(kb_event_callback_t cb)
{
    s_cb = cb;
}

extern "C" uint64_t matrix_kbd_prepare_sleep(void)
{
    if (!s_present) return 0;
    s_cb = NULL;
    s_stop = true;
    for (int i = 0; i < 100 && !s_stopped; i++) vTaskDelay(pdMS_TO_TICKS(5));

    /* All columns low: any held key pulls its row low. */
    for (int c = 0; c < s_cfg->ncols; c++) {
        gpio_set_level((gpio_num_t)s_cfg->col_pins[c], 0);
    }
    bool held = true;
    for (int waited = 0; waited <= 3000; waited += 20) {
        esp_rom_delay_us(SETTLE_US);
        held = false;
        for (int r = 0; r < s_cfg->nrows; r++) {
            if (gpio_get_level((gpio_num_t)s_cfg->row_pins[r]) == 0) held = true;
        }
        if (!held) break;
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    for (int c = 0; c < s_cfg->ncols; c++) {
        gpio_hold_en((gpio_num_t)s_cfg->col_pins[c]);
    }
    gpio_deep_sleep_hold_en();

    if (held) {
        ESP_LOGW(TAG, "a key is still held: no key wake");
        return 0;
    }
    uint64_t mask = 0;
    for (int r = 0; r < s_cfg->nrows; r++) {
        gpio_num_t g = (gpio_num_t)s_cfg->row_pins[r];
        if (!rtc_gpio_is_valid_gpio(g)) return 0;
        rtc_gpio_pullup_en(g);
        rtc_gpio_pulldown_dis(g);
        mask |= 1ULL << g;
    }
    /* The RTC pull-ups live in the RTC_PERIPH domain. */
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    return mask;
}
