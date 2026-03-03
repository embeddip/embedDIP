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

# Board selection (replaces old EMBEDDIP_TARGET_PLATFORM)
# Board selection automatically determines architecture (CPU type)
set(EMBEDDIP_BOARD "STM32F746G_DISCOVERY" CACHE STRING "Target board: STM32F746G_DISCOVERY, ESP32_CAM, or HOST")
set_property(CACHE EMBEDDIP_BOARD PROPERTY STRINGS "STM32F746G_DISCOVERY" "ESP32_CAM" "HOST")

# Include board configuration mappings
# This file sets: EMBEDDIP_ARCH, ARCH_SOURCES, BOARD_SOURCES, DEVICE_SOURCES,
#                 ARCH_INCLUDE_DIRS, BOARD_INCLUDE_DIRS, DEVICE_INCLUDE_DIRS,
#                 ARCH_DEFINES, BOARD_DEFINES, ARCH_LIBRARIES
include(cmake/BoardConfigs.cmake)

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

# === Legacy Board Source Files ===
# Keep backward compatibility with old board/ structure during migration
set(BOARD_COMMON_SOURCES
    board/common.c
    board/common.h
)

# Note: Board-specific sources, architecture sources, and device sources
# are now configured in cmake/BoardConfigs.cmake based on EMBEDDIP_BOARD
# The following variables are set by BoardConfigs.cmake:
#   - ARCH_SOURCES: Architecture-specific implementations (FFT, memory)
#   - BOARD_SOURCES: Board-specific initialization and configuration
#   - DEVICE_SOURCES: Device drivers for peripherals on this board
#   - ARCH_DEFINES: Architecture compile definitions
#   - BOARD_DEFINES: Board-specific compile definitions

# Combine with legacy sources for backward compatibility
list(APPEND BOARD_SOURCES ${BOARD_COMMON_SOURCES})

# === Create Library Target ===
add_library(embedDIP STATIC
    ${CORE_SOURCES}
    ${IMGPROC_SOURCES}
    ${ARCH_SOURCES}      # Architecture layer (CPU-specific: FFT, memory)
    ${BOARD_SOURCES}     # Board layer (hardware config and init)
    ${DEVICE_SOURCES}    # Device drivers (peripherals)
    ${WRAPPER_SOURCES}
)

# === Compiler Definitions ===
target_compile_definitions(embedDIP PUBLIC
    USE_EMBED_DIP
    ${ARCH_DEFINES}      # Architecture-specific defines (from BoardConfigs.cmake)
    ${BOARD_DEFINES}     # Board-specific defines (from BoardConfigs.cmake)
    $<$<BOOL:${EMBEDDIP_ENABLE_UART_LOGGING}>:ENABLE_UART_LOGGING=1>
    $<$<BOOL:${EMBEDDIP_ENABLE_IMAGE_PROCESSING}>:ENABLE_IMAGE_PROCESSING=1>
    $<$<BOOL:${EMBEDDIP_ENABLE_CAMERA_INPUT}>:ENABLE_CAMERA_INPUT=1>
    $<$<BOOL:${EMBEDDIP_ENABLE_DISPLAY_OUTPUT}>:ENABLE_DISPLAY_OUTPUT=1>
)

# === Include Directories ===
target_include_directories(embedDIP PUBLIC
    # Core library includes
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/core>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/imgproc>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/wrapper>

    # Legacy board includes (for backward compatibility during migration)
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/board>

    # Device driver includes (common)
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/device>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/device/camera>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/device/display>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/device/serial>

    # Install interface
    $<INSTALL_INTERFACE:include/embedDIP>
)

# Add architecture-specific includes (set by BoardConfigs.cmake)
if(ARCH_INCLUDE_DIRS)
    foreach(dir IN LISTS ARCH_INCLUDE_DIRS)
        target_include_directories(embedDIP PUBLIC
            $<BUILD_INTERFACE:${dir}>
        )
    endforeach()
endif()

# Add board-specific includes (set by BoardConfigs.cmake)
if(BOARD_INCLUDE_DIRS)
    foreach(dir IN LISTS BOARD_INCLUDE_DIRS)
        target_include_directories(embedDIP PUBLIC
            $<BUILD_INTERFACE:${dir}>
        )
    endforeach()
endif()

# Add device-specific includes (set by BoardConfigs.cmake)
if(DEVICE_INCLUDE_DIRS)
    foreach(dir IN LISTS DEVICE_INCLUDE_DIRS)
        target_include_directories(embedDIP PUBLIC
            $<BUILD_INTERFACE:${dir}>
        )
    endforeach()
endif()

# === Link Libraries ===
target_link_libraries(embedDIP PUBLIC m)  # Math library for powf(), etc.

# Add architecture-specific libraries (set by BoardConfigs.cmake)
if(ARCH_LIBRARIES)
    target_link_libraries(embedDIP PUBLIC ${ARCH_LIBRARIES})
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
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt)
        add_subdirectory(examples)
    else()
        message(WARNING "Examples directory exists but CMakeLists.txt not found")
    endif()
endif()

# === Configuration Summary ===
message(STATUS "=================================")
message(STATUS "embedDIP Configuration Summary")
message(STATUS "=================================")
message(STATUS "Version:            ${PROJECT_VERSION}")
message(STATUS "Target Board:       ${EMBEDDIP_BOARD}")
message(STATUS "Architecture:       ${EMBEDDIP_ARCH}")
message(STATUS "Build Tests:        ${EMBEDDIP_BUILD_TESTS}")
message(STATUS "Build Examples:     ${EMBEDDIP_BUILD_EXAMPLES}")
message(STATUS "Build Docs:         ${EMBEDDIP_BUILD_DOCS}")
message(STATUS "UART Logging:       ${EMBEDDIP_ENABLE_UART_LOGGING}")
message(STATUS "Image Processing:   ${EMBEDDIP_ENABLE_IMAGE_PROCESSING}")
message(STATUS "Camera Input:       ${EMBEDDIP_ENABLE_CAMERA_INPUT}")
message(STATUS "Display Output:     ${EMBEDDIP_ENABLE_DISPLAY_OUTPUT}")
message(STATUS "=================================")
