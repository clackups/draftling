/*
 * BLE HID Host keyboard driver.
 *
 * Uses ESP-IDF Bluedroid BLE stack for scanning and the unified HID
 * host component (esp_hidh) for HID-over-GATT. Only BLE is used
 * (no Classic BT) on every supported board.
 *
 * Note: esp_bt.h is the BT *controller* API, required on boards
 * with a native on-chip BLE controller (ESP32-S3). On the ESP32-P4
 * (M5Stack Tab5) there is no on-chip 2.4 GHz radio, so the BLE
 * controller actually lives on the on-board ESP32-C6 co-processor
 * and is reached over ESP-Hosted-MCU's SDIO transport. The
 * Bluedroid host runs on the P4 just the same; we attach a
 * hosted-VHCI HCI driver to it instead of initializing the native
 * esp_bt_controller. This switch is driven by
 * CONFIG_BT_CONTROLLER_DISABLED in ble_keyboard_init() below; see
 * docs/tab5-esp-hosted.md for the C6 slave firmware setup.
 *
 * Multi-pairing: stores up to MAX_BONDED bonded device addresses in
 * NVS.  On startup it tries the last-connected device first, then
 * other known devices, then scans for any new HID keyboard.
 *
 * Pairing: uses DisplayOnly IO capability.  When a new keyboard
 * pairs, a random 6-digit passkey is generated and shown on the
 * display; the user types it on the keyboard to confirm.
 *
 * Targets without a usable Bluedroid host (CONFIG_BT_BLUEDROID_ENABLED
 * unset -- either because the whole BT stack is off, or because the
 * IDF port for the target does not yet build the Bluedroid host) are
 * compiled to no-op stubs at the bottom of this file. They link the
 * same public API so Draftling startup code can call
 * ble_keyboard_init() unconditionally.
 */

#include "sdkconfig.h"
#include "ble_keyboard.h"

#if !defined(CONFIG_BT_BLUEDROID_ENABLED)

#include <esp_log.h>

extern "C" {

static const char *TAG_STUB = "BLEKeyboard";

void ble_keyboard_init(void)
{
    ESP_LOGW(TAG_STUB, "Bluedroid host not enabled in sdkconfig; "
                       "BLE keyboard support is disabled on this build.");
}
void ble_keyboard_disable(void)                                          {}
void ble_keyboard_enable(void)                                           {}
void ble_keyboard_forget_all(void)                                       {}
void ble_keyboard_set_callback(kb_event_callback_t /*cb*/)               {}
void ble_keyboard_set_passkey_callback(ble_passkey_cb_t /*cb*/)          {}
void ble_keyboard_set_connect_callback(ble_connect_cb_t /*cb*/)          {}
void ble_keyboard_set_status_text_callback(ble_status_text_cb_t /*cb*/)  {}
bool ble_keyboard_is_connected(void)                                     { return false; }
void ble_keyboard_start_scan(void)                                       {}
const char *ble_keyboard_get_device_name(void)                           { return ""; }
int ble_keyboard_get_battery_level(void)                                 { return -1; }

} /* extern "C" */

#else /* CONFIG_BT_BLUEDROID_ENABLED */

#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <esp_log.h>
#if !defined(CONFIG_BT_CONTROLLER_DISABLED)
/* esp_bt.h declares the on-chip BT *controller* API
 * (esp_bt_controller_init/enable/mem_release, ESP_BT_MODE_*,
 * BT_CONTROLLER_INIT_CONFIG_DEFAULT). On targets without a
 * native controller (ESP32-P4: CONFIG_BT_CONTROLLER_DISABLED=y),
 * the header is not even installed by ESP-IDF's `bt` component,
 * and we don't need any of those symbols -- the controller lives
 * on the C6 and is reached via ESP-Hosted's VHCI (see the
 * CONFIG_BT_CONTROLLER_DISABLED branch in ble_keyboard_init()). */
#include <esp_bt.h>
#endif
#include <esp_bt_main.h>
#include <esp_gap_ble_api.h>
#include <esp_gattc_api.h>
#include <esp_hidh.h>
#include <nvs_flash.h>
#include <nvs.h>

#if defined(CONFIG_BT_CONTROLLER_DISABLED)
/* Bluedroid HCI driver attachment API
 * (esp_bluedroid_attach_hci_driver +
 * esp_bluedroid_hci_driver_operations_t). Available in ESP-IDF's
 * bt component when the Bluedroid host is built without its
 * default on-chip HCI transport. */
#include <esp_bluedroid_hci.h>
/* ESP-Hosted hosted-VHCI driver (esp_hosted_bt_controller_*,
 * hosted_hci_bluedroid_*). Only pulled in when ESP-Hosted is in
 * use, i.e. on the ESP32-P4 build.
 *
 * NOTE: upstream esp_hosted_bluedroid.h (and the esp_hosted_bt.h
 * it transitively pulls in) ship WITHOUT an `extern "C"` guard,
 * so we must wrap them ourselves; otherwise this C++ TU looks for
 * the C++-mangled name `hosted_hci_bluedroid_open()` while the
 * actual symbol defined in espressif/esp_hosted's vhci_drv.c has
 * plain C linkage, producing an undefined-reference at link time.
 * esp_hosted.h itself is already C-guarded. */
#include <esp_hosted.h>
extern "C" {
#include <esp_hosted_bluedroid.h>
}
#endif

static const char *TAG = "BLEKeyboard";

/* BLE HID service UUID */
#define BLE_SVC_HID_UUID16         0x1812
/* BLE appearance values -- HID category */
#define BLE_APPEARANCE_HID_GENERIC 0x03C0
#define BLE_APPEARANCE_KEYBOARD    0x03C1

/* 128-bit BLE base UUID for HID service (little-endian):
 * 00001812-0000-1000-8000-00805f9b34fb */
static const uint8_t BLE_SVC_HID_UUID128[16] = {
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
    0x00, 0x10, 0x00, 0x00, 0x12, 0x18, 0x00, 0x00
};

/* Scan duration in seconds */
#define SCAN_DURATION_SEC          30

/* Safety-net timer period (ms).  Fires periodically until scanning
 * starts or a connection is established. */
#define STARTUP_SAFETY_TIMER_MS   3000

/* Stack size for the BLE connect task.  Needs room for
 * esp_hidh_dev_open() which uses significant stack space. */
#define CONNECT_TASK_STACK        6144

/* Stack size for the BLE init task.  Runs the full BT stack
 * bring-up including esp_hidh_init() which blocks on a BTC
 * semaphore -- must not run in app_main. */
#define BLE_INIT_TASK_STACK       8192


/* Maximum number of bonded keyboards stored in NVS */
#define MAX_BONDED  8

/* NVS namespace and keys */
#define NVS_NAMESPACE   "ble_kb"
#define NVS_KEY_COUNT   "bond_cnt"
#define NVS_KEY_LAST    "bond_last"
/* Per-device keys: "bond_0" .. "bond_7", each stores 7 bytes
 * (6-byte BDA + 1-byte address type).
 * Per-device name keys: "name_0" .. "name_7", each stores a
 * NUL-terminated device name string (up to 63 chars). */

/* ---- Bonded device storage ---- */

#define BONDED_NAME_LEN 64

typedef struct {
    esp_bd_addr_t       bda;
    esp_ble_addr_type_t addr_type;
    char                name[BONDED_NAME_LEN];
} bonded_dev_t;

static bonded_dev_t s_bonded[MAX_BONDED];
static int          s_bonded_count = 0;
static int          s_last_bonded  = -1; /* index into s_bonded */

/* Forward-declare status text callback (full declaration with other callbacks below) */
static ble_status_text_cb_t s_status_text_cb = NULL;

/* Device name -- declared here (before bonded_add) so it is visible
 * throughout the file.  Sized to match BONDED_NAME_LEN. */
static char s_dev_name[BONDED_NAME_LEN] = "";

/* Set true when ble_keyboard_disable() is called (USB keyboard hot-
 * plug path). Once set, every public entry point and internal task
 * short-circuits so no further BLE activity is started, no events
 * are dispatched to the editor, and no status text is pushed to the
 * UI. The flag is one-shot for the rest of this boot.
 *
 * Defined this far up the file so notify_status() / dispatch_key()
 * (which sit above the rest of the state block) can test it.  All
 * other state lives lower down, in the "State" block. */
static volatile bool s_disabled = false;

/* Helper: notify the UI of a human-readable status message */
static void notify_status(const char *fmt, ...)
    __attribute__((format(printf, 1, 2)));
static void notify_status(const char *fmt, ...)
{
    if (s_disabled) return;
    if (!s_status_text_cb) return;
    char buf[128];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    s_status_text_cb(buf);
}

/* Remove older bonded entries that share a name with a newer entry.
 * Two bond records with the same device name are almost always the
 * same physical keyboard at different resolvable addresses (NuPhy
 * Air60, e.g., re-randomises its private address after a re-pair),
 * and the stale address will fail the next connect attempt with
 * either a 25 s open watchdog or an SMP timeout. Keeping only the
 * newest entry (highest index) per name lets RECONN_KNOWN walk a
 * meaningful list. Returns true if anything was removed (caller
 * should bonded_save() if so). */
static void bonded_save(void);

static bool bonded_dedupe_by_name(void)
{
    bool changed = false;
    /* Walk from highest index to lowest; whenever we see a name,
     * keep that entry and delete any earlier (lower-index) entry
     * with the same non-empty name. */
    for (int i = s_bonded_count - 1; i >= 0; i--) {
        if (!s_bonded[i].name[0]) continue;
        for (int j = i - 1; j >= 0; j--) {
            if (!s_bonded[j].name[0]) continue;
            if (strcmp(s_bonded[i].name, s_bonded[j].name) != 0) continue;
            ESP_LOGI(TAG, "Dropping duplicate bond %d (\"%s\") "
                          "in favour of newer entry %d",
                     j, s_bonded[j].name, i);
            /* Shift entries above j down by one slot. */
            for (int k = j; k < s_bonded_count - 1; k++) {
                s_bonded[k] = s_bonded[k + 1];
            }
            s_bonded_count--;
            /* The outer index i was above j, so it shifted down. */
            i--;
            /* s_last_bonded fixup: a deletion at index j shifts all
             * higher indices down by one. */
            if (s_last_bonded == j) {
                /* The "last" entry was a duplicate of a newer one --
                 * promote the newer one (which is now at index i). */
                s_last_bonded = i;
            } else if (s_last_bonded > j) {
                s_last_bonded--;
            }
            changed = true;
        }
    }
    return changed;
}

static void bonded_load(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &h) != ESP_OK) return;

    uint8_t cnt = 0;
    if (nvs_get_u8(h, NVS_KEY_COUNT, &cnt) != ESP_OK) cnt = 0;
    if (cnt > MAX_BONDED) cnt = MAX_BONDED;

    int8_t last = -1;
    if (nvs_get_i8(h, NVS_KEY_LAST, &last) != ESP_OK) last = -1;

    s_bonded_count = 0;
    for (int i = 0; i < cnt; i++) {
        char key[12];
        snprintf(key, sizeof(key), "bond_%d", i);
        uint8_t buf[7];
        size_t len = sizeof(buf);
        if (nvs_get_blob(h, key, buf, &len) == ESP_OK && len == 7) {
            memcpy(s_bonded[s_bonded_count].bda, buf, 6);
            s_bonded[s_bonded_count].addr_type =
                (esp_ble_addr_type_t)buf[6];
            /* Load stored device name (if any) */
            char name_key[12];
            snprintf(name_key, sizeof(name_key), "name_%d", i);
            size_t nlen = sizeof(s_bonded[s_bonded_count].name);
            if (nvs_get_str(h, name_key,
                            s_bonded[s_bonded_count].name, &nlen) != ESP_OK) {
                s_bonded[s_bonded_count].name[0] = '\0';
            }
            s_bonded_count++;
        }
    }
    s_last_bonded = (last >= 0 && last < s_bonded_count) ? last : -1;
    nvs_close(h);
    /* Drop duplicate bonds for the same physical keyboard so
     * RECONN_KNOWN doesn't waste a 25 s open watchdog per stale
     * resolvable address. Persist the cleaned list. */
    if (bonded_dedupe_by_name()) {
        bonded_save();
    }
    ESP_LOGI(TAG, "Loaded %d bonded device(s), last=%d",
             s_bonded_count, s_last_bonded);
}

static void bonded_save(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h) != ESP_OK) return;

    nvs_set_u8(h, NVS_KEY_COUNT, (uint8_t)s_bonded_count);
    nvs_set_i8(h, NVS_KEY_LAST, (int8_t)s_last_bonded);

    for (int i = 0; i < s_bonded_count; i++) {
        char key[12];
        snprintf(key, sizeof(key), "bond_%d", i);
        uint8_t buf[7];
        memcpy(buf, s_bonded[i].bda, 6);
        buf[6] = (uint8_t)s_bonded[i].addr_type;
        nvs_set_blob(h, key, buf, sizeof(buf));
        /* Save device name */
        char name_key[12];
        snprintf(name_key, sizeof(name_key), "name_%d", i);
        if (s_bonded[i].name[0]) {
            nvs_set_str(h, name_key, s_bonded[i].name);
        } else {
            nvs_erase_key(h, name_key); /* no name stored */
        }
    }
    nvs_commit(h);
    nvs_close(h);
}

