/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that the copyright notice and
 * this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements sigsetjmp and siglongjmp for vlibc.
 */

#include "setjmp.h"
#include <setjmp.h>
#include "signal.h"

#ifdef setjmp
#undef setjmp
#endif
#ifdef longjmp
#undef longjmp
#endif
#ifdef sigsetjmp
#undef sigsetjmp
#endif
#ifdef siglongjmp
#undef siglongjmp
#endif

/*
 * Use the host C library's __sigsetjmp/__siglongjmp when available.  They may
 * not be exported on all systems so reference them weakly and fall back to the
 * older field-based implementation when missing.
 */

#if defined(__GNUC__)
extern int __sigsetjmp(sigjmp_buf, int) __attribute__((weak));
extern void __siglongjmp(sigjmp_buf, int) __attribute__((weak));
#else
extern int __sigsetjmp(sigjmp_buf, int);
extern void __siglongjmp(sigjmp_buf, int);
#endif

/* _longjmp from the system C library is used if __siglongjmp is absent. */
extern void _longjmp(jmp_buf, int);

int sigsetjmp(sigjmp_buf env, int save)
{
    if (&__sigsetjmp)
        return __sigsetjmp(env, save);

    if (save) {
        env[0].__mask_was_saved = 1;
        sigprocmask(SIG_BLOCK, NULL, &env[0].__saved_mask);
    } else {
        env[0].__mask_was_saved = 0;
    }
    return setjmp(env);
}

void siglongjmp(sigjmp_buf env, int val)
{
    if (&__siglongjmp) {
        __siglongjmp(env, val);
        __builtin_unreachable();
    }

    if (env[0].__mask_was_saved)
        sigprocmask(SIG_SETMASK, &env[0].__saved_mask, NULL);
    _longjmp(env, val);
}
