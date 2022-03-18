# Process this file with autoconf to produce a configure script.

AC_INIT([Common Pipeline Library], [7.1.4], [cpl-help@eso.org], [cpl])
AC_PREREQ([2.59])
LT_PREREQ([2.2.6])

AC_CONFIG_SRCDIR([cpl.h])
AC_CONFIG_AUX_DIR([admin])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
LT_CONFIG_LTDL_DIR([libltdl])

AC_CANONICAL_BUILD

AM_INIT_AUTOMAKE
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES])

AM_MAINTAINER_MODE

LT_INIT([disable-static])
AC_SUBST(LIBTOOL_DEPS)

LTDL_INIT([subproject], [convenience])

CPL_SET_PREFIX

# Immediately before every release do:
#-------------------------------------
#   if (the interface is totally unchanged from previous release)
#       REVISION++;
#   else {
#       /* interfaces have been added, removed or changed */
#       REVISION = 0;
#       CURRENT++;
#       if (any interfaces have been _added_ since last release)
#           AGE++;
#       if (any interfaces have been _removed_ or incompatibly changed)
#           AGE = 0;
#   }


# Order of arguments: VERSION, CURRENT, REVISION, AGE
CPL_CONFIG_VERSION([$VERSION], [27], [4], [1]) 

# Checks for programs.
AC_PROG_CC
AC_PROG_CC_C_O

ESO_PROG_CC_FLAG([std=c99], [
                               CPPFLAGS="$CPPFLAGS -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=500"
                               CFLAGS="$CFLAGS -std=c99"
                            ])
ESO_PROG_CC_FLAG([fno-common], [CFLAGS="$CFLAGS -fno-common"])

ESO_ENABLE_DEBUG(no)
ESO_ENABLE_STRICT(no)
ESO_ENABLE_PROFILE(no)


ESO_CHECK_DOCTOOLS

CPL_PATH_JAVA

CPL_ENABLE_THREADS(yes)

# Checks for libraries.
AC_CHECK_LIB(socket, socket, [LIBS="$LIBS -lsocket"])
AC_CHECK_LIB(nsl, inet_ntoa, [LIBS="$LIBS -lnsl"])

CPL_CONFIG_CEXT
CPL_CONFIG_CFITSIO([3.35])
CPL_CHECK_WCS([4.16])
CPL_CHECK_FFTW([3.3.3])

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_TIOCGWINSZ
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h stropts.h sys/ioctl.h \
                  sys/stat.h sys/times.h sys/time.h sys/types.h \
                  termios.h termio.h unistd.h time.h locale.h xlocale.h])


# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_C_BIGENDIAN
AC_C_LONG_DOUBLE

CPL_CHECK_VA_ARGS
CPL_CHECK_COMPLEX

AC_CHECK_SIZEOF(size_t)
AC_CHECK_SIZEOF(void *)
AC_CHECK_SIZEOF(double)
AC_SYS_LARGEFILE


# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_VPRINTF
AC_FUNC_STAT

# fdopen, fileno and dup allows stream duplication
# Additional presence of sigaction sigemptyset allows windows resizing
AC_CHECK_FUNCS([getcwd strdup strrchr fdopen fileno dup sigaction sigemptyset \
                stat getpid setenv llabs cbrt cbrtf clock_gettime gettimeofday])
# Even with -std=c99 these non-C99 functions may be defined,
# so check also if they are declared...
AC_CHECK_DECLS([stat, fdopen, fileno, dup, llabs])

ESO_FUNC_SYSCONF


CPL_SET_PATHS
CPL_CREATE_SYMBOLS(build)
CPL_EXPORT_DIRS(htmldir)

CPL_CHECK_MEMORYMODE
CPL_CHECK_CPU

CPL_ENABLE_GASGANO

# Create body of cpl_func.h
CPL_CONFIG_FUNC

if test ! x$"cpl_cv_with_system_cext" = xyes; then
    AC_CONFIG_SUBDIRS([libcext])
fi


AC_CONFIG_FILES([Makefile
                 Doxyfile
                 cplcore/Makefile
                 cplcore/tests/Makefile
                 cpldrs/Makefile
                 cpldrs/tests/Makefile
                 cplui/Makefile
                 cplui/tests/Makefile
                 cpldfs/Makefile
                 cpldfs/tests/Makefile
                 cpljava/Makefile])
AC_OUTPUT
