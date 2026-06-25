//
// Created by tomoya on 2026/04/21.
//

#include "ModularCANLib.h"
#include "main.h"

#include "cmsis_os.h"
#include "stdbool.h"

#include "ModularCANLib_Sys.h"
#include "ModularCANLib_DeviceInfo_Lookup.h"
#include "ModularCANLib_RTOS.h"

#include "RoboMas/ModularCANLib_RoboMas.h"
#include "RobStride/ModularCANLib_RobStride.h"
#include "CANBoards/ModularCANLib_CANBoards.h"
#include "ODrive/ModularCANLib_ODrive.h"

static void ModularCANLib_ProcessRXData_Once(const CAN_HandleTypeDef *hcan, const ModularCANLib_CANId_Type can_id_t, CAN_RxMsg* rx_msg)
{
    const uint32_t can_id = can_id_t == ModularCANLib_CAN_ID_TYPE_STD ? rx_msg->Header.StdId : rx_msg->Header.ExtId;
    // DeviceTypeを特定
    const bool is_robomas = ModularCANLib_RoboMas_IsRoboMasID_RX(can_id_t, can_id);
    const bool is_robstride = ModularCANLib_RobStride_IsRobStrideID_RX(can_id_t, can_id);
    const bool is_canboards = ModularCANLib_CANBoards_IsCANBoardsID_RX(can_id_t, can_id);
    const bool is_odrive = ModularCANLib_ODrive_IsODriveID_RX(can_id_t, can_id);
    /* ===================================
     *         @開発者  ここに追記する
     * ===================================*/

    // Deviceごとに処理
    // MODULARCANLIB_PRINTF_DEBUG("ModularCANLib_ProcessRXData_Once CAN ID: %lu(%s), flag: robomas=%u, robstride=%u\n",
    //     can_id, can_id_t == ModularCANLib_CAN_ID_TYPE_STD ? "STD" : "EXT", is_robomas, is_robstride);

    // クリティカルにするべき <- ?
    bool data_is_processed = false;
    if (is_robomas)
    {
        ModularCANLib_RoboMas_ProcessRxData(hcan, rx_msg);
        data_is_processed = true;
    }
    if (is_robstride) {
        ModularCANLib_RobStride_ProcessRxData(hcan, rx_msg);
        data_is_processed = true;
    }
    if (is_canboards) {
        ModularCANLib_CANBoards_ProcessRxData(hcan, rx_msg);
        data_is_processed = true;
    }
    if (is_odrive) {
        ModularCANLib_ODrive_ProcessRxData(hcan, rx_msg);
        data_is_processed = true;
    }
    /* ===================================
     *         @開発者  ここに追記する
     * ===================================*/
    // クリティカル終了

    if (!data_is_processed) {
        MODULARCANLIB_PRINTF_DEBUG("ModularCANLib_ProcessRXData_Once Unwatched CAN ID: %lu(%s)\n",
            can_id, can_id_t == ModularCANLib_CAN_ID_TYPE_STD ? "STD" : "EXT");
    }
}

void ModularCANLib_ProcessRXData(void)
{
    // 受信待機
    const osEvent evt = osSignalWait(MODULARCANLIB_TASK_CAN_RX_SIGNAL, osWaitForever);
    // シグナル受信以外のイベント（エラーなど）で抜けた場合は処理しない
    if (evt.status != osEventSignal) return;

    CAN_RxMsg rx_msg;
    for (int i = 0; i < NUM_OF_CANS; i++)
    {
        if (modularCANLib_UsedCANHandlers[i] == NULL) continue; // CAN(i+1)は使用されていない
        const CAN_HandleTypeDef *hcan = modularCANLib_UsedCANHandlers[i];

        // バッファ全てを処理
        // for Standard ID
        while (ModularCANLib_Sys_ReadFromBuffer(hcan, ModularCANLib_CAN_ID_TYPE_STD, &rx_msg) == HAL_OK)
        {
            ModularCANLib_ProcessRXData_Once(hcan, ModularCANLib_CAN_ID_TYPE_STD, &rx_msg);
        }
        // for External ID
        while (ModularCANLib_Sys_ReadFromBuffer(hcan, ModularCANLib_CAN_ID_TYPE_EXT, &rx_msg) == HAL_OK)
        {
            ModularCANLib_ProcessRXData_Once(hcan, ModularCANLib_CAN_ID_TYPE_EXT, &rx_msg);
        }
    }
}

