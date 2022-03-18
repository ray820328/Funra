/*
 * This file is part of the ESO C Extension Library
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
#  include <config.h>
#endif

#include <stdarg.h>
#if defined(HAVE_VSNPRINTF_C99) || defined(HAVE_VASPRINTF)
#  include <stdio.h>
#endif
#ifdef HAVE_VASPRINTF
#  include <stdlib.h>
#endif
#if ! defined(HAVE_VA_COPY_STYLE_FUNCTION) && ! defined(HAVE_VA_LIST_COPY_BY_VALUE)
#  include <string.h>
#endif
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#ifdef HAVE_LIMITS_H
#  include <limits.h>
#endif
#include <errno.h>

#include "cxthread.h"
#include "cxmemory.h"
#include "cxmessages.h"
#include "cxstrutils.h"
#include "cxutils.h"

#ifndef HAVE_VSNPRINTF_C99
#  include "snprintf.h"
#endif

#ifndef HAVE_VA_COPY_STYLE_FUNCTION
#  if defined(__GNUC__) && defined(__PPC__) && defined(_CALL_SYSV)
#    define CX_VA_COPY(ap1, ap2)  (*(ap1) = *(ap2))
#  elif !defined(HAVE_VA_LIST_COPY_BY_VALUE)
#    define CX_VA_COPY(ap1, ap2)  memmove((ap1), (ap2), sizeof(va_list))
#  else
#    define CX_VA_COPY(ap1, ap2)  ((ap1) = (ap2))
#  endif
#endif


/**
 * @defgroup cxutils Miscellaneous Utilities
 *
 * The module provides a portable implementation of a selection of
 * miscellaneous utility functions.
 *
 * @par Synopsis:
 * @code
 *   #include <cxutils.h>
 * @endcode
 */

/**@{*/

/*
 * Identifier for applications
 */

static cxchar *cx_program_name = NULL;

CX_LOCK_DEFINE_INITIALIZED_STATIC(cx_program_name);


/**
 * @brief
 *   Get the name of the application.
 *
 * @return The program's name string.
 *
 * The program's name is retrieved and returned to the caller. The returned
 * pointer is a library resource and must not be freed or modified.
 */

const cxchar *
cx_program_get_name(void)
{

    const cxchar* name = NULL;

    CX_LOCK(cx_program_name);
    name = cx_program_name;
    CX_UNLOCK(cx_program_name);

    return name;

}


/**
 * @brief
 *   Set the name of the application.
 *
 * @param name  The program name.
 *
 * @return Nothing.
 *
 * The program's name is set to the string @em name.
 *
 * @attention For thread-safety reasons this function may be called only once!
 */

void
cx_program_set_name(const cxchar *name)
{

    CX_LOCK(cx_program_name);
    cx_free(cx_program_name);
    cx_program_name = cx_strdup(name);
    CX_UNLOCK(cx_program_name);

    return;

}


/**
 * @brief
 *   Get the position of the first bit set, searching from left to right.
 *
 * @param mask   A 32 bit integer containing bit flags.
 * @param start  Bit position where the search starts.
 *
 * @return The bit position of the first bit set which is lower than
 *   @em start. If no bit is set -1 is returned.
 *
 * The function searches for the first bit set in @em mask, starting at
 * the bit position @em start - 1. The bit mask @em mask is searched from
 * left to right. If @em start is less than 0 or bigger than 32 the search
 * starts at the 31st bit.
 *
 * @see cx_bits_rfind()
 */

cxint cx_bits_find(cxuint32 mask, cxint start)
{
    register cxint n = (cxint)(sizeof(cxuint32) * 8);


    if (start < 0 || start > n)
        start = n;

    while (start > 0) {
        start--;
        if (mask & (1 << (cxuint32)start))
            return start;
    }

    return -1;
}


/**
 * @brief
 *   Get the position of the first bit set, searching from right to left.
 *
 * @param mask   A 32 bit integer containing bit flags.
 * @param start  Bit position where the search starts.
 *
 * @return The bit position of the first bit set which is higher than
 *   @em start. If no bit is set -1 is returned.
 *
 * The function searches for the first bit set in @em mask, starting at
 * the bit position @em start + 1. The bit mask @em mask is searched from
 * right to left. If @em start is less than 0 the search starts at the 1st
 * bit.
 *
 * @see cx_bits_find()
 */

