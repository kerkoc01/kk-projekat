#include <stdio.h>

void example(int *arr, int n) {
    int i;
    int x = 5;
    for (i = 0; i < n; i++) {
        arr[i] = x; // Store instruction with a value defined outside the loop
    }
}

int main() {
    int arr[10];
    example(arr, 10);
    for (int i = 0; i < 10; i++) {
        printf("%d ", arr[i]);
    }
    return 0;
}
