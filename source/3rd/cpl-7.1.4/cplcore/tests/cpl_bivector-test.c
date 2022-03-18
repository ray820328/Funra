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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

#include "cpl_bivector.h"
#include "cpl_test.h"
#include "cpl_tools.h"
#include "cpl_memory.h"
#include "cpl_math_const.h"

#ifndef FUNCTION_SIZE
#define FUNCTION_SIZE  1024
#endif

#define LIN_SIZE 3
#define POL_SIZE 3

static void cpl_bivector_sort_test(void);
static void cpl_bivector_sort_random(int);

static cpl_error_code cpl_bivector_sort_ok(cpl_bivector *,
                                           const cpl_bivector *,
                                           cpl_sort_direction,
                                           cpl_sort_mode, int);

/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/
int main(void)
{
    cpl_bivector    *   sinus;
    cpl_bivector    *   cosinus;
    cpl_bivector    *   tmp_fun;
    cpl_bivector    *   source;
    double          *   data_x,
                    *   data_y;
    const double    *   dnull;
    const cpl_vector*   vnull;
    cpl_vector      *   vec1;
    cpl_vector      *   vec2;
    cpl_vector      *   vec3;
    const double        scale = CPL_MATH_2PI / FUNCTION_SIZE;
    FILE            *   f_out;
    const char      *   filename = "cpl_bivector_dump.txt";
    cpl_error_code      error;
    const double        linear[LIN_SIZE] = {2.0, 3.0, 4.0};
    const double        interpol[POL_SIZE] = {CPL_MATH_LN10, CPL_MATH_E,
                                              CPL_MATH_PI};
    int                 i;
    

    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    cpl_bivector_sort_test();
    cpl_bivector_sort_random(1);
    cpl_bivector_sort_random(21);
    cpl_bivector_sort_random(41);
    cpl_bivector_sort_random(100);
    cpl_bivector_sort_random(1001);

    /* Test 1: NULL input to accessors */

    i = cpl_bivector_get_size(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_lt(i, 0);

    dnull = cpl_bivector_get_x_data(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(dnull);
    dnull = cpl_bivector_get_x_data_const(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(dnull);
    dnull = cpl_bivector_get_y_data(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(dnull);
    dnull = cpl_bivector_get_y_data_const(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(dnull);


    vnull = cpl_bivector_get_x(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(vnull);
    vnull = cpl_bivector_get_x_const(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(vnull);
    vnull = cpl_bivector_get_y(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(vnull);
    vnull = cpl_bivector_get_y_const(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(vnull);

    /* Create the first 1d function : sinus function */
    cpl_test_nonnull( sinus = cpl_bivector_new(FUNCTION_SIZE) );

    /* Fill the sinus function */
    data_x = cpl_bivector_get_x_data(sinus);
    data_y = cpl_bivector_get_y_data(sinus);
    cpl_test_eq_ptr(cpl_bivector_get_x_data_const(sinus), data_x);
    cpl_test_eq_ptr(cpl_bivector_get_y_data_const(sinus), data_y);

    for (i = 0; i < FUNCTION_SIZE; i++) {
        data_x[i] = i * scale;
        data_y[i] = sin(data_x[i]);
    }
    
    /* Create the second 1d function : cosinus function */
    cpl_test_nonnull (cosinus = cpl_bivector_new(FUNCTION_SIZE));

    /* Fill the cosinus function */
    data_x = cpl_bivector_get_x_data(cosinus);
    data_y = cpl_bivector_get_y_data(cosinus);
    for (i = 0; i < FUNCTION_SIZE; i++) {
        data_x[i] = i * scale - CPL_MATH_PI;
        data_y[i] = cos(data_x[i]);
    }
  
    /* Test cpl_bivector_get_y() */
    cpl_test_nonnull( cpl_bivector_get_y(sinus) );
    cpl_test_nonnull( cpl_bivector_get_x(sinus) );
    cpl_test_noneq_ptr( cpl_bivector_get_x(sinus), cpl_bivector_get_y(sinus) );

    /* Test cpl_bivector_get_size() */
    cpl_test_eq( cpl_bivector_get_size(sinus), FUNCTION_SIZE );
   
    /* Test cpl_bivector_dump() */
    f_out = fopen(filename, "w");
    cpl_test_nonnull( f_out );

    /* Will not print any values */
    cpl_bivector_dump(NULL, f_out);

    cpl_bivector_dump(sinus, f_out);
    cpl_test_zero( fclose(f_out) );

    /* Test cpl_bivector_read() */

    tmp_fun = cpl_bivector_read(NULL);
    cpl_test_error( CPL_ERROR_NULL_INPUT );
    cpl_test_null( tmp_fun );

    tmp_fun = cpl_bivector_read("/dev/null");
    cpl_test_error(CPL_ERROR_BAD_FILE_FORMAT);
    cpl_test_null( tmp_fun );

    cpl_test_nonnull( tmp_fun = cpl_bivector_duplicate(sinus) );
    cpl_bivector_delete(sinus);

    cpl_test_nonnull( sinus = cpl_bivector_read(filename) );

    cpl_test_zero(remove(filename));

    /* Measure the error incurred through the dump/read */
    cpl_test_vector_abs(cpl_bivector_get_x(tmp_fun),
                        cpl_bivector_get_x(sinus), 48.0 * FLT_EPSILON);

    cpl_test_vector_abs(cpl_bivector_get_y(tmp_fun),
                        cpl_bivector_get_y(sinus), 6.0 * FLT_EPSILON);

    cpl_bivector_delete(tmp_fun);
    
    /* Test cpl_bivector_interpolate_linear */
    tmp_fun = cpl_bivector_new(FUNCTION_SIZE);
    vec1 = cpl_bivector_get_x(tmp_fun);
    vec2 = cpl_bivector_get_x(sinus);

    error = cpl_vector_copy(vec1, vec2);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_vector_add_scalar(vec1, 0.5);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    /* Verify error handling of lower extrapolation */
    error = cpl_bivector_interpolate_linear(tmp_fun, sinus);
    cpl_test_eq_error(error, CPL_ERROR_DATA_NOT_FOUND);

    error = cpl_vector_subtract_scalar(vec1, 1.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    /* Verify error handling of upper extrapolation */
    error = cpl_bivector_interpolate_linear(tmp_fun, sinus);
    cpl_test_eq_error(error, CPL_ERROR_DATA_NOT_FOUND);

    /* Verify "interpolation" with values that an all-constant vector */
    error = cpl_vector_fill(cpl_bivector_get_x(sinus), 0.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_vector_fill(cpl_bivector_get_y(sinus), 0.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_vector_fill(cpl_bivector_get_x(tmp_fun), 0.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_bivector_interpolate_linear(tmp_fun, sinus);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_vector_abs(cpl_bivector_get_y(tmp_fun), cpl_bivector_get_y(sinus),
                        0.0);

    /* Free */
    cpl_bivector_delete(tmp_fun);
    cpl_bivector_delete(sinus);
    cpl_bivector_delete(cosinus);

    sinus = cpl_bivector_new(FUNCTION_SIZE+1);

    /* Create a function with sine values, with abscissa offset by 0.5 */
    data_x = cpl_bivector_get_x_data(sinus);
    data_y = cpl_bivector_get_y_data(sinus);
    for (i = 0; i < FUNCTION_SIZE + 1; i++) {
        data_x[i] = (i-0.5) * scale;
        data_y[i] = sin(data_x[i]);
    }
    
    tmp_fun = cpl_bivector_new(FUNCTION_SIZE);
    vec1 = cpl_bivector_get_x(tmp_fun);
    data_x = cpl_vector_get_data(vec1);
    for (i = 0; i < FUNCTION_SIZE; i++) data_x[i] = i * scale;

    error = cpl_bivector_interpolate_linear(tmp_fun, sinus);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    data_y = cpl_bivector_get_y_data(tmp_fun);
    cpl_test_nonnull(data_y);

    /* Check interpolation error at 0, pi/2, pi, 3*pi/2 */
    cpl_test_abs( data_y[0], 0.0, FLT_EPSILON);

    cpl_test_abs( data_y[FUNCTION_SIZE/2], 0.0, 10 * DBL_EPSILON);

    cpl_test_abs(  1.0, data_y[FUNCTION_SIZE/4],   1.0 / FUNCTION_SIZE );
    cpl_test_abs( -1.0, data_y[3*FUNCTION_SIZE/4], 1.0 / FUNCTION_SIZE );

    cpl_bivector_delete(tmp_fun);

    tmp_fun = cpl_bivector_duplicate(sinus);

    error = cpl_vector_copy(cpl_bivector_get_x(tmp_fun), cpl_bivector_get_x(sinus));
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_vector_fill(cpl_bivector_get_y(tmp_fun), 0.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_bivector_interpolate_linear(tmp_fun, sinus);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_vector_abs(cpl_bivector_get_y(tmp_fun), cpl_bivector_get_y(sinus),
                        10.0 * DBL_EPSILON);

    cpl_bivector_delete(tmp_fun);
    cpl_bivector_delete(sinus);

    vec1 = cpl_vector_wrap(LIN_SIZE, (double*)linear);
    source = cpl_bivector_wrap_vectors(vec1, vec1);

    vec2 = cpl_vector_wrap(POL_SIZE, (double*)interpol);
    vec3 = cpl_vector_new(POL_SIZE);
    tmp_fun = cpl_bivector_wrap_vectors(vec2, vec3);

    error = cpl_bivector_interpolate_linear(tmp_fun, source);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_vector_abs(vec2, vec3, DBL_EPSILON);

    cpl_bivector_unwrap_vectors(source);
    cpl_bivector_unwrap_vectors(tmp_fun);
    (void)cpl_vector_unwrap(vec1);
    (void)cpl_vector_unwrap(vec2);
    cpl_vector_delete(vec3);

    /* End of actual test code */
    return cpl_test_end(0);
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Call the function and validate its output
  @param  self   cpl_bivector to hold sorted result
  @param  other  Input cpl_bivector to sort
  @param  dir    CPL_SORT_ASCENDING or CPL_SORT_DESCENDING
  @param  mode   CPL_SORT_BY_X or CPL_SORT_BY_Y
  @param  line   __LINE__
  @return CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error
  @see cpl_bivector_sort
 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_bivector_sort_ok(cpl_bivector * self,
                                           const cpl_bivector * other,
                                           cpl_sort_direction dir,
                                           cpl_sort_mode mode, int line)
{
    const cpl_error_code error = cpl_bivector_sort(self, other, dir, mode);

    if (error != CPL_ERROR_NONE) {
        cpl_msg_error(cpl_func, "Failure from line %u", line);
    } else {
        cpl_vector * tosort  =
            cpl_vector_duplicate((mode == CPL_SORT_BY_X
                                  ? cpl_bivector_get_x_const
                                  : cpl_bivector_get_y_const)(other));
        const cpl_vector * sorted = (mode == CPL_SORT_BY_X
                                      ? cpl_bivector_get_x_const
                                      : cpl_bivector_get_y_const)(self);
        const double * data = cpl_vector_get_data_const(sorted);
        const int n  = cpl_bivector_get_size(self);
        const int ia = dir == CPL_SORT_ASCENDING ? 1 : 0;
        const int ib = dir == CPL_SORT_ASCENDING ? 0 : 1;
        int i;

        for (i = 1; i < n; i++) {
            cpl_test_leq(data[i - ia], data[i - ib]);
        }

        cpl_test_zero(cpl_vector_sort(tosort, dir));
        cpl_test_vector_abs(sorted, tosort, 0.0);
        cpl_vector_delete(tosort);
    }

    return error;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Test the CPL function
  @return   void

 */
/*----------------------------------------------------------------------------*/
static void cpl_bivector_sort_test(void)
{

    /* In an ascending stable sort, both arrays will have same permutations */
    const double   xval[]  = {8.0, 1.0, 2.0, 3.0, 4.0, 4.0, 6.0, 7.0, 0.0};
    const double   yval[]  = {8.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 0.0};
    const int      nvals   = (int)(sizeof(xval)/sizeof(double));
    cpl_vector   * tosortx = cpl_vector_wrap(nvals, (double*)xval);
    cpl_vector   * tosorty = cpl_vector_wrap(nvals, (double*)yval);
    cpl_bivector * tosort  = cpl_bivector_wrap_vectors(tosortx, tosorty);
    cpl_bivector * sorted  = cpl_bivector_new(nvals);
    cpl_bivector * sorted2 = cpl_bivector_new(nvals);
    cpl_bivector * toolong = cpl_bivector_new(nvals + 1);
    cpl_error_code error;

    /* 1: Check error handling */
    error = cpl_bivector_sort(NULL, tosort, CPL_SORT_ASCENDING, CPL_SORT_BY_X);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

    error = cpl_bivector_sort(sorted, NULL, CPL_SORT_ASCENDING, CPL_SORT_BY_X);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

    error = cpl_bivector_sort(sorted, tosort, CPL_SORT_ASCENDING, 2);
    cpl_test_eq_error(error, CPL_ERROR_UNSUPPORTED_MODE);

    error = cpl_bivector_sort(sorted, tosort, 2, CPL_SORT_BY_X);
    cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

    error = cpl_bivector_sort(toolong, tosort, CPL_SORT_ASCENDING, CPL_SORT_BY_X);
    cpl_test_eq_error(error, CPL_ERROR_INCOMPATIBLE_INPUT);

    /* 2: Sorting by X or Y doesn't matter with this data */
    error = cpl_bivector_sort_ok(sorted, tosort, CPL_SORT_ASCENDING,
                                 CPL_SORT_BY_X, __LINE__);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_bivector_sort_ok(sorted2, tosort, CPL_SORT_ASCENDING,
                                 CPL_SORT_BY_Y, __LINE__);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_vector_abs(cpl_bivector_get_x(sorted), cpl_bivector_get_x(sorted2),
                        0.0);
    cpl_test_vector_abs(cpl_bivector_get_y(sorted), cpl_bivector_get_y(sorted2),
                        0.0);

    /* 2: Sorting already sorted data doesn't matter */
    error = cpl_bivector_sort_ok(sorted2, sorted, CPL_SORT_ASCENDING,
                                 CPL_SORT_BY_X, __LINE__);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_vector_abs(cpl_bivector_get_x(sorted), cpl_bivector_get_x(sorted2),
                        0.0);
    cpl_test_vector_abs(cpl_bivector_get_y(sorted), cpl_bivector_get_y(sorted2),
                        0.0);
    /* 2: Ascending sort + descending sort + ascending sort == ascending sort */
    /* - for unique elements */
    error = cpl_bivector_sort_ok(sorted, tosort, CPL_SORT_ASCENDING,
                                 CPL_SORT_BY_X, __LINE__);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_bivector_sort_ok(sorted2, sorted, CPL_SORT_DESCENDING,
                                 CPL_SORT_BY_X, __LINE__);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_bivector_sort_ok(sorted, sorted2, CPL_SORT_ASCENDING,
                                 CPL_SORT_BY_X, __LINE__);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_bivector_sort_ok(sorted2, tosort, CPL_SORT_ASCENDING,
                                 CPL_SORT_BY_X, __LINE__);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_vector_abs(cpl_bivector_get_x(sorted), cpl_bivector_get_x(sorted2),
                        0.0);
    cpl_test_vector_abs(cpl_bivector_get_y(sorted), cpl_bivector_get_y(sorted2),
                        0.0);

    /* 2: Ascending sort + descending sort + ascending sort == ascending sort */
    /* - and for non-unique elements */
    error = cpl_bivector_sort_ok(sorted, tosort, CPL_SORT_ASCENDING,
                                 CPL_SORT_BY_Y, __LINE__);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_bivector_sort_ok(sorted2, sorted, CPL_SORT_DESCENDING,
                                 CPL_SORT_BY_Y, __LINE__);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_bivector_sort_ok(sorted, sorted2, CPL_SORT_ASCENDING,
                                 CPL_SORT_BY_Y, __LINE__);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_bivector_sort_ok(sorted2, tosort, CPL_SORT_ASCENDING,
                                 CPL_SORT_BY_Y, __LINE__);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_vector_abs(cpl_bivector_get_x(sorted), cpl_bivector_get_x(sorted2),
                        0.0);
    cpl_test_vector_abs(cpl_bivector_get_y(sorted), cpl_bivector_get_y(sorted2),
                        0.0);

    /* 3: In-place */
    error = cpl_vector_copy(cpl_bivector_get_x(sorted), tosortx);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_vector_copy(cpl_bivector_get_y(sorted), tosorty);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_bivector_sort_ok(sorted, sorted, CPL_SORT_ASCENDING,
                                 CPL_SORT_BY_X, __LINE__);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_vector_copy(cpl_bivector_get_x(sorted), tosortx);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_vector_copy(cpl_bivector_get_y(sorted), tosorty);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_bivector_sort_ok(sorted, sorted, CPL_SORT_ASCENDING,
                                 CPL_SORT_BY_Y, __LINE__);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_bivector_unwrap_vectors(tosort);
    cpl_test_eq_ptr(cpl_vector_unwrap(tosortx), xval);
    cpl_test_eq_ptr(cpl_vector_unwrap(tosorty), yval);
    cpl_bivector_delete(sorted);
    cpl_bivector_delete(sorted2);
    cpl_bivector_delete(toolong);

    return;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Test the CPL function using random numbers
  @param  size The number of elements to test with
  @return void

 */
/*----------------------------------------------------------------------------*/
static void cpl_bivector_sort_random(int size) {

    cpl_bivector * source = cpl_bivector_new(size);
    cpl_bivector * destination = cpl_bivector_new(size);
    double * xdata = cpl_bivector_get_x_data(source);
    cpl_image * ximage = cpl_image_wrap_double(size, 1, xdata);
    double * ydata = cpl_bivector_get_x_data(source);
    cpl_image * yimage = cpl_image_wrap_double(size, 1, ydata);
    cpl_error_code error;
    int inplace;

    cpl_test_leq(1, size);

    for (inplace = 0; inplace < 2; inplace++) {
        cpl_bivector * tosort = source;
        cpl_bivector * sorted = destination;

        error = cpl_image_fill_noise_uniform(ximage, -1.0, 1.0);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_vector_fill(cpl_bivector_get_y(source), 0.0);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        if (inplace) {
            tosort = sorted;
            error = cpl_bivector_copy(tosort, source);
            cpl_test_eq_error(error, CPL_ERROR_NONE);
        }

        /* Sort up and down on X */
        error = cpl_bivector_sort_ok(sorted, tosort, CPL_SORT_ASCENDING,
                                     CPL_SORT_BY_X, __LINE__);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_bivector_sort_ok(sorted, tosort, CPL_SORT_DESCENDING,
                                     CPL_SORT_BY_X, __LINE__);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        /* Same, with all values identical */
        error = cpl_bivector_sort_ok(sorted, tosort, CPL_SORT_ASCENDING,
                                     CPL_SORT_BY_Y, __LINE__);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_bivector_sort_ok(sorted, tosort, CPL_SORT_DESCENDING,
                                     CPL_SORT_BY_Y, __LINE__);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        /* Sort up and down on Y */
        error = cpl_image_fill_noise_uniform(yimage, -1.0, 1.0);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        if (inplace) {
            error = cpl_bivector_copy(tosort, source);
            cpl_test_eq_error(error, CPL_ERROR_NONE);
        }

        error = cpl_bivector_sort_ok(sorted, tosort, CPL_SORT_ASCENDING,
                                     CPL_SORT_BY_Y, __LINE__);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_bivector_sort_ok(sorted, tosort, CPL_SORT_DESCENDING,
                                     CPL_SORT_BY_Y, __LINE__);
        cpl_test_eq_error(error, CPL_ERROR_NONE);
    }

    cpl_test_eq_ptr(cpl_image_unwrap(ximage), xdata);
    cpl_test_eq_ptr(cpl_image_unwrap(yimage), ydata);
    cpl_bivector_delete(source);
    cpl_bivector_delete(destination);
}
