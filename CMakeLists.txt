cmake_minimum_required(VERSION 3.22)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/stm32_toolchain.cmake)

project(cthrobt C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# ── Compile definitions ───────────────────────────────────────────────────────
add_compile_definitions(
    USE_HAL_DRIVER
    STM32F767xx
    DEBUG
    SKIP_USB_INIT
    SKIP_ETH_INTERRUPT
    SKIP_PHY_INIT
    SIMULATION_MODE
)

# ── Compile flags ─────────────────────────────────────────────────────────────
add_compile_options(
    -Wall
    -fdata-sections
    -ffunction-sections
    $<$<CONFIG:Debug>:-ggdb3>
    $<$<CONFIG:Debug>:-O0>
    $<$<CONFIG:Release>:-O2>
)

# ── Include directories ───────────────────────────────────────────────────────
include_directories(
    Core/Inc
    Drivers/STM32F7xx_HAL_Driver/Inc
    Drivers/STM32F7xx_HAL_Driver/Inc/Legacy
    Drivers/CMSIS/Device/ST/STM32F7xx/Include
    Drivers/CMSIS/Include
    LWIP/App
    LWIP/Target
    Middlewares/Third_Party/LwIP/src/include
    Middlewares/Third_Party/LwIP/src/include/lwip
    Middlewares/Third_Party/LwIP/src/include/lwip/apps
    Middlewares/Third_Party/LwIP/src/include/lwip/priv
    Middlewares/Third_Party/LwIP/src/include/lwip/prot
    Middlewares/Third_Party/LwIP/src/include/netif
    Middlewares/Third_Party/LwIP/src/include/netif/ppp
    Middlewares/Third_Party/LwIP/src/include/compat/posix
    Middlewares/Third_Party/LwIP/src/include/compat/posix/arpa
    Middlewares/Third_Party/LwIP/src/include/compat/posix/net
    Middlewares/Third_Party/LwIP/src/include/compat/posix/sys
    Middlewares/Third_Party/LwIP/src/include/compat/stdc
    Middlewares/Third_Party/LwIP/system
    Middlewares/Third_Party/LwIP/system/arch
    Middlewares/Third_Party/FreeRTOS/Source/include
    Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS
    Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM7/r0p1
    Drivers/BSP/Components/lan8742
    Middlewares/ModularCANLib/Inc
    Middlewares/ModularCANLib/Inc/CANBoards
    Middlewares/ModularCANLib/Inc/RoboMas
    Middlewares/ModularCANLib/Inc/RobStride
    Middlewares/ModularCANLib/Inc/ODrive
)

# ── Source files ──────────────────────────────────────────────────────────────
file(GLOB_RECURSE CORE_SOURCES
    "Core/Src/*.c"
    "Core/Startup/*.s"
)

file(GLOB HAL_SOURCES
    "Drivers/STM32F7xx_HAL_Driver/Src/*.c"
    "Drivers/BSP/Components/lan8742/*.c"
)

file(GLOB_RECURSE FREERTOS_SOURCES
    "Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS/*.c"
    "Middlewares/Third_Party/FreeRTOS/Source/*.c"
    "Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM7/r0p1/*.c"
    "Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_4.c"
)

file(GLOB_RECURSE LWIP_SOURCES
    "LWIP/App/*.c"
    "LWIP/Target/*.c"
    "Middlewares/Third_Party/LwIP/src/*.c"
    "Middlewares/Third_Party/LwIP/system/OS/*.c"
)

file(GLOB_RECURSE MODULARCANLIB_SOURCES
    "Middlewares/ModularCANLib/Src/*.c"
)

# ── Executable ────────────────────────────────────────────────────────────────
add_executable(${PROJECT_NAME}.elf
    ${CORE_SOURCES}
    ${HAL_SOURCES}
    ${FREERTOS_SOURCES}
    ${LWIP_SOURCES}
    ${MODULARCANLIB_SOURCES}
)

# ── Linker ────────────────────────────────────────────────────────────────────
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/STM32F767ZITX_FLASH.ld)

target_link_options(${PROJECT_NAME}.elf PRIVATE
    -T${LINKER_SCRIPT}
    -Wl,-Map=${PROJECT_NAME}.map
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    -u _printf_float
)

# ── Post-build: generate .bin and .hex ────────────────────────────────────────
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.bin
    COMMAND ${OBJCOPY} -O ihex  $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.hex
    COMMAND ${SIZE}    $<TARGET_FILE:${PROJECT_NAME}.elf>
    COMMENT "Generating ${PROJECT_NAME}.bin / .hex"
)
