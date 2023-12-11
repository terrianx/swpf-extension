#include <stdio.h>

int main() {
    int SIZE = 100000;
    int A[100000];
    int B[100000];
    int out[100000];

    // populate A
    for (int i = 0; i < SIZE; ++i) {
        A[i] = i;
    }
    // A = {999999, 1, 999997, 3, 999995, 5, ...}
    for (int i = 0; i < SIZE/2; ++i) {
        A[i*2] = (SIZE-1)-i*2;
    }

    // populate B
    for (int i = 0; i < SIZE; ++i) {
        B[i] = i;
    }

    // double indirection
    for (int i = 0; i < SIZE; ++i) {
        out[i] = B[A[i]];
    }

    // print for correctness
    for (int i = 0; i < SIZE; ++i) {
        printf("%d\n", out[i]);
    }
}
