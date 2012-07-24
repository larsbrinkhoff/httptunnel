/*
common.c

Copyright (C) 1999, 2000 Lars Brinkhoff.  See COPYING for terms and conditions.

Code common to both htc and hts.
*/

#include <time.h>
#include <stdio.h>
#include <netdb_.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <syslog_.h>
#include <termios.h>
#include <sys/poll_.h>

#include "tunnel.h"
#include "common.h"

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#ifdef DEBUG_MODE
static void
log_level (int level, char *fmt0, va_list ap)
{
  if (debug_level >= level)
    {
      struct tm *t2;
      char s[40];
      time_t t;
      int i;

      time (&t);
      t2 = localtime (&t);
      strftime (s, sizeof s, "%Y%m%d %H%M%S ", t2);
      fputs (s, debug_file);

      for (i = 1; i < level; i++)
	fputs ("    ", debug_file);

      vfprintf (debug_file, fmt0, ap);
      fputc ('\n', debug_file);

      fflush (debug_file);
    }
}
#endif

void
log_exit (int status)
{
  log_notice ("exit with status = %d", status);
  exit (status);
}

void
log_notice (char *fmt0, ...)
{
  va_list ap;

  va_start (ap, fmt0);
#ifdef DEBUG_MODE
  log_level (1, fmt0, ap);
#else
  vsyslog (LOG_NOTICE, fmt0, ap);
#endif
  va_end (ap);
}

void
log_error (char *fmt0, ...)
{
  va_list ap;

  va_start (ap, fmt0);
#ifdef DEBUG_MODE
  log_level (2, fmt0, ap);
#else
  vsyslog (LOG_ERROR, fmt0, ap);
#endif
  va_end (ap);
}

#ifdef DEBUG_MODE
void
log_debug (char *fmt0, ...)
{
  va_list ap;

  va_start (ap, fmt0);
  log_level (3, fmt0, ap);
  va_end (ap);
}
#endif

#ifdef DEBUG_MODE
void
log_verbose (char *fmt0, ...)
{
  va_list ap;

  va_start (ap, fmt0);
  log_level (4, fmt0, ap);
  va_end (ap);
}
#endif

#ifdef DEBUG_MODE
void
log_annoying (char *fmt0, ...)
{
  va_list ap;

  va_start (ap, fmt0);
  log_level (5, fmt0, ap);
  va_end (ap);
}
#endif

int
server_socket (struct in_addr addr, int port, int backlog)
{
  struct sockaddr_in address;
  int i, s;

  s = socket (PF_INET, SOCK_STREAM, 0);
  if (s == -1)
    return -1;
  
  i = 1;
  if (setsockopt (s, SOL_SOCKET, SO_REUSEADDR, (void *)&i, sizeof i) == -1)
    {
      log_error ("server_socket: setsockopt SO_REUSEADDR: %s",
		 strerror (errno));
    }

  memset (&address, '\0', sizeof address);
#if defined(__FreeBSD__) || defined(__OpenBSD__)
  address.sin_len = sizeof address;
#endif
  address.sin_family = PF_INET;
  address.sin_port = htons ((short)port);
  address.sin_addr = addr;
  
  if (bind (s, (struct sockaddr *)&address, sizeof (address)) == -1)
    {
      close (s);
      return -1;
    }

  if (listen (s, (unsigned)backlog) == -1)
    {
      close (s);
      return -1;
    } 

  return s;
}

int
set_address (struct sockaddr_in *address, const char *host, int port)
{
  memset (address, '\0', sizeof *address);
#if defined(__FreeBSD__) || defined(__OpenBSD__)
  address->sin_len = sizeof *address;
#endif
  address->sin_family = PF_INET;
  address->sin_port = htons ((u_short)port);
  address->sin_addr.s_addr = inet_addr (host);

  if (address->sin_addr.s_addr == INADDR_NONE)
    {
      struct hostent *ent;
      unsigned int ip;

      log_annoying ("set_address: gethostbyname (\"%s\")", host);
      ent = gethostbyname (host);
      log_annoying ("set_address: ent = %p", ent);
      if (ent == 0)
	return -1;

      memcpy(&address->sin_addr.s_addr, ent->h_addr, (unsigned)ent->h_length);
      ip = ntohl (address->sin_addr.s_addr);
      log_annoying ("set_address: host = %d.%d.%d.%d",
		     ntohl (ip) >> 24,
		    (ntohl (ip) >> 16) & 0xff,
		    (ntohl (ip) >>  8) & 0xff,
		     ntohl (ip)        & 0xff);
    }

  return 0;
}

int
open_device (char *device)
{
  struct termios t;
  int fd;

  fd = open (device, O_RDWR | O_NONBLOCK);
  if (fd == -1)
    return -1;
  
  if (tcgetattr (fd, &t) == -1)
    {
      if (errno == ENOTTY || errno == EINVAL)
	return fd;
      else
	return -1;
    }
  t.c_iflag = 0;
  t.c_oflag = 0;
  t.c_lflag = 0;
  if (tcsetattr (fd, TCSANOW, &t) == -1)
    return -1;

  return fd;
}

