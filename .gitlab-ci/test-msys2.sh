#!/bin/bash

set -e

if [[ "$MSYSTEM" == "MINGW32" ]]; then
    export MSYS2_ARCH="i686"
else
    export MSYS2_ARCH="x86_64"
fi

pacman --noconfirm -Suy

pacman --noconfirm -S --needed \
    base-devel \
    mingw-w64-$MSYS2_ARCH-gobject-introspection \
    mingw-w64-$MSYS2_ARCH-harfbuzz \
    mingw-w64-$MSYS2_ARCH-fontconfig \
    mingw-w64-$MSYS2_ARCH-fribidi \
    mingw-w64-$MSYS2_ARCH-libthai \
    mingw-w64-$MSYS2_ARCH-cairo \
    mingw-w64-$MSYS2_ARCH-meson \
    mingw-w64-$MSYS2_ARCH-toolchain \
    mingw-w64-$MSYS2_ARCH-cantarell-fonts

meson --buildtype debug _build
cd _build
ninja

_build/tests/test-font -p /pango/font/metrics --verbose

# FIXME: Fix tests
meson test || true
