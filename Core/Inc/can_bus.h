#pragma once
#include "stm32f7xx_hal.h"
#include <stdint.h>

/*
 * CAN1 bus driver — PD0 (RX) / PD1 (TX), AF9
 *
 * TODO (before running on hardware):
 *   1. Confirm baud rate with HW team and update CAN_BAUD_PRESCALER.
 *   2. Confirm STM32 CAN1 pin assignment (PD0/PD1 assumed here).
 *      If the transceiver is wired to different pins, update HAL_CAN_MspInit
 *      in stm32f7xx_hal_msp.c.
 *
 * Timing (APB1 = 48 MHz, total TQ per bit = 1 + BS1 + BS2 = 16):
 *   500 kbps → Prescaler = 6   (default)
 *   1   Mbps → Prescaler = 3
 *   250 kbps → Prescaler = 12
 */
#define CAN_BAUD_PRESCALER  6          /* TODO: adjust for actual baud rate */

HAL_StatusTypeDef can_bus_init(void);

/* Send a standard-frame CAN message. len must be 0–8. */
HAL_StatusTypeDef can_send(uint32_t std_id, const uint8_t *data, uint8_t len);
