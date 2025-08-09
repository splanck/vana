; Temporary page table builder for 64-bit bring-up
; Creates identity mapping for low memory and maps the kernel to KERNEL_VMA

[BITS 32]

%define P           1
%define RW          (1 << 1)
%define G           (1 << 8)
%define FLAGS       (P | RW | G)

%define KERNEL_VMA         0xffffffff80000000
%define KERNEL_LOAD_ADDR   0x00100000 ; 1 MiB
%define ID_MAP_SIZE        (16 * 1024 * 1024)

global setup_pagetables

; Build initial paging structures
setup_pagetables:
    pushad

    ; Clear all tables
    mov edi, pml4_table
    mov ecx, (tables_end - pml4_table) / 4
    xor eax, eax
    rep stosd

    ; ----- PML4 -----
    mov eax, pdpt_identity
    or eax, FLAGS
    mov [pml4_table + 0*8], eax
    mov [pml4_table + 0*8 + 4], dword 0

    mov eax, pdpt_kernel
    or eax, FLAGS
    mov [pml4_table + 511*8], eax
    mov [pml4_table + 511*8 + 4], dword 0

    ; ----- PDPTs -----
    mov eax, pd_identity
    or eax, FLAGS
    mov [pdpt_identity + 0*8], eax
    mov [pdpt_identity + 0*8 + 4], dword 0

    mov eax, pd_kernel
    or eax, FLAGS
    mov [pdpt_kernel + 510*8], eax
    mov [pdpt_kernel + 510*8 + 4], dword 0

    ; ----- PD for identity map (16 MiB using 4K pages) -----
    mov ecx, 8                     ; 8 * 512 * 4K = 16 MiB
    mov esi, pt_identity
    mov edi, pd_identity
.pd_identity_loop:
    mov eax, esi
    or eax, FLAGS
    mov [edi], eax
    mov [edi+4], dword 0
    add esi, 4096
    add edi, 8
    loop .pd_identity_loop

    ; ----- PD for kernel mapping -----
    mov eax, pt_kernel
    or eax, FLAGS
    mov [pd_kernel + 0*8], eax
    mov [pd_kernel + 0*8 + 4], dword 0

    ; ----- PT entries for identity map -----
    mov ecx, ID_MAP_SIZE / 4096
    mov ebx, 0
    mov edi, pt_identity
.pt_identity_loop:
    mov eax, ebx
    or eax, FLAGS
    mov [edi], eax
    mov [edi+4], dword 0
    add ebx, 4096
    add edi, 8
    loop .pt_identity_loop

    ; ----- PT entries for kernel high mapping -----
    mov ecx, 512                  ; Map first 2 MiB of kernel
    mov ebx, KERNEL_LOAD_ADDR
    mov edi, pt_kernel
.pt_kernel_loop:
    mov eax, ebx
    or eax, FLAGS
    mov [edi], eax
    mov [edi+4], dword 0
    add ebx, 4096
    add edi, 8
    loop .pt_kernel_loop

    popad
    ret

; ---------------- Page table storage ----------------

align 4096
pml4_table:     times 512 dq 0

align 4096
pdpt_identity:  times 512 dq 0

align 4096
pd_identity:    times 512 dq 0

; 8 page tables for 16 MiB identity mapping
align 4096
pt_identity:    times (512*8) dq 0

align 4096
pdpt_kernel:    times 512 dq 0

align 4096
pd_kernel:      times 512 dq 0

align 4096
pt_kernel:      times 512 dq 0

align 4096
tables_end:

