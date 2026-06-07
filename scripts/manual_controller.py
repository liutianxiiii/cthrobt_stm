#!/usr/bin/env python3
"""
Interactive gamepad controller for Renode simulation.
Connects to bridge.py on TCP 7777 and sends button presses on keypress.

Usage:
    python3 scripts/manual_controller.py
"""
import sys, socket, json, tty, termios, threading, os, time

PORT = 7777
LOG  = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "received_signals.txt")

KEYS = {
    'a': 'A',
    'b': 'B',
    'w': 'HAT_UP',   # hat up (LY=+1 style)
    's': 'HAT_DOWN',
    'x': 'X',
    'y': 'Y',
}

HELP = """
┌─────────────────────────────────────────┐
│  Manual Gamepad Controller              │
│─────────────────────────────────────────│
│  a  →  Button A  (close gripper)        │
│  b  →  Button B  (open gripper)         │
│  x  →  Button X                         │
│  y  →  Button Y                         │
│  w  →  Hat UP                           │
│  s  →  Hat DOWN                         │
│  q  →  Quit                             │
└─────────────────────────────────────────┘
(每次按键 = 按下 + 自动松开)
"""

def send(sock, payload):
    line = json.dumps(payload, separators=(",", ":")) + "\n"
    sock.sendall(line.encode())

def press_button(sock, name):
    send(sock, {"buttons": {name: True}, "axes": {}, "hat": [0, 0]})
    time.sleep(0.05)
    send(sock, {"buttons": {}, "axes": {}, "hat": [0, 0]})

def press_hat(sock, x, y):
    send(sock, {"buttons": {}, "axes": {}, "hat": [x, y]})
    time.sleep(0.05)
    send(sock, {"buttons": {}, "axes": {}, "hat": [0, 0]})

def tail_log():
    seen = 0
    while True:
        try:
            content = open(LOG, encoding="utf-8").read()
            if len(content) > seen:
                for line in content[seen:].splitlines():
                    if "[UART3]" in line and "WriteChar" not in line:
                        clean = line.replace("[UART3]", "").strip()
                        # strip ANSI
                        import re
                        clean = re.sub(r'\x1b\[[0-9;]*m', '', clean).strip()
                        if clean:
                            print(f"\r  STM32> {clean}\n: ", end="", flush=True)
                seen = len(content)
        except FileNotFoundError:
            pass
        time.sleep(0.2)

def main():
    try:
        sock = socket.create_connection(("127.0.0.1", PORT), timeout=3)
    except OSError:
        print("ERROR: bridge.py not running. Start it first:")
        print("  python3 renode/bridge.py &")
        sys.exit(1)

    print(HELP)

    threading.Thread(target=tail_log, daemon=True).start()

    fd = sys.stdin.fileno()
    old = termios.tcgetattr(fd)
    try:
        tty.setraw(fd)
        while True:
            ch = sys.stdin.read(1)
            if ch == 'q':
                break
            elif ch == 'a':
                sys.stdout.write("\r[你按下了 A]\n")
                sys.stdout.flush()
                press_button(sock, 'A')
            elif ch == 'b':
                sys.stdout.write("\r[你按下了 B]\n")
                sys.stdout.flush()
                press_button(sock, 'B')
            elif ch == 'x':
                sys.stdout.write("\r[你按下了 X]\n")
                sys.stdout.flush()
                press_button(sock, 'X')
            elif ch == 'y':
                sys.stdout.write("\r[你按下了 Y]\n")
                sys.stdout.flush()
                press_button(sock, 'Y')
            elif ch == 'w':
                sys.stdout.write("\r[Hat UP]\n")
                sys.stdout.flush()
                press_hat(sock, 0, 1)
            elif ch == 's':
                sys.stdout.write("\r[Hat DOWN]\n")
                sys.stdout.flush()
                press_hat(sock, 0, -1)
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old)
        sock.close()
        print("\n[退出]")

if __name__ == "__main__":
    main()
