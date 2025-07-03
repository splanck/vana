.section .text
.globl _start
.align 4

_start:
    pop %eax          # argc
    mov %esp, %ebx    # argv
    push $0           # fake return address
    push %ebx         # argv
    push %eax         # argc
    call main
    mov %eax, %ebx    # status -> ebx
    mov $VANA_SYS_EXIT, %eax
    int $0x80
