/*
hts.c

Copyright (C) 1999, 2000 Lars Brinkhoff.  See COPYING for terms and conditions.

hts is the server half of httptunnel.  httptunnel creates a virtual
two-way data path tunneled in HTTP requests.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd_.h>
#include <signal.h>
#include <sys/poll_.h>
#include <sys/time.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

#include "common.h"

typedef struct
{
  char *me;
  char *device;
  const char *host;
  int port;
  char *forward_host;
  int forward_port;
  size_t content_length;
  char *pid_filename;
  int use_std;
  int use_daemon;
  int strict_content_length;
  int keep_alive;
  int max_connection_age;
  char *root;
  char *user;
} Arguments;

int debug_level = 0;
FILE *debug_file = NULL;

static void
usage (FILE *f, const char *me)
{
  fprintf (f,
"Usage: %s [OPTION]... [HOST:][PORT]\n"
"Listen for incoming httptunnel connections at PORT (default port is %d).\n"
"When a connection is made, I/O is redirected to the destination specified\n"
"by the --device, --forward-port or --stdin-stdout switch.\n"
"\n"
"  -c, --content-length BYTES     use HTTP PUT requests of BYTES size\n"
"                                 (k, M, and G postfixes recognized)\n"
"  -d, --device DEVICE            use DEVICE for input and output\n"
#ifdef DEBUG_MODE
"  -D, --debug [LEVEL]            enable debug mode\n"
#endif
"  -F, --forward-port HOST:PORT   connect to PORT at HOST and use it for \n"
"                                 input and output\n"
"  -h, --help                     display this help and exit\n"
"  -k, --keep-alive SECONDS       send keepalive bytes every SECONDS seconds\n"
"                                 (default is %d)\n"
#ifdef DEBUG_MODE
"  -l, --logfile FILE             specify logfile for debug output\n"
#endif
"  -M, --max-connection-age SEC   maximum time a connection will stay\n"
"                                 open is SEC seconds (default is %d)\n"
"  -r, --chroot ROOT              change root to ROOT\n"
"  -s, --stdin-stdout             use stdin/stdout for communication\n"
"                                 (implies --no-daemon)\n"
"  -S, --strict-content-length    always write Content-Length bytes in requests\n"
"  -u, --user USER                change user to USER\n"
"  -V, --version                  output version information and exit\n"
"  -w, --no-daemon                don't fork into the background\n"
"  -p, --pid-file LOCATION        write a PID file to LOCATION\n"
"\n"
"Report bugs to %s.\n",
	   me, DEFAULT_HOST_PORT, DEFAULT_KEEP_ALIVE,
	   DEFAULT_MAX_CONNECTION_AGE, BUG_REPORT_EMAIL);
}

static void
parse_arguments (int argc, char **argv, Arguments *arg)
{
  int c;

  /* defaults */

  arg->me = argv[0];
  arg->host = NULL;
  arg->port = DEFAULT_HOST_PORT;
  arg->device = NULL;
  arg->forward_host = NULL;
  arg->forward_port = -1;
  arg->content_length = DEFAULT_CONTENT_LENGTH;
  arg->pid_filename = NULL;
  arg->use_std = FALSE;
  arg->use_daemon = TRUE;
  arg->strict_content_length = FALSE;
  arg->keep_alive = DEFAULT_KEEP_ALIVE;
  arg->max_connection_age = DEFAULT_CONNECTION_MAX_TIME;
  arg->user = NULL;
  arg->root = NULL;
  
  for (;;)
    {
      int option_index = 0;
      static struct option long_options[] =
      {
	{ "help", no_argument, 0, 'h' },
	{ "version", no_argument, 0, 'V' },
	{ "no-daemon", no_argument, 0, 'w' },
	{ "user", required_argument, 0, 'u' },
#ifdef DEBUG_MODE
	{ "debug", required_argument, 0, 'D' },
	{ "logfile", required_argument, 0, 'l' },
#endif
	{ "chroot", required_argument, 0, 'r' },
	{ "stdin-stdout", no_argument, 0, 's' },
	{ "device", required_argument, 0, 'd' },
	{ "pid-file", required_argument, 0, 'p' },
	{ "keep-alive", required_argument, 0, 'k' },
	{ "forward-port", required_argument, 0, 'F' },
	{ "content-length", required_argument, 0, 'c' },
	{ "strict-content-length", no_argument, 0, 'S' },
	{ "max-connection-age", required_argument, 0, 'M' },
	{ 0, 0, 0, 0 }
      };

      static const char *short_options = "c:d:F:hk:M:p:sSVwu:r:"
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
	      log_exit (1);
	    }
	  break;
