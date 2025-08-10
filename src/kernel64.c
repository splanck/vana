#include <stdint.h>
#include "memory/paging/paging64.h"
#include "gdt/gdt.h"
#include "idt/idt64.h"
#include "task/process.h"
#include "syscall/syscall.h"

#define HHDM_BASE 0xffff800000000000ULL
#define DIRECT_MAP_PAGES (1024*1024) /* map first 1 GiB */

void kernel_main(void)
{
    paging64_init(HHDM_BASE);
    map_range(HHDM_BASE, 0, DIRECT_MAP_PAGES, PTE_RW | PTE_NX);
    gdt64_init();
    idt64_init();
    tss64_init(HHDM_BASE + 0x200000);
    syscall_init();
}
