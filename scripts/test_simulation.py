#!/usr/bin/env python3
"""
End-to-end simulation test for cthrobt_stm.

Starts Renode via bridge.py, sends test gamepad packets over TCP 7777,
then checks received_signals.txt for expected output.

Usage:
    python3 scripts/test_simulation.py
"""
import os
import sys
import json
import socket
import subprocess
import time
import re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LOG  = os.path.join(ROOT, "received_signals.txt")

# ── Test packets ─────────────────────────────────────────────────────────────
# Each entry: (description, json_payload, expected_substrings_in_output)
TEST_PACKETS = [
    # ── Gripper ───────────────────────────────────────────────────────────────
    (
        "A pressed → gripper CLOSE",
        {"buttons": {"A": True}, "axes": {}, "hat": [0, 0]},
        ["[GRIPPER] CLOSE"],
    ),
    (
        "A held (no repeat)",
        {"buttons": {"A": True}, "axes": {}, "hat": [0, 0]},
        ["A"],          # only prints status, no second CLOSE
    ),
    (
        "A released",
        {"buttons": {}, "axes": {}, "hat": [0, 0]},
        ["(idle)"],
    ),
    (
        "B pressed → gripper OPEN",
        {"buttons": {"B": True}, "axes": {}, "hat": [0, 0]},
        ["[GRIPPER] OPEN"],
    ),
    (
        "B released",
        {"buttons": {}, "axes": {}, "hat": [0, 0]},
        ["(idle)"],
    ),
    # ── Other inputs still work ───────────────────────────────────────────────
    (
        "left stick pushed right+up",
        {"buttons": {}, "axes": {"LX": 0.85, "LY": -0.72}, "hat": [0, 0]},
        ["LX=+0.850", "LY=-0.720"],
    ),
    (
        "hat UP",
        {"buttons": {}, "axes": {}, "hat": [0, 1]},
        ["HAT:UP"],
    ),
]

BRIDGE_PORT = 7777
BRIDGE_STARTUP_TIMEOUT = 12   # seconds to wait for Renode to boot
SEND_INTERVAL          = 0.15  # seconds between packets


def wait_for_port(host, port, timeout):
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            s = socket.create_connection((host, port), timeout=1)
            s.close()
            return True
        except OSError:
            time.sleep(0.5)
    return False


def send_packets(packets):
    s = socket.create_connection(("127.0.0.1", BRIDGE_PORT), timeout=5)
    for _, payload, _ in packets:
        line = json.dumps(payload, separators=(",", ":")) + "\n"
        s.sendall(line.encode())
        time.sleep(SEND_INTERVAL)
    s.close()


BOOT_PATTERNS = ["Task started", "Simulation mode", "Controller connected", "LwIP stack ready",
                 "Listening on port", "GRIPPER] init"]

def check_results(packets, log_path, wait=8):
    """
    For each packet, verify that ALL expected strings appear in the UART3
    output window produced by that packet.

    Each packet produces 1–2 UART3 lines:
      - optional [GRIPPER] line (when edge fires)
      - one [CTRL] line
    We walk the full output line-by-line and for each [CTRL] line we collect
    any immediately preceding [GRIPPER] line as belonging to the same packet.
    """
    deadline = time.time() + wait
    while time.time() < deadline:
        time.sleep(1)
        if not os.path.exists(log_path):
            continue
        content = open(log_path, encoding="utf-8").read()
        uart_lines = [l for l in content.splitlines()
                      if "[UART3]" in l and not any(bp in l for bp in BOOT_PATTERNS)]
        ctrl_count = sum(1 for l in uart_lines if "[CTRL]" in l)
        if ctrl_count >= len(packets):
            break

    content = open(log_path, encoding="utf-8").read() if os.path.exists(log_path) else ""
    uart_lines = [l for l in content.splitlines()
                  if "[UART3]" in l and not any(bp in l for bp in BOOT_PATTERNS)]

    # Group lines into per-packet windows: collect pending non-CTRL lines
    # until we see a [CTRL] line, then emit that group as one packet window.
    groups = []
    pending = []
    for line in uart_lines:
        if "[CTRL]" in line:
            pending.append(line)
            groups.append(" | ".join(pending))
            pending = []
        else:
            pending.append(line)

    print(f"\n{'─'*60}")
    print(f"  Packet windows received: {len(groups)}")
    print(f"{'─'*60}")

    passed = 0
    failed = 0
    for i, (desc, _, expected) in enumerate(packets):
        window = groups[i] if i < len(groups) else ""
        ok = all(e in window for e in expected)
        status = "PASS" if ok else "FAIL"
        if ok:
            passed += 1
        else:
            failed += 1
        print(f"  [{status}] {desc}")
        print(f"         expected : {expected}")
        print(f"         got      : {window.strip()}")

    print(f"{'─'*60}")
    print(f"  Result: {passed}/{len(packets)} passed")
    print(f"{'─'*60}\n")
    return failed == 0


def main():
    elf = os.path.join(ROOT, "build", "Debug", "cthrobt.elf")
    if not os.path.exists(elf):
        print(f"ERROR: firmware not built — run scripts/build.sh first")
        sys.exit(1)

    print("[TEST] Starting bridge.py + Renode...")
    bridge = subprocess.Popen(
        [sys.executable, os.path.join(ROOT, "renode", "bridge.py"), "--log", LOG],
        cwd=ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )

    print(f"[TEST] Waiting for Renode to boot (up to {BRIDGE_STARTUP_TIMEOUT}s)...")
    if not wait_for_port("127.0.0.1", BRIDGE_PORT, BRIDGE_STARTUP_TIMEOUT):
        print("[TEST] ERROR: bridge port 7777 never opened")
        bridge.terminate()
        sys.exit(1)

    print(f"[TEST] Bridge ready — sending {len(TEST_PACKETS)} test packets...")
    try:
        send_packets(TEST_PACKETS)
    except Exception as e:
        print(f"[TEST] ERROR sending packets: {e}")
        bridge.terminate()
        sys.exit(1)

    print("[TEST] Packets sent — waiting for firmware to process...")
    time.sleep(8)

    bridge.terminate()
    bridge.wait()

    ok = check_results(TEST_PACKETS, LOG)
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
