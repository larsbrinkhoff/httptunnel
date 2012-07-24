/*
port/poll.c

Copyright (C) 1999 Lars Brinkhoff.  See COPYING for terms and conditions.
*/

#include "config.h"

#ifndef HAVE_POLL

#include <unistd_.h>
#include <sys/poll_.h>
#include <sys/time.h>
#include <sys/types.h>

#ifndef HAVE_SELECT
#error "Must have either poll() or select()."
#endif

int
poll (struct pollfd *p, int n, int t)
{
  struct timeval t2, *t3;
  fd_set r, w, e;
  int i, m, ret;

  FD_ZERO (&r);
  FD_ZERO (&w);
  FD_ZERO (&e);
  m = -1;
  for (i = 0; i < n; i++)
    {
      if (p[i].fd < 0)
	continue;
      if (p[i].events & POLLIN)
        FD_SET (p[i].fd, &r);
      if (p[i].events & POLLOUT)
        FD_SET (p[i].fd, &w);
      FD_SET (p[i].fd, &e); /* or something */
      if (p[i].fd > m)
        m = p[i].fd;
    }

  if (m == -1)
    return 0;

  if (t < 0)
    t3 = NULL;
  else
    {
      t2.tv_sec = t / 1000;
      t2.tv_usec = 1000 * (t % 1000);
      t3 = &t2;
    }

  ret = select (m + 1, &r, &w, &e, t3);

  if (ret != -1)
    for (i = 0; i < n; i++)
      {
	p[i].revents = 0;
	if (FD_ISSET (p[i].fd, &r))
	  p[i].revents |= POLLIN;
	if (FD_ISSET (p[i].fd, &w))
	  p[i].revents |= POLLOUT;
	if (FD_ISSET (p[i].fd, &e))
	  p[i].revents |= POLLERR; /* or something */
      }

  return ret;
}

#endif /* HAVE_POLL */
