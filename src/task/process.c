#ifdef __x86_64__
#include "task/tss.h"
#include "gdt/gdt.h"
#include "memory/heap/kheap.h"
#include <stddef.h>

extern struct tss64 tss64;
extern void tss64_load(uint16_t tss_segment);

void tss64_init(uint64_t rsp0)
{
    uint64_t* tss = (uint64_t*)&tss64;
    for (size_t i = 0; i < sizeof(struct tss64)/sizeof(uint64_t); i++)
        tss[i] = 0;

    tss64.rsp0 = rsp0;

    const size_t ist_stack_size = 4096;
    void* ist1 = kzalloc(ist_stack_size);
    void* ist2 = kzalloc(ist_stack_size);
    void* ist3 = kzalloc(ist_stack_size);

    if (ist1)
        tss64.ist1 = (uint64_t)ist1 + ist_stack_size;
    if (ist2)
        tss64.ist2 = (uint64_t)ist2 + ist_stack_size;
    if (ist3)
        tss64.ist3 = (uint64_t)ist3 + ist_stack_size;

    tss64_load(GDT64_TSS_SELECTOR);
}

#else
#include "process.h"
#include "config.h"
#include "status.h"
#include "task/task.h"
#include "memory/memory.h"
#include "string/string.h"
#include "fs/file.h"
#include "memory/heap/kheap.h"
#include "memory/paging/paging.h"
#include "loader/formats/elfloader.h"
#include "kernel.h"

// The current process that is running
struct process* current_process = 0;

static struct process* processes[VANA_MAX_PROCESSES] = {};

int process_free_process(struct process* process);

/*
 * Zero a new `struct process`. This helper is used when loading a program
 * into an empty slot so all fields start in a known state.
 */
static void process_init(struct process* process)
{
    memset(process, 0, sizeof(struct process));
}

/*
 * Return the process whose task currently owns the CPU.
 */
struct process* process_current()
{
    return current_process;
}

/*
 * Fetch a process structure by its identifier. Returns NULL if the slot is
 * unused or the identifier is out of range.
 */
struct process* process_get(int process_id)
{
    if (process_id < 0 || process_id >= VANA_MAX_PROCESSES)
    {
        return NULL;
    }

    return processes[process_id];
}

/*
 * Mark `process` as the one currently scheduled. This affects helper
 * routines that query `process_current()` but actual context switching is
 * handled by `task_switch` on the task itself.
 */
int process_switch(struct process* process)
{
    current_process = process;
    return 0;
}

/*
 * Helper used by `process_malloc` to locate a free slot in the allocation
 * table that tracks memory owned by the process.
 */
static int process_find_free_allocation_index(struct process* process)
{
    int res = -ENOMEM;
    for (int i = 0; i < VANA_MAX_PROGRAM_ALLOCATIONS; i++)
    {
        if (process->allocations[i].ptr == 0)
        {
            res = i;
            break;
        }
    }

    return res;
}

/*
 * Allocate `size` bytes on behalf of a process. The kernel heap supplies
 * the backing memory which is then mapped into the process's address
 * space. Each allocation is tracked so it can be freed when the process
 * exits.
 */
