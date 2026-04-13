
#if !defined(RV_PROC_IO_H)
#define RV_PROC_IO_H
#include "rv32_io_setting.h"

#include "rv_typedef.h"
#include "../C2Rlib/utils/C2RUtil.h"
#include "stdio.h"

enum IO_OP_TYPE
{
	IO_Idle			= 0x0,	///	000
	///	2/23/15 : changed bit patterns
	IO_ReadData		= 0x4,	///	100
	IO_WriteData	= 0x5,	///	101
	IO_ReadStatus	= 0x6,	///	110
	IO_WriteCommand	= 0x7,	///	111
};

////enum IO_Enum
////{
///////	device ID
////	IO_AXI		= 0x0,
////	IO_SYSCTRL	= 0x1,
////};

#define		IO_DEV_ADDR_MASK	0xFFFFFFF0
#define		DEV_ADDR_MASK		0xF0000000
#if	VCU108
#define		UART_MASK			0x64011000
#define		UART_MASK1			0x64011010
#else
#define		UART_MASK_0			0x10000000
#define		UART_MASK_1			0x10000040
#endif

#define		SYSCTL_MASK			0x10001000

#define USE_GPIO_SLAVE    /// 10/11/22
#define	GPIO_MASK			0x10000080 // GPIO address 

#define USE_SHA256_SLAVE    /// 10/11/22
#define	SHA256_MASK			0x10000100 // SHA256 address 


////#define		MEM_DEV_ADDR_MASK	0x80000000
////#define		EXT_MEM_MASK		0x80000000

#define		RVDM_MASK			0x00000000	///	RV-DebuggerModule : 0x000 - 0xfff

#define		UART_R_DATA				0	///	qemu: #define		UART_R_RXTX				0
#define		UART_R_CTRL				1	///	qemu: #define		UART_R_DIV				1
#define     UART_R_LOCK             2
#define     UART_R_UNLOCK           3
#define		UART_R_MAX				4


#define MAX_IO_BUF_SIZE	3

int nxt_pnt(int cur_pnt);

template<typename T, int bw>
struct IOBuf
{
    BIT		not_empty _T(state), full _T(state);
    U4	rp _T(state), wp _T(state);
    T	buf[MAX_IO_BUF_SIZE] _BWT(bw) _T(state);	///	reg-file
    void reset() { rp = 0; wp = 0; full = 0; not_empty = 0; }
    void update(int rflag, int wflag) {
        if (rflag || wflag) {
            int nrp = (rflag) ? nxt_pnt(rp) : rp;
            int nwp = (wflag) ? nxt_pnt(wp) : wp;
            not_empty = (wflag) ? 1 : (nrp != nwp);
            full = (rflag) ? 0 : (nrp == nwp);
            rp = nrp;
            wp = nwp;
        }
    }
};

struct IO;

#if !defined(AXI_DATA_WIDTH)
#define AXI_DATA_WIDTH  32
#endif

#define SEPARATE_AXI_DTYPE_ATYPE	///	11/19/24

