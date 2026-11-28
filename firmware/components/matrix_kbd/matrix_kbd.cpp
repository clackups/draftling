/*
 * Built-in key-matrix keyboard. See include/matrix_kbd.h.
 */

#include "matrix_kbd.h"

#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_rom_sys.h>
#include <esp_sleep.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>

static const char *TAG = "MatrixKbd";

/* Scan period. Two equal scans in a row make a change (debounce), so a
 * key is reported 5..10 ms after it settles. */
#define SCAN_PERIOD_MS     5
/* Time for a row to follow a column driven LOW through the diode and
 * the internal pull-up (~45 kohm) before it is read. */
#define SETTLE_US          20
#define RELEASE_WAIT_MS    10000

#define MAX_KEYS (MATRIX_KBD_MAX_ROWS * MATRIX_KBD_MAX_COLS)

static matrix_kbd_config_t s_cfg;
static bool s_present;
static volatile bool s_stop;
static volatile bool s_stopped;
static kb_event_callback_t s_callback;
static TaskHandle_t s_task;

static bool    s_stable[MAX_KEYS];   /* debounced state */
static bool    s_last[MAX_KEYS];     /* previous raw scan */
static uint8_t s_sent[MAX_KEYS];     /* keycode sent on press, for its release */
static uint8_t s_mods;               /* HID modifier mask */
static bool    s_fn;

static void scan(bool *raw)
{
    for (int c = 0; c < s_cfg.ncols; c++) {
        gpio_set_level((gpio_num_t)s_cfg.cols[c], 0);
        esp_rom_delay_us(SETTLE_US);
        for (int r = 0; r < s_cfg.nrows; r++) {
            raw[r * s_cfg.ncols + c] =
                gpio_get_level((gpio_num_t)s_cfg.rows[r]) == 0;
        }
        gpio_set_level((gpio_num_t)s_cfg.cols[c], 1);
    }
}

static void dispatch(uint8_t keycode, bool pressed)
{
    if (!s_callback) return;
    kb_event_t ev = {};
    ev.modifier  = s_mods;
    ev.keycode   = keycode;
    ev.character = 0;     /* the editor maps keycode -> character */
    ev.pressed   = pressed;
    s_callback(&ev);
}

static void key_changed(int k, bool down)
{
    uint8_t code = s_cfg.keymap[k];
    if (code == MATRIX_KBD_FN) {
        s_fn = down;
        return;
    }
    if (code >= 0xE0 && code <= 0xE7) {
        uint8_t bit = (uint8_t)(1u << (code - 0xE0));
        if (down) s_mods |= bit; else s_mods &= (uint8_t)~bit;
        return;
    }
    if (down) {
        if (s_fn && s_cfg.fn_keymap && s_cfg.fn_keymap[k])
            code = s_cfg.fn_keymap[k];
        if (!code) return;
        s_sent[k] = code;
        dispatch(code, true);
    } else {
        /* Release what the press sent, even if Fn changed meanwhile. */
        code = s_sent[k];
        s_sent[k] = 0;
        if (code) dispatch(code, false);
    }
}

static void scan_task(void *arg)
{
    (void)arg;
    const int nkeys = s_cfg.nrows * s_cfg.ncols;
    bool raw[MAX_KEYS];
    TickType_t wake = xTaskGetTickCount();
    while (!s_stop) {
        scan(raw);
        for (int k = 0; k < nkeys; k++) {
            if (raw[k] == s_last[k] && raw[k] != s_stable[k]) {
                s_stable[k] = raw[k];
                key_changed(k, raw[k]);
            }
            s_last[k] = raw[k];
        }
        xTaskDelayUntil(&wake, pdMS_TO_TICKS(SCAN_PERIOD_MS));
    }
    s_stopped = true;
    vTaskDelete(NULL);
}

