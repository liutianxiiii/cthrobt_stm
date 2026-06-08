# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

STM32F767ZI (Nucleo-F767ZI) firmware that receives Nintendo Switch Pro Controller
input from a companion PC-side project (`cthrobt`, sibling directory `../cthrobt`)
and processes it in real time — printing button/axis/hat state over UART3 and
driving a servo gripper. It supports two run modes selected at compile time:

- **Real hardware**: Ethernet (LwIP TCP server on port 7777) + TIM3 PWM gripper
- **Renode simulation** (`SIMULATION_MODE`): gamepad JSON injected byte-by-byte
  into UART3 by `renode/bridge.py`, gripper actions only printf rather than
  drive PWM

The build currently always compiles with `SIMULATION_MODE` (and the related
`SKIP_*` flags) defined in `CMakeLists.txt`. To target real hardware, remove
`SIMULATION_MODE`, `SKIP_USB_INIT`, `SKIP_ETH_INTERRUPT`, `SKIP_PHY_INIT` from
`add_compile_definitions()`.

## Commands

All scripts live in `scripts/` with both `.bat` (Windows) and `.sh` (macOS/Linux)
variants; use whichever matches the host.

```bash
# Build (configures CMake + Ninja, locates arm-none-eabi-gcc automatically)
scripts\build.bat            # Windows
bash scripts/build.sh        # macOS/Linux
# Output: build/Debug/cthrobt.elf (+ .bin, .hex)

# Clean (deletes build/ entirely — needed when switching toolchain versions)
scripts\clean.bat
bash scripts/clean.sh

# First-time tool setup (installs arm-none-eabi-gcc, cmake, ninja, Renode)
bash scripts/setup.sh        # macOS/Linux; setup.bat for Windows
```