ModularCANLib_DeviceInfo_Type * ModularCANLib_DeviceInfo_Struct_Init(const ModularCANLib_DeviceType_Type device_type, const uint32_t name)
{
    if (modularCANLib_DeviceInfosCount >= MODULARCANLIB_MAX_DEVICES)
    {
        MODULARCANLIB_PRINTF_DEBUG("[ERROR] All Device Infos have already been used!\n");
        Error_Handler();
        return NULL;
    }
    // 空device_infoを取得
    ModularCANLib_DeviceInfo_Type *device_info = &modularCANLib_DeviceInfos[modularCANLib_DeviceInfosCount];
    modularCANLib_DeviceInfosCount++;

    // device type・name設定
    device_info->device_type = device_type;
    device_info->name = name;

    // 名前ルックアップテーブルに登録
    if (name >= MODULARCANLIB_MAX_DEVICES)
    {
        MODULARCANLIB_PRINTF_DEBUG("[ERROR] Device name %lu exceeds MODULARCANLIB_MAX_DEVICES!\n", name);
        Error_Handler();
        return NULL;
    }
    modularCANLib_NameTable[name] = device_info;

    switch (device_type)
    {
        case ModularCANLib_DeviceType_RoboMas_C610:
        case ModularCANLib_DeviceType_RoboMas_C620:
            ModularCANLib_RoboMas_DeviceParam_Struct_Init(&device_info->device_param.robomas_param);
            break;
        case ModularCANLib_DeviceType_RobStride_02:
        case ModularCANLib_DeviceType_RobStride_04:
        case ModularCANLib_DeviceType_RobStride_05_Edu:
            ModularCANLib_RobStride_DeviceParam_Struct_Init(&device_info->device_param.robstride_param);
            break;
        case ModularCANLib_DeviceType_CANBoards_MCMD4:
        case ModularCANLib_DeviceType_CANBoards_DYNAMIXEL_KONDO:
        case ModularCANLib_DeviceType_CANBoards_AIR:
        case ModularCANLib_DeviceType_CANBoards_SERVO:
            // なにもしない
            break;
        case ModularCANLib_DeviceType_ODrive_Pro:
            ModularCANLib_ODrive_DeviceParam_Struct_Init(&device_info->device_param.odrive_param);
            break;
            /* ===================================
             *         @開発者  ここに追記する
             * ===================================*/
        default:
            MODULARCANLIB_PRINTF_DEBUG("ModularCANLib_DeviceInfo_Struct_Init Unknown device type: %u\n", (uint8_t)device_type);
            Error_Handler();
            return NULL;
    }

    return device_info;
}

