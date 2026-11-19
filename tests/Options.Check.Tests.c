/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#include "TalaUtils.Options.h"
#include "Test.h"

/* The check of a table finds each kind of mistake, and passes a good table. */

static bool        Flag;
static bool        Other;
static const char* Text;

/** What Check writes about a table of the given options. */
static bool
Checked(const TalaUtils_Options_tOption* aIn_pOptions, size_t aCount, char* aOut_pText)
{
        TalaUtils_Options_tSpec spec    = { "tool", aIn_pOptions, aCount, NULL, false };
        FILE*                   pStream = Test_Capture();
        bool                    isGood  = TalaUtils_Options_Check(&spec, pStream);
        Test_Captured(pStream, aOut_pText, 256);
        return (isGood);
}

static void
AGoodTablePasses(void)
{
        char                            text[ 256 ];
        const TalaUtils_Options_tOption options[] = {
                { .shortName = 'a', .kind = TalaUtils_Options_Kind_Flag, .pFlag = &Flag },
                { .shortName  = 'o',
                  .pLongName  = "output",
                  .kind       = TalaUtils_Options_Kind_Text,
                  .pValueName = "file",
                  .ppText     = &Text },
        };
        CHECK(Checked(options, 2, text));
        CHECK_STRING(text, "");
}

static void
ANameGivenTwice(void)
{
        char                            text[ 256 ];
        const TalaUtils_Options_tOption options[] = {
                { .shortName = 'a', .kind = TalaUtils_Options_Kind_Flag, .pFlag = &Flag },
                { .shortName = 'a', .kind = TalaUtils_Options_Kind_Flag, .pFlag = &Other },
        };
        CHECK(!Checked(options, 2, text));
        CHECK_STRING(text, "tool: option -a: a name is given to two options\n");
}

static void
ATargetThatIsNotItsKinds(void)
{
        char                            text[ 256 ];
        const TalaUtils_Options_tOption options[] = {
                { .shortName  = 'a',
                  .kind       = TalaUtils_Options_Kind_Text,
                  .pValueName = "x",
                  .pFlag      = &Flag },
        };
        CHECK(!Checked(options, 1, text));
        CHECK_STRING(text, "tool: option -a: its target is not the one field its kind writes\n");
}

static void
AValueNameOnAFlag(void)
{
        char                            text[ 256 ];
        const TalaUtils_Options_tOption options[] = {
                { .pLongName  = "all",
                  .kind       = TalaUtils_Options_Kind_Flag,
                  .pValueName = "x",
                  .pFlag      = &Flag },
        };
        CHECK(!Checked(options, 1, text));
        CHECK_STRING(
                text,
                "tool: option -all: an option has a value name exactly when it takes a value\n");
}

static void
ALongNameThatReadsAsFlags(void)
{
        char                            text[ 256 ];
        const TalaUtils_Options_tOption options[] = {
                { .shortName = 'a', .kind = TalaUtils_Options_Kind_Flag, .pFlag = &Flag },
                { .shortName = 'l', .kind = TalaUtils_Options_Kind_Flag, .pFlag = &Other },
                { .pLongName  = "al",
                  .kind       = TalaUtils_Options_Kind_Text,
                  .pValueName = "x",
                  .ppText     = &Text },
        };
        CHECK(!Checked(options, 3, text));
        CHECK_STRING(text, "tool: option -al: the long name also reads as short flags\n");
}

static void
BadNames(void)
{
        char                            text[ 256 ];
        const TalaUtils_Options_tOption options[] = {
                { .kind = TalaUtils_Options_Kind_Flag, .pFlag = &Flag },
                { .shortName = '?', .kind = TalaUtils_Options_Kind_Flag, .pFlag = &Other },
                { .pLongName = "x", .kind = TalaUtils_Options_Kind_Flag, .pFlag = &Other },
        };
        CHECK(!Checked(options, 3, text));
        CHECK_STRING(
                text,
                "tool: option (no name): it has no name\n"
                "tool: option -?: a short name is a letter or a digit\n"
                "tool: option -x: a long name has two characters at least and no leading dash\n");
}

static const Test_tCase Cases[] = {
        { "a good table passes", AGoodTablePasses },
        { "a name given twice", ANameGivenTwice },
        { "a target that is not its kind's", ATargetThatIsNotItsKinds },
        { "a value name on a flag", AValueNameOnAFlag },
        { "a long name that reads as flags", ALongNameThatReadsAsFlags },
        { "bad names", BadNames },
};

TEST_SUITE(OptionsCheckTests, Cases);
