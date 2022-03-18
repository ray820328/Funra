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

#ifndef CPL_COLUMN_H
#define CPL_COLUMN_H

#if ((defined(__STDC_IEC_559_COMPLEX__) || (__STDC_VERSION__ >= 199901L) || \
        (__GNUC__ >= 3)) && !defined(__cplusplus))
#include <complex.h>
#endif
#include <cpl_macros.h>
#include <cpl_type.h>
#include <cpl_error.h>
#include <cpl_array.h>

CPL_BEGIN_DECLS


/*
 *  This is the type used for any flag used in cpl_column.
 */

typedef int cpl_column_flag;

typedef struct _cpl_column_ cpl_column;

void cpl_column_dump_structure(cpl_column *column);
void cpl_column_dump(cpl_column *column, cpl_size start, cpl_size count);

/*
 * Constructors and destructors:
 */

cpl_column *cpl_column_new_int(cpl_size length)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_new_long(cpl_size length)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_new_long_long(cpl_size length)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_new_cplsize(cpl_size length)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_new_float(cpl_size length)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_new_double(cpl_size length)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_new_float_complex(cpl_size length)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_new_double_complex(cpl_size length)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_new_string(cpl_size length)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_new_array(cpl_type, cpl_size, cpl_size)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_new_complex_from_arrays(const cpl_column *,
                                               const cpl_column *)
    CPL_ATTR_ALLOC CPL_INTERNAL;

cpl_error_code cpl_column_set_save_type(cpl_column *, cpl_type);
cpl_type cpl_column_get_save_type(const cpl_column *);

cpl_column *cpl_column_wrap_int(int *, cpl_size length)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_wrap_long(long *, cpl_size length)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_wrap_long_long(long long *, cpl_size length)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_wrap_cplsize(cpl_size *, cpl_size length)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_wrap_float(float *, cpl_size length)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_wrap_double(double *, cpl_size length)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_wrap_float_complex(_Complex float *, cpl_size length)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_wrap_double_complex(_Complex double *, cpl_size length)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_wrap_string(char **, cpl_size length)
     CPL_ATTR_ALLOC;

void *cpl_column_unwrap(cpl_column *);
void cpl_column_delete_but_strings(cpl_column *);
void cpl_column_delete_but_arrays(cpl_column *);

cpl_error_code cpl_column_copy_data(cpl_column *, const double *);
cpl_error_code cpl_column_copy_data_int(cpl_column *, const int *);
cpl_error_code cpl_column_copy_data_long(cpl_column *, const long *);
cpl_error_code cpl_column_copy_data_long_long(cpl_column *, const long long *);
cpl_error_code cpl_column_copy_data_cplsize(cpl_column *, const cpl_size *);
cpl_error_code cpl_column_copy_data_float(cpl_column *, const float *);
cpl_error_code cpl_column_copy_data_double(cpl_column *, const double *);
cpl_error_code cpl_column_copy_data_complex(cpl_column *,
                                            const _Complex double *);
cpl_error_code cpl_column_copy_data_float_complex(cpl_column *,
                                                  const _Complex float *);
cpl_error_code cpl_column_copy_data_double_complex(cpl_column *,
                                                   const _Complex double *);
cpl_error_code cpl_column_copy_data_string(cpl_column *, const char **);

void cpl_column_delete(cpl_column *);


/*
 * Methods:
 */

cpl_error_code cpl_column_set_name(cpl_column *, const char *);
const char *cpl_column_get_name(const cpl_column *);

cpl_error_code cpl_column_set_unit(cpl_column *, const char *);
const char *cpl_column_get_unit(const cpl_column *);

cpl_error_code cpl_column_set_format(cpl_column *, const char *);
const char *cpl_column_get_format(const cpl_column *);

cpl_size cpl_column_get_size(const cpl_column *);
cpl_size cpl_column_get_depth(const cpl_column *);
cpl_size cpl_column_get_dimensions(const cpl_column *);
cpl_error_code cpl_column_set_dimensions(cpl_column *, const cpl_array *);
cpl_size cpl_column_get_dimension(const cpl_column *, cpl_size);

cpl_type cpl_column_get_type(const cpl_column *);

