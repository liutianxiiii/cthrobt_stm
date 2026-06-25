//
// Created by tomoya on 26/04/28.
//

#include "RobStride/ModularCANLib_RobStride.h"

#include "can.h"
#include <math.h>  // 数学関数 (fabsf, fmaxf, fminf など) を使用するためにインクルード
#include <stdio.h> // 標準入出力関数 (MODULARCANLIB_PRINTF_DEBUG など) を使用するためにインクルード
#include <cmsis_os.h>
#include "memory.h"

#include "ModularCANLib_DeviceInfo_Def.h"
#include "ModularCANLib_Sys.h"
#include "RobStride/ModularCANLib_RobStride_Constant.h"

typedef struct {
    uint8_t id;
    ModularCANLib_DeviceInfo_Type *dev_info;
} MotorID_And_DeviceInfoPtr_Type;

// 変数
static MotorID_And_DeviceInfoPtr_Type table_MotorID_To_DeviceInfoPtr[NUM_OF_CANS][MODULARCANLIB_MAX_DEVICES];
static int tableSize_MotorID_To_DeviceInfoPtr[NUM_OF_CANS] = {0};
static ModularCANLib_DeviceInfo_Type *robStride_AllDevices[MODULARCANLIB_MAX_DEVICES] = {NULL};
static int size_robStride_AllDevices = 0;

// freeRTOS
osTimerId modularCANLib_RobStrideTimerHandle;
osStaticTimerDef_t modularCANLib_RobStrideTimerControlBlock;

// Private Function Prototypes --------------------------------

// Functions --------------------------------

static float uint_to_float(const uint16_t x, const float x_min, const float x_max) {
    const uint16_t type_max = 0xFFFF;
    const float span = x_max - x_min;
    return (float)x / (float)type_max * span + x_min;
}

static ModularCANLib_DeviceInfo_Type* Robstride_MotorID_To_DeviceInfoPtr(const CAN_HandleTypeDef* const hcan, const uint16_t motor_id) {
    const uint8_t can_idx = ModularCANLib_hcan2idx(hcan);
    for (int i = 0; i < tableSize_MotorID_To_DeviceInfoPtr[can_idx]; i++) {
        if (table_MotorID_To_DeviceInfoPtr[can_idx][i].id != motor_id) continue;
        return table_MotorID_To_DeviceInfoPtr[can_idx][i].dev_info;
    }
    return NULL;
}

static HAL_StatusTypeDef Robstride_SendBytes(CAN_HandleTypeDef *const hcan, const uint8_t motor_id, const uint8_t cmd_id, const uint16_t option, const uint8_t *const bytes, const uint32_t size) {
    const uint32_t can_id = cmd_id << 24 | option << 8 | motor_id;
    return ModularCANLib_Sys_SendBytes(hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, bytes, size);
}

static void Robstride_RequestReadParameter(const ModularCANLib_DeviceInfo_Type *const device_info, const uint16_t address) {
    uint8_t can_data[8];
    can_data[0] = address & 0x00FF;
    can_data[1] = address >> 8;
    const uint16_t option = 0x00 << 8 | ROBSTRIDE_MASTER_ID;
    Robstride_SendBytes(device_info->hcan, device_info->node_id, CMD_RAM_READ, option, can_data, sizeof(can_data));
}

static void Robstride_WriteFloatData(const ModularCANLib_DeviceInfo_Type *const device_info, const uint16_t address, const float data) {
    uint8_t can_data[8] = { 0x00 };
    can_data[0] = address & 0x00FF;
    can_data[1] = address >> 8;
    memcpy(&can_data[4], &data, 4);
    Robstride_SendBytes(device_info->hcan, device_info->node_id, CMD_RAM_WRITE, ROBSTRIDE_MASTER_ID, can_data, sizeof(can_data));
}

static void Robstride_WriteIntData(const ModularCANLib_DeviceInfo_Type *const device_info, const uint16_t address, const int data) {
    uint8_t can_data[8];
    can_data[0] = address & 0x00FF;
    can_data[1] = address >> 8;
    can_data[4] = (uint8_t)data;
    Robstride_SendBytes(device_info->hcan, device_info->node_id, CMD_RAM_WRITE, ROBSTRIDE_MASTER_ID, can_data, sizeof(can_data));
}

static void Get_Robstride_MCUID(const CAN_HandleTypeDef* const hcan, const uint8_t rxData[], uint8_t device_id) {
    ModularCANLib_DeviceInfo_Type *device_info = Robstride_MotorID_To_DeviceInfoPtr(hcan, device_id);
    if (device_info == NULL) {
        MODULARCANLIB_PRINTF_DEBUG("Get_Robstride_MCUID: device_info is NULL\n");
        return;
    }
    ModularCANLib_RobStride_FeedbackData_Type *const feedback = &device_info->device_param.robstride_param.feedback;
    memcpy(&feedback->mcu_id, &rxData[0], 8);
    feedback->mcu_id = feedback->_raw_data.mcu_id;
}

static uint8_t Robstride_get_switch_state(GPIO_TypeDef *const limit_port, const uint32_t limit_pin, const ModularCANLib_RobStride_SwitchType sw_type) {
    if (sw_type == ROBSTRIDE_SWITCH_NO) {                // ノーマリオープン (Normally Open) の場合
        return HAL_GPIO_ReadPin(limit_port, limit_pin);  // ピンがHighなら1 (アクティブ), Lowなら0 (非アクティブ)
    } else {                                             // ノーマリクローズ (Normally Close) の場合
        return !HAL_GPIO_ReadPin(limit_port, limit_pin); // ピンがLowなら1 (アクティブ), Highなら0 (非アクティブ)
    }
}

static void Robstride_SetMechPosToZero(ModularCANLib_DeviceInfo_Type *const dev_info) {
    uint8_t data[8] = { 0x00 }; // 送信するデータ配列を初期化
    data[0] = 0x01;             // コマンドデータ (機械的位置をゼロにするための特定データ)
    // CANメッセージを送信 (CMD_SET_MECH_POSITION_TO_ZERO コマンド)
    while (1) {
        Robstride_SendBytes(dev_info->hcan, dev_info->node_id, CMD_SET_MECH_POSITION_TO_ZERO, ROBSTRIDE_MASTER_ID, (uint8_t *)data, sizeof(data));
        osDelay(1); // 送信後に少し待機
        // フィードバックデータを読み取る (機械的位置がゼロに設定されたか確認)
        if (fabsf(dev_info->device_param.robstride_param.feedback.position - dev_info->device_param.robstride_param.offset_pos) < 0.1f) {
            break; // 機械的位置がゼロに設定されたらループを抜ける
        }
    }
}

