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

#ifndef CPL_IMAGE_IO_IMPL_H
#define CPL_IMAGE_IO_IMPL_H

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include <complex.h>

#include "cpl_image_io.h"
#include "cpl_tools.h"

#include <fitsio.h>

CPL_BEGIN_DECLS

/*-----------------------------------------------------------------------------
                            Function prototypes
 -----------------------------------------------------------------------------*/

cpl_image * cpl_image_wrap_(cpl_size, cpl_size, cpl_type, void *)
    CPL_ATTR_ALLOC;

/* Special extractor functions for complex images */
cpl_image * cpl_image_extract_real(const cpl_image *) CPL_ATTR_ALLOC;
cpl_image * cpl_image_extract_imag(const cpl_image *) CPL_ATTR_ALLOC;

cpl_image * cpl_image_extract_mod  (const cpl_image *) CPL_ATTR_ALLOC;
cpl_image * cpl_image_extract_phase(const cpl_image *) CPL_ATTR_ALLOC;

cpl_error_code cpl_image_fill_real (cpl_image *, const cpl_image *);
cpl_error_code cpl_image_fill_imag (cpl_image *, const cpl_image *);
cpl_error_code cpl_image_fill_mod  (cpl_image *, const cpl_image *);
cpl_error_code cpl_image_fill_phase(cpl_image *, const cpl_image *);

/* FIXME: Export ?  - add other types  ? */
cpl_error_code cpl_image_fill_int(cpl_image *, int);

cpl_image * cpl_image_load_(fitsfile *, int *, CPL_FITSIO_TYPE[], cpl_type *,
                            const char *, cpl_size, cpl_size,
                            cpl_boolean, cpl_size, cpl_size, cpl_size, cpl_size)
    CPL_ATTR_ALLOC;

cpl_error_code cpl_image_save_(const cpl_image * const*, cpl_size, cpl_boolean,
                               const char *, cpl_type,
                               const cpl_propertylist *, unsigned)
    CPL_INTERNAL;

void cpl_image_set_(cpl_image *, cpl_size, cpl_size, double)
    CPL_ATTR_NONNULL;

double cpl_image_get_(const cpl_image *,
                      cpl_size,
                      cpl_size)
#ifdef NDEBUG
    CPL_ATTR_PURE
#endif
    CPL_ATTR_NONNULL;

CPL_END_DECLS

#endif 

