/*
http.c
Copyright (C) 1999 Lars Brinkhoff.  See COPYING for terms and conditions.

bug alert: parse_header() doesn't handle header fields that are extended
over multiple lines.
*/

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http.h"
#include "common.h"

static inline ssize_t
http_method (int fd, Http_destination *dest,
	     Http_method method, ssize_t length)
{
  char str[1024]; /* FIXME: possible buffer overflow */
  Http_request *request;
  ssize_t n;

  if (fd == -1)
    {
      log_error ("http_method: fd == -1");
      return -1;
    }

  n = 0;
  if (dest->proxy_name != NULL)
    n = sprintf (str, "http://%s:%d", dest->host_name, dest->host_port);
  sprintf (str + n, "/index.html?crap=%ld", time (NULL));

  request = http_create_request (method, str, 1, 1);
  if (request == NULL)
    return -1;

  sprintf (str, "%s:%d", dest->host_name, dest->host_port);
  http_add_header (&request->header, "Host", str);

  if (length >= 0)
    {
      sprintf (str, "%ld", length);
      http_add_header (&request->header, "Content-Length", str);
    }

  http_add_header (&request->header, "Connection", "close");

  if (dest->proxy_authorization)
    http_add_header (&request->header, "Proxy-Authorization",
		     dest->proxy_authorization);

  if (dest->user_agent)
    http_add_header (&request->header, "User-Agent", dest->user_agent);

  n = http_write_request (fd, request);
  http_destroy_request (request);
  return n;
}

ssize_t
http_get (int fd, Http_destination *dest)
{
  return http_method (fd, dest, HTTP_GET, -1);
}

ssize_t
http_put (int fd, Http_destination *dest, size_t length)
{
  return http_method (fd, dest, HTTP_PUT, (ssize_t)length);
}

ssize_t
http_post (int fd, Http_destination *dest, size_t length)
{
  return http_method (fd, dest, HTTP_POST, (ssize_t)length);
}

int
http_error_to_errno (int err)
{
  /* Error codes taken from RFC2068. */
  switch (err)
    {
    case -1: /* system error */
      return errno;
    case -200: /* OK */
    case -201: /* Created */
    case -202: /* Accepted */
    case -203: /* Non-Authoritative Information */
    case -204: /* No Content */
    case -205: /* Reset Content */
    case -206: /* Partial Content */
      return 0;
    case -400: /* Bad Request */
      log_error ("http_error_to_errno: 400 bad request");
      return EIO;
    case -401: /* Unauthorized */
      log_error ("http_error_to_errno: 401 unauthorized");
      return EACCES;
    case -403: /* Forbidden */
      log_error ("http_error_to_errno: 403 forbidden");
      return EACCES;
    case -404: /* Not Found */
      log_error ("http_error_to_errno: 404 not found");
      return ENOENT;
    case -411: /* Length Required */
      log_error ("http_error_to_errno: 411 length required");
      return EIO;
    case -413: /* Request Entity Too Large */
      log_error ("http_error_to_errno: 413 request entity too large");
      return EIO;
    case -505: /* HTTP Version Not Supported       */
      log_error ("http_error_to_errno: 413 HTTP version not supported");
      return EIO;
    case -100: /* Continue */
    case -101: /* Switching Protocols */
    case -300: /* Multiple Choices */
    case -301: /* Moved Permanently */
    case -302: /* Moved Temporarily */
    case -303: /* See Other */
    case -304: /* Not Modified */
    case -305: /* Use Proxy */ 
    case -402: /* Payment Required */
    case -405: /* Method Not Allowed */
    case -406: /* Not Acceptable */
    case -407: /* Proxy Autentication Required */
    case -408: /* Request Timeout */
    case -409: /* Conflict */
    case -410: /* Gone */
    case -412: /* Precondition Failed */
    case -414: /* Request-URI Too Long */
    case -415: /* Unsupported Media Type */
    case -500: /* Internal Server Error */
    case -501: /* Not Implemented */
    case -502: /* Bad Gateway */
    case -503: /* Service Unavailable */
    case -504: /* Gateway Timeout */
      log_error ("http_error_to_errno: HTTP error %d", err);
      return EIO;
    default:
      log_error ("http_error_to_errno: unknown error %d", err);
      return EIO;
    }
}

static Http_method
http_string_to_method (const char *method, size_t n)
{
  if (strncmp (method, "GET", n) == 0)
    return HTTP_GET;
  if (strncmp (method, "PUT", n) == 0)
    return HTTP_PUT;
  if (strncmp (method, "POST", n) == 0)
    return HTTP_POST;
  if (strncmp (method, "OPTIONS", n) == 0)
    return HTTP_OPTIONS;
  if (strncmp (method, "HEAD", n) == 0)
    return HTTP_HEAD;
  if (strncmp (method, "DELETE", n) == 0)
    return HTTP_DELETE;
  if (strncmp (method, "TRACE", n) == 0)
    return HTTP_TRACE;
  return -1;
}

