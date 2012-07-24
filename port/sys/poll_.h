/*
port/sys/poll_.h

Copyright (C) 1999 Lars Brinkhoff.  See COPYING for terms and conditions.
*/

#ifndef PORT_SYS_POLL_H
#define PORT_SYS_POLL_H

#include "config.h"

#ifdef HAVE_SYS_POLL_H
#  include <sys/poll.h>
#else

#define POLLIN		0x01
#define POLLOUT		0x02
#define POLLERR		0x10
#define POLLHUP		0x20
#define POLLNVAL	0x40

struct pollfd
{
  int fd;
  int events;
  int revents;
};

extern int poll (struct pollfd *p, int n, int timeout);

#endif /* !HAVE_SYS_POLL_H */

#endif /* PORT_SYS_POLL_H */
