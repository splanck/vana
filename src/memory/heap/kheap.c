#include "kheap.h"
#include "heap.h"
#include "config.h"
#include "kernel.h"
#include "memory/memory.h"

struct heap kernel_heap;
struct heap_table kernel_heap_table;

/*
 * kheap_init() - Set up the kernel heap and its table.
 *
 * The kernel reserves a section of memory for dynamic allocation.
 * This routine prepares the block table located at
 * VANA_HEAP_TABLE_ADDRESS and calls heap_create() to clear all entries.
 */
void kheap_init()
{
    int total_table_entries = VANA_HEAP_SIZE_BYTES / VANA_HEAP_BLOCK_SIZE;
    kernel_heap_table.entries = (HEAP_BLOCK_TABLE_ENTRY*)(VANA_HEAP_TABLE_ADDRESS);
    kernel_heap_table.total = total_table_entries;

    void* end = (void*)(VANA_HEAP_ADDRESS + VANA_HEAP_SIZE_BYTES);
    int res = heap_create(&kernel_heap, (void*)(VANA_HEAP_ADDRESS), end, &kernel_heap_table);
    if (res < 0)
    {
        print("Failed to create heap\n");
    }

}

/*
 * kmalloc() - Kernel facing wrapper around heap_malloc().
 */
void* kmalloc(size_t size)
{
    return heap_malloc(&kernel_heap, size);
}

/*
 * kzalloc() - Allocate and zero a memory region from the kernel heap.
 */
void* kzalloc(size_t size)
{
    void* ptr = kmalloc(size);
    if (!ptr)
        return 0;

    memset(ptr, 0x00, size);
    return ptr;
}

/*
 * kfree() - Return memory to the kernel heap.
 */
void kfree(void* ptr)
{
    heap_free(&kernel_heap, ptr);
}
