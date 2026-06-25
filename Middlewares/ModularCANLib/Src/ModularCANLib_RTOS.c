//
// Created by tomoya on 26/04/22.
//

#include "ModularCANLib_RTOS.h"

#include "ModularCANLib.h"

osThreadId modularCANLibHandle;
uint32_t modularCANLibBuffer[ 1024 ];
osStaticThreadDef_t modularCANLibControlBlock;

bool modularCANLib_RTOS_Initialized = false;

void StartModularCANLib(void const * argument);

void ModularCANLib_RTOS_Init(void) {
    /* definition and creation of modularCANLib */
    osThreadStaticDef(modularCANLib, StartModularCANLib, osPriorityRealtime, 0, 1024, modularCANLibBuffer, &modularCANLibControlBlock);
    modularCANLibHandle = osThreadCreate(osThread(modularCANLib), NULL);

    modularCANLib_RTOS_Initialized = true;
}

void StartModularCANLib(void const * argument)
{
    /* USER CODE BEGIN StartModularCANLib */
    /* Infinite loop */
    for(;;)
    {
        ModularCANLib_ProcessRXData(); // CANデータ処理
    }
    /* USER CODE END StartModularCANLib */
}