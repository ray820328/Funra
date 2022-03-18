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

#ifndef CPL_TABLE_H
#define CPL_TABLE_H

#include <stdio.h>

#include "cpl_io.h"
#include "cpl_type.h"
#include "cpl_propertylist.h"
#include "cpl_array.h"

CPL_BEGIN_DECLS


typedef struct _cpl_table_ cpl_table;

/*
 * Legal logical operators for selections:
 */

typedef enum _cpl_table_select_operator_
{
    CPL_EQUAL_TO = 0,
    CPL_NOT_EQUAL_TO,
    CPL_GREATER_THAN,
    CPL_NOT_GREATER_THAN,
    CPL_LESS_THAN,
    CPL_NOT_LESS_THAN
} cpl_table_select_operator;

/*
 * Constructors and destructors:
 */

cpl_table *cpl_table_new(cpl_size)
    CPL_ATTR_ALLOC;

cpl_error_code cpl_table_new_column(cpl_table *, const char *, cpl_type);
cpl_error_code cpl_table_new_column_array(cpl_table *, const char *,
                                          cpl_type, cpl_size);

cpl_error_code cpl_table_set_column_savetype(cpl_table *, 
                                             const char *name, cpl_type);

cpl_error_code cpl_table_wrap_int(cpl_table *, int *, const char *);
cpl_error_code cpl_table_wrap_long_long(cpl_table *, long long *, const char *);
cpl_error_code cpl_table_wrap_float(cpl_table *, float *, const char *);
cpl_error_code cpl_table_wrap_double(cpl_table *, double *, const char *);
cpl_error_code cpl_table_wrap_float_complex(cpl_table *, 
                                            _Complex float *, const char *);
cpl_error_code cpl_table_wrap_double_complex(cpl_table *, 
                                             _Complex double *, const char *);
cpl_error_code cpl_table_wrap_string(cpl_table *, char **, const char *);

void *cpl_table_unwrap(cpl_table *, const char *);

cpl_error_code cpl_table_copy_structure(cpl_table *, const cpl_table *);

void cpl_table_delete(cpl_table *);


/*
 * Methods:
 */

cpl_size cpl_table_get_nrow(const cpl_table *);
cpl_size cpl_table_get_ncol(const cpl_table *);

cpl_type cpl_table_get_column_type(const cpl_table *, const char *name);

cpl_error_code cpl_table_set_column_unit(cpl_table *, const char *name, 
                                         const char *unit);
const char *cpl_table_get_column_unit(const cpl_table *, const char *name);

cpl_error_code cpl_table_set_column_format(cpl_table *, const char *name, 
                                           const char *format);
const char *cpl_table_get_column_format(const cpl_table *, const char *name);

cpl_error_code cpl_table_set_column_depth(cpl_table *, const char *, cpl_size);
cpl_size cpl_table_get_column_depth(const cpl_table *, const char *);
cpl_size cpl_table_get_column_dimensions(const cpl_table *, const char *);
cpl_error_code cpl_table_set_column_dimensions(cpl_table *, const char *,
                                               const cpl_array *);
cpl_size cpl_table_get_column_dimension(const cpl_table *, 
                                        const char *, cpl_size);

int *cpl_table_get_data_int(cpl_table *, const char *name);
const int *cpl_table_get_data_int_const(const cpl_table *, 
                                        const char *name);
long long *cpl_table_get_data_long_long(cpl_table *, const char *name);
const long long *cpl_table_get_data_long_long_const(const cpl_table *,
                                                    const char *name);
float *cpl_table_get_data_float(cpl_table *, const char *name);
const float *cpl_table_get_data_float_const(const cpl_table *, 
                                            const char *name);
double *cpl_table_get_data_double(cpl_table *, const char *name);
const double *cpl_table_get_data_double_const(const cpl_table *, 
                                              const char *name);
_Complex float *cpl_table_get_data_float_complex(cpl_table *, const char *name);
const _Complex float *cpl_table_get_data_float_complex_const(const cpl_table *, 
                                                            const char *name);
_Complex double *cpl_table_get_data_double_complex(cpl_table *, const char *);
const _Complex double *cpl_table_get_data_double_complex_const(const cpl_table *,
                                                              const char *);
char **cpl_table_get_data_string(cpl_table *, const char *name);
const char **cpl_table_get_data_string_const(const cpl_table *, 
                                                    const char *name);
cpl_array **cpl_table_get_data_array(cpl_table *, const char *);
const cpl_array **cpl_table_get_data_array_const(const cpl_table *, 
                                                        const char *);

