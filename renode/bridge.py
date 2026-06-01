#!/usr/bin/env python3
"""
bridge.py — bridges controller.py TCP → Renode UART3 injection

Usage:
    cd /Users/liutianxi/ctrlall/cthrobt_stm
    python3 renode/bridge.py [--log /path/to/output.txt]

How it works:
  1. Starts Renode as a subprocess, feeds commands via stdin pipe
  2. Listens on TCP 127.0.0.1:7777 for controller.py
  3. For each byte received, sends `sysbus.usart3 WriteChar <byte>` to Renode
  4. Captures Renode stdout (UART3 output) and writes to log file
"""
from __future__ import annotations
import os
import sys
import socket
import select
import subprocess
import threading
import argparse
import time
from datetime import datetime

import shutil, platform

def _find_renode() -> str:
    # Try PATH first (works on all platforms after install)
    found = shutil.which("renode")
    if found:
        return found
    # Windows fallback paths
    if platform.system() == "Windows":
        candidates = [
            r"C:\Program Files\Renode\bin\Renode.exe",
            r"C:\Program Files\Renode\renode.exe",
            r"C:\Program Files (x86)\Renode\renode.exe",
        ]
        for c in candidates:
            if os.path.exists(c):
                return c
    # macOS fallback
    mac_path = "/Applications/Renode.app/Contents/MacOS/renode"
    if os.path.exists(mac_path):
        return mac_path
    raise FileNotFoundError("renode not found. Install from https://renode.io")

RENODE_BIN  = _find_renode()
RESC_FILE   = os.path.join(os.path.dirname(__file__), "cthrobt.resc")
BRIDGE_PORT = 7777


def renode_writer(proc: subprocess.Popen, byte_queue: list, stop: threading.Event):
    """Read bytes from queue and send WriteChar commands to Renode stdin."""
    while not stop.is_set():
        if byte_queue:
            b = byte_queue.pop(0)
            cmd = f"sysbus.usart3 WriteChar {b}\n"
            try:
                proc.stdin.write(cmd.encode())
                proc.stdin.flush()
            except BrokenPipeError:
                break
        else:
            time.sleep(0.001)


def renode_reader(proc: subprocess.Popen, log_file, stop: threading.Event):
    """Read Renode stdout, filter noise, write to log file and print to terminal."""
    while not stop.is_set():
        line = proc.stdout.readline()
        if not line:
            break
        decoded = line.decode("utf-8", errors="replace").rstrip()
        # Skip Renode command echo lines (WriteChar noise)
        if "WriteChar" in decoded:
            continue
        log_file.write(decoded + "\n")
        log_file.flush()
        # Only print lines with actual STM32 output or bridge status
        if "[UART3]" in decoded or "[BRIDGE]" in decoded or "===" in decoded:
            print(decoded)


def main():
    parser = argparse.ArgumentParser(description="Renode UART bridge for cthrobt")
    default_log = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "received_signals.txt")
    parser.add_argument("--log", default=default_log,
                        help="Log file path (default: cthrobt_stm/received_signals.txt)")
    args = parser.parse_args()

    log_dir = os.path.dirname(args.log)
    if log_dir:
        os.makedirs(log_dir, exist_ok=True)
    log_file = open(args.log, "w", encoding="utf-8")
    ts = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    log_file.write(f"# Signal log started at {ts}\n")
    log_file.flush()

    print(f"[BRIDGE] Starting Renode simulation...")
    print(f"[BRIDGE] Log file: {args.log}")

    # Start Renode
    cwd = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    proc = subprocess.Popen(
        [RENODE_BIN, "--console", RESC_FILE],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        cwd=cwd,
    )

    byte_queue = []
    stop = threading.Event()

    t_write = threading.Thread(target=renode_writer, args=(proc, byte_queue, stop), daemon=True)
    t_read  = threading.Thread(target=renode_reader, args=(proc, log_file, stop), daemon=True)
    t_write.start()
    t_read.start()

    # Wait for Renode to boot
    time.sleep(4)

    # TCP server for controller.py
    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind(("127.0.0.1", BRIDGE_PORT))
    srv.listen(1)
    srv.setblocking(False)
    print(f"[BRIDGE] Waiting for controller.py on TCP 127.0.0.1:{BRIDGE_PORT}")

    clients = []

    try:
        while proc.poll() is None:
            readable, _, _ = select.select([srv] + clients, [], [], 0.1)
            for r in readable:
                if r is srv:
                    c, addr = srv.accept()
                    c.setblocking(False)
                    clients.append(c)
                    msg = f"[BRIDGE] controller.py connected from {addr[0]}:{addr[1]}"
                    print(msg)
                    log_file.write(msg + "\n")
                    log_file.flush()
                else:
                    try:
                        data = r.recv(4096)
                        if data:
                            byte_queue.extend(bytearray(data))
                        else:
                            clients.remove(r)
                            print("[BRIDGE] controller.py disconnected — waiting for Renode to finish processing...")
                            # Wait until all queued bytes have been written to Renode stdin
                            while byte_queue:
                                time.sleep(0.1)
                            # Wait for Renode output to go silent (no new [CTRL] output for 5 seconds)
                            last_size = os.path.getsize(log_file.name)
                            silent_start = time.time()
                            while True:
                                time.sleep(1)
                                current_size = os.path.getsize(log_file.name)
                                if current_size != last_size:
                                    last_size = current_size
                                    silent_start = time.time()
                                elif time.time() - silent_start >= 5:
                                    break
                            print("[BRIDGE] Queue drained. Log is ready for compare.py")
                    except Exception:
                        clients.remove(r)
    except KeyboardInterrupt:
        print("\n[BRIDGE] Shutting down...")
    finally:
        stop.set()
        proc.terminate()
        proc.wait()
        srv.close()
        log_file.write(f"# Session ended at {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        log_file.close()
        print(f"[BRIDGE] Log saved to: {args.log}")


if __name__ == "__main__":
    main()
