#include <stdio.h>

#ifndef NATIVE_MODE
static inline unsigned int read_mcycle(void)
{
    unsigned int v;
    asm volatile ("csrr %0, mcycle" : "=r"(v));
    return v;
}
#endif

int main(void)
{
    int result = 0;

#ifdef NATIVE_MODE
    for (int i = 0; i < 1000; i++) {
        result++;
    }
    printf("br_loop cycles = native\n");
#else
    unsigned int start_cycle = read_mcycle();

    asm volatile (
        "li   x5, 1000\n"
        "li   x6, 0\n"
        "1:\n"
        "addi x6, x6, 1\n"
        "addi x5, x5, -1\n"
        "bne  x5, x0, 1b\n"
        "mv   %0, x6\n"
        : "=r"(result)
        :
        : "x5", "x6"
    );

    unsigned int end_cycle = read_mcycle();
    printf("result = %d\n",result);
    printf("br_loop cycles = %d\n", (int)(end_cycle - start_cycle));
#endif

    return 0;
}