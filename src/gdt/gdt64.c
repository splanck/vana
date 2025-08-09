/**
 * @file gdt64.c
 * @brief Install the 64-bit Global Descriptor Table.
 */

#include "gdt.h"

void gdt64_init(void)
{
    extern struct gdt64_descriptor gdt64_descriptor;
    __asm__ volatile("lgdt %0" : : "m"(gdt64_descriptor));
}
