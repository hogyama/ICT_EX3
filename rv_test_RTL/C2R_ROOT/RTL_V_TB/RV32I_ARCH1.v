`include "RV32I_DEF1.v"

module RV32I_ARCH1_step   (
    /* global inputs */
    clk, rst_n,

    /* inputs */
    G_axi_raddr_s_ready, G_axi_waddr_s_ready, G_axi_rdat_s_data, G_axi_rdat_s_resp, 
    G_axi_rdat_s_valid, G_axi_wdat_s_ready, G_axi_wres_s_resp, G_axi_wres_s_valid, 
    G_intr_p_0, G_intr_p_1, G_intr_p_2, G_intr_p_3, G_intr_p_4, G_intr_p_5, G_intr_p_6, 
    G_dbg_idx, 

    /* outputs */
    G_axi_raddr_m_addr, G_axi_raddr_m_size, G_axi_raddr_m_valid, G_axi_raddr_m_len, 
    G_axi_raddr_m_prot, G_axi_waddr_m_addr, G_axi_waddr_m_size, G_axi_waddr_m_valid, 
    G_axi_waddr_m_len, G_axi_waddr_m_prot, G_axi_rdat_m_ready, G_axi_wdat_m_data, 
    G_axi_wdat_m_strobe, G_axi_wdat_m_valid, G_axi_wdat_m_last, G_axi_wres_m_ready, 
    G_dbg_data, G_dbg_rv_state, G_dbg_wfi, G_RV32I_ARCH1_step_OUT

  );
  parameter M_ID = 0; // module ID

  
  /*input ports*/
  input        clk;
  input        rst_n;
  input        G_axi_raddr_s_ready; /// <0,1> [U1]          ;
  input        G_axi_waddr_s_ready; /// <0,1> [U1]          ;
  input [31:0] G_axi_rdat_s_data  ; /// <0,4294967295> [U32];
  input [ 1:0] G_axi_rdat_s_resp  ; /// <0,3> [U2]          ;
  input        G_axi_rdat_s_valid ; /// <0,1> [U1]          ;
  input        G_axi_wdat_s_ready ; /// <0,1> [U1]          ;
  input [ 1:0] G_axi_wres_s_resp  ; /// <0,3> [U2]          ;
  input        G_axi_wres_s_valid ; /// <0,1> [U1]          ;
  input        G_intr_p_0         ; /// <0,1> [U1]          ;
  input        G_intr_p_1         ; /// <0,1> [U1]          ;
  input        G_intr_p_2         ; /// <0,1> [U1]          ;
  input        G_intr_p_3         ; /// <0,1> [U1]          ;
  input        G_intr_p_4         ; /// <0,1> [U1]          ;
  input        G_intr_p_5         ; /// <0,1> [U1]          ;
  input        G_intr_p_6         ; /// <0,1> [U1]          ;
  input [15:0] G_dbg_idx          ; /// <0,65535> [U16]     ;

  /*output ports*/
  output[31:0] G_axi_raddr_m_addr    ; /// <0,4294967295> [U32];
  output[ 1:0] G_axi_raddr_m_size    ; /// <0,3> [U2]          ;
  output       G_axi_raddr_m_valid   ; /// <0,1> [U1]          ;
  output       G_axi_raddr_m_len     ; /// <0,0> [U0]          ;
  output[ 2:0] G_axi_raddr_m_prot    ; /// <0,7> [U3]          ;
  output[31:0] G_axi_waddr_m_addr    ; /// <0,4294967295> [U32];
  output[ 1:0] G_axi_waddr_m_size    ; /// <0,3> [U2]          ;
  output       G_axi_waddr_m_valid   ; /// <0,1> [U1]          ;
  output       G_axi_waddr_m_len     ; /// <0,0> [U0]          ;
  output[ 2:0] G_axi_waddr_m_prot    ; /// <0,7> [U3]          ;
  output       G_axi_rdat_m_ready    ; /// <0,1> [U1]          ;
  output[31:0] G_axi_wdat_m_data     ; /// <0,4294967295> [U32];
  output[ 3:0] G_axi_wdat_m_strobe   ; /// <0,15> [U4]         ;
  output       G_axi_wdat_m_valid    ; /// <0,1> [U1]          ;
  output       G_axi_wdat_m_last     ; /// <0,1> [U1]          ;
  output       G_axi_wres_m_ready    ; /// <0,1> [U1]          ;
  output[31:0] G_dbg_data            ; /// <0,4294967295> [U32];
  output[ 3:0] G_dbg_rv_state        ; /// <0,15> [U4]         ;
  output       G_dbg_wfi             ; /// <0,1> [U1]          ;
  output       G_RV32I_ARCH1_step_OUT; /// <0,1> [U1]          ;

  wire [31:0] io_addr, io_wdata, sys_rdata, axi_rdata, sys_mtime_l, sys_mtime_h;
  wire [1:0] sys_op, axi_op, io_size;
  wire sys_active, sys_mtip, sys_meip, sys_rdata_valid, axi_active, axi_rdata_valid, io_pending;

  assign G_dbg_data = 32'b0;
  assign G_dbg_rv_state = 4'b0;
  assign G_dbg_wfi = 1'b0;

  wire io_active = sys_active | axi_active;
  
  reg [31:0] io_rdata;
  always @ (posedge clk or negedge rst_n) begin
    if (rst_n == 1'b0)
        io_rdata <= 32'b0;
    else begin
        if (sys_rdata_valid)
            io_rdata <= sys_rdata;
        else if (axi_rdata_valid)
            io_rdata <= axi_rdata;
    end
  end

  wire [6:0] intr = { G_intr_p_6, G_intr_p_5, G_intr_p_4, G_intr_p_3, G_intr_p_2, G_intr_p_1, G_intr_p_0 };

  RV32I_ARCH1_CORE M0 (
    /* global inputs */
    clk, rst_n,
    /* inputs */
    io_active, io_pending, io_rdata, sys_mtip, sys_meip, sys_mtime_l, sys_mtime_h, 
    /* outputs */
    io_addr, io_wdata, sys_op, axi_op, io_size, 
    G_RV32I_ARCH1_step_OUT
  );

  SYSCTRL M1 (clk, rst_n, sys_op, io_addr[4:0], io_wdata, intr, 7'b0,//internal_irq_clear_mask, 
            sys_active, sys_rdata_valid, sys_rdata, sys_mtip, sys_meip, sys_mtime_l, sys_mtime_h);

  AXI4_MASTER M2 (clk, rst_n, axi_op, io_size, io_addr, io_wdata, axi_rdata_valid, axi_rdata, axi_active, io_pending,
    /// axi inputs
    G_axi_raddr_s_ready, G_axi_waddr_s_ready, G_axi_rdat_s_data, G_axi_rdat_s_resp, 
    G_axi_rdat_s_valid, G_axi_wdat_s_ready, G_axi_wres_s_resp, G_axi_wres_s_valid, 
    /// axi outputs
    G_axi_raddr_m_addr, G_axi_raddr_m_size, G_axi_raddr_m_valid, G_axi_raddr_m_len, 
    G_axi_raddr_m_prot, G_axi_waddr_m_addr, G_axi_waddr_m_size, G_axi_waddr_m_valid, 
    G_axi_waddr_m_len, G_axi_waddr_m_prot, G_axi_rdat_m_ready, G_axi_wdat_m_data, 
    G_axi_wdat_m_strobe, G_axi_wdat_m_valid, G_axi_wdat_m_last, G_axi_wres_m_ready
    );

`define DBG_DISPLAY

