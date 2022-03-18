/*
 * This file is part of the ESO Common Pipeline Library
 * Copyright (C) 2001-2017 European Southern Observatory
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include <complex.h>

#include "cpl_test.h"

#include "cpl_init.h"
#include "cpl_errorstate.h"
#include "cpl_memory.h"
#include "cpl_msg.h"

#include "cpl_tools.h"
#include "cpl_stats_impl.h"
#include "cpl_io_fits.h"

#include "cpl_image_io_impl.h"
#include "cpl_image_basic_impl.h"

#include <errno.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
/* Needed for fabs() */
#include <math.h>

#undef CPL_HAVE_TIMES

#ifdef HAVE_SYS_TIMES_H
#include <sys/times.h>
#ifdef HAVE_TIME_H
#include <time.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#define CPL_HAVE_TIMES
#endif
#endif
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#endif

#undef CPL_HAVE_CLOCK_GETTIME
#undef CPL_HAVE_GETTIMEOFDAY

#ifdef HAVE_TIME_H
#ifdef HAVE_CLOCK_GETTIME
#include <time.h>
#define CPL_HAVE_CLOCK_GETTIME
#endif
#endif

#ifndef CPL_HAVE_CLOCK_GETTIME
#ifdef HAVE_SYS_TIME_H
#ifdef HAVE_GETTIMEOFDAY
#include <sys/time.h>
#define CPL_HAVE_GETTIMEOFDAY
#endif
#endif
#endif

#ifdef HAVE_SYS_TIME_H
/* Used for gettimeofday() */
#include <sys/time.h>
#endif

/* Needed for CFITSIO_VERSION */
#include <fitsio.h>

#if defined CPL_FFTW_INSTALLED || defined CPL_FFTWF_INSTALLED
/* Needed for fftw_version */
#include <fftw3.h>
#endif

#if defined CPL_WCS_INSTALLED && CPL_WCS_INSTALLED == 1
/* Used for WCSLIB_VERSION */
#include <wcslib.h>
#endif

#ifndef inline
#define inline /* inline */
#endif

/*----------------------------------------------------------------------------*/
/**
 * @defgroup cpl_test Unit testing functions
 *
 * This module provides various functions for unit testing.
 *
 * @par Synopsis:
 * @code
 *   #include "cpl_test.h"
 * @endcode
 */
/*----------------------------------------------------------------------------*/
/**@{*/

/*-----------------------------------------------------------------------------
                           Private variables
 -----------------------------------------------------------------------------*/

static cpl_errorstate cleanstate;
static cpl_size       cpl_test_count = 0;
static cpl_size       cpl_test_failures = 0;
static const char   * cpl_test_report = NULL;
static double         cpl_test_time_start;
static double         cpl_test_time_one;
static cpl_flops      cpl_test_flops_one = 0;
/* O: Uninitialized, 1: Initialized, 2: Deinitialized */
static int            cpl_test_state_ = 0;

/*-----------------------------------------------------------------------------
                        Private function prototypes
 -----------------------------------------------------------------------------*/

static void cpl_test_reset(cpl_errorstate);
static void cpl_errorstate_dump_debug(unsigned, unsigned, unsigned);
static void cpl_errorstate_dump_info(unsigned, unsigned, unsigned);
static const char * cpl_test_get_description(void) CPL_ATTR_CONST;
static void cpl_test_one(int, double, cpl_flops, cpl_errorstate, cpl_boolean,
                         const char *, cpl_boolean, const char *,
                         const char *, unsigned) CPL_ATTR_NONNULL;

static void cpl_test_dump_status(void);
static char * cpl_test_fits_file(const char *, const char *) CPL_ATTR_NONNULL;

/*-----------------------------------------------------------------------------
                            Function codes
 -----------------------------------------------------------------------------*/



/*----------------------------------------------------------------------------*/
/**
  @brief    Get the process CPU time, when available (from times())
  @return   The process CPU time in seconds.
  @note Will always return 0 if times() is unavailable

  Example of usage:
    @code

    int my_benchmark (void)
    {
        double cputime, tstop;
        const double tstart = cpl_test_get_cputime();

        myfunc();

        tstop = cpl_test_get_cputime();

        cputime = tstop - tstart;

        cpl_msg_info(cpl_func, "The call took %g seconds of CPU-time", cputime);

    }
   
    @endcode


 */
/*----------------------------------------------------------------------------*/
double cpl_test_get_cputime(void) {

#if defined HAVE_SYS_TIMES_H && defined HAVE_SYSCONF && defined _SC_CLK_TCK

    struct tms buf;

    (void)times(&buf);

    return (double)buf.tms_utime / (double)sysconf(_SC_CLK_TCK);

#else

    return 0.0;

#endif

}

/*----------------------------------------------------------------------------*/
/**
  @brief  Get the process wall-clock time, when available
  @return The process wall-clock time in seconds.
  @note   Will always return 0 if clock_gettime() and gettimeofday()
          are unavailable or failing
  @see  clock_gettime(), gettimeofday()

  Example of usage:
    @code

    int my_benchmark (void)
    {
        double walltime, tstop;
        const double tstart = cpl_test_get_walltime();

        myfunc();

        tstop = cpl_test_get_walltime();

        walltime = tstop - tstart;

        cpl_msg_info(cpl_func, "The call took %g seconds of wall-clock time",
                     walltime);

    }
   
    @endcode


 */
/*----------------------------------------------------------------------------*/
double cpl_test_get_walltime(void) {

#ifdef CPL_HAVE_CLOCK_GETTIME

    struct timespec buf;

    return clock_gettime(CLOCK_REALTIME, &buf) ? 0.0
        : (double)buf.tv_sec + 1.0e-9* (double)buf.tv_nsec;

#elif defined CPL_HAVE_GETTIMEOFDAY

    struct timeval buf;

    return gettimeofday(&buf, 0) ? 0.0
        : (double)buf.tv_sec + 1.0e-6 * (double)buf.tv_usec;
#else

    return 0.0;

#endif

}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Get the FLOPS count.
  @return   The FLOPS count
  @note This function is intended to be used only by the CPL test macros.

 */
