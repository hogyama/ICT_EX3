#include <stdio.h>

#ifndef NATIVE_MODE

static inline int do_rem(int a, int b)
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

static inline unsigned int do_remu(unsigned int a, unsigned int b)
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

static int do_rem(int a, int b)
{
    if (b == 0) return a;
    if (a == (int)0x80000000 && b == -1) return 0;
    return a % b;
}

static unsigned int do_remu(unsigned int a, unsigned int b)
{
    if (b == 0) return a;
    return a % b;
}

#endif

static int check_rem(int a, int b, int expected)
{
    int got = do_rem(a, b);

    if (got != expected) {
        printf("REM NG: a=%d b=%d got=%d exp=%d\n", a, b, got, expected);
        return 1;
    }

    return 0;
}

static int check_remu(unsigned int a, unsigned int b, unsigned int expected)
{
    unsigned int got = do_remu(a, b);

    if (got != expected) {
        printf("REMU NG: a=%x b=%x got=%x exp=%x\n", a, b, got, expected);
        return 1;
    }

    return 0;
}

int main(void)
{
    int err = 0;

    // signed REM
    err += check_rem( 7,  3,  1);
    err += check_rem(-7,  3, -1);
    err += check_rem( 7, -3,  1);
    err += check_rem(-7, -3, -1);

    // RISC-V spec special cases
    err += check_rem(123, 0, 123);
    err += check_rem(-123, 0, -123);
    err += check_rem((int)0x80000000, -1, 0);

    // unsigned REMU
    err += check_remu(7u, 3u, 1u);
    err += check_remu(0xffffffffu, 2u, 1u);
    err += check_remu(0x80000000u, 3u, 2u);
    err += check_remu(0x12345678u, 0u, 0x12345678u);

    if (err == 0) {
        printf("REM TEST PASS\n");
    } else {
        printf("REM TEST FAIL err=%d\n", err);
    }
    return err;
}