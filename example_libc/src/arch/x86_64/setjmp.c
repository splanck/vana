/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the setjmp functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "setjmp.h"

/*
 * setjmp() - save callee-saved registers and stack state.
 * Stores the program counter along with rbp, rbx and r12-r15
 * into the provided jmp_buf. Returns 0 when saving the state
 * and the value supplied to longjmp() when restoring.
 */
int setjmp(jmp_buf env)
{
    int r;
    __asm__ volatile(
        "mov %%rbx,0(%1)\n"
        "mov %%rbp,8(%1)\n"
        "mov %%r12,16(%1)\n"
        "mov %%r13,24(%1)\n"
        "mov %%r14,32(%1)\n"
        "mov %%r15,40(%1)\n"
        "mov %%rsp,48(%1)\n"
        "mov $1f,%%rax\n"
        "mov %%rax,56(%1)\n"
        "xor %%eax,%%eax\n"
        "1: mov %%eax,%0\n"
        : "=r"(r)
        : "r"(env)
        : "memory","rax"
    );
    return r;
}

/*
 * longjmp() - resume execution from a saved setjmp context.
 * Restores registers and jumps to the stored program counter.
 * A zero value is translated to one before returning.
 */
void longjmp(jmp_buf env, int val)
{
    if (val == 0)
        val = 1;
    __asm__ volatile(
        "mov %0,%%rax\n"
        "mov 0(%1),%%rbx\n"
        "mov 8(%1),%%rbp\n"
        "mov 16(%1),%%r12\n"
        "mov 24(%1),%%r13\n"
        "mov 32(%1),%%r14\n"
        "mov 40(%1),%%r15\n"
        "mov 48(%1),%%rsp\n"
        "mov 56(%1),%%rdx\n"
        "jmp *%%rdx\n"
        :
        : "r"((long)val), "r"(env)
        : "rax","rbx","rdx","r12","r13","r14","r15","memory"
    );
    __builtin_unreachable();
}
