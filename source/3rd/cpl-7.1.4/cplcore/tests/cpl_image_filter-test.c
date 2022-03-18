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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>
#include <float.h>
#include <assert.h>

#include "cpl_image_filter.h"
#include "cpl_image_gen.h"
#include "cpl_image_io.h"
#include "cpl_tools.h"
#include "cpl_test.h"
#include "cpl_image_bpm.h"
#include "cpl_memory.h"

/*-----------------------------------------------------------------------------
                                Defines
 -----------------------------------------------------------------------------*/

#define CPL_IMAGE_FILTER_LINEAR 0
#define CPL_IMAGE_FILTER_MORPHO 1
#define CPL_IMAGE_FILTER_SIZE   2

#ifndef IMAGE_SIZE_X
#define IMAGE_SIZE_X 64
#endif
#ifndef IMAGE_SIZE_Y
#define IMAGE_SIZE_Y IMAGE_SIZE_X
#endif

/*-----------------------------------------------------------------------------
                        Private function prototypes
 -----------------------------------------------------------------------------*/

static void cpl_image_filter_test(int, int, int, int);
#ifndef CPL_MEDIAN_SAMPLE
static void cpl_image_filter_test_5198(void);
#endif

static void cpl_image_filter_test_5320(void);

static void cpl_image_filter_test_1d(void);

/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/
int main(void)
{
    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    /* Insert tests below */
    cpl_image_filter_test_5320();
#ifndef CPL_MEDIAN_SAMPLE
    cpl_image_filter_test_5198();
#endif
    cpl_image_filter_test(8, 8, 0, 0);
    cpl_image_filter_test(8, 8, 1, 0);
    cpl_image_filter_test(8, 8, 0, 1);
    cpl_image_filter_test(8, 8, 1, 1);
    cpl_image_filter_test(8, 8, 2, 0);
    cpl_image_filter_test(8, 8, 0, 2);
    cpl_image_filter_test(8, 8, 2, 2);

    cpl_image_filter_test(31, 31, 2, 2);
    cpl_image_filter_test(15, 63, 1, 1);

    cpl_image_filter_test_1d();

    return cpl_test_end(0);
}


/*----------------------------------------------------------------------------*/
/**
  @brief  Test cpl_image_filter() and cpl_image_filter_mask()
  @param  nx      The image X-size
  @param  ny      The image Y-size
  @param  hm      The kernel X-halfsize
  @param  hn      The kernel Y-halfsize
  @param  type    The filtering type
  @return void

 */
