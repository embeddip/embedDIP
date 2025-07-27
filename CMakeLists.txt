add_library(embedDIP)

target_compile_definitions(embedDIP PUBLIC
    USE_EMBED_DIP
    ARM_MATH_CM7
    __FPU_PRESENT=1
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_language(C CXX)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/STM32F7xx_HAL_Driver/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/Device/ST/STM32F7xx/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Inc
)

# Include paths
target_include_directories(embedDIP PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/
    ${CMAKE_CURRENT_SOURCE_DIR}/dip/core
    ${CMAKE_CURRENT_SOURCE_DIR}/dip/imgproc
    ${CMAKE_CURRENT_SOURCE_DIR}/input/camera
    ${CMAKE_CURRENT_SOURCE_DIR}/input/serial
    ${CMAKE_CURRENT_SOURCE_DIR}/output/serial
    ${CMAKE_CURRENT_SOURCE_DIR}/output/display
    ${CMAKE_CURRENT_SOURCE_DIR}/hdl/ov5640
    ${CMAKE_CURRENT_SOURCE_DIR}/hdl/rk043fn48h
    ${CMAKE_CURRENT_SOURCE_DIR}/hdl/stm32f7g46
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp
)

# Core image processing
target_sources(embedDIP PRIVATE
    dip/core/image.c
    dip/core/stm32_common.c
    dip/core/memory_manager.c
    dip/imgproc/color.c
    dip/imgproc/pixel.c
    dip/imgproc/filter.c
    dip/imgproc/histogram.c
    dip/imgproc/fft.c
    input/serial/stm32_uart.c
    output/display//font8.c
    output/display//font12.c
    output/display//font16.c
    output/display//font20.c
    output/display//font24.c
    hdl/ov5640/ov5640.c
    input/camera/stm32_ov5640.c
    input/camera/stm32746g_discovery.c
    input/camera/stm32746g_discovery_sdram.c
    output/serial/serial.c
    output/display/stm32_rk043fn48h.c
    cpp/ImageWrapper.cpp
    cpp/CameraWrapper.cpp
    cpp/DisplayWrapper.cpp
    cpp/MemoryManager.cpp
    cpp/SerialWrapper.cpp
)

# Add CMSIS-DSP source files
file(GLOB_RECURSE CMSIS_DSP_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/Source/*.c
)

file(GLOB_RECURSE CMSIS_DSP_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/Source/**/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/Source/**/*.S
)


target_sources(embedDIP PRIVATE ${CMSIS_DSP_SRCS})
 
target_compile_definitions(embedDIP PUBLIC USE_EMBED_DIP)