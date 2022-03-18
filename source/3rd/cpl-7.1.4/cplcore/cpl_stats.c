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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <float.h>
#include <assert.h>

#include "cpl_memory.h"
#include "cpl_stats_impl.h"
#include "cpl_image_bpm.h"
#include "cpl_image_stats.h"
#include "cpl_mask.h"
#include "cpl_error_impl.h"
#include "cpl_tools.h"

#include "cpl_image_defs.h"

/*-----------------------------------------------------------------------------
                                   Defines
 -----------------------------------------------------------------------------*/

#define CPL_IMAGE_STATS_ALL             1
#define CPL_IMAGE_STATS_MINMAX          2
#define CPL_IMAGE_STATS_FLUX            3
#define CPL_IMAGE_STATS_VARIANCE        4
#define CPL_IMAGE_STATS_CENTROID        5
#define CPL_IMAGE_STATS_MEDIAN          6
#define CPL_IMAGE_STATS_MEDIAN_DEV      7
#define CPL_IMAGE_STATS_MAD             8

#define CONCAT(a,b) a ## _ ## b
#define CONCAT2X(a,b) CONCAT(a,b)

#define CPL_STATS_DUMP_ONE(OPERATOR, MEMBER, LABEL, CONVERTER)          \
    if (mode & OPERATOR) {                                           \
        mode ^= OPERATOR; /* Reset bit. At the end mode must be zero */ \
        cpl_ensure_code(fprintf(stream, "\t\t%-13s%" CONVERTER "\n", LABEL ":", \
                                self->MEMBER) > 0, CPL_ERROR_FILE_IO);  \
    }

#define CPL_STATS_DUMP_TWO(OPERATOR, MEMBER, LABEL, CONVERTER)          \
    if (mode & OPERATOR) {                                           \
        mode ^= OPERATOR; /* Reset bit. At the end mode must be zero */ \
        cpl_ensure_code(fprintf(stream, "\t\t%-13s%" CONVERTER "\n",    \
                                "X " LABEL ":", self->CONCAT2X(MEMBER, x)) \
                        > 0,  CPL_ERROR_FILE_IO);                       \
        cpl_ensure_code(fprintf(stream, "\t\t%-13s%" CONVERTER "\n",    \
                                "Y " LABEL ":", self->CONCAT2X(MEMBER, y)) \
                        > 0,  CPL_ERROR_FILE_IO);                       \
    }

/*----------------------------------------------------------------------------*/
/**
 * @defgroup cpl_stats Statistics
 *
 * This module provides functions to handle the cpl_stats object.
 * This object can contain the statistics that have been computed from
 * different CPL objects. Currently, only the function that computes
 * statistics on images (or images windows) is provided.
 *
 * @par Synopsis:
 * @code
 *   #include "cpl_stats.h"
 * @endcode
 */
/*----------------------------------------------------------------------------*/
/**@{*/

/*-----------------------------------------------------------------------------
                            Function codes
 -----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the minimum from a cpl_stats object
  @param    in  the cpl_stats object
  @return   the minimum value

  The call that created the cpl_stats object must have determined the
  minimum value.

  In case of error, the #_cpl_error_code_ code is set, and the returned double
  is undefined.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the requested stat has not been computed in in
 */