cxint cx_bits_rfind(cxuint32 mask, cxint start)
{

    register cxint n = (cxint)(sizeof(cxuint32) * 8) - 1;

    if (start < 0)
        start = -1;

    while (start < n) {
        start++;
        if (mask & (1 << (cxuint32)start))
            return start;
    }

    return -1;
}


/**
 * @brief
 *   Safe version of @b sprintf().
 *
 * @param string  Destination string.
 * @param n       Maximum number of characters to be written.
 * @param format  The format string.
 * @param ...     Arguments to be inserted into the format string.
 *
 * @return The number of characters (excluding the trailing null) which
 *   would have been written to the destination string if enough space
 *   had been available, i.e. if the destination string is large enough
 *   the function returns the number of characters written to the string.
 *
 * The function is a safe form of @b sprintf(). It writes output to
 * the string @em string, under the control of the format string @em format.
 * The format string specifies how the arguments are formatted for output.
 * All standard C conversion directives are supported.
 *
 * The difference compared to @b sprintf() is that the produced number of
 * characters does not exceed @em n (including the trailing null).
 *
 * @note
 *   The return value of @b cx_snprintf() conforms to the @b snprintf()
 *   function as standardized in ISO C99. This might be different from
 *   traditional implementations.
 *
 * @see cx_asprintf(), cx_strdupf()
 */

cxint cx_snprintf(cxchar *string, cxsize n, const cxchar *format, ...)
{

    va_list args;
    cxint nc;

    va_start(args, format);
    nc = cx_vsnprintf(string, n, format, args);
    va_end(args);

    return nc;

}

/**
 * @brief
 *   Safe version of @b vsprintf().
 *
 * @param string  Destination string.
 * @param n       Maximum number of characters to be written.
 * @param format  The format string.
 * @param args    List of arguments to be inserted into the format string.
 *
 * @return The number of characters (excluding the trailing null) which
 *   would have been written to the destination string if enough space
 *   had been available, i.e. if the destination string is large enough
 *   the function returns the number of characters written to the string.
 *
 * The function is a safe form of @b vsprintf(). It writes output to
 * the string @em string, under the control of the format string @em format.
 * The format string specifies how the arguments, provided through the
 * variable-length argument list @em args, are formatted for output.
 * All standard C conversion directives are supported.
 *
 * The difference compared to @b vsprintf() is that the produced number of
 * characters does not exceed @em n (including the trailing null).
 *
 * @note
 *   The return value of @b cx_vsnprintf() conforms to the @b vsnprintf()
 *   function as standardized in ISO C99. This might be different from
 *   traditional implementations.
 *
 * @see cx_vasprintf(), cx_strvdupf()
 */

cxint cx_vsnprintf(cxchar *string, cxsize n, const cxchar *format,
                   va_list args)
{

    if (n != 0 && string == NULL)
        return 0;

    if (format == NULL)
        return 0;


    /*
     * No availablility checks needed here. If the system does not provide
     * a vsnprintf() function with C99 semantics a local implementation
     * is used (see inclusion of snprintf.h at the beginning of this file),
     * i.e. vsnprintf will always be available. For further details check
     * the implementation itself in snprintf.c.
     */

    return vsnprintf(string, n, format, args);

}


/**
 * @brief
 *   Write formatted output to a newly allocated string.
 *
 * @param string  Address where the allocated string is stored.
 * @param format  The format string.
 * @param ...     Arguments to be inserted into the format string.
 *
 * @return The number of characters (excluding the trailing null) written to
 *   allocated string, i.e. its length. If sufficient space cannot be
 *   allocated, -1 is returned.
 *
 * The function is similar to @b cx_snprintf() or @b sprintf(). The difference
 * to @b cx_snprintf() is that the output created from the format string
 * @em format and the formatted arguments is placed into a string which is
 * allocated using @b cx_malloc(). All standard C conversion directives are
 * supported. The allocated string is always null terminated.
 *
 * The pointer to the allocated string buffer sufficiently large to hold
 * the string is returned to the caller in the @em string argument. This
 * pointer should be passed to @b cx_free to release the allocated storage
 * when it is no longer needed. If sufficient memory cannot be allocated
 * @em is set to @c NULL.
 *
 * @see cx_snprintf(), cx_strdupf(), cx_malloc(), cx_free()
 */

