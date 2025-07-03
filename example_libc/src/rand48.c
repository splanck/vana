/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that the copyright notice and
 * this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the rand48 functions for vlibc. Provides wrappers and helpers used by the standard library.
 */

#include "stdlib.h"
#include <stdint.h>

/* 48-bit linear congruential generator */
static uint64_t rand48_state = 0x1234abcd330eULL;
static uint64_t rand48_mult  = 0x5deece66dULL;
static uint64_t rand48_add   = 0xbULL;
#define RAND48_MASK ((1ULL << 48) - 1)

/* Advance the linear congruential generator state. */
static uint64_t rand48_step(void)
{
    rand48_state = (rand48_state * rand48_mult + rand48_add) & RAND48_MASK;
    return rand48_state;
}

/* Convert three 16-bit words to a 48-bit integer state. */
static uint64_t arr_to_u64(const unsigned short x[3])
{
    return ((uint64_t)x[2] << 32) | ((uint64_t)x[1] << 16) | (uint64_t)x[0];
}

/* Split a 48-bit state into three 16-bit words. */
static void u64_to_arr(uint64_t v, unsigned short x[3])
{
    x[0] = (unsigned short)(v & 0xffff);
    x[1] = (unsigned short)((v >> 16) & 0xffff);
    x[2] = (unsigned short)((v >> 32) & 0xffff);
}

/* Generate a double in [0,1) using the internal state. */
double drand48(void)
{
    return rand48_step() / (double)(1ULL << 48);
}

/* Generate a double using the supplied state array and update it. */
double erand48(unsigned short x[3])
{
    uint64_t v = arr_to_u64(x);
    v = (v * rand48_mult + rand48_add) & RAND48_MASK;
    u64_to_arr(v, x);
    return v / (double)(1ULL << 48);
}

/* Return a non-negative long using the internal generator. */
long lrand48(void)
{
    return (long)(rand48_step() >> 17);
}

/* Return a non-negative long using the provided state array. */
long nrand48(unsigned short x[3])
{
    uint64_t v = arr_to_u64(x);
    v = (v * rand48_mult + rand48_add) & RAND48_MASK;
    u64_to_arr(v, x);
    return (long)(v >> 17);
}

/* Seed the internal generator with the given 32-bit value. */
void srand48(long seedval)
{
    rand48_state = ((uint64_t)seedval << 16) | 0x330eULL;
    rand48_mult  = 0x5deece66dULL;
    rand48_add   = 0xbULL;
}

/* Replace the generator state and return the old state array. */
unsigned short *seed48(unsigned short seed16v[3])
{
    static unsigned short old[3];
    u64_to_arr(rand48_state, old);
    rand48_state = arr_to_u64(seed16v);
    return old;
}

/* Set the generator parameters and state from the provided array. */
void lcong48(unsigned short param[7])
{
    rand48_state = arr_to_u64(param);
    rand48_mult  = arr_to_u64(param + 3);
    rand48_add   = param[6];
}
