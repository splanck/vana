/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the strerror_r functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "stdio.h"
#include "string.h"

struct err_entry {
    int code;
    const char *msg;
};

extern const struct err_entry __vlibc_err_table[];

int strerror_r(int errnum, char *buf, size_t buflen)
{
    if (!buf || buflen == 0)
        return -1;
    for (size_t i = 0; __vlibc_err_table[i].msg; ++i) {
        if (__vlibc_err_table[i].code == errnum) {
            size_t len = strnlen(__vlibc_err_table[i].msg, buflen - 1);
            memcpy(buf, __vlibc_err_table[i].msg, len);
            buf[len] = '\0';
            return 0;
        }
    }
    snprintf(buf, buflen, "Unknown error %d", errnum);
    return 0;
}
