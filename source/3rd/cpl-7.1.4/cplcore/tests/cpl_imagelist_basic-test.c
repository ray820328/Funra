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
#include <float.h>

#include "cpl_imagelist_basic.h"
#include "cpl_imagelist_io.h"
#include "cpl_image_gen.h"
#include "cpl_image_bpm.h"
#include "cpl_image_io.h"
#include "cpl_stats.h" 
#include "cpl_msg.h"
#include "cpl_test.h"
#include "cpl_memory.h"
#include "cpl_math_const.h"

#include "cpl_polynomial.h"

/*-----------------------------------------------------------------------------
                                  Define
 -----------------------------------------------------------------------------*/

#define     IMAGESZ         10
#define     NFRAMES         10
#define     MAGIC_NUMBER    42

#define     IMAGESZBIG      512

#define     TEST            10.0
#define     BASE            10.0
#define     POW              2.0

/*-----------------------------------------------------------------------------
                            Private Function prototypes
 -----------------------------------------------------------------------------*/

static void cpl_imagelist_collapse_median_create_tests(cpl_size, cpl_boolean);
static void cpl_imagelist_collapse_median_create_bench(cpl_size);
static void cpl_imagelist_collapse_minmax_create_bench(cpl_size);
static
void cpl_imagelist_collapse_sigclip_create_bench(cpl_size, cpl_collapse_mode);
static
void cpl_imagelist_collapse_sigclip_create_test_one(const cpl_imagelist *);

void cpl_imagelist_collapse_create_test(cpl_type);
void cpl_imagelist_collapse_sigclip_create_test(void);
static void cpl_imagelist_collapse_median_create_test_one(void);

