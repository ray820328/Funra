# CPL_SET_PREFIX
#---------------
# Sets and the directory prefix for the package installation. If no
# directory prefix was given on the command line the default prefix
# is appended to the configure argument list and thus passed to
# the subdirs configure.
AC_DEFUN([CPL_SET_PREFIX],
[
  unset CDPATH
  # make $CPLDIR the default for the installation
  AC_PREFIX_DEFAULT(${CPLDIR:-/usr/local})

  if test "x$prefix" = "xNONE"; then
    prefix=$ac_default_prefix
    ac_configure_args="$ac_configure_args --prefix $prefix"
  fi
])


# CPL_CONFIG_VERSION(VERSION, [CURRENT], [REVISION], [AGE])
#----------------------------------------------------------
# Setup various version information, especially the libtool versioning
AC_DEFUN([CPL_CONFIG_VERSION],
[
    cpl_version_string="$1"

    cpl_version=`echo "$1" | sed -e 's/[[a-z,A-Z]].*$//'`

    cpl_major_version=`echo "$cpl_version" | \
    sed 's/\([[0-9]]*\).\(.*\)/\1/'`
    cpl_minor_version=`echo "$cpl_version" | \
    sed 's/\([[0-9]]*\).\([[0-9]]*\)\(.*\)/\2/'`
    cpl_micro_version=`echo "$cpl_version" | \
    sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`

    if test -z "$cpl_major_version"; then
        cpl_major_version=0
    fi

    if test -z "$cpl_minor_version"; then
        cpl_minor_version=0
    fi

    if test -z "$cpl_micro_version"; then
        cpl_micro_version=0
    fi

    CPL_VERSION="$cpl_version"
    CPL_VERSION_STRING="$cpl_version_string"
    CPL_MAJOR_VERSION=$cpl_major_version
    CPL_MINOR_VERSION=$cpl_minor_version
    CPL_MICRO_VERSION=$cpl_micro_version

    if test -z "$4"; then
        CPL_INTERFACE_AGE=0
    else
        CPL_INTERFACE_AGE="$4"
    fi

    CPL_BINARY_AGE=`expr 256 '*' $CPL_MINOR_VERSION + $CPL_MICRO_VERSION`
    CPL_BINARY_VERSION=`expr 65536 '*' $CPL_MAJOR_VERSION + $CPL_BINARY_AGE`

    AC_SUBST(CPL_VERSION)
    AC_SUBST(CPL_VERSION_STRING)
    AC_SUBST(CPL_MAJOR_VERSION)
    AC_SUBST(CPL_MINOR_VERSION)
    AC_SUBST(CPL_MICRO_VERSION)
    AC_SUBST(CPL_INTERFACE_AGE)
    AC_SUBST(CPL_BINARY_VERSION)
    AC_SUBST(CPL_BINARY_AGE)

    AC_DEFINE_UNQUOTED(CPL_MAJOR_VERSION, $CPL_MAJOR_VERSION,
                       [CPL major version number])
    AC_DEFINE_UNQUOTED(CPL_MINOR_VERSION, $CPL_MINOR_VERSION,
                       [CPL minor version number])
    AC_DEFINE_UNQUOTED(CPL_MICRO_VERSION, $CPL_MICRO_VERSION,
                       [CPL micro version number])
    AC_DEFINE_UNQUOTED(CPL_INTERFACE_AGE, $CPL_INTERFACE_AGE,
                       [CPL interface age])
    AC_DEFINE_UNQUOTED(CPL_BINARY_VERSION, $CPL_BINARY_VERSION,
                       [CPL binary version number])
    AC_DEFINE_UNQUOTED(CPL_BINARY_AGE, $CPL_BINARY_AGE,
                       [CPL binary age])

    ESO_SET_LIBRARY_VERSION([$2], [$3], [$4])

    AC_CONFIG_COMMANDS([cplcore/cpl_version.h.in],
                       [cfgfile="cplcore/cpl_version.h.in"
                        cat > $cfgfile << _CPLEOF

_CPLEOF
                        echo '#define CPL_VERSION_STRING "'$version_string'"' >> $cfgfile
                        echo "#define CPL_VERSION_CODE $version_code" >> $cfgfile
                        cat >> $cfgfile << _CPLEOF

#define CPL_VERSION(major, minor, micro) \\
(((major) * 65536) + ((minor) * 256) + (micro))

_CPLEOF
                       ], [version_code=$CPL_BINARY_VERSION version_string=$CPL_VERSION_STRING])

])


