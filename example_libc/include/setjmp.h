/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for non-local jump helpers.
 */
#ifndef SETJMP_H
#define SETJMP_H

#if defined(__has_include)
#  if __has_include("/usr/include/setjmp.h")
#    include "/usr/include/setjmp.h"
#  endif
#endif

#ifndef sigjmp_buf
typedef jmp_buf sigjmp_buf;
#endif

#ifndef sigsetjmp
int sigsetjmp(sigjmp_buf env, int save);
#endif
#ifndef siglongjmp
void siglongjmp(sigjmp_buf env, int val);
#endif

#endif /* SETJMP_H */