void ModularCANLib_AllDevice_And_CANSystem_Init()
{
    for (uint32_t i = 0; i < modularCANLib_DeviceInfosCount; i++)
    {
        ModularCANLib_DeviceInfo_Type *device_info = &modularCANLib_DeviceInfos[i];

        // hcan登録
        if (device_info->hcan == NULL)
        {
            MODULARCANLIB_PRINTF_DEBUG("ModularCANLib_AllDevice_Init device_info->hcan is NULL\n");
            continue;
        }
        const uint8_t can_idx = ModularCANLib_hcan2idx(device_info->hcan);
        modularCANLib_UsedCANHandlers[can_idx] = device_info->hcan;

        // deviceごとに起動処理
        switch (device_info->device_type)
        {
            case ModularCANLib_DeviceType_RoboMas_C610:
            case ModularCANLib_DeviceType_RoboMas_C620:
                ModularCANLib_RoboMas_Device_Init(device_info);
                break;
            case ModularCANLib_DeviceType_RobStride_02:
            case ModularCANLib_DeviceType_RobStride_04:
            case ModularCANLib_DeviceType_RobStride_05_Edu:
                ModularCANLib_RobStride_Device_Init(device_info);
                break;
            case ModularCANLib_DeviceType_CANBoards_MCMD4:
                ModularCANLib_CANBoards_MCMD_Device_Init(device_info);
                break;
            case ModularCANLib_DeviceType_CANBoards_DYNAMIXEL_KONDO:
                ModularCANLib_CANBoards_Dynamixel_Kondo_Device_Init(device_info);
                break;
            case ModularCANLib_DeviceType_CANBoards_AIR:
                ModularCANLib_CANBoards_Air_Device_Init(device_info);
                break;
            case ModularCANLib_DeviceType_CANBoards_SERVO:
                ModularCANLib_CANBoards_Servo_Device_Init(device_info);
                break;
            case ModularCANLib_DeviceType_ODrive_Pro:
                ModularCANLib_ODrive_Device_Init(device_info);
                break;
                /* ===================================
                 *         @開発者  ここに追記する
                 * ===================================*/
            default:
                MODULARCANLIB_PRINTF_DEBUG("ModularCANLib_Device_Init Unknown device type: %u\n", (uint8_t)device_info->device_type);
                break;
        }
    }

    // Lookupテーブルを作成
    ModularCANLib_MakeLookupTable();

    // AfterDeviceInit
    ModularCANLib_RoboMas_AfterDeviceInit();
    ModularCANLib_RobStride_AfterDeviceInit();
    ModularCANLib_CANBoards_AfterDeviceInit();
    ModularCANLib_ODrive_AfterDeviceInit();
    /* ===================================
     *         @開発者  ここに追記する
     * ===================================*/

    // FreeRTOS設定
    ModularCANLib_RTOS_Init();

    // CAN設定
    ModularCANLib_Sys_FilterBank_Init();
    ModularCANLib_Sys_ActivateNotification();

    // CANスタート
    for (int i = 0; i < NUM_OF_CANS; i++)
    {
        if (modularCANLib_UsedCANHandlers[i] == NULL) continue; // CAN(i+1)は使用されていない
        HAL_CAN_Start(modularCANLib_UsedCANHandlers[i]);
    }

    // Analysis表示
    MODULARCANLIB_PRINTF_INFO("========= ModularCANLib Info ===========\n");
    MODULARCANLIB_PRINTF_INFO("<< CAN Bus >>\n");
    MODULARCANLIB_PRINTF_INFO("  Detected: %u\n", NUM_OF_CANS);
    MODULARCANLIB_PRINTF_INFO("  USED:");
    for (int i = 0; i < NUM_OF_CANS; i++)
    {
        if (modularCANLib_UsedCANHandlers[i] == NULL) continue;
        MODULARCANLIB_PRINTF_INFO(" CAN%u", i+1);
    }
    MODULARCANLIB_PRINTF_INFO("\n");
    MODULARCANLIB_PRINTF_INFO("\n<< CAN Usage >>\n");
    for (int i = 0; i < NUM_OF_CANS; i++)
    {
        if (modularCANLib_UsedCANHandlers[i] == NULL) continue;
        MODULARCANLIB_PRINTF_INFO("  CAN%u:\n", i+1);
        ModularCANLib_DeviceInfo_Type **device_info_ptrs;
        uint32_t size;
        ModularCANLib_GetDeviceInfos_By_Hcan(modularCANLib_UsedCANHandlers[i], &device_info_ptrs, &size);
        MODULARCANLIB_PRINTF_INFO("    Num of Devices: %lu\n", size);
    }
    MODULARCANLIB_PRINTF_INFO("========= ModularCANLib Info ===========\n");
}

