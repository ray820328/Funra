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

#ifndef CPL_APERTURES_H
#define CPL_APERTURES_H

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include <stdio.h>
#include <cpl_vector.h>
#include <cpl_image.h>

CPL_BEGIN_DECLS

/*-----------------------------------------------------------------------------
                                   New types
 -----------------------------------------------------------------------------*/

typedef struct _cpl_apertures_ cpl_apertures;

/*-----------------------------------------------------------------------------
                            Function prototypes
 -----------------------------------------------------------------------------*/

/* IO functions */
void cpl_apertures_delete(cpl_apertures *);
void cpl_apertures_dump(const cpl_apertures *, FILE *);

/* Statistics computation */
cpl_apertures * cpl_apertures_new_from_image(const cpl_image *,
                                             const cpl_image *) CPL_ATTR_ALLOC;

/* Accessor functions */
cpl_size cpl_apertures_get_size(const cpl_apertures *);
double cpl_apertures_get_pos_x(const cpl_apertures *, cpl_size);
double cpl_apertures_get_pos_y(const cpl_apertures *, cpl_size);
double cpl_apertures_get_max_x(const cpl_apertures *, cpl_size)
    CPL_ATTR_DEPRECATED;
double cpl_apertures_get_max_y(const cpl_apertures *, cpl_size)
    CPL_ATTR_DEPRECATED;
double cpl_apertures_get_centroid_x(const cpl_apertures *, cpl_size);
double cpl_apertures_get_centroid_y(const cpl_apertures *, cpl_size);
cpl_size cpl_apertures_get_maxpos_x(const cpl_apertures *, cpl_size);
cpl_size cpl_apertures_get_maxpos_y(const cpl_apertures *, cpl_size);
cpl_size cpl_apertures_get_minpos_x(const cpl_apertures *, cpl_size);
cpl_size cpl_apertures_get_minpos_y(const cpl_apertures *, cpl_size);
cpl_size cpl_apertures_get_npix(const cpl_apertures *, cpl_size);
cpl_size cpl_apertures_get_left(const cpl_apertures *, cpl_size);
cpl_size cpl_apertures_get_left_y(const cpl_apertures *, cpl_size);
cpl_size cpl_apertures_get_right(const cpl_apertures *, cpl_size);
cpl_size cpl_apertures_get_right_y(const cpl_apertures *, cpl_size);
cpl_size cpl_apertures_get_top_x(const cpl_apertures *, cpl_size);
cpl_size cpl_apertures_get_top(const cpl_apertures *, cpl_size);
cpl_size cpl_apertures_get_bottom_x(const cpl_apertures *, cpl_size);
cpl_size cpl_apertures_get_bottom(const cpl_apertures *, cpl_size);
double cpl_apertures_get_max(const cpl_apertures *, cpl_size);
double cpl_apertures_get_min(const cpl_apertures *, cpl_size);
double cpl_apertures_get_mean(const cpl_apertures *, cpl_size);
double cpl_apertures_get_median(const cpl_apertures *, cpl_size);
double cpl_apertures_get_stdev(const cpl_apertures *, cpl_size);
double cpl_apertures_get_flux(const cpl_apertures *, cpl_size);

/* Sorting functions */
cpl_error_code cpl_apertures_sort_by_npix(cpl_apertures *);
cpl_error_code cpl_apertures_sort_by_max(cpl_apertures  *);
cpl_error_code cpl_apertures_sort_by_flux(cpl_apertures *);

/* Detection functions */
cpl_apertures * cpl_apertures_extract(const cpl_image *, const cpl_vector *,
                                      cpl_size *) CPL_ATTR_ALLOC;
cpl_apertures * cpl_apertures_extract_window(const cpl_image *,
                                             const cpl_vector *, cpl_size,
                                             cpl_size, cpl_size,
                                             cpl_size, cpl_size *)
    CPL_ATTR_ALLOC;

cpl_apertures * cpl_apertures_extract_mask(const cpl_image *,
                                           const cpl_mask *) CPL_ATTR_ALLOC;
cpl_apertures * cpl_apertures_extract_sigma(const cpl_image *,
                                            double) CPL_ATTR_ALLOC;

cpl_bivector* cpl_apertures_get_fwhm(const cpl_image*,
                                     const cpl_apertures*) CPL_ATTR_DEPRECATED;

CPL_END_DECLS

#endif 
