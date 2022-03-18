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

#include "cpl_image_bpm.h"
#include "cpl_image_gen.h"
#include "cpl_image_io.h"
#include "cpl_mask.h"
#include "cpl_test.h"
#include "cpl_memory.h"

/*-----------------------------------------------------------------------------
                                   Defines
 -----------------------------------------------------------------------------*/

#ifndef IMAGESZ
#define IMAGESZ            512
#endif

/*-----------------------------------------------------------------------------
                        Private function prototypes
 -----------------------------------------------------------------------------*/

static void cpl_image_bpm_reject_value_test(void);

/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/
int main(void)
{
    cpl_image * img;
    const cpl_mask * tmp_map;
    cpl_mask  * map;
    const int   nx = 10;
    const int   ny = 10;
    const int   nbad = 10;
    /* the bad pixels positions */
    const int   bad_pos_x[] = {4, 5, 6, 4, 5, 6, 4, 5, 6, 8};
    const int   bad_pos_y[] = {5, 5, 5, 6, 6, 6, 7, 7, 7, 9};
    cpl_error_code error;
    int         i;


    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    /* Insert tests below */
    /* Generate test image */
    cpl_msg_info(cpl_func, "Create a DOUBLE %dx%d test image", nx, ny);
    img = cpl_image_fill_test_create(nx, ny);

    tmp_map = cpl_image_get_bpm_const(img);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_null(tmp_map);

    /* Cause the bpm to be created */
    map = cpl_image_get_bpm(img);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(map);

    /* Set some bad pixels */
    cpl_msg_info(cpl_func, "Set %d bad pixels in the test image", nbad);
    for (i=0; i<nbad; i++) {
        cpl_test_zero( cpl_image_reject(img, bad_pos_x[i], bad_pos_y[i]) );
    }

    /* Count the nb of bad pixels */
    cpl_msg_info(cpl_func, "Count the number of bad pixels");
    cpl_test_eq(cpl_image_count_rejected(img), nbad);
    
    /* Test cpl_image_is_rejected() */
    cpl_msg_info(cpl_func, "Test if some pixels are good or bad");
    for (i=0; i<nbad; i++) {
        int    is_rejected;

        cpl_test( cpl_image_is_rejected(img, bad_pos_x[i], bad_pos_y[i]) );

        (void)cpl_image_get(img, bad_pos_x[i], bad_pos_y[i], &is_rejected);
        cpl_test_error( CPL_ERROR_NONE );
        cpl_test( is_rejected );

    }
    cpl_test_zero( cpl_image_is_rejected(img, 1, 1) );    

    /* Test cpl_image_accept() */
    cpl_msg_info(cpl_func, "Set the bad pixels as good");
    for (i=0; i<nbad; i++) {
        cpl_test_zero(cpl_image_accept(img, bad_pos_x[i], bad_pos_y[i]) );
        cpl_test_eq( cpl_image_count_rejected(img), nbad-(i+1) );    
    }
 
    /* Count the nb of bad pixels */
    cpl_msg_info(cpl_func, "Count the number of bad pixels");
    cpl_test_zero(cpl_image_count_rejected(img) );    
   
    /* Set some bad pixels */
    cpl_msg_info(cpl_func, "Set the bad pixels as bad again");
    for (i=0; i<nbad; i++) {
        cpl_test_zero(cpl_image_reject(img,  bad_pos_x[i], bad_pos_y[i]) );
    }
 
    /* Get the bad pixel map */
    cpl_msg_info(cpl_func, "Get the bad pixels map from the test image");
    tmp_map = cpl_image_get_bpm_const(img);
    cpl_test_nonnull( tmp_map );    
    map = cpl_image_get_bpm(img);
    cpl_test_nonnull( map );    
    cpl_test_eq_mask(tmp_map, map);
    map = cpl_mask_duplicate(tmp_map);    
    cpl_test_nonnull( map );
    
    /* Count the nb of bad pixels */
    cpl_msg_info(cpl_func, "Count the number of bad pixels");
    cpl_test_eq( cpl_image_count_rejected(img), nbad);
    
    /* Reset the bad pixels map */
    cpl_msg_info(cpl_func, "Reset the bad pixels map");
    cpl_test_zero(cpl_image_accept_all(img));    
    
    /* Count the nb of bad pixels */
    cpl_msg_info(cpl_func, "Count the number of bad pixels");
    cpl_test_zero(cpl_image_count_rejected(img));

    /* Set the bad pixels map back from the map */
    cpl_msg_info(cpl_func, "Set the bad pixels in the test image from the map");
    cpl_test_zero(cpl_image_reject_from_mask(img, map));    
    cpl_mask_delete(map);
    
    /* Count the nb of bad pixels */
    cpl_msg_info(cpl_func, "Count the number of bad pixels");
    cpl_test_eq( cpl_image_count_rejected(img), nbad);

    map = cpl_image_unset_bpm(img);
    cpl_test_nonnull(map);
    cpl_test_null(cpl_image_get_bpm_const(img));

    error = cpl_image_reject_from_mask(img, map);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_eq_mask(map, cpl_image_get_bpm_const(img));
    cpl_mask_delete(map);

    /* Reset the bad pixels map */
    cpl_msg_info(cpl_func, "Reset the bad pixels map");
    cpl_test_zero(cpl_image_accept_all(img));    

    i = cpl_image_is_rejected(img, 0, 1);
    cpl_test_error(CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_test_lt(i, 0);

    error = cpl_image_reject(img, 0, 1);
    cpl_test_eq_error(error, CPL_ERROR_ACCESS_OUT_OF_RANGE);

    error = cpl_image_accept(img, 0, 1);
    cpl_test_eq_error(error, CPL_ERROR_ACCESS_OUT_OF_RANGE);

    cpl_image_delete(img);

    i = cpl_image_is_rejected(NULL, 1, 1);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_lt(i, 0);

    error = cpl_image_reject(NULL, 1, 1);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

    error = cpl_image_accept(NULL, 1, 1);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

    map = cpl_image_unset_bpm(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(map);

    map = cpl_image_get_bpm(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(map);

    tmp_map = cpl_image_get_bpm_const(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(tmp_map);

    error = cpl_image_accept_all(NULL);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

    i = cpl_image_count_rejected(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_eq(i, -1);

    cpl_image_bpm_reject_value_test();

    return cpl_test_end(0);
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Test the CPL function
 */
/*----------------------------------------------------------------------------*/
static void cpl_image_bpm_reject_value_test(void)
{

    const cpl_type imtypes[] = {CPL_TYPE_DOUBLE, CPL_TYPE_FLOAT,
#ifdef CPL_FPCLASSIFY_COMPLEX
                                CPL_TYPE_DOUBLE_COMPLEX, CPL_TYPE_FLOAT_COMPLEX,
#endif
                                CPL_TYPE_INT};
    const size_t   ntyp = sizeof(imtypes)/sizeof(imtypes[0]);
    size_t         ityp;

    for (ityp = 0; ityp < ntyp; ityp++) {
        cpl_image * img = cpl_image_new(IMAGESZ, IMAGESZ, imtypes[ityp]);
        cpl_error_code code;
        cpl_size nbad;

        code = cpl_image_reject_value(img, CPL_VALUE_NOTFINITE);
        cpl_test_eq_error(code, CPL_ERROR_NONE);

        nbad = cpl_image_count_rejected(img);
        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_zero(nbad);

        code = cpl_image_reject_value(img, CPL_VALUE_ZERO);
        cpl_test_eq_error(code, CPL_ERROR_NONE);

        nbad = cpl_image_count_rejected(img);
        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_eq(nbad, IMAGESZ * IMAGESZ);

        /* Various errors */
        code = cpl_image_reject_value(img, 0); /* Result of binary and */
        cpl_test_eq_error(code, CPL_ERROR_UNSUPPORTED_MODE);

        code = cpl_image_reject_value(img, 1); /* Result of logical and/or */
        cpl_test_eq_error(code, CPL_ERROR_INVALID_TYPE);

        code = cpl_image_reject_value(img, ~CPL_VALUE_NOTFINITE);
        cpl_test_eq_error(code, CPL_ERROR_UNSUPPORTED_MODE);

        code = cpl_image_reject_value(NULL, CPL_VALUE_ZERO);
        cpl_test_eq_error(code, CPL_ERROR_NULL_INPUT);

        cpl_image_delete(img);
    }
}
