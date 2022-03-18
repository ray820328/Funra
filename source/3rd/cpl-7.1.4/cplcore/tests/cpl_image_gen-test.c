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

#include "cpl_memory.h"
#include "cpl_image_gen.h"
#include "cpl_image_io.h"
#include "cpl_test.h"

/*-----------------------------------------------------------------------------
                                  Define
 -----------------------------------------------------------------------------*/

#define         IMAGESZ     512

/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/
int main(void)
{
    cpl_image * img;
    double      cputime;


    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    /* Insert tests below */

    /* Test cpl_image_fill_noise_uniform() for double image */
    img = cpl_image_new(IMAGESZ, IMAGESZ, CPL_TYPE_DOUBLE) ;
    cputime = cpl_test_get_cputime();
    cpl_test_zero(cpl_image_fill_noise_uniform(img, -100, 100));

    cpl_msg_info("","Time to generate DOUBLE noise %dx%d image:      %g", 
                 IMAGESZ, IMAGESZ, cputime - cpl_test_get_cputime());
    cpl_image_delete(img);


    /* Test cpl_image_fill_noise_uniform() for float image */
    img = cpl_image_new(IMAGESZ, IMAGESZ, CPL_TYPE_FLOAT) ;
    cputime = cpl_test_get_cputime();
    cpl_test_zero(cpl_image_fill_noise_uniform(img, -100, 100));

    cpl_msg_info("","Time to generate FLOAT noise %dx%d image:       %g", 
                 IMAGESZ, IMAGESZ, cputime - cpl_test_get_cputime());
    cpl_image_delete(img) ;


    /* Test cpl_image_fill_gaussian() for double image */
    img = cpl_image_new(IMAGESZ, IMAGESZ, CPL_TYPE_DOUBLE) ;
    cputime = cpl_test_get_cputime();
    cpl_test_zero(cpl_image_fill_gaussian(img, IMAGESZ/4, IMAGESZ/4, 1.0, 100, 
                                          100));
    cpl_msg_info("","Time to generate DOUBLE gaussian %dx%d image:   %g", 
                 IMAGESZ/4, IMAGESZ/4, cputime - cpl_test_get_cputime()) ;
    cpl_image_delete(img) ;


    /* Test cpl_image_fill_gaussian() for float image */
    img = cpl_image_new(IMAGESZ, IMAGESZ, CPL_TYPE_FLOAT) ;
    cputime = cpl_test_get_cputime();
    cpl_test_zero(cpl_image_fill_gaussian(img, 3*IMAGESZ/4, 3*IMAGESZ/4,
                                          1.0, 100,  100));

    cpl_msg_info("","Time to generate FLOAT gaussian %dx%d image:    %g", 
                 3*IMAGESZ/4, 3*IMAGESZ/4, cputime - cpl_test_get_cputime()) ;
    cpl_image_delete(img) ;


    /* Test cpl_image_fill_test_create() */
    cputime = cpl_test_get_cputime();
    img = cpl_image_fill_test_create(IMAGESZ, IMAGESZ);
    cpl_test_nonnull(img);

    cpl_msg_info("","Time to generate the standard %dx%d test image: %g", 
                 IMAGESZ, IMAGESZ, cputime - cpl_test_get_cputime()) ;
    cpl_image_delete(img) ;
    

    return cpl_test_end(0);
}
