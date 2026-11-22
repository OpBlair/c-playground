# minishell

A lightweight Unix shell written in C using POSIX system calls (`fork`, `execvp`, `waitpid`, `chdir`, and `sigaction`).

## Current Features

- **Dynamic Prompt:** Displays the current working directory (`minishell:<cwd>$ `), falling back to `minishell> ` if `getcwd()` fails.
- **Custom Cursor:** Sets the terminal cursor to a blinking underscore (`\033[3 q`) on startup and resets it on exit.
- **Quote-Aware Tokenization:** Parses arguments in-place while stripping single and double quotes and collapsing extra spaces or tabs.
- **Built-in Commands:**
  - `cd [path]` — Changes directory directly in the parent shell process (defaults to `$HOME`, or `/` if `$HOME` is unset), including quoted directory paths.
  - `cd -` — Switches back to the previous working directory and prints it.
  - `exit` — Exits the shell cleanly.
  - Also exits cleanly on `EOF` (`Ctrl+D`).
- **External Command Execution:** Forks a child process and runs commands via `execvp()`, recovering cleanly on `fork()` failure and retrying `waitpid()` on `EINTR`.
- **Signal Handling (`SIGINT`):** Ignores `Ctrl+C` in the parent shell so the prompt stays alive, while restoring default `SIGINT` behavior (`SIG_DFL`) in child processes.

## Build & Run

```bash
gcc -Wall -Wextra -o minishell minishell.c
./minishell

```

## Roadmap

1. Smarter Parsing (Quotes & Multiple Spaces tokenization) - Done
2. I/O redirection (`<`, `>`, `>>`)
3. Pipelines (`|`)
4. Additional built-ins (`pwd`, `echo`, `export`, `unset`) and environment variable expansion (`$VAR`)
5. Interactiev Line editing and command history (`readline`)
