/*
port/syslog_.h

Copyright (C) 1999 Lars Brinkhoff.  See COPYING for terms and conditions.
*/

#ifndef PORT_SYSLOG_H
#define PORT_SYSLOG_H

#include "config.h"

#include <stdarg.h>

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#else
void syslog (int level, char *fmt0, ...);
#endif

#ifndef HAVE_VSYSLOG
void vsyslog (int level, const char *fmt0, va_list ap);
#endif

#endif
