# CPL_CHECK_CFITSIO(version)
#---------------------------
# Checks for the cfitsio library and header files. If version is given the
# the given string is compared to the preprocessor symbols CFITSIO_MAJOR
# and CFITSIO_MINOR to verify that at least the given version is available.
AC_DEFUN([CPL_CHECK_CFITSIO],
[
    cpl_cfitsio_check_version="$1"
    cpl_cfitsio_check_header="fitsio.h"
    cpl_cfitsio_check_libraries="-lcfitsio -lpthread -lm "

    cpl_cfitsio_pkgconfig="cfitsio"


    AC_REQUIRE([AC_SYS_LARGEFILE])
    AC_REQUIRE([ESO_CHECK_THREADS_POSIX])
    AC_REQUIRE([ESO_PROG_PKGCONFIG])

    AC_ARG_WITH(cfitsio,
                AS_HELP_STRING([--with-cfitsio],
                               [location where cfitsio is installed]),
                [
                    cpl_with_cfitsio=$withval
                ])

    AC_ARG_WITH(cfitsio-includes,
                AS_HELP_STRING([--with-cfitsio-includes],
                               [location of the cfitsio header files]),
                cpl_with_cfitsio_includes=$withval)

    AC_ARG_WITH(cfitsio-libs,
                AS_HELP_STRING([--with-cfitsio-libs],
                               [location of the cfitsio library]),
                cpl_with_cfitsio_libs=$withval)

    AC_ARG_ENABLE(cfitsio-test,
                  AS_HELP_STRING([--disable-cfitsio-test],
                                 [disables checks for the cfitsio library and headers]),
                  cpl_enable_cfitsio_test=$enableval,
                  cpl_enable_cfitsio_test=yes)

    AC_ARG_VAR([CFITSIODIR], [Location where cfitsio is installed])


    if test x"$cpl_enable_cfitsio_test" = xyes; then
        
        AC_MSG_CHECKING([for cfitsio])
        
        cpl_cfitsio_libs="$cpl_cfitsio_check_libraries"
    
        cpl_cfitsio_cflags="-I/usr/include/cfitsio"
        cpl_cfitsio_ldflags=""
    
        if test -n "${PKGCONFIG}"; then
    
            $PKGCONFIG --exists $cpl_cfitsio_pkgconfig
    
            if test x$? = x0; then
                cpl_cfitsio_cflags="`$PKGCONFIG --cflags $cpl_cfitsio_pkgconfig`"
                cpl_cfitsio_ldflags="`$PKGCONFIG --libs-only-L $cpl_cfitsio_pkgconfig`"
                cpl_cfitsio_libs="`$PKGCONFIG --libs-only-l $cpl_cfitsio_pkgconfig`"
            fi
        
        fi
        
        if test -n "$CFITSIODIR"; then
            cpl_cfitsio_cflags="-I$CFITSIODIR/include/cfitsio -I$CFITSIODIR/include"
            cpl_cfitsio_ldflags="-L$CFITSIODIR/lib64 -L$CFITSIODIR/lib"
        fi
        
        if test -n "$cpl_with_cfitsio"; then
            cpl_cfitsio_cflags="-I$cpl_with_cfitsio/include/cfitsio -I$cpl_with_cfitsio/include"
            cpl_cfitsio_ldflags="-L$cpl_with_cfitsio/lib64 -L$cpl_with_cfitsio/lib"
        fi    
        
        if test -n "$cpl_with_cfitsio_includes"; then
            cpl_cfitsio_cflags="-I$cpl_with_cfitsio_includes"
        fi
        
        if test -n "$cpl_with_cfitsio_libs"; then
            cpl_cfitsio_ldflags="-L$cpl_with_cfitsio_libs"
        fi

        # Check whether the header files and the library are present and
        # whether they can be used.
        
        cpl_have_cfitsio_libraries="unknown"
        cpl_have_cfitsio_headers="unknown"
            
        AC_LANG_PUSH(C)
        
        cpl_cfitsio_cflags_save="$CFLAGS"
        cpl_cfitsio_ldflags_save="$LDFLAGS"
        cpl_cfitsio_libs_save="$LIBS"
    
        CFLAGS="$cpl_cfitsio_cflags"
        LDFLAGS="$cpl_cfitsio_ldflags"
        LIBS="$cpl_cfitsio_libs"
    
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
                          [[
                          #include <$cpl_cfitsio_check_header>
                          ]],
                          [
                           fitsfile *fp;
                          ])],
                          [
                           cpl_have_cfitsio_headers="yes"
                          ],
                          [
                           cpl_have_cfitsio_headers="no"
                          ])

        if test x"$cpl_have_cfitsio_headers" = xyes; then
            
            AC_LINK_IFELSE([AC_LANG_PROGRAM(
                           [[
                           #include <$cpl_cfitsio_check_header>
                           ]],
                           [
                            float v;
                            ffvers(&v);
                           ])],
                           [
                            cpl_have_cfitsio_libraries="yes"
                           ],
                           [
                            cpl_have_cfitsio_libraries="no"
                           ])

        fi
                                
        AC_LANG_POP(C)
       
        CFLAGS="$cpl_cfitsio_cflags_save"
        LDFLAGS="$cpl_cfitsio_ldflags_save"
        LIBS="$cpl_cfitsio_libs_save"
        
        if test x"$cpl_have_cfitsio_libraries" = xno || \
            test x"$cpl_have_cfitsio_headers" = xno; then
            cpl_cfitsio_notfound=""
    
            if test x"$cpl_have_cfitsio_headers" = xno; then
                if test x"$cpl_have_cfitsio_libraries" = xno; then
                    cpl_cfitsio_notfound="(headers and libraries)"
                else
                    cpl_cfitsio_notfound="(headers)"
                fi
            else
                cpl_cfitsio_notfound="(libraries)"
            fi
    
            AC_MSG_RESULT([no])            
            AC_MSG_ERROR([cfitsio $cpl_cfitsio_notfound was not found on your system.])
        
        else
    
            AC_MSG_RESULT([yes])
    
            # Setup the symbols
    
            CFITSIO_INCLUDES="$cpl_cfitsio_cflags"
            CFITSIO_CFLAGS="$cpl_cfitsio_cflags"
            CFITSIO_LDFLAGS="$cpl_cfitsio_ldflags"
            LIBCFITSIO="$cpl_cfitsio_libs"
            
            if test -n "$cpl_cfitsio_check_version"; then
            
                # Check library version
    
                AC_MSG_CHECKING([for a cfitsio version >= $cpl_cfitsio_check_version])
    
                cpl_cfitsio_cflags_save="$CFLAGS"
                cpl_cfitsio_ldflags_save="$LDFLAGS"
                cpl_cfitsio_libs_save="$LIBS"
    
    			# AC_COMPUTE_INT internally runs a test program and thus depents on the
                # current runtime environment configuration! Since the following test for
                # checking the version number only needs the CFITSIO headers and not the
                # library make sure that the generated test does not depend on it. Leaving
                # the library dependency in place here can actually make the test fail
                # if the library is not found in the library search path!

                CFLAGS="$CFITSIO_INCLUDES"
                LDFLAGS="$CFITSIO_LDFLAGS"
                LIBS=""
               
                cpl_cfitsio_version=""
                AC_COMPUTE_INT([cpl_cfitsio_major], [CFITSIO_MAJOR],
                               [#include <fitsio.h>], [cpl_cfitsio_version="no"])
                AC_COMPUTE_INT([cpl_cfitsio_minor], [CFITSIO_MINOR],
                               [#include <fitsio.h>], [cpl_cfitsio_version="no"])
                AS_IF([test x"$cpl_cfitsio_version" = xno],
                      [cpl_cfitsio_version_found="unknown"],
                      [cpl_cfitsio_version_found="$cpl_cfitsio_major.$cpl_cfitsio_minor"])

                CFLAGS="$cpl_cfitsio_cflags_save"
                LDFLAGS="$cpl_cfitsio_ldflags_save"
                LIBS="$cpl_cfitsio_libs_save"

                if test x"$cpl_cfitsio_version" = xno; then
    
                    CFITSIO_INCLUDES=""
                    CFITSIO_CFLAGS=""
                    CFITSIO_LDFLAGS=""
                    LIBCFITSIO=""
    
                    AC_MSG_ERROR([cannot determine cfitsio version!])
    
                else
                    AS_VERSION_COMPARE([$cpl_cfitsio_check_version],
                                       [$cpl_cfitsio_version_found],
                                       [],
                                       [],
                                       [cpl_cfitsio_version="no"])
                                       
                    if test x"$cpl_cfitsio_version" = xno; then
                        AC_MSG_ERROR([installed cfitsio ($cpl_cfitsio_version_found) is too old])
                    else
                        AC_DEFINE_UNQUOTED([CPL_CFITSIO_VERSION],
                                           [$cpl_cfitsio_version_found],
                                           [used version of CFITSIO])
                        AC_MSG_RESULT([$cpl_cfitsio_version_found])

                    fi
                fi
                
            fi
            
            # Check whether cfitsio has large file support
    
            AC_LANG_PUSH(C)
    
            cpl_cfitsio_cflags_save="$CFLAGS"
            cpl_cfitsio_ldflags_save="$LDFLAGS"
            cpl_cfitsio_libs_save="$LIBS"
    
            CFLAGS="$CFITSIO_INCLUDES"
            LDFLAGS="$CFITSIO_LDFLAGS"
            LIBS="$LIBCFITSIO"
    
            AC_MSG_CHECKING([whether cfitsio provides fits_hdu_getoff()])
    
            AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
                              [[
                              #include <fitsio.h>
                              ]],
                              [
                              fitsfile f;
                              int sts;
                              fits_get_hduoff(&f, NULL, NULL, NULL, &sts);
                              ])],
                              [cpl_cfitsio_have_fits_get_hduoff="yes"],
                              [cpl_cfitsio_have_fits_get_hduoff="no"])
    
            AC_MSG_RESULT([$cpl_cfitsio_have_fits_get_hduoff])
    
            AC_MSG_CHECKING([whether cfitsio provides fits_get_hduaddrll()])
    
            AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
                              [[
                              #include <fitsio.h>
                              ]],
                              [
                              fitsfile f;
                              int sts;
                              fits_get_hduaddrll(&f, NULL, NULL, NULL, &sts);
                              ])],
                              [cpl_cfitsio_have_fits_get_hduaddrll="yes"],
                              [cpl_cfitsio_have_fits_get_hduaddrll="no"])
    
            AC_MSG_RESULT([$cpl_cfitsio_have_fits_get_hduaddrll])
    
            AC_LANG_POP(C)
    
            CFLAGS="$cpl_cfitsio_cflags_save"
            LDFLAGS="$cpl_cfitsio_ldflags_save"
            LIBS="$cpl_cfitsio_libs_save"
            
        fi

        # Check whether cfitsio is thread-safe

        AC_MSG_CHECKING([whether cfitsio was compiled with thread support])

        AC_LANG_PUSH(C)

        cpl_cfitsio_cflags_save="$CFLAGS"
        cpl_cfitsio_ldflags_save="$LDFLAGS"
        cpl_cfitsio_libs_save="$LIBS"

        CFLAGS="$CFITSIO_INCLUDES"
        LDFLAGS="$CFITSIO_LDFLAGS"
        LIBS="$LIBCFITSIO"

        AC_LINK_IFELSE([AC_LANG_PROGRAM(
                      [[
                      #include <pthread.h>
                      ]],
                      [
                      extern pthread_mutex_t Fitsio_InitLock;
                      pthread_mutex_init(&Fitsio_InitLock, (void *)0);
                      ])],
                      [cpl_cfitsio_is_thread_safe=yes],
                      [cpl_cfitsio_is_thread_safe=no])

        AC_MSG_RESULT([$cpl_cfitsio_is_thread_safe])

        AC_LANG_POP(C)

        CFLAGS="$cpl_cfitsio_cflags_save"
        LDFLAGS="$cpl_cfitsio_ldflags_save"
        LIBS="$cpl_cfitsio_libs_save"


        # Set compiler flags and libraries

        if test x"$cpl_cfitsio_have_fits_get_hduoff" = xyes || \
          test x"$cpl_cfitsio_have_fits_get_hduaddrll" = xyes; then

            if test x"$cpl_cfitsio_have_fits_get_hduoff"; then
                AC_DEFINE([HAVE_FITS_GET_HDUOFF], [1],
                          [Define if you have the `fits_get_hduoff' function])
            else
                AC_DEFINE([HAVE_FITS_GET_HDUADDRLL],  [1],
                          [Define if you have the `fits_get_hduaddrll' function])
            fi

        fi

    else

        AC_MSG_WARN([cfitsio checks have been disabled! This package may not build!])

        CFITSIO_INCLUDES=""
        CFITSIO_CFLAGS=""
        CFITSIO_LDFLAGS=""
        LIBCFITSIO=""

        cpl_cfitsio_is_thread_safe="undefined"
    fi

    AC_CACHE_VAL(cpl_cv_cfitsio_is_thread_safe,
                 cpl_cv_cfitsio_is_thread_safe=$cpl_cfitsio_is_thread_safe)

    AC_SUBST(CFITSIO_INCLUDES)
    AC_SUBST(CFITSIO_CFLAGS)
    AC_SUBST(CFITSIO_LDFLAGS)
    AC_SUBST(LIBCFITSIO)
    
])


