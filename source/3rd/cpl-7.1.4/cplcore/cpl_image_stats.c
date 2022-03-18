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
#include "cpl_stats_impl.h"
#include "cpl_error_impl.h"
#include "cpl_image_bpm.h"
#include "cpl_image_stats.h"
#include "cpl_mask.h"
#include "cpl_tools.h"

#include "cpl_image_defs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <float.h>

/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief    Define the body of a stats window function
  @param    op   Operation in lower case
  @param    OP   Operation in upper case
 */ 
/*----------------------------------------------------------------------------*/
#define CPL_IMAGE_GET_STATS_WINDOW_DOUBLE_BODY(op, OP)                  \
                                                                        \
    cpl_stats stats;                                                    \
    const cpl_error_code icode =                                        \
        cpl_stats_fill_from_image_window(&stats, image,                 \
                                         CPL_CONCAT2X(CPL_STATS, OP),   \
                                         llx, lly,  urx, ury);          \
    double res = 0.0;                                                   \
                                                                        \
    if (icode) {                                                        \
        (void)cpl_error_set_where_();                                   \
    } else {                                                            \
        /* Should not be able to fail now */                            \
        res = CPL_CONCAT2X(cpl_stats_get, op)((const cpl_stats *)&stats); \
    }                                                                   \
                                                                        \
    return res                                                     

/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief    Define the body of a stats function
  @param    op   Operation in lower case
  @param    OP   Operation in upper case
*/ 
/*----------------------------------------------------------------------------*/
#define CPL_IMAGE_GET_STATS_DOUBLE_BODY(op, OP)                         \
                                                                        \
    cpl_stats stats;                                                    \
    const cpl_error_code icode =                                        \
        cpl_stats_fill_from_image(&stats, image, CPL_CONCAT2X(CPL_STATS, OP)); \
    double res = 0.0;                                                   \
                                                                        \
                                                                        \
    if (icode) {                                                        \
        (void)cpl_error_set_where_();                                   \
    } else {                                                            \
        /* Should not be able to fail now */                            \
        res = CPL_CONCAT2X(cpl_stats_get, op)((const cpl_stats *)&stats); \
    }                                                                   \
                                                                        \
                                                                        \
    return res                                                     

/*-----------------------------------------------------------------------------
                            Function codes
 -----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    computes minimum pixel value over an image sub-window.
  @param    image       input image.
  @param    llx         Lower left x position (FITS convention, 1 for leftmost)
  @param    lly         Lower left y position (FITS convention, 1 for lowest)
  @param    urx         Upper right x position (FITS convention)
  @param    ury         Upper right y position (FITS convention)
  @return   the minimum value, or undefined on error
  @see      cpl_stats_new_from_window()
  @note In case of error, the #_cpl_error_code_ code is set.

  The specified bounds are included in the specified region.
  
  Images can be CPL_TYPE_FLOAT, CPL_TYPE_INT or CPL_TYPE_DOUBLE.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */ 
