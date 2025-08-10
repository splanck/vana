#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

typedef uint64_t (*syscall_handler_t)(uint64_t, uint64_t, uint64_t,
                                      uint64_t, uint64_t, uint64_t);

void syscall_register(int num, syscall_handler_t handler);
uint64_t syscall_dispatch(uint64_t num,
                          uint64_t arg1, uint64_t arg2, uint64_t arg3,
                          uint64_t arg4, uint64_t arg5, uint64_t arg6);
void syscall_init(void);

#endif
