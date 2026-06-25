//
// Created by tomoya on 2026/04/20.
//

#ifndef MODULARCANLIB_H
#define MODULARCANLIB_H

#include "ModularCANLib_Def.h"
#include "ModularCANLib_DeviceInfo_Def.h"
#include "ModularCANLib_Pid.h"
#include "ModularCANLib_Sys.h"
#include "ModularCANLib_DeviceInfo_Lookup.h"

// 各種
#include "RoboMas/ModularCANLib_RoboMas.h"
#include "RoboMas/ModularCANLib_RoboMas_DeviceParam.h"
#include "RobStride/ModularCANLib_RobStride.h"
#include "RobStride/ModularCANLib_RobStride_DeviceParam.h"
#include "CANBoards/ModularCANLib_CANBoards_DeviceParam.h"
#include "CANBoards/ModularCANLib_CANBoards.h"
#include "ODrive/ModularCANLib_ODrive.h"

/**
 * @brief CAN受信データのバッチ処理
 *
 * 登録されているすべてのCANハンドラ（バス）を走査し、内部バッファに保存されている
 * 未処理の受信メッセージをすべて読み出して処理します。
 *
 * - 標準ID（Standard ID）と拡張ID（Extended ID）の両方のバッファを処理します。
 * - 各メッセージに対し `ModularCANLib_ProcessRXData_Once` を呼び出し、デバイスごとの解析を実行します。
 * - 通常、メインループ内または高頻度のタイマー割り込み内で呼び出すことを想定しています。
 *
 * @note
 * `ModularCANLib_UsedCANHandlers` に登録されていないバスはスキップされます。
 */
void ModularCANLib_ProcessRXData(void);

/**
 * @brief  デバイス情報構造体のインスタンス確保と初期化
 *
 * @details ライブラリが保持する静的配列 `modularCANLib_DeviceInfos` から新しい要素を確保し、
 * 指定されたデバイスタイプに応じた初期値を設定します。
 * 最大デバイス数（MODULARCANLIB_MAX_DEVICES）を超えて確保しようとした場合はエラーを返します。
 *
 * @param[in]  device_type  確保するデバイスの種別（RoboMas_C610, C620 等）
 * @param[in]  name         デバイスに付与する名前（0始まりのenum値、最大MODULARCANLIB_MAX_DEVICES-1）
 *
 * @return ModularCANLib_DeviceInfo_Type* 確保された構造体へのポインタ。
 * 容量不足または不明なタイプの場合は NULL を返し、Error_Handler() を呼び出します。
 */
ModularCANLib_DeviceInfo_Type * ModularCANLib_DeviceInfo_Struct_Init(ModularCANLib_DeviceType_Type device_type, uint32_t name);

/**
 * @brief  全デバイスおよびCANシステムの一括完全初期化
 *
 * @details 本ライブラリにおける初期化シーケンスの最上位関数です。以下の処理を順次実行します：
 * 1. 登録済み全デバイスの走査とCANハンドルの紐付け。
 * 2. 各デバイス種別に応じた固有の初期化（RoboMaster等）の実行。
 * 3. 受信メッセージ高速検索用ルックアップテーブルの構築。
 * 4. 使用する全CANチャネルに対するフィルタバンクの設定。
 * 5. 全CANチャネルの送受信割り込み（TX/RX FIFO0,1）の有効化。
 *
 * @note    この関数を呼び出す前に、必要なすべてのデバイスが
 * `ModularCANLib_DeviceInfo_Struct_Init` によって登録済みである必要があります。
 *
 * @return void
 */
void ModularCANLib_AllDevice_And_CANSystem_Init();

/**
 * @brief 全デバイスの接続待機
 *
 * 登録されているすべてのデバイスが
 * 正常に検出されるまでループして待機します。
 */
void ModularCANLib_WaitForConnect();

#endif //MODULARCANLIB_H
