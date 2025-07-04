#ifndef ISR80H_HEAP_H
#define ISR80H_HEAP_H

/*
 * Memory allocation system call declarations.
 * Command 4 allocates from the current process heap and command 5
 * frees the given pointer.
 */

struct interrupt_frame;

void* isr80h_command4_malloc(struct interrupt_frame* frame);
void* isr80h_command5_free(struct interrupt_frame* frame);

#endif
