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


#define TYPE_ADD(a) CONCAT2X(a, PIXEL_TYPE)

static void
TYPE_ADD(filter_median_bf)(const PIXEL_TYPE *in, PIXEL_TYPE *out,
                           unsigned Nx, unsigned Ny, unsigned Rx, unsigned Ry,
                           unsigned mode)
{
    PIXEL_TYPE *data = cpl_malloc((size_t)(2*Rx+1) * (size_t)(2*Ry+1)
                                  * sizeof(*data));
    if (mode == CPL_BORDER_FILTER) {
#ifdef CPL_MEDIAN_SAMPLE
        const unsigned Nx_larger = Nx + 2*Rx;
        const unsigned Ny_larger = Ny + 2*Ry;

        PIXEL_TYPE *in_larger = cpl_malloc((size_t)Nx_larger * (size_t)Ny_larger
                                           * sizeof(*in_larger));
        assure( in_larger != NULL );

        TYPE_ADD(cpl_image_filter_fill_chess)(in_larger, in, Nx_larger,
                                              Ny_larger, Nx, Ny, Rx, Ry);

        TYPE_ADD(filter_median_bf)(in_larger, out,
                                   Nx_larger, Ny_larger,
                                   Rx, Ry,
                                   CPL_BORDER_CROP);

        cpl_free(in_larger);
#else
        cpl_size y;
        for (y = 0; y < Ny; y++) {
            const size_t y_1 = CX_MAX(0,    y - Ry);
            const size_t y2  = CX_MIN(Ny-1, y + Ry);
            cpl_size x;
            for (x = 0; x < Nx; x++) {
                const size_t     x1 = CX_MAX(0,    x - Rx);
                const size_t     x2 = CX_MIN(Nx-1, x + Rx);


                const PIXEL_TYPE    * inj  = in + y_1 * Nx;
                size_t k = 0;
                size_t i, j;

                for (j = y_1; j < 1 + y2; j++, inj += Nx) {
                    /* FIXME: use memcpy() */
                    for (i = x1; i < 1 + x2; i++) {
                        data[k++] = inj[i];
                    }
                }
                assert( k <= (size_t)(2*Rx+1) * (size_t)(2*Ry+1));

                out[x + y * Nx] = TYPE_ADD(cpl_tools_get_median)(data, k);
            }
        }
#endif
    } else {
        unsigned y;
        if (mode == CPL_BORDER_CROP) {
            out += Rx;
        } else {
            if (mode == CPL_BORDER_COPY) 
                (void)memcpy(out, in, (Ry*Nx+Rx)*sizeof(*out));
            out += Ry * Nx;
        }

        for (y = 0 + Ry; y < Ny-Ry; y++, out += Nx) {
            unsigned x = Rx;

            if (mode == CPL_BORDER_CROP) {
                out -= 2*Rx; /* Nx - 2 Rx medians in each row */
            } else if (mode == CPL_BORDER_COPY) {
                if (y != Ry) {
                    (void)memcpy(out-Rx, in + y * Nx - Rx, 2*Rx*sizeof(*out));
                }
            }

            for (; x < Nx - Rx; x++) {
                unsigned k = 0;
                unsigned i, j;
                for (j = y-Ry; j <= y+Ry; j++)
                    for (i = x-Rx; i <= x+Rx; i++)
                        data[k++] = in[i + j*Nx];
            
                out[x] = TYPE_ADD(cpl_tools_get_median)(data, k);
            }

            if (mode == CPL_BORDER_COPY) {
                if (y == Ny - Ry - 1) {
                    (void)memcpy(out + Nx-Rx, 
                                 in  + Nx-Rx + y * Nx,
                                 Rx*sizeof(*out));
                }
            }
        }

        if (mode == CPL_BORDER_COPY) {
            (void)memcpy(out, in + y * Nx, Ry*Nx*sizeof(*out));
        }
    }

    cpl_free(data);
    return;
}