/* Find a bonded device by BDA; returns index or -1 */
static int bonded_find(const esp_bd_addr_t bda)
{
    for (int i = 0; i < s_bonded_count; i++) {
        if (memcmp(s_bonded[i].bda, bda, 6) == 0) return i;
    }
    return -1;
}

/* Add or promote a bonded device to "last used" */
static void bonded_add(const esp_bd_addr_t bda,
                        esp_ble_addr_type_t addr_type)
{
    int idx = bonded_find(bda);
    if (idx < 0) {
        /* New device -- evict oldest if full */
        if (s_bonded_count >= MAX_BONDED) {
            /* Shift array down, dropping slot 0 */
            memmove(&s_bonded[0], &s_bonded[1],
                    (size_t)(MAX_BONDED - 1) * sizeof(bonded_dev_t));
            s_bonded_count = MAX_BONDED - 1;
        }
        idx = s_bonded_count;
        memcpy(s_bonded[idx].bda, bda, 6);
        s_bonded[idx].addr_type = addr_type;
        s_bonded[idx].name[0] = '\0';
        s_bonded_count++;
    }
    /* Update stored name from the current device name */
    if (s_dev_name[0]) {
        /* Use snprintf to copy with guaranteed NUL termination and
         * without tripping GCC's -Wstringop-truncation, which fires on
         * the strncpy(dst, src, sizeof(dst)-1) idiom under newer
         * toolchains (seen with ESP-IDF 5.5.4). */
        snprintf(s_bonded[idx].name, sizeof(s_bonded[idx].name),
                 "%s", s_dev_name);
    }
    s_last_bonded = idx;
    /* Drop older bond entries that share this device's name (stale
     * resolvable addresses for the same physical keyboard) before
     * we persist. Fixes s_last_bonded if our just-added entry moves. */
    bonded_dedupe_by_name();
    bonded_save();
    ESP_LOGI(TAG, "Bonded device saved at index %d (\"%s\")",
             s_last_bonded,
             (s_last_bonded >= 0) ? s_bonded[s_last_bonded].name : "");
}

/* ---- State ---- */

static kb_event_callback_t  s_callback = NULL;
static ble_passkey_cb_t     s_passkey_cb = NULL;
static ble_connect_cb_t     s_connect_cb = NULL;
static volatile bool s_connected  = false;
static volatile bool s_connecting = false;
static volatile bool s_scan_params_ready = false;
static volatile bool s_hidh_ready = false;
/* Maximum number of keycode slots in an extended boot / key-array
 * report.  Most NKRO key-array keyboards use up to 15 slots. */
#define MAX_KEY_SLOTS 20
static uint8_t s_prev_keys[MAX_KEY_SLOTS] = {0};
static int     s_prev_key_count = 0;
/* Previous full report buffer for NKRO bitmap comparison */
static uint8_t s_prev_report[32] = {0};
static int     s_prev_report_len = 0;

/* Detected keyboard report format (sticky after first detection).
 * REPORT_FMT_UNKNOWN  -- not yet determined for this connection
 * REPORT_FMT_BOOT     -- standard boot protocol (8 bytes, reserved byte)
 * REPORT_FMT_BOOT_EXT -- extended boot / key-array (variable length)
 * REPORT_FMT_NKRO_BM  -- NKRO bitmap
 */
typedef enum {
    REPORT_FMT_UNKNOWN = 0,
    REPORT_FMT_BOOT,
    REPORT_FMT_BOOT_EXT,
    REPORT_FMT_NKRO_BM,
} report_fmt_t;
static report_fmt_t s_report_fmt = REPORT_FMT_UNKNOWN;

static volatile int s_battery_level = -1;  /* -1 = unknown */

/* Active HIDH device handle (valid while connected) */
static esp_hidh_dev_t *s_hidh_dev = NULL;

/* Target device address saved during scan for deferred connection */
static esp_bd_addr_t s_target_bda;
static esp_ble_addr_type_t s_target_addr_type;

/* Timer used to retry scanning after a delay */
static TimerHandle_t s_scan_timer = NULL;

/* Reconnection state machine */
typedef enum {
    RECONN_LAST,     /* trying last-connected device */
    RECONN_KNOWN,    /* trying other known devices */
    RECONN_SCAN,     /* scanning for any HID keyboard */
} reconn_phase_t;

static reconn_phase_t s_reconn_phase = RECONN_LAST;
static int            s_reconn_idx   = 0; /* index for RECONN_KNOWN */
static TimerHandle_t  s_reconn_timer = NULL;

/* Reconnect backoff. Defaults to RECONN_DEFAULT_MS, but is extended
 * when the peer reports SMP_REPEATED_ATTEMPTS (0x61) in
 * ESP_GAP_BLE_AUTH_CMPL_EVT -- many keyboards (e.g. NuPhy Air60)
 * rate-limit pairing on their side, and hammering them at the
 * default 1 s interval just keeps reproducing the failure (and the
 * resulting BLE-bus storm wedges the epdiy `epd_prep` feeders).
 * Reset on a successful auth. */
#define RECONN_DEFAULT_MS   1000
#define RECONN_SMP_BASE_MS  5000
#define RECONN_SMP_MAX_MS   30000
static uint32_t       s_reconn_delay_ms      = RECONN_DEFAULT_MS;
static int            s_smp_repeat_attempts  = 0;

/* Start (or restart) the reconnect timer using the current
 * s_reconn_delay_ms backoff. xTimerChangePeriod() also starts the
 * timer if it is dormant, so one call suffices. */
static inline void reconn_timer_kick(void)
{
    if (!s_reconn_timer) return;
    xTimerChangePeriod(s_reconn_timer,
                       pdMS_TO_TICKS(s_reconn_delay_ms), 0);
}

/* Periodic startup timer: safety net that re-registers the GAP
 * callback and retries scanning until connected. */
static TimerHandle_t  s_startup_timer = NULL;

/* "Open watchdog" timer. esp_hidh_dev_open() blocks for the entire
 * BLE GATT connect + service discovery + HID report-map handshake,
 * which on a flaky link can stall well past the LE supervision
 * timeout. If the controller drops the link mid-handshake, the
 * Bluedroid disconnect path has been observed to crash on a
 * partially-initialised device record (Load Access Fault inside
 * gattc_conn_cb after rsn=0x8 connection-timeout).
 *
 * An earlier version of this watchdog called esp_ble_gap_disconnect()
 * from the timer callback to force the link down. That turned out to
 * be unsafe: while esp_hidh_dev_open() is still blocked inside service
 * discovery / CCCD writes the controller link may not actually be up
 * yet, so the synchronous disconnect is a no-op; when pairing later
 * completes the stack ends up in exactly the half-initialised state
 * we were trying to avoid, and the eventual controller supervision
 * timeout (rsn=0x8) panics inside gattc_conn_cb / bta_hh_le.
 *
 * Current behaviour: on watchdog fire we do NOT touch the link from
 * outside Bluedroid. Instead we set s_force_close_on_open and let the
 * blocking esp_hidh_dev_open() finish naturally. The ESP_HIDH_OPEN_EVENT
 * handler then calls esp_hidh_dev_close() on the fully-constructed
 * device record (success branch) or simply restarts the reconnect
 * sequence (failure branch). This guarantees the close runs against a
 * defined device record, which is what avoids the post-rsn-0x8 crash.
 *
 * If even that does not produce a HIDH event within
 * OPEN_STUCK_RECOVERY_MS after the watchdog fired, s_open_stuck_timer
 * spawns a recovery task that tears down and re-initialises the HIDH
 * subsystem so the next connect attempt starts from a clean state. */
static TimerHandle_t  s_open_watchdog_timer = NULL;
static TimerHandle_t  s_open_stuck_timer    = NULL;
/* Most recently armed open-watchdog duration (ms). Captured for the
 * log line in open_watchdog_cb so it always reports the budget that
 * actually elapsed, regardless of which connect path armed it. */
static uint32_t       s_open_watchdog_armed_ms = 0;
/* Long enough to cover a slow first-pair on a real keyboard
 * (observed ~16 s end-to-end on a NuPhy Air60 V2). Shorter values
 * caused the watchdog to fire on the success path. */
#define OPEN_WATCHDOG_MS        25000
/* Shorter watchdog for reconnects to a device we already have a bond
 * for. Historically this was 10 s on the assumption that a healthy
 * cached-services/CCCDs reconnect completes in well under 5 s, so any
 * longer was pure give-up budget. In practice some keyboards (observed:
 * NuPhy Air60 V2) take ~5 s just to renegotiate connection parameters
 * after AUTH_CMPL on a reconnect, and HIDH service discovery / CCCD
 * writes can run for another several seconds after that. With a 10 s
 * budget the watchdog fired mid-discovery on a perfectly good link and
 * forced us into the s_open_stuck_timer + esp_ble_gap_disconnect
 * recovery path, which trips a LoadProhibited crash inside
 * Bluedroid/HIDH cleanup on the half-built device record.
 *
 * Match OPEN_WATCHDOG_MS (25 s) so reconnects get the same headroom as
 * first-pairs. We still fail fast against a silently-dead bond via the
 * RECONN_KNOWN sweep that follows; this just stops the watchdog from
 * killing slow-but-healthy reconnects. */
#define OPEN_WATCHDOG_BONDED_MS 25000
/* Time after the watchdog fires before we assume the open is wedged
 * inside Bluedroid and the HIDH subsystem itself needs restarting.
 *
 * Recovery is a true last resort: tearing HIDH down and back up at
 * runtime is fragile (esp_hidh_deinit() refuses to run while any
 * device record exists, and a failed re-init can leave the host
 * permanently unusable). We must therefore wait long enough that
 * every natural failure path has had time to produce a HIDH event
 * (OPEN or CLOSE) which would clear s_force_close_on_open and stop
 * this timer before it fires. The known natural timeouts are:
 *   - SMP authentication timeout (~30 s)
 *   - LE supervision timeout (SUPERVISION_TIMEOUT_FLOOR_10MS, 32 s)
 *   - HIDH's own service-discovery / CCCD-write retries
 * 60 s after the watchdog (≈85 s after esp_hidh_dev_open() was
 * called) sits comfortably past all of these, so reaching this
 * callback is genuine evidence that no HIDH event will ever arrive
 * for this open attempt. */
#define OPEN_STUCK_RECOVERY_MS  60000

/* Cooldown applied when a reconnect attempt finds HIDH not ready
 * (i.e. a previous hidh_recover_task failed to re-arm the host).
 * Without this, connect_task bails out and immediately rearms the
 * reconnect timer at RECONN_DEFAULT_MS (1 s), producing a flood of
 * "HIDH not available, cannot connect" errors. The hidh_recover_task
 * itself reschedules its own retry via s_open_stuck_timer; this
 * cooldown just keeps the connect path from spinning in the
 * meantime. */
#define HIDH_DOWN_RETRY_MS      15000

/* LE supervision-timeout floor while a HIDH open is in flight, in
 * units of 10 ms (BLE-spec encoding). 3200 = 32 s, the BLE spec
 * maximum (0x0C80). Chosen to be longer than the worst HIDH
 * service-discovery / CCCD-write window we are willing to wait
 * (OPEN_WATCHDOG_MS), so the controller does not drop the link out
 * from under a still-half-built device record while we are
 * patiently waiting for HIDH to surface an OPEN event. Applied via:
 *  - esp_ble_gap_set_prefer_conn_params() before each connect, so the
 *    initial negotiation already lands at or above this value;
 *  - ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT handler, which bumps the
 *    timeout back up if the peer later renegotiates a shorter value
 *    (as some HID keyboards do after pairing for power reasons).
 * See the s_open_watchdog_timer doc block above for why we must NOT
 * try to force the link down from app code instead. */
#define SUPERVISION_TIMEOUT_FLOOR_10MS  3200

/* LE supervision timeout target once esp_hidh_dev_open() has completed
 * successfully and the device record is fully built. At that point the
 * 32 s floor above is no longer load-bearing -- a normal ~4 s timeout
 * (what most BLE-HID hosts use) gives us much faster disconnect
 * detection when a keyboard goes out of range or is powered off.
 *
 * In the log this is the difference between noticing a disconnect at
 * +32 s (the controller's gattc_conn_cb rsn=0x8 after the supervision
 * window) and noticing it at +4 s. */
#define SUPERVISION_TIMEOUT_STEADY_10MS 400

/* True from the moment connect_task calls esp_ble_gap_set_prefer_conn_params()
 * until the matching HIDH OPEN/CLOSE event arrives. While true, any
 * ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT bumps the supervision timeout to
 * SUPERVISION_TIMEOUT_FLOOR_10MS (protect the half-built record).
 * While false, the same handler instead clamps the timeout down to
 * SUPERVISION_TIMEOUT_STEADY_10MS so a later peer-initiated re-negotiation
 * cannot leave us with a stale 32 s window after we are fully open. */
static volatile bool s_open_in_flight = false;