static void Robstride_set_fb_data_raw(const CAN_HandleTypeDef* const hcan, const uint32_t ExtID, const uint8_t rxData[], const uint8_t device_id) {
    ModularCANLib_DeviceInfo_Type *const dev_info = Robstride_MotorID_To_DeviceInfoPtr(hcan, device_id);
    if (dev_info == NULL) {
        MODULARCANLIB_PRINTF_DEBUG("Robstride_set_fb_data_raw: device_info is NULL\n");
        return;
    }
    ModularCANLib_RobStride_FeedbackData_Type *const feedback = &dev_info->device_param.robstride_param.feedback;
    ModularCANLib_RobStride_FeedbackDataRaw_Type *const feedback_raw = &feedback->_raw_data;
    feedback_raw->_get_counter += 1;
    if (feedback_raw->_get_counter > 128) {
        feedback_raw->_get_counter = 3; // overflow対策
    }
    feedback_raw->pos = rxData[1] | rxData[0] << 8;
    feedback_raw->vel = rxData[3] | rxData[2] << 8;
    feedback_raw->torque = rxData[5] | rxData[4] << 8;
    feedback_raw->temp = rxData[7] | rxData[6] << 8;

    feedback->device_id = device_id;
    feedback->get_flag = (feedback_raw->_get_counter > 2);
    const uint8_t mode_status = (ExtID >> 22) & 0x03; // bit22と23を抽出
    feedback->mode_status = mode_status;

    switch (dev_info->device_type) {
        case ModularCANLib_DeviceType_RobStride_02:
            feedback->position = uint_to_float(feedback_raw->pos, P_MIN_ROBSTRIDE02, P_MAX_ROBSTRIDE02) * feedback->quant_per_rot * feedback->plus_minus;
            feedback->position += feedback->offset_pos;
            feedback->velocity = uint_to_float(feedback_raw->vel, V_MIN_ROBSTRIDE02, V_MAX_ROBSTRIDE02) * feedback->quant_per_rot * feedback->plus_minus;
            feedback->torque = uint_to_float(feedback_raw->torque, T_MIN_ROBSTRIDE02, T_MAX_ROBSTRIDE02);
            break;

        case ModularCANLib_DeviceType_RobStride_04:
            feedback->position = uint_to_float(feedback_raw->pos, P_MIN_ROBSTRIDE04, P_MAX_ROBSTRIDE04) * feedback->quant_per_rot * feedback->plus_minus;
            feedback->position += feedback->offset_pos;
            feedback->velocity = uint_to_float(feedback_raw->vel, V_MIN_ROBSTRIDE04, V_MAX_ROBSTRIDE04) * feedback->quant_per_rot * feedback->plus_minus;
            feedback->torque = uint_to_float(feedback_raw->torque, T_MIN_ROBSTRIDE04, T_MAX_ROBSTRIDE04);
            break;

        case ModularCANLib_DeviceType_RobStride_05_Edu:
            feedback->position = uint_to_float(feedback_raw->pos, P_MIN_ROBSTRIDE05, P_MAX_ROBSTRIDE05) * feedback->quant_per_rot * feedback->plus_minus;
            feedback->position += feedback->offset_pos;
            feedback->velocity = uint_to_float(feedback_raw->vel, V_MIN_ROBSTRIDE05, V_MAX_ROBSTRIDE05) * feedback->quant_per_rot * feedback->plus_minus;
            feedback->torque = uint_to_float(feedback_raw->torque, T_MIN_ROBSTRIDE05, T_MAX_ROBSTRIDE05);
            break;
        default:
            break;
    }
    //    printf("id: %u, pos: %f, vel: %f, torque: %f\n\r",
    //           device_id,
    //           feedback->position,
    //           feedback->velocity,
    //           feedback->torque);
    feedback->temperature = (int)((float)(feedback_raw->temp) / 10.0);
}

void Robstride_PresetParameters(ModularCANLib_DeviceInfo_Type *const dev_info) {
    Robstride_SetRunModeToOperation(dev_info); // 制御モードを設定
    Robstride_fb_init(&dev_info->device_param.robstride_param);                                             // フィードバックを初期化
}

void Robstride_SetRunModeToOperation(ModularCANLib_DeviceInfo_Type *const dev_info) {
    Robstride_ControlDisable(dev_info);//control_disable時のみ制御モードが変更可能
    Robstride_WriteIntData(dev_info, ADDR_RUN_MODE, RUN_MODE_OPERATION); // 実行モード (制御モード) を書き込み
    Robstride_ControlEnable(dev_info);
}

void Robstride_SetVelocityLimit(ModularCANLib_DeviceInfo_Type *const dev_info) {
    switch (dev_info->device_type) {                                                            // デバイスの種類によって制限値の上限が異なる
        case ModularCANLib_DeviceType_RobStride_02:                                                                 // Robstride_02 の場合
            if (dev_info->device_param.robstride_param.velocity_limit == ROBSTRIDE_VELOCITY_LIMIT_DISABLE) { // 速度制限が無効の場合
                dev_info->device_param.robstride_param.velocity_limit_size = 44.0f;                          // デフォルトの最大制限値を設定
            } else {                                                                       // 速度制限が有効の場合
                // 設定値を 0.0f と 44.0f の間にクリッピング
                dev_info->device_param.robstride_param.velocity_limit_size = fmaxf(fminf(dev_info->device_param.robstride_param.velocity_limit_size, 44.0f), 0.0f);
            }
            break;
        case ModularCANLib_DeviceType_RobStride_04:                                                                 // Robstride_04 の場合
            if (dev_info->device_param.robstride_param.velocity_limit == ROBSTRIDE_VELOCITY_LIMIT_DISABLE) { // 速度制限が無効の場合
                dev_info->device_param.robstride_param.velocity_limit_size = 15.0f;                          // デフォルトの最大制限値を設定
            } else {                                                                       // 速度制限が有効の場合
                // 設定値を 0.0f と 15.0f の間にクリッピング
                dev_info->device_param.robstride_param.velocity_limit_size = fmaxf(fminf(dev_info->device_param.robstride_param.velocity_limit_size, 15.0f), 0.0f);
            }
            break;
        case ModularCANLib_DeviceType_RobStride_05_Edu:
            if (dev_info->device_param.robstride_param.velocity_limit == ROBSTRIDE_VELOCITY_LIMIT_DISABLE) {
                dev_info->device_param.robstride_param.velocity_limit_size = 50.0f; // Max Speed: 50 rad/s
            } else {
                // 設定値を 0.0f と 50.0f の間にクリッピング
                dev_info->device_param.robstride_param.velocity_limit_size = fmaxf(fminf(dev_info->device_param.robstride_param.velocity_limit_size, 50.0f), 0.0f);
            }
            break;
            // defaultケースが抜けているため、Robstride_02,04,05Edu以外のデバイスタイプの場合、処理がスキップされる。
        default:
            break;
    }
    while (1) {
        Robstride_WriteFloatData(dev_info, ADDR_LIMIT_SPEED, dev_info->device_param.robstride_param.velocity_limit_size);                  // 速度制限値を書き込み
        Robstride_RequestReadParameter(dev_info, ADDR_LIMIT_SPEED);                                                      // 書き込み後に読み出し要求を送信して、設定が反映されたか確認
        osDelay(1);                                                                                                      // 書き込み後に少し待機
        if (fabsf(dev_info->device_param.robstride_param.feedback.limit_spd - dev_info->device_param.robstride_param.velocity_limit_size) < 0.01f) { // 設定が反映されたか確認
            break;                                                                                                       // 反映されたらループを抜ける
        }
    }
}