double cpl_table_get(const cpl_table *, const char *, cpl_size, int *null);
int cpl_table_get_int(const cpl_table *, const char *, cpl_size, int *null);
long long cpl_table_get_long_long(const cpl_table *, const char *, cpl_size,
                                  int *null);
float cpl_table_get_float(const cpl_table *, const char *, cpl_size, int *null);
double cpl_table_get_double(const cpl_table *, const char *, cpl_size, 
                                                                int *null);
_Complex double cpl_table_get_complex(const cpl_table *,
                                     const char *, cpl_size, int *);
_Complex float cpl_table_get_float_complex(const cpl_table *, 
                                          const char *, cpl_size, int *null);
_Complex double cpl_table_get_double_complex(const cpl_table *, 
                                          const char *, cpl_size, int *null);
const char *cpl_table_get_string(const cpl_table *, const char *, cpl_size);
const cpl_array *cpl_table_get_array(const cpl_table *, const char *, cpl_size);

cpl_error_code cpl_table_set(cpl_table *, const char *, cpl_size, double);
cpl_error_code cpl_table_set_int(cpl_table *, const char *, cpl_size, int);
cpl_error_code cpl_table_set_long_long(cpl_table *, const char *, cpl_size,
                                       long long);
cpl_error_code cpl_table_set_float(cpl_table *, const char *, cpl_size, float);
cpl_error_code cpl_table_set_double(cpl_table *, const char *, cpl_size, 
                                                                    double);
cpl_error_code cpl_table_set_complex(cpl_table *, 
                                     const char *, cpl_size, _Complex double);
cpl_error_code cpl_table_set_float_complex(cpl_table *, 
                                     const char *, cpl_size, _Complex float);
cpl_error_code cpl_table_set_double_complex(cpl_table *, 
                                     const char *, cpl_size, _Complex double);
cpl_error_code cpl_table_set_string(cpl_table *, const char *, cpl_size, 
                                    const char *);
cpl_error_code cpl_table_set_array(cpl_table *, const char *, cpl_size, 
                                   const cpl_array *);

cpl_error_code cpl_table_fill_column_window(cpl_table *, const char *, 
                                     cpl_size, cpl_size, double);
cpl_error_code cpl_table_fill_column_window_int(cpl_table *, const char *, 
                                         cpl_size, cpl_size, int);
cpl_error_code cpl_table_fill_column_window_long_long(cpl_table *, const char *,
                                                      cpl_size, cpl_size,
                                                      long long);
cpl_error_code cpl_table_fill_column_window_float(cpl_table *, const char *, 
                                           cpl_size, cpl_size, float);
cpl_error_code cpl_table_fill_column_window_double(cpl_table *, const char *, 
                                            cpl_size, cpl_size, double);
cpl_error_code cpl_table_fill_column_window_complex(cpl_table *, const char *, 
                                     cpl_size, cpl_size, _Complex double);
cpl_error_code cpl_table_fill_column_window_float_complex(cpl_table *, 
                     const char *, cpl_size, cpl_size, _Complex float);
cpl_error_code cpl_table_fill_column_window_double_complex(cpl_table *, 
                     const char *, cpl_size, cpl_size, _Complex double);
cpl_error_code cpl_table_fill_column_window_string(cpl_table *, const char *, 
                                      cpl_size, cpl_size, const char *);
cpl_error_code cpl_table_fill_column_window_array(cpl_table *, const char *, 
                                      cpl_size, cpl_size, const cpl_array *);

cpl_error_code cpl_table_copy_data_int(cpl_table *, const char *, const int *);
cpl_error_code cpl_table_copy_data_long_long(cpl_table *, const char *,
                                             const long long *);
cpl_error_code cpl_table_copy_data_float(cpl_table *, const char *, 
                                         const float *);
cpl_error_code cpl_table_copy_data_double(cpl_table *, const char *, 
                                          const double *);
cpl_error_code cpl_table_copy_data_float_complex(cpl_table *, const char *, 
                                                 const _Complex float *);
cpl_error_code cpl_table_copy_data_double_complex(cpl_table *, const char *, 
                                                  const _Complex double *);
cpl_error_code cpl_table_copy_data_string(cpl_table *, const char *, 
                                          const char **);

cpl_error_code cpl_table_shift_column(cpl_table *, const char *, cpl_size);

cpl_error_code cpl_table_set_invalid(cpl_table *, const char *, cpl_size);

