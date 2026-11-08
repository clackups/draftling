#pragma once
#include <stddef.h>
static inline int mbedtls_base64_encode(unsigned char *dst, size_t dlen, size_t *olen,
                                        const unsigned char *src, size_t slen)
{
    static const char *t = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t n = 0;
    for (size_t i = 0; i < slen; i += 3) {
        unsigned v = src[i] << 16 | (i + 1 < slen ? src[i + 1] << 8 : 0) | (i + 2 < slen ? src[i + 2] : 0);
        if (n + 4 > dlen) return -1;
        dst[n++] = t[(v >> 18) & 63]; dst[n++] = t[(v >> 12) & 63];
        dst[n++] = i + 1 < slen ? t[(v >> 6) & 63] : '=';
        dst[n++] = i + 2 < slen ? t[v & 63] : '=';
    }
    *olen = n;
    return 0;
}
