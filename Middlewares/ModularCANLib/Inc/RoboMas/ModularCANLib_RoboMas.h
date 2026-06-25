//
// Created by tomoya on 2026/04/21.
//

#ifndef MODULARCANLIB_ROBOMAS_DEF_H
#define MODULARCANLIB_ROBOMAS_DEF_H

#include "ModularCANLib_RoboMas_DeviceParam.h"
#include "ModularCANLib_Def.h"
#include "ModularCANLib_DeviceInfo_Def.h"
#include "ModularCANLib_Sys.h"

/**
 * @brief キャリブレーションを手動で終了させます。
 * 内部オフセットを確定させ、元の制御モードに復帰します。
 * @param device_info デバイス情報の構造体ポインタ
 */
void RoboMas_FinishCalib(ModularCANLib_DeviceInfo_Type *device_info);

/**
 * @brief 全てのCANバスに対して、計算された制御リクエスト（電流値）を送信します。
 * 内部でPID制御の更新、リミット処理、キャリブレーション終了判定、CANフレームの構築を行います。
 * @param update_freq_hz 制御ループの更新周波数 [Hz]
 */
void ModularCANLib_RoboMas_SendRequest(float update_freq_hz);

/**
 * @brief キャリブレーション（原点出し）を開始します。
 * 制御モードを一時的に速度制御に変更し、スイッチが反応するまで指定速度で動かします。
 * @param device_info デバイス情報の構造体ポインタ
 * @param calib_vel キャリブレーション時の移動速度
 * @param sw_type スイッチの接点タイプ (NO/NC)
 * @param limit_port リミットスイッチが接続されているGPIOポート
 * @param limit_pin リミットスイッチが接続されているGPIOピン
 */
void RoboMas_Calibration(ModularCANLib_DeviceInfo_Type *device_info, float calib_vel, ModularCANLib_DeviceParam_RoboMas_Switch_Type sw_type, GPIO_TypeDef* limit_port, uint16_t limit_pin);

/**
 * @brief 制御モード（位置/速度/電流）を変更します。
 * 変更時にPIDの内部状態などはリセットされます。
 * @param device_info デバイス情報の構造体ポインタ
 * @param new_ctrl_type 設定する制御モード
 */
void RoboMas_ChangeControl(ModularCANLib_DeviceInfo_Type *device_info, ModularCANLib_DeviceParam_RoboMas_Ctrl_Type new_ctrl_type);

/**
 * @brief 目標値を設定します。
 * 制御モードに応じて、位置[deg]、速度[rpm]、電流[A]として解釈されます。
 * @param device_info デバイス情報の構造体ポインタ
 * @param target_value 目標値
 */
void RoboMas_SetTarget(ModularCANLib_DeviceInfo_Type *device_info, float target_value);

/**
 * @brief 台形速度プロファイルで目標位置まで移動します（位置制御モード専用）。
 *
 * 加速・定速・減速の3フェーズで位置目標値を補間しながら追従させます。
 * pid_pos.kff に瞬時目標速度を直接設定し、
 * pid_vel.kff に current_ff_gain × 瞬時目標加速度 を設定します。
 * 単位換算は呼び出し前にユーザー側で行ってください。
 * 関数終了後は両 kff を 0 に戻します。
 * 制御モードが ROBOMAS_CTRL_POS でない場合は警告を出力して即座に返ります。
 *
 * @note 本関数はブロッキングです。移動完了まで呼び出し元に制御が戻りません。
 *
 * @param device_info     デバイス情報の構造体ポインタ
 * @param target_pos      目標位置
 * @param max_speed       最大速度（正の値）
 * @param acc             加速度（正の値）
 * @param current_ff_gain 電流フィードフォワードゲイン（加速度 → 電流の換算係数）
 */
void RoboMas_MoveWithRamp(ModularCANLib_DeviceInfo_Type *device_info, float target_pos, float max_speed, float acc, float current_ff_gain);

/**
 * @brief モーターの制御を有効にします（CAN送信フラグを立てる）。
 * @param device_info デバイス情報の構造体ポインタ
 */
void RoboMas_ControlEnable(ModularCANLib_DeviceInfo_Type *device_info);

