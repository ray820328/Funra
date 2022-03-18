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

#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#include <fitsio.h>


#include "cpl_test.h"
#include "cpl_frame.h"
#include "cpl_frame_impl.h"
#include "cpl_error.h"


int main(void)
{

    int i;
    int next;
    cpl_size np;
    int status = 0;
    const char *filename  = "cplframetest1.fits";
    const char *filename1 = "cplframeplane.fits";
    const char *filename2 = "cplframedump.txt";
    const long naxes[2] = {256, 256};
    const long nplanes[3] = {128, 256, 64};
    cpl_frame *frame, *_frame;
    fitsfile *file = NULL;
    cpl_error_code error;
    FILE * out;

    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    /*
     * Test 1: Create a frame, check its validity and destroy it
     *         again.
     */

    frame = cpl_frame_new();
    cpl_test_nonnull(frame);

    cpl_test_null(cpl_frame_get_filename(frame) );
    cpl_test_error(CPL_ERROR_DATA_NOT_FOUND);
    cpl_test_null(cpl_frame_get_tag(frame) );
    cpl_test_eq(cpl_frame_get_type(frame), CPL_FRAME_TYPE_NONE);
    cpl_test_eq(cpl_frame_get_group(frame), CPL_FRAME_GROUP_NONE);
    cpl_test_eq(cpl_frame_get_level(frame), CPL_FRAME_LEVEL_NONE);
    cpl_test_error(CPL_ERROR_NONE);

    cpl_frame_delete(frame);


    /*
     * Test 2: Create a frame, set its fields and verify the settings.
     */

    frame = cpl_frame_new();
    cpl_test_nonnull(frame);

    error = cpl_frame_set_filename(frame, "path_to_file");
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_eq_string(cpl_frame_get_filename(frame), "path_to_file");

    error = cpl_frame_set_tag(frame, "This is a tag");
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_eq_string(cpl_frame_get_tag(frame), "This is a tag");

    error = cpl_frame_set_type(frame, CPL_FRAME_TYPE_MATRIX);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_eq(cpl_frame_get_type(frame), CPL_FRAME_TYPE_MATRIX);

    error = cpl_frame_set_group(frame, CPL_FRAME_GROUP_PRODUCT);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_eq(cpl_frame_get_group(frame), CPL_FRAME_GROUP_PRODUCT);

    error = cpl_frame_set_level(frame, CPL_FRAME_LEVEL_TEMPORARY);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_eq(cpl_frame_get_level(frame), CPL_FRAME_LEVEL_TEMPORARY);


    /*
     * Test 3: Create a copy of the frame and verify that original
     *         and copy are identical but do not share any resources.
     */

    _frame = cpl_frame_duplicate(frame);

    cpl_test_nonnull(_frame);
    cpl_test(_frame != frame);

    cpl_test(cpl_frame_get_filename(frame) != cpl_frame_get_filename(_frame));
    cpl_test(cpl_frame_get_tag(frame)      != cpl_frame_get_tag(_frame));

    cpl_test_eq_string(cpl_frame_get_filename(frame),
                       cpl_frame_get_filename(_frame));

    cpl_test_eq_string(cpl_frame_get_tag(frame),
                       cpl_frame_get_tag(_frame));

    cpl_test_eq(cpl_frame_get_type(frame), cpl_frame_get_type(_frame));
    cpl_test_eq(cpl_frame_get_group(frame), cpl_frame_get_group(_frame));
    cpl_test_eq(cpl_frame_get_level(frame), cpl_frame_get_level(_frame));

    cpl_frame_delete(_frame);
    cpl_frame_delete(frame);

    (void)remove(filename);
    status = 0;

    fits_create_diskfile(&file, filename, &status);
    cpl_test_zero(status);

    /*
    * Create first HDU
    */
    fits_create_img(file, 8, 0, (long*)naxes, &status);
    cpl_test_zero(status);

    /*
    * Create 3 extensions
    */
    for(i = 0; i < 3; i++) {
        fits_insert_img(file, 16, 2, (long*)naxes, &status);
        cpl_test_zero(status);
    }
    fits_close_file(file, &status);
    cpl_test_zero(status);

    frame = cpl_frame_new();
    cpl_test_nonnull(frame);

    error = cpl_frame_set_filename(frame, filename);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_eq_string(cpl_frame_get_filename(frame), filename);

    next = cpl_frame_get_nextensions(frame);
    cpl_test_error(CPL_ERROR_NONE);

    cpl_test_eq(next, 3);

    cpl_frame_delete(frame);

    /*
     *
     * Test 3: Query the number of planes
     */


    (void)remove(filename1);
    status = 0;

    fits_create_diskfile(&file, filename1, &status);
    cpl_test_zero(status);

    /*
    * Create first HDU
    */
    fits_create_img(file, 8, 0, (long*)nplanes, &status);
    cpl_test_zero(status);

    fits_insert_img(file, 8, 3, (long*)nplanes, &status);
    cpl_test_zero(status);

    fits_close_file(file, &status);
    cpl_test_zero(status);

    frame = cpl_frame_new();
    cpl_test_nonnull(frame);

    error = cpl_frame_set_filename(frame, filename1);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_eq_string(cpl_frame_get_filename(frame), filename1);
    np = cpl_frame_get_nplanes(frame, 1);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_eq(np, 64);

    /*
     * Test 4: Dump the frame in a file in disk
     */
    error = cpl_frame_set_tag(frame, "CALIB");
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_frame_set_type(frame, CPL_FRAME_TYPE_IMAGE);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_frame_set_group(frame, CPL_FRAME_GROUP_PRODUCT);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_frame_set_level(frame, CPL_FRAME_LEVEL_TEMPORARY);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    out = fopen(filename2, "w");
    cpl_test_nonnull(out);
    cpl_frame_dump(frame, out);
    cpl_test_zero(fclose(out));

    cpl_frame_delete(frame);

    /*
     * All tests finished
     */

    return cpl_test_end(0);
}
