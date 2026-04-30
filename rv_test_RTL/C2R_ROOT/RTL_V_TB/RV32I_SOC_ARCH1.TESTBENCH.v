
module RV32I_SOC_ARCH1_testbench;

reg clk, rst_n; /// global clock, reset_n
/****** RV32I_SOC_ARCH1 inputs *******/
reg [15:0] G_io_gpio_sw_in; /// <0,65535> [U16];
reg G_io_intr_p_2, G_io_intr_p_3, G_io_intr_p_4, G_io_intr_p_5, G_io_intr_p_6 ;
reg G_rx_in_0, G_rx_in_1;
reg [ 7:0] G_probe_uart_port_tx_data_0, G_probe_uart_port_tx_data_1;
reg G_probe_uart_port_valid_in_0, G_probe_uart_port_valid_in_1;
/****** RV32I_SOC_ARCH1 outputs *******/
wire G_io_uart_0_tx, G_io_uart_0_rts, G_io_uart_1_tx, G_io_uart_1_rts;
wire  [ 7:0] G_io_gpio_seg7_dout, G_io_gpio_seg7_an;
wire  [14:0] G_io_gpio_led_out;
wire G_RV32I_SOC_ARCH1_OUT, G_io_uart_0_rx, G_io_uart_0_cts, G_io_uart_1_rx, G_io_uart_1_cts;
/****** PROBE_UART outputs *******/
wire G_probe_uart_port_ready_0, G_probe_uart_port_ready_1;

RV32I_SOC_ARCH1 M0 (
  /* inputs */
  clk, rst_n,
  G_io_uart_0_rx, G_io_uart_0_cts, G_io_uart_1_rx, G_io_uart_1_cts, G_io_gpio_sw_in, 
  G_io_intr_p_2, G_io_intr_p_3, G_io_intr_p_4, G_io_intr_p_5, G_io_intr_p_6, 
  /* outputs */
  G_io_uart_0_tx, G_io_uart_0_rts, G_io_uart_1_tx, G_io_uart_1_rts, G_io_gpio_seg7_dout, 
  G_io_gpio_seg7_an, G_io_gpio_led_out, G_RV32I_SOC_ARCH1_OUT
);

PROBE_UART_step M1 (
  /* inputs */
  clk, rst_n,
  1'b0, G_io_uart_0_tx, G_probe_uart_port_tx_data_0, G_probe_uart_port_valid_in_0, 
  /* outputs */
  G_io_uart_0_rx, G_probe_uart_port_ready_0
);

PROBE_UART_step M2 (
  /* inputs */
  clk, rst_n,
  1'b1, G_io_uart_1_tx, G_probe_uart_port_tx_data_1, G_probe_uart_port_valid_in_1, 
  /* outputs */
  G_io_uart_1_rx, G_probe_uart_port_ready_1
);

task RESET_INPUTS;
begin
  G_io_gpio_sw_in <= 0;
  G_io_intr_p_2 <= 0;
  G_io_intr_p_3 <= 0;
  G_io_intr_p_4 <= 0;
  G_io_intr_p_5 <= 0;
  G_io_intr_p_6 <= 0;
  G_rx_in_0 <= 0;
  G_rx_in_1 <= 0;
  G_probe_uart_port_tx_data_0 <= 0;
  G_probe_uart_port_tx_data_1 <= 0;
  G_probe_uart_port_valid_in_0 <= 0;
  G_probe_uart_port_valid_in_1 <= 0;
end
endtask

///////////////// global reset /////////////////
reg global_reset_n;
initial begin
  global_reset_n = 0; #2000; global_reset_n = 1;
end


///////////////// clk : period = 100 (0, 50, 50) , negEdgeFlag = 0 /////////////////
always begin
  #0; /// clkOffset --> avoid simultaneous multiple clock edges
  clk = 1; #50;
  clk = 0; #50;
end
initial begin
  clk = 0; /// clk = 0 @ t = 0
  rst_n = 0; /// activate reset @ t = 0
  RESET_INPUTS();
  @ (posedge global_reset_n);
  @ (negedge clk);
  rst_n <= 1; /// release reset (non-blocking assignment)
  $display($time, " : rst_n <-- 1 : release reset");
  @ (posedge clk);
  $display($time, " : first clk posedge");
end

integer intr_cycle;
initial begin
  intr_cycle = 0;
end

task DETECT_WFI;  /// called after @(negedge clk)
begin
  if (G_RV32I_SOC_ARCH1_OUT)
    G_io_intr_p_2 <= 1;
  intr_cycle <= intr_cycle + G_io_intr_p_2;
end
endtask

parameter MAX_CYCLE = 900000;
parameter MAX_INTR_CYCLE = 10;

integer cycle;

initial begin
  @(posedge rst_n);

  cycle = 0;
  while ((cycle < MAX_CYCLE) & (intr_cycle < MAX_INTR_CYCLE)) begin
      @(negedge clk);
      DETECT_WFI();
      cycle = cycle + 1;
  end 
  $display($stime, " : finish verilog simulation : %d cycles", cycle);
  $finish;
end /// initial

endmodule /// (RV32I_SOC_ARCH1_testbench)
