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

#ifndef CPL_MASK_H
#define CPL_MASK_H

/*-----------------------------------------------------------------------------
                                   New types
 -----------------------------------------------------------------------------*/

typedef struct _cpl_mask_ cpl_mask;

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include "cpl_image.h"
#include "cpl_matrix.h"
#include "cpl_filter.h"
#include "cpl_propertylist.h"

CPL_BEGIN_DECLS

/*-----------------------------------------------------------------------------
                    cpl_binary type definition
 -----------------------------------------------------------------------------*/

typedef unsigned char   cpl_binary;

/*-----------------------------------------------------------------------------
                                   Define
 -----------------------------------------------------------------------------*/

#define CPL_BINARY_1        (cpl_binary)1
#define CPL_BINARY_0        (cpl_binary)0

/*-----------------------------------------------------------------------------
                            Function prototypes
 -----------------------------------------------------------------------------*/

/* IO operations */
cpl_mask * cpl_mask_new(cpl_size, cpl_size) CPL_ATTR_ALLOC;
cpl_mask * cpl_mask_wrap(cpl_size, cpl_size, cpl_binary *) CPL_ATTR_ALLOC;
cpl_mask * cpl_mask_duplicate(const cpl_mask *) CPL_ATTR_ALLOC;
void cpl_mask_delete(cpl_mask *);
void * cpl_mask_unwrap(cpl_mask *);
cpl_error_code cpl_mask_dump_window(const cpl_mask *, cpl_size, cpl_size,
                                    cpl_size, cpl_size, FILE *);

cpl_mask * cpl_mask_load(const char *, cpl_size, cpl_size) CPL_ATTR_ALLOC;
cpl_mask * cpl_mask_load_window(const char *, cpl_size, cpl_size, cpl_size,
                                cpl_size, cpl_size, cpl_size) CPL_ATTR_ALLOC;
cpl_error_code cpl_mask_save(const cpl_mask *, const char *,
                             const cpl_propertylist *, unsigned);

/* Accessor functions */
cpl_binary * cpl_mask_get_data(cpl_mask *);
const cpl_binary * cpl_mask_get_data_const(const cpl_mask *);
cpl_binary cpl_mask_get(const cpl_mask *, cpl_size, cpl_size);
cpl_error_code cpl_mask_set(cpl_mask *, cpl_size, cpl_size, cpl_binary);
cpl_size cpl_mask_get_size_x(const cpl_mask *);
cpl_size cpl_mask_get_size_y(const cpl_mask *);

/* Basic operations */
cpl_boolean cpl_mask_is_empty(const cpl_mask *);
cpl_boolean cpl_mask_is_empty_window(const cpl_mask *, cpl_size, cpl_size,
                                     cpl_size, cpl_size);
cpl_size cpl_mask_count(const cpl_mask *);
cpl_size cpl_mask_count_window(const cpl_mask *, cpl_size, cpl_size,
                               cpl_size, cpl_size);
cpl_error_code cpl_mask_and(cpl_mask *, const cpl_mask *);
cpl_error_code cpl_mask_or(cpl_mask *, const cpl_mask *);
cpl_error_code cpl_mask_xor(cpl_mask *, const cpl_mask *);
cpl_error_code cpl_mask_not(cpl_mask *);
cpl_mask * cpl_mask_collapse_create(const cpl_mask *, int) CPL_ATTR_ALLOC;
cpl_mask * cpl_mask_extract(const cpl_mask *, cpl_size, cpl_size,
                            cpl_size, cpl_size) CPL_ATTR_ALLOC;
cpl_error_code cpl_mask_turn(cpl_mask *, int);
cpl_error_code cpl_mask_shift(cpl_mask *, cpl_size, cpl_size);
cpl_error_code cpl_mask_copy(cpl_mask *, const cpl_mask *, cpl_size, cpl_size);
cpl_error_code cpl_mask_flip(cpl_mask *, int);
cpl_error_code cpl_mask_move(cpl_mask *, cpl_size, const cpl_size *);
cpl_mask * cpl_mask_extract_subsample(const cpl_mask *,
                                      cpl_size, cpl_size) CPL_ATTR_ALLOC;

/* Morphological operations */
cpl_error_code cpl_mask_filter(cpl_mask *, const cpl_mask *, const cpl_mask *,
                               cpl_filter_mode, cpl_border_mode);
cpl_error_code cpl_mask_closing(cpl_mask *,
                                const cpl_matrix *) CPL_ATTR_DEPRECATED;
cpl_error_code cpl_mask_opening(cpl_mask *,
                                const cpl_matrix *) CPL_ATTR_DEPRECATED;
cpl_error_code cpl_mask_erosion(cpl_mask *,
                                const cpl_matrix *) CPL_ATTR_DEPRECATED;
cpl_error_code cpl_mask_dilation(cpl_mask *,
                                 const cpl_matrix *) CPL_ATTR_DEPRECATED;

/* Zones selection */
cpl_error_code cpl_mask_threshold_image(cpl_mask *, const cpl_image *,
                                        double, double, cpl_binary);
cpl_mask * cpl_mask_threshold_image_create(const cpl_image *,
                                           double, double) CPL_ATTR_ALLOC;

CPL_END_DECLS

#endif 
