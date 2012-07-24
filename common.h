/*
common.h

Copyright (C) 1999 Lars Brinkhoff.  See COPYING for terms and conditions.
*/

#ifndef COMMON_H
#define COMMON_H

#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio_.h>
#include <unistd_.h>
#include <string.h>
#include <syslog_.h>
#include <getopt.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdarg.h>

#include "tunnel.h"

#define DEFAULT_HOST_PORT 8888
#define DEFAULT_CONTENT_LENGTH (100 * 1024) /* bytes */
#define DEFAULT_KEEP_ALIVE 5 /* seconds */
#define DEFAULT_MAX_CONNECTION_AGE 300 /* seconds */
#define BUG_REPORT_EMAIL "bug-httptunnel@gnu.org"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#if defined (LOG_ERR) && !defined (LOG_ERROR)
#define LOG_ERROR LOG_ERR
#endif

extern int debug_level;
extern FILE *debug_file;

extern void log_exit (int status);
extern void log_notice (char *fmt0, ...);
extern void log_error (char *fmt0, ...);
#ifdef DEBUG_MODE
extern void log_debug (char *fmt0, ...);
extern void log_verbose (char *fmt0, ...);
extern void log_annoying (char *fmt0, ...);
#else
static inline void log_debug () {}
static inline void log_verbose () {}
static inline void log_annoying () {}
#endif

extern int server_socket (struct in_addr addr, int port, int backlog);
extern int set_address (struct sockaddr_in *address,
			const char *host, int port);
extern int open_device (char *device);
extern int handle_device_input (Tunnel *tunnel, int fd, int events);
extern int handle_tunnel_input (Tunnel *tunnel, int fd, int events);
extern void name_and_port (const char *nameport, char **name, int *port);
extern int atoi_with_postfix (const char *s_);
extern RETSIGTYPE log_sigpipe (int);
void dump_buf (FILE *f, unsigned char *buf, size_t len);

static inline ssize_t
read_all (int fd, void *buf, size_t len)
{
  ssize_t n, m, r;
  long flags;
  char *rbuf = buf;

  flags = fcntl (fd, F_GETFL);
  fcntl (fd, F_SETFL, flags & ~O_NONBLOCK);

  r = len;
  for (n = 0; n < len; n += m)
    {
      log_annoying ("read (%d, %p, %d) ...", fd, rbuf + n, len - n);
      m = read (fd, rbuf + n, len - n);
      log_annoying ("... = %d", m);
      if (m == 0)
	{
	  r = 0;
	  break;
	}
      else if (m == -1)
	{
	  if (errno != EAGAIN)
	    {
	      r = -1;
	      break;
	    }
	  else
	    m = 0;
	}
    }

  fcntl (fd, F_SETFL, flags);
  return r;
}

static inline ssize_t
write_all (int fd, void *data, size_t len)
{
  ssize_t n, m;
  char *wdata = data;

  for (n = 0; n < len; n += m)
    {
      log_annoying ("write (%d, %p, %d) ...", fd, wdata + n, len - n);
      m = write (fd, wdata + n, len - n);
      log_annoying ("... = %d", m);
      if (m == 0)
	return 0;
      else if (m == -1)
	{
	  if (errno != EAGAIN)
	    return -1;
	  else
	    m = 0;
	}
    }

  return len;
}

static inline int
do_connect (struct sockaddr_in *address)
{
  int fd;

  fd = socket (AF_INET, SOCK_STREAM, 0);
  if (fd == -1)
    return -1;

  if (connect (fd, (struct sockaddr *)address,
	       sizeof (struct sockaddr_in)) == -1)
    {
      close (fd);
      return -1;
    }

  return fd;
}

static inline void
handle_input (const char *type, Tunnel *tunnel, int fd, int events,
	      int (*handler)(Tunnel *tunnel, int fd, int events),
	      int *closed)
{
  if (events)
    {
      ssize_t n;

      n = handler (tunnel, fd, events);
      if (n == 0 || (n == -1 && errno != EAGAIN))
	{
	  if (n == 0)
	    log_debug ("%s closed", type);
	  else
	    log_error ("%s read error: %s", type, strerror (errno));
	  *closed = TRUE;
	}		
    }
}

#endif /* COMMON_H */
