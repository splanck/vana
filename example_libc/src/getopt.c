/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the getopt functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "getopt.h"
#include "stdio.h"
#include "string.h"

char *optarg = NULL;
int optind = 1;
int opterr = 1;
int optopt = 0;

static const char *next = NULL;

/*
 * Parse short options from argv according to optstring. The static
 * pointer 'next' is used to process clusters like -abc one letter at
 * a time and optarg is set when an option requires an argument.
 */
int getopt(int argc, char * const argv[], const char *optstring)
{

    if (!next || *next == '\0') {
        if (optind >= argc)
            return -1;
        const char *arg = argv[optind];
        if (arg[0] != '-' || arg[1] == '\0')
            return -1;
        if (strcmp(arg, "--") == 0) {
            optind++;
            return -1;
        }
        next = arg + 1;
        optind++;
    }

    char c = *next++;
    const char *pos = strchr(optstring, c);
    if (!pos) {
        optopt = c;
        if (opterr && optstring[0] != ':')
            fprintf(stderr, "unknown option -%c\n", c);
        return '?';
    }

    if (pos[1] == ':') {
        if (*next != '\0') {
            optarg = (char *)next;
            next = NULL;
        } else if (optind < argc) {
            optarg = argv[optind++];
            next = NULL;
        } else {
            optopt = c;
            if (opterr && optstring[0] != ':')
                fprintf(stderr, "option -%c requires argument\n", c);
            return optstring[0] == ':' ? ':' : '?';
        }
    } else {
        optarg = NULL;
    }

    return c;
}

/*
 * Handle GNU-style long options in addition to short options.
 * Options beginning with "--" are matched against the longopts
 * table and the index is stored in longindex when non-NULL.
 */
int getopt_long(int argc, char * const argv[], const char *optstring,
                const struct option *longopts, int *longindex)
{
    if (optind >= argc)
        return -1;

    const char *arg = argv[optind];
    if (arg[0] != '-' || arg[1] == '\0')
        return -1;

    if (strcmp(arg, "--") == 0) {
        optind++;
        return -1;
    }

    if (arg[1] == '-') {
        const char *name = arg + 2;
        const char *eq = strchr(name, '=');
        size_t namelen = eq ? (size_t)(eq - name) : strlen(name);

        for (int i = 0; longopts && longopts[i].name; i++) {
            if (strncmp(name, longopts[i].name, namelen) == 0 &&
                longopts[i].name[namelen] == '\0') {
                if (longindex)
                    *longindex = i;

                if (longopts[i].has_arg == required_argument) {
                    if (eq) {
                        optarg = (char *)(eq + 1);
                    } else if (optind + 1 < argc) {
                        optarg = argv[++optind];
                    } else {
                        optind++;
                        optopt = longopts[i].val;
                        if (opterr)
                            fprintf(stderr,
                                    "option '--%s' requires argument\n",
                                    longopts[i].name);
                        return '?';
                    }
                } else if (longopts[i].has_arg == optional_argument) {
                    optarg = eq ? (char *)(eq + 1) : NULL;
                } else {
                    if (eq && opterr)
                        fprintf(stderr,
                                "option '--%s' doesn't allow an argument\n",
                                longopts[i].name);
                    optarg = NULL;
                }

                optind++;
                if (longopts[i].flag) {
                    *longopts[i].flag = longopts[i].val;
                    return 0;
                }
                return longopts[i].val;
            }
        }

        optind++;
        if (opterr)
            fprintf(stderr, "unknown option '%s'\n", arg);
        optopt = 0;
        return '?';
    }

    return getopt(argc, argv, optstring);
}
