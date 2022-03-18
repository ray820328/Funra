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

#ifndef CPL_POLYNOMIAL_H
#define CPL_POLYNOMIAL_H

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include <cpl_macros.h>
#include <cpl_vector.h>
#include <cpl_bivector.h>
#include <cpl_error.h>
#include <stdio.h>

CPL_BEGIN_DECLS

/*-----------------------------------------------------------------------------
                                   New types
 -----------------------------------------------------------------------------*/

typedef struct _cpl_polynomial_ cpl_polynomial;

/*-----------------------------------------------------------------------------
                              Function prototypes
 -----------------------------------------------------------------------------*/

/* cpl_polynomial handling */
cpl_polynomial * cpl_polynomial_new(cpl_size) CPL_ATTR_ALLOC;
void cpl_polynomial_delete(cpl_polynomial *);
cpl_error_code cpl_polynomial_dump(const cpl_polynomial *, FILE *);
cpl_polynomial * cpl_polynomial_duplicate(const cpl_polynomial *)
    CPL_ATTR_ALLOC;
cpl_error_code cpl_polynomial_copy(cpl_polynomial *, const cpl_polynomial *);

/* Accessor functions */
double cpl_polynomial_get_coeff(const cpl_polynomial *, const cpl_size *);
cpl_error_code cpl_polynomial_set_coeff(cpl_polynomial *, const cpl_size *,
                                        double);

/* Basic operations */
int cpl_polynomial_compare(const cpl_polynomial *, const cpl_polynomial *,
                           double tol);
cpl_size cpl_polynomial_get_dimension(const cpl_polynomial *);
cpl_size cpl_polynomial_get_degree(const cpl_polynomial *);
double cpl_polynomial_eval(const cpl_polynomial *, const cpl_vector *);

cpl_polynomial * cpl_polynomial_extract(const cpl_polynomial *, cpl_size,
                                        const cpl_polynomial *) CPL_ATTR_ALLOC;

cpl_error_code cpl_polynomial_add(cpl_polynomial *,
                                  const cpl_polynomial *,
                                  const cpl_polynomial *);

cpl_error_code cpl_polynomial_subtract(cpl_polynomial *,
                                       const cpl_polynomial *,
                                       const cpl_polynomial *);

cpl_error_code cpl_polynomial_multiply(cpl_polynomial *,
                                       const cpl_polynomial *,
                                       const cpl_polynomial *);

cpl_error_code cpl_polynomial_multiply_scalar(cpl_polynomial *,
                                              const cpl_polynomial *, double);

cpl_error_code cpl_polynomial_derivative(cpl_polynomial *, cpl_size);

cpl_error_code cpl_polynomial_fit(cpl_polynomial    *,
                                  const cpl_matrix  *,
                                  const cpl_boolean *,
                                  const cpl_vector  *,
                                  const cpl_vector  *,
                                  cpl_boolean        ,
                                  const cpl_size    *,
                                  const cpl_size    *);

cpl_error_code cpl_vector_fill_polynomial_fit_residual(cpl_vector        *,
                                                       const cpl_vector  *,
                                                       const cpl_vector  *,
                                                       const cpl_polynomial *,
                                                       const cpl_matrix  *,
                                                       double *);

/* Basic operations on 1d-polynomials */
double cpl_polynomial_eval_1d(const cpl_polynomial *, double, double *);
double cpl_polynomial_eval_1d_diff(const cpl_polynomial *, double, double,
                                   double *);
cpl_error_code cpl_vector_fill_polynomial(cpl_vector *, const cpl_polynomial *,
                                          double, double);
cpl_error_code cpl_polynomial_solve_1d(const cpl_polynomial *, double, double *,
                                       cpl_size);

cpl_error_code cpl_polynomial_shift_1d(cpl_polynomial *, cpl_size, double);
cpl_polynomial * cpl_polynomial_fit_1d_create(const cpl_vector *,
                                              const cpl_vector *, cpl_size,
                                              double *) CPL_ATTR_DEPRECATED;

/* Basic operations on 2d-polynomials */
cpl_polynomial * cpl_polynomial_fit_2d_create(const cpl_bivector *,
                                              const cpl_vector *,
                                              cpl_size, double *)
    CPL_ATTR_DEPRECATED;

CPL_END_DECLS

#endif

