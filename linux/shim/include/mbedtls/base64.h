/* Linux: mbedtls_base64_encode() on OpenSSL. */
#pragma once
#include <stddef.h>
#include <openssl/evp.h>
#define MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL -0x002A
static inline int mbedtls_base64_encode(unsigned char *dst, size_t dlen, size_t *olen,
                                        const unsigned char *src, size_t slen)
{
    size_t need = (slen + 2) / 3 * 4 + 1;
    if (!dst || dlen < need) {
        *olen = need;
        return MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL;
    }
    *olen = (size_t)EVP_EncodeBlock(dst, src, (int)slen);
    return 0;
}
