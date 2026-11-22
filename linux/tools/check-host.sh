#!/bin/sh
# Report which of the Linux build's requirements this host lacks
# (`make check-host`; IO from the environment, default all backends).
# Package names are Debian / Ubuntu ones; see README.md for others.
# For a cross build (make check-host ARCH=arm64) the target's compilers
# and libraries (<pkg>:<arch>) are checked; the Makefile exports
# PKG_CONFIG_LIBDIR for the target.
IO=${IO:-wayland x11}
CROSS=${CROSS:-}
TRIPLET=${TRIPLET:-}
SFX=${CROSS:+:$CROSS}
missing=0
pkgs=""

need_cmd() {   # command, package
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "missing: $1 (package $2)"
        missing=1
        pkgs="$pkgs $2"
    fi
}

need_pc() {    # pkg-config module, package
    if ! pkg-config --exists "$1" 2>/dev/null; then
        echo "missing: pkg-config module $1 (package $2)"
        missing=1
        pkgs="$pkgs $2"
    fi
}

need_native_pc() {    # pkg-config module of this host (also when cross-building), package
    if ! env -u PKG_CONFIG_LIBDIR -u PKG_CONFIG_PATH pkg-config --exists "$1" 2>/dev/null; then
        echo "missing: pkg-config module $1 of this host (package $2)"
        missing=1
        pkgs="$pkgs $2"
    fi
}

# Every static archive the module's --static link line names.
need_static() {   # pkg-config module(s), package hint
    for lib in $(pkg-config --static --libs-only-l $1 2>/dev/null); do
        case $lib in
            -lpthread|-ldl|-lm|-lrt|-lc) continue ;;
            -l:*) name=${lib#-l:} ;;
            -l*) name=lib${lib#-l}.a ;;
            *) continue ;;
        esac
        found=
        for d in $(pkg-config --variable=libdir $1 2>/dev/null) \
                 $(${CC:-cc} -print-search-dirs 2>/dev/null | sed -n 's/^libraries: =//p' | tr ':' ' '); do
            [ -f "$d/$name" ] && { found=1; break; }
        done
        if [ -z "$found" ]; then
            echo "missing: static library $name (for $1; $2)"
            missing=1
        fi
    done
}

if [ -n "$CROSS" ]; then
    need_cmd "$TRIPLET-gcc" "gcc-$TRIPLET"
    need_cmd "$TRIPLET-g++" "g++-$TRIPLET"
    need_cmd "$TRIPLET-objdump" "binutils-$TRIPLET"
    CC=$TRIPLET-gcc
else
    need_cmd cc build-essential
    need_cmd c++ build-essential
fi
need_cmd make make
need_cmd git git
need_cmd curl curl
need_cmd python3 python3
need_cmd pkg-config pkgconf

need_pc libssl "libssl-dev$SFX"
# What OpenSSL's static link needs differs: nothing more with OpenSSL
# 3.0 (Ubuntu 22.04, Debian 12), these three on newer releases.
need_static "libssl libcrypto" "libssl-dev$SFX (and, where pkg-config --static --libs libssl libcrypto lists them, libjitterentropy3-dev$SFX zlib1g-dev$SFX libzstd-dev$SFX)"

case " $IO " in *" wayland "*)
    need_cmd meson meson
    need_cmd ninja ninja-build
    # wayland.mk builds wayland-scanner for this host
    need_native_pc expat libexpat1-dev
    need_pc libffi "libffi-dev$SFX"
    need_static libffi "libffi-dev$SFX"
    ;;
esac
case " $IO " in *" x11 "*)
    need_pc xcb "libxcb1-dev$SFX"
    need_pc xcb-xkb "libxcb-xkb-dev$SFX"
    need_static "xcb xcb-xkb" "libxcb1-dev$SFX, libxcb-xkb-dev$SFX, libxau-dev$SFX, libxdmcp-dev$SFX"
    ;;
esac

if [ $missing = 0 ]; then
    echo "All build requirements for IO=\"$IO\"${CROSS:+ (ARCH=$CROSS)} are present."
else
    [ -n "$pkgs" ] && echo "Install with: sudo apt install$(echo "$pkgs" | tr ' ' '\n' | sort -u | tr '\n' ' ' | sed 's/ $//')"
    exit 1
fi
