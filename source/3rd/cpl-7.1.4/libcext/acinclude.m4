# CEXT_ENABLE_DEBUG(debug=no)
#----------------------------
AC_DEFUN([CEXT_ENABLE_DEBUG],
[
    AC_REQUIRE([AC_PROG_CC])

    ESO_ENABLE_DEBUG([$1])

    if test x"$eso_cv_enable_debug" != xno; then
        CX_DEBUG_FLAGS="-DCX_ENABLE_DEBUG"
    else
        CX_DEBUG_FLAGS="-DCX_DISABLE_ASSERT"
    fi

    AC_SUBST(CX_DEBUG_FLAGS)
])


# CEXT_ENABLE_THREADS(threads=yes)
#---------------------------------
AC_DEFUN([CEXT_ENABLE_THREADS],
[
    AC_ARG_ENABLE(threads,
                  AC_HELP_STRING([--enable-threads],
                                 [enables thread support [default=$1]]),
                  cext_enable_threads=$enableval, cext_enable_threads=$1)

    AH_TEMPLATE([CX_THREADS_ENABLED],
                [Define if thread support is enabled.])

    if test x"$cext_enable_threads" = xyes; then
    
        ESO_CHECK_THREADS_POSIX
    
        AC_MSG_CHECKING([whether POSIX threads are available])
    
        if test x"$eso_cv_threads_posix" = xno; then
            AC_MSG_RESULT([no])
        else
        
            CFLAGS="$CFLAGS -D_REENTRANT"
            AC_DEFINE_UNQUOTED([CX_THREADS_ENABLED])
            
            if test x"$eso_cv_threads_posix_lib" = xyes; then
               echo $LIBS | grep -q -e "$LIBPTHREAD" || LIBS="$LIBPTHREAD $LIBS" 
            else
               CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
               LDFLAGS="$LDFLAGS $PTHREAD_CFLAGS"
            fi
                        
            AC_MSG_RESULT([yes])
        fi
        
    fi
    
])


# CEXT_CHECK_CHAR_BIT
#--------------------
# Check number of bits used per char.
AC_DEFUN([CEXT_CHECK_CHAR_BIT],
[
    cext_have_char_bit=no
    cext_have_char_8_bits=no

    AC_MSG_CHECKING([for bits per char])
    
    AC_CACHE_VAL(cext_cv_have_char_8_bits,
    [
        AC_LANG_PUSH(C)

        cext_cppflags_save="$CPPFLAGS"
        cext_cflags_save="$CFLAGS"
        cext_ldflags_save="$LDFLAGS"
        cext_libs_save="$LIBS"

        if test "$GCC" = "yes"; then
            CFLAGS="$CFLAGS -pedantic-errors"
            CPPFLAGS="$CPPFLAGS $CFLAGS"
        fi

        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include <limits.h>],
                                          [int i = CHAR_BIT;])],
                          cext_have_char_bit=yes,
                          cext_have_char_bit=no)

        if test "x$cext_have_char_bit" = "xyes"; then
            AC_RUN_IFELSE([AC_LANG_SOURCE(
            [
                #include <limits.h>

                int main() {

                  if (CHAR_BIT != 8)
                    return 1;

                  return 0;
                }
            ])], cext_have_char_8_bits=yes, cext_have_char_8_bits=no)
        else
            AC_RUN_IFELSE([AC_LANG_SOURCE(
            [
                #include <limits.h>

                int main() {

                  char c = 1;
                  int i = 0;

                  while (c) {
                    c <<= 1;
                    i++;
                  }

                  if (i != 8)
                    return 1;

                  return 0;
                }
            ])], cext_have_char_8_bits=yes, cext_have_char_8_bits=no)
        fi

        CPPFLAGS="$cext_cppflags_save"
        CFLAGS="$cext_cflags_save"
        LDFLAGS="$cext_ldflags_save"
        LIBS="$cext_libs_save"

        AC_LANG_POP(C)

        cext_cv_have_char_8_bits="cext_have_char_bit=$cext_have_char_bit \
        cext_have_char_8_bits=$cext_have_char_8_bits"

    ])

    eval "$cext_cv_have_char_8_bits"

    if test "$cext_have_char_8_bits" = "no"; then
        AC_MSG_ERROR([C type char is not 8 bits wide!])
    else
        if test x"$cext_have_char_bit" = xno; then
            AC_DEFINE(CHAR_BIT, 8, [Width of type 'char'])
        else
            AC_DEFINE(HAVE_CHAR_BIT, 1,
                      [Define if CHAR_BIT is defined in limits.h and equals 8])
        fi
        AC_MSG_RESULT([8])
    fi
])