extern "C" int matrix_kbd_init(const matrix_kbd_config_t *cfg)
{
    if (!cfg || !cfg->rows || !cfg->cols || !cfg->keymap ||
        cfg->nrows <= 0 || cfg->nrows > MATRIX_KBD_MAX_ROWS ||
        cfg->ncols <= 0 || cfg->ncols > MATRIX_KBD_MAX_COLS) {
        ESP_LOGE(TAG, "bad configuration");
        return -1;
    }
    s_cfg = *cfg;

    /* Undo what matrix_kbd_prepare_sleep() set up for deep sleep: the
     * column holds and the rows' RTC IO routing. */
    for (int c = 0; c < s_cfg.ncols; c++)
        gpio_hold_dis((gpio_num_t)s_cfg.cols[c]);
    for (int r = 0; r < s_cfg.nrows; r++) {
        gpio_num_t g = (gpio_num_t)s_cfg.rows[r];
        if (rtc_gpio_is_valid_gpio(g)) rtc_gpio_deinit(g);
    }

    uint64_t col_mask = 0, row_mask = 0;
    for (int c = 0; c < s_cfg.ncols; c++) col_mask |= 1ULL << s_cfg.cols[c];
    for (int r = 0; r < s_cfg.nrows; r++) row_mask |= 1ULL << s_cfg.rows[r];

    gpio_config_t g = {};
    g.intr_type    = GPIO_INTR_DISABLE;
    g.mode         = GPIO_MODE_OUTPUT;
    g.pin_bit_mask = col_mask;
    esp_err_t err = gpio_config(&g);
    if (err == ESP_OK) {
        for (int c = 0; c < s_cfg.ncols; c++)
            gpio_set_level((gpio_num_t)s_cfg.cols[c], 1);
        g.mode         = GPIO_MODE_INPUT;
        g.pin_bit_mask = row_mask;
        g.pull_up_en   = GPIO_PULLUP_ENABLE;
        err = gpio_config(&g);
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config failed: %s", esp_err_to_name(err));
        return -1;
    }

    memset(s_stable, 0, sizeof(s_stable));
    memset(s_last, 0, sizeof(s_last));
    memset(s_sent, 0, sizeof(s_sent));
    s_mods = 0;
    s_fn = false;
    s_stop = false;
    s_stopped = false;
    /* Internal-RAM stack (xTaskCreate's default): the task runs while
     * flash writes have the cache, and with it PSRAM, switched off. */
    if (xTaskCreate(scan_task, "matrix_kbd", 3072, NULL, 5, &s_task) != pdTRUE) {
        ESP_LOGE(TAG, "task create failed");
        return -1;
    }
    s_present = true;
    ESP_LOGI(TAG, "%dx%d key matrix ready", s_cfg.nrows, s_cfg.ncols);
    return 0;
}

extern "C" bool matrix_kbd_is_present(void)
{
    return s_present;
}

extern "C" void matrix_kbd_set_callback(kb_event_callback_t cb)
{
    s_callback = cb;
}

static bool any_key_down(void)
{
    bool raw[MAX_KEYS];
    scan(raw);
    for (int k = 0; k < s_cfg.nrows * s_cfg.ncols; k++)
        if (raw[k]) return true;
    return false;
}

extern "C" bool matrix_kbd_prepare_sleep(void)
{
    if (!s_present) return false;
    if (s_task && !s_stopped) {
        s_stop = true;
        for (int i = 0; i < 100 && !s_stopped; i++)
            vTaskDelay(pdMS_TO_TICKS(SCAN_PERIOD_MS));
    }
    s_callback = NULL;

    /* The keys that asked for sleep (Ctrl+P) may still be down; a held
     * key would wake the chip at once. */
    int waited = 0;
    while (any_key_down() && waited < RELEASE_WAIT_MS) {
        vTaskDelay(pdMS_TO_TICKS(20));
        waited += 20;
    }
    if (any_key_down()) {
        ESP_LOGW(TAG, "a key is still held -- keyboard wake not armed");
        return false;
    }

    uint64_t row_mask = 0;
    for (int r = 0; r < s_cfg.nrows; r++) {
        gpio_num_t g = (gpio_num_t)s_cfg.rows[r];
        if (!rtc_gpio_is_valid_gpio(g)) {
            ESP_LOGW(TAG, "row GPIO%d is not an RTC IO -- keyboard wake not armed",
                     (int)g);
            return false;
        }
        row_mask |= 1ULL << g;
    }

    /* Every column LOW through deep sleep: a key then pulls its row LOW. */
    for (int c = 0; c < s_cfg.ncols; c++) {
        gpio_set_level((gpio_num_t)s_cfg.cols[c], 0);
        gpio_hold_en((gpio_num_t)s_cfg.cols[c]);
    }

    /* The row pull-ups have to live in the RTC domain while asleep. */
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_err_t err = esp_sleep_enable_ext1_wakeup_io(row_mask, ESP_EXT1_WAKEUP_ANY_LOW);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "esp_sleep_enable_ext1_wakeup_io failed: %s",
                 esp_err_to_name(err));
        return false;
    }
    for (int r = 0; r < s_cfg.nrows; r++) {
        gpio_num_t g = (gpio_num_t)s_cfg.rows[r];
        rtc_gpio_pullup_en(g);
        rtc_gpio_pulldown_dis(g);
    }
    ESP_LOGI(TAG, "any key wakes the device");
    return true;
}
