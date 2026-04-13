# project name

SHELL=/usr/bin/bash -O extglob

PROJNAME = rv32

# specify items which appear in C2RTL Navigation Panel.

#SRCS= RVProc.cpp RVProcTest.cpp RVProc_io.cpp RVProc_io_ext.cpp fpu/Fpu_Float.cpp fpu/Fpu_Double.cpp 
SRCS= rv32.cpp rv32_io.cpp
SRCS+= ../C2Rlib/utils/C2RUtil.cpp ../C2Rlib/utils/ELFUtil.cpp
USE_C2RTL_OPT = 1
USE_SOC_MODEL = 1

C2RTL_OFF = 1

#VCU108 = 1
ifeq ($(VCU108), 1)
UART_16550 = 1
#UART_XILINX = 1
endif

TOP_FUNC_FILE = .TOP_FUNC
MODULES_FILE = .MODULES

include $(TOP_FUNC_FILE)
include $(MODULES_FILE)
$(TOP_FUNC_FILE): ;
$(MODULES_FILE): ;

include ../Makefile.in

ifeq ($(VCU108), 1)
CFLAGS += -DVCU108=1
endif
# UART type
#CFLAGS += -DUART_SIFIVE=1
ifeq ($(UART_16550), 1)
CFLAGS += -DUART_16550=1
endif
ifeq ($(UART_XILINX), 1)
CFLAGS += -DUART_XLNXLITE=1
endif

