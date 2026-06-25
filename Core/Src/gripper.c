#include "gripper.h"
#include <stdio.h>

/*
 * Gripper control — pneumatic air cylinder via ModularCANLib on CAN1.
 *
 * Node ID = 0 (confirmed).  Air port: PORT_1 (device_num = 0).
 *
 * IMPORTANT — gripper_init() is called from StartControllerTask() (not main),
 * because ModularCANLib_WaitForConnect() and osDelay() in the port-init loop
 * require the FreeRTOS scheduler to be running.
 *
 * IMPORTANT — ModularCANLib_AllDevice_And_CANSystem_Init() handles all CAN
 * hardware init (filter banks, HAL_CAN_Start, interrupts).  can_bus_init()
 * must NOT be called when this gripper is in use — they would double-init CAN1.
 *
 * TODO (before real-hardware build):
 *   1. Copy ~/ModularCANLib/Inc/ and ~/ModularCANLib/Src/ into this project
 *      (or add as a git submodule), and add source paths to CMakeLists.txt.
 *   2. Add HAL_CAN callbacks to stm32f7xx_it.c — see gripper.h for the
 *      two callback stubs that need to be added.
 *   3. Confirm AIR_ON/AIR_OFF polarity (currently: AIR_ON = gripper closes).
 *   4. Remove can_bus_init() call from main.c once wrist is also migrated.
 */

#ifndef SIMULATION_MODE
#include "ModularCANLib.h"
#include "can.h"          /* extern CAN_HandleTypeDef hcan1 (CubeMX-generated) */
#include "cmsis_os.h"     /* osDelay */

#define GRIPPER_NODE_ID  0   /* confirmed */

static ModularCANLib_DeviceInfo_Type               *dev_info_air;
static ModularCANLib_DeviceParam_CANBoards_Air_Type *air_param;
#endif /* !SIMULATION_MODE */

static GripperState g_state = GRIPPER_OPEN;

void gripper_init(void)
{
    g_state = GRIPPER_OPEN;
#ifndef SIMULATION_MODE
    /* Register device — second arg is the lookup name (arbitrary index, use 0) */
    dev_info_air = ModularCANLib_DeviceInfo_Struct_Init(
        ModularCANLib_DeviceType_CANBoards_AIR, 0);
    dev_info_air->hcan    = &hcan1;
    dev_info_air->node_id = GRIPPER_NODE_ID;

    /* Init all devices + CAN1 (filter banks, HAL_CAN_Start, interrupts) */
    ModularCANLib_AllDevice_And_CANSystem_Init();
    ModularCANLib_WaitForConnect();

    /* Init all 8 air ports (PORT_1=0 ... PORT_8=7) to OFF before use */
    air_param = ModularCANLib_CANBoards_Air_GetDeviceParam(dev_info_air);
    for (uint8_t i = PORT_1; i <= PORT_8; i++) {
        air_param->device_num = i;
        AirCylinder_Init(dev_info_air, AIR_OFF);
        osDelay(10);
    }
    air_param->device_num = PORT_3;  /* AIR3 confirmed */

    printf("[GRIPPER] init done\r\n");
#else
    printf("[GRIPPER] init — state: OPEN\n");
#endif
}

void gripper_close(void)
{
    if (g_state == GRIPPER_CLOSED) return;
    g_state = GRIPPER_CLOSED;
#ifndef SIMULATION_MODE
    /* TODO: swap AIR_ON/AIR_OFF if the physical valve is wired inverted */
    AirCylinder_SendOutput(dev_info_air, AIR_ON);
    printf("[GRIPPER] CLOSE\r\n");
#else
    printf("[GRIPPER] CLOSE\n");
#endif
}

void gripper_open(void)
{
    if (g_state == GRIPPER_OPEN) return;
    g_state = GRIPPER_OPEN;
#ifndef SIMULATION_MODE
    AirCylinder_SendOutput(dev_info_air, AIR_OFF);
    printf("[GRIPPER] OPEN\r\n");
#else
    printf("[GRIPPER] OPEN\n");
#endif
}

GripperState gripper_get_state(void)
{
    return g_state;
}
