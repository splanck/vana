section .asm
[BITS 32]

extern interrupt_handler
extern no_interrupt_handler
extern isr80h_handler

global idt_load
global no_interrupt
global enable_interrupts
global disable_interrupts
global isr80h_wrapper
global interrupt_pointer_table

enable_interrupts:
    sti
    ret

disable_interrupts:
    cli
    ret

idt_load:
    push ebp
    mov ebp, esp
    mov eax, [ebp+8]
    lidt [eax]
    pop ebp
    ret

no_interrupt:
    pushad
    call no_interrupt_handler
    popad
    iret

%macro interrupt 1
    global int%1
    int%1:
        pushad
        push esp
        push dword %1
        call interrupt_handler
        add esp, 8
        popad
        iret
%endmacro

%assign i 0
%rep 256
    interrupt i
%assign i i+1
%endrep

isr80h_wrapper:
    ; INTERRUPT FRAME START
    pushad
    ; INTERRUPT FRAME END
    push esp
    push eax
    call isr80h_handler
    mov [tmp_res], eax
    add esp, 8
    popad
    mov eax, [tmp_res]
    iretd

section .data
tmp_res: dd 0

%macro interrupt_array_entry 1
    dd int%1
%endmacro

interrupt_pointer_table:
%assign i 0
%rep 256
    interrupt_array_entry i
%assign i i+1
%endrep
