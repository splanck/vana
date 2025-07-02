[BITS 32]

global _start
extern c_start
extern vana_exit

section .asm

_start:
    call c_start
    call vana_exit
    ret
