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

#include <complex.h>

#include "cpl_type.h"

#include "cpl_error_impl.h"


/**
 * @defgroup cpl_type Type codes
 *
 * This module provides codes for the basic types (including @c char, @c int,
 * @c float, etc.) used in CPL. These type codes may be used to indicate the
 * type of a value stored in another object, the value of a property or the
 * pixel of an image for instance. In addition, a utility function is provided
 * to compute the size, which is required to store a value of the type indicated
 * by a given type code.
 * 
 * 
 * The module 
 * @par Synopsis
 * @code
 *   #include <cpl_type.h>
 * @endcode
 */

/**@{*/

/**
 * @brief
 *   Compute the size of a type.
 *
 * @param type  Type code to be evaluated.
 *
 * @return The size of the fundamental type, or 0 in case an invalid type
 *   code was given.
 *
 * The function computes the atomic size of the type @em type. The result
 * for fundamental types like @c CPL_TYPE_FLOAT is what you would expect
 * from the C @b sizeof() operator. For arrays, i.e. types having the 
 * @c CPL_TYPE_FLAG_ARRAY set the returned size is not the size of a pointer
 * to @c CPL_TYPE_FLOAT for instance, but the size of its fundamental type,
 * i.e. the returned size is same as for the type @c CPL_TYPE_FLOAT.
 *
 * Especially for the type @c CPL_TYPE_STRING, which is explicitly defined for
 * convenience reasons, the size returned by this function is the size of
 * @c CPL_TYPE_CHAR!
 */

size_t 
cpl_type_get_sizeof(cpl_type type)
{

    size_t sz;


    if ((type & CPL_TYPE_INVALID) || (type & CPL_TYPE_UNSPECIFIED))
        return 0;

    /*
     * We just want the size of the fundamental type, therefore the
     * array flag is removed. This also means that an explicit case
     * for CPL_TYPE_STRING is not necessary in the following switch.
     */

    type &= ~CPL_TYPE_FLAG_ARRAY;

    switch (type) {
        case CPL_TYPE_CHAR:
            sz = sizeof(char);
            break;

        case CPL_TYPE_UCHAR:
            sz = sizeof(unsigned char);
            break;

        case CPL_TYPE_BOOL:
            sz = sizeof(cpl_boolean);
            break;

        case CPL_TYPE_SHORT:
            sz = sizeof(short);
            break;

        case CPL_TYPE_USHORT:
            sz = sizeof(unsigned short);
            break;

        case CPL_TYPE_INT:
            sz = sizeof(int);
            break;

        case CPL_TYPE_UINT:
            sz = sizeof(unsigned int);
            break;

        case CPL_TYPE_LONG:
            sz = sizeof(long int);
            break;

        case CPL_TYPE_ULONG:
            sz = sizeof(unsigned long int);
            break;

        case CPL_TYPE_LONG_LONG:
            sz = sizeof(long long int);
            break;

        case CPL_TYPE_SIZE:
            sz = sizeof(cpl_size);
            break;

        case CPL_TYPE_FLOAT:
            sz = sizeof(float);
            break;

        case CPL_TYPE_DOUBLE:
            sz = sizeof(double);
            break;

        case CPL_TYPE_POINTER:
            sz = sizeof(void*);
            break;

        case CPL_TYPE_FLOAT_COMPLEX:
            sz = sizeof(float complex);
            break;

        case CPL_TYPE_DOUBLE_COMPLEX:
            sz = sizeof(double complex);
            break;

        default:
            sz = 0;
            break;
    }

    return sz;

}

/**
 * @brief
 *   Get a string with the name of a type, e.g. "char", "int", "float"
 *
 * @param type  Type code to be evaluated.
 *
 * @return A pointer to a string literal with the name or empty string on error.
 *
 */

