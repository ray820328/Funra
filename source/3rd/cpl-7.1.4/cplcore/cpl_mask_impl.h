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

#ifndef CPL_MASK_IMPL_H
#define CPL_MASK_IMPL_H

#include "cpl_mask.h"

CPL_BEGIN_DECLS

cpl_size cpl_mask_get_first_window(const cpl_mask *, cpl_size, cpl_size,
                                   cpl_size, cpl_size, cpl_binary);

void cpl_mask_and_(cpl_binary        *,
                   const cpl_binary  *,
                   const cpl_binary  *,
                   size_t)
    CPL_INTERNAL
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(1,3)))
#endif
    ;

void cpl_mask_or_(cpl_binary        *,
                  const cpl_binary  *,
                  const cpl_binary  *,
                  size_t)
    CPL_INTERNAL
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(1,3)))
#endif
    ;

void cpl_mask_xor_(cpl_binary        *,
                   const cpl_binary  *,
                   const cpl_binary  *,
                   size_t)
    CPL_INTERNAL
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(1,3)))
#endif
    ;

void cpl_mask_and_scalar(cpl_binary        *,
                         const cpl_binary  *,
                         cpl_bitmask,
                         size_t)
    CPL_INTERNAL
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(1)))
#endif
    ;

void cpl_mask_or_scalar(cpl_binary        *,
                        const cpl_binary  *,
                        cpl_bitmask,
                        size_t)
    CPL_INTERNAL
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(1)))
#endif
    ;

void cpl_mask_xor_scalar(cpl_binary        *,
                         const cpl_binary  *,
                         cpl_bitmask,
                         size_t)
    CPL_INTERNAL
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(1)))
#endif
    ;

cpl_mask * cpl_mask_extract_(const cpl_mask *,
                             cpl_size,
                             cpl_size,
                             cpl_size,
                             cpl_size)
    CPL_ATTR_ALLOC;

void cpl_mask_fill_window(cpl_mask *,
                          cpl_size,
                          cpl_size,
                          cpl_size,
                          cpl_size,
                          cpl_binary)
    CPL_INTERNAL CPL_ATTR_NONNULL;


CPL_END_DECLS

#endif
/* end of cpl_mask_impl.h */
