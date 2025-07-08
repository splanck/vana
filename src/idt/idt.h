#ifndef IDT_H
#define IDT_H

#include <stdint.h>

/* IDT gate type and attribute flags */
#define IDT_DESC_PRESENT 0x80
#define IDT_DESC_BIT32   0x08
#define IDT_DESC_INT_GATE 0x0E
#define IDT_DESC_RING3   0x60
#define IDT_DESC_RING0   0x00

#define IDT_DESC_KERNEL_INTERRUPT_GATE (IDT_DESC_PRESENT | IDT_DESC_RING0 | IDT_DESC_INT_GATE)
#define IDT_DESC_USER_INTERRUPT_GATE   (IDT_DESC_PRESENT | IDT_DESC_RING3 | IDT_DESC_INT_GATE)

struct interrupt_frame;
typedef void*(*ISR80H_COMMAND)(struct interrupt_frame* frame);

typedef void(*INTERRUPT_CALLBACK_FUNCTION)(struct interrupt_frame* frame);

struct idt_desc
{
    uint16_t offset_1;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_2;
} __attribute__((packed));

struct idtr_desc
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

struct interrupt_frame
{
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t reserved;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t ip;
    uint32_t cs;
    uint32_t flags;
    uint32_t esp;
    uint32_t ss;
} __attribute__((packed));

/** Set up the IDT and load it into the CPU. */
void idt_init();
/** Enable maskable CPU interrupts with the STI instruction. */
void enable_interrupts();
/** Disable maskable CPU interrupts with the CLI instruction. */
void disable_interrupts();
/** Register a new system call handler for INT 0x80. */
void isr80h_register_command(int command_id, ISR80H_COMMAND command);
/** Invoke a previously registered system call handler. */
void* isr80h_handle_command(int command, struct interrupt_frame* frame);
/** Assembly entry point wrapper for system calls. */
void* isr80h_handler(int command, struct interrupt_frame* frame);
/** Register a callback for a specific interrupt vector. */
int idt_register_interrupt_callback(int interrupt,
                                    INTERRUPT_CALLBACK_FUNCTION interrupt_callback);
/** Simple handler that acknowledges and ignores an interrupt. */
void interrupt_ignore(struct interrupt_frame* frame);

#endif
