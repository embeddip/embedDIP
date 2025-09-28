cmake_minimum_required(VERSION 3.15)
project(embedDIP C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

enable_language(C CXX ASM)

# === Gather source files ===
file(GLOB_RECURSE BOARD_SRC board/*.c)
file(GLOB_RECURSE CORE_SRC core/*.c)
file(GLOB_RECURSE IMGPROC_SRC imgproc/*.c)
file(GLOB_RECURSE SERIAL_SRC serial/*.c)
file(GLOB_RECURSE DEVICE_SRC device/**/*.c)
file(GLOB_RECURSE WRAPPER_SRC wrapper/*.cpp)

# === CMSIS-DSP Source Files (if not already linked in parent CMake) ===
file(GLOB_RECURSE CMSIS_DSP_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/Source/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/Source/**/*.S
)

# === Create the static library target ===
add_library(embedDIP STATIC
    ${BOARD_SRC}
    ${CORE_SRC}
    ${IMGPROC_SRC}
    ${SERIAL_SRC}
    ${DEVICE_SRC}
    ${CMSIS_DSP_SRCS}
    ${WRAPPER_SRC}
)

# === Add compile-time macros ===
target_compile_definitions(embedDIP PUBLIC
    USE_EMBED_DIP
    ARM_MATH_CM7
    __FPU_PRESENT=1
)

# === Include paths ===
target_include_directories(embedDIP PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/imgproc
    ${CMAKE_CURRENT_SOURCE_DIR}/serial
    ${CMAKE_CURRENT_SOURCE_DIR}/device/camera
    ${CMAKE_CURRENT_SOURCE_DIR}/device/display
    ${CMAKE_CURRENT_SOURCE_DIR}/board/stm32
    ${CMAKE_CURRENT_SOURCE_DIR}/board/esp32
    ${CMAKE_CURRENT_SOURCE_DIR}/wrapper

    # HAL and CMSIS if needed
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/STM32F7xx_HAL_Driver/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/Device/ST/STM32F7xx/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/Core/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Inc/


)
