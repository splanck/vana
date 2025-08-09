[BITS 64]

global _start
extern kernel_main

gdt64:
    dq 0                      ; Null descriptor
    dq 0x00af9a000000ffff     ; 64-bit code segment
    dq 0x00af92000000ffff     ; 64-bit data segment

gdtr:
    dw gdtr_end - gdt64 - 1
    dq gdt64

_start:
    lgdt [gdtr]

    lea rax, [rel .reload_cs]
    push 0x08
    push rax
    retfq
.reload_cs:
    xor eax, eax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    mov rsp, 0xffffffff80000000 + 0x200000
    mov rbp, rsp

    call kernel_main

.hang:
    hlt
    jmp .hang

gdtr_end:
