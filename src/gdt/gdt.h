#ifndef GDT_H
#define GDT_H

#include <stdint.h>

#define GDT_KERNEL_CODE_SELECTOR 0x08
#define GDT_KERNEL_DATA_SELECTOR 0x10
#define GDT_TSS_SELECTOR 0x28

struct gdt
{
    uint16_t segment;
    uint16_t base_first;
    uint8_t base;
    uint8_t access;
    uint8_t high_flags;
    uint8_t base_24_31_bits;
} __attribute__((packed));

struct gdt_structured
{
    uint32_t base;
    uint32_t limit;
    uint8_t type;
};

struct gdt_descriptor
{
    uint16_t size;
    uint32_t address;
} __attribute__((packed));

/**
 * Load the Global Descriptor Table and make it the active descriptor set.
 */
void gdt_load(struct gdt_descriptor* descriptor);
/**
 * Convert an array of structured descriptors into packed form ready for gdt_load.
 */
void gdt_structured_to_gdt(struct gdt* gdt, struct gdt_structured* structured_gdt,
                           int total_entries);

#endif
