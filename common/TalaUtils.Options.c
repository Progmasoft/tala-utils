/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "TalaUtils.Options.h"

/** The width the usage line is wrapped at, as OpenBSD's manual pages and usage lines keep. */
#define USAGE_WIDTH 80

/** Writes the usage line word by word and wraps it. */
typedef struct TalaUtils_Options_Writer
{
        FILE*  pOutput;
        size_t column;
        /** Where a continued line starts: under the first word after the program. */
        size_t indent;
} TalaUtils_Options_tWriter;

static bool
TakesValue(TalaUtils_Options_tKind aKind)
{
        return (aKind == TalaUtils_Options_Kind_Text || aKind == TalaUtils_Options_Kind_List
                || aKind == TalaUtils_Options_Kind_Number);
}

static const TalaUtils_Options_tOption*
FindLong(const TalaUtils_Options_tSpec* aIn_pSpec, const char* aIn_pName)
{
        for(size_t i = 0; i < aIn_pSpec->count; i++)
        {
                const TalaUtils_Options_tOption* pOption = &aIn_pSpec->pOptions[ i ];
                if(pOption->pLongName != NULL && strcmp(pOption->pLongName, aIn_pName) == 0)
                {
                        return (pOption);
                }
        }
        return (NULL);
}

static const TalaUtils_Options_tOption*
FindShort(const TalaUtils_Options_tSpec* aIn_pSpec, char aName)
{
        for(size_t i = 0; i < aIn_pSpec->count; i++)
        {
                if(aName != '\0' && aIn_pSpec->pOptions[ i ].shortName == aName)
                {
                        return (&aIn_pSpec->pOptions[ i ]);
                }
        }
        return (NULL);
}

static TalaUtils_Options_tResult
Result(TalaUtils_Options_tStatus aStatus, const TalaUtils_Options_tOption* aIn_pOption)
{
        TalaUtils_Options_tResult result = { aStatus, aIn_pOption, '\0', NULL, NULL };
        return (result);
}

static bool
Push(TalaUtils_Options_tList* aInOut_pList, const char* aIn_pWord)
{
        if(aInOut_pList->count >= aInOut_pList->capacity)
        {
                return (false);
        }
        aInOut_pList->ppItems[ aInOut_pList->count++ ] = aIn_pWord;
        return (true);
}

/**
 * @brief Reads a decimal integer within [aMinimum, aMaximum], as OpenBSD's strtonum does.
 *
 * @return NULL, or why the text is not one: "invalid", "too small", "too large".
 */
static const char*
ReadNumber(const char* aIn_pText, long long aMinimum, long long aMaximum, long long* aOut_pValue)
{
        char* pEnd      = NULL;
        errno           = 0;
        long long value = strtoll(aIn_pText, &pEnd, 10);
        if(pEnd == aIn_pText || *pEnd != '\0' || isspace((unsigned char) aIn_pText[ 0 ]))
        {
                return ("invalid");
        }
        if((errno == ERANGE && value == LLONG_MIN) || value < aMinimum)
        {
                return ("too small");
        }
        if((errno == ERANGE && value == LLONG_MAX) || value > aMaximum)
        {
                return ("too large");
        }
        *aOut_pValue = value;
        return (NULL);
}

/** Gives aIn_pOption what it was given: nothing for a flag or a count, aIn_pValue otherwise. */
static TalaUtils_Options_tResult
Apply(const TalaUtils_Options_tOption* aIn_pOption, const char* aIn_pValue)
{
        TalaUtils_Options_tResult result = Result(TalaUtils_Options_Status_Ok, aIn_pOption);
        switch(aIn_pOption->kind)
        {
        case TalaUtils_Options_Kind_Flag:
                *aIn_pOption->pFlag = true;
                break;
        case TalaUtils_Options_Kind_Count:
                if(*aIn_pOption->pCount < (unsigned) -1)
                {
                        (*aIn_pOption->pCount)++;
                }
                break;
        case TalaUtils_Options_Kind_Text:
                *aIn_pOption->ppText = aIn_pValue;
                break;
        case TalaUtils_Options_Kind_List:
                if(!Push(aIn_pOption->pList, aIn_pValue))
                {
                        result.status = TalaUtils_Options_Status_TooMany;
                }
                break;
        case TalaUtils_Options_Kind_Number:
                result.pReason = ReadNumber(aIn_pValue,
                                            aIn_pOption->minimum,
                                            aIn_pOption->maximum,
                                            aIn_pOption->pNumber);
                if(result.pReason != NULL)
                {
                        result.status = TalaUtils_Options_Status_BadNumber;
                        result.pWord  = aIn_pValue;
                }
                break;
        }
        return (result);
}

