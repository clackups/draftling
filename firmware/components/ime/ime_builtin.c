/* Dictionaries embedded in the firmware image (EMBED_FILES). */

#include "sdkconfig.h"
#include "ime.h"

#ifdef CONFIG_KB_LAYOUT_ENABLE_JA
extern const uint8_t _binary_ja_bin_start[];
#endif
#ifdef CONFIG_KB_LAYOUT_ENABLE_ZH
extern const uint8_t _binary_zh_bin_start[];
#endif

const uint8_t *ime_builtin_ja_dict(void)
{
#ifdef CONFIG_KB_LAYOUT_ENABLE_JA
    return _binary_ja_bin_start;
#else
    return NULL;
#endif
}

const uint8_t *ime_builtin_zh_dict(void)
{
#ifdef CONFIG_KB_LAYOUT_ENABLE_ZH
    return _binary_zh_bin_start;
#else
    return NULL;
#endif
}
