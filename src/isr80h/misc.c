#include "misc.h"
#include "task/task.h"

int isr80h_command0_sum(struct interrupt_frame* frame)
{
    (void)frame;
    int a = (int)task_get_stack_item(task_current(), 0);
    int b = (int)task_get_stack_item(task_current(), 1);
    int result = a + b;
    return result;
}
