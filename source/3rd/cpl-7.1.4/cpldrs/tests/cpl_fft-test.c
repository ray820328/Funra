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

#include "cpl_fft.h"

#include "cpl_test.h"

#include "cpl_image_io_impl.h"

/*-----------------------------------------------------------------------------
                                Defines
 -----------------------------------------------------------------------------*/

#ifndef IMAGESZ
#define IMAGESZ         10
#endif

#ifndef IMAGENZ
#define IMAGENZ 5
#endif

#ifndef CONSTANT
#define CONSTANT        200
#endif

/*----------------------------------------------------------------------------*/
/**
 * @defgroup cpl_fft_test Unit tests of the CPL FFT functions
 */
/*----------------------------------------------------------------------------*/



/*-----------------------------------------------------------------------------
                            Private Function prototypes
 -----------------------------------------------------------------------------*/
static void cpl_fft_image_test(void);
#if defined CPL_FFTWF_INSTALLED || defined CPL_FFTW_INSTALLED
static void cpl_fft_image_test_one(cpl_size, cpl_size, cpl_type);
static void cpl_fft_imagelist_test_one(cpl_size, cpl_size, cpl_size, cpl_type);
static
void cpl_fft_imagelist_test_image(cpl_size, cpl_size, cpl_size, cpl_type);

static void cpl_fft_image_test_correlate(cpl_size, cpl_size, cpl_type);

#endif

/*----------------------------------------------------------------------------*/
/**
   @brief   Unit tests of cpl_fft module
**/
/*----------------------------------------------------------------------------*/

int main(void)
{
    const cpl_type imtypes[] = {CPL_TYPE_DOUBLE, CPL_TYPE_FLOAT};
    cpl_boolean do_bench;
    int i;

    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    do_bench = cpl_msg_get_level() <= CPL_MSG_INFO;

    /* Insert tests below */
#ifdef _OPENMP
#pragma omp parallel for private(i)
#endif
    for (i = 0; i < 2; i++) {
        const cpl_size mz = do_bench ? 10 * IMAGENZ : IMAGENZ;
        cpl_size nz;

        /* Collect wisdom */
#ifdef CPL_FFTWF_INSTALLED
        if (imtypes[i] == CPL_TYPE_FLOAT) {
            cpl_fft_image_test_one( 16,  16, imtypes[i]);
            cpl_fft_image_test_one(  4,  32, imtypes[i]);
            cpl_fft_image_test_one(  4,   4, imtypes[i]);
            cpl_fft_image_test_one(  2, 128, imtypes[i]);
            cpl_fft_image_test_one(128,   2, imtypes[i]);
            cpl_fft_image_test_one(128,   1, imtypes[i]);
            cpl_fft_image_test_one(  1, 128, imtypes[i]);

            cpl_fft_image_test_correlate( 16,  16, imtypes[i]);
            cpl_fft_image_test_correlate( 64, 128, imtypes[i]);
            cpl_fft_image_test_correlate(128,  64, imtypes[i]);
            cpl_fft_image_test_correlate(128, 128, imtypes[i]);

            if (do_bench) {
                cpl_fft_image_test_one(256, 256, imtypes[i]);
                cpl_fft_image_test_correlate(512, 512, imtypes[i]);
            }
        }
#endif

#ifdef CPL_FFTW_INSTALLED
        if (imtypes[i] == CPL_TYPE_DOUBLE) {
            cpl_fft_image_test_one( 16,  16, imtypes[i]);
            cpl_fft_image_test_one( 32,   4, imtypes[i]);
            cpl_fft_image_test_one(  4,   4, imtypes[i]);
            cpl_fft_image_test_one(  2, 128, imtypes[i]);
            cpl_fft_image_test_one(128,   2, imtypes[i]);
            cpl_fft_image_test_one(128,   1, imtypes[i]);
            cpl_fft_image_test_one(  1, 128, imtypes[i]);

            cpl_fft_image_test_correlate( 16,  16, imtypes[i]);
            cpl_fft_image_test_correlate( 64, 128, imtypes[i]);
            cpl_fft_image_test_correlate(128,  64, imtypes[i]);
            cpl_fft_image_test_correlate(128, 128, imtypes[i]);

            if (do_bench) {
                cpl_fft_image_test_one(256, 256, imtypes[i]);
                cpl_fft_image_test_correlate(512, 512, imtypes[i]);
            }
        }
#endif



        for (nz = 1; nz <= 1 + mz; nz+= mz) {
#ifdef CPL_FFTWF_INSTALLED
            if (imtypes[i] == CPL_TYPE_FLOAT) {
                cpl_fft_imagelist_test_image( 16,  16, nz, imtypes[i]);
                cpl_fft_imagelist_test_image(  4,  32, nz, imtypes[i]);
                cpl_fft_imagelist_test_image(  4,   4, nz, imtypes[i]);
                cpl_fft_imagelist_test_image(  2, 128, nz, imtypes[i]);
                cpl_fft_imagelist_test_image(128,   2, nz, imtypes[i]);
                cpl_fft_imagelist_test_image(128,   1, nz, imtypes[i]);
                cpl_fft_imagelist_test_image(  1, 128, nz, imtypes[i]);

                if (do_bench) {
                    cpl_fft_imagelist_test_image(256, 256, nz, imtypes[i]);
                }
            }
#endif

#ifdef CPL_FFTW_INSTALLED
            if (imtypes[i] == CPL_TYPE_DOUBLE) {
                cpl_fft_imagelist_test_image( 16,  16, nz, imtypes[i]);
                cpl_fft_imagelist_test_image( 32,   4, nz, imtypes[i]);
                cpl_fft_imagelist_test_image(  4,   4, nz, imtypes[i]);
                cpl_fft_imagelist_test_image(  2, 128, nz, imtypes[i]);
                cpl_fft_imagelist_test_image(128,   2, nz, imtypes[i]);
                cpl_fft_imagelist_test_image(128,   1, nz, imtypes[i]);
                cpl_fft_imagelist_test_image(  1, 128, nz, imtypes[i]);

                if (do_bench) {
                    cpl_fft_imagelist_test_image(256, 256, nz, imtypes[i]);
                }
            }
#endif
        }
    }

    cpl_fft_image_test();

    /* End of tests */
    return cpl_test_end(0);
}


