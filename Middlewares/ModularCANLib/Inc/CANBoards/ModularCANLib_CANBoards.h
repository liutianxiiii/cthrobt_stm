/*
 * CAN_Main.h
 *
 *  Created on: Feb 6, 2020
 *      Author: Eater
 */

//reference https://hsdev.co.jp/stm32-can/

#ifndef _CAN_MAIN_H_
#define _CAN_MAIN_H_

#include "ModularCANLib_CANBoards_AirCylinder_Def.h"
#include "ModularCANLib_CANBoards_System_Def.h"
#include "ModularCANLib_CANBoards_MCMD_Def.h"
#include "ModularCANLib_CANBoards_Servo_Def.h"
#include "ModularCANLib_CANBoards_Dynamixel_Kondo_Def.h"
#include "ModularCANLib_Sys.h"
#include "can.h"

#define AWAKE_CMD (0)
#define FB_CMD (1)


typedef struct{
    uint8_t servo;
    uint8_t air;
    uint8_t mcmd4;
    uint8_t dynamixel_kondo;
} _ModularCANLib_CANBoards_NUM_OF_DEVICES;


/* ============================================================
 *  MCMD (モータコントローラ) 関連
 * ============================================================ */

/**
 * @brief MCMDの制御パラメータを変更する.
 *
 * PIDゲイン (kp, ki, kd, kff), 加速度制限, 重力補償ゲイン,
 * 制御タイプ, フィードバック種別などをCAN経由で送信する.
 * 内部的に CTRL1〜CTRL4 の4フレームに分割して送信する.
 *
 * @param device_info 対象デバイスの情報 (CANハンドル・パラメータを含む)
 */
void MCMD_ChangeControl(const ModularCANLib_DeviceInfo_Type *device_info);

/**
 * @brief MCMDを初期化する.
 *
 * エンコーダ方向, 回転方向, キャリブレーション設定, リミットスイッチ種別,
 * オフセット, キャリブレーションデューティ, 単位あたりのパルス数などを送信し,
 * 最後に MCMD_ChangeControl() を呼んで制御パラメータを適用する.
 *
 * @note 送信後に 50ms の待機が必要 (省略すると動作しない場合がある).
 *
 * @param device_info 対象デバイスの情報
 */
void MCMD_init(const ModularCANLib_DeviceInfo_Type *device_info);

/**
 * @brief MCMDにキャリブレーション実行を指示する.
 *
 * MCMD_CMD_CALIB コマンドをCAN送信する.
 * キャリブレーション完了を待つ場合は MCMD_Wait_For_Calib() を併用すること.
 *
 * @param device_info 対象デバイスの情報
 */
void MCMD_Calib(const ModularCANLib_DeviceInfo_Type *device_info);

/**
 * @brief MCMDのキャリブレーション完了をポーリング待機する.
 *
 * フィードバックの end_calib フラグが 1 になるまで 10ms ごとにポーリングする.
 * 完了後に "Calibration End" をシリアル出力する.
 *
 * @param device_info 対象デバイスの情報
 */
void MCMD_Wait_For_Calib(const ModularCANLib_DeviceInfo_Type *device_info);

/**
 * @brief MCMDの制御を有効化する.
 *
 * MCMD_CMD_ENABLE コマンドをCAN送信する.
 *
 * @param device_info 対象デバイスの情報
 */
void MCMD_Control_Enable(const ModularCANLib_DeviceInfo_Type *device_info);

/**
 * @brief MCMDの制御を無効化する.
 *
 * MCMD_CMD_DISABLE コマンドをCAN送信する.
 *
 * @param device_info 対象デバイスの情報
 */
void MCMD_Control_Disable(const ModularCANLib_DeviceInfo_Type *device_info);

/**
 * @brief MCMDに目標値を送信する.
 *
 * 制御タイプに応じて, 目標位置・速度・トルクなどを float 値で送信する.
 *
 * @param device_info 対象デバイスの情報
 * @param target      目標値 (単位は制御タイプ依存)
 */
void MCMD_SetTarget(const ModularCANLib_DeviceInfo_Type *device_info, float target);

/**
 * @brief MCMDのフィードバックデータを取得する.
 *
 * device_type が MCMD4 でない場合はエラーメッセージを出力し,
 * ゼロ初期化された構造体を返す.
 *
 * @param device_info 対象デバイスの情報
 * @return ModularCANLib_MCMD_Feedback_Type 最新のフィードバックデータ
 */
ModularCANLib_MCMD_Feedback_Type Get_MCMD_Feedback(const ModularCANLib_DeviceInfo_Type *device_info);


/* ============================================================
 *  Servo (サーボドライバ) 関連
 * ============================================================ */

