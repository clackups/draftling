/* Linux: the ESP32 ROM's zlib is miniz 1.x; the same release (v114,
 * fetched into build/ by the Makefile) is compiled once as its own
 * object (draftling.mk) and declared here. */
#pragma once
#define MINIZ_HEADER_FILE_ONLY
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#include "miniz.c"
