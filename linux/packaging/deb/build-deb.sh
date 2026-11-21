#!/bin/sh
# Build the Debian package of Draftling for Linux from programs that
# `make` has built (run by `make deb`, from linux/):
#
#   packaging/deb/build-deb.sh <version> "<packs>" <output dir>
#
# The tree is put together by `make install` (DESTDIR, PREFIX=/usr), so
# packaging for another system (RPM, Arch, a BSD port) can reuse that
# step and only replace this script. Needs dpkg-deb, objdump and strip
# (binutils; for a cross build the target's, passed as OBJDUMP / STRIP)
# and dpkg (for the architecture name).
#
# Environment (set by the Makefile): ARCH (Debian architecture of the
# package, default this host's), OUT (the build output folder, default
# build), STRIP, OBJDUMP, MAIN_PACK.
#
# The programs link everything but the C library statically, so the
# only dependency is libc6, at the newest glibc symbol version they
# use.
set -eu
umask 022

VERSION=$1
PACKS=$2
DIST=$3

[ -n "$VERSION" ] || { echo "build-deb.sh: no version" >&2; exit 1; }
ARCH=${ARCH:-$(dpkg --print-architecture)}
OUT=${OUT:-build}
STRIP=${STRIP:-strip}
OBJDUMP=${OBJDUMP:-objdump}
for tool in dpkg-deb dpkg "$OBJDUMP" "$STRIP"; do
    command -v "$tool" >/dev/null || { echo "build-deb.sh: $tool is missing" >&2; exit 1; }
done

STAGE=$OUT/deb-root
rm -rf "$STAGE"
make --no-print-directory install DESTDIR="$STAGE" PREFIX=/usr PACKS="$PACKS" \
    MAIN_PACK="${MAIN_PACK:-all}" ARCH="$ARCH"
"$STRIP" --strip-unneeded "$STAGE"/usr/bin/*   # built with -g

DOC=$STAGE/usr/share/doc/draftling
install -m 0644 packaging/deb/copyright "$DOC/copyright"
rm -f "$DOC/LICENSE"   # copyright carries it
printf 'draftling (%s) unstable; urgency=medium\n\n  * Release %s, see https://github.com/clackups/draftling/blob/main/CHANGELOG.md\n\n -- Draftling <clackups@gmail.com>  %s\n' \
    "$VERSION" "$VERSION" "$(date -R)" | gzip -9n > "$DOC/changelog.Debian.gz"

# libc6 (>= newest GLIBC_x.y any program needs)
GLIBC=$(for f in "$STAGE"/usr/bin/*; do "$OBJDUMP" -T "$f"; done |
    sed -n 's/.*GLIBC_\([0-9][0-9.]*\).*/\1/p' | sort -t. -k1,1n -k2,2n -k3,3n | tail -1)
DEPENDS="libc6 (>= ${GLIBC:-2.34})"

# "all" is every keyboard layout, input method and font in one program.
if [ "$PACKS" = all ]; then
    PACK_TEXT="Every language of Draftling: all keyboard layouts, input methods and fonts."
else
    PACK_TEXT="Language packs in this package: $PACKS (draftling-<pack>)."
fi

SIZE_KB=$(du -sk "$STAGE/usr" | cut -f1)
mkdir -p "$STAGE/DEBIAN"
cat > "$STAGE/DEBIAN/control" <<CTRL
Package: draftling
Version: $VERSION
Architecture: $ARCH
Maintainer: Draftling <clackups@gmail.com>
Installed-Size: $SIZE_KB
Depends: $DEPENDS
Recommends: network-manager
Section: editors
Priority: optional
Homepage: https://draftling.art/
Description: distraction-free Markdown and Fountain editor with Git sync
 The editor of the Draftling writing devices as a Linux program: a
 full-screen, keyboard-driven editor for Markdown, Fountain screenplays
 and plain text, organised in projects, with a built-in Git client that
 synchronises each project with a Git repository. Runs in a Wayland or
 X11 window.
 .
 $PACK_TEXT
CTRL
( cd "$STAGE" && find usr -type f -exec md5sum {} + | sort -k2 ) > "$STAGE/DEBIAN/md5sums"

mkdir -p "$DIST"
OUT="$DIST/draftling_${VERSION}_${ARCH}.deb"
dpkg-deb --root-owner-group -Zxz --build "$STAGE" "$OUT"
echo "Package: $OUT"
