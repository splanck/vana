/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the math functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "math.h"

/* Value of pi used by the trig approximations */
static const double PI = 3.14159265358979323846;

/*
 * fabs_approx() - lightweight absolute value used internally.
 */
static double fabs_approx(double x)
{
    return x < 0 ? -x : x;
}

/*
 * sin() - compute sine using a Taylor series approximation.
 * The input is reduced to the range [-pi, pi] for better accuracy.
 */
double sin(double x)
{
    while (x > PI)
        x -= 2.0 * PI;
    while (x < -PI)
        x += 2.0 * PI;

    double term = x;
    double sum = x;
    for (int i = 1; i < 10; ++i) {
        term *= -x * x / ((2 * i) * (2 * i + 1));
        sum += term;
    }
    return sum;
}

/*
 * cos() - compute cosine using a Taylor series approximation.
 * The angle is reduced similarly to sin().
 */
double cos(double x)
{
    while (x > PI)
        x -= 2.0 * PI;
    while (x < -PI)
        x += 2.0 * PI;

    double term = 1.0;
    double sum = 1.0;
    for (int i = 1; i < 10; ++i) {
        term *= -x * x / ((2 * i - 1) * (2 * i));
        sum += term;
    }
    return sum;
}

/*
 * tan() - compute tangent as sin(x)/cos(x).
 * Returns 0 when cosine is zero to avoid division by zero.
 */
double tan(double x)
{
    double s = sin(x);
    double c = cos(x);
    return c == 0.0 ? 0.0 : s / c;
}

/*
 * sqrt() - compute square root using Newton's method.
 * Returns 0 for non-positive inputs.
 */
double sqrt(double x)
{
    if (x <= 0.0)
        return 0.0;

    double guess = x;
    for (int i = 0; i < 20; ++i)
        guess = 0.5 * (guess + x / guess);
    return guess;
}

/*
 * log_approx() - natural logarithm approximation using
 * an arithmetic series expansion.
 */
static double log_approx(double x)
{
    if (x <= 0.0)
        return 0.0;
    double y = (x - 1.0) / (x + 1.0);
    double y2 = y * y;
    double term = y;
    double sum = 0.0;
    for (int i = 1; i < 10; i += 2) {
        sum += term / i;
        term *= y2;
    }
    return 2.0 * sum;
}

/*
 * exp_approx() - approximate exponential function using
 * a truncated power series.
 */
static double exp_approx(double x)
{
    double term = 1.0;
    double sum = 1.0;
    for (int i = 1; i <= 10; ++i) {
        term *= x / i;
        sum += term;
    }
    return sum;
}

/*
 * pow() - raise a base to an exponent using repeated
 * multiplication or the exp/log identities for fractional
 * exponents.
 */
double pow(double base, double exp)
{
    if (exp == 0.0)
        return 1.0;
    if (exp < 0.0)
        return 1.0 / pow(base, -exp);

    int iexp = (int)exp;
    if (fabs_approx(exp - iexp) < 1e-9) {
        double result = 1.0;
        for (int i = 0; i < iexp; ++i)
            result *= base;
        return result;
    }

    return exp_approx(exp * log_approx(base));
}

/* Return the natural logarithm using log_approx. */
double log(double x)
{
    return log_approx(x);
}

/* Return e raised to the given power using exp_approx. */
double exp(double x)
{
    return exp_approx(x);
}

/* Round toward negative infinity. */
double floor(double x)
{
    long i = (long)x;
    if (x < 0.0 && x != (double)i)
        --i;
    return (double)i;
}

/* Round toward positive infinity. */
double ceil(double x)
{
    long i = (long)x;
    if (x > 0.0 && x != (double)i)
        ++i;
    return (double)i;
}

/* Standard absolute value wrapper. */
double fabs(double x)
{
    return x < 0.0 ? -x : x;
}
