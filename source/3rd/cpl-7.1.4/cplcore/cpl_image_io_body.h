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

#if defined(CPL_OPERATION) && CPL_OPERATION == CPL_IMAGE_IO_RE_IM

 if (preal != NULL && pimag != NULL) {
     const size_t nxy = (size_t)(self->nx * self->ny);
     size_t       i;

     for (i = 0; i < nxy; i++) {
             preal[i] = CPL_CREAL(pself[i]);
             pimag[i] = CPL_CIMAG(pself[i]);
     }
 } else {
     error = CPL_ERROR_TYPE_MISMATCH;
 }

#elif defined(CPL_OPERATION) && CPL_OPERATION == CPL_IMAGE_IO_RE

 if (preal != NULL) {
     const size_t nxy = (size_t)(self->nx * self->ny);
     size_t       i;

     for (i = 0; i < nxy; i++) {
         preal[i] = CPL_CREAL(pself[i]);
     }
 } else {
     error = CPL_ERROR_TYPE_MISMATCH;
 }

#elif defined(CPL_OPERATION) && CPL_OPERATION == CPL_IMAGE_IO_IM

 if (pimag != NULL) {
     const size_t nxy = (size_t)(self->nx * self->ny);
     size_t       i;

     for (i = 0; i < nxy; i++) {
         pimag[i] = CPL_CIMAG(pself[i]);
     }
 } else {
     error = CPL_ERROR_TYPE_MISMATCH;
 }

#elif defined(CPL_OPERATION) && CPL_OPERATION == CPL_IMAGE_IO_ABS_ARG

 if (pabs != NULL && parg != NULL) {
     const size_t nxy = (size_t)(self->nx * self->ny);
     size_t       i;

     for (i = 0; i < nxy; i++) {
             pabs[i] = CPL_CABS(pself[i]);
             parg[i] = CPL_CARG(pself[i]);
     }
 } else {
     error = CPL_ERROR_TYPE_MISMATCH;
 }

#elif defined(CPL_OPERATION) && CPL_OPERATION == CPL_IMAGE_IO_ABS

 if (pabs != NULL) {
     const size_t nxy = (size_t)(self->nx * self->ny);
     size_t       i;

     for (i = 0; i < nxy; i++) {
         pabs[i] = CPL_CABS(pself[i]);
     }
 } else {
     error = CPL_ERROR_TYPE_MISMATCH;
 }

#elif defined(CPL_OPERATION) && CPL_OPERATION == CPL_IMAGE_IO_ARG

 if (parg != NULL) {
     const size_t nxy = (size_t)(self->nx * self->ny);
     size_t       i;

     for (i = 0; i < nxy; i++) {
         parg[i] = CPL_CARG(pself[i]);
     }
 } else {
     error = CPL_ERROR_TYPE_MISMATCH;
 }

#else

/* Type dependent macros */
#if defined CPL_CLASS && CPL_CLASS == CPL_CLASS_DOUBLE
#define CPL_TYPE            double
/* Cannot concatenate the reserved macro complex :-( */
#define CPL_TYPE_C          double_complex
#define CPL_TYPE_T          CPL_TYPE_DOUBLE
#define CPL_CONJ            conj
#define CPL_CREAL           creal
#define CPL_CIMAG           cimag
#define CPL_CABS            cabs
#define CPL_CARG            carg
#define CPL_CFITSIO_TYPE    TDOUBLE

#elif defined CPL_CLASS && CPL_CLASS == CPL_CLASS_FLOAT
#define CPL_TYPE            float
/* Cannot concatenate the reserved macro complex :-( */
#define CPL_TYPE_C          float_complex
#define CPL_TYPE_T          CPL_TYPE_FLOAT
#define CPL_CONJ            conjf
#define CPL_CREAL           crealf
#define CPL_CIMAG           cimagf
#define CPL_CABS            cabsf
#define CPL_CARG            cargf
#define CPL_CFITSIO_TYPE    TFLOAT

#elif defined CPL_CLASS && CPL_CLASS == CPL_CLASS_INT
#define CPL_TYPE            int
#define CPL_TYPE_T          CPL_TYPE_INT
#define CPL_CFITSIO_TYPE    TINT

