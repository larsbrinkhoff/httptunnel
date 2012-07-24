/*
port/daemon.c

Copyright (C) 1999 Lars Brinkhoff.  See COPYING for terms and conditions.
*/

#include "config.h"

#ifndef HAVE_DAEMON

#include <unistd_.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int
daemon (int nochdir, int noclose)
{
  if (fork () != 0)
    exit (0);

  if (!nochdir)
    chdir ("/");

  close (0);
  close (1);
  close (2);

  if (noclose == 0)
    {
      open ("/dev/null", O_RDONLY);
      open ("/dev/null", O_WRONLY);
      open ("/dev/null", O_WRONLY);
    }

  /* FIXME: disassociate from controlling terminal. */

  return 0;
}

#endif /* HAVE_DAEMON */
