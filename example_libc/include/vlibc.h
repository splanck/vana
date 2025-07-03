/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for library initialization.
 */
#ifndef VLIBC_H
#define VLIBC_H

/* Minimal entry point for the library. */
void vlibc_init(void);

/* Return default shell path used by system/popen */
const char *vlibc_default_shell(void);

#endif /* VLIBC_H */