# CPL_BASE_PATHS
#---------------
AC_DEFUN([CPL_BASE_PATHS],
[

    AC_REQUIRE([ESO_CHECK_EXTRA_LIBS])

    AC_MSG_CHECKING([for CPL])

    if test x"${prefix}" = xNONE; then
        cpl_prefix="$ac_default_prefix"
    else
        cpl_prefix="$prefix"
    fi

    if test x"$exec_prefix" = xNONE; then
        cpl_exec_prefix="$cpl_prefix"
        AC_MSG_RESULT([will be installed in $cpl_prefix])
    else
        cpl_exec_prefix="$exec_prefix"
        AC_MSG_RESULT([will be installed in $cpl_prefix and $cpl_exec_prefix])
    fi

    cpl_libraries="${cpl_exec_prefix}/lib"
    cpl_includes=${cpl_prefix}/include

    AC_SUBST(cpl_includes)
    AC_SUBST(cpl_libraries)

    CPLCORE_INCLUDES='-I$(top_srcdir)/cplcore -I$(top_builddir)/cplcore'
    CPLDRS_INCLUDES="-I\$(top_srcdir)/cpldrs -I\$(top_builddir)/cpldrs"
    CPLUI_INCLUDES='-I$(top_srcdir)/cplui -I$(top_builddir)/cplui'
    CPLDFS_INCLUDES='-I$(top_srcdir)/cpldfs -I$(top_builddir)/cpldfs'
    CPL_INCLUDES="$CPLDFS_INCLUDES $CPLUI_INCLUDES $CPLDRS_INCLUDES $CPLCORE_INCLUDES"

    CPL_LDFLAGS=""

    AC_SUBST(CPLCORE_INCLUDES)
    AC_SUBST(CPLDRS_INCLUDES)
    AC_SUBST(CPLUI_INCLUDES)
    AC_SUBST(CPLDFS_INCLUDES)
    AC_SUBST(CPL_LDFLAGS)

])


# CPL_SET_PATHS
#--------------
# Define auxiliary directories of the CPL tree.
AC_DEFUN([CPL_SET_PATHS],
[

    AC_REQUIRE([CPL_BASE_PATHS])

    if test -z "$apidocdir"; then
        apidocdir='${datadir}/doc/${PACKAGE}/html'
    fi
    AC_SUBST(apidocdir)
    
    if test -z "$configdir"; then
        configdir='${datadir}/${PACKAGE}/config'
    fi
    AC_SUBST(configdir)


    # Define a preprocesor symbol for the application search paths
    # Need to evaluate the expression 3 times to get to the full name

    config_dir="`eval echo $configdir`"
    config_dir="`eval echo $config_dir`"
    config_dir="`eval echo $config_dir`"

    AC_DEFINE_UNQUOTED(CPL_CONFIG_DIR, "$config_dir",
                       [Directory prefix for system configuration files])

])


# CPL_EXPORT_DIRS(dir=directory-list)
#------------------------------------
# Add extra directories to the command line arguments when configuring
# subdirectories.
AC_DEFUN([CPL_EXPORT_DIRS],
[

    for d in $1; do
        eval cpl_propagate_dir="\$$d"
        ac_configure_args="$ac_configure_args $d='$cpl_propagate_dir'"
    done

])


