add_library(embedDIP)

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
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/STM32F7xx_HAL_Driver/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/Device/ST/STM32F7xx/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Inc
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
    input/serial/stm32_uart.c
    hdl/ov5640/font8.c
    hdl/ov5640/font12.c
    hdl/ov5640/font16.c
    hdl/ov5640/font20.c
    hdl/ov5640/font24.c
    hdl/ov5640/ov5640.c
    input/camera/stm32_ov5640.c
    input/camera/stm32746g_discovery.c
    input/camera/stm32746g_discovery_sdram.c
    output/serial/serial.c
    output/display/stm32_rk043fn48h.c
)

target_compile_definitions(embedDIP PUBLIC USE_EMBED_DIP)