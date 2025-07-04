# Memory Management

This document summarizes the heap and paging implementations found under `src/memory` along with the basic memory utility functions.

On startup the kernel configures paging by building an identity-mapped 4 GB directory. The routine `paging_new_4gb()` allocates each page table, fills it with the correct flags, and returns the resulting chunk. Once created the directory is activated with `paging_switch` and paging is enabled in the CPU.

Each process receives its own directory based on this initial mapping so the kernel remains identity mapped while programs are mapped at `VANA_PROGRAM_VIRTUAL_ADDRESS`. Task switches simply call `paging_switch` to install the proper directory.

Dynamic memory inside the kernel is served from a dedicated heap created in `kheap_init()`. User programs allocate memory through `process_malloc` and release it with `process_free`; these helpers allocate from the kernel heap but map the pages into the requesting process.
## Heap (`heap.c`, `kheap.c`)

Kernel dynamic memory is provided by a simple block based heap. The heap uses a table (`struct heap_table`) where each byte describes a block. The relevant configuration values are defined in `src/config.h`:

```c
#define VANA_HEAP_SIZE_BYTES 104857600
#define VANA_HEAP_BLOCK_SIZE 4096
#define VANA_HEAP_ADDRESS 0x01000000
#define VANA_HEAP_TABLE_ADDRESS 0x00007E00
#define VANA_PROGRAM_VIRTUAL_ADDRESS 0x400000
#define VANA_USER_PROGRAM_STACK_SIZE (1024 * 16)
#define VANA_PROGRAM_VIRTUAL_STACK_ADDRESS_START 0x3FF000
```

The kernel heap is created in `kheap_init()` which sets up the table and calls `heap_create()`:

```c
struct heap kernel_heap;
struct heap_table kernel_heap_table;

void kheap_init()
{
    int total_table_entries = VANA_HEAP_SIZE_BYTES / VANA_HEAP_BLOCK_SIZE;
    kernel_heap_table.entries = (HEAP_BLOCK_TABLE_ENTRY*)(VANA_HEAP_TABLE_ADDRESS);
    kernel_heap_table.total = total_table_entries;

    void* end = (void*)(VANA_HEAP_ADDRESS + VANA_HEAP_SIZE_BYTES);
    int res = heap_create(&kernel_heap, (void*)(VANA_HEAP_ADDRESS), end, &kernel_heap_table);
    if (res < 0)
        print("Failed to create heap\n");
}
```

`heap_create()` validates alignment and zeroes the table entries so all blocks start free. Allocations are made in multiples of `VANA_HEAP_BLOCK_SIZE`. `heap_malloc()` aligns the requested size upward, finds a run of free blocks via `heap_get_start_block()`, marks them taken with `heap_mark_blocks_taken()` and returns the start address.

Freeing memory with `heap_free()` walks the table starting at the block corresponding to the pointer and clears entries until the `HEAP_BLOCK_HAS_NEXT` flag is no longer set.

`kmalloc`, `kzalloc` and `kfree` in `kheap.c` simply wrap these heap functions for kernel code.

User processes do not share this heap. Instead, `process_malloc()` and
`process_free()` manage per‑process allocations and are exposed to user space
through the system call commands 4 and 5. These helpers ultimately rely on the
kernel heap but map the memory into the requesting task's page directory.

## Paging (`paging.c`, `paging.asm`)

Paging is initialized by creating a 4 GB identity‑mapped page directory in `paging_new_4gb()`:

```c
struct paging_4gb_chunk* paging_new_4gb(uint8_t flags)
{
    uint32_t* directory = kzalloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE);
    int offset = 0;
    for (int i = 0; i < PAGING_TOTAL_ENTRIES_PER_TABLE; i++)
    {
        uint32_t* entry = kzalloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE);
        for (int b = 0; b < PAGING_TOTAL_ENTRIES_PER_TABLE; b++)
            entry[b] = (offset + (b * PAGING_PAGE_SIZE)) | flags;
        offset += (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE);
        directory[i] = (uint32_t)entry | flags | PAGING_IS_WRITEABLE;
    }
    struct paging_4gb_chunk* chunk_4gb = kzalloc(sizeof(struct paging_4gb_chunk));
    chunk_4gb->directory_entry = directory;
    return chunk_4gb;
}
```

The assembly routines `paging_load_directory` and `enable_paging` load the directory address into `CR3` and set the PG bit in `CR0`:

```asm
paging_load_directory:
    push ebp
    mov ebp, esp
    mov eax, [ebp+8]
    mov cr3, eax
    pop ebp
    ret

enable_paging:
    push ebp
    mov ebp, esp
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
    pop ebp
    ret
```

Higher level helpers such as `paging_map()` and `paging_map_range()` map physical pages into the current directory. Addresses passed to these functions must be 4 KB aligned; `paging_align_address()` and `paging_align_to_lower_page()` are provided to assist with alignment.

`paging_get_physical_address()` resolves a virtual address back to the physical frame by consulting the directory and page tables.

## Memory Utilities (`memory.c`)

`memory.c` contains small C implementations of `memset`, `memcmp` and `memcpy` used throughout the kernel:

```c
void* memset(void* ptr, int c, size_t size)
{
    char* c_ptr = (char*)ptr;
    for (int i = 0; i < size; i++)
        c_ptr[i] = (char)c;
    return ptr;
}
```

These helpers avoid relying on the C standard library during early boot and are linked into the kernel directly.
