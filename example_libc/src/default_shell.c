/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the default_shell functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "stdlib.h"

#ifndef VLIBC_SHELL_PATH
#define VLIBC_SHELL_PATH "/bin/sh"
#endif

/*
 * Return the path of the shell used by vlibc. If the environment
 * variable VLIBC_SHELL is set and non-empty it is used, otherwise
 * the compile time default is returned.
 */
const char *vlibc_default_shell(void)
{
    const char *s = getenv("VLIBC_SHELL");
    if (s && *s)
        return s;
    return VLIBC_SHELL_PATH;
}