#endif /* DEBUG_MODE */

	case 'F':
	  name_and_port (optarg, &arg->forward_host, &arg->forward_port);
	  if (arg->forward_port == -1)
	    {
	      fprintf (stderr, "%s: you must specify a port number.\n"
		               "%s: try '%s --help' for help.\n",
		       arg->me, arg->me, arg->me);
	      exit (1);
	    }
	  break;

	case 'h':
	  usage (stdout, arg->me);
	  exit (0);

	case 'k':
	  arg->keep_alive = atoi (optarg);
	  break;

	case 'M':
	  arg->max_connection_age = atoi (optarg);
	  break;

	case 'r':
	  arg->root = optarg;
	  break;

	case 's':
	  arg->use_std=TRUE;
	  arg->use_daemon=FALSE;
	  break;

	case 'S':
	  arg->strict_content_length = TRUE;
	  break;

	case 'u':
	  arg->user = optarg;
	  break;

	case 'V':
	  printf ("hts (%s) %s\n", PACKAGE, VERSION);
	  exit (0);

	case 'p':
	  arg->pid_filename = optarg;
	  break;

	case 'w':
	  arg->use_daemon = FALSE;
	  break;

	case '?':
	  break;

	default:
	  fprintf (stderr, "?? getopt returned character code 0%o ??\n", c);
	}
    }

  if (argc - 1 == optind)
    {
      char *colon = strchr (argv[optind], ':');

      if (colon)
	{
	  *colon = '\0';
	  arg->host = argv[optind];
	  arg->port = atoi (colon + 1);
	}
      else
	arg->port = atoi (argv[optind]);
    }
  else if (argc - 1 > optind)
    {
      usage (stderr, arg->me);
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

  if (debug_level == 0 && debug_file != NULL)
    {
      fprintf (stderr, "%s: --logfile can't be used without debugging\n",
	       arg->me);
      exit (1);
    }

  /* Removed test ((arg->device == NULL) == (arg->forward_port == -1))
   * by Sampo Niskanen - those have been tested already! */
  if (arg->port == -1 ||
      ((arg->forward_host == NULL) != (arg->forward_port == -1)))
    {
      usage (stderr, arg->me);
      exit (1);
    }
}

