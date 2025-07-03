/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the syscall functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#define _GNU_SOURCE
#include <stdarg.h>
#include <unistd.h>
#include <sys/syscall.h>
#include "errno.h"

long vlibc_syscall(long number, ...)
{
    va_list ap;
    va_start(ap, number);
    unsigned long a1 = va_arg(ap, unsigned long);
    unsigned long a2 = va_arg(ap, unsigned long);
    unsigned long a3 = va_arg(ap, unsigned long);
    unsigned long a4 = va_arg(ap, unsigned long);
    unsigned long a5 = va_arg(ap, unsigned long);
    unsigned long a6 = va_arg(ap, unsigned long);
    va_end(ap);

    register long r10 __asm__("r10") = (long)a4;
    register long r8  __asm__("r8")  = (long)a5;
    register long r9  __asm__("r9")  = (long)a6;
    long ret;
    __asm__ volatile ("syscall"
                      : "=a" (ret)
                      : "a" (number), "D" (a1), "S" (a2), "d" (a3),
                        "r" (r10), "r" (r8), "r" (r9)
                      : "rcx", "r11", "memory");
    return ret;
}
