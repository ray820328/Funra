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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifndef CPL_ARRAY_H
#define CPL_ARRAY_H

#include <stdio.h>
#if ((defined(__STDC_IEC_559_COMPLEX__) || (__STDC_VERSION__ >= 199901L) || \
        (__GNUC__ >= 3)) && !defined(__cplusplus))
#include <complex.h>
#endif
#include <cpl_macros.h>
#include <cpl_type.h>
#include <cpl_error.h>

CPL_BEGIN_DECLS


typedef struct _cpl_array_ cpl_array;

cpl_array *cpl_array_new(cpl_size, cpl_type)
     CPL_ATTR_ALLOC;
cpl_array *cpl_array_wrap_int(int *, cpl_size)
     CPL_ATTR_ALLOC;
cpl_array *cpl_array_wrap_long(long *, cpl_size)
     CPL_ATTR_ALLOC;
cpl_array *cpl_array_wrap_long_long(long long *, cpl_size)
     CPL_ATTR_ALLOC;
cpl_array *cpl_array_wrap_cplsize(cpl_size *, cpl_size)
     CPL_ATTR_ALLOC;
cpl_array *cpl_array_wrap_float(float *, cpl_size)
     CPL_ATTR_ALLOC;
cpl_array *cpl_array_wrap_double(double *, cpl_size)
     CPL_ATTR_ALLOC;
cpl_array *cpl_array_wrap_float_complex(_Complex float *, cpl_size)
     CPL_ATTR_ALLOC;
cpl_array *cpl_array_wrap_double_complex(_Complex double *, cpl_size)
     CPL_ATTR_ALLOC;
cpl_array *cpl_array_wrap_string(char **, cpl_size)
     CPL_ATTR_ALLOC;
cpl_array *cpl_array_new_complex_from_arrays(const cpl_array *,
                                             const cpl_array *)
    CPL_ATTR_ALLOC;

cpl_error_code cpl_array_copy_data(cpl_array *, const double *);
cpl_error_code cpl_array_copy_data_int(cpl_array *, const int *);
cpl_error_code cpl_array_copy_data_long(cpl_array *, const long *);
cpl_error_code cpl_array_copy_data_long_long(cpl_array *, const long long *);
cpl_error_code cpl_array_copy_data_cplsize(cpl_array *, const cpl_size *);
cpl_error_code cpl_array_copy_data_float(cpl_array *, const float *);
cpl_error_code cpl_array_copy_data_double(cpl_array *, const double *);
cpl_error_code cpl_array_copy_data_string(cpl_array *, const char **);
cpl_error_code cpl_array_copy_data_complex(cpl_array *, const _Complex double *);
cpl_error_code cpl_array_copy_data_float_complex(cpl_array *,
                                                 const _Complex float *);
cpl_error_code cpl_array_copy_data_double_complex(cpl_array *,
                                                  const _Complex double *);
void cpl_array_delete(cpl_array *);
void *cpl_array_unwrap(cpl_array *);
cpl_size cpl_array_get_size(const cpl_array *);
cpl_error_code cpl_array_set_size(cpl_array *, cpl_size);
cpl_type cpl_array_get_type(const cpl_array *);
int cpl_array_has_invalid(const cpl_array *);
int cpl_array_has_valid(const cpl_array *);
int cpl_array_is_valid(const cpl_array *, cpl_size);
cpl_size cpl_array_count_invalid(const cpl_array *);
int *cpl_array_get_data_int(cpl_array *);
const int *cpl_array_get_data_int_const(const cpl_array *);
long *cpl_array_get_data_long(cpl_array *);
const long *cpl_array_get_data_long_const(const cpl_array *);
long long *cpl_array_get_data_long_long(cpl_array *);
const long long *cpl_array_get_data_long_long_const(const cpl_array *);
cpl_size *cpl_array_get_data_cplsize(cpl_array *);
const cpl_size *cpl_array_get_data_cplsize_const(const cpl_array *);
float *cpl_array_get_data_float(cpl_array *);
const float *cpl_array_get_data_float_const(const cpl_array *);
double *cpl_array_get_data_double(cpl_array *);
const double *cpl_array_get_data_double_const(const cpl_array *);
_Complex float *cpl_array_get_data_float_complex(cpl_array *);
const _Complex float *cpl_array_get_data_float_complex_const(const cpl_array *);
const _Complex double *cpl_array_get_data_double_complex_const
                                                      (const cpl_array *);
_Complex double *cpl_array_get_data_double_complex(cpl_array *);
char **cpl_array_get_data_string(cpl_array *);
const char **cpl_array_get_data_string_const(const cpl_array *);
double cpl_array_get(const cpl_array *, cpl_size, int *);
int cpl_array_get_int(const cpl_array *, cpl_size, int *);
long cpl_array_get_long(const cpl_array *, cpl_size, int *);
long long cpl_array_get_long_long(const cpl_array *, cpl_size, int *);
cpl_size cpl_array_get_cplsize(const cpl_array *, cpl_size, int *);
float cpl_array_get_float(const cpl_array *, cpl_size, int *);
double cpl_array_get_double(const cpl_array *, cpl_size, int *);
const char *cpl_array_get_string(const cpl_array *, cpl_size);
_Complex double cpl_array_get_complex(const cpl_array *, cpl_size, int *);
_Complex float cpl_array_get_float_complex(const cpl_array *, cpl_size, int *);
_Complex double cpl_array_get_double_complex(const cpl_array *, cpl_size, int *);
cpl_error_code cpl_array_set(cpl_array *array, cpl_size, double);
cpl_error_code cpl_array_set_int(cpl_array *, cpl_size, int);
cpl_error_code cpl_array_set_long(cpl_array *, cpl_size, long);
cpl_error_code cpl_array_set_long_long(cpl_array *, cpl_size, long long);
cpl_error_code cpl_array_set_cplsize(cpl_array *, cpl_size, cpl_size);
cpl_error_code cpl_array_set_float(cpl_array *, cpl_size, float);
cpl_error_code cpl_array_set_double(cpl_array *, cpl_size, double);
cpl_error_code cpl_array_set_string(cpl_array *, cpl_size, const char *);
cpl_error_code cpl_array_set_invalid(cpl_array *, cpl_size);
cpl_error_code cpl_array_set_complex(cpl_array *, cpl_size, _Complex double);
cpl_error_code cpl_array_set_float_complex(cpl_array *, cpl_size, _Complex float);
cpl_error_code cpl_array_set_double_complex(cpl_array *, cpl_size, _Complex double);

