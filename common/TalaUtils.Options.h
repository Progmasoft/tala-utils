/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

/*
 * The command line parser of the utilities. A utility lists its options in a table, and the parser
 * fills the typed fields the table points at; nothing after it reads the text of argv again.
 *
 * The syntax is POSIX.1-2024's (XBD 12.2, Utility Syntax Guidelines) with the one-dash long
 * names of TalaOS:
 *
 * - An option starts with one dash. A long name has one dash too (`-login`); there are no `--name`
 *   options.
 * - A word is first matched whole against the long names; when none matches, it is read as short
 *   options grouped behind one dash: `-la` is `-l -a`.
 * - A short option's value is the rest of its word or the next word: `-ofile` or `-o file`. A long
 *   option's value is always the next word: `-arch arm64`.
 * - `--` ends the options. `-` alone is an operand.
 * - The first operand ends the options, unless the table allows options between operands (as ld64
 *   and as do).
 *
 * Messages and the usage line are written as OpenBSD's utilities write them.
 */

/** What an option does with what it is given. */
typedef enum TalaUtils_Options_Kind
{
        /** Sets a bool to true. */
        TalaUtils_Options_Kind_Flag,
        /** Adds one to a count each time it is given, as `-v -v`. */
        TalaUtils_Options_Kind_Count,
        /** Takes a value; the last one given wins. */
        TalaUtils_Options_Kind_Text,
        /** Takes a value each time; all of them are kept in order. */
        TalaUtils_Options_Kind_List,
        /** Takes a decimal integer within a range; the last one given wins. */
        TalaUtils_Options_Kind_Number,
} TalaUtils_Options_tKind;

/** Words kept in order: the values of a list option, or the operands. */
typedef struct TalaUtils_Options_List
{
        /** Room for capacity words; they point into argv. */
        const char** ppItems;
        /** How many words ppItems has room for; argc is always enough. */
        size_t capacity;
        /** How many words it holds. */
        size_t count;
} TalaUtils_Options_tList;

/** One option of a utility. Exactly one of the target fields is set, the one its kind writes. */
typedef struct TalaUtils_Options_Option
{
        /** The short name, as 'l' for -l, or '\0' when it has none. */
        char shortName;
        /** The long name without its dash, as "login" for -login, or NULL when it has none. */
        const char* pLongName;
        /** What the option does. */
        TalaUtils_Options_tKind kind;
        /** The name of its value in the usage line, as "file"; NULL for a flag or a count. */
        const char* pValueName;
        /** Kind_Flag: set to true. */
        bool* pFlag;
        /** Kind_Count: one is added each time. */
        unsigned* pCount;
        /** Kind_Text: the value. */
        const char** ppText;
        /** Kind_List: the values. */
        TalaUtils_Options_tList* pList;
        /** Kind_Number: the value. */
        long long* pNumber;
        /** Kind_Number: the smallest value allowed. */
        long long minimum;
        /** Kind_Number: the largest value allowed. */
        long long maximum;
} TalaUtils_Options_tOption;

/** The command line of one utility. */
typedef struct TalaUtils_Options_Spec
{
        /** The utility's name in messages, as "ls". */
        const char* pProgram;
        /** Its options. */
        const TalaUtils_Options_tOption* pOptions;
        /** How many options there are. */
        size_t count;
        /** The operands in the usage line, as "[file ...]", or NULL when it takes none. */
        const char* pOperands;
        /** True when options may follow operands, as in `ld64 a.o -o a.out`. */
        bool isInterleaved;
} TalaUtils_Options_tSpec;

/** How parsing ended. */
typedef enum TalaUtils_Options_Status
{
        /** Every word was read. */
        TalaUtils_Options_Status_Ok,
        /** A word names no option. */
        TalaUtils_Options_Status_UnknownOption,
        /** An option that takes a value is the last word. */
        TalaUtils_Options_Status_MissingValue,
        /** A number option's value is not a decimal integer in its range. */
        TalaUtils_Options_Status_BadNumber,
        /** A list, or the operands, has no room left. */
        TalaUtils_Options_Status_TooMany,
} TalaUtils_Options_tStatus;

/** What went wrong, for the message. */
typedef struct TalaUtils_Options_Result
{
        TalaUtils_Options_tStatus status;
        /** The option concerned, when the status names one. */
        const TalaUtils_Options_tOption* pOption;
        /** Status_UnknownOption: the unknown short name, or '\0' when the whole word is unknown. */
        char shortName;
        /** The word concerned: the unknown word, or the value that is not a number. */
        const char* pWord;
        /** BadNumber: "invalid", "too small" or "too large". */
        const char* pReason;
} TalaUtils_Options_tResult;

/**
 * @brief Checks a table: no name given twice, each option's target matching its kind, a value name
 *        for each option that takes a value, and no long name that also reads as short options.
 *
 * The tests of each utility call it on its table; Parse does not.
 *
 * @param[in] aIn_pSpec   The table.
 * @param[in] aIn_pErrors Where each problem is written, one line each.
 * @return true when the table has no problem.
 */
bool
TalaUtils_Options_Check(const TalaUtils_Options_tSpec* aIn_pSpec, FILE* aIn_pErrors);

/**
 * @brief Reads argv into the fields of the table and the operands into aOut_pOperands.
 *
 * argv[ 0 ] is the program and is skipped. The fields of options that are not given are left as
 * they are, so a utility sets its defaults first.
 *
 * @param[in]  aIn_pSpec      The table.
 * @param[in]  aArgumentCount argc.
 * @param[in]  aIn_ppWords    argv.
 * @param[out] aOut_pOperands The operands, in order; its count is set from 0.
 * @return Status_Ok, or what went wrong.
 */
TalaUtils_Options_tResult
TalaUtils_Options_Parse(const TalaUtils_Options_tSpec* aIn_pSpec,
                        int                            aArgumentCount,
                        char* const*                   aIn_ppWords,
                        TalaUtils_Options_tList*       aOut_pOperands);

/**
 * @brief Writes the message for a result that is not Status_Ok, as "ls: unknown option -- z".
 *
 * @param[in] aIn_pSpec   The table.
 * @param[in] aIn_pResult What Parse returned.
 * @param[in] aIn_pOutput Where to write, standard error for a utility.
 */
void
TalaUtils_Options_WriteError(const TalaUtils_Options_tSpec*   aIn_pSpec,
                             const TalaUtils_Options_tResult* aIn_pResult,
                             FILE*                            aIn_pOutput);

/**
 * @brief Writes the usage line, as "usage: ls [-al] [-w width] [file ...]", wrapped at 80 columns.
 *
 * Flags and counts with a short name only come first in one bracket, ordered as OpenBSD orders them
 * (digits, then letters, the capital before the small one); then short options that take a value;
 * then the long options in the order of the table.
 *
 * @param[in] aIn_pSpec   The table.
 * @param[in] aIn_pOutput Where to write.
 */
void
TalaUtils_Options_WriteUsage(const TalaUtils_Options_tSpec* aIn_pSpec, FILE* aIn_pOutput);

/**
 * @brief Parse; on a problem, the message and the usage line on standard error and exit status 1,
 *        as OpenBSD's utilities do.
 *
 * @param[in]  aIn_pSpec      The table.
 * @param[in]  aArgumentCount argc.
 * @param[in]  aIn_ppWords    argv.
 * @param[out] aOut_pOperands The operands.
 */
void
TalaUtils_Options_ParseOrExit(const TalaUtils_Options_tSpec* aIn_pSpec,
                              int                            aArgumentCount,
                              char* const*                   aIn_ppWords,
                              TalaUtils_Options_tList*       aOut_pOperands);
