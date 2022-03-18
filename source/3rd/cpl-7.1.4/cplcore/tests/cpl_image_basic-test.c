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

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <complex.h>

#include "cpl_image_basic.h"
#include "cpl_image_stats.h"
#include "cpl_image_filter.h"
#include "cpl_image_gen.h"
#include "cpl_image_bpm.h"
#include "cpl_image_io.h"
#include "cpl_stats.h"
#include "cpl_vector.h"
#include "cpl_memory.h"
#include "cpl_test.h"
#include "cpl_math_const.h"
#include "cpl_tools.h"

/*-----------------------------------------------------------------------------
                                   Defines
 -----------------------------------------------------------------------------*/

#ifndef IMAGESZ
#define IMAGESZ            512
#endif
#define MAGIC_PIXEL        42

#define POLY_SIZE 24

/*-----------------------------------------------------------------------------
                        Private function prototypes
 -----------------------------------------------------------------------------*/

static void cpl_image_flip_turn_test(const cpl_image *);
static void cpl_image_turn_flip_test(void);
static void cpl_image_copy_test(void);
static void cpl_image_collapse_median_test(void);
static void cpl_image_complex_mult(size_t o1, size_t o2, size_t n);
static void cpl_image_complex_mult_d(size_t o1, size_t o2, size_t n);
static void cpl_image_get_fwhm_test(void);
static void cpl_image_divide_test_one(void);
static void cpl_image_power_test(void);
static void cpl_image_hypot_test(void);
static void cpl_image_bitwise_test(cpl_size, cpl_size);
static void cpl_image_divide_create_test(void);

