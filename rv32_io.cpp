
#include "rv32.h"

int nxt_pnt(int cur_pnt){ return (cur_pnt == MAX_IO_BUF_SIZE - 1) ? 0 : cur_pnt + 1;}

#define BUF_EMPTY(buf)	(!(buf)->not_empty)
#define BUF_FULL(buf)	((buf)->full)

//#define DBG_SYSCTRL	///	4/8/24

#if 0	///	SW view of SYSCTRL
struct SYSCTRL {
	unsigned int irq_ctrl;			///	0x10001000
	unsigned int pad0;				///	0x10001004
	unsigned int ext_irq_enable;	///	0x10001008
	unsigned int pad1;				///	0x1000100c
	unsigned long long mtime;		///	0x10001010
	unsigned long long mtimecmp;	///	0x10001018
};
volatile struct SYSCTRL* sysctrl = (struct SYSCTRL*)0x10001000;
#endif

#define DETECT_INTR_POS_EDGE

void SYSCTRL::fsm(U2 io_op, U5 addr, U32 wdata, U32 intr, U32 interal_irq_clear_mask) {
#if defined(DETECT_INTR_POS_EDGE)
	unsigned intr_pos_edge = (~d_intr[0]) & (d_intr[1]);
	auto	n_ext_irq_pending = ext_irq_pending | intr_pos_edge;
	auto	n_ext_irq_enable = ext_irq_enable;
	d_intr[0] = d_intr[1];
	d_intr[1] = d_intr[2];
	d_intr[2] = intr;
#else
	auto	n_ext_irq_pending = ext_irq_pending | intr;
	auto	n_ext_irq_enable = ext_irq_enable;
#endif

	data_in = 0;
	active = 0;
	data_latched = 0;

#if 0
	irq_set = intr;
	irq_clr = 0;
#endif

	mtip = (mtime_h > mtimecmp_h) || ((mtime_h == mtimecmp_h) && (mtime_l >= mtimecmp_l));
	meip = ((n_ext_irq_pending & n_ext_irq_enable) != 0);

	if (io_op) {
		bool readFlag = (io_op == 1);
		active = 1;
		data_latched = readFlag;
		switch (addr) {
		case 0x00:
			if (readFlag) { data_in = n_ext_irq_pending & n_ext_irq_enable; }
			else		  { n_ext_irq_pending &= ~wdata; }	// clear bits
#if defined(DBG_SYSCTRL)	///	4/8/24
			if (!readFlag) { printf("n_ext_irq_pending <-- %08x\n", n_ext_irq_pending); }
#endif
			break;
		case 0x08:
			if (readFlag) { data_in = n_ext_irq_enable; }
			else		  { n_ext_irq_enable = wdata; }
#if defined(DBG_SYSCTRL)	///	4/8/24
			if (readFlag) { printf("data_in <-- n_ext_irq_enable(%08x)\n", n_ext_irq_enable); }
			else		  { printf("n_ext_irq_enable <-- %08x\n", n_ext_irq_enable); }
#endif
			break;
		case 0x10:
			if (readFlag) { data_in = mtime_l; }
			break;
		case 0x14:
			if (readFlag) { data_in = mtime_h; }
			break;
		case 0x18:
			if (readFlag) { data_in = mtimecmp_l; }
			else		  { mtimecmp_l = wdata; }
			break;
		case 0x1c:
			if (readFlag) { data_in = mtimecmp_h; }
			else	      { mtimecmp_h = wdata; }
			break;
		}

	}

#if defined(DBG_SYSCTRL) && !defined(SUPPRESS_MISC_LOG)	///	4/8/24
	if (ext_irq_pending != n_ext_irq_pending) {
		printf("ext_irq_pending : %08x --> %08x\n", ext_irq_pending, n_ext_irq_pending);
	}
#endif

	ext_irq_pending = n_ext_irq_pending & ~interal_irq_clear_mask;
	ext_irq_enable = n_ext_irq_enable;

	if (divider == 0) {
		incr_timer();
	}
	divider = (divider + 1) & 0xf;
}

void IO::fsm(U2 io_op, U2 io_size, U32 addr, U32 wdata, AXI4L::CH* axi, U32 intr, U32 internal_irq_clear_mask)
{
	BIT sys_mode = (io_op && (addr >> 5) == (0x10001000 >> 5));
	U2 sys_op = (sys_mode) ? io_op : 0;	
	U2 axi_op = (sys_mode) ? 0 : io_op;	
	sysctrl.fsm(sys_op, addr & 0x1f, wdata, intr, internal_irq_clear_mask);
	axim.fsm(axi_op, io_size, addr, wdata, axi);
	stalled = axim.stalled;
	active = sysctrl.active | axim.active;
	data_in = sysctrl.data_in | axim.data_in;
	data_latched = sysctrl.data_latched | axim.data_latched;
	if (data_latched) { latched_data = data_in; }
}

