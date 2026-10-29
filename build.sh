#!/bin/bash
set -e

# Build for non-jailbreak iOS arm64 via Sideloadly
# No Dobby required — uses vm_write for memory patching (safe on non-jb)

SDK=$(xcrun --sdk iphoneos --show-sdk-path)
CLANG=$(xcrun --sdk iphoneos --find clang 2>/dev/null || echo "clang")
ARCH=arm64

echo "Building CounterFlameCheat.dylib for iOS arm64 (non-jailbreak)..."

# Compile ImGui + our code
$CLANG -arch $ARCH -isysroot "$SDK" \
    -I. -I./imgui -I./imgui/backends \
    -framework Metal -framework MetalKit -framework QuartzCore -framework UIKit -framework Foundation \
    -std=c++20 -O2 -fvisibility=hidden -shared \
    cheat.cpp menu.mm main.cpp \
    imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp \
    imgui/backends/imgui_impl_metal.mm \
    -lc++ \
    -o CounterFlameCheat.dylib

echo "Build complete: CounterFlameCheat.dylib"
ls -la CounterFlameCheat.dylib