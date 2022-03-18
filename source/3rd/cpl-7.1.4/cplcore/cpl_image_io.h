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

#ifndef CPL_IMAGE_IO_H
#define CPL_IMAGE_IO_H

/*-----------------------------------------------------------------------------
                                   New types
 -----------------------------------------------------------------------------*/

typedef struct _cpl_image_ cpl_image;

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include "cpl_io.h"
#include "cpl_propertylist.h"
#include "cpl_mask.h"

#include <stdio.h>

CPL_BEGIN_DECLS

/*-----------------------------------------------------------------------------
                            Function prototypes
 -----------------------------------------------------------------------------*/

/* Image constructors */
cpl_image * cpl_image_new(cpl_size, cpl_size, cpl_type) CPL_ATTR_ALLOC;
cpl_image * cpl_image_wrap(cpl_size, cpl_size, cpl_type, void *) CPL_ATTR_ALLOC;
cpl_image * cpl_image_wrap_double(cpl_size, cpl_size, double *) CPL_ATTR_ALLOC;
cpl_image * cpl_image_wrap_float(cpl_size, cpl_size, float *) CPL_ATTR_ALLOC;
cpl_image * cpl_image_wrap_int(cpl_size, cpl_size, int *) CPL_ATTR_ALLOC;

cpl_image * cpl_image_wrap_double_complex(cpl_size, cpl_size,
                                          _Complex double *) CPL_ATTR_ALLOC;
cpl_image * cpl_image_wrap_float_complex(cpl_size, cpl_size,
                                         _Complex float *) CPL_ATTR_ALLOC;

cpl_image * cpl_image_load(const char *, cpl_type, cpl_size, cpl_size)
    CPL_ATTR_ALLOC;
cpl_image * cpl_image_load_window(const char *, cpl_type, cpl_size, cpl_size,
                                  cpl_size, cpl_size, cpl_size, cpl_size)
    CPL_ATTR_ALLOC;
cpl_image * cpl_image_new_from_mask(const cpl_mask *) CPL_ATTR_ALLOC;
cpl_image * cpl_image_labelise_mask_create(const cpl_mask *,
                                           cpl_size *) CPL_ATTR_ALLOC;

/* Get functions */
cpl_type cpl_image_get_type(const cpl_image *);
cpl_size cpl_image_get_size_x(const cpl_image *);
cpl_size cpl_image_get_size_y(const cpl_image *);
double cpl_image_get(const cpl_image *, cpl_size, cpl_size, int *);

_Complex double cpl_image_get_complex(const cpl_image *, cpl_size, cpl_size,
                                     int *);
cpl_error_code cpl_image_set_complex(cpl_image *, cpl_size, cpl_size,
                                     _Complex double);

cpl_mask * cpl_image_set_bpm(cpl_image *, cpl_mask *);
cpl_mask * cpl_image_unset_bpm(cpl_image *);
cpl_mask * cpl_image_get_bpm(cpl_image *);
const cpl_mask * cpl_image_get_bpm_const(const cpl_image *);
      void * cpl_image_get_data(cpl_image *);
const void * cpl_image_get_data_const(const cpl_image *);
      double * cpl_image_get_data_double(cpl_image *);
const double * cpl_image_get_data_double_const(const cpl_image *);
      float * cpl_image_get_data_float(cpl_image *);
const float * cpl_image_get_data_float_const(const cpl_image *);
      int * cpl_image_get_data_int(cpl_image *);
const int * cpl_image_get_data_int_const(const cpl_image *);
      _Complex double * cpl_image_get_data_double_complex(cpl_image *);
const _Complex double * cpl_image_get_data_double_complex_const(const cpl_image *);
      _Complex float * cpl_image_get_data_float_complex(cpl_image *);
const _Complex float * cpl_image_get_data_float_complex_const(const cpl_image *);

cpl_error_code cpl_image_conjugate(cpl_image *, const cpl_image *);
cpl_error_code cpl_image_fill_re_im(cpl_image *, cpl_image *,
                                    const cpl_image *);
cpl_error_code cpl_image_fill_abs_arg(cpl_image *, cpl_image *,
                                      const cpl_image *);

/* Set functions */
cpl_error_code cpl_image_set(cpl_image *, cpl_size, cpl_size, double);
cpl_error_code cpl_image_fill_rejected(cpl_image *, double);
cpl_error_code cpl_image_fill_window(cpl_image *, cpl_size, cpl_size,
                                     cpl_size, cpl_size, double);

/* Image destructor */
void cpl_image_delete(cpl_image *);
void * cpl_image_unwrap(cpl_image *);

/* Debugging functions */
cpl_error_code cpl_image_dump_structure(const cpl_image *, FILE *);
cpl_error_code cpl_image_dump_window(const cpl_image *, cpl_size, cpl_size,
                                     cpl_size, cpl_size, FILE *);

/* Others */
cpl_image * cpl_image_duplicate(const cpl_image *) CPL_ATTR_ALLOC;
cpl_image * cpl_image_cast(const cpl_image *, cpl_type) CPL_ATTR_ALLOC;

/* Saving function */
cpl_error_code cpl_image_save(const cpl_image *, const char *, cpl_type,
                              const cpl_propertylist *, unsigned);

CPL_END_DECLS

#endif 