void RV_AXI4_Master::fsm(U2 io_op, U2 io_size, U32 addr, U32 wdata, AXI4L::CH* axi) {
	io_op |= pending_op;
	data_latched = 0;
	data_in = 0;
	burst_length = 0;
	unsigned size = io_size;
	stalled = !MasterFSM::fsm(io_op, axi, addr, size, wdata, &data_in, &data_latched);
	pending_op = (stalled) ? io_op : IO_Idle;
	active = (io_op != IO_Idle);
}

bool AXI4L::MasterFSM::fsm(U2 op, CH *axi, AXI_AType addr, U3 size, AXI_DType dout, AXI_DType *din, BIT *data_latched) {
	BIT stalled = 0;
	switch (rw_state) {
	case RW_Init:
		if (op == AXI_Read) {
			out.setRead(addr, size, 1, 1, burst_length, mem_prot);    /// ra_addr, ra_size, ra_valid, rd_ready
			burst_count = burst_length;
			in.resetRead();
			stalled = 1;
			rw_state = RW_RAddr;
		}
		else if (op == AXI_Write) {  /// isWrite
			out.setWrite(addr, size, 1, dout, 0xf, 1, (burst_length == 0), 1, burst_length, mem_prot); /// wa_addr, wa_size, wa_valid,wd_data,wd_strobe,wd_valid,wr_ready
			burst_count = burst_length;
			in.resetWrite();
			stalled = 1;
			//*data_latched = 1;
			rw_state = RW_WAddr;
		}
		break;
	case RW_RAddr: {
		int n_rflag = (in.rdat.valid << 1) | in.raddr.ready;
		if (axi->raddr.s.ready) {
			n_rflag |= 1;
			in.raddr.set(axi->raddr.s);
			out.raddr.reset();
		}
		if (axi->rdat.s.valid) {
			if (burst_count == 0) {
				n_rflag |= 2; in.rdat.set(axi->rdat.s);
				out.rdat.reset();
			} else {
				burst_count--;
			}
			*din = axi->rdat.s.data;
			*data_latched = 1;
			if (axi->rdat.s.resp != RSP_OK) {
				printf("[AXI::Master] bad rdat.resp from slave!!!\n");
			}
		}
		if (n_rflag == 3) { rw_state = RW_Init; }
		else { stalled = 1; }
		break;
	}
	case RW_WAddr: {
		int n_wflag = (in.wres.valid << 2) | (in.wdat.ready << 1) | in.waddr.ready;
		int bad_resp = (in.wres.resp != RSP_OK);
		if (axi->waddr.s.ready) {
			n_wflag |= 1;
			in.waddr.set(axi->waddr.s);
			out.waddr.reset();
		}
		if (axi->wdat.s.ready) {
			if (burst_count == 0) {
				n_wflag |= 2;
				out.wdat.reset();
			} else {
				out.wdat.last = (burst_count == 1);
				out.wdat.data = dout;
				burst_count--;
				*data_latched = 1;
			}
			in.wdat.set(axi->wdat.s);
		}
		if (axi->wres.s.valid) {
			n_wflag |= 4;
			in.wres.set(axi->wres.s);
			out.wres.reset();
			if (axi->wres.s.resp != RSP_OK) {
#if !defined(SUPRESS_LOCK_ERROR_MSG)
				printf("[AXI::Master] bad wres.resp from slave!!! (lock/unlock error?)\n");
#endif
				bad_resp = 1;
			}
		}
		if (n_wflag == 7) {  /// 6/23/17 : if (bad response) { try again; }
			rw_state = RW_Init;
			if (bad_resp) { stalled = 1; }  /// try again
		}
		else { stalled = 1; }
		break;
	}
	}
	axi->setMARead(out.raddr, out.rdat);
	axi->setMAWrite(out.waddr, out.wdat, out.wres);
	return !stalled;
}

BIT UART::parity_bit(BIT parity) {
	switch (parity_type) {
	case IOP_Odd:	return !parity;
	case IOP_Even:	return parity;
	case IOP_High:	return 1;
	default:		return 0;
	}
}

#if 0	///	uart verilog from ex3_2013
parameter BAUD_PERIOD = 2812; /* 2812.5 */   /// = CLK_FREQ / BAUD_RATE
/// clk_freq = 27,000,000 Hz (27MHz)
/// BAUD_RATE = 9600 bps
/// BAUD_PERIOD = 27000000 / 9600 = 2812.5
#endif

/// 6/5/17 : C++!!!

//#define DBG_UART

