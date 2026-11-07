#!/bin/sh
# Builds the co-processor firmware, then the P4 upgrade tool that embeds
# it, for one board preset (default: m5stack_tab5; see cp/CMakePresets.json and
# p4/CMakePresets.json). Run with ESP-IDF v6.1+ exported
# (. $IDF_PATH/export.sh).
set -e
board=${1:-m5stack_tab5}
here=$(cd "$(dirname "$0")" && pwd)
(cd "$here/cp" && idf.py --preset "$board" build)
(cd "$here/p4" && idf.py --preset "$board" build)
echo
echo "Flash the upgrade tool onto the P4 (on the Tab5: its USB-C port):"
echo "  cd $here/p4 && idf.py --preset $board -p /dev/ttyACM0 flash monitor"
