/*
port/Netdb.h

Copyright (C) 1999 Lars Brinkhoff.  See COPYING for terms and conditions.
*/

#ifndef PORT_NETDB_H
#define PORT_NETDB_H

#include <netdb.h>

#include "config.h"

#ifndef HAVE_ENDPROTOENT
void endprotoent (void);
#endif

#endif /* PORT_NETDB_H */
