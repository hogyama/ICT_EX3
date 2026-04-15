#include "io.h"
#include <stdio.h>
#include <stdbool.h>
#if !defined(NATIVE_MODE)
#define PUTCH_MODE

typedef enum State{
    STATE_INPUT= 0,         // 数字入力待ち
    STATE_OPERATOR_ENTERED, // 演算子が直前に入力された状態
    STATE_RESULT_DISPLAYED, // =が押され、結果が表示された状態
    STATE_ERROR             // エラー状態。クリアされるまでこの状態を維持する
}State_t;
typedef struct Calculator_Status{
    int saved_value;   // 保持されている左側の数値
    char current_operator; // 現在入力されている演算子
    bool is_first_entered; // 最初の入力が行われたかどうか
    State_t state;
}status_t;
static status_t calc_status = {0, 0, 0, true, STATE_INPUT};
// とりあえず
// 数字と演算子と=以外はエラーを表示する
char get_valid_key(void);
// 計算を進める
void process_calc(char key);
// ディスプレイを更新
void update_display(int num);

int main(){
    return 0;
}
char get_valid_key(void){
    char key = _io_getch();
    while(1){
        // クリアはCかc
        if((key >= '0' && key <= '9') || key == '+' || key == '-' || key == '*' || key == '/' || key == '%' || key == '='){
            #if defined(PUTCH_MODE)
                _is_putch(key);
            #endif
            return key;
        }else if(key == 'C' || key == 'c'){
            _is_putch("\r");
            _is_putch("\n");
            return key;
        }
    }
}
void process_calc(char key){
    if(calc_status.state == STATE_ERROR){
        if(key == 'c' || key == 'C'){
            calc_status = (status_t){0, 0, 0, false, STATE_INPUT};
        }
    }
    else if(calc_status.state == STATE_INPUT){
        if(key >= '0' && key <= '9'){
            int num = key - '0';
            if(!calc_status.is_first_entered){
                calc_status.saved_value = num;
                calc_status.is_first_entered = true;
            }else{
                calc_status.saved_value = calc_status.saved_value * 10 + num;
            }
        }else if(key == '+' || key == '-' || key == '*' || key == '/' || key == '%'){
            calc_status.saved_value = calc_status.saved_value;
            calc_status.current_operator = key;
            calc_status.state = STATE_OPERATOR_ENTERED;
        }else if(key == '='){
            calc_status.state = STATE_RESULT_DISPLAYED;
        }else{
            char error_msg[] = "Error: invalid input. Please enter C or c to clear.\n";
            for(int i = 0; error_msg[i] != '\0'; i++){
                _is_putch(error_msg[i]);
            }
            calc_status.state = STATE_ERROR;
        }
    }
}   
#endif