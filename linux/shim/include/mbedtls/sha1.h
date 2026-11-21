/* Linux: mbedtls_sha1_* on OpenSSL's EVP digest API. */
#pragma once
#include <stddef.h>
#include <openssl/evp.h>
typedef struct { EVP_MD_CTX *ctx; } mbedtls_sha1_context;
static inline void mbedtls_sha1_init(mbedtls_sha1_context *c) { c->ctx = EVP_MD_CTX_new(); }
static inline int mbedtls_sha1_starts(mbedtls_sha1_context *c)
{
    return c->ctx && EVP_DigestInit_ex(c->ctx, EVP_sha1(), NULL) == 1 ? 0 : -1;
}
static inline int mbedtls_sha1_update(mbedtls_sha1_context *c, const unsigned char *d, size_t n)
{
    return c->ctx && EVP_DigestUpdate(c->ctx, d, n) == 1 ? 0 : -1;
}
static inline int mbedtls_sha1_finish(mbedtls_sha1_context *c, unsigned char out[20])
{
    unsigned int n = 0;
    return c->ctx && EVP_DigestFinal_ex(c->ctx, out, &n) == 1 && n == 20 ? 0 : -1;
}
static inline void mbedtls_sha1_free(mbedtls_sha1_context *c)
{
    EVP_MD_CTX_free(c->ctx);
    c->ctx = NULL;
}
