
#include "rv32.h"

//#define RVFILE  "rv_test/test1_rv32.out"
//#define RVFILE  "rv_test/test2_rv32.out"
//#define RVFILE  "rv_test/test3_rv32.out"
//#define RVFILE  "rv_test/test4_rv32.out"
//#define RVFILE  "rv_test/test5_rv32.out"
//#define RVFILE  "rv_test/test6_rv32.out"
//#define RVFILE  "rv_test/test7_rv32.out"
#define RVFILE  "rv_boot_uart/boot_rv32.out"
//#define USE_RV32ISIM
#define ENABLE_UART_INTR

#define NUM_MASTERS 1
#define NUM_SLAVES AXI4L::DI_DEV_COUNT

struct IO_PINS {
    UARTPin		uart[2];
    GPIOPin     gpio;
    InterruptPin<INTR_VEC_SIZE> intr;
    DebugInterface dbg;
};

#include "../C2Rlib/utils/ELFUtil.h"

struct RV32I_Testbench {
    C2R::Timer tm;
    IO_PINS io;
    struct InterruptModel {
        int state, count, count2;
        void reset() { state = 0; count = 0; count2 = 0; }
    } intr;
    RV32I_Testbench(const char* fname);
    void runSim(const char* fname);
    void runArch1(const char* fname, int mode); /// mode = 0 : REF_C, mode = 1 : RTL_C
    void updateInterrupt(int mode = 2); /// mode = 2 : sim
};

int main(int argc, char* argv[]) {
    const char* fname = RVFILE;
    if (argc >= 2) {
        fname = argv[1];
    }
    RV32I_Testbench rvtest(fname);
    return 0;
}

#define USE_UART_ARRAY
GPIO_AXI4L  axi_gpio;
UART_AXI4L  axi_uart[2];
AXI4L::CTRL<NUM_MASTERS, NUM_SLAVES> axi_bus_ctrl;
AXI4L::BUS<NUM_MASTERS, NUM_SLAVES>  axi_bus;

RV32ISim sim;
RV32I_ARCH1 arch1;

RV32I::DC rv32i_dc;
RV32I::SymbolTable rv32i_symTable;

void addRV32ISymbol(U32 addr, const char* symName) {
    rv32i_symTable.symTable[addr] = symName;
}

#define DISABLE_RTL_C_LOG
#if defined(__LLVM_C2RTL__)    /// C2RTL cannot handle fprintf for now...
#if defined(DISABLE_RTL_C_LOG)
#define P(m)
#define P1(m, p1)       //p1
#define P2(m, p1, p2)   //p1, p2
#else
#define P(m)            printf(m)
#define P1(m, p1)       printf(m, p1)
#define P2(m, p1, p2)   printf(m, p1, p2)
#endif
#else
#define P(m)            fprintf(RV32I::fp, m)
#define P1(m, p1)       fprintf(RV32I::fp, m, p1)
#define P2(m, p1, p2)   fprintf(RV32I::fp, m, p1, p2)
#endif

void RV32I::SymbolTable::printTable() {
    P("----------- Symbol Table -----------\n");
    for (auto i = symTable.begin(), e = symTable.end(); i != e; ++i) {
        P2("%08x : <%s>\n", i->first, i->second.c_str());
    }
}

void RV32I::SymbolTable::printSymbol(U32 pc) {
    auto i = symTable.find(pc);
    if (i != symTable.end()) {
        P2("=============== %08x : <%s> =================================\n", i->first, i->second.c_str());
    }
}

void RV32I::SymbolTable::printSymbolName(U32 pc) {
    auto i = symTable.find(pc);
    if (i != symTable.end()) {
        P1(" <%s> ", i->second.c_str());
    }
}

void dumpRV32I(U32 ir, U32 pc) {
    rv32i_dc.decode(ir);
    rv32i_symTable.printSymbol(pc);
    rv32i_dc.printDecode(pc, ir, 0, 0, 0);
}

#define SET_BIT(n, v, b)    n = ((n & ~(1 << (b))) | (((v) & 1) << (b)))

int RV32I_SOC_Sim(IO_PINS* io_pins, AXI4L::BUS<NUM_MASTERS, NUM_SLAVES>* axi_bus) {
    int val = 0;
    for (unsigned i = 0; i < 2; ++i) {
        axi_uart[i].step(&axi_bus->s_ch[AXI4L::DI_UART_0 + i], &io_pins->uart[i], &io_pins->intr.p[i]);
    }
    axi_gpio.step(&axi_bus->s_ch[AXI4L::DI_GPIO], &io_pins->gpio, &io_pins->dbg);
    val = sim.step(&axi_bus->m_ch[0], &io_pins->intr);
    axi_bus_ctrl.connectChannel(axi_bus);
    return val;
}

void SimReset_RV32I_SOC_Base() {  /// copy from zero-initialized objects
    static UART_AXI4L zinit_axi_uart;
    static AXI4L::CTRL<NUM_MASTERS, NUM_SLAVES> zinit_axi_bus_ctrl;
    static AXI4L::BUS<NUM_MASTERS, NUM_SLAVES> zinit_axi_bus;
    for (unsigned i = 0; i < 2; ++i) { axi_uart[i] = zinit_axi_uart; }
    axi_bus_ctrl = zinit_axi_bus_ctrl;
    axi_bus = zinit_axi_bus;
}


void SimReset_RV32I_SOC_Sim() {  /// copy from zero-initialized objects
    SimReset_RV32I_SOC_Base();
    static RV32ISim zinit_sim;
    sim = zinit_sim;
}

_C2R_MODULE_
int RV32I_SOC_ARCH1(IO_PINS* io, AXI4L::BUS<NUM_MASTERS, NUM_SLAVES>* axi) {
    int val = 0;
    for (unsigned i = 0; i < 2; ++i) {
        axi_uart[i].step(&axi->s_ch[AXI4L::DI_UART_0 + i], &io->uart[i], &io->intr.p[i]);
    }
    axi_gpio.step(&axi->s_ch[AXI4L::DI_GPIO], &io->gpio, &io->dbg);
    val = arch1.step(&axi->m_ch[0], &io->intr, &io->dbg);
    axi_bus_ctrl.connectChannel(axi);
    return val;
}

void SimReset_RV32I_SOC_ARCH1() {  /// copy from zero-initialized objects
    SimReset_RV32I_SOC_Base();
    static RV32I_ARCH1 zinit_arch1;
    arch1 = zinit_arch1;
}

#define FORCE_SIM_TERMINATION_MAX_CYCLE 1000000 //500000

#define RTL_V_TB_DIR        "C2R_ROOT/RTL_V_TB/"
#define RV32_MODULE_NAME    "RV32I_ARCH1_step"

void RV32I_Testbench::runSim(const char* fname) {
    RV32I::LogFile log(fname, 0);
    SimReset_RV32I_SOC_Sim();
    auto fn = RV32I::LogFile::extractFilename(fname);
    fn = RTL_V_TB_DIR + fn + "/";
    ELF_Parser::ElfMemoryInstaller elfMem(fname, RV32_MODULE_NAME, fn.c_str(), "mem_init",
        4, sim.mem, MEM_ADDR_BITS, &sim.pc, &sim.xreg[RV32I::R_sp], 1, 0, dumpRV32I, addRV32ISymbol);
    elfMem.InstallElf();
    rv32i_symTable.printTable();

    uart_ext_reset();
    memset(&io, 0, sizeof(io));
    tm.Reset("RV32ISim");
    tm.Set();

    //ext_interrupt_state = 0;
    intr.reset();

    bool flag;
    do {
        flag = RV32I_SOC_Sim(&io, &axi_bus);
        for (unsigned i = 0; i < 2; ++i) { io.uart[i].rx = uart_ext(i, io.uart[i].tx); }
        updateInterrupt();
        tm.cycles++;
    } while (!flag && tm.cycles < FORCE_SIM_TERMINATION_MAX_CYCLE);
    uart_ext_flush_rx();
    tm.Set();
}

#define SUPPRESS_RTL_LOG
#if defined(RUN_C2R_TEST)
#define G_ARCH1 G_RV32I_ARCH1_step_3
#define G_UART0 G_UART_AXI4L_step_0
#define	C2R_AUTO_OPENCLOSE
//#define C2R_DISABLE_SYS_VECTOR_OUTPUT
#if defined(C2R_CREATE_TB_VECTOR)
#undef C2R_DISABLE_SYS_VECTOR_OUTPUT
#endif
#if defined(SUPPRESS_RTL_LOG)
#define MAKE_RTL_C_SILENT
#endif
#include "C2R_ROOT/RTL_C/RV32I_SOC_ARCH1.RTL.SYS.h"
#endif

