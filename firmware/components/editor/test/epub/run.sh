#!/bin/bash
# Host test of epub.cpp (ZIP reading, package / TOC parsing, XHTML to
# Markdown): builds it natively with stub ESP-IDF headers (zlib stands
# in for the ROM's miniz) and reads test books written by make_book.py.
# Needs g++, python3 and zlib.
#
#   ./run.sh            build into ./out and run
#   OUT=/tmp/x ./run.sh build and run elsewhere
set -e
H=$(cd "$(dirname "$0")" && pwd)
E=$(cd "$H/../.." && pwd)
OUT=${OUT:-$H/out}
mkdir -p "$OUT"
python3 "$H/make_book.py" "$OUT"
F="-g -O0 -Wall -Wextra -Werror -fsanitize=address,undefined"
g++ $F -I"$H/stub" -I"$E/private_include" -std=gnu++20 \
    "$E/epub.cpp" "$H/test.cpp" -lz -o "$OUT/epub_test"
cd "$OUT" && ./epub_test
