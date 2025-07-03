/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for visual encoding helpers.
 */
#ifndef VIS_H
#define VIS_H

#include <stddef.h>

/* encoding flags */
#define VIS_OCTAL   0x01
#define VIS_CSTYLE  0x02
#define VIS_SP      0x04
#define VIS_TAB     0x08
#define VIS_NL      0x10
#define VIS_WHITE   (VIS_SP | VIS_TAB | VIS_NL)
#define VIS_SAFE    0x20
#define VIS_NOSLASH 0x40

/* states for unvis */
#define UNVIS_END        0x100
#define UNVIS_VALID      1
#define UNVIS_NOCHAR     0
#define UNVIS_SYNBAD    -1

int vis(char *dst, int c, int flag, int nextc);
int nvis(char *dst, size_t dlen, int c, int flag, int nextc);
int unvis(char *cp, int c, int *state, int flag);

#endif /* VIS_H */