#ifdef DEBUG_MODE
void
dump_buf (FILE *f, unsigned char *buf, size_t len)
{
  const int N = 20;
  int i, j;

  for (i = 0; i < len;)
    {
      fputc ('[', f);
      for (j = 0; j < N && i + j < len; j++)
	fprintf (f, "%02x", buf[i + j]);
      for (; j < N; j++)
	fprintf (f, "  ");
      fputc (']', f);
      fputc ('[', f);
      for (j = 0; j < N && i + j < len; j++)
	{
	  int c = buf[i + j];
	  if (c < ' ' || c > 126)
	    fputc ('.', f);
	  else
	    fputc (c, f);
	}
      fputc (']', f);
      fputc ('\n', f);
      i += j;
    }
}
#endif

int
handle_device_input (Tunnel *tunnel, int fd, int events)
{
  unsigned char buf[10240];
  ssize_t n, m;

  if (events & POLLIN)
    {
      n = read (fd, buf, sizeof buf);
      if (n == 0 || n == -1)
	{
	  if (n == -1 && errno != EAGAIN)
	    log_error ("handle_device_input: read() error: %s",
		       strerror (errno));
	  return n;
	}

#ifdef DEBUG_MODE
      log_annoying ("read %d bytes from device:", n);
      if (debug_level >= 5)
	dump_buf (debug_file, buf, (size_t)n);
#endif

      m = tunnel_write (tunnel, buf, (size_t)n);
      log_annoying ("tunnel_write (%p, %p, %d) = %d", tunnel, buf, n, m);
      return m;
    }
  else if (events & POLLHUP)
    {
      log_error ("handle_device_input: POLLHUP");
      sleep (5);
    }
  else if (events & POLLERR)
    log_error ("handle_device_input: POLLERR");
  else if (events & POLLNVAL)
    log_error ("handle_device_input: POLLINVAL");
  else
    log_error ("handle_device_input: none of the above");

  errno = EIO;
  return -1;
}

int
handle_tunnel_input (Tunnel *tunnel, int fd, int events)
{
  unsigned char buf[10240];
  ssize_t n, m;

  if (events & POLLIN)
    {
      n = tunnel_read (tunnel, buf, sizeof buf);
      if (n <= 0)
	{
log_annoying ("handle_tunnel_input: tunnel_read() = %d\n", n);
	  if (n == -1 && errno != EAGAIN)
	    log_error ("handle_tunnel_input: tunnel_read() error: %s",
		       strerror (errno));
	  return n;
	}

#ifdef DEBUG_MODE
      log_annoying ("read %d bytes from tunnel:", n);
      if (debug_level >= 5)
	dump_buf (debug_file, buf, (size_t)n);
#endif

      /* If fd == 0, then we are using --stdin-stdout so write to stdout,
       * not fd. */
      m = write_all (fd ? fd : 1, buf, (size_t)n);
      log_annoying ("write_all (%d, %p, %d) = %d", fd ? fd : 1, buf, n, m);
      return m;
    }
  else if (events & POLLHUP)
    log_error ("handle_device_input: POLLHUP");
  else if (events & POLLERR)
    log_error ("handle_device_input: PULLERR");
  else if (events & POLLNVAL)
    log_error ("handle_device_input: PULLINVAL");
  else
    log_error ("handle_device_input: none of the above");

  errno = EIO;
  return -1;
}

void
name_and_port (const char *nameport, char **name, int *port)
{
  char *p;

  *name = strdup (nameport);
  if (*name == NULL)
    {
      fprintf (stderr, "Out of memory\n");
      exit (1);
    }

  p = strchr (*name, ':');
  if (p != NULL)
    {
      *port = atoi (p + 1);
      *p = '\0';
    }
}

int
atoi_with_postfix (const char *s_)
{
  char *s = strdup (s_);
  int n = strlen (s);
  int factor = 1;
  int x;

  if (s == NULL)
    {
      fprintf (stderr, "Out of memory\n");
      exit (1);
    }

  switch (s[n - 1])
    {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      break;
    case 'k':
    case 'K':
      factor = 1024;
      break;
    case 'M':
      factor = 1024 * 1024;
      break;
    case 'G':
      factor = 1024 * 1024 * 1024;
      break;
    default:
      fprintf (stderr, "Unknown postfix: %c\n", s[n - 1]);
      exit (1);
    }

  if (factor != 1)
    s[n - 1] = '\0';

  x = factor * atoi (s);
  free (s);
  return x;
}

#ifdef DEBUG_MODE
RETSIGTYPE
log_sigpipe (int sig)
{
  log_debug ("caught SIGPIPE");
  signal (SIGPIPE, log_sigpipe);
}
#endif
