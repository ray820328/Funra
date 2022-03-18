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

#include "cxmemory.h"
#include "cxstrutils.h"
#include "cxstring.h"



/**
 * @defgroup cxstring Strings
 *
 * A @b cx_string is similar to a standard C string, except that it grows
 * automatically as text is appended or inserted. The character data the
 * string contains is '\\0' terminated in order to guarantee full
 * compatibility with string utility functions processing standard C strings.
 * Together with the character data it also stores the length of the string.
 */

/**@{*/

/*
 * Forward declaration. Needed because of attributes specification.
 */

inline static cxint
_cx_string_vsprintf(cx_string *, const cxchar *, va_list) CX_GNUC_PRINTF(2, 0);



/*
 * Create a new, empty string
 */

inline static cx_string *
_cx_string_new(void)
{

    cx_string *self = (cx_string *)cx_malloc(sizeof(cx_string));

    self->data = NULL;
    self->sz = 0;

    return self;

}


/*
 * Clear the contents of a string.
 */

inline static void
_cx_string_clear(cx_string *self)
{

    if (self->data != NULL) {
        cx_free(self->data);
        self->data = NULL;
    }

    self->sz = 0;

    return;

}


/*
 * Sets a string's value from a character array.
 */

inline static void
_cx_string_set(cx_string *self, const cxchar *data)
{

    cx_assert(self != NULL);

    if (self->data != NULL) {
        cx_free(self->data);
        self->data = NULL;
        self->sz = 0;
    }

    if (data) {
        self->data = cx_strdup(data);
        cx_assert(self->data != NULL);

        self->sz = strlen(self->data);
    }

    return;

}


/*
 * Gets a string's value
 */

inline static const cxchar *
_cx_string_get(const cx_string *self)
{

    return self->data;

}


/*
 * Write to a string under format control
 */

inline static cxint
_cx_string_vsprintf(cx_string *string, const cxchar *format, va_list args)
{

    cxint n;
    cxchar *tmp = NULL;

    n = cx_vasprintf(&tmp, format, args);

    if (tmp) {
        if (string->data != NULL) {
            cx_free(string->data);
        }

        string->data = tmp;
        string->sz = (cxsize) strlen(tmp);
    }

    return n;

}


/*
 * Remove leading whitespaces
 */

inline static cx_string *
_cx_string_trim(cx_string *self)
{

    cxchar *tmp = NULL;
    cxsize  i = 0;


    cx_assert(self != NULL);

    tmp = self->data;

    while (*tmp && isspace((cxint)*tmp)) {
        tmp++;
        i++;
    }

    memmove(self->data, tmp, strlen((cxchar*)tmp) + 1);
    self->sz -= i;

    return self;

}


/*
 * Remove trailing whitespaces
 */

inline static cx_string *
_cx_string_rtrim(cx_string *self)
{

    cxchar *tmp = NULL;
    cxsize  i = 0;


    cx_assert(self != NULL);

    tmp = self->data + self->sz - 1;

    while (isspace((cxint)*tmp)) {
        tmp--;
        i++;
    }

    *++tmp = (cxchar) '\0';
    self->sz -= i;

    return self;

}


/**
 * @brief
 *   Create a new, empty string container
 *
 * @return
 *   Pointer to newly created string.
 *
 * The function allocates memory for a new string object and initializes it
 * to represent the empty string.
 *
 * Using this constructor is the @b only way to correctly create and setup
 * a new string.
 */

cx_string *
cx_string_new(void)
{

    return _cx_string_new();

}


/**
 * @brief
 *   Create a copy a cx_string.
 *
 * @param self  The string to copy.
 *
 * @return Pointer to the newly created copy of @em string.
 */

cx_string *
cx_string_copy(const cx_string *self)
{

    cx_string *tmp;

    if (self == NULL)
        return NULL;

    tmp = _cx_string_new();
    _cx_string_set(tmp, _cx_string_get(self));

    return tmp;
}


/**
 * @brief
 *   Create a new string from a standard C string.
 *
 * @param value  The initial text to copy into the string.
 *
 * @return Pointer to newly created string.
 *
 * A new string is created and the text @em value is initially copied
 * into the string.
 */

cx_string *
cx_string_create(const cxchar *value)
{

    cx_string *self;


    self = _cx_string_new();
    _cx_string_set(self, value);

    return self;

}


/**
 * @brief
 *   Destroy a string.
 *
 * @param self  The string to destroy.
 *
 * @return Nothing.
 *
 * The function deallocates @em string's character buffer and finally
 * frees the memory allocated for @em string itself.
 */

