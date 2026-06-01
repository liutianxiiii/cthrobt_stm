#!/bin/bash
echo "=== Clean build directory (macOS/Linux) ==="
cd "$(dirname "$0")/.."
if [ -d "build" ]; then
    rm -rf build
    echo "Build directory removed."
else
    echo "Nothing to clean."
fi
