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

RENODE_BIN  = "/Applications/Renode.app/Contents/MacOS/renode"
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
    """Read Renode stdout and write to log file + print to terminal."""
    while not stop.is_set():
        line = proc.stdout.readline()
        if not line:
            break
        decoded = line.decode("utf-8", errors="replace").rstrip()
        print(decoded)
        log_file.write(decoded + "\n")
        log_file.flush()


def main():
    parser = argparse.ArgumentParser(description="Renode UART bridge for cthrobt")
    parser.add_argument("--log", default="/Users/liutianxi/ctrlall/cthrobt_stm/received_signals.txt",
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
                            print("[BRIDGE] controller.py disconnected")
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
