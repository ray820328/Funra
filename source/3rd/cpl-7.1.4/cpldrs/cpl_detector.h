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

#ifndef CPL_DETECTOR_H
#define CPL_DETECTOR_H

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include <cpl_error.h>
#include <cpl_image.h>
#include <cpl_imagelist.h>
#include <cpl_macros.h>

CPL_BEGIN_DECLS

/*-----------------------------------------------------------------------------
                            Function prototypes
 -----------------------------------------------------------------------------*/

/* Noise computation */
cpl_error_code cpl_flux_get_noise_window(const cpl_image *, const cpl_size *,
                                         cpl_size, cpl_size,
                                         double *, double *);
cpl_error_code cpl_flux_get_bias_window(const cpl_image *, const cpl_size *,
                                        cpl_size, cpl_size, double *, double *);
cpl_error_code cpl_flux_get_noise_ring(const cpl_image *, const double *,
                                       cpl_size, cpl_size, double *, double *);

/* Bad pixels cleaning */
cpl_error_code cpl_detector_interpolate_rejected(cpl_image *);

CPL_END_DECLS

#endif 
