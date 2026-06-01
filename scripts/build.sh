#!/bin/bash
set -e
echo "=== Build STM32 firmware (macOS/Linux) ==="
cd "$(dirname "$0")/.."

# Check arm-none-eabi-gcc
if ! command -v arm-none-eabi-gcc &>/dev/null; then
    echo "ERROR: arm-none-eabi-gcc not found."
    echo "macOS: brew install arm-none-eabi-gcc"
    echo "       or install STM32CubeIDE and add its toolchain to PATH"
    exit 1
fi

# cmake: prefer system, fall back to python module
if command -v cmake &>/dev/null; then
    CMAKE=cmake
elif python3 -m cmake --version &>/dev/null 2>&1; then
    CMAKE="python3 -m cmake"
else
    echo "ERROR: cmake not found. Install: brew install cmake  or  pip install cmake"
    exit 1
fi

echo "Configuring..."
$CMAKE -B build/Debug -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_TOOLCHAIN_FILE=cmake/stm32_toolchain.cmake \
    -G Ninja

echo "Building..."
$CMAKE --build build/Debug

echo ""
echo "Build complete: build/Debug/cthrobt.elf"
