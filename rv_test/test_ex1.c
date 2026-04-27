#include <stdio.h>
#include "io.h"
#define MAX_INPUT 100
int read_decimal(char buf[], int max_size);
int conv_string_to_int_decimal(char buf[]);

int main(void)
{
    int finished = 0;
#if !defined(NATIVE_MODE)
    gpio->data = 0;
#endif
    while(!finished) {
        char input[MAX_INPUT];
        int len = read_decimal(input, MAX_INPUT);
        if (len < 0) {
            finished = 1;
            continue;
        }
        if (len == 0) {
            continue;
        }
#if !defined(NATIVE_MODE)
        start_profiler();
#endif
        int answer = conv_string_to_int_decimal(input);
#if !defined(NATIVE_MODE)
        gpio->data = answer;
        end_profiler();
#endif
        printf("input = %s, answer = %d, hex = %x\n", input, answer, answer);
    }
    return 0;
}

int read_decimal(char buf[], int max_size)
{
    int i = 0;
    while (1) {
#if !defined(NATIVE_MODE)
        char ch = _io_getch();
#else
        char ch = getchar();
#endif
        if (ch == 'f') {
            return -1;
        }
        if (ch == '\n') {
            break;
        }
        if (ch == '\r') {
            continue;
        }
        if (i >= max_size - 1) {
            break;
        }
        if (ch >= '0' && ch <= '9') {
            buf[i++] = ch;
        }
    }
    buf[i] = '\0';
    return i;
}

int conv_string_to_int_decimal(char buf[])
{
    int value = 0;
    for (int i = 0; buf[i] != '\0'; i++) {
        int digit = buf[i] - '0';
        value = (value << 3) + (value << 1) + digit;
    }
    return value;
}