/*----------------------------------------------------------------------------*/
static void cpl_image_filter_test(int nx, int ny, int hm, int hn)
{

    const cpl_filter_mode filter_mode[] = {CPL_FILTER_MEDIAN,
                                           CPL_FILTER_STDEV, 
                                           CPL_FILTER_STDEV_FAST, 
                                           CPL_FILTER_AVERAGE, 
                                           CPL_FILTER_AVERAGE_FAST,
                                           CPL_FILTER_LINEAR,
                                           CPL_FILTER_LINEAR_SCALE};

    /* Whether the above filter is used with cpl_image_filter_mask() */
    const cpl_boolean filter_mask[] = {CPL_TRUE, CPL_TRUE, CPL_TRUE,
                                       CPL_TRUE, CPL_TRUE,
                                       CPL_FALSE, CPL_FALSE};
    /* Compare with different filter on a kernel also of all ones */
    const cpl_filter_mode filter_ones[]  = {CPL_FILTER_MEDIAN,
                                            CPL_FILTER_STDEV_FAST, 
                                            CPL_FILTER_STDEV, 
                                            CPL_FILTER_LINEAR,
                                            CPL_FILTER_LINEAR,
                                            CPL_FILTER_MORPHO,
                                            CPL_FILTER_MORPHO_SCALE};

    /* Whether the above filter is used with cpl_image_filter_mask() */
    const cpl_boolean filter_mask1[] = {CPL_TRUE, CPL_TRUE, CPL_TRUE,
                                        CPL_FALSE, CPL_FALSE,
                                        CPL_FALSE, CPL_FALSE};

    const cpl_type pixel_type[] = {CPL_TYPE_INT, CPL_TYPE_FLOAT,
                                   CPL_TYPE_DOUBLE};

    const int m = 1 + 2 * hm;
    const int n = 1 + 2 * hn;
    const double xmin = -100.0;
    const double xmax =  200.0;

    cpl_mask * window1 = cpl_mask_new(m, n);
    cpl_mask * window2 = cpl_mask_new(m+2, n+2);
    cpl_mask * shift   = cpl_mask_new(m, n);
    cpl_matrix * xwindow1 = cpl_matrix_new(n, m);
    cpl_matrix * xwindow2 = cpl_matrix_new(n+2, m+2);
    cpl_matrix * xshift   = cpl_matrix_new(n, m);
    unsigned itype1;
    int i, j;

    cpl_test_eq(sizeof(filter_mode)/sizeof(filter_mode[0]),
                sizeof(filter_mask)/sizeof(filter_mask[0]));

    cpl_test_eq(sizeof(filter_mask)/sizeof(filter_mask[0]),
                sizeof(filter_mask1)/sizeof(filter_mask1[0]));

    cpl_test_eq(sizeof(filter_ones)/sizeof(filter_ones[0]),
                sizeof(filter_mask1)/sizeof(filter_mask1[0]));

    cpl_test_zero(cpl_mask_not(window1));
    cpl_test_zero(cpl_matrix_fill(xwindow1, 1.0));

    for (j = 2; j <= n+1; j++) {
        for (i = 2; i <= m+1; i++) {
            cpl_mask_set(window2, i, j, CPL_BINARY_1);
            cpl_matrix_set(xwindow2, j-1, i-1, 1.0);
        }
    }
    cpl_test_eq(cpl_mask_count(window1), m*n);
    cpl_test_eq(cpl_mask_count(window2), m*n);

    for (itype1 = 0; 
         itype1 < sizeof(pixel_type)/sizeof(pixel_type[0]); 
         itype1++) {
        unsigned itype2;

        for (itype2 = 0; 
             itype2 < sizeof(pixel_type)/sizeof(pixel_type[0]); 
             itype2++) {
            unsigned ifilt;

            for (ifilt = 0; 
                 ifilt < sizeof(filter_mode)/sizeof(filter_mode[0]); 
                 ifilt++) {
                const cpl_error_code exp_err = m == 1 && n == 1
                    && (filter_mode[ifilt] == CPL_FILTER_STDEV ||
                        filter_mode[ifilt] == CPL_FILTER_STDEV_FAST)
                    ? CPL_ERROR_DATA_NOT_FOUND
                    : CPL_ERROR_NONE;

                if (filter_mode[ifilt] != CPL_FILTER_MEDIAN ||
                    itype1 == itype2) {

                    /* FIXME: Need to test with bad pixels as well */
                    cpl_image * testimg
                        = cpl_image_new(nx, ny, pixel_type[itype1]);
                    cpl_image * filtim1
                        = cpl_image_new(nx, ny, pixel_type[itype2]);
                    cpl_image * filtim2
                        = cpl_image_new(nx, ny, pixel_type[itype2]);

                    cpl_error_code error;
                    double tol = pixel_type[itype1] == CPL_TYPE_FLOAT
                        ? 5e-05 : 0.0;

                    cpl_msg_info(cpl_func, "%u <=> %u - filtering %u X %u %s "
                                 "to %s with %u X %u", filter_mode[ifilt],
                                 filter_ones[ifilt], nx, ny,
                                 cpl_type_get_name(pixel_type[itype1]),
                                 cpl_type_get_name(pixel_type[itype2]), m, n);

                    cpl_test_zero(cpl_image_fill_noise_uniform(testimg,
                                                               xmin, xmax));

                    error = filter_mask[ifilt]
                        ? cpl_image_filter_mask(filtim1, testimg, window1,
                                                filter_mode[ifilt],
                                                CPL_BORDER_FILTER)
                        : cpl_image_filter(filtim1, testimg, xwindow1,
                                           filter_mode[ifilt],
                                           CPL_BORDER_FILTER);
                    cpl_test_eq_error(error, exp_err);

                    error = filter_mask[ifilt]
                        ? cpl_image_filter_mask(filtim2, testimg, window2,
                                                filter_mode[ifilt],
                                                CPL_BORDER_FILTER)
                        : cpl_image_filter(filtim2, testimg, xwindow2,
                                           filter_mode[ifilt],
                                           CPL_BORDER_FILTER);
                    cpl_test_eq_error(error, exp_err);

                    if (exp_err == CPL_ERROR_NONE) {

                        if (filter_mode[ifilt] == CPL_FILTER_MEDIAN) {
                            /* FIXME: Cannot trust border (with even numbers) */
                            for (j = 1; j <= ny; j++) {
                                for (i = 1; i <= nx; i++) {
                                    if (j < n || ny - n < j ||
                                        i < m || nx - m < i) {
                                        cpl_image_reject(filtim1, i, j);
                                        cpl_image_reject(filtim2, i, j);
                                    }
                                }
                            }
                        } else {
                            /* filt1 and filt2 must be identical, because the
                               mask of filt2 equals filt1, except that is has a
                               halo of zeros */
                            if (filter_mode[ifilt] == CPL_FILTER_AVERAGE_FAST
                                || filter_mode[ifilt] == CPL_FILTER_STDEV_FAST) {
                                /* In this case filtim1 is actually done with 
                                   CPL_FILTER_AVERAGE */
                                tol = 10.0 * (xmax - xmin)
                                    * ((pixel_type[itype1] == CPL_TYPE_FLOAT || 
                                        pixel_type[itype2] == CPL_TYPE_FLOAT)
                                       ? FLT_EPSILON : DBL_EPSILON);
                            } else if (filter_mode[ifilt] == CPL_FILTER_STDEV
                                       && pixel_type[itype2] == CPL_TYPE_DOUBLE
                                       && m*n > 1 && nx > m && ny > n) {
                                /* Verify a non-border value */

                                int is_bad;
                                const double stdev
                                    = cpl_image_get_stdev_window(testimg,
                                                                 nx-m+1,
                                                                 ny-n+1, nx,
                                                                 ny);

                                const double filtered = cpl_image_get(filtim1,
                                                                      nx-hm,
                                                                      ny-hn,
                                                                      &is_bad);
                                if (!is_bad) {
                                    cpl_test_rel(stdev, filtered,
                                                 4.0 * DBL_EPSILON);
                                }
                            }

                            /* The precision loss is higher in this case */
                            if (filter_mode[ifilt] == CPL_FILTER_STDEV_FAST)
                                tol = 1.0;
                        }
                        cpl_test_image_abs(filtim1, filtim2, tol);

                        if (filter_ones[ifilt] != filter_mode[ifilt]) {
                            /* Result should equal that of a different filter
                               also with all ones */
                            /* The additions are reordered (sorted),
                               i.e. different round-off */
                            const cpl_boolean isfloat =
                                pixel_type[itype1] == CPL_TYPE_FLOAT || 
                                pixel_type[itype2] == CPL_TYPE_FLOAT;
                            tol = (xmax - xmin)
                                * (isfloat ? FLT_EPSILON : DBL_EPSILON);

                            if (filter_mode[ifilt] == CPL_FILTER_LINEAR_SCALE
                                && !isfloat) tol *= (double)(n*m);
                            else if (filter_mode[ifilt] == CPL_FILTER_LINEAR &&
                                     filter_ones[ifilt] == CPL_FILTER_MORPHO &&
                                     !isfloat)
                                    tol *= 1.5;
                            if (filter_mode[ifilt] == CPL_FILTER_AVERAGE_FAST)
                                tol *= (1.0 + (double)n)*(1.0 + (double)m);
                            if (filter_mode[ifilt] == CPL_FILTER_STDEV_FAST ||
                                filter_ones[ifilt] == CPL_FILTER_STDEV_FAST) {
                                if (pixel_type[itype1] == CPL_TYPE_INT &&
                                    pixel_type[itype2] == CPL_TYPE_INT)
                                    tol = -1.0; /* FIXME: Not supported */
                                else tol *= 100.0 * (double)(n * m);
                            }

                            error = filter_mask1[ifilt] ?
                                cpl_image_filter_mask(filtim2, testimg, window1,
                                                      filter_ones[ifilt],
                                                      CPL_BORDER_FILTER) :
                                cpl_image_filter(filtim2, testimg,
                                                 xwindow1,
                                                 filter_ones[ifilt],
                                                 CPL_BORDER_FILTER);
                            cpl_test_eq_error(error, exp_err);
                            if (tol >= 0.0)
                                cpl_test_image_abs(filtim1, filtim2, tol);
                        }

                        if (itype1 == itype2) {

                            /* If the input and output pixel types are identical
                               a mask with 1 element corresponds to a shift */

                            /* Try all possible 1-element masks */
                            for (j = 1; j <= n; j++) {
                                for (i = 1; i <= m; i++) {

                                    /* Empty mask */
                                    cpl_test_zero(cpl_mask_xor(shift, shift));
                                    cpl_test_zero(cpl_matrix_fill(xshift, 0.0));

                                    /* Set one element */
                                    cpl_test_zero(cpl_mask_set(shift, i, j,
                                                               CPL_BINARY_1));
                                    cpl_test_zero(cpl_matrix_set(xshift, n-j,
                                                                 i-1, 1.0));


                                    cpl_test_eq(cpl_mask_count(shift), 1);

                                    /* This filter corresponds to a shift of
                                       ((hm+1) - i, (hn+1) - j) */
                                    error = filter_mask[ifilt]
                                        ? cpl_image_filter_mask(filtim1,
                                                                testimg,
                                                                shift,
                                                                filter_mode[ifilt],
                                                                CPL_BORDER_FILTER)

                                        : cpl_image_filter(filtim1,
                                                           testimg,
                                                           xshift,
                                                           filter_mode[ifilt],
                                                           CPL_BORDER_FILTER);


                                    cpl_test_error(error);
                                    if (filter_mode[ifilt] == CPL_FILTER_STDEV
                                        || filter_mode[ifilt] ==
                                        CPL_FILTER_STDEV_FAST) {
                                        cpl_test_eq(error,
                                                    CPL_ERROR_DATA_NOT_FOUND);
                                    } else {
                                        cpl_test_zero(error);

                                        cpl_test_zero(cpl_image_copy(filtim2,
                                                                     testimg,
                                                                     1, 1));
                                        cpl_test_zero(cpl_image_shift(filtim2,
                                                                      (hm+1)-i,
                                                                      (hn+1)-j));

                                        cpl_test_image_abs(filtim1, filtim2,
                                                           0.0);
                                    }

                                    if (filter_mode[ifilt] == CPL_FILTER_MEDIAN) {
                                        error =
                                            cpl_image_filter_mask(filtim1,
                                                                  testimg,
                                                                  shift,
                                                                  filter_mode[ifilt],
                                                                  CPL_BORDER_COPY);
                                        cpl_test_eq_error(error,
                                                          CPL_ERROR_NONE);
                                        if (i == hm+1 && j == hn+1) {
                                            /* FIXME: Verify result also for
                                               non-zero shifts */

                                            cpl_test_image_abs(filtim1, testimg,
                                                               0.0);
                                        }

                                        error =
                                            cpl_image_filter_mask(filtim1,
                                                                  testimg,
                                                                  shift,
                                                                  filter_mode[ifilt],
                                                                  CPL_BORDER_NOP);
                                        cpl_test_eq_error(error,
                                                          CPL_ERROR_NONE);
                                        if (i == hm+1 && j == hn+1) {
                                            /* FIXME: Verify result also for
                                               non-zero shifts */

                                            cpl_test_image_abs(filtim1, testimg,
                                                               0.0);
                                        }
                                    }
                                }
                            }
                        }
                    }

                    cpl_image_delete(testimg);
                    cpl_image_delete(filtim1);
                    cpl_image_delete(filtim2);

                }
            }
        }
    }

    cpl_mask_delete(shift);
    cpl_mask_delete(window1);
    cpl_mask_delete(window2);
    cpl_matrix_delete(xshift);
    cpl_matrix_delete(xwindow1);
    cpl_matrix_delete(xwindow2);

    return;
}

