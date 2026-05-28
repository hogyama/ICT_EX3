#SRC_V = RV32I_SOC_ARCH1.RTL.v AXI4L_CTRL_1_3__connectChannel.RTL.v GPIO_AXI4L_step.RTL.v UART_AXI4L_step.RTL.v 
#SRC_V += RV32I_ARCH1.v RV32I_ARCH1_CORE.v PROBE_UART_step.RTL.v
#SRC_V += RV32I_SOC_ARCH1.TESTBENCH.v

#all: $(SRC_V)
	iverilog -o RV32I_SOC_ARCH1.out $(SRC_V)
	./RV32I_SOC_ARCH1.out

SRC_V  = RV32I_SOC_ARCH1.RTL.v
SRC_V += AXI4L_CTRL_1_3__connectChannel.RTL.v
SRC_V += GPIO_AXI4L_step.RTL.v
SRC_V += UART_AXI4L_step.RTL.v
SRC_V += RV32I_ARCH1.v
SRC_V += RV32I_ARCH1_CORE.v
SRC_V += PROBE_UART_step.RTL.v
SRC_V += RV32I_SOC_ARCH1.TESTBENCH.v

OUT = RV32I_SOC_ARCH1.out

all: $(OUT)
	./$(OUT)

$(OUT): $(SRC_V)
	iverilog -o $(OUT) $(SRC_V)

clean:
	rm -f $(OUT) *.vcd
