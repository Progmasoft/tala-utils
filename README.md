<!-- SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com> -->
<!-- SPDX-License-Identifier: MIT -->

# Tala Utils

Tala Utils are the base utilities of TalaOS: the BSD utilities and tools such as `sudo`. The `tala-utils` package
holds several executables, among them TalaSh.

TalaSh (`talash`) is the shell: a friendly interactive shell that is compatible with Unix and POSIX. It has none of
the extensions of Bash or Zsh. `sh` is a symbolic link to `talash`, and TalaSh is part of this package, not a
package of its own.

TalaSh reads these files as it starts, in this order:

| How TalaSh starts | What it reads |
| --- | --- |
| As a login shell (`-talash`, `talash -l`) | `/etc/profile`, then `~/.talash_profile`, or `~/.profile` when there is none |
| As an interactive shell, a login shell too | the file `$ENV` names, or `~/.talash_rc` when `ENV` is not set |
| For a script or `talash -c` | nothing |
| As `sh` | `/etc/profile` and `~/.profile` as a login shell, `$ENV` when interactive; no `.talash_` file |

`~/.profile` is the file every POSIX shell shares; `~/.talash_profile` can read it with `. "$HOME/.profile"`.
TalaSh skips, with a warning, a file that is not owned by the user or by root or that others can write, and it
ignores `ENV` when it runs set-user-ID or set-group-ID.

The utilities follow Unix in their command lines: options start with one dash, as `-l`, `-c` or a longer name such
as `-login`; there are no GNU-style two-dash `--options`.

They are written in C11 under the
[Progmasoft Core C Coding Guidelines](https://github.com/Progmasoft/progmasoft-cpp) and built with BSD Make.

No code is written yet.

## License

[MIT](LICENSE.txt), also covered by the [Progmasoft Patent Grant 1.1](PATENTS.md). Participation follows the
[Code of Conduct](CODE_OF_CONDUCT.md).