void UART::do_tx(U2 io_op, U32 src0)
{
	auto* buf = &tx.buf;
	int empty = BUF_EMPTY(buf), full = BUF_FULL(buf);
	int rp = buf->rp, wp = buf->wp;
	int wflag = (io_op == IO_WriteData) && !full, rflag = 0;
	tx.stalled = (io_op == IO_WriteData) && full;

	if (tx.state == IOS_Idle) {
		if (!empty) {
			rflag = 1;
			U8 w = buf->buf[rp];
			tx.activate(w);
			tx.bit_n = !0;	///	start_bit = 0;
#if defined(DBG_UART)
			printf("UART-TX (%2x : %c)\n", w, w);
#endif
		}
	}
	else {
		if (tx.tick < baud_period_m1) {
			int stop_at_half_period = (tx.state == IOS_Stop && tx.bit_pos && stop_count == 3);	///	stop_count = 3 indicates 1.5 bit length
			int is_half_period = (tx.tick == (baud_period_m1 >> 1));
			tx.tick++;
			if (stop_at_half_period && is_half_period) { tx.state = IOS_Idle; }	///	stop_length = 1.5 bit
		}
		else {	///	1 baud period reached...
			int bit = tx.word & 0x1;
			tx.tick = 0;
			switch (tx.state) {
			case IOS_Start:
				tx.bit_n = !bit;
				///	bit_pos : 0, 1, 2, ..., bit_count - 1
				if (tx.bit_pos == bit_count_m1) {
					tx.state = (parity_type != IOP_None) ? IOS_Parity : IOS_Stop;
					tx.bit_pos = 0;
				}
				else {
					tx.bit_pos++;
				}
				tx.parity ^= bit;
				tx.word >>= 1;
				break;
			case IOS_Parity:
				tx.bit_n = !parity_bit(tx.parity);
				tx.state = IOS_Stop;
				break;
			case IOS_Stop:
				if (tx.bit_pos < stop_count) {
					tx.bit_n = !1;	///	stop_bit = 1;
					tx.bit_pos++;
				}
				else {	///	stop_bit finished... ready to transmit next word
					if (!empty) {
						rflag = 1;
						U8 w = buf->buf[rp];
						tx.activate(w);
#if defined(DBG_UART)
						printf("UART-TX (%2x : %c)\n", w, w);
#endif
						tx.bit_n = !0;	///	start_bit = 0;
					}
					else {
						tx.state = IOS_Idle;
						tx.bit_n = !1;	///	stop_bit = 1;
					}
				}
				break;
			}
		}
	}
	if (wflag) { buf->buf[wp] = src0; }
	buf->update(rflag, wflag);
	tx.irq = rflag;
}

U32 UART::adjust_word()
{
	switch (bit_count_m1) {
	case 5 - 1:		return (rx.word >> 3);
	case 6 - 1:		return (rx.word >> 2);
	case 7 - 1:		return (rx.word >> 1);
	default:		return (rx.word >> 0);
	}
}

U32 UART::do_rx(U2 io_op)
{
	auto* buf = &rx.buf;
	int empty = BUF_EMPTY(buf);
	int full = BUF_FULL(buf);
	int wp = buf->wp;
	int rp = buf->rp;
	int rflag = (io_op == IO_ReadData) && !empty;
	int wflag = 0;
	U32 ret_val = 0;
	int bit_rx = (!rx.bit_n);
	rx.stalled = (io_op == IO_ReadData) && empty;

	if (rflag)
	{
		ret_val = buf->buf[rp];
#if defined(DBG_UART)
		printf("UART-RX (%2x : %c)\n", ret_val, ret_val);
#endif
	}
	if (rx.state == IOS_Idle && bit_rx == 0) {
		rx.activate(0);
	}	///	start_bit = 0
	else if (rx.state != IOS_Idle) {
		if (rx.tick < baud_period_m1) {
			int stop_at_half_period = (rx.state == IOS_Stop && rx.bit_pos && stop_count == 3);	///	stop_count = 3 indicates 1.5 bit length
			int is_half_period = (rx.tick == (baud_period_m1 >> 1));
			rx.tick++;
			if (is_half_period) {	///	sample at middle of baud period
				rx_bit_samp = bit_rx;
				if (rx.state == IOS_Start) {
					rx.word = (bit_rx << 7) | (rx.word >> 1);
					rx.parity ^= bit_rx;
				}	///	ignore parity_bit/end_bit
				if (stop_at_half_period) { rx.state = IOS_Idle; }
			}
		}
		else {	///	1 baud period reached...
			rx.tick = 0;
			switch (rx.state) {
			case IOS_Start:
				if (rx.bit_pos > bit_count_m1) {	///	0, 1, 2, ..., bit_count - 1, bit_count (need to include start_bit here...)
					rx.state = (parity_type != IOP_None) ? IOS_Parity : IOS_Stop;
					if ((wflag = !full)) {
						U32 rx_word = adjust_word();
						buf->buf[wp] = rx_word;
					}
					rx.bit_pos = 0;
				}
				else { rx.bit_pos++; }
				break;
			case IOS_Parity:
				if (parity_bit(rx.parity) != rx_bit_samp) { rx.error = 1; }
				rx.state = IOS_Stop;
				rx.bit_pos++;
				break;
			case IOS_Stop:
				if (rx_bit_samp != 1 || full) { rx.error = 1; }
				///	9/24/14 : for rx, only assume 1 stop-bit
				if (bit_rx == 0) { rx.activate(0); }	///	sample start-bit here!!!
				else { rx.state = IOS_Idle; }
			}
		}
	}
	buf->update(rflag, wflag);
	///	4/28/15 : modified again.... 
	rx.irq = !empty;
	return ret_val;
}

