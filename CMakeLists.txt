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

set(EMBEDDIP_TARGET_BOARD "" CACHE STRING "Target board (required): STM32F7 or ESP32")
set_property(CACHE EMBEDDIP_TARGET_BOARD PROPERTY STRINGS "STM32F7" "ESP32")

set(EMBEDDIP_ARCH "" CACHE STRING "Architecture family (required): ARM or XTENSA")
set_property(CACHE EMBEDDIP_ARCH PROPERTY STRINGS "ARM" "XTENSA")

set(EMBEDDIP_CPU "" CACHE STRING "CPU variant (required): CORTEX_M7, LX6, LX7")
set_property(CACHE EMBEDDIP_CPU PROPERTY STRINGS "CORTEX_M7" "LX6" "LX7")

option(EMBEDDIP_ENABLE_UART_LOGGING "Enable UART logging" ON)
option(EMBEDDIP_ENABLE_IMAGE_PROCESSING "Enable image processing modules" ON)
option(EMBEDDIP_ENABLE_CAMERA_INPUT "Enable camera input interfaces" ON)
option(EMBEDDIP_ENABLE_DISPLAY_OUTPUT "Enable display output interfaces" ON)
option(EMBEDDIP_BUILD_DOCS "Build documentation with Doxygen" OFF)

if(EMBEDDIP_TARGET_BOARD STREQUAL "")
    message(FATAL_ERROR "EMBEDDIP_TARGET_BOARD is required. Supported values: STM32F7, ESP32")
endif()

if(EMBEDDIP_ARCH STREQUAL "")
    message(FATAL_ERROR "EMBEDDIP_ARCH is required. Supported values: ARM, XTENSA")
endif()

if(EMBEDDIP_CPU STREQUAL "")
    message(FATAL_ERROR "EMBEDDIP_CPU is required. Supported values: CORTEX_M7, LX6, LX7")
endif()

# Explicit compatibility matrix between board, architecture family and CPU
set(_embeddip_pair_valid FALSE)
if(EMBEDDIP_TARGET_BOARD STREQUAL "STM32F7")
    if(EMBEDDIP_ARCH STREQUAL "ARM" AND EMBEDDIP_CPU STREQUAL "CORTEX_M7")
        set(_embeddip_pair_valid TRUE)
    endif()
elseif(EMBEDDIP_TARGET_BOARD STREQUAL "ESP32")
    if(EMBEDDIP_ARCH STREQUAL "XTENSA" AND (EMBEDDIP_CPU STREQUAL "LX6" OR EMBEDDIP_CPU STREQUAL "LX7"))
        set(_embeddip_pair_valid TRUE)
    endif()
endif()

if(NOT _embeddip_pair_valid)
    message(FATAL_ERROR
        "Invalid board/arch/cpu combination: ${EMBEDDIP_TARGET_BOARD} + ${EMBEDDIP_ARCH} + ${EMBEDDIP_CPU}. "
        "Supported: STM32F7+ARM+CORTEX_M7, ESP32+XTENSA+LX6, ESP32+XTENSA+LX7"
    )
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CORE_SOURCES
    core/error.c
    core/error.h
    core/memory_manager.h
    core/image.h
)

set(IMGPROC_SOURCES
    # Main header (includes all sub-modules)
    imgproc/pixel.h

    # Compression
    imgproc/compress.c
    imgproc/compress.h

    # Color operations
    imgproc/color.c
    imgproc/color.h

    # Filtering
    imgproc/filter.c
    imgproc/filter.h

    # Histogram operations
    imgproc/histogram.c
    imgproc/histogram.h

    # Thresholding and intensity transforms
    imgproc/thresh.c
    imgproc/thresh.h

    # Morphological operations
    imgproc/morph.c
    imgproc/morph.h

    # Segmentation algorithms
    imgproc/segmentation.c
    imgproc/segmentation.h

    # Connected components
    imgproc/connectedcomponents.c
    imgproc/connectedcomponents.h

    # Drawing primitives
    imgproc/drawing.c
    imgproc/drawing.h

    # Hough transform
    imgproc/hough.c
    imgproc/hough.h

    # Geometric transformations
    imgproc/imgwarp.c
    imgproc/imgwarp.h

    # Arithmetic and logical operations
    imgproc/misc.c
    imgproc/misc.h

    # FFT operations
    imgproc/fft.c
    imgproc/fft.h
)

set(DEVICE_COMMON_SOURCES
    device/camera/camera.h
    device/display/display.h
    device/serial/serial.h
)

set(WRAPPER_SOURCES
    wrapper/CameraWrapper.cpp
    wrapper/CameraWrapper.hpp
    wrapper/DisplayWrapper.cpp
    wrapper/DisplayWrapper.hpp
    wrapper/ImageWrapper.cpp
    wrapper/ImageWrapper.hpp
    wrapper/MemoryManager.hpp
    wrapper/SerialWrapper.cpp
    wrapper/SerialWrapper.hpp
)

