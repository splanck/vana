[BITS 64]

section .data

global tss64

tss64:
    dd 0           ; reserved0
    dd 0           ; pad
    dq 0           ; rsp0
    dq 0           ; rsp1
    dq 0           ; rsp2
    dq 0           ; reserved1
    dq 0           ; ist1
    dq 0           ; ist2
    dq 0           ; ist3
    dq 0           ; ist4
    dq 0           ; ist5
    dq 0           ; ist6
    dq 0           ; ist7
    dq 0           ; reserved2
    dw 0           ; reserved3
    dw 0           ; io map base

section .text

global tss64_load

tss64_load:
    mov ax, di
    ltr ax
    ret
