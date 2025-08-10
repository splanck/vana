#include "idt64.h"
#include "gdt/gdt.h"

static struct idt64_desc idt64_descriptors[IDT64_TOTAL_DESCRIPTORS];
static struct idtr64_desc idtr64_descriptor;

extern void* interrupt_pointer_table[IDT64_TOTAL_DESCRIPTORS];
extern void idt64_load(struct idtr64_desc* ptr);

static void idt64_set(int interrupt_no, void* address)
{
    struct idt64_desc* desc = &idt64_descriptors[interrupt_no];
    uint64_t addr = (uint64_t)address;
    desc->offset_low = addr & 0xFFFF;
    desc->selector = GDT64_KERNEL_CODE_SELECTOR;
    desc->ist = 0;
    desc->type_attr = 0x8E; /* present, ring0, interrupt gate */
    desc->offset_mid = (addr >> 16) & 0xFFFF;
    desc->offset_high = (uint32_t)(addr >> 32);
    desc->zero = 0;
}

void idt64_init(void)
{
    idtr64_descriptor.limit = sizeof(idt64_descriptors) - 1;
    idtr64_descriptor.base = (uint64_t)&idt64_descriptors;

    for (int i = 0; i < IDT64_TOTAL_DESCRIPTORS; i++)
        idt64_set(i, interrupt_pointer_table[i]);

    idt64_load(&idtr64_descriptor);
}
