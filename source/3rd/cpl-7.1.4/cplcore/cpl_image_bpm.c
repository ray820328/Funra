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

#include "cpl_image_bpm.h"
#include "cpl_tools.h"

#include "cpl_error_impl.h"
#include "cpl_image_defs.h"

#include <string.h>
/* Needed for fpclassify() */
#include <math.h>

/*-----------------------------------------------------------------------------
                                   Defines
 -----------------------------------------------------------------------------*/

/* These macros are needed for support of the different pixel types */

#define CONCAT(a,b) a ## _ ## b
#define CONCAT2X(a,b) CONCAT(a,b)

#define ADDTYPE(a) CONCAT2X(a, CPL_TYPE_NAME)

/*-----------------------------------------------------------------------------
                            Static Function Prototypes
 -----------------------------------------------------------------------------*/

#define CPL_TYPE double
#define CPL_TYPE_NAME double
#include "cpl_image_bpm_body.h"
#undef CPL_TYPE
#undef CPL_TYPE_NAME

#define CPL_TYPE float
#define CPL_TYPE_NAME float
#include "cpl_image_bpm_body.h"
#undef CPL_TYPE
#undef CPL_TYPE_NAME

#ifdef CPL_FPCLASSIFY_COMPLEX
#define CPL_TYPE double complex
#define CPL_TYPE_NAME double_complex
#include "cpl_image_bpm_body.h"
#undef CPL_TYPE
#undef CPL_TYPE_NAME

#define CPL_TYPE float complex
#define CPL_TYPE_NAME float_complex
#include "cpl_image_bpm_body.h"
#undef CPL_TYPE
#undef CPL_TYPE_NAME
#endif

#define CPL_TYPE int
#define CPL_TYPE_NAME int
#define CPL_TYPE_IS_INT
#include "cpl_image_bpm_body.h"
#undef CPL_TYPE
#undef CPL_TYPE_NAME
#undef CPL_TYPE_IS_INT

