/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the getline functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "stdio.h"
#include "memory.h"
#include "errno.h"

/*
 * getdelim() - read from a stream until the delimiter or EOF is encountered.
 * The buffer pointed to by lineptr is resized as needed.  Returns the number
 * of bytes read or -1 on error.
 */
ssize_t getdelim(char **lineptr, size_t *n, int delim, FILE *stream)
{
    if (!lineptr || !n || !stream) {
        errno = EINVAL;
        return -1;
    }
    if (!*lineptr || *n == 0) {
        *n = *n ? *n : 128;
        *lineptr = malloc(*n);
        if (!*lineptr) {
            errno = ENOMEM;
            return -1;
        }
    }
    size_t pos = 0;
    int c;
    while ((c = fgetc(stream)) != -1) {
        if (pos + 1 >= *n) {
            size_t new_size = *n * 2;
            char *tmp = realloc(*lineptr, new_size);
            if (!tmp) {
                errno = ENOMEM;
                return -1;
            }
            *lineptr = tmp;
            *n = new_size;
        }
        (*lineptr)[pos++] = (char)c;
        if (c == delim)
            break;
    }
    if (c == -1 && pos == 0)
        return -1;
    (*lineptr)[pos] = '\0';
    return (ssize_t)pos;
}

/*
 * getline() - convenience wrapper around getdelim() that uses a newline as
 * the delimiter.
 */
ssize_t getline(char **lineptr, size_t *n, FILE *stream)
{
    return getdelim(lineptr, n, '\n', stream);
}
