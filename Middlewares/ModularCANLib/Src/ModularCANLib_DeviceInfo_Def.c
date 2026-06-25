//
// Created by tomoya on 2026/04/21.
//

#include "ModularCANLib_DeviceInfo_Def.h"

#include "ModularCANLib_Def.h"

ModularCANLib_DeviceInfo_Type modularCANLib_DeviceInfos[MODULARCANLIB_MAX_DEVICES] = {0};
uint32_t modularCANLib_DeviceInfosCount = 0;
ModularCANLib_DeviceInfo_Type* modularCANLib_NameTable[MODULARCANLIB_MAX_DEVICES] = {NULL};

ModularCANLib_DeviceParam_RoboMas_Type* ModularCANLib_RoboMas_GetDeviceParam(ModularCANLib_DeviceInfo_Type *const device_info)
{
    if (device_info->device_type != ModularCANLib_DeviceType_RoboMas_C610 &&
        device_info->device_type != ModularCANLib_DeviceType_RoboMas_C620)
    {
        MODULARCANLIB_PRINTF_INFO("[ERROR] ModularCANLib_RoboMas_GetDeviceParam: device_type mismatch (got %u)\n", (unsigned)device_info->device_type);
        return NULL;
    }
    return &device_info->device_param.robomas_param;
}

ModularCANLib_DeviceParam_RobStride_Type* ModularCANLib_RobStride_GetDeviceParam(ModularCANLib_DeviceInfo_Type *const device_info)
{
    if (device_info->device_type != ModularCANLib_DeviceType_RobStride_02 &&
        device_info->device_type != ModularCANLib_DeviceType_RobStride_04 &&
        device_info->device_type != ModularCANLib_DeviceType_RobStride_05_Edu)
    {
        MODULARCANLIB_PRINTF_INFO("[ERROR] ModularCANLib_RobStride_GetDeviceParam: device_type mismatch (got %u)\n", (unsigned)device_info->device_type);
        return NULL;
    }
    return &device_info->device_param.robstride_param;
}

ModularCANLib_DeviceParam_CANBoards_Air_Type* ModularCANLib_CANBoards_Air_GetDeviceParam(ModularCANLib_DeviceInfo_Type *const device_info)
{
    if (device_info->device_type != ModularCANLib_DeviceType_CANBoards_AIR)
    {
        MODULARCANLIB_PRINTF_INFO("[ERROR] ModularCANLib_CANBoards_Air_GetDeviceParam: device_type mismatch (got %u)\n", (unsigned)device_info->device_type);
        return NULL;
    }
    return &device_info->device_param.air_param;
}

ModularCANLib_DeviceParam_CANBoards_Dynamixel_Kondo_Type* ModularCANLib_CANBoards_Dynamixel_Kondo_GetDeviceParam(ModularCANLib_DeviceInfo_Type *const device_info)
{
    if (device_info->device_type != ModularCANLib_DeviceType_CANBoards_DYNAMIXEL_KONDO)
    {
        MODULARCANLIB_PRINTF_INFO("[ERROR] ModularCANLib_CANBoards_Dynamixel_GetDeviceParam: device_type mismatch (got %u)\n", (unsigned)device_info->device_type);
        return NULL;
    }
    return &device_info->device_param.dynamixel_kondo_param;
}

ModularCANLib_DeviceParam_CANBoards_MCMD_Type* ModularCANLib_CANBoards_MCMD_GetDeviceParam(ModularCANLib_DeviceInfo_Type *const device_info)
{
    if (device_info->device_type != ModularCANLib_DeviceType_CANBoards_MCMD4)
    {
        MODULARCANLIB_PRINTF_INFO("[ERROR] ModularCANLib_CANBoards_MCMD_GetDeviceParam: device_type mismatch (got %u)\n", (unsigned)device_info->device_type);
        return NULL;
    }
    return &device_info->device_param.mcmd_param;
}

ModularCANLib_DeviceParam_CANBoards_Servo_Type* ModularCANLib_CANBoards_Servo_GetDeviceParam(ModularCANLib_DeviceInfo_Type *const device_info)
{
    if (device_info->device_type != ModularCANLib_DeviceType_CANBoards_SERVO)
    {
        MODULARCANLIB_PRINTF_INFO("[ERROR] ModularCANLib_CANBoards_Servo_GetDeviceParam: device_type mismatch (got %u)\n", (unsigned)device_info->device_type);
        return NULL;
    }
    return &device_info->device_param.servo_param;
}

ModularCANLib_DeviceParam_ODrive_Type* ModularCANLib_ODrive_GetDeviceParam(ModularCANLib_DeviceInfo_Type *const device_info)
{
    if (device_info->device_type != ModularCANLib_DeviceType_ODrive_Pro)
    {
        MODULARCANLIB_PRINTF_INFO("[ERROR] ModularCANLib_ODrive_GetDeviceParam: device_type mismatch (got %u)\n", (unsigned)device_info->device_type);
        return NULL;
    }
    return &device_info->device_param.odrive_param;
}