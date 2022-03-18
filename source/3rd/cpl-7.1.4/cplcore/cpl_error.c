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

#include <string.h>

#ifdef HAVE_LIBPTHREAD
#include <pthread.h>
#endif

#include <cxmessages.h>
#include <cxutils.h>

#include "cpl_error_impl.h"

/* Needed for cpl_sprintf() */
#include "cpl_memory.h"
/* Needed for CPL_STRINGIFY() */
#include "cpl_tools.h"

/* Needed for FLEN_STATUS */
#include <fitsio.h>


/**
 * @defgroup cpl_error Error handling
 *
 * This module provides functions to maintain the @em cpl_error_code
 * set by any CPL function, similarly to what is done with the @em errno 
 * variable of the standard C library. The following guidelines are 
 * respected:
 *
 * - If no error occurs in a CPL function the @em cpl_error_code will 
 *   remain unchanged.
 * - If an error occurs in a CPL function, a new CPL error is set, causing
 *   the @em cpl_error_code to be modified to the new error.
 *
 * A @em cpl_error_code equal to the enumeration constant 
 * @c CPL_ERROR_NONE would indicate no error condition. Note, 
 * however, that the @em cpl_error_code is only set when an error 
 * occurs, and it is not reset by successful function calls.
 * For this reason it may be appropriate in some cases to reset 
 * the @em cpl_error_code using the function @c cpl_error_reset(). 
 * The @em cpl_error_code set by a CPL function can be obtained by 
 * calling the function @c cpl_error_get_code(), but functions of 
 * type @em cpl_error_code would not only return this code directly, 
 * but would also return @c CPL_ERROR_NONE in case of success. Other 
 * CPL functions return zero on success, or a non-zero value to indicate 
 * a change of the @em cpl_error_code, while CPL functions returning 
 * a pointer would flag an error by returning a @c NULL.
 *
 * To each @em cpl_error_code is associated a standard error message,
 * that can be obtained by calling the function @c cpl_error_get_message(). 
 * Conventionally, no CPL function will ever display any error message, 
 * leaving to the caller the decision of how to handle a given error 
 * condition. A call to the function @c cpl_error_get_function() would 
 * return the name of the function where the error occurred, and the 
 * functions @c cpl_error_get_file() and @c cpl_error_get_line() would 
 * also return the name of the source file containing the function 
 * code, and the line number where the error occurred. The function
 * @c cpl_error_get_where() would gather all this items together, in
 * a colon-separated string.
 *
 * @par Synopsis:
 * @code
 *   #include <cpl_error.h>
 * @endcode
 */

/**@{*/

#ifndef CPL_LINESZ
#ifdef FITS_LINESZ
#define CPL_LINESZ FITS_LINESZ
#else
#define CPL_LINESZ 6
#endif
#endif

#define MAX_WHERE_LENGTH (2+(MAX_NAME_LENGTH)+(MAX_FILE_LENGTH)+(CPL_LINESZ))

/*-----------------------------------------------------------------------------
                                  Private variables 
 -----------------------------------------------------------------------------*/

#ifdef HAVE_LIBPTHREAD
static pthread_rwlock_t cpl_lock_error_status;
static pthread_rwlock_t cpl_lock_error_read_only;
#endif

static cpl_boolean cpl_error_status = CPL_FALSE;
static cpl_boolean cpl_error_read_only = CPL_FALSE;

#ifdef _OPENMP
#pragma omp threadprivate(cpl_error_status, cpl_error_read_only)
#endif

/*-----------------------------------------------------------------------------
                                  Private functions 
 -----------------------------------------------------------------------------*/

static cpl_error * cpl_error_fill(const char *, cpl_error_code, const char *,
                                  unsigned);
static cpl_error_code
cpl_error_set_message_macro_(const char *, cpl_error_code,
                             const char *, unsigned,
                             const char *, va_list) CPL_ATTR_PRINTF(5,0)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(3)))
#endif
    ;

