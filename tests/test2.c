#include <stdio.h>

int foo(int a) {
    int i, sum = 0;
    int b = 42; // Loop invariant

    for (i = 0; i < 10; i++) {
        sum += i;
        sum += a + b; // a + b is loop-invariant
    }
    return sum;
}

int main() {
    int result = foo(5);
    printf("Result: %d\n", result);
    return 0;
}