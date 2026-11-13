#!/bin/bash
# Host test of the Tamil input methods: builds the ime component
# natively (stub sdkconfig.h) and types key sequences into a fake
# document.
#
#   ./run.sh            build into ./out and run
#   OUT=/tmp/x ./run.sh build and run elsewhere
set -e
H=$(cd "$(dirname "$0")" && pwd)
C=$(cd "$H/../.." && pwd)
OUT=${OUT:-$H/out}
mkdir -p "$OUT"
gcc -Wall -Wextra -Werror -std=gnu11 -I"$H/stub" -I"$C/include" -I"$C" \
    -I"$C/../kb_layout/include" "$H/test_tamil.c" "$C"/ime.c "$C"/ime_tamil.c \
    "$C"/ime_tamil99.c "$C"/ime_tamil_tw.c "$C"/ime_taravinyas.c "$C"/ime_telex.c \
    "$C"/ime_hangul.c "$C"/ime_kana.c "$C"/ime_pinyin.c -o "$OUT/test_tamil"
"$OUT/test_tamil"
