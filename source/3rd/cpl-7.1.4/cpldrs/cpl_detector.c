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

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "cpl_detector.h"

#include <cpl_memory.h>
#include <cpl_math_const.h>
#include <cpl_error_impl.h>

#include <complex.h>
#include <string.h>
#include <math.h>
#include <assert.h>

/*-----------------------------------------------------------------------------
                                   Define
 -----------------------------------------------------------------------------*/

/* Computes the square of an euclidean distance bet. 2 points */
#define pdist(x1,y1,x2,y2) (((x1-x2)*(x1-x2))+((y1-y2)*(y1-y2)))
/** Computes the square of an euclidean distance bet. 2 points (polar coord) */
#define qdist(r1,t1,r2,t2) \
  ((r1*r1)+(r2*r2)-2*r1*r2*cos((t1-t2)))

#define CPL_CLASS_NONE      0
#define CPL_CLASS_DOUBLE    1
#define CPL_CLASS_FLOAT     2
#define CPL_CLASS_INT       3

/*----------------------------------------------------------------------------*/
/**
 * @defgroup cpl_detector   High-level functions to compute detector features
 *
 * @par Synopsis:
 * @code
 *   #include "cpl_detector.h"
 * @endcode
 */
/*----------------------------------------------------------------------------*/
/**@{*/

/*-----------------------------------------------------------------------------
                   Private Function declarations and definitions
 -----------------------------------------------------------------------------*/

/* Define the C-type dependent functions */

/* These three macros are needed for support of the different pixel types */

#define CONCAT(a,b) a ## _ ## b
#define CONCAT2X(a,b) CONCAT(a,b)

#define CPL_TYPE double complex
#define CPL_NAME_TYPE double_complex
#define CPL_SUM_TYPE double complex
#include "cpl_detector_body.h"
#undef CPL_TYPE
#undef CPL_NAME_TYPE

#define CPL_TYPE float complex
#define CPL_NAME_TYPE float_complex
#include "cpl_detector_body.h"
#undef CPL_TYPE
#undef CPL_NAME_TYPE
#undef CPL_SUM_TYPE

#define CPL_TYPE double
#define CPL_NAME_TYPE double
#define CPL_SUM_TYPE double
#include "cpl_detector_body.h"
#undef CPL_TYPE
#undef CPL_NAME_TYPE

#define CPL_TYPE float
#define CPL_NAME_TYPE float
#include "cpl_detector_body.h"
#undef CPL_TYPE
#undef CPL_NAME_TYPE

#define CPL_TYPE int
#define CPL_NAME_TYPE int
#define CPL_DO_ROUND
#include "cpl_detector_body.h"
#undef CPL_TYPE
#undef CPL_NAME_TYPE
#undef CPL_DO_ROUND
#undef CPL_SUM_TYPE

static cpl_error_code cpl_flux_get_window(const cpl_image *, const cpl_size *,
                                          cpl_size, cpl_size, double *,
                                          double *, double *, double *);

static cpl_bivector * cpl_bivector_gen_rect_poisson(const cpl_size *,
                                                    const cpl_size,
                                                    const cpl_size)
    CPL_ATTR_NONNULL CPL_ATTR_ALLOC;
static cpl_bivector * cpl_bivector_gen_ring_poisson(const double *,
                                                    const cpl_size,
                                                    const cpl_size)
    CPL_ATTR_NONNULL CPL_ATTR_ALLOC;

/*-----------------------------------------------------------------------------
                            Function codes
 -----------------------------------------------------------------------------*/