void Robstride_SetCurrentLimit(ModularCANLib_DeviceInfo_Type *const dev_info) {
    switch (dev_info->device_type) {                                                          // デバイスの種類によって制限値の上限が異なる
        case ModularCANLib_DeviceType_RobStride_02:                                                               // Robstride_02 の場合
            if (dev_info->device_param.robstride_param.current_limit == ROBSTRIDE_CURRENT_LIMIT_DISABLE) { // 電流制限が無効の場合
                dev_info->device_param.robstride_param.current_limit_size = 23.0f;                         // デフォルトの最大制限値を設定
            } else {                                                                     // 電流制限が有効の場合
                // 設定値を 0.0f と 23.0f の間にクリッピング
                dev_info->device_param.robstride_param.current_limit_size = fmaxf(fminf(dev_info->device_param.robstride_param.current_limit_size, 23.0f), 0.0f);
            }
            break;
        case ModularCANLib_DeviceType_RobStride_04: // Robstride_04 の場合
            if (dev_info->device_param.robstride_param.current_limit == ROBSTRIDE_CURRENT_LIMIT_DISABLE) {
                // 電流制限が無効の場合
                // TODO: 注意: ここでは velocity_limit_size が使われているが、current_limit_size の誤りと思われる。
                dev_info->device_param.robstride_param.velocity_limit_size = 90.0f; // デフォルトの最大制限値を設定
            } else {
                // 電流制限が有効の場合
                // TODO: 注意: ここでは velocity_limit_size が使われているが、current_limit_size の誤りと思われる。
                // 設定値を 0.0f と 90.0f の間にクリッピング
                dev_info->device_param.robstride_param.velocity_limit_size = fmaxf(fminf(dev_info->device_param.robstride_param.current_limit_size, 90.0f), 0.0f);
            }
            break;
        case ModularCANLib_DeviceType_RobStride_05_Edu:
            if (dev_info->device_param.robstride_param.current_limit == ROBSTRIDE_CURRENT_LIMIT_DISABLE) {
                dev_info->device_param.robstride_param.current_limit_size = 11.0f; // Max Current: 11 A
            } else {
                dev_info->device_param.robstride_param.current_limit_size = fmaxf(fminf(dev_info->device_param.robstride_param.current_limit_size, 11.0f), 0.0f);
            }
            break;
            // defaultケースが抜けているため、Robstride_04以外のデバイスタイプの場合、処理がスキップされる。
        default:
            break;
    }
    while (1) {
        Robstride_WriteFloatData(dev_info, ADDR_LIMIT_CURRENT, dev_info->device_param.robstride_param.current_limit_size); // 電流制限値を書き込み
        Robstride_RequestReadParameter(dev_info, ADDR_LIMIT_CURRENT);                                    // 書き込み後に読み出し要求を送信して、設定が反映されたか確認
        osDelay(1);                                                                                      // 書き込み後に少し待機
        if (fabsf(dev_info->device_param.robstride_param.feedback.limit_cur - dev_info->device_param.robstride_param.current_limit_size) < 0.01f) {
            // 設定が反映されたか確認
            break; // 反映されたらループを抜ける
        }
    }
}

void Robstride_SetTorqueLimit(ModularCANLib_DeviceInfo_Type *const dev_info) {
    switch (dev_info->device_type) {                                                        // デバイスの種類によって制限値の上限が異なる
        case ModularCANLib_DeviceType_RobStride_02:                                                             // Robstride_02 の場合
            if (dev_info->device_param.robstride_param.torque_limit == ROBSTRIDE_TORQUE_LIMIT_DISABLE) { // トルク制限が無効の場合
                dev_info->device_param.robstride_param.torque_limit_size = 17.0f;                        // デフォルトの最大制限値を設定
            } else {                                                                   // トルク制限が有効の場合
                // 設定値を 0.0f と 17.0f の間にクリッピング
                dev_info->device_param.robstride_param.torque_limit_size = fmaxf(fminf(dev_info->device_param.robstride_param.torque_limit_size, 17.0f), 0.0f);
            }
            break;
        case ModularCANLib_DeviceType_RobStride_04:                                                             // Robstride_04 の場合
            if (dev_info->device_param.robstride_param.torque_limit == ROBSTRIDE_TORQUE_LIMIT_DISABLE) { // トルク制限が無効の場合
                dev_info->device_param.robstride_param.torque_limit_size = 120.0f;                       // デフォルトの最大制限値を設定
            } else {                                                                   // トルク制限が有効の場合
                // 設定値を 0.0f と 120.0f の間にクリッピング
                dev_info->device_param.robstride_param.torque_limit_size = fmaxf(fminf(dev_info->device_param.robstride_param.torque_limit_size, 120.0f), 0.0f);
            }
            break;
        case ModularCANLib_DeviceType_RobStride_05_Edu:
            if (dev_info->device_param.robstride_param.torque_limit == ROBSTRIDE_TORQUE_LIMIT_DISABLE) {
                dev_info->device_param.robstride_param.torque_limit_size = 6.0f; // Max Torque: 6 Nm
            } else {
                dev_info->device_param.robstride_param.torque_limit_size = fmaxf(fminf(dev_info->device_param.robstride_param.torque_limit_size, 6.0f), 0.0f);
            }
            break;
            // defaultケースが抜けているため、Robstride_04以外のデバイスタイプの場合、処理がスキップされる。
        default:
            break;
    }
    Robstride_WriteFloatData(dev_info, ADDR_LIMIT_TORQUE, dev_info->device_param.robstride_param.torque_limit_size); // トルク制限値を書き込み
    // TODO: Robstride_SetVelocityLimit(), SetCurrentLimit() のように書き込みを確認しなくてもよいのか？
}

