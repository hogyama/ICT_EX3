#include <stdio.h>
#include <stdlib.h>
#include "io.h"

#define MAX_INPUT_SIZE 100

int main(){
#if !defined(NATIVE_MODE)
    start_profiler();
    gpio->data = 0;
#endif

    int terminated = 0;

    while(!terminated)
    {
        int i = 0;
        char c[MAX_INPUT_SIZE];
        int answer = 0;
        while(1) {
#if !defined(NATIVE_MODE)
            char ch = io_getch();
#else
            char ch = getchar();
#endif                
            if(i >= MAX_INPUT_SIZE - 1){
                break;
            }
            if(ch == 'f'){
                terminated = 1;
                break;
            }
            if(ch == '\n'){
                break;
            }
            if(ch == '\r'){
                continue;
            }
            if(ch >= '0' && ch <= '9'){
                c[i++] = ch;
            }
        }
        c[i] = '\0';
        if(i > 0) {
            for(int j = 0; j < i; j++){
                if(c[j] >= '0' && c[j] <= '9'){
                    answer = answer * 10 + (c[j] - '0');
                }
            }
#if !defined(NATIVE_MODE)
            gpio->data = answer;
#endif
            printf("answer = %d (0x%x)\n", answer, answer);
        }
    }
#if !defined(NATIVE_MODE)
    end_profiler();
#endif

    return 0;
}