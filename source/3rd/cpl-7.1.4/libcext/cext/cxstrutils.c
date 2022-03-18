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

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "cxmemory.h"
#include "cxmessages.h"
#include "cxslist.h"
#include "cxstrutils.h"
#include "cxutils.h"


/**
 * @defgroup cxstrutils String Utility Functions
 *
 * The module implements various string-related utility functions suitable
 * for creating, searching and modifying C strings.
 *
 * @par Synopsis:
 * @code
 *   #include <cxstrutils.h>
 * @endcode
 */

/**@{*/

inline static cxchar *
_cx_strskip(const cxchar *string, int (*ctype)(int))
{

    cx_assert(string != NULL);
    cx_assert(ctype != NULL);

    while (*string && ctype((cxuchar)*string)) {
        string++;
    }

    return (cxchar *)string;

}


/*
 * Remove leading whitespace characters from a string.
 */

inline static cxchar *
_cx_strtrim(cxchar *string)
{

    register cxchar *t = string;

    if (!string) {
        return NULL;
    }

    if (!*string) {
        return string;
    }

    while (*t && isspace((cxuchar)*t)) {
        t++;
    }

    memmove(string, t, strlen((cxchar *)t) + 1);

    return string;

}


/*
 * Remove trailing whitespace characters from a string.
 */

inline static cxchar *
_cx_strrtrim(cxchar *string)
{

    register cxchar *t;


    if (!string) {
        return NULL;
    }

    if (!*string) {
        return string;
    }

    t = string + strlen(string) - 1;
    while (isspace((cxuchar)*t)) {
        t--;
    }
    *++t = '\0';

    return string;

}


/*
 * Clone a zero terminated C string.
 */

inline static cxchar *
_cx_strdup(const cxchar *string)
{

    cxchar *s;
    cxsize sz;

    if (string) {
        sz = strlen(string) + 1;
        s = cx_malloc(sz * sizeof(cxchar));
        memcpy(s, string, sz);
    }
    else {
        s = NULL;
    }

    return s;

}


/*
 * Copy a C string returning a pointer to the terminating zero.
 */

inline static cxchar *
_cx_stpcpy(cxchar *dest, const cxchar *src)
{

#ifdef HAVE_STPCPY

    if (dest == NULL || src == NULL) {
        return NULL;
    }

    return stpcpy(dest, src);

#else

    register cxchar *d = dest;
    register const cxchar *s = src;

    if (dest == NULL || src == NULL) {
        return NULL;
    }

    while ((*d++ = *s++)) {
        ;
    }

    return d - 1;

#endif

}


/**
 * @brief
 *   Compare two strings ignoring the case of ASCII characters.
 *
 * @param s1  First string.
 * @param s2  Second string.
 *
 * @return An integer less than, equal to, or greater than zero if @em s1
 *   is found, respectively, to be less than, to match, or be greater than
 *   @em s2.
 *
 * The function compares the two strings @em s1 and @em s2 as @b strcmp()
 * does, but ignores the case of ASCII characters.
 */

cxint
cx_strcasecmp(const cxchar *s1, const cxchar *s2)
{

    cxint c1, c2;


    cx_assert(s1 != NULL);
    cx_assert(s2 != NULL);

    while (*s1 && *s2) {
        c1 = tolower(*s1);
        c2 = tolower(*s2);

        if (c1 != c2)
            return c1 - c2;

        s1++;
        s2++;
    }

    return ((cxint) *s1 - (cxint) *s2);

}


/**
 * @brief
 *   Compare the first n characters of two strings ignoring the case of
 *   ASCII characters.
 *
 * @param s1  First string.
 * @param s2  Second string.
 * @param n   Number of characters to compare.
 *
 * @return An integer less than, equal to, or greater than zero if the first
 *   @em n characters of @em s1 are found, respectively, to be less than, to
 *   match, or be greater than the first @em n characters of @em s2.
 *
 * The function compares the first @em n characters of the two strings @em s1
 * and @em s2 as @b strncmp() does, but ignores the case of ASCII characters.
 */

