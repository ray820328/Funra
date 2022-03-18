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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include "cpl_polynomial_impl.h"
#include "cpl_error_impl.h"
#include "cpl_test.h"
#include "cpl_tools.h"
#include "cpl_memory.h"
#include "cpl_math_const.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>

/*-----------------------------------------------------------------------------
                                   Defines
 -----------------------------------------------------------------------------*/

#define     POLY_COEF_NB    5
#define     VECTOR_SIZE 1024
#define     POLY_SIZE 24
#define     POLY_DIM 11
#define     BINOMIAL_SZ 8

/*-----------------------------------------------------------------------------
                                  Pricate functions
 -----------------------------------------------------------------------------*/

static void cpl_polynomial_eval_test_2d_one(void);

static void cpl_polynomial_test_delete(cpl_polynomial *);

static void cpl_polynomial_derivative_test_empty(cpl_polynomial *)
    CPL_ATTR_NONNULL;

static void cpl_polynomial_basic_test(FILE *)
    CPL_ATTR_NONNULL;

static void cpl_polynomial_shift_test_1d(FILE *)
    CPL_ATTR_NONNULL;

static void cpl_polynomial_shift_test_2d(FILE *, cpl_size)
    CPL_ATTR_NONNULL;

static void cpl_polynomial_shift_test_3d_one(FILE *)
    CPL_ATTR_NONNULL;
static void cpl_polynomial_shift_test_2d_one(FILE *)
    CPL_ATTR_NONNULL;

static void cpl_polynomial_extract_test_2d(FILE *, cpl_size)
    CPL_ATTR_NONNULL;

static void cpl_polynomial_extract_test_2d_one(FILE *)
    CPL_ATTR_NONNULL;
static void cpl_polynomial_extract_test_3d_one(FILE *)
    CPL_ATTR_NONNULL;

static void cpl_polynomial_test_1(FILE *)
    CPL_ATTR_NONNULL;

static void cpl_polynomial_test_2(FILE *)
    CPL_ATTR_NONNULL;

static void cpl_polynomial_multiply_scalar_test_one(FILE *)
    CPL_ATTR_NONNULL;

static void cpl_polynomial_test_3(void);
static void cpl_polynomial_test_4(void);

static void cpl_polynomial_test_5(FILE *)
    CPL_ATTR_NONNULL;
static void cpl_polynomial_test_6(void)
    CPL_ATTR_NONNULL;

static void cpl_polynomial_test_7(FILE *)
    CPL_ATTR_NONNULL;

static void cpl_polynomial_test_dfs06121(void);

static void cpl_polynomial_fit_3(FILE *)
    CPL_ATTR_NONNULL;

static void cpl_polynomial_fit_test_1d(FILE *);
static cpl_polynomial * cpl_polynomial_fit_test_2d(FILE *, cpl_boolean);
static void cpl_polynomial_fit_bench_2d(cpl_size, cpl_size, cpl_size);

static void cpl_vector_fill_polynomial_fit_residual_test(void);

static double cpl_vector_get_mse(const cpl_vector     *,
                                 const cpl_polynomial *,
                                 const cpl_matrix     *,
                                 double *);

static cpl_error_code cpl_polynomial_fit_cmp(cpl_polynomial    *,
                                             const cpl_matrix  *,
                                             const cpl_boolean *,
                                             const cpl_vector  *,
                                             const cpl_vector  *,
                                             cpl_boolean        ,
                                             const cpl_size    *,
                                             const cpl_size    *);

static void cpl_polynomial_fit_cmp_(const cpl_polynomial *,
                                    const cpl_matrix  *,
                                    const cpl_boolean *,
                                    const cpl_vector  *,
                                    const cpl_vector  *,
                                    cpl_boolean        ,
                                    const cpl_size    *,
                                    const cpl_size    *);

static cpl_error_code cpl_polynomial_multiply_1d(cpl_polynomial *, double);
static void cpl_polynomial_solve_1d_test_ok(const cpl_polynomial *, double,
                                            double, cpl_size, double, double);

static void cpl_polynomial_compare_test(void);
static void cpl_polynomial_multiply_test(void);
static void cpl_polynomial_multiply_test_2d(void);

static void cpl_polynomial_test_union(void);

static cpl_flops  mse_flops = 0;
static double     mse_secs  = 0.0;

/**@{*/

/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/
int main(void)
{
    FILE              * stream;

    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    stream = cpl_msg_get_level() > CPL_MSG_INFO
        ? fopen("/dev/null", "a") : stdout;

    cpl_test_nonnull( stream );

    cpl_polynomial_eval_test_2d_one();

    cpl_polynomial_multiply_scalar_test_one(stream);

    cpl_polynomial_test_union();
    cpl_polynomial_basic_test(stream);

    cpl_polynomial_shift_test_2d_one(stream);
    cpl_polynomial_shift_test_3d_one(stream);

    cpl_polynomial_shift_test_1d(stream);
    cpl_polynomial_shift_test_2d(stream, 0);
    cpl_polynomial_shift_test_2d(stream, 1);

    cpl_polynomial_extract_test_3d_one(stream);
    cpl_polynomial_extract_test_2d_one(stream);
    cpl_polynomial_extract_test_2d(stream, 0);
    cpl_polynomial_extract_test_2d(stream, 1);

    cpl_polynomial_test_1(stream);
    cpl_polynomial_test_2(stream);
    cpl_polynomial_test_3();
    cpl_polynomial_test_4();
    cpl_polynomial_test_5(stream);
    cpl_polynomial_test_6();
    cpl_polynomial_test_7(stream);

    cpl_polynomial_test_dfs06121();

    cpl_polynomial_fit_3(stream);

    cpl_polynomial_multiply_test();
    cpl_polynomial_multiply_test_2d();

    cpl_polynomial_compare_test();

    cpl_vector_fill_polynomial_fit_residual_test();

    cpl_polynomial_fit_test_1d(stream);
    if (cpl_msg_get_level() <= CPL_MSG_INFO) 
        cpl_polynomial_fit_bench_2d(1, 5 * POLY_SIZE, POLY_SIZE);

    if (mse_secs > 0.0)
        cpl_msg_info("","Speed of cpl_vector_fill_polynomial_fit_residual() "
                     "over %g secs [Mflop/s]: %g", mse_secs,
                     (double)mse_flops/mse_secs/1e6);
    if (stream != stdout) cpl_test_zero( fclose(stream) );

    return cpl_test_end(0);
}