# CPL_FUNC_GETOPT
#----------------
# Checks for GNU getopt_long declaration and function.
AC_DEFUN([CPL_FUNC_GETOPT],
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


# CPL_CONFIG_FUNC()
#----------------------------------------------------------
# Setup creation of body of cpl_func.h
AC_DEFUN([CPL_CONFIG_FUNC],
[

    AC_LINK_IFELSE([AC_LANG_SOURCE(
                   [
                     int main(void)
                     {
                         return (int)__func__;
                     }
                   ])],
                   cpl_cv_func_has_func=__func__,
                   cpl_cv_func_has_func='"\"\""',
                   cpl_cv_func_has_func='"\"\""')

    AC_CONFIG_COMMANDS([cplcore/cpl_func.h.in],
                       [cfgfile="cplcore/cpl_func.h.in"
                        echo "#define cpl_func $cpl_func_value" > $cfgfile
                       ], [cpl_func_value=$cpl_cv_func_has_func])

])


# CPL_PATH_JAVA
#--------------
# Checks for an existing java installation
AC_DEFUN([CPL_PATH_JAVA],
[

    AC_REQUIRE([AC_CANONICAL_BUILD])

    cpl_java_check_java="java"
    cpl_java_check_javac="javac"
    cpl_java_check_header="jni.h"
    cpl_java_check_header_md="jni_md.h"
    cpl_java_path=""

    AC_ARG_WITH(java,
                AC_HELP_STRING([--with-java],
                               [location where java is installed]),
                [
                    cpl_with_java=$withval
                ])

    AC_ARG_WITH(java-includes,
                AC_HELP_STRING([--with-java-includes],
                               [location of the java header files]),
                [
                    cpl_with_java_includes=$withval
                ])

    AC_ARG_WITH(java-includes-md,
                AC_HELP_STRING([--with-java-includes-md],
                               [location of the machine dependent java header files]),
                [
                    cpl_with_java_includes_md=$withval
                ])


    AC_MSG_CHECKING([for Java Development Kit])

    AC_ARG_VAR([JAVA], [Java application launcher])
    AC_ARG_VAR([JAVAC], [Java compiler command])

    AC_CACHE_VAL([cpl_cv_path_java],
                 [
                     if test x"$cpl_with_java" != xno; then

                          if test -z "$cpl_with_java"; then
                              test -n "$JAVA_HOME" && cpl_java_dirs="$JAVA_HOME/bin $JAVA_HOME/Commands"
                          else
                              cpl_java_dirs="$cpl_with_java/bin $cpl_with_java/Commands"
                          fi

                          ESO_FIND_FILE($cpl_java_check_java, $cpl_java_dirs, cpl_java_path)

                      else
                        cpl_java_path="no"
                      fi

                      if test x"$cpl_java_path" = xno; then
                          cpl_cv_path_java="no"
                          JAVA=":"
                          JAVAC=":"
                      else
                          JAVA="$cpl_java_path/$cpl_java_check_java"

                          if test -x $cpl_java_path/$cpl_java_check_javac; then
                              JAVAC="$cpl_java_path/$cpl_java_check_javac"
                          else
                              JAVAC=":"
                          fi

                          cpl_cv_path_java="`echo $cpl_java_path | sed -e 's;/bin$;;'`"
                          if test $cpl_cv_path_java = $cpl_java_path ; then
                              cpl_cv_path_java="`echo $cpl_java_path | sed -e 's;/Commands$;;'`"
                          fi
                      fi
                 ])

    AC_CACHE_VAL([cpl_cv_env_java_os],
                 [
                    if test x"$cpl_cv_path_java" = xno; then
                        cpl_cv_env_java_os="unknown"
                    else
                        case $build in
                        *-*-linux*)
                            cpl_cv_env_java_os="linux"
                            ;;
                        *-*-freebsd*)
                            cpl_cv_env_java_os="bsd"
                            ;;
                        *-*-solaris*)
                            cpl_cv_env_java_os="solaris"
                            ;;
                        *-*-hpux*)
                            cpl_cv_env_java_os="hp-ux"
                            ;;
                        *-*-irix*)
                            cpl_cv_env_java_os="irix"
                            ;;
                        *-*-aix*)
                            cpl_cv_env_java_os="aix"
                            ;;
                        *-*-sequent*)
                            cpl_cv_env_java_os="ptx"
                            ;;
                        *-*-os390*)
                            cpl_cv_env_java_os="os390"
                            ;;
                        *-*-os400*)
                            cpl_cv_env_java_os="os400"
                            ;;
                        *-apple-darwin*|*-apple-rhapsody*)
                            cpl_cv_env_java_os="darwin"
                            ;;
                        *-*-cygwin*|*-*-mingw*)
                            cpl_cv_env_java_os="win32"
                            ;;
                        *)
                            if test -d $cpl_cv_path_java/include/genunix; then
                                cpl_cv_env_java_os="genunix"
                            else
                                cpl_cv_env_java_os="unknown"
                            fi
                            ;;
                        esac
                    fi
                 ])

    AC_CACHE_VAL([cpl_cv_header_java],
                 [
                    if test x"$cpl_cv_path_java" = xno; then
                        cpl_java_includes="no"
                    else
                        if test -z "$cpl_with_java_includes"; then
                            cpl_java_incdirs="$cpl_cv_path_java/include $cpl_cv_path_java/Headers"
                        else
                            cpl_java_incdirs="$cpl_with_java_includes"
                        fi

                        ESO_FIND_FILE($cpl_java_check_header, $cpl_java_incdirs, cpl_java_includes)
                    fi

                    if test x"$cpl_java_includes" = xno; then
                        cpl_cv_header_java="no"
                    else
                        cpl_cv_header_java="$cpl_java_includes"
                    fi
                 ])


    AC_CACHE_VAL([cpl_cv_header_java_md],
                 [
                    if test x"$cpl_cv_path_java" = xno || test x"$cpl_cv_header_java" = xno; then
                        cpl_cv_header_java_md="no"
                    else
                        cpl_java_includes_md="no"

                        if test -z "$cpl_with_java_includes_md"; then

                            cpl_java_incdirs="$cpl_cv_header_java"

                            if test x"$cpl_cv_env_java_os" != xunknown; then
                                cpl_java_incdirs="$cpl_cv_header_java/$cpl_cv_env_java_os $cpl_java_incdirs"
                            fi

                            ESO_FIND_FILE($cpl_java_check_header_md, $cpl_java_incdirs, cpl_java_includes_md)
                        else
                            cpl_java_incdirs="$cpl_with_java_includes_md"
                            ESO_FIND_FILE($cpl_java_check_header_md, $cpl_java_incdirs, cpl_java_includes_md)
                        fi

                        cpl_cv_header_java_md="$cpl_java_includes_md"
                    fi
                 ])


    if test x"$cpl_cv_path_java" = xno; then
        AC_MSG_RESULT([no])
    else
        if test x"$cpl_cv_header_java" = xno || test x"$cpl_cv_header_java_md" = xno; then
            if test x"$cpl_cv_header_java" = xno; then
                if test x"$cpl_cv_header_java_md" = xno; then
                    cpl_java_notfound="$cpl_java_check_header and $cpl_java_check_header_md"
                else
                    cpl_java_notfound="$cpl_java_check_header"
                fi
            else
                cpl_java_notfound="$cpl_java_check_header_md"
            fi
            AC_MSG_RESULT([$cpl_cv_path_java, headers $cpl_java_notfound not found])
        else
            AC_MSG_RESULT([$cpl_cv_path_java, headers $cpl_cv_header_java, $cpl_cv_header_java_md])
        fi
    fi

    # Setup the Makefile symbols
    if test x"$cpl_cv_path_java" != xno; then
        
    
        if test -f "$cpl_cv_path_java/Commands/java" ; then
            JAVA="$cpl_cv_path_java/Commands/java"
        else
            JAVA="$cpl_cv_path_java/bin/java"
        fi

        if test x"$cpl_cv_header_java" != xno && test x"$cpl_cv_header_java_md" != xno; then
            JAVA_INCLUDES="-I$cpl_cv_header_java -I$cpl_cv_header_java_md"
        else
            JAVA_INCLUDES=""
        fi
    else
        JAVA=":"
        JAVA_INCLUDES=""
    fi

    AC_SUBST([JAVA])
    AC_SUBST([JAVAC])
    AC_SUBST([JAVA_INCLUDES])

])


