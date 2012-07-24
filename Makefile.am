## Copyright (C) 1999 Lars Brinkhoff

SUBDIRS = port

bin_PROGRAMS = htc hts
man_MANS = hts.1 htc.1

if SRCDIR
AM_CPPFLAGS = -I$(VPATH)/port
else
AM_CPPFLAGS = -Iport
endif

htc_SOURCES = htc.c common.c tunnel.c http.c base64.c
htc_LDADD = -Lport -lport
hts_SOURCES = hts.c common.c tunnel.c http.c
hts_LDADD = -Lport -lport

noinst_HEADERS = common.h tunnel.h http.h base64.h

EXTRA_DIST = TODO HACKING DISCLAIMER doc/rfc1945.txt doc/rfc2068.txt \
             FAQ doc/rfc2045.txt hts.1 htc.1 debian/changelog debian/control \
             debian/copyright debian/dirs debian/docs debian/rules \
             debian/prerm debian/postinst

squeaky-clean: maintainer-clean
	find . -name Makefile.in | xargs rm
	rm -f aclocal.m4 config.h.in configure *~