void Robstride_Calibration(ModularCANLib_DeviceInfo_Type *const device_info, float calib_velocity, const ModularCANLib_RobStride_SwitchType sw_type, GPIO_TypeDef *const limit_port, const uint32_t limit_pin) {
    switch (device_info->device_param.robstride_param.use_internal_offset) {
        case ROBSTRIDE_USE_OFFSET_POS_INTERNAL:               // 内部オフセットを使用する場合
            return;                                           // キャリブレーション不要
        case ROBSTRIDE_USE_OFFSET_POS_INITIAL:                // 初期位置をオフセットとして使用する場合
            Robstride_SetMechPosToZero(device_info); // 現在の機械的位置をゼロに設定
            return;                                           // キャリブレーション完了
        case ROBSTRIDE_USE_OFFSET_POS_CALIB:                  // キャリブレーションによりオフセットを設定する場合
            break;                                            // キャリブレーション処理へ進む
        default:                                              // その他の場合
            return;                                           // 何もしない
    }
    device_info->device_param.robstride_param.target.kp = 0.0f; // 制御モードを速度制御に設定（位置制御を無効化）
    Robstride_ControlEnable(device_info);                  // モータ制御を有効化
    if (device_info->device_param.robstride_param.rotation == ROBSTRIDE_ROT_CW) {     // 回転方向が時計回り(CW)の場合
        calib_velocity *= -1;                                       // キャリブレーション速度を反転
    }
    device_info->device_param.robstride_param.target.vel = calib_velocity; // 目標速度を設定
    while (!Robstride_get_switch_state(limit_port, limit_pin, sw_type)) {
        Robstride_SendTarget(device_info); // TODO FreeRTOS Timer化
        osDelay(1);
    } // リミットスイッチが押されるまで待機
    Robstride_ControlDisable(device_info);   // モータ制御を無効化
    Robstride_SetMechPosToZero(device_info); // 現在の機械的位置をゼロに設定 (原点確定)
}

// floatを任意のビット数の整数に変換する関数
static uint16_t float_to_uint(float x, float x_min, float x_max, int bits) {
    float span = x_max - x_min;
    float offset = x_min;
    // 値を最小・最大範囲内にクリッピング
    if (x > x_max) x = x_max;
    else if (x < x_min) x = x_min;

    // (x - min) / span * (2^bits - 1)
    return (uint16_t)(((x - offset) / span) * ((float)((1 << bits) - 1)));
}

void Robstride_SendTarget(const ModularCANLib_DeviceInfo_Type *const device_info) {
    // operationモード目標値設定

    // 1. 送信する各パラメータを取得
    const ModularCANLib_DeviceParam_RobStride_Type *const param = &device_info->device_param.robstride_param;
    const ModularCANLib_RobStride_Target_Type *const target = &param->target;
    const float plus_minus = param->rotation == ROBSTRIDE_ROT_CW ? -1.0f : 1.0f;
    const float p_des = (target->pos - param->offset_pos) / param->quant_per_rot * plus_minus; // 目標位置 (オフセットや回転方向計算は他のモードと同様に行う必要があります)
    const float v_des = target->vel / param->quant_per_rot * plus_minus;  // 目標速度 (基本は0)
    const float kp = target->kp;
    const float kd = target->kd;
    const float t_ff = target->torque_ff / param->quant_per_rot * plus_minus;   // フィードフォワードトルク (基本は0)

    // min max設定
    float p_min, p_max;
    float v_min, v_max;
    float kp_min, kp_max;
    float kd_min, kd_max;
    float t_min, t_max;
    switch (device_info->device_type) {
        case ModularCANLib_DeviceType_RobStride_02:
            p_min = P_MIN_ROBSTRIDE02, p_max = P_MAX_ROBSTRIDE02;
            v_min = V_MIN_ROBSTRIDE02, v_max = V_MAX_ROBSTRIDE02;
            kp_min = KP_MIN_ROBSTRIDE02, kp_max = KP_MAX_ROBSTRIDE02;
            kd_min = KD_MIN_ROBSTRIDE02, kd_max = KD_MAX_ROBSTRIDE02;
            t_min = T_MIN_ROBSTRIDE02, t_max = T_MAX_ROBSTRIDE02;
            break;
        case ModularCANLib_DeviceType_RobStride_04:
            p_min = P_MIN_ROBSTRIDE04, p_max = P_MAX_ROBSTRIDE04;
            v_min = V_MIN_ROBSTRIDE04, v_max = V_MAX_ROBSTRIDE04;
            kp_min = KP_MIN_ROBSTRIDE04, kp_max = KP_MAX_ROBSTRIDE04;
            kd_min = KD_MIN_ROBSTRIDE04, kd_max = KD_MAX_ROBSTRIDE04;
            t_min = T_MIN_ROBSTRIDE04, t_max = T_MAX_ROBSTRIDE04;
            break;
        case ModularCANLib_DeviceType_RobStride_05_Edu:
            p_min = P_MIN_ROBSTRIDE05, p_max = P_MAX_ROBSTRIDE05;
            v_min = V_MIN_ROBSTRIDE05, v_max = V_MAX_ROBSTRIDE05;
            kp_min = KP_MIN_ROBSTRIDE05, kp_max = KP_MAX_ROBSTRIDE05;
            kd_min = KD_MIN_ROBSTRIDE05, kd_max = KD_MAX_ROBSTRIDE05;
            t_min = T_MIN_ROBSTRIDE05, t_max = T_MAX_ROBSTRIDE05;
            break;
        default:
            MODULARCANLIB_PRINTF_DEBUG("Robstride_SendTarget Unknown device type: %u\n", device_info->device_type);
            return;
    }

    // 2. floatを整数に変換 (位置は16bit、他は12bit)
    const uint16_t p_int = float_to_uint(p_des, p_min, p_max, 16);
    const uint16_t v_int = float_to_uint(v_des, v_min, v_max, 12);
    const uint16_t kp_int = float_to_uint(kp, kp_min, kp_max, 12);
    const uint16_t kd_int = float_to_uint(kd, kd_min, kd_max, 12);
    const uint16_t t_int = float_to_uint(t_ff, t_min, t_max, 12);

    // 3. 8バイトにビットパッキング
    uint8_t data[8];
    data[0] = p_int >> 8;
    data[1] = p_int & 0xFF;
    data[2] = v_int >> 4;
    data[3] = ((v_int & 0xF) << 4) | (kp_int >> 8);
    data[4] = kp_int & 0xFF;
    data[5] = kd_int >> 4;
    data[6] = ((kd_int & 0xF) << 4) | (t_int >> 8);
    data[7] = t_int & 0xFF;

    // 送信
    Robstride_SendBytes(device_info->hcan, device_info->node_id, CMD_OPERATION, ROBSTRIDE_MASTER_ID, data, sizeof(data));
}

