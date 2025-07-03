/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the rand functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "stdlib.h"

static unsigned long rand_state = 1;

int rand(void)
{
    rand_state = rand_state * 1103515245 + 12345;
    return (int)((rand_state >> 16) & 0x7fff);
}

void srand(unsigned seed)
{
    rand_state = seed;
}

