
#include <stdio.h>
#include "io.h"

#define USE_MULT_INST

#if defined(USE_MULT_INST) && !defined(NATIVE_MODE)
#define MULT_INT(dst, rs1, rs2) asm volatile(".insn r 0x33, 0, 0x1, %0, %1, %2" : "=r"(dst) : "r"(rs1), "r"(rs2))    /// f3(0), f7(0), rd(x0), rs1(x0), rs2(x0)
#else
#define MULT_INT(dst, rs1, rs2) dst = (rs1) * (rs2)
#endif

int d[6] = { 2, 3, 5, 7, 11, 13 };
int main() {
#if !defined(NATIVE_MODE)
    gpio->data = 0;
#endif
    start_profiler();
    int sum = 0, i;
    for (i = 0; i < 6; i ++) {
        int a = d[i], b;
        WFI_BREAK;
        MULT_INT(b, a, a);
        sum += b;
        //sum += a * a;
    }
    end_profiler();
#if !defined(NATIVE_MODE)
    gpio->data = sum;
#endif
    printf("sum = %d (0x%x)\n", sum, sum);
//    end_profiler();
    return 0;
}

