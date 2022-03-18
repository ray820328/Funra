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

#include "cpl_image_io.h"
#include "cpl_vector.h"
#include "cpl_image_gen.h"
#include "cpl_memory.h"
#include "cpl_image_resample.h"
#include "cpl_test.h"
#include "cpl_tools.h"

/*-----------------------------------------------------------------------------
                                   Defines
 -----------------------------------------------------------------------------*/

#ifndef IMAGESZ
#define IMAGESZ   64
#endif

#define PIXRANGE 100

/*-----------------------------------------------------------------------------
                        Private function prototypes
 -----------------------------------------------------------------------------*/

static void check_kernel(cpl_image *, const cpl_image *, const cpl_image *,
                         const cpl_polynomial *, const cpl_polynomial *,
                         cpl_vector *, double, double);

static void cpl_image_warp_polynomial_test_turn(int, int, cpl_vector *, double);

static void cpl_image_warp_polynomial_test_shift(int, int, int, int,
                                                 cpl_vector *, double);

static void cpl_image_extract_subsample_test(void);
static void cpl_image_get_interpolated_test(void);

/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/
int main(void)
{
    cpl_image      * imf;
    cpl_image      * imd;
    cpl_image      * imtmp;
    cpl_image      * dx;
    cpl_image      * dy;
    cpl_image      * warped;
    cpl_polynomial * px;
    cpl_polynomial * py;
    cpl_vector     * xyprofile;
    const double     xyradius = 2.2; /* 2.3 gives much higher rounding ? */
    cpl_size         expo[2];
    FILE           * stream;
    cpl_size         i, j;


    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    /* Insert tests below */

    stream = cpl_msg_get_level() > CPL_MSG_INFO
        ? fopen("/dev/null", "a") : stdout;

    cpl_test_nonnull( stream );

    cpl_image_get_interpolated_test();

    cpl_image_extract_subsample_test();

    xyprofile = cpl_vector_new(1 + xyradius * CPL_KERNEL_TABSPERPIX);
    cpl_test_nonnull( xyprofile);

    for (i = 1-IMAGESZ; i < IMAGESZ; i += IMAGESZ/2) {
        for (j = 1-IMAGESZ; j < IMAGESZ; j += IMAGESZ/2) {
            cpl_image_warp_polynomial_test_shift(IMAGESZ, IMAGESZ, i, j,
                                                 xyprofile, xyradius);
        }
    }

    cpl_image_warp_polynomial_test_turn(IMAGESZ, IMAGESZ, xyprofile, xyradius);

    cpl_test_zero( cpl_vector_set_size(xyprofile, CPL_KERNEL_DEF_SAMPLES));
    cpl_test_zero( cpl_vector_fill_kernel_profile(xyprofile, CPL_KERNEL_DEFAULT,
                                                CPL_KERNEL_DEF_WIDTH) );

    /* Define the following polynomials : */
    /* x = 0.945946.u + -0.135135.v + -6.75676 */
    /* y = -0.202703.u + 0.743243.v + -12.8378 */

    px = cpl_polynomial_new(2);
    py = cpl_polynomial_new(2);

    expo[0] = 1;
    expo[1] = 0;
    cpl_polynomial_set_coeff(px, expo, 0.945946);
    expo[0] = 0;
    expo[1] = 1;
    cpl_polynomial_set_coeff(px, expo, -0.135135);
    expo[0] = 0;
    expo[1] = 0;
    cpl_polynomial_set_coeff(px, expo, -6.75676);
    expo[0] = 1;
    expo[1] = 0;
    cpl_polynomial_set_coeff(py, expo, -0.202703);
    expo[0] = 0;
    expo[1] = 1;
    cpl_polynomial_set_coeff(py, expo, 0.743243);
    expo[0] = 0;
    expo[1] = 0;
    cpl_polynomial_set_coeff(py, expo, -12.8378);
    
    /* First create a test image */
    cpl_msg_info("", "Create double and float test images.");
    imd = cpl_image_fill_test_create(IMAGESZ, IMAGESZ);
    imf = cpl_image_cast(imd, CPL_TYPE_FLOAT);

    cpl_msg_info("", "Apply the polynomial warping to the DOUBLE image.");
    /* Apply polynomial warping on the DOUBLE image */
    warped = cpl_image_new(IMAGESZ/3, IMAGESZ*2, CPL_TYPE_DOUBLE);
    cpl_test_nonnull( warped );

    cpl_test_zero( cpl_image_warp_polynomial(warped, imd, px, py, xyprofile, 2,
                                       xyprofile, 2) );

    cpl_image_multiply_scalar(warped, 0.0);

    cpl_test_zero( cpl_image_fill_jacobian_polynomial(warped, px, py) );

    cpl_image_multiply_scalar(warped, 0.0);

    /* Apply polynomial warping on the FLOAT image */
    cpl_test_zero( cpl_image_warp_polynomial(warped, imf, px, py, xyprofile, 2,
                                       xyprofile, 2) );
    cpl_image_multiply_scalar(warped, 0.0);
    
    cpl_polynomial_delete(px);
    cpl_polynomial_delete(py);
    
    /* Test cpl_image_warp() */
    dx = cpl_image_new(cpl_image_get_size_x(warped),
            cpl_image_get_size_y(warped), CPL_TYPE_DOUBLE);
    dy = cpl_image_new(cpl_image_get_size_x(warped),
            cpl_image_get_size_y(warped), CPL_TYPE_DOUBLE);
    cpl_image_add_scalar(dx, 1.0);
    cpl_image_add_scalar(dy, -2.0);
    
    cpl_test_zero( cpl_image_warp(warped, imd, dx, dy, xyprofile, 2, xyprofile, 2));
    cpl_image_multiply_scalar(warped, 0.0);

    cpl_test_zero( cpl_image_fill_jacobian(warped, dx, dy) );

    cpl_image_multiply_scalar(warped, 0.0);

    cpl_test_zero( cpl_image_warp(warped, imf, dx, dy, xyprofile, 2, xyprofile, 2));
    cpl_image_multiply_scalar(warped, 0.0);
    
    cpl_image_delete(dx);
    cpl_image_delete(dy);
    cpl_image_delete(warped);
    cpl_vector_delete(xyprofile);

    /* Test sub-sampling on imf and imd */
    cpl_msg_info("", "Sub sample imf and imd");
    imtmp = cpl_image_extract_subsample(imf, 2, 2);
    cpl_test_nonnull( imtmp );
    cpl_image_delete(imtmp);
    imtmp = cpl_image_extract_subsample(imd, 2, 2);
    cpl_test_nonnull( imtmp );
    cpl_image_delete(imtmp);
    cpl_image_delete(imf);
    cpl_image_delete(imd);

    if (stream != stdout) cpl_test_zero( fclose(stream) );

    return cpl_test_end(0);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Check if a pixel move with the kernel preserves the pixel values
  @param    temp      Preallocated image for interpolation
  @param    ref       Reference image verification
  @param    warp      Image to transform
  @param    px        Polynomial Pu(x,y)
  @param    py        Polynomial Pv(x,y)
  @param    kernel    Kernel type to test
  @param    xyprofile Interpolation weight as a function of the distance
  @param    xyradius  Positive inclusion radius
  @param    tol       Tolerance on pixel comparison
  @return   void

*/
/*----------------------------------------------------------------------------*/
static void check_kernel(cpl_image * temp,
                               const cpl_image * ref,
                               const cpl_image * warp,
                               const cpl_polynomial * px,
                               const cpl_polynomial * py,
                               cpl_vector * xyprofile,
                               double xyradius, double imtol)
{

    /* These kernels preserve the actual pixel-values */
    cpl_kernel kernels[] = {CPL_KERNEL_TANH, CPL_KERNEL_SINC, CPL_KERNEL_SINC2,
                            CPL_KERNEL_LANCZOS, CPL_KERNEL_HANN,
                            CPL_KERNEL_NEAREST,
                            CPL_KERNEL_HAMMING};
    /* FIXME: Integer tolerence higher (1 for int) for CPL_KERNEL_TANH */
    const double imtol0 = cpl_image_get_type(ref) == CPL_TYPE_INT ? 1.0
        : imtol * 10.0 * PIXRANGE;
    int ityp;

    for (ityp = 0; ityp < (int)(sizeof(kernels)/sizeof(kernels[0])); ityp++) {
        const double tol = ityp ? imtol : imtol0;

        cpl_msg_debug(cpl_func, "%g-tol transform with kernel-radius (%d): %g",
                      tol, kernels[ityp], xyradius);

        cpl_test_zero(cpl_vector_fill_kernel_profile(xyprofile, kernels[ityp],
                                                     xyradius));

        cpl_test_zero(cpl_image_warp_polynomial(temp, warp, px, py, xyprofile,
                                                xyradius, xyprofile, xyradius));

        cpl_test_image_abs(temp, ref, xyradius * xyradius * tol);

    }
    return;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Verify a warp-shift with an explicit shift
  @param    nx     Image X-size
  @param    ny     Image Y-size
  @param    dx     Shift in X
  @param    dy     Shift in Y
  @param    xyprofile Interpolation weight as a function of the distance
  @param    xyradius  Positive inclusion radius
  @return   void

*/
/*----------------------------------------------------------------------------*/
static void
cpl_image_warp_polynomial_test_shift(int nx, int ny, int dx, int dy,
                                     cpl_vector * xyprofile, double xyradius)
{

    const cpl_type   imtypes[] = {CPL_TYPE_DOUBLE, CPL_TYPE_FLOAT,
                                  CPL_TYPE_INT};

    const double     imtol[]  = {DBL_EPSILON, FLT_EPSILON, 0.0};

    cpl_polynomial * px = cpl_polynomial_new(2);
    cpl_polynomial * py = cpl_polynomial_new(2);
    cpl_mask       * mask = NULL;
    cpl_size         expo[2];
    int              ityp;

    /* The shift: (u,v) = (x-dx, y-dy) */

    expo[0] = 0;
    expo[1] = 0;
    cpl_polynomial_set_coeff(px, expo, -dx);
    cpl_polynomial_set_coeff(py, expo, -dy);
    expo[0] = 1;
    expo[1] = 0;
    cpl_polynomial_set_coeff(px, expo, 1.0);
    expo[0] = 0;
    expo[1] = 1;
    cpl_polynomial_set_coeff(py, expo, 1.0);


    for (ityp = 0; ityp < (int)(sizeof(imtypes)/sizeof(imtypes[0])); ityp++) {

        cpl_image * noise = cpl_image_new(nx, ny, imtypes[ityp]);
        cpl_image * trans = cpl_image_new(nx, ny, imtypes[ityp]);
        cpl_image * temp  = cpl_image_new(nx, ny, imtypes[ityp]);


        cpl_msg_info(cpl_func, "%d x %d shift a %d x %d %s image with a "
                     "polynomial-warp", dx, dy, nx, ny,
                     cpl_type_get_name(imtypes[ityp]));

        cpl_test_zero(cpl_image_fill_noise_uniform(noise, -PIXRANGE, PIXRANGE));

        if (mask == NULL) {
            int count;
            do {
                /* Need a mask with a mix of elements */
                cpl_mask_delete(mask);
                mask = cpl_mask_threshold_image_create(noise, -0.2 * PIXRANGE,
                                                       0.2 * PIXRANGE);
                count = cpl_mask_count(mask);
            } while (count == 0 || count == nx*ny);
            cpl_msg_info(cpl_func, "Rejecting %d of %d pixels", count, nx*ny);
            cpl_test_zero(cpl_image_reject_from_mask(noise, mask));
        }

        cpl_test_zero(cpl_image_copy(trans, noise, 1, 1));

        cpl_test_zero(cpl_image_shift(trans, dx, dy));

        check_kernel(temp, trans, noise, px, py,
                           xyprofile, xyradius, imtol[ityp]);

        cpl_image_delete(temp);
        cpl_image_delete(noise);
        cpl_image_delete(trans);

    }

    cpl_mask_delete(mask);
    cpl_polynomial_delete(px);
    cpl_polynomial_delete(py);

    return;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Verify a warp-turn with an explicit turn
  @param    nx     Image X-size
  @param    ny     Image Y-size
  @param    xyprofile Interpolation weight as a function of the distance
  @param    xyradius  Positive inclusion radius
  @return   void

*/
/*----------------------------------------------------------------------------*/
static void
cpl_image_warp_polynomial_test_turn(int nx, int ny, cpl_vector * xyprofile,
                                    double xyradius)
{

    const cpl_type   imtypes[] = {CPL_TYPE_DOUBLE, CPL_TYPE_FLOAT,
                                  CPL_TYPE_INT};

    const double     imtol[]  = {DBL_EPSILON, FLT_EPSILON, 0.0};

    cpl_polynomial * px = cpl_polynomial_new(2);
    cpl_polynomial * py = cpl_polynomial_new(2);
    cpl_mask       * mask = NULL;
    cpl_size         expo[2];
    int              ityp;

    /* The rotation: (u,v) = (y, N+1-x) */

    expo[0] = 0;
    expo[1] = 0;
    cpl_polynomial_set_coeff(py, expo, 1+IMAGESZ);
    expo[0] = 0;
    expo[1] = 1;
    cpl_polynomial_set_coeff(px, expo, 1);
    expo[0] = 1;
    expo[1] = 0;
    cpl_polynomial_set_coeff(py, expo, -1);


    for (ityp = 0; ityp < (int)(sizeof(imtypes)/sizeof(imtypes[0])); ityp++) {

        cpl_image * noise = cpl_image_new(nx, ny, imtypes[ityp]);
        cpl_image * trans = cpl_image_new(nx, ny, imtypes[ityp]);
        cpl_image * temp  = cpl_image_new(nx, ny, imtypes[ityp]);


        cpl_msg_info(cpl_func, "Turn a %d x %d %s image with a polynomial-"
                     "warp", nx, ny, cpl_type_get_name(imtypes[ityp]));

        cpl_test_zero(cpl_image_fill_noise_uniform(noise, -PIXRANGE, PIXRANGE));

        if (mask == NULL) {
            int count;
            do {
                /* Need a mask with a mix of elements */
                cpl_mask_delete(mask);
                mask = cpl_mask_threshold_image_create(noise, -0.2 * PIXRANGE,
                                                       0.2 * PIXRANGE);
                count = cpl_mask_count(mask);
            } while (count == 0 || count == nx*ny);
            cpl_msg_info(cpl_func, "Rejecting %d of %d pixels", count, nx*ny);
            cpl_test_zero(cpl_image_reject_from_mask(noise, mask));
        }

        cpl_test_zero(cpl_image_copy(trans, noise, 1, 1));

        cpl_test_zero(cpl_image_turn(trans, -1));

        check_kernel(temp, trans, noise, px, py, xyprofile, xyradius,
                     imtol[ityp]);

        cpl_image_delete(temp);
        cpl_image_delete(noise);
        cpl_image_delete(trans);

    }

    cpl_mask_delete(mask);
    cpl_polynomial_delete(px);
    cpl_polynomial_delete(py);

    return;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Test the CPL function
  @return   void
  @see cpl_image_extract_subsample(), cpl_image_rebin()
  @note Adding some real tests

*/
/*----------------------------------------------------------------------------*/
static void cpl_image_extract_subsample_test(void)
{
    cpl_image *carlo;
    cpl_image *subcarlo;
    cpl_image *expected;
    int data[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
                  10,11,12,13,14,15,16,17,18,19,
                  20,21,22,23,24,25,26,27,28,29,
                  30,31,32,33,34,35,36,37,38,39,
                  40,41,42,43,44,45,46,47,48,49,
                  50,51,52,53,54,55,56,57,58,59};

    int exp_data[] = {0, 2, 4, 6, 8,
                      20,22,24,26,28,
                      40,42,44,46,48};

    int exp_data2[] = {0, 3, 6, 9,
                       40,43,46,49};

    int exp_data3[] = {102,120,138,
                       222,240,258};

    int exp_data4[] = {264, 328};

    cpl_error_code error;

    carlo = cpl_image_wrap_int(10, 6, data);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(carlo);

    error = cpl_image_reject(carlo, 3, 3);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    subcarlo = cpl_image_extract_subsample(carlo, 2, 2);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(subcarlo);
    expected = cpl_image_wrap_int(5, 3, exp_data);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(expected);

    cpl_test_image_abs(subcarlo, expected, 0);

    cpl_image_delete(subcarlo);
    cpl_test_eq_ptr(cpl_image_unwrap(expected), exp_data);

    subcarlo = cpl_image_extract_subsample(carlo, 3, 4);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(subcarlo);
    expected = cpl_image_wrap_int(4, 2, exp_data2);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(expected);

    cpl_test_image_abs(subcarlo, expected, 0);

    cpl_image_delete(subcarlo);
    cpl_test_eq_ptr(cpl_image_unwrap(expected), exp_data2);

    subcarlo = cpl_image_rebin(carlo, 2, 2, 3, 2);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(subcarlo);
    expected = cpl_image_wrap_int(3, 2, exp_data3);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(expected);

    cpl_test_image_abs(subcarlo, expected, 0);

    cpl_image_delete(subcarlo);
    cpl_test_eq_ptr(cpl_image_unwrap(expected), exp_data3);

    subcarlo = cpl_image_rebin(carlo, 1, 1, 4, 4);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(subcarlo);
    expected = cpl_image_wrap_int(2, 1, exp_data4);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(expected);

    cpl_test_image_abs(subcarlo, expected, 0);

    cpl_image_delete(subcarlo);
    cpl_test_eq_ptr(cpl_image_unwrap(expected), exp_data4);
    cpl_test_eq_ptr(cpl_image_unwrap(carlo), data);

}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Test the CPL function
  @return   void
  @see cpl_image_get_interpolated()

*/
/*----------------------------------------------------------------------------*/
static void cpl_image_get_interpolated_test(void)
{

    const cpl_type ttype[] = {CPL_TYPE_INT, CPL_TYPE_FLOAT, CPL_TYPE_DOUBLE};
    const cpl_size tsize[] = {50, 500};
    const double value1[] = {2, 3};
    const double value2[] = {5, 7};
    const double mean = (value1[0] + value1[1] + value2[0] + value2[1])/4.0;
    const double txyrad[] = {-1.0, 0.0, CPL_KERNEL_DEF_WIDTH, 24.0, 100.0,
                             240.0};
    cpl_vector * xyprofile = cpl_vector_new(CPL_KERNEL_DEF_SAMPLES);
    size_t isize;

    for (isize = 0; isize < sizeof(tsize)/sizeof(tsize[0]); isize++) {
        size_t itype;
        const cpl_size imsize = tsize[isize];
        const double   c      = 0.5 + (double)imsize / 2.0;
        cpl_size i, j;
        cpl_error_code error;
        cpl_image * dimage = cpl_image_new(imsize, imsize, CPL_TYPE_DOUBLE);

        cpl_test_nonnull(dimage);

        for (j = 0; j < imsize; j++) {
            const double * pvalue = 2 * j < imsize ? value1 : value2;
            for (i = 0; i < imsize; i++) {
                (void)cpl_image_set(dimage, i+1, j+1,
                                      pvalue[2 * i < imsize ? 0 : 1]);
            }
        }

        for (itype = 0; itype < sizeof(ttype)/sizeof(ttype[0]); itype++) {
            size_t ixyrad;
            const cpl_type imtype = ttype[itype];

            cpl_image * image = imtype == CPL_TYPE_DOUBLE ? dimage :
                cpl_image_cast(dimage, imtype);

            for (ixyrad = 0; ixyrad < sizeof(txyrad)/sizeof(txyrad[0]);
                 ixyrad++) {
                const double xyradius = txyrad[ixyrad];
                double value, confidence;

                cpl_msg_info(cpl_func, "Interpolating from %d X %d %s-image, "
                             "r=%g", (int)imsize, (int)imsize, 
                             cpl_type_get_name(imtype), xyradius);
 
                error = cpl_vector_fill_kernel_profile(xyprofile,
                                                       CPL_KERNEL_DEFAULT,
                                                       xyradius);
                cpl_test_eq_error(error, xyradius > 0.0 ? CPL_ERROR_NONE
                                  : CPL_ERROR_ILLEGAL_INPUT);

                value = cpl_image_get_interpolated(image, c, c, xyprofile,
                                                   xyradius, xyprofile,
                                                   xyradius, &confidence);
                if (xyradius > 0.0) {
                    cpl_test_eq_error(error, CPL_ERROR_NONE);
                    cpl_test_abs(value, mean, 4.0 * xyradius * DBL_EPSILON);
                    if (xyradius < imsize) {
                        cpl_test_abs(confidence, 1.0, 30.0 * DBL_EPSILON);
                    } else {
                        cpl_test_abs(confidence, 1.0, xyradius * DBL_EPSILON);
                    }
                } else {
                    cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);
                }
            }
            if (image != dimage)
                cpl_image_delete(image);
        }
        cpl_image_delete(dimage);
    }

    cpl_vector_delete(xyprofile);
}
