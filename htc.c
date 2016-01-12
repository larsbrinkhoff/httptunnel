/*
htc.c

Copyright (C) 1999, 2000 Lars Brinkhoff.  See COPYING for terms and conditions.

htc is the client half of httptunnel.  httptunnel creates a virtual
two-way data path tunneled in HTTP requests.
*/

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd_.h>
#include <signal.h>
#include <sys/poll_.h>
#include <sys/time.h>
#include <sys/stat.h>

#include "common.h"
#include "base64.h"

#define DEFAULT_PROXY_PORT 8080
#define DEFAULT_PROXY_BUFFER_TIMEOUT 500 /* milliseconds */

typedef struct
{
  char *me;
  char *device;
  char *host_name;
  int host_port;
  char *proxy_name;
  int proxy_port;
  size_t proxy_buffer_size;
  int proxy_buffer_timeout;
  size_t content_length;
  int forward_port;
  int use_std;
  int use_daemon;
  int strict_content_length;
  int keep_alive;
  int max_connection_age;
  char *proxy_authorization;
  char *user_agent;
} Arguments;

#define NO_PROXY_BUFFER 0
#define NO_PROXY (NULL)

int debug_level = 0;
FILE *debug_file = NULL;

static void
usage (FILE *f, const char *me)
{
  fprintf (f,
"Usage: %s [OPTION]... HOST[:PORT]\n"
"Set up a httptunnel connection to PORT at HOST (default port is %d).\n"
"When a connection is made, I/O is redirected from the source specified\n"
"by the --device, --forward-port or --stdin-stdout switch to the tunnel.\n"
"\n"
"  -A, --proxy-authorization USER:PASSWORD  proxy authorization\n"
"  -z, --proxy-authorization-file FILE      proxy authorization file\n"
"  -B, --proxy-buffer-size BYTES  assume a proxy buffer size of BYTES bytes\n"
"                                 (k, M, and G postfixes recognized)\n"
"  -c, --content-length BYTES     use HTTP PUT requests of BYTES size\n"
"                                 (k, M, and G postfixes recognized)\n"
"  -d, --device DEVICE            use DEVICE for input and output\n"
#ifdef DEBUG_MODE
"  -D, --debug [LEVEL]            enable debugging mode\n"
#endif
"  -F, --forward-port PORT        use TCP port PORT for input and output\n"
"  -h, --help                     display this help and exit\n"
"  -k, --keep-alive SECONDS       send keepalive bytes every SECONDS seconds\n"
"                                 (default is %d)\n"
#ifdef DEBUG_MODE
"  -l, --logfile FILE             specify file for debugging output\n"
#endif
"  -M, --max-connection-age SEC   maximum time a connection will stay\n"
"                                 open is SEC seconds (default is %d)\n"
"  -P, --proxy HOSTNAME[:PORT]    use a HTTP proxy (default port is %d)\n"
"  -s, --stdin-stdout             use stdin/stdout for communication\n"
"                                 (implies --no-daemon)\n"
"  -S, --strict-content-length    always write Content-Length bytes in requests\n"
"  -T, --timeout TIME             timeout, in milliseconds, before sending\n"
"                                 padding to a buffering proxy\n"
"  -U, --user-agent STRING        specify User-Agent value in HTTP requests\n"
"  -V, --version                  output version information and exit\n"
"  -w, --no-daemon                don't fork into the background\n"
"\n"
"Report bugs to %s.\n",
	   me, DEFAULT_HOST_PORT, DEFAULT_KEEP_ALIVE,
	   DEFAULT_MAX_CONNECTION_AGE, DEFAULT_PROXY_PORT,
	   BUG_REPORT_EMAIL);
}

static int
wait_for_connection_on_socket (int s)
{
  struct sockaddr addr;
  socklen_t len;
  int t;

  len = sizeof addr;
  t = accept (s, &addr, &len);
  if (t == -1)
    return -1;

  return t;
}

