#!/bin/bash
# Host test of projects.cpp (project folders, documents outside
# projects and their ignore list): builds it natively with stub ESP-IDF
# headers, the SD card being a temporary folder (support in test.cpp).
#
#   ./run.sh            build into ./out and run
#   OUT=/tmp/x ./run.sh build and run elsewhere
set -e
H=$(cd "$(dirname "$0")" && pwd)
E=$(cd "$H/../.." && pwd)
OUT=${OUT:-$H/out}
mkdir -p "$OUT"
I="-I$H/stub -I$E/include -I$E/private_include -I$E/../sd_card/include"
F="-g -O0 -Wall -Wextra -Werror -fsanitize=address,undefined"
g++ $F $I -std=gnu++20 "$E/projects.cpp" "$H/test.cpp" -o "$OUT/projects_test"
"$OUT/projects_test"
