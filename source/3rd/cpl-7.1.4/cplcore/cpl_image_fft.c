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

/* Must be included first to ensure declaration of complex image accessors */
#include <complex.h>

#include "cpl_image_fft_impl.h"

#include "cpl_error_impl.h"
#include "cpl_memory.h"
#include "cpl_math_const.h"

#if defined CPL_FFTWF_INSTALLED || defined CPL_FFTW_INSTALLED
/* If FFTW is installed */
#include <fftw3.h>
#endif

#include <string.h>
#include <assert.h>

/**@{*/

/*-----------------------------------------------------------------------------
                                   Private functions
 -----------------------------------------------------------------------------*/

#if defined CPL_FFTWF_INSTALLED || defined CPL_FFTW_INSTALLED
static void * cpl_fft_aligned(void *, void *, size_t) CPL_ATTR_NONNULL;
#endif

#ifdef CPL_FFTW_INSTALLED
#  define CPL_FFTW fftw
/* Cannot concatenate the reserved macro complex :-( */
#  define CPL_FFTW_TYPE fftw_complex
#  define CPL_TYPE double
/* Cannot concatenate the reserved macro complex :-( */
#  define CPL_TYPE_C double_complex
#  include "cpl_fft_body.h"
#  undef CPL_FFTW
#  undef CPL_TYPE
#  undef CPL_TYPE_T
#  undef CPL_TYPE_C
#  undef CPL_FFTW_TYPE
#endif

#ifdef CPL_FFTWF_INSTALLED
#  define CPL_FFTW fftwf
/* Cannot concatenate the reserved macro complex :-( */
#  define CPL_FFTW_TYPE fftwf_complex
#  define CPL_TYPE float
/* Cannot concatenate the reserved macro complex :-( */
#  define CPL_TYPE_C float_complex
#  include "cpl_fft_body.h"
#  undef CPL_FFTW
#  undef CPL_TYPE
#  undef CPL_TYPE_T
#  undef CPL_TYPE_C
#  undef CPL_FFTW_TYPE
#endif

/*---------------------------------------------------------------------------*/
/**
  @internal
  @brief    Perform a FFT operation on an image
  @param  self  Pre-allocated output image to transform to
  @param  other Input image to transform from, use self for in-place transform
  @param  mode  CPL_FFT_FORWARD or CPL_FFT_BACKWARD, optionally CPL_FFT_NOSCALE
  @return CPL_ERROR_NONE or the corresponding #_cpl_error_code_ on error
  @see cpl_fft_image()
 */
/*---------------------------------------------------------------------------*/
cpl_error_code cpl_fft_image_(cpl_image * self, const cpl_image * other,
                              cpl_fft_mode mode)
{

    /*
      The below pointers hold temporary storage allocated and deallocated by
      cpl_fft_image__().
    */
    void * bufin  = NULL;
    void * bufout = NULL;

    return cpl_fft_image__(self, other, mode, NULL, &bufin, &bufout, CPL_TRUE)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;

}

/*---------------------------------------------------------------------------*/
/**
  @internal
  @brief Perform a FFT operation on an image
  @param self    Pre-allocated output image to transform to
  @param other   Input image to transform from, use self for in-place transform
  @param mode    CPL_FFT_FORWARD or CPL_FFT_BACKWARD, optionally CPL_FFT_NOSCALE
  @param pplan   NULL, or a pointer to keep the plan
  @param pbufin  A pointer to keep the input buffer
  @param pbufout A pointer to keep the output buffer
  @param is_last CPL_TRUE for the last call with the given pplan
  @return CPL_ERROR_NONE or the corresponding #_cpl_error_code_ on error
  @see cpl_fft_image()
 */
