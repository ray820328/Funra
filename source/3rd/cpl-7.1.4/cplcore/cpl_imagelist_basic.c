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
#include <assert.h>

#include <cxstrutils.h>

#include "cpl_memory.h"
#include "cpl_imagelist_basic.h"
#include "cpl_imagelist_io.h"
#include "cpl_image_basic.h"
#include "cpl_image_io.h"
#include "cpl_image_bpm.h"
#include "cpl_stats.h"
#include "cpl_error_impl.h"
#include "cpl_tools.h"
#include "cpl_mask.h"
#include "cpl_imagelist_defs.h"
#include "cpl_image_defs.h"

/*-----------------------------------------------------------------------------
                                   Define
 -----------------------------------------------------------------------------*/

#define CPL_IMLIST_BASIC_OPER               0
#define CPL_IMLIST_BASIC_IMAGE_LOCAL        1
#define CPL_IMLIST_BASIC_TIME_MEDIAN        2
#define CPL_IMLIST_BASIC_TIME_MINMAX        3
#define CPL_IMLIST_BASIC_TIME_SIGCLIP       4
#define CPL_IMLIST_BASIC_SWAP_AXIS          5

/**@{*/

/*-----------------------------------------------------------------------------
                        Private function prototypes
 -----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
                            Function codes
 -----------------------------------------------------------------------------*/

