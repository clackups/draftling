/*
 * Kermit server -- see include/kermit.h.
 *
 * A stop-and-wait Kermit server (no sliding windows) with long
 * packets, block checks 1, 2 and 3, control prefixing, optional
 * 8th-bit prefixing and repeat-count compression. Packet framing, the
 * block checks, the Send-Init parameter negotiation and the prefix
 * encoding follow the Kermit protocol as described at
 * https://www.kermitproject.org/kpackets.html and as implemented by
 * E-Kermit (github.com/KermitProject/ekermit), which has no server
 * mode of its own.
 *
 * Transaction overview (the client always starts one):
 *   I          parameter exchange; ACK carries ours
 *   S F [A] D.. Z .. B   upload: files are written under the root
 *   R          download: we send S F D.. Z .. B
 *   G<cmd>     generic command; short replies ride in the ACK, long
 *              ones (directory listings, TYPE) are sent like a file
 *              with an X (text) header
 */

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <time.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "kermit.h"
#include "kermit_port.h"

#define SOH         0x01
#define tochar(x)   ((uint8_t)((x) + 32))
#define unchar(c)   ((int)(c) - 32)
#define ctl(c)      ((uint8_t)((c) ^ 64))

/* Longest packet we accept (count of characters from LEN through the
 * block check, i.e. the extended length plus header). */
#define MY_MAXLX    9000
#define MY_TIMEOUT  10      /* seconds the client should wait for us */
#define MY_QCTL     '#'
#define MY_REPT     '~'
#define CAP_LP      2       /* long packets */
#define CAP_AT      8       /* attribute packets */
#define MAX_RETRY   10
/* Packet buffers. Senders count the advertised maximum differently
 * (C-Kermit's longest packets run a few characters past it), so the
 * receive check only enforces what fits here. */
#define PKT_BUF     (MY_MAXLX + 128)

#define TASK_STACK  (12 * 1024)

#define ROOT_MAX    32
#define PATH_MAX_K  320
#define PATH_BUF    (PATH_MAX_K + 8 + 1 + 256)   /* dir + "/" + name */

/* ---- State ---- */

typedef struct {
    int     seq;
    char    type;
    uint8_t *data;      /* NUL-terminated */
    int     len;
    int     bct;        /* block check type the packet validated with */
} kpkt_t;

/* Parameters negotiated with the client (last I or S exchange). */
static struct {
    int     maxl;       /* longest basic packet the client accepts */
    int     maxlx;      /* longest long packet (when lp) */
    bool    lp;         /* long packets */
    bool    at;         /* attribute packets */
    int     timo;       /* seconds to wait for the client */
    int     npad;
    uint8_t padc;
    uint8_t eol;
    uint8_t r_ctlq;     /* the client's control prefix */
    bool    ebq_on;
    uint8_t ebq;
    bool    rpt_on;
    uint8_t rptq;
    int     bct;        /* negotiated block check type */
} P;

static kermit_status_cb_t s_cb;
static StackType_t       *s_stack;      /* PSRAM, kept between runs */
static StaticTask_t       s_tcb;
static SemaphoreHandle_t  s_done;
static volatile bool      s_stop;
static volatile bool      s_running;
static bool               s_in_xfer;    /* a transaction is under way */

static char     s_root[ROOT_MAX];
static char     s_cwd[PATH_MAX_K];      /* relative to root, "" = root */

static uint8_t *s_rx;                   /* received packet, LEN..CHECK */
static uint8_t *s_tx;                   /* packet being sent */
static int      s_tx_len;               /* for retransmission */
static uint8_t *s_dbuf;                 /* encoded data field scratch */
static uint8_t  s_in[512];              /* raw input from the port */
static int      s_in_pos, s_in_len;
static int      s_cur_bct = 1;          /* block check for what we send */

/* ---- Helpers ---- */

static void status(bool changed, const char *fmt, ...)
{
    if (!s_cb) return;
    char msg[96];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    s_cb(msg, changed);
}

static int64_t now_ms(void)
{
    return esp_timer_get_time() / 1000;
}

static void params_default(void)
{
    memset(&P, 0, sizeof(P));
    P.maxl   = 80;
    P.maxlx  = 500;
    P.timo   = 5;
    P.padc   = 0;
    P.eol    = '\r';
    P.r_ctlq = '#';
    P.ebq    = '&';
    P.rptq   = MY_REPT;
    P.bct    = 1;
}

static bool is_prefix_char(uint8_t c)
{
    return (c > 32 && c < 63) || (c > 95 && c < 127);
}

