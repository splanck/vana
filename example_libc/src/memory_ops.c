/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the memory_ops functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "string.h"

/*
 * vmemset() - simple byte-wise memory initialisation used by vlibc internals.
 */
void *vmemset(void *s, int c, size_t n)
{
    unsigned char *p = s;
    while (n--)
        *p++ = (unsigned char)c;
    return s;
}

/*
 * vmemcpy() - copy n bytes from src to dest without optimisations.
 */
void *vmemcpy(void *dest, const void *src, size_t n)
{
    unsigned char *d = dest;
    const unsigned char *s = src;
    while (n--)
        *d++ = *s++;
    return dest;
}

/*
 * vmemmove() - overlapping-safe version of vmemcpy().
 */
void *vmemmove(void *dest, const void *src, size_t n)
{
    unsigned char *d = dest;
    const unsigned char *s = src;
    if (d == s || n == 0)
        return dest;
    if (d < s) {
        while (n--)
            *d++ = *s++;
    } else {
        d += n;
        s += n;
        while (n--)
            *--d = *--s;
    }
    return dest;
}

/*
 * vmemcmp() - compare two memory blocks byte by byte.
 */
int vmemcmp(const void *s1, const void *s2, size_t n)
{
    const unsigned char *p1 = s1;
    const unsigned char *p2 = s2;
    while (n--) {
        if (*p1 != *p2)
            return *p1 - *p2;
        p1++; p2++;
    }
    return 0;
}

/*
 * memset() - wrapper around vmemset().
 */
void *memset(void *s, int c, size_t n)
{
    return vmemset(s, c, n);
}

/*
 * memcpy() - wrapper around vmemcpy().
 */
void *memcpy(void *dest, const void *src, size_t n)
{
    return vmemcpy(dest, src, n);
}

/*
 * memmove() - wrapper around vmemmove().
 */
void *memmove(void *dest, const void *src, size_t n)
{
    return vmemmove(dest, src, n);
}

/*
 * memcmp() - wrapper around vmemcmp().
 */
int memcmp(const void *s1, const void *s2, size_t n)
{
    return vmemcmp(s1, s2, n);
}
