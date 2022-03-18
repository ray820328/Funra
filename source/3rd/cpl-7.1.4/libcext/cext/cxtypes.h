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

#ifndef _CX_TYPES_H
#define _CX_TYPES_H

#include <cxconfig.h>
#include <cxmacros.h>


CX_BEGIN_DECLS

/*
 * Some mathematical constants. Some strict ISO C implementations
 * don't provide them as symbols. The constants provide enough
 * digits for the 128 bit IEEE quad
 */

#define CX_E     2.7182818284590452353602874713526625L
#define CX_LN2   0.6931471805599453094172321214581766L
#define CX_LN10  2.3025850929940456840179914546843642L
#define CX_PI    3.1415926535897932384626433832795029L
#define CX_PI_2  1.5707963267948966192313216916397514L
#define CX_PI_4  0.7853981633974483096156608458198757L
#define CX_SQRT2 1.4142135623730950488016887242096981L


/*
 * Minimum and maximum constants for fixed size integer types
 */

#define CX_MININT8    ((cxint8)   0x80)
#define CX_MAXINT8    ((cxint8)   0x7f)
#define CX_MAXUINT8   ((cxuint8)  0xff)

#define CX_MININT16   ((cxint16)   0x8000)
#define CX_MAXINT16   ((cxint16)   0x7fff)
#define CX_MAXUINT16  ((cxuint16)  0xffff)

#define CX_MININT32   ((cxint32)   0x80000000)
#define CX_MAXINT32   ((cxint32)   0x7fffffff)
#define CX_MAXUINT32  ((cxuint32)  0xffffffff)

#define CX_MININT64   ((cxint64)   CX_INT64_CONSTANT(0x8000000000000000))
#define CX_MAXINT64   CX_INT64_CONSTANT(0x7fffffffffffffff)
#define CX_MAXUINT64  CX_INT64_CONSTANT(0xffffffffffffffffU)


/*
 * For completeness: Definitions for standard types
 */

typedef char      cxchar;
typedef short     cxshort;
typedef int       cxint;
typedef long      cxlong;
typedef long long cxllong;
typedef cxint     cxbool;

typedef unsigned char      cxuchar;
typedef unsigned short     cxushort;
typedef unsigned int       cxuint;
typedef unsigned long      cxulong;
typedef unsigned long long cxullong;
typedef cxuchar            cxbyte;

typedef float  cxfloat;
typedef double cxdouble;

typedef void * cxptr;
typedef const void * cxcptr;


/*
 * Generic, frequently used types.
 */

typedef cxint  (*cx_compare_func) (cxcptr a, cxcptr b);
typedef cxint  (*cx_compare_data_func) (cxcptr a, cxcptr b, cxptr data);
typedef cxbool (*cx_equal_func) (cxcptr a, cxcptr b);
typedef void   (*cx_free_func) (cxptr data);

CX_END_DECLS

#endif /* _CX_TYPES_H */
