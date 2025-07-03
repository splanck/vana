/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for string manipulation functions.
 */
#ifndef STRING_H
#define STRING_H

#include <stddef.h>

#if defined(__has_include)
#  if __has_include("/usr/include/string.h")
#    include "/usr/include/string.h"
#  endif
#endif

/* Length of a NUL terminated string without using libc */
size_t vstrlen(const char *s);
/* Bounded string length */
size_t strnlen(const char *s, size_t maxlen);
/* Copy a string including the terminating NUL */
char *vstrcpy(char *dest, const char *src);
/* Compare two strings up to n characters */
int vstrncmp(const char *s1, const char *s2, size_t n);
/* Full string comparison */
int strcmp(const char *s1, const char *s2);
/* Find first occurrence of character in string */
char *strchr(const char *s, int c);
/* Allocate a copy of a string */
char *strdup(const char *s);
/* Duplicate at most n characters */
char *strndup(const char *s, size_t n);
/* Copy at most n characters, padding with NULs */
char *strncpy(char *dest, const char *src, size_t n);
/* Append a string */
char *strcat(char *dest, const char *src);
/* Append at most n characters */
char *strncat(char *dest, const char *src, size_t n);
/* Copy and return pointer to trailing NUL */
char *stpcpy(char *dest, const char *src);
/* Bounded copy returning pointer to end */
char *stpncpy(char *dest, const char *src, size_t n);
/* Safe string copy */
size_t strlcpy(char *dst, const char *src, size_t size);
/* Safe string concatenation */
size_t strlcat(char *dst, const char *src, size_t size);
/* Memory copy helper used internally */
void *vmemcpy(void *dest, const void *src, size_t n);
/* Memory move helper used internally */
void *vmemmove(void *dest, const void *src, size_t n);
/* Memory set helper used internally */
void *vmemset(void *s, int c, size_t n);
/* Memory comparison helper used internally */
int vmemcmp(const void *s1, const void *s2, size_t n);

/* Standard memory copy */
void *memcpy(void *dest, const void *src, size_t n);
/* Memory move allowing overlap */
void *memmove(void *dest, const void *src, size_t n);
/* Copy until byte found */
void *memccpy(void *dest, const void *src, int c, size_t n);
/* Copy returning pointer past end */
void *mempcpy(void *dest, const void *src, size_t n);
/* Fill region of memory */
void *memset(void *s, int c, size_t n);
/* Compare two memory regions */
int memcmp(const void *s1, const void *s2, size_t n);
/* Search memory for a byte */
void *memchr(const void *s, int c, size_t n);
/* Reverse search memory for a byte */
void *memrchr(const void *s, int c, size_t n);
/* Search for byte sequence within a block */
void *memmem(const void *haystack, size_t haystacklen,
             const void *needle, size_t needlelen);
/* Span of bytes from accept */
size_t strspn(const char *s, const char *accept);
/* Span of bytes not in reject */
size_t strcspn(const char *s, const char *reject);
/* Locate any byte from accept */
char *strpbrk(const char *s, const char *accept);
/* Reverse search for character */
char *strrchr(const char *s, int c);
/* Find substring */
char *strstr(const char *haystack, const char *needle);
/* Tokenize string using static state */
char *strtok(char *str, const char *delim);
/* Reentrant tokenizer */
char *strtok_r(char *str, const char *delim, char **saveptr);
/* Simple field separator */
char *strsep(char **stringp, const char *delim);
/* Case-insensitive substring search */
char *strcasestr(const char *haystack, const char *needle);
/* Case-insensitive compare */
int strcasecmp(const char *s1, const char *s2);
/* Case-insensitive compare with limit */
int strncasecmp(const char *s1, const char *s2, size_t n);
/* Locale aware comparison */
int strcoll(const char *s1, const char *s2);
/* Locale transformation for sorting */
size_t strxfrm(char *dest, const char *src, size_t n);

#endif /* STRING_H */