# CPL_CHECK_CEXT(incdirs=[], libdirs=[])
#---------------------------------------
# Checks for the C extension library and header files.
AC_DEFUN([CPL_CHECK_CEXT],
[

    cpl_cext_check_header="cxtypes.h"
    cpl_cext_check_libraries="-lcext -lpthread"

    cpl_cext_pkgconfig="libcext"
    
    
    # Initialize directory search paths with the arguments provided

    if test -n "$1"; then
        cpl_cext_incdirs="$1"
    fi

    if test -n "$2"; then
        cpl_cext_libdirs="$2"
    fi


    AC_REQUIRE([ESO_PROG_PKGCONFIG])

    AC_ARG_WITH(cext,
                AS_HELP_STRING([--with-cext],
                               [location where libcext is installed]),
                [
                    cpl_with_cext=$withval
                ])

    AC_ARG_WITH(cext-includes,
                AS_HELP_STRING([--with-cext-includes],
                               [location of the libcext header files]),
                cpl_with_cext_includes=$withval)

    AC_ARG_WITH(cext-libs,
                AS_HELP_STRING([--with-cext-libs],
                               [location of the libcext library]),
                cpl_with_cext_libs=$withval)

    AC_ARG_ENABLE(cext-test,
                  AS_HELP_STRING([--disable-cext-test],
                                 [disables checks for the libcext library and headers]),
                  cpl_enable_cext_test=$enableval,
                  cpl_enable_cext_test=yes)


    if test x"$cpl_enable_cext_test" = xyes; then

        AC_MSG_CHECKING([for libcext])

        # If include directories and libraries are given as arguments, use them
        # initially. Otherwise assume a standard system installation of the
        # package. This may then updated in the following.
        
        cpl_cext_libs="$cpl_cext_check_libraries"
        cpl_cext_cflags="-I/usr/include/cext -I/usr/include"
        cpl_cext_ldflags=""
    
        if test -n "${PKGCONFIG}"; then
    
            $PKGCONFIG --exists $cpl_cext_pkgconfig
    
            if test x$? = x0; then
                cpl_cext_cflags="`$PKGCONFIG --cflags $cpl_cext_pkgconfig`"
                cpl_cext_ldflags="`$PKGCONFIG --libs-only-L $cpl_cext_pkgconfig`"
                cpl_cext_libs="`$PKGCONFIG --libs-only-l $cpl_cext_pkgconfig`"
            fi
        
        fi

        # Directories given as arguments replace a standard system installation
        # setup if they are given.
                
        if test -n "$cpl_cext_incdirs"; then
            cpl_cext_cflags="-I$cpl_cext_incdirs"
        fi
        
        if test -n "$cpl_cext_libdirs"; then
            cpl_cext_ldflags="-L$cpl_cext_libdirs"
        fi

        if test -n "$CPLDIR"; then
            cpl_cext_cflags="-I$CPLDIR/include/cext -I$CPLDIR/include"
            cpl_cext_ldflags="-L$CPLDIR/lib64 -L$CPLDIR/lib"
        fi
        
        if test -n "$cpl_with_cext"; then    
            cpl_cext_cflags="-I$cpl_with_cext/include/cext -I$cpl_with_cext/include"
            cpl_cext_ldflags="-L$cpl_with_cext/lib64 -L$cpl_with_cext/lib"
        fi    
        
        if test -n "$cpl_with_cext_includes"; then
            cpl_cext_cflags="-I$cpl_with_cext_includes"
        fi
        
        if test -n "$cpl_with_cext_libs"; then
            cpl_cext_ldflags="-L$cpl_with_cext_libs"
        fi
        
        
        # Check whether the header files and the library are present and
        # whether they can be used.
        
        cpl_have_cext_libraries="unknown"
        cpl_have_cext_headers="unknown"
            
        AC_LANG_PUSH(C)
        
        cpl_cext_cflags_save="$CFLAGS"
        cpl_cext_ldflags_save="$LDFLAGS"
        cpl_cext_libs_save="$LIBS"
    
        CFLAGS="$cpl_cext_cflags"
        LDFLAGS="$cpl_cext_ldflags"
        LIBS="$cpl_cext_libs"
    
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
                          [[
                          #include <$cpl_cext_check_header>
                          ]],
                          [
                           cxchar c;
                          ])],
                          [
                           cpl_have_cext_headers="yes"
                          ],
                          [
                           cpl_have_cext_headers="no"
                          ])

        if test x"$cpl_have_cext_headers" = xyes; then
        
            AC_LINK_IFELSE([AC_LANG_PROGRAM(
                           [[
                           #include <cxutils.h>
                           ]],
                           [
                            cx_program_set_name("MyProgram");
                           ])],
                           [
                            cpl_have_cext_libraries="yes"
                           ],
                           [
                            cpl_have_cext_libraries="no"
                           ])
        
        fi
                        
        AC_LANG_POP(C)
       
        CFLAGS="$cpl_cext_cflags_save"
        LDFLAGS="$cpl_cext_ldflags_save"
        LIBS="$cpl_cext_libs_save"
        
        if test x"$cpl_have_cext_libraries" = xno || \
            test x"$cpl_have_cext_headers" = xno; then
            cpl_cext_notfound=""
    
            if test x"$cpl_have_cext_headers" = xno; then
                if test x"$cpl_have_cext_libraries" = xno; then
                    cpl_cext_notfound="(headers and libraries)"
                else
                    cpl_cext_notfound="(headers)"
                fi
            else
                cpl_cext_notfound="(libraries)"
            fi
    
            AC_MSG_RESULT([no])            
            AC_MSG_ERROR([libcext $cpl_cext_notfound was not found on your system.])
            
        else
            AC_MSG_RESULT([yes])            

            CX_INCLUDES="$cpl_cext_cflags"
            CX_CFLAGS="$cpl_cext_cflags"
            CX_LDFLAGS="$cpl_cext_ldflags"
            LIBCEXT="$cpl_cext_libs"
        fi
    
    else

        AC_MSG_WARN([libcext checks have been disabled! This package may not build!])

        CX_INCLUDES=""
        CX_CFLAGS=""
        CX_LDFLAGS=""
        LIBCEXT=""

    fi

    AC_SUBST(CX_INCLUDES)
    AC_SUBST(CX_CFLAGS)
    AC_SUBST(CX_LDFLAGS)
    AC_SUBST(LIBCEXT)

])


