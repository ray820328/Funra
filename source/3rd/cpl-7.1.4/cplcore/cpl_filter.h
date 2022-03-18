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

#ifndef CPL_FILTER_H
#define CPL_FILTER_H

#include <cpl_macros.h>

CPL_BEGIN_DECLS

/*----------------------------------------------------------------------------*/
/**
 * @defgroup cpl_filter Filters
 *
 * This module provides definitions for filtering of a
 * @c cpl_image and a @c cpl_mask. The actual filtering functions are defined
 * in the @c cpl_image and @c cpl_mask modules.
 *
 * @par Synopsis:
 * @code
 *   #include "cpl_filter.h"
 * @endcode
 */
/*----------------------------------------------------------------------------*/

/**@{*/

/*-----------------------------------------------------------------------------
                                   New types
 -----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/**
 * 
 * @brief These are the supported border modes.
 * For a kernel of width 2n+1, the n left- and rightmost image/mask
 * columns do not have elements for the whole kernel. The same holds for the
 * top and bottom image/mask rows. The border mode defines the filtering of
 * such border pixels.
 * 
 */
/*----------------------------------------------------------------------------*/

enum _cpl_border_mode_
{
    CPL_BORDER_FILTER,
    /**<
         Filter the border using the reduced number of pixels. If in median
         filtering the number of pixels is even choose the mean of the two
         central values, after the borders have been filled with a chess-like
         pattern of +- inf 
    */

    CPL_BORDER_ZERO,
    /**<
         Set the border of the filtered image/mask to zero.
    */

    CPL_BORDER_CROP,
    /**<
         Crop the filtered image/mask.
    */

    CPL_BORDER_NOP,
    /**<
         Do not modify the border of the filtered image/mask.
    */

    CPL_BORDER_COPY
    /**<
         Copy the border of the input image/mask. For an in-place operation
         this has the no effect, identical to CPL_BORDER_NOP.
    */

};

/**
 * @brief
 *   The border mode type.
 */

typedef enum _cpl_border_mode_ cpl_border_mode;


/*----------------------------------------------------------------------------*/
/**
 * 
 * @brief These are the supported filter modes.
 * 
 */
/*----------------------------------------------------------------------------*/

enum _cpl_filter_mode_
{

    CPL_FILTER_EROSION,
    /**<
      The erosion filter (for a @c cpl_mask).

       @see cpl_mask_filter()
    */

    CPL_FILTER_DILATION,
    /**<
      The dilation filter (for a @c cpl_mask).

       @see cpl_mask_filter()
    */

    CPL_FILTER_OPENING,
    /**<
         The opening filter (for a @c cpl_mask).

       @see cpl_mask_filter()
    */

    CPL_FILTER_CLOSING,
    /**<
         The closing filter (for a @c cpl_mask).

       @see cpl_mask_filter()
    */

    CPL_FILTER_LINEAR,
    /**<
        A linear filter (for a @c cpl_image). The kernel elements are normalized
        with the sum of their absolute values. This implies that there must be
        at least one non-zero element in the kernel. The normalisation makes the
        kernel useful for filtering where flux conservation is desired.

        The kernel elements are thus used as weights like this:
    @verbatim
        Kernel           Image
                                 ...
           1 2 3         ... 1.0 2.0 3.0 ...
           4 5 6         ... 4.0 5.0 6.0 ...
           7 8 9         ... 7.0 8.0 9.0 ...
                                 ...
    @endverbatim
         The filtered value corresponding to the pixel whose value is 5.0 is:
         \f$\frac{(1*1.0+2*2.0+3*3.0+4*4.0+5*5.0+6*6.0+7*7.0+8*8.0+9*9.0)}
                 {1+2+3+4+5+6+7+8+9}\f$

       Filtering with @c CPL_FILTER_LINEAR and a flat kernel can be done faster
       with @c CPL_FILTER_AVERAGE.

       @see CPL_FILTER_LINEAR_SCALE, CPL_FILTER_AVERAGE, cpl_image_filter()
    */

    CPL_FILTER_LINEAR_SCALE,
    /**<
        A linear filter (for a @c cpl_image). Unlike @c CPL_FILTER_LINEAR the
        kernel elements are not normalized, so the filtered image will have
        its flux scaled with the sum of the weights of the kernel. Examples
        of linear, scaling kernels are gradient operators and edge detectors.

        The kernel elements are thus applied like this:
    @verbatim
        Kernel           Image
                                 ...
           1 2 3         ... 1.0 2.0 3.0 ...
           4 5 6         ... 4.0 5.0 6.0 ...
           7 8 9         ... 7.0 8.0 9.0 ...
                                 ...
    @endverbatim
         The filtered value corresponding to the pixel whose value is 5.0 is:
         \f$1*1.0+2*2.0+3*3.0+4*4.0+5*5.0+6*6.0+7*7.0+8*8.0+9*9.0\f$

       @see CPL_FILTER_LINEAR, cpl_image_filter()
    */

