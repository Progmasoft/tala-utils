<!-- SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com> -->
<!-- SPDX-License-Identifier: MIT -->

# Development

Tala Utils need BSD Make (`bmake`) and a C compiler; the code is C11 with
`-pedantic-errors -Wall -Wextra -Werror`.

```sh
bmake                                          # build/libcommon.a
bmake test                                     # the tests
bmake SANITIZE=-fsanitize=address,undefined test
bmake format                                   # clang-format 23
```

The tests are one program, `build/tests/tala-utils-tests`: `tests/Test.Main.c` runs the cases of each test file and
fails when a check fails. `tests/Options.Tests.c` parses with OpenBSD's `ls` table, whose usage line must be
OpenBSD's own, and with a linker-like table for values, numbers, lists, long names and operands between options;
`tests/Options.Check.Tests.c` gives `Check` a table with each kind of mistake.

CI runs the tests on Linux with AddressSanitizer and UndefinedBehaviorSanitizer and on macOS, checks the format, and
checks the workflows with zizmor and actionlint.
