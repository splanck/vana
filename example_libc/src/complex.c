/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that the copyright notice and
 * this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements basic complex math routines for vlibc.
 */

#include "complex.h"

/* Absolute value (magnitude) of a complex number. */
double cabs(double_complex z)
{
    return hypot(z.real, z.imag);
}

/* Phase angle of a complex number. */
double carg(double_complex z)
{
    return atan2(z.imag, z.real);
}

/* Complex exponential. */
double_complex cexp(double_complex z)
{
    double ex = exp(z.real);
    double_complex r;
    r.real = ex * cos(z.imag);
    r.imag = ex * sin(z.imag);
    return r;
}

/* Complex cosine. */
double_complex ccos(double_complex z)
{
    double_complex r;
    r.real = cos(z.real) * cosh(z.imag);
    r.imag = -sin(z.real) * sinh(z.imag);
    return r;
}

/* Complex sine. */
double_complex csin(double_complex z)
{
    double_complex r;
    r.real = sin(z.real) * cosh(z.imag);
    r.imag = cos(z.real) * sinh(z.imag);
    return r;
}