/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/
int main(void)
{
    const cpl_type pixel_type[] = {CPL_TYPE_INT, CPL_TYPE_FLOAT,
                                   CPL_TYPE_DOUBLE, CPL_TYPE_FLOAT_COMPLEX,
                                   CPL_TYPE_DOUBLE_COMPLEX};

    const cpl_size  size[] = {1, 2, 3, 5, 8};
    cpl_image     * img;
    cpl_size        nbad;
    cpl_size        bpm_ind, bpm_x, bpm_y;
    cpl_size        i, j;
    cpl_boolean     do_bench;
    cpl_imagelist * imlist1;
    cpl_imagelist * imlist2;
    unsigned        itype1;
    cpl_error_code  error;


    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    /* Insert tests below */

    do_bench = cpl_msg_get_level() <= CPL_MSG_INFO ? CPL_TRUE : CPL_FALSE;

    cpl_imagelist_collapse_median_create_test_one();
    cpl_imagelist_collapse_create_test(CPL_TYPE_INT);
    cpl_imagelist_collapse_create_test(CPL_TYPE_FLOAT);
    cpl_imagelist_collapse_create_test(CPL_TYPE_DOUBLE);

    cpl_imagelist_collapse_sigclip_create_test();

    for (itype1 = 0;
         itype1 < sizeof(pixel_type)/sizeof(pixel_type[0]); 
         itype1++) {
        unsigned itype2;


        for (itype2 = 0; 
             itype2 < sizeof(pixel_type)/sizeof(pixel_type[0]); 
             itype2++) {
            unsigned isize;

            for (isize = 0; 
                 isize < sizeof(size)/sizeof(size[0]); 
                 isize++) {
                const cpl_error_code exp_err = 
                    !(pixel_type[itype1] & CPL_TYPE_COMPLEX) &&
                     (pixel_type[itype2] & CPL_TYPE_COMPLEX) ?
                    CPL_ERROR_TYPE_MISMATCH : CPL_ERROR_NONE;


                const double xmin = -100.0;
                const double xmax =  200.0;

                cpl_image * img2;

                imlist1 = cpl_imagelist_new(); 
                imlist2 = cpl_imagelist_new();

                for (i = 0; i < size[isize]; i++) {
                    cpl_image * iimg = 
                        cpl_image_new(IMAGESZ, IMAGESZ, pixel_type[itype1]);

                    cpl_test_zero(cpl_image_fill_noise_uniform(iimg, xmin,
                                                               xmax));

                    cpl_test_zero(cpl_imagelist_set(imlist1, iimg, i));
                }

                for (i = 0; i < size[isize]; i++) {
                    cpl_image * iimg = 
                        cpl_image_new(IMAGESZ, IMAGESZ, pixel_type[itype2]);

                    cpl_test_zero(cpl_image_fill_noise_uniform(iimg, xmin,
                                                               xmax) );

                    cpl_test_zero( cpl_imagelist_set(imlist2, iimg, i) );
                }
        
                cpl_msg_info(cpl_func, "Basic ops., types %s and %s, with "
                             "imagelist size %" CPL_SIZE_FORMAT,
                             cpl_type_get_name(pixel_type[itype1]),
                             cpl_type_get_name(pixel_type[itype2]),
                             size[isize]);

                error = cpl_imagelist_add(imlist1, imlist2);
                cpl_test_eq_error(error, exp_err);

                error = cpl_imagelist_multiply(imlist1, imlist2);
                cpl_test_eq_error(error, exp_err);

                error = cpl_imagelist_subtract(imlist1, imlist2);
                cpl_test_eq_error(error, exp_err);

                error = cpl_imagelist_divide(imlist1, imlist2);
                cpl_test_eq_error(error, exp_err);

                cpl_imagelist_delete(imlist2);

                if (itype1 == itype2) { /* Avoid redundant tests */
                    if (pixel_type[itype1] == CPL_TYPE_INT ||
                        pixel_type[itype1] == CPL_TYPE_FLOAT ||
                        pixel_type[itype1] == CPL_TYPE_DOUBLE) {

                        cpl_imagelist_collapse_sigclip_create_test_one(imlist1);

                        cpl_msg_info(cpl_func, "Collapse type %s, with "
                                     "imagelist size %" CPL_SIZE_FORMAT,
                                     cpl_type_get_name(pixel_type[itype1]),
                                     size[isize]);

                        img  = cpl_imagelist_collapse_minmax_create(imlist1, 0, 0);
                        cpl_test_error(CPL_ERROR_NONE);
                        cpl_test_nonnull(img);

                        img2 = cpl_imagelist_collapse_create(imlist1);
                        cpl_test_error(CPL_ERROR_NONE);
                        cpl_test_nonnull(img2);

                        if (pixel_type[itype1] != CPL_TYPE_INT) {
                            /* FIXME: Cannot test int (due to DFS08578) */
                            const double eps
                                = pixel_type[itype1] == CPL_TYPE_FLOAT
                                ? FLT_EPSILON : DBL_EPSILON;
                            cpl_test_image_abs(img, img2, 300.0 * eps);
                        }

                        cpl_image_delete(img);
                        cpl_image_delete(img2);
                    } else {
                        /* Cannot sort complex numbers */
                        img = cpl_imagelist_collapse_minmax_create(imlist1, 0, 0);
                        cpl_test_error(CPL_ERROR_INVALID_TYPE);
                        cpl_test_null(img);

                        img = cpl_imagelist_collapse_median_create(imlist1);
                        cpl_test_error(CPL_ERROR_INVALID_TYPE);
                        cpl_test_null(img);
                    }
                }

                cpl_imagelist_delete(imlist1);
            }
        }
    }


    /* Test cpl_imagelist_swap_axis_create() */
    img = cpl_image_fill_test_create(IMAGESZ, 2*IMAGESZ);
    imlist1 = cpl_imagelist_new();
    for (i=0; i<3*IMAGESZ; i++) {
        cpl_test_zero(cpl_imagelist_set(imlist1,cpl_image_duplicate(img),i));
    }
    cpl_image_delete(img);

    imlist2 = cpl_imagelist_swap_axis_create(imlist1, CPL_SWAP_AXIS_XZ);
    cpl_test_nonnull(imlist2);
    cpl_imagelist_delete(imlist2);

    imlist2 = cpl_imagelist_swap_axis_create(imlist1, CPL_SWAP_AXIS_YZ);
    cpl_test_nonnull(imlist2);
    cpl_imagelist_delete(imlist2);

    /* Test cpl_imagelist_swap_axis_create() with bad pixels */
    cpl_image_reject(cpl_imagelist_get(imlist1, 0), 1, 1);
    cpl_image_reject(cpl_imagelist_get(imlist1, 1), 2, 2);
    cpl_image_reject(cpl_imagelist_get(imlist1, 2), 2, 5);
    imlist2 = cpl_imagelist_swap_axis_create(imlist1, CPL_SWAP_AXIS_XZ);
    cpl_test_nonnull(imlist2);

    cpl_test_eq(cpl_image_count_rejected(cpl_imagelist_get(imlist2, 0)), 1);
    cpl_test_eq(cpl_image_count_rejected(cpl_imagelist_get(imlist2, 1)), 2);
    cpl_test_zero(cpl_image_count_rejected(cpl_imagelist_get(imlist2, 2)) );
    cpl_imagelist_delete(imlist2);

    imlist2 = cpl_imagelist_swap_axis_create(imlist1, CPL_SWAP_AXIS_YZ);
    cpl_test_nonnull(imlist2);
    cpl_imagelist_delete(imlist1);

    cpl_test_eq(cpl_image_count_rejected(cpl_imagelist_get(imlist2,0)), 1);
    cpl_test_eq(cpl_image_count_rejected(cpl_imagelist_get(imlist2,1)), 1);
    cpl_test_zero(cpl_image_count_rejected(cpl_imagelist_get(imlist2,2)) );
    cpl_test_eq(cpl_image_count_rejected(cpl_imagelist_get(imlist2,4)), 1);
    cpl_imagelist_delete(imlist2);

    /* Create two images set */
    img = cpl_image_fill_test_create(IMAGESZ, IMAGESZ);
    imlist1 = cpl_imagelist_new();
    imlist2 = cpl_imagelist_new();
    for (i=0; i<NFRAMES; i++) {
        cpl_test_zero(cpl_imagelist_set(imlist1,cpl_image_duplicate(img),i));
        cpl_test_zero(cpl_imagelist_set(imlist2,cpl_image_duplicate(img),i));
    }
    cpl_image_delete(img);
    
    /* Test basic operations between images sets */
    cpl_msg_info("","Compute ((((imlist1+imlist2)*imlist2)-imlist2)/imlist2)");
    cpl_test_zero( cpl_imagelist_add(imlist1, imlist2)  );
    cpl_test_zero( cpl_imagelist_multiply(imlist1, imlist2) );
    cpl_test_zero( cpl_imagelist_subtract(imlist1, imlist2) );
    cpl_test_zero( cpl_imagelist_divide(imlist1, imlist2) );
    cpl_imagelist_delete(imlist2);

    /* Test basic operations between an image set and an image */
    cpl_msg_info("","Compute ((((imlist1+img)*img)-img)/img)");
    img = cpl_image_new(IMAGESZ, IMAGESZ, CPL_TYPE_DOUBLE);
    cpl_test_zero( cpl_image_fill_noise_uniform(img, -1, 1) );
    cpl_test_zero( cpl_imagelist_add_image(imlist1, img) );
    cpl_test_zero( cpl_imagelist_multiply_image(imlist1,img));
    cpl_test_zero( cpl_imagelist_subtract_image(imlist1,img));
    cpl_test_zero( cpl_imagelist_divide_image(imlist1, img) );
    cpl_image_delete(img);

    /* Test basic operations between an image set and a constant */
    cpl_msg_info("","Compute ((((imlist1+42)*42)-42)/42)");
    cpl_test_zero( cpl_imagelist_add_scalar(imlist1, MAGIC_NUMBER) );
    cpl_test_zero( cpl_imagelist_multiply_scalar(imlist1, MAGIC_NUMBER) );
    cpl_test_zero( cpl_imagelist_subtract_scalar(imlist1, MAGIC_NUMBER) );
    cpl_test_zero( cpl_imagelist_divide_scalar(imlist1, MAGIC_NUMBER) );

    /* Test cpl_imagelist_normalise() */
    cpl_msg_info("","Normalize imlist1");
    if (cpl_imagelist_normalise(imlist1, CPL_NORM_MEAN) != CPL_ERROR_NONE) {
        cpl_test_end(1);
    }

    /* Test cpl_imagelist_threshold() */
    cpl_msg_info("","Threshold imlist1");
    cpl_test_zero( cpl_imagelist_threshold(imlist1, 1.0, DBL_MAX, 1.0, 0.0));
    
    /* Add bad pixel maps in the images of imlist1 */
    /* image1's bad pixels : 99  */
    /* image2's bad pixels : 49, 99  */
    /* image3's bad pixels : 32, 49, 99  */
    /* image4's bad pixels : 24, 32, 49, 99  */
    /* image5's bad pixels : 19, 24, 32, 49, 99  */
    /* image6's bad pixels : 15, 19, 24, 32, 49, 99  */
    /* image7's bad pixels : 13, 15, 19, 24, 32, 49, 99  */
    /* image8's bad pixels : 11, 13, 15, 19, 24, 32, 49, 99  */
    /* image9's bad pixels : 10, 11, 13, 15, 19, 24, 32, 49, 99  */
    /* image10's bad pixels : 9, 10, 11, 13, 15, 19, 24, 32, 49, 99  */
    for (i=0; i < cpl_imagelist_get_size(imlist1); i++) {
        const cpl_size nx = cpl_image_get_size_x(cpl_imagelist_get(imlist1, i));
        const cpl_size ny = cpl_image_get_size_y(cpl_imagelist_get(imlist1, i));
        nbad = i + 1;
        cpl_test( nbad <= nx*ny );

        for (j=0; j<nbad; j++) {
            bpm_ind = (nx*ny) / (j+1) - 1;
            bpm_x = bpm_ind % nx + 1;
            bpm_y = bpm_ind / nx + 1;
            cpl_test_zero( cpl_image_reject(cpl_imagelist_get(imlist1, i),
                                                 bpm_x, bpm_y) );
        }
    }

    /* Test cpl_image_new_from_accepted() */
    cpl_msg_info("","Get the contribution map of imlist1");
    img = cpl_image_new_from_accepted(imlist1);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(img);
    cpl_image_delete(img);

    /* Test cpl_imagelist_collapse_create() */
    cpl_msg_info("","Average imlist1 to one image");
    img = cpl_imagelist_collapse_create(imlist1);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(img);
    cpl_test_eq( cpl_image_count_rejected(img), 1 );
    cpl_image_delete(img);

    /* Test cpl_imagelist_collapse_minmax_create() */
    cpl_msg_info("","Average with rejection on imlist1");
    img = cpl_imagelist_collapse_minmax_create(imlist1, 0, 0);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(img);
    cpl_image_delete(img);

    cpl_imagelist_delete(imlist1);

    cpl_imagelist_collapse_median_create_tests(NFRAMES, CPL_TRUE);
    cpl_imagelist_collapse_median_create_tests(NFRAMES, CPL_FALSE);
    cpl_imagelist_collapse_median_create_tests(NFRAMES*NFRAMES, CPL_TRUE);
    cpl_imagelist_collapse_median_create_tests(NFRAMES*NFRAMES, CPL_FALSE);

    if (do_bench) {
        /* cpl_msg_set_component_on(); */
        cpl_imagelist_collapse_sigclip_create_bench(5,   CPL_COLLAPSE_MEAN);
        cpl_imagelist_collapse_sigclip_create_bench(10,  CPL_COLLAPSE_MEAN);
        cpl_imagelist_collapse_sigclip_create_bench(20,  CPL_COLLAPSE_MEAN);

        cpl_imagelist_collapse_sigclip_create_bench(5,   CPL_COLLAPSE_MEDIAN);
        cpl_imagelist_collapse_sigclip_create_bench(10,  CPL_COLLAPSE_MEDIAN);
        cpl_imagelist_collapse_sigclip_create_bench(20,  CPL_COLLAPSE_MEDIAN);

        cpl_imagelist_collapse_sigclip_create_bench(5,
                                                    CPL_COLLAPSE_MEDIAN_MEAN);
        cpl_imagelist_collapse_sigclip_create_bench(10,
                                                    CPL_COLLAPSE_MEDIAN_MEAN);
        cpl_imagelist_collapse_sigclip_create_bench(20,
                                                    CPL_COLLAPSE_MEDIAN_MEAN);

        if (cpl_msg_get_level() <= CPL_MSG_DEBUG) {
            cpl_imagelist_collapse_median_create_bench(5);
            cpl_imagelist_collapse_median_create_bench(10);
            cpl_imagelist_collapse_median_create_bench(20);
            cpl_imagelist_collapse_median_create_bench(50);
            cpl_imagelist_collapse_median_create_bench(100);
            cpl_imagelist_collapse_minmax_create_bench(5);
            cpl_imagelist_collapse_minmax_create_bench(10);
            cpl_imagelist_collapse_minmax_create_bench(20);
            cpl_imagelist_collapse_minmax_create_bench(50);
            cpl_imagelist_collapse_minmax_create_bench(100);
        } else {
            cpl_imagelist_collapse_median_create_bench(3);
            cpl_imagelist_collapse_minmax_create_bench(3);
        }
        /* cpl_msg_set_component_off(); */
    } else {
        cpl_imagelist_collapse_sigclip_create_bench(3,   CPL_COLLAPSE_MEAN);
        cpl_imagelist_collapse_sigclip_create_bench(3,   CPL_COLLAPSE_MEDIAN);
        cpl_imagelist_collapse_sigclip_create_bench(3, 
                                                    CPL_COLLAPSE_MEDIAN_MEAN);
        cpl_imagelist_collapse_median_create_bench(3);
        cpl_imagelist_collapse_minmax_create_bench(3);
    }

    /* Test cpl_imagelist_{exponential, power, logarithm}() */
    {
    float pow_result = powf((float)TEST, (float)POW);
        float log_result = log10f((float)TEST);

    /* Power */
    imlist1 = cpl_imagelist_new();
    imlist2 = cpl_imagelist_new();

    for (i = 0; i < NFRAMES; i++) {
        img = cpl_image_new(IMAGESZ, IMAGESZ, CPL_TYPE_FLOAT);
        cpl_test_zero(cpl_image_add_scalar(img, (float)TEST));
        cpl_test_zero(cpl_imagelist_set(imlist1, img, i));
    }

    for (i = 0; i < NFRAMES; i++) {
        img = cpl_image_new(IMAGESZ, IMAGESZ, CPL_TYPE_FLOAT);
        cpl_test_zero(cpl_image_add_scalar(img, pow_result));
        cpl_test_zero(cpl_imagelist_set(imlist2, img, i));
    }

    cpl_test_zero(cpl_imagelist_power(imlist1, (float)POW));

    for (i = 0; i < NFRAMES; i++) {
        cpl_test_image_abs(cpl_imagelist_get(imlist1, i),
                   cpl_imagelist_get(imlist2, i), 0.0);
    }
    cpl_imagelist_delete(imlist1);
    cpl_imagelist_delete(imlist2);

    /* Logarithm */
    imlist1 = cpl_imagelist_new();
    imlist2 = cpl_imagelist_new();

    for (i = 0; i < NFRAMES; i++) {
        img = cpl_image_new(IMAGESZ, IMAGESZ, CPL_TYPE_FLOAT);
        cpl_test_zero(cpl_image_add_scalar(img, (float)TEST));
        cpl_test_zero(cpl_imagelist_set(imlist1, img, i));
    }

    for (i = 0; i < NFRAMES; i++) {
        img = cpl_image_new(IMAGESZ, IMAGESZ, CPL_TYPE_FLOAT);
        cpl_test_zero(cpl_image_add_scalar(img, log_result));
        cpl_test_zero(cpl_imagelist_set(imlist2, img, i));
    }

    cpl_test_zero(cpl_imagelist_logarithm(imlist1, (float)BASE));

    for (i = 0; i < NFRAMES; i++) {
        cpl_test_image_abs(cpl_imagelist_get(imlist1, i),
                   cpl_imagelist_get(imlist2, i), 0.0);
    }
    cpl_imagelist_delete(imlist1);
    cpl_imagelist_delete(imlist2);

    /* Exponential */
    imlist1 = cpl_imagelist_new();
    imlist2 = cpl_imagelist_new();

    for (i = 0; i < NFRAMES; i++) {
        img = cpl_image_new(IMAGESZ, IMAGESZ, CPL_TYPE_FLOAT);
        cpl_test_zero(cpl_image_add_scalar(img, (float)POW));
        cpl_test_zero(cpl_imagelist_set(imlist1, img, i));
    }

    for (i = 0; i < NFRAMES; i++) {
        img = cpl_image_new(IMAGESZ, IMAGESZ, CPL_TYPE_FLOAT);
        cpl_test_zero(cpl_image_add_scalar(img, (float)CPL_MATH_E));
        cpl_test_zero(cpl_imagelist_set(imlist2, img, i));
    }

    cpl_test_zero(cpl_imagelist_exponential(imlist1, (float)CPL_MATH_E));
    cpl_test_zero(cpl_imagelist_power(imlist2, (float)POW));

    for (i = 0; i < NFRAMES; i++) {
        cpl_test_image_abs(cpl_imagelist_get(imlist1, i),
                   cpl_imagelist_get(imlist2, i), 0.0);
    }
    cpl_imagelist_delete(imlist1);
    cpl_imagelist_delete(imlist2);
    }

    return cpl_test_end(0);
}


