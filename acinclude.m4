dnl HTTPTUNNEL_TYPE_SOCKLEN_T
dnl Check for the existance of type socklen_t.

AC_DEFUN(HTTPTUNNEL_TYPE_SOCKLEN_T,
[AC_CACHE_CHECK([for socklen_t], ac_cv_httptunnel_type_socklen_t,
[
  AC_TRY_COMPILE(
  [#include <sys/types.h>
   #include <sys/socket.h>],
  [socklen_t len = 42; return 0;],
  ac_cv_httptunnel_type_socklen_t=yes,
  ac_cv_httptunnel_type_socklen_t=no)
])
  if test $ac_cv_httptunnel_type_socklen_t != yes; then
    AC_DEFINE(socklen_t, int)
  fi
])


dnl HTTPTUNNEL_DEFINE_INADDR_NONE
dnl Check for the existance of define INADDR_NONE

AC_DEFUN(HTTPTUNNEL_DEFINE_INADDR_NONE,
[AC_CACHE_CHECK([whether INADDR_NONE is defined], ac_cv_httptunnel_define_inaddr_none,
[
  AC_TRY_COMPILE(
  [#include <sys/types.h>
   #include <netinet/in.h>],
  [return INADDR_NONE;],
  ac_cv_httptunnel_define_inaddr_none=yes,
  ac_cv_httptunnel_define_inaddr_none=no)
])
  if test $ac_cv_httptunnel_define_inaddr_none != yes; then
    AC_DEFINE(INADDR_NONE, 0xffffffff)
  fi
])
