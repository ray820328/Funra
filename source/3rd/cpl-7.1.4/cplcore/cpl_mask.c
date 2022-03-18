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

/* Verify self-sufficiency of cpl_mask.h by including it first */
#include "cpl_mask_defs.h"
#include "cpl_memory.h"
#include "cpl_tools.h"
#include "cpl_error_impl.h"
#include "cpl_io_fits.h"
#include "cpl_cfitsio.h"
#include "cpl_propertylist_impl.h"
#include "cpl_math_const.h"

/* Verify self-sufficiency of CPL header files by including system files last */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <float.h>

#include <assert.h>

#ifdef __SSE2__
#include <emmintrin.h>

#ifdef __clang__
#    define cpl_m_from_int64 (__m64)
#  else
#    define cpl_m_from_int64 _m_from_int64
#  endif
#endif

/*-----------------------------------------------------------------------------
                                   Defines
 -----------------------------------------------------------------------------*/

#if defined SIZEOF_SIZE_T && SIZEOF_SIZE_T == 4
/* Maximum half-size that fits in one word */
#define CPL_MASK_HXW  1
/* Maximum half-size that fits in two words */
#define CPL_MASK_HX2W  3
/* Word-size */
#define CPL_MASK_WORD 4
/* Maximum zero-padding */
#define CPL_MASK_PAD 3
/* Used for computing number of words to span 2hx + 1 */
#define CPL_MASK_DIV 2
/* Used to not a word and via multiplication for population count in one word */
#define CPL_MASK_NOT (((((((size_t)CPL_BINARY_1<<8) | (size_t)CPL_BINARY_1) <<8) | (size_t)CPL_BINARY_1)<<8) | (size_t)CPL_BINARY_1)
#elif defined SIZEOF_SIZE_T && SIZEOF_SIZE_T == 8
#define CPL_MASK_HXW  3
#define CPL_MASK_HX2W 7
#define CPL_MASK_WORD 8
#define CPL_MASK_PAD 7
#define CPL_MASK_DIV 4
#define CPL_MASK_NOT (((((((((((((((size_t)CPL_BINARY_1<<8) | (size_t)CPL_BINARY_1) <<8) | (size_t)CPL_BINARY_1)<<8) | (size_t)CPL_BINARY_1 )<<8)|(size_t)CPL_BINARY_1)<<8) | (size_t)CPL_BINARY_1) <<8) | (size_t)CPL_BINARY_1)<<8) | (size_t)CPL_BINARY_1)
#endif

#define CPL_MASK_PAD2 ((CPL_MASK_PAD<<1)|1)
#define CPL_MASK_WORD2 (CPL_MASK_WORD<<1)

#define CPL_MASK_NOT8 (((((((((((((((cpl_bitmask)CPL_BINARY_1<<8) | (cpl_bitmask)CPL_BINARY_1) <<8) | (cpl_bitmask)CPL_BINARY_1)<<8) | (cpl_bitmask)CPL_BINARY_1 )<<8)|(cpl_bitmask)CPL_BINARY_1)<<8) | (cpl_bitmask)CPL_BINARY_1) <<8) | (cpl_bitmask)CPL_BINARY_1)<<8) | (cpl_bitmask)CPL_BINARY_1)

#ifdef __SSE2__
#define CPL_MASK_REGISTER_SIZE 16
#define CPL_MASK_REGISTER_TYPE __m128i
#else
#define CPL_MASK_REGISTER_SIZE CPL_MASK_WORD
#define CPL_MASK_REGISTER_TYPE size_t
#endif

#define CPL_MASK_PAD2WORD(I) \
    ((CPL_MASK_WORD - ((I)&CPL_MASK_PAD)) & CPL_MASK_PAD)
#define CPL_MASK_PAD2WORD2(I) \
    ((CPL_MASK_WORD2 - ((I)&CPL_MASK_PAD2)) & CPL_MASK_PAD2)

/* These macros are needed for support of the different pixel types */

#define CONCAT(a,b) a ## _ ## b
#define CONCAT2X(a,b) CONCAT(a,b)

/* Multiple concatenation should append the terms starting from the left, */
/* to avoid forming intermediate identifiers that are reserved (e.g. _0_1) */
#define APPENDSIZE(a, b, c) CONCAT2X(CONCAT2X(CONCAT2X(cpl_mask, a), b), c)
#define APPENDOPER(a) CONCAT2X(CONCAT2X(cpl_mask, a), _)
#define APPENDOPERS(a) CONCAT2X(CONCAT2X(cpl_mask, a), scalar)
#define OPER2MM(a) CONCAT2X(CONCAT2X(_mm, a), si128)

/*----------------------------------------------------------------------------*/
/**
 * @defgroup cpl_mask   Masks of pixels
 *
 * This module provides functions to handle masks of pixels
 *
 * These masks are useful for object detection routines or bad pixel map
 * handling. Morphological routines (erosion, dilation, closing and
 * opening) and logical operations are provided. A cpl_mask is a kind of
 * binary array whose elements are of type cpl_binary and can take only
 * two values: either CPL_BINARY_0 or CPL_BINARY_1.
 *
 * The element indexing follows the FITS convention in the sense that the
 * lower left element in a CPL mask has index (1, 1).
 *
 * @par Synopsis:
 * @code
 *   #include "cpl_mask.h"
 * @endcode
 */
/*----------------------------------------------------------------------------*/
/**@{*/

/*-----------------------------------------------------------------------------
                                   Private functions
 -----------------------------------------------------------------------------*/
static void cpl_mask_and__(cpl_binary        *,
                           const cpl_binary  *,
                           const cpl_binary  *,
                           size_t)
#ifdef CPL_HAVE_ATTR_NONNULL
__attribute__((nonnull(1,3)))
#endif
    ;

static void cpl_mask_or__(cpl_binary        *,
                          const cpl_binary  *,
                          const cpl_binary  *,
                          size_t)
#ifdef CPL_HAVE_ATTR_NONNULL
__attribute__((nonnull(1,3)))
#endif
    ;

static void cpl_mask_xor__(cpl_binary        *,
                           const cpl_binary  *,
                           const cpl_binary  *,
                           size_t)
#ifdef CPL_HAVE_ATTR_NONNULL
__attribute__((nonnull(1,3)))
#endif
    ;

static
cpl_mask * cpl_mask_load_one(const char *, cpl_size, cpl_size, cpl_boolean,
                             cpl_size, cpl_size, cpl_size, cpl_size)
    CPL_ATTR_ALLOC;

static
cpl_mask * cpl_mask_load_(fitsfile*, int*, CPL_FITSIO_TYPE[], const char*,
                          cpl_size, cpl_size, cpl_boolean, cpl_size, cpl_size,
                          cpl_size, cpl_size) CPL_ATTR_ALLOC;

static
void cpl_mask_erosion_(cpl_binary *, const cpl_binary *, const cpl_binary *,
                       cpl_size, cpl_size, cpl_size, cpl_size, cpl_border_mode)
    CPL_ATTR_NONNULL;
static
void cpl_mask_dilation_(cpl_binary *, const cpl_binary *, const cpl_binary *,
                        cpl_size, cpl_size, cpl_size, cpl_size, cpl_border_mode)
    CPL_ATTR_NONNULL;
static
void cpl_mask_opening_(cpl_binary *, const cpl_binary *, const cpl_binary *,
                       cpl_size, cpl_size, cpl_size, cpl_size, cpl_border_mode)
    CPL_ATTR_NONNULL;
static
void cpl_mask_closing_(cpl_binary *, const cpl_binary *, const cpl_binary *,
                       cpl_size, cpl_size, cpl_size, cpl_size, cpl_border_mode)
    CPL_ATTR_NONNULL;

static cpl_mask * cpl_mask_new_from_matrix(const cpl_matrix *,
                                           double) CPL_ATTR_ALLOC;

#define CPL_MASK_BINARY_WOPER and
#define CPL_MASK_BINARY_OPER  &

#include "cpl_mask_binary.h"

#undef CPL_MASK_BINARY_WOPER
#undef CPL_MASK_BINARY_OPER

#define CPL_MASK_BINARY_WOPER or
#define CPL_MASK_BINARY_OPER  |

#include "cpl_mask_binary.h"

#undef CPL_MASK_BINARY_WOPER
#undef CPL_MASK_BINARY_OPER


#define CPL_MASK_BINARY_WOPER xor
#define CPL_MASK_BINARY_OPER  ^

#include "cpl_mask_binary.h"

#undef CPL_MASK_BINARY_WOPER
#undef CPL_MASK_BINARY_OPER


#ifdef CPL_MASK_NOT
/* Support Erosion/Dilation: general case */

#define GENERAL_CASE

#define OPERATION erosion
#define OPERATE_WORD(A) ((A) ^ CPL_MASK_NOT)
#define OPERATE_PIXEL(A) (!(A))
#define VALUE_TRUE  CPL_BINARY_0
#define VALUE_FALSE CPL_BINARY_1

#define HX n
#define HY n

#define CPL_MASK_FILTER_WORD                                    \
    const cpl_binary * otheri  = other + i;                     \
    const size_t * kernelkw = (const size_t *)kernel;           \
    size_t k, l;                                                \
                                                                \
    for (k = 0; k < 1 + 2 * hy; k++, otheri += nx,              \
             kernelkw += mxew) {                                \
        /* Points to first element to read */                   \
        const size_t * otheriw = (const size_t *)otheri;        \
                                                                \
        for (l = 0; l < (hx+CPL_MASK_DIV)/CPL_MASK_DIV; l++) {  \
            if (OPERATE_WORD(otheriw[l]) & kernelkw[l])         \
                break;                                          \
        }                                                       \
        if (l < (hx+CPL_MASK_DIV)/CPL_MASK_DIV) break;          \
                                                                \
    }                                                           \
    self[i] = k < 1+ 2 * hy ? VALUE_TRUE : VALUE_FALSE

#include "cpl_mask_body.h"

#undef OPERATION
#undef OPERATE_WORD
#undef OPERATE_PIXEL
#undef VALUE_TRUE
#undef VALUE_FALSE

#define OPERATION dilation
#define OPERATE_WORD(A) (A)
#define OPERATE_PIXEL(A) (A)
#define VALUE_TRUE  CPL_BINARY_1
#define VALUE_FALSE CPL_BINARY_0

#include "cpl_mask_body.h"

#undef OPERATION
#undef OPERATE_WORD
#undef OPERATE_PIXEL
#undef VALUE_TRUE
#undef VALUE_FALSE

#undef GENERAL_CASE

/* Support Erosion/Dilation: hx within 1 word and hy == 0 */

#undef CPL_MASK_FILTER_WORD
#undef HX
#undef HY

#define OPERATION erosion
#define OPERATE_WORD(A) ((A) ^ CPL_MASK_NOT)
#define OPERATE_PIXEL(A) (!(A))
#define VALUE_TRUE  CPL_BINARY_0
#define VALUE_FALSE CPL_BINARY_1

#define HX 1
#define HY 0

#define CPL_MASK_FILTER_WORD                                            \
    self[i] =                                                           \
        (OPERATE_WORD(*(const size_t*)(other + i       )) & kernelw[0]) \
        ? VALUE_TRUE : VALUE_FALSE

#include "cpl_mask_body.h"

#undef OPERATION
#undef OPERATE_WORD
#undef OPERATE_PIXEL
#undef VALUE_TRUE
#undef VALUE_FALSE


#define OPERATION dilation
#define OPERATE_WORD(A) (A)
#define OPERATE_PIXEL(A) (A)
#define VALUE_TRUE  CPL_BINARY_1
#define VALUE_FALSE CPL_BINARY_0

#include "cpl_mask_body.h"

#undef OPERATION
#undef OPERATE_WORD
#undef OPERATE_PIXEL
#undef VALUE_TRUE
#undef VALUE_FALSE

/* Support Erosion/Dilation: hx within 1 word and hy == 1 */

#undef CPL_MASK_FILTER_WORD
#undef HX
#undef HY

#define OPERATION erosion
#define OPERATE_WORD(A) ((A) ^ CPL_MASK_NOT)
#define OPERATE_PIXEL(A) (!(A))
#define VALUE_TRUE  CPL_BINARY_0
#define VALUE_FALSE CPL_BINARY_1

#define HX 1
#define HY 1

#define CPL_MASK_FILTER_WORD                                            \
    self[i] =                                                           \
        (OPERATE_WORD(*(const size_t*)(other + i       )) & kernelw[0]) || \
        (OPERATE_WORD(*(const size_t*)(other + i + nx  )) & kernelw[1]) || \
        (OPERATE_WORD(*(const size_t*)(other + i + nx*2)) & kernelw[2]) \
        ? VALUE_TRUE : VALUE_FALSE

#include "cpl_mask_body.h"

#undef OPERATION
#undef OPERATE_WORD
#undef OPERATE_PIXEL
#undef VALUE_TRUE
#undef VALUE_FALSE


#define OPERATION dilation
#define OPERATE_WORD(A) (A)
#define OPERATE_PIXEL(A) (A)
#define VALUE_TRUE  CPL_BINARY_1
#define VALUE_FALSE CPL_BINARY_0

#include "cpl_mask_body.h"

#undef OPERATION
#undef OPERATE_WORD
#undef OPERATE_PIXEL
#undef VALUE_TRUE
#undef VALUE_FALSE

/* Support Erosion/Dilation: hx within 1 word and hy == 2 */

#undef CPL_MASK_FILTER_WORD
#undef HX
#undef HY

#define OPERATION erosion
#define OPERATE_WORD(A) ((A) ^ CPL_MASK_NOT)
#define OPERATE_PIXEL(A) (!(A))
#define VALUE_TRUE  CPL_BINARY_0
#define VALUE_FALSE CPL_BINARY_1

#define HX 1
#define HY 2

#define CPL_MASK_FILTER_WORD                                            \
    self[i] =                                                           \
        (OPERATE_WORD(*(const size_t*)(other + i       )) & kernelw[0]) || \
        (OPERATE_WORD(*(const size_t*)(other + i + nx  )) & kernelw[1]) || \
        (OPERATE_WORD(*(const size_t*)(other + i + nx*2)) & kernelw[2]) || \
        (OPERATE_WORD(*(const size_t*)(other + i + nx*3)) & kernelw[3]) || \
        (OPERATE_WORD(*(const size_t*)(other + i + nx*4)) & kernelw[4]) \
        ? VALUE_TRUE : VALUE_FALSE

#include "cpl_mask_body.h"

#undef OPERATION
#undef OPERATE_WORD
#undef OPERATE_PIXEL
#undef VALUE_TRUE
#undef VALUE_FALSE


