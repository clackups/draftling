/*
 * M5Stack Tab5 attachable keyboard host.
 *
 * Minimal driver-NG (driver/i2c_master.h) driver for the Tab5
 * keyboard co-processor (I2C address 0x6D) plus its interrupt line
 * on GPIO 50. We bring the keyboard up in interrupt HID mode and turn
 * each queued HID report (modifier + keycode) into the same
 * kb_event_t press/release stream the ble_keyboard / usb_kbd
 * components emit, so the editor key handler is shared verbatim.
 *
 * Register map and event semantics follow
 * m5stack/M5Tab5-Keyboard-UserDemo
 * (components/m5_tab5_keyboard_component). Only the handful of
 * registers Draftling needs are implemented here; the full vendor
 * library (RGB binding modes, string mode, address re-flashing,
 * Arduino backend) is intentionally not vendored.
 */

#include "tab5_kbd.h"

#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <esp_log.h>
#include <esp_err.h>
#include <esp_timer.h>
#include <driver/gpio.h>

static const char *TAG = "Tab5Kbd";

/* ---- Tab5 keyboard register map ---- */
#define REG_INT_STA       0x01   /* interrupt status (bit1 = HID event) */
#define REG_EVENT_NUM     0x02   /* queued event count */
#define REG_KEYBOARD_MODE 0x10   /* working mode (1 = HID) */
#define REG_RGB_MODE      0x11   /* 0 = binding, 1 = custom */
#define REG_HID_EVENT     0x30   /* 2 bytes: [modifier, keycode] */
#define REG_RGB_COLOR     0x60   /* 7 bytes: B,G,R,rsvd,B,G,R */
#define REG_VERSION       0xFE   /* firmware version (presence probe) */

#define KB_MODE_HID       0x01
#define RGB_MODE_CUSTOM   0x01
#define INT_STA_HID_BIT   0x02

/* HID keycodes 0..3 are reserved error codes; 0 means "no key". */
#define HID_KEY_MIN       0x04

/* A 0xFF/0xFF report is the sentinel for "no event" returned when the
 * queue is read while empty; it must not be treated as a keypress. */
#define HID_NO_EVENT_BYTE 0xFF

/* Cap the per-IRQ drain so a wedged device cannot spin forever. */
#define EVENT_DRAIN_MAX   32

#define I2C_TIMEOUT_MS    50

static i2c_master_dev_handle_t s_dev = NULL;
static volatile bool s_present = false;
static int s_int_gpio = -1;

static kb_event_callback_t s_callback = NULL;

static TaskHandle_t  s_task  = NULL;
static QueueHandle_t s_queue = NULL;
static esp_timer_handle_t s_led_off_timer = NULL;

/* Previous HID state for press/release diffing. The Tab5 HID report
 * carries a single keycode slot, so one key is tracked at a time. */
static uint8_t s_prev_kc  = 0;
static uint8_t s_prev_mod = 0;

/* ---- Low-level register I/O ---- */

static bool reg_read(uint8_t reg, uint8_t *data, size_t len)
{
    if (!s_dev) return false;
    return i2c_master_transmit_receive(s_dev, &reg, 1, data, len,
                                       I2C_TIMEOUT_MS) == ESP_OK;
}

static bool reg_write(uint8_t reg, const uint8_t *data, size_t len)
{
    if (!s_dev) return false;
    uint8_t buf[8];
    if (len + 1 > sizeof(buf)) return false;
    buf[0] = reg;
    memcpy(&buf[1], data, len);
    return i2c_master_transmit(s_dev, buf, len + 1, I2C_TIMEOUT_MS) == ESP_OK;
}

static bool reg_write_byte(uint8_t reg, uint8_t value)
{
    return reg_write(reg, &value, 1);
}

static bool reg_read_byte(uint8_t reg, uint8_t *value)
{
    return reg_read(reg, value, 1);
}

/* ---- LED helpers ---- */

/* The RGB window is [RGB1_B, RGB1_G, RGB1_R, reserved, RGB2_B, RGB2_G,
 * RGB2_R]; both LEDs are written in one transaction. */
static void set_both_leds(uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t buf[7] = { b, g, r, 0x00, b, g, r };
    reg_write(REG_RGB_COLOR, buf, sizeof(buf));
}

static void led_off_timer_cb(void *arg)
{
    (void)arg;
    if (!s_present) return;
    set_both_leds(0, 0, 0);
}

/* ---- Key event dispatch ---- */

static void dispatch_key(uint8_t modifier, uint8_t keycode, bool pressed)
{
    if (!s_callback) return;
    kb_event_t ev = {};
    ev.modifier  = modifier;
    ev.keycode   = keycode;
    ev.character = 0;     /* editor does keycode->char mapping */
    ev.pressed   = pressed;
    s_callback(&ev);
}

/* HID report: modifier + single keycode (keycode 0 = key released).
 * Diff against the previous report to emit press/release events. */
static void process_hid_report(uint8_t mod, uint8_t kc)
{
    if (kc != s_prev_kc) {
        if (s_prev_kc >= HID_KEY_MIN) {
            dispatch_key(s_prev_mod, s_prev_kc, false);
        }
        if (kc >= HID_KEY_MIN) {
            dispatch_key(mod, kc, true);
        }
    }
    s_prev_kc  = kc;
    s_prev_mod = mod;
}

