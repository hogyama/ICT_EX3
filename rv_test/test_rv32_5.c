
#include <stdio.h>
#include "io.h"

#if !defined(NATIVE_MODE)
unsigned counter = 0;
void timer_handler() {
    counter ++;
    gpio->data = counter;
}
#endif

int main() {
#if defined(NATIVE_MODE)
    printf("do nothing...\n");
#else
    set_mtimer_interval(10/*msec*/);
    timer_interrupt_hook = timer_handler;
    enable_timer_interrupt();
    int timer_on = 1;
    printf("timer on\n");
    while (1) {
        if (io_getch()) {
            timer_on = !timer_on;   /// toggle timer_on
            if (timer_on) {
                asm volatile("csrs mie, %[new]" : : [new] "r" (0x00000080));	///	set meie[7] : machine-external-interrupt-enable
                printf("timer on\n");
            }
            else {
                asm volatile("csrc mie, %[new]" : : [new] "r" (0x00000080));	///	clear meie[7] : machine-external-interrupt-enable
                printf("timer off\n");
            }
        }
    }
#endif
    return 0;
}

