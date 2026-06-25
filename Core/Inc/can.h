/*
 * can.h — CAN1 handle declaration for ModularCANLib compatibility.
 *
 * CubeMX normally generates this file when CAN is enabled in the .ioc.
 * Since this project configures CAN manually (can_bus.c), we provide
 * the minimal declaration that ModularCANLib expects.
 */
#ifndef __CAN_H__
#define __CAN_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f7xx_hal.h"

extern CAN_HandleTypeDef hcan1;

#ifdef __cplusplus
}
#endif

#endif /* __CAN_H__ */
