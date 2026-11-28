/* The one miniz call epub.cpp makes (raw deflate, memory to memory),
 * done with the host's zlib. */
#pragma once
#include <stddef.h>
#include <zlib.h>
#define TINFL_DECOMPRESS_MEM_TO_MEM_FAILED ((size_t)(-1))
static inline size_t tinfl_decompress_mem_to_mem(void *out, size_t out_len,
                                                 const void *in, size_t in_len, int flags)
{
    (void)flags;
    z_stream z = {};
    if (inflateInit2(&z, -15) != Z_OK) return TINFL_DECOMPRESS_MEM_TO_MEM_FAILED;
    z.next_in = (Bytef *)in;
    z.avail_in = (uInt)in_len;
    z.next_out = (Bytef *)out;
    z.avail_out = (uInt)out_len;
    int r = inflate(&z, Z_FINISH);
    size_t n = z.total_out;
    inflateEnd(&z);
    return r == Z_STREAM_END ? n : TINFL_DECOMPRESS_MEM_TO_MEM_FAILED;
}
