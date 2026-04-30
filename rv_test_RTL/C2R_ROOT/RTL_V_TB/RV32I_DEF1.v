
//// INST_type
`define COMP  4'd9
`define COMPI 4'd8
`define LUI   4'd7
`define AUIPC 4'd6
`define LD    4'd5
`define ST    4'd4
`define JAL   4'd3
`define JALR  4'd2
`define BR    4'd1
`define SYS   4'd0

/// OP_type
`define ADD   4'd13
`define SUB   4'd12
`define LT    4'd11
`define LTU   4'd10
`define AND   4'd9
`define OR    4'd8
`define XOR   4'd7
`define SLL   4'd6
`define SRL   4'd5
`define SRA   4'd4
`define EQ    4'd3
`define NE    4'd2
`define GE    4'd1
`define GEU   4'd0

/// LT_type
`define LB    3'd4
`define LBU   3'd3
`define LH    3'd2
`define LHU   3'd1
`define LW    3'd0

/// ST_type
`define SB    2'd2
`define SH    2'd1
`define SW    2'd0

/// SYS_type
`define CSRRW    3'd7
`define CSRRS    3'd6
`define CSRRC    3'd5
`define CSRRWI   3'd4
`define CSRRSI   3'd3
`define CSRRCI   3'd2
`define MRET     3'd1
`define WFI      3'd0

/// MD_type
`define MUL      3'd7
`define MULH     3'd6
`define MULHSU   3'd5
`define MULHU    3'd4
`define DIV      3'd3
`define DIVU     3'd2
`define REM      3'd1
`define REMU     3'd0


