/* ========================================================================== */
/*  File: test_error.c                                                        */
/*  Brief: Unit tests for error handling system                               */
/*  SPDX-License-Identifier: MIT                                              */
/*  Copyright (c) 2024–2025                                                   */
/* ========================================================================== */

#include "unity.h"
#include "core/error.h"
#include <string.h>

/* Test Setup and Teardown */
void setUp(void)
{
    /* Called before each test */
}

void tearDown(void)
{
    /* Called after each test */
}

/* Test Cases */

void test_embeddip_status_str_success(void)
{
    const char* msg = embeddip_status_str(EMBEDDIP_OK);
    TEST_ASSERT_NOT_NULL(msg);
    TEST_ASSERT_EQUAL_STRING("Success", msg);
}

void test_embeddip_status_str_memory_errors(void)
{
    TEST_ASSERT_EQUAL_STRING("Out of memory",
        embeddip_status_str(EMBEDDIP_ERROR_OUT_OF_MEMORY));
    TEST_ASSERT_EQUAL_STRING("Null pointer",
        embeddip_status_str(EMBEDDIP_ERROR_NULL_PTR));
}

void test_embeddip_status_str_validation_errors(void)
{
    TEST_ASSERT_EQUAL_STRING("Invalid argument",
        embeddip_status_str(EMBEDDIP_ERROR_INVALID_ARG));
    TEST_ASSERT_EQUAL_STRING("Invalid format",
        embeddip_status_str(EMBEDDIP_ERROR_INVALID_FORMAT));
    TEST_ASSERT_EQUAL_STRING("Invalid size",
        embeddip_status_str(EMBEDDIP_ERROR_INVALID_SIZE));
    TEST_ASSERT_EQUAL_STRING("Invalid depth",
        embeddip_status_str(EMBEDDIP_ERROR_INVALID_DEPTH));
}

void test_embeddip_status_str_operation_errors(void)
{
    TEST_ASSERT_EQUAL_STRING("Operation not supported",
        embeddip_status_str(EMBEDDIP_ERROR_NOT_SUPPORTED));
    TEST_ASSERT_EQUAL_STRING("Not initialized",
        embeddip_status_str(EMBEDDIP_ERROR_NOT_INITIALIZED));
    TEST_ASSERT_EQUAL_STRING("Resource busy",
        embeddip_status_str(EMBEDDIP_ERROR_BUSY));
    TEST_ASSERT_EQUAL_STRING("Timeout",
        embeddip_status_str(EMBEDDIP_ERROR_TIMEOUT));
}

void test_embeddip_status_str_device_errors(void)
{
    TEST_ASSERT_EQUAL_STRING("Device error",
        embeddip_status_str(EMBEDDIP_ERROR_DEVICE_ERROR));
    TEST_ASSERT_EQUAL_STRING("I/O error",
        embeddip_status_str(EMBEDDIP_ERROR_IO_ERROR));
    TEST_ASSERT_EQUAL_STRING("Communication error",
        embeddip_status_str(EMBEDDIP_ERROR_COMMUNICATION));
}

void test_embeddip_status_str_data_errors(void)
{
    TEST_ASSERT_EQUAL_STRING("Overflow",
        embeddip_status_str(EMBEDDIP_ERROR_OVERFLOW));
    TEST_ASSERT_EQUAL_STRING("Underflow",
        embeddip_status_str(EMBEDDIP_ERROR_UNDERFLOW));
    TEST_ASSERT_EQUAL_STRING("Out of range",
        embeddip_status_str(EMBEDDIP_ERROR_OUT_OF_RANGE));
}

void test_embeddip_status_str_unknown_error(void)
{
    TEST_ASSERT_EQUAL_STRING("Unknown error",
        embeddip_status_str(EMBEDDIP_ERROR_UNKNOWN));
}

void test_embeddip_status_str_unrecognized_code(void)
{
    const char* msg = embeddip_status_str((embeddip_status_t)-999);
    TEST_ASSERT_NOT_NULL(msg);
    TEST_ASSERT_EQUAL_STRING("Unrecognized error code", msg);
}

void test_embeddip_success_helper(void)
{
    TEST_ASSERT_TRUE(embeddip_success(EMBEDDIP_OK));
    TEST_ASSERT_FALSE(embeddip_success(EMBEDDIP_ERROR_NULL_PTR));
    TEST_ASSERT_FALSE(embeddip_success(EMBEDDIP_ERROR_OUT_OF_MEMORY));
    TEST_ASSERT_FALSE(embeddip_success(EMBEDDIP_ERROR_INVALID_ARG));
}

void test_embeddip_failed_helper(void)
{
    TEST_ASSERT_FALSE(embeddip_failed(EMBEDDIP_OK));
    TEST_ASSERT_TRUE(embeddip_failed(EMBEDDIP_ERROR_NULL_PTR));
    TEST_ASSERT_TRUE(embeddip_failed(EMBEDDIP_ERROR_OUT_OF_MEMORY));
    TEST_ASSERT_TRUE(embeddip_failed(EMBEDDIP_ERROR_INVALID_ARG));
}

/* Main Test Runner */
int main(void)
{
    UnityBegin("test_error.c");

    RUN_TEST(test_embeddip_status_str_success);
    RUN_TEST(test_embeddip_status_str_memory_errors);
    RUN_TEST(test_embeddip_status_str_validation_errors);
    RUN_TEST(test_embeddip_status_str_operation_errors);
    RUN_TEST(test_embeddip_status_str_device_errors);
    RUN_TEST(test_embeddip_status_str_data_errors);
    RUN_TEST(test_embeddip_status_str_unknown_error);
    RUN_TEST(test_embeddip_status_str_unrecognized_code);
    RUN_TEST(test_embeddip_success_helper);
    RUN_TEST(test_embeddip_failed_helper);

    return UnityEnd();
}
