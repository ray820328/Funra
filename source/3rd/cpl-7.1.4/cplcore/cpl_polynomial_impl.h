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

#ifndef CPL_POLYNOMIAL_IMPL_H
#define CPL_POLYNOMIAL_IMPL_H

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include "cpl_polynomial.h"

CPL_BEGIN_DECLS

/*-----------------------------------------------------------------------------
                             Macro definitions
 -----------------------------------------------------------------------------*/

#undef CPL_POLYNOMIAL_UNION_IS_DOUBLE_SZ
#if defined(SIZEOF_VOID_P) && defined(SIZEOF_DOUBLE)
#  if SIZEOF_VOID_P <= SIZEOF_DOUBLE
#    define CPL_POLYNOMIAL_UNION_IS_DOUBLE_SZ
#  endif
#endif

/*-----------------------------------------------------------------------------
                            Function prototypes
 -----------------------------------------------------------------------------*/

void cpl_polynomial_empty(cpl_polynomial *);

cpl_error_code cpl_polynomial_fit_1d(cpl_polynomial *, const cpl_vector *,
                                     const cpl_vector *, cpl_size, cpl_size,
                                     cpl_boolean, double *);

void cpl_polynomial_shift_double(double *, cpl_size, double)
    CPL_ATTR_NONNULL;

cpl_error_code cpl_polynomial_solve_1d_(const cpl_polynomial *, double,
                                        double *, cpl_size, cpl_boolean);

cpl_size cpl_polynomial_get_1d_size(const cpl_polynomial *)
    CPL_ATTR_PURE CPL_ATTR_NONNULL;

double cpl_polynomial_eval_monomial(const cpl_polynomial *,
                                    const cpl_vector *);

CPL_END_DECLS

#endif 