/**
 * @brief サーボドライバを初期化する.
 *
 * パルス幅の最小・最大値, PWM周波数, 角度レンジ, 角度オフセットを
 * INIT1〜INIT3 の3フレームに分割してCAN送信する.
 *
 * @param device_info 対象デバイスの情報
 */
void ServoDriver_Init(const ModularCANLib_DeviceInfo_Type *device_info);

/**
 * @brief サーボドライバに目標角度を送信する.
 *
 * 送信に失敗した場合は Error_Handler() を呼び出す.
 *
 * @param device_info 対象デバイスの情報
 * @param angle       目標角度 [deg]
 */
void ServoDriver_SendValue(const ModularCANLib_DeviceInfo_Type *device_info, float angle);


/* ============================================================
 *  AirCylinder (空気圧シリンダ) 関連
 * ============================================================ */

/**
 * @brief 空気圧シリンダを初期化する.
 *
 * AIR_CMD_INIT コマンドと初期ポート状態を送信する.
 * 機構の初期状態 (ON / OFF) を明示的に指定する必要があるため,
 * AirCylinder_SendOutput() とは別関数として定義されている.
 * 送信に失敗した場合は Error_Handler() を呼び出す.
 *
 * @param device_info 対象デバイスの情報
 * @param param       初期ポート状態 (ON / OFF)
 */
void AirCylinder_Init(const ModularCANLib_DeviceInfo_Type *device_info,
                      ModularCANLib_Air_PortStatus_Typedef param);

/**
 * @brief 空気圧シリンダのポート出力を更新する.
 *
 * AIR_CMD_OUTPUT コマンドと目標ポート状態を送信する.
 * 送信に失敗した場合は Error_Handler() を呼び出す.
 *
 * @param device_info 対象デバイスの情報
 * @param param       目標ポート状態 (ON / OFF)
 */
void AirCylinder_SendOutput(const ModularCANLib_DeviceInfo_Type *device_info,
                            ModularCANLib_Air_PortStatus_Typedef param);


/* ============================================================
 *  Dynamixel_Kondo 関連
 * ============================================================ */

/**
 * @brief Dynamixelの制御パラメータを変更する.
 *
 * 速度PIDゲイン (ki, kp), 位置PIDゲイン (kd, ki, kp),
 * 制御タイプ, フィードバック種別, プロファイル加速度・速度を
 * CTRL1〜CTRL5 の5フレームに分割してCAN送信する.
 *
 * @param device_info 対象デバイスの情報
 */
void Dynamixel_ChangeControl(const ModularCANLib_DeviceInfo_Type *device_info);

/**
 * @brief Dynamixelを初期化する.
 *
 * モデル, ID, 回転方向, 1度あたりのパルス数を送信し,
 * 最後に Dynamixel_ChangeControl() を呼んで制御パラメータを適用する.
 *
 * @note 送信後に 50ms の待機が必要 (MCMD_init() からの継承, 詳細不明).
 *
 * @param device_info 対象デバイスの情報
 */
void Dynamixel_init(const ModularCANLib_DeviceInfo_Type *device_info);

/**
 * @brief Dynamixelの制御を有効化する.
 *
 * DYNAMIXEL_KONDO_CMD_ENABLE コマンドをCAN送信する.
 *
 * @param device_info 対象デバイスの情報
 */
void Dynamixel_Control_Enable(const ModularCANLib_DeviceInfo_Type *device_info);

/**
 * @brief Dynamixelの制御を無効化する.
 *
 * DYNAMIXEL_KONDO_CMD_DISABLE コマンドをCAN送信する.
 *
 * @param device_info 対象デバイスの情報
 */
void Dynamixel_Control_Disable(const ModularCANLib_DeviceInfo_Type *device_info);

/**
 * @brief Dynamixelに目標値を送信する.
 *
 * 制御タイプに応じて, 目標位置・速度などを float 値で送信する.
 *
 * @param device_info 対象デバイスの情報
 * @param target      目標値 (単位は制御タイプ依存)
 */
void Dynamixel_SetTarget(const ModularCANLib_DeviceInfo_Type *device_info, float target);

/**
 * @brief Dynamixelのフィードバックデータを取得する.
 *
 * device_type が DYNAMIXEL_KONDO でない場合はエラーメッセージを出力し,
 * ゼロ初期化された構造体を返す.
 *
 * @param device_info 対象デバイスの情報
 * @return ModularCANLib_CANDynamixel_Kondo_Feedback_Type 最新のフィードバックデータ
 */
ModularCANLib_CANDynamixel_Kondo_Feedback_Type Get_Dynamixel_Feedback(const ModularCANLib_DeviceInfo_Type *device_info);

