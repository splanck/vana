#ifndef VANA_SYSCALL_H
#define VANA_SYSCALL_H

/*
 * int 0x80 calling convention:
 *   EAX = system call number
 *   EBX, ECX, EDX, ESI, EDI, EBP = arguments
 *   Return value in EAX (negative value indicates error)
 */

enum vana_errno
{
    VANA_EPERM = -1,
    VANA_ENOENT = -2,
    VANA_EIO = -5,
    VANA_ENOMEM = -12,
    VANA_EINVAL = -22,
    VANA_ENOSYS = -38
};

enum vana_syscall
{
    VANA_SYS_EXIT = 1,
    VANA_SYS_WRITE = 2,
    VANA_SYS_READ = 3,
    VANA_SYS_OPEN = 4,
    VANA_SYS_CLOSE = 5,
    VANA_SYS_LSEEK = 6,
    VANA_SYS_BRK = 12
};

#endif
