/*
 * esp-hosted wire protocol pieces for the C6 upgrade (see hosted_proto.h).
 */
#include "hosted_proto.h"

#include <string.h>

const hp_dialect_t HP_DIALECT_MCU = {
    .name = "esp-hosted-mcu (RPC-V2)",
    .serial_if = 3,
    .priv_if = 5,
    .req_ep = "RPCRsp",
    .id_get_mac = 0x101,
    .id_ota_begin = 0x110,
    .id_ota_write = 0x111,
    .id_ota_end = 0x112,
    .id_ota_activate = 0x10a,
    .id_get_fw_version = 0x15e,
    .resp_offset = 0x100,
};

const hp_dialect_t HP_DIALECT_FG = {
    .name = "ESP-Hosted-FG (RPC-V1)",
    .serial_if = 2,
    .priv_if = 4,
    .req_ep = "ctrlResp",
    .id_get_mac = 101,
    .id_ota_begin = 116,
    .id_ota_write = 117,
    .id_ota_end = 118,
    .id_ota_activate = 0,
    .id_get_fw_version = 0,
    .resp_offset = 100,
};

static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static void wr16(uint8_t *p, uint16_t v) { p[0] = v & 0xFF; p[1] = v >> 8; }

static uint16_t checksum(const uint8_t *buf, size_t len, size_t cksum_off)
{
    uint16_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        if (i == cksum_off || i == cksum_off + 1) continue;
        sum += buf[i];
    }
    return sum;
}

int hp_frame_parse(const uint8_t *buf, size_t avail, hp_frame_t *f)
{
    memset(f, 0, sizeof(*f));
    if (avail < HP_HDR_V1_SIZE) return 0;

    uint16_t len, offset;
    if (buf[0] == HP_HDR_V2_MAGIC && buf[1] == 2 && avail >= HP_HDR_V2_SIZE) {
        f->v2 = true;
        f->if_type = buf[4] & 0x3F;
        f->if_num = buf[4] >> 6;
        f->flags = buf[5];
        f->pkt_type = buf[19];
        offset = rd16(buf + 8);
        len = rd16(buf + 10);
        if (offset < HP_HDR_V2_SIZE) return -1;
    } else {
        f->if_type = buf[0] & 0x0F;
        f->if_num = buf[0] >> 4;
        f->flags = buf[1];
        len = rd16(buf + 2);
        offset = rd16(buf + 4);
        f->pkt_type = buf[11];
        if (len == 0) return 0;           /* padding / dummy frame */
        if (offset < HP_HDR_V1_SIZE || offset > HP_HDR_V1_SIZE + 8) return -1;
    }
    if ((size_t)offset + len > avail) return -1;
    f->payload = buf + offset;
    f->len = len;
    f->total = offset + len;
    return 1;
}

size_t hp_frame_build_v1(uint8_t *buf, uint8_t if_type, uint8_t pkt_type,
                         const uint8_t *payload, uint16_t len, uint16_t seq)
{
    memset(buf, 0, HP_HDR_V1_SIZE);
    buf[0] = if_type & 0x0F;          /* if_num 0 */
    buf[1] = 0;
    wr16(buf + 2, len);
    wr16(buf + 4, HP_HDR_V1_SIZE);
    wr16(buf + 8, seq);
    buf[11] = pkt_type;
    if (len) memcpy(buf + HP_HDR_V1_SIZE, payload, len);
    size_t total = HP_HDR_V1_SIZE + len;
    /* Always correct: slaves built with checksums drop frames without. */
    wr16(buf + 6, checksum(buf, total, 6));
    return total;
}

