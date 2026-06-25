/*
 * CAN_Main.c
 *
 *  Created on: Feb 6, 2020
 *      Author: Eater
 */
#include "CANBoards/ModularCANLib_CANBoards.h"
#include "main.h"
#include "ModularCANLib_Def.h"
#include "ModularCANLib_DeviceInfo_Def.h"
#include "ModularCANLib_DeviceInfo_Lookup.h"
#include "ModularCANLib_Sys.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "cmsis_os.h"


uint32_t detected_node_id_list[NUM_OF_CANS][ModularCANLib_DeviceType_SUM] = {0};

ModularCANLib_DeviceInfo_Type *list_MCMD_dev_info[NUM_OF_CANS][MODULARCANLIB_MAX_NUM_OF_ONE_DEVICE] = {NULL};
uint32_t listSize_MCMD_dev_info[NUM_OF_CANS] = {0};
ModularCANLib_DeviceInfo_Type *list_Dynamixel_Kondo_dev_info[NUM_OF_CANS][MODULARCANLIB_MAX_NUM_OF_ONE_DEVICE] = {NULL};
uint32_t listSize_Dynamixel_Kondo_dev_info[NUM_OF_CANS] = {0};

static ModularCANLib_DYNAMIXEL_OR_KONDO dynamixel_kondo_board_initialized_type[MODULARCANLIB_MAX_NUM_OF_ONE_DEVICE] = {}; // [i]: node_id が i である Dynamixel_Kondo 基板がどのタイプに初期化されたか

bool flag_canboards_are_used = false;


//// MCMD
void MCMD_ChangeControl(const ModularCANLib_DeviceInfo_Type *const device_info){ // Ctrl typeを変更する.
    const ModularCANLib_DeviceParam_CANBoards_MCMD_Type *const hmcmd = &device_info->device_param.mcmd_param;
    float fdata[2];
    fdata[0] = hmcmd->ctrl_param.PID_param.kp;
    fdata[1] = hmcmd->ctrl_param.PID_param.ki;
    uint32_t can_id = Make_CAN_ID_from_CAN_Device(device_info, MCMD_CMD_CHANGE_CTRL1);
    ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, (uint8_t *)&fdata, sizeof(fdata));

    fdata[0] = hmcmd->ctrl_param.PID_param.kd;
    fdata[1] = hmcmd->ctrl_param.accel_limit_size;
    can_id = Make_CAN_ID_from_CAN_Device(device_info, MCMD_CMD_CHANGE_CTRL2);
    ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, (uint8_t *)&fdata, sizeof(fdata));

    fdata[0] = hmcmd->ctrl_param.PID_param.kff;
    fdata[1] = hmcmd->ctrl_param.gravity_compensation_gain;
    can_id = Make_CAN_ID_from_CAN_Device(device_info, MCMD_CMD_CHANGE_CTRL3);
    ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, (uint8_t *)&fdata, sizeof(fdata));

    uint8_t bdata[6];
    bdata[0] = hmcmd->ctrl_param.ctrl_type;
    bdata[1] = hmcmd->ctrl_param.accel_limit;
    bdata[2] = hmcmd->ctrl_param.feedback;
    bdata[3] = hmcmd->ctrl_param.timup_monitor;
    bdata[4] = hmcmd->fb_type;
    bdata[5] = hmcmd->ctrl_param.gravity_compensation; // TODO : new
    can_id = Make_CAN_ID_from_CAN_Device(device_info, MCMD_CMD_CHANGE_CTRL4);
    ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, bdata, sizeof(bdata));
}

void MCMD_init(const ModularCANLib_DeviceInfo_Type *const device_info){
    const ModularCANLib_DeviceParam_CANBoards_MCMD_Type *const hmcmd = &device_info->device_param.mcmd_param;
    uint8_t bdata[4];
    bdata[0] = hmcmd->enc_dir;
    bdata[1] = hmcmd->rot_dir;
    bdata[2] = hmcmd->calib;
    bdata[3] = hmcmd->limit_sw_type;
    uint32_t can_id = Make_CAN_ID_from_CAN_Device(device_info, MCMD_CMD_INIT1);
    ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, bdata, sizeof(bdata));

    float fdata[2];
    fdata[0] = hmcmd->offset;
    fdata[1] = hmcmd->calib_duty;
    can_id = Make_CAN_ID_from_CAN_Device(device_info, MCMD_CMD_INIT2);
    ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, (uint8_t *)&fdata, sizeof(fdata));

    fdata[0] = hmcmd->quant_per_unit;
    fdata[1] = 0;
    can_id = Make_CAN_ID_from_CAN_Device(device_info, MCMD_CMD_INIT3);
    ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, (uint8_t *)&fdata, sizeof(fdata));
    osDelay(50); // これないと動かない(なぜ?)
    MCMD_ChangeControl(device_info);
}

