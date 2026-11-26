#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h> // needed for EINTR check in waitpid
#include <fcntl.h> // IO redirection header file

#define MAX_LINE 1024
#define MAX_ARGS 64

// Function to reset cursor from _ to system default
static void reset_cursor(void){
    printf("\033[0 q");
    fflush(stdout);
}

// Helper for valid shell variable / identifier first character
static int is_valid_var_start(char c) {
    return isalpha((unsigned char)c) || c == '_';
}

// Helper for valid subsequent characters in a shell variable / identifier
static int is_valid_var_char(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

// Expand $VAR using getenv() into `out` while keeping single-quoted $VAR literal
static int expand_vars(const char *input, char *out, size_t out_size) {
    size_t in_idx = 0;
    size_t out_idx = 0;
    int in_single_quotes = 0;
    int in_double_quotes = 0;

    while (input[in_idx] != '\0') {
        char c = input[in_idx];

        // Track single quotes unless already inside double quotes
        if (c == '\'' && !in_double_quotes) {
            in_single_quotes = !in_single_quotes;
            if (out_idx + 1 >= out_size) return -1;
            out[out_idx++] = c;
            in_idx++;
            continue;
        }

        // Track double quotes unless already inside single quotes
        if (c == '"' && !in_single_quotes) {
            in_double_quotes = !in_double_quotes;
            if (out_idx + 1 >= out_size) return -1;
            out[out_idx++] = c;
            in_idx++;
            continue;
        }

        // Expand $VAR outside single quotes (both unquoted and double-quoted)
        if (c == '$' && !in_single_quotes && is_valid_var_start(input[in_idx + 1])) {
            size_t start = in_idx + 1;
            size_t end = start;
            while (is_valid_var_char(input[end])) {
                end++;
            }

            size_t name_len = end - start;
            char var_name[256];
            if (name_len >= sizeof(var_name)) {
                fprintf(stderr, "minishell: variable name too long\n");
                return -1;
            }
            memcpy(var_name, input + start, name_len);
            var_name[name_len] = '\0';

            // Look up variable value; unset variables expand to nothing
            const char *val = getenv(var_name);
            if (val != NULL) {
                size_t val_len = strlen(val);
                if (out_idx + val_len >= out_size) {
                    fprintf(stderr, "minishell: expanded line too long\n");
                    return -1;
                }
                memcpy(out + out_idx, val, val_len);
                out_idx += val_len;
            }

            in_idx = end;
            continue;
        }

        if (out_idx + 1 >= out_size) {
            fprintf(stderr, "minishell: input line too long\n");
            return -1;
        }
        out[out_idx++] = c;
        in_idx++;
    }

    out[out_idx] = '\0';
    return 0;
}

// Function for parsing quotes
int parse_command(char *input, char **args){
    int arg_count = 0;
    char *p = input;

    while (*p != '\0' && arg_count < MAX_ARGS - 1) {
        // skip leading whitespaces (spaces and tabs)
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        if (*p == '\0') {
            break;
        }

        // Mark the start of the token, and use 'dest' to build
        // the cleaned string in-place (stripping quotes)
        args[arg_count++] = p;
        char *dest = p;

        int in_single_quotes = 0;
        int in_double_quotes = 0;

        while (*p != '\0') {
            // Handle single quotes
            if (*p == '\'' && !in_double_quotes) {
                in_single_quotes = !in_single_quotes;
                p++; // skip the quote character
                continue;
            }
            // Handle double quotes
            if (*p == '\"' && !in_single_quotes) {
                in_double_quotes = !in_double_quotes;
                p++; // skip the quote character
                continue;
            }

            // If a whitespace is hit outside of quotes, end the token
            if ((*p == ' ' || *p == '\t') && !in_single_quotes && !in_double_quotes) {
                p++; // Advance past the space
                break;
            }

            // copy character to destination (shitfs left if quotes were removed)
            *dest++ = *p++;
        }

        // Null-terminate the token
        *dest = '\0';
    }

    args[arg_count] = NULL; // Required for execvp
    return arg_count; // Return safely after all tokens are parsed
}

// Built-in `pwd` using getcwd()
static void print_pwd(void) {
    char buf[MAX_LINE];
    if (getcwd(buf, sizeof(buf)) != NULL) {
        printf("%s\n", buf);
    } else {
        perror("minishell: pwd");
    }
}

// Built-in `echo` supporting one or more leading "-n" flags
static void print_echo(char *const args[]) {
    int i = 1;
    int print_newline = 1;

    // handle one or more leading "-n" flags
    while (args[i] != NULL && strcmp(args[i], "-n") == 0) {
        print_newline = 0;
        i++;
    }

    // Print remaining arguments separated by a single space
    while (args[i] != NULL) {
        fputs(args[i], stdout);
        if (args[i + 1] != NULL) {
            putchar(' ');
        }
        i++;
    }

    if (print_newline) {
        putchar('\n');
    }
    fflush(stdout);
}

// Validate identifier syntax for `export` and `unset`
static int is_valid_identifier(const char *name) {
    if (name == NULL || !is_valid_var_start(name[0])) {
        return 0;
    }
    for (size_t i = 1; name[i] != '\0'; i++) {
        if (!is_valid_var_char(name[i])) {
            return 0;
        }
    }
    return 1;
}

// Built-in `export`: sets environment variables with setenv() or lists `environ`
static void handle_export(char *const args[]) {
    if (args[1] == NULL) {
        extern char **environ;
        for (int i = 0; environ != NULL && environ[i] != NULL; i++) {
            printf("declare -x %s\n", environ[i]);
        }
        return;
    }

    for (int i = 1; args[i] != NULL; i++) {
        char *eq = strchr(args[i], '=');
        if (eq != NULL) {
            size_t name_len = (size_t)(eq - args[i]);
            char name[256];
            if (name_len == 0 || name_len >= sizeof(name)) {
                fprintf(stderr, "minishell: export: `%s': not a valid identifier\n", args[i]);
                continue;
            }
            memcpy(name, args[i], name_len);
            name[name_len] = '\0';

            if (!is_valid_identifier(name)) {
                fprintf(stderr, "minishell: export: `%s': not a valid identifier\n", args[i]);
                continue;
            }

            if (setenv(name, eq + 1, 1) != 0) {
                perror("minishell: export");
            }
        } else {
            if (!is_valid_identifier(args[i])) {
                fprintf(stderr, "minishell: export: `%s': not a valid identifier\n", args[i]);
                continue;
            }
            // Preserve existing value if already set; otherwise initialize empty
            if (getenv(args[i]) == NULL) {
                if (setenv(args[i], "", 0) != 0) {
                    perror("minishell: export");
                }
            }
        }
    }
}

// Built-in `unset`: removes environment variables with unsetenv()
static void handle_unset(char *const args[]) {
    if (args[1] == NULL) {
        fprintf(stderr, "minishell: unset: not enough arguments\n");
        return;
    }

    for (int i = 1; args[i] != NULL; i++) {
        if (!is_valid_identifier(args[i])) {
            fprintf(stderr, "minishell: unset: `%s': not a valid identifier\n", args[i]);
            continue;
        }
        if (unsetenv(args[i]) != 0) {
            perror("minishell: unset");
        }
    }
}

// Function to handle `cd`
static void handle_cd(char **args, char *prev_dir, size_t prev_dir_size) {
    const char *target = NULL;
    int print_dir = 0;

    if (args[1] == NULL) {
        // `cd` alone goes to $HOME, or "/" if unset
        target = getenv("HOME");
        if (target == NULL || *target == '\0') {
            target = "/";
        }
    } else if (strcmp(args[1], "-") == 0) {
        if (prev_dir[0] == '\0') {
            fprintf(stderr, "minishell: cd: OLDPWD not set\n");
            return;
        }
        target = prev_dir;
        print_dir = 1;
    } else {
        target = args[1];
    }

    char current_before_cd[MAX_LINE];
    int have_old = (getcwd(current_before_cd, sizeof(current_before_cd)) != NULL);

    if (chdir(target) != 0) {
        perror("minishell: cd failed");
        return;
    }

    if (have_old) {
        snprintf(prev_dir, prev_dir_size, "%s", current_before_cd);
    }

    if (print_dir) {
        char now_dir[MAX_LINE];
        if (getcwd(now_dir, sizeof(now_dir)) != NULL) {
            printf("%s\n", now_dir);
        }
    }
}

// --- IO REDIRECTION (<, >, >>) ---
typedef enum {
    REDIR_IN,       // <  : read stdin from file
    REDIR_OUT,      // >  : write stdout to file (truncate/create)
    REDIR_APPEND,   // >> : append stdout to file (create if missing)
} RedirType;

typedef struct {
    RedirType type; // which redirection operator was used
    char *file;     // target file path
} Redirection;

#define MAX_REDIRS 16 // max redirections allowed per command

// Extracts `<`, `>`, and `>>` tokens from `args` in-place, recording them in
// left-to-right order in `redirs`. Returns count (>= 0), or -1 on syntax error.
static int extract_redirections(char **args, Redirection *redirs, int max_redirs) {
    int read_idx = 0;
    int write_idx = 0;   // compacts non-redirection args back into `args`
    int redir_count = 0;

    while (args[read_idx] != NULL) {
        int is_in = (strcmp(args[read_idx], "<") == 0);
        int is_append = (strcmp(args[read_idx], ">>") == 0);
        int is_out = (strcmp(args[read_idx], ">") == 0);

        if (is_in || is_append || is_out) {
            // Redirection operator must be followed by a filename
            if (args[read_idx + 1] == NULL) {
                fprintf(stderr, "minishell: syntax error near unexpected token `newline'\n");
                return -1;
            }
            // Guard against overflowing the fixed `redirs` array
            if (redir_count >= max_redirs) {
                fprintf(stderr, "minishell: too many redirections\n");
                return -1;
            }

            RedirType t = is_in ? REDIR_IN : (is_append ? REDIR_APPEND : REDIR_OUT);
            redirs[redir_count].type = t;
            redirs[redir_count].file = args[read_idx + 1];
            redir_count++;

            read_idx += 2; // skip both the operator and target filename
            continue;
        }

        // Keep normal command/argument tokens so execvp gets clean argv
        args[write_idx++] = args[read_idx++];
    }

    args[write_idx] = NULL; // re-terminate cleaned args array for execvp
    return redir_count;
}

// Applies recorded redirections inside the child process before execvp().
// Returns 0 on success, -1 if open() or dup2() fails.
static int apply_redirections(const Redirection *redirs, int redir_count) {
    for (int i = 0; i < redir_count; i++) {
        int fd = -1;
        int target_fd = -1;

        if (redirs[i].type == REDIR_IN) {
            fd = open(redirs[i].file, O_RDONLY);
            target_fd = STDIN_FILENO; // redirect stdin (fd 0)
        } else if (redirs[i].type == REDIR_OUT) {
            // 0644 = rw-r--r-- permissions when creating a new file
            fd = open(redirs[i].file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            target_fd = STDOUT_FILENO; // redirect stdout (fd 1)
        } else { // REDIR_APPEND
            fd = open(redirs[i].file, O_WRONLY | O_CREAT | O_APPEND, 0644);
            target_fd = STDOUT_FILENO; // redirect stdout (fd 1)
        }

        if (fd < 0) {
            perror(redirs[i].file);
            return -1;
        }

        // Point standard fd (stdin/stdout) at the opened file
        if (dup2(fd, target_fd) < 0) {
            perror("minishell: dup2 failed");
            close(fd);
            return -1;
        }

        close(fd); // close original fd; duplicated fd stays active across execvp
    }

    return 0;
}

// Fork a child so `pwd` or `echo` redirections don't mutate the parent shell's fds
static int run_redirected_output_builtin(char **args, const Redirection *redirs, int redir_count) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        return -1;
    }

    if (pid == 0) {
        struct sigaction sa;
        sa.sa_handler = SIG_DFL;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGINT, &sa, NULL);

        if (apply_redirections(redirs, redir_count) < 0) {
            _exit(1);
        }

        if (strcmp(args[0], "pwd") == 0) {
            print_pwd();
        } else if (strcmp(args[0], "echo") == 0) {
            print_echo(args);
        }
        _exit(0);
    }

    int status;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) {
            perror("minishell: waitpid failed");
            break;
        }
    }
    return 0;
}

