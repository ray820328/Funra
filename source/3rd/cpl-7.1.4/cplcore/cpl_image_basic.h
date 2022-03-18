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

#ifndef CPL_IMAGE_BASIC_H
#define CPL_IMAGE_BASIC_H

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include "cpl_image.h"
#include "cpl_vector.h"

CPL_BEGIN_DECLS

/*-----------------------------------------------------------------------------
                                   Defines
 -----------------------------------------------------------------------------*/
/**
 * Each FFT mode is listed below.
 * The modes can be combined with bitwise or.
 */
#define CPL_FFT_DEFAULT      ((unsigned)0)
/* Inverse FFT */
#define CPL_FFT_INVERSE      ((unsigned)1 << 1)
/* No normalization */
#define CPL_FFT_UNNORMALIZED ((unsigned)1 << 2)
/* Swap halves of output */
#define CPL_FFT_SWAP_HALVES  ((unsigned)1 << 3)

enum _cpl_norm_ {
    CPL_NORM_SCALE,
    CPL_NORM_MEAN,
    CPL_NORM_FLUX,
    CPL_NORM_ABSFLUX
};

typedef enum _cpl_norm_ cpl_norm;

/*-----------------------------------------------------------------------------
                            Function prototypes
 -----------------------------------------------------------------------------*/

/* Basic operations */
cpl_image * cpl_image_add_create(const cpl_image *, const cpl_image *)
    CPL_ATTR_ALLOC;
cpl_image * cpl_image_subtract_create(const cpl_image *, const cpl_image *)
    CPL_ATTR_ALLOC;
cpl_image * cpl_image_multiply_create(const cpl_image *, const cpl_image *)
    CPL_ATTR_ALLOC;
cpl_image * cpl_image_divide_create(const cpl_image *, const cpl_image *)
    CPL_ATTR_ALLOC;
cpl_error_code cpl_image_add(cpl_image *, const cpl_image *);
cpl_error_code cpl_image_subtract(cpl_image *, const cpl_image *);
cpl_error_code cpl_image_multiply(cpl_image *, const cpl_image *);
cpl_error_code cpl_image_divide(cpl_image *, const cpl_image *);

cpl_error_code cpl_image_add_scalar(cpl_image *, double);
cpl_error_code cpl_image_subtract_scalar(cpl_image *, double);
cpl_error_code cpl_image_multiply_scalar(cpl_image *, double);
cpl_error_code cpl_image_divide_scalar(cpl_image *, double);
cpl_error_code cpl_image_power(cpl_image *, double);
cpl_error_code cpl_image_exponential(cpl_image *, double);
cpl_error_code cpl_image_logarithm(cpl_image *, double);
cpl_error_code cpl_image_normalise(cpl_image *, cpl_norm);
cpl_error_code cpl_image_abs(cpl_image *);
cpl_error_code cpl_image_hypot(cpl_image *,
                               const cpl_image *,
                               const cpl_image *);

cpl_error_code cpl_image_and(cpl_image *,
                             const cpl_image *,
                             const cpl_image *);
cpl_error_code cpl_image_or(cpl_image *,
                            const cpl_image *,
                            const cpl_image *);
cpl_error_code cpl_image_xor(cpl_image *,
                             const cpl_image *,
                             const cpl_image *);

cpl_error_code cpl_image_not(cpl_image *,
                             const cpl_image *);

cpl_error_code cpl_image_and_scalar(cpl_image *,
                                    const cpl_image *,
                                    cpl_bitmask);
cpl_error_code cpl_image_or_scalar(cpl_image *,
                                   const cpl_image *,
                                   cpl_bitmask);
cpl_error_code cpl_image_xor_scalar(cpl_image *,
                                    const cpl_image *,
                                    cpl_bitmask);

cpl_image * cpl_image_add_scalar_create(const cpl_image *, double)
    CPL_ATTR_ALLOC;
cpl_image * cpl_image_subtract_scalar_create(const cpl_image *, double)
    CPL_ATTR_ALLOC;
cpl_image * cpl_image_multiply_scalar_create(const cpl_image *, double)
    CPL_ATTR_ALLOC;
cpl_image * cpl_image_divide_scalar_create(const cpl_image *, double)
    CPL_ATTR_ALLOC;
cpl_image * cpl_image_power_create(const cpl_image *, double)
    CPL_ATTR_ALLOC;
cpl_image * cpl_image_exponential_create(const cpl_image *, double)
    CPL_ATTR_ALLOC;
cpl_image * cpl_image_logarithm_create(const cpl_image *, double)
    CPL_ATTR_ALLOC;
cpl_image * cpl_image_normalise_create(const cpl_image *, cpl_norm)
    CPL_ATTR_ALLOC;
cpl_image * cpl_image_abs_create(const cpl_image *)
    CPL_ATTR_ALLOC;

cpl_error_code cpl_image_threshold(cpl_image *, double, double, double, double);
cpl_image * cpl_image_average_create(const cpl_image *, const cpl_image *)
    CPL_ATTR_ALLOC;

/* Collapse functions */
cpl_image * cpl_image_collapse_window_create(const cpl_image *, cpl_size,
                                             cpl_size, cpl_size,
                                             cpl_size, int)
    CPL_ATTR_ALLOC;
cpl_image * cpl_image_collapse_create(const cpl_image *, int)
    CPL_ATTR_ALLOC;
cpl_image * cpl_image_collapse_median_create(const cpl_image *, int,
                                             cpl_size, cpl_size)
    CPL_ATTR_ALLOC;

/* Extraction function */
cpl_image * cpl_image_extract(const cpl_image *, cpl_size, cpl_size,
                              cpl_size, cpl_size) CPL_ATTR_ALLOC;
cpl_vector * cpl_vector_new_from_image_row(const cpl_image *, cpl_size)
    CPL_ATTR_ALLOC;
cpl_vector * cpl_vector_new_from_image_column(const cpl_image *, cpl_size)
    CPL_ATTR_ALLOC;

/* Rotation and Shift */
cpl_error_code cpl_image_turn(cpl_image *, int);
cpl_error_code cpl_image_shift(cpl_image *, cpl_size, cpl_size);

/* Insert an image in an other one */
cpl_error_code cpl_image_copy(cpl_image *, const cpl_image *,
                              cpl_size, cpl_size);

/* Symmetry function */
cpl_error_code cpl_image_flip(cpl_image *, int);

/* Pixels re-organization */
cpl_error_code cpl_image_move(cpl_image *, cpl_size, const cpl_size *);

/* Gaussian fit of an image zone */
cpl_error_code
cpl_image_fit_gaussian(const cpl_image *, cpl_size, cpl_size, cpl_size,
                       double *, double *, double *, double *,
                       double *, double *, double *) CPL_ATTR_DEPRECATED;

/* FWHM computation on a local maximum */
cpl_error_code cpl_image_get_fwhm(const cpl_image *, cpl_size, cpl_size,
                                  double *, double *);

/* FFT computation */
cpl_error_code cpl_image_fft(cpl_image *, cpl_image *, unsigned);

CPL_END_DECLS

#endif 