void
cx_string_delete(cx_string *self)
{

    if (self == NULL) {
        return;
    }

    _cx_string_clear(self);
    cx_free(self);

    return;

}


/**
 * @brief
 *   Computes the length of the string.
 *
 * @param self  The string.
 *
 * @return The string's length, or 0 for uninitialized or empty strings.
 *
 * Computes the length of the string.
 */

cxsize
cx_string_size(const cx_string *self)
{

    cx_assert(self != NULL);
    return self->sz;

}


/**
 * @brief
 *   Checks whether a string contains any characters.
 *
 * @param self  The string.
 *
 * @return The function returns TRUE if the string is empty, or FALSE
 *   otherwise.
 *
 * A string is considered to be empty if its size is 0 or if it has not
 * been initialized, i.e. no value has been assigned yet.
 */

cxbool
cx_string_empty(const cx_string *self)
{

    cx_assert(self != NULL);

    if (self->data == NULL)
        return TRUE;

    return self->sz == 0 ? TRUE : FALSE;
}


/**
 * @brief
 *   Assign a value to a string.
 *
 * @param self  The string.
 * @param data  Character array to be assigned.
 *
 * @return Nothing.
 *
 * Stores the contents of the character array pointed to by @em data
 * into the string.
 */

void
cx_string_set(cx_string *self, const cxchar *data)
{

    if (self == NULL) {
        return;
    }

    _cx_string_set(self, data);
    return;

}


/**
 * @brief
 *   Get the string's value.
 *
 * @param self  The string.
 *
 * @return A constant pointer to the string's @em data member, or @c NULL
 *   if the string is uninitialized.
 *
 * A pointer to the strings character data. The character array pointed
 * to by this pointer is an standard C string, i.e. '\\0' terminated and
 * can be used together with any string processing function from the
 * standard C library (but see below).
 *
 * @warning
 *   The string's data @b must @b not be modified using the returned
 *   pointer, otherwise the internal consistency may be lost.
 */

const cxchar *
cx_string_get(const cx_string *self)
{

    if (self == NULL) {
        return NULL;
    }

    return _cx_string_get(self);

}


/**
 * @brief
 *   Converts the string into uppercase.
 *
 * @param self  The string.
 *
 * @return The passed string @em self with all characters converted to
 *   uppercase, or @c NULL in case of errors.
 *
 * All lowercase letters stored in the string are converted to uppercase
 * letters. The conversion is done using the standard C function
 * @b toupper().
 */

cx_string *
cx_string_upper(cx_string *self)
{

    cxsize position = 0;

    if (self == NULL || self->data == NULL) {
        return NULL;
    }

    for (position = 0; position < self->sz; position++) {
        self->data[position] = toupper(self->data[position]);
    }

    self->data[self->sz] = '\0';

    return self;

}


/**
 * @brief
 *   Converts the string into lowercase.
 *
 * @param self  The string.
 *
 * @return The passed string @em self with all characters converted to
 *   lowercase, or @c NULL in case of errors.
 *
 * All uppercase letters stored in the string are converted to lowercase
 * letters. The conversion is done using the standard C function
 * @b tolower().
 */

cx_string *
cx_string_lower(cx_string *self)
{

    cxsize position = 0;

    if (self == NULL || self->data == NULL) {
        return NULL;
    }

    for (position = 0; position < self->sz; position++) {
        self->data[position] = tolower(self->data[position]);
    }

    self->data[self->sz] = '\0';

    return self;

}


/**
 * @brief
 *   Remove leading whitespaces from the string.
 *
 * @param self  The string.
 *
 * @return The passed string @em self with leading whitespaces removed,
 *   or @c NULL in case of errors.
 *
 * The function removes leading whitespace characters from the string.
 * Whitespace characters are recognized by the standard C function
 * @b isspace().
 */

cx_string *
cx_string_trim(cx_string *self)
{

    if (self == NULL || self->data == NULL) {
        return NULL;
    }

    return _cx_string_trim(self);

}


/**
 * @brief
 *   Remove trailing whitespaces from the string.
 *
 * @param self  The string.
 *
 * @return The passed string @em self with trailing whitespaces revoved,
 *   or @c NULL in case of errors.
 *
 * The function removes trailing whitespace characters from the string.
 * Whitespace characters are recognized by the standard C function
 * @b isspace().
 */

cx_string *
cx_string_rtrim(cx_string *self)
{

    if (self == NULL || self->data == NULL) {
        return NULL;
    }

    return _cx_string_rtrim(self);

}


