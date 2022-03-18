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

#include <math.h>

#include <cpl_image_gen.h>
#include <cpl_image_bpm.h>
#include <cpl_image_io.h>
#include <cpl_test.h>
#include <cpl_memory.h>

#include "cpl_detector.h"

/*-----------------------------------------------------------------------------
                                  Define
 -----------------------------------------------------------------------------*/

#define IMAGESZX         10
#define IMAGESZY         20
#define IMAGESZ2        100

/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/
int main(void)
{
    const cpl_type pixel_type[] = {CPL_TYPE_DOUBLE, CPL_TYPE_FLOAT,
                                   CPL_TYPE_INT, CPL_TYPE_DOUBLE_COMPLEX,
                                   CPL_TYPE_FLOAT_COMPLEX};
    const int      nbad = 10;
    /* The bad pixel positions */
    const int      bad_pos_x[] = {4, 5, 6, 4, 5, 6, 4, 5, 6, 8};
    const int      bad_pos_y[] = {5, 5, 5, 6, 6, 6, 7, 7, 7, 9};
    /* Two concentric circles in the image center */
    const double   zone1[]
        = {0.5*IMAGESZ2, 0.5*IMAGESZ2, IMAGESZ2/10.0, IMAGESZ2/5.0};
    const double   zone2[]
        = {0.5*IMAGESZ2, 0.5*IMAGESZ2, IMAGESZ2/10.0, IMAGESZ2/10.0};
    double         noise, errnoise;
    unsigned       itype;
    int            i;
    cpl_error_code error;


    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    /* Insert tests below */

    /* Test 0: Test with nbad pixels */
    cpl_test_eq(nbad, sizeof(bad_pos_x)/sizeof(bad_pos_x[0]));
    cpl_test_eq(nbad, sizeof(bad_pos_y)/sizeof(bad_pos_y[0]));

    for (itype = 0; 
         itype < sizeof(pixel_type)/sizeof(pixel_type[0]); 
         itype++) {
        const cpl_type mytype = pixel_type[itype];
        const cpl_boolean is_complex =
            pixel_type[itype] == CPL_TYPE_DOUBLE_COMPLEX ||
            pixel_type[itype] == CPL_TYPE_FLOAT_COMPLEX;

        cpl_image * im;
        cpl_image * copy;
        double      stdev;

        /* Create the test image */
        im = cpl_image_new(IMAGESZX, IMAGESZY, mytype);
        error = cpl_image_fill_noise_uniform(im, 10, 20);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        /* Test 1: cpl_detector_interpolate_rejected() may not modify a bad-pixel
           free image */
        copy = cpl_image_duplicate(im);
        cpl_test_nonnull(cpl_image_get_bpm(copy)); /* Create empty bpm */
        error = cpl_detector_interpolate_rejected(copy);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        if (!is_complex) cpl_test_image_abs(im, copy, 0.0);
        cpl_image_delete(copy);

        /* Set the bad pixels values */
        for (i = 0; i < nbad; i++) {
            if (!is_complex)
                cpl_image_set(im, bad_pos_x[i], bad_pos_y[i], FLT_MAX);
            cpl_image_reject(im, bad_pos_x[i], bad_pos_y[i]);
        }

        cpl_test_eq(cpl_image_count_rejected(im), nbad);

        /* Test 2: cpl_detector_interpolate_rejected() on some bad pixels */

        copy = cpl_image_duplicate(im);

        if (!is_complex) stdev = cpl_image_get_stdev(im);

        error = cpl_detector_interpolate_rejected(im);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        cpl_test_null(cpl_image_get_bpm_const(im));

        if (!is_complex) cpl_test_leq(cpl_image_get_stdev(im), stdev);

        if (pixel_type[itype] == CPL_TYPE_DOUBLE) {
            /* In absence of an actual unit test for the cleaning, a regression
               test may be used to ensure that no unintended changes occur */
            cpl_image * load
                = cpl_image_load("regression.fits", CPL_TYPE_DOUBLE, 0, 0);

            if (load == NULL) {
                cpl_test_error(CPL_ERROR_FILE_IO);
#ifdef CPL_DETECTOR_PREPARE_REGRESSION
                /*
                  Enable this call before making changes to 
                  cpl_detector_interpolate_rejected()
                  to enable a regression test of your changes.
                */
                cpl_image_save(im, "regression.fits", CPL_TYPE_DOUBLE,
                               NULL, CPL_IO_CREATE);
#endif
            } else {
                cpl_test_error(CPL_ERROR_NONE);
                cpl_test_image_abs(load, im, 20.0 * DBL_EPSILON);

                cpl_image_delete(load);
            }
        }

        if (!is_complex) {
            /* Test 2a: Verify that the cleaning is isotropic */

            for (i = 1; i < 4; i++) {
                cpl_image * turn = cpl_image_duplicate(copy);

                cpl_test_nonnull(turn);

                error = cpl_image_turn(turn, i);
                cpl_test_eq_error(error, CPL_ERROR_NONE);

                error = cpl_detector_interpolate_rejected(turn);
                cpl_test_eq_error(error, CPL_ERROR_NONE);
                cpl_test_null(cpl_image_get_bpm_const(turn));

                error = cpl_image_turn(turn, -i);
                cpl_test_eq_error(error, CPL_ERROR_NONE);

                /* Non-zero tolerance, since order of additions changed */
                if (!is_complex)
                    cpl_test_image_abs(turn, im, 40.0 * DBL_EPSILON);

                cpl_image_delete(turn);
            }
        }

        cpl_image_delete(copy);

        /* Reject all pixels */
        error = cpl_mask_not(cpl_image_get_bpm(im));
        cpl_test_eq_error(error, CPL_ERROR_NONE);
        cpl_test_eq(cpl_image_count_rejected(im), IMAGESZX * IMAGESZY);
        error = cpl_detector_interpolate_rejected(im);
        cpl_test_eq_error(error, CPL_ERROR_DATA_NOT_FOUND);

        /* Reject all but one pixel */
        error = cpl_image_accept(im, IMAGESZX, IMAGESZY);
        cpl_test_eq_error(error, CPL_ERROR_NONE);
        cpl_test_eq(cpl_image_count_rejected(im), IMAGESZX * IMAGESZY - 1);
        error = cpl_detector_interpolate_rejected(im);
        cpl_test_eq_error(error, CPL_ERROR_NONE);
        cpl_test_null(cpl_image_get_bpm_const(im));

        cpl_image_delete(im);

        /* Create an other test image */
        im = cpl_image_new(IMAGESZ2, IMAGESZ2, CPL_TYPE_DOUBLE);
        cpl_image_fill_noise_uniform(im, 10, 20);

        /* Test 3: the RON & BIAS computation */
        error = cpl_flux_get_noise_window(im, NULL, -1, -1, &noise, &errnoise);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_flux_get_bias_window(im, NULL, -1, -1, &noise, &errnoise);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        /* Test 4: the RON computation - on a ring */
        error = cpl_flux_get_noise_ring(im, zone1, -1, -1, &noise, &errnoise);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        /* Test 5: An empty ring */
        error = cpl_flux_get_noise_ring(im, zone2, -1, -1, &noise, &errnoise);
        cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

        /* Test 6A: NULL input */

        error = cpl_flux_get_noise_ring(im, NULL, -1, -1, &noise, &errnoise);
        cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

        error = cpl_flux_get_noise_ring(im, zone1, -1, -1, NULL, NULL);
        cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

        cpl_image_delete(im);

    }

    /* Test 6B: NULL input */
    error = cpl_detector_interpolate_rejected(NULL);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

    error = cpl_flux_get_noise_window(NULL, NULL, -1, -1, &noise, &errnoise);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

    error = cpl_flux_get_bias_window(NULL, NULL, -1, -1, &noise, &errnoise);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

    error = cpl_flux_get_noise_ring(NULL, zone1, -1, -1, &noise, &errnoise);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

    return cpl_test_end(0);
}