/* Copy src into dst[cap], cutting it short if needed. */
static void copy_trunc(char *dst, size_t cap, const char *src)
{
    size_t n = strlen(src);
    if (n >= cap) n = cap - 1;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

/* ---- Block checks ---- */

static unsigned sum_bytes(const uint8_t *s, int n)
{
    unsigned sum = 0;
    for (int i = 0; i < n; i++) sum += s[i];
    return sum;
}

static uint8_t chk1(const uint8_t *s, int n)
{
    unsigned sum = sum_bytes(s, n);
    return tochar((sum + ((sum & 0300) >> 6)) & 077);
}

/* Kermit's CRC-16 (CCITT polynomial, reflected, initial value 0). */
static unsigned crc16(const uint8_t *s, int n)
{
    unsigned crc = 0;
    for (int i = 0; i < n; i++) {
        crc ^= s[i];
        for (int b = 0; b < 8; b++)
            crc = (crc & 1) ? (crc >> 1) ^ 0x8408 : (crc >> 1);
    }
    return crc & 0xFFFF;
}

/* Write the type-bct check of s[0..n) to out; returns its length. */
static int make_check(const uint8_t *s, int n, int bct, uint8_t *out)
{
    if (bct == 2) {
        unsigned sum = sum_bytes(s, n) & 07777;
        out[0] = tochar((sum >> 6) & 077);
        out[1] = tochar(sum & 077);
        return 2;
    }
    if (bct == 3) {
        unsigned crc = crc16(s, n);
        out[0] = tochar((crc >> 12) & 017);
        out[1] = tochar((crc >> 6) & 077);
        out[2] = tochar(crc & 077);
        return 3;
    }
    out[0] = chk1(s, n);
    return 1;
}

/* ---- Packet output ---- */

/* Largest data field we may send with block check bct. */
static int data_max(int bct)
{
    int m = P.maxl - 2 - bct;
    if (P.lp) {
        int lx = P.maxlx - 6 - bct;
        if (lx > m) m = lx;
    }
    if (m > MY_MAXLX - 16) m = MY_MAXLX - 16;
    if (m < 10) m = 10;
    return m;
}

/* Build and send a packet. save=true keeps it for retransmission. */
static void tx_packet(char type, int seq, const uint8_t *data, int len,
                      int bct, bool save)
{
    uint8_t *b = s_tx;
    int i = 0;
    b[i++] = SOH;
    int lenpos = i++;
    b[i++] = tochar(seq & 63);
    b[i++] = (uint8_t)type;
    if (len + bct + 2 > 94) {
        int x = len + bct;
        b[lenpos] = tochar(0);
        b[i++] = tochar(x / 95);
        b[i++] = tochar(x % 95);
        b[i] = chk1(&b[lenpos], i - lenpos);
        i++;
    } else {
        b[lenpos] = tochar(len + bct + 2);
    }
    if (len > 0) {
        memcpy(&b[i], data, len);
        i += len;
    }
    i += make_check(&b[lenpos], i - lenpos, bct, &b[i]);
    b[i++] = P.eol;

    for (int k = 0; k < P.npad; k++) kport_write(&P.padc, 1);
    kport_write(b, i);
    if (save) s_tx_len = i;
}

static void resend(void)
{
    if (s_tx_len <= 0) return;
    for (int k = 0; k < P.npad; k++) kport_write(&P.padc, 1);
    kport_write(s_tx, s_tx_len);
}

/* ---- Data field encoding / decoding ---- */

/* Byte source with one byte of lookahead: a file or a memory block. */
typedef struct {
    FILE          *fp;
    const uint8_t *mem;
    size_t         mem_len, mem_pos;
    uint8_t        buf[1024];
    size_t         buf_len, buf_pos;
    long           count;           /* bytes consumed so far */
    bool           err;             /* the file could not be read to its end */
} ksrc_t;

static int src_peek(ksrc_t *s)
{
    if (s->fp) {
        if (s->buf_pos >= s->buf_len) {
            s->buf_len = fread(s->buf, 1, sizeof(s->buf), s->fp);
            s->buf_pos = 0;
            if (s->buf_len == 0) {
                if (ferror(s->fp)) s->err = true;
                return -1;
            }
        }
        return s->buf[s->buf_pos];
    }
    if (s->mem_pos >= s->mem_len) return -1;
    return s->mem[s->mem_pos];
}

static int src_get(ksrc_t *s)
{
    int c = src_peek(s);
    if (c < 0) return -1;
    if (s->fp) s->buf_pos++;
    else s->mem_pos++;
    s->count++;
    return c;
}

/* Encode one byte (no repeat prefix); returns the encoded length. */
static int enc_byte(uint8_t c, uint8_t *o)
{
    int n = 0;
    uint8_t a7 = c & 0x7F;
    if (P.ebq_on && (c & 0x80)) {
        o[n++] = P.ebq;
        c = a7;
    }
    if (a7 < 32 || a7 == 127) {
        o[n++] = MY_QCTL;
        c = ctl(c);
    } else if (a7 == MY_QCTL || (P.ebq_on && a7 == P.ebq) ||
               (P.rpt_on && a7 == P.rptq)) {
        o[n++] = MY_QCTL;
    }
    o[n++] = c;
    return n;
}

/* Fill out[] with up to max encoded bytes from src. Returns the
 * encoded length (0 at end of input). */
static int encode_from(ksrc_t *src, uint8_t *out, int max)
{
    int n = 0;
    /* Room for the largest item: two plain copies (2 x 4) or a
     * repeat prefix + count + an 8th-bit + control-prefixed byte. */
    while (n + 10 <= max) {
        int c = src_get(src);
        if (c < 0) break;
        int run = 1;
        if (P.rpt_on) {
            while (run < 94 && src_peek(src) == c) {
                src_get(src);
                run++;
            }
        }
        if (run >= 3) {
            out[n++] = P.rptq;
            out[n++] = tochar(run);
            n += enc_byte((uint8_t)c, &out[n]);
        } else {
            for (int r = 0; r < run; r++) n += enc_byte((uint8_t)c, &out[n]);
        }
    }
    return n;
}

static int encode_str(const char *s, uint8_t *out, int max)
{
    ksrc_t src = {0};
    src.mem = (const uint8_t *)s;
    src.mem_len = strlen(s);
    return encode_from(&src, out, max);
}

/* Decoded-output sink: a memory buffer, optionally flushed to a file. */
typedef struct {
    uint8_t *buf;
    size_t   len, cap;
    FILE    *fp;
    bool     err;
    long     total;
    bool     text;      /* text file: CR LF on the line, LF on the card */
    bool     cr;        /* a CR is held back (text mode) */
} ksink_t;

static void sink_raw(ksink_t *k, uint8_t c)
{
    if (k->len >= k->cap) {
        if (!k->fp) return;          /* memory sink: truncate */
        if (fwrite(k->buf, 1, k->len, k->fp) != k->len) k->err = true;
        k->len = 0;
    }
    k->buf[k->len++] = c;
    k->total++;
}

static void sink_put(ksink_t *k, uint8_t c)
{
    if (!k->text) {
        sink_raw(k, c);
        return;
    }
    if (k->cr) {
        k->cr = false;
        if (c != '\n') sink_raw(k, '\r');
    }
    if (c == '\r') k->cr = true;
    else sink_raw(k, c);
}

static void sink_flush(ksink_t *k)
{
    if (k->cr) {
        k->cr = false;
        sink_raw(k, '\r');
    }
    if (k->fp && k->len > 0) {
        if (fwrite(k->buf, 1, k->len, k->fp) != k->len) k->err = true;
        k->len = 0;
    }
}

static void decode_into(const uint8_t *in, int n, ksink_t *k)
{
    int i = 0;
    while (i < n) {
        int rpt = 1;
        uint8_t a = in[i++];
        if (P.rpt_on && a == P.rptq && i < n) {
            rpt = unchar(in[i++]);
            if (i >= n) break;
            a = in[i++];
        }
        uint8_t b8 = 0;
        if (P.ebq_on && a == P.ebq && i < n) {
            b8 = 0x80;
            a = in[i++] & 0x7F;
        }
        if (a == P.r_ctlq && i < n) {
            a = in[i++];
            uint8_t a7 = a & 0x7F;
            if ((a7 >= 0100 && a7 <= 0137) || a7 == '?') a = ctl(a);
        }
        a |= b8;
        for (; rpt > 0; rpt--) sink_put(k, a);
    }
}

/* Decode a packet's data field into a NUL-terminated string. */
static void decode_str(const kpkt_t *p, char *out, size_t cap)
{
    ksink_t k = { (uint8_t *)out, 0, cap - 1, NULL, false, 0, false, false };
    decode_into(p->data, p->len, &k);
    out[k.len] = '\0';
}

/* ---- Packet input ---- */

/* Next input byte: >= 0, -1 on timeout (deadline reached), -2 when
 * the server is being stopped. deadline 0 = wait forever. */
static int getb(int64_t deadline)
{
    while (s_in_pos >= s_in_len) {
        if (s_stop) return -2;
        uint32_t wait = 50;
        if (deadline) {
            int64_t left = deadline - now_ms();
            if (left <= 0) return -1;
            if (left < wait) wait = (uint32_t)left;
        }
        s_in_len = kport_read(s_in, sizeof(s_in), wait);
        s_in_pos = 0;
    }
    return s_in[s_in_pos++];
}

/* Receive one packet. timeout_s 0 = wait forever. Returns 1 for a
 * packet, 0 on timeout, -1 when stopping, -2 for a corrupt packet. */
static int rx_packet(kpkt_t *p, int timeout_s)
{
    int64_t deadline = timeout_s ? now_ms() + (int64_t)timeout_s * 1000 : 0;
    int c;
    for (;;) {
        /* Hunt for the start of a packet. */
        do {
            c = getb(deadline);
            if (c == -1) return 0;
            if (c == -2) return -1;
        } while (c != SOH);
restart:
        c = getb(deadline);
        if (c == -1) return 0;
        if (c == -2) return -1;
        if (c == SOH) goto restart;
        if (c < 32) continue;
        s_rx[0] = (uint8_t)c;
        int len = unchar(c);
        int hdr, total;
        if (len == 0) {                       /* long packet */
            for (int i = 1; i <= 5; i++) {
                c = getb(deadline);
                if (c == -1) return 0;
                if (c == -2) return -1;
                if (c == SOH) goto restart;
                if (c < 32) goto discard;
                s_rx[i] = (uint8_t)c;
            }
            if (chk1(s_rx, 5) != s_rx[5]) return -2;
            int xlen = unchar(s_rx[3]) * 95 + unchar(s_rx[4]);
            if (xlen < 1 || 6 + xlen + 1 > PKT_BUF) return -2;
            hdr = 6;
            total = 6 + xlen;
        } else if (len >= 3) {
            hdr = 3;
            total = 1 + len;
        } else {
            continue;                         /* reserved lengths */
        }
        /* The body may hold control characters the client leaves
         * unprefixed on a clean 8-bit line (C-Kermit's default
         * "cautious" prefixing sends TAB as is, for example); only SOH
         * marks the start of a new packet. */
        for (int i = (len == 0) ? 6 : 1; i < total; i++) {
            c = getb(deadline);
            if (c == -1) return 0;
            if (c == -2) return -1;
            if (c == SOH) goto restart;
            s_rx[i] = (uint8_t)c;
        }

        /* The block check length is not in the packet. Try the type
         * expected for this exchange first, then the others: a packet
         * that validates with the wrong one is practically impossible
         * for types 3 and 2, and type 1 is tried last. */
        int order[4] = { s_cur_bct, 3, 2, 1 };
        for (int t = 0; t < 4; t++) {
            int b = order[t];
            if (t > 0 && b == s_cur_bct) continue;
            if (total - hdr < b) continue;
            uint8_t chk[3];
            make_check(s_rx, total - b, b, chk);
            if (memcmp(chk, &s_rx[total - b], b) == 0) {
                p->seq  = unchar(s_rx[1]);
                p->type = (char)s_rx[2];
                p->data = &s_rx[hdr];
                p->len  = total - hdr - b;
                p->bct  = b;
                s_rx[total - b] = '\0';
                if (p->seq < 0 || p->seq > 63) return -2;
                return 1;
            }
        }
        return -2;
discard:
        ;
    }
}

/* ---- Parameter negotiation ---- */

static int build_params(uint8_t *d, bool ack)
{
    int n = 0;
    d[n++] = tochar(94);                    /* MAXL */
    d[n++] = tochar(MY_TIMEOUT);            /* TIME */
    d[n++] = tochar(0);                     /* NPAD */
    d[n++] = ctl(0);                        /* PADC */
    d[n++] = tochar('\r');                  /* EOL */
    d[n++] = MY_QCTL;                       /* QCTL */
    d[n++] = 'Y';                           /* QBIN: willing, not asking */
    d[n++] = (uint8_t)('0' + (ack ? P.bct : 3));                /* CHKT */
    d[n++] = ack ? (P.rpt_on ? P.rptq : ' ') : MY_REPT;         /* REPT */
    d[n++] = tochar(ack ? ((P.lp ? CAP_LP : 0) | (P.at ? CAP_AT : 0))
                        : (CAP_LP | CAP_AT));                   /* CAPAS */
    d[n++] = tochar(1);                     /* WINDO */
    d[n++] = tochar(MY_MAXLX / 95);         /* MAXLX1 */
    d[n++] = tochar(MY_MAXLX % 95);         /* MAXLX2 */
    return n;
}

/* Take the client's parameters from an I / S packet (ack=false: we
 * reply with an ACK) or from its ACK to our S (ack=true). */
static void take_params(const uint8_t *d, int n, bool from_ack)
{
    params_default();
    if (n >= 1) {
        P.maxl = unchar(d[0]);
        if (P.maxl < 10 || P.maxl > 94) P.maxl = 80;
    }
    if (n >= 2) {
        P.timo = unchar(d[1]);
        if (P.timo < 1) P.timo = 5;
        if (P.timo > 60) P.timo = 60;
    }
    if (n >= 3) P.npad = unchar(d[2]);
    if (P.npad < 0 || P.npad > 32) P.npad = 0;
    if (n >= 4) P.padc = ctl(d[3]);
    if (n >= 5) {
        P.eol = (uint8_t)unchar(d[4]);
        if (P.eol == 0 || P.eol >= 32) P.eol = '\r';
    }
    if (n >= 6 && is_prefix_char(d[5])) P.r_ctlq = d[5];
    if (n >= 7 && is_prefix_char(d[6])) {
        /* The client asks for 8th-bit prefixing with this character;
         * our QBIN 'Y' agrees. */
        P.ebq_on = true;
        P.ebq = d[6];
    }
    if (n >= 8) {
        int b = d[7] - '0';
        if (b == 5) b = 3;
        P.bct = (b >= 1 && b <= 3) ? b : 1;
    }
    if (n >= 9 && is_prefix_char(d[8]) &&
        (from_ack ? d[8] == MY_REPT : true)) {
        P.rpt_on = true;
        P.rptq = d[8];
    }
    int i = 9;
    if (n >= 10) {
        P.lp = (unchar(d[9]) & CAP_LP) != 0;
        P.at = (unchar(d[9]) & CAP_AT) != 0;
        while (i < n && (unchar(d[i]) & 1)) i++;   /* extra CAPAS bytes */
    }
    if (P.lp) {
        if (n > i + 3) {
            P.maxlx = unchar(d[i + 2]) * 95 + unchar(d[i + 3]);
            if (P.maxlx == 0) P.maxlx = 500;
        }
        if (P.maxlx > MY_MAXLX) P.maxlx = MY_MAXLX;
        if (P.maxlx < 40) P.lp = false;
    }
}

/* ---- Exchanges ---- */

static void send_ack(int seq, const uint8_t *data, int len)
{
    tx_packet('Y', seq, data, len, s_cur_bct, true);
}

static void send_nak(int seq)
{
    tx_packet('N', seq, NULL, 0, s_cur_bct, false);
}

static void send_error(int seq, const char *msg)
{
    int n = encode_str(msg, s_dbuf, 80);
    tx_packet('E', seq, s_dbuf, n, s_cur_bct, false);
    status(false, "Error: %s", msg);
}

static void report_peer_error(const kpkt_t *p)
{
    char msg[80];
    decode_str(p, msg, sizeof(msg));
    status(false, "Cancelled by the client: %s", msg[0] ? msg : "error");
}

/* Send a packet and wait for its ACK. Returns 1 when acknowledged (ack
 * holds the ACK, its data possibly empty), 0 when the exchange failed
 * (error reported to the client), -1 when the server is stopping. */
static int send_wait(char type, int seq, const uint8_t *d, int n, kpkt_t *ack)
{
    tx_packet(type, seq, d, n, s_cur_bct, true);
    for (int tries = 0;;) {
        int r = rx_packet(ack, P.timo);
        if (r == -1) return -1;
        if (r == 1) {
            if (ack->type == 'Y' && ack->seq == seq) return 1;
            if (ack->type == 'N' && ack->seq == ((seq + 1) & 63)) {
                ack->len = 0;   /* NAK for the next one = ACK */
                ack->data[0] = '\0';
                return 1;
            }
            if (ack->type == 'E') {
                report_peer_error(ack);
                return 0;
            }
            if (!(ack->type == 'N' && ack->seq == seq)) continue;
        }
        if (++tries > MAX_RETRY) {
            send_error(seq, "Too many retries");
            return 0;
        }
        resend();
    }
}

/* Wait for the client's packet number seq, re-ACKing duplicates of the
 * previous one and NAKing timeouts / corrupt packets. Returns 1, 0 or
 * -1 like send_wait(). */
static int recv_wait(int seq, kpkt_t *p)
{
    for (int tries = 0;;) {
        int r = rx_packet(p, P.timo);
        if (r == -1) return -1;
        if (r == 1) {
            if (p->type == 'E') {
                report_peer_error(p);
                return 0;
            }
            if (p->seq == seq) return 1;
            if (p->seq == ((seq + 63) & 63)) resend();   /* our ACK was lost */
        } else {
            send_nak(seq);
        }
        if (++tries > MAX_RETRY) {
            send_error(seq, "Too many retries");
            return 0;
        }
    }
}

/* ---- Paths ---- */

/* Resolve a client path (absolute = from the root, else from the
 * current directory) to a full path under the root. Returns false if
 * it would leave the root or is too long. rel (optional) receives the
 * path relative to the root, without leading slash. */
static bool resolve(const char *name, char *full, size_t cap, char *rel)
{
    char tmp[PATH_MAX_K];
    char out[PATH_MAX_K];
    size_t olen = 0;
    out[0] = '\0';

    if (name[0] == '/' || name[0] == '\\') {
        snprintf(tmp, sizeof(tmp), "%s", name);
    } else if (s_cwd[0]) {
        if ((size_t)snprintf(tmp, sizeof(tmp), "%s/%s", s_cwd, name) >= sizeof(tmp))
            return false;
    } else {
        snprintf(tmp, sizeof(tmp), "%s", name);
    }

    char *save = NULL;
    for (char *c = tmp; *c; c++) if (*c == '\\') *c = '/';
    for (char *tok = strtok_r(tmp, "/", &save); tok; tok = strtok_r(NULL, "/", &save)) {
        if (strcmp(tok, ".") == 0) continue;
        if (strcmp(tok, "..") == 0) {
            if (olen == 0) return false;
            char *sl = strrchr(out, '/');
            olen = sl ? (size_t)(sl - out) : 0;
            out[olen] = '\0';
            continue;
        }
        size_t tl = strlen(tok);
        if (olen + tl + 2 >= sizeof(out)) return false;
        if (olen) out[olen++] = '/';
        memcpy(&out[olen], tok, tl + 1);
        olen += tl;
    }
    if (rel) strcpy(rel, out);
    int w = olen ? snprintf(full, cap, "%s/%s", s_root, out)
                 : snprintf(full, cap, "%s", s_root);
    return w > 0 && (size_t)w < cap;
}

static bool has_wild(const char *s)
{
    return strpbrk(s, "*?") != NULL;
}

static bool wild_match(const char *pat, const char *s)
{
    while (*pat) {
        if (*pat == '*') {
            pat++;
            if (!*pat) return true;
            for (; *s; s++)
                if (wild_match(pat, s)) return true;
            return false;
        }
        if (!*s) return false;
        if (*pat != '?' && tolower((unsigned char)*pat) != tolower((unsigned char)*s))
            return false;
        pat++;
        s++;
    }
    return *s == '\0';
}

static bool is_dir(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

/* Split "dir/pattern" into the resolved directory and the last
 * component. Returns false for a path outside the root. */
static bool split_spec(const char *spec, char *dir, size_t dcap, char *pat, size_t pcap)
{
    const char *sl = strrchr(spec, '/');
    const char *bs = strrchr(spec, '\\');
    if (bs > sl) sl = bs;
    if (!sl) {
        snprintf(pat, pcap, "%s", spec);
        return resolve(".", dir, dcap, NULL);
    }
    char d[PATH_MAX_K];
    size_t dl = (size_t)(sl - spec);
    if (dl >= sizeof(d)) return false;
    memcpy(d, spec, dl);
    d[dl] = '\0';
    snprintf(pat, pcap, "%s", sl + 1);
    return resolve(dl ? d : "/", dir, dcap, NULL);
}

/* ---- Upload (client SENDs) ---- */

/* Lower-case a name that has no lower-case letters: C-Kermit sends
 * names in upper case by default ("common form") and converts them
 * back the same way when it receives. */
static void name_from_common_form(char *name)
{
    for (char *c = name; *c; c++)
        if (islower((unsigned char)*c)) return;
    for (char *c = name; *c; c++)
        *c = (char)tolower((unsigned char)*c);
}

/* Attribute packet: TAG, tochar(length), value, ... Returns true for a
 * text file (type attribute "A": CR LF line ends on the line). */
static bool attr_is_text(const kpkt_t *p)
{
    int i = 0;
    while (i + 1 < p->len) {
        uint8_t tag = p->data[i];
        int l = unchar(p->data[i + 1]);
        i += 2;
        if (l < 0 || i + l > p->len) break;
        if (tag == '"' && l > 0) return p->data[i] == 'A';
        i += l;
    }
    return false;
}

static void do_receive(const kpkt_t *sp)
{
    take_params(sp->data, sp->len, false);
    uint8_t prm[16];
    int pn = build_params(prm, true);
    s_cur_bct = 1;
    send_ack(sp->seq, prm, pn);
    s_cur_bct = P.bct;
    s_in_xfer = true;

    kpkt_t p;
    int seq = (sp->seq + 1) & 63;
    FILE *fp = NULL;
    char target[PATH_MAX_K + 8] = "";
    char part[PATH_MAX_K + 16] = "";
    char shown[64] = "";
    ksink_t sink = { 0 };
    uint8_t *wbuf = NULL;
    int64_t last_report = 0;
    int nfiles = 0;

    for (;;) {
        int r = recv_wait(seq, &p);
        if (r <= 0) break;

        if (p.type == 'F') {
            if (fp) {                   /* previous file never got its Z */
                fclose(fp);
                fp = NULL;
                unlink(part);
            }
            char name[PATH_MAX_K];
            decode_str(&p, name, sizeof(name));
            name_from_common_form(name);
            char rel[PATH_MAX_K];
            if (!name[0] || !resolve(name, target, sizeof(target), rel) ||
                !rel[0] || is_dir(target)) {
                send_error(p.seq, "Invalid file name");
                break;
            }
            snprintf(part, sizeof(part), "%s.kpart", target);
            fp = fopen(part, "wb");
            if (!fp) {
                send_error(p.seq, "Cannot create file");
                break;
            }
            if (!wbuf) wbuf = heap_caps_malloc(4096, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            if (!wbuf) {
                send_error(p.seq, "Out of memory");
                break;
            }
            sink = (ksink_t){ wbuf, 0, 4096, fp, false, 0, false, false };
            const char *base = strrchr(rel, '/');
            copy_trunc(shown, sizeof(shown), base ? base + 1 : rel);
            status(false, "Receiving %s", shown);
            last_report = now_ms();
            int n = encode_str(rel, s_dbuf, data_max(s_cur_bct));
            send_ack(p.seq, s_dbuf, n);
        } else if (p.type == 'A') {
            sink.text = attr_is_text(&p);
            send_ack(p.seq, NULL, 0);
        } else if (p.type == 'D') {
            if (!fp) {
                send_error(p.seq, "Data without a file");
                break;
            }
            decode_into(p.data, p.len, &sink);
            if (sink.err) {
                send_error(p.seq, "SD card write error");
                break;
            }
            send_ack(p.seq, NULL, 0);
            if (now_ms() - last_report >= 1000) {
                last_report = now_ms();
                status(false, "Receiving %s: %ld KB", shown, sink.total / 1024);
            }
        } else if (p.type == 'Z') {
            if (!fp) {
                send_ack(p.seq, NULL, 0);
            } else {
                sink_flush(&sink);
                bool ok = !sink.err && fclose(fp) == 0;
                fp = NULL;
                bool discard = p.len > 0 && p.data[0] == 'D';
                if (!ok || discard) {
                    unlink(part);
                    if (!ok) {
                        send_error(p.seq, "SD card write error");
                        break;
                    }
                    status(false, "Discarded %s", shown);
                } else {
                    unlink(target);
                    if (rename(part, target) != 0) {
                        unlink(part);
                        send_error(p.seq, "Cannot store file");
                        break;
                    }
                    nfiles++;
                    status(true, "Received %s (%ld bytes)", shown, sink.total);
                }
                send_ack(p.seq, NULL, 0);
            }
        } else if (p.type == 'B') {
            send_ack(p.seq, NULL, 0);
            break;
        } else {
            send_error(p.seq, "Unexpected packet");
            break;
        }
        seq = (seq + 1) & 63;
    }

    if (fp) {
        fclose(fp);
        unlink(part);
    }
    free(wbuf);
    if (nfiles > 1) status(true, "Received %d files", nfiles);
    s_in_xfer = false;
}

/* ---- Download / text replies (we send) ---- */

/* Start a send transaction: S packet, take the client's ACK. */
static int send_begin(void)
{
    uint8_t prm[16];
    int pn = build_params(prm, false);
    kpkt_t ack;
    s_cur_bct = 1;
    s_in_xfer = true;
    int r = send_wait('S', 0, prm, pn, &ack);
    if (r != 1) return r;
    take_params(ack.data, ack.len, true);
    s_cur_bct = P.bct;
    return 1;
}

/* Send one file (fp) or memory text (mem) under header type hdr ('F'
 * file name or 'X' text title). *seq is advanced. Returns 1 when sent,
 * 2 when the client asked to skip the rest of the batch, 0 / -1 on
 * failure / stop. */
static int send_one(int *seq, char hdr, const char *name, FILE *fp,
                    const char *mem, size_t mem_len)
{
    kpkt_t ack;
    int n = encode_str(name, s_dbuf, data_max(s_cur_bct));
    int r = send_wait(hdr, *seq, s_dbuf, n, &ack);
    if (r != 1) return r;
    *seq = (*seq + 1) & 63;

    bool refused = false;
    if (fp && P.at) {
        /* Binary type, exact size and modification time: the client
         * stores the bytes as they are on the card. */
        struct stat st;
        char a[64];
        int an = 0;
        a[an++] = '"';
        a[an++] = tochar(2);
        a[an++] = 'B';
        a[an++] = '8';
        if (fstat(fileno(fp), &st) == 0) {
            char v[24];
            int vl = snprintf(v, sizeof(v), "%ld", (long)st.st_size);
            a[an++] = '1';
            a[an++] = tochar(vl);
            memcpy(&a[an], v, vl);
            an += vl;
            struct tm tm;
            time_t mt = st.st_mtime;
            localtime_r(&mt, &tm);
            vl = (int)strftime(v, sizeof(v), "%Y%m%d %H:%M:%S", &tm);
            if (vl > 0) {
                a[an++] = '#';
                a[an++] = tochar(vl);
                memcpy(&a[an], v, vl);
                an += vl;
            }
        }
        r = send_wait('A', *seq, (const uint8_t *)a, an, &ack);
        if (r != 1) return r;
        *seq = (*seq + 1) & 63;
        refused = ack.len > 0 && ack.data[0] == 'N';
    }

    ksrc_t src = { 0 };
    src.fp = fp;
    src.mem = (const uint8_t *)mem;
    src.mem_len = mem_len;
    char zdata[1] = { 0 };
    int zlen = 0;
    int result = 1;
    int64_t last_report = now_ms();

    if (refused) {
        zdata[0] = 'D';
        zlen = 1;
    }
    while (!refused) {
        n = encode_from(&src, s_dbuf, data_max(s_cur_bct));
        if (n == 0) break;
        r = send_wait('D', *seq, s_dbuf, n, &ack);
        if (r != 1) return r;
        *seq = (*seq + 1) & 63;
        if (ack.len > 0 && (ack.data[0] == 'X' || ack.data[0] == 'Z')) {
            zdata[0] = 'D';             /* discard on the client side */
            zlen = 1;
            if (ack.data[0] == 'Z') result = 2;
            break;
        }
        if (fp && now_ms() - last_report >= 1000) {
            last_report = now_ms();
            status(false, "Sending %s: %ld KB", name, src.count / 1024);
        }
    }
    if (src.err) {
        /* A read error ends the data like the end of the file would:
         * tell the client to discard what it got, then abort, rather
         * than let a truncated copy pass for the file. */
        zdata[0] = 'D';
        zlen = 1;
    }
    r = send_wait('Z', *seq, (const uint8_t *)zdata, zlen, &ack);
    if (r != 1) return r;
    *seq = (*seq + 1) & 63;
    if (src.err) {
        status(true, "SD card read error: %s not sent", name);
        send_error(*seq, "SD card read error");
        return 0;
    }
    return result;
}

static void send_end(int seq)
{
    kpkt_t ack;
    send_wait('B', seq, NULL, 0, &ack);
}

/* Reply to a command with text: in the ACK when it fits, otherwise as
 * an X-headed text transfer. */
static void reply_text(int cmd_seq, const char *text)
{
    ksrc_t src = { 0 };
    src.mem = (const uint8_t *)text;
    src.mem_len = strlen(text);
    int max = data_max(s_cur_bct);
    int n = encode_from(&src, s_dbuf, max);
    if (src.mem_pos >= src.mem_len) {
        send_ack(cmd_seq, s_dbuf, n);
        return;
    }
    if (send_begin() != 1) return;
    int seq = 1;
    if (send_one(&seq, 'X', "", NULL, text, strlen(text)) >= 1)
        send_end(seq);
}

/* Growable text buffer for listings. */
typedef struct {
    char  *s;
    size_t len, cap;
} ktext_t;

static void text_add(ktext_t *t, const char *fmt, ...)
{
    char line[160];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);
    if (n < 0) return;
    if ((size_t)n >= sizeof(line)) n = sizeof(line) - 1;
    if (t->len + n + 1 > t->cap) {
        size_t nc = t->cap ? t->cap * 2 : 2048;
        while (nc < t->len + n + 1) nc *= 2;
        char *ns = heap_caps_realloc(t->s, nc, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!ns) return;
        t->s = ns;
        t->cap = nc;
    }
    memcpy(t->s + t->len, line, n + 1);
    t->len += n;
}

/* Collect the names of the regular files in dir matching pat. */
typedef struct {
    char (*names)[256];
    int count, cap;
} klist_t;

static void list_matches(const char *dir, const char *pat, klist_t *l)
{
    DIR *d = opendir(dir);
    if (!d) return;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (!wild_match(pat, e->d_name)) continue;
        char path[PATH_BUF];
        snprintf(path, sizeof(path), "%s/%s", dir, e->d_name);
        if (is_dir(path)) continue;
        if (l->count >= l->cap) {
            int nc = l->cap ? l->cap * 2 : 16;
            void *nn = heap_caps_realloc(l->names, (size_t)nc * 256,
                                         MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            if (!nn) break;
            l->names = nn;
            l->cap = nc;
        }
        snprintf(l->names[l->count++], 256, "%s", e->d_name);
    }
    closedir(d);
}

static void do_get(const kpkt_t *p)
{
    char spec[PATH_MAX_K];
    decode_str(p, spec, sizeof(spec));
    char dir[PATH_MAX_K + 8], pat[256];
    if (!spec[0] || !split_spec(spec, dir, sizeof(dir), pat, sizeof(pat))) {
        send_error(p->seq, "Invalid file name");
        return;
    }
    klist_t l = { 0 };
    if (has_wild(pat)) {
        list_matches(dir, pat, &l);
    } else {
        char path[PATH_BUF];
        snprintf(path, sizeof(path), "%s/%s", dir, pat);
        struct stat st;
        if (stat(path, &st) == 0 && !S_ISDIR(st.st_mode)) {
            l.names = heap_caps_malloc(256, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            if (l.names) {
                snprintf(l.names[0], 256, "%s", pat);
                l.count = l.cap = 1;
            }
        }
    }
    if (l.count == 0) {
        send_error(p->seq, "File not found");
        free(l.names);
        return;
    }

    if (send_begin() == 1) {
        int seq = 1;
        int sent = 0;
        int r = 1;
        for (int i = 0; i < l.count && r == 1; i++) {
            char path[PATH_BUF];
            snprintf(path, sizeof(path), "%s/%s", dir, l.names[i]);
            FILE *fp = fopen(path, "rb");
            if (!fp) {
                send_error(seq, "Cannot open file");
                r = 0;
                break;
            }
            status(false, "Sending %s", l.names[i]);
            r = send_one(&seq, 'F', l.names[i], fp, NULL, 0);
            fclose(fp);
            if (r >= 1) {
                sent++;
                status(false, "Sent %s", l.names[i]);
            }
        }
        if (r >= 1) {
            send_end(seq);
            if (sent > 1) status(false, "Sent %d files", sent);
        }
    }
    free(l.names);
}

/* ---- Generic (REMOTE) commands ---- */

/* Operand i (0-based) of a decoded G packet: tochar(length) + text. */
static bool g_arg(const char *g, int idx, char *out, size_t cap)
{
    const char *p = g + 1;
    for (int i = 0;; i++) {
        if (!*p) {
            out[0] = '\0';
            return false;
        }
        int l = unchar((uint8_t)*p++);
        if (l < 0) l = 0;
        size_t avail = strlen(p);
        if ((size_t)l > avail) l = (int)avail;
        if (i == idx) {
            size_t c = (size_t)l < cap - 1 ? (size_t)l : cap - 1;
            memcpy(out, p, c);
            out[c] = '\0';
            return true;
        }
        p += l;
    }
}

static void g_directory(int seq, const char *spec)
{
    char dir[PATH_MAX_K + 8], pat[256];
    if (!spec[0]) {
        if (!resolve(".", dir, sizeof(dir), NULL)) return;
        strcpy(pat, "*");
    } else {
        char full[PATH_MAX_K + 8];
        if (!has_wild(spec) && resolve(spec, full, sizeof(full), NULL) && is_dir(full)) {
            snprintf(dir, sizeof(dir), "%s", full);
            strcpy(pat, "*");
        } else if (!split_spec(spec, dir, sizeof(dir), pat, sizeof(pat))) {
            send_error(seq, "Invalid directory");
            return;
        }
    }
    DIR *d = opendir(dir);
    if (!d) {
        send_error(seq, "Cannot open directory");
        return;
    }
    ktext_t t = { 0 };
    int count = 0;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (!wild_match(pat, e->d_name)) continue;
        char path[PATH_BUF];
        snprintf(path, sizeof(path), "%s/%s", dir, e->d_name);
        struct stat st;
        if (stat(path, &st) != 0) continue;
        struct tm tm;
        time_t mt = st.st_mtime;
        localtime_r(&mt, &tm);
        if (S_ISDIR(st.st_mode)) {
            text_add(&t, "%-36s %10s  %04d-%02d-%02d %02d:%02d\r\n",
                     e->d_name, "<DIR>", tm.tm_year + 1900, tm.tm_mon + 1,
                     tm.tm_mday, tm.tm_hour, tm.tm_min);
        } else {
            text_add(&t, "%-36s %10ld  %04d-%02d-%02d %02d:%02d\r\n",
                     e->d_name, (long)st.st_size, tm.tm_year + 1900,
                     tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min);
        }
        count++;
    }
    closedir(d);
    if (count == 0) text_add(&t, "No files\r\n");
    reply_text(seq, t.s ? t.s : "");
    free(t.s);
}

static void g_delete(int seq, const char *spec)
{
    char dir[PATH_MAX_K + 8], pat[256];
    if (!spec[0] || !split_spec(spec, dir, sizeof(dir), pat, sizeof(pat))) {
        send_error(seq, "Invalid file name");
        return;
    }
    klist_t l = { 0 };
    list_matches(dir, pat, &l);
    int deleted = 0;
    for (int i = 0; i < l.count; i++) {
        char path[PATH_BUF];
        snprintf(path, sizeof(path), "%s/%s", dir, l.names[i]);
        if (unlink(path) == 0) deleted++;
    }
    free(l.names);
    if (deleted == 0) {
        send_error(seq, "No files deleted");
        return;
    }
    char msg[64];
    snprintf(msg, sizeof(msg), "%d file(s) deleted", deleted);
    status(true, "Deleted %d file(s)", deleted);
    reply_text(seq, msg);
}

static void g_type(int seq, const char *name)
{
    char path[PATH_MAX_K + 8];
    if (!name[0] || !resolve(name, path, sizeof(path), NULL) || is_dir(path)) {
        send_error(seq, "Invalid file name");
        return;
    }
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        send_error(seq, "File not found");
        return;
    }
    if (send_begin() == 1) {
        int s = 1;
        if (send_one(&s, 'X', name, fp, NULL, 0) >= 1) send_end(s);
    }
    fclose(fp);
}

static void do_generic(const kpkt_t *p)
{
    char g[2 * PATH_MAX_K + 8];
    decode_str(p, g, sizeof(g));
    char a[PATH_MAX_K], b[PATH_MAX_K];
    char full[PATH_MAX_K + 8], full2[PATH_MAX_K + 8];
    char rel[PATH_MAX_K];
    char msg[PATH_MAX_K + 64];
    g_arg(g, 0, a, sizeof(a));

    switch (g[0]) {
    case 'C':   /* CD */
        if (!a[0]) {
            s_cwd[0] = '\0';
        } else if (!resolve(a, full, sizeof(full), rel) || !is_dir(full)) {
            send_error(p->seq, "No such directory");
            return;
        } else {
            snprintf(s_cwd, sizeof(s_cwd), "%s", rel);
        }
        snprintf(msg, sizeof(msg), "/%s", s_cwd);
        reply_text(p->seq, msg);
        break;
    case 'A':   /* PWD */
        snprintf(msg, sizeof(msg), "/%s", s_cwd);
        reply_text(p->seq, msg);
        break;
    case 'D':   /* DIRECTORY */
        g_directory(p->seq, a);
        break;
    case 'E':   /* DELETE */
        g_delete(p->seq, a);
        break;
    case 'R':   /* RENAME */
        g_arg(g, 1, b, sizeof(b));
        if (!a[0] || !b[0] || !resolve(a, full, sizeof(full), NULL) ||
            !resolve(b, full2, sizeof(full2), rel) || !rel[0]) {
            send_error(p->seq, "Invalid file name");
            return;
        }
        if (rename(full, full2) != 0) {
            send_error(p->seq, "Rename failed");
            return;
        }
        status(true, "Renamed %s", a);
        reply_text(p->seq, "Renamed");
        break;
    case 'm':   /* MKDIR */
        if (!a[0] || !resolve(a, full, sizeof(full), rel) || !rel[0] ||
            mkdir(full, 0775) != 0) {
            send_error(p->seq, "Cannot create directory");
            return;
        }
        status(true, "Created directory %s", a);
        reply_text(p->seq, "Directory created");
        break;
    case 'd':   /* RMDIR */
        if (!a[0] || !resolve(a, full, sizeof(full), rel) || !rel[0] ||
            rmdir(full) != 0) {
            send_error(p->seq, "Cannot remove directory");
            return;
        }
        status(true, "Removed directory %s", a);
        reply_text(p->seq, "Directory removed");
        break;
    case 'U': { /* SPACE */
        struct statvfs vfs;
        if (statvfs(s_root, &vfs) != 0) {
            send_error(p->seq, "Cannot read disk usage");
            return;
        }
        unsigned long long bs = vfs.f_frsize ? vfs.f_frsize : vfs.f_bsize;
        snprintf(msg, sizeof(msg), "Total: %llu KB, free: %llu KB",
                 (unsigned long long)vfs.f_blocks * bs / 1024,
                 (unsigned long long)vfs.f_bavail * bs / 1024);
        reply_text(p->seq, msg);
        break;
    }
    case 'T':   /* TYPE */
        g_type(p->seq, a);
        break;
    case 'H':   /* HELP */
        reply_text(p->seq,
                   "Draftling Kermit server. Files are on the SD card.\r\n"
                   "SEND, GET, REMOTE: CD, PWD, DIRECTORY, DELETE, RENAME,\r\n"
                   "MKDIR, RMDIR, SPACE, TYPE, HELP. FINISH and BYE end the\r\n"
                   "session; the server keeps running until it is stopped\r\n"
                   "on the device.\r\n");
        break;
    case 'F':   /* FINISH */
    case 'L':   /* BYE */
        send_ack(p->seq, NULL, 0);
        status(false, "Client finished");
        break;
    default:
        send_error(p->seq, "Unimplemented server command");
        break;
    }
}

/* ---- Server task ---- */

static void server_loop(void)
{
    kpkt_t p;
    while (!s_stop) {
        s_cur_bct = 1;
        int r = rx_packet(&p, 0);
        if (r == -1) break;
        if (r != 1) continue;
        s_cur_bct = p.bct;
        switch (p.type) {
        case 'I': {
            take_params(p.data, p.len, false);
            uint8_t prm[16];
            int pn = build_params(prm, true);
            s_cur_bct = 1;
            send_ack(p.seq, prm, pn);
            break;
        }
        case 'S':
            do_receive(&p);
            break;
        case 'R':
            do_get(&p);
            break;
        case 'G':
            do_generic(&p);
            break;
        case 'E':
        case 'Y':
        case 'N':
            break;
        default:
            send_error(p.seq, "Unimplemented server command");
            break;
        }
        s_in_xfer = false;
    }
}

static void server_task(void *arg)
{
    (void)arg;
    status(false, "Waiting for the Kermit client");
    server_loop();
    if (s_in_xfer) {
        /* Tell the client the transfer is over. */
        int n = encode_str("Server stopped", s_dbuf, 80);
        tx_packet('E', 0, s_dbuf, n, s_cur_bct, false);
    }
    kport_close();
    heap_caps_free(s_rx);
    heap_caps_free(s_tx);
    heap_caps_free(s_dbuf);
    s_rx = s_tx = s_dbuf = NULL;
    s_running = false;
    xSemaphoreGive(s_done);
    vTaskDelete(NULL);
}

esp_err_t kermit_server_start(const char *root_dir, kermit_status_cb_t cb)
{
    if (s_running) return ESP_ERR_INVALID_STATE;
    if (!root_dir || strlen(root_dir) >= sizeof(s_root)) return ESP_ERR_INVALID_ARG;
    if (!s_done) {
        s_done = xSemaphoreCreateBinary();
        if (!s_done) return ESP_ERR_NO_MEM;
    }
    if (!s_stack) {
        s_stack = heap_caps_malloc(TASK_STACK, MALLOC_CAP_SPIRAM);
        if (!s_stack) return ESP_ERR_NO_MEM;
    }
    xSemaphoreTake(s_done, 0);

    s_rx   = heap_caps_malloc(PKT_BUF, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    s_tx   = heap_caps_malloc(PKT_BUF, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    s_dbuf = heap_caps_malloc(PKT_BUF, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_rx || !s_tx || !s_dbuf) {
        heap_caps_free(s_rx);
        heap_caps_free(s_tx);
        heap_caps_free(s_dbuf);
        s_rx = s_tx = s_dbuf = NULL;
        return ESP_ERR_NO_MEM;
    }

    snprintf(s_root, sizeof(s_root), "%s", root_dir);
    s_cwd[0] = '\0';
    s_cb = cb;
    s_stop = false;
    s_in_xfer = false;
    s_in_pos = s_in_len = 0;
    s_tx_len = 0;
    s_cur_bct = 1;
    params_default();

    esp_err_t err = kport_open();
    if (err != ESP_OK) {
        heap_caps_free(s_rx);
        heap_caps_free(s_tx);
        heap_caps_free(s_dbuf);
        s_rx = s_tx = s_dbuf = NULL;
        return err;
    }

    s_running = true;
    if (!xTaskCreateStatic(server_task, "kermit", TASK_STACK / sizeof(StackType_t),
                           NULL, 4, s_stack, &s_tcb)) {
        s_running = false;
        kport_close();
        heap_caps_free(s_rx);
        heap_caps_free(s_tx);
        heap_caps_free(s_dbuf);
        s_rx = s_tx = s_dbuf = NULL;
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

void kermit_server_stop(void)
{
    if (!s_running) return;
    s_stop = true;
    if (xSemaphoreTake(s_done, pdMS_TO_TICKS(5000)) != pdTRUE) {
        /* Still finishing an SD card operation; it exits on its own
         * and kermit_server_is_running() stays true until then. */
        return;
    }
}

bool kermit_server_is_running(void)
{
    return s_running;
}

int kermit_server_usb_connected(void)
{
    if (!s_running) return 0;
    return kport_usb_connected();
}
