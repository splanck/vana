#include "syscall.h"
#include <stdint.h>
#include <stddef.h>

int write(int fd, const void *buf, size_t len)
{
    return vana_syscall3(VANA_SYS_WRITE, fd, (int32_t)buf, (int32_t)len);
}

int read(int fd, void *buf, size_t len)
{
    return vana_syscall3(VANA_SYS_READ, fd, (int32_t)buf, (int32_t)len);
}

int close(int fd)
{
    return vana_syscall1(VANA_SYS_CLOSE, fd);
}

int open(const char *path, int flags)
{
    return vana_syscall3(VANA_SYS_OPEN, (int32_t)path, flags, 0);
}

int lseek(int fd, int offset, int whence)
{
    return vana_syscall3(VANA_SYS_LSEEK, fd, offset, whence);
}

void _exit(int status)
{
    vana_syscall1(VANA_SYS_EXIT, status);
    for(;;) {}
}

void *sbrk(intptr_t inc)
{
    return (void*)vana_syscall1(VANA_SYS_BRK, (int32_t)inc);
}

