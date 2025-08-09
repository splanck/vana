OUTPUT_FORMAT("elf64-x86-64")
ENTRY(_start)

KERNEL_PHYS = 1M;
KERNEL_VMA = 0xffffffff80000000;
_phys_to_virt_offset = KERNEL_VMA - KERNEL_PHYS;

PHDRS
{
    text   PT_LOAD FLAGS(5);
    rodata PT_LOAD FLAGS(4);
    data   PT_LOAD FLAGS(6);
}

SECTIONS
{
    . = KERNEL_VMA;
    _kernel_start = .;

    .text : ALIGN(0x1000) AT(KERNEL_PHYS)
    {
        _text = .;
        *(.text*)
        _etext = .;
    } :text

    .rodata : ALIGN(0x1000) AT(KERNEL_PHYS + (ADDR(.rodata) - KERNEL_VMA))
    {
        *(.rodata*)
    } :rodata

    .data : ALIGN(0x1000) AT(KERNEL_PHYS + (ADDR(.data) - KERNEL_VMA))
    {
        _data = .;
        *(.data*)
        _edata = .;
    } :data

    .bss : ALIGN(0x1000) (NOLOAD) AT(KERNEL_PHYS + (ADDR(.bss) - KERNEL_VMA))
    {
        _bss = .;
        *(COMMON)
        *(.bss*)
        _ebss = .;
    } :data

    .asm : ALIGN(0x1000) AT(KERNEL_PHYS + (ADDR(.asm) - KERNEL_VMA))
    {
        *(.asm*)
    } :text

    _kernel_end = .;
}
