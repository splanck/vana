/*
 * BSD 2-Clause License
 *
 * Purpose: Implements the ucontext functions for vlibc.
 */

#include "ucontext.h"
#include "stdlib.h"
#include "string.h"
#include "errno.h"
#include "process.h"
#include <stdarg.h>

#ifndef VLIBC_HAS_SYS_UCONTEXT

#if defined(__x86_64__)

/* trampoline for newly created contexts */
static void __attribute__((noreturn)) ctx_trampoline(void)
{
    ucontext_t *uc;
    __asm__ volatile("mov %%r12,%0" : "=r"(uc));

    void (*fn)(void) = uc->uc_func;
    long *a = uc->uc_args;
    switch (uc->uc_argc) {
    case 0: ((void (*)(void))fn)(); break;
    case 1: ((void (*)(long))fn)(a[0]); break;
    case 2: ((void (*)(long,long))fn)(a[0], a[1]); break;
    case 3: ((void (*)(long,long,long))fn)(a[0], a[1], a[2]); break;
    case 4: ((void (*)(long,long,long,long))fn)(a[0], a[1], a[2], a[3]); break;
    case 5: ((void (*)(long,long,long,long,long))fn)(a[0], a[1], a[2], a[3], a[4]); break;
    default:
        ((void (*)(long,long,long,long,long,long))fn)(a[0], a[1], a[2], a[3], a[4], a[5]);
        break;
    }

    if (uc->uc_link)
        setcontext(uc->uc_link);
    exit(0);
}

int getcontext(ucontext_t *ucp)
{
    if (!ucp) {
        errno = EINVAL;
        return -1;
    }
    sigprocmask(SIG_SETMASK, NULL, &ucp->uc_sigmask);
    __asm__ volatile(
        "mov %%rbx,%0\n"
        "mov %%rbp,%1\n"
        "mov %%r12,%2\n"
        "mov %%r13,%3\n"
        "mov %%r14,%4\n"
        "mov %%r15,%5\n"
        "mov %%rsp,%6\n"
        "lea 1f(%%rip),%%rax\n"
        "mov %%rax,%7\n"
        "xor %%eax,%%eax\n"
        "1:"
        : "=m"(ucp->rbx), "=m"(ucp->rbp), "=m"(ucp->r12), "=m"(ucp->r13),
          "=m"(ucp->r14), "=m"(ucp->r15), "=m"(ucp->rsp), "=m"(ucp->rip)
        :
        : "rax", "memory");
    return 0;
}

int setcontext(const ucontext_t *ucp)
{
    if (!ucp) {
        errno = EINVAL;
        return -1;
    }
    sigprocmask(SIG_SETMASK, &ucp->uc_sigmask, NULL);
    __asm__ volatile(
        "mov %0,%%rbx\n"
        "mov %1,%%rbp\n"
        "mov %2,%%r12\n"
        "mov %3,%%r13\n"
        "mov %4,%%r14\n"
        "mov %5,%%r15\n"
        "mov %6,%%rsp\n"
        "jmp *%7\n"
        :
        : "m"(ucp->rbx), "m"(ucp->rbp), "m"(ucp->r12), "m"(ucp->r13),
          "m"(ucp->r14), "m"(ucp->r15), "m"(ucp->rsp), "m"(ucp->rip)
        : "memory");
    __builtin_unreachable();
}

void makecontext(ucontext_t *ucp, void (*func)(void), int argc, ...)
{
    if (!ucp || !func)
        return;
    if (argc < 0)
        argc = 0;
    if (argc > 6)
        argc = 6;
    ucp->uc_func = func;
    ucp->uc_argc = argc;
    va_list ap;
    va_start(ap, argc);
    for (int i = 0; i < argc; i++)
        ucp->uc_args[i] = va_arg(ap, long);
    va_end(ap);

    char *sp = (char *)ucp->uc_stack.ss_sp + ucp->uc_stack.ss_size;
    sp = (char *)((uintptr_t)sp & ~15UL);
    sp -= 8; /* keep 16-byte alignment */
    ucp->rsp = (unsigned long)sp;
    ucp->rip = (unsigned long)ctx_trampoline;
    ucp->r12 = (unsigned long)ucp;
}

int swapcontext(ucontext_t *oucp, const ucontext_t *ucp)
{
    if (getcontext(oucp) == 0)
        return setcontext(ucp);
    return 0;
}

#else /* !__x86_64__ */

#include <setjmp.h>
#include <signal.h>

/* Fallback implementation using system jmp_buf. This relies on the
 * underlying C library's layout and mirrors the previous implementation
 * used on glibc systems. */

int getcontext(ucontext_t *ucp)
{
    if (!ucp) {
        errno = EINVAL;
        return -1;
    }
    sigprocmask(SIG_SETMASK, NULL, &ucp->uc_sigmask);
    if (setjmp(ucp->__jmpbuf) != 0)
        return 0;
    return 0;
}

int setcontext(const ucontext_t *ucp)
{
    if (!ucp) {
        errno = EINVAL;
        return -1;
    }
    sigprocmask(SIG_SETMASK, &ucp->uc_sigmask, NULL);
    longjmp(ucp->__jmpbuf, 1);
    __builtin_unreachable();
}

void makecontext(ucontext_t *ucp, void (*func)(void), int argc, ...)
{
    if (!ucp || !func)
        return;
    if (argc < 0)
        argc = 0;
    if (argc > 6)
        argc = 6;
    ucp->uc_func = func;
    ucp->uc_argc = argc;
    va_list ap;
    va_start(ap, argc);
    for (int i = 0; i < argc; i++)
        ucp->uc_args[i] = va_arg(ap, long);
    va_end(ap);

    char *sp = (char *)ucp->uc_stack.ss_sp + ucp->uc_stack.ss_size;
    sp = (char *)((uintptr_t)sp & ~15UL);
    sp -= 8;
    setjmp(ucp->__jmpbuf);
    struct __jmp_buf_tag *jb = (struct __jmp_buf_tag *)ucp->__jmpbuf;
    jb->__jmpbuf[6] = (long)sp;             /* rsp */
    jb->__jmpbuf[7] = (long)ctx_trampoline; /* rip */
    jb->__jmpbuf[2] = (long)ucp;            /* r12 */
}

int swapcontext(ucontext_t *oucp, const ucontext_t *ucp)
{
    if (getcontext(oucp) == 0)
        return setcontext(ucp);
    return 0;
}

#endif /* __x86_64__ */

#endif /* VLIBC_HAS_SYS_UCONTEXT */
