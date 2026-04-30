
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
        int uidx = 1 - i;
        //set_uart_ID(i);
        char ch;
        set_uart_ID(i);
        if (ub_rx->num_new_lines == 0) {
            io_putch('\n');
        }
        printf("TeraTerm-%d  >> ", i);
        set_uart_ID(uidx);
        io_putch('\r');
        printf("UART-%d echo << ", i);
        while (1) {
            set_uart_ID(i);
            ch = io_getch();
            gpio_data = (gpio_data << 8) | ch;
            gpio->data = gpio_data;
            if (ch == TERMINATE_CODE) {
                printf("\nCtrl-Z detected...\n");
                set_uart_ID(i);
                printf("\nCtrl-Z detected...\n");
                terminated = 1;
                break;
            }
            set_uart_ID(uidx);
            io_putch(ch);
            if (ch == '\n' || !ub_rx->not_empty) {
                if (ch == '\n') {
                    UARTBuf_consume_new_line(ub_rx);
                }
                else {
                    io_putch('\n');
                }
                printf("TeraTerm-%d  >> ", uidx);
                break;
            }
        }
    }
    set_uart_ID(0);
#endif
    return 0;
}

#if 0
//#define CRISS_CROSS_UART
void monitor_UART_RX() {
    for (int i = 0; i < 2; ++i) {
        set_uart_ID(i);
        printf("\nwaiting for key inputs from TeraTerm... (hit Ctrl-Z to abort)\n");
        printf("TX-%d >> ", i);
    }
    int wp[2];
    for (int i = 0; i < 2; ++i) {
        wp[i] = UART_BUF_RX[i].wp;
    }
    while (1) {
        for (int i = 0; i < 2; ++i) {
            UARTBuf* ub_rx = &UART_BUF_RX[i];
            if (ub_rx->stat < 0) {
                set_uart_ID(0); /// uart-0
                return; /// esc-code received : aborting
            }
            if (ub_rx->stat > 0 || ub_rx->full) {
#if defined(CRISS_CROSS_UART)
                int uidx = 1 - i;
#else
                int uidx = i;
#endif
                set_uart_ID(i);
                if (ub_rx->full) {
                    io_putch('\n');
                }
#if defined(CRISS_CROSS_UART)
                set_uart_ID(uidx);
                io_putch('\r');
#endif
                printf("RX-%d >> ", uidx);
                int newline = 0;
                while (ub_rx->not_empty) {
                    enter_critical_section();
                    char ch = UARTBuf_read(ub_rx);
                    leave_critical_section();
                    io_putch(ch);
                    if (ch == '\n') {
                        enter_critical_section();
                        ub_rx->stat--;
                        leave_critical_section();
                        newline = 1;
                        break;
                    }
                }
                if (!newline) {
                    printf("\n");
                }
                printf("TX-%d >> ", uidx);
#if defined(CRISS_CROSS_UART)
                set_uart_ID(i);
                printf("TX-%d >> ", i);
#endif
                //printf("TX-%d >> ", uidx);
            }
            if (wp[i] != ub_rx->wp) {
                *gpio = (i << 28) | (UARTBuf_read_last(ub_rx) << 16) | (UARTBuf_buffered_size(ub_rx) & 0xffff);
                wp[i] = ub_rx->wp;
            }
        }
    }
}

#endif