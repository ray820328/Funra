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

#include "cpl_imagelist_io.h"
#include "cpl_test.h"
#include "cpl_fits.h"
#include "cpl_tools.h"

/*-----------------------------------------------------------------------------
                                   Define
 -----------------------------------------------------------------------------*/

#ifndef IMAGESZ
#define IMAGESZ    127
#endif
#ifndef NIMAGES
#define NIMAGES     10
#endif

#define FILENAME "cpl_imagelist_test.fits"


/*-----------------------------------------------------------------------------
                                  Static functions
 -----------------------------------------------------------------------------*/

static void cpl_imagelist_save_compression_test(void);
static void cpl_imagelist_save_compression_bench(cpl_size, cpl_size,
                                                 cpl_size, cpl_size);

/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/
int main(void)
{
    const char    * file = FILENAME;
    cpl_imagelist * imlist;
    cpl_imagelist * imlist2;
    cpl_imagelist * imlista;
    cpl_imagelist * nulllist;
    cpl_image     * image;
    cpl_image     * nullimg;
    double          flags[NIMAGES];
    cpl_vector    * eraser;
    cpl_size        i;
    FILE          * stream;
    int             next;
#if IMAGESZ > 127
    const cpl_size  boxsize = 5;
#elif IMAGESZ < 25
    /* FIXME: Will the tests pass with this ? */
    const cpl_size  boxsize = 1;
#else
    const cpl_size  boxsize = IMAGESZ / 25;
#endif
    cpl_error_code  error;


    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    stream = cpl_msg_get_level() > CPL_MSG_INFO
        ? fopen("/dev/null", "a") : stdout;

    /* Insert tests below */

    cpl_test_nonnull(stream);

    eraser = cpl_vector_wrap(NIMAGES, flags);

    cpl_msg_info("", "Checking various failures");

    image = cpl_image_fill_test_create(IMAGESZ, IMAGESZ);
    cpl_test_nonnull( image );

    /* Test cpl_imagelist_new() */
    imlist = cpl_imagelist_new();
    cpl_test_nonnull( imlist );

    cpl_test_zero( cpl_imagelist_get_size(imlist) );
    cpl_test_eq( cpl_imagelist_is_uniform(imlist), 1);

    /* Failure tests involving NULL end empty lists */
    i = cpl_imagelist_get_size(NULL);
    cpl_test_error( CPL_ERROR_NULL_INPUT );
    cpl_test_eq( i, -1 );

    i = cpl_imagelist_is_uniform(NULL);
    cpl_test_error( CPL_ERROR_NULL_INPUT );
    cpl_test( i < 0);

    error = cpl_imagelist_set(NULL, image, 0);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT );

    error = cpl_imagelist_set(imlist, NULL, 0);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT );

    error = cpl_imagelist_set(imlist, image, -1);
    cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT );

    error = cpl_imagelist_set(imlist, image, 1);
    cpl_test_eq_error( error, CPL_ERROR_ACCESS_OUT_OF_RANGE );

    nullimg = cpl_imagelist_get(NULL, 0);
    cpl_test_error( CPL_ERROR_NULL_INPUT );
    cpl_test_null( nullimg );

    nullimg = cpl_imagelist_get(imlist, -1);
    cpl_test_error( CPL_ERROR_ILLEGAL_INPUT );
    cpl_test_null( nullimg );

    nullimg = cpl_imagelist_get(imlist, 0);
    cpl_test_error( CPL_ERROR_ACCESS_OUT_OF_RANGE );
    cpl_test_null( nullimg );

    error = cpl_imagelist_erase(imlist, NULL);
    cpl_test_eq_error( error, CPL_ERROR_NULL_INPUT );

    /* The elements of eraser are not initialized at this point,
       but they are also not supposed to be accessed */
    error = cpl_imagelist_erase(NULL, eraser);
    cpl_test_eq_error( error, CPL_ERROR_NULL_INPUT );

    error = cpl_imagelist_erase(imlist, eraser);
    cpl_test_eq_error( error, CPL_ERROR_INCOMPATIBLE_INPUT );

    nulllist = cpl_imagelist_duplicate(NULL);
    cpl_test_error( CPL_ERROR_NULL_INPUT );
    cpl_test_null( nulllist );

    error = cpl_imagelist_save(imlist, NULL,  CPL_TYPE_FLOAT, NULL,
                               CPL_IO_CREATE);
    cpl_test_eq_error( error, CPL_ERROR_NULL_INPUT );

    error = cpl_imagelist_save(NULL,  file, CPL_TYPE_FLOAT, NULL,
                               CPL_IO_CREATE);
    cpl_test_eq_error( error, CPL_ERROR_NULL_INPUT );

    error = cpl_imagelist_save(imlist, file,  CPL_TYPE_FLOAT, NULL,
                               CPL_IO_CREATE);
    cpl_test_eq_error( error, CPL_ERROR_ILLEGAL_INPUT );

    nulllist = cpl_imagelist_load(NULL, CPL_TYPE_INT, 0);
    cpl_test_error( CPL_ERROR_NULL_INPUT );
    cpl_test_null( nulllist );

    nulllist = cpl_imagelist_load(".", CPL_TYPE_INT, 0);
    cpl_test_error( CPL_ERROR_FILE_IO );
    cpl_test_null( nulllist );


    nulllist = cpl_imagelist_load_window(NULL, CPL_TYPE_INT, 0, 1, 1, 2, 2);
    cpl_test_error( CPL_ERROR_NULL_INPUT );
    cpl_test_null( nulllist );

    nulllist = cpl_imagelist_load_window(file, CPL_TYPE_INT, -1, 1, 1, 2, 2);
    cpl_test_error( CPL_ERROR_ILLEGAL_INPUT );
    cpl_test_null( nulllist );

    nulllist = cpl_imagelist_load_window(".", CPL_TYPE_INT, 0, 1, 1, 2, 2);
    cpl_test_error( CPL_ERROR_FILE_IO );
    cpl_test_null( nulllist );


    /* Test cpl_imagelist_duplicate() of empty list */
    imlist2 = cpl_imagelist_duplicate(imlist);
    cpl_test_nonnull( imlist2 );
    
    cpl_test_zero( cpl_imagelist_get_size(imlist2) );
    cpl_test_eq( cpl_imagelist_is_uniform(imlist2), 1);

    cpl_imagelist_empty(imlist2); /* Test cpl_imagelist_unset() */
    cpl_imagelist_delete(imlist2);
    
    cpl_msg_info("", "Create an image list of %d images", NIMAGES);

    /* Test cpl_imagelist_set() */
    for (i=0; i < NIMAGES; i++) {
        cpl_image * copy = cpl_image_fill_test_create(IMAGESZ, IMAGESZ);

        flags[i] = i % 2 ? 1.0 : -1.0;

        error = cpl_imagelist_set(imlist, copy, i);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        cpl_test_zero( cpl_imagelist_is_uniform(imlist) );
        cpl_test_eq( cpl_imagelist_get_size(imlist), i+1 );
        cpl_test_eq_ptr( cpl_imagelist_get(imlist, i), copy );

        /* Insert image twice, last image will remain twice in list */
        error = cpl_imagelist_set(imlist, copy, i + 1);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

    }
    cpl_image_delete(image);
    (void)cpl_imagelist_unset(imlist, NIMAGES);

    /* No real testing, but at least make use of 
       cpl_imagelist_dump_{structure, window} */
    cpl_imagelist_dump_structure(imlist, stream);

    cpl_imagelist_dump_window(imlist, 
                              (IMAGESZ / 2) - boxsize,
                              (IMAGESZ / 2) - boxsize,
                              (IMAGESZ / 2) + boxsize,
                              (IMAGESZ / 2) + boxsize,
                              stream);

    cpl_msg_info("", "Cast the image list");

    error = cpl_imagelist_cast(NULL, NULL, CPL_TYPE_UNSPECIFIED);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

    error = cpl_imagelist_cast(NULL, imlist, CPL_TYPE_UNSPECIFIED);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

    error = cpl_imagelist_cast(imlist, imlist, CPL_TYPE_UNSPECIFIED);
    cpl_test_eq_error(error, CPL_ERROR_INCOMPATIBLE_INPUT);

    imlist2 = cpl_imagelist_new();

    error = cpl_imagelist_cast(imlist2, imlist, CPL_TYPE_INVALID);
    cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_zero( cpl_imagelist_get_size(imlist2));

    error = cpl_imagelist_cast(imlist2, imlist, CPL_TYPE_UNSPECIFIED);
    cpl_test_eq_error(error, CPL_ERROR_INVALID_TYPE);
    cpl_test_zero( cpl_imagelist_get_size(imlist2));

    error = cpl_imagelist_cast(imlist2, imlist, CPL_TYPE_FLOAT);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_zero( cpl_imagelist_is_uniform(imlist2));
    cpl_test_eq( cpl_imagelist_get_size(imlist2), NIMAGES );

    error = cpl_imagelist_cast(imlist2, NULL, CPL_TYPE_INT);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_zero( cpl_imagelist_is_uniform(imlist2));
    cpl_test_eq( cpl_imagelist_get_size(imlist2), NIMAGES );

    error = cpl_imagelist_cast(imlist2, imlist, CPL_TYPE_UNSPECIFIED);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_zero( cpl_imagelist_is_uniform(imlist2));
    cpl_test_eq( cpl_imagelist_get_size(imlist2), 2 * NIMAGES );

    /* Test a cast with a shared image */
    error = cpl_imagelist_set(imlist2, cpl_imagelist_get(imlist2, 0),
                              cpl_imagelist_get_size(imlist2));
    cpl_test_eq( cpl_imagelist_get_size(imlist2), 2 * NIMAGES + 1);

    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_imagelist_cast(imlist2, NULL, CPL_TYPE_FLOAT_COMPLEX);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_zero( cpl_imagelist_is_uniform(imlist2));
    cpl_test_eq( cpl_imagelist_get_size(imlist2), 2 * NIMAGES + 1);

    error = cpl_imagelist_cast(imlist2, NULL, CPL_TYPE_FLOAT);
    cpl_test_eq_error(error, CPL_ERROR_TYPE_MISMATCH);
    cpl_test_zero( cpl_imagelist_is_uniform(imlist2));
    cpl_test_eq( cpl_imagelist_get_size(imlist2), 2 * NIMAGES + 1);

    cpl_imagelist_delete(imlist2);

    cpl_msg_info("", "Duplicate the image list");

    imlist2 = cpl_imagelist_duplicate(imlist);
    cpl_test_nonnull( imlist2 );

    cpl_test_zero( cpl_imagelist_is_uniform(imlist2));
    cpl_test_eq( cpl_imagelist_get_size(imlist2), NIMAGES );

    /* Failure tests involving non-empty, valid list */
    error = cpl_imagelist_save(imlist2, ".",  CPL_TYPE_FLOAT, NULL,
                               CPL_IO_CREATE);
    cpl_test_eq_error( error, CPL_ERROR_FILE_IO );

    remove(file);
    error = cpl_imagelist_save(imlist2, file,  CPL_TYPE_FLOAT, NULL,
                               CPL_IO_APPEND);
    cpl_test_eq_error( error, CPL_ERROR_FILE_IO );

    error = cpl_imagelist_save(imlist2, file,  CPL_TYPE_FLOAT, NULL,
                               CPL_IO_CREATE | CPL_IO_EXTEND);
    cpl_test_eq_error( error, CPL_ERROR_ILLEGAL_INPUT );

    error = cpl_imagelist_save(imlist2, file,  CPL_TYPE_FLOAT, NULL,
                               CPL_IO_CREATE | CPL_IO_EXTEND | CPL_IO_APPEND);
    cpl_test_eq_error( error, CPL_ERROR_ILLEGAL_INPUT );

    image = cpl_image_new(IMAGESZ, IMAGESZ, CPL_TYPE_INT);
    cpl_test_nonnull( image );

    error = cpl_imagelist_set(imlist2, image, 0);
    cpl_test_eq_error( error, CPL_ERROR_TYPE_MISMATCH );
    cpl_test_zero( cpl_imagelist_is_uniform(imlist2) );
    cpl_test_eq( cpl_imagelist_get_size(imlist2), NIMAGES );

    cpl_image_delete(image);

    image = cpl_image_new(IMAGESZ/2, IMAGESZ, CPL_TYPE_INT);
    cpl_test_nonnull( image );

    error = cpl_imagelist_set(imlist2, image, 0);
    cpl_test_eq_error( error, CPL_ERROR_INCOMPATIBLE_INPUT );
    cpl_test_zero( cpl_imagelist_is_uniform(imlist2) );
    cpl_test_eq( cpl_imagelist_get_size(imlist2), NIMAGES );

    nullimg = cpl_imagelist_get(imlist2, NIMAGES+1);
    cpl_test_error( CPL_ERROR_ACCESS_OUT_OF_RANGE );
    cpl_test_null( nullimg );

    cpl_test_zero( cpl_imagelist_is_uniform(imlist2) );
    cpl_test_eq( cpl_imagelist_get_size(imlist2), NIMAGES );

    cpl_imagelist_empty(imlist2); /* Test cpl_imagelist_unset() */

    /* Imagelist with 1 image */
    error = cpl_imagelist_set(imlist2, image, 0);
    cpl_test_eq_error( error, CPL_ERROR_NONE );

    /* Must be allowed to replace it w. image of different size/type  */
    image = cpl_image_new(IMAGESZ, IMAGESZ/2, CPL_TYPE_DOUBLE);
    cpl_test_nonnull( image );

    error = cpl_imagelist_set(imlist2, image, 0);
    cpl_test_eq_error( error, CPL_ERROR_NONE );

    cpl_imagelist_delete(imlist2);

    /* Normal conditions */

    /* Create double-length list, with images inserted twice */
    imlista = cpl_imagelist_duplicate(imlist);
    for (i = 0; i < NIMAGES; i++) {
        image = cpl_imagelist_get(imlista, i);

        error = cpl_imagelist_set(imlista, image, i + NIMAGES);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        cpl_test_zero( cpl_imagelist_is_uniform(imlista) );
        cpl_test_eq( cpl_imagelist_get_size(imlista), i+1 + NIMAGES );
        cpl_test_eq_ptr( cpl_imagelist_get_const(imlista, i + NIMAGES),
                         cpl_imagelist_get_const(imlista, i)  );    
    }

    /* Test cpl_imagelist_save() */
    cpl_msg_info("", "Save the image list to %s", file);
    error = cpl_imagelist_save(imlist, file,  CPL_TYPE_FLOAT, NULL,
                               CPL_IO_CREATE);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_fits(file);
    cpl_test_zero( cpl_fits_count_extensions(file));
    next = 0;

    error = cpl_imagelist_save(imlist, file,  CPL_TYPE_FLOAT, NULL,
                               CPL_IO_EXTEND);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_fits(file);
    cpl_test_eq( cpl_fits_count_extensions(file), ++next);

    /* cpl_image_load(): Test loading of NAXIS=3 data (DFS09929) */
    image = cpl_image_load(file, CPL_TYPE_UNSPECIFIED, 0, 0);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_image_abs(image, cpl_imagelist_get_const(imlist, 0),
                       15.0 * FLT_EPSILON);
    cpl_image_delete(image);

    /* Test cpl_imagelist_load() handling of negative extension */

    nulllist = cpl_imagelist_load(file, CPL_TYPE_INT, -1);
    cpl_test_error( CPL_ERROR_ILLEGAL_INPUT );
    cpl_test_null( nulllist );

    /* Append error test */
    error = cpl_imagelist_save(imlist, file, CPL_TYPE_DOUBLE, NULL,
                               CPL_IO_APPEND);
    cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

    image = cpl_image_new(IMAGESZ+1,IMAGESZ+1, CPL_TYPE_FLOAT);
    imlist2 = cpl_imagelist_new();
    cpl_imagelist_set(imlist2, image, 0);
    error = cpl_imagelist_save(imlist2, file, CPL_TYPE_FLOAT, NULL,
                               CPL_IO_APPEND);
    cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);
    cpl_imagelist_delete(imlist2);

    /* Tests cpl_imagelist_load() for no-casting type */
    imlist2 = cpl_imagelist_load(file, CPL_TYPE_UNSPECIFIED, 0);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(imlist2);

    cpl_test_imagelist_abs(imlist, imlist2, 10.0 * FLT_EPSILON);
    cpl_imagelist_delete(imlist2);

    error = cpl_imagelist_save(imlist, file,  CPL_TYPE_FLOAT, NULL,
                               CPL_IO_APPEND);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_fits(file);
    cpl_test_eq( cpl_fits_count_extensions(file), next);

    imlist2 = cpl_imagelist_load(file, CPL_TYPE_UNSPECIFIED, 1);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(imlist2);

    cpl_test_imagelist_abs(imlista, imlist2, 10.0 * FLT_EPSILON);
    cpl_imagelist_delete(imlista);
    
    error = cpl_imagelist_erase(imlist, eraser);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_zero( cpl_imagelist_is_uniform(imlist) );
    cpl_test_eq( 2*cpl_imagelist_get_size(imlist), NIMAGES );

    cpl_imagelist_empty(imlist); /* Test cpl_imagelist_unset() */
    cpl_imagelist_delete(imlist);

    /* Tests cpl_imagelist_load() for no-casting type */
    imlist = cpl_imagelist_load(file, CPL_TYPE_UNSPECIFIED, 0);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(imlist);

    image = cpl_imagelist_get(imlist, 0);
    cpl_test_eq(cpl_image_get_type(image), CPL_TYPE_FLOAT);

    cpl_imagelist_delete(imlist);

    imlist = cpl_imagelist_load_window(file, CPL_TYPE_UNSPECIFIED, 0,
                       IMAGESZ/4, IMAGESZ/4,
                       3*IMAGESZ/4, 3*IMAGESZ/4);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(imlist);

    image = cpl_imagelist_get(imlist, 0);
    cpl_test_eq(cpl_image_get_type(image), CPL_TYPE_FLOAT);

    cpl_imagelist_delete(imlist);

    /* Test cpl_imagelist_load() */
    cpl_msg_info("", "Load image list as type DOUBLE");
    imlist = cpl_imagelist_load(file, CPL_TYPE_DOUBLE, 0);

    cpl_test_nonnull( imlist );
    cpl_test_zero( cpl_imagelist_is_uniform(imlist) );
    cpl_test_eq( cpl_imagelist_get_size(imlist), NIMAGES );

    error = cpl_imagelist_set(imlist, cpl_imagelist_get(imlist, 0), 2);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_imagelist_erase(imlist, eraser);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_zero( cpl_imagelist_is_uniform(imlist) );
    cpl_test_eq( 2*cpl_imagelist_get_size(imlist), NIMAGES );

    cpl_imagelist_empty(imlist); /* Test cpl_imagelist_unset() */
    cpl_imagelist_delete(imlist);
    
    /* Test cpl_imagelist_load() */
    cpl_msg_info("", "Load image list as type FLOAT");
    imlist = cpl_imagelist_load(file, CPL_TYPE_FLOAT, 0);

    cpl_test_nonnull( imlist );
    cpl_test_zero( cpl_imagelist_is_uniform(imlist) );
    cpl_test_eq( cpl_imagelist_get_size(imlist), NIMAGES );

    error = cpl_imagelist_erase(imlist, eraser);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_zero( cpl_imagelist_is_uniform(imlist) );
    cpl_test_eq( 2*cpl_imagelist_get_size(imlist), NIMAGES );

    cpl_imagelist_empty(imlist); /* Test cpl_imagelist_unset() */
    cpl_imagelist_delete(imlist);

    /* Test cpl_imagelist_load() */
    cpl_msg_info("", "Load image list as type INTEGER");
    imlist = cpl_imagelist_load(file, CPL_TYPE_INT, 0);

    cpl_test_nonnull( imlist );
    cpl_test_zero( cpl_imagelist_is_uniform(imlist) );
    cpl_test_eq( cpl_imagelist_get_size(imlist), NIMAGES );

    error = cpl_imagelist_erase(imlist, eraser);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_zero( cpl_imagelist_is_uniform(imlist) );
    cpl_test_eq( 2*cpl_imagelist_get_size(imlist), NIMAGES );

    cpl_imagelist_empty(imlist); /* Test cpl_imagelist_unset() */
    cpl_imagelist_delete(imlist);

    /* Test cpl_imagelist_load_window() */
    cpl_msg_info("", "Load image list as type DOUBLE from a window");
    imlist = cpl_imagelist_load_window(file, CPL_TYPE_DOUBLE, 0, IMAGESZ/4,
                                       IMAGESZ/4, 3*IMAGESZ/4, 3*IMAGESZ/4);

    cpl_test_nonnull( imlist );
    cpl_test_zero( cpl_imagelist_is_uniform(imlist) );
    cpl_test_eq( cpl_imagelist_get_size(imlist), NIMAGES );

    error = cpl_imagelist_erase(imlist, eraser);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_zero( cpl_imagelist_is_uniform(imlist) );
    cpl_test_eq( 2*cpl_imagelist_get_size(imlist), NIMAGES );

    cpl_imagelist_empty(imlist); /* Test cpl_imagelist_unset() */
    cpl_imagelist_delete(imlist);
    

    /* Test cpl_imagelist_load_window() */
    cpl_msg_info("", "Load image list as type FLOAT from a window");
    imlist = cpl_imagelist_load_window(file, CPL_TYPE_FLOAT, 0, IMAGESZ/4,
                                       IMAGESZ/4, 3*IMAGESZ/4, 3*IMAGESZ/4);

    cpl_test_nonnull( imlist );
    cpl_test_zero( cpl_imagelist_is_uniform(imlist) );
    cpl_test_eq( cpl_imagelist_get_size(imlist), NIMAGES );

    error = cpl_imagelist_erase(imlist, eraser);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_zero( cpl_imagelist_is_uniform(imlist) );
    cpl_test_eq( 2*cpl_imagelist_get_size(imlist), NIMAGES );

    cpl_imagelist_empty(imlist); /* Test cpl_imagelist_unset() */
    cpl_imagelist_delete(imlist);
    

    /* Test cpl_imagelist_load_window() */
    cpl_msg_info("", "Load image list as type INTEGER from a window");
    imlist = cpl_imagelist_load_window(file, CPL_TYPE_INT, 0, IMAGESZ/4,
                                       IMAGESZ/4, 3*IMAGESZ/4, 3*IMAGESZ/4);

    cpl_test_nonnull( imlist );
    cpl_test_zero( cpl_imagelist_is_uniform(imlist) );
    cpl_test_eq( cpl_imagelist_get_size(imlist), NIMAGES );

    error = cpl_imagelist_erase(imlist, eraser);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_zero( cpl_imagelist_is_uniform(imlist) );
    cpl_test_eq( 2*cpl_imagelist_get_size(imlist), NIMAGES );

    /* Clean up  */

    cpl_imagelist_empty(imlist); /* Test cpl_imagelist_unset() */
    cpl_imagelist_delete(imlist);

    cpl_imagelist_empty(imlist2); /* Test cpl_imagelist_unset() */
    cpl_imagelist_delete(imlist2);
    cpl_vector_unwrap(eraser);

    cpl_imagelist_save_compression_test();

    if (cpl_msg_get_level() <= CPL_MSG_INFO) {
        cpl_imagelist_save_compression_bench(256, 256, 12, 256);
    } else {
        cpl_imagelist_save_compression_bench(IMAGESZ, IMAGESZ, 32, 3);
    }

    cpl_test_zero( remove(FILENAME) );

    if (stream != stdout) cpl_test_zero( fclose(stream) );

    /* End of tests */
    return cpl_test_end(0);
}