/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/
int main(void)
{
    cpl_image       *   imf1;
    cpl_image       *   imf2;
    cpl_image       *   imd1;
    cpl_image       *   imd2;
    cpl_image       *   imi1;
    cpl_image       *   imi2;
    cpl_image       *   imf;
    cpl_image       *   imd;
    cpl_image       *   imi;
    cpl_image       *   imtmp;
    cpl_image       *   imfc1;
    cpl_image       *   imfc2;
    cpl_image       *   imfc3;
    float complex   *   dataf1;
    float complex   *   dataf2;
    float complex   *   dataf3;
    cpl_image       *   imdc1;
    cpl_image       *   imdc2;
    cpl_image       *   imdc3;
    double complex  *   data1;
    double complex  *   data2;
    double complex  *   data3;
    cpl_vector      *   taylor;
    double          *   ddata;
    int                 nb_cut;
    const cpl_size      new_pos[9] = {9, 8, 7, 6, 5, 4, 3, 2, 1};
    double              err;
    double              value;
    int                 is_rejected;
    cpl_mask        *   mask;
    int                 is_bad;
    int                 i, j, k;
    cpl_error_code      error;
    cpl_boolean         do_bench;

    /*
     * FIXME: Ideally, we should simply use the macro I 
     * (ISO/IEC 9899:1999(E) - Section 7.3.1 - Paragraph 4)
     * for arithmetics involving the imaginary unit.
     *
     * However, the usage of this macro I makes gcc issue the following
     *
     * warning: imaginary constants are a GCC extension
     * 
     * when the flag --pedantic is on.
     * 
     * This is a bug in gcc (seems to be related to Bugzilla #7263) and, as a
     * workaround in order to avoid that warning, we compute ourselves the
     * value of the macro.
     */    
    double complex      cpl_i = csqrt(-1.0);

    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    do_bench = cpl_msg_get_level() <= CPL_MSG_INFO ? CPL_TRUE : CPL_FALSE;

    /* Insert tests below */

    cpl_image_bitwise_test(IMAGESZ, IMAGESZ);
    cpl_image_bitwise_test(1, 1);
    cpl_image_bitwise_test(17, 15);
    cpl_image_bitwise_test(3, 32);
    if (do_bench) {
        double secs = cpl_test_get_walltime();
        for (i = 0; i < 2; i++) {
            cpl_image_bitwise_test(IMAGESZ/2, IMAGESZ/2);
            cpl_image_bitwise_test(IMAGESZ,   IMAGESZ);
            cpl_image_bitwise_test(IMAGESZ*2, IMAGESZ*2);
            cpl_image_bitwise_test(IMAGESZ*4, IMAGESZ*4);
        }
        secs = cpl_test_get_walltime() - secs;
        cpl_msg_info(cpl_func, "Time to test bit-wise calls [s]: %g", secs);
    }
    cpl_image_hypot_test();
    cpl_image_power_test();
    cpl_image_divide_test_one();

    cpl_image_get_fwhm_test();
    cpl_image_collapse_median_test();
    cpl_image_copy_test();
    cpl_image_turn_flip_test();

    cpl_image_divide_create_test();

    /* Tests of cpl_image_collapse_window_create() */
    /* Tests of cpl_image_collapse_create() */

    imtmp = cpl_image_collapse_create(NULL, 1);
    cpl_test_error( CPL_ERROR_NULL_INPUT );
    cpl_test_null( imtmp );
    
    imtmp = cpl_image_collapse_window_create(NULL, 1, 1, 1, 1, 1);
    cpl_test_error( CPL_ERROR_NULL_INPUT );
    cpl_test_null( imtmp );
    
    imd = cpl_image_new(IMAGESZ, IMAGESZ, CPL_TYPE_DOUBLE);

    imtmp = cpl_image_collapse_create(imd, -1);
    cpl_test_error( CPL_ERROR_ILLEGAL_INPUT );
    cpl_test_null( imtmp );

    /* Set two pixels in the last columns to 1 */
    cpl_test_zero(cpl_image_set(imd, IMAGESZ-1, IMAGESZ, 1.0));
    cpl_test_zero(cpl_image_set(imd, IMAGESZ,   IMAGESZ, 1.0));

    /* No BPM */
    /* Collapse the x-direction to an image with 1 row */

    imtmp = cpl_image_collapse_create(imd, 1);
    cpl_test_eq( cpl_image_get_type(imtmp), cpl_image_get_type(imd));

    cpl_test_eq( cpl_image_get_size_x(imtmp), 1);
    cpl_test_eq( cpl_image_get_size_y(imtmp), IMAGESZ);

    cpl_test_zero( cpl_image_get(imtmp, 1, IMAGESZ-1, &is_bad));
    cpl_test_zero( is_bad );
    cpl_test_eq( cpl_image_get(imtmp, 1, IMAGESZ, &is_bad), 2);
    cpl_test_zero( is_bad );

    cpl_image_delete(imtmp);

    imtmp = cpl_image_collapse_window_create(imd, IMAGESZ-1, IMAGESZ-1,
                                             IMAGESZ, IMAGESZ, 1);
    cpl_test_eq( cpl_image_get_type(imtmp), cpl_image_get_type(imd));

    cpl_test_eq( cpl_image_get_size_x(imtmp), 1);
    cpl_test_eq( cpl_image_get_size_y(imtmp), 2);

    cpl_test_zero( cpl_image_get(imtmp, 1, 1, &is_bad));
    cpl_test_zero( is_bad );
    cpl_test_eq( cpl_image_get(imtmp, 1, 2, &is_bad), 2);
    cpl_test_zero( is_bad );

    cpl_image_delete(imtmp);

    /* Collapse the y-direction to an image with 1 column */
    imtmp = cpl_image_collapse_create(imd, 0);
    cpl_test_eq( cpl_image_get_type(imtmp), cpl_image_get_type(imd));

    cpl_test_eq( cpl_image_get_size_x(imtmp), IMAGESZ);
    cpl_test_eq( cpl_image_get_size_y(imtmp), 1);

    cpl_test_eq( cpl_image_get(imtmp, IMAGESZ - 1, 1, &is_bad), 1);
    cpl_test_zero( is_bad );
    cpl_test_eq( cpl_image_get(imtmp, IMAGESZ,     1, &is_bad), 1);
    cpl_test_zero( is_bad );

    cpl_image_delete(imtmp);


    imtmp = cpl_image_collapse_window_create(imd, IMAGESZ-1, IMAGESZ-1,
                                             IMAGESZ, IMAGESZ, 0);
    cpl_test_eq( cpl_image_get_type(imtmp), cpl_image_get_type(imd));

    cpl_test_eq( cpl_image_get_size_x(imtmp), 2);
    cpl_test_eq( cpl_image_get_size_y(imtmp), 1);

    cpl_test_eq( cpl_image_get(imtmp, 1, 1, &is_bad), 1);
    cpl_test_zero( is_bad );
    cpl_test_eq( cpl_image_get(imtmp, 2, 1, &is_bad), 1);
    cpl_test_zero( is_bad );

    cpl_image_delete(imtmp);

    /* Force creation of BPM */

    mask = cpl_image_get_bpm(imd);

    /* Collapse the x-direction to an image with 1 row */

    imtmp = cpl_image_collapse_window_create(imd, IMAGESZ-1, IMAGESZ-1,
                                             IMAGESZ, IMAGESZ, 1);
    cpl_test_eq( cpl_image_get_type(imtmp), cpl_image_get_type(imd));

    cpl_test_eq( cpl_image_get_size_x(imtmp), 1);
    cpl_test_eq( cpl_image_get_size_y(imtmp), 2);

    cpl_test_zero( cpl_image_get(imtmp, 1, 1, &is_bad));
    cpl_test_zero( is_bad );
    cpl_test_eq( cpl_image_get(imtmp, 1, 2, &is_bad), 2);
    cpl_test_zero( is_bad );

    cpl_image_delete(imtmp);

    /* Collapse the y-direction to an image with 1 column */

    imtmp = cpl_image_collapse_window_create(imd, IMAGESZ-1, IMAGESZ-1,
                                             IMAGESZ, IMAGESZ, 0);
    cpl_test_eq( cpl_image_get_type(imtmp), cpl_image_get_type(imd));

    cpl_test_eq( cpl_image_get_size_x(imtmp), 2);
    cpl_test_eq( cpl_image_get_size_y(imtmp), 1);

    cpl_test_eq( cpl_image_get(imtmp, 1, 1, &is_bad), 1);
    cpl_test_zero( is_bad );
    cpl_test_eq( cpl_image_get(imtmp, 2, 1, &is_bad), 1);
    cpl_test_zero( is_bad );

    cpl_image_delete(imtmp);

    /* Flag the two zero pixels in the last row as bad */
    cpl_test_zero( cpl_mask_set(mask, IMAGESZ-1, IMAGESZ-1, CPL_BINARY_1));
    cpl_test_zero( cpl_mask_set(mask, IMAGESZ,   IMAGESZ-1, CPL_BINARY_1));

    /* Collapse the x-direction to an image with 1 row */

    imtmp = cpl_image_collapse_window_create(imd, IMAGESZ-1, IMAGESZ-1,
                                             IMAGESZ, IMAGESZ, 1);
    cpl_test_eq( cpl_image_get_type(imtmp), cpl_image_get_type(imd));

    cpl_test_eq( cpl_image_get_size_x(imtmp), 1);
    cpl_test_eq( cpl_image_get_size_y(imtmp), 2);

    value = cpl_image_get(imtmp, 1, 1, &is_bad);
    cpl_test( is_bad );
    cpl_test_eq( cpl_image_get(imtmp, 1, 2, &is_bad), 2);
    cpl_test_zero( is_bad );

    cpl_image_delete(imtmp);

    /* Collapse the y-direction to an image with 1 column */

    imtmp = cpl_image_collapse_window_create(imd, IMAGESZ-1, IMAGESZ-1,
                                             IMAGESZ, IMAGESZ, 0);
    cpl_test_eq( cpl_image_get_type(imtmp), cpl_image_get_type(imd));

    cpl_test_eq( cpl_image_get_size_x(imtmp), 2);
    cpl_test_eq( cpl_image_get_size_y(imtmp), 1);

    cpl_test_eq( cpl_image_get(imtmp, 1, 1, &is_bad), 2);
    cpl_test_zero( is_bad );
    cpl_test_eq( cpl_image_get(imtmp, 2, 1, &is_bad), 2);
    cpl_test_zero( is_bad );

    cpl_image_delete(imtmp);

 
    /* Finished testing cpl_image_collapse_window_create() */
    /* Finished testing cpl_image_collapse_create() */

    cpl_image_delete(imd);

    /* Test for DFS 1724 */
    imd1 = cpl_image_new(2, 2, CPL_TYPE_DOUBLE);
    imd2 = cpl_image_new(2, 2, CPL_TYPE_DOUBLE);
    imi1 = cpl_image_new(2, 2, CPL_TYPE_INT);
    imi2 = cpl_image_new(2, 2, CPL_TYPE_INT);

    /* Fill images with values 0, 1, 2, 3 */
    value = 0.0;
    for (i=1; i <= 2; i++) {
        for (j=1; j <= 2; j++) {
            cpl_test_zero( cpl_image_set(imd1, i, j, value));
            cpl_test_zero( cpl_image_set(imd2, i, j, value));
            cpl_test_zero( cpl_image_set(imi1, i, j, value));
            cpl_test_zero( cpl_image_set(imi2, i, j, value));
            value++;
        }
    }
    /* Test cpl_image_divide_create() on integer and double */
    imd = cpl_image_divide_create(imd1, imd2);
    cpl_test_nonnull( imd);
    imi = cpl_image_divide_create(imi1, imi2);
    cpl_test_nonnull( imi);

    value = 0.0;
    for (i=1; i <= 2; i++) {
        for (j=1; j <= 2; j++, value++) {
            const double vald  = cpl_image_get(imd,  i, j, &is_bad);
            const double vali  = cpl_image_get(imi,  i, j, &is_bad);

            if (value != 0.0) cpl_test_abs(vald, 1.0, 0.0);
            if (value != 0.0) cpl_test_abs(vali, 1.0, 0.0);
        }
    }

    /* Test with lo_cut equal to hi_cut */
    error = cpl_image_threshold(imd1, (double)MAGIC_PIXEL, (double)MAGIC_PIXEL,
                                (double)MAGIC_PIXEL, (double)MAGIC_PIXEL);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_leq( cpl_image_get_max(imd1), (double)MAGIC_PIXEL);
    cpl_test_leq( (double)MAGIC_PIXEL, cpl_image_get_min(imd1));

    cpl_image_delete(imd1);
    cpl_image_delete(imd2);
    cpl_image_delete(imd);
    cpl_image_delete(imi1);
    cpl_image_delete(imi2);
    cpl_image_delete(imi);

    /* Test for DFS02782 */
    imi = cpl_image_new(1, 1, CPL_TYPE_INT);
    imd = cpl_image_new(1, 1, CPL_TYPE_DOUBLE);
    cpl_image_set(imi, 1, 1, 10);
    cpl_image_set(imd, 1, 1, 0.5);
    imi1 = cpl_image_divide_create(imi, imd);
    cpl_image_delete(imi);
    cpl_image_delete(imd);
    value = cpl_image_get(imi1, 1, 1, &is_bad);
    cpl_test_abs(value, 20.0, 0.0);
    cpl_image_delete(imi1);
    
    imd = cpl_image_new(IMAGESZ, IMAGESZ, CPL_TYPE_DOUBLE);

    /* Fill the image with a sinus */
    ddata = cpl_image_get_data(imd);
    cpl_test_nonnull( ddata );
    for (i=0; i < IMAGESZ; i++) {
        for (j=0; j < IMAGESZ; j++) {
            value = sin(i*CPL_MATH_2PI/IMAGESZ) * sin(j*CPL_MATH_2PI/IMAGESZ);
            if (cpl_image_set(imd, i+1, j+1, value)) break;
            if ( ddata[i + j * IMAGESZ] != value ) break;
            if (cpl_image_get(imd, i+1, j+1, &is_rejected) != value) break;
            if (is_rejected) break; /* Not really useful */
        }
        cpl_test_eq( j, IMAGESZ);
    }

    imd1 = cpl_image_duplicate(imd);
    cpl_test_nonnull( imd1 );

    cpl_test_zero( cpl_image_fft(imd1, NULL, CPL_FFT_DEFAULT) );

    cpl_test_zero( cpl_image_abs(imd1) );

    /* 4 FFT-components are non-zero */
    value = IMAGESZ*IMAGESZ/4;

    err = fabs(cpl_image_get(imd1, 2, 2, &is_rejected) - value);
    cpl_test_zero( cpl_image_set(imd1, 2, 2, err) );

    err = fabs(cpl_image_get(imd1, 2, IMAGESZ, &is_rejected) - value);
    cpl_test_zero( cpl_image_set(imd1, 2, IMAGESZ, err) );

    err = fabs(cpl_image_get(imd1, IMAGESZ, 2, &is_rejected) - value);
    cpl_test_zero( cpl_image_set(imd1, IMAGESZ, 2, err) );

    err = fabs(cpl_image_get(imd1, IMAGESZ, IMAGESZ, &is_rejected) - value);
    cpl_test_zero( cpl_image_set(imd1, IMAGESZ, IMAGESZ, err) );

    err = cpl_image_get_max(imd1);

    cpl_msg_info("", "FFT(%g) round-off [DBL_EPSILON]: %g < %d", value,
                  err/DBL_EPSILON, IMAGESZ*IMAGESZ);

    cpl_test_leq( fabs(err), IMAGESZ*IMAGESZ * DBL_EPSILON);

    cpl_image_delete(imd1);

    /* Create a Taylor-expansion of exp() */
    cpl_test_nonnull( taylor = cpl_vector_new(POLY_SIZE) );
    i = 0;
    cpl_vector_set(taylor, i, 1);
    for (i=1; i<POLY_SIZE; i++)
        cpl_vector_set(taylor, i, cpl_vector_get(taylor, i-1)/i);

    /* Evaluate exp() using Horners scheme on the Taylor expansion */
    imd2 = cpl_image_new(IMAGESZ, IMAGESZ, CPL_TYPE_DOUBLE);
    cpl_test_nonnull( imd2 );
    cpl_test_zero( cpl_image_add_scalar(imd2, cpl_vector_get(taylor,
                                                             POLY_SIZE-1)) );
    
    for (k=POLY_SIZE-1; k > 0; k--) {
        cpl_test_zero( cpl_image_multiply(imd2, imd) );
        if (k&1) {
          cpl_test_zero( cpl_image_add_scalar(imd2,
                                         cpl_vector_get(taylor, k-1)) );
        } else {
          cpl_test_zero( cpl_image_subtract_scalar(imd2,
                                              -cpl_vector_get(taylor, k-1)) );
        }
    }        

    /* Verify the result (against cpl_image_exponential() ) */
    imd1 = cpl_image_duplicate(imd);
    cpl_test_nonnull( imd1 );

    cpl_test_zero( cpl_image_exponential(imd1, CPL_MATH_E) );

    cpl_test_zero( cpl_image_subtract(imd2, imd1) );
    cpl_test_zero( cpl_image_divide(imd2, imd1) );
    cpl_test_zero( cpl_image_divide_scalar(imd2, DBL_EPSILON) );

    cpl_test_leq( fabs(cpl_image_get_max(imd2)), 2.64494 );
    cpl_test_leq( fabs(cpl_image_get_min(imd2)), 2.03626 );

    cpl_image_delete(imd2);
    /* Evaluate exp() using cpl_image_pow() on the Taylor expansion */
    imd2 = cpl_image_new(IMAGESZ, IMAGESZ, CPL_TYPE_DOUBLE);
    cpl_test_nonnull( imd2 );
    cpl_test_zero( cpl_image_add_scalar(imd2, cpl_vector_get(taylor, 0)) );
    
    /* POLY_SIZE > 10 on alphaev56:
        Program received signal SIGFPE, Arithmetic exception.
          0x200000a3ff0 in cpl_vector_multiply_scalar ()
    */
    for (k=1; k < POLY_SIZE; k++) {
        imtmp = cpl_image_duplicate(imd);
        cpl_test_zero( cpl_image_power(imtmp, k) );
        cpl_test_zero( cpl_image_multiply_scalar(imtmp, cpl_vector_get(taylor, k)) );
        cpl_test_zero( cpl_image_add(imd2, imtmp) );
        cpl_image_delete(imtmp);
    }        

    cpl_vector_delete(taylor);

    /* Much less precise than Horner ... */
    cpl_test_image_abs(imd2, imd1, 16 * DBL_EPSILON);

    /* Verify cpl_image_logarithm() ) */
    cpl_test_zero( cpl_image_logarithm(imd1, 10) );
    cpl_test_zero( cpl_image_multiply_scalar(imd1, CPL_MATH_LN10) );

    cpl_test_image_abs(imd, imd1, 3 * DBL_EPSILON);

    cpl_test_zero( cpl_image_exponential(imd, CPL_MATH_E) );

    cpl_test_zero( cpl_image_copy(imd1, imd,1,1) );

    cpl_test_zero( cpl_image_fft(imd1, NULL, CPL_FFT_DEFAULT) );
    cpl_image_delete(imd2);
    imd2 = cpl_image_duplicate(imd1);
    cpl_test_nonnull( imd2 );
    imtmp = cpl_image_duplicate(imd1);
    cpl_test_nonnull( imtmp );
    cpl_test_zero( cpl_image_copy(imd1, imd2, 1, 1) );

    cpl_test_zero( cpl_image_add(imd1, imd2) );
    cpl_test_zero( cpl_image_add(imd1, imd2) );
    cpl_test_zero( cpl_image_add(imd1, imd2) );
    cpl_test_zero( cpl_image_subtract(imd1, imd2) );
    cpl_test_zero( cpl_image_multiply_scalar(imtmp, 3) );

    cpl_test_eq(cpl_image_count_rejected(imtmp), 0);

    cpl_test_zero( cpl_image_divide(imtmp, imd1) );

    cpl_test_zero( cpl_image_abs(imtmp) );

    cpl_test_abs( modf(IMAGESZ*IMAGESZ - cpl_image_get_flux(imtmp),
                                  &err), 0.0, DBL_EPSILON);

    cpl_test_eq(cpl_image_count_rejected(imtmp), err);

    cpl_test_zero( cpl_image_fft(imd1, NULL, CPL_FFT_INVERSE) );

    cpl_test_zero( cpl_image_divide_scalar(imd1, 3) );

    cpl_test_image_abs(imd, imd1, 32 * DBL_EPSILON);

    cpl_test_zero( cpl_image_copy(imd1, imd, 1, 1) );

    cpl_test_zero( cpl_image_fft(imd1, NULL, CPL_FFT_INVERSE) );
    cpl_test_zero( cpl_image_fft(imd1, NULL, CPL_FFT_DEFAULT) );

    cpl_test_image_abs(imd, imd1, 32 * DBL_EPSILON);

    cpl_image_delete(imd);
    cpl_image_delete(imd1);
    cpl_image_delete(imd2);
    cpl_image_delete(imtmp);

    /* Create test images */
    imd1 = cpl_image_new(IMAGESZ, IMAGESZ, CPL_TYPE_DOUBLE);
    cpl_image_fill_noise_uniform(imd1, 1, 100);
    imf1 = cpl_image_cast(imd1, CPL_TYPE_FLOAT);
    imi1 = cpl_image_cast(imd1, CPL_TYPE_INT);

    cpl_image_flip_turn_test(imd1);
    cpl_image_flip_turn_test(imf1);
    cpl_image_flip_turn_test(imi1);

    imd2 = cpl_image_fill_test_create(IMAGESZ, IMAGESZ);
    cpl_image_save(imd2, "test.fits", CPL_TYPE_FLOAT, NULL,
                   CPL_IO_CREATE);
    remove("test.fits");
    imf2 = cpl_image_cast(imd2, CPL_TYPE_FLOAT);
    imi2 = cpl_image_cast(imd2, CPL_TYPE_INT);

    cpl_test_nonnull( imd1 );
    cpl_test_nonnull( imd2 );

    cpl_test_nonnull( imf1 );
    cpl_test_nonnull( imf2 );

    cpl_test_nonnull( imi1 );
    cpl_test_nonnull( imi2 );
  
    /* COMPUTE imi =(imi1-imi2)*imi1/(imi1+imi2) with local functions */
    cpl_msg_info("","Compute imi = (imi1-imi2)*imi1/(imi1+imi2) in INTEGER");
    imi = cpl_image_duplicate(imi1);
    cpl_test_zero( cpl_image_subtract(imi, imi2) );
    cpl_test_zero( cpl_image_multiply(imi, imi1) );
    imtmp = cpl_image_duplicate(imi1);
    cpl_test_zero( cpl_image_add(imtmp, imi2) );
    cpl_test_zero( cpl_image_divide(imi, imtmp) );
    cpl_image_delete(imtmp);
    
    /* Delete imi1 and imi2 */
    cpl_image_delete(imi1); 
    cpl_image_delete(imi2); 

    /* COMPUTE imf =(imf1-imf2)*imf1/(imf1+imf2) with local functions */
    cpl_msg_info("","Compute imf = (imf1-imf2)*imf1/(imf1+imf2) in FLOAT");
    imf = cpl_image_duplicate(imf1);
    cpl_test_zero( cpl_image_subtract(imf, imf2) );
    cpl_test_zero( cpl_image_multiply(imf, imf1) );
    imtmp = cpl_image_duplicate(imf1);
    cpl_test_zero( cpl_image_add(imtmp, imf2) );
    cpl_test_zero( cpl_image_divide(imf, imtmp) );
    cpl_image_delete(imtmp);
    
    /* Delete imf1 and imf2 */
    cpl_image_delete(imf1); 
    cpl_image_delete(imf2); 

    /* COMPUTE imd =(imd1-imd2)*imd1/(imd1+imd2) with local functions */
    cpl_msg_info("","Compute imd = (imd1-imd2)*imd1/(imd1+imd2) in DOUBLE");
    imd = cpl_image_duplicate(imd1);
    cpl_test_zero( cpl_image_subtract(imd, imd2) );
    cpl_test_zero( cpl_image_multiply(imd, imd1) );
    imtmp = cpl_image_duplicate(imd1);
    cpl_test_zero( cpl_image_add(imtmp, imd2) );
    cpl_test_zero( cpl_image_divide(imd, imtmp) );
    cpl_image_delete(imtmp);
    
    /* Delete imd1 and imd2 */
    cpl_image_delete(imd1); 
    cpl_image_delete(imd2); 

    /* At this point imi, imf and imd are int, resp float and double type */
    /* images with the same values - apart from differences in rounding. */

    /* Test cpl_image_average_create() */
    cpl_image_delete( cpl_image_average_create(imf, imd) );
    
    /* Test collapse functions */
    /* Set all pixels as bad */
    mask = cpl_mask_new(IMAGESZ, IMAGESZ);
    cpl_mask_not(mask);
    cpl_image_reject_from_mask(imd, mask);
    cpl_mask_delete(mask);
    /* Collapse with bad pixels */
    imtmp = cpl_image_collapse_create(imd, 0);
    cpl_test_eq( cpl_mask_count(cpl_image_get_bpm(imtmp)), IMAGESZ);
    
    cpl_image_delete(imtmp);
    /* Reset the bad pixlels in imd */
    cpl_image_accept_all(imd);
    /* Collapse */
    imtmp = cpl_image_collapse_create(imd, 0);
    cpl_test_zero( cpl_mask_count(cpl_image_get_bpm(imtmp)));
    cpl_image_delete(imtmp);
    /* Median collapse */
    imtmp = cpl_image_collapse_median_create(imf, 0, 10, 10);
    cpl_image_delete(imtmp);
    
    /* imf=((((imf+MAGIC_PIXEL)*MAGIC_PIXEL)-MAGIC_PIXEL)/-MAGIC_PIXEL)*/
    /* with non local functions */
    cpl_msg_info("","Compute imf = ((((imf+42)*42)-42)/-42)");
    cpl_test_zero( cpl_image_add_scalar(imf, (double)MAGIC_PIXEL) );
    cpl_test_zero( cpl_image_multiply_scalar(imf, (double)MAGIC_PIXEL) );
    cpl_test_zero( cpl_image_subtract_scalar(imf, (double)MAGIC_PIXEL) );
    cpl_test_zero( cpl_image_divide_scalar(imf, -(double)MAGIC_PIXEL) );

    /* imd=((((imd+MAGIC_PIXEL)*MAGIC_PIXEL)-MAGIC_PIXEL)/-MAGIC_PIXEL)*/
    /* with local functions */
    cpl_msg_info("","Compute imd = ((((imd+42)*42)-42)/-42)");
    cpl_test_zero( cpl_image_add_scalar(imd, (double)MAGIC_PIXEL) );
    cpl_test_zero( cpl_image_multiply_scalar(imd, (double)MAGIC_PIXEL) );
    cpl_test_zero( cpl_image_subtract_scalar(imd, (double)MAGIC_PIXEL) );
    cpl_test_zero( cpl_image_divide_scalar(imd, -(double)MAGIC_PIXEL) );

    /* imi=((((imi+MAGIC_PIXEL)*MAGIC_PIXEL)-MAGIC_PIXEL)/-MAGIC_PIXEL)*/
    /* with local functions */
    cpl_msg_info("","Compute imi = ((((imi+42)*42)-42)/-42)");
    cpl_test_zero( cpl_image_add_scalar(imi, (double)MAGIC_PIXEL) );
    cpl_test_zero( cpl_image_multiply_scalar(imi, (double)MAGIC_PIXEL) );
    cpl_test_zero( cpl_image_subtract_scalar(imi, (double)MAGIC_PIXEL) );
    cpl_test_zero( cpl_image_divide_scalar(imi, -(double)MAGIC_PIXEL) );
  
    /* Compute imi = |imi|, imf = |imf| and imd = |imd| */
    cpl_msg_info("","Take the absolute value of imi, imf and imd");
    imtmp = cpl_image_abs_create(imi);
    cpl_image_delete(imi); 
    imi = imtmp;
    imtmp = NULL;
    imtmp = cpl_image_abs_create(imf);
    cpl_image_delete(imf); 
    imf = imtmp;
    imtmp = NULL;
    imtmp = cpl_image_abs_create(imd);
    cpl_image_delete(imd); 
    imd = imtmp;
    imtmp = NULL;
   
    /* Test cpl_image_move() */
    cpl_msg_info("","Test the pixels moving function on imd");
    imd1 = cpl_image_extract(imd, 11, 11, 13, 16);
    nb_cut = 3;

    error = cpl_image_move(imd1, nb_cut, new_pos);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_image_delete(imd1);

    /* Test extraction w. a bad pixel */
    cpl_test_zero(cpl_image_reject(imd, 12, 14));
    cpl_test_eq(cpl_image_count_rejected(imd), 1);
    imd1 = cpl_image_extract(imd, 11, 11, 13, 16);
    cpl_test_eq(cpl_image_get_size_x(imd1), 3);
    cpl_test_eq(cpl_image_get_size_y(imd1), 6);
    cpl_test(cpl_image_is_rejected(imd1, 2, 4));
    cpl_image_delete(imd1);

    /* Threshold imi, imf and imd */
    cpl_msg_info("","Threshold imi, imf and imd");
    cpl_image_threshold(imi, -DBL_MAX, (double)MAGIC_PIXEL, 
            0.0, (double)MAGIC_PIXEL);
    cpl_image_threshold(imf, -DBL_MAX, (double)MAGIC_PIXEL, 
            0.0, (double)MAGIC_PIXEL);
    cpl_image_threshold(imd, -DBL_MAX, (double)MAGIC_PIXEL, 
            0.0, (double)MAGIC_PIXEL);

    /* Subtract the min of the image from the image -> imi, imf and imd */
    cpl_msg_info("","Subtract the minimum for imi, imf and imd");
    cpl_test_zero( cpl_image_subtract_scalar(imi, cpl_image_get_min(imi)) );
    cpl_test_zero( cpl_image_subtract_scalar(imf, cpl_image_get_min(imf)) );
    cpl_test_zero( cpl_image_subtract_scalar(imd, cpl_image_get_min(imd)) );

    /* Extract sub windows from imf and imd */
    cpl_msg_info("","Extract the central part of imi, imf and imd");
    imtmp = cpl_image_extract(imi, IMAGESZ/4, IMAGESZ/4, IMAGESZ/2, IMAGESZ/2);
    cpl_test_nonnull( imtmp );
    cpl_image_delete(imi); 
    imi = imtmp;
    imtmp = NULL;
    imtmp = cpl_image_extract(imf, IMAGESZ/4, IMAGESZ/4, IMAGESZ/2, IMAGESZ/2);
    cpl_test_nonnull( imtmp );
    cpl_image_delete(imf); 
    imf = imtmp;
    imtmp = NULL;
    imtmp = cpl_image_extract(imd, IMAGESZ/4, IMAGESZ/4, IMAGESZ/2, IMAGESZ/2);
    cpl_test_nonnull( imtmp );
    cpl_image_delete(imd); 
    imd = imtmp;
    imtmp = NULL;

    /* Test exp operation */
    cpl_msg_info("","Compute exp(imi), exp(imf) and exp(imd)");
    cpl_test_zero( cpl_image_exponential(imi, CPL_MATH_E) );
    cpl_test_zero( cpl_image_exponential(imf, CPL_MATH_E) );
    cpl_test_zero( cpl_image_exponential(imd, CPL_MATH_E) );
    
    /* Test log operation */
    cpl_msg_info("","Compute log(imi), log(imf) and log(imd)");
    cpl_test_zero( cpl_image_logarithm(imi, CPL_MATH_E) );
    cpl_test_zero( cpl_image_logarithm(imf, CPL_MATH_E) );
    cpl_test_zero( cpl_image_logarithm(imd, CPL_MATH_E) );
    
    /* Test ^ operation */
    cpl_msg_info("","Compute imi^2, imf^2 and imd^2");
    cpl_test_zero( cpl_image_power(imi, 2.0) );
    cpl_test_zero( cpl_image_power(imf, 2.0) );
    cpl_test_zero( cpl_image_power(imd, 2.0) );
    
    /* Test sqrt operation */
    cpl_msg_info("","Compute imf^0.5 and imd^0.5");
    cpl_test_zero( cpl_image_power(imf, 0.5) );
    cpl_test_zero( cpl_image_power(imd, 0.5) );

    /* TEST CROSS-TYPES OPERATIONS */
    cpl_msg_info("","Test cross types operations");
    imtmp = cpl_image_add_create(imf, imd);
    cpl_msg_info("","Get mean of imf + imd : %g", cpl_image_get_mean(imtmp));
    cpl_image_delete(imtmp);
    imtmp = cpl_image_add_create(imd, imf);
    cpl_msg_info("","Get mean of imd + imf : %g", cpl_image_get_mean(imtmp));
    cpl_image_delete(imtmp);
    imtmp = cpl_image_subtract_create(imf, imd);
    cpl_msg_info("","Get mean of imf - imd : %g", cpl_image_get_mean(imtmp));
    cpl_image_delete(imtmp);
    imtmp = cpl_image_subtract_create(imd, imf);
    cpl_msg_info("","Get mean of imd - imf : %g", cpl_image_get_mean(imtmp));
    cpl_image_delete(imtmp);
    imtmp = cpl_image_multiply_create(imf, imd);
    cpl_msg_info("","Get mean of imf * imd : %g", cpl_image_get_mean(imtmp));
    cpl_image_delete(imtmp);
    imtmp = cpl_image_multiply_create(imd, imf);
    cpl_msg_info("","Get mean of imd * imf : %g", cpl_image_get_mean(imtmp));
    cpl_image_delete(imtmp);
    imtmp = cpl_image_divide_create(imf, imd);
    cpl_msg_info("","Get mean of imf / imd : %g", cpl_image_get_mean(imtmp));
    cpl_image_delete(imtmp);
    imtmp = cpl_image_divide_create(imd, imf);
    cpl_msg_info("","Get mean of imd / imf : %g", cpl_image_get_mean(imtmp));
    cpl_image_delete(imtmp);
    
    /* Normalize imf and imd */
    cpl_msg_info("","Normalize imf and imd");
    imtmp = cpl_image_normalise_create(imf, CPL_NORM_MEAN);
    cpl_test_nonnull( imtmp );
    cpl_image_delete(imf); 
    imf = imtmp;
    imtmp = NULL;
    imtmp = cpl_image_normalise_create(imd, CPL_NORM_MEAN);
    cpl_test_nonnull( imtmp );
    cpl_image_delete(imd); 
    imd = imtmp;
    imtmp = NULL;
  
    /* Test the insertion function */
    cpl_msg_info("","Insert an image in an other one");
    imtmp = cpl_image_duplicate(imd);
    cpl_image_reject(imtmp, 1, 1);
    cpl_test_zero( cpl_image_copy(imd, imtmp, 100, 100) );
    cpl_test(cpl_image_is_rejected(imd, 100, 100));
    cpl_image_delete(imtmp);
    
    /* Test the shift function */
    cpl_msg_info("","Compute various shifts on imi, imf and imda");
    cpl_test_zero( cpl_image_shift(imd, 1, 1) );
    cpl_test_zero( cpl_image_shift(imf, 1, 1) );
    cpl_test_zero( cpl_image_shift(imi, 1, 1) );

    cpl_image_delete(imi);
    cpl_image_delete(imf);
    cpl_image_delete(imd);

    /* Test for DFS02049 : Handle the division by ZERO */
    /* Divide 3 | 4    by    2 | 0   -->  1.5 | X   */
           /* -   -          -   -          -   -   */
           /* 2 | 3          2 | 0          1 | X   */
    imtmp = cpl_image_new(2, 2, CPL_TYPE_DOUBLE);
    imd = cpl_image_new(2, 2, CPL_TYPE_DOUBLE);
    imf = cpl_image_new(2, 2, CPL_TYPE_FLOAT);
    imi = cpl_image_new(2, 2, CPL_TYPE_INT);
    for (i=0; i<2; i++) {
        for (j=0; j<2; j++) {
            if (i==0) cpl_image_set(imtmp, i+1, j+1, 2.0);
            cpl_image_set(imi, i+1, j+1, (double)(i+j+2));
            cpl_image_set(imf, i+1, j+1, (double)(i+j+2));
            cpl_image_set(imd, i+1, j+1, (double)(i+j+2));
        }
    } 
    imd1 = cpl_image_divide_create(imd, imtmp);
    imf1 = cpl_image_divide_create(imf, imtmp);
    imi1 = cpl_image_divide_create(imi, imtmp);
    cpl_test_nonnull( imd1 );
    cpl_test_nonnull( imf1 );
    cpl_test_nonnull( imi1 );

    cpl_test_abs( cpl_image_get_flux(imd1), 2.5, 0.0 );
    cpl_test_abs( cpl_image_get_flux(imf1), 2.5, 0.0 );
    cpl_test_abs( cpl_image_get_flux(imi1), 2.0, 0.0 );
    cpl_image_delete(imd1);
    cpl_image_delete(imf1);
    cpl_image_delete(imi1);

    cpl_test_zero( cpl_image_divide(imd, imtmp) );
    cpl_test_zero( cpl_image_divide(imf, imtmp) );
    cpl_test_zero( cpl_image_divide(imi, imtmp) );

    cpl_test_abs( cpl_image_get_flux(imd), 2.5, 0.0 );
    cpl_test_abs( cpl_image_get_flux(imf), 2.5, 0.0 );
    cpl_test_abs( cpl_image_get_flux(imi), 2.0, 0.0 );

    cpl_image_delete(imtmp);
    cpl_image_delete(imd);
    cpl_image_delete(imf);
    cpl_image_delete(imi);

    /* Test 0^0 operation */
    imf = cpl_image_new(IMAGESZ, IMAGESZ, CPL_TYPE_FLOAT);
    imd = cpl_image_new(IMAGESZ, IMAGESZ, CPL_TYPE_DOUBLE);

    imf1 = cpl_image_new(IMAGESZ, IMAGESZ, CPL_TYPE_FLOAT);
    imd1 = cpl_image_new(IMAGESZ, IMAGESZ, CPL_TYPE_DOUBLE);

    cpl_msg_info("","Compute imf(0.0)^0.0");
    cpl_test_zero( cpl_image_power(imf, 0.0) );
    cpl_test_zero( cpl_image_power(imd, 0.0) );
    cpl_test_zero( cpl_image_subtract_scalar(imf, 1.0) );
    cpl_test_zero( cpl_image_subtract_scalar(imd, 1.0) );

    cpl_test_image_abs(imf, imf1, 0.0);
    cpl_test_image_abs(imd, imd1, 0.0);

    cpl_image_delete(imf);
    cpl_image_delete(imd);
    cpl_image_delete(imf1);
    cpl_image_delete(imd1);

    /* New tests for complex images */

    /* DOUBLE */

    imdc1 = cpl_image_new(IMAGESZ, IMAGESZ, CPL_TYPE_DOUBLE_COMPLEX);
    imdc2 = cpl_image_new(IMAGESZ, IMAGESZ, CPL_TYPE_DOUBLE_COMPLEX);

    data1 = cpl_image_get_data_double_complex(imdc1);
    data2 = cpl_image_get_data_double_complex(imdc2);

    for (i = 0; i < IMAGESZ * IMAGESZ; i++) {
    data1[i] =     i +     i * cpl_i;
    data2[i] = 2 * i + 2 * i * cpl_i;
    }

    imdc3 = cpl_image_add_create(imdc1, imdc2);

    data3 = cpl_image_get_data_double_complex(imdc3);

    for (i = 0; i < IMAGESZ * IMAGESZ; i++) {
        cpl_test_abs( data3[i], 3 * i + 3 * i * cpl_i, 0.0 );
    }

    cpl_image_delete(imdc1);
    cpl_image_delete(imdc2);
    cpl_image_delete(imdc3);

    /* FLOAT */

    imfc1 = cpl_image_new(IMAGESZ, IMAGESZ, CPL_TYPE_FLOAT_COMPLEX);
    imfc2 = cpl_image_new(IMAGESZ, IMAGESZ, CPL_TYPE_FLOAT_COMPLEX);

    dataf1 = cpl_image_get_data_float_complex(imfc1);
    dataf2 = cpl_image_get_data_float_complex(imfc2);

    for (i = 0; i < IMAGESZ * IMAGESZ; i++) {
    dataf1[i] =     i +     i * cpl_i;
    dataf2[i] = 2 * i + 2 * i * cpl_i;
    }

    imfc3 = cpl_image_add_create(imfc1, imfc2);

    dataf3 = cpl_image_get_data_float_complex(imfc3);

    for (i = 0; i < IMAGESZ * IMAGESZ; i++) {
        cpl_test_abs( dataf3[i], 3 * i + 3 * i * cpl_i, 0.0 );
    }

    cpl_image_delete(imfc1);
    cpl_image_delete(imfc2);
    cpl_image_delete(imfc3);

    /* test complex multiplication with several alignments and sizes */
    cpl_image_complex_mult(0, 0, 64);
    cpl_image_complex_mult(1, 1, 64);
    cpl_image_complex_mult(1, 0, 64);
    cpl_image_complex_mult(0, 0, 63);
    cpl_image_complex_mult(1, 1, 63);
    cpl_image_complex_mult(1, 0, 63);

    /* test complex multiplication with several alignments and sizes */
    cpl_image_complex_mult_d(0, 0, 64);
    cpl_image_complex_mult_d(1, 1, 64);
    cpl_image_complex_mult_d(1, 0, 64);
    cpl_image_complex_mult_d(0, 0, 63);
    cpl_image_complex_mult_d(1, 1, 63);
    cpl_image_complex_mult_d(1, 0, 63);

    /* End of tests */
    return cpl_test_end(0);
}

