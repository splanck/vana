#include "syscall.h"
#include "gdt/gdt.h"

static syscall_handler_t syscall_table[256];

void syscall_register(int num, syscall_handler_t handler)
{
    if (num >= 0 && num < 256)
        syscall_table[num] = handler;
}

uint64_t syscall_dispatch(uint64_t num)
{
    syscall_handler_t handler = 0;
    if (num < 256)
        handler = syscall_table[num];
    if (!handler)
        return (uint64_t)-1;
    return handler();
}

static inline void wrmsr(uint32_t msr, uint64_t val)
{
    uint32_t low = (uint32_t)val;
    uint32_t high = (uint32_t)(val >> 32);
    __asm__ volatile("wrmsr" :: "c"(msr), "a"(low), "d"(high));
}

extern void syscall_entry(void);

void syscall_init(void)
{
    const uint64_t star = ((uint64_t)GDT64_KERNEL_CODE_SELECTOR << 32) |
                          ((uint64_t)GDT64_USER_CODE_SELECTOR << 48);
    wrmsr(0xC0000081, star);               /* MSR_STAR */
    wrmsr(0xC0000082, (uint64_t)syscall_entry); /* MSR_LSTAR */
    wrmsr(0xC0000084, 0x200);              /* MSR_SFMASK: mask IF */
}