/* Cache of the most recently negotiated connection interval / latency,
 * captured in ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT. Used by the
 * ESP_HIDH_OPEN_EVENT success path to actively request the steady-state
 * supervision timeout without regressing the interval/latency the peer
 * chose. Zero before the first UPDATE arrives -- in that case the
 * OPEN handler skips the proactive update and lets the next natural
 * UPDATE event (with s_open_in_flight already cleared) apply the
 * steady target. */
static uint16_t      s_cur_conn_int   = 0;
static uint16_t      s_cur_conn_lat   = 0;

/* Serializes esp_ble_gap_update_conn_params() requests. The BLE link
 * layer can only have a single Connection Parameter Update Request in
 * flight at a time; firing a second one while the first is still
 * pending produces Bluedroid's "l2cble_start_conn_update, the last
 * connection update command still pending" warning AND queues the
 * second request in the controller. On the M5Stack Tab5 (ESP-Hosted
 * BT controller running on the ESP32-C6 slave) the queued request
 * eventually fires after the peer has already settled on the new
 * parameters, the peer fails to answer it, and ~6 s later the
 * controller reports UPDATE_CONN_PARAMS_EVT with status=16
 * (ESP_BT_STATUS_TIMEOUT). The link layer's LL response timer then
 * keeps running and drops the link ~30 s later with rsn=0x22
 * (HCI_ERR_LMP_LL_RESPONSE_TIMEOUT), which manifests as the keyboard
 * disconnecting a few seconds after every successful connect.
 *
 * The on-chip BT controller used by the LilyGO T5 absorbs the spam
 * silently; the hosted ESP32-C6 firmware does not. Avoid generating
 * overlapping requests on both targets.
 *
 * s_conn_update_pending is set right before each accepted call to
 * esp_ble_gap_update_conn_params() and cleared on the matching
 * ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT (or on disconnect). While set,
 * further requests are skipped; the UPDATE_CONN_PARAMS_EVT handler
 * re-evaluates whether another request is still needed once it has
 * cleared the flag. */
static volatile bool s_conn_update_pending = false;

/* Wraps esp_ble_gap_update_conn_params() with the in-flight gate
 * described above. Returns ESP_OK on success (request submitted),
 * ESP_ERR_INVALID_STATE if skipped because another update is still
 * pending, or the underlying error from esp_ble_gap_update_conn_params()
 * on failure (in which case the pending flag is left clear so the
 * caller can retry later). */
static esp_err_t request_conn_param_update(
        esp_ble_conn_update_params_t *up)
{
    if (s_conn_update_pending) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err = esp_ble_gap_update_conn_params(up);
    if (err == ESP_OK) {
        s_conn_update_pending = true;
    }
    return err;
}

/* Set by open_watchdog_cb() to tell the ESP_HIDH_OPEN_EVENT handler
 * (and connect_task post-return) to treat this connection attempt as
 * abandoned: close the device cleanly and resume the reconnect
 * sequence instead of marking us connected. Cleared on every HIDH
 * event and whenever a fresh open is initiated. */
static volatile bool s_force_close_on_open = false;

/* Set by the OPEN_EVENT handler when it discards a watchdog-flagged
 * open (closed cleanly on a fully-constructed device record).
 * Consumed by the subsequent CLOSE_EVENT handler to (a) advance the
 * reconnect phase past RECONN_LAST so we don't immediately re-open
 * the same address while the peer is in a transient post-pair state,
 * and (b) extend the cool-off before the next attempt. The peer
 * needs time to settle; hammering it triggered the very rsn=0x8
 * stuck-open crash this whole subsystem exists to dodge. */
static volatile bool s_post_discard_cooloff = false;

/* Set by ESP_GAP_BLE_AUTH_CMPL_EVT when authentication fails with a
 * reason that causes Bluedroid to wipe the bond (e.g. SMP timeout
 * 0x63, MITM/PIN failures, etc -- anything other than rate-limit
 * 0x61). Consumed by start_reconnection() / hidh_callback's CLOSE
 * path to skip RECONN_LAST and RECONN_KNOWN and go straight to a
 * fresh scan, since the just-tried bonded entry is now guaranteed
 * to be dead and any other bonded entry for the same physical
 * keyboard (at a stale resolvable address) will fail the same way.
 * Cleared on successful auth, when scanning starts, and when the
 * user explicitly forgets bonds. */
static volatile bool s_last_auth_bond_dead = false;

/* Forward declarations */
static void start_reconnection(void);
static void reconn_timer_cb(TimerHandle_t timer);
static void startup_timer_cb(TimerHandle_t timer);
static void open_watchdog_cb(TimerHandle_t timer);
static void open_watchdog_start(uint32_t duration_ms);
static void open_watchdog_stop(void);
static void open_stuck_cb(TimerHandle_t timer);
static void gap_event_handler(esp_gap_ble_cb_event_t event,
                               esp_ble_gap_cb_param_t *param);
static void hidh_callback(void *handler_args, esp_event_base_t base,
                           int32_t id, void *event_data);

/* Helper: fill in standard BLE scan parameters */
static void fill_scan_params(esp_ble_scan_params_t *p)
{
    memset(p, 0, sizeof(*p));
    p->scan_type          = BLE_SCAN_TYPE_ACTIVE;
    p->own_addr_type      = BLE_ADDR_TYPE_PUBLIC;
    p->scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL;
    p->scan_interval      = 0x0060;  /* 60 ms */
    p->scan_window        = 0x0030;  /* 30 ms */
    p->scan_duplicate     = BLE_SCAN_DUPLICATE_DISABLE;
}

static bool key_in_report(uint8_t key, const uint8_t *keys, int count)
{
    for (int i = 0; i < count; i++) {
        if (keys[i] == key) return true;
    }
    return false;
}

/* Dispatch a single key event to the registered callback */
static void dispatch_key(uint8_t modifier, uint8_t keycode, bool pressed)
{
    if (s_disabled) return;
    if (!s_callback) return;
    kb_event_t ev = {};
    ev.modifier  = modifier;
    ev.keycode   = keycode;
    ev.character = 0;
    ev.pressed   = pressed;
    ESP_LOGD(TAG, "Key %s: kc=0x%02x mod=0x%02x",
             pressed ? "DOWN" : "UP", keycode, modifier);
    s_callback(&ev);
}

/* Standard boot-protocol keyboard report (or extended key-array).
 * Boot protocol: [0]=modifier, [1]=reserved(0), [2..N]=keycodes
 * Key-array (no reserved byte): [0]=modifier, [1..N]=keycodes
 * The caller provides a pointer to the key array and its count. */
static void process_key_array(const uint8_t *keys, int key_count,
                              uint8_t modifier)
{
    if (key_count > MAX_KEY_SLOTS) key_count = MAX_KEY_SLOTS;

    /* Detect newly pressed keys */
    for (int i = 0; i < key_count; i++) {
        uint8_t kc = keys[i];
        if (kc == 0) continue;
        if (!key_in_report(kc, s_prev_keys, s_prev_key_count)) {
            dispatch_key(modifier, kc, true);
        }
    }

    /* Detect released keys */
    for (int i = 0; i < s_prev_key_count; i++) {
        uint8_t kc = s_prev_keys[i];
        if (kc == 0) continue;
        if (!key_in_report(kc, keys, key_count)) {
            dispatch_key(modifier, kc, false);
        }
    }

    memset(s_prev_keys, 0, sizeof(s_prev_keys));
    int copy_count = key_count < MAX_KEY_SLOTS ? key_count : MAX_KEY_SLOTS;
    memcpy(s_prev_keys, keys, (size_t)copy_count);
    s_prev_key_count = copy_count;
}

/* Legacy wrapper: standard boot protocol [mod, reserved, k0..k5] */
static void process_boot_report(const uint8_t *data, int len)
{
    uint8_t modifier = data[0];
    const uint8_t *keys = &data[2]; /* skip reserved byte */
    int key_count = len - 2;
    if (key_count < 0) key_count = 0;
    process_key_array(keys, key_count, modifier);
}

/* NKRO bitmap keyboard report: modifier + N bitmap bytes.
 * Each bit in the bitmap represents a keycode (bit 0 of byte 1 =
 * keycode 0, bit 7 of byte 1 = keycode 7, etc.).
 * Compare with previous bitmap to detect press/release. */
static void process_nkro_report(const uint8_t *data, int len)
{
    uint8_t modifier = data[0];
    /* Bitmap bytes start at data[1], up to data[len-1] */
    int bm_len = len - 1;
    const uint8_t *bm = &data[1];

    /* Previous bitmap starts at s_prev_report[1] */
    int prev_bm_len = s_prev_report_len > 0 ? s_prev_report_len - 1 : 0;
    const uint8_t *prev_bm = &s_prev_report[1];

    /* Scan for changed bits */
    int max_bm = bm_len > prev_bm_len ? bm_len : prev_bm_len;
    if (max_bm > 30) max_bm = 30;  /* safety cap: 240 keycodes */

    for (int i = 0; i < max_bm; i++) {
        uint8_t cur  = (i < bm_len)     ? bm[i] : 0;
        uint8_t prev = (i < prev_bm_len) ? prev_bm[i] : 0;
        uint8_t diff = cur ^ prev;
        if (diff == 0) continue;

        for (int bit = 0; bit < 8; bit++) {
            if (diff & (1 << bit)) {
                uint8_t keycode = (uint8_t)(i * 8 + bit);
                /* HID keycodes 0-3 are reserved (ErrorRollOver,
                 * POSTFail, ErrorUndefined).  Valid keys start
                 * at 0x04 (KC_A). */
                if (keycode < 0x04) continue;
                bool pressed = (cur & (1 << bit)) != 0;
                dispatch_key(modifier, keycode, pressed);
            }
        }
    }

    /* Save current report for next comparison */
    int save_len = len;
    if (save_len > (int)sizeof(s_prev_report))
        save_len = (int)sizeof(s_prev_report);
    memcpy(s_prev_report, data, (size_t)save_len);
    s_prev_report_len = save_len;
}

static void process_keyboard_report(const uint8_t *data, int len,
                                    uint8_t report_id)
{
    if (len < 3 || !s_callback) return;

    /* Log raw report bytes for diagnosis */
    {
        char hex[96];
        int pos = 0;
        int dump_len = len > 30 ? 30 : len;
        for (int i = 0; i < dump_len && pos < (int)sizeof(hex) - 4; i++) {
            pos += snprintf(hex + pos, sizeof(hex) - (size_t)pos,
                            "%02x ", data[i]);
        }
        ESP_LOGD(TAG, "Report (%d bytes, id=%d): %s", len, report_id, hex);
    }

    /* Determine report format.
     *
     * Standard boot-protocol keyboard: 8 bytes.
     *   [0]=modifier  [1]=reserved(0)  [2..7]=keycodes (6 slots)
     *
     * Extended boot protocol (some keyboards): >8 bytes with
     *   the same layout, just more keycode slots.
     *   [0]=modifier  [1]=reserved(0)  [2..N]=keycodes
     *
     * Key-array without reserved byte (NKRO key-array): variable.
     *   [0]=modifier  [1..N]=keycodes  (no reserved byte)
     *
     * NKRO bitmap: modifier + N bitmap bytes where each BIT is a
     *   keycode.
     *
     * Format detection uses a "sticky" approach: once a format is
     * identified for this connection, it is reused for all subsequent
     * reports.  This is checked FIRST to avoid false positives where
     * modifier byte values (e.g. 0x01=LCtrl) coincide with report
     * IDs or reserved-byte patterns. */

    /* ---- Sticky format: use previously detected format ---- */
    switch (s_report_fmt) {
    case REPORT_FMT_BOOT:
        ESP_LOGD(TAG, "Boot protocol (sticky, %d bytes)", len);
        process_boot_report(data, len);
        return;
    case REPORT_FMT_BOOT_EXT:
        ESP_LOGD(TAG, "Key-array (sticky, %d bytes)", len);
        process_key_array(&data[1], len - 1, data[0]);
        return;
    case REPORT_FMT_NKRO_BM:
        ESP_LOGD(TAG, "NKRO bitmap (sticky, %d bytes)", len);
        process_nkro_report(data, len);
        return;
    case REPORT_FMT_UNKNOWN:
    default:
        break;  /* fall through to first-time detection */
    }

    /* ---- First-time format detection ---- */

    /* Case: exactly 8 bytes with reserved byte == 0 -> boot protocol */
    if (len == 8 && data[1] == 0) {
        s_report_fmt = REPORT_FMT_BOOT;
        ESP_LOGI(TAG, "Detected boot protocol (8 bytes)");
        process_boot_report(data, len);
        return;
    }

    /* Case: extended boot protocol (reserved byte present, > 8 bytes).
     * data[1]==0 is the reserved byte that distinguishes boot-style
     * reports from key-array / NKRO bitmap formats. */
    if (data[1] == 0 && len > 8) {
        s_report_fmt = REPORT_FMT_BOOT_EXT;
        ESP_LOGI(TAG, "Detected extended boot protocol (%d bytes)", len);
        process_boot_report(data, len);
        return;
    }

    /* For reports where byte[1] != 0 we must distinguish between
     * key-array and NKRO bitmap.
     *
     * Key insight: in a key-array, each non-zero byte IS a HID keycode
     * (0x04..0xE7).  In a bitmap, each byte is a bit-field where
     * individual bits represent keycodes.
     *
     * Heuristic: if any payload byte is 0x01-0x03, those are reserved
     * HID keycodes that would never appear in a key-array, so it must
     * be a bitmap.  Otherwise, check if any non-zero byte has multiple
     * bits set AND is a plausible keycode (>= 0x04).  In a bitmap,
     * having multiple bits set in one byte means multiple keys in the
     * same 8-keycode group are pressed simultaneously -- uncommon for
     * a first-report heuristic.  A key-array byte for common keys
     * (like 0x11=N) naturally has multiple bits set.
     *
     * If all non-zero bytes have exactly 1 bit set, it is ambiguous.
     * Default to NKRO bitmap to preserve backward compatibility. */
    {
        bool has_low_value = false;  /* any byte 0x01-0x03 */
        bool has_multi_bit = false;  /* any byte with >1 bit set */
        for (int i = 1; i < len; i++) {
            uint8_t b = data[i];
            if (b == 0) continue;
            if (b >= 0x01 && b <= 0x03) { has_low_value = true; break; }
            /* __builtin_popcount counts set bits */
            if (__builtin_popcount(b) > 1) has_multi_bit = true;
        }
        if (has_low_value) {
            /* Definitely bitmap: 0x01-0x03 are impossible as keycodes */
            s_report_fmt = REPORT_FMT_NKRO_BM;
            ESP_LOGI(TAG, "Detected NKRO bitmap format (%d bytes)", len);
            process_nkro_report(data, len);
            return;
        }
        if (has_multi_bit) {
            /* Byte like 0x11 (N) has bits 0 and 4 set -- common as a
             * keycode value but very unusual as a bitmap byte (would
             * need keycodes 0 and 4 pressed simultaneously).  Treat
             * as key-array. */
            s_report_fmt = REPORT_FMT_BOOT_EXT;
            ESP_LOGI(TAG, "Detected key-array format (%d bytes)", len);
            uint8_t modifier = data[0];
            process_key_array(&data[1], len - 1, modifier);
            return;
        }
        /* Ambiguous: all non-zero bytes are power-of-two values >= 4.
         * Default to NKRO bitmap for backward compatibility. */
        s_report_fmt = REPORT_FMT_NKRO_BM;
        ESP_LOGI(TAG, "Assuming NKRO bitmap format (%d bytes)", len);
        process_nkro_report(data, len);
    }
}

