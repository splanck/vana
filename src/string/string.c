#include "string.h"

/**
 * @file string.c
 * @brief Minimal C string helpers used by the kernel and user programs.
 *
 * The kernel cannot rely on the host C library, so a subset of standard
 * routines are reimplemented here. They are lightweight but compatible with the
 * usual libc counterparts.
 */

/**
 * Convert an uppercase ASCII character to lowercase.
 *
 * Characters outside the 'A'-'Z' range are returned unchanged. This helper is
 * used when implementing case-insensitive string comparisons within the
 * kernel.
 */
char tolower(char s1)
{
    if (s1 >= 65 && s1 <= 90)
    {
        s1 += 32;
    }

    return s1;
}

/**
 * Calculate the length of a NUL terminated string.
 *
 * Implemented so the kernel does not rely on the host's libc. Returns the
 * number of characters before the NUL byte.
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

/**
 * Return the length of a string but scan at most `max` bytes.
 *
 * This guards against reading beyond the provided buffer when a NUL terminator
 * may be missing. The count returned excludes the NUL if found.
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

/**
 * Version of strnlen that also stops at a custom terminator.
 *
 * Useful when parsing path strings where components are separated by a
 * specific character. Scans no more than `max` bytes.
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

/**
 * Compare two strings ignoring case for at most `n` characters.
 *
 * Differences are returned in the same fashion as strncmp. This is used by the
 * filesystem to compare path components in a case-insensitive manner.
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
/**
 * Compare two strings for at most `n` characters.
 *
 * Returns the difference between the first mismatching bytes. Used throughout
 * the kernel when parsing configuration strings and file names.
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

/**
 * Copy a NUL terminated string into the destination buffer.
 *
 * The caller must ensure `dest` is large enough. Used heavily by the loader
 * and filesystem code when constructing paths and messages.
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

/**
 * Safe string copy with explicit length.
 *
 * Copies up to `count-1` characters and always terminates `dest` with a NUL
 * byte. This avoids buffer overruns when handling user supplied data.
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
/**
 * Check if a character is an ASCII decimal digit.
 */
bool isdigit(char c)
{
    return c >= 48 && c <= 57;
}
/* Convert an ASCII digit to its numeric value. */
/**
 * Convert an ASCII digit into its numeric value.
 */
int tonumericdigit(char c)
{
    return c - 48;
}

/**
 * Convert an integer value to a decimal string.
 *
 * The output buffer must be large enough to hold the resulting digits and NUL
 * terminator. Used primarily for debug output.
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