`ifdef DBG_DISPLAY
    reg wfi_occurred;
    always @ (posedge clk or negedge rst_n)
        if (!rst_n)
            wfi_occurred <= 0;
        else
            wfi_occurred <= wfi_occurred | G_RV32I_ARCH1_step_OUT;

    always @ (negedge clk)
    if (rst_n & wfi_occurred) begin
        $display("%d:%d T:%b PC:%h IR:%h bus:%h:%h:%h:%h I:%h OP:%h sys_meip:%b", 
        M0.M4.minstret[23:0], M0.M4.mcycle[23:0], M0.M1.T, M0.PC, M0.IR, M0.bus0, M0.bus1, M0.bus2, M0.bus3,
        M0.I, M0.OP_type, sys_meip);
        if ((~M0.T[0] & M0.PC_LD) | M0.TRAP_occurred)
            $display("--------------------------------");
    end
`endif
endmodule   /// RV32I_ARCH1_step

module SYSCTRL (clk, rst_n, io_op, addr, wdata, intr, internal_irq_clear_mask, active, rdata_valid, rdata, 
                mtip, meip, mtime_l, mtime_h);
input clk, rst_n;
input [1:0] io_op;
input [4:0] addr;
input [31:0] wdata;
input [6:0] intr, internal_irq_clear_mask;
output active, rdata_valid;
output [31:0] rdata, mtime_l, mtime_h;
output mtip, meip;

    reg [6:0] d_intr[0:2];
    reg [31:0] ext_irq_pending, ext_irq_enable, mtime_l, mtime_h, mtimecmp_l, mtimecmp_h;
    reg [3:0] divider;

    always @ (posedge clk) begin
        d_intr[0] <= d_intr[1];
        d_intr[1] <= d_intr[2];
        d_intr[2] <= intr;
    end

    wire [6 :0] intr_pos_edge = (~d_intr[0]) & d_intr[1];
    wire [31:0] n_ext_irq_pending = ext_irq_pending | intr_pos_edge;
    wire [31:0] ext_irq_detected = n_ext_irq_pending & ext_irq_enable;
    wire readFlag = (io_op == 2'b01);
    wire writeFlag = (io_op == 2'b10);
    wire active = (|io_op);
    wire [31:0] ext_irq_clear_mask = (~readFlag & (addr == 5'h00)) ? wdata : { 25'b0, internal_irq_clear_mask };

    assign mtip = { mtime_h, mtime_l } >= { mtimecmp_h, mtimecmp_l };
    assign meip = (|ext_irq_detected);

//`define DBG_SYSCTRL
`ifdef DBG_SYSCTRL
    always @ (posedge clk) begin
        if (meip)
            $display("SYSCTRL : ext_irq_detected:%h n_ext_irq_pending:%h ext_irq_enable:%h",
                ext_irq_detected, n_ext_irq_pending, ext_irq_enable);
    end
`endif

//function READ_DATA;  
function [31:0] READ_DATA; // modified by LI in 2025
    input [4:0] addr;
    case (addr)
    5'h00:   READ_DATA = ext_irq_detected;
    5'h08:   READ_DATA = ext_irq_enable;
    5'h10:   READ_DATA = mtime_l;
    5'h14:   READ_DATA = mtime_h;
    5'h18:   READ_DATA = mtimecmp_l;
    5'h1c:   READ_DATA = mtimecmp_h;
    default: READ_DATA = 32'b0;
    endcase
endfunction

    assign rdata = (readFlag) ? READ_DATA(addr) : 32'b0;
    assign rdata_valid = readFlag;

    always @ (posedge clk or negedge rst_n)
        if (rst_n == 1'b0) begin
            d_intr[0] <= 0;
            d_intr[1] <= 0;
            d_intr[2] <= 0;
            ext_irq_pending <= 0;
            ext_irq_enable <= 0;
            mtime_l <= 0;
            mtime_h <= 0;
            mtimecmp_l <= 0;
            mtimecmp_h <= 0;
            divider <= 0;
        end
        else begin
            if (writeFlag)
                case (addr)
                //    5'h00: ext_irq_pending <= n_ext_irq_pending & ~wdata;
                    5'h08: begin
                        ext_irq_enable <= wdata;
                        $display("ext_irq_enable <- %h", wdata);
                    end
                //    5'h10: m_time_l : read-only
                //    5'h14: m_time_h : read-only
                    5'h18: mtimecmp_l <= wdata;
                    5'h1c: mtimecmp_h <= wdata;
                endcase
            if (divider == 4'b0)
                { mtime_h, mtime_l } <= { mtime_h, mtime_l } + 64'b1;
            divider <= divider + 1;
            ext_irq_pending <= n_ext_irq_pending & ~ext_irq_clear_mask;
        end

endmodule   ///   SYSCTRL

module AXI4_MASTER (clk, rst_n, io_op, io_size, addr, wdata, rdata_valid, rdata, active, stalled,
    /// axi inputs
    G_axi_raddr_s_ready, G_axi_waddr_s_ready, G_axi_rdat_s_data, G_axi_rdat_s_resp, 
    G_axi_rdat_s_valid, G_axi_wdat_s_ready, G_axi_wres_s_resp, G_axi_wres_s_valid, 
    /// axi outputs
    G_axi_raddr_m_addr, G_axi_raddr_m_size, G_axi_raddr_m_valid, G_axi_raddr_m_len, 
    G_axi_raddr_m_prot, G_axi_waddr_m_addr, G_axi_waddr_m_size, G_axi_waddr_m_valid, 
    G_axi_waddr_m_len, G_axi_waddr_m_prot, G_axi_rdat_m_ready, G_axi_wdat_m_data, 
    G_axi_wdat_m_strobe, G_axi_wdat_m_valid, G_axi_wdat_m_last, G_axi_wres_m_ready
);
input clk, rst_n;
input [1:0] io_op, io_size;
input [31:0] addr;
input [31:0] wdata;
output rdata_valid, active, stalled;
output [31:0] rdata;

input        G_axi_raddr_s_ready; /// <0,1> [U1]          ;
input        G_axi_waddr_s_ready; /// <0,1> [U1]          ;
input [31:0] G_axi_rdat_s_data  ; /// <0,4294967295> [U32];
input [ 1:0] G_axi_rdat_s_resp  ; /// <0,3> [U2]          ;
input        G_axi_rdat_s_valid ; /// <0,1> [U1]          ;
input        G_axi_wdat_s_ready ; /// <0,1> [U1]          ;
input [ 1:0] G_axi_wres_s_resp  ; /// <0,3> [U2]          ;
input        G_axi_wres_s_valid ; /// <0,1> [U1]          ;

output[31:0] G_axi_raddr_m_addr    ; /// <0,4294967295> [U32];
output[ 1:0] G_axi_raddr_m_size    ; /// <0,3> [U2]          ;
output       G_axi_raddr_m_valid   ; /// <0,1> [U1]          ;
output       G_axi_raddr_m_len     ; /// <0,0> [U0]          ;
output[ 2:0] G_axi_raddr_m_prot    ; /// <0,7> [U3]          ;
output[31:0] G_axi_waddr_m_addr    ; /// <0,4294967295> [U32];
output[ 1:0] G_axi_waddr_m_size    ; /// <0,3> [U2]          ;
output       G_axi_waddr_m_valid   ; /// <0,1> [U1]          ;
output       G_axi_waddr_m_len     ; /// <0,0> [U0]          ;
output[ 2:0] G_axi_waddr_m_prot    ; /// <0,7> [U3]          ;
output       G_axi_rdat_m_ready    ; /// <0,1> [U1]          ;
output[31:0] G_axi_wdat_m_data     ; /// <0,4294967295> [U32];
output[ 3:0] G_axi_wdat_m_strobe   ; /// <0,15> [U4]         ;
output       G_axi_wdat_m_valid    ; /// <0,1> [U1]          ;
output       G_axi_wdat_m_last     ; /// <0,1> [U1]          ;
output       G_axi_wres_m_ready    ; /// <0,1> [U1]          ;

    assign rdata = G_axi_rdat_s_data;
    assign rdata_valid = G_axi_rdat_s_valid;

    reg [ 1:0] pending_op;
    reg [ 2:0] rw_state;
    reg [ 3:0] burst_count;

    reg        in_raddr_s_ready; /// <0,1> [U1]          ;
    reg        in_waddr_s_ready; /// <0,1> [U1]          ;
    reg [31:0] in_rdat_s_data  ; /// <0,4294967295> [U32];
    reg [ 1:0] in_rdat_s_resp  ; /// <0,3> [U2]          ;
    reg        in_rdat_s_valid ; /// <0,1> [U1]          ;
    reg        in_wdat_s_ready ; /// <0,1> [U1]          ;
    reg [ 1:0] in_wres_s_resp  ; /// <0,3> [U2]          ;
    reg        in_wres_s_valid ; /// <0,1> [U1]          ;

    reg[31:0] G_axi_raddr_m_addr    ; /// <0,4294967295> [U32];
    reg[ 1:0] G_axi_raddr_m_size    ; /// <0,3> [U2]          ;
    reg       G_axi_raddr_m_valid   ; /// <0,1> [U1]          ;
    reg       G_axi_raddr_m_len     ; /// <0,0> [U0]          ;
    reg[ 2:0] G_axi_raddr_m_prot    ; /// <0,7> [U3]          ;
    reg[31:0] G_axi_waddr_m_addr    ; /// <0,4294967295> [U32];
    reg[ 1:0] G_axi_waddr_m_size    ; /// <0,3> [U2]          ;
    reg       G_axi_waddr_m_valid   ; /// <0,1> [U1]          ;
    reg       G_axi_waddr_m_len     ; /// <0,0> [U0]          ;
    reg[ 2:0] G_axi_waddr_m_prot    ; /// <0,7> [U3]          ;
    reg       G_axi_rdat_m_ready    ; /// <0,1> [U1]          ;
    reg[31:0] G_axi_wdat_m_data     ; /// <0,4294967295> [U32];
    reg[ 3:0] G_axi_wdat_m_strobe   ; /// <0,15> [U4]         ;
    reg       G_axi_wdat_m_valid    ; /// <0,1> [U1]          ;
    reg       G_axi_wdat_m_last     ; /// <0,1> [U1]          ;
    reg       G_axi_wres_m_ready    ; /// <0,1> [U1]          ;

task MA_RESET_RADDR;
begin
    G_axi_raddr_m_addr  <= 32'b0;
    G_axi_raddr_m_size  <= 2'b0;
    G_axi_raddr_m_valid <= 1'b0;
    G_axi_raddr_m_len   <= 1'b0;
    G_axi_raddr_m_prot  <= 3'b000;
end
endtask

task MA_RESET_WADDR;
begin
    G_axi_waddr_m_addr  <= 32'b0;
    G_axi_waddr_m_size  <= 2'b0;
    G_axi_waddr_m_valid <= 1'b0;
    G_axi_waddr_m_len   <= 1'b0;
    G_axi_waddr_m_prot  <= 3'b000;
end
endtask

task MA_RESET_WDAT;
begin
    G_axi_wdat_m_data   <= 32'b0;
    G_axi_wdat_m_strobe <= 2'b0;
    G_axi_wdat_m_valid  <= 1'b0;
    G_axi_wdat_m_last   <= 1'b0;
end
endtask

task MA_SET_READ;
begin
    G_axi_raddr_m_addr  <= addr;
    G_axi_raddr_m_size  <= io_size;
    G_axi_raddr_m_valid <= 1'b1;
    G_axi_raddr_m_len   <= 1'b0;
    G_axi_raddr_m_prot  <= 3'b000;
    G_axi_rdat_m_ready  <= 1'b1;
end
endtask

task MA_SET_WRITE;
begin
    G_axi_waddr_m_addr  <= addr;
    G_axi_waddr_m_size  <= io_size;
    G_axi_waddr_m_valid <= 1'b1;
    G_axi_waddr_m_len   <= 1'b0;
    G_axi_waddr_m_prot  <= 3'b000;
    G_axi_wdat_m_data   <= wdata;
    G_axi_wdat_m_strobe <= 4'b1111;
    G_axi_wdat_m_valid  <= 1'b1;
    G_axi_wdat_m_last   <= 1'b1;
    G_axi_wres_m_ready  <= 1'b1;
end
endtask

task SL_RESET_READ;
begin
    in_raddr_s_ready <= 1'b0;
    in_rdat_s_data   <= 32'b0;
    in_rdat_s_resp   <= 1'b0;
    in_rdat_s_valid  <= 1'b0;
end
endtask

task SL_RESET_WRITE;
begin
    in_waddr_s_ready   <= 1'b0;
    in_wdat_s_ready    <= 32'b0;
    in_wres_s_resp     <= 1'b0;
    in_wres_s_valid    <= 1'b0;
end
endtask

    wire [1:0] io_op2 = io_op | pending_op;
    wire RW_RAddr_complete = (G_axi_rdat_s_valid | in_rdat_s_valid) & (G_axi_raddr_s_ready | in_raddr_s_ready);
    wire RW_WAddr_complete = (G_axi_wres_s_valid | in_wres_s_valid) & (G_axi_wdat_s_ready | in_wdat_s_ready) & 
                            (G_axi_waddr_s_ready | in_waddr_s_ready);

    wire bad_resp = (rw_state == 2'b10) & (G_axi_wres_s_resp != 2'b00);

    assign active = (io_op2 != 2'b00);    /// AXI_Idle

    assign stalled = ((rw_state == 3'b000) & (io_op2 != 2'b00)) |              /// rw_state == RW_Init && io_op2 != AXI_Idle
                    ((rw_state == 3'b001) & ~RW_RAddr_complete) |             /// rw_state == RW_RAddr && !RW_RAddr_complete
                    ((rw_state == 3'b010) & (~RW_WAddr_complete | bad_resp)); /// rw_state == RW_WAddr && (!RW_WAddr_complete || bad_resp)

    always @ (posedge clk or negedge rst_n)
        ////////////// rst_n == 0 //////////////
        if (rst_n == 1'b0) begin
            pending_op  <= 2'b0;
            rw_state    <= 3'b0;
            burst_count <= 4'b0;
//            rdata       <= 32'b0;
            MA_RESET_RADDR();
            MA_RESET_WADDR();
            MA_RESET_WDAT();
            G_axi_rdat_m_ready <= 0;
            G_axi_wres_m_ready <= 0;
            SL_RESET_READ();
            SL_RESET_WRITE();
        end
        else begin
            ////////////// rw_state == RW_Init //////////////
            if (rw_state == 3'b000) begin   /// RW_Init:
                if (io_op2 == 2'b01) begin  /// AXI_Read:
                    MA_SET_READ();
                    burst_count <= 4'b0;
                    SL_RESET_READ();
                    rw_state <= 3'b001;   /// --> RW_RAddr:
                    //$display("RW_Init --> RW_RAddr");
                end
                else if (io_op2 == 2'b10) begin /// AXI_Write
                    MA_SET_WRITE();
                    burst_count <= 4'b0;
                    SL_RESET_WRITE();
                    rw_state <= 3'b010;   /// --> RW_WAddr:
                    //$display("RW_Init --> RW_WAddr");
                end
            end
            ////////////// rw_state == RW_RAddr //////////////
            else if (rw_state == 3'b001) begin /// RW_RAddr:
                //$display("RW_RAddr : ");
                if (G_axi_raddr_s_ready) begin
                    //$display("G_axi_raddr_s_ready");
                    in_raddr_s_ready <= 1'b1;
                    MA_RESET_RADDR();
                end
                if (G_axi_rdat_s_valid) begin
                    //$display("data_latched : rdata <- %h", G_axi_rdat_s_data);
                    in_rdat_s_data  <= G_axi_rdat_s_data;
                    in_rdat_s_resp  <= G_axi_rdat_s_resp;
                    in_rdat_s_valid <= 1'b1;
                    G_axi_rdat_m_ready <= 1'b0; /// MA_RESET_RDAT
//                    rdata <= G_axi_rdat_s_data;
                    // data_latched <-- 1;
                end
                if (RW_RAddr_complete) begin
                    rw_state <= 3'b000;  /// --> RW_Init
                    /// else stalled <-- 1;
                end
            end
            ////////////// rw_state == RW_WAddr //////////////
            else if (rw_state == 3'b010) begin /// RW_WAddr:
                if (G_axi_waddr_s_ready) begin
                    in_waddr_s_ready <= 1'b1;
                    MA_RESET_WADDR();
                end
                if (G_axi_wdat_s_ready) begin
                    in_wdat_s_ready <= 1'b1;
                    MA_RESET_WDAT();
                end
                if (G_axi_wres_s_valid) begin
                    in_wres_s_resp  <= G_axi_wres_s_resp;
                    in_wres_s_valid <= 1'b1;
                    G_axi_wres_m_ready <= 1'b0;;
                end
                if (RW_WAddr_complete)
                    rw_state <= 3'b000;  /// --> RW_Init
			        ///if (bad_resp) { stalled = 1; }  /// try again
                /// else stalled <-- 1;
            end
            pending_op <= (stalled) ? io_op2 : 2'b0;
        end

endmodule   /// AXI4_MASTER

