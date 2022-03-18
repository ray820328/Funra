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
#include <math.h>
#include <float.h>

#include "cpl_stats.h"
#include "cpl_image_stats.h"
#include "cpl_image_gen.h"
#include "cpl_image_io.h"
#include "cpl_image_bpm.h"
#include "cpl_memory.h"
#include "cpl_test.h"
#include "cpl_math_const.h"

/*-----------------------------------------------------------------------------
                                   defines
 -----------------------------------------------------------------------------*/

#ifndef IMAGE_SIZE_X
#define IMAGE_SIZE_X 256
#endif
#ifndef IMAGE_SIZE_Y
#define IMAGE_SIZE_Y IMAGE_SIZE_X
#endif

#if defined SIZEOF_SIZE_T && SIZEOF_SIZE_T == 4
#define TOL (800.0 * FLT_EPSILON)
#else
#define TOL (16.0 * DBL_EPSILON)
#endif

#define CPL_STAT_CMP(OP, FUNC)                                          \
    do {                                                                \
                                                                        \
        cpl_stats * statone;                                            \
                                                                        \
        cpl_test_nonnull( statone = cpl_stats_new_from_image            \
                          (img, CPL_CONCAT2X(CPL_STATS,OP)));           \
                                                                        \
        /* Compare result from one-stats object with all-stats object */ \
        cpl_test_abs( CPL_CONCAT2X(cpl_stats_get, FUNC)(statall),       \
                      CPL_CONCAT2X(cpl_stats_get, FUNC)(statone),       \
                      TOL);                                             \
                                                                        \
        cpl_stats_delete(statone);                                      \
                                                                        \
    } while (0)    


#define CPL_STAT_CMP_IMAGE(OP)                                          \
    do {                                                                \
                                                                        \
        /* Test NULL input */                                           \
        (void)CPL_CONCAT2X(cpl_stats_get, OP)(NULL);                    \
        cpl_test_error(CPL_ERROR_NULL_INPUT);                           \
                                                                        \
        (void)CPL_CONCAT2X(cpl_image_get, OP)(NULL);                    \
        cpl_test_error(CPL_ERROR_NULL_INPUT);                           \
                                                                        \
        (void)CPL_CONCAT2X(cpl_image_get, CPL_CONCAT2X(OP,window))      \
            (NULL, 1, 1, IMAGE_SIZE_X, IMAGE_SIZE_Y);                   \
            cpl_test_error(CPL_ERROR_NULL_INPUT);                       \
                                                                        \
        /* Test out-of-range windows parameters */                      \
        (void)CPL_CONCAT2X(cpl_image_get, CPL_CONCAT2X(OP,window))      \
            (img, 0, 1, IMAGE_SIZE_X, IMAGE_SIZE_Y);                    \
            cpl_test_error(CPL_ERROR_ACCESS_OUT_OF_RANGE);              \
                                                                        \
        (void)CPL_CONCAT2X(cpl_image_get, CPL_CONCAT2X(OP,window))      \
            (img, 1, 0, IMAGE_SIZE_X, IMAGE_SIZE_Y);                    \
            cpl_test_error(CPL_ERROR_ACCESS_OUT_OF_RANGE);              \
                                                                        \
        (void)CPL_CONCAT2X(cpl_image_get, CPL_CONCAT2X(OP,window))      \
            (img, 1, 1, IMAGE_SIZE_X+1, IMAGE_SIZE_Y);                  \
            cpl_test_error(CPL_ERROR_ACCESS_OUT_OF_RANGE);              \
                                                                        \
        (void)CPL_CONCAT2X(cpl_image_get, CPL_CONCAT2X(OP,window))      \
            (img, 1, 1, IMAGE_SIZE_X, IMAGE_SIZE_Y+1);                  \
            cpl_test_error(CPL_ERROR_ACCESS_OUT_OF_RANGE);              \
                                                                        \
        /* Compare result from stats object with image window accessor */ \
        cpl_test_abs( CPL_CONCAT2X(cpl_stats_get, OP)(statall),         \
                      CPL_CONCAT2X(cpl_image_get, CPL_CONCAT2X(OP,window)) \
                          (img, 1, 1, IMAGE_SIZE_X, IMAGE_SIZE_Y),      \
                      TOL);                                             \
                                                                        \
        /* Compare result from stats object with image accessor */      \
        cpl_test_abs( CPL_CONCAT2X(cpl_stats_get, OP)(statall),         \
                      CPL_CONCAT2X(cpl_image_get, OP)(img),             \
                      TOL);                                             \
    } while (0)    


