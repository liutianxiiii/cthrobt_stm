//
// Created by tomoya on 2026/05/09.
//

#ifndef ODRIVE_DEVICEPARAM_H
#define ODRIVE_DEVICEPARAM_H

#include <stdint.h>
#include <stdbool.h>
#include "can.h"

// ---- CAN ID ----
// STD 11bit: can_id = (node_id << 5) | cmd_id
// node_id を 0x00~0x07 に制限することで他モジュールとのID競合を回避できる（推奨）
#define ODRIVE_NODE_ID_MAX  (0x07u)
#define ODRIVE_CMD_ID_MAX   (0x1Fu)

typedef enum {
    ODRIVE_CMD_ID_GET_VERSION               = 0x00,
    ODRIVE_CMD_ID_HEARTBEAT                 = 0x01,
    ODRIVE_CMD_ID_ESTOP                     = 0x02,
    ODRIVE_CMD_ID_GET_ERROR                 = 0x03,
    ODRIVE_CMD_ID_RXSDO                     = 0x04,
    ODRIVE_CMD_ID_TXSDO                     = 0x05,
    ODRIVE_CMD_ID_ADDRESS                   = 0x06,
    ODRIVE_CMD_ID_SET_AXIS_STATE            = 0x07,
    ODRIVE_CMD_ID_GET_ENCODER_ESTIMATES     = 0x09,
    ODRIVE_CMD_ID_SET_CONTROLLER_MODE       = 0x0B,
    ODRIVE_CMD_ID_SET_INPUT_POS             = 0x0C,
    ODRIVE_CMD_ID_SET_INPUT_VEL             = 0x0D,
    ODRIVE_CMD_ID_SET_INPUT_TORQUE          = 0x0E,
    ODRIVE_CMD_ID_SET_LIMITS                = 0x0F,
    ODRIVE_CMD_ID_SET_TRAJ_VEL_LIMIT        = 0x11,
    ODRIVE_CMD_ID_SET_TRAJ_ACCEL_LIMITS     = 0x12,
    ODRIVE_CMD_ID_SET_TRAJ_INERTIA          = 0x13,
    ODRIVE_CMD_ID_GET_IQ                    = 0x14,
    ODRIVE_CMD_ID_GET_TEMPERATURE           = 0x15,
    ODRIVE_CMD_ID_REBOOT                    = 0x16,
    ODRIVE_CMD_ID_GET_BUS_VOLTAGE_CURRENT   = 0x17,
    ODRIVE_CMD_ID_CLEAR_ERRORS              = 0x18,
    ODRIVE_CMD_ID_SET_ABSOLUTE_POSITION     = 0x19,
    ODRIVE_CMD_ID_SET_POS_GAIN              = 0x1A,
    ODRIVE_CMD_ID_SET_VEL_GAINS             = 0x1B,
    ODRIVE_CMD_ID_GET_TORQUES               = 0x1C,
    ODRIVE_CMD_ID_GET_POWERS                = 0x1D,
    ODRIVE_CMD_ID_ENTER_DFU_MODE            = 0x1F,
} ModularCANLib_ODrive_CmdId;

typedef enum {
    ODRIVE_AXIS_STATE_UNDEFINED                    = 0x00,
    ODRIVE_AXIS_STATE_IDLE                         = 0x01,
    ODRIVE_AXIS_STATE_ENCODER_OFFSET_CALIBRATION   = 0x07,
    ODRIVE_AXIS_STATE_CLOSED_LOOP_CONTROL          = 0x08,
} ModularCANLib_ODrive_AxisState;

typedef enum {
    ODRIVE_INPUT_MODE_PASSTHROUGH = 0x01,
} ModularCANLib_ODrive_InputMode;