/*----------------------------------------------------------------------------*/
inline cpl_flops cpl_test_get_flops(void)
{
    return cpl_tools_get_flops();
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the number of CPL tests performed.
  @return   The test count
  @see cpl_test_get_failed()

 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_test_get_tested(void)
{
    return cpl_test_count;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the number of failed CPL tests.
  @return   The count of failed tests
  @see cpl_test_get_tested()

  Example of usage:
    @code

    void my_tester (void)
    {
        const cpl_size prefailed = cpl_test_get_failed();

        cpl_test(mytest());

        if (cpl_test_get_failed() > prefailed) {
          cpl_msg_info(cpl_func, "The function mytest() failed!");
        }
    }
   
    @endcode

 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_test_get_failed(void)
{
    return cpl_test_failures;
}

/*----------------------------------------------------------------------------*/
/**
  @brief  Get the amount of storage [bytes] for the CPL object
  @param  self The CPL object
  @return The size in bytes
  @note Passing NULL is allowed and will return zero

  Example of usage:
    @code

    int my_benchmark (void)
    {
        const size_t storage = cpl_test_get_bytes_vector(mydata);
        double walltime, tstop;
        const double tstart = cpl_test_get_walltime();

        myfunc(mydata);

        tstop = cpl_test_get_walltime();

        walltime = tstop - tstart;

        if (walltime > 0.0) {
          cpl_msg_info(cpl_func, "Processing rate: %g",
                       (double)storage/walltime);
        }
    }
   
    @endcode


 */
/*----------------------------------------------------------------------------*/
size_t cpl_test_get_bytes_vector(const cpl_vector * self)
{
    return self == NULL ? 0
        : (size_t)cpl_vector_get_size(self) * sizeof(double);
}

/*----------------------------------------------------------------------------*/
/**
  @brief  Get the amount of storage [bytes] for the CPL object
  @param  self The CPL object
  @return The size in bytes
  @note Passing NULL is allowed and will return zero
  @see cpl_test_get_bytes_vector

 */
/*----------------------------------------------------------------------------*/
size_t cpl_test_get_bytes_matrix(const cpl_matrix * self)
{
    return self == NULL ? 0
        : (size_t)cpl_matrix_get_nrow(self)
        * (size_t)cpl_matrix_get_ncol(self) * sizeof(double);
}

/*----------------------------------------------------------------------------*/
/**
  @brief  Get the amount of storage [bytes] for the CPL object
  @param  self The CPL object
  @return The size in bytes
  @note Passing NULL is allowed and will return zero
  @see cpl_test_get_bytes_vector

 */
/*----------------------------------------------------------------------------*/
size_t cpl_test_get_bytes_image(const cpl_image * self)
{
    return self == NULL ? 0
        : (size_t)cpl_image_get_size_x(self)
        * (size_t)cpl_image_get_size_y(self)
        * cpl_type_get_sizeof(cpl_image_get_type(self));
}

/*----------------------------------------------------------------------------*/
/**
  @brief  Get the amount of storage [bytes] for the CPL object
  @param  self The CPL object
  @return The size in bytes
  @note Passing NULL is allowed and will return zero
  @see cpl_test_get_bytes_vector

 */
/*----------------------------------------------------------------------------*/
size_t cpl_test_get_bytes_imagelist(const cpl_imagelist * self)
{
    return self == NULL ? 0
        : (size_t)cpl_imagelist_get_size(self)
        * (size_t)cpl_test_get_bytes_image(cpl_imagelist_get_const(self, 0));
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Initialize CPL and unit-test environment
  @param    filename       __FILE__ of unit-test source code for log-file
  @param    report         The email address for the error message
  @param    default_level  Default level for messaging system
  @return   void
  @see cpl_test_init()
  @note This function should only be called by cpl_test_init()

 */
/*----------------------------------------------------------------------------*/
void cpl_test_init_macro(const char * filename, const char * report,
                         cpl_msg_severity default_level)
{
        /* ISO/IEC 9899:1999 (E) 7.5 3:
           The value of errno is zero at program startup, but is never set to 
           zero by any library function. The value of errno may be set to
           nonzero by a library function call whether or not there is an error,
           provided the use of errno is not documented in the
           description of the function in this International Standard.
        */
        /* As such it is safe to read errno at this stage,
           but a non-zero value has no significance. */
    const int errnopre = errno;
    int errnoini;


    assert(report   != NULL);
    assert(filename != NULL);

#ifdef _OPENMP
#pragma omp master
#endif
    {

        cpl_test_report = report;

        errno = 0;
        /* make sure we are checking for memory leaks */
        if (CPL_XMEMORY_MODE == 0) {
            setenv("CPL_MEMORY_MODE", "1", 0);
        }

        cpl_init(CPL_INIT_DEFAULT);

        cpl_test_time_start = cpl_test_get_walltime();
        cpl_test_time_one   = cpl_test_time_start;

        errnoini = errno;
        errno = 0;

        /* Needed on alphaev56 */
        if (signal(SIGFPE, SIG_IGN) == SIG_ERR) {
            cpl_msg_warning(cpl_func, "Could not install new signal handler "
                            "(SIG_IGN) for SIGFPE");
        }

        cleanstate = cpl_errorstate_get();
   
        cpl_msg_set_level(default_level);
        cpl_msg_set_level_from_env();

        if (filename != NULL) {

            const char * dotpos = NULL;
            char * logfile = NULL;

            /* Create a new string, where the extension is replaced with .log */
            /* First trim away any directory names before the filename. Check
               for both POSIX and Windows directory separator characters. This
               means no files can have these characters in their name, which is
               a reasonable requirement for portable software anyway. */

            const char * fslashpos = strrchr(filename, '\\');
            const char * bslashpos = strrchr(filename, '/');
            const char * slashpos = bslashpos > fslashpos ? bslashpos : fslashpos;
            if (slashpos != NULL) filename = slashpos+1;

            /* Check the special case of having filename = "..". In that case
               set filename to use "." instead, so that we end up with having
               ".log" as the log file name. */
            if (strcmp(filename, "..") == 0) filename = ".";
               
            /* Append .log in case there is no extension. In case there is an
               extension shorter than three characters, the new string will
               also be (more than) long enough for the .log extension.
               Take into account possible directory separator characters. */

            dotpos = strrchr(filename, '.');
            logfile = cpl_sprintf("%s.log", filename);

            if (dotpos != NULL) {
                /* Need to write new extension after the last '.' */
                (void)strcpy(logfile + (1 + dotpos - filename), "log");
            }

            cpl_msg_set_log_name(logfile);

            if (cpl_error_get_code()) {
                /* The log-file name could not be set */
                cpl_msg_warning(cpl_func, "Ignoring failed setting of "
                                "log-file:");
                cpl_errorstate_dump(cleanstate, CPL_FALSE,
                                    cpl_errorstate_dump_one_warning);

                cpl_test_reset(cleanstate);
            }

            /* Drop .log */
            logfile[strlen(logfile)-strlen(".log")] = '\0';

            cpl_msg_set_domain(logfile);

            cpl_free(logfile);

        }

        cpl_msg_set_log_level(CPL_MSG_DEBUG);

        if (errnopre != 0) {
            /* May be useful for debugging - but see the above errno comment */
            /* See also DFS04285 */
            cpl_msg_debug(cpl_func, "%s() was called with errno=%d: %s (Unless "
                          "you are debugging code prior to the cpl_init() call "
                          "you can ignore this message)",
                          cpl_func, errnopre, strerror(errnopre));
        }

        if (errnoini != 0) {
            /* May be useful for debugging - but see the above errno comment */
            cpl_msg_debug(cpl_func, "cpl_init() set errno=%d: %s (Unless "
                          "you are debugging cpl_init() you can ignore "
                          "this message)", errnoini, strerror(errnoini));
        }

    }

#ifdef _OPENMP
    /* No thread can start testing before the master is ready */
#pragma omp barrier
#endif

    if (cpl_error_get_code() != CPL_ERROR_NONE) {
        cpl_errorstate_dump_one(1, 1, 1); /* Dump the most recent error */
        assert(cpl_error_get_code() == CPL_ERROR_NONE);
        abort();
    }

    cpl_msg_debug(cpl_func, "sizeof(cpl_size): %u", (unsigned)sizeof(cpl_size));
#ifdef OFF_T
    cpl_msg_debug(cpl_func, "sizeof(OFF_T)=%u", (unsigned)sizeof(OFF_T));
#endif
    cpl_msg_debug(cpl_func, "%s", cpl_get_description(CPL_DESCRIPTION_DEFAULT));

    if (errno != 0) {
        cpl_msg_warning(cpl_func, "%s() set errno=%d: %s", cpl_func, errno,
                        strerror(errno));
        errno = 0;
    }

    cpl_test_state_ = 1;

    return;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Test a given boolean expression
  @param  errnopre     errno prior to expression evaluation
  @param  flopspre     FLOP count prior to expression evaluation
  @param  twallpre     Wall clock time prior to expression calculation
  @param  statepre     CPL errorstate prior to expression evaluation
  @param  expr         The integer expression to evaluate
  @param  fail_on_zero Fail iff the expression is zero
  @param  expr_txt     The integer expression to evaluate as a string
  @param  function     cpl_func
  @param  file         __FILE__
  @param  line         __LINE__
  @see    cpl_test()
  @note   This function should only be called via cpl_test()
  @note   CPL_FALSE of the boolean is a failure, CPL_TRUE is not

 */
/*----------------------------------------------------------------------------*/
void cpl_test_macro(int errnopre, double twallpre, cpl_flops flopspre,
                    cpl_errorstate statepre, cpl_size expression,
                    cpl_boolean fail_on_zero, const char * expr_txt,
                    const char * function, const char * file, unsigned line)
{

    char * message = cpl_sprintf(fail_on_zero ? "(%s) = %" CPL_SIZE_FORMAT :
                                 "(%s) = %" CPL_SIZE_FORMAT " <=> 0",
                                 expr_txt, expression);
    const cpl_boolean bool = fail_on_zero
        ? (expression ? CPL_TRUE : CPL_FALSE)
        : (expression ? CPL_FALSE : CPL_TRUE);

    cpl_test_one(errnopre, twallpre, flopspre, statepre, bool, message,
                 CPL_FALSE, function, file, line);

    cpl_free(message);

    return;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Test if a pointer is NULL
  @param  errnopre       errno prior to expression evaluation
  @param  flopspre       FLOP count prior to expression evaluation
  @param  twallpre       Wall clock time prior to expression calculation
  @param  statepre       CPL errorstate prior to expression evaluation
  @param  pointer        The pointer to check, side-effects are allowed
  @param  pointer_string The pointer as a string
  @param  function       cpl_func
  @param  file           __FILE__
  @param  line           __LINE__
  @see    cpl_test_null
  @note   This function should only be called from the macro cpl_test_null()

 */
/*----------------------------------------------------------------------------*/
void cpl_test_null_macro(int errnopre, double twallpre, cpl_flops flopspre,
                         cpl_errorstate statepre,
                         const void * pointer,  const char * pointer_string,
                         const char * function, const char * file,
                         unsigned line)
{

    char * message = cpl_sprintf("(%s) = %p == NULL", pointer_string,
                                 pointer);

    cpl_test_one(errnopre, twallpre, flopspre, statepre,
                 pointer == NULL ? CPL_TRUE : CPL_FALSE,
                 message, CPL_FALSE, function, file, line);

    cpl_free(message);

    return;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Test if a pointer is non-NULL
  @param  errnopre       errno prior to expression evaluation
  @param  flopspre       FLOP count prior to expression evaluation
  @param  twallpre       Wall clock time prior to expression calculation
  @param  statepre       CPL errorstate prior to expression evaluation
  @param  pointer        The pointer to check, side-effects are allowed
  @param  pointer_string The pointer as a string
  @param  function       cpl_func
  @param  file           __FILE__
  @param  line           __LINE__
  @see    cpl_test_nonnull
  @note   This function should only be called from the macro cpl_test_nonnull()

 */
/*----------------------------------------------------------------------------*/
void cpl_test_nonnull_macro(int errnopre, double twallpre, cpl_flops flopspre,
                            cpl_errorstate statepre,
                            const void * pointer,  const char * pointer_string,
                            const char * function, const char * file,
                            unsigned line)
{

    char * message = cpl_sprintf("(%s) = %p != NULL", pointer_string,
                                 pointer);

    cpl_test_one(errnopre, twallpre, flopspre, statepre,
                 pointer != NULL ? CPL_TRUE : CPL_FALSE,
                 message, CPL_FALSE, function, file, line);

    cpl_free(message);

    return;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Test if two integer expressions are equal
  @param errnopre      errno prior to expression evaluation
  @param flopspre      FLOP count prior to expression evaluation
  @param twallpre      Wall clock time prior to expression calculation
  @param statepre      CPL errorstate prior to expression evaluation
  @param first         The first value in the comparison
  @param first_string  The first value as a string
  @param second        The second value in the comparison
  @param second_string The second value as a string
  @param function      cpl_func
  @param file          __FILE__
  @param line          __LINE__
  @see   cpl_test_eq
  @note  This function should only be called from the macro cpl_test_eq()

 */
/*----------------------------------------------------------------------------*/
void cpl_test_eq_macro(int errnopre, double twallpre, cpl_flops flopspre,
                       cpl_errorstate statepre,
                       cpl_size first,  const char * first_string,
                       cpl_size second, const char * second_string,
                       const char * function, const char * file, unsigned line)
{

    char * message = cpl_sprintf("(%s) = %" CPL_SIZE_FORMAT "; (%s) = %"
                                 CPL_SIZE_FORMAT, first_string, first,
                                 second_string, second);

    cpl_test_one(errnopre, twallpre, flopspre, statepre,
                 first == second ? CPL_TRUE : CPL_FALSE,
                 message, CPL_FALSE, function, file, line);

    cpl_free(message);

    return;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Test if two integer expressions are not equal
  @param errnopre      errno prior to expression evaluation
  @param flopspre      FLOP count prior to expression evaluation
  @param twallpre      Wall clock time prior to expression calculation
  @param statepre      CPL errorstate prior to expression evaluation
  @param first         The first value in the comparison
  @param first_string  The first value as a string
  @param second        The second value in the comparison
  @param second_string The second value as a string
  @param function      cpl_func
  @param file          __FILE__
  @param line          __LINE__
  @see   cpl_test_noneq
  @note  This function should only be called from the macro cpl_test_noneq()

 */
/*----------------------------------------------------------------------------*/
void cpl_test_noneq_macro(int errnopre, double twallpre, cpl_flops flopspre,
                          cpl_errorstate statepre,
                          cpl_size first,  const char * first_string,
                          cpl_size second, const char * second_string,
                          const char * function, const char * file,
                          unsigned line)
{

    char * message = cpl_sprintf("(%s) = %" CPL_SIZE_FORMAT "; (%s) = %"
                                 CPL_SIZE_FORMAT,
                                 first_string, first,
                                 second_string, second);

    cpl_test_one(errnopre, twallpre, flopspre, statepre,
                 first != second ? CPL_TRUE : CPL_FALSE,
                 message, CPL_FALSE, function, file, line);

    cpl_free(message);

    return;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Test if two pointer expressions are equal
  @param errnopre      errno prior to expression evaluation
  @param flopspre      FLOP count prior to expression evaluation
  @param twallpre      Wall clock time prior to expression calculation
  @param statepre      CPL errorstate prior to expression evaluation
  @param first         The first value in the comparison
  @param first_string  The first value as a string
  @param second        The second value in the comparison
  @param second_string The second value as a string
  @param function      cpl_func
  @param file          __FILE__
  @param line          __LINE__
  @see   cpl_test_eq_ptr
  @note  This function should only be called from the macro cpl_test_eq_ptr()

*/
/*----------------------------------------------------------------------------*/
void cpl_test_eq_ptr_macro(int errnopre, double twallpre, cpl_flops flopspre,
                           cpl_errorstate statepre,
                           const void * first,  const char * first_string,
                           const void * second, const char * second_string,
                           const char * function, const char * file,
                           unsigned line)
{

    char * message = cpl_sprintf("(%s) = %p; (%s) = %p",
                                 first_string, first,
                                 second_string, second);

    cpl_test_one(errnopre, twallpre, flopspre, statepre,
                 first == second ? CPL_TRUE : CPL_FALSE,
                 message, CPL_FALSE, function, file, line);

    cpl_free(message);

    return;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Test if two pointer expressions are not equal
  @param errnopre      errno prior to expression evaluation
  @param flopspre      FLOP count prior to expression evaluation
  @param twallpre      Wall clock time prior to expression calculation
  @param statepre      CPL errorstate prior to expression evaluation
  @param first         The first value in the comparison
  @param first_string  The first value as a string
  @param second        The second value in the comparison
  @param second_string The second value as a string
  @param function      cpl_func
  @param file          __FILE__
  @param line          __LINE__
  @see   cpl_test_noneq_ptr
  @note This function should only be called from the macro cpl_test_noneq_ptr()

 */
/*----------------------------------------------------------------------------*/
void cpl_test_noneq_ptr_macro(int errnopre, double twallpre, cpl_flops flopspre,
                              cpl_errorstate statepre,
                              const void * first,  const char * first_string,
                              const void * second, const char * second_string,
                              const char * function, const char * file,
                              unsigned line)
{

    char * message = cpl_sprintf("(%s) = %p; (%s) = %p",
                                 first_string, first,
                                 second_string, second);

    cpl_test_one(errnopre, twallpre, flopspre, statepre,
                 first != second ? CPL_TRUE : CPL_FALSE,
                 message, CPL_FALSE, function, file, line);

    cpl_free(message);

    return;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Test if two strings are equal
  @param errnopre      errno prior to expression evaluation
  @param flopspre      FLOP count prior to expression evaluation
  @param twallpre      Wall clock time prior to expression calculation
  @param statepre      CPL errorstate prior to expression evaluation
  @param first         The first string or NULL of the comparison
  @param first_string  The first value as a string
  @param second        The second string or NULL of the comparison
  @param second_string The second value as a string
  @param function      function name
  @param file          filename
  @param line          line number
  @see   cpl_test_eq_string()
  @note  This function should only be called from cpl_test_eq_string()
 */
/*----------------------------------------------------------------------------*/
void cpl_test_eq_string_macro(int errnopre, double twallpre, cpl_flops flopspre,
                              cpl_errorstate statepre,
                              const char * first,  const char * first_string,
                              const char * second, const char * second_string,
                              const char * function, 
                              const char * file, unsigned line)
{
    char * fsquote = first  == NULL ? NULL : cpl_sprintf("'%s'",  first);
    char * ssquote = second == NULL ? NULL : cpl_sprintf("'%s'", second);
    const char * fquote = fsquote == NULL ? "NULL" : fsquote;
    const char * squote = ssquote == NULL ? "NULL" : ssquote;
    char * message = cpl_sprintf("%s = %s; %s = %s", first_string, fquote,
                                 second_string, squote);

    cpl_free(fsquote);
    cpl_free(ssquote);

    cpl_test_one(errnopre, twallpre, flopspre, statepre,
                 first != NULL && second != NULL && strcmp(first, second) == 0, 
                 message, CPL_FALSE, function, file, line);

    cpl_free(message);

    return;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Test if two strings are not equal
  @param errnopre      errno prior to expression evaluation
  @param flopspre      FLOP count prior to expression evaluation
  @param twallpre      Wall clock time prior to expression calculation
  @param statepre      CPL errorstate prior to expression evaluation
  @param first         The first string or NULL of the comparison
  @param first_string  The first value as a string
  @param second        The second string or NULL of the comparison
  @param second_string The second value as a string
  @param function      function name
  @param file          filename
  @param line          line number
  @see   cpl_test_noneq_string()
  @note  This function should only be called from cpl_test_noneq_string()
 */
/*----------------------------------------------------------------------------*/
void cpl_test_noneq_string_macro(int errnopre, double twallpre,
                                 cpl_flops flopspre, cpl_errorstate statepre,
                                 const char * first, const char * first_string,
                                 const char * second,
                                 const char * second_string,
                                 const char * function, 
                                 const char * file, unsigned line)
{
    char * fsquote = first  == NULL ? NULL : cpl_sprintf("'%s'",  first);
    char * ssquote = second == NULL ? NULL : cpl_sprintf("'%s'", second);
    const char * fquote = fsquote == NULL ? "NULL" : fsquote;
    const char * squote = ssquote == NULL ? "NULL" : ssquote;
    char * message = cpl_sprintf("%s = %s; %s = %s", first_string, fquote,
                                 second_string, squote);

    cpl_free(fsquote);
    cpl_free(ssquote);

    cpl_test_one(errnopre, twallpre, flopspre, statepre,
                 first != NULL && second != NULL && strcmp(first, second) != 0, 
                 message, CPL_FALSE, function, file, line);

    cpl_free(message);

    return;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Test if a file is valid FITS using an external verification utility
  @param errnopre        errno prior to expression evaluation
  @param flopspre        FLOP count prior to expression evaluation
  @param twallpre        Wall clock time prior to expression calculation
  @param statepre        CPL errorstate prior to expression evaluation
  @param filename        The file to verify
  @param filename_string The file to verify as a string
  @param function        cpl_func
  @param file            __FILE__
  @param line            __LINE__
  @see   cpl_test_fits()
  @note  This function should only be called from cpl_test_fits()

 */
/*----------------------------------------------------------------------------*/
void cpl_test_fits_macro(int errnopre, double twallpre, cpl_flops flopspre,
                         cpl_errorstate statepre,
                         const char * filename, const char * filename_string,
                         const char * function, const char * file,
                         unsigned line)
{
    const char * checker = getenv(CPL_TEST_FITS);
    char * message;
    cpl_boolean expression;

    if (filename == NULL) {
        message = cpl_sprintf(CPL_TEST_FITS " unusable on NULL-file "
                              "%s", filename_string);
        expression = CPL_FALSE; /* Unable to do an actual test */
    } else if (checker == NULL) {

        message = cpl_test_fits_file(filename, filename_string);
        if (message != NULL) {
            expression = CPL_FALSE; /* File cannot be FITS */
        } else {
            /* The previous FITS validation is so primitive that its
               success is not reported */
            message = cpl_sprintf(CPL_TEST_FITS " undefined for file %s='%s', "
                                  "try: export " CPL_TEST_FITS "=fitsverify",
                                  filename_string, filename);
            expression = CPL_TRUE; /* Unable to do an actual test */
        }
    } else {
        const char * redir = cpl_msg_get_level() < CPL_MSG_WARNING ? ""
            : (cpl_msg_get_level() < CPL_MSG_ERROR ? " > /dev/null"
               : " > /dev/null 2>&1");
        char * cmd = cpl_sprintf("%s %s %s", checker, filename, redir);

        message = cpl_sprintf(CPL_TEST_FITS " on file %s: %s",
                              filename_string, cmd);

        expression = system(cmd) == 0 ? CPL_TRUE : CPL_FALSE;

        cpl_free(cmd);

    }

    cpl_test_one(errnopre, twallpre, flopspre, statepre, expression, message,
                 CPL_FALSE, function, file, line);

    cpl_free(message);

    return;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Test if two CPL masks are equal
  @param errnopre    errno prior to expression evaluation
  @param flopspre    FLOP count prior to expression evaluation
  @param twallpre    Wall clock time prior to expression calculation
  @param statepre    CPL errorstate prior to expression evaluation
  @param first       The first mask or NULL of the comparison
  @param first_mask  The first value as a string
  @param second      The second mask or NULL of the comparison
  @param second_mask The second value as a string
  @param function    function name
  @param file        filename
  @param line        line number
  @see   cpl_test_eq_mask()
  @note  This function should only be called from cpl_test_eq_mask()
 */
/*----------------------------------------------------------------------------*/
void cpl_test_eq_mask_macro(int errnopre, double twallpre, cpl_flops flopspre,
                            cpl_errorstate statepre,
                            const cpl_mask * first, const char * first_string,
                            const cpl_mask * second, const char * second_string,
                            const char * function, const char * file,
                            unsigned line)
{
    cpl_errorstate mystate = cpl_errorstate_get();
    const cpl_size nx1 = cpl_mask_get_size_x(first);
    const cpl_size ny1 = cpl_mask_get_size_y(first);
    const cpl_size nx2 = cpl_mask_get_size_x(second);
    const cpl_size ny2 = cpl_mask_get_size_y(second);
    cpl_boolean  expression;
    char * message;

    if (!cpl_errorstate_is_equal(mystate)) {
        cpl_error_set(cpl_func, cpl_error_get_code());
        expression = CPL_FALSE;
        message = cpl_sprintf("%s <=> %s input error",
                              first_string, second_string);
    } else if (nx1 != nx2 || ny1 != ny2) {
        expression = CPL_FALSE;
        message = cpl_sprintf("%s <=> %s incompatible input, nx: %"
                              CPL_SIZE_FORMAT " <=> %" CPL_SIZE_FORMAT ", ny: %"
                              CPL_SIZE_FORMAT " <=> %" CPL_SIZE_FORMAT,
                              first_string, second_string,
                              nx1, nx2, ny1, ny2);
    } else if (memcmp(cpl_mask_get_data_const(first),
                      cpl_mask_get_data_const(second), (size_t)(nx1 * ny1))) {
        /* The test has failed, now spend extra time to report why */
        cpl_size i;
        cpl_size k = 0;
        cpl_size n = 0;

        const cpl_binary * pbpm1 = cpl_mask_get_data_const(first);
        const cpl_binary * pbpm2 = cpl_mask_get_data_const(second);

        for (i = 0; i < nx1 * ny1; i++) {
            if (pbpm1[i] != pbpm2[i]) {
                k = i;
                n++;
            }
        }
        assert( n != 0 );

        expression = CPL_FALSE;
        message
            = cpl_sprintf("%s(%" CPL_SIZE_FORMAT ",%" CPL_SIZE_FORMAT
                          ") = %u <=> %u = %s(%" CPL_SIZE_FORMAT ",%"
                          CPL_SIZE_FORMAT ") (%" CPL_SIZE_FORMAT " of %"
                          CPL_SIZE_FORMAT " x %" CPL_SIZE_FORMAT " "
                          "differ(s))", first_string, 1+k%nx1, 1+k/nx1,
                          (unsigned)pbpm1[k], (unsigned)pbpm2[k], second_string,
                          1+k%nx2, 1+k/nx2, n, nx1, ny1);

    } else {
        expression = CPL_TRUE;
        message = cpl_sprintf("%s == %s", first_string, second_string);
    }

    cpl_test_one(errnopre, twallpre, flopspre, statepre, expression, message,
                 CPL_FALSE, function, file, line);
    cpl_errorstate_set(mystate);

    cpl_free(message);

    return;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Evaluate A <= B and update an internal counter if it is not true
  @param errnopre  errno prior to expression evaluation
  @param flopspre  FLOP count prior to expression evaluation
  @param twallpre  Wall clock time prior to expression calculation
  @param statepre  CPL errorstate prior to expression evaluation
  @param value     The double-precision number to test
  @param tolerance The double-precision upper limit to compare against
  @param function  cpl_func
  @param file      __FILE__
  @param line      __LINE__
  @see   cpl_test_leq()
  @note  This function should only be called via cpl_test_leq_macro()

 */
/*----------------------------------------------------------------------------*/
void cpl_test_leq_macro(int errnopre, double twallpre, cpl_flops flopspre,
                        cpl_errorstate statepre,
                        double value, const char * value_string,
                        double tolerance, const char * tolerance_string,
                        const char * function, const char * file, unsigned line)
{
    const cpl_boolean expression
        = (value <= tolerance) ? CPL_TRUE : CPL_FALSE;
    char * message = cpl_sprintf("%s = %g <= %g = %s", value_string,
                                 value, tolerance, tolerance_string);

    cpl_test_one(errnopre, twallpre, flopspre, statepre, expression, message,
                 CPL_FALSE, function, file, line);

    cpl_free(message);

    return;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Evaluate A < B and update an internal counter if it is not true
  @param errnopre  errno prior to expression evaluation
  @param flopspre  FLOP count prior to expression evaluation
  @param twallpre  Wall clock time prior to expression calculation
  @param statepre  CPL errorstate prior to expression evaluation
  @param value     The double-precision number to test
  @param tolerance The double-precision upper limit to compare against
  @param function  cpl_func
  @param file      __FILE__
  @param line      __LINE__
  @see   cpl_test_lt()
  @note  This function should only be called via cpl_test_leq_macro()

 */
/*----------------------------------------------------------------------------*/
void cpl_test_lt_macro(int errnopre, double twallpre, cpl_flops flopspre,
                       cpl_errorstate statepre,
                       double value, const char * value_string,
                       double tolerance, const char * tolerance_string,
                       const char * function, const char * file, unsigned line)
{
    const cpl_boolean expression
        = (value < tolerance) ? CPL_TRUE : CPL_FALSE;
    char * message = cpl_sprintf("%s = %g < %g = %s", value_string,
                                 value, tolerance, tolerance_string);

    cpl_test_one(errnopre, twallpre, flopspre, statepre, expression, message,
                 CPL_FALSE, function, file, line);

    cpl_free(message);

    return;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Test if two numerical expressions are 
         within a given (absolute) tolerance
  @param errnopre         errno prior to expression evaluation
  @param flopspre         FLOP count prior to expression evaluation
  @param twallpre         Wall clock time prior to expression calculation
  @param statepre         CPL errorstate prior to expression evaluation
  @param first            The first value in the comparison
  @param first_string     The first value as a string
  @param second           The second value in the comparison
  @param second_string    The second value as a string
  @param tolerance        A non-negative tolerance
  @param tolerance_string The tolerance as a string
  @param function         function name
  @param file             filename
  @param line             line number
  @see   cpl_test_abs()
  @note  This function should only be called from the macro cpl_test_abs()
 */
/*----------------------------------------------------------------------------*/
void cpl_test_abs_macro(int errnopre, double twallpre, cpl_flops flopspre,
                        cpl_errorstate statepre,
                        double first,  const char *first_string,
                        double second, const char *second_string,
                        double tolerance, const char *tolerance_string,
                        const char *function, const char *file, unsigned line)
{
    const cpl_boolean expression
        = (fabs(first - second) <= tolerance) ? CPL_TRUE : CPL_FALSE;
    char *message = cpl_sprintf("|%s - %s| = |%g - %g| = |%g| <= %g = %s",
                                first_string, second_string, first,
                                second, first - second, tolerance,
                                tolerance_string);

    cpl_test_one(errnopre, twallpre, flopspre, statepre, expression, message,
                 CPL_FALSE, function, file, line);

    cpl_free(message);

    return;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Test if two complex expressions are 
         within a given (absolute) tolerance
  @param errnopre         errno prior to expression evaluation
  @param flopspre         FLOP count prior to expression evaluation
  @param twallpre         Wall clock time prior to expression calculation
  @param statepre         CPL errorstate prior to expression evaluation
  @param first            The first value in the comparison
  @param first_string     The first value as a string
  @param second           The second value in the comparison
  @param second_string    The second value as a string
  @param tolerance        A non-negative tolerance
  @param tolerance_string The tolerance as a string
  @param function         function name
  @param file             filename
  @param line             line number
  @see   cpl_test_abs_complex()
  @note  This function should only be called from the macro 
                                                    cpl_test_abs_complex()
 */
/*----------------------------------------------------------------------------*/
void cpl_test_abs_complex_macro(int errnopre, double twallpre, 
                                cpl_flops flopspre, cpl_errorstate statepre,
                                double complex first,  
                                const char *first_string,
                                double complex second, 
                                const char *second_string,
                                double tolerance, 
                                const char *tolerance_string,
                                const char *function, const char *file, 
                                unsigned line)
{
    const cpl_boolean expression
        = (cabs(first - second) <= tolerance) ? CPL_TRUE : CPL_FALSE;
    char *message = 
      cpl_sprintf("|%s - %s| = |(%g%+gi) - (%g%+gi)| = "
                  "|%g%+gi| = %g <= %g = %s",
                                first_string, second_string, creal(first),
                                cimag(first), creal(second), cimag(second),
                                creal(first - second), cimag(first - second),
                                cabs(first - second),
                                tolerance, tolerance_string);

    cpl_test_one(errnopre, twallpre, flopspre, statepre, expression, message,
                 CPL_FALSE, function, file, line);

    cpl_free(message);

    return;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Test if two numerical expressions are 
         within a given relative tolerance
  @param errnopre         errno prior to expression evaluation
  @param flopspre         FLOP count prior to expression evaluation
  @param twallpre         Wall clock time prior to expression calculation
  @param statepre         CPL errorstate prior to expression evaluation
  @param first            The first value in the comparison
  @param first_string     The first value as a string
  @param second           The second value in the comparison
  @param second_string    The second value as a string
  @param tolerance        A non-negative tolerance
  @param tolerance_string The tolerance as a string
  @param function         function name
  @param file             filename
  @param line             line number
  @see   cpl_test_rel()
  @note  This function should only be called from the macro cpl_test_rel()
 */
/*----------------------------------------------------------------------------*/
void cpl_test_rel_macro(int errnopre, double twallpre, cpl_flops flopspre,
                        cpl_errorstate statepre,
                        double first,  const char *first_string,
                        double second, const char *second_string,
                        double tolerance, const char *tolerance_string,
                        const char *function, const char *file, unsigned line)
{
    char * message = NULL; /* Avoid (false) uninit warnings */
    cpl_boolean expression;

    if (tolerance < 0.0) {
        expression = CPL_FALSE;
        message = cpl_sprintf("%s = %g; %s = %g. Negative tolerance %s = %g",
                              first_string, first,
                              second_string, second,
                              tolerance_string, tolerance);
    } else if (first == second) {
        /* Not needed. Used only for prettier messaging */
        expression = CPL_TRUE;
        message = cpl_sprintf("%s = %g = %s. (Tolerance %s = %g)",
                              first_string, first, second_string,
                              tolerance_string, tolerance);

    } else if (first == 0.0) {
        /* Not needed. Used only for prettier messaging */
        expression = CPL_FALSE;
        message = cpl_sprintf("%s = zero; %s = non-zero (%g). (Tolerance "
                              "%s = %g)", first_string, second_string,
                              second, tolerance_string, tolerance);
    } else if (second == 0.0) {
        /* Not needed. Used only for prettier messaging */
        expression = CPL_FALSE;
        message = cpl_sprintf("%s = non-zero (%g); %s = zero. (Tolerance "
                              "%s = %g)", first_string, first, second_string,
                              tolerance_string, tolerance);
    } else if (fabs(first) < fabs(second)) {
        expression = fabs(first - second) <= tolerance * fabs(first)
            ? CPL_TRUE : CPL_FALSE;

        message = cpl_sprintf("|%s - %s|/|%s| = |%g - %g|/|%g| = |%g|/|%g|"
                              " <= %g = %s", first_string, second_string,
                              first_string, first, second, first,
                              first - second, first, tolerance,
                              tolerance_string);            

    } else {
        /* assert(fabs(second) < fabs(first)) */
        expression = fabs(first - second) <= tolerance * fabs(second)
            ? CPL_TRUE : CPL_FALSE;

        message = cpl_sprintf("|%s - %s|/|%s| = |%g - %g|/|%g| = |%g|/|%g|"
                              " <= %g = %s", first_string, second_string,
                              second_string, first, second, second,
                              first - second, second, tolerance,
                              tolerance_string);            

    }

    cpl_test_one(errnopre, twallpre, flopspre, statepre, expression, message,
                 CPL_FALSE, function, file, line);

    cpl_free(message);

    return;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Test if two CPL vectors are identical
          within a given (absolute) tolerance
  @param errnopre         errno prior to expression evaluation
  @param flopspre         FLOP count prior to expression evaluation
  @param twallpre         Wall clock time prior to expression calculation
  @param statepre         CPL errorstate prior to expression evaluation
  @param first            The first vector in the comparison
  @param first_string     The first vector as a string
  @param second           The second vector of identical size in the comparison
  @param second_string    The second vector as a string
  @param tolerance        A non-negative tolerance
  @param tolerance_string The tolerance as a string
  @param function         function name
  @param file             filename
  @param line             line number
  @see   cpl_test_vector_abs()
  @note This function should only be called from the macro cpl_test_vector_abs()
 */
/*----------------------------------------------------------------------------*/
void cpl_test_vector_abs_macro(int errnopre, double twallpre,
                               cpl_flops flopspre, cpl_errorstate statepre,
                               const cpl_vector * first,
                               const char *first_string,
                               const cpl_vector * second,
                               const char *second_string,
                               double tolerance, const char *tolerance_string,
                               const char *function, const char *file,
                               unsigned line)
{
    cpl_errorstate mystate = cpl_errorstate_get();
    cpl_vector   * diff    = cpl_vector_duplicate(first);
    cpl_boolean    expression;
    char         * message;

    (void)cpl_vector_subtract(diff, second);

    if (!cpl_errorstate_is_equal(mystate)) {
        cpl_error_set(cpl_func, cpl_error_get_code());
        expression = CPL_FALSE;

        message = cpl_sprintf("%s <=> %s (tol=%s) input error:",
                              first_string, second_string,
                              tolerance_string);
    } else {

        const double * pdiff = cpl_vector_get_data_const(diff);
        double difval = pdiff[0];
        const cpl_size n = cpl_vector_get_size(diff);
        cpl_size pos = 0;
        cpl_size i;

        for (i = 1; i < n; i++) {
            if (fabs(pdiff[i]) > fabs(difval)) {
                pos = i;
                difval = pdiff[i];
            }
        }

        if (cpl_errorstate_is_equal(mystate)) {
            const double val1 = cpl_vector_get(first,  pos);
            const double val2 = cpl_vector_get(second, pos);

            expression = (fabs(difval) <= tolerance) ? CPL_TRUE : CPL_FALSE;

            message = cpl_sprintf("|%s(%" CPL_SIZE_FORMAT ") - %s(%"
                                  CPL_SIZE_FORMAT ")| = "
                                  "|%g - %g| = |%g| <= %g = %s",
                                  first_string, pos,
                                  second_string, pos,
                                  val1, val2, difval, tolerance,
                                  tolerance_string);

        } else {
            cpl_error_set(cpl_func, cpl_error_get_code());

            expression = CPL_FALSE;

            message = cpl_sprintf("%s <=> %s (tol=%s) input error:",
                                  first_string, second_string,
                                  tolerance_string);
        }
    }

    cpl_test_one(errnopre, twallpre, flopspre, statepre, expression, message,
                 CPL_FALSE, function, file, line);

    cpl_errorstate_set(mystate);

    cpl_vector_delete(diff);
    cpl_free(message);

    return;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Test if two CPL matrices are identical
          within a given (absolute) tolerance
  @param errnopre         errno prior to expression evaluation
  @param flopspre         FLOP count prior to expression evaluation
  @param twallpre         Wall clock time prior to expression calculation
  @param statepre         CPL errorstate prior to expression evaluation
  @param first            The first matrix in the comparison
  @param first_string     The first matrix as a string
  @param second           The second matrix of identical size in the comparison
  @param second_string    The second matrix as a string
  @param tolerance        A non-negative tolerance
  @param tolerance_string The tolerance as a string
  @param function         function name
  @param file             filename
  @param line             line number
  @see   cpl_test_matrix_abs()
  @note This function should only be called from the macro cpl_test_matrix_abs()
 */
/*----------------------------------------------------------------------------*/
void cpl_test_matrix_abs_macro(int errnopre, double twallpre,
                               cpl_flops flopspre, cpl_errorstate statepre,
                               const cpl_matrix * first,
                               const char *first_string,
                               const cpl_matrix * second,
                               const char *second_string,
                               double tolerance, const char *tolerance_string,
                               const char *function, const char *file,
                               unsigned line)
{
    cpl_errorstate mystate = cpl_errorstate_get();
    cpl_matrix   * diff    = cpl_matrix_duplicate(first);
    cpl_boolean    expression;
    char         * message;

    (void)cpl_matrix_subtract(diff, second);

    if (!cpl_errorstate_is_equal(mystate)) {
        cpl_error_set(cpl_func, cpl_error_get_code());
        expression = CPL_FALSE;

        message = cpl_sprintf("%s <=> %s (tol=%s) input error:",
                              first_string, second_string,
                              tolerance_string);
    } else {

        const double * pdiff = cpl_matrix_get_data_const(diff);
        double difval = pdiff[0];
        const cpl_size ncol = cpl_matrix_get_ncol(diff);
        const cpl_size n    = cpl_matrix_get_nrow(diff) * ncol;
        cpl_size pos = 0;
        cpl_size i;

        for (i = 1; i < n; i++) {
            if (fabs(pdiff[i]) > fabs(difval)) {
                pos = i;
                difval = pdiff[i];
            }
        }

        if (cpl_errorstate_is_equal(mystate)) {
            const cpl_size irow = pos / ncol;
            const cpl_size icol = pos % ncol;
            const double val1 = cpl_matrix_get(first, irow, icol);
            const double val2 = cpl_matrix_get(second, irow, icol);

            expression = (fabs(difval) <= tolerance) ? CPL_TRUE : CPL_FALSE;

            message = cpl_sprintf("|%s(%" CPL_SIZE_FORMAT ",%" CPL_SIZE_FORMAT
                                  ") - %s(%" CPL_SIZE_FORMAT ",%"
                                  CPL_SIZE_FORMAT ")| = |%g - %g| = "
                                  "|%g| <= %g = %s",
                                  first_string, irow, icol,
                                  second_string, irow, icol,
                                  val1, val2, difval, tolerance,
                                  tolerance_string);

        } else {
            cpl_error_set(cpl_func, cpl_error_get_code());

            expression = CPL_FALSE;

            message = cpl_sprintf("%s <=> %s (tol=%s) input error:",
                                  first_string, second_string,
                                  tolerance_string);
        }
    }

    cpl_test_one(errnopre, twallpre, flopspre, statepre, expression, message,
                 CPL_FALSE, function, file, line);

    cpl_errorstate_set(mystate);

    cpl_matrix_delete(diff);
    cpl_free(message);

    return;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Test if two CPL numerical arrays are identical
         within a given (absolute) tolerance
  @param errnopre         errno prior to expression evaluation
  @param flopspre         FLOP count prior to expression evaluation
  @param twallpre         Wall clock time prior to expression calculation
  @param statepre         CPL errorstate prior to expression evaluation
  @param first            The first array in the comparison
  @param first_string     The first array as a string
  @param second           The second array of identical size in the comparison
  @param second_string    The second array as a string
  @param tolerance        A non-negative tolerance
  @param tolerance_string The tolerance as a string
  @param function         function name
  @param file             filename
  @param line             line number
  @see   cpl_test_array_abs()
  @note  This function should only be called from the macro cpl_test_array_abs()
 */
/*----------------------------------------------------------------------------*/
void cpl_test_array_abs_macro(int errnopre, double twallpre, cpl_flops flopspre,
                              cpl_errorstate statepre,
                              const cpl_array * first,
                              const char *first_string,
                              const cpl_array * second,
                              const char *second_string,
                              double tolerance, const char *tolerance_string,
                              const char *function, const char *file,
                              unsigned line)
{

    /* Modified from cpl_test_image_abs_macro() */

    cpl_errorstate mystate = cpl_errorstate_get();
    const cpl_type type1 = cpl_array_get_type(first);
    const cpl_type type2 = cpl_array_get_type(second);
#ifdef CPL_SIZE_FORMAT
    const cpl_size nbad1 = cpl_array_count_invalid(first);
    const cpl_size nbad2 = cpl_array_count_invalid(second);
    const cpl_size nx = cpl_array_get_size(first);
#else
    const int nbad1 = cpl_array_count_invalid(first);
    const int nbad2 = cpl_array_count_invalid(second);
    const int nx = cpl_array_get_size(first);
#endif

    cpl_array  * diff   = cpl_array_duplicate(first);
    cpl_boolean  expression;
    char * message;

    (void)cpl_array_subtract(diff, second);

    if (tolerance < 0.0) {
        expression = CPL_FALSE;
        message = cpl_sprintf("array1=%s; array2=%s. Negative tolerance %s = "
                              "%g", first_string, second_string,
                              tolerance_string, tolerance);
    } else if (!cpl_errorstate_is_equal(mystate)) {
        cpl_error_set(cpl_func, cpl_error_get_code());
        expression = CPL_FALSE;

#ifdef CPL_SIZE_FORMAT
        message = cpl_sprintf("%s(%" CPL_SIZE_FORMAT ", %s) <=> %s(%" 
                              CPL_SIZE_FORMAT ", %s) (tol=%s) input error:",
                              first_string, nx, cpl_type_get_name(type1),
                              second_string, cpl_array_get_size(second),
                              cpl_type_get_name(type2), tolerance_string);
#else
        message = cpl_sprintf("%s(%d, %s) <=> %s(%d, %s) (tol=%s) input error:",
                              first_string, nx, cpl_type_get_name(type1),
                              second_string, cpl_array_get_size(second),
                              cpl_type_get_name(type2), tolerance_string);
#endif
    } else if (nbad1 == nbad2 && nbad1 == nx) {
        expression = CPL_TRUE;
#ifdef CPL_SIZE_FORMAT
        message = cpl_sprintf("%s(%" CPL_SIZE_FORMAT ", %s) <=> %s(%" 
                              CPL_SIZE_FORMAT  ", %s) (tol=%s) All elements "
                              "are bad",
                              first_string, nx, cpl_type_get_name(type1),
                              second_string, cpl_array_get_size(second),
                              cpl_type_get_name(type2), tolerance_string);
#else
        message = cpl_sprintf("%s(%d, %s) <=> %s(%d, %s) (tol=%s) "
                              "All elements are bad", first_string, 
                              nx, cpl_type_get_name(type1),
                              second_string, cpl_array_get_size(second),
                              cpl_type_get_name(type2), tolerance_string);
#endif
    } else if (cpl_array_count_invalid(diff) == nx) {
        expression = CPL_FALSE;
#ifdef CPL_SIZE_FORMAT
        message = cpl_sprintf("%s(%" CPL_SIZE_FORMAT ", %s) <=> %s(%" 
                              CPL_SIZE_FORMAT ", %s) (tol=%s) All elements "
                              "are bad in the first (%" CPL_SIZE_FORMAT 
                              ") or second (%" CPL_SIZE_FORMAT "d) array",
                              first_string, nx, cpl_type_get_name(type1),
                              second_string, cpl_array_get_size(second),
                              cpl_type_get_name(type2), tolerance_string,
                              nbad1, nbad2);
#else
        message = cpl_sprintf("%s(%d, %s) <=> %s(%d, %s) (tol=%s) All elements "
                              "are bad in the first (%d) or second (%d) array",
                              first_string, nx, cpl_type_get_name(type1),
                              second_string, cpl_array_get_size(second),
                              cpl_type_get_name(type2), tolerance_string,
                              nbad1, nbad2);
#endif
    } else {

        const double maxdif = cpl_array_get_max(diff);
        const double mindif = cpl_array_get_min(diff);

        const cpl_boolean is_pos = (maxdif >= -mindif) ? CPL_TRUE : CPL_FALSE;
        const double difval = is_pos ? maxdif : mindif;

#ifdef CPL_SIZE_FORMAT
        cpl_size posx;
#else
        int      posx;
#endif
        const cpl_error_code error = (is_pos ? cpl_array_get_maxpos
                                      : cpl_array_get_minpos) (diff, &posx);

        int          is_bad1;
        int          is_bad2;

        const double val1
            = (type1 == CPL_TYPE_INT ? (double)cpl_array_get_int(first,  posx,
                                                                 &is_bad1)
               : (type1 == CPL_TYPE_FLOAT
                  ? (double)cpl_array_get_float(first,  posx, &is_bad1)
                  : cpl_array_get_double(first,  posx, &is_bad1)));

        const double val2
               = (type2 == CPL_TYPE_INT
                  ? (double)cpl_array_get_int(second,  posx, &is_bad2)
                  : (type2 == CPL_TYPE_FLOAT
                     ? (double)cpl_array_get_float(second,  posx, &is_bad2)
                     : cpl_array_get_double(second,  posx, &is_bad2)));

        if (!error && cpl_errorstate_is_equal(mystate)) {


            const char * rejstr1 = is_bad1 ? " invalid" : " valid";
            const char * rejstr2 = is_bad2 ? " invalid" : " valid";

            expression = (fabs(difval) <= tolerance) ? CPL_TRUE : CPL_FALSE;

            message = cpl_sprintf("|%s(%" CPL_SIZE_FORMAT ",%s, %s) - %s(%" 
                                  CPL_SIZE_FORMAT ",%s, %s)| = "
                                  "|%g - %g| = |%g| <= %g = %s",
                                  first_string, posx, rejstr1,
                                  cpl_type_get_name(type1), second_string,
                                  posx, rejstr2, cpl_type_get_name(type2),
                                  val1, val2, difval, tolerance, 
                                  tolerance_string);

        } else {
            cpl_error_set(cpl_func, cpl_error_get_code());

            expression = CPL_FALSE;

            message = cpl_sprintf("%s <=> %s (tol=%s) input error:",
                                  first_string, second_string,
                                  tolerance_string);
        }
    }

    cpl_test_one(errnopre, twallpre, flopspre, statepre, expression, message,
                 CPL_FALSE, function, file, line);
    if (!expression && cpl_errorstate_is_equal(mystate) &&
        cpl_msg_get_level() <= CPL_MSG_ERROR) {
        cpl_msg_warning(cpl_func, "Structure of the compared arrays:");
        cpl_array_dump_structure(first, stderr);
        cpl_array_dump_structure(second, stderr);
    }

    cpl_errorstate_set(mystate);

    cpl_array_delete(diff);
    cpl_free(message);

    return;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Test if two CPL images are identical
         within a given (absolute) tolerance
  @param errnopre         errno prior to expression evaluation
  @param flopspre         FLOP count prior to expression evaluation
  @param twallpre         Wall clock time prior to expression calculation
  @param statepre         CPL errorstate prior to expression evaluation
  @param first            The first image in the comparison
  @param first_string     The first image as a string
  @param second           The second image of identical size in the comparison
  @param second_string    The second image as a string
  @param tolerance        A non-negative tolerance
  @param tolerance_string The tolerance as a string
  @param function         function name
  @param file             filename
  @param line             line number
  @see   cpl_test_image_abs()
  @note  This function should only be called from the macro cpl_test_image_abs()
 */
/*----------------------------------------------------------------------------*/
void cpl_test_image_abs_macro(int errnopre, double twallpre, cpl_flops flopspre,
                              cpl_errorstate statepre,
                              const cpl_image * first,
                              const char *first_string,
                              const cpl_image * second,
                              const char *second_string,
                              double tolerance, const char *tolerance_string,
                              const char *function, const char *file,
                              unsigned line)
{
    cpl_errorstate mystate = cpl_errorstate_get();
    const char * stype1 = cpl_type_get_name(cpl_image_get_type(first));
    const char * stype2 = cpl_type_get_name(cpl_image_get_type(second));
    const cpl_size nbad1 = cpl_image_count_rejected(first);
    const cpl_size nbad2 = cpl_image_count_rejected(second);
    const cpl_size nx = cpl_image_get_size_x(first);
    const cpl_size ny = cpl_image_get_size_y(first);

    cpl_image  * cdiff   = cpl_image_subtract_create(first, second);
    cpl_image  * diff    = (cpl_image_get_type(cdiff) & CPL_TYPE_COMPLEX) ?
        cpl_image_extract_mod(cdiff) : cdiff;
    cpl_boolean  expression;
    char * message;

    if (tolerance < 0.0) {
        expression = CPL_FALSE;
        message = cpl_sprintf("image1=%s; image2=%s. Negative tolerance %s = "
                              "%g", first_string, second_string,
                              tolerance_string, tolerance);
    } else if (!cpl_errorstate_is_equal(mystate)) {
        cpl_error_set(cpl_func, cpl_error_get_code());
        expression = CPL_FALSE;

        message = cpl_sprintf("%s(%" CPL_SIZE_FORMAT ",%" CPL_SIZE_FORMAT
                              ", %s) <=> %s(%" CPL_SIZE_FORMAT ",%"
                              CPL_SIZE_FORMAT ", %s) (tol=%s) input error:",
                              first_string, nx, ny, stype1, second_string,
                              cpl_image_get_size_x(second),
                              cpl_image_get_size_y(second), stype2,
                              tolerance_string);
    } else if (nbad1 == nbad2 && nbad1 == nx * ny) {
        expression = CPL_TRUE;
        message = cpl_sprintf("%s(%" CPL_SIZE_FORMAT ",%" CPL_SIZE_FORMAT
                              ", %s) <=> %s(%" CPL_SIZE_FORMAT ",%"
                              CPL_SIZE_FORMAT ", %s) (tol=%s) All pixels "
                              "are bad",
                              first_string, nx, ny, stype1, second_string,
                              cpl_image_get_size_x(second),
                              cpl_image_get_size_y(second), stype2,
                              tolerance_string);
    } else if (cpl_image_count_rejected(diff) == nx * ny) {
        expression = CPL_FALSE;
        message = cpl_sprintf("%s(%" CPL_SIZE_FORMAT ",%" CPL_SIZE_FORMAT
                              ", %s) <=> %s(%" CPL_SIZE_FORMAT ",%"
                              CPL_SIZE_FORMAT ", %s) (tol=%s) All pixels "
                              "are bad in the first (%" CPL_SIZE_FORMAT
                              ") or second (%" CPL_SIZE_FORMAT ") image",
                              first_string, nx, ny, stype1,
                              second_string, cpl_image_get_size_x(second),
                              cpl_image_get_size_y(second), stype2,
                              tolerance_string, nbad1, nbad2);
    } else {

        cpl_stats istats;
        const cpl_error_code icode =
            cpl_stats_fill_from_image(&istats, diff, CPL_STATS_MIN
                                      | CPL_STATS_MAX
                                      | CPL_STATS_MINPOS
                                      | CPL_STATS_MAXPOS);
        const cpl_stats * stats = icode ? NULL : &istats;

        const double maxdif = cpl_stats_get_max(stats);
        const double mindif = cpl_stats_get_min(stats);

        const cpl_boolean is_pos = (maxdif >= -mindif) ? CPL_TRUE : CPL_FALSE;
        const double difval = is_pos ? maxdif : mindif;

        const cpl_size    posx
            = is_pos ? cpl_stats_get_max_x(stats) : cpl_stats_get_min_x(stats);
        const cpl_size    posy
            = is_pos ? cpl_stats_get_max_y(stats) : cpl_stats_get_min_y(stats);

        int          is_bad1;
        int          is_bad2;

        if (cpl_errorstate_is_equal(mystate)) {

            expression = (fabs(difval) <= tolerance) ? CPL_TRUE : CPL_FALSE;

            if (cpl_image_get_type(cdiff) & CPL_TYPE_COMPLEX) {
                const double complex val1 =
                    cpl_image_get_complex(first,  posx, posy, &is_bad1);
                const double complex val2 = 
                    (cpl_image_get_type(second) & CPL_TYPE_COMPLEX)
                    ? cpl_image_get_complex(second, posx, posy, &is_bad2)
                    : cpl_image_get(second, posx, posy, &is_bad2);

                const char * rejstr1 = is_bad1 ? " bad" : " not bad";
                const char * rejstr2 = is_bad2 ? " bad" : " not bad";

                message = cpl_sprintf("|%s(%" CPL_SIZE_FORMAT ",%"
                                      CPL_SIZE_FORMAT
                                      ",%s, %s) - %s(%" CPL_SIZE_FORMAT ",%"
                                      CPL_SIZE_FORMAT ",%s, %s)| = "
                                      "|%g - %g + I (%g - %g) | = |%g| "
                                      "<= %g = %s",
                                      first_string, posx, posy, rejstr1, stype1,
                                      second_string, posx, posy, rejstr2,
                                      stype2, creal(val1), creal(val2),
                                      cimag(val1), cimag(val2), difval,
                                      tolerance, tolerance_string);
            } else {
                const double val1 = cpl_image_get(first,  posx, posy, &is_bad1);
                const double val2 = cpl_image_get(second, posx, posy, &is_bad2);

                const char * rejstr1 = is_bad1 ? " bad" : " not bad";
                const char * rejstr2 = is_bad2 ? " bad" : " not bad";

                message = cpl_sprintf("|%s(%" CPL_SIZE_FORMAT ",%"
                                      CPL_SIZE_FORMAT
                                      ",%s, %s) - %s(%" CPL_SIZE_FORMAT ",%"
                                      CPL_SIZE_FORMAT ",%s, %s)| = "
                                      "|%g - %g| = |%g| <= %g = %s",
                                      first_string, posx, posy, rejstr1, stype1,
                                      second_string, posx, posy, rejstr2,
                                      stype2, val1, val2, difval, tolerance,
                                      tolerance_string);
            }

            if (!expression && cpl_msg_get_level() <= CPL_MSG_ERROR)
                cpl_stats_dump(stats, CPL_STATS_MIN
                               | CPL_STATS_MAX
                               | CPL_STATS_MINPOS
                               | CPL_STATS_MAXPOS, stderr);

        } else {
            cpl_error_set(cpl_func, cpl_error_get_code());

            expression = CPL_FALSE;

            message = cpl_sprintf("%s(%" CPL_SIZE_FORMAT ",%" CPL_SIZE_FORMAT
                                  ", %s) <=> %s(%" CPL_SIZE_FORMAT ",%"
                                  CPL_SIZE_FORMAT ", %s) (tol=%s) input error:",
                                  first_string, nx, ny, stype1,
                                  second_string, cpl_image_get_size_x(second),
                                  cpl_image_get_size_y(second), stype2,
                                  tolerance_string);
        }
    }

    cpl_test_one(errnopre, twallpre, flopspre, statepre, expression, message,
                 CPL_FALSE, function, file, line);
    if (!expression && cpl_errorstate_is_equal(mystate) &&
        cpl_msg_get_level() <= CPL_MSG_ERROR) {
        cpl_msg_warning(cpl_func, "Structure of the compared images:");
        cpl_image_dump_structure(first, stderr);
        cpl_image_dump_structure(second, stderr);
    }

    cpl_errorstate_set(mystate);

    cpl_image_delete(cdiff);
    if (diff != cdiff) cpl_image_delete(diff);
    cpl_free(message);

    return;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Test if two CPL images are identical
         within a given (relative) tolerance
  @param errnopre         errno prior to expression evaluation
  @param flopspre         FLOP count prior to expression evaluation
  @param twallpre         Wall clock time prior to expression calculation
  @param statepre         CPL errorstate prior to expression evaluation
  @param first            The first image in the comparison
  @param first_string     The first image as a string
  @param second           The second image of identical size in the comparison
  @param second_string    The second image as a string
  @param tolerance        A non-negative tolerance
  @param tolerance_string The tolerance as a string
  @param function         function name
  @param file             filename
  @param line             line number
  @see   cpl_test_image_rel()
  @note  This function should only be called from the macro cpl_test_image_rel()
 */
/*----------------------------------------------------------------------------*/
void cpl_test_image_rel_macro(int errnopre, double twallpre, cpl_flops flopspre,
                              cpl_errorstate statepre,
                              const cpl_image * first,
                              const char *first_string,
                              const cpl_image * second,
                              const char *second_string,
                              double tolerance, const char *tolerance_string,
                              const char *function, const char *file,
                              unsigned line)
{
    cpl_errorstate mystate = cpl_errorstate_get();
    const char * stype1 = cpl_type_get_name(cpl_image_get_type(first));
    const char * stype2 = cpl_type_get_name(cpl_image_get_type(second));
    const cpl_size nbad1 = cpl_image_count_rejected(first);
    const cpl_size nbad2 = cpl_image_count_rejected(second);
    const cpl_size nx = cpl_image_get_size_x(first);
    const cpl_size ny = cpl_image_get_size_y(first);

    cpl_image  * cdiff   = cpl_image_subtract_create(first, second);
    cpl_image  * diff    = (cpl_image_get_type(cdiff) & CPL_TYPE_COMPLEX) ?
        cpl_image_extract_mod(cdiff) : cdiff;
    cpl_boolean  expression;
    char * message;

    if (tolerance < 0.0) {
        expression = CPL_FALSE;
        message = cpl_sprintf("image1=%s; image2=%s. Negative tolerance %s = "
                              "%g", first_string, second_string,
                              tolerance_string, tolerance);
    } else if (!cpl_errorstate_is_equal(mystate)) {
        cpl_error_set(cpl_func, cpl_error_get_code());
        expression = CPL_FALSE;

        message = cpl_sprintf("%s(%" CPL_SIZE_FORMAT ",%" CPL_SIZE_FORMAT
                              ", %s) <=> %s(%" CPL_SIZE_FORMAT ",%"
                              CPL_SIZE_FORMAT ", %s) (tol=%s) input error:",
                              first_string, nx, ny, stype1, second_string,
                              cpl_image_get_size_x(second),
                              cpl_image_get_size_y(second), stype2,
                              tolerance_string);
    } else if (nbad1 == nbad2 && nbad1 == nx * ny) {
        expression = CPL_TRUE;
        message = cpl_sprintf("%s(%" CPL_SIZE_FORMAT ",%" CPL_SIZE_FORMAT
                              ", %s) <=> %s(%" CPL_SIZE_FORMAT ",%"
                              CPL_SIZE_FORMAT ", %s) (tol=%s) All pixels "
                              "are bad",
                              first_string, nx, ny, stype1, second_string,
                              cpl_image_get_size_x(second),
                              cpl_image_get_size_y(second), stype2,
                              tolerance_string);
    } else if (cpl_image_count_rejected(diff) == nx * ny) {
        expression = CPL_FALSE;
        message = cpl_sprintf("%s(%" CPL_SIZE_FORMAT ",%" CPL_SIZE_FORMAT
                              ", %s) <=> %s(%" CPL_SIZE_FORMAT ",%"
                              CPL_SIZE_FORMAT ", %s) (tol=%s) All pixels "
                              "are bad in the first (%" CPL_SIZE_FORMAT
                              ") or second (%" CPL_SIZE_FORMAT ") image",
                              first_string, nx, ny, stype1,
                              second_string, cpl_image_get_size_x(second),
                              cpl_image_get_size_y(second), stype2,
                              tolerance_string, nbad1, nbad2);
    } else {
        /* Create real-valued images with the absolute values */
        cpl_image * imabs1 = (cpl_image_get_type(first) & CPL_TYPE_COMPLEX) ?
            cpl_image_extract_mod(first) : cpl_image_abs_create(first);
        cpl_image * imabs2 = (cpl_image_get_type(second) & CPL_TYPE_COMPLEX) ?
            cpl_image_extract_mod(second) : cpl_image_abs_create(second);

        cpl_image * immin = cpl_image_min_create(imabs1, imabs2);

        cpl_error_code error = cpl_image_multiply_scalar(immin, tolerance);

        cpl_image * posit = cpl_image_subtract_create(immin, diff);

        cpl_stats istats;
        const cpl_error_code icode =
            cpl_stats_fill_from_image(&istats, posit, CPL_STATS_MIN
                                      | CPL_STATS_MINPOS);
        const cpl_stats * stats = icode ? NULL : &istats;

        const double difval = cpl_stats_get_min(stats);

        const cpl_size    posx = cpl_stats_get_min_x(stats);
        const cpl_size    posy = cpl_stats_get_min_y(stats);

        int          is_bad1;
        int          is_bad2;

        if (!error && cpl_errorstate_is_equal(mystate)) {
            const double mval = cpl_image_get(immin, posx, posy, &is_bad2);

            expression = difval >= 0.0 ? CPL_TRUE : CPL_FALSE;

            if (cpl_image_get_type(cdiff) & CPL_TYPE_COMPLEX) {
                const double complex val1 =
                    cpl_image_get_complex(first,  posx, posy, &is_bad1);
                const double complex val2 = 
                    (cpl_image_get_type(second) & CPL_TYPE_COMPLEX)
                    ? cpl_image_get_complex(second, posx, posy, &is_bad2)
                    : cpl_image_get(second, posx, posy, &is_bad2);

                const char * rejstr1 = is_bad1 ? " bad" : " not bad";
                const char * rejstr2 = is_bad2 ? " bad" : " not bad";

                message = cpl_sprintf("|%s(%" CPL_SIZE_FORMAT ",%"
                                      CPL_SIZE_FORMAT
                                      ",%s, %s) - %s(%" CPL_SIZE_FORMAT ",%"
                                      CPL_SIZE_FORMAT ",%s, %s)| = "
                                      "|%g - %g + I (%g - %g) | = |%g + I %g| "
                                      "<= %g",
                                      first_string, posx, posy, rejstr1, stype1,
                                      second_string, posx, posy, rejstr2,
                                      stype2, creal(val1), creal(val2),
                                      cimag(val1), cimag(val2),
                                      creal(val1) - creal(val2),
                                      cimag(val1) - cimag(val2), mval);
            } else {
                const double val1 = cpl_image_get(first,  posx, posy, &is_bad1);
                const double val2 = cpl_image_get(second, posx, posy, &is_bad2);

                const char * rejstr1 = is_bad1 ? " bad" : " not bad";
                const char * rejstr2 = is_bad2 ? " bad" : " not bad";

                message = cpl_sprintf("|%s(%" CPL_SIZE_FORMAT ",%"
                                      CPL_SIZE_FORMAT
                                      ",%s, %s) - %s(%" CPL_SIZE_FORMAT ",%"
                                      CPL_SIZE_FORMAT ",%s, %s)| = "
                                      "|%g - %g| = |%g| <= %g",
                                      first_string, posx, posy, rejstr1, stype1,
                                      second_string, posx, posy, rejstr2,
                                      stype2, val1, val2, val1 - val2, mval);
            }

            if (!expression && cpl_msg_get_level() <= CPL_MSG_ERROR)
                cpl_stats_dump(stats, CPL_STATS_MIN
                               | CPL_STATS_MINPOS, stderr);

        } else {
            cpl_error_set(cpl_func, cpl_error_get_code());

            expression = CPL_FALSE;

            message = cpl_sprintf("%s(%" CPL_SIZE_FORMAT ",%" CPL_SIZE_FORMAT
                                  ", %s) <=> %s(%" CPL_SIZE_FORMAT ",%"
                                  CPL_SIZE_FORMAT ", %s) (tol=%s) input error:",
                                  first_string, nx, ny, stype1,
                                  second_string, cpl_image_get_size_x(second),
                                  cpl_image_get_size_y(second), stype2,
                                  tolerance_string);
        }
        cpl_image_delete(imabs1);
        cpl_image_delete(imabs2);
        cpl_image_delete(immin);
        cpl_image_delete(posit);

    }

    cpl_test_one(errnopre, twallpre, flopspre, statepre, expression, message,
                 CPL_FALSE, function, file, line);
    if (!expression && cpl_errorstate_is_equal(mystate)) {
        cpl_msg_warning(cpl_func, "Structure of the compared images:");
        if (cpl_msg_get_level() <= CPL_MSG_ERROR) {
            cpl_image_dump_structure(first, stderr);
            cpl_image_dump_structure(second, stderr);
        }
    }

    cpl_errorstate_set(mystate);

    cpl_image_delete(cdiff);
    if (diff != cdiff) cpl_image_delete(diff);
    cpl_free(message);

    return;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Test if two CPL imagelists are identical
         within a given (absolute) tolerance
  @param errnopre         errno prior to expression evaluation
  @param flopspre         FLOP count prior to expression evaluation
  @param twallpre         Wall clock time prior to expression calculation
  @param statepre         CPL errorstate prior to expression evaluation
  @param first            The first imagelist in the comparison
  @param first_string     The first imagelist as a string
  @param second           The second list of identical size in the comparison
  @param second_string    The second imagelist as a string
  @param tolerance        A non-negative tolerance
  @param tolerance_string The tolerance as a string
  @param function         function name
  @param file             filename
  @param line             line number
  @see   cpl_test_imagelist_abs()
  @note  This function should only be called from cpl_test_imagelist_abs()
 */
/*----------------------------------------------------------------------------*/
void cpl_test_imagelist_abs_macro(int errnopre, double twallpre,
                                  cpl_flops flopspre, cpl_errorstate statepre,
                                  const cpl_imagelist * first,
                                  const char *first_string,
                                  const cpl_imagelist * second,
                                  const char *second_string,
                                  double tolerance,
                                  const char *tolerance_string,
                                  const char *function, const char *file,
                                  unsigned line)
{
    cpl_errorstate mystate = cpl_errorstate_get();
    const cpl_size sz1 = cpl_imagelist_get_size(first);
    const cpl_size sz2 = cpl_imagelist_get_size(second);
    cpl_boolean  expression;
    char * message = NULL;


    if (!cpl_errorstate_is_equal(mystate)) {
        cpl_error_set(cpl_func, cpl_error_get_code());
        expression = CPL_FALSE;

        message = cpl_sprintf("%s <=> %s (tol=%s) input error:",
                              first_string, second_string,
                              tolerance_string);
    } else if (sz1 != sz2) {
        expression = CPL_FALSE;
        message = cpl_sprintf("%s <=> %s (tol=%s) imagelist list sizes differ: "
                              "%" CPL_SIZE_FORMAT " <=> %" CPL_SIZE_FORMAT,
                              first_string, second_string,
                              tolerance_string, sz1, sz2);
    } else {
        const cpl_size failures = cpl_test_failures;
        cpl_size i;

        message = cpl_sprintf("|%s(%" CPL_SIZE_FORMAT ") - %s(%" CPL_SIZE_FORMAT
                              ")| <= %g = %s", first_string, sz1,
                              second_string, sz2,
                              tolerance, tolerance_string);

        for (i = 0; i < sz1; i++) {
            const cpl_image * img1 = cpl_imagelist_get_const(first,  i);
            const cpl_image * img2 = cpl_imagelist_get_const(second, i);

            char * img1string = cpl_sprintf("image %" CPL_SIZE_FORMAT
                                            " in first list",  1+i);
            char * img2string = cpl_sprintf("image %" CPL_SIZE_FORMAT
                                            " in second list", 1+i);

            cpl_test_image_abs_macro(errnopre, twallpre, flopspre, statepre,
                                     img1, img1string, img2, img2string,
                                     tolerance, tolerance_string, function,
                                     file, line);
            cpl_free(img1string);
            cpl_free(img2string);
        }

        expression = failures == cpl_test_failures ? CPL_TRUE : CPL_FALSE;
        cpl_test_failures = failures; /* Count as only one test ! */

    }

    cpl_test_one(errnopre, twallpre, flopspre, statepre, expression, message,
                 CPL_FALSE, function, file, line);
    if (!expression && cpl_errorstate_is_equal(mystate)) {
        cpl_msg_warning(cpl_func, "Structure of the compared imagelists:");
        if (cpl_msg_get_level() <= CPL_MSG_ERROR) {
            cpl_imagelist_dump_structure(first, stderr);
            cpl_imagelist_dump_structure(second, stderr);
        }
    }

    cpl_errorstate_set(mystate);
    cpl_free(message);

    return;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Test if two CPL polynomials are identical
         within a given (absolute) tolerance
  @param errnopre         errno prior to expression evaluation
  @param flopspre         FLOP count prior to expression evaluation
  @param twallpre         Wall clock time prior to expression calculation
  @param statepre         CPL errorstate prior to expression evaluation
  @param first            The first polynomial in the comparison
  @param first_string     The first polynomial as a string
  @param second           The second polynomial in the comparison
  @param second_string    The second polynomial as a string
  @param tolerance        A non-negative tolerance
  @param tolerance_string The tolerance as a string
  @param function         function name
  @param file             filename
  @param line             line number
  @see   cpl_test_polynomial_abs()
  @note  This function should only be called from the macro
         cpl_test_polynomial_abs()
 */
/*----------------------------------------------------------------------------*/
void cpl_test_polynomial_abs_macro(int errnopre, double twallpre,
                                   cpl_flops flopspre, cpl_errorstate statepre,
                                   const cpl_polynomial * first,
                                   const char *first_string,
                                   const cpl_polynomial * second,
                                   const char *second_string,
                                   double tolerance,
                                   const char *tolerance_string,
                                   const char *function, const char *file,
                                   unsigned line)
{
    cpl_errorstate mystate = cpl_errorstate_get();
    const int retval = cpl_polynomial_compare(first, second, tolerance);
    cpl_boolean  expression = retval ? CPL_FALSE : CPL_TRUE;
    char * message;

    if (cpl_errorstate_is_equal(mystate)) {
        const cpl_size dim1 = cpl_polynomial_get_dimension(first);
        const cpl_size dim2 = cpl_polynomial_get_dimension(second);
        cpl_error_set(cpl_func, cpl_error_get_code());

        message = cpl_sprintf("(dimension %d <=> %d. intol degree=%d) |%s - %s| "
                              "<= %g = %s", (int)dim1, (int)dim2,
                              retval ? retval - 1 : 0,
                              first_string, second_string,
                              tolerance, tolerance_string);

    } else {
        expression = CPL_FALSE;

        message = cpl_sprintf("%s <=> %s (tol=%s) input error:",
                              first_string, second_string,
                              tolerance_string);
    }

    cpl_test_one(errnopre, twallpre, flopspre, statepre, expression, message,
                 CPL_FALSE, function, file, line);

    if (retval > 0) {
        cpl_polynomial_dump(first,  stderr);
        cpl_polynomial_dump(second, stderr);
    }

    cpl_errorstate_set(mystate);

    cpl_free(message);

    return;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Test and reset the CPL error code
  @param errnopre          errno prior to expression evaluation
  @param flopspre          FLOP count prior to expression evaluation
  @param twallpre          Wall clock time prior to expression calculation
  @param statepre          CPL errorstate prior to expression evaluation
  @param errorstate        The expected CPL error code (incl. CPL_ERROR_NONE)
  @param errorstate_string The CPL error code as a string
  @param function          cpl_func
  @param file              __FILE__
  @param line              __LINE__
  @see   cpl_test_errorstate
  @note  This function should only be called from the macro
         cpl_test_errorstate()
 */
/*----------------------------------------------------------------------------*/
void cpl_test_errorstate_macro(int errnopre, double twallpre,
                               cpl_flops flopspre, cpl_errorstate statepre,
                               cpl_errorstate errorstate,
                               const char * errorstate_string,
                               const char * function, const char * file,
                               unsigned line)
{

    /* FIXME: Improve message */
    char * message = cpl_sprintf("%s <=> %d (%s)",
                                 errorstate_string,
                                 cpl_error_get_code(),
                                 cpl_error_get_message());

    cpl_test_one(errnopre, twallpre, flopspre, statepre,
                 cpl_errorstate_is_equal(errorstate) ? CPL_TRUE : CPL_FALSE,
                 message, CPL_TRUE, function, file, line);

    cpl_free(message);

    cpl_test_reset(errorstate);

    return;
}



/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Test and reset the CPL error code
  @param errnopre     errno prior to expression evaluation
  @param flopspre     FLOP count prior to expression evaluation
  @param twallpre     Wall clock time prior to expression calculation
  @param statepre     CPL errorstate prior to expression evaluation
  @param error        The expected CPL error code (incl. CPL_ERROR_NONE)
  @param error_string The CPL error code as a string
  @param function     cpl_func
  @param file         __FILE__
  @param line         __LINE__
  @see   cpl_test_error
  @note  This function should only be called from the macro cpl_test_error()

 */
/*----------------------------------------------------------------------------*/
void cpl_test_error_macro(int errnopre, double twallpre, cpl_flops flopspre,
                          cpl_errorstate statepre,
                          cpl_error_code error, const char * error_string,
                          const char * function, const char * file,
                          unsigned line)
{

    char * message = cpl_sprintf("(%s) = %d (%s) <=> %d (%s)",
                                 error_string, error,
                                 cpl_error_get_message_default(error),
                                 cpl_error_get_code(),
                                 cpl_error_get_message());

    cpl_test_one(errnopre, twallpre, flopspre, statepre,
                 cpl_error_get_code() == error ? CPL_TRUE : CPL_FALSE,
                 message, CPL_TRUE, function, file, line);

    cpl_free(message);

    cpl_test_reset(cleanstate);

    return;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Test if two CPL error expressions are equal, also to the CPL error code
  @param errnopre      errno prior to expression evaluation
  @param flopspre      FLOP count prior to expression evaluation
  @param twallpre      Wall clock time prior to expression calculation
  @param statepre      CPL errorstate prior to expression evaluation
  @param first         The first value in the comparison
  @param first_string  The first value as a string
  @param second        The second value in the comparison
  @param second_string The second value as a string
  @param function      cpl_func
  @param file          __FILE__
  @param line          __LINE__
  @see   cpl_test_error
  @note  This function should only be called from the macro cpl_test_eq_error()

 */
/*----------------------------------------------------------------------------*/
void cpl_test_eq_error_macro(int errnopre, double twallpre, cpl_flops flopspre,
                             cpl_errorstate statepre,
                             cpl_error_code first,  const char * first_string,
                             cpl_error_code second, const char * second_string,
                             const char * function, const char * file,
                             unsigned line)


{

    char * message = cpl_sprintf("(%s) = %d (%s) <=> (%s) = %d (%s) "
                                 "<=> %d (%s)", first_string, first,
                                 cpl_error_get_message_default(first),
                                 second_string, second,
                                 cpl_error_get_message_default(second),
                                 cpl_error_get_code(),
                                 cpl_error_get_message_default
                                 (cpl_error_get_code()));

    cpl_test_one(errnopre, twallpre, flopspre, statepre,
                 (first == second && cpl_error_get_code() == first)
                 ? CPL_TRUE : CPL_FALSE,
                 message, CPL_TRUE, function, file, line);

    cpl_free(message);

    cpl_test_reset(cleanstate);

    return;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Test if the memory system is empty
  @param errnopre errno prior to expression evaluation
  @param flopspre FLOP count prior to expression evaluation
  @param twallpre Wall clock time prior to expression calculation
  @param statepre CPL errorstate prior to expression evaluation
  @param function cpl_func
  @param file     __FILE__
  @param line     __LINE__
  @see   cpl_test_memory_is_empty
  @note  This function should only be called from the macro
         cpl_test_memory_is_empty()

 */
/*----------------------------------------------------------------------------*/
void cpl_test_memory_is_empty_macro(int errnopre, double twallpre,
                                    cpl_flops flopspre, cpl_errorstate statepre,
                                    const char * function, const char * file,
                                    unsigned line)
{
    const char * message;
    cpl_boolean ok;

    if (cpl_memory_is_empty() == -1) {
        message = "CPL memory system is empty (not testable)";
        ok = CPL_TRUE;
    } else {
        message = "CPL memory system is empty";
        ok = cpl_memory_is_empty() == 0 ? CPL_FALSE : CPL_TRUE;
    }

    cpl_test_one(errnopre, twallpre, flopspre, statepre, ok, message, CPL_FALSE,
                 function, file, line);

    if (!ok) {
        cpl_msg_indent_more();
        cpl_memory_dump();
        cpl_msg_indent_less();
    }

    return;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Finalize CPL and unit-testing environment and report any failures
  @param    nfail The number of failures counted apart from cpl_test() et al.
  @return   @em EXIT_SUCCESS iff the CPL errorstate is clean
  @note This function should be used for the final return from a unit test
  @see cpl_test_init()

  nfail should normally be zero, but may be set to a positive number when it
  is necessary to ensure a failure.
  nfail should only be negative in the unit test of the unit-test functions
  themselves.

  Example of usage:
    @code

    int main (void)
    {

        cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

        cpl_test(myfunc(&p));
        cpl_test(p != NULL);

        return cpl_test_end(0);
    }
   
    @endcode

 */
/*----------------------------------------------------------------------------*/
int cpl_test_end(cpl_size nfail)
{

    const int       errnopre = errno;
    const cpl_flops nflops   = cpl_tools_get_flops();
    cpl_boolean     ok       = CPL_TRUE;
    const cpl_size  mfail    = nfail + (cpl_size)cpl_test_failures;

    const double cpl_test_elap = cpl_test_get_walltime() - cpl_test_time_start;

#if defined HAVE_SYS_TIMES_H && defined _SC_CLK_TCK && defined HAVE_SYSCONF
    struct tms  buf;

    const clock_t clocks = times(&buf);

    const double  cputime   =(double)buf.tms_utime
        / (double)sysconf(_SC_CLK_TCK);
    const double  systime   =(double)buf.tms_stime
        / (double)sysconf(_SC_CLK_TCK);
    const double  chcputime =(double)buf.tms_cutime
        / (double)sysconf(_SC_CLK_TCK);
    const double  chsystime =(double)buf.tms_cstime
        / (double)sysconf(_SC_CLK_TCK);

    errno = 0;
    cpl_msg_debug(cpl_func, "Sizeof(clock_t): %u", (unsigned)sizeof(clocks));
    cpl_msg_debug(cpl_func, "sysconf(_SC_CLK_TCK): %u",
                  (unsigned)sysconf(_SC_CLK_TCK));
    cpl_msg_info(cpl_func,  "User   time to test [s]: %g",       cputime);
    cpl_msg_info(cpl_func,  "System time to test [s]: %g",       systime);
    cpl_msg_debug(cpl_func, "Child   user time to test [s]: %g", chcputime);
    cpl_msg_debug(cpl_func, "Child system time to test [s]: %g", chsystime);

#else
    errno = 0;
#endif

    if (cpl_test_state_ == 0) {
        cpl_msg_error(cpl_func, "Missing a previous call to cpl_test_init()");
        ok = CPL_FALSE;
    } else if (cpl_test_state_ == 2) {
        cpl_msg_error(cpl_func, "Repeated call to cpl_test_end()");
        ok = CPL_FALSE;
    }

    /* Need to close files here, to deallocate */
    cpl_test_zero(cpl_io_fits_end());

    if (cpl_test_elap > 0.0) {
        cpl_msg_info(cpl_func, "Actual time to test [s]: %g",
                     cpl_test_elap);
        cpl_msg_info(cpl_func, "The computational speed during this test "
                     "[MFLOP/s]: %g", 1e-6*(double)nflops/cpl_test_elap);
    } else {
        cpl_msg_info(cpl_func, "Number of MFLOPs in this test: %g",
                     1e-6*(double)nflops);
    }

    if (errnopre != 0) {
        cpl_msg_warning(cpl_func, "%s() was called with errno=%d: %s",
                        cpl_func, errnopre, strerror(errnopre));
    }

    /* Make sure that the failure is written */
    if (cpl_msg_get_level() == CPL_MSG_OFF) cpl_msg_set_level(CPL_MSG_ERROR);

    if (cpl_error_get_code() != CPL_ERROR_NONE) {
        ok = CPL_FALSE;
        cpl_msg_error(cpl_func, "The CPL errorstate was set by the unit "
                      "test(s)");
        cpl_msg_indent_more();
        cpl_errorstate_dump(cleanstate, CPL_FALSE, NULL);
        cpl_msg_indent_less();
    }

    if (mfail > 0) {
        ok = CPL_FALSE;
        cpl_msg_error(cpl_func, "%" CPL_SIZE_FORMAT " of %" CPL_SIZE_FORMAT
                      " test(s) failed", mfail, cpl_test_count);
    } else if (mfail < 0) {
        ok = CPL_FALSE;
        /* This special case is only foreseen to be reached by
           the unit test of the CPL unit test module */
        cpl_msg_error(cpl_func, "%" CPL_SIZE_FORMAT " of %" CPL_SIZE_FORMAT
                      " test(s) failed, %" CPL_SIZE_FORMAT " less than the "
                      "expected %" CPL_SIZE_FORMAT " failure(s)",
                      cpl_test_failures, cpl_test_count, -mfail, -nfail);
    } else {
        cpl_msg_info(cpl_func, "All %" CPL_SIZE_FORMAT " test(s) succeeded",
                     cpl_test_count);
    }

    if (!cpl_memory_is_empty()) {
        ok = CPL_FALSE;
        cpl_msg_error(cpl_func, "Memory leak detected:");
        cpl_msg_indent_more();
        cpl_memory_dump();
        cpl_msg_indent_less();
    } else if (cpl_msg_get_level() <= CPL_MSG_DEBUG) {
        cpl_memory_dump();
    }

    if (!ok) {
        cpl_msg_error(cpl_func, "This failure may indicate a bug in the tested "
                      "code");
        cpl_msg_error(cpl_func, "You can contribute to the improvement of the "
                      "software by emailing the logfile '%s' and the configure "
                      "logfile 'config.log' to %s", cpl_msg_get_log_name(),
                      cpl_test_report ? cpl_test_report : PACKAGE_BUGREPORT);
        cpl_msg_error(cpl_func, "System specifics:\n%s",
                      cpl_test_get_description());
    }


    cpl_test_dump_status();

    if (errno != 0) {
        cpl_msg_warning(cpl_func, "%s() set errno=%d: %s", cpl_func, errno,
                        strerror(errno));
        errno = 0;
    }

    cpl_test_state_ = 2;

    cpl_end();

    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

/**@}*/

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Dump the CPL errorstate and reset it
  @param    self  The errorstate to reset to
  @return   void

 */
/*----------------------------------------------------------------------------*/
static void cpl_test_reset(cpl_errorstate self)
{
    if (!cpl_errorstate_is_equal(self)) {
        cpl_errorstate_dump(self, CPL_FALSE, cpl_errorstate_dump_debug);
        cpl_errorstate_set(self);
    }
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Test an expression and update an internal counter if it fails
  @param errnopre   errno prior to expression evaluation
  @param twallpre   Wall clock time prior to expression calculation
  @param flopspre   FLOP count prior to expression evaluation
  @param twallpre   Wall clock time prior to expression calculation
  @param statepre   CPL errorstate prior to expression evaluation
  @param expression The expression to test (CPL_FALSE means failure)
  @param message    The text message associated with the expression
  @param function   function name
  @param file       filename
  @param line       line number

  The CPL FLOP count, the CPL errorstate and errno may have changed prior to
  or during to the expression evaluation.

    Success or failure,
    any CPL errorstate change prior to expression evaluation,
    any CPL errorstate change during expression evaluation,
    any errno change prior to expression evaluation,
    any errno change during expression evaluation.

 */
/*----------------------------------------------------------------------------*/
static void cpl_test_one(int errnopre, double twallpre, cpl_flops flopspre,
                         cpl_errorstate statepre, cpl_boolean expression,
                         const char *message, cpl_boolean expect_error,
                         const char *function, const char *file, unsigned line)
{

    const int myerrno = errno; /* Local copy, in case errno changes in here */
    char * errnopre_txt = errnopre == 0 ? NULL :
        cpl_sprintf(" Prior to this test errno=%d: %s.", errnopre,
                    strerror(errnopre));
    char * myerrno_txt = myerrno == errnopre ? NULL :
        cpl_sprintf(" This test set errno=%d: %s.", myerrno,
                    strerror(myerrno));
    const char * errnopre_msg = errnopre_txt == NULL ? "" : errnopre_txt;
    const char * myerrno_msg  = myerrno_txt  == NULL ? "" : myerrno_txt;
    const char * error_msg = cpl_errorstate_is_equal(cleanstate) ? "" :
        (expect_error ?
         (cpl_errorstate_is_equal(statepre) ? ""
          : "CPL error(s) set during this test.") :
         (cpl_errorstate_is_equal(statepre) ?
          " CPL error(s) set prior to this test." :
          (statepre == cleanstate ? " CPL error(s) set during this test." :
           " CPL error(s) set prior to and during this test.")));
    char * flopprev_txt = NULL;
    char * floptest_txt = NULL;

    assert(message  != NULL);
    assert(function != NULL);
    assert(file     != NULL);
    
    errno = 0;

#ifdef _OPENMP
#pragma omp atomic
#endif
        cpl_test_count++;

#ifdef _OPENMP
#pragma omp master
#endif
    {
        /* If two cpl_tests would be permitted to enter concurrently here,
           then the reported FLOP rate would be meaningless */

        const double cpl_test_time_now  = cpl_test_get_walltime();
        const double cpl_test_time_prev = cpl_test_time_one;
        const cpl_flops cpl_test_flops_now = cpl_tools_get_flops();
        const cpl_flops cpl_test_flops_prev = cpl_test_flops_one;

        cpl_test_time_one = cpl_test_time_now;
        cpl_test_flops_one = cpl_test_flops_now;

        if (flopspre > cpl_test_flops_prev) {
            const double cpl_test_time_between = twallpre - cpl_test_time_prev;
            const cpl_flops cpl_test_flops_between = flopspre
                - cpl_test_flops_prev;

            if (cpl_test_time_between > 0.0) {
                flopprev_txt = cpl_sprintf(" (%g FLOPs after the previous "
                                           "test and prior to this one at "
                                           "[MFLOP/s]: %g).",
                                           (double)cpl_test_flops_between,
                                           1e-6*(double)cpl_test_flops_between
                                           /cpl_test_time_between);
            } else {
                flopprev_txt = cpl_sprintf(" (%g FLOPs after the previous test "
                                           "and prior to this one).",
                                           (double)cpl_test_flops_between);
            }
        }

        if (cpl_test_flops_now > flopspre) {
            const double cpl_test_time_during = cpl_test_time_now - twallpre;
            const cpl_flops cpl_test_flops_during = cpl_test_flops_now
                - flopspre;

            if (cpl_test_time_during > 0.0) {
                floptest_txt = cpl_sprintf(" (%g FLOPs during this test at "
                                           "[MFLOP/s]: %g).",
                                           (double)cpl_test_flops_during,
                                           1e-6*(double)cpl_test_flops_during
                                           /cpl_test_time_during);

            } else {
                floptest_txt = cpl_sprintf(" (%g FLOPs during this test).",
                                           (double)cpl_test_flops_during);
            }
        }
    }

    if (flopprev_txt == NULL) flopprev_txt = cpl_strdup("");
    if (floptest_txt == NULL) floptest_txt = cpl_strdup("");

    if (cpl_test_state_ == 0) {
        cpl_msg_error(cpl_func, "Missing a previous call to cpl_test_init(): "
                      "Failure regardless of test");
#ifdef _OPENMP
#pragma omp atomic
#endif
        cpl_test_failures++;
    } else if (cpl_test_state_ == 2) {
        cpl_msg_error(cpl_func, "Test after call to cpl_test_end(): "
                      "Failure regardless of test");
#ifdef _OPENMP
#pragma omp atomic
#endif
        cpl_test_failures++;
    } else if (expression) {
        cpl_boolean has_error = error_msg[0] ? CPL_TRUE : CPL_FALSE;
        (has_error ? cpl_msg_info : cpl_msg_debug)
            (function, "Test %" CPL_SIZE_FORMAT " OK at %s:%u: %s.%s%s%s%s%s",
             cpl_test_count, file, line, message,
             error_msg, errnopre_msg, myerrno_msg, flopprev_txt, floptest_txt);
        cpl_errorstate_dump(cleanstate, CPL_FALSE, has_error
                            ? cpl_errorstate_dump_info
                            : cpl_errorstate_dump_debug);
    } else {
        cpl_msg_error(function, "Test %" CPL_SIZE_FORMAT " failed at %s:%u: "
                      "%s.%s%s%s%s%s", cpl_test_count, file, line, message,
                      error_msg, errnopre_msg, myerrno_msg, flopprev_txt,
                      floptest_txt);
        cpl_errorstate_dump(cleanstate, CPL_FALSE, NULL);
#ifdef _OPENMP
#pragma omp atomic
#endif
        cpl_test_failures++;
    }

    cpl_free(errnopre_txt);
    cpl_free(myerrno_txt);
    cpl_free(flopprev_txt);
    cpl_free(floptest_txt);

    if (errno != 0) {
        cpl_msg_debug(cpl_func, "%s() set errno=%d: %s", cpl_func, errno,
                      strerror(errno));
        errno = 0;
    }

    return;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Dump a single CPL error at debug messaging level
  @param    self      The number of the current error to be dumped
  @param    first     The number of the first error to be dumped
  @param    last      The number of the last error to be dumped
  @return   void
  @see cpl_errorstate_dump_one

 */
/*----------------------------------------------------------------------------*/
static void cpl_errorstate_dump_debug(unsigned self, unsigned first,
                                      unsigned last)
{

    const cpl_boolean is_reverse = first > last ? CPL_TRUE : CPL_FALSE;
    const unsigned    newest     = is_reverse ? first : last;
    const unsigned    oldest     = is_reverse ? last : first;
    const char      * revmsg     = is_reverse ? " in reverse order" : "";


    assert( oldest <= self );
    assert( newest >= self );

    if (newest == 0) {
        cpl_msg_debug(cpl_func, "No error(s) to dump");
        assert( oldest == 0);
    } else {
        assert( oldest > 0);
        assert( newest >= oldest);
        if (self == first) {
            if (oldest == 1) {
                cpl_msg_debug(cpl_func, "Dumping all %u error(s)%s:", newest,
                              revmsg);
            } else {
                cpl_msg_debug(cpl_func, "Dumping the %u most recent error(s) "
                              "out of a total of %u errors%s:",
                              newest - oldest + 1, newest, revmsg);
            }
            cpl_msg_indent_more();
        }

        cpl_msg_debug(cpl_func, "[%u/%u] '%s' (%u) at %s", self, newest,
                      cpl_error_get_message(), cpl_error_get_code(),
                      cpl_error_get_where());

        if (self == last) cpl_msg_indent_less();
    }
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Dump a single CPL error at debug messaging level
  @param    self      The number of the current error to be dumped
  @param    first     The number of the first error to be dumped
  @param    last      The number of the last error to be dumped
  @return   void
  @see cpl_errorstate_dump_debug

 */
/*----------------------------------------------------------------------------*/
static void cpl_errorstate_dump_info(unsigned self, unsigned first,
                                     unsigned last)
{

    const cpl_boolean is_reverse = first > last ? CPL_TRUE : CPL_FALSE;
    const unsigned    newest     = is_reverse ? first : last;
    const unsigned    oldest     = is_reverse ? last : first;
    const char      * revmsg     = is_reverse ? " in reverse order" : "";


    assert( oldest <= self );
    assert( newest >= self );

    if (newest == 0) {
        cpl_msg_info(cpl_func, "No error(s) to dump");
        assert( oldest == 0);
    } else {
        assert( oldest > 0);
        assert( newest >= oldest);
        if (self == first) {
            if (oldest == 1) {
                cpl_msg_info(cpl_func, "Dumping all %u error(s)%s:", newest,
                             revmsg);
            } else {
                cpl_msg_info(cpl_func, "Dumping the %u most recent error(s) "
                             "out of a total of %u errors%s:",
                             newest - oldest + 1, newest, revmsg);
            }
            cpl_msg_indent_more();
        }

        cpl_msg_info(cpl_func, "[%u/%u] '%s' (%u) at %s", self, newest,
                     cpl_error_get_message(), cpl_error_get_code(),
                     cpl_error_get_where());

        if (self == last) cpl_msg_indent_less();
    }
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    A string useful in error-reporting
  @return   Pointer to a string literal useful in error-reporting

 */
/*----------------------------------------------------------------------------*/
static const char * cpl_test_get_description(void)
{
    return "CPL version: " PACKAGE_VERSION
#if defined CPL_SIZE_BITS && CPL_SIZE_BITS == 32
        " (32-bit cpl_size)"
#else
        " (64-bit cpl_size)"
#endif
        "\n"
#ifdef CFITSIO_VERSION
        "CFITSIO version: " CPL_STRINGIFY(CFITSIO_VERSION) "\n"
#elif defined _FITSIO_H
        "CFITSIO version is less than 3.0\n"
#endif
#if defined WCSLIB_VERSION
        "WCSLIB version: " CPL_STRINGIFY(WCSLIB_VERSION) "\n"
#elif defined CPL_WCS_INSTALLED && CPL_WCS_INSTALLED == 1
        "WCSLIB available\n"
#else
        "WCSLIB unavailable\n"
#endif

#if defined CPL_FFTW_INSTALLED && defined CPL_FFTWF_INSTALLED
#if defined CPL_FFTW_VERSION && defined CPL_FFTWF_VERSION
        "FFTW (normal precision) version: " CPL_FFTW_VERSION "\n"
        "FFTW (single precision) version: " CPL_FFTWF_VERSION "\n"
#else
        "FFTW (normal and single precision) available\n"
#endif
#elif defined CPL_FFTW_INSTALLED
#if defined CPL_FFTW_VERSION
        "FFTW (normal precision) version: " CPL_FFTW_VERSION "\n"
        "FFTW (single precision) unavailable\n"
#else
        "FFTW (normal precision) available\n"
        "FFTW (single precision) unavailable\n"
#endif
#elif defined CPL_FFTWF_INSTALLED
#if defined CPL_FFTWF_VERSION
        "FFTW (normal precision) unavailable\n"
        "FFTW (single precision) version: " CPL_FFTWF_VERSION "\n"
#else
        "FFTW (normal precision) unavailable\n"
        "FFTW (single precision) available\n"
#endif
#else
       "FFTW unavailable\n"
#endif

#ifdef CPL_ADD_FLOPS
       "CPL FLOP counting is available\n"
#else
       "CPL FLOP counting is unavailable, enable with -DCPL_ADD_FLOPS\n"
#endif

#ifdef _OPENMP
        CPL_XSTRINGIFY(_OPENMP) ": " CPL_STRINGIFY(_OPENMP) "\n"
#endif

#ifdef SIZEOF_SIZE_T
        "SIZEOF_SIZE_T is defined as " CPL_STRINGIFY(SIZEOF_SIZE_T) "\n"
#else
        "SIZEOF_SIZE_T is not defined\n"
#endif

#ifdef OFF_T
        "OFF_T is defined as " CPL_STRINGIFY(OFF_T) "\n"
#else
        "OFF_T is not defined\n"
#endif

#if defined WORDS_BIGENDIAN && WORDS_BIGENDIAN == 1
        "This platform is big-endian\n"
#else
        "This platform is not big-endian\n"
#endif

#ifdef __DATE__
        "Compile date: " __DATE__ "\n"
#endif
#ifdef __TIME__
        "Compile time: " __TIME__ "\n"
#endif
#ifdef __STDC__
        CPL_XSTRINGIFY(__STDC__) ": " CPL_STRINGIFY(__STDC__) "\n"
#endif
#ifdef __STDC_VERSION__
        CPL_XSTRINGIFY(__STDC_VERSION__) ": "
        CPL_STRINGIFY(__STDC_VERSION__) "\n"
#endif
#ifdef __STDC_HOSTED__
        CPL_XSTRINGIFY(__STDC_HOSTED__) ": " CPL_STRINGIFY(__STDC_HOSTED__) "\n"
#endif
#ifdef __STDC_IEC_559__
        CPL_XSTRINGIFY(__STDC_IEC_559__) ": "
        CPL_STRINGIFY(__STDC_IEC_559__) "\n"
#endif
#ifdef __STDC_IEC_559_COMPLEX__
        CPL_XSTRINGIFY(__STDC_IEC_559_COMPLEX__) ": "
        CPL_STRINGIFY(__STDC_IEC_559_COMPLEX__) "\n"
#endif
#ifdef __STRICT_ANSI__
        /* gcc and Sun Studio 12.1 supports this */
        CPL_XSTRINGIFY(__STRICT_ANSI__) ": " CPL_STRINGIFY(__STRICT_ANSI__) "\n"
#endif
#ifdef __GNUC__
        "gcc version (major number): " CPL_STRINGIFY(__GNUC__) "\n"
#ifdef __GNUC_MINOR__
        "gcc version (minor number): " CPL_STRINGIFY(__GNUC_MINOR__) "\n"
#endif
#ifdef __GNUC_PATCHLEVEL__
        "gcc version (patch level): " CPL_STRINGIFY(__GNUC_PATCHLEVEL__) "\n"
#endif
#ifdef __VERSION__
        "Compiler version: " __VERSION__ "\n"
#endif
#ifdef __LP64__
        CPL_XSTRINGIFY(__LP64__) ": " CPL_STRINGIFY(__LP64__) "\n"
#endif
#ifdef __PIC__
        CPL_XSTRINGIFY(__PIC__) ": " CPL_STRINGIFY(__PIC__) "\n"
#endif
#ifdef __OPTIMIZE__
        CPL_XSTRINGIFY(__OPTIMIZE__) ": " CPL_STRINGIFY(__OPTIMIZE__) "\n"
#endif
#ifdef __TIMESTAMP__
        "Last modification of " __FILE__ ": " __TIMESTAMP__ "\n"
#endif
#endif
        ;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Dump various process information

 */
/*----------------------------------------------------------------------------*/
static void cpl_test_dump_status(void)
{

#if defined HAVE_GETPID && defined CPL_TEST_DUMP_STATUS

    const pid_t  pid  = getpid();
    char * file = cpl_sprintf("/proc/%u/status", (const unsigned)pid);
    FILE       * stream = fopen(file, "r");

    if (stream != NULL) {
        char line[CPL_MAX_MSG_LENGTH]; 

        while (fgets(line, CPL_MAX_MSG_LENGTH, stream) != NULL) {
            /* Ignore newline */
            char * retpos = memchr(line, '\n', CPL_MAX_MSG_LENGTH);
            if (retpos != NULL) *retpos = 0;

            cpl_msg_debug(cpl_func, "%s", line);
        }

        fclose(stream);
    }

    cpl_free(file);

#endif

    return;

}



/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Some simple FITS validation
  @param filename        The filename
  @param filename_string The filename as a string
  @note No input validation !

 */
/*----------------------------------------------------------------------------*/
static char * cpl_test_fits_file(const char * filename,
                                 const char * filename_string)
{

    char * self = NULL;

#ifdef HAVE_SYS_STAT_H
    struct stat buf;
    const int error = stat(filename, &buf);

    if (error) {
        self = cpl_sprintf("%s => %s stat() returned %d: %s",
                           filename_string, filename, error, strerror(errno));
    } else {
        const off_t size = buf.st_size;
        if (size == 0) {
            self = cpl_sprintf("%s => %s has zero size", filename_string,
                               filename);
        } else {
            const off_t rem = size % 2880;
            if (rem != 0) {
                self = cpl_sprintf("%s => %s has illegal size=%luB with %uB "
                                   "in excess of the 2880B-blocks",
                                   filename_string, filename,
                                   (long unsigned)size, (unsigned)rem);
            }
        }
    }
#endif

    return self;

}
