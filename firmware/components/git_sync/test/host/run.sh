#!/bin/bash
# Host test of the Git sync client: builds git_sync.cpp and the git_*.c
# sources natively (with stub ESP-IDF headers) and runs whole syncs
# against a real bare repository through `git http-backend`.
#
# Needs gcc / g++, libcrypto (OpenSSL, for SHA-1), git and curl (the
# first run downloads miniz 1.14, the zlib of the ESP32 ROM).
#
#   ./run.sh            build into ./out and run
#   OUT=/tmp/x ./run.sh build and run elsewhere
set -e
H=$(cd "$(dirname "$0")" && pwd)
G=$(cd "$H/../.." && pwd)
OUT=${OUT:-$H/out}
mkdir -p "$OUT"
if [ ! -f "$OUT/miniz.c" ]; then
    curl -sSfL -o "$OUT/miniz.c" \
        https://raw.githubusercontent.com/richgel999/miniz/v114/miniz.c
fi
I="-I$H/stub -I$OUT -I$G -I$G/include"
F="-g -O0 -fsanitize=address,undefined -fno-sanitize=alignment -Wno-deprecated-declarations"
gcc $F $I -w -c "$OUT/miniz.c" -o "$OUT/miniz.o"
for f in git_util git_odb git_pack git_merge git_net draftbox; do
    gcc $F $I -std=gnu11 -D_GNU_SOURCE -c "$G/$f.c" -o "$OUT/$f.o"
done
g++ $F $I -std=gnu++20 -c "$H/support.cpp" -o "$OUT/support.o"
g++ $F $I -std=gnu++20 -c "$H/test.cpp" -o "$OUT/test.o"
g++ $F "$OUT"/*.o -lcrypto -o "$OUT/git_sync_test"
cd "$OUT" && ./git_sync_test
