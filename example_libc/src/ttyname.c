/*
 * BSD 2-Clause License
 *
 * Purpose: Implements ttyname functions using /dev/tty links on BSD.
 */
#include "unistd.h"
#include "errno.h"
#include "string.h"
#include <unistd.h>
#include <stdio.h>

#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
int ttyname_r(int fd, char *buf, size_t len)
{
    if (!buf || len == 0)
        return EINVAL;
    char path[32];
    snprintf(path, sizeof(path), "/dev/fd/%d", fd);
    ssize_t n = readlink(path, buf, len - 1);
    if (n < 0)
        return errno;
    buf[n] = '\0';
    if (strcmp(buf, "/dev/tty") == 0) {
        n = readlink("/dev/tty", buf, len - 1);
        if (n >= 0)
            buf[n] = '\0';
    }
    return 0;
}
#else
extern int host_ttyname_r(int, char *, size_t) __asm("ttyname_r");
int ttyname_r(int fd, char *buf, size_t len)
{
    return host_ttyname_r(fd, buf, len);
}
#endif

char *ttyname(int fd)
{
    static __thread char namebuf[128];
    if (ttyname_r(fd, namebuf, sizeof(namebuf)) != 0)
        return NULL;
    return namebuf;
}