# CPL_ENABLE_GASGANO
#------------------
# Check whether the Gasgano interface library should/can be built.
AC_DEFUN([CPL_ENABLE_GASGANO],
[

    AC_REQUIRE([CPL_PATH_JAVA])

    AC_ARG_ENABLE(gasgano,
                  AC_HELP_STRING([--enable-gasgano],
                                 [build the gasgano support library [[default=yes]]]),
                  cpl_enable_gasgano=$enableval,
                  cpl_enable_gasgano=yes)

    AC_MSG_CHECKING([whether the Gasgano interface library can be built])

    if test x"$cpl_enable_gasgano" != xyes; then
        cpl_gasgano_support="no"
        AC_MSG_RESULT([no])
    else
        if test -n "$JAVA_INCLUDES"; then
            cpl_gasgano_support="yes"
            AC_MSG_RESULT([yes])
        else
            cpl_gasgano_support="no"
            AC_MSG_RESULT([no])
        fi

    fi

    AM_CONDITIONAL([GASGANO_SUPPORT], [test x"$cpl_gasgano_support" = xyes])

    GASGANO_SHREXT=""
    if test x"$cpl_gasgano_support" = xyes; then
        case $cpl_cv_env_java_os in
        darwin)
            GASGANO_SHREXT="-shrext .jnilib"
            ;;
        *)
            ;;
        esac
    fi

    AC_SUBST([GASGANO_SHREXT])

])


