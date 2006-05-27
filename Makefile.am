# Process this with automake to create Makefile.in

AM_CPPFLAGS = -I. -I$(top_srcdir) $(ZINCS)

bin_PROGRAMS = fastjar grepjar
fastjar_SOURCES = jartool.c dostime.c compress.c pushback.c shift.c jartool.h \
zipfile.h dostime.h compress.h pushback.h shift.h
fastjar_LDADD = $(ZLIBS)
fastjar_DEPENDENCIES = $(ZDEPS)
fastjar_CPPFLAGS = $(AM_CPPFLAGS) -DWITH_SHIFT_DOWN

grepjar_SOURCES = jargrep.c dostime.c compress.c pushback.c jartool.h \
zipfile.h dostime.h compress.h pushback.h jargrep.h
grepjar_LDADD = $(ZLIBS)
grepjar_DEPENDENCIES = $(ZDEPS)

AM_CFLAGS = @fastjar_warn_cflags@

info_TEXINFOS = fastjar.texi
fastjar_TEXINFOS = \
	fdl.texi
man_MANS = fastjar.1 grepjar.1
EXTRA_DIST = $(man_MANS) fastjar.texi texi2pod.pl


TEXI2POD = perl $(srcdir)/texi2pod.pl
POD2MAN = pod2man --center="GNU" --release="0.92"

.pod.1:
	-($(POD2MAN) --section=1 $< > $(@).T$$$$ && \
		mv -f $(@).T$$$$ $@) || \
		(rm -f $(@).T$$$$ && exit 1)

.INTERMEDIATE: fastjar.pod grepjar.pod

fastjar.pod: $(srcdir)/fastjar.texi
	-$(TEXI2POD) -D fastjar $< > $@

grepjar.pod: $(srcdir)/fastjar.texi
	-$(TEXI2POD) -D grepjar $< > $@