#
# CEXT_CHECK_SIZE_T
#------------------
# Check for an integer type with an equal size 
AC_DEFUN([CEXT_CHECK_SIZE_T],
[
    AC_MSG_CHECKING([for an integer type with the same size as size_t])
    
    case $ac_cv_sizeof_size_t in
    $ac_cv_sizeof_short)
        cext_size_type='short'
        ;;
    $ac_cv_sizeof_int)
        cext_size_type='int'
        ;;
    $ac_cv_sizeof_long)
        cext_size_type='long'
        ;;
    $ac_cv_sizeof_long_long)
        cext_size_type='long long'
        ;;
    *)
        AC_MSG_ERROR([No integer type matches size_t in size])
        ;;
    esac
        
    AC_MSG_RESULT([$cext_size_type])
    
    AC_CACHE_VAL(cext_cv_type_size_type,
                 [
                    cext_cv_type_size_type=$cext_size_type
                 ])
                 
])


#
# CEXT_CHECK_FORMAT_LONG_LONG
#----------------------------
# Check printing format specifier used for long long integers
AC_DEFUN([CEXT_CHECK_FORMAT_INT64],
[
    # List of printf formats to check for
    format_list="ll q"
    
    if test x"$ac_cv_sizeof_long_long" = x8; then
    
        AC_MSG_CHECKING([for printf format used for 64-bit integers])
        AC_CACHE_VAL(cext_cv_format_long_long,
                    [
                        for format in $format_list; do
                            AC_RUN_IFELSE([AC_LANG_SOURCE(
                            [[
                                #include <stdio.h>
                            
                                int main()
                                {
                                    long long b, a = -0x3AFAFAFAFAFAFAFALL;
                                    char buffer[1000];
                                    sprintf (buffer, "%${fmt}u", a);
                                    sscanf (buffer, "%${fmt}u", &b);
                                    return b != a;
                                }
                            ]])],
                            [cext_cv_format_long_long=${fmt}; break],
                            [], [:])
                        done
                    ])
        
        if test -n "$cext_cv_format_long_long"; then
            AC_MSG_RESULT([%${cext_cv_format_long_long}u])
            AC_DEFINE(HAVE_LONG_LONG_FORMAT, 1, [Define if the system printf can print a long long])
        else
            AC_MSG_RESULT([none])
        fi            
    fi
])


# The following type check macros are replacement versions for the actual
# autoconf checks, which are more type specific. However the actual autoconf
# macros are not available for autoconf version 2.59 (and older) which we
# still need to support for a while.
  
