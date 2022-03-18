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

#ifndef CPL_IMAGE_RESAMPLE_H
#define CPL_IMAGE_RESAMPLE_H

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <cpl_image.h>
#include <cpl_matrix.h>
#include <cpl_polynomial.h>
#include <cpl_vector.h>

/*-----------------------------------------------------------------------------
                            Function prototypes
 -----------------------------------------------------------------------------*/

CPL_BEGIN_DECLS

cpl_error_code cpl_image_warp_polynomial(cpl_image *, const cpl_image *,
                                         const cpl_polynomial *,
                                         const cpl_polynomial *,
                                         const cpl_vector *, double,
                                         const cpl_vector *, double);

cpl_error_code cpl_image_warp(cpl_image *, const cpl_image *,
                              const cpl_image *,
                              const cpl_image *,
                              const cpl_vector *, double,
                              const cpl_vector *, double);

cpl_error_code cpl_image_fill_jacobian_polynomial(cpl_image *,
                                                  const cpl_polynomial *poly_x,
                                                  const cpl_polynomial *poly_y);

cpl_error_code cpl_image_fill_jacobian(cpl_image *,
                                       const cpl_image *deltax,
                                       const cpl_image *deltay);

cpl_image *cpl_image_extract_subsample(const cpl_image *,
                                       cpl_size, cpl_size) CPL_ATTR_ALLOC;

cpl_image *cpl_image_rebin(const cpl_image *, cpl_size, cpl_size,
                           cpl_size, cpl_size) CPL_ATTR_ALLOC;

double cpl_image_get_interpolated(const cpl_image *, double, double,
                                  const cpl_vector *, double,
                                  const cpl_vector *, double, double *);

CPL_END_DECLS

#endif 

