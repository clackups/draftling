#!/bin/sh
# Regenerates the Latin Extended-A subset fonts of the default language
# pack: lowres_latina_NN.c and hires_latina_NN.c.
#
# Coverage: Latin Extended-A (U+0100..U+017F: Polish, Czech, Slovak,
# Hungarian, Croatian, Slovenian, Turkish, Crimean Tatar, Baltic, ...), the Romanian
# s / t with comma below (U+0218..U+021B), the spacing accents typed by
# the dead keys (caron U+02C7, breve U+02D8, dot above U+02D9, ring
# U+02DA, ogonek U+02DB, double acute U+02DD), the low / high double
# quotes (U+201D, U+201E) of the Polish and Romanian layouts and the
# en / em dashes and right single quote (U+2013, U+2014, U+2019) of the
# Crimean Tatar layout, and the characters of the French BEPO layout's
# AltGr layers and dead keys: schwa (U+018F, U+0259), Greek (U+0391..
# U+03C9, dead Greek), the consonants with a dot below (U+1E04..U+1E93),
# capital sharp s (U+1E9E), the opening curly quotes (U+2018, U+201C),
# daggers (U+2020, U+2021), narrow no-break space (U+202F), per mille
# (U+2030), primes (U+2032, U+2033), trade mark (U+2122), minus
# (U+2212), not equal (U+2260) and less / greater or equal (U+2264,
# U+2265); and for the French AZERTY (AFNOR) layout: ezh (U+01B7,
# U+0292), the non-breaking hyphen (U+2011), the low single quote
# (U+201A), the single guillemets (U+2039, U+203A), square root
# (U+221A), infinity (U+221E) and asymptotically equal (U+2243).
#
# Greybeard covers all of it. Hack lacks the schwa, ezh, the capital
# sharp s and the dot-below consonants; those come from Noto Sans
# Mono, as in gen_latinx.sh.
#
# Usage: gen_latina.sh <lv_font_conv.js> <greybeard ttf dir/> \
#                      <Hack-Regular.ttf> <NotoSansMono-Regular.ttf> <out dir>
# lv_font_conv must be commit 1f44ab4 (see AGENTS.md, "Reproducing the
# committed fonts"); Greybeard v1.0.0, Hack v3.003, Noto Sans Mono
# v2.014 (hinted/ttf).
set -e
CONV="node $1"; GB="$2"; HACK="$3"; NOTO="$4"; OUT="$5"

EXTA=0x100-0x17F,0x218-0x21B,0x2C7,0x2D8-0x2DB,0x2DD
GREEK=0x391-0x3A1,0x3A3-0x3A9,0x3B1-0x3C9
PUNCT=0x2011,0x2013-0x2014,0x2018-0x201A,0x201C-0x201E,0x2020-0x2021,0x202F-0x2030,0x2032-0x2033,0x2039-0x203A,0x2122,0x2212,0x221A,0x221E,0x2243,0x2260,0x2264-0x2265
NOHACK=0x18F,0x1B7,0x259,0x292,0x1E04-0x1E05,0x1E0C-0x1E0D,0x1E24-0x1E25,0x1E32-0x1E33,0x1E36-0x1E37,0x1E42-0x1E43,0x1E46-0x1E47,0x1E5A-0x1E5B,0x1E62-0x1E63,0x1E6C-0x1E6D,0x1E7E-0x1E7F,0x1E88-0x1E89,0x1E92-0x1E93,0x1E9E
RANGE=$EXTA,$NOHACK,$GREEK,$PUNCT

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
    $CONV --font "$HACK" -r $EXTA,$GREEK,$PUNCT --font "$NOTO" -r $NOHACK --size $2 --bpp 1 \
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
