/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the locale functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "locale.h"
#include "string.h"
#include "env.h"

#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
#define setlocale host_setlocale
#include "host/locale.h"
#undef setlocale
extern char *host_setlocale(int, const char *) __asm("setlocale");
#endif

static char current_locale[32] = "C";

static struct lconv c_lconv = {
    .decimal_point = ".",
    .thousands_sep = "",
    .grouping = "",
    .int_curr_symbol = "",
    .currency_symbol = "",
    .mon_decimal_point = ".",
    .mon_thousands_sep = "",
    .mon_grouping = "",
    .positive_sign = "",
    .negative_sign = "",
    .int_frac_digits = 127,
    .frac_digits = 127,
    .p_cs_precedes = 127,
    .p_sep_by_space = 127,
    .n_cs_precedes = 127,
    .n_sep_by_space = 127,
    .p_sign_posn = 127,
    .n_sign_posn = 127,
};

/*
 * setlocale() - set or query the program's current locale.
 * The category is mostly ignored unless forwarding to the
 * host's implementation. Passing NULL returns the current
 * locale name; an empty string selects the environment locale.
 */
char *setlocale(int category, const char *locale)
{
    (void)category;

    if (!locale)
        return current_locale;

    if (*locale == '\0') {
        const char *env = getenv("LC_ALL");
        if (!env || !*env)
            env = getenv("LANG");
        locale = env ? env : "C";
    }

    if (strcmp(locale, "C") == 0 || strcmp(locale, "POSIX") == 0) {
        strncpy(current_locale, locale, sizeof(current_locale) - 1);
        current_locale[sizeof(current_locale) - 1] = '\0';
        return current_locale;
    }

#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    char *ret = host_setlocale(category, locale);
    if (ret) {
        strncpy(current_locale, ret, sizeof(current_locale) - 1);
        current_locale[sizeof(current_locale) - 1] = '\0';
    }
    return ret;
#else
    (void)locale;
    return NULL;
#endif
}
/*
 * localeconv() - return a pointer to the locale formatting
 * conventions. Only the "C" locale is currently provided.
 */
struct lconv *localeconv(void)
{
    return &c_lconv;
}