/**@}*/


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Test cpl_image_complex multiplication
  @param    o1  image 1 offset
  @param    o2  image 2 offset
  @param    n   npixels
  @return   void

 */
/*----------------------------------------------------------------------------*/

static void cpl_image_complex_mult(size_t o1, size_t o2, size_t n)
{
    /* test multiplication, compares vector version with scalar version */
    const size_t nl = n + 2;
    const float complex cpl_i = csqrtf(-1.0);

    float complex * dataf1_ = cpl_malloc(nl * nl * sizeof(float complex));
    float complex * dataf2_ = cpl_malloc(nl * nl * sizeof(float complex));
    float complex * dataf3;
    float complex * dataf4 = cpl_malloc(n * n * sizeof(float complex));
    float complex * dataf1 = dataf1_;
    float complex * dataf2 = dataf2_;
    size_t do1 = ((intptr_t)dataf1) % 16;
    size_t do2 = ((intptr_t)dataf2) % 16;
    cpl_image * imfc1;
    cpl_image * imfc2;

    cpl_msg_info(cpl_func, "Float complex vector multiply, "
                 "size: %zu, offsets: %zu %zu", n * n, o1, o2);

    do1 = (do1 ? (16u - do1) / sizeof(float complex) : 0u) + o1;
    do2 = (do2 ? (16u - do2) / sizeof(float complex) : 0u) + o2;

    dataf1 = dataf1 + do1;
    dataf2 = dataf2 + do2;

    for (size_t i = 0; i < n * n; i++) {
        dataf1[i] = i + 12.2e-1 * i * cpl_i;
        dataf2[i] = 3.23 * i + 2. * i * cpl_i;
    }

    /* add multiplication overflows, ignored in -ffast-math mode */
    dataf1[0] = 1e30;
    dataf2[0] = 1e30;
    dataf1[1] = -1e30;
    dataf2[1] = -1e30;
    /* only one of the vector bad */
    dataf2[2] = NAN + 1e30 * cpl_i;
    dataf2[3] = -34.231e-5 + 4.111e+9 * cpl_i;
    dataf2[4] = NAN + INFINITY * cpl_i;
    dataf1[5] = 35.126e14 - INFINITY * cpl_i;
    dataf2[5] = 35.126e14 - INFINITY * cpl_i;
    /* subnormals */
    dataf1[6] = 0.0000032e-37f - 0.0000001e-36f * cpl_i;
    dataf2[6] = 0.16f - INFINITY * cpl_i;
    dataf1[7] = 0.0000032e-37f - 0.0000001e-36f * cpl_i;
    dataf2[7] = 1.16f - 0.0000001e-36f * cpl_i;

    for (size_t i = 0; i < n * n; i++)
        dataf4[i] = dataf1[i] * dataf2[i];

    imfc1 = cpl_image_wrap(n, n, CPL_TYPE_FLOAT_COMPLEX, dataf1);
    imfc2 = cpl_image_wrap(n, n, CPL_TYPE_FLOAT_COMPLEX, dataf2);

    dataf3 = cpl_image_get_data_float_complex(imfc1);
    /* multiply several times to get get multiplications
     * involving special float values */
    for (int j = 0; j < 3; j++) {
        cpl_msg_info(cpl_func, "iteration: %d", j);
        cpl_image_multiply(imfc1, imfc2);
        for (size_t i = 0; i < n * n; i++) {
            float r1 = creal(dataf3[i]);
            float r2 = creal(dataf4[i]);
            float i1 = cimag(dataf3[i]);
            float i2 = cimag(dataf4[i]);

            if (isnan(r1) || isnan(r2))
                cpl_test_eq(isnan(r1), isnan(r2));
            else if (isnan(i1) || isnan(i2))
                cpl_test_eq(isnan(i1), isnan(i2));
            else if (!isfinite(r1) || !isfinite(r2))
                cpl_test_eq(isinf(r1), isinf(r2));
            else if (!isfinite(i1) || !isfinite(i2))
                cpl_test_eq(isinf(i1), isinf(i2));
            else {
                /* this test will fail if SSE is enabled but mfpmath=387, as
                 * the comparison numbers are then generated with different
                 * precision, the relative difference stays below ~ 1e-6,
                 * also set mfpmath=sse for consistent results */
                cpl_test_abs(i1, i2, FLT_EPSILON);
                cpl_test_abs(r1, r2, FLT_EPSILON);
            }

            dataf4[i] *= dataf2[i];
        }
    }

    cpl_image_unwrap(imfc1);
    cpl_image_unwrap(imfc2);
    cpl_free(dataf1_);
    cpl_free(dataf2_);
    cpl_free(dataf4);
}


