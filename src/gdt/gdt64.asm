[BITS 64]

section .asm

global gdt64
global gdt64_descriptor

gdt64:
    dq 0                        ; Null descriptor
    dq 0x00af9a000000ffff       ; 64-bit code segment
    dq 0x00af92000000ffff       ; 64-bit data segment
    dq 0                        ; TSS descriptor (low 8 bytes)
    dq 0                        ; TSS descriptor (high 8 bytes)

gdt64_end:

gdt64_descriptor:
    dw gdt64_end - gdt64 - 1
    dq gdt64
