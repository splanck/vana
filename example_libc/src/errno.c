/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the errno functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "errno.h"

/* each thread maintains its own errno value */
__thread int errno;

/*
 * __errno_location() - return address of the thread-local errno
 * variable so callers can modify it directly.
 */
int *__errno_location(void)
{
    return &errno;
}
