#include "heap.h"
#include "kernel.h"
#include "status.h"
#include "memory/memory.h"
#include <stdbool.h>

/*
 * Block based heap allocator used by both the kernel and user processes.
 *
 * The heap is backed by a contiguous range of memory divided into fixed
 * size blocks (`VANA_HEAP_BLOCK_SIZE`). A table entry accompanies each block
 * describing whether the block is free, taken, the first in a run and/or has
 * a successor. Allocation requests are aligned to the block size and fulfilled
 * by finding a sequence of free blocks in the table.
 *
 * The routines below provide creation and validation helpers along with the
 * basic malloc/free interface implemented on top of this block table.
 */

/*
 * Ensure the provided heap table correctly describes the memory range
 * starting at `ptr` and ending at `end`. The size of the range must
 * equal `table->total` blocks.
 */
static int heap_validate_table(void* ptr, void* end, struct heap_table* table)
{
    int res = 0;

    size_t table_size = (size_t)(end - ptr);
    size_t total_blocks = table_size / VANA_HEAP_BLOCK_SIZE;
    if (table->total != total_blocks)
    {
        res = -EINVARG;
        goto out;
    }

out:
    return res;
}

/*
 * Verify that `ptr` is aligned to the heap block size boundary.
 */
static bool heap_validate_alignment(void* ptr)
{
    return ((unsigned int)ptr % VANA_HEAP_BLOCK_SIZE) == 0;
}

/*
 * Initialise a heap structure over the memory range [ptr, end). The
 * supplied table must contain one entry per block within this range.
 * All entries are cleared so the heap starts empty.
 */
int heap_create(struct heap* heap, void* ptr, void* end, struct heap_table* table)
{
    int res = 0;

    if (!heap_validate_alignment(ptr) || !heap_validate_alignment(end))
    {
        res = -EINVARG;
        goto out;
    }

    memset(heap, 0, sizeof(struct heap));
    heap->saddr = ptr;
    heap->table = table;

    res = heap_validate_table(ptr, end, table);
    if (res < 0)
    {
        goto out;
    }

    size_t table_size = sizeof(HEAP_BLOCK_TABLE_ENTRY) * table->total;
    memset(table->entries, HEAP_BLOCK_TABLE_ENTRY_FREE, table_size);

out:
    return res;
}

/*
 * Round `val` up to the next multiple of the heap block size.
 */
static uint32_t heap_align_value_to_upper(uint32_t val)
{
    if ((val % VANA_HEAP_BLOCK_SIZE) == 0)
    {
        return val;
    }

    val = (val - ( val % VANA_HEAP_BLOCK_SIZE));
    val += VANA_HEAP_BLOCK_SIZE;
    return val;
}

/*
 * Mask off flag bits and return the raw entry type for a block table entry.
 */
static int heap_get_entry_type(HEAP_BLOCK_TABLE_ENTRY entry)
{
    return entry & 0x0f;
}

/*
 * Scan the heap table for `total_blocks` consecutive free entries.
 * Returns the starting block index or -ENOMEM if none can be found.
 */
int heap_get_start_block(struct heap* heap, uint32_t total_blocks)
{
    struct heap_table* table = heap->table;
    int bc = 0;
    int bs = -1;

    for (size_t i = 0; i < table->total; i++)
    {
        if (heap_get_entry_type(table->entries[i]) != HEAP_BLOCK_TABLE_ENTRY_FREE)
        {
            bc = 0;
            bs = -1;
            continue;
        }

        // If this is the first block
        if (bs == -1)
        {
            bs = i;
        }
        bc++;
        if (bc == total_blocks)
        {
            break;
        }
    }

    if (bs == -1)
    {
        return -ENOMEM;
    }
    
    return bs;

}

/*
 * Convert a block index into an absolute memory address within the heap.
 */
void* heap_block_to_address(struct heap* heap, int block)
{
    return heap->saddr + (block * VANA_HEAP_BLOCK_SIZE);
}

/*
 * Mark `total_blocks` entries starting at `start_block` as taken in the
 * heap table. The first entry is flagged with HEAP_BLOCK_IS_FIRST and all
 * but the last entry set HEAP_BLOCK_HAS_NEXT so freeing can walk the chain.
 */
void heap_mark_blocks_taken(struct heap* heap, int start_block, int total_blocks)
{
    int end_block = (start_block + total_blocks)-1;
    
    HEAP_BLOCK_TABLE_ENTRY entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN | HEAP_BLOCK_IS_FIRST;
    if (total_blocks > 1)
    {
        entry |= HEAP_BLOCK_HAS_NEXT;
    }

    for (int i = start_block; i <= end_block; i++)
    {
        heap->table->entries[i] = entry;
        entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN;
        if (i != end_block -1)
        {
            entry |= HEAP_BLOCK_HAS_NEXT;
        }
    }
}

/*
 * Allocate `total_blocks` consecutive blocks and return their starting
 * address. If a suitable run cannot be found `NULL` is returned.
 */
void* heap_malloc_blocks(struct heap* heap, uint32_t total_blocks)
{
    void* address = 0;

    int start_block = heap_get_start_block(heap, total_blocks);
    if (start_block < 0)
    {
        goto out;
    }

    address = heap_block_to_address(heap, start_block);

    // Mark the blocks as taken
    heap_mark_blocks_taken(heap, start_block, total_blocks);

out:
    return address;
}

/*
 * Clear table entries starting at `starting_block` until a block without the
 * HEAP_BLOCK_HAS_NEXT flag is encountered. This effectively frees a previously
 * allocated run of blocks.
 */
void heap_mark_blocks_free(struct heap* heap, int starting_block)
{
    struct heap_table* table = heap->table;
    for (int i = starting_block; i < (int)table->total; i++)
    {
        HEAP_BLOCK_TABLE_ENTRY entry = table->entries[i];
        table->entries[i] = HEAP_BLOCK_TABLE_ENTRY_FREE;
        if (!(entry & HEAP_BLOCK_HAS_NEXT))
        {
            break;
        }
    }
}

/*
 * Translate a heap address back into its corresponding block index.
 */
int heap_address_to_block(struct heap* heap, void* address)
{
    return ((int)(address - heap->saddr)) / VANA_HEAP_BLOCK_SIZE;
}

/*
 * Public malloc entry point. The request size is aligned up to a block
 * boundary and the required number of blocks are allocated.
 */
void* heap_malloc(struct heap* heap, size_t size)
{
    size_t aligned_size = heap_align_value_to_upper(size);
    uint32_t total_blocks = aligned_size / VANA_HEAP_BLOCK_SIZE;
    return heap_malloc_blocks(heap, total_blocks);
}

/*
 * Free a block of memory previously returned by heap_malloc(). The starting
 * table entry is located via the address and then cleared along with any
 * chained entries.
 */
void heap_free(struct heap* heap, void* ptr)
{
    heap_mark_blocks_free(heap, heap_address_to_block(heap, ptr));
}