static const int uart_baud_period[16] = {
	BAUD_PERIOD(50),		///	0
	BAUD_PERIOD(300),		///	1
	BAUD_PERIOD(1200),		///	2
	BAUD_PERIOD(2400),		///	3
	BAUD_PERIOD(4800),		///	4
	BAUD_PERIOD(9600),		///	5
	BAUD_PERIOD(19200),		///	6
	BAUD_PERIOD(38400),		///	7
	BAUD_PERIOD(57600),		///	8
	BAUD_PERIOD(115200),	///	9
	BAUD_PERIOD(230400),	///	10
	BAUD_PERIOD(460800),	///	11
	BAUD_PERIOD(500000),	///	12
	BAUD_PERIOD(576000),	///	13
	BAUD_PERIOD(921600),	///	14
	1,						///	15 : 4/27/15 : added ULTRA_FAST_MODE
};

static const int uart_data_length[4] = { 5-1, 6-1, 7-1, 8-1 };	///	include start_bit and -1
static const int uart_stop_length[4] = { 3, 2, 2, 2 };	///	-1

void UART::write_command(U16 io_param, int devID, U32 cycle)
{
#if 0
	UART:
	imm15[0:0] : 1 bit : deactivate(1), deactivate(0)
	imm15[4:1] : 4 bits : baud setting
	0 : 50 bps
	1 : 300 bps
	2 : 1200 bps
	3 : 2400 bps
	4 : 4800 bps
	5 : 9600 bps
	6 : 19200 bps
	7 : 38400 bps
	8 : 57600 bps
	9 : 115200 bps
	10 : 230400 bps
	11 : 460800 bps
	12 : 500000 bps
	13 : 576000 bps
	14 : 921600 bps
	15 : clkFreq / 2 bps
	imm15[6:5] : 2 bits : data length
	0 : 5 bits
	1 : 6 bits
	2 : 7 bits
	3 : 8 bits
	imm15[7:7] : 1 bit : stop_bit length
	0 : 1 bit
	1 : 2 bit(6 / 7 / 8 - bit data), 1.5 bit(5 - bit data)
	imm15[10:8] : 3 bits : parity setting
	0 / 2 / 4 / 6 : no parity		->	0 : IOP_None
	1 : odd parity(XNOR)	->	1 : IOP_Odd
	3 : even parity(XOR)	->	3 : IOP_Even
	5 : high parity(1)		->	5 : IOP_High
	7 : low parity(0)		->	7 : IOP_Low
#endif
		int activate = (io_param) & 0x1;
	activated = activate;
	if (activate) {
		int baud_rate_idx   = (io_param >> 1) & 0xf;
		int data_length_idx = (io_param >> 5) & 0x3;
		int stop_length     = (io_param >> 7) & 0x1;
		int parity          = (io_param >> 8) & 0x7;
		///	_m1 --> minus 1
		baud_period_m1 = uart_baud_period[baud_rate_idx];
		bit_count_m1   = uart_data_length[data_length_idx];
		stop_count     = (stop_length) ? uart_stop_length[data_length_idx] : 1;
		parity_type    = ((parity & 0x1) == 0) ? 0 : parity;
	}
	if (!RVDisableMsg) {
		printf("[%6d] ", cycle);
		switch (devID) {
		case 0:		printf("<UART_AXI-0>"); break;
		case 1:		printf("<UART_EXT-0>"); break;
		case 2:		printf("<UART_AXI-1>"); break;
		default:	printf("<UART_EXT-1>"); break;
		}
		if (activate) {
			printf(" (activate) : baud_period(%d), bit_count(%d), stop_count(%d), parity_type(%d)\n",
				baud_period_m1 + 1, bit_count_m1 + 1, stop_count, parity_type);
		}
		else {
			printf(" (deactivate)\n");
		}
	}
	///	reset all status
	rx.reset_port();
	tx.reset_port();
}

U32 UART::read_status()
{
	int rx_error = rx.error;
	int rx_busy  = (rx.state != IOS_Idle);
	int rx_empty = BUF_EMPTY(&rx.buf);
	int rx_full  = BUF_FULL(&rx.buf);
	int tx_busy  = (tx.state != IOS_Idle);
	int tx_empty = BUF_EMPTY(&tx.buf);
	int tx_full  = BUF_FULL(&tx.buf);
	return ((cts << 6) | (rx_error << 5) | ((rx_busy | tx_busy) << 4) | (rx_empty << 3) | (rx_full << 2) | (tx_empty << 1) | (tx_full));
}

