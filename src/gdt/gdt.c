/*
 * gdt.c - encode and install Global Descriptor Table entries
 *
 * The kernel describes segments using struct gdt_structured.
 * encode_gdt_entry converts each structured entry into the packed
 * 8-byte format required by the CPU, setting limit granularity and
 * base fields. gdt_structured_to_gdt iterates over an array of
 * structured entries and produces the final table ready for gdt_load.
 */
#include "gdt.h"
#include "kernel.h"

static void encode_gdt_entry(uint8_t* target, struct gdt_structured source)
{
    if ((source.limit > 65536) && ((source.limit & 0xFFF) != 0xFFF))
    {
        panic("encode_gdt_entry: Invalid argument\n");
    }

    target[6] = 0x40;
    if (source.limit > 65536)
    {
        source.limit >>= 12;
        target[6] = 0xC0;
    }

    target[0] = source.limit & 0xFF;
    target[1] = (source.limit >> 8) & 0xFF;
    target[6] |= (source.limit >> 16) & 0x0F;

    target[2] = source.base & 0xFF;
    target[3] = (source.base >> 8) & 0xFF;
    target[4] = (source.base >> 16) & 0xFF;
    target[7] = (source.base >> 24) & 0xFF;

    target[5] = source.type;
}
/**
 * Convert an array of structured descriptors into packed GDT entries.
 * @param gdt             Output array of raw descriptors.
 * @param structured_gdt  Input array describing each segment.
 * @param total_entries   Number of entries in the array.
 */

void gdt_structured_to_gdt(struct gdt* gdt, struct gdt_structured* structured_gdt, int total_entries)
{
    for (int i = 0; i < total_entries; i++)
    {
        encode_gdt_entry((uint8_t*)&gdt[i], structured_gdt[i]);
    }
}

