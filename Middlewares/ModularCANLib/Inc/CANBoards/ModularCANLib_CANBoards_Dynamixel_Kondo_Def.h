/*
 * CAN_Dynamixel_Def.h
 *
 *  Created on: Nov 21, 2024
 *      Author: Akitomo KURAKU
 */

#ifndef INC_CAN_DYNAMIXEL_KONDO_DEF_H_
#define INC_CAN_DYNAMIXEL_KONDO_DEF_H_

// Includes --------------------------------

#include <stdint.h>
#include "ModularCANLib_Pid.h"

// Enumerates --------------------------------

typedef enum{
    DYNAMIXEL_MODEL_EX_106_PLUS,
    DYNAMIXEL_MODEL_MX_28,
    DYNAMIXEL_MODEL_MX_64,
    DYNAMIXEL_MODEL_MX_106,
    DYNAMIXEL_MODEL_DX_117,
    DYNAMIXEL_MODEL_RX_24F,
    DYNAMIXEL_MODEL_RX_64,
    DYNAMIXEL_MODEL_XM430_W350_R,
    DYNAMIXEL_MODEL_XH430_V350_R,
    DYNAMIXEL_MODEL_H42_20_S300_R,
}ModularCANLib_Dynamixel_MODEL; // DynamixelのModel Number

typedef enum{
    DYNAMIXEL_FB_ENABLE = 0,
    DYNAMIXEL_FB_DISABLE = 1
}ModularCANLib_Dynamixel_FB;  // MainマイコンにFeedbackを送るか否か

typedef enum{
    // Dynamixel
    DYNAMIXEL_FB_POS, // 位置
    DYNAMIXEL_FB_VEL, // 速度
    DYNAMIXEL_FB_CUR, // 電流
    DYNAMIXEL_FB_MOV, // 動いているかどうか
    DYNAMIXEL_FB_VOL, // Dynamixel の電源電圧
    DYNAMIXEL_FB_PWM, // PWM
    DYNAMIXEL_FB_TMP, // 温度
    DYNAMIXEL_FB_LOA, // 負荷

    // Kondo
    KONDO_FB_POS = DYNAMIXEL_FB_POS, // 位置
    KONDO_FB_VEL = DYNAMIXEL_FB_VEL, // 速度
    KONDO_FB_CUR = DYNAMIXEL_FB_CUR, // 電流
    KONDO_FB_VOL = DYNAMIXEL_FB_VOL, // Kondo の電源電圧
    KONDO_FB_TMP = DYNAMIXEL_FB_TMP, // 温度

}ModularCANLib_Dynamixel_Kondo_FB_TYPE; // Dynamixel_Kondo 基板 が Main に送る Feedback の種類

typedef enum{
    DYNAMIXEL_CTRL_POS,
    DYNAMIXEL_CTRL_VEL,
    DYNAMIXEL_CTRL_CUR,
    DYNAMIXEL_CTRL_EXPOS,
    DYNAMIXEL_CTRL_CUPOS,
    DYNAMIXEL_CTRL_PWM,
}ModularCANLib_Dynamixel_CTRL_TYPE;  // Dynamixelの制御タイプ

typedef enum{
    DYNAMIXEL_MOV = 1,
    DYNAMIXEL_STOP = 0
}ModularCANLib_Dynamixel_MOV; // モータの動作中/停止中

typedef enum{
    DYNAMIXEL_DIR_FW = 0,
    DYNAMIXEL_DIR_BC = 1
}ModularCANLib_Dynamixel_DIR;  // モーターの正転方向

typedef enum{
    DYNAMIXEL_KONDO_STATUS_INIT,
    DYNAMIXEL_KONDO_STATUS_ENABLE,
    DYNAMIXEL_KONDO_STATUS_DISABLE,
    DYNAMIXEL_KONDO_STATUS_CHANGE_CTRL,
}ModularCANLib_Dynamixel_Kondo_Status;  // Dynamixel / Kondoの状態

typedef enum {
    KONDO_FB_ENABLE = 0,
    KONDO_FB_DISABLE = 1
} ModularCANLib_Kondo_FB; // Main マイコンに Feedback を送るか否か

typedef enum {
    KONDO_CTRL_POS,
    KONDO_CTRL_VEL,
    KONDO_CTRL_TOR,
} ModularCANLib_Kondo_CTRL_TYPE; // Kondo の制御タイプ

typedef enum {
    KONDO_TRAJECTORY_NORMAL = 0,
    KONDO_TRAJECTORY_EVEN = 1,
    KONDO_TRAJECTORY_THIRDPOLY = 3,
    KONDO_TRAJECTORY_FORTHPOLY = 4,
    KONDO_TRAJECTORY_FIFTHPOLY = 5,
} ModularCANLib_Kondo_TRAJECTORY_TYPE; // Kondo の軌道生成タイプ

