#!/bin/bash
# Host test of epub.cpp (ZIP reading, package / TOC parsing, XHTML to
# Markdown): builds it natively with stub ESP-IDF headers and miniz
# 1.14 (the ROM's inflater) and reads test books written by
# make_book.py. Needs gcc / g++, python3 and curl (the first run
# downloads miniz).
#
#   ./run.sh            build into ./out and run
#   OUT=/tmp/x ./run.sh build and run elsewhere
set -e
H=$(cd "$(dirname "$0")" && pwd)
E=$(cd "$H/../.." && pwd)
OUT=${OUT:-$H/out}
mkdir -p "$OUT"
if [ ! -f "$OUT/miniz.c" ]; then
    curl -sSfL -o "$OUT/miniz.c" \
        https://raw.githubusercontent.com/richgel999/miniz/v114/miniz.c
fi
python3 "$H/make_book.py" "$OUT"
F="-g -O0 -fsanitize=address,undefined"
gcc $F -w -c "$OUT/miniz.c" -o "$OUT/miniz.o"
g++ $F -Wall -Wextra -Werror -I"$H/stub" -I"$OUT" -I"$E/private_include" -std=gnu++20 \
    "$E/epub.cpp" "$H/test.cpp" "$OUT/miniz.o" -o "$OUT/epub_test"
cd "$OUT" && ./epub_test
