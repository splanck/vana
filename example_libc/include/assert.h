/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for runtime assertion checks.
 */
#ifndef ASSERT_H
#define ASSERT_H

#include <stdio.h>
#include <stdlib.h>

#ifndef NDEBUG
#define assert(expr) ((expr) ? (void)0 : (fprintf(stderr, "assertion failed: %s (%s:%d)\n", #expr, __FILE__, __LINE__), abort()))
#else
#define assert(expr) ((void)0)
#endif

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define static_assert _Static_assert
#endif

#endif /* ASSERT_H */
