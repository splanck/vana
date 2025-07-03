/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the memstream functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "stdio.h"
#include "memory.h"
#include "string.h"
#include "errno.h"

/*
 * open_memstream() - create a memory backed FILE stream that dynamically
 * grows to hold all written data.  The resulting buffer address and size
 * are returned via the provided pointers when the stream is closed.
 */
FILE *open_memstream(char **bufp, size_t *sizep)
{
    if (!bufp || !sizep) {
        errno = EINVAL;
        return NULL;
    }
    FILE *f = malloc(sizeof(FILE));
    if (!f)
        return NULL;
    memset(f, 0, sizeof(FILE));
    atomic_flag_clear(&f->lock);
    f->fd = -1;
    f->is_mem = 1;
    f->writable = 1;
    f->mem_bufp = (void **)bufp;
    f->mem_sizep = sizep;
    f->bufsize = 128;
    f->buf = malloc(f->bufsize);
    if (!f->buf) {
        free(f);
        errno = ENOMEM;
        return NULL;
    }
    f->buf_owned = 1;
    *bufp = NULL;
    *sizep = 0;
    return f;
}

/*
 * open_wmemstream() - wide character variant of open_memstream().  The
 * stream stores wchar_t elements and returns the buffer and element count
 * on close.
 */
FILE *open_wmemstream(wchar_t **bufp, size_t *sizep)
{
    if (!bufp || !sizep) {
        errno = EINVAL;
        return NULL;
    }
    FILE *f = malloc(sizeof(FILE));
    if (!f)
        return NULL;
    memset(f, 0, sizeof(FILE));
    atomic_flag_clear(&f->lock);
    f->fd = -1;
    f->is_mem = 1;
    f->is_wmem = 1;
    f->writable = 1;
    f->mem_bufp = (void **)bufp;
    f->mem_sizep = sizep;
    f->bufsize = 128 * sizeof(wchar_t);
    f->buf = malloc(f->bufsize);
    if (!f->buf) {
        free(f);
        errno = ENOMEM;
        return NULL;
    }
    f->buf_owned = 1;
    *bufp = NULL;
    *sizep = 0;
    return f;
}

/*
 * fmemopen() - open a memory area as a FILE stream.  If buf is NULL a
 * new buffer of the specified size is allocated.  The mode string controls
 * readability and writability similar to fopen().
 */
FILE *fmemopen(void *buf, size_t size, const char *mode)
{
    if (size == 0) {
        errno = EINVAL;
        return NULL;
    }
    FILE *f = malloc(sizeof(FILE));
    if (!f)
        return NULL;
    memset(f, 0, sizeof(FILE));
    atomic_flag_clear(&f->lock);
    f->fd = -1;
    f->is_mem = 1;

    if (!mode)
        mode = "r";

    if (strcmp(mode, "r") == 0) {
        f->readable = 1;
    } else if (strcmp(mode, "r+") == 0) {
        f->readable = 1;
        f->writable = 1;
    } else if (strcmp(mode, "w") == 0) {
        f->writable = 1;
    } else if (strcmp(mode, "w+") == 0) {
        f->readable = 1;
        f->writable = 1;
    } else if (strcmp(mode, "a") == 0) {
        f->writable = 1;
        f->append = 1;
    } else if (strcmp(mode, "a+") == 0) {
        f->readable = 1;
        f->writable = 1;
        f->append = 1;
    } else {
        free(f);
        errno = EINVAL;
        return NULL;
    }
    f->buf = buf ? (unsigned char *)buf : malloc(size);
    if (!f->buf) {
        free(f);
        errno = ENOMEM;
        return NULL;
    }
    f->bufsize = size;
    f->buflen = (mode && mode[0] == 'w' && (!mode[1] || mode[1] != '+')) ? 0 : size;
    f->bufpos = (mode && mode[0] == 'a') ? f->buflen : 0;
    f->buf_owned = buf ? 0 : 1;
    return f;
}
