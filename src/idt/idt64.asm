[BITS 64]

section .text

global idt64_load

idt64_load:
    mov rax, rdi
    lidt [rax]
    ret

default_interrupt:
    iretq

section .data

global interrupt_pointer_table
interrupt_pointer_table:
    times 256 dq default_interrupt
