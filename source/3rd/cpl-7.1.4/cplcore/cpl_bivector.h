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

#ifndef CPL_BIVECTOR_H
#define CPL_BIVECTOR_H

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include <cpl_macros.h>
#include "cpl_vector.h"

#include <stdio.h>

CPL_BEGIN_DECLS

/*-----------------------------------------------------------------------------
                                   New types
 -----------------------------------------------------------------------------*/

typedef struct _cpl_bivector_ cpl_bivector;

typedef enum {
    CPL_SORT_BY_X,
    CPL_SORT_BY_Y
} cpl_sort_mode;

/*-----------------------------------------------------------------------------
                              Function prototypes
 -----------------------------------------------------------------------------*/

/* Constructors and destructors */
cpl_bivector * cpl_bivector_new(cpl_size) CPL_ATTR_ALLOC;
cpl_bivector * cpl_bivector_wrap_vectors(cpl_vector *,
                                         cpl_vector *) CPL_ATTR_ALLOC;
cpl_bivector * cpl_bivector_duplicate(const cpl_bivector *) CPL_ATTR_ALLOC;
void cpl_bivector_delete(cpl_bivector *);
void cpl_bivector_unwrap_vectors(cpl_bivector *);
void cpl_bivector_dump(const cpl_bivector *, FILE *);
cpl_bivector * cpl_bivector_read(const char *) CPL_ATTR_ALLOC;
cpl_error_code cpl_bivector_copy(cpl_bivector *, const cpl_bivector *);

/* Accessors functions */
cpl_size cpl_bivector_get_size(const cpl_bivector *);
      cpl_vector * cpl_bivector_get_x(cpl_bivector *);
      cpl_vector * cpl_bivector_get_y(cpl_bivector *);
const cpl_vector * cpl_bivector_get_x_const(const cpl_bivector *);
const cpl_vector * cpl_bivector_get_y_const(const cpl_bivector *);
      double * cpl_bivector_get_x_data(cpl_bivector *);
      double * cpl_bivector_get_y_data(cpl_bivector *);
const double * cpl_bivector_get_x_data_const(const cpl_bivector *);
const double * cpl_bivector_get_y_data_const(const cpl_bivector *);

/* Basic functionalities */
cpl_error_code cpl_bivector_interpolate_linear(cpl_bivector *,
                                               const cpl_bivector *);

cpl_error_code cpl_bivector_sort(cpl_bivector *, const cpl_bivector *,
                                 cpl_sort_direction, cpl_sort_mode);

CPL_END_DECLS

#endif

