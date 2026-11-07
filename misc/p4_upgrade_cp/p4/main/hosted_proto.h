/*
 * The parts of the esp-hosted wire protocol the C6 upgrade needs, for
 * every dialect a co-processor (such as the Tab5's C6) may speak.
 *
 * Frame header (one per SDIO frame):
 *   V1, 12 bytes (every MCU co-processor, 0.0.x - 3.x):
 *     0 if_type:4 | if_num:4, 1 flags, 2-3 len, 4-5 offset (12),
 *     6-7 checksum, 8-9 seq_num, 10 throttle, 11 pkt_type
 *   V2, 20 bytes (3.x, only after negotiation; parsed just in case):
 *     0 magic 0xE9, 1 version 2, 2-3 pkt_num, 4 if_type:6 | if_num:2,
 *     5 flags, 6 pkt_type, 7 frag_seq, 8-9 offset, 10-11 len,
 *     12-13 checksum, ..., 19 priv_pkt_type
 *   Checksum: 16-bit sum of header + payload with the checksum zeroed.
 *
 * Interface numbers: MCU builds use SERIAL = 3, PRIV = 5; Linux FG
 * builds ("legacy wire interface types") use SERIAL = 2, PRIV = 4.
 *
 * Serial payload: TLV 0x01 endpoint name, TLV 0x02 data (2-byte LE
 * lengths). The data is a protobuf: field 1 msg_type (1 req, 2 resp,
 * 3 event), field 2 msg_id, field 3 uid, then the message as the field
 * whose number is the msg_id.
 *
 * Dialects (RPC ids and endpoint names):
 *   MCU (esp-hosted-mcu 0.0.x - 3.x, RPC-V2): "RPCRsp" / "RPCEvt",
 *     GetMACAddress 0x101, OTABegin 0x110, OTAWrite 0x111, OTAEnd 0x112,
 *     OTAActivate 0x10a (2.6+), GetCoprocessorFwVersion 0x15e;
 *     response id = request id + 0x100.
 *   Linux FG (ESP-Hosted-FG, and 3.x built for FG peers, RPC-V1):
 *     "ctrlResp" / "ctrlEvnt", GetMACAddress 101, OTABegin 116,
 *     OTAWrite 117, OTAEnd 118 (no activate); response id = id + 100.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HP_HDR_V1_SIZE   12
#define HP_HDR_V2_SIZE   20
#define HP_HDR_V2_MAGIC  0xE9
#define HP_FLAG_MORE_FRAGMENT  0x01

/* Priv-interface start-up message. */
#define HP_PRIV_PKT_TYPE_EVENT   0x33   /* 0x00 on FG builds */
#define HP_PRIV_EVENT_INIT       0x22   /* 0x00 on FG builds */
#define HP_TLV_CAPABILITY        0x11
#define HP_TLV_CHIP_ID           0x12
#define HP_TLV_FIRMWARE_VERSION  0x17   /* 2.x+: u32 LE, (maj<<16)|(min<<8)|patch */
#define HP_TLV_RPC_VERSION       0x1A   /* 3.x */
#define HP_CAP_CHECKSUM_ENABLED  0x80

/* Host config reply (priv interface, same event layout). */
#define HP_TLV_HOST_CAPABILITIES 0x44
#define HP_TLV_RCVD_CHIP_ID      0x45

typedef struct {
    const char *name;
    uint8_t serial_if;
    uint8_t priv_if;
    const char *req_ep;
    uint32_t id_get_mac;
    uint32_t id_ota_begin;
    uint32_t id_ota_write;
    uint32_t id_ota_end;
    uint32_t id_ota_activate;   /* 0: none */
    uint32_t id_get_fw_version; /* 0: none; resp fields 2-4 = major, minor, patch */
    uint32_t resp_offset;       /* response id = request id + this */
} hp_dialect_t;

extern const hp_dialect_t HP_DIALECT_MCU;
extern const hp_dialect_t HP_DIALECT_FG;

typedef struct {
    bool v2;            /* V2 (20-byte) header */
    uint8_t if_type;
    uint8_t if_num;
    uint8_t flags;
    uint8_t pkt_type;   /* priv_pkt_type / hci_pkt_type */
    const uint8_t *payload;
    uint16_t len;
    uint16_t total;     /* bytes this frame occupies (offset + len) */
} hp_frame_t;

/* Parses the frame at buf. Returns 1 with *f filled, 0 when the rest of
 * the buffer is padding or a dummy frame (stop), -1 when it is not a
 * valid frame. */
int hp_frame_parse(const uint8_t *buf, size_t avail, hp_frame_t *f);

/* Builds a V1 frame; returns its total length. */
size_t hp_frame_build_v1(uint8_t *buf, uint8_t if_type, uint8_t pkt_type,
                         const uint8_t *payload, uint16_t len, uint16_t seq);

/* What the start-up message said. */
typedef struct {
    bool seen;
    bool fg_numbering;      /* came on the FG priv interface (4) */
    uint8_t capabilities;
    int chip_id;            /* -1: not sent */
    uint32_t fw_version;    /* 0: not sent (1.x or older) */
    int rpc_version;        /* -1: not sent (pre-3.x) */
} hp_init_info_t;

/* Parses a priv-interface event payload; true if it was the INIT event. */
bool hp_parse_init_event(const uint8_t *payload, uint16_t len, hp_init_info_t *info);

/* Builds the host config event payload (host capabilities 0, chip id
 * echoed back); returns its length. */
size_t hp_build_host_config(uint8_t *buf, size_t cap, int chip_id);

/* Serial TLV: endpoint name + data. */
size_t hp_serial_wrap(uint8_t *buf, size_t cap, const char *ep,
                      const uint8_t *data, size_t len);
bool hp_serial_unwrap(const uint8_t *buf, size_t len, char *ep, size_t ep_cap,
                      const uint8_t **data, size_t *data_len);

/* RPC request: msg_type 1, msg_id, uid, then field msg_id carrying
 * the submessage `sub`. */
size_t hp_rpc_build(uint8_t *buf, size_t cap, uint32_t msg_id, uint32_t uid,
                    const uint8_t *sub, size_t sub_len);

/* Submessages. */
size_t hp_sub_varint_field(uint8_t *buf, uint32_t field, uint32_t value);
size_t hp_sub_bytes_field(uint8_t *buf, size_t cap, uint32_t field,
                          const uint8_t *data, size_t len);

typedef struct {
    uint32_t msg_type;
    uint32_t msg_id;
    uint32_t uid;
    const uint8_t *sub;     /* the response submessage (field msg_id) */
    size_t sub_len;
} hp_rpc_msg_t;

bool hp_rpc_parse(const uint8_t *buf, size_t len, hp_rpc_msg_t *m);

/* Reads a varint (int32 / uint32) field of a submessage; false if the
 * field is absent. */
bool hp_sub_get_int(const uint8_t *sub, size_t len, uint32_t field, int32_t *out);

/* Reads a length-delimited field of a submessage. */
bool hp_sub_get_bytes(const uint8_t *sub, size_t len, uint32_t field,
                      const uint8_t **data, size_t *data_len);

#ifdef __cplusplus
}
#endif