#define CPL_OPERATION CPL_IMLIST_BASIC_OPER
/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Add two image lists, the first one is replaced by the result.
  @param    in1        first input image list (modified)
  @param    in2        image list to add
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
  @see      cpl_image_add()

  The two input lists must have the same size, the image number n in the
  list in2 is added to the image number n in the list in1.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the input images have different sizes
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_imagelist_add(
        cpl_imagelist         *    in1,
        const cpl_imagelist   *    in2)
{
#define CPL_OPERATOR cpl_image_add
#include "cpl_imagelist_basic_body.h"
#undef CPL_OPERATOR
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Subtract two image lists, the first one is replaced by the result.
  @param    in1        first input image list (modified)
  @param    in2        image list to subtract
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
  @see      cpl_image_subtract() 
  @see      cpl_imagelist_add()
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_imagelist_subtract(
        cpl_imagelist         *    in1,
        const cpl_imagelist   *    in2)
{
#define CPL_OPERATOR cpl_image_subtract
#include "cpl_imagelist_basic_body.h"
#undef CPL_OPERATOR
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Multiply two image lists, the first one is replaced by the result.
  @param    in1        first input image list (modified)
  @param    in2        image list to multiply
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
  @see      cpl_image_multiply()
  @see      cpl_imagelist_add()
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_imagelist_multiply(
        cpl_imagelist         *    in1,
        const cpl_imagelist   *    in2)
{
#define CPL_OPERATOR cpl_image_multiply
#include "cpl_imagelist_basic_body.h"
#undef CPL_OPERATOR
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Divide two image lists, the first one is replaced by the result.
  @param    in1        first input image list (modified)
  @param    in2        image list to divide
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
  @see      cpl_image_divide()
  @see      cpl_imagelist_add()
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_imagelist_divide(
        cpl_imagelist         *    in1,
        const cpl_imagelist   *    in2)
{
#define CPL_OPERATOR cpl_image_divide
#include "cpl_imagelist_basic_body.h"
#undef CPL_OPERATOR
}
#undef CPL_OPERATION

#define CPL_OPERATION CPL_IMLIST_BASIC_IMAGE_LOCAL
/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Add an image to an image list.
  @param    imlist  input image list (modified)
  @param    img     image to add
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
  @see      cpl_image_add()

  The passed image is added to each image of the passed image list.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_imagelist_add_image(
        cpl_imagelist   *    imlist,
        const cpl_image *    img)
{
#define CPL_OPERATOR cpl_image_add
#include "cpl_imagelist_basic_body.h"
#undef CPL_OPERATOR
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Subtract an image from an image list.
  @param    imlist    input image list (modified)
  @param    img        image to subtract
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
  @see      cpl_image_subtract()
  @see      cpl_imagelist_add_image()
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_imagelist_subtract_image(
        cpl_imagelist   *    imlist,
        const cpl_image *    img)
{
#define CPL_OPERATOR cpl_image_subtract
#include "cpl_imagelist_basic_body.h"
#undef CPL_OPERATOR
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Multiply an image list by an image.
  @param    imlist    input image list (modified)
  @param    img     image to multiply
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
  @see      cpl_image_multiply()
  @see      cpl_imagelist_add_image()
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_imagelist_multiply_image(
        cpl_imagelist   *    imlist,
        const cpl_image *    img)
{
#define CPL_OPERATOR cpl_image_multiply
#include "cpl_imagelist_basic_body.h"
#undef CPL_OPERATOR
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Divide an image list by an image.
  @param    imlist    input image list (modified)
  @param    img     image for division
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
  @see      cpl_image_divide()
  @see      cpl_imagelist_add_image()
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_imagelist_divide_image(
        cpl_imagelist   *    imlist,
        const cpl_image *    img)
{
#define CPL_OPERATOR cpl_image_divide
#include "cpl_imagelist_basic_body.h"
#undef CPL_OPERATOR
}
#undef CPL_OPERATION

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Elementwise addition of a scalar to each image in the imlist
  @param    imlist  Imagelist to be modified in place.
  @param    addend  Number to add
  @return   CPL_ERROR_NONE or the relevant the #_cpl_error_code_ on error
  @see      cpl_image_add_scalar()
 
  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_imagelist_add_scalar(
        cpl_imagelist * imlist,
        double          addend)
{
    cpl_size     i;

    /* Check inputs */
    cpl_ensure_code(imlist != NULL, CPL_ERROR_NULL_INPUT);
        
    for (i=0; i < imlist->ni; i++)
        cpl_ensure_code( !cpl_image_add_scalar(imlist->images[i], addend),
                         cpl_error_get_code());

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Elementwise subtraction of a scalar from each image in the imlist
  @param    imlist   Imagelist to be modified in place.
  @param    subtrahend  Number to subtract
  @return   CPL_ERROR_NONE or the relevant the #_cpl_error_code_ on error
  @see      cpl_imagelist_add_scalar()
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_imagelist_subtract_scalar(
        cpl_imagelist * imlist,
        double          subtrahend)
{
    cpl_size     i;

    /* Check inputs */
    cpl_ensure_code(imlist != NULL, CPL_ERROR_NULL_INPUT);
        
    for (i=0; i < imlist->ni; i++)
        cpl_ensure_code(!cpl_image_subtract_scalar(imlist->images[i],subtrahend),
                         cpl_error_get_code());

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Elementwise multiplication of the imlist with a scalar
  @param    imlist  Imagelist to be modified in place.
  @param    factor     Number to multiply with
  @return   CPL_ERROR_NONE or the relevant the #_cpl_error_code_ on error
  @see      cpl_imagelist_add_scalar()
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_imagelist_multiply_scalar(
        cpl_imagelist * imlist,
        double          factor)
{
    cpl_size     i;

    /* Check inputs */
    cpl_ensure_code(imlist != NULL, CPL_ERROR_NULL_INPUT);
        
    for (i=0; i < imlist->ni; i++)
        cpl_ensure_code( !cpl_image_multiply_scalar(imlist->images[i], factor),
                         cpl_error_get_code());

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Elementwise division of each image in the imlist with a scalar
  @param    imlist  Imagelist to be modified in place.
  @param    divisor    Non-zero number to divide with
  @return   CPL_ERROR_NONE or the relevant the #_cpl_error_code_ on error
  @see      cpl_imagelist_add_scalar()
 
  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_imagelist_divide_scalar(
        cpl_imagelist * imlist,
        double          divisor)
{
    cpl_size     i;

    /* Check inputs */
    cpl_ensure_code(imlist != NULL, CPL_ERROR_NULL_INPUT);
        
    for (i=0; i < imlist->ni; i++)
        cpl_ensure_code( !cpl_image_divide_scalar(imlist->images[i], divisor),
                         cpl_error_get_code());

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Compute the elementwise logarithm of each image in the imlist
  @param    imlist  Imagelist to be modified in place.
  @param    base       Base of the logarithm.
  @return   CPL_ERROR_NONE or the relevant the #_cpl_error_code_ on error
  @see      cpl_image_logarithm()
 
  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_imagelist_logarithm(
        cpl_imagelist * imlist,
        double          base)
{
    cpl_size     i;

    /* Check inputs */
    cpl_ensure_code(imlist != NULL, CPL_ERROR_NULL_INPUT);
        
    for (i=0; i < imlist->ni; i++)
        cpl_ensure_code( !cpl_image_logarithm(imlist->images[i], base),
                         cpl_error_get_code());

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Compute the elementwise exponential of each image in the imlist
  @param    imlist  Imagelist to be modified in place.
  @param    base       Base of the exponential.
  @return   CPL_ERROR_NONE or the relevant the #_cpl_error_code_ on error
  @see      cpl_image_exponential()
 
  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_imagelist_exponential(
        cpl_imagelist * imlist,
        double          base)
{
    cpl_size     i;

    /* Check inputs */
    cpl_ensure_code(imlist != NULL, CPL_ERROR_NULL_INPUT);
        
    for (i=0; i < imlist->ni; i++)
        cpl_ensure_code( !cpl_image_exponential(imlist->images[i], base),
                         cpl_error_get_code());

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Compute the elementwise power of each image in the imlist
  @param    imlist   Imagelist to be modified in place.
  @param    exponent Scalar exponent
  @return   CPL_ERROR_NONE or the relevant the #_cpl_error_code_ on error
  @see      cpl_image_power()
 
  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_imagelist_power(
        cpl_imagelist * imlist,
        double          exponent)
{
    cpl_size     i;

    /* Check inputs */
    cpl_ensure_code(imlist != NULL, CPL_ERROR_NULL_INPUT);
        
    for (i=0; i < imlist->ni; i++)
        cpl_ensure_code( !cpl_image_power(imlist->images[i], exponent),
                         cpl_error_get_code());

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Normalize each image in the list.
  @param    imlist  Imagelist to modify.
  @param    mode    Normalization mode.
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error
  @see      cpl_image_normalise()

  The list may be partly modified if an error occurs.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_imagelist_normalise(
        cpl_imagelist *  imlist,
        cpl_norm         mode)
{
    cpl_size            i;

    /* Check inputs */
    cpl_ensure_code(imlist != NULL, CPL_ERROR_NULL_INPUT);
    
    for (i=0; i<imlist->ni; i++)
        cpl_ensure_code(!cpl_image_normalise(imlist->images[i], mode), 
                        cpl_error_get_code());

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Threshold all pixel values to an interval.
  @param    imlist               Image list to threshold.
  @param    lo_cut          Lower bound.
  @param    hi_cut          Higher bound.
  @param    assign_lo_cut   Value to assign to pixels below low bound.
  @param    assign_hi_cut   Value to assign to pixels above high bound.
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error
  @see      cpl_image_threshold()

  Threshold the images of the list using cpl_image_threshold()
  The input image list is modified.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INCOMPATIBLE_INPUT if lo_cut is bigger than hi_cut
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_imagelist_threshold(
        cpl_imagelist * imlist,
        double          lo_cut,
        double          hi_cut,
        double          assign_lo_cut,
        double          assign_hi_cut)
{
    cpl_size                i;

    /* Check inputs */
    cpl_ensure_code(imlist != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(lo_cut < hi_cut, CPL_ERROR_INCOMPATIBLE_INPUT);
    
    for (i=0; i<imlist->ni; i++)
        cpl_ensure_code(!cpl_image_threshold(imlist->images[i], lo_cut, hi_cut,
                                             assign_lo_cut, assign_hi_cut),
                        cpl_error_get_code());

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @internal
  @brief    subtract one from an integer image where the mask is true
  @param    img  output integer image
  @param    mask input mask
 */
/*----------------------------------------------------------------------------*/
static void mask_sub(cpl_image * img, const cpl_mask * mask)
{
    size_t npix = cpl_image_get_size_x(img) * cpl_image_get_size_y(img);
    int * imgd = cpl_image_get_data_int(img);
    const cpl_binary * maskd = cpl_mask_get_data_const(mask);
    for (size_t i = 0; i < npix; i++) {
        imgd[i] -= maskd[i] != CPL_BINARY_0;
    }
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Create a contribution map from the bad pixel maps of the images.
  @param    imlist  The imagelist
  @return   The contributions map (a CPL_TYPE_INT cpl_image) or NULL on error
  @see cpl_imagelist_is_uniform()

  The returned map counts for each pixel the number of good pixels in the list.
  The returned map has to be deallocated with cpl_image_delete().

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the input image list is not valid
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_new_from_accepted(const cpl_imagelist * imlist)
{
    const cpl_image * first = cpl_imagelist_get_const(imlist, 0);
    const cpl_size    nx    = cpl_image_get_size_x(first);
    const cpl_size    ny    = cpl_image_get_size_y(first);
    const size_t      nz    = cpl_imagelist_get_size(imlist);
    cpl_image       * goodsum;
    size_t            i;
    

    cpl_ensure(imlist != NULL, CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(cpl_imagelist_is_uniform(imlist)==0, CPL_ERROR_ILLEGAL_INPUT,
               NULL);

    goodsum = cpl_image_new(nx, ny, CPL_TYPE_INT);
    (void)cpl_image_add_scalar(goodsum, (double)nz); /* Assume all good */


    /* Loop on the images */
    for (i = 0; i < nz; i++) {
        const cpl_mask * bpmap
            = cpl_image_get_bpm_const(cpl_imagelist_get_const(imlist, i));

        if (bpmap != NULL) {
            mask_sub(goodsum, bpmap);
        }
    }

    return goodsum;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Average an imagelist to a single image
  @param    imlist    the input images list
  @return   the average image or NULL on error case.
  @see cpl_imagelist_is_uniform()

  The returned image has to be deallocated with cpl_image_delete().

  The bad pixel maps of the images in the input list are taken into
  account, the result image pixels are flagged as rejected for those
  where there were no good pixel at the same position in the input image list.

  For integer pixel types, the averaging is performed using integer division.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the input image list is not valid
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_imagelist_collapse_create(const cpl_imagelist * imlist)
{
    cpl_image    * avg;
    cpl_image    * contrib;
    cpl_size       i;
    
    /* Check inputs */
    cpl_ensure(imlist != NULL, CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(cpl_imagelist_is_uniform(imlist)==0, CPL_ERROR_ILLEGAL_INPUT,
               NULL);

    /* Should not be able to fail now */
    
    /* Create avg with the first image */
    avg = cpl_image_duplicate(imlist->images[0]);
    cpl_image_fill_rejected(avg, 0.0);
    cpl_image_accept_all(avg);
        
    /* Add all images together */
    for (i=1; i<imlist->ni; i++) {
        cpl_image * tmp_im;

        if (cpl_image_get_bpm_const(imlist->images[i])) {
            tmp_im = cpl_image_duplicate(imlist->images[i]);
            cpl_image_fill_rejected(tmp_im, 0.0);
            cpl_image_accept_all(tmp_im);
        } else {
            tmp_im = imlist->images[i];
        }
        cpl_image_add(avg, tmp_im);
        if (tmp_im != imlist->images[i]) cpl_image_delete(tmp_im);
    }

    /* Compute the contribution map */
    contrib = cpl_image_new_from_accepted(imlist);

    /* Divide by the number of contributions */
    /* Any zero in the contribution map will lead to a bad pixel in avg */
    /* - bad pixels in avg are zero-valued due to the initial
       cpl_image_fill_rejected() */

    if (cpl_image_divide(avg, contrib)) {
        cpl_image_delete(avg);
        avg = NULL;
        (void)cpl_error_set_where_();
    }

    cpl_image_delete(contrib);

    return avg;    
}

#define CPL_OPERATION CPL_IMLIST_BASIC_TIME_MINMAX
/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Average with rejection an imagelist to a single image
  @param    self  The image list to average
  @param    nlow   Number of low rejected values
  @param    nhigh  Number of high rejected values
  @return   The average image or NULL on error
  @note The returned image has to be deallocated with cpl_image_delete().

  The input images are averaged, for each pixel position the nlow lowest pixels 
  and the nhigh highest pixels are discarded for the average computation.

  The input image list can be of type CPL_TYPE_INT, CPL_TYPE_FLOAT and
  CPL_TYPE_DOUBLE. The created image will be of the same type.

  On success each pixel in the created image is the average of the non-rejected
  values on the pixel position in the input image list.

  For a given pixel position any bad pixels (i.e. values) are handled as
  follows:
  Given n bad values on a given pixel position, n/2 of those values are assumed
  to be low outliers and n/2 of those values are assumed to be high outliers.
  Any low or high rejection will first reject up to n/2 bad values and if more
  values need to be rejected that rejection will take place on the good values.
  This rationale behind this is to allow the rejection of outliers to include
  bad pixels without introducing a bias.
  If for a given pixel all values in the input image list are rejected, the
  resulting pixel is set to zero and flagged as rejected.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an the input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the input image list is not valid or if
    the sum of the rejections is not lower than the number of images or if
    nlow or nhigh is negative
  - CPL_ERROR_INVALID_TYPE if the passed image list type is not supported
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_imagelist_collapse_minmax_create(const cpl_imagelist * self,
                                                 cpl_size              nlow,
                                                 cpl_size              nhigh)
{
    const cpl_image * imgself = cpl_imagelist_get_const(self, 0);
    const cpl_type    type    = cpl_image_get_type(imgself);
    const size_t      ni      = cpl_imagelist_get_size(self);
    const size_t      nx      = cpl_image_get_size_x(imgself);
    const size_t      ny      = cpl_image_get_size_y(imgself);
    const size_t      nrej    = (size_t)(nlow + nhigh);
    cpl_image       * avg     = NULL;

    
    cpl_ensure(self != NULL,  CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(cpl_imagelist_is_uniform(self)==0, CPL_ERROR_ILLEGAL_INPUT,
               NULL);
    cpl_ensure(nlow  >= 0,    CPL_ERROR_ILLEGAL_INPUT, NULL);
    cpl_ensure(nhigh >= 0,    CPL_ERROR_ILLEGAL_INPUT, NULL);
    cpl_ensure(ni    >  nrej, CPL_ERROR_ILLEGAL_INPUT, NULL);
    
    /* Switch on the data type */
    switch (type) {
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_imagelist_basic_body.h"
#undef CPL_CLASS

#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_imagelist_basic_body.h"
#undef CPL_CLASS

#define CPL_CLASS CPL_CLASS_INT
#include "cpl_imagelist_basic_body.h"
#undef CPL_CLASS
        default:
            (void)cpl_error_set_(CPL_ERROR_INVALID_TYPE);
    }

    return avg;
}
#undef CPL_OPERATION

#define CPL_OPERATION CPL_IMLIST_BASIC_TIME_SIGCLIP
/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief  Collapse an imagelist with kappa-sigma-clipping rejection
  @param  self       The input imagelist
  @param  kappalow   kappa-factor for lower clipping threshold
  @param  kappahigh  kappa-factor for upper clipping threshold
  @param  keepfrac   The fraction of values to keep (0.0 < keepfrac <= 1.0)
  @param  mode       Clipping mode, CPL_COLLAPSE_MEAN or CPL_COLLAPSE_MEDIAN
  @param  contrib    Pre-allocated integer-image for contribution map or NULL
  @return The collapsed image or NULL on error case.
  @note The returned image has to be deallocated with cpl_image_delete().

  The collapsing is an iterative process which will stop when it converges
  (i.e. an iteration did not reject any values for a given pixel) or
  when the next iteration would reduce the fraction of values to keep
  to less than or equal to keepfrac.

  A call with keepfrac == 1.0 will thus perform no clipping.

  Supported modes: 
      CPL_COLLAPSE_MEAN:
          The center value of the acceptance range will be the mean.
      CPL_COLLAPSE_MEDIAN:
          The center value of the acceptance range will be the median.
      CPL_COLLAPSE_MEDIAN_MEAN:
          The center value of the acceptance range will be the median in
          the first iteration and in subsequent iterations it will be the
          mean.
 
  For each pixel position the pixels whose value is higher than
  center + kappahigh * stdev or lower than center - kappalow * stdev 
  are discarded for the subsequent center and stdev computation, where center
  is defined according to the clipping mode, and stdev is the standard
  deviation of the values at that pixel position. Since the acceptance
  interval must be non-empty, the sum of kappalow and kappahigh must be
  positive. A typical call has both kappalow and kappahigh positive.

  The minimum number of values that the clipping can select is 2. This is
  because the clipping criterion is based on the sample standard deviation,
  which needs at least two values to be defined. This means that all calls
  with (positive) values of keepfrac less than 2/n will behave the same. To
  ensure that the values in (at least) i planes out of n are kept, keepfrac
  can be set to (i - 0.5) / n, e.g. to keep at least 50 out of 100 values,
  keepfrac can be set to 0.495.

  The output pixel is set to the mean of the non-clipped values, also in the
  median mode. Regardless of the input pixel type, the mean is computed in
  double precision. The result is then cast to the output-pixel type, which
  is identical to the input pixel type.

  The input parameter contrib is optional. It must be either NULL or point to a
  pre-allocated image of type CPL_TYPE_INT and size equal to the images in the 
  imagelist. On success, it will contain the contribution map, i.e.  the number
  of kept (non-clipped) values after the iterative process on every pixel.
  
  Bad pixels are ignored from the start. This means that with a sufficient
  number of bad pixels, the fraction of good values will be less than keepfrac.
  In this case no iteration is performed at all. If there is at least one
  good value available, then the mean will be based on the good value(s). If
  for a given pixel position there are no good values, then that pixel is
  set to zero, rejected as bad and if available the value in the
  contribution map is set to zero.

  The input imagelist can be of type CPL_TYPE_INT, CPL_TYPE_FLOAT and
  CPL_TYPE_DOUBLE.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_DATA_NOT_FOUND
    if there are less than 2 images in the list
  - CPL_ERROR_ILLEGAL_INPUT 
    if the sum of kappalow and kappahigh is non-positive, 
  - CPL_ERROR_ACCESS_OUT_OF_RANGE
    if keepfrac is outside the required interval which is 0.0 < keepfrac <= 1.0
  - CPL_ERROR_TYPE_MISMATCH
    if contrib is non-NULL but not of type CPL_TYPE_INT
  - CPL_ERROR_INCOMPATIBLE_INPUT if contrib is non-NULL but
       of a size incompatible with the input imagelist
  - CPL_ERROR_INVALID_TYPE if the type of the input imagelist is unsupported
  - CPL_ERROR_UNSUPPORTED_MODE if the passed mode is none of the above listed
 */
/*----------------------------------------------------------------------------*/
cpl_image *
cpl_imagelist_collapse_sigclip_create(const cpl_imagelist * self,
                                      double                kappalow,
                                      double                kappahigh,
                                      double                keepfrac,
                                      cpl_collapse_mode     mode,
                                      cpl_image           * contrib)
{
    const cpl_size    ni       = cpl_imagelist_get_size(self);
    const cpl_image * img      = cpl_imagelist_get_const(self, 0);
    const cpl_size    nx       = cpl_image_get_size_x(img);
    const cpl_size    ny       = cpl_image_get_size_y(img);
    cpl_image       * clipped  = NULL;
    int             * pcontrib = NULL;

    /* Check inputs */
    cpl_ensure(self               != NULL, CPL_ERROR_NULL_INPUT,          NULL);
    cpl_ensure(kappalow + kappahigh > 0.0, CPL_ERROR_ILLEGAL_INPUT,       NULL);
    cpl_ensure(keepfrac             > 0.0, CPL_ERROR_ACCESS_OUT_OF_RANGE, NULL);
    cpl_ensure(keepfrac            <= 1.0, CPL_ERROR_ACCESS_OUT_OF_RANGE, NULL);
    cpl_ensure(ni                   > 1,   CPL_ERROR_DATA_NOT_FOUND,      NULL);

    cpl_ensure(cpl_imagelist_is_uniform(self) == 0, CPL_ERROR_ILLEGAL_INPUT,
               NULL);

    cpl_ensure(mode == CPL_COLLAPSE_MEDIAN_MEAN || mode == CPL_COLLAPSE_MEDIAN
               || mode == CPL_COLLAPSE_MEAN,
               CPL_ERROR_UNSUPPORTED_MODE, NULL);

    if (contrib) {
        cpl_ensure(cpl_image_get_size_x(contrib) == nx,
                   CPL_ERROR_ILLEGAL_INPUT, NULL);
        cpl_ensure(cpl_image_get_size_y(contrib) == ny,
                   CPL_ERROR_ILLEGAL_INPUT, NULL);
        cpl_ensure(cpl_image_get_type(contrib)   == CPL_TYPE_INT,
                   CPL_ERROR_ILLEGAL_INPUT, NULL);

        pcontrib = cpl_image_get_data_int(contrib);
    }

    /* Switch on the data type */
    switch (cpl_image_get_type(img)) {
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_imagelist_basic_body.h"
#undef CPL_CLASS

#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_imagelist_basic_body.h"
#undef CPL_CLASS

#define CPL_CLASS CPL_CLASS_INT
#include "cpl_imagelist_basic_body.h"
#undef CPL_CLASS
    default:
        (void)cpl_error_set_(CPL_ERROR_INVALID_TYPE);
    }

    return clipped;
}
#undef CPL_OPERATION

#define CPL_OPERATION CPL_IMLIST_BASIC_TIME_MEDIAN
/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Create a median image from the input imagelist
  @param    self    The input image list
  @return   The median image of the input pixel type or NULL on error
  @note The created image has to be deallocated with cpl_image_delete()

  The input image list can be of type CPL_TYPE_INT, CPL_TYPE_FLOAT and
  CPL_TYPE_DOUBLE.

  On success each pixel in the created image is the median of the values on
  the same pixel position in the input image list. If for a given pixel all
  values in the input image list are rejected, the resulting pixel is set to
  zero and flagged as rejected.

  The median is defined here as the middle value of an odd number of sorted
  samples and for an even number of samples as the mean of the two central
  values. Note that with an even number of samples the median may not be
  among the input samples.

  Also, note that in the case of an even number of integer data, the mean
  value will be computed using integer arithmetic. Cast your integer data
  to a floating point pixel type if that is not the desired behavior.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the input image list is not valid
  - CPL_ERROR_INVALID_TYPE if the passed image list pixel type is not supported
 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_imagelist_collapse_median_create(const cpl_imagelist * self)
{
    cpl_image       * median = NULL;
    const cpl_image * imself = cpl_imagelist_get_const(self, 0);
    const size_t   nx        = (size_t)cpl_image_get_size_x(imself);
    const size_t   ny        = (size_t)cpl_image_get_size_y(imself);
    const cpl_type type      = cpl_image_get_type(imself);
    
    cpl_ensure(self != NULL, CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(cpl_imagelist_is_uniform(self) == 0, CPL_ERROR_ILLEGAL_INPUT,
               NULL);
    
    /* Switch on the data type */
    switch (type) {
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_imagelist_basic_body.h"
#undef CPL_CLASS

#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_imagelist_basic_body.h"
#undef CPL_CLASS

#define CPL_CLASS CPL_CLASS_INT
#include "cpl_imagelist_basic_body.h"
#undef CPL_CLASS
        default:
            (void)cpl_error_set_(CPL_ERROR_INVALID_TYPE);
    }

    return median;    
}
#undef CPL_OPERATION

#define CPL_OPERATION CPL_IMLIST_BASIC_SWAP_AXIS
/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_imagelist
  @brief    Swap the axis of an image list
  @param    ilist   The image list to swap
  @param    mode    The swapping mode      
  @return The swapped image list or NULL in error case
 
  This function is intended for users that want to use the cpl_imagelist
  object as a cube. Swapping the axis would give them access to the
  usual functions in the 3 dimensions. This has the cost that it
  duplicates the memory consumption, which can be a problem for big
  amounts of data. 

  Image list can be CPL_TYPE_INT, CPL_TYPE_FLOAT or CPL_TYPE_DOUBLE.
  The mode can be either CPL_SWAP_AXIS_XZ or CPL_SWAP_AXIS_YZ
  
  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if mode is not equal to one of the possible
    values or if the image list is not valid
  - CPL_ERROR_INVALID_TYPE if the passed image list type is not supported
 */
/*----------------------------------------------------------------------------*/
cpl_imagelist * cpl_imagelist_swap_axis_create(
        const cpl_imagelist *   ilist,
        cpl_swap_axis           mode)    
{
    cpl_imagelist       *   swapped;
    const cpl_image     *   image;
    cpl_image           *   cur_ima;
    const cpl_image     *   old_ima;
    cpl_size                ni, nx, ny;
    cpl_type                type;
    int                     recompute_bpm, nbpm;
    cpl_mask            *   cur_mask;
    cpl_binary          *   pcur_mask;
    cpl_binary          *   pold_mask;
    cpl_size                i, j, k;

    /* Check entries */
    cpl_ensure(ilist != NULL, CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(cpl_imagelist_is_uniform(ilist)==0, CPL_ERROR_ILLEGAL_INPUT,
               NULL);
    cpl_ensure(mode==CPL_SWAP_AXIS_XZ || mode==CPL_SWAP_AXIS_YZ, 
            CPL_ERROR_ILLEGAL_INPUT, NULL);

    /* Initialise */
    recompute_bpm = 0;
    image = cpl_imagelist_get_const(ilist, 0);
    ni = cpl_imagelist_get_size(ilist);
    nx = cpl_image_get_size_x(image);
    ny = cpl_image_get_size_y(image);
    type = cpl_image_get_type(image);

    /* Create the new image list */
    swapped = cpl_imagelist_new();

    /* Switch on image type */
    switch (type) {
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_imagelist_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_imagelist_basic_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_INT
#include "cpl_imagelist_basic_body.h"
#undef CPL_CLASS
        default:
            (void)cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            return NULL;
    }

    /* HANDLE THE BAD PIXELS MASKS */
    /* Check if there are bad pixels in the input image list */
    for (i=0; i<ni; i++) {
        image = cpl_imagelist_get_const(ilist, i);
        if (image->bpm != NULL) {
            recompute_bpm = 1;
            break;
        }
    }

    /* Need to compute the new bad pixels masks */
    if (recompute_bpm == 1) {
        /* SWAP X <-> Z */
        if (mode == CPL_SWAP_AXIS_XZ) {
            for (i=0; i<nx; i++) {
                cur_mask = cpl_mask_new(ni, ny);
                pcur_mask = cpl_mask_get_data(cur_mask);
                nbpm = 0;
                for (j=0; j<ni; j++) {
                    image = cpl_imagelist_get_const(ilist, j);
                    if (image->bpm != NULL) {
                        pold_mask = cpl_mask_get_data(image->bpm);
                        for (k=0; k<ny; k++) {
                            if (pold_mask[i+k*nx] == CPL_BINARY_1) {
                                pcur_mask[j+k*ni]=pold_mask[i+k*nx];
                                nbpm = 1;
                            }
                        }
                    }
                }

                /* Put the new bpm in the new image */
                if (nbpm > 0) {
                    cpl_image * swap = cpl_imagelist_get(swapped, i);
                    cpl_image_reject_from_mask(swap, cur_mask);
                }
                cpl_mask_delete(cur_mask);
            }
        } else {
        /* SWAP Y <-> Z */
            for (i=0; i<ny; i++) {
                cur_mask = cpl_mask_new(nx, ni);
                pcur_mask = cpl_mask_get_data(cur_mask);
                nbpm = 0;
                for (j=0; j<ni; j++) {
                    image = cpl_imagelist_get_const(ilist, j);
                    if (image->bpm != NULL) {
                        pold_mask = cpl_mask_get_data(image->bpm);
                        for (k=0; k<nx; k++) {
                            if (pold_mask[k+i*nx] == CPL_BINARY_1) {
                                pcur_mask[k+j*ni]=pold_mask[k+i*nx];
                                nbpm = 1;
                            }
                        }
                    }
                }

                /* Put the new bpm in the new image */
                if (nbpm > 0) {
                    cpl_image * swap = cpl_imagelist_get(swapped, i);
                    cpl_image_reject_from_mask(swap, cur_mask);
                }
                cpl_mask_delete(cur_mask);
            }
        }
    }
    return swapped;
}
#undef CPL_OPERATION

/**@}*/
