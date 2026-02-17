/* ==========================================
    Unity Project - A Test Framework for C
    Copyright (c) 2007-24 Mike Karlesky, Mark VanderVoord, Greg Williams
    [Released under MIT License. Please refer to license.txt for details]
========================================== */

#include "unity.h"
#include <stdio.h>
#include <string.h>

struct UNITY_STORAGE_T Unity;

/* Forward declarations */
static void UnityPrint(const char* string);
static void UnityPrintNumber(const int number);
static void UnityPrintHex(const unsigned int number, const char width);

/* Begin a test suite */
void UnityBegin(const char* filename)
{
    Unity.TestFile = filename;
    Unity.CurrentTestName = NULL;
    Unity.CurrentTestLineNumber = 0;
    Unity.NumberOfTests = 0;
    Unity.TestFailures = 0;
    Unity.TestIgnores = 0;
    Unity.CurrentTestFailed = 0;
    Unity.CurrentTestIgnored = 0;
}

/* End a test suite and return number of failures */
int UnityEnd(void)
{
    UnityPrint("\n-----------------------\n");
    UnityPrintNumber(Unity.NumberOfTests);
    UnityPrint(" Tests ");
    UnityPrintNumber(Unity.TestFailures);
    UnityPrint(" Failures ");
    UnityPrintNumber(Unity.TestIgnores);
    UnityPrint(" Ignored\n");

    if (Unity.TestFailures == 0U)
    {
        UnityPrint("OK\n");
    }
    else
    {
        UnityPrint("FAIL\n");
    }

    return (int)(Unity.TestFailures);
}

/* Run a test function */
void UnityDefaultTestRun(void (*Func)(void), const char* FuncName, const int FuncLineNum)
{
    Unity.CurrentTestName = FuncName;
    Unity.CurrentTestLineNumber = (uint32_t)FuncLineNum;
    Unity.NumberOfTests++;
    Unity.CurrentTestFailed = 0;
    Unity.CurrentTestIgnored = 0;

    if (setjmp(Unity.AbortFrame) == 0)
    {
        Func();
    }

    if (Unity.CurrentTestIgnored)
    {
        Unity.TestIgnores++;
        UnityPrint(FuncName);
        UnityPrint(":IGNORE\n");
    }
    else if (Unity.CurrentTestFailed)
    {
        Unity.TestFailures++;
    }
    else
    {
        UnityPrint(FuncName);
        UnityPrint(":PASS\n");
    }
}

/* Test failure handler */
void UnityFail(const char* message, const int line)
{
    Unity.CurrentTestFailed = 1;
    UnityPrint(Unity.CurrentTestName);
    UnityPrint(":");
    UnityPrintNumber(line);
    UnityPrint(":FAIL:");

    if (message != NULL)
    {
        UnityPrint(message);
    }

    UnityPrint("\n");
    longjmp(Unity.AbortFrame, 1);
}

/* Test ignore handler */
void UnityIgnore(const char* message, const int line)
{
    Unity.CurrentTestIgnored = 1;

    UnityPrint(Unity.CurrentTestName);
    UnityPrint(":");
    UnityPrintNumber(line);
    UnityPrint(":IGNORE:");

    if (message != NULL)
    {
        UnityPrint(message);
    }

    UnityPrint("\n");
    longjmp(Unity.AbortFrame, 1);
}

/* Test message */
void UnityMessage(const char* message, const int line)
{
    UnityPrint(Unity.CurrentTestName);
    UnityPrint(":");
    UnityPrintNumber(line);
    UnityPrint(":INFO:");

    if (message != NULL)
    {
        UnityPrint(message);
    }

    UnityPrint("\n");
}