int
main (int argc, char **argv)
{
  int closed;
  int fd = -1;
  Arguments arg;
  Tunnel *tunnel;
  FILE *pid_file;
  uid_t uid;
  gid_t gid;

  parse_arguments (argc, argv, &arg);

  if ((debug_level == 0 || debug_file != NULL) && arg.use_daemon)
    daemon (0, 1);

#ifdef DEBUG_MODE
  if (debug_level != 0 && debug_file == NULL)
    debug_file = stderr;
#else
  openlog ("hts", LOG_PID, LOG_DAEMON);
#endif

  log_notice ("hts (%s) %s started with arguments:", PACKAGE, VERSION);
  log_notice ("  me = %s", arg.me);
  log_notice ("  device = %s", arg.device ? arg.device : "(null)");
  if (arg.host)
    log_notice ("  port = %s:%d", arg.host, arg.port);
  else
    log_notice ("  port = %d", arg.port);
  log_notice ("  forward_port = %d", arg.forward_port);
  log_notice ("  forward_host = %s",
	      arg.forward_host ? arg.forward_host : "(null)");
  log_notice ("  content_length = %d", arg.content_length);
  log_notice ("  strict_content_length = %d", arg.strict_content_length);
  log_notice ("  use_std = %d", arg.use_std);
  log_notice ("  debug_level = %d", debug_level);
  log_notice ("  pid_filename = %s",
	      arg.pid_filename ? arg.pid_filename : "(null)");
  log_notice ("  chroot = %s", arg.root ? arg.root : "(null)");
  log_notice ("  user = %s", arg.user ? arg.user : "(null)");

  tunnel = tunnel_new_server (arg.host, arg.port, arg.content_length);
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
    log_debug ("tunnel_setopt max_connection_age error: %s", strerror (errno));

#ifdef DEBUG_MODE
  signal (SIGPIPE, log_sigpipe);
#else
  signal (SIGPIPE, SIG_IGN);
#endif

  if (arg.pid_filename != NULL)
    {
      pid_file = fopen (arg.pid_filename, "w+");
      if (pid_file == NULL)
        {
          fprintf (stderr, "Couldn't open pid file %s: %s\n",
		   arg.pid_filename, strerror (errno));
        }
      else
	{
          fprintf (pid_file, "%d\n", (int)getpid ());
	  if (fclose (pid_file))
            {
              fprintf (stderr, "Error closing pid file: %s\n", 
		       strerror (errno));
            }
         }
     }

  /* If requested to change user, get new uid and gid before chroot so we */
  /* don't need /etc/passwd & company in the chroot jail */
  if (arg.user)
    {
      struct passwd *pwd = getpwnam (arg.user);
      if (pwd == NULL)
        {
          log_error ("couldn't find user");
          log_exit (1);
	}
      uid = pwd->pw_uid;
      gid = pwd->pw_gid;
      if (setgroups (0, (const gid_t *)0) < 0)
        {
          log_error ("couldn't drop supplementary groups privileges");
          log_exit (1);
	}
      if (setgid (gid) < 0)
        {
          log_error ("couldn't change primary group");
          log_exit (1);
	}
      if (initgroups (arg.user, gid) < 0)
        {
	  /* non critical error */
          log_error ("couldn't add supplementary groups");
	}
    }

  /* Change root if requested */
  if (arg.root)
    {
      if (chroot (arg.root) < 0)
        {
          log_error ("couldn't change root");
          log_exit (1);
	}
      if (chdir ("/") < 0)
        {
          log_error ("couldn't change dir to new root");
          log_exit (1);
	}
      if (fclose (stdin) || fclose (stdout) || fclose (stderr))
        {
          log_error ("couldn't close stdin, stdout and/or stderr");
          log_exit (1);
	}
    }

  /* Change user if requested */
  if (arg.user)
    {
      if (setuid (uid) < 0)
        {
          log_error ("couldn't change user");
          log_exit (1);
	}
    }

  for (;;)
    {
      time_t last_tunnel_write;

      log_debug ("waiting for tunnel connection");

      if (arg.device != NULL)
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
	      log_notice ("changing fd from %d to 3", fd);
	      if (dup2 (fd, 3) != 3)
	        {
		  log_error ("couldn't dup2(%d, 3): %s", fd, strerror (errno));
		  log_exit (1);
		}
	    }
	} else if (arg.use_std) {
	  log_debug ("using stdin as fd");
	  fd = 0;
	  if (fcntl (fd, F_SETFL, O_NONBLOCK)==-1)
	    {
	      log_error ("couldn't set stdin to non-blocking mode: %s",
			 strerror (errno));
	      log_exit (1);
	    }
	  /* Usage of stdout (fd = 1) is checked later. */
	}

      if (tunnel_accept (tunnel) == -1)
	{
	  log_notice ("couldn't accept connection: %s", strerror (errno));
	  continue;
	}

      if (arg.forward_port != -1)
	{
	  struct sockaddr_in addr;

	  if (set_address (&addr, arg.forward_host, arg.forward_port) == -1)
	    {
	      log_error ("couldn't forward port to %s:%d: %s\n",
			 arg.forward_host, arg.forward_port, strerror (errno));
	      log_exit (1);
	    }

	  fd = do_connect (&addr);
	  log_debug ("do_connect (\"%s:%d\") = %d",
		 arg.forward_host, arg.forward_port, fd);
	  if (fd == -1)
	    {
	      log_error ("couldn't connect to %s:%d: %s\n",
			 arg.forward_host, arg.forward_port, strerror (errno));
	      log_exit (1);
	    }
	  /* Check that fd is not 0 (clash with --stdin-stdout) */
	  if (fd == 0)
	    {
	      log_notice ("changing fd from %d to 3", fd);
	      if (dup2 (fd, 3) != 3)
	        {
		  log_error ("couldn't dup2(%d, 3): %s", fd, strerror (errno));
		  log_exit (1);
		}
	    }
	}

      closed = FALSE;
      time (&last_tunnel_write);
      while (!closed)
	{
	  struct pollfd pollfd[2];
	  int timeout;
	  time_t t;
	  int n;
      
	  pollfd[0].fd = fd;
	  pollfd[0].events = POLLIN;
	  pollfd[1].fd = tunnel_pollin_fd (tunnel);
	  pollfd[1].events = POLLIN;

	  time (&t);
	  timeout = 1000 * (arg.keep_alive - (t - last_tunnel_write));
	  if (timeout < 0)
	    timeout = 0;

	  log_annoying ("poll () ...");
	  n = poll (pollfd, 2, timeout);
	  log_annoying ("... = %d", n);
	  if (n == -1)
	    {
	      log_error ("poll error: %s\n", strerror (errno));
	      log_exit (1);
	    }
	  else if (n == 0)
	    {
	      log_verbose ("poll() timed out");
	      tunnel_padding (tunnel, 1);
	      time (&last_tunnel_write);
	      continue;
	    }

	  log_annoying ("revents[0] = %x, revents[1] = %x, POLLIN = %x",
			pollfd[0].revents, pollfd[1].revents, POLLIN);
	  handle_input ("device or port", tunnel, fd, pollfd[0].revents,
			handle_device_input, &closed);
	  handle_input ("tunnel", tunnel, fd, pollfd[1].revents,
			handle_tunnel_input, &closed);

	  if (pollfd[0].revents & POLLIN)
	    time (&last_tunnel_write);
	}

      log_debug ("closing tunnel");
      if (fd != 0)
        {
          close (fd);
	}
      tunnel_close (tunnel);
      log_notice ("disconnected from FIXME:hostname:port");
    }

  log_debug ("destroying tunnel");
  tunnel_destroy (tunnel);
 
  log_exit (0);
}
