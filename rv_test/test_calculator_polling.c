#include "io.h"
#include <stdio.h>
#include <stdbool.h>
#if defined(NATIVE_MODE)
    #define _io_putch putchar
    #define _io_getch getchar
#endif
#define MAX_7SEG_SIZE 8
typedef enum State{
    STATE_INPUT= 0,         // 数字入力待ち
    STATE_OPERATOR_ENTERED, // 演算子が直前に入力された状態
    STATE_RESULT_DISPLAYED, // = が押され、結果が表示された状態
    STATE_ERROR             // エラー状態。クリアされるまでこの状態を維持する
}State_t;
typedef struct Calculator_Status{
    int current_value; // 演算子の右側にあるまだ確定していない数値
    int saved_value;   // 確定した演算結果
    char current_operator; // 現在入力されている演算子
    bool is_first_entered; // 最初の入力が行われたかどうか
    State_t state;
}status_t;
// 有効なキーが入力されるまで待つ
char get_valid_key();
// 計算を進める
void process_calc(char key);
// ディスプレイを更新
void update_display();
// 数値を7セグメント表示用のコードに変換
int calc_digit(int num, int* array);
status_t calc_status = {0, 0, 0, false, STATE_INPUT};
int main(){
    start_profiler();
    update_display();
    while(1){
        process_calc(get_valid_key());
        update_display();
    }
    end_profiler();
    return 0;
}
char get_valid_key(void){
    
    while(1){
        char key = _io_getch();
        // クリアはCかc
        if((key >= '0' && key <= '9') || key == '+' || key == '-' || key == '*' || key == '/' || key == '%' || key == '=')
        {
            return key;
        }else if(key == 'C' || key == 'c'){
            _io_putch('\r');
            _io_putch('\n');
            return key;
        }
    }
}
void process_calc(char key){
    switch(calc_status.state){
        case STATE_INPUT:
            if(key >= '0' && key <= '9'){
                int num = key - '0'; 
                if(!calc_status.is_first_entered) //初回
                {
                    calc_status.current_value = num;
                    calc_status.is_first_entered = true;
                }else{
                    calc_status.current_value = calc_status.current_value * 10 + num;
                }
            }else if(key == '+' || key == '-' || key == '*' || key == '/' || key == '%' || key == '='){
                if(calc_status.current_operator == 0){ //初回
                    calc_status.saved_value = calc_status.current_value;
                }
                switch(calc_status.current_operator){
                    case '+':
                        calc_status.saved_value += calc_status.current_value;
                        break;
                    case '-':
                        calc_status.saved_value -= calc_status.current_value;
                        break;
                    case '*':
                        calc_status.saved_value = calc_status.saved_value * calc_status.current_value;
                        break;
                    case '/':
                        if (calc_status.current_value == 0) { calc_status.state = STATE_ERROR; return; }
                        calc_status.saved_value = calc_status.saved_value / calc_status.current_value;
                        break;
                    case '%':
                        if (calc_status.current_value == 0) { calc_status.state = STATE_ERROR; return; }
                        calc_status.saved_value = calc_status.saved_value % calc_status.current_value;
                        break;
                }
                calc_status.current_value = 0;
                calc_status.current_operator = key;
                if(key == '='){
                    calc_status.state = STATE_RESULT_DISPLAYED;
                }else{
                    calc_status.state = STATE_OPERATOR_ENTERED;
                }
            }else{
                char error_msg[] = "Error: invalid input. Please enter C or c to clear.\n";
                for(int i = 0; error_msg[i] != '\0'; i++){
                    _io_putch(error_msg[i]);
                }
                calc_status.state = STATE_ERROR;
            }
            break;
        case STATE_OPERATOR_ENTERED:
            if(key >= '0' && key <= '9'){
                int num = key - '0';
                calc_status.current_value = num;
                calc_status.state = STATE_INPUT;
            }else{
                char error_msg[] = "Error: invalid input. Please enter C or c to clear.\n";
                for(int i = 0; error_msg[i] != '\0'; i++){
                    _io_putch(error_msg[i]);
                }
                calc_status.state = STATE_ERROR;
            }
            break;
        case STATE_RESULT_DISPLAYED:
            if(key == 'c'|| key == 'C'){
                calc_status = (status_t){0, 0, 0, false, STATE_INPUT};
            }else{
                char error_msg[] = "Error: invalid input. Please enter C or c to clear.\n";
                for(int i = 0; error_msg[i] != '\0'; i++){
                    _io_putch(error_msg[i]);
                }
                calc_status.state = STATE_ERROR;
            }
            break;
        case STATE_ERROR:
            if(key == 'c' || key == 'C')
            {
               calc_status = (status_t){0, 0, 0, false, STATE_INPUT};
            }
            break;
    }
}
#if !defined(NATIVE_MODE)
void update_display(){
    if(calc_status.state == STATE_ERROR){
        gpio->dout7SEG[1] = (
            S7_e
        );
        gpio->dout7SEG[0] = (
            S7_r << 24 | S7_r << 16 | S7_o << 8 | S7_r << 0
        ); 
    }else{
        int display_value;
        if(calc_status.state == STATE_INPUT) //　入力中は打っている文字を表示
        {
            display_value = calc_status.current_value;
        }else{ //　確定した結果を表示
            display_value = calc_status.saved_value;
        }
        //8個の7セグ配列
        int num[MAX_7SEG_SIZE] = {0};
        int digit = calc_digit(display_value,num);
        bool is_positive = (display_value >= 0);
        if (!is_positive && digit < MAX_7SEG_SIZE) {
            num[digit] = S7_minus; 
            digit++;
        }
        // overflow
        if((digit > 8 && is_positive) || (digit > 7 && !is_positive)){
            gpio->dout7SEG[1] = (
                S7_o << 24 | S7_v << 16 | S7_e << 8 | S7_r << 0
            );
            gpio->dout7SEG[0] = (
                S7_f << 24 | S7_l << 16 | S7_o << 8 | S7_w << 0 
            );
        }else{
            gpio->dout7SEG[1] = (
                num[7] << 24 | num[6] << 16 | num[5] << 8 | num[4] << 0
            );
            gpio->dout7SEG[0] = (
                num[3] << 24 | num[2] << 16 | num[1] << 8 | num[0] << 0 
            );
        }
    }
}
#else
void update_display(){
    if(calc_status.state == STATE_ERROR){
        printf("\r\n[7SEG DISPLAY: ERROR ]\r\n");
    } else {
        int display_value;
        if(calc_status.state == STATE_INPUT) display_value = calc_status.current_value;
        else display_value = calc_status.saved_value;
        
        printf("\r\n[7SEG DISPLAY: %d ]\r\n", display_value);
    }
}
#endif

#if !defined(NATIVE_MODE)
int calc_digit(int num, int* array){
    int digit = 0;
    if (num < 0) {
        num = -num;
    }
    if (num == 0) {
        array[0] = S7_0;
        return 1; 
    }
    while (num > 0) {
        switch(num%10){
            case 0:
                array[digit] = S7_0;
                break;
            case 1:
                array[digit] = S7_1;
                break;
            case 2: 
                array[digit] = S7_2;
                break;
            case 3:
                array[digit] = S7_3;
                break;
            case 4:
                array[digit] = S7_4;
                break;
            case 5:
                array[digit] = S7_5;
                break;
            case 6:
                array[digit] = S7_6;
                break;
            case 7:
                array[digit] = S7_7;
                break;
            case 8:
                array[digit] = S7_8;
                break;
            case 9:
                array[digit] = S7_9;
                break;
        }
        num = num / 10;         
        digit++;                 
    }
    return digit; 
}
#endif