/*----------------------------------------------------------------------------*/
/**
   @internal
   @brief  Unit tests of the function
   @see cpl_fft_image()
**/
/*----------------------------------------------------------------------------*/

static void cpl_fft_image_test(void)
{
    const cpl_type imtypes[] = {CPL_TYPE_DOUBLE, CPL_TYPE_FLOAT,
                                CPL_TYPE_INT, CPL_TYPE_DOUBLE_COMPLEX,
                                CPL_TYPE_FLOAT_COMPLEX};
    int            ityp;
    int            nok = 0; /* Number of successful calls */

    /* Insert tests below */

    /* Iterate through all pixel types */
    for (ityp = 0; ityp < (int)(sizeof(imtypes)/sizeof(imtypes[0])); ityp++) {
        const cpl_type imtype = imtypes[ityp];

        int ityp2;

        cpl_image * img1 = cpl_image_new(IMAGESZ, IMAGESZ, imtype);
        cpl_image * img3 = cpl_image_new(IMAGESZ, IMAGESZ, imtype);
        cpl_error_code error;

        /* Various error checks */
        error = cpl_fft_image(img3, NULL, CPL_FFT_FORWARD);
        cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

        error = cpl_fft_image(NULL, img3, CPL_FFT_FORWARD);
        cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

        error = cpl_fft_image(img3, img3, CPL_FFT_FORWARD | CPL_FFT_BACKWARD);
        if (imtype & CPL_TYPE_COMPLEX) {
            if (imtype & CPL_TYPE_DOUBLE) {
#ifdef CPL_FFTW_INSTALLED
                cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);
#else
                cpl_test_eq_error(error, CPL_ERROR_UNSUPPORTED_MODE);
#endif
            } else if (imtype & CPL_TYPE_FLOAT) {
#ifdef CPL_FFTWF_INSTALLED
                cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);
#else
                cpl_test_eq_error(error, CPL_ERROR_UNSUPPORTED_MODE);
#endif
            } else {
                cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);
            }
        } else if (imtype == CPL_TYPE_DOUBLE) {
#ifdef CPL_FFTW_INSTALLED
            cpl_test_eq_error(error, CPL_ERROR_TYPE_MISMATCH);
#else
            cpl_test_eq_error(error, CPL_ERROR_UNSUPPORTED_MODE);
#endif
        } else if (imtype == CPL_TYPE_FLOAT) {
#ifdef CPL_FFTWF_INSTALLED
            cpl_test_eq_error(error, CPL_ERROR_TYPE_MISMATCH);
#else
            cpl_test_eq_error(error, CPL_ERROR_UNSUPPORTED_MODE);
#endif
        } else {
            cpl_test_eq_error(error, CPL_ERROR_TYPE_MISMATCH);
        }

        if (!(imtype & CPL_TYPE_COMPLEX)) {
            error = cpl_image_fill_noise_uniform(img1, 0, CONSTANT);
            cpl_test_eq_error(error, CPL_ERROR_NONE);
        }

        for (ityp2 = 0; ityp2 < (int)(sizeof(imtypes)/sizeof(imtypes[0]));
             ityp2++) {
            const cpl_type imtype2 = imtypes[ityp2];
            cpl_image * img2 = cpl_image_new(IMAGESZ, IMAGESZ, imtype2);
            const cpl_image * imgin = img3;
            cpl_image * imgout = img2;
            int idir;
            /* No scaling on the forward transform has no effect */
            unsigned mode = CPL_FFT_FORWARD | CPL_FFT_NOSCALE;


            error = cpl_image_copy(img3, img1, 1, 1);
            cpl_test_eq_error(error, CPL_ERROR_NONE);

            /* Transform first forward, then backward */
            /* Those two iterations will succeed iff the input image
               and output image have matching non-integer precision */

            for (idir = 0; idir < 2; idir++, mode = CPL_FFT_BACKWARD,
                     imgin = img2, imgout = img3) {

                error = cpl_fft_image(imgout, imgin, mode);

                if (cpl_image_get_type(img3) == CPL_TYPE_FLOAT &&
                           cpl_image_get_type(img2) ==
                           (CPL_TYPE_FLOAT | CPL_TYPE_COMPLEX)) {
#ifdef CPL_FFTWF_INSTALLED
                    cpl_test_eq_error(CPL_ERROR_NONE, error);
                    nok++;

                    if (mode == CPL_FFT_BACKWARD) {
                        /* Transformed forward and backwards, so the result
                           should equal the original input */
                        cpl_test_image_abs(img1, img3,
                                           3.0 * FLT_EPSILON * CONSTANT);
                    }
#else
                    cpl_test_eq_error(CPL_ERROR_UNSUPPORTED_MODE, error);
#endif
                } else if (cpl_image_get_type(img3) == CPL_TYPE_DOUBLE &&
                           cpl_image_get_type(img2) ==
                           (CPL_TYPE_DOUBLE | CPL_TYPE_COMPLEX)) {
#ifdef CPL_FFTW_INSTALLED
                    cpl_test_eq_error(CPL_ERROR_NONE, error);
                    nok++;

                    if (mode == CPL_FFT_BACKWARD) {
                        /* Transformed forward and backwards, so the result
                           should equal the original input */
                        cpl_test_image_abs(img1, img3,
                                           5.0 * DBL_EPSILON * CONSTANT);
                    }
#else
                    cpl_test_eq_error(CPL_ERROR_UNSUPPORTED_MODE, error);
#endif

                } else if (cpl_image_get_type(img3) ==
                           (CPL_TYPE_DOUBLE | CPL_TYPE_COMPLEX) &&
                           cpl_image_get_type(img2) ==
                           (CPL_TYPE_DOUBLE | CPL_TYPE_COMPLEX)) {
#ifdef CPL_FFTW_INSTALLED
                    cpl_test_eq_error(CPL_ERROR_NONE, error);
#else
                    cpl_test_eq_error(CPL_ERROR_UNSUPPORTED_MODE, error);
#endif
                } else if (cpl_image_get_type(img3) ==
                           (CPL_TYPE_FLOAT | CPL_TYPE_COMPLEX) &&
                           cpl_image_get_type(img2) ==
                           (CPL_TYPE_FLOAT | CPL_TYPE_COMPLEX)) {
#ifdef CPL_FFTWF_INSTALLED
                    cpl_test_eq_error(CPL_ERROR_NONE, error);
#else
                    cpl_test_eq_error(CPL_ERROR_UNSUPPORTED_MODE, error);
#endif
                } else if ((imtype & CPL_TYPE_INT) ||
                           (imtype2 & CPL_TYPE_INT)) {
                    cpl_test_eq_error(CPL_ERROR_TYPE_MISMATCH, error);
                } else if ((imtype  & (CPL_TYPE_FLOAT | CPL_TYPE_DOUBLE | CPL_TYPE_INT)) !=
                           (imtype2 & (CPL_TYPE_FLOAT | CPL_TYPE_DOUBLE | CPL_TYPE_INT))) {
                    cpl_test_eq_error(CPL_ERROR_TYPE_MISMATCH, error);
                } else if (!((imtype  & CPL_TYPE_COMPLEX) ^
                             (imtype2 & CPL_TYPE_COMPLEX))) {
                    /* None or both are complex */
                    if (imtype == CPL_TYPE_DOUBLE) {
#ifdef CPL_FFTW_INSTALLED
                        cpl_test_eq_error(CPL_ERROR_TYPE_MISMATCH, error);
#else
                        cpl_test_eq_error(CPL_ERROR_UNSUPPORTED_MODE, error);
#endif
                    } else if (imtype == CPL_TYPE_FLOAT) {
#ifdef CPL_FFTWF_INSTALLED
                        cpl_test_eq_error(CPL_ERROR_TYPE_MISMATCH, error);
#else
                        cpl_test_eq_error(CPL_ERROR_UNSUPPORTED_MODE, error);
#endif
                    } else {
                        cpl_test_eq_error(CPL_ERROR_TYPE_MISMATCH, error);
                    }
                } else if (imtype & CPL_TYPE_DOUBLE) {
#ifdef CPL_FFTW_INSTALLED
                        cpl_test_eq_error(CPL_ERROR_TYPE_MISMATCH, error);
#else
                        cpl_test_eq_error(CPL_ERROR_UNSUPPORTED_MODE, error);
#endif
                } else if (imtype & CPL_TYPE_FLOAT) {
#ifdef CPL_FFTWF_INSTALLED
                        cpl_test_eq_error(CPL_ERROR_TYPE_MISMATCH, error);
#else
                        cpl_test_eq_error(CPL_ERROR_UNSUPPORTED_MODE, error);
#endif
                } else {
                    cpl_test_eq_error(CPL_ERROR_TYPE_MISMATCH, error);
                }
            }
            cpl_image_delete(img2);
        }
        cpl_image_delete(img1);
        cpl_image_delete(img3);
    }
