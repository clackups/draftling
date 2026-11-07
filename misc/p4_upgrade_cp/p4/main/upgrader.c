/*
 * Installs the embedded esp-hosted image on the board's co-processor
 * (the Tab5's ESP32-C6, "the C6" below), whatever esp-hosted
 * generation it runs now.
 *
 * 1. Power the C6, reset it, bring the SDIO card up. Bus settings are
 *    tried from fast to slow (4-bit 20 MHz down to 1-bit 400 kHz).
 * 2. Open the data path (host interrupt 0) and wait for the INIT event:
 *    its interface number says MCU or Linux FG numbering, its TLVs the
 *    firmware version (2.x+) and RPC version (3.x). Without a version
 *    TLV (1.x), the GetCoprocessorFwVersion RPC gives it.
 * 3. Probe the RPC dialects with GetMACAddress, the expected one first.
 * 4. OTA: begin, write the image in 1 KB chunks, end. 2.6+ firmware
 *    then needs OTAActivate to switch the boot partition; 1.x and FG
 *    firmware do that in OTAEnd and reboot on their own.
 * 5. Reset the C6 and reconnect: the INIT event must now report the
 *    new version.
 *
 * OTA writes the slot the C6 is not running from, and the boot slot
 * only changes after the image is validated, so a failure at any step
 * leaves the old firmware in place.
 */
#include "upgrader.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board.h"
#include "cp_sdio.h"
#include "hosted_proto.h"
#include "ui.h"

static const char *TAG = "upgrader";

extern const uint8_t cp_image_start[] asm("_binary_cp_hosted_bin_start");
extern const uint8_t cp_image_end[] asm("_binary_cp_hosted_bin_end");

#define RX_CAP          (32 * 1024)
#define SERIAL_CAP      4096
#define OTA_CHUNK       1024
#define CP_MAX_IMAGE    0x180000      /* smallest C6 OTA slot (1.x table) */

typedef struct {
    int width;
    int freq_khz;
} link_cfg_t;

static const link_cfg_t LINK_CFGS[] = {
    { 4, 20000 },
    { 4, 10000 },
    { 1, 10000 },
    { 1, 400 },
};

static uint8_t *s_rx;
static uint8_t *s_tx;
static uint8_t s_serial[SERIAL_CAP];
static size_t s_serial_len;
static uint16_t s_seq;
static uint32_t s_uid = 1;
static hp_init_info_t s_init;
/* C6 firmware version: from the INIT event (2.x+) or the
 * GetCoprocessorFwVersion RPC (every MCU generation); 0 = unknown. */
static uint32_t s_fw_version;
/* Submessage of the last RPC response (valid until the next read). */
static const uint8_t *s_last_sub;
static size_t s_last_sub_len;

/* Leftover bytes of a partial read (a frame cut by the buffer end). */
static size_t s_rx_keep;

/* The RPC response being waited for (msg_id), and the copy of it once
 * it arrived. Matching happens as frames are read, so other serial
 * messages in the same read (events) cannot displace it. */
static uint32_t s_want_id;
typedef struct {
    uint8_t data[SERIAL_CAP];
    size_t len;
    bool ready;
} serial_msg_t;
static serial_msg_t s_msg;

static uint32_t version_val(int maj, int min, int patch)
{
    return ((uint32_t)maj << 16) | ((uint32_t)min << 8) | (uint32_t)patch;
}

static void version_str(uint32_t v, char *buf, size_t len)
{
    snprintf(buf, len, "%u.%u.%u", (unsigned)((v >> 16) & 0xFF),
             (unsigned)((v >> 8) & 0xFF), (unsigned)(v & 0xFF));
}