static void
parse_arguments (int argc, char **argv, Arguments *arg)
{
  int c;

  /* defaults */

  arg->me = argv[0];
  arg->device = NULL;
  arg->forward_port = -1;
  arg->host_name = NULL;
  arg->host_port = DEFAULT_HOST_PORT;
  arg->proxy_name = NO_PROXY;
  arg->proxy_port = DEFAULT_PROXY_PORT;
  arg->proxy_buffer_size = NO_PROXY_BUFFER;
  arg->proxy_buffer_timeout = -1;
  arg->content_length = DEFAULT_CONTENT_LENGTH;
  arg->use_std = FALSE;
  arg->use_daemon = TRUE;
  arg->strict_content_length = FALSE;
  arg->keep_alive = DEFAULT_KEEP_ALIVE;
  arg->max_connection_age = DEFAULT_CONNECTION_MAX_TIME;
  arg->proxy_authorization = NULL;
  arg->user_agent = NULL;

  for (;;)
    {
      int option_index = 0;
      static struct option long_options[] =
      {
	{ "help", no_argument, 0, 'h' },
	{ "version", no_argument, 0, 'V' },
	{ "no-daemon", no_argument, 0, 'w' },
	{ "stdin-stdout", no_argument, 0, 's' },
#ifdef DEBUG_MODE
	{ "debug", required_argument, 0, 'D' },
	{ "logfile", required_argument, 0, 'l' },
#endif
	{ "proxy", required_argument, 0, 'P' },
	{ "device", required_argument, 0, 'd' },
	{ "timeout", required_argument, 0, 'T' },
	{ "keep-alive", required_argument, 0, 'k' },
	{ "user-agent", required_argument, 0, 'U' },
	{ "forward-port", required_argument, 0, 'F' },
	{ "content-length", required_argument, 0, 'c' },
	{ "strict-content-length", no_argument, 0, 'S' },
	{ "proxy-buffer-size", required_argument, 0, 'B' },
	{ "proxy-authorization", required_argument, 0, 'A' },
	{ "max-connection-age", required_argument, 0, 'M' },
	{ "proxy-authorization-file", required_argument, 0, 'z' },
	{ 0, 0, 0, 0 }
      };

      static const char *short_options = "A:B:c:d:F:hk:M:P:sST:U:Vwz:"
#ifdef DEBUG_MODE
	"D:l:"
#endif
	;

      c = getopt_long (argc, argv, short_options,
		       long_options, &option_index);
      if (c == -1)
	break;

      switch (c)
	{
	case 0:
	  fprintf (stderr, "option %s", long_options[option_index].name);
	  if (optarg)
	    fprintf (stderr, " with arg %s", optarg);
	  fprintf (stderr, "\n");
	  break;

	case 'A':
	  arg->proxy_authorization = optarg;
	  break;

	case 'B':
	  arg->proxy_buffer_size = atoi_with_postfix (optarg);
	  break;

	case 'c':
	  arg->content_length = atoi_with_postfix (optarg);
	  break;

	case 'd':
	  arg->device = optarg;
	  break;

#ifdef DEBUG_MODE
	case 'D':
	  if (optarg)
	    debug_level = atoi (optarg);
	  else
	    debug_level = 1;
	  break;

	case 'l':
	  debug_file = fopen (optarg, "w");
	  if (debug_file == NULL)
	    {
	      fprintf (stderr, "%s: couldn't open file %s for writing\n",
		       arg->me, optarg);
	      exit (1);
	    }
	  break;
#endif

	case 'F':
	  arg->forward_port = atoi (optarg);
	  break;

	case 'k':
	  arg->keep_alive = atoi (optarg);
	  break;

	case 'M':
	  arg->max_connection_age = atoi (optarg);
	  break;

	case 'h':
	  usage (stdout, arg->me);
	  exit (0);

	case 'P':
	  name_and_port (optarg, &arg->proxy_name, &arg->proxy_port);
	  if (arg->proxy_port == -1)
	    arg->proxy_port = DEFAULT_PROXY_PORT;
	  if (arg->proxy_buffer_timeout == -1)
	    arg->proxy_buffer_timeout = DEFAULT_PROXY_BUFFER_TIMEOUT;
	  break;

	case 's':
	  arg->use_std=TRUE;
	  arg->use_daemon=FALSE;
	  break;

	case 'S':
	  arg->strict_content_length = TRUE;
	  break;

	case 'T':
	  arg->proxy_buffer_timeout = atoi (optarg);
	  break;

	case 'U':
	  arg->user_agent = optarg;
	  break;

	case 'V':
	  printf ("htc (%s) %s\n", PACKAGE, VERSION);
	  exit (0);

	case 'w':
	  arg->use_daemon=FALSE;
	  break;

	case 'z':
	  {
	    struct stat s;
	    char *auth;
	    int f;

	    f = open (optarg, O_RDONLY);
	    if (f == -1)
	      {
		fprintf (stderr, "couldn't open %s: %s\n", optarg, strerror (errno));
		exit (1);
	      }

	    if (fstat (f, &s) == -1)
	      {
		fprintf (stderr, "error fstating %s: %s\n", optarg, strerror (errno));
		exit (1);
	      }

	    auth = malloc (s.st_size + 1);
	    if (auth == NULL)
	      {
		fprintf (stderr, "out of memory whilst allocating "
			"authentication string\n");
		exit (1);
	      }

	    if (read_all (f, auth, s.st_size) == -1)
	      {
		fprintf (stderr, "error reading %s: %s\n", optarg, strerror (errno));
		exit (1);
	      }

	    /*
	     * If file ends with a "\r\n" or "\n", chop them off.
	     */
	    if (s.st_size >= 1 && auth[s.st_size - 1] == '\n')
	      {
		s.st_size -=
		  (s.st_size >= 2 && auth[s.st_size - 2] == '\r') ? 2 : 1;
	      }

	    auth[s.st_size] = 0;
	    arg->proxy_authorization = auth;
	  }
	  break;

	case '?':
	  break;

	default:
	  fprintf (stderr, "?? getopt returned character code 0%o ??\n", c);
	}
    }

  if (optind == argc - 1)
    {
      name_and_port (argv[optind], &arg->host_name, &arg->host_port);
      if (arg->host_port == -1)
	arg->host_port = DEFAULT_HOST_PORT;
    }
  else
    {
      fprintf (stderr, "%s: the destination of the tunnel must be specified.\n"
	               "%s: try '%s --help' for help.\n",
	       arg->me, arg->me, arg->me);
      exit (1);
    }

  if (arg->device == NULL && arg->forward_port == -1 && !arg->use_std)
    {
      fprintf (stderr, "%s: one of --device, --forward-port or --stdin-stdout must be used.\n"
	               "%s: try '%s -help' for help.\n",
	       arg->me, arg->me, arg->me);
      exit (1);
    }

  if ((arg->device != NULL && arg->forward_port != -1) ||
      (arg->device != NULL && arg->use_std) ||
      (arg->forward_port != -1 && arg->use_std))
    {
      fprintf (stderr, "%s: only one of --device, --forward-port or --stdin-stdout can be used.\n"
	               "%s: try '%s --help' for help.\n",
	       arg->me, arg->me, arg->me);
      exit (1);
    }

  /* Removed test ((arg->device == NULL) == (arg->forward_port == -1))
   * by Sampo Niskanen - those have been tested already! */
  if (arg->host_name == NULL ||
      arg->host_port == -1 ||
      (arg->proxy_name != NO_PROXY && arg->proxy_port == -1))
    {
      usage (stderr, arg->me);
      exit (1);
    }

  if (debug_level == 0 && debug_file != NULL)
    {
      fprintf (stderr, "%s: --logfile can't be used without debugging\n",
	       arg->me);
      exit (1);
    }

  if (arg->proxy_name == NO_PROXY)
    {
      if (arg->proxy_buffer_size != NO_PROXY_BUFFER)
	{
	  fprintf (stderr, "%s: warning: --proxy-buffer-size can't be "
		   "used without --proxy\n", arg->me);
	  arg->proxy_buffer_size = NO_PROXY_BUFFER;
	}

      if (arg->proxy_buffer_timeout != -1)
	{
	  fprintf (stderr, "%s: warning: --proxy-buffer-timeout can't be "
		   "used without --proxy\n", arg->me);
	  arg->proxy_buffer_timeout = -1;
	}

      if (arg->proxy_authorization != NULL)
	{
	  fprintf (stderr, "%s: warning: --proxy-authorization can't be "
		   "used without --proxy\n", arg->me);
	  arg->proxy_authorization = NULL;
	}
    }
  else if (arg->proxy_buffer_size == NO_PROXY_BUFFER)
    arg->proxy_buffer_timeout = -1;
}

