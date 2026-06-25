//
// Created by tomoya on 2026/04/20.
//
#include "ModularCANLib_Sys.h"

/* ===================================
 *        STM NVIC割り込み関数たち
 * ===================================*/

// Rx FIFO 0メッセージ保留コールバック
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
    ModularCANLib_Sys_MsgPendingCallback(hcan, CAN_RX_FIFO0);
}

// Rx FIFO 1メッセージ保留コールバック
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan) {
    ModularCANLib_Sys_MsgPendingCallback(hcan, CAN_RX_FIFO1);
}
// Mailbox 0 送信完了コールバック
void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan) {
    ModularCANLib_Sys_TryTransmit(hcan);
}

// Mailbox 1 送信完了コールバック
void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan) {
    ModularCANLib_Sys_TryTransmit(hcan);
}

// Mailbox 2 送信完了コールバック
void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan) {
    ModularCANLib_Sys_TryTransmit(hcan);
}

// Mailbox 0 送信アボートコールバック
void HAL_CAN_TxMailbox0AbortCallback(CAN_HandleTypeDef *hcan) {
    ModularCANLib_Sys_TryTransmit(hcan);
}

// Mailbox 1 送信アボートコールバック
void HAL_CAN_TxMailbox1AbortCallback(CAN_HandleTypeDef *hcan) {
    ModularCANLib_Sys_TryTransmit(hcan);
}

// Mailbox 2 送信アボートコールバック
void HAL_CAN_TxMailbox2AbortCallback(CAN_HandleTypeDef *hcan) {
    ModularCANLib_Sys_TryTransmit(hcan);
}

// error handler
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan) {
    // const uint32_t error_code = hcan->ErrorCode;
    // MODULARCANLIB_PRINTF_DEBUG("CAN Error Code: 0x%08lX\n", error_code);
    // ここが呼ばれる場合、ハードウェアレベルでパケットが壊れている・破棄されています
}