# minishell

A lightweight Unix shell written in C using POSIX system calls (`fork`, `execvp`, `waitpid`, `chdir`, and `sigaction`) and GNU `readline`.

## Current Features

* **Interactive Line Editing & Command History (`readline`):** Uses `readline()` for interactive editing and `add_history()` to record non-whitespace commands for history navigation.
* **Dynamic Prompt:** Displays the current working directory (`minishell:<cwd>$ `), falling back to `minishell> ` if `getcwd()` fails.
* **Custom Cursor:** Sets the terminal cursor to a blinking underscore (`\033[3 q`) on startup and resets it on exit.
* **Quote-Aware Tokenization:** Parses arguments in-place while stripping single and double quotes and collapsing extra spaces or tabs.
* **Environment Variable Expansion (`$VAR`):** Expands `$VAR` references via `getenv()` in unquoted and double-quoted input before parsing, while keeping single-quoted `$VAR` literal.
* **Built-in Commands:**
* `cd [path]` — Changes directory directly in the parent shell process (defaults to `$HOME`, or `/` if `$HOME` is unset), including quoted directory paths.
* `cd -` — Switches back to the previous working directory and prints it.
* `pwd` — Prints the current working directory via `getcwd()` (supports output redirection and pipelines).
* `echo [-n] [args...]` — Prints arguments separated by spaces, omitting the trailing newline when `-n` is passed (supports output redirection and pipelines).
* `export [NAME=VALUE ...]` — Sets or updates environment variables via `setenv()` in the parent shell, or lists current environment variables when run with no arguments.
* `unset NAME ...` — Removes environment variables via `unsetenv()` in the parent shell.
* `exit` — Exits the shell cleanly.
* Also exits cleanly on `EOF` (`Ctrl+D`).


* **External Command Execution:** Forks a child process and runs commands via `execvp()`, recovering cleanly on `fork()` failure and retrying `waitpid()` on `EINTR`.
* **I/O Redirection (`<`, `>`, `>>`):** Supports input redirection (`<`), output overwrite (`>`), and output append (`>>`) on commands and pipeline stages.
* **Pipelines (`|`):** Connects multiple command stages via `pipe()`, `fork()`, and `dup2()`, waiting for all child processes.
* **Signal Handling (`SIGINT`):** Ignores `Ctrl+C` in the parent shell so the prompt stays alive, while restoring default `SIGINT` behavior (`SIG_DFL`) in child processes.

## Build & Run

```bash
gcc -Wall -Wextra -o minishell minishell.c -lreadline
./minishell

```

## Roadmap

1. Smarter Parsing (Quotes & Multiple Spaces tokenization) - Done
2. I/O redirection (`<`, `>`, `>>`) - Done
3. Pipelines (`|`) - Done
4. Additional built-ins (`pwd`, `echo`, `export`, `unset`) and environment variable expansion (`$VAR`) - Done
5. Interactive Line editing and command history (`readline`) - Done
---
