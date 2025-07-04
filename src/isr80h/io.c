
/*
 * I/O syscall handlers.
 *
 * These functions implement the basic terminal interface exposed through
 * the `isr80h` system call mechanism. User programs invoke them via
 * commands 1-3 to print strings, read keyboard input and write single
 * characters.
 */

#include "io.h"
#include "task/task.h"
#include "keyboard/keyboard.h"
#include "kernel.h"

/*
 * Copy a user string from the calling task and print it to the terminal.
 * The user pointer is expected to be the first item on the stack.
 */
void* isr80h_command1_print(struct interrupt_frame* frame)
{
    (void)frame;
    void* user_space_msg_buffer = task_get_stack_item(task_current(), 0);
    char buf[1024];
    copy_string_from_task(task_current(), user_space_msg_buffer, buf, sizeof(buf));
    print(buf);
    return 0;
}

/*
 * Return the next character from the keyboard queue.
 * The result is placed in the low byte of eax.
 */
void* isr80h_command2_getkey(struct interrupt_frame* frame)
{
    (void)frame;
    char c = keyboard_pop();
    return (void*)((int)c);
}

/*
 * Write a single character to the terminal using the default color.
 * The character is passed as the first parameter on the user stack.
 */
void* isr80h_command3_putchar(struct interrupt_frame* frame)
{
    char c = (char)(int)task_get_stack_item(task_current(), 0);
    terminal_writechar(c, 15);
    return 0;
}