cpl_error_code cpl_table_set_column_invalid(cpl_table *, 
                                            const char *, cpl_size, cpl_size);

int cpl_table_is_valid(const cpl_table *, const char *, cpl_size);
cpl_size cpl_table_count_invalid(const cpl_table *, const char *);
int cpl_table_has_invalid(const cpl_table *table, const char *name);
int cpl_table_has_valid(const cpl_table *table, const char *name);

cpl_error_code cpl_table_fill_invalid_int(cpl_table *, const char *, 
                                              int);
cpl_error_code cpl_table_fill_invalid_long_long(cpl_table *, const char *,
                                                long long);
cpl_error_code cpl_table_fill_invalid_float(cpl_table *, const char *, 
                                                float);
cpl_error_code cpl_table_fill_invalid_double(cpl_table *, const char *, 
                                                 double);
cpl_error_code cpl_table_fill_invalid_float_complex(cpl_table *, const char *, 
                                                _Complex float);
cpl_error_code cpl_table_fill_invalid_double_complex(cpl_table *, const char *, 
                                                 _Complex double);

cpl_error_code cpl_table_erase_column(cpl_table *, const char *);
cpl_error_code cpl_table_move_column(cpl_table *, const char *, cpl_table *);
cpl_error_code cpl_table_duplicate_column(cpl_table *, const char *, 
                                          const cpl_table *, const char *);

cpl_error_code cpl_table_name_column(cpl_table *, const char *, const char *);
int cpl_table_has_column(const cpl_table *, const char *);

const char *cpl_table_get_column_name(const cpl_table *) CPL_ATTR_DEPRECATED;
cpl_array *cpl_table_get_column_names(const cpl_table *table);

cpl_error_code cpl_table_set_size(cpl_table *, cpl_size);

cpl_table *cpl_table_duplicate(const cpl_table *)
    CPL_ATTR_ALLOC;

cpl_table *cpl_table_extract(const cpl_table *, cpl_size, cpl_size)
    CPL_ATTR_ALLOC;
cpl_table *cpl_table_extract_selected(const cpl_table *)
    CPL_ATTR_ALLOC;
cpl_array *cpl_table_where_selected(const cpl_table *)
    CPL_ATTR_ALLOC;

cpl_error_code cpl_table_erase_selected(cpl_table *);
cpl_error_code cpl_table_erase_window(cpl_table *, cpl_size, cpl_size);
cpl_error_code cpl_table_insert_window(cpl_table *, cpl_size, cpl_size);

int cpl_table_compare_structure(const cpl_table *, const cpl_table *);

cpl_error_code cpl_table_insert(cpl_table *, const cpl_table *, cpl_size);

cpl_error_code cpl_table_cast_column(cpl_table *, 
                                     const char *, const char *, cpl_type);

cpl_error_code cpl_table_add_columns(cpl_table *, 
                                     const char *, const char *);
cpl_error_code cpl_table_subtract_columns(cpl_table *, 
                                          const char *, const char *);
cpl_error_code cpl_table_multiply_columns(cpl_table *, 
                                          const char *, const char *);
cpl_error_code cpl_table_divide_columns(cpl_table *, 
                                        const char *, const char *);

cpl_error_code cpl_table_add_scalar(cpl_table *, const char *, double);
cpl_error_code cpl_table_subtract_scalar(cpl_table *, const char *, double);
cpl_error_code cpl_table_multiply_scalar(cpl_table *, const char *, double);
cpl_error_code cpl_table_divide_scalar(cpl_table *, const char *, double);
cpl_error_code cpl_table_add_scalar_complex(cpl_table *, 
                                            const char *, _Complex double);
cpl_error_code cpl_table_subtract_scalar_complex(cpl_table *, 
                                                 const char *, _Complex double);
cpl_error_code cpl_table_multiply_scalar_complex(cpl_table *, 
                                                 const char *, _Complex double);
cpl_error_code cpl_table_divide_scalar_complex(cpl_table *, 
                                               const char *, _Complex double);
cpl_error_code cpl_table_abs_column(cpl_table *, const char *);
cpl_error_code cpl_table_logarithm_column(cpl_table *, const char *, double);
cpl_error_code cpl_table_power_column(cpl_table *, const char *, double);
cpl_error_code cpl_table_exponential_column(cpl_table *, const char *, double);
cpl_error_code cpl_table_conjugate_column(cpl_table *, const char *);
cpl_error_code cpl_table_real_column(cpl_table *, const char *);
cpl_error_code cpl_table_imag_column(cpl_table *, const char *);
cpl_error_code cpl_table_arg_column(cpl_table *, const char *);