/*----------------------------------------------------------------------------*/
/**
  @brief    Unit test the CPL function
  @param    mz       Number of planes to test
  @param    test_bad True iff bad pixels are to be tested
  @return   void

 */
/*----------------------------------------------------------------------------*/
static void cpl_imagelist_collapse_median_create_tests(cpl_size mz,
                                                       cpl_boolean test_bad)
{

    const cpl_type pixel_type[] = {CPL_TYPE_INT, CPL_TYPE_FLOAT,
                                   CPL_TYPE_DOUBLE};

    const cpl_size nx = IMAGESZ;
    const cpl_size ny = IMAGESZBIG;
    const cpl_size nz = mz | 1; /* Test only odd sizes + plus 1 rejected */
    unsigned       itype;


    cpl_msg_info(cpl_func, "Testing with %" CPL_SIZE_FORMAT " %"
                 CPL_SIZE_FORMAT " X %" CPL_SIZE_FORMAT " images", nz, nx, ny);

    for (itype = 0;
         itype < sizeof(pixel_type)/sizeof(pixel_type[0]); 
         itype++) {

        const cpl_type type = pixel_type[itype];
        cpl_imagelist * imglist = cpl_imagelist_new();
        cpl_image     * image = NULL;
        cpl_size        i;
        cpl_error_code code;

        /* Fill imagelist with pixel-type type */
        /* The median of every pixel is zero */
        for (i = 0; i < nz; i++) {

            image = cpl_image_new(nx, ny, type);
            if (i > nz/2) {
                code = cpl_image_fill_noise_uniform(image, 1.0, (double)nz);
                cpl_test_eq_error(code, CPL_ERROR_NONE);
            } else if (i > 0) {
                code = cpl_image_fill_noise_uniform(image, (double)-nz, -1.0);
                cpl_test_eq_error(code, CPL_ERROR_NONE);
            }
            code = cpl_imagelist_set(imglist, image, i);
            cpl_test_eq_error(code, CPL_ERROR_NONE);
        }

        cpl_test_eq(cpl_imagelist_get_size(imglist), nz);

        if (test_bad) {
            /* Reject all pixels in the last, extra image */
            image = cpl_image_duplicate(image);
            code = cpl_mask_not(cpl_image_get_bpm(image));
            cpl_test_eq_error(code, CPL_ERROR_NONE);

            code = cpl_imagelist_set(imglist, image, nz);
            cpl_test_eq_error(code, CPL_ERROR_NONE);
            cpl_test_eq(cpl_imagelist_get_size(imglist), nz + 1);
        }

        image = cpl_imagelist_collapse_median_create(imglist);

        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_nonnull(image);
        cpl_test_eq(cpl_image_get_type(image), type);

        cpl_test_leq(cpl_image_get_min(image), 0.0);
        cpl_test_leq(cpl_image_get_max(image), 0.0);

        cpl_image_delete(image);
        cpl_imagelist_delete(imglist);

    }

    /* small (< L2 cache) imagelist test */
    {
        cpl_imagelist * l = cpl_imagelist_new();
        cpl_imagelist * lb = cpl_imagelist_new();
        size_t i, j, k;
        int rej;
        cpl_image * med, * medb;
        for (i = 0; i < 107; i++) {
            cpl_image * img = cpl_image_new(7, 13, CPL_TYPE_FLOAT);
            cpl_image * imgb = cpl_image_new(7, 13, CPL_TYPE_FLOAT);
            for (j = 0; j < 7; j++) {
                for (k = 0; k < 13; k++) {
                    cpl_image_set(img, j + 1, k + 1, k * 13 + j + i);
                    cpl_image_set(imgb, j + 1, k + 1, k * 13 + j + i);
                }
            }
            cpl_image_reject(imgb, 4, 12);
            cpl_image_reject(imgb, 5, 9);
            if (i == 3)
                cpl_image_reject(imgb, 7, 10);
            cpl_imagelist_set(l, img, i);
            cpl_imagelist_set(lb, imgb, i);
        }
        med = cpl_imagelist_collapse_median_create(l);
        medb = cpl_imagelist_collapse_median_create(lb);

        for (j = 0; j < 7; j++) {
            for (k = 0; k < 13; k++) {
                cpl_test_eq(cpl_image_get(med, j + 1, k + 1, &rej),
                            k * 13 + j + 107 / 2);
            }
        }
        cpl_image_set(med, 7, 10, cpl_image_get(med, 7, 10, &rej) + 0.5);
        cpl_test_image_abs(med, medb, 0);
        cpl_test(cpl_image_is_rejected(medb, 4, 12));
        cpl_test(cpl_image_is_rejected(medb, 5, 9));

        cpl_image_delete(med);
        cpl_image_delete(medb);
        cpl_imagelist_delete(l);
        cpl_imagelist_delete(lb);
    }

    return;

}