#ifndef CPL_MEDIAN_SAMPLE
/*----------------------------------------------------------------------------*/
/**
  @brief  Verify the border filtering with and without a central bad pixel
  @return void
  @note This test is not needed when the median of an even sized number of
        samples is defined as one of the sampled values

 */
/*----------------------------------------------------------------------------*/
static void cpl_image_filter_test_5198(void)
{

    cpl_image * img = cpl_image_new(100, 163, CPL_TYPE_DOUBLE);
    double   * d = cpl_image_get_data_double(img);
    cpl_mask * m = cpl_mask_new(3,3);
    cpl_image *res;
    cpl_image *resbpm;
    cpl_size i, j;

    for (i = 0; i < 100*163; i++) {
        d[i] = i;
    }


    res = cpl_image_duplicate(img);
    resbpm = cpl_image_duplicate(img);

    cpl_mask_not(m);

    cpl_image_filter_mask(res, img, m, CPL_FILTER_MEDIAN,
                          CPL_BORDER_FILTER);

    cpl_image_reject(img, 30, 30);
    cpl_image_filter_mask(resbpm, img, m, CPL_FILTER_MEDIAN,
                          CPL_BORDER_FILTER);

    for (j = 27; j < 33; j++) {
        for (i = 27; i < 33; i++) {
            cpl_image_set(res,    i, j, 0.0);
            cpl_image_set(resbpm, i, j, 0.0);
        }
    }

    cpl_test_image_abs(res, resbpm, 0.0);
    cpl_image_delete(img);
    cpl_image_delete(res);
    cpl_image_delete(resbpm);
    cpl_mask_delete(m);
}
#endif

