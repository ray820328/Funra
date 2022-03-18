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

#include "cpl_tools.h"
#include "cpl_memory.h"
#include "cpl_image_resample.h"
#include "cpl_image_io_impl.h"
#include "cpl_mask.h"
#include "cpl_error_impl.h"

#include "cpl_image_defs.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <float.h>

/*-----------------------------------------------------------------------------
                                   Defines
 -----------------------------------------------------------------------------*/

#define CPL_IMAGE_RESAMPLE_SUBSAMPLE    1
#define CPL_IMAGE_WARP                  2
#define CPL_IMAGE_WARP_POLYNOMIAL       3
#define CPL_IMAGE_REBIN                 4

#define CONCAT(a,b) a ## _ ## b
#define CONCAT2X(a,b) CONCAT(a,b)


/*-----------------------------------------------------------------------------
                        Private function prototypes
 -----------------------------------------------------------------------------*/

inline static double cpl_image_get_interpolated_double(const double *,
                                                       const cpl_binary *,
                                                       cpl_size, cpl_size,
                                                       double, double,
                                                       double, double,
                                                       double *,
                                                       const double *, double,
                                                       const double *, double,
                                                       double, double,
                                                       double *)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(1,9,10,12,16)))
#endif
    ;

inline static double cpl_image_get_interpolated_float(const float *,
                                                      const cpl_binary *,
                                                      cpl_size, cpl_size,
                                                      double, double,
                                                      double, double,
                                                      double *,
                                                      const double *, double,
                                                      const double *, double,
                                                      double, double,
                                                      double *)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(1,9,10,12,16)))
#endif
    ;

inline static double cpl_image_get_interpolated_int(const int *,
                                                    const cpl_binary *,
                                                    cpl_size, cpl_size,
                                                    double, double,
                                                    double, double,
                                                    double *,
                                                    const double *, double,
                                                    const double *, double,
                                                    double, double,
                                                    double *)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(1,9,10,12,16)))
#endif
    ;

/** @addtogroup cpl_image

    Usage:  define the following preprocessor symbols as needed, 
            then include this file
*/
/**@{*/

/*-----------------------------------------------------------------------------
                            Function codes
 -----------------------------------------------------------------------------*/
  
