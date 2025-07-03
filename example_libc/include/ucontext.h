/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for user context manipulation.
 */
#ifndef UCONTEXT_H
#define UCONTEXT_H

#include "signal.h"
#include <stdarg.h>

#ifdef VLIBC_HAS_SYS_UCONTEXT
#include_next <ucontext.h>
#else
#include <setjmp.h>

typedef struct ucontext {
    struct ucontext *uc_link;
    stack_t          uc_stack;
    sigset_t         uc_sigmask;
    void (*uc_func)(void);
    int              uc_argc;
    long             uc_args[6];
#if defined(__x86_64__)
    unsigned long    rbx, rbp, r12, r13, r14, r15;
    unsigned long    rsp, rip;
#else
    jmp_buf          __jmpbuf;
#endif
} ucontext_t;
#endif
#ifndef VLIBC_HAS_SYS_UCONTEXT
int getcontext(ucontext_t *ucp);
int setcontext(const ucontext_t *ucp);
void makecontext(ucontext_t *ucp, void (*func)(void), int argc, ...);
int swapcontext(ucontext_t *oucp, const ucontext_t *ucp);
#endif

#endif /* UCONTEXT_H */
