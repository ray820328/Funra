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

#include <complex.h>

#include "cpl_image_basic_impl.h"

#include "cpl_image_io_impl.h"

#include "cpl_memory.h"
#include "cpl_vector.h"
#include "cpl_image_stats.h"
#include "cpl_stats_impl.h"
#include "cpl_image_bpm.h"
#include "cpl_image_iqe.h"
#include "cpl_mask_impl.h"
#include "cpl_tools.h"
#include "cpl_errorstate.h"
#include "cpl_error_impl.h"
#include "cpl_math_const.h"
#include "cpl_image_defs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <complex.h>
#include <assert.h>
#include <errno.h>
/* for SIZE_MAX, intptr_t */
#include <stdint.h>


/*-----------------------------------------------------------------------------
                                   Defines
 -----------------------------------------------------------------------------*/

#define CPL_IMAGE_BASIC_ASSIGN              1
#define CPL_IMAGE_BASIC_ASSIGN_LOCAL        2
#define CPL_IMAGE_BASIC_THRESHOLD           3
#define CPL_IMAGE_BASIC_ABS                 4
#define CPL_IMAGE_BASIC_AVERAGE             5
#define CPL_IMAGE_BASIC_SUBMIN              6
#define CPL_IMAGE_BASIC_EXTRACT             7
#define CPL_IMAGE_BASIC_EXTRACTROW          8
#define CPL_IMAGE_BASIC_EXTRACTCOL          9
#define CPL_IMAGE_BASIC_COLLAPSE_MEDIAN     11
#define CPL_IMAGE_BASIC_ROTATE_INT_LOCAL    12
#define CPL_IMAGE_BASIC_FLIP_LOCAL          15
#define CPL_IMAGE_BASIC_MOVE_PIXELS         16
#define CPL_IMAGE_BASIC_OP_SCALAR           21
#define CPL_IMAGE_BASIC_SQRT                22
#define CPL_IMAGE_BASIC_DECLARE             23

#define CPL_IMAGE_BASIC_OPERATE             24
#define CPL_IMAGE_BASIC_DIVIDE              25
#define CPL_IMAGE_BASIC_OPERATE_LOCAL       26
#define CPL_IMAGE_BASIC_DIVIDE_LOCAL        27
#define CPL_IMAGE_BASIC_HYPOT               28

#define CPL_IMAGE_ADDITION(a,b,c)           a = (b) + (c)
#define CPL_IMAGE_ADDITIONASSIGN(a,b)       a += (b)
#define CPL_IMAGE_SUBTRACTION(a,b,c)        a = (b) - (c)
#define CPL_IMAGE_SUBTRACTIONASSIGN(a,b)    a -= (b)
#define CPL_IMAGE_MULTIPLICATION(a,b,c)     a = (b) * (c)
#define CPL_IMAGE_MULTIPLICATIONASSIGN(a,b) a *= (b)
#define CPL_IMAGE_DIVISION(a,b,c)           a = (b) / (c)
#define CPL_IMAGE_DIVISIONASSIGN(a,b)       a /= (b)
#define CPL_IMAGE_MINABS(a,b,c)             a = CPL_MATH_ABS1(b) < CPL_MATH_ABS2(c) ? (b) : (c)
  
/*-----------------------------------------------------------------------------
                            Private Function prototypes
 -----------------------------------------------------------------------------*/

static double cpl_vector_get_noise(const cpl_vector *, cpl_size);
static double cpl_vector_get_fwhm(const cpl_vector *, cpl_size, double);
static cpl_error_code cpl_fft(double *, double *, const unsigned *, int, int);

/* Declare and define the 8 hypot functions */
#define CPL_OPERATION CPL_IMAGE_BASIC_HYPOT

#define CPL_TYPE_T1 CPL_TYPE_FLOAT
#define CPL_TYPE1  float

#define CPL_TYPE_T2 CPL_TYPE_FLOAT
#define CPL_TYPE_T3 CPL_TYPE_FLOAT
#define CPL_TYPE2  float
#define CPL_TYPE3  float
#define CPL_HYPOT hypotf

#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T2
#undef CPL_TYPE_T3
#undef CPL_TYPE2
#undef CPL_TYPE3
#undef CPL_HYPOT


#define CPL_TYPE_T2 CPL_TYPE_FLOAT
#define CPL_TYPE_T3 CPL_TYPE_DOUBLE
#define CPL_TYPE2  float
#define CPL_TYPE3  double
#define CPL_HYPOT hypot
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T2
#undef CPL_TYPE_T3
#undef CPL_TYPE2
#undef CPL_TYPE3

#define CPL_TYPE_T2 CPL_TYPE_DOUBLE
#define CPL_TYPE_T3 CPL_TYPE_FLOAT
#define CPL_TYPE2  double
#define CPL_TYPE3  float
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T2
#undef CPL_TYPE_T3
#undef CPL_TYPE2
#undef CPL_TYPE3

#define CPL_TYPE_T2 CPL_TYPE_DOUBLE
#define CPL_TYPE_T3 CPL_TYPE_DOUBLE
#define CPL_TYPE2  double
#define CPL_TYPE3  double
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T2
#undef CPL_TYPE_T3
#undef CPL_TYPE2
#undef CPL_TYPE3

#undef CPL_TYPE_T1
#undef CPL_TYPE1
#define CPL_TYPE_T1 CPL_TYPE_DOUBLE
#define CPL_TYPE1  double
#undef CPL_HYPOT

#define CPL_TYPE_T2 CPL_TYPE_FLOAT
#define CPL_TYPE_T3 CPL_TYPE_FLOAT
#define CPL_TYPE2  float
#define CPL_TYPE3  float
#define CPL_HYPOT hypotf

#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T2
#undef CPL_TYPE_T3
#undef CPL_TYPE2
#undef CPL_TYPE3
#undef CPL_HYPOT

#define CPL_TYPE_T2 CPL_TYPE_FLOAT
#define CPL_TYPE_T3 CPL_TYPE_DOUBLE
#define CPL_TYPE2  float
#define CPL_TYPE3  double
#define CPL_HYPOT hypot
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T2
#undef CPL_TYPE_T3
#undef CPL_TYPE2
#undef CPL_TYPE3

#define CPL_TYPE_T2 CPL_TYPE_DOUBLE
#define CPL_TYPE_T3 CPL_TYPE_FLOAT
#define CPL_TYPE2  double
#define CPL_TYPE3  float
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T2
#undef CPL_TYPE_T3
#undef CPL_TYPE2
#undef CPL_TYPE3

#define CPL_TYPE_T2 CPL_TYPE_DOUBLE
#define CPL_TYPE_T3 CPL_TYPE_DOUBLE
#define CPL_TYPE2  double
#define CPL_TYPE3  double
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T2
#undef CPL_TYPE_T3
#undef CPL_TYPE2
#undef CPL_TYPE3

#undef CPL_TYPE_T1
#undef CPL_TYPE1
#undef CPL_HYPOT
#undef CPL_OPERATION


/* Declare and define the C-type dependent functions */
#define CPL_OPERATION CPL_IMAGE_BASIC_DECLARE

#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_image_basic_body.h"
#undef CPL_CLASS

#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_image_basic_body.h"
#undef CPL_CLASS

#define CPL_CLASS CPL_CLASS_INT
#include "cpl_image_basic_body.h"
#undef CPL_CLASS

#undef CPL_OPERATION

#ifdef __SSE3__
#include <pmmintrin.h>
#endif
#ifdef __SSE2__
#include <xmmintrin.h>

#ifdef __clang__
#    define cpl_m_from_int64 (__m64)
#  else
#    define cpl_m_from_int64 _m_from_int64
#  endif
#endif

#if defined __SSE2__ || defined __SSE3__

#if defined __SSE3__
#define CPL_MM_ADDSUB_PS(a, b) _mm_addsub_ps(a, b)
#define CPL_MM_ADDSUB_PD(a, b) _mm_addsub_pd(a, b)
#else
        /* faster than multiplying with 1,-1,1,-1 */
#define CPL_MM_ADDSUB_PS(a, b) \
    _mm_add_ps(a, _mm_xor_ps(b, (__m128)_mm_set_epi32(0x0u, 0x80000000u, \
                                                      0x0u, 0x80000000u)))
#define CPL_MM_ADDSUB_PD(a, b) \
  _mm_add_pd(a, _mm_xor_pd(b, (__m128d)_mm_set_epi64(cpl_m_from_int64(0x0llu), \
                                                     cpl_m_from_int64(0x8000000000000000llu))))
#endif

static cpl_error_code cpl_image_multiply_fcomplex_sse_(cpl_image       *,
                                                       const cpl_image *)
    CPL_ATTR_NONNULL;
static cpl_error_code cpl_image_multiply_dcomplex_sse_(cpl_image       *,
                                                       const cpl_image *)
    CPL_ATTR_NONNULL;
#endif

/*-----------------------------------------------------------------------------
                            Function codes
 -----------------------------------------------------------------------------*/

#define CPL_OPERATION CPL_IMAGE_BASIC_ASSIGN
/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Add two images.
  @param    image1  first operand
  @param    image2  second operand
  @return   1 newly allocated image or NULL on error

  Creates a new image, being the result of the operation, and returns it to
  the caller. The returned image must be deallocated using cpl_image_delete().
  The function supports images with different types among CPL_TYPE_INT, 
  CPL_TYPE_FLOAT and CPL_TYPE_DOUBLE. The returned image type is the one of the 
  first passed image.

  The bad pixels map of the result is the union of the bad pixels maps of
  the input images.
  
  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INCOMPATIBLE_INPUT if the input images have different sizes
  - CPL_ERROR_TYPE_MISMATCH if the second input image has complex type
    while the first one does not
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_add_create(
        const cpl_image *   image1, 
        const cpl_image *   image2) 
{
#define CPL_OPERATOR CPL_IMAGE_ADDITION
#include "cpl_image_basic_body.h"
#undef CPL_OPERATOR
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Subtract two images.
  @param    image1  first operand
  @param    image2  second operand
  @return   1 newly allocated image or NULL on error
  @see      cpl_image_add_create()
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_subtract_create(
        const cpl_image *   image1, 
        const cpl_image *   image2) 
{
#define CPL_OPERATOR CPL_IMAGE_SUBTRACTION
#include "cpl_image_basic_body.h"
#undef CPL_OPERATOR
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Multiply two images.
  @param    image1  first operand
  @param    image2  second operand
  @return   1 newly allocated image or NULL on error
  @see      cpl_image_add_create()
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_multiply_create(
        const cpl_image *   image1, 
        const cpl_image *   image2)
{
#define CPL_OPERATOR CPL_IMAGE_MULTIPLICATION
#include "cpl_image_basic_body.h"
#undef CPL_OPERATOR
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief    Create the minimum of two images.
  @param    image1  first operand
  @param    image2  second operand
  @return   1 newly allocated image or NULL on error
  @see      cpl_image_add_create()
  @note For each pixel position the new value is the one with the
        absolute minimum
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_min_create(
        const cpl_image *   image1, 
        const cpl_image *   image2)
{
#define CPL_OPERATOR CPL_IMAGE_MINABS
#include "cpl_image_basic_body.h"
#undef CPL_OPERATOR
}
#undef CPL_OPERATION
 
/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Divide two images.
  @param    image1  first operand
  @param    image2  second operand
  @return   1 newly allocated image or NULL on error
  @see      cpl_image_add_create()
  @see      cpl_image_divide()
     The result of division with a zero-valued pixel is marked as a bad
     pixel.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INCOMPATIBLE_INPUT if the input images have different sizes
  - CPL_ERROR_TYPE_MISMATCH if the second input image has complex type
    while the first one does not
  - CPL_ERROR_DIVISION_BY_ZERO is all pixels in the divisor are zero
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_divide_create(const cpl_image * image1, 
                                    const cpl_image * image2)
{

#define CPL_OPERATION CPL_IMAGE_BASIC_DIVIDE

    cpl_image               *   self;
    cpl_mask                *   zeros;
    cpl_binary              *   pzeros;
    cpl_size                    nzero = 0;

    cpl_ensure(image1     != NULL,       CPL_ERROR_NULL_INPUT,         NULL);
    cpl_ensure(image2     != NULL,       CPL_ERROR_NULL_INPUT,         NULL);
    cpl_ensure(image1->nx == image2->nx, CPL_ERROR_INCOMPATIBLE_INPUT, NULL);
    cpl_ensure(image1->ny == image2->ny, CPL_ERROR_INCOMPATIBLE_INPUT, NULL);

    /* Create the map of zero-divisors */
    zeros = cpl_mask_new(image2->nx, image2->ny);
    pzeros = cpl_mask_get_data(zeros);

    /* Switch on the first passed image type */
    switch (image1->type) {
    case CPL_TYPE_INT: {
        const int * p1 = (const int *)image1->pixels;
        int       * pout = (int *)cpl_malloc(image1->nx * image1->ny *
                                             sizeof(*pout));

        /* Switch on the second passed image type */
        switch (image2->type) {
#define CPL_TYPE_T CPL_TYPE_INT
#define CPL_TYPE   int
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_FLOAT
#define CPL_TYPE   float
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_DOUBLE
#define CPL_TYPE   double
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

        default:
            cpl_free(pout);
            (void)cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);
            pout = NULL;
        }
        self = pout ? cpl_image_wrap_int(image1->nx, image1->ny, pout) : NULL;
        break;
    }

    case CPL_TYPE_FLOAT: {
        const float * p1   = (const float *)image1->pixels;
        float       * pout = (float *)cpl_malloc(image1->nx * image1->ny *
                                                 sizeof(*pout));

        /* Switch on the second passed image type */
        switch (image2->type) {
#define CPL_TYPE_T CPL_TYPE_INT
#define CPL_TYPE   int
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_FLOAT
#define CPL_TYPE   float
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_DOUBLE
#define CPL_TYPE   double
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

        default:
            cpl_free(pout);
            (void)cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);
            pout = NULL;
        }
        self = pout ? cpl_image_wrap_float(image1->nx, image1->ny, pout) : NULL;
        break;
    }

    case CPL_TYPE_DOUBLE: {
        const double * p1   = (const double *)image1->pixels;
        double       * pout = (double *)cpl_malloc(image1->nx * image1->ny *
                                                 sizeof(*pout));

        /* Switch on the second passed image type */
        switch (image2->type) {
#define CPL_TYPE_T CPL_TYPE_INT
#define CPL_TYPE   int
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_FLOAT
#define CPL_TYPE   float
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_DOUBLE
#define CPL_TYPE   double
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

        default:
            cpl_free(pout);
            (void)cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);
            pout = NULL;
        }
        self = pout ? cpl_image_wrap_double(image1->nx, image1->ny, pout) : NULL;
        break;
    }

    case CPL_TYPE_FLOAT_COMPLEX: {
        const float complex * p1   = (const float complex *)image1->pixels;
        float complex       * pout = (float complex *)cpl_malloc(image1->nx *
                                                                 image1->ny *
                                                                 sizeof(*pout));

        /* Switch on the second passed image type */
        switch (image2->type) {
#define CPL_TYPE_T CPL_TYPE_INT
#define CPL_TYPE   int
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_FLOAT
#define CPL_TYPE   float
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_DOUBLE
#define CPL_TYPE   double
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_FLOAT_COMPLEX
#define CPL_TYPE   float complex
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_DOUBLE_COMPLEX
#define CPL_TYPE   double complex
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

        default:
            cpl_free(pout);
            (void)cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            pout = NULL;
        }
        self = pout ? cpl_image_wrap_float_complex(image1->nx, image1->ny, pout)
            : NULL;
        break;
    }

    case CPL_TYPE_DOUBLE_COMPLEX: {
        const double complex * p1   = (const double complex *)image1->pixels;
        double complex       * pout = (double complex *)cpl_malloc(image1->nx *
                                                                 image1->ny *
                                                                 sizeof(*pout));

        /* Switch on the second passed image type */
        switch (image2->type) {
#define CPL_TYPE_T CPL_TYPE_INT
#define CPL_TYPE   int
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_FLOAT
#define CPL_TYPE   float
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_DOUBLE
#define CPL_TYPE   double
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_FLOAT_COMPLEX
#define CPL_TYPE   float complex
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_DOUBLE_COMPLEX
#define CPL_TYPE   double complex
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE


        default:
            cpl_free(pout);
            (void)cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            pout = NULL;
        }
        self = pout ? cpl_image_wrap_double_complex(image1->nx, image1->ny,
                                                    pout) : NULL;
        break;
    }
    default:
        (void)cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        self = NULL;
    }

    if (nzero == image1->nx * image1->ny) {
        cpl_image_delete(self);
        (void)cpl_error_set_(CPL_ERROR_DIVISION_BY_ZERO);
        self = NULL;
    }

    if (self == NULL) {
        cpl_mask_delete(zeros);
    } else {
        /* Handle bad pixels map */
        if (image1->bpm == NULL && image2->bpm == NULL) {
            self->bpm = NULL;
        } else if (image1->bpm == NULL) {
            self->bpm = cpl_mask_duplicate(image2->bpm);
        } else if (image2->bpm == NULL) {
            self->bpm = cpl_mask_duplicate(image1->bpm);
        } else {
            self->bpm = cpl_mask_duplicate(image1->bpm);
            cpl_mask_or(self->bpm, image2->bpm);
        }

        /* Handle division by zero in the BPM */
        if (nzero != 0) {
            if (self->bpm == NULL) {
                self->bpm = zeros;
            } else {
                cpl_mask_or(self->bpm, zeros);
                cpl_mask_delete(zeros);
            }
        } else {
            cpl_mask_delete(zeros);
        }

        if (image1->type != CPL_TYPE_INT && image2->type != CPL_TYPE_INT) {
            cpl_tools_add_flops( image1->nx * image1->ny - nzero);
        }
    }

    return self;

#undef CPL_OPERATION

}