# CPL_CHECK_WCS(version)
#-----------------------
# Checks for the wcs library and header files.
AC_DEFUN([CPL_CHECK_WCS],
[

    cpl_wcs_check_version="$1"
    cpl_wcs_check_header="wcslib.h"
    cpl_wcs_check_libraries="-lwcs -lm"

    cpl_wcs_pkgconfig="wcslib"
    
    AC_REQUIRE([ESO_PROG_PKGCONFIG])

    AC_ARG_WITH(wcslib,
                AS_HELP_STRING([--with-wcslib],
                               [location where wcslib is installed]),
                [
                    cpl_with_wcs=$withval
                ])

    AC_ARG_WITH(wcslib-includes,
                AS_HELP_STRING([--with-wcslib-includes],
                               [location of the wcslib header files]),
                cpl_with_wcs_includes=$withval)

    AC_ARG_WITH(wcslib-libs,
                AS_HELP_STRING([--with-wcslib-libs],
                               [location of the wcslib library]),
                cpl_with_wcs_libs=$withval)


    AC_ARG_ENABLE(wcs,
                  AS_HELP_STRING([--disable-wcs],
                                 [disable WCS support.]),
                  [cpl_enable_wcs=$enable_wcs],
                  [cpl_enable_wcs="yes"])

    AC_CACHE_VAL([cpl_cv_enable_wcs],
                 [cpl_cv_enable_wcs=$cpl_enable_wcs])

    AC_ARG_VAR([WCSDIR], [Location where wcslib is installed])
    
    
    if test x"$cpl_enable_wcs" = xyes; then
    
        AC_MSG_CHECKING([for wcslib])
    
    
        # Initially assume a standard system installation of the package
        # This is updated in the following.
        
        cpl_wcs_libs="$cpl_wcs_check_libraries"
    
        cpl_wcs_cflags="-I/usr/include/wcslib"
        cpl_wcs_ldflags=""
    
        if test -n "${PKGCONFIG}"; then
    
            $PKGCONFIG --exists $cpl_wcs_pkgconfig
    
            if test x$? = x0; then
                cpl_wcs_cflags="`$PKGCONFIG --cflags $cpl_wcs_pkgconfig`"
                cpl_wcs_ldflags="`$PKGCONFIG --libs-only-L $cpl_wcs_pkgconfig`"
                cpl_wcs_libs="`$PKGCONFIG --libs-only-l $cpl_wcs_pkgconfig`"
            fi
        
        fi
        
        if test -n "$WCSDIR"; then
            cpl_wcs_cflags="-I$WCSDIR/include/wcslib -I$WCSDIR/include"
            cpl_wcs_ldflags="-L$WCSDIR/lib64 -L$WCSDIR/lib"
        fi
        
        if test -n "$cpl_with_wcs"; then
            cpl_wcs_cflags="-I$cpl_with_wcs/include/wcslib -I$cpl_with_wcs/include"
            cpl_wcs_ldflags="-L$cpl_with_wcs/lib64 -L$cpl_with_wcs/lib"
        fi    
        
        if test -n "$cpl_with_wcs_includes"; then
            cpl_wcs_cflags="-I$cpl_with_wcs_includes"
        fi
        
        if test -n "$cpl_with_wcs_libs"; then
            cpl_wcs_ldflags="-L$cpl_with_wcs_libs"
        fi
    
            
        # Check whether the header files and the library are present and
        # whether they can be used.
        
        cpl_have_wcs_libraries="unknown"
        cpl_have_wcs_headers="unknown"
            
        AC_LANG_PUSH(C)
        
        cpl_wcs_cflags_save="$CFLAGS"
        cpl_wcs_ldflags_save="$LDFLAGS"
        cpl_wcs_libs_save="$LIBS"
    
        CFLAGS="$cpl_wcs_cflags"
        LDFLAGS="$cpl_wcs_ldflags"
        LIBS="$cpl_wcs_libs"
    
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
                          [[
                          #include <stdio.h>
                          #include <$cpl_wcs_check_header>
                          ]],
                          [
                           struct wcsprm wcs;
                          ])],
                          [
                           cpl_have_wcs_headers="yes"
                          ],
                          [
                           cpl_have_wcs_headers="no"
                          ])
                          
        if test x"$cpl_have_wcs_headers" = xyes; then

            AC_LINK_IFELSE([AC_LANG_PROGRAM(
                           [[
                           #include <stdio.h>
                           #include <$cpl_wcs_check_header>
                           ]],
                           [
                            wcsini(0, 1, (void *)0);
                           ])],
                           [
                            cpl_have_wcs_libraries="yes"
                           ],
                           [
                            cpl_have_wcs_libraries="no"
                           ])

        fi                        

        AC_LANG_POP(C)
       
        CFLAGS="$cpl_wcs_cflags_save"
        LDFLAGS="$cpl_wcs_ldflags_save"
        LIBS="$cpl_wcs_libs_save"
        
        if test x"$cpl_have_wcs_libraries" = xno || \
            test x"$cpl_have_wcs_headers" = xno; then
            cpl_wcs_notfound=""
    
            if test x"$cpl_have_wcs_headers" = xno; then
                if test x"$cpl_have_wcs_libraries" = xno; then
                    cpl_wcs_notfound="(headers and libraries)"
                else
                    cpl_wcs_notfound="(headers)"
                fi
            else
                cpl_wcs_notfound="(libraries)"
            fi
    
            AC_MSG_RESULT([no])            
            AC_MSG_ERROR([wcslib $cpl_wcs_notfound was not found on your system.])
    
        else
    
            AC_MSG_RESULT([yes])
    
            # Setup the symbols
    
            WCS_INCLUDES="$cpl_wcs_cflags"
            WCS_CFLAGS="$cpl_wcs_cflags"
            WCS_LDFLAGS="$cpl_wcs_ldflags"
            LIBWCS="$cpl_wcs_libs"
            
            if test -n "$cpl_wcs_check_version"; then
            
                # Check library version
    
                AC_MSG_CHECKING([for a wcslib version >= $cpl_wcs_check_version])
    
                AC_LANG_PUSH(C)
    
                cpl_wcs_cflags_save="$CFLAGS"
                cpl_wcs_ldflags_save="$LDFLAGS"
                cpl_wcs_libs_save="$LIBS"
    
				# Do not add the library as a dependency here!
                CFLAGS="$WCS_CFLAGS"
                LDFLAGS="$WCS_LDFLAGS"
                LIBS=""
    
                AC_RUN_IFELSE([AC_LANG_PROGRAM(
                              [[
                              #include <stdio.h>
                              #include <wcslib.h>
        
                              #define stringify(v) stringify_arg(v)
                              #define stringify_arg(v) #v
                              ]],
                              [
                              char vmin[[]] = "$cpl_wcs_check_version";
                              char vlib[[]] = stringify(WCSLIB_VERSION);
        
                              int min_major = 0;
                              int min_minor = 0;
                              int min_micro = 0;
        
                              int lib_major = 0;
                              int lib_minor = 0;
                              int lib_micro = 0;
        
                              sscanf(vmin, "%d.%d.%d", &min_major, &min_minor, &min_micro);
                              sscanf(vlib, "%d.%d.%d", &lib_major, &lib_minor, &lib_micro);
        
                              FILE *f = fopen("conftest.out", "w");
                              fprintf(f, "%s\n", vlib);
                              fclose(f);
        
                              if (lib_major < min_major) {
                                  return 1;
                              }
                              else {
                                  if (lib_major == min_major) {
                                      if (lib_minor <  min_minor) {
                                          return 1;
                                      }
                                      else {
                                          if (lib_minor == min_minor) {
                                              if (lib_micro < min_micro) {
                                                  return 1;
                                              }
                                          }
                                      }
                                  }
                              }
        
                              return 0;
                              ])],
                              [
                               cpl_wcs_version="`cat conftest.out`";
                               rm -f conftest.out
                              ],
                              [
                               cpl_wcs_version="no";
                               cpl_wcs_version_found="unknown";
                               test -r conftest.out && cpl_wcs_version_found="`cat conftest.out`";
                               rm -f conftest.out
                              ])
        
                AC_MSG_RESULT([$cpl_wcs_version])
        
                AC_LANG_POP(C)
        
                CFLAGS="$cpl_wcs_cflags_save"
                LDFLAGS="$cpl_wcs_ldflags_save"
                LIBS="$cpl_wcs_libs_save"
    
    
                if test x"$cpl_wcs_version" = xno; then
    
                    AC_MSG_ERROR([Installed wcslib ($cpl_wcs_version_found) is too old])
    
                    WCS_INCLUDES=""
                    WCS_CFLAGS=""
                    WCS_LDFLAGS=""
                    LIBWCS=""
    
                fi
                
            fi
            
            AC_DEFINE_UNQUOTED([CPL_WCS_INSTALLED], 1, [Defined if WCS is available])
            
        fi
    
    else

        AC_MSG_WARN([WCS support is disabled! WCS support will not be build!])

        WCS_INCLUDES=""
        WCS_CFLAGS=""
        WCS_LDFLAGS=""
        LIBWCS=""

    fi
            
    AC_SUBST(WCS_INCLUDES)
    AC_SUBST(WCS_CFLAGS)
    AC_SUBST(WCS_LDFLAGS)
    AC_SUBST(LIBWCS)

])