#define OPERATION dilation
#define OPERATE_WORD(A) (A)
#define OPERATE_PIXEL(A) (A)
#define VALUE_TRUE  CPL_BINARY_1
#define VALUE_FALSE CPL_BINARY_0

#include "cpl_mask_body.h"

#undef OPERATION
#undef OPERATE_WORD
#undef OPERATE_PIXEL
#undef VALUE_TRUE
#undef VALUE_FALSE

/* Support Erosion/Dilation: hx within 1 word and hy == 3 */

#undef CPL_MASK_FILTER_WORD
#undef HX
#undef HY

#define OPERATION erosion
#define OPERATE_WORD(A) ((A) ^ CPL_MASK_NOT)
#define OPERATE_PIXEL(A) (!(A))
#define VALUE_TRUE  CPL_BINARY_0
#define VALUE_FALSE CPL_BINARY_1

#define HX 1
#define HY 3

#define CPL_MASK_FILTER_WORD                                            \
    self[i] =                                                           \
        (OPERATE_WORD(*(const size_t*)(other + i       )) & kernelw[0]) || \
        (OPERATE_WORD(*(const size_t*)(other + i + nx  )) & kernelw[1]) || \
        (OPERATE_WORD(*(const size_t*)(other + i + nx*2)) & kernelw[2]) || \
        (OPERATE_WORD(*(const size_t*)(other + i + nx*3)) & kernelw[3]) || \
        (OPERATE_WORD(*(const size_t*)(other + i + nx*4)) & kernelw[4]) || \
        (OPERATE_WORD(*(const size_t*)(other + i + nx*5)) & kernelw[5]) || \
        (OPERATE_WORD(*(const size_t*)(other + i + nx*6)) & kernelw[6]) \
        ? VALUE_TRUE : VALUE_FALSE

#include "cpl_mask_body.h"

#undef OPERATION
#undef OPERATE_WORD
#undef OPERATE_PIXEL
#undef VALUE_TRUE
#undef VALUE_FALSE


#define OPERATION dilation
#define OPERATE_WORD(A) (A)
#define OPERATE_PIXEL(A) (A)
#define VALUE_TRUE  CPL_BINARY_1
#define VALUE_FALSE CPL_BINARY_0

#include "cpl_mask_body.h"

#undef OPERATION
#undef OPERATE_WORD
#undef OPERATE_PIXEL
#undef VALUE_TRUE
#undef VALUE_FALSE

/* Support Erosion/Dilation: hx within 2 words and hy == 0 */

#undef CPL_MASK_FILTER_WORD
#undef HX
#undef HY

#define OPERATION erosion
#define OPERATE_WORD(A) ((A) ^ CPL_MASK_NOT)
#define OPERATE_PIXEL(A) (!(A))
#define VALUE_TRUE  CPL_BINARY_0
#define VALUE_FALSE CPL_BINARY_1

#define HX 2
#define HY 0

#define CPL_MASK_FILTER_WORD                                            \
    self[i] =                                                           \
        (OPERATE_WORD(*(const size_t*)(other + i       )) & kernelw[0]) || \
        (OPERATE_WORD(*(const size_t*)(other + i + CPL_MASK_WORD))      \
         & kernelw[1])                                                  \
        ? VALUE_TRUE : VALUE_FALSE

#include "cpl_mask_body.h"

#undef OPERATION
#undef OPERATE_WORD
#undef OPERATE_PIXEL
#undef VALUE_TRUE
#undef VALUE_FALSE


#define OPERATION dilation
#define OPERATE_WORD(A) (A)
#define OPERATE_PIXEL(A) (A)
#define VALUE_TRUE  CPL_BINARY_1
#define VALUE_FALSE CPL_BINARY_0

#include "cpl_mask_body.h"

#undef OPERATION
#undef OPERATE_WORD
#undef OPERATE_PIXEL
#undef VALUE_TRUE
#undef VALUE_FALSE

/* Support Erosion/Dilation: hx within 2 words and hy == 1 */

#undef CPL_MASK_FILTER_WORD
#undef HX
#undef HY

#define OPERATION erosion
#define OPERATE_WORD(A) ((A) ^ CPL_MASK_NOT)
#define OPERATE_PIXEL(A) (!(A))
#define VALUE_TRUE  CPL_BINARY_0
#define VALUE_FALSE CPL_BINARY_1

#define HX 2
#define HY 1

#define CPL_MASK_FILTER_WORD                                            \
    self[i] =                                                           \
        (OPERATE_WORD(*(const size_t*)(other + i       )) & kernelw[0]) || \
        (OPERATE_WORD(*(const size_t*)(other + i + CPL_MASK_WORD))      \
         & kernelw[1]) ||                                               \
        (OPERATE_WORD(*(const size_t*)(other + i + nx  )) & kernelw[2]) || \
        (OPERATE_WORD(*(const size_t*)(other + i + nx   + CPL_MASK_WORD)) \
         & kernelw[3]) ||                                               \
        (OPERATE_WORD(*(const size_t*)(other + i + nx*2)) & kernelw[4]) || \
        (OPERATE_WORD(*(const size_t*)(other + i + nx*2 + CPL_MASK_WORD)) \
         & kernelw[5])                                                  \
        ? VALUE_TRUE : VALUE_FALSE

#include "cpl_mask_body.h"

#undef OPERATION
#undef OPERATE_WORD
#undef OPERATE_PIXEL
#undef VALUE_TRUE
#undef VALUE_FALSE


#define OPERATION dilation
#define OPERATE_WORD(A) (A)
#define OPERATE_PIXEL(A) (A)
#define VALUE_TRUE  CPL_BINARY_1
#define VALUE_FALSE CPL_BINARY_0

#include "cpl_mask_body.h"

#undef OPERATION
#undef OPERATE_WORD
#undef OPERATE_PIXEL
#undef VALUE_TRUE
#undef VALUE_FALSE

/* Support Erosion/Dilation: hx within 2 words and hy == 2 */

#undef CPL_MASK_FILTER_WORD
#undef HX
#undef HY

#define OPERATION erosion
#define OPERATE_WORD(A) ((A) ^ CPL_MASK_NOT)
#define OPERATE_PIXEL(A) (!(A))
#define VALUE_TRUE  CPL_BINARY_0
#define VALUE_FALSE CPL_BINARY_1

#define HX 2
#define HY 2

#define CPL_MASK_FILTER_WORD                                            \
    self[i] =                                                           \
        (OPERATE_WORD(*(const size_t*)(other + i       )) & kernelw[0]) || \
        (OPERATE_WORD(*(const size_t*)(other + i + CPL_MASK_WORD))      \
         & kernelw[1]) ||                                               \
        (OPERATE_WORD(*(const size_t*)(other + i + nx  )) & kernelw[2]) || \
        (OPERATE_WORD(*(const size_t*)(other + i + nx   + CPL_MASK_WORD)) \
         & kernelw[3]) ||                                               \
        (OPERATE_WORD(*(const size_t*)(other + i + nx*2)) & kernelw[4]) || \
        (OPERATE_WORD(*(const size_t*)(other + i + nx*2 + CPL_MASK_WORD)) \
         & kernelw[5]) ||                                               \
        (OPERATE_WORD(*(const size_t*)(other + i + nx*3)) & kernelw[6]) || \
        (OPERATE_WORD(*(const size_t*)(other + i + nx*3 + CPL_MASK_WORD)) \
         & kernelw[7]) ||                                               \
        (OPERATE_WORD(*(const size_t*)(other + i + nx*4)) & kernelw[8]) || \
        (OPERATE_WORD(*(const size_t*)(other + i + nx*4 + CPL_MASK_WORD)) \
         & kernelw[9])                                                  \
        ? VALUE_TRUE : VALUE_FALSE

#include "cpl_mask_body.h"

#undef OPERATION
#undef OPERATE_WORD
#undef OPERATE_PIXEL
#undef VALUE_TRUE
#undef VALUE_FALSE


#define OPERATION dilation
#define OPERATE_WORD(A) (A)
#define OPERATE_PIXEL(A) (A)
#define VALUE_TRUE  CPL_BINARY_1
#define VALUE_FALSE CPL_BINARY_0

#include "cpl_mask_body.h"

#undef OPERATION
#undef OPERATE_WORD
#undef OPERATE_PIXEL
#undef VALUE_TRUE
#undef VALUE_FALSE

/* Support Erosion/Dilation: hx within 2 words and hy == 3 */
/* On 32-bit this covers kernels up to 7x7, on 64-bit up to 15x7 */

#undef CPL_MASK_FILTER_WORD
#undef HX
#undef HY

#define OPERATION erosion
#define OPERATE_WORD(A) ((A) ^ CPL_MASK_NOT)
#define OPERATE_PIXEL(A) (!(A))
#define VALUE_TRUE  CPL_BINARY_0
#define VALUE_FALSE CPL_BINARY_1

#define HX 2
#define HY 3

#define CPL_MASK_FILTER_WORD                                            \
    self[i] =                                                           \
        (OPERATE_WORD(*(const size_t*)(other + i       )) & kernelw[0]) || \
        (OPERATE_WORD(*(const size_t*)(other + i + CPL_MASK_WORD))      \
         & kernelw[1]) ||                                               \
        (OPERATE_WORD(*(const size_t*)(other + i + nx  )) & kernelw[2]) || \
        (OPERATE_WORD(*(const size_t*)(other + i + nx   + CPL_MASK_WORD)) \
         & kernelw[3]) ||                                               \
        (OPERATE_WORD(*(const size_t*)(other + i + nx*2)) & kernelw[4]) || \
        (OPERATE_WORD(*(const size_t*)(other + i + nx*2 + CPL_MASK_WORD)) \
         & kernelw[5]) ||                                               \
        (OPERATE_WORD(*(const size_t*)(other + i + nx*3)) & kernelw[6]) || \
        (OPERATE_WORD(*(const size_t*)(other + i + nx*3 + CPL_MASK_WORD)) \
         & kernelw[7]) ||                                               \
        (OPERATE_WORD(*(const size_t*)(other + i + nx*4)) & kernelw[8]) || \
        (OPERATE_WORD(*(const size_t*)(other + i + nx*4 + CPL_MASK_WORD)) \
         & kernelw[9]) ||                                               \
        (OPERATE_WORD(*(const size_t*)(other + i + nx*5)) & kernelw[10]) || \
        (OPERATE_WORD(*(const size_t*)(other + i + nx*5 + CPL_MASK_WORD)) \
         & kernelw[11]) ||                                              \
        (OPERATE_WORD(*(const size_t*)(other + i + nx*6)) & kernelw[12]) || \
        (OPERATE_WORD(*(const size_t*)(other + i + nx*6 + CPL_MASK_WORD)) \
         & kernelw[13])                                                 \
        ? VALUE_TRUE : VALUE_FALSE

#include "cpl_mask_body.h"

#undef OPERATION
#undef OPERATE_WORD
#undef OPERATE_PIXEL
#undef VALUE_TRUE
#undef VALUE_FALSE


#define OPERATION dilation
#define OPERATE_WORD(A) (A)
#define OPERATE_PIXEL(A) (A)
#define OPERATE_AND(A,B) ((A) & (B))
#define VALUE_TRUE  CPL_BINARY_1
#define VALUE_FALSE CPL_BINARY_0

#include "cpl_mask_body.h"

#undef OPERATION
#undef OPERATE_WORD
#undef OPERATE_PIXEL
#undef OPERATE_AND
#undef VALUE_TRUE
#undef VALUE_FALSE

#endif

/*-----------------------------------------------------------------------------
                            Function codes
 -----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/**
  @brief    Create a new cpl_mask
  @param    nx  The number of elements in the X-direction
  @param    ny  The number of elements in the Y-direction
  @return   1 newly allocated cpl_mask or NULL on error
  @note  The returned object must be deallocated using cpl_mask_delete().

  The created cpl_mask elements are all set to CPL_BINARY_0.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_ILLEGAL_INPUT if nx or ny is negative
 */
