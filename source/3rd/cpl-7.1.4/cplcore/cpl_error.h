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

#ifndef CPL_ERROR_H
#define CPL_ERROR_H

#include <stdlib.h>
#include <cpl_macros.h>
#include <cpl_func.h>

#include <cxutils.h>
 
CPL_BEGIN_DECLS

/*
 *
 * This module provides functions to maintain the @em cpl_error status.
 */


/*-----------------------------------------------------------------------------
                                   Defines
 -----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_error
  @brief    The maximum length of a CPL error message
  @see cpl_error_get_message()
 */
/*----------------------------------------------------------------------------*/
#ifndef CPL_ERROR_MAX_MESSAGE_LENGTH
#define CPL_ERROR_MAX_MESSAGE_LENGTH 256
#endif


/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_error
  @hideinitializer
  @brief    Flag to indicate support for variadic macros
  @see cpl_error_set_message()
  @note Unless already defined, tries to detect whether variadic macros are
        supported, typically at compile-time of a CPL-based application.
        This check, which is hardly robust, should support
        - disabling of variadic macros when compiling with @em gcc @em -ansi
        - enabling them when compiling with a C99 compliant compiler, such as
          @em gcc @em -std=c99
 */
/*----------------------------------------------------------------------------*/
#ifndef CPL_HAVE_VA_ARGS
#   if !defined __STDC_VERSION__ && defined __STRICT_ANSI__
      /* (llvm-)gcc -ansi and (llvm-)gcc -std=c89 do not define __STDC_VERSION__
         but they do define __STRICT_ANSI__ */
#     define CPL_HAVE_VA_ARGS 0
#  else
    /* (llvm-)gcc with other language standards (incl. gnu89 (equivalent to
       no language specification), iso9899:199409, c99) reaches this point */
    /* Sunstudio 12.1 c99 reaches this point since it defines __STDC_VERSION__
       to 199901L as expected (it also defines __STRICT_ANSI__) */
#     define CPL_HAVE_VA_ARGS 1
#  endif
#endif

/*----------------------------------------------------------------------------*/
/**
 * @ingroup cpl_error
   @hideinitializer
   @brief   Generic error handling macro
   @param   CONDITION    The condition to check
   @param   CODE         The CPL error code to set if @em CONDTION is non-zero
   @param   ACTION       A statement that is executed iff the @em CONDITION
                         evaluates to  non-zero.
   @param   ...          A printf-style message for cpl_error_set_message().
   @see cpl_error_set_message()
   @note This macro should not be used directly. It is defined only to support
   user-defined error handling macros. If CODE equals CPL_ERROR_NONE, a CPL
   error with code CPL_ERROR_UNSPECIFIED is created. The message may be a
   printf-like format string supplied with arguments unless variadic macros
   are not supported in which case only a (non-format) string is accepted.
   The provided @em CODE, @em ACTION and @em __VA_ARGS__ are evaluated/executed
   only if the @em CONDITION evaluates to false (zero). The @em ACTION @em break
   is unsupported (it currently causes the execution to continue after @em
   cpl_error_ensure()). Some of the below examples
   use a @em goto statement to jump to a single cleanup label, assumed to
   be located immediately before the function's unified cleanup-code and return
   point. This error-handling scheme is reminiscent of using exceptions in
   languages that support exceptions (C++, Java, ...). The use of @em goto and
   a single clean-up label per function "if the error-handling code is non-
   trivial, and if errors can occur in several places" is sanctioned by
   Kernigan & Richie: "The C Programming Language".
   For any other purpose @em goto should be avoided.

   Useful definitions might include

   @code

   #define assure(BOOL, CODE)                                                  \
     cpl_error_ensure(BOOL, CODE, goto cleanup, " ")

   #define  check(CMD)                                                         \
     cpl_error_ensure((CMD, cpl_error_get_code() == CPL_ERROR_NONE),           \
                      cpl_error_get_code(), goto cleanup, " ")

   #define assert(BOOL)                                                        \
     cpl_error_ensure(BOOL, CPL_ERROR_UNSPECIFIED, goto cleanup,               \
                      "Internal error, please report to " PACKAGE_BUGREPORT)

   @endcode

   or (same as above, but including printf-style error messages)

   @code

   #define assure(BOOL, CODE, ...)                                             \
     cpl_error_ensure(BOOL, CODE, goto cleanup, __VA_ARGS__)

   #define  check(CMD, ...)                                                    \
     cpl_error_ensure((CMD, cpl_error_get_code() == CPL_ERROR_NONE),           \
                      cpl_error_get_code(), goto cleanup, __VA_ARGS__)

   #define assert(BOOL, ...)                                                   \
     cpl_error_ensure(BOOL, CPL_ERROR_UNSPECIFIED, goto cleanup,               \
                      "Internal error, please report to " PACKAGE_BUGREPORT    \
                      " " __VA_ARGS__)
                      / *  Assumes that PACKAGE_BUGREPORT
                           contains no formatting special characters  * /
   
   @endcode

   or

   @code

   #define skip_if(BOOL)                                                       \
     cpl_error_ensure(BOOL, cpl_error_get_code() != CPL_ERROR_NONE ?           \
                      cpl_error_get_code() : CPL_ERROR_UNSPECIFIED,            \
                      goto cleanup, " ")

   @endcode

   The check macros in the examples above can be used to check a command
   which sets the cpl_error_code in case of failure (or, by use of a comma
   expression, a longer sequence of such commands):
   @code
   check(
       (x = cpl_table_get_int(table, "x", 0, NULL),
        y = cpl_table_get_int(table, "y", 0, NULL),
        z = cpl_table_get_int(table, "z", 0, NULL)),
       "Error reading wavelength catalogue");
   @endcode
 
*/
/*----------------------------------------------------------------------------*/

