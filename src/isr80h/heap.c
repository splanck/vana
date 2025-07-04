#include "heap.h"
#include "task/task.h"
#include "task/process.h"
#include <stddef.h>

/*
 * System call implementations for dynamic memory management.
 * Command 4 allocates from the calling process heap and command 5
 * releases the pointer back to that heap.
 */

/*
 * Allocate `size` bytes for the current process.
 * The size argument is taken from the user stack.
 * Returns a user-space pointer to the new block or NULL on failure.
 */
void* isr80h_command4_malloc(struct interrupt_frame* frame)
{
    (void)frame;
    size_t size = (int)task_get_stack_item(task_current(), 0);
    return process_malloc(task_current()->process, size);
}

/*
 * Free a pointer previously allocated with command 4.
 * The address to release is fetched from the user stack.
 */
void* isr80h_command5_free(struct interrupt_frame* frame)
{
    (void)frame;
    void* ptr_to_free = task_get_stack_item(task_current(), 0);
    process_free(task_current()->process, ptr_to_free);
    return 0;
}
