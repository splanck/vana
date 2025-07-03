/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the getcwd functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "stdlib.h"
#include "errno.h"
#include <sys/syscall.h>
#include <unistd.h>
#include "syscall.h"

/*
 * getcwd() - return the absolute path of the current working directory.
 * When buf is NULL a buffer is allocated that must later be freed by the
 * caller.  Returns NULL on failure with errno set.
 */
char *getcwd(char *buf, size_t size)
{
    if (buf) {
        if (size == 0) {
            errno = EINVAL;
            return NULL;
        }
#ifdef SYS_getcwd
        long ret = vlibc_syscall(SYS_getcwd, (long)buf, size, 0, 0, 0, 0);
        if (ret < 0) {
            errno = -ret;
            return NULL;
        }
        return (char *)buf;
#else
        (void)buf; (void)size; errno = ENOSYS; return NULL;
#endif
    }

    size_t cap = size ? size : 256;
    char *out = malloc(cap);
    if (!out)
        return NULL;

    for (;;) {
#ifdef SYS_getcwd
        long ret = vlibc_syscall(SYS_getcwd, (long)out, cap, 0, 0, 0, 0);
        if (ret >= 0)
            return out;
        if (-ret != ERANGE) {
            errno = -ret;
            break;
        }
#else
        (void)out; (void)cap; errno = ENOSYS; break;
#endif
        size_t new_cap = cap * 2;
        char *tmp = realloc(out, new_cap);
        if (!tmp) {
            free(out);
            errno = ENOMEM;
            return NULL;
        }
        out = tmp;
        cap = new_cap;
    }
    free(out);
    return NULL;
}
