
TEST_SRC = boot_rv32.c

#DIR_CC=~/tools/rv32ima/bin
DIR_CC=~/tools/rv32ia/bin
CC=$(DIR_CC)/riscv32-unknown-elf-gcc
LD=$(DIR_CC)/riscv32-unknown-elf-ld
DUMP=$(DIR_CC)/riscv32-unknown-elf-objdump
CFLAGS=-O2

CFLAGS_RV=-march=rv32iazicsr

MEM_START=0x00000000
DATA_START=0x00000c00

LDFLAGS= -I. -nostartfiles -static -Wl,--defsym=MEM_START=$(MEM_START),--defsym=DATA_START=$(DATA_START),-T,vcu108.lds
RV_SRC= entry2.S ../rv32_io_setting.h

OUT= boot_rv32

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

all: $(OUT).out $(OUT).dump

$(OUT).out: $(TEST_SRC) $(RV_SRC)
	$(CC) -o $@ $^ $(LDFLAGS) $(CFLAGS) $(CFLAGS_RV)

$(OUT).dump: $(OUT).out
	$(DUMP) -D $^ > $@

clean:
	rm -rf $(OUT).out *.o $(OUT).dump