/*----------------------------------------------------------------------------*/
/**
  @brief  Verify the isotropy of linear/average filtering
  @return void
  @see Ticket PIPE-5320

 */
/*----------------------------------------------------------------------------*/
static void cpl_image_filter_test_5320(void)
{
    cpl_error_code code;
    /* The dimensions of the matrix and image/mask objects are
       specified differently! */
    cpl_matrix * kernel  = cpl_matrix_new(91, 1);
    cpl_mask   * kernel2 = cpl_mask_new(1, 91);

    cpl_image * unfiltered = cpl_image_load("unfiltered_image.fits",
                                            CPL_TYPE_UNSPECIFIED, 0, 0);
    cpl_image * filtered;
    cpl_image * filtered2;

    double tol = 0.0;

    int itheta;
    int i, ifilter = CPL_FILTER_LINEAR;


    if (unfiltered == NULL) {
        cpl_msg_info(cpl_func, "Using noise image for test 5320");
        cpl_test_error(CPL_ERROR_FILE_IO);

        unfiltered = cpl_image_new(400, 400, CPL_TYPE_DOUBLE);

        code = cpl_image_fill_noise_uniform(unfiltered, 0.0, 1.0);
        cpl_test_eq_error(code, CPL_ERROR_NONE);
        tol = 8.0 * DBL_EPSILON;
    } else {
        cpl_test_error(CPL_ERROR_NONE);
    }

    filtered   = cpl_image_duplicate(unfiltered);
    filtered2  = cpl_image_duplicate(unfiltered);

    code = cpl_matrix_fill(kernel, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_mask_not(kernel2);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_image_filter(filtered, unfiltered, kernel,
                            CPL_FILTER_LINEAR, CPL_BORDER_FILTER);

    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_image_filter_mask(filtered2, unfiltered, kernel2,
                                 CPL_FILTER_AVERAGE, CPL_BORDER_FILTER);

    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_image_abs(filtered, filtered2, 6.0 * DBL_EPSILON);

    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_image_filter_mask(filtered2, unfiltered, kernel2,
                                 CPL_FILTER_AVERAGE_FAST, CPL_BORDER_FILTER);

    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_image_abs(filtered, filtered2, 256.0 * DBL_EPSILON);

    for (i = 0; i < 2; i +=1, ifilter = CPL_FILTER_AVERAGE) {
        for (itheta = 0; itheta < 4; itheta +=2) {

            code = cpl_image_flip(unfiltered, itheta);
            cpl_test_eq_error(code, CPL_ERROR_NONE);

            code = i == 0 ? cpl_image_filter(filtered2, unfiltered, kernel,
                                             ifilter, CPL_BORDER_FILTER)
                :  cpl_image_filter_mask(filtered2, unfiltered, kernel2,
                                         ifilter, CPL_BORDER_FILTER);

            cpl_test_eq_error(code, CPL_ERROR_NONE);

            code = cpl_image_flip(filtered2, itheta);
            cpl_test_eq_error(code, CPL_ERROR_NONE);

            cpl_test_image_abs(filtered, filtered2, tol);

            code = cpl_image_flip(unfiltered, itheta);
            cpl_test_eq_error(code, CPL_ERROR_NONE);

        }
    }

    cpl_matrix_delete(kernel);
    cpl_mask_delete(kernel2);
    cpl_image_delete(filtered);
    cpl_image_delete(filtered2);
    cpl_image_delete(unfiltered);
}

/*----------------------------------------------------------------------------*/
/**
  @brief  Verify a case of 1D-filtering, a running median
  @return void

 */
/*----------------------------------------------------------------------------*/
static void cpl_image_filter_test_1d(void)
{
    /* The length of the 1D-image from which to compute the running median */
    const int nx = IMAGE_SIZE_X;
    const int hsz = 15; /* Median half-size */
    const int winnx = 2 * hsz + 1; /* The running window size */

    cpl_image * raw = cpl_image_new(nx, 1, CPL_TYPE_FLOAT);
    cpl_image * filtered = cpl_image_new(nx - 2 * hsz, 1, CPL_TYPE_FLOAT);
    cpl_mask * kernel = cpl_mask_new(winnx, 1); 

    cpl_error_code code = cpl_image_fill_noise_uniform(raw, 0.0, 1.0);

    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_mask_not(kernel);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_image_filter_mask(filtered, raw, kernel, CPL_FILTER_MEDIAN,
                                 CPL_BORDER_CROP);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_mask_delete(kernel);
    cpl_image_delete(raw);
    cpl_image_delete(filtered);
}
