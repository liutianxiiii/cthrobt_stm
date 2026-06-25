/*
 * CAN_AirCylinder_Def.h
 *
 * Created on: 2/9/2022
 *     Author: Kotaro Okuda
 */

#ifndef _CAN_AIRCYLINDER_DEF_H_
#define _CAN_AIRCYLINDER_DEF_H_

#include "stdint.h"
#include "stdbool.h"

typedef enum {
    AIR_OFF = 0U, // Equal to GPIO_PIN_RESET
    AIR_ON        // Equal to GPIO_PIN_SET
} ModularCANLib_Air_PortStatus_Typedef;

typedef enum {
    PORT_1 = 0,
    PORT_2 = 1,
    PORT_3 = 2,
    PORT_4 = 3,
    PORT_5 = 4,
    PORT_6 = 5,
    PORT_7 = 6,
    PORT_8 = 7,
} ModularCANLib_Air_PortNumber_Type;


// エアシリンダのCAN送受信用コマンド
typedef enum{
    AIR_CMD_AWAKE,
    AIR_CMD_INIT,
    AIR_CMD_OUTPUT,
} ModularCANLib_CANAir_CMD;

typedef struct {
    uint8_t device_num;
    bool _get_flag;
} ModularCANLib_DeviceParam_CANBoards_Air_Type;

#endif /* _CAN_AIRCYLINDER_DEF_H_ */
