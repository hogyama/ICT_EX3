
#if !defined(RV32_IO_SETTING_H)
#define RV32_IO_SETTING_H
enum IO_Parity
{
	IOP_None = 0,
	IOP_Odd  = 1,
	IOP_Even = 3,
	IOP_High = 5,
	IOP_Low  = 7,
};

enum UART_BIT
{
	UART_BIT5 = 0,
	UART_BIT6 = 1,
	UART_BIT7 = 2,
	UART_BIT8 = 3,
};

enum UART_BAUD
{
	UART_BAUD_50		= 0,
	UART_BAUD_300		= 1,
	UART_BAUD_1200		= 2,
	UART_BAUD_2400		= 3,
	UART_BAUD_4800		= 4,
	UART_BAUD_9600		= 5,
	UART_BAUD_19200		= 6,
	UART_BAUD_38400		= 7,
	UART_BAUD_57600		= 8,
	UART_BAUD_115200	= 9,
	UART_BAUD_230400	= 10,
	UART_BAUD_460800	= 11,
	UART_BAUD_500000	= 12,
	UART_BAUD_576000	= 13,
	UART_BAUD_921600	= 14,	///	921.6 kbps
	///	4/27/15 : added UART_BAUD_ULTRA_FAST
	UART_BAUD_ULTRA_FAST = 15,	///	clkFreq / 2 bps --> 13 Mbps
};

#define CLK_FREQ	50000000	/// 50MHz --> Nexys-A7-100T clock (100MHz) converted down by clocking wizard module

#define	BAUD_PERIOD(baud_rate)	((CLK_FREQ / baud_rate) - 1)	///	1302 clks/bit


//#define UART_PARITY		IOP_Even
#define UART_PARITY		IOP_None
#define UART_STOP_BIT	0	///	1 bit
#define UART_LENGTH		3	///	8 bits
//#define UART_BAUD_ID	UART_BAUD_ULTRA_FAST
#define UART_BAUD_ID	UART_BAUD_38400	///	UART_BAUD_115200	///	UART_BAUD_921600
#define UART_PARAM		(((UART_PARITY) << 8) | ((UART_STOP_BIT) << 7) | ((UART_LENGTH) << 5) | ((UART_BAUD_ID) << 1))
#define UART_PARAM_WITH_BAUD_SETTING(buadID)		(((UART_PARITY) << 8) | ((UART_STOP_BIT) << 7) | ((UART_LENGTH) << 5) | ((buadID & 0xf) << 1))

/// uart.write_command((IOP_Even << 8) | (0x0 << 7) | (0x3 << 5) | (UART_BAUD_ULTRA_FAST << 1) | 1, (uidx << 2) | 1, cycle);
#if 0
UART:
imm15[10:8] : 3 bits : parity setting
	0 / 2 / 4 / 6 : no parity	->	0 : IOP_None
	1 : odd parity(XNOR)		->	1 : IOP_Odd
	3 : even parity(XOR)		->	3 : IOP_Even
	5 : high parity(1)			->	5 : IOP_High
	7 : low parity(0)			->	7 : IOP_Low
imm15[7:7] : 1 bit : stop_bit length
	0 : 1 bit
	1 : 2 bit(6 / 7 / 8 - bit data), 1.5 bit(5 - bit data)
imm15[6:5] : 2 bits : data length
	0 : 5 bits
	1 : 6 bits
	2 : 7 bits
	3 : 8 bits
imm15[4:1] : 4 bits : baud setting
	0  :     50 bps
	1  :    300 bps
	2  :   1200 bps
	3  :   2400 bps
	4  :   4800 bps
	5  :   9600 bps
	6  :  19200 bps
	7  :  38400 bps
	8  :  57600 bps
	9  : 115200 bps
	10 : 230400 bps
	11 : 460800 bps
	12 : 500000 bps
	13 : 576000 bps
	14 : 921600 bps
	15 : clkFreq / 2 bps
imm15[0]   : 1 bit : deactivate(1), deactivate(0)
#endif


///	 AAA
/// F   B
/// F   B
///  GGG
/// E   C
/// E   C
///  DDD  DP

enum Seg7_Pattern {
	/// DP,G,F,E,D,C,B,A
	S7_0 = 0b00111111,	///	0 : 0x3f
	S7_1 = 0b00000110,	///	1 : 0x06
	S7_2 = 0b01011011,	///	2 : 0x5b
	S7_3 = 0b01001111,	///	3 : 0x4f
	S7_4 = 0b01100110,	///	4 : 0x66
	S7_5 = 0b01101101,	///	5 : 0x6d
	S7_6 = 0b01111101,	///	6 : 0x7d
	S7_7 = 0b00100111,	///	7 : 0x27
	S7_8 = 0b01111111,	///	8 : 0x7f
	S7_9 = 0b01101111,	///	9 : 0x6f
	S7_a = 0b01110111,	///	A : 0x77
	S7_b = 0b01111100, 	///	b : 0x7c
	S7_c = 0b00111001,	///	C : 0x39
	S7_d = 0b01011110,	///	d : 0x5e
	S7_e = 0b01111001,	///	E : 0x79
	S7_f = 0b01110001,	///	F : 0x71
	S7_g = 0b00111101,	///	G
	S7_h = 0b01110110,	///	H
	S7_i = 0b00000100,	///	i
	S7_j = 0b00001110,	///	J
	S7_k = 0b01111000,	///	k
	S7_l = 0b00111000,	/// L
	S7_m = 0b00110111,	///	M
	S7_n = 0b01010100,	///	n
	S7_o = 0b01011100,	///	o
	S7_p = 0b01110011,	///	P
	S7_q = 0b01100111,	///	q
	S7_r = 0b01010000,	///	r
	S7_s = 0b11101101,	///	S.
	S7_t = 0b01110000,	///	t
	S7_u = 0b00111110,	///	U
	S7_v = 0b00011100,	///	v
	S7_w = 0b10111110,	///	W.
	S7_x = 0b11110110,	///	X.
	S7_y = 0b01101110,	///	y
	S7_z = 0b11011011,	///	Z.
	S7_under_score 	= 0b00001000,	///	_
	S7_minus		= 0b01000000,	///	-
	S7_period		= 0b10000000,	///	.
	S7_space		= 0b00000000,	///	<space>
	S7_equal		= 0b01001000,	///	=
};

#endif
