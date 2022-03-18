# ESO_PROG_CC_FLAG(FLAG, [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
#-----------------------------------------------------------------
AC_DEFUN([ESO_PROG_CC_FLAG],
[
    AC_REQUIRE([AC_PROG_CC])

    flag=`echo $1 | sed 'y%.=/+-%___p_%'`
    AC_CACHE_CHECK([whether $CC supports -$1],
                   [eso_cv_prog_cc_$flag],
                   [
                       eval "eso_cv_prog_cc_$flag=no"
                       AC_LANG_PUSH(C)

                       echo 'int main() { return 0; }' >conftest.$ac_ext

                       try_compile="`$CC -$1 -c conftest.$ac_ext 2>&1`"
                       if test -z "$try_compile"; then
                           try_link="`$CC -$1 -o conftest$ac_exeext \
                                    conftest.$ac_ext 2>&1`"
                           if test -z "$try_link"; then
                               eval "eso_cv_prog_cc_$flag=yes"
                           fi
                       fi
                       rm -f conftest*

                       AC_LANG_POP(C)
                   ])

    if eval "test \"`echo '$eso_cv_prog_cc_'$flag`\" = yes"; then
        :
        $2
    else
        :
        $3
    fi
])


# ESO_PROG_CC_ATTRIBUTE(VARIANT1, [VARIANT2], [CODE], [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
#----------------------------------------------------------------------------------------------
AC_DEFUN([ESO_PROG_CC_ATTRIBUTE],
[

    AC_CACHE_CHECK([if $CC supports __attribute__(( ifelse([$2], , [$1], [$2]) ))],
                   AS_TR_SH([eso_cv_prog_cc_attribute_$1]),
                   [
                       eso_save_CFLAGS="$CFLAGS"
                       CFLAGS="$CFLAGS"
                   
                       AC_COMPILE_IFELSE([AC_LANG_SOURCE([$3])],
                                         [eval "AS_TR_SH([eso_cv_prog_cc_attribute_$1])='yes'"],
                                         [eval "AS_TR_SH([eso_cv_prog_cc_attribute_$1])='no'"])
                       CFLAGS="$eso_save_CFLAGS"
                   ])
                   
    if eval "test x\$AS_TR_SH([eso_cv_prog_cc_attribute_$1]) = xyes"; then
        :
        $4
    else
        :
        $5
    fi
               
])


# ESO_PROG_CC_ATTRIBUTE_VISIBILITY(ARG, [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
#--------------------------------------------------------------------------------
AC_DEFUN([ESO_PROG_CC_ATTRIBUTE_VISIBILITY],
[

    ESO_PROG_CC_ATTRIBUTE([visibility_$1], [visibility("$1")],
                          [void __attribute__((visibility("$1"))) $1_function() { }],
                          [$2], [$3])

])


# ESO_ENABLE_DEBUG(debug=no)
#---------------------------
AC_DEFUN([ESO_ENABLE_DEBUG],
[
    AC_REQUIRE([AC_PROG_CC])

    AC_ARG_ENABLE(debug,
                  AS_HELP_STRING([--enable-debug],
                                 [creates debugging code [default=$1]]),
                  eso_enable_debug=$enableval, eso_enable_debug=$1)

    AC_CACHE_CHECK([whether debugging code should be created],
                   eso_cv_enable_debug,
                   eso_cv_enable_debug=$eso_enable_debug)

    if test x"$eso_cv_enable_debug" = xyes; then

        eso_clean_CFLAGS="`echo $CFLAGS | sed -e 's/-O[[0-9]]//g' \
                                              -e 's/-g[[0-9]]//g' \
                                              -e 's/-g[[a-z,A-Z]]* / /g' \
                                              -e 's/-[[Og]]//g'`"

        ESO_PROG_CC_FLAG([g3], [CFLAGS="$CFLAGS -g3"])

        if test x"$eso_cv_prog_cc_g3" = xyes; then
            CFLAGS="-g3"
        else
            if test x"$ac_cv_prog_cc_g" = xyes; then
                CFLAGS="-g"
            else
                CFLAGS=""
            fi
        fi

        ESO_PROG_CC_FLAG([ggdb], [CFLAGS="$CFLAGS -ggdb"])
        ESO_PROG_CC_FLAG([O0], [CFLAGS="$CFLAGS -O0"])
        ESO_PROG_CC_FLAG([rdynamic], [CFLAGS="$CFLAGS -rdynamic"])
        ESO_PROG_CC_FLAG([Wall], [CFLAGS="$CFLAGS -Wall"])
        ESO_PROG_CC_FLAG([W], [CFLAGS="$CFLAGS -W"])

        CFLAGS="$CFLAGS $eso_clean_CFLAGS"
        ESO_DEBUG_FLAGS="-DESO_ENABLE_DEBUG"
    else
        ESO_DEBUG_FLAGS="-DNDEBUG"
    fi

    AC_SUBST(ESO_DEBUG_FLAGS)
])


