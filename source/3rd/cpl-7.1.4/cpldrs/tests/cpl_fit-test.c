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
#  include <config.h>
#endif

/*-----------------------------------------------------------------------------
                                Includes
 -----------------------------------------------------------------------------*/

#include "cpl_fit.h"

#include "cpl_memory.h"
#include "cpl_stats.h"
#include "cpl_tools.h"
#include "cpl_test.h"
#include "cpl_math_const.h"

#include <string.h>
#include <math.h>
#include <float.h>

/*-----------------------------------------------------------------------------
                                Defines
 -----------------------------------------------------------------------------*/

#ifndef IMAGESZ
#define IMAGESZ         10
#endif

#ifndef IMAGESZFIT
#define IMAGESZFIT      256
#endif

#ifndef WINSTART
#define WINSTART      3
#endif

#define IMSZ 6

#define IM13 13

/*----------------------------------------------------------------------------*/
/**
 * @defgroup cpl_fit_test Testing of the CPL utilities
 */
/*----------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------
                            Defines
 -----------------------------------------------------------------------------*/

#define cpl_fit_imagelist_is_zero(A, B)              \
    cpl_fit_imagelist_is_zero_macro(A, B, __LINE__)
#define cpl_fit_image_is_zero(A, B)                  \
    cpl_fit_image_is_zero_macro(A, B, __LINE__)

/*-----------------------------------------------------------------------------
                            Private Function prototypes
 -----------------------------------------------------------------------------*/

static void cpl_fit_imagelist_polynomial_tests(void);
static void cpl_fit_imagelist_polynomial_bpm(void);
static void cpl_fit_image_gaussian_tests(cpl_type, FILE *);
static void cpl_fit_imagelist_is_zero_macro(const cpl_imagelist *, double,
                                               int);
static void cpl_fit_image_is_zero_macro(const cpl_image *, double, int);
static void eval_gauss(const double x[], const double a[], double *);
static int  check_gauss_success(const char **, const double *, const double *,
                                const char **, const double *, const double *,
                                const cpl_array *, const cpl_array *, 
                                double, double, double, const cpl_matrix *,
                                FILE *);
static void cpl_fit_image_gaussian_test_one(void);

static void cpl_fit_image_gaussian_test_local(void);

static void cpl_fit_image_gaussian_test_bpm(void);

static
cpl_imagelist * cpl_fit_imagelist_polynomial_window_test(const cpl_vector    *,
                                                         const cpl_imagelist *,
                                                         int, int, int, int,
                                                         int, int, cpl_boolean,
                                                         cpl_type, cpl_image *);

/*----------------------------------------------------------------------------*/
/**
   @internal
   @brief   Unit tests of fit module
**/
/*----------------------------------------------------------------------------*/

int main(void)
{
    FILE        * stream;

    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    stream = cpl_msg_get_level() > CPL_MSG_INFO
        ? fopen("/dev/null", "a") : stdout;

    cpl_test_nonnull( stream );

    /* Insert tests below */
    cpl_fit_imagelist_polynomial_bpm();
    cpl_fit_image_gaussian_tests(CPL_TYPE_INT, stream);
    cpl_fit_image_gaussian_tests(CPL_TYPE_FLOAT, stream);
    cpl_fit_image_gaussian_tests(CPL_TYPE_DOUBLE, stream);
    cpl_fit_imagelist_polynomial_tests();
    cpl_fit_image_gaussian_test_one();
    cpl_fit_image_gaussian_test_local();
    cpl_fit_image_gaussian_test_bpm();

    if (stream != stdout) cpl_test_zero( fclose(stream) );

    /* End of tests */
    return cpl_test_end(0);
}


