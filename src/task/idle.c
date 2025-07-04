#include "idle.h"
#include "task.h"
#include "process.h"
#include "kernel.h"
#include "memory/paging/paging.h"
#include "memory/memory.h"
#include <stdint.h>

extern struct paging_4gb_chunk* kernel_chunk;
extern struct task* current_task;
extern struct task* task_head;
extern struct task* task_tail;

static struct task idle_task_struct;
static struct process idle_process_struct;

static void idle_loop()
{
    for (;;)
    {
        asm volatile("hlt");
    }
}

int idle_task_init()
{
    memset(&idle_process_struct, 0, sizeof(idle_process_struct));
    memset(&idle_task_struct, 0, sizeof(idle_task_struct));

    idle_process_struct.task = &idle_task_struct;
    idle_task_struct.process = &idle_process_struct;
    idle_task_struct.page_directory = kernel_chunk;
    idle_task_struct.registers.ip = (uint32_t)idle_loop;
    idle_task_struct.registers.ss = KERNEL_DATA_SELECTOR;
    idle_task_struct.registers.cs = KERNEL_CODE_SELECTOR;
    idle_task_struct.registers.esp = 0;

    // Insert into task list
    if (!task_current())
    {
        task_head = &idle_task_struct;
        task_tail = &idle_task_struct;
        current_task = &idle_task_struct;
    }
    else
    {
        task_tail->next = &idle_task_struct;
        idle_task_struct.prev = task_tail;
        task_tail = &idle_task_struct;
    }

    // Make idle the current process by default
    process_switch(&idle_process_struct);

    return 0;
}

struct task* idle_task_get()
{
    return &idle_task_struct;
}

struct process* idle_process_get()
{
    return &idle_process_struct;
}