static void cpl_image_complex_mult_d(size_t o1, size_t o2, size_t n)
{
    /* test multiplication, compares vector version with scalar version */
    const size_t nl = n + 2;
    const double complex cpl_i = csqrt(-1.0);

    double complex * dataf1_ = cpl_malloc(nl * nl * sizeof(double complex));
    double complex * dataf2_ = cpl_malloc(nl * nl * sizeof(double complex));
    double complex * dataf3;
    double complex * dataf4 = cpl_malloc(n * n * sizeof(double complex));
    double complex * dataf1 = dataf1_;
    double complex * dataf2 = dataf2_;
    size_t do1 = ((intptr_t)dataf1) % 16;
    size_t do2 = ((intptr_t)dataf2) % 16;
    cpl_image * imfc1;
    cpl_image * imfc2;

    cpl_msg_info(cpl_func, "Double complex vector multiply, "
                 "size: %zu, offsets: %zu %zu", n * n, o1, o2);

    /* i386 aligns complex double 8 bytes */
    do1 = (do1 ? (16u - do1) / sizeof(double) : 0u) + o1;
    do2 = (do2 ? (16u - do2) / sizeof(double) : 0u) + o2;

    dataf1 = (double complex *)((double *)dataf1 + do1);
    dataf2 = (double complex *)((double *)dataf2 + do2);

    for (size_t i = 0; i < n * n; i++) {
        dataf1[i] = i + 12.2e-1 * i * cpl_i;
        dataf2[i] = 3.23 * i + 2. * i * cpl_i;
    }

    /* add multiplication overflows, ignored in -ffast-math mode */
    dataf1[0] = 1e30;
    dataf2[0] = 1e30;
    dataf1[1] = -1e30;
    dataf2[1] = -1e30;
    /* only one of the vector bad */
    dataf2[2] = NAN + 1e30 * cpl_i;
    dataf2[3] = -34.231e-5 + 4.111e+9 * cpl_i;
    dataf2[4] = NAN + INFINITY * cpl_i;
    dataf1[5] = 35.126e14 - INFINITY * cpl_i;
    dataf2[5] = 35.126e14 - INFINITY * cpl_i;
    /* subnormals */
    dataf1[6] = 0.00000000032e-307 - 0.0000000001e-306 * cpl_i;
    dataf2[6] = 0.16 - INFINITY * cpl_i;
    dataf1[7] = 0.0000000032e-307 - 443.1 * cpl_i;
    dataf2[7] = 1.16 - 0.00000000001e-306 * cpl_i;

    for (size_t i = 0; i < n * n; i++)
        dataf4[i] = dataf1[i] * dataf2[i];

    imfc1 = cpl_image_wrap(n, n, CPL_TYPE_DOUBLE_COMPLEX, dataf1);
    imfc2 = cpl_image_wrap(n, n, CPL_TYPE_DOUBLE_COMPLEX, dataf2);

    dataf3 = cpl_image_get_data_double_complex(imfc1);
    /* multiply several times to get get multiplications
     * involving special double values */
    for (int j = 0; j < 3; j++) {
        cpl_msg_info(cpl_func, "iteration: %d", j);
        cpl_image_multiply(imfc1, imfc2);
        for (size_t i = 0; i < n * n; i++) {
            double r1 = creal(dataf3[i]);
            double r2 = creal(dataf4[i]);
            double i1 = cimag(dataf3[i]);
            double i2 = cimag(dataf4[i]);

            if (isnan(r1) || isnan(r2))
                cpl_test_eq(isnan(r1), isnan(r2));
            else if (isnan(i1) || isnan(i2))
                cpl_test_eq(isnan(i1), isnan(i2));
            else if (!isfinite(r1) || !isfinite(r2))
                cpl_test_eq(isinf(r1), isinf(r2));
            else if (!isfinite(i1) || !isfinite(i2))
                cpl_test_eq(isinf(i1), isinf(i2));
            else {
                /* this test will fail if SSE is enabled but mfpmath=387, as
                 * the comparison numbers are then generated with different
                 * precision, the relative difference stays below ~ 1e-15,
                 * also set mfpmath=sse for consistent results */
                cpl_test_abs(i1, i2, DBL_EPSILON);
                cpl_test_abs(r1, r2, DBL_EPSILON);
            }

            dataf4[i] *= dataf2[i];
        }
    }

    cpl_image_unwrap(imfc1);
    cpl_image_unwrap(imfc2);
    cpl_free(dataf1_);
    cpl_free(dataf2_);
    cpl_free(dataf4);
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Test cpl_image_copy
  @return   void
  
 */
/*----------------------------------------------------------------------------*/
static void cpl_image_copy_test(void)
{

    cpl_image * im1 = cpl_image_new(IMAGESZ, IMAGESZ, CPL_TYPE_DOUBLE);
    cpl_image * im2 = cpl_image_new(IMAGESZ, IMAGESZ, CPL_TYPE_INT);
    cpl_error_code error;

    error = cpl_image_copy(NULL, im1, 1, 1);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

    error = cpl_image_copy(im1, NULL, 1, 1);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

    error = cpl_image_copy(im1, im2, 1, 1);
    cpl_test_eq_error(error, CPL_ERROR_TYPE_MISMATCH);

    cpl_image_delete(im1);
    im1 = cpl_image_new(IMAGESZ, IMAGESZ, CPL_TYPE_INT);

    error = cpl_image_copy(im1, im2, 0, 1);
    cpl_test_eq_error(error, CPL_ERROR_ACCESS_OUT_OF_RANGE);

    error = cpl_image_copy(im1, im2, 1, 0);
    cpl_test_eq_error(error, CPL_ERROR_ACCESS_OUT_OF_RANGE);

    error = cpl_image_copy(im1, im2, IMAGESZ+1, 1);
    cpl_test_eq_error(error, CPL_ERROR_ACCESS_OUT_OF_RANGE);

    error = cpl_image_copy(im1, im2, 1, IMAGESZ+1);
    cpl_test_eq_error(error, CPL_ERROR_ACCESS_OUT_OF_RANGE);

    /* Test copy of whole image */
    cpl_test_zero(cpl_image_add_scalar(im1, 1.0));

    cpl_test_zero(cpl_image_copy(im1, im2, 1, 1));

    cpl_test_zero(cpl_image_get_max(im1));

    /* Test copy of whole number of columns (all but two) */
    cpl_test_zero(cpl_image_add_scalar(im1, 1.0));

    cpl_test_zero(cpl_image_copy(im1, im2, 3, 1));

    cpl_test_abs(cpl_image_get_flux(im1), 2.0 * IMAGESZ, 0.0);
    cpl_test_abs(cpl_image_get_flux_window(im1, 1, 1, 1, IMAGESZ), IMAGESZ, 0.0);
    cpl_test_abs(cpl_image_get_flux_window(im1, 2, 1, 2, IMAGESZ), IMAGESZ, 0.0);
    cpl_test_abs(cpl_image_get_flux_window(im1, 3, 1, IMAGESZ, IMAGESZ), 0.0, 0.0);

    /* Test copy of whole number of rows (all but two) */
    cpl_test_zero(cpl_image_copy(im1, im2, 1, 1));

    cpl_test_zero(cpl_image_add_scalar(im1, 1.0));

    cpl_test_zero(cpl_image_copy(im1, im2, 1, 3));

    cpl_test_abs(cpl_image_get_flux(im1), 2.0 * IMAGESZ, 0.0);
    cpl_test_abs(cpl_image_get_flux_window(im1, 1, 1, IMAGESZ, 1), IMAGESZ, 0.0);
    cpl_test_abs(cpl_image_get_flux_window(im1, 1, 2, IMAGESZ, 2), IMAGESZ, 0.0);
    cpl_test_abs(cpl_image_get_flux_window(im1, 1, 3, IMAGESZ, IMAGESZ), 0.0, 0.0);

    cpl_image_delete(im1);
    cpl_image_delete(im2);

    return;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Test cpl_image_turn() and cpl_image_flip()
  @return   void
  
 */
/*----------------------------------------------------------------------------*/
static void cpl_image_turn_flip_test(void)
{

    const cpl_type ttype[] = {CPL_TYPE_INT, CPL_TYPE_FLOAT, CPL_TYPE_DOUBLE};
    size_t itype;
    const int npix[] = {1, 2, 3, 5, 10, 11};

    const int buf9[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};

    /* Rotate this 90 degrees clockwise to get buf9 */
    const int rot9[] = {7, 4, 1, 8, 5, 2, 9, 6, 3};

    /* This image is not modified while wrapping around this buffer */
    cpl_image * ref = cpl_image_new(3, 3, CPL_TYPE_INT);
    cpl_image * raw = cpl_image_new(3, 3, CPL_TYPE_INT);
    cpl_error_code error;

    cpl_test_eq_ptr(memcpy(cpl_image_get_data_int(ref), buf9, 9*sizeof(int)),
                    cpl_image_get_data_int_const(ref));

    /* Test 1: Verify direction of 90 degree rotation */
    cpl_test_eq_ptr(memcpy(cpl_image_get_data_int(raw), rot9, 9*sizeof(int)),
                    cpl_image_get_data_int_const(raw));

    error = cpl_image_turn(raw, 1);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_image_abs(raw, ref, 0.0);

    cpl_image_delete(raw);

    /* Test 1a: Verify flip around x=y and x=-y */

    cpl_image_flip_turn_test(ref);

    cpl_image_delete(ref);

    /* Test 2: Check error handling */
    error = cpl_image_turn(NULL, 1);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

    /* Test 3: Verify consistency of rotations across types */
    for (itype = 0; itype < sizeof(ttype)/sizeof(ttype[0]); itype++) {
        const cpl_type imtype = ttype[itype];
        size_t ipix;
        for (ipix = 0; ipix < sizeof(npix)/sizeof(npix[0]); ipix++) {
            const int nx = npix[ipix];
            size_t jpix;
            for (jpix = 0; jpix < sizeof(npix)/sizeof(npix[0]); jpix++) {
                const int ny = npix[jpix];
                cpl_mask * bpm;

                raw = cpl_image_new(nx, ny, imtype);

                error = cpl_image_fill_noise_uniform(raw, (double)-ny,
                                                     (double)nx);
                cpl_test_eq_error(error, CPL_ERROR_NONE);

                /* About 1/4 of the pixels are flagged */
                bpm = cpl_mask_threshold_image_create(raw, -0.25*ny, 0.25*nx);
                if (cpl_mask_count(bpm) < nx * ny) {
                    /* Will not test with no good pixels */
                    error = cpl_image_reject_from_mask(raw, bpm);
                    cpl_test_eq_error(error, CPL_ERROR_NONE);
                }
                cpl_mask_delete(bpm);

                ref = cpl_image_duplicate(raw);

                /* Turn the image all the way around */
                error = cpl_image_turn(raw, 1);
                cpl_test_eq_error(error, CPL_ERROR_NONE);
                error = cpl_image_turn(raw, 2);
                cpl_test_eq_error(error, CPL_ERROR_NONE);
                error = cpl_image_turn(raw, 3);
                cpl_test_eq_error(error, CPL_ERROR_NONE);
                error = cpl_image_turn(raw, 4);
                cpl_test_eq_error(error, CPL_ERROR_NONE);
                error = cpl_image_turn(raw,-1);
                cpl_test_eq_error(error, CPL_ERROR_NONE);
                error = cpl_image_turn(raw,-2);
                cpl_test_eq_error(error, CPL_ERROR_NONE);
                error = cpl_image_turn(raw,-3);
                cpl_test_eq_error(error, CPL_ERROR_NONE);
                error = cpl_image_turn(raw, 0);
                cpl_test_eq_error(error, CPL_ERROR_NONE);

                cpl_test_image_abs(raw, ref, 0.0);

                /* Flip the image all the way around */
                error = cpl_image_flip(raw, 0);
                cpl_test_eq_error(error, CPL_ERROR_NONE);
                error = cpl_image_flip(raw, 1);
                cpl_test_eq_error(error, CPL_ERROR_NONE);
                error = cpl_image_flip(raw, 2);
                cpl_test_eq_error(error, CPL_ERROR_NONE);
                error = cpl_image_flip(raw, 3);
                cpl_test_eq_error(error, CPL_ERROR_NONE);
                error = cpl_image_flip(raw, 0);
                cpl_test_eq_error(error, CPL_ERROR_NONE);
                error = cpl_image_flip(raw, 1);
                cpl_test_eq_error(error, CPL_ERROR_NONE);
                error = cpl_image_flip(raw, 2);
                cpl_test_eq_error(error, CPL_ERROR_NONE);
                error = cpl_image_flip(raw, 3);
                cpl_test_eq_error(error, CPL_ERROR_NONE);

                cpl_test_image_abs(raw, ref, 0.0);

                cpl_image_delete(ref);
                cpl_image_delete(raw);
            }
        }
    }
    return;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Test cpl_image_collapse_median_create()
  @return   void
  
 */
/*----------------------------------------------------------------------------*/
static void cpl_image_collapse_median_test(void)
{

    const int mx = IMAGESZ | 1;
    const int my = IMAGESZ/2 | 1;
    const cpl_type types[] = {CPL_TYPE_DOUBLE_COMPLEX,
                              CPL_TYPE_FLOAT_COMPLEX};
    const cpl_type imtypes[] = {CPL_TYPE_DOUBLE, CPL_TYPE_FLOAT,
                                CPL_TYPE_INT};
    cpl_image    * null;
    int            ityp;


    null = cpl_image_collapse_median_create(NULL, 0, 0, 0);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(null);

    /* Iterate through all non-supported pixel types */
    for (ityp = 0; ityp < (int)(sizeof(types)/sizeof(types[0])); ityp++) {
        const cpl_type imtype = types[ityp];
        cpl_image * img = cpl_image_new(mx, my, imtype);

        cpl_test_nonnull(img);

        null = cpl_image_collapse_median_create(img, 0, 0, 0);
        cpl_test_error(CPL_ERROR_INVALID_TYPE);
        cpl_test_null(null);

        cpl_image_delete(img);
    }


    /* Iterate through all supported pixel types */
    for (ityp = 0; ityp < (int)(sizeof(imtypes)/sizeof(imtypes[0])); ityp++) {
        const cpl_type imtype = imtypes[ityp];
        cpl_image * img = cpl_image_new(mx, my, imtype);
        int idrop;

        cpl_test_nonnull(img);

        cpl_image_fill_noise_uniform(img, 0.0, IMAGESZ);

        null = cpl_image_collapse_median_create(img, 0, my, 0);
        cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
        cpl_test_null(null);

        null = cpl_image_collapse_median_create(img, 0, 0, my);
        cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
        cpl_test_null(null);

        null = cpl_image_collapse_median_create(img, 1, 0, mx);
        cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
        cpl_test_null(null);

        null = cpl_image_collapse_median_create(img, 1, mx, 0);
        cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
        cpl_test_null(null);

        null = cpl_image_collapse_median_create(img, 2, 0, 0);
        cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
        cpl_test_null(null);

        /* Iterate through various drop combinations for dir == 0 */
        for (idrop = 0; idrop * 2 < 5; idrop ++) {

            cpl_image * filt = cpl_image_new(mx, 1 + 2 * idrop, imtype);
            cpl_mask  * mask = cpl_mask_new(1, my - 2 * idrop);
            int jdrop;
            cpl_error_code error;

            cpl_mask_not(mask);

            error = cpl_image_filter_mask(filt, img, mask,
                                          CPL_FILTER_MEDIAN, CPL_BORDER_CROP);
            cpl_test_eq_error(error, CPL_ERROR_NONE);

            for (jdrop = 0; jdrop <= 2 * idrop; jdrop++) {
                cpl_image * med
                    = cpl_image_collapse_median_create(img, 0, jdrop,
                                                       2 * idrop - jdrop);
                cpl_image * img1d = cpl_image_extract(filt, 1, 1 + jdrop, mx,
                                                      1 + jdrop);
                int k, is_bad, is_bad2;

                cpl_test_nonnull(med);
                cpl_test_nonnull(img1d);

                cpl_test_eq(cpl_image_get_type(med), imtype);
                cpl_test_eq(cpl_image_get_size_x(med), mx);
                cpl_test_eq(cpl_image_get_size_y(med), 1);

                cpl_test_image_abs(med, img1d, 0.0);

                cpl_image_delete(med);

                /* Fail on all bad pixels */
                cpl_mask_not(cpl_image_get_bpm(img));
                cpl_test_eq(cpl_image_count_rejected(img), mx * my);

                null = cpl_image_collapse_median_create(img, 0, jdrop,
                                                        2 * idrop - jdrop);
                cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);
                cpl_test_null(null);

                /* Accept all pixels in one column */
                for (k = 1; k <= my; k++) {
                    cpl_image_accept(img, mx/2, k);
                }

                cpl_test_eq(cpl_image_count_rejected(img), mx * my - my);

                med = cpl_image_collapse_median_create(img, 0, jdrop,
                                                       2 * idrop - jdrop);
                cpl_test_nonnull(med);

                cpl_test_eq(cpl_image_count_rejected(med), mx - 1);
                cpl_test_abs(cpl_image_get(med, mx/2, 1, &is_bad),
                             cpl_image_get(img1d, mx/2, 1, &is_bad2), 0.0);
                cpl_test_zero(is_bad);
                cpl_test_zero(is_bad2);

                cpl_test_abs(cpl_image_get(med, 1, 1, &is_bad), 0.0, 0.0);
                cpl_test(is_bad);

                cpl_image_delete(med);
                cpl_image_delete(img1d);
                cpl_image_accept_all(img);

                /* Accept all pixels in one row */
                cpl_mask_not(cpl_image_get_bpm(img));
                for (k = 1; k <= mx; k++) {
                    cpl_image_accept(img, k, my/2);
                }

                med = cpl_image_collapse_median_create(img, 0, jdrop,
                                                       2 * idrop - jdrop);
                cpl_test_nonnull(med);

                cpl_test_zero(cpl_image_count_rejected(med));

                img1d = cpl_image_extract(img, 1, my/2, mx, my/2);

                cpl_test_image_abs(med, img1d, 0.0);

                cpl_image_delete(img1d);
                cpl_image_delete(med);
                cpl_image_accept_all(img);
            }
            cpl_image_delete(filt);
            cpl_mask_delete(mask);
        }

        /* Iterate through various drop combinations for dir == 1 */
        for (idrop = 0; idrop * 2 < 5; idrop ++) {

            cpl_image * filt = cpl_image_new(1 + 2 * idrop, my, imtype);
            cpl_mask  * mask = cpl_mask_new(mx - 2 * idrop, 1);
            int jdrop;
            cpl_error_code error;

            cpl_mask_not(mask);

            error = cpl_image_filter_mask(filt, img, mask,
                                          CPL_FILTER_MEDIAN, CPL_BORDER_CROP);
            cpl_test_eq_error(error, CPL_ERROR_NONE);

            for (jdrop = 0; jdrop <= 2 * idrop; jdrop++) {
                cpl_image * med
                    = cpl_image_collapse_median_create(img, 1, jdrop,
                                                       2 * idrop - jdrop);
                cpl_image * img1d = cpl_image_extract(filt, 1 + jdrop, 1,
                                                      1 + jdrop, my);
                int k, is_bad, is_bad2;

                cpl_test_nonnull(med);
                cpl_test_nonnull(img1d);

                cpl_test_eq(cpl_image_get_type(med), imtype);
                cpl_test_eq(cpl_image_get_size_x(med), 1);
                cpl_test_eq(cpl_image_get_size_y(med), my);

                cpl_test_image_abs(med, img1d, 0.0);

                cpl_image_delete(med);

                /* Fail on all bad pixels */
                cpl_mask_not(cpl_image_get_bpm(img));
                cpl_test_eq(cpl_image_count_rejected(img), mx * my);

                null = cpl_image_collapse_median_create(img, 1, jdrop,
                                                        2 * idrop - jdrop);
                cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);
                cpl_test_null(null);

                /* Accept all pixels in one row */
                for (k = 1; k <= mx; k++) {
                    cpl_image_accept(img, k, my/2);
                }

                cpl_test_eq(cpl_image_count_rejected(img), mx * my - mx);

                med = cpl_image_collapse_median_create(img, 1, jdrop,
                                                       2 * idrop - jdrop);
                cpl_test_nonnull(med);

                cpl_test_eq(cpl_image_count_rejected(med), my - 1);
                cpl_test_abs(cpl_image_get(med, 1, my/2, &is_bad),
                             cpl_image_get(img1d, 1, my/2, &is_bad2), 0.0);

                cpl_test_zero(is_bad);
                cpl_test_zero(is_bad2);

                cpl_test_abs(cpl_image_get(med, 1, 1, &is_bad), 0.0, 0.0);
                cpl_test(is_bad);

                cpl_image_delete(med);

                cpl_image_accept_all(img);
                cpl_image_delete(img1d);


                /* Accept all pixels in one column */
                cpl_mask_not(cpl_image_get_bpm(img));
                for (k = 1; k <= my; k++) {
                    cpl_image_accept(img, mx/2, k);
                }

                med = cpl_image_collapse_median_create(img, 1, jdrop,
                                                       2 * idrop - jdrop);
                cpl_test_nonnull(med);

                cpl_test_zero(cpl_image_count_rejected(med));

                img1d = cpl_image_extract(img, mx/2, 1, mx/2, my);

                cpl_test_image_abs(med, img1d, 0.0);

                cpl_image_delete(img1d);
                cpl_image_delete(med);
                cpl_image_accept_all(img);

            }
            cpl_image_delete(filt);
            cpl_mask_delete(mask);
        }

        cpl_image_delete(img);
    }

    return;

}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Test cpl_image_flip(), cpl_image_turn()
  @param  self    The image to test
  @return void
  @see cpl_mask_flip_turn_test()

   A: 
       Rotate 90 counterclock-wise, then
       flip around the vertical axis, then
       flip around x=y, and
       it should be back to its original.

   B: 

       Rotate 90 counterclock-wise, then
       flip around the horizontal axis, then
       flip around x=-y, and
       it should be back to its original.


 */
/*----------------------------------------------------------------------------*/
static void cpl_image_flip_turn_test(const cpl_image * self)
{

    cpl_error_code error;
    cpl_image * tmp = cpl_image_duplicate(self);

    error = cpl_image_turn(tmp, -1);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_image_flip(tmp, 2);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_image_flip(tmp, 1);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_image_abs(self, tmp, 0.0);

    error = cpl_image_turn(tmp, -1);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_image_flip(tmp, 0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_image_flip(tmp, 3);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_image_abs(self, tmp, 0.0);

    cpl_image_delete(tmp);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Test the CPL function
  @return void
  @see cpl_image_get_fwhm()

 */
/*----------------------------------------------------------------------------*/
static void cpl_image_get_fwhm_test(void)
{

    /* Test case for DFS12346 */
    const double x[] = { 
        -13.9055,
        -2.24811,
        -4.60207,
        12.0745,
        -9.29113,
        19.4861,
        17.6999,
        32.2417,
        68.0725,
        120.824,
        172.361,
        242.671,
        302.048,
        355.937,
        348.458,
        309.819,
        240.489,
        179.107,
        117.223,
        66.3061,
        32.7849,
        22.9367,
    };

    const cpl_size sz = (cpl_size)(sizeof(x)/sizeof(x[0]));   

    /* x will _not_ be modified */
    cpl_image * im1d = cpl_image_wrap_double(sz, 1, (double*)x);

    cpl_size xmaxpos, ymaxpos;

    double xfwhm, yfwhm, xref;
    cpl_error_code code;

    cpl_msg_info(cpl_func, "Testing cpl_image_get_fwhm() with %u elements",
                 (unsigned)sz);

    code = cpl_image_get_maxpos(im1d, &xmaxpos, &ymaxpos);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_eq(xmaxpos, 14);
    cpl_test_eq(ymaxpos, 1);

    /* Test some error input */
    code = cpl_image_get_fwhm(NULL, xmaxpos, ymaxpos, &xfwhm, &yfwhm);
    cpl_test_eq_error(code, CPL_ERROR_NULL_INPUT);

    code = cpl_image_get_fwhm(im1d, xmaxpos, ymaxpos, NULL, &yfwhm);
    cpl_test_eq_error(code, CPL_ERROR_NULL_INPUT);

    code = cpl_image_get_fwhm(im1d, xmaxpos, ymaxpos, &xfwhm, NULL);
    cpl_test_eq_error(code, CPL_ERROR_NULL_INPUT);

    code = cpl_image_get_fwhm(im1d, 0, ymaxpos, &xfwhm, &yfwhm);
    cpl_test_eq_error(code, CPL_ERROR_ACCESS_OUT_OF_RANGE);

    code = cpl_image_get_fwhm(im1d, xmaxpos, 0, &xfwhm, &yfwhm);
    cpl_test_eq_error(code, CPL_ERROR_ACCESS_OUT_OF_RANGE);

    code = cpl_image_get_fwhm(im1d, 0, ymaxpos, &xfwhm, &yfwhm);
    cpl_test_eq_error(code, CPL_ERROR_ACCESS_OUT_OF_RANGE);

    code = cpl_image_get_fwhm(im1d, sz+1, ymaxpos, &xfwhm, &yfwhm);
    cpl_test_eq_error(code, CPL_ERROR_ACCESS_OUT_OF_RANGE);

    code = cpl_image_get_fwhm(im1d, xmaxpos, 2, &xfwhm, &yfwhm);
    cpl_test_eq_error(code, CPL_ERROR_ACCESS_OUT_OF_RANGE);

    code = cpl_image_get_fwhm(im1d, ymaxpos, ymaxpos, &xfwhm, &yfwhm);
    cpl_test_eq_error(code, CPL_ERROR_DATA_NOT_FOUND);

    /* Test with valid input */
    code = cpl_image_get_fwhm(im1d, xmaxpos, ymaxpos, &xfwhm, &yfwhm);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_abs(xfwhm, 6.7434, 1e-4);
    cpl_test_abs(yfwhm, -1.0, 0.0);

    /* Rotate the image 90 degrees (this does not modfy the pixel buffer) */
    code = cpl_image_turn(im1d, -1);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    /* Call again, with x/y variables swapped */
    xref = xfwhm;
    code = cpl_image_get_fwhm(im1d, ymaxpos, xmaxpos, &yfwhm, &xfwhm);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    /* Expect exactly same result */
    cpl_test_abs(xfwhm, xref, 0.0);
    cpl_test_abs(yfwhm, -1.0, 0.0);

    cpl_test_eq_ptr((const void*)x, cpl_image_unwrap(im1d));

}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Test the CPL function
  @return void
  @see cpl_image_divide()

 */
/*----------------------------------------------------------------------------*/
static void cpl_image_divide_test_one(void) {

    cpl_image * img1 = cpl_image_new(1, 2, CPL_TYPE_DOUBLE);
    cpl_image * img2 = cpl_image_new(1, 2, CPL_TYPE_DOUBLE);
    cpl_error_code code;

    code = cpl_image_add_scalar(img1, 5.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_image_add_scalar(img2, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);


    cpl_test_abs(cpl_image_get_flux(img1), 10.0, DBL_EPSILON);

    cpl_test_zero(cpl_image_count_rejected(img1));
    cpl_test_zero(cpl_image_count_rejected(img2));
    code = cpl_image_reject(img1, 1, 1);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_eq(cpl_image_count_rejected(img1), 1);

    cpl_test_abs(cpl_image_get_flux(img1), 5.0, DBL_EPSILON);

    code = cpl_image_divide(img1, img2);
    cpl_test_eq_error(code, CPL_ERROR_NONE);


    cpl_test_abs(cpl_image_get_flux(img1), 5.0, DBL_EPSILON);

    cpl_test_eq(cpl_image_count_rejected(img1), 1);

    cpl_image_delete(img1);
    cpl_image_delete(img2);

}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Test the CPL function
  @return void
  @see cpl_image_power()

 */
/*----------------------------------------------------------------------------*/
static void cpl_image_hypot_test(void) {

    const cpl_type htype[2] = {CPL_TYPE_FLOAT, CPL_TYPE_DOUBLE};

    for (int i1type = 0; i1type < 2; i1type++) {
        for (int i2type = 0; i2type < 2; i2type++) {
            for (int i3type = 0; i3type < 2; i3type++) {
                const double tol =
                    htype[i1type] == CPL_TYPE_FLOAT ||
                    htype[i2type] == CPL_TYPE_FLOAT ||
                    htype[i3type] == CPL_TYPE_FLOAT
                    ? 40.0 * FLT_EPSILON
                    : 40.0 * DBL_EPSILON;

                cpl_image * img1 = cpl_image_new(IMAGESZ, IMAGESZ, htype[i1type]);
                cpl_image * img2 = cpl_image_new(IMAGESZ, IMAGESZ, htype[i2type]);
                cpl_image * img3 = cpl_image_new(IMAGESZ, IMAGESZ, htype[i3type]);
                cpl_image * img1copy;
                cpl_image * img2copy;
                cpl_error_code code;

                code = cpl_image_fill_noise_uniform(img1, 1.0, MAGIC_PIXEL);
                cpl_test_eq_error(code, CPL_ERROR_NONE);
                img1copy = cpl_image_duplicate(img1);

                code = cpl_image_fill_noise_uniform(img2, 1.0, MAGIC_PIXEL);
                cpl_test_eq_error(code, CPL_ERROR_NONE);
                img2copy = cpl_image_duplicate(img2);

                code = cpl_image_hypot(img3, img1, img2);
                cpl_test_eq_error(code, CPL_ERROR_NONE);

                code = cpl_image_power(img1, 2.0);
                cpl_test_eq_error(code, CPL_ERROR_NONE);

                code = cpl_image_power(img2, 2.0);
                cpl_test_eq_error(code, CPL_ERROR_NONE);

                code = cpl_image_add(img1, img2);
                cpl_test_eq_error(code, CPL_ERROR_NONE);

                code = cpl_image_power(img1, 0.5);
                cpl_test_eq_error(code, CPL_ERROR_NONE);

                cpl_test_image_abs(img1, img3, tol);

                code = cpl_image_hypot(img1copy, NULL, img2copy);
                cpl_test_eq_error(code, CPL_ERROR_NONE);

                /* The in-place is not as accurate (w. all float data) ... */
                cpl_test_image_abs(img1copy, img3, 1.5 * tol);

                cpl_image_delete(img1);
                cpl_image_delete(img2);
                cpl_image_delete(img3);

                /* Verify handling of incompatible size */
                img3 = cpl_image_new(IMAGESZ+1, IMAGESZ, htype[i1type]);

                code = cpl_image_hypot(img3, img1copy, img2copy);
                cpl_test_eq_error(code, CPL_ERROR_INCOMPATIBLE_INPUT);

                code = cpl_image_hypot(img1copy, img3, img2copy);
                cpl_test_eq_error(code, CPL_ERROR_INCOMPATIBLE_INPUT);


                code = cpl_image_hypot(img1copy, img2copy, img3);
                cpl_test_eq_error(code, CPL_ERROR_INCOMPATIBLE_INPUT);

                code = cpl_image_hypot(img3, NULL, img2copy);
                cpl_test_eq_error(code, CPL_ERROR_INCOMPATIBLE_INPUT);

                code = cpl_image_hypot(img2copy, NULL, img3);
                cpl_test_eq_error(code, CPL_ERROR_INCOMPATIBLE_INPUT);

                cpl_image_delete(img3);

                /* Verify handling of unsupported type */
                img3 = cpl_image_new(IMAGESZ+1, IMAGESZ, htype[i1type]
                                     | CPL_TYPE_COMPLEX);

                code = cpl_image_hypot(img3, img1copy, img2copy);
                cpl_test_eq_error(code, CPL_ERROR_INVALID_TYPE);

                code = cpl_image_hypot(img1copy, img3, img2copy);
                cpl_test_eq_error(code, CPL_ERROR_INVALID_TYPE);


                code = cpl_image_hypot(img1copy, img2copy, img3);
                cpl_test_eq_error(code, CPL_ERROR_INVALID_TYPE);

                code = cpl_image_hypot(img3, NULL, img2copy);
                cpl_test_eq_error(code, CPL_ERROR_INVALID_TYPE);

                code = cpl_image_hypot(img2copy, NULL, img3);
                cpl_test_eq_error(code, CPL_ERROR_INVALID_TYPE);

                cpl_image_delete(img3);

                /* Verify handling of NULL pointers */
                code = cpl_image_hypot(img1copy, img2copy, NULL);
                cpl_test_eq_error(code, CPL_ERROR_NULL_INPUT);

                code = cpl_image_hypot(NULL, img1copy, img2copy);
                cpl_test_eq_error(code, CPL_ERROR_NULL_INPUT);

                cpl_image_delete(img1copy);
                cpl_image_delete(img2copy);
            }
        }
    }
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Test the CPL function
  @return void
  @see cpl_image_power()

 */
/*----------------------------------------------------------------------------*/
static void cpl_image_power_test(void) {

    cpl_image * img1 = cpl_image_new(IMAGESZ, IMAGESZ, CPL_TYPE_DOUBLE);
    cpl_image * img2;
    cpl_error_code code;

    /* Avoid division by zero for now */
    code = cpl_image_fill_noise_uniform(img1, 1.0, MAGIC_PIXEL);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    img2 = cpl_image_power_create(img1, 1.0);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(img2);

    code = cpl_image_power(img1, -2.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_image_power(img1, -0.5);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_image_abs(img1, img2, 40.0 * DBL_EPSILON);

    cpl_image_delete(img2);
    img2 = cpl_image_power_create(img1, 1.0);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(img2);

    code = cpl_image_power(img1, -3.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_image_power(img1, -1.0/3.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_image_abs(img1, img2, 100.0 * DBL_EPSILON);

    cpl_image_delete(img1);
    cpl_image_delete(img2);
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Test the CPL function
  @return void
  @see cpl_image_and(), cpl_image_or(), cpl_image_xor()

 */
/*----------------------------------------------------------------------------*/
static void cpl_image_bitwise_test(cpl_size nx, cpl_size ny)
{

    cpl_error_code (*fimage[3])(cpl_image *, const cpl_image *,
                               const cpl_image *)
        = {cpl_image_and, cpl_image_or, cpl_image_xor};
    cpl_error_code (*fscalar[3])(cpl_image *, const cpl_image *,
                                cpl_bitmask)
        = {cpl_image_and_scalar, cpl_image_or_scalar, cpl_image_xor_scalar};
    const size_t nfunc = sizeof(fimage)/sizeof(*fimage);
    
    cpl_image * imf   = cpl_image_new(nx,     ny,     CPL_TYPE_FLOAT);
    cpl_image * im01  = cpl_image_new(nx,     ny + 1, CPL_TYPE_INT);
    cpl_image * im10  = cpl_image_new(nx + 1, ny,     CPL_TYPE_INT);
    cpl_image * im0   = cpl_image_new(nx,     ny,     CPL_TYPE_INT);
    cpl_image * im1   = cpl_image_new(nx,     ny,     CPL_TYPE_INT);
    cpl_image * im2   = cpl_image_new(nx,     ny,     CPL_TYPE_INT);
    cpl_mask *  mask0 = cpl_mask_new(nx,      ny);
    cpl_error_code code;

    cpl_test_eq( sizeof(fimage), sizeof(fscalar));

    /* Test the handling of the 3 possible error types for all 7 functions */
    for (size_t i = 0; i < nfunc; i++) {
        const cpl_image * im3 = im1;
        for (size_t i2 = 0; i2 < 2; i2++, im3 = NULL) {
            code = fimage[i](NULL, im3, im2);
            cpl_test_eq_error(code, CPL_ERROR_NULL_INPUT);
            code = fimage[i](im1, im3, NULL);
            cpl_test_eq_error(code, CPL_ERROR_NULL_INPUT);
            code = fscalar[i](NULL, im3, 0);
            cpl_test_eq_error(code, CPL_ERROR_NULL_INPUT);

            code = fimage[i](im01, im3, im2);
            cpl_test_eq_error(code, CPL_ERROR_INCOMPATIBLE_INPUT);
            code = fimage[i](im10, im3, im2);
            cpl_test_eq_error(code, CPL_ERROR_INCOMPATIBLE_INPUT);
            code = fimage[i](im1, im3, im01);
            cpl_test_eq_error(code, CPL_ERROR_INCOMPATIBLE_INPUT);
            code = fimage[i](im1, im3, im10);
            cpl_test_eq_error(code, CPL_ERROR_INCOMPATIBLE_INPUT);

            code = fimage[i](imf, im3, im2);
            cpl_test_eq_error(code, CPL_ERROR_INVALID_TYPE);
            code = fimage[i](im1, im3, imf);
            cpl_test_eq_error(code, CPL_ERROR_INVALID_TYPE);
            code = fscalar[i](imf, im3, 0);
            cpl_test_eq_error(code, CPL_ERROR_INVALID_TYPE);

            if (i == 0) {
                code = cpl_image_not(NULL, im3);
                cpl_test_eq_error(code, CPL_ERROR_NULL_INPUT);
                code = cpl_image_not(imf, im3);
                cpl_test_eq_error(code, CPL_ERROR_INVALID_TYPE);
            }
        }

        code = fimage[i](im1, im01, im2);
        cpl_test_eq_error(code, CPL_ERROR_INCOMPATIBLE_INPUT);
        code = fimage[i](im1, im10, im2);
        cpl_test_eq_error(code, CPL_ERROR_INCOMPATIBLE_INPUT);
        code = fscalar[i](im1, im01, 0);
        cpl_test_eq_error(code, CPL_ERROR_INCOMPATIBLE_INPUT);
        code = fscalar[i](im1, im10, 0);
        cpl_test_eq_error(code, CPL_ERROR_INCOMPATIBLE_INPUT);
        code = fscalar[i](im01, im1, 0);
        cpl_test_eq_error(code, CPL_ERROR_INCOMPATIBLE_INPUT);
        code = fscalar[i](im10, im1, 0);
        cpl_test_eq_error(code, CPL_ERROR_INCOMPATIBLE_INPUT);

        code = fimage[i](im1, imf, im2);
        cpl_test_eq_error(code, CPL_ERROR_INVALID_TYPE);
        code = fscalar[i](im1, imf, 0);
        cpl_test_eq_error(code, CPL_ERROR_INVALID_TYPE);
    }

    code = cpl_image_not(im1, im01);
    cpl_test_eq_error(code, CPL_ERROR_INCOMPATIBLE_INPUT);
    code = cpl_image_not(im1, im10);
    cpl_test_eq_error(code, CPL_ERROR_INCOMPATIBLE_INPUT);

    code = cpl_image_fill_noise_uniform(im1,
                                        -cpl_tools_ipow(2.0, 31),
                                        cpl_tools_ipow(2.0, 31) - 1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_image_not(im2, im1);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_image_or(im0, im1, im2);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_mask_threshold_image(mask0, im0, -1.5, -0.5, CPL_BINARY_0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_zero(cpl_mask_count(mask0));

    code = cpl_image_xor(im0, im1, im2);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_mask_threshold_image(mask0, im0, -1.5, -0.5, CPL_BINARY_0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_zero(cpl_mask_count(mask0));

    code = cpl_image_and(im0, im1, im2);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_mask_threshold_image(mask0, im0, -0.5, 0.5, CPL_BINARY_0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_zero(cpl_mask_count(mask0));

    code = cpl_image_and_scalar(im0, im1, 15);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_mask_threshold_image(mask0, im0, -0.5, 15.5, CPL_BINARY_0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_zero(cpl_mask_count(mask0));
    if (cpl_mask_count(mask0)) {
        int idummy;
        cpl_test_eq(cpl_image_get(im0, 1, 1, &idummy), 15);
    }

    code = cpl_image_and_scalar(im0, im1, 0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_mask_threshold_image(mask0, im0, -0.5, 0.5, CPL_BINARY_0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_zero(cpl_mask_count(mask0));

    code = cpl_image_and_scalar(im0, im1, 15);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_mask_threshold_image(mask0, im0, -0.5, 15.5, CPL_BINARY_0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_zero(cpl_mask_count(mask0));

    code = cpl_image_and_scalar(im0, im1, (cpl_bitmask)-1);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_image_abs(im0, im1, 0.0);

    code = cpl_image_or_scalar(im0, im1, 0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_image_abs(im0, im1, 0.0);

    code = cpl_image_xor_scalar(im0, im1, 0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_image_abs(im0, im1, 0.0);

    code = cpl_image_or_scalar(im0, im1, (cpl_bitmask)-1);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_mask_threshold_image(mask0, im0, -1.5, -0.5, CPL_BINARY_0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_zero(cpl_mask_count(mask0));

    code = cpl_image_xor_scalar(im0, im1, (cpl_bitmask)-1);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_image_not(im0, NULL);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_image_abs(im0, im1, 0.0);

    cpl_image_delete(imf);
    cpl_image_delete(im01);
    cpl_image_delete(im10);
    cpl_image_delete(im0);
    cpl_image_delete(im1);
    cpl_image_delete(im2);
    cpl_mask_delete(mask0);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Test the cpl_image_divide_create function
  @return   void
  This test checks the behaviour of the cpl_image_divide_create function for
  various cases of division by zeros and bad pixel masks.
 */
/*----------------------------------------------------------------------------*/
static void cpl_image_divide_create_test(void)
{
    cpl_image * a = cpl_image_new(3, 3, CPL_TYPE_FLOAT);
    cpl_image * b = cpl_image_new(3, 3, CPL_TYPE_FLOAT);
    cpl_image * c = NULL;
    const cpl_mask * m = NULL;
    float * data_a = cpl_image_get_data_float(a);
    float * data_b = cpl_image_get_data_float(b);
    int i;

    /* Make sure the reference images are initialised. Otherwise stop here. */
    cpl_test_nonnull(a);
    cpl_test_nonnull(b);
    cpl_test_nonnull(data_a);
    cpl_test_nonnull(data_b);
    if (a == NULL || b == NULL || data_a == NULL || data_b == NULL) return;

    /*                [ 6 -7  8]
     * Set image A to [-3  4 -5]
     *                [ 0 -1  2]
     */
    for (i = 0; i < 9; ++i) data_a[i] = (float)i * (i % 2 == 0 ? 1.0 : -1.0);

    /* Image B should be all zeros from the function call cpl_image_new. */
    for (i = 0; i < 9; ++i) cpl_test(data_b[i] == 0.0);

    /* Check that dividing by image B with all zeros produces a CPL divide by
     * zero error code. */
    c = cpl_image_divide_create(a, b);
    cpl_test_null(c);
    cpl_test_error(CPL_ERROR_DIVISION_BY_ZERO);

    /* Check that the following holds A/B = C, where
     *     [ 6 -7  8]       1   [6 0 0]           [ 1  0  0]
     * A = [-3  4 -5]  B = -- * [0 4 5]  C = 10 * [ 0  1 -1]
     *     [ 0 -1  2]      10   [0 1 0]           [ 0 -1  0]
     * The bad pixel masks should be the following (X indicates bad, '.' good):
     *         [. X X]
     * C_bpm = [X . .]
     *         [X . X]
     */
    data_b[6] = 0.6; data_b[7] = 0.0; data_b[8] = 0.0;
    data_b[3] = 0.0; data_b[4] = 0.4; data_b[5] = 0.5;
    data_b[0] = 0.0; data_b[1] = 0.1; data_b[2] = 0.0;
    c = cpl_image_divide_create(a, b);
    cpl_test_nonnull(c);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_abs(cpl_image_get_data_float_const(c)[0],   0.0, FLT_EPSILON);
    cpl_test_abs(cpl_image_get_data_float_const(c)[1], -10.0, FLT_EPSILON);
    cpl_test_abs(cpl_image_get_data_float_const(c)[2],   0.0, FLT_EPSILON);
    cpl_test_abs(cpl_image_get_data_float_const(c)[3],   0.0, FLT_EPSILON);
    cpl_test_abs(cpl_image_get_data_float_const(c)[4],  10.0, FLT_EPSILON);
    cpl_test_abs(cpl_image_get_data_float_const(c)[5], -10.0, FLT_EPSILON);
    cpl_test_abs(cpl_image_get_data_float_const(c)[6],  10.0, FLT_EPSILON);
    cpl_test_abs(cpl_image_get_data_float_const(c)[7],   0.0, FLT_EPSILON);
    cpl_test_abs(cpl_image_get_data_float_const(c)[8],   0.0, FLT_EPSILON);
    /* Check the bad pixels. */
    m = cpl_image_get_bpm_const(c);
    cpl_test_nonnull(m);
    cpl_test(cpl_mask_get_data_const(m)[0] == CPL_BINARY_1);
    cpl_test(cpl_mask_get_data_const(m)[1] == CPL_BINARY_0);
    cpl_test(cpl_mask_get_data_const(m)[2] == CPL_BINARY_1);
    cpl_test(cpl_mask_get_data_const(m)[3] == CPL_BINARY_1);
    cpl_test(cpl_mask_get_data_const(m)[4] == CPL_BINARY_0);
    cpl_test(cpl_mask_get_data_const(m)[5] == CPL_BINARY_0);
    cpl_test(cpl_mask_get_data_const(m)[6] == CPL_BINARY_0);
    cpl_test(cpl_mask_get_data_const(m)[7] == CPL_BINARY_1);
    cpl_test(cpl_mask_get_data_const(m)[8] == CPL_BINARY_1);
    cpl_image_delete(c);

    /* Check that the following holds A/B = C, where
     *     [ 6 -7  8]       1   [6 7 8]           [ 1 -1  1]
     * A = [-3  4 -5]  B = -- * [3 4 5]  C = 10 * [-1  1 -1]
     *     [ 0 -1  2]      10   [9 1 2]           [ 0 -1  1]
     * The resultant bad pixel masks should be (X indicates bad, '.' good):
     *         [. . .]
     * C_bpm = [. . .]
     *         [. . .]
     */
    data_b[6] = 0.6; data_b[7] = 0.7; data_b[8] = 0.8;
    data_b[3] = 0.3; data_b[4] = 0.4; data_b[5] = 0.5;
    data_b[0] = 0.9; data_b[1] = 0.1; data_b[2] = 0.2;
    c = cpl_image_divide_create(a, b);
    cpl_test_nonnull(c);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_abs(cpl_image_get_data_float_const(c)[0],   0.0, FLT_EPSILON);
    cpl_test_abs(cpl_image_get_data_float_const(c)[1], -10.0, FLT_EPSILON);
    cpl_test_abs(cpl_image_get_data_float_const(c)[2],  10.0, FLT_EPSILON);
    cpl_test_abs(cpl_image_get_data_float_const(c)[3], -10.0, FLT_EPSILON);
    cpl_test_abs(cpl_image_get_data_float_const(c)[4],  10.0, FLT_EPSILON);
    cpl_test_abs(cpl_image_get_data_float_const(c)[5], -10.0, FLT_EPSILON);
    cpl_test_abs(cpl_image_get_data_float_const(c)[6],  10.0, FLT_EPSILON);
    cpl_test_abs(cpl_image_get_data_float_const(c)[7], -10.0, FLT_EPSILON);
    cpl_test_abs(cpl_image_get_data_float_const(c)[8],  10.0, FLT_EPSILON);
    /* Check that there are no bad pixels. Note, we check this using a slightly
     * different method from above to test also cpl_image_is_rejected. */
    m = cpl_image_get_bpm_const(c);
    cpl_test_null(m);
    cpl_test_zero(cpl_image_is_rejected(c, 1, 1));
    cpl_test_zero(cpl_image_is_rejected(c, 2, 1));
    cpl_test_zero(cpl_image_is_rejected(c, 3, 1));
    cpl_test_zero(cpl_image_is_rejected(c, 1, 2));
    cpl_test_zero(cpl_image_is_rejected(c, 2, 2));
    cpl_test_zero(cpl_image_is_rejected(c, 3, 2));
    cpl_test_zero(cpl_image_is_rejected(c, 1, 3));
    cpl_test_zero(cpl_image_is_rejected(c, 2, 3));
    cpl_test_zero(cpl_image_is_rejected(c, 3, 3));
    cpl_image_delete(c);

    cpl_image_delete(a);
    cpl_image_delete(b);
}
