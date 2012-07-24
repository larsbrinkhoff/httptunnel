/*
port/vsyslog.c

Copyright (C) 1999 Lars Brinkhoff.  See COPYING for terms and conditions.
*/

#include <stdio_.h>
#include <syslog_.h>
#include <stdarg.h>

#include "config.h"

#ifndef HAVE_SYSLOG
void
syslog (int level, const char *fmt0, ...)
{
#ifdef HAVE_VSYSLOG
  va_list ap;
  va_start (ap, fmt0);
  vsyslog (level, fmt0, ap);
  va_end (ap);
#else
  /* This system has neither syslog() nor vsyslog(), so do nothing. */
  /* FIXME: this function could open a file and log messages in it. */
#endif
}
#endif

#ifndef HAVE_VSYSLOG
void
vsyslog (int level, const char *fmt0, va_list ap)
{
  char buffer[512];

  if (vsnprintf (buffer, sizeof buffer, fmt0, ap) > 0)
    syslog (level, buffer);
}
#endif /* HAVE_VSYSLOG */