#if defined CPL_HAVE_VA_ARGS && CPL_HAVE_VA_ARGS == 1 || defined DOXYGEN_SKIP
#define cpl_error_ensure(CONDITION, CODE, ACTION, ...)                         \
    do if (CPL_UNLIKELY(!(CONDITION))) {                                       \
        /* Evaluate exactly once using a name-space protected variable name */ \
        const cpl_error_code cpl_ensure_error = (CODE);                        \
        (void)cpl_error_set_message_macro(cpl_func, cpl_ensure_error           \
                                          ? cpl_ensure_error :                 \
                                          CPL_ERROR_UNSPECIFIED,               \
                                           __FILE__, __LINE__, __VA_ARGS__);   \
        ACTION;                                                                \
    } while (0)
#else
#define cpl_error_ensure(CONDITION, CODE, ACTION, MSG)                         \
    do if (CPL_UNLIKELY(!(CONDITION))) {                                       \
        /* Evaluate exactly once using a name-space protected variable name */ \
        const cpl_error_code cpl_ensure_error = (CODE);                        \
        (void)cpl_error_set_message_one_macro(cpl_func, cpl_ensure_error       \
                                              ? cpl_ensure_error :             \
                                              CPL_ERROR_UNSPECIFIED,           \
                                              __FILE__, __LINE__, MSG);        \
        ACTION;                                                                \
    } while (0)
#endif


/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_error
  @hideinitializer
  @brief    Set an error code and return iff a boolean expression is false
  @param    BOOL      The boolean to evaluate
  @param    ERRCODE   The error code to set
  @param    RETURN    The value to return
  @note     This macro will cause a return from its calling function.
            If ERRCODE equals CPL_ERROR_NONE, the error-code is set
            to CPL_ERROR_UNSPECIFIED.
            The boolean is always evaluated (unlike assert()).
            This macro may not be used in inline'd functions.
  @see cpl_error_set()

 */
/*----------------------------------------------------------------------------*/
#define cpl_ensure(BOOL, ERRCODE, RETURN)                                      \
 cpl_error_ensure(BOOL, ERRCODE, return(RETURN), " ")

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_error
  @hideinitializer
  @brief    Set an error code and return it iff a boolean expression is false
  @param    BOOL      The boolean to evaluate
  @param    ERRCODE   The error code to set and return
  @see cpl_ensure()

 */
/*----------------------------------------------------------------------------*/
#define cpl_ensure_code(BOOL, ERRCODE)                                         \
 cpl_error_ensure(BOOL, ERRCODE, return(cpl_error_get_code()), " ")


/**
 * @ingroup cpl_error
 * @hideinitializer
 * @brief
 *   Set CPL error code, function name, source file and line number where 
 *   an error occurred.
 *
 * @param function  Character string with function name, cpl_func
 * @param code      Error code
 *
 * @return The specified error code.
 * @note If code is CPL_ERROR_NONE, nothing is done (and code is returned),
 *       if code is CPL_ERROR_HISTORY_LOST (but avoid that) then
 *       CPL_ERROR_UNSPECIFIED is set and returned.
 *
 * @hideinitializer
 */

