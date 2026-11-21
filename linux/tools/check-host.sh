#!/bin/sh
# Report which of the Linux build's requirements this host lacks
# (`make check-host`; IO from the environment, default all backends).
# Package names are Debian / Ubuntu ones; see README.md for others.
IO=${IO:-wayland x11 fbdev}
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

need_cmd cc build-essential
need_cmd c++ build-essential
need_cmd make make
need_cmd git git
need_cmd curl curl
need_cmd python3 python3
need_cmd pkg-config pkgconf

need_pc libssl libssl-dev
need_static "libssl libcrypto" "libssl-dev, libjitterentropy3-dev, zlib1g-dev, libzstd-dev"

case " $IO " in *" wayland "*)
    need_cmd meson meson
    need_cmd ninja ninja-build
    need_cmd wayland-scanner libwayland-bin
    need_pc libffi libffi-dev
    need_static libffi libffi-dev
    ;;
esac
case " $IO " in *" x11 "*)
    need_pc xcb libxcb1-dev
    need_pc xcb-xkb libxcb-xkb-dev
    need_static "xcb xcb-xkb" "libxcb1-dev, libxcb-xkb-dev, libxau-dev, libxdmcp-dev"
    ;;
esac
case " $IO " in *" fbdev "*)
    [ -f /usr/include/linux/fb.h ] || { echo "missing: Linux headers (package linux-libc-dev)"; missing=1; }
    ;;
esac

if [ $missing = 0 ]; then
    echo "All build requirements for IO=\"$IO\" are present."
else
    [ -n "$pkgs" ] && echo "Install with: sudo apt install$(echo "$pkgs" | tr ' ' '\n' | sort -u | tr '\n' ' ' | sed 's/ $//')"
    exit 1
fi
