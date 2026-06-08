# cthrobt_stm — STM32 Gamepad Receiver Firmware

[中文版](README.zh-CN.md)

STM32F767ZI (Nucleo-F767ZI) firmware that receives Nintendo Switch Pro Controller input from `cthrobt` (PC side) and processes it in real-time. Supports both real hardware (Ethernet/LwIP) and Renode simulation (UART3 injection).

## Architecture

```
┌──────────────────────────────────────────────────────────┐
│                   cthrobt_stm (STM32F767ZI)              │
│                                                          │
│  ┌──────────────┐    ┌─────────────────────────────┐    │
│  │  FreeRTOS    │    │     controller_task.c        │    │
│  │  Task        │───►│  JSON parser                 │    │
│  │  Scheduler   │    │  buttons / axes / hat        │    │
│  └──────────────┘    │  printf → UART3 (debug out)  │    │
│                      └─────────────────────────────┘    │
│                              ▲                           │
│         SIMULATION_MODE      │          Normal mode      │
│  UART3 (Renode WriteChar) ───┤                           │
│                              │  LwIP TCP port 7777       │
│                              │  (real Ethernet)          │
└──────────────────────────────────────────────────────────┘
```

### Key Components

| Path | Role |
|------|------|
| `Core/Src/controller_task.c` | Main logic: JSON parsing, state output via printf |
| `Core/Src/main.c` | HAL init, FreeRTOS task creation, LwIP init |
| `Core/Src/freertos.c` | Task definitions |
| `cmake/stm32_toolchain.cmake` | Cross-platform ARM GCC toolchain config |
| `renode/bridge.py` | Python bridge: TCP ↔ Renode UART3 injection |
| `renode/cthrobt.resc` | Renode simulation script |
| `renode/nucleo_f767zi.repl` | Renode platform description (STM32F767ZI peripherals) |

### Compile-time flags (CMakeLists.txt)

| Flag | Effect |
|------|--------|
| `SIMULATION_MODE` | Read gamepad data from UART3 (Renode) instead of LwIP TCP |
| `SKIP_USB_INIT` | Skip USB OTG init (not used) |
| `SKIP_ETH_INTERRUPT` | Skip ETH interrupt setup (handled by LwIP) |
| `SKIP_PHY_INIT` | Skip LAN8742 PHY init in simulation |

## Communication Protocol

### Input (from controller.py / bridge.py)
Newline-delimited JSON over TCP (real mode) or UART3 WriteChar injection (simulation):
```json
{"buttons":{"A":true,"B":false,...},"axes":{"LX":0.5,"LY":0.0,...},"hat":[0,1]}
```

### Output (via UART3 / printf)
The firmware parses each JSON line and outputs active elements:
```
[CTRL] A LX=+0.500  HAT:UP
[CTRL] B X ZR RX=-0.293
[CTRL] (idle)
```

## Renode Simulation

The simulation flow:
```
controller.py --mock
      │ TCP JSON packets
      ▼
renode/bridge.py
      │ sysbus.usart3 WriteChar <byte>  (one byte at a time)
      ▼
Renode (nucleo-f767zi)
      │ STM32 UART3 receive
      ▼
controller_task.c (SIMULATION_MODE)
      │ parse JSON → printf
      ▼
UART3 output → bridge.py captures → received_signals.txt
```

## Build

### Prerequisites

| Tool | Version | Install |
|------|---------|---------|
| arm-none-eabi-gcc | 14.2+ | `winget install Arm.GnuArmEmbeddedToolchain` / `brew install arm-none-eabi-gcc` |
| cmake | 3.22+ | `winget install Kitware.CMake` / `pip install cmake` |
| ninja | 1.11+ | `winget install Ninja-build.Ninja` / `pip install ninja` |
| Renode | 1.16+ | `winget install Renode.Renode` / https://renode.io |

### Build firmware

```bash
# macOS / Linux
bash scripts/build.sh

# Windows
scripts\build.bat
```

Output: `build/Debug/cthrobt.elf` (+ `.bin` and `.hex`)

