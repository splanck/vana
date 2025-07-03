/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the arc4random functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "stdlib.h"
#include "io.h"
#include "syscall.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <errno.h>

/*
 * Simple arc4random implementation. Entropy is gathered from the best
 * available source at runtime. getrandom(2) is attempted first when the
 * kernel supports it. If that fails we fall back to reading
 * /dev/urandom. Should both mechanisms be unavailable the buffer is
 * filled using the rand() PRNG which provides very little entropy.
 */

static ssize_t try_urandom(void *buf, size_t len)
{
#ifdef O_CLOEXEC
    int flags = O_RDONLY | O_CLOEXEC;
#else
    int flags = O_RDONLY;
#endif
    int fd = open("/dev/urandom", flags);
    if (fd < 0)
        return -1;
    size_t off = 0;
    while (off < len) {
        ssize_t r = read(fd, (unsigned char *)buf + off, len - off);
        if (r <= 0) {
            if (r < 0 && errno == EINTR)
                continue;
            break;
        }
        off += (size_t)r;
    }
    close(fd);
    return off == len ? (ssize_t)off : -1;
}

static ssize_t try_getrandom(void *buf, size_t len)
{
#ifdef SYS_getrandom
    size_t off = 0;
    while (off < len) {
        long r = vlibc_syscall(SYS_getrandom, (long)((unsigned char *)buf + off),
                               len - off, 0, 0, 0);
        if (r < 0) {
            if (r == -EINTR)
                continue;
            if (r == -ENOSYS)
                return -1;
            break;
        }
        off += (size_t)r;
    }
    return off == len ? (ssize_t)off : -1;
#else
    (void)buf;
    (void)len;
    return -1;
#endif
}

/*
 * fill_prng() - fallback PRNG using rand().
 * Used when no kernel entropy source is available to
 * populate the buffer with pseudo-random bytes.
 */
static void fill_prng(void *buf, size_t len)
{
    unsigned char *p = buf;
    for (size_t i = 0; i < len; i++)
        p[i] = (unsigned char)(rand() & 0xff);
}

/*
 * arc4random_buf() - fill buf with len bytes of random data.
 * Attempts getrandom() or /dev/urandom before falling back
 * to the internal PRNG implemented by fill_prng().
 */
void arc4random_buf(void *buf, size_t len)
{
    if (len == 0)
        return;

#ifdef SYS_getrandom
    static int have_getrandom = -1;
    if (have_getrandom == -1) {
        long r = vlibc_syscall(SYS_getrandom, 0, 0, 0, 0, 0);
        if (r >= 0 || r == -EINVAL)
            have_getrandom = 1;
        else if (r == -ENOSYS)
            have_getrandom = 0;
        else
            have_getrandom = 0;
    }
    if (have_getrandom == 1 &&
        try_getrandom(buf, len) == (ssize_t)len)
        return;
#endif

    if (try_urandom(buf, len) == (ssize_t)len)
        return;

    fill_prng(buf, len);
}

/*
 * arc4random() - return a random 32-bit value.
 * Thin wrapper around arc4random_buf().
 */
unsigned int arc4random(void)
{
    unsigned int v = 0;
    arc4random_buf(&v, sizeof(v));
    return v;
}

/*
 * arc4random_uniform() - unbiased bounded random number.
 * Generates a value in the range [0, upper_bound) without
 * modulo bias by discarding out of range results.
 */
unsigned int arc4random_uniform(unsigned int upper_bound)
{
    unsigned int r, min;

    if (upper_bound < 2)
        return 0;

    min = -upper_bound % upper_bound;
    for (;;) {
        r = arc4random();
        if (r >= min)
            break;
    }
    return r % upper_bound;
}

int rand_r(unsigned *state)
{
    *state = *state * 1103515245 + 12345;
    return (int)((*state >> 16) & 0x7fff);
}
