//
// Created by tomoya on 2026/05/09.
//

#ifndef ODRIVE_H
#define ODRIVE_H

#include <stdbool.h>
#include "ModularCANLib_ODrive_DeviceParam.h"
#include "ModularCANLib_Def.h"
#include "ModularCANLib_DeviceInfo_Def.h"
#include "ModularCANLib_Sys.h"

/* ========================================================
 *   ユーザー向けAPI
 *   概念レベルで canlib_odrive の関数群を継承する
 * ======================================================== */

/**
 * @brief ODriveデバイスを初期化します。
 * ホールセンサ未使用時はエンコーダオフセットキャリブレーションを実行し（osDelay使用）、
 * その後ユーザーが設定した制御モードとゲインをODriveに送信します。
 * WaitForConnect 完了後に呼び出してください。
 * @param device_info デバイス情報の構造体ポインタ
 */
void ODrive_Init(ModularCANLib_DeviceInfo_Type *device_info);

/**
 * @brief リミットスイッチを使ったキャリブレーション（原点出し）を実行します。
 * use_internal_offset が ODRIVE_USE_OFFSET_POS_CALIB の場合のみ動作します。
 * スイッチが反応するまで指定速度で動かし続けます（osDelay使用）。
 * @param device_info デバイス情報の構造体ポインタ
 * @param calib_velocity キャリブレーション時の速度 [ユーザー単位/s]
 * @param sw_type リミットスイッチのNO/NC
 * @param limit_port リミットスイッチのGPIOポート
 * @param limit_pin リミットスイッチのGPIOピン
 */
void ODrive_Calibration(ModularCANLib_DeviceInfo_Type *device_info, float calib_velocity,
                        ModularCANLib_ODrive_SwitchType sw_type,
                        GPIO_TypeDef *limit_port, uint32_t limit_pin);

/**
 * @brief 制御モード（位置/速度/トルク）を変更します。
 * 変更時にゲイン・リミットをODriveに送信します。
 * @param device_info デバイス情報の構造体ポインタ
 * @param new_ctrl_type 設定する制御モード
 */
void ODrive_ChangeControl(ModularCANLib_DeviceInfo_Type *device_info,
                          ModularCANLib_ODrive_CtrlType new_ctrl_type);

/**
 * @brief 目標値を設定してCANで即時送信します。
 * 制御モードに応じて位置[X]、速度[X/s]、トルク[N m]として解釈されます。
 * _enable_flag が立っていない場合は無視されます。
 * @param device_info デバイス情報の構造体ポインタ
 * @param target_value 目標値
 */
void ODrive_SetTarget(ModularCANLib_DeviceInfo_Type *device_info, float target_value);

/**
 * @brief モーター制御を有効にします（Closed Loop Controlに移行）。
 * @param device_info デバイス情報の構造体ポインタ
 */
void ODrive_ControlEnable(ModularCANLib_DeviceInfo_Type *device_info);

/**
 * @brief モーター制御を無効にします（IDLEに移行）。
 * @param device_info デバイス情報の構造体ポインタ
 */
void ODrive_ControlDisable(ModularCANLib_DeviceInfo_Type *device_info);

/* ========================================================
 *   Modular CANLib Sys向けの関数たち
 * ======================================================== */

/**
 * @brief DeviceParamの内部変数を初期化します。
 * ModularCANLib_DeviceInfo_Struct_Init から呼ばれます。
 * @param device_param 初期化対象のパラメータ構造体ポインタ
 */
void ModularCANLib_ODrive_DeviceParam_Struct_Init(ModularCANLib_DeviceParam_ODrive_Type *device_param);

/**
 * @brief ODriveデバイスをルックアップテーブルに登録します。
 * CAN初期化前に実行されます。
 * @param device_info デバイス情報の構造体ポインタ
 */
void ModularCANLib_ODrive_Device_Init(ModularCANLib_DeviceInfo_Type *device_info);

/**
 * @brief CAN初期化後のセットアップ処理です（現在は空）。
 */
void ModularCANLib_ODrive_AfterDeviceInit(void);

/**
 * @brief 指定デバイスが接続済みか確認します。
 * ハートビートを十分に受信し、かつInitializingエラーが解除されていれば接続済みと判定します。
 * @param device_info デバイス情報の構造体ポインタ
 * @return true: 接続済み, false: 未接続
 */
bool ModularCANLib_ODrive_IsDeviceConnected(const ModularCANLib_DeviceInfo_Type *device_info);

/**
 * @brief 登録済みの全ODriveデバイスが接続済みか確認します。
 * @return true: 全接続済み, false: 未接続デバイスあり
 */
bool ModularCANLib_ODrive_AllDevice_Connected(void);

/**
 * @brief 受信CAN IDがODriveのフィードバックIDか判定します。
 * STD ID かつ node_id <= ODRIVE_NODE_ID_MAX かつ
 * 有効なフィードバックcmd_idの場合に true を返します。
 * @param can_id_type CAN IDタイプ（STD/EXT）
 * @param can_id CAN ID
 * @return true: ODriveフィードバックID, false: それ以外
 */
bool ModularCANLib_ODrive_IsODriveID_RX(ModularCANLib_CANId_Type can_id_type, uint32_t can_id);

/**
 * @brief 受信CANメッセージを解析し、対応するデバイスのフィードバックを更新します。
 * @param hcan 受信CANハンドル
 * @param rx_msg 受信メッセージ構造体へのポインタ
 */
void ModularCANLib_ODrive_ProcessRxData(const CAN_HandleTypeDef *hcan, const CAN_RxMsg *rx_msg);

#endif // ODRIVE_H