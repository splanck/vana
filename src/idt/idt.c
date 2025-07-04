#include "idt.h"
#include "gdt/gdt.h"
#include "io/io.h"
#include "pic/pic.h"
#include "memory/memory.h"
#include "kernel.h"
#include "string/string.h"
#include "config.h"
#include "task/task.h"
#include "task/process.h"

#define IDT_TOTAL_DESCRIPTORS 256

static struct idt_desc idt_descriptors[IDT_TOTAL_DESCRIPTORS];
static struct idtr_desc idtr_descriptor;

extern void* interrupt_pointer_table[IDT_TOTAL_DESCRIPTORS];
extern void idt_load(struct idtr_desc* ptr);
extern void isr80h_wrapper();

static ISR80H_COMMAND isr80h_commands[VANA_MAX_ISR80H_COMMANDS];

static INTERRUPT_CALLBACK_FUNCTION interrupt_callbacks[IDT_TOTAL_DESCRIPTORS];

void interrupt_ignore(struct interrupt_frame* frame)
{
    (void)frame;
    outb(0xA0, 0x20);   // acknowledge slave
    outb(0x20, 0x20);   // acknowledge master
}

static void idt_handle_exception(struct interrupt_frame* frame)
{
    (void)frame;
    process_terminate(task_current()->process);
    task_next();
}

static void idt_set(int interrupt_no, void* address, uint8_t type_attr)
{
    struct idt_desc* desc = &idt_descriptors[interrupt_no];
    desc->offset_1 = (uint32_t)address & 0xFFFF;
    desc->selector = GDT_KERNEL_CODE_SELECTOR;
    desc->zero = 0;
    desc->type_attr = type_attr;
    desc->offset_2 = (uint32_t)address >> 16;
}

void no_interrupt_handler()
{
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
            char buf[32];
            int_to_string(interrupt, buf);
            print("Unhandled interrupt ");
            print(buf);
            print("\n");
            panic("");
        }
    }

    if (interrupt >= 0x20 && interrupt <= 0x2F)
    {
        pic_send_eoi(interrupt - 0x20);
    }
}

void idt_init()
{
    memset(idt_descriptors, 0, sizeof(idt_descriptors));
    idtr_descriptor.limit = sizeof(idt_descriptors) - 1;
    idtr_descriptor.base = (uint32_t)idt_descriptors;


    for (int i = 0; i < IDT_TOTAL_DESCRIPTORS; i++)
    {
        idt_set(i, interrupt_pointer_table[i], IDT_DESC_KERNEL_INTERRUPT_GATE);
    }

    idt_set(0x80, isr80h_wrapper, IDT_DESC_USER_INTERRUPT_GATE);

    memset(interrupt_callbacks, 0, sizeof(interrupt_callbacks));

    idt_load(&idtr_descriptor);

    for (int i = 0; i <= 31; i++)
    {
        idt_register_interrupt_callback(i, idt_handle_exception);
    }
    idt_register_interrupt_callback(0x27, interrupt_ignore);
    idt_register_interrupt_callback(0x2E, interrupt_ignore); // IDE (IRQ14)
    idt_register_interrupt_callback(0x2F, interrupt_ignore);
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
/*
 * Register a system call handler for the given command id.
 * Panics if the id is invalid or already registered.
 */

void isr80h_register_command(int command_id, ISR80H_COMMAND command)
{
    if (command_id < 0 || command_id >= VANA_MAX_ISR80H_COMMANDS)
    {
        panic("isr80h command out of bounds\n");
    }

    if (isr80h_commands[command_id])
    {
        panic("isr80h command already registered\n");
    }

    isr80h_commands[command_id] = command;
}
/*
 * Look up the handler for a command and execute it.
 * Returns the handler result or NULL for invalid commands.
 */

void* isr80h_handle_command(int command, struct interrupt_frame* frame)
{
    void* result = 0;

    if (command < 0 || command >= VANA_MAX_ISR80H_COMMANDS)
    {
        return 0;
    }

    ISR80H_COMMAND func = isr80h_commands[command];
    if (!func)
    {
        return 0;
    }

    result = func(frame);
    return result;
}
/*
 * Top level ISR for 0x80. Saves task state, dispatches the command and
 * restores the user page table.
 */

void* isr80h_handler(int command, struct interrupt_frame* frame)
{
    void* res = 0;
    kernel_page();
    task_current_save_state(frame);
    res = isr80h_handle_command(command, frame);
    task_page();
    return res;
}
