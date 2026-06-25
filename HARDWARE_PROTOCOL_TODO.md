# 硬件协议待确认项

仿真模式下所有逻辑已完备。以下步骤完成后即可切换到真实硬件。

---

## 爪子（气动阀）— ModularCANLib 接入

协议已确认（Node ID=0，PORT_1），代码逻辑已写完。
**剩余工作是把库文件加进项目，加4个中断回调。**

### 步骤 1：把 ModularCANLib 拷到项目里

```bash
# 在 cthrobt_stm 根目录执行
cp -r ~/ModularCANLib/Inc ./Middlewares/ModularCANLib/Inc
cp -r ~/ModularCANLib/Src ./Middlewares/ModularCANLib/Src
```

### 步骤 2：CMakeLists.txt 加源文件和头文件路径

在 `CMakeLists.txt` 的 `target_sources(...)` 里加：

```cmake
Middlewares/ModularCANLib/Src/ModularCANLib.c
Middlewares/ModularCANLib/Src/ModularCANLib_DeviceInfo_Def.c
Middlewares/ModularCANLib/Src/ModularCANLib_DeviceInfo_Lookup.c
Middlewares/ModularCANLib/Src/ModularCANLib_Sys.c
Middlewares/ModularCANLib/Src/ModularCANLib_RTOS.c
Middlewares/ModularCANLib/Src/ModularCANLib_Pid.c
Middlewares/ModularCANLib/Src/ModularCANLib_NVIC.c
Middlewares/ModularCANLib/Src/CANBoards/ModularCANLib_CANBoards.c
Middlewares/ModularCANLib/Src/CANBoards/ModularCANLib_CANBoards_Sys.c
```

在 `target_include_directories(...)` 里加：

```cmake
Middlewares/ModularCANLib/Inc
Middlewares/ModularCANLib/Inc/CANBoards
Middlewares/ModularCANLib/Inc/RoboMas
Middlewares/ModularCANLib/Inc/RobStride
Middlewares/ModularCANLib/Inc/ODrive
```

### 步骤 3：在 gripper.c 里改 include 路径

`gripper.c` 第 28 行改为：

```c
#include "ModularCANLib.h"   /* 已正确（上面 include 路径加进去后自动找到）*/
```

### 步骤 4：在 `Core/Src/stm32f7xx_it.c` 加 4 个 CAN 中断回调

ModularCANLib 使用中断驱动 CAN，必须加这 4 个回调才能收发。
在文件末尾的 `/* USER CODE BEGIN 1 */` 里加：

```c
#ifndef SIMULATION_MODE
#include "ModularCANLib_Sys.h"

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
    ModularCANLib_Sys_MsgPendingCallback(hcan, CAN_RX_FIFO0);
}
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan) {
    ModularCANLib_Sys_MsgPendingCallback(hcan, CAN_RX_FIFO1);
}
void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan) {
    ModularCANLib_Sys_TryTransmit(hcan);
}
void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan) {
    ModularCANLib_Sys_TryTransmit(hcan);
}
#endif
```

### 步骤 5：删掉 can_bus_init() 调用

`Core/Src/main.c` 里的 `can_bus_init()` 调用**必须删掉**。
`ModularCANLib_AllDevice_And_CANSystem_Init()` 已经做了 CAN1 的全部初始化（滤波器、HAL_CAN_Start、中断使能），两者同时调用会冲突。

> `can_bus.c` / `can_bus.h` 文件可以暂时保留（腕部 RDS3288 还在用），
> 但 `can_bus_init()` 不能再被调用。

### 步骤 6：删掉 SIMULATION_MODE 宏，烧录

```cmake
# CMakeLists.txt — 删掉这 4 行
# SIMULATION_MODE
# SKIP_USB_INIT
# SKIP_ETH_INTERRUPT
# SKIP_PHY_INIT
```

```bash
STM32_Programmer_CLI -c port=SWD -w build/Debug/cthrobt.bin 0x08000000 -rst
```

### 还剩一个 TODO

- **AIR_ON/AIR_OFF 极性**：当前 `AIR_ON = 爪子闭合`。
  接上硬件后如果方向反了，改 `gripper_close()` / `gripper_open()` 里的 `AIR_ON` ↔ `AIR_OFF`。

---

## 腕部（RDS3288）— 协议未知

| 常量 | 当前占位值 | 需要确认 |
|---|---|---|
| `WRIST_NODE_ID` | `0x02` | RDS3288 驱动板的 CAN Node ID |
| `WRIST_FRAME_ID` | `0x600 + Node ID` | 实际帧 ID |
| `WRIST_CMD_CW90[]` | `{ 0x00, 0x00 }` | 命令电机转到顺时针 90° 的数据域 |
| `WRIST_CMD_HOME[]` | `{ 0x00, 0x00 }` | 命令电机回到 0° 的数据域 |
| `WRIST_CMD_LEN` | `2` | 数据域字节数 |

需要向硬件同学确认：
1. Node ID？
2. 帧 ID 和数据域格式？
3. RDS3288 是否也用 ModularCANLib？（如果是，可以改成跟爪子一样的写法，`can_bus.c` 可以彻底删掉）

---

## 通用 — CAN 引脚确认

| 当前配置 | 需要确认 |
|---|---|
| PD0 = CAN1_RX, PD1 = CAN1_TX (AF9) | 实际接线是否一致；如接 PB8/PB9 则改 `stm32f7xx_hal_msp.c` |
| 500 kbps (Prescaler=6) | CAN 总线波特率是否一致 |
