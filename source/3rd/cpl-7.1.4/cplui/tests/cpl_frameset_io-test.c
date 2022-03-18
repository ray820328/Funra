/*
 *   This file is part of the ESO Common Pipeline Library
 *   Copyright (C) 2001-2017 European Southern Observatory
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#undef CX_DISABLE_ASSERT
#undef CX_LOG_DOMAIN

#include "cpl_test.h"
#include "cpl_fits.h"

#include <stdio.h>
#include <string.h>

#include <cxmemory.h>
#include <cxmessages.h>

#include <cpl_msg.h>
#include <cpl_memory.h>

#include "cpl_frameset_io.h"
#include "cpl_image_io.h"

#ifndef IMAGESZ
#define IMAGESZ 512
#endif

int main(void)
{

    const cxchar *names[] = {
        "cpl_frameset-io_test1.fits",
        "cpl_frameset-io_test2.fits",
        "cpl_frameset-io_test3.fits",
        "cpl_frameset-io_test4.fits",
    };

    const cxchar *tags[] = {
        "FLAT",
        "FLAT",
        "FLAT",
        "FLAT",
    };

    cpl_frame_group groups[] = {
        CPL_FRAME_GROUP_RAW,
        CPL_FRAME_GROUP_RAW,
        CPL_FRAME_GROUP_RAW,
        CPL_FRAME_GROUP_RAW,
    };

    cpl_frame_level levels[] = {
        CPL_FRAME_LEVEL_NONE,
        CPL_FRAME_LEVEL_NONE,
        CPL_FRAME_LEVEL_NONE,
        CPL_FRAME_LEVEL_NONE,
    };

    cxlong i;
    cxlong has_primary;
    cpl_frameset *frameset;
    cpl_image       *img1;
    cpl_imagelist *imlist;
    cpl_error_code error;


    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    /* Insert tests below */

    /*
     * Test 1A: Create a frameset and check its validity.
     */

    frameset = cpl_frameset_new();

    cpl_test_nonnull(frameset);
    cpl_test(cpl_frameset_is_empty(frameset));
    cpl_test_zero(cpl_frameset_get_size(frameset));


    /*
     * Test 1B: Load everything from an empty frameset
     *           - should fail, since the created imagelist may not be empty
     */

    imlist = cpl_imagelist_load_frameset(frameset, CPL_TYPE_FLOAT, 0, -1);
    
    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(imlist);

    /* 
     * Test 2A: Test error-handling of cpl_imagelist_load_frameset()
    */
    
    imlist = cpl_imagelist_load_frameset(NULL, CPL_TYPE_FLOAT, 0, -1);

    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(imlist);


     /* Add frames to the frame set created in the previous test
         and verify the data.*/
     /* The corresponding files do not exist yet */


    for (i = 0; (cxsize)i < CX_N_ELEMENTS(names); i++) {

        cpl_frame *_frame = cpl_frame_new();

        remove(names[i]);

        error = cpl_frame_set_filename(_frame, names[i]);
        cpl_test_eq_error(error, CPL_ERROR_NONE);
        error = cpl_frame_set_tag(_frame, tags[i]);
        cpl_test_eq_error(error, CPL_ERROR_NONE);
        error = cpl_frame_set_type(_frame, CPL_FRAME_TYPE_IMAGE);
        cpl_test_eq_error(error, CPL_ERROR_NONE);
        error = cpl_frame_set_group(_frame, groups[i]);
        cpl_test_eq_error(error, CPL_ERROR_NONE);
        error = cpl_frame_set_level(_frame, levels[i]);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_frameset_insert(frameset, _frame);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

    }

    cpl_test_zero(cpl_frameset_is_empty(frameset));
    cpl_test_eq(cpl_frameset_get_size(frameset), CX_N_ELEMENTS(names));
    cpl_test_eq(cpl_frameset_count_tags(frameset, tags[0]), 4);


    /* 
     * Test 2B: Test error-handling of cpl_imagelist_load_frameset()
                with missing files.
    */
    
    imlist = cpl_imagelist_load_frameset(frameset, CPL_TYPE_FLOAT, 0, -1);

    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(imlist);

    img1 = cpl_image_fill_test_create(IMAGESZ, IMAGESZ);
    cpl_test_nonnull(img1);
    
    /* 
     * Test 2BB: Test error-handling of cpl_imagelist_load_frameset()
                 on frame with a single image in primary HDU and more than
                 one image in 1st extension.
    */

    /* Create file, with image */
    error = cpl_image_save(img1, names[0], CPL_TYPE_FLOAT, NULL,
                             CPL_IO_CREATE);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    imlist = cpl_imagelist_new();
    error = cpl_imagelist_set(imlist, cpl_image_duplicate(img1), 0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_imagelist_set(imlist, cpl_image_duplicate(img1), 1);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_imagelist_save(imlist, names[0], CPL_TYPE_FLOAT, NULL,
                                 CPL_IO_EXTEND);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_imagelist_delete(imlist);
    
    /* Replicate the file */
    for (i = 1; (cxsize)i < CX_N_ELEMENTS(names); i++) {

        char * cmd = cpl_sprintf("cp %s %s", names[0], names[i]);

        cpl_test_zero(system(cmd));
        cpl_free(cmd);
    }

    imlist = cpl_imagelist_load_frameset(frameset, CPL_TYPE_FLOAT, 2, -1);

    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(imlist);


    /* 
    *         Create images to insert into frameset 
    *         First case: primary HDU image is NULL
    */
    
    for (has_primary = 0; has_primary <= 1; has_primary++) {
        const cxsize isave = 1;
        cxlong ii;

        /* The fits file contains three or four images */
        const cxlong nimg = 3 + has_primary;
        cxlong iplane;
    
        cpl_msg_info(cpl_func, "Testing with %u frames and %d X %d-images, "
                     "has_primary=%d", (unsigned)CX_N_ELEMENTS(names), IMAGESZ,
                     IMAGESZ, (int)has_primary);

        /* Replicate the file */
        for (ii = 0; (cxsize)ii < isave; ii++) {

            /* Create file, either with or without image */
            error = cpl_image_save(has_primary ? img1 : NULL, names[ii],
                                   CPL_TYPE_FLOAT, NULL, CPL_IO_CREATE);
            cpl_test_eq_error(error, CPL_ERROR_NONE);

            /* Append some extensions */

            for (i = has_primary; i < nimg; i++) {

                /* Append an extension */
                error = cpl_image_save(img1, names[ii], CPL_TYPE_FLOAT,
                                       NULL, CPL_IO_EXTEND);
                cpl_test_eq_error(error, CPL_ERROR_NONE);

            }
        }
        for (i = isave; (cxsize)i < CX_N_ELEMENTS(names); i++) {
 
            char * cmd = cpl_sprintf("cp %s %s", names[0], names[i]);
 
            cpl_test_zero(system(cmd));
            cpl_free(cmd);
        }
        if (has_primary) {
            /* If the above cp'ed FITS files are cached by CPL,
               then the cache is dirty, so refresh it */
            if (cpl_fits_get_mode() == CPL_FITS_START_CACHING)
                cpl_fits_set_mode(CPL_FITS_RESTART_CACHING);
        }
        /* 
         * Test 2D: Test error-handling of cpl_imagelist_load_frameset()
         with invalid type.
        */
    
        imlist = cpl_imagelist_load_frameset(frameset, CPL_TYPE_INVALID, 0, -1);

        cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
        cpl_test_null(imlist);

        /* 
         * Test 2E: Test error-handling of cpl_imagelist_load_frameset()
         with invalid plane number.
        */
    
        imlist = cpl_imagelist_load_frameset(frameset, CPL_TYPE_FLOAT, 2, -1);

        cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
        cpl_test_null(imlist);

        /* 
         * Test 2F: Test error-handling of cpl_imagelist_load_frameset()
         with invalid extension number.
        */
    
        imlist = cpl_imagelist_load_frameset(frameset, CPL_TYPE_FLOAT, 0, 1+nimg);

        cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
        cpl_test_null(imlist);

        /* 
         * Test 3: Load frameset into imagelist, asking for all (1) plane(s)
         *         in one extension.
         */

        for (iplane = 0; iplane <= 1; iplane++) {
            for (i = 0; i < nimg; i++) {
                imlist = cpl_imagelist_load_frameset(frameset, CPL_TYPE_FLOAT,
                                                     iplane, i+1-has_primary);
    
                cpl_test_error(CPL_ERROR_NONE);
                cpl_test_nonnull(imlist);
                cpl_test_eq(cpl_imagelist_get_size(imlist), cpl_frameset_get_size(frameset));
                cpl_imagelist_delete(imlist);
            }
        }

        /* 
         * Test 4: Load frameset into imagelist, asking for all (1) plane(s) in
                   all extensions.
         */

        for (iplane = 0; iplane <= 1; iplane++) {
            imlist = cpl_imagelist_load_frameset(frameset, CPL_TYPE_FLOAT, iplane, -1);
    
            cpl_test_error(CPL_ERROR_NONE);
            cpl_test_nonnull(imlist);
            cpl_test_eq(cpl_imagelist_get_size(imlist), nimg * cpl_frameset_get_size(frameset));

            cpl_imagelist_delete(imlist);
        }
    }

    cpl_image_delete(img1);
    cpl_frameset_delete(frameset);

    if (!cpl_error_get_code()) {
        for (i = 0; (cxsize)i < CX_N_ELEMENTS(names); i++) {
            cpl_test_zero(remove(names[i]));
        }
    }
     
    return cpl_test_end(0);
}
