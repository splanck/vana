[BITS 32]

global _start
extern c_start
extern vana_exit

section .asm

_start:
    call c_start
    push eax
    call vana_exit
    add esp, 4
    ret
