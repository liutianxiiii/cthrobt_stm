# cthrobt_stm — STM32 手柄接收固件

[English](README.md)

STM32F767ZI（Nucleo-F767ZI）固件，接收来自 PC 端 `cthrobt` 项目的 Nintendo Switch Pro
手柄输入并实时处理。同时支持真实硬件（以太网/LwIP）和 Renode 仿真（UART3 注入）两种模式。

## 架构

```
┌──────────────────────────────────────────────────────────┐
│                   cthrobt_stm (STM32F767ZI)              │
│                                                          │
│  ┌──────────────┐    ┌─────────────────────────────┐    │
│  │  FreeRTOS    │    │     controller_task.c        │    │
│  │  Task        │───►│  JSON 解析器                 │    │
│  │  Scheduler   │    │  buttons / axes / hat        │    │
│  └──────────────┘    │  printf → UART3（调试输出）  │    │
│                      └─────────────────────────────┘    │
│                              ▲                           │
│         SIMULATION_MODE      │          普通模式         │
│  UART3 (Renode WriteChar) ───┤                           │
│                              │  LwIP TCP 端口 7777       │
│                              │  （真实以太网）            │
└──────────────────────────────────────────────────────────┘
```

### 关键组件

| 路径 | 作用 |
|------|------|
| `Core/Src/controller_task.c` | 主逻辑：JSON 解析，通过 printf 输出状态 |
| `Core/Src/main.c` | HAL 初始化、FreeRTOS 任务创建、LwIP 初始化 |
| `Core/Src/freertos.c` | 任务定义 |
| `cmake/stm32_toolchain.cmake` | 跨平台 ARM GCC 工具链配置 |
| `renode/bridge.py` | Python 桥接程序：TCP ↔ Renode UART3 注入 |
| `renode/cthrobt.resc` | Renode 仿真脚本 |
| `renode/nucleo_f767zi.repl` | Renode 平台描述（STM32F767ZI 外设） |

### 编译期开关（CMakeLists.txt）

| 开关 | 作用 |
|------|------|
| `SIMULATION_MODE` | 从 UART3（Renode）而非 LwIP TCP 读取手柄数据 |
| `SKIP_USB_INIT` | 跳过 USB OTG 初始化（未使用） |
| `SKIP_ETH_INTERRUPT` | 跳过 ETH 中断设置（由 LwIP 处理） |
| `SKIP_PHY_INIT` | 仿真模式下跳过 LAN8742 PHY 初始化 |

## 通信协议

### 输入（来自 controller.py / bridge.py）
按行分隔的 JSON，通过 TCP（真实模式）或 UART3 WriteChar 注入（仿真模式）传输：
```json
{"buttons":{"A":true,"B":false,...},"axes":{"LX":0.5,"LY":0.0,...},"hat":[0,1]}
```

### 输出（通过 UART3 / printf）
固件解析每一行 JSON 并输出当前激活的元素：
```
[CTRL] A LX=+0.500  HAT:UP
[CTRL] B X ZR RX=-0.293
[CTRL] (idle)
```

## Renode 仿真

仿真流程：
```
controller.py --mock
      │ TCP JSON 数据包
      ▼
renode/bridge.py
      │ sysbus.usart3 WriteChar <byte>（逐字节注入）
      ▼
Renode (nucleo-f767zi)
      │ STM32 UART3 接收
      ▼
controller_task.c (SIMULATION_MODE)
      │ 解析 JSON → printf
      ▼
UART3 输出 → bridge.py 捕获 → received_signals.txt
```

## 构建

### 首次环境搭建

首次使用前运行 setup 脚本，自动安装所有必需工具（arm-none-eabi-gcc、cmake、ninja、Renode）：

```bash
# macOS / Linux
bash scripts/setup.sh

# Windows
scripts\setup.bat
```

### 前置依赖

| 工具 | 版本 | 安装方式 |
|------|------|---------|
| arm-none-eabi-gcc | 14.2+ | `winget install Arm.GnuArmEmbeddedToolchain` / `brew install arm-none-eabi-gcc` |
| cmake | 3.22+ | `winget install Kitware.CMake` / `pip install cmake` |
| ninja | 1.11+ | `winget install Ninja-build.Ninja` / `pip install ninja` |
| Renode | 1.16+ | `winget install Renode.Renode` / https://renode.io |

### 构建固件

```bash
# macOS / Linux
bash scripts/build.sh

# Windows
scripts\build.bat
```

输出：`build/Debug/cthrobt.elf`（以及 `.bin`、`.hex`）

### 清理

