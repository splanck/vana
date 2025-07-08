#include "heap.h"
#include "kernel.h"
#include "status.h"
#include "memory/memory.h"
#include <stdbool.h>

/*
 * Block based heap allocator used by both the kernel and user processes.
 *
 * The heap is backed by a contiguous range of memory divided into fixed size
 * blocks (`VANA_HEAP_BLOCK_SIZE`).  A parallel table mirrors this layout and
 * stores a byte per block describing its state:
 *
 *   +--------+--------+--------+         heap memory
 *   | block0 | block1 | block2 | ...
 *   +--------+--------+--------+
 *     |        |        |
 *     v        v        v
 *   +----+   +----+   +----+            block table
 *   |entry|  |entry|  |entry|  ...
 *   +----+   +----+   +----+
 *
 * Flags within each entry indicate whether the block is free or taken,
 * whether it is the first block in an allocated run and whether more blocks
 * follow.  Allocation requests are aligned to the block size and satisfied by
 * locating a chain of free entries.
 *
 * The routines below provide creation and validation helpers along with the
 * basic malloc/free interface implemented on top of this table.
 */

/*
 * heap_validate_table() - Sanity check that a heap table matches the memory
 *                         span it describes.
 *
 * The heap itself occupies [ptr, end) while the table should contain one
 * entry per block within that range.  This function verifies the caller
 * supplied table is consistent so later allocations do not walk off the end
 * of either array.
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
 * heap_validate_alignment() - Check pointer alignment against the block size.
 *
 * The allocator requires both the start and end of the heap to fall on
 * a block boundary so that block <-> address translations remain exact.
 */
static bool heap_validate_alignment(void* ptr)
{
    return ((unsigned int)ptr % VANA_HEAP_BLOCK_SIZE) == 0;
}

/*
 * heap_create() - Initialise a heap over [ptr, end).
 *
 * The function wires up the heap structure with the caller provided table
 * and performs a number of checks:
 *   1) `ptr` and `end` must be aligned so blocks begin on clean
 *      boundaries.  This prevents partial block usage.
 *   2) The table must be large enough to describe every block in the
 *      memory range.  heap_validate_table() verifies this.
 *
 * Once validated, all table entries are marked free so allocations can
 * start from a known state.
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
 * heap_align_value_to_upper() - Round up to the next block boundary.
 *
 * All allocations occur in whole blocks.  This helper converts a byte
 * size into the smallest multiple of VANA_HEAP_BLOCK_SIZE which keeps the
 * heap table and memory blocks perfectly in sync.
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
 * heap_get_entry_type() - Strip flag bits from a table entry.
 *
 * The upper bits of an entry record metadata such as "has next" and
 * "is first".  By masking them off we are left with just the free/taken
 * state which simplifies comparisons.
 */
static int heap_get_entry_type(HEAP_BLOCK_TABLE_ENTRY entry)
{
    return entry & 0x0f;
}

/*
 * heap_get_start_block() - Locate a run of free blocks.
 *
 * The allocator linearly scans the block table looking for
 * `total_blocks` consecutive free entries.  When a used entry is
 * encountered the current run length resets.  The first index of a
 * successful run is returned or -ENOMEM when no space remains.
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
 * heap_block_to_address() - Convert a table index to a heap pointer.
 *
 * The heap memory is contiguous so the address of block N is simply:
 *   start_address + (N * block_size)
 */
void* heap_block_to_address(struct heap* heap, int block)
{
    return heap->saddr + (block * VANA_HEAP_BLOCK_SIZE);
}

/*
 * heap_mark_blocks_taken() - Update the table to mark a range as used.
 *
 * The allocation is represented as a chain of entries in the table:
 *   [FIRST | HAS_NEXT] -> [TAKEN | HAS_NEXT] -> ... -> [TAKEN]
 * Each entry after the first clears the FIRST flag so freeing can
 * identify the start of a region.  All but the last set HAS_NEXT to form
 * a linked list-like structure.
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
 * heap_malloc_blocks() - Reserve a number of contiguous blocks.
 *
 * After searching for a suitable run via heap_get_start_block(), each
 * block is marked as taken so subsequent allocations will not reuse it.
 * The caller receives a pointer to the first byte of the run.
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
 * heap_mark_blocks_free() - Release an allocated block chain.
 *
 * Starting from `starting_block` the function walks forward clearing entries
 * until one is found without the HAS_NEXT flag.  This mirrors the chain set
 * up by heap_mark_blocks_taken() and ensures the entire region becomes
 * available again.
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
 * heap_address_to_block() - Convert a pointer into a table index.
 *
 * Useful when freeing memory: given a pointer into the heap the function
 * determines which entry in the table owns it by subtracting the heap
 * start and dividing by the block size.
 */
int heap_address_to_block(struct heap* heap, void* address)
{
    return ((int)(address - heap->saddr)) / VANA_HEAP_BLOCK_SIZE;
}

/*
 * heap_malloc() - Allocate a number of bytes from the heap.
 *
 * The requested size is rounded up so that allocations always start and end
 * on block boundaries.  After alignment the allocator simply reserves the
 * required number of blocks.
 */
void* heap_malloc(struct heap* heap, size_t size)
{
    size_t aligned_size = heap_align_value_to_upper(size);
    uint32_t total_blocks = aligned_size / VANA_HEAP_BLOCK_SIZE;
    return heap_malloc_blocks(heap, total_blocks);
}

/*
 * heap_free() - Release memory obtained from heap_malloc().
 *
 * The pointer is translated back into a block index and the chain of
 * entries is cleared via heap_mark_blocks_free().
 */
void heap_free(struct heap* heap, void* ptr)
{
    heap_mark_blocks_free(heap, heap_address_to_block(heap, ptr));
}
