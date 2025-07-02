#include "process.h"
#include "task/task.h"
#include "task/process.h"
#include "string/string.h"
#include "status.h"
#include "config.h"
#include "kernel.h"

void* isr80h_command6_process_load_start(struct interrupt_frame* frame)
{
    (void)frame;
    void* filename_user_ptr = task_get_stack_item(task_current(), 0);
    char filename[VANA_MAX_PATH];
    int res = copy_string_from_task(task_current(), filename_user_ptr, filename, sizeof(filename));
    if (res < 0)
    {
        return 0;
    }

    char path[VANA_MAX_PATH];
    strcpy(path, "0:/");
    strcpy(path+3, filename);

    struct process* process = 0;
    res = process_load_switch(path, &process);
    if (res < 0)
    {
        return 0;
    }

    task_switch(process->task);
    task_return(&process->task->registers);

    return 0;
}