cpl_error_code cpl_table_erase_invalid_rows(cpl_table *);
cpl_error_code cpl_table_erase_invalid(cpl_table *);

double cpl_table_get_column_max(const cpl_table *, const char *);
double cpl_table_get_column_min(const cpl_table *, const char *);
cpl_error_code cpl_table_get_column_maxpos(const cpl_table *, 
                                           const char *, cpl_size *);
cpl_error_code cpl_table_get_column_minpos(const cpl_table *, 
                                           const char *, cpl_size *);

_Complex double cpl_table_get_column_mean_complex(const cpl_table *, 
                                                 const char *);
double cpl_table_get_column_mean(const cpl_table *, const char *);
double cpl_table_get_column_median(const cpl_table *, const char *);
double cpl_table_get_column_stdev(const cpl_table *, const char *);

cpl_error_code cpl_table_sort(cpl_table *, const cpl_propertylist *);

cpl_size cpl_table_and_selected_window(cpl_table *, cpl_size, cpl_size);
cpl_size cpl_table_or_selected_window(cpl_table *, cpl_size, cpl_size);

cpl_size cpl_table_not_selected(cpl_table *);

cpl_error_code cpl_table_select_row(cpl_table *, cpl_size);
cpl_error_code cpl_table_unselect_row(cpl_table *, cpl_size);
cpl_error_code cpl_table_select_all(cpl_table *);
cpl_error_code cpl_table_unselect_all(cpl_table *);

cpl_size cpl_table_and_selected_invalid(cpl_table *, const char *);
cpl_size cpl_table_or_selected_invalid(cpl_table *, const char *);

cpl_size cpl_table_and_selected_int(cpl_table *, const char *, 
                                  cpl_table_select_operator, int);
cpl_size cpl_table_or_selected_int(cpl_table *, const char *, 
                                  cpl_table_select_operator, int);
cpl_size cpl_table_and_selected_long_long(cpl_table *, const char *,
                                          cpl_table_select_operator, long long);
cpl_size cpl_table_or_selected_long_long(cpl_table *, const char *,
                                         cpl_table_select_operator, long long);
cpl_size cpl_table_and_selected_float(cpl_table *, const char *, 
                                    cpl_table_select_operator, float);
cpl_size cpl_table_or_selected_float(cpl_table *, const char *, 
                                    cpl_table_select_operator, float);
cpl_size cpl_table_and_selected_double(cpl_table *, const char *, 
                                     cpl_table_select_operator, double);
cpl_size cpl_table_or_selected_double(cpl_table *, const char *, 
                                     cpl_table_select_operator, double);
cpl_size cpl_table_and_selected_float_complex(cpl_table *, const char *, 
                                    cpl_table_select_operator, _Complex float);
cpl_size cpl_table_or_selected_float_complex(cpl_table *, const char *, 
                                    cpl_table_select_operator, _Complex float);
cpl_size cpl_table_and_selected_double_complex(cpl_table *, const char *, 
                                     cpl_table_select_operator, _Complex double);
cpl_size cpl_table_or_selected_double_complex(cpl_table *, const char *, 
                                     cpl_table_select_operator, _Complex double);

cpl_size cpl_table_and_selected_string(cpl_table *, const char *, 
                                     cpl_table_select_operator, const char *);
cpl_size cpl_table_or_selected_string(cpl_table *, const char *, 
                                     cpl_table_select_operator, const char *);

cpl_size cpl_table_and_selected(cpl_table *, const char *, 
                         cpl_table_select_operator, const char *);
cpl_size cpl_table_or_selected(cpl_table *, const char *, 
                        cpl_table_select_operator, const char *);
int cpl_table_is_selected(const cpl_table *, cpl_size);

cpl_size cpl_table_count_selected(const cpl_table *);

void cpl_table_dump_structure(const cpl_table *, FILE *);
void cpl_table_dump(const cpl_table *, cpl_size, cpl_size, FILE *);

cpl_table *cpl_table_load(const char *, int, int)
    CPL_ATTR_ALLOC;
cpl_table *cpl_table_load_window(const char *, int, int, const cpl_array *,
                                 cpl_size, cpl_size)
    CPL_ATTR_ALLOC;
cpl_error_code cpl_table_save(const cpl_table *, const cpl_propertylist *, 
                              const cpl_propertylist *, const char *filename, 
                              unsigned mode);

CPL_END_DECLS

#endif
/* end of cpl_table.h */

