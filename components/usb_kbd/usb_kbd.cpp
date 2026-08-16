/*
 * USB HID keyboard host.
 *
 * Brings up the ESP-IDF USB Host stack and the espressif/usb_host_hid
 * managed component and opens any attached HID keyboard interface,
 * translating its keyboard input reports into the same kb_event_t
 * event stream the BLE keyboard component emits (modifier + keycode,
 * character=0; the editor does the keycode->char mapping itself).
 *
 * Two kinds of keyboard interfaces are recognised:
 *
 *  - Boot-protocol keyboards (bInterfaceSubClass = boot interface,
 *    bInterfaceProtocol = keyboard): the common case for plain USB
 *    keyboards. We force SET_PROTOCOL(BOOT) so we always get the
 *    fixed 8-byte report layout regardless of the device's native
 *    report-descriptor preferences.
 *
 *  - Generic "report protocol" interfaces (bInterfaceSubClass = 0,
 *    bInterfaceProtocol = 0) whose report descriptor happens to
 *    contain a standard keyboard Application collection (Usage Page
 *    Generic Desktop / Usage Keyboard) laid out exactly like the
 *    8-byte boot report. This covers composite devices built with
 *    TinyUSB (e.g. a combined keyboard+mouse HID interface that
 *    multiplexes both collections via Report IDs on a single
 *    interface, as used by the Smart Inclusive Keyboard project),
 *    which never advertise boot-protocol support even though their
 *    report layout matches it byte-for-byte. find_keyboard_report_id()
 *    parses the report descriptor to locate that collection's Report
 *    ID (if any), and the interface callback strips a leading Report
 *    ID byte before treating the rest as a boot-format report.
 *
 * The USB Host PHY + VBUS power gate must already be enabled before
 * calling usb_kbd_init(). On M5Stack Tab5 main.cpp does this via
 * bsp_usb_host_start() from the espressif/m5stack_tab5 BSP.
 *
 * Reference: ESP-IDF examples/peripherals/usb/host/hid (boot-mode
 * keyboard branch). We deliberately do NOT handle generic HID
 * devices other than the keyboard collection described above: this
 * component is only here to feed the editor with key events from a
 * physically-attached keyboard.
 */

#include "usb_kbd.h"

#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <esp_log.h>
#include <esp_err.h>

#include "usb/usb_host.h"
#include "usb/hid_host.h"
#include "usb/hid_usage_keyboard.h"

#include "ble_keyboard.h"

static const char *TAG = "USBKbd";

/* Public callback (mirrors ble_keyboard's s_callback) */
static kb_event_callback_t s_callback = NULL;
static usb_kbd_connect_cb_t s_connect_cb = NULL;

/* Set true while at least one HID keyboard interface is open */
static volatile bool s_kbd_connected = false;

/* Tasks + queue used to drive the HID host event pump from outside
 * the application thread. (The USB Host library task is owned by
 * the BSP -- see bsp_usb_host_start() in espressif/esp-bsp.) */
static TaskHandle_t  s_hid_event_task  = NULL;
static QueueHandle_t s_hid_event_queue = NULL;

/* Boot-protocol report state: track the previous 6 keycodes so we
 * emit one press/release event per change. */
#define KBD_BOOT_KEY_SLOTS 6
static uint8_t s_prev_keys[KBD_BOOT_KEY_SLOTS] = {0};
static uint8_t s_prev_mod = 0;

/* Recognised keyboard interfaces. Some physical keyboards expose
 * more than one HID interface that we identify as a keyboard, e.g.
 * a primary boot-protocol interface for the main key matrix plus a
 * second boot-protocol-flavoured interface for extra keys, or (as
 * with a composite USB keyboard+mouse built with TinyUSB, used by
 * the Smart Inclusive Keyboard project) a single generic "report
 * protocol" HID interface (bInterfaceSubClass = 0, bInterfaceProtocol
 * = 0) whose report descriptor multiplexes a keyboard collection and
 * a mouse collection via distinct Report IDs. hid_host_dev_params_t
 * for such an interface never matches HID_SUBCLASS_BOOT_INTERFACE /
 * HID_PROTOCOL_KEYBOARD, so we additionally parse the report
 * descriptor ourselves to find a top-level keyboard Application
 * collection and its Report ID (if any).
 *
 * We used to track only a single "active" keyboard handle, which
 * silently dropped all key input whenever a keyboard enumerated more
 * than one recognised interface: the second CONNECTED event
 * overwrote the single active handle, so input reports arriving on
 * the first (still open) interface were rejected by the handle
 * check and never reached the editor. Track every recognised
 * interface in a small fixed table instead so all of them can feed
 * key events. */
