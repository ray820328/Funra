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

#ifndef CPL_ERROR_IMPL_H
#define CPL_ERROR_IMPL_H

#include "cpl_error.h"

#include "cpl_type.h"
#include "cpl_tools.h"

#include <regex.h>

CPL_BEGIN_DECLS

/*-----------------------------------------------------------------------------
                                   Define
 -----------------------------------------------------------------------------*/
/*
 *  As the error handler itself should never fail, it cannot rely on
 *  allocating memory dynamically. Therefore, define max limits for
 *  string lengths, size of error queue.
 */

#ifdef PATH_MAX
#define MAX_FILE_LENGTH PATH_MAX
#else
#define MAX_FILE_LENGTH 4096
#endif

#define MAX_NAME_LENGTH 50

/**
 * @internal
 * @ingroup cpl_error
 * @brief Set CPL error code with the current location
 * @param code    Error code
 * @return The specified error code.
 * @see cpl_error_set()
 *
 */

#define cpl_error_set_(code) cpl_error_set(cpl_func, code)


/**
 * @internal
 * @ingroup cpl_error
 * @brief Propagate a CPL-error to the current location.
 * @return The preexisting CPL error code (possibly CPL_ERROR_NONE).
 * @see cpl_error_set_where()
 */

#define cpl_error_set_where_() cpl_error_set_where(cpl_func)

/**
 * @internal
 * @ingroup cpl_error
 * @brief Set CPL error code with the current location along with a text message
 * @param code        Error code
 * @param ...         Variable argument list with message
 * @return The CPL error code
 * @see cpl_error_set_message()
 */

#define cpl_error_set_message_(code, ...)               \
    cpl_error_set_message(cpl_func, code, __VA_ARGS__)


/**
 * @ingroup cpl_error
 * @internal
 * @brief
 *   Set CPL error code, function name, source file, line number where 
 *   a FITS error occurred along with a FITS specific text message
 *
 * @param code         CPL Error code
 * @param fitscode     The error code of the failed FITS (CFITSIO) call
 * @param fitsfunction The FITS (CFITSIO) function (_not_ as a string) or
 *                     the empty string
 * @param ...          Variable argument list with message
 * @return The CPL error code
 * @see cpl_error_set_message()
 *
 * Example of usage:
 *  @code
 *
 *  int ioerror = 0;
 *  (void)fits_open_diskfile(&file, name, READONLY, &ioerror);
 *
 *  if (ioerror) {
 *     return cpl_error_set_fits(CPL_ERROR_FILE_IO, ioerror,
 *                               fits_open_diskfile,
 *                               "filename='%s', xtnum=%d", name, position);
 *  }
 *
 * @endcode
 *
 * @hideinitializer
 */
#define cpl_error_set_fits(code, fitscode, fitsfunction, ...)   \
    cpl_error_set_fits_macro(cpl_func, code, fitscode,          \
                             CPL_STRINGIFY(fitsfunction),       \
                             __FILE__, __LINE__, __VA_ARGS__)

/**
 * @ingroup cpl_error
 * @internal
 * @brief
 *   Set CPL error code, function name, source file, line number where 
 *   a regex error occurred along with a regex specific text message
 *
 * @param code      CPL Error code
 * @param regcode   The error code of the failed regcomp() call
 * @param preg      The regex of the failed call
 * @param ...       Variable argument list with message
 * @return The CPL error code
 * @see regcomp(), cpl_error_set_message()
 *
 * Example of usage:
 *  @code
 *
 *  regex_t regex;
 *  const int regerror = regcomp(&regex, pattern, REG_EXTENDED | REG_NOSUB);
 *
 *  if (regerror) {
 *     return cpl_error_set_regex(CPL_ERROR_ILLEGAL_INPUT, regerror, &regex,
 *                               "pattern='%s', pattern);
 *  }
 *
 * @endcode
 *
 * @hideinitializer
 */
#define cpl_error_set_regex(code, regcode, preg, ...)           \
    cpl_error_set_regex_macro(cpl_func, code, regcode, preg,    \
                              __FILE__, __LINE__, __VA_ARGS__)

#ifndef CPL_WCS_INSTALLED
/* Make sure the WCS error macro can be invoked also when
   wcs_errmsg is not defined. This is useful for unit testing. */
#define wcs_errmsg NULL
#endif

/**
 * @ingroup cpl_error
 * @internal
 * @brief
 *   Set CPL error code, function name, source file, line number where 
 *   a WCSLIB error occurred along with a WCSLIB specific text message
 * @param code        CPL Error code
 * @param wcscode     The error code of the failed WCSLIB call
 * @param wcsfunction Character string with WCSLIB function name or
*                     the empty string
 * @param ...         Variable argument list with message
 * @return The CPL error code
 * @see cpl_error_set_message(), WCSLIB
 * @note This macro accesses
 *       extern const char *wcs_errmsg[];
 * If an internal inconsistency in WCSLIB causes a too large error number
 * to be returned, the behaviour is undefined (e.g. a segfault).
 *
 * Example of usage:
 *  @code
 *
 *  #ifdef CPL_WCS_INSTALLED
 *
 *  #include <wcslib.h>
 *
 *  const int wcserror = wcspih(shdr,np,0,0,&nrej,&nwcs,&wwcs);
 *
 *  if (wcserror) {
 *     return cpl_error_set_wcs(CPL_ERROR_ILLEGAL_INPUT, wcserror,
 *                               "wcspih", "np=%d", np);
 *  }
 *
 *  #endif
 *
 * @endcode
 *
 * @hideinitializer
 */
#define cpl_error_set_wcs(code, wcscode, wcsfunction, ...)              \
    cpl_error_set_wcs_macro(cpl_func, code, wcscode, wcsfunction,       \
                            wcs_errmsg, __FILE__, __LINE__, __VA_ARGS__)

struct _cpl_error_ {
    cpl_error_code  code;
    unsigned        line;
    char            function[MAX_NAME_LENGTH+1];
    char            file[MAX_FILE_LENGTH+1];
    char            msg[CPL_ERROR_MAX_MESSAGE_LENGTH];
};

/*-----------------------------------------------------------------------------
                       Prototypes of private functions 
 -----------------------------------------------------------------------------*/

cpl_error * cpl_errorstate_append(void);
const cpl_error * cpl_errorstate_find(void) CPL_ATTR_PURE;

cpl_boolean cpl_error_is_set(void) CPL_ATTR_PURE;
cpl_boolean cpl_error_is_readonly(void) CPL_ATTR_PURE;
void cpl_error_set_readonly(void);
void cpl_error_reset_readonly(void);

cpl_error_code
cpl_error_set_fits_macro(const char *, cpl_error_code, int,
                         const char *, const char *,
                         unsigned, const char *, ...) CPL_ATTR_PRINTF(7,8);

cpl_error_code
cpl_error_set_regex_macro(const char *, cpl_error_code, int,
                          const regex_t *, const char *,
                          unsigned, const char *, ...) CPL_ATTR_PRINTF(7,8);

cpl_error_code
cpl_error_set_wcs_macro(const char *, cpl_error_code, int,
                        const char *, const char *[], const char *,
                        unsigned, const char *, ...) CPL_ATTR_PRINTF(8,9);

#ifdef HAVE_LIBPTHREAD
void cpl_error_init_locks(void);
#endif

CPL_END_DECLS

#endif
/* end of cpl_error_impl.h */