cxint
cx_strncasecmp(const cxchar *s1, const cxchar *s2, cxsize n)
{

    cxint c1, c2;


    cx_assert(s1 != NULL);
    cx_assert(s2 != NULL);

    while (n && *s1 && *s2) {
        --n;

        c1 = tolower(*s1);
        c2 = tolower(*s2);

        if (c1 != c2)
            return c1 - c2;

        s1++;
        s2++;
    }

    if (!n)
        return 0;

    return ((cxint) *s1 - (cxint) *s2);

}


/**
 * @brief
 *   Test if a string represents an empty string.
 *
 * @param string   String to be tested.
 * @param pattern  String containing all allowed comment characters.
 *
 * @return The function returns 1 if the string is found to be empty or if
 *   the first non--whitespace character is one out of the set of provided
 *   comment characters. Otherwise the function returns 0.
 *
 * The function skips all leading whitespace characters in the string
 * @em string. Whitespace characters are recognized by @b isspace().
 * If the first character which is not a whitespace character is either
 * '\\0' or one out of the pattern string @em pattern, the string is
 * considered as empty and the function returns 1.
 *
 * If @em pattern is set to @c NULL there is no checking for special
 * characters that should be considered as whitespaces.
 */

cxint
cx_strempty(const cxchar *string, const cxchar *pattern)
{

    cx_assert(string != NULL);


    string = _cx_strskip(string, isspace);

    if (*string) {
        if (!pattern || !strchr(pattern, *string))
            return 0;
    }

    return 1;

}


/**
 * @brief
 *   Convert all uppercase characters in a string into lowercase
 *   characters.
 *
 * @param s  The string to convert.
 *
 * @return Returns a pointer to the converted string.
 *
 * Walks through the given string and turns uppercase characters into
 * lowercase characters using @b tolower().
 *
 * @see cx_strupper()
 */

cxchar *
cx_strlower(cxchar *s)
{

    cxchar *t = s;

    cx_assert(s != NULL);

    while (*t) {
        *t = tolower(*t);
        t++;
    }

    return s;

}


/**
 * @brief
 *   Convert all lowercase characters in a string into uppercase
 *   characters.
 *
 * @param s  The string to convert.
 *
 * @return Returns a pointer to the converted string.
 *
 * Walks through the given string and turns lowercase characters into
 * uppercase characters using @b toupper().
 *
 * @see strlower()
 */

cxchar *
cx_strupper(cxchar *s)
{

    cxchar *t = s;

    cx_assert(s != NULL);

    while (*t) {
        *t = toupper(*t);
        t++;
    }

    return s;

}


/**
 * @brief
 *   Remove leading whitespace characters from a string.
 *
 * @param string  String to be processed.
 *
 * @return The function returns a pointer to the modified string if no
 *   error occurred, otherwise @c NULL.
 *
 * The function removes leading whitespace characters, or from the string
 * @em string. Whitespace characters are recognized by @b isspace().
 */

cxchar *
cx_strtrim(cxchar *string)
{

    return _cx_strtrim(string);

}


/**
 * @brief
 *   Remove trailing whitespace characters from a string.
 *
 * @param string  String to be processed.
 *
 * @return The function returns a pointer to the modified string if no
 *   error occurred, otherwise @c NULL.
 *
 * The function removes trailing whitespace characters, or from the string
 * @em string. Whitespace characters are recognized by @b isspace().
 *
 */

cxchar *
cx_strrtrim(cxchar *string)
{

    return _cx_strrtrim(string);

}


/**
 * @brief
 *   Remove leading and trailing whitespace characters from a string.
 *
 * @return The function returns a pointer to the modified string if no
 *   error occurred, otherwise @c NULL.
 *
 * @param string  String to be processed.
 *
 * The function removes leading and trailing whitespace characters from
 * the string @em string. Whitespace characters are recognized
 * by @b isspace().
 */