void MCMD_Calib(const ModularCANLib_DeviceInfo_Type *const device_info){
    uint8_t bdata[4];
    const uint32_t can_id = Make_CAN_ID_from_CAN_Device(device_info, MCMD_CMD_CALIB);
    ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, bdata, sizeof(bdata));
}

void MCMD_Wait_For_Calib(const ModularCANLib_DeviceInfo_Type *const device_info){
    while(Get_MCMD_Feedback(device_info).end_calib!=1){
        osDelay(10);
    }
    printf("Calibration End\r\n");
}

void MCMD_Control_Enable(const ModularCANLib_DeviceInfo_Type *const device_info){
    uint8_t bdata[4];
    const uint32_t can_id = Make_CAN_ID_from_CAN_Device(device_info, MCMD_CMD_ENABLE);
    ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, bdata, sizeof(bdata));
}

void MCMD_Control_Disable(const ModularCANLib_DeviceInfo_Type *const device_info){
    uint8_t bdata[4];
    const uint32_t can_id = Make_CAN_ID_from_CAN_Device(device_info, MCMD_CMD_DISABLE);
    ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, bdata, sizeof(bdata));
}

void MCMD_SetTarget(const ModularCANLib_DeviceInfo_Type *const device_info, const float target){
    const uint32_t can_id = Make_CAN_ID_from_CAN_Device(device_info, MCMD_CMD_SET_TARGET);
    ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, (uint8_t *)&target, sizeof(target));
}

ModularCANLib_MCMD_Feedback_Type Get_MCMD_Feedback(const ModularCANLib_DeviceInfo_Type *const device_info){
    if (device_info->device_type != ModularCANLib_DeviceType_CANBoards_MCMD4){
        printf("Get_MCMD_Feedback() error\n\r");
        const ModularCANLib_MCMD_Feedback_Type null_feedback = {0, 0, 0, 0};
        return null_feedback;
    }
    return device_info->device_param.mcmd_param._feedback;
}

////servo
void ServoDriver_Init(const ModularCANLib_DeviceInfo_Type *const device_info){
    const ModularCANLib_DeviceParam_CANBoards_Servo_Type *const param = &device_info->device_param.servo_param;
    float fdata[2];
    fdata[0] = param->pulse_width_min;
    fdata[1] = param->pulse_width_max;
    uint32_t can_id = Make_CAN_ID_from_CAN_Device(device_info, SERVO_CMD_INIT1);
    ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, (uint8_t *)&fdata, sizeof(fdata));

    fdata[0] = param->pwm_frequency;
    fdata[1] = param->angle_range;
    can_id = Make_CAN_ID_from_CAN_Device(device_info, SERVO_CMD_INIT2);
    ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, (uint8_t *)&fdata, sizeof(fdata));

    fdata[0] = param->angle_offset;
    fdata[1] = 0.0f;
    can_id = Make_CAN_ID_from_CAN_Device(device_info, SERVO_CMD_INIT3);
    ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, (uint8_t *)&fdata, sizeof(fdata));
}

void ServoDriver_SendValue(const ModularCANLib_DeviceInfo_Type *const device_info, const float angle){
    const uint32_t can_id = Make_CAN_ID_from_CAN_Device(device_info, SERVO_CMD_SET_TARGET);
    if (ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, (uint8_t *)(&angle), sizeof(float)) != HAL_OK){
        Error_Handler();
    }
}

/* AirCylinderは初期化の際に, ON状態, もしくはOFF状態のどちらにするかは
 * 機構の初期状態に依存するので, 最初にON or OFFを指定してあげないといけない.
 * 従って, Air_Cylinder()とAirCylinder_SendOutput()の中身は実質同じとなっているが,
 * 初期化するという操作をすべてのデバイスに共通のものとして導入するために,
 * 異なる関数名を用いて明示的にしてある.
 * また, AirCylinderの基板の方にも, 最初は必ず初期化処理(AIR_CMD_INIT)が来るものとして定義してある.
 */

////AirCylinder
void AirCylinder_Init(const ModularCANLib_DeviceInfo_Type *const device_info, const ModularCANLib_Air_PortStatus_Typedef param){
    const uint32_t can_id = Make_CAN_ID_from_CAN_Device(device_info, AIR_CMD_INIT);
    if (ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, (uint8_t *)(&param), sizeof(ModularCANLib_Air_PortStatus_Typedef)) != HAL_OK){
        Error_Handler();
    }
}