#define MAX_KBD_IFACES 4

typedef struct {
    hid_host_device_handle_t handle;
    bool    in_use;
    bool    is_boot;
    bool    uses_id;
    uint8_t report_id;
} kbd_iface_t;

static kbd_iface_t s_kbd_ifaces[MAX_KBD_IFACES] = {};
static int s_kbd_iface_count = 0; /* number of slots with in_use == true */

static kbd_iface_t *find_kbd_iface(hid_host_device_handle_t handle)
{
    for (int i = 0; i < MAX_KBD_IFACES; i++) {
        if (s_kbd_ifaces[i].in_use && s_kbd_ifaces[i].handle == handle) {
            return &s_kbd_ifaces[i];
        }
    }
    return NULL;
}

static bool add_kbd_iface(hid_host_device_handle_t handle, bool is_boot,
                          bool uses_id, uint8_t report_id)
{
    for (int i = 0; i < MAX_KBD_IFACES; i++) {
        if (!s_kbd_ifaces[i].in_use) {
            s_kbd_ifaces[i].handle     = handle;
            s_kbd_ifaces[i].in_use     = true;
            s_kbd_ifaces[i].is_boot    = is_boot;
            s_kbd_ifaces[i].uses_id    = uses_id;
            s_kbd_ifaces[i].report_id  = report_id;
            s_kbd_iface_count++;
            return true;
        }
    }
    return false;
}

static bool remove_kbd_iface(hid_host_device_handle_t handle)
{
    for (int i = 0; i < MAX_KBD_IFACES; i++) {
        if (s_kbd_ifaces[i].in_use && s_kbd_ifaces[i].handle == handle) {
            s_kbd_ifaces[i] = (kbd_iface_t){};
            s_kbd_iface_count--;
            return true;
        }
    }
    return false;
}

/* Minimal HID report descriptor parser: looks for a top-level
 * Application collection tagged Usage Page "Generic Desktop"
 * (0x01) / Usage "Keyboard" (0x06), and returns the Report ID
 * active at the first Input item inside that collection (0 if the
 * descriptor never emits a Report ID item, meaning reports for this
 * interface are not prefixed with an ID byte). Only short items are
 * supported, which covers every descriptor generated by TinyUSB's
 * TUD_HID_REPORT_DESC_* helpers and the vast majority of real-world
 * HID devices. */
static bool find_keyboard_report_id(const uint8_t *desc, size_t len,
                                    uint8_t *out_report_id)
{
    uint16_t usage_page = 0;
    uint8_t  report_id = 0;
    uint16_t usage_stack[8];
    int      usage_count = 0;
    int      depth = 0;
    bool     in_kbd_collection = false;
    int      kbd_collection_depth = -1;
    bool     found = false;

    size_t i = 0;
    while (i < len) {
        uint8_t item = desc[i++];
        uint8_t size = item & 0x03;
        if (size == 3) size = 4;
        uint8_t type = (item >> 2) & 0x03;
        uint8_t tag  = (item >> 4) & 0x0F;

        uint32_t data = 0;
        for (int b = 0; b < size && i < len; b++) {
            data |= ((uint32_t)desc[i++]) << (8 * b);
        }

        if (type == 1) {              /* Global item */
            if (tag == 0x0) {
                usage_page = (uint16_t)data;        /* Usage Page */
            } else if (tag == 0x8) {
                report_id = (uint8_t)data;           /* Report ID */
            }
        } else if (type == 2) {       /* Local item */
            if (tag == 0x0 && usage_count < 8) {
                usage_stack[usage_count++] = (uint16_t)data; /* Usage */
            }
        } else if (type == 0) {       /* Main item */
            if (tag == 0xA) {          /* Collection */
                depth++;
                if (!in_kbd_collection && data == 0x01 /* Application */ &&
                    usage_page == 0x01 && usage_count > 0 &&
                    usage_stack[usage_count - 1] == 0x06 /* Keyboard */) {
                    in_kbd_collection = true;
                    kbd_collection_depth = depth;
                }
            } else if (tag == 0xC) {  /* End Collection */
                if (in_kbd_collection && depth == kbd_collection_depth) {
                    in_kbd_collection = false;
                    kbd_collection_depth = -1;
                }
                if (depth > 0) depth--;
            } else if (tag == 0x8) {  /* Input */
                if (in_kbd_collection && !found) {
                    found = true;
                    *out_report_id = report_id;
                }
            }
            usage_count = 0;           /* Local state resets after Main item */
        }
    }
    return found;
}

