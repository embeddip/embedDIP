# ============================================================================
#  embedDIP Test Suite
#  SPDX-License-Identifier: MIT
#  Copyright (c) 2024-2025
# ============================================================================

cmake_minimum_required(VERSION 3.15)

# Enable testing
enable_testing()

# Unity test framework
add_library(unity STATIC
    unity/unity.c
    unity/unity.h
)

target_include_directories(unity PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/unity
)

# Common test configuration
set(TEST_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/board
    ${CMAKE_SOURCE_DIR}/imgproc
    ${CMAKE_SOURCE_DIR}/device
    ${CMAKE_CURRENT_SOURCE_DIR}/unity
)

# Helper function to add a test
function(add_embeddip_test TEST_NAME TEST_SOURCE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})

    target_include_directories(${TEST_NAME} PRIVATE ${TEST_INCLUDE_DIRS})

    target_link_libraries(${TEST_NAME}
        embedDIP
        unity
    )

    # Add to CTest
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})

    # Set test properties
    set_tests_properties(${TEST_NAME} PROPERTIES
        TIMEOUT 30
        LABELS "unit"
    )
endfunction()

# Test files
add_embeddip_test(test_image test_image.c)
add_embeddip_test(test_color test_color.c)
add_embeddip_test(test_error test_error.c)

# Summary
message(STATUS "embedDIP Tests Configuration:")
message(STATUS "  Unity framework: Enabled")
message(STATUS "  Test executables: test_image, test_color, test_error")
message(STATUS "  Run tests with: ctest or make test")