/* ---- Unified HID Host callback ---- */

static void hidh_callback(void *handler_args, esp_event_base_t base,
                           int32_t id, void *event_data)
{
    if (s_disabled) return;
    esp_hidh_event_t event = (esp_hidh_event_t)id;
    esp_hidh_event_data_t *param = (esp_hidh_event_data_t *)event_data;

    switch (event) {
    case ESP_HIDH_OPEN_EVENT: {
        /* Capture before stop_watchdog clears the flag. */
        bool force_close = s_force_close_on_open;
        /* Either branch (success or failure) terminates the open
         * sequence -- disarm the watchdog. */
        open_watchdog_stop();
        /* The open sequence has produced an event one way or another;
         * the half-built-record window is closed and the
         * supervision-timeout floor is no longer needed. The success
         * branch below also actively requests the STEADY timeout. */
        s_open_in_flight = false;
        if (param->open.status == ESP_OK) {
            /* If the watchdog flagged this attempt as suspect (or
             * the user disabled BLE while we were blocked inside
             * dev_open), close the device cleanly NOW, on a
             * fully-constructed record, and restart reconnection.
             * This is the safe substitute for the previous
             * mid-pairing gap_disconnect, and is what avoids the
             * post-rsn-0x8 crash inside gattc_conn_cb. */
            if (force_close || s_disabled) {
                ESP_LOGW(TAG, "Discarding watchdog-flagged open: "
                              "closing device and retrying");
                /* Cool off and skip RECONN_LAST for the next cycle:
                 * the peer is in a transient post-pair state and an
                 * immediate retry against the same address was
                 * observed to get stuck inside CCCD writes (no
                 * OPEN_EVENT, leading to a rsn=0x8 crash). Try other
                 * bonded devices / a fresh scan first. */
                s_post_discard_cooloff = true;
                esp_hidh_dev_t *dev = param->open.dev;
                if (dev) {
                    esp_hidh_dev_close(dev);
                }
                /* A subsequent ESP_HIDH_CLOSE_EVENT will reset
                 * state and kick the reconnect timer; leave
                 * s_connecting set until then so we don't race a
                 * second connect on top of the close. */
                break;
            }
            s_connected  = true;
            s_connecting = false;
            s_hidh_dev   = param->open.dev;
            const uint8_t *addr = esp_hidh_dev_bda_get(param->open.dev);
            const char *name = esp_hidh_dev_name_get(param->open.dev);
            if (name && name[0]) {
                strncpy(s_dev_name, name, sizeof(s_dev_name) - 1);
                s_dev_name[sizeof(s_dev_name) - 1] = '\0';
            }
            /* Remember this device as bonded / last-used */
            if (addr) {
                bonded_add(addr, s_target_addr_type);
            }
            ESP_LOGI(TAG, "HID device connected: %s ("
                     "%02x:%02x:%02x:%02x:%02x:%02x)",
                     s_dev_name,
                     addr ? addr[0] : 0, addr ? addr[1] : 0,
                     addr ? addr[2] : 0, addr ? addr[3] : 0,
                     addr ? addr[4] : 0, addr ? addr[5] : 0);

            /* Proactively initiate BLE encryption.  HID keyboards
             * require an encrypted link for notifications to flow.
             * Without this, CCCD writes during HIDH open may fail
             * with "insufficient authentication" and the keyboard
             * will never send input reports.
             *
             * If the link is already encrypted (re-connection with
             * stored bonding keys), this call returns immediately. */
            if (addr) {
                esp_bd_addr_t enc_addr;
                memcpy(enc_addr, addr, sizeof(esp_bd_addr_t));
                esp_err_t enc_err = esp_ble_set_encryption(
                    enc_addr, ESP_BLE_SEC_ENCRYPT_MITM);
                ESP_LOGI(TAG, "Encryption requested: %s",
                         esp_err_to_name(enc_err));
            }

            if (s_connect_cb) s_connect_cb(true);

            /* Proactively shorten the LE supervision timeout from the
             * FLOOR (32 s, needed during open) down to STEADY (~4 s)
             * so disconnects are detected quickly. We can only do this
             * if we have a peer-negotiated interval cached -- if the
             * UPDATE_CONN_PARAMS_EVT hasn't fired yet, the next one
             * will see s_open_in_flight=false and apply the same
             * clamp. Use addr from the device record (the controller
             * tracks the connection by BDA). */
            if (addr && s_cur_conn_int != 0) {
                esp_ble_conn_update_params_t up = {};
                memcpy(up.bda, addr, sizeof(esp_bd_addr_t));
                up.min_int = s_cur_conn_int;
                up.max_int = s_cur_conn_int;
                up.latency = s_cur_conn_lat;
                up.timeout = SUPERVISION_TIMEOUT_STEADY_10MS;
                esp_err_t uerr = request_conn_param_update(&up);
                if (uerr == ESP_ERR_INVALID_STATE) {
                    /* Another update is already in flight; the
                     * UPDATE_CONN_PARAMS_EVT handler will re-evaluate
                     * once it completes and issue the steady-timeout
                     * request itself. */
                    ESP_LOGI(TAG,
                             "Steady supervision timeout %u (10 ms): "
                             "deferred (update in flight)",
                             (unsigned)up.timeout);
                } else {
                    ESP_LOGI(TAG,
                             "Requesting steady supervision timeout %u "
                             "(10 ms): %s",
                             (unsigned)up.timeout, esp_err_to_name(uerr));
                }
            }
        } else {
            ESP_LOGE(TAG, "HID open failed: %d", param->open.status);
            notify_status("Connection failed, retrying...");
            s_connecting = false;
            /* Continue reconnection sequence after a short delay */
            reconn_timer_kick();
        }
        break;
    }

    case ESP_HIDH_CLOSE_EVENT:
        /* Disarm the open watchdog: a CLOSE here either follows a
         * successful OPEN (already stopped above) or terminates a
         * connect that never reached OPEN. */
        open_watchdog_stop();
        s_open_in_flight = false;
        /* Any pending Connection Parameter Update Request the
         * controller was waiting on us for is implicitly cancelled
         * by the link teardown; clear the gate so the next connection
         * starts clean. */
        s_conn_update_pending = false;
        s_connected  = false;
        s_connecting = false;
        s_hidh_dev   = NULL;
        s_battery_level = -1;
        memset(s_prev_keys, 0, sizeof(s_prev_keys));
        s_prev_key_count = 0;
        s_report_fmt = REPORT_FMT_UNKNOWN;
        memset(s_prev_report, 0, sizeof(s_prev_report));
        s_prev_report_len = 0;
        s_dev_name[0] = '\0';
        ESP_LOGI(TAG, "HID device disconnected, reconnecting...");
        if (s_connect_cb) s_connect_cb(false);
        /* Reset reconnection to start with last-known device, unless
         * the preceding OPEN_EVENT discarded a watchdog-flagged open
         * against this same address -- in that case skip RECONN_LAST
         * for one cycle and use a longer cool-off so the peer can
         * settle out of its transient post-pair state. */
        if (s_post_discard_cooloff) {
            s_post_discard_cooloff = false;
            s_reconn_phase = RECONN_KNOWN;
            s_reconn_idx   = 0;
            uint32_t prev = s_reconn_delay_ms;
            s_reconn_delay_ms = RECONN_SMP_BASE_MS;
            reconn_timer_kick();
            s_reconn_delay_ms = prev;
        } else {
            s_reconn_phase = RECONN_LAST;
            s_reconn_idx   = 0;
            /* Defer the next reconnect attempt instead of jumping straight
             * into start_reconnection() / esp_hidh_dev_open() here.
             * Rationale: when the user rapidly toggles the keyboard
             * off/on, a fresh open stacked on top of an in-flight teardown
             * was observed to land in the gattc_conn_cb half-built-record
             * window (rsn=0x8 panic). Letting the BLE stack quiesce for
             * RECONN_DEFAULT_MS first avoids that race. */
            reconn_timer_kick();
        }
        break;

    case ESP_HIDH_INPUT_EVENT: {
        ESP_LOGD(TAG, "INPUT: usage=%d, map=%d, id=%d, len=%d",
                 (int)param->input.usage,
                 (int)param->input.map_index,
                 (int)param->input.report_id,
                 (int)param->input.length);
        /* Keyboard report: standard boot protocol (8 bytes) or
         * NKRO bitmap (variable length, typically >8 bytes).
         * process_keyboard_report handles both formats. */
        if (param->input.usage == ESP_HID_USAGE_KEYBOARD &&
            param->input.length >= 3) {
            process_keyboard_report(param->input.data,
                                    (int)param->input.length,
                                    (uint8_t)param->input.report_id);
        }
        break;
    }

    case ESP_HIDH_BATTERY_EVENT:
        s_battery_level = param->battery.level;
        ESP_LOGI(TAG, "Battery: %d%%", param->battery.level);
        break;

    default:
        ESP_LOGI(TAG, "HIDH event: %d", (int)event);
        break;
    }
}

/* ---- Advertisement data parser ---- */

/* Check whether a 16-bit UUID list (AD type) contains the HID service UUID */
static bool adv_has_hid_uuid16(uint8_t *adv_data, uint8_t adv_len,
                                uint8_t ad_type)
{
    uint8_t field_len = 0;
    uint8_t *field = esp_ble_resolve_adv_data(adv_data, ad_type, &field_len);
    for (int i = 0; field && i + 1 < (int)field_len; i += 2) {
        uint16_t uuid = (uint16_t)field[i] | ((uint16_t)field[i + 1] << 8);
        if (uuid == BLE_SVC_HID_UUID16) return true;
    }
    return false;
}

/* Check whether a 128-bit UUID list (AD type) contains the HID service UUID */
static bool adv_has_hid_uuid128(uint8_t *adv_data, uint8_t adv_len,
                                 uint8_t ad_type)
{
    uint8_t field_len = 0;
    uint8_t *field = esp_ble_resolve_adv_data(adv_data, ad_type, &field_len);
    for (int i = 0; field && i + 15 < (int)field_len; i += 16) {
        if (memcmp(&field[i], BLE_SVC_HID_UUID128, 16) == 0) return true;
    }
    return false;
}

