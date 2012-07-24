/*
port/vsnprintf.c

Copyright (C) 1999 Lars Brinkhoff.  See COPYING for terms and conditions.
*/

#include "config.h"

#include <stdio_.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>

#ifndef HAVE_VPRINTF
#error "Must have vfprintf() and vsprintf()."
#endif

int
vsmprintf (char **s, const char *format, va_list ap)
{
  size_t n;
  FILE *f;

  f = fopen ("/dev/null", "w");
  if (f == NULL)
    return -1;

  n = vfprintf (f, format, ap);
  fclose (f);

  *s = malloc (n + 1);
  if (*s == NULL)
    return -1;

  return vsprintf (*s, format, ap);
}

#ifndef HAVE_VSNPRINTF
int
vsnprintf (char *str, size_t n, const char *format, va_list ap)
{
  char *s;
  int m, r;

  m = vsmprintf (&s, format, ap);
  if (m == -1)
    return 0;

  if (m + 1 > n)
    {
      m = n - 1;
      r = -1;
    }
  else
    {
      r = m;
    }

  memcpy (str, s, m);
  free (s);
  str[m] = 0;
  return r;
}

int
snprintf (char *s, size_t n, const char *format, ...)
{
  va_list ap;
  int m;

  va_start (ap, format);
  m = vsnprintf (s, n, format, ap);
  va_end (ap);
  return m;
}
#endif /* HAVE_VSNPRINTF */