#define cpl_error_set(function, code) \
        cpl_error_set_message_macro(function, code, __FILE__, __LINE__, " ")


/**
 * @ingroup cpl_error
 * @hideinitializer
 * @brief Propagate a CPL-error to the current location.
 *
 * @param function  Character string with function name, cpl_func
 *
 * @return The preexisting CPL error code (possibly CPL_ERROR_NONE).
 * @see cpl_error_set()
 * @note If no error exists, nothing is done and CPL_ERROR_NONE is returned
 *
 * If a CPL error already exists, this function creates a new CPL error
 * with the preexisting CPL error code and the current location.
 *
 * In a function of type cpl_error_code an error can be propagated with:
 *  @code
 *
 *    return cpl_error_set_where(cpl_func);
 *
 * @endcode
 *
 * @hideinitializer
 */

#define cpl_error_set_where(function)                                          \
       cpl_error_set_message_macro(function, cpl_error_get_code(),             \
                                    __FILE__, __LINE__, " ")

/**
 * @ingroup cpl_error
 * @hideinitializer
 * @brief
 *   Set CPL error code, function name, source file and line number where 
 *   an error occurred along with a text message
 *
 * @param function    Character string with function name, cpl_func
 * @param code        Error code
 * @param ...         Variable argument list with message
 * @return The CPL error code
 * @see cpl_error_set()
 * @note The message is ignored if it is NULL, empty, or consists of a single
 *        ' ' (space). Otherwise, the user supplied message is appended to
 *       the default message using ': ' (colon+space) as delimiter.
 *       If the CPL-based application may use variadic macros, the message
 *       may be a printf-style format string supplied with matching arguments.
 *       If variadic macros are not allowed (e.g. when compiling with gcc -ansi)
 *       only a non-format string is accepted. Please be aware that the format
 *       string should always be a string literal, otherwise the code may be
 *       vulnerable to the socalled 'format string exploit'. Sufficiently recent
 *       versions of gcc supports the option -Wformat-security, which tries to
 *       warn of this issue. If variadic macros are not supported, a 
 *       printf-style message can be created prior to the call to
 *       cpl_error_set_message(), as in the below example.
 *
 * Examples of usage:
 * @code
 *
 *    if (image == NULL) {
 *        return cpl_error_set_message(cpl_func, CPL_ERROR_NULL_INPUT,
 *                                     "Image number %d is NULL", number);
 *    }
 *
 * @endcode
 *
 * @code
 *
 *    if (error_string != NULL) {
 *        return cpl_error_set_message(cpl_func, CPL_ERROR_ILLEGAL_INPUT,
 *                                     "%s", error_string);
 *    }
 *
 * @endcode
 *
 * Example of usage if and only if variadic macros are unavaiable (e.g. when
 * compiling with gcc -ansi):
 * @code
 *
 *    if (image == NULL) {
 *        char * my_txt = cpl_sprintf("Image number %d is NULL", number);
 *        const cpl_error_code my_code =
 *            cpl_error_set_message(cpl_func, CPL_ERROR_NULL_INPUT, my_txt);
 *        cpl_free(my_txt);
 *        return my_code;
 *    }
 *
 * @endcode
 *
 * @hideinitializer
 */
#if defined CPL_HAVE_VA_ARGS && CPL_HAVE_VA_ARGS == 1 || defined DOXYGEN_SKIP
#define cpl_error_set_message(function, code, ...)                             \
    cpl_error_set_message_macro(function, code, __FILE__, __LINE__, __VA_ARGS__)
#else
#define cpl_error_set_message(function, code, text)                        \
    cpl_error_set_message_one_macro(function, code, __FILE__, __LINE__, text)
#endif

/*-----------------------------------------------------------------------------
                                   New types
 -----------------------------------------------------------------------------*/


