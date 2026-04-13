
  .section  .text.init
  .globl _start
_start:
  la sp, stack_bot  /// initialize sp
  jal main
  wfi
  j _start
  .section .stack,"aw",@nobits
stack_top:
  .skip 0x2000
stack_bot:
