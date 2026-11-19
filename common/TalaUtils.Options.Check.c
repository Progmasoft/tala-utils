/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#include <ctype.h>
#include <string.h>

#include "TalaUtils.Options.h"

/*
 * The check of a table, which each utility's tests run, so that a mistake in a table is found
 * before the utility is shipped rather than by a user.
 */

/** Writes the option's names for a message, as "-l" or "-login". */
static void
WriteNames(const TalaUtils_Options_tOption* aIn_pOption, FILE* aIn_pErrors)
{
        if(aIn_pOption->shortName == '\0' && aIn_pOption->pLongName == NULL)
        {
                fputs("(no name)", aIn_pErrors);
        }
        if(aIn_pOption->shortName != '\0')
        {
                fprintf(aIn_pErrors, "-%c", aIn_pOption->shortName);
        }
        if(aIn_pOption->pLongName != NULL)
        {
                fprintf(aIn_pErrors,
                        "%s-%s",
                        aIn_pOption->shortName != '\0' ? "/" : "",
                        aIn_pOption->pLongName);
        }
}

static bool
Problem(const TalaUtils_Options_tSpec*   aIn_pSpec,
        const TalaUtils_Options_tOption* aIn_pOption,
        const char*                      aIn_pMessage,
        FILE*                            aIn_pErrors)
{
        fprintf(aIn_pErrors, "%s: option ", aIn_pSpec->pProgram);
        WriteNames(aIn_pOption, aIn_pErrors);
        fprintf(aIn_pErrors, ": %s\n", aIn_pMessage);
        return (false);
}

/** The one target field the option's kind writes is set, and no other. */
static bool
HasItsTarget(const TalaUtils_Options_tOption* aIn_pOption)
{
        const void* pTargets[ 5 ] = { aIn_pOption->pFlag,
                                      aIn_pOption->pCount,
                                      aIn_pOption->ppText,
                                      aIn_pOption->pList,
                                      aIn_pOption->pNumber };
        size_t      set           = 0;
        for(size_t i = 0; i < 5; i++)
        {
                set += (pTargets[ i ] != NULL) ? 1 : 0;
        }
        return (set == 1 && pTargets[ aIn_pOption->kind ] != NULL);
}

static bool
TakesValue(TalaUtils_Options_tKind aKind)
{
        return (aKind == TalaUtils_Options_Kind_Text || aKind == TalaUtils_Options_Kind_List
                || aKind == TalaUtils_Options_Kind_Number);
}

/** The names: one at least, a short one a letter or digit, a long one two characters at least. */
static const char*
NamesProblem(const TalaUtils_Options_tOption* aIn_pOption)
{
        if(aIn_pOption->shortName == '\0' && aIn_pOption->pLongName == NULL)
        {
                return ("it has no name");
        }
        if(aIn_pOption->shortName != '\0' && !isalnum((unsigned char) aIn_pOption->shortName))
        {
                return ("a short name is a letter or a digit");
        }
        if(aIn_pOption->pLongName != NULL
           && (strlen(aIn_pOption->pLongName) < 2 || aIn_pOption->pLongName[ 0 ] == '-'))
        {
                return ("a long name has two characters at least and no leading dash");
        }
        return (NULL);
}

/** What is wrong with one option alone, or NULL. */
static const char*
OptionProblem(const TalaUtils_Options_tOption* aIn_pOption)
{
        const char* pNames = NamesProblem(aIn_pOption);
        if(pNames != NULL)
        {
                return (pNames);
        }
        if((unsigned) aIn_pOption->kind > (unsigned) TalaUtils_Options_Kind_Number
           || !HasItsTarget(aIn_pOption))
        {
                return ("its target is not the one field its kind writes");
        }
        if(TakesValue(aIn_pOption->kind) != (aIn_pOption->pValueName != NULL))
        {
                return ("an option has a value name exactly when it takes a value");
        }
        if(aIn_pOption->kind == TalaUtils_Options_Kind_Number
           && aIn_pOption->minimum > aIn_pOption->maximum)
        {
                return ("its minimum is over its maximum");
        }
        return (NULL);
}

/** True when another option before aIndex has the same short or long name. */
static bool
IsRepeated(const TalaUtils_Options_tSpec* aIn_pSpec, size_t aIndex)
{
        const TalaUtils_Options_tOption* pOption = &aIn_pSpec->pOptions[ aIndex ];
        for(size_t i = 0; i < aIndex; i++)
        {
                const TalaUtils_Options_tOption* pOther = &aIn_pSpec->pOptions[ i ];
                if(pOption->shortName != '\0' && pOther->shortName == pOption->shortName)
                {
                        return (true);
                }
                if(pOption->pLongName != NULL && pOther->pLongName != NULL
                   && strcmp(pOther->pLongName, pOption->pLongName) == 0)
                {
                        return (true);
                }
        }
        return (false);
}

/** True when every letter of the long name is a short flag or count: -al would mean two things. */
static bool
ReadsAsFlags(const TalaUtils_Options_tSpec* aIn_pSpec, const char* aIn_pName)
{
        for(size_t i = 0; aIn_pName[ i ] != '\0'; i++)
        {
                bool isFlag = false;
                for(size_t j = 0; j < aIn_pSpec->count; j++)
                {
                        const TalaUtils_Options_tOption* pOther = &aIn_pSpec->pOptions[ j ];
                        isFlag                                  = isFlag
                                || (pOther->shortName == aIn_pName[ i ]
                                    && !TakesValue(pOther->kind));
                }
                if(!isFlag)
                {
                        return (false);
                }
        }
        return (true);
}

bool
TalaUtils_Options_Check(const TalaUtils_Options_tSpec* aIn_pSpec, FILE* aIn_pErrors)
{
        bool isGood = true;
        for(size_t i = 0; i < aIn_pSpec->count; i++)
        {
                const TalaUtils_Options_tOption* pOption  = &aIn_pSpec->pOptions[ i ];
                const char*                      pProblem = OptionProblem(pOption);
                if(pProblem == NULL && IsRepeated(aIn_pSpec, i))
                {
                        pProblem = "a name is given to two options";
                }
                if(pProblem == NULL && pOption->pLongName != NULL
                   && ReadsAsFlags(aIn_pSpec, pOption->pLongName))
                {
                        pProblem = "the long name also reads as short flags";
                }
                if(pProblem != NULL)
                {
                        isGood = Problem(aIn_pSpec, pOption, pProblem, aIn_pErrors);
                }
        }
        return (isGood);
}
