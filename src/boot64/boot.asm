BITS 16

; Boot loader to enter 64-bit long mode and jump to kernel
; Loads kernel64.elf at 1MiB and sets up temporary identity-mapped page tables

KERNEL_LOAD_ADDR equ 0x100000
CODE_SEG equ 0x08
DATA_SEG equ 0x10

global _start
_start:
    cli
    ; Enable A20
    in al, 0x92
    or al, 0x02
    out 0x92, al

    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    ; Load kernel64.elf (assumed 200 sectors from LBA 1)
    mov eax, 1          ; LBA start
    mov ecx, 200        ; sector count
    mov edi, KERNEL_LOAD_ADDR
    call ata_lba_read

    ; Setup GDT and enter protected mode
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    jmp CODE_SEG:pmode

[BITS 32]
pmode:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    ; Build temporary page tables
    call setup_pagetables

    ; Load PML4 physical address into CR3
    mov eax, pml4_table
    mov cr3, eax

    ; Enable PAE
    mov eax, cr4
    or eax, (1 << 5)
    mov cr4, eax

    ; Enable long mode
    mov ecx, 0xC0000080
    rdmsr
    or eax, (1 << 8)
    wrmsr

    ; Enable paging
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    ; Jump to 64-bit mode
    jmp 0x08:long_mode

[BITS 64]
long_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    mov rax, KERNEL_LOAD_ADDR
    jmp rax

[BITS 16]
;---------------- Disk routine ----------------
ata_lba_read:
    mov ebx, eax
    shr eax, 24
    or eax, 0xE0
    mov dx, 0x1F6
    out dx, al

    mov eax, ecx
    mov dx, 0x1F2
    out dx, al

    mov eax, ebx
    mov dx, 0x1F3
    out dx, al
    mov eax, ebx
    mov dx, 0x1F4
    shr eax, 8
    out dx, al
    mov eax, ebx
    mov dx, 0x1F5
    shr eax, 16
    out dx, al

    mov dx, 0x1F7
    mov al, 0x20
    out dx, al

.next:
    push ecx
.wait:
    mov dx, 0x1F7
    in al, dx
    test al, 8
    jz .wait

    mov ecx, 256
    mov dx, 0x1F0
    rep insw
    pop ecx
    loop .next
    ret

;---------------- GDT ----------------
gdt_start:
    dq 0
    gdt_code: dq 0x00af9a000000ffff
    gdt_data: dq 0x00af92000000ffff
gdt_end:

  gdt_descriptor:
      dw gdt_end - gdt_start - 1
      dd gdt_start

  %include "src/boot64/pagetables.asm"
