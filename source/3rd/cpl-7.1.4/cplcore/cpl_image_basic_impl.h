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

#ifndef CPL_IMAGE_BASIC_IMPL_H
#define CPL_IMAGE_BASIC_IMPL_H

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include "cpl_image_basic.h"

CPL_BEGIN_DECLS

/*-----------------------------------------------------------------------------
                            Function prototypes
 -----------------------------------------------------------------------------*/

cpl_image * cpl_image_min_create(const cpl_image *, const cpl_image *)
    CPL_INTERNAL
    CPL_ATTR_ALLOC;

void cpl_image_or_mask(cpl_image *, const cpl_image *, const cpl_image *)
    CPL_INTERNAL
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(1,3)))
#endif
    ;

void cpl_image_or_mask_unary(cpl_image *, const cpl_image *)
    CPL_INTERNAL
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(1)))
#endif
    ;

CPL_END_DECLS

#endif 
