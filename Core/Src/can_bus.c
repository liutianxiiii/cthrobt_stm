#include "can_bus.h"
#include "can.h"
#include <stdio.h>

/*
 * hcan1 is defined here (non-static) and declared extern in can.h.
 * ModularCANLib accesses it via can.h.
 *
 * can_bus_init() only calls HAL_CAN_Init() — filter bank setup,
 * HAL_CAN_Start(), and interrupt activation are handled by
 * ModularCANLib_AllDevice_And_CANSystem_Init() inside gripper_init().
 */
CAN_HandleTypeDef hcan1;

HAL_StatusTypeDef can_bus_init(void)
{
    hcan1.Instance                  = CAN1;
    hcan1.Init.Prescaler            = CAN_BAUD_PRESCALER;
    hcan1.Init.Mode                 = CAN_MODE_NORMAL;
    hcan1.Init.SyncJumpWidth        = CAN_SJW_1TQ;
    hcan1.Init.TimeSeg1             = CAN_BS1_13TQ;
    hcan1.Init.TimeSeg2             = CAN_BS2_2TQ;
    hcan1.Init.TimeTriggeredMode    = DISABLE;
    hcan1.Init.AutoBusOff           = ENABLE;
    hcan1.Init.AutoWakeUp           = DISABLE;
    hcan1.Init.AutoRetransmission   = ENABLE;
    hcan1.Init.ReceiveFifoLocked    = DISABLE;
    hcan1.Init.TransmitFifoPriority = DISABLE;

    if (HAL_CAN_Init(&hcan1) != HAL_OK) {
        printf("[CAN] HAL_CAN_Init failed\r\n");
        return HAL_ERROR;
    }

    /* Filter, HAL_CAN_Start, and interrupt activation are done by
     * ModularCANLib_AllDevice_And_CANSystem_Init() in gripper_init(). */
    printf("[CAN] CAN1 hardware ready (%u kbps)\r\n",
           (unsigned)(48000u / CAN_BAUD_PRESCALER / 16u));
    return HAL_OK;
}

HAL_StatusTypeDef can_send(uint32_t std_id, const uint8_t *data, uint8_t len)
{
    CAN_TxHeaderTypeDef hdr = {
        .StdId              = std_id,
        .IDE                = CAN_ID_STD,
        .RTR                = CAN_RTR_DATA,
        .DLC                = len,
        .TransmitGlobalTime = DISABLE,
    };
    uint32_t mailbox;
    HAL_StatusTypeDef st = HAL_CAN_AddTxMessage(&hcan1, &hdr,
                                                 (uint8_t *)data, &mailbox);
    if (st != HAL_OK)
        printf("[CAN] tx failed id=0x%03lX\r\n", std_id);
    return st;
}
