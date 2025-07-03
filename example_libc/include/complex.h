/*
 * BSD 2-Clause License
 *
 * Purpose: Basic complex number type and math prototypes.
 */
#ifndef COMPLEX_H
#define COMPLEX_H

#include "math.h"

typedef struct {
    double real;
    double imag;
} double_complex;

/* Return the magnitude of a complex value. */
double cabs(double_complex z);
/* Return the phase angle of a complex value in radians. */
double carg(double_complex z);
/* Compute the complex exponential. */
double_complex cexp(double_complex z);
/* Compute the cosine of a complex value. */
double_complex ccos(double_complex z);
/* Compute the sine of a complex value. */
double_complex csin(double_complex z);

#endif /* COMPLEX_H */
