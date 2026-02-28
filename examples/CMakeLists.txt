# ============================================================================
#  embedDIP Examples
#  SPDX-License-Identifier: MIT
#  Copyright (c) 2024-2025
# ============================================================================

cmake_minimum_required(VERSION 3.15)

message(STATUS "=================================")
message(STATUS "embedDIP Examples Configuration")
message(STATUS "=================================")
message(STATUS "Platform: ${EMBEDDIP_TARGET_PLATFORM}")

# Common include directories for all examples
set(EXAMPLE_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/imgproc
    ${CMAKE_SOURCE_DIR}/device
    ${CMAKE_SOURCE_DIR}/board
)

# Helper function to create an example executable
function(add_embeddip_example EXAMPLE_NAME EXAMPLE_DIR)
    set(TARGET_NAME example_${EXAMPLE_NAME})

    add_executable(${TARGET_NAME}
        ${EXAMPLE_DIR}/main.c
    )

    target_include_directories(${TARGET_NAME} PRIVATE
        ${EXAMPLE_INCLUDE_DIRS}
    )

    target_link_libraries(${TARGET_NAME}
        embedDIP
    )

    # Set output directory
    set_target_properties(${TARGET_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/${EXAMPLE_NAME}
    )

    message(STATUS "  [+] Example: ${EXAMPLE_NAME}")
endfunction()

# ============================================================================
# Platform-Specific Examples
# ============================================================================

if(EMBEDDIP_TARGET_PLATFORM STREQUAL "HOST")
    # All examples work on HOST platform
    add_embeddip_example(01_basic_image ${CMAKE_CURRENT_SOURCE_DIR}/01_basic_image)
    add_embeddip_example(02_color_conversion ${CMAKE_CURRENT_SOURCE_DIR}/02_color_conversion)
    add_embeddip_example(03_host_camera ${CMAKE_CURRENT_SOURCE_DIR}/03_host_camera)
    add_embeddip_example(04_filter2d_example ${CMAKE_CURRENT_SOURCE_DIR}/04_filter2d_example)

elseif(EMBEDDIP_TARGET_PLATFORM STREQUAL "STM32F7")
    # Basic examples work on STM32F7
    add_embeddip_example(01_basic_image ${CMAKE_CURRENT_SOURCE_DIR}/01_basic_image)
    add_embeddip_example(02_color_conversion ${CMAKE_CURRENT_SOURCE_DIR}/02_color_conversion)

    message(STATUS "  [!] Note: Camera/display examples require hardware configuration")

elseif(EMBEDDIP_TARGET_PLATFORM STREQUAL "ESP32")
    # Basic examples work on ESP32
    add_embeddip_example(01_basic_image ${CMAKE_CURRENT_SOURCE_DIR}/01_basic_image)
    add_embeddip_example(02_color_conversion ${CMAKE_CURRENT_SOURCE_DIR}/02_color_conversion)

    message(STATUS "  [!] Note: Camera/display examples require hardware configuration")

endif()

message(STATUS "=================================")
