/*
http.h
Copyright (C) 1999 Lars Brinkhoff.  See COPYING for terms and conditions.
*/

#include <sys/types.h>

/* All HTTP methods mankind (i.e. RFC2068) knows. */
/* Actually, Netscape has defined some CONNECT method, but */
/* I don't know much about it. */
typedef enum
{
  HTTP_GET,
  HTTP_PUT,
  HTTP_POST,
  HTTP_OPTIONS,
  HTTP_HEAD,
  HTTP_DELETE,
  HTTP_TRACE
} Http_method;

typedef struct http_header Http_header;
struct http_header
{
  const char *name;
  const char *value;
  Http_header *next; /* FIXME: this is ugly; need cons cell. */
};

typedef struct
{
  Http_method method;
  const char *uri;
  int major_version;
  int minor_version;
  Http_header *header;
} Http_request;

typedef struct
{
   int major_version;
   int minor_version;
   int status_code;
   const char *status_message;
   Http_header *header;
} Http_response;

typedef struct
{
  const char *host_name;
  int host_port;
  const char *proxy_name;
  int proxy_port;
  const char *proxy_authorization;
  const char *user_agent;
} Http_destination;

extern ssize_t http_get (int fd, Http_destination *dest);
extern ssize_t http_put (int fd, Http_destination *dest,
			 size_t content_length);
extern ssize_t http_post (int fd, Http_destination *dest,
			  size_t content_length);
extern int http_error_to_errno (int err);

extern Http_response *http_create_response (int major_version,
					    int minor_version,
					    int status_code,
					    const char *status_message);
extern ssize_t http_parse_response (int fd, Http_response **response);
extern void http_destroy_response (Http_response *response);

extern Http_header *http_add_header (Http_header **header,
				     const char *name,
				     const char *value);

extern Http_request *http_create_request (Http_method method,
					  const char *uri,
					  int major_version,
					  int minor_version);
extern ssize_t http_parse_request (int fd, Http_request **request);
extern ssize_t http_write_request (int fd, Http_request *request);
extern void http_destroy_request (Http_request *resquest);

extern const char *http_header_get (Http_header *header, const char *name);
