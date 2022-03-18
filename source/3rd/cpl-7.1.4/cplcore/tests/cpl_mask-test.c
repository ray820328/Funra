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

#include "cpl_mask.h"
#include "cpl_tools.h"
#include "cpl_memory.h"
#include "cpl_fits.h"
#include "cpl_test.h"

#include <string.h>
#include <math.h>

/*-----------------------------------------------------------------------------
                                  Defines
 -----------------------------------------------------------------------------*/

#ifndef IMAGESZ
#define IMAGESZ 512
#endif

#ifndef MASKSZ
#define MASKSZ 8
#endif

#define FILENAME "cpl_mask_test.fits"

/*-----------------------------------------------------------------------------
                                  Private functions
 -----------------------------------------------------------------------------*/

static cpl_error_code cpl_mask_shift_bf(cpl_mask *, int, int, cpl_binary);
static cpl_error_code cpl_mask_shift_filter(cpl_mask *, int, int,
                                            cpl_filter_mode);
static void cpl_mask_flip_turn_test(const cpl_mask *);
static void cpl_mask_shift_test(int, int);
static void cpl_mask_test(cpl_boolean);
static void cpl_mask_filter_test(int);
static void cpl_mask_filter_test_schalkoff(void);
static void cpl_mask_filter_test_matrix(int);
static void cpl_mask_filter_bench(int, int, int, int, int, int);
static void cpl_mask_count_bench(int, int);
static void cpl_mask_count_window_test(int mx, int my);

static cpl_error_code cpl_mask_closing_matrix(cpl_mask *, const cpl_matrix *);
static cpl_error_code cpl_mask_opening_matrix(cpl_mask *, const cpl_matrix *);
static cpl_error_code cpl_mask_erosion_matrix(cpl_mask *, const cpl_matrix *);
static cpl_error_code cpl_mask_dilation_matrix(cpl_mask *, const cpl_matrix *);

static cpl_error_code cpl_mask_fill_test(cpl_mask *, double);
static cpl_error_code cpl_mask_fill_border(cpl_mask *, int, int, cpl_binary);

static cpl_error_code cpl_mask_filter_bf(cpl_mask *, const cpl_mask *,
                                         const cpl_mask *, cpl_filter_mode,
                                         cpl_border_mode);
static
void cpl_mask_erosion_bf(cpl_binary *, const cpl_binary *, const cpl_binary *,
                       int, int, int, int, cpl_border_mode);
static
void cpl_mask_dilation_bf(cpl_binary *, const cpl_binary *, const cpl_binary *,
                        int, int, int, int, cpl_border_mode);
static
void cpl_mask_opening_bf(cpl_binary *, const cpl_binary *, const cpl_binary *,
                       int, int, int, int, cpl_border_mode);
static
void cpl_mask_closing_bf(cpl_binary *, const cpl_binary *, const cpl_binary *,
                       int, int, int, int, cpl_border_mode);


static double tshift   = 0.0;
static double tshiftbf = 0.0;
static double tshiftfl = 0.0;
static double tshiftb2 = 0.0;