bool hp_parse_init_event(const uint8_t *payload, uint16_t len, hp_init_info_t *info)
{
    if (len < 2) return false;
    uint8_t event_type = payload[0];
    uint8_t event_len = payload[1];
    if (event_type != HP_PRIV_EVENT_INIT && event_type != 0x00) return false;
    if ((size_t)event_len + 2 > len) event_len = len - 2;

    info->seen = true;
    info->chip_id = -1;
    info->fw_version = 0;
    info->rpc_version = -1;

    const uint8_t *p = payload + 2;
    size_t left = event_len;
    while (left >= 2) {
        uint8_t tag = p[0], tlen = p[1];
        if ((size_t)tlen + 2 > left) break;
        const uint8_t *v = p + 2;
        switch (tag) {
        case HP_TLV_CAPABILITY:
            if (tlen >= 1) info->capabilities = v[0];
            break;
        case HP_TLV_CHIP_ID:
            if (tlen >= 1) info->chip_id = v[0];
            break;
        case HP_TLV_FIRMWARE_VERSION:
            if (tlen >= 4) {
                info->fw_version = (uint32_t)v[0] | ((uint32_t)v[1] << 8) |
                                   ((uint32_t)v[2] << 16) | ((uint32_t)v[3] << 24);
            }
            break;
        case HP_TLV_RPC_VERSION:
            if (tlen >= 1) info->rpc_version = v[0];
            break;
        default:
            break;
        }
        p += tlen + 2;
        left -= tlen + 2;
    }
    return true;
}

size_t hp_build_host_config(uint8_t *buf, size_t cap, int chip_id)
{
    if (cap < 8) return 0;
    size_t n = 2;
    buf[n++] = HP_TLV_HOST_CAPABILITIES;
    buf[n++] = 1;
    buf[n++] = 0;
    if (chip_id >= 0) {
        buf[n++] = HP_TLV_RCVD_CHIP_ID;
        buf[n++] = 1;
        buf[n++] = (uint8_t)chip_id;
    }
    buf[0] = HP_PRIV_EVENT_INIT;
    buf[1] = (uint8_t)(n - 2);
    return n;
}

size_t hp_serial_wrap(uint8_t *buf, size_t cap, const char *ep,
                      const uint8_t *data, size_t len)
{
    size_t ep_len = strlen(ep);
    size_t total = 3 + ep_len + 3 + len;
    if (total > cap) return 0;
    size_t n = 0;
    buf[n++] = 0x01;
    wr16(buf + n, (uint16_t)ep_len); n += 2;
    memcpy(buf + n, ep, ep_len); n += ep_len;
    buf[n++] = 0x02;
    wr16(buf + n, (uint16_t)len); n += 2;
    memcpy(buf + n, data, len); n += len;
    return n;
}

bool hp_serial_unwrap(const uint8_t *buf, size_t len, char *ep, size_t ep_cap,
                      const uint8_t **data, size_t *data_len)
{
    bool have_ep = false, have_data = false;
    size_t pos = 0;
    ep[0] = '\0';
    while (pos + 3 <= len) {
        uint8_t type = buf[pos];
        uint16_t l = rd16(buf + pos + 1);
        pos += 3;
        if (pos + l > len) return false;
        if (type == 0x01) {
            size_t c = l < ep_cap - 1 ? l : ep_cap - 1;
            memcpy(ep, buf + pos, c);
            ep[c] = '\0';
            have_ep = true;
        } else if (type == 0x02) {
            *data = buf + pos;
            *data_len = l;
            have_data = true;
        } else {
            return false;
        }
        pos += l;
    }
    return have_ep && have_data;
}

/* ---- minimal protobuf ---- */

static size_t put_varint(uint8_t *buf, uint64_t v)
{
    size_t n = 0;
    do {
        uint8_t b = v & 0x7F;
        v >>= 7;
        buf[n++] = b | (v ? 0x80 : 0);
    } while (v);
    return n;
}

static bool get_varint(const uint8_t *buf, size_t len, size_t *pos, uint64_t *out)
{
    uint64_t v = 0;
    for (int shift = 0; shift < 64; shift += 7) {
        if (*pos >= len) return false;
        uint8_t b = buf[(*pos)++];
        v |= (uint64_t)(b & 0x7F) << shift;
        if (!(b & 0x80)) {
            *out = v;
            return true;
        }
    }
    return false;
}

size_t hp_sub_varint_field(uint8_t *buf, uint32_t field, uint32_t value)
{
    size_t n = put_varint(buf, ((uint64_t)field << 3) | 0);
    n += put_varint(buf + n, value);
    return n;
}

size_t hp_sub_bytes_field(uint8_t *buf, size_t cap, uint32_t field,
                          const uint8_t *data, size_t len)
{
    if (cap < len + 12) return 0;
    size_t n = put_varint(buf, ((uint64_t)field << 3) | 2);
    n += put_varint(buf + n, len);
    memcpy(buf + n, data, len);
    return n + len;
}

