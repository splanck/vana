#include "memory.h"

/*
 * Basic memory utilities used throughout the kernel.  These are
 * "do it yourself" versions of the usual C library routines and are
 * kept intentionally simple so that the kernel does not rely on any
 * external libc when bootstrapping.
 */

/*
 * memset() - Fill a block of memory with a byte value.
 *
 *      +--------- size ---------+
 * ptr: | c  c  c  c  ...        |
 *      +------------------------+
 *
 * @ptr  Destination memory address.
 * @c    Value to write (only the low byte is used).
 * @size Number of bytes to write.
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
 * memcmp() - Compare two memory buffers byte by byte.
 *
 * The routine walks both buffers until either @count bytes have been
 * examined or a mismatch is found.  It is used by the heap allocator and
 * paging code when checking table entries.
 *
 * @s1    First buffer.
 * @s2    Second buffer.
 * @count Number of bytes to compare.
 *
 * Returns 0 if the buffers are equal, a positive value if the first
 * differing byte in @s1 is greater, or a negative value if it is less.
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
 *
 *      dest ---> +----------------------+ <--- src
 *                |   len bytes copied   |
 *      dest ---> +----------------------+
 *
 * The buffers must not overlap as the copy proceeds in a simple
 * forward manner without any temporary storage.
 *
 * @dest Destination buffer.
 * @src  Source buffer.
 * @len  Number of bytes to copy.
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