static bool adv_is_hid_keyboard(uint8_t *adv_data, uint8_t adv_len)
{
    uint8_t *field;
    uint8_t field_len;

    /* Check appearance for keyboard (0x03C1) or generic HID (0x03C0) */
    field = esp_ble_resolve_adv_data(adv_data,
                                     ESP_BLE_AD_TYPE_APPEARANCE, &field_len);
    if (field && field_len >= 2) {
        uint16_t appearance = (uint16_t)field[0] |
                              ((uint16_t)field[1] << 8);
        if (appearance == BLE_APPEARANCE_KEYBOARD ||
            appearance == BLE_APPEARANCE_HID_GENERIC) return true;
    }

    /* Check 16-bit service UUID lists (complete, incomplete, solicitation) */
    if (adv_has_hid_uuid16(adv_data, adv_len,
                           ESP_BLE_AD_TYPE_16SRV_CMPL)) return true;
    if (adv_has_hid_uuid16(adv_data, adv_len,
                           ESP_BLE_AD_TYPE_16SRV_PART)) return true;
    if (adv_has_hid_uuid16(adv_data, adv_len,
                           ESP_BLE_AD_TYPE_SOL_SRV_UUID)) return true;

    /* Check 128-bit service UUID lists (complete, incomplete) */
    if (adv_has_hid_uuid128(adv_data, adv_len,
                            ESP_BLE_AD_TYPE_128SRV_CMPL)) return true;
    if (adv_has_hid_uuid128(adv_data, adv_len,
                            ESP_BLE_AD_TYPE_128SRV_PART)) return true;

    return false;
}

/* ---- HIDH readiness check ---- */

/* HIDH is initialized during ble_init_task() before scanning starts.
 * This helper is kept so connect_task can verify init succeeded. */
static inline bool is_hidh_ready(void)
{
    return s_hidh_ready;
}

/* ---- Connect task (runs esp_hidh_dev_open which blocks) ---- */

/* Helpers: arm/disarm the open-watchdog timer (see s_open_watchdog_timer
 * doc comment near the top of the file). Tolerates being called before
 * the timer is created (early init paths). */
static void open_watchdog_start(uint32_t duration_ms)
{
    if (!s_open_watchdog_timer) return;
    /* Fresh attempt: clear any leftover force-close from a previous
     * stuck open and reset both timers. */
    s_force_close_on_open = false;
    s_open_watchdog_armed_ms = duration_ms;
    /* Adjust the period to the caller's chosen budget. xTimerChangePeriod
     * also starts the timer, so the xTimerReset below is just to make
     * the starting-from-zero behaviour explicit. */
    xTimerChangePeriod(s_open_watchdog_timer,
                       pdMS_TO_TICKS(duration_ms), 0);
    xTimerReset(s_open_watchdog_timer, 0);
    if (s_open_stuck_timer) {
        xTimerStop(s_open_stuck_timer, 0);
    }
}

static void open_watchdog_stop(void)
{
    if (s_open_watchdog_timer) {
        xTimerStop(s_open_watchdog_timer, 0);
    }
    if (s_open_stuck_timer) {
        xTimerStop(s_open_stuck_timer, 0);
    }
    s_force_close_on_open = false;
}

/* Recovery task: tears HIDH down and re-initialises it. Spawned by
 * open_stuck_cb() only when the watchdog has fired AND no HIDH event
 * has arrived for OPEN_STUCK_RECOVERY_MS afterwards. Runs in its own
 * task because esp_hidh_init() blocks on the BTC task and must not
 * run from a FreeRTOS timer service.
 *
 * Atomicity rules (learned the hard way from a bricked-HIDH session):
 *  - Do NOT clear s_hidh_ready until esp_hidh_deinit() actually
 *    succeeds. If deinit returns ESP_ERR_INVALID_STATE ("Please
 *    disconnect all devices first!"), HIDH is still up and
 *    functional -- just bail out and let the natural OPEN/CLOSE
 *    event drive the next attempt via the existing
 *    s_force_close_on_open path.
 *  - If esp_hidh_init() fails after a successful deinit, do not
 *    silently leave the host down. Surface the failure on the UI
 *    and re-arm s_open_stuck_timer so we try again later instead
 *    of bricking the keyboard subsystem until reboot.
 *  - Only kick the reconnect timer when HIDH is actually ready
 *    again. connect_task() also has a HIDH-down cooldown as a
 *    belt-and-braces backstop. */
static void hidh_recover_task(void *arg)
{
    (void)arg;
    ESP_LOGE(TAG, "HIDH appears wedged; deinit+init to recover");
    notify_status("BLE stuck, restarting HID host...");

    esp_err_t derr = esp_hidh_deinit();
    if (derr == ESP_ERR_INVALID_STATE) {
        /* HIDH still has a device record it can't tear down, which
         * means esp_hidh_dev_open() is still blocked inside service
         * discovery / CCCD writes. We've already waited
         * OPEN_STUCK_RECOVERY_MS past the watchdog, so all the
         * natural timeouts (SMP ~30 s, LE supervision
         * SUPERVISION_TIMEOUT_FLOOR_10MS = 32 s, HIDH internal
         * retries) have had a chance to fire and clearly will not.
         *
         * At this point the link IS fully up -- we got AUTH_CMPL
         * and UPDATE_CONN_PARAMS before the wedge -- so a clean
         * host-initiated esp_ble_gap_disconnect() is well defined
         * (controller produces a local-user 0x16 termination, not
         * a half-built-record rsn=0x8). That forces HIDH to surface
         * an OPEN/CLOSE event which the existing
         * s_force_close_on_open path will then clean up.
         *
         * If even that doesn't shake HIDH free, re-arm the stuck
         * timer so we try the whole sequence again instead of
         * giving up forever (which would leave BLE permanently
         * unable to reconnect until reboot). */
        ESP_LOGW(TAG, "esp_hidh_deinit: %s (HIDH still up); forcing "
                      "link down via gap_disconnect to unwedge open",
                 esp_err_to_name(derr));
        notify_status("BLE stuck, forcing disconnect...");
        s_force_close_on_open = true;  /* belt-and-braces */
        esp_err_t derr2 = esp_ble_gap_disconnect(s_target_bda);
        if (derr2 != ESP_OK) {
            ESP_LOGW(TAG, "esp_ble_gap_disconnect: %s",
                     esp_err_to_name(derr2));
        }
        if (s_open_stuck_timer) {
            xTimerChangePeriod(s_open_stuck_timer,
                               pdMS_TO_TICKS(OPEN_STUCK_RECOVERY_MS),
                               0);
            xTimerReset(s_open_stuck_timer, 0);
        }
        vTaskDelete(NULL);
        return;
    }
    if (derr != ESP_OK) {
        ESP_LOGW(TAG, "esp_hidh_deinit: %s (proceeding with re-init)",
                 esp_err_to_name(derr));
    }

    /* Deinit succeeded (or returned a non-INVALID_STATE error we are
     * choosing to ignore): HIDH is now down. Mark it so and drop
     * any stale device handle. */
    s_hidh_ready = false;
    s_hidh_dev   = NULL;
    s_connected  = false;
    s_connecting = false;
    s_force_close_on_open = false;

    esp_hidh_config_t hidh_cfg = {};
    hidh_cfg.callback = hidh_callback;
    hidh_cfg.event_stack_size = 4096;
    esp_err_t ierr = esp_hidh_init(&hidh_cfg);
    if (ierr != ESP_OK) {
        ESP_LOGE(TAG, "esp_hidh_init (recovery) failed: %s; will "
                      "retry in %d ms",
                 esp_err_to_name(ierr), OPEN_STUCK_RECOVERY_MS);
        notify_status("BLE HID restart failed, retrying...");
        /* Re-arm the stuck timer so we make another attempt later
         * instead of leaving s_hidh_ready=false forever. Set the
         * watchdog flag so open_stuck_cb()'s guard passes when it
         * fires next. */
        s_force_close_on_open = true;
        if (s_open_stuck_timer) {
            xTimerChangePeriod(s_open_stuck_timer,
                               pdMS_TO_TICKS(OPEN_STUCK_RECOVERY_MS),
                               0);
            xTimerReset(s_open_stuck_timer, 0);
        }
        vTaskDelete(NULL);
        return;
    }

    s_hidh_ready = true;
    ESP_LOGI(TAG, "HIDH re-initialised");

    /* Kick the reconnect state machine; skip RECONN_LAST so we don't
     * immediately re-open the same address that just wedged HIDH. */
    if (!s_disabled) {
        s_post_discard_cooloff = false;
        s_reconn_phase = RECONN_KNOWN;
        s_reconn_idx   = 0;
        uint32_t prev = s_reconn_delay_ms;
        s_reconn_delay_ms = RECONN_SMP_BASE_MS;
        reconn_timer_kick();
        s_reconn_delay_ms = prev;
    }
    vTaskDelete(NULL);
}

/* Stuck-recovery timer callback. Only spawns the recovery task if the
 * watchdog flag is still set (i.e. no HIDH event has arrived since
 * the watchdog fired). */
static void open_stuck_cb(TimerHandle_t timer)
{
    (void)timer;
    if (s_disabled) return;
    if (!s_force_close_on_open) return;     /* HIDH event arrived */
    if (s_connected) return;
    xTaskCreate(hidh_recover_task, "hidh_recover", 4096, NULL, 3, NULL);
}

/* Fires when esp_hidh_dev_open() has been outstanding longer than
 * OPEN_WATCHDOG_MS. We do NOT call esp_ble_gap_disconnect() here --
 * see the s_open_watchdog_timer comment near the top of the file
 * for why. Instead we set s_force_close_on_open and let the eventual
 * ESP_HIDH_OPEN_EVENT close the device cleanly on a fully-defined
 * device record. As a backstop we arm s_open_stuck_timer to restart
 * HIDH if no event arrives at all. */
static void open_watchdog_cb(TimerHandle_t timer)
{
    (void)timer;
    if (s_disabled) return;
    if (!s_connecting || s_connected) return;
    ESP_LOGW(TAG, "esp_hidh_dev_open watchdog fired after %u ms; "
             "will close device cleanly on OPEN_EVENT to avoid "
             "stuck-pairing crash",
             (unsigned)s_open_watchdog_armed_ms);
    notify_status("Pairing stuck, retrying...");
    s_force_close_on_open = true;
    /* Intentionally leave s_connecting set and do not start the
     * reconnect timer here: the eventual OPEN/CLOSE event is what
     * resets state. If neither arrives, s_open_stuck_timer recovers. */
    if (s_open_stuck_timer) {
        xTimerReset(s_open_stuck_timer, 0);
    }
}

/* Maximum number of open-connection retries.  esp_hidh_dev_open()
 * can fail transiently if the remote device is slow to respond.
 * A short retry loop handles this.  Keep this low (2) so that
 * re-pairing scenarios (where the old address is unreachable) do
 * not block for too long before scanning for the new address. */
#define CONNECT_RETRIES     1
#define CONNECT_RETRY_MS  250

static void connect_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "Connecting to keyboard \"%s\" "
             "(%02x:%02x:%02x:%02x:%02x:%02x)...",
             s_dev_name,
             s_target_bda[0], s_target_bda[1], s_target_bda[2],
             s_target_bda[3], s_target_bda[4], s_target_bda[5]);
    notify_status("Connecting to %s...", s_dev_name);

    /* HIDH was initialized during ble_init_task().  If it failed,
     * we cannot connect. */
    if (!is_hidh_ready()) {
        ESP_LOGE(TAG, "HIDH not available, cannot connect");
        notify_status("BLE HID restarting, please wait...");
        s_connecting = false;
        /* Apply a long cooldown before the next reconnect attempt.
         * hidh_recover_task is responsible for restoring the host;
         * spinning the reconnect timer at RECONN_DEFAULT_MS here
         * just floods the log with "HIDH not available" while
         * recovery is in progress (or has failed and re-armed
         * itself via s_open_stuck_timer). */
        uint32_t prev = s_reconn_delay_ms;
        s_reconn_delay_ms = HIDH_DOWN_RETRY_MS;
        reconn_timer_kick();
        s_reconn_delay_ms = prev;
        vTaskDelete(NULL);
        return;
    }

    esp_hidh_dev_t *dev = NULL;
    /* Mark the open as in flight BEFORE we ask the controller to
     * negotiate connection parameters. The UPDATE_CONN_PARAMS_EVT
     * handler keys off this flag to decide whether to pin the
     * supervision timeout to the FLOOR (during open) or relax it to
     * the STEADY target (after open). */
    s_open_in_flight = true;
    /* Clear any leftover in-flight conn-param-update gate from a prior
     * link that may have torn down before its UPDATE_CONN_PARAMS_EVT
     * fired. Without this a stale "pending" flag would silently
     * suppress the steady-supervision-timeout request on the new
     * connection. */
    s_conn_update_pending = false;
    /* Belt-and-braces: hint the controller to negotiate a generous
     * supervision timeout from the start. The keyboard may renegotiate
     * post-pairing, in which case the ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT
     * handler bumps it back up.
     *
     * min_int=24 (30 ms), max_int=40 (50 ms): typical HID keyboard
     * range; intentionally not aggressive so we do not regress power
     * if the peer accepts it as-is. latency=0, supervision_timeout =
     * SUPERVISION_TIMEOUT_FLOOR_10MS. */
    esp_err_t pref_err = esp_ble_gap_set_prefer_conn_params(
        s_target_bda,
        24, 40, 0,
        SUPERVISION_TIMEOUT_FLOOR_10MS);
    ESP_LOGI(TAG, "Preferred conn params set (sto=%u 10 ms): %s",
             (unsigned)SUPERVISION_TIMEOUT_FLOOR_10MS,
             esp_err_to_name(pref_err));

    /* Pick the open-watchdog budget. Reconnect to a device we
     * already have a bond for completes in well under 5 s on a
     * healthy link (cached services/CCCDs), so 25 s is purely
     * give-up budget there. Reserve the full 25 s only for the
     * first-pair path (scan -> connect to a never-seen address),
     * which is what OPEN_WATCHDOG_MS was originally sized for. */
    bool is_bonded_target = (bonded_find(s_target_bda) >= 0);
    uint32_t watchdog_ms  = is_bonded_target
                            ? OPEN_WATCHDOG_BONDED_MS
                            : OPEN_WATCHDOG_MS;

    for (int attempt = 0; attempt < CONNECT_RETRIES; attempt++) {
        /* Arm the open watchdog before the blocking call. On fire
         * the callback sets s_force_close_on_open; the OPEN_EVENT
         * handler then closes the device cleanly. OPEN/CLOSE
         * handlers stop the timer. */
        open_watchdog_start(watchdog_ms);
        dev = esp_hidh_dev_open(s_target_bda,
                                ESP_HID_TRANSPORT_BLE,
                                s_target_addr_type);
        if (dev) {
            /* Belt-and-braces: if the user disabled BLE while we
             * were blocked inside esp_hidh_dev_open(), close the
             * device here -- the OPEN_EVENT handler bails out early
             * when s_disabled is set, so nobody else will. The
             * watchdog-flagged case is handled inside the OPEN
             * handler (which runs on the HIDH event task and may
             * race with this return), so do nothing here for it. */
            if (s_disabled) {
                ESP_LOGW(TAG, "BLE disabled mid-open; closing device");
                esp_hidh_dev_close(dev);
                dev = NULL;
            }
            break;
        }
        /* Open returned NULL -- no OPEN_EVENT will fire for this
         * attempt, so disarm the watchdog ourselves. */
        open_watchdog_stop();
        ESP_LOGW(TAG, "esp_hidh_dev_open attempt %d/%d failed, "
                 "retrying in %d ms...",
                 attempt + 1, CONNECT_RETRIES, CONNECT_RETRY_MS);
        notify_status("Connect attempt %d/%d failed,\nretrying...",
                      attempt + 1, CONNECT_RETRIES);
        vTaskDelay(pdMS_TO_TICKS(CONNECT_RETRY_MS));
    }

    if (!dev) {
        ESP_LOGE(TAG, "esp_hidh_dev_open failed after %d attempts "
                 "(GATT client may not be registered)",
                 CONNECT_RETRIES);
        notify_status("Connection failed, retrying...");
        s_connecting = false;
        /* Open never produced a device -- no HIDH event will arrive
         * to clear s_open_in_flight, so do it here. */
        s_open_in_flight = false;
        /* Retry reconnection after a delay */
        reconn_timer_kick();
    }
    vTaskDelete(NULL);
}

