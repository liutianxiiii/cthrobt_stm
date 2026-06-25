//
// Created by tomoya on 26/05/09.
//

#include "CANBoards/ModularCANLib_CANBoards_System_Def.h"

uint32_t Make_CAN_ID_from_CAN_Device(const ModularCANLib_DeviceInfo_Type*device_info, const uint8_t cmd) {
    const uint8_t node_type = (uint8_t)(device_info->device_type - ModularCANLib_DeviceType_CANBoards_MAIN) & (0b111);
    uint8_t device_num = 0;
    switch (device_info->device_type) {
        case ModularCANLib_DeviceType_CANBoards_MAIN:
        case ModularCANLib_DeviceType_CANBoards_DUMMY_1:
        case ModularCANLib_DeviceType_CANBoards_DUMMY_2:
        case ModularCANLib_DeviceType_CANBoards_DUMMY_3:
        case ModularCANLib_DeviceType_CANBoards_SERVO:
            break;
        case ModularCANLib_DeviceType_CANBoards_AIR:
            device_num = device_info->device_param.air_param.device_num;
            break;
        case ModularCANLib_DeviceType_CANBoards_MCMD4:
            device_num = device_info->device_param.mcmd_param.device_num;
            break;
        case ModularCANLib_DeviceType_CANBoards_DYNAMIXEL_KONDO:
            device_num = device_info->device_param.dynamixel_kondo_param.device_num;
            break;
        default:
            break;
    }
    return (((node_type&0b111)<<11) | (((device_info->node_id)&0b111)<<8) | ((device_num&0b111)<<5) | (cmd&0b11111) );
}