
#include <stdio.h>
#include "io.h"

int d[6] = { 2, 3, 5, 7, 11, 13 };
int main() {
#if !defined(NATIVE_MODE)
    gpio->data = 0;
#endif
    start_profiler();
    int sum = 0, i;
    for (i = 0; i < 6; i ++) {
        int a = d[i];
        WFI_BREAK;
        sum += a * a;
    }
    //end_profiler();
#if !defined(NATIVE_MODE)
    gpio->data = sum;
#endif
    printf("sum = %d (0x%x)\n", sum, sum);
    end_profiler();
    return 0;
}

