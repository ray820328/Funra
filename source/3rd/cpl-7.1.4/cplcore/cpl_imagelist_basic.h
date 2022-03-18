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

#ifndef CPL_IMAGELIST_BASIC_H
#define CPL_IMAGELIST_BASIC_H

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include "cpl_imagelist.h"

CPL_BEGIN_DECLS

/*-----------------------------------------------------------------------------
                                   Defines
 -----------------------------------------------------------------------------*/

enum _cpl_swap_axis_ {
    CPL_SWAP_AXIS_XZ,
    CPL_SWAP_AXIS_YZ
};

typedef enum _cpl_swap_axis_ cpl_swap_axis;

typedef enum {
    CPL_COLLAPSE_MEAN,
    CPL_COLLAPSE_MEDIAN,
    CPL_COLLAPSE_MEDIAN_MEAN
} cpl_collapse_mode;

/*-----------------------------------------------------------------------------
                            Function prototypes
 -----------------------------------------------------------------------------*/

/* Imagelist modifying functions */
cpl_error_code cpl_imagelist_add(cpl_imagelist *, const cpl_imagelist *);
cpl_error_code cpl_imagelist_subtract(cpl_imagelist *, const cpl_imagelist *);
cpl_error_code cpl_imagelist_multiply(cpl_imagelist *, const cpl_imagelist *);
cpl_error_code cpl_imagelist_divide(cpl_imagelist *, const cpl_imagelist *);

cpl_error_code cpl_imagelist_add_image(cpl_imagelist *, const cpl_image *);
cpl_error_code cpl_imagelist_subtract_image(cpl_imagelist *, const cpl_image *);
cpl_error_code cpl_imagelist_multiply_image(cpl_imagelist *, const cpl_image *);
cpl_error_code cpl_imagelist_divide_image(cpl_imagelist *, const cpl_image *);

cpl_error_code cpl_imagelist_add_scalar(cpl_imagelist *, double);
cpl_error_code cpl_imagelist_subtract_scalar(cpl_imagelist *, double);
cpl_error_code cpl_imagelist_multiply_scalar(cpl_imagelist *, double);
cpl_error_code cpl_imagelist_divide_scalar(cpl_imagelist *, double);
cpl_error_code cpl_imagelist_exponential(cpl_imagelist *, double);
cpl_error_code cpl_imagelist_power(cpl_imagelist *, double);
cpl_error_code cpl_imagelist_logarithm(cpl_imagelist *, double);

cpl_error_code cpl_imagelist_normalise(cpl_imagelist *, cpl_norm);
cpl_error_code cpl_imagelist_threshold(cpl_imagelist *, double, double,
                                       double, double);
cpl_imagelist * cpl_imagelist_swap_axis_create(const cpl_imagelist *,
                                               cpl_swap_axis) CPL_ATTR_ALLOC;

/* Imagelist to image functions */
cpl_image * cpl_image_new_from_accepted(const cpl_imagelist *) CPL_ATTR_ALLOC;
cpl_image * cpl_imagelist_collapse_create(const cpl_imagelist *) CPL_ATTR_ALLOC;
cpl_image * cpl_imagelist_collapse_median_create(const cpl_imagelist *)
    CPL_ATTR_ALLOC;
cpl_image * cpl_imagelist_collapse_minmax_create(const cpl_imagelist *,
                                                 cpl_size, cpl_size)
    CPL_ATTR_ALLOC;
cpl_image * cpl_imagelist_collapse_sigclip_create(const cpl_imagelist *,
                                                  double, double, double,
                                                  cpl_collapse_mode,
                                                  cpl_image *) CPL_ATTR_ALLOC;

CPL_END_DECLS

#endif 