#define CPL_OPERATION CPL_IMAGE_BASIC_ASSIGN_LOCAL
/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Add two images, store the result in the first image.
  @param    im1     first operand.
  @param    im2     second operand.
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
  
  The first input image is modified to contain the result of the operation.

  The bad pixel map of the first image becomes the union of the bad pixel 
  maps of the input images.
  
  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INCOMPATIBLE_INPUT if the input images have different sizes
  - CPL_ERROR_TYPE_MISMATCH if the second input image has complex type
    while the first one does not
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_add(
        cpl_image       *   im1, 
        const cpl_image *   im2)
{
#define CPL_OPERATOR CPL_IMAGE_ADDITIONASSIGN
#include "cpl_image_basic_body.h"
#undef CPL_OPERATOR
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Subtract two images, store the result in the first image.
  @param    im1     first operand.
  @param    im2     second operand.
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
  @see      cpl_image_add()
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_subtract(
        cpl_image       *   im1, 
        const cpl_image *   im2)
{
#define CPL_OPERATOR CPL_IMAGE_SUBTRACTIONASSIGN
#include "cpl_image_basic_body.h"
#undef CPL_OPERATOR
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Multiply two images, store the result in the first image.
  @param    im1     first operand.
  @param    im2     second operand.
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
  @see      cpl_image_add()
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_multiply(
        cpl_image       *   im1, 
        const cpl_image *   im2)
{
    /* Faster version of code generated with gcc -ffast-math */
    /* (NaNs and other float specials are no longer IEEE compliant) */
#if (defined __SSE3__ || defined __SSE2__)
    cpl_ensure_code(im1     != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(im2     != NULL, CPL_ERROR_NULL_INPUT);

    if (im1->type == CPL_TYPE_FLOAT_COMPLEX &&
        im2->type == CPL_TYPE_FLOAT_COMPLEX) {
        cpl_ensure_code(im1->nx == im2->nx, CPL_ERROR_INCOMPATIBLE_INPUT);
        cpl_ensure_code(im1->ny == im2->ny, CPL_ERROR_INCOMPATIBLE_INPUT);
        cpl_image_multiply_fcomplex_sse_(im1, im2);
        return CPL_ERROR_NONE;
    }
        else if (im1->type == CPL_TYPE_DOUBLE_COMPLEX &&
                 im2->type == CPL_TYPE_DOUBLE_COMPLEX) {
        cpl_ensure_code(im1->nx == im2->nx, CPL_ERROR_INCOMPATIBLE_INPUT);
        cpl_ensure_code(im1->ny == im2->ny, CPL_ERROR_INCOMPATIBLE_INPUT);
        cpl_image_multiply_dcomplex_sse_(im1, im2);
        return CPL_ERROR_NONE;
    }
    else
#endif
    {
#define CPL_OPERATOR CPL_IMAGE_MULTIPLICATIONASSIGN
#include "cpl_image_basic_body.h"
#undef CPL_OPERATOR
    }
}
#undef CPL_OPERATION

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Divide two images, store the result in the first image.
  @param    im1     first operand.
  @param    im2     second operand.
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
  @see      cpl_image_add()
  @note
     The result of division with a zero-valued pixel is marked as a bad
     pixel. 

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INCOMPATIBLE_INPUT if the input images have different sizes
  - CPL_ERROR_TYPE_MISMATCH if the second input image has complex type
    while the first one does not
  - CPL_ERROR_DIVISION_BY_ZERO is all pixels in the divisor are zero
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_divide(cpl_image       *   im1, 
                                const cpl_image *   im2)
{
#define CPL_OPERATION CPL_IMAGE_BASIC_DIVIDE_LOCAL

    cpl_mask       * zeros;
    cpl_binary     * pzeros;
    cpl_size         nzero = 0;

    cpl_ensure_code(im1     != NULL,    CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(im2     != NULL,    CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(im1->nx == im2->nx, CPL_ERROR_INCOMPATIBLE_INPUT);
    cpl_ensure_code(im1->ny == im2->ny, CPL_ERROR_INCOMPATIBLE_INPUT);

    assert( im1->pixels );
    assert( im2->pixels );

    /* Create the zeros map */
    /* Do not modify im1->bpm now, in case of failure below */
    if (im1->bpm != NULL) {
        zeros = im1->bpm;
    } else if (im2->bpm != NULL) {
        zeros = cpl_mask_duplicate(im2->bpm);
    } else {
        zeros = cpl_mask_new(im1->nx, im1->ny);
    }

    pzeros = cpl_mask_get_data(zeros);

    /* Switch on the first passed image type */
    switch (im1->type) {
    case CPL_TYPE_INT: {
        int * p1 = (int *)im1->pixels;

        /* Switch on the second passed image type */
        switch (im2->type) {
#define CPL_TYPE_T CPL_TYPE_INT
#define CPL_TYPE   int
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_FLOAT
#define CPL_TYPE   float
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_DOUBLE
#define CPL_TYPE   double
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

        default:
            if (zeros != im1->bpm) cpl_mask_delete(zeros);
            return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);
        }
        break;
    }

    case CPL_TYPE_FLOAT: {
        float * p1 = (float *)im1->pixels;

        /* Switch on the second passed image type */
        switch (im2->type) {
#define CPL_TYPE_T CPL_TYPE_INT
#define CPL_TYPE   int
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_FLOAT
#define CPL_TYPE   float
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_DOUBLE
#define CPL_TYPE   double
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

        default:
            if (zeros != im1->bpm) cpl_mask_delete(zeros);
            return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);
        }
        break;
    }
    case CPL_TYPE_DOUBLE: {
        double * p1 = (double *)im1->pixels;

        /* Switch on the second passed image type */
        switch (im2->type) {
#define CPL_TYPE_T CPL_TYPE_INT
#define CPL_TYPE   int
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_FLOAT
#define CPL_TYPE   float
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_DOUBLE
#define CPL_TYPE   double
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

        default:
            if (zeros != im1->bpm) cpl_mask_delete(zeros);
            return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);
        }
        break;
    }
    case CPL_TYPE_FLOAT_COMPLEX: {
            float complex * p1 = (float complex *)im1->pixels;

            /* Switch on the second passed image type */
            switch (im2->type) {
#define CPL_TYPE_T CPL_TYPE_INT
#define CPL_TYPE   int
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_FLOAT
#define CPL_TYPE   float
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_DOUBLE
#define CPL_TYPE   double
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_FLOAT_COMPLEX
#define CPL_TYPE   float complex
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_DOUBLE_COMPLEX
#define CPL_TYPE   double complex
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

            default:
                if (zeros != im1->bpm) cpl_mask_delete(zeros);
                return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            }
            break;
    }
    case CPL_TYPE_DOUBLE_COMPLEX: {
        double complex * p1 = (double complex *)im1->pixels;

        /* Switch on the second passed image type */
        switch (im2->type) {
#define CPL_TYPE_T CPL_TYPE_INT
#define CPL_TYPE   int
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_FLOAT
#define CPL_TYPE   float
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_DOUBLE
#define CPL_TYPE   double
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_FLOAT_COMPLEX
#define CPL_TYPE   float complex
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_DOUBLE_COMPLEX
#define CPL_TYPE   double complex
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

        default:
            if (zeros != im1->bpm) cpl_mask_delete(zeros);
            return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        }
        break;
    }

    default:
        if (zeros != im1->bpm) cpl_mask_delete(zeros);
        return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
    }

    if (nzero == im1->nx * im1->ny) {
        if (zeros != im1->bpm) cpl_mask_delete(zeros);
        return cpl_error_set_(CPL_ERROR_DIVISION_BY_ZERO);
    }

    if (im1->type != CPL_TYPE_INT && im2->type != CPL_TYPE_INT) {
        cpl_tools_add_flops( im1->nx * im1->ny - nzero);
    }

    /* Handle bad pixels map */
    if (im1->bpm != NULL && im2->bpm != NULL && im1->bpm != im2->bpm) {
        assert( zeros == im1->bpm );
        cpl_mask_or(im1->bpm, im2->bpm);
    } else if (im1->bpm == NULL && (im2->bpm != NULL || nzero > 0)) {
        assert( zeros != NULL );
        im1->bpm = zeros;
    } else if (im1->bpm != zeros) {
        cpl_mask_delete(zeros);
    }

    return CPL_ERROR_NONE;
}

#undef CPL_OPERATION

#define CPL_OPERATION CPL_IMAGE_BASIC_OP_SCALAR
#define CPL_OPERATOR CPL_IMAGE_ADDITIONASSIGN
/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Elementwise addition of a scalar to an image
  @param    self    Image to be modified in place.
  @param    scalar  Number to add
  @return   CPL_ERROR_NONE or the relevant the #_cpl_error_code_ on error
 
  Modifies the image by adding a number to each of its pixels.

  The operation is always performed in double precision, with a final
  cast of the result to the image pixel type.
  
  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_add_scalar(cpl_image * self,
                                    double      scalar)
{
    cpl_ensure_code(self != NULL, CPL_ERROR_NULL_INPUT);
    
    /* Switch on image type */
    switch (cpl_image_get_type(self)) {
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_INT
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_DOUBLE_COMPLEX
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_FLOAT_COMPLEX
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
    default:
        return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
    }
    return CPL_ERROR_NONE;
}
#undef CPL_OPERATOR

#define CPL_OPERATOR CPL_IMAGE_SUBTRACTIONASSIGN
/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Elementwise subtraction of a scalar from an image
  @param    self    Image to be modified in place.
  @param    scalar  Number to subtract
  @return   CPL_ERROR_NONE or the relevant the #_cpl_error_code_ on error
  @see      cpl_image_add_scalar()
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_subtract_scalar(cpl_image * self,
                                         double      scalar)
{
    cpl_ensure_code(self != NULL, CPL_ERROR_NULL_INPUT);
    
    /* Switch on image type */
    switch (cpl_image_get_type(self)) {
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_INT
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_DOUBLE_COMPLEX
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_FLOAT_COMPLEX
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
    default:
        return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
    }
    return CPL_ERROR_NONE;
}
#undef CPL_OPERATOR

#define CPL_OPERATOR CPL_IMAGE_MULTIPLICATIONASSIGN
/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Elementwise multiplication of an image with a scalar
  @param    self    Image to be modified in place.
  @param    scalar  Number to multiply with
  @return   CPL_ERROR_NONE or the relevant the #_cpl_error_code_ on error
  @see      cpl_image_add_scalar()
 */
/*----------------------------------------------------------------------------*/
    cpl_error_code cpl_image_multiply_scalar(cpl_image * self,
                                             double      scalar)
{
    cpl_ensure_code(self != NULL, CPL_ERROR_NULL_INPUT);
    
    /* Switch on image type */
    switch (cpl_image_get_type(self)) {
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_INT
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_DOUBLE_COMPLEX
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_FLOAT_COMPLEX
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
    default:
        return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
    }
    return CPL_ERROR_NONE;
}
#undef CPL_OPERATOR

#define CPL_OPERATOR CPL_IMAGE_DIVISIONASSIGN
/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Elementwise division of an image with a scalar
  @param    self    Image to be modified in place.
  @param    scalar  Non-zero number to divide with
  @return   CPL_ERROR_NONE or the relevant the #_cpl_error_code_ on error
  @see      cpl_image_add_scalar()
 
  Modifies the image by dividing each of its pixels with a number.

  If the scalar is zero the image is not modified and an error is returned.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_DIVISION_BY_ZERO a division by 0 occurs
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_divide_scalar(cpl_image * self,
                                       double      scalar)
{

    cpl_ensure_code(self   != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(scalar != 0.0,  CPL_ERROR_DIVISION_BY_ZERO);
    
    /* Switch on image type */
    switch (cpl_image_get_type(self)) {
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_INT
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_DOUBLE_COMPLEX
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_FLOAT_COMPLEX
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
    default:
        return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
    }
    return CPL_ERROR_NONE;
}
#undef CPL_OPERATOR
#undef CPL_OPERATION

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Compute the elementwise logarithm of the image.
  @param    self   Image to be modified in place.
  @param    base   Base of the logarithm.
  @return   CPL_ERROR_NONE or the relevant the #_cpl_error_code_ on error
 
  Modifies the image by computing the base-scalar logarithm of each of its
  pixels.

  Images can be of type CPL_TYPE_INT, CPL_TYPE_FLOAT or CPL_TYPE_DOUBLE.

  Pixels for which the logarithm is not defined are
  rejected and set to zero.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INVALID_TYPE if the passed image type is not supported
  - CPL_ERROR_ILLEGAL_INPUT if base is non-positive
  - CPL_ERROR_DIVISION_BY_ZERO if the base equals 1
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_logarithm(cpl_image * self, double base)
{

    cpl_error_code error;

    cpl_ensure_code(self != NULL, CPL_ERROR_NULL_INPUT);

    /* Switch on image type */
    switch (cpl_image_get_type(self)) {
    case CPL_TYPE_INT:
        error = cpl_image_logarithm_int(self, base);
        break;
    case CPL_TYPE_FLOAT:
        error = cpl_image_logarithm_float(self, base);
        break;
    case CPL_TYPE_DOUBLE:
        error = cpl_image_logarithm_double(self, base);
        break;
    default:
        return cpl_error_set_message_(CPL_ERROR_INVALID_TYPE, "type='%s'. "
                                      "base=%g", cpl_type_get_name
                                      (cpl_image_get_type(self)), base);
    }

    return error ? cpl_error_set_where_() : CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Compute the elementwise exponential of the image.
  @param    self   Image to be modified in place.
  @param    base    Base of the exponential.
  @return   CPL_ERROR_NONE or the relevant the #_cpl_error_code_ on error
 
  Modifies the image by computing the base-scalar exponential of each of its
  pixels.

  Images can be of type CPL_TYPE_INT, CPL_TYPE_FLOAT or CPL_TYPE_DOUBLE.

  Pixels for which the power of the given base is not defined are
  rejected and set to zero.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INVALID_TYPE if the passed image type is not supported

 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_exponential(cpl_image * self, double base)
{

    cpl_error_code error;

    cpl_ensure_code(self != NULL, CPL_ERROR_NULL_INPUT);

    /* Switch on image type */
    switch (cpl_image_get_type(self)) {
    case CPL_TYPE_INT:
        error = cpl_image_exponential_int(self, base);
        break;
    case CPL_TYPE_FLOAT:
        error = cpl_image_exponential_float(self, base);
        break;
    case CPL_TYPE_DOUBLE:
        error = cpl_image_exponential_double(self, base);
        break;
    default:
        return cpl_error_set_message_(CPL_ERROR_INVALID_TYPE, "type='%s'. "
                                      "base=%g", cpl_type_get_name
                                      (cpl_image_get_type(self)), base);
    }

    return error ? cpl_error_set_where_() : CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Compute the elementwise power of the image.
  @param    self     Image to be modified in place.
  @param    exponent Scalar exponent.
  @return   CPL_ERROR_NONE or the relevant the #_cpl_error_code_ on error
 
  Modifies the image by lifting each of its pixels to exponent.

  Images can be of type CPL_TYPE_INT, CPL_TYPE_FLOAT or CPL_TYPE_DOUBLE.

  Pixels for which the power to the given exponent is not defined are
  rejected and set to zero.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INVALID_TYPE if the passed image type is not supported
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_power(cpl_image * self, double exponent)
{

    cpl_error_code error;

    cpl_ensure_code(self != NULL, CPL_ERROR_NULL_INPUT);

    /* Switch on image type */
    switch (cpl_image_get_type(self)) {
    case CPL_TYPE_INT:
        error = cpl_image_power_int(self, exponent);
        break;
    case CPL_TYPE_FLOAT:
        error = cpl_image_power_float(self, exponent);
        break;
    case CPL_TYPE_DOUBLE:
        error = cpl_image_power_double(self, exponent);
        break;
    default:
        return cpl_error_set_message_(CPL_ERROR_INVALID_TYPE, "type='%s'. "
                                      "exponent=%g", cpl_type_get_name
                                      (cpl_image_get_type(self)), exponent);
    }

    return error ? cpl_error_set_where_() : CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    The bit-wise and of two images with integer pixels
  @param    self     Pre-allocated image to hold the result
  @param    first    First operand, or NULL for an in-place operation
  @param    second   Second operand
  @return   CPL_ERROR_NONE or the relevant the #_cpl_error_code_ on error
  @note CPL_TYPE_INT is required
  @see cpl_mask_and() for the equivalent logical operation

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INCOMPATIBLE_INPUT if the images have different sizes
  - CPL_ERROR_INVALID_TYPE if the passed image type is as required
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_and(cpl_image * self,
                             const cpl_image * first,
                             const cpl_image * second)
{
    cpl_ensure_code(self   != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(second != NULL, CPL_ERROR_NULL_INPUT);

    cpl_ensure_code(self->nx == second->nx, CPL_ERROR_INCOMPATIBLE_INPUT);
    cpl_ensure_code(self->ny == second->ny, CPL_ERROR_INCOMPATIBLE_INPUT);

    if (cpl_image_get_type(self) != CPL_TYPE_INT) {
        return cpl_error_set_message_(CPL_ERROR_INVALID_TYPE,
                                      "self-type='%s', not int",
                                      cpl_type_get_name
                                      (cpl_image_get_type(self)));
    }
    if (first != NULL) {
        cpl_ensure_code(self->nx == first->nx, CPL_ERROR_INCOMPATIBLE_INPUT);
        cpl_ensure_code(self->ny == first->ny, CPL_ERROR_INCOMPATIBLE_INPUT);
        if (cpl_image_get_type(first) != CPL_TYPE_INT) {
            return cpl_error_set_message_(CPL_ERROR_INVALID_TYPE,
                                          "first-type='%s', not int",
                                          cpl_type_get_name
                                          (cpl_image_get_type(first)));
        }
    }
    if (cpl_image_get_type(second) != CPL_TYPE_INT) {
        return cpl_error_set_message_(CPL_ERROR_INVALID_TYPE,
                                      "second-type='%s', not int",
                                      cpl_type_get_name
                                      (cpl_image_get_type(second)));
    }

    /* Cannot fail now */

    /* Update the output bad pixel map */
    cpl_image_or_mask(self, first, second);

    cpl_mask_and_((cpl_binary*)self->pixels,
                  first ? (const cpl_binary*)first->pixels : NULL,
                  (const cpl_binary*)second->pixels,
                  cpl_type_get_sizeof(CPL_TYPE_INT)
                  *(size_t)(self->nx * self->ny));

    return CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    The bit-wise or of two images with integer pixels
  @param    self     Pre-allocated image to hold the result
  @param    first    First operand, or NULL for an in-place operation
  @param    second   Second operand
  @return   CPL_ERROR_NONE or the relevant the #_cpl_error_code_ on error
  @note CPL_TYPE_INT is required
  @see cpl_mask_or() for the equivalent logical operation

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INCOMPATIBLE_INPUT if the images have different sizes
  - CPL_ERROR_INVALID_TYPE if the passed image type is as required
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_or(cpl_image * self,
                            const cpl_image * first,
                            const cpl_image * second)
{
    cpl_ensure_code(self   != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(second != NULL, CPL_ERROR_NULL_INPUT);

    cpl_ensure_code(self->nx == second->nx, CPL_ERROR_INCOMPATIBLE_INPUT);
    cpl_ensure_code(self->ny == second->ny, CPL_ERROR_INCOMPATIBLE_INPUT);

    if (cpl_image_get_type(self) != CPL_TYPE_INT) {
        return cpl_error_set_message_(CPL_ERROR_INVALID_TYPE,
                                      "self-type='%s', not int",
                                      cpl_type_get_name
                                      (cpl_image_get_type(self)));
    }
    if (first != NULL) {
        cpl_ensure_code(self->nx == first->nx, CPL_ERROR_INCOMPATIBLE_INPUT);
        cpl_ensure_code(self->ny == first->ny, CPL_ERROR_INCOMPATIBLE_INPUT);
        if (cpl_image_get_type(first) != CPL_TYPE_INT) {
            return cpl_error_set_message_(CPL_ERROR_INVALID_TYPE,
                                          "first-type='%s', not int",
                                          cpl_type_get_name
                                          (cpl_image_get_type(first)));
        }
    }
    if (cpl_image_get_type(second) != CPL_TYPE_INT) {
        return cpl_error_set_message_(CPL_ERROR_INVALID_TYPE,
                                      "second-type='%s', not int",
                                      cpl_type_get_name
                                      (cpl_image_get_type(second)));
    }

    /* Cannot fail now */

    /* Update the output bad pixel map */
    cpl_image_or_mask(self, first, second);

    cpl_mask_or_((cpl_binary*)self->pixels,
                 first ? (const cpl_binary*)first->pixels : NULL,
                 (const cpl_binary*)second->pixels,
                 cpl_type_get_sizeof(CPL_TYPE_INT)
                 *(size_t)(self->nx * self->ny));

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
   @ingroup cpl_image
   @brief    The bit-wise xor of two images with integer pixels
   @param    self     Pre-allocated image to hold the result
   @param    first    First operand, or NULL for an in-place operation
   @param    second   Second operand
   @return   CPL_ERROR_NONE or the relevant the #_cpl_error_code_ on error
   @note CPL_TYPE_INT is required
   @see cpl_mask_xor() for the equivalent logical operation

   Possible #_cpl_error_code_ set in this function:
   - CPL_ERROR_NULL_INPUT if an input pointer is NULL
   - CPL_ERROR_INCOMPATIBLE_INPUT if the images have different sizes
   - CPL_ERROR_INVALID_TYPE if the passed image type is as required
*/
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_xor(cpl_image * self,
                             const cpl_image * first,
                             const cpl_image * second)
{
    cpl_ensure_code(self   != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(second != NULL, CPL_ERROR_NULL_INPUT);

    cpl_ensure_code(self->nx == second->nx, CPL_ERROR_INCOMPATIBLE_INPUT);
    cpl_ensure_code(self->ny == second->ny, CPL_ERROR_INCOMPATIBLE_INPUT);

    if (cpl_image_get_type(self) != CPL_TYPE_INT) {
        return cpl_error_set_message_(CPL_ERROR_INVALID_TYPE,
                                      "self-type='%s', not int",
                                      cpl_type_get_name
                                      (cpl_image_get_type(self)));
    }
    if (first != NULL) {
        cpl_ensure_code(self->nx == first->nx, CPL_ERROR_INCOMPATIBLE_INPUT);
        cpl_ensure_code(self->ny == first->ny, CPL_ERROR_INCOMPATIBLE_INPUT);
        if (cpl_image_get_type(first) != CPL_TYPE_INT) {
            return cpl_error_set_message_(CPL_ERROR_INVALID_TYPE,
                                          "first-type='%s', not int",
                                          cpl_type_get_name
                                          (cpl_image_get_type(first)));
        }
    }
    if (cpl_image_get_type(second) != CPL_TYPE_INT) {
        return cpl_error_set_message_(CPL_ERROR_INVALID_TYPE,
                                      "second-type='%s', not int",
                                      cpl_type_get_name
                                      (cpl_image_get_type(second)));
    }


    /* Cannot fail now */

    /* Update the output bad pixel map */
    cpl_image_or_mask(self, first, second);

    cpl_mask_xor_((cpl_binary*)self->pixels,
                  first ? (const cpl_binary*)first->pixels : NULL,
                  (const cpl_binary*)second->pixels,
                  cpl_type_get_sizeof(CPL_TYPE_INT)
                  *(size_t)(self->nx * self->ny));

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
   @ingroup cpl_image
   @brief    The bit-wise complement (not) of an image with integer pixels
   @param    self     Pre-allocated image to hold the result
   @param    first    First operand, or NULL for an in-place operation
   @return   CPL_ERROR_NONE or the relevant the #_cpl_error_code_ on error
   @note CPL_TYPE_INT is required
   @see cpl_mask_not() for the equivalent logical operation

   Possible #_cpl_error_code_ set in this function:
   - CPL_ERROR_NULL_INPUT if an input pointer is NULL
   - CPL_ERROR_INCOMPATIBLE_INPUT if the images have different sizes
   - CPL_ERROR_INVALID_TYPE if the passed image type is as required
*/
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_not(cpl_image * self,
                             const cpl_image * first)
{
    cpl_ensure_code(self   != NULL, CPL_ERROR_NULL_INPUT);

    if (cpl_image_get_type(self) != CPL_TYPE_INT) {
        return cpl_error_set_message_(CPL_ERROR_INVALID_TYPE,
                                      "self-type='%s', not int",
                                      cpl_type_get_name
                                      (cpl_image_get_type(self)));
    }
    if (first != NULL) {
        cpl_ensure_code(self->nx == first->nx, CPL_ERROR_INCOMPATIBLE_INPUT);
        cpl_ensure_code(self->ny == first->ny, CPL_ERROR_INCOMPATIBLE_INPUT);
        if (cpl_image_get_type(first) != CPL_TYPE_INT) {
            return cpl_error_set_message_(CPL_ERROR_INVALID_TYPE,
                                          "first-type='%s', not int",
                                          cpl_type_get_name
                                          (cpl_image_get_type(first)));
        }
    }

    /* Cannot fail now */

    /* Update the output bad pixel map */
    cpl_image_or_mask_unary(self, first);

    cpl_mask_xor_scalar((cpl_binary*)self->pixels,
                        first ? (const cpl_binary*)first->pixels : NULL,
                        (cpl_bitmask)-1,
                        cpl_type_get_sizeof(CPL_TYPE_INT)
                        *(size_t)(self->nx * self->ny));

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    The bit-wise and of a scalar and an image with integer pixels
  @param    self     Pre-allocated image to hold the result
  @param    first    First operand, or NULL for an in-place operation
  @param    second   Second operand (scalar)
  @return   CPL_ERROR_NONE or the relevant the #_cpl_error_code_ on error
  @note CPL_TYPE_INT is required

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INCOMPATIBLE_INPUT if the images have different sizes
  - CPL_ERROR_INVALID_TYPE if the passed image type is as required
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_and_scalar(cpl_image * self,
                                    const cpl_image * first,
                                    cpl_bitmask second)
{
    cpl_ensure_code(self != NULL, CPL_ERROR_NULL_INPUT);

    if (cpl_image_get_type(self) != CPL_TYPE_INT) {
        return cpl_error_set_message_(CPL_ERROR_INVALID_TYPE,
                                      "self-type='%s', not int",
                                      cpl_type_get_name
                                      (cpl_image_get_type(self)));
    }
    if (first != NULL) {
        cpl_ensure_code(self->nx == first->nx, CPL_ERROR_INCOMPATIBLE_INPUT);
        cpl_ensure_code(self->ny == first->ny, CPL_ERROR_INCOMPATIBLE_INPUT);
        if (cpl_image_get_type(first) != CPL_TYPE_INT) {
            return cpl_error_set_message_(CPL_ERROR_INVALID_TYPE,
                                          "first-type='%s', not int",
                                          cpl_type_get_name
                                          (cpl_image_get_type(first)));
        }
    }

    /* Cannot fail now */

    /* Update the output bad pixel map */
    cpl_image_or_mask_unary(self, first);

    cpl_mask_and_scalar((cpl_binary*)self->pixels,
                        first ? (const cpl_binary*)first->pixels : NULL,
                        (second & (uint32_t)-1) | (second << 32),
                         cpl_type_get_sizeof(CPL_TYPE_INT)
                         *(size_t)(self->nx * self->ny));

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    The bit-wise or of a scalar and an image with integer pixels
  @param    self     Pre-allocated image to hold the result
  @param    first    First operand, or NULL for an in-place operation
  @param    second   Second operand (scalar)
  @return   CPL_ERROR_NONE or the relevant the #_cpl_error_code_ on error
  @note CPL_TYPE_INT is required

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INCOMPATIBLE_INPUT if the images have different sizes
  - CPL_ERROR_INVALID_TYPE if the passed image type is as required
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_or_scalar(cpl_image * self,
                                    const cpl_image * first,
                                    cpl_bitmask second)
{
    cpl_ensure_code(self != NULL, CPL_ERROR_NULL_INPUT);

    if (cpl_image_get_type(self) != CPL_TYPE_INT) {
        return cpl_error_set_message_(CPL_ERROR_INVALID_TYPE,
                                      "self-type='%s', not int",
                                      cpl_type_get_name
                                      (cpl_image_get_type(self)));
    }
    if (first != NULL) {
        cpl_ensure_code(self->nx == first->nx, CPL_ERROR_INCOMPATIBLE_INPUT);
        cpl_ensure_code(self->ny == first->ny, CPL_ERROR_INCOMPATIBLE_INPUT);
        if (cpl_image_get_type(first) != CPL_TYPE_INT) {
            return cpl_error_set_message_(CPL_ERROR_INVALID_TYPE,
                                          "first-type='%s', not int",
                                          cpl_type_get_name
                                          (cpl_image_get_type(first)));
        }
    }

    /* Cannot fail now */

    /* Update the output bad pixel map */
    cpl_image_or_mask_unary(self, first);

    cpl_mask_or_scalar((cpl_binary*)self->pixels,
                       first ? (const cpl_binary*)first->pixels : NULL,
                       (second & (uint32_t)-1) | (second << 32),
                       cpl_type_get_sizeof(CPL_TYPE_INT)
                       *(size_t)(self->nx * self->ny));

    return CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    The bit-wise xor of a scalar and an image with integer pixels
  @param    self     Pre-allocated image to hold the result
  @param    first    First operand, or NULL for an in-place operation
  @param    second   Second operand (scalar)
  @return   CPL_ERROR_NONE or the relevant the #_cpl_error_code_ on error
  @note CPL_TYPE_INT is required

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INCOMPATIBLE_INPUT if the images have different sizes
  - CPL_ERROR_INVALID_TYPE if the passed image type is as required
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_xor_scalar(cpl_image * self,
                                    const cpl_image * first,
                                    cpl_bitmask second)
{
    cpl_ensure_code(self != NULL, CPL_ERROR_NULL_INPUT);

    if (cpl_image_get_type(self) != CPL_TYPE_INT) {
        return cpl_error_set_message_(CPL_ERROR_INVALID_TYPE,
                                      "self-type='%s', not int",
                                      cpl_type_get_name
                                      (cpl_image_get_type(self)));
    }
    if (first != NULL) {
        cpl_ensure_code(self->nx == first->nx, CPL_ERROR_INCOMPATIBLE_INPUT);
        cpl_ensure_code(self->ny == first->ny, CPL_ERROR_INCOMPATIBLE_INPUT);
        if (cpl_image_get_type(first) != CPL_TYPE_INT) {
            return cpl_error_set_message_(CPL_ERROR_INVALID_TYPE,
                                          "first-type='%s', not int",
                                          cpl_type_get_name
                                          (cpl_image_get_type(first)));
        }
    }

    /* Cannot fail now */

    /* Update the output bad pixel map */
    cpl_image_or_mask_unary(self, first);

    cpl_mask_xor_scalar((cpl_binary*)self->pixels,
                        first ? (const cpl_binary*)first->pixels : NULL,
                        (second & (uint32_t)-1) | (second << 32),
                        cpl_type_get_sizeof(CPL_TYPE_INT)
                        *(size_t)(self->nx * self->ny));

    return CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    The pixel-wise Euclidean distance function of the images
  @param    self     Pre-allocated image to hold the result
  @param    first    First operand, or NULL for an in-place operation
  @param    second   Second operand
  @return   CPL_ERROR_NONE or the relevant the #_cpl_error_code_ on error

  The Euclidean distance function is useful for gaussian error propagation
  on addition/subtraction operations.

  For pixel values a and b the Euclidean distance c is defined as:
  $$c = sqrt{a^2 + b^2}$$

  first may be NULL, in this case the distance is computed in-place on self
  using second as the other operand.

  Images can be of type CPL_TYPE_FLOAT or CPL_TYPE_DOUBLE.

  If both input operands are of type CPL_TYPE_FLOAT the distance is computed
  in single precision (using hypotf()), otherwise in double precision
  (using hypot()).

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INCOMPATIBLE_INPUT if the images have different sizes
  - CPL_ERROR_INVALID_TYPE if the passed image type is not supported
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_hypot(cpl_image * self,
                               const cpl_image * first,
                               const cpl_image * second)
{

    cpl_error_code error;
    /* Only used to determine the type of the first hypot operand */
    const cpl_image * myfirst = first ? first : second;

    cpl_ensure_code(self   != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(second != NULL, CPL_ERROR_NULL_INPUT);

    if (cpl_image_get_type(self) != CPL_TYPE_DOUBLE &&
        cpl_image_get_type(self) != CPL_TYPE_FLOAT) {
        return cpl_error_set_message_(CPL_ERROR_INVALID_TYPE,
                                      "self-type='%s'", cpl_type_get_name
                                      (cpl_image_get_type(self)));
    }
    if (first != NULL && cpl_image_get_type(first) != CPL_TYPE_DOUBLE &&
        cpl_image_get_type(first) != CPL_TYPE_FLOAT) {
        return cpl_error_set_message_(CPL_ERROR_INVALID_TYPE,
                                      "first-type='%s'", cpl_type_get_name
                                      (cpl_image_get_type(first)));
    }
    if (cpl_image_get_type(second) != CPL_TYPE_DOUBLE &&
        cpl_image_get_type(second) != CPL_TYPE_FLOAT) {
        return cpl_error_set_message_(CPL_ERROR_INVALID_TYPE,
                                      "second-type='%s'", cpl_type_get_name
                                      (cpl_image_get_type(second)));
    }

    if (first != NULL) {
        cpl_ensure_code(self->nx == first->nx, CPL_ERROR_INCOMPATIBLE_INPUT);
        cpl_ensure_code(self->ny == first->ny, CPL_ERROR_INCOMPATIBLE_INPUT);
    }
    cpl_ensure_code(self->nx == second->nx, CPL_ERROR_INCOMPATIBLE_INPUT);
    cpl_ensure_code(self->ny == second->ny, CPL_ERROR_INCOMPATIBLE_INPUT);


    /* Switch on image type */
    switch ((cpl_image_get_type(self)    == CPL_TYPE_FLOAT ? 4 : 0) +
            (cpl_image_get_type(myfirst) == CPL_TYPE_FLOAT ? 2 : 0) +
            (cpl_image_get_type(second)  == CPL_TYPE_FLOAT ? 1 : 0)) {

    case 7: /* float, float, float */
        error = cpl_image_hypot_float_float_float(self, first, second);
        break;
    case 6: /* float, float, double */
        error = cpl_image_hypot_float_float_double(self, first, second);
        break;
    case 5: /* float, double, float */
        error = cpl_image_hypot_float_double_float(self, first, second);
        break;
    case 4: /* float, double, double */
        error = cpl_image_hypot_float_double_double(self, first, second);
        break;
    case 3: /* double, float, float */
        error = cpl_image_hypot_double_float_float(self, first, second);
        break;
    case 2: /* double, float, double */
        error = cpl_image_hypot_double_float_double(self, first, second);
        break;
    case 1: /* double, double, float */
        error = cpl_image_hypot_double_double_float(self, first, second);
        break;
    default: /* double, double, double */
        error = cpl_image_hypot_double_double_double(self, first, second);
        break;
    }

    return error ? cpl_error_set_where_() : CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Normalise pixels in an image.
  @param    image    Image operand.
  @param    mode     Normalisation mode.
  @return   CPL_ERROR_NONE, or the relevant #_cpl_error_code_ on error.
 
  Normalises an image according to a given criterion.
 
  Possible normalisations are:
  - CPL_NORM_SCALE sets the pixel interval to [0,1].
  - CPL_NORM_MEAN sets the mean value to 1.
  - CPL_NORM_FLUX sets the flux to 1.
  - CPL_NORM_ABSFLUX sets the absolute flux to 1.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_normalise(
        cpl_image * image, 
        cpl_norm    mode)
{
    double scale;


    cpl_ensure_code(image, CPL_ERROR_NULL_INPUT);

    switch (mode) {
        case CPL_NORM_SCALE: {
            cpl_stats stats;

            if (cpl_stats_fill_from_image(&stats, image, CPL_STATS_MIN
                                          | CPL_STATS_MAX)) {
                return cpl_error_set_where_();
            }
            scale = cpl_stats_get_max(&stats) - cpl_stats_get_min(&stats);
            if (scale > 0 &&
                cpl_image_subtract_scalar(image, cpl_stats_get_min(&stats))) {
                return cpl_error_set_where_();
            }
            break;
        }
        case CPL_NORM_MEAN: {
            scale = cpl_image_get_mean(image);
            break;
    }
        case CPL_NORM_FLUX: {
            scale = cpl_image_get_flux(image);
            break;
    }
        case CPL_NORM_ABSFLUX: {
            scale = cpl_image_get_absflux(image);
            break;
    }
        default:
            /* This case can only be reached if cpl_norm is extended in error */
            return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
    }

    cpl_ensure_code( !cpl_image_divide_scalar(image, scale),
                     cpl_error_get_code());

    return CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Create a new normalised image from an existing image.
  @param    image_in    Image operand.
  @param    mode        Normalisation mode.
  @return   1 newly allocated image or NULL on error
  @see      cpl_image_normalise 

  Stores the result in a newly allocated image and returns it.
  The returned image must be deallocated using cpl_image_delete().
 
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_normalise_create(
        const cpl_image *   image_in, 
        cpl_norm            mode)
{
    cpl_image  *   image_out;


    cpl_ensure(image_in, CPL_ERROR_NULL_INPUT, NULL);

    image_out = cpl_image_duplicate(image_in);

    if (cpl_image_normalise(image_out, mode)) {
        cpl_image_delete(image_out);
        image_out = NULL;
        (void)cpl_error_set_where_();
    }

    return image_out;
}


/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Create a new image by elementwise addition of a scalar to an image
  @param    image   Image to add
  @param    addend  Number to add
  @return   1 newly allocated image or NULL in case of an error
  @see      cpl_image_add_scalar

  Creates a new image, being the result of the operation, and returns it to
  the caller. The returned image must be deallocated using cpl_image_delete().
  The function supports images with different types among CPL_TYPE_INT, 
  CPL_TYPE_FLOAT and CPL_TYPE_DOUBLE. The type of the created image is that of
  the passed image.
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_add_scalar_create(
        const cpl_image   *   image,
        double                addend)
{
    cpl_image  * result =  cpl_image_duplicate(image);

    cpl_ensure(result, cpl_error_get_code(), NULL);

    if (cpl_image_add_scalar(result, addend)) {
        cpl_image_delete(result);
        result = NULL;
        (void)cpl_error_set_where_();
    }
    return result;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Create an image by elementwise subtraction of a scalar from an image
  @param    image       Image to be subtracted from
  @param    subtrahend  Number to subtract
  @return   1 newly allocated image or NULL in case of an error
  @see      cpl_image_subtract_scalar
  @see      cpl_image_add_scalar_create
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_subtract_scalar_create(
        const cpl_image   *   image,
        double                subtrahend)
{
    cpl_image  * result =  cpl_image_duplicate(image);


    cpl_ensure(result, cpl_error_get_code(), NULL);

    if (cpl_image_subtract_scalar(result, subtrahend)) {
        cpl_image_delete(result);
        result = NULL;
        (void)cpl_error_set_where_();
    }

    return result;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Create a new image by multiplication of a scalar and an image
  @param    image   Image to be multiplied
  @param    factor  Number to multiply with
  @return   1 newly allocated image or NULL in case of an error
  @see      cpl_image_multiply_scalar
  @see      cpl_image_add_scalar_create
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_multiply_scalar_create(
        const cpl_image   *   image,
        double                factor)
{
    cpl_image  * result =  cpl_image_duplicate(image);


    cpl_ensure(result, cpl_error_get_code(), NULL);

    if (cpl_image_multiply_scalar(result, factor)) {
        cpl_image_delete(result);
        result = NULL;
        (void)cpl_error_set_where_();
    }

    return result;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Create a new image by elementwise division of an image with a scalar
  @param    image    Image to divide
  @param    divisor  Non-zero number to divide with
  @return   1 newly allocated image or NULL in case of an error
  @see      cpl_image_divide_scalar
  @see      cpl_image_add_scalar_create
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_divide_scalar_create(
        const cpl_image   *   image,
        double                divisor)
{
    cpl_image  * result =  cpl_image_duplicate(image);


    cpl_ensure(result, cpl_error_get_code(), NULL);

    if (cpl_image_divide_scalar(result, divisor)) {
        cpl_image_delete(result);
        result = NULL;
        (void)cpl_error_set_where_();
    }

    return result;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Create a new image by taking the elementwise logarithm of an image
  @param    image   Image to take logarithm of
  @param    base    Base of the logarithm.
  @return   1 newly allocated image or NULL in case of an error
  @see      cpl_image_logarithm
  @see      cpl_image_add_scalar_create
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_logarithm_create(
        const cpl_image   *   image,
        double          base)
{
    cpl_image  * result =  cpl_image_duplicate(image);

    cpl_ensure(result, cpl_error_get_code(), NULL);

    if (cpl_image_logarithm(result, base)) {
        cpl_image_delete(result);
        result = NULL;
        (void)cpl_error_set_where_();
    }

    return result;
}
        
/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Create a new image by elementwise exponentiation of an image
  @param    image   Image to exponentiate
  @param    base    Base of the exponential
  @return   1 newly allocated image or NULL in case of an error
  @see      cpl_image_logarithm
  @see      cpl_image_add_scalar_create
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_exponential_create(
        const cpl_image   *   image,
        double          base)
{
    cpl_image  * result =  cpl_image_duplicate(image);

    cpl_ensure(result, cpl_error_get_code(), NULL);

    if (cpl_image_exponential(result, base)) {
        cpl_image_delete(result);
        result = NULL;
        (void)cpl_error_set_where_();
    }

    return result;
}
        
/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Create a new image by elementwise raising of an image to a power
  @param    image    Image to raise to a power
  @param    exponent scalar exponent
  @return   1 newly allocated image or NULL in case of an error
  @see      cpl_image_power
  @see      cpl_image_add_scalar_create
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_power_create(
        const cpl_image   *   image,
        double          exponent)
{
    cpl_image  * result =  cpl_image_duplicate(image);

    cpl_ensure(result, cpl_error_get_code(), NULL);

    if (cpl_image_power(result, exponent)) {
        cpl_image_delete(result);
        result = NULL;
        (void)cpl_error_set_where_();
    }

    return result;
}
        
#define CPL_OPERATION CPL_IMAGE_BASIC_THRESHOLD
/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Threshold an image to a given interval.
  @param    image_in            Image to threshold.
  @param    lo_cut              Lower bound.
  @param    hi_cut              Higher bound.
  @param    assign_lo_cut       Value to assign to pixels below low bound.
  @param    assign_hi_cut       Value to assign to pixels above high bound.
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
  
  Pixels outside of the provided interval are assigned the given values.

  Use FLT_MIN and FLT_MAX for floating point images and DBL_MIN and DBL_MAX 
  for double images for the lo_cut and hi_cut to avoid any pixel
  replacement.
  
  Images can be of type CPL_TYPE_INT, CPL_TYPE_FLOAT or CPL_TYPE_DOUBLE.
  lo_cut must be smaller than or equal to hi_cut.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INVALID_TYPE if the passed image type is not supported
  - CPL_ERROR_ILLEGAL_INPUT if lo_cut is greater than hi_cut
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_threshold(
        cpl_image *   image_in,
        double              lo_cut,
        double              hi_cut,
        double              assign_lo_cut,
        double              assign_hi_cut)
{

    cpl_ensure_code(image_in != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(lo_cut <= hi_cut, CPL_ERROR_ILLEGAL_INPUT);

    /* Switch on image type */
    switch (image_in->type) {
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_INT
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
        default:
          return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
    }
    return CPL_ERROR_NONE;
}
#undef CPL_OPERATION

#define CPL_OPERATION CPL_IMAGE_BASIC_ABS
/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Take the absolute value of an image.
  @param    image    Image to be modified in place
  @return   CPL_ERROR_NONE or the relevant the #_cpl_error_code_ on error
  
  Set each pixel to its absolute value.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_abs(cpl_image * image)
{
    /* Check entries */
    cpl_ensure_code(image, CPL_ERROR_NULL_INPUT);

    /* Switch on image type */
    switch (image->type) {
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_INT
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
        default:
            return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
    }

    return CPL_ERROR_NONE;
}

#undef CPL_OPERATION

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Take the absolute value of an image.
  @param    image_in    Image operand.
  @return   1 newly allocated image or NULL on error
  @see      cpl_image_abs
  
  For each pixel, out = abs(in). The returned image must be deallocated using
  cpl_image_delete().
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_abs_create(const cpl_image * image_in)
{

    cpl_image  * result = cpl_image_duplicate(image_in);

    cpl_ensure(result, cpl_error_get_code(), NULL);

    if (cpl_image_abs(result)) {
        cpl_image_delete(result);
        result = NULL;
        (void)cpl_error_set_where_();
    }

    return result;
}

#define CPL_OPERATION CPL_IMAGE_BASIC_AVERAGE
/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Build the average of two images.
  @param    image_1     First image operand.
  @param    image_2     Second image operand.
  @return   1 newly allocated image or NULL on error
 
  Builds the average of two images and returns a newly allocated image,
  to be deallocated using cpl_image_delete(). The average is arithmetic, i.e.
  outpix=(pix1+pix2)/2
  Images can be of type CPL_TYPE_INT, CPL_TYPE_FLOAT or CPL_TYPE_DOUBLE.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INVALID_TYPE if the passed image type is not supported
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_average_create(
        const cpl_image *   image_1,
        const cpl_image *   image_2)
{   
    cpl_image   *   image_out;
    int         *   pii2;
    float       *   pfi2;
    double      *   pdi2;

    /* Check entries   */
    cpl_ensure(image_1 && image_2, CPL_ERROR_NULL_INPUT, NULL);
   
    /* Input data images shall have the same sizes */
    cpl_ensure(image_1->nx == image_2->nx && image_1->ny == image_2->ny,
               CPL_ERROR_ILLEGAL_INPUT, NULL);

    /* Switch on first passed image type */
    switch (image_1->type) {
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_INT
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
        default:
            (void)cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            return NULL;
    }
    
    /* Handle bad pixels map */
    if (image_1->bpm == NULL && image_2->bpm == NULL) {
        image_out->bpm = NULL;
    } else if (image_1->bpm == NULL) {
        image_out->bpm = cpl_mask_duplicate(image_2->bpm);
    } else if (image_2->bpm == NULL) {
        image_out->bpm = cpl_mask_duplicate(image_1->bpm);
    } else {
        image_out->bpm = cpl_mask_duplicate(image_1->bpm);
        cpl_mask_or(image_out->bpm, image_2->bpm);
    }

    return image_out;
}
#undef CPL_OPERATION


/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Collapse an image region along its rows or columns.
  @param    self        Image to collapse.
  @param    llx         lower left x coord.
  @param    lly         lower left y coord
  @param    urx         upper right x coord
  @param    ury         upper right y coord
  @param    direction   Collapsing direction.
  @return   a newly allocated image or NULL on error
  @see      cpl_image_collapse_create()
  
  llx, lly, urx, ury are the image region coordinates in FITS convention.
  Those specified bounds are included in the collapsed region.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the specified window is not valid
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_collapse_window_create(const cpl_image * self,
                                             cpl_size          llx,
                                             cpl_size          lly,
                                             cpl_size          urx,
                                             cpl_size          ury,
                                             int               direction)
{
    cpl_image * other;


    /* Switch on image type */
    switch (cpl_image_get_type(self)) {
    case CPL_TYPE_DOUBLE:
        other = cpl_image_collapse_window_create_double(self, llx, lly, urx,
                                                        ury, direction);
        break;
    case CPL_TYPE_FLOAT:
        other = cpl_image_collapse_window_create_float(self, llx, lly, urx,
                                                       ury, direction);
        break;
    case CPL_TYPE_INT:
        other = cpl_image_collapse_window_create_int(self, llx, lly, urx,
                                                     ury, direction);
        break;
    default:
        /* NULL input will be go here, after having set a CPL error */
        other = NULL;
    }

    /* Propagate error, if any */
    if (other == NULL) (void)cpl_error_set_where_();
    
    return other;
}
/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Collapse an image along its rows or columns.
  @param    self       Input image to collapse.
  @param    direction  Collapsing direction.
  @return   1 newly allocated 1D image or NULL on error
 
  On success the function returns a 1D image, created by adding up all pixels
  on the same row or column.
 
  @verbatim
  Collapse along y (sum of rows):
 
  p7  p8  p9     Input image is a 3x3 image containing 9 pixels.
  p4  p5  p6     The output is an image containing one row with
  p1  p2  p3     3 pixels A, B, C, where:
  ----------
 
  A   B   C      A = p1+p4+p7
                 B = p2+p5+p8
                 C = p3+p6+p9

  If p7 is a bad pixel, A = (p1+p4)*3/2.
  If p1, p4, p7 are bad, A is flagged as bad.
  @endverbatim
 
  Provide the collapsing direction as an int. Give 0 to collapse along y
  (sum of rows) and get an image with a single row in output, or give 1
  to collapse along x (sum of columns) to get an image with a single
  column in output.
  Only the good pixels are collapsed.
  Images can be of type CPL_TYPE_INT, CPL_TYPE_FLOAT or CPL_TYPE_DOUBLE.
  The returned image must be deallocated using cpl_image_delete().

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INVALID_TYPE if the passed image type is not supported
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_collapse_create(const cpl_image * self,
                                      int               direction)
{
    cpl_image * other
        = cpl_image_collapse_window_create(self, 1, 1,
                                           cpl_image_get_size_x(self),
                                           cpl_image_get_size_y(self),
                                           direction);

    /* Propagate error, if any */
    cpl_ensure(other != NULL, cpl_error_get_code(), NULL);
   
    return other;
}

#define CPL_OPERATION CPL_IMAGE_BASIC_COLLAPSE_MEDIAN
/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Collapse an image along its rows or columns, with filtering.
  @param    self      Input image to collapse.
  @param    direction Collapsing direction.
  @param    drop_ll   Ignore this many lower rows/leftmost columns
  @param    drop_ur   Ignore this many upper rows/rightmost columns
  @return   1 newly allocated image having 1 row or 1 column or NULL on error
  @see      cpl_image_collapse_create()
  
  The collapsing direction is defined as for cpl_image_collapse_create().
  For each output pixel, the median of the corresponding non-ignored pixels
  is computed. A combination of bad pixels and drop parameters can cause a
  median value in the output image to be undefined. Such pixels will be
  flagged as bad and set to zero.

  If the output would contain only bad pixels an error is set.

  Images can be of type CPL_TYPE_INT, CPL_TYPE_FLOAT or CPL_TYPE_DOUBLE.
  The returned image must be deallocated using cpl_image_delete().

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if a rejection parameter is negative,
    or if the sum of ignored pixels is bigger than the image size
    in the collapsing direction
  - CPL_ERROR_INVALID_TYPE if the passed image type is not supported
  - CPL_ERROR_DATA_NOT_FOUND if the output image would have only bad pixels
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_collapse_median_create(const cpl_image * self,
                                             int               direction,
                                             cpl_size          drop_ll,
                                             cpl_size          drop_ur)
{
    cpl_image    * other = NULL;
    const cpl_size ndrop = drop_ll + drop_ur;

    cpl_ensure(self != NULL,    CPL_ERROR_NULL_INPUT,    NULL);
    cpl_ensure(drop_ll >= 0, CPL_ERROR_ILLEGAL_INPUT, NULL);
    cpl_ensure(drop_ur >= 0, CPL_ERROR_ILLEGAL_INPUT, NULL);

    if (direction == 0) {
        cpl_ensure(ndrop < self->ny, CPL_ERROR_ILLEGAL_INPUT, NULL);
    } else if (direction == 1) {
        cpl_ensure(ndrop < self->nx, CPL_ERROR_ILLEGAL_INPUT, NULL);
    } else {
        (void)cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }
    
    /* Switch on image type */
    switch (self->type) {
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_INT
#include "cpl_image_basic_body.h"
#undef CPL_CLASS

        default:
            (void)cpl_error_set_(CPL_ERROR_INVALID_TYPE);
    } 
    
    return other;
}
#undef CPL_OPERATION

#define CPL_OPERATION CPL_IMAGE_BASIC_EXTRACT
/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Extract a rectangular zone from an image into another image.
  @param    in    Input image
  @param    llx   Lower left X coordinate
  @param    lly   Lower left Y coordinate
  @param    urx   Upper right X coordinate
  @param    ury   Upper right Y coordinate
  @return   1 newly allocated image or NULL on error
  @note  The returned image must be deallocated using cpl_image_delete()
 
  The input coordinates define the extracted region by giving the coordinates 
  of the lower left and upper right corners (inclusive).
 
  Coordinates must be provided in the FITS convention: lower left
  corner of the image is at (1,1), x increasing from left to right,
  y increasing from bottom to top.
  Images can be of type CPL_TYPE_INT, CPL_TYPE_FLOAT or CPL_TYPE_DOUBLE.

  If the input image has a bad pixel map and if the extracted rectangle has
  bad pixel(s), then the extracted image will have a bad pixel map, otherwise
  it will not.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the window coordinates are not valid
  - CPL_ERROR_INVALID_TYPE if the passed image type is not supported
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_extract(const cpl_image * in,
                              cpl_size          llx,
                              cpl_size          lly,
                              cpl_size          urx,
                              cpl_size          ury)
{
    cpl_image * self = NULL;

    cpl_ensure(in != NULL,     CPL_ERROR_NULL_INPUT,    NULL);

    cpl_ensure(llx  >= 1,     CPL_ERROR_ILLEGAL_INPUT, NULL);
    cpl_ensure(llx  <= urx,   CPL_ERROR_ILLEGAL_INPUT, NULL);
    cpl_ensure(urx <= in->nx, CPL_ERROR_ILLEGAL_INPUT, NULL);

    cpl_ensure(lly  >= 1,     CPL_ERROR_ILLEGAL_INPUT, NULL);
    cpl_ensure(lly  <= ury,   CPL_ERROR_ILLEGAL_INPUT, NULL);
    cpl_ensure(ury <= in->ny, CPL_ERROR_ILLEGAL_INPUT, NULL);

    /* Switch on image type */
    switch (in->type) {
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_INT
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_DOUBLE_COMPLEX
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_FLOAT_COMPLEX
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
        default:
            /* It is an error in CPL to enter here */
            (void)cpl_error_set_(CPL_ERROR_INVALID_TYPE);
    }

    if (self != NULL && in->bpm != NULL) {
        /* Bad pixels handling */
        self->bpm = cpl_mask_extract_(in->bpm, llx, lly, urx, ury);
    }

    return self;
}
#undef CPL_OPERATION

#define CPL_OPERATION CPL_IMAGE_BASIC_EXTRACTROW
/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Extract a row from an image
  @param    image_in    Input image
  @param    pos         Position of the row (1 for the bottom one)
  @return   1 newly allocated cpl_vector or NULL on error
 
  Images can be of type CPL_TYPE_INT, CPL_TYPE_FLOAT or CPL_TYPE_DOUBLE.
  The returned vector must be deallocated using cpl_vector_delete().

  The bad pixels map is not taken into account in this function.
  
  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if pos is not valid
  - CPL_ERROR_INVALID_TYPE if the passed image type is not supported
 */
/*----------------------------------------------------------------------------*/
cpl_vector * cpl_vector_new_from_image_row(const cpl_image * image_in,
                                           cpl_size           pos)
{
    cpl_vector  *   out;
    double      *   out_data;

    /* Test entries */
    cpl_ensure(image_in, CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(pos>=1 && pos<=image_in->ny, CPL_ERROR_ILLEGAL_INPUT,NULL);
    
    /* Allocate output vector */
    out = cpl_vector_new(image_in->nx);
    out_data = cpl_vector_get_data(out);
    
    /* Switch on image type */
    switch (image_in->type) {
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_INT
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
        default:
            cpl_vector_delete(out);
            out = NULL;
            (void)cpl_error_set_(CPL_ERROR_INVALID_TYPE);
    }
    
    return out;
}
#undef CPL_OPERATION

#define CPL_OPERATION CPL_IMAGE_BASIC_EXTRACTCOL
/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Extract a column from an image
  @param    image_in    Input image
  @param    pos         Position of the column (1 for the left one)
  @return   1 newly allocated cpl_vector or NULL on error
 
  Images can be of type CPL_TYPE_INT, CPL_TYPE_FLOAT or CPL_TYPE_DOUBLE.
  The returned vector must be deallocated using cpl_vector_delete().

  The bad pixels map is not taken into account in this function.
  
  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if pos is not valid
  - CPL_ERROR_INVALID_TYPE if the passed image type is not supported
 */
/*----------------------------------------------------------------------------*/
cpl_vector * cpl_vector_new_from_image_column(const cpl_image * image_in,
                                              cpl_size          pos)
{
    cpl_vector * out;
    double     * out_data;

    /* Check entries */
    cpl_ensure(image_in != NULL,         CPL_ERROR_NULL_INPUT,    NULL);
    cpl_ensure(pos      >= 1,            CPL_ERROR_ILLEGAL_INPUT, NULL);
    cpl_ensure(pos      <= image_in->nx, CPL_ERROR_ILLEGAL_INPUT, NULL);
    
    /* Allocate output vector */
    out = cpl_vector_new(image_in->ny);
    out_data = cpl_vector_get_data(out);
    
    /* Switch on image type */
    switch (image_in->type) {
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_INT
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
        default:
            cpl_vector_delete(out);
            out = NULL;
            (void)cpl_error_set_(CPL_ERROR_INVALID_TYPE);
    }
    
    return out;
}
#undef CPL_OPERATION

#define CPL_OPERATION CPL_IMAGE_BASIC_ROTATE_INT_LOCAL
/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Rotate an image by a multiple of 90 degrees clockwise.
  @param    self    The image to rotate in place.
  @param    rot     The multiple: -1 is a rotation of 90 deg counterclockwise.
  @return   CPL_ERROR_NONE on success, otherwise the relevant #_cpl_error_code_
  @note The dimension of a rectangular image is changed.

  Images can be of type CPL_TYPE_INT, CPL_TYPE_FLOAT or CPL_TYPE_DOUBLE.

  The definition of the rotation relies on the FITS convention:
  The lower left corner of the image is at (1,1), x increasing from left to
  right, y increasing from bottom to top.

  For rotations of +90 or -90 degrees on rectangular non-1D-images,
  the pixel buffer is temporarily duplicated.

  rot may be any integer value, its modulo 4 determines the rotation:
  - -3 to turn 270 degrees counterclockwise.
  - -2 to turn 180 degrees counterclockwise.
  - -1 to turn  90 degrees counterclockwise.
  -  0 to not turn
  - +1 to turn  90 degrees clockwise (same as -3)
  - +2 to turn 180 degrees clockwise (same as -2).
  - +3 to turn 270 degrees clockwise (same as -1).

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INVALID_TYPE if the passed image type is not supported
*/
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_turn(cpl_image * self, int rot)
{

    cpl_ensure_code(self, CPL_ERROR_NULL_INPUT);

    rot %= 4;
    if (rot < 0) rot += 4;

    /* rot is 0, 1, 2 or 3. */

    /* Rotate the bad pixel map */
    if (rot != 0 && self->bpm != NULL) cpl_mask_turn(self->bpm, rot);

    /* Switch on the image type */
    switch (self->type) {
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_INT
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
        default:
            /* It is a bug in CPL to reach this point */
            return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
    }

    return CPL_ERROR_NONE;
}
#undef CPL_OPERATION

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Shift an image by integer offsets
  @param    self    The image to shift in place
  @param    dx The shift in X
  @param    dy The shift in Y
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE

  The new zones (in the result image) where no new value is computed are set 
  to 0 and flagged as bad pixels.
  The shift values have to be valid:
  -nx < dx < nx and -ny < dy < ny

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the requested shift is bigger than the
    image size
*/
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_shift(cpl_image * self,
                               cpl_size    dx,
                               cpl_size    dy)
{

    cpl_ensure_code(self != NULL, CPL_ERROR_NULL_INPUT);

    if (dx != 0 || dy != 0) {
        /* Rejected pixels are set to a zero-bit-pattern */
        if (cpl_tools_shift_window(self->pixels,
                                   cpl_type_get_sizeof(self->type),
                                   self->nx, self->ny, 0, dx, dy)) {
            return cpl_error_set_where_();
        }

        /* Shift the bad pixel map */
        if (self->bpm == NULL) self->bpm = cpl_mask_new(self->nx, self->ny);
        /* Cannot fail now */
        (void)cpl_mask_shift(self->bpm, dx, dy);
    }

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Copy one image into another
  @param    im1     the image in which im2 is inserted
  @param    im2     the inserted image
  @param    xpos    the x pixel position in im1 where the lower left pixel of 
                    im2 should go (from 1 to the x size of im1)
  @param    ypos    the y pixel position in im1 where the lower left pixel of 
                    im2 should go (from 1 to the y size of im1)
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error.
  @note The two pixel buffers may not overlap
 
  (xpos, ypos) must be a valid position in im1. If im2 is bigger than the place
  left in im1, the part that falls outside of im1 is simply ignored, an no 
  error is raised.
  The bad pixels are inherited from im2 in the concerned im1 zone.
  
  The two input images must be of the same type, namely one of
  CPL_TYPE_INT, CPL_TYPE_FLOAT, CPL_TYPE_DOUBLE.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_TYPE_MISMATCH if the input images are of different types
  - CPL_ERROR_INVALID_TYPE if the passed image type is not supported
  - CPL_ERROR_ACCESS_OUT_OF_RANGE if xpos or ypos are outside the
    specified range
*/
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_copy(cpl_image       *   im1,
                              const cpl_image *   im2,
                              cpl_size            xpos,
                              cpl_size            ypos)
{

    /* FIXME: Support overlapping pixel buffers ? */
    /* FIXME: Need to do pointer arithmetic */
    char          * pim1   = (char*)cpl_image_get_data(im1);
    const cpl_size  nx1    = cpl_image_get_size_x(im1); 
    const cpl_size  ny1    = cpl_image_get_size_y(im1); 
    const cpl_size  nx2    = cpl_image_get_size_x(im2); 
    const cpl_size  ny2    = cpl_image_get_size_y(im2); 
    /* Define the zone to modify in im1: xpos, ypos, urx, ury */
    const cpl_size  urx    = CX_MIN(nx1, nx2 + xpos - 1);
    const cpl_size  ury    = CX_MIN(ny1, ny2 + ypos - 1);
    const size_t    pixsz  = cpl_type_get_sizeof(cpl_image_get_type(im1));
    const size_t    linesz = (size_t)(urx - (xpos-1)) * pixsz;
    
    /* Check entries */
    cpl_ensure_code(im1       != NULL,      CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(im2       != NULL,      CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(xpos      >= 1,         CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_ensure_code(xpos      <= nx1,       CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_ensure_code(ypos      >= 1,         CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_ensure_code(ypos      <= ny1,       CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_ensure_code(im1->type == im2->type, CPL_ERROR_TYPE_MISMATCH);

    pim1 += (ypos - 1) * nx1 * pixsz;

    if (xpos == 1 && urx == nx1 && nx1 == nx2) {
        /* The zone consists of whole lines in both in1 and in2 */
        memcpy(pim1, im2->pixels, (ury - (ypos - 1)) * linesz);
    } else {
        /* FIXME: Need to do pointer arithmetic */
        const char   * pim2 = (const char*)im2->pixels;
        const size_t   sz1 = (size_t)nx1 * pixsz;
        const size_t   sz2 = (size_t)nx2 * pixsz;
        cpl_size       j;

        pim1 += (size_t)(xpos - 1) * pixsz;

        /* Loop on the zone */
        for (j = ypos - 1; j < ury; j++, pim1 += sz1, pim2 += sz2) {
            memcpy(pim1, pim2, linesz);
        }
    }

    /* Handle the bad pixels */
    if (im1->bpm != NULL || im2->bpm != NULL) {
        cpl_mask * bpm2;
        if (im1->bpm == NULL) im1->bpm = cpl_mask_new(im1->nx, im1->ny);
        bpm2 = im2->bpm ? im2->bpm : cpl_mask_new(im2->nx, im2->ny);

        cpl_mask_copy(im1->bpm, bpm2, xpos, ypos);

        if (bpm2 != im2->bpm) cpl_mask_delete(bpm2);
        /* FIXME: im1->bpm may be empty... */
    }

    return CPL_ERROR_NONE;
}

#define CPL_OPERATION CPL_IMAGE_BASIC_FLIP_LOCAL
/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Flip an image on a given mirror line.
  @param    im      the image to flip.  
  @param    angle   mirror line in polar coord. is theta = (PI/4) * angle 
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
 
  This function operates locally on the pixel buffer.

  angle can take one of the following values:
  - 0 (theta=0) to flip the image around the horizontal
  - 1 (theta=pi/4) to flip the image around y=x
  - 2 (theta=pi/2) to flip the image around the vertical
  - 3 (theta=3pi/4) to flip the image around y=-x

  Images can be of type CPL_TYPE_INT, CPL_TYPE_FLOAT or CPL_TYPE_DOUBLE.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the angle is different from the allowed values
  - CPL_ERROR_INVALID_TYPE if the passed image type is not supported
*/
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_flip(
        cpl_image   *   im,
        int             angle)
{
    /* Check entries */
    cpl_ensure_code(im != NULL, CPL_ERROR_NULL_INPUT);
    
    /* Switch on the image type */
    switch (im->type) {
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_INT
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
        default:
            return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
    }

    /* Flip the bad pixel map */
    if (im->bpm != NULL) cpl_mask_flip(im->bpm, angle);

    return CPL_ERROR_NONE;
}
#undef CPL_OPERATION


#define CPL_OPERATION CPL_IMAGE_BASIC_MOVE_PIXELS
/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Reorganize the pixels in an image
  @param    im          the image to reorganize
  @param    nb_cut      the number of cut in x and y
  @param    new_pos     array with the nb_cut^2 new positions
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE

  nb_cut^2 defines in how many tiles the images will be moved. Each tile will
  then be moved to an other place defined in new_pos. 1 will leave the image
  unchanged, 2 is used to move the quadrants, etc..
  new_pos contains nb_cut^2 values between 1 and nb_cut^2.
  The zones positions are counted from the lower left part of the image.
  It is not allowed to move two tiles to the same position (the relation
  between th new tiles positions and the initial position is bijective !).

  The image x and y sizes have to be multiples of nb_cut.

  @verbatim
  Example:

  16   17   18           6    5    4
  13   14   15           3    2    1

  10   11   12   ---->  12   11   10
   7    8    9           9    8    7

   4    5    6          18   17   16
   1    2    3          15   14   13

   image 3x6            cpl_image_move(image, 3, new_pos);
                        with new_pos = {9,8,7,6,5,4,3,2,1};
   @endverbatim

  The bad pixels are moved accordingly.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if nb_cut is not strictly positive or cannot
    divide one of the image sizes or if the new_pos array specifies to
    move two tiles to the same position.
  - CPL_ERROR_INVALID_TYPE if the passed image type is not supported
*/
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_move(cpl_image      * im,
                              cpl_size         nb_cut,
                              const cpl_size * new_pos)
{
    cpl_size        test_sum;
    cpl_size        tile_sz_x, tile_sz_y;
    cpl_size        tile_x, tile_y;
    cpl_size        npos, opos;
    cpl_size        i, j, k, l;

    /* Check entries */
    cpl_ensure_code(im,                   CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(new_pos,              CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(nb_cut > 0,           CPL_ERROR_ILLEGAL_INPUT);

    tile_sz_x = im->nx / nb_cut;
    tile_sz_y = im->ny / nb_cut;

    cpl_ensure_code(tile_sz_x * nb_cut == im->nx, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(tile_sz_y * nb_cut == im->ny, CPL_ERROR_ILLEGAL_INPUT);

    /* Test that new_pos takes all values between 1 and nb_cut*nb_cut */
    /* The test here is not strict, but should be sufficient */
    test_sum = 0;
    for (i=0; i<nb_cut*nb_cut; i++) test_sum += new_pos[i];
    cpl_ensure_code(test_sum==nb_cut*nb_cut*(nb_cut*nb_cut+1)/2,
            CPL_ERROR_ILLEGAL_INPUT);

    /* Switch on the image type */
    switch (im->type) {
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_INT
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_DOUBLE_COMPLEX
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_FLOAT_COMPLEX
#include "cpl_image_basic_body.h"
#undef CPL_CLASS
    default:
        return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
    }

    /* Handle the bad pixels */
    if (im->bpm != NULL) cpl_mask_move(im->bpm, nb_cut, new_pos);
    
    return CPL_ERROR_NONE;

}
#undef CPL_OPERATION

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Apply a gaussian fit on an image sub window
  @param    im      the input image
  @param    xpos    the x position of the center (1 for the first pixel)
  @param    ypos    the y position of the center (1 for the first pixel)
  @param    size    the window size in pixels, at least 4
  @param    norm    the norm of the gaussian or NULL
  @param    xcen    the x center of the gaussian or NULL
  @param    ycen    the y center of the gaussian or NULL
  @param    sig_x   the semi-major axis of the gaussian or NULL
  @param    sig_y   the semi-minor axis of the gaussian or NULL
  @param    fwhm_x  the FHHM in x or NULL
  @param    fwhm_y  the FHHM in y or NULL
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE 
  @see      cpl_fit_image_gaussian()
  @see      cpl_image_iqe()
  @deprecated If you need a 2D gaussian fit please 
  use the function @em cpl_fit_image_gaussian(). Please note that
  on CPL versions earlier than 5.1.0 this function was wrongly
  documented: the parameters @em sig_x and @em sig_y were defined
  as "the sigma in x (or y) of the gaussian", while actually they
  returned the semi-major and semi-minor axes of the gaussian distribution
  at 1-sigma. <b><i>PLEASE NOTE THAT IF YOU USED THIS FUNCTION FOR DETERMINING
  THE SPREAD OF A DISTRIBUTION ALONG THE X DIRECTION, THIS WAS VERY
  LIKELY OVERESTIMATED</i></b> (because @em sig_x was always assigned the
  semi-major axis of the distribution ignoring the rotation), 
  <b><i>WHILE THE SPREAD ALONG THE Y DIRECTION WOULD BE UNDERESTIMATED</i></b>.
  In addition to that, even with circular distributions this function 
  may lead to an underestimation of @em sig_x and @em sig_y (up to 25% 
  underestimation in the case of noiseless data with a box 4 times the 
  sigma, 1% underestimation in the case of noiseless data with a box 7 
  times the sigma). This latter problem is related to the function 
  @em cpl_image_iqe().

  This function is only acceptable for determining the position of a peak.
*/
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_fit_gaussian(
        const cpl_image *   im,
        cpl_size            xpos,
        cpl_size            ypos,
        cpl_size            size,
        double          *   norm,
        double          *   xcen,
        double          *   ycen,
        double          *   sig_x,
        double          *   sig_y,
        double          *   fwhm_x,
        double          *   fwhm_y) 
{
    const cpl_image * im_use;
    cpl_image       * im_cast = NULL;
    cpl_size          llx, lly, urx, ury;
    cpl_bivector    * stats;
    double          * pstats;

    /* Check entries */
    cpl_ensure_code(im != NULL,     CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(xpos >= 1,      CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(ypos >= 1,      CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(xpos <= im->nx, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(ypos <= im->ny, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(size >= 1,      CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(size < im->nx,  CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(size < im->ny,  CPL_ERROR_ILLEGAL_INPUT);

    /* Extraction zone */
    llx = xpos - size / 2;
    lly = ypos - size / 2;
    urx = xpos + size / 2;
    ury = ypos + size / 2;
    if (llx < 1) llx = 1;
    if (lly < 1) lly = 1;
    if (urx > im->nx) urx = im->nx;
    if (ury > im->ny) ury = im->ny;
    
    /* Convert the image to FLOAT, if needed */
    im_use = cpl_image_get_type(im) == CPL_TYPE_FLOAT ? im : 
        (im_cast = cpl_image_cast(im, CPL_TYPE_FLOAT));

    /* Call cpl_image_iqe */
    stats = cpl_image_iqe(im_use, llx, lly, urx, ury);
    cpl_image_delete(im_cast);

    if (stats == NULL) return cpl_error_set_where_();
    
    /* Write the results */
    pstats = cpl_bivector_get_x_data(stats);
    if (xcen) *xcen = pstats[0];
    if (ycen) *ycen = pstats[1];
    if (fwhm_x) *fwhm_x = pstats[2];
    if (fwhm_y) *fwhm_y = pstats[3];
    if (sig_x) *sig_x = pstats[2] / CPL_MATH_FWHM_SIG; 
    if (sig_y) *sig_y = pstats[3] / CPL_MATH_FWHM_SIG; 
    if (norm) *norm = pstats[5] * CPL_MATH_2PI *
        (pstats[2]*pstats[3]) / (CPL_MATH_FWHM_SIG*CPL_MATH_FWHM_SIG); 

    cpl_bivector_delete(stats);
    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Compute FWHM values in x and y for an object
  @param    in      the input image
  @param    xpos    the x position of the object (1 for the first pixel)
  @param    ypos    the y position of the object (1 for the first pixel)
  @param    fwhm_x  the computed FWHM in x or -1 on error
  @param    fwhm_y  the computed FWHM in y or -1 on error
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_

  This function uses a basic method: start from the center of the object
  and go away until the half maximum value is reached in x and y.

  For the FWHM in x (resp. y) to be computed, the image size in the x (resp. y)
  direction should be at least of 5 pixels.
  
  If for any reason, one of the FHWMs cannot be computed, its returned value 
  is -1.0, but an error is not necessarily raised. For example, if a 4 column
  image is passed, the fwhm_x would be -1.0, the fwhm_y would be correctly 
  computed, and no error would be raised.
  
  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_DATA_NOT_FOUND if (xpos, ypos) specifies a rejected pixel or a
      pixel with a non-positive value
  - CPL_ERROR_ACCESS_OUT_OF_RANGE if xpos or ypos is outside the image
    size range
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_get_fwhm(
        const cpl_image *   in,
        cpl_size            xpos,
        cpl_size            ypos,
        double          *   fwhm_x,
        double          *   fwhm_y)
{
    double              half_max;
    double              thres;
    const cpl_size      minimum_size = 5;
    int                 is_rejected;


    /* Check entries - and initialize *fwhm_{x,y} */
    if (fwhm_y != NULL) *fwhm_y = -1;
    cpl_ensure_code(fwhm_x,         CPL_ERROR_NULL_INPUT);
    *fwhm_x = -1;
    cpl_ensure_code(fwhm_y,         CPL_ERROR_NULL_INPUT);

    /* This call will check the validity of image, xpos and ypos */
    half_max = 0.5 * cpl_image_get(in, xpos, ypos, &is_rejected);

    cpl_ensure_code(is_rejected >= 0, cpl_error_get_code());
    cpl_ensure_code(!is_rejected, CPL_ERROR_DATA_NOT_FOUND);

    cpl_ensure_code(half_max > 0, CPL_ERROR_DATA_NOT_FOUND);
    
    /* FWHM in x */
    if (in->nx >= minimum_size) {
        cpl_errorstate pstate;

        /* Extract the vector centered on the maximum */
        cpl_vector * row = cpl_vector_new_from_image_row(in, ypos);

        /* If an error happened, update its location */
        cpl_ensure_code(row, cpl_error_get_code());

        pstate = cpl_errorstate_get();

        /* Find out threshold */
        thres = cpl_vector_get_noise(row, xpos);
        
        /* Compute the FWHM */
        if (cpl_errorstate_is_equal(pstate))
            *fwhm_x = cpl_vector_get_fwhm(row, xpos, half_max + thres * 0.5);

        cpl_vector_delete(row);

        /* Propagate the error, if any */
        cpl_ensure_code(cpl_errorstate_is_equal(pstate), cpl_error_get_code());

    }
     
    /* FWHM in y */
    if (in->ny >= minimum_size) {
        cpl_errorstate pstate;

        /* Extract the vector centered on the maximum */
        cpl_vector * col = cpl_vector_new_from_image_column(in, xpos);

        /* If an error happened, update its location */
        cpl_ensure_code(col, cpl_error_get_code());

        pstate = cpl_errorstate_get();

        /* Find out threshold */
        thres = cpl_vector_get_noise(col, ypos);
        
        /* Compute the FWHM */
        if (cpl_errorstate_is_equal(pstate))
            *fwhm_y = cpl_vector_get_fwhm(col, ypos, half_max + thres * 0.5);

        cpl_vector_delete(col);

        /* Propagate the error, if any */
        cpl_ensure_code(cpl_errorstate_is_equal(pstate), cpl_error_get_code());
    }
        
    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Fast Fourier Transform a square, power-of-two sized image
  @param    img_real   The image real part to be transformed in place
  @param    img_imag   The image imaginary part to be transformed in place
  @param    mode   The desired FFT options (combined with bitwise or)
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error

  The input images must be of double type.
 
  If the second passed image is NULL, the resulting imaginary part
  cannot be returned. This can be useful if the input is real and the
  output is known to also be real. But if the output has a significant
  imaginary part, you might want to pass a 0-valued image as the second
  parameter.
  
  Any rejected pixel is used as if it were a good pixel.

  The image must be square with a size that is a power of two.

  These are the supported FFT modes:
  CPL_FFT_DEFAULT: Default, forward FFT transform
  CPL_FFT_INVERSE: Inverse FFT transform
  CPL_FFT_UNNORMALIZED: Do not normalize (with N*N for N-by-N image) on inverse.
    Has no effect on forward transform.
  CPL_FFT_SWAP_HALVES: Swap the four quadrants of the result image.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if (one of) the input pointer(s) is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the image is not square or if the image size is
    not a power of 2.
  - CPL_ERROR_INVALID_TYPE if mode is 1, e.g. due to a logical or (||) of the
      allowed FFT options.
  - CPL_ERROR_UNSUPPORTED_MODE if mode is otherwise different from the allowed
       FFT options.
  - CPL_ERROR_INVALID_TYPE if the passed image type is not supported
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_fft(
        cpl_image   *   img_real, 
        cpl_image   *   img_imag, 
        unsigned        mode)
{
    unsigned                dim[2];
    double              *   imag_part;

    /* Check entries */
    cpl_ensure_code(img_real, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(mode != 1, CPL_ERROR_INVALID_TYPE);
    cpl_ensure_code(
            mode <= (CPL_FFT_INVERSE|CPL_FFT_UNNORMALIZED|CPL_FFT_SWAP_HALVES), 
            CPL_ERROR_UNSUPPORTED_MODE);
    cpl_ensure_code(img_real->nx==img_real->ny, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(img_real->type==CPL_TYPE_DOUBLE,CPL_ERROR_INVALID_TYPE);
    cpl_ensure_code(cpl_tools_is_power_of_2(img_real->nx)>=0,
                    CPL_ERROR_ILLEGAL_INPUT);
    if (img_imag != NULL) {
        cpl_ensure_code(img_real->nx==img_imag->nx,CPL_ERROR_ILLEGAL_INPUT);
        cpl_ensure_code(img_real->ny==img_imag->ny,CPL_ERROR_ILLEGAL_INPUT);
        cpl_ensure_code(img_imag->type==CPL_TYPE_DOUBLE,
                CPL_ERROR_INVALID_TYPE);
    }

    /* Initialize */
    dim[0] = dim[1] = (unsigned)img_real->nx;

    cpl_ensure_code((cpl_size)dim[0] == img_real->nx, CPL_ERROR_ILLEGAL_INPUT);

    if (img_imag == NULL) {
        /* Create the imaginary part and set it to 0  */
        imag_part = cpl_calloc(dim[0]*dim[1], sizeof(double));
    } else {
        /* Put the input imaginary part in a local object */
        imag_part = img_imag->pixels;
    }

    /* APPLY THE FFT HERE */
    cpl_ensure_code(!cpl_fft(img_real->pixels, imag_part, dim, 2,
                (mode & CPL_FFT_INVERSE) ? -1 : 1),cpl_error_get_code());

    /* Free the imaginary part result in the input image */
    if (img_imag == NULL) {
        cpl_free(imag_part);
    }

    /* Normalize on the inverse transform  */
    if (!(mode & CPL_FFT_UNNORMALIZED) && (mode & CPL_FFT_INVERSE)) {
        cpl_ensure_code(!cpl_image_divide_scalar(img_real, dim[0]*dim[1]),
                        cpl_error_get_code());
        if (img_imag != NULL) {
            cpl_ensure_code(!cpl_image_divide_scalar(img_imag, dim[0]*dim[1]),
                            cpl_error_get_code());
        }
    }

    /* Swap halves in both dimensions */
    if (mode & CPL_FFT_SWAP_HALVES) {
        const cpl_size new_pos[] = {4, 3, 2, 1};
        cpl_ensure_code(!cpl_image_move(img_real, 2, new_pos),
                        cpl_error_get_code());
        if (img_imag != NULL) {
            cpl_ensure_code(!cpl_image_move(img_imag, 2, new_pos),
                            cpl_error_get_code());
        }
    }

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
   @internal
   @ingroup cpl_image
   @brief    The bit-wise or of the input masks onto the output mask
   @param    self     Pre-allocated image to hold the result
   @param    first    First operand, or NULL for an in-place operation
   @param    second   Second operand
   @return   void
   @note Error checking assumed to have been performed by the caller
*/
/*----------------------------------------------------------------------------*/
void cpl_image_or_mask(cpl_image * self,
                       const cpl_image * first,
                       const cpl_image * second)
{

    const size_t nxy = (size_t)self->nx * (size_t)self->ny;

    if (self->bpm == NULL) {
        /* Create the bad pixel map, if an input one is non-NULL */
        if (first != NULL && first->bpm != NULL && second->bpm != NULL) {
            self->bpm = cpl_mask_wrap(self->nx, self->ny, cpl_malloc(nxy));
            cpl_mask_or_(cpl_mask_get_data(self->bpm),
                         cpl_mask_get_data_const(first->bpm),
                         cpl_mask_get_data_const(second->bpm), nxy);
        } else if (second->bpm != NULL) {
            self->bpm = cpl_mask_duplicate(second->bpm);
        } else if (first != NULL && first->bpm != NULL) {
            self->bpm = cpl_mask_duplicate(first->bpm);
        }
    } else {
        /* The self bpm is non-NULL. If first is NULL, then the operation is
           in-place, so self is an input parameter */
        if (first != NULL && first->bpm != NULL && second->bpm != NULL) {
            cpl_mask_or_(cpl_mask_get_data(self->bpm),
                         cpl_mask_get_data_const(first->bpm),
                         cpl_mask_get_data_const(second->bpm), nxy);
        } else if (first != NULL && first->bpm != NULL) {
            assert(second->bpm == NULL);
            (void)memcpy(cpl_mask_get_data(self->bpm),
                         cpl_mask_get_data_const(first->bpm), nxy);
        } else if (second->bpm != NULL) {
            if (first == NULL) {
                /* Self is an input parameter */
                cpl_mask_or(self->bpm, second->bpm);
            } else {
                /* First is non-NULL, but happens to have a NULL bpm */
                assert(first->bpm == NULL);
                (void)memcpy(cpl_mask_get_data(self->bpm),
                             cpl_mask_get_data_const(second->bpm), nxy);
            }
        } else if (first != NULL) {
            /* First is non-NULL, but happens to have a NULL bpm,
               so the result is an empty bpm */
            (void)memset(cpl_mask_get_data(self->bpm), 0, nxy);
        }
    }
}


/*----------------------------------------------------------------------------*/
/**
   @internal
   @ingroup cpl_image
   @brief    The bit-wise or of the input mask(s) onto the output mask
   @param    self     Pre-allocated image to hold the result
   @param    first    First operand, or NULL for an in-place operation
   @return   void
   @note Error checking assumed to have been performed by the caller
   @see cpl_image_or_mask
*/
/*----------------------------------------------------------------------------*/
void cpl_image_or_mask_unary(cpl_image * self,
                              const cpl_image * first)
{

    const size_t nxy = (size_t)self->nx * (size_t)self->ny;

    if (self->bpm == NULL) {
        /* Create the bad pixel map, if an input one is non-NULL */
        if (first != NULL && first->bpm != NULL) {
            self->bpm = cpl_mask_duplicate(first->bpm);
        }
    } else {
        /* The self bpm is non-NULL. If first is NULL, then the operation is
           in-place, so self is an input parameter */
        if (first != NULL && first->bpm != NULL) {
            (void)memcpy(cpl_mask_get_data(self->bpm),
                         cpl_mask_get_data_const(first->bpm), nxy);
        } else if (first != NULL) {
            /* First is non-NULL, but happens to have a NULL bpm,
               so the result is an empty bpm */
            (void)memset(cpl_mask_get_data(self->bpm), 0, nxy);
        }
    }
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief    N-dimensional FFT.
  @param    real        N-dimensional data set stored in 1d (real part).
  @param    imag        N-dimensional data set stored in 1d (imaginary part).
  @param    nn          Dimensions of the set.
  @param    ndim        How many dimensions this set has.
  @param    isign       Transform direction.
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE

  This routine is a public domain FFT. See extract of Usenet article below. 
  Found on http://www.tu-chemnitz.de/~arndt/joerg.html

  @verbatim
  From: alee@tybalt.caltech.edu (Andrew Lee)
  Newsgroups: comp.sources.misc
  Subject: N-dimensional, Radix 2 FFT Routine
  Date: 17 Jul 87 22:26:29 GMT
  Approved: allbery@ncoast.UUCP
  X-Archive: comp.sources.misc/8707/48

  [..]
  Now for the usage (finally):
  real[] and imag[] are the array of complex numbers to be transformed,
  nn[] is the array giving the dimensions (I mean size) of the array,
  ndim is the number of dimensions of the array, and
  isign is +1 for a forward transform, and -1 for an inverse transform.

  real[], imag[] and nn[] are stored in the "natural" order for C:
  nn[0] gives the number of elements along the leftmost index,
  nn[ndim - 1] gives the number of elements along the rightmost index.

  Additional notes: The routine does NO NORMALIZATION, so if you do a forward, 
  and then an inverse transform on an array, the result will be identical to 
  the original array MULTIPLIED BY THE NUMBER OF ELEMENTS IN THE ARRAY. Also, 
  of course, the dimensions of real[] and imag[] must all be powers of 2.
  @endverbatim

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_fft(
        double          *   real,
        double          *   imag,
        const unsigned  *   nn,
        int                 ndim,
        int                 isign)
{
    int                 idim;
    unsigned            i1, i2rev, i3rev, ibit;
    unsigned            ip2, ifp1, ifp2, k2, n;
    unsigned            nprev = 1, ntot = 1;
    register unsigned   i2, i3;
    double              theta;
    double              w_r, w_i, wp_r, wp_i;
    double              wtemp;
    double              temp_r, temp_i, wt_r, wt_i;
    double              t1, t2;

    /* Check entries */
    cpl_ensure_code(real, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(imag, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(nn, CPL_ERROR_NULL_INPUT);
    for (idim = 0; idim < ndim; ++idim) 
        cpl_ensure_code(cpl_tools_is_power_of_2(nn[idim])>=0,
                        CPL_ERROR_ILLEGAL_INPUT);

    /* Compute total number of complex values  */
    for (idim = 0; idim < ndim; ++idim) ntot *= nn[idim];

    for (idim = ndim - 1; idim >= 0; --idim) {
        n = nn[idim];

        ip2 = nprev * n;        /*  Unit step for next dimension */
        i2rev = 0;              /*  Bit reversed i2 */

        /* This is the bit reversal section of the routine */
        /* Loop over current dimension */
        for (i2 = 0; i2 < ip2; i2 += nprev) {
            if (i2 < i2rev)
                /*      Loop over lower dimensions      */
                for (i1 = i2; i1 < i2 + nprev; ++i1)
                    /*      Loop over higher dimensions  */
                    for (i3 = i1; i3 < ntot; i3 += ip2) {
                        i3rev = i3 + i2rev - i2;
                        temp_r = real[i3];
                        temp_i = imag[i3];
                        real[i3] = real[i3rev];
                        imag[i3] = imag[i3rev];
                        real[i3rev] = temp_r;
                        imag[i3rev] = temp_i;
                    }

            ibit = ip2;
            /* Increment from high end of i2rev to low */
            do {
                ibit >>= 1;
                i2rev ^= ibit;
            } while (ibit >= nprev && !(ibit & i2rev));
        }

        /* Here begins the Danielson-Lanczos section of the routine */
        /* Loop over step sizes    */
        for (ifp1 = nprev; ifp1 < ip2; ifp1 <<= 1) {
            ifp2 = ifp1 << 1;
            /*  Initialize for the trig. recurrence */
            theta = isign * CPL_MATH_2PI / (ifp2 / nprev);
            wp_r = sin(0.5 * theta);
            wp_r *= -2.0 * wp_r;
            wp_i = sin(theta);
            w_r = 1.0;
            w_i = 0.0;

            /* Loop by unit step in current dimension  */
            for (i3 = 0; i3 < ifp1; i3 += nprev) {
                /* Loop over lower dimensions      */
                for (i1 = i3; i1 < i3 + nprev; ++i1)
                    /* Loop over higher dimensions */
                    for (i2 = i1; i2 < ntot; i2 += ifp2) {
                        /* Danielson-Lanczos formula */
                        k2 = i2 + ifp1;
                        wt_r = real[k2];
                        wt_i = imag[k2];

                        /* Complex multiply using 3 real multiplies. */
                        real[k2] = real[i2] - (temp_r =
                            (t1 = w_r * wt_r) - (t2 = w_i * wt_i));
                        imag[k2] = imag[i2] - (temp_i =
                            (w_r + w_i) * (wt_r + wt_i) - t1 - t2);
                        real[i2] += temp_r;
                        imag[i2] += temp_i;
                    }
                /* Trigonometric recurrence */
                wtemp = w_r;
                /* Complex multiply using 3 real multiplies. */
                w_r += (t1 = w_r * wp_r) - (t2 = w_i * wp_i);
                w_i += (wtemp + w_i) * (wp_r + wp_i) - t1 - t2;
            }
        }
        nprev *= n;
    }
    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief    Compute the noise around a peak in a vector
  @param    vec     the input cpl_vector
  @param    pos     the peak position (from 1 to the vector size)
  @return   the noise value.

  The passed cpl_vector object must have at least two elements.
  
  In case of error, the #_cpl_error_code_ code is set, and the returned double
  is undefined.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT
 */
/*----------------------------------------------------------------------------*/
static double cpl_vector_get_noise(
        const cpl_vector    *   vec,
        cpl_size                pos)
{
    cpl_size            nelem;
    cpl_vector       *  smooth_vec;
    const double     *  vector_data;
    double              noise_left, noise_right;
    cpl_errorstate      prestate = cpl_errorstate_get();
    cpl_size            i;

    /* Check entries */
    cpl_ensure(vec,          CPL_ERROR_NULL_INPUT, -1.0);
    nelem = cpl_vector_get_size(vec);
    cpl_ensure(pos >= 1,     CPL_ERROR_ILLEGAL_INPUT, -1.0);
    cpl_ensure(pos <= nelem, CPL_ERROR_ILLEGAL_INPUT, -1.0);
    cpl_ensure(nelem > 1,    CPL_ERROR_ILLEGAL_INPUT, -1.0);

    /* Smooth out the array to be less sensitive to noise */
    smooth_vec = cpl_vector_filter_lowpass_create(vec, CPL_LOWPASS_LINEAR, 1);
    if (!cpl_errorstate_is_equal(prestate)) {
        /* Recover and use the unsmoothed vector */
        cpl_errorstate_set(prestate);
        cpl_vector_delete(smooth_vec);
        smooth_vec = NULL;
    }
    vector_data = cpl_vector_get_data_const(smooth_vec ? smooth_vec : vec);

    if (smooth_vec != NULL) {
        /* The smoothing (half-size is 1) may have moved the maximum 1 pixel */
        if (pos < nelem && vector_data[pos-1] < vector_data[pos]) {
            pos++;
        } else if (pos > 1 && vector_data[pos-1] < vector_data[pos-2]) {
            pos--;
        }
    }

    /* Find noise level on the left side of the peak. */
    i = pos - 1;
    while (i > 0) {
        if (vector_data[i] > vector_data[i-1]) i--;
        else break;
    }
    noise_left = vector_data[i];

    /* Find noise level on the right side of the peak */
    i = pos - 1;
    while (i < nelem-1) {
        if (vector_data[i] > vector_data[i+1]) i++;
        else break;
    }
    noise_right = vector_data[i];

    cpl_vector_delete(smooth_vec);

    return 0.5 * (noise_left + noise_right);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief    Compute the FWHM of an object in a cpl_vector
  @param    vec         the input cpl_vector
  @param    pos         the object position (from 1 to the vector size)
  @param    half_max    the half maximum value
  @return   the FWHM or a negative value on error
  @note The return value may be -1 with no error condition

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if pos is less than 1 or greater than the vec-length
 */
/*----------------------------------------------------------------------------*/
static double cpl_vector_get_fwhm(
        const cpl_vector    *   vec,
        cpl_size                pos,
        double                  half_max)
{
    const double * vec_data;
    cpl_size       nelem;
    double         x_left, x_right;
    double         y_1, y_2;
    cpl_size       i;

    /* Check entries */
    cpl_ensure(vec,          CPL_ERROR_NULL_INPUT, -1.0);
    nelem = cpl_vector_get_size(vec);
    cpl_ensure(pos >= 1,     CPL_ERROR_ILLEGAL_INPUT, -1.0);
    cpl_ensure(pos <= nelem, CPL_ERROR_ILLEGAL_INPUT, -1.0);

    vec_data = cpl_vector_get_data_const(vec);

    /* Object may be too noisy - or strange in some other way */
    if (vec_data[pos - 1] <= half_max) return -1.0;

    /* Find first pair of values, y(i) <= half_max < y(i+1)
         on the left of the maximum */
    i = pos - 1;

    while ((vec_data[i] > half_max) && (i > 0)) i--;
    if (vec_data[i] > half_max) return -1.0;  /* y_1 could not be found */

    y_1 = vec_data[i];
    y_2 = vec_data[i+1];

    /* assert ( y_1 <= half_max && half_max < y_2 ); */

    /* Assume linearity between y_1 and y_2 */
    x_left = (double)i + (half_max-y_1) / (y_2-y_1);

    /* assert( x_left >= i ); */

    /* Find first pair of values, y(i-1) > half_max >= y(i)
         on the right of the maximum */
    i = pos - 1;
 
    while ((vec_data[i] > half_max) && (i < nelem-1)) i++;
    if (vec_data[i] > half_max) return -1.0;   /* y_2 could not be found */

    y_1 = vec_data[i-1];
    y_2 = vec_data[i];

    /* assert( y_1 > half_max && half_max >= y_2 ); */

    /* Assume linearity between y_1 and y_2 */
    x_right = (double)i + (half_max-y_2) / (y_2-y_1);

    /* assert( x_right < i ); */

    if (x_right < x_left || x_right - x_left > FLT_MAX) return -1;

    return x_right - x_left;
}


#if (defined __SSE3__ || defined __SSE2__)


/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief    multiply two vectors of two complex floats
  @param    a     first operand
  @param    b     second operand
  @param    i     offset
  @return   result of the multiplication
  @see fcompl_mult_sse_aligned()

  Function not inlineable to work around code generation issue of gcc.
  The pointer additions must be hidden in this function or gcc will pointlessly
  add them to the inner loop of the complex multiplication even though they
  are only required in the unlikely branch.
  Tested with gcc <= 4.7. Up to 30% speed improvement on some machines.
 */
/*----------------------------------------------------------------------------*/
static void CPL_ATTR_NOINLINE
fcompl_mult_scalar2(float complex * a, const float complex * b, const size_t i)
{
    a[i] = a[i] * b[i];
    a[i + 1] = a[i + 1] * b[i + 1];
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief    check result for NaN and recompute correctly if so
  @return   true if nan was fixed and stored, false if no fixup required
            if false no store was done
  @see fcompl_mult_sse_aligned()
 */
/*----------------------------------------------------------------------------*/
static inline cpl_boolean
fcompl_fix_nan(__m128 res, float complex * a,
               const float complex * b,
               const size_t i)
{
#ifndef __FAST_MATH__
        /* check for NaN, redo all in libc if found */
        __m128 n = _mm_cmpneq_ps(res, res);
        if (CPL_UNLIKELY(_mm_movemask_ps(n) != 0)) {
            fcompl_mult_scalar2(a, b, i);
            return CPL_TRUE;
        }
        else
#endif
            return CPL_FALSE;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief    vertical multiply two vectors of each two complex floats
  @param    va     first operand (real, imag, real, imag)
  @param    vb     second operand (real, imag, real, imag)
  @return   result of the multiplication

  does not handle NaN's correctly
 */
/*----------------------------------------------------------------------------*/
static inline __m128
fcompl_mult_sse_fast(__m128 va, __m128 vb)
{
    /* optimized to SSE3 _mm_move[hl]dup_ps by gcc */
    __m128 reala = _mm_shuffle_ps(va, va, _MM_SHUFFLE(2, 2, 0, 0)); /* x x */
    __m128 imaga = _mm_shuffle_ps(va, va, _MM_SHUFFLE(3, 3, 1, 1)); /* w w */
    __m128 t1 = _mm_mul_ps(reala, vb); /* x*y x*z */
    __m128 sb = _mm_shuffle_ps(vb, vb, _MM_SHUFFLE(2, 3, 0, 1)); /* z y */
    __m128 t2 = _mm_mul_ps(imaga, sb); /* w*z w*y */
    return CPL_MM_ADDSUB_PS(t1, t2); /* x*y-w*z x*z+w*y*/
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief    complex multiplication loop, aligned
  @param    im1     first operand, must be 16 byte aligned
  @param    im2     second operand, must be 16 byte aligned
  @param    Nt      number of pixels
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
 */
/*----------------------------------------------------------------------------*/
static cpl_error_code
fcompl_mult_sse_aligned(float complex * a,
                        const float complex * b,
                        const size_t Nt)
{
    const size_t n = (Nt % 2) == 1 ? Nt - 1 : Nt;

    /* no overflow hint for the compiler */
    cpl_ensure_code(n < SIZE_MAX-1, CPL_ERROR_ACCESS_OUT_OF_RANGE);
    for (size_t i = 0; i < n; i+=2) {
        __m128 va = _mm_load_ps((const float *)&a[i]); /* x w */
        __m128 vb = _mm_load_ps((const float *)&b[i]); /* y z */
        __m128 res = fcompl_mult_sse_fast(va, vb);

        if (CPL_LIKELY(fcompl_fix_nan(res, a, b, i) == CPL_FALSE))
            _mm_store_ps((float *)&a[i], res);
    }

    if ((Nt) % 2 == 1)
        a[Nt - 1] = a[Nt - 1] * b[Nt - 1];

    return CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief    complex multiplication loop, unaligned
  @param    im1     first operand
  @param    im2     second operand.
  @param    Nt      number of pixels
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
 */
/*----------------------------------------------------------------------------*/
static cpl_error_code
fcompl_mult_sse_unaligned(float complex * a,
                          const float complex * b,
                          const size_t Nt)
{
    const size_t n = (Nt % 2) == 1 ? Nt - 1 : Nt;

    /* no overflow hint for the compiler */
    cpl_ensure_code(n < SIZE_MAX-1, CPL_ERROR_ACCESS_OUT_OF_RANGE);
    for (size_t i = 0; i < n; i+=2) {
        __m128 va = _mm_loadu_ps((const float *)&a[i]); /* x w */
        __m128 vb = _mm_loadu_ps((const float *)&b[i]); /* y z */
        __m128 res = fcompl_mult_sse_fast(va, vb);

        if (CPL_LIKELY(fcompl_fix_nan(res, a, b, i) == CPL_FALSE))
            _mm_storeu_ps((float *)&a[i], res);
    }

    if ((Nt) % 2 == 1)
        a[Nt - 1] = a[Nt - 1] * b[Nt - 1];

    return CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief    Multiply two float complex images using sse2 or 3
  @param    im1     first operand.
  @param    im2     second operand.
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
  @see      cpl_image_multiply()
  @note No error checking performed in this static function
 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_image_multiply_fcomplex_sse_(cpl_image       * im1,
                                                       const cpl_image * im2)
{
    const size_t Nt = im1->nx * im1->ny;
    float complex * a = im1->pixels;
    const float complex * b = im2->pixels;
    cpl_error_code err = CPL_ERROR_NONE;

    /* FIXME: cases for a xor b unaligned? */
    if (((intptr_t)a % 16) == 0 && ((intptr_t)b % 16) == 0)
        err = fcompl_mult_sse_aligned(a, b, Nt);
    else if (((intptr_t)a % 16) == 8 && ((intptr_t)b % 16) == 8) {
        a[0] = a[0] * b[0];
        err = fcompl_mult_sse_aligned(a + 1, b + 1, Nt - 1);
    }
    else
        err = fcompl_mult_sse_unaligned(a, b, Nt);


    /* Handle bad pixels map */
    if (im2->bpm != NULL) {
        if (im1->bpm == NULL) {
            im1->bpm = cpl_mask_duplicate(im2->bpm);
        } else {
            cpl_mask_or(im1->bpm, im2->bpm);
        }
    }

    return err;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief    multiply two vectors of two complex double
  @param    a     first operand
  @param    b     second operand
  @param    i     offset
  @return   result of the multiplication
  @see fcompl_mult_scalar2()

 */
/*----------------------------------------------------------------------------*/
static void CPL_ATTR_NOINLINE
dcompl_mult_scalar(double complex * a, const double complex * b, const size_t i)
{
    a[i] = a[i] * b[i];
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief    check result for NaN and recompute correctly if so
  @return   true if nan was fixed and stored, false if no fixup required
            if false no store was done
  @see dcompl_mult_sse_aligned()
 */
/*----------------------------------------------------------------------------*/
static inline cpl_boolean
dcompl_fix_nan(__m128d res, double complex * a,
               const double complex * b,
               const size_t i)
{
#ifndef __FAST_MATH__
        /* check for NaN, redo all in libc if found */
        __m128d n = _mm_cmpneq_pd(res, res);
        if (CPL_UNLIKELY(_mm_movemask_pd(n) != 0)) {
            dcompl_mult_scalar(a, b, i);
            return CPL_TRUE;
        }
        else
#endif
            return CPL_FALSE;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief    vertical multiply two vectors of each two complex double
  @param    va     first operand (real, imag, real, imag)
  @param    vb     second operand (real, imag, real, imag)
  @return   result of the multiplication

  does not handle NaN's correctly
 */
/*----------------------------------------------------------------------------*/
static inline __m128d
dcompl_mult_sse_fast(__m128d va, __m128d vb)
{
    /* optimized to SSE3 _mm_movedup_pd by gcc */
    __m128d rb = _mm_unpacklo_pd(vb, vb); /* y, y */
    __m128d ib = _mm_unpackhi_pd(vb, vb); /* w, w*/
    __m128d t1 = _mm_mul_pd(rb, va); /* y * x, y * z */
    __m128d t2 = _mm_mul_pd(ib, va); /* w * x, w * z*/
    __m128d sb = _mm_shuffle_pd(t2, t2, _MM_SHUFFLE2(0, 1)); /* w * z, w * x */
    return CPL_MM_ADDSUB_PD(t1, sb); /* x * y - y * w, z*y + x * w */
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief    complex multiplication loop, aligned
  @param    im1     first operand, must be 16 byte aligned
  @param    im2     second operand, must be 16 byte aligned
  @param    Nt      number of pixels
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
 */
/*----------------------------------------------------------------------------*/
static cpl_error_code CPL_ATTR_NOINLINE
dcompl_mult_sse_aligned(double complex * a,
                        const double complex * b,
                        const size_t n)
{
    /* no overflow hint for the compiler */
    cpl_ensure_code(n < SIZE_MAX, CPL_ERROR_ACCESS_OUT_OF_RANGE);
    for (size_t i = 0; i < n; i++) {
        __m128d va = _mm_load_pd((const double *)&a[i]); /* x w */
        __m128d vb = _mm_load_pd((const double *)&b[i]); /* y z */
        __m128d res = dcompl_mult_sse_fast(va, vb);

        if (CPL_LIKELY(dcompl_fix_nan(res, a, b, i) == CPL_FALSE))
            _mm_store_pd((double *)&a[i], res);
    }

    return CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief    complex multiplication loop, unaligned
  @param    im1     first operand
  @param    im2     second operand.
  @param    Nt      number of pixels
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
 */
/*----------------------------------------------------------------------------*/
static cpl_error_code
dcompl_mult_sse_unaligned(double complex * a,
                          const double complex * b,
                          const size_t n)
{
    /* no overflow hint for the compiler */
    cpl_ensure_code(n < SIZE_MAX, CPL_ERROR_ACCESS_OUT_OF_RANGE);
    for (size_t i = 0; i < n; i++) {
        __m128d va = _mm_loadu_pd((const double *)&a[i]); /* x w */
        __m128d vb = _mm_loadu_pd((const double *)&b[i]); /* y z */
        __m128d res = dcompl_mult_sse_fast(va, vb);

        if (CPL_LIKELY(dcompl_fix_nan(res, a, b, i) == CPL_FALSE))
            _mm_storeu_pd((double *)&a[i], res);
    }

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief    Multiply two double complex images using sse2 or 3
  @param    im1     first operand.
  @param    im2     second operand.
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
  @see      cpl_image_multiply()
  @note No error checking performed in this static function
 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_image_multiply_dcomplex_sse_(cpl_image       * im1,
                                                       const cpl_image * im2)
{
    const size_t Nt = im1->nx * im1->ny;
    double complex * a = im1->pixels;
    const double complex * b = im2->pixels;
    cpl_error_code err = CPL_ERROR_NONE;

    /* FIXME: cases for a xor b unaligned? */
    if (((intptr_t)a % 16) == 0 && ((intptr_t)b % 16) == 0)
        err = dcompl_mult_sse_aligned(a, b, Nt);
    else
        err = dcompl_mult_sse_unaligned(a, b, Nt);


    /* Handle bad pixels map */
    if (im2->bpm != NULL) {
        if (im1->bpm == NULL) {
            im1->bpm = cpl_mask_duplicate(im2->bpm);
        } else {
            cpl_mask_or(im1->bpm, im2->bpm);
        }
    }

    return err;
}
#endif