/*----------------------------------------------------------------------------*/
double cpl_stats_get_min(const cpl_stats * in)
{
    cpl_ensure(in!=NULL, CPL_ERROR_NULL_INPUT, 0.0);
    cpl_ensure(in->mode & (CPL_STATS_MIN | CPL_STATS_MINPOS),
               CPL_ERROR_ILLEGAL_INPUT, 0);
    return in->min;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the maximum from a cpl_stats object
  @param    in  the cpl_stats object
  @return   the maximum value
  @see      cpl_stats_get_min()
 */
/*----------------------------------------------------------------------------*/
double cpl_stats_get_max(const cpl_stats * in)
{
    cpl_ensure(in!=NULL, CPL_ERROR_NULL_INPUT, 0.0);
    cpl_ensure(in->mode & (CPL_STATS_MAX | CPL_STATS_MAXPOS),
               CPL_ERROR_ILLEGAL_INPUT, 0);
    return in->max;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the mean from a cpl_stats object
  @param    in  the cpl_stats object
  @return   the mean value
  @see      cpl_stats_get_min()
 */
/*----------------------------------------------------------------------------*/
double cpl_stats_get_mean(const cpl_stats * in)
{
    cpl_ensure(in!=NULL, CPL_ERROR_NULL_INPUT, 0.0);
    cpl_ensure(in->mode & CPL_STATS_MEAN, CPL_ERROR_ILLEGAL_INPUT, 0);
    return in->mean;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the median from a cpl_stats object
  @param    in  the cpl_stats object
  @return   the median value
  @see      cpl_stats_get_min()
 */
/*----------------------------------------------------------------------------*/
double cpl_stats_get_median(const cpl_stats * in)
{
    cpl_ensure(in!=NULL, CPL_ERROR_NULL_INPUT, 0.0);
    cpl_ensure(in->mode & CPL_STATS_MEDIAN, CPL_ERROR_ILLEGAL_INPUT, 0);
    return in->med;
}

/*----------------------------------------------------------------------------*/
/**
  @brief   Get the mean of the absolute median deviation from a cpl_stats object
  @param   in  the cpl_stats object
  @return  The mean of the absolute median deviation, or undefined on error
  @see     cpl_stats_get_min()
 */
/*----------------------------------------------------------------------------*/
double cpl_stats_get_median_dev(const cpl_stats * in)
{
    cpl_ensure(in!=NULL, CPL_ERROR_NULL_INPUT, 0.0);
    cpl_ensure(in->mode & CPL_STATS_MEDIAN_DEV, CPL_ERROR_ILLEGAL_INPUT, 0.0);
    return in->med_dev;
}

/*----------------------------------------------------------------------------*/
/**
  @brief   Get the median of the absolute median deviation
  @param   in  the cpl_stats object
  @return  The median of the absolute median deviation, or undefined on error
  @see     cpl_stats_get_min()
 */
/*----------------------------------------------------------------------------*/
double cpl_stats_get_mad(const cpl_stats * in)
{
    cpl_ensure(in!=NULL, CPL_ERROR_NULL_INPUT, 0.0);
    cpl_ensure(in->mode & CPL_STATS_MAD, CPL_ERROR_ILLEGAL_INPUT, 0.0);
    return in->mad;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the std. dev. from a cpl_stats object
  @param    in  the cpl_stats object
  @return   the standard deviation
  @see      cpl_stats_get_min()
 */
/*----------------------------------------------------------------------------*/
double cpl_stats_get_stdev(const cpl_stats * in)
{
    cpl_ensure(in!=NULL, CPL_ERROR_NULL_INPUT, -1);
    cpl_ensure(in->mode & CPL_STATS_STDEV, CPL_ERROR_ILLEGAL_INPUT, 0);
    return in->stdev;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the flux from a cpl_stats object
  @param    in  the cpl_stats object
  @return   the flux
  @see      cpl_stats_get_min()
 */
/*----------------------------------------------------------------------------*/
double cpl_stats_get_flux(const cpl_stats * in)
{
    cpl_ensure(in!=NULL, CPL_ERROR_NULL_INPUT, 0.0);
    cpl_ensure(in->mode & CPL_STATS_FLUX, CPL_ERROR_ILLEGAL_INPUT, 0);
    return in->flux;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the absolute flux from a cpl_stats object
  @param    in  the cpl_stats object
  @return   The absolute flux, or a negative number on error
  @see      cpl_stats_get_min()
 */
/*----------------------------------------------------------------------------*/
double cpl_stats_get_absflux(const cpl_stats * in)
{
    cpl_ensure(in!=NULL, CPL_ERROR_NULL_INPUT, -1);
    cpl_ensure(in->mode & CPL_STATS_ABSFLUX, CPL_ERROR_ILLEGAL_INPUT, -2);
    return in->absflux;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the sum of the squared values from a cpl_stats object
  @param    in  the cpl_stats object
  @return   the  square flux, or a negative number on error
  @see      cpl_stats_get_min()
 */
/*----------------------------------------------------------------------------*/
double cpl_stats_get_sqflux(const cpl_stats * in)
{
    cpl_ensure(in!=NULL, CPL_ERROR_NULL_INPUT, -1);
    cpl_ensure(in->mode & CPL_STATS_SQFLUX, CPL_ERROR_ILLEGAL_INPUT, -2);
    return in->sqflux;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the x centroid position from a cpl_stats object
  @param    in  the cpl_stats object
  @return   the x centroid
  @see      cpl_stats_get_min()
 */
/*----------------------------------------------------------------------------*/
double cpl_stats_get_centroid_x(const cpl_stats * in)
{
    cpl_ensure(in!=NULL, CPL_ERROR_NULL_INPUT, 0.0);
    cpl_ensure(in->mode & CPL_STATS_CENTROID, CPL_ERROR_ILLEGAL_INPUT,
               0);
    return in->centroid_x;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the y centroid position from a cpl_stats object
  @param    in  the cpl_stats object
  @return   the y centroid
  @see      cpl_stats_get_min()
 */
/*----------------------------------------------------------------------------*/
double cpl_stats_get_centroid_y(const cpl_stats * in)
{
    cpl_ensure(in!=NULL, CPL_ERROR_NULL_INPUT, 0.0);
    cpl_ensure(in->mode & CPL_STATS_CENTROID, CPL_ERROR_ILLEGAL_INPUT,
               0);
    return in->centroid_y;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the minimum x position from a cpl_stats object
  @param    in  the cpl_stats object
  @return   the x position (1 for the first pixel), non-positive on error.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_stats_get_min_x(const cpl_stats * in)
{
    cpl_ensure(in!=NULL, CPL_ERROR_NULL_INPUT, -1);
    cpl_ensure(in->mode & (CPL_STATS_MIN | CPL_STATS_MINPOS),
               CPL_ERROR_ILLEGAL_INPUT, 0);
    return in->min_x;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the minimum y position from a cpl_stats object
  @param    in  the cpl_stats object
  @return   the y position (1 for the first pixel), non-positive on error.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_stats_get_min_y(const cpl_stats * in)
{
    cpl_ensure(in!=NULL, CPL_ERROR_NULL_INPUT, -1);
    cpl_ensure(in->mode & (CPL_STATS_MIN | CPL_STATS_MINPOS),
               CPL_ERROR_ILLEGAL_INPUT, 0);
    return in->min_y;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the maximum x position from a cpl_stats object
  @param    in  the cpl_stats object
  @return   the x position (1 for the first pixel), non-positive on error.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_stats_get_max_x(const cpl_stats * in)
{
    cpl_ensure(in!=NULL, CPL_ERROR_NULL_INPUT, -1);
    cpl_ensure(in->mode & (CPL_STATS_MAX | CPL_STATS_MAXPOS),
               CPL_ERROR_ILLEGAL_INPUT, 0);
    return in->max_x;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the maximum y position from a cpl_stats object
  @param    in  the cpl_stats object
  @return   the y position (1 for the first pixel), non-positive on error.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_stats_get_max_y(const cpl_stats * in)
{
    cpl_ensure(in!=NULL, CPL_ERROR_NULL_INPUT, -1);
    cpl_ensure(in->mode & (CPL_STATS_MAX | CPL_STATS_MAXPOS),
               CPL_ERROR_ILLEGAL_INPUT, 0);
    return in->max_y;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the number of pixels from a cpl_stats object
  @param    in  the cpl_stats object
  @return   the number of pixels, -1 in error case.

  The creation of a cpl_stats object always causes the number of pixels
  to be determined.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_stats_get_npix(const cpl_stats * in)
{
    cpl_ensure(in!=NULL, CPL_ERROR_NULL_INPUT, -1);
    return in->npix;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Free memory associated to an cpl_stats object.
  @param    stats   the object to delete
  @return   void

  Frees all memory associated to a cpl_stats object. If the object @em stats
  is @c NULL, nothing is done and no error is set.
 */
/*----------------------------------------------------------------------------*/
void cpl_stats_delete(cpl_stats * stats)
{
    if (stats == NULL) return;
    cpl_free(stats);
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Compute various statistics of an image sub-window.
  @param    image       Input image.
  @param    mode        Bit field specifying which statistics to compute
  @param    llx         Lower left x position (FITS convention)
  @param    lly         Lower left y position (FITS convention)
  @param    urx         Upper right x position (FITS convention)
  @param    ury         Upper right y position (FITS convention)
  @return   1 newly allocated cpl_stats structure or NULL in error case

  Compute various image statistics.

  The specified bounds are included in the specified region.

  The statistics to compute is specified with a bit field,
  that may be set to any of these values
  - CPL_STATS_MIN
  - CPL_STATS_MAX
  - CPL_STATS_MEAN
  - CPL_STATS_MEDIAN
  - CPL_STATS_MEDIAN_DEV
  - CPL_STATS_MAD
  - CPL_STATS_STDEV
  - CPL_STATS_FLUX
  - CPL_STATS_ABSFLUX
  - CPL_STATS_SQFLUX
  - CPL_STATS_CENTROID
  - CPL_STATS_MINPOS
  - CPL_STATS_MAXPOS
  or any bitwise or (|) of these.
  For convenience the special value CPL_STATS_ALL may also be used,
  it is the conbination of all of the above values.

  E.g. the mode CPL_STATS_MIN | CPL_STATS_MEDIAN specifies
  the minimum and the median of the image.

  In the case of CPL_STATS_MIN and CPL_STATS_MAX where more than one set of
  coordinates share the extremum it is undefined which of those coordinates
  will be returned.

  On i386 platforms there can be significant differences in the round-off of the
  computation of single statistics and statistics computed via CPL_STATS_ALL.
  This is especially true for squared quantities such as the CPL_STATS_SQFLUX
  and CPL_STATS_STDEV. 

  Images can be CPL_TYPE_DOUBLE, CPL_TYPE_FLOAT, CPL_TYPE_INT.

  For the CPL_STATS_CENTROID computation, if there are negative pixels,
  the minimum value is added to all the pixels in order to have all
  pixels with positive values for computation.

  The returned object must be deallocated using cpl_stats_delete().

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ACCESS_OUT_OF_RANGE if the defined window is not in the image
  - CPL_ERROR_ILLEGAL_INPUT if the window definition is wrong (e.g llx > urx)
  - CPL_ERROR_DATA_NOT_FOUND if all the pixels are bad
  - CPL_ERROR_INVALID_TYPE if the passed image type is not supported
  - CPL_ERROR_INVALID_TYPE if mode is 1, e.g. due to a logical or (||)
      of the allowed options.
  - CPL_ERROR_UNSUPPORTED_MODE if mode is otherwise different from the
       allowed options.
 */
/*----------------------------------------------------------------------------*/
cpl_stats * cpl_stats_new_from_image_window(
        const cpl_image *   image,
        cpl_stats_mode      mode,
        cpl_size            llx,
        cpl_size            lly,
        cpl_size            urx,
        cpl_size            ury)
{
    /* Allocate stat object */
    cpl_stats  * self = cpl_malloc(sizeof(cpl_stats));
    const cpl_error_code code =
        cpl_stats_fill_from_image_window(self, image, mode, llx, lly, urx, ury);

    if (code) {
        (void)cpl_error_set_where_();
        cpl_free(self);
        self = NULL;
    }

    return self;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Compute various statistics of an image sub-window.
  @param    self        Pre-allocated struct to fill
  @param    image       Input image
  @param    mode        Bit field specifying which statistics to compute
  @return   CPL_ERROR_NONE or the relevant the #_cpl_error_code_
  @see cpl_stats_new_from_image_window()
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_stats_fill_from_image(cpl_stats * self,
                                         const cpl_image *   image,
                                         cpl_stats_mode      mode)
{
    const cpl_error_code code =
        cpl_stats_fill_from_image_window(self, image, mode, 1, 1,
                                         cpl_image_get_size_x(image),
                                         cpl_image_get_size_y(image));

    return code ? cpl_error_set_where_() : CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Compute various statistics of an image sub-window.
  @param    self        Pre-allocated struct to fill
  @param    image       Input image.
  @param    mode        Bit field specifying which statistics to compute
  @param    llx         Lower left x position (FITS convention)
  @param    lly         Lower left y position (FITS convention)
  @param    urx         Upper right x position (FITS convention)
  @param    ury         Upper right y position (FITS convention)
  @return   CPL_ERROR_NONE or the relevant the #_cpl_error_code_
  @see cpl_stats_new_from_image_window()
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_stats_fill_from_image_window(cpl_stats * self,
                                                const cpl_image *   image,
                                                cpl_stats_mode      mode,
                                                cpl_size            llx,
                                                cpl_size            lly,
                                                cpl_size            urx,
                                                cpl_size            ury)
{
    const size_t llxsz = (size_t)llx;
    const size_t llysz = (size_t)lly;
    const size_t urxsz = (size_t)urx;
    const size_t urysz = (size_t)ury;
    double         pix_sum = 0.0;
    double         sqr_sum = 0.0;
    double         abs_sum = 0.0;
    double         dev_sum = 0.0;
    double         pix_mean = 0.0;
    double         pix_var = 0.0; /* The accumulated variance sum */
    double         max_pix = DBL_MAX; /* Avoid (false) uninit warning */
    double         min_pix = DBL_MAX; /* Avoid (false) uninit warning */
    cpl_size       max_pos = -1; /* Avoid (false) uninit warning */
    cpl_size       min_pos = -1; /* Avoid (false) uninit warning */
    double         ipix    = 0.0; /* Counter of pixels used */
    cpl_size       npix;
    const cpl_size mpix = (urx-llx+1) * (ury-lly+1);
    cpl_size       pos;
    cpl_ifalloc    copybuf;
    /* Two statistics computation categories defined here */
    const cpl_stats_mode minmax_cat = mode &
        (CPL_STATS_MIN|CPL_STATS_MAX|CPL_STATS_MINPOS|
         CPL_STATS_MAXPOS|CPL_STATS_CENTROID);
    const cpl_stats_mode flux_cat = mode &
        (CPL_STATS_FLUX | CPL_STATS_ABSFLUX | CPL_STATS_SQFLUX);
    const cpl_stats_mode variance_cat = mode &
        (CPL_STATS_MEAN | CPL_STATS_STDEV);

    /* Index of 1st good pixel - used for initialization */
    cpl_size            firstgoodpos;
    /* The number of bad pixels inside the subwindow */
    cpl_size            nbadpix;
    /* A map of the the bad pixels in the input */
    const cpl_binary  * badmap;
    size_t              i, j;

    /* Test inputs */
    cpl_ensure_code(image != NULL,    CPL_ERROR_NULL_INPUT);

    cpl_ensure_code(llx >  0,         CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_ensure_code(lly >  0,         CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_ensure_code(urx <= image->nx, CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_ensure_code(ury <= image->ny, CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_ensure_code(llx <= urx,       CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(lly <= ury,       CPL_ERROR_ILLEGAL_INPUT);

    cpl_ensure_code(mode != 1, CPL_ERROR_INVALID_TYPE);
    cpl_ensure_code(mode != 0, CPL_ERROR_UNSUPPORTED_MODE);
    cpl_ensure_code((mode & ~CPL_STATS_ALL) == 0, CPL_ERROR_UNSUPPORTED_MODE);

    /* Need the median for the mean and median median absolute deviation */
    if (mode & (CPL_STATS_MEDIAN_DEV | CPL_STATS_MAD))
        mode |= CPL_STATS_MEDIAN;

    /* Get the bad pixels map */
    badmap = image->bpm == NULL ? NULL : cpl_mask_get_data_const(image->bpm);

    /* Get the first good pixel and the number of bad pixels */
    nbadpix = 0;
    if (badmap != NULL) {
        const cpl_binary * bpmj1 = badmap + (lly-1) * image->nx - 1;

        firstgoodpos = -1;
        for (j = llysz; j < 1 + urysz; j++, bpmj1 += image->nx) {
            for (i = llxsz; i < 1 + urxsz; i++) {
                if (bpmj1[i] == CPL_BINARY_1) {
                    nbadpix++;
                } else if (firstgoodpos < 0) {
                    firstgoodpos = (i - 1) + (j - 1) * image->nx;
                }
            }
        }

        /* Verify that there are good pixels */
        cpl_ensure_code(firstgoodpos >= 0, CPL_ERROR_DATA_NOT_FOUND);

    } else {
        firstgoodpos = (llx-1)+(lly-1)*image->nx;
    }
    npix = mpix - nbadpix;

    /* When a member of the struct is accessed check that it was initialized */
    self->mode = mode;

    self->npix = npix;

    /* Code duplication not avoidable for performance reasons */
    /* The tests should stay outside the loops */

    if (mode & CPL_STATS_MEDIAN) {
        /* Duplicate the pixels inside the window - maybe reused below */
        cpl_ifalloc_set(&copybuf,
                        (size_t)npix * cpl_type_get_sizeof(image->type));
        switch (image->type) {
#define CPL_OPERATION CPL_IMAGE_STATS_MEDIAN
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_stats_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_stats_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_INT
#include "cpl_stats_body.h"
#undef CPL_CLASS
#undef CPL_OPERATION
        default:
            /* See comment in previous switch() default: */
            cpl_ifalloc_free(&copybuf);
            return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        }
    } else {
        cpl_ifalloc_set(&copybuf, 0); /* Initialize for deallocation */
    }

    if (mode == CPL_STATS_ALL) {
        /* Switch on image type */
        switch (image->type) {
#define CPL_OPERATION CPL_IMAGE_STATS_ALL
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_stats_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_stats_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_INT
#include "cpl_stats_body.h"
#undef CPL_CLASS
#undef CPL_OPERATION
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
            cpl_ifalloc_free(&copybuf);
            return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        }

    } else {
        if (minmax_cat) {
            /* Switch on image type */
            switch (image->type) {
#define CPL_OPERATION CPL_IMAGE_STATS_MINMAX
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_stats_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_stats_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_INT
#include "cpl_stats_body.h"
#undef CPL_CLASS
#undef CPL_OPERATION
            default:
                /* See comment in previous switch() default: */
                cpl_ifalloc_free(&copybuf);
                return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            }

        }
        if (flux_cat) {
            /* Switch on image type */
            switch (image->type) {
#define CPL_OPERATION CPL_IMAGE_STATS_FLUX
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_stats_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_stats_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_INT
#include "cpl_stats_body.h"
#undef CPL_CLASS
#undef CPL_OPERATION
            default:
                /* See comment in previous switch() default: */
                cpl_ifalloc_free(&copybuf);
                return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            }
        }
        if (variance_cat) {
            /* Switch on image type */
            switch (image->type) {
#define CPL_OPERATION CPL_IMAGE_STATS_VARIANCE
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_stats_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_stats_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_INT
#include "cpl_stats_body.h"
#undef CPL_CLASS
#undef CPL_OPERATION
            default:
                /* See comment in previous switch() default: */
                cpl_ifalloc_free(&copybuf);
                return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            }
        }

        if (mode & CPL_STATS_MEDIAN_DEV) {
            switch (image->type) {
#define CPL_OPERATION CPL_IMAGE_STATS_MEDIAN_DEV
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_stats_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_stats_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_INT
#include "cpl_stats_body.h"
#undef CPL_CLASS
#undef CPL_OPERATION
            default:
                /* See comment in previous switch() default: */
                cpl_ifalloc_free(&copybuf);
                return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            }
        }
    }
    if (mode & CPL_STATS_MAD) {
        switch (image->type) {
#define CPL_OPERATION CPL_IMAGE_STATS_MAD
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_stats_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_stats_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_INT
#include "cpl_stats_body.h"
#undef CPL_CLASS
#undef CPL_OPERATION
        default:
            /* See comment in previous switch() default: */
            cpl_ifalloc_free(&copybuf);
            return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        }
    }

    if (mode & CPL_STATS_MIN)  self->min  = min_pix;
    if (mode & CPL_STATS_MAX)  self->max  = max_pix;
    if (mode & CPL_STATS_MEAN) self->mean = pix_mean;

    if (mode &CPL_STATS_STDEV) {

        /* Compute the bias-corrected standard deviation. */
        self->stdev = npix < 2 ? 0.0 : sqrt(pix_var/(double)(npix-1));
    }

    if (mode &CPL_STATS_CENTROID) {
        double sum_xz  = 0.0;
        double sum_yz  = 0.0;
        double sum_z   = 0.0;
        double sum_x   = 0.0;
        double sum_y   = 0.0;

        switch (image->type) {
#define CPL_OPERATION CPL_IMAGE_STATS_CENTROID
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_stats_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_stats_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_INT
#include "cpl_stats_body.h"
#undef CPL_CLASS
#undef CPL_OPERATION
          default:
              /* See comment in previous switch() default: */
              cpl_ifalloc_free(&copybuf);
              return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        }
        if (sum_z > 0.0) {
            self->centroid_x =  sum_xz / sum_z;
            self->centroid_y =  sum_yz / sum_z;
        } else {
            self->centroid_x = sum_x / (double)npix;
            self->centroid_y = sum_y / (double)npix;
        }
        /* The centroid has to be inside the provided sub-window */
        /* It can only fail to be that due to round-off, for example
           due to compiler optimization (apparently because sum_z is
           made into a register variable) */
        /* The round-off is especially likely to happen on a 1D-image,
           e.g. when lly == ury */

        if (self->centroid_x < (double)llx) {
            assert( (double)llx - self->centroid_x < FLT_EPSILON );
            self->centroid_x = (double)llx;
        } else if (self->centroid_x > (double)urx) {
            assert( self->centroid_x - (double)urx < FLT_EPSILON );
            self->centroid_x = (double)urx;
        }
        if (self->centroid_y < (double)lly) {
            assert( (double)lly - self->centroid_y < FLT_EPSILON );
            self->centroid_y = (double)lly;
        } else if (self->centroid_y > (double)ury) {
            assert( self->centroid_y - (double)ury < FLT_EPSILON );
            self->centroid_y = (double)ury;
        }
    }
    if (mode &CPL_STATS_FLUX) self->flux = pix_sum;
    if (mode &CPL_STATS_ABSFLUX) self->absflux = abs_sum;
    if (mode &CPL_STATS_SQFLUX) self->sqflux = sqr_sum;
    if (mode &CPL_STATS_MINPOS) {
        self->min_x = 1 + min_pos % image->nx;
        self->min_y = 1 + min_pos / image->nx;
    }
    if (mode &CPL_STATS_MAXPOS) {
        self->max_x = 1 + max_pos % image->nx;
        self->max_y = 1 + max_pos / image->nx;
    }

    if (mode & CPL_STATS_MEDIAN_DEV) self->med_dev = dev_sum / (double) npix;

    cpl_ifalloc_free(&copybuf);
    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Compute various statistics of an image.
  @param    image    input image.
  @param    mode     Bit field specifying which statistics to compute
  @return   1 newly allocated cpl_stats structure or NULL in error case
  @see      cpl_stats_new_from_image_window()
 */
/*----------------------------------------------------------------------------*/
cpl_stats * cpl_stats_new_from_image(const cpl_image * image,
                                     cpl_stats_mode    mode)
{
    cpl_stats * self;

    cpl_ensure(image != NULL, CPL_ERROR_NULL_INPUT, NULL);
    self = cpl_stats_new_from_image_window(image, mode, 1, 1, image->nx,
                                           image->ny);

    /* Propagate error, if any */
    if (self == NULL) (void)cpl_error_set_where_();

    return self;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Dump a cpl_stats object
  @param    self     cpl_stats object to dump
  @param    mode     Bit field specifying which statistics to dump
  @param    stream   The output stream
  @return   CPL_ERROR_NONE or the relevant the #_cpl_error_code_
  @see      cpl_stats_new_from_image_window()

  It is an error to request parameters that have not been set.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if mode specifies statistics that have not been
    computed
  - CPL_ERROR_FILE_IO if the write fails
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_stats_dump(const cpl_stats * self,
                              cpl_stats_mode    mode,
                              FILE            * stream)
{

    cpl_ensure_code(self   != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(stream != NULL, CPL_ERROR_NULL_INPUT);

    cpl_ensure_code((mode & ~self->mode) == 0, CPL_ERROR_ILLEGAL_INPUT);

    cpl_ensure_code( fprintf(stream, "\t\tPixel count: %" CPL_SIZE_FORMAT "\n",
                             self->npix) >0, CPL_ERROR_FILE_IO);

    CPL_STATS_DUMP_ONE(CPL_STATS_MIN,        min,      "Min",        ".8g");
    CPL_STATS_DUMP_ONE(CPL_STATS_MAX,        max,      "Max",        ".8g");
    CPL_STATS_DUMP_ONE(CPL_STATS_MEAN,       mean,     "Mean",       ".8g");
    CPL_STATS_DUMP_ONE(CPL_STATS_MEDIAN,     med,      "Median",     ".8g");
    CPL_STATS_DUMP_ONE(CPL_STATS_MEDIAN_DEV, med_dev,  "Median dev", ".8g");
    CPL_STATS_DUMP_ONE(CPL_STATS_MAD,        mad,      "MAD",        ".8g");
    CPL_STATS_DUMP_ONE(CPL_STATS_STDEV,      stdev,    "Std. dev",   ".8g");
    CPL_STATS_DUMP_ONE(CPL_STATS_FLUX,       flux,     "Flux",       ".8g");
    CPL_STATS_DUMP_ONE(CPL_STATS_ABSFLUX,    absflux,  "Abs flux",   ".8g");
    CPL_STATS_DUMP_ONE(CPL_STATS_SQFLUX,     sqflux,   "Sq. flux",   ".8g");

    CPL_STATS_DUMP_TWO(CPL_STATS_CENTROID,   centroid, "centroid",   ".8g");
    CPL_STATS_DUMP_TWO(CPL_STATS_MINPOS,     min,      "min. pos.",
                       CPL_SIZE_FORMAT);
    CPL_STATS_DUMP_TWO(CPL_STATS_MAXPOS,     max,      "max. pos.",
                       CPL_SIZE_FORMAT);

    /* Failure here means a new member has been added, without dump support */
    return mode ? cpl_error_set_(CPL_ERROR_UNSUPPORTED_MODE) : CPL_ERROR_NONE;
}

/**@}*/