#elif defined CPL_CLASS && CPL_CLASS == CPL_CLASS_DOUBLE_COMPLEX
#define CPL_TYPE            double complex
#define CPL_TYPE_T          CPL_TYPE_DOUBLE_COMPLEX
/* NOTE: Saving unsupported */

#elif defined CPL_CLASS && CPL_CLASS == CPL_CLASS_FLOAT_COMPLEX
#define CPL_TYPE            float complex
#define CPL_TYPE_T          CPL_TYPE_FLOAT_COMPLEX
/* NOTE: Saving unsupported */

#else
#undef CPL_TYPE
#undef CPL_TYPE_T
#undef CPL_CFITSIO_TYPE
#endif

#define CPL_TYPE_ADD(a) CPL_CONCAT2X(a, CPL_TYPE)
#define CPL_TYPE_ADD_CONST(a) CPL_CONCAT3X(a, CPL_TYPE, const)
#define CPL_TYPE_ADD_COMPLEX(a) CPL_CONCAT2X(a, CPL_TYPE_C)
#define CPL_TYPE_ADD_COMPLEX_CONST(a) CPL_CONCAT3X(a, CPL_TYPE_C, const)

#if defined(CPL_OPERATION) && CPL_OPERATION == CPL_IMAGE_IO_GET
    case CPL_TYPE_T: {
        const CPL_TYPE * pi = (CPL_TYPE*)image->pixels;
        value = (CPL_TYPE_RETURN)pi[pos];
        break;
    }
#elif defined(CPL_OPERATION) && CPL_OPERATION == CPL_IMAGE_IO_SET
    case CPL_TYPE_T: {
        CPL_TYPE * pi = (CPL_TYPE*)image->pixels;
        pi[pos] = (CPL_TYPE)value;
        break;
    }
#elif defined(CPL_OPERATION) && CPL_OPERATION == CPL_IMAGE_IO_GET_PIXELS

    /* Test entries */
    cpl_ensure(img, CPL_ERROR_NULL_INPUT, NULL);
    if (img->type != CPL_TYPE_T) {
        (void)cpl_error_set_message_(CPL_ERROR_TYPE_MISMATCH,
                                     "Expected pixel type %s, not %s",
                                     cpl_type_get_name(CPL_TYPE_T),
                                     cpl_type_get_name(img->type));
        return NULL;
    }

    return (CPL_CONST CPL_TYPE*)img->pixels;

#elif defined(CPL_OPERATION) && CPL_OPERATION == CPL_IMAGE_IO_SET_BADPIXEL

  case CPL_TYPE_T: {
      CPL_TYPE     * pi = (CPL_TYPE*)im->pixels;
      const CPL_TYPE acast = (const CPL_TYPE) a;
      const size_t   nxy = (size_t)(im->nx * im->ny);
      size_t         i;

      for (i = 0; i < nxy; i++) 
          if (pbpm[i] == CPL_BINARY_1) pi[i] = acast;
      break;
  }

#elif !defined(CPL_OPERATION)

static cpl_error_code CPL_TYPE_ADD(cpl_image_fill_re_im)(cpl_image *,
                                                         cpl_image *,
                                                         const cpl_image *);

static cpl_error_code CPL_TYPE_ADD(cpl_image_fill_re)(cpl_image *,
                                                      const cpl_image *);

static cpl_error_code CPL_TYPE_ADD(cpl_image_fill_im)(cpl_image *,
                                                      const cpl_image *);

static cpl_error_code CPL_TYPE_ADD(cpl_image_fill_abs_arg)(cpl_image *,
                                                         cpl_image *,
                                                         const cpl_image *);

static cpl_error_code CPL_TYPE_ADD(cpl_image_fill_abs)(cpl_image *,
                                                      const cpl_image *);

static cpl_error_code CPL_TYPE_ADD(cpl_image_fill_arg)(cpl_image *,
                                                      const cpl_image *);

static cpl_error_code CPL_TYPE_ADD(cpl_image_conjugate)(cpl_image *,
                                                        const cpl_image *)
    CPL_ATTR_NONNULL;

