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


/*----------------------------------------------------------------------------
                                   Includes
 ----------------------------------------------------------------------------*/

#include "cpl_image_fft_impl.h"

#include "cpl_error_impl.h"

/*---------------------------------------------------------------------------*/
/**
 * @defgroup cpl_fft FFTW wrappers
 *
 * This module provides FFTW wrappers
 *
 * @par Synopsis:
 * @code
 *   #include "cpl_fft.h"
 * @endcode
 */
/*---------------------------------------------------------------------------*/
/**@{*/

/*---------------------------------------------------------------------------*/
/**
  @brief    Perform a FFT operation on an image
  @param  self  Pre-allocated output image to transform to
  @param  other Input image to transform from, use self for in-place transform
  @param  mode  CPL_FFT_FORWARD or CPL_FFT_BACKWARD, optionally CPL_FFT_NOSCALE
  @return CPL_ERROR_NONE or the corresponding #_cpl_error_code_ on error
  
  This function performs an FFT on an image, using FFTW. CPL may be configured
  without this library, in this case an otherwise valid call will set and return
  the error CPL_ERROR_UNSUPPORTED_MODE.

  The input and output images must match in precision level. Integer images are
  not supported.

  In a forward transform the input image may be non-complex. In this case a
  real-to-complex transform is performed. This will only compute the first
  nx/2 + 1 columns of the transform. In this transform it is allowed to pass
  an output image with nx/2 + 1 columns.

  Similarly, in a backward transform the output image may be non-complex. In
  this case a complex-to-real transform is performed. This will only transform
  the first nx/2 + 1 columns of the input. In this transform it is allowed to
  pass an input image with nx/2 + 1 columns.

  Per default the backward transform scales (divides) the result with the
  number of elements transformed (i.e. the number of pixels in the result
  image). This scaling can be turned off with CPL_FFT_NOSCALE.

  If many transformations in the same direction are to be done on data of the
  same size and type, a reduction in the time required to perform the
  transformations can be achieved by adding the flag CPL_FFT_FIND_MEASURE
  to the first transformation.
  For a larger number of transformations a further reduction may be achived
  with the flag CPL_FFT_FIND_PATIENT and for an even larger number of
  transformations a further reduction may be achived with the flag
  CPL_FFT_FIND_EXHAUSTIVE.

  If many transformations are to be done then a reduction in the time required
  to perform the transformations can be achieved by using cpl_fft_imagelist().  

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an image is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the mode is illegal
  - CPL_ERROR_INCOMPATIBLE_INPUT if the image sizes do not match
  - CPL_ERROR_TYPE_MISMATCH if the image types are incompatible with each other
                            or with the transform
  - CPL_ERROR_UNSUPPORTED_MODE if FFTW has not been installed
 */
/*---------------------------------------------------------------------------*/
cpl_error_code cpl_fft_image(cpl_image * self, const cpl_image * other,
                             cpl_fft_mode mode)
{
    return cpl_fft_image_(self, other, mode)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief  Perform a FFT operation on the images in an imagelist
  @param  self  Pre-allocated output imagelist to transform to
  @param  other Input imagelist to transform from
  @param  mode  CPL_FFT_FORWARD or CPL_FFT_BACKWARD, optionally CPL_FFT_NOSCALE
  @return CPL_ERROR_NONE or the corresponding #_cpl_error_code_ on error
  @see cpl_fft_image()

 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_fft_imagelist(cpl_imagelist       * self,
                                 const cpl_imagelist * other,
                                 cpl_fft_mode          mode)
{
    const cpl_size    sizein   = cpl_imagelist_get_size(other);
    /*
      The below pointers hold temporary storage allocated by
      cpl_fft_image__(), the first call (if successful) allocates and
      the last call is responsible for the deallocation (so if the first
      call succeeds the subsequent call(s) are assumed to succeed).

    */
    void * plan   = NULL;
    void * bufin  = NULL;
    void * bufout = NULL;
    cpl_size i;

    cpl_ensure_code(self  != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(other != NULL, CPL_ERROR_NULL_INPUT);
        
    cpl_ensure_code(cpl_imagelist_get_size(self) == sizein,
                    CPL_ERROR_INCOMPATIBLE_INPUT);

    for (i = 0; i < sizein; i++) {
        cpl_image       * imgout = cpl_imagelist_get(self, i);
        const cpl_image * imgin  = cpl_imagelist_get_const(other, i);

        if (cpl_fft_image__(imgout, imgin, mode, &plan,
                            &bufin, &bufout, i+1 == sizein)) break;
    }

    return i == sizein ? CPL_ERROR_NONE : cpl_error_set_where_();
}

/**@}*/
