/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for basic math routines.
 */
#ifndef MATH_H
#define MATH_H

double sin(double x);
double cos(double x);
double tan(double x);
double sqrt(double x);
double pow(double base, double exp);
double log(double x);
double log10(double x);
double exp(double x);
double floor(double x);
double ceil(double x);
double fabs(double x);
double atan2(double y, double x);
double sinh(double x);
double cosh(double x);
double tanh(double x);

double hypot(double x, double y);
double round(double x);
double trunc(double x);

double fmod(double x, double y);
float fabsf(float x);
double ldexp(double x, int exp);
double log2(double x);
double fmin(double a, double b);
double fmax(double a, double b);
double copysign(double x, double y);

/* Floating-point classification */
#if defined(__has_builtin)
#  if !defined(isnan) && __has_builtin(__builtin_isnan)
#    define isnan(x) __builtin_isnan(x)
#  endif
#  if !defined(isinf)
#    if __has_builtin(__builtin_isinf_sign)
#      define isinf(x) __builtin_isinf_sign(x)
#    elif __has_builtin(__builtin_isinf)
#      define isinf(x) __builtin_isinf(x)
#    endif
#  endif
#  if !defined(isfinite) && __has_builtin(__builtin_isfinite)
#    define isfinite(x) __builtin_isfinite(x)
#  endif
#endif

#ifndef isnan
#define isnan(x) ((x) != (x))
#endif

#ifndef isinf
#define isinf(x) ((x) == (1.0/0.0) || (x) == (-1.0/0.0))
#endif

#ifndef isfinite
#define isfinite(x) (!isnan(x) && !isinf(x))
#endif

#endif /* MATH_H */
