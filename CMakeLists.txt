cmake_minimum_required(VERSION 3.15)

# Read version from embedDIP.h
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/embedDIP.h" VERSION_MAJOR_LINE REGEX "^#define EMBED_DIP_VERSION_MAJOR")
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/embedDIP.h" VERSION_MINOR_LINE REGEX "^#define EMBED_DIP_VERSION_MINOR")
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/embedDIP.h" VERSION_PATCH_LINE REGEX "^#define EMBED_DIP_VERSION_PATCH")

string(REGEX REPLACE "^.*MAJOR ([0-9]+).*$" "\\1" VERSION_MAJOR "${VERSION_MAJOR_LINE}")
string(REGEX REPLACE "^.*MINOR ([0-9]+).*$" "\\1" VERSION_MINOR "${VERSION_MINOR_LINE}")
string(REGEX REPLACE "^.*PATCH ([0-9]+).*$" "\\1" VERSION_PATCH "${VERSION_PATCH_LINE}")

project(embedDIP
    VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
    LANGUAGES C CXX
    DESCRIPTION "Portable embedded digital image processing library"
)

# === Build Options ===
option(EMBEDDIP_BUILD_TESTS "Build unit tests" OFF)
option(EMBEDDIP_BUILD_EXAMPLES "Build example projects" OFF)
option(EMBEDDIP_BUILD_DOCS "Build documentation with Doxygen" OFF)

# Target platform selection
set(EMBEDDIP_TARGET_PLATFORM "STM32F7" CACHE STRING "Target platform: STM32F7, ESP32, or HOST")
set_property(CACHE EMBEDDIP_TARGET_PLATFORM PROPERTY STRINGS "STM32F7" "ESP32" "HOST")

# Feature flags (can be overridden)
option(EMBEDDIP_ENABLE_UART_LOGGING "Enable UART logging" ON)
option(EMBEDDIP_ENABLE_IMAGE_PROCESSING "Enable image processing modules" ON)
option(EMBEDDIP_ENABLE_CAMERA_INPUT "Enable camera input interfaces" ON)
option(EMBEDDIP_ENABLE_DISPLAY_OUTPUT "Enable display output interfaces" ON)

# === Standards ===
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# === Core Source Files ===
set(CORE_SOURCES
    core/error.c
    core/error.h
    core/memory_manager.h
    core/image.h
)

# === Image Processing Source Files ===
set(IMGPROC_SOURCES
    imgproc/color.c
    imgproc/color.h
    imgproc/filter.c
    imgproc/filter.h
    imgproc/histogram.c
    imgproc/histogram.h
    imgproc/pixel.c
    imgproc/pixel.h
    imgproc/fft.h
)

# === Device Source Files (Common) ===
set(DEVICE_COMMON_SOURCES
    device/camera/camera.h
    device/display/display.h
    device/serial/serial.h
)

# === Wrapper Source Files (C++) ===
set(WRAPPER_SOURCES
    wrapper/CameraWrapper.cpp
    wrapper/CameraWrapper.hpp
    wrapper/DisplayWrapper.cpp
    wrapper/DisplayWrapper.hpp
    wrapper/ImageWrapper.cpp
    wrapper/ImageWrapper.hpp
    wrapper/MemoryManager.cpp
    wrapper/MemoryManager.hpp
    wrapper/SerialWrapper.cpp
    wrapper/SerialWrapper.hpp
)

# === Board-Specific Source Files ===
set(BOARD_COMMON_SOURCES
    board/common.c
    board/common.h
)

# Platform-specific sources
if(EMBEDDIP_TARGET_PLATFORM STREQUAL "STM32F7")
    set(BOARD_SOURCES
        ${BOARD_COMMON_SOURCES}
        board/stm32f7/board_stm32f7_common.c
        board/stm32f7/board_stm32f7_fft.c
        board/stm32f7/board_stm32f7_memory.c
        board/stm32f7/configs.h
    )

    set(DEVICE_SOURCES
        ${DEVICE_COMMON_SOURCES}
        device/camera/ov5640/stm32_ov5640.c
        device/display/rk043fn48h/stm32_rk043fn48h.c
        device/serial/stm32_uart/stm32_uart.c
    )

    set(PLATFORM_DEFINES
        STM32F7xx
        ARM_MATH_CM7
        __FPU_PRESENT=1
        TARGET_BOARD_STM32F7=1
    )

elseif(EMBEDDIP_TARGET_PLATFORM STREQUAL "ESP32")
    set(BOARD_SOURCES
        ${BOARD_COMMON_SOURCES}
        board/esp32/board_esp32_common.cpp
        board/esp32/board_esp32_fft.cpp
        board/esp32/board_esp32_memory.cpp
    )

    set(DEVICE_SOURCES
        ${DEVICE_COMMON_SOURCES}
        device/camera/ov2640/esp32_ov2640.cpp
        device/serial/esp32_uart/esp32_uart.cpp
    )

    set(PLATFORM_DEFINES
        ARDUINO_ARCH_ESP32
        TARGET_BOARD_ESP32=1
    )

