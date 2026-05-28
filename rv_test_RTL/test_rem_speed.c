#include <stdio.h>

#define ITER 8
#define N    8

#define USE_HW_REM
volatile int sx[N] = {
    12345, -12345, 98765, -98765,
    2147483647, -2147483647, 1000003, -1000003
};

volatile int sy[N] = {
    7, 7, -13, -13,
    97, -97, 123, -123
};

volatile unsigned int ux[N] = {
    12345u, 0xffffffffu, 0x80000000u, 987654321u,
    1000003u, 0x12345678u, 0xabcdef01u, 4000000000u
};

volatile unsigned int uy[N] = {
    7u, 3u, 97u, 12345u,
    123u, 257u, 65535u, 99991u
};

#ifndef NATIVE_MODE
static inline unsigned int read_mcycle(void)
{
    unsigned int v;
    asm volatile ("csrr %0, mcycle" : "=r"(v));
    return v;
}
#endif
#if defined(USE_HW_REM) && !defined(NATIVE_MODE)
static inline int test_rem(int a, int b)
{
    int r;

    asm volatile (
        "mv x5, %1\n"
        "mv x6, %2\n"
        ".word 0x0262e3b3\n"   // rem x7, x5, x6
        "mv %0, x7\n"
        : "=r"(r)
        : "r"(a), "r"(b)
        : "x5", "x6", "x7"
    );

    return r;
}
static inline unsigned int test_remu(unsigned int a, unsigned int b)
{
    unsigned int r;

    asm volatile (
        "mv x5, %1\n"
        "mv x6, %2\n"
        ".word 0x0262f3b3\n"   // remu x7, x5, x6
        "mv %0, x7\n"
        : "=r"(r)
        : "r"(a), "r"(b)
        : "x5", "x6", "x7"
    );

    return r;
}
#else
static inline int test_rem(int a, int b)
{
    return a % b;
}
static inline unsigned int test_remu(unsigned int a, unsigned int b)
{
    return a % b;
}
#endif

int main(void)
{
    volatile int signed_sum = 0;
    volatile unsigned int unsigned_sum = 0;

#ifndef NATIVE_MODE
    unsigned int start_cycle = read_mcycle();
#endif

    for (int k = 0; k < ITER; k++) {
        for (int i = 0; i < N; i++) {
            signed_sum += test_rem(sx[i], sy[i]);
            unsigned_sum += test_remu(ux[i], uy[i]);
        }
    }

#ifndef NATIVE_MODE
    unsigned int end_cycle = read_mcycle();

#ifdef USE_HW_REM
    printf("HW REM cycles = %d\n", (int)(end_cycle - start_cycle));
#else
    printf("SW REM cycles = %d\n", (int)(end_cycle - start_cycle));
#endif

    printf("signed_sum = %d\n", signed_sum);
    printf("unsigned_sum = %x\n", unsigned_sum);
#else
    printf("signed_sum = %d\n", signed_sum);
    printf("unsigned_sum = %x\n", unsigned_sum);
#endif
    return 0;
}