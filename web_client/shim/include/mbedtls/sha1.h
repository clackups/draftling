/* Web client: the mbedtls_sha1_* calls of the Git client
 * (git_util.c), on a small SHA-1 of our own (src/sha1_web.c) -- the
 * browser's crypto.subtle is asynchronous and has no streaming API.
 * Overrides linux/shim/include/mbedtls/sha1.h, which uses OpenSSL. */
#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    uint32_t h[5];
    uint64_t len;          /* bytes hashed so far */
    uint8_t buf[64];
    size_t used;           /* bytes in buf */
} mbedtls_sha1_context;
void mbedtls_sha1_init(mbedtls_sha1_context *c);
int mbedtls_sha1_starts(mbedtls_sha1_context *c);
int mbedtls_sha1_update(mbedtls_sha1_context *c, const unsigned char *d, size_t n);
int mbedtls_sha1_finish(mbedtls_sha1_context *c, unsigned char out[20]);
void mbedtls_sha1_free(mbedtls_sha1_context *c);
#ifdef __cplusplus
}
#endif
