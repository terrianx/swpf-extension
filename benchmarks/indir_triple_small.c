#include <stdio.h>

int main() {
    int x = 0;
    int A[10] = {0, 9, 1, 8, 2, 7, 3, 6, 4, 5};
    int B[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    int C[10] = {9, 0, 8, 1, 7, 2, 6, 3, 5, 4};

    // triple indirection
    for (int i = 0; i < 10; ++i) {
        x = C[B[A[i]]];
    }
}
