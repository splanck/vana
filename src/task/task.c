#include "task.h"
#include "kernel.h"
#include "status.h"
#include "process.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "string/string.h"
#include "memory/paging/paging.h"
#include "loader/formats/elfloader.h"
#include "idt/idt.h"

#ifdef __x86_64__
/* Assembly helper that restores registers and returns to user mode. */
extern void task_switch64(struct registers* regs);
#endif
/*
 * Pointer to the task that currently owns the CPU. The scheduler keeps
 * tasks in a doubly linked list and `current_task` always references the
 * head that is executing. Context switches update this pointer before
 * restoring the saved register state of the next task.
 */
struct task *current_task = 0;

/*
 * Head and tail of the run queue. Tasks are scheduled cooperatively in a
 * simple round-robin fashion. New tasks are appended to the tail and the
 * scheduler walks the list in order when `task_next()` is invoked.
 */
struct task *task_tail = 0;
struct task *task_head = 0;

int task_init(struct task *task, struct process *process);

/*
 * Return the task that is currently running. The scheduler updates the
 * `current_task` pointer whenever a context switch occurs so this helper
 * simply exposes that value.
 */
struct task *task_current()
{
    return current_task;
}

/*
 * Allocate and initialise a new task for the given process. New tasks are
 * inserted at the tail of the run queue so they participate in the
 * round-robin scheduling algorithm. The task starts with a fresh register
 * state and its own page directory created by `task_init()`.
 */
struct task *task_new(struct process *process)
{
    int res = 0;
    struct task *task = kzalloc(sizeof(struct task));
    if (!task)
    {
        res = -ENOMEM;
        goto out;
    }

    res = task_init(task, process);
    if (res != VANA_ALL_OK)
    {
        goto out;
    }

    if (task_head == 0)
    {
        task_head = task;
        task_tail = task;
        current_task = task;
        goto out;
    }

    task_tail->next = task;
    task->prev = task_tail;
    task_tail = task;

out:
    if (ISERR(res))
    {
        task_free(task);
        return ERROR(res);
    }

    return task;
}

/*
 * Return the next runnable task in the circular list. When the scheduler
 * reaches the end of the queue it wraps back to `task_head` so every task
 * eventually receives CPU time.
 */
struct task *task_get_next()
{
    if (!current_task->next)
    {
        return task_head;
    }

    return current_task->next;
}

/*
 * Detach a task from the scheduling list. Called when a task exits so the
 * scheduler will no longer select it. If the removed task is the current
 * one, `current_task` is advanced to the next runnable entry.
 */
static void task_list_remove(struct task *task)
{
    if (task->prev)
    {
        task->prev->next = task->next;
    }

    if (task == task_head)
    {
        task_head = task->next;
    }

    if (task == task_tail)
    {
        task_tail = task->prev;
    }

    if (task == current_task)
    {
        current_task = task_get_next();
    }
}

/*
 * Destroy a task and release its resources. The task is removed from the
 * run queue and its paging structures are freed so no stale mappings
 * remain. Called when a process exits.
 */
int task_free(struct task *task)
{
    paging_free_4gb(task->page_directory);
    task_list_remove(task);

    /* Finally free the task data structure itself */
    kfree(task);
    return 0;
}

/*
 * Select the next task in the run queue and perform a context switch to it.
 * This implements the cooperative round-robin behaviour where each task
 * voluntarily yields control and the scheduler rotates through the list.
 */
void task_next()
{
    struct task* next_task = task_get_next();
    if (!next_task)
    {
        panic("No more tasks!\n");
    }

    task_switch(next_task);
#ifdef __x86_64__
    task_switch64(&next_task->registers);
#else
    task_return(&next_task->registers);
#endif
}

/*
 * Install the given task's page directory and make it the running task.
 * The low-level assembly helper `task_return` restores the saved CPU
 * state after this call so that execution resumes in the context of the
 * new task.
 */
int task_switch(struct task *task)
{
    current_task = task;
    paging_switch(task->page_directory);
    return 0;
}

/*
 * Copy the interrupt frame into the task structure so the scheduler can
 * restore the exact CPU state later. This is invoked whenever a task
 * yields via an interrupt or syscall.
 */