struct AXI4L
{
	enum TransferSize { // in bytes
		TRANSFER_SIZE_1   = 0,
		TRANSFER_SIZE_2   = 1,
		TRANSFER_SIZE_4   = 2,
		TRANSFER_SIZE_8   = 3,
		TRANSFER_SIZE_16  = 4,
		TRANSFER_SIZE_32  = 5,
		TRANSFER_SIZE_64  = 6,
		TRANSFER_SIZE_128 = 7,
	};
	enum RespEnum { RSP_OK = 0, RSP_EXOK = 1, RSP_SLV_ERR = 2, RSP_DEC_ERR = 3, };
    typedef U32 AXI_DType;
#define AXI_ADDR_POS_MASK   0x3
	typedef U32 AXI_AType;
    static void getBitPosMask32b(unsigned &bitPos, unsigned &bitMask, AXI_AType addr, U3 size) {
        bitPos = (addr & AXI_ADDR_POS_MASK) << 3;
        switch (size) {
        case 0: bitMask = 0xff; break;  /// TRANSFER_SIZE_1 : 1 byte
        case 1: bitMask = 0xffff; break; /// TRANSFER_SIZE_2 : 2 bytes
        default: bitMask = 0xffffffff; break; /// TRANSFER_SIZE_4 : 4 bytes;
        }
    }
    static void AXI_DType_to_UINT32(const AXI_DType * axi_data, U32 * ud32, AXI_AType addr, U3 size) {
        if (sizeof(AXI_DType) == sizeof(U32)) {
            *ud32 = (U32)(*axi_data);
        } else {
            unsigned bitPos, bitMask;
            getBitPosMask32b(bitPos, bitMask, addr, size);
            *ud32 = (U32)(((*axi_data) >> bitPos) & bitMask);
        }
    }
    static void UINT32_to_AXI_DType(const U32 * ud32, AXI_DType * axi_data, AXI_AType addr, U3 size) {
        if (sizeof(AXI_DType) == sizeof(U32)) {
            *axi_data = (AXI_DType)(*ud32);
        } else {
            unsigned bitPos, bitMask;
            getBitPosMask32b(bitPos, bitMask, addr, size);
            *axi_data = ((((AXI_DType)(*ud32)) & bitMask) << bitPos);
        }
    }
	struct CH { // channel
		struct ADDR { /// addr-read, addr-write
			struct MA {
                AXI_AType addr; U3 size; BIT valid; U4 len; U3 prot;
                void set(AXI_AType a, U3 s, BIT v, U4 l, U3 p) { addr = a; size = s; valid = v; len = l; prot = p; }
                void set(MA &m) { set(m.addr, m.size, m.valid, m.len, m.prot); }
                void reset() { set(0, TRANSFER_SIZE_4, 0, 0, 7); }
			} m;
			struct SL {
				BIT ready;
                void set(BIT r) { ready = r; }
                void set(SL &s) { set(s.ready); }
                void reset() { set(0); }
			} s;
		} raddr, waddr;
		struct RDAT {  /// read-data
			struct MA {
				BIT ready;
                void set(BIT r) { ready = r; }
                void set(MA &m) { set(m.ready); }
                void reset() { set(0); }
			} m;
			struct SL {
                AXI_DType data; U2 resp; BIT valid, last;
                void set(AXI_DType d, U2 r, BIT v, BIT l) { data = d; valid = v; resp = r; last = l; }
                void set(SL &s) { set(s.data, s.resp, s.valid, s.last); }
                void reset() { set(0, 0, 0, 0); }
			} s;
		} rdat;
		struct WDAT {  /// write-data
			struct MA {
                AXI_DType data; U4 strobe; BIT valid, last;
                void set(AXI_DType d, U4 s, BIT v, BIT l) { data = d; strobe = s; valid = v; last = l; }
                void set(MA &m) { set(m.data, m.strobe, m.valid, m.last); }
                void reset() { set(0, 0, 0, 0); }
			} m;
			struct SL {
				BIT ready;
                void set(BIT r) { ready = r; }
                void set(SL &s) { set(s.ready); }
                void reset() { set(0); }
			} s;
		} wdat;
		struct WRES {  /// write-resp
			struct MA {
				BIT ready;
                void set(BIT r) { ready = r; }
                void set(MA &m) { set(m.ready); }
                void reset() { set(0); }
			} m;
			struct SL {
				U2 resp; BIT valid;
                void set(U2 r, BIT v) { resp = r; valid = v; }
                void set(SL &s) { set(s.resp, s.valid); }
                void reset() { set(0, 0); }
			} s;
		} wres;
        void setMARead(ADDR::MA &ra, RDAT::MA &rd) { raddr.m.set(ra); rdat.m.set(rd); }
        void setMAWrite(ADDR::MA &wa, WDAT::MA &wd, WRES::MA &wr) { waddr.m.set(wa); wdat.m.set(wd); wres.m.set(wr); }
        void setMARead(CH &c) { raddr.m.set(c.raddr.m); rdat.m.set(c.rdat.m); }
        void setMAWrite(CH &c) { waddr.m.set(c.waddr.m); wdat.m.set(c.wdat.m); wres.m.set(c.wres.m); }
        void resetMA() { resetMARead(); resetMAWrite(); }
        void resetMARead() { raddr.m.reset(); rdat.m.reset(); }
        void resetMAWrite() { waddr.m.reset(); wdat.m.reset(); wres.m.reset(); }
        void resetSL() { resetSLRead(); resetSLWrite(); }
        void setSLRead(ADDR::SL &ra, RDAT::SL &rd) { raddr.s.set(ra); rdat.s.set(rd); }
        void setSLWrite(ADDR::SL &wa, WDAT::SL &wd, WRES::SL &wr) { waddr.s.set(wa); wdat.s.set(wd); wres.s.set(wr); }
        void setSLRead(CH &c) { raddr.s.set(c.raddr.s); rdat.s.set(c.rdat.s); }
        void setSLWrite(CH &c) { waddr.s.set(c.waddr.s); wdat.s.set(c.wdat.s); wres.s.set(c.wres.s); }
        void resetSLRead() { raddr.s.reset(); rdat.s.reset(); }
        void resetSLWrite() { waddr.s.reset(); wdat.s.reset(); wres.s.reset(); }
		static void connectRCh(CH &mc, CH &sc) { sc.setMARead(mc);  mc.setSLRead(sc); }
		static void connectWCh(CH &mc, CH &sc) { sc.setMAWrite(mc); mc.setSLWrite(sc); }
	} _T(direct_signal);    /// struct CH
	struct PORT {   /// for AXI-FSM IOs : split each MA/SL channels and merge as MA/SL PORTs
		struct MA { /// for AXI-Master-FSM outPorts
			CH::ADDR::MA raddr, waddr; CH::RDAT::MA rdat; CH::WDAT::MA wdat; CH::WRES::MA wres;
            void setRead(AXI_AType ra_addr, U3 size, BIT ra_valid, BIT rd_ready, U4 len, U3 prot)
			{ raddr.set(ra_addr, size, ra_valid, len, prot); rdat.set(rd_ready); }
            void setWrite(AXI_AType wa_addr, U2 size, BIT wa_valid, AXI_DType wd_data, U4 wd_strobe,
                BIT wd_valid, BIT wd_last, BIT wr_ready, U4 len, U3 prot)
			{ waddr.set(wa_addr, size, wa_valid, len, prot); wdat.set(wd_data, wd_strobe, wd_valid, wd_last); wres.set(wr_ready); }
		};
		struct SL { /// for AXI-Slave-FSM outPorts, AXI-Master-FSM inPorts
			CH::ADDR::SL raddr, waddr; CH::RDAT::SL rdat; CH::WDAT::SL wdat; CH::WRES::SL wres;
            void resetRead() { raddr.reset(); rdat.reset(); }
            void resetWrite() { waddr.reset(); wdat.reset(); wres.reset(); }
		};
	};
	enum DeviceID {
		DI_INVALID = 0xffff,
		DI_UART_0 = 0x0000,
		DI_UART_1,
		DI_GPIO,
		DI_DEV_COUNT,
	};
	static unsigned decodeAddr(AXI_DType addr) {
		switch (addr & IO_DEV_ADDR_MASK) {
		case UART_MASK_0:	return DI_UART_0;
		case UART_MASK_1:	return DI_UART_1;
		case GPIO_MASK:		return DI_GPIO;
		}
		return 0;   /// 12/7/21
	}
	struct MasterFSM;
	struct SlaveFSM;
	struct VirtualBUS {
		static VirtualBUS *virtualBus;
		int dummy;  /// sorry... we need at least 1 member data in base class....
		void showInterruptCount() {}
	};
	struct MasterFSM {
		PORT::MA out _T(state); /// for driving MA-outputs
		PORT::SL in _T(state);  /// for latching MA-inputs
		enum RWState { RW_Init, RW_RAddr, RW_WAddr, };
		enum AXIOp { AXI_Idle, AXI_Read, AXI_Write, };	///	2/13/25
		U3	rw_state _T(state);
		U4	burst_count _T(state);
		U4	burst_length;
		U3	mem_prot;
		bool fsm(U2 op, CH *axi, AXI_AType addr, U3 size, AXI_DType dout, AXI_DType *din, BIT *data_latched);
        unsigned getReadChannelStat() { return ((in.rdat.valid << 1) | in.raddr.ready); }
        unsigned getWriteChannelStat() { return ((in.wres.valid << 2) | (in.wdat.ready << 1) | in.waddr.ready); }
	};
#define TOTAL_INTERRUPTS    5   /// # of interrupts per CPU
#define INTERRUPT_MASK      ((1 << TOTAL_INTERRUPTS) - 1)

#define DBG_AXI_SLAVE
	struct SlaveFSM {
#define INIT_VIRTUAL_BUS_SLAVE(dev, axi)    /// nothing...
		PORT::SL out _T(state);
		enum ReadState { R_Init, R_Addr, R_End, };
		enum WriteState { W_Init, W_AddrData, W_AddrDataBurst, W_End, };
		U3 r_state _T(state), w_state _T(state);
		AXI_AType raddr _T(state), waddr _T(state);
		U3 rsize _T(state), wsize _T(state);
		BIT raddr_end _T(state), waddr_end _T(state), wres_end _T(state); /// indicate that raddr/waddr are released by master
		BIT writePending _T(state);   /// need to indicate difference of write-operation and mutex
		BIT intr _T(state), nxt_intr;	///	2/22/25
		int readFlag, writeFlag;
        U32 writeData;
        U32 cycle _T(state);
		BIT mutexError;
		U4	burst_count_r _T(state), burst_count_w _T(state);
		BIT		burst_end_r; _T(state)
#if defined(DBG_AXI_SLAVE)  /// 4/18/22
        U32   dbg_rcount _T(state), dbg_wcount _T(state);
#endif
		/// methods
		void fsmRead(CH *axi);
		void fsmWrite(CH *axi);
		void fsm(CH *axi) {
			nxt_intr = 0;	///	2/22/25
			preFsmUser();
			fsmRead(axi);
			fsmWrite(axi);
			fsmUser();
			intr = nxt_intr;	///	2/22/25
			cycle++;
		}
		virtual BIT devRead(AXI_DType *data, AXI_AType addr, U3 size) = 0;
        virtual BIT devReadSetup(AXI_AType addr, unsigned blen) { return 1; }
		virtual BIT devWrite(AXI_DType data, AXI_AType addr, U3 size) = 0;
		virtual BIT devWriteSetup(AXI_AType addr) { return 1; }
		virtual void preFsmUser() { readFlag = 0; writeFlag = 0; writeData = 0; mutexError = 0; };
		virtual void fsmUser() {};
    };
	template <int MC, int SC> struct BUS : VirtualBUS {
		CH m_ch[MC], s_ch[SC];
		void resetAllChannelSinks() {
			for (unsigned i = 0; i < SC; ++i) { s_ch[i].resetMA(); }
			for (unsigned i = 0; i < MC; ++i) { m_ch[i].resetSL(); }
		}
	};
	struct MasterStatus {   /// ok, another problem... C2R crashes if MasterStatus is declared under BUS
							/// put _T(state) inside MasterStatus/SlaveStatus
		U8   slaveID _T(state); ////    0xffff : invalid device
		BIT     active _T(state);
		BIT        granted;     /// wire
		BIT        requested;   /// wire
		U8      reqSlaveID;  /// wire
		void set(U8 sID, BIT a) { slaveID = sID; active = a; }
		void set(MasterStatus &ms) { set(ms.slaveID, ms.active); }
		void reset() { slaveID = 0; active = 0; }
		void resetFlags() { granted = 0; requested = 0; reqSlaveID = 0; }
	};
	template <int MC> struct SlaveStatus {
		U8    masterID _T(state);
		BIT      active _T(state);
	};
	template <int MC, int SC> struct CTRL {
		/// first try... assume MA_COUNT == 1; (no arbitration)
		/// put _T(state) inside MasterStatus/SlaveStatus
		MasterStatus MRStat[MC], MWStat[MC];
		SlaveStatus<MC> SRStat[SC], SWStat[SC];
		U4     roundRobinRVal _T(state), roundRobinWVal _T(state);
		void checkMRReq(BUS<MC, SC> *bus, int midx) {
			CH &mc = bus->m_ch[midx];
			MasterStatus &rs = MRStat[midx];
			rs.resetFlags();
			if (!rs.active && mc.raddr.m.valid) {
				U32 sidx = decodeAddr(mc.raddr.m.addr);
				if (!SRStat[sidx].active && !bus->s_ch[sidx].rdat.s.valid) {
					rs.requested = 1;
					rs.reqSlaveID = sidx;
				}
			}
		}
		void checkMWReq(BUS<MC, SC> *bus, int midx) {
			CH &mc = bus->m_ch[midx];
			MasterStatus &ws = MWStat[midx];
			ws.resetFlags();
			if (!ws.active && mc.waddr.m.valid) {
				unsigned sidx = decodeAddr(mc.waddr.m.addr);
				if (!SWStat[sidx].active && !bus->s_ch[sidx].wres.s.valid) {
					ws.requested = 1;
					ws.reqSlaveID = sidx;
				}
			}
		}
		BIT arbitrate(U4 sidx, MasterStatus *ms, U4 rrVal) {
			BIT flag = 0;
			for (int i = 0; i < MC; ++i) {  /// 1st round
				if (i >= rrVal && !flag && ms[i].requested && ms[i].reqSlaveID == sidx) {
					ms[i].granted = 1; flag = 1;
				}
			}
			for (int i = 0; i < MC; ++i) {  /// 2nd round
				if (!flag && ms[i].requested && ms[i].reqSlaveID == sidx) {
					ms[i].granted = 1; flag = 1;
				}
			}
			return flag;
		}
		void arbitrateMR() {
			int granted = 0;
			for (int i = 0; i < SC; ++i) { granted |= arbitrate(i, MRStat, roundRobinRVal); }
			if (granted) { roundRobinRVal = (roundRobinRVal < MC - 1) ? roundRobinRVal + 1 : 0; }
		}
		void arbitrateMW() {
			int granted = 0;
			for (int i = 0; i < SC; ++i) { granted |= arbitrate(i, MWStat, roundRobinWVal); }
			if (granted) { roundRobinWVal = (roundRobinWVal < MC - 1) ? roundRobinWVal + 1 : 0; }
		}
		_C2R_FUNC(1)
		void connectChannel(BUS<MC, SC> *bus) {
			bus->resetAllChannelSinks();
			int i, j;
			for (i = 0; i < MC; ++i) { checkMRReq(bus, i); checkMWReq(bus, i); }
			arbitrateMR(); arbitrateMW();
			for (i = 0; i < MC; ++i) {  /// i : const (due to loop-unrolling), j : variable
				if (updateMRStat(bus, i, &j)) { CH::connectRCh(bus->m_ch[i], bus->s_ch[j]); }
			}
			for (i = 0; i < MC; ++i) {
				if (updateMWStat(bus, i, &j)) { CH::connectWCh(bus->m_ch[i], bus->s_ch[j]); }
			}
		}
		BIT updateMRStat(BUS<MC, SC> *bus, int midx, int *sidx) {
			CH &mc = bus->m_ch[midx];
			MasterStatus &rs = MRStat[midx];
			*sidx = rs.slaveID;
			if (rs.granted) {
				*sidx = rs.reqSlaveID;
				rs.set(*sidx, 1);
				SRStat[*sidx].active = 1;
				SRStat[*sidx].masterID = midx;
				return 1;
			}
			if (rs.active && mc.rdat.m.ready && bus->s_ch[*sidx].rdat.s.last) {
				rs.reset();  SRStat[*sidx].active = 0; return 1;
			}
			else { return rs.active; }
		}
		BIT updateMWStat(BUS<MC, SC> *bus, int midx, int *sidx) {
			CH &mc = bus->m_ch[midx];
			MasterStatus &ws = MWStat[midx];
			*sidx = ws.slaveID;
			if (ws.granted) {
				*sidx = ws.reqSlaveID;
				ws.set(*sidx, 1);
				SWStat[*sidx].active = 1;
				SWStat[*sidx].masterID = midx;
				return 1;
			}
			if (ws.active && mc.wres.m.ready && bus->s_ch[*sidx].wres.s.valid) {
				ws.reset(); SWStat[*sidx].active = 0;
				return 1;
			}
			else { return ws.active; }
		}
	};
#if !defined(GCC_TEMPLATE_PATCH)
    // 4/21/22 : MC = 1, SC = 1 case
    template<> struct CTRL<1, 1> {
        _C2R_FUNC(1) void connectChannel(BUS<1, 1> *bus) {
            bus->resetAllChannelSinks();
            CH::connectRCh(bus->m_ch[0], bus->s_ch[0]);
            CH::connectWCh(bus->m_ch[0], bus->s_ch[0]);
        }
    };
#endif
};

