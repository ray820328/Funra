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

#include "cpl_image_iqe.h"
#include "cpl_image_io.h"
#include "cpl_test.h"
#include "cpl_memory.h"

/*-----------------------------------------------------------------------------
                                   Defines
 -----------------------------------------------------------------------------*/
#ifndef IMAGESZ
#define IMAGESZ            512
#endif

#ifndef IMAGEMIN
#define IMAGEMIN 185
#endif

#ifndef IMAGEMAX
#define IMAGEMAX 225
#endif


/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/
int main(void)
{
    cpl_image        * imf;
    cpl_image        * imd;
    cpl_bivector     * res;
    FILE             * stream;


    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    /* Insert tests below */

    stream = cpl_msg_get_level() > CPL_MSG_INFO
        ? fopen("/dev/null", "a") : stdout;

    cpl_test_nonnull( stream );

    /* Create the images */
    imd = cpl_image_fill_test_create(IMAGESZ, IMAGESZ);
    cpl_test_nonnull(imd);
    imf = cpl_image_cast(imd, CPL_TYPE_FLOAT);
    cpl_test_nonnull(imf);

    /* Check error handling */

    res = cpl_image_iqe(NULL, IMAGEMIN, IMAGEMIN, IMAGEMAX, IMAGEMAX);

    cpl_test_error( CPL_ERROR_NULL_INPUT );
    cpl_test_null(res);

    res = cpl_image_iqe(imd, IMAGEMIN, IMAGEMIN, IMAGEMAX, IMAGEMAX);

    cpl_test_error( CPL_ERROR_INVALID_TYPE );
    cpl_test_null(res);

    res = cpl_image_iqe(imf, IMAGESZ, IMAGESZ, IMAGESZ+1, IMAGESZ+1);

    cpl_test_error( CPL_ERROR_ILLEGAL_INPUT );
    cpl_test_null(res);

    res = cpl_image_iqe(imf, IMAGEMIN, IMAGEMIN, IMAGEMIN+2, IMAGEMAX);
    cpl_test_error( CPL_ERROR_ILLEGAL_INPUT );
    cpl_test_null(res);

    res = cpl_image_iqe(imf, IMAGEMIN, IMAGEMIN, IMAGEMAX, IMAGEMIN+2);
    cpl_test_error( CPL_ERROR_ILLEGAL_INPUT );
    cpl_test_null(res);

    /* Compute the IQE */
    res = cpl_image_iqe(imf, IMAGEMIN, IMAGEMIN, IMAGEMAX, IMAGEMAX);

    cpl_test_error( CPL_ERROR_NONE );
    cpl_test_nonnull(res);
    cpl_test_eq( cpl_bivector_get_size(res), 7);

    cpl_bivector_dump(res, stream);

    cpl_bivector_delete(res);

    cpl_image_delete(imd); 
    cpl_image_delete(imf);
    
    /*
     * Regression test for a copy and paste bug found in line cpl_image_iqe.c:745.
     */
    imf = cpl_image_new(200, 64, CPL_TYPE_FLOAT);
    cpl_test_nonnull(imf);
    cpl_image_fill_noise_uniform(imf, -10, 10);
    cpl_test_error( CPL_ERROR_NONE );
    imd = cpl_image_duplicate(imf);
    cpl_test_nonnull(imd);
    cpl_image_fill_gaussian(imd, 168, 32, 50000.0, 15, 4);
    cpl_test_error( CPL_ERROR_NONE );
    cpl_image_add(imf, imd);
    cpl_test_error( CPL_ERROR_NONE );
    res = cpl_image_iqe(imf, 1, 1, cpl_image_get_size_x(imf),
                        cpl_image_get_size_y(imf));
    cpl_test_error( CPL_ERROR_NONE );
    cpl_test_nonnull(res);
    cpl_bivector_delete(res);
    cpl_image_delete(imd);
    cpl_image_delete(imf);

    if (stream != stdout) cpl_test_zero( fclose(stream) );

    return cpl_test_end(0);
}

/**@}*/

