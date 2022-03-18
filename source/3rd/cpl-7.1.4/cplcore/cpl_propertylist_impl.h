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

#ifndef CPL_PROPERTYLIST_IMPL_H
#define CPL_PROPERTYLIST_IMPL_H

#include <fitsio.h>

#include "cpl_propertylist.h"
#include "cpl_property_impl.h"

CPL_BEGIN_DECLS

cpl_error_code cpl_propertylist_to_fitsfile(fitsfile *file,
                                            const cpl_propertylist *self,
                                            const char *to_rm); 

cpl_propertylist *cpl_propertylist_from_fitsfile(fitsfile *file);

cpl_propertylist *
cpl_propertylist_load_name_(const char*,
                            cpl_size,
                            cpl_size,
                            const cpl_cstr *[],
                            cpl_size,
                            const cpl_cstr *[],
                            int)

#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(1)))
#endif
    
    CPL_ATTR_ALLOC;

cpl_error_code cpl_fits_add_properties(fitsfile *,
                                       const cpl_propertylist *, 
                                       const char *);

cpl_error_code
cpl_propertylist_set_comment_cx(cpl_propertylist *, const cpl_cstr *,
                                const cpl_cstr *)
    CPL_ATTR_NONNULL;

int
cpl_propertylist_has_cx(const cpl_propertylist *, const cpl_cstr *)
    CPL_ATTR_NONNULL;

cpl_type
cpl_propertylist_get_type_cx(const cpl_propertylist *, const cpl_cstr *)
    CPL_ATTR_NONNULL;

int
cpl_propertylist_erase_cx(cpl_propertylist *, const cpl_cstr *)
    CPL_ATTR_NONNULL;

CPL_END_DECLS

#endif /* CPL_PROPERTYLIST_IMPL_H */
