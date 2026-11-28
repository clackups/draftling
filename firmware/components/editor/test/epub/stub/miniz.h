/* The ROM's zlib is miniz 1.x: the same release (v114, fetched by
 * run.sh) is compiled as its own object and declared here, as the
 * Linux build does (linux/shim/include/miniz.h). */
#pragma once
#define MINIZ_HEADER_FILE_ONLY
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#include "miniz.c"