void AirCylinder_SendOutput(const ModularCANLib_DeviceInfo_Type *const device_info, const ModularCANLib_Air_PortStatus_Typedef param){
    const uint32_t can_id = Make_CAN_ID_from_CAN_Device(device_info, AIR_CMD_OUTPUT);
    if (ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, (uint8_t *)(&param), sizeof(ModularCANLib_Air_PortStatus_Typedef)) != HAL_OK){
        Error_Handler();
    }
}

////Dynamixel
void Dynamixel_ChangeControl(const ModularCANLib_DeviceInfo_Type *const device_info){ // 制御パラメータを変更する.
    if(dynamixel_kondo_board_initialized_type[device_info->node_id] == DKBOARD_INITIALIZED_AS_KONDO){
        printf("Dynamixel_Kondo Board No. %u has already been initialized as a Kondo Board!\n", device_info->node_id);
        return;
    }

    const ModularCANLib_DeviceParam_CANBoards_Dynamixel_Kondo_Type *const hdynamixel = &device_info->device_param.dynamixel_kondo_param;

    {
        const float fdata[2] = {
            [0] = hdynamixel->ctrl_param_dynamixel.PID_VEL_param.ki,
            [1] = hdynamixel->ctrl_param_dynamixel.PID_VEL_param.kp
        };
        const uint32_t can_id = Make_CAN_ID_from_CAN_Device(device_info, DYNAMIXEL_KONDO_CMD_CHANGE_CTRL1);
        ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, (uint8_t *)&fdata, sizeof(fdata));
    }

    {
        const float fdata[2] = {
            [0] = hdynamixel->ctrl_param_dynamixel.PID_POS_param.kd,
            [1] = hdynamixel->ctrl_param_dynamixel.PID_POS_param.ki
        };
        const uint32_t can_id = Make_CAN_ID_from_CAN_Device(device_info, DYNAMIXEL_KONDO_CMD_CHANGE_CTRL2);
        ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, (uint8_t *)&fdata, sizeof(fdata));
    }

    {
        const float fdata[2] = {
            [0] = hdynamixel->ctrl_param_dynamixel.PID_POS_param.kp,
            [1] = 0.0f
        };
        const uint32_t can_id = Make_CAN_ID_from_CAN_Device(device_info, DYNAMIXEL_KONDO_CMD_CHANGE_CTRL3);
        ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, (uint8_t *)&fdata, sizeof(fdata));
    }

    {
        const uint8_t bdata[3] = {
            [0] = hdynamixel->ctrl_param_dynamixel.ctrl_type,
            [1] = hdynamixel->ctrl_param_dynamixel.feedback,
            [2] = hdynamixel->fb_type,
        };
        const uint32_t can_id = Make_CAN_ID_from_CAN_Device(device_info, DYNAMIXEL_KONDO_CMD_CHANGE_CTRL4);
        ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, (uint8_t *)&bdata, sizeof(bdata));
    }

    {
        const float fdata[2] = {
            [0] = hdynamixel->ctrl_param_dynamixel.profile_acceleration,
            [1] = hdynamixel->ctrl_param_dynamixel.profile_velocity,
        };
        const uint32_t can_id = Make_CAN_ID_from_CAN_Device(device_info, DYNAMIXEL_KONDO_CMD_CHANGE_CTRL5);
        ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, (uint8_t *)&fdata, sizeof(fdata));
    }
}

void Dynamixel_init(const ModularCANLib_DeviceInfo_Type *const device_info){
    if(dynamixel_kondo_board_initialized_type[device_info->node_id] == DKBOARD_INITIALIZED_AS_KONDO){
        printf("Dynamixel_Kondo Board No. %u has already been initialized as a Kondo Board!\n", device_info->node_id);
        return;
    }else{
        dynamixel_kondo_board_initialized_type[device_info->node_id] = DKBOARD_INITIALIZED_AS_DYNAMIXEL;
    }

    const ModularCANLib_DeviceParam_CANBoards_Dynamixel_Kondo_Type *const hdynamixel = &device_info->device_param.dynamixel_kondo_param;

    {
        const uint8_t bdata[4] = {
            [0] = DKBOARD_INITIALIZED_AS_DYNAMIXEL,
            [1] = hdynamixel->model,
            [2] = hdynamixel->id,
            [3] = hdynamixel->rot_dir,
        };
        const uint32_t can_id = Make_CAN_ID_from_CAN_Device(device_info, DYNAMIXEL_KONDO_CMD_INIT1);
        ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, (uint8_t *)&bdata, sizeof(bdata));
    }

    {
        const float fdata[1] = {
            [0] = hdynamixel->quant_per_degree,
        };
        const uint32_t can_id = Make_CAN_ID_from_CAN_Device(device_info, DYNAMIXEL_KONDO_CMD_INIT2);
        ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, (uint8_t *)&fdata, sizeof(fdata));
    }

    osDelay(50); // これないと動かない(なぜ?) ← MCMD_init()から継承したため詳細不明
    Dynamixel_ChangeControl(device_info);
}