/*----------------------------------------------------------------------------*/
/**
  @brief    Benchmark the CPL function
  @param    mz  Number of planes to test
  @return   void
 */
/*----------------------------------------------------------------------------*/
static void cpl_imagelist_collapse_median_create_bench(cpl_size mz)
{

    cpl_imagelist * imglist = cpl_imagelist_new();
    cpl_image     * image;
    const cpl_size  nz = mz;
    const cpl_size  nx = 2*IMAGESZBIG;
    const cpl_size  ny = 2*IMAGESZBIG;
    double          secs;
    size_t          bytes;
    cpl_size        i;


    cpl_msg_info(cpl_func, "Benchmarking with %" CPL_SIZE_FORMAT " %"
                 CPL_SIZE_FORMAT " X %" CPL_SIZE_FORMAT " images", nz, nx, ny);

    /* Fill imagelist */
    for (i = 0; i < nz; i++) {
        image = cpl_image_new(nx, ny, CPL_TYPE_FLOAT);
        cpl_test_zero(cpl_image_fill_noise_uniform(image, 0.0, 1.0));
        cpl_test_zero(cpl_imagelist_set(imglist, image, i));
    }

    cpl_test_eq(cpl_imagelist_get_size(imglist), nz);

    cpl_msg_info(cpl_func, "Testing with %" CPL_SIZE_FORMAT " %"
                 CPL_SIZE_FORMAT " X %" CPL_SIZE_FORMAT " images", nz, nx, ny);

    bytes = cpl_test_get_bytes_imagelist(imglist);

    secs  = cpl_test_get_cputime();

    image = cpl_imagelist_collapse_median_create(imglist);

    secs = cpl_test_get_cputime() - secs;

    cpl_test_nonnull( image);
    cpl_test_error( CPL_ERROR_NONE);
    cpl_image_delete(image);

    cpl_msg_info(cpl_func, "Time spent median collapsing with %"
                 CPL_SIZE_FORMAT " X %" CPL_SIZE_FORMAT " X %" CPL_SIZE_FORMAT
                 ": [s]: %g", nz, nx, ny, secs);

    if (secs > 0.0) {
        cpl_msg_info(cpl_func,"Processing rate [MB/s]: %g",
                     1e-6 * (double)bytes / secs);
    }

    cpl_imagelist_delete(imglist);

    return;     

}

