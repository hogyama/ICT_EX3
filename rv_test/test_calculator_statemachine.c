#include "io.h"
#include <stdio.h>
#include <stdbool.h>
#define MAX_7SEG_SIZE 8
#if defined(NATIVE_MODE)
    #define _io_putch putchar
    #define _io_getch getchar
#endif
typedef enum state{
    STATE_WAIT_REG2,  // 1番目のレジスタへの入力が終わり、次のレジスタへの入力を待っている状態
    STATE_INPUT_REG2, // 2番目のレジスタへの入力状態
    STATE_RESULT,     // =が押され、結果が表示されている状態
    STATE_ERROR     // エラー状態。クリアされるまでこの状態を維持する
}state_t;
typedef struct Calculator_Status{
    int reg1; 
    int reg1_dp; // 小数点以下の桁数を保持 -1は小数点がまだ入力されていないことを示す
    int reg2; 
    int reg2_dp; // 小数点以下の桁数を保持 -1は小数点がまだ入力されていないことを示す
    char operator;
    state_t state;
}cal_status_t;
typedef enum Seg7_Pattern seg7_t;
static seg7_t display[MAX_7SEG_SIZE] = {S7_space, S7_space, S7_space, S7_space, S7_space, S7_space, S7_space, S7_space};
static cal_status_t calc_status = {0, -1, 0, -1, '+', STATE_WAIT_REG2}; // 最初は0+reg2の状態とみなす
/**
 * 有効なキーが入力されるまで待つ
 */
char get_valid_key();
/**
 * 計算を進める
 */
void process_calc(char key);
/**
 * ディスプレイを更新
 */