const char *
cpl_type_get_name(cpl_type type)
{

    const char * self = "";

    if ((type & CPL_TYPE_POINTER) || (type & CPL_TYPE_FLAG_ARRAY)) {

        /* switch on type without POINTER and ARRAY bits */
        switch (type & ~CPL_TYPE_POINTER & ~CPL_TYPE_FLAG_ARRAY) {
            /* Here follows all "usable" types as arrays */

        case CPL_TYPE_CHAR:
            /* string: case CPL_TYPE_STRING | CPL_TYPE_FLAG_ARRAY: */
            self = "char array";
            break;

        case CPL_TYPE_UCHAR:
            self = "unsigned char array";
            break;

        case CPL_TYPE_BOOL:
            self = "boolean array";
            break;

        case CPL_TYPE_SHORT:
            self = "short array";
            break;

        case CPL_TYPE_USHORT:
            self = "unsigned short array";
            break;

        case CPL_TYPE_INT:
            self = "int array";
            break;

        case CPL_TYPE_UINT:
            self = "unsigned int array";
            break;

        case CPL_TYPE_LONG:
            self = "long int array";
            break;

        case CPL_TYPE_ULONG:
            self = "unsigned long int array";
            break;

        case CPL_TYPE_LONG_LONG:
            self = "long long int array";
            break;

        case CPL_TYPE_SIZE:
            self = "cpl size array";
            break;

        case CPL_TYPE_FLOAT:
            self = "float array";
            break;

        case CPL_TYPE_DOUBLE:
            self = "double array";
            break;

        case CPL_TYPE_FLOAT_COMPLEX:
            self = "float complex array";
            break;

        case CPL_TYPE_DOUBLE_COMPLEX:
            self = "double complex array";
            break;

        case CPL_TYPE_STRING:
            /* char: case CPL_TYPE_STRING | CPL_TYPE_FLAG_ARRAY: */
            self = "string array";
            break;

        default:
            (void)cpl_error_set_message_(CPL_ERROR_INVALID_TYPE,
                                         "array/pointer type=0x%x", (int)type);
            break;
        }

    } else {

        switch (type) {
            /* FIXME: This is a valid value for the enum... */
        case CPL_TYPE_FLAG_ARRAY:
            self = "array";
            break;

            /* FIXME: This is a valid value for the enum... */
        case CPL_TYPE_INVALID:
            self = "invalid";
            break;

        case CPL_TYPE_CHAR:
            self = "char";
            break;

        case CPL_TYPE_UCHAR:
            self = "unsigned char";
            break;

        case CPL_TYPE_BOOL:
            self = "boolean";
            break;

        case CPL_TYPE_SHORT:
            self = "short";
            break;

        case CPL_TYPE_USHORT:
            self = "unsigned short";
            break;

        case CPL_TYPE_INT:
            self = "int";
            break;

        case CPL_TYPE_UINT:
            self = "unsigned int";
            break;

        case CPL_TYPE_LONG:
            self = "long int";
            break;

        case CPL_TYPE_ULONG:
            self = "unsigned long int";
            break;

        case CPL_TYPE_LONG_LONG:
            self = "long long int";
            break;

        case CPL_TYPE_SIZE:
            self = "cpl size";
            break;

        case CPL_TYPE_FLOAT:
            self = "float";
            break;

        case CPL_TYPE_DOUBLE:
            self = "double";
            break;

        case CPL_TYPE_POINTER:
            self = "pointer";
            break;

        case CPL_TYPE_FLOAT_COMPLEX:
            self = "float complex";
            break;

        case CPL_TYPE_DOUBLE_COMPLEX:
            self = "double complex";
            break;

        case CPL_TYPE_UNSPECIFIED:
            self = "unspecified";
            break;

        case CPL_TYPE_STRING:
            self = "string";
            break;

        default:
            (void)cpl_error_set_message_(CPL_ERROR_INVALID_TYPE,
                                         "type=0x%x", (int)type);
            break;
        }
    }

    return self;

}

/**@}*/
