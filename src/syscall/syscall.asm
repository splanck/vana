[BITS 64]

section .bss
align 8
syscall_user_rsp: resq 1

section .text
global syscall_entry
extern syscall_dispatch
extern tss64

syscall_entry:
    mov rdi, rax                ; syscall number -> first argument
    swapgs
    mov [syscall_user_rsp], rsp ; save user stack pointer
    mov rsp, [tss64 + 8]        ; switch to kernel stack (rsp0)
    sub rsp, 8                  ; maintain 16-byte alignment
    push rcx                    ; save return RIP
    push r11                    ; save RFLAGS
    call syscall_dispatch       ; dispatch based on syscall number
    pop r11                     ; restore RFLAGS
    pop rcx                     ; restore return RIP
    add rsp, 8                  ; remove alignment padding
    mov rsp, [syscall_user_rsp] ; restore user stack
    swapgs
    sysretq
