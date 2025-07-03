/*
 * BSD 2-Clause License
 *
 * Purpose: Portable implementation of ftok using stat and
 *          the inode/device algorithm.
 */

#include "sys/ipc.h"
#include "sys/stat.h"
#include "stdint.h"
#include "errno.h"

key_t ftok(const char *pathname, int proj_id)
{
    struct stat st;
    if (stat(pathname, &st) != 0)
        return (key_t)-1;

    return (key_t)((st.st_ino & 0xffff) |
                   ((st.st_dev & 0xff) << 16) |
                   ((proj_id & 0xff) << 24));
}