# CEXT_TYPE_LONG_LONG_INT
#------------------------
# Check whether the compiler supports the 'long long int' type.
AC_DEFUN([CEXT_TYPE_LONG_LONG_INT],
[

    AC_CHECK_TYPE([long long int], [], [], [])
                      
    if test x"$ac_cv_type_long_long_int" = xyes; then
        AC_DEFINE([HAVE_LONG_LONG_INT], [1],
                  [Define to 1 if the system has the type `long long int'.])
    fi
            
])


# CEXT_TYPE_UNSIGNED_LONG_LONG_INT
#---------------------------------
# Check whether the compiler supports the 'unsigned long long int' type.
AC_DEFUN([CEXT_TYPE_UNSIGNED_LONG_LONG_INT],
[

    AC_CHECK_TYPE([unsigned long long int], [], [], [])
                      
    if test x"$ac_cv_type_unsigned_long_long_int" = xyes; then
        AC_DEFINE([HAVE_UNSIGNED_LONG_LONG_INT], [1],
                  [Define to 1 if the system has the type `unsigned long long int'.])
    fi
            
])


# CEXT_TYPE_INTMAX_T
# ------------------
# Check whether the compiler supports the 'intmax_t' type.
AC_DEFUN([CEXT_TYPE_INTMAX_T],
[

    AC_REQUIRE([CEXT_TYPE_LONG_LONG_INT])
    AC_CHECK_TYPE([intmax_t],
                  [
                      AC_DEFINE([HAVE_INTMAX_T], [1],
                                 [Define to 1 if the system has the type `intmax_t'.])
                  ],
                  [
                      test $ac_cv_type_long_long_int = yes \
                            && ac_type='long long int' \
                            || ac_type='long int'
                       AC_DEFINE_UNQUOTED([intmax_t], [$ac_type],
                                          [Define to the widest signed integer type if <stdint.h> and <inttypes.h> do not define.])
                  ])
                 
])


# CEXT_TYPE_UINTMAX_T
# -------------------
# Check whether the compiler supports the 'uintmax_t' type.
AC_DEFUN([CEXT_TYPE_UINTMAX_T],
[

    AC_REQUIRE([CEXT_TYPE_UNSIGNED_LONG_LONG_INT])
    AC_CHECK_TYPE([uintmax_t],
                  [
                      AC_DEFINE([HAVE_UINTMAX_T], [1],
                                 [Define to 1 if the system has the type `uintmax_t'.])
                  ],
                  [
                      test $ac_cv_type_long_long_int = yes \
                            && ac_type='unsigned long long int' \
                            || ac_type='unsigned long int'
                       AC_DEFINE_UNQUOTED([uintmax_t], [$ac_type],
                                          [Define to the widest unsigned integer type if <stdint.h> and <inttypes.h> do not define.])
                  ])
                 
])


# CEXT_TYPE_UINTPTR_T
# -------------------
# Check whether the compiler supports the 'uintptr_t' type.
AC_DEFUN([CEXT_TYPE_UINTPTR_T],
[
    AC_CHECK_TYPE([uintptr_t],
                  [
                       AC_DEFINE([HAVE_UINTPTR_T], 1,
                                 [Define to 1 if the system has the type `uintptr_t'.])
                  ],
                  [
                       for ac_type in 'unsigned int' 'unsigned long int' \
                         'unsigned long long int'; do
                           AC_COMPILE_IFELSE(
                           [
                               AC_LANG_BOOL_COMPILE_TRY([AC_INCLUDES_DEFAULT],
                                                        [[sizeof (void *) <= sizeof ($ac_type)]])
                           ],
                           [
                               AC_DEFINE_UNQUOTED([uintptr_t], [$ac_type],
                                                  [Define to the type of an unsigned integer type wide enough to hold a pointer, if such a type exists, and if the system does not define it.])
                               ac_type=
                           ])
                           test -z "$ac_type" && break
                           done
                  ])

])


# CEXT_TYPE_LONG_DOUBLE
# ---------------------
# Check whether the compiler supports the 'long double' type.
AC_DEFUN([CEXT_TYPE_LONG_DOUBLE],
[

    AC_CACHE_CHECK([for long double], [ac_cv_type_long_double],
                   [
                       if test "$GCC" = yes; then
                           ac_cv_type_long_double=yes
                       else
                           AC_COMPILE_IFELSE(
                           [
                               AC_LANG_BOOL_COMPILE_TRY(
                               [[
                                 /* The Stardent Vistra knows sizeof (long double), but does not support it.  */
                                 long double foo = 0.0L;
                               ]],
                               [[/* On Ultrix 4.3 cc, long double is 4 and double is 8.  */
                                 sizeof (double) <= sizeof (long double)]])],
                               [ac_cv_type_long_double=yes],
                               [ac_cv_type_long_double=no])
                       fi
                   ])
    if test $ac_cv_type_long_double = yes; then
        AC_DEFINE([HAVE_LONG_DOUBLE], [1],
                  [Define to 1 if the system has the type `long double'.])
    fi
    
])


#
# CEXT_SET_PATHS
#---------------
# Define auxiliary directories of the installed directory tree.
AC_DEFUN([CEXT_SET_PATHS],
[

    if test -z "$apidocdir"; then
        apidocdir='${datadir}/doc/${PACKAGE}/html'
    fi

    AC_SUBST(apidocdir)

    if test -z "$pkgincludedir"; then
        pkgincludedir='${includedir}/cext'
    fi
       
    if test -z "$configdir"; then
        configdir='${includedir}'
    fi

    AC_SUBST(configdir)
    
    if test -z "$pkgconfigdir"; then
        pkgconfigdir='${libdir}/pkgconfig'
    fi

    AC_SUBST(pkgconfigdir)

])


# CEXT_SET_SYMBOLS
#-----------------
# Setup various Makefile symbols used for building the package
AC_DEFUN([CEXT_SET_SYMBOLS],
[

    CEXT_INCLUDES='-I$(top_builddir)/cext -I$(top_srcdir)'
    AC_SUBST(CEXT_INCLUDES)
    
])


# CEXT_CREATE_CXCONFIG
#---------------------
# Create the source for cxconfig.h in cext
AC_DEFUN([CEXT_CREATE_CXCONFIG],
[
    AC_REQUIRE([CEXT_CHECK_CHAR_BIT])
    
    AC_CONFIG_COMMANDS(cxconfig.h,
                       [cfgfile="cext/cxconfig.h"
                        tcfgfile="$cfgfile-tmp"
                        cat > $tcfgfile << _CEXTEOF
/*
 * cxconfig.h: This is a generated file! Do not edit this file!
 *             All changes will be lost!
 */
 
#ifndef CXCONFIG_H_
#define CXCONFIG_H_

_CEXTEOF

                        if test x$cext_have_limits_h = xyes; then
                            echo '#include <limits.h>' >> $tcfgfile
                        fi

                        if test x$cext_have_float_h = xyes; then
                            echo '#include <float.h>' >> $tcfgfile
                        fi

                        if test x$cext_have_values_h = xyes; then
                            echo '#include <values.h>' >> $tcfgfile
                        fi

                        if test x$cext_have_stdint_h = xyes; then
                            echo '#include <stdint.h>' >> $tcfgfile
                        fi

                        if test x$cext_have_inttypes_h = xyes; then
                            echo '#include <inttypes.h>' >> $tcfgfile
                        fi

                        cat >> $tcfgfile << _CEXTEOF

#include <cxmacros.h>


CX_BEGIN_DECLS

/*
 * Limits for numerical data types
 */

#define CX_MINSHORT  $cext_cxshort_min
#define CX_MAXSHORT  $cext_cxshort_max
#define CX_MAXUSHORT $cext_cxushort_max

#define CX_MININT    $cext_cxint_min
#define CX_MAXINT    $cext_cxint_max
#define CX_MAXUINT   $cext_cxuint_max

#define CX_MINLONG   $cext_cxlong_min
#define CX_MAXLONG   $cext_cxlong_max
#define CX_MAXULONG  $cext_cxulong_max

#define CX_MINFLOAT  $cext_cxfloat_min
#define CX_MAXFLOAT  $cext_cxfloat_max

#define CX_MINDOUBLE $cext_cxdouble_min
#define CX_MAXDOUBLE $cext_cxdouble_max
_CEXTEOF

                        # This should be true for any modern C/C++ compiler
                        # In addition this has been verified before by
                        # the CX_CHECK_CHAR_BITS() call.
                        if test x"$cext_have_char_8_bits" = xyes; then
                            cat >> $tcfgfile << _CEXTEOF
                          
                          
/*
 * Number of bits per char
 */
 
#define CX_CHAR_BIT 8
_CEXTEOF

                        else
                            echo '#error "Type char is not 8 bits wide!"' >> $tcfgfile
                        fi
                        
                        cat >> $tcfgfile << _CEXTEOF
                        
/*
 * Fixed size integer types
 */
_CEXTEOF

                        cat >> $tcfgfile << _CEXTEOF
                        
/* Macros for formatted output */
 
#define CX_PRINTF_FORMAT_INT8    $cxint8_print_format
#define CX_PRINTF_FORMAT_UINT8   $cxuint8_print_format

#define CX_PRINTF_FORMAT_INT16    $cxint16_print_format
#define CX_PRINTF_FORMAT_UINT16   $cxuint16_print_format

#define CX_PRINTF_FORMAT_INT32    $cxint32_print_format
#define CX_PRINTF_FORMAT_UINT32   $cxuint32_print_format

#define CX_PRINTF_FORMAT_INT64    $cxint64_print_format
#define CX_PRINTF_FORMAT_UINT64   $cxuint64_print_format

/* Macros for formatted output */

#define CX_SCANF_FORMAT_INT8    $cxint8_scan_format
#define CX_SCANF_FORMAT_UINT8   $cxuint8_scan_format

#define CX_SCANF_FORMAT_INT16    $cxint16_scan_format
#define CX_SCANF_FORMAT_UINT16   $cxuint16_scan_format

#define CX_SCANF_FORMAT_INT32    $cxint32_scan_format
#define CX_SCANF_FORMAT_UINT32   $cxuint32_scan_format

#define CX_SCANF_FORMAT_INT64    $cxint64_scan_format
#define CX_SCANF_FORMAT_UINT64   $cxuint64_scan_format
_CEXTEOF

                        if test -n "$cxint8"; then
                            cat >> $tcfgfile << _CEXTEOF
                            
/* Type definitions */

typedef $cxint8 cxint8;
typedef $cxuint8 cxuint8;
_CEXTEOF
                        fi
                        
                        if test -n "$cxint16"; then
                            cat >> $tcfgfile << _CEXTEOF
                            
typedef $cxint16 cxint16;
typedef $cxuint16 cxuint16;
_CEXTEOF
                        fi

                        if test -n "$cxint32"; then
                            cat >> $tcfgfile << _CEXTEOF
                            
typedef $cxint32 cxint32;
typedef $cxuint32 cxuint32;
_CEXTEOF
                        fi

                        if test -n "$cxint64"; then
                            cat >> $tcfgfile << _CEXTEOF
                            
${cext_extension} typedef $cxint64 cxint64;
${cext_extension} typedef $cxuint64 cxuint64;

#define CX_INT64_CONSTANT(val)   $cxint64_constant
#define CX_UINT64_CONSTANT(val)  $cxuint64_constant
_CEXTEOF
                        fi

                        cat >> $tcfgfile << _CEXTEOF

#define CX_SIZEOF_VOID_P  $cext_void_p
#define CX_SIZEOF_SIZE_T  $cext_size_t
_CEXTEOF

                        cat >> $tcfgfile << _CEXTEOF

/*
 * Size type
 */
 
#define CX_PRINTF_FORMAT_SIZE_TYPE    $cxsize_print_format
#define CX_PRINTF_FORMAT_SSIZE_TYPE   $cxssize_print_format

#define CX_SCANF_FORMAT_SIZE_TYPE    $cxsize_scan_format
#define CX_SCANF_FORMAT_SSIZE_TYPE   $cxssize_scan_format

typedef signed $cext_size_type_define cxssize;                         
typedef unsigned $cext_size_type_define cxsize;

#define CX_MINSSIZE  CX_MIN$cext_msize_type                         
#define CX_MAXSSIZE  CX_MAX$cext_msize_type                         
#define CX_MAXSIZE   CX_MAXU$cext_msize_type                         

                         
typedef cxint64 cxoffset;

#define CX_MINOFFSET CX_MININT64
#define CX_MAXOFFSET CX_MAXINT64
_CEXTEOF

                        if test -z "$cext_unknown_void_p"; then
                            cat >> $tcfgfile << _CEXTEOF

/*
 * Pointer to integer conversion
 */
 
#define CX_POINTER_TO_INT(ptr)   ((cxint) ${cext_ptoi_cast} (ptr))
#define CX_POINTER_TO_UINT(ptr)  ((cxint) ${cext_ptoui_cast} (ptr))

#define CX_INT_TO_POINTER(val)   ((cxptr) ${cext_ptoi_cast} (val))
#define CX_UINT_TO_POINTER(val)   ((cxptr) ${cext_ptoui_cast} (val))
_CEXTEOF
                        else
                            echo '#error Size of generic pointer is unknown - This should never happen' >> $tcfgfile
                        fi
                        
                        cat >> $tcfgfile << _CEXTEOF

#ifdef __cplusplus
#  define CX_HAVE_INLINE  1
#else
$cext_inline
#endif

#ifdef __cplusplus
#  define CX_CAN_INLINE  1
_CEXTEOF
                        if test x"$cext_can_inline" = xyes; then
                            cat >> $tcfgfile << _CEXTEOF
#else
#  define CX_CAN_INLINE  1
_CEXTEOF
                        fi
                        
                        cat >> $tcfgfile << _CEXTEOF
#endif
_CEXTEOF

                        if test x"$cext_have_gnuc_visibility" = xyes; then
                            cat >> $tcfgfile << _CEXTEOF
                            
#define CX_HAVE_GNUC_VISIBILITY 1
_CEXTEOF
                        fi
                        
                        cat >> $tcfgfile << _CEXTEOF
                        
#if defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590)
#  define CX_GNUC_INTERNAL __attribute__((visibility("hidden")))
#elif defined(__SUNPRO_C) && (__SUNPRO_C >= 0x550)
#  define CX_GNUC_INTERNAL __hidden
#elif defined (__GNUC__) && defined (CX_HAVE_GNUC_VISIBILITY)
#  define CX_GNUC_INTERNAL __attribute__((visibility("hidden")))
#else
#  define CX_GNUC_INTERNAL  /* empty */
#endif
_CEXTEOF

                        cat >> $tcfgfile << _CEXTEOF

CX_END_DECLS

#endif /* CXCONFIG_H_ */
_CEXTEOF
                        
                        if test -f $cfgfile; then
                            if cmp -s $tcfgfile $cfgfile; then
                                AC_MSG_NOTICE([$cfgfile is unchanged])
                                rm -f $tcfgfile
                            else
                                mv $tcfgfile $cfgfile
                            fi
                        else
                            mv $tcfgfile $cfgfile
                        fi
                       ],
                       [
                        # Supported compiler attributes
                        if test x"$eso_cv_prog_cc_attribute_visibility_hidden" = xyes; then
                            cext_have_gnuc_visibility=yes
                        fi
                        
                        # Number of bits per char
                        eval $cext_cv_have_char_8_bits
                        
                        if test x"$ac_cv_type_int8_t" = xyes; then
                            cxint8=int8_t
                            cxuint8=uint8_t
                            cxint8_print_format='PRIi8'
                            cxuint8_print_format='PRIu8'
                            cxint8_scan_format='SCNi8'
                            cxuint8_scan_format='SCNu8'
                        else
                            cxint8='signed char'
                            cxuint8='unsigned char'
                            cxint8_print_format='"hhi"'
                            cxuint8_print_format='"hhu"'
                            cxint8_scan_format='"hhi"'
                            cxuint8_scan_format='"hhu"'
                        fi
                                                
                        # 16 bit integers
                        if test x"$ac_cv_type_int16_t" = xyes; then
                            cxint16=int16_t
                            cxuint16=uint16_t
                            cxint16_print_format='PRIi16'
                            cxuint16_print_format='PRIu16'
                            cxint16_scan_format='SCNi16'
                            cxuint16_scan_format='SCNu16'
                        else
                            case 2 in
                            $ac_cv_sizeof_short)
                                cxint16='signed short'
                                cxuint16='unsigned short'
                                cxint16_print_format='"hi"'
                                cxuint16_print_format='"hu"'
                                cxint16_scan_format='"hi"'
                                cxuint16_scan_format='"hu"'
                                ;;
                            $ac_cv_sizeof_int)
                                cxint16='signed int'
                                cxuint16='unsigned int'
                                cxint16_print_format='"i"'
                                cxuint16_print_format='"u"'
                                cxint16_scan_format='"i"'
                                cxuint16_scan_format='"u"'
                                ;;
                            esac
                        fi

                        # 32 bit integers
                        if test x"$ac_cv_type_int32_t" = xyes; then
                            cxint32=int32_t
                            cxuint32=uint32_t
                            cxint32_print_format='PRIi32'
                            cxuint32_print_format='PRIu32'
                            cxint32_scan_format='SCNi32'
                            cxuint32_scan_format='SCNu32'
                        else
                            case 4 in
                            $ac_cv_sizeof_short)
                                cxint32='signed short'
                                cxuint32='unsigned short'
                                cxint32_print_format='"hi"'
                                cxuint32_print_format='"hu"'
                                cxint32_scan_format='"hi"'
                                cxuint32_scan_format='"hu"'
                                ;;
                            $ac_cv_sizeof_int)
                                cxint32='signed int'
                                cxuint32='unsigned int'
                                cxint32_print_format='"i"'
                                cxuint32_print_format='"u"'
                                cxint32_scan_format='"i"'
                                cxuint32_scan_format='"u"'
                                ;;
                            $ac_cv_sizeof_long)
                                cxint32='signed long'
                                cxuint32='unsigned long'
                                cxint32_print_format='"li"'
                                cxuint32_print_format='"lu"'
                                cxint32_scan_format='"li"'
                                cxuint32_scan_format='"lu"'
                                ;;
                            esac
                        fi

                        # 64 bit integers
                        if test x"$ac_cv_type_int64_t" = xyes; then
                            cxint64=int64_t
                            cxuint64=uint64_t
                            cxint64_print_format='PRIi64'
                            cxuint64_print_format='PRIu64'
                            cxint64_scan_format='SCNi64'
                            cxuint64_scan_format='SCNu64'
                            
                            case 8 in
                            $ac_cv_sizeof_int)
                                cxint64_constant='(val)'
                                cxuint64_constant='(val)'
                                ;;
                            $ac_cv_sizeof_long)
                                cxint64_constant='(val##L)'
                                cxuint64_constant='(val##UL)'
                                ;;
                            $ac_cv_sizeof_long_long)
                                cext_extension="CX_GNUC_EXTENSION "
                                cxint64_constant='(CX_GNUC_EXTENSION (val##LL))'
                                cxuint64_constant='(CX_GNUC_EXTENSION (val##ULL))'
                                ;;
                            esac
                        else
                            case 8 in
                            $ac_cv_sizeof_int)
                                cxint64='signed int'
                                cxuint64='unsigned int'
                                cxint64_print_format='"i"'
                                cxuint64_print_format='"u"'
                                cxint64_scan_format='"i"'
                                cxuint64_scan_format='"u"'
                                cext_extension=""
                                cxint64_constant='(val)'
                                cxuint64_constant='(val)'
                                ;;
                            $ac_cv_sizeof_long)
                                cxint64='signed long'
                                cxuint64='unsigned long'
                                cxint64_print_format='"li"'
                                cxuint64_print_format='"lu"'
                                cxint64_scan_format='"li"'
                                cxuint64_scan_format='"lu"'
                                cext_extension=""
                                cxint64_constant='(val##L)'
                                cxuint64_constant='(val##UL)'
                                ;;
                            $ac_cv_sizeof_long_long)
                                cxint64='signed long long'
                                cxuint64='unsigned long long'
                                if test -n "$cext_cv_format_long_long"; then
                                    cxint64_print_format='"'$cext_cv_format_long_long'i"'
                                    cxuint64_print_format='"'$cext_cv_format_long_long'u"'
                                    cxint64_scan_format='"'$cext_cv_format_long_long'i"'
                                    cxuint64_scan_format='"'$cext_cv_format_long_long'u"'
                                fi
                                cext_extension="CX_GNUC_EXTENSION "
                                cxint64_constant='(CX_GNUC_EXTENSION (val##LL))'
                                cxuint64_constant='(CX_GNUC_EXTENSION (val##ULL))'
                                ;;
                            esac
                        fi
                        
                        # Sizes of types
                        cext_size_t="$ac_cv_sizeof_size_t"
                        cext_void_p="$ac_cv_sizeof_void_p"
                        
                        # Size type
                        cext_size_type_define="$cext_cv_type_size_type"
                        
                        case $cext_cv_type_size_type in
                        short)
                            cxsize_print_format='"hu"'
                            cxssize_print_format='"hi"'
                            cxsize_scan_format='"hu"'
                            cxssize_scan_format='"hi"'
                            cext_msize_type='SHRT'
                            ;;
                        int)
                            cxsize_print_format='"u"'
                            cxssize_print_format='"i"'
                            cxsize_scan_format='"u"'
                            cxssize_scan_format='"i"'
                            cext_msize_type='INT'
                            ;;
                        long)
                            cxsize_print_format='"lu"'
                            cxssize_print_format='"li"'
                            cxsize_scan_format='"lu"'
                            cxssize_scan_format='"li"'
                            cext_msize_type='LONG'
                            ;;
                        "long long")
                            cxsize_print_format='"'$cext_cv_format_long_long'u"'
                            cxssize_print_format='"'$cext_cv_format_long_long'i"'
                            cxsize_scan_format='"'$cext_cv_format_long_long'u"'
                            cxssize_scan_format='"'$cext_cv_format_long_long'i"'
                            cext_msize_type='INT64'
                            ;;
                        esac
                        
                        # Pointer integer conversions
                        case $ac_cv_sizeof_void_p in
                        $ac_cv_sizeof_int)
                            cext_ptoi_cast=''
                            cext_ptoui_cast=''
                            ;;
                        $ac_cv_sizeof_long)
                            cext_ptoi_cast='(cxlong)'
                            cext_ptoui_cast='(cxlong)'
                            ;;
                        $ac_cv_sizeof_long_long)
                            cext_ptoi_cast='(cxint64)'
                            cext_ptoui_cast='(cxint64)'
                            ;;
                        *)
                            cext_unknown_void_p=yes
                            ;;
                        esac
                        
                        # Standard integer types
                        if test x$ac_cv_header_stdint_h = xyes; then
                            cext_have_stdint_h=yes
                        fi

                        if test x$ac_cv_header_inttypes_h = xyes; then
                            cext_have_inttypes_h=yes
                        fi
                        
                        # type limits
                        case xyes in
                        x$ac_cv_header_limits_h)
                            cext_have_limits_h=yes
                            cext_cxshort_min=SHRT_MIN
                            cext_cxshort_max=SHRT_MAX
                            cext_cxushort_max=USHRT_MAX
                            cext_cxint_min=INT_MIN
                            cext_cxint_max=INT_MAX
                            cext_cxuint_max=UINT_MAX
                            cext_cxlong_min=LONG_MIN
                            cext_cxlong_max=LONG_MAX
                            cext_cxulong_max=ULONG_MAX
                            ;;
                        x$ac_cv_header_values_h)
                            cext_have_values_h=yes
                            cext_cxshort_min=MINSHORT
                            cext_cxshort_max=MAXSHORT
                            cext_cxushort_max="(((cxushort)CX_MAXSHORT)*2+1)"
                            cext_cxint_min=MININT
                            cext_cxint_max=MAXINT
                            cext_cxuint_max="(((cxuint)CX_MAXINT)*2+1)"
                            cext_cxlong_min=MINLONG
                            cext_cxlong_max=MAXLONG
                            cext_cxulong_max="(((cxulong)CX_MAXLONG)*2+1)"
                            ;;
                        esac

                        case xyes in
                        x$ac_cv_header_float_h)
                            cext_have_float_h=yes
                            cext_cxfloat_min=FLT_MIN
                            cext_cxfloat_max=FLT_MAX
                            cext_cxdouble_min=DBL_MIN
                            cext_cxdouble_max=DBL_MAX
                            ;;
                        x$ac_cv_header_values_h)
                            cext_have_values_h=yes
                            cext_cxfloat_min=MINFLOAT
                            cext_cxfloat_max=MAXFLOAT
                            cext_cxdouble_min=MINDOUBLE
                            cext_cxdouble_max=MAXDOUBLE
                            ;;
                        esac
                       ])
])