# ESO_ENABLE_STRICT(strict=no)
#-----------------------------
AC_DEFUN([ESO_ENABLE_STRICT],
[
	AC_REQUIRE([AC_PROG_EGREP])
    AC_REQUIRE([AC_PROG_CC])

    AC_ARG_ENABLE(strict,
                  AS_HELP_STRING([--enable-strict],
                                 [compiles with strict compiler options (may not work!) [default=$1]]),
                  eso_enable_strict=$enableval, eso_enable_strict=$1)

    AC_CACHE_CHECK([whether strict compiler options should be used],
                   eso_cv_enable_strict,
                   eso_cv_enable_strict=$eso_enable_strict)


    if test x"$eso_cv_enable_strict" = xyes; then
    
    	eso_enable_strict_std_set=no
    	
        if test -n "$CFLAGS"; then
            echo $CFLAGS | $EGREP '(\-std=|-ansi)' >/dev/null 2>&1
            if test x"$?" = x0; then
            	eso_enable_strict_std_set=yes
            fi
        fi
        
        if test x"$eso_enable_strict_std_set" = xno; then
        	ESO_PROG_CC_FLAG([std=c99], [CFLAGS="$CFLAGS -std=c99"])
        fi
        
        ESO_PROG_CC_FLAG([pedantic], [CFLAGS="$CFLAGS -pedantic"])
        
    fi
])


# ESO_ENABLE_PROFILE(profile=no)
#-----------------------------
AC_DEFUN([ESO_ENABLE_PROFILE],
[
    AC_REQUIRE([AC_PROG_CC])

    AC_ARG_ENABLE(profile,
                  AS_HELP_STRING([--enable-profile],
                                 [compiles with compiler options necessary for profiling (may not work!) [default=$1]]),
                  eso_enable_profile=$enableval, eso_enable_profile=$1)

    AC_CACHE_CHECK([whether profiling compiler options should be used],
                   eso_cv_enable_profile,
                   eso_cv_enable_profile=$eso_enable_profile)


    if test x"$eso_cv_enable_profile" = xyes; then
        ESO_PROG_CC_FLAG([pg], [CFLAGS="$CFLAGS -pg"])
        ESO_PROG_CC_FLAG([g], [CFLAGS="$CFLAGS -g"])
        ESO_PROG_CC_FLAG([static-libgcc], [CFLAGS="$CFLAGS -static-libgcc"])

        AC_ENABLE_SHARED(no)
        AC_ENABLE_STATIC(yes)
    fi
])


# ESO_CHECK_DOCTOOLS
#-------------------
AC_DEFUN([ESO_CHECK_DOCTOOLS],
[
    AC_ARG_VAR([DOXYGEN], [doxygen command])
    AC_PATH_PROG([DOXYGEN], [doxygen])

    AC_ARG_VAR([LATEX], [latex command])
    AC_PATH_PROG([LATEX], [latex])


    if test -z "${DOXYGEN}"; then
        DOXYGEN=":"
    fi

    if test -z "${LATEX}"; then
        LATEX=":"
    fi

])


