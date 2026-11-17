#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

        // echo what they typed
        printf("You ran: %s\n", input);
    }

    return 0;
}