static const char *
http_method_to_string (Http_method method)
{
  switch (method)
    {
    case HTTP_GET: return "GET";
    case HTTP_PUT: return "PUT";
    case HTTP_POST: return "POST";
    case HTTP_OPTIONS: return "OPTIONS";
    case HTTP_HEAD: return "HEAD";
    case HTTP_DELETE: return "DELETE";
    case HTTP_TRACE: return "TRACE";
    }
  return "(uknown)";
}

static ssize_t
read_until (int fd, int ch, char **data)
{
  char *buf, *buf2;
  ssize_t n, len, buf_size;

  *data = NULL;

  buf_size = 100;
  buf = malloc (buf_size);
  if (buf == NULL)
    {
      log_error ("read_until: out of memory");
      return -1;
    }

  len = 0;
  while ((n = read_all (fd, buf + len, 1)) == 1)
    {
      if (buf[len++] == ch)
	break;
      if (len + 1 == buf_size)
	{
	  buf_size *= 2;
	  buf2 = realloc (buf, buf_size);
	  if (buf2 == NULL)
	    {
	      log_error ("read_until: realloc failed");
	      free (buf);
	      return -1;
	    }
	  buf = buf2;
	}
    }
  if (n <= 0)
    {
      free (buf);
      if (n == 0)
	log_error ("read_until: closed");
      else
	log_error ("read_until: read error: %s", strerror (errno));
      return n;
    }

  /* Shrink to minimum size + 1 in case someone wants to add a NUL. */
  buf2 = realloc (buf, len + 1);
  if (buf2 == NULL)
    log_error ("read_until: realloc: shrink failed"); /* not fatal */
  else
    buf = buf2;

  *data = buf;
  return len;
}

static inline Http_header *
http_alloc_header (const char *name, const char *value)
{
  Http_header *header;

  header = malloc (sizeof (Http_header));
  if (header == NULL)
    return NULL;

  header->name = header->value = NULL;
  header->name = strdup (name);
  header->value = strdup (value);
  if (name == NULL || value == NULL)
    {
      if (name == NULL)
	free ((char *)name);
      if (value == NULL)
	free ((char *)value);
      free (header);
      return NULL;
    }

  return header;
}

Http_header *
http_add_header (Http_header **header, const char *name, const char *value)
{
  Http_header *new_header;

  new_header = http_alloc_header (name, value);
  if (new_header == NULL)
    return NULL;

  new_header->next = NULL;
  while (*header)
    header = &(*header)->next;
  *header = new_header;

  return new_header;
}

static ssize_t
parse_header (int fd, Http_header **header)
{
  unsigned char buf[2];
  char *data;
  Http_header *h;
  size_t len;
  ssize_t n;

  *header = NULL;

  n = read_all (fd, buf, 2);
  if (n <= 0)
    return n;
  if (buf[0] == '\r' && buf[1] == '\n')
    return n;

  h = malloc (sizeof (Http_header));
  if (h == NULL)
    {
      log_error ("parse_header: malloc failed");
      return -1;
    }
  *header = h;
  h->name = NULL;
  h->value = NULL;

  n = read_until (fd, ':', &data);
  if (n <= 0)
    return n;
  data = realloc (data, n + 2);
  if (data == NULL)
    {
      log_error ("parse_header: realloc failed");
      return -1;
    }
  memmove (data + 2, data, n);
  memcpy (data, buf, 2);
  n += 2;
  data[n - 1] = 0;
  h->name = data;
  len = n;

  n = read_until (fd, '\r', &data);
  if (n <= 0)
    return n;
  data[n - 1] = 0;
  h->value = data;
  len += n;

  n = read_until (fd, '\n', &data);
  if (n <= 0)
    return n;
  free (data);
  if (n != 1)
    {
      log_error ("parse_header: invalid line ending");
      return -1;
    }
  len += n;

  log_verbose ("parse_header: %s:%s", h->name, h->value);

  n = parse_header (fd, &h->next);
  if (n <= 0)
    return n;
  len += n;

  return len;
}

