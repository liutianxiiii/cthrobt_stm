//
// Created by tomoya on 26/04/28.
//

#ifndef MODULARCANLIB_ROBSTRIDE_H
#define MODULARCANLIB_ROBSTRIDE_H

#include "ModularCANLib_DeviceInfo_Def.h"
#include "ModularCANLib_Sys.h"

void Robstride_PresetParameters(ModularCANLib_DeviceInfo_Type *dev_info);

void Robstride_SetRunModeToOperation(ModularCANLib_DeviceInfo_Type *dev_info);

void Robstride_SetVelocityLimit(ModularCANLib_DeviceInfo_Type *dev_info);

void Robstride_SetCurrentLimit(ModularCANLib_DeviceInfo_Type *dev_info);

void Robstride_SetTorqueLimit(ModularCANLib_DeviceInfo_Type *dev_info);

void Robstride_Calibration(ModularCANLib_DeviceInfo_Type *device_info, float calib_velocity, ModularCANLib_RobStride_SwitchType sw_type, GPIO_TypeDef *limit_port, uint32_t limit_pin);

void Robstride_SendTarget(const ModularCANLib_DeviceInfo_Type *device_info);

void Robstride_ControlEnable(ModularCANLib_DeviceInfo_Type *dev_info);

void Robstride_ControlDisable(ModularCANLib_DeviceInfo_Type *dev_info);

void Robstride_CheckActiveReportStatus(ModularCANLib_DeviceInfo_Type *device_info);

void Robstride_RequestAllParameters(ModularCANLib_DeviceInfo_Type *device_info);

void Robstride_PrintAllParameters(const ModularCANLib_RobStride_FeedbackData_Type *fb_data);

void Robstride_Debug_Check_All_Parameters(ModularCANLib_DeviceInfo_Type *device_info);

void Robstride_fb_init(ModularCANLib_DeviceParam_RobStride_Type *device_param);

float Robstride_MeasureCoefficient_Torque2Acc(ModularCANLib_DeviceInfo_Type *device_info, float torque, float speed);

void Robstride_MoveWithRamp(ModularCANLib_DeviceInfo_Type *device_info, float target_pos, float max_speed, float acc);

/* ========================================================
 *            Modular CANLib Sys向けの関数たち
 * ======================================================== */

bool ModularCANLib_RobStride_IsRobStrideID_RX(ModularCANLib_CANId_Type can_id_type, uint32_t can_id);

void ModularCANLib_RobStride_ProcessRxData(const CAN_HandleTypeDef* hcan, const CAN_RxMsg* rx_msg);

void ModularCANLib_RobStride_DeviceParam_Struct_Init(ModularCANLib_DeviceParam_RobStride_Type *device_param);

void ModularCANLib_RobStride_Device_Init(ModularCANLib_DeviceInfo_Type *device_info);

void ModularCANLib_RobStride_AfterDeviceInit();

bool ModularCANLib_RobStride_IsDeviceConnected(const ModularCANLib_DeviceInfo_Type *device_info);

bool ModularCANLib_RobStride_AllDevice_Connected();


#endif //MODULARCANLIB_ROBSTRIDE_H