set(BOARD_COMMON_SOURCES
    board/common.c
    board/common.h
)

# Load board and architecture profiles (kept next to board/arch source trees
# to make onboarding new ports straightforward).
string(TOLOWER "${EMBEDDIP_TARGET_BOARD}" EMBEDDIP_BOARD_PROFILE)
string(TOLOWER "${EMBEDDIP_ARCH}" EMBEDDIP_ARCH_PROFILE)

set(EMBEDDIP_BOARD_PROFILE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/board/${EMBEDDIP_BOARD_PROFILE}/board_profile.cmake")
set(EMBEDDIP_ARCH_PROFILE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/arch/${EMBEDDIP_ARCH_PROFILE}/arch_profile.cmake")

if(NOT EXISTS "${EMBEDDIP_BOARD_PROFILE_FILE}")
    message(FATAL_ERROR "Board profile not found: ${EMBEDDIP_BOARD_PROFILE_FILE}")
endif()
if(NOT EXISTS "${EMBEDDIP_ARCH_PROFILE_FILE}")
    message(FATAL_ERROR "Architecture profile not found: ${EMBEDDIP_ARCH_PROFILE_FILE}")
endif()

include("${EMBEDDIP_BOARD_PROFILE_FILE}")
include("${EMBEDDIP_ARCH_PROFILE_FILE}")

# === Create Library Target ===
add_library(embedDIP STATIC
    ${CORE_SOURCES}
    ${IMGPROC_SOURCES}
    ${EMBEDDIP_BOARD_SOURCES}
    ${EMBEDDIP_ARCH_SOURCES}
    ${EMBEDDIP_DEVICE_SOURCES}
    ${WRAPPER_SOURCES}
)

# === Compiler Definitions ===
target_compile_definitions(embedDIP PUBLIC
    USE_EMBED_DIP
    ${EMBEDDIP_BOARD_DEFINES}
    ${EMBEDDIP_ARCH_DEFINES}
    $<$<BOOL:${EMBEDDIP_ENABLE_UART_LOGGING}>:ENABLE_UART_LOGGING=1>
    $<$<BOOL:${EMBEDDIP_ENABLE_IMAGE_PROCESSING}>:ENABLE_IMAGE_PROCESSING=1>
    $<$<BOOL:${EMBEDDIP_ENABLE_CAMERA_INPUT}>:ENABLE_CAMERA_INPUT=1>
    $<$<BOOL:${EMBEDDIP_ENABLE_DISPLAY_OUTPUT}>:ENABLE_DISPLAY_OUTPUT=1>
)

if(EMBEDDIP_ARCH_PRIVATE_DEFINES)
    target_compile_definitions(embedDIP PRIVATE ${EMBEDDIP_ARCH_PRIVATE_DEFINES})
endif()

if(EMBEDDIP_ARCH_COMPILE_OPTIONS)
    target_compile_options(embedDIP PUBLIC ${EMBEDDIP_ARCH_COMPILE_OPTIONS})
endif()

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

if(EMBEDDIP_BOARD_INCLUDE_DIRS)
    foreach(_embeddip_board_inc IN LISTS EMBEDDIP_BOARD_INCLUDE_DIRS)
        target_include_directories(embedDIP PUBLIC
            $<BUILD_INTERFACE:${_embeddip_board_inc}>
        )
    endforeach()
endif()

# Board-specific include dependencies from parent project layout
if(EMBEDDIP_TARGET_BOARD STREQUAL "STM32F7")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../Drivers")
        target_include_directories(embedDIP PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../Middlewares/Third_Party/LibJPEG/include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../LIBJPEG/Target>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/STM32F7xx_HAL_Driver/Inc>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/Device/ST/STM32F7xx/Include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/Core/Include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/Include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../Core/Inc>
        )

        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../Middlewares/Third_Party/LibJPEG/include/jpeglib.h")
            target_compile_definitions(embedDIP PUBLIC EMBEDDIP_HAVE_LIBJPEG=1)
        else()
            message(WARNING "LibJPEG headers not found for embedDIP compression module.")
        endif()

        # CMSIS-DSP sources for STM32 (C and assembly files)
        file(GLOB_RECURSE CMSIS_DSP_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/Source/*.c
            ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/Source/*.S
        )
        if(CMSIS_DSP_SOURCES)
            target_sources(embedDIP PRIVATE ${CMSIS_DSP_SOURCES})
            set_source_files_properties(${CMSIS_DSP_SOURCES}
                PROPERTIES COMPILE_DEFINITIONS "__FPU_PRESENT=1"
            )
            message(STATUS "Added CMSIS-DSP sources (C + ASM)")
        endif()
    else()
        message(WARNING "CMSIS/HAL drivers not found. You may need to specify include paths manually.")
    endif()
endif()

# === Link Libraries ===
target_link_libraries(embedDIP PUBLIC m)

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
    ${CMAKE_CURRENT_SOURCE_DIR}/embedDIPConfig.cmake.in
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
