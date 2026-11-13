#!/bin/sh
# Regenerates the Latin Extended-A subset fonts of the default language
# pack: lowres_latina_NN.c and hires_latina_NN.c.
#
# Coverage: Latin Extended-A (U+0100..U+017F: Polish, Czech, Slovak,
# Hungarian, Croatian, Slovenian, Turkish, Baltic, ...), the Romanian
# s / t with comma below (U+0218..U+021B), the spacing accents typed by
# the dead keys (caron U+02C7, breve U+02D8, dot above U+02D9, ring
# U+02DA, ogonek U+02DB, double acute U+02DD) and the low / high double
# quotes (U+201D, U+201E) of the Polish and Romanian layouts.
#
# Greybeard and Hack both cover all of it.
#
# Usage: gen_latina.sh <lv_font_conv.js> <greybeard ttf dir/> \
#                      <Hack-Regular.ttf> <out dir>
# lv_font_conv must be commit 1f44ab4 (see AGENTS.md, "Reproducing the
# committed fonts"); Greybeard v1.0.0, Hack v3.003.
set -e
CONV="node $1"; GB="$2"; HACK="$3"; OUT="$4"

RANGE=0x100-0x17F,0x218-0x21B,0x2C7,0x2D8-0x2DB,0x2DD,0x201D-0x201E

fixup() {
    # Replace lv_font_conv's include block with a plain #include "lvgl.h"
    # (see AGENTS.md, "Post-generation Fix-up").
    python3 - "$1" <<'PY'
import re, sys
p = sys.argv[1]
s = open(p).read()
s = re.sub(r'#ifdef __has_include.*?#endif\n\n#ifdef LV_LVGL_H_INCLUDE_SIMPLE.*?#endif\n',
           '#include "lvgl.h"\n', s, count=1, flags=re.S)
open(p, 'w').write(s)
PY
}

snap() {
    # Snap every non-zero advance to the Hack cell width (1/16 px units)
    # so the monospace grid stays aligned, as for the other Hack fonts.
    python3 - "$1" "$2" <<'PY'
import re, sys
p, cell = sys.argv[1], int(sys.argv[2]) * 16
s = open(p).read()
s = re.sub(r'\.adv_w = ([1-9][0-9]*)', '.adv_w = %d' % cell, s)
open(p, 'w').write(s)
PY
}

gb() { # slot ttf_px size extra_opts
    n=lowres_latina_$1
    $CONV --font "${GB}Greybeard-$2px.ttf" $4 -r $RANGE --size $3 --bpp 1 \
        --format lvgl --no-compress --lv-fallback lowres_$1_la_next \
        --lv-font-name $n -o "$OUT/$n.c"
    fixup "$OUT/$n.c"
}
gb 11 11 11
gb 14 14 14
gb 16 16 16
gb 18 18 18 --autohint-off
gb 22 22 22 --autohint-off
gb 26 22 26 --autohint-off
gb 30 15 30
gb 34 17 34

hk() { # slot size cell
    n=hires_latina_$1
    $CONV --font "$HACK" -r $RANGE --size $2 --bpp 1 \
        --format lvgl --no-compress --lv-fallback hires_$1_la_next \
        --lv-font-name $n -o "$OUT/$n.c"
    fixup "$OUT/$n.c"
    snap "$OUT/$n.c" $3
}
hk 9 16 10
hk 11 19 11
hk 14 21 13
hk 16 25 15
hk 18 28 17
hk 22 34 21
hk 26 41 25
hk 30 47 28