#define RECT_RON_HS         4
#define RECT_RON_SAMPLES    100
/*----------------------------------------------------------------------------*/
/**
  @brief    Compute the readout noise in a rectangle.
  @param    diff        Input image, usually a difference frame.
  @param    zone_def    Zone where the readout noise is to be computed.
  @param    ron_hsize   to specify half size of squares (<0 to use default)
  @param    ron_nsamp   to specify the nb of samples (<0 to use default)
  @param    noise       Output parameter: noise in the frame.
  @param    error       Output parameter: error on the noise.
  @return   CPL_ERROR_NONE on success or the relevant #_cpl_error_code_ on error
  @see      rand()
  @note     No calls to srand() are made from CPL

  This function is meant to compute the readout noise in a frame by means of a
  MonteCarlo approach. The input is a frame, usually a difference between two
  frames taken with the same settings for the acquisition system, although no
  check is done on that, it is up to the caller to feed in the right kind of
  frame.

  The provided zone is an array of four integers specifying the zone to take
  into account for the computation. The integers specify ranges as xmin, xmax,
  ymin, ymax, where these coordinates are given in the FITS notation (x from 1
  to lx, y from 1 to ly and bottom to top). Specify NULL instead of an array of
  four values to use the whole frame in the computation.

  The algorithm will create typically 100 9x9 windows on the frame, scattered
  optimally using a Poisson law. In each window, the standard deviation of all
  pixels in the window is computed and this value is stored. The readout noise
  is the median of all computed standard deviations, and the error is the
  standard deviation of the standard deviations.

  Both noise and error are returned by modifying a passed double. If you do
  not care about the error, pass NULL.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if diff or noise is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the specified window (zone_def) is invalid
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_flux_get_noise_window(const cpl_image * diff,
                                         const cpl_size  * zone_def,
                                         cpl_size          ron_hsize,
                                         cpl_size          ron_nsamp,
                                         double          * noise,
                                         double          * error)
{
    return cpl_flux_get_window(diff, zone_def, ron_hsize, ron_nsamp,
                               noise, error, NULL, NULL)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Compute the bias in a rectangle.
  @param    diff        Input image, usually a difference frame.
  @param    zone_def    Zone where the bias is to be computed.
  @param    ron_hsize   to specify half size of squares (<0 to use default)
  @param    ron_nsamp   to specify the nb of samples (<0 to use default)
  @param    bias        Output parameter: bias in the frame.
  @param    error       Output parameter: error on the bias.
  @return   CPL_ERROR_NONE on success or the relevant #_cpl_error_code_ on error
  @see      cpl_flux_get_noise_window(), rand()
  @note     No calls to srand() are made from CPL
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_flux_get_bias_window(const cpl_image * diff,
                                        const cpl_size  * zone_def,
                                        cpl_size          ron_hsize,
                                        cpl_size          ron_nsamp,
                                        double          * bias,
                                        double          * error)
{
    return cpl_flux_get_window(diff, zone_def, ron_hsize, ron_nsamp,
                               NULL, NULL, bias, error)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/*
  @brief    Internal function, wrapped by cpl_flux_get_{noise,bias}_window().
 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_flux_get_window(
        const cpl_image *   diff,
        const cpl_size  *   zone_def,
        cpl_size            ron_hsize,
        cpl_size            ron_nsamp,
        double          *   noise,
        double          *   noise_error,
        double          *   bias,
        double          *   bias_error)
{
    cpl_errorstate prestate = cpl_errorstate_get();
    const cpl_size hsize = ron_hsize < 0 ? RECT_RON_HS      : ron_hsize;
    const cpl_size nsamples = ron_nsamp < 0 ? RECT_RON_SAMPLES : ron_nsamp;
    cpl_bivector * sample_reg;
    cpl_vector   * rms_list = NULL;
    cpl_vector   * bias_list = NULL;
    cpl_size       rect[4];
    cpl_size       zone[4];
    const double * px;
    const double * py;
    double       * pr1 = NULL;
    double       * pr2 = NULL;
    cpl_size       i;

    /* Test entries */
    cpl_ensure_code(diff  != NULL,                 CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(noise != NULL || bias != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(nsamples > 0,                  CPL_ERROR_ILLEGAL_INPUT);

    /* Generate nsamples window centers in the image */
    if (zone_def != NULL) {
        rect[0] = zone_def[0] + hsize + 1; /* xmin */
        rect[1] = zone_def[1] - hsize - 1; /* xmax */
        rect[2] = zone_def[2] + hsize + 1; /* ymin */
        rect[3] = zone_def[3] - hsize - 1; /* ymax */
    } else {
        rect[0] = hsize + 1;                /* xmin */
        rect[1] = cpl_image_get_size_x(diff) - hsize - 1;    /* xmax */
        rect[2] = hsize + 1;                /* ymin */
        rect[3] = cpl_image_get_size_y(diff) - hsize - 1;    /* ymax */
    }

    cpl_ensure_code( rect[0] < rect[1] && rect[2] < rect[3],
                CPL_ERROR_ILLEGAL_INPUT);

    /* Generate n+1 regions, because the first region is always at (0,0) */
    /* and it would bias the measurement. */
    sample_reg = cpl_bivector_gen_rect_poisson(rect, nsamples+1, nsamples+1);
    cpl_ensure_code( sample_reg != NULL, CPL_ERROR_ILLEGAL_INPUT);

    px = cpl_bivector_get_x_data_const(sample_reg);
    py = cpl_bivector_get_y_data_const(sample_reg);

    /* Now, for each window center, extract a vignette and compute the */
    /* signal RMS in it. Store this rms into a table. */
    if (noise) rms_list  = cpl_vector_new(nsamples);
    if (bias)  bias_list = cpl_vector_new(nsamples);

    if (noise) pr1 = cpl_vector_get_data(rms_list);
    if (bias)  pr2 = cpl_vector_get_data(bias_list);

    for (i=0; i<nsamples; i++) {
        zone[0] = (cpl_size)px[i+1] - hsize;
        zone[1] = (cpl_size)px[i+1] + hsize;
        zone[2] = (cpl_size)py[i+1] - hsize;
        zone[3] = (cpl_size)py[i+1] + hsize;
        if (noise) 
        pr1[i] = cpl_image_get_stdev_window(diff, 
        zone[0], zone[2], zone[1], zone[3]);
    if (bias) 
        pr2[i] = cpl_image_get_mean_window(diff, 
                zone[0], zone[2], zone[1], zone[3]);
    }
    cpl_bivector_delete(sample_reg);

    if (noise_error != NULL) *noise_error = cpl_vector_get_stdev(rms_list);
    if (bias_error  != NULL) *bias_error  = cpl_vector_get_stdev(bias_list);

    /* The final computed RMS is the median of all values.  */
    /* This call will modify the rms_list */
    if (noise) *noise = cpl_vector_get_median(rms_list);
    if (bias)   *bias = cpl_vector_get_median(bias_list);

    cpl_vector_delete(rms_list);
    cpl_vector_delete(bias_list);

    return cpl_errorstate_is_equal(prestate) ? CPL_ERROR_NONE
        : cpl_error_set_where_();
}
#undef RECT_RON_HS
#undef RECT_RON_SAMPLES

#define RING_RON_HS         4    
#define RING_RON_SAMPLES    50
/*----------------------------------------------------------------------------*/
/**
  @brief    Compute the readout noise in a ring.
  @param    diff    Input image, usually a difference frame.
  @param    zone_def    Zone where the readout noise is to be computed.
  @param    ron_hsize   to specify half size of squares (<0 to use default)
  @param    ron_nsamp   to specify the nb of samples (<0 to use default)
  @param    noise       On success, *noise is the noise in the frame.
  @param    error       NULL, or on success, *error is the error in the noise
  @return   CPL_ERROR_NONE on success or the relevant #_cpl_error_code_ on error
  @see      cpl_flux_get_noise_window(), rand()
  @note     No calls to srand() are made from CPL

  The provided zone is an array of four integers specifying the zone to take
  into account for the computation. The integers specify a ring as x, y,
  r1, r2 where these coordinates are given in the FITS notation (x from 1
  to nx, y from 1 to ny and bottom to top). 

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if diff noise or zone_def is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the internal radius is bigger than the
    external one in zone_def
  - CPL_ERROR_DATA_NOT_FOUND if an insufficient number of samples were found
    inside the ring
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_flux_get_noise_ring(
        const cpl_image *   diff,
        const double    *   zone_def,
        cpl_size            ron_hsize,
        cpl_size            ron_nsamp,
        double          *   noise,
        double          *   error)
{
    cpl_bivector    *   sample_reg;
    const double    *   px;
    const double    *   py;
    const cpl_size      hsize = ron_hsize < 0 ? RING_RON_HS      : ron_hsize;
    const cpl_size      nsamp = ron_nsamp < 0 ? RING_RON_SAMPLES : ron_nsamp;
    cpl_vector      *   rms_list;
    cpl_size            nx, ny;
    cpl_size            i, rsize;

    cpl_ensure_code(diff,     CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(noise,    CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(zone_def, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(zone_def[2] < zone_def[3], CPL_ERROR_ILLEGAL_INPUT);

    nx = cpl_image_get_size_x(diff);
    ny = cpl_image_get_size_y(diff);

    cpl_ensure_code(2 * ron_hsize < nx, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(2 * ron_hsize < ny, CPL_ERROR_ILLEGAL_INPUT);

    /* Generate n+1 regions, because the first region is always at a given */
    /* position and it would bias the measurement. */
    sample_reg = cpl_bivector_gen_ring_poisson(zone_def, nsamp+1, nsamp+1);
    cpl_ensure_code(sample_reg != NULL, CPL_ERROR_ILLEGAL_INPUT);
    px = cpl_bivector_get_x_data_const(sample_reg);
    py = cpl_bivector_get_y_data_const(sample_reg);

    /* Now, for each valid sample, extract a vignette and compute the RMS */
    rms_list = cpl_vector_new(nsamp + 1);
    assert( rms_list != NULL );
    rsize = 0;
    for (i=0; i < nsamp + 1; i++) {
        cpl_errorstate prestate = cpl_errorstate_get();
        cpl_size       zone[4];
        /* Add 0.5 to round off to nearest (positive) integer */
        const double   dx = 0.5 + zone_def[0] + px[i] * cos(py[i]);
        const double   dy = 0.5 + zone_def[1] + px[i] * sin(py[i]);
        double stdev;

        zone[0] = (cpl_size)dx - hsize;
        if (zone[0] < 1) continue;

        zone[1] = (cpl_size)dx + hsize;
        if (zone[1] > nx) continue;

        zone[2] = (cpl_size)dy - hsize;
        if (zone[2] < 1) continue;

        zone[3] = (cpl_size)dy + hsize;
        if (zone[3] > ny) continue;

        stdev = cpl_image_get_stdev_window(diff, zone[0], zone[2],
                                           zone[1], zone[3]);

        if (!cpl_errorstate_is_equal(prestate)) {
            /* All pixels could be bad */
            cpl_errorstate_set(prestate);
            continue;
        }

        cpl_vector_set(rms_list, rsize, stdev);
        rsize++;
        /* At this point rms_list has rsize elements */
    }

    cpl_bivector_delete(sample_reg);

    if (4 * rsize < nsamp) {
        cpl_vector_delete(rms_list);
        return cpl_error_set_message_(CPL_ERROR_DATA_NOT_FOUND,
                                      "Need at least %" CPL_SIZE_FORMAT "/4 "
                                      "(not %" CPL_SIZE_FORMAT ") samples"
                                      " to compute noise", nsamp, rsize);
    }

    /* Ignore all but the first rsize elements */
    rms_list = cpl_vector_wrap(rsize, (double*)cpl_vector_unwrap(rms_list));

    /* The error is the rms of the rms */
    if (error != NULL) *error = cpl_vector_get_stdev(rms_list);
    
    /* The final computed RMS is the median of all values.  */
    /* This call will modify the rms_list */
    *noise = cpl_vector_get_median(rms_list);
    
    cpl_vector_delete(rms_list);

    return CPL_ERROR_NONE;
}
#undef RING_RON_HLX
#undef RING_RON_HLY
#undef RING_RON_SAMPLES

