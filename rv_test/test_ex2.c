#include <stdlib.h>
#include "io.h"
#include <stdio.h>
#define MAX_INPUT_SIZE 100
int get_number();
char get_char();
int main(){
    printf("Enter a number: ");
    int num1 = get_number();
    while (1) {
        printf("Enter operator: ");
        char op = get_char();
        if(op == '='){
            printf("Result: %d\n", num1);
            #if !defined(NATIVE_MODE)
                gpio->data= num1;
            #endif
            break;
        }
        if(op != '+' && op != '-' && op != '*' && op != '/' && op != '%'){
            printf("Error: invalid operator\n");
        }
        printf("Enter a number: ");
        int num2 = get_number();
        switch(op){
            case '+':
                num1 = num1 + num2;
                break;
            case '-':
                num1 = num1 - num2;
                break;
            case '*':
                num1 = num1 * num2;
                break;
            case '/':
                if(num2 == 0){
                    printf("Error: division by zero\n");
                    continue;
                }
                num1 = num1 / num2;
                break;
            case '%':
                if(num2 == 0){
                    printf("Error: division by zero\n");
                    continue;
                }
                num1 = num1 % num2;
                break;
        }
    }
    return 0;
}
// 何も入力しなかったら0を返す
// 数字以外が入力されたらエラーを表示して再度入力を促す
int get_number(){
    int i = 0;
    char c[MAX_INPUT_SIZE];
    char ch;
    while(1){
        if(i >= MAX_INPUT_SIZE - 1){
            break;
        }
        #if !defined(NATIVE_MODE)
            ch = io_getch();
        #else
            ch = getchar();
        #endif
        if(ch == '\n'){
            break;
        }
        if(ch == '\r'){
            continue;
        }
        if(ch >= '0' && ch <= '9'){
            c[i++] = ch;
            continue;
        }
        printf("Error: invalid input\n");
        continue;
    }
    c[i] = '\0';
    if(i > 0){
        int answer = 0;
        for(int j = 0; j < i; j++){
            if(c[j] >= '0' && c[j] <= '9'){
                answer = answer * 10 + (c[j] - '0');
            }
        }
        return answer;
    }
    return 0;
}
// 改行と復帰以外の文字が入力されるまで待ち、入力された文字を返す
char get_char(){
    char c;
    int is_valid = 0;
    while(1){
        #if !defined(NATIVE_MODE)
            c = io_getch();
        #else
            c = getchar();
        #endif
        if(c != '\n' && c != '\r'){
            is_valid = 1;
        }
        if(is_valid && c == '\n'){
            break;
        }
        if(is_valid && c == '\r'){
            continue;
        }
        continue;
    }
    return c;
}