#define CPL_STAT_CMP_VECTOR(OP, TOL)                                    \
    do {                                                                \
        cpl_vector * vtest                                              \
            = cpl_vector_wrap(IMAGE_SIZE_X*IMAGE_SIZE_Y,                \
                              cpl_image_get_data_double(img));          \
        cpl_test_nonnull(vtest);                                        \
        cpl_test_abs(CPL_CONCAT2X(cpl_stats_get, OP)(statall),          \
                     CPL_CONCAT2X(cpl_vector_get, OP)(vtest),           \
                     TOL* DBL_EPSILON);                                 \
        cpl_test_nonnull(cpl_vector_unwrap(vtest));                     \
    } while (0)    

/*-----------------------------------------------------------------------------
                        Private function prototypes
 -----------------------------------------------------------------------------*/

static void cpl_stats_new_all_bench(const cpl_image *, int);
static void cpl_stats_new_std_bench(const cpl_image *, int);
static void cpl_stats_new_median_bench(const cpl_image *, int);

/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/
int main(void)
{
    const cpl_type img_types[] = {CPL_TYPE_DOUBLE, CPL_TYPE_FLOAT, CPL_TYPE_INT};
    const cpl_stats_mode istats[] = {CPL_STATS_MIN,  CPL_STATS_MAX,
                                     CPL_STATS_MEDIAN, CPL_STATS_MEDIAN_DEV,
                                     CPL_STATS_MAD,
                                     CPL_STATS_STDEV, CPL_STATS_FLUX,
                                     CPL_STATS_ABSFLUX, CPL_STATS_SQFLUX,
                                     CPL_STATS_MINPOS, CPL_STATS_MAXPOS,
                                     CPL_STATS_CENTROID, CPL_STATS_MEAN};
    FILE          * stream;
    unsigned        itype;
    cpl_boolean     do_bench;
    cpl_mask      * ones;
    cpl_error_code  error;


    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    do_bench = cpl_msg_get_level() <= CPL_MSG_INFO ? CPL_TRUE : CPL_FALSE;

    stream = cpl_msg_get_level() > CPL_MSG_INFO
        ? fopen("/dev/null", "a") : stdout;

    /* Test 1: NULL image */
    cpl_test_null( cpl_stats_new_from_image(NULL, CPL_STATS_ALL) );
    cpl_test_error(CPL_ERROR_NULL_INPUT);

    error = cpl_stats_dump(NULL, CPL_STATS_ALL, stream);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

    ones = cpl_mask_new(IMAGE_SIZE_X, IMAGE_SIZE_Y);
    cpl_test_nonnull(ones);
    error = cpl_mask_not(ones);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    for (itype = 0; itype < sizeof(img_types)/sizeof(img_types[0]); itype++) {
        const cpl_type img_type = img_types[itype];
        cpl_image * img = cpl_image_new(IMAGE_SIZE_X, IMAGE_SIZE_Y, img_type);
        cpl_stats * statall;
        cpl_stats * nullstat;
        unsigned    istat;
        double      med1, med2;
        double      sig1, sig2;

        cpl_msg_info(cpl_func, "Testing %d X %d-image (type %u)",
                     IMAGE_SIZE_X, IMAGE_SIZE_Y, (unsigned)img_type);

        cpl_test_nonnull(img);

        /* Test 2: Illegal stat-ops */
        nullstat = cpl_stats_new_from_image(img, 0); /* Binary and */
        cpl_test_error(CPL_ERROR_UNSUPPORTED_MODE);
        cpl_test_null(nullstat);

        nullstat = cpl_stats_new_from_image(img, 1); /* Logical or */
        cpl_test_error(CPL_ERROR_INVALID_TYPE);
        cpl_test_null(nullstat);

        nullstat = cpl_stats_new_from_image(img, ~CPL_STATS_ALL);
        cpl_test_error(CPL_ERROR_UNSUPPORTED_MODE);
        cpl_test_null(nullstat);

        nullstat = cpl_stats_new_from_image_window(img, CPL_STATS_ALL, 2, 2,
                                                   1, 1);
        cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
        cpl_test_null(nullstat);

        nullstat = cpl_stats_new_from_image_window(img, CPL_STATS_ALL, 1, 1,
                                                   IMAGE_SIZE_X,
                                                   IMAGE_SIZE_Y+1);
        cpl_test_error(CPL_ERROR_ACCESS_OUT_OF_RANGE);
        cpl_test_null(nullstat);

        nullstat = cpl_stats_new_from_image_window(img, CPL_STATS_ALL, 0, 0,
                                                   IMAGE_SIZE_X,
                                                   IMAGE_SIZE_Y);
        cpl_test_error(CPL_ERROR_ACCESS_OUT_OF_RANGE);
        cpl_test_null(nullstat);

        error = cpl_image_fill_noise_uniform(img, (double)-IMAGE_SIZE_Y,
                                             (double)IMAGE_SIZE_X);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        statall = cpl_stats_new_from_image(img, CPL_STATS_ALL);
        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_nonnull(statall);

        /* Test 3: NULL stream */
        error = cpl_stats_dump(statall, CPL_STATS_ALL, NULL);
        cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

        /* Test 4: Illegal stat-ops on valid stat object */
        error = cpl_stats_dump(statall, ~CPL_STATS_ALL, stream);
        cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

        error = cpl_stats_dump(statall, 0, stream); /* This is allowed */
        cpl_test_eq_error(error, CPL_ERROR_NONE);
        error = cpl_stats_dump(statall, 1, stream); /* Result of logical or */
        cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

        /* Test 5: Dump all */
        cpl_test_zero( cpl_stats_dump(statall, CPL_STATS_ALL, stream) );

        for (istat = 0; istat < sizeof(istats)/sizeof(istats[0]); istat++) {
            cpl_stats * statone;

            cpl_test_nonnull( statone
                              = cpl_stats_new_from_image(img, istats[istat]));

            /* Test 6: Illegal dump on valid object */
            error = cpl_stats_dump(statone, CPL_STATS_ALL | (~istats[istat]),
                                   stream);
            cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

            /* Test 7: Valid dump on valid object */
            error = cpl_stats_dump(statone, istats[istat], stream);
            cpl_test_eq_error(error, CPL_ERROR_NONE);

            cpl_stats_delete(statone);
        }

        /* Test 7a: 
           Compare result from one-stats object with all-stats object */

        CPL_STAT_CMP(MIN, min);
        CPL_STAT_CMP(MAX, max);
        CPL_STAT_CMP(MEAN, mean);
        CPL_STAT_CMP(MEDIAN, median);
        CPL_STAT_CMP(MEDIAN_DEV, median_dev);
        CPL_STAT_CMP(MAD, mad);
        CPL_STAT_CMP(STDEV, stdev);
        CPL_STAT_CMP(FLUX, flux);
        CPL_STAT_CMP(ABSFLUX, absflux);
        CPL_STAT_CMP(SQFLUX, sqflux);
        CPL_STAT_CMP(CENTROID, centroid_x);
        CPL_STAT_CMP(CENTROID, centroid_y);
        CPL_STAT_CMP(MINPOS, min_y);
        CPL_STAT_CMP(MINPOS, min_x);
        CPL_STAT_CMP(MAXPOS, max_y);
        CPL_STAT_CMP(MAXPOS, max_x);

        /* Test 8: Verify that CPL_STAT_ALL yields equal result */
        CPL_STAT_CMP_IMAGE(min);
        CPL_STAT_CMP_IMAGE(max);
        CPL_STAT_CMP_IMAGE(mean);
        CPL_STAT_CMP_IMAGE(median);
        CPL_STAT_CMP_IMAGE(stdev);
        CPL_STAT_CMP_IMAGE(flux);
        CPL_STAT_CMP_IMAGE(absflux);
        CPL_STAT_CMP_IMAGE(sqflux);
        CPL_STAT_CMP_IMAGE(centroid_x);
        CPL_STAT_CMP_IMAGE(centroid_y);

        cpl_test_abs( cpl_stats_get_median(statall),
                      cpl_image_get_mad_window(img, 1, 1, IMAGE_SIZE_X,
                                               IMAGE_SIZE_Y, &sig1),
                      0.0);

        cpl_test_abs( cpl_stats_get_median(statall),
                      cpl_image_get_mad(img, &sig2), 0.0);

        cpl_test_abs( sig1, sig2, 0.0);

        /* Pixel values are not Gaussian, so the MAD cannot be used to estimate
           the standard deviation. */
        cpl_msg_info(cpl_func, "MAD as std.dev. estimator: MAD * 1.4826 = %g "
                     "<=> %g = st.dev.", sig1 * CPL_MATH_STD_MAD,
                     cpl_stats_get_stdev(statall));

        cpl_test_abs( cpl_stats_get_median(statall),
                      cpl_image_get_median_dev_window(img, 1, 1, IMAGE_SIZE_X,
                                                      IMAGE_SIZE_Y, &sig1),
                      0.0);

        cpl_test_abs( cpl_stats_get_median(statall),
                      cpl_image_get_median_dev(img, &sig2), 0.0);

        cpl_test_abs( sig1, sig2, 0.0);

        if (img_type == CPL_TYPE_DOUBLE) {
            /* FIXME: Tolerance needed for mean and stdev */
            CPL_STAT_CMP_VECTOR(min, 0.0);
            CPL_STAT_CMP_VECTOR(max, 0.0);
            CPL_STAT_CMP_VECTOR(mean, 16.0);
            CPL_STAT_CMP_VECTOR(stdev, 16.0 * IMAGE_SIZE_X);
        }

        cpl_stats_delete(statall);

        med2 = cpl_image_get_median(img);

        /* Test cpl_image_get_median_dev() */
        med1 = cpl_image_get_median_dev(img, &sig1);
        cpl_test_abs( med1, med2, 0.0 );

        cpl_test_leq( 0.0, sig1 );
    
        if (do_bench) {
            cpl_stats_new_all_bench(img, 500);
            cpl_stats_new_std_bench(img, 500);
            cpl_stats_new_median_bench(img, 100);
        } else {
            cpl_stats_new_all_bench(img, 1);
            cpl_stats_new_std_bench(img, 1);
            cpl_stats_new_median_bench(img, 1);
        }

        /* Check error handling on all-rejected image */
        cpl_test_zero(cpl_image_reject_from_mask(img, ones));

        (void)cpl_image_get_median_dev(img, &sig1);
        cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);

        (void)cpl_image_get_median_dev(img, NULL);
        cpl_test_error(CPL_ERROR_NULL_INPUT);

        cpl_image_delete(img);

        (void)cpl_image_get_median_dev(NULL, &sig1);
        cpl_test_error(CPL_ERROR_NULL_INPUT);

    }

    cpl_mask_delete(ones);

    if (stream != stdout) cpl_test_zero( fclose(stream) );

    return cpl_test_end(0);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Benchmark the CPL function
  @param image The image to test on
  @param n     The number of repeats
  @return   void
 */