/*----------------------------------------------------------------------------*/
double cpl_image_get_min_window(const cpl_image * image,
                                cpl_size          llx,
                                cpl_size          lly,
                                cpl_size          urx,
                                cpl_size          ury)
{
    CPL_IMAGE_GET_STATS_WINDOW_DOUBLE_BODY(min, MIN);
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    computes minimum pixel value over an image.
  @param    image       input image.
  @return   the minimum value
  @see      cpl_image_get_min_window()

  In case of error, the #_cpl_error_code_ code is set, and the returned double
  is undefined.
  Images can be CPL_TYPE_FLOAT, CPL_TYPE_INT or CPL_TYPE_DOUBLE.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */ 
/*----------------------------------------------------------------------------*/
double cpl_image_get_min(const cpl_image * image)
{
    CPL_IMAGE_GET_STATS_DOUBLE_BODY(min, MIN);
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    computes maximum pixel value over an image sub-window.
  @param    image       input image.
  @param    llx         Lower left x position (FITS convention, 1 for leftmost)
  @param    lly         Lower left y position (FITS convention, 1 for lowest)
  @param    urx         Upper right x position (FITS convention)
  @param    ury         Upper right y position (FITS convention)
  @return   the maximum value
  @see      cpl_image_get_min_window()
 */ 
/*----------------------------------------------------------------------------*/
double cpl_image_get_max_window(const cpl_image * image,
                                cpl_size          llx,
                                cpl_size          lly,
                                cpl_size          urx,
                                cpl_size          ury)
{
    CPL_IMAGE_GET_STATS_WINDOW_DOUBLE_BODY(max, MAX);
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    computes maximum pixel value over an image.
  @param    image       input image.
  @return   the maximum value
  @see      cpl_image_get_min()
 */ 
/*----------------------------------------------------------------------------*/
double cpl_image_get_max(const cpl_image * image)
{
    CPL_IMAGE_GET_STATS_DOUBLE_BODY(max, MAX);
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    computes mean pixel value over an image sub-window.
  @param    image       input image.
  @param    llx         Lower left x position (FITS convention, 1 for leftmost)
  @param    lly         Lower left y position (FITS convention, 1 for lowest)
  @param    urx         Upper right x position (FITS convention)
  @param    ury         Upper right y position (FITS convention)
  @return   the mean value
  @see      cpl_image_get_min_window()
 */ 
/*----------------------------------------------------------------------------*/
double cpl_image_get_mean_window(
        const cpl_image   *   image,
        cpl_size        llx,
        cpl_size        lly,
        cpl_size        urx,
        cpl_size        ury)
{
    CPL_IMAGE_GET_STATS_WINDOW_DOUBLE_BODY(mean, MEAN);
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    computes mean pixel value over an image.
  @param    image       input image.
  @return   the mean value
  @see      cpl_image_get_min()
 */ 
/*----------------------------------------------------------------------------*/
double cpl_image_get_mean(const cpl_image * image)
{
    CPL_IMAGE_GET_STATS_DOUBLE_BODY(mean, MEAN);
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    computes median pixel value over an image sub-window.
  @param    image       Input image.
  @param    llx         Lower left x position (FITS convention, 1 for leftmost)
  @param    lly         Lower left y position (FITS convention, 1 for lowest)
  @param    urx         Upper right x position (FITS convention)
  @param    ury         Upper right y position (FITS convention)
  @return   The median value, or undefined on error

  The specified bounds are included in the specified region.

  In case of error, the #_cpl_error_code_ code is set, and the returned value
  is undefined.
  Images can be CPL_TYPE_FLOAT, CPL_TYPE_INT or CPL_TYPE_DOUBLE.

  For a finite population or sample, the median is the middle value of an odd
  number of values (arranged in ascending order) or any value between the two
  middle values of an even number of values.
  For an even number of elements in the array, the mean of the two central
  values is returned. Note that in this case, the median might not belong
  to the input array.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the window is outside the image or if
    there are only bad pixels in the window
  - CPL_ERROR_INVALID_TYPE if the passed image type is not supported
 */ 
/*----------------------------------------------------------------------------*/
double cpl_image_get_median_window(const cpl_image * image,
                                   cpl_size          llx,
                                   cpl_size          lly,
                                   cpl_size          urx,
                                   cpl_size          ury)
{
    CPL_IMAGE_GET_STATS_WINDOW_DOUBLE_BODY(median, MEDIAN);
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    computes median pixel value over an image.
  @param    image   Input image.
  @return   the median value
  @see      cpl_image_get_median_window()

  In case of error, the #_cpl_error_code_ code is set, and the returned double
  is undefined.
  Images can be CPL_TYPE_FLOAT, CPL_TYPE_INT or CPL_TYPE_DOUBLE.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */ 
/*----------------------------------------------------------------------------*/
double cpl_image_get_median(const cpl_image * image)
{
    CPL_IMAGE_GET_STATS_DOUBLE_BODY(median, MEDIAN);
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    computes pixel standard deviation over an image sub-window.
  @param    image       input image.
  @param    llx         Lower left x position (FITS convention, 1 for leftmost)
  @param    lly         Lower left y position (FITS convention, 1 for lowest)
  @param    urx         Upper right x position (FITS convention)
  @param    ury         Upper right y position (FITS convention)
  @return   the standard deviation value
  @see      cpl_image_get_min_window()
 */ 
/*----------------------------------------------------------------------------*/
double cpl_image_get_stdev_window(const cpl_image * image,
                                  cpl_size          llx,
                                  cpl_size          lly,
                                  cpl_size          urx,
                                  cpl_size          ury)
{
    CPL_IMAGE_GET_STATS_WINDOW_DOUBLE_BODY(stdev, STDEV);
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    computes pixel standard deviation over an image.
  @param    image       input image.
  @return   the standard deviation value
  @see      cpl_image_get_min()
 */ 
/*----------------------------------------------------------------------------*/
double cpl_image_get_stdev(const cpl_image * image)
{
    CPL_IMAGE_GET_STATS_DOUBLE_BODY(stdev, STDEV);
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Computes the sum of pixel values over an image sub-window
  @param    image       input image.
  @param    llx         Lower left x position (FITS convention, 1 for leftmost)
  @param    lly         Lower left y position (FITS convention, 1 for lowest)
  @param    urx         Upper right x position (FITS convention)
  @param    ury         Upper right y position (FITS convention)
  @return   the flux value
  @see      cpl_image_get_min_window()
 */ 
/*----------------------------------------------------------------------------*/
double cpl_image_get_flux_window(const cpl_image * image,
                                 cpl_size          llx,
                                 cpl_size          lly,
                                 cpl_size          urx,
                                 cpl_size          ury)
{
    CPL_IMAGE_GET_STATS_WINDOW_DOUBLE_BODY(flux, FLUX);
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Computes the sum of pixel values over an image.
  @param    image       input image.
  @return   the flux value
  @see      cpl_image_get_min()
 */ 
/*----------------------------------------------------------------------------*/
double cpl_image_get_flux(const cpl_image * image)
{
    cpl_ensure(image, CPL_ERROR_NULL_INPUT, 0.0);
    return cpl_image_get_flux_window(image, 1, 1, image->nx, image->ny);
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Computes the sum of absolute values over an image sub-window
  @param    image       input image.
  @param    llx         Lower left x position (FITS convention, 1 for leftmost)
  @param    lly         Lower left y position (FITS convention, 1 for lowest)
  @param    urx         Upper right x position (FITS convention)
  @param    ury         Upper right y position (FITS convention)
  @return   the absolute flux (sum of |pixels|) value
  @see      cpl_image_get_min_window()
 */ 
/*----------------------------------------------------------------------------*/
double cpl_image_get_absflux_window(const cpl_image * image,
                                    cpl_size          llx,
                                    cpl_size          lly,
                                    cpl_size          urx,
                                    cpl_size          ury)
{
    CPL_IMAGE_GET_STATS_WINDOW_DOUBLE_BODY(absflux, ABSFLUX);
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Computes the sum of squared values over an image sub-window
  @param    image       input image.
  @param    llx         Lower left x position (FITS convention, 1 for leftmost)
  @param    lly         Lower left y position (FITS convention, 1 for lowest)
  @param    urx         Upper right x position (FITS convention)
  @param    ury         Upper right y position (FITS convention)
  @return   the square flux
  @see      cpl_image_get_min_window()
 */ 
/*----------------------------------------------------------------------------*/
double cpl_image_get_sqflux_window(
        const cpl_image   *   image,
        cpl_size        llx,
        cpl_size        lly,
        cpl_size        urx,
        cpl_size        ury)
{
    CPL_IMAGE_GET_STATS_WINDOW_DOUBLE_BODY(sqflux, SQFLUX);
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Computes the sum of absolute values over an image
  @param    image       input image.
  @return   the absolute flux (sum of |pixels|) value
  @see      cpl_image_get_min()
 */ 
/*----------------------------------------------------------------------------*/
double cpl_image_get_absflux(const cpl_image * image)
{
    CPL_IMAGE_GET_STATS_DOUBLE_BODY(absflux, ABSFLUX);
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Computes the sum of squared values over an image
  @param    image       input image.
  @return   the sqaure flux
  @see      cpl_image_get_min()
 */ 
/*----------------------------------------------------------------------------*/
double cpl_image_get_sqflux(const cpl_image * image)
{
    CPL_IMAGE_GET_STATS_DOUBLE_BODY(sqflux, SQFLUX);
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Computes the x centroid value over the whole image
  @param    image       input image.
  @return   the x centroid value
  @see      cpl_image_get_min_window()
 */ 
/*----------------------------------------------------------------------------*/
double cpl_image_get_centroid_x(const cpl_image * image)
{
    CPL_IMAGE_GET_STATS_DOUBLE_BODY(centroid_x, CENTROID);
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Computes the x centroid value over an image sub-window
  @param    image       input image.
  @param    llx         Lower left x position (FITS convention, 1 for leftmost)
  @param    lly         Lower left y position (FITS convention, 1 for lowest)
  @param    urx         Upper right x position (FITS convention)
  @param    ury         Upper right y position (FITS convention)
  @return   the x centroid value
  @see      cpl_image_get_min_window()
 */ 
/*----------------------------------------------------------------------------*/
double cpl_image_get_centroid_x_window(const cpl_image * image,
                                       cpl_size          llx,
                                       cpl_size          lly,
                                       cpl_size          urx,
                                       cpl_size          ury)
{
    CPL_IMAGE_GET_STATS_WINDOW_DOUBLE_BODY(centroid_x, CENTROID);
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Computes the y centroid value over the whole image
  @param    image       input image.
  @return   the y centroid value
  @see      cpl_image_get_min_window()
 */ 
/*----------------------------------------------------------------------------*/
double cpl_image_get_centroid_y(const cpl_image * image)
{
    CPL_IMAGE_GET_STATS_DOUBLE_BODY(centroid_y, CENTROID);
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Computes the y centroid value over an image sub-window
  @param    image       input image.
  @param    llx         Lower left x position (FITS convention, 1 for leftmost)
  @param    lly         Lower left y position (FITS convention, 1 for lowest)
  @param    urx         Upper right x position (FITS convention)
  @param    ury         Upper right y position (FITS convention)
  @return   the y centroid value
  @see      cpl_image_get_min_window()
 */ 
/*----------------------------------------------------------------------------*/
double cpl_image_get_centroid_y_window(const cpl_image * image,
                                       cpl_size          llx,
                                       cpl_size          lly,
                                       cpl_size          urx,
                                       cpl_size          ury)
{
    CPL_IMAGE_GET_STATS_WINDOW_DOUBLE_BODY(centroid_y, CENTROID);
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Computes minimum pixel value and position over an image sub window.
  @param    image       Input image.
  @param    llx         Lower left x position (FITS convention, 1 for leftmost)
  @param    lly         Lower left y position (FITS convention, 1 for lowest)
  @param    urx         Upper right x position (FITS convention)
  @param    ury         Upper right y position (FITS convention)
  @param    px          ptr to the x coordinate of the minimum position
  @param    py          ptr to the y coordinate of the minimum position
  @return   CPL_ERROR_NONE or the #_cpl_error_code_ on error
  @see      cpl_image_get_min_window()

  The specified bounds are included in the specified region.

  Images can be CPL_TYPE_FLOAT, CPL_TYPE_INT or CPL_TYPE_DOUBLE.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
*/
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_get_minpos_window(const cpl_image * image,
                                           cpl_size          llx,
                                           cpl_size          lly,
                                           cpl_size          urx,
                                           cpl_size          ury,
                                           cpl_size        * px,
                                           cpl_size        * py)
{
    cpl_stats stats;

    cpl_ensure_code(px != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(py != NULL, CPL_ERROR_NULL_INPUT);

    if (cpl_stats_fill_from_image_window(&stats, image, CPL_STATS_MINPOS,
                                         llx, lly,  urx, ury)) {
        return cpl_error_set_where_();
    }
                                                                        
    /* Should not be able to fail now */

    *px = cpl_stats_get_min_x(&stats);
    *py = cpl_stats_get_min_y(&stats);

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Computes minimum pixel value and position over an image.
  @param    image       Input image.
  @param    px          ptr to the x coordinate of the minimum position
  @param    py          ptr to the y coordinate of the minimum position
  @return   CPL_ERROR_NONE or the #_cpl_error_code_ on error
  @see      cpl_image_get_minpos_window()

  Images can be CPL_TYPE_FLOAT, CPL_TYPE_INT or CPL_TYPE_DOUBLE.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_get_minpos(const cpl_image * image,
                                    cpl_size        * px,
                                    cpl_size        * py)
{

    cpl_stats stats;

    cpl_ensure_code(px != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(py != NULL, CPL_ERROR_NULL_INPUT);

    if (cpl_stats_fill_from_image(&stats, image, CPL_STATS_MINPOS)) {
        return cpl_error_set_where_();
    }
                                                                       
    /* Should not be able to fail now */

    *px = cpl_stats_get_min_x(&stats);
    *py = cpl_stats_get_min_y(&stats);

    return CPL_ERROR_NONE;

}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Computes maximum pixel value and position over an image sub window.
  @param    image       Input image.
  @param    llx         Lower left x position (FITS convention, 1 for leftmost)
  @param    lly         Lower left y position (FITS convention, 1 for lowest)
  @param    urx         Upper right x position (FITS convention)
  @param    ury         Upper right y position (FITS convention)
  @param    px          ptr to the x coordinate of the maximum position
  @param    py          ptr to the y coordinate of the maximum position
  @return   CPL_ERROR_NONE or the #_cpl_error_code_ on error
  @see      cpl_image_get_minpos_window()
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_get_maxpos_window(const cpl_image * image,
                                           cpl_size          llx,
                                           cpl_size          lly,
                                           cpl_size          urx,
                                           cpl_size          ury,
                                           cpl_size        * px,
                                           cpl_size        * py)
{
    cpl_stats stats;

    cpl_ensure_code(px != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(py != NULL, CPL_ERROR_NULL_INPUT);

    if (cpl_stats_fill_from_image_window(&stats, image, CPL_STATS_MAXPOS,
                                            llx, lly,  urx, ury)) {
        return cpl_error_set_where_();
    }
                                                                        
    /* Should not be able to fail now */

    *px = cpl_stats_get_max_x(&stats);
    *py = cpl_stats_get_max_y(&stats);

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Computes maximum pixel value and position over an image.
  @param    image       Input image.
  @param    px          ptr to the x coordinate of the maximum position
  @param    py          ptr to the y coordinate of the maximum position
  @return   CPL_ERROR_NONE or the #_cpl_error_code_ on error
  @see      cpl_image_get_minpos()
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_image_get_maxpos(const cpl_image * image,
                                    cpl_size        * px,
                                    cpl_size        * py)
{
    cpl_stats stats;

    cpl_ensure_code(px != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(py != NULL, CPL_ERROR_NULL_INPUT);

    if (cpl_stats_fill_from_image(&stats, image, CPL_STATS_MAXPOS)) {
        return cpl_error_set_where_();
    }
                                                                        
    /* Should not be able to fail now */

    *px = cpl_stats_get_max_x(&stats);
    *py = cpl_stats_get_max_y(&stats);

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief  Computes median and mean absolute median deviation on an image window
  @param  image  Input image.
  @param  llx    Lower left x position (FITS convention, 1 for leftmost)
  @param  lly    Lower left y position (FITS convention, 1 for lowest)
  @param  urx    Upper right x position (FITS convention)
  @param  ury    Upper right y position (FITS convention)
  @param  sigma  The mean of the absolute median deviation of the (good) pixels
  @return The median of the non-bad pixels
  @see    cpl_image_get_mad_window()

  For each non-bad pixel in the window the absolute deviation from the median is
  computed.
  The mean of these absolute deviations in returned via sigma, while the median
  itself is returned by the function.
  The computed median and sigma may be a robust estimate of the mean and standard
  deviation of the pixels. The sigma is however still sensitive to outliers.
  See cpl_image_get_mad_window() for a more robust estimator.

  Images can be CPL_TYPE_FLOAT, CPL_TYPE_INT or CPL_TYPE_DOUBLE.
  On error the #_cpl_error_code_ code is set and the return value is undefined.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INVALID_TYPE if the passed image type is not supported
  - CPL_ERROR_ILLEGAL_INPUT if the specified window is illegal
  - CPL_ERROR_DATA_NOT_FOUND if all the pixels are bad in the image window
 */
/*----------------------------------------------------------------------------*/
double cpl_image_get_median_dev_window(const cpl_image * image,
                                       cpl_size          llx,
                                       cpl_size          lly,
                                       cpl_size          urx,
                                       cpl_size          ury,
                                       double          * sigma)
{
    cpl_stats stats;
    double median;


    cpl_ensure_code(sigma != NULL, CPL_ERROR_NULL_INPUT);

    if (cpl_stats_fill_from_image_window(&stats, image, CPL_STATS_MEDIAN_DEV,
                                            llx, lly,  urx, ury)) {
        return cpl_error_set_where_();
    }
                                                                        
    /* Should not be able to fail now */

    median = cpl_stats_get_median(&stats);
    *sigma = cpl_stats_get_median_dev(&stats);

    return median;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief  Computes median and mean absolute median deviation on an image window
  @param  image  Input image.
  @param  sigma  The mean of the absolute median deviation of the (good) pixels
  @return The median of the non-bad pixels
  @see    cpl_image_get_median_dev_window()

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INVALID_TYPE if the passed image type is not supported
  - CPL_ERROR_DATA_NOT_FOUND if all the pixels are bad in the image
 */
/*----------------------------------------------------------------------------*/
double cpl_image_get_median_dev(const cpl_image * image,
                                double          * sigma)
{
    cpl_stats stats;
    double median;


    cpl_ensure_code(sigma != NULL, CPL_ERROR_NULL_INPUT);

    if (cpl_stats_fill_from_image(&stats, image, CPL_STATS_MEDIAN_DEV)) {
        return cpl_error_set_where_();
    }
                                                                        
    /* Should not be able to fail now */

    median = cpl_stats_get_median(&stats);
    *sigma = cpl_stats_get_median_dev(&stats);

    return median;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief  Computes median and median absolute deviation (MAD) on an image window
  @param  image  Input image.
  @param  llx    Lower left x position (FITS convention, 1 for leftmost)
  @param  lly    Lower left y position (FITS convention, 1 for lowest)
  @param  urx    Upper right x position (FITS convention)
  @param  ury    Upper right y position (FITS convention)
  @param  sigma  The median of the absolute median deviation of the good pixels
  @return The median of the non-bad pixels
  @see    cpl_image_get_median_window(), CPL_MATH_STD_MAD

  For each non-bad pixel in the window the absolute deviation from the median is
  computed.
  The median of these absolute deviations in returned via sigma, while the median
  itself is returned by the function.

  If the pixels are gaussian, the computed sigma is a robust and consistent
  estimate of the standard deviation in the sense that the standard deviation
  is approximately k * MAD, where k is a constant equal to approximately 1.4826.
  CPL defines CPL_MATH_STD_MAD as this scaling constant.

  Images can be CPL_TYPE_FLOAT, CPL_TYPE_INT or CPL_TYPE_DOUBLE.
  On error the #_cpl_error_code_ code is set and the return value is undefined.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INVALID_TYPE if the passed image type is not supported
  - CPL_ERROR_ILLEGAL_INPUT if the specified window is illegal
  - CPL_ERROR_DATA_NOT_FOUND if all the pixels are bad in the image window
 */
/*----------------------------------------------------------------------------*/
double cpl_image_get_mad_window(const cpl_image * image,
                                cpl_size          llx,
                                cpl_size          lly,
                                cpl_size          urx,
                                cpl_size          ury,
                                double          * sigma)
{
    cpl_stats stats;
    double median;


    cpl_ensure_code(sigma != NULL, CPL_ERROR_NULL_INPUT);

    if (cpl_stats_fill_from_image_window(&stats, image, CPL_STATS_MAD,
                                            llx, lly,  urx, ury)) {
        return cpl_error_set_where_();
    }
                                                                        
    /* Should not be able to fail now */

    median = cpl_stats_get_median(&stats);
    *sigma = cpl_stats_get_mad(&stats);

    return median;
}

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief  Computes median and median absolute deviation (MAD) on an image
  @param  image  Input image.
  @param  sigma  The median of the absolute median deviation of the good pixels
  @return The median of the non-bad pixels
  @see    cpl_image_get_mad_window()

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INVALID_TYPE if the passed image type is not supported
  - CPL_ERROR_DATA_NOT_FOUND if all the pixels are bad in the image
 */
/*----------------------------------------------------------------------------*/
double cpl_image_get_mad(const cpl_image * image,
                         double          * sigma)
{
    cpl_stats stats;
    double median;


    cpl_ensure_code(sigma != NULL, CPL_ERROR_NULL_INPUT);

    if (cpl_stats_fill_from_image(&stats, image, CPL_STATS_MAD)) {
        return cpl_error_set_where_();
    }
                                                                        
    /* Should not be able to fail now */

    median = cpl_stats_get_median(&stats);
    *sigma = cpl_stats_get_mad(&stats);

    return median;
}