static void cpl_polynomial_test_union(void)
{
    cpl_polynomial * poly1 = cpl_polynomial_new(1);
    const cpl_size  i0     = POLY_SIZE;
    cpl_error_code  code;

    code = cpl_polynomial_set_coeff(poly1, &i0, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

#ifdef CPL_POLYNOMIAL_UNION_IS_DOUBLE_SZ
    cpl_test_eq(cpl_polynomial_get_1d_size(poly1),
                (1+i0) * (cpl_size)sizeof(double));
#endif

    cpl_test_assert(cpl_test_get_failed() == 0);

    cpl_polynomial_test_delete(poly1);
}

static void cpl_polynomial_test_1(FILE * stream)
{
    cpl_polynomial  *   poly1;
    cpl_polynomial  *   poly2;
    cpl_polynomial  *   nullpoly;
    double              x = 3.14;
    double              eps;
    cpl_size            i;
    cpl_error_code      code;

    nullpoly = cpl_polynomial_extract(NULL, 0, NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    
    cpl_test_null( nullpoly );

    code = cpl_polynomial_derivative(NULL, 0);
    cpl_test_eq_error(code, CPL_ERROR_NULL_INPUT);

    /* Create a polynomial */
        
    poly1 = cpl_polynomial_new(1);
    cpl_polynomial_dump(poly1, stream);

    i = cpl_polynomial_get_degree(poly1);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(i);

    code = cpl_polynomial_multiply_scalar(poly1, poly1, 0.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    i = cpl_polynomial_get_degree(poly1);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(i);

    /* Various tests regarding 0-degree polynomials */

    nullpoly = cpl_polynomial_extract(poly1, 0, poly1);
    cpl_test_error(CPL_ERROR_INVALID_TYPE);
    cpl_test_null( nullpoly );

    code = cpl_polynomial_derivative(poly1, 1);

    cpl_test_eq_error(code, CPL_ERROR_ACCESS_OUT_OF_RANGE);

    code = cpl_polynomial_derivative(poly1, 0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    eps = cpl_polynomial_eval_1d(poly1, 5, &x);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_abs( eps, 0.0, 0.0);

    cpl_test_abs(x, 0.0, 0.0 );

    code = cpl_polynomial_solve_1d(poly1, 5, &x, 1);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_abs(x, 0.0, 0.0 );

    eps = cpl_polynomial_eval_1d(poly1, 5, &x);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_abs( eps, 0.0, 0.0);

    cpl_test_abs(x, 0.0, 0.0 );

    /* Properly react to a 0-degree polynomial with no roots */
    i = 0;
    code = cpl_polynomial_set_coeff(poly1, &i, 1);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    eps = cpl_polynomial_get_coeff(poly1, &i);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_abs( eps, 1.0, 0.0 );

    code = cpl_polynomial_solve_1d(poly1, 1, &x, 1);
    cpl_test_eq_error(code, CPL_ERROR_DIVISION_BY_ZERO);

    /* Equivalent to setting all coefficients to zero */

    code = cpl_polynomial_derivative(poly1, 0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_abs( cpl_polynomial_eval_1d(poly1, 5, &x), 0.0, 0.0);

    cpl_test_abs( x, 0.0, 0.0);

    i = VECTOR_SIZE;
    code = cpl_polynomial_set_coeff(poly1, &i, 1) ;
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    cpl_test_abs( cpl_polynomial_get_coeff(poly1, &i), 1.0, 0.0 );

    code = cpl_polynomial_set_coeff(poly1, &i, 0) ;
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_eq( cpl_polynomial_get_dimension(poly1), 1);
    cpl_test_zero( cpl_polynomial_get_degree(poly1));

    cpl_test( poly2 = cpl_polynomial_duplicate(poly1) );

    cpl_test_zero( cpl_polynomial_get_degree(poly2));

    cpl_polynomial_test_delete(poly2);

    cpl_test( poly2 = cpl_polynomial_new(VECTOR_SIZE) );

    cpl_test_eq( cpl_polynomial_get_dimension(poly2), VECTOR_SIZE );
    cpl_test_zero( cpl_polynomial_get_degree(poly2));

    code = cpl_polynomial_copy(poly2, poly1);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_eq( cpl_polynomial_get_dimension(poly2), 1 );
    cpl_test_zero( cpl_polynomial_get_degree(poly2));

    cpl_polynomial_test_delete(poly1);
    cpl_polynomial_test_delete(poly2);

}

static void cpl_polynomial_test_2(FILE * stream)
{
    cpl_polynomial  *   poly1;
    cpl_polynomial  *   poly2;
    double              x = 3.14;
    double              z1;
    cpl_vector      *   vec;
    double          *   vec_data;
    cpl_size            i, j;
    int                 icmp;
    cpl_error_code      code;

    poly1 = cpl_polynomial_new(1);

    /* Set the coefficients : 1 + 2.x + 3.x^2 + 4.x^3 + 5.x^4 */
    for (i = 0; i < POLY_COEF_NB; i++) {
        if (i % (POLY_COEF_NB-2) != 0) continue;
        code = cpl_polynomial_set_coeff(poly1, &i, (double)(i+1));
        cpl_test_eq_error(code, CPL_ERROR_NONE);
        cpl_polynomial_dump(poly1, stream);
    }
    for (i = 0; i < POLY_COEF_NB; i++) {
        if (i % (POLY_COEF_NB-2) == 0) continue;
        cpl_polynomial_dump(poly1, stream);
        code = cpl_polynomial_set_coeff(poly1, &i, (double)(i+1));
        cpl_test_eq_error(code, CPL_ERROR_NONE);
    }
    cpl_polynomial_dump(poly1, stream);
    /* Test cpl_polynomial_get_degree() */
    i = cpl_polynomial_get_degree(poly1);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq(i , POLY_COEF_NB-1);

    /* Test cpl_polynomial_get_coeff() */
    for (i=0; i < POLY_COEF_NB ; i++) {
        const double coeff = cpl_polynomial_get_coeff(poly1, &i);
        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_abs(coeff, (double)(i+1), 0.0);
    }

    /* Test cpl_polynomial_eval() */
    vec = cpl_vector_new(1);
    vec_data = cpl_vector_get_data(vec);
    vec_data[0] = x;
    z1 = cpl_polynomial_eval(poly1, vec);
    cpl_test_lt(0.0, z1);
    cpl_vector_delete(vec);

    /* Dump polynomials */
    cpl_polynomial_dump(poly1, stream);

    /* Test cpl_polynomial_duplicate() and cpl_polynomial_copy() */
    poly2 = cpl_polynomial_duplicate(poly1);

    icmp = cpl_polynomial_compare(poly1, poly2, 0.0);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(icmp);

    code = cpl_polynomial_copy(poly1, poly2);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    icmp = cpl_polynomial_compare(poly1, poly2, 0);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(icmp);

    /* This should copy the polynomial */
    i = 9 + cpl_polynomial_get_degree(poly1);
    code = cpl_polynomial_set_coeff(poly1, &i, 42.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    i = 10 + cpl_polynomial_get_degree(poly1);
    code = cpl_polynomial_set_coeff(poly1, &i, 42.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_polynomial_multiply_scalar(poly1, poly2, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    cpl_test_polynomial_abs( poly1, poly2, 0.0);

    i = 10 + cpl_polynomial_get_degree(poly1);
    code = cpl_polynomial_set_coeff(poly1, &i, 42.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_polynomial_multiply_scalar(poly1, poly2, 0.5);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    i = cpl_polynomial_get_degree(poly1);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    j = cpl_polynomial_get_degree(poly2);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    cpl_test_eq(i, j);

    code = cpl_polynomial_multiply_scalar(poly1, poly1, 2.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    icmp = cpl_polynomial_compare(poly1, poly2, 0);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(icmp);

    code = cpl_polynomial_multiply_scalar(poly1, poly2, 0.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    i = cpl_polynomial_get_degree(poly1);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(i);

    code = cpl_polynomial_multiply_scalar(poly2, poly2, 0.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    i = cpl_polynomial_get_degree(poly2);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(i);

    /* Free */
    cpl_polynomial_test_delete(poly1);
    cpl_polynomial_test_delete(poly2);
}

static void cpl_polynomial_test_3(void)
{
    cpl_polynomial  *   poly1;
    double              x;
    cpl_size            i;
    cpl_error_code      code;

    x = 3.14;

    /* Properly react to a polynomial with one real root at x = 1
       and a proper (positive) 1st guess */
    poly1 = cpl_polynomial_new(1);

    i = 0;
    code = cpl_polynomial_set_coeff(poly1, &i, -1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    i++;
    code = cpl_polynomial_set_coeff(poly1, &i, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    i = 2 * POLY_SIZE;
    code = cpl_polynomial_set_coeff(poly1, &i, -1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    i++;
    code = cpl_polynomial_set_coeff(poly1, &i, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_polynomial_solve_1d(poly1, 5, &x, 1);
    cpl_test_eq_error( code, CPL_ERROR_NONE);

    cpl_test_abs( x,  1.0, 0.0 );

    cpl_polynomial_solve_1d_test_ok(poly1, 2.0, 2.0, 1, 0.0, 0.0);
    cpl_polynomial_solve_1d_test_ok(poly1, 2.0, 2.1, 1, 0.0, 0.0);
    /* A double root */
    cpl_polynomial_solve_1d_test_ok(poly1, 1.0, 2.0, 1, 5e7 * DBL_EPSILON,
                                    3.0 * DBL_EPSILON);
    cpl_polynomial_solve_1d_test_ok(poly1, 1.0, 2.0, 2, 5e5 * DBL_EPSILON, 0.0);

    cpl_polynomial_test_delete(poly1);

}

static void cpl_polynomial_test_4(void)
{
    cpl_polynomial  *   poly1;
    double              x, y, z2;
    cpl_vector      *   vec;
    cpl_vector      *   taylor;
    double          *   vec_data;
    double              xmax = 0; /* Maximum rounding error on x */
    double              ymax = 0; /* Maximum rounding error on y */
    double              eps;
    cpl_size            i;
    cpl_error_code      code;

    x = 3.14;
    y = 2.72;


    poly1 = cpl_polynomial_new(1);

    /* Create a tmporary vector - with a small length */
    cpl_test_nonnull( vec = cpl_vector_new(POLY_SIZE) );
    
    /* Fill the vector with a exp() Taylor series */
    vec_data = cpl_vector_get_data(vec);
    cpl_test_nonnull( vec_data );
    i = 0;
    vec_data[i] = 1;
    for (i=1 ; i < POLY_SIZE ; i++) {
        vec_data[i] = vec_data[i-1] / (double)i;
    }

    for (i=POLY_SIZE-1 ; i >= 0; i--) {
        code = cpl_polynomial_set_coeff(poly1, &i, vec_data[i]) ;
        cpl_test_eq_error(code, CPL_ERROR_NONE);
    }

    cpl_test_eq( cpl_polynomial_get_degree(poly1), POLY_SIZE-1);

    cpl_test_abs(cpl_polynomial_eval_1d(poly1, 0, &y), 1.0, DBL_EPSILON);
    cpl_test_abs( y, 1.0,  DBL_EPSILON);

    /* See how far away from zero the approximation holds */
    x = DBL_EPSILON;
    while ( fabs(cpl_polynomial_eval_1d(poly1, x, NULL)-exp( x))
          < DBL_EPSILON * exp( x) &&
            fabs(cpl_polynomial_eval_1d(poly1,-x, &y)-exp(-x))
          < DBL_EPSILON * exp(-x) ) {
        /* Differentation of exp() does not change anything
           - but in the case of a Taylor approximation one term is
           lost and with it a bit of precision */
        cpl_test_rel( exp(-x), y, 7.5 * DBL_EPSILON);
        x *= 2;
    }
    x /= 2;
    /* FIXME: Verify the correctness of this test */
    cpl_test_leq( -x, -FLT_EPSILON); /* OK for POLY_SIZE >= 4 */

    z2 = 2 * x / VECTOR_SIZE;

    /* Evaluate a sequence of exp()-approximations */
    taylor = cpl_vector_new(VECTOR_SIZE);
    cpl_test_zero( cpl_vector_fill_polynomial(taylor, poly1, -x, z2));

    vec_data = cpl_vector_get_data(taylor);

    cpl_test_nonnull( vec_data );
    for (i=0 ; i < VECTOR_SIZE ; i++) {
        const double xapp = -x + (double)i * z2; 
        const double yapp = exp( xapp );

        /* cpl_vector_fill_polynomial() is just a wrapper */
        cpl_test_abs( vec_data[i], cpl_polynomial_eval_1d(poly1, xapp, &y),
                      2*DBL_EPSILON);

        if ( fabs(y - yapp ) > ymax * yapp )
            ymax = fabs(y - yapp ) / yapp;

        if ( fabs(vec_data[i] - yapp ) > xmax * yapp )
            xmax = fabs(vec_data[i] - yapp ) / yapp;
    }

    cpl_msg_info("","Rounding on %d-term Taylor-exp() in range [%g; %g] "
        "[DBL_EPSILON]: %7.5f %7.5f", POLY_SIZE, -x, x, xmax/DBL_EPSILON,
        ymax/DBL_EPSILON);

    cpl_test_leq( xmax, 2.0*DBL_EPSILON);
    cpl_test_leq( xmax, ymax);
    cpl_test_leq( ymax, 7.39*DBL_EPSILON);

    vec_data = cpl_vector_get_data(vec);
    eps = vec_data[0];

    /* Solve p(y) = exp(x/2) - i.e. compute a logarithm */
    i = 0;
    vec_data[i] -= exp(x/2);
    cpl_polynomial_set_coeff(poly1, &i, vec_data[i]);
    code = cpl_polynomial_solve_1d(poly1, -x * x, &y, 1);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    vec_data[0] = eps;
    cpl_polynomial_set_coeff(poly1, &i, vec_data[i]);

    /* Check solution - allow up to 2 * meps rounding */
    cpl_test_rel( y, x/2,  2.0*DBL_EPSILON);

    /* Check Residual - allow up to 2 * meps rounding */
    cpl_test_rel( cpl_polynomial_eval_1d(poly1, y, NULL), exp(x/2), 2.0 * DBL_EPSILON );

    /* Free */
    cpl_vector_delete(vec);
    cpl_vector_delete(taylor);

    cpl_polynomial_test_delete(poly1);
}

static void cpl_polynomial_test_5(FILE * stream)
{
    cpl_polynomial  *   poly1;
    cpl_polynomial  *   poly2;
    cpl_polynomial  *   poly3;
    double              x, y, z1, z2;
    double              z, x1;
    /* Some binomial coefficients */
    double              p15[8] = {1,15,105,455,1365,3003,5005,6435};
    double              eps;
    cpl_size            expo[POLY_DIM];
    cpl_size            i, j;
    cpl_error_code      code;

    x = 3.14;
    y = 2.72;


    poly1 = cpl_polynomial_new(1);

    i = 0;
    cpl_polynomial_set_coeff(poly1, &i, 2);
    i++;
    cpl_polynomial_set_coeff(poly1, &i, 2);
    i++;
    cpl_polynomial_set_coeff(poly1, &i, 1);

    cpl_test_eq( cpl_polynomial_get_degree(poly1), 2);

    /* Properly react on a polynomial with no real roots */
    code = cpl_polynomial_solve_1d(poly1, 0, &x, 1);
    cpl_test_eq_error(code, CPL_ERROR_DIVISION_BY_ZERO);

    i = 0;
    cpl_polynomial_set_coeff(poly1, &i, 4);

    /* Properly react on a polynomial with no real roots */
    code = cpl_polynomial_solve_1d(poly1, 0, &x, 1);
    cpl_test_eq_error(code, CPL_ERROR_CONTINUE);

    cpl_polynomial_test_delete(poly1);

    poly1 = cpl_polynomial_new(1);

    /* The simplest 15-degree polynomial */
    i =15;
    cpl_polynomial_set_coeff(poly1, &i, 1);

    cpl_test_eq( cpl_polynomial_get_degree(poly1), i);

    code = cpl_polynomial_solve_1d(poly1, 10, &x, i);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_abs( x, 0.0, DBL_EPSILON);
    cpl_test_abs( cpl_polynomial_eval_1d(poly1, x, &y), 0.0, DBL_EPSILON);
    cpl_test_abs( y, 0.0, DBL_EPSILON);

    /* -1 is a root with multiplicity 15 */
    x1 = -1;

    poly2 = cpl_polynomial_duplicate(poly1);
    code = cpl_polynomial_shift_1d(poly2, 0, -x1) ;
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    /* poly2 now holds the binomial coefficients for n = 15 */

    i = 0;
    for (i = 0; i < 8; i++) {
        cpl_polynomial_set_coeff(poly1, &i, p15[i]);
        j = 15 - i;
        cpl_polynomial_set_coeff(poly1, &j, p15[i]);
    }

    i = 15;

    cpl_test_eq( cpl_polynomial_get_degree(poly1), i);

    for (i = 0; i < 8; i++) {
        j = 15 - i;
        z1 = cpl_polynomial_get_coeff(poly1, &i);
        z2 = cpl_polynomial_get_coeff(poly2, &i);
        z  = cpl_polynomial_get_coeff(poly2, &j);
        cpl_test_rel( z1, z2, 1.0 * DBL_EPSILON);
        cpl_test_rel( z,  z2, 1.0 * DBL_EPSILON);
    }

    cpl_test_zero( cpl_polynomial_compare(poly1,poly2,0) );
    
    i = 15;
    code = cpl_polynomial_solve_1d(poly1, 10*x1, &x, i);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    z = cpl_polynomial_eval_1d(poly1, x, &y);
    cpl_msg_info("", "(X+1)^15 (%" CPL_SIZE_FORMAT "): %g %g %g", i,
                 (x-x1)/DBL_EPSILON, z/DBL_EPSILON, y/DBL_EPSILON);
    cpl_test_rel( x, x1, DBL_EPSILON );
    cpl_test_abs( z, 0.0, DBL_EPSILON );
    cpl_test_abs( y, 0.0, DBL_EPSILON);

    /* Lots of round-off here, which depends on the long double precision */
    i = 5;
    code = cpl_polynomial_solve_1d(poly1, -10*x1, &x, i);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    z = cpl_polynomial_eval_1d(poly1, x, &y);
    cpl_msg_info("", "(X+1)^15 (%" CPL_SIZE_FORMAT ") " CPL_LDBL_EPSILON_STR
                 ": %g %g %g", i, x-x1,
                 (double)(z/CPL_LDBL_EPSILON),
                 (double)(y/CPL_LDBL_EPSILON));
    cpl_test_rel( x, x1, 0.35 );  /* alphaev56 */
    cpl_test_abs( z, 0.0, 18202.0   * DBL_EPSILON);
    cpl_test_abs( y, 0.0, 1554616.0 * DBL_EPSILON); /* alphaev56 */

    i = 15;
    eps = 2 * DBL_EPSILON;
    cpl_polynomial_set_coeff(poly1, &i, 1 + eps);

    cpl_test( cpl_polynomial_compare(poly1,poly2,-eps/2) < 0);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_zero( cpl_polynomial_compare(poly1,poly2,eps) );
    cpl_test( cpl_polynomial_compare(poly1,poly2,eps/2) > 0);

    cpl_polynomial_test_delete(poly2);

    poly2 = cpl_polynomial_new(POLY_DIM);
    cpl_polynomial_dump(poly2, stream);

    cpl_test_eq( cpl_polynomial_get_dimension(poly2), POLY_DIM);
    cpl_test_zero( cpl_polynomial_get_degree(poly2));

    /* Set and reset some (outragous) coefficient */
    for (j = 0; j <POLY_DIM;j++) expo[j] = j*VECTOR_SIZE + POLY_SIZE;
    code = cpl_polynomial_set_coeff(poly2, expo, 1);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    cpl_test_abs( cpl_polynomial_get_coeff(poly2, expo), 1.0, 0.0);
    code = cpl_polynomial_set_coeff(poly2, expo, 0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_zero( cpl_polynomial_get_degree(poly2));

    for (i = 0; i <= 15 ; i++) {
        double coeff;

        for (j = 0; j <POLY_DIM;j++) expo[j] = 0;
        expo[POLY_DIM/3] = i;

        coeff = cpl_polynomial_get_coeff(poly1, &i);
        cpl_test_error(CPL_ERROR_NONE);

        code = cpl_polynomial_set_coeff(poly2, expo, coeff);
        cpl_test_eq_error(code, CPL_ERROR_NONE);
    }

    /* Scale polynomial (with integer to avoid rounding)  */
    i = 15;
    cpl_polynomial_set_coeff(poly1, &i, 1);
    for (i = 0; i <= 15;i++) {
        const double coeff = cpl_polynomial_get_coeff(poly1, &i);

        cpl_test_error(CPL_ERROR_NONE);

        code = cpl_polynomial_set_coeff(poly1, &i, 5.0 * coeff);
        cpl_test_eq_error(code, CPL_ERROR_NONE);
    }

    i = 15;
    j = i - 1;
    code = cpl_polynomial_shift_1d(poly1, 0,
                                   -cpl_polynomial_get_coeff(poly1, &j)
                                   /(cpl_polynomial_get_coeff(poly1, &i)*(double)i));
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    i = 15;
    cpl_test_abs( cpl_polynomial_get_coeff(poly1, &i), 5.0, 0.0);

    for (i = 14; i >=0; i--)
        cpl_test_abs( cpl_polynomial_get_coeff(poly1, &i), 0.0, 0.0);

    code = cpl_polynomial_copy(poly1, poly2) ;
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_eq( cpl_polynomial_get_dimension(poly2), POLY_DIM);
    cpl_test_zero( cpl_polynomial_compare(poly1, poly2, 0) );

    i = 12;
    for (j = 0; j <POLY_DIM;j++) expo[j] = 0;
    expo[POLY_DIM/3] = i;
    code = cpl_polynomial_set_coeff(poly2, expo,
                                    cpl_polynomial_get_coeff(poly1, expo) *
                                    (1.0 + DBL_EPSILON));
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_lt(0, cpl_polynomial_compare(poly1, poly2, 0));

    code = cpl_polynomial_set_coeff(poly2, expo,
                                    cpl_polynomial_get_coeff(poly1, expo));
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_zero( cpl_polynomial_compare(poly1, poly2, 0) );

    x = cpl_polynomial_get_coeff(poly1, expo);

    for (j = 0; j <POLY_DIM;j++) expo[j] = 0;
    expo[POLY_DIM-1] = i;

    code = cpl_polynomial_set_coeff(poly2, expo, x) ;
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_lt(0, cpl_polynomial_compare(poly1, poly2, 0));

    cpl_polynomial_test_delete(poly2);

    poly3 = cpl_polynomial_new(POLY_DIM-1);
    for (j = 0; j <POLY_DIM;j++) expo[j] = 0;
    code = cpl_polynomial_set_coeff(poly3, expo, -1.0) ;
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    poly2 = cpl_polynomial_extract(poly1, 1, poly3);

    cpl_polynomial_test_delete(poly3);

    cpl_test_eq( cpl_polynomial_get_dimension(poly2),
                 cpl_polynomial_get_dimension(poly1) - 1);

    cpl_polynomial_dump(poly2, stream);

    cpl_polynomial_test_delete(poly1);
    cpl_polynomial_test_delete(poly2);

}

static void cpl_polynomial_test_6(void)
{
    cpl_polynomial  *   poly1;
    cpl_polynomial  *   poly2;
    double              x, y, z1, z2;
    double              z, y_1, y2, x1, x2;
    cpl_vector      *   vec;
    double              xmax = 0; /* Maximum rounding error on x */
    double              eps;
    cpl_size            expo[POLY_DIM];
    cpl_size            i, j;
    cpl_size            k;
    cpl_error_code      code;

    x = 3.14;
    y = 2.72;

    poly1 = cpl_polynomial_new(1);

    for (i = 0; i <=15;i++)
        cpl_polynomial_set_coeff(poly1, &i, 0);

    i = 0;
    cpl_polynomial_set_coeff(poly1, &i, 2);
    i++;
    cpl_polynomial_set_coeff(poly1, &i, -3);
    i = 3;
    cpl_polynomial_set_coeff(poly1, &i, 1);

    cpl_test_eq( cpl_polynomial_get_degree(poly1), i);

    /* 1 is a double root */
    x1 = 1;
    x2 = -2;
    y2 = 9;

    i = 2;
    code = cpl_polynomial_solve_1d(poly1, 10*x1, &x, i);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    z = cpl_polynomial_eval_1d(poly1, x, &y);
    cpl_msg_info("", "(X-1)^2(X+2) (%g) " CPL_LDBL_EPSILON_STR
                 ": %g %g %g", x1,
                 (double)((x-x1)/x1/CPL_LDBL_EPSILON), z/DBL_EPSILON,
                 (double)(y/CPL_LDBL_EPSILON));
     /* Lots of loss: */
    cpl_test_rel( x, x1, 250e8*CPL_LDBL_EPSILON );
    cpl_test_abs( z, 0.0, fabs(x1)*DBL_EPSILON );
    /* p`(x) should be zero */
    cpl_test_abs( y, 0.0, 15e10*CPL_LDBL_EPSILON );

    i = 1;
    code = cpl_polynomial_solve_1d(poly1, 10*x2, &x, i);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    z = cpl_polynomial_eval_1d(poly1, x, &y);
    cpl_msg_info("", "(X-1)^2(X+2) (%g) [DBL_EPSILON]: %g %g %g", x2,
                 (x-x2)/x2/DBL_EPSILON, z/DBL_EPSILON, (y-y2)/y2/DBL_EPSILON);
    cpl_test_abs( x, x2, fabs(x2)*DBL_EPSILON );
    cpl_test_abs( z, 0.0, fabs(x2)*DBL_EPSILON );
    cpl_test_abs( y, y2, DBL_EPSILON );

    /* When eps != 0 multiplicity should be 1 and 3 */
    eps = 0.5;
    x1 = 1 + eps;
    x2 = 1;
    y_1 = 0.125;

    i = 0;
    cpl_polynomial_set_coeff(poly1, &i, 1 + eps);
    i++;
    cpl_polynomial_set_coeff(poly1, &i, -4 - 3 * eps);
    i++;
    cpl_polynomial_set_coeff(poly1, &i, 6 + 3 * eps);
    i++;
    cpl_polynomial_set_coeff(poly1, &i, -4 - eps);
    i++;
    cpl_polynomial_set_coeff(poly1, &i, 1);

    code = cpl_polynomial_solve_1d(poly1, 10*x1, &x, 1);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    z = cpl_polynomial_eval_1d(poly1, x, &y);
    cpl_msg_info("", "(X-1)^3(X-1-eps) (%g) " CPL_LDBL_EPSILON_STR
                 ": %g %g %g", x1,
                 (double)((x-x1)/x1/CPL_LDBL_EPSILON), z/DBL_EPSILON,
                 (double)((y-y_1)/y_1/DBL_EPSILON));
    cpl_test_rel( x, x1, 32768.0 * CPL_LDBL_EPSILON );

    /* Need factor 3 on P-III Coppermine */
    cpl_test_abs( z, 0.0, 3.0*fabs(x1)*DBL_EPSILON );
    cpl_test_abs( y, y_1, 70e3*CPL_LDBL_EPSILON );

    i = 3;
    /* Try to solve for the triple-root - cannot get a good solution */
    code = cpl_polynomial_solve_1d(poly1, -10*x2, &x, i);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    z = cpl_polynomial_eval_1d(poly1, x, &y);
    cpl_msg_info("", "(X-1)^3(X-1-eps) (%g) [DBL_EPSILON]: %g %g %g", x2,
                 (x-x2)/x2/DBL_EPSILON, z/DBL_EPSILON,
                 y/DBL_EPSILON);
    /*
       Huge variance in accuracy:
       [DBL_EPSILON]:
       i686:      -3.33964e+06  0.00195312 -0.000488281
       sun4u:     105           0          -3.67206e-12
       9000/785:  105           0          -3.67206e-12
       alpha:     3.93538e+09  -1          -5161

       [LDBL_EPSILON]:
       i686:      -6.83958e+09  4          -1
       sun4u:      1.21057e+20  0          -4.2336e+06
       9000/785:   1.21057e+20  0          -4.2336e+06
       alpha:      3.93538e+09 -1          -5161
     */

    cpl_test_abs( x2, x, fabs(x2)*5e9*DBL_EPSILON );
    cpl_test_abs( z, 0.0, 1.5*fabs(x2)*DBL_EPSILON );
    cpl_test_abs( y, 0.0, 6e3*DBL_EPSILON );

    /* Try to decrease eps... */
    eps = 0.1;
    x1 = 1 + eps;
    x2 = 1;
    i = 0;
    cpl_polynomial_set_coeff(poly1, &i, 1 + eps);
    i++;
    cpl_polynomial_set_coeff(poly1, &i, -4 - 3 * eps);
    i++;
    cpl_polynomial_set_coeff(poly1, &i, 6 + 3 * eps);
    i++;
    cpl_polynomial_set_coeff(poly1, &i, -4 - eps);
    i++;
    cpl_polynomial_set_coeff(poly1, &i, 1);

    /* Cannot get a good solution */
    cpl_test_abs(cpl_polynomial_eval_1d(poly1, x1, &y_1), 0.0,
                            2.45 * DBL_EPSILON);
    code = cpl_polynomial_solve_1d(poly1, 10*x1, &x, 1);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    z = cpl_polynomial_eval_1d(poly1, x, &y);
    cpl_msg_info("", "(X-1)^3(X-1-eps) (%g) [DBL_EPSILON]: %g %g %g", x1,
                 (x-x1)/x1/DBL_EPSILON, z/DBL_EPSILON, (y-y_1)/y_1/DBL_EPSILON);
    cpl_test_abs( x, x1, fabs(x1)*2220 * DBL_EPSILON );
    cpl_test_abs( z, 0.0, fabs(x1) * 1e-5 * DBL_EPSILON );
    cpl_test_abs( y, y_1, 147e3*fabs(y_1)*DBL_EPSILON );


    /* Try to solve for the triple-root - cannot get a good solution */
    i = 3;
    code = cpl_polynomial_solve_1d(poly1, -10*x2, &x, 1);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    z = cpl_polynomial_eval_1d(poly1, x, &y);
    cpl_msg_info("", "(X-1)^3(X-1-eps) (%g) [DBL_EPSILON]: %g %g %g", x2,
                 (x-x2)/x2/DBL_EPSILON, z/DBL_EPSILON, y/DBL_EPSILON);

    /*
       Small variance in accuracy:
       [DBL_EPSILON]:
       i686:       -5.72688e+10   2.92578   -218506
       sun4u:      -5.72658e+10   2.92597   -218483
       9000/785:   -5.72658e+10   2.92597   -218483
       alpha:      -5.18581e+10   4         -179161
     */

    cpl_test_abs( x2, x, fabs(x2)*58e9*DBL_EPSILON );
    cpl_test_abs( z, 0.0, fabs(x2)*4*DBL_EPSILON );
    cpl_test_abs( y, 0.0, 22e4*DBL_EPSILON );


    /* An approximate dispersion relation with central wavelength at
       2.26463 micron (ISAAC.2002-11-13T05:30:53.242) */
    i = 0;
    cpl_polynomial_set_coeff(poly1, &i, 22016 - 22646.3);
    i++;
    cpl_polynomial_set_coeff(poly1, &i, 1.20695);
    i++;
    cpl_polynomial_set_coeff(poly1, &i, 3.72002e-05);
    i++;
    cpl_polynomial_set_coeff(poly1, &i, -3.08727e-08);
    i++;
    cpl_polynomial_set_coeff(poly1, &i, 0);

    cpl_test_eq( cpl_polynomial_get_degree(poly1), 3);

    x1 = 512.5; /* Central wavelength should be here */

    i = 1;
    cpl_polynomial_eval_1d(poly1, x1, &y_1);
    code = cpl_polynomial_solve_1d(poly1, 0, &x, i);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    z1 = cpl_polynomial_eval_1d(poly1, x, &y);
    cpl_msg_info("", "ISAAC (%g) [FLT_EPSILON]: %g %g %g", x1,
                 (x-x1)/x1/FLT_EPSILON, z1/DBL_EPSILON, (y-y_1)/y_1/FLT_EPSILON);

    cpl_test_abs( z1, 0.0, fabs(x1)*0.6*DBL_EPSILON );
    cpl_test_abs( y, y_1, 1e3*fabs(y_1)*FLT_EPSILON );

    /* Shift the polynomial 12.5 places */
    eps = 12.5;
    x2 = x1 - eps;
    x1 = x;
    code = cpl_polynomial_shift_1d(poly1, 0, eps);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_polynomial_eval_1d(poly1, x2, &y2);
    cpl_test_abs( y_1, y2, y_1*DBL_EPSILON);
    code = cpl_polynomial_solve_1d(poly1, 0, &x, i);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    y_1 = y;
    z2 = cpl_polynomial_eval_1d(poly1, x, &y);
    cpl_msg_info("", "Shift (%g) [DBL_EPSILON]: %g %g %g %g", eps,
                 (x-x2)/x2/DBL_EPSILON, z2/DBL_EPSILON,
                 (x1-x-eps)/x2/DBL_EPSILON, (y_1-y)/y_1/DBL_EPSILON);

    /* Root and derivative should be preserved to machine accuracy */
    cpl_test_abs( x1-x, eps, fabs(x2)*1.024*DBL_EPSILON);
    cpl_test_abs( y_1, y, 2*fabs(y_1)*DBL_EPSILON);

    x1 = 1e3;
    x2 = -1e2;

    xmax = cpl_polynomial_eval_1d(poly1, x2, NULL)
         - cpl_polynomial_eval_1d(poly1, x1, NULL)
         - cpl_polynomial_eval_1d_diff(poly1, x2, x1, &y);

    cpl_msg_info("", "Diff (%g:%g) [DBL_EPSILON]: %g ", x2,x1,xmax/DBL_EPSILON);

    cpl_test_abs(xmax, 0.0, 1.05 * fabs(x1)*DBL_EPSILON);
    cpl_test_abs(cpl_polynomial_eval_1d(poly1, x2, NULL), y, DBL_EPSILON);

    cpl_polynomial_test_delete(poly1);

    /* A small performance test - twice to test DFS02166 */
    for (k=0; k < 1 ; k++) {

        vec = cpl_vector_new(POLY_DIM);
        cpl_test_zero( cpl_vector_fill(vec, 1));

        poly2 = cpl_polynomial_new(POLY_DIM);
        for (i = 0; i < POLY_SIZE; i++) {
            for (j = 0; j < POLY_DIM; j++) expo[j] = j + i;
            code = cpl_polynomial_set_coeff(poly2, expo, 1);
            cpl_test_eq_error(code, CPL_ERROR_NONE);
        }

        for (i = 0; i < POLY_SIZE*VECTOR_SIZE; i++) {
            eps = cpl_polynomial_eval(poly2, vec);
        }
        cpl_polynomial_test_delete(poly2);
        cpl_vector_delete(vec);

    }
   
    for (k=0; k < 1 ; k++) {

        vec = cpl_vector_new(POLY_DIM);
        cpl_test_zero( cpl_vector_fill(vec, 1));

        poly2 = cpl_polynomial_new(POLY_DIM);
        for (i = 0; i < POLY_SIZE; i++) {
            for (j = 0; j < POLY_DIM; j++) expo[j] = j + i;
            code = cpl_polynomial_set_coeff(poly2, expo, 1);
            cpl_test_eq_error(code, CPL_ERROR_NONE);
        }

        for (i = 0; i < POLY_SIZE*VECTOR_SIZE; i++) {
            eps = cpl_polynomial_eval(poly2, vec);
        }
        cpl_polynomial_test_delete(poly2);
        cpl_vector_delete(vec);

    }
}

static void cpl_polynomial_test_7(FILE * stream)
{
    cpl_polynomial  *   nullpoly;
    cpl_polynomial  *   poly1;
    cpl_polynomial  *   poly2;
    cpl_polynomial  *   poly3;
    double              x, y, z1;
    double              z, x1, x2;
    cpl_vector      *   vec;
    double              xmax = 0; /* Maximum rounding error on x */
    double              eps;
    cpl_size            dim1, dim2;
    cpl_size            deg1, deg2;
    const cpl_size      expo0[2] = {0, 0};
    const cpl_size      i0 = 0;
    cpl_error_code      code;

    x = 3.14;
    y = 2.72;


    poly1 = cpl_polynomial_fit_test_2d(stream, CPL_TRUE);
    cpl_polynomial_test_delete(poly1);
    poly1 = cpl_polynomial_fit_test_2d(stream, CPL_FALSE);

    poly3 = cpl_polynomial_new(1);

    nullpoly = cpl_polynomial_extract(poly1, -1, poly3);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nullpoly);

    nullpoly = cpl_polynomial_extract(poly1, 2, poly3);
    cpl_test_error(CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_test_null(nullpoly);

    code = cpl_polynomial_dump(poly1, stream);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    poly2 = cpl_polynomial_extract(poly1, 0, poly3);

    dim2 = cpl_polynomial_get_dimension(poly2);
    cpl_test_error(CPL_ERROR_NONE);
    dim1 = cpl_polynomial_get_dimension(poly1);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq(dim2, dim1 - 1);

    /* poly1 comes from cpl_polynomial_fit_2d_create() */
    deg2 = cpl_polynomial_get_degree(poly2);
    cpl_test_error(CPL_ERROR_NONE);
    deg1 = cpl_polynomial_get_degree(poly1);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq(deg2, deg1);

    code = cpl_polynomial_dump(poly2, stream);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    eps = cpl_polynomial_get_coeff(poly2, &i0);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_polynomial_test_delete(poly2);

    poly2 = cpl_polynomial_extract(poly1, 1, poly3);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(poly2);

    dim2 = cpl_polynomial_get_dimension(poly2);
    cpl_test_error(CPL_ERROR_NONE);
    dim1 = cpl_polynomial_get_dimension(poly1);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq(dim2, dim1 - 1);

    /* poly1 comes from cpl_polynomial_fit_2d_create() */
    deg2 = cpl_polynomial_get_degree(poly2);
    cpl_test_error(CPL_ERROR_NONE);
    deg1 = cpl_polynomial_get_degree(poly1);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq(deg2, deg1);

    code = cpl_polynomial_dump(poly2, stream);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    xmax = eps - cpl_polynomial_get_coeff(poly2, &i0);
    cpl_polynomial_test_delete(poly2);

    cpl_test_abs( xmax, 0.0, 0.0);

    /* Use this block to benchmark 2D cpl_polynomial_eval() */
    vec = cpl_vector_new(2);

    code = cpl_vector_fill(vec, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    for (cpl_size k = 0; k < VECTOR_SIZE*VECTOR_SIZE; k++) {
        eps = cpl_polynomial_eval(poly1, vec);
        cpl_test_error(CPL_ERROR_NONE);
    }

    cpl_vector_delete(vec);
    cpl_polynomial_test_delete(poly1);


    /* Create and differentiate a 1d-polynomial with uniform coefficients */
    poly1 = cpl_polynomial_new(1);

    x = CPL_MATH_PI_4;
    for (cpl_size i = 0; i < POLY_SIZE; i++) {
        code = cpl_polynomial_set_coeff(poly1, &i, x);
        cpl_test_eq_error(code, CPL_ERROR_NONE);
    }

    x1 = x;
    for (cpl_size i = POLY_SIZE-1; i > 0; i--) {

        x1 *= (double)i;
        code = cpl_polynomial_derivative(poly1, 0);
        cpl_test_eq_error(code, CPL_ERROR_NONE);
    }

    deg1 = cpl_polynomial_get_degree(poly1);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(deg1);

     /* Verify the value of the constant term */
    cpl_test_abs( x1, cpl_polynomial_get_coeff(poly1, &i0), 0.0);

    cpl_polynomial_test_delete(poly1);

    /* Create and collapse and differentiate a 2d-polynomial with
       uniform coefficients */
    poly1 = cpl_polynomial_new(2);

    x = CPL_MATH_PI_4;
    y = CPL_MATH_E;
    x1 = 1.0;
    for (cpl_size i = 0; i < POLY_SIZE; i++) {
        for (cpl_size j = 0; j < POLY_SIZE; j++) {
            const cpl_size expo[2] = {i, j};

            code = cpl_polynomial_set_coeff(poly1, expo, x);
            cpl_test_eq_error(code, CPL_ERROR_NONE);
        }
        if (i > 0) x1 *= (double)i;
    }

    code = cpl_polynomial_set_coeff(poly3, &i0, y);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    for (cpl_size j = 0; j < 2; j++) {

        poly2 = cpl_polynomial_extract(poly1, j, poly3);
        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_nonnull(poly2);

        z1 = 0.0;
        for (cpl_size i = 0; i < POLY_SIZE; i++) {
            z1 += pow(y, (double)i);
        }

        z1 *= x;

        for (cpl_size i = 0; i < POLY_SIZE; i++) {
            z = cpl_polynomial_get_coeff(poly2, &i);
            cpl_test_error(CPL_ERROR_NONE);
            cpl_test_abs( z, z1, 2.0*POLY_SIZE*FLT_EPSILON);
        }

        code = cpl_polynomial_derivative(poly2, 0);
        cpl_test_eq_error(code, CPL_ERROR_NONE);

        z = cpl_polynomial_get_coeff(poly2, &i0);

        for (cpl_size i = 1; i < POLY_SIZE-1; i++) {
            z1 = cpl_polynomial_get_coeff(poly2, &i);
            cpl_test_error(CPL_ERROR_NONE);
            cpl_test_abs( z * (double)(i+1), z1, 11.0*POLY_SIZE*FLT_EPSILON);
        }

        cpl_polynomial_test_delete(poly2);
    }

    cpl_polynomial_test_delete(poly3);

    for (cpl_size i = POLY_SIZE-1; i > 0; i--) {
        code = cpl_polynomial_derivative(poly1, 0);
        cpl_test_eq_error(code, CPL_ERROR_NONE);

        code = cpl_polynomial_derivative(poly1, 1);
        cpl_test_eq_error(code, CPL_ERROR_NONE);
    }

    deg1 = cpl_polynomial_get_degree(poly1);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(deg1);

    x2 = cpl_polynomial_get_coeff(poly1, expo0);
    cpl_test_error(CPL_ERROR_NONE);

    /* The constant term is huge at this point
       - reduce by a factor eps which is inaccurate */
    eps = x2 * DBL_EPSILON > 1 ? x2 * DBL_EPSILON : 1;
    xmax = x2/eps - x * x1 * x1/eps;
    cpl_test_abs( xmax, 0.0, 1.0);


    cpl_polynomial_test_delete(poly1);
}

static void cpl_polynomial_test_dfs06121(void)
{
    cpl_polynomial  *   poly2;
    cpl_size            expo[2];
    cpl_size            i;
    cpl_error_code      code;


    /* Test for DFS06121 */
    poly2 = cpl_polynomial_new(2);
    expo[0] = 0;

    expo[1] = 0;
    code = cpl_polynomial_set_coeff(poly2, expo, -256.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    expo[1] = 1;
    code = cpl_polynomial_set_coeff(poly2, expo, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    expo[1] = 2;
    code = cpl_polynomial_set_coeff(poly2, expo, 0.001);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_polynomial_derivative(poly2, 0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    i = cpl_polynomial_get_degree(poly2);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(i);

    code = cpl_polynomial_derivative(poly2, 0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    i = cpl_polynomial_get_degree(poly2);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(i);

    code = cpl_polynomial_derivative(poly2, 1);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    i = cpl_polynomial_get_degree(poly2);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(i);

    cpl_polynomial_test_delete(poly2);

}

/**@}*/

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief      Test the CPL function
  @return    void

 */
/*----------------------------------------------------------------------------*/
static void cpl_vector_fill_polynomial_fit_residual_test(void)
{

    cpl_error_code    code;

    /* Test 1: NULL input */
    code = cpl_vector_fill_polynomial_fit_residual(NULL, NULL, NULL, NULL,
                                                    NULL, NULL);

    cpl_test_eq_error(code, CPL_ERROR_NULL_INPUT);

    return;

}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Test cpl_polynomial_fit() in 2D
  @param    stream  A stream to dump to
  @param    dimdeg  Passed to cpl_polynomial_fit()
  @return   void

 */
/*----------------------------------------------------------------------------*/
static cpl_polynomial * cpl_polynomial_fit_test_2d(FILE * stream,
                                                   cpl_boolean dimdeg)
{

    cpl_matrix     * xy_pos;
    cpl_vector     * xpoint;
    cpl_vector     * xpoint0;
    const double     xy_offset = 100;
    cpl_size         degree;
    cpl_polynomial * poly1  = cpl_polynomial_new(2);
    cpl_polynomial * poly2  = cpl_polynomial_new(2);
    cpl_polynomial * poly2a = cpl_polynomial_new(2);
    cpl_vector     * vec;
    double           xmax = 0.0; /* Maximum rounding code on x */
    double           ymax = 0.0; /* Maximum rounding code on y */
    double           eps;
    double           x, y;
    const cpl_size   zerodeg2[]  = {0, 0};
    const cpl_size   zerodeg2a[] = {1, 1};
    cpl_size         i, j, k;
    double         * dxy_pos;
    double         * dvec;
    cpl_error_code   code;


    /* Try to fit increasing degrees to f(x,y) = sqrt(x)*log(1+y)
       - with exactly as many points as there are coefficient to determine
         thus the residual should be exactly zero.
    */

    xpoint0 = cpl_vector_new(2);
    cpl_vector_set(xpoint0, 0, 0.0);
    cpl_vector_set(xpoint0, 1, 0.0);

    xpoint = cpl_vector_new(2);
    /* f(1/4,sqrt(e)-1) = 1/4 */
    cpl_vector_set(xpoint, 0, 0.25+xy_offset);
    cpl_vector_set(xpoint, 1, sqrt(CPL_MATH_E)-1+xy_offset);

    for (degree = 0; degree < POLY_SIZE; degree++) {
        const cpl_size maxdeg[2] = {degree, degree};
        const cpl_size nc = dimdeg ? (degree+1)*(degree+1)
            : (degree+1)*(degree+2)/2;

        cpl_msg_info(cpl_func, "Fitting degree=%d, nc=%d, dimdeg=%d",
                     (int)degree, (int)nc, (int)dimdeg);

        k = 0;

        vec = cpl_vector_new(nc);
        dvec = cpl_vector_get_data(vec);
        xy_pos = cpl_matrix_new(2, nc);
        dxy_pos = cpl_matrix_get_data(xy_pos);

        for (i = 0; i < 1 + degree; i++) {
            for (j=0; j < 1 + (dimdeg ? degree : i); j++, k++) {
                dxy_pos[     k] = (double)i+xy_offset;
                dxy_pos[nc + k] = (double)j+xy_offset;
                dvec[        k] = sqrt((double)i) * log((double)(j+1));
            }
        }

        cpl_test_eq( k, nc );

        /* First try with mindeg = 1 */
        code = cpl_polynomial_fit_cmp(poly2a, xy_pos, NULL, vec, NULL,
                                      dimdeg, zerodeg2a, maxdeg);
        if (degree == 0) {
            cpl_test_eq_error(code, CPL_ERROR_ILLEGAL_INPUT);
        } else if (code == CPL_ERROR_SINGULAR_MATRIX) {
            cpl_msg_info(cpl_func, "Singular fit: degree=%d, nc=%d, dimdeg=%d",
                         (int)degree, (int)nc, (int)dimdeg);
            cpl_test_error(CPL_ERROR_SINGULAR_MATRIX);
            cpl_test_leq(11, nc); /* Currently attainable precision */
        } else {
            cpl_test_eq_error(code, CPL_ERROR_NONE);

            cpl_test_eq( cpl_polynomial_get_dimension(poly2a), 2 );
            cpl_test_eq( cpl_polynomial_get_degree(poly2a),
                         dimdeg ? 2 * degree : degree );

            cpl_polynomial_dump(poly2a, stream);

            eps = cpl_polynomial_get_coeff(poly2a, zerodeg2);
            cpl_test_abs(eps, 0.0, 0.0);

            eps = cpl_polynomial_eval(poly2a, xpoint0);
            cpl_test_abs(eps, 0.0, 0.0);

            (void)cpl_vector_get_mse(vec, poly2a, xy_pos, NULL);
            cpl_test_error(CPL_ERROR_NONE);
        }

        code = cpl_polynomial_fit_cmp(poly2, xy_pos, NULL, vec, NULL,
                                       dimdeg, zerodeg2, maxdeg);
        cpl_test_eq_error(code, CPL_ERROR_NONE);

        cpl_test_eq( cpl_polynomial_get_dimension(poly2), 2 );
        cpl_test_eq( cpl_polynomial_get_degree(poly2),
                     dimdeg ? 2 * degree : degree );

        cpl_polynomial_dump(poly2, stream);

        eps = cpl_vector_get_mse(vec, poly2, xy_pos, NULL);

        /* The increase in mse must be bound */
        if (xmax > 0) cpl_test_leq( eps/xmax, 5.39953e+09);

        cpl_matrix_delete(xy_pos);

        cpl_msg_info(cpl_func,"2D-Polynomial with degree %" CPL_SIZE_FORMAT
                     " (dimdeg=%d) fit of a %" CPL_SIZE_FORMAT "-point "
                     "dataset has a mean square error ratio to a %"
                     CPL_SIZE_FORMAT "-degree fit: %g (%g > %g)", degree,
                     dimdeg, nc, degree-1, xmax > 0.0 ? eps/xmax : 0.0,
                     eps, xmax);
        xmax = eps;

        eps = cpl_polynomial_eval(poly2, xpoint);

        if (nc < (dimdeg ? 25 : 40))
            cpl_test_abs( eps, 0.25, fabs(0.25 - ymax));

        cpl_vector_delete(vec);
        if (fabs(0.25-eps) >= fabs(0.25 - ymax) && degree > 0) {
            /* Should be able to fit at least a 5-degree polynomial
               with increased accuracy - and without code-margin */
            cpl_msg_info(cpl_func,"2D-Polynomial with degree %" CPL_SIZE_FORMAT
                         " fit of a %" CPL_SIZE_FORMAT "-point dataset has a "
                         "greater residual than a %" CPL_SIZE_FORMAT "-degree "
                         "fit to a %" CPL_SIZE_FORMAT "-point dataset: "
                         "fabs(%g) > fabs(%g)", degree, nc, degree-1,
                         degree*(degree+1)/2, eps-0.25, ymax-0.25);
            break;
        }

        ymax = eps;
        

    }

    /* Try to fit increasing degrees to f(x,y) = sqrt(x)*log(1+y)
       - with a constant, overdetermined number of points
       - The residual should decrease with increasing degree until the system
       becomes near singular */

    /* f(1/4,sqrt(e)-1) = 1/4 */
    cpl_vector_set(xpoint, 0, 0.25+xy_offset);
    cpl_vector_set(xpoint, 1, sqrt(CPL_MATH_E)-1-xy_offset);

    k = 0;

    vec    = cpl_vector_new(   2 * POLY_SIZE * 2 * POLY_SIZE);
    dvec = cpl_vector_get_data(vec);
    xy_pos = cpl_matrix_new(2, 2 * POLY_SIZE * 2 * POLY_SIZE);
    dxy_pos = cpl_matrix_get_data(xy_pos);

    for (i = 0, k = 0; i < 2 * POLY_SIZE; i++) {
        for (j=0; j < 2 * POLY_SIZE; j++, k++) {

            x = (double) i * 0.5;
            y = (double) j * 2.0;

            dxy_pos[                                k] = (double)i+xy_offset;
            dxy_pos[2 * POLY_SIZE * 2 * POLY_SIZE + k] = (double)j-xy_offset;
            dvec[                                   k] = sqrt(x) * log1p(y);
        }
    }

    cpl_test_eq( 2 * POLY_SIZE * 2 * POLY_SIZE, k );

    ymax = 0;
    for (degree = 0; degree < POLY_SIZE; degree++) {
        const cpl_size maxdeg[2] = {degree, degree};
        const cpl_size nc = 2 * POLY_SIZE * 2 * POLY_SIZE;
        double mse;

        code = cpl_polynomial_fit_cmp(poly2, xy_pos, NULL, vec, NULL,
                                   dimdeg, zerodeg2, maxdeg);
        cpl_test_eq_error(code, CPL_ERROR_NONE);

        cpl_test_eq( cpl_polynomial_get_dimension(poly2), 2 );
        if (dimdeg)
            cpl_test_eq(cpl_polynomial_get_degree(poly2), degree * 2);
        else
            cpl_test_eq(cpl_polynomial_get_degree(poly2), degree);

        code = cpl_polynomial_dump(poly2, stream);
        cpl_test_eq_error(code, CPL_ERROR_NONE);

        mse = cpl_vector_get_mse(vec, poly2, xy_pos, NULL);

        eps = cpl_polynomial_eval(poly2, xpoint);

        cpl_msg_info(cpl_func, "2D-Polynomial with degree %" CPL_SIZE_FORMAT
                     " fit of a %" CPL_SIZE_FORMAT "-point dataset "
                     "has a mean square error: %g (P0-code=%g)",
                     degree, nc, mse, 0.25-eps);

        /* mse must decrease */
        if (degree > 0) {
            if (fabs(eps-0.25) > fabs(ymax - 0.25))
                cpl_msg_info(cpl_func, "2D-Polynomial with degree %"
                             CPL_SIZE_FORMAT " fit of a %" CPL_SIZE_FORMAT
                             "-point dataset has a larger error than a %"
                             CPL_SIZE_FORMAT "-degree fit: fabs(%g-0.25) > "
                             "fabs(%g-0.25)",
                             degree, nc, degree - 1, eps, ymax);
            if (mse > xmax) {
                cpl_msg_info(cpl_func, "2D-Polynomial with degree %"
                             CPL_SIZE_FORMAT " fit of a %" CPL_SIZE_FORMAT
                             "-point dataset has a larger mean square error "
                             "than a %" CPL_SIZE_FORMAT "-degree fit: %g > %g",
                             degree, nc, degree - 1, mse, xmax);
                break;
            }
        }

        xmax = mse;
        ymax = eps;

        code = cpl_polynomial_copy(poly1, poly2);
        cpl_test_eq_error(code, CPL_ERROR_NONE);
    }

    cpl_vector_delete(vec);
    cpl_matrix_delete(xy_pos);

    cpl_vector_delete(xpoint);
    cpl_vector_delete(xpoint0);
    cpl_polynomial_test_delete(poly2);
    cpl_polynomial_test_delete(poly2a);

    return poly1;

}



/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief      Benchmark the CPL function
  @param nr   The number of repeats
  @param nc   The number of coefficients
  @param nd   The max degree
  @return    void

 */
/*----------------------------------------------------------------------------*/
static void cpl_polynomial_fit_bench_2d(cpl_size nr, cpl_size nc, cpl_size nd)
{


    cpl_polynomial * poly2  = cpl_polynomial_new(2);
    cpl_vector     * vec    = cpl_vector_new(nc * nc);
    cpl_matrix     * xy_pos = cpl_matrix_new(2, nc * nc);
    cpl_flops        flops  = 0;
    double           secs   = 0.0;
    double           xmax   = DBL_MAX; /* This valuie should not be read */
    cpl_size         i, j, k, degree = 0;
    cpl_error_code   code = CPL_ERROR_NONE;



    for (i=0, k = 0; i < nc; i++) {
        for (j=0; j < nc; j++, k++) {
            const double x = (double) i * 0.5;
            const double y = (double) j * 2.0;

            code = cpl_matrix_set(xy_pos, 0, k, (double)i);
            cpl_test_eq_error(code, CPL_ERROR_NONE);

            code = cpl_matrix_set(xy_pos, 1, k, (double)j);
            cpl_test_eq_error(code, CPL_ERROR_NONE);

            code = cpl_vector_set(vec,   k, sqrt(x)*log1p(y));
            cpl_test_eq_error(code, CPL_ERROR_NONE);
        }
    }

    cpl_test_eq( k, nc * nc );

    for (i = 0; i < nr; i++) {
        for (degree = 0; degree < nd; degree++) {
            double mse;
            const cpl_flops flops0 = cpl_tools_get_flops();
            const double    secs0  = cpl_test_get_cputime();

            code = cpl_polynomial_fit_cmp(poly2, xy_pos, NULL, vec, NULL,
                                       CPL_FALSE, NULL, &degree);

            secs += cpl_test_get_cputime() - secs0;
            flops += cpl_tools_get_flops() - flops0;

            cpl_test_eq_error(code, CPL_ERROR_NONE);

            if (code) break;

            mse = cpl_vector_get_mse(vec, poly2, xy_pos, NULL);

            if (degree > 0 && mse > xmax) break;

            xmax = mse;

        }
        if (code) break;
    }

    cpl_vector_delete(vec);
    cpl_matrix_delete(xy_pos);
    cpl_polynomial_test_delete(poly2);

    cpl_msg_info("","Speed while fitting %" CPL_SIZE_FORMAT " 2D-points with "
                 "up to degree %" CPL_SIZE_FORMAT " in %g secs [Mflop/s]: %g",
                 nc*nc, degree, secs, (double)flops/secs/1e6);

}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Test cpl_polynomial_fit() in 1D
  @param    stream  A stream to dump to
  @return   void

 */
/*----------------------------------------------------------------------------*/
static void cpl_polynomial_fit_test_1d(FILE * stream)
{

    const double      dvec6[] = {1, 3, 5, 2, 4, 6};
    const double      dtay6[] = {0, 2*4, 2*20, 2*1, 2*10, 2*35};
    const double      dvec4[] = {1, 3, 4, 6};
    const double      dtay4[] = {0, 2*4, 2*10, 2*35};
    const double      dsq6[]  = {1, 9, 25, 4, 16, 36};
    CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual)
    cpl_matrix      * samppos1 = cpl_matrix_wrap(1, 6, (double*)dvec6);
    cpl_vector      * taylor = cpl_vector_wrap(6, (double*)dtay6);
    cpl_matrix      * samppos14 = cpl_matrix_wrap(1, 4, (double*)dvec4);
    cpl_vector      * taylor4 = cpl_vector_wrap(4, (double*)dtay4);
    CPL_DIAG_PRAGMA_POP
    cpl_matrix      * samppos;
    cpl_vector      * fitvals;
    cpl_polynomial  * poly1a = cpl_polynomial_new(1);
    cpl_polynomial  * poly1b = cpl_polynomial_new(1);
    cpl_polynomial  * poly2 = cpl_polynomial_new(2);
    const cpl_boolean symsamp = CPL_TRUE;
    const cpl_size    zerodeg = 0;
    const cpl_size    sqdeg  = 2;
    const cpl_size    maxdeg = 3;
    cpl_size          mindeg, errdeg, deg1;
    double            eps, rechisq;
    double            xmax; /* Maximum rounding code on x */
    double            ymax; /* Maximum rounding code on y */
    double            zmax;
    cpl_error_code    code;
    cpl_size          i, j;


    /* Test 1: NULL input */
    code = cpl_polynomial_fit_cmp(NULL, NULL, NULL, NULL, NULL, CPL_FALSE,
                               NULL, NULL);
    cpl_test_eq_error(code, CPL_ERROR_NULL_INPUT);

    code = cpl_polynomial_fit_cmp(NULL, samppos1, NULL, taylor, NULL,
                               CPL_FALSE, NULL, &maxdeg);
    cpl_test_eq_error(code, CPL_ERROR_NULL_INPUT);

    code = cpl_polynomial_fit_cmp(poly1a, NULL, NULL, taylor, NULL,
                               CPL_FALSE, NULL, &maxdeg);
    cpl_test_eq_error(code, CPL_ERROR_NULL_INPUT);

    code = cpl_polynomial_fit_cmp(poly1a, samppos1, NULL, NULL, NULL,
                               CPL_FALSE, NULL, &maxdeg);
    cpl_test_eq_error(code, CPL_ERROR_NULL_INPUT);

    code = cpl_polynomial_fit_cmp(poly1a, samppos1, NULL, taylor, NULL,
                               CPL_FALSE, NULL, NULL);
    cpl_test_eq_error(code, CPL_ERROR_NULL_INPUT);

    /* Test 1a: negative maxdeg */
    errdeg = -1;
    code = cpl_polynomial_fit_cmp(poly1a, samppos1, NULL, taylor, NULL,
                               CPL_FALSE, NULL, &errdeg);
    cpl_test_eq_error(code, CPL_ERROR_ILLEGAL_INPUT);

    /* Test 1b: maxdeg less than mindeg*/
    errdeg = 0;
    code = cpl_polynomial_fit_cmp(poly1a, samppos1, NULL, taylor, NULL,
                               CPL_FALSE, &maxdeg, &errdeg);
    cpl_test_eq_error(code, CPL_ERROR_ILLEGAL_INPUT);

    /* Test 1c: Wrong dimension of poly */

    code = cpl_polynomial_fit_cmp(poly2, samppos1, NULL, taylor, NULL,
                               CPL_FALSE, NULL, &maxdeg);
    cpl_test_eq_error(code, CPL_ERROR_INCOMPATIBLE_INPUT);

    /* Test 2: Unordered insertion */

    code = cpl_polynomial_fit_cmp(poly1a, samppos1, NULL, taylor, NULL,
                               CPL_FALSE, &zerodeg, &maxdeg);

    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_polynomial_fit_cmp(poly1a, samppos1, NULL, taylor, NULL,
                                  CPL_FALSE, &zerodeg, &maxdeg);

    cpl_test_eq_error(code, CPL_ERROR_NONE);

    eps = cpl_vector_get_mse(taylor, poly1a, samppos1, &rechisq);

    cpl_test_leq(0.0, rechisq);

    cpl_test_abs( eps, 0.0, 4359*DBL_EPSILON*DBL_EPSILON); /* alphaev56 */

    /* Test 3: Symmetric 1D sampling (also test dimdeg and reset of preset
               1D-coeffs) */
    errdeg = maxdeg + 1;
    code = cpl_polynomial_set_coeff(poly1b, &errdeg, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_polynomial_fit_cmp(poly1b, samppos1, &symsamp, taylor, NULL,
                               CPL_TRUE, &zerodeg, &maxdeg);

    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_polynomial_abs(poly1a, poly1b, DBL_EPSILON);

    cpl_test_abs( cpl_vector_get_mse(taylor, poly1b, samppos1, NULL), 0.0,
                  fabs(eps));

    /* Test 3A: Same, except mindeg set to 1 */
    mindeg = 1;
    code = cpl_polynomial_set_coeff(poly1b, &errdeg, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_polynomial_fit_cmp(poly1b, samppos1, &symsamp, taylor, NULL,
                               CPL_TRUE, &mindeg, &maxdeg);

    cpl_test_eq_error(code, CPL_ERROR_NONE);

    deg1 = cpl_polynomial_get_degree(poly1b);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq(deg1, maxdeg);
    cpl_test_abs(cpl_polynomial_get_coeff(poly1b, &zerodeg), 0.0, 0.0);

    code = cpl_polynomial_set_coeff(poly1a, &zerodeg, 0.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_polynomial_abs(poly1a, poly1b, 700.0 * DBL_EPSILON);

    cpl_test_abs( cpl_vector_get_mse(taylor, poly1b, samppos1, NULL), 0.0,
                  70.0 * fabs(eps));

    /* Test 3B: Symmetric, non-equidistant 1D sampling (also test dimdeg and
                reset of preset 1D-coeffs) */
    code = cpl_polynomial_fit_cmp(poly1a, samppos14, NULL, taylor4, NULL,
                               CPL_TRUE, &zerodeg, &maxdeg);

    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_polynomial_fit_cmp(poly1b, samppos14, &symsamp, taylor4, NULL,
                               CPL_TRUE, &zerodeg, &maxdeg);

    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_polynomial_abs(poly1a, poly1b, DBL_EPSILON);

    /* Test 3C: Same except mindeg set to 1 */
    code = cpl_polynomial_fit_cmp(poly1b, samppos14, &symsamp, taylor4, NULL,
                               CPL_TRUE, &mindeg, &maxdeg);

    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_eq(cpl_polynomial_get_degree(poly1b), maxdeg);
    cpl_test_abs(cpl_polynomial_get_coeff(poly1b, &zerodeg), 0.0, 0.0);

    code = cpl_polynomial_set_coeff(poly1a, &zerodeg, 0.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_polynomial_abs(poly1a, poly1b, 600.0 * DBL_EPSILON);

    /* Not overdetermined, so must pass NULL for rechisq */
    eps = cpl_vector_get_mse(taylor4, poly1a, samppos14, NULL);

    cpl_test_error(CPL_ERROR_NONE);

    cpl_test_abs( cpl_vector_get_mse(taylor4, poly1b, samppos14, NULL), 0.0,
                  283.0 * fabs(eps));

    /* Test 4: Only one distinct sampling point */
    samppos = cpl_matrix_new(1, 6);
    cpl_test_zero(cpl_matrix_fill(samppos, 1.0));

    /*  - should not be able to fit with only one distinct x-value */    
    code = cpl_polynomial_fit_cmp(poly1a, samppos, &symsamp, taylor, NULL,
                               CPL_TRUE, &zerodeg, &maxdeg);
    cpl_test_eq_error(code, CPL_ERROR_SINGULAR_MATRIX);

    /* Test 4B: - unless the degree is 0 */
    code = cpl_polynomial_fit_cmp(poly1a, samppos, &symsamp, taylor, NULL,
                               CPL_TRUE, &zerodeg, &zerodeg);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    deg1 = cpl_polynomial_get_degree(poly1a);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(deg1);

    cpl_test_abs( cpl_polynomial_get_coeff(poly1a, &zerodeg),
                  cpl_vector_get_mean(taylor), DBL_EPSILON);


    /* Test 4B: - or unless mindeg equals the degree */
    for (i = 1; i < POLY_SIZE; i++) {
        code = cpl_polynomial_fit_cmp(poly1a, samppos, &symsamp, taylor, NULL,
                                   CPL_TRUE, &i, &i);
        cpl_test_eq_error(code, CPL_ERROR_NONE);

        cpl_test_eq(cpl_polynomial_get_degree(poly1a), i);

        /* FIXME: Weak test, sampling positions are all at 1.0 */
        /* Another test should be added */
        cpl_test_abs( cpl_polynomial_get_coeff(poly1a, &i),
                      cpl_vector_get_mean(taylor), 16.0 * DBL_EPSILON);

        code = cpl_polynomial_set_coeff(poly1a, &i, 0.0);
        cpl_test_eq_error(code, CPL_ERROR_NONE);

        deg1 = cpl_polynomial_get_degree(poly1a);
        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_zero(deg1);
    }

    /* Test 5: Try to fit j-coefficients to a sqrt() sampled j
                times at 0,1,...,j-1
       - since sqrt(0) == 0, the constant coefficient of the fit
         should be zero */

    fitvals = cpl_vector_new(1);

    xmax = 0.0;
    ymax = -DBL_EPSILON;
    zmax = 0.0;
    for (j=1; j <= POLY_SIZE; j++) {
        const cpl_size degree = j - 1;
        cpl_test_zero( cpl_matrix_set_size(samppos, 1, j) );
        cpl_test_zero( cpl_vector_set_size(fitvals,    j) );
        for (i=0; i < j; i++) {
            cpl_test_zero( cpl_matrix_set(samppos, 0, i, (double)i) );
            cpl_test_zero( cpl_vector_set(fitvals, i, sqrt((double)i)) );
        }

        code = cpl_polynomial_fit_cmp(poly1a, samppos, &symsamp, fitvals, NULL,
                                   CPL_TRUE, &zerodeg, &degree);

        if (code == CPL_ERROR_SINGULAR_MATRIX) {

            cpl_msg_info("FIXME","1D-Polynomial fit of a %" CPL_SIZE_FORMAT
                         "-point dataset with degree %" CPL_SIZE_FORMAT
                         " leads to a (near) singular system of equations",
                         j, degree);
            cpl_test_eq_error(code, CPL_ERROR_SINGULAR_MATRIX);
            break;
        }

        cpl_test_eq_error( code, CPL_ERROR_NONE );

        cpl_test_eq( cpl_polynomial_get_dimension(poly1a), 1 );
        cpl_test_eq( cpl_polynomial_get_degree(poly1a), degree );

        cpl_polynomial_dump(poly1a, stream);

        eps = cpl_vector_get_mse(fitvals, poly1a, samppos, NULL);

        /* The increase in mse must be bound 
           (i686 can manage with a bound of 957 instead of 9016) */
        if (xmax > 0) cpl_test_leq( eps/xmax, 9016);
        xmax = eps;

        eps = cpl_polynomial_get_coeff(poly1a, &zerodeg);
        if (fabs(eps) > fabs(zmax)) {
            cpl_msg_info(cpl_func,"1D-Polynomial with degree %" CPL_SIZE_FORMAT
                         " fit of a %" CPL_SIZE_FORMAT "-point dataset has a "
                         "greater code on P0 than a %" CPL_SIZE_FORMAT
                         "-degree fit to a %" CPL_SIZE_FORMAT "-point dataset: "
                         "fabs(%g) > fabs(%g)",
                         degree, j, degree-1, j-1, eps, zmax);
        }
        /* Should loose at most one decimal digit per degree
           fabs(eps) < DBL_EPSILON * 10 ** (degree-2)  */
        cpl_test_abs( eps, 0.0, DBL_EPSILON * pow(10, (double)(j-3)) );
        zmax = eps;

        /* Compute approximation to sqrt(0.25)
           - this will systematically be too low,
             but less and less so with increasing degree
           - until the approximation goes bad */
        eps = cpl_polynomial_eval_1d(poly1a, 0.25, NULL);

        if (j < 18) cpl_test_abs( eps, 0.5, fabs(0.5 - ymax));
        if (eps <= ymax) {
            /* Should be able to fit at least an 18-degree polynomial
               with increased accuracy - and without code-margin */
            cpl_msg_info(cpl_func, "1D-Polynomial with degree %"
                         CPL_SIZE_FORMAT " fit of a %" CPL_SIZE_FORMAT "-point "
                         "dataset has a greater residual than a %"
                         CPL_SIZE_FORMAT "-degree fit to a %" CPL_SIZE_FORMAT
                         "-point dataset: %g > %g",
                         degree, j, degree-1, j-1, 0.5-eps, 0.5-ymax);
            break;
        }

        ymax = eps;

    }

    /* And the mse itself must be bound */
    cpl_test_leq( eps, 0.411456 * cpl_error_margin);

    /* Test 6: Try to fit increasing degrees to a sqrt() */    

    cpl_test_zero( cpl_matrix_set_size(samppos, 1, VECTOR_SIZE) );
    cpl_test_zero( cpl_vector_set_size(fitvals, VECTOR_SIZE) );
    for (i=0; i < VECTOR_SIZE; i++) {
        cpl_test_zero( cpl_matrix_set(samppos, 0, i, (double)i + 1e4) );
        cpl_test_zero( cpl_vector_set(fitvals, i, sqrt((double)i)) );
    }

    eps = FLT_MAX;
    for (i = 0; i < VECTOR_SIZE-1; i++) {

        xmax = eps;
        code = cpl_polynomial_fit_cmp(poly1a, samppos, &symsamp, fitvals, NULL,
                                   CPL_TRUE, &zerodeg, &i);
        cpl_test_eq_error(code, CPL_ERROR_NONE);

        cpl_test_eq( cpl_polynomial_get_dimension(poly1a), 1 );
        cpl_test_eq( cpl_polynomial_get_degree(poly1a), i );

        eps = cpl_vector_get_mse(fitvals, poly1a, samppos, NULL);

        if (eps > xmax) {
            /* Should be able to fit at least a 8-degree polynomial
               with no degradation - and without code-margin
               (i686 can manage one degree more) */
            if (i < 9) cpl_test_leq( eps, xmax);
            cpl_msg_info(cpl_func, "1D-Polynomial with degree %"
                         CPL_SIZE_FORMAT " fit of a %d-point "
                         "dataset has a higher mean square error than a %"
                         CPL_SIZE_FORMAT "-degree fit: %g > %g",
                         i, VECTOR_SIZE, i-1, eps, xmax);
            break;
        }

    }

    /* Test 7A: Fit a 2nd degree polynomial to a parabola, using mindeg */

    cpl_vector_delete(fitvals);
    CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual);
    fitvals = cpl_vector_wrap(6, (double*)dsq6); /* Not modified */
    CPL_DIAG_PRAGMA_POP;

    mindeg = 1;

    code = cpl_polynomial_fit_cmp(poly1a, samppos1, NULL, fitvals, NULL,
                               CPL_TRUE, &mindeg, &sqdeg);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_eq( cpl_polynomial_get_dimension(poly1a), 1 );
    cpl_test_eq( cpl_polynomial_get_degree(poly1a), sqdeg );

    cpl_test_abs( cpl_polynomial_get_coeff(poly1a, &zerodeg), 0.0, DBL_EPSILON);
    cpl_test_abs( cpl_polynomial_get_coeff(poly1a, &mindeg),  0.0,
                  6.0 * DBL_EPSILON);
    cpl_test_abs( cpl_polynomial_get_coeff(poly1a,  &sqdeg),  1.0, DBL_EPSILON);

    /* Test 7A: Fit a 2nd degree polynomial to a parabola, using mindeg */

    code = cpl_polynomial_fit_cmp(poly1a, samppos1, NULL, fitvals, NULL,
                               CPL_TRUE, &sqdeg, &sqdeg);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_eq( cpl_polynomial_get_dimension(poly1a), 1 );
    cpl_test_eq( cpl_polynomial_get_degree(poly1a), sqdeg );

    cpl_test_abs( cpl_polynomial_get_coeff(poly1a, &zerodeg), 0.0, DBL_EPSILON);
    cpl_test_abs( cpl_polynomial_get_coeff(poly1a, &mindeg),  0.0, DBL_EPSILON);
    cpl_test_abs( cpl_polynomial_get_coeff(poly1a,  &sqdeg),  1.0, DBL_EPSILON);


    cpl_polynomial_test_delete(poly1a);
    cpl_polynomial_test_delete(poly1b);
    cpl_polynomial_test_delete(poly2);
    (void)cpl_matrix_unwrap(samppos1);
    (void)cpl_vector_unwrap(taylor);
    (void)cpl_matrix_unwrap(samppos14);
    (void)cpl_vector_unwrap(taylor4);
    cpl_matrix_delete(samppos);
    cpl_vector_unwrap(fitvals);

    return;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief      Get the mean squared code from a vector of residuals
  @param fitvals   The fitted values
  @param fit       The fitted polynomial
  @param samppos   The sampling points
  @param prechisq  If non-NULL, the reduced chi square
  @return  the mse, or negative on NULL input

 */
/*----------------------------------------------------------------------------*/
static double cpl_vector_get_mse(const cpl_vector     * fitvals,
                                 const cpl_polynomial * fit,
                                 const cpl_matrix     * samppos,
                                 double               * prechisq)
{

    const cpl_size np = cpl_vector_get_size(fitvals);
    cpl_vector   * residual;
    double         mse;
    cpl_flops      flops  = 0;
    double         secs   = 0.0;

    cpl_ensure(fitvals != NULL, CPL_ERROR_NULL_INPUT, -1.0);
    cpl_ensure(fit     != NULL, CPL_ERROR_NULL_INPUT, -2.0);
    cpl_ensure(samppos != NULL, CPL_ERROR_NULL_INPUT, -3.0);

    residual = cpl_vector_new(1+np/2); /* Just to test the resizing... */

    flops = cpl_tools_get_flops();
    secs  = cpl_test_get_cputime();

    cpl_vector_fill_polynomial_fit_residual(residual, fitvals, NULL, fit,
                                            samppos, prechisq);

    mse_secs += cpl_test_get_cputime() - secs;
    mse_flops += cpl_tools_get_flops() - flops;

    mse = cpl_vector_product(residual, residual) / (double)np;
    cpl_vector_delete(residual);

    return mse;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief   Fit and compare to a higher dimension fit with a zero-degree
  @param  self    Polynomial of dimension d to hold the coefficients
  @param  samppos Matrix of p sample positions, with d rows and p columns
  @param  sampsym NULL, or d booleans, true iff the sampling is symmetric
  @param  fitvals Vector of the p values to fit
  @param  fitsigm Uncertainties of the sampled values, or NULL for all ones
  @param  dimdeg  True iff there is a fitting degree per dimension
  @param  mindeg  Pointer to 1 or d minimum fitting degree(s), or NULL
  @param  maxdeg  Pointer to 1 or d maximum fitting degree(s), at least mindeg
  @return CPL_ERROR_NONE on success, else the relevant #_cpl_error_code_
  @see cpl_polynomial_fit()

 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_polynomial_fit_cmp(cpl_polynomial    * self,
                                      const cpl_matrix  * samppos,
                                      const cpl_boolean * sampsym,
                                      const cpl_vector  * fitvals,
                                      const cpl_vector  * fitsigm,
                                      cpl_boolean         dimdeg,
                                      const cpl_size    * mindeg,
                                      const cpl_size    * maxdeg)
{

    const cpl_error_code code = cpl_polynomial_fit(self, samppos, sampsym,
                                                   fitvals, fitsigm, dimdeg,
                                                   mindeg, maxdeg);
    if (code == CPL_ERROR_NONE && self != NULL) {
        cpl_polynomial_fit_cmp_(self, samppos, sampsym, fitvals,
                                fitsigm, dimdeg, mindeg, maxdeg);
    }

    return cpl_error_set_(code); /* Tested and reset by caller */
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief   Fit and compare to a higher dimension fit with a zero-degree
  @param  self    Polynomial of dimension d to hold the coefficients
  @param  samppos Matrix of p sample positions, with d rows and p columns
  @param  sampsym NULL, or d booleans, true iff the sampling is symmetric
  @param  fitvals Vector of the p values to fit
  @param  fitsigm Uncertainties of the sampled values, or NULL for all ones
  @param  dimdeg  True iff there is a fitting degree per dimension
  @param  mindeg  Pointer to 1 or d minimum fitting degree(s), or NULL
  @param  maxdeg  Pointer to 1 or d maximum fitting degree(s), at least mindeg
  @return void
  @see cpl_polynomial_fit_cmp()

 */
/*----------------------------------------------------------------------------*/
void cpl_polynomial_fit_cmp_(const cpl_polynomial * self,
                             const cpl_matrix  * samppos,
                             const cpl_boolean * sampsym,
                             const cpl_vector  * fitvals,
                             const cpl_vector  * fitsigm,
                             cpl_boolean         dimdeg,
                             const cpl_size    * mindeg,
                             const cpl_size    * maxdeg)
{
    const cpl_error_code code = CPL_ERROR_NONE;
    const cpl_size mdim = cpl_polynomial_get_dimension(self);
    const cpl_size ndim = 1 + mdim;
    cpl_polynomial * self1p = cpl_polynomial_new(ndim);

    /* This is quaranteed to fail */
    const cpl_error_code error1e = cpl_polynomial_fit(self1p, samppos,
                                                      sampsym, fitvals,
                                                      fitsigm, dimdeg,
                                                      mindeg, maxdeg);
    cpl_test_error(error1e);
    cpl_test(error1e == code || error1e == CPL_ERROR_INCOMPATIBLE_INPUT);

    if (samppos != NULL && (mdim == 1 || dimdeg == CPL_TRUE)) {
        cpl_polynomial * zeropol = cpl_polynomial_new(mdim);
        const cpl_size np = cpl_matrix_get_ncol(samppos);
        const cpl_size nc = cpl_matrix_get_nrow(samppos);
        cpl_matrix * samppos1p = cpl_matrix_new(1 + nc, np);

        cpl_boolean * sampsym1p = sampsym
            ? (cpl_boolean *)cpl_malloc((size_t)ndim * sizeof(*sampsym1p))
            : NULL;
        cpl_size * mindeg1p = mindeg
            ? (cpl_size*)cpl_malloc((size_t)ndim * sizeof(*mindeg1p))
            : NULL;
        cpl_size * maxdeg1p = maxdeg
            ? (cpl_size*)cpl_malloc((size_t)ndim * sizeof(*maxdeg1p))
            : NULL;

        for (cpl_size idim = 0; idim < ndim; idim++) {
            cpl_error_code error1p;
            cpl_size i, j;

            /* First copy all rows to new matrix, leaving one with zeros */
            for (j = 0; j < idim; j++) {
                if (j <= nc) {
                    for (i = 0; i < np; i++) {
                        cpl_matrix_set(samppos1p, j, i,
                                       cpl_matrix_get(samppos, j, i));
                    }
                }
                if (sampsym1p != NULL) sampsym1p[j] = sampsym[j];
                if (mindeg1p  != NULL) mindeg1p [j] = mindeg [j];
                if (maxdeg1p  != NULL) maxdeg1p [j] = maxdeg [j];
            }
            if (j <= nc) {
                for (i = 0; i < np; i++) {
                    cpl_matrix_set(samppos1p, j, i, 0.0);
                }
            }
            if (sampsym1p != NULL) sampsym1p[j] = CPL_TRUE;
            if (mindeg1p  != NULL) mindeg1p [j] = 0;
            if (maxdeg1p  != NULL) maxdeg1p [j] = 0;

            for (j++; j < ndim; j++) {
                if (j <= nc) {
                    for (i = 0; i < np; i++) {
                        cpl_matrix_set(samppos1p, j, i,
                                       cpl_matrix_get(samppos, j-1, i));
                    }
                }
                if (sampsym1p != NULL) sampsym1p[j] = sampsym[j-1];
                if (mindeg1p  != NULL) mindeg1p [j] = mindeg [j-1];
                if (maxdeg1p  != NULL) maxdeg1p [j] = maxdeg [j-1];
            }

            error1p = cpl_polynomial_fit(self1p, samppos1p,
                                         sampsym1p, fitvals,
                                         fitsigm, CPL_TRUE,
                                         mindeg1p, maxdeg1p);
            if (ndim != 2) {
                cpl_test_eq_error(error1p, CPL_ERROR_UNSUPPORTED_MODE);
            } else {
                const cpl_size degree = cpl_polynomial_get_degree(self);
                if (!code && error1p == CPL_ERROR_SINGULAR_MATRIX &&
                    1 + degree == np && np >= 20) {

                    cpl_test_eq_error(error1p, CPL_ERROR_SINGULAR_MATRIX);

                    /* In the non-overdetermined case, the multi-variate fit
                       is not as accurate as the uni-variate,
                       for degree 20 the difference is a failure */

                    cpl_msg_debug(cpl_func, "2D-fail(%d:%" CPL_SIZE_FORMAT
                                  ":%" CPL_SIZE_FORMAT ":%" CPL_SIZE_FORMAT
                                  "): S: %" CPL_SIZE_FORMAT "x %"
                                  CPL_SIZE_FORMAT, code, idim, maxdeg1p[0],
                                  maxdeg1p[1], 1 + nc, np);
                    if (cpl_msg_get_level() <= CPL_MSG_DEBUG)
                        cpl_matrix_dump(samppos1p, stdout);
                } else if (!error1p && !code) {
                    const cpl_size i0 = 0;
                    const double k0 = cpl_polynomial_get_coeff(self, &i0);
                    cpl_polynomial * self0p
                        = cpl_polynomial_extract(self1p, idim, zeropol);
                    const double mytol = mindeg != NULL && mindeg[0] > 0
                        ? 6.0 : 1.0;

                    /* FIXME: Need relative polynomial comparison */
                    cpl_test_polynomial_abs(self, self0p, CX_MAX(fabs(k0),1)
                                            * pow(10.0, (double)degree)
                                            * mytol * DBL_EPSILON);
                    cpl_polynomial_test_delete(self0p);


CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual);
                    if (mindeg != NULL) {
                        double           mse = 0.0;
                        cpl_polynomial * self1d = cpl_polynomial_new(1);
                        cpl_vector     * x_pos =
                            cpl_vector_wrap(np,
                                            cpl_matrix_get_data((cpl_matrix *)samppos1p));
                        cpl_error_code code1d;
CPL_DIAG_PRAGMA_POP;
                        code1d = cpl_polynomial_fit_1d(self1d, x_pos, fitvals,
                                                     mindeg[0], maxdeg[0],
                                                     sampsym ? sampsym[0] : CPL_FALSE,
                                                     &mse);
                        if (code1d == CPL_ERROR_SINGULAR_MATRIX) {
                            cpl_test_eq_error(code1d, CPL_ERROR_SINGULAR_MATRIX);
                        } else if (code1d == CPL_ERROR_DIVISION_BY_ZERO) {
                            cpl_test_eq_error(code1d, CPL_ERROR_DIVISION_BY_ZERO);
                        } else {
                            cpl_test_eq_error(code1d, CPL_ERROR_NONE);
                            cpl_test_polynomial_abs(self1d, self, (double)degree
                                                    * mytol * DBL_EPSILON);
                            cpl_msg_info(cpl_func, "1D %d-degree fit to %d "
                                         "points has mean square error: %g",
                                         (int)maxdeg[0], (int)np, mse);
                        }
                        (void)cpl_vector_unwrap(x_pos);
                        cpl_polynomial_test_delete(self1d);
                    }
                } else if (!error1p && code == CPL_ERROR_SINGULAR_MATRIX) {
                    cpl_test_error(CPL_ERROR_NONE);
                    cpl_msg_warning(cpl_func, "Adding a dummy-dimension to "
                                    "%" CPL_SIZE_FORMAT "-degree, %"
                                    CPL_SIZE_FORMAT " dimension polynomial "
                                    "removes matrix singularity",
                                    degree, mdim);
                } else {
                    cpl_test_eq_error(error1p, code);
                }
            }

        }

        cpl_matrix_delete(samppos1p);
        cpl_polynomial_test_delete(zeropol);
        cpl_free(sampsym1p);
        cpl_free(mindeg1p);
        cpl_free(maxdeg1p);
    }

    cpl_polynomial_test_delete(self1p);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Multiply a 1D-polynomial by (x-root), increasing its degree by 1
  @param    self  The 1D-polynomial to modify
  @param    root  The new root to use for the extension
  @return   CPL_ERROR_NONE or the relevant CPL error code.
  
 */
/*----------------------------------------------------------------------------*/
static
cpl_error_code cpl_polynomial_multiply_1d(cpl_polynomial * self, double root)
{

    cpl_polynomial * poly1d;
    cpl_size degree, i = 0;
    cpl_error_code code;

    cpl_ensure_code(self  != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(cpl_polynomial_get_dimension(self) == 1,
                     CPL_ERROR_ILLEGAL_INPUT);

    degree = cpl_polynomial_get_degree(self);
    cpl_ensure_code(degree > 0 || cpl_polynomial_get_coeff(self, &i) != 0.0,
                    CPL_ERROR_DATA_NOT_FOUND);

    poly1d = cpl_polynomial_new(1);

    i = 0;
    code = cpl_polynomial_set_coeff(poly1d, &i, -root);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    i = 1;
    code = cpl_polynomial_set_coeff(poly1d, &i, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_polynomial_multiply(self, self, poly1d);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_polynomial_test_delete(poly1d);

    cpl_ensure_code(cpl_polynomial_get_degree(self) == degree + 1,
                    CPL_ERROR_UNSPECIFIED);

    return CPL_ERROR_NONE;
}



/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Test solving for the specified root, when multiplied onto a polynomial
  @param  self    1D-Polynomial multiplied by (x-root)
  @param  root    The root to solve for
  @param  xfirst  The supplied 1st guess
  @param  mmulti  The root multiplicity
  @param  xtol    The acceptable tolerance on the solution
  @param  restol  The acceptable residual
  @return void
  @see cpl_polynomial_solve_1d()

 */
/*----------------------------------------------------------------------------*/
static void cpl_polynomial_solve_1d_test_ok(const cpl_polynomial * self,
                                            double root, double xfirst,
                                            cpl_size nmulti,
                                            double xtol, double restol)
{

    cpl_polynomial * copy;
    cpl_error_code code;


    copy = cpl_polynomial_duplicate(self);
    cpl_test_error(CPL_ERROR_NONE);

    code = cpl_polynomial_multiply_1d(copy, root);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    cpl_test_eq(cpl_polynomial_get_degree(copy),
                cpl_polynomial_get_degree(self) + 1);

    for (int itry = 0; itry < 2; itry++) {
        double x, eps;

        code = cpl_polynomial_solve_1d(copy, xfirst, &x, nmulti);
        cpl_test_eq_error(code, CPL_ERROR_NONE);

        cpl_test_abs(x, root, xtol);

        eps = cpl_polynomial_eval_1d(copy, x, NULL);
        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_abs( 0.0, eps, restol);

        /* Solve a second time, using the solution as a first guess.
           This will test the stopping criterion in the first iteration */
        xfirst  = x;
        restol *= restol;
    }

    cpl_polynomial_test_delete(copy);
}

static void cpl_polynomial_fit_3(FILE * stream) {
   /* Speed [km/h] */
   const double v0[] = {55, 66, 77, 88, 99, 110, 121, 132, 143, 154, 165, 176, 182, 187, 193, 204, 209, 220};
   /* Force [Wh/km] */
   const double F0[] = {79, 87, 97, 110, 126, 144, 164, 186, 211, 238, 267, 298, 315, 332, 349, 386, 405, 445};
   const size_t n = sizeof(v0)/sizeof(v0[0]);
   double v1[n], P1[n];

    cpl_polynomial  * fit1d     = cpl_polynomial_new(1);
    cpl_polynomial  * deriv     = NULL;
    cpl_polynomial  * vderiv    = NULL;
    cpl_polynomial  * solver    = cpl_polynomial_new(1);
    cpl_matrix      * samppos1d = cpl_matrix_wrap(1, n, v1);
    cpl_vector      * fitvals   = cpl_vector_wrap(n, P1);
    const cpl_boolean sampsym   = CPL_FALSE;
    const cpl_size    maxdeg1d  = 3; // Fit 4 coefficients
    cpl_error_code    code;
    const double powers[] = {3000, 11000, 22000, 36000, 50000, 80000, 117000, 120000, 180000};
    const size_t npow = sizeof(powers)/sizeof(powers[0]);

    const cpl_size    mplot = 200;
    cpl_bivector    * vvplot = cpl_bivector_new(mplot);
    cpl_vector      * v0plot = cpl_bivector_get_x(vvplot);
    cpl_vector      * v1plot = cpl_bivector_get_y(vvplot);
    const double vmin = 0/3.6;   /* m/s */
    const double vmax = 250/3.6;
    double vi1, pi1, vi2, pi2;
    const cpl_boolean is_debug = cpl_msg_get_level() == CPL_MSG_DEBUG;

    const char * filei = "optispeed.txt";
    FILE*  sfilei = fopen(is_debug ? filei : "/dev/null", "w");

    const char * file0 = "power0.txt";
    FILE*  sfile0 = fopen(is_debug ? file0 : "/dev/null", "w");

    const char * file1 = "power1.txt";
    FILE*  sfile1 = fopen(is_debug ? file1 : "/dev/null", "w");

    const char * filep = "power.txt";
    FILE*  sfilep = fopen(is_debug ? filep : "/dev/null", "w");

    for (size_t i = 0; i < n; i++) {
        v1[i] = v0[i] / 3.6; /* Convert km/h to m/s */
        P1[i] = F0[i] * v0[i]; /* Convert Force to Power [W] */
    }

    code =
        cpl_polynomial_fit(fit1d, samppos1d, &sampsym, fitvals, NULL,
                           CPL_FALSE, NULL, &maxdeg1d);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    cpl_msg_info(cpl_func, "Fitting %d-degree to %d values:", (int)maxdeg1d,
                 (int)n);
    cpl_polynomial_dump(fit1d, stream);

    deriv = cpl_polynomial_duplicate(fit1d);
    code = cpl_polynomial_derivative(deriv, 0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    vderiv = cpl_polynomial_duplicate(deriv);
    code = cpl_polynomial_multiply_1d(vderiv, 0.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_polynomial_subtract(solver, fit1d, vderiv);
    cpl_polynomial_dump(solver, stream);

    vi1 = 120.0 / 3.6;
    pi1 = cpl_polynomial_eval_1d(fit1d, vi1, NULL);
    vi2 = 140.0 / 3.6;
    pi2 = cpl_polynomial_eval_1d(fit1d, vi2, NULL);

    cpl_msg_info(cpl_func, "P(120): %g, P(140): %g, ratio: %g",
                 pi1, pi2, pi2/pi1);

    for (cpl_size i = 0; i < (cpl_size)n; i++) {
        const double v = v0[i];
        const double p = P1[i];

        fprintf(sfile0, "%g %g\n", v, p / 1e3);
    }
    fclose(sfile0);

    for (cpl_size i = 0; i < mplot; i++) {
        const double v = 240.0 / 3.6 * i / (mplot - 1.0);
        const double f = cpl_polynomial_eval_1d(deriv, v, NULL);

        fprintf(sfile1, "%g %g\n", v * 3.6, f / 1e3);
    }
    fclose(sfile1);
   
    for (cpl_size i = 0; i < mplot; i++) {
        const double v = 240.0 / 3.6 * i / (mplot - 1.0);
        const double p = cpl_polynomial_eval_1d(fit1d, v, NULL);

        fprintf(sfilep, "%g %g\n", v * 3.6, p / 1e3);
    }
    fclose(sfilep);
   
    for (cpl_size i = 0; i < mplot/4; i++) {
        const double poweri = 300 + 240000 * i / (mplot/4 - 1.0);
        const cpl_size    i0 = 0;
        const double c0 = cpl_polynomial_get_coeff(solver, &i0);
        cpl_error_code code1 = cpl_polynomial_set_coeff(solver, &i0,
                                                        c0 + poweri);
        double vc, pc, ve;

        cpl_test_eq_error(code1, CPL_ERROR_NONE);

        code1 = cpl_polynomial_solve_1d(solver, 20, &vc, 1);
        cpl_test_eq_error(code1, CPL_ERROR_NONE);

        pc = cpl_polynomial_eval_1d(fit1d, vc, NULL);

        ve = vc * poweri / (poweri + pc);

        fprintf(sfilei, "%g %g\n", vc * 3.6, ve * 3.6);

        code1 = cpl_polynomial_set_coeff(solver, &i0, c0);
        cpl_test_eq_error(code1, CPL_ERROR_NONE);

    }
    fclose(sfilei);

    for (cpl_size i = 0; i < (cpl_size)npow; i++) {
        const cpl_size    i0 = 0;
        const double c0 = cpl_polynomial_get_coeff(solver, &i0);
        cpl_error_code code1 = cpl_polynomial_set_coeff(solver, &i0,
                                                        c0 + powers[i]);
        double vc, pc, ve;
        char * file = cpl_sprintf("pc_%d.txt", (int)powers[i]);
        FILE*  sfile = fopen(is_debug ? file : "/dev/null", "w");

        cpl_test_eq_error(code1, CPL_ERROR_NONE);

        code1 = cpl_polynomial_solve_1d(solver, 20, &vc, 1);
        cpl_test_eq_error(code1, CPL_ERROR_NONE);

        pc = cpl_polynomial_eval_1d(fit1d, vc, NULL);

        ve = vc * powers[i] / (powers[i] + pc);

        cpl_msg_info(cpl_func,
                     "Power %g kW: Vcar=%.1f km/h "
                     "Veff=%.1f km/h Pcar=%.1f kW F=%.0f Wh/km",
                     powers[i]/1e3, vc * 3.6, ve * 3.6,
                     pc/1e3, pc/vc/3.6);

        code1 = cpl_polynomial_set_coeff(solver, &i0, c0);
        cpl_test_eq_error(code1, CPL_ERROR_NONE);

        for (cpl_size j = 0; j < mplot; j++) {
            const double u0 = vmin + (vmax - vmin) * j / (mplot - 1.0);
            const double p0 = cpl_polynomial_eval_1d(fit1d, u0, NULL);
            const double u1 = u0 * powers[i] / (powers[i] + p0);

            cpl_vector_set(v0plot, j, u0 * 3.6);
            cpl_vector_set(v1plot, j, u1 * 3.6);
        }
        cpl_bivector_dump(vvplot, sfile);
        cpl_free(file);
        fclose(sfile);
    }

    (void)cpl_matrix_unwrap(samppos1d);
    (void)cpl_vector_unwrap(fitvals);
    cpl_bivector_delete(vvplot);
    cpl_polynomial_test_delete(fit1d);
    cpl_polynomial_test_delete(deriv);
    cpl_polynomial_test_delete(vderiv);
    cpl_polynomial_test_delete(solver);
   
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief      Test the CPL function
  @return    void

 */
/*----------------------------------------------------------------------------*/
static void cpl_polynomial_multiply_test(void)
{
    const double x1 = -1.0;
    cpl_polynomial * poly10 = cpl_polynomial_new(1);
    cpl_polynomial * poly1a = cpl_polynomial_new(1);
    cpl_polynomial * poly1b = cpl_polynomial_new(1);
    cpl_polynomial * poly1c = cpl_polynomial_new(1);
    cpl_polynomial * poly1d = cpl_polynomial_new(1);
    cpl_polynomial * poly1p = cpl_polynomial_new(1); /* Previous */
    cpl_polynomial * poly30 = cpl_polynomial_new(POLY_DIM);
    cpl_polynomial * poly3a = cpl_polynomial_new(POLY_DIM);
    cpl_polynomial * poly3b = cpl_polynomial_new(POLY_DIM);
    cpl_polynomial * poly3c = cpl_polynomial_new(POLY_DIM);
    cpl_polynomial * poly3d = cpl_polynomial_new(POLY_DIM);

    const cpl_size p0[POLY_DIM] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const cpl_size p1[POLY_DIM] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    cpl_error_code code;

    /* Doubler */
    code = cpl_polynomial_set_coeff(poly10, p0, 2.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_polynomial_set_coeff(poly30, p0, 2.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    /* Accumulate Pascal's triangle into the a-polynomial */

    /* Create the polynomial 1, uni- and multi-variate */
    code = cpl_polynomial_set_coeff(poly1a, p0, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_polynomial_set_coeff(poly3a, p0, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    /* Create the polynomial (x + 1), uni- and multi-variate */
    code = cpl_polynomial_set_coeff(poly1b, p0, -x1);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_polynomial_set_coeff(poly1b, p1, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_polynomial_set_coeff(poly3b, p0, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_polynomial_set_coeff(poly3b, p1, -x1);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    for (cpl_size i = 0; i < POLY_SIZE; i++) {
        double x;
        cpl_size degree;

        code = cpl_polynomial_copy(poly1p, poly1a);
        cpl_test_eq_error(code, CPL_ERROR_NONE);

        /* Commutative */
        code = cpl_polynomial_multiply(poly1c, poly1a, poly1b);
        cpl_test_eq_error(code, CPL_ERROR_NONE);
        code = cpl_polynomial_multiply(poly1d, poly1b, poly1a);
        cpl_test_eq_error(code, CPL_ERROR_NONE);

        cpl_test_polynomial_abs(poly1c, poly1d, 0.0);

        /* Multiply with 2 */
        code = cpl_polynomial_add(poly1a, poly1d, poly1c);
        cpl_test_eq_error(code, CPL_ERROR_NONE);

        code = cpl_polynomial_multiply(poly1d, poly10, poly1c);
        cpl_test_eq_error(code, CPL_ERROR_NONE);
        cpl_test_polynomial_abs(poly1d, poly1a, 0.0);

        code = cpl_polynomial_multiply(poly1d, poly1c, poly10);
        cpl_test_eq_error(code, CPL_ERROR_NONE);
        cpl_test_polynomial_abs(poly1d, poly1a, 0.0);

        /* Save for next iteration */
        code = cpl_polynomial_copy(poly1a, poly1c);
        cpl_test_eq_error(code, CPL_ERROR_NONE);

        /* Same for multi-variate */
        code = cpl_polynomial_multiply(poly3c, poly3a, poly3b);
        cpl_test_eq_error(code, CPL_ERROR_NONE);

        code = cpl_polynomial_multiply(poly3d, poly3b, poly3a);
        cpl_test_eq_error(code, CPL_ERROR_NONE);
        cpl_test_polynomial_abs(poly3c, poly3d, 0.0);

        code = cpl_polynomial_add(poly3a, poly3d, poly3c);
        cpl_test_eq_error(code, CPL_ERROR_NONE);

        code = cpl_polynomial_multiply(poly3d, poly30, poly3c);
        cpl_test_eq_error(code, CPL_ERROR_NONE);
        cpl_test_polynomial_abs(poly3d, poly3a, 0.0);

        code = cpl_polynomial_multiply(poly3d, poly3c, poly30);
        cpl_test_eq_error(code, CPL_ERROR_NONE);
        cpl_test_polynomial_abs(poly3d, poly3a, 0.0);

        /* Save for next iteration */
        code = cpl_polynomial_copy(poly3a, poly3c);
        cpl_test_eq_error(code, CPL_ERROR_NONE);

        /* Degree increases with each multiplication */
        degree = cpl_polynomial_get_degree(poly1a);
        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_eq(degree, i + 1);

        degree = cpl_polynomial_get_degree(poly3a);
        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_eq(degree, i + 1);

        for (cpl_size j = 0; j <= degree; j++) {
            /* Dimension independent */
            const cpl_size pi[POLY_DIM] = {j, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
            const double c1 = cpl_polynomial_get_coeff(poly1a, pi);
            const double c3 = cpl_polynomial_get_coeff(poly3a, pi);

            cpl_test_error(CPL_ERROR_NONE);

            cpl_test_abs(c1, c3, 0.0);

            /* Verify Pascal's triangle: Each current row element is summed up
               from two above, except the first and last that are unity */
            if (0 < j && j < degree) {
                const cpl_size k = j - 1;
                const double cp = cpl_polynomial_get_coeff(poly1p, &k);
                const double cj = cpl_polynomial_get_coeff(poly1p, &j);

                cpl_test_abs(c1, cp + cj, 0.0);
            } else {
                cpl_test_abs(c1, 1.0, 0.0);
            }
        }

        /* The polynomial must have the root with degree multiplicity */
        code = cpl_polynomial_solve_1d(poly1a, -x1, &x, degree);
        cpl_test_eq_error(code, CPL_ERROR_NONE);
        cpl_test_abs(x, x1, 0.0);

    }

    cpl_polynomial_test_delete(poly10);
    cpl_polynomial_test_delete(poly1a);
    cpl_polynomial_test_delete(poly1b);
    cpl_polynomial_test_delete(poly1c);
    cpl_polynomial_test_delete(poly1d);
    cpl_polynomial_test_delete(poly1p);
    cpl_polynomial_test_delete(poly30);
    cpl_polynomial_test_delete(poly3a);
    cpl_polynomial_test_delete(poly3b);
    cpl_polynomial_test_delete(poly3c);
    cpl_polynomial_test_delete(poly3d);
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief      Test the CPL function
  @return    void

 */
/*----------------------------------------------------------------------------*/
static void cpl_polynomial_compare_test(void)
{
    cpl_polynomial * poly1a = cpl_polynomial_new(1);
    cpl_polynomial * poly1b = cpl_polynomial_new(1);
    cpl_polynomial * poly3a = cpl_polynomial_new(POLY_DIM);
    cpl_polynomial * poly3b = cpl_polynomial_new(POLY_DIM);
    cpl_error_code code;
    int retval;

    /* Illegal input */
    retval = cpl_polynomial_compare(poly1a, NULL, 0.0);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_lt(retval, 0);

    retval = cpl_polynomial_compare(NULL, poly1a, 0.0);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_lt(retval, 0);

    retval = cpl_polynomial_compare(poly1a, poly1b, -1.0);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_lt(retval, 0);

    /* OK, on zero-polynomials */
    retval = cpl_polynomial_compare(poly1a, poly1b, 0.0);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(retval);

    retval = cpl_polynomial_compare(poly3a, poly3b, 0.0);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(retval);

    /* Fail, on zero-polynomials */
    retval = cpl_polynomial_compare(poly1a, poly3b, 0.0);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq(retval, POLY_DIM - 1);

    for (cpl_size dim = 1; dim <= POLY_DIM; dim++) {
        for (cpl_size degree = 0; degree < 4; degree++) {
            const cpl_size p0[POLY_DIM] = {degree, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

            cpl_polynomial_empty(poly1a);
            cpl_polynomial_empty(poly1b);
            cpl_polynomial_empty(poly3a);
            cpl_polynomial_empty(poly3b);

            /* non-zero 1D */
            code = cpl_polynomial_set_coeff(poly1a, p0, 0.5);
            cpl_test_eq_error(code, CPL_ERROR_NONE);

            retval = cpl_polynomial_get_degree(poly1a);
            cpl_test_eq_error(code, CPL_ERROR_NONE);
            cpl_test_eq(retval, degree);

            retval = cpl_polynomial_get_degree(poly1b);
            cpl_test_eq_error(code, CPL_ERROR_NONE);
            cpl_test_zero(retval);

            retval = cpl_polynomial_compare(poly1a, poly1b, 0.0);
            cpl_test_error(CPL_ERROR_NONE);
            cpl_test_eq(retval, degree + 1);

            retval = cpl_polynomial_compare(poly1b, poly1a, 0.0);
            cpl_test_error(CPL_ERROR_NONE);
            cpl_test_eq(retval, degree + 1);

            retval = cpl_polynomial_compare(poly1a, poly1b, 0.75);
            cpl_test_error(CPL_ERROR_NONE);
            cpl_test_zero(retval);

            code = cpl_polynomial_set_coeff(poly1b, p0, -0.125);
            cpl_test_eq_error(code, CPL_ERROR_NONE);

            retval = cpl_polynomial_compare(poly1a, poly1b, 0.75);
            cpl_test_error(CPL_ERROR_NONE);
            cpl_test_zero(retval);

            retval = cpl_polynomial_compare(poly1a, poly1b, 0.5);
            cpl_test_error(CPL_ERROR_NONE);
            cpl_test_eq(retval, degree + 1);

            /* non-zero 3D */
            code = cpl_polynomial_set_coeff(poly3a, p0, 0.5);
            cpl_test_eq_error(code, CPL_ERROR_NONE);

            retval = cpl_polynomial_compare(poly3a, poly3b, 0.0);
            cpl_test_error(CPL_ERROR_NONE);
            cpl_test_eq(retval, degree + 1);

            retval = cpl_polynomial_compare(poly3a, poly3b, 0.75);
            cpl_test_error(CPL_ERROR_NONE);
            cpl_test_zero(retval);

            code = cpl_polynomial_set_coeff(poly3b, p0, -0.125);
            cpl_test_eq_error(code, CPL_ERROR_NONE);

            retval = cpl_polynomial_compare(poly3a, poly3b, 0.75);
            cpl_test_error(CPL_ERROR_NONE);
            cpl_test_zero(retval);

            retval = cpl_polynomial_compare(poly3a, poly3b, 0.5);
            cpl_test_error(CPL_ERROR_NONE);
            cpl_test_eq(retval, degree + 1);
        }
    }

    cpl_polynomial_test_delete(poly1a);
    cpl_polynomial_test_delete(poly1b);
    cpl_polynomial_test_delete(poly3a);
    cpl_polynomial_test_delete(poly3b);

}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief      Test the CPL function
  @return    void

 */
/*----------------------------------------------------------------------------*/
static void cpl_polynomial_multiply_test_2d(void)
{

    cpl_polynomial * poly2r = cpl_polynomial_new(2);
    cpl_polynomial * poly2a = cpl_polynomial_new(2);
    cpl_polynomial * poly2b = cpl_polynomial_new(2);
    cpl_polynomial * poly2c = cpl_polynomial_new(2);
    cpl_size         expo[2], degree;
    double           value;
    int              icmp;
    cpl_error_code   code;

    expo[0] = 0; expo[1] = 0;
    code = cpl_polynomial_set_coeff(poly2a, expo, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_polynomial_set_coeff(poly2b, expo, -1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_polynomial_set_coeff(poly2r, expo, -1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    expo[0] = 1; expo[1] = 0;
    code = cpl_polynomial_set_coeff(poly2a, expo, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_polynomial_set_coeff(poly2b, expo, -1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_polynomial_set_coeff(poly2r, expo, -2.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    expo[0] = 1; expo[1] = 1;
    code = cpl_polynomial_set_coeff(poly2a, expo, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_polynomial_set_coeff(poly2b, expo, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    expo[0] = 2; expo[1] = 0;
    code = cpl_polynomial_set_coeff(poly2r, expo, -1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    expo[0] = 2; expo[1] = 2;
    code = cpl_polynomial_set_coeff(poly2r, expo, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    degree = cpl_polynomial_get_degree(poly2r);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq(degree, 4);

    code = cpl_polynomial_multiply(poly2c, poly2a, poly2b);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    
    icmp = cpl_polynomial_compare(poly2c, poly2r, 0.0);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(icmp);
    
    code = cpl_polynomial_multiply(poly2c, poly2b, poly2a);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    
    icmp = cpl_polynomial_compare(poly2c, poly2r, 0.0);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(icmp);
    
    code = cpl_polynomial_multiply(poly2b, poly2b, poly2a);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    
    icmp = cpl_polynomial_compare(poly2b, poly2r, 0.0);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(icmp);

    code = cpl_polynomial_subtract(poly2a, poly2r, poly2b);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    degree = cpl_polynomial_get_degree(poly2a);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(degree);

    expo[0] = 0; expo[1] = 0;
    value = cpl_polynomial_get_coeff(poly2a, expo);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_abs(value, 0.0, 0.0);
    
    cpl_polynomial_test_delete(poly2a);
    cpl_polynomial_test_delete(poly2b);
    cpl_polynomial_test_delete(poly2c);
    cpl_polynomial_test_delete(poly2r);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Test the CPL function
  @param    stream  A stream to dump to
  @return   void

 */
/*----------------------------------------------------------------------------*/
static void cpl_polynomial_shift_test_1d(FILE * stream)
{

    cpl_polynomial * poly1 = cpl_polynomial_new(1);
    cpl_polynomial * poly2;
    /* Some binomial coefficients */
    double           p15[BINOMIAL_SZ] = {1,15,105,455,1365,3003,5005,6435};
    cpl_size         deg1;
    const cpl_size   i8  = 8;
    const cpl_size   i15 = 2 * i8 - 1;
    cpl_size         i;
    /* -1 is a root with multiplicity 8 */
    const double     x1 = -1.0;
    cpl_error_code   code;

    /* An 15-degree monomial */
    code = cpl_polynomial_set_coeff(poly1, &i15, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    deg1 = cpl_polynomial_get_degree(poly1);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq(deg1, i15);

    poly2 = cpl_polynomial_duplicate(poly1);
    code = cpl_polynomial_shift_1d(poly2, 0, -x1) ;
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    /* poly2 now holds the binomial coefficients for n = 15 */

    code = cpl_polynomial_dump(poly2, stream);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    for (i = 0; i < i8; i++) {
        const cpl_size j = i15 - i;

        code = cpl_polynomial_set_coeff(poly1, &i, p15[i]);
        cpl_test_eq_error(code, CPL_ERROR_NONE);
        code = cpl_polynomial_set_coeff(poly1, &j, p15[i]);
        cpl_test_eq_error(code, CPL_ERROR_NONE);
    }

    deg1 = cpl_polynomial_get_degree(poly1);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq(deg1, i15);

    for (i = 0; i < i8; i++) {
        const cpl_size j = i15 - i;

        const double z1 = cpl_polynomial_get_coeff(poly1, &i);
        const double z2 = cpl_polynomial_get_coeff(poly2, &i);
        const double z  = cpl_polynomial_get_coeff(poly2, &j);

        cpl_test_rel( z1, z2, DBL_EPSILON);
        cpl_test_rel( z,  z2, DBL_EPSILON);
    }

    cpl_test_polynomial_abs(poly1, poly2, 0.0);

    cpl_polynomial_test_delete(poly1);
    cpl_polynomial_test_delete(poly2);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Test the CPL function
  @param    stream  A stream to dump to
  @return   void

 */
/*----------------------------------------------------------------------------*/
static void cpl_polynomial_shift_test_2d_one(FILE * stream)
{
    cpl_polynomial * poly1 = cpl_polynomial_new(2);
    cpl_polynomial * poly2;
    cpl_size         expo[2];
    cpl_error_code   code;

    expo[1] = 2;

    expo[0] = 0;
    code = cpl_polynomial_set_coeff(poly1, expo, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    expo[0] = 1;
    code = cpl_polynomial_set_coeff(poly1, expo, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    expo[0] = 2;
    code = cpl_polynomial_set_coeff(poly1, expo, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    poly2 = cpl_polynomial_duplicate(poly1);

    /* Shift back and forth, creates zero-polynomials (temporarily) */
    code = cpl_polynomial_shift_1d(poly1, 1, 1.0);
    code = cpl_polynomial_shift_1d(poly1, 1, -1.0);

    code = cpl_polynomial_dump(poly1, stream);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_polynomial_abs(poly1, poly2, 0.0);

    cpl_polynomial_test_delete(poly1);
    cpl_polynomial_test_delete(poly2);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Test the CPL function
  @param    stream  A stream to dump to
  @return   void

 */
/*----------------------------------------------------------------------------*/
static void cpl_polynomial_shift_test_3d_one(FILE * stream)
{
    cpl_polynomial * poly1 = cpl_polynomial_new(3);
    cpl_polynomial * poly2;
    cpl_size         expo[3];
    cpl_error_code   code;
    const cpl_error_code ecode = CPL_ERROR_NONE;

    expo[2] = 2;

    expo[1] = expo[0] = 0;
    code = cpl_polynomial_set_coeff(poly1, expo, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    expo[0] = 1;
    code = cpl_polynomial_set_coeff(poly1, expo, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    expo[0] = 2;
    code = cpl_polynomial_set_coeff(poly1, expo, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    poly2 = cpl_polynomial_duplicate(poly1);

    /* Shift back and forth, creates zero-polynomials (temporarily) */
    code = cpl_polynomial_shift_1d(poly1, 0, 1.0);
    cpl_test_eq_error(code, ecode);
    code = cpl_polynomial_shift_1d(poly1, 1, 1.0);
    cpl_test_eq_error(code, ecode);
    code = cpl_polynomial_shift_1d(poly1, 2, 1.0);
    cpl_test_eq_error(code, ecode);
    code = cpl_polynomial_shift_1d(poly1, 2, -1.0);
    cpl_test_eq_error(code, ecode);
    code = cpl_polynomial_shift_1d(poly1, 1, -1.0);
    cpl_test_eq_error(code, ecode);
    code = cpl_polynomial_shift_1d(poly1, 0, -1.0);
    cpl_test_eq_error(code, ecode);

    code = cpl_polynomial_dump(poly1, stream);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_polynomial_abs(poly1, poly2, 0.0);

    cpl_polynomial_test_delete(poly1);
    cpl_polynomial_test_delete(poly2);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Test the CPL function
  @param    stream  A stream to dump to
  @param    idim    Passed to tested function, 0 or 1
  @return   void

 */
/*----------------------------------------------------------------------------*/
static void cpl_polynomial_shift_test_2d(FILE * stream, cpl_size idim)
{

    cpl_polynomial * poly1 = cpl_polynomial_new(2);
    cpl_polynomial * poly2;
    /* Some binomial coefficients */
    double           p15[BINOMIAL_SZ] = {1,15,105,455,1365,3003,5005,6435};
    cpl_size         deg1;
    const cpl_size   i8  = 8;
    const cpl_size   i15 = 2 * i8 - 1;
    cpl_size         i;
    /* -1 is a root with multiplicity 8 */
    const double     x1 = -1.0;
    cpl_error_code   code;

    cpl_test_lt(-1, idim);
    cpl_test_lt(idim, 2);

    for (cpl_size k = 0; k < i8; k++) {
        const cpl_size   expo1[2] = {idim ? k : i15, idim ? i15 : k};

        /* An 15-degree monomial */
        code = cpl_polynomial_set_coeff(poly1, expo1, 1.0);
        cpl_test_eq_error(code, CPL_ERROR_NONE);
    }

    deg1 = cpl_polynomial_get_degree(poly1);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq(deg1, i15 + i8 - 1);

    poly2 = cpl_polynomial_duplicate(poly1);
    code = cpl_polynomial_shift_1d(poly2, idim, -x1) ;
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    /* poly2 now holds the binomial coefficients for n = 15 */

    code = cpl_polynomial_dump(poly2, stream);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    for (cpl_size k = 0; k < i8; k++) {
        for (i = 0; i < i8; i++) {
            const cpl_size j = i15 - i;
            const cpl_size expoi[2] = {idim ? k : i, idim ? i : k};
            const cpl_size expoj[2] = {idim ? k : j, idim ? j : k};

            code = cpl_polynomial_set_coeff(poly1, expoi, p15[i]);
            cpl_test_eq_error(code, CPL_ERROR_NONE);
            code = cpl_polynomial_set_coeff(poly1, expoj, p15[i]);
            cpl_test_eq_error(code, CPL_ERROR_NONE);
        }
    }

    deg1 = cpl_polynomial_get_degree(poly1);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq(deg1, i15 + i8 -1);

    for (cpl_size k = 0; k < i8; k++) {
        for (i = 0; i < i8; i++) {
            const cpl_size j = i15 - i;
            const cpl_size expoi[2] = {idim ? k : i, idim ? i : k};
            const cpl_size expoj[2] = {idim ? k : j, idim ? j : k};

            const double z1 = cpl_polynomial_get_coeff(poly1, expoi);
            const double z2 = cpl_polynomial_get_coeff(poly2, expoi);
            const double z  = cpl_polynomial_get_coeff(poly2, expoj);

            cpl_test_rel( z1, z2, DBL_EPSILON);
            cpl_test_rel( z,  z2, DBL_EPSILON);
        }
    }

    cpl_test_polynomial_abs(poly1, poly2, 0.0);

    cpl_polynomial_test_delete(poly1);
    cpl_polynomial_test_delete(poly2);

}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Test the CPL function
  @param    stream  A stream to dump to
  @return   void

 */
/*----------------------------------------------------------------------------*/
static void cpl_polynomial_extract_test_3d_one(FILE * stream)
{
    const cpl_size   nc = 3;
    cpl_polynomial * poly1a = cpl_polynomial_new(1);
    cpl_polynomial * poly2a = cpl_polynomial_new(2);
    cpl_polynomial * poly3 = cpl_polynomial_new(3);
    cpl_error_code code;

    /* Coefficients span powers of 2 from 1 to nc^3 */
    for (cpl_size k = 0; k < nc; k++) {
        for (cpl_size j = 0; j < nc; j++) {
            for (cpl_size i = 0; i < nc; i++) {
                const cpl_size expo[3] = {i, j, k};
                const double value = (double)(1 << ((nc * k + j) * nc + i));

                code = cpl_polynomial_set_coeff(poly3, expo, value);
                cpl_test_eq_error(code, CPL_ERROR_NONE);
            }
        }
    }

    code = cpl_polynomial_dump(poly3, stream);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    for (cpl_size ival = -2; ival <= 2; ival++) {
        /* Evaluate at this point */
        double         xval[3] = {(double)ival, (double)ival, (double)ival};
        cpl_vector   * vxyz    = cpl_vector_wrap(3, xval);
        const double   xyz     = cpl_polynomial_eval(poly3, vxyz);
        const cpl_size expo2[2] = {0, 0};

        /* Setup collapsing polynomial2 */
        code = cpl_polynomial_set_coeff(poly2a, expo2, xval[0]);
        cpl_test_eq_error(code, CPL_ERROR_NONE);
        code = cpl_polynomial_set_coeff(poly1a, expo2, xval[0]);
        cpl_test_eq_error(code, CPL_ERROR_NONE);

        /* Collapsing each dimension and then evaluating
           must give to same result*/
        for (cpl_size idim1 = 0; idim1 < 3; idim1++) {
            cpl_polynomial * poly2 = cpl_polynomial_extract(poly3, idim1,
                                                            poly2a);
            for (cpl_size idim2 = 0; idim2 < 2; idim2++) {
                cpl_polynomial * poly1 = cpl_polynomial_extract(poly2, idim2,
                                                                poly1a);

                const double x = cpl_polynomial_eval_1d(poly1, xval[0], NULL);
                const cpl_size nfail = cpl_test_get_failed();

                cpl_test_error(CPL_ERROR_NONE);
                code = cpl_polynomial_dump(poly1, stream);
                cpl_test_eq_error(code, CPL_ERROR_NONE);

                cpl_test_abs(xyz, x, 0.0);
                if (nfail != cpl_test_get_failed()) {
                    cpl_msg_error(cpl_func, "COLLAPSE(%d): %d %d", (int)ival,
                                  (int)idim1, (int)idim2);
                    code = cpl_polynomial_dump(poly3, stderr);
                    code = cpl_polynomial_dump(poly2, stderr);
                    code = cpl_polynomial_dump(poly1, stderr);
                }

                cpl_polynomial_test_delete(poly1);
            }
            cpl_polynomial_test_delete(poly2);
        }
        (void)cpl_vector_unwrap(vxyz);
    }

    cpl_polynomial_test_delete(poly1a);
    cpl_polynomial_test_delete(poly2a);
    cpl_polynomial_test_delete(poly3);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Test the CPL function
  @param    stream  A stream to dump to
  @return   void

 */
/*----------------------------------------------------------------------------*/
static void cpl_polynomial_extract_test_2d_one(FILE * stream)
{
    const cpl_size   nc = 5;
    cpl_polynomial * poly1a = cpl_polynomial_new(1);
    cpl_polynomial * poly2a = cpl_polynomial_new(2);
    cpl_error_code code;

    /* Coefficients span powers of 2 from 1 to nc^2 */
    for (cpl_size j = 0; j < nc; j++) {
        for (cpl_size i = 0; i < nc; i++) {
            const cpl_size expo[2] = {i, j};
            const double value = (double)(1 << (nc * j + i));

            code = cpl_polynomial_set_coeff(poly2a, expo, value);
            cpl_test_eq_error(code, CPL_ERROR_NONE);
        }
    }

    code = cpl_polynomial_dump(poly2a, stream);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    for (cpl_size ival = -2; ival <= 2; ival++) {
        /* Evaluate at this point */
        double         xval[2] = {(double)ival, (double)ival};
        cpl_vector   * vxy     = cpl_vector_wrap(2, xval);
        double         xy      = cpl_polynomial_eval(poly2a, vxy);
        const cpl_size i0      = 0;

        /* Setup collapsing polynomial */
        code = cpl_polynomial_set_coeff(poly1a, &i0, xval[0]);
        cpl_test_eq_error(code, CPL_ERROR_NONE);

        /* Collapsing either dimension and then evaluating
           must give to same result*/
        for (cpl_size idim = 0; idim < 2; idim++) {
            cpl_polynomial * poly1b = cpl_polynomial_extract(poly2a, idim,
                                                             poly1a);
            const double x = cpl_polynomial_eval_1d(poly1b, xval[0], NULL);

            cpl_test_error(CPL_ERROR_NONE);
            code = cpl_polynomial_dump(poly1b, stream);
            cpl_test_eq_error(code, CPL_ERROR_NONE);

            cpl_test_abs(xy, x, 0.0);

            cpl_polynomial_test_delete(poly1b);
        }
        (void)cpl_vector_unwrap(vxy);
    }

    cpl_polynomial_test_delete(poly1a);
    cpl_polynomial_test_delete(poly2a);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Test the CPL function
  @param    stream  A stream to dump to
  @param    idim    Passed to tested function
  @return   void

 */
/*----------------------------------------------------------------------------*/
static void cpl_polynomial_extract_test_2d(FILE * stream, cpl_size idim)
{

    cpl_polynomial * nullpoly;
    cpl_polynomial * zeropol = cpl_polynomial_new(1);
    cpl_polynomial * poly1 = cpl_polynomial_new(2);
    cpl_polynomial * poly2 = NULL;
    const cpl_size   i0 = 0;
    cpl_size dim, deg;
    double value;
    cpl_error_code   code;

    nullpoly = cpl_polynomial_extract(poly1, idim, poly2);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(nullpoly);

    nullpoly = cpl_polynomial_extract(poly2, idim, poly1);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(nullpoly);

    nullpoly = cpl_polynomial_extract(poly1, idim, poly1);
    cpl_test_error(CPL_ERROR_INCOMPATIBLE_INPUT);
    cpl_test_null(nullpoly);

    for (cpl_size icollapse = -1; icollapse < 2; icollapse++) {
        const double collapseval = (double)icollapse;

        cpl_msg_info(cpl_func, "Testing extraction at X=%g", collapseval);
        cpl_polynomial_empty(poly1);

        code = cpl_polynomial_set_coeff(zeropol, &i0, collapseval);
        cpl_test_eq_error(code, CPL_ERROR_NONE);

        cpl_polynomial_test_delete(poly2);
        poly2 = cpl_polynomial_extract(poly1, idim, zeropol);
        cpl_test_error(CPL_ERROR_NONE);
        dim = cpl_polynomial_get_dimension(poly2);
        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_eq(dim, 1);
        deg = cpl_polynomial_get_degree(poly2);
        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_eq(deg, 0);

        value = cpl_polynomial_get_coeff(poly2, &idim);
        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_abs(value, 0.0, 0.0);

        for (cpl_size j = 0; j <= POLY_SIZE; j++) {
            const cpl_size pows1[2] = {idim ? j : 0, idim ? 0 : j};
            const cpl_size pows2[2] = {idim ? j : 1, idim ? 1 : j};

            code = cpl_polynomial_set_coeff(poly1, pows1, (double)(j+1));
            cpl_test_eq_error(code, CPL_ERROR_NONE);
            code = cpl_polynomial_set_coeff(poly1, pows2, (double)(j+1));
            cpl_test_eq_error(code, CPL_ERROR_NONE);
        }

        cpl_polynomial_test_delete(poly2);
        poly2 = cpl_polynomial_extract(poly1, idim, zeropol);
        cpl_test_error(CPL_ERROR_NONE);
        dim = cpl_polynomial_get_dimension(poly2);
        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_eq(dim, 1);
        deg = cpl_polynomial_get_degree(poly2);
        cpl_test_error(CPL_ERROR_NONE);
        if (icollapse == -1) {
            cpl_test_zero(deg);
        } else {
            cpl_test_eq(deg, POLY_SIZE);
        }
        code = cpl_polynomial_dump(poly2, stream);
        cpl_test_eq_error(code, CPL_ERROR_NONE);
    }

    cpl_polynomial_test_delete(poly1);
    cpl_polynomial_test_delete(poly2);
    cpl_polynomial_test_delete(zeropol);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief      Test the CPL function
  @param    stream  A stream to dump to
  @return    void

 */
/*----------------------------------------------------------------------------*/
static void cpl_polynomial_basic_test(FILE * stream)
{
    /* Some binomial coefficients */
    const double     p15[BINOMIAL_SZ] = {1,15,105,455,1365,3003,5005,6435};
    cpl_size         expo[POLY_DIM];
    cpl_polynomial * poly1;
    cpl_polynomial * poly2;
    cpl_polynomial * poly3;
    cpl_polynomial * nullpoly;
    cpl_size         i, j, k;
    double           x;
    cpl_error_code   code;

    nullpoly = cpl_polynomial_new(0);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nullpoly);

    j = cpl_polynomial_get_dimension(nullpoly);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_eq(j, -1);

    j = cpl_polynomial_get_degree(nullpoly);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_eq(j, -1);

    i = 0;
    code = cpl_polynomial_set_coeff(nullpoly, &i, 0.0);
    cpl_test_eq_error(code, CPL_ERROR_NULL_INPUT);

    x = cpl_polynomial_get_coeff(nullpoly, &i);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_abs(x, 0.0, 0.0);

    for (i = 1; i <= POLY_DIM; i++) {
        int icmp;

        poly1 = cpl_polynomial_new(i);
        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_nonnull(poly1);

        j = cpl_polynomial_get_dimension(poly1);
        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_eq(j, i);

        j = cpl_polynomial_get_degree(poly1);
        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_zero(j);

        x = cpl_polynomial_get_coeff(poly1, NULL);
        cpl_test_error(CPL_ERROR_NULL_INPUT);
        cpl_test_abs(x, 0.0, 0.0);

        icmp = cpl_polynomial_compare(nullpoly, poly1, 0.0);
        cpl_test_error(CPL_ERROR_NULL_INPUT);
        cpl_test_eq(icmp, -1);

        icmp = cpl_polynomial_compare(poly1, nullpoly, 0.0);
        cpl_test_error(CPL_ERROR_NULL_INPUT);
        cpl_test_eq(icmp, -2);

        icmp = cpl_polynomial_compare(poly1, poly1, -1.0);
        cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
        cpl_test_eq(icmp, -5);

        for (cpl_size ii = 1; ii <= i; ii++) {
            (void)memset(expo, 0, sizeof(expo));
            for (k = 0; k < BINOMIAL_SZ; k++) {
                expo[ii - 1] = k;

                x = cpl_polynomial_get_coeff(poly1, expo);
                cpl_test_error(CPL_ERROR_NONE);
                cpl_test_abs(x, (k == 0 && ii > 1) ? p15[k] : 0.0, 0.0);

                code = cpl_polynomial_set_coeff(poly1, expo, p15[k]);
                cpl_test_eq_error(code, CPL_ERROR_NONE);

                x = cpl_polynomial_get_coeff(poly1, expo);
                cpl_test_error(CPL_ERROR_NONE);
                cpl_test_abs(x, p15[k], 0.0);

                j = cpl_polynomial_get_dimension(poly1);
                cpl_test_error(CPL_ERROR_NONE);
                cpl_test_eq(j, i);

                j = cpl_polynomial_get_degree(poly1);
                cpl_test_error(CPL_ERROR_NONE);
                cpl_test_eq(j, ii == 1 ? k : (BINOMIAL_SZ-1));

                poly2 = cpl_polynomial_duplicate(poly1);
                cpl_test_error(CPL_ERROR_NONE);
                cpl_test_nonnull(poly2);

                j = cpl_polynomial_get_dimension(poly2);
                cpl_test_error(CPL_ERROR_NONE);
                cpl_test_eq(j, i);

                j = cpl_polynomial_get_degree(poly2);
                cpl_test_error(CPL_ERROR_NONE);
                cpl_test_eq(j, ii == 1 ? k : (BINOMIAL_SZ-1));

                icmp = cpl_polynomial_compare(poly1, poly2, 0.0);
                cpl_test_error(CPL_ERROR_NONE);
                cpl_test_zero(icmp);

                cpl_polynomial_empty(poly2);
                cpl_test_error(CPL_ERROR_NONE);

                j = cpl_polynomial_get_dimension(poly2);
                cpl_test_error(CPL_ERROR_NONE);
                cpl_test_eq(j, i);

                j = cpl_polynomial_get_degree(poly2);
                cpl_test_error(CPL_ERROR_NONE);
                cpl_test_zero(j);

                icmp = cpl_polynomial_compare(poly1, poly2, 0.0);
                cpl_test_error(CPL_ERROR_NONE);
                cpl_test_lt(0, icmp);

                icmp = cpl_polynomial_compare(poly2, poly1, 0.0);
                cpl_test_error(CPL_ERROR_NONE);
                cpl_test_lt(0, icmp);

                code = cpl_polynomial_copy(poly2, poly1);
                cpl_test_eq_error(code, CPL_ERROR_NONE);

                j = cpl_polynomial_get_dimension(poly2);
                cpl_test_error(CPL_ERROR_NONE);
                cpl_test_eq(j, i);

                j = cpl_polynomial_get_degree(poly2);
                cpl_test_error(CPL_ERROR_NONE);
                cpl_test_eq(j, ii == 1 ? k : (BINOMIAL_SZ-1));

                icmp = cpl_polynomial_compare(poly1, poly2, 0.0);
                cpl_test_error(CPL_ERROR_NONE);
                cpl_test_zero(icmp);

                poly3 = cpl_polynomial_new(i);
                cpl_test_error(CPL_ERROR_NONE);
                cpl_test_nonnull(poly3);

                code = cpl_polynomial_add(poly3, poly2, poly1);
                cpl_test_eq_error(code, CPL_ERROR_NONE);

                code = cpl_polynomial_multiply_scalar(poly2, poly1, 2.0);
                cpl_test_eq_error(code, CPL_ERROR_NONE);

                cpl_test_polynomial_abs(poly3, poly2, 0.0);

                code = cpl_polynomial_add(poly3, poly3, poly3);
                cpl_test_eq_error(code, CPL_ERROR_NONE);

                code = cpl_polynomial_multiply_scalar(poly2, poly2, 2.0);
                cpl_test_eq_error(code, CPL_ERROR_NONE);

                cpl_test_polynomial_abs(poly3, poly2, 0.0);

                code = cpl_polynomial_subtract(poly3, poly3, poly1);
                cpl_test_eq_error(code, CPL_ERROR_NONE);

                code = cpl_polynomial_subtract(poly3, poly3, poly2);
                cpl_test_eq_error(code, CPL_ERROR_NONE);

                code = cpl_polynomial_multiply_scalar(poly3, poly3, -1.0);
                cpl_test_eq_error(code, CPL_ERROR_NONE);

                code = cpl_polynomial_multiply_scalar(poly2, poly2, 0.25);
                cpl_test_eq_error(code, CPL_ERROR_NONE);

                icmp = cpl_polynomial_compare(poly3, poly2, 0.0);
                cpl_test_error(CPL_ERROR_NONE);
                cpl_test_zero(icmp);

                icmp = cpl_polynomial_compare(poly1, poly2, 0.0);
                cpl_test_error(CPL_ERROR_NONE);
                cpl_test_zero(icmp);

                /* Validate total collapse to zero-polynomial */
                code = cpl_polynomial_subtract(poly3, poly3, poly3);
                cpl_test_eq_error(code, CPL_ERROR_NONE);
                j = cpl_polynomial_get_degree(poly3);
                cpl_test_error(CPL_ERROR_NONE);
                cpl_test_zero(j);

                code = cpl_polynomial_multiply_scalar(poly2, poly2, -1.0);
                cpl_test_eq_error(code, CPL_ERROR_NONE);
                code = cpl_polynomial_subtract(poly2, poly2, poly2);
                cpl_test_eq_error(code, CPL_ERROR_NONE);
                j = cpl_polynomial_get_degree(poly2);
                cpl_test_error(CPL_ERROR_NONE);
                cpl_test_zero(j);

                cpl_polynomial_test_delete(poly3);
                cpl_polynomial_test_delete(poly2);
            }
        }

        code = cpl_polynomial_dump(nullpoly, stream);
        cpl_test_eq_error(code, CPL_ERROR_NULL_INPUT);

        code = cpl_polynomial_dump(poly1, NULL);
        cpl_test_eq_error(code, CPL_ERROR_NULL_INPUT);

        code = cpl_polynomial_dump(poly1, stream);
        cpl_test_eq_error(code, CPL_ERROR_NONE);

        cpl_polynomial_empty(poly1);
        cpl_test_error(CPL_ERROR_NONE);

        code = cpl_polynomial_dump(poly1, stream);
        cpl_test_eq_error(code, CPL_ERROR_NONE);

        cpl_polynomial_test_delete(poly1);
        cpl_test_error(CPL_ERROR_NONE);
    }

}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief      Test the CPL function
  @param    stream  A stream to dump to
  @return    void

 */
/*----------------------------------------------------------------------------*/
static void cpl_polynomial_multiply_scalar_test_one(FILE * stream)
{

    cpl_polynomial * poly2a = cpl_polynomial_new(2) ;
    cpl_polynomial * poly2b = cpl_polynomial_new(2);
    cpl_error_code code;
    const cpl_size degree = 2;

    /* Setup two polynomials, the second with a NULL-pointer where the
       1st has a 1D-polynomial */

    for (cpl_size j = 0; j < degree; j++) {
        for (cpl_size i = 0; i < degree; i++) {
            const cpl_size expo[2] = {i, j};
            const double value = (double)(1 << (degree * j + i));

            code = cpl_polynomial_set_coeff(poly2a, expo, value);
            cpl_test_eq_error(code, CPL_ERROR_NONE);

            if (j == 0) {

                if (i == 0) {
                    /* Test while poly2b is empty */

                    code = cpl_polynomial_multiply_scalar(poly2b, poly2a, 0.5);
                    cpl_test_eq_error(code, CPL_ERROR_NONE);

                    /* Redo set coeff */
                    code = cpl_polynomial_set_coeff(poly2a, expo, value);
                    cpl_test_eq_error(code, CPL_ERROR_NONE);

                }

                continue;
            }

            code = cpl_polynomial_set_coeff(poly2b, expo, value);
            cpl_test_eq_error(code, CPL_ERROR_NONE);
        }
    }

    code = cpl_polynomial_multiply_scalar(poly2a, poly2b, 0.5);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_polynomial_dump(poly2b, stream);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_polynomial_test_delete(poly2a);
    cpl_polynomial_test_delete(poly2b);
    cpl_test_error(CPL_ERROR_NONE);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Delete a polynomail and emptying it by derivative
  @param    self    The polynomial to delete
  @return   void

 */
/*----------------------------------------------------------------------------*/
static void cpl_polynomial_test_delete(cpl_polynomial * self)
{
    if (self != NULL) {
        cpl_polynomial_derivative_test_empty(self);
        cpl_polynomial_delete(self);
    }
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Empty a polynomial by derivative
  @param    self    The polynomial to test and empty
  @param    stream  A stream to dump to
  @return   void

 */
/*----------------------------------------------------------------------------*/
static void cpl_polynomial_derivative_test_empty(cpl_polynomial * self)
{
    const cpl_size dim    = cpl_polynomial_get_dimension(self);
    const cpl_size deg    = cpl_polynomial_get_degree(self);
    cpl_size*      expo0  = cpl_calloc(dim, sizeof(*expo0));
    double         coeff0 = cpl_polynomial_get_coeff(self, expo0);

    cpl_test_error(CPL_ERROR_NONE);

    if (deg > 0 || coeff0 != 0.0) {
        cpl_polynomial* copy = NULL;
        cpl_size       deg0;
        cpl_error_code code;

        cpl_msg_info(cpl_func, "Derivative-emptying dim %d, degree %d",
                     (int)dim, (int)deg);

        switch (dim) {
        case 1:
            for (cpl_size i = deg; i > 0; i--) {
                cpl_size mydeg;

                code = cpl_polynomial_derivative(self, 0);
                cpl_test_eq_error(code, CPL_ERROR_NONE);

                mydeg = cpl_polynomial_get_degree(self);
                cpl_test_error(CPL_ERROR_NONE);

                cpl_test_eq(i, 1 + mydeg);
            }

            coeff0 = cpl_polynomial_get_coeff(self, expo0);
            cpl_test_error(CPL_ERROR_NONE);

            cpl_test_lt(0.0, fabs(coeff0));

            CPL_ATTR_FALLTRHU; /* fall through */
        default:
            copy = cpl_polynomial_duplicate(self);

            for (cpl_size j = 0; j < dim; j++) {
                const cpl_size jj = j < 2 ? (1 - j) : j; /* Switch 0 and 1 */
                const cpl_size newdeg1 = cpl_polynomial_get_degree(self);
                const cpl_size newdeg2 = cpl_polynomial_get_degree(copy);

                for (cpl_size i = CPL_MAX(newdeg1, newdeg2); i > 0; i--) {
                    cpl_size mydeg1;
                    cpl_size mydeg2;

                    code = cpl_polynomial_derivative(self, j);
                    cpl_test_eq_error(code, CPL_ERROR_NONE);

                    code = cpl_polynomial_derivative(copy, jj);
                    cpl_test_eq_error(code, CPL_ERROR_NONE);

                    mydeg1 = cpl_polynomial_get_degree(self);
                    cpl_test_error(CPL_ERROR_NONE);

                    mydeg2 = cpl_polynomial_get_degree(copy);
                    cpl_test_error(CPL_ERROR_NONE);

                    if (mydeg1 == newdeg1 && mydeg2 == newdeg2) break;
                }
                if (newdeg1 == 0 && newdeg2 == 0) break;
            }
        }

        deg0 = cpl_polynomial_get_degree(self);
        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_zero(deg0);

        code = cpl_polynomial_derivative(self, 0);
        cpl_test_eq_error(code, CPL_ERROR_NONE);

        coeff0 = cpl_polynomial_get_coeff(self, expo0);
        cpl_test_error(CPL_ERROR_NONE);

        cpl_test(coeff0 != 0.0 ? CPL_FALSE : CPL_TRUE);

        if (copy != NULL) {

            deg0 = cpl_polynomial_get_degree(copy);
            cpl_test_error(CPL_ERROR_NONE);
            cpl_test_zero(deg0);

            code = cpl_polynomial_derivative(copy, 0);
            cpl_test_eq_error(code, CPL_ERROR_NONE);

            coeff0 = cpl_polynomial_get_coeff(copy, expo0);
            cpl_test_error(CPL_ERROR_NONE);

            cpl_test(coeff0 != 0.0 ? CPL_FALSE : CPL_TRUE);

            cpl_polynomial_test_delete(copy);
        }
    }

    cpl_free(expo0);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief      Test the evaluation functions in cpl_polynomial. 
  @return  void
  
  This function will test the evaluation functions in cpl_polynomial for
  multivariate polynomials. It will compare the results using both
  a "direct" evaluation and the Horner evaluation algorithm.

 The polynomial evaluated is:

  P(x,y) =  x^4 -1e-5 x^5 y = x^4 (1 - 1e-5 x^1 y^1)
 
 */
/*----------------------------------------------------------------------------*/
static void cpl_polynomial_eval_test_2d_one()
{
    /* Create a 2-Dimensions polynomial */
    cpl_polynomial   *  poly2d = cpl_polynomial_new(2);
    cpl_size            expo_2d[2];
    double              xy[2];
    cpl_vector       *  xeval_2d = cpl_vector_wrap(2, xy);
    double              evalh, evalm;
    cpl_error_code      code;
    
    expo_2d[0] = 4;
    expo_2d[1] = 0;
    code = cpl_polynomial_set_coeff(poly2d, expo_2d, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    expo_2d[0] = 5;
    expo_2d[1] = 1;
    code = cpl_polynomial_set_coeff(poly2d, expo_2d, -1e-5);  
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    
    /* Evaluate at a specific point, at a root */
    xy[0] = 1e3;
    xy[1] = 1e2;
    evalh = cpl_polynomial_eval(poly2d, xeval_2d);
    cpl_test_error(CPL_ERROR_NONE);
    evalm = cpl_polynomial_eval_monomial(poly2d, xeval_2d);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_msg_info(cpl_func, "Evaluation at (polynomial zero) [%g, %g] using "
                 "horner vs classical evaluation: %g - %g = %g", xy[0], xy[1],
                 evalh, evalm, evalh - evalm);

    /* Evaluate at a specific point, at another root */
    xy[0] = 1e5;
    xy[1] = 1e0;
    evalh = cpl_polynomial_eval(poly2d, xeval_2d);
    cpl_test_error(CPL_ERROR_NONE);
    evalm = cpl_polynomial_eval_monomial(poly2d, xeval_2d);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_msg_info(cpl_func, "Evaluation at (polynomial zero) [%g, %g] using "
                 "horner vs classical evaluation: %g - %g = %g", xy[0], xy[1],
                 evalh, evalm, evalh - evalm);

    /* Evaluate at a specific point, not near a root */
    xy[0] = 2.0;
    xy[1] = 3.0;
    evalh = cpl_polynomial_eval(poly2d, xeval_2d);
    cpl_test_error(CPL_ERROR_NONE);
    evalm = cpl_polynomial_eval_monomial(poly2d, xeval_2d);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_msg_info(cpl_func, "Evaluation at [%g, %g] using "
                 "horner vs classical evaluation: %g - %g = %g", xy[0], xy[1],
                 evalh, evalm, evalh - evalm);
    
    /* Evaluate at a specific point, closer to a root */
    xy[0] = 2e4;
    xy[1] = 1e0;
    evalh = cpl_polynomial_eval(poly2d, xeval_2d);
    cpl_test_error(CPL_ERROR_NONE);
    evalm = cpl_polynomial_eval_monomial(poly2d, xeval_2d);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_msg_info(cpl_func, "Evaluation at (polynomial zero) [%g, %g] using "
                 "horner vs classical evaluation: %g - %g = %g", xy[0], xy[1],
                 evalh, evalm, evalh - evalm);

    /* Free */
    cpl_polynomial_delete(poly2d);
    cpl_vector_unwrap(xeval_2d);
}
