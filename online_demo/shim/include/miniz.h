/* Online demo: no zlib is built in, so compressed EPUB entries cannot
 * be read (epub.cpp then reports the book as damaged); stored entries
 * still work. Declares what epub.cpp uses, as the ROM's miniz.h does,
 * with an inflater that always fails. */
#pragma once
#include <stddef.h>
#include <stdint.h>
typedef unsigned char mz_uint8;
typedef uint32_t mz_uint32;
typedef enum { TINFL_STATUS_FAILED = -1, TINFL_STATUS_DONE = 0 } tinfl_status;
enum { TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF = 4 };
typedef struct { mz_uint32 m_state; } tinfl_decompressor;
#define tinfl_init(r) do { (r)->m_state = 0; } while (0)
static inline tinfl_status tinfl_decompress(tinfl_decompressor *r, const mz_uint8 *in,
                                            size_t *in_size, mz_uint8 *out_start,
                                            mz_uint8 *out_next, size_t *out_size,
                                            const mz_uint32 flags)
{
    (void)r; (void)in; (void)in_size; (void)out_start; (void)out_next; (void)flags;
    *out_size = 0;
    return TINFL_STATUS_FAILED;
}
