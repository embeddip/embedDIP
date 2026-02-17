/* ==========================================
    Unity Project - A Test Framework for C
    Copyright (c) 2007-24 Mike Karlesky, Mark VanderVoord, Greg Williams
    [Released under MIT License. Please refer to license.txt for details]
========================================== */

#ifndef UNITY_FRAMEWORK_H
#define UNITY_FRAMEWORK_H

#include <stdint.h>
#include <stddef.h>
#include <setjmp.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Unity Configuration */
#ifndef UNITY_EXCLUDE_FLOAT
#define UNITY_EXCLUDE_FLOAT
#endif

#ifndef UNITY_EXCLUDE_DOUBLE
#define UNITY_EXCLUDE_DOUBLE
#endif

/* Unity Internal Structures */
struct UNITY_STORAGE_T
{
    const char* TestFile;
    const char* CurrentTestName;
    uint32_t CurrentTestLineNumber;
    uint32_t NumberOfTests;
    uint32_t TestFailures;
    uint32_t TestIgnores;
    uint8_t CurrentTestFailed;
    uint8_t CurrentTestIgnored;
    jmp_buf AbortFrame;
};

extern struct UNITY_STORAGE_T Unity;

/* Test Running Macros */
#define RUN_TEST(func) UnityDefaultTestRun(func, #func, __LINE__)

/* Test Assertion Macros */
#define TEST_FAIL_MESSAGE(message)  UnityFail(message, __LINE__)
#define TEST_FAIL()                 TEST_FAIL_MESSAGE(NULL)
#define TEST_IGNORE_MESSAGE(message) UnityIgnore(message, __LINE__)
#define TEST_IGNORE()               TEST_IGNORE_MESSAGE(NULL)
#define TEST_MESSAGE(message)       UnityMessage(message, __LINE__)

#define TEST_ASSERT(condition) \
    do { if (!(condition)) { TEST_FAIL_MESSAGE("Expected TRUE Was FALSE"); } } while(0)

#define TEST_ASSERT_TRUE(condition) TEST_ASSERT(condition)
#define TEST_ASSERT_FALSE(condition) TEST_ASSERT(!(condition))

#define TEST_ASSERT_NULL(pointer) \
    do { if ((pointer) != NULL) { TEST_FAIL_MESSAGE("Expected NULL"); } } while(0)

#define TEST_ASSERT_NOT_NULL(pointer) \
    do { if ((pointer) == NULL) { TEST_FAIL_MESSAGE("Expected Non-NULL"); } } while(0)

#define TEST_ASSERT_EQUAL_INT(expected, actual) \
    UnityAssertEqualNumber((int)(expected), (int)(actual), NULL, __LINE__, UNITY_DISPLAY_STYLE_INT)

#define TEST_ASSERT_EQUAL_UINT(expected, actual) \
    UnityAssertEqualNumber((int)(expected), (int)(actual), NULL, __LINE__, UNITY_DISPLAY_STYLE_UINT)

#define TEST_ASSERT_EQUAL_HEX8(expected, actual) \
    UnityAssertEqualNumber((int)(expected), (int)(actual), NULL, __LINE__, UNITY_DISPLAY_STYLE_HEX8)

#define TEST_ASSERT_EQUAL_HEX16(expected, actual) \
    UnityAssertEqualNumber((int)(expected), (int)(actual), NULL, __LINE__, UNITY_DISPLAY_STYLE_HEX16)

#define TEST_ASSERT_EQUAL_HEX32(expected, actual) \
    UnityAssertEqualNumber((int)(expected), (int)(actual), NULL, __LINE__, UNITY_DISPLAY_STYLE_HEX32)

#define TEST_ASSERT_EQUAL_PTR(expected, actual) \
    UnityAssertEqualNumber((intptr_t)(expected), (intptr_t)(actual), NULL, __LINE__, UNITY_DISPLAY_STYLE_POINTER)

#define TEST_ASSERT_EQUAL_STRING(expected, actual) \
    UnityAssertEqualString((const char*)(expected), (const char*)(actual), NULL, __LINE__)

#define TEST_ASSERT_EQUAL_MEMORY(expected, actual, len) \
    UnityAssertEqualMemory((void*)(expected), (void*)(actual), (uint32_t)(len), 1, NULL, __LINE__)

/* Display Styles */
typedef enum
{
    UNITY_DISPLAY_STYLE_INT,
    UNITY_DISPLAY_STYLE_UINT,
    UNITY_DISPLAY_STYLE_HEX8,
    UNITY_DISPLAY_STYLE_HEX16,
    UNITY_DISPLAY_STYLE_HEX32,
    UNITY_DISPLAY_STYLE_POINTER
} UNITY_DISPLAY_STYLE_T;

/* Unity Functions */
void UnityBegin(const char* filename);
int UnityEnd(void);
void UnityDefaultTestRun(void (*Func)(void), const char* FuncName, const int FuncLineNum);
void UnityFail(const char* message, const int line);
void UnityIgnore(const char* message, const int line);
void UnityMessage(const char* message, const int line);

void UnityAssertEqualNumber(const int expected, const int actual,
                           const char* msg, const unsigned short line,
                           const UNITY_DISPLAY_STYLE_T style);

void UnityAssertEqualString(const char* expected, const char* actual,
                           const char* msg, const unsigned short line);

void UnityAssertEqualMemory(const void* expected, const void* actual,
                           const uint32_t length, const uint32_t num_elements,
                           const char* msg, const unsigned short line);

#ifdef __cplusplus
}
#endif

#endif /* UNITY_FRAMEWORK_H */