// --- IO REDIRECTION ENDS ---

// --- HANDLE PIPELINE -----
#define MAX_PIPELINE_STAGES 32

static void handle_pipeline(char **args) {
    char *stages[MAX_PIPELINE_STAGES][MAX_ARGS];
    int stage_count = 0;
    int arg_in_stage = 0;

    // 1. Split `args` on unquoted "|" tokens into separate command argvs
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "|") == 0) {
            if (arg_in_stage == 0) {
                fprintf(stderr, "minishell: syntax error near unexpected token `|'\n");
                return;
            }
            if (stage_count >= MAX_PIPELINE_STAGES - 1) {
                fprintf(stderr, "minishell: too many pipeline stages\n");
                return;
            }
            stages[stage_count][arg_in_stage] = NULL;
            stage_count++;
            arg_in_stage = 0;
        } else {
            if (arg_in_stage >= MAX_ARGS - 1) {
                fprintf(stderr, "minishell: too many arguments in stage\n");
                return;
            }
            stages[stage_count][arg_in_stage++] = args[i];
        }
    }

    // Reject trailing "|" (e.g., `ls |`)
    if (arg_in_stage == 0) {
        fprintf(stderr, "minishell: syntax error near unexpected token `newline'\n");
        return;
    }
    stages[stage_count][arg_in_stage] = NULL;
    stage_count++;

    pid_t pids[MAX_PIPELINE_STAGES];
    int prev_read_fd = -1;
    int spawned = 0;

    // 2. Wire pipes and fork each stage
    for (int i = 0; i < stage_count; i++) {
        int pipefd[2] = {-1, -1};
        int has_next = (i < stage_count - 1);

        if (has_next && pipe(pipefd) < 0) {
            perror("minishell: pipe failed");
            if (prev_read_fd != -1) {
                close(prev_read_fd);
            }
            break;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("minishell: fork failed");
            if (prev_read_fd != -1) {
                close(prev_read_fd);
            }
            if (has_next) {
                close(pipefd[0]);
                close(pipefd[1]);
            }
            break;
        }

        if (pid == 0) {
            // --- CHILD PROCESS ---
            struct sigaction sa;
            sa.sa_handler = SIG_DFL;
            sigemptyset(&sa.sa_mask);
            sa.sa_flags = 0;
            sigaction(SIGINT, &sa, NULL);

            // Connect stdin to previous stage's pipe read end
            if (prev_read_fd != -1) {
                if (dup2(prev_read_fd, STDIN_FILENO) < 0) {
                    perror("minishell: dup2 failed");
                    _exit(1);
                }
                close(prev_read_fd);
            }

            // Connect stdout to current stage's pipe write end
            if (has_next) {
                close(pipefd[0]); // child does not read its own output pipe
                if (dup2(pipefd[1], STDOUT_FILENO) < 0) {
                    perror("minishell: dup2 failed");
                    _exit(1);
                }
                close(pipefd[1]);
            }

            // Extract and apply stage-local `<`, `>`, `>>` redirections
            Redirection redirs[MAX_REDIRS];
            int redir_count = extract_redirections(stages[i], redirs, MAX_REDIRS);
            if (redir_count < 0) {
                _exit(1);
            }
            if (stages[i][0] == NULL) {
                _exit(0);
            }
            if (apply_redirections(redirs, redir_count) < 0) {
                _exit(1);
            }

            // Support `pwd` and `echo` inside pipeline stages
            if (strcmp(stages[i][0], "pwd") == 0) {
                print_pwd();
                _exit(0);
            }
            if (strcmp(stages[i][0], "echo") == 0) {
                print_echo(stages[i]);
                _exit(0);
            }

            execvp(stages[i][0], stages[i]);
            fprintf(stderr, "minishell: command not found: %s\n", stages[i][0]);
            _exit(127);
        }

        // --- PARENT PROCESS ---
        if (prev_read_fd != -1) {
            close(prev_read_fd); // done with prior stage's read fd
        }
        if (has_next) {
            close(pipefd[1]); // critical: close write fd so downstream readers see EOF
            prev_read_fd = pipefd[0]; // pass read fd into next iteration
        } else {
            prev_read_fd = -1;
        }

        pids[spawned++] = pid;
    }

    // 3. Wait for all spawned child processes
    int last_signaled = 0;
    for (int i = 0; i < spawned; i++) {
        int status;
        while (waitpid(pids[i], &status, 0) < 0) {
            if (errno != EINTR) {
                perror("minishell: waitpid failed");
                break;
            }
        }
        if (WIFSIGNALED(status)) {
            last_signaled = 1;
        }
    }

    if (last_signaled) {
        printf("\n");
    }
}
// ---- END HANDLE PIPELINE ----

