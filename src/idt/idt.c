#include "idt.h"
#include "gdt/gdt.h"
#include "io/io.h"
#include "memory/memory.h"
#include "kernel.h"

#define IDT_TOTAL_DESCRIPTORS 256

static struct idt_desc idt_descriptors[IDT_TOTAL_DESCRIPTORS];
static struct idtr_desc idtr_descriptor;

extern void* interrupt_pointer_table[IDT_TOTAL_DESCRIPTORS];
extern void idt_load(struct idtr_desc* ptr);

static INTERRUPT_CALLBACK_FUNCTION interrupt_callbacks[IDT_TOTAL_DESCRIPTORS];

void interrupt_ignore(struct interrupt_frame* frame)
{
    (void)frame;
    outb(0x20, 0x20);
}

static void idt_handle_exception(struct interrupt_frame* frame)
{
    (void)frame;
    print("CPU exception\n");
    outb(0x20, 0x20);
}

static void idt_set(int interrupt_no, void* address)
{
    struct idt_desc* desc = &idt_descriptors[interrupt_no];
    desc->offset_1 = (uint32_t)address & 0xFFFF;
    desc->selector = GDT_KERNEL_CODE_SELECTOR;
    desc->zero = 0;
    desc->type_attr = 0x8E;
    desc->offset_2 = (uint32_t)address >> 16;
}

void no_interrupt_handler()
{
    outb(0x20, 0x20);
}

void interrupt_handler(int interrupt, struct interrupt_frame* frame)
{
    if (interrupt < IDT_TOTAL_DESCRIPTORS)
    {
        if (interrupt_callbacks[interrupt])
        {
            interrupt_callbacks[interrupt](frame);
        }
        else
        {
            panic("Unhandled interrupt\n");
        }
    }
    outb(0x20, 0x20);
}

void idt_init()
{
    memset(idt_descriptors, 0, sizeof(idt_descriptors));
    idtr_descriptor.limit = sizeof(idt_descriptors) - 1;
    idtr_descriptor.base = (uint32_t)idt_descriptors;


    for (int i = 0; i < IDT_TOTAL_DESCRIPTORS; i++)
    {
        idt_set(i, interrupt_pointer_table[i]);
    }

    memset(interrupt_callbacks, 0, sizeof(interrupt_callbacks));

    idt_load(&idtr_descriptor);

    for (int i = 0; i <= 31; i++)
    {
        idt_register_interrupt_callback(i, idt_handle_exception);
    }
    idt_register_interrupt_callback(0x27, interrupt_ignore);
}

int idt_register_interrupt_callback(int interrupt, INTERRUPT_CALLBACK_FUNCTION callback)
{
    if (interrupt < 0 || interrupt >= IDT_TOTAL_DESCRIPTORS)
    {
        return -1;
    }

    interrupt_callbacks[interrupt] = callback;
    return 0;
}