cxchar *
cx_strstrip(cxchar *string)
{

    return _cx_strrtrim(_cx_strtrim(string));

}


/**
 * @brief
 *   Locate the first character in a string that does not belong to a
 *   given character class.
 *
 * @param string  String to be processed.
 * @param ctype   Character class test function.
 *
 * @return Pointer to the first character that is not a member of
 *   the character class described by @em ctype.
 *
 * Searches the string @em string for the first occurence of a character
 * which does not belong to a certain character class. The character
 * class is represented through a function that returns a non zero value
 * if a character belongs to that class and 0 otherwise. Such functions
 * are the character classification routines like @b isspace() for
 * instance. It is expected that the input string is properly terminated.
 * In case the whole string consists of characters of the specified class
 * the function will return the location of the terminating '\\0'.
 */

cxchar *
cx_strskip(const cxchar *string, int (*ctype)(int))
{

    if (!string) {
        return NULL;
    }

    return _cx_strskip(string, ctype);

}


/**
 * @brief
 *   Duplicate a string.
 *
 * @param string  String to be duplicated.
 *
 * @return Newly allocated copy of the original string.
 *
 * Duplicates the input string @em string. The newly allocated copy returned
 * to the caller can be deallocated using @b cx_free().
 */

cxchar *
cx_strdup(const cxchar *string)
{

    return _cx_strdup(string);

}


/**
 * @brief
 *   Duplicate the first n charactes of a string.
 *
 * @param string  Source string
 * @param n       Maximum number of characters to be duplicated.
 *
 * @return Newly allocated copy of the first @em n characters of @em string.
 *
 * Duplicates the first @em n characters of the source string @em string,
 * returning the copied characters in newly allocated string of the size
 * @em n + 1. The returned string is always null terminated. If the length
 * of @em string is less than @em n the returned string is padded with nulls.
 * The newly allocated string can be deallocated using @b cx_free().
 */

cxchar *
cx_strndup(const cxchar *string, cxsize n)
{

    cxchar *s;

    if (string) {
        s = cx_calloc(n + 1, sizeof(cxchar));
        memcpy(s, string, n);
        s[n] = '\0';
    }
    else
        s = NULL;

    return s;

}


/**
 * @brief
 *   Create a string from a variable-length argument list under format
 *   control.
 *
 * @param format  The format string.
 * @param args    Variable-length arguments to be inserted into @em format.
 *
 * @return An newly allocated string containing the formatted result.
 *
 * The function is similar to @b vsprintf() but calculates the size needed
 * to store the formatted result string and allocates the memory. The
 * newly allocated string can be deallocated using @b cx_free().
 */

cxchar *
cx_strvdupf(const cxchar *format, va_list args)
{

    cxchar *buffer;


    if (format == NULL)
        return NULL;

    cx_vasprintf(&buffer, format, args);

    return buffer;

}


/**
 * @brief
 *   Copy a string returning a pointer to its end.
 *
 * @param dest  Destination string.
 * @param src   Source string.
 *
 * @return Pointer to the terminating '\\0' of the concatenated string.
 *
 * The function copies the string @em src, including its terminating '\\0',
 * to the string @em dest. The source and the destination string may not
 * overlap and the destination buffer must be large enough to receive the
 * copy.
 */

cxchar *
cx_stpcpy(cxchar *dest, const cxchar *src)
{

    return _cx_stpcpy(dest, src);

}


/**
 * @brief
 *   Deallocate a @c NULL terminated string array.
 *
 * @param sarray  String array to deallocate
 *
 * @return Nothing.
 *
 * The function deallocates the array of strings @em sarray and any string
 * it possibly contains.
 */

void
cx_strfreev(cxchar **sarray)
{

    if (sarray) {
        register int i = 0;

        while (sarray[i]) {
            cx_free(sarray[i]);
            i++;
        }

        cx_free(sarray);
    }

    return;

}


