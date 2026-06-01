# Development Notes — cthrobt_stm

## Session: 2026-06-01 (Windows 11 environment setup)

### What was done
- Ported build system from macOS to Windows 11
- Fixed hardcoded macOS paths throughout the project
- Confirmed Renode simulation + communication test (100% pass rate)

### Files modified from macOS original

| File | Change |
|------|--------|
| `cmake/stm32_toolchain.cmake` | Replaced hardcoded STM32CubeIDE macOS path with cross-platform `find_program(arm-none-eabi-gcc)` |
| `.vscode/c_cpp_properties.json` | Changed `compilerPath` to Windows ARM toolchain path |
| `.vscode/tasks.json` | Changed generator `Unix Makefiles` → `Ninja`; fixed `rm -rf` → `cmake -E remove_directory`; removed macOS PATH prefix |
| `renode/bridge.py` | Windows Renode path detection; Windows log path; queue drain detection; WriteChar echo filter |
| `Core/Src/controller_task.c` | `#ifdef SIMULATION_MODE` uses `\n` only (not `\r\n`) to avoid Renode UART hook splitting |
| `CMakeLists.txt` | Added `-u _printf_float` linker flag (newlib-nano requires this for `%f` support) |

### Key bugs fixed

**`%f` printf outputting nothing**
- Root cause: `-specs=nano.specs` (newlib-nano) disables float printf by default
- Fix: add `-u _printf_float` to linker options in CMakeLists.txt
- Symptom: `[CTRL] A LX=` (value missing after `=`)

**`\r\n` splitting UART3 line hook**
- Root cause: Renode fires `AddLineHook` on `\r` AND `\n` separately, causing output to split
- Fix: `#ifdef SIMULATION_MODE` uses `printf("[CTRL] %s\n", ...)` (no `\r`)
- Symptom: received line cut at axis name before value

**Renode processing lag**
- Root cause: `bridge.py` injects bytes one at a time via `sysbus.usart3 WriteChar N` — Renode processes these asynchronously, much slower than real-time
- Fix: `bridge.py` monitors log file size for 5-second silence after queue drains before signaling readiness
- Symptom: compare.py run too early → only 4/20 packets received

### Tool setup (Windows 11)

| Tool | Install method | Path |
|------|---------------|------|
| arm-none-eabi-gcc 14.2 | `winget install Arm.GnuArmEmbeddedToolchain` | `C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\14.2 rel1\bin` |
| cmake 4.3 | `pip install cmake` + `winget install Kitware.CMake` | Via `python -m cmake` |
| ninja 1.13 | `pip install ninja` + `winget install Ninja-build.Ninja` | Via winget packages dir |
| Renode 1.16 | `winget install Renode.Renode` | `C:\Program Files\Renode\bin\Renode.exe` |

### Current state
- Firmware builds successfully (FLASH ~109KB / 2MB, RAM ~70KB / 512KB)
- Renode simulation working with Nucleo-F767ZI platform definition
- Communication test: 20/20 packets matched (100% pass rate)
- `scripts/` directory contains Windows (.bat) and macOS (.sh) build/run scripts

## Next steps
- [ ] Test on real Nucleo-F767ZI hardware over Ethernet
- [ ] Implement binary protocol parsing in `controller_task.c` (JSON only currently)
- [ ] Add watchdog timer for production use
- [ ] Consider adding RX packet count / error stats output via UART3

## Known issues / gotchas
- **Build requires PATH setup**: On a new Windows terminal, arm-none-eabi-gcc and ninja are not auto-added to PATH until shell restart after winget install. `build.bat` handles this automatically.
- **Renode boot time**: `bridge.py` waits 4 seconds for Renode to initialize before accepting TCP connections. Don't run mock immediately after starting bridge.py.
- **SIMULATION_MODE only**: Current build always compiles with `SIMULATION_MODE` defined. To build for real hardware, remove this flag from `add_compile_definitions()` in CMakeLists.txt.
