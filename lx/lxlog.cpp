#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>

#include "lxlog.h"

using namespace lx;

void log::print (enum log_level level, const char *func, int line, const char *fmt, va_list ap)
{
    const char *level_name[] =
    {
        "ERR",
        "WAR",
        "NOT",
        "INF",
        "DBG",
        "TRA",
    };

    char *str;
    const char *lname;

    lname = "   ";
    if (level < (sizeof (level_name) / sizeof (level_name[0])))
        lname = level_name[level];

    str = NULL;
    vasprintf (&str, fmt, ap);

    printf ("%12s %16s.%5d %s: %s\n", this->name, func, line, lname, str);
    free (str);
}