/* Handles one received frame. */
static void handle_frame(const hp_frame_t *f)
{
    /* Priv interface: 5 on MCU numbering, 4 on FG numbering. */
    if ((f->if_type == 5 || f->if_type == 4) && f->len >= 2 &&
        (f->payload[0] == HP_PRIV_EVENT_INIT || f->payload[0] == 0x00)) {
        hp_init_info_t info = {0};
        if (hp_parse_init_event(f->payload, f->len, &info)) {
            info.fg_numbering = (f->if_type == 4);
            s_init = info;
            ESP_LOGI(TAG, "INIT event on if %u: caps 0x%02x chip %d fw 0x%06" PRIx32
                     " rpc %d", f->if_type, info.capabilities, info.chip_id,
                     info.fw_version, info.rpc_version);
            return;
        }
    }
    /* Serial interface: 3 on MCU numbering, 2 on FG numbering. */
    if (f->if_type == 3 || f->if_type == 2) {
        if (s_serial_len + f->len > sizeof(s_serial)) {
            ESP_LOGW(TAG, "serial message too long, dropped");
            s_serial_len = 0;
            return;
        }
        memcpy(s_serial + s_serial_len, f->payload, f->len);
        s_serial_len += f->len;
        if (f->flags & HP_FLAG_MORE_FRAGMENT) return;

        const uint8_t *data;
        size_t data_len;
        char ep[16];
        hp_rpc_msg_t msg;
        if (hp_serial_unwrap(s_serial, s_serial_len, ep, sizeof(ep), &data, &data_len) &&
            data_len <= sizeof(s_msg.data) && hp_rpc_parse(data, data_len, &msg)) {
            if (msg.msg_type == 2 && s_want_id && msg.msg_id == s_want_id && !s_msg.ready) {
                memcpy(s_msg.data, data, data_len);
                s_msg.len = data_len;
                s_msg.ready = true;
            } else {
                ESP_LOGD(TAG, "serial message on '%s': type %" PRIu32 " id 0x%" PRIx32
                         " (ignored)", ep, msg.msg_type, msg.msg_id);
            }
        }
        s_serial_len = 0;
    }
}

/* Reads once from the C6 (waiting up to timeout_ms) and handles every
 * frame in the data. */
static esp_err_t pump(uint32_t timeout_ms)
{
    size_t got = 0;
    esp_err_t err = cp_sdio_recv(s_rx + s_rx_keep, RX_CAP - s_rx_keep, &got, timeout_ms);
    if (err != ESP_OK) return err;
    size_t avail = s_rx_keep + got;
    size_t pos = 0;
    while (pos < avail) {
        hp_frame_t f;
        int r = hp_frame_parse(s_rx + pos, avail - pos, &f);
        if (r == 0) {
            pos = avail;            /* padding */
            break;
        }
        if (r < 0) {
            /* A frame cut by the end of the buffer: keep it for the
             * next read. Anything else is garbage: drop the rest. */
            if (avail - pos < CP_SDIO_MAX_FRAME && got == RX_CAP - s_rx_keep) break;
            ESP_LOGW(TAG, "invalid frame data (%u bytes) dropped", (unsigned)(avail - pos));
            pos = avail;
            break;
        }
        handle_frame(&f);
        pos += f.total;
    }
    s_rx_keep = avail - pos;
    if (s_rx_keep) memmove(s_rx, s_rx + pos, s_rx_keep);
    return ESP_OK;
}

static esp_err_t send_frame(uint8_t if_type, uint8_t pkt_type, const uint8_t *payload,
                            uint16_t len)
{
    if ((size_t)len + HP_HDR_V1_SIZE > CP_SDIO_MAX_FRAME) return ESP_ERR_INVALID_SIZE;
    size_t n = hp_frame_build_v1(s_tx, if_type, pkt_type, payload, len, s_seq++);
    return cp_sdio_send(s_tx, n, 2000);
}

/* Resets the C6, brings the card up with `cfg` and opens the data path;
 * waits up to init_wait_ms for the INIT event. */
