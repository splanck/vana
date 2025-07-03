/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for wide character helpers.
 */
#ifndef WCHAR_H
#define WCHAR_H

#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include "stdio.h"

#ifndef __wchar_t_defined
typedef int wchar_t;
#define __wchar_t_defined
#endif

#ifndef __wint_t_defined
typedef int wint_t;
#define __wint_t_defined
#endif

/* Opaque multibyte conversion state */
typedef struct { int __dummy; } mbstate_t;

/* Convert a multibyte sequence to a wide character */
int mbtowc(wchar_t *pwc, const char *s, size_t n);
/* Convert a wide character to a multibyte sequence */
int wctomb(char *s, wchar_t wc);
int mblen(const char *s, size_t n);
/* Length of a wide-character string */
size_t wcslen(const wchar_t *s);
/* Copy a wide-character string */
wchar_t *wcscpy(wchar_t *dest, const wchar_t *src);
/* Bounded wide string copy */
wchar_t *wcsncpy(wchar_t *dest, const wchar_t *src, size_t n);
/* Compare wide-character strings */
int wcscmp(const wchar_t *s1, const wchar_t *s2);
/* Bounded wide string comparison */
int wcsncmp(const wchar_t *s1, const wchar_t *s2, size_t n);
/* Duplicate a wide-character string */
wchar_t *wcsdup(const wchar_t *s);
/* Search a wide-character string for a character */
wchar_t *wcschr(const wchar_t *s, wchar_t c);
/* Search a wide-character string from the end */
wchar_t *wcsrchr(const wchar_t *s, wchar_t c);
/* Locate a substring in a wide-character string */
wchar_t *wcsstr(const wchar_t *haystack, const wchar_t *needle);
/* Collate wide-character strings */
int wcscoll(const wchar_t *s1, const wchar_t *s2);
/* Transform wide-character string for collation */
size_t wcsxfrm(wchar_t *dest, const wchar_t *src, size_t n);
/* Tokenize a wide-character string */
wchar_t *wcstok(wchar_t *str, const wchar_t *delim, wchar_t **saveptr);
/* Display width of a wide character */
int wcwidth(wchar_t wc);
/* Display width of at most n wide chars */
int wcswidth(const wchar_t *s, size_t n);
/* Convert multibyte to wide character with state */
size_t mbrtowc(wchar_t *pwc, const char *s, size_t n, mbstate_t *ps);
/* Convert wide char to multibyte with state */
size_t wcrtomb(char *s, wchar_t wc, mbstate_t *ps);
/* Convert a multibyte string to wide chars */
size_t mbstowcs(wchar_t *dst, const char *src, size_t n);
/* Convert wide char string to multibyte */
size_t wcstombs(char *dst, const wchar_t *src, size_t n);
/* Convert multibyte string to wide chars with state */
size_t mbsrtowcs(wchar_t *dst, const char **src, size_t n, mbstate_t *ps);
/* Convert wide char string to multibyte with state */
size_t wcsrtombs(char *dst, const wchar_t **src, size_t n, mbstate_t *ps);
/* Return length of next multibyte sequence */
size_t mbrlen(const char *s, size_t n, mbstate_t *ps);
/* Check whether state is initial */
int mbsinit(const mbstate_t *ps);
/* Convert single byte to wide character */
wint_t btowc(int c);
/* Convert wide character to single byte */
int wctob(wchar_t wc);

/* Wide-string to number conversions */
long wcstol(const wchar_t *nptr, wchar_t **endptr, int base);
unsigned long wcstoul(const wchar_t *nptr, wchar_t **endptr, int base);
long long wcstoll(const wchar_t *nptr, wchar_t **endptr, int base);
unsigned long long wcstoull(const wchar_t *nptr, wchar_t **endptr, int base);
intmax_t wcstoimax(const wchar_t *nptr, wchar_t **endptr, int base);
uintmax_t wcstoumax(const wchar_t *nptr, wchar_t **endptr, int base);
double wcstod(const wchar_t *nptr, wchar_t **endptr);
float wcstof(const wchar_t *nptr, wchar_t **endptr);
long double wcstold(const wchar_t *nptr, wchar_t **endptr);

/* Wide-character formatted output */
int wprintf(const wchar_t *format, ...);
int fwprintf(FILE *stream, const wchar_t *format, ...);
int swprintf(wchar_t *str, size_t size, const wchar_t *format, ...);
int vwprintf(const wchar_t *format, va_list ap);
int vfwprintf(FILE *stream, const wchar_t *format, va_list ap);
int vswprintf(wchar_t *str, size_t size, const wchar_t *format, va_list ap);

/* Wide-character formatted input */
int wscanf(const wchar_t *format, ...);
int fwscanf(FILE *stream, const wchar_t *format, ...);
int swscanf(const wchar_t *str, const wchar_t *format, ...);
int vwscanf(const wchar_t *format, va_list ap);
int vfwscanf(FILE *stream, const wchar_t *format, va_list ap);
int vswscanf(const wchar_t *str, const wchar_t *format, va_list ap);

/* Time formatting */
#include "time.h"
/* Format time information using wide characters */
size_t wcsftime(wchar_t *s, size_t max, const wchar_t *format,
                const struct tm *tm);

/* Wide-character byte I/O */
    wint_t fgetwc(FILE *stream);
    wint_t fputwc(wchar_t wc, FILE *stream);
#define getwc(stream) fgetwc(stream)
#define putwc(wc, stream) fputwc(wc, stream)

/* Wide-character memory operations */
wchar_t *wmemcpy(wchar_t *dest, const wchar_t *src, size_t n);
wchar_t *wmemmove(wchar_t *dest, const wchar_t *src, size_t n);
wchar_t *wmemset(wchar_t *s, wchar_t c, size_t n);
int wmemcmp(const wchar_t *s1, const wchar_t *s2, size_t n);
wchar_t *wmemchr(const wchar_t *s, wchar_t c, size_t n);

#endif /* WCHAR_H */
