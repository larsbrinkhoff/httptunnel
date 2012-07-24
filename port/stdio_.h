/*
port/stdio_.h

Copyright (C) 1999 Lars Brinkhoff.  See COPYING for terms and conditions.
*/

#ifndef PORT_STDIO_H
#define PORT_STDIO_H

#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>

#include "config.h"

extern int vsmprintf (char **str, const char *format, va_list ap);
#ifndef HAVE_VSNPRINTF
extern int vsnprintf (char *str, size_t n, const char *format, va_list ap);
extern int snprintf (char *str, size_t n, const char *format, ...);
#endif

#endif /* PORT_STDIO_H */