/*----------------------------------------------------------------------------*/
/**
  @brief    Test saving with compression
  @return   void
  @see cpl_imagelist_save()
 */
/*----------------------------------------------------------------------------*/
static void cpl_imagelist_save_compression_test(void)
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

    for (ityp = 0; ityp < ntyp; ityp++) 
    {
        const cpl_type bpps[] = {CPL_TYPE_UCHAR, CPL_TYPE_SHORT,
                                 CPL_TYPE_USHORT, CPL_TYPE_INT,
                                 CPL_TYPE_FLOAT, CPL_TYPE_DOUBLE,
                                 CPL_TYPE_UNSPECIFIED};
        const size_t   nbpp = sizeof(bpps)/sizeof(bpps[0]);
        size_t         ibpp;

        const cpl_type imtype = imtypes[ityp];
        cpl_image * img = cpl_image_new(IMAGESZ, IMAGESZ, imtype);
        cpl_imagelist * imglist = cpl_imagelist_new();
        cpl_error_code error;


        error = cpl_image_fill_noise_uniform(img, 0.0, maxval);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_imagelist_set(imglist, img, 0);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        for (ibpp = 0; ibpp < nbpp; ibpp++)
        {
            const cpl_io_type comp_meths[] = {CPL_IO_COMPRESS_GZIP, 
                                              CPL_IO_COMPRESS_RICE, 
                                              CPL_IO_COMPRESS_HCOMPRESS,
                                              CPL_IO_COMPRESS_PLIO};
            const size_t ncomp = sizeof(comp_meths)/sizeof(comp_meths[0]);
            size_t icomp;
            const int bitpix =
                cpl_tools_get_bpp(bpps[ibpp] == CPL_TYPE_UNSPECIFIED
                                  ? imtype : bpps[ibpp]);

            int ext = 0;

            error = cpl_imagelist_save(imglist, FILENAME, bpps[ibpp], NULL,
                                       CPL_IO_CREATE);            
            cpl_test_eq_error(error, CPL_ERROR_NONE);

            /* Tests with compression */
            for(icomp = 0; icomp < ncomp; icomp++)
            {
                const cpl_io_type comp_method = comp_meths[icomp];

                /* The compression method flag must be non-zero */
                cpl_test(comp_method);

                /* Saving with compression in non supported combinations */
                error = cpl_imagelist_save(NULL, FILENAME, bpps[ibpp], NULL,
                                           CPL_IO_EXTEND | comp_method);
                cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

                /* Compression is only supported when adding new extensions, 
                 * not creating a new file and saving data in the main header */
                error = cpl_imagelist_save(imglist, FILENAME, bpps[ibpp], NULL,
                                           CPL_IO_CREATE | comp_method);
                cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

                error = cpl_imagelist_save(imglist, FILENAME, bpps[ibpp], NULL,
                                           CPL_IO_APPEND | comp_method);
                cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

                for(size_t icomp2 = 0; icomp2 < icomp; icomp2++) {
                    const cpl_io_type comp_method2 = comp_meths[icomp2];

                    error = cpl_imagelist_save(imglist, FILENAME, bpps[ibpp], NULL,
                                           CPL_IO_EXTEND | comp_method
                                           | comp_method2);
                    cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

                    error = cpl_imagelist_save(imglist, FILENAME, bpps[ibpp], NULL,
                                           CPL_IO_APPEND | comp_method
                                           | comp_method2);
                    cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

                    for(size_t icomp3 = 0; icomp3 < icomp2; icomp3++) {
                        const cpl_io_type comp_method3 = comp_meths[icomp3];

                        error = cpl_imagelist_save(imglist, FILENAME, bpps[ibpp], NULL,
                                               CPL_IO_EXTEND | comp_method
                                               | comp_method2 | comp_method3);
                        cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

                        error = cpl_imagelist_save(imglist, FILENAME, bpps[ibpp], NULL,
                                               CPL_IO_APPEND | comp_method
                                               | comp_method2 | comp_method3);
                        cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);
                    }

                }

                error = cpl_imagelist_save(imglist, FILENAME, bpps[ibpp], NULL,
                                       CPL_IO_EXTEND | CPL_IO_COMPRESS_GZIP |
                                       CPL_IO_COMPRESS_RICE| 
                                       CPL_IO_COMPRESS_HCOMPRESS |
                                       CPL_IO_COMPRESS_PLIO);
                cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

                error = cpl_imagelist_save(imglist, FILENAME, bpps[ibpp], NULL,
                                       CPL_IO_APPEND | CPL_IO_COMPRESS_GZIP |
                                       CPL_IO_COMPRESS_RICE| 
                                       CPL_IO_COMPRESS_HCOMPRESS |
                                       CPL_IO_COMPRESS_PLIO);
                cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);


                error = cpl_imagelist_save(imglist, FILENAME, bpps[ibpp], NULL,
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

                    img_load = cpl_image_load(FILENAME, CPL_TYPE_UNSPECIFIED,
                                              0,  ext);
                    cpl_test_error(CPL_ERROR_NONE);

                    cpl_test_eq(cpl_image_get_size_x(img_load), IMAGESZ);
                    cpl_test_eq(cpl_image_get_size_y(img_load), IMAGESZ);

                    if (imtype == CPL_TYPE_INT) {
                        cpl_test_image_abs(img, img_load, 0.0);
                    } else if (cpl_image_get_type(img_load) != CPL_TYPE_INT) {
                        cpl_test_image_abs(img, img_load, prec * maxval);
                    }
                    cpl_image_delete(img_load);

                    /* Can (currently) not insert images into an existing,
                       compressed cube */
                    error = cpl_imagelist_save(imglist, FILENAME, bpps[ibpp],
                                               NULL, CPL_IO_APPEND);
                    cpl_test_eq_error(error, CPL_ERROR_UNSUPPORTED_MODE);
                }
            }
        }
        cpl_imagelist_delete(imglist);
    }
}                                               

