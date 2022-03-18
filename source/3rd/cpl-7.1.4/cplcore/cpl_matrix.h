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

#ifndef CPL_MATRIX_H
#define CPL_MATRIX_H

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include <cpl_macros.h>
#include <cpl_type.h>
#include <cpl_error.h>
#include <cpl_array.h>

#include <stdio.h>

CPL_BEGIN_DECLS

/*-----------------------------------------------------------------------------
                                   Typedefs
 -----------------------------------------------------------------------------*/

typedef struct _cpl_matrix_ cpl_matrix;

/*-----------------------------------------------------------------------------
                            Function prototypes
 -----------------------------------------------------------------------------*/

/*
 * IO methods
 */

cpl_matrix *cpl_matrix_new(cpl_size, cpl_size) CPL_ATTR_ALLOC;
cpl_matrix *cpl_matrix_wrap(cpl_size, cpl_size, double *) CPL_ATTR_ALLOC;
void cpl_matrix_delete(cpl_matrix *);
void *cpl_matrix_unwrap(cpl_matrix *);
void cpl_matrix_dump(const cpl_matrix *, FILE *);

/*
 * Accessors 
 */

cpl_size cpl_matrix_get_nrow(const cpl_matrix *);
cpl_size cpl_matrix_get_ncol(const cpl_matrix *);
      double *cpl_matrix_get_data(cpl_matrix *);
const double *cpl_matrix_get_data_const(const cpl_matrix *);
double cpl_matrix_get(const cpl_matrix *, cpl_size, cpl_size);
cpl_error_code cpl_matrix_set(cpl_matrix *, cpl_size, cpl_size, double);

/*
 * Copying methods
 */

cpl_matrix *cpl_matrix_duplicate(const cpl_matrix *) CPL_ATTR_ALLOC;
cpl_matrix *cpl_matrix_extract(const cpl_matrix *, 
                               cpl_size, cpl_size, cpl_size, cpl_size,
                               cpl_size, cpl_size) CPL_ATTR_ALLOC;
cpl_matrix *cpl_matrix_extract_row(const cpl_matrix *, cpl_size) CPL_ATTR_ALLOC;
cpl_matrix *cpl_matrix_extract_column(const cpl_matrix *, cpl_size)
    CPL_ATTR_ALLOC;
cpl_matrix *cpl_matrix_extract_diagonal(const cpl_matrix *, cpl_size)
    CPL_ATTR_ALLOC;

/*
 * Writing methods
 */

cpl_error_code cpl_matrix_copy(cpl_matrix *, const cpl_matrix *,
                               cpl_size, cpl_size);
cpl_error_code cpl_matrix_fill(cpl_matrix *, double);
cpl_error_code cpl_matrix_fill_row(cpl_matrix *, double, cpl_size);
cpl_error_code cpl_matrix_fill_column(cpl_matrix *, double, cpl_size);
cpl_error_code cpl_matrix_fill_diagonal(cpl_matrix *, double, cpl_size);
cpl_error_code cpl_matrix_fill_window(cpl_matrix *, double, 
                                      cpl_size, cpl_size, cpl_size, cpl_size);

/*
 * Test methods
 */

int cpl_matrix_is_zero(const cpl_matrix *, double);
int cpl_matrix_is_diagonal(const cpl_matrix *, double);
int cpl_matrix_is_identity(const cpl_matrix *, double);

/*
 * Sorting methods
 */

cpl_error_code cpl_matrix_sort_rows(cpl_matrix *, int);
cpl_error_code cpl_matrix_sort_columns(cpl_matrix *, int);

/*
 * Handling methods
 */