void AXI4L::SlaveFSM::fsmRead(AXI4L::CH* axi) {
	axi->setSLRead(out.raddr, out.rdat);
	switch (r_state) {
	case R_Init:
		if (axi->raddr.m.valid && devReadSetup(axi->raddr.m.addr, axi->raddr.m.len)) {
			raddr = axi->raddr.m.addr;
			rsize = axi->raddr.m.size;
			burst_count_r = axi->raddr.m.len;
			burst_end_r = 0;
			out.raddr.set(1);
			raddr_end = 0;
			r_state = R_Addr;
		}
		break;
	case R_Addr: {
		out.raddr.reset();
		if (!axi->raddr.m.valid) { raddr_end = 1; }
		auto d = out.rdat.data;
		if ((out.rdat.valid && burst_end_r) || devRead(&d, raddr, rsize)) {
			out.rdat.set(d, RSP_OK, 1, (burst_count_r == 0));
#if defined(DBG_AXI_SLAVE)  /// 4/18/22
			dbg_rcount++;
#endif
			if (burst_count_r == 0) {
				if (axi->rdat.m.ready) { r_state = R_End; }
				burst_end_r = 1;
			}
			else {
				burst_count_r--;
			}
		}
		break;
	}
	case R_End: /// make sure raddr.valid is deasserted BEFORE going back to R_Init
		out.rdat.reset();
		if (raddr_end || !axi->raddr.m.valid) { r_state = R_Init; }
		break;
	}
}

void AXI4L::SlaveFSM::fsmWrite(AXI4L::CH* axi) {
	switch (w_state) {
	case W_Init:
		if (axi->waddr.m.valid && devWriteSetup(axi->waddr.m.addr)) {
			waddr = axi->waddr.m.addr;
			wsize = axi->waddr.m.size;
			burst_count_w = axi->waddr.m.len;
			out.waddr.set(1);
			waddr_end = 0;
			if (axi->waddr.m.len) {
				out.wdat.set(1);
				w_state = W_AddrDataBurst;
			}
			else {
				w_state = W_AddrData;
			}
		}
		break;
	case W_AddrData:
		out.waddr.reset();
		if (!axi->waddr.m.valid) { waddr_end = 1; }
		if (axi->wdat.m.valid && devWrite(axi->wdat.m.data, waddr, wsize)) {
			wres_end = 0;
			out.wdat.set(1);
			w_state = W_End;
			out.wres.set((mutexError) ? RSP_SLV_ERR : RSP_OK, 1);   /// 6/23/17
			writePending = (writeFlag == 1);
#if defined(DBG_AXI_SLAVE)  /// 4/18/22
			dbg_wcount++;
#endif
		}
		break;
	case W_AddrDataBurst:
		out.waddr.reset();
		if (!axi->waddr.m.valid) { waddr_end = 1; }
		if (axi->wdat.m.valid) {
			if (devWrite(axi->wdat.m.data, waddr, wsize)) {
				wres_end = 0;
				if (burst_count_w == 0) {
					out.wdat.reset();
					w_state = W_End;
					out.wres.set((mutexError) ? RSP_SLV_ERR : RSP_OK, 1);   /// 6/23/17
				}
				else {
					burst_count_w--;
				}
				writePending = (writeFlag == 1);
#if defined(DBG_AXI_SLAVE)  /// 4/18/22
				dbg_wcount++;
#endif
			}
		}
		break;
	case W_End:
		out.wdat.reset();
		{
			BIT nxt_wres_end = wres_end;
			if (axi->wres.m.ready) { wres_end = 1; nxt_wres_end = 1; }
			if ((nxt_wres_end || axi->wres.m.ready) && (waddr_end || !axi->waddr.m.valid)) {
				out.wres.reset();
				w_state = W_Init;
			}
		}
		break;
	}
	axi->setSLWrite(out.waddr, out.wdat, out.wres);
}

BIT UART_AXI4L::devRead(AXI4L::AXI_DType* data, AXI4L::AXI_AType addr, U3 size) {
	BIT retFlag = 0;
	U32 dout = 0;
	if (addr & 0x7) {
		dout = uart.read_status();
		retFlag = 1;
	}
	else if (uart.activated && !BUF_EMPTY(&uart.rx.buf)) {
		readFlag = 1;
		dout = uart.rx.buf.buf[uart.rx.buf.rp];
		retFlag = 1;
	}
	AXI4L::UINT32_to_AXI_DType(&dout, data, addr, size);
	uart_id = (addr >> 6) & 1;	///	uart_id : 0  --> 0x10000000, 1 --> 0x10000040
	return retFlag;
}

////#define DBG_UART_W

BIT UART_AXI4L::devWrite(AXI4L::AXI_DType data, AXI4L::AXI_AType addr, U3 size) {
	/// 6/22/17 : watch out for virtualBus behavior!!!
	if (U32 idx = (addr & 0xf) >> 2) {   /// 6/17/17 : mutex!!!
		switch (idx) {
		case UART_R_CTRL: writeFlag = 2; break;  /// write_command address
		}
	}
	else if (uart.activated && !BUF_FULL(&uart.tx.buf))
	{
		writeFlag = 1;
	}
	if (writeFlag) { AXI4L::AXI_DType_to_UINT32(&data, &writeData, addr, size); }
	uart_id = (addr >> 6) & 1;	///	uart_id : 0  --> 0x10000000, 1 --> 0x10000040
	return (writeFlag != 0);
}