#define CPL_OPERATION CPL_DET_CLEAN_BAD_PIX
/*----------------------------------------------------------------------------*/
/**
  @brief    Interpolate any bad pixels in an image and delete the bad pixel map
  @param    self    The image to clean
  @return   The #_cpl_error_code_ or CPL_ERROR_NONE

  The value of a bad pixel is interpolated from the good pixels among the
  8 nearest. (If all but one of the eight neighboring pixels are bad, the
  interpolation becomes a nearest neighbor interpolation). For integer
  images the interpolation in done with floating-point and rounded to the
  nearest integer.

  If there are pixels for which all of the eight neighboring pixels are bad,
  a subsequent interpolation pass is done, where the already interpolated
  pixels are included as source for the interpolation.

  The interpolation passes are repeated until all bad pixels have been
  interpolated. In the worst case, all pixels will be interpolated from a
  single good pixel.
  
  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_DATA_NOT_FOUND if all pixels are bad
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_detector_interpolate_rejected(cpl_image * self) 
{
    const cpl_mask * mask = cpl_image_get_bpm_const(self);

    if (mask != NULL) {
        const cpl_size nx = cpl_image_get_size_x(self);
        const cpl_size ny = cpl_image_get_size_y(self);
        const cpl_binary * bpm = cpl_mask_get_data_const(mask);
        cpl_binary * doit = memchr(bpm, CPL_BINARY_1,
                                   (size_t)nx * (size_t)ny * sizeof(*bpm));

        if (doit) {

            /* Switch on image type */
            switch (cpl_image_get_type(self)) {
            case CPL_TYPE_DOUBLE:
                cpl_ensure_code(!cpl_detector_interpolate_rejected_double
                                ((double*)cpl_image_get_data(self), bpm, nx, ny,
                                 doit), cpl_error_get_code());
                break;
            case CPL_TYPE_FLOAT:
                cpl_ensure_code(!cpl_detector_interpolate_rejected_float
                                ((float*)cpl_image_get_data(self), bpm, nx, ny,
                                 doit), cpl_error_get_code());
                break;
            case CPL_TYPE_INT:
                cpl_ensure_code(!cpl_detector_interpolate_rejected_int
                                ((int*)cpl_image_get_data(self), bpm, nx, ny,
                                 doit), cpl_error_get_code());
                break;
            case CPL_TYPE_DOUBLE_COMPLEX:
                cpl_ensure_code(!cpl_detector_interpolate_rejected_double_complex
                                ((double complex*)cpl_image_get_data(self),
                                 bpm, nx, ny, doit), cpl_error_get_code());
                break;
            case CPL_TYPE_FLOAT_COMPLEX:
                cpl_ensure_code(!cpl_detector_interpolate_rejected_float_complex
                                ((float complex*)cpl_image_get_data(self),
                                 bpm, nx, ny, doit), cpl_error_get_code());
                break;
            default:
                return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            }
        }

        (void)cpl_image_accept_all(self); /* bpm could be empty */
    } else if (self == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    return CPL_ERROR_NONE;
}
#undef CPL_OPERATION

