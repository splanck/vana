/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the termios functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "termios.h"

extern int host_tcgetattr(int, struct termios *) __asm__("tcgetattr");
extern int host_tcsetattr(int, int, const struct termios *) __asm__("tcsetattr");
extern void host_cfmakeraw(struct termios *) __asm__("cfmakeraw");
extern int host_tcdrain(int) __asm__("tcdrain");
extern int host_tcflow(int, int) __asm__("tcflow");
extern int host_tcflush(int, int) __asm__("tcflush");
extern int host_tcsendbreak(int, int) __asm__("tcsendbreak");
extern int host_cfsetispeed(struct termios *, speed_t) __asm__("cfsetispeed");
extern int host_cfsetospeed(struct termios *, speed_t) __asm__("cfsetospeed");
extern speed_t host_cfgetispeed(const struct termios *) __asm__("cfgetispeed");
extern speed_t host_cfgetospeed(const struct termios *) __asm__("cfgetospeed");

int tcgetattr(int fd, struct termios *t)
{
    return host_tcgetattr(fd, t);
}

int tcsetattr(int fd, int act, const struct termios *t)
{
    return host_tcsetattr(fd, act, t);
}

void cfmakeraw(struct termios *t)
{
    host_cfmakeraw(t);
}

int tcdrain(int fd)
{
    return host_tcdrain(fd);
}

int tcflow(int fd, int act)
{
    return host_tcflow(fd, act);
}

int tcflush(int fd, int qs)
{
    return host_tcflush(fd, qs);
}

int tcsendbreak(int fd, int dur)
{
    return host_tcsendbreak(fd, dur);
}

int cfsetispeed(struct termios *t, speed_t sp)
{
    return host_cfsetispeed(t, sp);
}

int cfsetospeed(struct termios *t, speed_t sp)
{
    return host_cfsetospeed(t, sp);
}

speed_t cfgetispeed(const struct termios *t)
{
    return host_cfgetispeed(t);
}

speed_t cfgetospeed(const struct termios *t)
{
    return host_cfgetospeed(t);
}
