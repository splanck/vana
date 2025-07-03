/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the abort functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "stdlib.h"
#include "process.h"

/*
 * abort() - send SIGABRT to the current process and then exit.
 */
void abort(void)
{
    kill(getpid(), SIGABRT);
    _exit(1);
}
