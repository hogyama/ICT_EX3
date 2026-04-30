
#include <stdio.h>
#include "io.h"

#define BUF_SIZE    40
char rx_buf[BUF_SIZE];
int main() {
#if defined(NATIVE_MODE)
    printf("do nothing...\n");
#else
    gpio->data = 0;
    int terminated = 0;
    unsigned gpio_data = 0;
    int i;
    for (i = 0; i < 2; ++i) {
        set_uart_ID(i);
        printf("\nTeraTerm-%d  >> ", i);
    }
    while (!terminated) {
        for (i = 0; i < 2; ++i) {
            if (!UART_RX_EMPTY(_uart[i])) {
                break; /// UART[i] RX is not empty
            }
        }
        if (i == 2) { continue; }
        set_uart_ID(i);
        int rx_pos = 0;
        while (1) {
            char ch = io_getch();
            gpio_data = (gpio_data << 8) | ch;
            gpio->data = gpio_data;
            if (ch == TERMINATE_CODE) {
                rx_buf[rx_pos] = 0;
                printf("%s\nCtrl-Z detected...\n", rx_buf);
                set_uart_ID(1 - i);
                printf("\nCtrl-Z detected...\n");
                terminated = 1;
                break;
            }
            rx_buf[rx_pos++] = ch;
            if (ch == '\n' || rx_pos == BUF_SIZE - 1) {
                rx_buf[rx_pos] = 0;
                if (ch != '\n') { io_putch('\n'); }
                printf("UART-%d echo << ", i);
                printf("%s", rx_buf);
                if (ch != '\n') { io_putch('\n'); }
                printf("TeraTerm-%d  >> ", i);
                rx_pos = 0;
                break;
            }
        }
    }
    set_uart_ID(0);
#endif
    return 0;
}

