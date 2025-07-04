#include "string.h"

/*
 * Minimal string utility routines used throughout the kernel.  These
 * helpers implement common libc functionality such as measuring string
 * length and copying buffers without relying on the host C library.
 * The primitives here are shared by both kernel and user space code.
 */

/*
 * Convert an uppercase ASCII character to lowercase.  Characters
 * outside the A-Z range are returned unchanged.
 */
char tolower(char s1)
{
    if (s1 >= 65 && s1 <= 90)
    {
        s1 += 32;
    }

    return s1;
}

/*
 * Return the length of a NUL terminated string.  This avoids relying
 * on the host implementation of strlen.
 */
int strlen(const char* ptr)
{
    int i = 0;
    while(*ptr != 0)
    {
        i++;
        ptr += 1;
    }

    return i;
}

/*
 * Bounded string length.  Scan at most max bytes looking for the
 * terminating NUL and return the number of characters seen.
 */
int strnlen(const char* ptr, int max)
{
    int i = 0;
    for (i = 0; i < max; i++)
    {
        if (ptr[i] == 0)
            break;
    }

    return i;
}

/*
 * Variant of strnlen that also stops at the provided terminator
 * character in addition to a NUL byte.
 */
int strnlen_terminator(const char* str, int max, char terminator)
{
    int i = 0;
    for(i = 0; i < max; i++)
    {
        if (str[i] == '\0' || str[i] == terminator)
            break;
    }

    return i;
}

/*
 * Case-insensitive string compare of at most n characters.
 * Differences are returned similarly to strncmp.
 */
int istrncmp(const char* s1, const char* s2, int n)
{
    unsigned char u1, u2;
    while(n-- > 0)
    {
        u1 = (unsigned char)*s1++;
        u2 = (unsigned char)*s2++;
        if (u1 != u2 && tolower(u1) != tolower(u2))
            return u1 - u2;
        if (u1 == '\0')
            return 0;
    }

    return 0;
}
/*
 * Compare two strings for at most n characters and return
 * the difference of the first mismatching byte.
 */
int strncmp(const char* str1, const char* str2, int n)
{
    unsigned char u1, u2;

    while(n-- > 0)
    {
        u1 = (unsigned char)*str1++;
        u2 = (unsigned char)*str2++;
        if (u1 != u2)
            return u1 - u2;
        if (u1 == '\0')
            return 0;
    }

    return 0;
}

/*
 * Copy the NUL terminated string src into dest.  The destination
 * buffer must be large enough to hold the entire source string.
 */
char* strcpy(char* dest, const char* src)
{
    char* res = dest;
    while(*src != 0)
    {
        *dest = *src;
        src += 1;
        dest += 1;
    }

    *dest = 0x00;

    return res;
}

/*
 * Copy at most count-1 characters from src into dest and always
 * NUL terminate the destination buffer.
 */
char* strncpy(char* dest, const char* src, int count)
{
    int i = 0;
    for (i = 0; i < count-1; i++)
    {
        if (src[i] == 0x00)
            break;

        dest[i] = src[i];
    }

    dest[i] = 0x00;
    return dest;
}

/* Return true if the ASCII character c is a decimal digit. */
bool isdigit(char c)
{
    return c >= 48 && c <= 57;
}
/* Convert an ASCII digit to its numeric value. */
int tonumericdigit(char c)
{
    return c - 48;
}

/*
 * Convert an integer value to its decimal string representation.
 * The output buffer must be large enough to hold the result.
 */
void int_to_string(int value, char* out)
{
    char temp[32];
    int i = 0;

    if (value == 0)
    {
        out[0] = '0';
        out[1] = '\0';
        return;
    }

    while (value > 0)
    {
        int digit = value % 10;
        temp[i++] = (char)(digit + '0');
        value /= 10;
    }

    int j = 0;
    while (i > 0)
    {
        out[j++] = temp[--i];
    }
    out[j] = '\0';
}
