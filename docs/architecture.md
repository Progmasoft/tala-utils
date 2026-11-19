<!-- SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com> -->
<!-- SPDX-License-Identifier: MIT -->

# Architecture

## One repository, several executables, one version

Tala Utils are one repository that builds several executables, as `ls`, `cp` or TalaSh. They are released together:
every executable has the version in `VERSION`, which the build passes to the code as `TALAUTILS_VERSION`.

| Directory | What it holds |
| --- | --- |
| `common/` | the code the executables share, built into `build/libcommon.a` |
| `tests/` | the tests of `common/` |
| `<name>/` | one directory for each executable, when it is written |

## The command line parser, `common/TalaUtils.Options.h`

Each executable lists its options in a table of `TalaUtils_Options_tOption`: a short name, a long name or both,
what the option does (`Kind_Flag`, `Kind_Count`, `Kind_Text`, `Kind_List`, `Kind_Number`), and the one field of the
right type it writes. `TalaUtils_Options_Parse` fills those fields and the operands; nothing after it reads the text
of `argv` again. The parser is written here, by hand, with no library for command lines.

The syntax is POSIX.1-2024's (XBD 12.2, Utility Syntax Guidelines) with TalaOS's one-dash long names:

| Words | Meaning |
| --- | --- |
| `-la` | `-l -a`: short options grouped behind one dash |
| `-ofile`, `-o file` | the value of a short option, in its word or the next one |
| `-dylib`, `-arch arm64` | a long name, one dash; its value is always the next word |
| `--` | the end of the options |
| `-` | an operand |

A word is matched whole against the long names first and read as grouped short options only when none matches, so
`-dylib` is never `-d -y -l -i -b`. The first operand ends the options unless the table sets `isInterleaved`, as a
linker's does (`ld64 a.o -o a.out b.o`). A number is a decimal integer within the option's range, read as OpenBSD's
`strtonum` reads it; a list keeps every value in order.

Errors and the usage line are written as OpenBSD's utilities write them, and `TalaUtils_Options_ParseOrExit` ends the
program with status 1 after them:

```text
ls: unknown option -- z
usage: ls [-1AaCcdFfgHhikLlmnopqRrSsTtux] [file ...]
```

The usage line comes from the table: flags with a short name only in one bracket, in OpenBSD's order (digits, then
letters, the capital before the small one), then the short options that take a value, then the long options, then the
operands, wrapped at 80 columns under the first word.

`TalaUtils_Options_Check` checks a table: no name given twice, the target field matching the kind, a value name exactly
for the options that take a value, and no long name that would also read as grouped short flags (`-al` beside `-a`
and `-l`). Each executable's tests run it on its table.
