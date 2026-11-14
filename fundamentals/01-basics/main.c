#include <stdio.h>
#include <string.h>

int main() {
    // ==========================================
    // SECTION 1: Variables, Data Types & Operators
    // ==========================================
    printf("=== SECTION 1: Variables & Operators ===\n");
    
    int age = 25;
    float temperature = 36.6f;
    double pi = 3.14159265359;
    char grade = 'A';

    // Using sizeof() operator
    printf("Size of int: %zu bytes\n", sizeof(int));
    printf("Size of float: %zu bytes\n", sizeof(float));
    printf("Size of double: %zu bytes\n", sizeof(double));
    printf("Size of char: %zu byte\n\n", sizeof(char));

    // Arithmetic operators
    int a = 10, b = 3;
    printf("Arithmetic: %d + %d = %d\n", a, b, a + b);
    printf("Modulo (Remainder): %d %% %d = %d\n", a, b, a % b);

    // Logical operators
    int isAdult = (age >= 18); // 1 (true)
    int hasPermission = 0;     // 0 (false)
    printf("Logical AND (isAdult && hasPermission): %d\n\n", isAdult && hasPermission);

    // ==========================================
    // SECTION 2: Input & Output (stdio.h)
    // ==========================================
    printf("=== SECTION 2: Input & Output ===\n");
    
    char name[50];
    int userAge;

    // Safe string input with fgets
    printf("Enter your full name: ");
    if (fgets(name, sizeof(name), stdin) != NULL) {
        // Remove trailing newline character left by fgets if present
        name[strcspn(name, "\n")] = 0;
    }

    // Formatted input with scanf
    printf("Enter your age: ");
    scanf("%d", &userAge);

    // Formatted output with printf
    printf("Hello, %s! You are %d years old.\n\n", name, userAge);

    // Clear input buffer leftover (optional safety step after scanf)
    while (getchar() != '\n');

    // ==========================================
    // SECTION 3: Conditionals
    // ==========================================
    printf("=== SECTION 3: Conditionals ===\n");
    
    // if / else if / else
    if (userAge < 13) {
        printf("Category: Child\n");
    } else if (userAge >= 13 && userAge < 20) {
        printf("Category: Teenager\n");
    } else {
        printf("Category: Adult\n");
    }

    // switch statement
    int choice = 2;
    printf("Switch menu choice: %d -> ", choice);
    switch (choice) {
        case 1:
            printf("Selected Easy mode.\n");
            break;
        case 2:
            printf("Selected Medium mode.\n");
            break;
        case 3:
            printf("Selected Hard mode.\n");
            break;
        default:
            printf("Invalid choice.\n");
    }
    printf("\n");

    // ==========================================
    // SECTION 4: Loops (for, while, do-while)
    // ==========================================
    printf("=== SECTION 4: Loops ===\n");

    // 1. For loop with break and continue
    printf("For loop (skipping 3, breaking at 5):\n");
    for (int i = 1; i <= 6; i++) {
        if (i == 3) {
            continue; // Skip iteration 3
        }
        if (i == 5) {
            break;    // Exit loop completely at 5
        }
        printf("%d ", i);
    }
    printf("\n\n");

    // 2. While loop
    printf("While loop countdown: ");
    int count = 3;
    while (count > 0) {
        printf("%d ", count);
        count--;
    }
    printf("Liftoff!\n\n");

    // 3. Do-While loop (guarantees at least one execution)
    int passwordAttempt;
    do {
        printf("Enter the secret code (enter 7 to unlock): ");
        scanf("%d", &passwordAttempt);
    } while (passwordAttempt != 7);
    
    printf("Access Granted!\n");

    return 0;
}
