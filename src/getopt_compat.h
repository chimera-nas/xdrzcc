// SPDX-FileCopyrightText: 2026 Ben Jarvis
// SPDX-License-Identifier: LGPL-2.1-only

#pragma once

#ifndef _WIN32
#include <getopt.h>
#else // ifndef _WIN32

#include <string.h>

/* Only switches without arguments are needed. Support grouped switches and
 * the -- terminator without introducing a POSIX runtime dependency. */
static int optind = 1;

static int
getopt(
    int         argc,
    char *const argv[],
    const char *options)
{
    static const char *next;
    int                option;

    if (!next || !*next) {
        if (optind >= argc || argv[optind][0] != '-' || !argv[optind][1]) {
            return -1;
        }
        if (strcmp(argv[optind], "--") == 0) {
            optind++;
            return -1;
        }
        next = argv[optind++] + 1;
    }

    option = (unsigned char) *next++;
    return strchr(options, option) ? option : '?';
} // getopt

#endif // ifndef _WIN32
