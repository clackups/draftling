/* Draftling's HarfBuzz configuration overrides, included after
 * hb-config.hh (HB_CONFIG_OVERRIDE_LAST_H, see CMakeLists.txt). */
#pragma once

/* HB_TINY turns on HB_NO_ERRNO, which replaces errno with a
 * thread-local variable of its own; that clashes with the errno of
 * the ESP-IDF C library. Use the real one. */
#undef HB_NO_ERRNO