/*-----------------------------------------------------------------------------
                            Function codes
 -----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Reject pixels with the specified special value(s)
  @param    self   Input image to modify
  @param    mode   Bit field specifying which special value(s) to reject

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INVALID_TYPE if mode is 1, e.g. due to a logical or (||)
      of the allowed options or if the pixel type is complex
  - CPL_ERROR_UNSUPPORTED_MODE if mode is otherwise different from the
       allowed options.
*/
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_reject_value(cpl_image * self, cpl_value mode)
{

    cpl_error_code error = CPL_ERROR_NONE;

    switch (cpl_image_get_type(self)) {
    case CPL_TYPE_DOUBLE:
        error = cpl_image_reject_value_double(self, mode);
        break;
    case CPL_TYPE_FLOAT:
        error = cpl_image_reject_value_float(self, mode);
        break;
#ifdef CPL_FPCLASSIFY_COMPLEX
    case CPL_TYPE_DOUBLE_COMPLEX:
        error = cpl_image_reject_value_double_complex(self, mode);
        break;
    case CPL_TYPE_FLOAT_COMPLEX:
        error = cpl_image_reject_value_float_complex(self, mode);
        break;
#endif
    case CPL_TYPE_INT:
        error = cpl_image_reject_value_int(self, mode);
        break;
    default:
        /* NULL input and unsupported pixel types */
        error = self != NULL ? CPL_ERROR_INVALID_TYPE : CPL_ERROR_NULL_INPUT;
        break;
    }

    return error ? cpl_error_set_(error) : CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Test if a pixel is good or bad
  @param    im  the input image
  @param    x   the x pixel position in the image (first pixel is 1)
  @param    y   the y pixel position in the image (first pixel is 1)
  @return   1 if the pixel is bad, 0 if the pixel is good, negative on error.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ACCESS_OUT_OF_RANGE if the specified position is out of
    the image
 */
/*----------------------------------------------------------------------------*/
int cpl_image_is_rejected(const cpl_image * im,
                          cpl_size          x,
                          cpl_size          y)
{

    /* Test entries */
    cpl_ensure(im != NULL,  CPL_ERROR_NULL_INPUT, -1);
    cpl_ensure(x >= 1,      CPL_ERROR_ACCESS_OUT_OF_RANGE, -2);
    cpl_ensure(y >= 1,      CPL_ERROR_ACCESS_OUT_OF_RANGE, -3);
    cpl_ensure(x <= im->nx, CPL_ERROR_ACCESS_OUT_OF_RANGE, -4);
    cpl_ensure(y <= im->ny, CPL_ERROR_ACCESS_OUT_OF_RANGE, -5);

    /* Get the pixel info */
    if (im->bpm != NULL) {
        const cpl_binary val = cpl_mask_get(im->bpm, x, y);
        return val == CPL_BINARY_0 ? 0 : 1;
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Count the number of bad pixels declared in an image
  @param    im  the input image
  @return   the number of bad pixels or -1 if the input image is NULL

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_image_count_rejected(const cpl_image * im)
{

    cpl_ensure(im != NULL, CPL_ERROR_NULL_INPUT, -1);

    return im->bpm == NULL ? 0 : cpl_mask_count(im->bpm);
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @internal
  @brief    Set a pixel as bad in an image
  @param    im  the input image
  @param    x   the x pixel position in the image (first pixel is 1)
  @param    y   the y pixel position in the image (first pixel is 1)
  @return   nothing
  @see cpl_image_accept_()
  @note No input validation in this function

 */
/*----------------------------------------------------------------------------*/
void cpl_image_reject_(cpl_image * im,
                       cpl_size    x,
                       cpl_size    y)
{
    cpl_binary  *   pbpm;

    /* Create the bad pixels mask if empty */
    if (im->bpm == NULL) im->bpm = cpl_mask_new(im->nx, im->ny);

    /* Get the access to the data */
    pbpm = cpl_mask_get_data(im->bpm);

    /* Set the bad pixel */
    pbpm[(x-1) + (y-1) * im->nx] = CPL_BINARY_1;

}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Set a pixel as bad in an image
  @param    im  the input image
  @param    x   the x pixel position in the image (first pixel is 1)
  @param    y   the y pixel position in the image (first pixel is 1)
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ACCESS_OUT_OF_RANGE if the specified position is out of
  the image
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_reject(cpl_image * im,
                                cpl_size    x,
                                cpl_size    y)
{
    /* Test entries */
    cpl_ensure_code(im != NULL,  CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(x >= 1,      CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_ensure_code(y >= 1,      CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_ensure_code(x <= im->nx, CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_ensure_code(y <= im->ny, CPL_ERROR_ACCESS_OUT_OF_RANGE);

    cpl_image_reject_(im, x, y);

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @internal
  @brief    Set a pixel as good in an image
  @param    im  the input image
  @param    x   the x pixel position in the image (first pixel is 1)
  @param    y   the y pixel position in the image (first pixel is 1)
  @return   nothing
  @see cpl_image_accept_()
  @note No input validation in this function

 */
/*----------------------------------------------------------------------------*/
void cpl_image_accept_(cpl_image * im,
                       cpl_size    x,
                       cpl_size    y)
{
    if (im->bpm != NULL) {
        /* Get the access to the data */
        cpl_binary * pbpm = cpl_mask_get_data(im->bpm);

        /* Set the good pixel */
        pbpm[(x-1) + (y-1) * im->nx] = CPL_BINARY_0;
    }
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Set a pixel as good in an image
  @param    im  the input image
  @param    x   the x pixel position in the image (first pixel is 1)
  @param    y   the y pixel position in the image (first pixel is 1)
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ACCESS_OUT_OF_RANGE if the specified position is out of
  the image
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_accept(cpl_image * im,
                                cpl_size    x,
                                cpl_size    y)
{
    /* Test entries */
    cpl_ensure_code(im != NULL,  CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(x >= 1,      CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_ensure_code(y >= 1,      CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_ensure_code(x <= im->nx, CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_ensure_code(y <= im->ny, CPL_ERROR_ACCESS_OUT_OF_RANGE);

    cpl_image_accept_(im, x, y);

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Set all pixels in the image as good
  @param    self  the input image
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_accept_all(cpl_image * self)
{

    cpl_mask_delete(cpl_image_unset_bpm(self));

    return self != NULL ? CPL_ERROR_NONE : cpl_error_set_where_();
}


/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Set the bad pixels in an image as defined in a mask
  @param    im  the input image
  @param    map the mask defining the bad pixels
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE

  If the input image has a bad pixel map prior to the call, it is overwritten.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if the input image or the input map is NULL
  - CPL_ERROR_INCOMPATIBLE_INPUT if the image and the map have different sizes
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_reject_from_mask(cpl_image      * im,
                                          const cpl_mask * map)
{

    /* Test entries */
    cpl_ensure_code(im  != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(map != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(im->nx==cpl_mask_get_size_x(map),
                    CPL_ERROR_INCOMPATIBLE_INPUT);
    cpl_ensure_code(im->ny==cpl_mask_get_size_y(map),
                    CPL_ERROR_INCOMPATIBLE_INPUT);

    /* Copy the provided mask - cpl_image_get_bpm(im)
       will create a new pixel-map if one does not exist */
    (void)memcpy(cpl_mask_get_data(cpl_image_get_bpm(im)),
                 cpl_mask_get_data_const(map),
                 (size_t)im->nx * (size_t)im->ny * sizeof(cpl_binary));

    return CPL_ERROR_NONE;
}