/*----------------------------------------------------------------------------*/
static void cpl_stats_new_all_bench(const cpl_image * image, int n)
{

    double       secs;
    int          i;

    secs  = cpl_test_get_cputime();

    for (i = 0; i < n; i++) {
        cpl_stats * self
            = cpl_stats_new_from_image_window(image, CPL_STATS_ALL, 2, 2,
                                              IMAGE_SIZE_X-1, IMAGE_SIZE_Y-1);

        cpl_test_nonnull(self);

        cpl_stats_delete(self);
    }

    secs = cpl_test_get_cputime() - secs;

    cpl_msg_info(cpl_func,"Time spent computing %d %d X %d - sized image "
                 "statistics [s]: %g", n, IMAGE_SIZE_X, IMAGE_SIZE_Y, secs);
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Benchmark the CPL function
  @param image The image to test on
  @param n     The number of repeats
  @return   void
 */
/*----------------------------------------------------------------------------*/
static void cpl_stats_new_std_bench(const cpl_image * image, int n)
{

    double       secs;
    int          i;

    secs  = cpl_test_get_cputime();

    for (i = 0; i < n; i++) {
        cpl_stats * self
            = cpl_stats_new_from_image_window(image,
                                              CPL_STATS_MEAN | CPL_STATS_STDEV,
                                              2, 2,
                                              IMAGE_SIZE_X-1, IMAGE_SIZE_Y-1);

        cpl_test_nonnull(self);

        cpl_stats_delete(self);
    }

    secs = cpl_test_get_cputime() - secs;

    cpl_msg_info(cpl_func,"Time spent computing %d %d X %d - sized image "
                 "mean and stdev [s]: %g", n, IMAGE_SIZE_X, IMAGE_SIZE_Y, secs);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Benchmark the CPL function
  @param image The image to test on
  @param n     The number of repeats
  @return   void
 */
/*----------------------------------------------------------------------------*/
static void cpl_stats_new_median_bench(const cpl_image * image, int n)
{

    double       secs;
    int          i;

    secs  = cpl_test_get_cputime();

    for (i = 0; i < n; i++) {
        cpl_stats * self
            = cpl_stats_new_from_image_window(image, CPL_STATS_MEDIAN, 2, 2,
                                              IMAGE_SIZE_X-1, IMAGE_SIZE_Y-1);

        cpl_test_nonnull(self);

        cpl_stats_delete(self);
    }

    secs = cpl_test_get_cputime() - secs;

    cpl_msg_info(cpl_func,"Time spent computing %d %d X %d - sized image "
                 "median [s]: %g", n, IMAGE_SIZE_X, IMAGE_SIZE_Y, secs);
}
