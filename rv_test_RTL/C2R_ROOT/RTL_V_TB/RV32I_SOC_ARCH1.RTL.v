
module RV32I_SOC_ARCH1   (
    /* global inputs */
    clk, rst_n,
    /* inputs */
    G_io_uart_0_rx, G_io_uart_0_cts, G_io_uart_1_rx, G_io_uart_1_cts, G_io_gpio_sw_in, 
    G_io_intr_p_2, G_io_intr_p_3, G_io_intr_p_4, G_io_intr_p_5, G_io_intr_p_6, 
    /* outputs */
    G_io_uart_0_tx, G_io_uart_0_rts, G_io_uart_1_tx, G_io_uart_1_rts, G_io_gpio_seg7_dout, 
    G_io_gpio_seg7_an, G_io_gpio_led_out, G_RV32I_SOC_ARCH1_OUT
  );

  /*input ports*/
  input        clk;
  input        rst_n;
  input        G_io_uart_0_rx ; /// <0,1> [U1]     ;
  input        G_io_uart_0_cts; /// <0,1> [U1]     ;
  input        G_io_uart_1_rx ; /// <0,1> [U1]     ;
  input        G_io_uart_1_cts; /// <0,1> [U1]     ;
  input [15:0] G_io_gpio_sw_in; /// <0,65535> [U16];
  input        G_io_intr_p_2  ; /// <0,1> [U1]     ;
  input        G_io_intr_p_3  ; /// <0,1> [U1]     ;
  input        G_io_intr_p_4  ; /// <0,1> [U1]     ;
  input        G_io_intr_p_5  ; /// <0,1> [U1]     ;
  input        G_io_intr_p_6  ; /// <0,1> [U1]     ;

  /*output ports*/
  output       G_io_uart_0_tx       ; /// <0,1> [U1]     ;
  output       G_io_uart_0_rts      ; /// <0,1> [U1]     ;
  output       G_io_uart_1_tx       ; /// <0,1> [U1]     ;
  output       G_io_uart_1_rts      ; /// <0,1> [U1]     ;
  output[ 7:0] G_io_gpio_seg7_dout  ; /// <0,255> [U8]   ;
  output[ 7:0] G_io_gpio_seg7_an    ; /// <0,255> [U8]   ;
  output[14:0] G_io_gpio_led_out    ; /// <0,32767> [U15];
  output       G_RV32I_SOC_ARCH1_OUT; /// <0,1> [U1]     ;

  /*internal wires*/
  wire         G_axi_s_ch_0_raddr_s_ready; /// <0,1> [U1]          ;
  wire         G_axi_s_ch_0_waddr_s_ready; /// <0,1> [U1]          ;
  wire[31:0]   G_axi_s_ch_0_rdat_s_data  ; /// <0,4294967295> [U32];
  wire         G_axi_s_ch_0_rdat_s_resp  ; /// <0,0> [U0]          ;
  wire         G_axi_s_ch_0_rdat_s_valid ; /// <0,1> [U1]          ;
  wire         G_axi_s_ch_0_rdat_s_last  ; /// <0,1> [U1]          ;
  wire         G_axi_s_ch_0_wdat_s_ready ; /// <0,1> [U1]          ;
  wire         G_axi_s_ch_0_wres_s_resp  ; /// <0,0> [U0]          ;
  wire         G_axi_s_ch_0_wres_s_valid ; /// <0,1> [U1]          ;
  wire         G_io_intr_p_0             ; /// <0,1> [U1]          ;
  wire         G_axi_s_ch_1_raddr_s_ready; /// <0,1> [U1]          ;
  wire         G_axi_s_ch_1_waddr_s_ready; /// <0,1> [U1]          ;
  wire[31:0]   G_axi_s_ch_1_rdat_s_data  ; /// <0,4294967295> [U32];
  wire         G_axi_s_ch_1_rdat_s_resp  ; /// <0,0> [U0]          ;
  wire         G_axi_s_ch_1_rdat_s_valid ; /// <0,1> [U1]          ;
  wire         G_axi_s_ch_1_rdat_s_last  ; /// <0,1> [U1]          ;
  wire         G_axi_s_ch_1_wdat_s_ready ; /// <0,1> [U1]          ;
  wire         G_axi_s_ch_1_wres_s_resp  ; /// <0,0> [U0]          ;
  wire         G_axi_s_ch_1_wres_s_valid ; /// <0,1> [U1]          ;
  wire         G_io_intr_p_1             ; /// <0,1> [U1]          ;
  wire         G_axi_s_ch_2_raddr_s_ready; /// <0,1> [U1]          ;
  wire         G_axi_s_ch_2_waddr_s_ready; /// <0,1> [U1]          ;
  wire[31:0]   G_axi_s_ch_2_rdat_s_data  ; /// <0,4294967295> [U32];
  wire         G_axi_s_ch_2_rdat_s_resp  ; /// <0,0> [U0]          ;
  wire         G_axi_s_ch_2_rdat_s_valid ; /// <0,1> [U1]          ;
  wire         G_axi_s_ch_2_rdat_s_last  ; /// <0,1> [U1]          ;
  wire         G_axi_s_ch_2_wdat_s_ready ; /// <0,1> [U1]          ;
  wire         G_axi_s_ch_2_wres_s_resp  ; /// <0,0> [U0]          ;
  wire         G_axi_s_ch_2_wres_s_valid ; /// <0,1> [U1]          ;
  wire[15:0]   G_io_dbg_idx              ; /// <0,65535> [U16]     ;
  wire[31:0]   G_axi_m_ch_0_raddr_m_addr ; /// <0,4294967295> [U32];
  wire[ 1:0]   G_axi_m_ch_0_raddr_m_size ; /// <0,3> [U2]          ;
  wire         G_axi_m_ch_0_raddr_m_valid; /// <0,1> [U1]          ;
  wire         G_axi_m_ch_0_raddr_m_len  ; /// <0,0> [U0]          ;
  wire[ 2:0]   G_axi_m_ch_0_raddr_m_prot ; /// <0,7> [U3]          ;
  wire[31:0]   G_axi_m_ch_0_waddr_m_addr ; /// <0,4294967295> [U32];
  wire[ 1:0]   G_axi_m_ch_0_waddr_m_size ; /// <0,3> [U2]          ;
  wire         G_axi_m_ch_0_waddr_m_valid; /// <0,1> [U1]          ;
  wire         G_axi_m_ch_0_waddr_m_len  ; /// <0,0> [U0]          ;
  wire[ 2:0]   G_axi_m_ch_0_waddr_m_prot ; /// <0,7> [U3]          ;
  wire         G_axi_m_ch_0_rdat_m_ready ; /// <0,1> [U1]          ;
  wire[31:0]   G_axi_m_ch_0_wdat_m_data  ; /// <0,4294967295> [U32];
  wire[ 3:0]   G_axi_m_ch_0_wdat_m_strobe; /// <0,15> [U4]         ;
  wire         G_axi_m_ch_0_wdat_m_valid ; /// <0,1> [U1]          ;
  wire         G_axi_m_ch_0_wdat_m_last  ; /// <0,1> [U1]          ;
  wire         G_axi_m_ch_0_wres_m_ready ; /// <0,1> [U1]          ;
  wire[31:0]   G_io_dbg_data             ; /// <0,4294967295> [U32];
  wire[ 3:0]   G_io_dbg_rv_state         ; /// <0,15> [U4]         ;
  wire         G_io_dbg_wfi              ; /// <0,1> [U1]          ;
  wire         G_axi_m_ch_0_raddr_s_ready; /// <0,1> [U1]          ;
  wire         G_axi_m_ch_0_waddr_s_ready; /// <0,1> [U1]          ;
  wire[31:0]   G_axi_m_ch_0_rdat_s_data  ; /// <0,4294967295> [U32];
  wire[ 1:0]   G_axi_m_ch_0_rdat_s_resp  ; /// <0,3> [U2]          ;
  wire         G_axi_m_ch_0_rdat_s_valid ; /// <0,1> [U1]          ;
  wire         G_axi_m_ch_0_rdat_s_last  ; /// <0,1> [U1]          ;
  wire         G_axi_m_ch_0_wdat_s_ready ; /// <0,1> [U1]          ;
  wire[ 1:0]   G_axi_m_ch_0_wres_s_resp  ; /// <0,3> [U2]          ;
  wire         G_axi_m_ch_0_wres_s_valid ; /// <0,1> [U1]          ;
  wire[31:0]   G_axi_s_ch_0_raddr_m_addr ; /// <0,4294967295> [U32];
  wire[ 2:0]   G_axi_s_ch_0_raddr_m_size ; /// <0,7> [U3]          ;
  wire         G_axi_s_ch_0_raddr_m_valid; /// <0,1> [U1]          ;
  wire[ 3:0]   G_axi_s_ch_0_raddr_m_len  ; /// <0,15> [U4]         ;
  wire[ 2:0]   G_axi_s_ch_0_raddr_m_prot ; /// <0,7> [U3]          ;
  wire[31:0]   G_axi_s_ch_0_waddr_m_addr ; /// <0,4294967295> [U32];
  wire[ 2:0]   G_axi_s_ch_0_waddr_m_size ; /// <0,7> [U3]          ;
  wire         G_axi_s_ch_0_waddr_m_valid; /// <0,1> [U1]          ;
  wire[ 3:0]   G_axi_s_ch_0_waddr_m_len  ; /// <0,15> [U4]         ;
  wire[ 2:0]   G_axi_s_ch_0_waddr_m_prot ; /// <0,7> [U3]          ;
  wire         G_axi_s_ch_0_rdat_m_ready ; /// <0,1> [U1]          ;
  wire[31:0]   G_axi_s_ch_0_wdat_m_data  ; /// <0,4294967295> [U32];
  wire[ 3:0]   G_axi_s_ch_0_wdat_m_strobe; /// <0,15> [U4]         ;
  wire         G_axi_s_ch_0_wdat_m_valid ; /// <0,1> [U1]          ;
  wire         G_axi_s_ch_0_wdat_m_last  ; /// <0,1> [U1]          ;
  wire         G_axi_s_ch_0_wres_m_ready ; /// <0,1> [U1]          ;
  wire[31:0]   G_axi_s_ch_1_raddr_m_addr ; /// <0,4294967295> [U32];
  wire[ 2:0]   G_axi_s_ch_1_raddr_m_size ; /// <0,7> [U3]          ;
  wire         G_axi_s_ch_1_raddr_m_valid; /// <0,1> [U1]          ;
  wire[ 3:0]   G_axi_s_ch_1_raddr_m_len  ; /// <0,15> [U4]         ;
  wire[ 2:0]   G_axi_s_ch_1_raddr_m_prot ; /// <0,7> [U3]          ;
  wire[31:0]   G_axi_s_ch_1_waddr_m_addr ; /// <0,4294967295> [U32];
  wire[ 2:0]   G_axi_s_ch_1_waddr_m_size ; /// <0,7> [U3]          ;
  wire         G_axi_s_ch_1_waddr_m_valid; /// <0,1> [U1]          ;
  wire[ 3:0]   G_axi_s_ch_1_waddr_m_len  ; /// <0,15> [U4]         ;
  wire[ 2:0]   G_axi_s_ch_1_waddr_m_prot ; /// <0,7> [U3]          ;
  wire         G_axi_s_ch_1_rdat_m_ready ; /// <0,1> [U1]          ;
  wire[31:0]   G_axi_s_ch_1_wdat_m_data  ; /// <0,4294967295> [U32];
  wire[ 3:0]   G_axi_s_ch_1_wdat_m_strobe; /// <0,15> [U4]         ;
  wire         G_axi_s_ch_1_wdat_m_valid ; /// <0,1> [U1]          ;
  wire         G_axi_s_ch_1_wdat_m_last  ; /// <0,1> [U1]          ;
  wire         G_axi_s_ch_1_wres_m_ready ; /// <0,1> [U1]          ;
  wire[31:0]   G_axi_s_ch_2_raddr_m_addr ; /// <0,4294967295> [U32];
  wire[ 2:0]   G_axi_s_ch_2_raddr_m_size ; /// <0,7> [U3]          ;
  wire         G_axi_s_ch_2_raddr_m_valid; /// <0,1> [U1]          ;
  wire[ 3:0]   G_axi_s_ch_2_raddr_m_len  ; /// <0,15> [U4]         ;
  wire[ 2:0]   G_axi_s_ch_2_raddr_m_prot ; /// <0,7> [U3]          ;
  wire[31:0]   G_axi_s_ch_2_waddr_m_addr ; /// <0,4294967295> [U32];
  wire[ 2:0]   G_axi_s_ch_2_waddr_m_size ; /// <0,7> [U3]          ;
  wire         G_axi_s_ch_2_waddr_m_valid; /// <0,1> [U1]          ;
  wire[ 3:0]   G_axi_s_ch_2_waddr_m_len  ; /// <0,15> [U4]         ;
  wire[ 2:0]   G_axi_s_ch_2_waddr_m_prot ; /// <0,7> [U3]          ;
  wire         G_axi_s_ch_2_rdat_m_ready ; /// <0,1> [U1]          ;
  wire[31:0]   G_axi_s_ch_2_wdat_m_data  ; /// <0,4294967295> [U32];
  wire[ 3:0]   G_axi_s_ch_2_wdat_m_strobe; /// <0,15> [U4]         ;
  wire         G_axi_s_ch_2_wdat_m_valid ; /// <0,1> [U1]          ;
  wire         G_axi_s_ch_2_wdat_m_last  ; /// <0,1> [U1]          ;
  wire         G_axi_s_ch_2_wres_m_ready ; /// <0,1> [U1]          ;

  UART_AXI4L_step M0 (
  /*inputs*/
    .clk(clk), .rst_n(rst_n),
    .G_axi_raddr_m_addr     (G_axi_s_ch_0_raddr_m_addr ), // tgt[CLK*] <-- src[CLK*]
    .G_axi_raddr_m_valid    (G_axi_s_ch_0_raddr_m_valid), // tgt[CLK*] <-- src[CLK*]
    .G_axi_raddr_m_len      (G_axi_s_ch_0_raddr_m_len  ), // tgt[CLK*] <-- src[CLK*]
    .G_axi_waddr_m_addr     (G_axi_s_ch_0_waddr_m_addr ), // tgt[CLK*] <-- src[CLK*]
    .G_axi_waddr_m_valid    (G_axi_s_ch_0_waddr_m_valid), // tgt[CLK*] <-- src[CLK*]
    .G_axi_waddr_m_len      (G_axi_s_ch_0_waddr_m_len  ), // tgt[CLK*] <-- src[CLK*]
    .G_axi_rdat_m_ready     (G_axi_s_ch_0_rdat_m_ready ), // tgt[CLK*] <-- src[CLK*]
    .G_axi_wdat_m_data      (G_axi_s_ch_0_wdat_m_data  ), // tgt[CLK*] <-- src[CLK*]
    .G_axi_wdat_m_valid     (G_axi_s_ch_0_wdat_m_valid ), // tgt[CLK*] <-- src[CLK*]
    .G_axi_wres_m_ready     (G_axi_s_ch_0_wres_m_ready ), // tgt[CLK*] <-- src[CLK*]
    .G_uart_pin_rx          (G_io_uart_0_rx            ), // tgt[CLK*] <-- src[CLK*]
    .G_uart_pin_cts         (G_io_uart_0_cts           ), // tgt[CLK*] <-- src[CLK*]
  /*outputs*/
    .G_axi_raddr_s_ready    (G_axi_s_ch_0_raddr_s_ready), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_waddr_s_ready    (G_axi_s_ch_0_waddr_s_ready), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_rdat_s_data      (G_axi_s_ch_0_rdat_s_data  ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_rdat_s_resp      (G_axi_s_ch_0_rdat_s_resp  ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_rdat_s_valid     (G_axi_s_ch_0_rdat_s_valid ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_rdat_s_last      (G_axi_s_ch_0_rdat_s_last  ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_wdat_s_ready     (G_axi_s_ch_0_wdat_s_ready ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_wres_s_resp      (G_axi_s_ch_0_wres_s_resp  ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_wres_s_valid     (G_axi_s_ch_0_wres_s_valid ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_uart_pin_tx          (G_io_uart_0_tx            ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_uart_pin_rts         (G_io_uart_0_rts           ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_intr_out             (G_io_intr_p_0             )  // src[CLK*] --> tgt[CLK*] // latched  
  );

/// unitNode(UART_AXI4L_step) : CFG[CLK*] IO[CLK*]
  UART_AXI4L_step #(1) M1 (
  /*inputs*/
    .clk(clk), .rst_n(rst_n),
    .G_axi_raddr_m_addr     (G_axi_s_ch_1_raddr_m_addr ), // tgt[CLK*] <-- src[CLK*]
    .G_axi_raddr_m_valid    (G_axi_s_ch_1_raddr_m_valid), // tgt[CLK*] <-- src[CLK*]
    .G_axi_raddr_m_len      (G_axi_s_ch_1_raddr_m_len  ), // tgt[CLK*] <-- src[CLK*]
    .G_axi_waddr_m_addr     (G_axi_s_ch_1_waddr_m_addr ), // tgt[CLK*] <-- src[CLK*]
    .G_axi_waddr_m_valid    (G_axi_s_ch_1_waddr_m_valid), // tgt[CLK*] <-- src[CLK*]
    .G_axi_waddr_m_len      (G_axi_s_ch_1_waddr_m_len  ), // tgt[CLK*] <-- src[CLK*]
    .G_axi_rdat_m_ready     (G_axi_s_ch_1_rdat_m_ready ), // tgt[CLK*] <-- src[CLK*]
    .G_axi_wdat_m_data      (G_axi_s_ch_1_wdat_m_data  ), // tgt[CLK*] <-- src[CLK*]
    .G_axi_wdat_m_valid     (G_axi_s_ch_1_wdat_m_valid ), // tgt[CLK*] <-- src[CLK*]
    .G_axi_wres_m_ready     (G_axi_s_ch_1_wres_m_ready ), // tgt[CLK*] <-- src[CLK*]
    .G_uart_pin_rx          (G_io_uart_1_rx            ), // tgt[CLK*] <-- src[CLK*]
    .G_uart_pin_cts         (G_io_uart_1_cts           ), // tgt[CLK*] <-- src[CLK*]
  /*outputs*/
    .G_axi_raddr_s_ready    (G_axi_s_ch_1_raddr_s_ready), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_waddr_s_ready    (G_axi_s_ch_1_waddr_s_ready), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_rdat_s_data      (G_axi_s_ch_1_rdat_s_data  ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_rdat_s_resp      (G_axi_s_ch_1_rdat_s_resp  ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_rdat_s_valid     (G_axi_s_ch_1_rdat_s_valid ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_rdat_s_last      (G_axi_s_ch_1_rdat_s_last  ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_wdat_s_ready     (G_axi_s_ch_1_wdat_s_ready ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_wres_s_resp      (G_axi_s_ch_1_wres_s_resp  ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_wres_s_valid     (G_axi_s_ch_1_wres_s_valid ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_uart_pin_tx          (G_io_uart_1_tx            ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_uart_pin_rts         (G_io_uart_1_rts           ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_intr_out             (G_io_intr_p_1             )  // src[CLK*] --> tgt[CLK*] // latched  
  );

/// unitNode(GPIO_AXI4L_step) : CFG[CLK*] IO[CLK*]
  GPIO_AXI4L_step M2 (
  /*inputs*/
    .clk(clk), .rst_n(rst_n),
    .G_axi_raddr_m_valid    (G_axi_s_ch_2_raddr_m_valid), // tgt[CLK*] <-- src[CLK*]
    .G_axi_raddr_m_len      (G_axi_s_ch_2_raddr_m_len  ), // tgt[CLK*] <-- src[CLK*]
    .G_axi_waddr_m_addr     (G_axi_s_ch_2_waddr_m_addr ), // tgt[CLK*] <-- src[CLK*]
    .G_axi_waddr_m_valid    (G_axi_s_ch_2_waddr_m_valid), // tgt[CLK*] <-- src[CLK*]
    .G_axi_waddr_m_len      (G_axi_s_ch_2_waddr_m_len  ), // tgt[CLK*] <-- src[CLK*]
    .G_axi_rdat_m_ready     (G_axi_s_ch_2_rdat_m_ready ), // tgt[CLK*] <-- src[CLK*]
    .G_axi_wdat_m_data      (G_axi_s_ch_2_wdat_m_data  ), // tgt[CLK*] <-- src[CLK*]
    .G_axi_wdat_m_valid     (G_axi_s_ch_2_wdat_m_valid ), // tgt[CLK*] <-- src[CLK*]
    .G_axi_wres_m_ready     (G_axi_s_ch_2_wres_m_ready ), // tgt[CLK*] <-- src[CLK*]
    .G_gpio_pin_sw_in       (G_io_gpio_sw_in           ), // tgt[CLK*] <-- src[CLK*]
    .G_dbg_data             (G_io_dbg_data             ), // tgt[CLK*] <-- src[CLK*]
    .G_dbg_rv_state         (G_io_dbg_rv_state         ), // tgt[CLK*] <-- src[CLK*]
    .G_dbg_wfi              (G_io_dbg_wfi              ), // tgt[CLK*] <-- src[CLK*]
  /*outputs*/
    .G_axi_raddr_s_ready    (G_axi_s_ch_2_raddr_s_ready), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_waddr_s_ready    (G_axi_s_ch_2_waddr_s_ready), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_rdat_s_data      (G_axi_s_ch_2_rdat_s_data  ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_rdat_s_resp      (G_axi_s_ch_2_rdat_s_resp  ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_rdat_s_valid     (G_axi_s_ch_2_rdat_s_valid ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_rdat_s_last      (G_axi_s_ch_2_rdat_s_last  ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_wdat_s_ready     (G_axi_s_ch_2_wdat_s_ready ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_wres_s_resp      (G_axi_s_ch_2_wres_s_resp  ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_wres_s_valid     (G_axi_s_ch_2_wres_s_valid ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_gpio_pin_seg7_dout   (G_io_gpio_seg7_dout       ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_gpio_pin_seg7_an     (G_io_gpio_seg7_an         ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_gpio_pin_led_out     (G_io_gpio_led_out         ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_dbg_idx              (G_io_dbg_idx              )  // src[CLK*] --> tgt[CLK*] // latched  
  );

/// unitNode(RV32I_ARCH1_step) : CFG[CLK*] IO[CLK*]
wire       G_RV32I_ARCH1_step_3_OUT    ;
  RV32I_ARCH1_step M3 (
  /*inputs*/
    .clk(clk), .rst_n(rst_n),
    .G_axi_raddr_s_ready    (G_axi_m_ch_0_raddr_s_ready), // tgt[CLK*] <-- src[CLK*]
    .G_axi_waddr_s_ready    (G_axi_m_ch_0_waddr_s_ready), // tgt[CLK*] <-- src[CLK*]
    .G_axi_rdat_s_data      (G_axi_m_ch_0_rdat_s_data  ), // tgt[CLK*] <-- src[CLK*]
    .G_axi_rdat_s_resp      (G_axi_m_ch_0_rdat_s_resp  ), // tgt[CLK*] <-- src[CLK*]
    .G_axi_rdat_s_valid     (G_axi_m_ch_0_rdat_s_valid ), // tgt[CLK*] <-- src[CLK*]
    .G_axi_wdat_s_ready     (G_axi_m_ch_0_wdat_s_ready ), // tgt[CLK*] <-- src[CLK*]
    .G_axi_wres_s_resp      (G_axi_m_ch_0_wres_s_resp  ), // tgt[CLK*] <-- src[CLK*]
    .G_axi_wres_s_valid     (G_axi_m_ch_0_wres_s_valid ), // tgt[CLK*] <-- src[CLK*]
    .G_intr_p_0             (G_io_intr_p_0             ), // tgt[CLK*] <-- src[CLK*]
    .G_intr_p_1             (G_io_intr_p_1             ), // tgt[CLK*] <-- src[CLK*]
    .G_intr_p_2             (G_io_intr_p_2             ), // tgt[CLK*] <-- src[CLK*]
    .G_intr_p_3             (G_io_intr_p_3             ), // tgt[CLK*] <-- src[CLK*]
    .G_intr_p_4             (G_io_intr_p_4             ), // tgt[CLK*] <-- src[CLK*]
    .G_intr_p_5             (G_io_intr_p_5             ), // tgt[CLK*] <-- src[CLK*]
    .G_intr_p_6             (G_io_intr_p_6             ), // tgt[CLK*] <-- src[CLK*]
    .G_dbg_idx              (G_io_dbg_idx              ), // tgt[CLK*] <-- src[CLK*]
  /*outputs*/
    .G_axi_raddr_m_addr     (G_axi_m_ch_0_raddr_m_addr ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_raddr_m_size     (G_axi_m_ch_0_raddr_m_size ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_raddr_m_valid    (G_axi_m_ch_0_raddr_m_valid), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_raddr_m_len      (G_axi_m_ch_0_raddr_m_len  ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_raddr_m_prot     (G_axi_m_ch_0_raddr_m_prot ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_waddr_m_addr     (G_axi_m_ch_0_waddr_m_addr ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_waddr_m_size     (G_axi_m_ch_0_waddr_m_size ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_waddr_m_valid    (G_axi_m_ch_0_waddr_m_valid), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_waddr_m_len      (G_axi_m_ch_0_waddr_m_len  ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_waddr_m_prot     (G_axi_m_ch_0_waddr_m_prot ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_rdat_m_ready     (G_axi_m_ch_0_rdat_m_ready ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_wdat_m_data      (G_axi_m_ch_0_wdat_m_data  ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_wdat_m_strobe    (G_axi_m_ch_0_wdat_m_strobe), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_wdat_m_valid     (G_axi_m_ch_0_wdat_m_valid ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_wdat_m_last      (G_axi_m_ch_0_wdat_m_last  ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_axi_wres_m_ready     (G_axi_m_ch_0_wres_m_ready ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_dbg_data             (G_io_dbg_data             ), // src[CLK*] --> tgt[CLK*] // latched  
    .G_dbg_rv_state         (G_io_dbg_rv_state         ), // src[CLK*] --> tgt[CLK*] // latched  
    .G_dbg_wfi              (G_io_dbg_wfi              ), // src[CLK*] --> tgt[CLK*] // latched  
    .G_RV32I_ARCH1_step_OUT (G_RV32I_ARCH1_step_3_OUT  )  // src[CLK*] --> tgt[CLK*] // latched  
  );
assign     G_RV32I_SOC_ARCH1_OUT        =  G_RV32I_ARCH1_step_3_OUT  ; //  <FB>(comb-Out) (NL) [F(0.0)<0>,B(0.0)<0>] <0,1> [U1] P1 OP(assign:=)(rv32.cpp:L147)[RV32I_SOC_ARCH1]

/// unitNode(AXI4L_CTRL_1_3__connectChannel) : CFG[CLK*] IO[CLK*]
  AXI4L_CTRL_1_3__connectChannel M4 (
  /*inputs*/
    .clk(clk), .rst_n(rst_n),
    .G_bus_m_ch_0_raddr_m_addr  (G_axi_m_ch_0_raddr_m_addr       ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_m_ch_0_raddr_m_size  ({1'b0,G_axi_m_ch_0_raddr_m_size}), // tgt[CLK*] <-- src[CLK*]
    .G_bus_m_ch_0_raddr_m_valid (G_axi_m_ch_0_raddr_m_valid      ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_m_ch_0_raddr_m_len   ({3'b0,G_axi_m_ch_0_raddr_m_len} ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_m_ch_0_raddr_m_prot  (G_axi_m_ch_0_raddr_m_prot       ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_m_ch_0_waddr_m_addr  (G_axi_m_ch_0_waddr_m_addr       ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_m_ch_0_waddr_m_size  ({1'b0,G_axi_m_ch_0_waddr_m_size}), // tgt[CLK*] <-- src[CLK*]
    .G_bus_m_ch_0_waddr_m_valid (G_axi_m_ch_0_waddr_m_valid      ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_m_ch_0_waddr_m_len   ({3'b0,G_axi_m_ch_0_waddr_m_len} ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_m_ch_0_waddr_m_prot  (G_axi_m_ch_0_waddr_m_prot       ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_m_ch_0_rdat_m_ready  (G_axi_m_ch_0_rdat_m_ready       ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_m_ch_0_wdat_m_data   (G_axi_m_ch_0_wdat_m_data        ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_m_ch_0_wdat_m_strobe (G_axi_m_ch_0_wdat_m_strobe      ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_m_ch_0_wdat_m_valid  (G_axi_m_ch_0_wdat_m_valid       ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_m_ch_0_wdat_m_last   (G_axi_m_ch_0_wdat_m_last        ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_m_ch_0_wres_m_ready  (G_axi_m_ch_0_wres_m_ready       ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_s_ch_0_raddr_s_ready (G_axi_s_ch_0_raddr_s_ready      ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_s_ch_0_waddr_s_ready (G_axi_s_ch_0_waddr_s_ready      ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_s_ch_0_rdat_s_data   (G_axi_s_ch_0_rdat_s_data        ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_s_ch_0_rdat_s_resp   ({1'b0,G_axi_s_ch_0_rdat_s_resp} ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_s_ch_0_rdat_s_valid  (G_axi_s_ch_0_rdat_s_valid       ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_s_ch_0_rdat_s_last   (G_axi_s_ch_0_rdat_s_last        ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_s_ch_0_wdat_s_ready  (G_axi_s_ch_0_wdat_s_ready       ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_s_ch_0_wres_s_resp   ({1'b0,G_axi_s_ch_0_wres_s_resp} ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_s_ch_0_wres_s_valid  (G_axi_s_ch_0_wres_s_valid       ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_s_ch_1_raddr_s_ready (G_axi_s_ch_1_raddr_s_ready      ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_s_ch_1_waddr_s_ready (G_axi_s_ch_1_waddr_s_ready      ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_s_ch_1_rdat_s_data   (G_axi_s_ch_1_rdat_s_data        ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_s_ch_1_rdat_s_resp   ({1'b0,G_axi_s_ch_1_rdat_s_resp} ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_s_ch_1_rdat_s_valid  (G_axi_s_ch_1_rdat_s_valid       ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_s_ch_1_rdat_s_last   (G_axi_s_ch_1_rdat_s_last        ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_s_ch_1_wdat_s_ready  (G_axi_s_ch_1_wdat_s_ready       ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_s_ch_1_wres_s_resp   ({1'b0,G_axi_s_ch_1_wres_s_resp} ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_s_ch_1_wres_s_valid  (G_axi_s_ch_1_wres_s_valid       ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_s_ch_2_raddr_s_ready (G_axi_s_ch_2_raddr_s_ready      ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_s_ch_2_waddr_s_ready (G_axi_s_ch_2_waddr_s_ready      ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_s_ch_2_rdat_s_data   (G_axi_s_ch_2_rdat_s_data        ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_s_ch_2_rdat_s_resp   ({1'b0,G_axi_s_ch_2_rdat_s_resp} ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_s_ch_2_rdat_s_valid  (G_axi_s_ch_2_rdat_s_valid       ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_s_ch_2_rdat_s_last   (G_axi_s_ch_2_rdat_s_last        ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_s_ch_2_wdat_s_ready  (G_axi_s_ch_2_wdat_s_ready       ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_s_ch_2_wres_s_resp   ({1'b0,G_axi_s_ch_2_wres_s_resp} ), // tgt[CLK*] <-- src[CLK*]
    .G_bus_s_ch_2_wres_s_valid  (G_axi_s_ch_2_wres_s_valid       ), // tgt[CLK*] <-- src[CLK*]
  /*outputs*/
    .G_bus_m_ch_0_raddr_s_ready (G_axi_m_ch_0_raddr_s_ready      ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_m_ch_0_waddr_s_ready (G_axi_m_ch_0_waddr_s_ready      ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_m_ch_0_rdat_s_data   (G_axi_m_ch_0_rdat_s_data        ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_m_ch_0_rdat_s_resp   (G_axi_m_ch_0_rdat_s_resp        ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_m_ch_0_rdat_s_valid  (G_axi_m_ch_0_rdat_s_valid       ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_m_ch_0_rdat_s_last   (G_axi_m_ch_0_rdat_s_last        ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_m_ch_0_wdat_s_ready  (G_axi_m_ch_0_wdat_s_ready       ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_m_ch_0_wres_s_resp   (G_axi_m_ch_0_wres_s_resp        ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_m_ch_0_wres_s_valid  (G_axi_m_ch_0_wres_s_valid       ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_0_raddr_m_addr  (G_axi_s_ch_0_raddr_m_addr       ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_0_raddr_m_size  (G_axi_s_ch_0_raddr_m_size       ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_0_raddr_m_valid (G_axi_s_ch_0_raddr_m_valid      ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_0_raddr_m_len   (G_axi_s_ch_0_raddr_m_len        ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_0_raddr_m_prot  (G_axi_s_ch_0_raddr_m_prot       ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_0_waddr_m_addr  (G_axi_s_ch_0_waddr_m_addr       ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_0_waddr_m_size  (G_axi_s_ch_0_waddr_m_size       ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_0_waddr_m_valid (G_axi_s_ch_0_waddr_m_valid      ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_0_waddr_m_len   (G_axi_s_ch_0_waddr_m_len        ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_0_waddr_m_prot  (G_axi_s_ch_0_waddr_m_prot       ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_0_rdat_m_ready  (G_axi_s_ch_0_rdat_m_ready       ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_0_wdat_m_data   (G_axi_s_ch_0_wdat_m_data        ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_0_wdat_m_strobe (G_axi_s_ch_0_wdat_m_strobe      ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_0_wdat_m_valid  (G_axi_s_ch_0_wdat_m_valid       ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_0_wdat_m_last   (G_axi_s_ch_0_wdat_m_last        ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_0_wres_m_ready  (G_axi_s_ch_0_wres_m_ready       ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_1_raddr_m_addr  (G_axi_s_ch_1_raddr_m_addr       ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_1_raddr_m_size  (G_axi_s_ch_1_raddr_m_size       ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_1_raddr_m_valid (G_axi_s_ch_1_raddr_m_valid      ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_1_raddr_m_len   (G_axi_s_ch_1_raddr_m_len        ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_1_raddr_m_prot  (G_axi_s_ch_1_raddr_m_prot       ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_1_waddr_m_addr  (G_axi_s_ch_1_waddr_m_addr       ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_1_waddr_m_size  (G_axi_s_ch_1_waddr_m_size       ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_1_waddr_m_valid (G_axi_s_ch_1_waddr_m_valid      ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_1_waddr_m_len   (G_axi_s_ch_1_waddr_m_len        ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_1_waddr_m_prot  (G_axi_s_ch_1_waddr_m_prot       ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_1_rdat_m_ready  (G_axi_s_ch_1_rdat_m_ready       ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_1_wdat_m_data   (G_axi_s_ch_1_wdat_m_data        ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_1_wdat_m_strobe (G_axi_s_ch_1_wdat_m_strobe      ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_1_wdat_m_valid  (G_axi_s_ch_1_wdat_m_valid       ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_1_wdat_m_last   (G_axi_s_ch_1_wdat_m_last        ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_1_wres_m_ready  (G_axi_s_ch_1_wres_m_ready       ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_2_raddr_m_addr  (G_axi_s_ch_2_raddr_m_addr       ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_2_raddr_m_size  (G_axi_s_ch_2_raddr_m_size       ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_2_raddr_m_valid (G_axi_s_ch_2_raddr_m_valid      ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_2_raddr_m_len   (G_axi_s_ch_2_raddr_m_len        ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_2_raddr_m_prot  (G_axi_s_ch_2_raddr_m_prot       ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_2_waddr_m_addr  (G_axi_s_ch_2_waddr_m_addr       ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_2_waddr_m_size  (G_axi_s_ch_2_waddr_m_size       ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_2_waddr_m_valid (G_axi_s_ch_2_waddr_m_valid      ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_2_waddr_m_len   (G_axi_s_ch_2_waddr_m_len        ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_2_waddr_m_prot  (G_axi_s_ch_2_waddr_m_prot       ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_2_rdat_m_ready  (G_axi_s_ch_2_rdat_m_ready       ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_2_wdat_m_data   (G_axi_s_ch_2_wdat_m_data        ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_2_wdat_m_strobe (G_axi_s_ch_2_wdat_m_strobe      ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_2_wdat_m_valid  (G_axi_s_ch_2_wdat_m_valid       ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_2_wdat_m_last   (G_axi_s_ch_2_wdat_m_last        ), // src[CLK*] --> tgt[CLK*] // unlatched
    .G_bus_s_ch_2_wres_m_ready  (G_axi_s_ch_2_wres_m_ready       )  // src[CLK*] --> tgt[CLK*] // unlatched
  );

  /*probe assignments*/

endmodule // RV32I_SOC_ARCH1_probed