void Dynamixel_Control_Enable(const ModularCANLib_DeviceInfo_Type *const device_info){
    if(dynamixel_kondo_board_initialized_type[device_info->node_id] == DKBOARD_INITIALIZED_AS_KONDO){
        printf("Dynamixel_Kondo Board No. %u has already been initialized as a Kondo Board!\n", device_info->node_id);
        return;
    }

    {
        const uint8_t bdata[4] = {};
        const uint32_t can_id = Make_CAN_ID_from_CAN_Device(device_info, DYNAMIXEL_KONDO_CMD_ENABLE);
        ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, (uint8_t *)&bdata, sizeof(bdata));
    }
}

void Dynamixel_Control_Disable(const ModularCANLib_DeviceInfo_Type *const device_info){
    if(dynamixel_kondo_board_initialized_type[device_info->node_id] == DKBOARD_INITIALIZED_AS_KONDO){
        printf("Dynamixel_Kondo Board No. %u has already been initialized as a Kondo Board!\n", device_info->node_id);
        return;
    }

    {
        const uint8_t bdata[4] = {};
        const uint32_t can_id = Make_CAN_ID_from_CAN_Device(device_info, DYNAMIXEL_KONDO_CMD_DISABLE);
        ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, (uint8_t *)&bdata, sizeof(bdata));
    }
}

void Dynamixel_SetTarget(const ModularCANLib_DeviceInfo_Type *const device_info, const float target){
    if(dynamixel_kondo_board_initialized_type[device_info->node_id] == DKBOARD_INITIALIZED_AS_KONDO){
        printf("Dynamixel_Kondo Board No. %u has already been initialized as a Kondo Board!\n", device_info->node_id);
        return;
    }

    {
        const uint32_t can_id = Make_CAN_ID_from_CAN_Device(device_info, DYNAMIXEL_KONDO_CMD_SET_TARGET);
        ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, (uint8_t *)&target, sizeof(target));
    }
}

ModularCANLib_CANDynamixel_Kondo_Feedback_Type Get_Dynamixel_Feedback(const ModularCANLib_DeviceInfo_Type *const device_info){
    const ModularCANLib_CANDynamixel_Kondo_Feedback_Type null_feedback = {.fb_type = 0, .status = 0, .value = NAN};

    if (device_info->device_type != ModularCANLib_DeviceType_CANBoards_DYNAMIXEL_KONDO){
        printf("Get_Dynamixel_Kondo_Feedback() error\n\r");
        return null_feedback;
    }

    if(dynamixel_kondo_board_initialized_type[device_info->node_id] == DKBOARD_INITIALIZED_AS_KONDO){
        printf("Dynamixel_Kondo Board No. %u has already been initialized as a Kondo Board!\n", device_info->node_id);
        return null_feedback;
    }

    return device_info->device_param.dynamixel_kondo_param._feedback;
}

////Kondo
void Kondo_ChangeControl(const ModularCANLib_DeviceInfo_Type *const device_info){ // 制御パラメータを変更する.
    if(dynamixel_kondo_board_initialized_type[device_info->node_id] == DKBOARD_INITIALIZED_AS_DYNAMIXEL){
        printf("Dynamixel_Kondo Board No. %u has already been initialized as a Dynamixel Board!\n", device_info->node_id);
        return;
    }

    const ModularCANLib_DeviceParam_CANBoards_Dynamixel_Kondo_Type *const hkondo = &device_info->device_param.dynamixel_kondo_param;

    {
        const float fdata[2] = {};
        const uint32_t can_id = Make_CAN_ID_from_CAN_Device(device_info, DYNAMIXEL_KONDO_CMD_CHANGE_CTRL1);
        ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, (uint8_t *)&fdata, sizeof(fdata));
    }

    {
        const float fdata[2] = {};
        const uint32_t can_id = Make_CAN_ID_from_CAN_Device(device_info, DYNAMIXEL_KONDO_CMD_CHANGE_CTRL2);
        ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, (uint8_t *)&fdata, sizeof(fdata));

    }

    {
        const float fdata[2] = {};
        const uint32_t can_id = Make_CAN_ID_from_CAN_Device(device_info, DYNAMIXEL_KONDO_CMD_CHANGE_CTRL3);
        ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, (uint8_t *)&fdata, sizeof(fdata));
    }
    
    {
        const uint8_t bdata[3] = {
            [0] = hkondo->ctrl_param_kondo.ctrl_type,
            [1] = hkondo->ctrl_param_kondo.feedback,
            [2] = hkondo->fb_type,
        };
        const uint32_t can_id = Make_CAN_ID_from_CAN_Device(device_info, DYNAMIXEL_KONDO_CMD_CHANGE_CTRL4);
        ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, (uint8_t *)&bdata, sizeof(bdata));
    }

    {
        const uint8_t bdata[1] = {
            [0] = hkondo->ctrl_param_kondo.trajectory_type,
        };
        const uint32_t can_id = Make_CAN_ID_from_CAN_Device(device_info, DYNAMIXEL_KONDO_CMD_CHANGE_CTRL5);
        ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, (uint8_t *)&bdata, sizeof(bdata));
    }

    
}

