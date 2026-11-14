#!/bin/bash
# Host test of the Devanagari font: builds deva_font.c and the vendored
# HarfBuzz natively (stub LVGL / sdkconfig headers), links the
# generated font data in and shapes and draws sample lines; the image
# is written to $OUT/deva.pgm.
#
#   ./run.sh            build into ./out and run
#   OUT=/tmp/x ./run.sh build and run elsewhere
set -e
H=$(cd "$(dirname "$0")" && pwd)
F=$(cd "$H/../.." && pwd)
HB=$(cd "$F/../harfbuzz" && pwd)
OUT=${OUT:-$H/out}
mkdir -p "$OUT"
if [ ! -f "$OUT/hb.o" ] || [ "$HB/src/harfbuzz.cc" -nt "$OUT/hb.o" ]; then
    g++ -O1 -DHB_TINY -DHB_NO_MT -DHB_CONFIG_OVERRIDE_LAST_H='"hb_draftling_config.h"' \
        -I"$HB/config" -c "$HB/src/harfbuzz.cc" -o "$OUT/hb.o"
fi
# Link the font data in under the names ESP-IDF's EMBED_FILES gives it.
(cd "$F/deva" && ld -r -b binary -o "$OUT/blobs.o" deva_layout.ttf deva_lowres_*.bin)
CFLAGS="-O1 -Wall -Wextra -Werror -I$H/stub -I$F/include -I$F -I$HB/src -DHB_TINY -DHB_NO_MT"
gcc $CFLAGS -c "$F/deva_font.c" -o "$OUT/deva_font.o"
gcc $CFLAGS -c "$H/test_deva.c" -o "$OUT/test_deva.o"
g++ "$OUT/deva_font.o" "$OUT/test_deva.o" "$OUT/hb.o" "$OUT/blobs.o" -lm -o "$OUT/test_deva"
cd "$OUT" && ./test_deva
