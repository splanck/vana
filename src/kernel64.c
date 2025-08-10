#include <stdint.h>
#include "memory/paging/paging64.h"

#define HHDM_BASE 0xffff800000000000ULL
#define DIRECT_MAP_PAGES (1024*1024) /* map first 1 GiB */

void kernel_main(void)
{
    paging64_init(HHDM_BASE);
    map_range(HHDM_BASE, 0, DIRECT_MAP_PAGES, PTE_RW | PTE_NX);
}
