//
// Created by tomoya on 26/04/28.
//

#ifndef MODULARCANLIB_ROBSTRIDE_DEVICEPARAM_H
#define MODULARCANLIB_ROBSTRIDE_DEVICEPARAM_H

#include <stdint.h>
#include "ModularCANLib_Pid.h"

typedef enum {
    ROBSTRIDE_VELOCITY_LIMIT_ENABLE = 1,
    ROBSTRIDE_VELOCITY_LIMIT_DISABLE = 0
} ModularCANLib_RobStride_VelocityLimit; // 制限

typedef enum {
    ROBSTRIDE_CURRENT_LIMIT_ENABLE = 1,
    ROBSTRIDE_CURRENT_LIMIT_DISABLE = 0
} ModularCANLib_RobStride_CurrentLimit; // 制限

typedef enum {
    ROBSTRIDE_TORQUE_LIMIT_ENABLE = 1,
    ROBSTRIDE_TORQUE_LIMIT_DISABLE = 0
} ModularCANLib_RobStride_TorqueLimit;

typedef enum {
    ROBSTRIDE_USE_OFFSET_POS_INTERNAL = 0, // 内部で保存された原点を用いる
    ROBSTRIDE_USE_OFFSET_POS_INITIAL = 1,  // 初期位置を原点とする
    ROBSTRIDE_USE_OFFSET_POS_CALIB = 2,    // Calibした後の位置を原点とする
} ModularCANLib_RobStride_UseOffsetPos;

typedef enum {
    ROBSTRIDE_ROT_ACW = 0, // 半時計回り anti-clock-wise
    ROBSTRIDE_ROT_CW = 1   // 時計回り clock-wise
} ModularCANLib_RobStride_Rot;           // 回転方向

typedef enum {
    ROBSTRIDE_SWITCH_NO = 0, // normally open
    ROBSTRIDE_SWITCH_NC = 1, // normally closed
} ModularCANLib_RobStride_SwitchType;

typedef struct {
    uint8_t _get_counter; // dataを受け取った回数 (offset計算用, max:128)
    int64_t _rot_num;     // 回転数
    uint16_t pos;
    uint16_t vel;
    uint16_t torque;
    uint8_t temp;
    uint8_t mcu_id;
} ModularCANLib_RobStride_FeedbackDataRaw_Type;

typedef struct {
    uint8_t device_id;
    uint8_t master_id;
    uint8_t mcu_id;
    uint8_t get_flag;
    float plus_minus;
    float offset_pos;
    float quant_per_rot;
    float position;
    float velocity;
    float current;
    float torque;
    int temperature;

    uint8_t mode_status; // 0:Disable, 1:Calib, 2:Enable

    uint8_t run_mode;     // 0x7005
    float iq_ref;         // 0x7006
    float spd_ref;        // 0x700A
    float limit_torque;   // 0x700B
    float cur_kp;         // 0x7010
    float cur_ki;         // 0x7011
    float cur_filt_gain;  // 0x7014
    float loc_ref;        // 0x7016
    float limit_spd;      // 0x7017
    float limit_cur;      // 0x7018
    float loc_kp;         // 0x701E
    float spd_kp;         // 0x701F
    float spd_ki;         // 0x7020
    float spd_filt_gain;  // 0x7021
    float acc_rad;        // 0x7022
    float vel_max_pp;     // 0x7024 (Position mode PP)
    float acc_set_pp;     // 0x7025 (Position mode PP)
    uint16_t epscan_time; // 0x7026
    uint32_t can_timeout; // 0x7028
    uint8_t zero_sta;     // 0x7029
    float add_offset;     // 0x702B

    // 読み取り専用値
    float vbus; // 0x701C

    // private
    ModularCANLib_RobStride_FeedbackDataRaw_Type _raw_data;
} ModularCANLib_RobStride_FeedbackData_Type;

typedef struct {
    float pos; // 位置目標値
    float vel; // 速度目標値
    float kp; // 位置Pゲイン
    float kd; // 位置Dゲイン=速度Pゲイン
    float torque_ff; // トルクff項 [Nm]
} ModularCANLib_RobStride_Target_Type;

typedef struct {
    ModularCANLib_RobStride_UseOffsetPos use_internal_offset;
    ModularCANLib_RobStride_VelocityLimit velocity_limit;
    ModularCANLib_RobStride_CurrentLimit current_limit;
    ModularCANLib_RobStride_TorqueLimit torque_limit;
    ModularCANLib_RobStride_Rot rotation;
    float offset_pos;
    float velocity_limit_size; // Limit_Spd [rad/s]
    float current_limit_size;  // Limit_Cur [A]
    float torque_limit_size;   // Limit_Torque
    float quant_per_rot;
    ModularCANLib_RobStride_Target_Type target;

    ModularCANLib_RobStride_FeedbackData_Type feedback;
    // ↓ don't change
    float _req_value;
    uint8_t _enable_flag;
} ModularCANLib_DeviceParam_RobStride_Type;

#endif //MODULARCANLIB_ROBSTRIDE_DEVICEPARAM_H