void Robstride_ControlEnable(ModularCANLib_DeviceInfo_Type *const dev_info) {
    uint8_t data[8] = { 0x00 }; // 送信するデータ配列を初期化 (内容はCMD_ENABLEでは使用されないことが多いが、形式として送信)
    // CANメッセージを送信 (CMD_ENABLE コマンド)
    while (1) {
        Robstride_SendBytes(dev_info->hcan, dev_info->node_id, CMD_ENABLE, ROBSTRIDE_MASTER_ID, (uint8_t *)data, sizeof(data));
        osDelay(1);                                                                        // 書き込み後に少し待機
        if (dev_info->device_param.robstride_param.feedback.mode_status == ROBSTRIDE_STATE_ENABLE) { // モータが有効になったか確認
            break;                                                                         // 有効になったらループを抜ける
        }
    }
    dev_info->device_param.robstride_param._enable_flag = 1; // 有効フラグを立てる

    // 安全のため、モータがEnableになった後にリミット類を設定
    Robstride_SetVelocityLimit(dev_info);
    Robstride_SetCurrentLimit(dev_info);
    Robstride_SetTorqueLimit(dev_info);
}

void Robstride_ControlDisable(ModularCANLib_DeviceInfo_Type *const dev_info) {
    uint8_t data[8] = { 0x00 }; // 送信するデータ配列を初期化 (内容はCMD_RESETでは使用されないことが多いが、形式として送信)
    // CANメッセージを送信 (CMD_RESET コマンド)
    while (1) {
        Robstride_SendBytes(dev_info->hcan, dev_info->node_id, CMD_RESET, ROBSTRIDE_MASTER_ID, data, sizeof(data));
        osDelay(1);                                                                         // 書き込み後に少し待機
        if (dev_info->device_param.robstride_param.feedback.mode_status == ROBSTRIDE_STATE_DISABLE) { // モータが無効になったか確認
            break;                                                                          // 無効になったらループを抜ける
        }
    }
    dev_info->device_param.robstride_param._enable_flag = 0; // 有効フラグを降ろす
}

void Robstride_CheckActiveReportStatus(ModularCANLib_DeviceInfo_Type *const device_info) {
    Robstride_RequestReadParameter(device_info, ADDR_EPSCAN_TIME);
}

void Robstride_RequestAllParameters(ModularCANLib_DeviceInfo_Type *const device_info) {
    MODULARCANLIB_PRINTF_DEBUG("--- Requesting all parameters for device ID: %d ---\n\r", device_info->node_id);

    // マクロを使用してアドレスの配列を初期化
    const uint16_t addresses[] = ROBSTRIDE_READABLE_ADDRESS_LIST;

    const uint8_t num_addresses = sizeof(addresses) / sizeof(addresses[0]);

    for (uint8_t i = 0; i < num_addresses; i++) {
        Robstride_RequestReadParameter(device_info, addresses[i]);
        osDelay(1); // モーターの応答とCANバスの負荷を考慮して短い遅延を入れる
    }
}

void Robstride_PrintAllParameters(const ModularCANLib_RobStride_FeedbackData_Type *const fb_data) {
    if (fb_data == NULL) {
        MODULARCANLIB_PRINTF_DEBUG("Error: Feedback data pointer is NULL.\n\r");
        return;
    }
    MODULARCANLIB_PRINTF_DEBUG("\n\r--- Robstride Parameters (Device ID: %d) ---\n\r", fb_data->device_id);
    MODULARCANLIB_PRINTF_DEBUG(" [Real-time Data]\n\r");
    MODULARCANLIB_PRINTF_DEBUG("  - Position:         %f\n\r", fb_data->position);
    MODULARCANLIB_PRINTF_DEBUG("  - Velocity:         %f\n\r", fb_data->velocity);
    MODULARCANLIB_PRINTF_DEBUG("  - Current (IqF):    %f\n\r", fb_data->current);
    MODULARCANLIB_PRINTF_DEBUG("  - Torque (Type 2):  %f\n\r", fb_data->torque);
    MODULARCANLIB_PRINTF_DEBUG("  - Temperature:      %d C\n\r", fb_data->temperature);
    MODULARCANLIB_PRINTF_DEBUG("  - Bus Voltage:      %f V\n\r", fb_data->vbus);
    MODULARCANLIB_PRINTF_DEBUG("\n\r [Control & Limit Settings]\n\r");
    MODULARCANLIB_PRINTF_DEBUG("  - Run Mode:         %u\n\r", fb_data->run_mode);
    MODULARCANLIB_PRINTF_DEBUG("  - Iq Ref:           %f A\n\r", fb_data->iq_ref);
    MODULARCANLIB_PRINTF_DEBUG("  - Speed Ref:        %f rad/s\n\r", fb_data->spd_ref);
    MODULARCANLIB_PRINTF_DEBUG("  - Position Ref:     %f rad\n\r", fb_data->loc_ref);
    MODULARCANLIB_PRINTF_DEBUG("  - Limit Torque:     %f Nm\n\r", fb_data->limit_torque);
    MODULARCANLIB_PRINTF_DEBUG("  - Limit Speed:      %f rad/s\n\r", fb_data->limit_spd);
    MODULARCANLIB_PRINTF_DEBUG("  - Limit Current:    %f A\n\r", fb_data->limit_cur);
    MODULARCANLIB_PRINTF_DEBUG("\n\r [PID Gains]\n\r");
    MODULARCANLIB_PRINTF_DEBUG("  - Current Kp:       %f\n\r", fb_data->cur_kp);
    MODULARCANLIB_PRINTF_DEBUG("  - Current Ki:       %f\n\r", fb_data->cur_ki);
    MODULARCANLIB_PRINTF_DEBUG("  - Current Filt Gain:%f\n\r", fb_data->cur_filt_gain);
    MODULARCANLIB_PRINTF_DEBUG("  - Position Kp:      %f\n\r", fb_data->loc_kp);
    MODULARCANLIB_PRINTF_DEBUG("  - Speed Kp:         %f\n\r", fb_data->spd_kp);
    MODULARCANLIB_PRINTF_DEBUG("  - Speed Ki:         %f\n\r", fb_data->spd_ki);
    MODULARCANLIB_PRINTF_DEBUG("  - Speed Filt Gain:  %f\n\r", fb_data->spd_filt_gain);
    MODULARCANLIB_PRINTF_DEBUG("\n\r [Profile Settings (PP Mode)]\n\r");
#ifndef USE_OLD_FIRMWARE_ADDRESSES
    // 新しいファームウェアのみに存在するパラメータ
    MODULARCANLIB_PRINTF_DEBUG("  - Acceleration:     %f rad/s^2\n\r", fb_data->acc_rad);
#endif
    MODULARCANLIB_PRINTF_DEBUG("  - Max Velocity:     %f rad/s\n\r", fb_data->vel_max_pp);
    MODULARCANLIB_PRINTF_DEBUG("  - Acceleration Set: %f rad/s^2\n\r", fb_data->acc_set_pp);
    MODULARCANLIB_PRINTF_DEBUG("\n\r [System Settings]\n\r");
    MODULARCANLIB_PRINTF_DEBUG("  - Active Report Time:%u\n\r", fb_data->epscan_time);
    MODULARCANLIB_PRINTF_DEBUG("  - CAN Timeout:      %lu\n\r", fb_data->can_timeout);
#ifndef USE_OLD_FIRMWARE_ADDRESSES
    // 新しいファームウェアのみに存在するパラメータ
    MODULARCANLIB_PRINTF_DEBUG("  - Zero Flag (zero_sta): %u\n\r", fb_data->zero_sta);
    MODULARCANLIB_PRINTF_DEBUG("  - Add Offset:           %f\n\r", fb_data->add_offset);
#endif
    MODULARCANLIB_PRINTF_DEBUG("------------------------------------------\n\r\n\r");
}