/**
 * @brief
 *   Remove leading and trailing whitespaces from the string.
 *
 * @param self  The string.
 *
 * @return The passed string @em self with leading and trailing whitespaces
 *   removed, or @c NULL in case of errors.
 *
 * The function removes leading and trailing whitespace characters
 * from the string. Whitespace characters are recognized by the
 * standard C function @b isspace().
 */

cx_string *
cx_string_strip(cx_string *self)
{

    if (self == NULL || self->data == NULL) {
        return NULL;
    }

    return _cx_string_rtrim(_cx_string_trim(self));

}


/**
 * @brief
 *   Prepend an array of characters to the string.
 *
 * @param self  The string.
 * @param data  Pointer to character array to be prepended.
 *
 * @return The passed string @em self with the characters prepended,
 *   or @c NULL in case of errors.
 *
 * The function adds the contents of the character buffer @em data to
 * the beginning of the string. If @em data is a @c NULL pointer the
 * string @em self is not modified.
 */

cx_string *
cx_string_prepend(cx_string *self, const cxchar *data)
{

    if (self == NULL) {
        return NULL;
    }

    if (data) {
        cxchar *tmp = NULL;

        cxsize ts = 0;
        cxsize lv = 0;


        lv = (cxsize) strlen(data);
        ts = self->sz + lv;

        tmp = (cxchar *) cx_malloc(sizeof(cxchar) * (ts + 1));

        strncpy(tmp, data, lv);
        strncpy(tmp + lv, self->data, self->sz);

        if (self->data != NULL) {
            cx_free(self->data);
        }

        self->data = tmp;
        self->sz = ts;
        self->data[self->sz] = '\0';
    }

    return self;

}


/**
 * @brief
 *   Append an array of characters to the string.
 *
 * @param self  The string.
 * @param data  Pointer to character array to be appended.
 *
 * @return The passed string @em self with the characters appended,
 *   or @c NULL in case of errors.
 *
 * The function adds the contents of the character buffer @em data to
 * the end of the string. If @em data is a @c NULL pointer the string
 * @em self is not modified.
 */

cx_string *
cx_string_append(cx_string *self, const cxchar *data)
{

    if (self == NULL) {
        return NULL;
    }

    if (data && *data != '\0') {
        cxchar *tmp = NULL;

        cxsize ts  = 0;
        cxsize lv  = 0;


        lv = (cxsize) strlen(data);
        ts = self->sz + lv;

        tmp = (cxchar*) cx_malloc(sizeof(cxchar) * (ts + 1));

        strncpy(tmp, self->data, self->sz);
        strncpy(tmp + self->sz, data, lv);

        if (self->data != NULL)
            cx_free(self->data);

        self->data = tmp;
        self->sz = ts;
        self->data[self->sz] = '\0';
    }

    return self;

}


/**
 * @brief
 *   Inserts a copy of a string at a given position.
 *
 * @param self      The string.
 * @param position  Character position at which the data is inserted.
 * @param data      Pointer to character array to be inserted.
 *
 * @return The passed string @em self with the characters inserted,
 *   or @c NULL in case of errors.
 *
 * The function inserts the contents of the character buffer @em data at
 * the character index @em position into the string, expanding the string
 * if necessary. Character positions start counting from 0. If @em data is
 * a @c NULL pointer the string @em self is not modified.
 */

cx_string *
cx_string_insert(cx_string *self, cxssize position, const cxchar *data)
{

    if (self == NULL) {
        return NULL;
    }

    if (position < 0 || position > (cxssize)self->sz) {
        return NULL;
    }

    if (data) {
        cxchar *tmp = NULL;
        cxsize vl = (cxsize)strlen(data);

        tmp = (cxchar *)cx_malloc(sizeof(cxchar) * (self->sz + vl + 1));

        strncpy(tmp, self->data, position);
        strncpy(tmp + position, data, vl);
        strncpy(tmp + position + vl, self->data + position,
                self->sz - position);

        if (self->data != NULL) {
            cx_free(self->data);
        }

        self->data = tmp;
        self->sz += vl;
        self->data[self->sz] = '\0';
    }

    return self;

}


