/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the wctype functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "wctype.h"

/* Check if wide character is alphabetic. */
int iswalpha(wint_t wc)
{
    if (wc < 0)
        return 0;
    if ((unsigned)wc < 0x80)
        return isalpha((unsigned char)wc);
    if ((wc >= 0xC0 && wc <= 0xD6) ||
        (wc >= 0xD8 && wc <= 0xF6) ||
        (wc >= 0xF8 && wc <= 0xFF))
        return 1;
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_iswalpha(wint_t) __asm("iswalpha");
    return host_iswalpha(wc);
#else
    return 0;
#endif
}

/* Check if wide character is a decimal digit. */
int iswdigit(wint_t wc)
{
    return (wc >= L'0' && wc <= L'9');
}

/* Check if wide character is alphanumeric. */
int iswalnum(wint_t wc)
{
    return iswalpha(wc) || iswdigit(wc);
}

/* Check if wide character is whitespace. */
int iswspace(wint_t wc)
{
    switch (wc) {
    case L' ': case L'\t': case L'\n': case L'\r': case L'\v': case L'\f':
        return 1;
    case 0xA0:
        return 1; /* non-breaking space */
    default:
        return 0;
    }
}

/* Check if wide character is uppercase. */
int iswupper(wint_t wc)
{
    if (wc < 0)
        return 0;
    if ((unsigned)wc < 0x80)
        return isupper((unsigned char)wc);
    if ((wc >= 0xC0 && wc <= 0xD6) || (wc >= 0xD8 && wc <= 0xDE))
        return 1;
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_iswupper(wint_t) __asm("iswupper");
    return host_iswupper(wc);
#else
    return 0;
#endif
}

/* Check if wide character is lowercase. */
int iswlower(wint_t wc)
{
    if (wc < 0)
        return 0;
    if ((unsigned)wc < 0x80)
        return islower((unsigned char)wc);
    if ((wc >= 0xDF && wc <= 0xF6) || (wc >= 0xF8 && wc <= 0xFF))
        return 1;
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_iswlower(wint_t) __asm("iswlower");
    return host_iswlower(wc);
#else
    return 0;
#endif
}

/* Check if wide character is a hexadecimal digit. */
int iswxdigit(wint_t wc)
{
    if (wc >= L'0' && wc <= L'9')
        return 1;
    if (wc >= L'a' && wc <= L'f')
        return 1;
    if (wc >= L'A' && wc <= L'F')
        return 1;
    return 0;
}

/* Convert wide character to lowercase. */
wint_t towlower(wint_t wc)
{
    if (iswupper(wc))
        return wc + (L'a' - L'A');
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    if ((unsigned)wc >= 0x80) {
        extern wint_t host_towlower(wint_t) __asm("towlower");
        return host_towlower(wc);
    }
#endif
    return wc;
}

/* Convert wide character to uppercase. */
wint_t towupper(wint_t wc)
{
    if (iswlower(wc))
        return wc - (L'a' - L'A');
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    if ((unsigned)wc >= 0x80) {
        extern wint_t host_towupper(wint_t) __asm("towupper");
        return host_towupper(wc);
    }
#endif
    return wc;
}

/* Check if wide character is printable. */
int iswprint(wint_t wc)
{
    if (wc < 0)
        return 0;
    if ((unsigned)wc < 0x80)
        return isprint((unsigned char)wc);
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_iswprint(wint_t) __asm("iswprint");
    return host_iswprint(wc);
#else
    return (wc >= 0x20 && wc < 0x7F);
#endif
}

/* Check if wide character is a control character. */
int iswcntrl(wint_t wc)
{
    if (wc < 0)
        return 0;
    if ((unsigned)wc < 0x80)
        return iscntrl((unsigned char)wc);
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_iswcntrl(wint_t) __asm("iswcntrl");
    return host_iswcntrl(wc);
#else
    return (wc < 0x20) || (wc == 0x7F);
#endif
}

/* Check if wide character is punctuation. */
int iswpunct(wint_t wc)
{
    if (wc < 0)
        return 0;
    if ((unsigned)wc < 0x80)
        return ispunct((unsigned char)wc);
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_iswpunct(wint_t) __asm("iswpunct");
    return host_iswpunct(wc);
#else
    if ((wc >= 0x21 && wc <= 0x2F) ||
        (wc >= 0x3A && wc <= 0x40) ||
        (wc >= 0x5B && wc <= 0x60) ||
        (wc >= 0x7B && wc <= 0x7E))
        return 1;
    return 0;
#endif
}

/* Check if wide character has a visible representation. */
int iswgraph(wint_t wc)
{
    if (wc < 0)
        return 0;
    if ((unsigned)wc < 0x80)
        return isgraph((unsigned char)wc);
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_iswgraph(wint_t) __asm("iswgraph");
    return host_iswgraph(wc);
#else
    return (wc >= 0x21 && wc < 0x7F);
#endif
}

/* Check if wide character is blank (space or tab). */
int iswblank(wint_t wc)
{
    if (wc == L' ' || wc == L'\t')
        return 1;
    if ((unsigned)wc < 0x80)
        return 0;
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    extern int host_iswblank(wint_t) __asm("iswblank");
    return host_iswblank(wc);
#else
    return 0;
#endif
}

