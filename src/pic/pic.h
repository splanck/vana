#ifndef PIC_H
#define PIC_H

#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1
#define PIC_EOI 0x20

/** Notify the PIC that the specified IRQ has been handled. */
void pic_send_eoi(int irq);

#endif
