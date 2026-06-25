//
// Created by tomoya on 26/04/22.
//

#ifndef MODULARCANLIB_RTOS_H
#define MODULARCANLIB_RTOS_H

#include "cmsis_os.h"
#include <stdbool.h>

#define MODULARCANLIB_TASK_CAN_RX_SIGNAL 0x01
extern osThreadId modularCANLibHandle;

extern bool modularCANLib_RTOS_Initialized;

/**
 * @brief CAN通信用RTOSリソースの初期化
 *
 * CAN受信タスクなどで使用するセマフォ（canRxSem）を静的に生成・初期化します。
 * この関数は、OSのスケジューラが開始される前、または通信を開始する前に
 * 一度だけ呼び出す必要があります。
 * * @note  静的メモリ確保（Static Allocation）を使用しているため、
 * 制御ブロック（Control Block）が事前に定義されている必要があります。
 *
 * @param  None
 * @retval None
 */
void ModularCANLib_RTOS_Init(void);

#endif //MODULARCANLIB_RTOS_H