struct RV_AXI4_Master : AXI4L::MasterFSM {
	U32 data_in;    /// from slave
	U2	pending_op _T(state);
	BIT	active, stalled, data_latched;
	void fsm(U2 io_op, U2 io_size, U32 addr, U32 wdata, AXI4L::CH* axi);
};

#define MODIFY_SYSCTRL_ADDR

#define DETECT_INTR_POS_EDGE

struct SYSCTRL {
	U32	ext_irq_pending _T(state);
	U32	ext_irq_enable _T(state);

#if defined(DETECT_INTR_POS_EDGE)
	unsigned d_intr[3] _T(state) _BW(7);
#endif

	BIT	meip, mtip;
	// timer
	U32	mtime_l _T(state), mtime_h _T(state);
	U32	mtimecmp_l _T(state), mtimecmp_h _T(state);

	U4	divider _T(state);
	//
	U32	data_in;
	BIT	active, data_latched;
	void fsm(U2 io_op, U5 addr, U32 wdata, U32 intr, U32 interal_irq_clear_mask);
	void incr_timer() {
		auto	n_mtime_l = mtime_l;
		auto	n_mtime_h = mtime_h;

		if (n_mtime_l == 0xffffffff) {
			n_mtime_l = 0;
			n_mtime_h++;
		}
		else {
			n_mtime_l++;
		}
		mtime_l = n_mtime_l;
		mtime_h = n_mtime_h;
	}
};

