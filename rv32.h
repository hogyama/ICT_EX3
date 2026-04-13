#if !defined(RV32_H)
#define RV32_H

#if 0
-----------------------------------------------------------
Arithmetic instructions:
R-type : opcode[6:0](0110011), rd[11:7], func3[14:12], rs1[19:15], rs2[24:20], funct7[31:25]
ADD  : funct3(000), funct7(0000000)
SUB  : funct3(000), funct7(0100000)
SLT  : funct3(010), funct7(0000000) : Less-Than Signed 
SLTU : funct3(011), funct7(0000000) : Less-Than Unsigned
AND  : funct3(111), funct7(0000000)
OR   : funct3(110), funct7(0000000)
XOR  : funct3(100), funct7(0000000)
SLL  : funct3(001), funct7(0000000)
SRL  : funct3(101), funct7(0000000) : Shift-Right Logical
SRA  : funct3(101), funct7(0100000) : Shift-Right Arithmetic
I-type : opcode[6:0](0010011), rd[11:7], func3[14:12], rs1[19:15], {20{imm[31]},imm[31:20]}
ADDI  : funct3(000)
SLTI  : funct3(010) : Less-Than Signed 
SLTIU : funct3(011) : Less-Than Unsigned
ANDI  : funct3(111)
ORI   : funct3(110)
XORI  : funct3(100)
SLLI  : funct3(001), funct7(0000000)
SRLI  : funct3(101), funct7(0000000) : Shift-Right Logical
SRAI  : funct3(101), funct7(0100000) : Shift-Right Arithmetic
-----------------------------------------------------------
20-bit Upper Immediate Instructions:
U-type : opcode[6:0], rd[11:7], {imm[31:12],12'b0}
LUI   : opcode(0110111) : Load Upper Immediate
AUIPC : opcode(0010111) : Add Upper Immediate to PC
-----------------------------------------------------------
Load Instructions:
I-type : opcode[6:0](0000011), rd[11:7], func3[14:12], rs1[19:15], {20{imm[31]},imm[31:20]}
LB  : funct3(000) : load byte
LH  : funct3(001) : load half-word
LW  : funct3(010) : load word
LBU : funct3(100) : load byte unsigned
LHU : funct3(101) : load half-word unsigned
---------------------------------------------------------- -
Store Instructions :
S-type : opcode[6:0](0100011), func3[14:12], rs1[19:15], rs2[24:20], {20{imm[31]},imm[31:25],imm[11:7]}
SB : funct3(000) : store byte
SH : funct3(001) : store half-word
SW : funct3(010) : store word
-----------------------------------------------------------
Jump Instructions:
JAL  : J-type : opcode[6:0](1101111), rd[11:7], func3[14:12], rs1[19:15], {20{imm[31]},imm[31:20]}
JALR : I-type : opcode[6:0](1100111), rd[11:7], func3[14:12](000), rs1[19:15], {20{imm[31]},imm[19:12],imm[20],imm[30:21],1'b0}
-----------------------------------------------------------
Branch Instructions :
B-type : opcode[6:0](1100011), func3[14:12], rs1[19:15], rs2[24:20], {20{imm[31]},imm[7],imm[30:25],imm[11:8],1'b0}
BEQ  : funct3(000) : branch Equal
BNE  : funct3(001) : branch Not-Equal
BLT  : funct3(100) : branch Less-Then Signed
BGE  : funct3(101) : branch Greater-or-Equal Signed
BLTU : funct3(110) : branch Less-Then Unsigned
BGEU : funct3(111) : branch Greater-or-Equal Unsigned
-----------------------------------------------------------
Special Instructions :
WFI : opcode[6:0](1110011), inst[31:20](0001 0000 0101), inst[19:7](13'b0)
#endif

#include "rv_typedef.h"

#define MEM_BYTE_POS_BW 2                       /// MEM_BYTE_POS = 1 << 2 = 4;
//#define MEM_ADDR_BITS   (24 - MEM_BYTE_POS_BW)  /// MEM_ADDR_BITS = 24 - 2 = 22;
#define MEM_ADDR_BITS   (18 - MEM_BYTE_POS_BW)  /// MEM_ADDR_BITS = 18 - 2 = 16;
#define MEM_SIZE        (1 << MEM_ADDR_BITS)    /// MEM_SIZE = 1 << 16 words
#define MEM_SIZE_BYTES  (MEM_SIZE << MEM_BYTE_POS_BW)

#define GET_FLD(d, b, w)            (((d) >> (b)) & ((1 << (w)) - 1))
#define IR_FLD(b, w)                GET_FLD(ir, b, w)   /// ((ir >> (b)) & ((1 << (w)) - 1))
#define IR_SEXT(b)                  ((S32)(ir & (1 << 31))) >> (b)

#if 0
x0	    zero	hardwired zero	-
x1	    ra	    return address	Caller
x2	    sp	    stack pointer	Callee
x3	    gp	    global pointer	-
x4	    tp	    thread pointer	-
x5-7	t0-2	temporary registers	Caller
x8	    s0 / fp	saved register / frame pointer	Callee
x9	    s1	    saved register	Callee
x10-11	a0-1	function arguments / return values	Caller
x12-17	a2-7	function arguments	Caller
x18-27	s2-11	saved registers	Callee
x28-31	t3-6	temporary registers
#endif

#include "stdio.h"
#include <map>
#include <string>

struct SYSCTRL;

struct RV32I {
    struct SymbolTable {
        std::map<unsigned, std::string> symTable;
        void printTable();
        void printSymbol(U32 pc);
        void printSymbolName(U32 pc);
    };
    static FILE* fp;
    struct LogFile {
        LogFile(const char* fname, int type) { open(fname, type); }
        ~LogFile() { close(); }
        void open(const char* fname, int type);
        void close();
        static std::string extractFilename(const char* fname);
    };

    #define ENABLE_MD /// mult/div

    enum CodeEnum {
        /// opcode[6:0]
        I_COMP  = 0b0110011,
        I_COMPI = 0b0010011,
        I_LUI   = 0b0110111,
        I_AUIPC = 0b0010111,
        I_LD    = 0b0000011,
        I_ST    = 0b0100011,
        I_JAL   = 0b1101111,
        I_JALR  = 0b1100111,
        I_BR    = 0b1100011,
        I_SYS   = 0b1110011,    /// WFI, CSRXX
        /// funct3/funct7 : arithmetic instructions
        O_add = 0b10000000000000,
        O_sub = 0b01000000000000,
        O_lt  = 0b00100000000000,
        O_ltu = 0b00010000000000,
        O_and = 0b00001000000000,
        O_or  = 0b00000100000000,
        O_xor = 0b00000010000000,
        O_sll = 0b00000001000000,
        O_srl = 0b00000000100000,
        O_sra = 0b00000000010000,
        O_eq  = 0b00000000001000,
        O_ne  = 0b00000000000100,
        O_ge  = 0b00000000000010,
        O_geu = 0b00000000000001,

#if defined(ENABLE_MD)
        /// funct3/funct7 : mulxx,div,mod instructions
        O_mul    = 0b10000000,
        O_mulh   = 0b01000000,
        O_mulhsu = 0b00100000,
        O_mulhu  = 0b00010000,
        O_div    = 0b00001000,
        O_divu   = 0b00000100,
        O_rem    = 0b00000010,
        O_remu   = 0b00000001,
#endif

        /// funct3 : load instructions
        O_lb  = 0b10000,
        O_lbu = 0b01000,
        O_lh  = 0b00100,
        O_lhu = 0b00010,
        O_lw  = 0b00001,
        /// funct3 : store instructions
        O_sb  = 0b100,
        O_sh  = 0b010,
        O_sw  = 0b001,
        /// system instructions
        O_csrrw  = 0b10000000,  /// f3 = 001    /// rd <- csr, csr <- rs1           : (rd  == x0) skips csr read
        O_csrrs  = 0b01000000,  /// f3 = 010    /// rd <- csr, csr <- csr | rs1     : (rs1 == x0) skips csr write
        O_csrrc  = 0b00100000,  /// f3 = 011    /// rd <- csr, csr <- csr & ~rs1    : (rs1 == x0) skips csr write
        O_csrrwi = 0b00010000,  /// f3 = 101    /// rd <- csr, csr <- imm5          : (rd  == x0) skips csr read
        O_csrrsi = 0b00001000,  /// f3 = 110    /// rd <- csr, csr <- csr | imm5    : (imm5 == 0) skips csr write
        O_csrrci = 0b00000100,  /// f3 = 111    /// rd <- csr, csr <- csr & ~imm5   : (imm5 == 0) skips csr write
        O_mret   = 0b00000010,
        O_wfi    = 0b00000001,
        O_csrr_mask = (O_csrrw | O_csrrs | O_csrrc | O_csrrwi | O_csrrsi | O_csrrci),
        O_csrr_imm_mask = (O_csrrwi | O_csrrsi | O_csrrci),
        /// register names
        R_zero = 0,
        R_ra = 1,
        R_sp = 2,
        R_gp = 3,
        R_tp = 4,
        R_t0, R_t1, R_t2,   /// x5-x7
        R_s0, R_s1,         /// x8-x9
        R_a0, R_a1, R_a2, R_a3, R_a4, R_a5, R_a6, R_a7,                 /// x10-x17
        R_s2, R_s3, R_s4, R_s5, R_s6, R_s7, R_s8, R_s9, R_s10, R_s11,   /// x18-x27
        R_t3, R_t4, R_t5, R_t6,                                         /// x28-x31
    };
    struct INST_TYPE {
        BIT COMP, COMPI, LUI, AUIPC, LD, ST, JAL, JALR, BR, SYS;
        void reset() { COMP = 0; COMPI = 0; LUI = 0; AUIPC = 0; LD = 0; ST = 0; JAL = 0; JALR = 0; BR = 0; SYS = 0; }
        void set(UCHAR opcode);
    };
    struct OP_TYPE {
        BIT ADD, SUB, LT, LTU, AND, OR, XOR, SLL, SRL, SRA, EQ, NE, GE, GEU;
        void reset() {
            ADD = 0; SUB = 0; LT = 0; LTU = 0; AND = 0; OR = 0; XOR = 0; SLL = 0; SRL = 0; SRA = 0;
            EQ = 0; NE = 0; GE = 0; GEU = 0;
        }
        void set(USHORT opID);
    };
    static BIT   ld_sext(UCHAR ld_type) { return (ld_type & (O_lb | O_lh)) != 0; }
    static UCHAR ld_size(UCHAR ld_type) { return (ld_type & (O_lb | O_lbu)) ? 0 : (ld_type & (O_lh | O_lhu)) ? 1 : 2; }
    static UCHAR st_size(UCHAR st_type) { return (st_type & O_sb) ? 0 : (st_type & O_sh) ? 1 : 2; }
    struct DC {
        U32 imm;
        UCHAR opcode, I_rs1, I_rs2, I_rd;
        USHORT opID;
#if defined(ENABLE_MD)
        BIT isMD;
#endif
        BIT error;
#if defined(ENABLE_MD)
        void reset() { imm = 0; opcode = 0; I_rs1 = 0; I_rs2 = 0; I_rd = 0; opID = 0; isMD = 0; error = 0; }
#else
        void reset() { imm = 0; opcode = 0; I_rs1 = 0; I_rs2 = 0; I_rd = 0; opID = 0; error = 0; }
#endif
        void decode(U32 ir, BIT ir_valid = 1);
        bool validateComp(UCHAR f3, UCHAR f7, bool isCOMPI);
        USHORT decComp(UCHAR f3, UCHAR f7, bool isCOMPI);
#if defined(ENABLE_MD)
        USHORT decMD(UCHAR f3);
#endif
        USHORT decBranch(UCHAR f3);
        USHORT decLD(UCHAR f3);
        USHORT decST(UCHAR f3);
        USHORT decSYS(USHORT imm12, UCHAR f3, BIT zero_19_7);
        void setRegID(UCHAR rdID, UCHAR rs1ID = 0, UCHAR rs2ID = 0);
        static U32 imm_Itype(U32 ir);
        static U32 imm_Utype(U32 ir);
        static U32 imm_Stype(U32 ir);
        static U32 imm_Jtype(U32 ir);
        static U32 imm_Btype(U32 ir);
        void printDecode(U32 addr, U32 ir, U32* xreg, BIT br_active, BIT sys_br_active);
    };
    struct TrapInfo {
        U32 active, cause, tval;
        void reset() { active = 0; cause = 0; tval = 0; }
        void set(U32 tcause, U32 mtval = 0) { active = 1; cause = tcause; tval = mtval; }
    };
    struct BranchInfo {
        U32 addr;
        BIT active;
        void reset() { addr = 0; active = 0; }
        void set(U32 br_addr) { addr = br_addr; active = 1; }
    };
    struct CSR {
        enum CSR_ENUM {
#define CSR_ENUM
#include "rv32csr_inc.h"
        };
#define CSR_REG_DEF
#include "rv32csr_inc.h"
#if 0
        BIT mcycle_carry _T(state), minstret_carry _T(state);
#endif
        BIT error;
        BIT checkAccess(U12 csr_addr, BIT write) {
            int access_type = (csr_addr >> 10) & 3; /// 11 : read-only, others: read/write
            if (write && (access_type == 3)) {
                return 0;	// NG : write access not allowed in read-only register
            }
            switch (csr_addr) {
#define CSR_CHECK
#include "rv32csr_inc.h"
                return 1;   /// can access (csr_addr) register
            default:
                return 0;   /// (csr_addr) register does not exist
            }
        }
        U32 read(U12 csr_addr, U32 time, U32 timeh) {
            switch (csr_addr) {
#define CSR_READ
#include "rv32csr_inc.h"
            case 0xc01:
                return time;
            case 0xc81:
                return timeh;
            default:
                return 0;
            }
        }
        void write(U12 csr_addr, U32 wdata) {
            switch (csr_addr) {
#define CSR_WRITE
#include "rv32csr_inc.h"
            default:
                break;
            }
        }
        void update_counter(BIT instret_flag);
        void reset() { error = 0; }
        BIT interrupt_detected();
        U32 do_trap(U32 pc);
        U32 do_mret();
        U32 do_csrrx(U32 rs1, U32 imm, U32 csr_op, U32 rdID, U32 time, U32 timeh);
        void writeback(U32 csr_val, U32 rs1, U12 imm, U8 csr_op);
    };
    static void printCompName(USHORT opID, bool immFlag);
#if defined(ENABLE_MD)
    static void printMDName(USHORT opID);
#endif
    static void printBranchName(USHORT opID);
    static void printLDName(USHORT opID);
    static void printSTName(USHORT opID);
    static void printSYSName(USHORT opID);
    static void printRegName(U8 rID);
    static void printImmValue(U32 imm, int hexMode);
    static void printAddrOffset(U32 addr, U32 ofs);
    static bool isShiftOp(USHORT opID) {
        switch (opID) {
        case O_sll: case O_srl: case O_sra: return true;
        default:                            return false;
        }
    }
    static void storeMemByteEnable(U8* mem8, U32 din, U4 byte_enable);
    static bool formatRead(U32 addr, U3 ld_type, U32& dout);
    static bool formatWrite(U32 addr, U3 st_type, U32& din, U4& byte_enable);
    static U32 compute(U32 in1, U32 in2, USHORT op);
    static U32 ALUcompute(U32 x, U32 y, OP_TYPE& op_type);
    static U32 sll(U32 x, U5 y);
    static U32 srx(U32 x, U5 y, BIT sra);
    static U32 getOperand(U5 id, U32 xreg[]) {
        return (id == 0) ? 0 : xreg[id & 0x1f];
    }
    static void writeback(U5 id, U32 xreg[], U32 rd) {
        if (id) { xreg[id & 0x1f] = rd; }
    }
};

#include "rv32_io.h"

#define INTR_VEC_SIZE   7

struct RV32ISim {
    U32 pc, ir;
    U32 xreg[32];
    U32 mem[MEM_SIZE];
    RV32I::DC dc;
    RV32I::CSR csr;
    struct EX {
        U32 rd;
        RV32I::BranchInfo br;
        BIT wfi, error, trap_occurred;
        void reset() { rd = 0; br.reset(); wfi = 0; error = 0; trap_occurred = 0; }
    } ex;
    IO io;
    void execute(AXI4L::CH* axi, U32 intr);
    int step(AXI4L::CH* axi, InterruptPin<INTR_VEC_SIZE>* intr_p);
};

struct RV32I_ARCH1 {
    U32 pc _T(state), tr _T(state);
    U32 ir _T(state);
    BIT flg _T(state);
    BIT IO_LD_completed _T(state);
    U32 bus0, bus1, bus2, bus3;
    BIT wfi, mret, csrrx, trap_occurred, interrupt_detected, inst_completed;
    struct CTRL;
    struct MEMORY {
        U32 addr, din, dout;
        U2 byte_pos _T(state);
        U5 ld_type _T(state);
        UCHAR WE;   /// write_enable
        U32 m[MEM_SIZE] _T(memory_init);
        BIT error;
        void reset() { addr = 0; din = 0; WE = 0; error = 0; }
        void formatWrite(U3 st_type, U32 data) {
            din = data;
            if (!RV32I::formatWrite(addr, st_type, din, WE)) {
                error = 1;
            }
        }
        void doWrite() { RV32I::storeMemByteEnable((U8*)&m[addr >> 2], din, WE); }
        void setRead(U3 ldt) {
            dout = m[addr >> 2];
            ld_type = ldt;
            byte_pos = addr & 3;
        }
    } mem;
    RV32I::DC dc;
    RV32I::INST_TYPE I;
    RV32I::OP_TYPE op;
    U5 ld_type;
    U3 st_type;
    U8 csr_op;
    U2 csr_rw;
#if defined(ENABLE_MD)
    U8 md_type;
#endif
//#define TEST_WFI_DBG
    enum DBG_ACTION {
        DBG_run         = 0,    /// advance until next wfi
        DBG_step_cycle  = 1,    /// advance 1 cycle
        DBG_step_in     = 2,    /// advance 1 instruction
        DBG_step_over   = 3,    /// advance 1 call or 1 instruction
        DBG_step_out    = 4,    /// advance until return instruction
        DBG_ignore_wfi  = 8,    /// ignore all wfi instructions
    };
    struct DBG_STATE {
        unsigned step_mode, step_wait_irq_clear, step_wfi, ignore_wfi;
        U3 step_halt_condition _T(state);
        U32 internal_irq_clear_mask;
        U2 step_pending _T(state), halt_condition _T(state);
        U32 step_target_pc _T(state);
        BIT wfi _T(state);
#if defined(TEST_WFI_DBG)
        U32 pc _T(state);
        U2  sc _T(state);
#endif
        void reset() { step_mode = 0; step_wait_irq_clear = 0; step_wfi = 0; ignore_wfi = 0; internal_irq_clear_mask = 0; }
    } dbg_state;
    void ctrl_decode();
    struct TIMER {
        U2 sc _T(state);
        BIT clr;
        BIT t[4];
        void update_counter() {
            if (clr) { sc = 0; }
            else     { sc = (sc + 1) & 0b11; }
        }
        void set_dec_output() {
            for (int i = 0; i < 4; ++i) { t[i] = 0; }
            t[sc & 3] = 1;
        }
        BIT operator[](int i) { return t[i]; }
    } T;
    struct RegFile {
        U32 xreg[32] _T(state);
        U32 getOperand(UCHAR rsID) { return RV32I::getOperand(rsID, xreg); }
        void writeBack(UCHAR rdID, U32 rd) { RV32I::writeback(rdID, xreg, rd); }
    } rf;
#if defined(ENABLE_MD)
    struct MD {
        U32 a _T(state), b _T(state), c _T(state), d _T(state);
        U4 stt _T(state);
        BIT pending;
        U32 compute(U32 i0, U32 i1, U8 md_type);
    } md;
#endif
    IO io;
    RV32I::CSR csr _T(state);
    void reset_sig() {
        bus0 = 0; bus1 = 0; bus2 = 0; bus3 = 0; wfi = 0; mret = 0; csrrx = 0; trap_occurred = 0;
        interrupt_detected = 0; inst_completed = 0; mem.reset(); T.clr = 0; I.reset(); op.reset(); 
        ld_type = 0; st_type = 0; csr_op = 0; csr_rw = 0; dbg_state.reset(); T.set_dec_output();
#if defined(ENABLE_MD)
        md_type = 0;
#endif
    }
    _C2R_FUNC(1)
    int step(AXI4L::CH* axi, InterruptPin<INTR_VEC_SIZE>* intr_p, DebugInterface* dbg);
    void updateMemIO(AXI4L::CH* axi, U32 intr);
    U32 formatReadData();
    void updateDebugInterface(DebugInterface* dbg);
};

#endif
