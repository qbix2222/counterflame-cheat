#!/bin/bash
SDK=$(xcrun --sdk iphoneos --show-sdk-path)
CLANG=$(xcrun --sdk iphoneos --find clang 2>/dev/null || echo "clang")
ARCH=arm64

$CLANG -arch $ARCH -isysroot "$SDK" \
    -I. -I./imgui -I./imgui/backends \
    -framework Metal -framework QuartzCore -framework UIKit -framework Foundation \
    -std=c++20 -O2 -fvisibility=hidden -shared \
    cheat.cpp menu.cpp main.cpp \
    imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp \
    imgui/backends/imgui_impl_metal.cpp \
    -o CounterFlameCheat.dylib

echo "Build complete: CounterFlameCheat.dylib"
