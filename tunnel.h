/*
tunnel.h

Copyright (C) 1999 Lars Brinkhoff.  See COPYING for terms and conditions.
*/

/*
This is the programming interface to the HTTP tunnel.  It consists
of the following functions:

Tunnel *tunnel_new_client (const char *host, int host_port,
                           const char *proxy, int proxy_port,
			   size_t content_length);

  Create a new HTTP tunnel client.

Tunnel *tunnel_new_server (const char *host, int port,
			   size_t content_length);

  Create a new HTTP tunnel server.  If CONTENT_LENGTH is 0, the
  Content-Length of the HTTP GET response will be determined
  automatically in some way.  If HOST is not NULL, use it to bind the
  server socket to a specific network interface.

int tunnel_connect (Tunnel *tunnel);

  Open the tunnel.  (Client only.)

int tunnel_accept (Tunnel *tunnel);

  Accept a tunnel connection.  (Server only.)

int tunnel_pollin_fd (Tunnel *tunnel);

  Return a file descriptor that can be used to poll for input from
  the tunnel.

ssize_t tunnel_read (Tunnel *tunnel, void *data, size_t length);
ssize_t tunnel_write (Tunnel *tunnel, void *data, size_t length);

  Read or write to the tunnel.  Same semantics as read() and write().
  Watch out for return values less than LENGTH.

int tunnel_padding (Tunnel *tunnel, size_t length);

  Send LENGTH pad bytes.

int tunnel_maybe_pad (Tunnel *tunnel, size_t length);

  Pad to nearest even multiple of LENGTH.

int tunnel_setopt (Tunnel *tunnel, const char *opt, void *data);
int tunnel_getopt (Tunnel *tunnel, const char *opt, void *data);

  Set or get a tunnel option.  Valid options are:

  * strict_content_length

    DATA must be a pointer to an int.  If the int is nonzero, the
    tunnel will always honor Content-Length.  Otherwise, less than
    Content-Length bytes may be sent in a request.

  * keep_alive

    DATA must be a pointer to an int.  If the int is nonzero,
    keep-alive bytes will be sent when the connection is idle.
    Otherwise, no keep-alive bytes will be sent.

  * max_connection_age

    DATA must be a pointer to an int.  The int specifies the maximum
    time a connection will be kept open, in seconds.

  * proxy_authorization

    DATA must be a pointer to a char pointer.  The char pointer
    specifies the proxy authorization string, or NULL if no proxy
    authorization string is to be used.  When this option is set, the
    string will be copied into a newly malloced memory region.
    Likewise, when the option is read, the returned string is copied
    into a newly malloced memory region which the caller must accept
    responsibility to manage.

  * user_agent

    DATA must be a pointer to a char pointer.  The char pointer
    specifies the User-Agent field to be used in HTTP request headers,
    or is NULL is no User-Agent field is to be used.  When this option
    is set, the string will be copied into a newly malloced memory
    region.  Likewise, when the option is read, the returned string is
    copied into a newly malloced memory region which the caller must
    accept responsibility to manage.

int tunnel_close (Tunnel *tunnel);

  Close the tunnel.

void tunnel_destroy (Tunnel *tunnel);

  Free all resources associated with the tunnel object.  */

#ifndef TUNNEL_H
#define TUNNEL_H

#include "config.h"
#include <sys/types.h>

#define DEFAULT_CONNECTION_MAX_TIME 300

typedef struct tunnel Tunnel;

extern Tunnel *tunnel_new_client (const char *host, int host_port,
				  const char *proxy, int proxy_port,
				  size_t content_length);
extern Tunnel *tunnel_new_server (const char *host, int port,
                                  size_t content_length);
extern int tunnel_connect (Tunnel *tunnel);
extern int tunnel_accept (Tunnel *tunnel);
extern int tunnel_pollin_fd (Tunnel *tunnel);
extern ssize_t tunnel_read (Tunnel *tunnel, void *data, size_t length);
extern ssize_t tunnel_write (Tunnel *tunnel, void *data, size_t length);
extern ssize_t tunnel_padding (Tunnel *tunnel, size_t length);
extern int tunnel_maybe_pad (Tunnel *tunnel, size_t length);
extern int tunnel_setopt (Tunnel *tunnel, const char *opt, void *data);
extern int tunnel_getopt (Tunnel *tunnel, const char *opt, void *data);
extern int tunnel_close (Tunnel *tunnel);
extern void tunnel_destroy (Tunnel *tunnel);

#endif /* TUNNEL_H */
