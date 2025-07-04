#include "kheap.h"
#include "heap.h"
#include "config.h"
#include "kernel.h"
#include "memory/memory.h"

struct heap kernel_heap;
struct heap_table kernel_heap_table;

// Configure the kernel_heap structure and its block table
// so dynamic memory allocations can be serviced.
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

// Allocate memory from the kernel heap
void* kmalloc(size_t size)
{
    return heap_malloc(&kernel_heap, size);
}

// Allocate zero-initialized memory from the kernel heap
void* kzalloc(size_t size)
{
    void* ptr = kmalloc(size);
    if (!ptr)
        return 0;

    memset(ptr, 0x00, size);
    return ptr;
}

// Free a block previously allocated with kmalloc/kzalloc
void kfree(void* ptr)
{
    heap_free(&kernel_heap, ptr);
}
