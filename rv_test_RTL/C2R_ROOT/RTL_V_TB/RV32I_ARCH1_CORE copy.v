`include "RV32I_DEF1.v"

module RV32I_ARCH1_CORE   (
    /*input ports*/
    clk, rst_n,
    IO_active, IO_pending, IO_dout, SYS_mtip, SYS_meip, SYS_mtime_l, SYS_mtime_h,
    /*output ports*/
    MEM_addr, MEM_din, SYS_op, AXI_op, IO_size, 
    G_RV32I_ARCH1_step_OUT
  );
  /*input ports*/
  input        clk, rst_n, IO_active, IO_pending;
  input [31:0] IO_dout, SYS_mtime_l, SYS_mtime_h;
  input        SYS_mtip, SYS_meip;
  /*output ports*/
  output[31:0] MEM_addr, MEM_din;
  output[ 1:0] SYS_op, AXI_op, IO_size;
  output       G_RV32I_ARCH1_step_OUT;

    /********************* registers ***************************/
    reg [31:0] IR, PC, TR, XREG[1:31];
    reg FLG;
    /********************* wires ***************************/
    wire [ 3:0] MEM_we;
    wire [31:0] MEM_dout;
    wire [ 9:0] I; /// { COMP, COMPI, LUI, AUIPC, LD, ST, JAL, JALR, BR, SYS }
    wire [ 4:0] I_rd, I_rs1, I_rs2, RF_rd;
    wire [31:0] imm;
    wire [13:0] OP_type;   /// { ADD, SUB, LT, LTU, AND, OR, XOR, SLL, SRL, SRA, EQ, NE, GE, GEU }
    wire [ 4:0] LD_type;   /// { LB, LBU, LH, LHU, LW }
    wire [ 2:0] ST_type;   /// { SB, SH, SW }
    wire [ 7:0] SYS_type;  /// { CSRRW, CSRRS, CSRRC, CSRRWI, CSRRSI, CSRRCI, MRET, WFI }
    wire [ 7:0] MD_type;   /// { MUL, MULH, MULHSU, MULHU, DIV, DIVU, REM, REMU }
    wire [ 3:0] T;  /// T[0] -> T[1] -> T[2] -> T[3] : timing variable
    wire CSRRXI, CSRRX, MD_pending, INTR_detected, TRAP_occurred, INST_completed;
    wire rs1_sel, stall, SC_EN, SC_CLR, I_LD_BR, PC_LD;
    wire [1:0] MEM_op;
    wire [31:0] rs1, rs2, rd, ALU_out, MD_out, CSR_out, mtvec, LD_dout, MEM_out, bus0, bus1, bus2, bus3;

    /********************* wire continuous assignments ***************************/
    assign CSRRXI = SYS_type[`CSRRWI] | SYS_type[`CSRRSI] | SYS_type[`CSRRCI];
    assign CSRRX  = CSRRXI | SYS_type[`CSRRW] | SYS_type[`CSRRS] | SYS_type[`CSRRC];

    assign rs1 = (T[2] & (I_rs1 != 5'b0)) ? ((CSRRXI) ? I_rs1 : XREG[I_rs1]) : 32'b0;
    assign rs2 = (T[2] & (I_rs2 != 5'b0)) ? XREG[I_rs2] : 32'b0;
    assign rs1_sel = I[`COMP] | I[`COMPI] | I[`LUI] | I[`LD] | I[`ST] | I[`JALR] | I[`BR] | CSRRX;
    assign bus0 = (I[`ST]) ? rs2 : PC;
    assign bus1 = (T[2] & rs1_sel) ? rs1 : (T[0]) ? PC : TR;
    assign bus2 = (T[2] & (I[`COMP] | I[`BR])) ? rs2 : imm;
    assign bus3 = (TRAP_occurred) ? mtvec : (MEM_out | ALU_out | MD_out | CSR_out);

    assign I_LD_BR = I[`LD] | I[`BR];
    assign stall = MD_pending | IO_pending;
    assign SC_EN = (~stall) & (~SYS_type[`WFI] | INTR_detected);
    assign INST_completed = ((T[2] & ~I_LD_BR) | (T[3] & I_LD_BR)) & SC_EN;
    assign SC_CLR = INST_completed | TRAP_occurred;

    assign MEM_out = (T[1] | (T[3] & I[`LD])) ? LD_dout : 32'b0;
    assign MEM_addr = (T[0]) ? bus1 : bus3;
    
    assign PC_LD = T[0] | (T[2] & (I[`JAL] | I[`JALR])) | (T[3] & I[`BR] & FLG) | SYS_type[`MRET];
    assign RF_rd = ((T[2] & ~I[`LD]) | (T[3] & I[`LD])) ? I_rd : 5'b0;
    assign rd = (T[2] & (I[`JAL] | I[`JALR])) ? bus0 : bus3;

    assign G_RV32I_ARCH1_step_OUT = SYS_type[`WFI];

    /********************* module instantiation ***************************/
    RV32_DC M0 (IR, T, I, I_rd, I_rs1, I_rs2, imm, OP_type, LD_type, ST_type, SYS_type, MD_type);
    INST_TIMER M1 (clk, rst_n, SC_EN, SC_CLR, T);
    ALU M2 (bus1, bus2, OP_type, ALU_out);
    MD M3 (clk, rst_n, bus1, bus2, MD_type, MD_out, MD_pending);
    CSR M4 (clk, rst_n, T, INST_completed, SYS_type, imm[11:0], I[`SYS] & (|RF_rd), rs1, bus1, 
        CSR_out, mtvec, INTR_detected, TRAP_occurred, SYS_mtip, SYS_meip, SYS_mtime_l, SYS_mtime_h);
    MEM_IO_SELECTOR M5 (LD_type, ST_type, MEM_addr, MEM_op, SYS_op, AXI_op, IO_size);
    FORMAT_READ  M6 (clk, (|MEM_op), MEM_addr, LD_type, MEM_dout, IO_dout, LD_dout);
    FORMAT_WRITE M7 (MEM_addr, ST_type, bus0, MEM_din, MEM_we);
    MEMORY M8 (clk, MEM_we, MEM_din, (|MEM_op), MEM_addr[17:2], MEM_dout);

    /********************* register assignments ***************************/
    always @ (posedge clk or negedge rst_n) begin
        if (rst_n == 1'b0) begin
            PC <= 32'h1000;
            TR <= 32'b0;
            IR <= 32'b0;
            FLG <= 1'b0;
        end
        else if (~stall) begin
            if (RF_rd != 5'b0)
                XREG[RF_rd] <= rd;
            if (PC_LD)
                PC <= { bus3[31:1], 1'b0 };
            if (T[0])
                TR <= bus1;
            if (T[1])
                IR <= bus3;
            if (T[2] & I[`BR])
                FLG <= bus3[0];
        end
    end

endmodule // RV32I_ARCH1_CORE

module RV32_DC (ir, T, I, I_rd, I_rs1, I_rs2, imm, OP_type, LD_type, ST_type, SYS_type, MD_type);
input  [31:0] ir;
input  [ 3:0] T;
output [ 9:0] I;
output [ 4:0] I_rd, I_rs1, I_rs2;
output [31:0] imm;
output [13:0] OP_type;
output [ 4:0] LD_type;
output [ 2:0] ST_type;
output [ 7:0] SYS_type, MD_type;

wire [6:0] opcode = ir[6:0];
wire [6:0] funct7 = ir[31:25];
wire [2:0] funct3 = ir[14:12];

function [10:0] DEC_I;
    input [6:0] opcode;
    case(opcode)
        7'b0110011: DEC_I = (10'b1 <<`COMP);
        7'b0010011: DEC_I = (10'b1 <<`COMPI);
        7'b0110111: DEC_I = (10'b1 <<`LUI);
        7'b0010111: DEC_I = (10'b1 <<`AUIPC);
        7'b0000011: DEC_I = (10'b1 <<`LD);
        7'b0100011: DEC_I = (10'b1 <<`ST);
        7'b1101111: DEC_I = (10'b1 <<`JAL);
        7'b1100111: DEC_I = (10'b1 <<`JALR);
        7'b1100011: DEC_I = (10'b1 <<`BR);
        7'b1110011: DEC_I = (10'b1 <<`SYS);
        default:    DEC_I = 10'b0;
    endcase
