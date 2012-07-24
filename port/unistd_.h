/*
port/unistd_.h

Copyright (C) 1999 Lars Brinkhoff.  See COPYING for terms and conditions.
*/

#ifndef PORT_UNISTD_H
#define PORT_UNISTD_H

#include "config.h"

#ifndef HAVE_DAEMON
extern int daemon (int, int);
#endif

#include <unistd.h>

#endif /* PORT_UNISTD_H */