// ---- HANDLE VAR ------

// ---- END HANDLE VAR ----
int main(void) {
    char input[MAX_LINE];
    char expanded[MAX_LINE];
    char prev_dir[MAX_LINE] = "";

    // --- START AT THE ROOT DIRECTORY OF THE OS ---
    if (chdir("/") != 0) {
        perror("minishell: failed to change to root directory");
    }

    // --- SET TERMINAL CURSOR TO A BLINKING UNDERSCORE ---
    // \033[3 q = Blinking underline cursor shape]
    printf("\033[3 q");
    fflush(stdout);
    atexit(reset_cursor);

    // --- IGNORE SIGINT IN THE PARENT SHELL ---
    struct sigaction sa;
    sa.sa_handler = SIG_IGN; // tell the shell to ignore Ctrl+C
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    while(1) {
        // --- FETCH CURRENT WORKING DIRECTORY ---
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("minishell:%s$ ", cwd);
        } else {
            // print prompt
            printf("minishell> ");
        }
        
        fflush(stdout);

        // Read input
        if (fgets(input, MAX_LINE, stdin) == NULL) {
            break; // Handle EOF (Ctrl+D)
        }

        // Remove the trailing newline character left by fgets
        input[strcspn(input, "\n")] = 0;

        // if user pressed enter, loop again
        if (strlen(input) == 0) {
            continue;
        }

        // Expand $VAR before quote stripping so quote boundaries stay clear
        if (expand_vars(input, expanded, sizeof(expanded)) < 0) {
            continue;
        }

        // Parse before built-ins so quoted paths (like cd "my dir") are unquoted
        char *args[MAX_ARGS];
        parse_command(expanded, args);

        // if the line was just spaces/empty after parsing, loop again
        if (args[0] == NULL) {
            continue;
        }

        // if user types 'exit', quit the shell
        if (strcmp(args[0], "exit") == 0) {
            break;
        }

        // --- HANDLE 'cd' BUILT-IN ---
        if (strcmp(args[0], "cd") == 0) {
            handle_cd(args, prev_dir, sizeof(prev_dir));
            continue; // skip fork/exec and loop back to prompt
        }

        // --- HANDLE 'export' AND 'unset' BUILT-INS (PARENT PROCESS) ---
        if (strcmp(args[0], "export") == 0) {
            handle_export(args);
            continue;
        }
        if (strcmp(args[0], "unset") == 0) {
            handle_unset(args);
            continue;
        }

        // --- EXECUTION LOGIC ---
        // tokenization (splitting "ls -l" into ["ls", "-l", NULL])
        // (moved above built-ins so `cd` can use parsed arguments)
        
        /*
        int i = 0;
        char *token = strtok(input, " ");
        while (token != NULL && i < 63) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL; //execvp requires a NULL-terminated array
        */

        // Handle Pipeline (|)
        int has_pipe = 0;
        for (int i = 0; args[i] != NULL; i++) {
            if (strcmp(args[i], "|") == 0) {
                has_pipe = 1;
                break;
            }
        }

        if (has_pipe) {
            handle_pipeline(args);
            continue;
        }

        // Extract `<`, `>`, and `>>` before forking so syntax errors don't spawn children
        Redirection redirs[MAX_REDIRS];
        int redir_count = extract_redirections(args, redirs, MAX_REDIRS);
        if (redir_count < 0) {
            continue;
        }
        // Skip execution if line only contained redirections (e.g., `> out.txt`)
        if (args[0] == NULL) {
            continue;
        }

        // Run `pwd` and `echo` directly, or fork when redirecting output
        if (strcmp(args[0], "pwd") == 0) {
            if (redir_count > 0) {
                run_redirected_output_builtin(args, redirs, redir_count);
            } else {
                print_pwd();
            }
            continue;
        }
        if (strcmp(args[0], "echo") == 0) {
            if (redir_count > 0) {
                run_redirected_output_builtin(args, redirs, redir_count);
            } else {
                print_echo(args);
            }
            continue;
        }
        
        // Fork a child procecss
        pid_t pid = fork();

        if (pid < 0) {
            perror("Fork failed");
            continue; // stay in shell instead of exiting on fork error
        } else if (pid == 0) {
            // --- CHILD PROCESS ---
            // The child process handles Ctrl+C normally
            sa.sa_handler = SIG_DFL;
            sigaction(SIGINT, &sa, NULL);

            // Apply redirections in child only so parent shell's stdin/stdout stay intact
            if (apply_redirections(redirs, redir_count) < 0) {
                _exit(1);
            }

            // Execute the command. If it succeeds, replace the process.
            if (execvp(args[0], args) < 0){
                printf("minishell: command not found: %s\n", args[0]);
            }
            _exit(127); // Exit child without flushing parent stdio buffers
        } else {
            // --- PARENT PROCESS ---
            // Wait for child process to finish running the command    
            int status;
            while (waitpid(pid, &status, 0) < 0) {
                if (errno != EINTR) {
                    perror("minishell: waitpid failed");
                    break;
                }
            }
            if (WIFSIGNALED(status)) {
                printf("\n"); // keep prompt on a new line after signal kill
            }
        }
    }

    return 0;
}

// https://designed-to-scale.vercel.app
// strtok
/*
canonical mode, raw mode
*/
