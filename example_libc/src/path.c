/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the path functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "stdlib.h"
#include "string.h"
#include "memory.h"

/*
 * basename() - return the last path component of the provided string.  A
 * newly allocated string is returned and must be freed by the caller.
 */
char *basename(const char *path)
{
    if (!path || !*path)
        return strdup(".");

    const char *start = path;
    const char *end = path + strlen(path);
    while (end > start && *(end - 1) == '/')
        --end;
    if (end == start)
        return strdup("/");

    const char *p = end;
    while (p > start && *(p - 1) != '/')
        --p;

    size_t len = end - p;
    char *out = malloc(len + 1);
    if (!out)
        return NULL;
    memcpy(out, p, len);
    out[len] = '\0';
    return out;
}

/*
 * dirname() - return the directory portion of a path.  The result is
 * allocated with malloc and should be freed by the caller.
 */
char *dirname(const char *path)
{
    if (!path || !*path)
        return strdup(".");

    const char *start = path;
    const char *end = path + strlen(path);
    while (end > start && *(end - 1) == '/')
        --end;
    if (end == start)
        return strdup("/");

    const char *p = end;
    while (p > start && *(p - 1) != '/')
        --p;
    while (p > start && *(p - 1) == '/')
        --p;

    size_t len = p - start;
    if (len == 0)
        return strdup(".");

    char *out = malloc(len + 1);
    if (!out)
        return NULL;
    memcpy(out, start, len);
    out[len] = '\0';
    return out;
}

