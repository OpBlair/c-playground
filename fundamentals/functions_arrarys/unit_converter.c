#include <stdio.h>

#define MAX_HISTORY 20

typedef struct {
    double input_val;
    double output_val;
    char label[32];
} ConversionRecord;

/* Function prototypes */
void print_menu(void);
double km_to_miles(double km);
double miles_to_km(double miles);
double c_to_f(double celsius);
double f_to_c(double fahrenheit);
double kg_to_lbs(double kg);
double lbs_to_kg(double lbs);
void print_history(const ConversionRecord history[], int count);

int main(void) {
    ConversionRecord history[MAX_HISTORY];
    int history_count = 0;
    int choice;
    double value, result;

    printf("=== C Playground: Unit Converter ===\n");

    while (1) {
        print_menu();
        printf("Choose an option: ");

        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n");
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {}
            continue;
        }

        if (choice == 0) {
            printf("Exiting Unit Converter. Keep building!\n");
            break;
        }

        if (choice == 7) {
            print_history(history, history_count);
            continue;
        }

        if (choice < 1 || choice > 6) {
            printf("Please pick a valid option (0-7).\n");
            continue;
        }

        printf("Enter value to convert: ");
        if (scanf("%lf", &value) != 1) {
            printf("Invalid numeric value.\n");
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {}
            continue;
        }

        const char *label = "";
        switch (choice) {
            case 1:
                result = km_to_miles(value);
                label = "km -> mi";
                break;
            case 2:
                result = miles_to_km(value);
                label = "mi -> km";
                break;
            case 3:
                result = c_to_f(value);
                label = "C -> F";
                break;
            case 4:
                result = f_to_c(value);
                label = "F -> C";
                break;
            case 5:
                result = kg_to_lbs(value);
                label = "kg -> lbs";
                break;
            case 6:
                result = lbs_to_kg(value);
                label = "lbs -> kg";
                break;
            default:
                result = 0.0;
                break;
        }

        printf("Result: %.4f (%s)\n", result, label);

        if (history_count < MAX_HISTORY) {
            history[history_count].input_val = value;
            history[history_count].output_val = result;
            snprintf(history[history_count].label, sizeof(history[history_count].label), "%s", label);
            history_count++;
        } else {
            printf("History full (max %d entries kept).\n", MAX_HISTORY);
        }
    }

    return 0;
}

void print_menu(void) {
    printf("\n--- Conversions ---\n");
    printf("1. Kilometers to Miles\n");
    printf("2. Miles to Kilometers\n");
    printf("3. Celsius to Fahrenheit\n");
    printf("4. Fahrenheit to Celsius\n");
    printf("5. Kilograms to Pounds\n");
    printf("6. Pounds to Kilograms\n");
    printf("7. View Conversion History\n");
    printf("0. Exit\n");
}

double km_to_miles(double km) {
    return km * 0.621371;
}

double miles_to_km(double miles) {
    return miles / 0.621371;
}

double c_to_f(double celsius) {
    return (celsius * 9.0 / 5.0) + 32.0;
}

double f_to_c(double fahrenheit) {
    return (fahrenheit - 32.0) * 5.0 / 9.0;
}

double kg_to_lbs(double kg) {
    return kg * 2.20462;
}

double lbs_to_kg(double lbs) {
    return lbs / 2.20462;
}

void print_history(const ConversionRecord history[], int count) {
    if (count == 0) {
        printf("No conversions recorded yet.\n");
        return;
    }

    printf("\n--- Recent Conversions (%d) ---\n", count);
    for (int i = 0; i < count; i++) {
        printf("%d. %.4f -> %.4f [%s]\n",
               i + 1,
               history[i].input_val,
               history[i].output_val,
               history[i].label);
    }
}
