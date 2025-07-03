/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the atexit functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "stdlib.h"

#define ATEXIT_MAX 32

static void (*handlers[ATEXIT_MAX])(void);
static int handler_count = 0;

static void (*quick_handlers[ATEXIT_MAX])(void);
static int quick_count = 0;

int atexit(void (*fn)(void))
{
    if (!fn || handler_count >= ATEXIT_MAX)
        return -1;
    handlers[handler_count++] = fn;
    return 0;
}

int at_quick_exit(void (*func)(void))
{
    if (!func || quick_count >= ATEXIT_MAX)
        return -1;
    quick_handlers[quick_count++] = func;
    return 0;
}

/*
 * __run_atexit() - invoke regular atexit handlers in reverse registration
 * order.  Used by the exit() implementation to run cleanups.
 */
void __run_atexit(void)
{
    for (int i = handler_count - 1; i >= 0; --i) {
        if (handlers[i])
            handlers[i]();
    }
}

/*
 * __run_quick_exit() - invoke quick exit handlers in reverse order.
 * Called internally by quick_exit() and _Exit() wrappers.
 */
static void __run_quick_exit(void)
{
    for (int i = quick_count - 1; i >= 0; --i) {
        if (quick_handlers[i])
            quick_handlers[i]();
    }
}

extern void _exit(int);

/*
 * quick_exit() - run registered quick exit handlers and then terminate the
 * process without flushing stdio buffers.
 */
void quick_exit(int status)
{
    __run_quick_exit();
    _exit(status);
}