typedef enum DYNAMIXEL_OR_KONDO {
    DKBOARD_UNINITIALIZED,
    DKBOARD_INITIALIZED_AS_DYNAMIXEL,
    DKBOARD_INITIALIZED_AS_KONDO,
} ModularCANLib_DYNAMIXEL_OR_KONDO;

// Structs --------------------------------

typedef struct{  // 制御パラメータたち
    ModularCANLib_Dynamixel_CTRL_TYPE ctrl_type; // 制御方法
    ModularCANLib_PidParam_Type PID_POS_param; // 位置PIDのパラメータ（未実装）
    ModularCANLib_PidParam_Type PID_VEL_param; // 速度PIDのパラメータ（未実装）
    float profile_acceleration; // プロファイルの最大加速度
    float profile_velocity; // プロファイルの最大速度
    ModularCANLib_Dynamixel_FB feedback; // フィードバックをするか否か
}ModularCANLib_Dynamixel_Control_Param;

typedef struct{  // 制御パラメータたち
    ModularCANLib_Kondo_CTRL_TYPE ctrl_type;             // 制御方法
    ModularCANLib_PidParam_Type PID_POS_param;       // 位置 PID のパラメータ（未実装）
    ModularCANLib_PidParam_Type PID_VEL_param;       // 速度 PID のパラメータ（未実装）
    ModularCANLib_Kondo_FB feedback;                     // フィードバックをするか否か
    ModularCANLib_Kondo_TRAJECTORY_TYPE trajectory_type; // 軌道生成タイプ
}ModularCANLib_Kondo_Control_Param;

typedef struct{
    // 64bitまで
    float value;  // 値
    ModularCANLib_Dynamixel_Kondo_FB_TYPE fb_type;  // feedbackのタイプ
    ModularCANLib_Dynamixel_Kondo_Status status;  // Dynamixel / Kondo の状態
}ModularCANLib_CANDynamixel_Kondo_Feedback_Type;  // 実際のFeedback時に送信する構造体


typedef struct{
    uint8_t device_num;
    ModularCANLib_Dynamixel_MODEL model; // Dynamixelの型番（Kondo 時は意味なし）
    uint8_t id; // Dynamixel / Kondo に設定されているID

    ModularCANLib_Dynamixel_Kondo_FB_TYPE fb_type; // どのパラメータをMainに送るのか

    ModularCANLib_Dynamixel_DIR rot_dir; // 正転／逆転（Kondo 時は意味なし）

    float quant_per_degree; // 1°に対して, 何倍の値を使いたいか

    ModularCANLib_Dynamixel_Control_Param ctrl_param_dynamixel; // 制御パラメータたち（上で定義）
    ModularCANLib_Kondo_Control_Param ctrl_param_kondo; // 制御パラメータたち（上で定義）

    ModularCANLib_CANDynamixel_Kondo_Feedback_Type _feedback;
    bool _get_flag;
}ModularCANLib_DeviceParam_CANBoards_Dynamixel_Kondo_Type;

// Enumerates --------------------------------

typedef enum CANDynamixel_Kondo_Main_CMD{
    // カッコ内は中身のデータのbit数（1コマンド当たり64bitまで）
    // コマンドの種類は32種類まで

    DYNAMIXEL_KONDO_CMD_AWAKE = 0,
    DYNAMIXEL_KONDO_CMD_FB = 1, // Dynamixel_Kondo_Feedback_Typedef(48)

    // Dynamixel の場合 / Kondo の場合
    DYNAMIXEL_KONDO_CMD_INIT1,        // d_or_k(8), model(8), id(8), rot_dir(8) / d_or_k(8), id(8)
    DYNAMIXEL_KONDO_CMD_INIT2,        // quant_per_degree(32) / quant_per_degree(32)
    DYNAMIXEL_KONDO_CMD_CHANGE_CTRL1, // vel_ki(32), vel_kp(32) / empty(0)
    DYNAMIXEL_KONDO_CMD_CHANGE_CTRL2, // pos_kd(32), pos_ki(32) / empty(0)
    DYNAMIXEL_KONDO_CMD_CHANGE_CTRL3, // pos_kp(32) / empty(0)
    DYNAMIXEL_KONDO_CMD_CHANGE_CTRL4, // ctrl_type(8), feedback(8), fb_type(8) / ctrl_type(8), feedback(8), fb_type(8)
    DYNAMIXEL_KONDO_CMD_CHANGE_CTRL5, // profile_acceleration(32), profile_velocity(32) / trajectory_type(8)

    DYNAMIXEL_KONDO_CMD_ENABLE,
    DYNAMIXEL_KONDO_CMD_DISABLE,
    DYNAMIXEL_KONDO_CMD_SET_TARGET, // float(32) / target(32), time[s](32)
}ModularCANLib_CANDynamixel_Kondo_CMD; // CANで送受信するコマンド



#endif /* INC_CAN_DYNAMIXEL_KONDO_DEF_H_ */
