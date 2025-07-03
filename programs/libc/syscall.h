#ifndef VANA_LIBC_SYSCALL_H
#define VANA_LIBC_SYSCALL_H

#include <stdint.h>
#include <vana/syscall.h>

static inline int32_t vana_syscall0(int32_t n)
{
    int32_t ret;
    asm volatile("int $0x80"
                 : "=a"(ret)
                 : "a"(n)
                 : "memory");
    return ret;
}

static inline int32_t vana_syscall1(int32_t n, int32_t a1)
{
    int32_t ret;
    asm volatile("int $0x80"
                 : "=a"(ret)
                 : "a"(n), "b"(a1)
                 : "memory");
    return ret;
}

static inline int32_t vana_syscall2(int32_t n, int32_t a1, int32_t a2)
{
    int32_t ret;
    asm volatile("int $0x80"
                 : "=a"(ret)
                 : "a"(n), "b"(a1), "c"(a2)
                 : "memory");
    return ret;
}

static inline int32_t vana_syscall3(int32_t n, int32_t a1, int32_t a2, int32_t a3)
{
    int32_t ret;
    asm volatile("int $0x80"
                 : "=a"(ret)
                 : "a"(n), "b"(a1), "c"(a2), "d"(a3)
                 : "memory");
    return ret;
}

static inline int32_t vana_syscall4(int32_t n, int32_t a1, int32_t a2, int32_t a3, int32_t a4)
{
    int32_t ret;
    asm volatile("int $0x80"
                 : "=a"(ret)
                 : "a"(n), "b"(a1), "c"(a2), "d"(a3), "S"(a4)
                 : "memory");
    return ret;
}

static inline int32_t vana_syscall5(int32_t n, int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5)
{
    int32_t ret;
    asm volatile("int $0x80"
                 : "=a"(ret)
                 : "a"(n), "b"(a1), "c"(a2), "d"(a3), "S"(a4), "D"(a5)
                 : "memory");
    return ret;
}

static inline int32_t vana_syscall6(int32_t n, int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6)
{
    int32_t ret;
    register int32_t arg6 asm("ebp") = a6;
    asm volatile("int $0x80"
                 : "=a"(ret)
                 : "a"(n), "b"(a1), "c"(a2), "d"(a3), "S"(a4), "D"(a5), "r"(arg6)
                 : "memory");
    return ret;
}

#endif /* VANA_LIBC_SYSCALL_H */
