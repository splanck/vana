/*
 * paging.c - page directory and mapping helpers
 *
 * Provides creation of identity-mapped page tables, utilities to map
 * virtual addresses, translate them back to physical addresses and
 * switch directories. Each paging_4gb_chunk owns a directory with
 * 1024 page tables covering the full 4GiB space.
 */
#include "paging.h"
#include "memory/memory.h"
#include "memory/heap/kheap.h"
#include <stdbool.h>
#include <stdint.h>

void paging_load_directory(uint32_t* directory);

static uint32_t* current_directory = 0;
/**
 * paging_new_4gb() - Create an identity mapped paging directory.
 *
 * The layout is the classic two level x86 scheme:
 *
 *   Directory[1024] --> Table[1024] --> 4KiB page
 *             |                          (physical)
 *             +--> Table[1024] --> ...
 *
 * Each directory entry maps a 4MB region.  By setting up 1024 of them the
 * full 4GiB address space is identity mapped.  The supplied @flags specify
 * attributes for every page such as PRESENT or WRITEABLE.
 */

struct paging_4gb_chunk* paging_new_4gb(uint8_t flags)
{
    uint32_t* directory = kzalloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE);
    int offset = 0;
    for (int i = 0; i < PAGING_TOTAL_ENTRIES_PER_TABLE; i++)
    {
        uint32_t* entry = kzalloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE);
        for (int b = 0; b < PAGING_TOTAL_ENTRIES_PER_TABLE; b++)
        {
            entry[b] = (offset + (b * PAGING_PAGE_SIZE)) | flags;
        }
        offset += (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE);
        directory[i] = (uint32_t)entry | flags | PAGING_IS_WRITEABLE;
    }

    struct paging_4gb_chunk* chunk_4gb = kzalloc(sizeof(struct paging_4gb_chunk));
    chunk_4gb->directory_entry = directory;
    return chunk_4gb;
}

/*
 * paging_switch() - Replace the current paging directory.
 *
 * Updates CR3 via paging_load_directory() so the CPU begins using the
 * new directory for address translation.  The pointer is also stored in
 * current_directory for later queries.
 */
void paging_switch(struct paging_4gb_chunk* directory)
{
    paging_load_directory(directory->directory_entry);
    current_directory = directory->directory_entry;
}

/*
 * paging_free_4gb() - Tear down a paging directory created by
 * paging_new_4gb().  Each page table referenced from the directory is
 * freed and finally the directory itself.
 */
void paging_free_4gb(struct paging_4gb_chunk* chunk)
{
    for (int i = 0; i < PAGING_TOTAL_ENTRIES_PER_TABLE; i++)
    {
        uint32_t entry = chunk->directory_entry[i];
        uint32_t* table = (uint32_t*)(entry & 0xfffff000);
        kfree(table);
    }

    kfree(chunk->directory_entry);
    kfree(chunk);
}

/*
 * paging_4gb_chunk_get_directory() - Helper to access the raw directory
 * pointer from a paging chunk structure.
 */
uint32_t* paging_4gb_chunk_get_directory(struct paging_4gb_chunk* chunk)
{
    return chunk->directory_entry;
}

/*
 * paging_is_aligned() - Convenience wrapper to check 4KiB alignment.
 */
bool paging_is_aligned(void* addr)
{
    return ((uint32_t)addr % PAGING_PAGE_SIZE) == 0;
}

/*
 * paging_get_indexes() - Split a virtual address into directory and table
 * indexes.  The address must be aligned so page offsets are zero.
 */
static int paging_get_indexes(void* virtual_address, uint32_t* directory_index_out, uint32_t* table_index_out)
{
    int res = 0;
    if (!paging_is_aligned(virtual_address))
    {
        res = -1;
        goto out;
    }

    *directory_index_out = ((uint32_t)virtual_address / (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE));
    *table_index_out = ((uint32_t)virtual_address % (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE) / PAGING_PAGE_SIZE);
out:
    return res;
}

/*
 * paging_align_address() - Round an address up to the next page boundary.
 */
void* paging_align_address(void* ptr)
{
    if ((uint32_t)ptr % PAGING_PAGE_SIZE)
    {
        return (void*)((uint32_t)ptr + PAGING_PAGE_SIZE - ((uint32_t)ptr % PAGING_PAGE_SIZE));
    }

    return ptr;
}

/*
 * paging_align_to_lower_page() - Truncate an address down to the nearest
 * page boundary.  Used when translating non aligned virtual addresses back
 * to physical ones.
 */
void* paging_align_to_lower_page(void* addr)
{
    uint32_t _addr = (uint32_t) addr;
    _addr -= (_addr % PAGING_PAGE_SIZE);
    return (void*) _addr;
}