void UART_AXI4L::fsmUser() {
	if (writeFlag == 2) {
		uart.write_command((U16)writeData, uart_id << 1, (U32)cycle);
	}
	else if (uart.activated) {
		uart.do_rx((readFlag) ? IO_ReadData : IO_Idle);
		uart.do_tx((writeFlag) ? IO_WriteData : IO_Idle, (U32)writeData);
#if 0	///	2/22/25
		if (uart.rx.irq) { nxt_intrFlag |= 4; }
		if (uart.tx.irq) { nxt_intrFlag |= 8; }
#endif
	}
#if 1	///	2/22/25
	nxt_intr = uart.rx.irq | uart.tx.irq;
#endif
}

void UART::set_pin(UARTPin* uart_pin)
{
	///	use 2-DFFs to latch uart rx signal
	rx.bit_n = rx_bit_d_n;
	rx_bit_d_n = !uart_pin->rx;
	uart_pin->tx = !tx.bit_n;
	uart_pin->rts = !activated;	///	active-low
	cts = uart_pin->cts;
}

#include <string>

#if 1	///	2/24/25
//#include <conio.h>
/// 5/4/17
#define X_UART_BUF_SIZE	100//60//1000
////const char x_uart_tx_msg[] = "testing uartx_rx!!\n" "\033" "******";
////#define X_UART_TX_MSG_SIZE sizeof(x_uart_tx_msg)
struct X_UART
{
	U8 uidx _T(state);
	std::string rx_buf, tx_buf;
	int tx_pnt, tx_off;
	int rx_event_flag;
	U32 cycle _T(state);
	UART uart;
	U8 state _T(state);
	void update(int rx_data) {
		rx_buf += (char)rx_data;
		if (rx_data == '\n') {
			printf("[%6d] <UART_X-%d.RX> %s", cycle, uidx, rx_buf.c_str());
			rx_buf.clear();
		}
		rx_event_flag = 1;
	}
	int step(BIT tx)
	{
		int cur_state = state;
		if (cur_state == 0) {
			uart.write_command(UART_PARAM | 1, (uidx << 2) | 1, cycle);
			state = 1;
			tx_buf.clear();
			rx_buf.clear();
			//uart.tx.bit_n = 0;
			tx_pnt = 0;
			tx_off = 0;
		}
		else {
			int rx_data, buf_empty = BUF_EMPTY(&uart.rx.buf);
			rx_data = uart.do_rx((buf_empty) ? IO_Idle : IO_ReadData);
			uart.rx.bit_n = !tx;
			if (!buf_empty)
			{
				update(rx_data);
			}
			if (tx_buf.empty() || BUF_FULL(&uart.tx.buf)) {
				uart.do_tx(IO_Idle, 0);
			}
			else {
				uart.do_tx(IO_WriteData, tx_buf[tx_pnt++]);
				if (tx_pnt == (int)tx_buf.size()) {
					tx_buf.clear();
					tx_pnt = 0;
				}
			}
		}
		cycle++;
		return !uart.tx.bit_n;
	}
	void setupStreamInput() {
		//return;
		if (tx_off && !tx_buf.empty()) { return; }
		printf("Set UART StreamInput[#%d] : ", uidx);
		while (1) {
#if 1
			int c = getchar();
#else
			int c = _getch();
#endif
			tx_buf += (char)c;
			if (c == 0x1b) {	///	ESC
				tx_off = 1;
				printf("Stream turned OFF!!\n");
				tx_buf.clear();
				break;
			}
			else if (c == '\n') {
				break;
			}
		}
		tx_pnt = 0;
	}
	void flush_rx() {
		if (!rx_buf.empty()) {
			update('\n');
		}
	}
	void reset(int idx) {
		uidx = idx;
		cycle = 0;
		state = 0;
	}
	int tx_stream_ready() {
		int tx_ready = rx_event_flag && !uart.rx.buf.not_empty && !uart.tx.buf.not_empty && tx_buf.empty() && !tx_off;
		if (tx_ready) {
			rx_event_flag = 0;
		}
		return tx_ready;
	}
};

