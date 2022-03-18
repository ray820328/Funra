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

/* Needed for cpl_test_abs_complex() */
#include <complex.h>

#include <cpl_test.h>

#ifdef HAVE_UNISTD_H
/* Used for sleep() */
#include <unistd.h>
#endif

/*-----------------------------------------------------------------------------
                                  Defines
 -----------------------------------------------------------------------------*/

#ifndef IMAGESZ
#define IMAGESZ 100
#endif

#define CPL_TEST_FITS_NAME "cpl_test-test.fits"

#define B2880                                                           \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"  \


/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/
int main(void)
{

    int nxfail = 0; /* The number of expected failures */
    cpl_msg_severity level;
    cpl_vector     * vec1;
    cpl_vector     * vec2;
    cpl_matrix     * mat1;
    cpl_matrix     * mat2;
    cpl_array      * arr1i;
    cpl_array      * arr2i;
    cpl_array      * arr1f;
    cpl_array      * arr2f;
    cpl_array      * arr1d;
    cpl_array      * arr2d;
    cpl_array      * arr1s;
    cpl_array      * arr2s;
    cpl_image      * img1;
    cpl_image      * img2;
    cpl_image      * img3;
    cpl_imagelist  * imglist1;
    cpl_imagelist  * imglist2;
    cpl_polynomial * poly1;
    cpl_polynomial * poly2;
    cpl_mask       * mask1;
    cpl_mask       * mask2;
    cpl_errorstate   cleanstate;
    cpl_errorstate   errstate;
    cpl_error_code   error;
    double           wallstart, cpustart;
    double           wallstop,  cpustop;
    cpl_size         tested, failed;
    FILE           * fp;
    const unsigned int tsleep = 1;
    unsigned int       tleft;

    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    wallstart = cpl_test_get_walltime();
    cpustart  = cpl_test_get_cputime();

    tested = cpl_test_get_tested();
    failed = cpl_test_get_failed();
    cpl_test_zero(tested);
    cpl_test_zero(failed);

    tested = cpl_test_get_tested();
    failed = cpl_test_get_failed();
    cpl_test_eq(tested, 2);
    cpl_test_zero(failed);

    cpl_test(41);
    cpl_test_zero(0);

    cpl_test_null(NULL);
    cpl_test_nonnull(cpl_func);

    cpl_test_eq_ptr(NULL, NULL);
    cpl_test_eq_ptr(cpl_func, cpl_func);

    cpl_test_noneq_ptr(NULL, cpl_func);
    cpl_test_noneq_ptr(cpl_func, NULL);

    cpl_test_eq(1, 1);

    cleanstate = cpl_errorstate_get();
    cpl_test_errorstate(cleanstate);

    (void)cpl_error_set(cpl_func, CPL_ERROR_EOL);

    errstate = cpl_errorstate_get();
    cpl_test_errorstate(errstate);
    cpl_test_error(CPL_ERROR_EOL);

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq_error(CPL_ERROR_NONE, CPL_ERROR_NONE);

    /* Set a CPL error */
    (void)cpl_msg_set_log_name(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);

    /* Set a CPL error */
    (void)cpl_msg_set_log_name(NULL);
    cpl_test_eq_error(CPL_ERROR_NULL_INPUT, cpl_error_get_code());

    cpl_test_noneq(0, 1);

    cpl_test_eq_string("", "");
    cpl_test_eq_string(cpl_func, cpl_func);

    cpl_test_noneq_string("", cpl_func);
    cpl_test_noneq_string("", cpl_func);

    cpl_test_abs(0.0, 0.0, 0.0);

    cpl_test_abs(0.0, 1.0, 1.0);
    cpl_test_abs(1.0, 0.0, 1.0);

    cpl_test_abs_complex(0.0, 0.0, 0.0);

    cpl_test_abs_complex(0.0, _Complex_I, 1.0);
    cpl_test_abs_complex(_Complex_I, 0.0, 1.0);

    cpl_test_rel(0.0, 0.0, 0.0);

    cpl_test_rel(1.0, 1.0, 0.0);

    cpl_test_rel(-1.0,  -2.0, 1.0);
    cpl_test_rel(-2.0,  -1.0, 1.0);

    cpl_test_leq(0.0, 0.0);
    cpl_test_lt(0.0,  1.0);

    vec1 = cpl_vector_new(IMAGESZ);
    error = cpl_vector_fill(vec1, 0.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    vec2 = cpl_vector_duplicate(vec1);
    cpl_test_vector_abs(vec1, vec2, 0.0);

    error = cpl_vector_add_scalar(vec1, 1.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_vector_abs(vec1, vec2, 1.0 + FLT_EPSILON); /* Round-off */

    cpl_test_eq(cpl_test_get_bytes_vector(vec1),
                cpl_test_get_bytes_vector(vec2));

    mat1 = cpl_matrix_new(IMAGESZ, IMAGESZ);
    error = cpl_matrix_fill(mat1, 0.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    mat2 = cpl_matrix_duplicate(mat1);
    cpl_test_matrix_abs(mat1, mat2, 0.0);

    error = cpl_matrix_add_scalar(mat1, 1.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_matrix_abs(mat1, mat2, 1.0 + FLT_EPSILON); /* Round-off */

    arr1i = cpl_array_new(IMAGESZ, CPL_TYPE_INT);
    error = cpl_array_fill_window(arr1i, 0, IMAGESZ, 0.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    arr2i = cpl_array_duplicate(arr1i);
    cpl_test_array_abs(arr1i, arr2i, 0.0);

    error = cpl_array_add_scalar(arr1i, 1.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_array_abs(arr1i, arr2i, 1.0 + FLT_EPSILON); /* Round-off */


    arr1f = cpl_array_new(IMAGESZ, CPL_TYPE_FLOAT);
    error = cpl_array_fill_window(arr1f, 0, IMAGESZ, 0.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    arr2f = cpl_array_duplicate(arr1f);
    cpl_test_array_abs(arr1f, arr2f, 0.0);

    error = cpl_array_add_scalar(arr1f, 1.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_array_abs(arr1f, arr2f, 1.0 + FLT_EPSILON); /* Round-off */


    arr1d = cpl_array_new(IMAGESZ, CPL_TYPE_DOUBLE);
    error = cpl_array_fill_window(arr1d, 0, IMAGESZ, 0.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    arr2d = cpl_array_duplicate(arr1d);
    cpl_test_array_abs(arr1d, arr2d, 0.0);

    error = cpl_array_add_scalar(arr1d, 1.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_array_abs(arr1d, arr2d, 1.0 + FLT_EPSILON); /* Round-off */

    /* Arrays with different types */
    cpl_test_array_abs(arr1d, arr2f, 1.0 + FLT_EPSILON); /* Round-off */
    cpl_test_array_abs(arr1d, arr2i, 1.0 + FLT_EPSILON); /* Round-off */
    cpl_test_array_abs(arr1f, arr2i, 1.0 + FLT_EPSILON); /* Round-off */

    arr1s = cpl_array_new(IMAGESZ, CPL_TYPE_STRING);
    cpl_test_nonnull(arr1s);
    arr2s = cpl_array_new(IMAGESZ, CPL_TYPE_STRING);
    cpl_test_nonnull(arr2s);

    /* FIXME: Add test for arrays of type string */
    cpl_test_noneq_ptr(arr1s, arr2s);

    img1 = cpl_image_fill_test_create(IMAGESZ, IMAGESZ);
    img2 = cpl_image_duplicate(img1);

    cpl_test_image_abs(img1, img2, 0.0);
    cpl_test_image_rel(img1, img2, 0.0);

    error = cpl_image_multiply_scalar(img1, 2.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_image_rel(img1, img2, 1.0);

    error = cpl_image_multiply_scalar(img1, 0.5);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_image_add_scalar(img1, 1.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_image_abs(img1, img2, 1.0 + FLT_EPSILON); /* Round-off */

    cpl_test_eq(cpl_test_get_bytes_image(img1),
                cpl_test_get_bytes_image(img2));

    imglist1 = cpl_imagelist_new();
    cpl_imagelist_set(imglist1, img1, 0);
    cpl_imagelist_set(imglist1, img2, 1);
    imglist2 = cpl_imagelist_duplicate(imglist1);

    error = cpl_imagelist_add_scalar(imglist2, 1.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_imagelist_abs(imglist1, imglist2, 1.0 + FLT_EPSILON);

    cpl_test_eq(cpl_test_get_bytes_imagelist(imglist1),
                cpl_test_get_bytes_imagelist(imglist2));

    poly1 = cpl_polynomial_new(2);
    poly2 = cpl_polynomial_duplicate(poly1);

    cpl_test_polynomial_abs(poly1, poly2, 0.0);

    mask1 = cpl_mask_new(IMAGESZ, IMAGESZ);
    mask2 = cpl_mask_new(IMAGESZ, IMAGESZ);
    error = cpl_mask_not(mask2);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_eq_mask(mask1, mask1);
    cpl_test_eq_mask(mask2, mask2);

    cpl_test_zero(cpl_test_get_bytes_vector(NULL));
    cpl_test_zero(cpl_test_get_bytes_matrix(NULL));
    cpl_test_zero(cpl_test_get_bytes_image(NULL));
    cpl_test_zero(cpl_test_get_bytes_imagelist(NULL));

    if (getenv(CPL_TEST_FITS)) {
        /* FIXME: Create real, valid FITS here ? */
    }

    failed = cpl_test_get_failed();
    cpl_test_zero(failed);

    /* Turn off messaging during testing of expected failures */

    level = cpl_msg_get_level();
    cpl_msg_set_level(CPL_MSG_OFF);

    cpl_test(0);                    nxfail++;
    cpl_test_zero(41);              nxfail++;

    cpl_test_null(cpl_func);        nxfail++;
    cpl_test_nonnull(NULL);         nxfail++;

    cpl_test_eq_ptr(NULL, cpl_func); nxfail++;
    cpl_test_eq_ptr(cpl_func, NULL);nxfail++;

    cpl_test_noneq_ptr(NULL, NULL);          nxfail++;
    cpl_test_noneq_ptr(cpl_func, cpl_func);  nxfail++;

    cpl_test_errorstate(errstate);  nxfail++;

    cpl_test_error(CPL_ERROR_EOL);  nxfail++;

    cpl_test_eq_error(CPL_ERROR_NONE, CPL_ERROR_EOL);  nxfail++;

    /* Set a CPL error */
    (void)cpl_msg_set_log_name(NULL);
    cpl_test_errorstate(cleanstate);  nxfail++;

    /* Set a CPL error */
    (void)cpl_msg_set_log_name(NULL);
    cpl_test_error(CPL_ERROR_NONE); nxfail++;

    /* Set a CPL error */
    (void)cpl_msg_set_log_name(NULL);
    cpl_test_eq_error(CPL_ERROR_NONE, cpl_error_get_code()); nxfail++;

    /* Set a CPL error */
    (void)cpl_msg_set_log_name(NULL);
    cpl_test_eq_error(CPL_ERROR_EOL, CPL_ERROR_EOL); nxfail++;

    cpl_test_eq(0, 1);              nxfail++;

    cpl_test_noneq(0, 0);           nxfail++;

    cpl_test_eq_string("A", "B");   nxfail++;
    cpl_test_eq_string(NULL, "");   nxfail++;
    cpl_test_eq_string(NULL, NULL); nxfail++;

    cpl_test_noneq_string(cpl_func, cpl_func); nxfail++;
    cpl_test_noneq_string(cpl_func, NULL);     nxfail++;
    cpl_test_noneq_string(NULL,     cpl_func); nxfail++;
    cpl_test_noneq_string(NULL,     NULL);     nxfail++;

    cpl_test_abs(0.0, 0.0, -1.0);   nxfail++;

    cpl_test_abs(0.0, 2.0, 1.0);    nxfail++;
    cpl_test_abs(2.0, 0.0, 1.0);    nxfail++;


    cpl_test_abs_complex(0.0, 0.0, -1.0);   nxfail++;

    cpl_test_abs_complex(0.0, 2.0 * _Complex_I, 1.0);    nxfail++;
    cpl_test_abs_complex(2.0 * _Complex_I, 0.0, 1.0);    nxfail++;


    cpl_test_rel(0.0, 0.0, -1.0);   nxfail++;
    cpl_test_rel(0.0, 1.0, 1.0);    nxfail++;
    cpl_test_rel(1.0, 0.0, 1.0);    nxfail++;

    cpl_test_rel(1.0, 3.0, 1.0);    nxfail++;

    cpl_test_leq(1.0, 0.0);         nxfail++;
    cpl_test_lt(0.0,  0.0);         nxfail++;

    cpl_test_vector_abs(NULL, NULL, 1.0); nxfail++;

    cpl_test_vector_abs(vec1, vec2, 0.5); nxfail++;

    cpl_test_matrix_abs(mat1, mat2, 0.5); nxfail++;

    cpl_test_array_abs(NULL, NULL, 1.0); nxfail++;

    cpl_test_array_abs(arr1i, arr2i, 0.5); nxfail++;

    cpl_test_array_abs(arr1f, arr2f, 0.5); nxfail++;

    cpl_test_array_abs(arr1d, arr2d, 0.5); nxfail++;

    cpl_test_array_abs(arr1s, arr2s, 0.0); nxfail++;

    cpl_test_image_abs(img1, NULL, 1.0); nxfail++;
    cpl_test_image_abs(NULL, img2, 1.0); nxfail++;
    cpl_test_image_rel(img1, NULL, 1.0); nxfail++;
    cpl_test_image_rel(NULL, img2, 1.0); nxfail++;

    cpl_test_image_abs(img1, img2, 0.5); nxfail++;

    cpl_test_image_rel(img1, img1, -0.5); nxfail++;

    img3 = cpl_image_duplicate(img1);

    error = cpl_image_multiply_scalar(img1, 2.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_image_rel(img1, img3, 1.0 - FLT_EPSILON); nxfail++;
    error = cpl_image_multiply_scalar(img1, 0.5);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_imagelist_abs(NULL,     NULL,     1.0); nxfail++;
    cpl_test_imagelist_abs(imglist1, NULL,     1.0); nxfail++;
    cpl_test_imagelist_abs(NULL,     imglist2, 1.0); nxfail++;
    cpl_test_imagelist_abs(imglist1, imglist2, 0.5); nxfail++;
    (void)cpl_imagelist_unset(imglist1, 0);
    cpl_test_imagelist_abs(imglist1, imglist2, 1.0 + FLT_EPSILON); nxfail++;

    cpl_test_polynomial_abs(poly1, poly2, -1.0); nxfail++;

    cpl_polynomial_delete(poly2);
    poly2 = cpl_polynomial_new(cpl_polynomial_get_dimension(poly1)+1);

    cpl_test_polynomial_abs(poly1, poly2, 0.0); nxfail++;
    cpl_test_polynomial_abs(NULL,  poly2, 0.0); nxfail++;
    cpl_test_polynomial_abs(poly1, NULL,  0.0); nxfail++;
    cpl_test_polynomial_abs(NULL,  NULL,  0.0); nxfail++;

    cpl_test_eq_mask(mask1, NULL);  nxfail++;
    cpl_test_eq_mask(NULL,  mask1); nxfail++;
    cpl_test_eq_mask(mask1, mask2); nxfail++;

    cpl_mask_delete(mask2);
    mask2 = cpl_mask_new(IMAGESZ+1, IMAGESZ);
    cpl_test_eq_mask(mask1, mask2); nxfail++;

    cpl_mask_delete(mask2);
    mask2 = cpl_mask_new(IMAGESZ, IMAGESZ+1);
    cpl_test_eq_mask(mask1, mask2); nxfail++;

    cpl_test_fits(NULL); nxfail++;
    cpl_test_fits("/dev/null"); nxfail++;

    if (getenv(CPL_TEST_FITS)) {
        cpl_test_fits("."); nxfail++;
    }

    fp = fopen(CPL_TEST_FITS_NAME, "w");
    cpl_test_nonnull(fp);
    if (fp != NULL) {
        /* This file size is non-FITS */
        const size_t size = fwrite(B2880, 1, 80, fp)
            + fwrite(B2880, 1, 2880, fp)
            + fwrite(B2880, 1, 2880, fp);

        cpl_test_zero(fclose(fp));

        cpl_test_eq(size, 80 + 2880 + 2880);

        cpl_test_fits(CPL_TEST_FITS_NAME); nxfail++;

        cpl_test_zero(remove(CPL_TEST_FITS_NAME));
    } else {
        (void)remove(CPL_TEST_FITS_NAME);
    }
    cpl_test_fits(CPL_TEST_FITS_NAME); nxfail++;

    /* Tests done - reinstate normal messaging */

    cpl_msg_set_level(level);

    cpl_msg_info(cpl_func, "Did %d failure tests", nxfail);

    failed = cpl_test_get_failed();
    cpl_test_eq(failed, nxfail);

    cpl_vector_delete(vec1);
    cpl_vector_delete(vec2);
    cpl_matrix_delete(mat1);
    cpl_matrix_delete(mat2);
    cpl_array_delete(arr1i);
    cpl_array_delete(arr2i);
    cpl_array_delete(arr1f);
    cpl_array_delete(arr2f);
    cpl_array_delete(arr1d);
    cpl_array_delete(arr2d);
    cpl_array_delete(arr1s);
    cpl_array_delete(arr2s);
    cpl_image_delete(img1);
    cpl_image_delete(img2);
    cpl_image_delete(img3);
    (void)cpl_imagelist_unset(imglist1, 0);
    cpl_imagelist_delete(imglist1);
    cpl_imagelist_delete(imglist2);
    cpl_polynomial_delete(poly1);
    cpl_polynomial_delete(poly2);
    cpl_mask_delete(mask1);
    cpl_mask_delete(mask2);

#ifdef HAVE_UNISTD_H
    tleft = sleep(tsleep);
#else
    tleft = tsleep;
#endif

    wallstop = cpl_test_get_walltime();
    cpustop  = cpl_test_get_cputime();

    cpl_test_leq(wallstart + (double)tsleep, wallstop + (double)tleft);
    cpl_test_leq(cpustart, cpustop);

    cpl_msg_info(cpl_func, "Wall-clock time [s]: %g", wallstop-wallstart);
    cpl_msg_info(cpl_func, "CPU time [s]: %g", cpustop-cpustart);

    return cpl_test_end(-nxfail);

}