/*----------------------------------------------------------------------------*/
/**
  @brief    Benchmark the CPL function
  @param    mz  Number of planes to test
  @return   void
 */
/*----------------------------------------------------------------------------*/
static void cpl_imagelist_collapse_minmax_create_bench(cpl_size mz)
{

    cpl_imagelist * imglist = cpl_imagelist_new();
    cpl_image     * image;
    const cpl_size  nz = mz;
    const cpl_size  nx = 2*IMAGESZBIG;
    const cpl_size  ny = 2*IMAGESZBIG;
    double          secs;
    size_t          bytes;
    cpl_size        i;


    cpl_msg_info(cpl_func, "Benchmarking with %" CPL_SIZE_FORMAT " %"
                 CPL_SIZE_FORMAT " X %" CPL_SIZE_FORMAT " images", nz, nx, ny);

    /* Fill imagelist */
    for (i = 0; i < nz; i++) {
        image = cpl_image_new(nx, ny, CPL_TYPE_FLOAT);
        cpl_test_zero(cpl_image_fill_noise_uniform(image, 0.0, 1.0));
        cpl_test_zero(cpl_imagelist_set(imglist, image, i));
    }

    cpl_test_eq(cpl_imagelist_get_size(imglist), nz);

    cpl_msg_info(cpl_func, "Testing with %" CPL_SIZE_FORMAT " %"
                 CPL_SIZE_FORMAT " X %" CPL_SIZE_FORMAT " images", nz, nx, ny);

    bytes = cpl_test_get_bytes_imagelist(imglist);

    secs  = cpl_test_get_cputime();

    image = cpl_imagelist_collapse_minmax_create(imglist, 1, 1);

    secs = cpl_test_get_cputime() - secs;

    cpl_test_nonnull( image);
    cpl_test_error( CPL_ERROR_NONE);
    cpl_image_delete(image);

    cpl_msg_info(cpl_func, "Time spent min-max collapsing with %"
                 CPL_SIZE_FORMAT " X %" CPL_SIZE_FORMAT " X %"
                 CPL_SIZE_FORMAT ": [s]: %g", nz, nx, ny, secs);

    if (secs > 0.0) {
        cpl_msg_info(cpl_func,"Processing rate [MB/s]: %g",
                     1e-6 * (double)bytes / secs);
    }

    cpl_imagelist_delete(imglist);

    return;     

}


/*----------------------------------------------------------------------------*/
/**
  @brief    Test the CPL function
  @param    self Imagelist to test
  @return   void
 */