#if defined(TEST_WFI_DBG)
void RV32I_Testbench::updateInterrupt(int mode) {
    if (tm.cycles == 500) { io.intr.set(0xf << 2); }  /// raise ext_intr = 0xf @ 500 cycles
    else if (tm.cycles == 510) { io.intr.clear(0xf << 2); }  /// clear ext_intr = 0x0 @ 510 cycles
    /// detect if machine-interrupt is enabled and uart[0] interrupt is enabled
    unsigned ext_interrupt_enabled = 0, pc = 0, axi_uart_tx_empty = 0;
    int wfi = 0, step_wfi = 0;
    if (mode == 0) {
        ext_interrupt_enabled = ((arch1.csr.mie >> 11) & (arch1.csr.mstatus >> 3) & (arch1.io.sysctrl.ext_irq_enable) & 1);
        pc = arch1.pc;
        axi_uart_tx_empty = !axi_uart[0].uart.tx.buf.not_empty;
        wfi = arch1.wfi;
        step_wfi = arch1.dbg_state.step_wfi;
    }
#if defined(RUN_C2R_TEST)
    else if (mode == 1) {
        auto& rtl_arch1 = G_ARCH1._this;
        auto& rtl_uart0 = G_UART0._this;
        ext_interrupt_enabled = (((rtl_arch1.csr.mie >> 11) & 1) && ((rtl_arch1.csr.mstatus >> 3) & 1) && (rtl_arch1.io.sysctrl.ext_irq_enable & 1));
        pc = rtl_arch1.pc;
        axi_uart_tx_empty = !rtl_uart0.uart.tx.buf.not_empty;
    }
#endif
    if (tm.cycles == 0) {
        intr.state = 0;
    }
#if defined(TEST_WFI_DBG)
    if (mode == 0 && (arch1.wfi || arch1.dbg_state.step_wfi)) {
        if (intr.count == 0) {
            if (intr.count2 == 0) {
                io.gpio.sw_in = RV32I_ARCH1::DBG_step_cycle << 12;// 0x01 << 6;
            }
            else if (intr.count2 == 10) {
                io.gpio.sw_in = RV32I_ARCH1::DBG_step_in << 12;// 0x02 << 6;
            }
            else if (intr.count2 == 20) {
                io.gpio.sw_in = RV32I_ARCH1::DBG_step_over << 12;// 0x04 << 6;
            }
            else if (intr.count2 == 30) {
                io.gpio.sw_in = RV32I_ARCH1::DBG_step_out << 12;// 0x08 << 6;
            }
            else if (intr.count2 == 40) {
                io.gpio.sw_in = RV32I_ARCH1::DBG_ignore_wfi << 12;// 0x10 << 6;
            }
        }
        if (intr.state == 0) {
            intr.count = 0;
            intr.state = 1;
        }
        else if (intr.state == 1) {
            if (intr.count == 10) {
                io.intr.set(0x4);
            }
            else if (intr.count == 15) {
                io.intr.clear(0x4);
            }
            else if (intr.count > 15 && arch1.dbg_state.step_wfi) {
                intr.state = 2;
            }
            intr.count++;
        }
        else if (intr.state == 2 && intr.count2 < 100) {
            intr.count = 0;
            intr.state = 1;
            intr.count2++;
        }
    }
#else
    int uart_ext_tx_ready = uart_ext_tx_stream_ready(0);
    if (ext_interrupt_enabled && uart_ext_tx_ready && axi_uart_tx_empty) {
        printf("[%6d] pc = %x, ext_interrupt_enable = %d\n", (int)tm.cycles, pc, ext_interrupt_enabled);
        ext_interrupt_state++;
        uart_ext_setup_stream_input(0);
    }
#endif
}
#else
void RV32I_Testbench::updateInterrupt(int mode) {}
#endif

/// RV32I_ARCH1_step_G__this_mem_m_0
void RV32I_Testbench::runArch1(const char* fname, int mode) {
    U32* mem, * pc, * sp;
    int (*RV32I_SOC)(IO_PINS * io_pins, AXI4L::BUS<NUM_MASTERS, NUM_SLAVES>*axi_bus);
#if defined(RUN_C2R_TEST)
    auto& rtl = G_ARCH1._this;
    if (mode) {
#if !defined(_DEBUG)
        SimReset_RV32I_SOC_ARCH1_RTL();
#endif
        mem = rtl.mem.m; pc = &rtl.pc; sp = &rtl.rf.xreg[RV32I::R_sp];
        RV32I_SOC = RV32I_SOC_ARCH1_RTL;
    } else {
#else
        {
#endif
        SimReset_RV32I_SOC_ARCH1();
        mem = arch1.mem.m; pc = &arch1.pc; sp = &arch1.rf.xreg[RV32I::R_sp];
        RV32I_SOC = RV32I_SOC_ARCH1;
    }
    RV32I::LogFile log(fname, 1 + mode);
    auto fn = RV32I::LogFile::extractFilename(fname);
    fn = RTL_V_TB_DIR + fn + "/";
    ELF_Parser::ElfMemoryInstaller elfMem(fname, RV32_MODULE_NAME, fn.c_str(), "G__this_mem_m",
        4, mem, MEM_ADDR_BITS, pc, sp, 1, 0, dumpRV32I, addRV32ISymbol);
    ELF_Parser::Info::dontPrintParseLog = (mode == 1);
    elfMem.InstallElf();
    rv32i_symTable.printTable();
    uart_ext_reset();
    memset(&io, 0, sizeof(io));
    tm.Reset((mode) ? "RV32I_ARCH1_RTL" : "RV32I_ARCH1");
    tm.Set();
    intr.reset();
    bool flag;
    do {
        flag = RV32I_SOC(&io, &axi_bus);
        for (unsigned i = 0; i < 2; ++i) { io.uart[i].rx = uart_ext(i, io.uart[i].tx); }
        updateInterrupt(mode);
        tm.cycles++;
#if defined(TEST_WFI_DBG)
    } while (tm.cycles < FORCE_SIM_TERMINATION_MAX_CYCLE);
#else
    } while (!flag && tm.cycles < FORCE_SIM_TERMINATION_MAX_CYCLE);
#endif
    uart_ext_flush_rx();
    tm.Set();
}

#include <string>

std::string RV32I::LogFile::extractFilename(const char* fname) {
    std::string fn = fname;
    auto p1 = fn.find_last_of('/');
    if (p1 != std::string::npos) {
        fn = fn.substr(p1 + 1);
    }
    p1 = fn.find_last_of('.');
    if (p1 != std::string::npos) {
        fn = fn.substr(0, p1);
    }
    return fn;
}
void RV32I::LogFile::open(const char* fname, int type) {
    C2R_ASSERT(fp == stdout);
    auto fn = extractFilename(fname);
    fn += "." + std::to_string(type);
    fn += ".log";
    FILE* fp0 = fopen(fn.c_str(), "w");
    if (fp0) { fp = fp0; }
}
void RV32I::LogFile::close() {
    if (fp != stdout) { fclose(fp); }
    fp = stdout;
}

FILE* RV32I::fp = stdout;

#define SKIP_REF_C_ON_RUN_C2R_TEST

RV32I_Testbench::RV32I_Testbench(const char* fname) {
#if defined(USE_RV32ISIM)
    runSim(fname);
    printf("=======================================\n");
#elif defined(RUN_C2R_TEST)
#if !defined(SKIP_REF_C_ON_RUN_C2R_TEST)
    runArch1(fname, 0); /// mode = 0 : REF_C
    printf("=======================================\n");
#endif
    runArch1(fname, 1); /// mode = 1 : RTL_C
    printf("=======================================\n");
#else
    runArch1(fname, 0); /// mode = 0 : REF_C
    printf("=======================================\n");
#endif
}

int RV32ISim::step(AXI4L::CH* axi, InterruptPin<INTR_VEC_SIZE>* intr_p) {
    U32 curPC = pc;
    ir = mem[pc >> 2];   /// fetch
    dc.decode(ir);
    execute(axi, intr_p->vec());
    if (!io.stalled) {
        rv32i_symTable.printSymbol(curPC);
        P2("[%6d:%6d] ", csr.minstret, csr.mcycle);
        dc.printDecode(curPC, ir, xreg, ex.br.active, ex.trap_occurred | (dc.opcode == RV32I::I_SYS && dc.opID == RV32I::O_mret));
    }
    if (dc.error || ex.error || ex.wfi) {
        printf("dc.error = %d, ex.error = %d, wfi = %d\n", dc.error, ex.error, ex.wfi);
        printf("instret = %d\n", csr.minstret);
    }
    csr.update_counter(!io.stalled & !ex.trap_occurred);
    return dc.error || ex.error || ex.wfi;
}
#define MEM_ADDR_OUT_OF_BOUNDS(addr)    ((addr >> (MEM_ADDR_BITS + MEM_BYTE_POS_BW)) != 0)