void Kondo_init(const ModularCANLib_DeviceInfo_Type *const device_info){
    if(dynamixel_kondo_board_initialized_type[device_info->node_id] == DKBOARD_INITIALIZED_AS_DYNAMIXEL){
        printf("Dynamixel_Kondo Board No. %u has already been initialized as a Dynamixel Board!\n", device_info->node_id);
        return;
    }else{
        dynamixel_kondo_board_initialized_type[device_info->node_id] = DKBOARD_INITIALIZED_AS_KONDO;
    }

    const ModularCANLib_DeviceParam_CANBoards_Dynamixel_Kondo_Type *const hkondo = &device_info->device_param.dynamixel_kondo_param;

    {
        const uint8_t bdata[2] = {
            [0] = DKBOARD_INITIALIZED_AS_KONDO,
            [1] = hkondo->id,
        };
        const uint32_t can_id = Make_CAN_ID_from_CAN_Device(device_info, DYNAMIXEL_KONDO_CMD_INIT1);
        ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, (uint8_t *)&bdata, sizeof(bdata));
    }
    
    {
        float fdata[1] = {
            [0] = hkondo->quant_per_degree,
        };
        const uint32_t can_id = Make_CAN_ID_from_CAN_Device(device_info, DYNAMIXEL_KONDO_CMD_INIT2);
        ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, (uint8_t *)&fdata, sizeof(fdata));
    }
    

    osDelay(50); // これないと動かない(なぜ?) ← MCMD_init()から継承したため詳細不明
    Kondo_ChangeControl(device_info);
}

void Kondo_Control_Enable(const ModularCANLib_DeviceInfo_Type *const device_info){
    if(dynamixel_kondo_board_initialized_type[device_info->node_id] == DKBOARD_INITIALIZED_AS_DYNAMIXEL){
        printf("Dynamixel_Kondo Board No. %u has already been initialized as a Dynamixel Board!\n", device_info->node_id);
        return;
    }

    {
        const uint8_t bdata[4] = {};
        const uint32_t can_id = Make_CAN_ID_from_CAN_Device(device_info, DYNAMIXEL_KONDO_CMD_ENABLE);
        ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, (uint8_t *)&bdata, sizeof(bdata));
    }
}

void Kondo_Control_Disable(const ModularCANLib_DeviceInfo_Type *const device_info){
    if(dynamixel_kondo_board_initialized_type[device_info->node_id] == DKBOARD_INITIALIZED_AS_DYNAMIXEL){
        printf("Dynamixel_Kondo Board No. %u has already been initialized as a Dynamixel Board!\n", device_info->node_id);
        return;
    }

    {
        const uint8_t bdata[4] = {};
        const uint32_t can_id = Make_CAN_ID_from_CAN_Device(device_info, DYNAMIXEL_KONDO_CMD_DISABLE);
        ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, (uint8_t *)&bdata, sizeof(bdata));
    }
}

void Kondo_SetTarget(const ModularCANLib_DeviceInfo_Type *const device_info, const float target){
    if(dynamixel_kondo_board_initialized_type[device_info->node_id] == DKBOARD_INITIALIZED_AS_DYNAMIXEL){
        printf("Dynamixel_Kondo Board No. %u has already been initialized as a Dynamixel Board!\n", device_info->node_id);
        return;
    }

    {
        const uint32_t can_id = Make_CAN_ID_from_CAN_Device(device_info, DYNAMIXEL_KONDO_CMD_SET_TARGET);
        ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, (uint8_t *)&target, sizeof(target));
    }
}

void Kondo_SetTarget_With_Time(const ModularCANLib_DeviceInfo_Type *const device_info, const float target, const float time_sec){
    if(dynamixel_kondo_board_initialized_type[device_info->node_id] == DKBOARD_INITIALIZED_AS_DYNAMIXEL){
        printf("Dynamixel_Kondo Board No. %u has already been initialized as a Dynamixel Board!\n", device_info->node_id);
        return;
    }

    {
        const float fdata[2] = {
            [0] = target,
            [1] = time_sec,
        };
        const uint32_t can_id = Make_CAN_ID_from_CAN_Device(device_info, DYNAMIXEL_KONDO_CMD_SET_TARGET);
        ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_EXT, (uint8_t *)&fdata, sizeof(fdata));
    }
}

