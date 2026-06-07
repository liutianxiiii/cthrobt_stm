#include "gripper.h"
#include <stdio.h>

/*
 * Gripper control — servo via TIM3 CH1 (PB4, AF2).
 *
 * Real hardware wiring:
 *   PB4 (CN9 pin 6 / Arduino D5) → servo signal wire
 *   5 V rail                     → servo power
 *   GND                          → servo ground
 *
 * PWM spec (50 Hz, 1 µs tick):
 *   APB1 timer clock = 96 MHz, prescaler = 95 → 1 MHz tick
 *   ARR = 19999 → 20 ms period = 50 Hz
 *   CCR_CLOSE = 1000 → 1 ms pulse (closed position)
 *   CCR_OPEN  = 2000 → 2 ms pulse (open   position)
 *
 * Adjust CCR values to match your servo and gripper travel.
 */

#ifndef SIMULATION_MODE
#include "stm32f7xx_hal.h"

#define GRIPPER_CHANNEL     TIM_CHANNEL_1
#define GRIPPER_CCR_CLOSE   1000u
#define GRIPPER_CCR_OPEN    2000u

/* htim3 is initialised by MX_TIM3_Init() in main.c */
extern TIM_HandleTypeDef htim3;

static void set_pwm(uint32_t ccr)
{
    __HAL_TIM_SET_COMPARE(&htim3, GRIPPER_CHANNEL, ccr);
}
#endif /* !SIMULATION_MODE */

static GripperState g_state = GRIPPER_OPEN;

void gripper_init(void)
{
#ifndef SIMULATION_MODE
    /* MX_TIM3_Init() already configured the timer; just start PWM output */
    HAL_TIM_PWM_Start(&htim3, GRIPPER_CHANNEL);
    set_pwm(GRIPPER_CCR_OPEN);
#endif
    g_state = GRIPPER_OPEN;
    printf("[GRIPPER] init — state: OPEN\n");
}

void gripper_close(void)
{
    if (g_state == GRIPPER_CLOSED) return;
    g_state = GRIPPER_CLOSED;
#ifndef SIMULATION_MODE
    set_pwm(GRIPPER_CCR_CLOSE);
#endif
    printf("[GRIPPER] CLOSE\n");
}

void gripper_open(void)
{
    if (g_state == GRIPPER_OPEN) return;
    g_state = GRIPPER_OPEN;
#ifndef SIMULATION_MODE
    set_pwm(GRIPPER_CCR_OPEN);
#endif
    printf("[GRIPPER] OPEN\n");
}

GripperState gripper_get_state(void)
{
    return g_state;
}
