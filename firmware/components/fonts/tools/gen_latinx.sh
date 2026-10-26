#!/bin/sh
# Regenerates the Latin-extension subset fonts of the Vietnamese,
# Korean, Japanese and Chinese language packs: greybeard_latinx_NN.c
# and hack_latinx_NN.c.
#
# Coverage: the Vietnamese letters (U+0102/0103, U+0110/0111,
# U+0128/0129, U+0168/0169, U+01A0/01A1, U+01AF/01B0 and Latin
# Extended Additional U+1EA0..U+1EF9), the dong sign (U+20AB) and the
# typographic punctuation used in CJK / Vietnamese text (en / em dash,
# curly quotes, bullet, per mille).
#
# Greybeard covers all of it. Hack has the punctuation but no Latin
# Extended Additional, so the Hack-family Vietnamese letters come from
# Noto Sans Mono (SIL OFL 1.1), whose cell is 0.600 em against Hack's
# 0.602 em. (DejaVu Sans Mono, which Hack is derived from, lacks 46 of
# the letters with stacked marks, so it cannot be used.)
#
# Usage: gen_latinx.sh <lv_font_conv.js> <greybeard ttf dir/ or ""> \
#                      <Hack-Regular.ttf> <NotoSansMono-Regular.ttf> <out dir>
# lv_font_conv must be commit 1f44ab4 (see AGENTS.md, "Reproducing the
# committed fonts"); Greybeard v1.0.0, Hack v3.003, Noto Sans Mono
# v2.014 (hinted/ttf, notofonts/latin-greek-cyrillic release).
set -e
CONV="node $1"; GB="$2"; HACK="$3"; NOTO="$4"; OUT="$5"

VI=0x102-0x103,0x110-0x111,0x128-0x129,0x168-0x169,0x1A0-0x1A1,0x1AF-0x1B0,0x1EA0-0x1EF9
PUNCT=0x2013-0x2014,0x2018-0x201D,0x2022,0x2030,0x20AB

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
    n=greybeard_latinx_$1
    $CONV --font "${GB}Greybeard-$2px.ttf" $4 -r $VI,$PUNCT --size $3 --bpp 1 \
        --format lvgl --no-compress --lv-fallback greybeard_$1_lx_next \
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
    n=hack_latinx_$1
    $CONV --font "$HACK" -r $PUNCT --font "$NOTO" -r $VI --size $2 --bpp 1 \
        --format lvgl --no-compress --lv-fallback hack_$1_lx_next \
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