void Robstride_Debug_Check_All_Parameters(ModularCANLib_DeviceInfo_Type *const device_info) {
    Robstride_RequestAllParameters(device_info);
    osDelay(100); // 全てのパラメータが更新されるまで少し待機
    Robstride_PrintAllParameters(&device_info->device_param.robstride_param.feedback);
}

static void Robstride_ProcessParameter(const CAN_HandleTypeDef* const hcan, const uint8_t rxData[], const uint8_t device_id) {
    const uint16_t address = rxData[0] | rxData[1] << 8;
    uint8_t uint8_data;
    // int16_t int16_data;
    float float_data;
    uint16_t uint16_data;
    uint32_t uint32_data;

    ModularCANLib_DeviceInfo_Type *device_info = Robstride_MotorID_To_DeviceInfoPtr(hcan, device_id);
    if (device_info == NULL) {
        MODULARCANLIB_PRINTF_DEBUG("Robstride_ProcessParameter: Unknown device_id: %u\n", device_id);
        return;
    }
    ModularCANLib_RobStride_FeedbackData_Type *const feedback = &device_info->device_param.robstride_param.feedback;
    ModularCANLib_RobStride_FeedbackDataRaw_Type *const feedback_raw = &feedback->_raw_data;

    switch (address) {
        case ADDR_RUN_MODE: // 0x7005
            memcpy(&uint8_data, &rxData[4], sizeof(uint8_data));
            feedback->run_mode = uint8_data;
            break;
        case ADDR_IQ_REF: // 0x7006
            memcpy(&float_data, &rxData[4], sizeof(float_data));
            feedback->iq_ref = float_data;
            break;
        case ADDR_SPEED_REF: // 0x700A
            memcpy(&float_data, &rxData[4], sizeof(float_data));
            feedback->spd_ref = float_data;
            break;
        case ADDR_LIMIT_TORQUE: // 0x700B
            memcpy(&float_data, &rxData[4], sizeof(float_data));
            feedback->limit_torque = float_data;
            break;
        case ADDR_CURRENT_KP: // 0x7010
            memcpy(&float_data, &rxData[4], sizeof(float_data));
            feedback->cur_kp = float_data;
            break;
        case ADDR_CURRENT_KI: // 0x7011
            memcpy(&float_data, &rxData[4], sizeof(float_data));
            feedback->cur_ki = float_data;
            break;
        case ADDR_CURRENT_FILTER_GAIN: // 0x7014
            memcpy(&float_data, &rxData[4], sizeof(float_data));
            feedback->cur_filt_gain = float_data;
            break;
        case ADDR_LOC_REF: // 0x7016
            memcpy(&float_data, &rxData[4], sizeof(float_data));
            feedback->loc_ref = float_data;
            break;
        case ADDR_LIMIT_SPEED: // 0x7017
            memcpy(&float_data, &rxData[4], sizeof(float_data));
            feedback->limit_spd = float_data;
            break;
        case ADDR_LIMIT_CURRENT: // 0x7018
            memcpy(&float_data, &rxData[4], sizeof(float_data));
            feedback->limit_cur = float_data;
            break;
        case ADDR_MECH_POS:
            memcpy(&float_data, &rxData[4], sizeof(float_data));
            feedback->position = float_data * feedback->quant_per_rot * feedback->plus_minus;
            feedback->position += feedback->offset_pos;
            // printf("motor%d: pos %f\n\r", (int)device_id, feedback->position);
            feedback_raw->_get_counter += 1;
            if (feedback_raw->_get_counter > 128) {
                feedback_raw->_get_counter = 3; // overflow対策
            }
            feedback->get_flag = (feedback_raw->_get_counter > 2);
            break;
        case ADDR_IQF:
            memcpy(&float_data, &rxData[4], sizeof(float_data));
            feedback->current = float_data * feedback->plus_minus;
            feedback_raw->_get_counter += 1;
            // printf("motor%d: current %f\n\r", (int)device_id, feedback->current);
            if (feedback_raw->_get_counter > 128) {
                feedback_raw->_get_counter = 3; // overflow対策
            }
            feedback->get_flag = (feedback_raw->_get_counter > 2);
            break;
        case ADDR_MECH_VEL:
            memcpy(&float_data, &rxData[4], sizeof(float_data));
            feedback->velocity = float_data * feedback->quant_per_rot * feedback->plus_minus;
            // printf("motor%d: vel %f\n\r", (int)device_id, feedback->velocity);
            feedback_raw->_get_counter += 1;
            if (feedback_raw->_get_counter > 128) {
                feedback_raw->_get_counter = 3; // overflow対策
            }
            feedback->get_flag = (feedback_raw->_get_counter > 2);
            break;
        case ADDR_VBUS: // 0x701C
            memcpy(&float_data, &rxData[4], sizeof(float_data));
            feedback->vbus = float_data;
            break;
        case ADDR_LOC_KP: // 0x701E
            memcpy(&float_data, &rxData[4], sizeof(float_data));
            feedback->loc_kp = float_data;
            break;
        case ADDR_SPD_KP: // 0x701F
            memcpy(&float_data, &rxData[4], sizeof(float_data));
            feedback->spd_kp = float_data;
            break;
        case ADDR_SPD_KI: // 0x7020
            memcpy(&float_data, &rxData[4], sizeof(float_data));
            feedback->spd_ki = float_data;
            break;
        case ADDR_SPD_FILTER_GAIN: // 0x7021
            memcpy(&float_data, &rxData[4], sizeof(float_data));
            feedback->spd_filt_gain = float_data;
            break;
        case ADDR_ACC_RAD: // 0x7022
            memcpy(&float_data, &rxData[4], sizeof(float_data));
            feedback->acc_rad = float_data;
            break;
        case ADDR_VEL_MAX_PP: // 0x7024
            memcpy(&float_data, &rxData[4], sizeof(float_data));
            feedback->vel_max_pp = float_data;
            break;
        case ADDR_ACC_SET_PP: // 0x7025
            memcpy(&float_data, &rxData[4], sizeof(float_data));
            feedback->acc_set_pp = float_data;
            break;
        case ADDR_EPSCAN_TIME: // 0x7026
            memcpy(&uint16_data, &rxData[4], sizeof(uint16_data));
            feedback->epscan_time = uint16_data;
            break;
        case ADDR_CAN_TIMEOUT: // 0x7028
            memcpy(&uint32_data, &rxData[4], sizeof(uint32_data));
            feedback->can_timeout = uint32_data;
            break;
        case ADDR_ZERO_STA: // 0x7029
            memcpy(&uint8_data, &rxData[4], sizeof(uint8_data));
            feedback->zero_sta = uint8_data;
            break;
        case ADDR_ADD_OFFSET: // 0x702B
            memcpy(&float_data, &rxData[4], sizeof(float_data));
            feedback->add_offset = float_data;
            break;
        default:
            break;
    }
}