cpl_error_code cpl_array_fill_window(cpl_array *, cpl_size, cpl_size, double);
cpl_error_code cpl_array_fill_window_int(cpl_array *, cpl_size, cpl_size, int);
cpl_error_code cpl_array_fill_window_long(cpl_array *, cpl_size, cpl_size, long);
cpl_error_code cpl_array_fill_window_long_long(cpl_array *, cpl_size, cpl_size,
                                               long long);
cpl_error_code cpl_array_fill_window_cplsize(cpl_array *, cpl_size, cpl_size,
                                             cpl_size);
cpl_error_code cpl_array_fill_window_float(cpl_array *, cpl_size, cpl_size, float);
cpl_error_code cpl_array_fill_window_double(cpl_array *, cpl_size, cpl_size, double);
cpl_error_code cpl_array_fill_window_string(cpl_array *, cpl_size, cpl_size,
                                            const char *);
cpl_error_code cpl_array_fill_window_complex(cpl_array *, 
                                             cpl_size, cpl_size, _Complex double);
cpl_error_code cpl_array_fill_window_float_complex(cpl_array *, cpl_size, cpl_size,
                                                   _Complex float);
cpl_error_code cpl_array_fill_window_double_complex(cpl_array *, cpl_size, cpl_size,
                                                    _Complex double);
cpl_error_code cpl_array_fill_window_invalid(cpl_array *, cpl_size, cpl_size);
cpl_array *cpl_array_duplicate(const cpl_array *)
     CPL_ATTR_ALLOC;
cpl_array *cpl_array_extract(const cpl_array *, cpl_size start, cpl_size count)
     CPL_ATTR_ALLOC;
cpl_array *cpl_array_extract_real(const cpl_array *)
     CPL_ATTR_ALLOC;
cpl_array *cpl_array_extract_imag(const cpl_array *)
     CPL_ATTR_ALLOC;
cpl_error_code cpl_array_insert_window(cpl_array *, cpl_size start, cpl_size count);
cpl_error_code cpl_array_erase_window(cpl_array *, cpl_size start, cpl_size count);
cpl_error_code cpl_array_insert(cpl_array *, const cpl_array *, cpl_size);

cpl_error_code cpl_array_add(cpl_array *, const cpl_array *);
cpl_error_code cpl_array_subtract(cpl_array *, const cpl_array *);
cpl_error_code cpl_array_multiply(cpl_array *, const cpl_array *);
cpl_error_code cpl_array_divide(cpl_array *, const cpl_array *);
cpl_error_code cpl_array_add_scalar(cpl_array *, double);
cpl_error_code cpl_array_subtract_scalar(cpl_array *, double);
cpl_error_code cpl_array_multiply_scalar(cpl_array *, double);
cpl_error_code cpl_array_divide_scalar(cpl_array *, double);
cpl_error_code cpl_array_abs(cpl_array *);
cpl_error_code cpl_array_arg(cpl_array *);
cpl_error_code cpl_array_logarithm(cpl_array *, double);
cpl_error_code cpl_array_power(cpl_array *, double);
cpl_error_code cpl_array_exponential(cpl_array *, double);
cpl_error_code cpl_array_power_complex(cpl_array *, _Complex double);
cpl_error_code cpl_array_add_scalar_complex(cpl_array *, _Complex double);
cpl_error_code cpl_array_subtract_scalar_complex(cpl_array *, _Complex double);
cpl_error_code cpl_array_multiply_scalar_complex(cpl_array *, _Complex double);
cpl_error_code cpl_array_divide_scalar_complex(cpl_array *, _Complex double);

_Complex double cpl_array_get_mean_complex(const cpl_array *);
double cpl_array_get_mean(const cpl_array *);
double cpl_array_get_median(const cpl_array *);
double cpl_array_get_stdev(const cpl_array *);

double cpl_array_get_max(const cpl_array *);
double cpl_array_get_min(const cpl_array *);
cpl_error_code cpl_array_get_maxpos(const cpl_array *, cpl_size *);
cpl_error_code cpl_array_get_minpos(const cpl_array *, cpl_size *);
void cpl_array_dump_structure(const cpl_array *, FILE *);
void cpl_array_dump(const cpl_array *, cpl_size start, cpl_size count, FILE *);
cpl_array *cpl_array_cast(const cpl_array *, cpl_type)
     CPL_ATTR_ALLOC;

CPL_END_DECLS

#endif
/* end of cpl_array.h */

