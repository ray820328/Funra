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
#include <cpl_image_io.h>
#include <cpl_image_bpm.h>
#include <cpl_stats.h>
#include <cpl_msg.h>
#include <cpl_memory.h>
#include <cpl_mask.h>
#include <cpl_test.h>

#include "cpl_apertures.h"

/*-----------------------------------------------------------------------------
                                   Define
 -----------------------------------------------------------------------------*/

#define IMAGESZ     10
#define IMAGESZ2    512

#define CPL_APERTS_CMP_IMAGE(OP)                                               \
    do {                                                                       \
        cpl_test_abs( CPL_CONCAT(cpl_apertures_get, OP)(aperts, 1),            \
                      CPL_CONCAT2X(cpl_image_get, CPL_CONCAT(OP,window))       \
                      (imb, 1, 1, IMAGESZ, IMAGESZ),                           \
                       4.0 * DBL_EPSILON);                                     \
    } while (0)    



/*-----------------------------------------------------------------------------
                                  Private functions
 -----------------------------------------------------------------------------*/

static void cpl_apertures_test_one(const cpl_apertures *, FILE *);

/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/
int main(void)
{
    const cpl_type img_type[] = {CPL_TYPE_DOUBLE, CPL_TYPE_FLOAT, CPL_TYPE_INT};
    cpl_size (*ap_get_cpl_size[])(const cpl_apertures *, cpl_size) =
        {cpl_apertures_get_maxpos_x,
         cpl_apertures_get_maxpos_y,
         cpl_apertures_get_minpos_x,
         cpl_apertures_get_minpos_y,
         cpl_apertures_get_npix,
         cpl_apertures_get_left,
         cpl_apertures_get_left_y,
         cpl_apertures_get_right,
         cpl_apertures_get_right_y,
         cpl_apertures_get_top_x,
         cpl_apertures_get_top,
         cpl_apertures_get_bottom_x,
         cpl_apertures_get_bottom};
    const size_t napsize = sizeof(ap_get_cpl_size)/sizeof(*ap_get_cpl_size);

    double (*ap_get_double_pos[])(const cpl_apertures *, cpl_size) = {
        cpl_apertures_get_pos_x,
        cpl_apertures_get_pos_y,
        cpl_apertures_get_centroid_x,
        cpl_apertures_get_centroid_y};
    const size_t napdpos = sizeof(ap_get_double_pos)/sizeof(*ap_get_double_pos);

    double (*ap_get_double[])(const cpl_apertures *, cpl_size) = {
        cpl_apertures_get_max,
        cpl_apertures_get_min,
        cpl_apertures_get_mean,
        cpl_apertures_get_median,
        cpl_apertures_get_stdev,
        cpl_apertures_get_flux};
    const size_t napdbl = sizeof(ap_get_double)/sizeof(*ap_get_double);

    size_t       isize;
    cpl_image     * imd;
    cpl_mask      * bpm;
    cpl_apertures * aperts;
    cpl_apertures * nullapert;
    double        * val;
    const double    psigmas[] = {5, 2, 1, 0.5};
    const double    maxval = 10.0;
    cpl_vector    * sigmas;
    unsigned        itype;
    FILE          * stream;
    cpl_error_code  error;
    cpl_size        errsize, nsize, pos;
    cpl_boolean     is_bench;
    int             nreps, ireps;

    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    /* Insert tests below */

    stream = cpl_msg_get_level() > CPL_MSG_INFO
        ? fopen("/dev/null", "a") : stdout;

    is_bench = cpl_msg_get_level() > CPL_MSG_INFO ? CPL_FALSE : CPL_TRUE;
    nreps = is_bench ? 10 : 1;

    cpl_msg_info(cpl_func, "Repetitions for benchmarking: %d", nreps);

    /* Test 1: NULL pointers */
    cpl_apertures_delete(NULL); /* Just verify the return */
    cpl_apertures_dump(NULL, NULL); /* Just verify the return */
    cpl_apertures_dump(NULL, stream); /* Just verify the return */

    for (isize = 0; isize < napsize; isize++) {
        errsize = ap_get_cpl_size[isize](NULL, 1);
        cpl_test_error(CPL_ERROR_NULL_INPUT);
        cpl_test_lt(errsize, 0.0);
    }

    for (isize = 0; isize < napdpos; isize++) {
        const double errpos = ap_get_double_pos[isize](NULL, 1);
        cpl_test_error(CPL_ERROR_NULL_INPUT);
        cpl_test_lt(errpos, 0.0);
    }

    for (isize = 0; isize < napdbl; isize++) {
        (void)ap_get_double[isize](NULL, 1);
        cpl_test_error(CPL_ERROR_NULL_INPUT);
    }

    /* Statistics computation */
    nullapert = cpl_apertures_new_from_image(NULL, NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(nullapert);

    /* Accessor functions */
    errsize = cpl_apertures_get_size(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_eq(errsize, -1);


    /* Sorting functions */
    error = cpl_apertures_sort_by_npix(NULL);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

    error = cpl_apertures_sort_by_max(NULL);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

    error = cpl_apertures_sort_by_flux(NULL);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

    /* Detection functions */
    nullapert = cpl_apertures_extract(NULL, NULL, NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(nullapert);
    nullapert = cpl_apertures_extract_window(NULL, NULL, 1, 1, 2, 2, NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(nullapert);
    nullapert = cpl_apertures_extract_sigma(NULL, 1.0);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(nullapert);

    imd = cpl_image_new(IMAGESZ, IMAGESZ, CPL_TYPE_DOUBLE);

    val = cpl_image_get_data_double(imd);
    val[33] = val[34] = val[35] = val[36] = 1.0;
    val[43] = val[44] = val[45] = val[46] = 1.0;
    val[53] = val[54] = val[56] = 1.0;
    val[55] = maxval;
    val[63] = val[64] = val[65] = val[66] = 1.0;
    val[88] = 2.0 * maxval;

    bpm = cpl_mask_threshold_image_create(imd, 0.5, 0.5 + maxval);
    error = cpl_mask_not(bpm);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    for (ireps = 0; ireps < nreps; ireps++) {
        for (itype = 0; itype < sizeof(img_type)/sizeof(img_type[0]); itype++) {
            const cpl_size naperts = 2;
            const cpl_type type = img_type[itype];
            cpl_image * labels = cpl_image_new(IMAGESZ, IMAGESZ, CPL_TYPE_INT);
            cpl_image * img = cpl_image_cast(imd, type);
            cpl_image * imb = cpl_image_duplicate(img);
            cpl_mask  * bin;
            cpl_size    xpos, ypos;

            error = cpl_image_reject_from_mask(imb, bpm);
            cpl_test_eq_error(error, CPL_ERROR_NONE);

            /* Create test image and the labels */
            cpl_msg_info(cpl_func, "Testing with a %d X %d %s-image",
                         IMAGESZ, IMAGESZ, cpl_type_get_name(type));

            /* Test 3: with a 'bad' label image */
            nullapert = cpl_apertures_new_from_image(img, NULL);
            cpl_test_error(CPL_ERROR_NULL_INPUT);
            cpl_test_null(nullapert);

            cpl_test_nonnull(labels);

            nullapert = cpl_apertures_new_from_image(img, labels);
            cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
            cpl_test_null(nullapert);

            error = cpl_image_set(labels, 1, 1, 1.0);
            cpl_test_eq_error(error, CPL_ERROR_NONE);

            aperts = cpl_apertures_new_from_image(img, labels);
            cpl_test_error(CPL_ERROR_NONE);
            cpl_test_nonnull(aperts);

            cpl_apertures_test_one(aperts, stream);

            nsize = cpl_apertures_get_size(aperts);
            cpl_test_error(CPL_ERROR_NONE);
            cpl_test_eq(nsize, 1);

            cpl_apertures_delete(aperts);

            if (type != CPL_TYPE_INT) {
                /* Test (error) handling of a complex image */
                cpl_image * imgc = cpl_image_cast(imd, type | CPL_TYPE_COMPLEX);

                aperts = cpl_apertures_new_from_image(imgc, labels);
                cpl_test_error(CPL_ERROR_TYPE_MISMATCH);
                cpl_test_null(aperts);
                cpl_image_delete(imgc);
            }

            /* Test other error handling */
            error = cpl_image_set(labels, 1, 1, 2.0);
            cpl_test_eq_error(error, CPL_ERROR_NONE);

            aperts = cpl_apertures_new_from_image(img, labels);
            cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);
            cpl_test_null(aperts);

            cpl_image_delete(labels);

            /* Test 4: Thresholding */
            bin = cpl_mask_threshold_image_create(img, 0.5, DBL_MAX);
            cpl_test_nonnull(bin);
            labels = cpl_image_labelise_mask_create(bin, NULL);
            cpl_test_nonnull(labels);
            cpl_mask_delete(bin);

            /* Compute statistics on the apertures */
            cpl_msg_info("","Compute statistics on detected apertures");
            aperts = cpl_apertures_new_from_image(img, labels);
            cpl_test_error(CPL_ERROR_NONE);
            cpl_test_nonnull(aperts);

            cpl_apertures_dump(aperts, NULL); /* Just verify the return */

            nsize = cpl_apertures_get_size(aperts);
            cpl_test_error(CPL_ERROR_NONE);
            cpl_test_eq(nsize, naperts);

            error = cpl_apertures_sort_by_npix(aperts);
            cpl_test_eq_error(error, CPL_ERROR_NONE);

            cpl_apertures_test_one(aperts, stream);

            cpl_msg_info("","Compare statistics with those from same "
                         "image with %" CPL_SIZE_FORMAT "rejected pixels",
                         cpl_image_count_rejected(imb));

            cpl_test_eq(cpl_apertures_get_npix(aperts, 1),
                        IMAGESZ * IMAGESZ - cpl_image_count_rejected(imb));

            CPL_APERTS_CMP_IMAGE(min);
            CPL_APERTS_CMP_IMAGE(max);
            CPL_APERTS_CMP_IMAGE(mean);
            CPL_APERTS_CMP_IMAGE(median);
            CPL_APERTS_CMP_IMAGE(stdev);
            CPL_APERTS_CMP_IMAGE(flux);
            CPL_APERTS_CMP_IMAGE(centroid_x);
            CPL_APERTS_CMP_IMAGE(centroid_y);

            error = cpl_image_get_maxpos(imb, &xpos, &ypos);

            cpl_test_eq(cpl_apertures_get_left(aperts, 1), 4);
            cpl_test_eq(cpl_apertures_get_right(aperts, 1), 7);
            cpl_test_eq(cpl_apertures_get_left_y(aperts, 1), 4);
            cpl_test_eq(cpl_apertures_get_right_y(aperts, 1), 4);

            cpl_test_eq(cpl_apertures_get_bottom(aperts, 1), 4);
            cpl_test_eq(cpl_apertures_get_top(aperts, 1), 7);
            cpl_test_eq(cpl_apertures_get_bottom_x(aperts, 1), 4);
            cpl_test_eq(cpl_apertures_get_top_x(aperts, 1), 4);

            cpl_test_leq(cpl_apertures_get_left(aperts, 1), xpos);
            cpl_test_leq(xpos, cpl_apertures_get_right(aperts, 1));

            cpl_test_leq(cpl_apertures_get_bottom(aperts, 1), ypos);
            cpl_test_leq(ypos, cpl_apertures_get_top(aperts, 1));

            cpl_apertures_delete(aperts);
    
            /* Set a bad pixel and recompute the apertures */
            error = cpl_image_reject(img, 6, 6);
            cpl_test_eq_error(error, CPL_ERROR_NONE);

            aperts = cpl_apertures_new_from_image(img, labels);
            cpl_test_error(CPL_ERROR_NONE);
            cpl_test_nonnull(aperts);

            cpl_apertures_test_one(aperts, stream);

            nsize = cpl_apertures_get_size(aperts);
            cpl_test_error(CPL_ERROR_NONE);
            cpl_test_eq(nsize, naperts);

            error = cpl_image_set(labels, IMAGESZ, IMAGESZ, -1.0);
            nullapert = cpl_apertures_new_from_image(img, labels);
            cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
            cpl_test_null(nullapert);

            cpl_image_delete(labels);

            /* Get the biggest object */
            cpl_msg_info("","Get the biggest aperture:");
            error = cpl_apertures_sort_by_npix(aperts);
            cpl_test_eq_error(error, CPL_ERROR_NONE);
            cpl_msg_info("","number of pixels: %" CPL_SIZE_FORMAT,
                         cpl_apertures_get_npix(aperts, 1));

            cpl_apertures_test_one(aperts, stream);

            cpl_test_leq(cpl_apertures_get_npix(aperts, 2),
                         cpl_apertures_get_npix(aperts, 1));

            /* Get the brightest object */
            cpl_msg_info("","Get the brightest aperture:");
            error = cpl_apertures_sort_by_flux(aperts);
            cpl_test_eq_error(error, CPL_ERROR_NONE);
            cpl_msg_info("","flux: %g", cpl_apertures_get_flux(aperts, 1));

            cpl_apertures_test_one(aperts, stream);

            cpl_test_leq(cpl_apertures_get_flux(aperts, 2),
                         cpl_apertures_get_flux(aperts, 1));

            /* Get the aperture with the highest peak */
            cpl_msg_info("","Get the aperture with the highest peak:");
            error = cpl_apertures_sort_by_max(aperts);
            cpl_test_eq_error(error, CPL_ERROR_NONE);
            cpl_msg_info("","maximum value: %g", cpl_apertures_get_max(aperts, 1));

            cpl_apertures_test_one(aperts, stream);

            cpl_test_abs(cpl_apertures_get_max(aperts, 1),
                         cpl_image_get_max(img), 0.0);

            cpl_test_leq(cpl_apertures_get_max(aperts, 2),
                         cpl_apertures_get_max(aperts, 1));

            for (isize = 0; isize < napsize; isize++) {
                errsize = ap_get_cpl_size[isize](aperts, 0);
                cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
                cpl_test_lt(errsize, 0.0);
                errsize = ap_get_cpl_size[isize](aperts, 1+naperts);
                cpl_test_error(CPL_ERROR_ACCESS_OUT_OF_RANGE);
                cpl_test_lt(errsize, 0.0);
            }

            for (isize = 0; isize < napdpos; isize++) {
                double errpos = ap_get_double_pos[isize](aperts, 0);
                cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
                cpl_test_lt(errpos, 0.0);
                errpos = ap_get_double_pos[isize](aperts, 1+naperts);
                cpl_test_error(CPL_ERROR_ACCESS_OUT_OF_RANGE);
                cpl_test_lt(errpos, 0.0);
            }

            for (isize = 0; isize < napdbl; isize++) {
                (void)ap_get_double[isize](aperts, 0);
                cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
                (void)ap_get_double[isize](aperts, 1+naperts);
                cpl_test_error(CPL_ERROR_ACCESS_OUT_OF_RANGE);
            }

            cpl_apertures_delete(aperts);
            cpl_image_delete(imb);
            cpl_image_delete(img);

        }
    }

    cpl_image_delete(imd);
 
    /* Create a new test image */
    imd = cpl_image_fill_test_create(IMAGESZ2, IMAGESZ2);
    
    sigmas = cpl_vector_wrap(4, (double*)psigmas);

    /* Test cpl_apertures_extract() */
    cpl_msg_info("","Test the apertures detection with a thresholding method");
    aperts = cpl_apertures_extract(imd, sigmas, &pos);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(aperts);

    cpl_apertures_test_one(aperts, stream);

    cpl_apertures_delete(aperts);

    /* Test error handling of cpl_apertures_extract_sigma() */
    nullapert = cpl_apertures_extract_sigma(imd, 100.0);
    cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);
    cpl_test_null(nullapert);

    /* Test error handling of cpl_apertures_extract_mask() */
    nullapert = cpl_apertures_extract_mask(imd, NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(nullapert);

    cpl_mask_delete(bpm);
    bpm = cpl_mask_new(IMAGESZ2, IMAGESZ2);
    nullapert = cpl_apertures_extract_mask(imd, bpm);
    cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);
    cpl_test_null(nullapert);

    /* Test cpl_apertures_extract_window() */
    cpl_msg_info("","Test the apertures detection (threshold) on a zone");
    aperts = cpl_apertures_extract_window(imd, sigmas, IMAGESZ2/4, IMAGESZ2/4,
                                          3*IMAGESZ2/4, 3*IMAGESZ2/4, &pos);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(aperts);

    cpl_apertures_test_one(aperts, stream);

    nsize = cpl_apertures_get_size(aperts);
    cpl_test_error(CPL_ERROR_NONE);

    error = cpl_apertures_sort_by_max(aperts);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_eq(nsize, cpl_apertures_get_size(aperts));

    cpl_apertures_test_one(aperts, stream);

    for (pos = 1; pos < nsize; pos++) {
        cpl_test_leq(cpl_apertures_get_max(aperts, pos+1),
                     cpl_apertures_get_max(aperts, pos));
    }

    error = cpl_apertures_sort_by_npix(aperts);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_eq(nsize, cpl_apertures_get_size(aperts));

    cpl_apertures_test_one(aperts, stream);

    for (pos = 1; pos < nsize; pos++) {
        cpl_test_leq(cpl_apertures_get_npix(aperts, pos+1),
                     cpl_apertures_get_npix(aperts, pos));
    }


    error = cpl_apertures_sort_by_flux(aperts);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_eq(nsize, cpl_apertures_get_size(aperts));

    cpl_apertures_test_one(aperts, stream);

    for (pos = 1; pos < nsize; pos++) {
        cpl_test_leq(cpl_apertures_get_flux(aperts, pos+1),
                     cpl_apertures_get_flux(aperts, pos));
    }

    cpl_vector_unwrap(sigmas);
    cpl_apertures_delete(aperts);
    cpl_image_delete(imd);
    cpl_mask_delete(bpm);

    if (stream != stdout) cpl_test_zero( fclose(stream) );

    return cpl_test_end(0);
}



/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Subject the aperture to some self-consistency checks
  @param  self    The object to test
  @param  stream  The stream to dump to
  @return void

 */
/*----------------------------------------------------------------------------*/
static void cpl_apertures_test_one(const cpl_apertures * self, FILE * stream)
{
    const cpl_size nsize = cpl_apertures_get_size(self);
    cpl_size i;

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test(nsize);

    cpl_apertures_dump(self, stream);

    for (i = 1; i <= nsize; i++) {
        const cpl_size npix = cpl_apertures_get_npix(self, i);

        cpl_test_error(CPL_ERROR_NONE);

        cpl_test(npix);

        if (npix == 1) {
            const double stdev_err = cpl_apertures_get_stdev(self, i);

            cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);
            cpl_test_lt(stdev_err, 0.0);
        }

        cpl_test_leq(cpl_apertures_get_left(self, i),
                     cpl_apertures_get_pos_x(self, i));
        cpl_test_leq(cpl_apertures_get_pos_x(self, i),
                     cpl_apertures_get_right(self, i));

        cpl_test_leq(cpl_apertures_get_bottom(self, i),
                     cpl_apertures_get_pos_y(self, i));
        cpl_test_leq(cpl_apertures_get_pos_y(self, i),
                     cpl_apertures_get_top(self, i));

        cpl_test_leq(cpl_apertures_get_left(self, i),
                     cpl_apertures_get_maxpos_x(self, i));
        cpl_test_leq(cpl_apertures_get_maxpos_x(self, i),
                     cpl_apertures_get_right(self, i));

        cpl_test_leq(cpl_apertures_get_bottom(self, i),
                     cpl_apertures_get_maxpos_y(self, i));
        cpl_test_leq(cpl_apertures_get_maxpos_y(self, i),
                     cpl_apertures_get_top(self, i));

        cpl_test_leq(cpl_apertures_get_left(self, i),
                     cpl_apertures_get_minpos_x(self, i));
        cpl_test_leq(cpl_apertures_get_minpos_x(self, i),
                     cpl_apertures_get_right(self, i));

        cpl_test_leq(cpl_apertures_get_bottom(self, i),
                     cpl_apertures_get_minpos_y(self, i));
        cpl_test_leq(cpl_apertures_get_minpos_y(self, i),
                     cpl_apertures_get_top(self, i));

        cpl_test_leq(cpl_apertures_get_min(self, i),
                     cpl_apertures_get_mean(self, i));
        cpl_test_leq(cpl_apertures_get_mean(self, i),
                     cpl_apertures_get_max(self, i));

        cpl_test_leq(cpl_apertures_get_min(self, i),
                     cpl_apertures_get_median(self, i));
        cpl_test_leq(cpl_apertures_get_median(self, i),
                     cpl_apertures_get_max(self, i));

        cpl_test_error(CPL_ERROR_NONE);
    }
}