/*---------------------------------------------------------------------------*/
cpl_error_code cpl_fft_image__(cpl_image * self, const cpl_image * other,
                               cpl_fft_mode mode, void * pplan,
                               void * pbufin, void * pbufout,
                               cpl_boolean is_last)
{
    const cpl_type typin  = cpl_image_get_type(other);
    const cpl_type typout = cpl_image_get_type(self);

    const cpl_size lnxin  = cpl_image_get_size_x(other);
    const cpl_size lnyin  = cpl_image_get_size_y(other);
    const cpl_size lnxout = cpl_image_get_size_x(self);
    const cpl_size lnyout = cpl_image_get_size_y(self);

    const int      nxin   = (int)lnxin;
    const int      nyin   = (int)lnyin;
    const int      nxout  = (int)lnxout;
    const int      nyout  = (int)lnyout;
    const int      nxh    = ((mode & CPL_FFT_FORWARD) ? nxin : nxout) / 2 + 1;

    cpl_error_code error;
#if defined CPL_FFTWF_INSTALLED || defined CPL_FFTW_INSTALLED
    unsigned rigor = FFTW_ESTIMATE; /* Default value */


    if (mode & CPL_FFT_FIND_EXHAUSTIVE) {
        rigor = FFTW_EXHAUSTIVE;
        /* Reset bit. At the end the bitmask must be zero */
        mode ^= CPL_FFT_FIND_EXHAUSTIVE;
    } else if (mode & CPL_FFT_FIND_PATIENT) {
        rigor = FFTW_PATIENT;
        /* Reset bit. At the end the bitmask must be zero */
        mode ^= CPL_FFT_FIND_PATIENT;
    } else if (mode & CPL_FFT_FIND_MEASURE) {
        rigor = FFTW_MEASURE;
        /* Reset bit. At the end the bitmask must be zero */
        mode ^= CPL_FFT_FIND_MEASURE;
    }

#endif

    cpl_ensure_code(self  != NULL,             CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(other != NULL,             CPL_ERROR_NULL_INPUT);

    /* FFTW (only) supports dimensions that fit within an int type */
    cpl_ensure_code((cpl_size)nxin  == lnxin,  CPL_ERROR_UNSUPPORTED_MODE);
    cpl_ensure_code((cpl_size)nyin  == lnyin,  CPL_ERROR_UNSUPPORTED_MODE);
    cpl_ensure_code((cpl_size)nxout == lnxout, CPL_ERROR_UNSUPPORTED_MODE);
    cpl_ensure_code((cpl_size)nyout == lnyout, CPL_ERROR_UNSUPPORTED_MODE);

    if ((mode & CPL_FFT_FORWARD) && !(typin & CPL_TYPE_COMPLEX)) {
        cpl_ensure_code(nxin == nxout || nxout == nxh,
                        CPL_ERROR_INCOMPATIBLE_INPUT);
    } else if ((mode & CPL_FFT_BACKWARD) && !(typout & CPL_TYPE_COMPLEX)) {
        cpl_ensure_code(nxin == nxout || nxin == nxh,
                        CPL_ERROR_INCOMPATIBLE_INPUT);
    } else {
        cpl_ensure_code(nxin == nxout,         CPL_ERROR_INCOMPATIBLE_INPUT);
    }
    cpl_ensure_code(lnyin == lnyout,           CPL_ERROR_INCOMPATIBLE_INPUT);

   if (typin & CPL_TYPE_FLOAT) {

       cpl_ensure_code(typout & CPL_TYPE_FLOAT, CPL_ERROR_TYPE_MISMATCH);

#ifdef CPL_FFTWF_INSTALLED
       error = cpl_fft_image_float(self, other, mode, rigor, (fftwf_plan*)pplan,
                                   (fftwf_complex**)pbufin,
                                   (fftwf_complex**)pbufout, is_last);
#else
       error = CPL_ERROR_UNSUPPORTED_MODE;
#endif
   } else if (typin & CPL_TYPE_DOUBLE) {

       cpl_ensure_code(typout & CPL_TYPE_DOUBLE, CPL_ERROR_TYPE_MISMATCH);

#ifdef CPL_FFTW_INSTALLED
       error = cpl_fft_image_double(self, other, mode, rigor, (fftw_plan*)pplan,
                                    (fftw_complex**)pbufin,
                                    (fftw_complex**)pbufout, is_last);
#else
       error = CPL_ERROR_UNSUPPORTED_MODE;
#endif
   } else {
       error = CPL_ERROR_TYPE_MISMATCH;
   }

   /* Set or propagate error, if any */
   return cpl_error_set_message_(error, "mode=%d (%d). %d X %d (%s) => %d X"
                                 " %d (%s)", (int)mode, (int)is_last,
                                 nxin, nyin, cpl_type_get_name(typin),
                                 nxout, nyout, cpl_type_get_name(typout));
}

/*---------------------------------------------------------------------------*/
/**
  @internal
  @brief    Shift an image using FFT
  @param  self   Pre-allocated output image to transform to
  @param  other  Input image to transform from, use NULL for in-place transform
  @param  xshift The number of pixels to cyclic right-shift (pixel 1 is left)
  @param  yshift The number of pixels to cyclic up-shift    (pixel 1 is low)
  @return CPL_ERROR_NONE or the corresponding #_cpl_error_code_ on error
  @see cpl_fft_image()
  @note Only double-type images supported...
 */
/*---------------------------------------------------------------------------*/
cpl_error_code cpl_fft_image_cycle(cpl_image * self,
                                   const cpl_image * other,
                                   double xshift,
                                   double yshift)
{

    const cpl_size nx   = cpl_image_get_size_x(self);
    const cpl_size ny   = cpl_image_get_size_y(self);
    const cpl_type type = cpl_image_get_type(self);

    if (nx < 1) {
        return cpl_error_set_where_();
    } else if (type != CPL_TYPE_DOUBLE) {
        return cpl_error_set_(CPL_ERROR_UNSUPPORTED_MODE);
    } else if (other != NULL && type != cpl_image_get_type(other)) {
        return cpl_error_set_(CPL_ERROR_INCOMPATIBLE_INPUT);
    } else if (other != NULL && nx != cpl_image_get_size_x(other)) {
        return cpl_error_set_(CPL_ERROR_INCOMPATIBLE_INPUT);
    } else if (other != NULL && ny != cpl_image_get_size_y(other)) {
        return cpl_error_set_(CPL_ERROR_INCOMPATIBLE_INPUT);
    } else {
        /* FIXME: Use 1D-FFT to handle special case of zero-valued x/y-shift */

        /* With R2C & C2R: only half the columns */
        const cpl_size nh  = nx / 2 + 1;
        const double   nxy = (double)(nx * ny);
        double complex * pfft =
            (double complex*)cpl_malloc(nh * ny * sizeof(*pfft));
        cpl_image* imgfft  = cpl_image_wrap_double_complex(nh, ny, pfft);
        const cpl_image* imguse = other ? other : self;
        cpl_error_code code;

        code = cpl_fft_image_(imgfft, imguse, CPL_FFT_FORWARD);

        assert(!code);

        for (size_t j = 0; j < (size_t)ny; j++) {
            const double yscale = (j < (size_t)(ny+1)/2 ? -(double)j
                                   : (double)(ny - j)) * yshift / (double)ny;
            for (size_t i = 0; i < (size_t)nh; i++) {
                const double kx = i < (size_t)nh ? -(double)i : (double)(nx - i);
                const double xyshift = yscale + kx * xshift / (double)nx;
                const double complex scale
                    = cexp((CPL_MATH_2PI * xyshift) * I)
                    / nxy; /* Apply also the FFT scaling */

                pfft[i + j * nh] *= scale;
            }
        }

        code = cpl_fft_image_(self, imgfft, CPL_FFT_BACKWARD | CPL_FFT_NOSCALE);
        assert(!code);
        cpl_image_delete(imgfft);
    }

    return CPL_ERROR_NONE;
}


/**@}*/

#if defined CPL_FFTWF_INSTALLED || defined CPL_FFTW_INSTALLED
/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Return aligned (4 bits or matching align mask) pointer
  @param  self  Pointer to return, if aligned
  @param  other Aligned pointer, returned if self is not aligned
  @param  align Pattern
  @return Aligned pointer
  @note It is not checked whether other is in fact aligned

 */
/*----------------------------------------------------------------------------*/
static void * cpl_fft_aligned(void * self, void * other, size_t align)
{
#ifdef HAVE_FFTW_ALIGNMENT_OF
    /* FIXME: Available from FFTW 3.3.4 */
    int iself, iother;
#ifdef _OPENMP
#pragma omp critical(cpl_fft_fftw)
#endif
    {
        iself  = fftw_alignment_of((double*)self);
        iother = fftw_alignment_of((double*)other);
    }

    return iself == iother ? self : other;
#else
    const size_t myself = (const size_t)self;
    size_t lsb = 1;

    /* Find number of zero-valued LSBs */
    while (!(align & lsb) && lsb < 16) {
        lsb <<=1;
        lsb |= 1;
    }
    lsb >>= 1;

    /* Iff self has a 1 where lsb has a 1, then self is not aligned */
    return (myself & lsb) ? other : self;
#endif
}
#endif
