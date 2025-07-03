/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that the copyright notice and
 * this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements fopencookie for vlibc.
 */

#include "stdio.h"
#include "memory.h"
#include "string.h"

FILE *fopencookie(void *cookie, const char *mode,
                  cookie_io_functions_t functions)
{
    (void)mode; /* modes currently ignored */
    FILE *f = malloc(sizeof(FILE));
    if (!f)
        return NULL;
    memset(f, 0, sizeof(FILE));
    atomic_flag_clear(&f->lock);
    f->fd = -1;
    f->is_cookie = 1;
    f->cookie = cookie;
    f->cookie_read = functions.read;
    f->cookie_write = functions.write;
    f->cookie_seek = functions.seek;
    f->cookie_close = functions.close;
    return f;
}

struct fun_bridge {
    const void *cookie;
    int (*readfn)(void *, char *, int);
    int (*writefn)(void *, const char *, int);
    fpos_t (*seekfn)(void *, fpos_t, int);
    int (*closefn)(void *);
};

/* read wrapper passed to fopencookie */
static ssize_t fun_read(void *c, char *buf, size_t n)
{
    struct fun_bridge *b = c;
    if (!b->readfn)
        return 0;
    int r = b->readfn((void *)b->cookie, buf, (int)n);
    return r < 0 ? -1 : r;
}

/* write wrapper passed to fopencookie */
static ssize_t fun_write(void *c, const char *buf, size_t n)
{
    struct fun_bridge *b = c;
    if (!b->writefn)
        return (ssize_t)n;
    int r = b->writefn((void *)b->cookie, buf, (int)n);
    return r < 0 ? -1 : r;
}

/* seek wrapper passed to fopencookie */
static int fun_seek(void *c, off_t *off, int whence)
{
    struct fun_bridge *b = c;
    if (!b->seekfn)
        return -1;
    fpos_t r = b->seekfn((void *)b->cookie, (fpos_t)*off, whence);
    if (r == (fpos_t)-1)
        return -1;
    *off = (off_t)r;
    return 0;
}

/* close wrapper passed to fopencookie */
static int fun_close(void *c)
{
    struct fun_bridge *b = c;
    int r = 0;
    if (b->closefn)
        r = b->closefn((void *)b->cookie);
    free(b);
    return r;
}

/* BSD funopen implementation using cookie_io_functions_t */
FILE *funopen(const void *cookie,
              int (*readfn)(void *, char *, int),
              int (*writefn)(void *, const char *, int),
              fpos_t (*seekfn)(void *, fpos_t, int),
              int (*closefn)(void *))
{
    struct fun_bridge *b = malloc(sizeof(*b));
    if (!b)
        return NULL;
    b->cookie = cookie;
    b->readfn = readfn;
    b->writefn = writefn;
    b->seekfn = seekfn;
    b->closefn = closefn;

    cookie_io_functions_t io = {
        .read = fun_read,
        .write = fun_write,
        .seek = fun_seek,
        .close = fun_close,
    };
    return fopencookie(b, "", io);
}