### Clean

```bash
bash scripts/clean.sh    # macOS / Linux
scripts\clean.bat        # Windows
```

## Run Simulation

```bash
# Terminal 1 — Start Renode simulation
bash scripts/run_simulation.sh    # macOS / Linux
scripts\run_simulation.bat        # Windows

# Terminal 2 — Send mock gamepad data (from cthrobt directory)
bash ../cthrobt/scripts/run_mock.sh
```

Wait for `Queue drained. Log is ready for compare.py`, then run compare (from cthrobt directory):
```bash
bash ../cthrobt/scripts/compare.sh
```

## Scripts Reference

All scripts are in the `scripts/` directory. Each has a `.bat` (Windows) and `.sh` (macOS/Linux) version.

| Script | Windows | macOS/Linux | Description |
|--------|---------|-------------|-------------|
| **build** | `scripts\build.bat` | `bash scripts/build.sh` | Configure and build the firmware. Automatically locates arm-none-eabi-gcc and ninja. Output: `build/Debug/cthrobt.elf`. Run after first clone or after changing source files. |
| **run_simulation** | `scripts\run_simulation.bat` | `bash scripts/run_simulation.sh` | Start Renode simulation with `cthrobt.elf`. Launches `renode/bridge.py` which: (1) starts Renode, (2) listens on TCP 7777 for controller.py, (3) injects received bytes into STM32 UART3, (4) captures STM32 output to `received_signals.txt`. Prints "Log is ready for compare.py" when all data has been processed. |
| **clean** | `scripts\clean.bat` | `bash scripts/clean.sh` | Delete the `build/` directory entirely. Run before a full rebuild or when switching toolchain versions. |

### Typical workflows

**First time / after source changes — build first:**
```
scripts\build.bat          (Windows)
bash scripts/build.sh      (macOS/Linux)
```

---

**Normal operation — real gamepad → Renode simulation:**
```
[Terminal 1 — cthrobt_stm]          [Terminal 2 — cthrobt]
scripts\run_simulation.bat    →      (wait for "Waiting for controller.py")
                                     scripts\run_controller.bat
                                     (connect Nintendo gamepad, press buttons)
Ctrl+C to stop
```

---

**Communication test — no gamepad needed:**
```
[Terminal 1 — cthrobt_stm]          [Terminal 2 — cthrobt]
scripts\run_simulation.bat    →      (wait for "Waiting for controller.py")
                                     scripts\run_mock.bat
(wait for "Log is ready")     →      scripts\compare.bat
                                     open logs\test_report_*.md
```

---

**Debug without Renode — use standalone simulator:**
```
[Terminal 1 — cthrobt]              [Terminal 2 — cthrobt]
scripts\run_simulator.bat     →      scripts\run_controller.bat
                                     (or scripts\run_mock.bat)
```
> Note: `run_simulator.bat` is in the `cthrobt` project directory.

### What `run_simulation` does in detail

```
bridge.py starts Renode
    └─ loads renode/cthrobt.resc
           └─ loads renode/nucleo_f767zi.repl  (STM32F767ZI peripherals)
           └─ loads build/Debug/cthrobt.elf
           └─ starts GDB server on :3333
           └─ hooks UART3 output to log
    └─ waits 4s for Renode to boot
    └─ opens TCP :7777 for controller.py
    └─ for each received byte → WriteChar to Renode UART3
    └─ after controller disconnects: waits for silence → writes "Log is ready"
```

## Flash to Real Hardware

```bash
# Using STM32CubeProgrammer CLI
STM32_Programmer_CLI -c port=SWD -w build/Debug/cthrobt.bin 0x08000000 -rst
```

After flashing, connect Ethernet and run `controller.py` pointed at the board's IP:
```bash
python controller.py --config config.ini  # set host = <board IP> in config.ini
```

## Memory Usage (typical)

| Region | Used | Total | % |
|--------|------|-------|---|
| FLASH | ~109 KB | 2 MB | ~5% |
| RAM | ~70 KB | 512 KB | ~14% |
