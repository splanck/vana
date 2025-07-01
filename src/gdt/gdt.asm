section .asm
[BITS 32]

global gdt_load

CODE_SEG equ 0x08
DATA_SEG equ 0x10

; void gdt_load(struct gdt_descriptor* descriptor)
gdt_load:
    push ebp
    mov ebp, esp

    mov eax, [ebp+8] ; pointer to descriptor
    lgdt [eax]

    ; Reload segment registers
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Far jump to reload code segment
    jmp CODE_SEG:flush
flush:
    pop ebp
    ret