/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief  Split a complex image into its real and/or imaginary part(s)
  @param  im_real Pre-allocated image to hold the real part, or @c NULL
  @param  im_imag Pre-allocated image to hold the imaginary part, or @c NULL
  @param  self    Complex image to process
  @return CPL_ERROR_NONE or #_cpl_error_code_ on error
  @note   At least one output image must be non-NULL. The images must match
          in size and precision
  @see cpl_image_fill_re_im()
 */
/*----------------------------------------------------------------------------*/
static cpl_error_code CPL_TYPE_ADD(cpl_image_fill_re_im)(cpl_image * im_real,
                                                         cpl_image * im_imag,
                                                         const cpl_image * self)
{

#define CPL_OPERATION CPL_IMAGE_IO_RE_IM

    const CPL_TYPE complex * pself
        = CPL_TYPE_ADD_COMPLEX_CONST(cpl_image_get_data)(self);
    cpl_error_code error = CPL_ERROR_NONE;

    cpl_ensure_code(im_real != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(im_imag != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(pself   != NULL, CPL_ERROR_INVALID_TYPE);

    if ((im_real->type & CPL_TYPE_COMPLEX) &&
        (im_imag->type & CPL_TYPE_COMPLEX)) {
        CPL_TYPE complex * preal
            = CPL_TYPE_ADD_COMPLEX(cpl_image_get_data)(im_real);
        CPL_TYPE complex * pimag
            = CPL_TYPE_ADD_COMPLEX(cpl_image_get_data)(im_imag);

#include "cpl_image_io_body.h"

    } else if (im_real->type & CPL_TYPE_COMPLEX) {
        CPL_TYPE complex * preal
            = CPL_TYPE_ADD_COMPLEX(cpl_image_get_data)(im_real);
        CPL_TYPE * pimag = CPL_TYPE_ADD(cpl_image_get_data)(im_imag);

#include "cpl_image_io_body.h"

    } else if (im_imag->type & CPL_TYPE_COMPLEX) {
        CPL_TYPE * preal = CPL_TYPE_ADD(cpl_image_get_data)(im_real);
        CPL_TYPE complex * pimag
            = CPL_TYPE_ADD_COMPLEX(cpl_image_get_data)(im_imag);

#include "cpl_image_io_body.h"

    } else {
        CPL_TYPE * preal = CPL_TYPE_ADD(cpl_image_get_data)(im_real);
        CPL_TYPE * pimag = CPL_TYPE_ADD(cpl_image_get_data)(im_imag);

#include "cpl_image_io_body.h"

    }

    return error ? cpl_error_set_(error) : CPL_ERROR_NONE;

#undef CPL_OPERATION

}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief  Fill an image with the real part of a complex image
  @param  im_real Pre-allocated image to hold the real part
  @param  self    Complex image to process
  @return CPL_ERROR_NONE or #_cpl_error_code_ on error
  @note   The images must match in size and precision
  @see cpl_image_fill_re_im()
 */
/*----------------------------------------------------------------------------*/
static cpl_error_code CPL_TYPE_ADD(cpl_image_fill_re)(cpl_image * im_real,
                                                      const cpl_image * self)
{

#define CPL_OPERATION CPL_IMAGE_IO_RE

    const CPL_TYPE complex * pself
        = CPL_TYPE_ADD_COMPLEX_CONST(cpl_image_get_data)(self);
    cpl_error_code error = CPL_ERROR_NONE;

    cpl_ensure_code(im_real != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(pself   != NULL, CPL_ERROR_INVALID_TYPE);

    if (im_real->type & CPL_TYPE_COMPLEX) {
        CPL_TYPE complex * preal
            = CPL_TYPE_ADD_COMPLEX(cpl_image_get_data)(im_real);

#include "cpl_image_io_body.h"

    } else {
        CPL_TYPE * preal = CPL_TYPE_ADD(cpl_image_get_data)(im_real);

#include "cpl_image_io_body.h"

    }

    return error ? cpl_error_set_(error) : CPL_ERROR_NONE;

#undef CPL_OPERATION

}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief  Fill an image with the imaginary part of a complex image
  @param  im_imag Pre-allocated image to hold the imaginary part
  @param  self    Complex image to process
  @return CPL_ERROR_NONE or #_cpl_error_code_ on error
  @note   The images must match in size and precision
          @see cpl_image_fill_re_im()
*/
/*----------------------------------------------------------------------------*/
static cpl_error_code CPL_TYPE_ADD(cpl_image_fill_im)(cpl_image * im_imag,
                                                      const cpl_image * self)
{

#define CPL_OPERATION CPL_IMAGE_IO_IM

    const CPL_TYPE complex * pself
        = CPL_TYPE_ADD_COMPLEX_CONST(cpl_image_get_data)(self);
    cpl_error_code error = CPL_ERROR_NONE;

    cpl_ensure_code(im_imag != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(pself   != NULL, CPL_ERROR_INVALID_TYPE);

    if (im_imag->type & CPL_TYPE_COMPLEX) {
        CPL_TYPE complex * pimag
            = CPL_TYPE_ADD_COMPLEX(cpl_image_get_data)(im_imag);

#include "cpl_image_io_body.h"

    } else {
        CPL_TYPE * pimag = CPL_TYPE_ADD(cpl_image_get_data)(im_imag);

#include "cpl_image_io_body.h"

    }

    return error ? cpl_error_set_(error) : CPL_ERROR_NONE;

#undef CPL_OPERATION

}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief  Split a complex image into its absolute and argument part(s)
  @param  im_abs Pre-allocated image to hold the absolute part, or @c NULL
  @param  im_arg Pre-allocated image to hold the argument part, or @c NULL
  @param  self   Complex image to process
  @return CPL_ERROR_NONE or #_cpl_error_code_ on error
  @note   At least one output image must be non-NULL. The images must match
          in size and precision
  @see cpl_image_fill_re_im()
 */
/*----------------------------------------------------------------------------*/
static
cpl_error_code CPL_TYPE_ADD(cpl_image_fill_abs_arg)(cpl_image * im_abs,
                                                    cpl_image * im_arg,
                                                    const cpl_image * self)
{

#define CPL_OPERATION CPL_IMAGE_IO_ABS_ARG

    const CPL_TYPE complex * pself
        = CPL_TYPE_ADD_COMPLEX_CONST(cpl_image_get_data)(self);
    cpl_error_code error = CPL_ERROR_NONE;

    cpl_ensure_code(im_abs != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(im_arg != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(pself   != NULL, CPL_ERROR_INVALID_TYPE);

    if ((im_abs->type & CPL_TYPE_COMPLEX) &&
        (im_arg->type & CPL_TYPE_COMPLEX)) {
        CPL_TYPE complex * pabs
            = CPL_TYPE_ADD_COMPLEX(cpl_image_get_data)(im_abs);
        CPL_TYPE complex * parg
            = CPL_TYPE_ADD_COMPLEX(cpl_image_get_data)(im_arg);

#include "cpl_image_io_body.h"

    } else if (im_abs->type & CPL_TYPE_COMPLEX) {
        CPL_TYPE complex * pabs
            = CPL_TYPE_ADD_COMPLEX(cpl_image_get_data)(im_abs);
        CPL_TYPE * parg = CPL_TYPE_ADD(cpl_image_get_data)(im_arg);

#include "cpl_image_io_body.h"

    } else if (im_arg->type & CPL_TYPE_COMPLEX) {
        CPL_TYPE * pabs = CPL_TYPE_ADD(cpl_image_get_data)(im_abs);
        CPL_TYPE complex * parg
            = CPL_TYPE_ADD_COMPLEX(cpl_image_get_data)(im_arg);

#include "cpl_image_io_body.h"

    } else {
        CPL_TYPE * pabs = CPL_TYPE_ADD(cpl_image_get_data)(im_abs);
        CPL_TYPE * parg = CPL_TYPE_ADD(cpl_image_get_data)(im_arg);

#include "cpl_image_io_body.h"

    }

    return error ? cpl_error_set_(error) : CPL_ERROR_NONE;

#undef CPL_OPERATION

}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief  Fill an image with the absolute part of a complex image
  @param  im_abs Pre-allocated image to hold the absolute part
  @param  self   Complex image to process
  @return CPL_ERROR_NONE or #_cpl_error_code_ on error
  @note   The images must match in size and precision
  @see cpl_image_fill_abs_arg()
 */
/*----------------------------------------------------------------------------*/
static cpl_error_code CPL_TYPE_ADD(cpl_image_fill_abs)(cpl_image * im_abs,
                                                       const cpl_image * self)
{

#define CPL_OPERATION CPL_IMAGE_IO_ABS

    const CPL_TYPE complex * pself
        = CPL_TYPE_ADD_COMPLEX_CONST(cpl_image_get_data)(self);
    cpl_error_code error = CPL_ERROR_NONE;

    cpl_ensure_code(im_abs != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(pself   != NULL, CPL_ERROR_INVALID_TYPE);

    if (im_abs->type & CPL_TYPE_COMPLEX) {
        CPL_TYPE complex * pabs
            = CPL_TYPE_ADD_COMPLEX(cpl_image_get_data)(im_abs);

#include "cpl_image_io_body.h"

    } else {
        CPL_TYPE * pabs = CPL_TYPE_ADD(cpl_image_get_data)(im_abs);

#include "cpl_image_io_body.h"

    }

    return error ? cpl_error_set_(error) : CPL_ERROR_NONE;

#undef CPL_OPERATION

}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief  Fill an image with the argument part of a complex image
  @param  im_arg Pre-allocated image to hold the argument part, or @c NULL
  @param  self   Complex image to process
  @return CPL_ERROR_NONE or #_cpl_error_code_ on error
  @note   At least one output image must be non-NULL. The images must match
          in size and precision
  @see cpl_image_fill_re_im()
 */
/*----------------------------------------------------------------------------*/
static cpl_error_code CPL_TYPE_ADD(cpl_image_fill_arg)(cpl_image * im_arg,
                                                       const cpl_image * self)
{

#define CPL_OPERATION CPL_IMAGE_IO_ARG

    const CPL_TYPE complex * pself
        = CPL_TYPE_ADD_COMPLEX_CONST(cpl_image_get_data)(self);
    cpl_error_code error = CPL_ERROR_NONE;

    cpl_ensure_code(im_arg != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(pself  != NULL, CPL_ERROR_INVALID_TYPE);

    if (im_arg->type & CPL_TYPE_COMPLEX) {
        CPL_TYPE complex * parg
            = CPL_TYPE_ADD_COMPLEX(cpl_image_get_data)(im_arg);

#include "cpl_image_io_body.h"

    } else {
        CPL_TYPE * parg = CPL_TYPE_ADD(cpl_image_get_data)(im_arg);

#include "cpl_image_io_body.h"

    }

    return error ? cpl_error_set_(error) : CPL_ERROR_NONE;

#undef CPL_OPERATION

}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @brief  Complex conjugate the pixels in a complex image of a specific type
  @param  self  Pre-allocated complex image to hold the result
  @param  other The complex image to conjugate, may equal self
  @return CPL_ERROR_NONE or #_cpl_error_code_ on error
  @note   The two images must match in size and precision
  @see cpl_image_conjugate()
 */
/*----------------------------------------------------------------------------*/
static cpl_error_code CPL_TYPE_ADD(cpl_image_conjugate)(cpl_image * self,
                                                        const cpl_image * other)
{

    CPL_TYPE complex       * pout = CPL_TYPE_ADD_COMPLEX
        (cpl_image_get_data)(self);
    const CPL_TYPE complex * pin = CPL_TYPE_ADD_COMPLEX_CONST
        (cpl_image_get_data)(other);
    const size_t             nxy = (size_t)(self->nx * self->ny);
    size_t                   i;

    cpl_ensure_code(pout != NULL, CPL_ERROR_INVALID_TYPE);
    cpl_ensure_code(pin  != NULL, CPL_ERROR_INVALID_TYPE);

    for (i = 0; i < nxy; i++) {
        pout[i] = CPL_CONJ(pin[i]);
    }

    return CPL_ERROR_NONE;
}

#endif

#undef CPL_TYPE
#undef CPL_TYPE_C
#undef CPL_TYPE_T
#undef CPL_CFITSIO_TYPE
#undef CPL_CONJ
#undef CPL_CREAL
#undef CPL_CIMAG
#undef CPL_CABS
#undef CPL_CARG
#undef CPL_TYPE_ADD
#undef CPL_TYPE_ADD_CONST
#undef CPL_TYPE_ADD_COMPLEX
#undef CPL_TYPE_ADD_COMPLEX_CONST

#endif