/**
 * @brief
 *   Erase a portion of the string.
 *
 * @param self      The string.
 * @param position  Position of the first character to be erased.
 * @param length    Number of characters to erase.
 *
 * @return The passed string @em self with the characters removed, or
 *   @c NULL in case of errors.
 *
 * The function removes @em length characters from the string starting
 * at the character index @em position. The number of characters to be
 * removed is inclusive the character at index @em position.
 * The characters following the removed portion are shifted to fill
 * the gap. Character positions start counting from 0.
 *
 * If the number of characters to erase @em length is less the @c 0 all
 * characters starting at @em position up to the end of the string are
 * erased.
 */

cx_string *
cx_string_erase(cx_string *self, cxssize position, cxssize length)
{

    cxchar *tmp = NULL;


    if (self == NULL) {
        return NULL;
    }

    if (position < 0 || position > (cxssize)self->sz) {
        return NULL;
    }

    if (length < 0) {
        length = self->sz - position;
    }
    else {
        if (position + length > (cxssize)self->sz) {
            return NULL;
        }
    }

    if (position + length < (cxssize)self->sz) {
        tmp = (cxchar *) cx_malloc(sizeof(cxchar) * (self->sz - length + 1));

        strncpy(tmp, self->data, position);
        strncpy(tmp + position, self->data + position + length,
                self->sz - (position + length));
        
        if (self->data != NULL) {
            cx_free(self->data);
        }
    
        self->data = tmp;
    }

    self->sz -= length;
    
    if (self->data != NULL) {
        self->data[self->sz] = '\0';
    }

    return self;

}


/**
 * @brief
 *   Truncate the string.
 *
 * @param self      The string.
 * @param length    The length to which the string is truncated.
 *
 * @return The truncated string @em self, or @c NULL in case of errors.
 *
 * The function removes all characters from the string starting at
 * the character index @em length up to the end of the string, effectively
 * truncating the string from its original size to a string of length
 * @em length.
 *
 * Calling the truncate method is equivalent to:
 * @code
 *   cx_string *s;
 *
 *   cx_string_erase(s, length, -1);
 * @endcode
 */

cx_string *
cx_string_truncate(cx_string *self, cxsize length)
{

    if (self == NULL) {
        return NULL;
    }

    self->sz = CX_MIN(length, self->sz);
    self->data[self->sz] = '\0';

    return self;

}


/**
 * @brief
 *   Compare two cx_string for equality.
 *
 * @param string1  First cx_string.
 * @param string2  Second cx_string.
 *
 * @return TRUE if equal, FALSE if not.
 *
 * The function checks whether two strings are equal. Two strings are equal
 * if their values match on a character by character basis.
 */

cxbool
cx_string_equal(const cx_string *string1, const cx_string *string2)
{

    cxchar *p, *q;

    cxsize  i = string1->sz;

    if (i != string2->sz) {
        return FALSE;
    }

    p = string1->data;
    q = string2->data;

    while (i) {
        if(*p != *q) {
            return FALSE;
        }

        p++;
        q++;
        i--;
    }

    return TRUE;

}


/**
 * @brief
 *   Compare two strings.
 *
 * @param string1  First cx_string.
 * @param string2  Second cx_string.
 *
 * @return The function returns an interger less than, equal to, or greater
 *   than @c 0 if @em string1 is found, respectively, to be less than, to
 *   match, or to be greater than @em string2.
 *
 * The function compares @em string2 with @em string in the same way as
 * the standard C function @b strcmp() does.
 */

cxint
cx_string_compare(const cx_string *string1, const cx_string *string2)
{

  return (cxint) strcmp(string1->data, string2->data);

}


/**
 * @brief
 *   Compare two strings ignoring the case of characters.
 *
 * @param string1  First cx_string.
 * @param string2  Second cx_string.
 *
 * @return The function returns an interger less than, equal to, or greater
 *   than @c 0 if @em string1 is found, respectively, to be less than, to
 *   match, or to be greater than @em string2.
 *
 * The function compares @em string2 with @em string in the same way as
 * the standard C function @b strcmp() does, but ignores the case of
 * ASCII characters.
 */

cxint
cx_string_casecmp(const cx_string *string1, const cx_string *string2)
{

    if (string1 == NULL) {
        return -1;
    }

    if (string2 == NULL) {
        return 1;
    }

    return cx_strcasecmp(string1->data, string2->data);

}


/**
 * @brief
 *   Compare the first n characters of two strings ignoring the case of
 *   characters.
 *
 * @param string1  First string.
 * @param string2  Second string.
 * @param n        Number of characters to compare.
 *
 * @return An integer less than, equal to, or greater than zero if the first
 *   @em n characters of @em string1 are found, respectively, to be less than,
 *   to match, or be greater than the first @em n characters of @em string2.
 *
 * The function compares the first @em n characters of the two strings
 * @em string1 and @em string2 as @b strncmp() does, but ignores the case of
 * ASCII characters.
 */

