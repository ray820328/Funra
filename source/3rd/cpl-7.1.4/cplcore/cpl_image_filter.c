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

#include "cpl_memory.h"
#include "cpl_error_impl.h"
#include "cpl_image_filter_impl.h"
#include "cpl_image_bpm.h"
#include "cpl_mask.h"
#include "cpl_tools.h"

#include "cpl_image_defs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include <assert.h>

#include <float.h>
#include <limits.h>

/*-----------------------------------------------------------------------------
                                   Defines
 -----------------------------------------------------------------------------*/

/* These macros are needed for support of the different pixel types */

#define CONCAT(a,b) a ## _ ## b
#define CONCAT2X(a,b) CONCAT(a,b)

#define ADDTYPE(a) CONCAT2X(a, IN_TYPE)
#define ADDTYPE_TWO(a) CONCAT2X(a, CONCAT2X(OUT_TYPE, IN_TYPE))

/*-----------------------------------------------------------------------------
                            Static Function Prototypes
 -----------------------------------------------------------------------------*/

/* First includes involve no casting */
#define IN_EQ_OUT

#define IN_TYPE double
#define PIX_TYPE double
/* FIXME: Use DBL_MIN */
#define PIX_MIN (-DBL_MAX)
#define PIX_MAX DBL_MAX
#include "cpl_filter_median.c"

#define IN_TYPE float
#define PIX_TYPE float
/* FIXME: Use FLT_MIN */
#define PIX_MIN (-FLT_MAX)
#define PIX_MAX FLT_MAX
#include "cpl_filter_median.c"

#define IN_TYPE int
#define PIX_TYPE int
/* FIXME: Use INT_MIN */
#define PIX_MIN (-INT_MAX)
#define PIX_MAX INT_MAX
#include "cpl_filter_median.c"

#define IN_TYPE double
#define OUT_TYPE double
#define ACCU_TYPE double
#define ACCU_FLOATTYPE double
#include "cpl_image_filter_body.h"

#define IN_TYPE float
#define OUT_TYPE float
#define ACCU_TYPE float
#define ACCU_FLOATTYPE float
#include "cpl_image_filter_body.h"

#define IN_TYPE int
#define OUT_TYPE int
#define ACCU_TYPE int 
#define ACCU_TYPE_IS_INT
#define ACCU_FLOATTYPE double
#include "cpl_image_filter_body.h"

#undef IN_EQ_OUT

#define IN_TYPE double
#define OUT_TYPE float
#define ACCU_TYPE double
#define ACCU_FLOATTYPE double
#include "cpl_image_filter_body.h"

#define IN_TYPE double
#define OUT_TYPE int
#define ACCU_TYPE double
#define ACCU_FLOATTYPE double
#include "cpl_image_filter_body.h"

#define IN_TYPE float
#define OUT_TYPE double
#define ACCU_TYPE double
#define ACCU_FLOATTYPE double
#include "cpl_image_filter_body.h"

#define IN_TYPE float
#define OUT_TYPE int
#define ACCU_TYPE float
#define ACCU_FLOATTYPE float
#include "cpl_image_filter_body.h"


#define IN_TYPE int
#define OUT_TYPE double
#define ACCU_TYPE double
#define ACCU_FLOATTYPE double
#include "cpl_image_filter_body.h"

#define IN_TYPE int
#define OUT_TYPE float
#define ACCU_TYPE float
#define ACCU_FLOATTYPE float
#include "cpl_image_filter_body.h"

static cpl_mask * cpl_mask_new_from_matrix(const cpl_matrix *, double, double);

