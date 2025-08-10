#include "paging64.h"

static uint64_t hhdm_offset = 0;
static uint64_t next_free_phys = 0x200000; /* start at 2 MiB for tables */
static pte_t* pml4 = 0;

static uint64_t alloc_page_phys(void)
{
    uint64_t phys = next_free_phys;
    next_free_phys += PAGE_SIZE;
    /* zero memory via direct map */
    uint64_t* ptr = (uint64_t*)phys_to_virt(phys);
    for (size_t i = 0; i < PAGE_SIZE / sizeof(uint64_t); i++)
        ptr[i] = 0;
    return phys;
}

static pte_t* alloc_table(void)
{
    uint64_t phys = alloc_page_phys();
    return (pte_t*)phys_to_virt(phys);
}

static inline uint64_t rdmsr(uint32_t msr)
{
    uint32_t low, high;
    __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

static inline void wrmsr(uint32_t msr, uint64_t val)
{
    uint32_t low = (uint32_t)val;
    uint32_t high = (uint32_t)(val >> 32);
    __asm__ volatile("wrmsr" :: "c"(msr), "a"(low), "d"(high));
}

void paging64_init(uint64_t hhdm_base)
{
    hhdm_offset = hhdm_base;
    pml4 = alloc_table();

    /* Create PML4 entry for the higher half direct map */
    uint16_t hhdm_i = (hhdm_base >> 39) & 0x1FF;
    pte_t* hhdm_pdpt = alloc_table();
    pml4[hhdm_i] = virt_to_phys((uint64_t)hhdm_pdpt) | PTE_P | PTE_RW;

    /* Map a direct window of physical memory */
    map_range(hhdm_base, 0, DIRECT_MAP_PAGES, PTE_RW | PTE_NX);

    /* Load PML4 into CR3 */
    uint64_t cr3 = virt_to_phys((uint64_t)pml4);
    __asm__ volatile("mov %0, %%cr3" :: "r"(cr3));

    /* Enable NXE */
    uint64_t efer = rdmsr(0xC0000080);
    efer |= (1ull << 11); /* NXE */
    wrmsr(0xC0000080, efer);
}

uint64_t phys_to_virt(uint64_t phys)
{
    if (hhdm_offset && phys < (uint64_t)DIRECT_MAP_PAGES * PAGE_SIZE)
        return phys + hhdm_offset;
    return phys;
}

uint64_t virt_to_phys(uint64_t virt)
{
    uint64_t hhdm_end = hhdm_offset + (uint64_t)DIRECT_MAP_PAGES * PAGE_SIZE;
    if (hhdm_offset && virt >= hhdm_offset && virt < hhdm_end)
        return virt - hhdm_offset;

    uint16_t pml4_i = (virt >> 39) & 0x1FF;
    uint16_t pdpt_i = (virt >> 30) & 0x1FF;
    uint16_t pd_i   = (virt >> 21) & 0x1FF;
    uint16_t pt_i   = (virt >> 12) & 0x1FF;

    if (!(pml4[pml4_i] & PTE_P))
        return 0;
    pte_t* pdpt = (pte_t*)phys_to_virt(pml4[pml4_i] & 0x000ffffffffff000ULL);
    if (!(pdpt[pdpt_i] & PTE_P))
        return 0;
    pte_t* pd = (pte_t*)phys_to_virt(pdpt[pdpt_i] & 0x000ffffffffff000ULL);
    if (!(pd[pd_i] & PTE_P))
        return 0;
    pte_t* pt = (pte_t*)phys_to_virt(pd[pd_i] & 0x000ffffffffff000ULL);
    pte_t entry = pt[pt_i];
    if (!(entry & PTE_P))
        return 0;
    return (entry & 0x000ffffffffff000ULL) | (virt & 0xFFF);
}

int map_page(uint64_t virt, uint64_t phys, uint64_t flags)
{
    uint16_t pml4_i = (virt >> 39) & 0x1FF;
    uint16_t pdpt_i = (virt >> 30) & 0x1FF;
    uint16_t pd_i   = (virt >> 21) & 0x1FF;
    uint16_t pt_i   = (virt >> 12) & 0x1FF;

    if (!(pml4[pml4_i] & PTE_P)) {
        pte_t* new_pdpt = alloc_table();
        pml4[pml4_i] = virt_to_phys((uint64_t)new_pdpt) | PTE_P | PTE_RW;
    }
    pte_t* pdpt = (pte_t*)phys_to_virt(pml4[pml4_i] & 0x000ffffffffff000ULL);

    if (!(pdpt[pdpt_i] & PTE_P)) {
        pte_t* new_pd = alloc_table();
        pdpt[pdpt_i] = virt_to_phys((uint64_t)new_pd) | PTE_P | PTE_RW;
    }
    pte_t* pd = (pte_t*)phys_to_virt(pdpt[pdpt_i] & 0x000ffffffffff000ULL);

    if (!(pd[pd_i] & PTE_P)) {
        pte_t* new_pt = alloc_table();
        pd[pd_i] = virt_to_phys((uint64_t)new_pt) | PTE_P | PTE_RW;
    }
    pte_t* pt = (pte_t*)phys_to_virt(pd[pd_i] & 0x000ffffffffff000ULL);

    pt[pt_i] = (phys & 0x000ffffffffff000ULL) | (flags & 0xFFF) | PTE_P;
    return 0;
}

int map_range(uint64_t virt, uint64_t phys, size_t count, uint64_t flags)
{
    for (size_t i = 0; i < count; i++) {
        int res = map_page(virt + i * PAGE_SIZE, phys + i * PAGE_SIZE, flags);
        if (res < 0)
            return res;
    }
    return 0;
}

int unmap(uint64_t virt)
{
    uint16_t pml4_i = (virt >> 39) & 0x1FF;
    uint16_t pdpt_i = (virt >> 30) & 0x1FF;
    uint16_t pd_i   = (virt >> 21) & 0x1FF;
    uint16_t pt_i   = (virt >> 12) & 0x1FF;

    if (!(pml4[pml4_i] & PTE_P))
        return -1;
    pte_t* pdpt = (pte_t*)phys_to_virt(pml4[pml4_i] & 0x000ffffffffff000ULL);
    if (!(pdpt[pdpt_i] & PTE_P))
        return -1;
    pte_t* pd = (pte_t*)phys_to_virt(pdpt[pdpt_i] & 0x000ffffffffff000ULL);
    if (!(pd[pd_i] & PTE_P))
        return -1;
    pte_t* pt = (pte_t*)phys_to_virt(pd[pd_i] & 0x000ffffffffff000ULL);
    pt[pt_i] = 0;
    return 0;
}
