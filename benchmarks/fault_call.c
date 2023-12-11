#include <stdio.h>

int foo(int arr[], int size) {
    arr[size-1] = size;
}

int main() {
    int x = 0;
    int A[10] = {0, 9, 1, 8, 2, 7, 3, 6, 4, 5};
    int B[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    // double indirection
    for (int i = 0; i < 10; ++i) {
        x = B[A[i]];
        foo(A, 10); // function call
    }
}