static esp_err_t link_up(const link_cfg_t *cfg, uint32_t init_wait_ms)
{
    memset(&s_init, 0, sizeof(s_init));
    s_fw_version = 0;
    s_serial_len = 0;
    s_rx_keep = 0;
    s_msg.ready = false;

    cp_reset();
    esp_err_t err = cp_sdio_host_init(cfg->width);
    if (err != ESP_OK) return err;
    err = cp_sdio_card_init(cfg->freq_khz, 4000);
    if (err != ESP_OK) return err;

    err = cp_sdio_send_intr(CP_INTR_OPEN_DATA_PATH);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "open data path interrupt failed: %s", esp_err_to_name(err));
        return err;
    }
    int64_t deadline = esp_timer_get_time() + (int64_t)init_wait_ms * 1000;
    while (!s_init.seen && esp_timer_get_time() < deadline) {
        err = pump(200);
        if (err != ESP_OK && err != ESP_ERR_TIMEOUT) {
            ESP_LOGW(TAG, "read failed: %s", esp_err_to_name(err));
            return err;
        }
    }
    if (s_init.seen && !s_init.fg_numbering) {
        /* What every MCU host answers; informational for the C6. */
        uint8_t cfgbuf[16];
        size_t n = hp_build_host_config(cfgbuf, sizeof(cfgbuf), s_init.chip_id);
        send_frame(5, HP_PRIV_PKT_TYPE_EVENT, cfgbuf, (uint16_t)n);
    }
    return ESP_OK;
}

/* Sends an RPC request and waits for its response. *code is the
 * response's `resp` field (0 when absent: protobuf default). */
static esp_err_t rpc_call(const hp_dialect_t *d, uint32_t req_id,
                          const uint8_t *sub, size_t sub_len, uint32_t timeout_ms,
                          uint32_t resp_field, int32_t *code)
{
    static uint8_t rpc_buf[OTA_CHUNK + 64];
    static uint8_t ser_buf[OTA_CHUNK + 96];
    uint32_t uid = s_uid++;

    size_t n = hp_rpc_build(rpc_buf, sizeof(rpc_buf), req_id, uid, sub, sub_len);
    size_t m = n ? hp_serial_wrap(ser_buf, sizeof(ser_buf), d->req_ep, rpc_buf, n) : 0;
    if (!m) return ESP_ERR_INVALID_SIZE;

    s_msg.ready = false;
    s_want_id = req_id + d->resp_offset;
    esp_err_t err = send_frame(d->serial_if, 0, ser_buf, (uint16_t)m);
    if (err != ESP_OK) {
        s_want_id = 0;
        return err;
    }

    int64_t deadline = esp_timer_get_time() + (int64_t)timeout_ms * 1000;
    while (!s_msg.ready && esp_timer_get_time() < deadline) {
        err = pump(100);
        if (err != ESP_OK && err != ESP_ERR_TIMEOUT) {
            s_want_id = 0;
            return err;
        }
    }
    s_want_id = 0;
    if (!s_msg.ready) return ESP_ERR_TIMEOUT;

    hp_rpc_msg_t msg;
    hp_rpc_parse(s_msg.data, s_msg.len, &msg);
    int32_t c = 0;
    if (msg.sub) hp_sub_get_int(msg.sub, msg.sub_len, resp_field, &c);
    *code = c;
    s_last_sub = msg.sub;
    s_last_sub_len = msg.sub_len;
    return ESP_OK;
}

/* GetMACAddress(STA): true if the C6 answered in this dialect. */
static bool probe(const hp_dialect_t *d)
{
    uint8_t sub[8];
    size_t n = hp_sub_varint_field(sub, 1, 1);
    for (int attempt = 0; attempt < 2; attempt++) {
        int32_t code;
        if (rpc_call(d, d->id_get_mac, sub, n, 1500, 2, &code) == ESP_OK) return true;
    }
    return false;
}

