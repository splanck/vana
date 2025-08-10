#ifndef PAGING64_H
#define PAGING64_H

#include <stdint.h>
#include <stddef.h>

/* 64-bit page table entry */
typedef uint64_t pte_t;

/* Flag masks */
#define PTE_P   (1ull << 0)  /* Present */
#define PTE_RW  (1ull << 1)  /* Read/write */
#define PTE_US  (1ull << 2)  /* User/supervisor */
#define PTE_PWT (1ull << 3)  /* Write-through */
#define PTE_PCD (1ull << 4)  /* Cache disable */
#define PTE_A   (1ull << 5)  /* Accessed */
#define PTE_D   (1ull << 6)  /* Dirty */
#define PTE_PS  (1ull << 7)  /* Page size */
#define PTE_G   (1ull << 8)  /* Global */
#define PTE_NX  (1ull << 63) /* No execute */

#define PAGE_SIZE 0x1000

/* Higher half direct map base and size */
#define HHDM_BASE 0xffff800000000000ULL
#define DIRECT_MAP_PAGES (1024*1024) /* map first 1 GiB */

void paging64_init(uint64_t hhdm_base);

int map_page(uint64_t virt, uint64_t phys, uint64_t flags);
int map_range(uint64_t virt, uint64_t phys, size_t count, uint64_t flags);
int unmap(uint64_t virt);
uint64_t virt_to_phys(uint64_t virt);
uint64_t phys_to_virt(uint64_t phys);

#endif