static void drain_events(void)
{
    uint8_t status = 0;
    if (!reg_read_byte(REG_INT_STA, &status)) return;

    if (status & INT_STA_HID_BIT) {
        uint8_t count = 0;
        if (reg_read_byte(REG_EVENT_NUM, &count)) {
            int guard = 0;
            while (count > 0 && guard < EVENT_DRAIN_MAX) {
                uint8_t buf[2];
                if (reg_read(REG_HID_EVENT, buf, 2)) {
                    if (!(buf[0] == HID_NO_EVENT_BYTE &&
                          buf[1] == HID_NO_EVENT_BYTE)) {
                        process_hid_report(buf[0], buf[1]);
                    }
                }
                count--;
                guard++;
            }
        }
    }

    /* Acknowledge: clearing the status register releases the INT line. */
    reg_write_byte(REG_INT_STA, 0x00);
}

/* ---- Interrupt plumbing ---- */

static void IRAM_ATTR int_isr(void *arg)
{
    (void)arg;
    if (!s_queue) return;
    uint32_t token = 1;
    BaseType_t hp_woken = pdFALSE;
    xQueueSendFromISR(s_queue, &token, &hp_woken);
    if (hp_woken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

static void kbd_task(void *arg)
{
    (void)arg;
    uint32_t token;
    while (true) {
        if (xQueueReceive(s_queue, &token, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        /* Coalesce a burst of edges: the device queue holds the real
         * events, so one pass through drain_events() handles them. */
        while (xQueueReceive(s_queue, &token, 0) == pdTRUE) {
        }
        drain_events();
    }
}

static bool setup_interrupt(int int_gpio)
{
    gpio_config_t io = {};
    io.intr_type    = GPIO_INTR_NEGEDGE;
    io.mode         = GPIO_MODE_INPUT;
    io.pin_bit_mask = 1ULL << int_gpio;
    io.pull_up_en   = GPIO_PULLUP_ENABLE;
    io.pull_down_en = GPIO_PULLDOWN_DISABLE;
    if (gpio_config(&io) != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config(INT GPIO%d) failed", int_gpio);
        return false;
    }

    s_queue = xQueueCreate(8, sizeof(uint32_t));
    if (!s_queue) {
        ESP_LOGE(TAG, "int queue create failed");
        return false;
    }

    if (xTaskCreate(kbd_task, "tab5_kbd", 4096, NULL, 5, &s_task) != pdTRUE) {
        ESP_LOGE(TAG, "kbd task create failed");
        return false;
    }

    esp_err_t err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "gpio_install_isr_service failed: %s",
                 esp_err_to_name(err));
        return false;
    }
    if (gpio_isr_handler_add((gpio_num_t)int_gpio, int_isr, NULL) != ESP_OK) {
        ESP_LOGE(TAG, "gpio_isr_handler_add failed");
        return false;
    }
    return true;
}

/* ---- Public API ---- */

extern "C" int tab5_kbd_init(i2c_master_bus_handle_t bus, int int_gpio)
{
    if (s_present) {
        return 0;     /* already initialised this boot */
    }
    if (!bus) {
        ESP_LOGW(TAG, "no I2C bus handle; Tab5 keyboard disabled");
        return -1;
    }

    i2c_device_config_t dev_cfg = {};
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address  = TAB5_KBD_I2C_ADDR;
    dev_cfg.scl_speed_hz    = 100000;
    if (i2c_master_bus_add_device(bus, &dev_cfg, &s_dev) != ESP_OK) {
        ESP_LOGW(TAG, "i2c_master_bus_add_device failed");
        s_dev = NULL;
        return -1;
    }

    /* Presence probe: read the firmware version register. A keyboard
     * that is not attached does not ACK, so this fails and we go
     * permanently idle (no further I2C traffic until the next boot). */
    uint8_t version = 0;
    if (!reg_read_byte(REG_VERSION, &version)) {
        ESP_LOGI(TAG, "No Tab5 keyboard attached (version probe failed)");
        i2c_master_bus_rm_device(s_dev);
        s_dev = NULL;
        return -1;
    }
    ESP_LOGI(TAG, "Tab5 keyboard detected (firmware 0x%02X)", version);

    /* Enter HID mode and clear any stale events / interrupt latch. */
    reg_write_byte(REG_KEYBOARD_MODE, KB_MODE_HID);
    reg_write_byte(REG_EVENT_NUM, 0x00);
    reg_write_byte(REG_INT_STA, 0x00);

    /* Custom RGB mode so the indicator LEDs are under our control
     * (not bound to keypresses). Greet with both LEDs green, then
     * turn them off after one second. */
    reg_write_byte(REG_RGB_MODE, RGB_MODE_CUSTOM);
    set_both_leds(0, 255, 0);

    esp_timer_create_args_t targs = {};
    targs.callback = led_off_timer_cb;
    targs.name     = "tab5_kbd_led";
    if (esp_timer_create(&targs, &s_led_off_timer) == ESP_OK) {
        esp_timer_start_once(s_led_off_timer, 1000 * 1000);
    } else {
        ESP_LOGW(TAG, "LED-off timer create failed; leaving LEDs green");
    }

    s_int_gpio = int_gpio;
    if (!setup_interrupt(int_gpio)) {
        ESP_LOGE(TAG, "interrupt setup failed; Tab5 keyboard disabled");
        if (s_dev) {
            i2c_master_bus_rm_device(s_dev);
            s_dev = NULL;
        }
        return -1;
    }

    s_present = true;
    ESP_LOGI(TAG, "Tab5 keyboard ready (HID interrupt mode, INT GPIO%d)",
             int_gpio);

    /* Service any event already queued between mode-set and ISR setup. */
    drain_events();
    return 0;
}

extern "C" bool tab5_kbd_is_present(void)
{
    return s_present;
}

extern "C" void tab5_kbd_set_callback(kb_event_callback_t cb)
{
    s_callback = cb;
}