/*-----------------------------------------------------------------------------
                                  Definitions of functions 
 -----------------------------------------------------------------------------*/

/**
 * @brief
 *   Reset the @em cpl_error_code.
 *
 * @return Nothing.
 *
 * This function initialises the @em cpl_error_code to @c CPL_ERROR_NONE.
 */

void cpl_error_reset(void)
{

#ifdef HAVE_LIBPTHREAD
    pthread_rwlock_wrlock(&cpl_lock_error_status);
    pthread_rwlock_rdlock(&cpl_lock_error_read_only);
#endif

    if (!cpl_error_read_only) cpl_error_status = CPL_FALSE;

#ifdef HAVE_LIBPTHREAD
    pthread_rwlock_unlock(&cpl_lock_error_status);
    pthread_rwlock_unlock(&cpl_lock_error_read_only);
#endif

}

/*
 * @internal
 * @brief
 *   Set CPL error code, function name, source file & line number where it
 *   occurred along with a text message
 * 
 * @param function Character string with function name, cpl_func
 * @param code     Error code
 * @param file     Character string with source file name (__FILE__)
 * @param line     Integer with line number (__LINE__)
 * @param text     Text to append to error message, may be printf-like
 * @param ...      Optional, variable argument list for the printf-like text
 * @return The CPL error code, or CPL_ERROR_UNSPECIFIED if the code is 
 *         CPL_ERROR_HISTORY_LOST.
 * @note This function is only provided for cpl_error_set*() macros.
 * @see cpl_error_set_message()
 */
cpl_error_code cpl_error_set_message_macro(const char * function,
                                           cpl_error_code code,
                                           const char * file, unsigned line,
                                           const char * text, ...)
{


    va_list        arglist;
    cpl_error_code lcode;


    va_start(arglist, text);

    lcode = cpl_error_set_message_macro_(function, code, file, line, text,
                                         arglist);

    va_end(arglist);

    return lcode;
}


/*----------------------------------------------------------------------------*/
/**
 * @internal
 * @brief
 *   Set CPL error code, function name, source file, line number where 
 *   an FITS error occurred along with a FITS specific text message
 * @param function     Character string with function name, cpl_func
 * @param code         CPL Error code
 * @param fitscode     The error code of the failed (CFITSIO) I/O call
 * @param fitsfunction Character string with (CFITSIO) function name
 * @param file         Character string with source file name (__FILE__)
 * @param line         Integer with line number (__LINE__)
 * @param text         Text to append to error message, may be printf-like
 * @param ...          Optional, variable argument list for the printf-like text
 * @return The CPL error code
 * @see cpl_error_set_message()
 * @note This function should only be called from the cpl_error_set_fits() macro
 */
cpl_error_code cpl_error_set_fits_macro(const char * function,
                                        cpl_error_code code,
                                        int fitscode,
                                        const char * fitsfunction,
                                        const char * file,
                                        unsigned line,
                                        const char * text,
                                        ...)
{


    char              cfitsio_msg[FLEN_ERRMSG];
    va_list           arglist;
    cpl_error_code    lcode;
    char            * newformat;
    const cpl_boolean has_name = fitsfunction && strcmp(fitsfunction, "\"\"");


    fits_get_errstatus(fitscode, cfitsio_msg);

    cfitsio_msg[FLEN_ERRMSG-1] = '\0' ; /* Better safe than sorry */

    newformat = cpl_sprintf("\"%s\" from CFITSIO "
#ifdef CFITSIO_VERSION
                            /* Used from v. 3.0 */
                            "(ver. " CPL_STRINGIFY(CFITSIO_VERSION) ") "
#endif

                            "%s%s=%d. %s", cfitsio_msg,
                            has_name ? fitsfunction : "",
                            has_name ? "()" : "error",
                            fitscode, text);

    va_start(arglist, text);

    lcode = cpl_error_set_message_macro_(function, code, file, line,
                                         newformat, arglist);

    va_end(arglist);

    cpl_free(newformat);

    return lcode;


}

