# cthrobt STM32 模拟运行指南

## 项目结构

```
ctrlall/
├── cthrobt/                  # 手柄控制端（PC 侧）
│   ├── controller.py         # 读取手柄，发送 JSON 到 STM32
│   ├── simulator.py          # Python 版 STM32 网络模拟器（协议调试用）
│   ├── config.ini            # controller.py 配置
│   └── logs/                 # controller.py 日志
│
└── cthrobt_stm/              # STM32 固件（本项目）
    ├── Core/                 # 应用代码
    ├── renode/
    │   ├── bridge.py         # 桥接脚本（核心启动入口）
    │   ├── cthrobt.resc      # Renode 模拟脚本
    │   └── nucleo_f767zi.repl # NUCLEO-F767ZI 平台定义
    ├── build/Debug/
    │   └── cthrobt.elf       # 编译产物
    └── received_signals.txt  # 手柄信号日志（运行后生成）
```

---

## 系统架构

```
手柄
  │
  ▼
controller.py ──TCP 7777──► bridge.py ──WriteChar──► Renode（STM32 固件）
                                                          │
                                                       UART3 输出
                                                          │
                                                          ▼
                                               received_signals.txt
```

- **controller.py**：读取手柄输入，以 JSON 格式发送到 `127.0.0.1:7777`
- **bridge.py**：接收 controller.py 的 TCP 数据，通过 Renode 的 monitor 接口注入到 STM32 的 UART3
- **Renode**：运行 STM32F767ZI 固件，固件解析 JSON 并通过 UART3 打印手柄信号
- **received_signals.txt**：记录 STM32 固件输出的所有信号

---

## 依赖安装

### ARM GCC（已通过 STM32CubeIDE 内置，无需额外安装）

路径：
```
/Applications/STM32CubeIDE.app/Contents/Eclipse/plugins/
  com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.macosaarch64_1.0.0.202602081740/
  tools/bin/
```

### Renode（已安装到 `/Applications/Renode.app`）

如需重新安装：从 [github.com/renode/renode/releases](https://github.com/renode/renode/releases) 下载 `renode-*-dotnet.osx-arm64-portable.dmg`，拖入 `/Applications`。

### pygame（controller.py 依赖）

```bash
brew install sdl2 sdl2_image sdl2_mixer sdl2_ttf
pip3 install pygame
```

---

## 编译固件

```bash
cd /Users/liutianxi/ctrlall/cthrobt_stm

ARM_GCC_PATH="/Applications/STM32CubeIDE.app/Contents/Eclipse/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.macosaarch64_1.0.0.202602081740/tools/bin"
export PATH="$ARM_GCC_PATH:$PATH"

cmake -B build/Debug \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=cmake/stm32_toolchain.cmake \
  -G "Unix Makefiles"

cmake --build build/Debug -- -j4
```

编译成功后输出 `build/Debug/cthrobt.elf`。

---

## 启动模拟（完整流程）

### 第一步：启动 STM32 模拟器 + 桥接

```bash
cd /Users/liutianxi/ctrlall/cthrobt_stm
python3 renode/bridge.py
```

启动后会看到：
```
[BRIDGE] Starting Renode simulation...
[BRIDGE] Log file: /Users/liutianxi/ctrlall/cthrobt_stm/received_signals.txt
[BRIDGE] Waiting for controller.py on TCP 127.0.0.1:7777
...
[UART3] [BOOT] FreeRTOS starting...
[UART3] [CTRL] Task started
[UART3] [CTRL] Simulation mode - reading gamepad data from UART3
```

### 第二步：连接手柄并启动 controller.py

新开一个终端：

```bash
cd /Users/liutianxi/ctrlall/cthrobt
python3 controller.py
```

看到 `[NET] Connected → 127.0.0.1:7777` 表示连接成功。

### 第三步：操作手柄

按下手柄按钮或推动摇杆，`received_signals.txt` 实时更新。

---

## 查看信号日志

```bash
# 实时监看
tail -f /Users/liutianxi/ctrlall/cthrobt_stm/received_signals.txt

# 只看手柄信号行
grep "\[CTRL\]" /Users/liutianxi/ctrlall/cthrobt_stm/received_signals.txt
```

### 输出示例

```
[UART3] [CTRL] Simulation mode - reading gamepad data from UART3
[UART3] [CTRL] (idle)
[UART3] [CTRL] A
[UART3] [CTRL] B  A
[UART3] [CTRL] LX=+0.523  LY=-0.210
[UART3] [CTRL] HAT:UP
[UART3] [CTRL] ZL  LX=+0.800
```

---

## 配置说明

### controller.py 配置（`cthrobt/config.ini`）

| 参数 | 当前值 | 说明 |
|------|--------|------|
| `host` | `127.0.0.1` | 模拟时连本地桥接器；真实板子改为 `192.168.0.20` |
| `port` | `7777` | 固定不变 |
| `protocol` | `json` | JSON 格式（模拟器支持） |
| `only_on_change` | `true` | 只在状态改变时发送 |
| `deadzone` | `0.05` | 摇杆死区阈值 |

### bridge.py 参数

```bash
python3 renode/bridge.py --log /path/to/output.txt
```

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `--log` | `cthrobt_stm/received_signals.txt` | 日志输出文件 |

---

## 对接真实 STM32 板子

1. 修改 `cthrobt/config.ini`：
   ```ini
   host = 192.168.0.20   # STM32 的实际 IP
   port = 7777
   ```

2. 编译不含模拟宏的固件（去掉 `CMakeLists.txt` 中的模拟相关定义）：
   在 `CMakeLists.txt` 里删除以下几行：
   ```cmake
   SIMULATION_MODE
   SKIP_USB_INIT
   SKIP_ETH_INTERRUPT
   SKIP_PHY_INIT
   ```

3. 通过 STM32CubeIDE 或 `st-flash` 烧录 `build/Debug/cthrobt.elf`

4. 直接运行 controller.py，数据通过以太网发送到板子

---

## 常见问题

**Q: bridge.py 启动后看不到 `[CTRL] Simulation mode`？**
- 等待约 5 秒，Renode 需要时间初始化
- 检查 `build/Debug/cthrobt.elf` 是否存在

**Q: controller.py 显示 `Connection refused`？**
- 确认 bridge.py 已启动并显示 `Waiting for controller.py on TCP 127.0.0.1:7777`

**Q: 手柄没有被识别？**
- 确认手柄已插入并在 `config.ini` 中 `name_contains` 为空（自动选第一个设备）
- 检查 pygame 是否正确安装：`python3 -c "import pygame; pygame.init(); print(pygame.joystick.get_count())"`

**Q: 如何停止模拟？**
- 在 bridge.py 终端按 `Ctrl+C`