/** The value of an option whose value is the next word, moving *aInOut_pIndex past it. */
static const char*
NextWord(int aArgumentCount, char* const* aIn_ppWords, int* aInOut_pIndex)
{
        if(*aInOut_pIndex + 1 >= aArgumentCount)
        {
                return (NULL);
        }
        (*aInOut_pIndex)++;
        return (aIn_ppWords[ *aInOut_pIndex ]);
}

/** Reads the short options grouped in one word, as -la or -ofile. */
static TalaUtils_Options_tResult
ReadShortOptions(const TalaUtils_Options_tSpec* aIn_pSpec,
                 int                            aArgumentCount,
                 char* const*                   aIn_ppWords,
                 int*                           aInOut_pIndex)
{
        const char* pNames = aIn_ppWords[ *aInOut_pIndex ] + 1;
        for(size_t i = 0; pNames[ i ] != '\0'; i++)
        {
                const TalaUtils_Options_tOption* pOption = FindShort(aIn_pSpec, pNames[ i ]);
                if(pOption == NULL)
                {
                        TalaUtils_Options_tResult result = Result(
                                TalaUtils_Options_Status_UnknownOption,
                                NULL);
                        // A word that starts with an unknown letter was meant as a long name.
                        result.shortName = (i == 0 && pNames[ 1 ] != '\0') ? '\0' : pNames[ i ];
                        result.pWord     = aIn_ppWords[ *aInOut_pIndex ];
                        return (result);
                }
                if(!TakesValue(pOption->kind))
                {
                        Apply(pOption, NULL);
                        continue;
                }
                const char* pValue = (pNames[ i + 1 ] != '\0')
                        ? &pNames[ i + 1 ]
                        : NextWord(aArgumentCount, aIn_ppWords, aInOut_pIndex);
                if(pValue == NULL)
                {
                        return (Result(TalaUtils_Options_Status_MissingValue, pOption));
                }
                return (Apply(pOption, pValue));
        }
        return (Result(TalaUtils_Options_Status_Ok, NULL));
}

/** Reads the option word at *aInOut_pIndex and its value; the index stays on the last word. */
static TalaUtils_Options_tResult
ReadOption(const TalaUtils_Options_tSpec* aIn_pSpec,
           int                            aArgumentCount,
           char* const*                   aIn_ppWords,
           int*                           aInOut_pIndex)
{
        const TalaUtils_Options_tOption* pOption = FindLong(aIn_pSpec,
                                                            aIn_ppWords[ *aInOut_pIndex ] + 1);
        if(pOption == NULL)
        {
                return (ReadShortOptions(aIn_pSpec, aArgumentCount, aIn_ppWords, aInOut_pIndex));
        }
        if(!TakesValue(pOption->kind))
        {
                return (Apply(pOption, NULL));
        }
        const char* pValue = NextWord(aArgumentCount, aIn_ppWords, aInOut_pIndex);
        if(pValue == NULL)
        {
                return (Result(TalaUtils_Options_Status_MissingValue, pOption));
        }
        return (Apply(pOption, pValue));
}

TalaUtils_Options_tResult
TalaUtils_Options_Parse(const TalaUtils_Options_tSpec* aIn_pSpec,
                        int                            aArgumentCount,
                        char* const*                   aIn_ppWords,
                        TalaUtils_Options_tList*       aOut_pOperands)
{
        bool isOptions        = true;
        aOut_pOperands->count = 0;
        for(int i = 1; i < aArgumentCount; i++)
        {
                const char* pWord = aIn_ppWords[ i ];
                if(isOptions && strcmp(pWord, "--") == 0)
                {
                        isOptions = false;
                        continue;
                }
                if(!isOptions || pWord[ 0 ] != '-' || pWord[ 1 ] == '\0')
                {
                        isOptions = isOptions && aIn_pSpec->isInterleaved;
                        if(!Push(aOut_pOperands, pWord))
                        {
                                return (Result(TalaUtils_Options_Status_TooMany, NULL));
                        }
                        continue;
                }
                TalaUtils_Options_tResult result
                        = ReadOption(aIn_pSpec, aArgumentCount, aIn_ppWords, &i);
                if(result.status != TalaUtils_Options_Status_Ok)
                {
                        return (result);
                }
        }
        return (Result(TalaUtils_Options_Status_Ok, NULL));
}

