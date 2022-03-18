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

#include "cpl_io_fits.h"
#include "cpl_test.h"
#include "cpl_tools.h"
#include "cpl_math_const.h"
#include "cpl_memory.h"
#include "cpl_plot.h"

#include <float.h>
#include <assert.h>
#include <math.h>


/* Must have three digits, for perl-test to work */
#define VECTOR_SIZE 256
#define VECTOR_CUT 24

/* alphaev56 SIGFPE's with more than 20 */
#define POLY_SIZE 20

/*-----------------------------------------------------------------------------
                        Private function prototypes
 -----------------------------------------------------------------------------*/

static void cpl_vector_cycle_test(cpl_size);
static void cpl_vector_save_bench(int);
static void cpl_vector_get_stdev_bench(int);
static void cpl_vector_corr_bench(int);
static void cpl_vector_fit_gaussian_test_one(FILE *);
static void cpl_vector_valarray_bench_one(cpl_vector *, cpl_vector *);
static void cpl_vector_valarray_bench(cpl_size);
/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/
int main(void)
{
    double        xc;
    double        emax = 0; /* Maximum observed xc-error */
    double        tmp;
    cpl_vector  * null;
    cpl_vector  * sinus;
    cpl_vector  * cosinus;
    cpl_vector  * tmp_vec;
    cpl_vector  * taylor;
    cpl_vector  * tmp_vec2;
    cpl_vector  * vxc;
    cpl_vector  * vxc1;
    cpl_vector  * vxc3;
    double      * data;
    const double  five[] = {1,2,3,4,5};
    const cpl_size vdif = VECTOR_SIZE - VECTOR_CUT > VECTOR_CUT
                       ? VECTOR_CUT : VECTOR_SIZE - VECTOR_CUT;
    const cpl_size vdif2 = (VECTOR_SIZE - VECTOR_CUT)/2 > VECTOR_CUT/2
                        ? VECTOR_CUT/2 : (VECTOR_SIZE - VECTOR_CUT)/2;
    cpl_size      delta;
    cpl_size      half_search;
    cpl_size           i,k;
    cpl_boolean   do_bench;
    FILE        * stream;
    FILE        * f_out;
    char          filename[1024];
    cpl_boolean did_test_large = CPL_FALSE;
    cpl_error_code error;
    const int omp_num_threads =
#ifdef _OPENMP
        /* Measure scaled speed-up */
        getenv("OMP_NUM_THREADS") ? atoi(getenv("OMP_NUM_THREADS")) :
#endif
        1;
    const int npe = omp_num_threads > 1 ? omp_num_threads : 1;

    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    stream = cpl_msg_get_level() > CPL_MSG_INFO
        ? fopen("/dev/null", "a") : stdout;

    do_bench = cpl_msg_get_level() <= CPL_MSG_INFO ? CPL_TRUE : CPL_FALSE;

    /* Insert tests below */

    cpl_test_nonnull( stream );

    /* Test both odd and even numbered */
    cpl_vector_cycle_test(13);
    cpl_vector_cycle_test(16);
    cpl_vector_cycle_test(VECTOR_SIZE);
    cpl_vector_cycle_test(VECTOR_SIZE + 1);

    cpl_vector_fit_gaussian_test_one(stream);

    null = cpl_vector_new(0);

    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(null);

    /* Verify cpl_vector_get_median() with even number of samples (DFS12089) */
    /* Test on unsorted vector */
    tmp_vec = cpl_vector_new(4);
    cpl_vector_set(tmp_vec, 0, 0.0);
    cpl_vector_set(tmp_vec, 1, 1.0);
    cpl_vector_set(tmp_vec, 2, 3.0);
    cpl_vector_set(tmp_vec, 3, 2.0);
    cpl_test_rel(1.5, cpl_vector_get_median(tmp_vec), DBL_EPSILON);
    cpl_test_rel(1.5, cpl_vector_get_median_const(tmp_vec), DBL_EPSILON);
    /* Test on sorted vector */
    cpl_vector_set(tmp_vec, 0, 0.0);
    cpl_vector_set(tmp_vec, 1, 1.0);
    cpl_vector_set(tmp_vec, 2, 2.0);
    cpl_vector_set(tmp_vec, 3, 3.0);
    cpl_test_rel(1.5, cpl_vector_get_median(tmp_vec), DBL_EPSILON);
    cpl_test_rel(1.5, cpl_vector_get_median_const(tmp_vec), DBL_EPSILON);
    cpl_vector_delete(tmp_vec);
    
    /* Create the vector sinus */
    cpl_test_nonnull( sinus = cpl_vector_new(VECTOR_SIZE) );
    
    /* Test cpl_vector_get_size() */
    cpl_test_eq( cpl_vector_get_size(sinus), VECTOR_SIZE );

    /* Fill the vector sinus */
    /* Test cpl_vector_get_data(), cpl_vector_set(), cpl_vector_get() */
    data = cpl_vector_get_data(sinus);
    cpl_test_nonnull( data );
    for (i=0; i < VECTOR_SIZE; i++) {
        const double value = sin(i*CPL_MATH_2PI/VECTOR_SIZE);
        cpl_test_zero( cpl_vector_set(sinus, i, value) );
        cpl_test_eq(  cpl_vector_get(sinus, i), data[i] );
    }

    /* Create a Taylor-expansion of exp() */
    cpl_test_nonnull( taylor = cpl_vector_new(POLY_SIZE) );
    i = 0;
    cpl_vector_set(taylor, i, 1);
    for (i=1; i<POLY_SIZE; i++)
        cpl_vector_set(taylor, i, cpl_vector_get(taylor, i-1)/i);

    /* Evaluate exp(sinus) using Horners scheme on the Taylor expansion */
    tmp_vec2 = cpl_vector_new(VECTOR_SIZE);
    cpl_test_nonnull(tmp_vec2);
    error = cpl_vector_fill(tmp_vec2, cpl_vector_get(taylor, POLY_SIZE-1));
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    
    for (k=POLY_SIZE-1; k > 0; k--) {
        cpl_test_zero( cpl_vector_multiply(tmp_vec2, sinus) );
        if (k&1) {
          cpl_test_zero( cpl_vector_add_scalar(tmp_vec2,
                                         cpl_vector_get(taylor, k-1)) );
        } else {
          cpl_test_zero( cpl_vector_subtract_scalar(tmp_vec2,
                                              -cpl_vector_get(taylor, k-1)) );
        }
    }        

    /* Verify the result (against cpl_vector_exponential() ) */
    cpl_test( tmp_vec = cpl_vector_duplicate(sinus) );

    cpl_test_zero( cpl_vector_exponential(tmp_vec, CPL_MATH_E) );

    cpl_test_zero( cpl_vector_subtract(tmp_vec2, tmp_vec) );
    cpl_test_zero( cpl_vector_divide(tmp_vec2, tmp_vec) );
    cpl_test_zero( cpl_vector_divide_scalar(tmp_vec2, DBL_EPSILON) );

    cpl_test_leq( fabs(cpl_vector_get_max(tmp_vec2)), 2.60831 );
    cpl_test_leq( fabs(cpl_vector_get_min(tmp_vec2)), 2.03626 );

    /* Evaluate exp() using cpl_vector_pow() on the Taylor expansion */
    cpl_test_zero( cpl_vector_fill(tmp_vec2, cpl_vector_get(taylor, 0)) );

    /* POLY_SIZE > 20 on alphaev56:
        Program received signal SIGFPE, Arithmetic exception.
          0x200000a3ff0 in cpl_vector_multiply_scalar ()
    */
    for (k=1; k < POLY_SIZE; k++) {
        cpl_vector * vtmp = cpl_vector_duplicate(sinus);
        cpl_test_zero( cpl_vector_power(vtmp, k) );
        cpl_test_zero( cpl_vector_multiply_scalar(vtmp, cpl_vector_get(taylor, k)) );
        cpl_test_zero( cpl_vector_add(tmp_vec2, vtmp) );
        cpl_vector_delete(vtmp);
    }        

    /* Much less precise than Horner ... */
    cpl_test_vector_abs(tmp_vec, tmp_vec2, 8.0 * DBL_EPSILON);

    /* cpl_vector_fill(): Test with NULL and zero value */
    error = cpl_vector_fill(NULL, 0.0);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);
    error = cpl_vector_fill(taylor, 0.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_abs(cpl_vector_get_max(taylor), 0.0, 0.0);
    cpl_test_abs(cpl_vector_get_min(taylor), 0.0, 0.0);

    cpl_vector_delete(taylor);

    /* Verify cpl_vector_logarithm() ) */
    cpl_test_zero( cpl_vector_logarithm(tmp_vec, CPL_MATH_E) );
    for (i=0; i < VECTOR_SIZE; i++) {
        const double value = cpl_vector_get(sinus, i);
        double lerror = value - cpl_vector_get(tmp_vec,  i);
        if (2*i == VECTOR_SIZE) {
            /* value should really be zero */
            cpl_test_abs( value, 0.0, 0.552 * DBL_EPSILON );
        } else {
          if (value != 0) lerror /= value;
          cpl_test_abs( lerror, 0.0, 330 * DBL_EPSILON );
        }
    }

    /* Verify cpl_vector_power() */
    cpl_test_zero(  cpl_vector_copy(tmp_vec, sinus) );
    /* Just be positive */
    cpl_test_zero( cpl_vector_exponential(tmp_vec, CPL_MATH_E) );

    cpl_test_zero( cpl_vector_copy(tmp_vec2, tmp_vec) );

    cpl_test_zero( cpl_vector_sqrt(tmp_vec2) );
    cpl_test_zero( cpl_vector_power(tmp_vec, 0.5) );

    /* Necessary on AMD 64 (x86_64) Linux */
    cpl_test_vector_abs(tmp_vec, tmp_vec2, 1.1 * DBL_EPSILON);

    cpl_test_zero(  cpl_vector_copy(tmp_vec, sinus) );
    cpl_test_zero( cpl_vector_exponential(tmp_vec, CPL_MATH_E) );

    cpl_test_zero(  cpl_vector_multiply(tmp_vec2, tmp_vec) );

    cpl_test_zero( cpl_vector_power(tmp_vec, 1.5) );

    cpl_test_vector_abs(tmp_vec, tmp_vec2, 8.0 * DBL_EPSILON);

    cpl_test_zero(  cpl_vector_copy(tmp_vec2, tmp_vec) );

    cpl_test_zero( cpl_vector_power(tmp_vec, 2) );

    cpl_test_zero(  cpl_vector_divide(tmp_vec2, tmp_vec) );

    cpl_test_zero( cpl_vector_power(tmp_vec, -0.5) );

    cpl_test_vector_abs(tmp_vec, tmp_vec2, 8.0 * DBL_EPSILON);

    cpl_test_zero( cpl_vector_fill(tmp_vec, 1) );

    cpl_test_zero( cpl_vector_power(tmp_vec2, 0) );

    cpl_test_vector_abs(tmp_vec, tmp_vec2, 0.0);

    cpl_vector_delete(tmp_vec2);

    /* Test 0^0 */
    cpl_test_nonnull( tmp_vec2 = cpl_vector_new(VECTOR_SIZE) );
    for (i = 0; i < VECTOR_SIZE; i++) 
    cpl_test_zero( cpl_vector_set(tmp_vec2, i, 0.0) );

    cpl_test_zero( cpl_vector_power(tmp_vec2, 0.0) );
    cpl_test_vector_abs(tmp_vec, tmp_vec2, 0.0);

    cpl_vector_delete(tmp_vec);
    cpl_vector_delete(tmp_vec2);

    /* Test cpl_vector_dump() */
    cpl_vector_dump(NULL, stream);
    cpl_vector_dump(sinus, stream);

    /* Test failures on cpl_vector_read() */
    tmp_vec = cpl_vector_read(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null( tmp_vec );

    tmp_vec = cpl_vector_read("/nonexisting");
    cpl_test_error(CPL_ERROR_FILE_IO);
    cpl_test_null( tmp_vec );

    tmp_vec = cpl_vector_read("/dev/null");
    cpl_test_error(CPL_ERROR_BAD_FILE_FORMAT);
    cpl_test_null( tmp_vec );

    /* Test correct case on cpl_vector_read() */
    sprintf(filename, "cpl_vector_dump.txt");
    cpl_test_nonnull( filename );
    cpl_test_nonnull( f_out = fopen(filename, "w") );
    cpl_vector_dump(sinus, f_out);
    fclose(f_out);

    tmp_vec = cpl_vector_read("cpl_vector_dump.txt");
    cpl_test_zero( remove("cpl_vector_dump.txt") );
    cpl_test_nonnull( tmp_vec );

    cpl_test_eq( cpl_vector_get_size(tmp_vec), cpl_vector_get_size(sinus) );

    /* Test cpl_vector_save() / cpl_vector_load() */
    error = cpl_vector_save(tmp_vec, "cpl_vector_save.fits", CPL_TYPE_DOUBLE,
                            NULL, CPL_IO_CREATE | CPL_IO_EXTEND); 
    cpl_test_eq_error( error, CPL_ERROR_ILLEGAL_INPUT );

    error = cpl_vector_save(tmp_vec, "cpl_vector_save.fits", CPL_TYPE_DOUBLE,
                            NULL, CPL_IO_APPEND); 
    cpl_test_eq_error( error, CPL_ERROR_ILLEGAL_INPUT );

    error = cpl_vector_save(tmp_vec, "cpl_vector_save.fits", CPL_TYPE_DOUBLE,
                            NULL, CPL_IO_CREATE | CPL_IO_APPEND); 
    cpl_test_eq_error( error, CPL_ERROR_ILLEGAL_INPUT );

    error = cpl_vector_save(tmp_vec, "cpl_vector_save.fits", CPL_TYPE_DOUBLE,
                            NULL, CPL_IO_CREATE); 
    cpl_test_eq_error( error, CPL_ERROR_NONE );
    cpl_test_fits("cpl_vector_save.fits");

    tmp_vec2 = cpl_vector_load("cpl_vector_save.fits", 0);
    cpl_test_error( CPL_ERROR_NONE );
    cpl_test_zero( remove("cpl_vector_save.fits") );
    cpl_test_nonnull( tmp_vec2 );

    /* Verify that the save/load did not change the vector */
    cpl_test_vector_abs(tmp_vec, tmp_vec2, 0.0);

    cpl_vector_delete(tmp_vec2);

    /* Repeat test cpl_vector_save() / cpl_vector_load() on APPEND mode*/
    error = cpl_vector_save(tmp_vec, "cpl_vector_save.fits",
                            CPL_TYPE_DOUBLE, NULL, CPL_IO_CREATE); 
    cpl_test_eq_error( error, CPL_ERROR_NONE );
    cpl_test_fits("cpl_vector_save.fits");

    error = cpl_vector_save(tmp_vec, "cpl_vector_save.fits",
                            CPL_TYPE_DOUBLE, NULL, CPL_IO_EXTEND);
    cpl_test_eq_error( error, CPL_ERROR_NONE );
    cpl_test_fits("cpl_vector_save.fits");

    /* Verify that the save/load did not change the vector on 0. ext. */
    tmp_vec2 = cpl_vector_load("cpl_vector_save.fits", 0);
    cpl_test_nonnull( tmp_vec2 );
    cpl_test_vector_abs(tmp_vec, tmp_vec2, 0.0);
    cpl_vector_delete(tmp_vec2);

    /* Verify that the save/load did not change the vector on 1. ext. */
    tmp_vec2 = cpl_vector_load("cpl_vector_save.fits", 1);
    cpl_test_nonnull( tmp_vec2 );
    cpl_test_vector_abs(tmp_vec, tmp_vec2, 0.0);
    cpl_vector_delete(tmp_vec2);

    if (!cpl_io_fits_is_enabled()) {
        /* Decrease the number of elements by one,
           thus verifying that an external application may modify the file */
        if (system("perl -pi -e 'BEGIN{sleep(1)};/NAXIS1/ and s/"
                   CPL_STRINGIFY(VECTOR_SIZE) "/sprintf(\"%d\","
                   CPL_STRINGIFY(VECTOR_SIZE-1)
                   ")/e' cpl_vector_save.fits") == 0) {

            tmp_vec2 = cpl_vector_load("cpl_vector_save.fits", 0);
            cpl_test_error(CPL_ERROR_NONE);
            cpl_test_nonnull( tmp_vec2 );
            cpl_test_eq(cpl_vector_get_size(tmp_vec2), VECTOR_SIZE-1);
            cpl_vector_delete(tmp_vec2);
        }
    }

    if (sizeof(cpl_size) == 4) {
#if !defined CPL_SIZE_BITS || CPL_SIZE_BITS != 32
        if (!cpl_io_fits_is_enabled()) {
            /* Cannot load a vector longer than 2**31 - 1 */

            /* Increase the number of elements to more than 2**31 */
            if (system("perl -pi -e '/NAXIS1/ and s/       "
                       CPL_STRINGIFY(VECTOR_SIZE)
                       "/2200000000/' cpl_vector_save.fits") == 0) {

                tmp_vec2 = cpl_vector_load("cpl_vector_save.fits", 0);
                cpl_test_error(CPL_ERROR_UNSUPPORTED_MODE);
                cpl_test_null( tmp_vec2 );
                if (tmp_vec2 != NULL) {
                    /* The original size is VECTOR_SIZE */
                    cpl_test_noneq(cpl_vector_get_size(tmp_vec2), VECTOR_SIZE);
                    cpl_vector_delete(tmp_vec2);
                    cpl_test_assert(0);
                }
                did_test_large = CPL_TRUE;
            }
        }
#endif
#ifdef CPL_TEST_LARGE
    } else if (sizeof(cpl_size) == 8) {
        cpl_vector * long_vec = cpl_vector_new(2200000000L);
        error = cpl_vector_save(long_vec, "cpl_vector_save.fits",
                                CPL_TYPE_DOUBLE, NULL, CPL_IO_CREATE);
        cpl_test_eq_error(error, CPL_ERROR_NONE);
        cpl_test_fits("cpl_vector_save.fits");
        cpl_vector_delete(long_vec);
        long_vec = cpl_vector_load("cpl_vector_save.fits", 0);
        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_nonnull(long_vec);
        cpl_vector_delete(long_vec);
        did_test_large = CPL_TRUE;
#endif
    }

    if (!did_test_large) {
        cpl_msg_info(cpl_func, "I/O-testing of large vectors inactive");
    }

    cpl_test_zero( remove("cpl_vector_save.fits") );

    /* Loss of precision in cpl_vector_dump() */
    cpl_test_vector_abs(tmp_vec, sinus, 10.0 * FLT_EPSILON);

    cpl_vector_subtract(tmp_vec, sinus);

    /* Same loss for positive as for negative numbers */
    cpl_test_abs( cpl_vector_get_max(tmp_vec)+cpl_vector_get_min(tmp_vec), 0.0,
        2.5 * DBL_EPSILON);

    cpl_vector_delete(tmp_vec);

    /* Test cpl_vector_duplicate */
    tmp_vec = cpl_vector_duplicate(sinus);
    cpl_test_vector_abs(tmp_vec, sinus, 0.0);

    /* Test fill function */
    cpl_test_eq_error( cpl_vector_fill(tmp_vec, 1.0), CPL_ERROR_NONE );
    cpl_test_abs( cpl_vector_get_mean(tmp_vec), 1.0, DBL_EPSILON );

    cpl_test_abs( cpl_vector_get_sum(tmp_vec), (double)(VECTOR_SIZE),
                  DBL_EPSILON * sqrt((double)(VECTOR_SIZE)));

    (void)cpl_vector_get_sum(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);

    /* Test extract function */
    tmp_vec2 = cpl_vector_extract(tmp_vec, 0, VECTOR_SIZE/2, 1);
    cpl_test_nonnull( tmp_vec2 );

    cpl_vector_delete(tmp_vec2);

    null = cpl_vector_extract(NULL, 0, 1, 1);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null( null );
    
    null = cpl_vector_extract(tmp_vec, 2, 1, 1);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null( null );

    null = cpl_vector_extract(tmp_vec, 1, 2, 2);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null( null );

    null = cpl_vector_extract(tmp_vec, -1, 2, 1);
    cpl_test_error(CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_test_null( null );

    null = cpl_vector_extract(tmp_vec, 0, VECTOR_SIZE + 2, 1);
    cpl_test_error(CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_test_null( null );

    CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual);
    vxc = cpl_vector_wrap(5, (double*)five);
    CPL_DIAG_PRAGMA_POP;

    vxc1 = cpl_vector_extract(vxc, 1, 4, 1);
    cpl_test_error(CPL_ERROR_NONE);

    cpl_test_eq_ptr(five, cpl_vector_unwrap(vxc));

    CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual);
    vxc = cpl_vector_wrap(4, (double*)five + 1);
    CPL_DIAG_PRAGMA_POP;
    cpl_test_vector_abs(vxc, vxc1, 0.0);

    (void)cpl_vector_unwrap(vxc);
    cpl_vector_delete(vxc1);

    /* Create the vector cosinus */
    cosinus = cpl_vector_new(VECTOR_SIZE);
    cpl_test_eq( cpl_vector_get_size(sinus), VECTOR_SIZE );
    
    /* Fill the vector cosinus */
    data = cpl_vector_get_data(cosinus);
    cpl_test_nonnull( data );
    for (i=0; i<VECTOR_SIZE; i++) data[i] = cos(i*CPL_MATH_2PI/VECTOR_SIZE);

    /* Test mean function */
    cpl_test_abs( cpl_vector_get_mean(cosinus), 0.0, 1.68*DBL_EPSILON );
    /* Test stdev function  (NB: the mean-value of cosinus-squared is 1/2) */
    (void)cpl_vector_get_stdev(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);

    cpl_test_abs( cpl_vector_get_stdev(cosinus),
                  sqrt(VECTOR_SIZE*0.5/(VECTOR_SIZE - 1)),
                  0.36 * DBL_EPSILON * sqrt(VECTOR_SIZE));

    /* Test copy function */
    cpl_test_eq_error( cpl_vector_copy(tmp_vec, cosinus), CPL_ERROR_NONE );
    cpl_test_vector_abs(tmp_vec, cosinus, 0.0);

    cpl_vector_delete(tmp_vec);

    /* Test add & sub functions */
    tmp_vec = cpl_vector_duplicate(sinus);
    cpl_test_vector_abs(tmp_vec, sinus, 0.0);
    cpl_vector_add(tmp_vec, cosinus);
    cpl_vector_subtract(tmp_vec, sinus);

    cpl_test_vector_abs(tmp_vec, cosinus, DBL_EPSILON);

    /* Test cpl_vector_subtract_scalar() function */
    cpl_test_eq_error( cpl_vector_subtract_scalar(tmp_vec, 2), CPL_ERROR_NONE );

   /* Test div function */
    cpl_test_eq_error( cpl_vector_divide(tmp_vec, tmp_vec), CPL_ERROR_NONE );
    cpl_test_leq( cpl_vector_get_mean(tmp_vec) - 1, DBL_EPSILON );

    cpl_vector_delete(tmp_vec);

    /* Test dot-product - using orthogonal vectors and pythagoras */
    cpl_test_leq( cpl_vector_product(sinus, cosinus),
                       DBL_EPSILON*VECTOR_SIZE);

    cpl_test_abs( cpl_vector_product(  sinus,   sinus) +
                  cpl_vector_product(cosinus, cosinus),
                  VECTOR_SIZE, DBL_EPSILON*VECTOR_SIZE );

    /* Test filtering */
    tmp_vec = cpl_vector_filter_lowpass_create(sinus, CPL_LOWPASS_LINEAR, 2);
    cpl_test_eq( cpl_vector_get_size(tmp_vec), cpl_vector_get_size(sinus) );
    cpl_vector_delete(tmp_vec);

    tmp_vec = cpl_vector_filter_median_create(sinus, 2);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq( cpl_vector_get_size(tmp_vec), VECTOR_SIZE );
    cpl_vector_delete(tmp_vec);

    tmp_vec = cpl_vector_filter_median_create(sinus, VECTOR_SIZE/2);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq( cpl_vector_get_size(tmp_vec), VECTOR_SIZE );
    if (2 * (VECTOR_SIZE/2) == cpl_vector_get_size(tmp_vec))
        cpl_test_vector_abs(tmp_vec, sinus, 0.0);

    cpl_vector_delete(tmp_vec);

    null = cpl_vector_filter_median_create(sinus, -1);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(null);

    null = cpl_vector_filter_median_create(sinus, 1 + VECTOR_SIZE/2);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(null);

    null = cpl_vector_filter_median_create(NULL, 0);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(null);

    /* Test existence of cpl_vector_fit_gaussian() */
    error = cpl_vector_fit_gaussian(NULL, NULL, NULL, NULL, CPL_FIT_ALL,
                                    NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT );

    /* sinus <- sinus*sinus */
    cpl_test_eq_error( cpl_vector_multiply(sinus, sinus), CPL_ERROR_NONE );

    /* Multiply by -1 */
    cpl_test_eq_error( cpl_vector_multiply_scalar(sinus, -1), CPL_ERROR_NONE );
    
    /* Add 1 */
    cpl_test_eq_error( cpl_vector_add_scalar(sinus, 1), CPL_ERROR_NONE );
   
    /* sinus <- sqrt(1-sinus^2) */
    cpl_test_eq_error( cpl_vector_sqrt(sinus), CPL_ERROR_NONE );

    /* Compute the absolute value of cosinus */
    data = cpl_vector_get_data(cosinus);
    cpl_test_nonnull( data );
    for (i=0; i<VECTOR_SIZE; i++) data[i] = fabs(data[i]);

    /* Compare fabs(cosinus) with sqrt(1-sinus^2) */
    cpl_test_vector_abs(sinus, cosinus, 10.0 * DBL_EPSILON);

    cpl_test_zero(cpl_vector_copy(sinus, cosinus));

    cpl_test_zero(cpl_vector_sort(cosinus, CPL_SORT_ASCENDING));
    for (i=1; i<VECTOR_SIZE; i++) cpl_test_leq( data[i-1], data[i]);

    cpl_test_zero(cpl_vector_sort(sinus, CPL_SORT_DESCENDING));
    data = cpl_vector_get_data(sinus);
    cpl_test_nonnull( data );
    for (i=1; i<VECTOR_SIZE; i++) cpl_test_leq( data[i], data[i-1] );

    cpl_test_abs( cpl_vector_get_mean(cosinus), cpl_vector_get_mean(sinus),
                  15.5*DBL_EPSILON );

    /* Create a 1-element array */
    tmp = 0.0;
    tmp_vec = cpl_vector_wrap(1, &tmp);
    cpl_test_nonnull( tmp_vec );

    error = cpl_vector_sort(tmp_vec,  CPL_SORT_ASCENDING);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_abs(tmp, 0.0, 0.0);

    error = cpl_vector_sort(tmp_vec, CPL_SORT_DESCENDING);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_abs(tmp, 0.0, 0.0);

    cpl_test_eq_ptr(cpl_vector_unwrap(tmp_vec), &tmp);

    error = cpl_vector_sort(sinus, 2);
    cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

    error = cpl_vector_sort(NULL, CPL_SORT_ASCENDING);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

    cpl_vector_set_size(sinus, 1);

    cpl_test_zero(cpl_vector_set(sinus, 0, 0.0));
    cpl_test_zero(cpl_vector_sort(sinus, CPL_SORT_DESCENDING));
    cpl_test_abs(cpl_vector_get(sinus, 0), 0.0, 0.0);
    cpl_test_zero(cpl_vector_sort(sinus, CPL_SORT_ASCENDING));
    cpl_test_abs(cpl_vector_get(sinus, 0), 0.0, 0.0);

    cpl_vector_delete(cosinus);
    cpl_vector_delete(sinus);

    /* Create the double-length vector sinus */
    cpl_test_nonnull( sinus = cpl_vector_new(2*VECTOR_SIZE) );
    
    /* Fill the vector sinus */
    data = cpl_vector_get_data(sinus);
    cpl_test_nonnull( data );
    for (i=0; i<2*VECTOR_SIZE; i++) data[i] = sin(i*CPL_MATH_2PI/VECTOR_SIZE);


    /* Create the vector cosinus */
    cpl_test_nonnull( cosinus = cpl_vector_new(VECTOR_SIZE) );
    
    /* Fill the vector cosinus */
    data = cpl_vector_get_data(cosinus);
    cpl_test_nonnull( data );
    for (i=0; i<VECTOR_SIZE; i++) data[i] = cos(i*CPL_MATH_2PI/VECTOR_SIZE);

    /* Create the vector tmp_vec */
    tmp_vec = cpl_vector_new(VECTOR_SIZE-1);
    cpl_test_nonnull( tmp_vec );
    cpl_test_eq_error( cpl_vector_fill(tmp_vec, 1.0), CPL_ERROR_NONE );

    vxc1 = cpl_vector_new(1);
    vxc3 = cpl_vector_new(3);
   /*
    Various error conditions
    */

    delta = cpl_vector_correlate(NULL, sinus, sinus);
    cpl_test_error( CPL_ERROR_NULL_INPUT );
    cpl_test( delta < 0 );

    delta = cpl_vector_correlate(vxc1, NULL, sinus);
    cpl_test_error( CPL_ERROR_NULL_INPUT );
    cpl_test( delta < 0 );

    delta = cpl_vector_correlate(vxc1, sinus, NULL);
    cpl_test_error( CPL_ERROR_NULL_INPUT );
    cpl_test( delta < 0 );

    delta = cpl_vector_correlate(cosinus, sinus, sinus);
    cpl_test_error( CPL_ERROR_ILLEGAL_INPUT );
    cpl_test( delta < 0 );

    delta = cpl_vector_correlate(vxc1, cosinus, sinus);
    cpl_test_error( CPL_ERROR_ILLEGAL_INPUT );
    cpl_test( delta < 0 );

    delta = cpl_vector_correlate(vxc3, cosinus, tmp_vec);
    cpl_test_error( CPL_ERROR_ILLEGAL_INPUT );
    cpl_test( delta < 0 );

    cpl_test_zero( cpl_vector_correlate(vxc1, cosinus, tmp_vec));
    cpl_test_zero( cpl_vector_get(vxc1, 0) );

    cpl_vector_delete(tmp_vec);

    cpl_test_zero( cpl_vector_multiply_scalar(sinus,   CPL_MATH_SQRT2) );
    cpl_test_zero( cpl_vector_add_scalar(     sinus,   CPL_MATH_PI   ) );
    cpl_test_zero( cpl_vector_multiply_scalar(cosinus, CPL_MATH_SQRT2) );

    cpl_test_zero( cpl_vector_correlate(vxc1, sinus, sinus));
    /* without -O3 a zero-tolereance would be OK */
    cpl_test_leq( 1.0 - cpl_vector_get(vxc1, 0), 144.0*DBL_EPSILON );

    cpl_test_zero( cpl_vector_correlate(vxc1, cosinus, cosinus) );
    xc = cpl_vector_get(vxc1, 0);
    cpl_test_abs( xc, 1.0, 5.0 * DBL_EPSILON );
    if (fabs(1-xc) > emax) emax = fabs(1-xc);

    if (VECTOR_SIZE % 2 == 0) {
        /* Sinus and cosinus have zero cross-correlation with zero shift */
        cpl_test_zero( cpl_vector_correlate(vxc1, sinus, cosinus) );
        xc = cpl_vector_get(vxc1, 0);
        cpl_test_leq( fabs(xc), 2.82*DBL_EPSILON );
        if (fabs(xc) > emax) emax = fabs(xc);
    }

    /*  cosinus and -cosinus have cross-correlation -1 with zero shift */
    tmp_vec = cpl_vector_duplicate(cosinus);
    cpl_test_vector_abs(tmp_vec, cosinus, 0.0);
    cpl_test_zero( cpl_vector_divide_scalar(tmp_vec, -1) );

    cpl_test_zero( cpl_vector_correlate(vxc1, tmp_vec, cosinus) );
    xc = cpl_vector_get(vxc1, 0);
    cpl_vector_delete(tmp_vec);
    cpl_test_abs( xc, -1.0, 5.0 * DBL_EPSILON );
    if (fabs(1+xc) > emax) emax = fabs(1+xc);

    vxc = cpl_vector_new( 1 );
    if (VECTOR_SIZE % 2 == 0) {
        /* Cross-correlation between sinus and cosinus grows to maximum
           at shift of pi/2 */
        for (i=0; i<VECTOR_SIZE/4; i++) {
            const double xcp = xc;
            half_search = i+1;

            cpl_test_zero( cpl_vector_set_size(vxc, 2*half_search + 1) );

            delta = cpl_vector_correlate(vxc, sinus, cosinus);
            xc = cpl_vector_get(vxc, delta);
            cpl_test( xc > xcp );
            cpl_test_eq( llabs(delta-(i+1)), i+1 );
        }
        cpl_test_abs( xc, 1.0, 260*DBL_EPSILON);
        half_search = VECTOR_SIZE/3;
        cpl_test_zero( cpl_vector_set_size(vxc, 2*half_search + 1) );
        delta = cpl_vector_correlate(vxc, sinus, cosinus);
        xc = cpl_vector_get(vxc, delta );
        cpl_test_eq( llabs(delta-VECTOR_SIZE/3), VECTOR_SIZE/4 );
        if (fabs(1-xc) > emax) emax = fabs(1-xc);
    }

    cpl_vector_delete(sinus);

    /* Vectors of almost the same length - no commutativity */

    /* Create the vector sinus */
    cpl_test_nonnull( sinus = cpl_vector_new(VECTOR_SIZE-VECTOR_CUT) );
    
    /* Fill the vector sinus */
    data = cpl_vector_get_data(sinus);
    cpl_test_nonnull( data );
    for (i=0; i<cpl_vector_get_size(sinus); i++)
        data[i] = cos(i*CPL_MATH_2PI/VECTOR_SIZE);

    /* Compare with no shift - other than half the length difference */
    half_search = VECTOR_SIZE;
    cpl_test_zero( cpl_vector_set_size(vxc, 2*half_search + 1) );

    delta = cpl_vector_correlate(vxc, cosinus, sinus);
    xc = cpl_vector_get(vxc, delta);
    delta -= VECTOR_SIZE;
    cpl_test_zero( delta + VECTOR_CUT/2);
    cpl_test_abs( xc, 1.0, 16.5 * DBL_EPSILON );
    if (fabs(1-xc) > emax) emax = fabs(1-xc);

    /* Compare with increasing shift and increasing drop of elements
       - only up to the length-difference */
    for (k = 1; k < vdif; k++) {

        for (i=0; i<cpl_vector_get_size(sinus); i++)
            data[i] = cos((i+k)*CPL_MATH_2PI/VECTOR_SIZE);

        delta = cpl_vector_correlate(vxc, cosinus, sinus);
        xc = cpl_vector_get(vxc, delta);
        delta -= VECTOR_SIZE;
        cpl_test_eq( delta + VECTOR_CUT/2, k );
        cpl_test_abs( xc, 1.0, 18.5 * DBL_EPSILON );
        if (fabs(1-xc) > emax) emax = fabs(1-xc);

    }

    /* Continue - maximum xc found with drop */
    for (; k < vdif; k++) {
        half_search = k-VECTOR_CUT/2;
        cpl_test_zero( cpl_vector_set_size(vxc, 2*half_search + 1) );

        for (i=0; i<cpl_vector_get_size(sinus); i++)
            data[i] = cos((i+k)*CPL_MATH_2PI/VECTOR_SIZE);

        delta = cpl_vector_correlate(vxc, cosinus, sinus);
        xc = cpl_vector_get(vxc, delta);
        delta -= half_search;
        cpl_test_abs( xc, 1.0, 25.0 * DBL_EPSILON );
        cpl_test_eq( delta + VECTOR_CUT/2, k );
        if (fabs(1-xc) > emax) emax = fabs(1-xc);
    }

    /* Compare with increasing negative shift and increasing drop of elements
       - only up to half the length-difference */
    half_search = VECTOR_CUT;
    cpl_test_zero( cpl_vector_set_size(vxc, 2*half_search + 1) );
    xc = 1;
    for (k = 1; k < vdif2; k++) {
        const double xcp = xc;

        for (i=0; i<cpl_vector_get_size(sinus); i++)
            data[i] = cos((i-k)*CPL_MATH_2PI/VECTOR_SIZE);

        delta = cpl_vector_correlate(vxc, cosinus, sinus);
        xc = cpl_vector_get(vxc, delta);
        delta -= half_search;
        cpl_test_leq( xc, xcp );
        cpl_test_leq( 0.0, delta + k + VECTOR_CUT/2 );
    }

    cpl_vector_delete(sinus);

    /* Vectors of the same length - commutativity */

    sinus = cpl_vector_duplicate(cosinus);
    cpl_test_vector_abs(sinus, cosinus, 0.0);

    half_search = VECTOR_CUT;
    cpl_test_zero( cpl_vector_set_size(vxc, 2*half_search + 1) );
    delta = cpl_vector_correlate(vxc, sinus, cosinus);
    xc = cpl_vector_get(vxc, delta);
    delta -= half_search;
    cpl_test_zero( delta );
    cpl_test_abs( xc, 1.0, 5.0 * DBL_EPSILON );
    if (fabs(1-xc) > emax) emax = fabs(1-xc);

    /* Verify commutativity */
    cpl_test_eq( delta+half_search, cpl_vector_correlate(vxc, cosinus, sinus) );
    cpl_test_eq( xc, cpl_vector_get(vxc, delta+half_search) );

    data = cpl_vector_get_data(sinus);
    cpl_test_nonnull( data );

    half_search = VECTOR_SIZE/2;
    cpl_test_zero( cpl_vector_set_size(vxc, 2*half_search + 1) );
    /* Compare with increasing shift and increasing drop of elements
       -  delta tests will not hold for large shifts */
    xc = 1;
    for (k = 1; k < VECTOR_SIZE/50; k+=7) {
        const double xcp = xc;
        double xcn;

        for (i=0; i<VECTOR_SIZE; i++) data[i] = cos((i+k)*CPL_MATH_2PI/VECTOR_SIZE);

        delta = cpl_vector_correlate(vxc, cosinus, sinus);
        xc = cpl_vector_get(vxc, delta);
        delta -= half_search;
        cpl_test_eq( k, delta );
        cpl_test( xc < xcp );

        /* Commutativity */
        delta = cpl_vector_correlate(vxc, sinus, cosinus);
        xcn = cpl_vector_get(vxc, delta);
        delta -= half_search;
        cpl_test_eq( k, -delta);
        cpl_test_abs( xcn, xc, 7.0 * DBL_EPSILON); /* SUSE 9.0 */

        /* Shift in opposite direction, i.e. reverse sign on k */
        for (i=0; i<VECTOR_SIZE; i++) data[i] = cos((i-k)*CPL_MATH_2PI/VECTOR_SIZE);

        delta = cpl_vector_correlate(vxc, cosinus, sinus);
        xc = cpl_vector_get(vxc, delta);
        delta -= half_search;
        cpl_test_zero( k + delta);
        cpl_test( xc < xcp );
        
    }

    half_search = VECTOR_SIZE;
    cpl_test_zero( cpl_vector_set_size(vxc, 2*half_search + 1) );

    /* Check with pseudo-random data */
    srand(1);
    for (i=0; i<VECTOR_SIZE; i++) data[i] = 2.0*cpl_drand() - 1.0;

    cpl_vector_copy(cosinus, sinus);

    cpl_test_eq( cpl_vector_correlate(vxc, cosinus, sinus), half_search );
    /* without -O3 a zero-tolereance would be OK */
    cpl_test_leq( 1.0 - cpl_vector_get(vxc, half_search), 3.5*DBL_EPSILON );

    half_search = VECTOR_SIZE/2;
    cpl_test_zero( cpl_vector_set_size(vxc, 2*half_search + 1) );

    for (k = 2; k < VECTOR_SIZE-2; k+=2) {
        double * pcosinus;

        cpl_vector_delete(cosinus);
        cosinus  = cpl_vector_new(VECTOR_SIZE-k);
        pcosinus = cpl_vector_get_data(cosinus);
        cpl_test_nonnull( pcosinus );

        for (i=0; i<VECTOR_SIZE-k; i++) pcosinus[i] = data[i];

        delta = cpl_vector_correlate(vxc, sinus, cosinus);
        xc = cpl_vector_get(vxc, delta);
        delta -= half_search;
        cpl_test_leq( delta, 0.0 );
        cpl_test_abs( xc, 1.0, 23.5 * DBL_EPSILON );
        if (fabs(1-xc) > emax) emax = fabs(1-xc);

    }

    cpl_msg_info("","Largest cross-correlation rounding error [DBL_EPSILON]: "
                 "%g", emax/DBL_EPSILON);

    if (do_bench) {

        cpl_vector_corr_bench(4 * npe);
        cpl_vector_get_stdev_bench(64 * npe);

        /* cpl_msg_set_component_on(); */
        cpl_vector_save_bench(200);
        /* cpl_msg_set_component_off(); */
    } else {
        cpl_vector_corr_bench(1);
        cpl_vector_get_stdev_bench(1);
        cpl_vector_save_bench(1);
    }

    if (do_bench) {
        cpl_vector_valarray_bench(10);
        cpl_vector_valarray_bench(50000);
        cpl_vector_valarray_bench(500000);
        cpl_vector_valarray_bench(5000000);
    } else {
        cpl_vector_valarray_bench(10);
    }

    if (getenv("CPL_VALARRAY_SIZE")) {
        const int nvalarray = atoi(getenv("CPL_VALARRAY_SIZE"));
        cpl_vector_valarray_bench(nvalarray);
    }

    /* Free and return */
    cpl_vector_delete(cosinus);
    cpl_vector_delete(sinus);

    cpl_vector_delete(vxc);
    cpl_vector_delete(vxc1);
    cpl_vector_delete(vxc3);

    if (stream != stdout) cpl_test_zero( fclose(stream) );

    /* End of tests */
    return cpl_test_end(0);
}

/**@}*/


/*----------------------------------------------------------------------------*/
/**
  @brief    Benchmark the CPL function
  @param n  The number of repeats
  @return   void
 */
/*----------------------------------------------------------------------------*/
static void cpl_vector_corr_bench(int n)
{

    double       secs;
    const cpl_size nsize   = 10*VECTOR_SIZE*VECTOR_SIZE;
    cpl_vector * cosinus = cpl_vector_new(nsize);
    cpl_vector * vxc = cpl_vector_new(5*VECTOR_SIZE | 1);
    double     * data = cpl_vector_get_data(cosinus);
    cpl_flops    flops0;
    const size_t bytes = (size_t)n * cpl_test_get_bytes_vector(cosinus);
    int          i;

    /* Fill the vector cosinus */
    for (i=0; i < nsize; i++) data[i] = cos(i*CPL_MATH_2PI/nsize);

    flops0 = cpl_tools_get_flops();
    secs  = cpl_test_get_walltime();

#ifdef _OPENMP
#pragma omp parallel for private(i)
#endif
    for (i = 0; i < n; i++) {
        cpl_vector_correlate(vxc, cosinus, cosinus);
    }

    secs = cpl_test_get_walltime() - secs;
    flops0 = cpl_tools_get_flops() - flops0;

    if (secs > 0.0) {
        cpl_msg_info("","Speed during %d correlations of size %" CPL_SIZE_FORMAT
                     " in %g secs [Mflop/s]: %g (%g)", n, nsize, secs,
                     flops0/secs/1e6, (double)flops0);
        cpl_msg_info(cpl_func,"Processing rate [MB/s]: %g",
                     1e-6 * (double)bytes / secs);
    }

    cpl_vector_delete(cosinus);
    cpl_vector_delete(vxc);

}
/*----------------------------------------------------------------------------*/
/**
  @brief    Benchmark the CPL function
  @param n  The number of repeats
  @return   void
 */
/*----------------------------------------------------------------------------*/
static void cpl_vector_get_stdev_bench(int n)
{

    double       secs;
    const cpl_size nsize   = 10 * VECTOR_SIZE*VECTOR_SIZE;
    cpl_vector * cosinus = cpl_vector_new(nsize);
    double     * data = cpl_vector_get_data(cosinus);
    cpl_flops    flops0;
    const size_t bytes = (size_t)n * cpl_test_get_bytes_vector(cosinus);
    int          i;

    /* Fill the vector cosinus */
    for (i=0; i < nsize; i++) data[i] = cos(i*CPL_MATH_2PI/nsize);

    flops0 = cpl_tools_get_flops();
    secs  = cpl_test_get_walltime();

#ifdef _OPENMP
#pragma omp parallel for private(i)
#endif
    for (i = 0; i < n; i++) {
        cpl_test_abs( cpl_vector_get_stdev(cosinus),
                      sqrt(nsize*0.5/(nsize - 1)),
                      0.36 * DBL_EPSILON * sqrt(nsize));
    }

    secs = cpl_test_get_walltime() - secs;
    flops0 = cpl_tools_get_flops() - flops0;

    if (secs > 0.0) {
        cpl_msg_info(cpl_func,"Speed during %d standard devs of size %"
                     CPL_SIZE_FORMAT "in %g secs [Mflop/s]: %g (%g)", n, nsize,
                     secs, flops0/secs/1e6, (double)flops0);
        cpl_msg_info(cpl_func,"Processing rate [MB/s]: %g",
                     1e-6 * (double)bytes / secs);
    }

    cpl_vector_delete(cosinus);

}

/*----------------------------------------------------------------------------*/
/**
  @brief    Benchmark the CPL function
  @param n  The number of repeats
  @return   void
 */
/*----------------------------------------------------------------------------*/
static void cpl_vector_save_bench(int n)
{

    const int          nprops = 100;
    int                i;
    double             secs;
    const char       * filename = "cpl_vector_save_bench.fits";
    const double       vval[] = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
    const int          nvals = (int)(sizeof(vval)/sizeof(double));
    cpl_propertylist * qclist = cpl_propertylist_new();
    char               key[81];
    const cpl_vector * vec;
    size_t bytes;


    CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual);
    vec = cpl_vector_wrap(nvals, (double*)vval);
    CPL_DIAG_PRAGMA_POP;
    bytes = (size_t)n * cpl_test_get_bytes_vector(vec);

    cpl_msg_info(cpl_func, "Benchmarking with %d %d-length vectors", n, nvals);

    for (i = 0; i < nprops; i++) {

        const int nlen = snprintf(key, 81, "ESO QC CARD%04d", i);

        cpl_test( nlen > 0 && nlen < 81);

        cpl_test_zero( cpl_propertylist_append_int(qclist, key, i));

    }

    cpl_test_eq( cpl_propertylist_get_size(qclist), nprops);


    secs  = cpl_test_get_cputime();

    for (i = 0; i < n; i++) {

       cpl_test_zero( cpl_vector_save(vec, filename, CPL_TYPE_DOUBLE,
                                           qclist, CPL_IO_CREATE));
    }

    secs = cpl_test_get_cputime() - secs;

    cpl_msg_info(cpl_func,"Time spent saving %d %d-sized vectors [s]: %g",
                 n, nvals, secs);

    if (secs > 0.0) {
        cpl_msg_info(cpl_func,"Processing rate [MB/s]: %g",
                     1e-6 * (double)bytes / secs);
    }
    cpl_test_fits(filename);
    cpl_test_zero( remove(filename) );

    CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual);
    cpl_vector_unwrap((cpl_vector*)vec);
    CPL_DIAG_PRAGMA_POP;

    cpl_propertylist_delete(qclist);

    return;

}