/*----------------------------------------------------------------------------*/
/**
  @brief    Benchmark saving with compression
  @param  nx NX
  @param  ny NY
  @param  nz NZ
  @param  nr Number of repetitions
  @return   void
  @see cpl_image_save()
 */
/*----------------------------------------------------------------------------*/
static void cpl_imagelist_save_compression_bench(cpl_size nx, cpl_size ny,
                                                 cpl_size nz, cpl_size nr)
{

    cpl_image * img = cpl_image_new(nx, ny, CPL_TYPE_INT);
    cpl_imagelist * imglist = cpl_imagelist_new();
    cpl_error_code error;
    cpl_size i;
    double tsum = 0.0;

    error = cpl_image_fill_noise_uniform(img, 0.0, 255.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    for (i = 0; i < nz; i++) {
        error = cpl_imagelist_set(imglist, img, i);
        cpl_test_eq_error(error, CPL_ERROR_NONE);
    }

    error = cpl_image_save(NULL, FILENAME, CPL_TYPE_UNSPECIFIED, NULL,
                           CPL_IO_CREATE);            
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_imagelist_save(imglist, FILENAME, CPL_TYPE_UNSPECIFIED,
                               NULL, CPL_IO_EXTEND | CPL_IO_COMPRESS_RICE);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_fits(FILENAME);
    cpl_test_eq( cpl_fits_count_extensions(FILENAME), 1);

#ifdef _OPENMP
#pragma omp parallel for private(i) reduction(+ : tsum)
#endif
    for (i = 0; i < nr; i++) {
        const double t0 = cpl_test_get_walltime();
        cpl_imagelist * imgload = cpl_imagelist_load(FILENAME,
                                                     CPL_TYPE_UNSPECIFIED, 1);
        const double t1 = cpl_test_get_walltime() - t0;
        int j;

        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_eq(cpl_imagelist_get_size(imgload), nz);

        tsum += t1;

        for (j = 0; j < nz; j++) {
            cpl_test_image_abs(cpl_imagelist_get_const(imgload, j), img, 0.0);
        }

        cpl_imagelist_delete(imgload);
    }
    cpl_msg_info(cpl_func, "Time to load Rice-compressed image list %d X %d "
                 "X %d %d times [s]: %g", (int)nx, (int)ny, (int)nz, (int)nr,
                 tsum);

    cpl_imagelist_unwrap(imglist);
    cpl_image_delete(img);
}

