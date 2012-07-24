/*
base64.h
Copyright (C) 1999 Lars Brinkhoff.  See COPYING for terms and conditions.
*/

#include <sys/types.h>

ssize_t encode_base64 (const void *data, size_t length, char **code);

