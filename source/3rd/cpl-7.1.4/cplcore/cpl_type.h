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

#ifndef CPL_TYPE_H
#define CPL_TYPE_H

#include <cxtypes.h>
#include <cpl_macros.h>

CPL_BEGIN_DECLS

/**@{*/

/**
 * @ingroup cpl_type
 *
 * @brief
 *   The CPL type codes and flags.
 */

enum _cpl_type_ {

    /* flags */

    /**
     * Flag indicating whether a type is an array or a basic type.
     * @hideinitializer
     */
    CPL_TYPE_FLAG_ARRAY  = 1 << 0,

    /* Padded for future extensions */

    /* types */

    /**
     * Invalid or undetermined type.
     * @hideinitializer
     */
    CPL_TYPE_INVALID     = 1 << 4,

    /**
     * Type code corresponding to type @c char.
     * @hideinitializer
     */
    CPL_TYPE_CHAR        = 1 << 5,

    /**
     * Type code corresponding to type @c unsigned char.
     * @hideinitializer
     */
    CPL_TYPE_UCHAR       = 1 << 6,

    /**
     * Type code corresponding to the boolean type.
     * @hideinitializer
     */
    CPL_TYPE_BOOL        = 1 << 7,

    /**
     * Type code corresponding to type @c short.
     * @hideinitializer
     */
    CPL_TYPE_SHORT       = 1 << 8,

    /**
     * Type code corresponding to type @c unsigned short.
     * @hideinitializer
     */
    CPL_TYPE_USHORT      = 1 << 9,

    /**
     * Type code corresponding to type @c int.
     * @hideinitializer
     */
    CPL_TYPE_INT         = 1 << 10,

    /**
     * Type code corresponding to type @c unsigned int.
     * @hideinitializer
     */
    CPL_TYPE_UINT        = 1 << 11,

    /**
     * Type code corresponding to type @c long.
     * @hideinitializer
     */
    CPL_TYPE_LONG        = 1 << 12,

    /**
     * Type code corresponding to type @c unsigned long.
     * @hideinitializer
     */
    CPL_TYPE_ULONG       = 1 << 13,

    /**
     * Type code corresponding to type @c long @c long.
     * @hideinitializer
     */
    CPL_TYPE_LONG_LONG   = 1 << 14,

    /**
     * Type code corresponding to type @c cpl_size
     * @hideinitializer
     */
    CPL_TYPE_SIZE        = 1 << 15,

    /**
     * Type code corresponding to type @c float.
     * @hideinitializer
     */
    CPL_TYPE_FLOAT       = 1 << 16,

    /**
     * Type code corresponding to type @c double.
     * @hideinitializer
     */
    CPL_TYPE_DOUBLE      = 1 << 17,

    /**
     * Type code corresponding to a pointer type.
     * @hideinitializer
     */
    CPL_TYPE_POINTER     = 1 << 18,

    /**
     * Type code corresponding to a complex type.
     * @hideinitializer
     */
    CPL_TYPE_COMPLEX     = 1 << 19,

    /**
     * Type code to be used for inheritance of original FITS type.
     * @hideinitializer
     */
    CPL_TYPE_UNSPECIFIED = 1 << 20,

    /**
     * Type code corresponding to type @c cpl_bitmask
     * @hideinitializer
     */
    CPL_TYPE_BITMASK     = 1 << 21,

    /**
     * Type code corresponding to a character array.
     * @hideinitializer
     */
    CPL_TYPE_STRING  = (CPL_TYPE_CHAR | CPL_TYPE_FLAG_ARRAY),

    /**
     * Type code corresponding to type @c float complex.
     * @hideinitializer
     */
    CPL_TYPE_FLOAT_COMPLEX = (CPL_TYPE_FLOAT | CPL_TYPE_COMPLEX),

    /**
     * Type code corresponding to type @c double complex.
     * @hideinitializer
     */
    CPL_TYPE_DOUBLE_COMPLEX = (CPL_TYPE_DOUBLE | CPL_TYPE_COMPLEX)
};


/**
 * @ingroup cpl_type
 *
 * @brief
 *   The type code type.
 */

typedef enum _cpl_type_ cpl_type;


enum _cpl_boolean_ {
    CPL_FALSE = 0,
    CPL_TRUE = !CPL_FALSE
};

typedef enum _cpl_boolean_ cpl_boolean;

/**
 * @ingroup cpl_type
 *
 * @brief The type used for sizes and indices in CPL
 */

#if defined CPL_SIZE_BITS && CPL_SIZE_BITS == 32

typedef int cpl_size; /* The type as is was up to CPL 5.3 */

#else

typedef long long cpl_size;

#endif

#include <stdint.h>

/*----------------------------------------------------------------------------*/
/**
 * 
 * @brief The CPL bitmask type for bitmask operations
 * @note The CPL bitmask is currently used only for bit-wise operations on CPL
 * images of integer pixel type, which are 32-bits wide.
 * For forward compatibility the CPL bitmask is 64 bits wide, which is cast to
 * 32 bits when used with a CPL_TYPE_INT.
 */
/*----------------------------------------------------------------------------*/
typedef uint64_t cpl_bitmask;

/**
 * @def CPL_SIZE_MIN
 *
 * @hideinitializer
 * @ingroup cpl_type
 *
 * @brief
 *   Minimum value a variable of type @em cpl_size can hold.
 */

/**
 * @def CPL_SIZE_MAX
 *
 * @hideinitializer
 * @ingroup cpl_type
 *
 * @brief
 *   Maximum value a variable of type @em cpl_size can hold.
 */

#if defined CPL_SIZE_BITS && CPL_SIZE_BITS == 32

#define CPL_SIZE_MIN (CX_MININT)
#define CPL_SIZE_MAX (CX_MAXINT)

#else

#define CPL_SIZE_MIN (LLONG_MIN)
#define CPL_SIZE_MAX (LLONG_MAX)

#endif


/**
 * @hideinitializer
 * @ingroup cpl_type
 *
 * @brief The format specifier for the type cpl_size 
 * @note It is "ld" when cpl_size is a @c long int and "d" when it is an @c int
 * @see cpl_size
 *
 * It can be used like this:
 *  @code
 *    cpl_size i = my_index();
 *
 *    return cpl_sprintf("The index is %" CPL_SIZE_FORMAT "\n", i);
 *
 * @endcode
 */
#if defined CPL_SIZE_BITS && CPL_SIZE_BITS == 32
#define CPL_SIZE_FORMAT "d"
#else
#define CPL_SIZE_FORMAT "lld"
#endif

/**@}*/

size_t cpl_type_get_sizeof(cpl_type type) CPL_ATTR_CONST;

const char * cpl_type_get_name(cpl_type type);

CPL_END_DECLS

#endif /* CPL_TYPE_H */