endfunction

    assign I = DEC_I(opcode);
    wire R_type = I[`COMP];
    wire I_type = I[`COMPI] | I[`LD] | I[`JALR] | I[`SYS];
    wire U_type = I[`LUI]   | I[`AUIPC];
    wire S_type = I[`ST];
    wire J_type = I[`JAL];
    wire B_type = I[`BR];

    assign I_rd  = (R_type | I_type | U_type | J_type) ? ir[11: 7] : 5'b00000;
    assign I_rs1 = (R_type | I_type | S_type | B_type) ? ir[19:15] : 5'b00000;
    assign I_rs2 = (R_type | S_type | B_type)          ? ir[24:20] : 5'b00000;

    wire [31:0] imm_i = (I_type) ? {{20{ir[31]}}, ir[31:20]} : 32'b0;
    wire [31:0] imm_s = (S_type) ? {{20{ir[31]}}, ir[31:25], ir[11:7]} : 32'b0;
    wire [31:0] imm_b = (B_type) ? {{20{ir[31]}}, ir[7], ir[30:25], ir[11:8], 1'b0} : 32'b0;
    wire [31:0] imm_j = (J_type) ? {{12{ir[31]}}, ir[19:12], ir[20], ir[30:21], 1'b0} : 32'b0;
    wire [31:0] imm_u = (U_type) ? {ir[31:12], 12'b0} : 32'b0;
    assign      imm = (T[0]) ? 32'd4 : (imm_i | imm_s | imm_b | imm_j | imm_u);
    wire        md_flag = (funct7 == 7'b0000001) & I[`COMP];

function [13:0] DEC_COMP;
    input I_COMPI;
    input [6:0] funct7;
    input [2:0] funct3;
    reg f7_0, f7_32;
    begin
    f7_0  = (funct7 == 7'b0000000);
    f7_32 = (funct7 == 7'b0100000);
    case(funct3)
        3'b000:     DEC_COMP = (I_COMPI | f7_0) ? (14'b1 << `ADD) : (f7_32) ? (14'b1 << `SUB) : 14'b0;
        3'b010:     DEC_COMP = (14'b1 << `LT);
        3'b011:     DEC_COMP = (14'b1 << `LTU);
        3'b111:     DEC_COMP = (14'b1 << `AND);
        3'b110:     DEC_COMP = (14'b1 << `OR);
        3'b100:     DEC_COMP = (14'b1 << `XOR);
        3'b001:     DEC_COMP = (14'b1 << `SLL);
        default:    DEC_COMP = (f7_0) ? (14'b1 << `SRL) : (f7_32) ? (14'b1 << `SRA) : 14'b0;
    endcase
    end
endfunction

function [13:0] DEC_BR;
    input [2:0] funct3;
    begin
    case(funct3)
        3'b000:     DEC_BR = (14'b1 << `EQ);
        3'b001:     DEC_BR = (14'b1 << `NE);
        3'b100:     DEC_BR = (14'b1 << `LT);
        3'b110:     DEC_BR = (14'b1 << `LTU);
        3'b101:     DEC_BR = (14'b1 << `GE);
        3'b111:     DEC_BR = (14'b1 << `GEU);
        default:    DEC_BR = 14'b0;
    endcase
    end
endfunction

function [4:0] DEC_LD;
input [2:0] funct3;
    case(funct3)
        3'b000:     DEC_LD = (5'b1 << `LB);
        3'b100:     DEC_LD = (5'b1 << `LBU);
        3'b001:     DEC_LD = (5'b1 << `LH);
        3'b101:     DEC_LD = (5'b1 << `LHU);
        3'b010:     DEC_LD = (5'b1 << `LW);
        default:    DEC_LD = 5'b0;
    endcase