static void ModularCANLib_PrintConnectedDevices()
{
    for (uint32_t i = 0; i < modularCANLib_DeviceInfosCount; i++)
    {
        const ModularCANLib_DeviceInfo_Type *device_info = &modularCANLib_DeviceInfos[i];
        MODULARCANLIB_PRINTF_INFO("Device[%lu]: Type=", i);
        switch (device_info->device_type)
        {
            case ModularCANLib_DeviceType_RoboMas_C610:
                MODULARCANLIB_PRINTF_INFO("RoboMas C610, Connected=%u\n",
                    ModularCANLib_RoboMas_IsDeviceConnected(device_info) ? 1 : 0);
                break;
            case ModularCANLib_DeviceType_RoboMas_C620:
                MODULARCANLIB_PRINTF_INFO("RoboMas C620, Connected=%u\n",
                    ModularCANLib_RoboMas_IsDeviceConnected(device_info) ? 1 : 0);
                break;
            case ModularCANLib_DeviceType_RobStride_02:
                MODULARCANLIB_PRINTF_INFO("RobStride 02, Connected=%u\n",
                    ModularCANLib_RobStride_IsDeviceConnected(device_info) ? 1 : 0);
                    break;
            case ModularCANLib_DeviceType_RobStride_04:
                MODULARCANLIB_PRINTF_INFO("RobStride 04, Connected=%u\n",
                    ModularCANLib_RobStride_IsDeviceConnected(device_info) ? 1 : 0);
                break;
            case ModularCANLib_DeviceType_RobStride_05_Edu:
                MODULARCANLIB_PRINTF_INFO("RobStride 05 Education, Connected=%u\n",
                    ModularCANLib_RobStride_IsDeviceConnected(device_info) ? 1 : 0);
                break;
            case ModularCANLib_DeviceType_CANBoards_MCMD4:
                MODULARCANLIB_PRINTF_INFO("CANBoards MCMD4, Connected=%u\n",
                    ModularCANLib_CANBoards_MCMD_IsDeviceConnected(device_info) ? 1 : 0);
                break;
            case ModularCANLib_DeviceType_CANBoards_AIR:
                MODULARCANLIB_PRINTF_INFO("CANBoards Air, Connected=%u\n",
                    ModularCANLib_CANBoards_Air_IsDeviceConnected(device_info) ? 1 : 0);
                break;
            case ModularCANLib_DeviceType_CANBoards_SERVO:
                MODULARCANLIB_PRINTF_INFO("CANBoards Servo, Connected=%u\n",
                    ModularCANLib_CANBoards_Servo_IsDeviceConnected(device_info) ? 1 : 0);
                break;
            case ModularCANLib_DeviceType_CANBoards_DYNAMIXEL_KONDO:
                MODULARCANLIB_PRINTF_INFO("CANBoards Dynamixel_Kondo, Connected=%u\n",
                    ModularCANLib_CANBoards_Dynamixel_Kondo_IsDeviceConnected(device_info) ? 1 : 0);
                break;
            case ModularCANLib_DeviceType_ODrive_Pro:
                MODULARCANLIB_PRINTF_INFO("ODrive Pro node%lu, Connected=%u\n",
                    device_info->node_id,
                    ModularCANLib_ODrive_IsDeviceConnected(device_info) ? 1 : 0);
                break;
        /* ===================================
         *         @開発者  ここに追記する
         * ===================================*/
        default:
            break;
        }
    }

    // その他検出情報
    ModularCANLib_CANBoards_PrintDetection();
    /* ===================================
     *         @開発者  ここに追記する
     * ===================================*/
}

void ModularCANLib_WaitForConnect()
{
    uint32_t lt = 0;
    for (;;)
    {
        osDelay(5);

        if (HAL_GetTick() - lt > 500)
        {
            // 500 msに一度接続状況を出力
            ModularCANLib_PrintConnectedDevices();
            lt = HAL_GetTick();
        }

        if (!ModularCANLib_RoboMas_AllDevice_Connected()) continue;
        if (!ModularCANLib_RobStride_AllDevice_Connected()) continue;
        if (!ModularCANLib_CANBoards_AllDevice_Connected()) continue;
        if (!ModularCANLib_ODrive_AllDevice_Connected()) continue;
        /* ===================================
         *         @開発者  ここに追記する
         * ===================================*/

        ModularCANLib_PrintConnectedDevices();

        return; // すべて検出
    }
}