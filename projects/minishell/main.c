#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_LINE 1024
#define MAX_ARGS 64

// Function to reset cursor from _ to system default
static void reset_cursor(void){
    printf("\033[0 q");
    fflush(stdout);
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

int main(void) {
    char input[MAX_LINE];
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

        // if user types 'exit', quit the shell
        if (strcmp(input, "exit") == 0) {
            break;
        }

        // --- HANDLE 'cd' BUILT-IN ---
        if (strcmp(input, "cd") == 0 || strncmp(input, "cd ", 3) == 0 ||  strncmp(input, "cd\t", 3) == 0) {
            // Extract the path after "cd "
            char *path = input + 2;

            // skip any tabs or spaces after "cd"
            while (*path == ' ' || *path == '\t'){
                path++;
            } 

            // Trim trailing spaces or tabs
            size_t len = strlen(path);
            while (len > 0 && (path[len - 1] == ' ' || path[len - 1] == '\t')){
                path[--len] = '\0';
            }

            // Default to HOME (or "/" if HOME is unset)
            int print_dir = 0;
            if (*path == '\0') {
                // Standard POSIX: `cd` alone goes to $HOME
                path = getenv("HOME");
                if (path == NULL) {
                    path = "/";
                }
            } else if (strcmp(path, "-") == 0) {
                // switch to previous working directory
                if (prev_dir[0] == '\0') {
                    fprintf(stderr, "minishell: cd: OLDPWD not set\n");
                    continue;
                }
                path = prev_dir;
                print_dir = 1;
            }

            char current_before_cd[MAX_LINE];
            int have_old = (getcwd(current_before_cd, sizeof(current_before_cd)) != NULL);

            // Call the system's chdir function directly in the parent shell
            if (chdir(path) != 0) {
                perror("minishell: cd failed");
                continue;
            }

            if (have_old) {
                snprintf(prev_dir, sizeof(prev_dir), "%s", current_before_cd);
            }

            if (print_dir) {
                char now_dir[MAX_LINE];
                if (getcwd(now_dir, sizeof(now_dir)) != NULL){
                    printf("%s\n", now_dir);
                }
            }
            continue; // skip fork/exec and loop back to prompt
        }

        // --- EXECUTION LOGIC ---
        // tokenization (splitting "ls -l" into ["ls", "-l", NULL])
        char *args[MAX_ARGS];
        parse_command(input, args);

        // if the line was just spaces/empty after parsing, loop again
        if (args[0] == NULL) {
            continue;
        }
        
        /*
        int i = 0;
        char *token = strtok(input, " ");
        while (token != NULL && i < 63) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL; //execvp requires a NULL-terminated array
        */
        
        // Fork a child procecss
        pid_t pid = fork();

        if (pid < 0) {
            perror("Fork failed");
            exit(1);
        } else if (pid == 0) {
            // --- CHILD PROCESS ---
            // The child process handles Ctrl+C normally
            sa.sa_handler = SIG_DFL;
            sigaction(SIGINT, &sa, NULL);

            // Execute the command. If it succeeds, replace the process.
            if (execvp(args[0], args) < 0){
                printf("minishell: command not found: %s\n", args[0]);
            }
            exit(1); // Exit child if execvp fails
        } else {
            // --- PARENT PROCESS ---
            // Wait for child process to finish running the command    
            int status;
            waitpid(pid, &status, 0);
        }
    }

    return 0;
}
// strtok
