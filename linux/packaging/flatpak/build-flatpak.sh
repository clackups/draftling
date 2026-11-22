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
# Needs flatpak-builder, the flathub remote and, for a foreign
# architecture, qemu-user-binfmt -- see README.md.
set -eu

VERSION=$1
ARCHES=$2
DIST=$3

[ -n "$VERSION" ] || { echo "build-flatpak.sh: no version" >&2; exit 1; }
command -v flatpak-builder >/dev/null || { echo "build-flatpak.sh: flatpak-builder is missing" >&2; exit 1; }

APP=art.draftling.Draftling
MANIFEST=packaging/flatpak/$APP.yml
REPO=build/flatpak-repo
STATE=build/flatpak-builder
mkdir -p "$DIST"

flatpak remote-add --user --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo

for arch in $ARCHES; do
    echo "== Flatpak $VERSION $arch"
    flatpak-builder --arch="$arch" --user --install-deps-from=flathub \
        --default-branch=stable --force-clean --state-dir="$STATE" \
        --repo="$REPO" "build/flatpak-build-$arch" "$MANIFEST"
    flatpak build-bundle --arch="$arch" \
        --runtime-repo=https://dl.flathub.org/repo/flathub.flatpakrepo \
        "$REPO" "$DIST/draftling_${VERSION}_$arch.flatpak" "$APP" stable
    echo "   $DIST/draftling_${VERSION}_$arch.flatpak"
done
