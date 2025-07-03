/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for floating-point environment control.
 */
#ifndef FENV_H
#define FENV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/* Exception macros */
#define FE_INEXACT    0x01
#define FE_DIVBYZERO  0x04
#define FE_UNDERFLOW  0x08
#define FE_OVERFLOW   0x10
#define FE_INVALID    0x20
#define FE_ALL_EXCEPT (FE_INEXACT | FE_DIVBYZERO | \
                       FE_UNDERFLOW | FE_OVERFLOW | FE_INVALID)

/* Rounding mode macros */
#define FE_TONEAREST  0
#define FE_DOWNWARD   0x400
#define FE_UPWARD     0x800
#define FE_TOWARDZERO 0xc00

typedef unsigned int fenv_t;
typedef unsigned int fexcept_t;

int feclearexcept(int excepts);
int fegetexceptflag(fexcept_t *flagp, int excepts);
int fesetexceptflag(const fexcept_t *flagp, int excepts);
int feraiseexcept(int excepts);
int fetestexcept(int excepts);
int fegetround(void);
int fesetround(int mode);
int fegetenv(fenv_t *envp);
int feholdexcept(fenv_t *envp);
int fesetenv(const fenv_t *envp);
int feupdateenv(const fenv_t *envp);

#ifdef __cplusplus
}
#endif

#endif /* FENV_H */
