[BITS 64]

section .text

global page_fault_stub
extern page_fault_handler

page_fault_stub:
    ; Save general purpose registers
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; Load CR2 into first argument (RDI)
    mov rdi, cr2
    ; Load error code into second argument (RSI)
    mov rsi, [rsp + 15*8]

    call page_fault_handler

    hlt