/*----------------------------------------------------------------------------*/
/**
 * @internal
 * @brief
 *   Set CPL error code, function name, source file, line number where 
 *   a regex error occurred along with a regex specific text message
 * @param function     Character string with function name, cpl_func
 * @param code         CPL Error code
 * @param regcode      The error code of the failed regcomp() call
 * @param preg         The regex of the failed call
 * @param file         Character string with source file name (__FILE__)
 * @param line         Integer with line number (__LINE__)
 * @param text         Text to append to error message, may be printf-like
 * @param ...          Optional, variable argument list for the printf-like text
 * @return The CPL error code
 * @see cpl_error_set_message()
 * @note This function should only be called from cpl_error_set_regex()
 */
cpl_error_code cpl_error_set_regex_macro(const char * function,
                                        cpl_error_code code,
                                        int regcode,
                                        const regex_t * preg,
                                        const char * file,
                                        unsigned line,
                                        const char * text,
                                        ...)
{


    cpl_error_code lcode;
    va_list        arglist;
    char         * newformat;


    if (preg == NULL) {
        /* Passing NULL to regerror() seems to fill with the information
           of the previous error, which we do not want here */
        newformat = cpl_sprintf("regcomp(NULL)=%d. %s", regcode, text);
    } else {
        char regex_msg[CPL_ERROR_MAX_MESSAGE_LENGTH];

        (void)regerror(regcode, preg, regex_msg, CPL_ERROR_MAX_MESSAGE_LENGTH);

        newformat = cpl_sprintf("\"%s\" from regcomp()=%d. %s", regex_msg,
                                regcode, text);
    }

    va_start(arglist, text);

    lcode = cpl_error_set_message_macro_(function, code, file, line,
                                         newformat, arglist);

    va_end(arglist);

    cpl_free(newformat);

    return lcode;

}



/*----------------------------------------------------------------------------*/
/**
 * @internal
 * @brief
 *   Set CPL error code, function name, source file, line number where 
 *   an WCSLIB error occurred along with a WCSLIB specific text message
 * @param function    Character string with function name, cpl_func
 * @param code        CPL Error code
 * @param wcscode     The error code of the failed WCSLIB call
 * @param wcsfunction Character string with WCSLIB function name
 * @param wcserrmsg   The WCSLIB array of error messages
 * @param file        Character string with source file name (__FILE__)
 * @param line        Integer with line number (__LINE__)
 * @param text        Text to append to error message, may be printf-like
 * @param ...         Optional, variable argument list for the printf-like text
 * @return The CPL error code
 * @see cpl_error_set_message()
 * @note This function should only be called from the cpl_error_set_wcs() macro
 * 
 */
cpl_error_code cpl_error_set_wcs_macro(const char * function,
                                       cpl_error_code code,
                                       int wcserror,
                                       const char * wcsfunction,
                                       const char * wcserrmsg[],
                                       const char * file,
                                       unsigned line,
                                       const char * text,
                                       ...)
{

    cpl_error_code    lcode;
    va_list           arglist;
    char            * newformat;
    const cpl_boolean has_name = wcsfunction && strlen(wcsfunction);


    if (wcserror < 0) {
        newformat = cpl_sprintf("%s%s=%d < 0. %s",
                            has_name ? wcsfunction : "",
                            has_name ? "()" : "error", wcserror, text);
    } else if (wcserrmsg == NULL) {
        newformat = cpl_sprintf("%s%s()=%d. wcs_errmsg[] == NULL. %s",
                                has_name ? wcsfunction : "",
                                has_name ? "()" : "error", wcserror, text);
    } else if (wcserrmsg[wcserror] == NULL) {
        newformat = cpl_sprintf("%s%s()=%d. wcs_errmsg[%d] == NULL. %s",
                                has_name ? wcsfunction : "",
                                has_name ? "()" : "error",
                                wcserror, wcserror, text);
    } else {
        newformat = cpl_sprintf("\"%s\" from %s%s()=%d. %s", wcserrmsg[wcserror],
                                has_name ? wcsfunction : "",
                                has_name ? "()" : "error",
                                wcserror, text);
    }

    va_start(arglist, text);

    lcode = cpl_error_set_message_macro_(function, code, file, line,
                                         newformat, arglist);

    va_end(arglist);

    cpl_free(newformat);

    return lcode;

}


