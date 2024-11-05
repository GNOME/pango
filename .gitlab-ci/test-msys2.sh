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

meson setup --buildtype debug _build
meson compile -C _build
meson test -C _build -t "$MESON_TEST_TIMEOUT_MULTIPLIER" --print-errorlogs --suite pango
