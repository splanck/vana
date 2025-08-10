[BITS 64]

section .bss
align 8
syscall_user_rsp: resq 1

section .text
global syscall_entry
extern syscall_dispatch
extern tss64

syscall_entry:
    swapgs
    mov [syscall_user_rsp], rsp ; save user stack pointer
    mov rsp, [tss64 + 8]        ; switch to kernel stack (rsp0)

    push rcx                    ; save return RIP
    push r11                    ; save RFLAGS
    push r9                     ; sixth argument

    mov r11, rdi                ; preserve first argument
    mov rcx, rdx                ; third argument
    mov rdx, rsi                ; second argument
    mov rsi, r11                ; first argument
    mov rdi, rax                ; syscall number
    mov r11, r8                 ; save fifth argument
    mov r8, r10                 ; fourth argument
    mov r9, r11                 ; fifth argument

    call syscall_dispatch       ; dispatch based on syscall number

    add rsp, 8                  ; discard sixth argument
    pop r11                     ; restore RFLAGS
    pop rcx                     ; restore return RIP
    mov rsp, [syscall_user_rsp] ; restore user stack
    swapgs
    sysretq