/**
 * @brief
 *   Get the last @em cpl_error_code set.
 *
 * @return  @em cpl_error_code of last occurred CPL error.
 *
 * Get @em cpl_error_code of last occurred error.
 */

cpl_error_code cpl_error_get_code(void)
{
    cpl_error_code code = CPL_ERROR_NONE;

#ifdef HAVE_LIBPTHREAD
    pthread_rwlock_rdlock(&cpl_lock_error_status);
#endif

    if (cpl_error_status) {
        const cpl_error * error = cpl_errorstate_find();

        code = error->code;
    }

#ifdef HAVE_LIBPTHREAD
    pthread_rwlock_unlock(&cpl_lock_error_status);
#endif

    return code;
}


/**
 * @brief
 *   Get the text message of the current CPL error.
 *
 * @return The text message of the current CPL error.
 * @see cpl_error_get_message_default(), cpl_error_set_message()
 *
 * If the @em cpl_error_code is equal to @c CPL_ERROR_NONE,
 * an empty string is returned. Otherwise, the message is the default
 * message for the current CPL error code, possibly extended with a
 * custom message supplied when the error was set.
 *
 */

const char * cpl_error_get_message(void)
{

    const cpl_error * error;

#ifdef HAVE_LIBPTHREAD
    pthread_rwlock_rdlock(&cpl_lock_error_status);
#endif

    if (!cpl_error_status) {
#ifdef HAVE_LIBPTHREAD
        pthread_rwlock_unlock(&cpl_lock_error_status);
#endif

        return cpl_error_get_message_default(CPL_ERROR_NONE);
    }

#ifdef HAVE_LIBPTHREAD
    pthread_rwlock_unlock(&cpl_lock_error_status);
#endif

    error = cpl_errorstate_find();

    /* assert(error->code != CPL_ERROR_NONE); */

    return strlen(error->msg) ? error->msg
        : cpl_error_get_message_default(error->code);
}

/**
 * @brief
 *   Get the function name where the last CPL error occurred.
 *
 * @return Identifier string of the function name where the last CPL error
 *         occurred.
 *
 * Get the function name where the last CPL error occurred.
 */

const char *cpl_error_get_function(void)
{
    const char * function = "";

#ifdef HAVE_LIBPTHREAD
    pthread_rwlock_rdlock(&cpl_lock_error_status);
#endif

    if (cpl_error_status) {
        const cpl_error * error = cpl_errorstate_find();

        function = error->function;
    }

#ifdef HAVE_LIBPTHREAD
    pthread_rwlock_unlock(&cpl_lock_error_status);
#endif

    return function;
}


/**
 * @brief
 *   Get function name, source file and line number where the last 
 *   CPL error occurred.
 *
 * @return String containing function name, source file and line number
 *   separated by colons (:).
 *
 * Get where the last CPL error occurred in the form 
 * @c function_name:source_file:line_number
 */

const char *cpl_error_get_where(void)
{

    static char cpl_error_where_string[MAX_WHERE_LENGTH];

#ifdef _OPENMP
#pragma omp threadprivate(cpl_error_where_string)
#endif

    (void)cx_snprintf((cxchar *)cpl_error_where_string,
                      (cxsize)MAX_WHERE_LENGTH, (const cxchar *)"%s:%s:%u",
                      cpl_error_get_function(), cpl_error_get_file(),
                      cpl_error_get_line());

    return cpl_error_where_string;

}


/**
 * @brief
 *   Get the source code file name where the last CPL error occurred.
 *
 * @return Name of source file name where the last CPL error occurred.
 *
 * Get the source code file name where the last CPL error occurred.
 */