There is no unit test suite for the firmware itself (it's embedded C). The
"test" is an end-to-end simulation comparison:

```bash
# Terminal 1: start Renode + bridge (waits for controller.py on TCP 7777)
scripts\run_simulation.bat   # Windows
bash scripts/run_simulation.sh

# Terminal 2 (from the sibling ../cthrobt directory): send mock gamepad data
bash ../cthrobt/scripts/run_mock.sh
```
Wait for `Queue drained. Log is ready for compare.py`, then (from `../cthrobt`)
run `bash ../cthrobt/scripts/compare.sh`, which diffs expected vs.
`received_signals.txt` and writes a report to `../cthrobt/logs/`.

There is also a self-contained Python harness, `scripts/test_simulation.py`,
which starts `renode/bridge.py`, sends a fixed table of test packets over TCP,
and asserts on substrings in `received_signals.txt` — run it directly with
`python3 scripts/test_simulation.py` to check a single scenario without the
cthrobt project.

Flashing real hardware (after rebuilding without `SIMULATION_MODE`):
```bash
STM32_Programmer_CLI -c port=SWD -w build/Debug/cthrobt.bin 0x08000000 -rst
```

## Architecture

```
controller.py (PC) ──TCP/JSON──► [real HW: LwIP TCP :7777]  ──┐
                                 [sim: bridge.py → UART3 WriteChar] ──► controller_task.c
                                                                          │
                                                              parses JSON, edge-detects A/B
                                                                          │
                                                               ┌──────────┴──────────┐
                                                               ▼                     ▼
                                                   printf → UART3 (debug)    gripper.c (TIM3 PWM
                                                   "[CTRL] A LX=+0.500"       or printf stub)
```

Key files:
- `Core/Src/controller_task.c` — FreeRTOS task entry point (`StartControllerTask`).
  Hand-rolled (non-library) JSON scanner that pulls `buttons`, `axes`, `hat` out
  of newline-delimited JSON lines without a full parser — see `parse_and_print()`.
  Branches at compile time on `SIMULATION_MODE`: either polls `huart3` byte-by-byte
  via `HAL_UART_Receive`, or runs an LwIP `netconn` TCP server on port `CTRL_PORT`
  (7777). A/B button state is edge-detected (`prev_a`/`prev_b`) and dispatches to
  `gripper_close()`/`gripper_open()` — holding a button does not repeat the action.
- `Core/Src/gripper.c` — Servo control via TIM3 Channel 1 PWM on PB4. Guarded by
  `#ifndef SIMULATION_MODE`: real builds drive `htim3` CCR (`GRIPPER_CCR_CLOSE`
  =1000 / `GRIPPER_CCR_OPEN`=2000, 50 Hz/20 ms period), simulation builds only
  printf `[GRIPPER] OPEN`/`[GRIPPER] CLOSE`. `htim3` is initialized by
  `MX_TIM3_Init()` in `main.c`.
- `Core/Src/main.c` — HAL/clock/peripheral init, FreeRTOS task creation, LwIP init.
- `Core/Src/freertos.c` — FreeRTOS task/idle-memory glue (mostly CubeMX-generated).
- `cmake/stm32_toolchain.cmake` — cross-platform ARM GCC toolchain file; locates
  `arm-none-eabi-gcc` via `find_program` (was hardcoded to a macOS STM32CubeIDE
  path before the Windows port — keep it portable).
- `renode/bridge.py` — Python bridge for simulation: starts Renode, loads
  `cthrobt.resc`/`nucleo_f767zi.repl`/the built `.elf`, listens on TCP :7777 for
  `controller.py`, injects each received byte into UART3 via
  `sysbus.usart3 WriteChar`, and captures UART3 output to `received_signals.txt`.
  It detects "queue drained" by watching the log file for ~5s of silence (Renode
  processes injected bytes asynchronously, much slower than real time).
- `STM32F767ZITX_FLASH.ld` / `_RAM.ld` — linker scripts (flash vs. RAM targets).
- `cthrobt.ioc` — STM32CubeMX project file; source of truth for peripheral config
  (e.g. TIM3 CH1 PWM on PB4, prescaler 95 / ARR 19999 for 50 Hz). Regenerating
  code from this file will overwrite CubeMX-managed regions in `Core/Src`.

## Communication protocol

Newline-delimited JSON, one packet per line, both in real TCP mode and simulation
UART3 injection:
```json
{"buttons":{"A":true,"B":false,...},"axes":{"LX":0.5,"LY":0.0,...},"hat":[0,1]}
```
Firmware echoes parsed state over UART3/printf, e.g. `[CTRL] A LX=+0.500  HAT:UP`
or `[CTRL] (idle)` when nothing is active, plus `[GRIPPER] OPEN`/`[GRIPPER] CLOSE`
on A/B edges.

## Gotchas (from DEVELOPMENT_NOTES.md)

- **`%f` printf prints nothing**: newlib-nano (`-specs=nano.specs`) disables float
  printf by default. The fix is the `-u _printf_float` linker flag already present
  in `CMakeLists.txt` — don't remove it, or `LX=` will print with no value.
- **`\r\n` splits UART3 lines in Renode**: Renode's `AddLineHook` fires on `\r`
  *and* `\n` separately. `controller_task.c` therefore prints with `\n` only
  under `#ifdef SIMULATION_MODE`, and `\r\n` otherwise (real UART/terminal).
  Preserve this distinction when touching printf calls in that file.
- **Renode runs slower than real time**: bytes are injected one at a time via
  `WriteChar`; `bridge.py` waits for ~5 seconds of log-file silence before
  declaring the queue drained. Don't assume immediate processing when scripting
  against the simulation.
- New Windows terminals may not have `arm-none-eabi-gcc`/`ninja` on PATH right
  after a winget install (needs a shell restart) — `build.bat` already works
  around this by probing the default install locations.

## Hardware reference (for real-board work)

See `HARDWARE_SETUP.md` (Chinese) and `GRIPPER_HARDWARE_SETUP.md` for full wiring
and CubeMX details. Summary:
- Servo signal → PB4 (CN9 pin 6 / Arduino D5, TIM3_CH1, AF2); power from 5V rail
  (not 3.3V — stall current causes brown-out), ground to any GND.
- Board uses a **static IP** `192.168.0.20` on TCP port 7777 (no DHCP); the host
  PC must be on the same `192.168.0.x` subnet.
- Debug output is on USART3 (PD8 TX / PD9 RX) via the ST-Link virtual COM port,
  115200 baud.
