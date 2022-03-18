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

#ifndef CPL_VECTOR_IMPL_H
#define CPL_VECTOR_IMPL_H

#include "cpl_vector.h"

CPL_BEGIN_DECLS

double * cpl_vector_rewrap(cpl_vector *, cpl_size, double *);

cpl_size cpl_vector_get_size_(const cpl_vector *)
    CPL_ATTR_PURE CPL_ATTR_NONNULL;

double * cpl_vector_get_data_(cpl_vector *)
#ifdef NDEBUG
    CPL_ATTR_PURE
#endif
    CPL_ATTR_NONNULL;

const double * cpl_vector_get_data_const_(const cpl_vector *)
#ifdef NDEBUG
    CPL_ATTR_PURE
#endif
    CPL_ATTR_NONNULL;

double cpl_vector_get_(const cpl_vector *, cpl_size)
#ifdef NDEBUG
    CPL_ATTR_PURE
#endif
    CPL_ATTR_NONNULL;

void cpl_vector_set_(cpl_vector *,
                     cpl_size,
                     double)
    CPL_ATTR_NONNULL;

#endif
/* end of cpl_vector_impl.h */