void* process_malloc(struct process* process, size_t size)
{
    void* ptr = kzalloc(size);
    if (!ptr)
    {
        goto out_err;
    }

    int index = process_find_free_allocation_index(process);
    if (index < 0)
    {
        goto out_err;
    }

    int res = paging_map_to(process->task->page_directory, ptr, ptr, paging_align_address(ptr+size), PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
    if (res < 0)
    {
        goto out_err;
    }

    process->allocations[index].ptr = ptr;
    process->allocations[index].size = size;
    return ptr;

out_err:
    if(ptr)
    {
        kfree(ptr);
    }
    return 0;
}

/*
 * Check whether a pointer belongs to the given process's allocation list.
 * This guards against a process attempting to free memory it does not own.
 */
static bool process_is_process_pointer(struct process* process, void* ptr)
{
    for (int i = 0; i < VANA_MAX_PROGRAM_ALLOCATIONS; i++)
    {
        if (process->allocations[i].ptr == ptr)
            return true;
    }

    return false;
}

/*
 * Remove an allocation record from the process's table. This is called when
 * memory is freed so that subsequent checks do not report it as still owned
 * by the process.
 */
static void process_allocation_unjoin(struct process* process, void* ptr)
{
    for (int i = 0; i < VANA_MAX_PROGRAM_ALLOCATIONS; i++)
    {
        if (process->allocations[i].ptr == ptr)
        {
            process->allocations[i].ptr = 0x00;
            process->allocations[i].size = 0;
        }
    }
}

/*
 * Look up an allocation by virtual address within a process. Returns the
 * allocation record or NULL if the address does not correspond to memory
 * owned by the process.
 */
static struct process_allocation* process_get_allocation_by_addr(struct process* process, void* addr)
{
    for (int i = 0; i < VANA_MAX_PROGRAM_ALLOCATIONS; i++)
    {
        if (process->allocations[i].ptr == addr)
            return &process->allocations[i];
    }

    return 0;
}


/*
 * Free all heap allocations that a process has made during its lifetime.
 * This prevents memory leaks when a process exits and ensures no stale
 * mappings remain in its address space.
 */
int process_terminate_allocations(struct process* process)
{
    for (int i = 0; i < VANA_MAX_PROGRAM_ALLOCATIONS; i++)
    {
        if (process->allocations[i].ptr)
        {
            process_free(process, process->allocations[i].ptr);
        }
    }

    return 0;
}

/*
 * Release memory allocated for a raw binary executable that was loaded
 * directly into kernel space.
 */
int process_free_binary_data(struct process* process)
{
    if (process->ptr)
    {
        kfree(process->ptr);
    }
    return 0;
}

/*
 * Close and discard an ELF file previously loaded for a process. The ELF
 * loader allocated this handle and paging mappings, so drop our reference
 * here.
 */
int process_free_elf_data(struct process* process)
{
    if (process->elf_file)
    {
        elf_close(process->elf_file);
    }

    return 0;
}
/* Dispatch to the correct cleanup routine based on the program filetype. */
int process_free_program_data(struct process* process)
{
    int res = 0;
    switch(process->filetype)
    {
        case PROCESS_FILETYPE_BINARY:
            res = process_free_binary_data(process);
        break;

        case PROCESS_FILETYPE_ELF:
            res = process_free_elf_data(process);
        break;

        default:
            res = -EINVARG;
    }
    return res;
}

/*
 * Switch to the first available process in the global table. Used when the
 * current process exits so the scheduler always has something to run.
 */
void process_switch_to_any()
{
    for (int i = 0; i < VANA_MAX_PROCESSES; i++)
    {
        if (processes[i])
        {
            process_switch(processes[i]);
            return;
        }
    }


    panic("No processes to switch too\n");
}

/*
 * Remove a process from the global process table. If it was the currently
 * running process the scheduler switches to another valid entry.
 */
static void process_unlink(struct process* process)
{
    processes[process->id] = 0x00;

    if (current_process == process)
    {
        process_switch_to_any();
    }
}

/*
 * Release all resources owned by a process including heap allocations,
 * program image, stack and task structure. Called when a process exits to
 * ensure all memory mappings are removed and no kernel memory leaks occur.
 */
int process_free_process(struct process* process)
{
    int res = 0;
    process_terminate_allocations(process);
    process_free_program_data(process);

    // Free the process stack memory.
    if (process->stack)
    {    
        kfree(process->stack);
        process->stack = NULL;
    }
    // Free the task
    if (process->task)
    {
        task_free(process->task);
        process->task = NULL;
    }

    kfree(process);

out:
    return res;
}

/*
 * Unlink a process and free it so another task can run. This is the public
 * entry point used by the syscall layer when a program calls `exit`.
 */
int process_terminate(struct process* process)
{
    // Unlink the process from the process array.
    process_unlink(process);

    int res = process_free_process(process);
    if (res < 0)
    {
        goto out;
    }


out:
    return res;
}

/*
 * Retrieve the argument vector for a running process. Used by the user
 * space C library to implement `main(int argc, char** argv)`.
 */
void process_get_arguments(struct process* process, int* argc, char*** argv)
{
    *argc = process->arguments.argc;
    *argv = process->arguments.argv;
}

/*
 * Count the number of arguments in a linked list of `command_argument`
 * structures.
 */
int process_count_command_arguments(struct command_argument* root_argument)
{
    struct command_argument* current = root_argument;
    int i = 0;
    while(current)
    {
        i++;
        current = current->next;
    }

    return i;
}


/*
 * Copy a linked list of command arguments into the process's address space
 * so the user program can access them on start-up.
 */
int process_inject_arguments(struct process* process, struct command_argument* root_argument)
{
    int res = 0;
    struct command_argument* current = root_argument;
    int i = 0;
    int argc = process_count_command_arguments(root_argument);
    if (argc == 0)
    {
        res = -EIO;
        goto out;
    }

    char **argv = process_malloc(process, sizeof(const char*) * argc);
    if (!argv)
    {
        res = -ENOMEM;
        goto out;
    }


    while(current)
    {
        char* argument_str = process_malloc(process, sizeof(current->argument));
        if (!argument_str)
        {
            res = -ENOMEM;
            goto out;
        }

        strncpy(argument_str, current->argument, sizeof(current->argument));
        argv[i] = argument_str;
        current = current->next;
        i++;
    }

    process->arguments.argc = argc;
    process->arguments.argv = argv;
out:
    return res;
}
/*
 * Free memory previously allocated with `process_malloc`. The mapping is
 * removed from the task's page directory and the allocation record is
 * cleared.
 */
void process_free(struct process* process, void* ptr)
{
    // Unlink the pages from the process for the given address
    struct process_allocation* allocation = process_get_allocation_by_addr(process, ptr);
    if (!allocation)
    {
        // Oops its not our pointer.
        return;
    }

    int res = paging_map_to(process->task->page_directory, allocation->ptr, allocation->ptr, paging_align_address(allocation->ptr+allocation->size), 0x00);
    if (res < 0)
    {
        return;
    }

    // Unjoin the allocation
    process_allocation_unjoin(process, ptr);

    // We can now free the memory.
    kfree(ptr);
}

/*
 * Load a raw binary executable from disk into kernel memory. The program
 * image is stored in `process->ptr` so it can later be mapped into the
 * task's address space.
 */
static int process_load_binary(const char* filename, struct process* process)
{
    void* program_data_ptr = 0x00;
    int res = 0;
    int fd = fopen(filename, "r");
    if (!fd)
    {
        res = -EIO;
        goto out;
    }

    struct file_stat stat;
    res = fstat(fd, &stat);
    if (res != VANA_ALL_OK)
    {
        goto out;
    }

    program_data_ptr = kzalloc(stat.filesize);
    if (!program_data_ptr)
    {
        res = -ENOMEM;
        goto out;
    }

    if (fread(program_data_ptr, stat.filesize, 1, fd) != 1)
    {
        res = -EIO;
        goto out;
    }

    process->filetype = PROCESS_FILETYPE_BINARY;
    process->ptr = program_data_ptr;
    process->size = stat.filesize;

out:
    if (res < 0)
    {
        if (program_data_ptr)
        {
            kfree(program_data_ptr);
        }
    }
    fclose(fd);
    return res;
}

/*
 * Parse an ELF executable and attach the resulting handle to the process.
 * The loader validates the file and keeps the program headers so segments
 * can be mapped with correct permissions.
 */
static int process_load_elf(const char* filename, struct process* process)
{
    int res = 0;
    struct elf_file* elf_file = 0;
    res = elf_load(filename, &elf_file);
    if (ISERR(res))
    {
        goto out;
    }

    process->filetype = PROCESS_FILETYPE_ELF;
    process->elf_file = elf_file;
out:
    return res;
}
/*
 * Try loading an ELF file first and fall back to a raw binary if needed.
 * This allows the same API to support both executable formats.
 */
static int process_load_data(const char* filename, struct process* process)
{
    int res = 0;
    res = process_load_elf(filename, process);
    if (res == -EINFORMAT)
    {
        res = process_load_binary(filename, process);
    }

    return res;
}

/*
 * Map the raw binary image into user space at the standard program load
 * address. The memory was already allocated by `process_load_binary`.
 */
int process_map_binary(struct process* process)
{
    int res = 0;
    paging_map_to(process->task->page_directory, (void*) VANA_PROGRAM_VIRTUAL_ADDRESS, process->ptr, paging_align_address(process->ptr + process->size), PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL | PAGING_IS_WRITEABLE);
    return res;
}

/* Map all loadable ELF segments for the process. */
/*
 * Map all loadable ELF segments for the process using the information
 * parsed by the loader. Each program header is mapped with permissions
 * derived from its flags so read-only sections remain protected.
 */
static int process_map_elf(struct process* process)
{
    int res = 0;

    struct elf_file* elf_file = process->elf_file;
    struct elf_header* header = elf_header(elf_file);
    struct elf32_phdr* phdrs = elf_pheader(header);
    for (int i = 0; i < header->e_phnum; i++)
    {
        struct elf32_phdr* phdr = &phdrs[i];
        void* phdr_phys_address = elf_phdr_phys_address(elf_file, phdr);
        int flags = PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL;
        if (phdr->p_flags & PF_W)
        {
            flags |= PAGING_IS_WRITEABLE;
        }
        res = paging_map_to(process->task->page_directory, paging_align_to_lower_page((void*)phdr->p_vaddr), paging_align_to_lower_page(phdr_phys_address), paging_align_address(phdr_phys_address+phdr->p_memsz), flags);
        if (ISERR(res))
        {
            break;
        }
    }
    return res;
}
/*
 * Map the program image and stack for a newly created process. This
 * dispatches to the ELF or binary helpers based on file type and then
 * installs the user stack.
 */
int process_map_memory(struct process* process)
{
    int res = 0;

    switch(process->filetype)
    {
        case PROCESS_FILETYPE_ELF:
            res = process_map_elf(process);
        break;

        case PROCESS_FILETYPE_BINARY:
            res = process_map_binary(process);
        break;

        default:
            panic("process_map_memory: Invalid filetype\n");
    }

    if (res < 0)
    {
        goto out;
    }

    // Finally map the stack
    paging_map_to(process->task->page_directory, (void*)VANA_PROGRAM_VIRTUAL_STACK_ADDRESS_END, process->stack, paging_align_address(process->stack+VANA_USER_PROGRAM_STACK_SIZE), PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL | PAGING_IS_WRITEABLE);
out:
    return res;
}

/* Return the index of an unused entry in the global process array. */
int process_get_free_slot()
{
    for (int i = 0; i < VANA_MAX_PROCESSES; i++)
    {
        if (processes[i] == 0)
            return i;
    }

    return -EISTKN;
}

/*
 * High-level helper used by the shell to load a program. It simply
 * allocates a free slot and forwards to `process_load_for_slot`.
 */
int process_load(const char* filename, struct process** process)
{
    int res = 0;
    int process_slot = process_get_free_slot();
    if (process_slot < 0)
    {
        res = -EISTKN;
        goto out;
    }

    res = process_load_for_slot(filename, process, process_slot);
out:
    return res;
}

/*
 * Convenience wrapper that loads a program and immediately switches to it.
 * Used by the shell when launching a new foreground task.
 */
int process_load_switch(const char* filename, struct process** process)
{
    int res = process_load(filename, process);
    if (res == 0)
    {
        process_switch(*process);
    }

    return res;
}

/*
 * Load a program into a specific slot within the global process table.
 * This function drives the loader: it allocates the `struct process`,
 * loads the executable (ELF or raw binary) and then creates the initial
 * task with its own page directory. Finally `process_map_memory` is called
 * to map the program image and stack so the scheduler can run it.
 */
int process_load_for_slot(const char* filename, struct process** process, int process_slot)
{
    int res = 0;
    struct process* _process;

    if (process_get(process_slot) != 0)
    {
        res = -EISTKN;
        goto out;
    }

    _process = kzalloc(sizeof(struct process));
    if (!_process)
    {
        res = -ENOMEM;
        goto out;
    }

    process_init(_process);
    res = process_load_data(filename, _process);
    if (res < 0)
    {
        goto out;
    }

    _process->stack = kzalloc(VANA_USER_PROGRAM_STACK_SIZE);
    if (!_process->stack)
    {
        res = -ENOMEM;
        goto out;
    }

    strncpy(_process->filename, filename, sizeof(_process->filename));
    _process->id = process_slot;

    // Create a task
    _process->task = task_new(_process);
    if (ERROR_I(_process->task) == 0)
    {
        res = ERROR_I(_process->task);

        // Task is NULL due to error code being returned in task_new.
        _process->task = NULL;
        goto out;
    }


    res = process_map_memory(_process);
    if (res < 0)
    {
        goto out;
    }

    *process = _process;

    // Add the process to the array
    processes[process_slot] = _process;

out:
    if (ISERR(res))
    {
        if (_process)
        {
            process_free_process(_process);
            _process = NULL;
            *process = NULL;
        }

       // Free the process data
    }
    return res;
}
#endif