/*----------------------------------------------------------------------------*/
/**
  @brief    Reproduce DFS06126, original version by H. Lorch
  @param    stream Output dump stream
  @return   void
 */
/*----------------------------------------------------------------------------*/
static void cpl_vector_fit_gaussian_test_one(FILE * stream)
{

    const int       N = 50;
    cpl_vector      *yval = cpl_vector_new(N);
    cpl_vector      *xval = cpl_vector_new(N);
    cpl_vector      *ysig = cpl_vector_new(N);

    cpl_matrix      *cov    = NULL;
    cpl_matrix      *matrix = NULL;

    const double    in_sigma = 10.0,
        in_centre = 25.0,
        peak = 769.52;

    int             n;
    double          pos = 0.0,
        centre,
        offset,
        sigma,
        area,
        mse,
        chisq;
    cpl_error_code error;
    

    for (n = 0; n < N; n++) {
        const double d = (double)pos - in_centre;

        error = cpl_vector_set(xval, n, pos);
        cpl_test_eq_error(error, CPL_ERROR_NONE);
        error = cpl_vector_set(yval, n, peak*exp(-d*d/(2.0*in_sigma*in_sigma)));
        cpl_test_eq_error(error, CPL_ERROR_NONE);
            
        /* the following line seems to make it fail.
         * normally, it should have no influence at all since all sigmas
         * are the same. strangely, using 1.0/sqrt(N-1) also fails,
         * but modifying this value slightly (e.g. by adding 1e-6)
         * lets the fitting succeed. is there a meaning in the failure
         * for 1.0/sqrt(integer)? */
            
        error = cpl_vector_set(ysig, n, 1.0/sqrt(N));
        cpl_test_eq_error(error, CPL_ERROR_NONE);
            
        pos += 1.0;
        /* create one missing value,
         * this has no special meaning, just replicates the generation of
         * the test data with which I found the problem
         */
        if (n == 34)
            pos += 1.0;
    }


    cpl_vector_dump(xval, stream);
    cpl_vector_dump(yval, stream);
    cpl_vector_dump(ysig, stream);

    error = cpl_vector_fit_gaussian(xval,
                                    NULL,
                                    yval,
                                    ysig,
                                    CPL_FIT_ALL,
                                    &centre,
                                    &sigma,
                                    &area,
                                    &offset,
                                    &mse,
                                    &chisq,
                                    &cov);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_msg_info(cpl_func, "%d-length Gaussian fit, center: %g", N, centre);
    cpl_msg_info(cpl_func, "%d-length Gaussian fit, sigma:  %g", N, sigma);
    cpl_msg_info(cpl_func, "%d-length Gaussian fit, area:   %g", N, area);
    cpl_msg_info(cpl_func, "%d-length Gaussian fit, offset: %g", N, offset);
    cpl_msg_info(cpl_func, "%d-length Gaussian fit, MSE:    %g", N, mse);
    cpl_msg_info(cpl_func, "%d-length Gaussian fit, chisq:  %g", N, chisq);

    /* The covariance matrix must be 4 X 4, symmetric, positive definite */

    cpl_test_nonnull(cov);

    cpl_test_eq(cpl_matrix_get_nrow(cov), 4);
    cpl_test_eq(cpl_matrix_get_ncol(cov), 4);

    matrix = cpl_matrix_transpose_create(cov);
    cpl_test_matrix_abs(cov, matrix, DBL_EPSILON);

    error = cpl_matrix_decomp_chol(matrix);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_matrix_dump(cov, stream);

    cpl_vector_delete(yval);
    cpl_vector_delete(ysig);
    cpl_vector_delete(xval);
    cpl_matrix_delete(cov);
    cpl_matrix_delete(matrix);
    
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Perform a number of benchmarks
  @param a  One pre-allocated vector
  @param b  A second pre-allocated vector of the same length
  @return   void
 */
/*----------------------------------------------------------------------------*/
static void cpl_vector_valarray_bench_one(cpl_vector * a, cpl_vector * b)
{
  const cpl_size n = cpl_vector_get_size(a);
  double         cputime, tstop;
  const double   tstart = cpl_test_get_cputime();

  cpl_vector_add(a, b);
  cpl_vector_subtract(a, b);
  cpl_vector_multiply(a, b);
  cpl_vector_divide(a, b);
  cpl_vector_add_scalar(a, 10);
  cpl_vector_subtract_scalar(a, 10);
  cpl_vector_multiply_scalar(a, 10);
  cpl_vector_divide_scalar(a, 10);
  cpl_vector_get_min(a);
  cpl_vector_get_max(a);
  cpl_vector_get_sum(a);
  cpl_vector_get_mean(a);
  cpl_vector_get_stdev(a);

  tstop = cpl_test_get_cputime();
  cputime = tstop - tstart;

  cpl_msg_info(cpl_func, "valarray-test. n=%u. CPU-time [ms]: %g",
               (unsigned)n, 1e3 * cputime);

}

/*----------------------------------------------------------------------------*/
/**
  @brief    Perform a number of benchmarks of a given length
  @param n  The length of the vector(s) to benchmark
  @return   void
 */
/*----------------------------------------------------------------------------*/
static void cpl_vector_valarray_bench(cpl_size n)
{

    cpl_vector * a = cpl_vector_new(n);
    cpl_vector * b = cpl_vector_new(n);

    cpl_vector_fill(a, 10.0);
    cpl_vector_fill(b, 15.0);

    cpl_vector_valarray_bench_one(a, b);

    cpl_vector_delete(a);
    cpl_vector_delete(b);
}



/*----------------------------------------------------------------------------*/
/**
  @brief    Test the CPL function
  @param n  The length of the vector(s) to test
  @return   void
 */
/*----------------------------------------------------------------------------*/
static void cpl_vector_cycle_test(cpl_size n)
{

    const cpl_boolean do_plot = cpl_msg_get_level() <= CPL_MSG_INFO
        ? CPL_TRUE : CPL_FALSE;
    cpl_vector* src;
    cpl_vector* dest;
    cpl_error_code code;

    src  = cpl_vector_new(n);
    dest = cpl_vector_new(n + 1);

    /* Shift something non-constant, so the correctness can be verified */
    code = cpl_vector_fill_kernel_profile(src, CPL_KERNEL_SINC, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_vector_cycle(NULL, NULL, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_NULL_INPUT);

    code = cpl_vector_cycle(NULL, src, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_NULL_INPUT);

    code = cpl_vector_cycle(dest, src, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_INCOMPATIBLE_INPUT);

    cpl_vector_delete(dest);
    dest = cpl_vector_new(n);

    code = cpl_vector_cycle(dest, src, CPL_MATH_SQRT2 + (double)n/3.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    if (do_plot) {
        code = cpl_plot_vector("", "w lines", "", dest);
        cpl_test_error(code);
    }

    code = cpl_vector_cycle(dest, NULL, -CPL_MATH_SQRT2 - (double)n/3.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    if (cpl_tools_is_power_of_2(n)) {
        cpl_test_vector_abs(dest, src, 0.5 / n);
    } else {
        cpl_test_vector_abs(dest, src, 10.0 * DBL_EPSILON);
    }

    if (do_plot) {
        code = cpl_vector_subtract(dest, src);
        cpl_test_eq_error(code, CPL_ERROR_NONE);

        code = cpl_plot_vector("", "w lines", "", dest);
        cpl_test_error(code);
    }

    code = cpl_vector_cycle(dest, src, 0.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    cpl_test_vector_abs(dest, src, 0.0);

    code = cpl_vector_cycle(dest, NULL, 0.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    cpl_test_vector_abs(dest, src, 0.0);

    /* Should not alias input, but it is supported */
    code = cpl_vector_cycle(dest, dest, 0.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    cpl_test_vector_abs(dest, src, 0.0);

    code = cpl_vector_cycle(dest, src, -1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    if (n > 1) {
        cpl_test_abs(cpl_vector_get(dest, n-1), cpl_vector_get(src, 0), 0.0);
        cpl_test_abs(cpl_vector_get(dest, 0), cpl_vector_get(src, 1), 0.0);
    }

    code = cpl_vector_cycle(dest, NULL, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    cpl_test_vector_abs(dest, src, 0.0);

    code = cpl_vector_cycle(dest, NULL, -2.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    if (n > 2) {
        cpl_test_abs(cpl_vector_get(dest, n-2), cpl_vector_get(src, 0), 0.0);
        cpl_test_abs(cpl_vector_get(dest, 0), cpl_vector_get(src, 2), 0.0);
    }

    code = cpl_vector_cycle(dest, NULL,  2.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    cpl_test_vector_abs(dest, src, 0.0);

    /* Perform a range of shifts, that will cancel each other out */
    for (cpl_size i = -1 - 2 * n; i <= 1 + 2 * n; i++) {
        code = cpl_vector_cycle(dest, NULL, i);
        cpl_test_eq_error(code, CPL_ERROR_NONE);
    }
    cpl_test_vector_abs(dest, src, 0.0);

    /* Fill the vector with a sine curve
       - any linear combination of full sine and cosine curves
       will be cycled accurately... */

    for (cpl_size i=0; i < n; i++) {
        const double value = sin(i * CPL_MATH_2PI / n);
        code = cpl_vector_set(src, i, value);
        cpl_test_eq_error(code, CPL_ERROR_NONE);
    }

    code = cpl_vector_cycle(dest, src, CPL_MATH_E + (double)n/5.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_vector_cycle(dest, NULL, -CPL_MATH_E - (double)n/5.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_vector_abs(dest, src, 20.0 * DBL_EPSILON);

    /* Vectors of length 1 */

    code = cpl_vector_set_size(src, 1);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_vector_set_size(dest, 1);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_vector_cycle(dest, src, 0.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    cpl_test_vector_abs(dest, src, 0.0);

    code = cpl_vector_cycle(dest, src, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    cpl_test_vector_abs(dest, src, 0.0);

    code = cpl_vector_cycle(dest, src, -1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    cpl_test_vector_abs(dest, src, 0.0);

    cpl_vector_delete(src);
    cpl_vector_delete(dest);

}