// エラービット番号 (1 << bit) でエラーレジスタをマスクする
typedef enum {
    ODRIVE_ERROR_INITIALIZING        = 0,
    ODRIVE_ERROR_SYSTEM_LEVEL        = 1,
    ODRIVE_ERROR_TIMING_ERROR        = 2,
    ODRIVE_ERROR_MISSING_ESTIMATE    = 3,
    ODRIVE_ERROR_BAD_CONFIG          = 4,
    ODRIVE_ERROR_DRV_FAULT           = 5,
    ODRIVE_ERROR_MISSING_INPUT       = 6,
    ODRIVE_ERROR_DC_BUS_OVER_VOLTAGE = 8,
    ODRIVE_ERROR_DC_BUS_UNDER_VOLTAGE= 9,
    ODRIVE_ERROR_DC_BUS_OVER_CURRENT = 10,
    ODRIVE_ERROR_CURRENT_LIMIT_VIOLATION = 12,
    ODRIVE_ERROR_MOTOR_OVER_TEMP     = 13,
    ODRIVE_ERROR_VELOCITY_LIMIT_VIOLATION = 15,
    ODRIVE_ERROR_POSITION_LIMIT_VIOLATION = 16,
    ODRIVE_ERROR_WATCHDOG_TIMER_EXPIRED   = 24,
    ODRIVE_ERROR_ESTOP_REQUESTED          = 25,
    ODRIVE_ERROR_SPINOUT_DETECTED         = 26,
    ODRIVE_ERROR_CALIBRATION_ERROR        = 30,
} ModularCANLib_ODrive_Error;

typedef enum {
    ODRIVE_CTRL_TORQUE = 0x01,
    ODRIVE_CTRL_VEL    = 0x02,
    ODRIVE_CTRL_POS    = 0x03,
} ModularCANLib_ODrive_CtrlType;

typedef enum {
    ODRIVE_USE_OFFSET_POS_INTERNAL = 1, // 起動時位置を原点とする
    ODRIVE_USE_OFFSET_POS_CALIB    = 2, // Calibration後の位置を原点とする
} ModularCANLib_ODrive_UseOffsetPos;

typedef enum {
    ODRIVE_ROT_ACW = 0, // 反時計回り
    ODRIVE_ROT_CW  = 1, // 時計回り
} ModularCANLib_ODrive_Rot;

typedef enum {
    ODRIVE_SWITCH_NO = 0, // normally open
    ODRIVE_SWITCH_NC = 1, // normally closed
} ModularCANLib_ODrive_SwitchType;

// ---- Feedback ----

typedef struct {
    uint8_t  _get_counter; // 受信カウンタ (接続判定用, max:128)
    uint32_t error;
    uint8_t  axis_state;
    float    pos;    // [rev]
    float    vel;    // [rev/s]
    float    torque; // [N m]
} ModularCANLib_ODrive_FeedbackDataRaw_Type;

typedef struct {
    uint8_t  get_flag;
    uint32_t error;
    uint8_t  axis_state;
    float    position; // ユーザー単位 [X]
    float    velocity; // ユーザー単位 [X/s]
    float    torque;   // [N m]
    ModularCANLib_ODrive_FeedbackDataRaw_Type _raw_data;
} ModularCANLib_ODrive_FeedbackData_Type;

// ---- Device Param ----

typedef struct {
    // ユーザーが設定するパラメータ
    float pos_kp;         // 位置Pゲイン [(X/s) / X]
    float vel_kp;         // 速度Pゲイン [N m / (X/s)]
    float vel_ki;         // 速度Iゲイン [N m / X]
    ModularCANLib_ODrive_CtrlType     ctrl_type;
    ModularCANLib_ODrive_UseOffsetPos use_internal_offset;
    ModularCANLib_ODrive_Rot          rotation;
    float velocity_limit; // [X/s]
    float current_limit;  // [A]
    float quant_per_rot;  // [X / rev]  ユーザー単位への換算係数
    float offset_pos;     // [X] 原点オフセット
    bool  is_using_hall_encoder; // true: ホールセンサ使用 (encoder offset calib 不要)

    // 内部変数 (変更しない)
    float _target_value;
    bool  _enable_flag;

    // Calibration用内部変数
    bool _is_calibrating;
    ModularCANLib_ODrive_SwitchType _sw_type;
    GPIO_TypeDef *_limit_port;
    uint32_t      _limit_pin;
    ModularCANLib_ODrive_CtrlType _ctrl_type_before_calib;

    // フィードバック
    ModularCANLib_ODrive_FeedbackData_Type feedback;
} ModularCANLib_DeviceParam_ODrive_Type;

#endif // ODRIVE_DEVICEPARAM_H