ModularCANLib_CANDynamixel_Kondo_Feedback_Type Get_Kondo_Feedback(const ModularCANLib_DeviceInfo_Type *const device_info){
    const ModularCANLib_CANDynamixel_Kondo_Feedback_Type null_feedback = {.fb_type = 0, .status = 0, .value = NAN};

    if (device_info->device_type != ModularCANLib_DeviceType_CANBoards_DYNAMIXEL_KONDO){
        printf("Get_Dynamixel_Kondo_Feedback() error\n\r");
        return null_feedback;
    }

    if(dynamixel_kondo_board_initialized_type[device_info->node_id] == DKBOARD_INITIALIZED_AS_DYNAMIXEL){
        printf("Dynamixel_Kondo Board No. %u has already been initialized as a Dynamixel Board!\n", device_info->node_id);
        return null_feedback;
    }

    return device_info->device_param.dynamixel_kondo_param._feedback;
}

static ModularCANLib_DeviceInfo_Type* ModularCANLib_CANBoards_FindDevInfo(const CAN_HandleTypeDef *const hcan, const ModularCANLib_DeviceType_Type device_type, const uint8_t node_id, const uint8_t device_num) {
    const uint8_t can_id = ModularCANLib_hcan2idx(hcan);
    ModularCANLib_DeviceInfo_Type **list_target = NULL; // 可読性(笑)のためだけの変数
    uint32_t listSize = 0; // 可読性(笑)のためだけの変数
    switch (device_type) {
        case ModularCANLib_DeviceType_CANBoards_DYNAMIXEL_KONDO:
            list_target = list_Dynamixel_Kondo_dev_info[can_id];
            listSize = listSize_Dynamixel_Kondo_dev_info[can_id];
            for (uint32_t i = 0; i < listSize; i++) {
                ModularCANLib_DeviceInfo_Type *const dev_info = list_target[i];
                if (dev_info->node_id == node_id && dev_info->device_param.dynamixel_kondo_param.device_num == device_num) return dev_info;
            }
            return NULL;
        case ModularCANLib_DeviceType_CANBoards_MCMD4:
            list_target = list_MCMD_dev_info[can_id];
            listSize = listSize_MCMD_dev_info[can_id];
            for (uint32_t i = 0; i < listSize; i++) {
                ModularCANLib_DeviceInfo_Type *const dev_info = list_target[i];
                if (dev_info->node_id == node_id && dev_info->device_param.mcmd_param.device_num == device_num) return dev_info;
            }
            return NULL;
        default:
            return NULL;
    }
}

static void ModularCANLib_CANBoards_SetGetFlag(ModularCANLib_DeviceInfo_Type* const device_info) {
    switch (device_info->device_type) {
        case ModularCANLib_DeviceType_CANBoards_DYNAMIXEL_KONDO:
            device_info->device_param.dynamixel_kondo_param._get_flag = true;
            break;
        case ModularCANLib_DeviceType_CANBoards_MCMD4:
            device_info->device_param.mcmd_param._get_flag = true;
            break;
        case ModularCANLib_DeviceType_CANBoards_AIR:
            device_info->device_param.air_param._get_flag = true;
            break;
        case ModularCANLib_DeviceType_CANBoards_SERVO:
            device_info->device_param.servo_param._get_flag = true;
            break;
        default:
            break;
    }
}

bool ModularCANLib_CANBoards_IsCANBoardsID_RX(const ModularCANLib_CANId_Type can_id_type, const uint32_t can_id) {
    if (can_id_type != ModularCANLib_CAN_ID_TYPE_EXT) return false;
    if (!flag_canboards_are_used) return false;
    return can_id <= 0x3FFF;
}