struct IO
{
	SYSCTRL			sysctrl;
	RV_AXI4_Master	axim;
	enum AXI_State { S_Idle, };
	U32 data_in, latched_data _T(state);    /// from slave
	BIT	active, stalled, data_latched;
	void fsm(U2 io_op, U2 io_size, U32 addr, U32 wdata, AXI4L::CH* axi, U32 intr, U32 internal_irq_clear_mask);
};

enum IO_OP_STATE
{
	IOS_Idle = 0,
	IOS_Start = 1,
	IOS_Parity = 2,
	IOS_Stop = 3,
	IOS_Ack = 4,
};

#define ESC	0x1b	///	ESC code


struct UART_PORT
{
	IOBuf<unsigned char, 8> buf;
	U16	word _T(state);
	U2	state _T(state);
	BIT bit_n _T(state), parity _T(state), error _T(state);
	U4	bit_pos _T(state);
	U16	tick _T(state);
	BIT stalled, irq;
	void reset_port() { state = IOS_Idle; error = 0; tick = 0; buf.reset(); bit_n = 0; }
	void activate(U32 w) { word = w; state = IOS_Start; bit_pos = 0; parity = 0; }
};

struct UARTPin
{
	BIT tx _T(direct_signal), rx _T(direct_signal);		///	tx : output, rx : input
	BIT rts _T(direct_signal), cts _T(direct_signal);	///	rts (Request To Send) : output, cts (Clear To Send) : input
};

