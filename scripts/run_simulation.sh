#!/bin/bash
echo "=== Run Renode Simulation (macOS/Linux) ==="
echo "Starts Renode with cthrobt.elf and waits for controller.py on port 7777."
echo "Run scripts/run_mock.sh (in cthrobt) in a separate terminal to send data."
echo

cd "$(dirname "$0")/.."

if [ ! -f "build/Debug/cthrobt.elf" ]; then
    echo "ERROR: Firmware not built. Run scripts/build.sh first."
    exit 1
fi

python3 renode/bridge.py "$@"
