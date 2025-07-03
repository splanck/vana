/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the init functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "vlibc.h"
#include "stdio.h"
#include "memory.h"
#include "string.h"

void vlibc_init(void)
{
    stdin = malloc(sizeof(FILE));
    if (stdin) {
        memset(stdin, 0, sizeof(FILE));
        atomic_flag_clear(&stdin->lock);
        stdin->fd = 0;
    }

    stdout = malloc(sizeof(FILE));
    if (stdout) {
        memset(stdout, 0, sizeof(FILE));
        atomic_flag_clear(&stdout->lock);
        stdout->fd = 1;
    }

    stderr = malloc(sizeof(FILE));
    if (stderr) {
        memset(stderr, 0, sizeof(FILE));
        atomic_flag_clear(&stderr->lock);
        stderr->fd = 2;
    }
}
