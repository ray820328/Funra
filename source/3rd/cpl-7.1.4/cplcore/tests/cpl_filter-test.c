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

/*-----------------------------------------------------------------------------
                                Includes
 -----------------------------------------------------------------------------*/

#include <cpl_image_filter_impl.h>

#include <cpl_tools.h>
#include <cpl_memory.h>
#include <cpl_test.h>

/* sqrt() */
#include <math.h>
#include <assert.h>

/* memcpy() */
#include <string.h>

/*-----------------------------------------------------------------------------
                                Defines
 -----------------------------------------------------------------------------*/

#ifndef BENCHSIZE
#define BENCHSIZE 32
#endif

#define assure(x) assert(x)

#ifdef bool
#define mybool bool
#else
#define mybool unsigned char
#endif

#ifdef true
#define mytrue true
#else
#define mytrue 1
#endif

#ifdef false
#define myfalse false
#else
#define myfalse 0
#endif

/*-----------------------------------------------------------------------------
                                Private function prototypes
 -----------------------------------------------------------------------------*/

static void test_filter(void);

static void benchmark(cpl_filter_mode filter,
                      cpl_border_mode border_mode, unsigned);

static double rand_gauss(void);
static void test_cpl_image_filter_double(unsigned, unsigned, unsigned,
                                         unsigned, unsigned, unsigned,
                                         unsigned, unsigned);
static void test_cpl_image_filter_float(unsigned, unsigned, unsigned,
                                        unsigned, unsigned, unsigned,
                                        unsigned, unsigned);
static void test_cpl_image_filter_int(unsigned, unsigned, unsigned,
                                      unsigned, unsigned, unsigned,
                                      unsigned, unsigned);

static void test_cpl_image_filter(int, int,
                                  int, int, 
                                  cpl_filter_mode,
                                  cpl_border_mode);

#ifdef CPL_FILTER_TEST_AVERAGE_FAST

static void
image_filter_average_ref_double(double *, const double *,
                                int, int, int,
                                int, unsigned);


static void
image_filter_average_ref_float(float *, const float *,
                               int, int, int,
                               int, unsigned);

static void
image_filter_average_ref_int(int *, const int *,
                             int, int, int,
                             int, unsigned);

#elif 1

static void
image_filter_average_bf_double(double *, const double *,
                                  int, int, int,
                                  int, unsigned);

static void
image_filter_average_bf_float(float *, const float *,
                                  int, int, int,
                                  int, unsigned);

static void
image_filter_average_bf_int(int *, const int *,
                                  int, int, int,
                                  int, unsigned);

#else
static cpl_error_code filter_average_bf(cpl_image *, const cpl_image *,
                                        unsigned, unsigned, unsigned);

#endif

/*----------------------------------------------------------------------------*/
/**
 * @defgroup cpl_filter_test Testing of the CPL filter functions
 */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/**
   @brief   Test of cpl_image_filter
**/
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/**
   @brief   Unit tests of filter module
**/
/*----------------------------------------------------------------------------*/
int main(void)
{
    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    test_filter();

    return cpl_test_end(0);
}