/* Queued HID-host driver event (we cannot call hid_host_device_open
 * from the driver callback itself -- it runs in the HID host driver
 * context). */
typedef struct {
    hid_host_device_handle_t handle;
    hid_host_driver_event_t  event;
    void                    *arg;
} hid_event_t;

/* ---- USB Host library task ---- */
/* Owned by the BSP on Tab5 (bsp_usb_host_start spawns usb_lib_task).
 * No-op in this component to avoid a duplicate usb_host_install
 * call. */

/* ---- HID host event helpers ---- */

static bool key_in_array(uint8_t kc, const uint8_t *keys, int n)
{
    for (int i = 0; i < n; i++) {
        if (keys[i] == kc) return true;
    }
    return false;
}

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

/* Boot-protocol HID keyboard report: 8 bytes.
 *   [0]   = modifier bitmap (HID_*_CTRL / SHIFT / ALT / GUI, same
 *           bit layout as KB_MOD_*)
 *   [1]   = reserved (always 0)
 *   [2-7] = up to 6 simultaneous keycodes; 0 = empty slot
 */
static void process_boot_kbd_report(const uint8_t *data, size_t len)
{
    if (len < 3) return;     /* malformed; ignore */

    uint8_t mod = data[0];
    /* data[1] is the reserved byte; keycodes start at data[2]. */
    const uint8_t *keys = &data[2];
    int key_count = (int)len - 2;
    if (key_count > KBD_BOOT_KEY_SLOTS) key_count = KBD_BOOT_KEY_SLOTS;

    /* Newly pressed keys (in current report, not in previous) */
    for (int i = 0; i < key_count; i++) {
        uint8_t kc = keys[i];
        /* HID keycodes 0..3 are reserved error codes; skip. */
        if (kc < 0x04) continue;
        if (!key_in_array(kc, s_prev_keys, KBD_BOOT_KEY_SLOTS)) {
            dispatch_key(mod, kc, true);
        }
    }

    /* Released keys (in previous report, not in current) */
    for (int i = 0; i < KBD_BOOT_KEY_SLOTS; i++) {
        uint8_t kc = s_prev_keys[i];
        if (kc < 0x04) continue;
        if (!key_in_array(kc, keys, key_count)) {
            dispatch_key(mod, kc, false);
        }
    }

    /* Save state for the next diff. */
    memset(s_prev_keys, 0, sizeof(s_prev_keys));
    memcpy(s_prev_keys, keys, (size_t)key_count);
    s_prev_mod = mod;
}

/* ---- HID host driver callbacks ---- */

/* Per-interface callback: input reports, disconnect, transfer error. */
static void hid_iface_cb(hid_host_device_handle_t hid_dev_handle,
                          const hid_host_interface_event_t event,
                          void *arg)
{
    switch (event) {
    case HID_HOST_INTERFACE_EVENT_INPUT_REPORT: {
        kbd_iface_t *iface = find_kbd_iface(hid_dev_handle);
        if (!iface) return;

        uint8_t buf[64];
        size_t  buf_len = 0;
        if (hid_host_device_get_raw_input_report_data(hid_dev_handle,
                                                       buf, sizeof(buf),
                                                       &buf_len) != ESP_OK) {
            return;
        }
        const uint8_t *report = buf;
        size_t report_len = buf_len;
        if (!iface->is_boot && iface->uses_id) {
            /* Generic report-protocol interface with Report IDs (e.g.
             * a composite keyboard+mouse HID interface): each report
             * is prefixed with its Report ID byte. Only process
             * reports carrying the keyboard's own ID; a mouse report
             * on the same interface is silently ignored here. */
            if (report_len < 1 || buf[0] != iface->report_id) return;
            report++;
            report_len--;
        }
        process_boot_kbd_report(report, report_len);
        break;
    }
    case HID_HOST_INTERFACE_EVENT_DISCONNECTED:
        hid_host_device_close(hid_dev_handle);
        if (!remove_kbd_iface(hid_dev_handle)) {
            /* Not an interface we recognised as a keyboard: e.g. a
             * companion mouse / vendor interface on a composite
             * device, or an interface we opened only to inspect its
             * report descriptor and then rejected as non-keyboard.
             * Its disconnect (including one we trigger ourselves by
             * closing it right after probing) must not be mistaken
             * for a keyboard going away. */
            break;
        }
        if (s_kbd_iface_count > 0) {
            /* Another recognised keyboard interface on the same (or
             * a different) device is still open; the keyboard as a
             * whole is still connected. */
            break;
        }
        ESP_LOGI(TAG, "USB HID keyboard disconnected");
        s_kbd_connected = false;
        /* Release any held keys so the editor does not see stuck
         * modifiers after a hot-unplug. */
        for (int i = 0; i < KBD_BOOT_KEY_SLOTS; i++) {
            uint8_t kc = s_prev_keys[i];
            if (kc >= 0x04) dispatch_key(s_prev_mod, kc, false);
        }
        memset(s_prev_keys, 0, sizeof(s_prev_keys));
        s_prev_mod = 0;
        if (s_connect_cb) s_connect_cb(false);
        /* The wired keyboard is gone. Hand input back to the BLE
         * keyboard subsystem (if BLE was initialised at boot) so
         * the device starts scanning for a Bluetooth keyboard
         * again. No-op when BLE has never been brought up. */
        ble_keyboard_enable();
        break;
    case HID_HOST_INTERFACE_EVENT_TRANSFER_ERROR:
        ESP_LOGW(TAG, "USB HID transfer error");
        break;
    default:
        ESP_LOGD(TAG, "USB HID unhandled iface event %d", (int)event);
        break;
    }
}

