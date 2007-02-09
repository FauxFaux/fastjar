# Process this with automake to create Makefile.in

SUBDIRS = lib
ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = -I. -I$(top_srcdir) -I$(top_srcdir)/lib
AM_CFLAGS = @fastjar_warn_cflags@
AM_LDFLAGS = -no-undefined
LDADD = $(ZLIBS) lib/libgnu.la libfastjar.la 

bin_PROGRAMS = fastjar grepjar

lib_LTLIBRARIES = libfastjar.la

libfastjar_la_SOURCES = \
	compress.c \
	compress.h \
	dostime.c \
	dostime.h \
	pushback.c \
	pushback.h \
	shift.c \
	shift.h \
	zipfile.h

libfastjar_la_LIBADD = $(ZLIBS)
libfastjar_la_DEPENDENCIES = $(DEPENDENCIES)

fastjar_SOURCES = jartool.c jartool.h
grepjar_SOURCES = jargrep.c jargrep.h

info_TEXINFOS = fastjar.texi
fastjar_TEXINFOS = \
	fdl.texi
man_MANS = fastjar.1 grepjar.1
EXTRA_DIST = $(man_MANS) fastjar.texi texi2pod.pl


TEXI2POD = perl $(srcdir)/texi2pod.pl
POD2MAN = pod2man --center="GNU" --release=@VERSION@

.pod.1:
	-($(POD2MAN) --section=1 $< > $(@).T$$$$ && \
		mv -f $(@).T$$$$ $@) || \
		(rm -f $(@).T$$$$ && exit 1)

.INTERMEDIATE: fastjar.pod grepjar.pod

fastjar.pod: $(srcdir)/fastjar.texi
	-$(TEXI2POD) -D fastjar $< > $@

grepjar.pod: $(srcdir)/fastjar.texi
	-$(TEXI2POD) -D grepjar $< > $@

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

# Support for static checkers
pscan:
	$(PSCAN) $(SOURCES)

flawfinder:
	$(FLAWFINDER) $(SOURCES)

rats:
	$(RATS) $(SOURCES)

SPLINT_FLAGS=-I . -DHAVE_CONFIG_H +posixlib +weak
splint:
	$(SPLINT) $(SPLINT_FLAGS) $(libfastjar_la_SOURCES)
	$(SPLINT) $(SPLINT_FLAGS) $(fastjar_SOURCES)
	$(SPLINT) $(SPLINT_FLAGS) $(grepjar_SOURCES)

