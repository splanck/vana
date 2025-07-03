/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the nl_langinfo function for vlibc. Provides minimal locale tables or defers to the host implementation on BSD systems.
 */

#include "langinfo.h"

#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
#define nl_langinfo host_nl_langinfo
#include "host/langinfo.h"
#undef nl_langinfo
extern char *host_nl_langinfo(nl_item) __asm("nl_langinfo");

char *nl_langinfo(nl_item item)
{
    return host_nl_langinfo(item);
}
#else

static const char *const abday[7] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static const char *const day[7] = {
    "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday"
};

static const char *const abmon[12] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static const char *const mon[12] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

char *nl_langinfo(nl_item item)
{
    switch (item) {
    case CODESET:    return (char *)"ASCII";
    case D_T_FMT:    return (char *)"%a %b %e %H:%M:%S %Y";
    case D_FMT:      return (char *)"%m/%d/%y";
    case T_FMT:      return (char *)"%H:%M:%S";
    case T_FMT_AMPM: return (char *)"%I:%M:%S %p";
    case AM_STR:     return (char *)"AM";
    case PM_STR:     return (char *)"PM";
    case DAY_1:      return (char *)day[0];
    case DAY_2:      return (char *)day[1];
    case DAY_3:      return (char *)day[2];
    case DAY_4:      return (char *)day[3];
    case DAY_5:      return (char *)day[4];
    case DAY_6:      return (char *)day[5];
    case DAY_7:      return (char *)day[6];
    case ABDAY_1:    return (char *)abday[0];
    case ABDAY_2:    return (char *)abday[1];
    case ABDAY_3:    return (char *)abday[2];
    case ABDAY_4:    return (char *)abday[3];
    case ABDAY_5:    return (char *)abday[4];
    case ABDAY_6:    return (char *)abday[5];
    case ABDAY_7:    return (char *)abday[6];
    case MON_1:      return (char *)mon[0];
    case MON_2:      return (char *)mon[1];
    case MON_3:      return (char *)mon[2];
    case MON_4:      return (char *)mon[3];
    case MON_5:      return (char *)mon[4];
    case MON_6:      return (char *)mon[5];
    case MON_7:      return (char *)mon[6];
    case MON_8:      return (char *)mon[7];
    case MON_9:      return (char *)mon[8];
    case MON_10:     return (char *)mon[9];
    case MON_11:     return (char *)mon[10];
    case MON_12:     return (char *)mon[11];
    case ABMON_1:    return (char *)abmon[0];
    case ABMON_2:    return (char *)abmon[1];
    case ABMON_3:    return (char *)abmon[2];
    case ABMON_4:    return (char *)abmon[3];
    case ABMON_5:    return (char *)abmon[4];
    case ABMON_6:    return (char *)abmon[5];
    case ABMON_7:    return (char *)abmon[6];
    case ABMON_8:    return (char *)abmon[7];
    case ABMON_9:    return (char *)abmon[8];
    case ABMON_10:   return (char *)abmon[9];
    case ABMON_11:   return (char *)abmon[10];
    case ABMON_12:   return (char *)abmon[11];
    case RADIXCHAR:  return (char *)".";
    case THOUSEP:    return (char *)"";
    case YESEXPR:    return (char *)"^[yY]";
    case NOEXPR:     return (char *)"^[nN]";
    case CRNCYSTR:   return (char *)"";
    default:        return "";
    }
}
#endif