endfunction

function [2:0] DEC_ST;
input [2:0] funct3;
    case(funct3)
        3'b000:     DEC_ST = (3'b1 << `SB);
        3'b001:     DEC_ST = (3'b1 << `SH);
        3'b010:     DEC_ST = (3'b1 << `SW);
        default:    DEC_ST = 3'b0;
    endcase
endfunction

function [7:0] DEC_SYS;
input [11:0] imm12;
input [2:0] funct3;
input zero_19_7;
    case(funct3)
        3'b001: DEC_SYS = (8'b1 << `CSRRW);
        3'b010: DEC_SYS = (8'b1 << `CSRRS);
        3'b011: DEC_SYS = (8'b1 << `CSRRC);
        3'b101: DEC_SYS = (8'b1 << `CSRRWI);
        3'b110: DEC_SYS = (8'b1 << `CSRRSI);
        3'b111: DEC_SYS = (8'b1 << `CSRRCI);
        default:
            DEC_SYS = (zero_19_7 & (imm12 == 12'h302)) ? (8'b1 << `MRET) :
                           (zero_19_7 & (imm12 == 12'h105)) ? (8'b1 << `WFI) : 8'b0;
    endcase
endfunction

function [7:0] DEC_MD;
input [2:0] funct3;
    case(funct3)
        3'b000:  DEC_MD = (8'b1 << `MUL);
        3'b001:  DEC_MD = (8'b1 << `MULH);
        3'b010:  DEC_MD = (8'b1 << `MULHSU);
        3'b011:  DEC_MD = (8'b1 << `MULHU);
        3'b100:  DEC_MD = (8'b1 << `DIV);
        3'b101:  DEC_MD = (8'b1 << `DIVU);
        3'b110:  DEC_MD = (8'b1 << `REM);
        default: DEC_MD = (8'b1 << `REMU);
    endcase
endfunction

    wire [13:0] COMP_type = (T[2] & (~md_flag) & (I[`COMP] | I[`COMPI])) ? DEC_COMP(I[`COMPI], funct7, funct3) : 14'b0;
    wire [13:0] BR_type = (T[2] & I[`BR]) ? DEC_BR(funct3) : 14'b0;
    wire add_cond = T[0] | (T[2] & (I[`LUI] | I[`AUIPC] | I[`LD] | I[`ST] | I[`JAL] | I[`JALR])) | (T[3] & I[`BR]);
    assign OP_type = (add_cond) ? (14'b1 << `ADD) : (COMP_type | BR_type);
    assign LD_type = (T[2] & I[`LD]) ? DEC_LD(funct3) : (T[0]) ? (5'b1 << `LW) : 5'b0;
    assign ST_type = (T[2] & I[`ST]) ? DEC_ST(funct3) : 3'b0;
    assign SYS_type = (T[2] & I[`SYS]) ? DEC_SYS(ir[31:20], funct3, (ir[19:7] == 13'b0)) : 8'b0;
    assign MD_type = (T[2] & md_flag & I[`COMP]) ? DEC_MD(funct3) : 8'b0;

endmodule   /// RV32I_DC

module ALU (x, y, OP_type, out);
input [31:0] x, y;
input [13:0] OP_type;
output [31:0] out;

function [31:0] SLL;
input [31:0] x;
input [4:0] y;
reg [31:0] x1, x2, x4, x8;
begin
    x1  = (y[0]) ? {  x[30:0], 1'b0 } : x;
    x2  = (y[1]) ? { x1[29:0], 2'b0 } : x1;
    x4  = (y[2]) ? { x2[27:0], 4'b0 } : x2;
    x8  = (y[3]) ? { x4[23:0], 8'b0 } : x4;
    SLL = (y[4]) ? { x8[15:0], 16'b0 } : x8;
end
endfunction

function [31:0] SRX;
input [31:0] x;
input [4:0] y;
input sra;
reg [31:0] x1, x2, x4, x8;
reg t;
begin
    t = x[31] & sra;
    x1  = (y[0]) ? { t, x[31:1] } : x;
    x2  = (y[1]) ? { {2{t}}, x1[31:2] } : x1;
    x4  = (y[2]) ? { {4{t}}, x2[31:4] } : x2;
    x8  = (y[3]) ? { {8{t}}, x4[31:8] } : x4;
    SRX = (y[4]) ? { {16{t}}, x8[31:16] } : x8;
end
endfunction

    wire        sub = (OP_type[`SUB] | OP_type[`LTU] | OP_type[`GEU] | OP_type[`LT] | OP_type[`GE]);
    wire [31:0] y2 = (sub) ? ~y : y;
    wire [32:0] as33 = {1'b0, x} + {1'b0, y2} + sub;
    wire        c31 = as33[32];
    wire [31:0] as32 = as33[31:0];
    wire [31:0] exor = x ^ y;
    wire        ne = (|exor);
    wire        sign_x = x[31];
    wire        sign_y = y[31];
    wire        ge = c31 ^ sign_x ^ sign_y;
    wire [31:0] a_as32 = (OP_type[`ADD] | OP_type[`SUB]) ? as32 : 32'b0;
    wire [31:0] a_and = (OP_type[`AND]) ? (x & y) : 32'b0;
    wire [31:0] a_or  = (OP_type[`OR]) ? (x | y) : 32'b0;
    wire [31:0] a_xor = (OP_type[`XOR]) ? exor : 32'b0;
    wire [31:0] a_sll = (OP_type[`SLL]) ? SLL(x, y[4:0]) : 32'b0;
    wire [31:0] a_srx = (OP_type[`SRL] | OP_type[`SRA]) ? SRX(x, y[4:0], OP_type[`SRA]) : 32'b0;
    wire        cmp = (OP_type[`LTU] & ~c31) | (OP_type[`GEU] & c31) |
                      (OP_type[`LT]  & ~ge)  | (OP_type[`GE]  & ge)  |
                      (OP_type[`NE]  & ne)   | (OP_type[`EQ]  & ~ne);
    assign out = a_as32 | a_and | a_or | a_xor | a_sll | a_srx | {31'b0, cmp};

endmodule   /// ALU

module MD (clk, rst_n, x, y, MD_type, out, pending); /// MD: Multiply/Divide module
input clk, rst_n;
input [31:0] x, y;
input [7:0] MD_type;
output [31:0] out;
output pending;
    reg stt;
    reg [31:0] a, b, c;

    wire [31:0] a1 = (~stt) ? x : a << 1;
    wire [31:0] b1 = (~stt) ? y : b >> 1;
    wire [31:0] c1 = (~stt) ? 32'b0 : (b & 1) ? c + a : c;

    always @ (posedge clk or negedge rst_n)
    if (rst_n == 1'b0) begin
        stt <= 1'b0;
        a <= 32'b0;
        b <= 32'b0;
        c <= 32'b0;
    end
    else if (MD_type[`MUL]) begin /// currently, only MUL instruction is implemented
        if (~stt)
            stt <= 1'b1;
        else if (b1 == 32'b0)
            stt <= 1'b0;
        a <= a1;
        b <= b1;
        c <= c1;
    end
    else if (MD_type[`DIV]) begin
        
    end
    assign pending = MD_type[`MUL] & ((~stt) | (stt & (b1 != 32'b0)));
    assign out = (stt) ? c1 : 32'b0;
    module DIVU_REMU(
        x,
        y,
        quotient,
        remainder
        clk,
        rst_n,
    ); 
        input [31:0] x; //被除数
        input [31:0] y; //除数
        output [31:0] quotient; //商
        output [31:0] remainder; //あまり
        
        reg stt; //スタートフラグ
        reg [32:0] a //計算経過
        reg [31:0] divisor //保持する除数
        wire flag;
        flag = (~stt) ? 0 : a[32];
        always @(posedge clk or negedge rst_n) begin
            if(rst_n == 0) begin
                //reg reset
                stt <= 1'b0;
                a <= 33'b0;
                divisor <= 32'b0;
            end
            else if (~stt)
        end
    endmodule
endmodule


module INST_TIMER (clk, rst_n, en, clr, T);
input clk, rst_n;
input en, clr;
output [3:0] T;

    reg [1:0] SC;
    always @ (posedge clk or negedge rst_n)
    if (rst_n == 1'b0)
        SC <= 2'b0;
    else if (en)
        SC <= (clr) ? 2'b0 : SC + 1;

    assign T[0] = (SC == 2'b00);
    assign T[1] = (SC == 2'b01);
    assign T[2] = (SC == 2'b10);
    assign T[3] = (SC == 2'b11);
endmodule   /// INST_TIMER

module CSR (clk, rst_n, T, instret_flag, SYS_type, csr_addr, rd_flag, rs1, pc, csr_out, mtvec,
            intr_detected, trap_occurred, sys_mtip, sys_meip, sys_mtime_l, sys_mtime_h);
input clk, rst_n;
input [3:0] T;
input rd_flag, sys_mtip, sys_meip, instret_flag;
input [7:0] SYS_type;
input [11:0] csr_addr;
input [31:0] rs1, pc, sys_mtime_l, sys_mtime_h;
output [31:0] csr_out, mtvec;
output intr_detected, trap_occurred;

reg [31:0] mstatus, mie, mtvec, mepc, mcause, mtval, mip, mcycle, mcycleh, minstret, minstreth;

function [31:0] READ_CSR;
input [11:0] csr_addr;
    case (csr_addr)
        12'h300: READ_CSR = mstatus;
        12'h304: READ_CSR = mie;
        12'h305: READ_CSR = mtvec;
        12'h341: READ_CSR = mepc;
        12'h342: READ_CSR = mcause;
        12'h343: READ_CSR = mtval;
        12'h344: READ_CSR = mip;
        12'hb00: READ_CSR = mcycle;
        12'hb02: READ_CSR = minstret;
        12'hb80: READ_CSR = mcycleh;
        12'hb82: READ_CSR = minstreth;
        12'hc00: READ_CSR = mcycle;
        12'hc01: READ_CSR = sys_mtime_l;
        12'hc02: READ_CSR = minstret;
        12'hc80: READ_CSR = mcycleh;
        12'hc81: READ_CSR = sys_mtime_h;
        12'hc82: READ_CSR = minstreth;
        default: READ_CSR = 32'b0;
    endcase
endfunction

    wire [31:0] irq_vec = mie & mip;

    assign intr_detected = mstatus[3] & (irq_vec != 32'b0);
    assign trap_occurred = intr_detected & T[0];
    assign csr_out = (SYS_type[`MRET]) ? mepc : (rd_flag) ? READ_CSR(csr_addr) : 32'b0;

    wire csrrw = (SYS_type[`CSRRW] | SYS_type[`CSRRWI]);
    wire csrrs = (SYS_type[`CSRRS] | SYS_type[`CSRRSI]);
    wire csrrc = (SYS_type[`CSRRC] | SYS_type[`CSRRCI]);
    //wire [31:0] csr_wdata = (csrrw) ? rs1 : (csrrs) ? csr_out | rs1 : csr_out & ~rs1;
    wire [31:0] csr_wdata = (csrrw) ? rs1 : (csrrs) ? READ_CSR(csr_addr) | rs1 : READ_CSR(csr_addr) & ~rs1; // bug fixed by LI in 2025
    wire csr_w = (csrrw | csrrs | csrrc) & (|rs1);

    wire mcycle_w    = (csr_w) & ((csr_addr == 12'hb00) | (csr_addr == 12'hc00));
    wire mcycleh_w   = (csr_w) & ((csr_addr == 12'hb80) | (csr_addr == 12'hc80));
    wire minstret_w  = (csr_w) & ((csr_addr == 12'hb02) | (csr_addr == 12'hc02));
    wire minstreth_w = (csr_w) & ((csr_addr == 12'hb82) | (csr_addr == 12'hc82));

    wire [63:0] nxt_mcycle   = { mcycleh, mcycle }    + 64'b1;
    wire [64:0] nxt_minstret = { minstreth, minstret } + {63'b0, instret_flag};

 
    always @ (posedge clk or negedge rst_n)
    if (rst_n == 1'b0) begin
        mstatus   <= 32'b0;
        mie       <= 32'b0;
        mtvec     <= 32'b0;
        mepc      <= 32'b0;
        mcause    <= 32'b0;
        mtval     <= 32'b0;
        mip       <= 32'b0;
        mcycle    <= 32'b0;
        mcycleh   <= 32'b0;
        minstret  <= 32'b0;
        minstreth <= 32'b0;
    end
    else begin
        if (trap_occurred) begin
            $display("trap_occurred : mstatus:%h mie:%h mip:%h mtvec:%h", mstatus, mie, mip, mtvec);
            if (irq_vec[7])
                ///mcause <= 32'h80000080;
                mcause <= 32'h80000007; /// Timer mcause code = 7. Added in Apr.2026 by DJ
            else if (irq_vec[11])
                ///mcause <= 32'h80000800;
                mcause <= 32'h8000000b; ///External mcause code = 11. Added in Apr.2026 by DJ
            mtval <= 0;
            mstatus <= (mstatus[3] << 7) | (32'b11 << 11);  /// mstatus : {mpie[7] <- mie, mpp[21:11] <- mode(3), mie[3] <- 0}
            mepc <= pc;
        end
        else if (SYS_type[`MRET]) begin   /// Added in Apr.2026 by DJ
            mstatus[3] <= mstatus[7];  /// MIE  <- MPIE
            mstatus[7] <= 1'b1;        /// MPIE <- 1
            mstatus[12:11] <= 2'b00;   /// MPP  <- U-mode 
        end
        else if (csrrw | csrrs | csrrc) begin
            case (csr_addr)
                12'h300: mstatus   <= csr_wdata & 32'b0000_0000_0000_0000_0001_1000_1000_1000;
                12'h304: mie       <= csr_wdata & 32'b0000_0000_0000_0000_0000_1000_1000_1000;
                12'h305: mtvec     <= csr_wdata & 32'b1111_1111_1111_1111_1111_1111_1111_1100;
                12'h341: mepc      <= csr_wdata;
                12'h342: mcause    <= csr_wdata;
                12'h343: mtval     <= csr_wdata;
            endcase
            //$display("csrrw:%b csrrs:%b csrrc:%b mstatus:%h csr_addr:%h csr_wdata:%h",
            //            csrrw, csrrs, csrrc, mstatus, csr_addr, csr_wdata);
        end
        mip[11] <= sys_meip;    /// sysctrl : ext_interrupt_pending
        mip[7]  <= sys_mtip;    /// sysctrl : timer_interrupt_pending
        mcycle    <= (mcycle_w)   ? csr_wdata : nxt_mcycle[31: 0];
        mcycleh   <= (mcycleh_w)  ? csr_wdata : nxt_mcycle[63:32];
        minstret  <= (minstret_w)  ? csr_wdata : nxt_minstret[31: 0];
        minstreth <= (minstreth_w) ? csr_wdata : nxt_minstret[63:32];
    end

endmodule   /// CSR

module MEM_IO_SELECTOR (LD_type, ST_type, addr, mem_op, sys_op, axi_op, size);
input  [4:0]  LD_type;
input  [2:0]  ST_type;
input  [31:0] addr;
output [1:0]  mem_op, sys_op, axi_op, size;

    wire ld_active = (|LD_type);
    wire st_active = (|ST_type);
    wire addr_out_of_bounds = ((addr >> 18) != 0);  /// memory address : 18 bits --> 2^18 = 256 KBytes
    wire [1:0] op = { st_active, ld_active };
    assign mem_op = (addr_out_of_bounds) ? 2'b0 : op;

    wire sys_flag = (addr[31:5] == (32'h10001000 >> 5)); /// sysctrl : 0x10001000 - 0x1000101f

    assign sys_op = (addr_out_of_bounds & sys_flag)  ? op : 2'b0;
    assign axi_op = (addr_out_of_bounds & ~sys_flag) ? op : 2'b0;

    wire [1:0] sz_st = (st_active) ? ((ST_type[`SB]) ? 2'b00 : (ST_type[`SH]) ? 2'b01 : 2'b10) : 2'b00;
    wire [1:0] sz_ld = (ld_active) ? ((LD_type[`LB] | LD_type[`LBU]) ? 2'b00 : 
                        (LD_type[`LH] | LD_type[`LHU]) ? 2'b01 : 2'b10) : 2'b00;
    assign size = sz_st | sz_ld;

endmodule   /// MEM_IO_SELECTOR

module FORMAT_READ (clk, MEM_active, MEM_addr, LD_type, MEM_dout, IO_dout, LD_dout);
input clk, MEM_active;
input [31:0] MEM_addr, MEM_dout, IO_dout;
input [4:0] LD_type;
output [31:0] LD_dout;

    reg[31:0] MEM_addr_d;
    reg[4:0]  LD_type_d;
    reg       MEM_active_d;
    always @ (posedge clk) begin
        MEM_addr_d <= MEM_addr;
        LD_type_d <= LD_type;
        MEM_active_d <= MEM_active;
    end
    wire [31:0] dout = (MEM_active_d) ? MEM_dout : IO_dout;

    wire  [1:0] byte_pos = MEM_addr_d[1:0];
    wire  [4:0] sft_val = { byte_pos, 3'b0 };
    wire  [1:0] sz = (LD_type_d[`LB] | LD_type_d[`LBU]) ? 2'b00 : 
                    (LD_type_d[`LH] | LD_type_d[`LHU]) ? 2'b01 : 2'b10;
    wire  [4:0] msb_pos = (sz == 2'b00) ? 5'd7 : (sz == 2'b01) ? 5'd15 : 5'd31;
    wire [31:0] mask = (sz == 2'b00) ? 32'hff : (sz == 2'b01) ? 32'hffff : 32'hffffffff;
    wire [31:0] sext_mask = ~mask;
    wire [31:0] dout2 = (dout >> sft_val) & mask;
    wire        sext_flag = (LD_type_d[`LB] | LD_type_d[`LH]) & dout2[msb_pos];
    wire        LD_active = (|LD_type_d);    /// LD_type != 0

    assign LD_dout = (LD_active) ? ((sext_flag) ? (dout2 | sext_mask) : dout2) : 32'b0;
    
endmodule /// FORMAT_READ

module FORMAT_WRITE (addr, ST_type, data, data_formatted, byte_enable);
input [31:0] addr, data;
input [2:0] ST_type;
output [31:0] data_formatted;
output [3:0] byte_enable;

    wire [1:0] byte_pos = addr[1:0];
    wire [4:0] sft_val = { byte_pos, 3'b0 };
    wire [1:0] sz = (ST_type[`SB]) ? 2'b00 : 
                    (ST_type[`SH]) ? 2'b01 : 2'b10;
    wire st_active = (|ST_type);    /// ST_type != 0
    assign data_formatted = (st_active) ? (data << sft_val) : 32'b0;
    assign byte_enable = (~st_active) ? 4'b0 : (sz == 2'b00) ? (4'b0001 << byte_pos) : (sz == 2'b01) ? (4'b0011 << byte_pos) : 4'b1111;

endmodule /// FORMAT_WRITE

`include "RV32I_ARCH1_step_MemInitFileName.v"

module MEMORY (clk, we, din, ce, addr, dout);
  parameter MEM_SIZE = 65536; // default memory size
  parameter ADDR_WIDTH = 16; // default memory address width
  parameter DATA_WIDTH = 32; // default memory data width
  parameter WE_WIDTH   =  4; // default write-enable width
  parameter BYTE_WIDTH = DATA_WIDTH / WE_WIDTH; // byte width

  input                  clk ;
  input [  WE_WIDTH-1:0] we ;
  input [DATA_WIDTH-1:0] din ;
  input                  ce  ;
  input [ADDR_WIDTH-1:0] addr;
  output[DATA_WIDTH-1:0] dout;
  reg   [BYTE_WIDTH-1:0] mem0[0:MEM_SIZE-1], mem1[0:MEM_SIZE-1], mem2[0:MEM_SIZE-1], mem3[0:MEM_SIZE-1];
  reg   [BYTE_WIDTH-1:0] bout0, bout1, bout2, bout3;

initial   $readmemh(`MEM_INIT_G__this_mem_m_0, mem0);
initial   $readmemh(`MEM_INIT_G__this_mem_m_1, mem1);
initial   $readmemh(`MEM_INIT_G__this_mem_m_2, mem2);
initial   $readmemh(`MEM_INIT_G__this_mem_m_3, mem3);

  always @ (posedge clk)
  if (ce == 1'b1) begin
    if (we[0] == 1'b1) begin
      mem0[addr] <= din[BYTE_WIDTH*1-1:BYTE_WIDTH*0];
    end
    else bout0   <= mem0[addr];
    if (we[1] == 1'b1) begin
      mem1[addr] <= din[BYTE_WIDTH*2-1:BYTE_WIDTH*1];
    end
    else bout1   <= mem1[addr];
    if (we[2] == 1'b1) begin
      mem2[addr] <= din[BYTE_WIDTH*3-1:BYTE_WIDTH*2];
    end
    else bout2   <= mem2[addr];
    if (we[3] == 1'b1) begin
      mem3[addr] <= din[BYTE_WIDTH*4-1:BYTE_WIDTH*3];
    end
    else bout3   <= mem3[addr];
  end

  assign dout = {bout3, bout2, bout1, bout0};

endmodule // RV32I_ARCH1_MEMORY

