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

#ifndef CPL_IMAGELIST_IO_H
#define CPL_IMAGELIST_IO_H

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include "cpl_image.h"
#include "cpl_imagelist.h"
#include "cpl_vector.h"

CPL_BEGIN_DECLS

/*-----------------------------------------------------------------------------
                            Function prototypes
 -----------------------------------------------------------------------------*/

/* Imagelist constructors */
cpl_imagelist * cpl_imagelist_new(void) CPL_ATTR_ALLOC;
cpl_imagelist * cpl_imagelist_load(const char *, cpl_type, cpl_size)
    CPL_ATTR_ALLOC;
cpl_imagelist * cpl_imagelist_load_window(const char *, cpl_type, cpl_size,
                                          cpl_size, cpl_size, cpl_size,
                                          cpl_size) CPL_ATTR_ALLOC;

/* Imagelist accessors */
cpl_size cpl_imagelist_get_size(const cpl_imagelist *);
      cpl_image * cpl_imagelist_get(cpl_imagelist *, cpl_size);
const cpl_image * cpl_imagelist_get_const(const cpl_imagelist *, cpl_size);
cpl_error_code cpl_imagelist_set(cpl_imagelist *, cpl_image *, cpl_size);
cpl_image * cpl_imagelist_unset(cpl_imagelist *, cpl_size);

/* Imagelist destructor */
void cpl_imagelist_delete(cpl_imagelist *);
void cpl_imagelist_unwrap(cpl_imagelist *);
void cpl_imagelist_empty(cpl_imagelist *);

/* Others */
cpl_imagelist * cpl_imagelist_duplicate(const cpl_imagelist *) CPL_ATTR_ALLOC;
cpl_error_code cpl_imagelist_erase(cpl_imagelist *, const cpl_vector *);
int cpl_imagelist_is_uniform(const cpl_imagelist *);

cpl_error_code cpl_imagelist_cast(cpl_imagelist *, const cpl_imagelist *,
                                  cpl_type);

cpl_error_code cpl_imagelist_dump_structure(const cpl_imagelist *, FILE *);
cpl_error_code cpl_imagelist_dump_window(const cpl_imagelist *,
                     cpl_size, cpl_size, cpl_size, cpl_size, FILE *);

/* Saving function */
cpl_error_code cpl_imagelist_save(const cpl_imagelist *, const char *,
                                  cpl_type, const cpl_propertylist *,
                                  unsigned);
CPL_END_DECLS

#endif 