/*----------------------------------------------------------------------------*/
static
void cpl_imagelist_collapse_sigclip_create_test_one(const cpl_imagelist * self)
{

    const unsigned    clip_mode[] = {CPL_COLLAPSE_MEAN, CPL_COLLAPSE_MEDIAN,
                                     CPL_COLLAPSE_MEDIAN_MEAN};
    const int         nmode       = sizeof(clip_mode)/sizeof(clip_mode[0]);
    const cpl_size    nsize       = cpl_imagelist_get_size(self);
    const cpl_image * image       = cpl_imagelist_get_const(self, 0);
    const cpl_type    type        = cpl_image_get_type(image);
    const cpl_size    nx          = cpl_image_get_size_x(image);
    const cpl_size    ny          = cpl_image_get_size_y(image);
    cpl_image       * map         = cpl_image_new(nx, ny, CPL_TYPE_INT);
    int               imode;


    cpl_msg_info(cpl_func, "Testing %s with %" CPL_SIZE_FORMAT " images of "
                 "type %s", "cpl_imagelist_collapse_sigclip_create", nsize,
                 cpl_type_get_name(type));

    for (imode = 0; imode < nmode; imode++) {
        const unsigned mode = clip_mode[imode];
        cpl_size       ikeep;
        cpl_size       jkeep = nsize;

        for (ikeep = nsize; ikeep > 0; ikeep--) {
            const double keepfrac = (ikeep == nsize ? ikeep : ikeep + 0.5)
                / (double)nsize;

            cpl_image  * clipped
                = cpl_imagelist_collapse_sigclip_create(self, 0.5, 1.5,
                                                        keepfrac, mode, map);

            if (nsize == 1) {
                cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);
                cpl_test_null(clipped);
            } else if (type & CPL_TYPE_COMPLEX) {
                cpl_test_error(CPL_ERROR_INVALID_TYPE);
                cpl_test_null(clipped);
            } else {
                cpl_image * contrib;
                cpl_mask  * bpm;
                /* In no-clip collapsed image */
                cpl_size    minpix, maxpix, maxbad;

                cpl_test_error(CPL_ERROR_NONE);
                cpl_test_nonnull(clipped);

                cpl_test_eq(cpl_image_get_type(clipped),   type);
                cpl_test_eq(cpl_image_get_size_x(clipped), nx);
                cpl_test_eq(cpl_image_get_size_x(clipped), ny);

                contrib = cpl_image_new_from_accepted(self);
                cpl_test_error(CPL_ERROR_NONE);
                cpl_test_nonnull(contrib);

                minpix = (cpl_size)cpl_image_get_min(contrib);
                maxpix = (cpl_size)cpl_image_get_max(contrib);
                maxbad = nsize - minpix;

                bpm = cpl_mask_threshold_image_create(map, -0.5, 0.5);

                if (cpl_image_get_min(map) == 0) {
                    cpl_test_eq_mask(bpm, cpl_image_get_bpm_const(clipped));
                } else {
                    cpl_test_null(cpl_image_get_bpm_const(clipped));
                    cpl_test_zero(cpl_mask_count(bpm)); 

                    cpl_test_leq(CX_MAX(1, ikeep - maxbad),
                                 cpl_image_get_min(map));
                }

                cpl_test_leq(cpl_image_get_max(map), CX_MIN(jkeep, maxpix));
                /* Contribution cannot increase with decreasing keepfrac */
                jkeep = (cpl_size)cpl_image_get_max(map);

                if (ikeep == nsize) {
                    /* Nothing was rejected */

                    cpl_image * average = cpl_imagelist_collapse_create(self);


                    cpl_test_error(CPL_ERROR_NONE);
                    cpl_test_nonnull(average);

                    cpl_test_image_abs(contrib, map, 0.0);

                    /* The clipped image is not (much) different from the
                       averaged one - except for integer pixels, where the
                       average is computed using integer arithmetics,
                       while the clipped uses floating point */
                    cpl_test_image_abs(clipped, average, nsize * 
                                       (type == CPL_TYPE_INT ? 1 : 150.0 *
                                        (type == CPL_TYPE_DOUBLE ? DBL_EPSILON
                                         :  FLT_EPSILON)));
                    cpl_image_delete(average);
                }

                cpl_mask_delete(bpm);
                cpl_image_delete(contrib);
                cpl_image_delete(clipped);
            }
        }

    }
    cpl_image_delete(map);
}

/*----------------------------------------------------------------------------*/
/**
  @brief  Benchmark the CPL function
  @param  mz    Number of planes to test
  @param  mode  Clipping mode, CPL_COLLAPSE_MEAN, CPL_COLLAPSE_MEDIAN or
                CPL_COLLAPSE_MEDIAN_MEAN
  @return void
  @note Modified from cpl_imagelist_collapse_minmax_create_bench()
 */
/*----------------------------------------------------------------------------*/
static void cpl_imagelist_collapse_sigclip_create_bench(cpl_size          mz,
                                                        cpl_collapse_mode mode)
{

    cpl_imagelist * imglist = cpl_imagelist_new();
    const cpl_size  nz = mz;
    const cpl_size  nx = 2*IMAGESZBIG;
    const cpl_size  ny = 2*IMAGESZBIG;
    cpl_image     * image = NULL;
    cpl_image     * map = cpl_image_new(nx, ny, CPL_TYPE_INT);
    cpl_error_code error;
    double          secs;
    cpl_type        type;
    size_t          bytes;
    cpl_size        i;


    cpl_msg_info(cpl_func, "Benchmarking with %" CPL_SIZE_FORMAT " %"
                 CPL_SIZE_FORMAT " X %" CPL_SIZE_FORMAT " images", nz, nx, ny);

    /* Fill imagelist */
    for (i = 0; i < nz; i++) {
        image = cpl_image_new(nx, ny, CPL_TYPE_DOUBLE);
        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_nonnull(image);

        error = cpl_image_fill_noise_uniform(image, 0.0, 1.0);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_imagelist_set(imglist, image, i);
        cpl_test_eq_error(error, CPL_ERROR_NONE);
    }

    type = cpl_image_get_type(image);

    cpl_test_eq(cpl_imagelist_get_size(imglist), nz);

    cpl_msg_info(cpl_func, "Testing with %" CPL_SIZE_FORMAT " %"
                 CPL_SIZE_FORMAT " X %" CPL_SIZE_FORMAT " images", nz, nx, ny);

    bytes = cpl_test_get_bytes_imagelist(imglist);

    secs  = cpl_test_get_cputime();

    image = cpl_imagelist_collapse_sigclip_create(imglist, 0.5, 1.5,
                                                  0.25, mode, map);

    secs = cpl_test_get_cputime() - secs;

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq(cpl_image_get_size_x(image), nx);
    cpl_test_eq(cpl_image_get_size_y(image), ny);
    cpl_test_eq(cpl_image_get_type(image), type);
    cpl_image_delete(image);

    cpl_test_leq(1, cpl_image_get_min(map));

    cpl_msg_info(cpl_func, "Time spent kappa-sigma-clipping around the %s "
                 "with %" CPL_SIZE_FORMAT " X %" CPL_SIZE_FORMAT " X %"
                 CPL_SIZE_FORMAT ": [s]: %g", mode == CPL_COLLAPSE_MEAN
                 ? "mean" : (mode == CPL_COLLAPSE_MEDIAN ? "median" :
                             "median+mean"), nz, nx, ny, secs);

    if (secs > 0.0) {
        cpl_msg_info(cpl_func,"Processing rate [MB/s]: %g",
                     1e-6 * (double)bytes / secs);
    }

    cpl_imagelist_delete(imglist);
    cpl_image_delete(map);

    return;     

}



/*----------------------------------------------------------------------------*/
/**
  @brief    Test the CPL function
  @param    type The pixel type to test
  @return   void
 */
/*----------------------------------------------------------------------------*/
void cpl_imagelist_collapse_create_test(cpl_type type)
{

    cpl_imagelist * imlist1 = cpl_imagelist_new();
    cpl_image     * img = cpl_image_new(IMAGESZ, IMAGESZ, type);
    cpl_error_code  error;
    cpl_size i;


    /* Fill the list with integers 1 .. 2n+1 */
    /* Their mean:   0.5 * (2n+1) * (2n+2)  / 2n+1 = n + 1  */

    for (i=0; i < 2 * IMAGESZ + 1; i++) {
        error = cpl_image_add_scalar(img, 1.0);
        cpl_test_eq_error(error, CPL_ERROR_NONE);
        error = cpl_imagelist_set(imlist1, cpl_image_duplicate(img), i);
        cpl_test_eq_error(error, CPL_ERROR_NONE);
    }

    cpl_image_delete(img);

    cpl_test_eq(cpl_imagelist_get_size(imlist1), 2 * IMAGESZ + 1);

    img = cpl_imagelist_collapse_create(imlist1);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(img);
    cpl_test_eq(cpl_image_get_type(img), type);
    cpl_test_abs(cpl_image_get_min(img), IMAGESZ + 1.0, DBL_EPSILON);
    cpl_test_abs(cpl_image_get_max(img), IMAGESZ + 1.0, DBL_EPSILON);

    cpl_image_delete(img);
    cpl_imagelist_delete(imlist1);

    return;     

}