void RV32ISim::execute(AXI4L::CH* axi, U32 intr) {
    ex.reset();
    csr.reset();
    U32 rs1 = RV32I::getOperand(dc.I_rs1, xreg);
    U32 rs2 = RV32I::getOperand(dc.I_rs2, xreg);

    U32 pc_imm = pc + dc.imm;
    U32 nxt_pc = pc + 4;
    U32 rs1_imm = rs1 + dc.imm;
    U32 op2 = (dc.opcode == RV32I::I_COMPI) ? dc.imm : rs2;

    U4 we = 0;  /// write_enable : byte_enable
    U32 wdata = rs2;
    if (dc.opcode == RV32I::I_ST) {
        if (!RV32I::formatWrite(rs1_imm, dc.opID & 7, wdata, we)) {
            ex.error = 1;
        }
    }
    BIT addr_out_of_bounds = MEM_ADDR_OUT_OF_BOUNDS(rs1_imm);
    U2 io_op = (addr_out_of_bounds) ? (dc.opcode == RV32I::I_LD) | ((dc.opcode == RV32I::I_ST) << 1) : 0;
    U2 io_sz = (io_op == 0b01) ? RV32I::ld_size(dc.opID & 0x1f) : RV32I::st_size(dc.opID & 7);
    io.fsm(io_op, io_sz, rs1_imm, wdata, axi, intr, 0);   /// SYSCTRL, AXI

    if (io.stalled) {
        return;
    }
    if (io.data_latched) {
        ex.rd = io.data_in;
    }
    if (!io.active) {
        if (csr.interrupt_detected()) {
            pc = csr.do_trap(pc);
            ex.trap_occurred = 1;
            return;
        }
        switch (dc.opcode) {
        case RV32I::I_COMP:
        case RV32I::I_COMPI:
        case RV32I::I_BR: {
            U32 comp_out = RV32I::compute(rs1, op2, dc.opID);
            ex.rd = comp_out;
            if (dc.opcode == RV32I::I_BR && (comp_out & 1)) {
                ex.br.set(pc_imm);
            }
            break;
        }
        case RV32I::I_LUI:      ex.rd = dc.imm;             break;
        case RV32I::I_AUIPC:    ex.rd = pc_imm;             break;
        case RV32I::I_LD:       ex.rd = mem[rs1_imm >> 2];  break;
        case RV32I::I_ST:       RV32I::storeMemByteEnable((U8*)&mem[rs1_imm >> 2], wdata, we); break;
        case RV32I::I_JAL:      ex.rd = nxt_pc; ex.br.set(pc_imm);                  break;
        case RV32I::I_JALR:     ex.rd = nxt_pc; ex.br.set(rs1_imm & 0xfffffffe);    break;
        case RV32I::I_SYS:
            if (dc.opID == RV32I::O_wfi) {
                ex.wfi = 1;
            }
            else if (dc.opID == RV32I::O_mret) {
                ex.br.set(csr.do_mret());
            }
            else if (dc.opID & RV32I::O_csrr_mask) {
                U32 rs1_val = (dc.opID & RV32I::O_csrr_imm_mask) ? dc.I_rs1 : rs1;
                ex.rd = csr.do_csrrx(rs1_val, dc.imm, dc.opID, dc.I_rd, io.sysctrl.mtime_l, io.sysctrl.mtime_h);
            }
            break;
        }
        csr.mip = (io.sysctrl.meip << 11) | (io.sysctrl.mtip << 7);   /// update { mip.meip[11], mip.meip[7] }
    }
    if (dc.opcode == RV32I::I_LD) {
        if (!RV32I::formatRead(rs1_imm, dc.opID & 0x1f, ex.rd)) {
            ex.error = 1;
        }
    }
    if (dc.I_rd) {
        xreg[dc.I_rd & 0x1f] = ex.rd;
    }
    pc = (ex.br.active) ? ex.br.addr : nxt_pc;
}