cpl_error_code cpl_matrix_threshold_small(cpl_matrix *, double);
cpl_error_code cpl_matrix_swap_rows(cpl_matrix *, cpl_size, cpl_size);
cpl_error_code cpl_matrix_swap_columns(cpl_matrix *, cpl_size, cpl_size);
cpl_error_code cpl_matrix_swap_rowcolumn(cpl_matrix *, cpl_size);
cpl_error_code cpl_matrix_flip_rows(cpl_matrix *);
cpl_error_code cpl_matrix_flip_columns(cpl_matrix *);
cpl_error_code cpl_matrix_erase_rows(cpl_matrix *, cpl_size, cpl_size);
cpl_error_code cpl_matrix_erase_columns(cpl_matrix *, cpl_size, cpl_size);
cpl_error_code cpl_matrix_set_size(cpl_matrix *, cpl_size, cpl_size);
cpl_error_code cpl_matrix_resize(cpl_matrix *, cpl_size, cpl_size,
                                 cpl_size, cpl_size);
cpl_error_code cpl_matrix_append(cpl_matrix *, const cpl_matrix *, int);   

/*
 * Basic operations
 */

cpl_error_code cpl_matrix_shift(cpl_matrix *, cpl_size, cpl_size);
cpl_error_code cpl_matrix_add(cpl_matrix *, const cpl_matrix *);
cpl_error_code cpl_matrix_subtract(cpl_matrix *, const cpl_matrix *);
cpl_error_code cpl_matrix_multiply(cpl_matrix *, const cpl_matrix *);
cpl_error_code cpl_matrix_divide(cpl_matrix *, const cpl_matrix *);

cpl_error_code cpl_matrix_add_scalar(cpl_matrix *, double);
cpl_error_code cpl_matrix_subtract_scalar(cpl_matrix *, double);
cpl_error_code cpl_matrix_multiply_scalar(cpl_matrix *, double);
cpl_error_code cpl_matrix_divide_scalar(cpl_matrix *, double);
cpl_error_code cpl_matrix_logarithm(cpl_matrix *, double);
cpl_error_code cpl_matrix_exponential(cpl_matrix *, double);
cpl_error_code cpl_matrix_power(cpl_matrix *, double);
cpl_matrix *cpl_matrix_product_create(const cpl_matrix *,
                                      const cpl_matrix *) CPL_ATTR_ALLOC;
cpl_matrix *cpl_matrix_transpose_create(const cpl_matrix *) CPL_ATTR_ALLOC;
cpl_error_code cpl_matrix_product_normal(cpl_matrix *,
                                         const cpl_matrix *);

cpl_error_code cpl_matrix_product_transpose(cpl_matrix *,
                                            const cpl_matrix *,
                                            const cpl_matrix *);

/*
 * More complex operations
 */

double cpl_matrix_get_determinant(const cpl_matrix *);
cpl_matrix *cpl_matrix_solve(const cpl_matrix *,
                             const cpl_matrix *) CPL_ATTR_ALLOC;
cpl_matrix *cpl_matrix_solve_normal(const cpl_matrix *,
                                    const cpl_matrix *) CPL_ATTR_ALLOC;
cpl_matrix *cpl_matrix_solve_svd(const cpl_matrix *,
                                 const cpl_matrix *) CPL_ATTR_ALLOC;
cpl_matrix *cpl_matrix_invert_create(const cpl_matrix *) CPL_ATTR_ALLOC;

cpl_error_code cpl_matrix_decomp_lu(cpl_matrix *, cpl_array *, int *);
cpl_error_code cpl_matrix_solve_lu(const cpl_matrix *, cpl_matrix *,
                                   const cpl_array *);
cpl_error_code cpl_matrix_decomp_chol(cpl_matrix *);
cpl_error_code cpl_matrix_solve_chol(const cpl_matrix *, cpl_matrix *);

/*
 * Stats methods
 */

double cpl_matrix_get_mean(const cpl_matrix *);
double cpl_matrix_get_median(const cpl_matrix *);
double cpl_matrix_get_stdev(const cpl_matrix *);
double cpl_matrix_get_min(const cpl_matrix *);
double cpl_matrix_get_max(const cpl_matrix *);
cpl_error_code cpl_matrix_get_minpos(const cpl_matrix *, cpl_size *,
                                     cpl_size *);
cpl_error_code cpl_matrix_get_maxpos(const cpl_matrix *, cpl_size *,
                                     cpl_size *);

CPL_END_DECLS

#endif
/* end of cpl_matrix.h */

