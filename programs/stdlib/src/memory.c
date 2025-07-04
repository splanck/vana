#include "memory.h"

/*
 * Basic memory utilities used by the example programs. These
 * provide small stand-alone versions of common C library
 * functions for working with raw buffers.
 */

/*
 * memset() - Fill a memory block with the given byte value.
 */
void* memset(void* ptr, int c, size_t size)
{
    char* c_ptr = (char*) ptr;
    /* set each byte in the block */
    for (int i = 0; i < size; i++)
    {
        c_ptr[i] = (char) c;
    }
    return ptr;
}

/*
 * memcmp() - Compare two buffers byte by byte.
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
 * memcpy() - Copy data from one buffer to another.
 */
void* memcpy(void* dest, void* src, int len)
{
    char *d = dest;
    char *s = src;
    while (len--)
    {
        *d++ = *s++;
    }
    return dest;
}
