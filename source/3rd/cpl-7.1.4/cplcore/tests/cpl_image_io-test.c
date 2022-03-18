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

#include "cpl_image_io_impl.h"
#include "cpl_test.h"
#include "cpl_fits.h"

/*-----------------------------------------------------------------------------
                                   Defines
 -----------------------------------------------------------------------------*/

#ifndef IMAGESZ
#define IMAGESZ            32
#endif

#define FILENAME "cpl_image_test.fits"

/*-----------------------------------------------------------------------------
                                  Static functions
 -----------------------------------------------------------------------------*/

static double cpl_image_get_precision(cpl_type, cpl_type, double);
static void cpl_image_fill_int_test(void);
static void cpl_image_save_compression_test(void);

/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/
int main(void)
{
    const cpl_type     imtypes[] = {CPL_TYPE_DOUBLE, CPL_TYPE_FLOAT,
                                    CPL_TYPE_INT, CPL_TYPE_DOUBLE_COMPLEX,
                                    CPL_TYPE_FLOAT_COMPLEX};
    const cpl_type bpps[] = {CPL_TYPE_UCHAR, CPL_TYPE_SHORT,
                             CPL_TYPE_USHORT, CPL_TYPE_INT,
                             CPL_TYPE_FLOAT, CPL_TYPE_DOUBLE};
    const double minval[] = {0.0,   -32767.0,     0.0, -2147483647.0,
                             -65535.0, -2147483647.0};
    const double maxval[] = {255.0,  32767.0, 65535.0,  2147483647.0,
                             65535.0, 2147483647.0};
    const int          nbad = 5;
    const int          bad_pos_x[] = {20,  20,  30,  31,  10};
    const int          bad_pos_y[] = {30,  10,  20,  20,  22};
    const int          nbpp = (int)(sizeof(bpps)/sizeof(bpps[0]));
    FILE             * stream;
    const cpl_image  * nullimg;
    int                ibpp;
    int                ityp;
    int                i;


    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    stream = cpl_msg_get_level() > CPL_MSG_INFO
        ? fopen("/dev/null", "a") : stdout;

    /* Insert tests below */

    cpl_test_nonnull(stream);

    (void)remove(FILENAME); /* May (also) exist with wrong perms */

    /* Check the image type size */
    cpl_test_eq( cpl_type_get_sizeof(CPL_TYPE_DOUBLE), sizeof(double));
    cpl_test_eq( cpl_type_get_sizeof(CPL_TYPE_FLOAT),  sizeof(float));
    cpl_test_eq( cpl_type_get_sizeof(CPL_TYPE_INT),    sizeof(int));

    cpl_test_eq( cpl_type_get_sizeof(CPL_TYPE_DOUBLE_COMPLEX), 
         sizeof(double complex));
    cpl_test_eq( cpl_type_get_sizeof(CPL_TYPE_FLOAT_COMPLEX), 
         sizeof(float complex));

    /* Check error handling in image creation */
    nullimg = cpl_image_new(0, IMAGESZ, CPL_TYPE_INT);    
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nullimg);

    nullimg = cpl_image_new(IMAGESZ, 0, CPL_TYPE_INT);    
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nullimg);

    nullimg = cpl_image_new(IMAGESZ, IMAGESZ, CPL_TYPE_CHAR);    
    cpl_test_error(CPL_ERROR_INVALID_TYPE);
    cpl_test_null(nullimg);

    nullimg = cpl_image_load(NULL, CPL_TYPE_UNSPECIFIED, 0, 0);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(nullimg);

    nullimg = cpl_image_load_window(NULL, CPL_TYPE_UNSPECIFIED, 0, 0,
                                    1, 1, 1, 1);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(nullimg);

    nullimg = cpl_image_load(FILENAME, CPL_TYPE_UNSPECIFIED, 0, 0);
    cpl_test_error(CPL_ERROR_FILE_IO);
    cpl_test_null(nullimg);

    nullimg = cpl_image_load(".", CPL_TYPE_UNSPECIFIED, 0, 0);
    cpl_test_error(CPL_ERROR_FILE_IO);
    cpl_test_null(nullimg);

    /* Error handling in the cpl_image_wrap*() functions */
    nullimg = cpl_image_wrap(1, 1, CPL_TYPE_INT, NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(nullimg);

    nullimg = cpl_image_wrap(1, 1, CPL_TYPE_POINTER, (void*)stream);
    cpl_test_error(CPL_ERROR_INVALID_TYPE);
    cpl_test_null(nullimg);

    nullimg = cpl_image_wrap(0, 1, CPL_TYPE_INT, (void*)stream);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nullimg);

    nullimg = cpl_image_wrap(1, 0, CPL_TYPE_INT, (void*)stream);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nullimg);


    nullimg = cpl_image_wrap_double(1, 1, NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(nullimg);

    nullimg = cpl_image_wrap_double(0, 1, (void*)stream);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nullimg);

    nullimg = cpl_image_wrap_double(1, 0, (void*)stream);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nullimg);

    nullimg = cpl_image_wrap_float(1, 1, NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(nullimg);

    nullimg = cpl_image_wrap_float(0, 1, (void*)stream);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nullimg);

    nullimg = cpl_image_wrap_float(1, 0, (void*)stream);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nullimg);

    nullimg = cpl_image_wrap_int(1, 1, NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(nullimg);

    nullimg = cpl_image_wrap_int(0, 1, (void*)stream);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nullimg);

    nullimg = cpl_image_wrap_int(1, 0, (void*)stream);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nullimg);

    nullimg = cpl_image_wrap_double_complex(1, 1, NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(nullimg);

    nullimg = cpl_image_wrap_double_complex(0, 1, (void*)stream);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nullimg);

    nullimg = cpl_image_wrap_double_complex(1, 0, (void*)stream);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nullimg);

    nullimg = cpl_image_wrap_float_complex(1, 1, NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(nullimg);

    nullimg = cpl_image_wrap_float_complex(0, 1, (void*)stream);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nullimg);

    nullimg = cpl_image_wrap_float_complex(1, 0, (void*)stream);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nullimg);


    cpl_image_fill_int_test();

    /* Create a data-less HDU, and append another to it */

    for (ibpp = 0; ibpp < nbpp; ibpp++) {

        cpl_msg_info(cpl_func, "Testing saving with save type '%s' and no "
                     "pixels", cpl_type_get_name(bpps[ibpp]));

        cpl_test_zero(cpl_image_save(NULL, FILENAME, bpps[ibpp],
                                     NULL, CPL_IO_CREATE));
        cpl_test_fits(FILENAME);

        cpl_test_zero( cpl_fits_count_extensions(FILENAME) );


        cpl_test_zero(cpl_image_save(NULL, FILENAME, bpps[ibpp],
                                     NULL, CPL_IO_EXTEND));
        cpl_test_fits(FILENAME);

        cpl_test_eq( cpl_fits_count_extensions(FILENAME), 1 );
    }

    /* Check error handling with no data unit */
    nullimg = cpl_image_load(FILENAME, CPL_TYPE_UNSPECIFIED, 0, 0);
    cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);
    cpl_test_null(nullimg);

    /* Iterate through all pixel types */
    for (ityp = 0; ityp < (int)(sizeof(imtypes)/sizeof(imtypes[0])); ityp++) {
        const cpl_type imtype = imtypes[ityp];
        const void * nulldata;
        cpl_error_code error;
        int ityp2;

        cpl_image * img = cpl_image_new(IMAGESZ, IMAGESZ, imtype);

        cpl_image * copy;

        cpl_msg_info(cpl_func, "Testing image with type '%s'",
                     cpl_type_get_name(imtype));

        cpl_test_nonnull(img);

        error = cpl_image_dump_structure(img, stream);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        cpl_test_eq(cpl_image_get_type(img),   imtype);
        cpl_test_eq(cpl_image_get_size_x(img), IMAGESZ);
        cpl_test_eq(cpl_image_get_size_y(img), IMAGESZ);    

        /* Test cpl_image_get_data() */
        cpl_test_nonnull( cpl_image_get_data(img) );
        cpl_test_nonnull( cpl_image_get_data_const(img) );

        if (imtype != CPL_TYPE_FLOAT_COMPLEX && 
            imtype != CPL_TYPE_DOUBLE_COMPLEX) {
            /* Largest number with 8 BPP */
            error = cpl_image_add_scalar(img, 255.0);
            cpl_test_eq_error(error, CPL_ERROR_NONE);
        }

        if (imtype == CPL_TYPE_DOUBLE) {
            cpl_test_nonnull( cpl_image_get_data_double(img) );
            cpl_test_nonnull( cpl_image_get_data_double_const(img) );

            nulldata = cpl_image_get_data_float(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null(nulldata);
            nulldata = cpl_image_get_data_float_const(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );

            nulldata = cpl_image_get_data_int(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );
            nulldata = cpl_image_get_data_int_const(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );

            nulldata = cpl_image_get_data_double_complex(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );
            nulldata = cpl_image_get_data_double_complex_const(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );

            nulldata = cpl_image_get_data_float_complex(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );
            nulldata = cpl_image_get_data_float_complex_const(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );

            copy = cpl_image_wrap_double(IMAGESZ, IMAGESZ,
                                         cpl_image_get_data_double(img));

        } else if (imtype == CPL_TYPE_FLOAT) {
            cpl_test_nonnull( cpl_image_get_data_float(img) );
            cpl_test_nonnull( cpl_image_get_data_float_const(img) );

            nulldata = cpl_image_get_data_double(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );
            nulldata = cpl_image_get_data_double_const(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );

            nulldata = cpl_image_get_data_int(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );
            nulldata = cpl_image_get_data_int_const(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );

            nulldata = cpl_image_get_data_double_complex(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );
            nulldata = cpl_image_get_data_double_complex_const(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );

            nulldata = cpl_image_get_data_float_complex(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );
            nulldata = cpl_image_get_data_float_complex_const(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );

            copy = cpl_image_wrap_float(IMAGESZ, IMAGESZ,
                                        cpl_image_get_data_float(img));

        } else if (imtype == CPL_TYPE_INT) {
            cpl_test_nonnull( cpl_image_get_data_int(img) );
            cpl_test_nonnull( cpl_image_get_data_int_const(img) );

            nulldata = cpl_image_get_data_double(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );
            nulldata = cpl_image_get_data_double_const(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );

            nulldata = cpl_image_get_data_float(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );
            nulldata = cpl_image_get_data_float_const(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );

            nulldata = cpl_image_get_data_double_complex(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );
            nulldata = cpl_image_get_data_double_complex_const(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );

            nulldata = cpl_image_get_data_float_complex(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );
            nulldata = cpl_image_get_data_float_complex_const(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );

            copy = cpl_image_wrap_int(IMAGESZ, IMAGESZ,
                                      cpl_image_get_data_int(img));

        } else if (imtype == CPL_TYPE_DOUBLE_COMPLEX) {
            cpl_test_nonnull( cpl_image_get_data_double_complex(img) );
            cpl_test_nonnull( cpl_image_get_data_double_complex_const(img) );

            nulldata = cpl_image_get_data_double(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );
            nulldata = cpl_image_get_data_double_const(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );

            nulldata = cpl_image_get_data_float(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );
            nulldata = cpl_image_get_data_float_const(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );

            nulldata = cpl_image_get_data_int(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );
            nulldata = cpl_image_get_data_int_const(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );

            nulldata = cpl_image_get_data_float_complex(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );
            nulldata = cpl_image_get_data_float_complex_const(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );

            copy = cpl_image_wrap_double_complex(
        IMAGESZ, IMAGESZ, cpl_image_get_data_double_complex(img) );

        } else { /* CPL_TYPE_FLOAT_COMPLEX */
            cpl_test_nonnull( cpl_image_get_data_float_complex(img) );
            cpl_test_nonnull( cpl_image_get_data_float_complex_const(img) );

            nulldata = cpl_image_get_data_double(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );
            nulldata = cpl_image_get_data_double_const(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );

            nulldata = cpl_image_get_data_float(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );
            nulldata = cpl_image_get_data_float_const(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );

            nulldata = cpl_image_get_data_int(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );
            nulldata = cpl_image_get_data_int_const(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );

            nulldata = cpl_image_get_data_double_complex(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );
            nulldata = cpl_image_get_data_double_complex_const(img);
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null( nulldata );

            copy = cpl_image_wrap_float_complex(
        IMAGESZ, IMAGESZ, cpl_image_get_data_float_complex(img) );
        }

        cpl_test_nonnull(copy);

        cpl_test_noneq_ptr(img, copy);

        cpl_test_eq_ptr(cpl_image_get_data_const(img),
                        cpl_image_get_data_const(copy));

        cpl_test_nonnull(cpl_image_unwrap(copy));
        copy = cpl_image_wrap(IMAGESZ, IMAGESZ, imtype,
                              cpl_image_get_data(img));

        cpl_test_nonnull(copy);

        cpl_test_noneq_ptr(img, copy);

        cpl_test_eq_ptr(cpl_image_get_data_const(img),
                        cpl_image_get_data_const(copy));

    if ( imtype != CPL_TYPE_FLOAT_COMPLEX && 
         imtype != CPL_TYPE_DOUBLE_COMPLEX)
        cpl_test_image_abs(img, copy, 0.0);
    else {
        cpl_image * real_img  = cpl_image_extract_real(img);
        cpl_image * imag_img  = cpl_image_extract_imag(img);
        cpl_image * real_copy = cpl_image_extract_real(copy);
        cpl_image * imag_copy = cpl_image_extract_imag(copy);

        cpl_test_image_abs(real_img, real_copy, 0.0);
        cpl_test_image_abs(imag_img, imag_copy, 0.0);

        /* Test cpl_image_conjugate() and cpl_image_fill_re_im() */

        error = cpl_image_conjugate(img, img);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_image_multiply_scalar(imag_copy, -1.0);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_image_fill_re_im(real_img, imag_img, img);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        cpl_test_image_abs(real_img, real_copy, 0.0);
        cpl_test_image_abs(imag_img, imag_copy, 0.0);

        error = cpl_image_fill_re_im(NULL, imag_img, img);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_image_fill_re_im(real_img, NULL, img);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        cpl_test_image_abs(real_img, real_copy, 0.0);
        cpl_test_image_abs(imag_img, imag_copy, 0.0);

        /* Test in-place */
        error = cpl_image_fill_re_im(copy, NULL, copy);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        cpl_test_image_abs(copy, real_img, 0.0);

        error = cpl_image_fill_re_im(NULL, copy, copy);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        cpl_image_delete(real_img);
        cpl_image_delete(imag_img);
        cpl_image_delete(real_copy);
        cpl_image_delete(imag_copy);

    }

        cpl_test_nonnull(cpl_image_unwrap(copy));

    /* Saving is not supported for complex types */
    if ( imtype != CPL_TYPE_FLOAT_COMPLEX && 
         imtype != CPL_TYPE_DOUBLE_COMPLEX) {
        for (ibpp = 0; ibpp < nbpp; ibpp++) {
            int j;

            cpl_msg_info(cpl_func, "Testing saving with BITPIX=%d and pixels "
                         "ranging: %g -> %g", bpps[ibpp], minval[ibpp],
                         maxval[ibpp]);

            /* Use bpp-specific value */
            cpl_test_zero(cpl_image_fill_noise_uniform(img, minval[ibpp],
                                                       maxval[ibpp]));

            /* First, do some failure testing */

            /* NULL filename - and no image */
            error = cpl_image_save(NULL, NULL, bpps[ibpp], NULL,
                                   CPL_IO_CREATE);
            cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

            /* Directory as filename - and no image */
            error = cpl_image_save(NULL, ".", bpps[ibpp], NULL,
                                   CPL_IO_CREATE);
            cpl_test_eq_error(error, CPL_ERROR_FILE_IO);

            /* Invalid mode - and no image */
            error = cpl_image_save(NULL, FILENAME, bpps[ibpp], NULL, -1);
            cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

            /* More failure tests, with an image */
            /* NULL filename */
            error = cpl_image_save(img, NULL, bpps[ibpp], NULL,
                                   CPL_IO_CREATE);
            cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

            /* Directory as filename */
            error = cpl_image_save(img, ".", bpps[ibpp], NULL,
                                   CPL_IO_CREATE);
            cpl_test_eq_error(error, CPL_ERROR_FILE_IO);

            /* Invalid BPP */
            error = cpl_image_save(img, FILENAME, 0, NULL,
                                   CPL_IO_CREATE);
            cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

            /* Invalid mode */
            error = cpl_image_save(img, FILENAME, bpps[ibpp], NULL, -1);
            cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

            /* Illegal mode combination */
            error = cpl_image_save(img, FILENAME, bpps[ibpp], NULL,
                                   CPL_IO_CREATE | CPL_IO_EXTEND);
            cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

            /* Illegal mode combination */
            error = cpl_image_save(img, FILENAME, bpps[ibpp], NULL,
                                   CPL_IO_APPEND);
            cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

            /* Create/Append file */
            error = cpl_image_save(img, FILENAME, bpps[ibpp], NULL,
                                   ibpp ? CPL_IO_EXTEND : CPL_IO_CREATE);
            cpl_test_eq_error(error, CPL_ERROR_NONE);
            cpl_test_fits(FILENAME);

            cpl_test_eq( cpl_fits_count_extensions(FILENAME), ibpp );

            error = cpl_image_save(img, FILENAME, bpps[ibpp] == CPL_TYPE_USHORT
                                   ? CPL_TYPE_SHORT : bpps[ibpp],
                                   NULL, CPL_IO_APPEND);
            cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

            cpl_test_eq( cpl_fits_count_extensions(FILENAME), ibpp );

            for (j = 0; j < 1; j++) { /* Only 1 iteration, no append */
                /* Tests for no-casting type */
                copy = cpl_image_load(FILENAME, CPL_TYPE_UNSPECIFIED,
                                      j, ibpp);
                cpl_test_error(CPL_ERROR_NONE);
                cpl_test_nonnull(copy);

                if (bpps[ibpp] == CPL_TYPE_UCHAR  ||
                    bpps[ibpp] == CPL_TYPE_SHORT   ||
                    bpps[ibpp] == CPL_TYPE_USHORT ||
                    bpps[ibpp] == CPL_TYPE_INT     ) {
                    cpl_test_eq(cpl_image_get_type(copy), CPL_TYPE_INT);
                    cpl_test_image_abs(copy, img, 0.0);
                } else if (bpps[ibpp] == CPL_TYPE_FLOAT) {
                    cpl_test_eq(cpl_image_get_type(copy), CPL_TYPE_FLOAT);
                    cpl_test_image_abs(copy, img, maxval[ibpp] * FLT_EPSILON);
                } else {
                    cpl_test_eq(cpl_image_get_type(copy), CPL_TYPE_DOUBLE);
                    cpl_test_image_abs(copy, img, maxval[ibpp] * DBL_EPSILON);
                }

                cpl_image_delete(copy);
            }

            /* Check error handling in windowed image loading */
            nullimg = cpl_image_load_window(FILENAME, CPL_TYPE_UNSPECIFIED,
                                            0, ibpp,
                                            0, IMAGESZ/4,
                                            3*IMAGESZ/4, 3*IMAGESZ/4);
            cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
            cpl_test_null(nullimg);

            nullimg = cpl_image_load_window(FILENAME, CPL_TYPE_UNSPECIFIED,
                                            0, ibpp,
                                            IMAGESZ/4, 0,
                                            3*IMAGESZ/4, 3*IMAGESZ/4);
            cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
            cpl_test_null(nullimg);

            nullimg = cpl_image_load_window(FILENAME, CPL_TYPE_UNSPECIFIED,
                                            0, ibpp,
                                            3*IMAGESZ/4, IMAGESZ/4,
                                            IMAGESZ/4, 3*IMAGESZ/4);
            cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
            cpl_test_null(nullimg);

            nullimg = cpl_image_load_window(FILENAME, CPL_TYPE_UNSPECIFIED,
                                            0, ibpp,
                                            IMAGESZ/4, 3*IMAGESZ/4,
                                            3*IMAGESZ/4, IMAGESZ/4);
            cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
            cpl_test_null(nullimg);

            nullimg = cpl_image_load_window(FILENAME, CPL_TYPE_UNSPECIFIED,
                                            0, ibpp,
                                            IMAGESZ/4, IMAGESZ/4,
                                            IMAGESZ+1, 3*IMAGESZ/4);
            cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
            cpl_test_null(nullimg);

            nullimg = cpl_image_load_window(FILENAME, CPL_TYPE_UNSPECIFIED,
                                            0, ibpp,
                                            IMAGESZ/4, IMAGESZ/4,
                                            3*IMAGESZ/4, IMAGESZ+1);
            cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
            cpl_test_null(nullimg);


            copy = cpl_image_load_window(FILENAME, CPL_TYPE_UNSPECIFIED,
                                         0, ibpp,
                                         IMAGESZ/4, IMAGESZ/4,
                                         3*IMAGESZ/4, 3*IMAGESZ/4);
            cpl_test_nonnull(copy);
            cpl_test_error(CPL_ERROR_NONE);

            if (bpps[ibpp] == CPL_TYPE_UCHAR  ||
                bpps[ibpp] == CPL_TYPE_SHORT   ||
                bpps[ibpp] == CPL_TYPE_USHORT ||
                bpps[ibpp] == CPL_TYPE_INT     )
                cpl_test_eq(cpl_image_get_type(copy), CPL_TYPE_INT);
            else if (bpps[ibpp] == CPL_TYPE_FLOAT)
                cpl_test_eq(cpl_image_get_type(copy), CPL_TYPE_FLOAT);
            else
                cpl_test_eq(cpl_image_get_type(copy), CPL_TYPE_DOUBLE);

            cpl_image_delete(copy);

            /* Load it again with casting */
            for (ityp2 = 0;
                 ityp2 < (int)(sizeof(imtypes)/sizeof(imtypes[0]) - 2);
                 ityp2++) { /* The '- 2' is to avoid complex types */
                const cpl_type imtype2 = imtypes[ityp2];

                copy = cpl_image_load(ibpp % 2 ? FILENAME : "./" FILENAME,
                                      imtype2, 0, ibpp);

                cpl_test_nonnull(copy);
                cpl_test_eq(cpl_image_get_type(copy), imtype2);

                if (imtype == imtype2) {
                    const double precision
                        = cpl_image_get_precision(imtype, bpps[ibpp],
                                                  maxval[ibpp]);
                    cpl_test_image_abs(img, copy, precision);
                }

                cpl_image_delete(copy);

                copy = cpl_image_load_window(FILENAME, imtype2, 0, ibpp,
                                             1, 1,
                                             IMAGESZ, IMAGESZ);

                cpl_test_nonnull(copy);
                cpl_test_eq(cpl_image_get_type(copy), imtype2);

                if (imtype == imtype2) {
                    const double precision
                        = cpl_image_get_precision(imtype, bpps[ibpp],
                                                  maxval[ibpp]);
                    cpl_test_image_abs(img, copy, precision);
                }

                cpl_image_delete(copy);

                copy = cpl_image_load_window(FILENAME, imtype2, 0, ibpp,
                                             IMAGESZ/4, IMAGESZ/4,
                                             3*IMAGESZ/4, 3*IMAGESZ/4);
                cpl_test_nonnull(copy);
                cpl_test_eq(cpl_image_get_type(copy), imtype2);


                cpl_test_eq(cpl_image_get_size_x(copy), 1+IMAGESZ/2);
                cpl_test_eq(cpl_image_get_size_y(copy), 1+IMAGESZ/2);

                cpl_image_delete(copy);


                copy = cpl_image_load_window(FILENAME, imtype2, 0, ibpp,
                                             IMAGESZ, IMAGESZ,
                                             IMAGESZ, IMAGESZ);

                cpl_test_nonnull(copy);
                cpl_test_eq(cpl_image_get_type(copy), imtype2);

                cpl_test_eq(cpl_image_get_size_x(copy), 1);
                cpl_test_eq(cpl_image_get_size_y(copy), 1);

                cpl_image_delete(copy);


                copy = cpl_image_load_window(FILENAME, imtype2, 0, ibpp,
                                             2, 2, 1, 1);

                cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
                cpl_test_null(copy);

        }
        }

        /* Append one data-less extension (DFS06130) */

        cpl_test_zero(cpl_image_save(NULL, FILENAME, CPL_TYPE_UCHAR,
                     NULL, CPL_IO_EXTEND));
        cpl_test_fits(FILENAME);

        /* Create overflow with 8 BPP */
        cpl_image_threshold(img, 256.0, 256.0, 256.0, 256.0);

        cpl_image_save(img, FILENAME, CPL_TYPE_UCHAR, NULL,
               CPL_IO_EXTEND);
        cpl_test_error(CPL_ERROR_FILE_IO);


        /* Create underflow with 16 BPP signed */
        cpl_image_subtract_scalar(img, (double)(1<<15) + 256.0 + 1.0);

        cpl_image_save(img, FILENAME, CPL_TYPE_SHORT, NULL,
               CPL_IO_EXTEND);
        cpl_test_error(CPL_ERROR_FILE_IO);


        /* Create overflow with 16 BPP unsigned */
        cpl_image_add_scalar(img, (double)(3<<16));

        cpl_image_save(img, FILENAME, CPL_TYPE_USHORT, NULL,
               CPL_IO_EXTEND);
        cpl_test_error(CPL_ERROR_FILE_IO);

        if (imtype != CPL_TYPE_INT) {
        /* Create overflow with 32 BPP signed */
        cpl_image_add_scalar(img, 4294967296.0);

        cpl_image_save(img, FILENAME, CPL_TYPE_INT, NULL,
                   CPL_IO_EXTEND);
        cpl_test_error(CPL_ERROR_FILE_IO);

        }

    } /* End of test on saving for non-complex types */

    /* Saving tests specific for double images */
        if (imtype == CPL_TYPE_DOUBLE) {

            /* Create overflow with 32 BPP float */
            cpl_image_add_scalar(img, 1e40);

            cpl_test_zero(cpl_image_save(img, FILENAME, CPL_TYPE_FLOAT,
                                         NULL, CPL_IO_CREATE));
            cpl_test_error(CPL_ERROR_NONE);
            cpl_test_fits(FILENAME);

            copy = cpl_image_load(FILENAME, CPL_TYPE_FLOAT, 0, 0);

            cpl_test_nonnull(copy);

            /* The file will actually be saved, with inf values all over ! */
            cpl_test_leq(cpl_image_get_max(img), cpl_image_get_min(copy));

            cpl_image_delete(copy);

        }

        /* Create a bad pixel map for this image */
        /* FIXME: Move to cpl_image_bpm-test.c */

        for (i=0; i < nbad-1; i++) 
            cpl_test_zero(cpl_image_reject(img, bad_pos_x[i], bad_pos_y[i]));

        error = cpl_image_dump_structure(img, stream);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_image_dump_window(img, bad_pos_x[0]-1, bad_pos_y[0]-1,
                                      bad_pos_x[0]+1, bad_pos_y[0]+1, stream);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        cpl_test_eq( nbad-1, cpl_image_count_rejected(img) );

        cpl_test_zero(cpl_image_reject(img, bad_pos_x[nbad-1],
                                       bad_pos_y[nbad-1]));

        cpl_test_eq( nbad, cpl_image_count_rejected(img) );

        for (i=0; i < nbad-1; i++) 
            cpl_test_zero(cpl_image_reject(img, bad_pos_x[i], bad_pos_y[i]));

        cpl_test_eq( nbad, cpl_image_count_rejected(img) );

        cpl_test_zero(cpl_image_fill_rejected(img, 0.0) );

        cpl_test_eq( cpl_image_count_rejected(img), nbad);

        /* Test cpl_image_duplicate() */
        copy = cpl_image_duplicate(img);

        cpl_test_nonnull(copy);

        cpl_test_eq( cpl_image_count_rejected(copy), nbad);

    if ( imtype != CPL_TYPE_FLOAT_COMPLEX && 
         imtype != CPL_TYPE_DOUBLE_COMPLEX)
        cpl_test_image_abs(img, copy, 0.0);
    else {
        cpl_image * real_img  = cpl_image_extract_real(img);
        cpl_image * imag_img  = cpl_image_extract_imag(img);
        cpl_image * real_copy = cpl_image_extract_real(copy);
        cpl_image * imag_copy = cpl_image_extract_imag(copy);

        cpl_test_image_abs(real_img, real_copy, 0.0);
        cpl_test_image_abs(imag_img, imag_copy, 0.0);

        /* Test cpl_image_conjugate() and cpl_image_fill_re_im() */

        error = cpl_image_conjugate(img, img);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_image_multiply_scalar(imag_copy, -1.0);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_image_fill_re_im(real_img, imag_img, img);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        cpl_test_image_abs(real_img, real_copy, 0.0);
        cpl_test_image_abs(imag_img, imag_copy, 0.0);

        /* Test in-place */
        error = cpl_image_fill_re_im(copy, NULL, copy);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        cpl_test_image_abs(copy, real_img, 0.0);

        error = cpl_image_fill_re_im(NULL, copy, copy);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        cpl_image_delete(real_img);
        cpl_image_delete(imag_img);
        cpl_image_delete(real_copy);
        cpl_image_delete(imag_copy);
    }

    cpl_image_delete(copy);


    for (ityp2 = 0; ityp2 < (int)(sizeof(imtypes)/sizeof(imtypes[0]));
         ityp2++) {
        const cpl_type imtype2 = imtypes[ityp2];

        copy = cpl_image_cast(img, imtype2);

        if (((imtype2  == CPL_TYPE_DOUBLE ||
              imtype2  == CPL_TYPE_FLOAT  ||
              imtype2  == CPL_TYPE_INT)
             &&
             (imtype == CPL_TYPE_DOUBLE_COMPLEX ||
              imtype == CPL_TYPE_FLOAT_COMPLEX))) {
            cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
            cpl_test_null(copy);
        } else {
            cpl_test_error(CPL_ERROR_NONE);
            cpl_test_nonnull(copy);

            if (imtype == imtype2) {
                cpl_image * nullimage;

                if ( imtype != CPL_TYPE_FLOAT_COMPLEX && 
                     imtype != CPL_TYPE_DOUBLE_COMPLEX)
                    cpl_test_image_abs(img, copy, 0.0);
                else {
                    cpl_image * real_img  = cpl_image_extract_real(img);
                    cpl_image * imag_img  = cpl_image_extract_imag(img);
                    cpl_image * real_copy = cpl_image_extract_real(copy);
                    cpl_image * imag_copy = cpl_image_extract_imag(copy);

                    /* Test cpl_image_conjugate() and cpl_image_fill_re_im() */

                    error = cpl_image_conjugate(img, img);
                    cpl_test_eq_error(error, CPL_ERROR_NONE);

                    error = cpl_image_multiply_scalar(imag_copy, -1.0);
                    cpl_test_eq_error(error, CPL_ERROR_NONE);

                    error = cpl_image_fill_re_im(real_img, imag_img, img);
                    cpl_test_eq_error(error, CPL_ERROR_NONE);

                    cpl_test_image_abs(real_img, real_copy, 0.0);
                    cpl_test_image_abs(imag_img, imag_copy, 0.0);

                    error = cpl_image_fill_re_im(NULL, imag_img, img);
                    cpl_test_eq_error(error, CPL_ERROR_NONE);

                    error = cpl_image_fill_re_im(real_img, NULL, img);
                    cpl_test_eq_error(error, CPL_ERROR_NONE);

                    cpl_test_image_abs(real_img, real_copy, 0.0);
                    cpl_test_image_abs(imag_img, imag_copy, 0.0);

                    /* Test in-place */
                    error = cpl_image_fill_re_im(copy, NULL, copy);
                    cpl_test_eq_error(error, CPL_ERROR_NONE);

                    cpl_test_image_abs(copy, real_img, 0.0);

                    error = cpl_image_fill_re_im(NULL, copy, copy);
                    cpl_test_eq_error(error, CPL_ERROR_NONE);

                    cpl_image_delete(real_img);
                    cpl_image_delete(imag_img);
                    cpl_image_delete(real_copy);
                    cpl_image_delete(imag_copy);
                }
                nullimage = cpl_image_cast(img, CPL_TYPE_UNSPECIFIED);
                cpl_test_error(CPL_ERROR_INVALID_TYPE);
                cpl_test_null(nullimage);
            }

            cpl_test_eq( cpl_image_count_rejected(copy), nbad);

            cpl_image_delete(copy);
        }
    }

    cpl_test_zero( cpl_image_accept_all(img));

    cpl_test_zero( cpl_image_count_rejected(img));
    
    cpl_image_delete(img);

    }

    /* Image compression tests */
    cpl_image_save_compression_test();

    cpl_test_zero( remove(FILENAME) );

    if (stream != stdout) cpl_test_zero( fclose(stream) );

    return cpl_test_end(0);
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Get the precision obtainable for the combined types
  @param  pixeltype The CPL image pixel type
  @param  bpptype   The CFITSIO pixel type to be used on disk
  @param  rel2abs The largest absolute value used in the image
  @note   Ugly, incorrect kludge for CPL_TYPE_FLOAT + CPL_TYPE_DOUBLE
 */
/*----------------------------------------------------------------------------*/
static double cpl_image_get_precision(cpl_type pixeltype, cpl_type bpptype,
                                      double rel2abs)
{

    double precision = 0.0;

    if (pixeltype == CPL_TYPE_DOUBLE) {
        switch (bpptype) {
        case CPL_TYPE_DOUBLE:
            break;
        case CPL_TYPE_FLOAT:
            precision = FLT_EPSILON * rel2abs;
            break;
        default:
            precision = 1.0;
        }
    } else if (pixeltype == CPL_TYPE_FLOAT) {
        switch (bpptype) {
        case CPL_TYPE_DOUBLE:
        case CPL_TYPE_FLOAT:
            break;
        default:
            precision = 1.0;
        }
    }

    return precision;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief      Test the CPL function
  @return    void

 */
/*----------------------------------------------------------------------------*/
static void cpl_image_fill_int_test(void)
{

    cpl_image * self = cpl_image_new(2, 3, CPL_TYPE_FLOAT);
    cpl_error_code error;

    error = cpl_image_fill_int(NULL, 0);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

    error = cpl_image_fill_int(self, 0);
    cpl_test_eq_error(error, CPL_ERROR_TYPE_MISMATCH);

    cpl_image_delete(self);
    self = cpl_image_new(2, 3, CPL_TYPE_INT);

    error = cpl_image_fill_int(self, 0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_zero(cpl_image_get_min(self));
    cpl_test_zero(cpl_image_get_max(self));

    error = cpl_image_fill_int(self, -2);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_eq(cpl_image_get_min(self), -2);
    cpl_test_eq(cpl_image_get_max(self), -2);

    cpl_image_delete(self);

}

/*----------------------------------------------------------------------------*/
/**
  @brief    Test saving with compression
  @return   void
  @see cpl_image_save()
 */
/*----------------------------------------------------------------------------*/
static void cpl_image_save_compression_test(void)
{
    /* Default CFITSIO quantization parameter for lossy floating point
       compression is q = 1, reducing precision to 4.08 % */
    const double prec = 0.0408;
    const double maxval = 255.0;
    const cpl_type imtypes[] = {CPL_TYPE_DOUBLE, CPL_TYPE_FLOAT,
                                CPL_TYPE_INT};
    const size_t   ntyp = sizeof(imtypes)/sizeof(imtypes[0]);
    size_t         ityp;

    /* Saving with different image types, bitpix and compression types */

    for (ityp = 0; ityp < ntyp; ityp++) {
        const cpl_type bpps[] = {CPL_TYPE_UCHAR, CPL_TYPE_SHORT,
                                 CPL_TYPE_USHORT, CPL_TYPE_INT,
                                 CPL_TYPE_FLOAT, CPL_TYPE_DOUBLE,
                                 CPL_TYPE_UNSPECIFIED};
        const size_t   nbpp = sizeof(bpps)/sizeof(bpps[0]);

        const cpl_type imtype = imtypes[ityp];
        cpl_image * img = cpl_image_new(IMAGESZ, IMAGESZ, imtype);
        cpl_error_code error;
        int isig;

        error = cpl_image_add_scalar(img, maxval);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        for (isig=0; isig < 2; isig++) {
            size_t         ibpp;

            for (ibpp = 0; ibpp < nbpp; ibpp++) {
                const cpl_io_type comp_meths[] = {CPL_IO_COMPRESS_GZIP, 
                                                  CPL_IO_COMPRESS_RICE, 
                                                  CPL_IO_COMPRESS_HCOMPRESS,
                                                  CPL_IO_COMPRESS_PLIO};
                const size_t ncomp = sizeof(comp_meths)/sizeof(comp_meths[0]);
                const int bitpix =
                    cpl_tools_get_bpp(bpps[ibpp] == CPL_TYPE_UNSPECIFIED
                                      ? imtype : bpps[ibpp]);
                size_t icomp;
                int ext = 0;

                error = cpl_image_save(img, FILENAME, bpps[ibpp], NULL,
                                       CPL_IO_CREATE);            
                cpl_test_eq_error(error, CPL_ERROR_NONE);

                /* Tests with compression */
                for (icomp = 0; icomp < ncomp; icomp++) {
                    const cpl_io_type comp_method = comp_meths[icomp];

                    /* The compression method flag must be non-zero */
                    cpl_test(comp_method);

                    /* Saving with compression in non supported combinations */
                    error = cpl_image_save(NULL, FILENAME, bpps[ibpp], NULL,
                                           CPL_IO_EXTEND | comp_method);
                    cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

                    /* Compression is only supported when adding new extensions, 
                     * not creating a new file and saving data in the main header */
                    error = cpl_image_save(img, FILENAME, bpps[ibpp], NULL,
                                           CPL_IO_CREATE | comp_method);
                    cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

                    for (size_t icomp2 = 0; icomp2 < icomp; icomp2++) {
                        const cpl_io_type comp_method2 = comp_meths[icomp2];

                        error = cpl_image_save(img, FILENAME, bpps[ibpp], NULL,
                                               CPL_IO_EXTEND | comp_method
                                               | comp_method2);
                        cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

                        for (size_t icomp3 = 0; icomp3 < icomp2; icomp3++) {
                            const cpl_io_type comp_method3 = comp_meths[icomp3];

                            error = cpl_image_save(img, FILENAME, bpps[ibpp], NULL,
                                                   CPL_IO_EXTEND | comp_method
                                                   | comp_method2 | comp_method3);
                            cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);
                        }

                    }

                    error = cpl_image_save(img, FILENAME, bpps[ibpp], NULL,
                                           CPL_IO_EXTEND | CPL_IO_COMPRESS_GZIP |
                                           CPL_IO_COMPRESS_RICE | 
                                           CPL_IO_COMPRESS_HCOMPRESS |
                                           CPL_IO_COMPRESS_PLIO);
                    cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);


                    error = cpl_image_save(img, FILENAME, bpps[ibpp], NULL,
                                           CPL_IO_EXTEND | comp_method);

                    if (
#ifndef CPL_IO_COMPRESSION_LOSSY
                        /* Currently, compression only allowed with integer data */
                        bitpix < 0 ||
#endif
                        /* FP-data compresses only to int, float or double */ 
                        /* RICE-compression supports only int, float or double */
                        /* In fact, this applies to all compressions... */
                        abs(bitpix) < 32) {

                        cpl_test_eq_error(error, CPL_ERROR_UNSUPPORTED_MODE);

                    } else {

                        cpl_image * img_load;

                        cpl_test_eq_error(error, CPL_ERROR_NONE);

                        cpl_test_fits(FILENAME);
                        ext++;

                        cpl_test_eq( cpl_fits_count_extensions(FILENAME), ext);

                        img_load = cpl_image_load(FILENAME,
                                                  CPL_TYPE_UNSPECIFIED,
                                                  0,  ext);

                        cpl_test_error(CPL_ERROR_NONE);
                        cpl_test_nonnull(img_load);

                        if (img_load != NULL) {

                            cpl_test_eq(cpl_image_get_size_x(img_load),
                                        IMAGESZ);
                            cpl_test_eq(cpl_image_get_size_y(img_load),
                                        IMAGESZ);

                            if (imtype == CPL_TYPE_INT) {
                                cpl_test_image_abs(img, img_load, 0.0);
                            } else {
                                cpl_test_image_abs(img, img_load,
                                                   prec * maxval);
                            }
                            cpl_image_delete(img_load);
                        }
                    }
                }
            }

            error = cpl_image_subtract_scalar(img, maxval);
            cpl_test_eq_error(error, CPL_ERROR_NONE);

            error = cpl_image_fill_noise_uniform(img, 0.0, maxval);
            cpl_test_eq_error(error, CPL_ERROR_NONE);

        }

        cpl_image_delete(img);
    }
}