static void
TYPE_ADD(test_cpl_image_filter)(unsigned Nx, unsigned Ny,
                                unsigned Rx, unsigned Ry,
                                unsigned filter,
                                unsigned border_mode,
                                unsigned Nreps1,
                                unsigned Nreps2)
{

    PIXEL_TYPE *in  = cpl_malloc((size_t)Nx * (size_t)Ny * sizeof(*in));
    /* Too large in crop mode */
    PIXEL_TYPE *out = cpl_malloc((size_t)Nx * (size_t)Ny * sizeof(*out));
    /* Too large in crop mode */
    PIXEL_TYPE *ref = cpl_malloc((size_t)Nx * (size_t)Ny * sizeof(*ref));
    cpl_mask * mask = cpl_mask_new(1 + 2 * Rx, 1 + 2 * Ry);
    const double myeps
#ifdef PIXEL_TYPE_IS_INT
        = 0.0;
#else
        = filter == CPL_FILTER_MEDIAN ? 0.0
        : 1e1 * Nx * Ny * (sizeof(PIXEL_TYPE) == 4 ? FLT_EPSILON : DBL_EPSILON);
#endif
    cpl_error_code error;

    assure( Nx > 0 );
    assure( Ny > 0 );

    assure( in  != NULL );
    assure( out != NULL );
    assure( ref != NULL );
    assure( mask != NULL );

    error = cpl_mask_not(mask);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    {
        unsigned i, j;
        for (j = 0; j < Ny; j++) {
            for (i = 0; i < Nx; i++) {
                in[i + j*Nx] = (PIXEL_TYPE)(10.0 + rand_gauss()*3.0);
                /* fprintf(stderr, "(%d, %d): %f", i, j, in[i+j*Nx]); */
            }
            /* fprintf(stderr, "\n"); */
        }
    }

    {
        const unsigned Nxc = border_mode == CPL_BORDER_CROP
            ? Nx - 2 * Rx : Nx;
        const unsigned Nyc = border_mode == CPL_BORDER_CROP
            ? Ny - 2 * Ry : Ny;
        cpl_image  *imgin  = TYPE_ADD(cpl_image_wrap)(Nx, Ny, in);
        cpl_image  *imgout = TYPE_ADD(cpl_image_wrap)(Nxc, Nyc, out);
        cpl_image  *imgref = TYPE_ADD(cpl_image_wrap)(Nxc, Nyc, ref);
        
        unsigned i;
        for (i = 0; i < Nreps2; i++) {
            double t, tbf;
            unsigned j;
            
            if (border_mode == CPL_BORDER_NOP) {
                /* The border will not be set by the filtering,
                   so preset it (to zero) */
                memset(out, 0, Nxc*Nyc * sizeof(PIXEL_TYPE));
                memset(ref, 0, Nxc*Nyc * sizeof(PIXEL_TYPE));
            }

            /* binary heap + column arrays */
            error = CPL_ERROR_NONE;
            t = cpl_test_get_cputime();
            for (j = 0; j < Nreps1; j++) {
                error |= cpl_image_filter_mask(imgout, imgin, mask, filter,
                                               border_mode);

            }
            cpl_test_eq_error(error, CPL_ERROR_NONE);

            t = cpl_test_get_cputime()-t;
            if (1) {
                tbf = cpl_test_get_cputime();
                for (j = 0; j < Nreps1; j++) {
                    if (filter == CPL_FILTER_MEDIAN) {
                        TYPE_ADD(filter_median_bf)(in, ref, Nx, Ny, Rx, Ry,
                                                   border_mode);
                    } else if (filter == CPL_FILTER_STDEV) {
                        cpl_test(0); /* FIXME: Unsupported */
                    } else {
#ifdef CPL_FILTER_TEST_AVERAGE_FAST
                        TYPE_ADD(image_filter_average_ref)(ref, in, Nx, Ny,
                                                           Rx, Ry, border_mode);
#elif 1
                        TYPE_ADD(image_filter_average_bf)(ref, in, Nx, Ny,
                                                          Rx, Ry, border_mode);
#else
                        error = filter_average_bf(imgref, imgin, Rx, Ry,
                                                  border_mode);
                        cpl_test_eq_error(error, CPL_ERROR_NONE);
#endif
                    }
                }
                tbf = cpl_test_get_cputime()-tbf;
                cpl_msg_info(cpl_func, "Time to %u-filter %u X %u "
                             CPL_STRINGIFY(PIXEL_TYPE) " with %u X %u [s]: %f %f",
                             border_mode, Nx, Ny, Rx, Ry, t, tbf);            

                cpl_test_image_abs(imgout, imgref, myeps);
            }
        }
        cpl_test_eq_ptr(cpl_image_unwrap(imgin),  in);
        cpl_test_eq_ptr(cpl_image_unwrap(imgout), out);
        cpl_test_eq_ptr(cpl_image_unwrap(imgref), ref);
    }

    cpl_free(in);
    cpl_free(out);
    cpl_free(ref);
    cpl_mask_delete(mask);

}

#ifdef CPL_FILTER_TEST_AVERAGE_FAST

