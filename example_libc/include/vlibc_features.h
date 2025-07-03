/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for optional syscall feature macros.
 */
#ifndef VLIBC_FEATURES_H
#define VLIBC_FEATURES_H

/*
 * Feature detection macros for optional syscalls.
 * The build system defines VLIBC_HAVE_ACCEPT4, VLIBC_HAVE_PIPE2 and
 * VLIBC_HAVE_DUP3 when the respective system calls are available.
 * If a macro is not provided, it defaults to 0 so the code can fall
 * back to portable implementations.
 */

#ifndef VLIBC_HAVE_ACCEPT4
#define VLIBC_HAVE_ACCEPT4 0
#endif

#ifndef VLIBC_HAVE_PIPE2
#define VLIBC_HAVE_PIPE2 0
#endif

#ifndef VLIBC_HAVE_DUP3
#define VLIBC_HAVE_DUP3 0
#endif

#endif /* VLIBC_FEATURES_H */