void update_display();
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
        if((key >= '0' && key <= '9') || key == '+' || key == '-' || key == '*' || key == '/' || key == '%' || key == '=' || key == '.')
        {
            return key;
        }
        else if(key == 'C' || key == 'c')
        {
            _io_putch('\r');
            _io_putch('\n');
            return key;
        }
    }
}
void process_calc(char key){
    switch(calc_status.state)
    {
        case STATE_WAIT_REG2:
            if(key >= '0' && key <= '9')
            {
                int num = key - '0';
                calc_status.reg2 = num;
                calc_status.state = STATE_INPUT_REG2;
            }else if(key == '+' || key == '-' || key == '*' || key == '/' || key == '%')
            {
                calc_status.operator = key;
            }
            else
            {
                char error_msg[] = "Error: invalid input. Please enter C or c to clear.\n";
                for(int i = 0; error_msg[i] != '\0'; i++){
                    _io_putch(error_msg[i]);
                }
                calc_status.state = STATE_ERROR;
            }
            break;
        case STATE_INPUT_REG2:
            if(key >= '0' && key <= '9')
            {
                int num = key - '0';
                if(calc_status.reg2_dp  >= 0) 
                {
                    calc_status.reg2_dp++;
                }
                calc_status.reg2 = calc_status.reg2 * 10 + num;
            }
            else if(key == '.')
            {
                if(calc_status.reg2_dp == -1){
                    calc_status.reg2_dp = 0;
                }
                else{
                    char error_msg[] = "Error: invalid input. Please enter C or c to clear.\n";
                    for(int i = 0; error_msg[i] != '\0'; i++){
                        _io_putch(error_msg[i]);
                    }
                    calc_status.state = STATE_ERROR;
                }
            }
            else if(key == '+' || key == '-' || key == '*' || key == '/' || key == '%' || key == '=')
            {
                if(calc_status.reg2_dp == 0){
                    char error_msg[] = "Error: invalid input. Please enter C or c to clear.\n";
                    for(int i = 0; error_msg[i] != '\0'; i++){
                        _io_putch(error_msg[i]);
                    }
                    calc_status.state = STATE_ERROR;
                    return;
                }
                
                int r1_dp = (calc_status.reg1_dp == -1) ? 0 : calc_status.reg1_dp;
                int r2_dp = (calc_status.reg2_dp == -1) ? 0 : calc_status.reg2_dp;
                // 小数点の位置を合わせる
                while(r1_dp < r2_dp) {
                    calc_status.reg1 *= 10;
                    r1_dp++;
                }
                while(r2_dp < r1_dp) {
                    calc_status.reg2 *= 10;
                    r2_dp++;
                }
                calc_status.reg1_dp = r1_dp;
                calc_status.reg2_dp = r2_dp;
                switch(calc_status.operator)
                {
                    case '+':
                        calc_status.reg1 = calc_status.reg1 + calc_status.reg2;
                        break;
                    case '-':
                        calc_status.reg1 = calc_status.reg1 - calc_status.reg2;
                        break;
                    case '*':
                        calc_status.reg1 = calc_status.reg1 * calc_status.reg2;
                        calc_status.reg1_dp = calc_status.reg1_dp + calc_status.reg2_dp; 
                        break;
                    case '/':
                    // ゼロ除算のチェック
                        if (calc_status.reg2 == 0) { 
                            calc_status.state = STATE_ERROR;
                            return; 
                        }else{
                                calc_status.reg1 = calc_status.reg1 * 1000 / calc_status.reg2;
                                calc_status.reg1_dp = 3; 
                        }
                        break;
                    case '%':
                        if (calc_status.reg2 == 0) { 
                            calc_status.state = STATE_ERROR; 
                            return;
                        }else{
                            if(calc_status.reg1_dp != 0){
                                char error_msg[] = "Error: modulus operator does not support decimal numbers. Please enter C or c to clear.\n";
                                for(int i = 0; error_msg[i] != '\0'; i++){
                                    _io_putch(error_msg[i]);
                                }
                                calc_status.state = STATE_ERROR;
                                return;
                            }
                            calc_status.reg1 = calc_status.reg1 % calc_status.reg2;
                        }
                        break;
                }
                if(key == '='){
                    calc_status.state = STATE_RESULT;
                }else{
                    calc_status.operator = key;
                    calc_status.state = STATE_WAIT_REG2;
                }
            }
            else
            {
                char error_msg[] = "Error: invalid input. Please enter C or c to clear.\n";
                for(int i = 0; error_msg[i] != '\0'; i++){
                    _io_putch(error_msg[i]);
                }
                calc_status.state = STATE_ERROR;
            }
            break;
        case STATE_RESULT:
            if(key == 'c' || key == 'C')
            {
               calc_status = (cal_status_t){0, -1, 0, -1, '+', STATE_WAIT_REG2};
            }else if(key >= '0' && key <= '9')
            {
                calc_status = (cal_status_t){0, -1, 0, -1, '+', STATE_WAIT_REG2};
                process_calc(key);
            }else if(key == '+' || key == '-' || key == '*' || key == '/' || key == '%' )
            {
                calc_status.reg2 = 0;
                calc_status.reg2_dp = -1;
                calc_status.operator = key;
                calc_status.state = STATE_WAIT_REG2;
            }else
            {
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
               calc_status = (cal_status_t){0, -1, 0, -1, '+', STATE_WAIT_REG2};
            }else
            {
                char error_msg[] = "Error: invalid input. Please enter C or c to clear.\n";
                for(int i = 0; error_msg[i] != '\0'; i++){
                        _io_putch(error_msg[i]);
                }
                calc_status.state = STATE_ERROR;
            }
            break;
    }return;
}
#if !defined(NATIVE_MODE)
void update_display(){
    if(calc_status.state == STATE_ERROR){
        printf("\r\n[7SEG DISPLAY: ERROR ]\r\n");
    } else {
        int display_value;
        int dp_position;
        if(calc_status.state == STATE_RESULT || calc_status.state == STATE_WAIT_REG2){
            display_value = calc_status.reg1;
            dp_position = calc_status.reg1_dp;
        } else {
            display_value = calc_status.reg2;
            dp_position = calc_status.reg2_dp;
        }
        if (dp_position == -1) {
            printf("\r\n[7SEG DISPLAY: %d ]\r\n", display_value);
        } else if (dp_position == 0) {
            printf("\r\n[7SEG DISPLAY: %d. ]\r\n", display_value);
        } else {
            int divisor = 1;
            for(int i = 0; i < dp_position; i++){
                divisor *= 10;
            }
            int int_part = display_value / divisor;
            int frac_part = display_value % divisor;
            if (frac_part < 0) frac_part = -frac_part;
            if (display_value < 0 && int_part == 0) {
                printf("\r\n[7SEG DISPLAY: -0.%0*d ]\r\n", dp_position, frac_part);
            } else {
                printf("\r\n[7SEG DISPLAY: %d.%0*d ]\r\n", int_part, dp_position, frac_part);
            }
        }
    }
}
#else
void update_display(){
    if(calc_status.state == STATE_ERROR){
        display[0] = S7_r;
        display[1] = S7_o;
        display[2] = S7_r;
        display[3] = S7_r;
        display[4] = S7_e;
        display[5] = S7_space; 
        display[6] = S7_space;
        display[7] = S7_space;
    }else{
        int display_value;
        int dp_position;
        int digit = 0;
        bool is_positive;
        bool is_dp_set;
        if(calc_status.state == STATE_RESULT){
            display_value = calc_status.reg1;
            dp_position = calc_status.reg1_dp;
        }else{
            display_value = calc_status.reg2;
            dp_position = calc_status.reg2_dp;
        }
        is_positive = (display_value >= 0);
        is_dp_set = (dp_position != -1);
        // 小数点の位置を考慮して表示する数字を計算
        if (display_value < 0) {
            display_value = -display_value;
        }
        if (display_value == 0) {
            display[0] = S7_0;
            digit = 1;
        }
        while (display_value > 0){
            switch(display_value%10){
                case 0:
                    display[digit] = S7_0;
                    break;
                case 1:
                    display[digit] = S7_1;
                    break;
                case 2: 
                    display[digit] = S7_2;
                    break;
                case 3:
                    display[digit] = S7_3;
                    break;
                case 4:
                    display[digit] = S7_4;
                    break;
                case 5:
                    display[digit] = S7_5;
                    break;
                case 6:
                    display[digit] = S7_6;
                    break;
                case 7:
                    display[digit] = S7_7;
                    break;
                case 8:
                    display[digit] = S7_8;
                    break;
                case 9:
                    display[digit] = S7_9;
                    break;
            }
            display_value = display_value / 10;         
            digit++;
            if((is_positive && digit > MAX_7SEG_SIZE) || (!is_positive && digit > MAX_7SEG_SIZE - 1)){
                display[7] = S7_o;
                display[6] = S7_v;
                display[5] = S7_e;
                display[4] = S7_r;
                display[3] = S7_f;
                display[2] = S7_l;
                display[1] = S7_o;
                display[0] = S7_w;
                return;
            }
        }
        if (is_dp_set) {
            while (digit <= dp_position) {
            display[digit] = S7_0;
            digit++;
            }
        }
        if(is_dp_set){
            display[dp_position] |= S7_period; 
        }
        if(!is_positive){
            display[digit] = S7_minus;
            digit++;
        }
        for(int i = digit; i < MAX_7SEG_SIZE; i++){
            display[i] = S7_space;
        }
    }
    gpio->dout7SEG[1] = (
        display[7] << 24 | display[6] << 16 | display[5] << 8 | display[4] << 0
    );
    gpio->dout7SEG[0] = (
        display[3] << 24 | display[2] << 16 | display[1] << 8 | display[0] << 0 
    );  
}
#endif