/* ---- Reconnection to known devices ---- */

/* Try to directly connect to a known bonded device (no scan needed) */
static void try_connect_bonded(int idx)
{
    if (idx < 0 || idx >= s_bonded_count) return;
    s_connecting = true;
    memcpy(s_target_bda, s_bonded[idx].bda, 6);
    s_target_addr_type = s_bonded[idx].addr_type;
    /* Use stored device name, falling back to BDA if unknown */
    if (s_bonded[idx].name[0]) {
        strncpy(s_dev_name, s_bonded[idx].name, sizeof(s_dev_name) - 1);
        s_dev_name[sizeof(s_dev_name) - 1] = '\0';
    } else {
        snprintf(s_dev_name, sizeof(s_dev_name),
                 "%02x:%02x:%02x:%02x:%02x:%02x",
                 s_target_bda[0], s_target_bda[1], s_target_bda[2],
                 s_target_bda[3], s_target_bda[4], s_target_bda[5]);
    }
    ESP_LOGI(TAG, "Trying bonded device %d "
             "(%02x:%02x:%02x:%02x:%02x:%02x)...",
             idx,
             s_target_bda[0], s_target_bda[1], s_target_bda[2],
             s_target_bda[3], s_target_bda[4], s_target_bda[5]);
    xTaskCreate(connect_task, "ble_connect", CONNECT_TASK_STACK,
                NULL, 2, NULL);
}

static void start_reconnection(void)
{
    if (s_disabled) return;
    if (s_connected || s_connecting) return;

    /* If the last attempt's auth failure proves the bond is gone,
     * skip both RECONN_LAST and RECONN_KNOWN -- they will all fail
     * the same way and each costs ~25 s of open watchdog. Go
     * straight to a scan, which finds the keyboard in milliseconds
     * once it is advertising. */
    if (s_last_auth_bond_dead && s_reconn_phase != RECONN_SCAN) {
        ESP_LOGI(TAG, "Last auth failed (bond removed); "
                      "skipping bonded phases, scanning instead");
        s_reconn_phase = RECONN_SCAN;
    }

    switch (s_reconn_phase) {
    case RECONN_LAST:
        if (s_last_bonded >= 0 && s_last_bonded < s_bonded_count) {
            ESP_LOGI(TAG, "Reconnect phase: trying last-connected");
            int li = s_last_bonded;
            const char *lname = s_bonded[li].name[0]
                                ? s_bonded[li].name : "last keyboard";
            notify_status("Trying %s...", lname);
            s_reconn_phase = RECONN_KNOWN;
            s_reconn_idx   = 0;
            try_connect_bonded(s_last_bonded);
            return;
        }
        s_reconn_phase = RECONN_KNOWN;
        s_reconn_idx   = 0;
        /* fall through */

    case RECONN_KNOWN:
        while (s_reconn_idx < s_bonded_count) {
            int idx = s_reconn_idx++;
            if (idx == s_last_bonded) continue; /* already tried */
            ESP_LOGI(TAG, "Reconnect phase: trying known device %d", idx);
            const char *kname = s_bonded[idx].name[0]
                                ? s_bonded[idx].name : "known keyboard";
            notify_status("Trying %s...", kname);
            try_connect_bonded(idx);
            return;
        }
        s_reconn_phase = RECONN_SCAN;
        /* fall through */

    case RECONN_SCAN:
        ESP_LOGI(TAG, "Reconnect phase: scanning for new keyboards");
        notify_status("Scanning for keyboards...");
        /* Scanning will either re-establish a connection (which
         * resets bookkeeping naturally) or time out and re-enter the
         * full reconnect cycle via scan_timer_cb. Either way the
         * "skip bonded" flag has done its job. */
        s_last_auth_bond_dead = false;
        ble_keyboard_start_scan();
        break;
    }
}

/* ---- Scan retry timer callback ---- */

static void scan_timer_cb(TimerHandle_t timer)
{
    (void)timer;
    if (!s_connected && !s_connecting) {
        /* Reset reconnection to try all phases again */
        s_reconn_phase = RECONN_LAST;
        s_reconn_idx   = 0;
        start_reconnection();
    }
}

static void reconn_timer_cb(TimerHandle_t timer)
{
    (void)timer;
    start_reconnection();
}

/* Safety-net timer: fires periodically after init until scanning
 * starts or a connection is established.  Handles the (unlikely) case
 * where the initial set_scan_params or start_scanning fails silently. */
static void startup_timer_cb(TimerHandle_t timer)
{
    (void)timer;
    if (s_connected || s_connecting) {
        /* Goal achieved -- stop the periodic timer */
        xTimerStop(s_startup_timer, 0);
        return;
    }

    ESP_LOGW(TAG, "Safety-net timer fired (scan_params_ready=%d)",
             (int)s_scan_params_ready);

    /* Re-register our GAP callback as a precaution. */
    esp_ble_gap_register_callback(gap_event_handler);

    if (!s_scan_params_ready) {
        ESP_LOGW(TAG, "Scan params not ready; re-sending");
        esp_ble_scan_params_t scan_params;
        fill_scan_params(&scan_params);
        esp_err_t err = esp_ble_gap_set_scan_params(&scan_params);
        if (err == ESP_OK) {
            /* The call succeeded (queued to controller).  Force params
             * ready -- the completion event will arrive at our GAP
             * callback and confirm, but we proceed immediately to avoid
             * another 3-second wait. */
            s_scan_params_ready = true;
        } else {
            ESP_LOGE(TAG, "Re-set scan params failed: %s",
                     esp_err_to_name(err));
            /* Don't force ready; next timer tick will retry */
            return;
        }
    }

    s_reconn_phase = RECONN_LAST;
    s_reconn_idx   = 0;
    start_reconnection();
}

/* ---- BLE GAP callback ---- */