const char *cpl_error_get_file(void)
{
    const char * file = "";

#ifdef HAVE_LIBPTHREAD
    pthread_rwlock_rdlock(&cpl_lock_error_status);
#endif

    if (cpl_error_status) {
        const cpl_error * error = cpl_errorstate_find();

        file = error->file;
    }

#ifdef HAVE_LIBPTHREAD
    pthread_rwlock_unlock(&cpl_lock_error_status);
#endif

    return file;
}


/**
 * @brief
 *   Get the line number where the last CPL error occurred.
 *
 * @return  Line number of the source file where the last CPL error occurred.
 *
 * Get the line number of the source file where the last CPL error occurred.
 */

unsigned cpl_error_get_line(void)
{
    unsigned line = 0;

#ifdef HAVE_LIBPTHREAD
    pthread_rwlock_rdlock(&cpl_lock_error_status);
#endif

    if (cpl_error_status) {
        const cpl_error * error = cpl_errorstate_find();

        line = error->line;
    }

#ifdef HAVE_LIBPTHREAD
    pthread_rwlock_unlock(&cpl_lock_error_status);
#endif

    return line;
}


/*----------------------------------------------------------------------------*/
/**
  @brief  Return the standard CPL error message of the current CPL error
  @param  code The error code of the current CPL error
  @return The standard CPL error message of the current CPL error

 */
/*----------------------------------------------------------------------------*/
const char * cpl_error_get_message_default(cpl_error_code code)
{
    const char * message;

    switch (code) {
        case CPL_ERROR_NONE: 
            message = "";
            break;
        case CPL_ERROR_UNSPECIFIED: 
            message = "An unspecified error"; 
            break;
        case CPL_ERROR_HISTORY_LOST: 
            message = "The actual error was lost"; 
            break;
        case CPL_ERROR_DUPLICATING_STREAM: 
            message = "Cannot duplicate output stream";
            break;
        case CPL_ERROR_ASSIGNING_STREAM: 
            message = "Cannot associate a stream with a file descriptor";
            break;
        case CPL_ERROR_FILE_IO: 
            message = "File read/write error";
            break;
        case CPL_ERROR_BAD_FILE_FORMAT: 
            message = "Bad file format";
            break;
        case CPL_ERROR_FILE_ALREADY_OPEN: 
            message = "File already open";
            break;
        case CPL_ERROR_FILE_NOT_CREATED: 
            message = "File cannot be created";
            break;
        case CPL_ERROR_FILE_NOT_FOUND: 
            message = "File not found";
            break;
        case CPL_ERROR_DATA_NOT_FOUND: 
            message = "Data not found";
            break;
        case CPL_ERROR_ACCESS_OUT_OF_RANGE: 
            message = "Access beyond boundaries";
            break;
        case CPL_ERROR_NULL_INPUT: 
            message = "Null input data";
            break;
        case CPL_ERROR_INCOMPATIBLE_INPUT: 
            message = "Input data do not match";
            break;
        case CPL_ERROR_ILLEGAL_INPUT: 
            message = "Illegal input";
            break;
        case CPL_ERROR_ILLEGAL_OUTPUT: 
            message = "Illegal output";
            break;
        case CPL_ERROR_UNSUPPORTED_MODE: 
            message = "Unsupported mode";
            break;
        case CPL_ERROR_SINGULAR_MATRIX: 
            message = "Singular matrix";
            break;
        case CPL_ERROR_DIVISION_BY_ZERO: 
            message = "Division by zero";
            break;
        case CPL_ERROR_TYPE_MISMATCH: 
            message = "Type mismatch";
            break;
        case CPL_ERROR_INVALID_TYPE: 
            message = "Invalid type";
            break;
        case CPL_ERROR_CONTINUE: 
            message = "The iterative process did not converge";
            break;
        case CPL_ERROR_NO_WCS: 
            message = "The WCS functionalities are missing";
            break;
        case CPL_ERROR_EOL: 
            message = "A user-defined error"; 
            break;
        default: 
            message = "A user-defined error"; 
            break;
    }

    return message;
}