void ModularCANLib_CANBoards_ProcessRxData(const CAN_HandleTypeDef* hcan, const CAN_RxMsg* rx_msg) {
    const uint8_t can_id = ModularCANLib_hcan2idx(hcan);
    // IDからデバイスを推定
    const uint8_t device_num = (rx_msg->Header.ExtId >> 5) & 0b111;
    const uint8_t node_id = (rx_msg->Header.ExtId >> 8) & 0b111;
    const ModularCANLib_DeviceType_Type device_type = (ModularCANLib_DeviceType_Type)(((rx_msg->Header.ExtId >> 11) & 0b111) + ModularCANLib_DeviceType_CANBoards_MAIN);
    ModularCANLib_DeviceInfo_Type *const dev_info = ModularCANLib_CANBoards_FindDevInfo(hcan, device_type, node_id, device_num);
    const uint8_t extracted_cmd = rx_msg->Header.ExtId & 0b11111;
    // MODULARCANLIB_PRINTF_DEBUG("device type: %u, id: %u, dnum: %u, cmd: %u\n", device_type, device_num, can_id, extracted_cmd);
    if(extracted_cmd == AWAKE_CMD){
        // awake コマンドを受信した場合
        detected_node_id_list[can_id][device_type] |= 0b1 << rx_msg->Data[0];
        // get_flag更新
        // 複数のDeviceInfoが更新されることに注意
        ModularCANLib_DeviceInfo_Type **device_infos;
        uint32_t size;
        ModularCANLib_GetDeviceInfos_By_DeviceType(device_type, &device_infos, &size);
        for (uint32_t i = 0; i < size; i++) {
            if (device_infos[i]->node_id != node_id) continue;
            ModularCANLib_CANBoards_SetGetFlag(device_infos[i]);
        }
    }else if(extracted_cmd == FB_CMD){
        // feedback コマンドを受信した場合
        if (dev_info == NULL) return;
        if(device_type == ModularCANLib_DeviceType_CANBoards_MCMD4) {
            memcpy(&dev_info->device_param.mcmd_param._feedback, rx_msg->Data, sizeof(ModularCANLib_MCMD_Feedback_Type));
        }else if(device_type == ModularCANLib_DeviceType_CANBoards_DYNAMIXEL_KONDO){
            memcpy(&dev_info->device_param.dynamixel_kondo_param._feedback, rx_msg->Data, sizeof(ModularCANLib_CANDynamixel_Kondo_Feedback_Type));
        }else{
            printf("Error\n\r");
        }
    }
}

void ModularCANLib_CANBoards_MCMD_Device_Init(ModularCANLib_DeviceInfo_Type *const device_info) {
    flag_canboards_are_used = true;
    // listに登録
    const uint8_t can_id = ModularCANLib_hcan2idx(device_info->hcan);
    list_MCMD_dev_info[can_id][listSize_MCMD_dev_info[can_id]] = device_info;
    listSize_MCMD_dev_info[can_id]++;
}

void ModularCANLib_CANBoards_Air_Device_Init(ModularCANLib_DeviceInfo_Type *const device_info) {
    flag_canboards_are_used = true;
}

void ModularCANLib_CANBoards_Servo_Device_Init(ModularCANLib_DeviceInfo_Type *const device_info) {
    flag_canboards_are_used = true;
}

void ModularCANLib_CANBoards_Dynamixel_Kondo_Device_Init(ModularCANLib_DeviceInfo_Type *const device_info) {
    flag_canboards_are_used = true;
    // listに登録
    const uint8_t can_id = ModularCANLib_hcan2idx(device_info->hcan);
    list_Dynamixel_Kondo_dev_info[can_id][listSize_Dynamixel_Kondo_dev_info[can_id]] = device_info;
    listSize_Dynamixel_Kondo_dev_info[can_id]++;
}

void ModularCANLib_CANBoards_AfterDeviceInit() {
    // すべてのInit関数を呼び出し
    // ModularCANLib_DeviceInfo_Type **device_infos;
    // uint32_t size;
    //
    // // Servo
    // ModularCANLib_GetDeviceInfos_By_DeviceType(ModularCANLib_DeviceType_CANBoards_SERVO, &device_infos, &size);
    // for (uint32_t i = 0; i < size; i++) {
    //     ServoDriver_Init(device_infos[i]);
    // }
    //
    // // Air
    // ModularCANLib_GetDeviceInfos_By_DeviceType(ModularCANLib_DeviceType_CANBoards_AIR, &device_infos, &size);
    // for (uint32_t i = 0; i < size; i++) {
    //     for(uint8_t j=PORT_1; j<=PORT_8; j++){  //すべてのポートを初期化しないとAir基板は動かない
    //         device_infos[i]->device_param.air_param.device_num = i; // (i番ポートを指定)
    //         AirCylinder_Init(device_infos[i], AIR_OFF);
    //         osDelay(10);  // このdelayは必要
    //     }
    // }
    //
    // // MCMD4
    // ModularCANLib_GetDeviceInfos_By_DeviceType(ModularCANLib_DeviceType_CANBoards_MCMD4, &device_infos, &size);
    // for (uint32_t i = 0; i < size; i++) {
    //     MCMD_init(device_infos[i]);
    // }
    //
    // // Dynamixel
    // ModularCANLib_GetDeviceInfos_By_DeviceType(ModularCANLib_DeviceType_CANBoards_DYNAMIXEL, &device_infos, &size);
    // for (uint32_t i = 0; i < size; i++) {
    //     Dynamixel_init(device_infos[i]);
    // }
}

