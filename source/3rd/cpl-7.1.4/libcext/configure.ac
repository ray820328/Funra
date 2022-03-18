# Process this file with autoconf to produce a configure script.

AC_INIT([C Extension Library], [1.2.4], [cpl-help@eso.org], [cext])
AC_PREREQ([2.59])

AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_AUX_DIR([admin])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE

AM_MAINTAINER_MODE

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
#

# Order of arguments: CURRENT, REVISION, AGE
ESO_SET_LIBRARY_VERSION([2], [3], [2])


# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O

ESO_PROG_CC_FLAG([std=c99], [
                               CPPFLAGS="$CPPFLAGS -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=500"
                               CFLAGS="$CFLAGS -std=c99"
                            ])
ESO_PROG_CC_FLAG([fno-builtin], [CFLAGS="$CFLAGS -fno-builtin"])
ESO_PROG_CC_FLAG([fno-common], [CFLAGS="$CFLAGS -fno-common"])

CEXT_ENABLE_DEBUG(no)
ESO_ENABLE_STRICT(no)

ESO_CHECK_DOCTOOLS

AC_ENABLE_STATIC(yes)
AC_ENABLE_SHARED(yes)


AC_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)

CEXT_ENABLE_THREADS(yes)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stddef.h stdint.h stdarg.h inttypes.h \
                  float.h limits.h locale.h values.h unistd.h])

AC_HEADER_STAT


# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE

ESO_PROG_CC_ATTRIBUTE_VISIBILITY([hidden])

CEXT_TYPE_LONG_LONG_INT
CEXT_TYPE_UNSIGNED_LONG_LONG_INT
CEXT_TYPE_INTMAX_T
CEXT_TYPE_UINTMAX_T
CEXT_TYPE_UINTPTR_T
CEXT_TYPE_LONG_DOUBLE

AC_TYPE_SIZE_T

if test x"$ac_cv_header_stdint_h" = xyes; then
    AC_CHECK_TYPE(int8_t, [], [], [#include <stdint.h>])
    AC_CHECK_TYPE(int16_t, [], [], [#include <stdint.h>])
    AC_CHECK_TYPE(int32_t, [], [], [#include <stdint.h>])
    AC_CHECK_TYPE(int64_t, [], [], [#include <stdint.h>])
fi

AC_CHECK_TYPES([ptrdiff_t])

CEXT_CHECK_CHAR_BIT
if test x$cext_have_char_8_bits != xyes; then
    AC_MSG_ERROR([cext requires the type char to be 8 bits wide.])
fi

AC_CHECK_SIZEOF(char)
AC_CHECK_SIZEOF(short)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(long long)
AC_CHECK_SIZEOF(size_t)
AC_CHECK_SIZEOF(void *)

if test x"$ac_cv_type_int64_t" = xno && test x"$ac_cv_sizeof_long" != x8 && test x"$ac_cv_sizeof_long_long" != x8; then
    AC_MSG_ERROR([A 64 bit integer type is required. Consider using the GNU C compiler])
fi

CEXT_CHECK_SIZE_T

AC_CHECK_MEMBERS([struct lconv.decimal_point, struct lconv.thousands_sep],
                 [], [], [#include <locale.h>])

AC_SYS_LARGEFILE

# Checks for library functions.
AC_CHECK_FUNCS([strchr memcpy sprintf localeconv])

AC_FUNC_LSTAT
AC_FUNC_STAT

ESO_FUNC_VA_COPY([CX_VA_COPY])
ESO_FUNC_REALLOC_SANITY
ESO_FUNC_STRDUP
ESO_FUNC_STPCPY
ESO_FUNC_VSNPRINTF
ESO_FUNC_VASPRINTF
ESO_FUNC_SYSCONF
ESO_FUNC_FPATHCONF

CEXT_SET_PATHS
CEXT_SET_SYMBOLS

CEXT_CREATE_CXCONFIG


# Library log domain
AC_DEFINE([CX_LOG_DOMAIN],
          ["CxLib"], [Log domain internally used by the library])

AC_CONFIG_FILES(Makefile
                Doxyfile
				cext.pc
                cext/Makefile
                tests/Makefile)
AC_OUTPUT
