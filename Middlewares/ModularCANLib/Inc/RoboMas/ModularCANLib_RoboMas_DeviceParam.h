//
// Created by tomoya on 2026/04/21.
//

#ifndef MODULARCANLIB_ROBOMAS_DEVICEPARAM_H
#define MODULARCANLIB_ROBOMAS_DEVICEPARAM_H

#include "stdbool.h"
#include "can.h"

#include "ModularCANLib_Pid.h"

typedef enum {
    ROBOMAS_CTRL_POS = 0,
    ROBOMAS_CTRL_VEL = 1,
    ROBOMAS_CTRL_CURRENT = 2
} ModularCANLib_DeviceParam_RoboMas_Ctrl_Type;  // C620の制御タイプ


typedef enum {
    ROBOMAS_LIMIT_ENABLE = 0,
    ROBOMAS_LIMIT_DISABLE = 1
} ModularCANLib_DeviceParam_RoboMas_Limit_Type;  // 制限


typedef enum {
    ROBOMAS_USE_OFFSET_POS_DISABLE = 0,
    ROBOMAS_USE_OFFSET_POS_INTERNAL = 1,  // 初期位置を原点とする
    ROBOMAS_USE_OFFSET_POS_CALIB = 2,  // Calibした後の位置を原点とする
} ModularCANLib_DeviceParam_RoboMas_UseOffsetPos_Type;  // M3508自体のencoderのoffset処理を行うか


typedef enum {
    ROBOMAS_ROT_ACW = 0,  // 半時計回り anti-clock-wise
    ROBOMAS_ROT_CW = 1  // 時計回り clock-wise
} ModularCANLib_DeviceParam_RoboMas_Rot_Type;  // 回転方向


typedef enum {
    ROBOMAS_SWITCH_NO = 0, // normally open, default LOW
    ROBOMAS_SWITCH_NC = 1, // normally closed, default HIGH
} ModularCANLib_DeviceParam_RoboMas_Switch_Type;

typedef struct {
    uint8_t _get_counter; // dataを受け取った回数 (offset計算用, max:128)
    int64_t _rot_num;  //回転数
    uint16_t pos;
    uint16_t _internal_offset_pos;  // encoderの初期位置自体のoffset
    uint16_t pos_pre;

    int16_t vel;
    int16_t cur;
} My_ModularCANLib_RoboMas_FeedbackData_Raw_Type;

typedef struct {
    uint8_t device_id;
    uint8_t get_flag;
    float position;
    float velocity;
    float current;
    My_ModularCANLib_RoboMas_FeedbackData_Raw_Type _raw_data;
} ModularCANLib_RoboMas_FeedbackData_Type;

typedef struct
{
    ModularCANLib_PidParam_Type pid_pos, pid_vel;

    ModularCANLib_DeviceParam_RoboMas_Ctrl_Type ctrl_type;
    ModularCANLib_DeviceParam_RoboMas_UseOffsetPos_Type use_internal_offset;
    ModularCANLib_DeviceParam_RoboMas_Limit_Type current_limit;
    ModularCANLib_DeviceParam_RoboMas_Limit_Type velocity_limit;
    ModularCANLib_DeviceParam_RoboMas_Rot_Type rotation;
    float current_limit_size;
    float velocity_limit_size;
    float quant_per_rot;
    float offset_pos;

    // ↓ don't change
    float _target_value;
    float _req_value;
    bool _enable_flag;

    // for calibration
    bool _is_calibrating;
    ModularCANLib_DeviceParam_RoboMas_Switch_Type _sw_type;
    GPIO_TypeDef* _limit_port;
    uint16_t _limit_pin;
    ModularCANLib_DeviceParam_RoboMas_Ctrl_Type _ctrl_type_before_calib; // キャリブレーション開始前の制御モード

    // fb param
    ModularCANLib_RoboMas_FeedbackData_Type feedback;
} ModularCANLib_DeviceParam_RoboMas_Type;

#endif //MODULARCANLIB_ROBOMAS_DEVICEPARAM_H