# CPL_ENABLE_THREADS(threads=yes)
#--------------------------------
AC_DEFUN([CPL_ENABLE_THREADS],
[

    AC_REQUIRE([ESO_CHECK_THREADS_POSIX])
    AC_REQUIRE([CPL_OPENMP])
    
    
    AC_ARG_ENABLE(threads,
                  AC_HELP_STRING([--enable-threads],
                                 [enables thread support [default=$1]]),
                  cpl_enable_threads=$enableval, cpl_enable_threads=$1)

    AH_TEMPLATE([CPL_THREADS_ENABLED],
                [Define if thread support is enabled.])

    AC_MSG_CHECKING([whether thread support is available])
    
    if test x"$cpl_enable_threads" = xyes; then
    
        if test x"$eso_cv_threads_posix" = xno && \
          test x"$ac_cv_prog_c_openmp" = xno; then
            AC_MSG_RESULT([no])
            AC_MSG_ERROR([Thread support was requested, but the system does not support it!])
        else
        
            CFLAGS="-D_REENTRANT $CFLAGS"
            AC_DEFINE_UNQUOTED([CPL_THREADS_ENABLED])

            AC_MSG_RESULT([yes])
            
            case $ac_cv_prog_c_openmp in
            "none needed" | unsupported)
                if test x"$eso_cv_threads_posix_lib" = xyes; then
                    echo $LIBS | grep -q -e "$LIBPTHREAD" || LIBS="$LIBPTHREAD $LIBS" 
                else
                   CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
                   LDFLAGS="$LDFLAGS $PTHREAD_CFLAGS"
                fi
                ;;
            *)
                # FIXME: Explicitly add the OpenMP runtime library, since libtool
                #        removes the compiler flag when linking!
                AC_SEARCH_LIBS([omp_get_num_threads], [gomp mtsk omp], [],
                               AC_MSG_ERROR([OpenMP runtime environment not found!]))

                CFLAGS="$CFLAGS $OPENMP_CFLAGS"
                LDFLAGS="$LDFLAGS $OPENMP_CFLAGS"                
                ;;
            esac

	    	cpl_threads_enabled=yes
            
        fi
      
    else
    	cpl_threads_enabled=no
        AC_MSG_RESULT([disabled])        
    fi
    
    AC_CACHE_VAL(cpl_cv_threads_enabled,
    		     cpl_cv_threads_enabled=$cpl_threads_enabled)
    
])