/*----------------------------------------------------------------------------*/
/**
   @brief   Unit tests of filter module
**/
/*----------------------------------------------------------------------------*/
static void test_filter(void)
{
    const cpl_filter_mode filter_mode[] = {CPL_FILTER_AVERAGE, 
                                           CPL_FILTER_AVERAGE_FAST, 
                                           CPL_FILTER_MEDIAN};

    const cpl_border_mode border_mode[] = {CPL_BORDER_NOP,
                                           CPL_BORDER_FILTER,
                                           CPL_BORDER_CROP,
                                           CPL_BORDER_COPY};
    const int dopix[] = {1, 2, 3, 4, 7, 8};
    const int dor[] = {0, 1, 2, 3, 7};

    unsigned ifilt;
    
    for (ifilt = 0; 
         ifilt < sizeof(filter_mode)/sizeof(filter_mode[0]); 
         ifilt++) {

        unsigned ibord;
        
        for (ibord = 0; 
             ibord < sizeof(border_mode)/sizeof(border_mode[0]); 
             ibord++) {

            unsigned ix;

            if ((filter_mode[ifilt] == CPL_FILTER_STDEV ||
                 filter_mode[ifilt] == CPL_FILTER_AVERAGE ||
                 filter_mode[ifilt] == CPL_FILTER_AVERAGE_FAST) &&
                border_mode[ibord] != CPL_BORDER_FILTER) continue;
                        
            for (ix = 0; ix < sizeof(dopix)/sizeof(dopix[0]); ix++) {
                const int nx = dopix[ix];
                unsigned iy;            
                
                for (iy = 0; iy < sizeof(dopix)/sizeof(dopix[0]); iy++) {
                    const int ny = dopix[iy];
                    unsigned irx;

                    for (irx = 0; irx < sizeof(dor)/sizeof(dor[0]); irx++) {
                        const int rx = 
                            2*dor[irx] + 1 >= nx ? (nx-1)/2 : dor[irx];
                        unsigned iry;
                        
                        for (iry = 0; iry < sizeof(dor)/sizeof(dor[0]); iry++) {
                            const int ry =
                                2*dor[iry] + 1 >= ny ? (ny-1)/2 : dor[iry];
                            
                            if (2*rx + 1 <= nx &&
                                2*ry + 1 <= ny) {
                                test_cpl_image_filter(nx, ny, rx, ry,
                                                         filter_mode[ifilt],
                                                         border_mode[ibord]);
                            }
                        }
                    }
                }
            }
            benchmark(filter_mode[ifilt], border_mode[ibord], BENCHSIZE);
        }
    }
}

static void test_cpl_image_filter(int nx, int ny,
                                     int rx, int ry, 
                                     cpl_filter_mode filter,
                                     cpl_border_mode border_mode)
{
    cpl_msg_debug(cpl_func,
                  "Testing %dx%d image, "
                  "%dx%d kernel, filter = %d, border = %d",
                  nx, ny, rx, ry, 
                  filter, border_mode);
    
    /* Failure runs */
    {
        cpl_image * in  = cpl_image_new(nx, ny, CPL_TYPE_FLOAT);
        cpl_image * out = cpl_image_new(nx, ny, CPL_TYPE_FLOAT);
        cpl_mask * mask = cpl_mask_new(1 + 2 * rx, 1 + 2 * ry);
        cpl_error_code error;
     
        cpl_test_nonnull(in);
        cpl_test_nonnull(out);
        cpl_test_nonnull(mask);
        
        error = cpl_image_filter_mask(NULL, in, mask, filter, border_mode);
        cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);


        error = cpl_image_filter_mask(in, NULL, mask, filter, border_mode);
        cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

        error = cpl_image_filter_mask(in, out, NULL, filter, border_mode);
        cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);
        
        cpl_image_delete(in);
        cpl_image_delete(out);
        cpl_mask_delete(mask);
    }
    
    /* Successful runs */
    test_cpl_image_filter_double(nx, ny, rx, ry, filter, border_mode, 1, 1);
    test_cpl_image_filter_float(nx, ny, rx, ry, filter, border_mode, 1, 1);
    test_cpl_image_filter_int(nx, ny, rx, ry, filter, border_mode, 1, 1);

    return;
}

static
void benchmark(cpl_filter_mode filter,
               cpl_border_mode border_mode, unsigned benchsize)
{
    test_cpl_image_filter_float(benchsize, benchsize, 1, 1,
                                                 filter, border_mode, 100, 3);
    test_cpl_image_filter_double(benchsize, benchsize, 1, 1,
                                                  filter, border_mode, 100, 3);
    test_cpl_image_filter_int(benchsize, benchsize, 1, 1,
                                               filter, border_mode, 100, 3);
    
    test_cpl_image_filter_float(benchsize, benchsize, 2, 2,
                                              filter, border_mode, 20, 3);
    test_cpl_image_filter_double(benchsize, benchsize, 2, 2,
                                               filter, border_mode, 20, 3);
    test_cpl_image_filter_int(benchsize, benchsize, 2, 2,
                                            filter, border_mode, 20, 3);
        
    test_cpl_image_filter_float(benchsize, benchsize, 14, 14,
                                              filter, border_mode, 20, 3);
    test_cpl_image_filter_double(benchsize, benchsize, 14, 14,
                                               filter, border_mode, 20, 3);
    test_cpl_image_filter_int(benchsize, benchsize, 14, 14,
                                            filter, border_mode, 20, 3);
    return;
}            