bool ModularCANLib_CANBoards_MCMD_IsDeviceConnected(const ModularCANLib_DeviceInfo_Type *const device_info) {
    return device_info->device_param.mcmd_param._get_flag;
}

bool ModularCANLib_CANBoards_Air_IsDeviceConnected(const ModularCANLib_DeviceInfo_Type *const device_info) {
    return device_info->device_param.air_param._get_flag;
}

bool ModularCANLib_CANBoards_Servo_IsDeviceConnected(const ModularCANLib_DeviceInfo_Type *const device_info) {
    return device_info->device_param.servo_param._get_flag;
}

bool ModularCANLib_CANBoards_Dynamixel_Kondo_IsDeviceConnected(const ModularCANLib_DeviceInfo_Type *const device_info) {
    return device_info->device_param.dynamixel_kondo_param._get_flag;
}

bool ModularCANLib_CANBoards_AllDevice_Connected() {
    ModularCANLib_DeviceInfo_Type **device_infos;
    uint32_t size;

    // Servo
    ModularCANLib_GetDeviceInfos_By_DeviceType(ModularCANLib_DeviceType_CANBoards_SERVO, &device_infos, &size);
    for (uint32_t i = 0; i < size; i++) {
        if (!ModularCANLib_CANBoards_Servo_IsDeviceConnected(device_infos[i])) return false;
    }

    // Air
    ModularCANLib_GetDeviceInfos_By_DeviceType(ModularCANLib_DeviceType_CANBoards_AIR, &device_infos, &size);
    for (uint32_t i = 0; i < size; i++) {
        if (!ModularCANLib_CANBoards_Air_IsDeviceConnected(device_infos[i])) return false;
    }

    // MCMD4
    ModularCANLib_GetDeviceInfos_By_DeviceType(ModularCANLib_DeviceType_CANBoards_MCMD4, &device_infos, &size);
    for (uint32_t i = 0; i < size; i++) {
        if (!ModularCANLib_CANBoards_MCMD_IsDeviceConnected(device_infos[i])) return false;
    }

    // Dynamixel
    ModularCANLib_GetDeviceInfos_By_DeviceType(ModularCANLib_DeviceType_CANBoards_DYNAMIXEL_KONDO, &device_infos, &size);
    for (uint32_t i = 0; i < size; i++) {
        if (!ModularCANLib_CANBoards_Dynamixel_Kondo_IsDeviceConnected(device_infos[i])) return false;
    }

    return true;
}

void ModularCANLib_CANBoards_PrintDetection() {
    MODULARCANLIB_PRINTF_INFO("<Detected CANBoards Devices>\n");
    for (uint8_t can_id = 0; can_id < NUM_OF_CANS; can_id++) {
        MODULARCANLIB_PRINTF_INFO("CAN%u:\n", can_id+1);
        // Air
        MODULARCANLIB_PRINTF_INFO("  Air: ");
        for (uint8_t i = 0; i < 32; i++) {
            if ((detected_node_id_list[can_id][ModularCANLib_DeviceType_CANBoards_AIR] & (0b1 << i)) == 0) continue;
            MODULARCANLIB_PRINTF_INFO("No.%u ", i);
        }
        // Servo
        MODULARCANLIB_PRINTF_INFO("\n  Servo: ");
        for (uint8_t i = 0; i < 32; i++) {
            if ((detected_node_id_list[can_id][ModularCANLib_DeviceType_CANBoards_SERVO] & (0b1 << i)) == 0) continue;
            MODULARCANLIB_PRINTF_INFO("No.%u ", i);
        }
        // MCMD
        MODULARCANLIB_PRINTF_INFO("\n  MCMD: ");
        for (uint8_t i = 0; i < 32; i++) {
            if ((detected_node_id_list[can_id][ModularCANLib_DeviceType_CANBoards_MCMD4] & (0b1 << i)) == 0) continue;
            MODULARCANLIB_PRINTF_INFO("No.%u ", i);
        }
        // Dynamixel
        MODULARCANLIB_PRINTF_INFO("\n  Dynamixel_Kondo: ");
        for (uint8_t i = 0; i < 32; i++) {
            if ((detected_node_id_list[can_id][ModularCANLib_DeviceType_CANBoards_DYNAMIXEL_KONDO] & (0b1 << i)) == 0) continue;
            MODULARCANLIB_PRINTF_INFO("No.%u ", i);
        }
        MODULARCANLIB_PRINTF_INFO("\n");
    }
}