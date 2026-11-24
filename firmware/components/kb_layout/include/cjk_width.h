/* cjk_width.h -- the code points drawn two monospace cells wide (CJK
 * ideographs, kana, Hangul syllables, full-width forms). Shared by the
 * CJK font driver, the input methods and the editor's width estimates,
 * which must agree on it. Header-only, no dependencies. */
#pragma once

#include <stdbool.h>
#include <stdint.h>

static inline bool cjk_cp_is_wide(uint32_t cp)
{
    return (cp >= 0x2E80 && cp <= 0x2FDF) ||
           (cp >= 0x3000 && cp <= 0x33FF) ||
           (cp >= 0x3400 && cp <= 0x4DBF) ||
           (cp >= 0x4E00 && cp <= 0x9FFF) ||
           (cp >= 0xAC00 && cp <= 0xD7A3) ||
           (cp >= 0xF900 && cp <= 0xFAFF) ||
           (cp >= 0xFE30 && cp <= 0xFE4F) ||
           (cp >= 0xFF01 && cp <= 0xFF60) ||
           (cp >= 0xFFE0 && cp <= 0xFFE6);
}
