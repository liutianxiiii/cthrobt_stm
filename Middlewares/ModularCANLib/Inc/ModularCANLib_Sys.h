//
// Created by tomoya on 2026/04/20.
//

#ifndef MODULARCANLIB_SYS_H
#define MODULARCANLIB_SYS_H

#include "can.h"
#include "stdbool.h"

#include "ModularCANLib_Def.h"

#define CAN_TX_TIMEOUT_MS 10

#define CAN_RX_BUF_SIZE 128

// 受信データ格納用構造体
typedef struct {
    CAN_RxHeaderTypeDef Header;
    uint8_t Data[8];
} CAN_RxMsg;

// リングバッファ
// Standard IDとExtended IDでバッファを分離
typedef struct {
    CAN_RxMsg Buffer[CAN_RX_BUF_SIZE];
    volatile uint32_t Head;
    volatile uint32_t Tail;
} CAN_RxRingBuffer;

extern CAN_RxRingBuffer rx_std_ring[NUM_OF_CANS]; // Standard IDバッファ {CAN1, CAN2, ...}
extern CAN_RxRingBuffer rx_ext_ring[NUM_OF_CANS]; // Extended IDバッファ {CAN1, CAN2, ...}


#define CAN_TX_BUF_SIZE 128

// 受信データ格納用構造体
typedef struct {
    CAN_TxHeaderTypeDef Header;
    uint8_t Data[8];
} CAN_TxMsg;

// リングバッファ
// Standard IDとExtended IDでバッファを分離
typedef struct {
    CAN_TxMsg Buffer[CAN_TX_BUF_SIZE];
    volatile uint32_t Head;
    volatile uint32_t Tail;
} CAN_TxRingBuffer;

extern CAN_TxRingBuffer tx_std_ring[NUM_OF_CANS]; // Standard IDバッファ {CAN1, CAN2, ...}
extern CAN_TxRingBuffer tx_ext_ring[NUM_OF_CANS]; // Extended IDバッファ {CAN1, CAN2, ...}

//
extern CAN_HandleTypeDef *modularCANLib_UsedCANHandlers[NUM_OF_CANS];

// 公開割り込み関数
HAL_StatusTypeDef ModularCANLib_Sys_MsgPendingCallback(CAN_HandleTypeDef *hcan, uint32_t rx_fifo);
void ModularCANLib_Sys_TryTransmit(CAN_HandleTypeDef* hcan);

// 公開関数
/**
 * @brief  使用中の全CANバスに対するフィルタバンク一括初期化
 *
 * @details 内部で管理されている `ModularCANLib_UsedCANHandlers` 配列を走査し、
 * 有効な（NULLではない）CANハンドルが登録されているすべてのCANチャネルに対して
 * 受信フィルタの設定（ModularCANLib_FilterBank_Init_OneCAN）を実行します。
 * 特定のCANで初期化に失敗した場合は、エラーハンドラを呼び出し、戻り値にエラー状態を保持します。
 *
 * @note    この関数を呼び出す前に `ModularCANLib_AllDevice_Init` 等を実行し、
 * 使用するCANハンドルが配列に登録されている必要があります。
 *
 * @return  HAL_StatusTypeDef  すべての初期化が成功した場合は HAL_OK。
 * 一つ以上のチャネルで失敗した場合は、HAL_ERROR もしくは
 * 失敗した処理のステータスを返します。
 */
HAL_StatusTypeDef ModularCANLib_Sys_FilterBank_Init();

/**
 * @brief  全使用チャネルのCAN送受信割り込み一括有効化
 *
 * @details ライブラリ内で管理されている有効なすべてのCANハンドルに対し、
 * 以下の割り込み通知を有効化します：
 * - CAN_IT_RX_FIFO0_MSG_PENDING: FIFO 0 受信保留割り込み
 * - CAN_IT_RX_FIFO1_MSG_PENDING: FIFO 1 受信保留割り込み
 * - CAN_IT_TX_MAILBOX_EMPTY: 送信メールボックス空き割り込み
 *
 * これにより、データ受信時の自動解析（ProcessRxData）および
 * 送信完了/中断時の連続送信処理（TryTransmit）がコールバック経由で動作可能になります。
 *
 * @return  HAL_StatusTypeDef  すべてのチャネルで有効化に成功すれば HAL_OK。
 * 一つ以上のチャネルで失敗した場合は HAL_ERROR を返します。
 */
HAL_StatusTypeDef ModularCANLib_Sys_ActivateNotification();

/**
 * @brief  CANハンドルから対応するインデックス(0-2)を取得します。
 * @details CAN1なら0、CAN2なら1、CAN3なら2を返します。
 * このインデックスは、ライブラリ内部の管理用配列の添え字として使用されます。
 *
 * @param  hcan: STM32 HAL CANハンドルのポインタ
 * @retval 変換後のインデックス (0, 1, or 2)。
 * 未知のインスタンスの場合はデバッグメッセージを出力し、デフォルトで0を返します。
 */
uint8_t ModularCANLib_hcan2idx(const CAN_HandleTypeDef* hcan);

/**
 * @brief  CANハンドルがModularCANLibの制御対象か判定します。
 * @details CANxのハンドルhcanxからModularCANLibが制御しているDeviceがそのバスに存在するか判定します。
 * 存在する場合にはtrue, しない場合にはfalseが返されます。
 *
 * @param  hcan: STM32 HAL CANハンドルのポインタ
 * @retval ModularCANLibの制御対象かどうか (true: yes | false: no)
 * 未知のインスタンスの場合はデバッグメッセージを出力し、デフォルトでfalseを返します。
 */
bool ModularCANLib_hcanIsUnderControl(const CAN_HandleTypeDef* hcan);

/**
  * @brief  ソフトウェアリングバッファから受信メッセージを1つ読み出す
  * @param  hcan: CANハンドラへのポインタ
  * @param  can_id_t: 読み出し対象のIDタイプ (Standard または Extended)
  * @param  pDest: 読み出したメッセージを格納する構造体へのポインタ
  * @retval HAL_StatusTypeDef: HAL_OK (成功), HAL_ERROR (バッファが空)
  */
HAL_StatusTypeDef ModularCANLib_Sys_ReadFromBuffer(const CAN_HandleTypeDef* hcan, ModularCANLib_CANId_Type can_id_t, CAN_RxMsg *pDest);

/**
 * @brief  CANの送信Mailboxを監視し、タイムアウトしたものをアボートする
 * （メインループ等で定期的に呼び出す）
 */
void ModularCANLib_Sys_Check_Tx_Timeout(void);

/**
 * @brief  任意のバイト配列を分割・送信キューへ追加する
 * @param  hcan: CANハンドラへのポインタ
 * @param  id: CAN ID
 * @param  id_type: CAN IDのタイプ (Standard / Extended)
 * @param  bytes: 送信するデータ配列のポインタ
 * @param  size: 送信するデータサイズ
 * @retval HAL_StatusTypeDef: 処理結果
 */
HAL_StatusTypeDef ModularCANLib_Sys_SendBytes(CAN_HandleTypeDef* hcan, uint32_t id, ModularCANLib_CANId_Type id_type, const uint8_t *bytes, uint32_t size);

void ModularCANLib_Sys_DebugPrint();

#endif //MODULARCANLIB_SYS_H