/*----------------------------------------------------------------------------*/
/**
   @internal
   @brief   Average filter
   @param   out          filtered image
   @param   in           raw image
   @param   nx           Number of rows on image
   @param   ny           Number of cols on image
   @param   hsizex       width of filter window is 2*radius_x + 1
   @param   hsizey       height of filter window is 2*radius_y + 1
   @return  void
   @see uves_filter_image_average()
*/
/*----------------------------------------------------------------------------*/
static void
TYPE_ADD(image_filter_average_ref)(PIXEL_TYPE * out, const PIXEL_TYPE * in,
                                   int nx, int ny, int hsizex, int hsizey,
                                   unsigned border_mode)
{

    PIXEL_TYPE * aux = calloc((nx+1)*(ny+1), sizeof(PIXEL_TYPE));
    int         i;

    assert(border_mode == CPL_BORDER_FILTER);

    /* First build auxillary image:
     *
     * aux(x,y) = sum_{i=0,x-1} sum_{j=0,y-1}  image(i,j)
     *          = sum of rectangle (0,0)-(x-1,y-1)
     *
     */


    /* Column x=0 and row y=0 are already zero and need not be calculated,
     * start from 1.    */

    for (i = 0; i < (nx+1)*(ny+1); i++)
    {
        int x = i % (nx+1);
        int y = i / (nx+1);

        if ( x >= 1 && y >= 1)
        {
            aux[x + y*(nx+1)] = (PIXEL_TYPE)in[x-1 + (y-1) * nx]
            + aux  [x-1 +    y * (nx+1)]
            + aux  [x   + (y-1)* (nx+1)]
            - aux  [x-1 + (y-1)* (nx+1)];
        }

        /* Proof of induction step
         * (assume that formula holds up to (x-1,y) and (x,y-1) and prove formula for (x,y))
         *
         *  aux(x,y) = image(x-1, y-1) + aux(x-1, y) + aux(x, y-1) - aux(x-1, y-1)  (see code)
         *
         *  = image(x-1, y-1)
         *  + sum_{i=0,x-2}_{j=0,y-1} image(i,j)  _
         *  + sum_{i=0,x-1}_{j=0,y-2} image(i,j)   \_ sum_{j=0,y-2} image(x-1, j)
         *  - sum_{i=0,x-2}_{j=0,y-2} image(i,j)  _/
         *
         *  = sum_{i=0,x-2}_{j=0,y-1} image(i,j)
         *  + sum_          {j=0,y-1} image(x-1, j)
         *
         *  = sum_{j=0,y-1} [ ( sum_{i=0,x-2} image(i,j) ) + image(x-1,j) ]
         *  = sum_{j=0,y-1}     sum_{i=0,x-1} image(i,j)      q.e.d.
         *
         *  It's simpler when you draw it...
         */
    }

    /* Then calculate average = (flux in window) / (image size) */
    for (i = 0; i < nx*ny; i++)
    {
        int x = (i % nx);
        int y = (i / nx);

        int lower, upper;
        int left, right;

        lower = y - hsizey; if (lower <   0) lower = 0;
        upper = y + hsizey; if (upper >= ny) upper = ny - 1;

        left  = x - hsizex; if (left  <   0) left  = 0;
        right = x + hsizex; if (right >= nx) right = nx - 1;

        out[x + y*nx] = (PIXEL_TYPE)((
        (
            aux[(right+1) + (upper+1)*(nx+1)] +
            aux[ left     +  lower   *(nx+1)] -
            aux[ left     + (upper+1)*(nx+1)] -
            aux[(right+1) +  lower   *(nx+1)]
            )
        /
        (PIXEL_TYPE) ( (upper-lower+1) * (right-left+1) )));
    }

    free(aux);

    return;
}

#else

/*----------------------------------------------------------------------------*/
/**
   @internal
   @brief   Average filter
   @param   out          filtered image
   @param   in           raw image
   @param   nx           Number of rows on image
   @param   ny           Number of cols on image
   @param   hsizex       width of filter window is 2*radius_x + 1
   @param   hsizey       height of filter window is 2*radius_y + 1
   @return  void
   @see uves_filter_image_average()
*/
/*----------------------------------------------------------------------------*/
static void
TYPE_ADD(image_filter_average_bf)(PIXEL_TYPE * out, const PIXEL_TYPE * in,
                                  int Nx, int Ny, int hsizex, int hsizey,
                                  unsigned border_mode)
{

    int y;

    assert(border_mode == CPL_BORDER_FILTER);


    for (y = 0; y < Ny; y++, out += Nx) {
        int x;

        for (x = 0; x < Nx; x++) {
            int k = 0;
            int i, j;
            PIXEL_TYPE sum = (PIXEL_TYPE)0;

            for (j = y < hsizey ? 0 : y-hsizey;
                 j <= (y+hsizey >= Ny ? Ny - 1 : y+hsizey); j++) {
                for (i = x < hsizex ? 0 : x-hsizex;
                     i <= (x+hsizex >= Nx ? Nx-1 : x+hsizex); i++) {
                    sum += in[i + j*Nx];
                    k++;
                }
            }
            
            out[x] = sum/(PIXEL_TYPE)k;
        }
    }

    return;
}

#endif


#undef SWAP_ONE
#undef SORT_ONE
