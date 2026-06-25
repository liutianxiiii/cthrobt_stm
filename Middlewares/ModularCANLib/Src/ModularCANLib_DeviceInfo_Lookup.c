//
// Created by tomoya on 2026/04/21.
//

#include "ModularCANLib_DeviceInfo_Lookup.h"

#include "ModularCANLib_DeviceInfo_Def.h"
#include "ModularCANLib_Sys.h"

static ModularCANLib_DeviceInfo_Type* table_can_to_deviceInfo[3][MODULARCANLIB_MAX_DEVICES] = {NULL};
static uint32_t table_size_can_to_deviceInfo[3] = {0};
static ModularCANLib_DeviceInfo_Type* table_deviceType_to_deviceInfo[ModularCANLib_DeviceType_SUM][MODULARCANLIB_MAX_NUM_OF_ONE_DEVICE] = {NULL};
static uint32_t table_size_deviceType_to_deviceInfo[ModularCANLib_DeviceType_SUM] = {0};

void ModularCANLib_MakeLookupTable(void)
{
    for (int i = 0; i < modularCANLib_DeviceInfosCount; i++)
    {
        if (modularCANLib_DeviceInfos[i].hcan == NULL) continue;
        ModularCANLib_DeviceInfo_Type *device_info = &modularCANLib_DeviceInfos[i];

        // lockupテーブルに登録
        // table_can_to_deviceInfo
        const uint8_t can_idx = ModularCANLib_hcan2idx(device_info->hcan);
        table_can_to_deviceInfo[can_idx][table_size_can_to_deviceInfo[can_idx]] = device_info;
        table_size_can_to_deviceInfo[can_idx]++;

        // table_deviceType_to_deviceInfo
        table_deviceType_to_deviceInfo[device_info->device_type][table_size_deviceType_to_deviceInfo[device_info->device_type]] = device_info;
        table_size_deviceType_to_deviceInfo[device_info->device_type]++;
    }
}

void ModularCANLib_GetDeviceInfos_By_Hcan(const CAN_HandleTypeDef *hcan, ModularCANLib_DeviceInfo_Type ***device_info_ptrs, uint32_t *size)
{
    const uint8_t can_idx = ModularCANLib_hcan2idx(hcan);
    if (table_size_can_to_deviceInfo[can_idx] == 0)
    {
        device_info_ptrs = NULL;
        *size = 0;
        return;
    }

    *device_info_ptrs = table_can_to_deviceInfo[can_idx];
    *size = table_size_can_to_deviceInfo[can_idx];
}

void ModularCANLib_GetDeviceInfos_By_DeviceType(const ModularCANLib_DeviceType_Type device_type, ModularCANLib_DeviceInfo_Type ***device_info_ptrs, uint32_t *size)
{
    if (table_size_deviceType_to_deviceInfo[device_type] == 0)
    {
        device_info_ptrs = NULL;
        *size = 0;
        return;
    }

    *device_info_ptrs = table_deviceType_to_deviceInfo[device_type];
    *size = table_size_deviceType_to_deviceInfo[device_type];
}

ModularCANLib_DeviceInfo_Type* ModularCANLib_GetDeviceInfo_By_Name(const uint32_t name)
{
    if (name >= MODULARCANLIB_MAX_DEVICES) return NULL;
    return modularCANLib_NameTable[name];
}