/** The name an option is called by in messages: its short name, or its long name. */
static void
WriteName(const TalaUtils_Options_tOption* aIn_pOption, FILE* aIn_pOutput)
{
        if(aIn_pOption->shortName != '\0')
        {
                fputc(aIn_pOption->shortName, aIn_pOutput);
        } else
        {
                fputs(aIn_pOption->pLongName, aIn_pOutput);
        }
}

void
TalaUtils_Options_WriteError(const TalaUtils_Options_tSpec*   aIn_pSpec,
                             const TalaUtils_Options_tResult* aIn_pResult,
                             FILE*                            aIn_pOutput)
{
        fprintf(aIn_pOutput, "%s: ", aIn_pSpec->pProgram);
        switch(aIn_pResult->status)
        {
        case TalaUtils_Options_Status_Ok:
                fputs("no error", aIn_pOutput);
                break;
        case TalaUtils_Options_Status_UnknownOption:
                if(aIn_pResult->shortName != '\0')
                {
                        fprintf(aIn_pOutput, "unknown option -- %c", aIn_pResult->shortName);
                } else
                {
                        fprintf(aIn_pOutput, "unknown option %s", aIn_pResult->pWord);
                }
                break;
        case TalaUtils_Options_Status_MissingValue:
                fputs("option requires an argument -- ", aIn_pOutput);
                WriteName(aIn_pResult->pOption, aIn_pOutput);
                break;
        case TalaUtils_Options_Status_BadNumber:
                fputs("value of -", aIn_pOutput);
                WriteName(aIn_pResult->pOption, aIn_pOutput);
                fprintf(aIn_pOutput, " is %s: %s", aIn_pResult->pReason, aIn_pResult->pWord);
                break;
        case TalaUtils_Options_Status_TooMany:
                if(aIn_pResult->pOption == NULL)
                {
                        fputs("too many operands", aIn_pOutput);
                        break;
                }
                fputs("too many values for -", aIn_pOutput);
                WriteName(aIn_pResult->pOption, aIn_pOutput);
                break;
        }
        fputc('\n', aIn_pOutput);
}

/** Writes one word of the usage line, starting a new line under the first word when it is full. */
static void
Put(TalaUtils_Options_tWriter* aInOut_pWriter, const char* aIn_pWord)
{
        size_t length = strlen(aIn_pWord);
        if(aInOut_pWriter->column > aInOut_pWriter->indent
           && aInOut_pWriter->column + 1 + length > USAGE_WIDTH)
        {
                fprintf(aInOut_pWriter->pOutput, "\n%*s", (int) aInOut_pWriter->indent, "");
                aInOut_pWriter->column = aInOut_pWriter->indent;
        } else
        {
                fputc(' ', aInOut_pWriter->pOutput);
                aInOut_pWriter->column++;
        }
        fputs(aIn_pWord, aInOut_pWriter->pOutput);
        aInOut_pWriter->column += length;
}

/** The place of a short name in OpenBSD's order: digits, then letters, the capital first. */
static int
Rank(char aName)
{
        unsigned char name = (unsigned char) aName;
        if(isdigit(name))
        {
                return (name - '0');
        }
        return (10 + 2 * (tolower(name) - 'a') + (islower(name) ? 1 : 0));
}

static bool
IsShortOnly(const TalaUtils_Options_tOption* aIn_pOption)
{
        return (aIn_pOption->shortName != '\0' && aIn_pOption->pLongName == NULL);
}

