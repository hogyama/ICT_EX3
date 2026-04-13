
#include "../rv_test/io.h"

GPIO* gpio = (GPIO* ) GPIO_ADDR;
UART* uart[2] = { (UART*)UART_0_ADDR, (UART*)UART_1_ADDR };


void putbyte(char ch)
{
	while (UART_TX_FULL(uart[0])) {} /// while (tx_full == 1);
	uart[0]->data = ch;
}

void putline(const char *s)
{
	char ch;
	while (ch = *s++)
		putbyte(ch);
}

void putnum(unsigned val) {
    if (val >= 0 && val <= 9) { putbyte('0' + val); }
    else if (val >= 0xa && val <= 0xf) { putbyte('a' + val - 0xa); }
    else { putbyte('?'); }
}

void putdec(unsigned val) {
    if (val >= 10) { putdec(val / 10); }
    putnum(val % 10);
}

void _puthex(unsigned val, int len) {
    if (val >> 4) { _puthex(val >> 4, len - 1); }
    else {
        for (int i = 0; i < len - 1; i++) {
            putbyte('0');
        }
    }
    putnum(val & 0xf);
}

#define puthex(v)   _puthex(v, 8)

unsigned getbyte() {
    while (UART_RX_EMPTY(uart[0])) {} /// while (rx_empty == 1);
    return uart[0]->data;
}

unsigned getint() {
    unsigned val = getbyte();
    val |= getbyte() << 8;
    val |= getbyte() << 16;
    val |= getbyte() << 24;
    return val;
}

int main() {
    /// activate uart
#if 1
    uart[0]->ctrl = 1 | UART_PARAM;
    uart[1]->ctrl = 1 | UART_PARAM;

#else
    unsigned sw_in = *gpio;
    *gpio = sw_in;
    gpio[3] = sw_in;
    if (sw_in == 0) {
        * (unsigned short*)(uart + 4) = 1 | UART_PARAM;
    }
    else {
        * (unsigned short*)(uart + 4) = 1 | UART_PARAM_WITH_BAUD_SETTING(sw_in & 0xf);
    }
#endif
    /// first message : rv32 --> host(PC)
    putline("\n==========================================================\n");
    putline("rv32 uart boot loader ready!! waiting for file transfer...\n");
    putline("==========================================================\n");
    gpio->dout7SEG[1] = (S7_u << 24) | (S7_a << 16) | (S7_r << 8) | (S7_t);
    gpio->dout7SEG[0] = (S7_b << 24) | (S7_o << 16) | (S7_o << 8) | (S7_t);
    unsigned checksum = 0, wordcount = 0;
    unsigned entry_point = getint();
    //gpio->data = entry_point;
    unsigned section_count = getint();
    wordcount+= 2;
    checksum += entry_point + section_count;
    for (int i = 0; i < section_count; ++i) {
        int addr = getint(), size = getint();
        wordcount += 2;
        checksum += addr + size;
        unsigned *paddr = (unsigned*)addr;
        for (int i = 0; i < size; i += 4) {
            unsigned w = getint();  /// write to memory
            checksum += w;
            wordcount++;
            gpio->data = wordcount;
            *(paddr++) = w;
            if (!(i & 0x3f)) {
                putbyte('.');
            }
        }
    }
    unsigned r_wordcount = getint();
    unsigned r_checksum = getint();
    putline("\ntransfer complete!!!\n\n");
    putline("entry_point = 0x"); puthex(entry_point); putbyte('\n');
    putline("section_count = "); putdec(section_count); putbyte('\n');
    putline("word_count = "); putdec(wordcount); putbyte('\n');
    putline("checksum = 0x"); puthex(checksum); putbyte('\n');
    putline("r_word_count = "); putdec(r_wordcount); putbyte('\n');
    putline("r_checksum = 0x"); puthex(r_checksum); putbyte('\n');
#define SHOW_INST_COUNT     64
    unsigned* paddr = (unsigned*) entry_point;
    for (int i = 0; i < SHOW_INST_COUNT; ++i) {
        if ((i & 3) == 0) {
            putbyte('\n');
            puthex((unsigned)paddr);
            putline(" : ");
        }
        puthex(*(paddr++));
        putbyte(' ');
    }
    if ((wordcount == r_wordcount) && (checksum == r_checksum)) {
        putline("\nuart boot loader transfer succeeded!! press any key to run your program... ");
        getbyte();
        asm volatile("jr %[new]" : : [new] "r" (entry_point));       /// jump to entry_point
    }
    else {
        putline("uart boot loader transfer failed... aborting\n");
    }
    return 0;
}