size_t hp_rpc_build(uint8_t *buf, size_t cap, uint32_t msg_id, uint32_t uid,
                    const uint8_t *sub, size_t sub_len)
{
    if (cap < sub_len + 32) return 0;
    size_t n = 0;
    n += put_varint(buf + n, (1u << 3) | 0);
    n += put_varint(buf + n, 1);                 /* msg_type = Req */
    n += put_varint(buf + n, (2u << 3) | 0);
    n += put_varint(buf + n, msg_id);
    n += put_varint(buf + n, (3u << 3) | 0);
    n += put_varint(buf + n, uid);
    n += put_varint(buf + n, ((uint64_t)msg_id << 3) | 2);
    n += put_varint(buf + n, sub_len);
    if (sub_len) memcpy(buf + n, sub, sub_len);
    return n + sub_len;
}

/* Walks the fields of a message; calls back for each. */
typedef bool (*field_cb_t)(uint32_t field, uint32_t wire, uint64_t value,
                           const uint8_t *data, size_t data_len, void *ctx);

static bool walk(const uint8_t *buf, size_t len, field_cb_t cb, void *ctx)
{
    size_t pos = 0;
    while (pos < len) {
        uint64_t key;
        if (!get_varint(buf, len, &pos, &key)) return false;
        uint32_t field = (uint32_t)(key >> 3);
        uint32_t wire = key & 7;
        uint64_t value = 0;
        const uint8_t *data = NULL;
        size_t data_len = 0;
        switch (wire) {
        case 0:
            if (!get_varint(buf, len, &pos, &value)) return false;
            break;
        case 1:
            if (pos + 8 > len) return false;
            pos += 8;
            break;
        case 2: {
            uint64_t l;
            if (!get_varint(buf, len, &pos, &l) || pos + l > len) return false;
            data = buf + pos;
            data_len = (size_t)l;
            pos += (size_t)l;
            break;
        }
        case 5:
            if (pos + 4 > len) return false;
            pos += 4;
            break;
        default:
            return false;
        }
        if (cb && !cb(field, wire, value, data, data_len, ctx)) return true;
    }
    return true;
}

static bool rpc_cb(uint32_t field, uint32_t wire, uint64_t value,
                   const uint8_t *data, size_t data_len, void *ctx)
{
    hp_rpc_msg_t *m = ctx;
    if (field == 1 && wire == 0) m->msg_type = (uint32_t)value;
    else if (field == 2 && wire == 0) m->msg_id = (uint32_t)value;
    else if (field == 3 && wire == 0) m->uid = (uint32_t)value;
    else if (wire == 2 && field > 3 && m->msg_id && field == m->msg_id) {
        m->sub = data;
        m->sub_len = data_len;
    } else if (wire == 2 && field > 4 && !m->sub) {
        /* msg_id normally precedes the payload; keep the first message
         * field in case it does not. */
        m->sub = data;
        m->sub_len = data_len;
    }
    return true;
}

bool hp_rpc_parse(const uint8_t *buf, size_t len, hp_rpc_msg_t *m)
{
    memset(m, 0, sizeof(*m));
    if (!walk(buf, len, rpc_cb, m)) return false;
    return m->msg_type != 0 && m->msg_id != 0;
}

typedef struct {
    uint32_t field;
    bool found;
    uint64_t value;
    const uint8_t *data;
    size_t data_len;
} find_ctx_t;

static bool find_cb(uint32_t field, uint32_t wire, uint64_t value,
                    const uint8_t *data, size_t data_len, void *ctx)
{
    find_ctx_t *f = ctx;
    if (field != f->field) return true;
    f->found = true;
    f->value = value;
    f->data = data;
    f->data_len = data_len;
    (void)wire;
    return false;
}

bool hp_sub_get_int(const uint8_t *sub, size_t len, uint32_t field, int32_t *out)
{
    find_ctx_t f = { .field = field };
    if (!walk(sub, len, find_cb, &f) || !f.found) return false;
    *out = (int32_t)(uint32_t)f.value;   /* int32 negatives are 64-bit varints */
    return true;
}

bool hp_sub_get_bytes(const uint8_t *sub, size_t len, uint32_t field,
                      const uint8_t **data, size_t *data_len)
{
    find_ctx_t f = { .field = field };
    if (!walk(sub, len, find_cb, &f) || !f.found || !f.data) return false;
    *data = f.data;
    *data_len = f.data_len;
    return true;
}
