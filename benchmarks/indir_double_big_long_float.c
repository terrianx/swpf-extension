#include <stdio.h>

int main() {
    int x = 0;
    int A[1000000];
    int B[1000000];

    // populate A
    for (int i = 0; i < 1000000; ++i) {
        A[i] = i;
    }
    // A = {999999, 1, 999997, 3, 999995, 5, ...}
    for (int i = 0; i < 500000; ++i) {
        A[i*2] = 999999-i*2;
    }

    // populate B
    for (int i = 0; i < 1000000; ++i) {
        B[i] = i;
    }

    for (int y = 0; y < 10000; ++y) {
        // double indirection
        for (int i = 0; i < 1000000; ++i) {
            x += B[A[i]];

            // some overlapping work to exploit prefetching
            float y = (float) x;
            float z = y * x;
            y = z / x;
            z *= y;
        }
    }
}
