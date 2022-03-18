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
#include <cpl_imagelist_io.h>
#include <cpl_image_io.h>
#include <cpl_image_basic.h>
#include <cpl_bivector.h>
#include <cpl_vector.h>
#include <cpl_memory.h>
#include <cpl_msg.h>
#include <cpl_test.h>
#include <cpl_plot.h>

/* cpl_drand() */
#include <cpl_tools.h>

#include "cpl_apertures.h"
#include "cpl_geom_img.h"

/*-----------------------------------------------------------------------------
                                Define
 -----------------------------------------------------------------------------*/

#ifndef IMAGESZ
#define     IMAGESZ             256
#endif
#define     NFRAMES             10
#define     NSIGMAS              4
#define     MAX_SHIFT_ERROR1    15
#define     MAX_SHIFT_ERROR2    0.1

/*-----------------------------------------------------------------------------
                                  Pricate functions
 -----------------------------------------------------------------------------*/

static void cpl_geom_img_offset_saa_one(cpl_kernel);

static
void cpl_geom_img_offset_saa_bench(cpl_geom_combine, int, int, int, int, int);

static void cpl_imagelist_fill_shifted(cpl_imagelist *, cpl_size,
                                       const double *,  const double *);

/**@{*/

/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/
int main(void)
{
    /* These kernels preserve the actual pixel-values */
    cpl_kernel kernels[] = {CPL_KERNEL_DEFAULT,
                            CPL_KERNEL_NEAREST};
    const cpl_geom_combine geoms[] = {CPL_GEOM_INTERSECT, CPL_GEOM_UNION,
                                      CPL_GEOM_FIRST};
    /* Shift by non-integer amount to evaluate resampling */
    const double        off_x_init[] = { 0.0, -6.5, -18.5, 54.5,  33.5,
                                        46.5, -3.5,  36.5, 42.5, -13.5};
    const double        off_y_init[] = { 0.0, 13.5,   3.5, 8.5,  32.5,
                                        22.5, 18.5, -56.5, 3.5,  10.5};
    cpl_imagelist   *   iset;
    cpl_image       *   img;
    cpl_bivector    *   offs_est;
    cpl_vector      *   off_vec_x;
    cpl_vector      *   off_vec_y;
    cpl_bivector    *   offs_ref;
    cpl_apertures   *   aperts;
    int                 naperts;
    cpl_bivector    *   aperts_pos;
    cpl_vector      *   aperts_pos_x;
    cpl_vector      *   aperts_pos_y;
    cpl_vector      *   correl;
    const double        psigmas[] = {5, 2, 1, 0.5};
    cpl_vector      *   sigmas;
    cpl_image       **  combined;
    int                 i;
    cpl_size            pos;


    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    /* Verify the test data */
    cpl_test_eq(sizeof(off_x_init), NFRAMES * sizeof(off_x_init[0]));
    cpl_test_eq(sizeof(off_y_init), sizeof(off_x_init));
    cpl_test_eq(sizeof(psigmas),    NSIGMAS * sizeof(psigmas[0]));
    for (i = 0; i < NFRAMES; i++) {
        cpl_test_leq(fabs(off_x_init[i]), IMAGESZ);
        cpl_test_leq(fabs(off_y_init[i]), IMAGESZ);
    }

    cpl_geom_img_offset_saa_one(CPL_KERNEL_DEFAULT);
    cpl_geom_img_offset_saa_one(CPL_KERNEL_NEAREST);

    if (cpl_msg_get_level() <= CPL_MSG_INFO) {
        const double tprev = cpl_test_get_cputime();
        const cpl_flops fprev = cpl_tools_get_flops();
        double tpost, cputime;
        cpl_flops fpost, nflops;

#ifndef _OPENMP
        cpl_geom_img_offset_saa_bench(CPL_GEOM_FIRST, 10, 16, 4*IMAGESZ,
                                      4*IMAGESZ, 0);
        cpl_geom_img_offset_saa_bench(CPL_GEOM_FIRST,  6, 18, 4*IMAGESZ,
                                      4*IMAGESZ, 1);
#endif

#ifdef _OPENMP
#pragma omp parallel for private(i)
#endif
        for (i=0; i < 8; i++) {
            cpl_geom_img_offset_saa_bench(CPL_GEOM_FIRST,  6, 18, 4*IMAGESZ,
                                          4*IMAGESZ, 1);
        }

        tpost = cpl_test_get_cputime();
        fpost = cpl_tools_get_flops();
        cputime = tpost - tprev;
        nflops = fpost - fprev;
        cpl_msg_info(cpl_func, "Time to benchmark [s]: %g (%g MFLOP/s)",
                     cputime,
                     cputime > 0.0 ? (double)nflops/cputime/1e6 : 0.0);

    } else {
        cpl_geom_img_offset_saa_bench(CPL_GEOM_FIRST, 1, 4, IMAGESZ/4,
                                      IMAGESZ/4, 1);
    }


    /* Bivector with 1 zero-valued element */
    off_vec_x = cpl_vector_new(1);
    cpl_vector_set(off_vec_x, 0, 0.0);
    offs_ref = cpl_bivector_wrap_vectors(off_vec_x, off_vec_x);


    /* Test with empty imagelist */
    iset = cpl_imagelist_new();

    combined = cpl_geom_img_offset_saa(iset, offs_ref,
                                       CPL_KERNEL_DEFAULT,
                                       0, 0, CPL_GEOM_FIRST,
                                       NULL, NULL);

    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(combined);

    /* Insert one image into imagelist */
    img = cpl_image_fill_test_create(IMAGESZ, IMAGESZ);
    cpl_imagelist_set(iset, img, 0);

    for (i = 0; i < (int)(sizeof(geoms)/sizeof(geoms[0])); i++) {
        const cpl_geom_combine geom = geoms[i];

        /* Shift and add */
        cpl_msg_info("", "Shift and add single image with geom number %d",
                     (int)geom);
        combined = cpl_geom_img_offset_saa(iset, offs_ref,
                                           CPL_KERNEL_DEFAULT,
                                           0, 0, geom,
                                           NULL, NULL);

        cpl_test_error(CPL_ERROR_NONE);

        cpl_test_nonnull(combined);
        cpl_test_nonnull(combined[0]);
        cpl_test_nonnull(combined[1]);

        cpl_test_eq(cpl_image_get_type(combined[1]), CPL_TYPE_INT);

        cpl_test_eq(cpl_image_get_min(combined[1]), 1);
        cpl_test_eq(cpl_image_get_max(combined[1]), 1);

        cpl_test_zero(cpl_image_count_rejected(combined[0]));

        cpl_test_eq(cpl_image_get_size_x(combined[0]),
                    cpl_image_get_size_x(combined[1]));

        cpl_test_eq(cpl_image_get_size_y(combined[0]),
                    cpl_image_get_size_y(combined[1]));

        cpl_test_image_abs(combined[0], cpl_imagelist_get_const(iset, 0), 0.0);

        cpl_image_delete(combined[0]);
        cpl_image_delete(combined[1]);
        cpl_free(combined);
    }

    cpl_bivector_unwrap_vectors(offs_ref);
    cpl_vector_delete(off_vec_x);

    cpl_imagelist_fill_shifted(iset, NFRAMES-1, off_x_init, off_y_init);

    cpl_test_eq(cpl_imagelist_get_size(iset), NFRAMES);
    
    /* Not modified */
    off_vec_x = cpl_vector_wrap(NFRAMES, (double*)off_x_init);
    off_vec_y = cpl_vector_wrap(NFRAMES, (double*)off_y_init);
    offs_est = cpl_bivector_new(NFRAMES);
    cpl_vector_copy(cpl_bivector_get_x(offs_est), off_vec_x);
    cpl_vector_copy(cpl_bivector_get_y(offs_est), off_vec_y);

    /* Distort the estimate */
    cpl_vector_add_scalar(cpl_bivector_get_x(offs_est), 2.0);
    cpl_vector_add_scalar(cpl_bivector_get_y(offs_est), -3.0);

    sigmas = cpl_vector_wrap(NSIGMAS, (double*)psigmas); /* Not modified */

    cpl_test_error(CPL_ERROR_NONE);

    /* Get some cross-correlation apertures */
    aperts = cpl_apertures_extract(cpl_imagelist_get_const(iset, 0), sigmas,
                                   &pos);
    cpl_vector_unwrap(sigmas);

    cpl_test_nonnull(aperts);

    naperts = cpl_apertures_get_size(aperts);

    cpl_test_leq(1, naperts);

    cpl_msg_info("","Detected %d apertures at sigma %g (%" CPL_SIZE_FORMAT "/%"
                 CPL_SIZE_FORMAT ")", naperts, psigmas[pos], 1+pos,
                 (cpl_size)NSIGMAS);
    if (cpl_msg_get_level() <= CPL_MSG_DEBUG)
        cpl_apertures_dump(aperts, stdout);

    aperts_pos = cpl_bivector_new(naperts);
    aperts_pos_x = cpl_bivector_get_x(aperts_pos);
    aperts_pos_y = cpl_bivector_get_y(aperts_pos);
    for (i=0; i<naperts; i++) {
        cpl_vector_set(aperts_pos_x, i, cpl_apertures_get_pos_x(aperts, i+1));
        cpl_vector_set(aperts_pos_y, i, cpl_apertures_get_pos_y(aperts, i+1));
    }
    cpl_apertures_delete(aperts);

    cpl_test_error(CPL_ERROR_NONE);
   
    /* Refine the offsets with cpl_geom_img_offset_fine */
    cpl_msg_info("","Refine the offsets for %d images using %" CPL_SIZE_FORMAT
                 " anchors", NFRAMES, cpl_bivector_get_size(aperts_pos));
    correl = cpl_vector_new(NFRAMES);

    offs_ref = cpl_geom_img_offset_fine(iset, offs_est, aperts_pos,
                                        15, 15, 15, 15, correl);

    cpl_test_nonnull(offs_ref);

    cpl_test_eq(cpl_bivector_get_size(offs_ref), NFRAMES);

    cpl_vector_delete(correl);
    cpl_bivector_delete(offs_est);
    cpl_bivector_delete(aperts_pos);

    cpl_test_vector_abs(cpl_bivector_get_x(offs_ref), off_vec_x,
                        MAX_SHIFT_ERROR2);
    cpl_test_vector_abs(cpl_bivector_get_y(offs_ref), off_vec_y,
                        MAX_SHIFT_ERROR2);

    cpl_test_nonnull(cpl_vector_unwrap(off_vec_x));
    cpl_test_nonnull(cpl_vector_unwrap(off_vec_y));

    for (i = 0; i < (int)(sizeof(geoms)/sizeof(geoms[0])); i++) {
        const cpl_geom_combine geom = geoms[i];
        const int rejmin = 1;
        const int rejmax = 1;
        const int maximg = NFRAMES - rejmin - rejmax;
        /* Called like this, cpl_geom_img_offset_combine() is just
           a wrapper around cpl_geom_img_offset_saa() */
        cpl_image ** combined2
            = cpl_geom_img_offset_combine(iset, offs_ref, 0, NULL, NULL, NULL,
                                          0, 0, 0, 0, rejmin, rejmax, geom);
        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_nonnull(combined2);

        /* Shift and add */
        cpl_msg_info("", "Shift and add with geom number %d", (int)geom);
        combined = cpl_geom_img_offset_saa(iset, offs_ref,  CPL_KERNEL_DEFAULT,
                                           rejmin, rejmax, geom, NULL, NULL);
        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_nonnull(combined);

        if (combined == NULL) continue;

        cpl_test_image_abs(combined[0], combined2[0], 0.0);
        cpl_test_image_abs(combined[1], combined2[1], 0.0);

        cpl_image_delete(combined2[0]);
        cpl_image_delete(combined2[1]);
        cpl_free(combined2);

        cpl_test_eq(cpl_image_get_type(combined[1]), CPL_TYPE_INT);

        if (cpl_image_get_min(combined[1]) == 0) {
            cpl_test(cpl_image_count_rejected(combined[0]));
        } else {
            cpl_test_leq(1, cpl_image_get_min(combined[1]));
            cpl_test_zero(cpl_image_count_rejected(combined[0]));
        }

        cpl_test_eq(cpl_image_get_size_x(combined[0]),
                    cpl_image_get_size_x(combined[1]));

        cpl_test_eq(cpl_image_get_size_y(combined[0]),
                    cpl_image_get_size_y(combined[1]));

        cpl_test_leq(cpl_image_get_max(combined[1]), maximg);

        if (geom == CPL_GEOM_INTERSECT) {
            cpl_test_eq(cpl_image_get_max(combined[1]), maximg);
            cpl_test_leq(1, cpl_image_get_min(combined[1]));
        } else if (geom == CPL_GEOM_FIRST) {
            /* FIXME: Should at least be 1 */
            cpl_test_leq(0, cpl_image_get_min(combined[1]));
        } else if (geom == CPL_GEOM_UNION) {
            cpl_test_leq(0, cpl_image_get_min(combined[1]));
        }

        cpl_msg_info("", "Minimum value in contribution map: %g",
                     cpl_image_get_min(combined[1]));

        cpl_image_delete(combined[0]);
        cpl_image_delete(combined[1]);
        cpl_free(combined);

    }
    
    /* Shift and add without bad pixels */

    for (i=0; i < NFRAMES; i++) {
        cpl_image_accept_all(cpl_imagelist_get(iset, i));
    }


    for (i = 0; i < (int)(sizeof(geoms)/sizeof(geoms[0])); i++) {
        int ityp;
        for (ityp = 0; ityp < (int)(sizeof(kernels)/sizeof(kernels[0]));
             ityp++) {

            const cpl_geom_combine geom = geoms[i];
            const int rejmin = 1;
            const int rejmax = 1;
            const int maximg = NFRAMES - rejmin - rejmax;

            /* Shift and add */
            cpl_msg_info("", "Shift and add with geom number %d and kernel "
                         "type %d", (int)geom, (int)kernels[ityp]);
            combined = cpl_geom_img_offset_saa(iset, offs_ref, kernels[ityp],
                                               rejmin, rejmax, geom,
                                               NULL, NULL);
            cpl_test_error(CPL_ERROR_NONE);
            cpl_test_nonnull(combined);

            if (combined == NULL) continue;

            cpl_test_eq(cpl_image_get_type(combined[1]), CPL_TYPE_INT);

            if (cpl_image_get_min(combined[1]) == 0) {
                cpl_test(cpl_image_count_rejected(combined[0]));
            } else {
                cpl_test_leq(1, cpl_image_get_min(combined[1]));
                cpl_test_zero(cpl_image_count_rejected(combined[0]));
            }

            cpl_test_eq(cpl_image_get_size_x(combined[0]),
                        cpl_image_get_size_x(combined[1]));

            cpl_test_eq(cpl_image_get_size_y(combined[0]),
                        cpl_image_get_size_y(combined[1]));

            cpl_test_leq(cpl_image_get_max(combined[1]), maximg);

            if (geom == CPL_GEOM_INTERSECT) {
                cpl_test_eq(cpl_image_get_max(combined[1]), maximg);
                cpl_test_leq(1, cpl_image_get_min(combined[1]));
            } else if (geom == CPL_GEOM_FIRST) {
                /* FIXME: Should at least be 1 */
                cpl_test_leq(0, cpl_image_get_min(combined[1]));
            } else if (geom == CPL_GEOM_UNION) {
                cpl_test_leq(0, cpl_image_get_min(combined[1]));
            }

            cpl_msg_info("", "Minimum value in contribution map: %g",
                         cpl_image_get_min(combined[1]));

            cpl_image_delete(combined[0]);
            cpl_image_delete(combined[1]);
            cpl_free(combined);
        }
    }

    cpl_bivector_delete(offs_ref);
    img = cpl_imagelist_unset(iset, 0);
    cpl_imagelist_delete(iset);

    /* Shift and add of two uniform images - with no offsets */

    iset = cpl_imagelist_new();
    cpl_imagelist_set(iset, img, 0);
    cpl_image_threshold(img, 1.0, 1.0, 1.0, 1.0);
    cpl_image_accept_all(img);
    img = cpl_image_duplicate(img);
    cpl_imagelist_set(iset, img, 1);

    off_vec_x = cpl_vector_new(2);
    cpl_vector_fill(off_vec_x, 0.0);
    offs_ref = cpl_bivector_wrap_vectors(off_vec_x,
                                         cpl_vector_duplicate(off_vec_x));

    if (cpl_msg_get_level() <= CPL_MSG_DEBUG)
        cpl_plot_image("","","", img);

    for (i = 0; i < (int)(sizeof(geoms)/sizeof(geoms[0])); i++) {
        int ityp;
        for (ityp = 0; ityp < (int)(sizeof(kernels)/sizeof(kernels[0]));
             ityp++) {

            const cpl_geom_combine geom = geoms[i];
            double pos_x, pos_y;

            cpl_msg_info("", "Shift and add with geom number %d and kernel "
                         "type %d", (int)geom, (int)kernels[ityp]);

            combined = cpl_geom_img_offset_saa(iset, offs_ref, kernels[ityp],
                                               0, 0, geom, &pos_x, &pos_y);

            cpl_test_nonnull(combined);

            if (combined == NULL) continue;

            cpl_test_eq(cpl_image_get_size_x(combined[0]), IMAGESZ);
            cpl_test_eq(cpl_image_get_size_x(combined[0]), IMAGESZ);

            cpl_test_eq(cpl_image_get_type(combined[1]), CPL_TYPE_INT);

            cpl_test_eq(cpl_image_get_size_x(combined[0]),
                        cpl_image_get_size_x(combined[1]));

            cpl_test_eq(cpl_image_get_size_y(combined[0]),
                        cpl_image_get_size_y(combined[1]));

            if (cpl_image_get_min(combined[1]) == 0) {
                cpl_test(cpl_image_count_rejected(combined[0]));
            } else {
                cpl_test_leq(1, cpl_image_get_min(combined[1]));
                cpl_test_zero(cpl_image_count_rejected(combined[0]));
            }

            cpl_test_eq(cpl_image_get_max(combined[1]), 2);

            if (geom == CPL_GEOM_INTERSECT) {
                cpl_test_eq(cpl_image_get_max(combined[1]), 2);
                /* FIXME: Should at least be 1 */
                cpl_test_leq(0, cpl_image_get_min(combined[1]));
            } else if (geom == CPL_GEOM_FIRST) {
                /* FIXME: Minimum value is zero */
                cpl_test_leq(0, cpl_image_get_min(combined[1]));
            } else if (geom == CPL_GEOM_UNION) {
                cpl_test_leq(0, cpl_image_get_min(combined[1]));
            }

#ifdef TEST_RESAMPLING
            /* Resampling introduces noise at the edge */
            /* NB: Comparison works for all modes, due to zero offset ... */
            cpl_test_image_abs(combined[0], img, MAX_SHIFT_ERROR2);
#endif

            if (cpl_msg_get_level() <= CPL_MSG_DEBUG) {
                cpl_image_subtract(combined[0], img);
                cpl_plot_image("","","", combined[0]);
            }

            cpl_image_delete(combined[0]);
            cpl_image_delete(combined[1]);
            cpl_free(combined);


            /* Now try to combine two images, the second shifted along the X-axis */

            cpl_image_shift(img, -MAX_SHIFT_ERROR1, 0);
            cpl_image_accept_all(img);

            cpl_vector_set(off_vec_x, 1, MAX_SHIFT_ERROR1);

            combined = cpl_geom_img_offset_saa(iset, offs_ref, kernels[ityp],
                                               0, 0, geom, &pos_x, &pos_y);

            cpl_test_error(CPL_ERROR_NONE);
            cpl_test_nonnull(combined);

            if (combined == NULL) continue;

            cpl_test_eq(cpl_image_get_max(combined[1]), 2);

            if (cpl_image_get_min(combined[1]) == 0) {
                cpl_test(cpl_image_count_rejected(combined[0]));
            } else {
                cpl_test_zero(cpl_image_count_rejected(combined[0]));
            }

            cpl_test_eq(cpl_image_get_size_y(combined[0]), IMAGESZ);

            if (geom == CPL_GEOM_INTERSECT) {

#ifdef SAVE_COMBINED
                cpl_image_save(combined[0], "PI.fits", CPL_TYPE_DOUBLE,
                               NULL, CPL_IO_CREATE);
                cpl_image_save(combined[1], "CI.fits", CPL_TYPE_UCHAR,
                               NULL, CPL_IO_CREATE);
#endif

                cpl_test_eq(cpl_image_get_size_x(combined[0]), IMAGESZ
                            - MAX_SHIFT_ERROR1);

            } else if (geom == CPL_GEOM_FIRST) {

#ifdef SAVE_COMBINED
                cpl_image_save(combined[0], "PF.fits", CPL_TYPE_DOUBLE,
                               NULL, CPL_IO_CREATE);
                cpl_image_save(combined[1], "CF.fits", CPL_TYPE_UCHAR,
                               NULL, CPL_IO_CREATE);
#endif


                cpl_test_eq(cpl_image_get_size_x(combined[0]), IMAGESZ);

            } else if (geom == CPL_GEOM_UNION) {
                cpl_test_eq(cpl_image_get_size_x(combined[0]), IMAGESZ
                            + MAX_SHIFT_ERROR1);

#ifdef SAVE_COMBINED
                cpl_image_save(combined[0], "PU.fits", CPL_TYPE_DOUBLE,
                               NULL, CPL_IO_CREATE);
                cpl_image_save(combined[1], "CU.fits", CPL_TYPE_UCHAR,
                               NULL, CPL_IO_CREATE);
#endif

                cpl_test_eq(cpl_image_get_min(combined[1]), 1);
            }


            img = cpl_imagelist_get(iset, 0);

            if (cpl_msg_get_level() <= CPL_MSG_DEBUG) {
                cpl_plot_image("","","", combined[0]);

                if (geom == CPL_GEOM_FIRST) {
                    cpl_image_subtract(combined[0], img);
                    cpl_plot_image("","","", combined[0]);
                }
            }

            cpl_image_delete(combined[0]);
            cpl_image_delete(combined[1]);
            cpl_free(combined);

            /* Reset offset and 2nd image */
            cpl_vector_fill(off_vec_x, 0.0);
            img = cpl_image_duplicate(img);
            cpl_imagelist_set(iset, img, 1);

        }
    }

    cpl_imagelist_delete(iset);
    cpl_bivector_delete(offs_ref);

    return cpl_test_end(0);
}

