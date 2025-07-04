# Task Scheduler and Process Management

This document outlines how the cooperative scheduler in `src/task/` creates
new tasks, performs context switches and manages processes.

## Task creation (`task.c/h`)

The `struct task` structure records the CPU state and memory mapping of a
running thread:

```c
struct task
{
    struct paging_4gb_chunk* page_directory;
    struct registers registers;
    struct process* process;
    struct task* next;
    struct task* prev;
};
```

`task_new(process*)` allocates a task and calls `task_init()` which sets up a
blank 4 GB page directory using `paging_new_4gb()` and initial register values.
The entry point defaults to `VANA_PROGRAM_VIRTUAL_ADDRESS` or the ELF entry
address when the process was loaded from an ELF file.  Segment selectors are
initialised to the user data and code selectors and `ESP` starts at
`VANA_PROGRAM_VIRTUAL_STACK_ADDRESS_START`.

Created tasks are inserted into a doubly linked list headed by
`task_head`. `current_task` always points at the running task and
`task_get_next()` returns the next list element or wraps back to the head.

## Context switching (`task.asm`)

A context switch is triggered by calling `task_switch()` which updates
`current_task` and loads the new task's page directory with `paging_switch()`.
Execution then jumps to the assembly routine `task_return(struct registers*)`
which restores the saved registers and executes `iretd`:

```asm
push dword [ebx+44] ; SS
push dword [ebx+40] ; ESP
mov eax, [ebx+36]
or eax, 0x200
push eax            ; EFLAGS
push dword [ebx+32] ; CS
push dword [ebx+28] ; EIP
...
iretd
```

`task_run_first_ever_task()` uses this mechanism to start the very first user
process after initialization.  `task_next()` performs a simple round‑robin step
by selecting `task_get_next()` and returning to the new task.
`task_current_save_state()` copies the interrupt frame into a task when a
kernel interrupt occurs so the scheduler can later resume it.

The `tss_load()` helper in `tss.asm` loads the Task State Segment selector so
interrupts use a known kernel stack.

## Process management (`process.c/h`)

A `struct process` owns the resources of a program and its main task:

```c
struct process
{
    uint16_t id;
    char filename[VANA_MAX_PATH];
    struct task* task;
    struct process_allocation allocations[VANA_MAX_PROGRAM_ALLOCATIONS];
    PROCESS_FILETYPE filetype;
    union { void* ptr; struct elf_file* elf_file; };
    void* stack;
    uint32_t size;
    struct keyboard_buffer { char buffer[VANA_KEYBOARD_BUFFER_SIZE];
                             int tail; int head; } keyboard;
    struct process_arguments arguments;
};
```

`process_load_for_slot()` allocates the structure, loads the program (ELF or raw
binary) and maps it into the new task's address space via
`process_map_memory()`.  The created process is placed in the global process
array and can be made current with `process_switch()` or
`process_load_switch()` which loads a program and immediately switches to it.

When a process terminates (`process_terminate()`), its allocations are freed and
its task removed from the list.  Exceptions or an explicit `exit` system call
end the process and `task_next()` is invoked to run the next available task.

Overall the scheduler cooperates with the process loader to maintain a list of
ready tasks and switches between them whenever a task finishes or yields to the
kernel.
