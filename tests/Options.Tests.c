/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#include <string.h>

#include "TalaUtils.Options.h"
#include "Test.h"

/*
 * The parser against two tables: OpenBSD's ls, whose usage line it must write exactly as OpenBSD's
 * does, and a linker-like table with values, numbers, lists, long names and interleaved operands.
 */

#define WORDS(...) ((char* const[]) { "program", __VA_ARGS__ })
#define COUNT(...) ((int) (sizeof(WORDS(__VA_ARGS__)) / sizeof(char*)))
#define PARSE(dA_Spec, dA_Operands, ...) \
        TalaUtils_Options_Parse(&(dA_Spec), COUNT(__VA_ARGS__), WORDS(__VA_ARGS__), &(dA_Operands))

/** The flags of OpenBSD's ls, one field each. */
static bool Ls_flags[ 128 ];

static const char Ls_names[] = "1AaCcdFfgHhikLlmnopqRrSsTtux";

static TalaUtils_Options_tOption Ls_options[ sizeof(Ls_names) - 1 ];

static TalaUtils_Options_tSpec
LsSpec(void)
{
        for(size_t i = 0; i < sizeof(Ls_names) - 1; i++)
        {
                TalaUtils_Options_tOption option = { 0 };
                option.shortName                 = Ls_names[ i ];
                option.kind                      = TalaUtils_Options_Kind_Flag;
                option.pFlag                     = &Ls_flags[ (unsigned char) Ls_names[ i ] ];
                Ls_options[ i ]                  = option;
        }
        memset(Ls_flags, 0, sizeof(Ls_flags));
        TalaUtils_Options_tSpec spec
                = { "ls", Ls_options, sizeof(Ls_names) - 1, "[file ...]", false };
        return (spec);
}

/** The fields of the linker-like table. */
typedef struct Link
{
        const char*             pOutput;
        long long               jobs;
        const char*             pLibraryDirectories[ 4 ];
        TalaUtils_Options_tList libraryDirectories;
        unsigned                verbose;
        bool                    isDylib;
        const char*             pArchitecture;
} tLink;

static tLink Link;

static TalaUtils_Options_tOption Link_options[ 6 ];

static TalaUtils_Options_tSpec
LinkSpec(size_t aListCapacity)
{
        memset(&Link, 0, sizeof(Link));
        Link.jobs                           = 1;
        Link.libraryDirectories             = (TalaUtils_Options_tList) { Link.pLibraryDirectories,
                                                                          aListCapacity,
                                                                          0 };
        TalaUtils_Options_tOption options[] = {
                { .shortName  = 'o',
                  .kind       = TalaUtils_Options_Kind_Text,
                  .pValueName = "file",
                  .ppText     = &Link.pOutput },
                { .shortName  = 'j',
                  .kind       = TalaUtils_Options_Kind_Number,
                  .pValueName = "jobs",
                  .pNumber    = &Link.jobs,
                  .minimum    = 1,
                  .maximum    = 64 },
                { .shortName  = 'L',
                  .kind       = TalaUtils_Options_Kind_List,
                  .pValueName = "dir",
                  .pList      = &Link.libraryDirectories },
                { .shortName = 'v', .kind = TalaUtils_Options_Kind_Count, .pCount = &Link.verbose },
                { .pLongName = "dylib",
                  .kind      = TalaUtils_Options_Kind_Flag,
                  .pFlag     = &Link.isDylib },
                { .pLongName  = "arch",
                  .kind       = TalaUtils_Options_Kind_Text,
                  .pValueName = "name",
                  .ppText     = &Link.pArchitecture },
        };
        memcpy(Link_options, options, sizeof(options));
        TalaUtils_Options_tSpec spec = { "ld64", Link_options, 6, "file ...", true };
        return (spec);
}

static const char* Operand_items[ 8 ];

static TalaUtils_Options_tList
Operands(void)
{
        TalaUtils_Options_tList operands = { Operand_items, 8, 0 };
        return (operands);
}

/** What WriteError writes for a result. */
static void
ErrorText(const TalaUtils_Options_tSpec*   aIn_pSpec,
          const TalaUtils_Options_tResult* aIn_pResult,
          char*                            aOut_pText)
{
        FILE* pStream = Test_Capture();
        TalaUtils_Options_WriteError(aIn_pSpec, aIn_pResult, pStream);
        Test_Captured(pStream, aOut_pText, 256);
}