# CPL_CHECK_FFTW(version)
#------------------------
# Checks for the FFTW library and header files.
AC_DEFUN([CPL_CHECK_FFTW],
[

    cpl_fftw_check_version="$1"
    cpl_fftw_check_header="fftw3.h"
    cpl_fftw_check_libraries="-lfftw3 -lm"
    cpl_fftwf_check_libraries="-lfftw3f -lm"

    cpl_fftw_pkgconfig="fftw3"
    cpl_fftwf_pkgconfig="fftw3f"


    AC_REQUIRE([ESO_PROG_PKGCONFIG])

    AC_ARG_WITH(fftw,
            AS_HELP_STRING([--with-fftw],
                           [location where fftw is installed]),
            [
                cpl_with_fftw=$withval
            ])

    AC_ARG_WITH(fftw-includes,
                AS_HELP_STRING([--with-fftw-includes],
                               [location of the fftw header files]),
                cpl_with_fftw_includes=$withval)

    AC_ARG_WITH(fftw-libs,
                AS_HELP_STRING([--with-fftw-libs],
                               [location of the fftw libraries]),
                cpl_with_fftw_libs=$withval)

    AC_ARG_ENABLE(fft,
                  AS_HELP_STRING([--disable-fft],
                                 [disable FFt support.]),
                  [cpl_enable_fft=$enable_fft],
                  [cpl_enable_fft="yes"])

    AC_CACHE_VAL([cpl_cv_enable_fft],
                 [cpl_cv_enable_fft=$cpl_enable_fft])

    AC_ARG_VAR([FFTWDIR], [Location where fftw is installed])
    
    
    if test x"$cpl_enable_fft" = xyes; then
    
        AC_MSG_CHECKING([for fftw (double precision)])
    
    
        # Initially assume a standard system installation of the package
        # This is updated in the following.
        
        cpl_fftw_libs="$cpl_fftw_check_libraries"
    
        cpl_fftw_cflags="-I/usr/include"
        cpl_fftw_ldflags=""
    
        if test -n "${PKGCONFIG}"; then
    
            $PKGCONFIG --exists $cpl_fftw_pkgconfig
    
            if test x$? = x0; then
                cpl_fftw_cflags="`$PKGCONFIG --cflags $cpl_fftw_pkgconfig`"
                cpl_fftw_ldflags="`$PKGCONFIG --libs-only-L $cpl_fftw_pkgconfig`"
                cpl_fftw_libs="`$PKGCONFIG --libs-only-l $cpl_fftw_pkgconfig`"
            fi
        
        fi
        
        if test -n "$FFTWDIR"; then
            cpl_fftw_cflags="-I$FFTWDIR/include"
            cpl_fftw_ldflags="-L$FFTWDIR/lib64 -L$FFTWDIR/lib"
        fi
        
        if test -n "$cpl_with_fftw"; then
            cpl_fftw_cflags="-I$cpl_with_fftw/include"
            cpl_fftw_ldflags="-L$cpl_with_fftw/lib64 -L$cpl_with_fftw/lib"
        fi    
        
        if test -n "$cpl_with_fftw_includes"; then
            cpl_fftw_cflags="-I$cpl_with_fftw_includes"
        fi
        
        if test -n "$cpl_with_fftw_libs"; then
            cpl_fftw_ldflags="-L$cpl_with_fftw_libs"
        fi
    
            
        # Check whether the header files and the library are present and
        # whether they can be used.
        
        cpl_have_fftw_libraries="unknown"
        cpl_have_fftw_headers="unknown"
            
        AC_LANG_PUSH(C)
        
        cpl_fftw_cflags_save="$CFLAGS"
        cpl_fftw_ldflags_save="$LDFLAGS"
        cpl_fftw_libs_save="$LIBS"
    
        CFLAGS="$cpl_fftw_cflags"
        LDFLAGS="$cpl_fftw_ldflags"
        LIBS="$cpl_fftw_libs"
    
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
                          [[
                          #include <$cpl_fftw_check_header>
                          ]],
                          [
                           fftw_plan plan;
                          ])],
                          [
                           cpl_have_fftw_headers="yes"
                          ],
                          [
                           cpl_have_fftw_headers="no"
                          ])
    
        if test x"$cpl_have_fftw_headers" = xyes; then
        
            AC_LINK_IFELSE([AC_LANG_PROGRAM(
                           [[
                           #include <$cpl_fftw_check_header>
                           ]],
                           [
                            fftw_plan_dft_1d(0, (void *)0, (void *)0, 0, 0);
                           ])],
                           [
                            cpl_have_fftw_libraries="yes"
                           ],
                           [
                            cpl_have_fftw_libraries="no"
                           ])
                           
        fi
                        
        AC_LANG_POP(C)
       
        CFLAGS="$cpl_fftw_cflags_save"
        LDFLAGS="$cpl_fftw_ldflags_save"
        LIBS="$cpl_fftw_libs_save"
        
        if test x"$cpl_have_fftw_libraries" = xno || \
            test x"$cpl_have_fftw_headers" = xno; then
            cpl_fftw_notfound=""
    
            if test x"$cpl_have_fftw_headers" = xno; then
                if test x"$cpl_have_fftw_libraries" = xno; then
                    cpl_fftw_notfound="(headers and libraries)"
                else
                    cpl_fftw_notfound="(headers)"
                fi
            else
                cpl_fftw_notfound="(libraries)"
            fi
    
            AC_MSG_RESULT([no])            
            AC_MSG_ERROR([fftw $cpl_fftw_notfound was not found on your system.])
    
        else
    
            AC_MSG_RESULT([yes])
    
            # Setup the symbols
    
            FFTW_INCLUDES="$cpl_fftw_cflags"
            FFTW_CFLAGS="$cpl_fftw_cflags"
            FFTW_LDFLAGS="$cpl_fftw_ldflags"
            LIBFFTW="$cpl_fftw_libs"
            
            if test -n "$cpl_fftw_check_version"; then
            
                # Check library version
    
                AC_MSG_CHECKING([for a fftw version >= $cpl_fftw_check_version])
    
                AC_LANG_PUSH(C)
    
                cpl_fftw_cflags_save="$CFLAGS"
                cpl_fftw_ldflags_save="$LDFLAGS"
                cpl_fftw_libs_save="$LIBS"
    
                CFLAGS="$FFTW_CFLAGS"
                LDFLAGS="$FFTW_LDFLAGS"
                LIBS="$LIBFFTW"
    
                AC_RUN_IFELSE([AC_LANG_PROGRAM(
                              [[
                              #include <stdio.h>
                              #include <stdlib.h>
                              #include <string.h>
                              #include <fftw3.h>
                              ]],
                              [
                              char vmin[[]] = "$cpl_fftw_check_version";
                              char *vstr    = malloc(strlen(fftw_version) + 1);
                              char *vlib    = vstr;
                              char *suffix  = NULL;
        
                              int min_major = 0;
                              int min_minor = 0;
                              int min_micro = 0;
        
                              int lib_major = 0;
                              int lib_minor = 0;
                              int lib_micro = 0;
                              strcpy(vstr, fftw_version);
        
                              vlib = strchr(vstr, '-') + 1;
                              suffix = strrchr(vlib, '-');
        
                              if (suffix) {
                                  *suffix = '\0';
                              }
        
                              sscanf(vmin, "%d.%d.%d", &min_major, &min_minor, &min_micro);
                              sscanf(vlib, "%d.%d.%d", &lib_major, &lib_minor, &lib_micro);
        
                              FILE* f = fopen("conftest.out", "w");
                              fprintf(f, "%s\n", vlib);
                              fclose(f);
        
                              free(vstr);
        
                              if (lib_major < min_major) {
                                  return 1;
                              }
                              else {
                                  if (lib_major == min_major) {
                                      if (lib_minor <  min_minor) {
                                          return 1;
                                      }
                                      else {
                                          if (lib_minor == min_minor) {
                                              if (lib_micro < min_micro) {
                                                  return 1;
                                              }
                                          }
                                      }
                                  }
                              }
        
                              return 0;
                              ])],
                              [
                               cpl_fftw_version="`cat conftest.out`";
                               rm -f conftest.out
                              ],
                              [
                               cpl_fftw_version="no";
                               cpl_fftw_version_found="unknown";
                               test -r conftest.out && cpl_fftw_version_found="`cat conftest.out`";
                               rm -f conftest.out
                              ])
        
                AC_MSG_RESULT([$cpl_fftw_version])
        
                AC_LANG_POP(C)
        
                CFLAGS="$cpl_fftw_cflags_save"
                LDFLAGS="$cpl_fftw_ldflags_save"
                LIBS="$cpl_fftw_libs_save"
    
    
                if test x"$cpl_fftw_version" = xno; then
    
                    FFTW_INCLUDES=""
                    FFTW_CFLAGS=""
                    FFTW_LDFLAGS=""
                    LIBFFTW=""
    
                    AC_MSG_ERROR([Installed double precision fftw ($cpl_fftw_version_found) is too old])
    
                fi
                
                AC_DEFINE_UNQUOTED([CPL_FFTW_VERSION], ["$cpl_fftw_version"],
                                   [Defined to the available FFTW version (double precision)])

            fi
            
            AC_DEFINE_UNQUOTED([CPL_FFTW_INSTALLED], 1,
                               [Defined if FFTW (double precision) is available])
            
        fi
    
        AC_MSG_CHECKING([for fftw (single precision)])
    
    
        # Initially assume a standard system installation of the package
        # This is updated in the following.
        
        cpl_fftw_libs="$cpl_fftwf_check_libraries"
    
        cpl_fftw_cflags="-I/usr/include"
        cpl_fftw_ldflags=""
    
        if test -n "${PKGCONFIG}"; then
    
            $PKGCONFIG --exists $cpl_fftwf_pkgconfig
    
            if test x$? = x0; then
                cpl_fftw_cflags="`$PKGCONFIG --cflags $cpl_fftwf_pkgconfig`"
                cpl_fftw_ldflags="`$PKGCONFIG --libs-only-L $cpl_fftwf_pkgconfig`"
                cpl_fftw_libs="`$PKGCONFIG --libs-only-l $cpl_fftwf_pkgconfig`"
            fi
        
        fi
        
        if test -n "$FFTWDIR"; then
            cpl_fftw_cflags="-I$FFTWDIR/include"
            cpl_fftw_ldflags="-L$FFTWDIR/lib64 -L$FFTWDIR/lib"
        fi
        
        if test -n "$cpl_with_fftw"; then
            cpl_fftw_cflags="-I$cpl_with_fftw/include"
            cpl_fftw_ldflags="-L$cpl_with_fftw/lib64 -L$cpl_with_fftw/lib"
        fi    
        
        if test -n "$cpl_with_fftw_includes"; then
            cpl_fftw_cflags="-I$cpl_with_fftw_includes"
        fi
        
        if test -n "$cpl_with_fftw_libs"; then
            cpl_fftw_ldflags="-L$cpl_with_fftw_libs"
        fi
    
            
        # Check whether the header files and the library are present and
        # whether they can be used.
        
        cpl_have_fftw_libraries="unknown"
        cpl_have_fftw_headers="unknown"
            
        AC_LANG_PUSH(C)
        
        cpl_fftw_cflags_save="$CFLAGS"
        cpl_fftw_ldflags_save="$LDFLAGS"
        cpl_fftw_libs_save="$LIBS"
    
        CFLAGS="$cpl_fftw_cflags"
        LDFLAGS="$cpl_fftw_ldflags"
        LIBS="$cpl_fftw_libs"
    
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
                          [[
                          #include <$cpl_fftw_check_header>
                          ]],
                          [
                           fftwf_plan plan;
                          ])],
                          [
                           cpl_have_fftw_headers="yes"
                          ],
                          [
                           cpl_have_fftw_headers="no"
                          ])
    
        if test x"$cpl_have_fftw_headers" = xyes; then
        
            AC_LINK_IFELSE([AC_LANG_PROGRAM(
                           [[
                           #include <$cpl_fftw_check_header>
                           ]],
                           [
                            fftwf_plan_dft_1d(0, (void *)0, (void *)0, 0, 0);
                           ])],
                           [
                            cpl_have_fftw_libraries="yes"
                           ],
                           [
                            cpl_have_fftw_libraries="no"
                           ])
 
        fi                       
        
        AC_LANG_POP(C)
       
        CFLAGS="$cpl_fftw_cflags_save"
        LDFLAGS="$cpl_fftw_ldflags_save"
        LIBS="$cpl_fftw_libs_save"
        
        if test x"$cpl_have_fftw_libraries" = xno || \
            test x"$cpl_have_fftw_headers" = xno; then
            cpl_fftw_notfound=""
    
            if test x"$cpl_have_fftw_headers" = xno; then
                if test x"$cpl_have_fftw_libraries" = xno; then
                    cpl_fftw_notfound="(headers and libraries)"
                else
                    cpl_fftw_notfound="(headers)"
                fi
            else
                cpl_fftw_notfound="(libraries)"
            fi
    
            AC_MSG_RESULT([no])            
            AC_MSG_ERROR([fftw $cpl_fftw_notfound was not found on your system.])
    
        else
    
            AC_MSG_RESULT([yes])
    
            # Setup the symbols
    
            FFTWF_INCLUDES="$cpl_fftw_cflags"
            FFTWF_CFLAGS="$cpl_fftw_cflags"
            FFTWF_LDFLAGS="$cpl_fftw_ldflags"
            LIBFFTWF="$cpl_fftw_libs"
            
            if test -n "$cpl_fftw_check_version"; then
            
                # Check library version
    
                AC_MSG_CHECKING([for a fftw version >= $cpl_fftw_check_version])
    
                AC_LANG_PUSH(C)
    
                cpl_fftw_cflags_save="$CFLAGS"
                cpl_fftw_ldflags_save="$LDFLAGS"
                cpl_fftw_libs_save="$LIBS"
    
                CFLAGS="$FFTWF_CFLAGS"
                LDFLAGS="$FFTWF_LDFLAGS"
                LIBS="$LIBFFTWF"
    
                AC_RUN_IFELSE([AC_LANG_PROGRAM(
                              [[
                              #include <stdio.h>
                              #include <stdlib.h>
                              #include <string.h>
                              #include <fftw3.h>
                              ]],
                              [
                              char vmin[[]] = "$cpl_fftw_check_version";
                              char *vstr    = malloc(strlen(fftwf_version) + 1);
                              char *vlib    = vstr;
                              char *suffix  = NULL;
        
                              int min_major = 0;
                              int min_minor = 0;
                              int min_micro = 0;
        
                              int lib_major = 0;
                              int lib_minor = 0;
                              int lib_micro = 0;
                              strcpy(vstr, fftwf_version);
        
                              vlib = strchr(vstr, '-') + 1;
                              suffix = strrchr(vlib, '-');
        
                              if (suffix) {
                                  *suffix = '\0';
                              }
        
                              sscanf(vmin, "%d.%d.%d", &min_major, &min_minor, &min_micro);
                              sscanf(vlib, "%d.%d.%d", &lib_major, &lib_minor, &lib_micro);
        
                              FILE* f = fopen("conftest.out", "w");
                              fprintf(f, "%s\n", vlib);
                              fclose(f);
        
                              free(vstr);
        
                              if (lib_major < min_major) {
                                  return 1;
                              }
                              else {
                                  if (lib_major == min_major) {
                                      if (lib_minor <  min_minor) {
                                          return 1;
                                      }
                                      else {
                                          if (lib_minor == min_minor) {
                                              if (lib_micro < min_micro) {
                                                  return 1;
                                              }
                                          }
                                      }
                                  }
                              }
        
                              return 0;
                              ])],
                              [
                               cpl_fftwf_version="`cat conftest.out`";
                               rm -f conftest.out
                              ],
                              [
                               cpl_fftwf_version="no";
                               cpl_fftwf_version_found="unknown";
                               test -r conftest.out && cpl_fftwf_version_found="`cat conftest.out`";
                               rm -f conftest.out
                              ])
        
                AC_MSG_RESULT([$cpl_fftwf_version])
        
                AC_LANG_POP(C)
        
                CFLAGS="$cpl_fftw_cflags_save"
                LDFLAGS="$cpl_fftw_ldflags_save"
                LIBS="$cpl_fftw_libs_save"
    
    
                if test x"$cpl_fftwf_version" = xno; then
    
                    FFTWF_INCLUDES=""
                    FFTWF_CFLAGS=""
                    FFTWF_LDFLAGS=""
                    LIBFFTWF=""
    
                    AC_MSG_ERROR([Installed single precision fftw ($cpl_fftwf_version_found) is too old])
    
                fi
                
                AC_DEFINE_UNQUOTED([CPL_FFTWF_VERSION], ["$cpl_fftwf_version"],
                                   [Defined to the available FFTW version (single precision)])
        
            fi
            
            AC_DEFINE_UNQUOTED([CPL_FFTWF_INSTALLED], 1,
                               [Defined if FFTW (single precision) is available])
            
        fi

    else

        AC_MSG_WARN([FFT support is disabled! FFT support will not be build!])

        FFTW_INCLUDES=""
        FFTW_CFLAGS=""
        FFTW_LDFLAGS=""
        LIBFFTW=""
        
        FFTWF_INCLUDES=""
        FFTWF_LDFLAGS=""
        LIBFFTWF=""
        
    fi
            
    AC_SUBST(FFTW_INCLUDES)
    AC_SUBST(FFTW_CFLAGS)
    AC_SUBST(FFTW_LDFLAGS)
    AC_SUBST(LIBFFTW)

    AC_SUBST(FFTWF_INCLUDES)
    AC_SUBST(FFTWF_CFLAGS)
    AC_SUBST(FFTWF_LDFLAGS)
    AC_SUBST(LIBFFTWF)

])


