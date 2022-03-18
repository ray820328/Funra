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

#ifndef CPL_MATRIX_IMPL_H
#define CPL_MATRIX_IMPL_H

#include "cpl_matrix.h"
 
CPL_BEGIN_DECLS

cpl_matrix * cpl_matrix_product_normal_create(const cpl_matrix *)
    CPL_ATTR_ALLOC;

cpl_error_code cpl_matrix_solve_spd(cpl_matrix *, cpl_matrix *);

cpl_error_code cpl_matrix_product(cpl_matrix *, const cpl_matrix *,
                                  const cpl_matrix *);

cpl_error_code cpl_matrix_product_bilinear(cpl_matrix *,
                                           const cpl_matrix *,
                                           const cpl_matrix *);

cpl_error_code cpl_matrix_solve_chol_transpose(const cpl_matrix *,
                                               cpl_matrix *);

cpl_error_code cpl_matrix_solve_(cpl_matrix *, cpl_matrix *, cpl_array *)
    CPL_ATTR_NONNULL;

cpl_error_code cpl_matrix_fill_identity(cpl_matrix *);

CPL_END_DECLS

#endif
/* end of cpl_matrix_impl.h */