/*----------------------------------------------------------------------------*/
cpl_mask * cpl_mask_new(cpl_size nx, cpl_size ny)
{
    /* Need to validate input for cpl_calloc() */
    cpl_ensure(nx > 0, CPL_ERROR_ILLEGAL_INPUT, NULL);
    cpl_ensure(ny > 0, CPL_ERROR_ILLEGAL_INPUT, NULL);

    /* Cannot fail now */
    return cpl_mask_wrap(nx, ny, (cpl_binary*)cpl_calloc((size_t)nx
                                                         * (size_t)ny,
                                                         sizeof(cpl_binary)));
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Create a cpl_mask from existing data
  @param    nx  number of element in x direction
  @param    ny  number of element in y direction
  @param    data    Pointer to array of nx*ny cpl_binary
  @return   1 newly allocated cpl_mask or NULL in case of an error
  @note The returned object must be deallocated using cpl_mask_unwrap().

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if nx or ny is negative or zero
 */
/*----------------------------------------------------------------------------*/
cpl_mask * cpl_mask_wrap(cpl_size nx, cpl_size ny, cpl_binary * data)
{
    cpl_mask    *   m;

    /* Test input */
    cpl_ensure(data != NULL, CPL_ERROR_NULL_INPUT,    NULL);
    cpl_ensure(nx   >  0,    CPL_ERROR_ILLEGAL_INPUT, NULL);
    cpl_ensure(ny   >  0,    CPL_ERROR_ILLEGAL_INPUT, NULL);

    /* Allocate memory */
    m = cpl_malloc(sizeof(cpl_mask));
    m->nx = nx;
    m->ny = ny;
    m->data = data;

    return m;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Duplicates a cpl_mask
  @param    in  the mask to duplicate
  @return   1 newly allocated cpl_mask or NULL on error

  The returned object must be deallocated using cpl_mask_delete().

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if in is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_mask * cpl_mask_duplicate(const cpl_mask * in)
{
    if (in != NULL) {
        /* Create the output mask, cannot fail now */
        const size_t sz = (size_t)in->nx * (size_t)in->ny * sizeof(cpl_binary);

        return cpl_mask_wrap(in->nx, in->ny,
                             memcpy(cpl_malloc(sz), in->data, sz));
    }

    (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
    return NULL;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Delete a cpl_mask
  @param    m    cpl_mask to delete
  @return   void

   The function deallocates the memory used by the mask @em m.
   If @em m is @c NULL, nothing is done, and no error is set.

 */
/*----------------------------------------------------------------------------*/
void cpl_mask_delete(cpl_mask * m)
{
    if (m != NULL) {
        cpl_free(m->data);
        cpl_free(m);
    }
    return;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Delete a cpl_mask except the data array
  @param    m    cpl_mask to delete
  @return   A pointer to the data array or NULL if the input is NULL.
  @note The data array must be deallocated, otherwise a memory leak occurs.

 */
/*----------------------------------------------------------------------------*/
void * cpl_mask_unwrap(cpl_mask * m)
{
    void * data = NULL;

    if (m != NULL) {
        data = (void *) m->data;
        cpl_free(m);
    }

    return data;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get a pointer to the data part of the mask
  @param    in      the input mask
  @return   Pointer to the data or NULL on error

  The returned pointer refers to already allocated data.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_binary * cpl_mask_get_data(cpl_mask * in)
{
    cpl_ensure(in, CPL_ERROR_NULL_INPUT, NULL);
    return in->data;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get a pointer to the data part of the mask
  @param    in      the input mask
  @return   Pointer to the data or NULL on error
  @see cpl_mask_get_data
 */
/*----------------------------------------------------------------------------*/
const cpl_binary * cpl_mask_get_data_const(const cpl_mask * in)
{
    cpl_ensure(in, CPL_ERROR_NULL_INPUT, NULL);

    return in->data;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the value of a mask at a given position
  @param    self    The input mask
  @param    xpos    Pixel x position (FITS convention, 1 for leftmost)
  @param    ypos    Pixel y position (FITS convention, 1 for lowest)
  @return   The mask value or undefined if an error code is set

  The mask value can be either CPL_BINARY_0 or CPL_BINARY_1

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if xpos or ypos is out of bounds
 */
/*----------------------------------------------------------------------------*/
cpl_binary cpl_mask_get(const cpl_mask * self,
                        cpl_size         xpos,
                        cpl_size         ypos)
{
    /* Test entries */
    cpl_ensure(self != NULL,     CPL_ERROR_NULL_INPUT,    CPL_BINARY_0);
    cpl_ensure(xpos >  0,        CPL_ERROR_ILLEGAL_INPUT, CPL_BINARY_0);
    cpl_ensure(ypos >  0,        CPL_ERROR_ILLEGAL_INPUT, CPL_BINARY_0);
    cpl_ensure(xpos <= self->nx, CPL_ERROR_ILLEGAL_INPUT, CPL_BINARY_0);
    cpl_ensure(ypos <= self->ny, CPL_ERROR_ILLEGAL_INPUT, CPL_BINARY_0);

    return self->data[(xpos-1) + (ypos-1) * self->nx];
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Set a value in a mask at a given position
  @param    self      the input mask
  @param    xpos    Pixel x position (FITS convention, 1 for leftmost)
  @param    ypos    Pixel y position (FITS convention, 1 for lowest)
  @param    value   the value to set in the mask
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE

  The value can be either CPL_BINARY_0 or CPL_BINARY_1

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if xpos or ypos is out of bounds or if value
        is different from CPL_BINARY_0 and CPL_BINARY_1
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_mask_set(cpl_mask * self,
                            cpl_size   xpos,
                            cpl_size   ypos,
                            cpl_binary value)
{

    cpl_ensure_code(self != NULL,     CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(xpos > 0,         CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(ypos > 0,         CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(xpos <= self->nx, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(ypos <= self->ny, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(value == CPL_BINARY_0 || value == CPL_BINARY_1,
                    CPL_ERROR_ILLEGAL_INPUT);

    self->data[(xpos-1) + (ypos-1) * self->nx] = value;

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the x size of the mask
  @param    in      the input mask
  @return   The mask x size, or -1 on NULL input

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_mask_get_size_x(const cpl_mask * in)
{
    /* Test entries */
    cpl_ensure(in, CPL_ERROR_NULL_INPUT, -1);
    return in->nx;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the y size of the mask
  @param    in      the input mask
  @return   The mask y size, or -1 on NULL input

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_mask_get_size_y(const cpl_mask * in)
{
    /* Test entries */
    cpl_ensure(in, CPL_ERROR_NULL_INPUT, -1);
    return in->ny;
}


/*----------------------------------------------------------------------------*/
/**
  @brief    Dump a mask
  @param    self    mask to dump
  @param    llx     Lower left x position (FITS convention, 1 for leftmost)
  @param    lly     Lower left y position (FITS convention, 1 for lowest)
  @param    urx     Upper right x position (FITS convention)
  @param    ury     Upper right y position (FITS convention)
  @param    stream  Output stream, accepts @c stdout or @c stderr
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_FILE_IO if a write operation fails
  - CPL_ERROR_ACCESS_OUT_OF_RANGE if the defined window is not in the mask
  - CPL_ERROR_ILLEGAL_INPUT if the window definition is wrong (e.g llx > urx)
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_mask_dump_window(const cpl_mask * self, cpl_size llx,
                                    cpl_size lly, cpl_size urx,
                                    cpl_size ury, FILE * stream)
{
    const cpl_error_code err = CPL_ERROR_FILE_IO;
    cpl_size i, j;

    cpl_ensure_code(self   != NULL,  CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(stream != NULL,  CPL_ERROR_NULL_INPUT);

    cpl_ensure_code(llx > 0,         CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_ensure_code(llx <= urx,      CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(urx <= self->nx, CPL_ERROR_ACCESS_OUT_OF_RANGE);

    cpl_ensure_code(lly > 0,         CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_ensure_code(lly <= ury,      CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(ury <= self->ny, CPL_ERROR_ACCESS_OUT_OF_RANGE);

    cpl_ensure_code( fprintf(stream, "#----- mask: %" CPL_SIZE_FORMAT
                             " <= x <= %" CPL_SIZE_FORMAT ", %" CPL_SIZE_FORMAT
                             " <= y <= %" CPL_SIZE_FORMAT " -----\n",
                             llx, urx, lly, ury) > 0, err);

    cpl_ensure_code( fprintf(stream, "\tX\tY\tvalue\n") > 0, err);

    for (i = llx; i <= urx; i++) {
        for (j = lly; j <= ury; j++) {
            const cpl_binary value = self->data[(i-1) + (j-1) * self->nx];
            cpl_ensure_code( fprintf(stream, "\t%" CPL_SIZE_FORMAT "\t%"
                                     CPL_SIZE_FORMAT "\t%d\n", i, j, (int)value)
                             > 0, err);
        }
    }

    return CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
  @brief    Return CPL_TRUE iff a mask has no elements set (to CPL_BINARY_1)
  @param    self  The mask to search
  @return   CPL_TRUE iff the mask has no elements set (to CPL_BINARY_1)
  @see cpl_mask_is_empty_window()

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_boolean cpl_mask_is_empty(const cpl_mask * self)
{

    if (self == NULL) {
        (void)cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return CPL_FALSE;
    } else {
        const cpl_size idx = cpl_mask_get_first_window(self, 1, 1,
                                                       self->nx, self->ny,
                                                       CPL_BINARY_1);

        /* Propagate the error, if any */
        if (idx < -1) (void)cpl_error_set_where_();

        return idx == -1 ? CPL_TRUE : CPL_FALSE;
    }
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Return CPL_TRUE iff a mask has no elements set in the window
  @param    self  The mask to search
  @param    llx   Lower left x position (FITS convention, 1 for leftmost)
  @param    lly   Lower left y position (FITS convention, 1 for lowest)
  @param    urx   Upper right x position (FITS convention)
  @param    ury   Upper right y position (FITS convention)
  @return   CPL_TRUE iff the mask has no elements set (to CPL_BINARY_1)

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the window coordinates are not valid
 */
/*----------------------------------------------------------------------------*/
cpl_boolean cpl_mask_is_empty_window(const cpl_mask * self,
                                     cpl_size         llx,
                                     cpl_size         lly,
                                     cpl_size         urx,
                                     cpl_size         ury)
{
    const cpl_size idx = cpl_mask_get_first_window(self, llx, lly, urx, ury,
                                                   CPL_BINARY_1);

    /* Propagate the error, if any */
    if (idx < -1) (void)cpl_error_set_where_();

    return idx == -1 ? CPL_TRUE : CPL_FALSE;
}


/*----------------------------------------------------------------------------*/
/**
  @brief    Get the number of occurences of CPL_BINARY_1
  @param    in      the input mask
  @return   the number of occurences of CPL_BINARY_1 or -1 on error
  @see cpl_mask_count_window()

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_mask_count(const cpl_mask * in)
{

    /* Test inputs */
    cpl_ensure(in, CPL_ERROR_NULL_INPUT, -1);

    return cpl_mask_count_window(in, 1, 1, in->nx, in->ny);

}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the number of occurences of CPL_BINARY_1 in a window
  @param    self    The mask to count
  @param    llx     Lower left x position (FITS convention, 1 for leftmost)
  @param    lly     Lower left y position (FITS convention, 1 for lowest)
  @param    urx     Upper right x position (FITS convention)
  @param    ury     Upper right y position (FITS convention)
  @return   the number of occurences of CPL_BINARY_1 or -1 on error

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the window coordinates are invalid
 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_mask_count_window(const cpl_mask * self,
                               cpl_size         llx,
                               cpl_size         lly,
                               cpl_size         urx,
                               cpl_size         ury)
{
    /* Point to first row with element(s) to check */
    const cpl_binary * pi
        = self ? self->data + (size_t)self->nx * (size_t)(lly-1) : NULL;
    cpl_size           count = 0;


    cpl_ensure(self != NULL,     CPL_ERROR_NULL_INPUT,    -1);
    cpl_ensure(llx  > 0,         CPL_ERROR_ILLEGAL_INPUT, -1);
    cpl_ensure(llx  <= urx,      CPL_ERROR_ILLEGAL_INPUT, -1);
    cpl_ensure(urx  <= self->nx, CPL_ERROR_ILLEGAL_INPUT, -1);

    cpl_ensure(lly  > 0,         CPL_ERROR_ILLEGAL_INPUT, -1);
    cpl_ensure(lly  <= ury,      CPL_ERROR_ILLEGAL_INPUT, -1);
    cpl_ensure(ury  <= self->ny, CPL_ERROR_ILLEGAL_INPUT, -1);


    for (cpl_size j = lly - 1; j < ury; j++, pi += self->nx) {
        cpl_size i = llx - 1;
#ifdef CPL_MASK_WORD
        /* FIXME: Should really ensure byte-wise iteration to word-boundary */
        for (; i < CPL_MIN(llx - 1 + CPL_MASK_PAD2WORD2(llx - 1), urx); i++) {
            if (pi[i] != CPL_BINARY_0) count++;
        }
        /* Use multiplication to sum up all one-bytes in one word into the
           single (most significant byte) using an idea of Julian Taylor. */
        for (; i < (urx & ~CPL_MASK_PAD2); i+= CPL_MASK_WORD2) {
            const size_t countz = *(const size_t*)(pi + i)
                                + *(const size_t*)(pi + i + CPL_MASK_WORD);

           /* Instead of shifting one could also accumulate from an
              unsigned character pointing to the MSB of the product,
              but on one tested 64-bit platform this is not faster. */
            count += (countz * CPL_MASK_NOT) >> (CPL_MASK_PAD * 8);
        }
        /* Count remainder */
#endif
        for (; i < urx; i++) {
            if (pi[i] != CPL_BINARY_0) count++;
        }
    }

    return count;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Performs a logical AND of one mask onto another
  @param    in1   first mask, to be modified
  @param    in2   second mask
  @return   CPL_ERROR_NONE on success, otherwise the relevant #_cpl_error_code_
  @see cpl_image_and()

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the two masks have different sizes
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_mask_and(cpl_mask        *   in1,
                            const cpl_mask  *   in2)
{
    cpl_ensure_code(in1     != NULL,    CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(in2     != NULL,    CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(in1->nx == in2->nx, CPL_ERROR_INCOMPATIBLE_INPUT);
    cpl_ensure_code(in1->ny == in2->ny, CPL_ERROR_INCOMPATIBLE_INPUT);

    cpl_mask_and_(in1->data, NULL, in2->data, (size_t)(in1->nx * in1->ny));

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Performs a logical AND of one mask onto another
  @param    self     Pre-allocated mask to hold the result
  @param    first    First operand, or NULL for an in-place operation
  @param    second   Second operand
  @param    nxy      The number of elements
  @return   void
  @see cpl_mask_and()

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the two masks have different sizes
 */
/*----------------------------------------------------------------------------*/
void cpl_mask_and_(cpl_binary       * self,
                   const cpl_binary * first,
                   const cpl_binary * second,
                   size_t             nxy)
{
    const cpl_binary * myfirst = first ? first : self;

    if (myfirst != second) {
        cpl_mask_and__(self, first ? first : NULL, second, nxy);
    } else if (first != NULL) {
        assert( first == second );
        (void)memcpy(self, first, nxy);
    }
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Performs a logical OR of one mask onto another
  @param    in1   first mask, to be modified
  @param    in2   second mask
  @return   CPL_ERROR_NONE on success, otherwise the relevant #_cpl_error_code_
  @see cpl_image_or()

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the two masks have different sizes
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_mask_or(cpl_mask        *   in1,
                           const cpl_mask  *   in2)
{
    cpl_ensure_code(in1     != NULL,    CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(in2     != NULL,    CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(in1->nx == in2->nx, CPL_ERROR_INCOMPATIBLE_INPUT);
    cpl_ensure_code(in1->ny == in2->ny, CPL_ERROR_INCOMPATIBLE_INPUT);

    cpl_mask_or_(in1->data, NULL, in2->data, (size_t)(in1->nx * in1->ny));

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Performs a logical OR of one mask onto another
  @param    self     Pre-allocated mask to hold the result
  @param    first    First operand, or NULL for an in-place operation
  @param    second   Second operand
  @param    nxy      The number of elements
  @return   void
  @see cpl_mask_or()

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the two masks have different sizes
 */
/*----------------------------------------------------------------------------*/
void cpl_mask_or_(cpl_binary       * self,
                  const cpl_binary * first,
                  const cpl_binary * second,
                  size_t             nxy)
{
    const cpl_binary * myfirst = first ? first : self;

    if (myfirst != second) {
        cpl_mask_or__(self, first ? first : NULL, second, nxy);
    } else if (first != NULL) {
        assert( first == second );
        (void)memcpy(self, first, nxy);
    }
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Performs a logical XOR of one mask onto another
  @param    in1   first mask, to be modified
  @param    in2   second mask
  @return   CPL_ERROR_NONE on success, otherwise the relevant #_cpl_error_code_
  @see cpl_image_xor()
  @note Passing the same mask twice will clear it

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the two masks have different sizes
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_mask_xor(cpl_mask        *   in1,
                            const cpl_mask  *   in2)
{

    cpl_ensure_code(in1     != NULL,    CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(in2     != NULL,    CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(in1->nx == in2->nx, CPL_ERROR_INCOMPATIBLE_INPUT);
    cpl_ensure_code(in1->ny == in2->ny, CPL_ERROR_INCOMPATIBLE_INPUT);

    cpl_mask_xor_(in1->data, NULL, in2->data, (size_t)(in1->nx * in1->ny));

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Performs a logical XOR of one mask onto another
  @param    self     Pre-allocated mask to hold the result
  @param    first    First operand, or NULL for an in-place operation
  @param    second   Second operand
  @param    nxy      The number of elements
  @return   void
  @see cpl_mask_xor()

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the two masks have different sizes
 */
/*----------------------------------------------------------------------------*/
void cpl_mask_xor_(cpl_binary       * self,
                   const cpl_binary * first,
                   const cpl_binary * second,
                   size_t             nxy)
{
    const cpl_binary * myfirst = first ? first : self;

    if (myfirst != second) {
        cpl_mask_xor__(self, first ? first : NULL, second, nxy);
    } else {
        (void)memset(self, 0, nxy);
    }
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Performs a logical NOT on a mask
  @param    in   mask to be modified
  @return   CPL_ERROR_NONE on success, otherwise the relevant #_cpl_error_code_
  @see cpl_image_not()

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_mask_not(cpl_mask * in)
{
    cpl_ensure_code(in     != NULL,    CPL_ERROR_NULL_INPUT);

    cpl_mask_xor_scalar(in->data, NULL, CPL_MASK_NOT8,
                        (size_t)(in->nx * in->ny));

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Collapse a mask
  @param    in    input mask to collapse
  @param    dir   collapsing direction
  @return   the newly allocated mask or NULL on error
  @note The returned mask must be deallocated using cpl_mask_delete().

  direction 0 collapses along y, producing a nx by 1 mask
  direction 1 collapses along x, producing a 1 by ny mask

  The resulting mask element is set to CPL_BINARY_1 iff all elements of the
  associated column (resp. row) in the input mask are set to CPL_BINARY_1.

  @verbatim
  Direction 0 collapse:

  1 0 1    Input mask.
  0 1 1
  0 0 1
  -----
  0 0 1    Only the third element is set to CPL_BINARY_1 since all input
           elements of that column are set to CPL_BINARY_1.
  @endverbatim

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT
  - CPL_ERROR_ILLEGAL_INPUT
 */
/*----------------------------------------------------------------------------*/
cpl_mask * cpl_mask_collapse_create(const cpl_mask  *   in,
                                    int                 dir)
{
    cpl_mask         * out;
    const cpl_binary * pin;
    cpl_size           j;

    /* Check entries */
    cpl_ensure(in != NULL, CPL_ERROR_NULL_INPUT, NULL);

    /* Get access to bpm_in */
    pin = in->data;

    /* Check the collapsing direction */
    if (dir == 0) {
        cpl_binary   * pout = (cpl_binary*)cpl_malloc((size_t)in->nx
                                                      * sizeof(cpl_binary));
        /* Process whole words */
        const cpl_size nword = in->nx / (cpl_size)sizeof(size_t);

        /* Skip first + last CPL_BINARY_0 - valued columns in out */
        /* This overhead is accepted because in the typical case out
           will have many CPL_BINARY_0;s, leading to a significant
           reduction in iterations */
        cpl_size ifirst   = 0;
        cpl_size ilast    = nword-1; /* Can't use unsigned: nword may be zero */
        /* Unlike in perl, in C ?: cannot be used to choose the assignee */
        cpl_size * pfirst = &ifirst;
        cpl_size * plast  = &ilast;

        /* Initialize out with 1st row of in */
        size_t     * word1 = (size_t*)memcpy(pout, pin, (size_t)in->nx);
        pin += in->nx;

        /* To achieve stride-1 access remaining rows of pin are and'ed onto
           pout.
           No more iterations are needed if pout is all CPL_BINARY_0.
        */
        for (j = 1; j < in->ny; j++, pin += in->nx) {
            const size_t * word2  = (const size_t*)pin;
            unsigned       donext = 0; /* Done iff out is all _0 */
            const cpl_size klast = ilast; /* ilast is updated via plast */
            cpl_size       i;

            for (i = ifirst; i < 1 + klast; i++) {
                /* donext is also used to ensure an update of ifirst the first
                   time a CPL_BINARY_1 is set, and to update ilast for other
                   occurences (including the last one) of CPL_BINARY_1 */
                if ((word1[i] &= word2[i])) *(donext++ ? plast : pfirst) = i;
            }

            /* Typical masks will fall through here */
            for (i = nword * (cpl_size)sizeof(size_t); i < in->nx; i++) {
                if ((pout[i] &= pin[i])) donext = 1;
            }
            if (!donext) break;
        }

        out = cpl_mask_wrap(in->nx, 1, pout);

    } else if (dir == 1) {

        /* Using memchr() this direction is about three times faster
           for some 4k by 4k masks on a 32 bit Intel Xeon */

        cpl_binary * pout = (cpl_binary*)cpl_malloc((size_t)in->ny
                                                    * sizeof(cpl_binary));

        /* Each iteration may do multiple rows. Therefore, if the last computed
           value is CPL_BINARY_1, j will be 1 + in->ny after the loop */
        for (j = 0; j < in->ny; j++) {

            /* pfind points to 1st element in the j'th row */
            const cpl_binary * pfind = pin + (size_t)in->nx * (size_t)j;
            /* szfind is the remaining number of elements to search */
            const size_t szfind = (size_t)in->nx * (size_t)(in->ny - j);

            /* Look for next CPL_BINARY_0 */
            /* In the worst case the last column is all CPL_BINARY_0, the rest
               is CPL_BINARY_1, then ny memchr() calls have to be made, each
               scanning nx elements */
            const cpl_binary * pnext = memchr(pfind, CPL_BINARY_0, szfind);

            /* Row numbers less than this one are set to CPL_BINARY_1 */
            const cpl_size row0 = pnext != NULL
                ? (cpl_size)((size_t)pnext-(size_t)pin) / in->nx : in->ny;

            for (; j < row0; j++) {
                pout[j] = CPL_BINARY_1;
            }

            /* Unless we are done, collapse the j'th row to CPL_BINARY_0 */
            if (pnext != NULL) pout[j] = CPL_BINARY_0;
        }

        out = cpl_mask_wrap(1, in->ny, pout);

    } else {
        out = NULL;
        (void)cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
    }

   /* out is NULL if dir is illegal */
    return out;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Extract a mask from an other one
  @param    in      input mask
  @param    llx     Lower left x position (FITS convention, 1 for leftmost)
  @param    lly     Lower left y position (FITS convention, 1 for lowest)
  @param    urx     Upper right x position (FITS convention)
  @param    ury     Upper right y position (FITS convention)
  @return   1 newly allocated mask or NULL on error.
  @note The returned mask must be deallocated using cpl_mask_delete().

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_ILLEGAL_INPUT if the zone falls outside the mask
  - CPL_ERROR_NULL_INPUT if the input mask is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_mask * cpl_mask_extract(const cpl_mask * in,
                            cpl_size         llx,
                            cpl_size         lly,
                            cpl_size         urx,
                            cpl_size         ury)
{

    cpl_errorstate prestate = cpl_errorstate_get();
    cpl_mask * self = cpl_mask_extract_(in, llx, lly, urx, ury);

    if (self == NULL) {
        if (cpl_errorstate_is_equal(prestate)) {
            self = cpl_mask_new(urx - llx + 1, ury - lly + 1);
        } else {
            (void)cpl_error_set_where_();
        }
    }

    return self;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Rotate a mask by a multiple of 90 degrees clockwise.
  @param    self    Mask to rotate in place
  @param    rot     The multiple: -1 is a rotation of 90 deg counterclockwise.
  @return   CPL_ERROR_NONE on success, otherwise the relevant #_cpl_error_code_
  @see      cpl_image_turn()

  rot may be any integer value, its modulo 4 determines the rotation:
  - -3 to turn 270 degrees counterclockwise.
  - -2 to turn 180 degrees counterclockwise.
  - -1 to turn  90 degrees counterclockwise.
  -  0 to not turn
  - +1 to turn  90 degrees clockwise (same as -3)
  - +2 to turn 180 degrees clockwise (same as -2).
  - +3 to turn 270 degrees clockwise (same as -1).

  The definition of the rotation relies on the FITS convention:
  The lower left corner of the image is at (1,1), x increasing from left to
  right, y increasing from bottom to top.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if self is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_mask_turn(cpl_mask * self, int rot)
{
    cpl_mask         * loc;
    const cpl_binary * ploc;
    cpl_binary       * pself;
    cpl_size           i, j;

    /* Check entries */
    cpl_ensure_code(self, CPL_ERROR_NULL_INPUT);

    rot %= 4;
    if (rot < 0) rot += 4;


    /* Create the local mask */
    loc = cpl_mask_duplicate(self);
    ploc = cpl_mask_get_data_const(loc);
    pself = cpl_mask_get_data(self);

    /* Compute the new positions */
    /* Switch on the kind of rotation */
    /* rot is 0, 1, 2 or 3. */
    switch (rot) {
        case 1:
            self->nx = loc->ny;
            self->ny = loc->nx;
            pself += ((self->ny)-1)* (self->nx);
            for (j=0; j<(self->nx); j++) {
                for (i=0; i<(self->ny); i++) {
                    *pself = *ploc++;
                    pself -= (self->nx);
                }
                pself += (self->nx)*(self->ny)+1;
            }
            break;
        case 2:
            for (i=0; i<(self->nx)*(self->ny); i++)
                pself[i] = ploc[(self->ny)*(self->nx)-1-i];
            break;
        case 3:
            self->nx = loc->ny;
            self->ny = loc->nx;
            pself += (self->nx)-1;
            for (j=0; j<(self->nx); j++) {
                for (i=0; i<(self->ny); i++) {
                    *pself = *ploc++;
                    pself += (self->nx);
                }
                pself -= (self->nx)*(self->ny)+1;
            }
            break;
        default:
            break;
    }
    cpl_mask_delete(loc);

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Shift a mask
  @param    self Mask to shift in place
  @param    dx   Shift in X
  @param    dy   Shift in Y
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
  @see      cpl_image_turn()

  The 'empty zone' in the shifted mask is set to CPL_BINARY_1.
  The shift values have to be valid:
  -nx < dx < nx and -ny < dy < ny

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if in is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the offsets are too big
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_mask_shift(cpl_mask * self,
                              cpl_size   dx,
                              cpl_size   dy)
{
    cpl_ensure_code(self != NULL, CPL_ERROR_NULL_INPUT);

    return cpl_tools_shift_window(self->data, sizeof(*(self->data)),
                                  self->nx, self->ny, CPL_BINARY_1, dx, dy)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Insert a mask in an other one
  @param    in1     mask in which in2 is inserted
  @param    in2     mask to insert
  @param    x_pos   the x pixel position in in1 where the lower left pixel of
                    in2 should go (from 1 to the x size of in1)
  @param    y_pos   the y pixel position in in1 where the lower left pixel of
                    in2 should go (from 1 to the y size of in1)
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if in1 or in2 is NULL
  - CPL_ERROR_ILLEGAL_INPUT if x_pos, y_pos is outside in1
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_mask_copy(cpl_mask        *   in1,
                             const cpl_mask  *   in2,
                             cpl_size            x_pos,
                             cpl_size            y_pos)
{
    cpl_binary       * pin1;
    const cpl_binary * pin2;
    cpl_size           urx, ury;
    size_t             linesz;

    /* Test entries */
    cpl_ensure_code(in1   != NULL,    CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(in2   != NULL,    CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(x_pos >= 1,       CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(x_pos <= in1->nx, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(y_pos >= 1,       CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(y_pos <= in1->ny, CPL_ERROR_ILLEGAL_INPUT);

    /* Define the zone to modify in in1: x_pos, y_pos, urx, ury */
    urx = CX_MIN(in1->nx, in2->nx + x_pos - 1);
    ury = CX_MIN(in1->ny, in2->ny + y_pos - 1);

    /* Get access to the data */
    pin1 = cpl_mask_get_data(in1) + (y_pos - 1) * in1->nx;
    pin2 = cpl_mask_get_data_const(in2);
    linesz = (size_t)(urx - (x_pos-1)) * sizeof(cpl_binary);

    if (x_pos == 1 && urx == in1->nx && in1->nx == in2->nx) {
        /* The zone consists of whole lines in both in1 and in2 */
        (void)memcpy(pin1, pin2, (size_t)(ury - (y_pos - 1)) * linesz);
    } else {
        cpl_size j;

        pin1 += (x_pos - 1);

        /* Loop on the zone */
        for (j = y_pos - 1; j < ury; j++, pin1 += in1->nx, pin2 += in2->nx) {
            (void)memcpy(pin1, pin2, linesz);
        }
    }
    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Flip a mask on a given mirror line.
  @param    in      mask to flip
  @param    angle   mirror line in polar coord. is theta = (PI/4) * angle
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE

  angle can take one of the following values:
  - 0 (theta=0) to flip the image around the horizontal
  - 1 (theta=pi/4) to flip the image around y=x
  - 2 (theta=pi/2) to flip the image around the vertical
  - 3 (theta=3pi/4) to flip the image around y=-x

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if in is NULL
  - CPL_ERROR_ILLEGAL_INPUT if angle is not as specified
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_mask_flip(cpl_mask * in, int angle)
{
    cpl_binary * pin;
    cpl_size nx, ny;
    cpl_size i, j;

    /* Check entries */
    cpl_ensure_code(in != NULL, CPL_ERROR_NULL_INPUT);

    /* Initialise */
    pin = cpl_mask_get_data(in);
    nx = in->nx;
    ny = in->ny;

    /* Compute the new positions */
    /* Switch on the kind of flipping */
    switch (angle) {
    case 0: {
        const size_t rowsize = (size_t)nx * sizeof(cpl_binary);
        cpl_binary row[nx];
        cpl_binary * pfirst = pin;
        cpl_binary * plast  = pin + (ny-1) * nx;

        for (j = 0; j < ny/2; j++, pfirst += nx, plast -= nx) {
            (void)memcpy(row,    pfirst, rowsize);
            (void)memcpy(pfirst, plast,  rowsize);
            (void)memcpy(plast,  row,    rowsize);
        }
        break;
    }
    case 2: {

        for (j = 0; j < ny; j++, pin += nx) {
            for (i = 0; i < nx/2; i++) {
                const cpl_binary tmp = pin[i];
                pin[i] = pin[nx-1-i];
                pin[nx-1-i] = tmp;
            }
        }
        break;
    }
    case 1: {
        if (nx == ny) {
            cpl_binary * pt = pin;

            for (j = 0; j < nx; j++, pt += nx) {
                for (i = 0; i < j; i++) {
                    const cpl_binary tmp = pt[i];
                    pt[i] = pin[j + i * nx];
                    pin[j + i * nx] = tmp;
                }
            }
        } else {
            /* Duplicate the input mask */
            cpl_mask * loc = cpl_mask_duplicate(in);
            const cpl_binary * ploc = cpl_mask_get_data_const(loc);

            in->nx = ny;
            in->ny = nx;
            for (j=0; j<nx; j++) {
                for (i=0; i<ny; i++) {
                    *pin++ = *ploc;
                    ploc += nx;
                }
                ploc -= (nx*ny-1);
            }
            cpl_mask_delete(loc);
        }
        break;
    }
    case 3: {
        if (nx == ny) {
            cpl_binary * pt = pin;

            for (j = 0; j < nx; j++, pt += nx) {
                for (i = 0; i < nx - j; i++) {
                    const cpl_binary tmp = pt[i];
                    pt[i] = pin[(nx - 1 - j) + (nx - 1 - i) * nx];
                    pin[(nx - 1 - j) + (nx - 1 - i) * nx] = tmp;
                }
            }
        } else {
            /* Duplicate the input mask */
            cpl_mask * loc = cpl_mask_duplicate(in);
            const cpl_binary * ploc = cpl_mask_get_data_const(loc);

            in->nx = ny;
            in->ny = nx;
            ploc += (nx*ny-1);
            for (j=0; j<nx; j++) {
                for (i=0; i<ny; i++) {
                    *pin++ = *ploc;
                    ploc -= nx;
                }
                ploc += (nx*ny-1);
            }
            cpl_mask_delete(loc);
        }
        break;
    }
    default:
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
    }

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Reorganize the pixels in a mask
  @param    in      mask to collapse
  @param    nb_cut  the number of cut in x and y
  @param    new_pos array with the nb_cut^2 new positions
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
  @see      cpl_image_move()

  nb_cut must be positive and divide the size of the input mask in x and y.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if in or new_pos is NULL
  - CPL_ERROR_ILLEGAL_INPUT if nb_cut is not as requested.
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_mask_move(cpl_mask       * in,
                             cpl_size         nb_cut,
                             const cpl_size * new_pos)
{
    cpl_size            tile_sz_x, tile_sz_y, tile_x, tile_y;
    cpl_size            npos, opos;
    cpl_mask        *   loc;
    const cpl_binary*   ploc;
    cpl_binary      *   pin;
    cpl_size            i, j, k, l;

    /* Check entries */
    cpl_ensure_code(in      != NULL,      CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(new_pos != NULL,      CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(nb_cut > 0,           CPL_ERROR_ILLEGAL_INPUT);

    tile_sz_x = in->nx / nb_cut;
    tile_sz_y = in->ny / nb_cut;

    cpl_ensure_code(tile_sz_x * nb_cut == in->nx, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(tile_sz_y * nb_cut == in->ny, CPL_ERROR_ILLEGAL_INPUT);

    /* Create local mask */
    loc = cpl_mask_duplicate(in);
    ploc = cpl_mask_get_data_const(loc);
    pin = cpl_mask_get_data(in);

    /* Loop to move the pixels */
    for (j=0; j<nb_cut; j++) {
        for (i=0; i<nb_cut; i++) {
            tile_x = (new_pos[i+j*nb_cut]-1) % nb_cut;
            tile_y = (new_pos[i+j*nb_cut]-1) / nb_cut;
            for (l=0; l<tile_sz_y; l++) {
                for (k=0; k<tile_sz_x; k++) {
                    opos=(k+i*tile_sz_x)      + in->nx * (l+j*tile_sz_y);
                    npos=(k+tile_x*tile_sz_x) + in->nx * (l+tile_y*tile_sz_y);
                    pin[npos] = ploc[opos];
                }
            }
        }
    }
    cpl_mask_delete(loc);

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Subsample a mask
  @param    in      input mask
  @param    xstep   Take every xstep pixel in x
  @param    ystep   Take every ystep pixel in y
  @return   the newly allocated mask or NULL on error case
  @see      cpl_image_extract_subsample()

  The returned mask must be deallocated using cpl_mask_delete().

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if in is NULL
  - CPL_ERROR_ILLEGAL_INPUT if xstep and ystep are not greater than zero
 */
/*----------------------------------------------------------------------------*/
cpl_mask *cpl_mask_extract_subsample(const cpl_mask *in,
                                     cpl_size        xstep,
                                     cpl_size        ystep)
{
    cpl_size          new_nx, new_ny;
    cpl_mask         *out;
    cpl_binary       *pout;
    const cpl_binary *pin;
    cpl_size          i, j;


    cpl_ensure(in    != NULL, CPL_ERROR_NULL_INPUT,    NULL);
    cpl_ensure(xstep > 0,     CPL_ERROR_ILLEGAL_INPUT, NULL);
    cpl_ensure(ystep > 0,     CPL_ERROR_ILLEGAL_INPUT, NULL);

    new_nx = (in->nx - 1)/xstep + 1;
    new_ny = (in->ny - 1)/ystep + 1;

    out = cpl_mask_new(new_nx, new_ny);
    pin = cpl_mask_get_data_const(in);
    pout = cpl_mask_get_data(out);

    for (j = 0; j < in->ny; j += ystep, pin += ystep*in->nx)
        for (i = 0; i < in->nx; i += xstep)
            *pout++ = pin[i];

    return out;
}



/*----------------------------------------------------------------------------*/
/**
  @brief  Filter a mask using a binary kernel
  @param  self   Pre-allocated mask to hold the filtered result
  @param  other  Mask to filter
  @param  kernel Elements to use, if set to CPL_BINARY_1
  @param  filter CPL_FILTER_EROSION, CPL_FILTER_DILATION, CPL_FILTER_OPENING,
                 CPL_FILTER_CLOSING
  @param  border CPL_BORDER_NOP, CPL_BORDER_ZERO or CPL_BORDER_COPY
  @return CPL_ERROR_NONE or the relevant CPL error code

  The two masks must have equal dimensions.

  The kernel must have an odd number of rows and an odd number of columns.

  At least one kernel element must be set to CPL_BINARY_1.

  For erosion and dilation:
  In-place filtering is not supported, but the input buffer may overlap all but
  the 1+h first rows of the output buffer, where 1+2*h is the number of rows in
  the kernel.

  For opening and closing:
  Opening is implemented as an erosion followed by a dilation, and closing
  is implemented as a dilation followed by an erosion. As such a temporary,
  internal buffer the size of self is allocated and used. Consequently,
  in-place opening and closing is supported with no additional overhead,
  it is achieved by passing the same mask as both self and other.

  Duality and idempotency:
    Erosion and Dilation have the duality relations:
    not(dil(A,B)) = er(not(A), B) and
    not(er(A,B)) = dil(not(A), B).

    Opening and closing have similar duality relations:
    not(open(A,B)) = close(not(A), B) and
    not(close(A,B)) = open(not(A), B).

    Opening and closing are both idempotent, i.e.
    open(A,B) = open(open(A,B),B) and
    close(A,B) = close(close(A,B),B).

  The above duality and idempotency relations do _not_ hold on the mask border
  (with the currently supported border modes).

  Unnecessary large kernels:
  Adding an empty border to a given kernel should not change the outcome of
  the filtering. However doing so widens the border of the mask to be filtered
  and therefore has an effect on the filtering of the mask border. Since an
  unnecessary large kernel is also more costly to apply, such kernels should
  be avoided.

   @par A 1 x 3 erosion filtering example (error checking omitted for brevity)
   @code
     cpl_mask * kernel = cpl_mask_new(1, 3); 
     cpl_mask_not(kernel);
     cpl_mask_filter(filtered, raw, kernel, CPL_FILTER_EROSION,
                                            CPL_BORDER_NOP);
     cpl_mask_delete(kernel);
   @endcode

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL.
  - CPL_ERROR_ILLEGAL_INPUT if the kernel has a side of even length.
  - CPL_ERROR_DATA_NOT_FOUND If the kernel is empty.
  - CPL_ERROR_ACCESS_OUT_OF_RANGE If the kernel has a side longer than the
                                  input mask.
  - CPL_ERROR_INCOMPATIBLE_INPUT If the input and output masks have
                                 incompatible sizes.
  - CPL_ERROR_UNSUPPORTED_MODE If the output pixel buffer overlaps the input
                               one (or the kernel), or the border/filter mode
                               is unsupported.
 */
/*----------------------------------------------------------------------------*/

cpl_error_code cpl_mask_filter(cpl_mask * self, const cpl_mask * other,
                               const cpl_mask * kernel,
                               cpl_filter_mode filter,
                               cpl_border_mode border)
{

    /* Modified from cpl_image_filter_mask() :-((((( */

    const cpl_size nsx    = cpl_mask_get_size_x(self);
    const cpl_size nsy    = cpl_mask_get_size_y(self);
    const cpl_size nx     = cpl_mask_get_size_x(other);
    const cpl_size ny     = cpl_mask_get_size_y(other);
    const cpl_size mx     = cpl_mask_get_size_x(kernel);
    const cpl_size my     = cpl_mask_get_size_y(kernel);
    const cpl_size hsizex = mx >> 1;
    const cpl_size hsizey = my >> 1;
    const cpl_binary * pmask  = cpl_mask_get_data_const(kernel);
    const cpl_binary * pother = cpl_mask_get_data_const(other);
    cpl_binary       * pself  = cpl_mask_get_data(self);
    /* assert( sizeof(cpl_binary) == 1 ) */
    const cpl_binary * polast = pother + nx * ny;
    const cpl_binary * psrows = pself + nsx * (1 + hsizey);
    /* pmask may not overlap pself at all */
    const cpl_binary * pmlast = pmask + mx * my;
    const cpl_binary * pslast = pself + nsx * nsy;
    /* In filtering it is generally faster with a special case for the 
       full kernel. Some tests indicate that this is not the case for mask
       filtering. This may be due to the typically small kernels
       - and the simple operations involved.
    */
    void (*filter_func)(cpl_binary *, const cpl_binary *, const cpl_binary *,
                        cpl_size, cpl_size, cpl_size, cpl_size,
                        cpl_border_mode) CPL_ATTR_NONNULL;


    cpl_ensure_code(self   != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(other  != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(kernel != NULL, CPL_ERROR_NULL_INPUT);

    if (filter == CPL_FILTER_OPENING || filter == CPL_FILTER_CLOSING) {
        cpl_ensure_code(border == CPL_BORDER_NOP ||
                        border == CPL_BORDER_ZERO ||
                        border == CPL_BORDER_COPY,
                        CPL_ERROR_UNSUPPORTED_MODE);
    } else {
        /* pself has to be above all of the other buffer, or */
        /* ...pother has to be above the first hsize+1 rows of pself */
        cpl_ensure_code(pself >= polast || pother >= psrows,
                        CPL_ERROR_UNSUPPORTED_MODE);
        cpl_ensure_code(border == CPL_BORDER_NOP || border == CPL_BORDER_ZERO ||
                        border == CPL_BORDER_COPY, CPL_ERROR_UNSUPPORTED_MODE);
    }

    /* If this check fails, the caller is doing something really weird... */
    cpl_ensure_code(pmask >= pslast || pself >= pmlast,
                    CPL_ERROR_UNSUPPORTED_MODE);

    /* Only odd-sized masks allowed */
    cpl_ensure_code((mx&1) == 1, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code((my&1) == 1, CPL_ERROR_ILLEGAL_INPUT);

    cpl_ensure_code(mx <= nx, CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_ensure_code(my <= ny, CPL_ERROR_ACCESS_OUT_OF_RANGE);

#ifdef CPL_MASK_FILTER_CROP
    if (border == CPL_BORDER_CROP) {
        cpl_ensure_code(nsx == nx - 2 * hsizex, CPL_ERROR_INCOMPATIBLE_INPUT);

        cpl_ensure_code(nsy == ny - 2 * hsizey, CPL_ERROR_INCOMPATIBLE_INPUT);

    } else
#endif
        {
        cpl_ensure_code(nsx == nx, CPL_ERROR_INCOMPATIBLE_INPUT);

        cpl_ensure_code(nsy == ny, CPL_ERROR_INCOMPATIBLE_INPUT);

    }

    cpl_ensure_code(!cpl_mask_is_empty(kernel), CPL_ERROR_DATA_NOT_FOUND);

    filter_func = NULL;

#ifdef CPL_MASK_NOT
    if (filter == CPL_FILTER_EROSION) {
        filter_func = cpl_mask_erosion_;
    } else if (filter == CPL_FILTER_DILATION) {
        filter_func = cpl_mask_dilation_;
    } else if (filter == CPL_FILTER_OPENING) {
        filter_func = cpl_mask_opening_;
    } else if (filter == CPL_FILTER_CLOSING) {
        filter_func = cpl_mask_closing_;
    }
#endif

    if (filter_func != NULL) {

        /* Pad kernel rows to a multiple of four/eight bytes so each instruction
           can process four/eight bytes. */
        const cpl_size mxe   = 1 + (mx | CPL_MASK_PAD);
        cpl_mask * meven = cpl_mask_new(mxe, my);
        cpl_binary * even = cpl_mask_get_data(meven);

        cpl_mask_copy(meven, kernel, 1, 1);
        filter_func(pself, pother, even, nx, ny, hsizex, hsizey, border);
        cpl_mask_delete(meven);

    } else {
        return cpl_error_set_message_(CPL_ERROR_UNSUPPORTED_MODE,
                                      "filter=%u. border=%u",
                                      (unsigned)filter, (unsigned)border);
    }

    return CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
  @brief    Compute a morphological opening
  @param    in      input mask to filter
  @param    ker     binary kernel (0 for 0, any other value is considered as 1)
  @return   CPL_ERROR_NONE on success or the #_cpl_error_code_ on failure
  @see cpl_mask_filter()
  @deprecated Replace this call with
     cpl_mask_filter() using CPL_FILTER_OPENING and CPL_BORDER_ZERO.

 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_mask_opening(
        cpl_mask            *   in,
        const cpl_matrix    *   ker)
{

    cpl_mask * mask = cpl_mask_new_from_matrix(ker, FLT_MIN);

    const cpl_error_code error =
        cpl_mask_filter(in, in, mask, CPL_FILTER_OPENING, CPL_BORDER_ZERO);

    cpl_mask_delete(mask);

    return cpl_error_set_(error);
}


/*----------------------------------------------------------------------------*/
/**
  @brief    Compute a morphological closing
  @param    in      input mask to filter
  @param    ker     binary kernel (0 for 0, any other value is considered as 1)
  @return   CPL_ERROR_NONE on success or the #_cpl_error_code_ on failure
  @see cpl_mask_filter()
  @deprecated Replace this call with
    cpl_mask_filter() using CPL_FILTER_CLOSING and CPL_BORDER_ZERO.

 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_mask_closing(
        cpl_mask            *   in,
        const cpl_matrix    *   ker)
{

    cpl_mask * mask = cpl_mask_new_from_matrix(ker, FLT_MIN);

    const cpl_error_code error =
        cpl_mask_filter(in, in, mask, CPL_FILTER_CLOSING, CPL_BORDER_ZERO);

    cpl_mask_delete(mask);

    return cpl_error_set_(error);
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Compute a morphological erosion
  @param    in      input mask to filter
  @param    ker     binary kernel (0 for 0, any other value is considered as 1)
  @return   CPL_ERROR_NONE on success or the #_cpl_error_code_ on failure
  @see cpl_mask_filter()
  @deprecated Replace this call with
     cpl_mask_filter() using CPL_FILTER_EROSION and CPL_BORDER_ZERO.

 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_mask_erosion(
        cpl_mask            *   in,
        const cpl_matrix    *   ker)
{

    cpl_mask * mask = cpl_mask_new_from_matrix(ker, FLT_MIN);
    cpl_mask * self = cpl_mask_duplicate(in);

    const cpl_error_code error =
        cpl_mask_filter(in, self, mask, CPL_FILTER_EROSION, CPL_BORDER_ZERO);

    cpl_mask_delete(self);
    cpl_mask_delete(mask);

    return cpl_error_set_(error);
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Compute a morphological dilation
  @param    in      input mask to filter
  @param    ker     binary kernel (0 for 0, any other value is considered as 1)
  @return   CPL_ERROR_NONE on success or the #_cpl_error_code_ on failure
  @see cpl_mask_filter()
  @deprecated Replace this call with
    cpl_mask_filter() using CPL_FILTER_DILATION and CPL_BORDER_ZERO.

 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_mask_dilation(
        cpl_mask            *   in,
        const cpl_matrix    *   ker)
{

    cpl_mask * mask = cpl_mask_new_from_matrix(ker, FLT_MIN);
    cpl_mask * self = cpl_mask_duplicate(in);

    const cpl_error_code error =
        cpl_mask_filter(in, self, mask, CPL_FILTER_DILATION, CPL_BORDER_ZERO);

    cpl_mask_delete(self);
    cpl_mask_delete(mask);

    return cpl_error_set_(error);
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Select parts of an image with provided thresholds
  @param    self    Mask to flag according to threshold
  @param    image   Image to threshold.
  @param    lo_cut  Lower bound for threshold.
  @param    hi_cut  Higher bound for threshold.
  @param    inval   This value (CPL_BINARY_1 or CPL_BINARY_0) is assigned where
                    the pixel value is not marked as rejected and is strictly
                    inside the provided interval.
                    The other positions are assigned the other value.
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error

  The input image type can be CPL_TYPE_DOUBLE, CPL_TYPE_FLOAT or CPL_TYPE_INT.
  If lo_cut is greater than or equal to hi_cut, then the mask is filled with
  outval.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_UNSUPPORTED_MODE if the pixel type is unsupported
  - CPL_ERROR_INCOMPATIBLE_INPUT if the mask and image have different sizes
  - CPL_ERROR_ILLEGAL_INPUT if inval is not one of CPL_BINARY_1 or CPL_BINARY_0
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_mask_threshold_image(cpl_mask * self,
                                        const cpl_image * image,
                                        double lo_cut, double hi_cut,
                                        cpl_binary inval)
{

    const cpl_size nx     = cpl_mask_get_size_x(self);
    const cpl_size ny     = cpl_mask_get_size_y(self);
    const cpl_size npix   = nx * ny;
    const void   * pixels = cpl_image_get_data_const(image);
    cpl_binary   * bpm    = cpl_mask_get_data(self);
    const cpl_mask   * bmask = cpl_image_get_bpm_const(image);
    const cpl_binary * pmask = bmask ? cpl_mask_get_data_const(bmask) : NULL;
    const cpl_binary outval  = inval == CPL_BINARY_1 ? CPL_BINARY_0
        : CPL_BINARY_1;
    int i;

    cpl_ensure_code(bpm    != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(pixels != NULL, CPL_ERROR_NULL_INPUT);

    cpl_ensure_code(nx == cpl_image_get_size_x(image),
                    CPL_ERROR_INCOMPATIBLE_INPUT);
    cpl_ensure_code(ny == cpl_image_get_size_y(image),
                    CPL_ERROR_INCOMPATIBLE_INPUT);
    cpl_ensure_code(inval == CPL_BINARY_1 || inval == CPL_BINARY_0,
                    CPL_ERROR_ILLEGAL_INPUT);

    /* Switch on the input data type */
    switch (cpl_image_get_type(image)) {
    case CPL_TYPE_DOUBLE: {
        const double * pi = (const double*)pixels;
        for (i=0; i < npix; i++) {
            bpm[i] = (pmask == NULL || !pmask[i]) &&
                lo_cut < pi[i] && pi[i] < hi_cut ? inval : outval;
        }
        break;
    }
    case CPL_TYPE_FLOAT: {
        const float * pi = (const float*)pixels;
        for (i=0; i < npix; i++) {
            bpm[i] = (pmask == NULL || !pmask[i]) &&
                lo_cut < pi[i] && pi[i] < hi_cut ? inval : outval;
        }
        break;
    }
    case CPL_TYPE_INT: {
        const int * pi = (const int*)pixels;
        for (i=0; i < npix; i++) {
            bpm[i] = (pmask == NULL || !pmask[i]) &&
                lo_cut < pi[i] && pi[i] < hi_cut ? inval : outval;
        }
        break;
    }
    default:
        return cpl_error_set_(CPL_ERROR_UNSUPPORTED_MODE);
    }

    return CPL_ERROR_NONE;
}
/*----------------------------------------------------------------------------*/
/**
  @brief    Select parts of an image with provided thresholds
  @param    in      Image to threshold.
  @param    lo_cut  Lower bound for threshold.
  @param    hi_cut  Higher bound for threshold.
  @return   1 newly allocated mask or NULL on error
  @note The returned mask must be deallocated with cpl_mask_delete()
  @see cpl_mask_threshold_image()

 */
/*----------------------------------------------------------------------------*/
cpl_mask * cpl_mask_threshold_image_create(const cpl_image * in,
                                           double            lo_cut,
                                           double            hi_cut)
{
    cpl_mask * self = cpl_mask_new(cpl_image_get_size_x(in),
                                   cpl_image_get_size_y(in));

    if (cpl_mask_threshold_image(self, in, lo_cut, hi_cut, CPL_BINARY_1)) {
        cpl_error_set_where_();
        cpl_mask_delete(self);
        self = NULL;
    }

    return self;
}


/*----------------------------------------------------------------------------*/
/**
  @brief    Save a mask to a FITS file
  @param    self       mask to write to disk
  @param    filename   Name of the file to write
  @param    pl         Property list for the output header or NULL
  @param    mode       The desired output options
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error
  @see cpl_propertylist_save()

  This function saves a mask to a FITS file. If a property list
  is provided, it is written to the header where the mask is written.

  The type used in the file is CPL_TYPE_UCHAR (8 bit unsigned).

  Supported output modes are CPL_IO_CREATE (create a new file) and
  CPL_IO_EXTEND (append a new extension to an existing file)
 
  The output mode CPL_IO_EXTEND can be combined (via bit-wise or) with an
  option for tile-compression. This compression is lossless.
  The options are:
  CPL_IO_COMPRESS_GZIP, CPL_IO_COMPRESS_RICE, CPL_IO_COMPRESS_HCOMPRESS,
  CPL_IO_COMPRESS_PLIO.
 

  Note that in append mode the file must be writable (and do not take for
  granted that a file is writable just because it was created by the same
  application, as this depends from the system @em umask).
  
  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the mode is not supported
  - CPL_ERROR_FILE_NOT_CREATED if the output file cannot be created
  - CPL_ERROR_FILE_IO if the data cannot be written to the file
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_mask_save(const cpl_mask         * self,
                             const char             * filename,
                             const cpl_propertylist * pl,
                             unsigned                 mode)
{

    /* FIXME: This code is a simplification of cpl_image_save() */

    cpl_error_code error = CPL_ERROR_NONE;

    fitsfile        *  fptr;
    int                fio_status=0;
    const char       * badkeys = mode & CPL_IO_EXTEND
        ? CPL_FITS_BADKEYS_EXT  "|" CPL_FITS_COMPRKEYS
        : CPL_FITS_BADKEYS_PRIM "|" CPL_FITS_COMPRKEYS;
    CPL_FITSIO_TYPE    naxes[2];

    /* Count number of compression flags */
    const unsigned ncompress = ((mode & CPL_IO_COMPRESS_GZIP) != 0) +
        ((mode & CPL_IO_COMPRESS_HCOMPRESS) != 0) +
        ((mode & CPL_IO_COMPRESS_PLIO) != 0) +
        ((mode & CPL_IO_COMPRESS_RICE) != 0);


    cpl_ensure_code(self     != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(filename != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(((mode & CPL_IO_CREATE) != 0) ^ 
                    ((mode & CPL_IO_EXTEND) != 0),
                    CPL_ERROR_ILLEGAL_INPUT);

    cpl_ensure_code(mode < CPL_IO_MAX, CPL_ERROR_ILLEGAL_INPUT);

    naxes[0] = self->nx;
    naxes[1] = self->ny;

    if (mode & CPL_IO_EXTEND) {

        /* Only one type of compression is allowed */
        cpl_ensure_code(ncompress <= 1, CPL_ERROR_ILLEGAL_INPUT);

        /* Open the file */
        cpl_io_fits_open_diskfile(&fptr, filename, READWRITE, &fio_status);
        if (fio_status != 0) {
            return cpl_error_set_fits(CPL_ERROR_FILE_IO, fio_status,
                                      fits_open_diskfile,
                                      "filename='%s', mode=%d",
                                      filename, mode);
        }

        if (mode & CPL_IO_COMPRESS_GZIP) 
            fits_set_compression_type(fptr, GZIP_1, &fio_status);
        else if (mode & CPL_IO_COMPRESS_HCOMPRESS) 
            fits_set_compression_type(fptr, HCOMPRESS_1, &fio_status);
        else if (mode & CPL_IO_COMPRESS_PLIO) 
            fits_set_compression_type(fptr, PLIO_1, &fio_status);
        else if (mode & CPL_IO_COMPRESS_RICE) 
            fits_set_compression_type(fptr, RICE_1, &fio_status);

        if (fio_status != 0) {
            int fio_status2 = 0;
            cpl_io_fits_close_file(fptr, &fio_status2);

            return cpl_error_set_fits(CPL_ERROR_FILE_IO, fio_status,
                                      fits_set_compression_type,
                                      "filename='%s', mode=%d",
                                      filename, mode);
        }
    } else if (ncompress > 0) {
        /* Compression is only allowed in extensions */
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
    } else {
        /* Create the file */
        char * sval = cpl_sprintf("!%s", filename);

        cpl_io_fits_create_file(&fptr, sval, &fio_status);
        cpl_free(sval);
        if (fio_status != 0) {
            int fio_status2 = 0;
            cpl_io_fits_close_file(fptr, &fio_status2);
            return cpl_error_set_fits(CPL_ERROR_FILE_IO, fio_status,
                                      fits_create_file, "filename='%s', "
                                      "mode=%d", filename, mode);
        }
    }

    /* Save the data in a new HDU appended to the file */
    CPL_FITSIO_CREATE_IMG(fptr, BYTE_IMG, 2, naxes, &fio_status);
    
    if (fio_status != 0) {
        int fio_status2 = 0;
        cpl_io_fits_close_file(fptr, &fio_status2);
        return cpl_error_set_fits(CPL_ERROR_FILE_IO, fio_status,
                                  CPL_FITSIO_CREATE_IMG_E, "filename='%s', "
                                  "mode=%d", filename, mode);
    }
    
    if (mode & CPL_IO_CREATE) {
        /* Add DATE */
        fits_write_date(fptr, &fio_status);
        if (fio_status != 0) {
            int fio_status2 = 0;
            cpl_io_fits_close_file(fptr, &fio_status2);
            return cpl_error_set_fits(CPL_ERROR_FILE_IO, fio_status,
                                      fits_write_date, "filename='%s', "
                                      "mode=%d", filename, mode);
        }
    }

    /* Add the property list */
    if (cpl_fits_add_properties(fptr, pl, badkeys) != CPL_ERROR_NONE) {
        cpl_io_fits_close_file(fptr, &fio_status);
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
    }

    /* Write the pixels */
    fits_write_img(fptr, TBYTE, 1, (LONGLONG)self->nx * (LONGLONG)self->ny,
                   (void*)self->data, &fio_status);

    if (fio_status != 0) {
        error = cpl_error_set_fits(CPL_ERROR_FILE_IO, fio_status,
                                   fits_write_img, "filename='%s', "
                                   "mode=%d", filename, mode);
        fio_status = 0;
    }

    /* Close and write on disk */
    cpl_io_fits_close_file(fptr, &fio_status);

    /* Check  */
    if (fio_status != 0) {
        error = cpl_error_set_fits(CPL_ERROR_FILE_IO, fio_status,
                                   fits_close_file, "filename='%s', "
                                   "mode=%d", filename, mode);
    }
    
    return error;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Load a mask from a FITS file.
  @param    filename    Name of the file to load from.
  @param    pnum        Plane number in the Data Unit (0 for first)
  @param    xtnum       Extension number in the file (0 for primary HDU)
  @return   1 newly allocated mask or NULL on error
  @see cpl_image_load()

  This function loads a mask from a FITS file (NAXIS=2 or 3).
  Each non-zero pixel is set to CPL_BINARY_1.

  The returned mask has to be deallocated with cpl_mask_delete().

  'xtnum' specifies from which extension the mask should be loaded.
  This could be 0 for the main data section (files without extension),
  or any number between 1 and N, where N is the number of extensions
  present in the file.

  The requested plane number runs from 0 to nplanes-1, where nplanes is
  the number of planes present in the requested data section.
 
  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_FILE_IO if the file cannot be opened or does not exist
  - CPL_ERROR_BAD_FILE_FORMAT if the data cannot be loaded from the file 
  - CPL_ERROR_ILLEGAL_INPUT if the passed extension number is negative
  - CPL_ERROR_DATA_NOT_FOUND if the specified extension has no mask data
 */
/*----------------------------------------------------------------------------*/
cpl_mask * cpl_mask_load(const char * filename,
                         cpl_size     pnum,
                         cpl_size     xtnum)
{
    cpl_mask * self = cpl_mask_load_one(filename, pnum, xtnum,
                                        CPL_FALSE, 0, 0, 0, 0);

    if (self == NULL) (void)cpl_error_set_where_();

    return self;
}


/*----------------------------------------------------------------------------*/
/**
  @brief  Load a mask from a FITS file.
  @param  filename    Name of the file to load from.
  @param  pnum        Plane number in the Data Unit (0 for first)
  @param  xtnum       Extension number in the file.
  @param  llx         Lower left  x position (FITS convention, 1 for leftmost)
  @param  lly         Lower left  y position (FITS convention, 1 for lowest)
  @param  urx         Upper right x position (FITS convention)
  @param  ury         Upper right y position (FITS convention)
  @return 1 newly allocated mask or NULL on error
  @see    cpl_mask_load()

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_FILE_IO if the file does not exist
  - CPL_ERROR_BAD_FILE_FORMAT if the data cannot be loaded from the file 
  - CPL_ERROR_ILLEGAL_INPUT if the passed position is invalid
 */
/*----------------------------------------------------------------------------*/
cpl_mask * cpl_mask_load_window(const char * filename,
                                cpl_size     pnum,
                                cpl_size     xtnum,
                                cpl_size     llx,
                                cpl_size     lly,
                                cpl_size     urx,
                                cpl_size     ury)
{

    cpl_mask * self = cpl_mask_load_one(filename, pnum, xtnum,
                                        CPL_TRUE, llx, lly, urx, ury);

    if (self == NULL) (void)cpl_error_set_where_();

    return self;

}


/**@}*/


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Fill the specified window with the given value
  @param    self    input mask
  @param    llx     Lower left x position (FITS convention, 1 for leftmost)
  @param    lly     Lower left y position (FITS convention, 1 for lowest)
  @param    urx     Upper right x position (FITS convention)
  @param    ury     Upper right y position (FITS convention)
  @param    value   Value to fill with
  @return   Nothing
  @note No input validation in this internal function

 */
/*----------------------------------------------------------------------------*/
void cpl_mask_fill_window(cpl_mask * self,
                          cpl_size     llx,
                          cpl_size     lly,
                          cpl_size     urx,
                          cpl_size     ury,
                          cpl_binary value)
{
    if (llx == 1 && urx == self->nx) {
        (void)memset(self->data + (size_t)self->nx * (size_t)(lly - 1), value,
                     (size_t)(self->nx * (1 + ury - lly)) * sizeof(cpl_binary));

    } else {
        for (cpl_size j = lly; j <= ury; j++) {
            (void)memset(self->data + (size_t)(self->nx * (j - 1) + llx - 1),
                         value, (size_t)(1 + urx -llx) * sizeof(cpl_binary));
        }
    }
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Extract a mask from an other one, return NULL instead of empty
  @param    in      input mask
  @param    llx     Lower left x position (FITS convention, 1 for leftmost)
  @param    lly     Lower left y position (FITS convention, 1 for lowest)
  @param    urx     Upper right x position (FITS convention)
  @param    ury     Upper right y position (FITS convention)
  @return   1 newly allocated mask or NULL
  @see cpl_mask_extract()
  @note The returned mask must be deallocated using cpl_mask_delete().

 */
/*----------------------------------------------------------------------------*/
cpl_mask * cpl_mask_extract_(const cpl_mask * in,
                             cpl_size         llx,
                             cpl_size         lly,
                             cpl_size         urx,
                             cpl_size         ury)
{
    const cpl_size first = cpl_mask_get_first_window(in, llx, lly, urx, ury,
                                                     CPL_BINARY_1);

    if (first >= 0) {
        /* first is the number of good (zero) elements before the first bad */
        const cpl_size nx = urx - llx + 1;
        const cpl_size ny = ury - lly + 1;
        /* Create the buffer of the extracted mask */
        cpl_binary *pout = (cpl_binary*)cpl_malloc((size_t)(nx * ny)
                                                   * sizeof(*pout));

        if (first > 0) {
            (void)memset(pout, 0, first * sizeof(cpl_binary));
        }

        if (nx == in->nx) {
            /* Consecutive, remaining buffer is extracted from in */
            (void)memcpy(pout + first,
                         in->data + first + (size_t)in->nx * (size_t)(lly - 1),
                         (size_t)(nx * ny - first) * sizeof(cpl_binary));
        } else {
            const cpl_size my   = first / nx; /* Whole zero-rows copied */
            const cpl_size part = first - nx * my; /* Partially copied row */
            const cpl_binary * pin = in->data + (llx - 1)
                + (size_t)in->nx * (size_t)(lly - 1 + my);
            cpl_binary       * pj  = pout + nx * my;
            cpl_size           j = my;

            if (part > 0) {
                /* Complete partial row */
                (void)memcpy(pj + part, pin + part,
                             (size_t)(nx - part) * sizeof(cpl_binary));
                j++;
                pin += in->nx;
                pj  += nx;
            }

            /* Loop over the rows to extract */
            for (; j < ny; j++, pin += in->nx, pj += nx) {
                (void)memcpy(pj, pin, (size_t)nx * sizeof(cpl_binary));
            }
        }

        return cpl_mask_wrap(nx, ny, pout);
    } else if (first < -1) {
        (void)cpl_error_set_where_();
    }

    return NULL;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Load an mask from a FITS file.
  @param  filename    Name of the file to load from.
  @param  pnum        Plane number in the Data Unit (0 for first)
  @param  xtnum       Extension number in the file (0 for primary HDU)
  @param  do_window   True for (and only for) a windowed load
  @param  llx         Lower left  x position (FITS convention, 1 for leftmost)
  @param  lly         Lower left  y position (FITS convention, 1 for lowest)
  @param  urx         Upper right x position (FITS convention)
  @param  ury         Upper right y position (FITS convention)
  @return 1 newly allocated mask or NULL on error
  @see cpl_mask_load()
*/
/*----------------------------------------------------------------------------*/
static cpl_mask * cpl_mask_load_one(const char * filename,
                                    cpl_size     pnum,
                                    cpl_size     xtnum,
                                    cpl_boolean  do_window,
                                    cpl_size     llx,
                                    cpl_size     lly,
                                    cpl_size     urx,
                                    cpl_size     ury)
{

    /* FIXME: This code is a simplification of cpl_image_load_one() */

    int             error = 0;
    int             naxis = 0;
    /* May read from Data Unit w. NAXIS=[23] */
    CPL_FITSIO_TYPE naxes[3] ={0, 0, 0};
    fitsfile * fptr;
    cpl_mask * self;

    /* FIXME: Version 3.24 of fits_open_diskfile() seg-faults on NULL.
       If ever fixed in CFITSIO, this check should be removed */
    cpl_ensure(filename != NULL, CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(xtnum   >= 0,     CPL_ERROR_ILLEGAL_INPUT, NULL);

    if (cpl_io_fits_open_diskfile(&fptr, filename, READONLY, &error)) {
        /* If the file did not exist:    error = 104 (FILE_NOT_OPENED) */
        /* If the file had permission 0: error = 104 (FILE_NOT_OPENED) */
        /* If the file was empty:        error = 107 (END_OF_FILE) */
        /* If the file was a directory:  error = 108 (READ_ERROR) */
        (void)cpl_error_set_fits(CPL_ERROR_FILE_IO, error, fits_open_diskfile,
                                 "filename='%s', pnum=%" CPL_SIZE_FORMAT
                                 ", xtnum=%" CPL_SIZE_FORMAT,
                                 filename, pnum, xtnum);
        return NULL;
    }

    self = cpl_mask_load_(fptr, &naxis, naxes, filename, pnum,
                          xtnum, do_window, llx, lly, urx, ury);

    if (cpl_io_fits_close_file(fptr, &error)) {
        cpl_mask_delete(self);
        self = NULL;
        (void)cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT, error,
                                 fits_close_file, "filename='%s', pnum=%"
                                 CPL_SIZE_FORMAT ", xtnum=%" CPL_SIZE_FORMAT,
                                 filename, pnum, xtnum);
    } else if (self == NULL) {
        cpl_error_set_where_();
    }

    return self;
}

/*----------------------------------------------------------------------------*/
/*
  @internal
  @brief Internal mask loading function
  @param fptr      CFITSIO structure of the already opened FITS file
  @param pnaxis    If it points to 0, set *pnaxis to NAXIS else use as such
  @param naxes     If 1st is 0, fill w. NAXIS[12[3]] else use as such
  @param filename  The named of the opened file (only for error messages)
  @param pnum      Plane number in the Data Unit (0 for first)
  @param lhdumov   Absolute extension number to move to first (0 for primary)
  @param do_window True for (and only for) a windowed load
  @param llx       Lower left x position (FITS convention, 1 for leftmost)
  @param lly       Lower left y position (FITS convention, 1 for lowest)
  @param urx       Specifies the window position
  @param ury       Specifies the window position
  @return   1 newly allocated mask or NULL if mask cannot be loaded.
  @see cpl_mask_load_one()

  This function reads from an already opened FITS-file, which is useful when
  multiple masks are to be loaded from the same file.

  To avoid repeated calls to determine NAXIS, NAXISi and optionally the
  CPL pixel-type most suitable for the the actual FITS data, the parameters
  pnaxis and naxes can be used both for input and for output.

  The extension number lhdumov is numbered with 1 for the first extension,
  i.e. 0 moves to the primary HDU. Any negative value causes no move.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_BAD_FILE_FORMAT if a CFITSIO call fails.
  - CPL_ERROR_DATA_NOT_FOUND if the specified extension has no mask data.
                             This code is relied on as being part of the API!
  - CPL_ERROR_INCOMPATIBLE_INPUT if NAXIS is OK but the data unit is empty
  - CPL_ERROR_ILLEGAL_INPUT If the plane number is out of range, or in a
                            windowed read the window is invalid

 */
/*----------------------------------------------------------------------------*/
cpl_mask * cpl_mask_load_(fitsfile       * fptr,
                          int            * pnaxis,
                          CPL_FITSIO_TYPE naxes[],
                          const char     * filename,
                          cpl_size         pnum,
                          cpl_size         lhdumov,
                          cpl_boolean      do_window,
                          cpl_size         llx,
                          cpl_size         lly,
                          cpl_size         urx,
                          cpl_size         ury)
{

    /* FIXME: This code is a simplification of cpl_image_load_() */

    const int      hdumov = (int)lhdumov;
    int            error = 0;
    cpl_mask     * self;
    cpl_binary   * pixels;
    const long int inc[3] = {1, 1, 1};
    long int       fpixel[3];
    long int       lpixel[3];
    cpl_size       nx, ny;
    size_t         i;

    cpl_ensure(fptr      != NULL, CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(pnaxis    != NULL, CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(naxes     != NULL, CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(filename  != NULL, CPL_ERROR_NULL_INPUT, NULL);
    /* CFITSIO only supports int */
    cpl_ensure((cpl_size)(1+hdumov) == 1+lhdumov, CPL_ERROR_ILLEGAL_INPUT,
               NULL);

    if (hdumov >= 0 && fits_movabs_hdu(fptr, 1+hdumov, NULL, &error)) {
        (void)cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT, error,
                                 fits_movabs_hdu, "filename='%s', pnum=%"
                                 CPL_SIZE_FORMAT ", " "hdumov=%d",
                                 filename, pnum, hdumov);
        return NULL;
    }

    /* Get NAXIS, if needed */
    if (*pnaxis == 0 && fits_get_img_dim(fptr, pnaxis, &error)) {
        (void)cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT, error,
                                 fits_get_img_dim, "filename='%s', pnum=%"
                                 CPL_SIZE_FORMAT ", hdumov=%d",
                                 filename, pnum, hdumov);
        return NULL;
    }
    /* Verify NAXIS before trying anything else */
    if (*pnaxis != 2 && *pnaxis != 3) {
        (void)cpl_error_set_message_(CPL_ERROR_DATA_NOT_FOUND,
                                    "filename='%s', pnum=%" CPL_SIZE_FORMAT
                                    ", hdumov=%d, NAXIS=%d",
                                     filename, pnum, hdumov, *pnaxis);
        return NULL;
    }

    /* Get NAXIS[12[3]], if needed */
    if (naxes[0] == 0 && CPL_FITSIO_GET_SIZE(fptr, *pnaxis, naxes, &error)) {
        (void)cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT, error,
                                 CPL_FITSIO_GET_SIZE, "filename='%s', "
                                 "pnum=%" CPL_SIZE_FORMAT ", hdumov=%d, "
                                 "NAXIS=%d", filename, pnum, hdumov, *pnaxis);
        return NULL;
    }

    /* Verify NAXIS[123] */
    if (naxes[0] == 0 || naxes[1] == 0) {
        /* FIXME: Is this actually possible with a non-zero NAXIS ? */
        (void)cpl_error_set_message_(CPL_ERROR_INCOMPATIBLE_INPUT,
                                    "filename='%s', pnum=%" CPL_SIZE_FORMAT
                                     ", hdumov=%d, NAXIS=%d, NAXIS1=%ld, "
                                     "NAXIS2=%ld", filename, pnum, hdumov,
                                    *pnaxis, (long)naxes[0], (long)naxes[1]);
        return NULL;
    }
    if (*pnaxis == 3 && naxes[2] == 0) {
        /* FIXME: Is this actually possible with a non-zero NAXIS ? */
        (void)cpl_error_set_message_(CPL_ERROR_INCOMPATIBLE_INPUT,
                                    "filename='%s', pnum=%" CPL_SIZE_FORMAT
                                     ", hdumov=%d, NAXIS=%d, NAXIS1=%ld, "
                                     "NAXIS2=%ld NAXIS3=0",
                                     filename, pnum, hdumov, *pnaxis,
                                     (long)naxes[0], (long)naxes[1]);
        return NULL;
    }

    if (do_window) {
        /* Verify the window size */
        /* If the naxes[] passed is from a previous succesful call here,
           then this check is redundant. Don't rely on that. */
        if (llx < 1 || lly < 1 || urx > naxes[0] || ury > naxes[1]
            || urx < llx || ury < lly) {
            (void)cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT,
                                         "filename='%s', pnum=%"
                                         CPL_SIZE_FORMAT ", hdumov=%d, "
                                         "llx=%" CPL_SIZE_FORMAT ", "
                                         "lly=%" CPL_SIZE_FORMAT ", "
                                         "urx=%" CPL_SIZE_FORMAT ", "
                                         "ury=%" CPL_SIZE_FORMAT ", "
                                         "NAXIS=%d, NAXIS1=%ld, NAXIS2=%ld",
                                         filename, pnum, hdumov, llx, lly,
                                         urx, ury, *pnaxis, (long)naxes[0],
                                         (long)naxes[1]);
            return NULL;
        }
    } else {
        llx = lly = 1;
        urx = naxes[0];
        ury = naxes[1];
    }

    /* Create the zone definition. The 3rd element not defined for NAXIS = 2 */
    fpixel[0] = llx;
    fpixel[1] = lly;
    lpixel[0] = urx;
    lpixel[1] = ury;
    if (*pnaxis == 3) {

        /* Verify plane number */
        if (pnum + 1 > naxes[2]) {
            (void)cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT,
                                         "filename='%s', pnum=%" CPL_SIZE_FORMAT
                                         ", hdumov=%d, NAXIS=3, NAXIS1=%ld, "
                                         "NAXIS2=%ld, NAXIS3=%ld", filename,
                                         pnum, hdumov, (long)naxes[0],
                                         (long)naxes[1], (long)naxes[2]);
            return NULL;
        }

        fpixel[2] = lpixel[2] = pnum + 1;
    } else if (pnum != 0) {
        /* May not ask for any plane but the first when NAXIS == 2 */
        (void)cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT,
                                    "filename='%s', pnum=%" CPL_SIZE_FORMAT
                                     ", hdumov=%d, NAXIS=%d, NAXIS1=%ld, "
                                     "NAXIS2=%ld", filename, pnum, hdumov,
                                     *pnaxis, (long)naxes[0], (long)naxes[1]);
        return NULL;
    }

    nx = urx - llx + 1;
    ny = ury - lly + 1;

    pixels = (cpl_binary*)cpl_malloc((size_t)nx * (size_t)ny * sizeof(*pixels));

    if (cpl_fits_read_subset(fptr, TBYTE, fpixel, lpixel, inc,
                             NULL, pixels, NULL, &error)) {
        cpl_free(pixels);
        (void)cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT, error,
                                 fits_read_subset, "filename='%s', "
                                 "pnum=%" CPL_SIZE_FORMAT ", hdumov=%d, "
                                 "NAXIS=%d, NAXIS1=%ld, NAXIS2=%ld",
                                 filename, pnum, hdumov, *pnaxis,
                                 (long)naxes[0], (long)naxes[1]);
        return NULL;
    }

    /* Cast to cpl_binary */
    for (i = 0; i < (size_t)(nx * ny); i++) {
        pixels[i] = pixels[i] ? CPL_BINARY_1 : CPL_BINARY_0;
    }

    self = cpl_mask_wrap(nx, ny, pixels);

    if (self == NULL) {
        cpl_free(pixels);
        (void)cpl_error_set_where_();
    }

    return self;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Find 1st element with the given value (zero for first)
  @param    self  The mask to search
  @param    llx   Lower left x position (FITS convention, 1 for leftmost)
  @param    lly   Lower left y position (FITS convention, 1 for lowest)
  @param    urx   Upper right x position (FITS convention)
  @param    ury   Upper right y position (FITS convention)
  @return   The index (zero for first), or -1 when not found, or < -1 on error
  @note When found, the index is in the frame of the window

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the window coordinates are not valid
 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_mask_get_first_window(const cpl_mask * self,
                                   cpl_size         llx,
                                   cpl_size         lly,
                                   cpl_size         urx,
                                   cpl_size         ury,
                                   cpl_binary       value)
{

    const cpl_binary  * pi;
    const cpl_binary  * found;

    cpl_ensure(self != NULL,    CPL_ERROR_NULL_INPUT, -2);

    cpl_ensure(llx <= urx,      CPL_ERROR_ILLEGAL_INPUT, -3);
    cpl_ensure(llx > 0,         CPL_ERROR_ILLEGAL_INPUT, -4);
    cpl_ensure(urx <= self->nx, CPL_ERROR_ILLEGAL_INPUT, -5);

    cpl_ensure(lly <= ury,      CPL_ERROR_ILLEGAL_INPUT, -6);
    cpl_ensure(lly > 0,         CPL_ERROR_ILLEGAL_INPUT, -7);
    cpl_ensure(ury <= self->ny, CPL_ERROR_ILLEGAL_INPUT, -8);

    /* Point to 1st element in first row to read */
    pi = self->data + (lly-1)*self->nx + llx - 1;

    if (llx == 1 && urx == self->nx) {
        /* Special case: Window extends to left and right mask border */
        found = memchr(pi, value,
                       (size_t)self->nx * (size_t)(ury-lly+1) * sizeof(*pi));
    } else {
        const cpl_binary * pii = pi;
        const cpl_size     nx  = urx - llx + 1;

        for (cpl_size j = lly - 1; j < ury; j++, pii += self->nx,
                 pi += self->nx - nx) {
            found = memchr(pii, value, nx * sizeof(*pii));
            if (found) break;
        }
    }

    return found == NULL ? -1 : found - pi;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Apply the erosion filter
  @param  self    The output binary 2D array to hold the filtered result
  @param  other   The input  binary 2D array to filter
  @param  kernel  The input  binary 2D kernel - rows padded with 0 to fit word
  @param  nx      The X-size of the input array
  @param  ny      The Y-size of the input array
  @param  hx      The X-half-size of the kernel
  @param  hy      The Y-half-size of the kernel
  @param  border  CPL_BORDER_NOP, CPL_BORDER_ZERO or CPL_BORDER_COPY
  @return void
  @note No error checking in this internal function!

  FIXME: Changes to this function must also be made to cpl_mask_dilation_()

 */
/*----------------------------------------------------------------------------*/
static
void cpl_mask_erosion_(cpl_binary * self, const cpl_binary * other,
                       const cpl_binary * kernel, cpl_size nx, cpl_size ny,
                       cpl_size hx, cpl_size hy, cpl_border_mode border)
{

#ifdef CPL_MASK_NOT
    if (hx <= CPL_MASK_HXW && hy == 0) {
        cpl_mask_erosion_1_0(self, other, kernel, nx, ny, hx, hy, border);
    } else if (hx <= CPL_MASK_HXW && hy == 1) {
        cpl_mask_erosion_1_1(self, other, kernel, nx, ny, hx, hy, border);
    } else if (hx <= CPL_MASK_HXW && hy == 2) {
        cpl_mask_erosion_1_2(self, other, kernel, nx, ny, hx, hy, border);
    } else if (hx <= CPL_MASK_HXW && hy == 3) {
        cpl_mask_erosion_1_3(self, other, kernel, nx, ny, hx, hy, border);
    } else if (hx <= CPL_MASK_HX2W && hy == 0) {
        cpl_mask_erosion_2_0(self, other, kernel, nx, ny, hx, hy, border);
    } else if (hx <= CPL_MASK_HX2W && hy == 1) {
        cpl_mask_erosion_2_1(self, other, kernel, nx, ny, hx, hy, border);
    } else if (hx <= CPL_MASK_HX2W && hy == 2) {
        cpl_mask_erosion_2_2(self, other, kernel, nx, ny, hx, hy, border);
    } else if (hx <= CPL_MASK_HX2W && hy == 3) {
        cpl_mask_erosion_2_3(self, other, kernel, nx, ny, hx, hy, border);
    } else {
        cpl_mask_erosion_n_n(self, other, kernel, nx, ny, hx, hy, border);
    }
#endif
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Apply the dilation filter
  @param  self    The output binary 2D array to hold the filtered result
  @param  other   The input  binary 2D array to filter
  @param  kernel  The input  binary 2D kernel - rows padded with 0 to fit word
  @param  nx      The X-size of the input array
  @param  ny      The Y-size of the input array
  @param  hx      The X-half-size of the kernel
  @param  hy      The Y-half-size of the kernel
  @param  border  CPL_BORDER_NOP, CPL_BORDER_ZERO or CPL_BORDER_COPY
  @return void
  @note No error checking in this internal function!
  @see cpl_mask_erosion_()
 
  Modified from cpl_mask_erosion_().
 
  FIXME: Changes to this function must also be made to cpl_mask_erosion_()

 */
/*----------------------------------------------------------------------------*/
static
void cpl_mask_dilation_(cpl_binary * self, const cpl_binary * other,
                       const cpl_binary * kernel, cpl_size nx, cpl_size ny,
                        cpl_size hx, cpl_size hy, cpl_border_mode border)
{
#ifdef CPL_MASK_NOT
    if (hx <= CPL_MASK_HXW && hy == 0) {
        cpl_mask_dilation_1_0(self, other, kernel, nx, ny, hx, hy, border);
    } else if (hx <= CPL_MASK_HXW && hy == 1) {
        cpl_mask_dilation_1_1(self, other, kernel, nx, ny, hx, hy, border);
    } else if (hx <= CPL_MASK_HXW && hy == 2) {
        cpl_mask_dilation_1_2(self, other, kernel, nx, ny, hx, hy, border);
    } else if (hx <= CPL_MASK_HXW && hy == 3) {
        cpl_mask_dilation_1_3(self, other, kernel, nx, ny, hx, hy, border);
    } else if (hx <= CPL_MASK_HX2W && hy == 0) {
        cpl_mask_dilation_2_0(self, other, kernel, nx, ny, hx, hy, border);
    } else if (hx <= CPL_MASK_HX2W && hy == 1) {
        cpl_mask_dilation_2_1(self, other, kernel, nx, ny, hx, hy, border);
    } else if (hx <= CPL_MASK_HX2W && hy == 2) {
        cpl_mask_dilation_2_2(self, other, kernel, nx, ny, hx, hy, border);
    } else if (hx <= CPL_MASK_HX2W && hy == 3) {
        cpl_mask_dilation_2_3(self, other, kernel, nx, ny, hx, hy, border);
    } else {
        cpl_mask_dilation_n_n(self, other, kernel, nx, ny, hx, hy, border);
    }
#endif
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Apply the opening filter
  @param  self    The output binary 2D array to hold the filtered result
  @param  other   The input  binary 2D array to filter
  @param  kernel  The input  binary 2D kernel - rows padded with 0 to fit word
  @param  nx      The X-size of the input array
  @param  ny      The Y-size of the input array
  @param  hx      The X-half-size of the kernel
  @param  hy      The Y-half-size of the kernel
  @param  border  CPL_BORDER_NOP, CPL_BORDER_ZERO or CPL_BORDER_COPY
  @return void
  @note No error checking in this internal function!
  @note  Fast handling of 3 x 3 (and 1 x 3) kernels
  @see cpl_mask_erosion_(), cpl_mask_dilation()
 
 */
/*----------------------------------------------------------------------------*/
static
void cpl_mask_opening_(cpl_binary * self, const cpl_binary * other,
                       const cpl_binary * kernel, cpl_size nx, cpl_size ny,
                       cpl_size hx, cpl_size hy, cpl_border_mode border)
{
    /* Border of temporary buffer needs initialization from first filtering */
    const cpl_border_mode border1 = border == CPL_BORDER_NOP
        ? CPL_BORDER_COPY : border;
    cpl_binary * middle = cpl_malloc((size_t)nx * (size_t)ny
                                     * sizeof(cpl_binary));

    cpl_mask_erosion_(middle, other, kernel, nx, ny, hx, hy, border1);
    cpl_mask_dilation_(self, middle, kernel, nx, ny, hx, hy, border);
    cpl_free(middle);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Apply the closing filter
  @param  self    The output binary 2D array to hold the filtered result
  @param  other   The input  binary 2D array to filter
  @param  kernel  The input  binary 2D kernel - rows padded with 0 to fit word
  @param  nx      The X-size of the input array
  @param  ny      The Y-size of the input array
  @param  hx      The X-half-size of the kernel
  @param  hy      The Y-half-size of the kernel
  @param  border  CPL_BORDER_NOP, CPL_BORDER_ZERO or CPL_BORDER_COPY
  @return void
  @note No error checking in this internal function!
  @note  Fast handling of 3 x 3 (and 1 x 3) kernels
  @see cpl_mask_dilation_(), cpl_mask_erosion()
 
 */
/*----------------------------------------------------------------------------*/
static
void cpl_mask_closing_(cpl_binary * self, const cpl_binary * other,
                       const cpl_binary * kernel, cpl_size nx, cpl_size ny,
                       cpl_size hx, cpl_size hy, cpl_border_mode border)
{
    /* Border of temporary buffer needs initialization from first filtering */
    const cpl_border_mode border1 = border == CPL_BORDER_NOP
        ? CPL_BORDER_COPY : border;
    cpl_binary * middle = cpl_malloc((size_t)nx * (size_t)ny
                                     * sizeof(cpl_binary));

    cpl_mask_dilation_(middle, other, kernel, nx, ny, hx, hy, border1);
    cpl_mask_erosion_(self, middle, kernel, nx, ny, hx, hy, border);
    cpl_free(middle);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Fill a mask from a matrix
  @param    kernel The matrix
  @param    tol    The non-zero tolerance
  @return   the mask on success, or NULL on error
  @note This function is different from the similar one in cpl_image_filter.c.

  Example:

    matrix       ->    mask

    1.0 0.0 1.0       1 0 1
    0.0 0.0 0.0       0 0 0
    0.0 0.0 0.0       0 0 0

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
static
cpl_mask * cpl_mask_new_from_matrix(const cpl_matrix * kernel, double tol)
{

    const cpl_size mx   = cpl_matrix_get_ncol(kernel);
    const cpl_size my   = cpl_matrix_get_nrow(kernel);
    cpl_mask     * self;


    cpl_ensure(kernel != NULL, CPL_ERROR_NULL_INPUT, NULL);

    self = cpl_mask_new(mx, my);

    for (cpl_size j = 0; j < my; j++) {
        for (cpl_size i = 0; i < mx; i++) {
            const double value = cpl_matrix_get(kernel, my - j - 1, i);

            if (fabs(value) > tol)
                cpl_mask_set(self, i + 1, j + 1, CPL_BINARY_1);
        }
    }

    return self;
}
