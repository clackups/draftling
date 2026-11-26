/*
 * Web client: SHA-1 for the Git client's object ids (FIPS 180-4),
 * behind the mbedtls_sha1_* API of shim/include/mbedtls/sha1.h.
 */

#include <string.h>

#include "mbedtls/sha1.h"

static uint32_t rol(uint32_t x, int n)
{
    return (x << n) | (x >> (32 - n));
}

static void block(mbedtls_sha1_context *c, const uint8_t *p)
{
    uint32_t w[80];
    for (int i = 0; i < 16; i++)
        w[i] = (uint32_t)p[4 * i] << 24 | (uint32_t)p[4 * i + 1] << 16 |
               (uint32_t)p[4 * i + 2] << 8 | p[4 * i + 3];
    for (int i = 16; i < 80; i++) w[i] = rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    uint32_t a = c->h[0], b = c->h[1], cc = c->h[2], d = c->h[3], e = c->h[4];
    for (int i = 0; i < 80; i++) {
        uint32_t f, k;
        if (i < 20)      { f = (b & cc) | (~b & d);           k = 0x5A827999u; }
        else if (i < 40) { f = b ^ cc ^ d;                    k = 0x6ED9EBA1u; }
        else if (i < 60) { f = (b & cc) | (b & d) | (cc & d); k = 0x8F1BBCDCu; }
        else             { f = b ^ cc ^ d;                    k = 0xCA62C1D6u; }
        uint32_t t = rol(a, 5) + f + e + k + w[i];
        e = d;
        d = cc;
        cc = rol(b, 30);
        b = a;
        a = t;
    }
    c->h[0] += a;
    c->h[1] += b;
    c->h[2] += cc;
    c->h[3] += d;
    c->h[4] += e;
}

void mbedtls_sha1_init(mbedtls_sha1_context *c)
{
    memset(c, 0, sizeof(*c));
}

int mbedtls_sha1_starts(mbedtls_sha1_context *c)
{
    static const uint32_t iv[5] = { 0x67452301u, 0xEFCDAB89u, 0x98BADCFEu,
                                    0x10325476u, 0xC3D2E1F0u };
    memcpy(c->h, iv, sizeof(iv));
    c->len = 0;
    c->used = 0;
    return 0;
}

int mbedtls_sha1_update(mbedtls_sha1_context *c, const unsigned char *d, size_t n)
{
    c->len += n;
    if (c->used) {
        size_t take = 64 - c->used < n ? 64 - c->used : n;
        memcpy(c->buf + c->used, d, take);
        c->used += take;
        d += take;
        n -= take;
        if (c->used < 64) return 0;
        block(c, c->buf);
        c->used = 0;
    }
    for (; n >= 64; d += 64, n -= 64) block(c, d);
    memcpy(c->buf, d, n);
    c->used = n;
    return 0;
}

int mbedtls_sha1_finish(mbedtls_sha1_context *c, unsigned char out[20])
{
    uint64_t bits = c->len * 8;
    uint8_t pad[72] = { 0x80 };
    size_t padn = (c->used < 56 ? 56 : 120) - c->used;
    uint8_t lenb[8];
    for (int i = 0; i < 8; i++) lenb[i] = (uint8_t)(bits >> (56 - 8 * i));
    mbedtls_sha1_update(c, pad, padn);
    mbedtls_sha1_update(c, lenb, 8);
    for (int i = 0; i < 5; i++) {
        out[4 * i]     = (uint8_t)(c->h[i] >> 24);
        out[4 * i + 1] = (uint8_t)(c->h[i] >> 16);
        out[4 * i + 2] = (uint8_t)(c->h[i] >> 8);
        out[4 * i + 3] = (uint8_t)c->h[i];
    }
    return 0;
}

void mbedtls_sha1_free(mbedtls_sha1_context *c)
{
    memset(c, 0, sizeof(*c));
}
