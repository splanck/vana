[BITS 64]

section .text

global task_switch64

; void task_switch64(struct registers* regs)
; Pointer to regs is passed in RDI

task_switch64:
    ; Save base pointer to r11
    mov r11, rdi

    ; Restore general purpose registers
    mov r15, [r11 + 0]
    mov r14, [r11 + 8]
    mov r13, [r11 + 16]
    mov r12, [r11 + 24]
    mov rbx, [r11 + 32]
    mov rbp, [r11 + 40]
    mov rdi, [r11 + 48]
    mov rsi, [r11 + 56]
    mov rdx, [r11 + 64]
    mov rcx, [r11 + 72]
    mov r8,  [r11 + 80]
    mov r9,  [r11 + 88]
    mov rax, [r11 + 96]

    ; Prepare rflags with interrupts enabled
    mov r10, [r11 + 120]
    or r10, 0x200

    ; Push return frame for iretq
    push qword [r11 + 136]    ; ss
    push qword [r11 + 128]    ; rsp
    push r10                  ; rflags
    push qword [r11 + 112]    ; cs
    push qword [r11 + 104]    ; rip
    iretq