struct UART {
	UART_PORT	rx, tx;
	BIT			activated _T(state), rx_bit_d_n _T(state), rx_bit_samp _T(state), cts _T(state);
	U16			baud_period_m1 _T(state);
	U4			bit_count_m1 _T(state);
	U2			stop_count _T(state);
	U3			parity_type _T(state), pending_op _T(state);
	U32			data_in;
	BIT			active, stalled, data_latched;
	void fsm(BIT cancel_io_insn, U3 io_op_in, U32 src0, int devID);
	U32 read_status();
	void write_command(U16 io_param, int devID, U32 cycle);
	void do_tx(U2 io_op, U32 src0);
	U32 do_rx(U2 io_op);
	U32 adjust_word();
	BIT parity_bit(BIT parity);
	void set_pin(UARTPin* uart_pin);
};

struct UART_AXI4L : AXI4L::SlaveFSM {
	UART uart;
	unsigned uart_id;
	BIT devRead(AXI4L::AXI_DType* data, AXI4L::AXI_AType addr, U3 size);
	BIT devWrite(AXI4L::AXI_DType data, AXI4L::AXI_AType addr, U3 size);
	void fsmUser();
	_C2R_FUNC(1)
		void step(AXI4L::CH* axi, UARTPin* uart_pin, BIT* intr_out) {
		uart_id = 0;
		uart.set_pin(uart_pin);
		*intr_out = intr;
		fsm(axi);
	}
};

