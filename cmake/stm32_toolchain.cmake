set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

# Find arm-none-eabi-gcc from PATH (works on macOS, Windows, Linux)
find_program(ARM_NONE_EABI_GCC arm-none-eabi-gcc REQUIRED)
get_filename_component(ARM_GCC_DIR ${ARM_NONE_EABI_GCC} DIRECTORY)

if(WIN32)
    set(TOOLCHAIN_PREFIX ${ARM_GCC_DIR}/arm-none-eabi-)
    set(TOOL_EXT ".exe")
else()
    set(TOOLCHAIN_PREFIX ${ARM_GCC_DIR}/arm-none-eabi-)
    set(TOOL_EXT "")
endif()

set(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}gcc${TOOL_EXT})
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++${TOOL_EXT})
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc${TOOL_EXT})
set(OBJCOPY            ${TOOLCHAIN_PREFIX}objcopy${TOOL_EXT})
set(OBJDUMP            ${TOOLCHAIN_PREFIX}objdump${TOOL_EXT})
set(SIZE               ${TOOLCHAIN_PREFIX}size${TOOL_EXT})

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# STM32F767ZI: Cortex-M7 with double-precision FPU
set(CPU_FLAGS "-mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard")

set(CMAKE_C_FLAGS_INIT   "${CPU_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${CPU_FLAGS}")
set(CMAKE_ASM_FLAGS_INIT "${CPU_FLAGS} -x assembler-with-cpp")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${CPU_FLAGS} -specs=nosys.specs -specs=nano.specs")