cxint cx_asprintf(cxchar **string, const cxchar *format, ...)
{

    va_list args;
    cxint nc;

    va_start(args, format);
    nc = cx_vasprintf(string, format, args);
    va_end(args);

    return nc;

}


/**
 * @brief
 *   Write formatted output to a newly allocated string with a
 *   variable-length argument list.
 *
 * @param string  Address where the allocated string is stored.
 * @param format  The format string.
 * @param args    List of arguments to be inserted into the format string.
 *
 * @return The number of characters (excluding the trailing null) written to
 *   allocated string, i.e. its length. If sufficient space cannot be
 *   allocated, -1 is returned.
 *
 * The function is similar to @b cx_vsnprintf() or @b vsprintf(). The
 * difference to @b cx_vsnprintf() is that the output, created from the
 * format string @em format and the arguments given by the variable-length
 * argument list @em args, is placed into a string which is allocated using
 * @b cx_malloc(). All standard C conversion directives are supported. The
 * allocated string is always null terminated.
 *
 * The pointer to the allocated string buffer sufficiently large to hold
 * the string is returned to the caller in the @em string argument. This
 * pointer should be passed to @b cx_free to release the allocated storage
 * when it is no longer needed. If sufficient memory cannot be allocated
 * @em is set to @c NULL.
 *
 * @see cx_vsnprintf(), cx_strvdupf(), cx_malloc(), cx_free()
 */

cxint cx_vasprintf(cxchar **string, const cxchar *format, va_list args)
{

    cxint nc;


    if (format == NULL)
        return 0;
    else {

#ifdef HAVE_VASPRINTF

        if (!cx_memory_is_system_malloc()) {

            cxchar *buffer;

            nc = vasprintf(&buffer, format, args);
            *string = cx_strdup(buffer);

            /*
             * vasprintf() uses system services for the buffer allocation,
             * therefore it has to be released again with the system free()
             */

            free(buffer);

        }
        else {
            nc = vasprintf(string, format, args);
        }

#else /* !HAVE_VASPRINTF */

        va_list args2;


        *string = NULL;


        /*
         * Just get the required buffer size. Don't consume the original
         * args, we'll need it again.
         */

        CX_VA_COPY(args2, args);
        nc = cx_vsnprintf(NULL, 0, format, args2);

#ifdef HAVE_VA_COPY_STYLE_FUNCTION
        va_end(args2);  /* Only if va_copy() or __va_copy() was used */
#endif

        cx_assert(nc >= -1);    /* possible integer overflow if nc > INT_MAX */

        if (nc >= 0) {

            *string = (cxchar *)cx_malloc((cxsize)nc + 1);
            if (*string == NULL) {
                nc = -1;
            }
            else {
                int nc2 = cx_vsnprintf(*string, nc + 1, format, args);
               (void) nc2;  /* Prevent warnings if cx_assert is disabled. */
                cx_assert(nc2 == nc);
            }

        }

#endif /* !HAVE_VASPRINTF */

    }

    return nc;

}


/**
 * @brief
 *   Get the maximum length of a line supported by the system.
 *
 * @return The length of a line including the trailing zero.
 *
 * The function uses the @b sysconf() function to determine the maximum
 * length of a line buffer that is supported by the system and available
 * for utility programs. If the @b sysconf() facility is not available
 * the function returns a guessed value of 4096 characters as the maximum
 * length of a line taking into account the trailing zero.
 */

cxlong
cx_line_max(void)
{

    cxlong sz = 4096;  /* Guessed default */

#if defined(HAVE_SYSCONF) && defined(_SC_LINE_MAX)
    sz = sysconf(_SC_LINE_MAX);
#endif

    return sz;

}


/**
 * @brief
 *   Allocate a line buffer with the maximum size supported by the system.
 *
 * @return A pointer allocated memory.
 *
 * The function creates a line buffer with the maximum length supported
 * by the system. The size of the buffer is determined calling
 * @b cx_line_max() which gives the maximum size including the
 * trailing zero.
 */

cxchar *
cx_line_alloc(void)
{

    cxlong sz = cx_line_max();

    return (cxchar *)cx_calloc(sz, sizeof(cxchar));

}
/**@}*/