elseif(EMBEDDIP_TARGET_PLATFORM STREQUAL "HOST")
    set(BOARD_SOURCES
        ${BOARD_COMMON_SOURCES}
        board/host/memory_host.c
    )

    set(DEVICE_SOURCES
        ${DEVICE_COMMON_SOURCES}
    )

    set(PLATFORM_DEFINES
        TARGET_BOARD_HOST=1
    )

    message(STATUS "HOST build: Using standard malloc/free for memory management")
    message(STATUS "HOST build: Camera and device drivers will be stubs")

else()
    message(FATAL_ERROR "Unknown platform: ${EMBEDDIP_TARGET_PLATFORM}. Must be STM32F7, ESP32, or HOST")
endif()

# === Create Library Target ===
add_library(embedDIP STATIC
    ${CORE_SOURCES}
    ${IMGPROC_SOURCES}
    ${BOARD_SOURCES}
    ${DEVICE_SOURCES}
    ${WRAPPER_SOURCES}
)

# === Compiler Definitions ===
target_compile_definitions(embedDIP PUBLIC
    USE_EMBED_DIP
    ${PLATFORM_DEFINES}
    $<$<BOOL:${EMBEDDIP_ENABLE_UART_LOGGING}>:ENABLE_UART_LOGGING=1>
    $<$<BOOL:${EMBEDDIP_ENABLE_IMAGE_PROCESSING}>:ENABLE_IMAGE_PROCESSING=1>
    $<$<BOOL:${EMBEDDIP_ENABLE_CAMERA_INPUT}>:ENABLE_CAMERA_INPUT=1>
    $<$<BOOL:${EMBEDDIP_ENABLE_DISPLAY_OUTPUT}>:ENABLE_DISPLAY_OUTPUT=1>
)

# === Include Directories ===
target_include_directories(embedDIP PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/core>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/imgproc>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/board>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/device/camera>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/device/display>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/device/serial>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/wrapper>
    $<INSTALL_INTERFACE:include/embedDIP>
)

# Platform-specific includes
if(EMBEDDIP_TARGET_PLATFORM STREQUAL "STM32F7")
    # Try to find CMSIS and HAL includes relative to project
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../Drivers")
        target_include_directories(embedDIP PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/STM32F7xx_HAL_Driver/Inc>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/Device/ST/STM32F7xx/Include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/Core/Include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/Include>
        )

        # CMSIS-DSP sources for STM32
        file(GLOB_RECURSE CMSIS_DSP_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/Source/*.c
        )
        if(CMSIS_DSP_SOURCES)
            target_sources(embedDIP PRIVATE ${CMSIS_DSP_SOURCES})
            message(STATUS "Added CMSIS-DSP sources: ${CMSIS_DSP_SOURCES}")
        endif()
    else()
        message(WARNING "CMSIS/HAL drivers not found. You may need to specify include paths manually.")
    endif()

    target_include_directories(embedDIP PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/board/stm32f7>
    )

elseif(EMBEDDIP_TARGET_PLATFORM STREQUAL "ESP32")
    target_include_directories(embedDIP PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/board/esp32>
    )
endif()

# === Compiler Warnings ===
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(embedDIP PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
    )
endif()

# === Install Targets ===
include(GNUInstallDirs)

install(TARGETS embedDIP
    EXPORT embedDIPTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install headers
install(FILES
    embedDIP.h
    embedDIP.hpp
    embedDIP_configs.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/embedDIP
)

install(DIRECTORY
    core/
    imgproc/
    device/
    board/
    wrapper/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/embedDIP
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# === Export Configuration ===
install(EXPORT embedDIPTargets
    FILE embedDIPTargets.cmake
    NAMESPACE embedDIP::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/embedDIP
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/embedDIPConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/embedDIPConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/embedDIP
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/embedDIPConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/embedDIPConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/embedDIPConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/embedDIP
)

# === Subdirectories ===
if(EMBEDDIP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(EMBEDDIP_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# === Configuration Summary ===
message(STATUS "=================================")
message(STATUS "embedDIP Configuration Summary")
message(STATUS "=================================")
message(STATUS "Version:            ${PROJECT_VERSION}")
message(STATUS "Target Platform:    ${EMBEDDIP_TARGET_PLATFORM}")
message(STATUS "Build Tests:        ${EMBEDDIP_BUILD_TESTS}")
message(STATUS "Build Examples:     ${EMBEDDIP_BUILD_EXAMPLES}")
message(STATUS "Build Docs:         ${EMBEDDIP_BUILD_DOCS}")
message(STATUS "UART Logging:       ${EMBEDDIP_ENABLE_UART_LOGGING}")
message(STATUS "Image Processing:   ${EMBEDDIP_ENABLE_IMAGE_PROCESSING}")
message(STATUS "Camera Input:       ${EMBEDDIP_ENABLE_CAMERA_INPUT}")
message(STATUS "Display Output:     ${EMBEDDIP_ENABLE_DISPLAY_OUTPUT}")
message(STATUS "=================================")