int
main (int argc, char **argv)
{
  int s = -1;
  int fd = -1;
  Arguments arg;
  Tunnel *tunnel;
  int closed;

  parse_arguments (argc, argv, &arg);

  if ((debug_level == 0 || debug_file != NULL) && arg.use_daemon)
    daemon (0, 1);

#ifdef DEBUG_MODE
  if (debug_level != 0 && debug_file == NULL)
    debug_file = stderr;
#else
  openlog ("htc", LOG_PID, LOG_DAEMON);
#endif

  log_notice ("htc (%s) %s started with arguments:", PACKAGE, VERSION);
  log_notice ("  me = %s", arg.me);
  log_notice ("  device = %s", arg.device ? arg.device : "(null)");
  log_notice ("  host_name = %s", arg.host_name ? arg.host_name : "(null)");
  log_notice ("  host_port = %d", arg.host_port);
  log_notice ("  proxy_name = %s", arg.proxy_name ? arg.proxy_name : "(null)");
  log_notice ("  proxy_port = %d", arg.proxy_port);
  log_notice ("  proxy_buffer_size = %d", arg.proxy_buffer_size);
  log_notice ("  proxy_buffer_timeout = %d", arg.proxy_buffer_timeout);
  log_notice ("  content_length = %d", arg.content_length);
  log_notice ("  forward_port = %d", arg.forward_port);
  log_notice ("  max_connection_age = %d", arg.max_connection_age);
  log_notice ("  use_std = %d", arg.use_std);
  log_notice ("  strict_content_length = %d", arg.strict_content_length);
  log_notice ("  keep_alive = %d", arg.keep_alive);
  log_notice ("  proxy_authorization = %s",
	      arg.proxy_authorization ? arg.proxy_authorization : "(null)");
  log_notice ("  user_agent = %s", arg.user_agent ? arg.user_agent : "(null)");
  log_notice ("  debug_level = %d", debug_level);


  if (arg.forward_port != -1)
    {
      struct in_addr addr;

      addr.s_addr = INADDR_ANY;
      s = server_socket (addr, arg.forward_port, 0);
      log_debug ("server_socket (%d) = %d", arg.forward_port, s);
      if (s == -1)
	{
	  log_error ("couldn't create server socket: %s", strerror (errno));
	  log_exit (1);
	}
    }

#ifdef DEBUG_MODE
  signal (SIGPIPE, log_sigpipe);
#else
  signal (SIGPIPE, SIG_IGN);
#endif

  for (;;)
    {
      time_t last_tunnel_write;

      if (arg.device)
	{
	  fd = open_device (arg.device);
	  log_debug ("open_device (\"%s\") = %d", arg.device, fd);
	  if (fd == -1)
	    {
	      log_error ("couldn't open %s: %s",
			 arg.device, strerror (errno));
	      log_exit (1);
	    }
	  /* Check that fd is not 0 (clash with --stdin-stdout) */
	  if (fd == 0)
	    {
	      log_notice("changing fd from %d to 3",fd);
	      if (dup2 (fd, 3) != 3)
	        {
		  log_error ("couldn't dup2 (%d, 3): %s",fd,strerror(errno));
		  log_exit (1);
		}
	    }
	}
      else if (arg.forward_port != -1)
	{
	  log_debug ("waiting for connection on port %d", arg.forward_port);
	  fd = wait_for_connection_on_socket (s);
	  log_debug ("wait_for_connection_on_socket (%d) = %d", s, fd);
	  if (fd == -1)
	    {
	      log_error ("couldn't forward port %d: %s",
			 arg.forward_port, strerror (errno));
	      log_exit (1);
	    }
	  /* Check that fd is not 0 (clash with --stdin-stdout) */
	  if (fd == 0)
	    {
	      log_notice ("changing fd from %d to 3",fd);
	      if (dup2 (fd, 3) != 3)
	        {
		  log_error ("couldn't dup2 (%d, 3): %s",fd,strerror(errno));
		  log_exit (1);
		}
	    }
	} else if (arg.use_std) {
	  log_debug ("using stdin as fd");
	  fd = 0;
	  if (fcntl(fd,F_SETFL,O_NONBLOCK)==-1)
	    {
	      log_error ("couldn't set stdin to non-blocking mode: %s",
			 strerror(errno));
	      log_exit (1);
	    }
	  /* Usage of stdout (fd = 1) is checked later. */
	}

      log_debug ("creating a new tunnel");
      tunnel = tunnel_new_client (arg.host_name, arg.host_port,
				  arg.proxy_name, arg.proxy_port,
				  arg.content_length);
      if (tunnel == NULL)
	{
	  log_error ("couldn't create tunnel");
	  log_exit (1);
	}

      if (tunnel_setopt (tunnel, "strict_content_length",
			 &arg.strict_content_length) == -1)
	log_debug ("tunnel_setopt strict_content_length error: %s",
		   strerror (errno));

      if (tunnel_setopt (tunnel, "keep_alive",
			 &arg.keep_alive) == -1)
	log_debug ("tunnel_setopt keep_alive error: %s", strerror (errno));

      if (tunnel_setopt (tunnel, "max_connection_age",
			 &arg.max_connection_age) == -1)
	log_debug ("tunnel_setopt max_connection_age error: %s",
		   strerror (errno));

      if (arg.proxy_authorization != NULL)
	{
	  ssize_t len;
	  char *auth;

	  len = encode_base64 (arg.proxy_authorization,
			       strlen (arg.proxy_authorization),
			       &auth);
	  if (len == -1)
	    {
	      log_error ("encode_base64 error: %s", strerror (errno));
	    }
	  else
	    {
	      char *str = malloc (len + 7);

	      if (str == NULL)
		{
		  log_error ("out of memory when encoding "
			     "authorization string");
		  log_exit (1);
		}

	      strcpy (str, "Basic ");
	      strcat (str, auth);
	      free (auth);
	
	      if (tunnel_setopt (tunnel, "proxy_authorization", str) == -1)
		log_error ("tunnel_setopt proxy_authorization error: %s",
			   strerror (errno));

	      free (str);
	    }
	}

      if (arg.user_agent != NULL)
	{
	  if (tunnel_setopt (tunnel, "user_agent", arg.user_agent) == -1)
	    log_error ("tunnel_setopt user_agent error: %s",
		       strerror (errno));
	}

      if (tunnel_connect (tunnel) == -1)
	{
	  log_error ("couldn't open tunnel: %s", strerror (errno));
	  log_exit (1);
	}
      if (arg.proxy_name)
	log_notice ("connected to %s:%d via %s:%d",
		    arg.host_name, arg.host_port,
		    arg.proxy_name, arg.proxy_port);
      else
	log_notice ("connected to %s:%d", arg.host_name, arg.host_port);

      closed = FALSE;
      time (&last_tunnel_write);
      while (!closed)
	{
	  struct pollfd pollfd[2];
	  int keep_alive_timeout;
	  int timeout;
	  time_t t;
	  int n;

	  pollfd[0].fd = fd;
	  pollfd[0].events = POLLIN;
	  pollfd[1].fd = tunnel_pollin_fd (tunnel);
	  pollfd[1].events = POLLIN;
      
	  time (&t);
	  timeout = 1000 * (arg.keep_alive - (t - last_tunnel_write));
	  keep_alive_timeout = TRUE;
	  if (timeout < 0)
	    timeout = 0;
	  if (arg.proxy_buffer_timeout != -1 &&
	      arg.proxy_buffer_timeout < timeout)
	    {
	      timeout = arg.proxy_buffer_timeout;
	      keep_alive_timeout = FALSE;
	    }

	  log_annoying ("poll () ...");
	  n = poll (pollfd, 2, timeout);
	  log_annoying ("... = %d", n);
	  if (n == -1)
	    {
	      log_error ("poll error: %s", strerror (errno));
	      log_exit (1);
	    }
	  else if (n == 0)
	    {
	      log_verbose ("poll() timed out");
	      if (keep_alive_timeout)
		{
		  tunnel_padding (tunnel, 1);
		  time (&last_tunnel_write);
		}
	      else
		{
		  if (tunnel_maybe_pad (tunnel, arg.proxy_buffer_size) > 0)
		    time (&last_tunnel_write);
		}
	      continue;
	    }
      
	  handle_input ("device or port", tunnel, fd, pollfd[0].revents,
			handle_device_input, &closed);
	  handle_input ("tunnel", tunnel, fd, pollfd[1].revents,
			handle_tunnel_input, &closed);

	  if (pollfd[0].revents & POLLIN)
	    time (&last_tunnel_write);
	}

      log_debug ("destroying tunnel");
      if (fd != 0)
        {
          close (fd);
	}
      tunnel_destroy (tunnel);
      if (arg.proxy_name)
	log_notice ("disconnected from %s:%d via %s:%d",
		    arg.host_name, arg.host_port,
		    arg.proxy_name, arg.proxy_port);
      else
	log_notice ("disconnected from %s%d", arg.host_name, arg.host_port);
    }

  log_debug ("closing server socket");
  close (s);

  log_exit (0);
}