/**
 * Map a single 4KiB page within a paging_4gb_chunk.
 * Both `virt` and `phys` must be page aligned.
 * @param directory  Target paging chunk whose directory is updated.
 * @param virt       Virtual address to map.
 * @param phys       Physical address to map to.
 * @param flags      Entry flags combined with the physical frame.
 * @return           0 on success or negative on alignment error.
 */
/*
 * paging_map() - Map a single 4KiB page.
 *
 * Both virtual and physical addresses must already be aligned.  The
 * function simply composes the page table entry with the provided flags
 * and updates the directory using paging_set().
 */
int paging_map(struct paging_4gb_chunk* directory, void* virt, void* phys, int flags)
{
    if (((unsigned int)virt % PAGING_PAGE_SIZE) || ((unsigned int) phys % PAGING_PAGE_SIZE))
    {
        return -1;
    }

    return paging_set(directory->directory_entry, virt, (uint32_t) phys | flags);
}

/*
 * paging_map_range() - Map several sequential pages.
 */
int paging_map_range(struct paging_4gb_chunk* directory, void* virt, void* phys, int count, int flags)
{
    int res = 0;
    for (int i = 0; i < count; i++)
    {
        res = paging_map(directory, virt, phys, flags);
        if (res < 0)
            break;
        virt += PAGING_PAGE_SIZE;
        phys += PAGING_PAGE_SIZE;
    }

    return res;
}

/*
 * paging_map_to() - Map a range given explicit start and end physical
 * addresses.  All three addresses must be page aligned.
 */
int paging_map_to(struct paging_4gb_chunk *directory, void *virt, void *phys, void *phys_end, int flags)
{
    int res = 0;
    if ((uint32_t)virt % PAGING_PAGE_SIZE)
    {
        res = -1;
        goto out;
    }
    if ((uint32_t)phys % PAGING_PAGE_SIZE)
    {
        res = -1;
        goto out;
    }
    if ((uint32_t)phys_end % PAGING_PAGE_SIZE)
    {
        res = -1;
        goto out;
    }

    if ((uint32_t)phys_end < (uint32_t)phys)
    {
        res = -1;
        goto out;
    }

    uint32_t total_bytes = (uint32_t)phys_end - (uint32_t)phys;
    int total_pages = total_bytes / PAGING_PAGE_SIZE;
    res = paging_map_range(directory, virt, phys, total_pages, flags);
out:
    return res;
}

/*
 * paging_set() - Write a raw value into the page tables.
 *
 * Used internally after computing the correct indices.  The 0xfffff000 mask
 * strips flag bits from the directory entry yielding the physical address of
 * the page table.  Alignment checks ensure the caller passed a page aligned
 * address so the indexes are valid.
 */
int paging_set(uint32_t* directory, void* virt, uint32_t val)
{
    if (!paging_is_aligned(virt))
    {
        return -1;
    }

    uint32_t directory_index = 0;
    uint32_t table_index = 0;
    int res = paging_get_indexes(virt, &directory_index, &table_index);
    if (res < 0)
    {
        return res;
    }

    uint32_t entry = directory[directory_index];
    uint32_t* table = (uint32_t*)(entry & 0xfffff000);
    table[table_index] = val;

    return 0;
}

/**
 * Translate a virtual address to the mapped physical address.
 * The lookup uses the provided paging directory and handles non page-aligned
 * addresses by rounding down to the nearest page.
 * @param directory  Paging directory to search.
 * @param virt       Virtual address to translate.
 * @return           The physical address mapped to `virt`.
 */
/*
 * paging_get_physical_address() - Translate a virtual address.
 *
 * Non aligned addresses are rounded down first and the offset re-added to
 * the resulting physical page.  The paging_get() helper performs the table
 * lookup.
 */
void* paging_get_physical_address(uint32_t* directory, void* virt)
{
    void* virt_addr_new = (void*) paging_align_to_lower_page(virt);
    void* difference = (void*)((uint32_t) virt - (uint32_t) virt_addr_new);
    return (void*)((paging_get(directory, virt_addr_new) & 0xfffff000) + (uint32_t)difference);
}

/*
 * paging_get() - Fetch the raw table entry for a virtual address.
 */
uint32_t paging_get(uint32_t* directory, void* virt)
{
    uint32_t directory_index = 0;
    uint32_t table_index = 0;
    paging_get_indexes(virt, &directory_index, &table_index);

    uint32_t entry = directory[directory_index];
    uint32_t* table = (uint32_t*)(entry & 0xfffff000);
    return table[table_index];
}
