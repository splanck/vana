#ifndef IDT64_H
#define IDT64_H

#include <stdint.h>

#define IDT64_TOTAL_DESCRIPTORS 256

struct idt64_desc {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed));

struct idtr64_desc {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

void idt64_init(void);

#endif