    CPL_FILTER_AVERAGE,
    /**<
         An average filter, i.e. the output pixel is the
         arithmetic average of the surrounding (1 + 2 * hsizex)
         * (1 + 2 * hsizey) pixels.
         The cost per pixel is O(hsizex*hsizey).
         The two images may have different pixel types.
         When the input and output pixel types
         are identical, the arithmetic is done with that type,
         e.g. int for two integer images. When the input and
         output pixel types differ, the arithmetic is done in
         double precision when one of the two images have
         pixel type CPL_TYPE_DOUBLE, otherwise float is used.

       @see CPL_FILTER_AVERAGE_FAST, cpl_image_filter_mask()
    */

    CPL_FILTER_AVERAGE_FAST,
    /**<
         The same as @c CPL_FILTER_AVERAGE, except that it uses a running
         average, which will lead to a significant loss of precision if
         there are large differences in the magnitudes of the input pixels.
         The cost per pixel is O(1) if all elements in the kernel are used,
         otherwise the filtering is done as for CPL_FILTER_AVERAGE.

       @see cpl_image_filter_mask()
    */

    CPL_FILTER_MEDIAN,
    /**<
         A median filter (for a @c cpl_image). The pixel types of the input
         and output images must be identical.

       @see cpl_image_filter_mask()
    */

    CPL_FILTER_STDEV,
    /**<
         The filtered value is the standard deviation of the included
         input pixels.
 
       @verbatim
        Kernel                 Image
                                       ...
              1   0   1           ... 1.0 2.0 3.0 ...
              0   1   0           ... 4.0 5.0 6.0 ...
              1   0   1           ... 7.0 8.0 9.0 ...
                                          ...
       @endverbatim
         The pixel with value 5.0 will have a filtered value of:
         std_dev(1.0, 3.0, 5.0, 7.0, 9.0)
 
       @see CPL_FILTER_STDEV_FAST, cpl_image_filter_mask()
    */

    CPL_FILTER_STDEV_FAST,
    /**<
         The same as @c CPL_FILTER_STDEV, except that it uses the same running
         method employed in @c CPL_FILTER_AVERAGE_FAST, which will lead to a
         significant loss of precision if there are large differences in the
         magnitudes of the input pixels. As for @c CPL_FILTER_AVERAGE_FAST, the
         cost per pixel is O(1) if all elements are used, otherwise the
         filtering is done as for @c CPL_FILTER_STDEV.

       @see cpl_image_filter_mask()
    */

    CPL_FILTER_MORPHO,
    /**<
        A morphological filter (for a @c cpl_image). The kernel elements are
        normalized with the sum of their absolute values. This implies that
        there must be at least one non-zero element in the kernel. The
        normalisation makes the kernel useful for filtering where flux
        conservation is desired.

        The kernel elements are used as weights on the sorted values covered by
        the kernel:
    @verbatim
        Kernel           Image
                                 ...
           1 2 3         ... 4.0 6.0 5.0 ...
           4 5 6         ... 3.0 1.0 2.0 ...
           7 8 9         ... 7.0 8.0 9.0 ...
                                 ...
    @endverbatim
        The filtered value corresponding to the pixel whose value is 5.0 is:
        \f$\frac{(1*1.0+2*2.0+3*3.0+4*4.0+5*5.0+6*6.0+7*7.0+8*8.0+9*9.0)}{1+2+3+4+5+6+7+8+9}\f$

       @see CPL_FILTER_MORPHO_SCALE, cpl_image_filter()
    */

    CPL_FILTER_MORPHO_SCALE
    /**<
        A morphological filter (for a @c cpl_image). Unlike @c CPL_FILTER_MORPHO
        the kernel elements are not normalized, so the filtered image will have
        its flux scaled with the sum of the weights of the kernel.

        The kernel elements are thus applied to the sorted values covered by
        the kernel:
    @verbatim
        Kernel           Image
                                 ...
           1 2 3         ... 4.0 6.0 5.0 ...
           4 5 6         ... 3.0 1.0 2.0 ...
           7 8 9         ... 7.0 8.0 9.0 ...
                                 ...
    @endverbatim
        The filtered value corresponding to the pixel whose value is 5.0 is:
        \f$1*1.0+2*2.0+3*3.0+4*4.0+5*5.0+6*6.0+7*7.0+8*8.0+9*9.0\f$

        @see CPL_FILTER_MORPHO, cpl_image_filter()
    */

};

/**
 * @brief
 *   The filter mode type.
 */

typedef enum _cpl_filter_mode_ cpl_filter_mode;

/**@}*/

CPL_END_DECLS

#endif 