/*----------------------------------------------------------------------------*/
/**
  @brief    Test the CPL function
  @return   void
 */
/*----------------------------------------------------------------------------*/
void cpl_imagelist_collapse_sigclip_create_test(void)
{

    const unsigned  clip_mode[] = {CPL_COLLAPSE_MEAN, CPL_COLLAPSE_MEDIAN,
                                   CPL_COLLAPSE_MEDIAN_MEAN};
    const int       nmode       = sizeof(clip_mode)/sizeof(clip_mode[0]);
    cpl_imagelist * imlist1     = cpl_imagelist_new();
    cpl_image     * nullimg; /* Expected to be NULL */
    cpl_image     * map = cpl_image_new(1, 1, CPL_TYPE_INT);
    cpl_error_code  error;
    int             imode;
    cpl_size        nz, i;


    for (imode = 0; imode < nmode; imode++) {
        const unsigned mode = clip_mode[imode];
        cpl_image    * img = cpl_image_new(1, 1, CPL_TYPE_DOUBLE);

        cpl_imagelist_empty(imlist1);

        /* Fill the list with equal number of zero- and one-valued images */
        /* Their mean:   0.5             */
        /* Their St.dev: 0.5 * n / (n-1) */

        for (i=0; i < IMAGESZ; i++) {
            error = cpl_imagelist_set(imlist1, cpl_image_duplicate(img), i);
            cpl_test_eq_error(error, CPL_ERROR_NONE);
        }
        cpl_image_add_scalar(img, 1.0);
        for (; i < 2 * IMAGESZ; i++) {
            error = cpl_imagelist_set(imlist1, cpl_image_duplicate(img), i);
            cpl_test_eq_error(error, CPL_ERROR_NONE);
        }
        nz = 2 * IMAGESZ;

        cpl_image_delete(img);

        cpl_test_eq(cpl_imagelist_get_size(imlist1), nz);

        /* kappa greater than (n-1)/n means no iterations at all */
        img = cpl_imagelist_collapse_sigclip_create(imlist1, 2.0, 2.0, 0.5,
                                                    mode, map);
        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_nonnull(img);
        cpl_test_abs(cpl_image_get_min(img), 0.5, DBL_EPSILON);
        cpl_test_abs(cpl_image_get_max(img), 0.5, DBL_EPSILON);
        cpl_test_eq(cpl_image_get_min(map), 2 * IMAGESZ);
        cpl_test_eq(cpl_image_get_max(map), 2 * IMAGESZ);

        cpl_image_delete(img);

        /* kappa less than (n-1)/n means 1 iteration, still with no clipping */
        img = cpl_imagelist_collapse_sigclip_create(imlist1, 0.5, 0.5, 0.5,
                                                    mode, map);
        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_nonnull(img);
        cpl_test_abs(cpl_image_get_min(img), 0.5, DBL_EPSILON);
        cpl_test_abs(cpl_image_get_max(img), 0.5, DBL_EPSILON);
        cpl_test_eq(cpl_image_get_min(map), 2 * IMAGESZ);
        cpl_test_eq(cpl_image_get_max(map), 2 * IMAGESZ);


        /* Append an image once, with the average value */
        cpl_test_zero(cpl_imagelist_set(imlist1, img, nz++));
        cpl_test_eq(cpl_imagelist_get_size(imlist1), nz);

        /* It is not possible to clip all but one value, so all are kept */
        img = cpl_imagelist_collapse_sigclip_create(imlist1, 0.5, 0.5, 1.0 / nz,
                                                    mode, map);

        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_nonnull(img);
        cpl_test_abs(cpl_image_get_min(img), 0.5, DBL_EPSILON);
        cpl_test_abs(cpl_image_get_max(img), 0.5, DBL_EPSILON);
        cpl_test_eq(cpl_image_get_min(map), 2 * IMAGESZ + 1);
        cpl_test_eq(cpl_image_get_max(map), 2 * IMAGESZ + 1);

        /* Append the average image once more */
        cpl_test_zero(cpl_imagelist_set(imlist1, img, nz++));
        cpl_test_eq(cpl_imagelist_get_size(imlist1), nz);

        /* It is possible to clip all but two values */
        img = cpl_imagelist_collapse_sigclip_create(imlist1, 0.5, 0.5, 1.0 / nz,
                                                    mode, map);

        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_nonnull(img);
        cpl_test_abs(cpl_image_get_min(img), 0.5, DBL_EPSILON);
        cpl_test_abs(cpl_image_get_max(img), 0.5, DBL_EPSILON);
        cpl_test_eq(cpl_image_get_min(map), 2);
        cpl_test_eq(cpl_image_get_max(map), 2);

        /* Append two more values, one a bit below and one a bit above
           the mean */
        error = cpl_image_subtract_scalar(img, 1.0 / IMAGESZ);
        cpl_test_eq_error(error, CPL_ERROR_NONE);
        cpl_test_zero(cpl_imagelist_set(imlist1, img, nz++));
        cpl_test_eq(cpl_imagelist_get_size(imlist1), nz);

        img = cpl_image_duplicate(cpl_imagelist_get_const(imlist1, nz-1));
        error = cpl_image_add_scalar(img, 2.0 / IMAGESZ);
        cpl_test_eq_error(error, CPL_ERROR_NONE);
        cpl_test_zero(cpl_imagelist_set(imlist1, img, nz++));
        cpl_test_eq(cpl_imagelist_get_size(imlist1), nz);

        /* Clip no values */
        img = cpl_imagelist_collapse_sigclip_create(imlist1, 1.0 / nz, 1.0 / nz,
                                                    1.0 / nz, mode, map);
        cpl_test_abs(cpl_image_get_min(img), 0.5, DBL_EPSILON);
        cpl_test_abs(cpl_image_get_max(img), 0.5, DBL_EPSILON);

        cpl_image_delete(img);

        /* Clip all but the two central values */
        img = cpl_imagelist_collapse_sigclip_create(imlist1, 0.75, 0.75,
                                                    1.0 / nz, mode, map);

        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_nonnull(img);
        cpl_test_abs(cpl_image_get_min(img), 0.5, DBL_EPSILON);
        cpl_test_abs(cpl_image_get_max(img), 0.5, DBL_EPSILON);
        cpl_test_eq(cpl_image_get_min(map), 2);
        cpl_test_eq(cpl_image_get_max(map), 2);

        cpl_image_delete(img);

        /* Clip all but the four most central values */
        img = cpl_imagelist_collapse_sigclip_create(imlist1, 1.0, 1.0, 3.5 / nz,
                                                    mode, map);

        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_nonnull(img);
        cpl_test_abs(cpl_image_get_min(img), 0.5, DBL_EPSILON);
        cpl_test_abs(cpl_image_get_max(img), 0.5, DBL_EPSILON);
        cpl_test_eq(cpl_image_get_min(map), 4);
        cpl_test_eq(cpl_image_get_max(map), 4);

        cpl_image_delete(img);

        /* Test error handling */

        img = cpl_imagelist_collapse_sigclip_create(imlist1, 2.0, 2.0, 0.5,
                                                    mode, NULL);
        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_nonnull(img);
        cpl_image_delete(img);


        nullimg = cpl_imagelist_collapse_sigclip_create(NULL, 1.0, 1.0, 0.75,
                                                        mode, NULL);
        cpl_test_error(CPL_ERROR_NULL_INPUT);
        cpl_test_null(nullimg);

        nullimg = cpl_imagelist_collapse_sigclip_create(imlist1, 0.0, 0.0, 0.75,
                                                        mode, NULL);
        cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
        cpl_test_null(nullimg);

        nullimg = cpl_imagelist_collapse_sigclip_create(imlist1, -1.0, 1.0,
                                                        0.75, mode, NULL);
        cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
        cpl_test_null(nullimg);

        nullimg = cpl_imagelist_collapse_sigclip_create(imlist1, 1.0, 1.0, 0.0,
                                                        mode, NULL);
        cpl_test_error(CPL_ERROR_ACCESS_OUT_OF_RANGE);
        cpl_test_null(nullimg);

        nullimg = cpl_imagelist_collapse_sigclip_create(imlist1, 1.0, 1.0,
                                                        1.0 + DBL_EPSILON,
                                                        mode, NULL);
        cpl_test_error(CPL_ERROR_ACCESS_OUT_OF_RANGE);
        cpl_test_null(nullimg);

        img = cpl_image_new(IMAGESZ, IMAGESZ, CPL_TYPE_FLOAT);
        nullimg = cpl_imagelist_collapse_sigclip_create(imlist1, 1.0, 1.0, 0.75,
                                                        mode, img);
        cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
        cpl_test_null(nullimg);

        cpl_image_delete(img);

        img = cpl_image_new(IMAGESZ, IMAGESZ + 1, CPL_TYPE_INT);
        nullimg = cpl_imagelist_collapse_sigclip_create(imlist1, 1.0, 1.0, 0.75,
                                                        mode, img);
        cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
        cpl_test_null(nullimg);

        cpl_image_delete(img);

        img = cpl_image_new(IMAGESZ + 1, IMAGESZ, CPL_TYPE_INT);
        nullimg = cpl_imagelist_collapse_sigclip_create(imlist1, 1.0, 1.0, 0.75,
                                                        mode, img);
        cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
        cpl_test_null(nullimg);

        cpl_imagelist_empty(imlist1);

        nullimg = cpl_imagelist_collapse_sigclip_create(imlist1, 1.0, 1.0, 0.75,
                                                        mode, NULL);
        cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);
        cpl_test_null(nullimg);

        error = cpl_imagelist_set(imlist1, img, 0);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        nullimg = cpl_imagelist_collapse_sigclip_create(imlist1, 1.0, 1.0, 0.75,
                                                        mode, NULL);
        cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);
        cpl_test_null(nullimg);

        cpl_imagelist_empty(imlist1);

        img = cpl_image_new(1, 1, CPL_TYPE_FLOAT_COMPLEX);
        error = cpl_imagelist_set(imlist1, img, 0);
        img = cpl_image_new(1, 1, CPL_TYPE_FLOAT_COMPLEX);
        error = cpl_imagelist_set(imlist1, img, 1);

        nullimg = cpl_imagelist_collapse_sigclip_create(imlist1, 1.0, 1.0, 0.75,
                                                        mode, NULL);
        cpl_test_error(CPL_ERROR_INVALID_TYPE);
        cpl_test_null(nullimg);

        cpl_imagelist_empty(imlist1);

        img = cpl_image_new(1, 1, CPL_TYPE_DOUBLE_COMPLEX);
        error = cpl_imagelist_set(imlist1, img, 0);
        img = cpl_image_new(1, 1, CPL_TYPE_DOUBLE_COMPLEX);
        error = cpl_imagelist_set(imlist1, img, 1);

        nullimg = cpl_imagelist_collapse_sigclip_create(imlist1, 1.0, 1.0, 0.75,
                                                        mode, NULL);
        cpl_test_error(CPL_ERROR_INVALID_TYPE);
        cpl_test_null(nullimg);

    }

    nullimg = cpl_imagelist_collapse_sigclip_create(imlist1, 1.0, 1.0, 0.75,
                                                    CPL_COLLAPSE_MEAN +
                                                    CPL_COLLAPSE_MEDIAN +
                                                    CPL_COLLAPSE_MEDIAN_MEAN,
                                                    NULL);
    cpl_test_error(CPL_ERROR_UNSUPPORTED_MODE);
    cpl_test_null(nullimg);

    cpl_imagelist_delete(imlist1);
    cpl_image_delete(map);

}

/*----------------------------------------------------------------------------*/
/**
  @brief    Test the CPL function
  @return   void
  @note This test activates the cache optimization given a 18MB L2 cache
 */
/*----------------------------------------------------------------------------*/
static void cpl_imagelist_collapse_median_create_test_one(void)
{

    cpl_imagelist * imlist = cpl_imagelist_new();
    cpl_image * img;
    cpl_size i;

    for (i = 0 ; i < 300; i++) {
        img = cpl_image_new(IMAGESZBIG, IMAGESZBIG/2, CPL_TYPE_FLOAT);
        cpl_image_reject(img, IMAGESZBIG, IMAGESZBIG/2);
        cpl_imagelist_set(imlist, img, i);
    }
    img = cpl_imagelist_collapse_median_create(imlist);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(img);

    cpl_test(cpl_image_is_rejected(img, IMAGESZBIG, IMAGESZBIG/2));

    cpl_image_delete(img);
    cpl_imagelist_delete(imlist);
}