static ssize_t
http_write_header (int fd, Http_header *header)
{
  ssize_t n = 0, m;

  if (header == NULL)
    return write_all (fd, "\r\n", 2);

  m = write_all (fd, (void *)header->name, strlen (header->name));
  if (m == -1)
    {
      return -1;
    }
  n += m;

  m = write_all (fd, ": ", 2);
  if (m == -1)
    {
      return -1;
    }
  n += m;

  m = write_all (fd, (void *)header->value, strlen (header->value));
  if (m == -1)
    {
      return -1;
    }
  n += m;

  m = write_all (fd, "\r\n", 2);
  if (m == -1)
    {
      return -1;
    }
  n += m;

  m = http_write_header (fd, header->next);
  if (m == -1)
    {
      return -1;
    }
  n += m;

  return n;
}

static void
http_destroy_header (Http_header *header)
{
  if (header == NULL)
    return;

  http_destroy_header (header->next);

  if (header->name)
    free ((char *)header->name);
  if (header->value)
    free ((char *)header->value);
  free (header);
}

static inline Http_response *
http_allocate_response (const char *status_message)
{
  Http_response *response;

  response = malloc (sizeof (Http_response));
  if (response == NULL)
    return NULL;

  response->status_message = strdup (status_message);
  if (response->status_message == NULL)
    {
      free (response);
      return NULL;
    }

  return response;
}

Http_response *
http_create_response (int major_version,
		     int minor_version,
		     int status_code,
		     const char *status_message)
{
  Http_response *response;

  response = http_allocate_response (status_message);
  if (response == NULL)
    return NULL;

  response->major_version = major_version;
  response->minor_version = minor_version;
  response->status_code = status_code;
  response->header = NULL;

  return response;
}

ssize_t
http_parse_response (int fd, Http_response **response_)
{
  Http_response *response;
  char *data;
  size_t len;
  ssize_t n;

  *response_ = NULL;

  response = malloc (sizeof (Http_response));
  if (response == NULL)
    {
      log_error ("http_parse_response: out of memory");
      return -1;
    }

  response->major_version = -1;
  response->minor_version = -1;
  response->status_code = -1;
  response->status_message = NULL;
  response->header = NULL;

  n = read_until (fd, '/', &data);
  if (n <= 0)
    {
      free (response);
      return n;
    }
  else if (n != 5 || memcmp (data, "HTTP", 4) != 0)
    {
      log_error ("http_parse_response: expected \"HTTP\"");
      free (data);
      free (response);
      return -1;
    }
  free (data);
  len = n;

  n = read_until (fd, '.', &data);
  if (n <= 0)
    {
      free (response);
      return n;
    }
  data[n - 1] = 0;
  response->major_version = atoi (data);
  log_verbose ("http_parse_response: major version = %d",
	       response->major_version);
  free (data);
  len += n;

  n = read_until (fd, ' ', &data);
  if (n <= 0)
    {
      free (response);
      return n;
    }
  data[n - 1] = 0;
  response->minor_version = atoi (data);
  log_verbose ("http_parse_response: minor version = %d",
	       response->minor_version);
  free (data);
  len += n;

  n = read_until (fd, ' ', &data);
  if (n <= 0)
    {
      free (response);
      return n;
    }
  data[n - 1] = 0;
  response->status_code = atoi (data);
  log_verbose ("http_parse_response: status code = %d",
	       response->status_code);
  free (data);
  len += n;

  n = read_until (fd, '\r', &data);
  if (n <= 0)
    {
      free (response);
      return n;
    }
  data[n - 1] = 0;
  response->status_message = data;
  log_verbose ("http_parse_response: status message = \"%s\"",
	       response->status_message);
  len += n;

  n = read_until (fd, '\n', &data);
  if (n <= 0)
    {
      http_destroy_response (response);
      return n;
    }
  free (data);
  if (n != 1)
    {
      log_error ("http_parse_request: invalid line ending");
      http_destroy_response (response);
      return -1;
    }
  len += n;

  n = parse_header (fd, &response->header);
  if (n <= 0)
    {
      http_destroy_response (response);
      return n;
    }
  len += n;

  *response_ = response;
  return len;
}

void
http_destroy_response (Http_response *response)
{
  if (response->status_message)
    free ((char *)response->status_message);
  http_destroy_header (response->header);
  free (response);
}

static inline Http_request *
http_allocate_request (const char *uri)
{
  Http_request *request;

  request = malloc (sizeof (Http_request));
  if (request == NULL)
    return NULL;

  request->uri = strdup (uri);
  if (request->uri == NULL)
    {
      free (request);
      return NULL;
    }

  return request;
}

Http_request *
http_create_request (Http_method method,
		     const char *uri,
		     int major_version,
		     int minor_version)
{
  Http_request *request;

  request = http_allocate_request (uri);
  if (request == NULL)
    return NULL;

  request->method = method;
  request->major_version = major_version;
  request->minor_version = minor_version;
  request->header = NULL;

  return request;
}

