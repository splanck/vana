/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms,
 * with or without modification, are permitted provided that the copyright
 * notice and this permission notice appear in all copies. This software is
 * provided "as is" without warranty.
 *
 * Purpose: Implement posix_madvise for vlibc. The function wraps madvise
 * and returns POSIX error codes instead of setting errno.
 */

#include "sys/mman.h"
#include "errno.h"
#include "unistd.h"
#include <stdint.h>

int posix_madvise(void *addr, size_t len, int advice)
{
    size_t ps = 4096;
    uintptr_t start = (uintptr_t)addr;
    uintptr_t end = start + len;

    if (end < start)
        return EINVAL;

    uintptr_t aligned_start = start & ~(uintptr_t)(ps - 1);
    uintptr_t aligned_end = (end + ps - 1) & ~(uintptr_t)(ps - 1);

    int r = madvise((void *)aligned_start,
                    aligned_end - aligned_start,
                    advice);
    return r == -1 ? errno : 0;
}