cxint
cx_string_ncasecmp(const cx_string *string1, const cx_string *string2,
                   cxsize n)
{

    if (string1 == NULL)
        return -1;

    if (string2 == NULL)
        return 1;

    return cx_strncasecmp(string1->data, string2->data, n);

}


/**
 * @brief
 *   Writes to a string under format control.
 *
 * @param self    The string to write to.
 * @param format  The format string.
 * @param ...     The arguments to insert into @em format.
 *
 * @return The number of characters (excluding the trailing null) written to
 *  @em self, i.e. its length. If sufficient space cannot be allocated, -1 is
 *  returned.
 *
 * The function writes the formatted character array to the string.
 * The function works similar to @b sprintf() function, except that
 * the string's buffer expands automatically to contain the formatted
 * result. The previous contents of the string is destroyed.
 */

cxint
cx_string_sprintf(cx_string *self, const char *format, ...)
{

    cxint n;

    va_list ap;


    cx_assert(self != NULL);
    cx_assert(format != NULL);

    va_start(ap, format);
    n = _cx_string_vsprintf(self, format, ap);
    va_end(ap);

    return n;

}


/**
 * @brief
 *   Write to the string from a variable-length argument list under
 *   format control.
 *
 * @param self    The string.
 * @param format  The format string.
 * @param args    Variable-length arguments to be inserted into @em format.
 *
 * @return The number of characters (excluding the trailing null) written to
 *  @em self, i.e. its length. If sufficient space cannot be allocated, -1 is
 *  returned.
 *
 * The function writes the formatted character array to the string.
 * The function works similar to @b vsprintf() function, except that
 * the string's buffer expands automatically to contain the formatted
 * result. The previous contents of the string is destroyed.
 */

cxint
cx_string_vsprintf(cx_string *self, const cxchar *format, va_list args)
{

    cx_assert(self != NULL);
    cx_assert(format != NULL);

    return _cx_string_vsprintf(self, format, args);

}


/**
 * @brief
 *   Print the value of a cx_string to the standard output.
 *
 * @param string  A cx_string.
 *
 * This function is provided for debugging purposes. It just writes the
 * strings contents to the standard output using @b cx_print().
 */

void
cx_string_print(const cx_string *string)
{

    if (string == NULL)
        return;

    cx_print("%s", _cx_string_get(string));

}


/**
 * @brief
 *   Replace a given character with a new character in a portion of a string.
 *
 * @param self      A cx_string.
 * @param start     The initial position of the range.
 * @param end       The final position of the range.
 * @param old_value The character to be replaced.
 * @param new_value The character used as replacement.
 *
 * @return Nothing.
 *
 * The function replaces the all occurrences of the character @em old_value
 * with the character @em new_value in the given range. The range of
 * characters considered is given by the initial position @em start and the
 * final position @em end, and does not include the final position, i.e.
 * the range of characters is defined as [start, end).
 *
 * If @em start is larger than the size of the string @em string, the
 * function does nothing.
 */

void
cx_string_replace_character(cx_string *self, cxsize start, cxsize end,
                            cxchar old_value, cxchar new_value)
{

    cxsize i;


    if (self == NULL) {
        return;
    }

    if ((start >= self->sz)) {
        return;
    }

    end = (end > self->sz) ? self->sz : end;

    for (i = start; i < end; ++i) {

        if (self->data[i] == old_value) {
            self->data[i] = new_value;
        }

    }

    return;

}


/**
 * @brief
 *   Resize a string to a given length.
 *
 * @param self  A cx_string.
 * @param size  The new length of the string.
 * @param c     The character used to fill the new character space
 *
 * @return Nothing.
 *
 * The function resizes a string to a new length @em size. If the new length
 * is smaller than the current length of the string, the string is shortened
 * to its first @em size characters. If @em size is larger than the current
 * length of the string its current contents is extended by adding to its end
 * as many characters as needed to reach a length of @em size characters. In
 * this latter case the added characters are initialized with the character
 * @em c.
 *
 * Specifying zero as the new length of the string, results in the empty
 * string.
 *
 * @see cx_string_truncate(), cx_string_extend()
 */