#if 0
imm_i = {20{inst[31]}, inst[31:20]};
imm_s = {20{inst[31]}, inst[31:25], inst[11:7]};
imm_b = {20{inst[31]}, inst[7], inst[30:25], inst[11:8], 1'b0};
imm_j = {12{inst[31]}, inst[19:12], inst[20], inst[30:21], 1'b0};
imm_u = {inst[31:20], 12'b0}; 
#endif

#define PN(n)           case O_##n: if (immFlag) P(#n "i "); else P(#n " ");  break
#define PN2(n, m, m2)   case O_##n: if (immFlag) P(#m " ");  else P(#m2 " "); break
void RV32I::printCompName(USHORT opID, bool immFlag) {
    switch (opID) {
        PN(add); PN(sub);
        PN2(lt,  slt,  slti);
        PN2(ltu, sltu, sltiu);
        PN(and); PN(or); PN(xor);
        PN(sll); PN(srl); PN(sra);
    }
}
void RV32I::printMDName(USHORT opID) {
    switch (opID) {
        case O_mul:    P("mul ");    break;
        case O_mulh:   P("mulh ");   break;
        case O_mulhsu: P("mulhsu "); break;
        case O_mulhu:  P("mulhu ");  break;
        case O_div:    P("div ");    break;
        case O_divu:   P("divu ");   break;
        case O_rem:    P("rem ");    break;
        case O_remu:   P("remu ");   break;
    }
}
#undef PN
#undef PN2
#define PN(n)        case O_##n: P("b" #n " "); break
void RV32I::printBranchName(USHORT opID) { switch (opID) { PN(eq); PN(ne); PN(lt); PN(ltu); PN(ge); PN(geu); } }
#undef PN
#define PN(n)        case O_##n: P(#n " "); break
void RV32I::printLDName(USHORT opID)  { switch (opID) { PN(lb); PN(lbu); PN(lh); PN(lhu); PN(lw); } }
void RV32I::printSTName(USHORT opID)  { switch (opID) { PN(sb); PN(sh); PN(sw); } }
void RV32I::printSYSName(USHORT opID) {
    switch (opID) { PN(wfi); PN(mret); PN(csrrw); PN(csrrs); PN(csrrc); PN(csrrwi); PN(csrrsi); PN(csrrci); }
}
#undef PN
#define PN(n)        case R_##n: P(#n); break
void RV32I::printRegName(U8 rID) {
    switch (rID) {
        PN(zero); PN(ra); PN(sp); PN(gp); PN(tp);
        PN(t0); PN(t1); PN(t2);
        PN(s0); PN(s1);
        PN(a0); PN(a1); PN(a2); PN(a3); PN(a4); PN(a5); PN(a6); PN(a7);
        PN(s2); PN(s3); PN(s4); PN(s5); PN(s6); PN(s7); PN(s8); PN(s9); PN(s10); PN(s11);
        PN(t3); PN(t4); PN(t5); PN(t6);
    }
}
#undef PN
void RV32I::printImmValue(U32 imm, int hexMode) {
    if      (hexMode == 1) { P1("0x%x", imm); }
    else if (hexMode == 2) { P1("%x", imm);   }
    else                   { P1("%d", (S32)imm); }
}
void RV32I::printAddrOffset(U32 addr, U32 ofs) {
    printImmValue(addr + ofs, 2);
    if (ofs >> 31) { P2(" (%x - 0x%x)", addr, -(S32)ofs); }
    else           { P2(" (%x + 0x%x)", addr,  ofs); }
}
void RV32I::DC::printDecode(U32 addr, U32 ir, U32* xreg, BIT br_active, BIT sys_br_active) {
#define PR(n)           printRegName(I_##n)
#define PI(n)           printImmValue(imm, n)
#define PSI(n)          printImmValue((n) ? imm & 0x1f : imm, n)
#define PU              printImmValue((imm >> 12), 1)
#define PA(val)         printImmValue(val, 2)
#if defined(__LLVM_C2RTL__)
#define PSYM(pc)
#else
#define PSYM(pc) rv32i_symTable.printSymbolName(pc)
#endif

    P2("%8x:  %08x   ", addr, ir);
    if (error) {
        P("< decode error >\n");
        return;
    }
    bool pseudo_instruction_flag = (opcode == I_COMPI && opID == O_add && (!imm || !I_rs1));
    switch (opcode) {
#if defined(ENABLE_MD)
    case I_COMP:
        if (isMD) {
            printMDName(opID); 
        } else {
            printCompName(opID, 0); 
        }
        PR(rd); P(","); PR(rs1); P(","); PR(rs2); break;
#else
    case I_COMP:    printCompName(opID, 0); PR(rd); P(","); PR(rs1); P(","); PR(rs2); break;
#endif
    case I_COMPI:
        if (pseudo_instruction_flag) {  /// pseudo-instruction
            if (!I_rs1) { P("li "); PR(rd); P(","); PI(0);   P(" ("); } /// load immediate
            else        { P("mv "); PR(rd); P(","); PR(rs1); P(" ("); } /// move
        }
        printCompName(opID, 1); PR(rd); P(","); PR(rs1); P(","); PSI(isShiftOp(opID));
        if (pseudo_instruction_flag) { P(")"); }
        break;
    case I_LUI:     P("lui ");   PR(rd); P(","); PU; break;
    case I_AUIPC:   P("auipc "); PR(rd); P(","); PU; break;
    case I_LD:      printLDName(opID); PR(rd); P(",");  PI(0); P("("); PR(rs1); P(")"); break;
    case I_ST:      printSTName(opID); PR(rs2); P(","); PI(0); P("("); PR(rs1); P(")"); break;
    case I_JAL:     P("jal "); PR(rd); P(","); printAddrOffset(addr, imm); PSYM(addr + imm);
        break;
    case I_JALR:
        if (I_rd == 0 && I_rs1 == R_ra && !imm) { P("ret : "); }
        P("jalr "); PR(rd); P(","); PI(0); P("("); PR(rs1); P(")");
        break;
    case I_BR:      printBranchName(opID); PR(rs1); P(","); PR(rs2); P(","); printAddrOffset(addr, imm); break;
    case I_SYS:
        printSYSName(opID);
        if (opID & O_csrr_mask) {
            PR(rd); P(",");
            if (opID & O_csrr_imm_mask) { PI(I_rs1); }
            else { PR(rs1); }
        }
        break;
    }
    if (xreg) {
        if (I_rd) { P(" : "); PR(rd); P1("(%08x)", getOperand(I_rd, xreg)); }
        if (I_rs1 == R_ra && !imm) {
            P(", "); printRegName(R_a0); P1("(%08x) <return-value>", xreg[R_a0]);
        }
    }
    P("\n");
    if (sys_br_active)  { P("********************************************************\n"); }
    else if (br_active) { P("--------------------------------------------------------\n"); }
}
#undef PR
#undef PI
#undef PSI
#undef PU
#undef PA

#if 0
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
#endif
bool RV32I::DC::validateComp(UCHAR f3, UCHAR f7, bool isCOMPI) {
    switch (f3) {
    case 0b000: return (isCOMPI || f7 == 0b0000000 || f7 == 0b0100000); /// ADD, SUB
    case 0b101: return (f7 == 0b0000000 || f7 == 0b0100000);            /// SRL, SRA
    case 0b001: return (f7 == 0b0000000);                               /// SLL
    default:    return (isCOMPI || f7 == 0b0000000);                    /// SLT, SLTU, AND, OR, XOR
    }
}
USHORT RV32I::DC::decComp(UCHAR f3, UCHAR f7, bool isCOMPI) {
    if (!validateComp(f3, f7, isCOMPI)) { return 0; }   /// error!
    switch (f3) {
    case 0b000: return (isCOMPI || f7 == 0b0000000) ? O_add : O_sub;
    case 0b010: return O_lt;
    case 0b011: return O_ltu;
    case 0b111: return O_and;
    case 0b110: return O_or;
    case 0b100: return O_xor;
    case 0b001: return O_sll;
    default:    return (f7 == 0b0000000) ? O_srl : O_sra;
    }
}
#if defined(ENABLE_MD)
USHORT RV32I::DC::decMD(UCHAR f3) {
    switch (f3) {
        case 0b000: return O_mul;
        case 0b001: return O_mulh;
        case 0b010: return O_mulhsu;
        case 0b011: return O_mulhu;
        case 0b100: return O_div;
        case 0b101: return O_divu;
        case 0b110: return O_rem;
        default   : return O_remu;
    }
}
#endif

USHORT RV32I::DC::decBranch(UCHAR f3) {
    switch (f3) {
    case 0b000: return O_eq;    /// BEQ  : funct3(000) : branch Equal
    case 0b001: return O_ne;    /// BNE  : funct3(001) : branch Not-Equal
    case 0b100: return O_lt;    /// BLT  : funct3(100) : branch Less-Then Signed
    case 0b101: return O_ge;    /// BGE  : funct3(101) : branch Greater-or-Equal Signed
    case 0b110: return O_ltu;   /// BLTU : funct3(110) : branch Less-Then Unsigned
    case 0b111: return O_geu;   /// BGEU : funct3(111) : branch Greater-or-Equal Unsigned
    default:    return 0;       /// error!
    }
}
USHORT RV32I::DC::decLD(UCHAR f3) {
    switch (f3) {
    case 0b000: return O_lb;  /// funct3(000) : LB (load byte)
    case 0b100: return O_lbu; /// funct3(100) : LBU(load byte unsigned)
    case 0b001: return O_lh;  /// funct3(001) : LH (load half-word signed)
    case 0b101: return O_lhu; /// funct3(101) : LHU(load half-word unsigned)
    case 0b010: return O_lw;  /// funct3(010) : LW (load word)
    default:    return 0;  /// error!
    }
}
USHORT RV32I::DC::decST(UCHAR f3) {
    switch (f3) {
    case 0b000: return O_sb;  /// funct3(000) : SB (store byte))
    case 0b001: return O_sh;  /// funct3(001) : SH (store half-word)
    case 0b010: return O_sw;  /// funct3(010) : SW (store word)
    default:    return 0;  /// error!
    }
}
USHORT RV32I::DC::decSYS(USHORT imm12, UCHAR f3, BIT zero_19_7) {
    if (f3) {
        switch (f3) {
        case 0b001: return O_csrrw;     /// rd <- csr, csr <- rs1           : (rd  == x0) skips csr read
        case 0b010: return O_csrrs;     /// rd <- csr, csr <- csr | rs1     : (rs1 == x0) skips csr write
        case 0b011: return O_csrrc;     /// rd <- csr, csr <- csr & ~rs1    : (rs1 == x0) skips csr write
        case 0b101: return O_csrrwi;    /// rd <- csr, csr <- imm5          : (rd  == x0) skips csr read
        case 0b110: return O_csrrsi;    /// rd <- csr, csr <- csr | imm5    : (imm5 == 0) skips csr write
        case 0b111: return O_csrrci;    /// rd <- csr, csr <- csr & ~imm5   : (imm5 == 0) skips csr write
        }
    } else if (zero_19_7) {
        if      (imm12 == 0x302) { return O_mret; }
        else if (imm12 == 0x105) { return O_wfi; }
    }
    return 0;   /// error!!
}
void RV32I::DC::setRegID(UCHAR rdID, UCHAR rs1ID, UCHAR rs2ID) {
    I_rd = rdID; I_rs1 = rs1ID; I_rs2 = rs2ID;
}
void RV32I::DC::decode(U32 ir, BIT ir_valid) {
    reset();
    opcode        = IR_FLD( 0, 7);
    UCHAR rdID    = IR_FLD( 7, 5);
    UCHAR rs1ID   = IR_FLD(15, 5);
    UCHAR rs2ID   = IR_FLD(20, 5);
    UCHAR f3      = IR_FLD(12, 3);
    UCHAR f7      = IR_FLD(25, 7);
    USHORT imm12  = IR_FLD(20, 12);
    BIT zero_19_7 = (IR_FLD(7, 13) == 0);
    //bool isCOMPI = (opcode == I_COMPI);
#if defined(ENABLE_MD)
    isMD = (opcode == I_COMP && f7 == 0b0000001);
#endif
    ///////////// decode funct3/funct7 /////////////
    switch (opcode) {
#if defined(ENABLE_MD)
    case I_COMP:  opID = (isMD) ? decMD(f3) : decComp(f3, f7, 0);   setRegID(rdID, rs1ID, rs2ID); error = (!opID) & ir_valid;  break;
#else
    case I_COMP:  opID = decComp(f3, f7, 0);            setRegID(rdID, rs1ID, rs2ID); error = (!opID) & ir_valid;  break;
#endif
    case I_COMPI: opID = decComp(f3, f7, 1);            setRegID(rdID, rs1ID);        error = (!opID) & ir_valid;  break;
    case I_LUI:
    case I_AUIPC:                                       setRegID(rdID);                                            break;
    case I_LD:    opID = decLD(f3);                     setRegID(rdID, rs1ID);        error = (!opID) & ir_valid;  break;
    case I_ST:    opID = decST(f3);                     setRegID(0,    rs1ID, rs2ID); error = (!opID) & ir_valid;  break;
    case I_JAL:                                         setRegID(rdID);                                            break;
    case I_JALR:                                        setRegID(rdID, rs1ID);                                     break;
    case I_BR:    opID = decBranch(f3);                 setRegID(0,    rs1ID, rs2ID); error = (!opID) & ir_valid;  break;
    case I_SYS:   opID = decSYS(imm12, f3, zero_19_7);  setRegID(rdID, rs1ID);        error = (!opID) & ir_valid;  break;
    default:      error = ir_valid;   /// DecError!
    }
    ///////////// decode imm  /////////////
    switch (opcode) {
    case I_COMPI:
    case I_LD:
    case I_JALR:    imm = imm_Itype(ir); break;
    case I_LUI:
    case I_AUIPC:   imm = imm_Utype(ir); break;
    case I_ST:      imm = imm_Stype(ir); break;
    case I_JAL:     imm = imm_Jtype(ir); break;
    case I_BR:      imm = imm_Btype(ir); break;
    case I_SYS:     imm = IR_FLD(20, 12); break;
    }
}

U32 RV32I::DC::imm_Itype(U32 ir) { return ((S32)ir) >> 20; }
U32 RV32I::DC::imm_Stype(U32 ir) { return IR_SEXT(20) | (IR_FLD(25, 7) <<  5) | IR_FLD(7, 5); }
U32 RV32I::DC::imm_Btype(U32 ir) { return IR_SEXT(20) | (IR_FLD( 7, 1) << 11) | (IR_FLD(25, 6) << 5) | (IR_FLD(8, 4) << 1); }
U32 RV32I::DC::imm_Utype(U32 ir) { return ir & 0xfffff000; }
U32 RV32I::DC::imm_Jtype(U32 ir) { return IR_SEXT(12) | (IR_FLD(12, 8) << 12) | (IR_FLD(20, 1) << 11) | (IR_FLD(21, 10) << 1); }

U32 RV32I::compute(U32 in0, U32 in1, USHORT op) {
    U32 sft_val = in1 & 0x1f;
    BIT ult = (in0 < in1), sign0 = (in0 >> 31), sign1 = (in1 >> 31);
    BIT slt = ult ^ sign0 ^ sign1, ne = (in0 != in1);
    switch (op) {
    case O_add: return in0 + in1;
    case O_sub: return in0 - in1;
    case O_and: return in0 & in1;
    case O_or:  return in0 | in1;
    case O_xor: return in0 ^ in1;
    case O_sll: return in0 << sft_val;
    case O_srl:
    case O_sra: {
        S32 s32 = (op == O_sra) ? (S32)(in0 & 0x80000000) : 0;
        return (s32 >> sft_val) | (in0 >> sft_val);
    }
    case O_lt:  return slt;
    case O_ltu: return ult;
    case O_ge:  return !slt;
    case O_geu: return !ult;
    case O_eq:  return !ne;
    case O_ne:  return ne;
    default:    return 0;
    }
}

U32 RV32I::ALUcompute(U32 x, U32 y, OP_TYPE& op) {
    UCHAR sft_val = y & 0x1f;
    BIT sub = (op.SUB | op.LTU | op.GEU | op.LT | op.GE);
    U32 y2 = (sub) ? ~y : y;
    U64 as64 = (U64)x + (U64)y2 + sub;
    BIT c31 = (as64 >> 32) & 1;         /// c31 = geu (great-or-equal unsigned)
    U32 as32 = (U32)as64;
    U32 exor = x ^ y;
    BIT eq = !exor; /// equals
    BIT sign_x = (x >> 31), sign_y = (y >> 31);
    BIT ge = c31 ^ sign_x ^ sign_y;     /// ge (great-or-equal signed)
    if (op.ADD | op.SUB) { return as32; }
    if (op.AND)          { return x & y; }
    if (op.OR)           { return x | y; }
    if (op.XOR)          { return exor; }
    if (op.SLL)          { return sll(x, sft_val); }
    if (op.SRL | op.SRA) { return srx(x, sft_val, op.SRA); }
    return (op.LTU & !c31) | (op.GEU & c31) | (op.LT & !ge) | (op.GE & ge) | (op.EQ & eq) | (op.NE & !eq);
}

#define Y(b)    ((y >> b) & 1)
U32 RV32I::sll(U32 x, U5 y) {
    U32 xL1 = (Y(0)) ? (x   << 1) : x;
    U32 xL2 = (Y(1)) ? (xL1 << 2) : xL1;
    U32 xL4 = (Y(2)) ? (xL2 << 4) : xL2;
    U32 xL8 = (Y(3)) ? (xL4 << 8) : xL4;
    return    (Y(4)) ? (xL8 << 16) : xL8;
}
U32 RV32I::srx(U32 x, U5 y, BIT sra) {
    S32 t = (sra) ? (x & 0x80000000) : 0;
    U32 xR1 = (Y(0)) ? t | (x   >> 1) : x;   t >>= 1;
    U32 xR2 = (Y(1)) ? t | (xR1 >> 2) : xR1; t >>= 2;
    U32 xR4 = (Y(2)) ? t | (xR2 >> 4) : xR2; t >>= 4;
    U32 xR8 = (Y(3)) ? t | (xR4 >> 8) : xR4; t >>= 8;
    return    (Y(4)) ? t | (xR8 >> 16) : xR8;
}
#undef Y

/// { 2, 3, 5, 7, 11 }
/// 2*3 + 3*4 + 5*6 + 7*8 + 11*12 = 236 (0xec)

#if 0
O_lb  = 0b10000,
O_lbu = 0b01000,
O_lh  = 0b00100,
O_lhu = 0b00010,
O_lw  = 0b00001,
O_sb  = 0b100,
O_sh  = 0b010,
O_sw  = 0b001,
#endif

bool RV32I::formatRead(U32 addr, U5 ld_type, U32& dout) {
    U32 bytePos = addr & 3;     /// 00, 01, 10, 11
    dout >>= (bytePos << 3);    /// zero-extend
    /// check address alignment
    BIT sext = ld_sext(ld_type);
    UCHAR sz = ld_size(ld_type);
    switch (sz) {
    case 0b00:  /// load byte
        dout &= 0xff;
        if (sext && (dout & 0x80)) {
            dout |= 0xffffff00;
        }
        break;  /// all bytePos are allowed in byte loads
    case 0b01:  /// load half-word
        if (bytePos & 1) { return 0; }  /// error! half-word must align in 2-byte boundary
        dout &= 0xffff;
        if (sext && (dout & 0x8000)) {
            dout |= 0xffff0000;
        }
        break;
    case 0b10:  /// load word
        if (bytePos) { return 0; }      /// error! word must align in 4-byte boundary
        break;
    default: return 0;     /// error!
    }
    return 1;   /// ok!
}

bool RV32I::formatWrite(U32 addr, U3 st_type, U32& din, U4& byte_enable) {
    U32 bytePos = addr & 3;         /// 00, 01, 10, 11
    din <<= (bytePos << 3);
    /// check address alignment and set byte_enable
    UCHAR sz = st_size(st_type);
    switch (sz) {
    case 0b00:  /// store byte
        byte_enable = 0b0001 << bytePos;
        break;
    case 0b01:  /// store half-word
        if (bytePos & 1) { return 0; }  /// error! half-word must align in 2-byte boundary
        byte_enable = 0b0011 << bytePos;
        break;
    case 0b10:  /// store word
        if (bytePos) { return 0; }      /// error! word must align in 4-byte boundary
        byte_enable = 0b1111;
        break;
    default:
        return 0;
    }
    return 1;
}

void RV32I::storeMemByteEnable(U8* mem8, U32 din, U4 byte_enable) {
    U8* din8 = (U8*)&din;
    if (byte_enable & 0x1) { mem8[0] = din8[0]; }
    if (byte_enable & 0x2) { mem8[1] = din8[1]; }
    if (byte_enable & 0x4) { mem8[2] = din8[2]; }
    if (byte_enable & 0x8) { mem8[3] = din8[3]; }
}

///  interrupt_pending & interrupt_enable & mstatus.mie[3] (machine-interrupt-enabled)
BIT RV32I::CSR::interrupt_detected() { return (mip & mie) && (mstatus & 0x8); }

void RV32I::CSR::update_counter(BIT instret_flag) {
#if 0
    auto nxt_mcycle = mcycle + 1;
    mcycleh += mcycle_carry;    /// spend 1 cycle to propagate mcycle_carry to mcycleh
    mcycle_carry = (nxt_mcycle == 0);
    mcycle = nxt_mcycle;
    if (instret_flag) {
        auto nxt_minstret = minstret + 1;
        minstreth += minstret_carry;    /// spend 1 cycle to propagate minstret_carry to minstreth
        minstret_carry = (nxt_minstret == 0);
        minstret = nxt_minstret;
    }
#else
    auto nxt_mcycle = mcycle + 1;
    mcycleh += (nxt_mcycle == 0);
    mcycle = nxt_mcycle;
    if (instret_flag) {
        auto nxt_minstret = minstret + 1;
        minstreth += (nxt_minstret == 0);
        minstret = nxt_minstret;
    }
#endif
}

U32 RV32I::CSR::do_mret() { /// mode <- mstatus.mpp, mstatus : {mie <- mpie, mpie <- 1, mpp <- 0}, pc <- mepc
    ///////////////// mstatus update ///////////////////
    BIT xie = GET_FLD(mstatus, 7, 1);   /// mstatus.mpie[7]
    //BIT mode = GET_FLD(mstatus, 11, 2); /// mstatus.mpp[12:11] : mode is assumed to be fixed to 3 (machine-mode)
    mstatus = (xie << 3) | (1 << 7) | (0 << 11);    /// mstatus : {mie[3] <- mpie[7], mpie[7] <- 1, mpp[12:11] <- 0}
    return mepc;
}

U32 RV32I::CSR::do_trap(U32 pc) {
    //////////// interrupt detected here -> jump to trap-handler at mtvec address //////////////
    U32 irq_vec = (mip & mie);  /// irq_vec = interrupt_pending & interrupt_enable;
    ///////////////// mcause update ///////////////////
    if      (irq_vec & (1 << 11)) { mcause = 11 | (1 << 31); }    /// machine external interrupt : msb indicates interrupt trap
    else if (irq_vec & (1 <<  7)) { mcause =  7 | (1 << 31); }    /// machine timer interrupt    : msb indicates interrupt trap
    ///////////////// mtval update  ///////////////////
    mtval = 0;  /// not used in interrupt --> should be extended when exception traps are implemented
    /// mode <- 3, mstatus : {mpie <- mie, mpp <- mode(3), mie <- 0}, mepc <- pc, mtval <- tval, pc <- mtvec
    ///////////////// mstatus update ///////////////////
    BIT xie = GET_FLD(mstatus, 3, 1);   /// mstatus.mie[3]
    mstatus = (0 << 3) | (xie << 7) | (3 << 11);    /// mstatus : {mpie[7] <- mie, mpp[21:11] <- mode(3), mie[3] <- 0}
    ///////////////// mepc update ///////////////////
    mepc = pc;
    ///////////////// set branch target ///////////////////
    return mtvec;
}

U32 RV32I::CSR::do_csrrx(U32 rs1, U32 imm, U32 csr_op, U32 rdID, U32 time, U32 timeh) {
    BIT skipWrite = 0, skipRead = 0;
    U32 rd = 0;
    if (csr_op & (O_csrrw | O_csrrwi)) { skipRead = (rdID == 0); }     /// (I_rd  == 0) skips csr read
    else { skipWrite = (rs1 == 0); }    /// (I_rs1 == 0) skips csr write
    if (!checkAccess(imm, !skipWrite)) {
        error = 1;
    } else {
        rd = (skipRead) ? 0 : read(imm, time, timeh);
        if (!skipWrite) {
            U32 csr_wdata = 0;
            if      (csr_op & (O_csrrw | O_csrrwi)) { csr_wdata = rs1; }
            else if (csr_op & (O_csrrs | O_csrrsi)) { csr_wdata = rd | rs1; }
            else                                    { csr_wdata = rd & ~rs1; }
            write(imm, csr_wdata);
        }
    }
    return rd;
}

void RV32I::CSR::writeback(U32 csr_val, U32 rs1, U12 imm, U8 csr_op) {
    U32 csr_wdata = 0;
    if      (csr_op & (O_csrrw | O_csrrwi)) { csr_wdata = rs1; }
    else if (csr_op & (O_csrrs | O_csrrsi)) { csr_wdata = csr_val | rs1; }
    else                                    { csr_wdata = csr_val & ~rs1; }
    write(imm & 0xfff, csr_wdata);
}

#define SET(n)         case I_##n: n = 1; return
void RV32I::INST_TYPE::set(UCHAR opcode) {
    switch (opcode) {
        SET(COMP); SET(COMPI); SET(LUI); SET(AUIPC); SET(LD); SET(ST); SET(JAL); SET(JALR); SET(BR); SET(SYS);
    }
}
#undef SET
void RV32I::OP_TYPE::set(USHORT opID) {
    ADD = (opID >> 13) & 1;
    SUB = (opID >> 12) & 1;
    LT  = (opID >> 11) & 1;
    LTU = (opID >> 10) & 1;
    AND = (opID >>  9) & 1;
    OR  = (opID >>  8) & 1;
    XOR = (opID >>  7) & 1;
    SLL = (opID >>  6) & 1;
    SRL = (opID >>  5) & 1;
    SRA = (opID >>  4) & 1;
    EQ  = (opID >>  3) & 1;
    NE  = (opID >>  2) & 1;
    GE  = (opID >>  1) & 1;
    GEU = (opID >>  0) & 1;
}
void RV32I_ARCH1::ctrl_decode() {
    dc.decode(ir, (T[2] | T[3])); /// ir_valid = (T[2] | T[3])
    I.set(dc.opcode);
    if (T[2]) { /// op/ld/st bit-vectors are generated only at T[2]
#if defined(ENABLE_MD)
        if (dc.isMD)                        { md_type = dc.opID & 0xff; }
        else if (I.COMP | I.COMPI | I.BR)   { op.set(dc.opID); }
#else
        if (I.COMP | I.COMPI | I.BR)    { op.set(dc.opID); }
#endif
        if (I.LD)                       { ld_type = dc.opID & 0x1f; }
        if (I.ST)                       { st_type = dc.opID & 0x7; }
        if (I.SYS) {
            csr_op = dc.opID & 0xff;
            csr_rw = ((dc.I_rd != 0) << 1) | (dc.I_rs1 != 0);
            if      (dc.opID == RV32I::O_wfi)      { wfi = 1; }
            else if (dc.opID == RV32I::O_mret)     { mret = 1; }
            else if (dc.opID & RV32I::O_csrr_mask) { csrrx = 1; }
        }
    }
    if (T[0] | (T[2] & (I.LUI | I.AUIPC | I.LD | I.ST | I.JAL | I.JALR)) | (T[3] & I.BR)) {
        op.set(RV32I::O_add);
    }
    if (T[0]) {
        ld_type = RV32I::O_lw;
    }
}

/// bus0 : src{pc, rs2},           dst{mem.din, rf.rd}
/// bus1 : src{pc, tr, rs1},       dst{mem.addr, tr, alu.in1}
/// bus2 : src{imm, rs2},          dst{alu.in2}
/// bus3 : src{mem.dout, alu.out}, dst{pc, mem.addr, ctrl.ir, ctrl.flag, rf.rd}

/// T[0]           : mem.addr = pc, mem.SetRead(I_LD_TNST), tr = pc, pc = alu(pc,+4,I_OP_ADD)
/// T[1]           : ir = mem.dout
/// T[2] & I_COMP  : rd = alu(rs1, rs2, op_type)
/// T[2] & I_COMPI : rd = alu(rs1, imm, op_type)
/// T[2] & I_LUI   : rd = alu(zero, imm, I_OP_ADD)
/// T[2] & I_AUIPC : rd = alu(tr, imm, I_OP_ADD)
/// T[2] & I_LD    : mem.addr = alu(rs1, imm, I_OP_ADD), mem.SetRead(ld_type)
/// T[3] & I_LD    : rd = mem.dout
/// T[2] & I_ST    : mem.addr = alu(rs1, imm, I_OP_ADD), mem.din = rs2, mem.SetWrite(st_type)
/// T[2] & I_JAL   : rd = pc, pc = alu(tr, imm, I_OP_ADD)
/// T[2] & I_JALR  : rd = pc, pc = alu(rs1, imm, I_OP_ADD)
/// T[2] & I_BR    : flg = alu(rs1, rs2, op_type)
/// T[3] & I_BR    : if (flg) pc = alu(rs1, imm, I_OP_ADD)
/// T[2] & I_SYS & I_CSRRX : rd = csrrx(imm);

void RV32I_ARCH1::updateMemIO(AXI4L::CH* axi, U32 intr) {
    mem.addr = (T[0]) ? bus1 : bus3;
    if (st_type) {
        mem.formatWrite(st_type, bus0);
    }
    ///////// IO address is defined outside of the memory address space
    BIT addr_out_of_bounds = MEM_ADDR_OUT_OF_BOUNDS(mem.addr);
    ///////// io_op[1:0] = {I.ST, I.LD} //////////////
    U2 io_op = (T[2] & (I.LD | I.ST) & addr_out_of_bounds) ? (I.LD | (I.ST << 1)) : 0;
    U2 io_sz = (io_op == 0b01) ? RV32I::ld_size(dc.opID & 0x1f) : RV32I::st_size(dc.opID & 7);
    ///////// IO access requires multiple cycles to complete : IO_stalled means IO access is pending //////////////
    io.fsm(io_op, io_sz, mem.addr, mem.din, axi, intr, dbg_state.internal_irq_clear_mask);
    IO_LD_completed = (io_op == 0b01 && !io.stalled);
    ///////// normal memory access //////////
    if (!io_op) {
        if (st_type) {          ///// write byte-shifted data with WE(write-enable)
            mem.doWrite();
        } else if (ld_type) {   ///// read 32-bit data : byte-shifting and sign/zero-extension is done in formatReadData
            mem.setRead(ld_type);
        }
    }
}
U32 RV32I_ARCH1::formatReadData() {
    ////////// IO read data is stored in axim.latched_data //////////////
    U32 dout = (IO_LD_completed) ? io.latched_data : mem.dout;
    if (!RV32I::formatRead(mem.byte_pos, mem.ld_type, dout)) {
        mem.error = 1;
    }
    return dout;
}

#if defined(ENABLE_MD)
U32 RV32I_ARCH1::MD::compute(U32 i1, U32 i2, U8 md_type) {
    U32 md_out = 0, b1, c1;
    unsigned n_stt = stt;
    pending = 0;
#if 1
    if (md_type == RV32I::O_mul) {
        switch (stt) {
        case 0:
            a = i1; b = i2; c = 0;
            n_stt = 1;
            break;
        default:
            c1 = (b & 1) ? c + a : c;
            b1 = b >> 1;
            if (!b1) {
                n_stt = 0;
            }
            md_out = c1;
            a <<= 1;
            b = b1;
            c = c1;
            break;
        }
    }
    pending = (n_stt != 0);
    stt = n_stt;
#else
    if (md_type == RV32I::O_mul) {
        md_out = i1 * i2;
    }
#endif
    return md_out;
}
#endif

#if 1
int RV32I_ARCH1::step(AXI4L::CH* axi, InterruptPin<INTR_VEC_SIZE>* intr, DebugInterface* dbg) {
    reset_sig();
    ctrl_decode();
    U32 rs1 = (T[2]) ? rf.getOperand(dc.I_rs1) : 0;
    U32 rs2 = (T[2]) ? rf.getOperand(dc.I_rs2) : 0;
    U32 imm = (T[0]) ? 4 : dc.imm;
    ///////////////// csr ///////////////////
    if (csr_rw && !csr.checkAccess(imm & 0xfff, csr_rw & 1)) {
        csr.error = 1;
    }
    if (csr_op & RV32I::O_csrr_imm_mask) {
        rs1 = dc.I_rs1;
    }
    /// bus0 : {pc, rs2}           --> {mem.din, rf.rd}
    bus0 = (I.ST) ? rs2 : pc;
    /// bus1 : {pc, tr, rs1}       --> {mem.addr, tr, alu.in1}
    bus1 = (T[2] & (I.COMP | I.COMPI | I.LUI | I.LD | I.ST | I.JALR | I.BR | csrrx)) ? rs1 : ((T[0]) ? pc : tr);
    /// bus2 : {imm, rs2}          --> {alu.in2}
    bus2 = (T[2] & (I.COMP | I.BR)) ? rs2 : imm;
    //////////// ALU : {bus1, bus2} --> {bus3} /////////////
    /// bus3 : {mem.dout, alu.out} --> {pc, mem.addr, ctrl.ir, ctrl.flag, rf.rd}
    interrupt_detected = csr.interrupt_detected();
    BIT I_LD_BR = I.LD | I.BR;
    inst_completed = (T[2] & !I_LD_BR) | (T[3] & I_LD_BR);
    updateDebugInterface(dbg);
    trap_occurred = (T[0] & interrupt_detected & !dbg_state.step_mode);  /// detect interrupt on T[0] cycle
    U32 mem_out = (T[1] | (T[3] & I.LD)) ? formatReadData() : 0;
    U32 alu_out = RV32I::ALUcompute(bus1, bus2, op);
    U32 csr_val = (csrrx && (csr_rw & 2)) ? csr.read(imm & 0xfff, io.sysctrl.mtime_l, io.sysctrl.mtime_h) : 0;
    U32 mepc = (mret) ? csr.mepc : 0;
#if defined(ENABLE_MD)
    U32 md_out = md.compute(bus1, bus2, md_type);
    bus3 = (trap_occurred) ? csr.mtvec : (mem_out | alu_out | md_out | csr_val | mepc);
#else
    bus3 = (trap_occurred) ? csr.mtvec : (mem_out | alu_out | csr_val | mepc);
#endif
    //////////// LD/ST, IO read/write /////////////
    updateMemIO(axi, intr->vec());

#if defined(TEST_WFI_DBG)
    if (dbg_state.step_mode && (wfi || (pc != dbg_state.pc) || (dbg_state.step_wfi && interrupt_detected))) {
        printf("[%6d] pc:%08x ir:%08x T:%d%d%d%d dbg_idx:%03x wfi:%d:%d wait_irq:%d irq:%d mem:%08x:%08x "
            "ld:%02x st:%x\n",
            csr.mcycle, pc, ir, T[3], T[2], T[1], T[0], (dbg->idx >> 6) & 0x3ff,
            wfi, dbg_state.step_wfi, dbg_state.step_wait_irq_clear, interrupt_detected, mem.addr, mem.dout,
            ld_type, st_type);
    }
    dbg_state.pc = pc;
    dbg_state.sc = T.sc;
#endif

    BIT br_active = (T[2] & (I.JAL | I.JALR)) | (T[3] & I.BR & flg) | (csr_op == RV32I::O_mret);

    BIT dbg_stall = dbg_state.step_wait_irq_clear | (dbg_state.step_wfi & !interrupt_detected);

#if defined(ENABLE_MD)
    BIT stall = dbg_stall | md.pending | io.stalled;
#else
    BIT stall = dbg_stall | io.stalled;
#endif

    //////////// rd writeback to RegFile //////////
    UCHAR rdID = ((T[2] & !I.LD) | (T[3] & I.LD)) ? dc.I_rd : 0;
    U32 rd = (T[2] & (I.JAL | I.JALR)) ? bus0 : bus3;
    if (!stall) {
        rf.writeBack(rdID, rd);
        //////////// csr writeback, update csr.mip (Machine Interrupt Pending) ////////////
        if (trap_occurred) { csr.do_trap(bus1); }
#if 1
        else if (csr_rw & 1) { csr.writeback(csr_val, bus1, bus2, csr_op); }
#else
        else if (csr_rw & 1) { csr.writeback(csr_val, rs1, imm & 0xfff, csr_op); }
#endif
        else if (mret) { csr.do_mret(); }
    }
    csr.mip = (io.sysctrl.meip << 11) | (io.sysctrl.mtip << 7);   /// update { mip.meip[11], mip.meip[7] }

    //////////// pc update //////////////
    if (!stall) {
        if (T[0] | br_active) { pc = bus3 & 0xfffffffe; }
        //////////// tr update //////////////
        if (T[0]) { tr = bus1; }
        //////////// ir update //////////////
        if (T[1]) { ir = bus3; }
        //////////// flg update /////////////
        if (T[2] & I.BR) { flg = bus3 & 1; }
    }
    //////////// Timer/counter update //////////////
    BIT wfi_or_step_wfi = wfi | dbg_state.step_wfi;
    if (!stall && (!dbg_state.step_wait_irq_clear) && (!wfi_or_step_wfi || interrupt_detected || dbg_state.ignore_wfi)) {
        T.clr = inst_completed | trap_occurred;
        if (T.clr) { /// end of instruction execution
#if !defined(__LLVM_C2RTL__)
            rv32i_symTable.printSymbol(tr);
#endif
            P2("[%6d:%6d] ", csr.minstret, csr.mcycle);
            dc.printDecode(tr, ir, rf.xreg, br_active, trap_occurred | mret);
        }
        T.update_counter();
    }
#if !defined(TEST_WFI_DBG)
    if (dc.error | mem.error | wfi) {
        printf("dc.error = %d, mem.error = %d, wfi = %d\n", dc.error, mem.error, wfi);
        printf("minstret = %d, mcycle = %d\n", csr.minstret, csr.mcycle);
    }
#endif
    csr.update_counter(!io.stalled & !trap_occurred & T.clr);

    return dc.error | mem.error | wfi;
}
#else
int RV32I_ARCH1::step(AXI4L::CH* axi, InterruptPin<INTR_VEC_SIZE>* intr, DebugInterface* dbg) {
    reset_sig();
    ctrl_decode();
    U32 rs1 = (T[2]) ? rf.getOperand(dc.I_rs1) : 0;
    U32 rs2 = (T[2]) ? rf.getOperand(dc.I_rs2) : 0;
    U32 imm = (T[0]) ? 4 : dc.imm;
    ///////////////// csr ///////////////////
    if (csr_rw && !csr.checkAccess(imm & 0xfff, csr_rw & 1)) {
        csr.error = 1;
    }
    if (csr_op & RV32I::O_csrr_imm_mask) {
        rs1 = dc.I_rs1;
    }
    /// bus0 : {pc, rs2}           --> {mem.din, rf.rd}
    bus0 = (I.ST) ? rs2 : pc;
    /// bus1 : {pc, tr, rs1}       --> {mem.addr, tr, alu.in1}
    bus1 = (T[2] & (I.COMP | I.COMPI | I.LUI | I.LD | I.ST | I.JALR | I.BR | csrrx)) ? rs1 : ((T[0]) ? pc : tr);
    /// bus2 : {imm, rs2}          --> {alu.in2}
    bus2 = (T[2] & (I.COMP | I.BR)) ? rs2 : imm;
    //////////// ALU : {bus1, bus2} --> {bus3} /////////////
    /// bus3 : {mem.dout, alu.out} --> {pc, mem.addr, ctrl.ir, ctrl.flag, rf.rd}
    interrupt_detected = csr.interrupt_detected();
    BIT I_LD_BR = I.LD | I.BR;
    inst_completed = (T[2] & !I_LD_BR) | (T[3] & I_LD_BR);
    updateDebugInterface(dbg);
    trap_occurred = (T[0] & interrupt_detected & !dbg_state.step_mode);  /// detect interrupt on T[0] cycle
    U32 mem_out = (T[1] | (T[3] & I.LD)) ? formatReadData() : 0;
    U32 alu_out = RV32I::ALUcompute(bus1, bus2, op);
    U32 csr_val = (csrrx && (csr_rw & 2)) ? csr.read(imm & 0xfff, io.sysctrl.mtime_l, io.sysctrl.mtime_h) : 0;
    U32 mepc = (mret) ? csr.mepc : 0;
#if defined(ENABLE_MD)
    U32 md_out = md.compute(bus1, bus2, md_type);
    bus3 = (trap_occurred) ? csr.mtvec : (mem_out | alu_out | md_out | csr_val | mepc);
#else
    bus3 = (trap_occurred) ? csr.mtvec : (mem_out | alu_out | csr_val | mepc);
#endif
    //////////// LD/ST, IO read/write /////////////
    updateMemIO(axi, intr->vec());

#if defined(TEST_WFI_DBG)
    if (dbg_state.step_mode && (wfi || (pc != dbg_state.pc) || (dbg_state.step_wfi && interrupt_detected))) {
        printf("[%6d] pc:%08x ir:%08x T:%d%d%d%d dbg_idx:%03x wfi:%d:%d wait_irq:%d irq:%d mem:%08x:%08x "
            "ld:%02x st:%x\n",
            csr.mcycle, pc, ir, T[3], T[2], T[1], T[0], (dbg->idx >> 6) & 0x3ff,
            wfi, dbg_state.step_wfi, dbg_state.step_wait_irq_clear, interrupt_detected, mem.addr, mem.dout,
            ld_type, st_type);
    }
    dbg_state.pc = pc;
    dbg_state.sc = T.sc;
#endif

    BIT br_active = (T[2] & (I.JAL | I.JALR)) | (T[3] & I.BR & flg) | (csr_op == RV32I::O_mret);

    BIT dbg_stall = dbg_state.step_wait_irq_clear | (dbg_state.step_wfi & !interrupt_detected);

    //////////// rd writeback to RegFile //////////
    UCHAR rdID = ((T[2] & !I.LD) | (T[3] & I.LD)) ? dc.I_rd : 0;
    U32 rd = (T[2] & (I.JAL | I.JALR)) ? bus0 : bus3;
    if (!dbg_stall && !md.pending) {
        rf.writeBack(rdID, rd);
        //////////// csr writeback, update csr.mip (Machine Interrupt Pending) ////////////
        if (trap_occurred) { csr.do_trap(bus1); }
        else if (csr_rw & 1) { csr.writeback(csr_val, rs1, imm & 0xfff, csr_op); }
        else if (mret) { csr.do_mret(); }
    }
    csr.mip = (io.sysctrl.meip << 11) | (io.sysctrl.mtip << 7);   /// update { mip.meip[11], mip.meip[7] }

    //////////// pc update //////////////
    if (!dbg_stall) {
        if (T[0] | br_active) { pc = bus3 & 0xfffffffe; }
        //////////// tr update //////////////
        if (T[0]) { tr = bus1; }
        //////////// ir update //////////////
        if (T[1]) { ir = bus3; }
        //////////// flg update /////////////
        if (T[2] & I.BR) { flg = bus3 & 1; }
    }
    //////////// Timer/counter update //////////////
    BIT wfi_or_step_wfi = wfi | dbg_state.step_wfi;
#if defined(ENABLE_MD)
    if ((!io.stalled) && (!md.pending) && (!dbg_state.step_wait_irq_clear) && (!wfi_or_step_wfi || interrupt_detected || dbg_state.ignore_wfi)) {
#else
    if ((!io.stalled) && (!dbg_state.step_wait_irq_clear) && (!wfi_or_step_wfi || interrupt_detected || dbg_state.ignore_wfi)) {
#endif
        T.clr = inst_completed | trap_occurred;
        if (T.clr) { /// end of instruction execution
#if !defined(__LLVM_C2RTL__)
            rv32i_symTable.printSymbol(tr);
#endif
            P2("[%6d:%6d] ", csr.minstret, csr.mcycle);
            dc.printDecode(tr, ir, rf.xreg, br_active, trap_occurred | mret);
        }
        T.update_counter();
    }
#if !defined(TEST_WFI_DBG)
    if (dc.error | mem.error | wfi) {
        printf("dc.error = %d, mem.error = %d, wfi = %d\n", dc.error, mem.error, wfi);
        printf("minstret = %d, mcycle = %d\n", csr.minstret, csr.mcycle);
    }
#endif
    csr.update_counter(!io.stalled & !trap_occurred & T.clr);

    return dc.error | mem.error | wfi;
}
#endif

#undef P
#undef P1
#undef P2

void RV32I_ARCH1::updateDebugInterface(DebugInterface* dbg) {
    dbg->wfi = wfi | (dbg_state.step_pending != 0);
    dbg->rv_state = (T[3] << 3) | (T[2] << 2) | (T[1] << 1) | (T[0]);
    switch (dbg->idx & 0x3f) {  /// idx[5:0]
    case 0x00:  dbg->data = pc;             break;
    case 0x20:  dbg->data = ir;             break;
    case 0x21:  dbg->data = csr.mstatus;    break;
    case 0x22:  dbg->data = csr.mie;        break;
    case 0x23:  dbg->data = csr.mtvec;      break;
    //case 0x23:  dbg->data = csr.mscratch;   break;
    case 0x24:  dbg->data = csr.mepc;       break;
    case 0x25:  dbg->data = csr.mcause;     break;
    //case 0x25:  dbg->data = csr.mtval;      break;
    case 0x26:  dbg->data = csr.mip;        break;
    case 0x27:  dbg->data = csr.mcycle;     break;
    case 0x28:  dbg->data = csr.mcycleh;    break;
    case 0x29:  dbg->data = csr.minstret;   break;
    case 0x2a:  dbg->data = csr.minstreth;  break;
    case 0x30:  dbg->data = io.sysctrl.ext_irq_enable;  break;
    case 0x31:  dbg->data = io.sysctrl.mtime_l;         break;
    case 0x32:  dbg->data = io.sysctrl.mtime_h;         break;
    case 0x33:  dbg->data = io.sysctrl.mtimecmp_l;      break;
    case 0x34:  dbg->data = io.sysctrl.mtimecmp_h;      break;
    default:    dbg->data = rf.xreg[dbg->idx & 0x1f];   break;  /// 0x01 - 0x1f : xreg[idx]
    }
    unsigned dbg_com = (dbg->idx >> 12) & 0xf;    /// idx[15:6]
    unsigned dbg_mode = DBG_run;
    BIT button_irq = (io.sysctrl.ext_irq_enable & io.sysctrl.ext_irq_pending & 0x7c) != 0;
    if (dbg_com & 0x8) {
        dbg_state.ignore_wfi = 1;
    }
    else if (dbg_com >= DBG_step_cycle && dbg_com <= DBG_step_out) {
        dbg_mode = dbg_com;
        dbg_state.step_mode = (button_irq | (dbg_state.step_pending != 0));
    }
    if (dbg_state.step_mode && (dbg_state.step_pending == 1)) {
        dbg_state.internal_irq_clear_mask = 0x7c;
    }
    BIT call_inst = (I.JAL | I.JALR) & (dc.I_rd == RV32I::R_ra);
    BIT return_inst = (I.JALR & (!dc.I_rd) & (dc.I_rs1 == RV32I::R_ra) & !dc.imm);
    /// (wfi) --> [interrupt_detected]
    if (wfi & interrupt_detected & button_irq & dbg_state.step_mode) {
        dbg_state.step_pending = 1; /// stall next cycle for all step-types
        if (dbg_mode == DBG_step_over) {
            dbg_mode = DBG_step_in;
        }
        dbg_state.step_halt_condition = dbg_mode;   /// { step_cycle, step_in, step_over, step_out }
        dbg_state.step_target_pc = 0;// (dbg_mode == DBG_step_over) ? pc + 4 : 0;
#if defined(TEST_WFI_DBG)
        printf("[%6d:%08x:%d ] TEST_WFI_DBG : step_pending : 0 -> 1 (dbg_mode: 0x%x)\n", csr.mcycle, pc, T.sc, dbg_mode);
#endif
    }
    /// (step_wfi) --> [interrupt_detected]
    else if (dbg_state.step_pending == 3) {
        dbg_state.step_wfi = 1;
        if (interrupt_detected & button_irq) {
#if defined(TEST_WFI_DBG)
            printf("[%6d:%08x:%d*] TEST_WFI_DBG : step_pending : 3 -> 1 (dbg_mode: 0x%x)\n",
                csr.mcycle, pc, T.sc, dbg_mode);
#endif
            if (dbg_mode == DBG_step_over && !call_inst) {
                dbg_mode = DBG_step_in;
            }
            dbg_state.step_pending = dbg_state.step_mode;
            dbg_state.step_halt_condition = dbg_mode;   /// { step_cycle, step_in, step_over, step_out }
            dbg_state.step_target_pc = (dbg_mode == DBG_step_over) ? pc + 4 : 0;
        }
    }
    /// (*wfi) --> [interrupt_detected] --> [wait_irq_clear]
    else if (dbg_state.step_pending == 1) {
        dbg_state.step_wait_irq_clear = 1;
        if (!interrupt_detected) {  /// halt until interrupt is disabled internally
            if (dbg_state.step_halt_condition == DBG_step_cycle) {
                dbg_state.step_pending = 3; /// dbg_step_wfi is restored
                dbg_state.step_wfi = 1;
#if defined(TEST_WFI_DBG)
                printf("[%6d:%08x:%d ] TEST_WFI_DBG : step_pending : 1 -> 3\n",
                    csr.mcycle, pc, T.sc);
#endif
            }
            else {  /// step_in, step_over, step_out
                dbg_state.step_pending = 2; /// temporarily release step until halting condition
#if defined(TEST_WFI_DBG)
                printf("[%6d:%08x:%d ] TEST_WFI_DBG : step_pending : 1 -> 2\n", csr.mcycle, pc, T.sc);
#endif
            }
        }
    }
    /// (*wfi) --> [interrupt_detected] --> [wait_irq_clear] --> [halt_cond_x]
    else if (dbg_state.step_pending == 2) { /// dbg_step_wfi is temporarily released 
        if (T[0]) {
            //BIT return_completed = (I.JALR & (!dc.I_rd) & (dc.I_rs1 == RV32I::R_ra) & !dc.imm);
            BIT halt_cond_0 = (dbg_state.step_halt_condition == DBG_step_in);
            BIT halt_cond_1 = (dbg_state.step_halt_condition == DBG_step_over) & (dbg_state.step_target_pc == pc);
            BIT halt_cond_2 = (dbg_state.step_halt_condition == DBG_step_out) & (dbg_state.step_target_pc == 0) & return_inst;
            if (halt_cond_0 | halt_cond_1 | halt_cond_2) {
                dbg_state.step_pending = 3; /// dbg_step_wfi is restored
                dbg_state.step_wfi = 1;
#if defined(TEST_WFI_DBG)
                printf("[%6d:%08x:%d ] TEST_WFI_DBG : step_pending : 2 -> 3 (halt_cond:%d%d%d)\n",
                    csr.mcycle, pc, T.sc, halt_cond_0, halt_cond_1, halt_cond_2);
#endif
            }
            else if ((dbg_state.step_halt_condition == DBG_step_out) && (dbg_state.step_target_pc == pc)) {
                dbg_state.step_target_pc = 0;
#if defined(TEST_WFI_DBG)
                printf("[%6d:%08x:%d ] TEST_WFI_DBG : step_out (reached    %08x)\n",
                    csr.mcycle, pc, T.sc, pc);
#endif
            }
            else {
            }
        }
        else if (T[2]) {
            if (dbg_state.step_halt_condition == DBG_step_out) {
                if ((dbg_state.step_target_pc == 0) && call_inst) {
                    dbg_state.step_target_pc = pc;  /// next pc after call
#if defined(TEST_WFI_DBG)
                    printf("[%6d:%08x:%d ] TEST_WFI_DBG : step_out (skip until %08x)\n",
                        csr.mcycle, pc, T.sc, pc);
#endif
                }
            }
        }
    }
    dbg_state.wfi = wfi | dbg_state.step_wfi;
}