static void
GroupedFlagsAndTheEndOfOptions(void)
{
        TalaUtils_Options_tSpec   spec     = LsSpec();
        TalaUtils_Options_tList   operands = Operands();
        TalaUtils_Options_tResult result   = PARSE(spec, operands, "-la", "-F", "--", "-R", "-");
        CHECK_INTEGER(result.status, TalaUtils_Options_Status_Ok);
        CHECK(Ls_flags[ 'l' ] && Ls_flags[ 'a' ] && Ls_flags[ 'F' ] && !Ls_flags[ 'R' ]);
        CHECK_INTEGER(operands.count, 2);
        CHECK_STRING(operands.ppItems[ 0 ], "-R");
        CHECK_STRING(operands.ppItems[ 1 ], "-");
}

static void
TheFirstOperandEndsTheOptions(void)
{
        TalaUtils_Options_tSpec   spec     = LsSpec();
        TalaUtils_Options_tList   operands = Operands();
        TalaUtils_Options_tResult result   = PARSE(spec, operands, "-1", "dir", "-l", "--");
        CHECK_INTEGER(result.status, TalaUtils_Options_Status_Ok);
        CHECK(Ls_flags[ '1' ] && !Ls_flags[ 'l' ]);
        CHECK_INTEGER(operands.count, 3);
        CHECK_STRING(operands.ppItems[ 1 ], "-l");
        CHECK_STRING(operands.ppItems[ 2 ], "--");
}

static void
UnknownOptionsAreReportedAsOpenBsdDoes(void)
{
        TalaUtils_Options_tSpec   spec     = LsSpec();
        TalaUtils_Options_tList   operands = Operands();
        char                      text[ 256 ];
        TalaUtils_Options_tResult result = PARSE(spec, operands, "-lz");
        CHECK_INTEGER(result.status, TalaUtils_Options_Status_UnknownOption);
        ErrorText(&spec, &result, text);
        CHECK_STRING(text, "ls: unknown option -- z\n");
        result = PARSE(spec, operands, "-z");
        ErrorText(&spec, &result, text);
        CHECK_STRING(text, "ls: unknown option -- z\n");
}

static void
TheUsageLineOfLsIsOpenBsds(void)
{
        TalaUtils_Options_tSpec spec = LsSpec();
        char                    text[ 256 ];
        FILE*                   pStream = Test_Capture();
        TalaUtils_Options_WriteUsage(&spec, pStream);
        Test_Captured(pStream, text, sizeof(text));
        CHECK_STRING(text, "usage: ls [-1AaCcdFfgHhikLlmnopqRrSsTtux] [file ...]\n");
}

static void
ValuesCountsAndLists(void)
{
        TalaUtils_Options_tSpec   spec     = LinkSpec(4);
        TalaUtils_Options_tList   operands = Operands();
        TalaUtils_Options_tResult result   = PARSE(spec,
                                                   operands,
                                                   "-oa.out",
                                                   "-o",
                                                   "b.out",
                                                   "-vvL",
                                                   "lib",
                                                   "-j",
                                                   "8",
                                                   "-v",
                                                   "-Lusr");
        CHECK_INTEGER(result.status, TalaUtils_Options_Status_Ok);
        CHECK_STRING(Link.pOutput, "b.out");
        CHECK_INTEGER(Link.jobs, 8);
        CHECK_INTEGER(Link.verbose, 3);
        CHECK_INTEGER(Link.libraryDirectories.count, 2);
        CHECK_STRING(Link.pLibraryDirectories[ 0 ], "lib");
        CHECK_STRING(Link.pLibraryDirectories[ 1 ], "usr");
}

static void
LongNamesAndOperandsBetweenOptions(void)
{
        TalaUtils_Options_tSpec   spec     = LinkSpec(4);
        TalaUtils_Options_tList   operands = Operands();
        TalaUtils_Options_tResult result
                = PARSE(spec, operands, "a.o", "-dylib", "-arch", "arm64", "b.o", "--", "-c.o");
        CHECK_INTEGER(result.status, TalaUtils_Options_Status_Ok);
        CHECK(Link.isDylib);
        CHECK_STRING(Link.pArchitecture, "arm64");
        CHECK_INTEGER(operands.count, 3);
        CHECK_STRING(operands.ppItems[ 0 ], "a.o");
        CHECK_STRING(operands.ppItems[ 1 ], "b.o");
        CHECK_STRING(operands.ppItems[ 2 ], "-c.o");
}

