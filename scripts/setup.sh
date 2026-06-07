#!/bin/bash
# Setup script for cthrobt_stm — installs all required tools.
# Usage: bash scripts/setup.sh
set -e

OS="$(uname -s)"
echo "=== cthrobt_stm Setup ($OS) ==="
echo

# ── 1. arm-none-eabi-gcc ──────────────────────────────────────────────────────
if command -v arm-none-eabi-gcc &>/dev/null; then
    echo "[OK] arm-none-eabi-gcc: $(arm-none-eabi-gcc --version | head -1)"
else
    echo "[INSTALL] arm-none-eabi-gcc..."
    if [ "$OS" = "Darwin" ]; then
        brew install --cask gcc-arm-embedded
    else
        sudo apt-get update -q
        sudo apt-get install -y gcc-arm-none-eabi binutils-arm-none-eabi
    fi
    echo "[OK] arm-none-eabi-gcc installed."
fi

# ── 2. cmake ──────────────────────────────────────────────────────────────────
if command -v cmake &>/dev/null; then
    echo "[OK] cmake: $(cmake --version | head -1)"
elif python3 -m cmake --version &>/dev/null 2>&1; then
    echo "[OK] cmake (python module): $(python3 -m cmake --version | head -1)"
else
    echo "[INSTALL] cmake..."
    if [ "$OS" = "Darwin" ]; then
        brew install cmake
    else
        sudo apt-get install -y cmake
    fi
    echo "[OK] cmake installed."
fi

# ── 3. ninja ──────────────────────────────────────────────────────────────────
if command -v ninja &>/dev/null; then
    echo "[OK] ninja: $(ninja --version)"
else
    echo "[INSTALL] ninja..."
    if [ "$OS" = "Darwin" ]; then
        brew install ninja
    else
        sudo apt-get install -y ninja-build
    fi
    echo "[OK] ninja installed."
fi

# ── 4. python3 ────────────────────────────────────────────────────────────────
if command -v python3 &>/dev/null; then
    echo "[OK] python3: $(python3 --version)"
else
    echo "[INSTALL] python3..."
    if [ "$OS" = "Darwin" ]; then
        brew install python3
    else
        sudo apt-get install -y python3
    fi
fi

# ── 5. renode ─────────────────────────────────────────────────────────────────
if command -v renode &>/dev/null; then
    echo "[OK] renode found."
else
    echo "[INSTALL] renode..."
    if [ "$OS" = "Darwin" ]; then
        brew install --cask renode
    else
        # Renode .deb from official releases
        RENODE_DEB="renode_1.16.0+20250324gitf1b6a08fa_amd64.deb"
        RENODE_URL="https://github.com/renode/renode/releases/download/v1.16.0/${RENODE_DEB}"
        TMP="/tmp/${RENODE_DEB}"
        echo "Downloading Renode from GitHub releases..."
        curl -L "$RENODE_URL" -o "$TMP"
        sudo dpkg -i "$TMP"
        rm -f "$TMP"
    fi
    echo "[OK] renode installed."
fi

# ── 6. Vendor code (Drivers/ and Middlewares/) ────────────────────────────────
echo
if [ -d "$(dirname "$0")/../Drivers" ] && [ -d "$(dirname "$0")/../Middlewares" ]; then
    echo "[OK] Drivers/ and Middlewares/ already present."
else
    echo "┌─────────────────────────────────────────────────────────────────┐"
    echo "│  ACTION REQUIRED: Regenerate vendor code                        │"
    echo "│                                                                 │"
    echo "│  Drivers/ and Middlewares/ are not tracked in git.             │"
    echo "│  Regenerate them from the CubeMX project file:                 │"
    echo "│                                                                 │"
    echo "│    1. Open STM32CubeMX                                          │"
    echo "│    2. File → Load Project → select cthrobt.ioc                 │"
    echo "│    3. Project → Generate Code                                   │"
    echo "│                                                                 │"
    echo "│  After that, run: bash scripts/build.sh                        │"
    echo "└─────────────────────────────────────────────────────────────────┘"
fi

echo
echo "=== Setup complete. Next: bash scripts/build.sh ==="