X_UART x_uart[2];
#else
/// 5/4/17
#define X_UART_BUF_SIZE	60//1000
//const char x_uart_tx_msg0[] = "testing uart_rx!!\n" "******";
const char x_uart_tx_msg[] = "testing uartx_rx!!\n" "\033" "******";
//const char x_uart_tx_esc = x_uart_tx_msg[19];
#define X_UART_TX_MSG_SIZE sizeof(x_uart_tx_msg)
#if 0
static const char tx_msg[] = "testing uart_rx!!\n";
static const char tx_msg_tail[] = "******";
#endif
struct X_UART
{
	U8 uidx _T(state);
	const U8 tx_buf[X_UART_TX_MSG_SIZE];
	U32 cycle _T(state);
	UART uart;
	U8 state _T(state), cpu_mode _T(state);
	U8 rx_buf[X_UART_BUF_SIZE] _T(state);
	U16 rx_pnt _T(state), tx_pnt _T(state), trig_pnt _T(state);
	BIT clear_rx_buf _T(state);
	U32 rx_cycle _T(state);
	U32 mode _T(state);
	void update(int rx_data, int update) {
		int updateCondition = 0;
		updateCondition |= update;
		updateCondition |= (rx_data == '\n');
		updateCondition |= (rx_pnt == X_UART_BUF_SIZE - 2);

		rx_buf[rx_pnt] = (rx_data & 0x100) ? 0 : rx_data;
		if (updateCondition) {
			if (state != 2)
				printf("[%6d] <UART_X-%d.RX> %s", cycle, uidx, rx_buf);
			else
				printf("%s", rx_buf);
			if ((mode & 1) || (rx_data != '\n')) {
				if (rx_data == '\n') mode |= 1;
				state = 2;
			}
			else {
				state = 1;
			}
			rx_pnt = 0;
			clear_rx_buf = 1;
		}
		else { rx_pnt++; }
	}
	int step(BIT tx)
	{
		int cur_state = state;
		if (cur_state == 0 || cur_state == 3) {
			///	4/27/15 : NOTE : baud-rate setting must match with that of the application
#if 1
			uart.write_command(UART_PARAM | 1, (uidx << 2) | 1, cycle);
#else
			uart.write_command((IOP_Even << 8) | (0x0 << 7) | (0x3 << 5) | (UART_BAUD_ULTRA_FAST << 1) | 1, (uidx << 2) | 1, cycle);
#endif
			cpu_mode = (cur_state == 3) ? 1 : 0;
			state = 1;
			rx_pnt = 0;
			tx_pnt = 0;
			uart.tx.bit_n = 0;
		}
		else {
			int rx_data, buf_empty = BUF_EMPTY(&uart.rx.buf);
			rx_data = uart.do_rx((buf_empty) ? IO_Idle : IO_ReadData);
			uart.rx.bit_n = !tx;
			if (!buf_empty)
			{
				update(rx_data, 0);
				rx_cycle = cycle;
			}
			else if (clear_rx_buf) {
				clear_rx_buf = 0;
				for (int i = 0; i < X_UART_BUF_SIZE; ++i) { rx_buf[i] = 0; }
			}
			else if ((rx_pnt != 0) && (rx_cycle + 1000 < cycle)) {
				update(0, 1);
			}
			if (tx_pnt != 0) {
				int buf_full = BUF_FULL(&uart.tx.buf);
				int tx_data = trig_pnt;

				if (!buf_full) {
					tx_pnt = 0;
					trig_pnt = 0xff;
				}
				uart.do_tx((buf_full) ? IO_Idle : IO_WriteData, tx_data);
			}
			else {
				uart.do_tx(IO_Idle, 0);
			}
		}
		cycle++;
		return !uart.tx.bit_n;
	}
	void set_tx(int ch) {
		tx_pnt = 1;
		trig_pnt = ch;
	}
	void flush_rx() {
		if (rx_pnt) {
			rx_buf[rx_pnt] = 0;
			printf("[%6d] <UART_X.RX> msg = %s\n<<<<flushed...\n", cycle, rx_buf);
		}
		rx_pnt = 0;
	}
	void switch_mode() {
		state = 3;
		cycle = 0;
	}
	void set_mode(int mode0) {
		mode = mode0;
		cycle = 0;
		state = 0;
	}
};

X_UART x_uart[2] = {
	{0, "testing uart0_rx!!\n" "\033" "******", 0},
	{1, "testing uart1_rx!!\n" "\033" "******", 0}
};
#endif

#if defined(DBG_BBL)
BIT dbgFlag = 0;
#endif

#if 1
int uart_ext(int uidx, BIT tx)	{ return x_uart[uidx].step(tx); }
void uart_ext_flush_rx()		{
	x_uart[0].flush_rx();
	x_uart[1].flush_rx();
}
void uart_ext_reset() {
	x_uart[0].reset(0);
	x_uart[1].reset(1);
}
void uart_ext_setup_stream_input(int uidx) { x_uart[uidx].setupStreamInput(); }
int uart_ext_tx_stream_ready(int uidx) { return x_uart[uidx].tx_stream_ready(); }
#else
void x_uart_update(int rx_data, int update)
{
	//int updateCondition = (rx_data == 0) || (rx_data == '\n') || (x_uart.rx_pnt == X_UART_BUF_SIZE - 2);
	int updateCondition = 0;
#if	0
	updateCondition |= (rx_data == 0);
#endif
	updateCondition |= update;
	updateCondition |= (rx_data == '\n');
	updateCondition |= (x_uart.rx_pnt == X_UART_BUF_SIZE - 2);

	x_uart.rx_buf[x_uart.rx_pnt] = (rx_data & 0x100) ? 0 : rx_data;
#if	0
	{
		const char trigger_msg[] = "(hit keys...):";
		if (trigger_msg[x_uart.trig_pnt] == rx_data) {
			if (x_uart.trig_pnt == sizeof(trigger_msg) - 2) {   /// detected trigger_msg
				x_uart.state = 2;   /// goto state = 2;
			}
			else { ++x_uart.trig_pnt; }
		}
		else { x_uart.trig_pnt = 0; }
	}
#endif
	if (updateCondition) {
		if (x_uart.state != 2)
			printf("[%6d] <UART_X.RX> msg = %s", x_uart.cycle, x_uart.rx_buf);
		else
			printf("%s", x_uart.rx_buf);
#if defined(DBG_BBL)
#define MSG_MATCH   "[    0.000000] Memory:"
		bool match = strncmp((const char*)x_uart.rx_buf, MSG_MATCH, sizeof(MSG_MATCH) - 1) == 0;
		if (match)
			printf("\nMATCH!!!!\n");
		if (x_uart.state != 2) {
			if (dbgFlag == 0 && match) {
				dbgFlag = 1;
			}
			else if (dbgFlag == 1) {
				dbgFlag = 0;
			}
		}
#endif
		if ((x_uart.mode & 1) || (rx_data != '\n')) {
			if (rx_data == '\n') x_uart.mode |= 1;
			x_uart.state = 2;
		}
		else
			x_uart.state = 1;
		x_uart.rx_pnt = 0;
		x_uart.clear_rx_buf = 1;
	}
	else { x_uart.rx_pnt++; }
}

