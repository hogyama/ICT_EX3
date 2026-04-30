
#include <stdio.h>
#include "io.h"

int main() {
#if defined(NATIVE_MODE)
    printf("do nothing...\n");
#else
    enable_UART_interrupt();
    gpio->data = 0;
    int terminated = 0;
    unsigned gpio_data = 0;
    int i;
    for (i = 0; i < 2; ++i) {
        set_uart_ID(i);
        printf("\nTeraTerm-%d  >> ", i);
    }
    while (!terminated) {
        UARTBuf* ub_rx;
        for (i = 0; i < 2; ++i) {
            ub_rx = &UART_BUF_RX[i];
            if (ub_rx->num_new_lines || ub_rx->terminate_code || ub_rx->full) {
                break; /// UART[i] RX is not empty
            }
        }
        if (i == 2) { continue; }
        set_uart_ID(i);
        char ch;
        if (ub_rx->num_new_lines == 0) {
            io_putch('\n');
        }
        printf("UART-%d echo << ", i);
        while (1) {
            ch = io_getch();
            gpio_data = (gpio_data << 8) | ch;
            gpio->data = gpio_data;
            if (ch == TERMINATE_CODE) {
                printf("\nCtrl-Z detected...\n");
                set_uart_ID(1 - i);
                printf("\nCtrl-Z detected...\n");
                terminated = 1;
                break;
            }
            io_putch(ch);
            if (ch == '\n' || !ub_rx->not_empty) {
                if (ch == '\n') {
                    UARTBuf_consume_new_line(ub_rx);
                }
                else {
                    io_putch('\n');
                }
                printf("TeraTerm-%d  >> ", i);
                break;
            }
        }
    }
    set_uart_ID(0);
#endif
    return 0;
}