#ifdef HAVE_LIBPTHREAD
/*----------------------------------------------------------------------------*/
/**
  @brief  Lock the RW locks of the global variables in this module
 */
/*----------------------------------------------------------------------------*/
void cpl_error_init_locks(void)
{
    pthread_rwlock_init(&cpl_lock_error_status, NULL);
    pthread_rwlock_init(&cpl_lock_error_read_only, NULL);
}
#endif

/**@}*/


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Get the status of the CPL error state
  @return   True iff an error code has been set
  @note This function may only be used by the cpl_errorstate module.

  CPL_FALSE: The CPL error state is clear, no history may be read.
  CPL_TRUE:  The CPL error state has been set, and contains at least one
             CPL error state (with a non-zero error code).

 */
/*----------------------------------------------------------------------------*/
cpl_boolean cpl_error_is_set(void)
{
    cpl_boolean status;

#ifdef HAVE_LIBPTHREAD
    pthread_rwlock_rdlock(&cpl_lock_error_status);
#endif

    status = cpl_error_status;

#ifdef HAVE_LIBPTHREAD
    pthread_rwlock_unlock(&cpl_lock_error_status);
#endif

    return status;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Get the read-only status of the CPL error system
  @return   True iff the CPL error system is read-only
  @note This function may only be used by the cpl_errorstate module.

 */
/*----------------------------------------------------------------------------*/
cpl_boolean cpl_error_is_readonly(void)
{
    cpl_boolean read_only;

#ifdef HAVE_LIBPTHREAD
    pthread_rwlock_rdlock(&cpl_lock_error_read_only);
#endif

    read_only = cpl_error_read_only;

#ifdef HAVE_LIBPTHREAD
    pthread_rwlock_unlock(&cpl_lock_error_read_only);
#endif

    return read_only;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Set the status of the CPL error system to read-only
  @note This function may only be used by the cpl_errorstate module.

 */
/*----------------------------------------------------------------------------*/
void cpl_error_set_readonly(void)
{

#ifdef HAVE_LIBPTHREAD
    pthread_rwlock_wrlock(&cpl_lock_error_read_only);
#endif

    cpl_error_read_only = CPL_TRUE;

#ifdef HAVE_LIBPTHREAD
    pthread_rwlock_unlock(&cpl_lock_error_read_only);
#endif

}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Set the status of the CPL error system to read/write
  @note This function may only be used by the cpl_errorstate module.

 */
/*----------------------------------------------------------------------------*/
void cpl_error_reset_readonly(void)
{

#ifdef HAVE_LIBPTHREAD
    pthread_rwlock_wrlock(&cpl_lock_error_read_only);
#endif

    cpl_error_read_only = CPL_FALSE;

#ifdef HAVE_LIBPTHREAD
    pthread_rwlock_unlock(&cpl_lock_error_read_only);
#endif

}

/*
 * @internal
 * @brief Set the basics of a CPL error and return the struct
 * @param function Character string with function name, cpl_func
 * @param code     Error code
 * @param file     Character string with source file name (__FILE__)
 * @param line     Integer with line number (__LINE__)
 * @return A pointer to the struct of the error.
 * @see cpl_error_set_message_macro()
 */
static cpl_error * cpl_error_fill(const char * function, cpl_error_code code,
                                  const char * file, unsigned line)
{

    cpl_error * error = cpl_errorstate_append();

    cx_assert(error != NULL);
    cx_assert(code != CPL_ERROR_NONE);

#ifdef HAVE_LIBPTHREAD
    pthread_rwlock_rdlock(&cpl_lock_error_read_only);
#endif

    cx_assert(!cpl_error_read_only);

#ifdef HAVE_LIBPTHREAD
    pthread_rwlock_unlock(&cpl_lock_error_read_only);

    pthread_rwlock_wrlock(&cpl_lock_error_status);
#endif

    cpl_error_status = CPL_TRUE;

#ifdef HAVE_LIBPTHREAD
    pthread_rwlock_unlock(&cpl_lock_error_status);
#endif

    error->code = code;
    error->line = line;

    if (function == NULL) {
        error->function[0] = '\0';
    } else {
        (void)strncpy(error->function, function, MAX_NAME_LENGTH);
        error->function[MAX_NAME_LENGTH] = '\0';
    }

    if (file == NULL) {
        error->file[0] = '\0';
    } else {
        (void)strncpy(error->file, file, MAX_FILE_LENGTH);
        error->file[MAX_FILE_LENGTH] = '\0';
    }

    error->msg[0] = '\0';

    return error;

}

/*
 * @internal
 * @brief
 *   Set CPL error code, function name, source file & line number where it
 *   occurred along with a text message and a variable argument list
 * @param function Character string with function name (cpl_func)
 * @param code     Error code
 * @param file     Character string with source file name (__FILE__)
 * @param line     Integer with line number (__LINE__)
 * @param text     Text to append to error message, may be printf-like
 * @param arglist  Optional, variable argument list for the printf-like text
 * @return The CPL error code, or CPL_ERROR_UNSPECIFIED if the code is 
 *         CPL_ERROR_HISTORY_LOST.
 * @see cpl_error_set_message_macro()
 */
static
cpl_error_code cpl_error_set_message_macro_(const char * function,
                                            cpl_error_code code,
                                            const char * file, unsigned line,
                                            const char * text, va_list arglist)
{
    /* Check copied from cpl_error_set_message_one_macro() */
    char * message = (text != NULL && text[0] != '\0' &&
                      (text[0] != ' ' || text[1] != '\0'))
        ? cpl_vsprintf(text, arglist) : NULL;
    const char * usemsg = message ? message : text;

    const cpl_error_code lcode = cpl_error_set_message_one_macro(function, code,
                                                                 file, line,
                                                                 usemsg);
    cpl_free((void*)message);

    return lcode;
}


/*
 * @internal
 * @brief
 *   Set CPL error code, function name, source file & line number where it
 *   occurred along with a text message and a variable argument list
 * @param function Character string with function name (cpl_func)
 * @param code     Error code
 * @param file     Character string with source file name (__FILE__)
 * @param line     Integer with line number (__LINE__)
 * @param text     Text to append to error message
 * @return The CPL error code, or CPL_ERROR_UNSPECIFIED if the code is 
 *         CPL_ERROR_HISTORY_LOST.
 * @see cpl_error_set_message_macro_()
 */
cpl_error_code cpl_error_set_message_one_macro(const char * function,
                                               cpl_error_code code,
                                               const char * file,
                                               unsigned line,
                                               const char * text)
{

    const cpl_error_code lcode = code == CPL_ERROR_HISTORY_LOST
        ? CPL_ERROR_UNSPECIFIED : code;

#ifdef HAVE_LIBPTHREAD
    pthread_rwlock_rdlock(&cpl_lock_error_read_only);
#endif

    if (!cpl_error_read_only && code != CPL_ERROR_NONE) {

        cpl_error * error = cpl_error_fill(function, lcode, file, line);

        if (text != NULL && text[0] != '\0' &&
            (text[0] != ' ' || text[1] != '\0')) {
            /* The user supplied a message */
            /* Calling this function with text NULL or empty is supported,
               but causes a compiler warning on some systems. To support calls
               that do not set a message, call with a single space causes that
               user message to be ignored. */

            /* Concatenate the standard message and the user supplied message */

            cx_assert( error != NULL );
            (void)cx_snprintf((cxchar *)error->msg,
                              (cxsize)CPL_ERROR_MAX_MESSAGE_LENGTH,
                              (const cxchar *)"%s: %s",
                              cpl_error_get_message_default(lcode),
                              text);
        }
    }

#ifdef HAVE_LIBPTHREAD
    pthread_rwlock_unlock(&cpl_lock_error_read_only);
#endif

    return lcode;

}