/** The short-only options whose value-taking matches aTakesValue, in OpenBSD's order. */
static size_t
SortedShort(const TalaUtils_Options_tSpec*    aIn_pSpec,
            bool                              aTakesValue,
            const TalaUtils_Options_tOption** aOut_ppOptions,
            size_t                            aCapacity)
{
        size_t count = 0;
        for(size_t i = 0; i < aIn_pSpec->count && count < aCapacity; i++)
        {
                const TalaUtils_Options_tOption* pOption = &aIn_pSpec->pOptions[ i ];
                if(!IsShortOnly(pOption) || TakesValue(pOption->kind) != aTakesValue)
                {
                        continue;
                }
                size_t place = count++;
                while(place > 0
                      && Rank(aOut_ppOptions[ place - 1 ]->shortName) > Rank(pOption->shortName))
                {
                        aOut_ppOptions[ place ] = aOut_ppOptions[ place - 1 ];
                        place--;
                }
                aOut_ppOptions[ place ] = pOption;
        }
        return (count);
}

/** Writes "[-name]" or "[-name value]" for an option. */
static void
PutOption(TalaUtils_Options_tWriter* aInOut_pWriter, const TalaUtils_Options_tOption* aIn_pOption)
{
        char        word[ 160 ];
        const char* pName          = aIn_pOption->pLongName;
        char        shortName[ 2 ] = { aIn_pOption->shortName, '\0' };
        if(aIn_pOption->shortName != '\0')
        {
                pName = shortName;
        }
        if(TakesValue(aIn_pOption->kind))
        {
                snprintf(word, sizeof(word), "[-%s %s]", pName, aIn_pOption->pValueName);
        } else
        {
                snprintf(word, sizeof(word), "[-%s]", pName);
        }
        Put(aInOut_pWriter, word);
}

/** Writes "[-1Aal]", the flags and counts that have only a short name. */
static void
PutFlags(TalaUtils_Options_tWriter* aInOut_pWriter, const TalaUtils_Options_tSpec* aIn_pSpec)
{
        const TalaUtils_Options_tOption* pFlags[ 128 ];
        size_t count = SortedShort(aIn_pSpec, false, pFlags, sizeof(pFlags) / sizeof(pFlags[ 0 ]));
        if(count == 0)
        {
                return;
        }
        char word[ 132 ] = "[-";
        for(size_t i = 0; i < count; i++)
        {
                word[ 2 + i ] = pFlags[ i ]->shortName;
        }
        word[ 2 + count ] = ']';
        word[ 3 + count ] = '\0';
        Put(aInOut_pWriter, word);
}

void
TalaUtils_Options_WriteUsage(const TalaUtils_Options_tSpec* aIn_pSpec, FILE* aIn_pOutput)
{
        fprintf(aIn_pOutput, "usage: %s", aIn_pSpec->pProgram);
        size_t                    start  = strlen("usage: ") + strlen(aIn_pSpec->pProgram);
        TalaUtils_Options_tWriter writer = { aIn_pOutput, start, start + 1 };
        PutFlags(&writer, aIn_pSpec);
        const TalaUtils_Options_tOption* pValued[ 128 ];
        size_t                           count
                = SortedShort(aIn_pSpec, true, pValued, sizeof(pValued) / sizeof(pValued[ 0 ]));
        for(size_t i = 0; i < count; i++)
        {
                PutOption(&writer, pValued[ i ]);
        }
        for(size_t i = 0; i < aIn_pSpec->count; i++)
        {
                if(!IsShortOnly(&aIn_pSpec->pOptions[ i ]))
                {
                        PutOption(&writer, &aIn_pSpec->pOptions[ i ]);
                }
        }
        if(aIn_pSpec->pOperands != NULL)
        {
                Put(&writer, aIn_pSpec->pOperands);
        }
        fputc('\n', aIn_pOutput);
}

void
TalaUtils_Options_ParseOrExit(const TalaUtils_Options_tSpec* aIn_pSpec,
                              int                            aArgumentCount,
                              char* const*                   aIn_ppWords,
                              TalaUtils_Options_tList*       aOut_pOperands)
{
        TalaUtils_Options_tResult result
                = TalaUtils_Options_Parse(aIn_pSpec, aArgumentCount, aIn_ppWords, aOut_pOperands);
        if(result.status == TalaUtils_Options_Status_Ok)
        {
                return;
        }
        TalaUtils_Options_WriteError(aIn_pSpec, &result, stderr);
        TalaUtils_Options_WriteUsage(aIn_pSpec, stderr);
        exit(1);
}