void task_save_state(struct task *task, struct interrupt_frame *frame)
{
    task->registers.ip = frame->ip;
    task->registers.cs = frame->cs;
    task->registers.flags = frame->flags;
    task->registers.esp = frame->esp;
    task->registers.ss = frame->ss;
    task->registers.eax = frame->eax;
    task->registers.ebp = frame->ebp;
    task->registers.ebx = frame->ebx;
    task->registers.ecx = frame->ecx;
    task->registers.edi = frame->edi;
    task->registers.edx = frame->edx;
    task->registers.esi = frame->esi;
}
/*
 * Safely copy a string from a user task into kernel memory. The caller
 * supplies the destination buffer in kernel space and the virtual address
 * within the task. Paging is temporarily modified so the kernel can read
 * the task's memory while avoiding faults.
 */
int copy_string_from_task(struct task* task, void* virtual, void* phys, int max)
{
    if (max >= PAGING_PAGE_SIZE)
    {
        return -EINVARG;
    }

    int res = 0;
    char* tmp = kzalloc(max);
    if (!tmp)
    {
        res = -ENOMEM;
        goto out;
    }

    uint32_t* task_directory = task->page_directory->directory_entry;
    uint32_t old_entry = paging_get(task_directory, tmp);
    paging_map(task->page_directory, tmp, tmp, PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
    paging_switch(task->page_directory);
    strncpy(tmp, virtual, max);
    kernel_page();

    res = paging_set(task_directory, tmp, old_entry);
    if (res < 0)
    {
        res = -EIO;
        goto out_free;
    }

    strncpy(phys, tmp, max);

out_free:
    kfree(tmp);
out:
    return res;
}
/*
 * Save the CPU state of the currently running task. This is a thin wrapper
 * around `task_save_state` used by the interrupt stubs to snapshot the
 * registers before scheduling another task.
 */
void task_current_save_state(struct interrupt_frame *frame)
{
    if (!task_current())
    {
        panic("No current task to save\n");
    }

    struct task *task = task_current();
    task_save_state(task, frame);
}

/*
 * Switch to the page directory of the current task while keeping kernel
 * privileges. This is used when the kernel needs to access user memory.
 */
int task_page()
{
    user_registers();
    task_switch(current_task);
    return 0;
}

/* Switch the paging context to that belonging to `task`. */
int task_page_task(struct task* task)
{
    user_registers();
    paging_switch(task->page_directory);
    return 0;
}

/*
 * Start execution of the very first user task. The boot code sets up a
 * single task in the run queue and once paging and interrupts are ready
 * this function installs that task's directory and jumps to
 * `task_return` to enter user mode.
 */
void task_run_first_ever_task()
{
    if (!current_task)
    {
        panic("task_run_first_ever_task(): No current task exists!\n");
    }

    task_switch(task_head);
#ifdef __x86_64__
    task_switch64(&task_head->registers);
#else
    task_return(&task_head->registers);
#endif
}

/*
 * Initialise a task structure for a newly created process. A dedicated
 * page directory is created so the process has its own virtual address
 * space while still retaining the kernel mappings. The initial register
 * values place the task at the program entry point with a fresh user stack.
 */
int task_init(struct task *task, struct process *process)
{
    memset(task, 0, sizeof(struct task));
    /* Map the entire 4GB address space to itself to start with */
    task->page_directory = paging_new_4gb(PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
    if (!task->page_directory)
    {
        return -EIO;
    }

    task->registers.ip = VANA_PROGRAM_VIRTUAL_ADDRESS;
    if (process->filetype == PROCESS_FILETYPE_ELF)
    {
        task->registers.ip = elf_header(process->elf_file)->e_entry;
    }

    task->registers.ss = USER_DATA_SEGMENT;
    task->registers.cs = USER_CODE_SEGMENT;
    task->registers.esp = VANA_PROGRAM_VIRTUAL_STACK_ADDRESS_START;

    task->process = process;

    return 0;
}

/*
 * Utility used by the syscall layer to peek at values on a task's user
 * stack. The scheduler temporarily switches to the task's paging context
 * so the value can be read safely.
 */
void* task_get_stack_item(struct task* task, int index)
{
    void* result = 0;

    uint32_t* sp_ptr = (uint32_t*) task->registers.esp;

    // Switch to the given tasks page
    task_page_task(task);

    result = (void*) sp_ptr[index];

    // Switch back to the kernel page
    kernel_page();

    return result;
}

/*
 * Translate a user virtual address into its physical counterpart according
 * to the task's paging directory. Useful when the kernel must perform I/O
 * using buffers that reside in user space.
 */
void* task_virtual_address_to_physical(struct task* task, void* virtual_address)
{
    return paging_get_physical_address(task->page_directory->directory_entry, virtual_address);
}
