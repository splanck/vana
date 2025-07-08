#include "pic.h"
#include "io/io.h"

/**
 * @file pic.c
 * @brief Helpers for interacting with the Programmable Interrupt Controller.
 *
 * The master and slave PICs live at I/O ports 0x20/0xA0 (commands) and
 * 0x21/0xA1 (data).  During early boot the kernel remaps them so hardware
 * IRQs start at vector 0x20, leaving the first 32 vectors for CPU
 * exceptions. Each interrupt must be acknowledged with an End Of Interrupt
 * command.  If the interrupt originated from the slave (IRQs 8-15) both
 * controllers require an acknowledgement.
 */

/**
 * Send an End Of Interrupt command to the PICs.
 *
 * @param irq The IRQ number being acknowledged. IRQs >= 8 originate from the
 *            slave controller and therefore both controllers must be updated.
 */
void pic_send_eoi(int irq)
{
    if (irq >= 8)
    {
        /* Inform the slave PIC that the IRQ has been handled. */
        outb(PIC2_COMMAND, PIC_EOI);
    }
    /* Always notify the master PIC as well. */
    outb(PIC1_COMMAND, PIC_EOI);
}
