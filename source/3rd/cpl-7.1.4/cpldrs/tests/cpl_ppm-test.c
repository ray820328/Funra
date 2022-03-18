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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>

#include "cpl_test.h"
#include "cpl_math_const.h"
#include "cpl_ppm.h"
#include "cpl_error.h"
#include "cpl_msg.h"
#include "cpl_memory.h"

int main(void)
{
    cpl_matrix *pattern  = NULL;
    cpl_matrix *data     = NULL;
    cpl_matrix *rdata    = NULL;
    cpl_matrix *mpattern = NULL;
    cpl_matrix *mdata    = NULL;
    cpl_array  *matches  = NULL;
    cpl_array  *nullarray= NULL;
    cpl_matrix *rotate   = NULL;
    cpl_matrix *twobytwo = NULL;

    const double buffer[] = {96.807119, 6.673062,
                             47.828109, 90.953442,
                             35.169238, 93.253366,
                             65.443582, 2.107025,
                             51.220486, 20.201893,
                             93.997703, 20.408227,
                             37.882893, 79.311394,
                             28.820079, 26.715673,
                             35.682260, 12.837355,
                             70.329776, 73.741373,
                             80.252114, 91.523087,
                             51.138163, 76.205738,
                             45.638141, 47.106201,
                             29.955025, 61.255939,
                             7.338079, 49.818536,
                             21.958749, 4.145198,
                             56.491598, 69.786858,
                             95.098640, 91.660836,
                             63.040224, 60.542222,
                             93.767861, 14.260710,
                             80.744116, 87.765564,
                             34.668937, 18.627008,
                             67.076958, 63.489016,
                             45.342681, 2.759218,
                             76.326371, 15.672457,
                             76.500591, 56.578485,
                             7.195544, 27.638754,
                             32.784223, 52.833685,
                             74.744955, 62.739249,
                             14.089624, 82.083033,
                             12.557785, 36.048373,
                             86.228231, 69.049383,
                             5.835231, 81.326871,
                             60.710220, 68.875455,
                             41.869094, 54.478081,
                             83.136166, 22.613209,
                             42.243645, 17.805103,
                             41.240218, 9.320603,
                             81.294120, 86.582899,
                             12.079821, 57.620490,
                             2.255356, 88.580412,
                             14.198976, 9.450900,
                             16.219166, 46.983199,
                             62.284586, 90.964121,
                             9.722447, 76.374210,
                             73.047154, 22.280233,
                             12.422583, 59.275385,
                             91.329616, 18.257814,
                             40.602257, 52.039836,
                             87.133270, 82.471350,
                             6.517916, 70.269436,
                             5.084560, 48.761561,
                             88.074539, 46.324777,
                             58.082164, 69.368659,
                             32.907676, 70.161985,
                             26.989149, 35.163032,
                             58.742397, 41.188125,
                             44.613932, 74.961563,
                             88.171324, 6.898518,
                             65.925684, 97.893771,
                             83.272728, 38.972839,
                             20.174004, 95.695311,
                             98.248224, 11.503620,
                             13.953125, 38.850481,
                             63.543456, 1.086395,
                             21.321831, 70.061372,
                             71.355831, 26.406390,
                             18.822933, 59.430370,
                             72.731168, 76.905097,
                             28.799029, 5.638844,
                             47.067082, 55.788179,
                             40.801876, 5.809480,
                             96.976304, 85.415809,
                             80.771043, 85.147628,
                             92.314327, 46.696728,
                             83.041400, 75.587054,
                             85.669566, 3.215404,
                             71.282365, 83.917790,
                             14.719024, 85.235491,
                             22.768271, 78.262480,
                             86.321886, 44.090102,
                             48.323852, 57.677717,
                             70.496492, 67.146785,
                             17.108088, 43.227660,
                             44.051883, 45.907117,
                             48.866504, 91.118965,
                             1.695296, 89.668380,
                             96.928445, 98.671600,
                             75.084189, 77.699488,
                             83.819228, 67.398515,
                             24.396216, 66.860628,
                             42.985570, 10.065782,
                             70.076031, 14.267935,
                             93.983572, 84.795055,
                             99.503426, 16.751843,
                             63.057535, 85.825312,
                             60.841945, 11.381387,
                             43.503029, 31.338437,
                             78.528172, 60.611117,
                             74.566097, 22.580055};

    const int permute[]
        = {8,  2,  1, 13,  7,  3,  5,  9, 14,  4,  0,  6, 11, 10, 12,
           23, 17, 16, 28, 22, 18, 20, 24, 29, 19, 15, 21, 26, 25, 27};

    int i, j, k, npattern, ndata;

    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);


    /*
     * Create a list of 3 points (0,9), 9,0), and (9,9):
     *
     *                0 9 9
     *                9 0 9
     */

    pattern = cpl_matrix_new(2, 3);
    cpl_matrix_set(pattern, 0, 0, 0.0);
    cpl_matrix_set(pattern, 0, 1, 9.0);
    cpl_matrix_set(pattern, 0, 2, 9.0);
    cpl_matrix_set(pattern, 1, 0, 9.0);
    cpl_matrix_set(pattern, 1, 1, 0.0);
    cpl_matrix_set(pattern, 1, 2, 9.0);

    /*
     * Create an identical data matrix
     */

    data = cpl_matrix_duplicate(pattern);

    /*
     * Now try to match points: the transformation should be an identity
     * (rotation angle 0, scaling 0, translation 0):
     */

    cpl_msg_info(cpl_func, "Trying to match 3 identical points:");

    matches = cpl_ppm_match_points(data, 3, 1, pattern, 3, 0,
                                   0.1, 1, &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(matches);
    cpl_test_nonnull(mpattern);
    cpl_test_nonnull(mdata);
    cpl_test_eq(cpl_matrix_get_ncol(mpattern), 3);
    cpl_test_eq(cpl_array_get_size(matches), 3);

    cpl_msg_info(cpl_func, "%" CPL_SIZE_FORMAT " points matched",
                 cpl_matrix_get_ncol(mpattern));

    cpl_array_delete(matches);
    cpl_matrix_delete(mpattern);
    cpl_matrix_delete(mdata);

    cpl_msg_info(cpl_func, "Scale data points by 2:");

    cpl_matrix_multiply_scalar(data, 2.0);

    matches = cpl_ppm_match_points(data, 3, 1, pattern, 3, 0,
                                   0.1, 1, &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(matches);
    cpl_test_nonnull(mpattern);
    cpl_test_nonnull(mdata);
    cpl_test_eq(cpl_matrix_get_ncol(mpattern), 3);
    cpl_test_eq(cpl_array_get_size(matches), 3);

    cpl_msg_info(cpl_func, "%" CPL_SIZE_FORMAT " points matched",
                 cpl_matrix_get_ncol(mpattern));

    cpl_array_delete(matches);
    cpl_matrix_delete(mpattern);
    cpl_matrix_delete(mdata);

    cpl_msg_info(cpl_func, "Rotate data points by +45 degrees:");

    rotate = cpl_matrix_new(2, 2);
    cpl_matrix_set(rotate, 0, 0, sqrt(2)/2);
    cpl_matrix_set(rotate, 0, 1, -sqrt(2)/2);
    cpl_matrix_set(rotate, 1, 0, sqrt(2)/2);
    cpl_matrix_set(rotate, 1, 1, sqrt(2)/2);

    rdata = cpl_matrix_product_create(rotate, data);

    cpl_matrix_delete(rotate);
    cpl_matrix_delete(data);

    matches = cpl_ppm_match_points(rdata, 3, 1, pattern, 3, 0,
                                   0.1, 1, &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(matches);
    cpl_test_nonnull(mpattern);
    cpl_test_nonnull(mdata);
    cpl_test_eq(cpl_matrix_get_ncol(mpattern), 3);
    cpl_test_eq(cpl_array_get_size(matches), 3);

    cpl_msg_info(cpl_func, "%" CPL_SIZE_FORMAT " points matched",
                 cpl_matrix_get_ncol(mpattern));

    cpl_array_delete(matches);
    cpl_matrix_delete(mpattern);
    cpl_matrix_delete(mdata);

    cpl_msg_info(cpl_func, "Shift data points by some vector:");

    cpl_matrix_add_scalar(rdata, -15);

    matches = cpl_ppm_match_points(rdata, 3, 1, pattern, 3, 0,
                                   0.1, 1, &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(matches);
    cpl_test_nonnull(mpattern);
    cpl_test_nonnull(mdata);
    cpl_test_eq(cpl_matrix_get_ncol(mpattern), 3);
    cpl_test_eq(cpl_array_get_size(matches), 3);

    cpl_msg_info(cpl_func, "%" CPL_SIZE_FORMAT " points matched",
                 cpl_matrix_get_ncol(mpattern));

    cpl_array_delete(matches);
    cpl_matrix_delete(mpattern);
    cpl_matrix_delete(mdata);
    cpl_matrix_delete(pattern);
    cpl_matrix_delete(rdata);

    /*
     * Now we start testing the same, with longer lists of points.
     * Matrices will still be identical (no extra points), but only
     * the first 3 points will be used for pattern matching, and the
     * rest will be recovered...
     *
     * Create a list of 8 points:
     *
     *                0 9 9 1 1 5 2 3
     *                9 0 9 0 3 4 1 7
     */

    pattern = cpl_matrix_new(2, 8);
    cpl_matrix_set(pattern, 0, 0, 0.0);
    cpl_matrix_set(pattern, 0, 1, 9.0);
    cpl_matrix_set(pattern, 0, 2, 9.0);
    cpl_matrix_set(pattern, 0, 3, 1.0);
    cpl_matrix_set(pattern, 0, 4, 1.0);
    cpl_matrix_set(pattern, 0, 5, 5.0);
    cpl_matrix_set(pattern, 0, 6, 2.0);
    cpl_matrix_set(pattern, 0, 7, 3.0);
    cpl_matrix_set(pattern, 1, 0, 9.0);
    cpl_matrix_set(pattern, 1, 1, 0.0);
    cpl_matrix_set(pattern, 1, 2, 9.0);
    cpl_matrix_set(pattern, 1, 3, 0.0);
    cpl_matrix_set(pattern, 1, 4, 3.0);
    cpl_matrix_set(pattern, 1, 5, 4.0);
    cpl_matrix_set(pattern, 1, 6, 1.0);
    cpl_matrix_set(pattern, 1, 7, 7.0);


    /*
     * Create an identical data matrix
     */

    data = cpl_matrix_duplicate(pattern);

    /*
     * Now try to match points: the transformation should be an identity
     * (rotation angle 0, scaling 0, translation 0):
     */

    cpl_msg_info(cpl_func, "Trying to match 8 identical points:");

    matches = cpl_ppm_match_points(data, 4, 1, pattern, 3, 0,
                                   0.1, 1, &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(matches);
    cpl_test_nonnull(mpattern);
    cpl_test_nonnull(mdata);
    cpl_test_eq(cpl_matrix_get_ncol(mpattern), 8);
    cpl_test_eq(cpl_array_get_size(matches), 8);

    cpl_msg_info(cpl_func, "%" CPL_SIZE_FORMAT " points matched",
                 cpl_matrix_get_ncol(mpattern));

    cpl_array_delete(matches);
    cpl_matrix_delete(mpattern);
    cpl_matrix_delete(mdata);

    cpl_msg_info(cpl_func, "Remove a point from data:");

    rdata = cpl_matrix_duplicate(data);
    cpl_matrix_erase_columns(rdata, 6, 1);

    matches = cpl_ppm_match_points(rdata, 4, 1, pattern, 3, 0,
                                   0.1, 1, &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(matches);
    cpl_test_nonnull(mpattern);
    cpl_test_nonnull(mdata);
    cpl_test_eq(cpl_matrix_get_ncol(mpattern), 7);
    cpl_test_eq(cpl_array_get_size(matches), 8);
    cpl_test_eq(cpl_array_count_invalid(matches), 1);

    cpl_msg_info(cpl_func, "%" CPL_SIZE_FORMAT " points matched",
                 cpl_matrix_get_ncol(mpattern));

    cpl_array_delete(matches);
    cpl_matrix_delete(mpattern);
    cpl_matrix_delete(mdata);
    cpl_matrix_delete(rdata);


    cpl_msg_info(cpl_func, "Rotate data points by -27 degrees:");

    rotate = cpl_matrix_new(2, 2);
    cpl_matrix_set(rotate, 0, 0, cos(-27./180.*CPL_MATH_PI));
    cpl_matrix_set(rotate, 0, 1, -sin(-27./180.*CPL_MATH_PI));
    cpl_matrix_set(rotate, 1, 0, sin(-27./180.*CPL_MATH_PI));
    cpl_matrix_set(rotate, 1, 1, cos(-27./180.*CPL_MATH_PI));

    rdata = cpl_matrix_product_create(rotate, data);

    cpl_matrix_delete(rotate);
    cpl_matrix_delete(data);

    matches = cpl_ppm_match_points(rdata, 4, 1, pattern, 3, 0,
                                   0.1, 1, &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(matches);
    cpl_test_nonnull(mpattern);
    cpl_test_nonnull(mdata);
    cpl_test_eq(cpl_matrix_get_ncol(mpattern), 8);
    cpl_test_eq(cpl_array_get_size(matches), 8);

    cpl_msg_info(cpl_func, "%" CPL_SIZE_FORMAT " points matched",
                 cpl_matrix_get_ncol(mpattern));

    cpl_matrix_delete(rdata);
    cpl_array_delete(matches);
    cpl_matrix_delete(mpattern);
    cpl_matrix_delete(mdata);
    cpl_matrix_delete(pattern);

    /*
     * At this point we use a random distribution of 100 points.
     * Only the first 70 will be written to pattern, the other 30
     * will be "false detections". And only the first 10 pattern 
     * points and the first 20 data points will be used for direct
     * pattern matching - the remaining points will be recovered
     * in the second step. The data matrix will be rotated by 95
     * degrees and rescaled by a factor 2.35.
     */

    cpl_msg_info(cpl_func, "100 random distributed data points "
                 "(70 in pattern, 30 false-detections), data rescaled "
                 "by 2.35 and rotated by 95 degrees...");

    pattern = cpl_matrix_new(2, 100);
    for (k = 0, i = 0; i < 2; i++) {
        for (j = 0; j < 100; j++, k++) {
            cpl_matrix_set(pattern, i, j, buffer[k]);
        }
    }

    data = cpl_matrix_duplicate(pattern);
    cpl_matrix_erase_columns(pattern, 70, 30);
    cpl_matrix_multiply_scalar(data, 2.35);
    rotate = cpl_matrix_new(2, 2);
    cpl_matrix_set(rotate, 0, 0, cos(95./180.*CPL_MATH_PI));
    cpl_matrix_set(rotate, 0, 1, -sin(95./180.*CPL_MATH_PI));
    cpl_matrix_set(rotate, 1, 0, sin(95./180.*CPL_MATH_PI));
    cpl_matrix_set(rotate, 1, 1, cos(95./180.*CPL_MATH_PI));

    rdata = cpl_matrix_product_create(rotate, data);

    cpl_matrix_delete(rotate);
    cpl_matrix_delete(data);

    matches = cpl_ppm_match_points(rdata, 20, 1, pattern, 10, 0,
                                   0.1, 0.1, &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(matches);
    cpl_test_nonnull(mpattern);
    cpl_test_nonnull(mdata);
    cpl_test_eq(cpl_matrix_get_ncol(mpattern), 70);
    cpl_test_eq(cpl_array_get_size(matches), 70);

    cpl_msg_info(cpl_func, "%" CPL_SIZE_FORMAT " points matched",
                 cpl_matrix_get_ncol(mpattern));

    cpl_matrix_delete(rdata);
    cpl_array_delete(matches);
    cpl_matrix_delete(mpattern);
    cpl_matrix_delete(mdata);
    cpl_matrix_delete(pattern);

    /*
     * Now with a random distribution of 20 points. This time
     * the whole data and the whole pattern (limited to 10 points)
     * are used. This means that a 50% contamination is present
     * on the data side already at the first pattern matching step.
     * The data matrix will be again rotated by 95 degrees and 
     * rescaled by a factor 2.35.
     */

    npattern = 10;
    ndata = 20;

    cpl_msg_info(cpl_func, "%d random distributed data points "
                 "(%d in pattern, %d in data, i.e., including %d "
                 "false-detections), data rescaled "
                 "by 2.35 and rotated by 95 degrees...",
                 ndata, npattern, ndata, ndata - npattern);

    pattern = cpl_matrix_new(2, ndata);
    for (k = 0, i = 0; i < 2; i++) {
        for (j = 0; j < ndata; j++, k++) {
            cpl_matrix_set(pattern, i, j, buffer[k]);
        }
    }

    data = cpl_matrix_duplicate(pattern);
    cpl_matrix_erase_columns(pattern, npattern, ndata - npattern);
    cpl_matrix_multiply_scalar(data, 2.35);
    rotate = cpl_matrix_new(2, 2);
    cpl_matrix_set(rotate, 0, 0, cos(95./180.*CPL_MATH_PI));
    cpl_matrix_set(rotate, 0, 1, -sin(95./180.*CPL_MATH_PI));
    cpl_matrix_set(rotate, 1, 0, sin(95./180.*CPL_MATH_PI));
    cpl_matrix_set(rotate, 1, 1, cos(95./180.*CPL_MATH_PI));

    rdata = cpl_matrix_product_create(rotate, data);

    cpl_matrix_delete(rotate);
    cpl_matrix_delete(data);

    matches = cpl_ppm_match_points(rdata, ndata, 1, pattern, npattern, 0,
                                   0.1, 0.1, &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(matches);
    cpl_test_nonnull(mpattern);
    cpl_test_nonnull(mdata);
    cpl_test_eq(cpl_matrix_get_ncol(mpattern), npattern);
    cpl_test_eq(cpl_array_get_size(matches), npattern);

    cpl_msg_info(cpl_func, "%" CPL_SIZE_FORMAT " points matched",
                 cpl_matrix_get_ncol(mpattern));

    cpl_matrix_delete(rdata);
    cpl_array_delete(matches);
    cpl_matrix_delete(mpattern);
    cpl_matrix_delete(mdata);
    cpl_matrix_delete(pattern);

    /*
     * Now with a random distribution of 10 points. This time
     * the pattern is larger than the data set by 5 points. 
     * This means that points are missing on the data side.
     * The data matrix will be again rotated by 95 degrees and 
     * rescaled by a factor 2.35.
     */

    npattern = 15;
    ndata = 10;
    
    cpl_msg_info(cpl_func, "%d random distributed data points "
                 "(%d in pattern, %d in data, i.e., with %d "
                 "missing detections), data rescaled "
                 "by 2.35 and rotated by 95 degrees...",
                 ndata, npattern, ndata, npattern - ndata);

    pattern = cpl_matrix_new(2, npattern);
    for (k = 0, i = 0; i < 2; i++) {
        for (j = 0; j < npattern; j++, k++) {
            cpl_matrix_set(pattern, i, j, buffer[permute[k]]);
        }
    }

    data = cpl_matrix_new(2, npattern);
    for (k = 0, i = 0; i < 2; i++) {
        for (j = 0; j < npattern; j++, k++) {
            cpl_matrix_set(data, i, j, buffer[k]);
        }
    }
    
    cpl_matrix_erase_columns(data, ndata, npattern - ndata);
    cpl_matrix_multiply_scalar(data, 2.35);
    rotate = cpl_matrix_new(2, 2);
    cpl_matrix_set(rotate, 0, 0, cos(95./180.*CPL_MATH_PI));
    cpl_matrix_set(rotate, 0, 1, -sin(95./180.*CPL_MATH_PI));
    cpl_matrix_set(rotate, 1, 0, sin(95./180.*CPL_MATH_PI));
    cpl_matrix_set(rotate, 1, 1, cos(95./180.*CPL_MATH_PI));

    rdata = cpl_matrix_product_create(rotate, data);

    cpl_matrix_delete(rotate);
    cpl_matrix_delete(data);
    
    matches = cpl_ppm_match_points(rdata, ndata, 1, pattern, npattern, 0,
                                   0.1, 0.1, &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(matches);
    cpl_test_nonnull(mpattern);
    cpl_test_nonnull(mdata);
    cpl_test_eq(cpl_matrix_get_ncol(mpattern), ndata);
    cpl_test_eq(cpl_array_get_size(matches), npattern);

    cpl_msg_info(cpl_func, "%" CPL_SIZE_FORMAT " points matched",
                 cpl_matrix_get_ncol(mpattern));

    cpl_matrix_delete(rdata);
    cpl_array_delete(matches);
    cpl_matrix_delete(mpattern);
    cpl_matrix_delete(mdata);
    cpl_matrix_delete(pattern);

    /*
     * Now with a random distribution of 20 aligned points. 
     * The whole data and the whole pattern (limited to 10 points)
     * are used. This means that a 50% contamination is present
     * on the data side already at the first pattern matching step.
     * The data matrix will be rescaled by a factor 2.35.
     */

    npattern = 10;
    ndata = 20;

    cpl_msg_info(cpl_func, "%d random distributed aligned data points "
                 "(%d in pattern, %d in data, i.e., including %d "
                 "false-detections), data rescaled by 2.35...",
                 ndata, npattern, ndata, ndata - npattern);

    pattern = cpl_matrix_new(2, ndata);
    for (j = 0; j < ndata; j++, k++)
        cpl_matrix_set(pattern, 0, j, buffer[k]);
    for (j = 0; j < ndata; j++, k++)
        cpl_matrix_set(pattern, 1, j, 0.0);

    data = cpl_matrix_duplicate(pattern);
    cpl_matrix_erase_columns(pattern, npattern, ndata - npattern);
    cpl_matrix_multiply_scalar(data, 2.35);

    matches = cpl_ppm_match_points(data, ndata, 1, pattern, npattern, 0,
                                   0.1, 0.1, &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(matches);
    cpl_test_nonnull(mpattern);
    cpl_test_nonnull(mdata);
    cpl_test_eq(cpl_matrix_get_ncol(mpattern), npattern);
    cpl_test_eq(cpl_array_get_size(matches), npattern);

    cpl_msg_info(cpl_func, "%" CPL_SIZE_FORMAT " points matched",
                 cpl_matrix_get_ncol(mpattern));

    cpl_matrix_delete(mpattern);
    cpl_matrix_delete(mdata);
    cpl_array_delete(matches);

    /* Use data also as pattern */
    matches = cpl_ppm_match_points(data, ndata, 1, data, ndata, 0,
                                   0.1, 0.1, &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(matches);
    cpl_test_nonnull(mpattern);
    cpl_test_nonnull(mdata);
    cpl_test_eq(cpl_matrix_get_ncol(mpattern), ndata);
    cpl_test_eq(cpl_array_get_size(matches), ndata);

    cpl_msg_info(cpl_func, "%" CPL_SIZE_FORMAT " points matched",
                 cpl_matrix_get_ncol(mpattern));

    cpl_matrix_delete(mpattern);
    cpl_matrix_delete(mdata);
    cpl_array_delete(matches);

    cpl_msg_info(cpl_func, "Check handling of NULL input");
    nullarray = cpl_ppm_match_points(NULL, ndata, 1, pattern, npattern, 0,
                                     0.1, 0.1, &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(nullarray);
    cpl_test_null(mpattern);
    cpl_test_null(mdata);

    nullarray = cpl_ppm_match_points(data, ndata, 1, NULL, npattern, 0,
                                     0.1, 0.1, &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(nullarray);
    cpl_test_null(mpattern);
    cpl_test_null(mdata);

    nullarray = cpl_ppm_match_points(data, ndata, 1, pattern, npattern, 0,
                                     0.1, 0.1, NULL, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nullarray);
    cpl_test_null(mpattern);

    nullarray = cpl_ppm_match_points(data, ndata, 1, pattern, npattern, 0,
                                     0.1, 0.1, &mdata, NULL, NULL, NULL);

    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nullarray);
    cpl_test_null(mdata);

    cpl_msg_info(cpl_func, "Check handling of other invalid input");

    /* ndata too small */
    nullarray = cpl_ppm_match_points(data, 2, 1, pattern, npattern, 0,
                                     0.1, 0.1, &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nullarray);
    cpl_test_null(mpattern);
    cpl_test_null(mdata);

    /* ndata too large */
    nullarray = cpl_ppm_match_points(data, ndata+1, 1, pattern, npattern, 0,
                                     0.1, 0.1, &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_test_null(nullarray);
    cpl_test_null(mpattern);
    cpl_test_null(mdata);

    /* npattern too large */
    nullarray = cpl_ppm_match_points(data, ndata, 1, pattern, npattern+1, 0,
                                     0.1, 0.1, &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_test_null(nullarray);
    cpl_test_null(mpattern);
    cpl_test_null(mdata);

    /* npattern too small */
    nullarray = cpl_ppm_match_points(data, ndata, 1, pattern, 2, 0,
                                     0.1, 0.1, &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nullarray);
    cpl_test_null(mpattern);
    cpl_test_null(mdata);

    /* err_data and err_pattern non-positive */
    nullarray = cpl_ppm_match_points(data, ndata, 0.0, pattern, npattern, 0.0,
                                     0.1, 0.1, &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nullarray);
    cpl_test_null(mpattern);
    cpl_test_null(mdata);

    /* tolerance negative */
    nullarray = cpl_ppm_match_points(data, ndata, 1.0, pattern, npattern, 0.0,
                                     -0.1, 0.1, &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nullarray);
    cpl_test_null(mpattern);
    cpl_test_null(mdata);

    /* radius negative */
    nullarray = cpl_ppm_match_points(data, ndata, 1.0, pattern, npattern, 0.0,
                                     0.1, -0.1, &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nullarray);
    cpl_test_null(mpattern);
    cpl_test_null(mdata);

    /* Too few columns in data */
    twobytwo = cpl_matrix_new(2, 2);
    matches = cpl_ppm_match_points(twobytwo, ndata, 1, pattern, npattern, 0,
                                   0.1, 0.1, &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nullarray);
    cpl_test_null(mpattern);
    cpl_test_null(mdata);

    /* Too few columns in pattern */
    matches = cpl_ppm_match_points(data, ndata, 1, twobytwo, npattern, 0,
                                   0.1, 0.1, &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nullarray);
    cpl_test_null(mpattern);
    cpl_test_null(mdata);

    cpl_matrix_delete(twobytwo);
    cpl_matrix_delete(data);
    cpl_matrix_delete(pattern);

    return cpl_test_end(0);
}