# CPL_CONFIG_CFITSIO(version)
#----------------------------
# Adds an extra check to CPL_CHECK_CFITSIO. It checks whether the libcfitsio
# found is thread-safe in case building CPL with thread support was enabled.
# Otherwise the package configuration is aborted.
#
# This macro has to be called after CPL_ENABLE_THREADS has been called!
AC_DEFUN([CPL_CONFIG_CFITSIO],
[
	CPL_CHECK_CFITSIO([$1])
	
	if test x"$cpl_cv_threads_enabled" = xyes && \
		test x"$cpl_cv_cfitsio_is_thread_safe" = xno; then
		AC_MSG_ERROR([Building with thread support requires a thread-safe libcfitsio! Please update your cfitsio installation])
	fi

	
])


# CPL_CONFIG_CEXT(srcdir=libcext)
#--------------------------------
# Checks whether libcext provided by the system should be used for building
# CPL, or the packaged version in the subdirectory 'srcdir'. If 'srcdir' is
# not given it defaults to 'libcext'
AC_DEFUN([CPL_CONFIG_CEXT],
[
    
    libcext="libcext"
    
    AC_ARG_WITH(system-cext,
                AC_HELP_STRING([--with-system-cext],
                               [Use the system cext installation for building [default=no]]),
                [
                    cpl_with_system_cext=$withval
                ])
                
    if test x"$cpl_with_system_cext" = xyes; then
        CPL_CHECK_CEXT
        libcext=""
    else
    
        if test -n "$1"; then
            libcext="$1"
        fi
        
        if test ! -d "$srcdir/$libcext"; then
            AC_MSG_ERROR([libcext source tree was not found in `$srcdir/$libcext'])
        else
            AC_CACHE_VAL(cpl_cv_with_system_cext,
                         cpl_cv_with_system_cext=$cpl_with_system_cext)


            # Setup the symbols

            CX_INCLUDES="-I\$(top_builddir)/libcext/cext"
            CX_INCLUDES="$CX_INCLUDES -I\$(top_srcdir)/$libcext/cext"
            CX_LDFLAGS=""
            LIBCEXT="\$(top_builddir)/libcext/cext/libcext.la"

            AC_SUBST([CX_INCLUDES])
            AC_SUBST([CX_LDFLAGS])
            AC_SUBST([LIBCEXT])
        fi
        
    fi

    AC_SUBST([libcext])
            
])


# CHECK MEMORY MODE
#------------------
AC_DEFUN([CPL_CHECK_MEMORYMODE],
[
    AC_MSG_CHECKING([for extended memory mode])

    AC_ARG_ENABLE(memory-mode,
                AC_HELP_STRING([--enable-memory-mode=M],
                               [where M=0 switches off the internal memory
                                handling (default), M=1 exits the program
                                whenever a memory allocation fails,
                                M=2 switches on the internal memory handling]),
                [
                    cpl_memory_flag=yes
                    # $enableval=yes when no argument is given
                    cpl_memory_mode=$enableval
                ])

    AC_ARG_ENABLE(max-ptrs,
                AC_HELP_STRING([--enable-max-ptrs=MAXPTRS],
                               [MAXPTRS Set MAXPTRS as the maximum number of
                pointers allowed when using memory-mode=2 (200003)]),
                [
                    cpl_max_ptrs_flag=yes
                    cpl_max_ptrs=$enableval
                ])

    # Pending: check cpl_max_ptrs is numeric, otherwise AC_MSG_ERROR 
    if test "x$cpl_max_ptrs_flag" = xyes ; then
        CPL_MAXPTRS_CFLAGS="-DCPL_XMEMORY_MAXPTRS=$cpl_max_ptrs"
    else
        CPL_MAXPTRS_CFLAGS="-DCPL_XMEMORY_MAXPTRS=200003"
    fi

    if test "x$cpl_memory_flag" = xyes ; then
        CPL_CFLAGS="-DCPL_XMEMORY_MODE=$cpl_memory_mode"
        case $cpl_memory_mode in
        yes)
          CPL_CFLAGS="-DCPL_XMEMORY_MODE=0 -DCPL_XMEMORY_MAXPTRS=1"
          break ;;
        0|1)
          CPL_CFLAGS="-DCPL_XMEMORY_MODE=$cpl_memory_mode -DCPL_XMEMORY_MAXPTRS=1"
          break ;;
        2)
          CPL_CFLAGS="-DCPL_XMEMORY_MODE=2 $CPL_MAXPTRS_CFLAGS"
          break ;;
        *)
          AC_MSG_ERROR([Option --enable-memory-mode=$cpl_memory_mode not valid. Please check!])
          break ;;
        esac

    else
        CPL_CFLAGS="-DCPL_XMEMORY_MODE=0 -DCPL_XMEMORY_MAXPTRS=1"
    fi

    AC_MSG_RESULT([CPL_CFLAGS=$CPL_CFLAGS])
    AC_SUBST(CPL_CFLAGS)
])


# CPL_CHECK_VA_ARGS
#------------------
# Checks for the support of variadic macros
AC_DEFUN([CPL_CHECK_VA_ARGS],
[

    AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
                      [[#define cpl_check_va_args(A, ...) printf(A, __VA_ARGS__)]],
                      [[]])],
                      AC_DEFINE(CPL_HAVE_VA_ARGS, 1,
                      Define to 1 if you have variadic macros.),
                      AC_MSG_WARN([Variadic macros are not available]))
])


# CPL_CHECK_COMPLEX
#------------------
# Checks for the support of complex primitive types
AC_DEFUN([CPL_CHECK_COMPLEX],
[

    AH_TEMPLATE([CPL_HAVE_COMPLEX_C99],
                [Define if C99 complex number number arithmetic is supported.])

    AC_MSG_CHECKING([whether complex number arithmetic is supported])

    AC_LANG_PUSH(C)
    
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
                      [[#include <complex.h>]],
                      [[
                          double _Complex a, b;
                          a = 2.0 + 2.0 * _Complex_I;
                          b = cabs(a);
                      ]])],
                      [cpl_have_complex_types_c99=yes],
                      [cpl_have_complex_types_c99=no])

    AC_LANG_POP(C)
    
    if test x"$cpl_have_complex_types_c99" = xno; then
        AC_MSG_RESULT([no])
        AC_MSG_ERROR([complex number arithmetic is not supported!])
    fi
        
    AC_MSG_RESULT([yes])
    AC_DEFINE([CPL_HAVE_COMPLEX_C99], [1])
])


# CPL_CHECK_CPU
#--------------
# Try to determine the Cache size - using /proc/cpuinfo
AC_DEFUN([CPL_CHECK_CPU],
[
    AC_MSG_CHECKING([/proc/cpuinfo for the CPU cache and cores])

    if test -r "/proc/cpuinfo"; then
        cpl_cpu_cache_string=`grep "cache size" /proc/cpuinfo | head -1 | perl -ne 's/^\D+//;s/\b\s*kB\b\D*/<<10/i || s/\b\s*MB\b\D*/<<20/i;/^\d+/ and print eval'`
        if test -n "$cpl_cpu_cache_string"; then
            AC_DEFINE_UNQUOTED(CPL_CPU_CACHE, $cpl_cpu_cache_string, [Defined to number of bytes in (L2) Cache])
            AC_MSG_RESULT([CPU cache $cpl_cpu_cache_string B])
        fi
        cpl_cpu_core_string=`grep "cpu cores" /proc/cpuinfo | head -1 | perl -ne 's/^\D+//g;/^\d+/ and print'`
        if test -n "$cpl_cpu_core_string"; then
            AC_DEFINE_UNQUOTED(CPL_CPU_CORES, $cpl_cpu_core_string, [Defined to number of cores (possibly sharing the (L2) Cache)])
            AC_MSG_RESULT([CPU cores $cpl_cpu_core_string])
        fi
    fi

])
