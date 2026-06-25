//
// Created by tomoya on 2026/04/21.
//

#ifndef MODUALRCANLIB_DEVICEINFO_DEF_H
#define MODUALRCANLIB_DEVICEINFO_DEF_H

#include "can.h"

// 各DeviceParam
#include "RoboMas/ModularCANLib_RoboMas_DeviceParam.h"
#include "RobStride/ModularCANLib_RobStride_DeviceParam.h"
#include "CANBoards/ModularCANLib_CANBoards_DeviceParam.h"
#include "ODrive/ModularCANLib_ODrive_DeviceParam.h"
/* ===================================
 *         @開発者  ここに追記する
 * ===================================*/

typedef enum : uint32_t
{
    ModularCANLib_DeviceType_RoboMas_C610 = 0,
    ModularCANLib_DeviceType_RoboMas_C620,

    ModularCANLib_DeviceType_RobStride_02,
    ModularCANLib_DeviceType_RobStride_04,
    ModularCANLib_DeviceType_RobStride_05_Edu,

    ModularCANLib_DeviceType_CANBoards_MAIN,  // メイン基板
    ModularCANLib_DeviceType_CANBoards_DUMMY_1, // 今後の追加のために空けておく
    ModularCANLib_DeviceType_CANBoards_DUMMY_2, // 今後の追加のために空けておく
    ModularCANLib_DeviceType_CANBoards_DUMMY_3, // 今後の追加のために空けておく
    ModularCANLib_DeviceType_CANBoards_SERVO,     // servoの基板
    ModularCANLib_DeviceType_CANBoards_AIR,       // エアシリンダ
    ModularCANLib_DeviceType_CANBoards_MCMD4,     // device 0,1 100A モタドラ
    ModularCANLib_DeviceType_CANBoards_DYNAMIXEL_KONDO, // DYNAMIXEL_KONDO基板

    ModularCANLib_DeviceType_ODrive_Pro, // ODrive Pro (STD ID 11bit)
    /* ===================================
     *         @開発者  ここに追記する
     * ===================================*/
    ModularCANLib_DeviceType_SUM,
} ModularCANLib_DeviceType_Type;

// DeviceParamのUnion
typedef union
{
    ModularCANLib_DeviceParam_RoboMas_Type robomas_param;
    ModularCANLib_DeviceParam_RobStride_Type robstride_param;
    ModularCANLib_DeviceParam_CANBoards_Air_Type air_param;
    ModularCANLib_DeviceParam_CANBoards_Dynamixel_Kondo_Type dynamixel_kondo_param;
    ModularCANLib_DeviceParam_CANBoards_MCMD_Type mcmd_param;
    ModularCANLib_DeviceParam_CANBoards_Servo_Type servo_param;
    ModularCANLib_DeviceParam_ODrive_Type odrive_param;
    /* ===================================
     *         @開発者  ここに追記する
     * ===================================*/
} ModularCANLib_DeviceParam_Type;

typedef struct
{
    ModularCANLib_DeviceType_Type device_type;
    CAN_HandleTypeDef *hcan;
    uint32_t node_id;
    uint32_t name;
    ModularCANLib_DeviceParam_Type device_param;
} ModularCANLib_DeviceInfo_Type;

#define MODULARCANLIB_MAX_NUM_OF_ONE_DEVICE 16 // 一つの種類のDevice最大接続数
#define MODULARCANLIB_MAX_DEVICES 64 // Deviceの合計最大接続数
extern ModularCANLib_DeviceInfo_Type modularCANLib_DeviceInfos[MODULARCANLIB_MAX_DEVICES];
extern uint32_t modularCANLib_DeviceInfosCount;
extern ModularCANLib_DeviceInfo_Type* modularCANLib_NameTable[MODULARCANLIB_MAX_DEVICES];

/**
 * @brief device_typeを検証してdevice_paramへのポインタを返す関数群
 * device_typeが一致しない場合はprintfでエラーを出力してNULLを返す。
 */
ModularCANLib_DeviceParam_RoboMas_Type* ModularCANLib_RoboMas_GetDeviceParam(ModularCANLib_DeviceInfo_Type *device_info);
ModularCANLib_DeviceParam_RobStride_Type* ModularCANLib_RobStride_GetDeviceParam(ModularCANLib_DeviceInfo_Type *device_info);
ModularCANLib_DeviceParam_CANBoards_Air_Type* ModularCANLib_CANBoards_Air_GetDeviceParam(ModularCANLib_DeviceInfo_Type *device_info);
ModularCANLib_DeviceParam_CANBoards_Dynamixel_Kondo_Type* ModularCANLib_CANBoards_Dynamixel_Kondo_GetDeviceParam(ModularCANLib_DeviceInfo_Type *device_info);
ModularCANLib_DeviceParam_CANBoards_MCMD_Type* ModularCANLib_CANBoards_MCMD_GetDeviceParam(ModularCANLib_DeviceInfo_Type *device_info);
ModularCANLib_DeviceParam_CANBoards_Servo_Type* ModularCANLib_CANBoards_Servo_GetDeviceParam(ModularCANLib_DeviceInfo_Type *device_info);
ModularCANLib_DeviceParam_ODrive_Type* ModularCANLib_ODrive_GetDeviceParam(ModularCANLib_DeviceInfo_Type *device_info);

#endif //MODUALRCANLIB_DEVICEINFO_DEF_H