/**@}*/

/*----------------------------------------------------------------------------*/
/*
  @brief    Generate points with a Poisson scattering property in a rectangle.
  @param    r       Array of 4 integers as [xmin,xmax,ymin,ymax]
  @param    np      Number of points to generate.
  @param    homog   Homogeneity factor.
  @return   Newly allocated cpl_bivector object or NULL in error case
  @note No NULL-pointer check in this internal function

  The returned object must be deallocated using cpl_bivector_delete().

  <b>POISSON POINT GENERATION</b>

  Without homogeneity factor, the idea is to generate a set of np points within
  a given rectangle defined by (xmin xmax ymin ymax).
  All these points obey a Poisson law, i.e. no couple of points is closer to
  each other than a minimal distance. This minimal distance is defined as a
  function of the input requested rectangle and the requested number of points
  to generate. We apply the following formula:

  @f$ d_{min} = \sqrt{\frac{W \times H}{\sqrt{2}}} @f$

  Where W and H stand for the rectangle width and height. Notice that the
  system in which the rectangle vertices are given is completely left
  unspecified, generated points will have coordinates in the specified x and y
  ranges.

  With a specified homogeneity factor h (0 < h <= np), the generation algorithm
  is different.

  the Poisson law applies for any h consecutive points in the final output, but
  not for the whole point set. This enables us to generate groups of points
  which statisfy the Poisson law, without constraining the whole set. This
  actually is equivalent to dividing the rectangle in h regions of equal
  surface, and generate points randomly in each of these regions, changing
  region at each point.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_ILLEGAL_INPUT if np is not strictly positive
 */