/**
 * @brief
 *   Split a string into pieces at a given delimiter.
 *
 * @param string      The string to split.
 * @param delimiter   String specifying the locations where to split.
 * @param max_tokens  The maximum number of tokens the string is split into.
 *
 * @return The function returns a newly allocated, @c NULL terminated
 *   array of strings, or @c NULL in case of an error.
 *
 * The function breaks up the string @em string into, at most, @em max_tokens
 * pieces at the places indicated by @em delimiter. If @em max_tokens is
 * reached, the remainder of the string is appended to the last token. If
 * @em max_tokens is less than 1 the string @em string is split completely.
 *
 * The delimiter string @em delimiter never shows up in any of the resulting
 * strings, unless @em max_tokens is reached.
 *
 * As a special case, the result of splitting the empty string "" is an empty
 * vector, not a vector containing a single string.
 *
 * The created result vector can be deallocated using @b cx_strfreev().
 */

cxchar **
cx_strsplit(const cxchar *string, const cxchar *delimiter, cxint max_tokens)
{

    cxchar **sarray, *s;
    cxuint n = 0;

    cx_slist *slist = cx_slist_new();
    cx_slist_iterator sl;

    const cxchar *remainder = string;


    if (!string || !delimiter)
        return NULL;

    if (*delimiter == '\0')
        return NULL;

    if (max_tokens < 1)
        max_tokens = CX_MAXINT;

    s = strstr(remainder, delimiter);
    if (s) {
        cxsize sz = strlen(delimiter);

        while (--max_tokens && s) {
            cxsize length;
            cxchar *token;

            length = s - remainder;

            token = cx_malloc((length + 1) * sizeof(cxchar));
            strncpy(token, remainder, length);
            token[length] = '\0';

            cx_slist_push_front(slist, token);
            n++;

            remainder = s + sz;
            s = strstr(remainder, delimiter);
        }
    }

    if (*string) {
        n++;
        cx_slist_push_front(slist, _cx_strdup(remainder));
    }

    sarray = cx_malloc((n + 1) * sizeof(cxchar *));
    sarray[n--] = NULL;

    sl = cx_slist_begin(slist);
    while (sl != cx_slist_end(slist)) {
        sarray[n--] = cx_slist_get(slist, sl);
        sl = cx_slist_next(slist, sl);
    }

    cx_slist_delete(slist);

    return sarray;

}

/**
 * @brief
 *   Join strings from an array of strings.
 *
 * @param separator  Optional separator string.
 * @param sarray     Array of strings to join.
 *
 * @return A newly allocated string containing the joined input strings
 *   separated by @em separator, or @c NULL in case of error.
 *
 * The function builds a single string from the strings referenced by
 * @em sarray. The array of input strings @em sarray has to be @c NULL
 * terminated. Optionally, a separator string can be passed through
 * @em separator which will then be inserted between two strings. If no
 * separator should be inserted when joining, @em separator must be set
 * to @c NULL.
 */

cxchar *
cx_strjoinv(const cxchar *separator, cxchar **sarray)
{

    cxchar *string, *s;


    if (!sarray)
        return NULL;

    if (separator == NULL)
        separator = "";

    if (*sarray) {
        cxint i;
        cxsize sz;

        /*
         * Compute the required size of the result string
         */

        sz = 1 + strlen(sarray[0]);

        i = 1;
        while (sarray[i]) {
            sz += strlen(sarray[i]);
            ++i;
        }

        sz += strlen(separator) * (i - 1);


        /*
         * Join the elements
         */

        string = cx_malloc(sz * sizeof(cxchar));
        s = _cx_stpcpy(string, sarray[0]);

        i = 1;
        while (sarray[i]) {
            s = _cx_stpcpy(s, separator);
            s = _cx_stpcpy(s, sarray[i]);
            ++i;
        }
    }
    else {
        string = _cx_strdup("");
    }

    return string;

}
/**@}*/
