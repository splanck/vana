/*
 * BSD 2-Clause License
 *
 * Purpose: Implements locale object helpers for vlibc. When running on
 * BSD systems these wrap the host implementations. Otherwise only the
 * "C" locale is supported.
 */
#include "locale.h"
#include "string.h"
#include "errno.h"

#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)

#define newlocale  host_newlocale
#define duplocale  host_duplocale
#define freelocale host_freelocale
#define uselocale  host_uselocale
#include "host/locale.h"
#undef newlocale
#undef duplocale
#undef freelocale
#undef uselocale

extern locale_t host_newlocale(int, const char *, locale_t) __asm("newlocale");
extern locale_t host_duplocale(locale_t) __asm("duplocale");
extern void     host_freelocale(locale_t) __asm("freelocale");
extern locale_t host_uselocale(locale_t) __asm("uselocale");

locale_t newlocale(int mask, const char *loc, locale_t base)
{
    return host_newlocale(mask, loc, base);
}

locale_t duplocale(locale_t loc)
{
    return host_duplocale(loc);
}

void freelocale(locale_t loc)
{
    host_freelocale(loc);
}

locale_t uselocale(locale_t loc)
{
    return host_uselocale(loc);
}

#else /* !BSD */

struct __vlibc_locale {
    int dummy;
};

static struct __vlibc_locale c_locale = {0};
static locale_t current_locale_obj = &c_locale;

locale_t newlocale(int mask, const char *locale, locale_t base)
{
    (void)mask;
    (void)base;
    if (!locale || strcmp(locale, "C") == 0 || strcmp(locale, "POSIX") == 0)
        return &c_locale;
    errno = EINVAL;
    return NULL;
}

locale_t duplocale(locale_t loc)
{
    if (loc == &c_locale)
        return &c_locale;
    errno = EINVAL;
    return NULL;
}

void freelocale(locale_t loc)
{
    (void)loc;
}

locale_t uselocale(locale_t loc)
{
    locale_t old = current_locale_obj;
    if (loc != (locale_t)0) {
        if (loc == LC_GLOBAL_LOCALE)
            current_locale_obj = &c_locale;
        else
            current_locale_obj = loc;
    }
    return old;
}

#endif /* BSD */
