#include "io.h"
#include <stdio.h>
#include <stdbool.h>
#define MAX_VALUE 9999999 // 表示できる最大値。8桁目は符号に用いる
#define OVERFLOW_VALUE (MAX_VALUE + 1) 
#define DIV_ACCURACY 7 
#define SCALE 10000000
#define MAX_7SEG_SIZE 8
#if defined(NATIVE_MODE)
    #define _io_putch putchar
    #define _io_getch getchar
#endif
typedef struct number{
    int value; // 小数点を無視 1.2ならvalueは12
    int dp; // 小数点以下の桁数 1.2ならdpは1 
}num_t;
typedef enum state{
    Q_N = 0,
    Q_ZERO,
    Q_DEC,
    Q_OP,
    Q_RE,
    Q_ER
}state_t;
typedef struct process_status{
    state_t current_state;
    state_t prev_state;
    num_t n1; //Q_REのとき、計算結果が入る
    num_t n2; //Q_N,Q_ZERO,Q_DECのとき、入力された数値が入る
    char operator;
}process_status_t;
typedef struct key_flag{
    bool is_period;
    bool is_zero;
    bool is_n;
    bool is_op;
    bool is_equal;
    bool is_clear;
    bool is_backspace;
    bool is_memory_add;
    bool is_memory_sub;
    bool is_memory_clear;
    bool is_memory_recall;
}key_flag_t;
typedef enum Memory_State{
    M_N = 0,
    M_ZERO,
    M_DEC
}memory_state_t;
typedef struct memory{
    num_t n;
    memory_state_t last_changed_state;
}memory_t;
typedef enum Seg7_Pattern seg7_t;
// prototype
char get_valid_key(void);
void update_flag(char key, key_flag_t* key_flag);
void update_process_status(char key, key_flag_t* key_flag, process_status_t* process_status);
void calc_result(process_status_t* process_status);
void update_display(process_status_t* process_status);
// process_statusの状態遷移
void process_n(char key, key_flag_t *key_flag, process_status_t* process_status);
void process_zero(char key, key_flag_t *key_flag, process_status_t* process_status);
void process_dec(char key, key_flag_t *key_flag, process_status_t* process_status);
void process_op(char key, key_flag_t *key_flag, process_status_t* process_status);
void process_re(char key, key_flag_t *key_flag, process_status_t* process_status);
void process_er(char key, key_flag_t *key_flag, process_status_t* process_status);
// print
void print_setup();
void print_error(void);
void print_overflow(void);
// display
void display_error(void);
void display_overflow(void);
void display_num(int value, int dp, state_t prev_state);
// memory
void memory_clear(); // MC
memory_t memory_recall(); // MR
bool memory_add(num_t a); // M+
bool memory_subtract(num_t a); // M-
// helper function
static int max(int a, int b);
static int count_digits(int value);
static int conv_to_7seg(int num);
// inline
static inline void clear_process(process_status_t* process_status){
    *process_status = (process_status_t){Q_OP, Q_OP, {0, 0}, {0, 0}, '+'};
    memory_clear();
}
// global variable
static memory_t memory = {{0,0}, Q_ZERO}; 
int main(){
    start_profiler();
    process_status_t process_status;
    key_flag_t key_flag = {false, false, false, false, false, false};
    // 初期状態は n1 ,operator は未定義にせず、0, '+' をダミーとして保持する
    process_status = (process_status_t){Q_OP, Q_OP, {0, 0}, {0, 0}, '+'};
    print_setup();
    while(1){
        char key = get_valid_key();
        if(key == EOF) break;
        update_flag(key, &key_flag);
        update_process_status(key, &key_flag, &process_status);
        update_display(&process_status);
    }
    end_profiler();
}
// 入力が有効なキーであるかをチェックする関数
char get_valid_key(void){
    char key;
    while(1){
        key = _io_getch();
        if((key >= '0' && key <= '9') || key == '+' || key == '-' || key == '*' || key == '/' || key == '%' || key == '=' || key == '.' || key == 'C' || key == 'c' || key == (char)0x08 ){
            return key;
        }else if(key == 'P' || key == 'N' || key == 'R' || key == 'D'){
            return key;
        }else if(key == 'p' || key == 'n' || key == 'r' || key == 'd'){
            return key;
        }
    } 
}
// 入力されたキーに応じてkey_flagを更新する関数
void update_flag(char key, key_flag_t* key_flag){
    key_flag->is_period = (key == '.');
    key_flag->is_zero = (key == '0');
    key_flag->is_n = (key >= '1' && key <= '9');
    key_flag->is_op = (key == '+' || key == '-' || key == '*' || key == '/' || key == '%');
    key_flag->is_equal = (key == '=');
    key_flag->is_clear = (key == 'C' || key == 'c');
    key_flag->is_backspace = (key == 0x08);
    key_flag->is_memory_add = (key == 'P' || key == 'p');
    key_flag->is_memory_sub = (key == 'N' || key == 'n');
    key_flag->is_memory_clear = (key == 'D' || key == 'd');
    key_flag->is_memory_recall = (key == 'R' || key == 'r');
    return;
}
// 現在の状態と入力されたキーに応じて状態遷移を行う関数
void update_process_status(char key, key_flag_t* key_flag, process_status_t* process_status){
    process_status->prev_state = process_status->current_state;
    switch(process_status->current_state){
        case Q_N:
            process_n(key, key_flag, process_status);
            break;
        case Q_ZERO:
            process_zero(key, key_flag, process_status);
            break;
        case Q_DEC:
            process_dec(key, key_flag, process_status);
            break;
        case Q_OP:
            process_op(key, key_flag, process_status);
            break;         
        case Q_ER:
            process_er(key, key_flag, process_status);
            break;
        case Q_RE:
            process_re(key, key_flag, process_status);  
            break;
    }
}
// 計算を行う関数
void calc_result(process_status_t* process_status){
    long long a = process_status->n1.value;
    long long b = process_status->n2.value;
    int dp_a = process_status->n1.dp;
    int dp_b = process_status->n2.dp;
    long long result_value;
    int result_dp;
    int max_dp = max(process_status->n1.dp, process_status->n2.dp);    
    switch(process_status->operator){
        case '+':
        case '-':
            while(dp_a < max_dp){
                a *= 10;
                dp_a++;
            }
            while(dp_b < max_dp){
                b *= 10;
                dp_b++;
            }
            result_value = (process_status->operator == '+') ? (a + b) : (a - b);
            result_dp = max_dp;
            break;
        case '*':
            result_value = a * b;
            result_dp = dp_a + dp_b; 
            break;
        case '/':
            if(b == 0){
                process_status->current_state = Q_ER;
                return;
            }
            result_value = a * SCALE / b;
            result_dp = dp_a - dp_b + DIV_ACCURACY;     
            break;
        case '%':
            if(b == 0){
                process_status->current_state = Q_ER;
                return;
            }
            if(max_dp > 0){
                process_status->current_state = Q_ER; // 剰余演算は小数を扱わないため
                return;
            }
            result_value = a % b;
            result_dp = 0;
            break;
    }
    // 結果を表示可能な形式に変換する
    // 小数点以下の桁数が足りない場合   
    while(result_dp < 0){ // result_valueを10倍し、小数点を右に動かすことを繰り返す
        result_value *= 10;
        result_dp++;
    }
    // 小数点以下の桁数が多すぎる場合 
    while(result_dp > MAX_7SEG_SIZE - 1){ // result_valueを10で割って切り捨て、小数点を左に動かすことを繰り返す
        result_value /= 10;
        result_dp--; 
    }
    // 答えが小数で表示可能な範囲を超える場合は切り捨て
    while(result_dp > 0 && (result_value > MAX_VALUE || result_value < -MAX_VALUE)){ 
        result_value /= 10;
        result_dp--;
    }
    // 小数点以下の末尾の0を削除
    while(result_dp > 0 && result_value % 10 == 0){ // 小数点以下の末尾の0を削除
        result_value /= 10;
        result_dp--;
    }
    // 答えが整数で表示可能な範囲を超える場合はエラー
    if(result_dp == 0 && (result_value > MAX_VALUE || result_value < -MAX_VALUE)){
        process_status->n1.value = OVERFLOW_VALUE;
        process_status->n1.dp = 0;
        process_status->n2 = (num_t){0,0};
        process_status->current_state = Q_ER;
        return;
    }
    process_status->n1.value = (int)result_value;
    process_status->n1.dp = result_dp;
    process_status->n2 = (num_t){0,0};
}
// 状態に応じて表示を更新する関数
void update_display(process_status_t* process_status){
    if(process_status->current_state == Q_ER){
        if(process_status->prev_state != Q_ER){
            if(process_status->n1.value == OVERFLOW_VALUE){
                print_overflow();
                display_overflow();
            }else{
                print_error();
                display_error();
            }
        }
    }else if(process_status->current_state == Q_RE){
        display_num(process_status->n1.value, process_status->n1.dp, process_status->prev_state);
    }else if(process_status->current_state == Q_OP){
        display_num(process_status->n1.value, process_status->n1.dp, process_status->prev_state); // 演算子を表示するため、小数点以下の桁数を1増やす
    }else if(process_status->current_state == Q_N || process_status->current_state == Q_ZERO || process_status->current_state == Q_DEC){
        display_num(process_status->n2.value, process_status->n2.dp, process_status->prev_state);
    }
}
// ==== 状態遷移の関数 ====
void process_n(char key, key_flag_t *key_flag, process_status_t* process_status){
    if(key_flag->is_n || key_flag -> is_zero){
        process_status->n2.value = process_status->n2.value * 10 + (key - '0');
        if(process_status->n2.value > MAX_VALUE){
            process_status->current_state = Q_ER;
        }
    }
    if(key_flag->is_period){
        process_status->current_state = Q_DEC;
    }
    if(key_flag->is_op){
        calc_result(process_status); // n1にn1 op n2を格納
        if(process_status->current_state != Q_ER){
            process_status->operator = key;
            process_status->current_state = Q_OP;
        }
    }
    if(key_flag->is_equal){
        calc_result(process_status);
        if(process_status->current_state != Q_ER){
            process_status->current_state = Q_RE;
        }
    }
    if(key_flag->is_clear){
        clear_process(process_status);
    }
    if(key_flag->is_backspace){
        if(process_status->n2.value < 9){
            process_status->current_state = Q_OP;
            process_status->n2 = (num_t){0,0};
        }else{        
            process_status->n2.value = process_status->n2.value / 10;  
        }
    }
    // メモリに加減算、リコールする際はn2を使う
    if(key_flag->is_memory_add){
        if(!memory_add(process_status->n2)){
            memory_clear();
        }
    }
    if(key_flag->is_memory_sub){
        if(!(memory_subtract(process_status->n2))){
            memory_clear();
        }
    }
    if(key_flag->is_memory_recall){
        memory_t mem = memory_recall();
        process_status->n2 = mem.n;
        process_status->current_state = (state_t)mem.last_changed_state;
    }
    // メモリクリアは常に有効
    if(key_flag->is_memory_clear){
        memory_clear();
    }
    return;
}
void process_zero(char key, key_flag_t *key_flag, process_status_t* process_status){
    if(key_flag->is_period){
        process_status->current_state = Q_DEC;
    }
    if(key_flag->is_op){
        calc_result(process_status); // n1にn1 op n2を格納
        process_status->operator = key;
        process_status->current_state = Q_OP;
    }
    if(key_flag->is_equal){
        calc_result(process_status);
        process_status->current_state = Q_RE;
    }
    if(key_flag->is_n){
        process_status->n2.value = key - '0';
        process_status->current_state = Q_N;
    }
    if(key_flag->is_zero){
        process_status->n2.value = 0;
        process_status->current_state = Q_ZERO;
    }
    if(key_flag->is_clear){
        clear_process(process_status);
    }
    if(key_flag->is_backspace){
        process_status->current_state = Q_OP;
        process_status->n2 = (num_t){0,0};
    }
    // メモリに加減算、リコールする際はn2を使う
    if(key_flag->is_memory_add){

        if(!memory_add(process_status->n2)){
            memory_clear();
        }
    }
    if(key_flag->is_memory_sub){
        if(!(memory_subtract(process_status->n2))){
            memory_clear();
        }
    }
    if(key_flag->is_memory_recall){
        memory_t mem = memory_recall();
        process_status->n2 = mem.n;
        process_status->current_state = (state_t)mem.last_changed_state; // memoryの状態に合わせてQ_N, Q_ZERO, Q_DECのいずれかに遷移
    }
    if(key_flag->is_memory_clear){
        memory_clear();
    }
    
    return;
}
void process_dec(char key, key_flag_t *key_flag, process_status_t* process_status){
    if(key_flag->is_period){
        process_status->current_state = Q_ER;
    }
    if(key_flag->is_zero || key_flag->is_n){
        process_status->n2.value = process_status->n2.value * 10 + (key - '0');
        if(process_status->n2.value > MAX_VALUE){
            process_status->current_state = Q_ER;
        }
        process_status->n2.dp++;
        if(process_status->n2.dp > MAX_7SEG_SIZE - 1){
            process_status->current_state = Q_ER;
        }else{
            process_status->current_state = Q_DEC;
        }
    }
    if(key_flag->is_op){
        calc_result(process_status); // n1にn1 op n2を格納
        process_status->operator = key;
        process_status->current_state = Q_OP;
    }
    if(key_flag->is_equal){
        calc_result(process_status);
        process_status->current_state = Q_RE;
    }
    if(key_flag->is_clear){
        clear_process(process_status);
    }
    if(key_flag->is_backspace){
        if(process_status->n2.dp > 0){
            process_status->n2.value = process_status->n2.value / 10;  
            process_status->n2.dp--; // 12.3をbackspaceなら12.
        }else{
            process_status->current_state = Q_N; // 12.をbackspaceなら12
        }
    }
    // メモリに加減算、リコールする際はn2を使う
    if(key_flag->is_memory_add){
        if(!memory_add(process_status->n2)){
            memory_clear();
        }
    }
    if(key_flag->is_memory_sub){
        if(!(memory_subtract(process_status->n2))){
            memory_clear();
        }
    }
    if(key_flag->is_memory_recall){
        memory_t mem = memory_recall();
        process_status->n2 = mem.n;
        process_status->current_state = (state_t)mem.last_changed_state;
    }
    if(key_flag->is_memory_clear){
        memory_clear();
    }
    return;
}
void process_op(char key, key_flag_t *key_flag, process_status_t* process_status){
    if(key_flag->is_period || key_flag->is_equal || key_flag->is_op || key_flag->is_memory_add || key_flag->is_memory_sub){
        process_status->current_state = Q_ER;
    }
    if(key_flag->is_n){
        process_status->n2.value = key - '0';
        process_status->current_state = Q_N;
    }
    if(key_flag->is_zero){
        process_status->n2.value = 0;
        process_status->current_state = Q_ZERO;
    }
    if(key_flag->is_clear){
        clear_process(process_status);
    }
    if(key_flag->is_backspace){
        num_t temp = process_status->n1;
        state_t temp_state = process_status->prev_state;
        // 演算子を削除して入力状態へ戻す
        // operator は未定義にせず、初期状態と同様に '+' をダミーとして保持する
        *process_status = (process_status_t){temp_state, Q_OP, {0,0}, temp, '+'}; 
    }
    // メモリに加減算する際はn1を使う
    if(key_flag->is_memory_add){
        if(!memory_add(process_status->n1)){
            memory_clear();
        }
    }
    if(key_flag->is_memory_sub){
        if(!(memory_subtract(process_status->n1))){
            memory_clear();
        }
    }
    // リコールはn2に値をセットして状態遷移する
    if(key_flag->is_memory_recall){
        memory_t mem = memory_recall();
        process_status->n2 = mem.n;
        process_status->current_state = (state_t)mem.last_changed_state;
    }
    if(key_flag->is_memory_clear){
        memory_clear();
    }
    return;
}
void process_er(char key, key_flag_t *key_flag, process_status_t* process_status){
    if(key_flag->is_clear){
        clear_process(process_status);
    }else{
        process_status->current_state = Q_ER;
    }
    return;
}
void process_re(char key, key_flag_t *key_flag, process_status_t* process_status){
    if(key_flag->is_period){
        process_status->current_state = Q_ER;
    }
    if(key_flag->is_zero){
        *process_status = (process_status_t){Q_ZERO, Q_RE, {0, 0}, {0, 0}, '+'};
    }
    if(key_flag->is_n){
        *process_status = (process_status_t){Q_N, Q_RE, {0, 0}, {key - '0', 0}, '+'};
    }
    if(key_flag->is_op){
        *process_status = (process_status_t){Q_OP, Q_RE, process_status->n1, {0, 0}, key};
    }
    if(key_flag->is_equal){
        process_status->current_state = Q_RE;
    }
    if(key_flag->is_clear){
        clear_process(process_status);
    }
    if(key_flag->is_backspace){
        process_status->current_state = Q_ER;
    }   
    // メモリに加減算する際はn1を使う
    if(key_flag->is_memory_add){
        if(!memory_add(process_status->n1)){
            memory_clear();
        }
    }
    if(key_flag->is_memory_sub){
        if(!(memory_subtract(process_status->n1))){
            memory_clear();
        }
    }
    // 新規入力としてn2に値をセットして状態遷移する
    if(key_flag->is_memory_recall){
        process_status->n1 = (num_t){0,0}; 
        process_status->operator = '+'; // ダミー
        memory_t mem = memory_recall();
        process_status->n2 = mem.n;
        process_status->current_state = (state_t)mem.last_changed_state;
    }
    if(key_flag->is_memory_clear){
        memory_clear();
    }
    return;    
}
// ==== printの関数 ====
void print_setup(){
    char setup_msg[] =
        "Calculator functions:\n"
        "  [0-9] : 数字入力\n"
        "  [.]   : 小数点入力\n"
        "  [+]   : 加算\n"
        "  [-]   : 減算\n"
        "  [*]   : 乗算\n"
        "  [/]   : 除算\n"
        "  [%]   : 剰余(整数のみ)\n"
        "  [=]   : 計算実行\n"
        "  [P]   : M+\n"
        "  [N]   : M-\n"
        "  [R]   : MR\n"
        "  [D]   : MC\n"
        "  [BS]  : 1文字削除\n" 
        "  [C]   : クリア\n"
        "\n"
        "注意:\n"
        "  - 小数入力に対応しています。\n"
        "  - 剰余演算は小数を扱いません。\n"
        "  - 表示範囲を超えると OVERFLOW を表示します。\n"
        "  - 不正な入力では ERROR になります。\n"
        "  - メモリに加算/減算する際は、現在の入力値や計算結果がメモリに加算/減算されます。\n"
        "  - メモリリコールすると、メモリの値が現在の入力値としてセットされます。\n"
        "  - メモリがオーバーフローした場合は、強制的にメモリがクリアされます。\n"
        "入力例: 10+12-1.23*60/80=\n";
    for(int i = 0; setup_msg[i] != '\0'; i++){
        _io_putch(setup_msg[i]);
    }
}
void print_error(){
    char error_msg[] = "Error: invalid input. Please enter C or c to clear.\n";
    for(int i = 0; error_msg[i] != '\0'; i++){
        _io_putch(error_msg[i]);
    }
}
void print_overflow(void){
    char overflow_msg[] = "Error: overflow. Please enter C or c to clear.\n";
    for(int i = 0; overflow_msg[i] != '\0'; i++){
        _io_putch(overflow_msg[i]);
    }
}
// ==== displayの関数 ====
void display_error(void){
#if !defined(NATIVE_MODE)
    gpio->dout7SEG[1] = (
        S7_space << 24 | S7_space << 16 | S7_space << 8 | S7_e << 0
    );
    gpio->dout7SEG[0] = (
        S7_r << 24 | S7_r << 16 | S7_o << 8 | S7_r << 0
    );
#endif
}
void display_overflow(void){
#if !defined(NATIVE_MODE)
    gpio->dout7SEG[1] = (
        S7_o << 24 | S7_v << 16 | S7_e << 8 | S7_r << 0
    );
    gpio->dout7SEG[0] = (
        S7_f << 24 | S7_l << 16 | S7_o << 8 | S7_w << 0 
    );
#endif
}
void display_num(int value, int dp, state_t prev_state){
#if !defined(NATIVE_MODE)
    int num[MAX_7SEG_SIZE] = {S7_space, S7_space, S7_space, S7_space, S7_space, S7_space, S7_space, S7_space};
    int abs_value = (value >= 0) ? value : -value;
    bool is_positive = (value >= 0);
    int i;
    for(i = 0; i < MAX_7SEG_SIZE - 1; i++){
        num[i] = conv_to_7seg(abs_value % 10);
        abs_value /= 10;
        if(abs_value == 0) break;
    }
    while(dp > i){ 
        i++; 
        num[i] = S7_0;
    } 
    if(!is_positive){
        num[7] = S7_minus; 
    }
    if(dp > 0){
        num[dp] |= S7_period;
    }
    if(prev_state == Q_DEC && dp == 0){
        num[0] |= S7_period; // 小数点以下の桁数が0のときは、整数部の最後の数字に小数点をつける
    }
    gpio->dout7SEG[1] = (
        num[7] << 24 |
        num[6] << 16 |
        num[5] << 8 |
        num[4] << 0
    );
    gpio->dout7SEG[0] = (
        num[3] << 24 |
        num[2] << 16 |
        num[1] << 8 |
        num[0] << 0
    );
#else
    // パソコンデバック
    printf("value = %d, dp = %d\n", value,dp);
#endif
}
// ==== memoryの関数 ====
bool memory_add(num_t a){
    if(memory.n.dp != a.dp){
        // dpを合わせる
        while(memory.n.dp < a.dp){
            memory.n.value *= 10;
            memory.n.dp++;
        }
        while(memory.n.dp > a.dp){
            a.value *= 10;
            a.dp++;
        }
    }
    memory.n.value += a.value;
    memory.n.dp = (memory.last_changed_state == M_DEC) ? memory.n.dp : a.dp;
    if(memory.n.value == 0 && memory.n.dp == 0){
        memory.last_changed_state = M_ZERO;
    }else if(memory.n.dp > 0){
        memory.last_changed_state = M_DEC;
    }else{
        memory.last_changed_state = M_N;
    }
    if(memory.n.value > MAX_VALUE || memory.n.value < -MAX_VALUE){
        return false; // オーバーフロー
    }
    return true;
}
bool memory_subtract(num_t a){
    if(memory.n.dp != a.dp){
        // dpを合わせる
        while(memory.n.dp < a.dp){
            memory.n.value *= 10;
            memory.n.dp++;
        }
        while(memory.n.dp > a.dp){
            a.value *= 10;
            a.dp++;
        }
    }
    memory.n.value -= a.value;
    memory.n.dp = (memory.last_changed_state == M_DEC) ? memory.n.dp : a.dp;
    if(memory.n.value == 0 && memory.n.dp == 0){
        memory.last_changed_state = M_ZERO;
    }else if(memory.n.dp > 0){
        memory.last_changed_state = M_DEC;
    }else{
        memory.last_changed_state = M_N;
    }
    if(memory.n.value > MAX_VALUE || memory.n.value < -MAX_VALUE){
        return false; // オーバーフロー
    }
    return true;
}
void memory_clear(){
    memory.n = (num_t){0,0};
    memory.last_changed_state = M_ZERO;
}
memory_t memory_recall(){
    return memory;
}
// ==== helper function ====
int max(int a, int b){
    return (a > b) ? a : b;
}
int count_digits(int value){
    int abs_value = (value >= 0) ? value : -value;
    int digits = 1;

    while(abs_value >= 10){
        abs_value /= 10;
        digits++;
    }
    return digits;
}
int conv_to_7seg(int num){
#if !defined(NATIVE_MODE)
    switch(num){
        case 0: return S7_0;
        case 1: return S7_1;
        case 2: return S7_2;
        case 3: return S7_3;
        case 4: return S7_4;
        case 5: return S7_5;
        case 6: return S7_6;
        case 7: return S7_7;
        case 8: return S7_8;
        case 9: return S7_9;
        default: return S7_space;
    }
#else
    return 0;
#endif
}
