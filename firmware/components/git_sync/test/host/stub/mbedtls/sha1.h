#pragma once
#define OPENSSL_SUPPRESS_DEPRECATED
#include <openssl/sha.h>
typedef SHA_CTX mbedtls_sha1_context;
static inline void mbedtls_sha1_init(mbedtls_sha1_context *c) { (void)c; }
static inline int mbedtls_sha1_starts(mbedtls_sha1_context *c) { return SHA1_Init(c) ? 0 : -1; }
static inline int mbedtls_sha1_update(mbedtls_sha1_context *c, const unsigned char *d, size_t n) { return SHA1_Update(c, d, n) ? 0 : -1; }
static inline int mbedtls_sha1_finish(mbedtls_sha1_context *c, unsigned char o[20]) { return SHA1_Final(o, c) ? 0 : -1; }
static inline void mbedtls_sha1_free(mbedtls_sha1_context *c) { (void)c; }
