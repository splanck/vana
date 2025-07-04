#include "pic.h"
#include "io/io.h"

/*
 * pic.c - Basic helpers for the Programmable Interrupt Controller.
 *
 * During early boot the master and slave PICs are remapped in
 * `kernel.asm` so hardware IRQs use vectors 0x20-0x2F. All lines are
 * initially masked. Once an interrupt is handled an End Of Interrupt
 * (EOI) command must be sent to the controller. For IRQs delivered by
 * the slave both PICs must receive an EOI.
*/

// Send an EOI to the PICs for the specified IRQ number.
// IRQs >= 8 originate from the slave and require acknowledging both chips.
void pic_send_eoi(int irq)
{
    if (irq >= 8)
    {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}
