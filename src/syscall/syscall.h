#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

typedef uint64_t (*syscall_handler_t)(void);

void syscall_register(int num, syscall_handler_t handler);
uint64_t syscall_dispatch(uint64_t num);
void syscall_init(void);

#endif
