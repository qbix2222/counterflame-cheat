#!/bin/bash
set -e

SDK=$(xcrun --sdk iphoneos --show-sdk-path)
CLANG=$(xcrun --sdk iphoneos --find clang 2>/dev/null || echo "clang")
ARCH=arm64

DOBBY_LIB_DIR="./Dobby/build_for_ios_arm64"
DOBBY_LIB_PATH="$DOBBY_LIB_DIR/libdobby.a"
DOBBY_INCLUDE_PATH="./Dobby/include"

if [ ! -f "$DOBBY_LIB_PATH" ]; then
    echo "ERROR: Dobby static library not found at $DOBBY_LIB_PATH"
    exit 1
fi

$CLANG -arch $ARCH -isysroot "$SDK" \
    -I. -I./imgui -I./imgui/backends -I"$DOBBY_INCLUDE_PATH" \
    -framework Metal -framework QuartzCore -framework UIKit -framework Foundation \
    -std=c++20 -O2 -fvisibility=hidden -shared \
    cheat.cpp menu.mm main.cpp cache_shim.c \
    imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp \
    imgui/backends/imgui_impl_metal.mm \
    -L"$DOBBY_LIB_DIR" -ldobby \
    -lc++ \
    -o CounterFlameCheat.dylib

echo "Build complete: CounterFlameCheat.dylib"
