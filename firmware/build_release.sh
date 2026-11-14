#!/usr/bin/env bash
# Build every web-flasher image of a release (every board x every
# language pack) in parallel. Run from anywhere with ESP-IDF exported.
#
#   ./build_release.sh                 # all flasher boards, all packs
#   ./build_release.sh xteink_x4_pro   # only the boards named
#
# Builds of one project tree share managed_components/,
# dependencies.lock and the component manager's cache (its git checkout
# of epdiy takes a lock, so two CMake runs at once fail), and the
# component manager deletes the components the board being configured
# does not use. So the CMake step (where the component manager runs)
# is done for one build at a time ("reconfigure"), and only the
# compile / link step ("build", which then finds nothing to
# reconfigure) runs in parallel. Even that may only overlap for boards
# with the same dependency set: the boards are split into groups by
# dependency set, and the groups run one after the other.
#
# PARALLEL builds run at once (default: half the CPUs), each with
# IDF_PY_BUILD_JOBS compile jobs (default: 4). Incremental builds spend
# much of their time in single-threaded steps (linking, image
# generation), so the two together deliberately oversubscribe the
# CPUs. Logs go to
# build/release-logs/<board>-<pack>.log.

set -u
cd "$(dirname "$0")"

PACKS="default vietnamese korean japanese chinese indic"
# One line per dependency set.
DEP_GROUPS=(
    "xteink_x4_pro xteink_x4_classic waveshare_rlcd42 waveshare_epaper_397 lilygo_t5_epd_s3_pro seeed_reterminal_e1001 seeed_reterminal_sticky m5stack_papermono"
    "microjournal_rev5"
    "m5stack_tab5"
)

NCPU=$(nproc)
PARALLEL=${PARALLEL:-$(( NCPU / 2 > 0 ? NCPU / 2 : 1 ))}
export IDF_PY_BUILD_JOBS=${IDF_PY_BUILD_JOBS:-4}
LOGDIR=build/release-logs
mkdir -p "$LOGDIR"
: > "$LOGDIR/failed"

wanted() {   # board
    [ ${#ONLY[@]} -eq 0 ] && return 0
    local w
    for w in "${ONLY[@]}"; do [ "$w" = "$1" ] && return 0; done
    return 1
}
ONLY=("$@")

configure_one() {   # board pack
    local log="$LOGDIR/$1-$2.log"
    if ! idf.py --preset "$1" --lang-pack "$2" reconfigure > "$log" 2>&1; then
        echo "FAILED  $1 $2 configure (see $log)"
        echo "$1 $2" >> "$LOGDIR/failed"
    fi
}

build_one() {   # board pack
    local log="$LOGDIR/$1-$2.log"
    grep -qx "$1 $2" "$LOGDIR/failed" && return   # configure failed
    if idf.py --preset "$1" --lang-pack "$2" build >> "$log" 2>&1; then
        echo "ok      $1 $2"
    else
        echo "FAILED  $1 $2 (see $log)"
        echo "$1 $2" >> "$LOGDIR/failed"
    fi
}
export -f build_one
export LOGDIR

for group in "${DEP_GROUPS[@]}"; do
    queue=()
    for b in $group; do
        if wanted "$b"; then
            for p in $PACKS; do queue+=("$b $p"); done
        fi
    done
    [ ${#queue[@]} -eq 0 ] && continue
    for item in "${queue[@]}"; do configure_one $item; done
    printf '%s\n' "${queue[@]}" |
        xargs -r -P "$PARALLEL" -L 1 bash -c 'build_one "$0" "$1"'
done

if [ -s "$LOGDIR/failed" ]; then
    echo "Failed builds:"; cat "$LOGDIR/failed"; exit 1
fi
echo "All builds succeeded."