ssize_t
http_parse_request (int fd, Http_request **request_)
{
  Http_request *request;
  char *data;
  size_t len;
  ssize_t n;

  *request_ = NULL;

  request = malloc (sizeof (Http_request));
  if (request == NULL)
    {
      log_error ("http_parse_request: out of memory");
      return -1;
    }

  request->method = -1;
  request->uri = NULL;
  request->major_version = -1;
  request->minor_version = -1;
  request->header = NULL;

  n = read_until (fd, ' ', &data);
  if (n <= 0)
    {
      free (request);
      return n;
    }
  request->method = http_string_to_method (data, n - 1);
  if (request->method == -1)
    {
      log_error ("http_parse_request: expected an HTTP method");
      free (data);
      free (request);
      return -1;
    }
  data[n - 1] = 0;
  log_verbose ("http_parse_request: method = \"%s\"", data);
  free (data);
  len = n;

  n = read_until (fd, ' ', &data);
  if (n <= 0)
    {
      free (request);
      return n;
    }
  data[n - 1] = 0;
  request->uri = data;
  len += n;
  log_verbose ("http_parse_request: uri = \"%s\"", request->uri);

  n = read_until (fd, '/', &data);
  if (n <= 0)
    {
      http_destroy_request (request);
      return n;
    }
  else if (n != 5 || memcmp (data, "HTTP", 4) != 0)
    {
      log_error ("http_parse_request: expected \"HTTP\"");
      free (data);
      http_destroy_request (request);
      return -1;
    }
  free (data);
  len = n;

  n = read_until (fd, '.', &data);
  if (n <= 0)
    {
      http_destroy_request (request);
      return n;
    }
  data[n - 1] = 0;
  request->major_version = atoi (data);
  log_verbose ("http_parse_request: major version = %d",
	       request->major_version);
  free (data);
  len += n;

  n = read_until (fd, '\r', &data);
  if (n <= 0)
    {
      http_destroy_request (request);
      return n;
    }
  data[n - 1] = 0;
  request->minor_version = atoi (data);
  log_verbose ("http_parse_request: minor version = %d",
	       request->minor_version);
  free (data);
  len += n;

  n = read_until (fd, '\n', &data);
  if (n <= 0)
    {
      http_destroy_request (request);
      return n;
    }
  free (data);
  if (n != 1)
    {
      log_error ("http_parse_request: invalid line ending");
      http_destroy_request (request);
      return -1;
    }
  len += n;

  n = parse_header (fd, &request->header);
  if (n <= 0)
    {
      http_destroy_request (request);
      return n;
    }
  len += n;

  *request_ = request;
 return len;
}

ssize_t
http_write_request (int fd, Http_request *request)
{
  char str[1024]; /* FIXME: buffer overflow */
  ssize_t n = 0;
  size_t m;
  
  m = sprintf (str, "%s %s HTTP/%d.%d\r\n",
	       http_method_to_string (request->method),
	       request->uri,
	       request->major_version,
	       request->minor_version);
  m = write_all (fd, str, m);
  log_verbose ("http_write_request: %s", str);
  if (m == -1)
    {
      log_error ("http_write_request: write error: %s", strerror (errno));
      return -1;
    }
  n += m;

  m = http_write_header (fd, request->header);
  if (m == -1)
    {
      return -1;
    }
  n += m;

  return n;
}

void
http_destroy_request (Http_request *request)
{
  if (request->uri)
    free ((char *)request->uri);
  http_destroy_header (request->header);
  free (request);
}

static Http_header *
http_header_find (Http_header *header, const char *name)
{
  if (header == NULL)
    return NULL;

  if (strcmp (header->name, name) == 0)
    return header;

  return http_header_find (header->next, name);
}

const char *
http_header_get (Http_header *header, const char *name)
{
  Http_header *h;

  h = http_header_find (header, name);
  if (h == NULL)
    return NULL;

  return h->value;
}

#if 0
void
http_header_set (Http_header **header, const char *name, const char *value)
{
  Http_header *h;
  size_t n;
  char *v;

  n = strlen (value);
  v = malloc (n + 1);
  if (v == NULL)
    fail;
  memcpy (v, value, n + 1);

  h = http_header_find (*header, name);
  if (h == NULL)
    {
      Http_header *h2;

      h2 = malloc (sizeof (Http_header));
      if (h2 == NULL)
	fail;

      n = strlen (name);
      h2->name = malloc (strlen (name) + 1);
      if (h2->name == NULL)
	fail;
      memcpy (h2->name, name, n + 1);

      h2->value = v;

      h2->next = *header;

      *header = h2;

      return NULL;
    }
  else
    {
      free (h->value);
      h->value = v;
    }
}
#endif
