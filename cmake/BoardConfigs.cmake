# ==============================================================================
# embedDIP Board Configuration Mappings
# ==============================================================================
#
# This file maps board selections to their corresponding:
# - Architecture (CPU type)
# - Architecture sources and includes
# - Board sources and includes
# - Device sources (peripherals specific to that board)
# - Compile definitions
# - Link libraries
#
# When user selects a board (e.g., STM32F746G_DISCOVERY), this file
# automatically determines which architecture to use and what code to compile.
#
# ==============================================================================

message(STATUS "Configuring for board: ${EMBEDDIP_BOARD}")

# ==============================================================================
# STM32F746G-Discovery
# ==============================================================================
if(EMBEDDIP_BOARD STREQUAL "STM32F746G_DISCOVERY")
    # Architecture information
    set(EMBEDDIP_ARCH "ARM_CORTEX_M7")
    set(EMBEDDIP_CPU "cortex-m7")
    set(EMBEDDIP_FPU "fpv5-d16")
    set(EMBEDDIP_FLOAT_ABI "hard")

    # Architecture layer sources
    set(ARCH_SOURCES
        arch/arm_cortex_m7/arch_fft.c
        arch/arm_cortex_m7/arch_memory.c
    )

    set(ARCH_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/arch
        ${CMAKE_CURRENT_SOURCE_DIR}/arch/arm_cortex_m7
    )

    set(ARCH_DEFINES
        ARCH_ARM_CORTEX_M7=1
        ARM_MATH_CM7
        __FPU_PRESENT=1
        __FPU_USED=1
    )

    # Board layer sources
    # Note: No board_init needed - this is a library, not a standalone application
    # Board initialization is handled by user's main application (STM32CubeMX, HAL_Init, etc.)
    set(BOARD_SOURCES
        # boards/stm32f746g_discovery/board_init.c  # Not needed - user handles init
    )

    set(BOARD_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/boards/stm32f746g_discovery
    )

    set(BOARD_DEFINES
        BOARD_STM32F746G_DISCOVERY=1
        STM32F746xx
        STM32F7xx
        USE_HAL_DRIVER
    )

    # Try to find STM32 HAL and CMSIS includes
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../Drivers")
        list(APPEND BOARD_INCLUDE_DIRS
            ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/STM32F7xx_HAL_Driver/Inc
            ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/Device/ST/STM32F7xx/Include
            ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/Core/Include
            ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/Include
            ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Inc
        )

        # CMSIS-DSP sources (if building with them)
        file(GLOB_RECURSE CMSIS_DSP_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/Source/*.c
        )
        if(CMSIS_DSP_SOURCES)
            list(APPEND ARCH_SOURCES ${CMSIS_DSP_SOURCES})
            message(STATUS "Added CMSIS-DSP sources: ${CMAKE_MATCH_COUNT} files")
        endif()
    else()
        message(WARNING "STM32 Drivers not found at ../Drivers. You may need to specify paths manually.")
    endif()

    # Device drivers for this board
    set(DEVICE_SOURCES
        device/camera/ov5640/ov5640.c
        device/camera/ov5640/ov5640_reg.c
        device/camera/ov5640/stm32_ov5640.c
        device/camera/ov5640/stm32746g_discovery.c
        device/display/rk043fn48h/stm32_rk043fn48h.c
        device/display/rk043fn48h/font8.c
        device/display/rk043fn48h/font12.c
        device/display/rk043fn48h/font16.c
        device/display/rk043fn48h/font20.c
        device/display/rk043fn48h/font24.c
        device/serial/stm32_uart/stm32_uart.c
    )

    set(DEVICE_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/device
        ${CMAKE_CURRENT_SOURCE_DIR}/device/camera
        ${CMAKE_CURRENT_SOURCE_DIR}/device/camera/ov5640
        ${CMAKE_CURRENT_SOURCE_DIR}/device/display
        ${CMAKE_CURRENT_SOURCE_DIR}/device/display/rk043fn48h
        ${CMAKE_CURRENT_SOURCE_DIR}/device/serial
        ${CMAKE_CURRENT_SOURCE_DIR}/device/serial/stm32_uart
    )

    # Architecture libraries
    set(ARCH_LIBRARIES
        # Math library
        m
        # CMSIS-DSP library (if pre-compiled)
        # ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/Lib/libarm_cortexM7lfdp_math.a
    )

# ==============================================================================
# ESP32-CAM
# ==============================================================================
elseif(EMBEDDIP_BOARD STREQUAL "ESP32_CAM")
    # Architecture information
    set(EMBEDDIP_ARCH "XTENSA_LX6")
    set(EMBEDDIP_CPU "xtensa-lx6")

    # Architecture layer sources
    set(ARCH_SOURCES
        arch/xtensa_lx6/arch_fft.cpp
        arch/xtensa_lx6/arch_memory.cpp
    )

    set(ARCH_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/arch
        ${CMAKE_CURRENT_SOURCE_DIR}/arch/xtensa_lx6
    )

    set(ARCH_DEFINES
        ARCH_XTENSA_LX6=1
        ARDUINO_ARCH_ESP32
        ESP32
    )

    # Board layer sources
    # Note: No board_init needed - this is a library, not a standalone application
    # Board initialization is handled by user's main application (Arduino setup(), ESP-IDF app_main, etc.)
    set(BOARD_SOURCES
        # boards/esp32_cam/board_init.cpp  # Not needed - user handles init
    )

    set(BOARD_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/boards/esp32_cam
    )

    set(BOARD_DEFINES
        BOARD_ESP32_CAM=1
    )

    # Device drivers for this board
    set(DEVICE_SOURCES
        device/camera/ov2640/oc2640.cpp
        device/camera/ov2640/esp32_ov2640.cpp
        device/serial/esp32_uart/esp32_uart.cpp
    )

    set(DEVICE_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/device
        ${CMAKE_CURRENT_SOURCE_DIR}/device/camera
        ${CMAKE_CURRENT_SOURCE_DIR}/device/camera/ov2640
        ${CMAKE_CURRENT_SOURCE_DIR}/device/serial
        ${CMAKE_CURRENT_SOURCE_DIR}/device/serial/esp32_uart
    )

    # ESP32 libraries (ESP-IDF/Arduino framework provides these)
    set(ARCH_LIBRARIES
        # ESP-DSP and other libraries linked via Arduino framework
    )

# ==============================================================================
# HOST (x86/x64 for testing)
# ==============================================================================
elseif(EMBEDDIP_BOARD STREQUAL "HOST")
    # Architecture information
    set(EMBEDDIP_ARCH "HOST")
    set(EMBEDDIP_CPU "native")

    # Architecture layer sources (software implementations)
    set(ARCH_SOURCES
        arch/host/arch_fft.c
        arch/host/arch_memory.c
    )

    set(ARCH_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/arch
        ${CMAKE_CURRENT_SOURCE_DIR}/arch/host
    )

    set(ARCH_DEFINES
        ARCH_HOST=1
    )

    # Board layer sources
    # Note: No board_init needed - this is a library, user app handles init
    set(BOARD_SOURCES
        # boards/host/board_init.c  # Not needed - user handles init
    )

    set(BOARD_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/boards/host
    )

    set(BOARD_DEFINES
        BOARD_HOST=1
    )

    # Device drivers (file-based I/O for simulation)
    set(DEVICE_SOURCES
        device/camera/host/host_camera.c
        device/display/host/host_display.c
        device/serial/host/host_uart.c
    )

    set(DEVICE_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/device
        ${CMAKE_CURRENT_SOURCE_DIR}/device/camera/host
        ${CMAKE_CURRENT_SOURCE_DIR}/device/display/host
        ${CMAKE_CURRENT_SOURCE_DIR}/device/serial/host
    )

    # Standard libraries
    set(ARCH_LIBRARIES
        m       # Math library
        pthread # Threading (for testing)
    )

# ==============================================================================
# Unsupported Board
# ==============================================================================
else()
    message(FATAL_ERROR
        "Unsupported board: ${EMBEDDIP_BOARD}\n"
        "Supported boards:\n"
        "  - STM32F746G_DISCOVERY (ARM Cortex-M7)\n"
        "  - ESP32_CAM (Xtensa LX6)\n"
        "  - HOST (x86/x64 for testing)\n"
        "\n"
        "To add a new board:\n"
        "  1. Create boards/<board_name>/board_config.h\n"
        "  2. Add configuration in cmake/BoardConfigs.cmake\n"
        "  3. See docs/ARCHITECTURE_REFACTORING.md for details"
    )
endif()

# ==============================================================================
# Summary
# ==============================================================================
message(STATUS "")
message(STATUS "Board Configuration Summary:")
message(STATUS "  Board:        ${EMBEDDIP_BOARD}")
message(STATUS "  Architecture: ${EMBEDDIP_ARCH}")
message(STATUS "  CPU:          ${EMBEDDIP_CPU}")
message(STATUS "  Arch sources: ${ARCH_SOURCES}")
message(STATUS "  Board sources: ${BOARD_SOURCES}")
message(STATUS "")