/*----------------------------------------------------------------------------*/
static cpl_bivector * cpl_bivector_gen_rect_poisson(const cpl_size * r,
                                                    const cpl_size   np,
                                                    const cpl_size   homog)
{
    double              min_dist;
    cpl_size            i;
    cpl_size            gnp;
    cpl_bivector    *   list;
    double              cand_x, cand_y;
    cpl_boolean         ok;
    cpl_size            start_ndx;
    cpl_size            xmin, xmax, ymin, ymax;
    /* Corrected Homogeneity factor */
    const cpl_size      homogc = 0 < homog && homog < np ? homog : np;
    double          *   px;
    double          *   py;

    /* error handling: test arguments are correct */
    cpl_ensure( np > 0, CPL_ERROR_ILLEGAL_INPUT, NULL);

    list = cpl_bivector_new(np);
    cpl_ensure(list, CPL_ERROR_NULL_INPUT,NULL);
    px = cpl_bivector_get_x_data(list);
    py = cpl_bivector_get_y_data(list);

    xmin = r[0];
    xmax = r[1];
    ymin = r[2];
    ymax = r[3];

    min_dist   = CPL_MATH_SQRT1_2*(double)((xmax-xmin)*(ymax-ymin))
        / (double)(homogc+1);
    gnp        = 1;
    px[0]      = 0;
    py[0]      = 0;

    /* First: generate <homog> points */
    while (gnp < homogc) {
        /* Pick a random point within requested range */
        cand_x = cpl_drand() * (double)(xmax - xmin) + (double)xmin;
        cand_y = cpl_drand() * (double)(ymax - ymin) + (double)ymin;

        /* Check the candidate obeys the minimal Poisson distance */
        ok = CPL_TRUE;
        for (i=0; i<gnp; i++) {
            if (pdist(cand_x, cand_y, px[i], py[i])<min_dist) {
                /* does not check Poisson law: reject point */
                ok = CPL_FALSE;
                break;
            }
        }
        if (ok) {
            /* obeys Poisson law: register the point as valid */
            px[gnp] = cand_x;
            py[gnp] = cand_y;
            gnp ++;
        }
    }

    /* Iterative process: */
    /* Pick points out of Poisson distance of the last <homogc-1> points. */
    start_ndx=0;
    while (gnp < np) {
        /* Pick a random point within requested range */
        cand_x = cpl_drand() * (double)(xmax - xmin) + (double)xmin;
        cand_y = cpl_drand() * (double)(ymax - ymin) + (double)ymin;

        /* Check the candidate obeys the minimal Poisson distance */
        ok = CPL_TRUE;
        for (i=0; i<homogc; i++) {
            if (pdist(cand_x,
                      cand_y,
                      px[start_ndx+i],
                      py[start_ndx+i]) < min_dist) {
                /* does not check Poisson law: reject point */
                ok = CPL_FALSE;
                break;
            }
        }
        if (ok) {
            /* obeys Poisson law: register the point as valid */
            px[gnp] = cand_x;
            py[gnp] = cand_y;
            gnp ++;
        }
    }

    /* Iterative process: */
    /* Pick points out of Poisson distance of the last <homogc-1> points. */
    start_ndx=0;
    while (gnp < np) {
        /* Pick a random point within requested range */
        cand_x = cpl_drand() * (double)(xmax - xmin) + (double)xmin;
        cand_y = cpl_drand() * (double)(ymax - ymin) + (double)ymin;

        /* Check the candidate obeys the minimal Poisson distance */
        ok = CPL_TRUE;
        for (i=0; i<homogc; i++) {
            if (pdist(cand_x,
                      cand_y,
                      px[start_ndx+i],
                      py[start_ndx+i]) < min_dist) {
                /* does not check Poisson law: reject point */
                ok = CPL_FALSE;
                break;
            }
        }
        if (ok) {
            /* obeys Poisson law: register the point as valid */
            px[gnp] = cand_x;
            py[gnp] = cand_y;
            gnp ++;
            start_ndx ++;
        }
    }
    return list;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Generate points with a Poisson scattering property in a ring.
  @param    r       Array of 4 doubles as [x,y,r1,r2]
  @param    np      Number of points to generate.
  @param    homog   Homogeneity factor.
  @return   Newly allocated cpl_bivector object or NULL in error case
  @see      cpl_bivector_gen_rect_poisson
  @note The points are in polar coordinates with radians.
  @note No NULL-pointer check in this internal function

  The returned object must be deallocated using cpl_bivector_delete().

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_ILLEGAL_INPUT if np is not strictly positive
 */
/*----------------------------------------------------------------------------*/
static cpl_bivector * cpl_bivector_gen_ring_poisson(
        const double    *   r,
        const cpl_size      np,
        const cpl_size      homog)
{
    double              min_dist;
    cpl_size            gnp;
    cpl_bivector    *   list = cpl_bivector_new(np);
    double              cand_r, cand_t;
    cpl_boolean         ok;
    cpl_size            start_ndx;
    cpl_size            r1, r2;
    cpl_size            i;
    /* Corrected Homogeneity factor */
    const cpl_size      homogc = 0 < homog && homog < np ? homog : np;
    double          *   px = cpl_bivector_get_x_data(list);
    double          *   py = cpl_bivector_get_y_data(list);

    /* Check entries */
    cpl_ensure(list != NULL, CPL_ERROR_ILLEGAL_INPUT, NULL);

    r1 = (cpl_size)r[2];
    r2 = (cpl_size)r[3];

    min_dist    = (CPL_MATH_PI_2/CPL_MATH_SQRT1_2)*(double)((r2*r2)-(r1*r1))
        / (double)(homogc+1);
    gnp         = 1;
    px[0]       = (double)r1;
    py[0]       = 0.0;

    /* First: generate <homog> points */
    while (gnp < homogc) {
        /* Pick a random point within requested range */
        cand_r = cpl_drand() * (double)(r2 - r1) + (double)r1;
        cand_t = cpl_drand() * CPL_MATH_2PI;

        /* Check the candidate obeys the minimal Poisson distance */
        ok = CPL_TRUE;
        for (i=0; i<gnp; i++) {
            if (qdist(cand_r, cand_t, px[i], py[i]) < min_dist) {
                /* does not check Poisson law: reject point */
                ok = CPL_FALSE;
                break;
            }
        }
        if (ok) {
            /* obeys Poisson law: register the point as valid */
            px[gnp] = cand_r;
            py[gnp] = cand_t;
            gnp ++;
        }
    }

    /* Iterative process: */
    /* Pick points out of Poisson distance of the last <homogc-1> points. */
    start_ndx = 0;
    while (gnp < np) {
        /* Pick a random point within requested range */
        cand_r = cpl_drand() * (double)(r2 - r1) + (double)r1;
        cand_t = cpl_drand() * CPL_MATH_2PI;

        /* Check the candidate obeys the minimal Poisson distance */
        ok = CPL_TRUE;
        for (i=0; i<homogc; i++) {
            if (qdist(cand_r,
                      cand_t,
                      px[start_ndx+i],
                      py[start_ndx+i]) < min_dist) {
                /* does not check Poisson law: reject point */
                ok = CPL_FALSE;
                break;
            }
        }
        if (ok) {
            /* obeys Poisson law: register the point as valid */
            px[gnp] = cand_r;
            py[gnp] = cand_t;
            gnp ++;
            start_ndx ++;
        }
    }
    return list;
}