/* Firmware version from the RPC (0 if the dialect has none). */
static uint32_t query_fw_version(const hp_dialect_t *d)
{
    if (!d->id_get_fw_version) return 0;
    int32_t code = 0;
    if (rpc_call(d, d->id_get_fw_version, NULL, 0, 2000, 1, &code) != ESP_OK || code != 0) {
        return 0;
    }
    int32_t maj = 0, min = 0, patch = 0;
    hp_sub_get_int(s_last_sub, s_last_sub_len, 2, &maj);
    hp_sub_get_int(s_last_sub, s_last_sub_len, 3, &min);
    hp_sub_get_int(s_last_sub, s_last_sub_len, 4, &patch);
    if (!maj && !min && !patch) return 0;
    return version_val(maj, min, patch);
}

static const hp_dialect_t *connect(const link_cfg_t **cfg_out)
{
    for (size_t i = 0; i < sizeof(LINK_CFGS) / sizeof(LINK_CFGS[0]); i++) {
        const link_cfg_t *cfg = &LINK_CFGS[i];
        ui_log("Trying SDIO %d-bit at %d kHz", cfg->width, cfg->freq_khz);
        if (link_up(cfg, 4000) != ESP_OK) {
            ui_log("  no SDIO card answer");
            continue;
        }
        if (s_init.seen) {
            ui_log("  " BOARD_CP_SHORT " sent its start-up message (%s numbering)",
                   s_init.fg_numbering ? "Linux FG" : "MCU");
        } else {
            ui_log("  no start-up message; probing RPC directly");
        }
        const hp_dialect_t *order[2];
        if (s_init.seen && s_init.fg_numbering) {
            order[0] = &HP_DIALECT_FG;
            order[1] = &HP_DIALECT_MCU;
        } else {
            order[0] = &HP_DIALECT_MCU;
            order[1] = &HP_DIALECT_FG;
        }
        for (int k = 0; k < 2; k++) {
            ui_log("  RPC probe: %s", order[k]->name);
            if (probe(order[k])) {
                ui_log("  " BOARD_CP_SHORT " answered");
                s_fw_version = s_init.fw_version;
                if (!s_fw_version) s_fw_version = query_fw_version(order[k]);
                *cfg_out = cfg;
                return order[k];
            }
        }
        /* The card answered but no RPC did: ask the C6 to reset its
         * SDIO interface before the next bus setting (link_up() resets
         * the whole chip as well). */
        cp_sdio_send_intr(CP_INTR_RESET);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    return NULL;
}

/* Checks the embedded image and reads its version ("X.Y.Z"). */
static bool check_image(size_t size, uint32_t *version, char *ver_str, size_t ver_len)
{
    const uint8_t *img = cp_image_start;
    if (size < 256 || img[0] != 0xE9) {
        ui_log("Embedded " BOARD_CP_SHORT " image is not an ESP app image");
        return false;
    }
    uint16_t chip = img[12] | (img[13] << 8);
    if (chip != BOARD_CP_CHIP_ID) {
        ui_log("Embedded image is for chip id %u, not the " BOARD_CP_NAME, chip);
        return false;
    }
    if (size > CP_MAX_IMAGE) {
        ui_log("Embedded image (%u bytes) is larger than a 1.x OTA slot", (unsigned)size);
        return false;
    }
    /* esp_app_desc_t follows the image header (24) and the first
     * segment header (8): magic 0xABCD5432, version[32] at +16. */
    const uint8_t *desc = img + 32;
    uint32_t magic = desc[0] | (desc[1] << 8) | (desc[2] << 16) | ((uint32_t)desc[3] << 24);
    if (magic != 0xABCD5432) {
        ui_log("Embedded image has no app description");
        return false;
    }
    char v[33];
    memcpy(v, desc + 16, 32);
    v[32] = '\0';
    strlcpy(ver_str, v, ver_len);
    int maj = 0, min = 0, patch = 0;
    if (sscanf(v, "%d.%d.%d", &maj, &min, &patch) != 3) {
        ui_log("Embedded image version '%s' is not X.Y.Z", v);
        return false;
    }
    *version = version_val(maj, min, patch);
    return true;
}

static void describe_firmware(int line)
{
    char v[16];
    if (s_fw_version) {
        version_str(s_fw_version, v, sizeof(v));
        ui_detail(line, BOARD_CP_SHORT " firmware now: esp-hosted %s%s", v,
                  s_init.rpc_version >= 0 ? " (3.x protocol)" : "");
    } else if (s_init.seen) {
        ui_detail(line, BOARD_CP_SHORT " firmware now: esp-hosted 1.x or older (no version reported)");
    } else {
        ui_detail(line, BOARD_CP_SHORT " firmware now: unknown (no start-up message)");
    }
}

static bool run(void)
{
    size_t size = cp_image_end - cp_image_start;
    uint32_t target = 0;
    char target_str[33];

    ui_phase(UI_STATE_WORKING, "Checking the new " BOARD_CP_SHORT " firmware");
    if (!check_image(size, &target, target_str, sizeof(target_str))) {
        ui_phase(UI_STATE_ERROR, "This tool was built with a bad " BOARD_CP_SHORT " image");
        return false;
    }
    ui_detail(0, "New " BOARD_CP_SHORT " firmware: esp-hosted %s (%u KB)", target_str,
              (unsigned)(size / 1024));

    ui_phase(UI_STATE_WORKING, "Powering the " BOARD_CP_NAME);
    if (cp_power_on() != ESP_OK) {
        ui_phase(UI_STATE_ERROR, "Could not switch the " BOARD_CP_SHORT " supply on");
        return false;
    }

    ui_phase(UI_STATE_WORKING, "Talking to the " BOARD_CP_NAME);
    const link_cfg_t *cfg = NULL;
    const hp_dialect_t *d = connect(&cfg);
    if (!d) {
        ui_phase(UI_STATE_ERROR, "The " BOARD_CP_SHORT " does not answer in any known esp-hosted dialect");
        ui_detail(1, "Its firmware may not be esp-hosted. Flash it over USB "
                     "(see misc/p4_upgrade_cp/README.md).");
        return false;
    }
    describe_firmware(1);
    ui_detail(2, "Link: SDIO %d-bit, %d kHz; protocol: %s", cfg->width,
              cfg->freq_khz, d->name);

    if (s_fw_version == target) {
        ui_phase(UI_STATE_OK, "The " BOARD_CP_SHORT " already runs esp-hosted %s", target_str);
        if (!ui_ask("Install again", "Done", 0, false)) return true;
    }

    ui_phase(UI_STATE_WORKING, "Preparing the " BOARD_CP_SHORT " flash (may take a minute)");
    int32_t code = 0;
    esp_err_t err = rpc_call(d, d->id_ota_begin, NULL, 0, 120000, 1, &code);
    if (err != ESP_OK || code != 0) {
        ui_phase(UI_STATE_ERROR, "OTA begin failed (%s, code %" PRId32 ")",
                 esp_err_to_name(err), code);
        return false;
    }

    ui_phase(UI_STATE_WORKING, "Writing esp-hosted %s to the " BOARD_CP_SHORT, target_str);
    ui_progress(0);
    static uint8_t sub[OTA_CHUNK + 16];
    int last_pct = -1;
    int64_t t0 = esp_timer_get_time();
    for (size_t off = 0; off < size; off += OTA_CHUNK) {
        size_t chunk = size - off < OTA_CHUNK ? size - off : OTA_CHUNK;
        size_t n = hp_sub_bytes_field(sub, sizeof(sub), 1, cp_image_start + off, chunk);
        err = rpc_call(d, d->id_ota_write, sub, n, 15000, 1, &code);
        if (err != ESP_OK || code != 0) {
            ui_progress(-1);
            ui_phase(UI_STATE_ERROR, "Write failed at %u KB (%s, code %" PRId32 ")",
                     (unsigned)(off / 1024), esp_err_to_name(err), code);
            ui_detail(3, "The " BOARD_CP_SHORT " keeps its old firmware. Press RESET to try again.");
            return false;
        }
        int pct = (int)((off + chunk) * 100 / size);
        if (pct != last_pct) {
            ui_progress(pct);
            last_pct = pct;
        }
    }
    ui_log("Image written in %" PRId64 " s", (esp_timer_get_time() - t0) / 1000000);

    ui_phase(UI_STATE_WORKING, "Verifying the image on the " BOARD_CP_SHORT);
    err = rpc_call(d, d->id_ota_end, NULL, 0, 60000, 1, &code);
    if (err != ESP_OK || code != 0) {
        ui_progress(-1);
        ui_phase(UI_STATE_ERROR, "The " BOARD_CP_SHORT " rejected the image (%s, code %" PRId32 ")",
                 esp_err_to_name(err), code);
        ui_detail(3, "The " BOARD_CP_SHORT " keeps its old firmware. Press RESET to try again.");
        return false;
    }

    /* 2.6+ switches the boot slot in OTAActivate; older firmware did it
     * in OTAEnd and does not know the request (the reply is then an
     * error or nothing, both fine). */
    bool needs_activate = s_fw_version >= version_val(2, 6, 0);
    if (d->id_ota_activate) {
        ui_phase(UI_STATE_WORKING, "Activating the new firmware");
        err = rpc_call(d, d->id_ota_activate, NULL, 0, 10000, 1, &code);
        if (needs_activate && (err != ESP_OK || code != 0)) {
            ui_progress(-1);
            ui_phase(UI_STATE_ERROR, "Activation failed (%s, code %" PRId32 ")",
                     esp_err_to_name(err), code);
            ui_detail(3, "The " BOARD_CP_SHORT " keeps its old firmware. Press RESET to try again.");
            return false;
        }
        ui_log("Activate: %s, code %" PRId32 "%s", esp_err_to_name(err), code,
               needs_activate ? "" : " (not needed by this firmware)");
    }
    ui_progress(-1);

    ui_phase(UI_STATE_WORKING, "Restarting the " BOARD_CP_SHORT);
    /* Old firmware restarts by itself 5 s after OTAEnd / activate; wait
     * for that, then reset it anyway for a clean start. */
    vTaskDelay(pdMS_TO_TICKS(6000));

    ui_phase(UI_STATE_WORKING, "Checking the new firmware");
    const link_cfg_t *cfg2 = NULL;
    const hp_dialect_t *d2 = connect(&cfg2);
    if (!d2 || !s_init.seen) {
        ui_phase(UI_STATE_ERROR, "The " BOARD_CP_SHORT " does not answer after the upgrade");
        ui_detail(3, "Power the " BOARD_NAME " off and on, then run this tool again.");
        return false;
    }
    describe_firmware(1);
    if (s_fw_version != target) {
        ui_phase(UI_STATE_ERROR, "The " BOARD_CP_SHORT " still runs its old firmware");
        ui_detail(3, "Press RESET to run the upgrade again.");
        return false;
    }
    ui_phase(UI_STATE_OK, "Done: the " BOARD_CP_SHORT " runs esp-hosted %s", target_str);
    ui_detail(3, "Now flash a Draftling build for esp-hosted 3.1.");
    return true;
}

static void upgrader_task(void *arg)
{
    s_rx = cp_sdio_alloc(RX_CAP + 64);
    s_tx = cp_sdio_alloc(CP_SDIO_MAX_FRAME + 64);
    if (!s_rx || !s_tx) {
        ui_phase(UI_STATE_ERROR, "Out of memory");
    } else {
        run();
    }
    vTaskDelete(NULL);
}

void upgrader_start(void)
{
    xTaskCreate(upgrader_task, "upgrader", 8192, NULL, 5, NULL);
}
