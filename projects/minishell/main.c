#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_LINE 1024

int main(void) {
    char input[MAX_LINE];

    while(1) {
        // print prompt
        printf("minishell> ");
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

        // --- NEW: EXECUTION LOGIC ---
        // tokenization (splitting "ls -l" into ["ls", "-l", NULL])
        char *args[64];
        int i = 0;
        char *token = strtok(input, " ");
        while (token != NULL && i < 63) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL; //execvp requires a NULL-terminated array

        // Fork a child procecss
        pid_t pid = fork();

        if (pid < 0) {
            perror("Fork failed");
            exit(1);
        } else if (pid == 0) {
            // --- CHILD PROCESS ---
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