/**
 * @brief Kondoの制御パラメータを変更する.
 *
 * 制御タイプ, フィードバック種別, プロファイルの種類を
 * CTRL1〜CTRL5 の5フレームに分割してCAN送信する.
 *
 * @param device_info 対象デバイスの情報
 */
void Kondo_ChangeControl(const ModularCANLib_DeviceInfo_Type *device_info);

/**
 * @brief Kondoを初期化する.
 *
 * ID, 1度あたりのパルス数を送信し,
 * 最後に Kondo_ChangeControl() を呼んで制御パラメータを適用する.
 *
 * @note 送信後に 50ms の待機が必要 (MCMD_init() からの継承, 詳細不明).
 *
 * @param device_info 対象デバイスの情報
 */
void Kondo_init(const ModularCANLib_DeviceInfo_Type *device_info);

/**
 * @brief Kondoの制御を有効化する.
 *
 * DYNAMIXEL_KONDO_CMD_ENABLE コマンドをCAN送信する.
 *
 * @param device_info 対象デバイスの情報
 */
void Kondo_Control_Enable(const ModularCANLib_DeviceInfo_Type *device_info);

/**
 * @brief Kondoの制御を無効化する.
 *
 * DYNAMIXEL_KONDO_CMD_DISABLE コマンドをCAN送信する.
 *
 * @param device_info 対象デバイスの情報
 */
void Kondo_Control_Disable(const ModularCANLib_DeviceInfo_Type *device_info);

/**
 * @brief Kondoに目標値を送信する.
 *
 * 制御タイプに応じて, 目標位置・速度などを float 値で送信する.
 *
 * @param device_info 対象デバイスの情報
 * @param target      目標値 (単位は制御タイプ依存)
 */
void Kondo_SetTarget(const ModularCANLib_DeviceInfo_Type *device_info, float target);

/**
 * @brief Kondoに目標値とプロファイル用の目標時間を送信する.
 *
 * 制御タイプに応じて, 目標位置・速度などとプロファイル用の目標時間を float 値で送信する.
 *
 * @param device_info 対象デバイスの情報
 * @param target      目標値 (単位は制御タイプ依存)
 * @param time_sec    プロファイル用の目標時間 (秒単位)
 */
void Kondo_SetTarget_With_Time(const ModularCANLib_DeviceInfo_Type *device_info, float target, float time_sec);

/**
 * @brief Kondoのフィードバックデータを取得する.
 *
 * device_type が DYNAMIXEL_KONDO でない場合はエラーメッセージを出力し,
 * ゼロ初期化された構造体を返す.
 *
 * @param device_info 対象デバイスの情報
 * @return ModularCANLib_CANDynamixel_Kondo_Feedback_Type 最新のフィードバックデータ
 */
ModularCANLib_CANDynamixel_Kondo_Feedback_Type Get_Kondo_Feedback(const ModularCANLib_DeviceInfo_Type *device_info);


/* ============================================================
 *  共通
 * ============================================================ */

bool ModularCANLib_CANBoards_IsCANBoardsID_RX(ModularCANLib_CANId_Type can_id_type, uint32_t can_id);

void ModularCANLib_CANBoards_ProcessRxData(const CAN_HandleTypeDef* hcan, const CAN_RxMsg* rx_msg);

void ModularCANLib_CANBoards_MCMD_Device_Init(ModularCANLib_DeviceInfo_Type *device_info);

void ModularCANLib_CANBoards_Air_Device_Init(ModularCANLib_DeviceInfo_Type *device_info);

void ModularCANLib_CANBoards_Servo_Device_Init(ModularCANLib_DeviceInfo_Type *device_info) ;

void ModularCANLib_CANBoards_Dynamixel_Kondo_Device_Init(ModularCANLib_DeviceInfo_Type *device_info);

void ModularCANLib_CANBoards_AfterDeviceInit(void);

bool ModularCANLib_CANBoards_MCMD_IsDeviceConnected(const ModularCANLib_DeviceInfo_Type *device_info);

bool ModularCANLib_CANBoards_Air_IsDeviceConnected(const ModularCANLib_DeviceInfo_Type *device_info);

bool ModularCANLib_CANBoards_Servo_IsDeviceConnected(const ModularCANLib_DeviceInfo_Type *device_info);

bool ModularCANLib_CANBoards_Dynamixel_Kondo_IsDeviceConnected(const ModularCANLib_DeviceInfo_Type *device_info);

bool ModularCANLib_CANBoards_AllDevice_Connected(void);

void ModularCANLib_CANBoards_PrintDetection(void);

#endif /* _CAN_MAIN_H_ */
