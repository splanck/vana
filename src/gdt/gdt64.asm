[BITS 64]

section .asm

global gdt64
global gdt64_descriptor
extern tss64

gdt64:
    dq 0                        ; Null descriptor
    dq 0x00af9a000000ffff       ; 64-bit code segment
    dq 0x00af92000000ffff       ; 64-bit data segment
    ; TSS descriptor
    dw 0x0067                   ; limit
    dw tss64 & 0xFFFF           ; base 0:15
    db (tss64 >> 16) & 0xFF     ; base 16:23
    db 0x89                     ; type (available 64-bit TSS)
    db ((0x0067 >> 16) & 0xF)   ; limit 16:19 and flags
    db (tss64 >> 24) & 0xFF     ; base 24:31
    dd tss64 >> 32              ; base 32:63
    dd 0                        ; reserved

gdt64_end:

gdt64_descriptor:
    dw gdt64_end - gdt64 - 1
    dq gdt64
