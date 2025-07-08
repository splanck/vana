[BITS 32]

section .asm

global paging_load_directory
global enable_paging

; paging_load_directory - set CR3 to the provided page directory
paging_load_directory:
    push ebp
    mov ebp, esp
    mov eax, [ebp+8]
    mov cr3, eax
    pop ebp
    ret

; enable_paging - turn on paging by setting the PG bit in CR0
enable_paging:
    push ebp
    mov ebp, esp
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
    pop ebp
    ret
