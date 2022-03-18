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
#include <complex.h>

#include "cpl_tools.h"
#include "cpl_error_impl.h"
#include "cpl_math_const.h"
#include "cpl_image_gen.h"
#include "cpl_image_basic.h"

#include "cpl_image_defs.h"

/*-----------------------------------------------------------------------------
                                   Defines
 -----------------------------------------------------------------------------*/

#define CPL_IMAGE_GEN_NOISE_UNIFORM     1
#define CPL_IMAGE_GEN_GAUSSIAN          2
#define CPL_IMAGE_GEN_POLYNOMIAL        3

#define CPL_IMAGE_GEN_NBPOINTS 10

/* Sun Studio 12.1 expands the macro I to 1.0iF which the compiler does not
   recognize, so try something else */
#ifdef _Imaginary_I
#define CPL_I _Imaginary_I
#elif defined _Complex_I
#define CPL_I _Complex_I
#else
#define CPL_I I
#endif


/*-----------------------------------------------------------------------------
                            Function prototypes
 -----------------------------------------------------------------------------*/

static double cpl_gaussian_2d(double, double, double, double, double);

/*-----------------------------------------------------------------------------
                            Function codes
 -----------------------------------------------------------------------------*/

#define CPL_OPERATION CPL_IMAGE_GEN_NOISE_UNIFORM
/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Generate an image with uniform random noise distribution.
  @param    ima         the image to generate
  @param    min_pix     Minimum output pixel value.
  @param    max_pix     Maximum output pixel value.
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE

  Generate an image with a uniform random noise distribution. Pixel values are
  within the provided bounds.
  This function expects an already allocated image. 
  The input image type can be CPL_TYPE_INT, CPL_TYPE_FLOAT or CPL_TYPE_DOUBLE.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if (one of) the input pointer(s) is NULL
  - CPL_ERROR_ILLEGAL_INPUT if min_pix is bigger than max_pix
  - CPL_ERROR_INVALID_TYPE if the passed image type is not supported
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_fill_noise_uniform(
        cpl_image   *   ima,
        double          min_pix,
        double          max_pix)
{
    int             i, j;

    /* Test entries */
    cpl_ensure_code(ima, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(min_pix<max_pix, CPL_ERROR_ILLEGAL_INPUT);
    
    /* Switch on image type */
    switch (ima->type) {
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_image_gen_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_image_gen_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_INT
#include "cpl_image_gen_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_DOUBLE_COMPLEX
#include "cpl_image_gen_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_FLOAT_COMPLEX
#include "cpl_image_gen_body.h"
#undef CPL_CLASS
        default:
          return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
    }
    return CPL_ERROR_NONE;
}
#undef CPL_OPERATION

#define CPL_OPERATION CPL_IMAGE_GEN_GAUSSIAN
/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Generate an image from a 2d gaussian function.
  @param    ima         the gaussian image to generate
  @param    xcen        x position of the center (1 for the first pixel)
  @param    ycen        y position of the center (1 for the first pixel)
  @param    norm        norm of the gaussian.
  @param    sig_x       Sigma in x for the gaussian distribution.
  @param    sig_y       Sigma in y for the gaussian distribution.
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
  
  This function expects an already allocated image. 
  This function generates an image of a 2d gaussian. The gaussian is
  defined by the position of its centre, given in pixel coordinates inside
  the image with the FITS convention (x from 1 to nx, y from 1 to ny), its 
  norm and the value of sigma in x and y.

  f(x, y) = (norm/(2*pi*sig_x*sig_y)) * exp(-(x-xcen)^2/(2*sig_x^2)) * 
            exp(-(y-ycen)^2/(2*sig_y^2))

  The input image type can be CPL_TYPE_FLOAT or CPL_TYPE_DOUBLE.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if (one of) the input pointer(s) is NULL
  - CPL_ERROR_INVALID_TYPE if the passed image type is not supported
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_fill_gaussian(
        cpl_image   *   ima,
        double          xcen,
        double          ycen,
        double          norm,
        double          sig_x,
        double          sig_y)
{
    double              x, y;
    int                 i, j;
    
    /* Test entries */
    cpl_ensure_code(ima, CPL_ERROR_NULL_INPUT);

    /* Switch on image type */
    switch (ima->type) {
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_image_gen_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_image_gen_body.h"
#undef CPL_CLASS
        default:
          return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
    }
    return CPL_ERROR_NONE;
}
#undef CPL_OPERATION

#define CPL_OPERATION CPL_IMAGE_GEN_POLYNOMIAL
/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Generate an image from a 2d polynomial function.
  @param    ima         the polynomial image to generate
  @param    poly        the 2d polynomial
  @param    startx      the x value associated with the left pixels column
  @param    stepx       the x step
  @param    starty      the y value associated with the bottom pixels row
  @param    stepy       the y step
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
  
  This function expects an already allocated image. 
  The pixel value of the pixel (i, j) is set to 
  poly(startx+(i-1)*stepx, starty+(j-1)*stepy).

  The input image type can be CPL_TYPE_FLOAT or CPL_TYPE_DOUBLE.

  If you want to generate an image whose pixel values are the values of
  the polynomial applied to the pixel positions, just call
  cpl_image_fill_polynomial(ima, poly, 1.0, 1.0, 1.0, 1.0);
  
  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if (one of) the input pointer(s) is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the polynomial's dimension is not 2
  - CPL_ERROR_INVALID_TYPE if the passed image type is not supported
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_fill_polynomial(
        cpl_image               *   ima,
        const cpl_polynomial    *   poly,
        double                      startx,
        double                      stepx,
        double                      starty,
        double                      stepy)
{
    cpl_vector  *   x;
    double      *   xdata;
    int             i, j;

    /* Test entries */
    cpl_ensure_code(ima && poly, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(cpl_polynomial_get_dimension(poly) == 2, 
            CPL_ERROR_ILLEGAL_INPUT);

    /* Switch on image type */
    switch (ima->type) {
#define CPL_CLASS CPL_CLASS_DOUBLE
#include "cpl_image_gen_body.h"
#undef CPL_CLASS
#define CPL_CLASS CPL_CLASS_FLOAT
#include "cpl_image_gen_body.h"
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
  @brief    Generate a test image with pixel type CPL_TYPE_DOUBLE
  @param    nx    x size
  @param    ny    y size
  @return    1 newly allocated image or NULL on error
  
  Generates a reference pattern for testing purposes only.
  The created image has to be deallocated with cpl_image_delete().

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_ILLEGAL_INPUT if nx or ny is non-positive

 */
/*----------------------------------------------------------------------------*/
cpl_image * cpl_image_fill_test_create(cpl_size nx,
                                       cpl_size ny)
{

    const double xposrate[]
        = {0.4,  0.5,  0.8,  0.9,  0.2,  0.5,  0.9,  0.3,  0.1,  0.7};

    const double yposrate[10]
        = {0.4,  0.2,  0.3,  0.9,  0.7,  0.8,  0.9,  0.8,  0.7,  0.7};

    double         xpos[CPL_IMAGE_GEN_NBPOINTS];
    double         ypos[CPL_IMAGE_GEN_NBPOINTS];

    cpl_image    * test_im  = cpl_image_new(nx, ny, CPL_TYPE_DOUBLE);
    cpl_image    * tmp_im   = cpl_image_new(nx, ny, CPL_TYPE_DOUBLE);
    const cpl_size nbpoints = CPL_IMAGE_GEN_NBPOINTS;
    const double   sigma    = 10.0;
    const double   flux     = 1000.0;
    cpl_size       i;


    cpl_ensure(nx > 0, CPL_ERROR_ILLEGAL_INPUT, NULL);
    cpl_ensure(ny > 0, CPL_ERROR_ILLEGAL_INPUT, NULL);

    for (i=0; i < nbpoints; i++) {
        xpos[i] = (double)nx * xposrate[i];
        ypos[i] = (double)ny * yposrate[i];
    }
    
    /* Set the background */
    cpl_image_fill_noise_uniform(test_im, -1, 1);
    
    /* Put fake stars */
    for (i = 0; i < nbpoints; i++) {
        cpl_image_fill_gaussian(tmp_im, xpos[i], ypos[i], 10.0, sigma, sigma);
        cpl_image_multiply_scalar(tmp_im, flux / (double)(i+1));
        cpl_image_add(test_im, tmp_im);
    }

    cpl_image_delete(tmp_im);

    return test_im;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief    Compute the value of a Gaussian function at a given point.
  @param    x       x coordinate where to compute the function.
  @param    y       y coordinate where to compute the function.
  @param    norm    The norm of the gaussian.
  @param    sig_x   Sigma in x for the Gauss distribution.
  @param    sig_y   Sigma in y for the Gauss distribution.
  @return   the gaussian value

  Compute the value of a 2d Gaussian function at a given point:

  f(x, y) = (norm/(2*pi*sig_x*sig_y)) * exp(-x^2/(2*sig_x^2)) * 
            exp(-y^2/(2*sig_y^2))
 */
/*----------------------------------------------------------------------------*/
static double cpl_gaussian_2d(double  x,
                              double  y,
                              double  norm,
                              double  sig_x,
                              double  sig_y)
{
    return norm / (sig_x * sig_y * CPL_MATH_2PI *
                   exp(x * x / (2.0 * sig_x * sig_x) +
                       y * y / (2.0 * sig_y * sig_y)));
}