_C2R_FUNC(1)
int uart_ext(BIT tx)
{
	int cur_state = x_uart.state;
	////#if !defined(__LLVM_C2RTL__)   /// 3/2/17 : move this to top
	////    if (x_uart.cpu_mode == 1 || cur_state == 3) { cpu.cycle++; }
	////#endif
	if (cur_state == 0 || cur_state == 3) {
		///	4/27/15 : NOTE : baud-rate setting must match with that of the application
		x_uart.uart.write_command((IOP_Even << 8) | (0x0 << 7) | (0x3 << 5) | (UART_BAUD_ULTRA_FAST << 1) | 1, 1, x_uart.cycle);
		x_uart.cpu_mode = (cur_state == 3) ? 1 : 0;
		x_uart.state = 1;
		x_uart.rx_pnt = 0;
		x_uart.tx_pnt = 0;
		x_uart.uart.tx.bit_n = 0;
	}
	else {
		int rx_data, buf_empty = BUF_EMPTY(&x_uart.uart.rx.buf);
		rx_data = x_uart.uart.do_rx((buf_empty) ? IO_Idle : IO_ReadData);
		/// 5/6/17 : x_uart.uart.rx.bit_n should be assigned AFTER its reference (inside uart_rx)
		x_uart.uart.rx.bit_n = !tx;
		if (!buf_empty)
		{
			x_uart_update(rx_data, 0);
			x_uart.rx_cycle = x_uart.cycle;
		}
		else if (x_uart.clear_rx_buf) {
			x_uart.clear_rx_buf = 0;
			for (int i = 0; i < X_UART_BUF_SIZE; ++i) { x_uart.rx_buf[i] = 0; }
		}
		else if ((x_uart.rx_pnt != 0) && (x_uart.rx_cycle + 1000 < x_uart.cycle)) {
			x_uart_update(0, 1);
		}
#if	0
		if (cur_state == 2) {
			int buf_full = BUF_FULL(&x_uart.uart.tx.buf);
			UINT8 tx_data = x_uart.tx_buf[x_uart.tx_pnt];
			if (tx_data == 0) {
				tx_data = x_uart.tx_buf[0];
				x_uart.tx_pnt = 0;
			}
			else if (!buf_full)
			{
				x_uart.tx_pnt++;
			}
			x_uart.uart.do_tx((buf_full) ? IO_Idle : IO_WriteData, tx_data);
		}
#endif
		if (x_uart.tx_pnt != 0) {
			int buf_full = BUF_FULL(&x_uart.uart.tx.buf);
#if 1   /// 7/1/20
			int tx_data = x_uart.trig_pnt;
#else
			UINT8 tx_data = x_uart.trig_pnt;
#endif

			if (!buf_full) {
				x_uart.tx_pnt = 0;
				x_uart.trig_pnt = 0xff;
			}

			x_uart.uart.do_tx((buf_full) ? IO_Idle : IO_WriteData, tx_data);
		}
		else {
			x_uart.uart.do_tx(IO_Idle, 0);
		}
	}
	x_uart.cycle++;
	return !x_uart.uart.tx.bit_n;
}

void uart_ext_tx(int ch)
{
	x_uart.tx_pnt = 1;
	x_uart.trig_pnt = ch;
}

void uart_ext_flush_rx()
{
	if (x_uart.rx_pnt) {
		x_uart.rx_buf[x_uart.rx_pnt] = 0;
		printf("[%6d] <UART_X.RX> msg = %s\n<<<<flushed...\n", x_uart.cycle, x_uart.rx_buf);
	}
	x_uart.rx_pnt = 0;
}

void uart_ext_switch_mode()
{
	x_uart.state = 3;
	x_uart.cycle = 0;
}

void uart_ext_set_mode(int mode)
{
	x_uart.mode = mode;
	x_uart.cycle = 0;
}

#endif