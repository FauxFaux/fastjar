# Process this with automake to create Makefile.in

INCLUDES = -I. -I$(top_srcdir) $(ZINCS)

bin_PROGRAMS = fastjar grepjar
fastjar_SOURCES = jartool.c dostime.c compress.c pushback.c jartool.h zipfile.h dostime.h compress.h pushback.h
grepjar_SOURCES = jargrep.c dostime.c compress.c pushback.c jartool.h zipfile.h dostime.h compress.h pushback.h

AM_CFLAGS = @fastjar_warn_cflags@