#
# CPL_CREATE_SYMBOLS(build=[])
#-----------------------------
# Sets the Makefile symbols for the CPL libraries. If an argument is
# provided the symbols are setup for building CPL, if no argument is
# given (default) the symbols are set for using the libraries
# for external package development.
AC_DEFUN([CPL_CREATE_SYMBOLS],
[

    if test -z "$1"; then
        LIBCPLCORE='-lcplcore'
        LIBCPLDRS='-lcpldrs'
        LIBCPLUI='-lcplui'
        LIBCPLDFS='-lcpldfs'
    else
        LIBCPLCORE='$(top_builddir)/cplcore/libcplcore.la'
        LIBCPLDRS='$(top_builddir)/cpldrs/libcpldrs.la'
        LIBCPLUI='$(top_builddir)/cplui/libcplui.la'
        LIBCPLDFS='$(top_builddir)/cpldfs/libcpldfs.la'
    fi

   AC_SUBST(LIBCPLCORE)
   AC_SUBST(LIBCPLDRS)
   AC_SUBST(LIBCPLUI)
   AC_SUBST(LIBCPLDFS)

])


# CPL_CHECK_LIBS
#---------------
# Checks for the CPL libraries and header files.
AC_DEFUN([CPL_CHECK_LIBS],
[

    cpl_check_version="$1"
    cpl_check_header="cpl.h"
    cpl_check_libraries="-lcplcore"
    
    cpl_pkgconfig="cpl"


    AC_ARG_WITH(cpl,
                AS_HELP_STRING([--with-cpl],
                               [location where CPL is installed]),
                [cpl_with_cpl=$withval],
                [cpl_with_cpl=""])

    AC_ARG_WITH(cpl-includes,
                AS_HELP_STRING([--with-cpl-includes],
                               [location of the CPL header files]),
                [cpl_with_cpl_includes=$withval],
                [cpl_with_cpl_includes=""]
                )

    AC_ARG_WITH(cpl-libs,
                AS_HELP_STRING([--with-cpl-libs],
                               [location of the CPL library]),
                [cpl_with_cpl_libs=$withval],
                [cpl_with_cpl_libs=""])

    AC_ARG_ENABLE(cpl-test,
                  AS_HELP_STRING([--disable-cpl-test],
                                 [disables checks for the CPL library and headers]),
                  cpl_enable_cpl_test=$enableval,
                  cpl_enable_cpl_test=yes)


    AC_MSG_CHECKING([for CPL])

    if test x"$cpl_enable_cpl_test" = xyes; then
    
        # Initially assume a standard system installation of the
        # package. This may then be updated by the following checks.
        
        cpl_libs="$cpl_check_libraries"
        cpl_cflags="-I/usr/include/cpl -I/usr/include"
        cpl_ldflags=""
    
        if test -n "${PKGCONFIG}"; then
    
            $PKGCONFIG --exists $cpl_pkgconfig
    
            if test x$? = x0; then
                cpl_cflags="`$PKGCONFIG --cflags $cpl_pkgconfig`"
                cpl_ldflags="`$PKGCONFIG --libs-only-L $cpl_pkgconfig`"
                cpl_libs="`$PKGCONFIG --libs-only-l $cpl_pkgconfig`"
            fi
        
        fi

        # Directories given as arguments replace a standard system installation
        # setup if they are given.
                
        if test -n "$CPLDIR"; then
            cpl_cflags="-I$CPLDIR/include/cpl -I$CPLDIR/include"
            cpl_ldflags="-L$CPLDIR/lib64 -L$CPLDIR/lib"
        fi
        
        if test -n "$cpl_with_cpl"; then    
            cpl_cflags="-I$cpl_with_cpl/include/cpl -I$cpl_with_cpl/include"
            cpl_ldflags="-L$cpl_with_cpl/lib64 -L$cpl_with_cpl/lib"
        fi    
        
        if test -n "$cpl_with_cpl_includes"; then
            cpl_cflags="-I$cpl_with_cpl_includes"
        fi
        
        if test -n "$cpl_with_cpl_libs"; then
            cpl_ldflags="-L$cpl_with_cpl_libs"
        fi

        # Check whether the header files and the library are present and
        # whether they can be used.
        
        cpl_have_libraries="unknown"
        cpl_have_headers="unknown"
            
        AC_LANG_PUSH(C)
        
        cpl_cflags_save="$CFLAGS"
        cpl_ldflags_save="$LDFLAGS"
        cpl_libs_save="$LIBS"
    
        CFLAGS="$cpl_cflags"
        LDFLAGS="$cpl_ldflags"
        LIBS="$cpl_libs"
    
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
                          [[
                          #include <$cpl_check_header>
                          ]],
                          [
                           cpl_image *image;
                          ])],
                          [
                           cpl_have_headers="yes"
                          ],
                          [
                           cpl_have_headers="no"
                          ])

        if test x"$cpl_have_headers" = xyes; then
        
            AC_LINK_IFELSE([AC_LANG_PROGRAM(
                           [[
                           #include <$cpl_check_header>
                           ]],
                           [
                            cpl_init(0);
                           ])],
                           [
                            cpl_have_libraries="yes"
                           ],
                           [
                            cpl_have_libraries="no"
                           ])

        fi
                                
        AC_LANG_POP(C)
       
        CFLAGS="$cpl_cflags_save"
        LDFLAGS="$cpl_ldflags_save"
        LIBS="$cpl_libs_save"
        
        if test x"$cpl_have_libraries" = xno || \
            test x"$cpl_have_headers" = xno; then
            cpl_notfound=""
    
            if test x"$cpl_have_headers" = xno; then
                if test x"$cpl_have_libraries" = xno; then
                    cpl_notfound="(headers and libraries)"
                else
                    cpl_notfound="(headers)"
                fi
            else
                cpl_notfound="(libraries)"
            fi
    
            AC_MSG_RESULT([no])            
            AC_MSG_ERROR([CPL $cpl_notfound was not found on your system.])
            
        else
            AC_MSG_RESULT([yes])            

            CPL_INCLUDES="$cpl_cflags"
            CPL_CFLAGS="$cpl_cflags"
            CPL_LDFLAGS="$cpl_ldflags"
            CPL_CREATE_SYMBOLS

            if test -n "$cpl_check_version"; then
            
                # Check library version
    
                AC_MSG_CHECKING([for a cpl version >= $cpl_check_version])
    
                AC_LANG_PUSH(C)
    
                cpl_cflags_save="$CFLAGS"
                cpl_ldflags_save="$LDFLAGS"
                cpl_libs_save="$LIBS"
    
				# Do not add the library as a dependency here!
                CFLAGS="$CPL_CFLAGS"
                LDFLAGS="$CPL_LDFLAGS"
                LIBS=""
    
                AC_RUN_IFELSE([AC_LANG_PROGRAM(
                              [[
                              #include <stdio.h>
                              #include <cpl.h>
                              
                              #define stringify(v) stringify_arg(v)
                              #define stringify_arg(v) #v
                              ]],
                              [
                              char vmin[[]] = "$cpl_check_version";
                              char vlib[[]] = CPL_VERSION_STRING;
        
                              int min_major = 0;
                              int min_minor = 0;
                              int min_micro = 0;
        
                              int lib_major = 0;
                              int lib_minor = 0;
                              int lib_micro = 0;
        
                              sscanf(vmin, "%d.%d.%d", &min_major, &min_minor, &min_micro);
                              sscanf(vlib, "%d.%d.%d", &lib_major, &lib_minor, &lib_micro);
        
                              FILE *f = fopen("conftest.out", "w");
                              fprintf(f, "%s\n", vlib);
                              fclose(f);
        
                              if (lib_major < min_major) {
                                  return 1;
                              }
                              else {
                                  if (lib_major == min_major) {
                                      if (lib_minor <  min_minor) {
                                          return 1;
                                      }
                                      else {
                                          if (lib_minor == min_minor) {
                                              if (lib_micro < min_micro) {
                                                  return 1;
                                              }
                                          }
                                      }
                                  }
                              }
        
                              return 0;
                              ])],
                              [
                               cpl_version="`cat conftest.out`";
                               rm -f conftest.out
                              ],
                              [
                               cpl_version="no";
                               cpl_version_found="unknown";
                               test -r conftest.out && cpl_version_found="`cat conftest.out`";
                               rm -f conftest.out
                              ])
        
                AC_MSG_RESULT([$cpl_version])
        
                AC_LANG_POP(C)
        
                CFLAGS="$cpl_cflags_save"
                LDFLAGS="$cpl_ldflags_save"
                LIBS="$cpl_libs_save"
    
    
                if test x"$cpl_version" = xno; then
    
                    AC_MSG_ERROR([Installed CPL ($cpl_version_found) is too old])
    
                    CPL_INCLUDES=""
                    CPL_CFLAGS=""
                    CPL_LDFLAGS=""
                    
                    LIBCPLCORE=""
                    LIBCPLDRS=""
                    LIBCPLUI=""
                    LIBCPLDFS=""
    
                fi
                
            fi
            
        fi
        
    else

        AC_MSG_RESULT([disabled])
        AC_MSG_WARN([CPL checks have been disabled! This package may not build!])

        CPL_INCLUDES=""
        CPL_CFLAGS=""
        CPL_LDFLAGS=""
        
        LIBCPLCORE=""
        LIBCPLDRS=""
        LIBCPLUI=""
        LIBCPLDFS=""

    fi

    AC_SUBST(CPL_INCLUDES)
    AC_SUBST(CPL_CFLAGS)
    AC_SUBST(CPL_LDFLAGS)
    
    AC_SUBST(LIBCPLCORE)
    AC_SUBST(LIBCPLDRS)
    AC_SUBST(LIBCPLUI)
    AC_SUBST(LIBCPLDFS)
    
])
