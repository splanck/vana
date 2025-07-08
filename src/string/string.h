#ifndef STRING_H
#define STRING_H

#include <stdbool.h>

/** Return the length of a NUL terminated string. */
int strlen(const char* ptr);
/** Bounded string length that scans at most `max` bytes. */
int strnlen(const char* ptr, int max);
/** True if the character is 0-9. */
bool isdigit(char c);
/** Convert an ASCII digit into its numeric value. */
int tonumericdigit(char c);
/** Copy a NUL terminated string into dest. */
char* strcpy(char* dest, const char* src);
/** Length limited string copy that always NUL terminates. */
char* strncpy(char* dest, const char* src, int count);
/** Compare two strings up to `n` characters. */
int strncmp(const char* str1, const char* str2, int n);
/** Case-insensitive string compare for at most `n` chars. */
int istrncmp(const char* s1, const char* s2, int n);
/** Like strnlen but stops at the terminator character as well. */
int strnlen_terminator(const char* str, int max, char terminator);
/** Convert an uppercase ASCII character to lowercase. */
char tolower(char s1);
/** Convert an integer to a decimal string. */
void int_to_string(int value, char* out);

#endif
