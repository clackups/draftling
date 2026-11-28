/* Online demo: no zlib is built in, so compressed EPUB entries cannot
 * be read (epub.cpp then reports the book as damaged); stored entries
 * still work. The one call epub.cpp makes, declared as in the ROM's
 * miniz.h. */
#pragma once
#include <stddef.h>
#define TINFL_DECOMPRESS_MEM_TO_MEM_FAILED ((size_t)(-1))
static inline size_t tinfl_decompress_mem_to_mem(void *out, size_t out_len,
                                                 const void *in, size_t in_len, int flags)
{
    (void)out; (void)out_len; (void)in; (void)in_len; (void)flags;
    return TINFL_DECOMPRESS_MEM_TO_MEM_FAILED;
}