/* Driver-level callback: device connected / disconnected. Runs in
 * the HID host driver task. We just push the event into a queue and
 * let s_hid_event_task open the device (the open call needs a
 * normal task context). */
static void hid_drv_cb(hid_host_device_handle_t hid_dev_handle,
                       const hid_host_driver_event_t event, void *arg)
{
    if (!s_hid_event_queue) return;
    hid_event_t evt = {
        .handle = hid_dev_handle,
        .event  = event,
        .arg    = arg,
    };
    xQueueSend(s_hid_event_queue, &evt, 0);
}

static void hid_event_task(void *arg)
{
    hid_event_t evt;
    while (true) {
        if (xQueueReceive(s_hid_event_queue, &evt, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        if (evt.event != HID_HOST_DRIVER_EVENT_CONNECTED) continue;

        hid_host_dev_params_t dev_params = {};
        if (hid_host_device_get_params(evt.handle, &dev_params) != ESP_OK) {
            continue;
        }
        ESP_LOGI(TAG, "USB HID device connected (sub_class=%d, proto=%d)",
                 (int)dev_params.sub_class, (int)dev_params.proto);

        /* Fast path: a device that advertises itself as a boot-
         * protocol keyboard interface. Most plain USB keyboards take
         * this path. */
        bool is_boot_kbd = (dev_params.sub_class == HID_SUBCLASS_BOOT_INTERFACE &&
                            dev_params.proto == HID_PROTOCOL_KEYBOARD);

        /* Open every HID interface (not just boot keyboards): some
         * composite devices (e.g. a TinyUSB-based keyboard+mouse
         * combo) expose a single generic "report protocol" interface
         * (sub_class = 0, proto = 0) whose report descriptor
         * multiplexes a keyboard collection and a mouse collection
         * via distinct Report IDs. We can only tell them apart from a
         * plain mouse/generic HID device by parsing the report
         * descriptor, which requires the device to be open first. */
        hid_host_device_config_t cfg = {
            .callback     = hid_iface_cb,
            .callback_arg = NULL,
        };
        if (hid_host_device_open(evt.handle, &cfg) != ESP_OK) {
            ESP_LOGE(TAG, "hid_host_device_open failed");
            continue;
        }

        bool    found_kbd = false;
        bool    uses_report_id = false;
        uint8_t report_id = 0;
        if (!is_boot_kbd) {
            size_t desc_len = 0;
            uint8_t *desc = hid_host_get_report_descriptor(evt.handle, &desc_len);
            if (desc && desc_len > 0) {
                found_kbd = find_keyboard_report_id(desc, desc_len, &report_id);
                /* Report ID 0 is reserved by the HID spec and never
                 * used as an actual ID, so report_id != 0 reliably
                 * means the descriptor prefixes every report on this
                 * interface with an ID byte. */
                uses_report_id = found_kbd && report_id != 0;
            }
            if (!found_kbd) {
                ESP_LOGI(TAG, "Ignoring non-keyboard HID device");
                hid_host_device_close(evt.handle);
                continue;
            }
        }

        if (is_boot_kbd) {
            /* Force boot protocol so we always get the fixed 8-byte
             * report layout regardless of the keyboard's native
             * report-descriptor preferences. */
            esp_err_t pe = hid_class_request_set_protocol(evt.handle,
                                                          HID_REPORT_PROTOCOL_BOOT);
            if (pe != ESP_OK) {
                ESP_LOGW(TAG, "set_protocol(BOOT) failed: %s "
                              "(keyboard may send report-descriptor format "
                              "instead of boot)", esp_err_to_name(pe));
            }
            /* SetIdle 0 -> only report on state change (no autorepeat
             * floods). */
            hid_class_request_set_idle(evt.handle, 0, 0);
        }
        if (hid_host_device_start(evt.handle) != ESP_OK) {
            ESP_LOGE(TAG, "hid_host_device_start failed");
            hid_host_device_close(evt.handle);
            continue;
        }
        ESP_LOGI(TAG, "USB HID keyboard ready");
        bool was_connected = (s_kbd_iface_count > 0);
        if (!add_kbd_iface(evt.handle, is_boot_kbd, uses_report_id, report_id)) {
            ESP_LOGW(TAG, "too many keyboard interfaces, dropping this one");
            hid_host_device_close(evt.handle);
            continue;
        }
        s_kbd_connected = true;

        if (was_connected) {
            /* Another keyboard interface (e.g. a second interface on
             * the same physical keyboard) was already recognised;
             * the keyboard-connected state and BLE handoff already
             * happened for the first one. */
            continue;
        }

        /* A wired keyboard now owns input. If BLE was brought up
         * earlier this boot (BLE keyboard was already paired and
         * the user just plugged in a USB keyboard), tear it down
         * so the device stops scanning, stops emitting BLE status
         * messages, and stops dispatching duplicate key events.
         * No-op if BLE was never initialised. */
        ble_keyboard_disable();

        /* Notify after ble_keyboard_disable(): that call fires the
         * BLE connect callback with `false` to tell the UI the BLE
         * link is gone. Firing our own "USB connected" notification
         * afterwards ensures the editor's pending-connect state
         * lands on `true` and the UI returns to the editor / file
         * browser rather than getting stuck on the "Keyboard
         * disconnected" prompt screen. */
        if (s_connect_cb) s_connect_cb(true);
    }
}

/* ---- Public API ---- */

extern "C" int usb_kbd_init(void)
{
    /* NOTE: We deliberately do NOT call usb_host_install() or spawn a
     * usb_lib_task here. On Tab5 bsp_usb_host_start() already does
     * both (see espressif/esp-bsp bsp/m5stack_tab5/src/bsp_usb.c).
     * Calling usb_host_install a second time returns
     * ESP_ERR_INVALID_STATE. Callers on boards without that BSP
     * helper must install the host library themselves before
     * invoking usb_kbd_init(). */

    /* Spin up the queue + task that will service hid_drv_cb events. */
    s_hid_event_queue = xQueueCreate(8, sizeof(hid_event_t));
    if (!s_hid_event_queue) {
        ESP_LOGE(TAG, "hid event queue create failed");
        return -1;
    }
    if (xTaskCreate(hid_event_task, "usb_hid_evt", 4096, NULL, 4,
                    &s_hid_event_task) != pdTRUE) {
        ESP_LOGE(TAG, "hid_event task create failed");
        return -1;
    }

    /* Install the HID host driver. It spawns its own background task
     * to drive control transfers. */
    const hid_host_driver_config_t hid_cfg = {
        .create_background_task = true,
        .task_priority          = 5,
        .stack_size             = 4096,
        .core_id                = 0,
        .callback               = hid_drv_cb,
        .callback_arg           = NULL,
    };
    esp_err_t err = hid_host_install(&hid_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "hid_host_install failed: %s", esp_err_to_name(err));
        return -1;
    }

    ESP_LOGI(TAG, "HID host driver installed; waiting for keyboard");
    return 0;
}

extern "C" bool usb_kbd_is_connected(void)
{
    return s_kbd_connected;
}

extern "C" void usb_kbd_set_callback(kb_event_callback_t cb)
{
    s_callback = cb;
}

extern "C" void usb_kbd_set_connect_callback(usb_kbd_connect_cb_t cb)
{
    s_connect_cb = cb;
}
