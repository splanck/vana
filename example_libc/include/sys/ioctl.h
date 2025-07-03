/*
 * BSD 2-Clause License
 *
 * Purpose: Device-specific IO control interface.
 */
#ifndef SYS_IOCTL_H
#define SYS_IOCTL_H

#include <sys/types.h>

#if defined(__has_include)
#  if __has_include("/usr/include/x86_64-linux-gnu/sys/ioctl.h")
#    include "/usr/include/x86_64-linux-gnu/sys/ioctl.h"
#  elif __has_include("/usr/include/sys/ioctl.h")
#    include "/usr/include/sys/ioctl.h"
#  endif
#endif

int ioctl(int fd, unsigned long req, ...);

#endif /* SYS_IOCTL_H */