/**
 * @brief モーターの制御を無効にします。
 * @param device_info デバイス情報の構造体ポインタ
 */
void RoboMas_ControlDisable(ModularCANLib_DeviceInfo_Type *device_info);

/**
 * @brief キャリブレーションが終了しているか確認します。
 * @param device_info デバイス情報の構造体ポインタ
 * @return true: 終了（通常動作中）, false: キャリブレーション中
 */
bool RoboMas_IsCalibrationEnded(const ModularCANLib_DeviceInfo_Type *device_info);

/**
 * @brief キャリブレーションが完了するまでブロッキング（待機）します。
 * @param device_info デバイス情報の構造体ポインタ
 */
void RoboMas_Wait_For_Calib(const ModularCANLib_DeviceInfo_Type *device_info);

/**
 * @brief RoboMasterデバイスパラメータ構造体の初期値を設定します。
 * ターゲット値のリセット、フラグの初期化、およびPIDコントローラの初期化を行います。
 * @param device_param 初期化対象のパラメータ構造体ポインタ
 */
void ModularCANLib_RoboMas_DeviceParam_Struct_Init(ModularCANLib_DeviceParam_RoboMas_Type *device_param);

/**
 * @brief RoboMasterデバイスの初期化およびCAN管理テーブルへの登録を行います。
 * @param device_info デバイス情報の構造体ポインタ
 */
void ModularCANLib_RoboMas_Device_Init(ModularCANLib_DeviceInfo_Type *device_info);

/**
 * @brief RoboMasデバイス初期化後のタイマーセットアップ
 * * 全てのCANバスを走査し、RoboMasデバイスが登録されている場合にのみ
 * 制御ループ用の周期タイマー（1kHz）を生成・起動します。
 */
void ModularCANLib_RoboMas_AfterDeviceInit(void);

/**
 * @brief 指定したRoboMasterデバイスが通信中（接続済み）か確認します。
 * @return true: 接続済み, false: 未接続
 */
bool ModularCANLib_RoboMas_IsDeviceConnected(const ModularCANLib_DeviceInfo_Type *dev_info);

/**
 * @brief 全ての登録済みRoboMasterデバイスが通信中（接続済み）か確認します。
 * @return true: 全て接続済み, false: 未接続のデバイスあり
 */
bool ModularCANLib_RoboMas_AllDevice_Connected();

/**
 * @brief 受信したCAN IDがRoboMasterモーターのフィードバックIDかどうかを判定します。
 *
 * RoboMaster用のESC（C610, C620など）が
 * フィードバックデータを送信する際に使用する標準ID（0x201 〜 0x208）の範囲内
 * であるかを確認します。
 *
 * @param[in] can_id_type CAN IDのタイプ（標準/拡張）。
 * @param[in] can_id      判定対象のCAN ID。
 *
 * @retval true  RoboMasterモーターのフィードバックIDの範囲内（0x201-0x208）かつ標準ID。
 * @retval false 範囲外、または拡張IDの場合。
 */
bool ModularCANLib_RoboMas_IsRoboMasID_RX(ModularCANLib_CANId_Type can_id_type, uint32_t can_id);

/**
 * @brief  RoboMaster用CAN受信データの解析およびフィードバック更新
 *
 * @details 受信したCANメッセージのIDからRoboMasterのID（1-8）を算出し、
 * 登録されているC610およびC620デバイスの中から、該当するCANハンドルと
 * IDを持つデバイスを検索します。一致するデバイスが見つかった場合、
 * そのデバイスのフィードバックデータ（回転速度、トルク、角度等）を更新します。
 *
 * @note    RoboMaster IDは `StdId - 0x200` として算出されます。
 *
 * @param[in] hcan   受信したCANのハンドル（HAL_Status等を確認するためのポインタ）
 * @param[in] rx_msg 受信したCANメッセージ構造体へのポインタ
 *
 * @return void
 */
void ModularCANLib_RoboMas_ProcessRxData(const CAN_HandleTypeDef* hcan, const CAN_RxMsg* rx_msg);

#endif //MODULARCANLIB_ROBOMAS_DEF_H
