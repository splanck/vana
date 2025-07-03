/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements pthread attribute helpers for vlibc.
 */

#include "pthread.h"
#include <errno.h>

/*
 * pthread_attr_init() - initialize an attribute object with
 * default joinable state and no stack size requirement.
 */
int pthread_attr_init(pthread_attr_t *attr)
{
    if (!attr)
        return EINVAL;
    attr->detachstate = PTHREAD_CREATE_JOINABLE;
    attr->stacksize = 0;
    return 0;
}

/*
 * pthread_attr_destroy() - placeholder for destroying an attribute
 * object. Nothing is dynamically allocated so it is a no-op.
 */
int pthread_attr_destroy(pthread_attr_t *attr)
{
    (void)attr;
    return 0;
}

/*
 * pthread_attr_setdetachstate() - set whether a thread created with
 * this attribute is joinable or detached.
 */
int pthread_attr_setdetachstate(pthread_attr_t *attr, int state)
{
    if (!attr)
        return EINVAL;
    if (state != PTHREAD_CREATE_JOINABLE && state != PTHREAD_CREATE_DETACHED)
        return EINVAL;
    attr->detachstate = state;
    return 0;
}

/*
 * pthread_attr_getdetachstate() - retrieve the detach state stored
 * in the attribute object.
 */
int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *state)
{
    if (!attr || !state)
        return EINVAL;
    *state = attr->detachstate;
    return 0;
}

/*
 * pthread_attr_setstacksize() - specify the minimum stack size that a
 * thread should be created with.
 */
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t size)
{
    if (!attr)
        return EINVAL;
    if (size < 16384)
        return EINVAL;
    attr->stacksize = size;
    return 0;
}

/*
 * pthread_attr_getstacksize() - query the stack size set on the
 * attribute object.
 */
int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *size)
{
    if (!attr || !size)
        return EINVAL;
    *size = attr->stacksize;
    return 0;
}
