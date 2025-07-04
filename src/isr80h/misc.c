#include "misc.h"
#include "task/task.h"

/*
 * Miscellaneous system call stubs used for demonstration and testing.
 *
 * These handlers show how simple syscalls can be implemented. They pull
 * arguments directly from the calling task's stack using
 * `task_get_stack_item` and return their results in `eax` via the
 * standard `isr80h` convention.
 */

/*
 * Example command 0 handler.
 *
 * Fetches the first two integers from the user stack, adds them together
 * and returns the sum. User programs call this via `vana_sum` in the
 * standard library which issues command 0 through `int 0x80`.
 */
void* isr80h_command0_sum(struct interrupt_frame* frame)
{
    (void)frame;
    int a = (int)task_get_stack_item(task_current(), 0);
    int b = (int)task_get_stack_item(task_current(), 1);
    int result = a + b;
    return (void*) result;
}