int *cpl_column_get_data_int(cpl_column *);
const int *cpl_column_get_data_int_const(const cpl_column *);
long *cpl_column_get_data_long(cpl_column *);
const long *cpl_column_get_data_long_const(const cpl_column *);
long long *cpl_column_get_data_long_long(cpl_column *);
const long long *cpl_column_get_data_long_long_const(const cpl_column *);
cpl_size *cpl_column_get_data_cplsize(cpl_column *);
const cpl_size *cpl_column_get_data_cplsize_const(const cpl_column *);
float *cpl_column_get_data_float(cpl_column *);
const float *cpl_column_get_data_float_const(const cpl_column *);
double *cpl_column_get_data_double(cpl_column *);
const double *cpl_column_get_data_double_const(const cpl_column *);
_Complex float *cpl_column_get_data_float_complex(cpl_column *);
const _Complex float *
cpl_column_get_data_float_complex_const(const cpl_column *);
_Complex double *cpl_column_get_data_double_complex(cpl_column *);
const _Complex double *
cpl_column_get_data_double_complex_const(const cpl_column *);
char **cpl_column_get_data_string(cpl_column *);
const char **cpl_column_get_data_string_const(const cpl_column *);
cpl_array **cpl_column_get_data_array(cpl_column *);
const cpl_array **cpl_column_get_data_array_const(const cpl_column *);
cpl_column_flag *cpl_column_get_data_invalid(cpl_column *);
const cpl_column_flag *cpl_column_get_data_invalid_const(const cpl_column *);
cpl_error_code cpl_column_set_data_invalid(cpl_column *, 
                                           cpl_column_flag *, cpl_size);

cpl_error_code cpl_column_set_size(cpl_column *, cpl_size);
cpl_error_code cpl_column_set_depth(cpl_column *, cpl_size);

double cpl_column_get(const cpl_column *, cpl_size, int *);
_Complex double cpl_column_get_complex(const cpl_column *, cpl_size , int *);
int cpl_column_get_int(const cpl_column *, cpl_size, int *); 
long cpl_column_get_long(const cpl_column *, cpl_size, int *);
long long cpl_column_get_long_long(const cpl_column *, cpl_size, int *);
cpl_size cpl_column_get_cplsize(const cpl_column *, cpl_size, int *);
float cpl_column_get_float(const cpl_column *, cpl_size, int *); 
double cpl_column_get_double(const cpl_column *, cpl_size, int *); 
_Complex float cpl_column_get_float_complex(const cpl_column *, cpl_size,
                                            int *);
_Complex double cpl_column_get_double_complex(const cpl_column *, cpl_size,
                                              int *);
char *cpl_column_get_string(cpl_column *, cpl_size);
const char *cpl_column_get_string_const(const cpl_column *, cpl_size);
cpl_array *cpl_column_get_array(cpl_column *, cpl_size);
const cpl_array *cpl_column_get_array_const(const cpl_column *, cpl_size);

cpl_error_code cpl_column_set(cpl_column *, cpl_size, double); 
cpl_error_code cpl_column_set_int(cpl_column *, cpl_size, int); 
cpl_error_code cpl_column_set_long(cpl_column *, cpl_size, long);
cpl_error_code cpl_column_set_long_long(cpl_column *, cpl_size, long long);
cpl_error_code cpl_column_set_cplsize(cpl_column *, cpl_size, cpl_size);
cpl_error_code cpl_column_set_float(cpl_column *, cpl_size, float); 
cpl_error_code cpl_column_set_double(cpl_column *, cpl_size, double); 
cpl_error_code cpl_column_set_complex(cpl_column *, cpl_size, _Complex double);
cpl_error_code cpl_column_set_float_complex(cpl_column *, cpl_size, _Complex float);
cpl_error_code cpl_column_set_double_complex(cpl_column *, cpl_size, _Complex double);
cpl_error_code cpl_column_set_string(cpl_column *, cpl_size, const char *);
cpl_error_code cpl_column_set_array(cpl_column *, cpl_size, const cpl_array *);

