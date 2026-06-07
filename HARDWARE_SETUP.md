# 硬件连接指南

本文档描述将固件部署到真实 NUCLEO-F767ZI 开发板所需的全部步骤。

---

## 1. 所需硬件

| 物品 | 说明 |
|---|---|
| NUCLEO-F767ZI | STM32F767ZI 开发板 |
| USB Type-A to Mini-B 线 | 连接 ST-Link（CN1），用于供电、烧录和调试串口 |
| RJ45 网线 | 连接开发板以太网口（CN6）到路由器或电脑 |
| 标准舵机 | 爪子执行器，信号电压 3.3V 兼容 |
| 杜邦线若干 | 舵机接线 |

---

## 2. 舵机接线

| 舵机线颜色 | 连接到 | 说明 |
|---|---|---|
| 信号线（橙/黄） | PB4 — CN9 第 6 脚（Arduino D5） | PWM 控制信号 |
| 电源线（红） | 5V — CN8 第 7 脚（VIN）或外部 5V | 不要接 3.3V |
| 地线（棕/黑） | GND — CN8 第 11 脚或任意 GND | |

> 舵机堵转电流较大，建议使用外部 5V 电源单独供电，避免 Nucleo 板过流。

---

## 3. 网络连接

开发板使用**静态 IP，不使用 DHCP**：

| 参数 | 值 |
|---|---|
| 板子 IP | `192.168.0.20` |
| 子网掩码 | `255.255.255.0` |
| 网关 | 无（0.0.0.0） |
| 监听端口 | TCP 7777 |

**电脑侧需要在同一子网**，例如将电脑有线网卡手动配置为：
- IP：`192.168.0.10`（任意 `192.168.0.x`，非 20）
- 子网掩码：`255.255.255.0`

连接方式：用网线将 Nucleo CN6（RJ45）直连电脑，或同接路由器。

---

## 4. STM32CubeMX 配置（TIM3 PWM）

确认 `cthrobt.ioc` 中已配置：

| 外设 | 设置 |
|---|---|
| TIM3 Channel 1 | PWM Generation CH1 |
| 引脚 | PB4（AF2，TIM3_CH1） |
| Prescaler | 95 |
| Counter Period (ARR) | 19999 |
| CH1 Pulse（初始值）| 2000 |

详细步骤见 [GRIPPER_HARDWARE_SETUP.md](GRIPPER_HARDWARE_SETUP.md)。

---

## 5. 修改编译配置

编辑 `CMakeLists.txt`，删除以下四个仿真专用宏：

```cmake
# 删除这四行：
SKIP_USB_INIT
SKIP_ETH_INTERRUPT
SKIP_PHY_INIT
SIMULATION_MODE
```

删除后重新编译：

```bash
bash scripts/build.sh
```

---

## 6. 烧录固件

使用 STM32CubeProgrammer（命令行）：

```bash
STM32_Programmer_CLI -c port=SWD -w build/Debug/cthrobt.bin 0x08000000 -v -rst
```

或直接用 STM32CubeIDE 点击 Run 烧录。

---

## 7. 查看调试输出

Nucleo 的 ST-Link VCP（USB 虚拟串口）连接到 USART3（PD8 TX / PD9 RX），波特率 115200。

连接 USB 后打开串口监视器（如 minicom、screen 或 STM32CubeIDE 终端）：

```bash
# macOS
screen /dev/tty.usbmodem* 115200

# Linux
screen /dev/ttyACM0 115200
```

正常启动时应看到：

```
[GRIPPER] init — state: OPEN
[CTRL] Task started
[CTRL] LwIP stack ready
[CTRL] Listening on port 7777
```

---

## 8. 运行手柄控制器

修改 `controller.py` 的配置文件，将目标地址改为板子 IP：

```ini
# /Users/liutianxi/ltxcth/cthrobt/config.ini
[network]
host = 192.168.0.20
port = 7777
```

然后运行：

```bash
cd /Users/liutianxi/ltxcth/cthrobt
python3 controller.py
```

---

## 9. 验证

| 操作 | 预期 |
|---|---|
| 按手柄 A | 串口打印 `[GRIPPER] CLOSE`，舵机转到关闭位置（1ms 脉冲） |
| 按手柄 B | 串口打印 `[GRIPPER] OPEN`，舵机转到打开位置（2ms 脉冲） |
| 持续按住 | 不重复触发（边沿检测） |

---

## 10. 引脚汇总

| 功能 | 引脚 | 连接器位置 |
|---|---|---|
| 舵机 PWM 信号 | PB4 | CN9 第 6 脚 |
| 以太网 | RMII（PA1/PA2/PC1/PC4/PC5/PB11/PB13） | CN6 RJ45 |
| 调试串口 TX | PD8 | ST-Link VCP（USB） |
| 调试串口 RX | PD9 | ST-Link VCP（USB） |
| 供电 / 烧录 | — | CN1 Mini-B USB |
