#include "pic.h"
#include "io/io.h"

/**
 * @file pic.c
 * @brief Helpers for interacting with the Programmable Interrupt Controller.
 *
 * The PIC is remapped during early boot so that hardware IRQs begin at vector
 * 0x20. Each interrupt must be acknowledged with an End Of Interrupt command.
 * If the interrupt originated from the slave (IRQs 8-15) both PICs require an
 * acknowledgement.
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
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}