static void
MissingValues(void)
{
        TalaUtils_Options_tSpec   spec     = LinkSpec(4);
        TalaUtils_Options_tList   operands = Operands();
        char                      text[ 256 ];
        TalaUtils_Options_tResult result = PARSE(spec, operands, "-o");
        CHECK_INTEGER(result.status, TalaUtils_Options_Status_MissingValue);
        ErrorText(&spec, &result, text);
        CHECK_STRING(text, "ld64: option requires an argument -- o\n");
        result = PARSE(spec, operands, "a.o", "-arch");
        ErrorText(&spec, &result, text);
        CHECK_STRING(text, "ld64: option requires an argument -- arch\n");
}

static void
NumbersOutsideTheirRange(void)
{
        TalaUtils_Options_tSpec spec     = LinkSpec(4);
        TalaUtils_Options_tList operands = Operands();
        char                    text[ 256 ];
        const char*             cases[][ 2 ] = {
                { "0", "ld64: value of -j is too small: 0\n" },
                { "65", "ld64: value of -j is too large: 65\n" },
                { "8x", "ld64: value of -j is invalid: 8x\n" },
                { " 8", "ld64: value of -j is invalid:  8\n" },
                { "99999999999999999999",
                  "ld64: value of -j is too large: 99999999999999999999\n" },
        };
        for(size_t i = 0; i < sizeof(cases) / sizeof(cases[ 0 ]); i++)
        {
                char* const               words[] = { "ld64", "-j", (char*) cases[ i ][ 0 ] };
                TalaUtils_Options_tResult result
                        = TalaUtils_Options_Parse(&spec, 3, words, &operands);
                CHECK_INTEGER(result.status, TalaUtils_Options_Status_BadNumber);
                ErrorText(&spec, &result, text);
                CHECK_STRING(text, cases[ i ][ 1 ]);
        }
        CHECK_INTEGER(Link.jobs, 1);
}

static void
UnknownLongNamesAndFullLists(void)
{
        TalaUtils_Options_tSpec   spec     = LinkSpec(1);
        TalaUtils_Options_tList   operands = Operands();
        char                      text[ 256 ];
        TalaUtils_Options_tResult result = PARSE(spec, operands, "-dylibs");
        ErrorText(&spec, &result, text);
        CHECK_STRING(text, "ld64: unknown option -dylibs\n");
        result = PARSE(spec, operands, "-La", "-Lb");
        CHECK_INTEGER(result.status, TalaUtils_Options_Status_TooMany);
        ErrorText(&spec, &result, text);
        CHECK_STRING(text, "ld64: too many values for -L\n");
        TalaUtils_Options_tList none = { Operand_items, 0, 0 };
        result                       = PARSE(spec, none, "a.o");
        ErrorText(&spec, &result, text);
        CHECK_STRING(text, "ld64: too many operands\n");
}

static void
TheUsageLineIsWrappedUnderTheFirstWord(void)
{
        TalaUtils_Options_tSpec spec = LinkSpec(4);
        char                    text[ 256 ];
        FILE*                   pStream = Test_Capture();
        TalaUtils_Options_WriteUsage(&spec, pStream);
        Test_Captured(pStream, text, sizeof(text));
        CHECK_STRING(
                text,
                "usage: ld64 [-v] [-j jobs] [-L dir] [-o file] [-dylib] [-arch name] file ...\n");
        spec.pOperands = "[file ...] [another-file ...] [and-a-third-file ...]";
        pStream        = Test_Capture();
        TalaUtils_Options_WriteUsage(&spec, pStream);
        Test_Captured(pStream, text, sizeof(text));
        CHECK_STRING(text,
                     "usage: ld64 [-v] [-j jobs] [-L dir] [-o file] [-dylib] [-arch name]\n"
                     "            [file ...] [another-file ...] [and-a-third-file ...]\n");
}

static const Test_tCase Cases[] = {
        { "grouped flags and the end of options", GroupedFlagsAndTheEndOfOptions },
        { "the first operand ends the options", TheFirstOperandEndsTheOptions },
        { "unknown options are reported as OpenBSD does", UnknownOptionsAreReportedAsOpenBsdDoes },
        { "the usage line of ls is OpenBSD's", TheUsageLineOfLsIsOpenBsds },
        { "values, counts and lists", ValuesCountsAndLists },
        { "long names and operands between options", LongNamesAndOperandsBetweenOptions },
        { "missing values", MissingValues },
        { "numbers outside their range", NumbersOutsideTheirRange },
        { "unknown long names and full lists", UnknownLongNamesAndFullLists },
        { "the usage line is wrapped under the first word",
          TheUsageLineIsWrappedUnderTheFirstWord },
};

TEST_SUITE(OptionsTests, Cases);
