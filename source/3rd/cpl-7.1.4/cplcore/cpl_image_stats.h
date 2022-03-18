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

#ifndef CPL_IMAGE_STATS_H
#define CPL_IMAGE_STATS_H

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include "cpl_image.h"

CPL_BEGIN_DECLS

/*-----------------------------------------------------------------------------
                            Function prototypes
 -----------------------------------------------------------------------------*/

double cpl_image_get_min(const cpl_image *);
double cpl_image_get_min_window(const cpl_image *, cpl_size, cpl_size,
                                cpl_size, cpl_size);

double cpl_image_get_max(const cpl_image *);
double cpl_image_get_max_window(const cpl_image *, cpl_size, cpl_size,
                                cpl_size, cpl_size);

double cpl_image_get_mean(const cpl_image *);
double cpl_image_get_mean_window(const cpl_image *, cpl_size, cpl_size,
                                 cpl_size, cpl_size);

double cpl_image_get_median(const cpl_image *);
double cpl_image_get_median_window(const cpl_image *, cpl_size, cpl_size,
                                   cpl_size, cpl_size);

double cpl_image_get_stdev(const cpl_image *);
double cpl_image_get_stdev_window(const cpl_image *, cpl_size, cpl_size,
                                  cpl_size, cpl_size);

double cpl_image_get_flux(const cpl_image *);
double cpl_image_get_flux_window(const cpl_image *, cpl_size, cpl_size,
                                 cpl_size, cpl_size);

double cpl_image_get_absflux(const cpl_image *);
double cpl_image_get_absflux_window(const cpl_image *, cpl_size, cpl_size,
                                    cpl_size, cpl_size);

double cpl_image_get_sqflux(const cpl_image *);
double cpl_image_get_sqflux_window(const cpl_image *, cpl_size, cpl_size,
                                   cpl_size, cpl_size);

double cpl_image_get_centroid_x(const cpl_image *);
double cpl_image_get_centroid_x_window(const cpl_image *, cpl_size, cpl_size,
                                       cpl_size, cpl_size);

double cpl_image_get_centroid_y(const cpl_image *);
double cpl_image_get_centroid_y_window(const cpl_image *, cpl_size, cpl_size,
                                       cpl_size, cpl_size);

cpl_error_code cpl_image_get_minpos(const cpl_image *, cpl_size *, cpl_size *);
cpl_error_code cpl_image_get_minpos_window(const cpl_image *, cpl_size,
                                           cpl_size, cpl_size, cpl_size,
                                           cpl_size *, cpl_size *);
cpl_error_code cpl_image_get_maxpos(const cpl_image *, cpl_size *, cpl_size *);
cpl_error_code cpl_image_get_maxpos_window(const cpl_image *, cpl_size,
                                           cpl_size, cpl_size, cpl_size,
                                           cpl_size *, cpl_size *);

double cpl_image_get_median_dev(const cpl_image *, double *);
double cpl_image_get_median_dev_window(const cpl_image *, cpl_size, cpl_size,
                                       cpl_size, cpl_size, double *);

double cpl_image_get_mad(const cpl_image *, double *);
double cpl_image_get_mad_window(const cpl_image *, cpl_size, cpl_size,
                                cpl_size, cpl_size, double *);

CPL_END_DECLS

#endif 