#if defined CPL_FFTWF_INSTALLED && defined CPL_FFTW_INSTALLED
    cpl_test_eq(nok, 4); /* Forward and backward of float and double */
#elif defined CPL_FFTWF_INSTALLED
    cpl_msg_warning(cpl_func, "Double precision FFT not available for "
                    "unit testing");
    cpl_test_eq(nok, 2); /* Forward and backward of type float */
#elif defined CPL_FFTW_INSTALLED
    cpl_msg_warning(cpl_func, "Single precision FFT not available for "
                    "unit testing");
    cpl_test_eq(nok, 2); /* Forward and backward of type double */
#else
    cpl_msg_warning(cpl_func, "FFT not available for unit testing");
    cpl_test_zero(nok);
#endif

}

#if defined CPL_FFTWF_INSTALLED || defined CPL_FFTW_INSTALLED

/*----------------------------------------------------------------------------*/
/**
   @internal
   @brief Unit tests of the function
   @param nx   Size in x (the number of columns)
   @param ny   Size in y (the number of rows)
   @param type One of CPL_TYPE_DOUBLE or CPL_TYPE_FLOAT
   @see cpl_fft_image()
**/
/*----------------------------------------------------------------------------*/

static void cpl_fft_image_test_one(cpl_size nx, cpl_size ny, cpl_type type)
{

    const int      rigor   = CPL_FFT_FIND_MEASURE;
    cpl_image    * image1r = cpl_image_new(nx, ny, type);
    cpl_image    * image1c;
    cpl_image    * image2  = cpl_image_new(nx, ny, type);
    cpl_image    * image3r = cpl_image_new(nx, ny, type | CPL_TYPE_COMPLEX);
    cpl_image    * image3c = cpl_image_new(nx, ny, type | CPL_TYPE_COMPLEX);
    cpl_image    * image3h = cpl_image_new(nx/2+1, ny, type | CPL_TYPE_COMPLEX);
    cpl_image    * image4;
    cpl_image    * image4r;
    cpl_image    * image4c;
    cpl_image    * image5  = cpl_image_new(nx, ny, type);
    cpl_error_code error;

    error = cpl_image_fill_noise_uniform(image1r, 0.0, 1.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    image1c = cpl_image_cast(image1r, type | CPL_TYPE_COMPLEX);

    /* Real-to-complex, both full size */
    error = cpl_fft_image(image3r, image1r, CPL_FFT_FORWARD | rigor);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    /* Extract half of r2c transform */
    image4 = cpl_image_extract(image3r, 1, 1, nx/2 + 1, ny);

    /* Real-to-complex, complex is half size */
    error = cpl_fft_image(image3h, image1r, CPL_FFT_FORWARD | rigor);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    /* That half has to match the transform onto the half-sized image */
    cpl_test_image_abs(image3h, image4, 80.0 * (type == CPL_TYPE_DOUBLE ?
                       DBL_EPSILON : FLT_EPSILON));

    /* Complex-to-complex of same real values */
    error = cpl_fft_image(image3c, image1c, CPL_FFT_FORWARD | rigor);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    /* In-place complex-to-complex of same real values */
    error = cpl_fft_image(image1c, image1c, CPL_FFT_FORWARD | rigor);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_image_abs(image3c, image1c, type == CPL_TYPE_DOUBLE ?
                       128.0 * DBL_EPSILON : 40.0 * FLT_EPSILON);

    /* Extract half of c2c transform */
    cpl_image_delete(image4);
    image4 = cpl_image_extract(image3c, 1, 1, nx/2 + 1, ny);

    cpl_test_image_abs(image3h, image4, 128.0 * nx *
                       (type == CPL_TYPE_DOUBLE ? DBL_EPSILON : FLT_EPSILON));

    /* Complex-to-real, both full size */
    error = cpl_fft_image(image2, image3r, CPL_FFT_BACKWARD | rigor);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    /* The back-transformed must match the original image */
    cpl_test_image_abs(image1r, image2, 6.0 * (type == CPL_TYPE_DOUBLE ?
                                               DBL_EPSILON : FLT_EPSILON));

    /* Complex-to-real, complex is half size */
    error = cpl_fft_image(image2, image3h, CPL_FFT_BACKWARD | rigor);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    /* The back-transformed must match the original image */
    cpl_test_image_abs(image1r, image2, 6.0 * (type == CPL_TYPE_DOUBLE ?
                                               DBL_EPSILON : FLT_EPSILON));

    /* Complex-to-complex of same real values */
    error = cpl_fft_image(image3r, image3c, CPL_FFT_BACKWARD | rigor);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    /* In-place complex-to-complex of same real values */
    error = cpl_fft_image(image3c, image3c, CPL_FFT_BACKWARD | rigor);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_image_abs(image3r, image3c, 3.2 * (type == CPL_TYPE_DOUBLE ?
                       DBL_EPSILON : FLT_EPSILON));

    /* The back-transformed must match the original image - on the real part */
    image4r = cpl_image_extract_real(image3r);

    cpl_test_image_abs(image1r, image4r, 6.0 * (type == CPL_TYPE_DOUBLE ?
                                                DBL_EPSILON : FLT_EPSILON));

    /* The back-transformed must have a zero-valued imaginary part */
    image4c = cpl_image_extract_imag(image3r);
    cpl_image_delete(image4);
    image4 = cpl_image_new(nx, ny, type);

    cpl_test_image_abs(image4c, image4, 2.0 * (type == CPL_TYPE_DOUBLE ?
                                               DBL_EPSILON : FLT_EPSILON));

    cpl_image_delete(image1r);
    cpl_image_delete(image1c);
    cpl_image_delete(image2);
    cpl_image_delete(image3r);
    cpl_image_delete(image3c);
    cpl_image_delete(image3h);
    cpl_image_delete(image4);
    cpl_image_delete(image4r);
    cpl_image_delete(image4c);
    cpl_image_delete(image5);

}

/*----------------------------------------------------------------------------*/
/**
   @internal
   @brief Unit tests of the function
   @param nx   Size in x (the number of columns)
   @param ny   Size in y (the number of rows)
   @param nz   Size in z (the number of planes/images)
   @param type One of CPL_TYPE_DOUBLE or CPL_TYPE_FLOAT
   @see cpl_fft_image()
**/
/*----------------------------------------------------------------------------*/

static void cpl_fft_imagelist_test_one(cpl_size nx, cpl_size ny, cpl_size nz,
                                       cpl_type type)
{
    const int       rigor   = CPL_FFT_FIND_MEASURE;
    cpl_imagelist * ilist1r = cpl_imagelist_new();
    cpl_imagelist * ilist1c = cpl_imagelist_new();
    cpl_imagelist * ilist2  = cpl_imagelist_new();
    cpl_imagelist * ilist3r = cpl_imagelist_new();
    cpl_imagelist * ilist3c = cpl_imagelist_new();
    cpl_imagelist * ilist3h = cpl_imagelist_new();
    cpl_imagelist * ilist4  = cpl_imagelist_new();
    cpl_imagelist * ilist4r = cpl_imagelist_new();
    cpl_imagelist * ilist4c = cpl_imagelist_new();
    cpl_imagelist * ilistr  = cpl_imagelist_new();
    cpl_imagelist * ilistc  = cpl_imagelist_new();
    cpl_imagelist * ilist5  = cpl_imagelist_new();

    cpl_error_code error;
    cpl_size       i;

    for (i = 0; i < nz; i++) {
        cpl_image    * image1r = cpl_image_new(nx, ny, type);
        cpl_image    * image1c;
        cpl_image    * image2  = cpl_image_new(nx, ny, type);
        cpl_image    * image3r = cpl_image_new(nx, ny, type | CPL_TYPE_COMPLEX);
        cpl_image    * image3c;
        cpl_image    * image3h = cpl_image_new(nx/2+1, ny, type
                                               | CPL_TYPE_COMPLEX);
        cpl_image    * image5  = cpl_image_new(nx, ny, type);

        error = cpl_image_fill_noise_uniform(image1r, 0.0, 1.0);
        cpl_test_eq_error(error, CPL_ERROR_NONE);
        error = cpl_imagelist_set(ilist1r, image1r, i);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        image1c = cpl_image_cast(image1r, type | CPL_TYPE_COMPLEX);
        error = cpl_imagelist_set(ilist1c, image1c, i);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_imagelist_set(ilist2 , image2,  i);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_imagelist_set(ilist3r, image3r, i);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        image3c = cpl_image_new(nx, ny, type | CPL_TYPE_COMPLEX);
        error = cpl_imagelist_set(ilist3c, image3c, i);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_imagelist_set(ilist3h, image3h, i);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_imagelist_set(ilist5,  image5,  i);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

    }


    /* Real-to-complex, both full size */
    error = cpl_fft_imagelist(ilist3r, ilist1r, CPL_FFT_FORWARD | rigor);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    /* Extract half of r2c transform */
    for (i = 0; i < nz; i++) {
        const cpl_image * image3r = cpl_imagelist_get_const(ilist3r, i);
        cpl_image       * image4  = cpl_image_extract(image3r, 1, 1,
                                                      nx/2 + 1, ny);

        error = cpl_imagelist_set(ilist4, image4, i);
        cpl_test_eq_error(error, CPL_ERROR_NONE);
    }

    /* Real-to-complex, complex is half size */
    error = cpl_fft_imagelist(ilist3h, ilist1r, CPL_FFT_FORWARD | rigor);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    /* That half has to match the transform onto the half-sized image */
    cpl_test_imagelist_abs(ilist3h, ilist4, 80.0 * (type == CPL_TYPE_DOUBLE ?
                       DBL_EPSILON : FLT_EPSILON));

    /* Complex-to-complex of same real values */
    error = cpl_fft_imagelist(ilist3c, ilist1c, CPL_FFT_FORWARD | rigor);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    /* In-place complex-to-complex of same real values */
    error = cpl_fft_imagelist(ilist1c, ilist1c, CPL_FFT_FORWARD | rigor);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_imagelist_abs(ilist3c, ilist1c, 2.0 * (nx + ny) *
                           (type == CPL_TYPE_DOUBLE ?
                            DBL_EPSILON : FLT_EPSILON));

    /* Extract half of c2c transform */
    cpl_imagelist_empty(ilist4);
    for (i = 0; i < nz; i++) {
        const cpl_image * image3c = cpl_imagelist_get_const(ilist3c, i);
        cpl_image       * image4  = cpl_image_extract(image3c, 1, 1,
                                                      nx/2 + 1, ny);

        error = cpl_imagelist_set(ilist4, image4, i);
        cpl_test_eq_error(error, CPL_ERROR_NONE);
    }

    cpl_test_imagelist_abs(ilist3h, ilist4, 128.0 * nx *
                       (type == CPL_TYPE_DOUBLE ? DBL_EPSILON : FLT_EPSILON));

    /* Complex-to-real, both full size */
    error = cpl_fft_imagelist(ilist2, ilist3r, CPL_FFT_BACKWARD | rigor);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    /* The back-transformed must match the original image */
    cpl_test_imagelist_abs(ilist1r, ilist2, 6.0 * (type == CPL_TYPE_DOUBLE ?
                                                   DBL_EPSILON : FLT_EPSILON));

    /* Complex-to-real, complex is half size */
    error = cpl_fft_imagelist(ilist2, ilist3h, CPL_FFT_BACKWARD | rigor);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    /* The back-transformed must match the original image */
    cpl_test_imagelist_abs(ilist1r, ilist2, 6.0 * (type == CPL_TYPE_DOUBLE ?
                                                   DBL_EPSILON : FLT_EPSILON));

    /* Complex-to-complex of same real values */
    error = cpl_fft_imagelist(ilist3r, ilist3c, CPL_FFT_BACKWARD | rigor);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    /* In-place complex-to-complex of same real values */
    error = cpl_fft_imagelist(ilist3c, ilist3c, CPL_FFT_BACKWARD | rigor);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_imagelist_abs(ilist3r, ilist3c, 8.0 * (type == CPL_TYPE_DOUBLE ?
                                                    DBL_EPSILON : FLT_EPSILON));

    /* The back-transformed must match the original image - on the real part */
    /* - and the back-transformed must have a zero-valued imaginary part */

    cpl_imagelist_empty(ilist4);
    for (i = 0; i < nz; i++) {
        const cpl_image * image3r = cpl_imagelist_get_const(ilist3r, i);
        cpl_image       * image4r = cpl_image_extract_real(image3r);
        cpl_image       * image4c = cpl_image_extract_imag(image3r);
        cpl_image       * image4  = cpl_image_new(nx, ny, type);


        error = cpl_imagelist_set(ilist4r, image4r, i);
        cpl_test_eq_error(error, CPL_ERROR_NONE);
        error = cpl_imagelist_set(ilist4c, image4c, i);
        cpl_test_eq_error(error, CPL_ERROR_NONE);
        error = cpl_imagelist_set(ilist4, image4, i);
        cpl_test_eq_error(error, CPL_ERROR_NONE);
    }

    cpl_test_imagelist_abs(ilist1r, ilist4r, 6.0 * (type == CPL_TYPE_DOUBLE ?
                                                    DBL_EPSILON : FLT_EPSILON));

    cpl_test_imagelist_abs(ilist4c, ilist4, 2.0 * (type == CPL_TYPE_DOUBLE ?
                                                   DBL_EPSILON : FLT_EPSILON));

    cpl_imagelist_delete(ilist1r);
    cpl_imagelist_delete(ilist1c);
    cpl_imagelist_delete(ilist2);
    cpl_imagelist_delete(ilist3r);
    cpl_imagelist_delete(ilist3c);
    cpl_imagelist_delete(ilist3h);
    cpl_imagelist_delete(ilist4);
    cpl_imagelist_delete(ilist4r);
    cpl_imagelist_delete(ilist4c);
    cpl_imagelist_delete(ilistr);
    cpl_imagelist_delete(ilistc);
    cpl_imagelist_delete(ilist5);


}

/*----------------------------------------------------------------------------*/
/**
   @internal
   @brief Benchmark cpl_fft_imagelist() aginst cpl_fft_image()
   @param nx   Size in x (the number of columns)
   @param ny   Size in y (the number of rows)
   @param nz   Size in z (the number of planes/images)
   @param type One of CPL_TYPE_DOUBLE or CPL_TYPE_FLOAT
   @see cpl_fft_imagelist_test_one() cpl_fft_image_test_one()
**/
/*----------------------------------------------------------------------------*/
static void cpl_fft_imagelist_test_image(cpl_size nx, cpl_size ny, cpl_size nz,
                                         cpl_type type)
{

    cpl_flops flopl0, flopl1, flopi0, flopi1;
    double timel0, timel1, timei0, timei1;
    cpl_size i;


    flopl0 = cpl_test_get_flops();
    timel0 = cpl_test_get_cputime();

    cpl_fft_imagelist_test_one(nx, ny, nz, type);

    flopl1 = cpl_test_get_flops() - flopl0;
    timel1 = cpl_test_get_cputime() - timel0;


    flopi0 = cpl_test_get_flops();
    timei0 = cpl_test_get_cputime();

    for (i = 0; i < nz; i++) {
        cpl_fft_image_test_one(nx, ny, type);
    }

    flopi1 = cpl_test_get_flops() - flopi0;
    timei1 = cpl_test_get_cputime() - timei0;

    if (timei1 > 0.0 && timel1 > 0.0) {
        cpl_msg_info(cpl_func, "List vs single %d X %d X %d (%s): %g <=> %g "
                     "[s] (%g <=> %g [MFLOP/s])", (int)nx, (int)ny,
                     (int)nz, cpl_type_get_name(type), timel1, timei1,
                     1e-6*(double)flopl1/timel1,
                     1e-6*(double)flopi1/timei1);
    } else {
        cpl_msg_info(cpl_func, "List vs single %d X %d X %d (%s): %g <=> %g "
                     "[s] (%g <=> %g [MFLOP])", (int)nx, (int)ny,
                     (int)nz, cpl_type_get_name(type), timel1, timei1,
                     1e-6*(double)flopl1, 1e-6*(double)flopi1);
    }
}



/*----------------------------------------------------------------------------*/
/**
   @internal
   @brief Try to use the FFT for correlation
   @param nx   Size in x (the number of columns)
   @param ny   Size in y (the number of rows)
   @param type One of CPL_TYPE_DOUBLE or CPL_TYPE_FLOAT
   @see cpl_fft_image_test_one()
**/
/*----------------------------------------------------------------------------*/
static
void cpl_fft_image_test_correlate(cpl_size nx, cpl_size ny, cpl_type type)
{

    cpl_image * ia = cpl_image_new(nx, ny, type);
    cpl_image * ib = cpl_image_new(nx, ny, type);
    cpl_image * ic = cpl_image_new(nx, ny, type);
    cpl_image * fa = cpl_image_new(nx, ny, type | CPL_TYPE_COMPLEX);
    cpl_image * fb = cpl_image_new(nx, ny, type | CPL_TYPE_COMPLEX);
    cpl_image * fc = cpl_image_new(nx, ny, type | CPL_TYPE_COMPLEX);
    cpl_imagelist * iab = cpl_imagelist_new();
    cpl_imagelist * fab = cpl_imagelist_new();
    cpl_size xmax, ymax;
    cpl_error_code code;

    code = cpl_imagelist_set(iab, ia, 0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_imagelist_set(iab, ib, 1);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_imagelist_set(fab, fa, 0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_imagelist_set(fab, fb, 1);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_image_fill_gaussian(ia, nx/2.0, ny/2.0, 1.0, 1.0, 1.0);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_image_copy(ib, ia, 1, 1);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_image_shift(ib, nx/4, ny/4);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_fft_imagelist(fab, iab, CPL_FFT_FORWARD);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    /* Auto-correlate */
    code = cpl_image_conjugate(fc, fa);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_image_multiply(fc, fa);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_fft_image(ic, fc, CPL_FFT_BACKWARD | CPL_FFT_NOSCALE);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_image_get_maxpos(ic, &xmax, &ymax);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_eq(xmax, 1);
    cpl_test_eq(ymax, 1);

    /* Cross-correlate */
    code = cpl_image_conjugate(fc, fb);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_image_multiply(fc, fa);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_fft_image(ic, fc, CPL_FFT_BACKWARD | CPL_FFT_NOSCALE);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    code = cpl_image_get_maxpos(ic, &xmax, &ymax);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    cpl_test_eq(xmax, 1 + nx/2 + nx/4);
    cpl_test_eq(ymax, 1 + ny/2 + ny/4);

    cpl_imagelist_delete(iab);
    cpl_imagelist_delete(fab);
    cpl_image_delete(ic);
    cpl_image_delete(fc);
}

#endif