/**@}*/

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief      Benchmark the CPL function
  @param mode CPL_GEOM_INTERSECT, CPL_GEOM_UNION, CPL_GEOM_FIRST
  @param nr   The number of repeats
  @param nz   The number of planes
  @param nx   The image X-size
  @param ny   The image Y-size
  @param no   The number of outlier pixels to ignore (both min and max)
  @return    void
 */
/*----------------------------------------------------------------------------*/
static
void cpl_geom_img_offset_saa_bench(cpl_geom_combine mode, int nr, int nz,
                                   int nx, int ny, int no)
{

    cpl_bivector  * offset  = cpl_bivector_new(nz);
    cpl_vector    * off_x   = cpl_bivector_get_x(offset);
    cpl_vector    * off_y   = cpl_bivector_get_y(offset);
    cpl_image     * imgd    = cpl_image_fill_test_create(nx, ny);
    cpl_image     * img     = cpl_image_cast(imgd, CPL_TYPE_FLOAT);
    cpl_imagelist * imglist = cpl_imagelist_new();
    cpl_image    ** combined;
    double          cputime = 0.0;
    size_t          bytes = 0;
    cpl_flops       nflops = 0;

    int ir, iz;

    cpl_test_leq(1, nz);

    /* Create bivector of shifts, from 0.4 to 0.6 of the pixel range */
    /* Create list of shifted images */
    cpl_vector_set(off_x, 0, 0.0);
    cpl_vector_set(off_y, 0, 0.0);

    cpl_image_delete(imgd);

    cpl_imagelist_set(imglist, img, 0);


    for (iz = 1; iz < nz; iz++) {
        cpl_image * copy = cpl_image_duplicate(img);
        const int dx = (int)(0.1 * nx - 0.2 * nx * cpl_drand());
        const int dy = (int)(0.1 * ny - 0.2 * ny * cpl_drand());

        cpl_vector_set(off_x, iz, (double)dx);
        cpl_vector_set(off_y, iz, (double)dy);

        cpl_image_shift(copy, -dx, -dy);
        cpl_image_accept_all(copy);
        cpl_imagelist_set(imglist, copy, iz);
    }

    if (cpl_msg_get_level() <= CPL_MSG_DEBUG)
        cpl_bivector_dump(offset, stdout);

    bytes = (size_t)nr * cpl_test_get_bytes_imagelist(imglist);

    for (ir = 0; ir < nr; ir++) {
        const cpl_flops flops  = cpl_tools_get_flops();
        const double secs = cpl_test_get_cputime();

        combined = cpl_geom_img_offset_saa(imglist, offset, CPL_KERNEL_DEFAULT,
                                           no, no, mode, NULL, NULL);
        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_nonnull(combined);

        cputime += cpl_test_get_cputime() - secs;
        nflops += cpl_tools_get_flops() - flops;

        if (combined == NULL) continue;

        cpl_test_nonnull(combined[0]);
        cpl_test_nonnull(combined[1]);

        cpl_image_delete(combined[0]);
        cpl_image_delete(combined[1]);
        cpl_free(combined);
    }

    cpl_msg_info(cpl_func, "Time to benchmark with mode=%d, nr=%d, nz=%d, "
                 "nx=%d, ny=%d, no=%d [s]: %g (%g MFLOP/s)",
                 mode, nr, nz, nx, ny, no, cputime,
                 cputime > 0.0 ? (double)nflops/cputime/1e6 : 0.0);
    if (cputime > 0.0) {
        cpl_msg_info(cpl_func,"Processing rate [MB/s]: %g",
                     1e-6 * (double)bytes / cputime);
    }

    cpl_bivector_delete(offset);
    cpl_imagelist_delete(imglist);

    return;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief      Create imagelist of images shifted from the 1st image
  @param self Imagelist with one image to append to
  @param napp The number of shifted images to append
  @param dx   The array of n+1 X-shifts (0.0 as 1st element)
  @param dy   The array of n+1 Y-shifts (0.0 as 1st element)
  @return    void
  @note On return the number of images in self will be n+1

 */
/*----------------------------------------------------------------------------*/
static
void cpl_imagelist_fill_shifted(cpl_imagelist * self, cpl_size napp,
                                const double * dx, const double * dy)
{

    const cpl_image * img         = cpl_imagelist_get_const(self, 0);
    const cpl_size    type        = cpl_image_get_type  (img);
    const cpl_size    nx          = cpl_image_get_size_x(img);
    const cpl_size    ny          = cpl_image_get_size_x(img);
    const cpl_size    ishift_0[2] = {0, 0};
    const cpl_size    ishift_x[2] = {1, 0};
    const cpl_size    ishift_y[2] = {0, 1};

    const double      xyradius = CPL_KERNEL_DEF_WIDTH;
  
    cpl_vector      * xyprofile = cpl_vector_new(CPL_KERNEL_DEF_SAMPLES);

    cpl_polynomial  * shift_x = cpl_polynomial_new(2);
    cpl_polynomial  * shift_y = cpl_polynomial_new(2);
    cpl_error_code    error;
    cpl_size          i;


    cpl_test_eq(cpl_imagelist_get_size(self), 1);
    cpl_test_leq(1, napp);
    cpl_test_nonnull(dx);
    cpl_test_nonnull(dy);


    /* Identity transforms */
    error = cpl_polynomial_set_coeff(shift_x, ishift_x, 1.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_polynomial_set_coeff(shift_y, ishift_y, 1.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    /* Resampling profile */
    error = cpl_vector_fill_kernel_profile(xyprofile, CPL_KERNEL_DEFAULT,
                                           xyradius);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    /* Append images to image set */
    for (i=1; i < napp+1; i++) {
        cpl_image * copy = cpl_image_new(nx, ny, type);

        /* Shift in X and Y */
        error = cpl_polynomial_set_coeff(shift_x, ishift_0, dx[i]);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_polynomial_set_coeff(shift_y, ishift_0, dy[i]);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_image_warp_polynomial(copy, img, shift_x, shift_y,
                                          xyprofile, xyradius,
                                          xyprofile, xyradius);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_imagelist_set(self, copy, i);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

    }

    cpl_polynomial_delete(shift_x);
    cpl_polynomial_delete(shift_y);
    cpl_vector_delete(xyprofile);

    cpl_test_eq(cpl_imagelist_get_size(self), napp+1);

}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief      Test the CPL function
  @param     kernel Kernel type
  @return    void
  @note On return the number of images in self will be n+1

 */
/*----------------------------------------------------------------------------*/
static
void cpl_geom_img_offset_saa_one(cpl_kernel kernel)
{

    const int        nz = 2 + NFRAMES;
    cpl_imagelist * imglist = cpl_imagelist_new();
    cpl_image    ** combined;
    cpl_bivector  * offset  = cpl_bivector_new(nz);
    cpl_vector    * off_x   = cpl_bivector_get_x(offset);
    cpl_vector    * off_y   = cpl_bivector_get_y(offset);
    cpl_error_code  error;
    cpl_size        iz;
    cpl_image     * central0;
    cpl_image     * central1;

    for (iz = 0; iz < nz; iz++) {
        cpl_image * img = cpl_image_new(IMAGESZ, IMAGESZ, CPL_TYPE_FLOAT);

        cpl_test_nonnull(img);

        /* Insert flat images with known sum of the non-rejected planes */
        error = cpl_image_add_scalar(img, (double)(nz - iz - nz/5));
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_imagelist_set(imglist, img, iz);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        cpl_vector_set(off_x, iz, iz ? cpl_drand() : 0.0);
        cpl_vector_set(off_y, iz, iz ? cpl_drand() : 0.0);
    }

    combined = cpl_geom_img_offset_saa(imglist, offset, kernel, nz/5, nz/4,
                                       CPL_GEOM_INTERSECT, NULL, NULL);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(combined);

    error = cpl_image_dump_structure(combined[1], stdout);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_eq(cpl_image_get_max(combined[0]),
                (nz - nz/5 - nz/4 + 1) /2.0);
    cpl_test_eq(cpl_image_get_max(combined[1]), nz - nz/5
                - nz/4);

    central0 = cpl_image_extract(combined[0], 3, 3, IMAGESZ - 2, IMAGESZ - 2);
    central1 = cpl_image_extract(combined[1], 3, 3, IMAGESZ - 2, IMAGESZ - 2);

    cpl_test_eq(cpl_image_get_min(central0),
                (nz - nz/5 - nz/4 + 1) /2.0);
    cpl_test_eq(cpl_image_get_min(central1), nz - nz/5 - nz/4);

    cpl_image_delete(combined[0]);
    cpl_image_delete(combined[1]);
    cpl_free(combined);

    cpl_image_delete(central0);
    cpl_image_delete(central1);

    cpl_imagelist_delete(imglist);
    cpl_bivector_delete(offset);

}