/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/
int main(void)
{
    cpl_mask        *   bin1,
                    *   bin2,
                    *   bin1_dup;
    cpl_mask        *   null;
    cpl_image       *   labels1,
                    *   labels2;
    int                 count;
    cpl_size            nb_objs1,
                        nb_objs2;
    const cpl_size      new_pos[] = {1,2,3,4};
    const int           nsize = IMAGESZ;
    cpl_binary      *   bins;
    cpl_error_code      error;
    cpl_boolean         do_bench;
    FILE            *   stream;

    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    do_bench = cpl_msg_get_level() <= CPL_MSG_INFO ? CPL_TRUE : CPL_FALSE;

    stream = cpl_msg_get_level() > CPL_MSG_INFO
        ? fopen("/dev/null", "a") : stdout;

    /* Insert tests below */

    cpl_test_nonnull( stream );

    cpl_mask_count_window_test(7, 7);

    if (do_bench) {
        cpl_mask_count_bench(1024, 1000);
    } else {
        cpl_mask_count_bench(10, 1);
    }
    cpl_mask_filter_test_schalkoff();
    cpl_mask_filter_test(IMAGESZ);

    cpl_mask_filter_bench(do_bench ? 1024 : 32, do_bench ? 3 : 1, 0, 1, 0, 0);
    cpl_mask_filter_bench(do_bench ? 1024 : 32, do_bench ? 3 : 1, 0, 1, 1, 1);
    cpl_mask_filter_bench(do_bench ? 1024 : 32, do_bench ? 3 : 1, 0, 1, 2, 2);
    cpl_mask_filter_bench(do_bench ? 1024 : 32, do_bench ? 3 : 1, 0, 1, 3, 3);
    cpl_mask_filter_bench(do_bench ? 1024 : 32, do_bench ? 3 : 1, 2, 3, 0, 0);
    cpl_mask_filter_bench(do_bench ? 1024 : 32, do_bench ? 3 : 1, 2, 3, 1, 1);
    cpl_mask_filter_bench(do_bench ? 1024 : 32, do_bench ? 3 : 1, 2, 3, 2, 2);
    cpl_mask_filter_bench(do_bench ? 1024 : 32, do_bench ? 3 : 1, 2, 3, 3, 3);

    cpl_mask_filter_test_matrix(2);
    cpl_mask_filter_test_matrix(do_bench ? IMAGESZ/4 : 32);

    cpl_mask_test(do_bench);

    cpl_mask_shift_test(1, 1);
    cpl_mask_shift_test(1, 2);
    cpl_mask_shift_test(2, 1);
    cpl_mask_shift_test(2, 2);
    cpl_mask_shift_test(MASKSZ, 1);
    cpl_mask_shift_test(1, MASKSZ);
    cpl_mask_shift_test(MASKSZ, 2);
    cpl_mask_shift_test(2, MASKSZ);
    cpl_mask_shift_test(MASKSZ, 3);
    cpl_mask_shift_test(3, MASKSZ);
    cpl_mask_shift_test(MASKSZ, MASKSZ);

    if (do_bench) {
        cpl_mask_shift_test(IMAGESZ/4, 4 * MASKSZ);
    }

    cpl_msg_info(cpl_func, "Time to shift [s]: %g <=> %g (brute force)",
                 tshift, tshiftbf);
    cpl_msg_info(cpl_func, "Time to filter shift [s]: %g <=> %g (brute force)",
                 tshiftfl, tshiftb2);

    /* Create test image */
    cpl_msg_info("","Create a %dx%d test image", nsize, nsize);
    bin1 = cpl_mask_new(nsize, nsize);
    cpl_test_nonnull(bin1);

    error = cpl_mask_fill_test(bin1, 2.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_zero(cpl_mask_is_empty(bin1));

    cpl_mask_flip_turn_test(bin1);

    /* Test wrap / unwrap */
    bins = cpl_calloc(100, sizeof(cpl_binary));
    bin2 = cpl_mask_wrap(10, 10, bins);
    cpl_test(cpl_mask_is_empty(bin2));

    cpl_mask_delete(bin2);
    bins = cpl_calloc(100, sizeof(cpl_binary));
    bin2 = cpl_mask_wrap(10, 10, bins);
    cpl_test(cpl_mask_is_empty(bin2));
    cpl_test_eq_ptr(bins, cpl_mask_unwrap(bin2));
    cpl_free(bins);

    /* Binarize the image with a second threshold */
    bin2 = cpl_mask_new(nsize, nsize);
    cpl_test_nonnull(bin2);

    error = cpl_mask_fill_test(bin2, 5.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_zero(cpl_mask_is_empty(bin2));

    /* Test cpl_mask_get() */
    /* Test cpl_mask_set() */
    count = cpl_mask_count(bin1);
    if (cpl_mask_get(bin1, 1, 1) == CPL_BINARY_0) {
        cpl_test_zero(cpl_mask_count_window(bin1, 1, 1, 1, 1));
        cpl_mask_set(bin1, 1, 1, CPL_BINARY_1);
        cpl_test_eq(cpl_mask_count(bin1), count + 1);
        cpl_test_eq(cpl_mask_count_window(bin1, 1, 1, 1, 1), 1);
    } else {
        cpl_test_eq(cpl_mask_count_window(bin1, 1, 1, 1, 1), 1);
        cpl_mask_set(bin1, 1, 1, CPL_BINARY_0);
        cpl_test_eq(cpl_mask_count(bin1), count - 1);
        cpl_test_zero(cpl_mask_count_window(bin1, 1, 1, 1, 1));
        /* Later a non-zero mask is required */
        cpl_mask_set(bin1, 1, 1, CPL_BINARY_1);
    }

    /* Test error handling of cpl_mask_set() */
    error = cpl_mask_set(NULL, 1, 1, CPL_BINARY_1);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

    error = cpl_mask_set(bin1, 0, 1, CPL_BINARY_1);
    cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

    error = cpl_mask_set(bin1, 1, 0, CPL_BINARY_1);
    cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

    error = cpl_mask_set(bin1, 1, 1+nsize, CPL_BINARY_1);
    cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

    error = cpl_mask_set(bin1, 1+nsize, 1, CPL_BINARY_1);
    cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

    error = cpl_mask_set(bin1, 1, 1, (cpl_binary)-1);
    cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

    /* Compute number of selected pixels */
    cpl_test_eq(cpl_mask_count(bin1),
                cpl_mask_count_window(bin1, 1, 1, nsize, nsize));
    
    /* Test cpl_mask_duplicate() */
    bin1_dup = cpl_mask_duplicate(bin1);
    cpl_test_eq_mask(bin1, bin1_dup);
    cpl_test_eq(cpl_mask_is_empty(bin1), cpl_mask_is_empty(bin1_dup));
    cpl_test_eq(cpl_mask_count(bin1),    cpl_mask_count(bin1_dup));

    /* Test error handling of cpl_mask_collapse_create() */
    null = cpl_mask_collapse_create(NULL, 0);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(null);

    null = cpl_mask_collapse_create(bin1, 2);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(null);

    null = cpl_mask_collapse_create(bin1, -1);
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(null);


    /* Test cpl_mask_turn() */
    /* Rotate the mask all the way around */
    cpl_test(cpl_mask_turn(bin1,   9) == CPL_ERROR_NONE);
    cpl_test(cpl_mask_turn(bin1,   6) == CPL_ERROR_NONE);
    cpl_test(cpl_mask_turn(bin1, -13) == CPL_ERROR_NONE);
    cpl_test(cpl_mask_turn(bin1,  10) == CPL_ERROR_NONE);
    cpl_test(cpl_mask_turn(bin1,   4) == CPL_ERROR_NONE);
    cpl_test_eq_mask(bin1, bin1_dup);
    /* Compare to its copy */
    cpl_test_zero(cpl_mask_xor(bin1_dup, bin1));
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_zero(cpl_mask_count(bin1_dup));
    cpl_test_eq(cpl_mask_is_empty(bin1_dup), CPL_TRUE);
    cpl_mask_delete(bin1_dup);

    error = cpl_mask_save(bin1, FILENAME, NULL, CPL_IO_CREATE);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_mask_save(bin1, FILENAME, NULL, CPL_IO_EXTEND);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_fits(FILENAME);
    cpl_test_eq(cpl_fits_count_extensions(FILENAME), 1);

    bin1_dup = cpl_mask_load(FILENAME, 0, 1);
    cpl_test_error(CPL_ERROR_NONE);

    cpl_test_eq_mask(bin1, bin1_dup);
    cpl_mask_delete(bin1_dup);

    bin1_dup = cpl_mask_load_window(FILENAME, 0, 1, 1, 1, nsize, nsize );
    cpl_test_error(CPL_ERROR_NONE);

    cpl_test_eq_mask(bin1, bin1_dup);
    cpl_mask_delete(bin1_dup);

    cpl_test_zero( remove(FILENAME) );

    /* Labelize the binary maps */
    labels1 = cpl_image_labelise_mask_create(bin1, &nb_objs1);
    cpl_test_nonnull(labels1);

    cpl_msg_info("","Labelize bin1 ---> %" CPL_SIZE_FORMAT " objects found",
                 nb_objs1);
    labels2 = cpl_image_labelise_mask_create(bin2, &nb_objs2);
    cpl_test_nonnull(labels2);

    cpl_msg_info("","Labelize bin2 ---> %" CPL_SIZE_FORMAT " objects found",
                 nb_objs2);
   
    /* sprintf(outname, "labelized1.fits"); */
    /* cpl_image_save(labels1, outname, CPL_TYPE_FLOAT, NULL,
                      CPL_IO_CREATE); */
    /* sprintf(outname, "labelized2.fits"); */
    /* cpl_image_save(labels2, outname, CPL_TYPE_FLOAT, NULL,
                      CPL_IO_CREATE); */
    
    cpl_image_delete(labels1);
    cpl_image_delete(labels2);

    /* The following tests need to be done on a non-empty mask */ 
    cpl_test_zero(cpl_mask_is_empty(bin2));

    cpl_test_zero(cpl_mask_set(bin1, IMAGESZ, IMAGESZ, CPL_BINARY_1));
    cpl_test_zero(cpl_mask_set(bin2, IMAGESZ, IMAGESZ, CPL_BINARY_0));

    /* Test cpl_mask_and */
    cpl_test_zero(cpl_mask_and(bin1, bin2));
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_leq(cpl_mask_count(bin1), cpl_mask_count(bin2));
    
    /* Test cpl_mask_or */
    cpl_test_zero(cpl_mask_or(bin1, bin2));
    cpl_test_error(CPL_ERROR_NONE);

    cpl_test_eq_mask(bin1, bin2);
     
    /* Test cpl_mask_not */
    cpl_test_zero(cpl_mask_not(bin2));
    cpl_test_error(CPL_ERROR_NONE);

    /* Test cpl_mask_xor */
    cpl_test_zero(cpl_mask_xor(bin1, bin2));
    cpl_test_error(CPL_ERROR_NONE);

    /* bin1 is now all ones... */

    /* Test cpl_mask_copy() */
    cpl_test_zero(cpl_mask_copy(bin2, bin1, 1, 1));
    cpl_test_error(CPL_ERROR_NONE);

    cpl_test_eq_mask(bin1, bin2);

    cpl_test_zero(cpl_mask_not(bin2));
    cpl_test_error(CPL_ERROR_NONE);

    cpl_test(cpl_mask_is_empty(bin2));

    /* Test error handling in cpl_mask_copy() */
    error = cpl_mask_copy(NULL, bin2, 1, 1);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

    error = cpl_mask_copy(bin1, NULL, 1, 1);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

    error = cpl_mask_copy(bin2, bin1, 0, 1);
    cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

    error = cpl_mask_copy(bin2, bin1, 1, 0);
    cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

    error = cpl_mask_copy(bin2, bin1, nsize+1, 1);
    cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

    error = cpl_mask_copy(bin2, bin1, 1, nsize+1);
    cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

    cpl_mask_delete(bin2);
    
    /* Test cpl_mask_extract() */
    bin2 = cpl_mask_extract(bin1, 1, 1, 1, 1);
    cpl_test_nonnull(bin2);
    cpl_test_eq(cpl_mask_get_size_x(bin2), 1);
    cpl_test_eq(cpl_mask_get_size_y(bin2), 1);
    cpl_mask_delete(bin2);
    
    /* Test cpl_mask_shift() */
    cpl_test_zero(cpl_mask_shift(bin1, 1, 1));
    cpl_test_error(CPL_ERROR_NONE);
    
    cpl_test_zero(cpl_mask_set(bin1, IMAGESZ, IMAGESZ, CPL_BINARY_1));

    cpl_test_zero(cpl_mask_is_empty(bin1));

    if (IMAGESZ % 2 == 0) {
        /* Test cpl_mask_move() */
        cpl_test_zero(cpl_mask_move(bin1, 2, new_pos));
        cpl_test_error(CPL_ERROR_NONE);
    }

    /* Test cpl_mask_extract_subsample() */
    bin2 = cpl_mask_extract_subsample(bin1, 2, 2);
    cpl_test_nonnull(bin2);

    cpl_test_zero(cpl_mask_dump_window(bin1, 1, 1, MASKSZ/2, MASKSZ/2, stream));
    cpl_test_zero(cpl_mask_dump_window(bin2, 1, 1, 1, 1, stream));

    error = cpl_mask_dump_window(bin2, 1, 1, IMAGESZ, MASKSZ, stream);
    cpl_test_eq_error(error, CPL_ERROR_ACCESS_OUT_OF_RANGE);
    error = cpl_mask_dump_window(bin2, 1, 1, MASKSZ, IMAGESZ, stream);
    cpl_test_eq_error(error, CPL_ERROR_ACCESS_OUT_OF_RANGE);
    error = cpl_mask_dump_window(bin2, 0, 1, MASKSZ, MASKSZ, stream);
    cpl_test_eq_error(error, CPL_ERROR_ACCESS_OUT_OF_RANGE);
    error = cpl_mask_dump_window(bin2, 1, 0, MASKSZ, MASKSZ, stream);
    cpl_test_eq_error(error, CPL_ERROR_ACCESS_OUT_OF_RANGE);

    error = cpl_mask_dump_window(NULL, 1, 1, MASKSZ, MASKSZ, stream);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

    error = cpl_mask_dump_window(bin1, 1, 1, MASKSZ, MASKSZ, NULL);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

    cpl_mask_delete(bin1);
    cpl_mask_delete(bin2);

    if (stream != stdout) cpl_test_zero( fclose(stream) );

    /* End of tests */
    return cpl_test_end(0);
}



/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Shift a mask using brute force
  @param    in      mask to shift
  @param    x_shift shift to apply in x
  @param    y_shift shift to apply in y
  @param    edge_fill The value to fill at the border
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
  @see      cpl_image_shift()

 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_mask_shift_bf(cpl_mask * in, 
                                        int        x_shift,
                                        int        y_shift,
                                        cpl_binary edge_fill)
{
    int                 xs_abs, ys_abs;
    cpl_mask        *   loc;
    cpl_binary      *   ploc;
    cpl_binary      *   pin;
    int                 i, j;
    const int nx = cpl_mask_get_size_x(in);
    const int ny = cpl_mask_get_size_y(in); 
    /* Initialise */
    xs_abs = abs(x_shift);
    ys_abs = abs(y_shift);
    
   /* Check entries */
    cpl_ensure_code(in != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code((xs_abs<nx)&&(ys_abs<ny), CPL_ERROR_ILLEGAL_INPUT);

    /* Handle the case where nothing has to be done */
    if (x_shift==0 && y_shift==0) return CPL_ERROR_NONE;

    /* Create the local mask */
    loc = cpl_mask_duplicate(in);
    ploc = cpl_mask_get_data(loc);
    pin = cpl_mask_get_data(in);

    for (j=0; j<ny; j++) {
        for (i=0; i<nx; i++) {
            if ((i-x_shift>=0) && (i-x_shift<nx) &&
                    (j-y_shift>=0) && (j-y_shift<ny)) {
                pin[i+j*nx] = ploc[(i-x_shift)+(j-y_shift)*nx];
            } else {
                pin[i+j*nx] = edge_fill;
            }
        }
    }
    cpl_mask_delete(loc);
    return CPL_ERROR_NONE;
}




/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Shift a mask using erosion or dilation
  @param    self    Mask to shift
  @param    x_shift Shift to apply in x
  @param    y_shift Shift to apply in y
  @param    filter  CPL_FILTER_EROSION or CPL_FILTER_DILATION
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
  @see      cpl_image_shift()

  Dilation (and erosion) can be used to shift, but it does not handle
  the borders very well. Also it is slow. But it is useful for testing.

  To shift a mask 1 pixel up and 1 pixel right with CPL_FILTER_EROSION or
  CPL_FILTER_DILATION and a 3 by 3 kernel, one should set to CPL_BINARY_1
  the bottom leftmost kernel element - at row 3, column 1, i.e.
  @code
  cpl_mask * kernel = cpl_mask_new(3, 3);
  cpl_mask_set(kernel, 1, 1);
  @endcode

  (The border of the output will depend on the border mode, but none of
   the currently available borders support shifting pixels into the border).

 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_mask_shift_filter(cpl_mask * self,
                                            int        x_shift,
                                            int        y_shift,
                                            cpl_filter_mode filter)
{

    const int mx = 1 + 2 * abs(x_shift);
    const int my = 1 + 2 * abs(y_shift);
    cpl_mask * copy = cpl_mask_duplicate(self);
    cpl_mask * kernel = cpl_mask_new(mx, my);
    const cpl_error_code error
        = cpl_mask_set(kernel, (mx + 1)/2 - x_shift, (my + 1)/2 - y_shift,
                       CPL_BINARY_1)
        || cpl_mask_filter(self, copy, kernel, filter,
                           CPL_BORDER_ZERO);

    cpl_mask_delete(kernel);
    cpl_mask_delete(copy);

    return error ? cpl_error_set_where(cpl_func) : CPL_ERROR_NONE;

}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Test cpl_mask_shift()
  @param  m      The kernel X-size
  @param  n      The kernel Y-size
  @return void

 */
/*----------------------------------------------------------------------------*/
static void cpl_mask_shift_test(int m, int n)
{
    cpl_mask  * mask0   = cpl_mask_new(m, n);
    cpl_error_code error;


    cpl_msg_info(cpl_func, "Shift(%d,%d)", m, n);

    error = cpl_mask_shift(NULL, 0, 0);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);


    error = cpl_mask_shift(mask0, m, 0);
    cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

    error = cpl_mask_shift(mask0, -m, 0);
    cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);


    error = cpl_mask_shift(mask0, 0, n);
    cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

    error = cpl_mask_shift(mask0, 0, -n);
    cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

    if (m == 1 && n == 1) {
        cpl_mask * mask1 = cpl_mask_new(m, n);
        cpl_test_zero(cpl_mask_shift(mask1, 0, 0));
        cpl_mask_delete(mask1);
    } else {
        cpl_mask * mask1 = cpl_mask_new(m, n);
        cpl_mask * mask2 = cpl_mask_new(m, n);
        int count;

        do {
            /* Need a mask with a mix of elements */
            cpl_mask_fill_test(mask0, 0.0);
            count = cpl_mask_count(mask0);
        } while (count == 0 || count == m * n);

        for (int j = 1-n; j < n; j++) {
            for (int i = 1-m; i < m; i++) {
                double time0, time1;

                cpl_mask_copy(mask1, mask0, 1, 1);
                cpl_mask_copy(mask2, mask0, 1, 1);

                time0 = cpl_test_get_cputime();
                cpl_test_zero(cpl_mask_shift(mask1, i, j));
                time1 = cpl_test_get_cputime();
                if (time1 > time0) tshift += time1 - time0;

                time0 = cpl_test_get_cputime();
                cpl_test_zero(cpl_mask_shift_bf(mask2, i, j, CPL_BINARY_1));
                time1 = cpl_test_get_cputime();
                if (time1 > time0) tshiftbf += time1 - time0;

                cpl_test_eq_mask(mask1, mask2);

                if (4 * abs(i) < m && 4 * abs(j) < n) {

                    const cpl_filter_mode filter[] = {CPL_FILTER_EROSION,
                                                      CPL_FILTER_DILATION};

                    const size_t nfilter = sizeof(filter)/sizeof(*filter);
                    size_t k;

                    for (k = 0; k < nfilter; k++) {

                        cpl_mask_copy(mask1, mask0, 1, 1);

                        cpl_mask_fill_border(mask1, 2 * abs(i), 2 * abs(j),
                                             CPL_BINARY_0);

                        cpl_mask_copy(mask2, mask1, 1, 1);

                        time0 = cpl_test_get_cputime();
                        error = cpl_mask_shift_filter(mask1, i, j,
                                                      filter[k]);
                        cpl_test_eq_error(error, CPL_ERROR_NONE);
                        time1 = cpl_test_get_cputime();
                        if (time1 > time0) tshiftfl += time1 - time0;

                        time0 = cpl_test_get_cputime();
                        cpl_mask_shift_bf(mask2, i, j, CPL_BINARY_0);
                        time1 = cpl_test_get_cputime();
                        if (time1 > time0) tshiftb2 += time1 - time0;

                        cpl_test_eq_mask(mask1, mask2);
                    }

                }
            }
        }
        cpl_mask_delete(mask1);
        cpl_mask_delete(mask2);
    }

    cpl_mask_delete(mask0);
}



/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Fill the mask with a test mask
  @param self       Mask to be filled
  @param threshold  Binarization threshold
  @return CPL_ERROR_NONE or the relevant #_cpl_error_code_
  
 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_mask_fill_test(cpl_mask * self, double threshold)
{

    const int nx = cpl_mask_get_size_x(self);
    const int ny = cpl_mask_get_size_y(self); 
    cpl_image  * img = cpl_image_fill_test_create(nx, ny);
    double       med_dist;

    /* Compute the median and the mean dist. to the median of the image */
    const double median = cpl_image_get_median_dev(img, &med_dist);
    const double minval = median + threshold * med_dist;
    cpl_mask * other = cpl_mask_threshold_image_create(img, minval, DBL_MAX);

    cpl_ensure_code(img != NULL, cpl_error_get_code());

    cpl_image_delete(img);

    cpl_ensure_code(!cpl_mask_copy(self, other, 1, 1), cpl_error_get_code());

    cpl_mask_delete(other);

    return CPL_ERROR_NONE;
}



/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Fill the mask border
  @param self  Mask to be filled at the border
  @param hx    Number of borders columns to fill
  @param hy    Number of borders rows to fill
  @param fill  Value to use in fill
  @param threshold  Binarization threshold
  @return CPL_ERROR_NONE or the relevant #_cpl_error_code_
  
 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_mask_fill_border(cpl_mask * self, int hx, int hy,
                                           cpl_binary fill)
{

    const int nx = cpl_mask_get_size_x(self);
    const int ny = cpl_mask_get_size_y(self);
    cpl_binary * pself = cpl_mask_get_data(self);
    int j;


    cpl_ensure_code(pself != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(hx >= 0, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(hy >= 0, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(2 * hx <= nx, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(2 * hy <= ny, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(fill == CPL_BINARY_0 || fill == CPL_BINARY_1,
                    CPL_ERROR_ILLEGAL_INPUT);

    /* Modified from cpl_mask_erosion_() */

    /* Handle border for first hy rows and first hx elements of next row */
    (void)memset(pself, fill, hx + hy * nx);

    /* self-col now indexed from -hy to hy */
    pself += hy * nx;

    for (j = hy; j < ny-hy; j++, pself += nx) {

        if (j > hy) {
            /* Do also last hx border elements of previous row */
            (void)memset(pself - hx, fill, 2 * hx);
        }
    }

    /* Do also last hx border elements of previous row */
    (void)memset(pself - hx, fill, hx + hy * nx);

    return CPL_ERROR_NONE;

}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @param do_bench True iff benchmarking is enabled
  @brief          Test various cpl_mask functions
  @return         void
  
 */
/*----------------------------------------------------------------------------*/
static void cpl_mask_test(cpl_boolean do_bench)
{

    const int npix[] = {1, 2, 3, 5, 10, 11, do_bench ? 2*IMAGESZ : 13};

    const cpl_binary buf9[] = {CPL_BINARY_1, CPL_BINARY_0, CPL_BINARY_1,
                               CPL_BINARY_0, CPL_BINARY_0, CPL_BINARY_1,
                               CPL_BINARY_1, CPL_BINARY_0, CPL_BINARY_1};

    /* Rotation 90 degrees clockwise */
    const cpl_binary rot9[] = {CPL_BINARY_1, CPL_BINARY_1, CPL_BINARY_1,
                               CPL_BINARY_0, CPL_BINARY_0, CPL_BINARY_0,
                               CPL_BINARY_1, CPL_BINARY_0, CPL_BINARY_1};

    /* This mask is not modified while wrapping around this buffer */
    cpl_mask     * ref = cpl_mask_wrap(3, 3, (cpl_binary*)rot9);
    cpl_mask     * raw = cpl_mask_new(3, 3);
    cpl_error_code error;
    size_t         ipix;
    double         tflip = 0.0;
    double         tturn = 0.0;
    double         tassi = 0.0;
    double         tcol0 = 0.0;
    double         tcol1 = 0.0;

    /* Test 1: Verify direction of 90 degree rotation */
    cpl_test_nonnull(memcpy(cpl_mask_get_data(raw), buf9, 9*sizeof(cpl_binary)));

    cpl_test_zero(cpl_mask_turn(raw, 1));
    cpl_test_eq_mask(raw, ref);

    cpl_mask_delete(raw);
    (void)cpl_mask_unwrap(ref);

    /* Test 2: Check error handling */
    error = cpl_image_turn(NULL, 1);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

    /* Test 3: Verify consistency of rotations across types */
    for (ipix = 0; ipix < sizeof(npix)/sizeof(npix[0]); ipix++) {
        const int nx = npix[ipix];
        size_t jpix;
        for (jpix = 0; jpix < sizeof(npix)/sizeof(npix[0]); jpix++) {
            const int ny = npix[jpix];

            cpl_image * noise = cpl_image_new(nx, ny, CPL_TYPE_FLOAT);
            cpl_mask * collapsed;

            double ttemp0, ttemp1;
            int i;

            cpl_test_zero(cpl_image_fill_noise_uniform(noise, (double)-ny,
                                                       (double)nx));

            /* About 1/4 of the pixels are flagged */
            raw = cpl_mask_threshold_image_create(noise, -0.25*ny, 0.25*nx);
            cpl_image_delete(noise);

            ref = cpl_mask_extract(raw, 1, 1, nx, ny);

            ttemp0 = cpl_test_get_cputime();

            /* Turn the mask all the way around */
            cpl_test_zero(cpl_mask_turn(raw, 1));
            cpl_test_zero(cpl_mask_turn(raw, 2));
            cpl_test_zero(cpl_mask_turn(raw, 3));
            cpl_test_zero(cpl_mask_turn(raw, 4));
            cpl_test_zero(cpl_mask_turn(raw,-1));
            cpl_test_zero(cpl_mask_turn(raw,-2));
            cpl_test_zero(cpl_mask_turn(raw,-3));
            cpl_test_zero(cpl_mask_turn(raw, 0));

            ttemp1 = cpl_test_get_cputime();
            if (ttemp1 > ttemp0) tturn += ttemp1 - ttemp0;

            cpl_test_eq_mask(raw, ref);

            ttemp0 = cpl_test_get_cputime();

            /* Flip the mask all the way around */
            cpl_test_zero(cpl_mask_flip(raw, 0));
            cpl_test_zero(cpl_mask_flip(raw, 1));
            cpl_test_zero(cpl_mask_flip(raw, 2));
            cpl_test_zero(cpl_mask_flip(raw, 3));
            cpl_test_zero(cpl_mask_flip(raw, 0));
            cpl_test_zero(cpl_mask_flip(raw, 1));
            cpl_test_zero(cpl_mask_flip(raw, 2));
            cpl_test_zero(cpl_mask_flip(raw, 3));

            ttemp1 = cpl_test_get_cputime();
            if (ttemp1 > ttemp0) tflip += ttemp1 - ttemp0;

            cpl_test_eq_mask(raw, ref);

            ttemp0 = cpl_test_get_cputime();
            cpl_test_zero(cpl_mask_or(raw, ref));
            cpl_test_eq_mask(raw, ref);

            cpl_test_zero(cpl_mask_and(raw, ref));
            cpl_test_eq_mask(raw, ref);

            /* Empty the mask */
            cpl_test_zero(cpl_mask_xor(raw, ref));
            cpl_test(cpl_mask_is_empty(raw));

            /* cpl_mask_copy()... */
            cpl_test_nonnull(memcpy(cpl_mask_get_data(raw),
                                    cpl_mask_get_data_const(ref), 
                                   nx * ny * sizeof(cpl_binary)));

            /* Empty the mask */
            cpl_test_zero(cpl_mask_not(raw));
            cpl_test_zero(cpl_mask_and(raw, ref));
            cpl_test(cpl_mask_is_empty(raw));


            /* cpl_mask_copy()... */
            cpl_test_nonnull(memcpy(cpl_mask_get_data(raw),
                                    cpl_mask_get_data_const(ref),
                                    nx * ny * sizeof(cpl_binary)));

            /* Fill the mask */
            cpl_test_zero(cpl_mask_not(raw));
            cpl_test_zero(cpl_mask_or(raw, ref));
            cpl_test_eq(cpl_mask_count(raw), nx * ny);

            ttemp1 = cpl_test_get_cputime();
            if (ttemp1 > ttemp0) tassi += ttemp1 - ttemp0;

            for (i = 1; i <= nx; i++) {
                /* First collapsed element will be CPL_BINARY_0 */
                cpl_mask_set(raw, i, 1, CPL_BINARY_0);
            }
            if (ny > 2) {
                /* Second collapsed element will be CPL_BINARY_0 */
                cpl_mask_set(raw, 1, 2, CPL_BINARY_0);
            }

            ttemp0 = cpl_test_get_cputime();

            collapsed = cpl_mask_collapse_create(raw, 1);
            ttemp1 = cpl_test_get_cputime();
            if (ttemp1 > ttemp0) tcol1 += ttemp1 - ttemp0;

            cpl_test_error(CPL_ERROR_NONE);
            cpl_test_nonnull(collapsed);

            cpl_test_eq(cpl_mask_get_size_x(collapsed), 1);
            cpl_test_eq(cpl_mask_get_size_y(collapsed), ny);

            cpl_test_zero(cpl_mask_get(collapsed, 1, 1));
            if (ny > 2) {
                cpl_test_zero(cpl_mask_get(collapsed, 1, 2));
            }
            cpl_test_eq(cpl_mask_count(collapsed), ny > 2 ? ny - 2 : ny - 1);

            cpl_mask_delete(collapsed);

            /* cpl_mask_copy()... */
            cpl_test_nonnull(memcpy(cpl_mask_get_data(raw),
                                    cpl_mask_get_data_const(ref),
                                    nx * ny * sizeof(cpl_binary)));


            /* Fill the mask */
            ttemp0 = cpl_test_get_cputime();
            cpl_test_zero(cpl_mask_not(raw));
            cpl_test_zero(cpl_mask_or(raw, ref));
            ttemp1 = cpl_test_get_cputime();
            if (ttemp1 > ttemp0) tassi += ttemp1 - ttemp0;
            cpl_test_eq(cpl_mask_count(raw), nx * ny);

            for (i = 1; i <= ny; i++) {
                /* First collapsed element will be CPL_BINARY_0 */
                cpl_mask_set(raw, 1, i, CPL_BINARY_0);
            }
            if (nx > 2) {
                /* Second collapsed element will be CPL_BINARY_0 */
                cpl_mask_set(raw, 2, 1, CPL_BINARY_0);
            }

            ttemp0 = cpl_test_get_cputime();

            collapsed = cpl_mask_collapse_create(raw, 0);
            ttemp1 = cpl_test_get_cputime();
            if (ttemp1 > ttemp0) tcol0 += ttemp1 - ttemp0;

            cpl_test_error(CPL_ERROR_NONE);
            cpl_test_nonnull(collapsed);

            cpl_test_eq(cpl_mask_get_size_x(collapsed), nx);
            cpl_test_eq(cpl_mask_get_size_y(collapsed), 1);

            cpl_test_zero(cpl_mask_get(collapsed, 1, 1));
            if (nx > 2) {
                cpl_test_zero(cpl_mask_get(collapsed, 2, 1));
            }
            cpl_test_eq(cpl_mask_count(collapsed), nx > 2 ? nx - 2 : nx - 1);

            cpl_mask_delete(collapsed);

            if (nx > 1 && ny > 1) {
                int nzero = 0;
                const int nmin = nx < ny ? nx : ny;

                /* Fill the mask */
                ttemp0 = cpl_test_get_cputime();
                cpl_test_zero(cpl_mask_xor(raw, raw));
                cpl_test(cpl_mask_is_empty(raw));
                cpl_test_zero(cpl_mask_not(raw));
                ttemp1 = cpl_test_get_cputime();
                if (ttemp1 > ttemp0) tassi += ttemp1 - ttemp0;
                cpl_test_eq(cpl_mask_count(raw), nx * ny);

                /* Every other element on the diagonal is CPL_BINARY_0 */
                for (i = 1; i <= nmin; i += 2) {
                    cpl_test_zero(cpl_mask_set(raw, i, i, CPL_BINARY_0));
                    nzero++;
                }
                cpl_test_eq(cpl_mask_count(raw), nx * ny-nzero);

                ttemp0 = cpl_test_get_cputime();

                collapsed = cpl_mask_collapse_create(raw, 1);
                ttemp1 = cpl_test_get_cputime();
                if (ttemp1 > ttemp0) tcol1 += ttemp1 - ttemp0;

                cpl_test_error(CPL_ERROR_NONE);
                cpl_test_nonnull(collapsed);

                cpl_test_eq(cpl_mask_get_size_x(collapsed), 1);
                cpl_test_eq(cpl_mask_get_size_y(collapsed), ny);

                for (i = 1; i <= nmin; i += 2) {
                    cpl_test_zero(cpl_mask_get(collapsed, 1, i));
                }

                cpl_test_eq(cpl_mask_count(collapsed), ny - nzero);

                cpl_mask_delete(collapsed);


                ttemp0 = cpl_test_get_cputime();

                collapsed = cpl_mask_collapse_create(raw, 0);
                ttemp1 = cpl_test_get_cputime();
                if (ttemp1 > ttemp0) tcol0 += ttemp1 - ttemp0;

                cpl_test_error(CPL_ERROR_NONE);
                cpl_test_nonnull(collapsed);

                cpl_test_eq(cpl_mask_get_size_x(collapsed), nx);
                cpl_test_eq(cpl_mask_get_size_y(collapsed), 1);

                for (i = 1; i <= nmin; i += 2) {
                    cpl_test_zero(cpl_mask_get(collapsed, i, 1));
                }

                cpl_test_eq(cpl_mask_count(collapsed), nx - nzero);

                cpl_mask_delete(collapsed);


                /* Fill the mask */
                ttemp0 = cpl_test_get_cputime();
                cpl_test_zero(cpl_mask_xor(raw, raw));
                cpl_test(cpl_mask_is_empty(raw));
                cpl_test_zero(cpl_mask_not(raw));
                ttemp1 = cpl_test_get_cputime();
                if (ttemp1 > ttemp0) tassi += ttemp1 - ttemp0;
                cpl_test_eq(cpl_mask_count(raw), nx * ny);

                /* The first row is all CPL_BINARY_0 */
                for (i = 1; i <= nx; i++) {
                    cpl_test_zero(cpl_mask_set(raw, i, 1, CPL_BINARY_0));
                }
                cpl_test_eq(cpl_mask_count(raw), nx * (ny-1));

                ttemp0 = cpl_test_get_cputime();

                collapsed = cpl_mask_collapse_create(raw, 0);
                ttemp1 = cpl_test_get_cputime();
                if (ttemp1 > ttemp0) tcol0 += ttemp1 - ttemp0;

                cpl_test_error(CPL_ERROR_NONE);
                cpl_test_nonnull(collapsed);

                cpl_test_eq(cpl_mask_get_size_x(collapsed), nx);
                cpl_test_eq(cpl_mask_get_size_y(collapsed), 1);

                cpl_test(cpl_mask_is_empty(collapsed));

                cpl_mask_delete(collapsed);

            }

            cpl_mask_delete(ref);
            cpl_mask_delete(raw);
        }
    }

    cpl_msg_info(cpl_func, "Time to flip [s]: %g", tflip);
    cpl_msg_info(cpl_func, "Time to turn [s]: %g", tturn);
    cpl_msg_info(cpl_func, "Time to assign [s]: %g", tassi);
    cpl_msg_info(cpl_func, "Time to collapse 0 [s]: %g", tcol0);
    cpl_msg_info(cpl_func, "Time to collapse 1 [s]: %g", tcol1);

    return;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief       Test the CPL function
  @param nsize The object size to use for testing
  @return      void
  
 */
/*----------------------------------------------------------------------------*/
static void cpl_mask_filter_test(int nsize)
{

    cpl_error_code error;
    const cpl_filter_mode filter[] = {CPL_FILTER_EROSION, CPL_FILTER_DILATION,
                                      CPL_FILTER_OPENING, CPL_FILTER_CLOSING};

    const size_t nfilter = sizeof(filter)/sizeof(*filter);
    size_t k;

    /* Build a 3x3 kernel */
    cpl_mask  * kernel = cpl_mask_new(3, 3);

    cpl_mask * mask1 = cpl_mask_new(nsize, nsize);
    cpl_mask * copy1 = cpl_mask_new(nsize, nsize);

    cpl_msg_info(cpl_func,"Testing filtering functions with %d x %d mask",
                 nsize, nsize);

    error = cpl_mask_fill_test(mask1, 2.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_zero(cpl_mask_is_empty(mask1));

    cpl_mask_set(kernel, 1, 2, CPL_BINARY_1);
    cpl_mask_set(kernel, 2, 2, CPL_BINARY_1);
    cpl_mask_set(kernel, 3, 2, CPL_BINARY_1);
    cpl_mask_set(kernel, 2, 1, CPL_BINARY_1);
    cpl_mask_set(kernel, 2, 3, CPL_BINARY_1);
    cpl_test_eq(cpl_mask_count(kernel), 5);

    for (k = 0; k < nfilter; k++) {
        cpl_mask * evenr = cpl_mask_new(3, 4);
        cpl_mask * evenc = cpl_mask_new(4, 3);

        cpl_mask * id = cpl_mask_new(1,1);
        cpl_mask * copy2 = cpl_mask_new(nsize, nsize);

        /* Error: NULL pointer */
        error = cpl_mask_filter(NULL, mask1, kernel, filter[k],
                                CPL_BORDER_ZERO);
        cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

        error = cpl_mask_filter(copy1, NULL, kernel, filter[k],
                                CPL_BORDER_ZERO);
        cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

        error = cpl_mask_filter(copy1, mask1, NULL, filter[k],
                                CPL_BORDER_ZERO);
        cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

        /* Error: Even sized kernels */
        error = cpl_mask_filter(copy1, mask1, evenr, filter[k],
                                CPL_BORDER_ZERO);
        cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

        error = cpl_mask_filter(copy1, mask1, evenc, filter[k],
                                CPL_BORDER_ZERO);
        cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

        /* Error: Empty kernel */
        error = cpl_mask_filter(copy1, mask1, id, filter[k], CPL_BORDER_ZERO);
        cpl_test_eq_error(error, CPL_ERROR_DATA_NOT_FOUND);

        /* Error: Unsupported border modes */
        error = cpl_mask_filter(copy1, mask1, id, filter[k], CPL_BORDER_CROP);
        cpl_test_eq_error(error, CPL_ERROR_UNSUPPORTED_MODE);

        error = cpl_mask_filter(copy1, mask1, id, filter[k], CPL_BORDER_FILTER);
        cpl_test_eq_error(error, CPL_ERROR_UNSUPPORTED_MODE);

        /* The identity kernel */
        error = cpl_mask_not(id);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_mask_filter(copy1, mask1, id, filter[k], CPL_BORDER_ZERO);

        cpl_test_eq_error(error, CPL_ERROR_NONE);
        cpl_test_eq_mask(copy1, mask1);

        /* Rely on the border already being equal to the reference */
        error = cpl_mask_filter(copy1, mask1, id, filter[k], CPL_BORDER_NOP);

        cpl_test_eq_error(error, CPL_ERROR_NONE);
        cpl_test_eq_mask(copy1, mask1);

        cpl_mask_delete(evenr);
        cpl_mask_delete(evenc);
        cpl_mask_delete(copy2);
        cpl_mask_delete(id);

    }

    error = cpl_mask_filter(copy1, mask1, kernel, CPL_FILTER_EROSION,
                            CPL_BORDER_NOP);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_lt(cpl_mask_count(copy1), cpl_mask_count(mask1));

    error = cpl_mask_filter(copy1, mask1, kernel, CPL_FILTER_MEDIAN,
                            CPL_BORDER_NOP);
    cpl_test_eq_error(error, CPL_ERROR_UNSUPPORTED_MODE);


    cpl_mask_delete(mask1);
    cpl_mask_delete(copy1);
    cpl_mask_delete(kernel);

}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief       Test the CPL function using example from Schalkoff
  @param nsize The object size to use for testing
  @return      void
  @see R. Schallkoff, "Digital Image Processing and Computer Vision"
  
 */
/*----------------------------------------------------------------------------*/
static void cpl_mask_filter_test_schalkoff(void)
{

#define nx 21
#define ny 18

    const int borders[] = {CPL_BORDER_NOP, CPL_BORDER_ZERO, CPL_BORDER_COPY};
    const size_t nborders = sizeof(borders)/sizeof(borders[0]);

    /* Binary Image 6.36a  */
    const cpl_binary schallkoff_A[nx*ny] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 
        0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    /* Binary Image 6.37b from R. Schallkoff (Dilation w. 3x3 full kernel) */
    const cpl_binary schallkoff_D[nx*ny] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 
        0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 
        0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 
        0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 
        0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 
        0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 
        0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 
        0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 
        0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    /* Binary Image 6.37c from R. Schallkoff (Erosion w. 3x3 full kernel) */
    const cpl_binary schallkoff_E[nx*ny] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    /* Binary Image 6.39a from R. Schallkoff (Opening w. 3x3 full kernel) */
    const cpl_binary schallkoff_O[nx*ny] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    /* Binary Image 6.39b from R. Schallkoff (Closing w. 3x3 full kernel) */
    const cpl_binary schallkoff_C[nx*ny] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 
        0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    cpl_mask * mask0 = cpl_mask_new(nx, ny);

    /* NOT modified */
    cpl_mask * maskA = cpl_mask_wrap(nx, ny, (cpl_binary*)schallkoff_A);
    cpl_mask * maskD = cpl_mask_wrap(nx, ny, (cpl_binary*)schallkoff_D);
    cpl_mask * maskE = cpl_mask_wrap(nx, ny, (cpl_binary*)schallkoff_E);
    cpl_mask * maskO = cpl_mask_wrap(nx, ny, (cpl_binary*)schallkoff_O);
    cpl_mask * maskC = cpl_mask_wrap(nx, ny, (cpl_binary*)schallkoff_C);

    /* Build a 3x3 kernel inside a 5 x 5 mask */
    cpl_mask * kernel = cpl_mask_new(5, 5);

    cpl_error_code error;
    size_t iborder;

    cpl_msg_info(cpl_func, "Schalkoff filter test on " CPL_STRINGIFY(nx) " x "
                 CPL_STRINGIFY(ny) " mask");

    cpl_mask_flip_turn_test(maskA);

    /* Fill 3x3 kernel */
    cpl_mask_set(kernel, 2, 2, CPL_BINARY_1);
    cpl_mask_set(kernel, 2, 3, CPL_BINARY_1);
    cpl_mask_set(kernel, 2, 4, CPL_BINARY_1);
    cpl_mask_set(kernel, 3, 2, CPL_BINARY_1);
    cpl_mask_set(kernel, 3, 3, CPL_BINARY_1);
    cpl_mask_set(kernel, 3, 4, CPL_BINARY_1);
    cpl_mask_set(kernel, 4, 2, CPL_BINARY_1);
    cpl_mask_set(kernel, 4, 3, CPL_BINARY_1);
    cpl_mask_set(kernel, 4, 4, CPL_BINARY_1);
    cpl_test_eq(cpl_mask_count(kernel), 3 * 3);


    for (iborder = 0; iborder < nborders; iborder++) {
        int iflip;
        for (iflip = 0; iflip < 3; iflip++) {

            /* Test also with flipped images since the mask is traversed in 
               one specific direction */

            cpl_mask * flip = cpl_mask_new(nx, ny);

            /* Test erosion */

            error = cpl_mask_copy(flip, maskA, 1, 1);
            cpl_test_eq_error(error, CPL_ERROR_NONE);

            if (iflip != 1) {
                error = cpl_mask_flip(flip, iflip);
                cpl_test_eq_error(error, CPL_ERROR_NONE);
            }

            error = cpl_mask_filter(mask0, flip, kernel, CPL_FILTER_EROSION,
                                    borders[iborder]);
            cpl_test_eq_error(error, CPL_ERROR_NONE);

            if (iflip != 1) {
                error = cpl_mask_flip(mask0, iflip);
                cpl_test_eq_error(error, CPL_ERROR_NONE);
            }

            cpl_test_eq_mask(maskE, mask0);

            /* Test dilation */

            error = cpl_mask_copy(flip, maskA, 1, 1);
            cpl_test_eq_error(error, CPL_ERROR_NONE);

            if (iflip != 1) {
                error = cpl_mask_flip(flip, iflip);
                cpl_test_eq_error(error, CPL_ERROR_NONE);
            }


            error = cpl_mask_filter(mask0, flip, kernel, CPL_FILTER_DILATION,
                                    borders[iborder]);
            cpl_test_eq_error(error, CPL_ERROR_NONE);

            if (iflip != 1) {
                error = cpl_mask_flip(mask0, iflip);
                cpl_test_eq_error(error, CPL_ERROR_NONE);
            }

            cpl_test_eq_mask(maskD, mask0);

            /* Test duality of erosion and dilation (kernel is symmetric) */

            cpl_mask_not(flip);

            error = cpl_mask_filter(mask0, flip, kernel, CPL_FILTER_DILATION,
                                    borders[iborder]);
            cpl_test_eq_error(error, CPL_ERROR_NONE);

            cpl_mask_not(mask0);
            /* No duality on border */
            cpl_mask_fill_border(mask0, 3, 3, CPL_BINARY_0);

            if (iflip != 1) {
                error = cpl_mask_flip(mask0, iflip);
                cpl_test_eq_error(error, CPL_ERROR_NONE);
            }

            cpl_test_eq_mask(maskE, mask0);

            /* Test duality of dilation and erosion (kernel is symmetric) */

            error = cpl_mask_filter(mask0, flip, kernel, CPL_FILTER_EROSION,
                                    borders[iborder]);
            cpl_test_eq_error(error, CPL_ERROR_NONE);

            cpl_mask_not(mask0);
            /* No duality on border */
            cpl_mask_fill_border(mask0, 3, 3, CPL_BINARY_0);

            if (iflip != 1) {
                error = cpl_mask_flip(mask0, iflip);
                cpl_test_eq_error(error, CPL_ERROR_NONE);
            }

            cpl_test_eq_mask(maskD, mask0);

            if (borders[iborder] != CPL_BORDER_CROP) { /* Always true... */
                int nidem;

                /* Test opening */

                error = cpl_mask_copy(flip, maskA, 1, 1);
                cpl_test_eq_error(error, CPL_ERROR_NONE);

                if (iflip != 1) {
                    error = cpl_mask_flip(flip, iflip);
                    cpl_test_eq_error(error, CPL_ERROR_NONE);
                }

                nidem = 2;
                do {
                    /* Idempotency test as well */
                    error = cpl_mask_filter(mask0, flip, kernel,
                                            CPL_FILTER_OPENING,
                                            borders[iborder]);
                    cpl_test_eq_error(error, CPL_ERROR_NONE);
                } while (--nidem);

                if (iflip != 1) {
                    error = cpl_mask_flip(mask0, iflip);
                    cpl_test_eq_error(error, CPL_ERROR_NONE);
                }

                cpl_test_eq_mask(maskO, mask0);

                /* Test closing */

                error = cpl_mask_copy(flip, maskA, 1, 1);
                cpl_test_eq_error(error, CPL_ERROR_NONE);

                if (iflip != 1) {
                    error = cpl_mask_flip(flip, iflip);
                    cpl_test_eq_error(error, CPL_ERROR_NONE);
                }

                nidem = 2;
                do {
                    /* Idempotency test as well */
                    error = cpl_mask_filter(mask0, flip, kernel,
                                            CPL_FILTER_CLOSING,
                                            borders[iborder]);
                    cpl_test_eq_error(error, CPL_ERROR_NONE);
                } while (--nidem > 0);

                if (iflip != 1) {
                    error = cpl_mask_flip(mask0, iflip);
                    cpl_test_eq_error(error, CPL_ERROR_NONE);
                }

                cpl_test_eq_mask(maskC, mask0);

                /* Test duality of opening and closing */

                cpl_mask_not(flip);

                error = cpl_mask_filter(mask0, flip, kernel, CPL_FILTER_OPENING,
                                        borders[iborder]);
                cpl_test_eq_error(error, CPL_ERROR_NONE);

                cpl_mask_not(mask0);
                /* No duality on border */
                cpl_mask_fill_border(mask0, 3, 3, CPL_BINARY_0);

                if (iflip != 1) {
                    error = cpl_mask_flip(mask0, iflip);
                    cpl_test_eq_error(error, CPL_ERROR_NONE);
                }

                cpl_test_eq_mask(maskC, mask0);

                /* Test duality of closing and opening */

                error = cpl_mask_filter(mask0, flip, kernel, CPL_FILTER_CLOSING,
                                        borders[iborder]);
                cpl_test_eq_error(error, CPL_ERROR_NONE);

                cpl_mask_not(mask0);
                /* No duality on border */
                cpl_mask_fill_border(mask0, 3, 3, CPL_BINARY_0);

                if (iflip != 1) {
                    error = cpl_mask_flip(mask0, iflip);
                    cpl_test_eq_error(error, CPL_ERROR_NONE);
                }

                cpl_test_eq_mask(maskO, mask0);

            }


            /* Tests for one flip done */
            cpl_mask_delete(flip);

        }
    }

    cpl_test_eq_ptr(cpl_mask_unwrap(maskA), schallkoff_A);
    cpl_test_eq_ptr(cpl_mask_unwrap(maskD), schallkoff_D);
    cpl_test_eq_ptr(cpl_mask_unwrap(maskE), schallkoff_E);
    cpl_test_eq_ptr(cpl_mask_unwrap(maskO), schallkoff_O);
    cpl_test_eq_ptr(cpl_mask_unwrap(maskC), schallkoff_C);

    cpl_mask_delete(mask0);
    cpl_mask_delete(kernel);

#undef nx
#undef ny

}



/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief       Test the CPL function against the matrix based ones
  @param nsize The object size to use for testing
  @return      void
  
 */
/*----------------------------------------------------------------------------*/
static void cpl_mask_filter_test_matrix(int nsize) {

    const int maxhxy = CX_MIN(9, nsize/4);

    cpl_error_code (*ffilter[4])(cpl_mask *, const cpl_matrix *)
        = {cpl_mask_erosion_matrix, cpl_mask_dilation_matrix,
           cpl_mask_opening_matrix, cpl_mask_closing_matrix};

    const cpl_filter_mode mfilter[] = {CPL_FILTER_EROSION, CPL_FILTER_DILATION,
                                       CPL_FILTER_OPENING, CPL_FILTER_CLOSING};

    const cpl_filter_mode mdual[] = {CPL_FILTER_DILATION, CPL_FILTER_EROSION,
                                     CPL_FILTER_CLOSING,  CPL_FILTER_OPENING};

    const size_t nfilter = sizeof(ffilter)/sizeof(*ffilter);
    size_t k;

    cpl_mask * raw = cpl_mask_new(nsize, nsize);
    int hx, hy;
    cpl_error_code error;
    double time_mask = 0.0;
    double time_bf   = 0.0;
    double time_matrix = 0.0;

    error = cpl_mask_fill_test(raw, 2.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_msg_info(cpl_func, "Matrix testing of cpl_mask_filter() filters with "
                 "%d x %d mask", nsize, nsize);

    for (hy = 0; hy < maxhxy; hy++) {
        for (hx = 0; hx < maxhxy; hx++) {
            const int mx = 1 + 2 * hx;
            const int my = 1 + 2 * hy;
            int do_full;

            if (mx > nsize || my > nsize) continue;

            for (do_full = mx * my > 1 ? 0 : 1; do_full < 2; do_full++) {
                cpl_mask   * kernel = cpl_mask_new(mx, my);
                cpl_matrix * matrix = cpl_matrix_new(my, mx);

                cpl_mask * filtered = cpl_mask_new(nsize, nsize);
                cpl_mask * rawcopy = cpl_mask_new(nsize, nsize);

                if (do_full) {
                    error = cpl_mask_not(kernel);
                    cpl_test_eq_error(error, CPL_ERROR_NONE);
                    error = cpl_matrix_fill(matrix, 1.0);
                    cpl_test_eq_error(error, CPL_ERROR_NONE);
                } else {
                    int i, j;
                    int ncount;
                    do {
                        error = cpl_mask_fill_test(kernel, 0.0);
                        cpl_test_eq_error(error, CPL_ERROR_NONE);
                        ncount = cpl_mask_count(kernel);
                    } while (ncount == 0 || ncount == mx * my);

                    for (j = 0; j < my; j++) {
                        for (i = 0; i < mx; i++) {
                            if (cpl_mask_get(kernel, i + 1, j + 1))
                                cpl_matrix_set(matrix, my - j - 1, i, 1.0);
                        }
                    }

                    cpl_msg_info(cpl_func, "The %u filters w. %d x %d kernel "
                                 "(full and %d element(s))", (unsigned)nfilter,
                                 mx, my, ncount);
                }

                for (k = 0; k < nfilter; k++) {
                    double     time0, time1;

                    error = cpl_mask_copy(rawcopy, raw, 1, 1);
                    cpl_test_eq_error(error, CPL_ERROR_NONE);

                    time0 = cpl_test_get_cputime();
                    error = cpl_mask_filter(filtered, raw, kernel,
                                            mfilter[k], CPL_BORDER_ZERO);
                    cpl_test_eq_error(error, CPL_ERROR_NONE);
                    time1 = cpl_test_get_cputime();
                    if (time1 > time0)
                        time_mask += time1 - time0;

                    time0 = cpl_test_get_cputime();
                    error = ffilter[k](rawcopy, matrix);
                    cpl_test_eq_error(error, CPL_ERROR_NONE);
                    time1 = cpl_test_get_cputime();
                    if (time1 > time0)
                        time_matrix += time1 - time0;

                    cpl_test_eq_mask(filtered, rawcopy);

                    time0 = cpl_test_get_cputime();
                    error = cpl_mask_filter_bf(filtered, raw, kernel,
                                               mfilter[k], CPL_BORDER_ZERO);
                    cpl_test_eq_error(error, CPL_ERROR_NONE);
                    time1 = cpl_test_get_cputime();
                    if (time1 > time0)
                        time_bf += time1 - time0;

                    cpl_test_eq_mask(filtered, rawcopy);

                    if (mfilter[k] == CPL_FILTER_OPENING ||
                        mfilter[k] == CPL_FILTER_CLOSING) {

                        /* Opening and closing can also be done in-place */

                        error = cpl_mask_copy(rawcopy, raw, 1, 1);
                        cpl_test_eq_error(error, CPL_ERROR_NONE);
                        error = cpl_mask_filter(rawcopy, rawcopy, kernel,
                                                mfilter[k], CPL_BORDER_ZERO);
                        cpl_test_eq_error(error, CPL_ERROR_NONE);

                        cpl_test_eq_mask(filtered, rawcopy);
                    }

                    /* Duality test */
                    cpl_mask_not(raw);

                    error = cpl_mask_filter(filtered, raw, kernel,
                                            mdual[k], CPL_BORDER_ZERO);
                    cpl_test_eq_error(error, CPL_ERROR_NONE);

                    cpl_mask_not(filtered);

                    /* No duality on border */
                    cpl_mask_fill_border(filtered, mx, my, CPL_BINARY_0);
                    cpl_mask_fill_border(rawcopy,  mx, my, CPL_BINARY_0);

                    cpl_test_eq_mask(filtered, rawcopy);

                    error = cpl_mask_filter_bf(filtered, raw, kernel,
                                               mdual[k], CPL_BORDER_ZERO);
                    cpl_test_eq_error(error, CPL_ERROR_NONE);

                    cpl_mask_not(filtered);

                    /* No duality on border */
                    cpl_mask_fill_border(filtered, mx, my, CPL_BINARY_0);
                    cpl_mask_fill_border(rawcopy,  mx, my, CPL_BINARY_0);

                    cpl_test_eq_mask(filtered, rawcopy);

                    /* End of duality test - restore */
                    cpl_mask_not(raw);

                    if (mfilter[k] == CPL_FILTER_OPENING ||
                        mfilter[k] == CPL_FILTER_CLOSING) {
                        /* Idempotency test */
                        int nidem = 2;

                        do {
                            error = cpl_mask_filter(filtered, raw, kernel,
                                                    mfilter[k], CPL_BORDER_ZERO);
                            cpl_test_eq_error(error, CPL_ERROR_NONE);
                        } while (--nidem > 0);

                        /* No idempotency on border */
                        cpl_mask_fill_border(filtered, mx, my, CPL_BINARY_0);

                        cpl_test_eq_mask(filtered, rawcopy);

                        nidem = 2;

                        do {
                            error = cpl_mask_filter_bf(filtered, raw, kernel,
                                                       mfilter[k],
                                                       CPL_BORDER_ZERO);
                            cpl_test_eq_error(error, CPL_ERROR_NONE);
                        } while (--nidem > 0);

                        /* No idempotency on border */
                        cpl_mask_fill_border(filtered, mx, my, CPL_BINARY_0);

                        cpl_test_eq_mask(filtered, rawcopy);

                    }
                }

                cpl_mask_delete(filtered);
                cpl_mask_delete(rawcopy);
                cpl_mask_delete(kernel);
                cpl_matrix_delete(matrix);
            }
        }
    }

    if (time_mask > 0.0 || time_matrix > 0.0 || time_bf > 0.0)
        cpl_msg_info(cpl_func, "Time to filter, mask <=> brute force <=> "
                     "matrix [s]: %g <=> %g <=> %g",
                     time_mask, time_bf, time_matrix);

    cpl_mask_delete(raw);
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief       Benchmark the CPL function
  @param nsize The object size to use for testing
  @param nreps The number of repetitions
  @return      void
  
 */
/*----------------------------------------------------------------------------*/
static void cpl_mask_count_bench(int nsize, int nreps) {

    cpl_mask * mask1 = cpl_mask_new(nsize, nsize);
    cpl_mask * mask2;
    cpl_error_code error;
    double time0, time_mask = 0.0;
    int irep;

    error = cpl_mask_fill_test(mask1, 2.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    mask2 = cpl_mask_duplicate(mask1);
    error = cpl_mask_not(mask2);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    time0 = cpl_test_get_cputime();

    for (irep = 0; irep < nreps; irep++) {

        cpl_test_eq(cpl_mask_count(mask1) + cpl_mask_count(mask2),
                    nsize * nsize);
    }

    time_mask = cpl_test_get_cputime() - time0;

    cpl_msg_info(cpl_func, "Time to count elements 2 X %d mask of size %d x %d "
                 "[s]: %g", nreps, nsize, nsize, time_mask);
    cpl_mask_delete(mask1);
    cpl_mask_delete(mask2);
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief       Test the CPL function against the matrix based ones
  @param nsize The object size to use for testing
  @param nreps The number of repetitions
  @return      void
  
 */
/*----------------------------------------------------------------------------*/
static void cpl_mask_filter_bench(int nsize, int nreps, int minhx, int maxhx,
                                  int minhy, int maxhy) {

    const cpl_filter_mode mfilter[] = {CPL_FILTER_EROSION, CPL_FILTER_DILATION,
                                       CPL_FILTER_OPENING, CPL_FILTER_CLOSING};

    const cpl_filter_mode mdual[] = {CPL_FILTER_DILATION, CPL_FILTER_EROSION,
                                     CPL_FILTER_CLOSING,  CPL_FILTER_OPENING};

    const size_t nfilter = sizeof(mfilter)/sizeof(*mfilter);
    size_t k;

    cpl_mask * raw = cpl_mask_new(nsize, nsize);
    int hx, hy;
    cpl_error_code error;
    double time_mask = 0.0;
    double time_bf   = 0.0;
    int irep;

    error = cpl_mask_fill_test(raw, 2.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_msg_info(cpl_func, "Matrix testing of cpl_mask_filter() filters with "
                 "%d x %d mask", nsize, nsize);

    for (irep = 0; irep < nreps; irep++) {
        for (hy = minhy; hy <= maxhy; hy++) {
            for (hx = minhx; hx <= maxhx; hx++) {
                const int mx = 1 + 2 * hx;
                const int my = 1 + 2 * hy;
                int do_full;

                if (mx > nsize || my > nsize) continue;

                for (do_full = mx * my > 1 ? 0 : 1; do_full < 2; do_full++) {
                    cpl_mask   * kernel = cpl_mask_new(mx, my);

                    cpl_mask * filtered = cpl_mask_new(nsize, nsize);
                    cpl_mask * rawcopy = cpl_mask_new(nsize, nsize);

                    if (do_full) {
                        error = cpl_mask_not(kernel);
                        cpl_test_eq_error(error, CPL_ERROR_NONE);
                    } else {
                        int ncount;
                        do {
                            error = cpl_mask_fill_test(kernel, 0.0);
                            cpl_test_eq_error(error, CPL_ERROR_NONE);
                            ncount = cpl_mask_count(kernel);
                        } while (ncount == 0 || ncount == mx * my);

                        cpl_msg_info(cpl_func, "The %u filters w. %d x %d kernel "
                                     "(full and %d element(s))", (unsigned)nfilter,
                                     mx, my, ncount);
                    }

                    for (k = 0; k < nfilter; k++) {
                        double     time0, time1;

                        time0 = cpl_test_get_cputime();
                        error = cpl_mask_filter(filtered, raw, kernel,
                                                mfilter[k], CPL_BORDER_ZERO);
                        cpl_test_eq_error(error, CPL_ERROR_NONE);
                        time1 = cpl_test_get_cputime();
                        if (time1 > time0)
                            time_mask += time1 - time0;

                        time0 = cpl_test_get_cputime();
                        error = cpl_mask_filter_bf(rawcopy, raw, kernel,
                                                   mfilter[k], CPL_BORDER_ZERO);
                        cpl_test_eq_error(error, CPL_ERROR_NONE);
                        time1 = cpl_test_get_cputime();
                        if (time1 > time0)
                            time_bf += time1 - time0;

                        cpl_test_eq_mask(filtered, rawcopy);

                        if (mfilter[k] == CPL_FILTER_OPENING ||
                            mfilter[k] == CPL_FILTER_CLOSING) {

                            /* Opening and closing can also be done in-place */

                            error = cpl_mask_copy(rawcopy, raw, 1, 1);
                            cpl_test_eq_error(error, CPL_ERROR_NONE);
                            error = cpl_mask_filter(rawcopy, rawcopy, kernel,
                                                    mfilter[k], CPL_BORDER_ZERO);
                            cpl_test_eq_error(error, CPL_ERROR_NONE);

                            cpl_test_eq_mask(filtered, rawcopy);
                        }

                        /* Duality test */
                        cpl_mask_not(raw);

                        error = cpl_mask_filter(filtered, raw, kernel,
                                                mdual[k], CPL_BORDER_ZERO);
                        cpl_test_eq_error(error, CPL_ERROR_NONE);

                        cpl_mask_not(filtered);

                        /* No duality on border */
                        cpl_mask_fill_border(filtered, mx, my, CPL_BINARY_0);
                        cpl_mask_fill_border(rawcopy,  mx, my, CPL_BINARY_0);

                        cpl_test_eq_mask(filtered, rawcopy);

                        error = cpl_mask_filter_bf(filtered, raw, kernel,
                                                   mdual[k], CPL_BORDER_ZERO);
                        cpl_test_eq_error(error, CPL_ERROR_NONE);

                        cpl_mask_not(filtered);

                        /* No duality on border */
                        cpl_mask_fill_border(filtered, mx, my, CPL_BINARY_0);
                        cpl_mask_fill_border(rawcopy,  mx, my, CPL_BINARY_0);

                        cpl_test_eq_mask(filtered, rawcopy);

                        /* End of duality test - restore */
                        cpl_mask_not(raw);

                        if (mfilter[k] == CPL_FILTER_OPENING ||
                            mfilter[k] == CPL_FILTER_CLOSING) {
                            /* Idempotency test */
                            int nidem = 2;

                            do {
                                error = cpl_mask_filter(filtered, raw, kernel,
                                                        mfilter[k], CPL_BORDER_ZERO);
                                cpl_test_eq_error(error, CPL_ERROR_NONE);
                            } while (--nidem > 0);

                            /* No idempotency on border */
                            cpl_mask_fill_border(filtered, mx, my, CPL_BINARY_0);

                            cpl_test_eq_mask(filtered, rawcopy);

                            nidem = 2;

                            do {
                                error = cpl_mask_filter_bf(filtered, raw, kernel,
                                                           mfilter[k],
                                                           CPL_BORDER_ZERO);
                                cpl_test_eq_error(error, CPL_ERROR_NONE);
                            } while (--nidem > 0);

                            /* No idempotency on border */
                            cpl_mask_fill_border(filtered, mx, my, CPL_BINARY_0);

                            cpl_test_eq_mask(filtered, rawcopy);

                        }
                    }

                    cpl_mask_delete(filtered);
                    cpl_mask_delete(rawcopy);
                    cpl_mask_delete(kernel);
                }
            }
        }
    }

    if (time_mask > 0.0 || time_bf > 0.0)
        cpl_msg_info(cpl_func, "Time to filter, mask <=> brute force [s]: "
                     "%g <=> %g", time_mask, time_bf);

    cpl_mask_delete(raw);
}



/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Compute a morphological opening
  @param    in      input mask
  @param    ker     binary kernel (0 for 0, any other value is considered as 1)
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
  @note The original filter function, only used for testing

  The morphological opening is an erosion followed by a dilation.
  The input mask is modified.
  The input kernel should have an odd number of rows and columns.
  The maximum size of the kernel is 31x31.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if (one of) the input pointer(s) is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the kernel is such that the erosion or the
    dilation cannot be done
 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_mask_opening_matrix(
        cpl_mask            *   in,
        const cpl_matrix    *    ker)
{
    cpl_ensure_code(in && ker, CPL_ERROR_NULL_INPUT);
    if (cpl_mask_erosion_matrix(in, ker) != CPL_ERROR_NONE)
        return CPL_ERROR_ILLEGAL_INPUT;
    if (cpl_mask_dilation_matrix(in, ker) != CPL_ERROR_NONE)
        return CPL_ERROR_ILLEGAL_INPUT;
    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Compute a morphological closing
  @param    in      input mask
  @param    ker     binary kernel (0 for 0, any other value is considered as 1)
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
  @note The original filter function, only used for testing

  The morphological closing is a dilation followed by an erosion.
  The input mask is modified.
  The input kernel should have an odd number of rows and columns.
  The maximum size of the kernel is 31x31.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if (one of) the input pointer(s) is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the kernel is such that the erosion or the
    dilation cannot be done
 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_mask_closing_matrix(
        cpl_mask            *   in,
        const cpl_matrix    *   ker)
{
    cpl_ensure_code(in && ker, CPL_ERROR_NULL_INPUT);
    if (cpl_mask_dilation_matrix(in, ker) != CPL_ERROR_NONE)
        return CPL_ERROR_ILLEGAL_INPUT;
    if (cpl_mask_erosion_matrix(in, ker) != CPL_ERROR_NONE)
        return CPL_ERROR_ILLEGAL_INPUT;
    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Compute a morphological erosion
  @param    in      input mask
  @param    ker     binary kernel (0 for 0, any other value is considered as 1)
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
  @note The original filter function, only used for testing

  The input mask is modified.
  The input kernel should have an odd number of rows and columns.
  The maximum size of the kernel is 31x31.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if (one of) the input pointer(s) is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the kernel is not as requested
 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_mask_erosion_matrix(
        cpl_mask            *   in,
        const cpl_matrix    *   ker)
{
    const int    nx      = cpl_mask_get_size_x(in);
    const int    ny      = cpl_mask_get_size_y(in);
    cpl_binary * datain  = cpl_mask_get_data(in);
    cpl_binary * dataout;
    cpl_mask        *   out;
    const double    *   ker_arr;
    int                 nc, nr;
    int                 hsx, hsy;
    int                 curr_pos, im_pos, filt_pos;
    int                 i, j, k, l;

    /* Test entries */
    cpl_ensure_code(in && ker, CPL_ERROR_NULL_INPUT);

    /* Get kernel informations */
    nr = cpl_matrix_get_nrow(ker);
    nc = cpl_matrix_get_ncol(ker);
    ker_arr = cpl_matrix_get_data_const(ker);

    /* Test the kernel validity */
    cpl_ensure_code(nc%2 && nr%2, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(nc<=31 && nr<=31, CPL_ERROR_ILLEGAL_INPUT);

    /* Initialise */
    hsx = (nc-1) / 2;
    hsy = (nr-1) / 2;

    /* Create a tmp mask */
    out = cpl_mask_new(nx, ny);
    dataout = cpl_mask_get_data(out);

    /* Main filter loop */
    for (j=0; j<ny; j++) {
        for (i=0; i<nx; i++) {
            /* Curent pixel position */
            curr_pos = i + j*nx;
            /* Edges are not computed   */
            if ((i<hsx) || (i>=nx-hsx) || (j<hsy) || (j>=ny-hsy)) {
                dataout[curr_pos] = CPL_BINARY_0;
            } else {
                /* Initialise */
                dataout[curr_pos] = CPL_BINARY_1;
                /* Go into upper left corner of current pixel   */
                im_pos = curr_pos - hsx + hsy*nx;
                filt_pos = 0;
                for (k=0; k<nr; k++) {
                    for (l=0; l<nc; l++) {
                        if (((datain)[im_pos] == CPL_BINARY_0) &&
                            (fabs(ker_arr[filt_pos]) > FLT_MIN))
                            (dataout)[curr_pos] = CPL_BINARY_0;
                        /* Next col */
                        filt_pos++;
                        im_pos++;
                    }
                    /* Next row */
                    im_pos -= nx + nc;
                }
            }
        }
    }
    memcpy(datain, dataout, nx * ny * sizeof(cpl_binary));
    cpl_mask_delete(out);
    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Compute a morphological dilation
  @param    in      input mask
  @param    ker     binary kernel (0 for 0, any other value is considered as 1)
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
  @note The original filter function, only used for testing

  The input mask is modified.
  The input kernel should have an odd number of rows and columns.
  The maximum size of the kernel is 31x31.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if (one of) the input pointer(s) is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the kernel is not as requested
 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_mask_dilation_matrix(
        cpl_mask            *   in,
        const cpl_matrix    *   ker)
{
    const int nx = cpl_mask_get_size_x(in);
    const int ny = cpl_mask_get_size_y(in);
    cpl_binary * datain  = cpl_mask_get_data(in);
    cpl_binary * dataout;
    cpl_mask        *   out;
    const double    *   ker_arr;
    int                 nc, nr;
    int                 hsx, hsy;
    int                 curr_pos, im_pos, filt_pos;
    int                 i, j, k, l;

    /* Test entries */
    cpl_ensure_code(in && ker, CPL_ERROR_NULL_INPUT);

    /* Get kernel informations */
    nr = cpl_matrix_get_nrow(ker);
    nc = cpl_matrix_get_ncol(ker);
    ker_arr = cpl_matrix_get_data_const(ker);

    /* Test the kernel validity */
    cpl_ensure_code(nc%2 && nr%2, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(nc<=31 && nr<=31, CPL_ERROR_ILLEGAL_INPUT);

    /* Initialise */
    hsx = (nc-1) / 2;
    hsy = (nr-1) / 2;

    /* Create a tmp mask */
    out = cpl_mask_new(nx, ny);
    dataout = cpl_mask_get_data(out);

    /* Main filter loop */
    for (j=0; j<ny; j++) {
        for (i=0; i<nx; i++) {
            /* Curent pixel position */
            curr_pos = i + j*nx;
            /* Edges are not computed   */
            if ((i<hsx) || (i>=nx-hsx) || (j<hsy) || (j>=ny-hsy)) {
                (dataout)[curr_pos] = CPL_BINARY_0;
            } else {
                /* Initialise */
                (dataout)[curr_pos] = CPL_BINARY_0;
                /* Go into upper left corner of current pixel   */
                im_pos = curr_pos - hsx + hsy*nx;
                filt_pos = 0;
                for (k=0; k<nr; k++) {
                    for (l=0; l<nc; l++) {
                        if (((datain)[im_pos] == CPL_BINARY_1) &&
                                (fabs(ker_arr[filt_pos]) > FLT_MIN))
                            (dataout)[curr_pos] = CPL_BINARY_1;
                        /* Next col */
                        filt_pos++;
                        im_pos++;
                    }
                    /* Next row */
                    im_pos -= nx + nc;
                }
            }
        }
    }
    memcpy(datain, dataout, nx * ny * sizeof(cpl_binary));
    cpl_mask_delete(out);
    return CPL_ERROR_NONE;
}



/*----------------------------------------------------------------------------*/
/**
  @brief  Brute force (single byte) filter a mask using a binary kernel
  @param  self   Filtered mask
  @param  other  Mask to filter
  @param  kernel Elements to use, if set to CPL_BINARY_1
  @param  filter CPL_FILTER_EROSION, CPL_FILTER_DILATION, CPL_FILTER_OPENING,
                 CPL_FILTER_CLOSING
  @param  border CPL_BORDER_NOP, CPL_BORDER_ZERO or CPL_BORDER_COPY
  @return CPL_ERROR_NONE or the relevant CPL error code
  @see cpl_mask_filter()

 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_mask_filter_bf(cpl_mask * self, const cpl_mask * other,
                               const cpl_mask * kernel,
                               cpl_filter_mode filter,
                               cpl_border_mode border)
{

    /* Modified from cpl_mask_filter() :-((((( */

    const int nsx    = cpl_mask_get_size_x(self);
    const int nsy    = cpl_mask_get_size_y(self);
    const int nx     = cpl_mask_get_size_x(other);
    const int ny     = cpl_mask_get_size_y(other);
    const int mx     = cpl_mask_get_size_x(kernel);
    const int my     = cpl_mask_get_size_y(kernel);
    const int hsizex = mx >> 1;
    const int hsizey = my >> 1;
    const cpl_binary * pmask  = cpl_mask_get_data_const(kernel);
    const cpl_binary * pother = cpl_mask_get_data_const(other);
    cpl_binary       * pself  = cpl_mask_get_data(self);
    /* assert( sizeof(cpl_binary) == 1 ) */
    const cpl_binary * polast = pother + nx * ny;
    const cpl_binary * psrows = pself + nsx * (1 + hsizey);
    /* pmask may not overlap pself at all */
    const cpl_binary * pmlast = pmask + mx * my;
    const cpl_binary * pslast = pself + nsx * nsy;
    /* In filtering it is generally faster with a special case for the 
       full kernel. Some tests indicate that this is not the case of mask
       filtering. This may be due to the typically small kernels
       - and the simple operations involved.
    */
    void (*filter_func)(cpl_binary *, const cpl_binary *, const cpl_binary *,
                        int, int, int, int, cpl_border_mode);


    cpl_ensure_code(self   != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(other  != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(kernel != NULL, CPL_ERROR_NULL_INPUT);

    if (filter == CPL_FILTER_OPENING || filter == CPL_FILTER_CLOSING) {
        cpl_ensure_code(border == CPL_BORDER_ZERO || border == CPL_BORDER_COPY,
                        CPL_ERROR_UNSUPPORTED_MODE);
    } else {
        /* pself has to be above all of the other buffer, or */
        /* ...pother has to be above the first hsize+1 rows of pself */
        cpl_ensure_code(pself >= polast || pother >= psrows,
                        CPL_ERROR_UNSUPPORTED_MODE);
        cpl_ensure_code(border == CPL_BORDER_NOP || border == CPL_BORDER_ZERO ||
                        border == CPL_BORDER_COPY, CPL_ERROR_UNSUPPORTED_MODE);
    }

    /* If this check fails, the caller is doing something really weird... */
    cpl_ensure_code(pmask >= pslast || pself >= pmlast,
                    CPL_ERROR_UNSUPPORTED_MODE);

    /* Only odd-sized masks allowed */
    cpl_ensure_code((mx&1) == 1, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code((my&1) == 1, CPL_ERROR_ILLEGAL_INPUT);

    cpl_ensure_code(mx <= nx, CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_ensure_code(my <= ny, CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (border == CPL_BORDER_CROP) {
        cpl_ensure_code(nsx == nx - 2 * hsizex, CPL_ERROR_INCOMPATIBLE_INPUT);

        cpl_ensure_code(nsy == ny - 2 * hsizey, CPL_ERROR_INCOMPATIBLE_INPUT);

    } else {
        cpl_ensure_code(nsx == nx, CPL_ERROR_INCOMPATIBLE_INPUT);

        cpl_ensure_code(nsy == ny, CPL_ERROR_INCOMPATIBLE_INPUT);

    }

    cpl_ensure_code(!cpl_mask_is_empty(kernel), CPL_ERROR_DATA_NOT_FOUND);

    filter_func = NULL;

    if (filter == CPL_FILTER_EROSION) {
        filter_func = cpl_mask_erosion_bf;
    } else if (filter == CPL_FILTER_DILATION) {
        filter_func = cpl_mask_dilation_bf;
    } else if (filter == CPL_FILTER_OPENING) {
        filter_func = cpl_mask_opening_bf;
    } else if (filter == CPL_FILTER_CLOSING) {
        filter_func = cpl_mask_closing_bf;
    }

    if (filter_func != NULL) {

        filter_func(pself, pother, pmask, nx, ny, hsizex, hsizey, border);

    } else {
        return cpl_error_set(cpl_func, CPL_ERROR_UNSUPPORTED_MODE);
    }

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Apply the erosion filter
  @param  self    The output binary 2D array to hold the filtered result
  @param  other   The input  binary 2D array to filter
  @param  kernel  The input  binary 2D kernel
  @param  nx      The X-size of the input array
  @param  ny      The Y-size of the input array
  @param  hx      The X-half-size of the kernel
  @param  hy      The Y-half-size of the kernel
  @return void
  @see cpl_mask_erosion_()
  @note No error checking in this internal function!

  FIXME: Changes to this function must also be made to cpl_mask_dilation_bf()

 */
/*----------------------------------------------------------------------------*/
static
void cpl_mask_erosion_bf(cpl_binary * self, const cpl_binary * other,
                       const cpl_binary * kernel,
                       int nx, int ny, int hx, int hy, cpl_border_mode border)
{

    /* Modified from cpl_mask_erosion_() :-((((( */

    if (border == CPL_BORDER_NOP || border == CPL_BORDER_ZERO ||
        border == CPL_BORDER_COPY) {
        const int mx = 2 * hx + 1;
        int i, j;

        /* Handle border for first hy rows and first hx elements of next row */
        if (border == CPL_BORDER_ZERO) {
            (void)memset(self, CPL_BINARY_0, hx + hy * nx);
        } else if (border == CPL_BORDER_COPY) {
            (void)memcpy(self, other, hx + hy * nx);
        }

        self   += hy * nx;
        kernel += hx;

        for (j = hy; j < ny-hy; j++, self += nx, other += nx) {

            if (j > hy) {
                /* Do also last hx border elements of previous row */
                if (border == CPL_BORDER_ZERO) {
                    (void)memset(self - hx, CPL_BINARY_0, 2 * hx);
                } else if (border == CPL_BORDER_COPY) {
                    (void)memcpy(self - hx, other - hx + hy * nx, 2 * hx);
                }
            }

            for (i = hx; i < nx-hx; i++) {
                const cpl_binary * otheri  = other + i;
                const cpl_binary * kernelk = kernel;
                int k, l;

                for (k = -hy; k <= hy; k++, otheri += nx, kernelk += mx) {
                    /* FIXME: Do binary ops using 64, 32, 16 bit integers */
                    for (l = -hx; l <= hx; l++) {
                        if (!otheri[l] && kernelk[l]) break;
                    }
                    if (l <= hx) break;
                }
                self[i] = k <= hy ? CPL_BINARY_0 : CPL_BINARY_1;
            }
        }

        /* Do also last hx border elements of previous row */
        if (border == CPL_BORDER_ZERO) {
            (void)memset(self - hx, CPL_BINARY_0, hx + hy * nx);
        } else if (border == CPL_BORDER_COPY) {
            (void)memcpy(self - hx, other - hx + hy * nx, hx + hy * nx);
        }
    }
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Apply the dilation filter
  @param  self    The output binary 2D array to hold the filtered result
  @param  other   The input  binary 2D array to filter
  @param  kernel  The input  binary 2D kernel - rows padded with 0 to fit u32
  @param  nx      The X-size of the input array
  @param  ny      The Y-size of the input array
  @param  hx      The X-half-size of the kernel
  @param  hy      The Y-half-size of the kernel
  @return void
  @note No error checking in this internal function!
  @see cpl_mask_erosion_bf()
 
  Modified from cpl_mask_erosion_bf().
 
  FIXME: Changes to this function must also be made to cpl_mask_erosion_bf()

 */
/*----------------------------------------------------------------------------*/
static
void cpl_mask_dilation_bf(cpl_binary * self, const cpl_binary * other,
                       const cpl_binary * kernel,
                       int nx, int ny, int hx, int hy, cpl_border_mode border)
{

    if (border == CPL_BORDER_NOP || border == CPL_BORDER_ZERO ||
        border == CPL_BORDER_COPY) {
        const int mx = 2 * hx + 1;
        int i, j;

        /* Handle border for first hy rows and first hx elements of next row */
        if (border == CPL_BORDER_ZERO) {
            (void)memset(self, CPL_BINARY_0, hx + hy * nx);
        } else if (border == CPL_BORDER_COPY) {
            (void)memcpy(self, other, hx + hy * nx);
        }

        self  += hy * nx;  /* self-col now indexed from -hy to hy */
        kernel += hx;

        for (j = hy; j < ny-hy; j++, self += nx, other += nx) {

            if (j > hy) {
                /* Do also last hx border elements of previous row */
                if (border == CPL_BORDER_ZERO) {
                    (void)memset(self - hx, CPL_BINARY_0, 2 * hx);
                } else if (border == CPL_BORDER_COPY) {
                    (void)memcpy(self - hx, other - hx + hy * nx, 2 * hx);
                }
            }

            for (i = hx; i < nx-hx; i++) {
                const cpl_binary * otheri  = other + i;
                const cpl_binary * kernelk = kernel;
                int k, l;

                for (k = -hy; k <= hy; k++, otheri += nx, kernelk += mx) {
                    /* FIXME: Do binary ops using 64, 32, 16 bit integers */
                    for (l = -hx; l <= hx; l++) {
                        if (otheri[l] && kernelk[l]) break;
                    }
                    if (l <= hx) break;
                }
                self[i] = k <= hy ? CPL_BINARY_1 : CPL_BINARY_0;
            }
        }

        /* Do also last hx border elements of previous row */
        if (border == CPL_BORDER_ZERO) {
            (void)memset(self - hx, CPL_BINARY_0, hx + hy * nx);
        } else if (border == CPL_BORDER_COPY) {
            (void)memcpy(self - hx, other - hx + hy * nx, hx + hy * nx);
        }
    }
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Apply the opening filter
  @param  self    The output binary 2D array to hold the filtered result
  @param  other   The input  binary 2D array to filter
  @param  kernel  The input  binary 2D kernel - rows padded with 0 to fit u32
  @param  nx      The X-size of the input array
  @param  ny      The Y-size of the input array
  @param  hx      The X-half-size of the kernel
  @param  hy      The Y-half-size of the kernel
  @return void
  @note No error checking in this internal function!
  @see cpl_mask_opening_()
 
 */
/*----------------------------------------------------------------------------*/
static
void cpl_mask_opening_bf(cpl_binary * self, const cpl_binary * other,
                       const cpl_binary * kernel,
                       int nx, int ny, int hx, int hy, cpl_border_mode border)
{
    cpl_binary * middle = cpl_malloc((size_t)nx * ny * sizeof(cpl_binary));

    cpl_mask_erosion_bf(middle, other, kernel, nx, ny, hx, hy, border);
    cpl_mask_dilation_bf(self, middle, kernel, nx, ny, hx, hy, border);
    cpl_free(middle);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Apply the closing filter
  @param  self    The output binary 2D array to hold the filtered result
  @param  other   The input  binary 2D array to filter
  @param  kernel  The input  binary 2D kernel - rows padded with 0 to fit u32
  @param  nx      The X-size of the input array
  @param  ny      The Y-size of the input array
  @param  hx      The X-half-size of the kernel
  @param  hy      The Y-half-size of the kernel
  @return void
  @note No error checking in this internal function!
  @see cpl_mask_closing_()
 
 */
/*----------------------------------------------------------------------------*/
static
void cpl_mask_closing_bf(cpl_binary * self, const cpl_binary * other,
                       const cpl_binary * kernel,
                       int nx, int ny, int hx, int hy, cpl_border_mode border)
{
    cpl_binary * middle = cpl_malloc((size_t)nx * ny * sizeof(cpl_binary));

    cpl_mask_dilation_bf(middle, other, kernel, nx, ny, hx, hy, border);
    cpl_mask_erosion_bf(self,   middle, kernel, nx, ny, hx, hy, border);
    cpl_free(middle);
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Test cpl_mask_flip(), cpl_mask_turn()
  @param  self    The mask to test
  @return void

   A: 
       Rotate 90 counterclock-wise, then
       flip around the vertical axis, then
       flip around x=y, and
       it should be back to its original.

   B: 

       Rotate 90 counterclock-wise, then
       flip around the horizontal axis, then
       flip around x=-y, and
       it should be back to its original.


 */
/*----------------------------------------------------------------------------*/
static void cpl_mask_flip_turn_test(const cpl_mask * self)
{

    cpl_error_code error;
    cpl_mask * tmp = cpl_mask_duplicate(self);

    error = cpl_mask_turn(tmp, -1);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_mask_flip(tmp, 2);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_mask_flip(tmp, 1);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_eq_mask(self, tmp);

    error = cpl_mask_turn(tmp, -1);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_mask_flip(tmp, 0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_mask_flip(tmp, 3);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_test_eq_mask(self, tmp);

    cpl_mask_delete(tmp);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Test the CPL function
  @param  mx      The maximum X-size of the tested object
  @param  my      The maximum Y-size of the tested object
  @return void
  @note No error checking in this internal function!
 
 */
/*----------------------------------------------------------------------------*/
static void cpl_mask_count_window_test(int mx, int my)
{
    int ntested = 0;

    for (int nx = 1; nx <= mx; nx++) {
        for (int ny = 1; ny <= my; ny++) {
            cpl_mask           * maskoff = cpl_mask_new(nx, ny);
            cpl_mask           * maskon  = cpl_mask_new(nx, ny);
            const cpl_error_code error = cpl_mask_not(maskon);

            cpl_test_eq_error(error, CPL_ERROR_NONE);

            for (int llx = 1; llx <= nx; llx++) {
                for (int lly = 1; lly <= ny; lly++) {
                    for (int urx = llx; urx <= nx; urx++) {
                        for (int ury = lly; ury <= ny; ury++) {
                            const int ncount  = (1 + urx - llx)
                                              * (1 + ury - lly);
                            const cpl_size ijon =
                                cpl_mask_count_window(maskon, llx, lly,
                                                      urx, ury);
                            const cpl_size ijoff =
                                cpl_mask_count_window(maskoff, llx, lly,
                                                      urx, ury);

                            cpl_test_error(CPL_ERROR_NONE);
                            cpl_test_zero(ijoff);
                            cpl_test_eq(ijon, ncount);
                            ntested += 2;
                        }
                    }
                }
            }
            cpl_mask_delete(maskon);
            cpl_mask_delete(maskoff);
        }
    }

    cpl_msg_info(cpl_func, "Tests of cpl_mask_count_window() w. %d X %d mask: "
                 "%d", mx, my, ntested);
}
