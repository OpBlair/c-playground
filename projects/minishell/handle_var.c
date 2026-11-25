#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

static void print_echo(char *const args[]){
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
        if(args[i + 1] != NULL) {
            putchar(' ');
        }
        i++;        
    }

    if (print_newline) {
        putchar('\n');
    }
}

int main(void) {
    char buf[256];

    // pwd standalone getcwd()
    if (getcwd(buf, sizeof(buf)) != NULL) {
        printf("%s\n", buf);
    } else {
        perror("getcwd");
    }

    // $VAR lookup standalone getenv()
    char *user = getenv("USER");
    if (user != NULL) {
        printf("%s\n", user);
    } else {
        printf("USER is not set\n");
    }

    // export and unset standalone setenv(), unsetenv()
    if (setenv("DEMO_VAR", "hello_minishell", 1) != 0) {
        perror("setenv");
    } else {
        char *test_value = getenv("DEMO_VAR");
        if ( test_value != NULL) {
            printf("%s\n", test_value);
        }
    }

    if (unsetenv("DEMO_VAR") != 0){
        perror("unsetenv");
    } else if (getenv("DEMO_VAR") == NULL) {
        printf("variable doesn't exist\n");
    }
    
    // echo standalone -n \n
    char *sample1[] = {"echo", "hello", "world", NULL};
    char *sample2[] = {"echo", "-n", "hello", "world", NULL};
    char *sample3[] = {"echo", NULL};

    print_echo(sample1);
    print_echo(sample2);
    print_echo(sample3);

    return 0;
}