cpl_error_code cpl_column_fill(cpl_column *, cpl_size, cpl_size, double); 
cpl_error_code cpl_column_fill_int(cpl_column *, cpl_size, cpl_size, int); 
cpl_error_code cpl_column_fill_long(cpl_column *, cpl_size, cpl_size, long);
cpl_error_code cpl_column_fill_long_long(cpl_column *, cpl_size, cpl_size, long long);
cpl_error_code cpl_column_fill_cplsize(cpl_column *, cpl_size, cpl_size, cpl_size);
cpl_error_code cpl_column_fill_float(cpl_column *, cpl_size, cpl_size, float); 
cpl_error_code cpl_column_fill_double(cpl_column *, cpl_size, cpl_size, double);
cpl_error_code cpl_column_fill_complex(cpl_column *, cpl_size, cpl_size, _Complex double);
cpl_error_code cpl_column_fill_float_complex(cpl_column *, 
                                             cpl_size, cpl_size, _Complex float);
cpl_error_code cpl_column_fill_double_complex(cpl_column *, 
                                              cpl_size, cpl_size, _Complex double);
cpl_error_code cpl_column_fill_string(cpl_column *, cpl_size, cpl_size, const char *);
cpl_error_code cpl_column_fill_array(cpl_column *, cpl_size, cpl_size, const cpl_array *);

cpl_error_code cpl_column_copy_segment(cpl_column *, cpl_size, cpl_size, double *);
cpl_error_code cpl_column_copy_segment_int(cpl_column *, cpl_size, cpl_size, int *);
cpl_error_code cpl_column_copy_segment_long(cpl_column *, cpl_size, cpl_size, long *);
cpl_error_code cpl_column_copy_segment_long_long(cpl_column *, cpl_size,
                                                 cpl_size, long long *);
cpl_error_code cpl_column_copy_segment_cplsize(cpl_column *, cpl_size,
                                               cpl_size, cpl_size *);
cpl_error_code cpl_column_copy_segment_float(cpl_column *, cpl_size, cpl_size, float *);
cpl_error_code cpl_column_copy_segment_double(cpl_column *, cpl_size, cpl_size, double *);
cpl_error_code cpl_column_copy_segment_complex(cpl_column *, 
                                               cpl_size, cpl_size, _Complex double *);
cpl_error_code cpl_column_copy_segment_float_complex(cpl_column *, 
                                               cpl_size, cpl_size, _Complex float *);
cpl_error_code cpl_column_copy_segment_double_complex(cpl_column *, 
                                               cpl_size, cpl_size, _Complex double *);
cpl_error_code cpl_column_copy_segment_string(cpl_column *, cpl_size, cpl_size, char **);
cpl_error_code cpl_column_copy_segment_array(cpl_column *, cpl_size, cpl_size, 
                                             cpl_array **);

cpl_error_code cpl_column_shift(cpl_column *, cpl_size);

cpl_error_code cpl_column_set_invalid(cpl_column *, cpl_size);
cpl_error_code cpl_column_fill_invalid(cpl_column *, cpl_size, cpl_size);
int cpl_column_is_invalid(const cpl_column *, cpl_size);
int cpl_column_has_invalid(const cpl_column *);
int cpl_column_has_valid(const cpl_column *);
cpl_size cpl_column_count_invalid(cpl_column *);
cpl_size cpl_column_count_invalid_const(const cpl_column *) CPL_INTERNAL;

cpl_error_code cpl_column_fill_invalid_int(cpl_column *, int);
cpl_error_code cpl_column_fill_invalid_long(cpl_column *, long);
cpl_error_code cpl_column_fill_invalid_long_long(cpl_column *, long long);
cpl_error_code cpl_column_fill_invalid_cplsize(cpl_column *, cpl_size);
cpl_error_code cpl_column_fill_invalid_float(cpl_column *, float);
cpl_error_code cpl_column_fill_invalid_double(cpl_column *, double);
cpl_error_code cpl_column_fill_invalid_float_complex(cpl_column *, 
                                                     _Complex float);
cpl_error_code cpl_column_fill_invalid_double_complex(cpl_column *, 
                                                     _Complex double);

cpl_error_code cpl_column_erase_segment(cpl_column *, cpl_size, cpl_size);
cpl_error_code cpl_column_erase_pattern(cpl_column *, int *);
cpl_error_code cpl_column_insert_segment(cpl_column *, cpl_size, cpl_size);