void
cx_string_resize(cx_string *self, cxsize size, cxchar c)
{

    if (self) {

        if (size == 0) {
            _cx_string_clear(self);
        }
        else {

            cxchar *s = cx_malloc((size + 1) * sizeof(cxchar));

            if (size > self->sz) {
                memcpy(s, self->data, self->sz);
                memset(&s[self->sz], c, size - self->sz);


            }
            else {
                memcpy(s, self->data, size);
            }
            s[size] = '\0';

            cx_free(self->data);
            self->data = s;
            self->sz   = size;

        }

    }

    return;

}


/**
 * @brief
 *   Extend a string to a given length.
 *
 * @param self  A cx_string.
 * @param size  The number of characters by which the string is enlarged.
 * @param c     The character used to fill the new character space
 *
 * @return Nothing.
 *
 * The function extends a string by adding to its end @em size number of of
 * characters. The added characters are initialized with the character
 * @em c.
 *
 * Extending a string with zero characters leaves the string untouched.
 *
 * @see cx_string_resize()
 */

void
cx_string_extend(cx_string *self, cxsize size, cxchar c)
{

    if (self && (size > 0)) {

        cxsize _size = self->sz + size;

        cxchar *s = cx_calloc(_size + 1, sizeof(cxchar));


        memcpy(s, self->data, self->sz);
        memset(s + self->sz, c, size);

        cx_free(self->data);
        self->data = s;
        self->sz   = _size;

    }

    return;

}


/**
 * @brief
 *   Search a string for the first character that does not match any of the
 *   given characters.
 *
 * @param self       A string.
 * @param characters Another string with a set of characters to be used in
 *                   the search of the string.
 *
 * @return
 *   The function returns the position of the first character that does not
 *   match, or the position one past the last element of the string(i.e. the
 *   current size of the string) if no such character exists.
 *
 * The function searches the given string for the first character that does
 * not match any of the characters specified in the set of characters
 * @em characters.
 */

cxsize
cx_string_find_first_not_of(const cx_string *self, const cxchar *characters)
{

    const cxchar *data;
    const cxchar *c;

    cx_assert(self != NULL);
    cx_assert(characters != NULL);

    if (characters[0] == '\0') {
        return self->sz;
    }

    data = self->data;

    while (*data && ((c = strchr(characters, *data)) != NULL)) {
        ++data;
    }

    return data - self->data;

}


/**
 * @brief
 *   Search a string for the last character that does not match any of the
 *   given characters.
 *
 * @param self       A string.
 * @param characters Another string with a set of characters to be used in
 *                   the search of the string.
 *
 * @return
 *   The function returns the position of the last character that does not
 *   match, or the position one past the last element of the string(i.e. the
 *   current size of the string) if no such character exists.
 *
 * The function searches the given string for the last character that does
 * not match any of the characters specified in the set of characters
 * @em characters.
 */

cxsize
cx_string_find_last_not_of(const cx_string *self, const cxchar *characters)
{

    const cxchar *data;

    cxsize pos;


    cx_assert(self != NULL);
    cx_assert(characters != NULL);

    if (characters[0] == '\0') {
        return self->sz;
    }

    data = self->data + self->sz - 1;

    while ((data != self->data) && (strchr(characters, *data) != NULL)) {
        --data;
    }

    if (data == self->data) {
        pos = (strchr(characters, *data) != NULL) ? self->sz : 0;
    }
    else {
        pos = data - self->data;
    }

    return pos;

}


/**
 * @brief
 *   Create a new string from a portion of a string.
 *
 * @param self A string.
 * @param pos  Position of the first character of the substring.
 * @param len  The length of the substring.
 *
 * @return
 *   The function returns a new string initialized with the characters of
 *   the specified substring.
 *
 * The function constructs a new string with its value initialized to a copy
 * of a substring of @em self. The substring is specified by the position
 * of the first character of the substring @em pos, and the number of
 * characters of the substring @em len (or the end of the string, whichever
 * comes first).
 *
 * If @em pos is equal to the length of @em self, an empty string is returned.
 * It is an error if @em pos is greater than the length of @em self.
 */

cx_string *
cx_string_substr(const cx_string *self, cxsize pos, cxsize len)
{

    cx_string *sstr = NULL;

    cx_assert(self != NULL);
    cx_assert(pos <= self->sz);


    if (pos == self->sz) {
        sstr = cx_string_new();
    }
    else {

        len = CX_MIN(len, self->sz - pos);

        cxchar *str = cx_malloc((len + 1) * sizeof(cxchar));

        memcpy(str, self->data + pos, len);
        str[len] = '\0';

        sstr = cx_string_create(str);
        cx_free(str);

    }

    return sstr;

}
/**@}*/