# ESO_PROG_AR
#------------
# Checks if ar is in the path
AC_DEFUN([ESO_PROG_AR],
[
    AC_CHECK_PROG(AR, ar, ar, NONE)

    if test x"$AR" = xNONE; then
        AC_MSG_ERROR([Cannot find \'ar\'])
    fi

])


# ESO_PROG_PKGCONFIG
#-------------------
# Checks if pkg-config is in the path
AC_DEFUN([ESO_PROG_PKGCONFIG],
[
	AC_ARG_VAR([PKGCONFIG], [pkg-config command])
	AC_CHECK_PROG([PKGCONFIG], [pkg-config], [pkg-config])

])


# ESO_CHECK_EXTRA_LIBS
#---------------------
# Check for non-standard headers and libraries
AC_DEFUN([ESO_CHECK_EXTRA_LIBS],
[

    AC_ARG_WITH(extra-includes,
                AS_HELP_STRING([--with-extra-includes=DIR],
                               [adds non standard include paths]),
                eso_with_extra_includes=$withval, eso_with_extra_includes=NONE)

    AC_ARG_WITH(extra-libs,
                AS_HELP_STRING([--with-extra-libs=DIR],
                              [adds non standard library paths]),
                eso_with_extra_libs=$withval, eso_with_extra_libs=NONE)

    AC_MSG_CHECKING([for extra includes])
    AC_CACHE_VAL([eso_cv_with_extra_includes],
                 [
                     eso_cv_with_extra_includes=$eso_with_extra_includes
                 ])

    if test x"$eso_cv_with_extra_includes" != xNONE; then
        eso_save_IFS=$IFS
        IFS=':'

        for dir in $eso_cv_with_extra_includes; do
            EXTRA_INCLUDES="$EXTRA_INCLUDES -I$dir"
        done

        IFS=$eso_save_IFS
        AC_MSG_RESULT(added)
    else
        AC_MSG_RESULT(no)
    fi


    AC_MSG_CHECKING([for extra libs])
    AC_CACHE_VAL([eso_cv_with_extra_libs],
                 [
                     eso_cv_with_extra_libs=$eso_with_extra_libs
                 ])

    if test x"$eso_cv_with_extra_libs" != xNONE; then
        eso_save_IFS=$IFS
        IFS=':'

        for dir in $eso_cv_with_extra_libs; do
            EXTRA_LDFLAGS="$EXTRA_LDFLAGS -L$dir"
        done

        IFS=$eso_save_IFS
        AC_MSG_RESULT(added)
    else
        AC_MSG_RESULT(no)
    fi

])


# ESO_CHECK_THREADS_POSIX
#------------------------
# Check whether the POSIX threads are available. The cached result is
# set to 'yes' if either the compiler supports the '-pthread' flag, or linking
# with the POSIX thread library works, and the header file defining the POSIX
# threads symbols is present. If POSIX threads are not supported, the
# result is set to 'no'. Whether the compiler supports POSIX threads,
# or whether the library, and the header are available is stored in cache
# variables.  
AC_DEFUN([ESO_CHECK_THREADS_POSIX],
[
    AC_REQUIRE([AC_PROG_CC])

    ESO_PROG_CC_FLAG([pthread], [], [])
    
    AC_CHECK_LIB([pthread], [pthread_create],
                 [eso_threads_have_libpthread=yes],
                 [eso_threads_have_libpthread=no])

    AC_CHECK_HEADER([pthread.h],
                    [eso_threads_have_pthread_h=yes],
                    [eso_threads_have_pthread_h=no])

    if test x"$eso_threads_have_pthread_h" != xyes; then
        eso_threads_posix=no
    else
        if test x"$eso_threads_have_libpthread" != xyes && \
          test x"$eso_cv_prog_cc_pthread" != xyes; then
            eso_threads_posix=no
        else
            eso_threads_posix=yes
        fi
    fi
    
    
    # Setup the POSIX thread symbols

    if test x"$eso_threads_have_pthread_h" = xyes; then
        AC_DEFINE([HAVE_PTHREAD_H], [1],
                  [Define to 1 if you have <pthread.h>.])
    fi
    
    if test x"$eso_threads_posix" = xyes; then
    
        if test x"$eso_cv_prog_cc_pthread" = xyes; then
            PTHREAD_CFLAGS="-pthread"
        else
            PTHREAD_CFLAGS=""
        fi
        
        if test x"$eso_threads_have_libpthread" = xyes; then
            LIBPTHREAD="-lpthread"
        else
            LIBPTHREAD=""
        fi
        
    fi  

    AC_CACHE_VAL(eso_cv_threads_posix_header,
                 eso_cv_threads_posix_header=$eso_threads_have_pthread_h)          
    AC_CACHE_VAL(eso_cv_threads_posix_lib,
                 eso_cv_threads_posix_lib=$eso_threads_have_libpthread)          
    AC_CACHE_VAL(eso_cv_threads_posix_flags,
                 eso_cv_threads_posix_flags=$eso_cv_prog_cc_pthread)          
    AC_CACHE_VAL(eso_cv_threads_posix,
                 eso_cv_threads_posix=$eso_threads_posix)

    AC_SUBST(PTHREAD_CFLAGS)
    AC_SUBST(LIBPTHREAD)
    
])


# ESO_CHECK_FUNC(FUNCTION, INCLUDES, SYMBOL)
#-------------------------------------------
# Checks whether a function is available and declared.
AC_DEFUN([ESO_CHECK_FUNC],
[

    AC_LANG_PUSH(C)

    AC_CHECK_DECL($1, [], [], [$2])

    eso_save_CFLAGS="$CFLAGS"

    if test x"$GCC" = xyes; then
        CFLAGS="$CFLAGS -pedantic-errors"
    fi

    AC_CHECK_FUNC($1)

    CFLAGS="$eso_save_CFLAGS"

    AC_LANG_POP(C)

    if test x"$ac_cv_have_decl_$1" = xyes &&
       test x"$ac_cv_func_$1" = xyes; then
        AC_DEFINE($3)
    fi

])


# ESO_FUNC_VSNPRINTF_C99
#-----------------------
# Check whether vsnprintf() has C99 semantics.
AC_DEFUN([ESO_FUNC_VSNPRINTF_C99],
[

    AH_TEMPLATE([HAVE_VSNPRINTF_C99],
                [Define if you have the C99 `vsnprintf' function.])

    AC_CACHE_CHECK([whether vsnprintf has C99 semantics],
                   [eso_cv_func_vsnprintf_c99],
                   [
                       AC_LANG_PUSH(C)

                       eso_cppflags_save="$CPPFLAGS"
                       eso_cflags_save="$CFLAGS"
                       eso_ldflags_save="$LDFLAGS"
                       eso_libs_save="$LIBS"

                       if test x$GCC = xyes; then
                           CFLAGS="$CFLAGS -pedantic-errors"
                           CPPFLAGS="$CPPFLAGS $CFLAGS"
                       fi

                       AC_RUN_IFELSE(
                       [
AC_LANG_PROGRAM(
[[
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

int
doit(char * s, ...)
{
    char buffer[32];
    va_list args;
    int q, r;

    va_start(args, s);
    q = vsnprintf(NULL, 0, s, args);
    r = vsnprintf(buffer, 5, s, args);
    va_end(args);

    if (q != 7 || r != 7)
      exit(1);

    exit(0);
}
]],
[[
    doit((char*)"1234567");
    exit(1);
]])
                       ],
                       [eso_cv_func_vsnprintf_c99=yes],
                       [eso_cv_func_vsnprintf_c99=no],
                       [eso_cv_func_vsnprintf_c99=no])

                       CPPFLAGS="$eso_cppflags_save"
                       CFLAGS="$eso_cflags_save"
                       LDFLAGS="$eso_ldflags_save"
                       LIBS="$eso_libs_save"

                       AC_LANG_POP(C)
                   ])

# Note that the default is to be pessimistic in the case of cross compilation.
# If you know that the target has a C99 vsnprintf(), you can get around this
# by setting eso_func_vsnprintf_c99 to yes, as described in the Autoconf
# manual.

    if test x$eso_cv_func_vsnprintf_c99 = xyes; then
        AC_DEFINE(HAVE_VSNPRINTF_C99)
    fi

])


# ESO_CHECK_PRINTF_FORMATS
#-------------------------
# Checks for printf() format peculiarities.
AC_DEFUN([ESO_CHECK_PRINTF_FORMATS],
[

    # Check if string format for NULL is `(null)'

    AH_TEMPLATE([HAVE_PRINTF_STR_FMT_NULL],
                [Define if printf outputs `(null)' when printing NULL using
                 `%s'])

    AC_RUN_IFELSE(
    [
AC_LANG_PROGRAM(
[[
#include <stdio.h>
#include <string.h>
]],
[[
    char s[128];

    sprintf(s, "%s", NULL);
    return strncmp(s, "(null)", 6) ? 1 : 0;
]])
    ],
    [eso_have_printf_str_format_null=yes],
    [eso_have_printf_str_format_null=no],
    [eso_have_printf_str_format_null=no])

    if test x$eso_have_printf_str_format_null = xyes; then
        AC_DEFINE(HAVE_PRINTF_STR_FMT_NULL)
    fi


    # Check if pointer format for NULL is `(nil)'

    AH_TEMPLATE([HAVE_PRINTF_PTR_FMT_NIL],
                [Define if printf outputs `(nil)' when printing NULL using
                 `%p'])

    AC_RUN_IFELSE(
    [
AC_LANG_PROGRAM(
[[
#include <stdio.h>
#include <string.h>
]],
[[
    char s[128];

    sprintf(s, "%p", NULL);
    return strncmp(s, "(nil)", 5) ? 1 : 0;
]])
    ],
    [eso_have_printf_ptr_format_nil=yes],
    [eso_have_printf_ptr_format_nil=no],
    [eso_have_printf_ptr_format_nil=no])

    if test x$eso_have_printf_ptr_format_nil = xyes; then
        AC_DEFINE(HAVE_PRINTF_PTR_FMT_NIL)
    fi


    # Check if output for `%p' is the same as `%#x'

    AH_TEMPLATE([HAVE_PRINTF_PTR_FMT_ALTERNATE],
                [Define if printf format `%p' produces the same output as
                 `%#x' or `%#lx'])

    AC_RUN_IFELSE(
    [
AC_LANG_PROGRAM(
[[
#include <stdio.h>
#include <string.h>
]],
[[
    char s1[128], s2[128];

    sprintf(s1, "%p", s1);
    sprintf(s2, "%#x", s1);
    return strncmp(s1, s2, 3) ? 1 : 0;
]])
    ],
    [eso_have_printf_ptr_format_alternate=yes],
    [eso_have_printf_ptr_format_alternate=no],
    [eso_have_printf_ptr_format_alternate=no])

    if test x$eso_have_printf_ptr_format_alternate = xyes; then
        AC_DEFINE(HAVE_PRINTF_PTR_FMT_ALTERNATE)
    fi


    # Check if pointers are treated as signed

    AH_TEMPLATE([HAVE_PRINTF_PTR_FMT_SIGNED],
                [Define if printf treats pointers as signed when using a sign
                 flag])

    AC_RUN_IFELSE(
    [
AC_LANG_PROGRAM(
[[
#include <stdio.h>
]],
[[
    char s[128];

    sprintf(s, "%+p", s);
    return s[0] == '+' ? 0 : 1;
]])
    ],
    [eso_have_printf_ptr_format_signed=yes],
    [eso_have_printf_ptr_format_signed=no],
    [eso_have_printf_ptr_format_signed=no])

    if test x$eso_have_printf_ptr_format_signed = xyes; then
        AC_DEFINE(HAVE_PRINTF_PTR_FMT_SIGNED)
    fi


    # Check if default precision for conversion specifier `g' is 1 (as
    # required by ISO C) or 6.

    AH_TEMPLATE([HAVE_PRINTF_FLT_FMT_G_STD],
                [Define if printf default precision for format `g' is 1
                 (ISO C standard) or 6])

    AC_RUN_IFELSE(
    [
AC_LANG_PROGRAM(
[[
#include <stdio.h>
]],
[[
    char s1[128], s2[128];
    int n1, n2;

    sprintf(s1, "%g%n", 1.123456, &n1);
    sprintf(s2, "%.1g%n", 1.123456, &n2);
    return n1 > n2 ? 1 : 0;
]])
    ],
    [eso_have_printf_flt_format_g_std=yes],
    [eso_have_printf_flt_format_g_std=no],
    [eso_have_printf_flt_format_g_std=no])

    if test x$eso_have_printf_flt_format_g_std = xyes; then
        AC_DEFINE(HAVE_PRINTF_FLT_FMT_G_STD)
    fi

])


# ESO_FUNC_VSNPRINTF
#-------------------
# Checks for vsnprintf and snprintf declaration and function.
AC_DEFUN([ESO_FUNC_VSNPRINTF],
[

    eso_compile_snprintf=no

    AH_TEMPLATE([HAVE_VSNPRINTF],
                [Define if you have the `vsnprintf' function])
    ESO_CHECK_FUNC(vsnprintf, [
#include <stdio.h>
#include <stdarg.h>
                              ], HAVE_VSNPRINTF)

    if test x$ac_cv_func_vsnprintf = xyes &&
       test x$ac_cv_have_decl_vsnprintf = xyes; then

        ESO_FUNC_VSNPRINTF_C99

        if test x$eso_cv_func_vsnprintf_c99 != xyes; then
            eso_compile_snprintf=yes
        fi

    else
        eso_compile_snprintf=yes
    fi

    if test x$eso_compile_snprintf = xyes; then
        if test -n "$LIBTOOL"; then
            SNPRINTF=snprintf.lo
        else
            SNPRINTF=snprintf.$ac_objext
        fi
    fi

    AC_SUBST(SNPRINTF)

    # The symbols defined by the following macro are only needed to setup the
    # vsnprintf() replacement. May be useless if the vsnprintf implementation
    # changes.
    ESO_CHECK_PRINTF_FORMATS

    AH_TEMPLATE([HAVE_SNPRINTF],
                [Define if you have the `snprintf' function])
    ESO_CHECK_FUNC(snprintf, [#include <stdio.h>], HAVE_SNPRINTF)

])


# ESO_FUNC_VASPRINTF
#-------------------
# Checks for vasprintf and asprintf declaration and function.
AC_DEFUN([ESO_FUNC_VASPRINTF],
[

    AH_TEMPLATE([HAVE_VASPRINTF],
                [Define if you have the `vasprintf' function])
    ESO_CHECK_FUNC(vasprintf, [
#include <stdio.h>
#include <stdarg.h>
                              ], HAVE_VASPRINTF)

    AH_TEMPLATE([HAVE_ASPRINTF],
                [Define if you have the `asprintf' function])
    ESO_CHECK_FUNC(asprintf, [
#include <stdio.h>
                             ], HAVE_ASPRINTF)

])


# ESO_FUNC_FPATHCONF
#-------------------
# Checks for fpathconf declaration and function.
AC_DEFUN([ESO_FUNC_FPATHCONF],
[

    AH_TEMPLATE([HAVE_FPATHCONF],
                [Define if you have the `fpathconf' function])
    ESO_CHECK_FUNC(fpathconf, [#include <unistd.h>], HAVE_FPATHCONF)

    # If we have fpathconf we should also have pathconf, but who knows.
    AH_TEMPLATE([HAVE_PATHCONF],
                [Define if you have the `pathconf' function])
    ESO_CHECK_FUNC(pathconf, [#include <unistd.h>], HAVE_PATHCONF)

])


# ESO_FUNC_SYSCONF
#-----------------
# Checks for sysconf declaration and function.
AC_DEFUN([ESO_FUNC_SYSCONF],
[

    AH_TEMPLATE([HAVE_SYSCONF],
                [Define if you have the `sysconf' function])
    ESO_CHECK_FUNC(sysconf, [#include <unistd.h>], HAVE_SYSCONF)

])


# ESO_FUNC_GETOPT
#----------------
# Checks for GNU getopt_long declaration and function.
AC_DEFUN([ESO_FUNC_GETOPT],
[

    AH_TEMPLATE([HAVE_GETOPT_LONG],
                [Define if you have the `getopt_long' function])

    ESO_CHECK_FUNC(getopt_long, [#include <getopt.h>], HAVE_GETOPT_LONG)

    if test x"$ac_cv_func_getopt_long" = xno ||
       test x"$eso_cv_have_decl_getopt_long" = xno; then
        if test -n "$LIBTOOL"; then
            GETOPT="getopt.lo getopt1.lo"
        else
            GETOPT="getopt.$ac_objext getopt1.$ac_objext"
        fi
    fi

    AC_SUBST(GETOPT)


])


# ESO_FUNC_GETPWUID
#------------------
# Checks for getpwuid declaration and function.
AC_DEFUN([ESO_FUNC_GETPWUID],
[

    AH_TEMPLATE([HAVE_GETPWUID],
                [Define if you have the `getpwuid' function])

    ESO_CHECK_FUNC(getpwuid, [#include <pwd.h>], HAVE_GETPWUID)

])


# ESO_FUNC_GETUID
#----------------
AC_DEFUN([ESO_FUNC_GETUID],
[

    AH_TEMPLATE([HAVE_GETUID],
                [Define if you have the `getuid' function])

    ESO_CHECK_FUNC(getuid, [#include <unistd.h>], HAVE_GETUID)

])


# ESO_FUNC_LSTAT
#---------------
AC_DEFUN([ESO_FUNC_LSTAT],
[

    AH_TEMPLATE([HAVE_LSTAT],
                [Define if you have the `lstat' function])

    ESO_CHECK_FUNC(lstat, [#include <sys/stat.h>], HAVE_LSTAT)

])


# ESO_FUNC_STRDUP
#----------------
AC_DEFUN([ESO_FUNC_STRDUP],
[

    AH_TEMPLATE([HAVE_STRDUP],
                [Define if you have the `strdup' function])

    ESO_CHECK_FUNC(strdup, [#include <string.h>], HAVE_STRDUP)

])


# ESO_FUNC_STPCPY
#----------------
AC_DEFUN([ESO_FUNC_STPCPY],
[

    AH_TEMPLATE([HAVE_STPCPY],
                [Define if you have the `stpcpy' function])

    ESO_CHECK_FUNC(stpcpy, [#include <stpcpy.h>], HAVE_STPCPY)

])


# ESO_FUNC_SYMLINK
#-----------------
AC_DEFUN([ESO_FUNC_SYMLINK],
[

    AH_TEMPLATE([HAVE_SYMLINK],
                [Define if you have the `symlink' function])

    ESO_CHECK_FUNC(symlink, [#include <unistd.h>], HAVE_SYMLINK)

])


# ESO_FUNC_WORDEXP
#-----------------
AC_DEFUN([ESO_FUNC_WORDEXP],
[

    AH_TEMPLATE([HAVE_WORDEXP],
                [Define if you have the `wordexp' function])

    ESO_CHECK_FUNC(wordexp, [#include <wordexp.h>], HAVE_WORDEXP)

])


# ESO_FUNC_GETTIMEOFDAY
#----------------------
AC_DEFUN([ESO_FUNC_GETTIMEOFDAY],
[

    AH_TEMPLATE([HAVE_GETTIMEOFDAY],
                [Define if you have the `gettimeofday' function])

    ESO_CHECK_FUNC(gettimeofday,
                   [
                       #include <unistd.h>
                       #include <sys/time.h>
                   ],
                   HAVE_GETTIMEOFDAY)
])


# ESO_FUNC_VA_COPY(symbol)
#-------------------------
# Check for an implementation of va_copy(). The argument which must be
# given is the preprocessor symbol that is defined to be either va_copy
# or __va_copy depending on the available function, provided that an
# implementation of va_copy is available at all.
AC_DEFUN([ESO_FUNC_VA_COPY],
[

    # Check for all three va_copy possibilities, so we get
    # all results in config.log for bug reports.

    # Check for availability of va_copy(). This is ISO C. Available with
    # gcc since version 3.0.
    AC_CACHE_CHECK([for an implementation of va_copy()],
                   [eso_cv_have_va_copy],
                   [
                       AC_RUN_IFELSE(
                       [
AC_LANG_PROGRAM(
[[
#include <stdlib.h>
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif

void f(int i, ...)
{
  va_list args1, args2;
  va_start (args1, i);
  va_copy (args2, args1);
   
  if (va_arg (args2, int) != 42 || va_arg (args1, int) != 42)
    exit (1);
   
  va_end (args1);
  va_end (args2);
}
]],
[[
  f(0, 42);
]])
                       ],
                       [eso_cv_have_va_copy=yes],
                       [eso_cv_have_va_copy=no],
                       [eso_cv_have_va_copy=no])
                   ])


    # Check for availability of __va_copy(). Some compilers provide
    # this. Available with gcc since version 2.8.1.
    AC_CACHE_CHECK([for an implementation of __va_copy()],
                   [eso_cv_have__va_copy],
                   [
                       AC_RUN_IFELSE(
                       [
AC_LANG_PROGRAM(
[[
#include <stdlib.h>
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif

void f(int i, ...)
{
  va_list args1, args2;

  va_start (args1, i);
  __va_copy (args2, args1);

  if (va_arg (args2, int) != 42 || va_arg (args1, int) != 42)
    exit (1);

  va_end (args1);
  va_end (args2);
}
]],
[[
  f(0, 42);
]])
                       ],
                       [eso_cv_have__va_copy=yes],
                       [eso_cv_have__va_copy=no],
                       [eso_cv_have__va_copy=no])
                   ])

    AH_TEMPLATE([HAVE_VA_COPY],
                [Define if you have an implementation of `va_copy()'.])
    AH_TEMPLATE([HAVE___VA_COPY],
                [Define if you have an implementation of `__va_copy()'.])

    if test "x$eso_cv_have_va_copy" = "xyes"; then
        eso_func_va_copy=va_copy
        AC_DEFINE(HAVE_VA_COPY)
    else
        if test "x$eso_cv_have__va_copy" = "xyes"; then
            eso_func_va_copy=__va_copy
            AC_DEFINE(HAVE___VA_COPY)
        fi
    fi

    AH_TEMPLATE([HAVE_VA_COPY_STYLE_FUNCTION],
                [Define if you have an implementation of a `va_copy()' style
                 function.])
    AH_TEMPLATE([$1], [A `va_copy()' style function])

    if test -n "$eso_func_va_copy"; then
        AC_DEFINE_UNQUOTED([$1], $eso_func_va_copy)
        AC_DEFINE(HAVE_VA_COPY_STYLE_FUNCTION)
    fi

    # Check whether va_lists can be copied by value
    AC_CACHE_CHECK([whether va_lists can be copied by value],
                   [eso_cv_have_va_value_copy],
                   [
                       AC_RUN_IFELSE(
                       [
AC_LANG_PROGRAM(
[[                            
#include <stdlib.h>
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif

void f(int i, ...)
{
  va_list args1, args2;
  va_start (args1, i);
  args2 = args1;
    
  if (va_arg (args2, int) != 42 || va_arg (args1, int) != 42)
    exit (1);
    
  va_end (args1);
  va_end (args2);
}
]],
[[
  f(0, 42);
]])
                       ],
                       [eso_cv_have_va_value_copy=yes],
                       [eso_cv_have_va_value_copy=no],
                       [eso_cv_have_va_value_copy=no])
                   ])

    AH_TEMPLATE([HAVE_VA_LIST_COPY_BY_VALUE],
                [Define if `va_lists' can be copied by value])
    if test "x$eso_cv_have_va_value_copy" = "xyes"; then
        AC_DEFINE(HAVE_VA_LIST_COPY_BY_VALUE)
    fi

])


# ESO_FUNC_REALLOC_SANITY
#-------------------------
# Check whether realloc(NULL,) works.
AC_DEFUN([ESO_FUNC_REALLOC_SANITY],
[
    AC_CACHE_CHECK([whether realloc(NULL,) works],
                   [eso_cv_have_sane_realloc],
                   [
                       AC_RUN_IFELSE(
                       [
AC_LANG_PROGRAM(
[[
#include <stdlib.h>
]],
[[
  return realloc (0, sizeof (int)) == 0;
]])
                       ],
                       [eso_cv_have_sane_realloc=yes],
                       [eso_cv_have_sane_realloc=no],
                       [eso_cv_have_sane_realloc=no])
                   ])

    AH_TEMPLATE([HAVE_WORKING_REALLOC],
                [Define if realloc(NULL,) works])

    if test x$eso_cv_have_sane_realloc = xyes; then
        AC_DEFINE(HAVE_WORKING_REALLOC)
    fi

])


# ESO_FIND_FILE(file, directories, variable)
#------------------------------------------
# Search for file in directories. Set variable to the first location
# where file was found, if file is not found at all variable is set to NO.
AC_DEFUN([ESO_FIND_FILE],
[
    $3=no

    for i in $2; do
        for j in $1; do

            echo "configure: __oline__: $i/$j" >&AC_FD_CC

            if test -r "$i/$j"; then
                echo "taking that" >&AC_FD_CC
                $3=$i
                break 2
            fi
        done
    done
])


# ESO_SET_LIBRARY_VERSION([CURRENT], [REVISION], [AGE])
#------------------------------------------------------
# Sets the libtool versioning symbols LT_CURRENT, LT_REVISION, LT_AGE.
AC_DEFUN([ESO_SET_LIBRARY_VERSION],
[

    if test -z "$1"; then
        LT_CURRENT=0
    else
        LT_CURRENT="$1"
    fi

    if test -z "$2"; then
        LT_REVISION=0
    else
        LT_REVISION="$2"
    fi

    if test -z "$3"; then
        LT_AGE=0
    else
        LT_AGE="$3"
    fi

    AC_SUBST(LT_CURRENT)
    AC_SUBST(LT_REVISION)
    AC_SUBST(LT_AGE)
])
