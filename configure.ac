dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.61)
AC_INIT([fastjar],[0.95-pre]) 
AC_CONFIG_SRCDIR([jartool.h])
AM_INIT_AUTOMAKE([-Wall gnu 1.10])
AC_CONFIG_HEADERS([config.h])

dnl Checks for programs.
AC_PROG_CC
gl_EARLY
AC_PROG_INSTALL
AC_EXEEXT

AC_DISABLE_STATIC
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)

AM_MAINTAINER_MODE

dnl Add warning flags if we are using gcc.
if test "$GCC" = yes; then
  fastjar_warn_cflags='-W -Wall -pedantic -Wstrict-prototypes -Wmissing-prototypes -Wwrite-strings'
fi
AC_SUBST(fastjar_warn_cflags)

dnl Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS_ONCE([fcntl.h unistd.h sys/param.h stdlib.h limits.h])

AC_CHECK_FUNCS_ONCE([ftruncate _chsize])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_OFF_T
AC_STRUCT_TM

# mkdir takes a single argument on some systems.
gcc_AC_FUNC_MKDIR_TAKES_ONE_ARG

# check for fixed size types.
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T

dnl Check byte order
AC_C_BIGENDIAN

AC_ARG_WITH(system-zlib,
[  --with-system-zlib      use installed libz])

ZLIBS=
ZDEPS=
ZINCS=
use_zlib=maybe
AC_CHECK_LIB(z, deflate, ZLIBS=-lz, use_zlib=no)

if test "$use_zlib" = no; then
   # Brain dead way to find tree's zlib.
   AC_MSG_ERROR([Need zlib])
fi
AC_SUBST(ZLIBS)
AC_SUBST(ZDEPS)
AC_SUBST(ZINCS)

gl_INIT

# GCC LOCAL CHANGE
# We would like to our source tree to be readonly.  However when releases or
# pre-releases are generated, the man pages need to be included as they are
# converted from the texi files via perl which we don't require end users to
# have installed.
# Therefore we have --enable-generated-files-in-srcdir to do just that.

AC_MSG_CHECKING([whether to place generated files in the source directory])
  dnl generated-files-in-srcdir is disabled by default
  AC_ARG_ENABLE(generated-files-in-srcdir, 
[  --enable-generated-files-in-srcdir
                          put copies of generated files in source dir
                          intended for creating source tarballs for users
                          without texinfo, perl, bison or flex.],
      generated_files_in_srcdir=$enableval,
      generated_files_in_srcdir=no)

AC_MSG_RESULT($generated_files_in_srcdir)
AM_CONDITIONAL(GENINSRC, test x$generated_files_in_srcdir = xyes)

AC_CONFIG_FILES([lib/Makefile Makefile install-defs.sh])
AC_OUTPUT
