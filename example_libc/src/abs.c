/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that the copyright notice and
 * this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements abs, labs, llabs, div, ldiv and lldiv for vlibc.
 */

#include "stdlib.h"

/* Return the absolute value of a signed int. */
int abs(int j)
{
    unsigned int u = (unsigned int)j;
    if (j < 0)
        u = 0u - u;
    return (int)u;
}

/* Return the absolute value of a signed long. */
long labs(long j)
{
    unsigned long u = (unsigned long)j;
    if (j < 0)
        u = 0ul - u;
    return (long)u;
}

/* Return the absolute value of a signed long long. */
long long llabs(long long j)
{
    unsigned long long u = (unsigned long long)j;
    if (j < 0)
        u = 0ull - u;
    return (long long)u;
}

/* Compute quotient and remainder of numer divided by denom. */
div_t div(int numer, int denom)
{
    div_t r;
    r.quot = numer / denom;
    r.rem  = numer % denom;
    return r;
}

/* Long variant of div(). Returns quotient and remainder. */
ldiv_t ldiv(long numer, long denom)
{
    ldiv_t r;
    r.quot = numer / denom;
    r.rem  = numer % denom;
    return r;
}

/* Long long variant of div(). Returns quotient and remainder. */
lldiv_t lldiv(long long numer, long long denom)
{
    lldiv_t r;
    r.quot = numer / denom;
    r.rem  = numer % denom;
    return r;
}