```bash
bash scripts/clean.sh    # macOS / Linux
scripts\clean.bat        # Windows
```

## 运行仿真

```bash
# 终端 1 — 启动 Renode 仿真
bash scripts/run_simulation.sh    # macOS / Linux
scripts\run_simulation.bat        # Windows

# 终端 2 — 发送模拟手柄数据（在 cthrobt 目录下执行）
bash ../cthrobt/scripts/run_mock.sh
```

等待出现 `Queue drained. Log is ready for compare.py`，然后在 cthrobt 目录下运行对比脚本：
```bash
bash ../cthrobt/scripts/compare.sh
```

## 脚本说明

所有脚本位于 `scripts/` 目录，每个脚本都有 `.bat`（Windows）和 `.sh`（macOS/Linux）两个版本。

| 脚本 | Windows | macOS/Linux | 说明 |
|--------|---------|-------------|-------------|
| **setup** | `scripts\setup.bat` | `bash scripts/setup.sh` | 首次安装所需工具：arm-none-eabi-gcc、cmake、ninja、Renode。克隆仓库后、首次构建前运行一次。 |
| **build** | `scripts\build.bat` | `bash scripts/build.sh` | 配置并构建固件，自动定位 arm-none-eabi-gcc 和 ninja。输出：`build/Debug/cthrobt.elf`。首次克隆或修改源码后运行。 |
| **run_simulation** | `scripts\run_simulation.bat` | `bash scripts/run_simulation.sh` | 使用 `cthrobt.elf` 启动 Renode 仿真。运行 `renode/bridge.py`：(1) 启动 Renode，(2) 在 TCP 7777 端口监听 controller.py，(3) 将收到的字节注入 STM32 UART3，(4) 将 STM32 输出捕获到 `received_signals.txt`。处理完所有数据后打印 "Log is ready for compare.py"。 |
| **clean** | `scripts\clean.bat` | `bash scripts/clean.sh` | 完全删除 `build/` 目录。在完整重建或更换工具链版本前运行。 |

### 典型工作流

**首次运行 / 修改源码后 — 先构建：**
```
scripts\build.bat          (Windows)
bash scripts/build.sh      (macOS/Linux)
```

---

**正常操作 — 真实手柄 → Renode 仿真：**
```
[终端 1 — cthrobt_stm]              [终端 2 — cthrobt]
scripts\run_simulation.bat    →      （等待 "Waiting for controller.py"）
                                     scripts\run_controller.bat
                                     （连接 Nintendo 手柄，按下按键）
按 Ctrl+C 停止
```

---

**通信测试 — 无需手柄：**
```
[终端 1 — cthrobt_stm]              [终端 2 — cthrobt]
scripts\run_simulation.bat    →      （等待 "Waiting for controller.py"）
                                     scripts\run_mock.bat
（等待 "Log is ready"）        →      scripts\compare.bat
                                     打开 logs\test_report_*.md
```

---

**不使用 Renode 调试 — 使用独立模拟器：**
```
[终端 1 — cthrobt]                  [终端 2 — cthrobt]
scripts\run_simulator.bat     →      scripts\run_controller.bat
                                     （或 scripts\run_mock.bat）
```
> 注意：`run_simulator.bat` 位于 `cthrobt` 项目目录中。

### `run_simulation` 详细执行过程

```
bridge.py 启动 Renode
    └─ 加载 renode/cthrobt.resc
           └─ 加载 renode/nucleo_f767zi.repl（STM32F767ZI 外设描述）
           └─ 加载 build/Debug/cthrobt.elf
           └─ 在 :3333 启动 GDB 服务器
           └─ 将 UART3 输出挂钩到日志
    └─ 等待 4 秒让 Renode 启动完成
    └─ 在 TCP :7777 端口开放，等待 controller.py 连接
    └─ 每收到一个字节 → 通过 WriteChar 注入 Renode UART3
    └─ controller 断开连接后：等待静默 → 写入 "Log is ready"
```

## 烧录到真实硬件

```bash
# 使用 STM32CubeProgrammer CLI
STM32_Programmer_CLI -c port=SWD -w build/Debug/cthrobt.bin 0x08000000 -rst
```

烧录完成后，连接以太网并运行指向开发板 IP 的 `controller.py`：
```bash
python controller.py --config config.ini  # 在 config.ini 中设置 host = <开发板 IP>
```

## 内存占用（典型值）

| 区域 | 已用 | 总量 | 占比 |
|--------|------|-------|---|
| FLASH | ~109 KB | 2 MB | ~5% |
| RAM | ~70 KB | 512 KB | ~14% |