static void cpl_fit_imagelist_polynomial_tests(void)
{

    const double     ditval[]    = {0.0, 1.0, 2.0, 3.0, 4.0, 
                                    5.0, 6.0, 7.0, 8.0};
    const double     neqditval[] = {1.0,   2.0,  4.0,  6.0, 10.0,
                                    14.0, 16.0, 18.0, 19.0};
    cpl_imagelist  * fit;
    cpl_imagelist  * fit_eq;
    cpl_imagelist  * input;
    cpl_image      * dfiterror
        = cpl_image_new(IMAGESZFIT, IMAGESZFIT, CPL_TYPE_DOUBLE);
    cpl_image      * ffiterror
        = cpl_image_new(IMAGESZFIT, IMAGESZFIT, CPL_TYPE_FLOAT);
    cpl_image      * ifiterror
        = cpl_image_new(IMAGESZFIT, IMAGESZFIT, CPL_TYPE_INT);
    cpl_image      * cfiterror
        = cpl_image_new(IMAGESZFIT, IMAGESZFIT, CPL_TYPE_FLOAT_COMPLEX);
    cpl_image      * dfiterrorwin;
    const int        ndits = (int)(sizeof(ditval)/sizeof(double));
    CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual)
    cpl_vector     * vdit  = cpl_vector_wrap(ndits, (double*)ditval);
    cpl_vector     * nvdit = cpl_vector_wrap(ndits, (double*)neqditval);
    CPL_DIAG_PRAGMA_POP
    cpl_polynomial * vfiller = cpl_polynomial_new(1);
    const double     sqsum = 204.0; /* Sum of squares of ditvals */
    const double     mytol = 5.52 * FLT_EPSILON;
    cpl_size         i;
    const cpl_type   pixel_type[] = {CPL_TYPE_DOUBLE, CPL_TYPE_FLOAT,
                                     CPL_TYPE_INT};
    const int        ntypes = (int)(sizeof(pixel_type)/sizeof(pixel_type[0]));
    int              ntest;
    cpl_flops   flopsum = 0;
    cpl_flops   flops;
    double      secs = 0.0;
    size_t      bytes = 0;
    double      cputime;
    double      demax;
    cpl_error_code error;


    cpl_msg_info(cpl_func, "Testing with %d %d X %d images",
                 ndits, IMAGESZFIT, IMAGESZFIT);

    fit = cpl_fit_imagelist_polynomial(NULL, NULL, 0, 0, CPL_FALSE,
                                       CPL_TYPE_FLOAT, NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null( fit );
    cpl_imagelist_delete(fit);

    fit = cpl_fit_imagelist_polynomial_window(NULL, NULL, 1, 1,
                                              IMAGESZFIT, IMAGESZFIT,
                                              0, 0, CPL_FALSE,
                                              CPL_TYPE_FLOAT, NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null( fit );
    cpl_imagelist_delete(fit);

    input = cpl_imagelist_new();
    fit = cpl_fit_imagelist_polynomial(vdit, input, 0, 0, CPL_FALSE,
                                       CPL_TYPE_FLOAT, NULL);
    cpl_test_error(CPL_ERROR_INCOMPATIBLE_INPUT);
    cpl_test_null( fit );
    cpl_imagelist_delete(fit);

    fit = cpl_fit_imagelist_polynomial_window(vdit, input, 1, 1,
                                              IMAGESZFIT, IMAGESZFIT,
                                              0, 0, CPL_FALSE,
                                              CPL_TYPE_FLOAT, NULL);
    cpl_test_error(CPL_ERROR_INCOMPATIBLE_INPUT);
    cpl_test_null( fit );
    cpl_imagelist_delete(fit);

    fit = cpl_fit_imagelist_polynomial(vdit, input, 1, 1, CPL_FALSE,
                                       CPL_TYPE_FLOAT, NULL);
    cpl_test_error(CPL_ERROR_INCOMPATIBLE_INPUT);
    cpl_test_null( fit );
    cpl_imagelist_delete(fit);

    fit = cpl_fit_imagelist_polynomial_window(vdit, input, 1, 1,
                                              IMAGESZFIT, IMAGESZFIT,
                                              0, 0, CPL_FALSE,
                                              CPL_TYPE_FLOAT, NULL);
    cpl_test_error(CPL_ERROR_INCOMPATIBLE_INPUT);
    cpl_test_null( fit );
    cpl_imagelist_delete(fit);


    /* Test with all types of pixels */
    for (ntest = 0; ntest < ntypes; ntest++) {

        const cpl_type test_type = pixel_type[ntest];

        cpl_image * image = cpl_image_new(IMAGESZFIT, IMAGESZ, test_type);

        
        cpl_msg_info(cpl_func, "Fitting with pixel type: %s",
                     cpl_type_get_name(test_type));

        error = cpl_imagelist_set(input, image, 0);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        image = cpl_image_duplicate(image);

        error = cpl_image_fill_noise_uniform(image, 1.0, 20.0);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_image_multiply_scalar(image, ditval[1]);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_imagelist_set(input, image, 1);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        /* A perfectly linear set */
        for (i = 2; i < ndits; i++) {

            image
                = cpl_image_multiply_scalar_create(cpl_imagelist_get(input, 1),
                                                   ditval[i]);

            error = cpl_imagelist_set(input, image, i);
            cpl_test_eq_error(error, CPL_ERROR_NONE);

        }

        flops = cpl_tools_get_flops();

        cputime = cpl_test_get_cputime();

        fit = cpl_fit_imagelist_polynomial_window_test(vdit, input, 1, 1,
                                                       IMAGESZFIT, IMAGESZ,
                                                       1, ndits-1, CPL_FALSE,
                                                       test_type, NULL);
        secs += cpl_test_get_cputime() - cputime;
        flopsum += cpl_tools_get_flops() - flops;
        bytes += cpl_test_get_bytes_imagelist(input);

        cpl_test_error(CPL_ERROR_NONE );
        cpl_test_eq( cpl_imagelist_get_size(fit), ndits - 1 );

        /* The linarity must be equal to the values in image 1
           - normalize */
        error = cpl_image_divide(cpl_imagelist_get(fit, 0),
                                 cpl_imagelist_get(input, 1));
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        /* Subtract the expected value in the 1st image */
        error = cpl_image_subtract_scalar(cpl_imagelist_get(fit, 0), 1.0);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        cpl_fit_imagelist_is_zero(fit, IMAGESZFIT * mytol);

        cpl_imagelist_delete(fit);

        flops = cpl_tools_get_flops();

        cputime = cpl_test_get_cputime();

        fit = cpl_fit_imagelist_polynomial_window_test(vdit, input,
                                                       WINSTART, WINSTART,
                                                       IMAGESZFIT - WINSTART,
                                                       IMAGESZ - WINSTART,
                                                       1, ndits-1, CPL_FALSE,
                                                       test_type, NULL);
        secs += cpl_test_get_cputime() - cputime;
        flopsum += cpl_tools_get_flops() - flops;
        bytes += cpl_test_get_bytes_imagelist(input);


        cpl_test_error(CPL_ERROR_NONE );
        cpl_test_eq( cpl_imagelist_get_size(fit), ndits - 1 );

        cpl_imagelist_delete(fit);

        /* 
         * For the _window() version of the fitting function, we don't test
         * the correctness of the products any further here,
         * as it would add too much complexity and the objective inside 
         * this loop is mainly to test on different pixel tests. We will do
         * do it in later steps.
         */

        cpl_imagelist_delete(input);
        input = cpl_imagelist_new();

        /*
         * Repeat with non-equidistant but symmetric x-axis values
         * and check validity of the eq_dist in this case
         */

        image = cpl_image_new(IMAGESZFIT, IMAGESZ, test_type);

        error = cpl_image_fill_noise_uniform(image, 1.0, 20.0);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_image_multiply_scalar(image, neqditval[0]);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_imagelist_set(input, image, 0);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        /* A perfectly linear set */
        for (i = 1; i < ndits; i++) {

            image
                = cpl_image_multiply_scalar_create(cpl_imagelist_get(input, 0),
                                                   neqditval[i]);

            error = cpl_imagelist_set(input, image, i);
            cpl_test_eq_error(error, CPL_ERROR_NONE);

        }

        /* Fit without is_eqdist option */
        flops = cpl_tools_get_flops();

        cputime = cpl_test_get_cputime();

        fit = cpl_fit_imagelist_polynomial_window_test(nvdit, input, 1, 1,
                                                       IMAGESZFIT, IMAGESZ,
                                                       1, ndits-1, CPL_FALSE,
                                                       test_type, NULL);
        secs += cpl_test_get_cputime() - cputime;
        flopsum += cpl_tools_get_flops() - flops;
        bytes += cpl_test_get_bytes_imagelist(input);

        cpl_test_error(CPL_ERROR_NONE );
        cpl_test_eq( cpl_imagelist_get_size(fit), ndits - 1 );

        /* Repeat with is_eqdist */
        flops = cpl_tools_get_flops();

        cputime = cpl_test_get_cputime();

        fit_eq = cpl_fit_imagelist_polynomial_window_test(nvdit, input, 1, 1,
                                                          IMAGESZFIT, IMAGESZ,
                                                          1, ndits-1, CPL_TRUE,
                                                          test_type, NULL);
        secs += cpl_test_get_cputime() - cputime;
        flopsum += cpl_tools_get_flops() - flops;
        bytes += cpl_test_get_bytes_imagelist(input);

        cpl_test_imagelist_abs(fit, fit_eq, mytol);

        cpl_imagelist_delete(fit);
        cpl_imagelist_delete(fit_eq);
        cpl_imagelist_delete(input);
        input = cpl_imagelist_new();

    }



    /* 2nd order function check with and without eq_dist */
    for (i=0; i < ndits; i++) {
        cpl_image * image = cpl_image_new(IMAGESZFIT, IMAGESZFIT,
                                          CPL_TYPE_DOUBLE);

        error = cpl_image_add_scalar(image, neqditval[i]*neqditval[i]);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_imagelist_set(input, image, i);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        cpl_msg_debug(cpl_func, "Dit and mean of input image no. %"
                      CPL_SIZE_FORMAT ": %g %g",
                      i, ditval[i], cpl_image_get_mean(image));
    }

    fit = cpl_fit_imagelist_polynomial_window_test(nvdit, input, 1, 1,
                                                   IMAGESZFIT, IMAGESZFIT,
                                                   1, ndits, CPL_FALSE,
                                                   CPL_TYPE_FLOAT, NULL);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(fit);

    fit_eq = cpl_fit_imagelist_polynomial_window_test(nvdit, input, 1, 1,
                                                      IMAGESZFIT, IMAGESZFIT,
                                                      1, ndits, CPL_TRUE,
                                                      CPL_TYPE_FLOAT, NULL);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(fit_eq);

    cpl_test_imagelist_abs(fit, fit_eq, mytol);

    cpl_imagelist_delete(fit);
    cpl_imagelist_delete(fit_eq);
    cpl_imagelist_delete(input);
    input = cpl_imagelist_new();

    cpl_test_eq_ptr(cpl_vector_unwrap(nvdit), neqditval);

    /* Create a list of images with a 2nd order function */
    for (i=0; i < ndits; i++) {
        cpl_image * image = cpl_image_new(IMAGESZFIT, IMAGESZFIT,
                                          CPL_TYPE_DOUBLE);

        error = cpl_image_add_scalar(image, ditval[i]*ditval[i]);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_imagelist_set(input, image, i);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        cpl_msg_info(cpl_func, "Dit and mean of input image no. %"
                     CPL_SIZE_FORMAT ": %g %g",
                     i, ditval[i], cpl_image_get_mean(image));
    }

    /* Fit a high-order overdetermined system and a maximum-order,
       non-overdetermined system */

    for (i = ndits-1; i <= ndits; i++) {

        fit = cpl_fit_imagelist_polynomial_window_test(vdit, input, 1, 1,
                                                       IMAGESZFIT, IMAGESZFIT,
                                                       1, i, CPL_FALSE,
                                                       CPL_TYPE_FLOAT, NULL);
#if defined SIZEOF_SIZE_T && SIZEOF_SIZE_T == 4
        if (i == ndits && fit == NULL) {
            /* FIXME: When fitting a non-overdetermined, maximum-order system
               on (some) 32-bit machines the rounding errors in the Vandermonde
               matrix become dominant to the point that the Cholesky
               decomposition fails.
            */
            cpl_test_error(CPL_ERROR_SINGULAR_MATRIX);
        } else
#endif
            {
                cpl_test_error(CPL_ERROR_NONE);
                cpl_test_nonnull(fit);
                cpl_imagelist_delete(fit);
            }

    }

    /* Illegal max-degree */
    fit = cpl_fit_imagelist_polynomial(vdit, input, 1, 0, CPL_FALSE,
                                       CPL_TYPE_FLOAT, NULL);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null( fit );

    /* Illegal min-degree */
    fit = cpl_fit_imagelist_polynomial(vdit, input, -1, 0, CPL_FALSE,
                                       CPL_TYPE_FLOAT, NULL);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null( fit );

    /* Illegal pixeltype */
    fit = cpl_fit_imagelist_polynomial(vdit, input, 0, 0, CPL_FALSE,
                                       CPL_TYPE_INVALID, NULL);
    cpl_test_error(CPL_ERROR_UNSUPPORTED_MODE);
    cpl_test_null( fit );

    flops = cpl_tools_get_flops();
    cputime = cpl_test_get_cputime();

    /* Fit with zero-order term */
    /* Also, try to use an integer-type image for fitting error */
    fit = cpl_fit_imagelist_polynomial_window_test(vdit, input, 1, 1,
                                                   IMAGESZFIT, IMAGESZFIT,
                                                   0, 2, CPL_TRUE,
                                                   CPL_TYPE_FLOAT,
                                                   ifiterror);
    secs += cpl_test_get_cputime() - cputime;
    flopsum += cpl_tools_get_flops() - flops;
    bytes += cpl_test_get_bytes_imagelist(input);

    cpl_test_error(CPL_ERROR_NONE );
    cpl_test_eq( cpl_imagelist_get_size(fit), 3 );
    cpl_fit_image_is_zero(ifiterror, mytol); 

    error = cpl_image_subtract_scalar(cpl_imagelist_get(fit, 2), 1.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_fit_imagelist_is_zero(fit, mytol);

    cpl_imagelist_delete(fit);


    flops = cpl_tools_get_flops();
    cputime = cpl_test_get_cputime();

    /* Fit with zero-order term */
    /* Also, try to use an integer-type image for fitting error */
    fit = cpl_fit_imagelist_polynomial_window_test(vdit, input, 1, 1,
                                                   IMAGESZFIT, IMAGESZFIT,
                                                   0, ndits-1, CPL_TRUE,
                                                   CPL_TYPE_FLOAT,
                                                   ifiterror);
    secs += cpl_test_get_cputime() - cputime;
    flopsum += cpl_tools_get_flops() - flops;
    bytes += cpl_test_get_bytes_imagelist(input);

    cpl_test_error(CPL_ERROR_NONE );
    cpl_test_eq( cpl_imagelist_get_size(fit), ndits );
    cpl_fit_image_is_zero(ifiterror, mytol);

    error = cpl_image_subtract_scalar(cpl_imagelist_get(fit, 2), 1.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_fit_imagelist_is_zero(fit, mytol);

    cpl_imagelist_delete(fit);

    /* Fit without zero-order term */
    fit = cpl_fit_imagelist_polynomial_window_test(vdit, input, 1, 1,
                                                   IMAGESZFIT, IMAGESZFIT,
                                                   1, ndits-1, CPL_FALSE,
                                                   CPL_TYPE_FLOAT, dfiterror);
    secs += cpl_test_get_cputime() - cputime;
    flopsum += cpl_tools_get_flops() - flops;
    bytes += cpl_test_get_bytes_imagelist(input);

    cpl_test_error(CPL_ERROR_NONE );
    cpl_test_eq( cpl_imagelist_get_size(fit), ndits-1 );

    error = cpl_image_subtract_scalar(cpl_imagelist_get(fit, 1), 1.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_fit_imagelist_is_zero(fit, mytol);
    cpl_fit_image_is_zero(dfiterror, mytol);

    cpl_imagelist_delete(fit);

    /* A valid fit, except that the fitting error is complex */
    fit = cpl_fit_imagelist_polynomial(vdit, input, 1, ndits-1, CPL_FALSE,
                                       CPL_TYPE_FLOAT, cfiterror);
    cpl_test_error(CPL_ERROR_UNSUPPORTED_MODE);
    cpl_test_null(fit);

    /* A valid fit, except that the fitting type is complex */
    fit = cpl_fit_imagelist_polynomial(vdit, input, 1, ndits-1, CPL_FALSE,
                                       CPL_TYPE_FLOAT_COMPLEX, ffiterror);
    cpl_test_error(CPL_ERROR_UNSUPPORTED_MODE);
    cpl_test_null(fit);

    flops = cpl_tools_get_flops();
    cputime = cpl_test_get_cputime();

    /* Repeat previous test on _window() function */
    dfiterrorwin = cpl_image_new(IMAGESZFIT - 2 * WINSTART + 1,
                                 IMAGESZ - 2 * WINSTART + 1,
                                 CPL_TYPE_DOUBLE);

    fit = cpl_fit_imagelist_polynomial_window_test(vdit, input,
                                                   WINSTART, WINSTART,
                                                   IMAGESZFIT - WINSTART,
                                                   IMAGESZ - WINSTART,
                                                   1, ndits-1, CPL_FALSE,
                                                   CPL_TYPE_FLOAT,
                                                   dfiterrorwin);
    secs += cpl_test_get_cputime() - cputime;
    flopsum += cpl_tools_get_flops() - flops;
    bytes += cpl_test_get_bytes_imagelist(input); /* FIXME: Too large... */

    cpl_test_error(CPL_ERROR_NONE );
    cpl_test_eq( cpl_imagelist_get_size(fit), ndits-1 );

    cpl_test_eq(cpl_image_get_size_x(cpl_imagelist_get(fit, 0)), 
                IMAGESZFIT - 2 * WINSTART + 1 );
    cpl_test_eq(cpl_image_get_size_y(cpl_imagelist_get(fit, 0)), 
                IMAGESZ - 2 * WINSTART + 1 );

    error = cpl_image_subtract_scalar(cpl_imagelist_get(fit, 1), 1.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_fit_imagelist_is_zero(fit, mytol);
    cpl_fit_image_is_zero(dfiterrorwin, mytol);

    cpl_imagelist_delete(fit);
    cpl_image_delete(dfiterrorwin);

    flops = cpl_tools_get_flops();
    cputime = cpl_test_get_cputime();

    /* Fit with no zero- and 1st-order terms */
    fit = cpl_fit_imagelist_polynomial_window_test(vdit, input, 1, 1,
                                                   IMAGESZFIT, IMAGESZFIT,
                                                   2, ndits, CPL_TRUE,
                                                   CPL_TYPE_FLOAT, ffiterror);
    secs += cpl_test_get_cputime() - cputime;
    flopsum += cpl_tools_get_flops() - flops;
    bytes += cpl_test_get_bytes_imagelist(input);

    cpl_test_error(CPL_ERROR_NONE );
    cpl_test_eq( cpl_imagelist_get_size(fit), ndits-1 );

    error = cpl_image_subtract_scalar(cpl_imagelist_get(fit, 0), 1.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_fit_imagelist_is_zero(fit, mytol);
    cpl_fit_image_is_zero(ffiterror, mytol);

    cpl_imagelist_delete(fit);

    flops = cpl_tools_get_flops();
    cputime = cpl_test_get_cputime();

    /* Fit with one zero-term */
    fit = cpl_fit_imagelist_polynomial_window_test(vdit, input, 1, 1,
                                                   IMAGESZFIT, IMAGESZFIT,
                                                   0, 0, CPL_TRUE,
                                                   CPL_TYPE_FLOAT,
                                                   dfiterror);
    secs += cpl_test_get_cputime() - cputime;
    flopsum += cpl_tools_get_flops() - flops;
    bytes += cpl_test_get_bytes_imagelist(input);

    cpl_test_error(CPL_ERROR_NONE );
    cpl_test_eq( cpl_imagelist_get_size(fit), 1 );

    error = cpl_image_subtract_scalar(cpl_imagelist_get(fit, 0),
                                      sqsum/(double)ndits);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_fit_imagelist_is_zero(fit, mytol);

    cpl_imagelist_delete(fit);

    cpl_imagelist_delete(input);

    cpl_test_eq_ptr(cpl_vector_unwrap(vdit), ditval);

    /* Try to fit as many coefficients are there are data points */
    /* Also, use floats this time */

    /* Use a polynomial to filler vdit */
    i = 0;
    error = cpl_polynomial_set_coeff(vfiller, &i, 0.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    i = 1;
    error = cpl_polynomial_set_coeff(vfiller, &i, 1.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    input = cpl_imagelist_new();

    vdit = cpl_vector_new(1);

    for (ntest = 1; ntest <= ndits; ntest++) {
        const double gain = 4.0; /* Some random number */

        cpl_msg_info(cpl_func, "Fitting %d coefficients to as many points",
                     ntest);

        error = cpl_vector_set_size(vdit, ntest);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_vector_fill_polynomial(vdit, vfiller, 0.0, 1.0);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        /* Create a list of images with a 2nd order function */
        for (i = ntest - 1; i < ntest; i++) {
            cpl_image * image = cpl_image_new(IMAGESZFIT, IMAGESZFIT,
                                              CPL_TYPE_FLOAT);

            error = cpl_image_add_scalar(image, gain * ditval[i]*ditval[i]);
            cpl_test_eq_error(error, CPL_ERROR_NONE);

            error = cpl_imagelist_set(input, image, i);
            cpl_test_eq_error(error, CPL_ERROR_NONE);

            cpl_msg_info(cpl_func, "Dit and mean of input image no. %"
                         CPL_SIZE_FORMAT ": %g %g",
                         i, ditval[i], cpl_image_get_mean(image));
        }

        /* Ready for fitting */

        flops = cpl_tools_get_flops();
        cputime = cpl_test_get_cputime();

        /* Fit with zero-order term */
        fit = cpl_fit_imagelist_polynomial_window_test(vdit, input, 1, 1,
                                                       IMAGESZFIT, IMAGESZFIT,
                                                       0, ntest-1, CPL_TRUE,
                                                       CPL_TYPE_FLOAT,
                                                       ffiterror);
        if (cpl_error_get_code() != CPL_ERROR_NONE) {
            cpl_test_error(CPL_ERROR_SINGULAR_MATRIX );
            cpl_test_null( fit );
            cpl_msg_warning(cpl_func, "Could not fit %d coefficients to as many "
                            "points", ntest);


            break;
        }

        secs += cpl_test_get_cputime() - cputime;
        flopsum += cpl_tools_get_flops() - flops;
        bytes += cpl_test_get_bytes_imagelist(input);

        cpl_test_eq( cpl_imagelist_get_size(fit), ntest );

        if (ntest == 2) {
            error = cpl_image_subtract_scalar(cpl_imagelist_get(fit, 1), gain);
            cpl_test_eq_error(error, CPL_ERROR_NONE);
        } else if (ntest > 2) {
            error = cpl_image_subtract_scalar(cpl_imagelist_get(fit, 2), gain);
            cpl_test_eq_error(error, CPL_ERROR_NONE);
        }

        cpl_fit_imagelist_is_zero(fit, ntest * mytol);

        cpl_fit_image_is_zero(ffiterror, ntest * mytol);

        cpl_imagelist_delete(fit);
    }

    cpl_imagelist_delete(input);
    cpl_vector_delete(vdit);

    /* Multiple samples at three different (equidistant) points */

    input = cpl_imagelist_new();

    vdit = cpl_vector_new(9);
    error = cpl_vector_set(vdit, 0, 5.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_vector_set(vdit, 1, 5.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_vector_set(vdit, 2, 5.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_vector_set(vdit, 3, 3.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_vector_set(vdit, 4, 3.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_vector_set(vdit, 5, 3.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_vector_set(vdit, 6, 7.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_vector_set(vdit, 7, 7.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_vector_set(vdit, 8, 7.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_imagelist_set(input, cpl_image_fill_test_create(IMAGESZFIT,
                                                                IMAGESZFIT), 0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_imagelist_set(input, cpl_image_fill_test_create(IMAGESZFIT,
                                                                IMAGESZFIT), 1);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_imagelist_set(input, cpl_image_fill_test_create(IMAGESZFIT,
                                                                IMAGESZFIT), 2);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_imagelist_set(input, cpl_image_fill_test_create(IMAGESZFIT,
                                                                IMAGESZFIT), 3);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_imagelist_set(input, cpl_image_fill_test_create(IMAGESZFIT,
                                                                IMAGESZFIT), 4);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_imagelist_set(input, cpl_image_fill_test_create(IMAGESZFIT,
                                                                IMAGESZFIT), 5);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_imagelist_set(input, cpl_image_fill_test_create(IMAGESZFIT,
                                                                IMAGESZFIT), 6);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_imagelist_set(input, cpl_image_fill_test_create(IMAGESZFIT,
                                                                IMAGESZFIT), 7);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_imagelist_set(input, cpl_image_fill_test_create(IMAGESZFIT,
                                                                IMAGESZFIT), 8);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    /* Fit a non-over-determined set without constant and linear terms */
    flops = cpl_tools_get_flops();
    cputime = cpl_test_get_cputime();

    fit = cpl_fit_imagelist_polynomial_window_test(vdit, input, 1, 1,
                                                   IMAGESZFIT, IMAGESZFIT,
                                                   2, 4, CPL_FALSE,
                                                   CPL_TYPE_DOUBLE, dfiterror);

    secs += cpl_test_get_cputime() - cputime;
    flopsum += cpl_tools_get_flops() - flops;
    bytes += cpl_test_get_bytes_imagelist(input);

    cpl_test_error(CPL_ERROR_NONE );
    cpl_test_eq( cpl_imagelist_get_size(fit), 3 );

    cpl_fit_image_is_zero(dfiterror, DBL_MAX);

    cpl_imagelist_delete(fit);

    /* Fit a non-over-determined set with a constant term */
    flops = cpl_tools_get_flops();
    cputime = cpl_test_get_cputime();

    fit = cpl_fit_imagelist_polynomial_window_test(vdit, input, 1, 1,
                                              IMAGESZFIT, IMAGESZFIT,
                                              0, 2, CPL_FALSE,
                                              CPL_TYPE_DOUBLE, dfiterror);

    secs += cpl_test_get_cputime() - cputime;
    flopsum += cpl_tools_get_flops() - flops;
    bytes += cpl_test_get_bytes_imagelist(input);

    cpl_test_error(CPL_ERROR_NONE );
    cpl_test_eq( cpl_imagelist_get_size(fit), 3 );

    cpl_fit_image_is_zero(dfiterror, DBL_MAX);

    demax = cpl_image_get_sqflux(dfiterror);

    cpl_imagelist_delete(fit);


    /* Repeat the previous test - this time exploiting that
       the sampling points are equidistant */
    flops = cpl_tools_get_flops();
    cputime = cpl_test_get_cputime();

    fit = cpl_fit_imagelist_polynomial_window_test(vdit, input, 1, 1,
                                                   IMAGESZFIT, IMAGESZFIT,
                                                   0, 2, CPL_TRUE,
                                                   CPL_TYPE_DOUBLE, dfiterror);

    secs += cpl_test_get_cputime() - cputime;
    flopsum += cpl_tools_get_flops() - flops;
    bytes += cpl_test_get_bytes_imagelist(input);

    cpl_test_error(CPL_ERROR_NONE );
    cpl_test_eq( cpl_imagelist_get_size(fit), 3 );

    cpl_fit_image_is_zero(dfiterror, DBL_MAX);

    cpl_test_leq( cpl_image_get_sqflux(dfiterror), demax );

    cpl_imagelist_delete(fit);

    /* Repeat the previous test - after sorting the sampling points */
    error = cpl_vector_sort(vdit, CPL_SORT_ASCENDING);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    flops = cpl_tools_get_flops();
    cputime = cpl_test_get_cputime();

    fit = cpl_fit_imagelist_polynomial_window_test(vdit, input, 1, 1,
                                                   IMAGESZFIT, IMAGESZFIT,
                                                   0, 2, CPL_TRUE,
                                                   CPL_TYPE_DOUBLE, dfiterror);

    secs += cpl_test_get_cputime() - cputime;
    flopsum += cpl_tools_get_flops() - flops;
    bytes += cpl_test_get_bytes_imagelist(input);

    cpl_test_error(CPL_ERROR_NONE );
    cpl_test_eq( cpl_imagelist_get_size(fit), 3 );

    cpl_fit_image_is_zero(dfiterror, DBL_MAX);

    cpl_imagelist_delete(fit);

    /* Fit an under-determined set without a constant term */
    fit = cpl_fit_imagelist_polynomial(vdit, input, 1, 4, CPL_FALSE,
                                       CPL_TYPE_DOUBLE, dfiterror);

    cpl_test_error(CPL_ERROR_SINGULAR_MATRIX );
    cpl_test_null( fit );

    cpl_imagelist_delete(input);
    cpl_vector_delete(vdit);


    cpl_msg_info("","Speed while fitting with image size %d in %g secs "
                 "[Mflop/s]: %g", IMAGESZFIT, secs,
                 (double)flopsum/secs/1e6);
    if (secs > 0.0) {
        cpl_msg_info(cpl_func,"Processing rate [MB/s]: %g",
                     1e-6 * (double)bytes / secs);
    }

    cpl_image_delete(cfiterror);
    cpl_image_delete(dfiterror);
    cpl_image_delete(ffiterror);
    cpl_image_delete(ifiterror);
    cpl_polynomial_delete(vfiller);

}



/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Verify that all elements in an imagelist are zero (within a tolerance)
  @param  self    The list of images to check
  @param  tol     The non-negative tolerance
  param   line    The line number of the caller
  @return void

 */
/*----------------------------------------------------------------------------*/
static void cpl_fit_imagelist_is_zero_macro(const cpl_imagelist * self,
                                               double tol, int line)
{

    const cpl_size n = cpl_imagelist_get_size(self);
    cpl_size i;

    for (i = 0; i < n; i++) {

        /* FIXME: Need traceback of failure */

        cpl_fit_image_is_zero_macro(cpl_imagelist_get_const(self, i), tol,
                                    line);
    }
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Verify that all elements in an image are zero (within a tolerance)
  @param  self    The image to check
  @param  tol     The non-negative tolerance
  param   line    The line number of the caller
  @return void

 */
/*----------------------------------------------------------------------------*/
static void cpl_fit_image_is_zero_macro(const cpl_image * self, double tol,
                                           int line)
{

    cpl_stats * stats = cpl_stats_new_from_image(self,
                                                 CPL_STATS_MIN | CPL_STATS_MAX
                                                 | CPL_STATS_MEAN);

    const double mymin = cpl_stats_get_min(stats);
    const double mymax = cpl_stats_get_max(stats);

    /* FIXME: Need traceback of failure */
    cpl_test(line > 0);

    cpl_test_leq( fabs(mymin), tol );
    cpl_test_leq( fabs(mymax), tol );

    cpl_stats_delete(stats);

}

static void cpl_fit_image_gaussian_tests(cpl_type pixeltype, FILE * stream)
{
    cpl_image  *box        = NULL;
    cpl_image  *image      = NULL;
    cpl_image  *eimage     = NULL;
    cpl_matrix *covariance = NULL;
    cpl_matrix *phys_cov   = NULL;
    cpl_array  *parameters = NULL;
    cpl_array  *err_params = NULL;
    cpl_array  *fit_params = NULL;
    double      major, minor, angle, rms, chisq, value;
    const int   nx         = 30;
    const int   ny         = 30;
    const int   lnx        = 2438;
    const int   lny        = 2438;
    int         i, j;
    cpl_error_code error;

    double x[2];
    const char  *p[7] = { /* Parameter names */
                            "Background       ",
                            "Normalisation    ",
                            "Correlation      ",
                            "Center position x",
                            "Center position y",
                            "Sigma x          ",
                            "Sigma y          "};

    double u[7] = { /* Parameter values of simulated gaussian */
                            2.130659, 
                       104274.700696, 
                            0.779320, 
                          796.851741, 
                         1976.324361, 
                            4.506552, 
                            3.248286 };

    const double e[7] = { /* Errors on parameters of simulated gaussian */
                            2.029266,
                          955.703656, 
                            0.004452, 
                            0.035972, 
                            0.025949, 
                            0.037186, 
                            0.026913 };

    const char  *fp[3] = { /* Physical parameter names */
                            "Angle            ",
                            "Major semi-axis  ",
                            "Minor semi-axis  "};
                            
    const double fu[3] = { /* Physical parameter values of simulated gaussian */
                            33.422720,
                             5.276152,
                             1.738563};

    const double fe[3] = { /* Errors on physical parameters of gaussian */
                             0.238359,
                             0.283202,
                             0.052671};


    /*
     * Fill image with model gaussian.
     */

    box = cpl_image_new(nx, ny, pixeltype);

    u[3] -= 782;
    u[4] -= 1961;
    for (j = 1; j <= ny; j++) {
        x[1] = j;
        for (i = 1; i <= nx; i++) {
             x[0] = i;
             eval_gauss(x, u, &value);
             cpl_image_set(box, i, j, value);
        }
    }
    u[3] += 782;
    u[4] += 1961;


    /*
     * Insert it into a bigger image.
     */

    image = cpl_image_new(lnx, lny, pixeltype);
    cpl_image_copy(image, box, 783, 1962);
    cpl_image_delete(box); box = NULL;


    /*
     * Add some noise, and create corresponding error image
     * (not really physical, but this is just for testing).
     */

    eimage = cpl_image_new(lnx, lny, pixeltype);
    cpl_image_fill_noise_uniform(eimage, -50, 50);
    cpl_image_add(image, eimage);
    cpl_image_delete(eimage);
    eimage = cpl_image_new(lnx, lny, pixeltype);
    cpl_image_fill_noise_uniform(eimage, 50, 60);


    /*
     * Allocate the parameters array, with the error array, and leave
     * them empty (for the moment).
     */

    parameters = cpl_array_new(7, CPL_TYPE_DOUBLE);

    err_params = cpl_array_new(7, CPL_TYPE_DOUBLE);


    /*
     * Now see if we can find a gaussian without any first guess:
     */

    for (int itry = 0; itry < 2; itry++) {

        phys_cov = NULL;
        covariance = NULL;
        error = cpl_fit_image_gaussian(image, eimage, 797, 1976, 30, 30,
                                       parameters, err_params, fit_params, 
                                       &rms, &chisq, &covariance, &major, 
                                       &minor, &angle, &phys_cov);
        cpl_test_eq_error(error, CPL_ERROR_NONE);
        if (!error || cpl_msg_get_level() <= CPL_MSG_INFO)
            cpl_test(check_gauss_success(p, u, e, fp, fu, fe, parameters,
                                         err_params, angle, major, minor,
                                         phys_cov, stream));

        cpl_matrix_delete(covariance);
        cpl_matrix_delete(phys_cov);

        /*
         * Now retry with the fit as first-guess, all parameters free.
         */
    }


    /*
     * Now retry with a first-guess, all parameters free.
     */

    fit_params = cpl_array_new(7, CPL_TYPE_INT);

    for (i = 0; i < 7; i++)
        cpl_array_set(fit_params, i, 1);

    for (i = 0; i < 7; i++)
        cpl_array_set(parameters, i, u[i]);

    for (i = 0; i < 7; i++)
        cpl_array_set(err_params, i, e[i]);

    for (int itry = 0; itry < 2; itry++) {

        phys_cov = NULL;
        covariance = NULL;
        error = cpl_fit_image_gaussian(image, eimage, 797, 1976, 30, 30,
                                       parameters, err_params, fit_params, 
                                       &rms, &chisq, &covariance, &major, 
                                       &minor, &angle, &phys_cov);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        if (!error || cpl_msg_get_level() <= CPL_MSG_INFO)
            cpl_test(check_gauss_success(p, u, e, fp, fu, fe, parameters,
                                         err_params, angle, major, minor,
                                         phys_cov, stream));

        cpl_matrix_delete(covariance);
        cpl_matrix_delete(phys_cov);

        /*
         * Now retry with a first-guess, two parameters are frozen.
         */

        for (i = 0; i < 7; i++)
            cpl_array_set(parameters, i, u[i]);

        for (i = 0; i < 7; i++)
            cpl_array_set(err_params, i, e[i]);

        cpl_array_set(fit_params, 0, 0);
        cpl_array_set(fit_params, 4, 0);

        phys_cov = NULL;
        covariance = NULL;
        error = cpl_fit_image_gaussian(image, eimage, 797, 1976, 30, 30,
                                       parameters, err_params, fit_params,
                                       &rms, &chisq, &covariance, &major,
                                       &minor, &angle, &phys_cov);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        if (!error || cpl_msg_get_level() <= CPL_MSG_INFO)
            cpl_test(check_gauss_success(p, u, e, fp, fu, fe, parameters,
                                         err_params, angle, major, minor,
                                         phys_cov, stream));

        cpl_matrix_delete(covariance);
        cpl_matrix_delete(phys_cov);


        /*
         * Now try without eny error propagation
         */

        phys_cov = NULL;
        covariance = NULL;
        error = cpl_fit_image_gaussian(image, NULL, 797, 1976, 30, 30,
                                       parameters, NULL, fit_params,
                                       &rms, NULL, NULL, &major,
                                       &minor, &angle, NULL);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        if (!error || cpl_msg_get_level() <= CPL_MSG_INFO)
            cpl_test(check_gauss_success(p, u, e, fp, fu, fe, parameters,
                                         err_params, angle, major, minor,
                                         phys_cov, stream));
    }

    /*
     * Try different error situations:
     */

    error = cpl_fit_image_gaussian(NULL, eimage, 797, 1976, 30, 30,
                                   parameters, err_params, fit_params,
                                   &rms, &chisq, &covariance, &major,
                                   &minor, &angle, &phys_cov);

    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

    error = cpl_fit_image_gaussian(image, eimage, 797, 1976, 30, 30,
                                   NULL, err_params, fit_params,
                                   &rms, &chisq, &covariance, &major,
                                   &minor, &angle, &phys_cov);

    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

    error = cpl_fit_image_gaussian(image, eimage, -1, 1976, 30, 30,
                                   parameters, err_params, fit_params,
                                   &rms, &chisq, &covariance, &major,
                                   &minor, &angle, &phys_cov);

    cpl_test_eq_error(error, CPL_ERROR_ACCESS_OUT_OF_RANGE);

    error = cpl_fit_image_gaussian(image, eimage, 797, 197600, 30, 30,
                                   parameters, err_params, fit_params,
                                   &rms, &chisq, &covariance, &major,
                                   &minor, &angle, &phys_cov);

    cpl_test_eq_error(error, CPL_ERROR_ACCESS_OUT_OF_RANGE);

    error = cpl_fit_image_gaussian(image, eimage, 797, 1976, 300000, 30,
                                   parameters, err_params, fit_params,
                                   &rms, &chisq, &covariance, &major,
                                   &minor, &angle, &phys_cov);

    cpl_test_eq_error(error, CPL_ERROR_ACCESS_OUT_OF_RANGE);

    error = cpl_fit_image_gaussian(image, eimage, 797, 1976, 2, 30,
                                   parameters, err_params, fit_params,
                                   &rms, &chisq, &covariance, &major,
                                   &minor, &angle, &phys_cov);
    
    cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

    error = cpl_fit_image_gaussian(image, eimage, 797, 1976, 30, 300000,
                                   parameters, err_params, fit_params,
                                   &rms, &chisq, &covariance, &major,
                                   &minor, &angle, &phys_cov);

    cpl_test_eq_error(error, CPL_ERROR_ACCESS_OUT_OF_RANGE);

    error = cpl_fit_image_gaussian(image, eimage, 797, 1976, 30, 2,
                                   parameters, err_params, fit_params,
                                   &rms, &chisq, &covariance, &major,
                                   &minor, &angle, &phys_cov);

    cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

    box = cpl_image_new(46, 72, pixeltype);
    error = cpl_fit_image_gaussian(image, box, 797, 1976, 30, 30,
                                   parameters, err_params, fit_params,
                                   &rms, &chisq, &covariance, &major,
                                   &minor, &angle, &phys_cov);
    cpl_image_delete(box);
    cpl_test_eq_error(error, CPL_ERROR_INCOMPATIBLE_INPUT);

    error = cpl_fit_image_gaussian(image, eimage, 797, 1976, 30, 30,
                                   parameters, fit_params, fit_params,
                                   &rms, &chisq, &covariance, &major,
                                   &minor, &angle, &phys_cov);

    cpl_test_eq_error(error, CPL_ERROR_INVALID_TYPE);

    error = cpl_fit_image_gaussian(image, eimage, 797, 1976, 30, 30,
                                   fit_params, err_params, fit_params,
                                   &rms, &chisq, &covariance, &major,
                                   &minor, &angle, &phys_cov);

    cpl_test_eq_error(error, CPL_ERROR_INVALID_TYPE);

    error = cpl_fit_image_gaussian(image, eimage, 797, 1976, 30, 30,
                                   parameters, err_params, err_params,
                                   &rms, &chisq, &covariance, &major,
                                   &minor, &angle, &phys_cov);

    cpl_test_eq_error(error, CPL_ERROR_INVALID_TYPE);

    error = cpl_fit_image_gaussian(image, NULL, 797, 1976, 30, 30,
                                   parameters, err_params, fit_params,
                                   &rms, &chisq, &covariance, &major,
                                   &minor, &angle, &phys_cov);

    cpl_test_eq_error(error, CPL_ERROR_DATA_NOT_FOUND);

    for (i = 0; i < 7; i++)
        cpl_array_set(fit_params, i, 0); /* All frozen */

    error = cpl_fit_image_gaussian(image, eimage, 797, 1976, 30, 30,
                                   parameters, err_params, fit_params,
                                   &rms, &chisq, &covariance, &major,
                                   &minor, &angle, &phys_cov);

    cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

    cpl_image_delete(image);
    image = cpl_image_new(lnx, lny, CPL_TYPE_FLOAT_COMPLEX);
    cpl_image_delete(eimage);
    eimage = cpl_image_duplicate(image);

    error = cpl_fit_image_gaussian(image, eimage, 797, 1976, 30, 30,
                                   parameters, err_params, fit_params,
                                   &rms, &chisq, &covariance, &major,
                                   &minor, &angle, &phys_cov);

    cpl_test_eq_error(error, CPL_ERROR_INVALID_TYPE);

    cpl_image_delete(image);
    image = cpl_image_new(lnx, lny, CPL_TYPE_DOUBLE_COMPLEX);
    cpl_image_delete(eimage);
    eimage = cpl_image_duplicate(image);

    error = cpl_fit_image_gaussian(image, eimage, 797, 1976, 30, 30,
                                   parameters, err_params, fit_params,
                                   &rms, &chisq, &covariance, &major,
                                   &minor, &angle, &phys_cov);

    cpl_test_eq_error(error, CPL_ERROR_INVALID_TYPE);

    cpl_image_delete(image);
    image = cpl_image_new(lnx, lny, pixeltype == CPL_TYPE_DOUBLE
                          ? CPL_TYPE_FLOAT : CPL_TYPE_DOUBLE);

    error = cpl_fit_image_gaussian(image, eimage, 797, 1976, 30, 30,
                                   parameters, err_params, fit_params,
                                   &rms, &chisq, &covariance, &major,
                                   &minor, &angle, &phys_cov);

    cpl_test_eq_error(error, CPL_ERROR_TYPE_MISMATCH);

    cpl_image_delete(image);
    cpl_image_delete(eimage);
    cpl_array_delete(parameters);
    cpl_array_delete(err_params);
    cpl_array_delete(fit_params);

}


static void eval_gauss(const double x[], const double a[], double *result)
{
    if (a[5] == 0.0) {
        if (x[0] == a[3]) {
            *result = DBL_MAX;
        }
        else {
            *result = 0.0;
        }
    }
    else if (a[6] == 0.0) {
        if (x[1] == a[4]) {
            *result = DBL_MAX;
        }
        else {
            *result = 0.0;
        }
    }
    else {
        double b1 = -0.5 / (1 - a[2] * a[2]);
        double b2 = (x[0] - a[3]) / a[5];
        double b3 = (x[1] - a[4]) / a[6];

        *result = a[0]
                + a[1] / (CPL_MATH_2PI * a[5] * a[6] * sqrt(1 - a[2] * a[2]))
                * exp(b1 * (b2 * b2 - 2 * a[2] * b2 * b3 + b3 * b3));
    }

    return;
}


static int check_gauss_success(const char  **p, 
                               const double *u, 
                               const double *e,
                               const char  **fp, 
                               const double *fu, 
                               const double *fe,
                               const cpl_array *parameters,
                               const cpl_array *err_params,
                               double        angle,
                               double        major,
                               double        minor,
                               const cpl_matrix *phys_cov,
                               FILE * stream)
{
    cpl_matrix *matrix  = NULL;
    int         success = 0;
    int         i;


    if (phys_cov == NULL) {
        matrix = cpl_matrix_new(3, 3);
        cpl_matrix_fill(matrix, 0.0);
        phys_cov = matrix;
    } else {
        /* The covariance matrix must be 3 X 3, symmetric, positive definite */
        cpl_error_code error;

        cpl_test_eq(cpl_matrix_get_nrow(phys_cov), 3);
        cpl_test_eq(cpl_matrix_get_ncol(phys_cov), 3);

        matrix = cpl_matrix_transpose_create(phys_cov);
        cpl_test_matrix_abs(phys_cov, matrix, DBL_EPSILON);

        error = cpl_matrix_decomp_chol(matrix);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        cpl_matrix_dump(phys_cov, stream);

    }

    for (i = 0; i < 7; i++) {

        /*
         * The info message is only printed if 
         * cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_INFO);
         * or if running with
         * export CPL_MSG_LEVEL=info
         */

        cpl_msg_info(cpl_func, "%s = %f +- %f", p[i],
                                cpl_array_get_double(parameters, i, NULL),
                                cpl_array_get_double(err_params, i, NULL));

        /*
         * The result must not differ from expectations more than 3 sigmas.
         * The reason of this loose test is that cpl_image_fill_noise_uniform()
         * gives different results depending on how many times it is called,
         * and other tests may be added in future.
         * It may therefore happen that a test "fails" because of an offset 
         * passing the 3-sigma limit (1% probability). A solution to this 
         * problem would be to store simulated images in the CPL repository.
         */

        success = fabs(cpl_array_get_double(parameters, i, NULL) - u[i])
                < 3.0 * e[i]; 

        if (!success) {
            cpl_msg_error(cpl_func, "Expected value for %s = %f +- %f", p[i],
                          u[i], e[i]);
            cpl_msg_error(cpl_func, "Obtained value for %s = %f +- %f", p[i],
                          cpl_array_get_double(parameters, i, NULL),
                          cpl_array_get_double(err_params, i, NULL));
            cpl_matrix_delete(matrix);
            return success;
        }
    }
    
    cpl_msg_info(cpl_func, "%s = %f +- %f (degrees)", fp[0],
                 angle * CPL_MATH_DEG_RAD, 
                 sqrt(cpl_matrix_get(phys_cov, 0, 0)) * CPL_MATH_DEG_RAD);

    success = fabs(angle * CPL_MATH_DEG_RAD - fu[0])
            < 3.0 * fe[0] * CPL_MATH_DEG_RAD;

    if (!success) {
        cpl_msg_error(cpl_func, "Expected value for %s = %f +- %f", fp[0],
                      fu[0], fe[0]);
        cpl_msg_error(cpl_func, "Obtained value for %s = %f +- %f", fp[0],
                      angle * CPL_MATH_DEG_RAD,
                      sqrt(cpl_matrix_get(phys_cov, 0, 0)) * CPL_MATH_DEG_RAD);
        cpl_matrix_delete(matrix);
        return success;
    }

    cpl_msg_info(cpl_func, "%s = %f +- %f", fp[1],
                 major, sqrt(cpl_matrix_get(phys_cov, 1, 1)));

    success = fabs(major - fu[1]) < 3.0 * fe[1];

    if (!success) {
        cpl_msg_error(cpl_func, "Expected value for %s = %f +- %f", fp[1],
                      fu[1], fe[1]);
        cpl_msg_error(cpl_func, "Obtained value for %s = %f +- %f", fp[1],
                      major,  sqrt(cpl_matrix_get(phys_cov, 1, 1)));
        cpl_matrix_delete(matrix);
        return success;
    }

    cpl_msg_info(cpl_func, "%s = %f +- %f\n", fp[2],
                 minor, sqrt(cpl_matrix_get(phys_cov, 2, 2)));

    success = fabs(minor - fu[2]) < 3.0 * fe[2];

    if (!success) {
        cpl_msg_error(cpl_func, "Expected value for %s = %f +- %f", fp[2],
                      fu[2], fe[2]);
        cpl_msg_error(cpl_func, "Obtained value for %s = %f +- %f", fp[2],
                      major,  sqrt(cpl_matrix_get(phys_cov, 2, 2)));
    }

    cpl_matrix_delete(matrix);

    return success;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief   Test cpl_fit_image_gaussian() with no returned covariance matrix
  @return  void
  @see DFS10000

 */
/*----------------------------------------------------------------------------*/
static void cpl_fit_image_gaussian_test_one(void)
{
    double         rms = 0.0;
    double         redchi = 0.0;
    cpl_image    * image;
    cpl_image    * imerr;
    cpl_array    * params;
    cpl_array    * paramserr;
    cpl_error_code error;

    params = cpl_array_new(7, CPL_TYPE_DOUBLE);
    paramserr = cpl_array_new(7, CPL_TYPE_DOUBLE);

    image = cpl_image_new(IMAGESZFIT, IMAGESZFIT, CPL_TYPE_DOUBLE);
    error = cpl_image_fill_gaussian(image, IMAGESZFIT/2, IMAGESZFIT/2,
                                    20.0, 20.0, 20.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    imerr = cpl_image_new(IMAGESZFIT, IMAGESZFIT, CPL_TYPE_DOUBLE);

    error = cpl_image_fill_noise_uniform(imerr,1.0,1.00001);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_fit_image_gaussian(image, imerr, IMAGESZFIT/2, IMAGESZFIT/2,
                                   IMAGESZFIT/5, IMAGESZFIT/5,
                                   params, paramserr, NULL, &rms, NULL,
                                   NULL, NULL, NULL, NULL, NULL);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_fit_image_gaussian(image, imerr, IMAGESZFIT/2, IMAGESZFIT/2,
                                   IMAGESZFIT/5, IMAGESZFIT/5,
                                   params, paramserr, NULL, &rms, &redchi,
                                   NULL, NULL, NULL, NULL, NULL);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_image_delete(image);
    cpl_image_delete(imerr);
    cpl_array_delete(params);
    cpl_array_delete(paramserr);

}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief   Test the function
  @see cpl_fit_imagelist_polynomial_window

 */
/*----------------------------------------------------------------------------*/
static
cpl_imagelist * 
cpl_fit_imagelist_polynomial_window_test(const cpl_vector    * x_pos,
                                         const cpl_imagelist * values,
                                         int                   llx,
                                         int                   lly,
                                         int                   urx,
                                         int                   ury,
                                         int                   mindeg,
                                         int                   maxdeg,
                                         cpl_boolean           is_symsamp,
                                         cpl_type              pixeltype,
                                         cpl_image           * fiterror)
{

    cpl_imagelist * self = 
        cpl_fit_imagelist_polynomial_window(x_pos, values, llx, lly, urx, ury,
                                            mindeg, maxdeg, is_symsamp,
                                            pixeltype, fiterror);
    if (self != NULL) {
        const cpl_size np = cpl_vector_get_size(x_pos);
        cpl_imagelist * icopy = cpl_imagelist_duplicate(values);
        cpl_vector    * vcopy = cpl_vector_new(np + 1);
        cpl_image * bad = cpl_image_duplicate(cpl_imagelist_get_const(icopy, 0));
        const cpl_size mx = cpl_image_get_size_x(bad);
        const cpl_size my = cpl_image_get_size_y(bad);
        cpl_mask * mask = cpl_mask_new(mx, my);
        cpl_imagelist * fitbad;
        cpl_image * baderror = fiterror ? cpl_image_duplicate(fiterror) : NULL;
        const double tol = pixeltype == CPL_TYPE_INT ? 0.0
            : (pixeltype == CPL_TYPE_FLOAT
#if defined SIZEOF_SIZE_T && SIZEOF_SIZE_T == 4
               /* FIXME: Extreme loss of precision on (some) 32-bit machines */
               ? 0.004 : 30.0 * FLT_EPSILON
#else
               ? FLT_EPSILON : 800.0 * DBL_EPSILON
#endif
               );
        cpl_error_code error;

        cpl_msg_info(cpl_func, "Testing with %" CPL_SIZE_FORMAT " X %"
                     CPL_SIZE_FORMAT " bad pixels", mx, my);

        cpl_test_eq_ptr(memcpy(cpl_vector_get_data(vcopy),
                               cpl_vector_get_data_const(x_pos),
                               (size_t)np * sizeof(double)),
                        cpl_vector_get_data_const(vcopy));

        /* The value of this sampling point is not used */
        error = cpl_vector_set(vcopy, np, DBL_MAX);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_imagelist_set(icopy, bad, np);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_mask_not(mask);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_image_reject_from_mask(bad, mask);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        cpl_test_eq(cpl_image_count_rejected(bad), mx * my);

        cpl_test_eq(cpl_imagelist_get_size(icopy), np + 1);
        cpl_test_eq(cpl_vector_get_size(vcopy), np + 1);

        fitbad = cpl_fit_imagelist_polynomial_window(vcopy, icopy, llx, lly,
                                                     urx, ury, mindeg, maxdeg,
                                                     is_symsamp, pixeltype,
                                                     baderror);
        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_nonnull(fitbad);
        cpl_test_imagelist_abs(self, fitbad, tol);
        if (fiterror != NULL) {
            cpl_test_image_abs(fiterror, baderror, tol);
        }

        cpl_imagelist_delete(icopy);
        cpl_imagelist_delete(fitbad);
        cpl_image_delete(baderror);
        cpl_vector_delete(vcopy);
        cpl_mask_delete(mask);

    }

    return self;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief   Test the function
  @see cpl_fit_imagelist_polynomial

 */
/*----------------------------------------------------------------------------*/
static void cpl_fit_imagelist_polynomial_bpm(void)
{


    const double     ditval[]    = {0.0, 1.0, 2.0, 3.0, 4.0, 
                                    5.0, 6.0, 7.0, 8.0};
    const int        ndits = (int)(sizeof(ditval)/sizeof(double));
    CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual)
    cpl_vector     * vdit  = cpl_vector_wrap(ndits, (double*)ditval);
    CPL_DIAG_PRAGMA_POP

    const int mx = 2; /* Two pixels: one all good, one mixed */
    const int my = 1;

    cpl_image      * dfiterror = cpl_image_new(mx, my, CPL_TYPE_DOUBLE);
    cpl_imagelist * imlist = cpl_imagelist_new();
    cpl_imagelist * fit;
    int is_bad = 0;

    int i;

    /* Test 1: A perfectly linear fit */

    for (i = 0; i < ndits; i++) {
        const double value = ditval[i]; 
        cpl_image * bad = cpl_image_new(mx, my, CPL_TYPE_DOUBLE);
        cpl_image_set(bad, 1, 1, value);
        cpl_image_set(bad, 2, 1, value);
        cpl_imagelist_set(imlist, bad, i);
        if (i & 1) {
            const cpl_error_code error = cpl_image_reject(bad, 1, 1);
            cpl_test_eq_error(error, CPL_ERROR_NONE);
        }
    }

    fit = cpl_fit_imagelist_polynomial(vdit, imlist, 0, 1, CPL_FALSE,
                                       CPL_TYPE_DOUBLE, dfiterror);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq(cpl_imagelist_get_size(fit), 2);

    cpl_test_abs(cpl_image_get(cpl_imagelist_get(fit, 0), 1, 1, &is_bad),
                 0.0, 4.0 * DBL_EPSILON);
    cpl_test_zero(is_bad);
    cpl_test_rel(cpl_image_get(cpl_imagelist_get(fit, 1), 1, 1, &is_bad),
                 1.0, 2.0 * DBL_EPSILON);
    cpl_test_zero(is_bad);

    cpl_test_abs(cpl_image_get(dfiterror, 1, 1, &is_bad), 0.0, DBL_EPSILON);
    cpl_test_zero(is_bad);


    /* Test 2: A perfectly quadratic fit */

    for (i = 0; i < ndits; i++) {
        const double value = ditval[i] * ditval[i];
        cpl_image * bad = cpl_imagelist_get(imlist, i);
        cpl_image_set(bad, 1, 1, value);
        cpl_image_set(bad, 2, 1, value);
        if (i & 1) {
            const cpl_error_code error = cpl_image_reject(bad, 1, 1);
            cpl_test_eq_error(error, CPL_ERROR_NONE);
        }
    }

    cpl_imagelist_delete(fit);
    fit = cpl_fit_imagelist_polynomial(vdit, imlist, 0, 2, CPL_FALSE,
                                       CPL_TYPE_DOUBLE, dfiterror);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq(cpl_imagelist_get_size(fit), 3);

    cpl_test_abs(cpl_image_get(cpl_imagelist_get(fit, 0), 1, 1, &is_bad),
                 0.0, 12.0 * DBL_EPSILON);
    cpl_test_zero(is_bad);
    cpl_test_abs(cpl_image_get(cpl_imagelist_get(fit, 1), 1, 1, &is_bad),
                 0.0, 24.0 * DBL_EPSILON);
    cpl_test_zero(is_bad);
    cpl_test_rel(cpl_image_get(cpl_imagelist_get(fit, 2), 1, 1, &is_bad),
                 1.0, 2.0 * DBL_EPSILON);
    cpl_test_zero(is_bad);

    cpl_test_abs(cpl_image_get(dfiterror, 1, 1, &is_bad), 0.0, DBL_EPSILON);
    cpl_test_zero(is_bad);

    /* Test 3: A perfectly quadratic fit, without the constant+linear terms */

    for (i = 0; i < ndits; i++) {
        const double value = ditval[i] * ditval[i];
        cpl_image * bad = cpl_imagelist_get(imlist, i);
        cpl_image_set(bad, 1, 1, value);
        cpl_image_set(bad, 2, 1, value);
        if (i & 1) {
            const cpl_error_code error = cpl_image_reject(bad, 1, 1);
            cpl_test_eq_error(error, CPL_ERROR_NONE);
        }
    }

    cpl_imagelist_delete(fit);
    fit = cpl_fit_imagelist_polynomial(vdit, imlist, 2, 2, CPL_FALSE,
                                       CPL_TYPE_DOUBLE, dfiterror);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq(cpl_imagelist_get_size(fit), 1);

    cpl_test_rel(cpl_image_get(cpl_imagelist_get(fit, 0), 1, 1, &is_bad),
                 1.0, 2.0 * DBL_EPSILON);
    cpl_test_zero(is_bad);

    cpl_test_abs(cpl_image_get(dfiterror, 1, 1, &is_bad), 0.0, DBL_EPSILON);
    cpl_test_zero(is_bad);

    cpl_test_eq_ptr(cpl_vector_unwrap(vdit), ditval);
    cpl_imagelist_delete(imlist);
    cpl_imagelist_delete(fit);
    cpl_image_delete(dfiterror);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief   Test cpl_fit_image_gaussian() using local data
  @return  void
  @see PIPE-8125

 */
/*----------------------------------------------------------------------------*/
static void cpl_fit_image_gaussian_test_bpm(void)
{
    cpl_array * params = cpl_array_new(7, CPL_TYPE_DOUBLE);
    const double values[IM13 * IM13]
        = {856.781, 832.831, 876.031, 894.781, 1015.83, 1160.33, 1276.03, 1295.73, 1274.58, 1173.18, 1042.88, 970.481, 912.681,
           894.575, 919.775, 925.975, 984.075, 1109.33, 1233.38, 1291.92, 1312.77, 1198.62, 1120.77, 1038.83, 1016.58, 975.075,
           980.519, 980.419, 999.319, 1021.72, 1255.82, 1536.47, 1735.57, 1686.22, 1493.17, 1196.32, 1071.27, 1110.17, 1076.17,
           1076.92, 1076.47, 1052.22, 1192.12, 1647.82, 2262.62, 2979.22, 3011.62, 2484.32, 1769.47, 1389.27, 1211.72, 1159.97,
           1164.42, 1188.47, 1198.07, 1537.72, 2522.32, 4171.52, 5487.42, 5950.92, 4712.87, 3176.17, 1998.07, 1494.47, 1321.77,
           1320.96, 1352.11, 1480.96, 2152.26, 3932.41, 6727.36, 9276.11, 9706.51, 7825.66, 5090.06, 2894.41, 1878.91, 1498.41,
           1411.61, 1496.16, 1751.36, 2709.61, 5201.41, 8742.31, 12128.6, 12650.6, -4.0375, 6567.81, 3663.51, 2145.31, 1623.26,
           1411.12, 1544.52, 1822.82, 2780.97, 5355.67, 8861.92, 12061, 12846.3, 10185.7, 6598.87, 3677.82, 2073.72, 1517.12,
           1377.82, 1478.97, 1764.62, 2456.02, 4256.12, 7063.37, 9463.02, 9937.97, 8086.52, 4928.32, 2861.67, 1698.72, 1353.57,
           1219.54, 1313.94, 1447.39, 1844.84, 2740.39, 4471.04, 5562.24, 6004.69, 4866.64, 3301.69, 1990.74, 1307.34, 1171.99,
           1057.14, 1130.09, 1146.79, 1242.09, 1617.19, 2364.34, 2982.64, 3218.14, 2812.74, 2019.04, 1492.14, 1201.34, 1051.94,
           910.269, 929.319, 988.369, 1011.37, 1107.87, 1442.02, 1782.92, 1927.97, 1893.52, 1681.77, 1432.52, 1192.67, 1014.22,
           889.488, 846.738, 876.587, 917.688, 802.588, 1168.19, 1421.54, 1701.69, 1707.54, 1562.19, 1312.04, 1156.39, 999.287};

    const size_t image_bytes = IM13 * IM13 * sizeof(double);
    double     * image_data  = (double*)cpl_malloc(image_bytes);
    cpl_image  * image       = cpl_image_wrap_double(IM13, IM13, image_data);
    cpl_image  * errim;

    const double xfrac = 0.115;
    const double yfrac = 0.511;

    cpl_test_nonnull(image);

    cpl_test_eq_ptr(memcpy(image_data, values, image_bytes), image_data);

    errim = cpl_image_abs_create(image);
    cpl_image_power(errim, 0.5);
    cpl_image_reject_value(errim, CPL_VALUE_ZERO);

    cpl_array_set(params, 3, 1.0 + (IM13 - 1) * 0.5 + xfrac);
    cpl_array_set(params, 4, 1.0 + (IM13 - 1) * 0.5 + yfrac);

    for (int idoerrim = 0; idoerrim < 2; idoerrim++) {
        const cpl_image * use_errim = idoerrim ? errim : NULL;

        cpl_msg_info(cpl_func, idoerrim ? "Testing with error image" :
                     "Testing without error image");

        for (int idobpm = 0; idobpm < 2; idobpm++) {
            double rms0, major0, minor0, angle0;
            double rms1 = 0.0, major1 = 0.0, minor1 = 0.0, angle1 = 0.0;
            double drms = 0.0, dmajor = 0.0, dminor = 0.0, dangle = 0.0;
            cpl_error_code code;

            cpl_msg_info(cpl_func, idobpm ? "Testing with bpm" :
                         "Testing without bpm");

            if (idobpm) {
                cpl_size nbad;
                cpl_mask * nullmask;
                cpl_mask * bpm = cpl_mask_threshold_image_create(image,
                                                                 -FLT_MAX, 0.0);

                cpl_test_error(CPL_ERROR_NONE);

                nullmask = cpl_image_set_bpm(image, bpm);
                cpl_test_error(CPL_ERROR_NONE);

                cpl_test_null(nullmask);

                nbad = cpl_image_count_rejected(image);
                cpl_test_error(CPL_ERROR_NONE);
                cpl_test_eq(nbad, 1);
            }

            for (int iretry = 0; iretry < 2; iretry++) {
                cpl_matrix    * phys_cov  = NULL;
                cpl_matrix    **pphys_cov = use_errim ? &phys_cov : NULL;

                cpl_msg_info(cpl_func, iretry ? "Testing with first guess" :
                             "Testing without first guess");

                rms0 = rms1;
                major0 = major1;
                minor0 = minor1;
                angle0 = angle1;

                code = cpl_fit_image_gaussian(image, use_errim, IM13/2, IM13/2,
                                              IM13, IM13, params, NULL, NULL,
                                              &rms1, NULL, NULL, &major1,
                                              &minor1, &angle1, pphys_cov);

                if (code == CPL_ERROR_SINGULAR_MATRIX) {
                    cpl_test(idoerrim);
                    cpl_test_eq_error(code, CPL_ERROR_SINGULAR_MATRIX);
                    cpl_test_null(phys_cov);

                    /* Retry without covariance matrix */
                    code = cpl_array_fill_window_invalid(params, 0, 7);
                    cpl_test_eq_error(code, CPL_ERROR_NONE);
                    cpl_array_set(params, 3, 1.0 + (IM13 - 1) * 0.5 + xfrac);
                    cpl_array_set(params, 4, 1.0 + (IM13 - 1) * 0.5 + yfrac);

                    code = cpl_fit_image_gaussian(image, use_errim, IM13/2,
                                                  IM13/2, IM13, IM13, params,
                                                  NULL, NULL, &rms1, NULL, NULL,
                                                  &major1, &minor1, &angle1,
                                                  NULL);

                    cpl_test_eq_error(code, CPL_ERROR_NONE);
                } else if (idoerrim) {
                    cpl_test_eq_error(code, CPL_ERROR_NONE);
                    cpl_test_nonnull(phys_cov);
                    cpl_matrix_delete(phys_cov);
                } else {
                    cpl_test_eq_error(code, CPL_ERROR_NONE);
                    cpl_test_null(phys_cov);

                    code = cpl_fit_image_gaussian(image, NULL, IM13/2, IM13/2,
                                                  IM13, IM13, params, NULL,
                                                  NULL, &rms1, NULL, NULL,
                                                  &major1, &minor1, &angle1,
                                                  &phys_cov);
                    cpl_test_eq_error(code, CPL_ERROR_DATA_NOT_FOUND);
                }
            }

            cpl_msg_info(cpl_func, CPL_STRINGIFY(IM13) " X " CPL_STRINGIFY(IM13)
                         " double-image: Major: %g - %g = %g. Minor: %g - %g "
                         "= %g. Angle: %g - %g = %g. RMS=%g - %g = %g",
                         major1, major0, major1 - major0,
                         minor1, minor0, minor1 - minor0,
                         angle1, angle0, angle1 - angle0,
                         rms1,   rms0,   rms1 - rms0);

            if (idobpm) {
                /* With bpm, the change is less or equal. FIXME: Why ? */
                cpl_test_leq(fabs(dmajor), fabs(major1 - major0));
                cpl_test_leq(fabs(dminor), fabs(minor1 - minor0));
                cpl_test_leq(fabs(dangle), fabs(angle1 - angle0));
                cpl_test_leq(fabs(drms),   fabs(rms1   - rms0));
            }

            code = cpl_image_accept_all(image);
            cpl_test_eq_error(code, CPL_ERROR_NONE);

            dmajor = major1 - major0;
            dminor = minor1 - minor0;
            dangle = angle1 - angle0;
            drms   = rms1   - rms0;
        }
    }

    cpl_array_delete(params);
    cpl_image_delete(image);
    cpl_image_delete(errim);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief   Test cpl_fit_image_gaussian() using local data
  @return  void
  @see DFS11865

 */
/*----------------------------------------------------------------------------*/
static void cpl_fit_image_gaussian_test_local(void)
{

    cpl_array * params = cpl_array_new(7, CPL_TYPE_DOUBLE);

    const int values[][IMSZ * IMSZ]
        = {{111, 477, 1596, 1263, 194, 64, 255, 970, 3210, 2640, 469, 141, 1565,
            8840, 30095, 17625, 2381, 656, 4982, 33104, 65535, 63588, 7276,
            2377, 1822, 12167, 37548, 20715, 2597, 856, 353, 1440, 4042, 3121,
            535, 135},
           {408, 880, 1043, 719, 249, 93, 1512, 2730, 2253, 1159, 470, 212,
            11366, 22069, 18434, 7102, 1898, 789, 15630, 52992, 65535, 43330,
            11619, 2834, 3422, 13283, 31354, 32311, 13946, 3225, 616, 1717,
            3143, 3298, 2029, 662},
           {161, 587, 1618, 1487, 409, 103, 475, 1681, 4511, 4295, 1302, 308,
            2009, 10686, 40660, 42502, 10990, 1559, 4526, 26557, 65535, 65535,
            16232, 2942, 1136, 6054, 17506, 13985, 3569, 697, 249, 1057, 2944,
            2536, 633, 115},
           {684, 1620, 2240, 2021, 1041, 346, 3549, 8272, 11679, 9524, 4789,
            1661, 18252, 45616, 65535, 57247, 28755, 9689, 7025, 17892, 26878,
            23738, 12277, 4084, 905, 2057, 3227, 2941, 1546, 586, 275, 651, 998,
            978, 581, 218},
           {419, 964, 1470, 1165, 428, 118, 2547, 7622, 7975, 3391, 979, 335,
            5931, 29549, 51281, 22243, 3797, 1143, 4522, 21513, 65535, 62954,
            13954, 2253, 1293, 4305, 15660, 26446, 12002, 1870, 243, 849,  2525,
           3565, 2228, 491},
           {207, 747, 2302, 3383, 2176, 762, 867, 2512, 10026, 22327, 16365,
            3663, 2689, 11998, 55855, 65535, 27336, 4542, 3587, 20606, 52265,
            33975, 7420, 1594, 1275, 5343, 7709, 4121, 1060, 319, 210, 744,
            1303, 1024, 391, 87}
    };

    const double major[] = {5.80423, 1.25846, 0.9219, 1.35805, 1.08065,
                            1.09118};
    const double minor[] = {2.82963, 0.661684, 0.714119, 0.601602, 0.661207,
                            0.679018};
    const double angle[] = {0.865826, 0.318316, -0.13699, 0.0127396, 0.613033,
                            -0.556366};

    const size_t n = sizeof(values)/sizeof(values[0]);

    const size_t image_bytes = IMSZ * IMSZ * sizeof(int);
    int        * image_data  = (int*)cpl_malloc(image_bytes);
    cpl_image  * image       = cpl_image_wrap_int(IMSZ, IMSZ, image_data);

    cpl_test_nonnull(image);

    cpl_test_assert(sizeof(major)/sizeof(major[0]) == n);
    cpl_test_assert(sizeof(minor)/sizeof(minor[0]) == n);
    cpl_test_assert(sizeof(angle)/sizeof(angle[0]) == n);

    for (size_t i = 0; i < n; i++) {
        const double tol = 1e-5;
        const cpl_size nx = IMSZ;
        const cpl_size ny = IMSZ;
        double mymajor, myminor, myangle, rms;
        cpl_error_code error;

        cpl_test_eq_ptr(memcpy(image_data, values[i], image_bytes), image_data);

        error = cpl_array_fill_window_invalid(params, 1, 7);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_fit_image_gaussian(image, NULL, nx/2, ny/2, nx, ny,
                                       params, NULL, NULL, &rms, NULL, NULL,
                                       &mymajor, &myminor, &myangle, NULL);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        cpl_msg_info(cpl_func, CPL_STRINGIFY(IMSZ) " X " CPL_STRINGIFY(IMSZ)
                     " int-image %d/%d: Major: %g - %g = %g. Minor: %g - %g "
                     "= %g. Angle: %g - %g = %g. RMS=%g", (int)i + 1, (int)n,
                     mymajor, major[i], mymajor - major[i],
                     myminor, minor[i], myminor - minor[i],
                     myangle, angle[i], myangle - angle[i], rms);

        cpl_test_abs(mymajor, major[i], tol);
        cpl_test_abs(myminor, minor[i], tol);
        cpl_test_abs(myangle, angle[i], tol);

        if (i == 0) continue; /* Converges to a non-global minimum... */

        /* Re-try with the solution as 1st guess */
        error = cpl_fit_image_gaussian(image, NULL, nx/2, ny/2, nx, ny,
                                       params, NULL, NULL, &rms, NULL, NULL,
                                       &mymajor, &myminor, &myangle, NULL);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        cpl_msg_info(cpl_func, CPL_STRINGIFY(IMSZ) " X " CPL_STRINGIFY(IMSZ)
                     " int-image %d/%d: Major: %g - %g = %g. Minor: %g - %g "
                     "= %g. Angle: %g - %g = %g. RMS=%g (retry)",
                     (int)i + 1, (int)n,
                     mymajor, major[i], mymajor - major[i],
                     myminor, minor[i], myminor - minor[i],
                     myangle, angle[i], myangle - angle[i], rms);

        cpl_test_abs(mymajor, major[i], tol);
        cpl_test_abs(myminor, minor[i], tol);
        cpl_test_abs(myangle, angle[i], tol);

    }

    cpl_array_delete(params);
    cpl_image_delete(image);
}