#define DBG_GPIO

#if defined(DBG_GPIO)
#include <stdio.h>
#endif

struct DebugInterface {
	U32 data;
	U4  rv_state;
	U16 idx;
	BIT wfi;
};

///	2/26/25 : Nexys-A7-100T board 
struct GPIOPin {	///	2 x 32-bits
	//AXI4L::AXI_DType dout[2], din[2];
	U16 sw_in;		///	input : SW[15:0]
	U8  seg7_dout;	/// output : { ca, cb, cc, cd, ce, cf, cg, da } : active low
	U8	seg7_an;	///	output : AN[7:0] : active low
	U15 led_out;	///	output : LED[14:0]
} _T(direct_signal);

struct GPIO_AXI4L : AXI4L::SlaveFSM {
	/// 7-seg refresh rate : 1ms - 16ms --> set to 1ms @ 100MHz
	///	--> refresh period = 10^8 / 10^3 = 10^5 = 10,000 clks --> (1 << 13) = 8192 clks
#define SEG7_REFRESH_PERIOD	(1 << 13)
#define SEG7_REFRESH_MASK	(SEG7_REFRESH_PERIOD - 1)
	U32 dout _T(state), dbg_dout _T(state);
	U32 dout_7seg[2] _T(state);
	U32 dout_led _T(state);
	U16 din _T(state);
	U32 writeDataGPIO;
	U32 clk _T(state);
	//U12 refresh_phase _T(state);
	U3 digit_idx _T(state);	///	[2:0]
	U8 an _T(state), digit _T(state);
	BIT digit_active _T(state);
	U2 digit_mode _T(state), cur_digit_mode;
	unsigned mode;
	BIT devRead(AXI4L::AXI_DType* data, AXI4L::AXI_AType addr, U3 size) {
		readFlag = 1;
		*data = din;
#if defined(DBG_GPIO)   /// 7/22/23
		printf("[%6d] gpio.din = (%08x)\n", clk, din);
#endif
		return 1;
	}
	BIT devWrite(AXI4L::AXI_DType data, AXI4L::AXI_AType addr, U3 size) {
		mode = ((addr & 0x7f) >> 2);
		writeFlag = 1;
		writeDataGPIO = data;
#if defined(DBG_GPIO)   /// 7/22/23
		printf("[%6d] gpio.dout[%d] = (%08x)\n", clk, mode, writeDataGPIO);
#endif
		return 1;
	}
	void fsmUser() {
		if (writeFlag) {
			switch (mode) {
				case 0: dout = writeDataGPIO; break;
				case 1: dout_7seg[0] = writeDataGPIO; break;
				case 2: dout_7seg[1] = writeDataGPIO; break;
				default: dout_led = writeDataGPIO; break;
			}
			if (mode <= 2) {
				digit_mode = (mode == 0) ? 0 : 1;
			}
			digit_active = 1;
		}
	}
	U8 hex2seg7(U4 hex_val) {
		switch (hex_val) {
		case 0x0: return S7_0;
		case 0x1: return S7_1;
		case 0x2: return S7_2;
		case 0x3: return S7_3;
		case 0x4: return S7_4;
		case 0x5: return S7_5;
		case 0x6: return S7_6;
		case 0x7: return S7_7;
		case 0x8: return S7_8;
		case 0x9: return S7_9;
		case 0xa: return S7_a;	///	A
		case 0xb: return S7_b;	///	b
		case 0xc: return S7_c;	///	C
		case 0xd: return S7_d;	///	d
		case 0xe: return S7_e;	///	E
		default:  return S7_f;	///	F
		}
	}
	void update_refresh() {
		U3 nxt_idx = (digit_idx + 1) & 7;	///	hex-digit index : 0 - 7
		if (cur_digit_mode == 0) {
			digit = hex2seg7((dout >> (nxt_idx << 2)) & 0xf);
		}
		else if (cur_digit_mode == 2) {
			digit = hex2seg7((dbg_dout >> (nxt_idx << 2)) & 0xf);
		}
		else {
			int pos = (nxt_idx >> 2) & 1;
			int shf = (nxt_idx & 3) << 3;
			digit = (dout_7seg[pos] >> shf) & 0xff;
		}
		an = (1 << nxt_idx);
		digit_idx = nxt_idx;
		//refresh_phase ++;
	}
	_C2R_FUNC(1)
	void step(AXI4L::CH* axi, GPIOPin* gpio_pin, DebugInterface* dbg) {
		gpio_pin->seg7_an = ~an;
		gpio_pin->seg7_dout = ~digit;
		gpio_pin->led_out = ((dbg->wfi) ? dbg->rv_state : dout_led) & 0x7fff;
#if 1
		cur_digit_mode = digit_mode;
		if (dbg->wfi) {
			cur_digit_mode = ((gpio_pin->sw_in >> 6) & 1) ? digit_mode : 2;
			dbg_dout = dbg->data;
			dbg->idx = din & 0xffff;
		}
#else
		if (dbg->wfi) {
#if 1
			digit_mode = ((gpio_pin->sw_in >> 7) & 1);
#else
			digit_mode = 2;
#endif
			dbg_dout = dbg->data;
			dbg->idx = din & 0xffff;
		}
#endif
#if 1
		if ((digit_active || dbg->wfi) && (clk & SEG7_REFRESH_MASK) == 0) { update_refresh(); }
#else
		if (digit_active && (clk & SEG7_REFRESH_MASK) == 0) { update_refresh(); }
#endif
		writeDataGPIO = 0;
		fsm(axi);
		din = gpio_pin->sw_in;
		clk++;
	}
};

template <int INTR_SIZE> struct InterruptPin {
	BIT p[INTR_SIZE];
	void set(unsigned mask) {
		for (unsigned i = 0; i < INTR_SIZE; ++i) {
			if (mask & (1 << i)) { p[i] = 1; }
		}
	}
	void clear(unsigned mask) {
		for (unsigned i = 0; i < INTR_SIZE; ++i) {
			if (mask & (1 << i)) { p[i] = 0; }
		}
	}
	unsigned vec() {
		unsigned intr_vec = 0;
		for (unsigned i = 0; i < INTR_SIZE; ++i) {
			intr_vec |= (p[i] << i);
		}
		return intr_vec;
	}
};

int uart_ext(int uidx, BIT tx);
void uart_ext_flush_rx();
void uart_ext_reset();
void uart_ext_setup_stream_input(int uidx);
int uart_ext_tx_stream_ready(int uidx);
#endif