static
double rand_gauss(void)
{
    static double V1, V2, S;
    static int phase = 0;
    double X;
    
    if(phase == 0) {
    do {
        double U1 = (double)rand() / RAND_MAX;
        double U2 = (double)rand() / RAND_MAX;
        
        V1 = 2 * U1 - 1;
        V2 = 2 * U2 - 1;
        S = V1 * V1 + V2 * V2;
    } while(S >= 1 || S == 0);
    
    X = V1 * sqrt(-2 * log(S) / S);
    } else
    X = V2 * sqrt(-2 * log(S) / S);
    
    phase = 1 - phase;
    
    return X;
}

#if 0
static cpl_error_code filter_average_bf(cpl_image * self,
                                        const cpl_image * other,
                                        unsigned hsizex, unsigned hsizey,
                                        unsigned mode)
{

    const int nx = cpl_image_get_size_x(self);
    const int ny = cpl_image_get_size_y(self);
    cpl_matrix * kernel = cpl_matrix_new(1+2*hsizex, 1+2*hsizey);
    cpl_image * copy;
    cpl_errorstate prevstate = cpl_errorstate_get();

    cpl_matrix_fill(kernel, 1.0);

    copy = cpl_image_filter_linear(other, kernel);

    if (cpl_error_get_code()) {
        cpl_errorstate_dump(prevstate, CPL_FALSE, NULL);
    }

    assert(copy != NULL);

    assert(cpl_image_get_type(copy) == cpl_image_get_type(self));
    assert(cpl_image_get_size_x(copy) == nx);
    assert(cpl_image_get_size_y(copy) == ny);

    assert((mode & ~CPL_BORDER_MODE) == CPL_FILTER_AVERAGE ||
           (mode & ~CPL_BORDER_MODE) == CPL_FILTER_AVERAGE_FAST);

    switch (cpl_image_get_type(self)) {
        case CPL_TYPE_DOUBLE: {
            const double * sour = cpl_image_get_data_double_const(copy);
            double * dest = cpl_image_get_data_double(self);

            (void)memcpy(dest, sour, nx*ny*sizeof(double));

            break;
        }

        case CPL_TYPE_FLOAT: {
            const float * sour = cpl_image_get_data_float_const(copy);
            float * dest = cpl_image_get_data_float(self);

            (void)memcpy(dest, sour, nx*ny*sizeof(float));

            break;
        }
        case CPL_TYPE_INT: {
            const int * sour = cpl_image_get_data_int_const(copy);
            int * dest = cpl_image_get_data_int(self);

            (void)memcpy(dest, sour, nx*ny*sizeof(int));

            break;
        }
        default:
            /* It is an error in CPL to reach this point */
            (void)cpl_error_set_(CPL_ERROR_UNSPECIFIED);
    }

    cpl_image_delete(copy);

    return cpl_error_get_code();

}

#endif

/* These macros are needed for support of the different pixel types */

#define CONCAT(a,b) a ## _ ## b
#define CONCAT2X(a,b) CONCAT(a,b)

#define PIXEL_TYPE double
#define PIXEL_MIN (-DBL_MAX)
#define PIXEL_MAX DBL_MAX
#include "cpl_filter_body.h"
#undef PIXEL_TYPE
#undef PIXEL_MIN
#undef PIXEL_MAX

#define PIXEL_TYPE float
#define PIXEL_MIN (-FLT_MAX)
#define PIXEL_MAX FLT_MAX
#include "cpl_filter_body.h"
#undef PIXEL_TYPE
#undef PIXEL_MIN
#undef PIXEL_MAX

#define PIXEL_TYPE_IS_INT 1
#define PIXEL_TYPE int
#define PIXEL_MIN (-INT_MAX)
#define PIXEL_MAX INT_MAX
#include "cpl_filter_body.h"
#undef PIXEL_TYPE
#undef PIXEL_TYPE_IS_INT
#undef PIXEL_MIN
#undef PIXEL_MAX
