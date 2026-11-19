/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdio.h>

/*
 * A small test harness: each test file gives its cases, Test.Main.c runs them and counts the
 * failed checks.
 */

/** A test case. */
typedef void (*Test_tFn_Case)(void);

/** A named test case. */
typedef struct Test_Case
{
        const char*   pName;
        Test_tFn_Case function;
} Test_tCase;

/** The test cases of one file. */
typedef struct Test_Suite
{
        const Test_tCase* pCases;
        size_t            count;
} Test_tSuite;

/** Defines the suite of a test file from an array of cases. */
#define TEST_SUITE(dA_Name, dA_Cases) \
        const Test_tSuite dA_Name = { (dA_Cases), sizeof(dA_Cases) / sizeof((dA_Cases)[ 0 ]) }

/**
 * @brief Records a failed check.
 *
 * @param[in] aIn_pFile    The file of the check.
 * @param[in] aLine        The line of the check.
 * @param[in] aIn_pMessage What failed.
 */
void
Test_Fail(const char* aIn_pFile, int aLine, const char* aIn_pMessage);

/**
 * @brief Checks that two strings are equal, and reports both when they are not.
 *
 * @param[in] aIn_pFile     The file of the check.
 * @param[in] aLine         The line of the check.
 * @param[in] aIn_pActual   The string the code gave, or NULL.
 * @param[in] aIn_pExpected The string it should give, or NULL.
 */
void
Test_CheckString(const char* aIn_pFile,
                 int         aLine,
                 const char* aIn_pActual,
                 const char* aIn_pExpected);

/**
 * @brief Checks that two integers are equal, and reports both when they are not.
 *
 * @param[in] aIn_pFile  The file of the check.
 * @param[in] aLine      The line of the check.
 * @param[in] aActual    The value the code gave.
 * @param[in] aExpected  The value it should give.
 */
void
Test_CheckInteger(const char* aIn_pFile, int aLine, long long aActual, long long aExpected);

/**
 * @brief Opens a temporary stream for code that writes, to read back what it wrote.
 *
 * @return The stream; Test_Captured reads it back and closes it.
 */
FILE*
Test_Capture(void);

/**
 * @brief Reads back what was written to a stream from Test_Capture, and closes it.
 *
 * @param[in]  aIn_pStream The stream.
 * @param[out] aOut_pText  The text, ended by NUL.
 * @param[in]  aCapacity   The size of aOut_pText.
 */
void
Test_Captured(FILE* aIn_pStream, char* aOut_pText, size_t aCapacity);

#define CHECK(dA_Condition)                                           \
        do                                                            \
        {                                                             \
                if(!(dA_Condition))                                   \
                {                                                     \
                        Test_Fail(__FILE__, __LINE__, #dA_Condition); \
                }                                                     \
        } while(0)

#define CHECK_STRING(dA_Actual, dA_Expected) \
        Test_CheckString(__FILE__, __LINE__, (dA_Actual), (dA_Expected))

#define CHECK_INTEGER(dA_Actual, dA_Expected) \
        Test_CheckInteger(__FILE__, __LINE__, (long long) (dA_Actual), (long long) (dA_Expected))
