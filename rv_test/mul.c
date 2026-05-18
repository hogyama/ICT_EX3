#include <stdint.h>
#include "io.h"
volatile int64_t  sa = -1234567;
volatile int64_t  sb = 123;

volatile uint32_t ua = 4000000000u;
volatile uint32_t ub = 12345u;

volatile int32_t  q_m;
volatile int32_t  q_s;
volatile int32_t  r_s;
volatile uint32_t q_u;
volatile uint32_t r_u;

int main(void)
{
    q_m = sa * sb;
    q_s = sa / sb;   // div
    r_s = sa % sb;   // rem

    q_u = ua / ub;   // divu
    r_u = ua % ub;   // remu

    return 0;
}