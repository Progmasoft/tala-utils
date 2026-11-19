/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#include <string.h>

#include "Test.h"

extern const Test_tSuite OptionsTests;
extern const Test_tSuite OptionsCheckTests;

/** How many checks failed. */
static int Test_failures;

void
Test_Fail(const char* aIn_pFile, int aLine, const char* aIn_pMessage)
{
        Test_failures++;
        fprintf(stderr, "%s:%d: failed: %s\n", aIn_pFile, aLine, aIn_pMessage);
}

void
Test_CheckString(const char* aIn_pFile,
                 int         aLine,
                 const char* aIn_pActual,
                 const char* aIn_pExpected)
{
        if(aIn_pActual == aIn_pExpected
           || (aIn_pActual != NULL && aIn_pExpected != NULL
               && strcmp(aIn_pActual, aIn_pExpected) == 0))
        {
                return;
        }
        Test_failures++;
        fprintf(stderr,
                "%s:%d: failed:\n  got:      \"%s\"\n  expected: \"%s\"\n",
                aIn_pFile,
                aLine,
                aIn_pActual != NULL ? aIn_pActual : "(null)",
                aIn_pExpected != NULL ? aIn_pExpected : "(null)");
}

void
Test_CheckInteger(const char* aIn_pFile, int aLine, long long aActual, long long aExpected)
{
        if(aActual == aExpected)
        {
                return;
        }
        Test_failures++;
        fprintf(stderr,
                "%s:%d: failed: got %lld, expected %lld\n",
                aIn_pFile,
                aLine,
                aActual,
                aExpected);
}

FILE*
Test_Capture(void)
{
        FILE* pStream = tmpfile();
        if(pStream == NULL)
        {
                perror("tmpfile");
        }
        return (pStream);
}

void
Test_Captured(FILE* aIn_pStream, char* aOut_pText, size_t aCapacity)
{
        aOut_pText[ 0 ] = '\0';
        if(aIn_pStream == NULL)
        {
                return;
        }
        rewind(aIn_pStream);
        size_t length        = fread(aOut_pText, 1, aCapacity - 1, aIn_pStream);
        aOut_pText[ length ] = '\0';
        fclose(aIn_pStream);
}

int
main(void)
{
        const Test_tSuite* pSuites[] = { &OptionsTests, &OptionsCheckTests };
        size_t             cases     = 0;
        for(size_t i = 0; i < sizeof(pSuites) / sizeof(pSuites[ 0 ]); i++)
        {
                for(size_t j = 0; j < pSuites[ i ]->count; j++)
                {
                        int before = Test_failures;
                        pSuites[ i ]->pCases[ j ].function();
                        printf("%s %s\n",
                               Test_failures == before ? "ok  " : "FAIL",
                               pSuites[ i ]->pCases[ j ].pName);
                        cases++;
                }
        }
        printf("%zu cases, %d failed checks\n", cases, Test_failures);
        return (Test_failures == 0 ? 0 : 1);
}
