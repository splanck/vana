#include "memory.h"

/*
 * Basic memory utilities used throughout the kernel. These
 * simple implementations provide the functionality of the C
 * standard library without relying on an external libc.
 */

/*
 * memset() - Fill a block of memory with a byte value.
 * @ptr:  destination memory address
 * @c:    value to write (only the low byte is used)
 * @size: number of bytes to write
 *
 * Returns the destination pointer for convenience.
 */
void* memset(void* ptr, int c, size_t size)
{
    char* c_ptr = (char*) ptr;
    /* write the byte value into each position */
    for (int i = 0; i < size; i++)
    {
        c_ptr[i] = (char) c;
    }
    return ptr;
}

/*
 * memcmp() - Compare two memory buffers.
 * @s1:   first buffer
 * @s2:   second buffer
 * @count: number of bytes to compare
 *
 * Returns 0 if the buffers are equal, a positive value if the
 * first differing byte in @s1 is greater, or a negative value if
 * it is less.
 */
int memcmp(void* s1, void* s2, int count)
{
    char* c1 = s1;
    char* c2 = s2;
    while (count-- > 0)
    {
        if (*c1++ != *c2++)
        {
            return c1[-1] < c2[-1] ? -1 : 1;
        }
    }

    return 0;
}

/*
 * memcpy() - Copy bytes from one buffer to another.
 * @dest: destination buffer
 * @src:  source buffer
 * @len:  number of bytes to copy
 *
 * The buffers must not overlap.
 */
void* memcpy(void* dest, void* src, int len)
{
    char *d = dest;
    char *s = src;
    /* copy byte by byte */
    while (len--)
    {
        *d++ = *s++;
    }
    return dest;
}
