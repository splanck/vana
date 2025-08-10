#include <stdint.h>
#include <stddef.h>
#include "memory/paging/paging64.h"
#include "gdt/gdt.h"
#include "idt/idt64.h"
#include "task/process.h"
#include "syscall/syscall.h"


extern char _text[], _etext[], _data[], _edata[], _bss[], _ebss[], _phys_to_virt_offset[];

static size_t pages_count(uint64_t start, uint64_t end)
{
    return (end - start + PAGE_SIZE - 1) / PAGE_SIZE;
}

void kernel_main(void)
{
    paging64_init(HHDM_BASE);

    uint64_t offset = (uint64_t)_phys_to_virt_offset;

    /* Map kernel .text as RX */
    map_range((uint64_t)_text, (uint64_t)_text - offset,
              pages_count((uint64_t)_text, (uint64_t)_etext), 0);

    /* Map .data and .bss as RW and NX */
    map_range((uint64_t)_data, (uint64_t)_data - offset,
              pages_count((uint64_t)_data, (uint64_t)_edata), PTE_RW | PTE_NX);
    map_range((uint64_t)_bss, (uint64_t)_bss - offset,
              pages_count((uint64_t)_bss, (uint64_t)_ebss), PTE_RW | PTE_NX);

    gdt64_init();
    idt64_init();
    tss64_init(HHDM_BASE + 0x200000);
    syscall_init();
}