/* Assert equal numbers */
void UnityAssertEqualNumber(const int expected, const int actual,
                           const char* msg, const unsigned short line,
                           const UNITY_DISPLAY_STYLE_T style)
{
    if (expected != actual)
    {
        Unity.CurrentTestFailed = 1;
        UnityPrint(Unity.CurrentTestName);
        UnityPrint(":");
        UnityPrintNumber((int)line);
        UnityPrint(":FAIL: Expected ");

        switch (style)
        {
            case UNITY_DISPLAY_STYLE_HEX8:
            case UNITY_DISPLAY_STYLE_HEX16:
            case UNITY_DISPLAY_STYLE_HEX32:
                UnityPrintHex((unsigned int)expected, (char)style);
                UnityPrint(" Was ");
                UnityPrintHex((unsigned int)actual, (char)style);
                break;
            case UNITY_DISPLAY_STYLE_POINTER:
                UnityPrint("0x");
                UnityPrintHex((unsigned int)expected, 8);
                UnityPrint(" Was 0x");
                UnityPrintHex((unsigned int)actual, 8);
                break;
            default:
                UnityPrintNumber(expected);
                UnityPrint(" Was ");
                UnityPrintNumber(actual);
                break;
        }

        if (msg != NULL)
        {
            UnityPrint(" ");
            UnityPrint(msg);
        }

        UnityPrint("\n");
        longjmp(Unity.AbortFrame, 1);
    }
}

/* Assert equal strings */
void UnityAssertEqualString(const char* expected, const char* actual,
                           const char* msg, const unsigned short line)
{
    if (expected == NULL && actual == NULL)
    {
        return;
    }

    if (expected == NULL || actual == NULL || strcmp(expected, actual) != 0)
    {
        Unity.CurrentTestFailed = 1;
        UnityPrint(Unity.CurrentTestName);
        UnityPrint(":");
        UnityPrintNumber((int)line);
        UnityPrint(":FAIL: Expected \"");
        UnityPrint(expected ? expected : "NULL");
        UnityPrint("\" Was \"");
        UnityPrint(actual ? actual : "NULL");
        UnityPrint("\"");

        if (msg != NULL)
        {
            UnityPrint(" ");
            UnityPrint(msg);
        }

        UnityPrint("\n");
        longjmp(Unity.AbortFrame, 1);
    }
}

/* Assert equal memory */
void UnityAssertEqualMemory(const void* expected, const void* actual,
                           const uint32_t length, const uint32_t num_elements,
                           const char* msg, const unsigned short line)
{
    const unsigned char* ptr_exp = (const unsigned char*)expected;
    const unsigned char* ptr_act = (const unsigned char*)actual;
    uint32_t elements = length * num_elements;

    if (expected == actual)
    {
        return;
    }

    if (expected == NULL || actual == NULL)
    {
        UnityFail("Null Pointer", (int)line);
        return;
    }

    for (uint32_t i = 0; i < elements; i++)
    {
        if (ptr_exp[i] != ptr_act[i])
        {
            Unity.CurrentTestFailed = 1;
            UnityPrint(Unity.CurrentTestName);
            UnityPrint(":");
            UnityPrintNumber((int)line);
            UnityPrint(":FAIL: Memory Mismatch at byte ");
            UnityPrintNumber((int)i);

            if (msg != NULL)
            {
                UnityPrint(" ");
                UnityPrint(msg);
            }

            UnityPrint("\n");
            longjmp(Unity.AbortFrame, 1);
        }
    }
}

/* Helper: Print a string */
static void UnityPrint(const char* string)
{
    if (string != NULL)
    {
        printf("%s", string);
    }
}

/* Helper: Print a number */
static void UnityPrintNumber(const int number)
{
    printf("%d", number);
}

/* Helper: Print hex number */
static void UnityPrintHex(const unsigned int number, const char width)
{
    switch (width)
    {
        case 2:  /* HEX8 */
            printf("0x%02X", (unsigned int)(number & 0xFF));
            break;
        case 4:  /* HEX16 */
            printf("0x%04X", (unsigned int)(number & 0xFFFF));
            break;
        case 8:  /* HEX32 */
            printf("0x%08X", number);
            break;
        default:
            printf("0x%X", number);
            break;
    }
}