static void Robstride_ProcessFault(const uint8_t rxData[], const uint8_t device_id) {
    if ((rxData[0] & 0x01) != 0) {
        printf("motor%d:Motor over temperature fault\n\r", (int)device_id);
    }
    if ((rxData[0] & 0x02) != 0) {
        printf("motor%d:Driver chip failure\n\r", (int)device_id);
    }
    if ((rxData[0] & 0x04) != 0) {
        printf("motor%d:Undervoltage fault\n\r", (int)device_id);
    }
    if ((rxData[0] & 0x08) != 0) {
        printf("motor%d:Overvoltage fault\n\r", (int)device_id);
    }
    if ((rxData[0] & 0x10) != 0) {
        printf("motor%d:B-phase current sampling overcurrent\n\r", (int)device_id);
    }
    if ((rxData[0] & 0x20) != 0) {
        printf("motor%d:C-phase current sampling overcurrent\n\r", (int)device_id);
    }
    if ((rxData[0] & 0x80) != 0) {
        printf("motor%d:Encoder not calibrated\n\r", (int)device_id);
    }
    if ((rxData[0] & 0x100) != 0) {
        printf("motor%d:Overload fault\n\r", (int)device_id);
    }
    if ((rxData[0] & 0x10000) != 0) {
        printf("motor%d:A-phase current sampling overcurrent\n\r", (int)device_id);
    }
    if ((rxData[4] & 0x01) != 0) {
        printf("motor%d:Motor over temperature warning\n\r", (int)device_id);
    }
}

void Robstride_fb_init(ModularCANLib_DeviceParam_RobStride_Type *const device_param) {
    ModularCANLib_RobStride_FeedbackData_Type *const feedback = &device_param->feedback;
    if (device_param->rotation == ROBSTRIDE_ROT_CW) {
        feedback->plus_minus = -1;
    } else {
        feedback->plus_minus = 1;
    }
    feedback->quant_per_rot = device_param->quant_per_rot;
    feedback->offset_pos = device_param->offset_pos;
}

static void ModularCANLib_RobStrideTimerCallback(void const * argument) {
    for (int i = 0; i < size_robStride_AllDevices; i++) {
        const ModularCANLib_DeviceInfo_Type *const dev_info = robStride_AllDevices[i];
        if (!dev_info->device_param.robomas_param._enable_flag) continue;
        Robstride_SendTarget(dev_info);
    }
}

float Robstride_MeasureCoefficient_Torque2Acc(ModularCANLib_DeviceInfo_Type *const device_info, const float torque, const float speed) {
    if (device_info->device_type != ModularCANLib_DeviceType_RobStride_02 &&
        device_info->device_type != ModularCANLib_DeviceType_RobStride_04 &&
        device_info->device_type != ModularCANLib_DeviceType_RobStride_05_Edu) return 0.0f;

    ModularCANLib_DeviceParam_RobStride_Type *const param = &device_info->device_param.robstride_param;
    ModularCANLib_RobStride_Target_Type const given_target = param->target;
    param->target.kp = 0.0f;
    param->target.kd = 1.0f;
    param->target.vel = 0.0f;
    while (fabsf(param->feedback.velocity) < 0.01f) osDelay(1); // 減速まち
    param->target.kd = 0.0f;
    param->target.torque_ff = torque;
    const uint32_t st = HAL_GetTick();
    while (fabsf(param->feedback.velocity) > speed) osDelay(1); // 加速まち
    const uint32_t et = HAL_GetTick();
    // 停止
    param->target = given_target;
    const float dt = (float)(et - st) / 1000.0f;
    return speed / dt / torque;
}

void Robstride_MoveWithRamp(ModularCANLib_DeviceInfo_Type *const device_info, const float target_pos, const float max_speed, const float acc) {
    if (device_info->device_type != ModularCANLib_DeviceType_RobStride_02 &&
        device_info->device_type != ModularCANLib_DeviceType_RobStride_04 &&
        device_info->device_type != ModularCANLib_DeviceType_RobStride_05_Edu) return;

    ModularCANLib_DeviceParam_RobStride_Type *const param = &device_info->device_param.robstride_param;
    const ModularCANLib_RobStride_FeedbackData_Type *const feedback = &param->feedback;
    ModularCANLib_RobStride_Target_Type *const target = &param->target;

    if (!param->_enable_flag) return;

    const float start_pos = feedback->position;
    const float delta_pos = target_pos - start_pos;

    // 移動距離がゼロの場合は即座に終了
    if (fabsf(delta_pos) < 1e-5f) {
        target->vel = 0.0f;
        target->pos = target_pos;
        return;
    }

    // 台形or三角形の判定
    // (最高速度に到達するための加減速距離の合計) >= (実際の移動距離) なら三角形
    const bool is_triangle_curve = (max_speed * max_speed / acc) >= fabsf(delta_pos);

    // t1: 加速モード終了時刻
    // t2: 定速モード終了時刻
    // t3: 減速モード終了時刻
    float t1, t2, t3;
    if (is_triangle_curve) {
        t1 = t2 = sqrtf(fabsf(delta_pos) / acc);
        t3 = 2.0f * t1;
    }
    else {
        t1 = max_speed / acc;
        t2 = t1 + (fabsf(delta_pos) - (max_speed * max_speed / acc)) / max_speed;
        t3 = t1 + t2; // t2 + t1 と同義 (減速にかかる時間は加速と同じ)
    }
    // 移動方向を考慮した加速度
    const float real_acc = acc * (delta_pos > 0 ? 1.0f : -1.0f);

    printf("pos: %f->%f, vel=%f, acc=%f(%f), is_triangle=%u\n", start_pos, target_pos, max_speed, acc, real_acc, is_triangle_curve);


    // 定速区間の位置（固定値）
    const float pos_at_t1 = 0.5f * real_acc * t1 * t1;
    const float pos_at_t2 = pos_at_t1 + real_acc * t1 * (t2 - t1);

    // 移動開始
    const uint32_t st = HAL_GetTick();
    uint32_t nt = HAL_GetTick();
    float dt = (float)(nt - st) * 0.001f;

    // 加速モード
    while (dt < t1) {
        target->vel = real_acc * dt;
        target->pos = start_pos + 0.5f * real_acc * dt * dt;
        osDelay(1);
        nt = HAL_GetTick();
        dt = (float)(nt - st) * 0.001f;
    }

    // 定速モード (三角形の時は t1 == t2 なのでこのループは即座にスキップされます)
    while (dt < t2) {
        target->vel = real_acc * t1;
        target->pos = start_pos + pos_at_t1 + real_acc * t1 * (dt - t1);
        osDelay(1);
        nt = HAL_GetTick();
        dt = (float)(nt - st) * 0.001f;
    }

    // 減速モード
    while (dt < t3) {
        const float dt_diff = dt - t2; // t2からの経過時間
        target->vel = real_acc * (t1 - dt_diff);
        // 位置 = t2時点の位置 + (初速 * 時間) - (1/2 * 加速度 * 時間^2)
        target->pos = start_pos + pos_at_t2 + (real_acc * t1 * dt_diff) - (0.5f * real_acc * dt_diff * dt_diff);
        osDelay(1);
        nt = HAL_GetTick();
        dt = (float)(nt - st) * 0.001f;
    }

    // 後処理
    target->vel = 0.0f;
    target->pos = target_pos;
}

