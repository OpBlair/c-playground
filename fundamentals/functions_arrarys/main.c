#include <stdio.h>
#include <stddef.h>

#define CAPACITY 10

/* Function declarations (prototypes) */
void print_array(const int arr[], size_t length);
int sum_array(const int arr[], size_t length);
double average_array(const int arr[], size_t length);
int find_max(const int arr[], size_t length);
void double_elements(int arr[], size_t length);
void reverse_array(int arr[], size_t length);

int main(void) {
    /* Array initialization: remaining slots default to 0 */
    int scores[CAPACITY] = {12, 45, 7, 89, 34, 21};
    size_t active_len = 6;

    printf("Initial array:\n");
    print_array(scores, active_len);

    /* Read-only functions (const int arr[]) don't modify the caller's array */
    int total = sum_array(scores, active_len);
    double avg = average_array(scores, active_len);
    int max_val = find_max(scores, active_len);

    printf("\nSum: %d\n", total);
    printf("Average: %.2f\n", avg);
    printf("Max: %d\n", max_val);

    /* Mutating functions modify the original array in place */
    double_elements(scores, active_len);
    printf("\nAfter doubling elements:\n");
    print_array(scores, active_len);

    reverse_array(scores, active_len);
    printf("\nAfter reversing in place:\n");
    print_array(scores, active_len);

    return 0;
}

void print_array(const int arr[], size_t length) {
    printf("[");
    for (size_t i = 0; i < length; i++) {
        printf("%d", arr[i]);
        if (i + 1 < length) {
            printf(", ");
        }
    }
    printf("]\n");
}

int sum_array(const int arr[], size_t length) {
    int sum = 0;
    for (size_t i = 0; i < length; i++) {
        sum += arr[i];
    }
    return sum;
}

double average_array(const int arr[], size_t length) {
    if (length == 0) {
        return 0.0;
    }
    /* Cast to double so integer division doesn't truncate the average */
    return (double)sum_array(arr, length) / (double)length;
}

int find_max(const int arr[], size_t length) {
    if (length == 0) {
        return 0;
    }
    int best = arr[0];
    for (size_t i = 1; i < length; i++) {
        if (arr[i] > best) {
            best = arr[i];
        }
    }
    return best;
}

void double_elements(int arr[], size_t length) {
    for (size_t i = 0; i < length; i++) {
        arr[i] *= 2;
    }
}

void reverse_array(int arr[], size_t length) {
    if (length < 2) {
        return;
    }
    size_t left = 0;
    size_t right = length - 1;
    while (left < right) {
        int temp = arr[left];
        arr[left] = arr[right];
        arr[right] = temp;
        left++;
        right--;
    }
}
