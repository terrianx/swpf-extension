#include <stdio.h>

int main() {
    int SIZE = 600000;
    int x = 0;
    int A[600000];
    int B[600000];
    int C[600000];

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

    // populate C
    for (int i = 0; i < SIZE; ++i) {
        C[i] = i;
    }
    // C = {0, 999999, 2, 999997, 4, 999995, ...}
    for (int i = 0; i < SIZE/2; ++i) {
        C[i*2 + 1] = (SIZE-1)-i*2;
    }

    // triple indirection
    for (int i = 0; i < SIZE/2; ++i) {
        x = C[B[A[i]]];
    }
}