static void gap_event_handler(esp_gap_ble_cb_event_t event,
                               esp_ble_gap_cb_param_t *param)
{
    /* SCAN_PARAM_SET_COMPLETE_EVT must be processed even while
     * disabled so s_scan_params_ready latches and a later
     * ble_keyboard_enable() can start scanning without an extra
     * round-trip. All other events are suppressed when disabled. */
    if (s_disabled && event != ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT) {
        return;
    }
    ESP_LOGI(TAG, "GAP event: %d", (int)event);

    switch (event) {
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
        if (param->scan_param_cmpl.status == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(TAG, "Scan parameters set");
            s_scan_params_ready = true;
            /* Scan params configured via normal path -- stop safety-net */
            if (s_startup_timer) {
                xTimerStop(s_startup_timer, 0);
            }
            /* Now that scan params are ready, start the reconnection
             * sequence (which may trigger a scan). Skip while
             * disabled (USB keyboard active) -- ble_keyboard_enable()
             * will kick off scanning when input switches back to BLE. */
            if (!s_disabled) {
                start_reconnection();
            }
        } else {
            ESP_LOGE(TAG, "Scan param set failed: 0x%x",
                     param->scan_param_cmpl.status);
        }
        break;

    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
        if (param->scan_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
            notify_status("Scanning for keyboards...");
            ESP_LOGI(TAG, "Scanning for BLE HID keyboards...");
            /* Scanning started -- stop the periodic safety-net timer */
            if (s_startup_timer) {
                xTimerStop(s_startup_timer, 0);
            }
        } else {
            ESP_LOGE(TAG, "Scan start failed: 0x%x",
                     param->scan_start_cmpl.status);
        }
        break;

    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
        esp_ble_gap_cb_param_t::ble_scan_result_evt_param *scan =
            &param->scan_rst;

        if (scan->search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) {
            if (s_connected || s_connecting) break;

            uint8_t total_len = scan->adv_data_len + scan->scan_rsp_len;

            /* Extract device name for logging */
            char tmp_name[64] = "";
            uint8_t name_len = 0;
            uint8_t *name = esp_ble_resolve_adv_data(
                scan->ble_adv, ESP_BLE_AD_TYPE_NAME_CMPL, &name_len);
            if (!name) {
                name = esp_ble_resolve_adv_data(
                    scan->ble_adv, ESP_BLE_AD_TYPE_NAME_SHORT,
                    &name_len);
            }
            if (name && name_len > 0) {
                int len = (int)name_len;
                if (len > (int)sizeof(tmp_name) - 1)
                    len = (int)sizeof(tmp_name) - 1;
                memcpy(tmp_name, name, (size_t)len);
                tmp_name[len] = '\0';
            }

            if (adv_is_hid_keyboard(scan->ble_adv, total_len)) {
                if (tmp_name[0]) {
                    strncpy(s_dev_name, tmp_name, sizeof(s_dev_name) - 1);
                    s_dev_name[sizeof(s_dev_name) - 1] = '\0';
                }

                /* Prioritize already-bonded devices during scan */
                bool is_known = (bonded_find(scan->bda) >= 0);
                if (!is_known) {
                    ESP_LOGI(TAG, "New keyboard found: \"%s\" "
                             "(%02x:%02x:%02x:%02x:%02x:%02x)",
                             s_dev_name,
                             scan->bda[0], scan->bda[1], scan->bda[2],
                             scan->bda[3], scan->bda[4], scan->bda[5]);
                } else {
                    ESP_LOGI(TAG, "Known keyboard found: \"%s\" "
                             "(%02x:%02x:%02x:%02x:%02x:%02x)",
                             s_dev_name,
                             scan->bda[0], scan->bda[1], scan->bda[2],
                             scan->bda[3], scan->bda[4], scan->bda[5]);
                }

                /* Save target address and stop scanning */
                s_connecting = true;
                memcpy(s_target_bda, scan->bda, sizeof(esp_bd_addr_t));
                s_target_addr_type = scan->ble_addr_type;
                notify_status("Found: %s\nConnecting...", s_dev_name);
                esp_ble_gap_stop_scanning();
            } else {
                /* Log non-HID devices at DEBUG level for diagnostics */
                ESP_LOGD(TAG, "Skipping non-HID device: \"%s\" "
                         "(%02x:%02x:%02x:%02x:%02x:%02x) "
                         "adv=%d rsp=%d",
                         tmp_name,
                         scan->bda[0], scan->bda[1], scan->bda[2],
                         scan->bda[3], scan->bda[4], scan->bda[5],
                         scan->adv_data_len, scan->scan_rsp_len);
            }
        } else if (scan->search_evt == ESP_GAP_SEARCH_INQ_CMPL_EVT) {
            /* Scan complete -- retry after a delay if not connected */
            if (!s_connected && !s_connecting) {
                ESP_LOGI(TAG, "Scan complete, no keyboard found. "
                         "Retrying...");
                notify_status("No keyboard found.\nRetrying...");
                if (s_scan_timer) {
                    xTimerStart(s_scan_timer, 0);
                }
            }
        }
        break;
    }

    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        /* Scanning stopped -- initiate HID connection if target found */
        if (s_connecting) {
            xTaskCreate(connect_task, "ble_connect", CONNECT_TASK_STACK,
                        NULL, 2, NULL);
        }
        break;

    case ESP_GAP_BLE_SEC_REQ_EVT:
        ESP_LOGI(TAG, "Security request from peer");
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
        break;

    case ESP_GAP_BLE_KEY_EVT:
        ESP_LOGI(TAG, "BLE key exchange, type=%d",
                 (int)param->ble_security.ble_key.key_type);
        break;

    case ESP_GAP_BLE_PASSKEY_NOTIF_EVT: {
        /* We are DisplayOnly -- the stack generated a passkey for us
         * to show.  The user types this number on the keyboard. */
        uint32_t passkey = param->ble_security.key_notif.passkey;
        ESP_LOGI(TAG, "Passkey notification: %06lu",
                 (unsigned long)passkey);
        if (s_passkey_cb) {
            s_passkey_cb(passkey);
        }
        break;
    }

    case ESP_GAP_BLE_NC_REQ_EVT: {
        /* Numeric Comparison -- used by some keyboards with Secure
         * Connections.  Show the number on the display and auto-accept.
         * The user verifies the same number is shown on the keyboard. */
        uint32_t nc_num = param->ble_security.key_notif.passkey;
        ESP_LOGI(TAG, "Numeric comparison: %06lu",
                 (unsigned long)nc_num);
        if (s_passkey_cb) {
            s_passkey_cb(nc_num);
        }
        esp_ble_confirm_reply(param->ble_security.key_notif.bd_addr, true);
        break;
    }

    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT: {
        /* The peer (or our own preferred-params hint) negotiated a new
         * set of link parameters. The supervision timeout we want
         * depends on whether an HIDH open is still in flight:
         *
         *  - Open in flight (s_open_in_flight=true): pin to
         *    SUPERVISION_TIMEOUT_FLOOR_10MS (32 s). esp_hidh_dev_open()
         *    stays blocked through the entire service-discovery +
         *    CCCD-write handshake (observed up to ~16 s on a NuPhy
         *    Air60 V2 first-pair; see OPEN_WATCHDOG_MS). Most HID
         *    keyboards negotiate ~5-7 s, so if the peer drops
         *    mid-handshake the controller fires rsn=0x8 into
         *    gattc_conn_cb against a half-built device record and
         *    Bluedroid panics. The floor keeps that disconnect
         *    *outside* the vulnerable window.
         *
         *  - Open complete (s_open_in_flight=false): clamp to
         *    SUPERVISION_TIMEOUT_STEADY_10MS (~4 s) so a real
         *    disconnect (keyboard powered off, out of range) is
         *    detected in seconds instead of taking the full 32 s.
         *    The half-built record window is gone, so the floor is
         *    no longer load-bearing.
         *
         * We deliberately leave min_int / max_int / latency at what
         * the peer chose, to avoid regressing power or latency. */
        auto *p = &param->update_conn_params;
        /* Clear the in-flight gate before logging / re-evaluating so
         * the helper below will issue a fresh request if one is still
         * needed. A status=ESP_BT_STATUS_TIMEOUT (16) result here
         * means the peer never answered the previous request (this is
         * exactly what we are trying to stop sending in the first
         * place); clearing the gate either way is correct because the
         * controller is no longer waiting on us. */
        s_conn_update_pending = false;
        ESP_LOGI(TAG,
                 "Conn params updated: status=%d int=%u lat=%u sto=%u",
                 (int)p->status, (unsigned)p->conn_int,
                 (unsigned)p->latency, (unsigned)p->timeout);
        if (p->status == ESP_BT_STATUS_SUCCESS) {
            /* Cache the peer-negotiated interval/latency so the
             * OPEN_EVENT success path can request a steady-timeout
             * update without regressing them. */
            s_cur_conn_int = p->conn_int;
            s_cur_conn_lat = p->latency;
        }
        uint16_t target_to = s_open_in_flight
                             ? SUPERVISION_TIMEOUT_FLOOR_10MS
                             : SUPERVISION_TIMEOUT_STEADY_10MS;
        if (p->status == ESP_BT_STATUS_SUCCESS &&
            p->timeout != target_to) {
            /* p->min_int / p->max_int reflect the range that was just
             * accepted, so they are guaranteed valid as a pair (min
             * <= max). Re-submit them as-is to avoid regressing the
             * peer's chosen interval/latency; only the supervision
             * timeout is changed. */
            esp_ble_conn_update_params_t up = {};
            memcpy(up.bda, p->bda, sizeof(esp_bd_addr_t));
            up.min_int = p->min_int;
            up.max_int = p->max_int;
            up.latency = p->latency;
            up.timeout = target_to;
            esp_err_t uerr = request_conn_param_update(&up);
            if (uerr == ESP_ERR_INVALID_STATE) {
                /* Should not happen -- we just cleared the gate -- but
                 * keep the message distinct so it is obvious if a
                 * future caller adds a racing path. */
                ESP_LOGI(TAG,
                         "Supervision timeout %s to %u (10 ms): "
                         "deferred (update in flight)",
                         s_open_in_flight ? "bump" : "relax",
                         (unsigned)up.timeout);
            } else {
                ESP_LOGI(TAG,
                         "Requesting supervision timeout %s to %u "
                         "(10 ms): %s",
                         s_open_in_flight ? "bump" : "relax",
                         (unsigned)up.timeout, esp_err_to_name(uerr));
            }
        }
        break;
    }

    case ESP_GAP_BLE_AUTH_CMPL_EVT:
        if (param->ble_security.auth_cmpl.success) {
            ESP_LOGI(TAG, "BLE authentication success");
            /* Dismiss passkey overlay */
            if (s_passkey_cb) {
                s_passkey_cb(BLE_PASSKEY_DISMISS);
            }
            /* Clear any SMP-rate-limit backoff accumulated from
             * previous failed attempts. */
            s_smp_repeat_attempts = 0;
            s_reconn_delay_ms     = RECONN_DEFAULT_MS;
            s_last_auth_bond_dead = false;
            /* Encryption is now established.  On ESP-IDF Bluedroid the
             * HIDH library discovers services and writes CCCDs after
             * the BLE connection is up; with "Just Works" or fast
             * pairing the encryption is established before CCCD writes,
             * so they succeed on the first open.  No close-and-reopen
             * is needed -- it would waste GATT notification slots
             * (CONFIG_BT_GATTC_NOTIF_REG_MAX). */
        } else {
            uint8_t reason = param->ble_security.auth_cmpl.fail_reason;
            ESP_LOGE(TAG, "BLE authentication failed, reason: 0x%x",
                     reason);
            /* Any auth failure except plain rate-limit (0x61) means
             * Bluedroid has just discarded the bond for this address
             * (see BT_APPL: bta_dm_ble_smp_cback remove bond,rsn 99).
             * The remaining bonded entries either point at the same
             * physical keyboard (a stale resolvable address that
             * will fail the same way) or unrelated devices that the
             * user is not trying to connect to right now -- in
             * either case walking RECONN_KNOWN just wastes 25 s of
             * watchdog per stale address. Flag so start_reconnection
             * jumps straight to RECONN_SCAN on the next attempt. */
            if (reason != 0x61) {
                s_last_auth_bond_dead = true;
            }
            if (s_passkey_cb) {
                s_passkey_cb(BLE_PASSKEY_DISMISS);
            }
            /* Notify the UI of the (failed) connect attempt so that
             * apply_pending_connect_state() latches a single GC16
             * full refresh before the subsequent
             * notify_status("Connection failed, retrying...") and
             * any per-attempt status flicker repaints the BLE prompt
             * label. Without this, the back-to-back near-full GL16
             * partials issued while Bluedroid is still hammering the
             * bus post-auth-fail wedge epdiy's `epd_prep` feeders
             * (busy-spin at top priority on both cores, starves
             * IDLE0, trips the task watchdog). Idempotent w.r.t. a
             * later ESP_HIDH_CLOSE_EVENT, which also calls this. */
            if (s_connect_cb) {
                s_connect_cb(false);
            }
            /* SMP_REPEATED_ATTEMPTS (0x61): the peer is rate-limiting
             * pairing. Hammering it at the default 1 s interval just
             * keeps reproducing the failure (and the resulting
             * post-fail BLE-bus storm keeps stressing the EPD rail).
             * Back off exponentially up to RECONN_SMP_MAX_MS. Reset
             * on a successful auth above. */
            if (reason == 0x61) {
                if (s_smp_repeat_attempts < 30) {
                    s_smp_repeat_attempts++;
                }
                uint32_t delay_ms = RECONN_SMP_BASE_MS;
                for (int i = 1; i < s_smp_repeat_attempts; i++) {
                    delay_ms *= 2;
                    if (delay_ms >= RECONN_SMP_MAX_MS) {
                        delay_ms = RECONN_SMP_MAX_MS;
                        break;
                    }
                }
                s_reconn_delay_ms = delay_ms;
                notify_status("Pairing rejected,\n"
                              "retrying in %lu s...",
                              (unsigned long)(delay_ms / 1000));
            }
        }
        break;

    default:
        break;
    }
}

/* ---- BLE init task (runs in its own FreeRTOS task) ---- */

/* HIDH bring-up runs in a dedicated task because esp_hidh_init()
 * internally calls esp_ble_gattc_app_register() which blocks on a
 * semaphore waiting for the BTC task to confirm the registration.
 * Running this from app_main can deadlock because app_main sits at
 * priority 1 on core 0 and the BTC task may not get enough CPU
 * time to process the request while app_main is blocked.  A
 * dedicated task at priority 3 avoids this and also allows the
 * rest of app_main to proceed.
 *
 * The earlier steps (Classic mem release, BT controller init/enable
 * and Bluedroid init/enable) are now performed synchronously from
 * ble_keyboard_init() instead of from this task.  Doing them
 * synchronously is important on memory-constrained boards (notably
 * the M5Stack PaperS3, where the epdiy framebuffers leave only
 * ~191 KB of internal heap on boot): if Bluedroid bring-up races
 * with esp_wifi_init() in main(), WiFi can grab enough internal RAM
 * that BTU_StartUp later fails to allocate its workqueue and the
 * device asserts and reboots in a loop. */

static void ble_init_task(void *arg)
{
    (void)arg;

    /* Initialize HIDH (HID Host).  The GATTC callback must already
     * be registered (in ble_keyboard_init) so that esp_ble_hidh_init
     * can receive the ESP_GATTC_REG_EVT and unblock. */
    {
        esp_hidh_config_t hidh_cfg = {};
        hidh_cfg.callback = hidh_callback;
        hidh_cfg.event_stack_size = 4096;
        ESP_LOGI(TAG, "Initializing HIDH...");
        esp_err_t hidh_err = esp_hidh_init(&hidh_cfg);
        if (hidh_err != ESP_OK) {
            ESP_LOGE(TAG, "esp_hidh_init failed: %s",
                     esp_err_to_name(hidh_err));
        } else {
            s_hidh_ready = true;
            ESP_LOGI(TAG, "HIDH initialized");
        }
    }

    /* Register our GAP callback AFTER HIDH init so it is not
     * overwritten by esp_hidh_init()'s internal registration. */
    esp_err_t gap_err = esp_ble_gap_register_callback(gap_event_handler);
    if (gap_err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ble_gap_register_callback failed: %s",
                 esp_err_to_name(gap_err));
    } else {
        ESP_LOGI(TAG, "GAP callback registered");
    }

    /* Set BLE security parameters.
     * IO capability = DisplayOnly so the stack generates a random
     * 6-digit passkey for each pairing attempt and delivers it via
     * ESP_GAP_BLE_PASSKEY_NOTIF_EVT.  The user types this number on
     * the keyboard to complete pairing.  MITM flag is set so the
     * stack actually uses the passkey entry protocol.
     *
     * These are set AFTER HIDH init and GAP callback registration
     * to keep the BTC queue empty during HIDH's synchronous GATTC
     * registration. */
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;
    esp_ble_io_cap_t io_cap = ESP_IO_CAP_OUT;
    uint8_t key_size = 16;
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key  = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;

    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE,
                                   &auth_req, sizeof(auth_req));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE,
                                   &io_cap, sizeof(io_cap));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE,
                                   &key_size, sizeof(key_size));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY,
                                   &init_key, sizeof(init_key));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY,
                                   &rsp_key, sizeof(rsp_key));
    ESP_LOGI(TAG, "Security parameters configured");

    /* Set scan parameters -- triggers async
     * SCAN_PARAM_SET_COMPLETE_EVT which starts the reconnection /
     * scan sequence. */
    esp_ble_scan_params_t scan_params;
    fill_scan_params(&scan_params);
    esp_err_t sp_err = esp_ble_gap_set_scan_params(&scan_params);
    if (sp_err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ble_gap_set_scan_params failed: %s",
                 esp_err_to_name(sp_err));
    } else {
        ESP_LOGI(TAG, "Scan params sent to controller");
    }

    /* Create scan retry timer (one-shot, 2-second delay) */
    s_scan_timer = xTimerCreate("scan_retry", pdMS_TO_TICKS(2000),
                                pdFALSE, NULL, scan_timer_cb);

    /* Create reconnection timer (one-shot, 1-second delay) */
    s_reconn_timer = xTimerCreate("reconn", pdMS_TO_TICKS(1000),
                                  pdFALSE, NULL, reconn_timer_cb);

    /* Create the open-watchdog timer (one-shot). See
     * s_open_watchdog_timer doc comment near the top of this file. */
    s_open_watchdog_timer = xTimerCreate("ble_open_wd",
                                         pdMS_TO_TICKS(OPEN_WATCHDOG_MS),
                                         pdFALSE, NULL, open_watchdog_cb);
    /* Secondary "stuck" timer: armed by the watchdog if no HIDH event
     * arrives. Triggers a HIDH deinit/init via a recovery task. */
    s_open_stuck_timer    = xTimerCreate("ble_open_stuck",
                                         pdMS_TO_TICKS(OPEN_STUCK_RECOVERY_MS),
                                         pdFALSE, NULL, open_stuck_cb);

    /* Reconnection will be started from the SCAN_PARAM_SET_COMPLETE_EVT
     * callback once scan parameters are ready. */
    s_reconn_phase = RECONN_LAST;
    s_reconn_idx   = 0;

    /* Start a periodic safety-net timer.  Retries scanning if the
     * initial SCAN_PARAM_SET_COMPLETE_EVT is somehow missed. */
    s_startup_timer = xTimerCreate("ble_start",
                                   pdMS_TO_TICKS(STARTUP_SAFETY_TIMER_MS),
                                   pdTRUE, NULL, startup_timer_cb);
    if (s_startup_timer) {
        xTimerStart(s_startup_timer, 0);
    }

    ESP_LOGI(TAG, "BLE keyboard host initialized");
    vTaskDelete(NULL);
}

