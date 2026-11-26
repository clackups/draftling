#!/bin/sh
# Build the Flatpak bundles of Draftling for Linux (run by
# `make flatpak`, from linux/):
#
#   packaging/flatpak/build-flatpak.sh <version> "<arches>" <output dir>
#
# One flatpak-builder run per architecture (Flatpak names: x86_64,
# aarch64) into one local repository, build/flatpak-repo, then one
# single-file bundle per architecture:
# <output dir>/draftling_<version>_<arch>.flatpak. A foreign
# architecture runs its build under qemu-user (binfmt_misc). The SDK
# and runtime come from Flathub (installed per user when missing).
#
# flatpak-builder: the host's when it is 1.4 or newer, otherwise
# Flathub's org.flatpak.Builder (installed per user when missing):
# older ones (Ubuntu 22.04 has 1.2) run appstream-compose, which the
# 25.08 SDK no longer has. The releases are built on Ubuntu 22.04 (for
# the Debian packages' glibc), in an LXC container, which also has no
# /dev/fuse (-> --disable-rofiles-fuse) and no session bus, which
# org.flatpak.Builder needs to run flatpak on the host
# (-> dbus-run-session).
#
# Needs flatpak, the flathub remote and, for a foreign architecture, a
# binfmt_misc entry for its qemu with the F flag (qemu-user-static; in
# a container, installed on the container's host) -- see README.md.
set -eu

VERSION=$1
ARCHES=$2
DIST=$3

[ -n "$VERSION" ] || { echo "build-flatpak.sh: no version" >&2; exit 1; }
command -v flatpak >/dev/null || { echo "build-flatpak.sh: flatpak is missing" >&2; exit 1; }

APP=art.draftling.Draftling
HERE=$(pwd)
TOP=$(cd .. && pwd)
MANIFEST=$HERE/packaging/flatpak/$APP.yml
REPO=$HERE/build/flatpak-repo
STATE=$HERE/build/flatpak-builder
mkdir -p "$DIST"

flatpak remote-add --user --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo

# Which flatpak-builder to run.
builder_ok() {
    v=$(flatpak-builder --version 2>/dev/null | sed -n 's/^flatpak-builder[ -]*\([0-9][0-9.]*\).*/\1/p')
    [ -n "$v" ] || return 1
    major=${v%%.*}; rest=${v#*.}; minor=${rest%%.*}
    [ "$major" -gt 1 ] || { [ "$major" -eq 1 ] && [ "$minor" -ge 4 ]; }
}
if command -v flatpak-builder >/dev/null && builder_ok; then
    set -- flatpak-builder
else
    echo "== host flatpak-builder missing or older than 1.4: using Flathub's org.flatpak.Builder"
    flatpak info --user org.flatpak.Builder >/dev/null 2>&1 ||
        flatpak install --user -y --noninteractive flathub org.flatpak.Builder
    set -- flatpak run --filesystem=host --filesystem="$TOP" --cwd="$HERE" org.flatpak.Builder
    # It runs flatpak on the host through the session bus.
    if [ -z "${DBUS_SESSION_BUS_ADDRESS:-}" ]; then
        command -v dbus-run-session >/dev/null ||
            { echo "build-flatpak.sh: no D-Bus session bus and no dbus-run-session" >&2; exit 1; }
        set -- dbus-run-session -- "$@"
    fi
fi
[ -c /dev/fuse ] || set -- "$@" --disable-rofiles-fuse

host_arch=$(uname -m)
for arch in $ARCHES; do
    if [ "$arch" != "$host_arch" ]; then
        qemu=$(case $arch in aarch64) echo qemu-aarch64 ;; x86_64) echo qemu-x86_64 ;; *) echo "qemu-$arch" ;; esac)
        entry=/proc/sys/fs/binfmt_misc/$qemu
        if ! grep -q '^enabled' "$entry" 2>/dev/null || ! grep -q '^flags:.*F' "$entry"; then
            echo "build-flatpak.sh: $arch needs an enabled $qemu binfmt_misc entry with the F flag" >&2
            echo "  (install qemu-user-static; in a container, on the container's host)" >&2
            exit 1
        fi
    fi
    echo "== Flatpak $VERSION $arch"
    "$@" --arch="$arch" --user --install-deps-from=flathub \
        --default-branch=stable --force-clean --state-dir="$STATE" \
        --repo="$REPO" "$HERE/build/flatpak-build-$arch" "$MANIFEST"
    flatpak build-bundle --arch="$arch" \
        --runtime-repo=https://dl.flathub.org/repo/flathub.flatpakrepo \
        "$REPO" "$DIST/draftling_${VERSION}_$arch.flatpak" "$APP" stable
    echo "   $DIST/draftling_${VERSION}_$arch.flatpak"
done
