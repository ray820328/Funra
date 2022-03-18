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

#ifndef CPL_VECTOR_H
#define CPL_VECTOR_H

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include <stdio.h>

#include "cpl_type.h"
#include "cpl_error.h"
#include "cpl_propertylist.h"
#include "cpl_matrix.h"
#include "cpl_io.h"

CPL_BEGIN_DECLS

/*-----------------------------------------------------------------------------
                                Defines
 -----------------------------------------------------------------------------*/

#ifndef CPL_KERNEL_DEFAULT
#define CPL_KERNEL_DEFAULT CPL_KERNEL_TANH
#endif

/* Suggested resolution of interpolation profile */
#ifndef CPL_KERNEL_TABSPERPIX
#define CPL_KERNEL_TABSPERPIX   1000
#endif

/* Suggested radius of pixel interpolation */
#ifndef CPL_KERNEL_DEF_WIDTH
#define CPL_KERNEL_DEF_WIDTH    2.0
#endif

/* Suggested length of interpolation profile */
#define CPL_KERNEL_DEF_SAMPLES \
    (1+(cpl_size)((CPL_KERNEL_TABSPERPIX) * (CPL_KERNEL_DEF_WIDTH)))


/* FIXME: Will disappear. Do not use in new code! */
#define cpl_wlcalib_xc_convolve_create_kernel   \
    cpl_vector_new_lss_kernel
#define cpl_wlcalib_xc_convolve                 \
    cpl_vector_convolve_symmetric


/*-----------------------------------------------------------------------------
                                   New types
 -----------------------------------------------------------------------------*/

typedef enum {
    CPL_LOWPASS_LINEAR,
    CPL_LOWPASS_GAUSSIAN
} cpl_lowpass;

typedef enum {
    CPL_KERNEL_TANH,
    CPL_KERNEL_SINC,
    CPL_KERNEL_SINC2,
    CPL_KERNEL_LANCZOS,
    CPL_KERNEL_HAMMING,
    CPL_KERNEL_HANN,
    CPL_KERNEL_NEAREST
} cpl_kernel;

typedef enum {
    CPL_FIT_CENTROID = 1 << 1,
    CPL_FIT_STDEV    = 1 << 2,
    CPL_FIT_AREA     = 1 << 3,
    CPL_FIT_OFFSET   = 1 << 4,
    CPL_FIT_ALL      = (1 << 1) |
                       (1 << 2) |
                       (1 << 3) |
                       (1 << 4)
} cpl_fit_mode;

typedef enum {
    /* Must use these values for backwards compatability of cpl_vector_sort() */
    CPL_SORT_DESCENDING = -1,
    CPL_SORT_ASCENDING = 1
} cpl_sort_direction;

typedef struct _cpl_vector_ cpl_vector;

/*-----------------------------------------------------------------------------
                              Function prototypes
 -----------------------------------------------------------------------------*/

/* Constructors and destructors */
cpl_vector * cpl_vector_new(cpl_size) CPL_ATTR_ALLOC;
cpl_vector * cpl_vector_wrap(cpl_size, double *) CPL_ATTR_ALLOC;
void cpl_vector_delete(cpl_vector *);
void * cpl_vector_unwrap(cpl_vector *);
cpl_vector * cpl_vector_read(const char *) CPL_ATTR_ALLOC;
void cpl_vector_dump(const cpl_vector *, FILE *);
cpl_vector * cpl_vector_load(const char *, cpl_size) CPL_ATTR_ALLOC;
cpl_error_code cpl_vector_save(const cpl_vector *, const char *, cpl_type,
        const cpl_propertylist *, unsigned);
cpl_vector * cpl_vector_duplicate(const cpl_vector *) CPL_ATTR_ALLOC;
cpl_error_code cpl_vector_copy(cpl_vector *, const cpl_vector *);

/* Accessor functions */
cpl_size cpl_vector_get_size(const cpl_vector *);
      double * cpl_vector_get_data(cpl_vector *);
const double * cpl_vector_get_data_const(const cpl_vector *);
double cpl_vector_get(const cpl_vector *, cpl_size);
cpl_error_code cpl_vector_set_size(cpl_vector *, cpl_size);
cpl_error_code cpl_vector_set(cpl_vector *, cpl_size, double);

/* Basic operations */
cpl_error_code cpl_vector_add(cpl_vector *, const cpl_vector *);
cpl_error_code cpl_vector_subtract(cpl_vector *, const cpl_vector *);
cpl_error_code cpl_vector_multiply(cpl_vector *, const cpl_vector *);
cpl_error_code cpl_vector_divide(cpl_vector *, const cpl_vector *);
double cpl_vector_product(const cpl_vector *, const cpl_vector *);
cpl_error_code cpl_vector_cycle(cpl_vector *, const cpl_vector *, double);
cpl_error_code cpl_vector_sort(cpl_vector *, cpl_sort_direction);
cpl_error_code cpl_vector_add_scalar(cpl_vector *, double);
cpl_error_code cpl_vector_subtract_scalar(cpl_vector *, double);
cpl_error_code cpl_vector_multiply_scalar(cpl_vector *, double);
cpl_error_code cpl_vector_divide_scalar(cpl_vector *, double);
cpl_error_code cpl_vector_logarithm(cpl_vector *, double);
cpl_error_code cpl_vector_exponential(cpl_vector *, double);
cpl_error_code cpl_vector_power(cpl_vector *, double);

cpl_error_code cpl_vector_fill(cpl_vector *, double);
cpl_error_code cpl_vector_sqrt(cpl_vector *);
cpl_size cpl_vector_find(const cpl_vector *, double);
cpl_vector * cpl_vector_extract(const cpl_vector *, cpl_size,
                                cpl_size, cpl_size) CPL_ATTR_ALLOC;

/* Statistics on cpl_vector */
cpl_size cpl_vector_get_minpos(const cpl_vector *);
cpl_size cpl_vector_get_maxpos(const cpl_vector *);
double cpl_vector_get_min(const cpl_vector *);
double cpl_vector_get_max(const cpl_vector *);
double cpl_vector_get_sum(const cpl_vector *);
double cpl_vector_get_mean(const cpl_vector *);
double cpl_vector_get_median(cpl_vector *);
double cpl_vector_get_median_const(const cpl_vector *);
double cpl_vector_get_stdev(const cpl_vector *);
cpl_size cpl_vector_correlate(cpl_vector *, const cpl_vector *, const cpl_vector *);

/* Filtering */
cpl_vector * cpl_vector_filter_lowpass_create(const cpl_vector *, cpl_lowpass,
                                              cpl_size) CPL_ATTR_ALLOC;
cpl_vector * cpl_vector_filter_median_create(const cpl_vector *,
                                             cpl_size) CPL_ATTR_ALLOC;

cpl_error_code cpl_vector_fill_kernel_profile(cpl_vector *, cpl_kernel, double);

/* Fitting */
cpl_error_code cpl_vector_fit_gaussian(const cpl_vector *, const cpl_vector *,
                       const cpl_vector *, const cpl_vector *,
                       cpl_fit_mode,
                       double *, double *,
                       double *, double *,
                       double *, double *,
                       cpl_matrix **);

cpl_vector * cpl_vector_new_lss_kernel(double, double) CPL_ATTR_DEPRECATED;
cpl_error_code cpl_vector_convolve_symmetric(cpl_vector *, const cpl_vector *)
    CPL_ATTR_DEPRECATED;

CPL_END_DECLS

#endif