cpl_column *cpl_column_duplicate(const cpl_column *)
     CPL_ATTR_ALLOC;

cpl_column *cpl_column_extract(cpl_column *, cpl_size, cpl_size)
     CPL_ATTR_ALLOC;

cpl_column *cpl_column_cast_to_int(const cpl_column *)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_cast_to_long(const cpl_column *)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_cast_to_long_long(const cpl_column *)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_cast_to_cplsize(const cpl_column *)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_cast_to_float(const cpl_column *)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_cast_to_double(const cpl_column *)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_cast_to_float_complex(const cpl_column *)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_cast_to_double_complex(const cpl_column *)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_cast_to_int_array(const cpl_column *)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_cast_to_long_array(const cpl_column *)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_cast_to_long_long_array(const cpl_column *)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_cast_to_cplsize_array(const cpl_column *)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_cast_to_float_array(const cpl_column *)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_cast_to_double_array(const cpl_column *)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_cast_to_float_complex_array(const cpl_column *)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_cast_to_double_complex_array(const cpl_column *)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_cast_to_int_flat(const cpl_column *)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_cast_to_long_flat(const cpl_column *)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_cast_to_long_long_flat(const cpl_column *)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_cast_to_cplsize_flat(const cpl_column *)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_cast_to_float_flat(const cpl_column *)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_cast_to_double_flat(const cpl_column *)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_cast_to_float_complex_flat(const cpl_column *)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_cast_to_double_complex_flat(const cpl_column *)
     CPL_ATTR_ALLOC;

cpl_error_code cpl_column_merge(cpl_column *, const cpl_column *, cpl_size);

cpl_error_code cpl_column_add(cpl_column *, cpl_column *);
cpl_error_code cpl_column_subtract(cpl_column *, cpl_column *);
cpl_error_code cpl_column_multiply(cpl_column *, cpl_column *);
cpl_error_code cpl_column_divide(cpl_column *, cpl_column *);

cpl_error_code cpl_column_add_scalar(cpl_column *, double);
cpl_error_code cpl_column_add_scalar_complex(cpl_column *, _Complex double);
cpl_error_code cpl_column_subtract_scalar(cpl_column *, double);
cpl_error_code cpl_column_subtract_scalar_complex(cpl_column *, _Complex double);
cpl_error_code cpl_column_multiply_scalar(cpl_column *, double);
cpl_error_code cpl_column_multiply_scalar_complex(cpl_column *, _Complex double);
cpl_error_code cpl_column_divide_scalar(cpl_column *, double);
cpl_error_code cpl_column_divide_scalar_complex(cpl_column *, _Complex double);
cpl_error_code cpl_column_absolute(cpl_column *);
cpl_column *cpl_column_absolute_complex(cpl_column *)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_phase_complex(cpl_column *)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_extract_real(const cpl_column *)
     CPL_ATTR_ALLOC;
cpl_column *cpl_column_extract_imag(const cpl_column *)
     CPL_ATTR_ALLOC;
cpl_error_code cpl_column_logarithm(cpl_column *, double);
cpl_error_code cpl_column_power(cpl_column *, double);
cpl_error_code cpl_column_power_complex(cpl_column *,
                                        double complex);
cpl_error_code cpl_column_exponential(cpl_column *, double);
cpl_error_code cpl_column_conjugate(cpl_column *);

double cpl_column_get_mean(const cpl_column *);
_Complex double cpl_column_get_mean_complex(const cpl_column *);
double cpl_column_get_median(const cpl_column *);
double cpl_column_get_stdev(const cpl_column *);

double cpl_column_get_max(const cpl_column *);
double cpl_column_get_min(const cpl_column *);
cpl_error_code cpl_column_get_maxpos(const cpl_column *, cpl_size *);
cpl_error_code cpl_column_get_minpos(const cpl_column *, cpl_size *);

void cpl_column_unset_null_all(cpl_column *) CPL_ATTR_NONNULL CPL_INTERNAL;

CPL_END_DECLS

#endif
/* end of cpl_column.h */