/* ---- Public API ---- */

extern "C" void ble_keyboard_init(void)
{
    /* Synchronous bring-up of the BT controller and Bluedroid stack.
     *
     * These steps are deliberately performed in the caller's
     * context (not in ble_init_task) so that ble_keyboard_init()
     * does not return until the Bluedroid workqueues and timers
     * have been allocated.  This guarantees that any subsequent
     * subsystem the caller brings up cannot starve Bluedroid of
     * internal RAM mid-bring-up.  On boards with little internal
     * heap (e.g. M5Stack PaperS3) the previous fully-async layout
     * caused BTU_StartUp to fail allocating its workqueue and the
     * device rebooted in a loop. */

    /* Load bonded device list from NVS */
    bonded_load();

#if defined(CONFIG_BT_CONTROLLER_DISABLED)
    /* Hosted-BT path (ESP32-P4 / M5Stack Tab5).
     *
     * The P4 has no on-chip BT controller; the BLE controller lives
     * on the on-board ESP32-C6 and is reached over ESP-Hosted's SDIO
     * transport. CONFIG_BT_CONTROLLER_DISABLED tells ESP-IDF to omit
     * the native esp_bt_controller_* code, so we must:
     *   1. bring up the hosted BT controller via the C6;
     *   2. open the hosted-VHCI HCI driver;
     *   3. attach it to Bluedroid as its HCI transport;
     * before the standard esp_bluedroid_init/enable calls below.
     *
     * The actual SDIO transport (esp_hosted_init +
     * esp_hosted_connect_to_slave) is already up: it is brought up
     * in main.cpp ahead of ble_keyboard_init() so that any failure
     * is logged once with full context rather than aborting the BLE
     * stack here.
     *
     * Mirrors examples/host_bluedroid_ble_compatibility_test/main
     * from espressif/esp-hosted-mcu. */
    ESP_ERROR_CHECK(esp_hosted_bt_controller_init());
    ESP_ERROR_CHECK(esp_hosted_bt_controller_enable());
    ESP_LOGI(TAG, "Hosted BT controller enabled (BLE on C6)");

    hosted_hci_bluedroid_open();
    esp_bluedroid_hci_driver_operations_t hci_ops = {
        .send                   = hosted_hci_bluedroid_send,
        .check_send_available   = hosted_hci_bluedroid_check_send_available,
        .register_host_callback = hosted_hci_bluedroid_register_host_callback,
    };
    ESP_ERROR_CHECK(esp_bluedroid_attach_hci_driver(&hci_ops));
    ESP_LOGI(TAG, "Hosted VHCI attached to Bluedroid");
#else
    /* Native BT controller path (ESP32-S3). */

    /* Release Classic BT memory (ESP32-S3 is BLE-only) */
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    ESP_LOGI(TAG, "Classic BT memory released");

    /* Initialize BT controller in BLE mode */
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
    ESP_LOGI(TAG, "BT controller enabled (BLE mode)");
#endif /* CONFIG_BT_CONTROLLER_DISABLED */

    /* Initialize Bluedroid */
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());
    ESP_LOGI(TAG, "Bluedroid enabled");

    /* Register the GATTC callback BEFORE esp_hidh_init().
     *
     * esp_ble_hidh_init() (called internally by esp_hidh_init)
     * calls esp_ble_gattc_app_register() and then waits on a
     * semaphore for the ESP_GATTC_REG_EVT callback.  However,
     * esp_ble_hidh_init() does NOT register the GATTC callback
     * itself -- it relies on the caller to attach
     * esp_hidh_gattc_event_handler via
     * esp_ble_gattc_register_callback() beforehand.
     *
     * Without this registration, the GATTC registration event is
     * never delivered, the semaphore is never signalled, and
     * esp_hidh_init() blocks forever.
     *
     * See esp_hidh_gattc.h and the official ESP-IDF HID host
     * example (examples/bluetooth/esp_hid_host). */
    ESP_ERROR_CHECK(
        esp_ble_gattc_register_callback(esp_hidh_gattc_event_handler));
    ESP_LOGI(TAG, "GATTC callback registered");

    /* Spawn ble_init_task to handle esp_hidh_init() and the
     * subsequent scan/security setup asynchronously.  HIDH init
     * itself blocks on a semaphore that the BTC task signals, so
     * it must not run on the priority-1 main task. */
    xTaskCreate(ble_init_task, "ble_init", BLE_INIT_TASK_STACK,
                NULL, 3, NULL);
}

extern "C" void ble_keyboard_set_callback(kb_event_callback_t callback)
{
    s_callback = callback;
}

extern "C" void ble_keyboard_disable(void)
{
    /* Idempotent. Reversible via ble_keyboard_enable(): the public
     * callback pointers are preserved so the editor keeps getting
     * key events when BLE comes back online after a USB keyboard
     * is unplugged. */
    if (s_disabled) return;
    s_disabled = true;

    ESP_LOGI(TAG, "Disabling BLE keyboard (USB keyboard active)");

    /* Tell the UI that the BLE link is no longer active. The
     * connect callback pointer is intentionally NOT cleared so a
     * later ble_keyboard_enable() + reconnect can fire it again. */
    if (s_connect_cb) {
        s_connect_cb(false);
    }

    /* Stop our retry / safety timers so they cannot kick a fresh
     * scan after we have torn everything down below. */
    if (s_scan_timer)          { xTimerStop(s_scan_timer,          0); }
    if (s_reconn_timer)        { xTimerStop(s_reconn_timer,        0); }
    if (s_startup_timer)       { xTimerStop(s_startup_timer,       0); }
    if (s_open_watchdog_timer) { xTimerStop(s_open_watchdog_timer, 0); }
    if (s_open_stuck_timer)    { xTimerStop(s_open_stuck_timer,    0); }
    s_force_close_on_open = false;

    /* Tell the controller to stop scanning. Returns an error if no
     * scan is in flight, which we ignore. */
    esp_ble_gap_stop_scanning();

    /* If a keyboard is currently connected, close the HIDH device
     * so the controller link to the peripheral drops cleanly. */
    if (s_hidh_dev) {
        esp_hidh_dev_close(s_hidh_dev);
        s_hidh_dev = NULL;
    }

    s_connected         = false;
    s_connecting        = false;
    s_battery_level     = -1;
    /* s_scan_params_ready stays true: the controller's scan-param
     * state is preserved across stop_scanning, so when we re-enable
     * we can call esp_ble_gap_start_scanning() directly without
     * waiting for another SCAN_PARAM_SET_COMPLETE_EVT. */
}

extern "C" void ble_keyboard_enable(void)
{
    if (!s_disabled) return;
    ESP_LOGI(TAG, "Re-enabling BLE keyboard (USB keyboard gone)");
    s_disabled = false;
    /* Reset reconnection bookkeeping so we walk through the bonded
     * device list from the top, then fall back to a full scan. */
    s_reconn_phase = RECONN_LAST;
    s_reconn_idx   = 0;
    notify_status("Searching for BLE keyboard...");
    /* If scan params are already known to the controller, restart
     * the reconnection sequence immediately. Otherwise the
     * SCAN_PARAM_SET_COMPLETE_EVT handler will pick up s_disabled =
     * false and call start_reconnection() itself as soon as
     * ble_init_task() finishes setting the scan parameters. */
    if (s_scan_params_ready) {
        start_reconnection();
    }
}

/* Erase every stored keyboard bond and start scanning for a new
 * keyboard from scratch.  Removes all bonded device entries from
 * the Bluedroid stack and from the NVS namespace, disconnects any
 * currently-connected keyboard, resets the reconnection state
 * machine to skip the bonded phases, and immediately starts a
 * fresh scan.
 *
 * This is the "forget all keyboards" action: after this call the
 * device behaves as if it has never paired with any keyboard. */
extern "C" void ble_keyboard_forget_all(void)
{
    ESP_LOGI(TAG, "Forgetting all bonded keyboards");

    /* Stop all pending retry timers so they cannot restart old
     * reconnection attempts while we are clearing state. */
    if (s_scan_timer)          { xTimerStop(s_scan_timer,          0); }
    if (s_reconn_timer)        { xTimerStop(s_reconn_timer,        0); }
    if (s_open_watchdog_timer) { xTimerStop(s_open_watchdog_timer, 0); }
    if (s_open_stuck_timer)    { xTimerStop(s_open_stuck_timer,    0); }

    /* Disconnect the current keyboard if connected. */
    if (s_hidh_dev) {
        esp_hidh_dev_close(s_hidh_dev);
        s_hidh_dev = NULL;
    }
    s_connected  = false;
    s_connecting = false;
    s_battery_level = -1;
    s_force_close_on_open = false;

    /* Notify the UI that the keyboard disconnected before we
     * announce scanning, so status transitions are coherent. */
    if (s_connect_cb) {
        s_connect_cb(false);
    }

    /* Remove all bonds from the Bluedroid stack so the controller
     * will not attempt encrypted reconnects to stale devices. */
    for (int i = 0; i < s_bonded_count; i++) {
        esp_ble_remove_bond_device(s_bonded[i].bda);
    }

    /* Clear the in-memory bond list. */
    s_bonded_count = 0;
    s_last_bonded  = -1;
    s_dev_name[0]  = '\0';

    /* Erase the persisted bond list from NVS. */
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h) == ESP_OK) {
        nvs_erase_all(h);
        nvs_commit(h);
        nvs_close(h);
    }

    /* Reset all reconnection bookkeeping and go straight to a
     * scan (no bonded phases to try). */
    s_reconn_phase        = RECONN_SCAN;
    s_reconn_idx          = 0;
    s_reconn_delay_ms     = RECONN_DEFAULT_MS;
    s_smp_repeat_attempts = 0;
    s_last_auth_bond_dead = false;

    notify_status("Searching for new keyboard...");

    if (s_scan_params_ready) {
        ble_keyboard_start_scan();
    }
    /* If scan params are not yet ready (very early in init), the
     * SCAN_PARAM_SET_COMPLETE_EVT handler will start the scan once
     * the controller is ready, as usual. */
}

extern "C" void ble_keyboard_set_passkey_callback(ble_passkey_cb_t cb)
{
    s_passkey_cb = cb;
}

extern "C" void ble_keyboard_set_connect_callback(ble_connect_cb_t cb)
{
    s_connect_cb = cb;
}

extern "C" void ble_keyboard_set_status_text_callback(ble_status_text_cb_t cb)
{
    s_status_text_cb = cb;
}

extern "C" bool ble_keyboard_is_connected(void)
{
    if (s_disabled) return false;
    return s_connected;
}

extern "C" void ble_keyboard_start_scan(void)
{
    if (s_disabled) return;
    if (!s_connected && !s_connecting) {
        if (!s_scan_params_ready) {
            ESP_LOGW(TAG, "Scan params not ready yet, deferring scan");
            return;
        }
        esp_err_t err = esp_ble_gap_start_scanning(SCAN_DURATION_SEC);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ble_gap_start_scanning failed: %s",
                     esp_err_to_name(err));
        } else {
            ESP_LOGI(TAG, "Scan started (%d s)", SCAN_DURATION_SEC);
        }
    }
}

extern "C" const char *ble_keyboard_get_device_name(void)
{
    return s_dev_name;
}

extern "C" int ble_keyboard_get_battery_level(void)
{
    return s_battery_level;
}

#endif /* CONFIG_BT_BLUEDROID_ENABLED */