/*-----------------------------------------------------------------------------
                            Function codes
 -----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief  Filter an image using a binary kernel
  @param  self   Pre-allocated image to hold the filtered result
  @param  other  Image to filter
  @param  kernel Pixels to use, if set to CPL_BINARY_1
  @param  filter CPL_FILTER_MEDIAN, CPL_FILTER_AVERAGE and more, see below
  @param  border CPL_BORDER_FILTER and more, see below
  @return CPL_ERROR_NONE or the relevant CPL error code

  The kernel must have an odd number of rows and an odd number of columns.

  The two images must have equal dimensions, except for the border mode
  CPL_BORDER_CROP, where the input image must have 2 * hx columns more and
  2 * hy rows more than the output image, where the kernel has size
  (1 + 2 * hx, 1 + 2 * hy).

  In standard deviation filtering the kernel must have at least two elements
  set to CPL_BINARY_1, for others at least one element must be set to
  CPL_BINARY_1.

  Supported pixel types are: CPL_TYPE_INT, CPL_TYPE_FLOAT and CPL_TYPE_DOUBLE.

  In median filtering the two images must have the same pixel type.

  In standard deviation filtering a filtered pixel must be computed from at
  least two input pixels, for other filters at least one input pixel must be
  available. Output pixels where this is not the case are set to zero and
  flagged as rejected.

  In-place filtering is not supported.

  Supported modes:

  CPL_FILTER_MEDIAN:
         CPL_BORDER_FILTER, CPL_BORDER_COPY, CPL_BORDER_NOP, CPL_BORDER_CROP.

  CPL_FILTER_AVERAGE:
         CPL_BORDER_FILTER

  CPL_FILTER_AVERAGE_FAST:
         CPL_BORDER_FILTER

  CPL_FILTER_STDEV:
         CPL_BORDER_FILTER

  CPL_FILTER_STDEV_FAST:
         CPL_BORDER_FILTER


   @par Example:
   @code
     cpl_image_filter_mask(filtered, raw, kernel, CPL_FILTER_MEDIAN,
                           CPL_BORDER_FILTER);
   @endcode

  To shift an image 1 pixel up and 1 pixel right with the CPL_FILTER_MEDIAN
  filter and a 3 by 3 kernel, one should set to CPL_BINARY_1 the bottom
  leftmost kernel element - at row 3, column 1, i.e.
  @code
     cpl_mask * kernel = cpl_mask_new(3, 3);
     cpl_mask_set(kernel, 1, 1);
  @endcode

  The kernel required to do a 5 x 5 median filtering is created like this:
  @code
     cpl_mask * kernel = cpl_mask_new(5, 5); 
     cpl_mask_not(kernel);
  @endcode

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL.
  - CPL_ERROR_ILLEGAL_INPUT if the kernel has a side of even length.
  - CPL_ERROR_DATA_NOT_FOUND If the kernel is empty, or in case of
       CPL_FILTER_STDEV if the kernel has only one element set to CPL_BINARY_1.
  - CPL_ERROR_ACCESS_OUT_OF_RANGE If the kernel has a side longer than the
                                  input image.
  - CPL_ERROR_INVALID_TYPE if the passed image type is not supported.
  - CPL_ERROR_TYPE_MISMATCH if in median filtering the input and output pixel
                             types differ.
  - CPL_ERROR_INCOMPATIBLE_INPUT If the input and output images have
                                 incompatible sizes.
  - CPL_ERROR_UNSUPPORTED_MODE If the output pixel buffer overlaps the input
                               one (or the kernel), or the border/filter mode
                               is unsupported.
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_filter_mask(cpl_image * self,
                                     const cpl_image * other,
                                     const cpl_mask * kernel,
                                     cpl_filter_mode filter,
                                     cpl_border_mode border)
{

    const cpl_size nsx    = cpl_image_get_size_x(self);
    const cpl_size nsy    = cpl_image_get_size_y(self);
    const cpl_size nx     = cpl_image_get_size_x(other);
    const cpl_size ny     = cpl_image_get_size_y(other);
    const cpl_size mx = cpl_mask_get_size_x(kernel);
    const cpl_size my = cpl_mask_get_size_y(kernel);
    const cpl_size hsizex = mx >> 1;
    const cpl_size hsizey = my >> 1;
    const cpl_binary * pmask  = cpl_mask_get_data_const(kernel);
    const void       * pother = cpl_image_get_data_const(other);
    const void       * pself  = cpl_image_get_data_const(self);
    /* assert( sizeof(char) == 1 ) */
    const void * polast = (const void*)((const char*)pother
                                        + (size_t)nx * (size_t)ny
                                        * cpl_type_get_sizeof
                                        (cpl_image_get_type(other)));
    /* pmask may not overlap pself at all */
    const void * pmlast = (const void*)(pmask + (size_t)mx * (size_t)my);
    const void * pslast = (const void*)((const char*)pself
                                        + (size_t)nsx * (size_t)nsy
                                        * cpl_type_get_sizeof
                                        (cpl_image_get_type(self)));
    cpl_binary       * pbpmself;
    const cpl_binary * badmap;
    /* There are four different types of input:
       1) No bpm, non-full kernel
       2) No bpm,     full kernel
       3)    bpm, non-full kernel
       4)    bpm,     full kernel
    */

    /* Input type 1: Not used (yet) */
    /* Input type 2 */
    void               (*filter_fast)(void *, const void *,
                                      cpl_size, cpl_size, cpl_size, cpl_size,
                                      cpl_border_mode);
    /* Input type 3 */
    cpl_error_code     (*filter_bpm)(void *, cpl_binary **,
                                     const void *, const cpl_binary *,
                                     const cpl_binary *, cpl_size, cpl_size,
                                     cpl_size, cpl_size, cpl_border_mode);
    /* Input type 4 */
    void               (*filter_full)(void *, cpl_binary **,
                                      const void *, const cpl_binary *,
                                      cpl_size, cpl_size, cpl_size, cpl_size,
                                      cpl_border_mode);
    cpl_boolean  same_type = cpl_image_get_type(self)
        == cpl_image_get_type(other);
    cpl_boolean  is_full;


    cpl_ensure_code(self   != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(other  != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(kernel != NULL, CPL_ERROR_NULL_INPUT);

    /* pself has to be above all of the other pixel buffer, or */
    /* ...pother has to be above the first hsize+1 rows of pself */
    /* ... unless they have different pixel types, in which case the
           two pixel buffers may not overlap at all */
    if (!(pself >= polast || pother >= pslast)) {
        return cpl_error_set_message_(CPL_ERROR_UNSUPPORTED_MODE,
                                     "pself=%p, polast=%p, pother=%p, "
                                     "pslast=%p, same_type=%d",
                                     pself, polast, pother, pslast,
                                     same_type);
    }

    /* If this check fails, the caller is doing something really weird... */
    cpl_ensure_code((const void*)pmask >= pslast ||
                    pself >= (const void*)pmlast,
                    CPL_ERROR_UNSUPPORTED_MODE);

    /* Only odd-sized masks allowed */
    cpl_ensure_code((mx&1) == 1, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code((my&1) == 1, CPL_ERROR_ILLEGAL_INPUT);

    if (filter == CPL_FILTER_STDEV || filter == CPL_FILTER_STDEV_FAST) {
        /* Mask must have at least two non-zero elements */
        const cpl_binary * pmask1 = memchr(pmask, CPL_BINARY_1,
                                           (size_t)mx * (size_t)my
                                           * sizeof(*pmask));
        const cpl_binary * pmask2;

        cpl_ensure_code(pmask1 != NULL, CPL_ERROR_DATA_NOT_FOUND);

        /* Rely on memchr() to return NULL on zero-size */
        pmask2 = memchr(pmask1 + sizeof(*pmask), CPL_BINARY_1,
                        ((size_t)mx * (size_t)my - (1 + pmask1 - pmask))
                        * sizeof(*pmask));

        cpl_ensure_code(pmask2 != NULL, CPL_ERROR_DATA_NOT_FOUND);
    } else {
        /* Mask may not be empty */
        cpl_ensure_code(!cpl_mask_is_empty(kernel), CPL_ERROR_DATA_NOT_FOUND);
    }

    cpl_ensure_code(mx <= nx, CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_ensure_code(my <= ny, CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (border == CPL_BORDER_CROP) {
        cpl_ensure_code(nsx == nx - 2 * hsizex, CPL_ERROR_INCOMPATIBLE_INPUT);

        cpl_ensure_code(nsy == ny - 2 * hsizey, CPL_ERROR_INCOMPATIBLE_INPUT);

    } else {
        cpl_ensure_code(nsx == nx, CPL_ERROR_INCOMPATIBLE_INPUT);

        cpl_ensure_code(nsy == ny, CPL_ERROR_INCOMPATIBLE_INPUT);

    }

    /* Get and reset all bad pixels, if any */
    pbpmself = self->bpm != NULL
        ? memset(cpl_mask_get_data(self->bpm), CPL_BINARY_0,
                 (size_t)nsx * (size_t)nsy)
        : NULL;

    badmap = other->bpm == NULL || cpl_mask_is_empty(other->bpm)
        ? NULL : cpl_mask_get_data_const(other->bpm);

    is_full = memchr(pmask, CPL_BINARY_0,
                     (size_t)mx * (size_t)my * sizeof(*pmask)) == NULL
        ? CPL_TRUE : CPL_FALSE;

    filter_fast = NULL;
    filter_bpm  = NULL;
    filter_full = NULL;


    /* FIXME:
       For sufficiently large kernels and for a sufficiently small
       (but non-zero) number of bad pixels it may be faster to:
       1) Process all pixels with the no-bpm version.
       2) Dilate the (non-empty) bpm with the kernel,
       3) Redo the pixels covered by the dilated bpm

       - The two pixel buffer passes should ideally be done block-wise.
       - median filter with a full kernel should be done first, since it
         has the highest potential for gain
    */

    if (filter == CPL_FILTER_MEDIAN) {
        const cpl_boolean use_all = is_full && badmap == NULL;

        if (hsizex == 0 && hsizey == 0) {
            const cpl_error_code error = cpl_image_copy(self, other, 1, 1);
            cpl_ensure_code(!error, error);
            return CPL_ERROR_NONE;
        }

        cpl_ensure_code(same_type, CPL_ERROR_TYPE_MISMATCH);

        switch (cpl_image_get_type(self)) {
        case CPL_TYPE_DOUBLE:
            if (use_all) {
                filter_fast = cpl_filter_median_fast_double;
            } else {
                filter_bpm = cpl_filter_median_slow_double;
            }
            break;
        case CPL_TYPE_FLOAT:
            if (use_all) {
                filter_fast = cpl_filter_median_fast_float;
            } else {
                filter_bpm = cpl_filter_median_slow_float;
            }
            break;
        case CPL_TYPE_INT:
            if (use_all) {
                filter_fast = cpl_filter_median_fast_int;
            } else {
                filter_bpm = cpl_filter_median_slow_int;
            }
            break;
        default:
        /*
         * Currently, it is an error in CPL to reach this point, as all
         * possible types for images (see cpl_image_new()) are supported.
         *
         * However, it could happen, if cpl_image_new() were extended to
         * support images of a new type and the new type were not supported
         * in this function. For that case, we keep setting the appropriate
         * error code in this default section.
         */
            return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        }

    } else if (filter == CPL_FILTER_STDEV ||
               (filter == CPL_FILTER_STDEV_FAST && !is_full)) {
        cpl_ensure_code(border == CPL_BORDER_FILTER,
                        CPL_ERROR_UNSUPPORTED_MODE);

        switch (cpl_image_get_type(self)) {
        case CPL_TYPE_DOUBLE: {
            switch (cpl_image_get_type(other)) {
            case CPL_TYPE_DOUBLE:
                filter_bpm = cpl_filter_stdev_slow_double_double;
                break;
            case CPL_TYPE_FLOAT:
                filter_bpm = cpl_filter_stdev_slow_double_float;
                break;
            case CPL_TYPE_INT:
                filter_bpm = cpl_filter_stdev_slow_double_int;
                break;
            default:
                /* See comment in previous switch() default: */
                return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            }

            break;
        }

        case CPL_TYPE_FLOAT: {
            switch (cpl_image_get_type(other)) {
            case CPL_TYPE_DOUBLE:
                filter_bpm = cpl_filter_stdev_slow_float_double;
                break;
            case CPL_TYPE_FLOAT:
                filter_bpm = cpl_filter_stdev_slow_float_float;
                break;
            case CPL_TYPE_INT:
                filter_bpm = cpl_filter_stdev_slow_float_int;
                break;
            default:
                /* See comment in previous switch() default: */
                return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            }
            break;
        }


        case CPL_TYPE_INT: {
            switch (cpl_image_get_type(other)) {
            case CPL_TYPE_DOUBLE:
                filter_bpm = cpl_filter_stdev_slow_int_double;
                break;
            case CPL_TYPE_FLOAT:
                filter_bpm = cpl_filter_stdev_slow_int_float;
                break;
            case CPL_TYPE_INT:
                filter_bpm = cpl_filter_stdev_slow_int_int;
                break;
            default:
                /* See comment in previous switch() default: */
                return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            }
            break;
        }

        default:
            /* See comment in previous switch() default: */
            return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        }
    } else if (filter == CPL_FILTER_AVERAGE ||
               (filter == CPL_FILTER_AVERAGE_FAST && !is_full)) {

        cpl_ensure_code(border == CPL_BORDER_FILTER,
                        CPL_ERROR_UNSUPPORTED_MODE);

        switch (cpl_image_get_type(self)) {
        case CPL_TYPE_DOUBLE: {
            switch (cpl_image_get_type(other)) {
            case CPL_TYPE_DOUBLE:
                filter_bpm = cpl_filter_average_slow_double_double;
                break;
            case CPL_TYPE_FLOAT:
                filter_bpm = cpl_filter_average_slow_double_float;
                break;
            case CPL_TYPE_INT:
                filter_bpm = cpl_filter_average_slow_double_int;
                break;
            default:
                /* See comment in previous switch() default: */
                return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            }

            break;
        }

        case CPL_TYPE_FLOAT: {
            switch (cpl_image_get_type(other)) {
            case CPL_TYPE_DOUBLE:
                filter_bpm = cpl_filter_average_slow_float_double;
                break;
            case CPL_TYPE_FLOAT:
                filter_bpm = cpl_filter_average_slow_float_float;
                break;
            case CPL_TYPE_INT:
                filter_bpm = cpl_filter_average_slow_float_int;
                break;
            default:
                /* See comment in previous switch() default: */
                return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            }
            break;
        }


        case CPL_TYPE_INT: {
            switch (cpl_image_get_type(other)) {
            case CPL_TYPE_DOUBLE:
                filter_bpm = cpl_filter_average_slow_int_double;
                break;
            case CPL_TYPE_FLOAT:
                filter_bpm = cpl_filter_average_slow_int_float;
                break;
            case CPL_TYPE_INT:
                filter_bpm = cpl_filter_average_slow_int_int;
                break;
            default:
                /* See comment in previous switch() default: */
                return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            }
            break;
        }

        default:
            /* See comment in previous switch() default: */
            return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        }
    } else if (filter == CPL_FILTER_AVERAGE_FAST) {

        cpl_ensure_code(border == CPL_BORDER_FILTER,
                        CPL_ERROR_UNSUPPORTED_MODE);

        if (badmap != NULL) {
            /* Full kernel, bad pixels */
            switch (cpl_image_get_type(self)) {
            case CPL_TYPE_DOUBLE: {
                switch (cpl_image_get_type(other)) {
                case CPL_TYPE_DOUBLE:
                    filter_full = cpl_filter_average_bpm_double_double;
                    break;
                case CPL_TYPE_FLOAT:
                    filter_full = cpl_filter_average_bpm_double_float;
                    break;
                case CPL_TYPE_INT:
                    filter_full = cpl_filter_average_bpm_double_int;
                    break;
                default:
                    /* See comment in previous switch() default: */
                    return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
                }

                break;
            }

            case CPL_TYPE_FLOAT: {
                switch (cpl_image_get_type(other)) {
                case CPL_TYPE_DOUBLE:
                    filter_full = cpl_filter_average_bpm_float_double;
                    break;
                case CPL_TYPE_FLOAT:
                    filter_full = cpl_filter_average_bpm_float_float;
                    break;
                case CPL_TYPE_INT:
                    filter_full = cpl_filter_average_bpm_float_int;
                    break;
                default:
                    /* See comment in previous switch() default: */
                    return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
                }
                break;
            }


            case CPL_TYPE_INT: {
                switch (cpl_image_get_type(other)) {
                case CPL_TYPE_DOUBLE:
                    filter_full = cpl_filter_average_bpm_int_double;
                    break;
                case CPL_TYPE_FLOAT:
                    filter_full = cpl_filter_average_bpm_int_float;
                    break;
                case CPL_TYPE_INT:
                    filter_full = cpl_filter_average_bpm_int_int;
                    break;
                default:
                    /* See comment in previous switch() default: */
                    return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
                }
                break;
            }

            default:
                /* See comment in previous switch() default: */
                return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            }
        } else {
            /* Full kernel, no bad pixels */
            switch (cpl_image_get_type(self)) {
            case CPL_TYPE_DOUBLE: {
                switch (cpl_image_get_type(other)) {
                case CPL_TYPE_DOUBLE:
                    filter_fast = cpl_filter_average_double_double;
                    break;
                case CPL_TYPE_FLOAT:
                    filter_fast = cpl_filter_average_double_float;
                    break;
                case CPL_TYPE_INT:
                    filter_fast = cpl_filter_average_double_int;
                    break;
                default:
                    /* See comment in previous switch() default: */
                    return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
                }

                break;
            }

            case CPL_TYPE_FLOAT: {
                switch (cpl_image_get_type(other)) {
                case CPL_TYPE_DOUBLE:
                    filter_fast = cpl_filter_average_float_double;
                    break;
                case CPL_TYPE_FLOAT:
                    filter_fast = cpl_filter_average_float_float;
                    break;
                case CPL_TYPE_INT:
                    filter_fast = cpl_filter_average_float_int;
                    break;
                default:
                    /* See comment in previous switch() default: */
                    return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
                }
                break;
            }


            case CPL_TYPE_INT: {
                switch (cpl_image_get_type(other)) {
                case CPL_TYPE_DOUBLE:
                    filter_fast = cpl_filter_average_int_double;
                    break;
                case CPL_TYPE_FLOAT:
                    filter_fast = cpl_filter_average_int_float;
                    break;
                case CPL_TYPE_INT:
                    filter_fast = cpl_filter_average_int_int;
                    break;
                default:
                    /* See comment in previous switch() default: */
                    return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
                }
                break;
            }

            default:
                /* See comment in previous switch() default: */
                return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            }
        }
    } else if (filter == CPL_FILTER_STDEV_FAST) {

        cpl_ensure_code(border == CPL_BORDER_FILTER,
                        CPL_ERROR_UNSUPPORTED_MODE);

        if (badmap != NULL) {
        /* FIXME : 
           Implement cpl_filter_stdev_bpm_* similarly to
                     cpl_filter_average_bpm_* and replace 
                     cpl_filter_stdev_slow_* with it */

            /* Full kernel, bad pixels */
            switch (cpl_image_get_type(self)) {
            case CPL_TYPE_DOUBLE: {
                switch (cpl_image_get_type(other)) {
                case CPL_TYPE_DOUBLE:
                    filter_bpm = cpl_filter_stdev_slow_double_double;
                    break;
                case CPL_TYPE_FLOAT:
                    filter_bpm = cpl_filter_stdev_slow_double_float;
                    break;
                case CPL_TYPE_INT:
                    filter_bpm = cpl_filter_stdev_slow_double_int;
                    break;
                default:
                    /* See comment in previous switch() default: */
                    return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
                }

                break;
            }

            case CPL_TYPE_FLOAT: {
                switch (cpl_image_get_type(other)) {
                case CPL_TYPE_DOUBLE:
                    filter_bpm = cpl_filter_stdev_slow_float_double;
                    break;
                case CPL_TYPE_FLOAT:
                    filter_bpm = cpl_filter_stdev_slow_float_float;
                    break;
                case CPL_TYPE_INT:
                    filter_bpm = cpl_filter_stdev_slow_float_int;
                    break;
                default:
                    /* See comment in previous switch() default: */
                    return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
                }
                break;
            }


            case CPL_TYPE_INT: {
                switch (cpl_image_get_type(other)) {
                case CPL_TYPE_DOUBLE:
                    filter_bpm = cpl_filter_stdev_slow_int_double;
                    break;
                case CPL_TYPE_FLOAT:
                    filter_bpm = cpl_filter_stdev_slow_int_float;
                    break;
                case CPL_TYPE_INT:
                    filter_bpm = cpl_filter_stdev_slow_int_int;
                    break;
                default:
                    /* See comment in previous switch() default: */
                    return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
                }
                break;
            }

            default:
                /* See comment in previous switch() default: */
                return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            }
        } else {
            /* Full kernel, no bad pixels */
            switch (cpl_image_get_type(self)) {
            case CPL_TYPE_DOUBLE: {
                switch (cpl_image_get_type(other)) {
                case CPL_TYPE_DOUBLE:
                    filter_fast = cpl_filter_stdev_double_double;
                    break;
                case CPL_TYPE_FLOAT:
                    filter_fast = cpl_filter_stdev_double_float;
                    break;
                case CPL_TYPE_INT:
                    filter_fast = cpl_filter_stdev_double_int;
                    break;
                default:
                    /* See comment in previous switch() default: */
                    return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
                }

                break;
            }

            case CPL_TYPE_FLOAT: {
                switch (cpl_image_get_type(other)) {
                case CPL_TYPE_DOUBLE:
                    filter_fast = cpl_filter_stdev_float_double;
                    break;
                case CPL_TYPE_FLOAT:
                    filter_fast = cpl_filter_stdev_float_float;
                    break;
                case CPL_TYPE_INT:
                    filter_fast = cpl_filter_stdev_float_int;
                    break;
                default:
                    /* See comment in previous switch() default: */
                    return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
                }
                break;
            }

            case CPL_TYPE_INT: {
                switch (cpl_image_get_type(other)) {
                case CPL_TYPE_DOUBLE:
                    filter_fast = cpl_filter_stdev_int_double;
                    break;
                case CPL_TYPE_FLOAT:
                    filter_fast = cpl_filter_stdev_int_float;
                    break;
                case CPL_TYPE_INT:
                    filter_fast = cpl_filter_stdev_int_int;
                    break;
                default:
                    /* See comment in previous switch() default: */
                    return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
                }
                break;
            }

            default:
                /* See comment in previous switch() default: */
                return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            }
        }
    }

    if (filter_fast != NULL) {
        filter_fast(cpl_image_get_data(self),
                    cpl_image_get_data_const(other),
                    nx, ny, hsizex, hsizey,
                    (unsigned)border);
#ifndef CPL_MEDIAN_SAMPLE
        if (filter == CPL_FILTER_MEDIAN && border == CPL_BORDER_FILTER) {
            const cpl_type type = cpl_image_get_type(self);

            assert(same_type);
            assert(is_full);
            assert(badmap == NULL);

            if (type == CPL_TYPE_DOUBLE) {
                cpl_filter_median_even_double(cpl_image_get_data(self),
                                              cpl_image_get_data_const(other),
                                              nx, ny, hsizex, hsizey);
            } else if (type == CPL_TYPE_FLOAT) {
                cpl_filter_median_even_float(cpl_image_get_data(self),
                                             cpl_image_get_data_const(other),
                                             nx, ny, hsizex, hsizey);
            } else {
                assert( type == CPL_TYPE_INT);
                cpl_filter_median_even_int(cpl_image_get_data(self),
                                           cpl_image_get_data_const(other),
                                           nx, ny, hsizex, hsizey);
            }
        }
#endif
    } else if (filter_full != NULL) {
        filter_full(cpl_image_get_data(self), &pbpmself,
                   cpl_image_get_data_const(other), badmap,
                   nx, ny, hsizex, hsizey,
                   border);
    } else if (filter_bpm != NULL) {

        if (filter_bpm(cpl_image_get_data(self), &pbpmself,
                       cpl_image_get_data_const(other), badmap,
                       pmask, nx, ny, hsizex, hsizey,
                       border)) {
            return cpl_error_set_where_();
        }
    } else {
        return cpl_error_set_(CPL_ERROR_UNSUPPORTED_MODE);
    }

    if (self->bpm == NULL && pbpmself != NULL) {
        /* A new BPM was created for the output */
        self->bpm = cpl_mask_wrap(nx, ny, pbpmself);
    }

    return CPL_ERROR_NONE;

}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief  Filter an image using a floating-point kernel
  @param  self   Pre-allocated image to hold the filtered result
  @param  other  Image to filter
  @param  kernel Pixel weigths
  @param  filter CPL_FILTER_LINEAR, CPL_FILTER_MORPHO
  @param  border CPL_BORDER_FILTER
  @return CPL_ERROR_NONE or the relevant CPL error code
  @see cpl_image_filter_mask

  The two images must have equal dimensions.

  The kernel must have an odd number of rows and an odd number of columns
  and at least one non-zero element.

  For scaling filters (@c CPL_FILTER_LINEAR_SCALE and
  @c CPL_FILTER_MORPHO_SCALE) the flux of the filtered image will be scaled
  with the sum of the weights of the kernel. If for a given input pixel location
  the kernel covers only bad pixels, the filtered pixel value is flagged as
  bad and set to zero.

  For flux-preserving filters (@c CPL_FILTER_LINEAR and @c CPL_FILTER_MORPHO)
  the filtered pixel must have at least one input pixel with a non-zero weight
  available. Output pixels where this is not the case are set to zero and
  flagged as bad.

  In-place filtering is not supported.

  Supported filters: @c CPL_FILTER_LINEAR, @c CPL_FILTER_MORPHO,
                     @c CPL_FILTER_LINEAR_SCALE, @c CPL_FILTER_MORPHO_SCALE.

  Supported borders modes: @c CPL_BORDER_FILTER

   @par Example:
   @code
     cpl_image_filter(filtered, raw, kernel, CPL_FILTER_LINEAR,
                                             CPL_BORDER_FILTER);
   @endcode

  Beware that the 1st pixel - at (1,1) - in an image is the lower left,
  while the 1st element in a matrix  - at (0,0) - is the top left. Thus to
  shift an image 1 pixel up and 1 pixel right with the CPL_FILTER_LINEAR and
  a 3 by 3 kernel, one should set to 1.0 the bottom leftmost matrix element
  which is at row 3, column 1, i.e.
  @code
      cpl_matrix_set(kernel, 2, 0);
  @endcode

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL.
  - CPL_ERROR_ILLEGAL_INPUT if the kernel has a side of even length.
  - CPL_ERROR_DIVISION_BY_ZERO If the kernel is a zero-matrix.
  - CPL_ERROR_ACCESS_OUT_OF_RANGE If the kernel has a side longer than the
                                  input image.
  - CPL_ERROR_INVALID_TYPE if the passed image type is not supported.
  - CPL_ERROR_TYPE_MISMATCH if in median filtering the input and output pixel
                             types differ.
  - CPL_ERROR_INCOMPATIBLE_INPUT If the input and output images have
                                 incompatible sizes.
  - CPL_ERROR_UNSUPPORTED_MODE If the output pixel buffer overlaps the input
                               one (or the kernel), or the border/filter mode
                               is unsupported.
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_filter(cpl_image        * self,
                                const cpl_image  * other,
                                const cpl_matrix * kernel,
                                cpl_filter_mode    filter,
                                cpl_border_mode    border)
{

    const cpl_size nsx    = cpl_image_get_size_x(self);
    const cpl_size nsy    = cpl_image_get_size_y(self);
    const cpl_size nx     = cpl_image_get_size_x(other);
    const cpl_size ny     = cpl_image_get_size_y(other);
    const cpl_size mx = cpl_matrix_get_ncol(kernel);
    const cpl_size my = cpl_matrix_get_nrow(kernel);
    const cpl_size hsizex = mx >> 1;
    const cpl_size hsizey = my >> 1;
    const double * pkernel  = cpl_matrix_get_data_const(kernel);
    const void   * pother = cpl_image_get_data_const(other);
    const void   * pself  = cpl_image_get_data_const(self);
    /* assert( sizeof(char) == 1 ) */
    const void * polast = (const void*)((const char*)pother
                                        + (size_t)nx * (size_t)ny
                                        * cpl_type_get_sizeof
                                        (cpl_image_get_type(other)));
    /* pkernel may not overlap pself at all */
    const void * pmlast = (const void*)(pkernel + mx * my);
    const void * pslast = (const void*)((const char*)pself
                                        + (size_t)nsx * (size_t)nsy
                                        * cpl_type_get_sizeof
                                        (cpl_image_get_type(self)));
    cpl_binary       * pbpmself;
    const cpl_binary * badmap;
    cpl_error_code     (*filter_func)(void *, cpl_binary **,
                                      const void *, const cpl_binary *,
                                      const double *, cpl_size, cpl_size,
                                      cpl_boolean,
                                      cpl_size, cpl_size, cpl_border_mode);
    cpl_boolean dodiv = CPL_FALSE;
    cpl_size i;

    cpl_ensure_code(self    != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(other   != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(kernel  != NULL, CPL_ERROR_NULL_INPUT);

    /* pself has to be above all of the other pixel buffer, or */
    /* ...pother has to be above the first hsize+1 rows of pself */
    /* ... unless they have different pixel types, in which case the
           two pixel buffers may not overlap at all */
    cpl_ensure_code(pself >= polast || pother >= pslast,
        CPL_ERROR_UNSUPPORTED_MODE);

    /* If this check fails, the caller is doing something really weird... */
    cpl_ensure_code((const void*)pkernel >= pslast ||
                    pself >= (const void*)pmlast,
                    CPL_ERROR_UNSUPPORTED_MODE);

    /* Only odd-sized kernels allowed */
    cpl_ensure_code((mx&1) == 1, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code((my&1) == 1, CPL_ERROR_ILLEGAL_INPUT);

    cpl_ensure_code(mx <= nx, CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_ensure_code(my <= ny, CPL_ERROR_ACCESS_OUT_OF_RANGE);

    /* kernel may not be empty */
    for (i=0 ; i < mx * my ; i++) {
        if (pkernel[i] != 0.0) break;
    }
    cpl_ensure_code(i < mx * my, CPL_ERROR_DIVISION_BY_ZERO);

    if (border == CPL_BORDER_CROP) {
        cpl_ensure_code(nsx == nx - 2 * hsizex, CPL_ERROR_INCOMPATIBLE_INPUT);

        cpl_ensure_code(nsy == ny - 2 * hsizey, CPL_ERROR_INCOMPATIBLE_INPUT);

    } else {
        cpl_ensure_code(nsx == nx, CPL_ERROR_INCOMPATIBLE_INPUT);

        cpl_ensure_code(nsy == ny, CPL_ERROR_INCOMPATIBLE_INPUT);

    }

    /* Get and reset all bad pixels, if any */
    pbpmself = self->bpm != NULL
        ? memset(cpl_mask_get_data(self->bpm), CPL_BINARY_0,
                 (size_t)nsx * (size_t)nsy)
        : NULL;

    badmap = other->bpm == NULL || cpl_mask_is_empty(other->bpm)
        ? NULL : cpl_mask_get_data_const(other->bpm);

    filter_func = NULL;

    if (filter == CPL_FILTER_LINEAR || filter == CPL_FILTER_LINEAR_SCALE) {

        cpl_ensure_code(border == CPL_BORDER_FILTER,
                        CPL_ERROR_UNSUPPORTED_MODE);

        dodiv = filter == CPL_FILTER_LINEAR;

        switch (cpl_image_get_type(self)) {
        case CPL_TYPE_DOUBLE: {
            switch (cpl_image_get_type(other)) {
            case CPL_TYPE_DOUBLE:
                filter_func = cpl_filter_linear_slow_double_double;
                break;
            case CPL_TYPE_FLOAT:
                filter_func = cpl_filter_linear_slow_double_float;
                break;
            case CPL_TYPE_INT:
                filter_func = cpl_filter_linear_slow_double_int;
                break;
            default:
        /*
         * Currently, it is an error in CPL to reach this point, as all
         * possible types for images (see cpl_image_new()) are supported.
         *
         * However, it could happen, if cpl_image_new() were extended to
         * support images of a new type and the new type were not supported
         * in this function. For that case, we keep setting the appropriate
         * error code in this default section.
         */
                return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            }

            break;
        }

        case CPL_TYPE_FLOAT: {
            switch (cpl_image_get_type(other)) {
            case CPL_TYPE_DOUBLE:
                filter_func = cpl_filter_linear_slow_float_double;
                break;
            case CPL_TYPE_FLOAT:
                filter_func = cpl_filter_linear_slow_float_float;
                break;
            case CPL_TYPE_INT:
                filter_func = cpl_filter_linear_slow_float_int;
                break;
            default:
                /* See comment in previous switch() default: */
                return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            }
            break;
        }


        case CPL_TYPE_INT: {
            switch (cpl_image_get_type(other)) {
            case CPL_TYPE_DOUBLE:
                filter_func = cpl_filter_linear_slow_int_double;
                break;
            case CPL_TYPE_FLOAT:
                filter_func = cpl_filter_linear_slow_int_float;
                break;
            case CPL_TYPE_INT:
                filter_func = cpl_filter_linear_slow_int_int;
                break;
            default:
                /* See comment in previous switch() default: */
                return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            }
            break;
        }

        default:
            /* See comment in previous switch() default: */
            return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        }

    } else if (filter == CPL_FILTER_MORPHO ||
               filter == CPL_FILTER_MORPHO_SCALE) {

        cpl_ensure_code(border == CPL_BORDER_FILTER,
                        CPL_ERROR_UNSUPPORTED_MODE);

        dodiv = filter == CPL_FILTER_MORPHO;

        switch (cpl_image_get_type(self)) {
        case CPL_TYPE_DOUBLE: {
            switch (cpl_image_get_type(other)) {
            case CPL_TYPE_DOUBLE:
                filter_func = cpl_filter_morpho_slow_double_double;
                break;
            case CPL_TYPE_FLOAT:
                filter_func = cpl_filter_morpho_slow_double_float;
                break;
            case CPL_TYPE_INT:
                filter_func = cpl_filter_morpho_slow_double_int;
                break;
            default:
        /*
         * Currently, it is an error in CPL to reach this point, as all
         * possible types for images (see cpl_image_new()) are supported.
         *
         * However, it could happen, if cpl_image_new() were extended to
         * support images of a new type and the new type were not supported
         * in this function. For that case, we keep setting the appropriate
         * error code in this default section.
         */
                return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            }

            break;
        }

        case CPL_TYPE_FLOAT: {
            switch (cpl_image_get_type(other)) {
            case CPL_TYPE_DOUBLE:
                filter_func = cpl_filter_morpho_slow_float_double;
                break;
            case CPL_TYPE_FLOAT:
                filter_func = cpl_filter_morpho_slow_float_float;
                break;
            case CPL_TYPE_INT:
                filter_func = cpl_filter_morpho_slow_float_int;
                break;
            default:
                /* See comment in previous switch() default: */
                return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            }
            break;
        }


        case CPL_TYPE_INT: {
            switch (cpl_image_get_type(other)) {
            case CPL_TYPE_DOUBLE:
                filter_func = cpl_filter_morpho_slow_int_double;
                break;
            case CPL_TYPE_FLOAT:
                filter_func = cpl_filter_morpho_slow_int_float;
                break;
            case CPL_TYPE_INT:
                filter_func = cpl_filter_morpho_slow_int_int;
                break;
            default:
                /* See comment in previous switch() default: */
                return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            }
            break;
        }

        default:
            /* See comment in previous switch() default: */
            return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        }
    }

    cpl_ensure_code(filter_func != NULL, CPL_ERROR_UNSUPPORTED_MODE);

    if (filter_func(cpl_image_get_data(self), &pbpmself,
                    cpl_image_get_data_const(other), badmap,
                    pkernel, nx, ny, dodiv,
                    hsizex, hsizey, border)) {
        return cpl_error_set_where_();
    }

    if (self->bpm == NULL && pbpmself != NULL) {
        /* A new BPM was created for the output */
        self->bpm = cpl_mask_wrap(nx, ny, pbpmself);
    }

    return CPL_ERROR_NONE;

}


/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Compute a linear filtering
  @param    in   The image to filter
  @param    ker  The kernel
  @return   The filtered image or NULL on error
  @see      cpl_image_filter()
  @deprecated Replace this call with 
     cpl_image_filter() using CPL_FILTER_LINEAR and CPL_BORDER_FILTER.

 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_filter_linear(const cpl_image  * in,
                                    const cpl_matrix * ker)
{
    cpl_image * self;

    cpl_ensure(in != NULL,  CPL_ERROR_NULL_INPUT, NULL);

    self = cpl_image_new(in->nx, in->ny, in->type);

    if (cpl_image_filter(self, in, ker, CPL_FILTER_LINEAR,
                         CPL_BORDER_FILTER)) {
        cpl_image_delete(self);
        self = NULL;
        (void)cpl_error_set_where_();
    }

    return self;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Filter an image in spatial domain with a morpho kernel.
  @param    in          Image to filter.
  @param    ker         Filter definition.
  @return   1 newly allocated image or NULL in error case.
  @see      cpl_image_filter()
  @deprecated Replace this call with 
     cpl_image_filter() using CPL_FILTER_MORPHO and CPL_BORDER_FILTER.

 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_filter_morpho(const cpl_image  * in,
                                    const cpl_matrix * ker)
{
    cpl_image * self;

    cpl_ensure(in != NULL,  CPL_ERROR_NULL_INPUT, NULL);

    self = cpl_image_new(in->nx, in->ny, in->type);

    if (cpl_image_filter(self, in, ker, CPL_FILTER_MORPHO,
                         CPL_BORDER_FILTER)) {
        cpl_image_delete(self);
        self = NULL;
        (void)cpl_error_set_where_();
    }

    return self;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Apply a spatial median filter to an image
  @param    in      Image to filter.
  @param    ker     the kernel
  @return   1 newly allocated image or NULL in error case
  @see      cpl_image_filter_mask
  @deprecated Replace this call with 
     cpl_image_filter_mask() using CPL_FILTER_MEDIAN and CPL_BORDER_FILTER.

 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_filter_median(const cpl_image  * in,
                                    const cpl_matrix * ker)
{

    cpl_image * self;
    cpl_mask  * mask;

    cpl_ensure(in != NULL,  CPL_ERROR_NULL_INPUT, NULL);

    mask = cpl_mask_new_from_matrix(ker, 1.0, 1e-5);

    cpl_ensure(mask != NULL, cpl_error_get_code(), NULL);

    self = cpl_image_new(in->nx, in->ny, in->type);

    if (cpl_image_filter_mask(self, in, mask, CPL_FILTER_MEDIAN,
                              CPL_BORDER_FILTER)) {
        cpl_image_delete(self);
        self = NULL;
        (void)cpl_error_set_where_();
    }

    cpl_mask_delete(mask);

    return self;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Standard deviation filter
  @param    in      input image
  @param    ker        the kernel
  @return   a newly allocated filtered image or NULL on error
  @see      cpl_image_filter_mask
  @deprecated Replace this call with 
     cpl_image_filter_mask() using CPL_FILTER_STDEV and CPL_BORDER_FILTER.

 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_filter_stdev(const cpl_image  * in,
                                   const cpl_matrix * ker)
{

    cpl_image * self;
    cpl_mask  * mask;

    cpl_ensure(in != NULL,  CPL_ERROR_NULL_INPUT, NULL);

    mask = cpl_mask_new_from_matrix(ker, 1.0, 1e-5);

    cpl_ensure(mask != NULL, cpl_error_get_code(), NULL);

    self = cpl_image_new(in->nx, in->ny, in->type);

    if (cpl_mask_get_size_x(mask) * cpl_mask_get_size_y(mask) > 1 &&
        cpl_image_filter_mask(self, in, mask, CPL_FILTER_STDEV,
                              CPL_BORDER_FILTER)) {
        cpl_image_delete(self);
        self = NULL;
        (void)cpl_error_set_where_();
    }

    cpl_mask_delete(mask);

    return self;

}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief    Fill a mask from a matrix
  @param    kernel The matrix
  @param    value  The value to interpret as CPL_BINARY_1
  @param    tol    The tolerance on the value
  @return   the mask on success, or NULL on error

  Example:

    matrix       ->    mask

    1.0 0.0 1.0       0 0 0
    0.0 0.0 0.0       0 0 0
    0.0 0.0 0.0       1 0 1

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
static cpl_mask * cpl_mask_new_from_matrix(const cpl_matrix * kernel,
                                           double value, double tol)
{

    const cpl_size nx   = cpl_matrix_get_ncol(kernel);
    const cpl_size ny   = cpl_matrix_get_nrow(kernel);
    const cpl_size n    = nx * ny;
    const double * data = cpl_matrix_get_data_const(kernel);
    cpl_mask     * self;
    cpl_binary   * pself;
    cpl_size       i;

    cpl_ensure(kernel != NULL, CPL_ERROR_NULL_INPUT, NULL);

    self  = cpl_mask_new(nx, ny);
    pself = cpl_mask_get_data(self);

    for (i=0; i < n; i++) {
        if (fabs(data[i] - value) < tol) pself[i] = CPL_BINARY_1;
    }

    return self;
}