/* ========================================================
 *            Modular CANLib Sys向けの関数たち
 * ======================================================== */

bool ModularCANLib_RobStride_IsRobStrideID_RX(const ModularCANLib_CANId_Type can_id_type, const uint32_t can_id) {
    if (can_id_type != ModularCANLib_CAN_ID_TYPE_EXT) return false;
    if (size_robStride_AllDevices == 0) return false;

    if (can_id >= 0x00FE && can_id <= 0x7FFE) return true;
    if (can_id >= 0x02000000 && can_id <= 0x02C07F7F) return true;
    if (can_id >= 0x11000000 && can_id <= 0x11007F7F) return true;
    if (can_id >= 0x15000000 && can_id <= 0x15007F7F) return true;
    return false;
}

void ModularCANLib_RobStride_ProcessRxData(const CAN_HandleTypeDef* hcan, const CAN_RxMsg* rx_msg) {
    uint8_t motor_id = 0;
    const uint32_t ExtId = rx_msg->Header.ExtId;
    if (ExtId >= 0x00FE && ExtId <= 0x7FFE) {
        motor_id = (uint8_t)(ExtId >> 8);
        //    printf("response1 from motor from %d\n\r", (int)motor_id);
        Get_Robstride_MCUID(hcan, rx_msg->Data, motor_id);
    } else if (ExtId >= 0x02000000 && ExtId <= 0x02C07F7F) {
        // uint32_t master_id = (uint8_t)(ExtId & 0xFF);
        motor_id = (uint8_t)((ExtId >> 8) & 0xFF);
        Robstride_set_fb_data_raw(hcan, ExtId, rx_msg->Data, motor_id);
        //    printf("response2 from motor from %d to %d\n\r", (int)motor_id, (int)master_id);
    } else if (ExtId >= 0x11000000 && ExtId <= 0x11007F7F) {
        // uint32_t master_id = (uint8_t)(ExtId & 0xFF);
        motor_id = (uint8_t)((ExtId >> 8) & 0xFF);
        Robstride_ProcessParameter(hcan, rx_msg->Data, motor_id);
        // printf("response3 from motor from %d to %d\n\r", (int)motor_id, (int)master_id);
    } else if (ExtId >= 0x15000000 && ExtId <= 0x15007F7F) {
        // uint32_t master_id = (uint8_t)((ExtId >> 8)) & 0xFF;
        motor_id = (uint8_t)(ExtId & 0xFF);
        Robstride_ProcessFault(rx_msg->Data, motor_id);
        // printf("response4 from motor from %d to %d\n\r", (int)motor_id, (int)master_id);
    } else {
        printf("No such response\n\r");
    }
}

void ModularCANLib_RobStride_DeviceParam_Struct_Init(ModularCANLib_DeviceParam_RobStride_Type *device_param) {
    // target 初期化
    // TODO 関数化
    ModularCANLib_RobStride_Target_Type *target = &device_param->target;
    target->pos = 0.0f;
    target->vel = 0.0f;
    target->kp = 0.0f;
    target->kd = 0.0f;
    target->torque_ff = 0.0f;

    device_param->_enable_flag = 0;                // 有効フラグの初期値 (無効)
}

void ModularCANLib_RobStride_Device_Init(ModularCANLib_DeviceInfo_Type *device_info) {
    Robstride_fb_init(&device_info->device_param.robstride_param);
    const uint8_t can_idx = ModularCANLib_hcan2idx(device_info->hcan);
    table_MotorID_To_DeviceInfoPtr[can_idx][tableSize_MotorID_To_DeviceInfoPtr[can_idx]].id = device_info->node_id;
    table_MotorID_To_DeviceInfoPtr[can_idx][tableSize_MotorID_To_DeviceInfoPtr[can_idx]].dev_info = device_info;
    tableSize_MotorID_To_DeviceInfoPtr[can_idx]++;
    robStride_AllDevices[size_robStride_AllDevices] = device_info;
    size_robStride_AllDevices++;
}

void ModularCANLib_RobStride_AfterDeviceInit() {
    /* definition and creation of robstTimer */
    osTimerStaticDef(robstTimer, ModularCANLib_RobStrideTimerCallback, &modularCANLib_RobStrideTimerControlBlock);
    modularCANLib_RobStrideTimerHandle = osTimerCreate(osTimer(robstTimer), osTimerPeriodic, NULL);

    MODULARCANLIB_PRINTF_DEBUG("robomasTimer osTimerStart\n");
    osTimerStart(modularCANLib_RobStrideTimerHandle, 1); // 1 kHz
}

bool ModularCANLib_RobStride_IsDeviceConnected(const ModularCANLib_DeviceInfo_Type *const device_info) {
    return device_info->device_param.robstride_param.feedback.get_flag;
}

bool ModularCANLib_RobStride_AllDevice_Connected() {
    for (int i = 0; i < size_robStride_AllDevices; i++) {
        const ModularCANLib_DeviceInfo_Type *const dev_info = robStride_AllDevices[i];
        const uint8_t dummy[8] = {0};
        Robstride_SendBytes(dev_info->hcan, 1, 0, 0, dummy, 8);
    }
    osDelay(2); // 応答まち
    for (int i = 0; i < size_robStride_AllDevices; i++) {
        const ModularCANLib_DeviceInfo_Type *const dev_info = robStride_AllDevices[i];
        if (!dev_info->device_param.robstride_param.feedback.get_flag) return false;
    }
    return true;
}