/**
 * @ingroup cpl_error
 * @brief 
 *   Available error codes.

     @c CPL_ERROR_NONE is equal to zero and compares to less than all other
     error codes.

     All error codes are guaranteed to be less than @c CPL_ERROR_EOL
     (End Of List). @c CPL_ERROR_EOL allows user defined error
     codes to be added in this fashion:

    @code

      const int my_first_error_code  = 0 + CPL_ERROR_EOL;
      const int my_second_error_code = 1 + CPL_ERROR_EOL;
      const int my_third_error_code  = 2 + CPL_ERROR_EOL;

      if (is_connected() == CPL_FALSE) {
         return cpl_error_set_message(cpl_func, my_first_error_code, "No "
                                      "connection to host %s (after %u tries)",
                                      hostname, max_attempts);
      }
  
    @endcode

    Extensive use of user defined error codes should be avoided. Instead a
    request of new CPL error codes should be emailed to cpl-help@eso.org.

 */

enum _cpl_error_code_ {
    CPL_ERROR_NONE = 0,         
    /**< No error condition */
    CPL_ERROR_UNSPECIFIED = 1,
    /**< An unspecified error */
    CPL_ERROR_HISTORY_LOST,
    /**< The actual CPL error has been lost. Do not use to create a CPL error */
    CPL_ERROR_DUPLICATING_STREAM,
    /**< Could not duplicate output stream */
    CPL_ERROR_ASSIGNING_STREAM,
    /**< Could not associate a stream with a file descriptor */
    CPL_ERROR_FILE_IO,          
    /**< Permission denied */
    CPL_ERROR_BAD_FILE_FORMAT,  
    /**< Input file had not the expected format */
    CPL_ERROR_FILE_ALREADY_OPEN,
    /**< Attempted to open a file twice */
    CPL_ERROR_FILE_NOT_CREATED, 
    /**< Could not create a file */
    CPL_ERROR_FILE_NOT_FOUND,   
    /**< A specified file or directory was not found */
    CPL_ERROR_DATA_NOT_FOUND,           
    /**< Data searched within a valid object were not found */
    CPL_ERROR_ACCESS_OUT_OF_RANGE,      
    /**< Data were accessed beyond boundaries */
    CPL_ERROR_NULL_INPUT,               
    /**< A @c NULL pointer was found where a valid pointer was expected */
    CPL_ERROR_INCOMPATIBLE_INPUT,       
    /**< Data that had to be processed together did not match */
    CPL_ERROR_ILLEGAL_INPUT,            
    /**< Illegal values were detected */
    CPL_ERROR_ILLEGAL_OUTPUT,           
    /**< A given operation would have generated an illegal object */
    CPL_ERROR_UNSUPPORTED_MODE,         
    /**< The requested functionality is not supported */
    CPL_ERROR_SINGULAR_MATRIX,          
    /**< Could not invert a matrix */
    CPL_ERROR_DIVISION_BY_ZERO,         
    /**< Attempted to divide a number by zero */
    CPL_ERROR_TYPE_MISMATCH,             
    /**< Data were not of the expected type */
    CPL_ERROR_INVALID_TYPE,         
    /**< Data type was unsupported or invalid */
    CPL_ERROR_CONTINUE,
    /**< An iterative process did not converge */
    CPL_ERROR_NO_WCS,
    /**< The WCS functionalities are missing */
    CPL_ERROR_EOL
    /**< To permit extensibility of error handling.
         It is a coding error to use this within CPL itself! */
};

/**
 * @ingroup cpl_error
 * @brief
 *   The cpl_error_code type definition
 */

typedef enum _cpl_error_code_ cpl_error_code;

typedef struct _cpl_error_ cpl_error;

/*-----------------------------------------------------------------------------
                              Function prototypes
 -----------------------------------------------------------------------------*/

void           cpl_error_reset(void);
cpl_error_code
cpl_error_set_message_macro(const char *, cpl_error_code,
                            const char *, unsigned,
                            const char *, ...) CPL_ATTR_PRINTF(5,6)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(3)))
#endif
     ;

cpl_error_code
cpl_error_set_message_one_macro(const char *, cpl_error_code,
                                const char *, unsigned,
                                const char *)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(3)))
#endif
    ;

cpl_error_code  cpl_error_get_code(void)     CPL_ATTR_PURE;
const char     *cpl_error_get_message(void)  CPL_ATTR_PURE;
const char     *cpl_error_get_message_default(cpl_error_code) CPL_ATTR_CONST;
const char     *cpl_error_get_function(void) CPL_ATTR_PURE;
const char     *cpl_error_get_file(void)     CPL_ATTR_PURE;
unsigned        cpl_error_get_line(void)     CPL_ATTR_PURE;
const char     *cpl_error_get_where(void)    CPL_ATTR_PURE;

CPL_END_DECLS

#endif
/* end of cpl_error.h */