#define CPL_OPERATION CPL_IMAGE_WARP
/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Warp an image
  @param    out         Pre-allocated destination image to hold the result
  @param    in          Source image to warp
  @param    deltax      The x shift of each pixel, same image size as out
  @param    deltay      The y shift of each pixel, same image size as out
  @param    xprofile    Interpolation weight as a function of the distance in X
  @param    xradius     Positive inclusion radius in the X-dimension
  @param    yprofile    Interpolation weight as a function of the distance in Y
  @param    yradius     Positive inclusion radius in the Y-dimension
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error
  @see cpl_image_get_interpolated()

  The pixel value at the (integer) position (u, v) in the destination image is
  interpolated from the (typically non-integer) pixel position (x, y) in the
  source image, where

  @verbatim
  x = u - deltax(u, v),
  y = v - deltay(u, v).
  @endverbatim

  The identity transform is thus given by deltax and deltay filled with
  zeros.

  The first pixel is (1, 1), located in the lower left.
  'out' and 'in' may have different sizes, while deltax and deltay must have
  the same size as 'out'. deltax and deltay must have pixel type
  CPL_TYPE_DOUBLE.

  Beware that extreme transformations may lead to blank images.

  'out' and 'in' may be of type CPL_TYPE_INT, CPL_TYPE_FLOAT or CPL_TYPE_DOUBLE.
  
  Examples of profiles and radius are:
  
  @verbatim
  xprofile = cpl_vector_new(CPL_KERNEL_DEF_SAMPLES);
  cpl_vector_fill_kernel_profile(profile, CPL_KERNEL_DEFAULT,
        CPL_KERNEL_DEF_WIDTH);
  xradius = CPL_KERNEL_DEF_WIDTH;
  @endverbatim

  In case a correction for flux conservation were required, please create
  a correction map using the function @c cpl_image_fill_jacobian().

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if (one of) the input pointer(s) is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the input images sizes are incompatible
  or if the delta images are not of type CPL_TYPE_DOUBLE
  - CPL_ERROR_INVALID_TYPE if the passed image type is not supported
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_warp(
        cpl_image               *   out, 
        const cpl_image         *   in, 
        const cpl_image         *   deltax,
        const cpl_image         *   deltay, 
        const cpl_vector        *   xprofile,
        double                      xradius,
        const cpl_vector        *   yprofile,
        double                      yradius) 
{
    cpl_mask        *   bad;
    cpl_binary      *   pbad;
    int                 hasbad;
    const double        sqxradius = xradius * xradius;
    const double        sqyradius = yradius * yradius;
    double              sqyxratio;
    double              xtabsperpix;
    double              ytabsperpix;
    const double    *   pxprof;
    const double    *   pyprof;
    cpl_ifalloc         yweightbuf;
    double          *   yweight;
    cpl_size            ixprolen;
    cpl_size            iyprolen;
    const double    *   pdeltax;
    const double    *   pdeltay;
    const cpl_binary*   inbpm;

    /* Check entries */
    cpl_ensure_code(out,    CPL_ERROR_NULL_INPUT); 
    cpl_ensure_code(in,     CPL_ERROR_NULL_INPUT); 
    cpl_ensure_code(deltax, CPL_ERROR_NULL_INPUT); 
    cpl_ensure_code(deltay, CPL_ERROR_NULL_INPUT); 
    cpl_ensure_code(cpl_image_get_size_x(deltax) == cpl_image_get_size_x(out), 
                            CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(cpl_image_get_size_y(deltax) == cpl_image_get_size_y(out), 
                            CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(cpl_image_get_size_x(deltay) == cpl_image_get_size_x(out), 
                            CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(cpl_image_get_size_y(deltay) == cpl_image_get_size_y(out), 
                            CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(cpl_image_get_type(deltax) == CPL_TYPE_DOUBLE, 
                            CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(cpl_image_get_type(deltay) == CPL_TYPE_DOUBLE, 
                            CPL_ERROR_ILLEGAL_INPUT);

    ixprolen = cpl_vector_get_size(xprofile);
    cpl_ensure_code(ixprolen > 0, CPL_ERROR_ILLEGAL_INPUT);
    iyprolen = cpl_vector_get_size(yprofile);
    cpl_ensure_code(iyprolen > 0, CPL_ERROR_ILLEGAL_INPUT);

    cpl_ensure_code(xradius > 0, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(yradius > 0, CPL_ERROR_ILLEGAL_INPUT);

    /* Initialise */
    bad = NULL;
    hasbad = 0;
    pxprof  = cpl_vector_get_data_const(xprofile);
    pyprof  = cpl_vector_get_data_const(yprofile);
    sqyxratio = sqyradius / sqxradius;
    xtabsperpix = (double)(ixprolen-1)/xradius;
    ytabsperpix = (double)(iyprolen-1)/yradius;
    cpl_ifalloc_set(&yweightbuf,
                    (1 + (size_t)(2.0 * yradius)) * sizeof(*yweight));
    yweight = (double*)cpl_ifalloc_get(&yweightbuf);

    /* Access the offsets */
    pdeltax = cpl_image_get_data_double_const(deltax);
    pdeltay = cpl_image_get_data_double_const(deltay);

    /* Create the bad pixels mask */
    bad = cpl_mask_new(out->nx, out->ny);
    pbad = cpl_mask_get_data(bad);

    inbpm = in->bpm ? cpl_mask_get_data_const(in->bpm) : NULL;

    switch (in->type) {
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_image_resample_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_image_resample_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_INT
#include "cpl_image_resample_body.h"
#undef CPL_CLASS
    default:
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);

    }
    cpl_ifalloc_free(&yweightbuf);

    /* Handle bad pixels */
    if (hasbad) cpl_image_reject_from_mask(out, bad);
    cpl_mask_delete(bad);

    return CPL_ERROR_NONE;
}
#undef CPL_OPERATION


#define CPL_OPERATION CPL_IMAGE_WARP_POLYNOMIAL
/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Warp an image according to a 2D polynomial transformation.
  @param    out       Pre-allocated image to hold the result
  @param    in        Image to warp.
  @param    poly_x    Defines source x-pos corresponding to destination (u,v).
  @param    poly_y    Defines source y-pos corresponding to destination (u,v).
  @param    xprofile  Interpolation weight as a function of the distance in X
  @param    xradius   Positive inclusion radius in the X-dimension
  @param    yprofile  Interpolation weight as a function of the distance in Y
  @param    yradius   Positive inclusion radius in the Y-dimension
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error
  @see cpl_image_get_interpolated()

  'out' and 'in'  may have different dimensions and types.

  The pair of 2D polynomials are used internally like this

  @verbatim
  x = cpl_polynomial_eval(poly_x, (u, v));
  y = cpl_polynomial_eval(poly_y, (u, v));
  @endverbatim

  where (u,v) are (integer) pixel positions in the destination image and (x,y)
  are the corresponding pixel positions (typically non-integer) in the source
  image.

  The identity transform (poly_x(u,v) = u, poly_y(u,v) = v) would thus
  overwrite the 'out' image with the 'in' image, starting from the lower left
  if the two images are of different sizes.

  Beware that extreme transformations may lead to blank images.

  The input image type may be CPL_TYPE_INT, CPL_TYPE_FLOAT or CPL_TYPE_DOUBLE.
  
  In case a correction for flux conservation were required, please create
  a correction map using the function @c cpl_image_fill_jacobian_polynomial().

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if (one of) the input pointer(s) is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the polynomial dimensions are not 2
  - CPL_ERROR_INVALID_TYPE if the passed image type is not supported
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_warp_polynomial(
        cpl_image            * out, 
        const cpl_image      * in, 
        const cpl_polynomial * poly_x,
        const cpl_polynomial * poly_y, 
        const cpl_vector     * xprofile,
        double                 xradius,
        const cpl_vector     * yprofile,
        double                 yradius) 
{
    double       x, y;
    cpl_vector * val = NULL;
    double     * pval;
    cpl_mask   * bad = NULL;
    cpl_binary * pbad;
    double value, confidence;
    cpl_size     i, j;
    int hasbad = 0;

    const double sqxradius = xradius * xradius;
    const double sqyradius = yradius * yradius;
    double sqyxratio;

    double         xtabsperpix;
    double         ytabsperpix;
    const double * pxprof;
    const double * pyprof;

    cpl_ifalloc    yweightbuf;
    double       * yweight;

    cpl_size ixprolen;
    cpl_size iyprolen;
    const cpl_binary*   inbpm;


    /* Check entries */
    cpl_ensure_code(out,    CPL_ERROR_NULL_INPUT); 
    cpl_ensure_code(in,     CPL_ERROR_NULL_INPUT); 
    cpl_ensure_code(poly_x, CPL_ERROR_NULL_INPUT); 
    cpl_ensure_code(poly_y, CPL_ERROR_NULL_INPUT); 
    cpl_ensure_code(cpl_polynomial_get_dimension(poly_x) == 2, 
                            CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(cpl_polynomial_get_dimension(poly_y) == 2, 
                            CPL_ERROR_ILLEGAL_INPUT);


    ixprolen = cpl_vector_get_size(xprofile);
    cpl_ensure(ixprolen > 0,   cpl_error_get_code(), -3);

    iyprolen = cpl_vector_get_size(yprofile);
    cpl_ensure(iyprolen > 0,   cpl_error_get_code(), -4);

    cpl_ensure(xradius > 0,    CPL_ERROR_ILLEGAL_INPUT, -5);
    cpl_ensure(yradius > 0,    CPL_ERROR_ILLEGAL_INPUT, -6);


    pxprof  = cpl_vector_get_data_const(xprofile);
    pyprof  = cpl_vector_get_data_const(yprofile);

    sqyxratio = sqyradius/sqxradius;

    xtabsperpix = (double)(ixprolen-1) / xradius;
    ytabsperpix = (double)(iyprolen-1) / yradius;

    cpl_ifalloc_set(&yweightbuf,
                    (1 + (size_t)(2.0 * yradius)) * sizeof(*yweight));
    yweight = (double*)cpl_ifalloc_get(&yweightbuf);

    val = cpl_vector_new(2);
    pval = cpl_vector_get_data(val);

    bad = cpl_mask_new(out->nx, out->ny);
    pbad = cpl_mask_get_data(bad);

    inbpm = in->bpm ? cpl_mask_get_data_const(in->bpm) : NULL;

    switch (in->type) {
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_image_resample_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_image_resample_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_INT
#include "cpl_image_resample_body.h"
#undef CPL_CLASS
    default:
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);

    }

    cpl_vector_delete(val);
    cpl_ifalloc_free(&yweightbuf);

    if (hasbad) cpl_image_reject_from_mask(out, bad);

    cpl_mask_delete(bad);

    return CPL_ERROR_NONE;
}
#undef CPL_OPERATION

#define CPL_OPERATION CPL_IMAGE_RESAMPLE_SUBSAMPLE
/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Sub-sample an image
  @param    image   The image to subsample
  @param    xstep   Take every xstep pixel in x
  @param    ystep   Take every ystep pixel in y
  @return   The newly allocated sub-sampled image or NULL in error case
  @see cpl_image_extract

  step represents the sampling step in x and y: both steps = 2 will create an
  image with a quarter of the pixels of the input image.

  image type can be CPL_TYPE_INT, CPL_TYPE_FLOAT and CPL_TYPE_DOUBLE.
  If the image has bad pixels, they will be resampled in the same way.

  The flux of the sampled pixels will be preserved, while the flux of the
  pixels not sampled will be lost. Using steps = 2 in each direction on a
  uniform image will thus create an image with a quarter of the flux.
  
  The returned image must be deallocated using cpl_image_delete().

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if the input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if xstep, ystep are not positive
  - CPL_ERROR_INVALID_TYPE if the passed image type is not supported
*/
/*----------------------------------------------------------------------------*/
cpl_image *cpl_image_extract_subsample(const cpl_image *image,
                                       cpl_size         xstep,
                                       cpl_size         ystep)
{
    cpl_image *out_im;
    cpl_size   new_nx, new_ny;
    cpl_size   i, j;


    cpl_ensure(image != NULL, CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(xstep > 0, CPL_ERROR_ILLEGAL_INPUT, NULL);
    cpl_ensure(ystep > 0, CPL_ERROR_ILLEGAL_INPUT, NULL);

    new_nx = (image->nx - 1)/xstep + 1;
    new_ny = (image->ny - 1)/ystep + 1;

    switch (image->type) {
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_image_resample_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_image_resample_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_INT
#include "cpl_image_resample_body.h"
#undef CPL_CLASS
        default:
            (void)cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            return NULL;
    }

    /* Sub sample the bad pixel map too */

    if (image->bpm != NULL) 
        out_im->bpm = cpl_mask_extract_subsample(image->bpm, xstep, ystep);

    return out_im;
}
#undef CPL_OPERATION


#define CPL_OPERATION CPL_IMAGE_REBIN
/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Rebin an image
  @param    image   The image to rebin
  @param    xstart  start x position of binning (starting from 1...)
  @param    ystart  start y position of binning (starting from 1...)
  @param    xstep   Bin size in x.
  @param    ystep   Bin size in y.
  @return   The newly allocated rebinned image or NULL in case of error

  If both bin sizes in x and y are = 2, an image with (about) a quarter 
  of the pixels of the input image will be created. Each new pixel
  will be the sum of the values of all contributing input pixels.
  If a bin is incomplete (i.e., the input image size is not a multiple 
  of the bin sizes), it is not computed.

  xstep and ystep must not be greater than the sizes of the rebinned 
  region.

  The input image type can be CPL_TYPE_INT, CPL_TYPE_FLOAT and CPL_TYPE_DOUBLE.
  If the image has bad pixels, they will be propagated to the rebinned
  image "pessimistically", i.e., if at least one of the contributing
  input pixels is bad, then the corresponding output pixel will also 
  be flagged "bad". If you need an image of "weights" for each rebinned 
  pixel, just cast the input image bpm into a CPL_TYPE_INT image, and
  apply cpl_image_rebin() to it too.

  The returned image must be deallocated using cpl_image_delete().

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if the input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if xstep, ystep, xstart, ystart are not positive
  - CPL_ERROR_INVALID_TYPE if the passed image type is not supported
*/
/*----------------------------------------------------------------------------*/
cpl_image *cpl_image_rebin(const cpl_image *image,
                           cpl_size         xstart,
                           cpl_size         ystart,
                           cpl_size         xstep,
                           cpl_size         ystep)
{
    cpl_image     *out_im;
    cpl_size       nx, ny;
    cpl_size       new_nx, new_ny;
    const cpl_size old_nx = cpl_image_get_size_x(image);
    cpl_size       i, j;


    cpl_ensure(image != NULL, CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(xstart > 0, CPL_ERROR_ILLEGAL_INPUT, NULL);
    cpl_ensure(ystart > 0, CPL_ERROR_ILLEGAL_INPUT, NULL);
    cpl_ensure(xstep  > 0, CPL_ERROR_ILLEGAL_INPUT, NULL);
    cpl_ensure(ystep  > 0, CPL_ERROR_ILLEGAL_INPUT, NULL);

    new_nx = (image->nx - xstart + 1)/xstep;
    new_ny = (image->ny - ystart + 1)/ystep;

    cpl_ensure(new_nx > 0, CPL_ERROR_ILLEGAL_INPUT, NULL);
    cpl_ensure(new_ny > 0, CPL_ERROR_ILLEGAL_INPUT, NULL);

    nx = new_nx * xstep + xstart - 1;
    ny = new_ny * ystep + ystart - 1;


    switch (image->type) {
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_image_resample_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_image_resample_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_INT
#include "cpl_image_resample_body.h"
#undef CPL_CLASS
        default:
            (void)cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            return NULL;
    }

    /* Propagate the bad pixel map if present */

    if (image->bpm != NULL) {
        const cpl_binary *pin;
        cpl_binary       *pout;
        cpl_size          pos;

        out_im->bpm = cpl_mask_new(new_nx, new_ny);
        pin = cpl_mask_get_data_const(image->bpm);
        pout = cpl_mask_get_data(out_im->bpm);

        for (j = ystart - 1; j < ny; j++) {
            for (i = xstart - 1; i < nx; i++) {
                pos = (i-xstart+1)/xstep + ((j-ystart+1)/ystep)*new_nx;
                if (pout[pos] == CPL_BINARY_0)
                    pout[pos] = pin[i + j*nx];
            }
        }
    }

    return out_im;
}
#undef CPL_OPERATION


/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Interpolate a pixel 
  @param    source    Interpolation source
  @param    xpos      Pixel x floating-point position (FITS convention)
  @param    ypos      Pixel y floating-point position (FITS convention)
  @param    xprofile  Interpolation weight as a function of the distance in X
  @param    xradius   Positive inclusion radius in the X-dimension
  @param    yprofile  Interpolation weight as a function of the distance in Y
  @param    yradius   Positive inclusion radius in the Y-dimension
  @param    pconfid   Confidence level of the interpolated value (range 0 to 1)
  @return   The interpolated pixel value, or undefined on error
  @see cpl_image_get()

  If the X- and Y-radii are identical the area of inclusion is a circle,
  otherwise it is an ellipse, with the larger of the two radii as the
  semimajor axis and the other as the semiminor axis.

  The radii are only required to be positive. However, for small radii,
  especially radii less than 1/sqrt(2), (xpos, ypos) may be located such that
  no source pixels are included in the interpolation, causing the interpolated
  pixel value to be undefined.

  The X- and Y-profiles can be generated with
  cpl_vector_fill_kernel_profile(profile, radius).
  For profiles generated with cpl_vector_fill_kernel_profile() it is
  important to use the same radius both there and in
  cpl_image_get_interpolated().

  A good profile length is CPL_KERNEL_DEF_SAMPLES,
  using radius CPL_KERNEL_DEF_WIDTH.

  On error *pconfid is negative (unless pconfid is NULL).
  Otherwise, if *pconfid is zero, the interpolated pixel-value is undefined.
  Otherwise, if *pconfid is less than 1, the area of inclusion is close to the
  image border or contains rejected pixels.

  The input image type can be CPL_TYPE_INT, CPL_TYPE_FLOAT and CPL_TYPE_DOUBLE.
  
  Here is an example of a simple image unwarping (with error-checking omitted
  for brevity):

    const double xyradius = CPL_KERNEL_DEF_WIDTH;
  
    cpl_vector * xyprofile = cpl_vector_new(CPL_KERNEL_DEF_SAMPLES);
    cpl_image  * unwarped  = cpl_image_new(nx, ny, CPL_TYPE_DOUBLE);

    cpl_vector_fill_kernel_profile(xyprofile, CPL_KERNEL_DEFAULT, xyradius);

    for (iv = 1; iv <= ny; iv++) {
        for (iu = 1; iu <= nx; iu++) {
            double confidence;
            const double x = my_unwarped_x();
            const double y = my_unwarped_y();

            const double value = cpl_image_get_interpolated(warped, x, y,
                                                            xyprofile, xyradius,
                                                            xyprofile, xyradius,
                                                            &confidence);

            if (confidence > 0)
                cpl_image_set(unwarped, iu, iv, value);
            else
                cpl_image_reject(unwarped, iu, iv);
        }
    }

    cpl_vector_delete(xyprofile);


  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if (one of) the input pointer(s) is NULL
  - CPL_ERROR_ILLEGAL_INPUT if xradius, xprofile, yprofile and yradius
    are not as requested
  - CPL_ERROR_INVALID_TYPE if the passed image type is not supported
*/
/*----------------------------------------------------------------------------*/
double cpl_image_get_interpolated(const cpl_image  * source,
                                  double             xpos,
                                  double             ypos,
                                  const cpl_vector * xprofile,
                                  double             xradius,
                                  const cpl_vector * yprofile,
                                  double             yradius,
                                  double           * pconfid)

{

    const double * pxprof  = cpl_vector_get_data_const(xprofile);
    const double * pyprof  = cpl_vector_get_data_const(yprofile);

    const double sqxradius = xradius * xradius;
    const double sqyradius = yradius * yradius;
    double sqyxratio;

    double xtabsperpix, ytabsperpix;

    const cpl_size nx       = cpl_image_get_size_x(source);
    const cpl_size ny       = cpl_image_get_size_y(source);
    const cpl_size ixprolen = cpl_vector_get_size(xprofile);
    const cpl_size iyprolen = cpl_vector_get_size(yprofile);
    /* Use the stack except for ridiculously large radii that would use
       too much. A negative Y radius is cast to a very large number */
    const size_t    ywsize   = 1 + (size_t)(2.0 * yradius);
    cpl_ifalloc  yweightbuf;
    double     * yweight;
    double value;


    cpl_ensure(pconfid !=NULL, CPL_ERROR_NULL_INPUT, -1);
    *pconfid = -1;

    cpl_ensure(nx > 0,         cpl_error_get_code(), -2);
    cpl_ensure(ixprolen > 0,   cpl_error_get_code(), -3);
    cpl_ensure(iyprolen > 0,   cpl_error_get_code(), -4);

    cpl_ensure(xradius > 0,    CPL_ERROR_ILLEGAL_INPUT, -5);
    cpl_ensure(yradius > 0,    CPL_ERROR_ILLEGAL_INPUT, -6);

    xtabsperpix = (double)(ixprolen-1) / xradius;
    ytabsperpix = (double)(iyprolen-1) / yradius;

    sqyxratio = sqyradius/sqxradius;

    cpl_ifalloc_set(&yweightbuf, ywsize * sizeof(*yweight));
    yweight = (double*)cpl_ifalloc_get(&yweightbuf);

    switch (source->type) {
    case CPL_TYPE_DOUBLE:
        value = cpl_image_get_interpolated_double
            ((const double*)source->pixels,
             source->bpm ? cpl_mask_get_data_const(source->bpm) : NULL,
             nx, ny,
             xtabsperpix, ytabsperpix,
             xpos, ypos, yweight,
             pxprof, xradius,
             pyprof, yradius,
             sqyradius, sqyxratio,
             pconfid);
        break;
    case CPL_TYPE_FLOAT:

        value = cpl_image_get_interpolated_float
            ((const float*)source->pixels,
             source->bpm ? cpl_mask_get_data_const(source->bpm) : NULL,
             nx, ny,
             xtabsperpix, ytabsperpix,
             xpos, ypos, yweight,
             pxprof, xradius,
             pyprof, yradius,
             sqyradius, sqyxratio,
             pconfid);
        break;
    case CPL_TYPE_INT:

        value = cpl_image_get_interpolated_int
            ((const int*)source->pixels,
             source->bpm ? cpl_mask_get_data_const(source->bpm) : NULL,
             nx, ny,
             xtabsperpix, ytabsperpix,
             xpos, ypos, yweight,
             pxprof, xradius,
             pyprof, yradius,
             sqyradius, sqyxratio,
             pconfid);
        break;

    default:
        value = 0;
        cpl_error_set_(CPL_ERROR_INVALID_TYPE);

    }

    cpl_ifalloc_free(&yweightbuf);
    return value;

}


/**
 @ingroup cpl_image
 * @brief    Compute area change ratio for a 2D polynomial transformation.
 *
 * @param    out       Pre-allocated image to hold the result
 * @param    poly_x    Defines source x-pos corresponding to destination (u,v).
 * @param    poly_y    Defines source y-pos corresponding to destination (u,v).
 *
 * @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error
 *
 * @see cpl_image_warp_polynomial()
 *
 *
 * Given an input image with pixel coordinates (x, y) which is 
 * mapped into an output image with pixel coordinates (u, v), and 
 * the polynomial inverse transformation (u, v) to (x, y) as in
 * @ref cpl_image_warp_polynomial(), this function writes the density 
 * of the (u, v) coordinate system relative to the (x, y) coordinates 
 * for each (u, v) pixel of image @em out.
 *
 * This is trivially obtained by computing the absolute value of the
 * determinant of the Jacobian of the transformation for each pixel
 * of the (u, v) image @em out.
 *
 * Typically this function would be used to determine a flux-conservation
 * factor map for the target image specified in function
 * @c cpl_image_warp_polynomial(). For example,
 *
 * @verbatim
 * cpl_image_warp_polynomial(out, in, poly_x, poly_y, xprof, xrad, yprof, yrad);
 * correction_map = cpl_image_new(cpl_image_get_size_x(out),
 *                                cpl_image_get_size_y(out),
 *                                cpl_image_get_type(out));
 * cpl_image_fill_jacobian_polynomial(correction_map, poly_x, poly_y);
 * out_flux_corrected = cpl_image_multiply_create(out, correction_map);
 * @endverbatim
 *
 * where @em out_flux_corrected is the resampled image @em out after
 * correction for flux conservation.
 *
 * @note
 *   The map produced by this function is not applicable for
 *   flux conservation in case the transformation implies severe
 *   undersampling of the original signal.
 *
 * Possible #_cpl_error_code_ set in this function:
 * - CPL_ERROR_NULL_INPUT if (one of) the input pointer(s) is NULL
 * - CPL_ERROR_ILLEGAL_INPUT if the polynomial dimensions are not 2
 * - CPL_ERROR_INVALID_TYPE if the passed image type is not supported
 */

cpl_error_code cpl_image_fill_jacobian_polynomial(cpl_image *out,
                                                  const cpl_polynomial *poly_x,
                                                  const cpl_polynomial *poly_y)
{
    cpl_polynomial *dxdu;
    cpl_polynomial *dxdv;
    cpl_polynomial *dydu;
    cpl_polynomial *dydv;

    cpl_vector     *val;
    double         *pval;

    double         *ddata;
    float          *fdata;

    cpl_size        nx, ny;
    cpl_size        i, j;
    cpl_error_code error = CPL_ERROR_NONE;


    if (out == NULL || poly_x == NULL || poly_y == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_polynomial_get_dimension(poly_x) != 2 ||
        cpl_polynomial_get_dimension(poly_y) != 2)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    if (cpl_image_get_type(out) != CPL_TYPE_FLOAT &&
        cpl_image_get_type(out) != CPL_TYPE_DOUBLE)
        return cpl_error_set_(CPL_ERROR_INVALID_TYPE);


    /*
     * Compute the partial derivatives of the transformation:
     */

    dxdu = cpl_polynomial_duplicate(poly_x);
    dxdv = cpl_polynomial_duplicate(poly_x);
    dydu = cpl_polynomial_duplicate(poly_y);
    dydv = cpl_polynomial_duplicate(poly_y);

    cpl_polynomial_derivative(dxdu, 0);
    cpl_polynomial_derivative(dxdv, 1);
    cpl_polynomial_derivative(dydu, 0);
    cpl_polynomial_derivative(dydv, 1);


    /*
     * This section is for assigning the (local) scaling factor
     * to each pixel of the reference image.
     */

    nx = cpl_image_get_size_x(out);
    ny = cpl_image_get_size_y(out);

    val = cpl_vector_new(2);
    pval = cpl_vector_get_data(val);

    switch (cpl_image_get_type(out)) {
    case CPL_TYPE_FLOAT:
        fdata = cpl_image_get_data_float(out);
        for (j=0; j < ny; j++) {
            pval[1] = (double)(j + 1);
            for (i=0; i < nx; i++) {
                pval[0] = (double)(i + 1);
                *fdata++ = (float)(cpl_polynomial_eval(dxdu, val)
                                   * cpl_polynomial_eval(dydv, val)
                                   - cpl_polynomial_eval(dxdv, val)
                                   * cpl_polynomial_eval(dydu, val));
            }
        }
        break;
    case CPL_TYPE_DOUBLE:
        ddata = cpl_image_get_data_double(out);
        for (j=0; j < ny; j++) {
            pval[1] = (double)(j + 1);
            for (i=0; i < nx; i++) {
                pval[0] = (double)(i + 1);
                *ddata++ = cpl_polynomial_eval(dxdu, val)
                         * cpl_polynomial_eval(dydv, val)
                         - cpl_polynomial_eval(dxdv, val)
                         * cpl_polynomial_eval(dydu, val);
            }
        }
        break;
    default:
        /* It is an error in CPL to reach this point */
        error = CPL_ERROR_UNSPECIFIED;
        break;
    }

    cpl_vector_delete(val);
    cpl_polynomial_delete(dxdu);
    cpl_polynomial_delete(dxdv);
    cpl_polynomial_delete(dydu);
    cpl_polynomial_delete(dydv);


    /*
     * Ensure the scale factor is positive...
     */

    if (!error) error = cpl_image_abs(out);

    /* Propagate error, if any */
    return cpl_error_set_(error);
}


/**
 @ingroup cpl_image
 * @brief    Compute area change ratio for a transformation map.
 *
 * @param    out       Pre-allocated image to hold the result
 * @param    deltax    The x shifts for each pixel
 * @param    deltay    The y shifts for each pixel
 *
 * @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error
 *
 * @see cpl_image_warp()
 *
 * The shifts images @em deltax and @em deltay, describing the
 * transformation, must be of type CPL_TYPE_DOUBLE and of the 
 * same size as @em out. For each pixel (u, v) of the @em out
 * image, the deltax and deltay code the following transformation:
 *
 * @verbatim
 * u - deltax(u,v) = x
 * v - deltay(u,v) = y
 * @endverbatim
 *
 * This function writes the density of the (u, v) coordinate 
 * system relative to the (x, y) coordinates for each (u, v) 
 * pixel of image @em out.
 *
 * This is trivially obtained by computing the absolute value of the
 * determinant of the Jacobian of the transformation for each pixel
 * of the (u, v) image @em out.
 *
 * The partial derivatives are estimated at the position (u, v)
 * in the following way:
 *
 * @verbatim
 * dx/du = 1 + 1/2 ( deltax(u-1, v) - deltax(u+1, v) )
 * dx/dv =     1/2 ( deltax(u, v-1) - deltax(u, v+1) )
 * dy/du =     1/2 ( deltay(u-1, v) - deltay(u+1, v) )
 * dy/dv = 1 + 1/2 ( deltay(u, v-1) - deltay(u, v+1) )
 * @endverbatim
 *
 * Typically this function would be used to determine a flux-conservation
 * factor map for the target image specified in function
 * @c cpl_image_warp(). For example,
 *
 * @verbatim
 * cpl_image_warp(out, in, deltax, deltay, xprof, xrad, yprof, yrad);
 * correction_map = cpl_image_new(cpl_image_get_size_x(out),
 *                                cpl_image_get_size_y(out),
 *                                cpl_image_get_type(out));
 * cpl_image_fill_jacobian(correction_map, deltax, deltay);
 * out_flux_corrected = cpl_image_multiply_create(out, correction_map);
 * @endverbatim
 *
 * where @em out_flux_corrected is the resampled image @em out after
 * correction for flux conservation.
 *
 * @note
 *   The map produced by this function is not applicable for
 *   flux conservation in case the transformation implies severe
 *   undersampling of the original signal.
 *
 * Possible #_cpl_error_code_ set in this function:
 * - CPL_ERROR_NULL_INPUT if (one of) the input pointer(s) is NULL
 * - CPL_ERROR_ILLEGAL_INPUT if the polynomial dimensions are not 2
 * - CPL_ERROR_INVALID_TYPE if the passed image type is not supported
 */

cpl_error_code cpl_image_fill_jacobian(cpl_image *out,
                                       const cpl_image *deltax,
                                       const cpl_image *deltay)
{
    cpl_image    *dxdu;
    cpl_image    *dxdv;
    cpl_image    *dydu;
    cpl_image    *dydv;

    double       *d_dxdu;
    double       *d_dxdv;
    double       *d_dydu;
    double       *d_dydv;
    const double *ddeltax;
    const double *ddeltay;

    double       *ddata;
    float        *fdata;

    cpl_size      nx, ny, npix;
    cpl_size      i, j, pos;
    cpl_error_code error = CPL_ERROR_NONE;


    if (out == NULL || deltax == NULL || deltay == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (cpl_image_get_type(out) != CPL_TYPE_FLOAT &&
        cpl_image_get_type(out) != CPL_TYPE_DOUBLE)
        return cpl_error_set_(CPL_ERROR_INVALID_TYPE);

    if (cpl_image_get_type(deltax) != CPL_TYPE_DOUBLE ||
        cpl_image_get_type(deltay) != CPL_TYPE_DOUBLE)
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    nx = cpl_image_get_size_x(out);
    ny = cpl_image_get_size_y(out);
    
    if (nx != cpl_image_get_size_x(deltax) ||
        nx != cpl_image_get_size_x(deltay) ||
        ny != cpl_image_get_size_y(deltax) ||
        ny != cpl_image_get_size_y(deltay)) {
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
    }


    /*
     * Compute the partial derivatives of the transformation:
     */

    dxdu = cpl_image_new(nx, ny, CPL_TYPE_DOUBLE);
    dxdv = cpl_image_new(nx, ny, CPL_TYPE_DOUBLE);
    dydu = cpl_image_new(nx, ny, CPL_TYPE_DOUBLE);
    dydv = cpl_image_new(nx, ny, CPL_TYPE_DOUBLE);

    d_dxdu  = cpl_image_get_data_double(dxdu);
    d_dxdv  = cpl_image_get_data_double(dxdv);
    d_dydu  = cpl_image_get_data_double(dydu);
    d_dydv  = cpl_image_get_data_double(dydv);
    ddeltax = cpl_image_get_data_double_const(deltax);
    ddeltay = cpl_image_get_data_double_const(deltay);


    /*
     * Note: borders are excluded
     */

    for (j = 1; j < ny - 1; j++) {
        pos = j * nx;
        for (i = 1; i < nx - 1; i++) {
            pos++;
            d_dxdu[pos] = (ddeltax[pos-1] - ddeltax[pos+1]) / 2 + 1;
            d_dxdv[pos] = (ddeltax[pos-nx] - ddeltax[pos+nx]) / 2;
            d_dydu[pos] = (ddeltay[pos-1] - ddeltay[pos+1]) / 2;
            d_dydv[pos] = (ddeltay[pos-nx] - ddeltay[pos+nx]) / 2 + 1;
        }
    }


    /*
     * This section is for assigning the (local) scaling factor
     * to each pixel of the reference image.
     */

    npix = nx * ny;

    switch (cpl_image_get_type(out)) {
    case CPL_TYPE_FLOAT:
        fdata = cpl_image_get_data_float(out);
        for (i = 0; i < npix; i++) {
            *fdata++ = (float)((*d_dxdu++) * (*d_dydv++)
                               - (*d_dxdv++) * (*d_dydu++));
        }
        break;
    case CPL_TYPE_DOUBLE:
        ddata = cpl_image_get_data_double(out);
        for (i = 0; i < npix; i++) {
            *ddata++ = (*d_dxdu++) * (*d_dydv++) - (*d_dxdv++) * (*d_dydu++);
        }
        break;
    default:

        /* It is an error in CPL to reach this point */
        error = CPL_ERROR_UNSPECIFIED;
        break;
    }

    cpl_image_delete(dxdu);
    cpl_image_delete(dxdv);
    cpl_image_delete(dydu);
    cpl_image_delete(dydv);


    /*
     * Ensure the scale factor is positive...
     */

    if (!error) error = cpl_image_abs(out);

    /* Propagate error, if any */
    return cpl_error_set_(error);

}

/**@}*/

#define CPL_CLASS CPL_CLASS_DOUBLE
#undef CPL_OPERATION
#include "cpl_image_resample_body.h"
#undef CPL_CLASS

#define CPL_CLASS CPL_CLASS_FLOAT
#undef CPL_OPERATION
#include "cpl_image_resample_body.h"
#undef CPL_CLASS

#define CPL_CLASS CPL_CLASS_INT
#undef CPL_OPERATION
#include "cpl_image_resample_body.h"
#undef CPL_CLASS


