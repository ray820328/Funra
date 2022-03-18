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

static void
ADDTYPE_TWO(cpl_filter_average)(void *, const void *, cpl_size, cpl_size,
                                cpl_size, cpl_size, cpl_border_mode);


static void
ADDTYPE_TWO(cpl_filter_average_bpm)(void *, cpl_binary **,
                                    const void *, const cpl_binary *,
                                    cpl_size, cpl_size, cpl_size, cpl_size,
                                    cpl_border_mode);

static cpl_error_code
ADDTYPE_TWO(cpl_filter_average_slow)(void *, cpl_binary **,
                                     const void *, const cpl_binary *,
                                     const cpl_binary *, cpl_size, cpl_size,
                                     cpl_size, cpl_size, cpl_border_mode);

static void
ADDTYPE_TWO(cpl_filter_stdev)(void *, const void *, cpl_size, cpl_size,
                              cpl_size, cpl_size, cpl_border_mode);

static cpl_error_code
ADDTYPE_TWO(cpl_filter_stdev_slow)(void *, cpl_binary **,
                                   const void *, const cpl_binary *,
                                   const cpl_binary *, cpl_size, cpl_size,
                                   cpl_size, cpl_size, cpl_border_mode);

static cpl_error_code
ADDTYPE_TWO(cpl_filter_linear_slow)(void *, cpl_binary **,
                                    const void *, const cpl_binary *,
                                    const double *, cpl_size, cpl_size,
                                    cpl_boolean,
                                    cpl_size, cpl_size, cpl_border_mode);

static cpl_error_code
ADDTYPE_TWO(cpl_filter_morpho_slow)(void *, cpl_binary **,
                                    const void *, const cpl_binary *,
                                    const double *, cpl_size, cpl_size,
                                    cpl_boolean,
                                    cpl_size, cpl_size, cpl_border_mode);

#ifndef CPL_FILTER_NO_RECIPROCAL
#ifndef ACCU_TYPE_IS_INT
#define ALLOW_RECIPROCAL
#endif
#endif

#ifdef ALLOW_RECIPROCAL
  /* If possible, and in order to speed-up the code, the central part of
     averaged image is done with multiplication instead of division, incurring
     one extra round-off */
#define RUNSUM_MEAN (runsum * rnpix)
#else
#define RUNSUM_MEAN (runsum / npix)
#endif


/*
  The functionss below are documented with Doxygen as usual,
  but the preprocessor generated function names mean that
  doxygen cannot actually parse it. So it is all ignored.
  @cond
 */

#ifdef IN_EQ_OUT

/* Start of no-cast code */

static cpl_error_code ADDTYPE(cpl_filter_median_slow)(void *, cpl_binary **,
                                                      const void *,
                                                      const cpl_binary *,
                                                      const cpl_binary *,
                                                      cpl_size, cpl_size,
                                                      cpl_size, cpl_size,
                                                      cpl_border_mode);

#ifndef CPL_MEDIAN_SAMPLE
static void
ADDTYPE(cpl_filter_median_even)(void *, const void *,
                                cpl_size, cpl_size,
                                cpl_size, cpl_size) CPL_ATTR_NONNULL;

/*----------------------------------------------------------------------------*/
/**
   @internal
   @ingroup cpl_image
   @brief  Median filter border positions with even samples and no bpm
   @param  vout   The pixel buffer to hold the filtered result
   @param  vin    The input pixel buffer to be filtered
   @param  nx    image width
   @param  ny    image height
   @param  rx    filtering half-size in x
   @param  ry    filtering half-size in y
   @return void
   @note This function must be called after a fast-filtering (i.e. full-kernel
         and no bad pixels) with border mode set to filter, in order to
         refilter the positions that have an even number of samples

   For a position on the left or right side of the image, where an odd number
   of columns of the filtering kernel is outside the input buffer, there is
   an even number of samples.

   For a position on the top or bottom part of the image, where an odd number
   of rows of the filtering kernel is outside the input buffer, there is
   an even number of samples.

   In the four corners of the image, there is an even number of samples, if
   one or both of the above is true.
   
*/
/*----------------------------------------------------------------------------*/
static void ADDTYPE(cpl_filter_median_even)(void * vout, const void * vin,
                                            cpl_size nx, cpl_size ny,
                                            cpl_size rx, cpl_size ry)
{

    OUT_TYPE * out = (OUT_TYPE*)vout;
    const IN_TYPE  * in  = (const IN_TYPE*)vin;
    IN_TYPE window[(2 * rx + 1) * (2 * ry + 1)];
    cpl_size     x, y;


    /* FIXME: Create 4 shorter loops... ? */
    for (y = 0; y < ny; y++) {
        const size_t y_1 = CX_MAX(0,    y - ry);
        const size_t y2  = CX_MIN(ny-1, y + ry);
        for (x = 0; x < nx; x++) {
            const size_t     x1 = CX_MAX(0,    x - rx);
            const size_t     x2 = CX_MIN(nx-1, x + rx);

            if (((2 * ry - (y2 - y_1)) & 1) != 0 ||
                ((2 * rx - (x2 - x1 )) & 1) != 0) {
                /* The pixel position has an even number of samples */

                const IN_TYPE * inj = in + y_1 * nx;
                size_t j, k = 0;

                for (j = y_1; j < 1 + y2; j++, inj += nx, k += 1 + x2 - x1) {
                    (void)memcpy(window + k, inj + x1,
                                 (1 + x2 - x1) * sizeof(IN_TYPE));
                }
                assert( k % 2 == 0);

                out[x + y * nx] = ADDTYPE(cpl_tools_get_median)(window, k);
            }
        }
    }
}
#endif

/*----------------------------------------------------------------------------*/
/**
   @internal
   @ingroup cpl_image
   @brief  Median filter with bad pixels
   @param  vout   The pixel buffer to hold the filtered result
   @param  ppbpm *ppbpm is the bpm, or NULL if still to be created
   @param  vin    The input pixel buffer to be filtered
   @param  bpm   The input bad pixel map, or NULL if none
   @param  pmask The window
   @param  nx    image width
   @param  ny    image height
   @param  rx    filtering half-size in x
   @param  ry    filtering half-size in y
   @return void
   
*/
/*----------------------------------------------------------------------------*/
static cpl_error_code
ADDTYPE(cpl_filter_median_slow)(void * vout, cpl_binary ** ppbpm,
                                const void * vin,
                                const cpl_binary * bpm,
                                const cpl_binary * pmask,
                                cpl_size nx, cpl_size ny,
                                cpl_size rx, cpl_size ry,
                                cpl_border_mode mode)
{

    if (mode == CPL_BORDER_FILTER) {
        OUT_TYPE * out = (OUT_TYPE*)vout;
        const IN_TYPE  * in  = (const IN_TYPE*)vin;
        IN_TYPE window[(2 * rx + 1) * (2 * ry + 1)];
        cpl_size     x, y;

        for (y = 0; y < ny; y++) {
            const size_t y_1 = CX_MAX(0,    y - ry);
            const size_t y2  = CX_MIN(ny-1, y + ry);
            for (x = 0; x < nx; x++) {
                const size_t     x1 = CX_MAX(0,    x - rx);
                const size_t     x2 = CX_MIN(nx-1, x + rx);
                const IN_TYPE    * inj  = in + y_1 * nx;
                const cpl_binary * pmaskj = pmask
                     + (rx - x) + ((ry - y) + y_1) * (2 * rx + 1);
                size_t k = 0;
                size_t i, j;

                if (bpm == NULL) {
                    for (j = y_1; j < 1 + y2; j++, inj += nx,
                             pmaskj += 2 * rx + 1) {
                        for (i = x1; i < 1 + x2; i++) {
                            if (pmaskj[i])
                                window[k++] = inj[i];
                        }
                    }
                } else {
                    const cpl_binary * bpmj = bpm + y_1 * nx;

                    for (j = y_1; j < 1 + y2; j++, inj += nx, bpmj += nx,
                             pmaskj += 2 * rx + 1) {
                        for (i = x1; i < 1 + x2; i++) {
                            if (!bpmj[i] && pmaskj[i])
                                window[k++] = inj[i];
                        }
                    }
                }
                if (k > 0) {
                    out[x + y * nx] = ADDTYPE(cpl_tools_get_median)(window, k);
                } else {
                    /* Flag as bad - and set to zero */
                    if (*ppbpm == NULL) {
                        *ppbpm = cpl_calloc((size_t)nx * (size_t)ny,
                                            sizeof(cpl_binary));
                    }
                    (*ppbpm)[x + y * nx] = CPL_BINARY_1;
                    out[x + y * nx] = (OUT_TYPE)0;
                }
            }
        }
    } else if (mode == CPL_BORDER_NOP || mode == CPL_BORDER_COPY ||
               mode == CPL_BORDER_CROP) {
        OUT_TYPE      * out = (OUT_TYPE*)vout;
        const IN_TYPE * in  = (const IN_TYPE*)vin;
        IN_TYPE window[(2 * rx + 1) * (2 * ry + 1)];
        cpl_size     x, y;
        cpl_size     nxo = mode == CPL_BORDER_CROP ? nx - 2 * rx : nx;
        cpl_size     nyo = mode == CPL_BORDER_CROP ? ny - 2 * ry : ny;

        if (mode == CPL_BORDER_COPY) {
            /* Copy the first ry row(s) and
               then the first rx pixel(s) of the next row */
            (void)memcpy(vout, vin, (size_t)(nx * ry + rx) * sizeof(*out));
        }

        for (y = ry; y < ny - ry; y++) {
            const size_t y_1 = y - ry;
            const size_t y2  = y + ry;
            const size_t yo = mode == CPL_BORDER_CROP ? y_1 : (size_t)y;

            for (x = rx; x < nx - rx; x++) {
                const size_t     x1 = x - rx;
                const size_t     x2 = x + rx;
                const size_t     xo = mode == CPL_BORDER_CROP ? x1 : (size_t)x;
                const IN_TYPE    * inj  = in + y_1 * nx;
                const cpl_binary * pmaskj = pmask
                     + (rx - x) + ((ry - y) + y_1) * (2 * rx + 1);
                size_t k = 0;
                size_t i, j;

                if (bpm == NULL) {
                    for (j = y_1; j < 1 + y2; j++, inj += nx,
                             pmaskj += 2 * rx + 1) {
                        for (i = x1; i < 1 + x2; i++) {
                            if (pmaskj[i])
                                window[k++] = inj[i];
                        }
                    }
                } else {
                    const cpl_binary * bpmj = bpm + y_1 * nx;

                    for (j = y_1; j < 1 + y2; j++, inj += nx, bpmj += nx,
                             pmaskj += 2 * rx + 1) {
                        for (i = x1; i < 1 + x2; i++) {
                            if (!bpmj[i] && pmaskj[i])
                                window[k++] = inj[i];
                        }
                    }
                }
                if (k > 0) {
                    out[xo + yo * nxo] =
                        ADDTYPE(cpl_tools_get_median)(window, k);
                } else {
                    /* Flag as bad - and set to zero */
                    if (*ppbpm == NULL) {
                        *ppbpm = cpl_calloc((size_t)nxo * (size_t)nyo,
                                            sizeof(cpl_binary));
                    }
                    (*ppbpm)[xo + yo * nxo] = CPL_BINARY_1;
                    out[xo + yo * nxo] = (OUT_TYPE)0;
                }
            }

            if (mode == CPL_BORDER_COPY && rx > 0 && y < ny - ry - 1) {
                /* Copy the last rx pixel(s) of this row and
                   the first rx of the next */
                (void)memcpy(out + (size_t)(nx * y + x),
                             in + (size_t)(nx * y + x),
                             2 * (size_t)rx * sizeof(*out));
            }
        }

        if (mode == CPL_BORDER_COPY) {
            /* Copy the last rx pixel(s) of the previous row and
               the last ry row(s)*/
            (void)memcpy(out + (size_t)(nx * y - rx),
                         in + (size_t)(nx * y - rx),
                         (size_t)(nx * ry + rx) * sizeof(*out));
        }
    } else {
        /* FIXME: Support more modes ? */
        return cpl_error_set_(CPL_ERROR_UNSUPPORTED_MODE);
    }

    return CPL_ERROR_NONE;
}

/* End of no-cast code */
#endif

/*----------------------------------------------------------------------------*/
/**
   @internal
   @ingroup cpl_image
   @brief  Average filter with bad pixels
   @param  vout  The pixel buffer to hold the filtered result
   @param  ppbpm *ppbpm is the bpm, or NULL if still to be created
   @param  vin   The input pixel buffer to be filtered
   @param  bpm   The input bad pixel map, or NULL if none
   @param  pmask The window
   @param  nx    image width
   @param  ny    image height
   @param  rx    filtering half-size in x
   @param  ry    filtering half-size in y
   @return void
   @see cpl_filter_median_slow
   @note FIXME: Modified from cpl_filter_median_slow
   
*/
/*----------------------------------------------------------------------------*/
static cpl_error_code
ADDTYPE_TWO(cpl_filter_average_slow)(void * vout, cpl_binary ** ppbpm,
                                     const void * vin,
                                     const cpl_binary * bpm,
                                     const cpl_binary * pmask,
                                     cpl_size nx, cpl_size ny,
                                     cpl_size rx, cpl_size ry,
                                     cpl_border_mode mode)
{

    if (mode == CPL_BORDER_FILTER) {
        OUT_TYPE     * out = (OUT_TYPE*)vout;
        const IN_TYPE * in = (const IN_TYPE*)vin;
        cpl_size   x, y;

        for (y = 0; y < ny; y++) {
            const size_t y_1 = (size_t)CX_MAX(0,    y - ry);
            const size_t y2  = (size_t)CX_MIN(ny-1, y + ry);
            for (x = 0; x < nx; x++) {
                const size_t x1 = (size_t)CX_MAX(0,    x - rx);
                const size_t x2 = (size_t)CX_MIN(nx-1, x + rx);
                const IN_TYPE    * inj  = in + y_1 * nx;
                const cpl_binary * pmaskj = pmask
                     + (rx - x) + ((ry - y) + y_1) * (2 * rx + 1);
                ACCU_TYPE meansum = (ACCU_TYPE)0; /* Sum for int else mean */

                size_t k = 0;
                size_t i, j;


                if (bpm == NULL) {
                    for (j = y_1; j < 1 + y2; j++, inj += nx,
                             pmaskj += 2 * rx + 1) {
                        for (i = x1; i < 1 + x2; i++) {
                            if (pmaskj[i]) {
#if defined ACCU_TYPE_IS_INT || !defined CPL_FILTER_AVERAGE_SLOW
                                meansum += (ACCU_TYPE)inj[i];
                                k++;
#else
                                /* This recurrence is taken from
                                   cpl_vector_get_mean() */
                                /* It is turned off by default, since
                                   the number of samples is limited */
                                meansum += (inj[i] - meansum) / ++k;
#endif
                            }
                        }
                    }
                } else {
                    const cpl_binary * bpmj = bpm + y_1 * nx;

                    for (j = y_1; j < 1 + y2; j++, inj += nx, bpmj += nx,
                             pmaskj += 2 * rx + 1) {
                        for (i = x1; i < 1 + x2; i++) {
                            if (!bpmj[i] && pmaskj[i]) {
#if defined ACCU_TYPE_IS_INT || !defined CPL_FILTER_AVERAGE_SLOW
                                meansum += (ACCU_TYPE)inj[i];
                                k++;
#else
                                /* This recurrence is taken from
                                   cpl_vector_get_mean() */
                                /* It is turned off by default, since
                                   the number of samples is limited */
                                meansum += (inj[i] - meansum) / ++k;
#endif
                            }
                        }
                    }
                }
                if (k > 0) {
#if defined ACCU_TYPE_IS_INT || !defined CPL_FILTER_AVERAGE_SLOW
                    meansum /= (ACCU_TYPE)k;
#endif
                } else {
                    /* Flag as bad - and set to zero */
                    if (*ppbpm == NULL) {
                        *ppbpm = cpl_calloc((size_t)nx * (size_t)ny,
                                            sizeof(cpl_binary));
                    }
                    (*ppbpm)[x + y * nx] = CPL_BINARY_1;
                }
                out[x + y * nx] = (OUT_TYPE)meansum;
            }
        }
    } else {
        /* FIXME: Support more modes ? */
        return cpl_error_set_(CPL_ERROR_UNSUPPORTED_MODE);
    }

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
   @internal
   @ingroup cpl_image
   @brief  Stdev filter with bad pixels
   @param  vout  The pixel buffer to hold the filtered result
   @param  ppbpm *ppbpm is the bpm, or NULL if still to be created
   @param  vin   The input pixel buffer to be filtered
   @param  bpm   The input bad pixel map, or NULL if none
   @param  pmask The window
   @param  nx    image width
   @param  ny    image height
   @param  rx    filtering half-size in x
   @param  ry    filtering half-size in y
   @return void
   @see cpl_filter_average_slow, cpl_tools_get_variancesum
   @note FIXME: Modified from cpl_filter_average_slow
   
*/
/*----------------------------------------------------------------------------*/
static cpl_error_code
ADDTYPE_TWO(cpl_filter_stdev_slow)(void * vout, cpl_binary ** ppbpm,
                                    const void * vin,
                                    const cpl_binary * bpm,
                                    const cpl_binary * pmask,
                                    cpl_size nx, cpl_size ny,
                                    cpl_size rx, cpl_size ry,
                                    cpl_border_mode mode)
{

    if (mode == CPL_BORDER_FILTER) {
        OUT_TYPE * out = (OUT_TYPE*)vout;
        const IN_TYPE  * in  = (const IN_TYPE*)vin;
        cpl_size   x, y;

        for (y = 0; y < ny; y++) {
            const size_t y_1 = CX_MAX(0,    y - ry);
            const size_t y2  = CX_MIN(ny-1, y + ry);
            for (x = 0; x < nx; x++) {
                const size_t     x1 = CX_MAX(0,    x - rx);
                const size_t     x2 = CX_MIN(nx-1, x + rx);
                const IN_TYPE   * inj  = in + y_1 * nx;
                const cpl_binary * pmaskj = pmask
                     + (rx - x) + ((ry - y) + y_1) * (2 * rx + 1);
                ACCU_FLOATTYPE varsum = 0.0;
                ACCU_FLOATTYPE mean = 0.0;

                double k = 0;
                size_t i, j;

                if (bpm == NULL) {
                    for (j = y_1; j < 1 + y2; j++, inj += nx,
                             pmaskj += 2 * rx + 1) {
                        for (i = x1; i < 1 + x2; i++) {
                            if (pmaskj[i]) {
                                const ACCU_FLOATTYPE delta
                                    = (ACCU_FLOATTYPE)inj[i] - mean;

                                varsum += k * delta * delta / (k + 1.0);
                                mean   += delta / (k + 1.0);
                                k += 1.0;
                            }
                        }
                    }
                } else {
                    const cpl_binary * bpmj = bpm + y_1 * nx;

                    for (j = y_1; j < 1 + y2; j++, inj += nx, bpmj += nx,
                             pmaskj += 2 * rx + 1) {
                        for (i = x1; i < 1 + x2; i++) {
                            if (!bpmj[i] && pmaskj[i]) {
                                const ACCU_FLOATTYPE delta
                                    = (ACCU_FLOATTYPE)inj[i] - mean;

                                varsum += k * delta * delta / (k + 1.0);
                                mean   += delta / (k + 1.0);
                                k += 1.0;
                            }
                        }
                    }
                }
                if (k > 1) {
                    out[x + y * nx] = (OUT_TYPE)sqrt(varsum / (k - 1.0));
                } else {
                    /* Flag as bad - and set to zero */
                    if (*ppbpm == NULL) {
                        *ppbpm = cpl_calloc((size_t)nx * (size_t)ny,
                                            sizeof(cpl_binary));
                    }
                    (*ppbpm)[x + y * nx] = CPL_BINARY_1;
                    out[x + y * nx] = (OUT_TYPE)0;
                }
            }
        }
    } else {
        /* FIXME: Support more modes ? */
        return cpl_error_set_(CPL_ERROR_UNSUPPORTED_MODE);
    }

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
   @internal
   @ingroup cpl_image
   @brief  Stdev filter without bad pixels
   @param  vout  The pixel buffer to hold the filtered result
   @param  vin   The input pixel buffer to be filtered
   @param  nx    image width
   @param  ny    image height
   @param  rx    filtering half-size in x
   @param  ry    filtering half-size in y
   @return void
   @see cpl_filter_stdev_slow
   
*/
/*----------------------------------------------------------------------------*/
static void
ADDTYPE_TWO(cpl_filter_stdev)(void * vout,
                              const void * vin,
                              cpl_size nx, cpl_size ny,
                              cpl_size hsizex, cpl_size hsizey,
                              cpl_border_mode border_mode)
{

    OUT_TYPE * out = (OUT_TYPE*)vout;
    const IN_TYPE  * in  = (const IN_TYPE*)vin;

    double * colvarsum;
    double * colmean;
    cpl_size npix, npixy;
    cpl_size x, y, i;

    double npixcol, ncol;

    assert(out != NULL);
    assert(in != NULL);

    assert( hsizex   >= 0 );
    assert( hsizey   >= 0 );
    assert( 2*hsizex <  nx ); /* Caller does not (need to) support this */
    assert( 2*hsizey <  ny ); /* Caller does not (need to) support this */

    assert( hsizex > 0 || hsizey > 0);

    /* FIXME: Support more modes ? */
    assert( border_mode == CPL_BORDER_FILTER );

    /* The number of pixels prior to the first stdev */
    npixy = hsizey;
    npix = (1+hsizex) * npixy;

    /* Initialize the column sums */
    colvarsum = cpl_calloc((size_t)nx, sizeof(double));
    colmean   = cpl_calloc((size_t)nx, sizeof(double));

    npixcol = 0.0;

    for (y=0; y < hsizey; y++) {
        for (i=0; i < nx; i++) {
            const double delta = (double)in[i + y * nx] - colmean[i];

            colvarsum[i] += npixcol / (npixcol + 1.0) * delta * delta;
            colmean[i]   += delta / (npixcol + 1.0);
        }

        npixcol += 1.0;
    }

    
    /* Loop over all the first rows, that cannot be covered by the kernel */
    for (y = 0; y < 1 + hsizey; y++) {

        double runvarsum = (double)0;
        double runmean   = (double)0;


        npixy ++;
        npix = (hsizex)*npixy;
    
        ncol = 0.0;

        for (x = 0; x < 1; x ++) {

            /* Update the first 1 + hsizex column sums */
            for (i = 0; i < 1 + hsizex; i ++) {
                const double delta    = 
                    (double)in[i + (y + hsizey) * nx] - colmean[i];
                double rundelta;
 
                colvarsum[i] += npixcol / (npixcol + 1.0) * delta * delta;
                colmean[i]   += delta / (npixcol + 1.0);

                rundelta = colmean[i] - runmean;

                runvarsum += 
                    colvarsum[i] + ncol / (ncol + 1.0) * (npixcol + 1.0) *
                    rundelta * rundelta;
                runmean   += rundelta / (ncol + 1.0);
        
                ncol += 1.0;
            }

            npix += npixy;

            out[x + y * nx] = (OUT_TYPE)sqrt(runvarsum/(double)(npix - 1));
        }

        for (; x < 1 + hsizex; x ++) {
            const double delta = 
                (double)in[x + hsizex + (y + hsizey) * nx] - 
                colmean[x + hsizex];
            double rundelta;
        
            colvarsum[x + hsizex] += npixcol / (npixcol + 1.0) * delta * delta;
            colmean[x + hsizex]   += delta / (npixcol + 1.0);

            rundelta = colmean[x + hsizex] - runmean;

            runvarsum += 
                colvarsum[x + hsizex] + ncol / (ncol + 1.0) * (npixcol + 1.0) *
                rundelta * rundelta;
            runmean   += rundelta / (ncol + 1.0);

            ncol += 1.0;
            npix += npixy;

            out[x + y * nx] = (OUT_TYPE)sqrt(runvarsum/(double)(npix - 1));
        }

        for (; x < nx-hsizex; x ++) {
            const double delta = 
                (double)in[x + hsizex + (y + hsizey) * nx] - 
                colmean[x + hsizex];
            double rundelta;
        
            colvarsum[x + hsizex] += npixcol / (npixcol + 1.0) * delta * delta;
            colmean[x + hsizex]   += delta / (npixcol + 1.0);

            if (ncol > 1.0) {
                rundelta = colmean[x - hsizex - 1] - runmean;

                runvarsum -= 
                    colvarsum[x - hsizex - 1] + 
                    ncol / (ncol - 1.0) * (npixcol + 1.0) *
                    rundelta * rundelta;
                runmean   -= rundelta / (ncol - 1.0);

                rundelta = colmean[x + hsizex] - runmean;

                runvarsum += 
                    colvarsum[x + hsizex] + 
                    (ncol - 1.0) / ncol * (npixcol + 1.0) *
                    rundelta * rundelta;
                runmean   += rundelta / ncol;
            } else {
                runvarsum = colvarsum[x + hsizex];
            }

            out[x + y * nx] = (OUT_TYPE)sqrt(runvarsum/(double)(npix - 1));
        }

        for (; x < nx; x++) {
            const double rundelta = colmean[x - hsizex - 1] - runmean;

            runvarsum -= 
                colvarsum[x - hsizex - 1] + 
                ncol / (ncol - 1.0) * (npixcol + 1.0) *
                rundelta * rundelta;
            runmean   -= rundelta / (ncol - 1.0);

            ncol -= 1.0;
            npix -= npixy;

            out[x + y * nx] = (OUT_TYPE)sqrt(runvarsum/(double)(npix - 1));
        }

        npixcol += 1.0;
    }

    /* Loop over the central rows */
    for (;y < ny-hsizey; y++) {

        double runvarsum = (double)0;
        double runmean   = (double)0;

        npix = (hsizex)*npixy;

        ncol = 0.0;

        for (x = 0; x < 1; x++) {

            /* Update the first 1 + hsizex column sums */
            for (i = 0; i < 1 + hsizex; i ++) {
                double delta;
                double rundelta;

                if (npixcol > 1.0) {
                    delta = (double)in[i + (y - hsizey - 1) * nx] - colmean[i];
 
                    colvarsum[i] -= 
                        npixcol / (npixcol - 1.0) * delta * delta;
                    colmean[i]   -= delta / (npixcol - 1.0);

                    delta = (double)in[i + (y + hsizey) * nx] - colmean[i];
 
                    colvarsum[i] += 
                        (npixcol - 1.0) / npixcol * delta * delta;
                    colmean[i]   += delta / npixcol;
                } else {
                    colvarsum[i] = 0.0;
                    colmean[i]   = (double)in[i + (y + hsizey) * nx];
                }

                rundelta = colmean[i] - runmean;

                runvarsum += 
                    colvarsum[i] + ncol / (ncol + 1.0) * npixcol *
                    rundelta * rundelta;
                runmean   += rundelta / (ncol + 1.0);
        
                ncol += 1.0;
            }

            npix += npixy;

            out[x + y * nx] = (OUT_TYPE)sqrt(runvarsum/(double)(npix - 1));
        }

        for (; x < 1 + hsizex; x++) {
            double delta;
            double rundelta;

            if (npixcol > 1.0) {
                delta = 
                    (double)in[x + hsizex + (y - hsizey - 1) * nx] - 
                    colmean[x + hsizex];
 
                colvarsum[x + hsizex] -= 
                    npixcol / (npixcol - 1.0) * delta * delta;
                colmean[x + hsizex]   -= delta / (npixcol - 1.0);

                delta = 
                    (double)in[x + hsizex + (y + hsizey) * nx] - 
                    colmean[x + hsizex];
 
                colvarsum[x + hsizex] += 
                    (npixcol - 1.0) / npixcol * delta * delta;
                colmean[x + hsizex]   += delta / npixcol;
            } else {
                colvarsum[x + hsizex] = 0.0;
                colmean[x + hsizex]   = 
                    (double)in[x + hsizex + (y + hsizey) * nx];
            }

            rundelta = colmean[x + hsizex] - runmean;

            runvarsum += 
                colvarsum[x + hsizex] + ncol / (ncol + 1.0) * npixcol *
                rundelta * rundelta;
            runmean   += rundelta / (ncol + 1.0);

            ncol += 1.0;
            npix += npixy;

            out[x + y * nx] = (OUT_TYPE)sqrt(runvarsum/(double)(npix - 1));
        }

     
        for (; x < nx-hsizex; x++) {
            double delta;
            double rundelta;

            if (npixcol > 1.0) {
                delta = 
                    (double)in[x + hsizex + (y - hsizey - 1) * nx] - 
                    colmean[x + hsizex];
 
                colvarsum[x + hsizex] -= 
                    npixcol / (npixcol - 1.0) * delta * delta;
                colmean[x + hsizex]   -= delta / (npixcol - 1.0);

                delta = 
                    (double)in[x + hsizex + (y + hsizey) * nx] - 
                    colmean[x + hsizex];
 
                colvarsum[x + hsizex] += 
                    (npixcol - 1.0) / npixcol * delta * delta;
                colmean[x + hsizex]   += delta / npixcol;
            } else {
                colvarsum[x + hsizex] = 0.0;
                colmean[x + hsizex]   = 
                    (double)in[x + hsizex + (y + hsizey) * nx];
            }

            if (ncol > 1.0) {
                rundelta = colmean[x - hsizex - 1] - runmean;

                runvarsum -= 
                    colvarsum[x - hsizex - 1] + 
                    ncol / (ncol - 1.0) * npixcol * rundelta * rundelta;
                runmean   -= rundelta / (ncol - 1.0);

                rundelta = colmean[x + hsizex] - runmean;

                runvarsum += 
                    colvarsum[x + hsizex] + 
                    (ncol - 1.0) / ncol * npixcol * rundelta * rundelta;
                runmean   += rundelta / ncol;
            } else {
                runvarsum = colvarsum[x + hsizex];
            }

            out[x + y * nx] = (OUT_TYPE)sqrt(runvarsum/(double)(npix - 1));
        }

        for (; x < nx; x++) {
            const double rundelta = colmean[x - hsizex - 1] - runmean;

            runvarsum -= 
                colvarsum[x - hsizex - 1] + 
                ncol / (ncol - 1.0) * npixcol *    rundelta * rundelta;
            runmean   -= rundelta / (ncol - 1.0);

            ncol -= 1.0;
            npix -= npixy;

            out[x + y * nx] = (OUT_TYPE)sqrt(runvarsum/(double)(npix - 1));
        }
    }

    /* Loop over all the last rows, that cannot be covered by the kernel */
    for (;y < ny; y++) {

        double runvarsum = (double)0;
        double runmean   = (double)0;

        npixy --;
        npix = (hsizex)*npixy;

        ncol = 0.0;

        for (x = 0; x < 1; x++) {

            /* Update the first 1 + hsizex column sums */
            for (i = 0; i < 1 + hsizex; i ++) {
                const double delta = 
                    (double)in[i + (y - hsizey - 1) * nx] - colmean[i];
                double rundelta;

                colvarsum[i] -= 
                    npixcol / (npixcol - 1.0) * delta * delta;
                colmean[i]   -= delta / (npixcol - 1.0);

                rundelta = colmean[i] - runmean;

                runvarsum += 
                    colvarsum[i] + ncol / (ncol + 1.0) * (npixcol - 1.0) *
                    rundelta * rundelta;
                runmean   += rundelta / (ncol + 1.0);

                ncol += 1.0;
            }

            npix += npixy;

            out[x + y * nx] = (OUT_TYPE)sqrt(runvarsum/(double)(npix - 1));
        }

        for (; x < 1 + hsizex; x++) {
            const double delta =
                (double)in[x + hsizex + (y - hsizey - 1) * nx] - 
                colmean[x + hsizex];
            double rundelta;
 
            colvarsum[x + hsizex] -= 
                npixcol / (npixcol - 1.0) * delta * delta;
            colmean[x + hsizex]   -= delta / (npixcol - 1.0);

            rundelta = colmean[x + hsizex] - runmean;

            runvarsum += 
                colvarsum[x + hsizex] + ncol / (ncol + 1.0) * (npixcol - 1.0) *
                rundelta * rundelta;
            runmean   += rundelta / (ncol + 1.0);

            npix += npixy;
            ncol += 1.0;

            out[x + y * nx] = (OUT_TYPE)sqrt(runvarsum/(double)(npix - 1));
        }

    
        for (; x < nx-hsizex; x++) {
            const double delta =
                (double)in[x + hsizex + (y - hsizey - 1) * nx] - 
                colmean[x + hsizex];
            double rundelta;
 
            colvarsum[x + hsizex] -= 
                npixcol / (npixcol - 1.0) * delta * delta;
            colmean[x + hsizex]   -= delta / (npixcol - 1.0);

            if (ncol > 1.0) {
                rundelta = colmean[x - hsizex - 1] - runmean;

                runvarsum -= 
                    colvarsum[x - hsizex - 1] + 
                    ncol / (ncol - 1.0) * (npixcol - 1.0) * rundelta * rundelta;
                runmean   -= rundelta / (ncol - 1.0);

                rundelta = colmean[x + hsizex] - runmean;

                runvarsum += 
                    colvarsum[x + hsizex] + 
                    (ncol - 1.0) / ncol * (npixcol - 1.0) * rundelta * rundelta;
                runmean   += rundelta / ncol;
            } else {
                runvarsum = colvarsum[x + hsizex];
            }

            out[x + y * nx] = (OUT_TYPE)sqrt(runvarsum/(double)(npix - 1));
        }

        for (; x < nx; x++) {
            const double rundelta = colmean[x - hsizex - 1] - runmean;

            runvarsum -= 
                colvarsum[x - hsizex - 1] + 
                ncol / (ncol - 1.0) * (npixcol - 1.0) * rundelta * rundelta;
            runmean   -= rundelta / (ncol - 1.0);

            ncol -= 1.0;
            npix -= npixy;

            out[x + y * nx] = (OUT_TYPE)sqrt(runvarsum/(double)(npix - 1));
        }

        npixcol -= 1.0;
    }

    cpl_free(colvarsum);
    cpl_free(colmean);

    return;

}

/*----------------------------------------------------------------------------*/
/**
   @internal
   @ingroup cpl_image
   @brief  Average filter without bad pixels
   @param  vout  The pixel buffer to hold the filtered result
   @param  vin   The input pixel buffer to be filtered
   @param  nx    image width
   @param  ny    image height
   @param  rx    filtering half-size in x
   @param  ry    filtering half-size in y
   @return void
   @note FIXME: Lots of code duplication inside
   
*/
/*----------------------------------------------------------------------------*/
static void
ADDTYPE_TWO(cpl_filter_average)(void * vout, const void * vin,
                                cpl_size nx, cpl_size ny,
                                cpl_size hsizex, cpl_size hsizey,
                                cpl_border_mode border_mode)
{

    OUT_TYPE * out = (OUT_TYPE*)vout;
    const IN_TYPE  * in  = (const IN_TYPE*)vin;

    ACCU_TYPE * colsum;
    cpl_size npix, npixy;
    cpl_size x, y, i;

    assert(out != NULL);
    assert(in != NULL);

    assert( hsizex   >= 0 );
    assert( hsizey   >= 0 );
    assert( 2*hsizex <  nx ); /* Caller does not (need to) support this */
    assert( 2*hsizey <  ny ); /* Caller does not (need to) support this */

    /* FIXME: Support more modes ? */
    assert( border_mode == CPL_BORDER_FILTER );

    /* The number of pixels prior to the first average */
    npixy = hsizey;
    npix = (1+hsizex) * npixy;

    /* Initialize the column sums */
    colsum = cpl_calloc((size_t)nx, sizeof(*colsum));
    for (y=0; y < hsizey; y++) {
        for (i=0; i < nx; i++) {
            colsum[i] += in[i + y * nx];
        }
    }

    /* Loop over all the first rows, that cannot be covered by the kernel */
    for (y = 0; y < 1 + hsizey; y++) {

        ACCU_TYPE runsum = (ACCU_TYPE)0;

#ifdef ALLOW_RECIPROCAL
        ACCU_TYPE rnpix;
#endif

        npixy ++;
        npix = (hsizex)*npixy;

        for (x = 0; x < 1; x ++) {

            /* Update the first 1 + hsizex column sums */
            for (i = 0; i < 1 + hsizex; i ++) {
              colsum[i] += in[i + (y + hsizey) * nx];
              runsum += colsum[i];
            }

            npix += npixy;

            out[x + y * nx] = (OUT_TYPE)(runsum/(ACCU_TYPE)npix);
        }

        for (; x < 1 + hsizex; x ++) {

            /* Update the new colsum */
            colsum[x + hsizex] += in[x + hsizex + (y + hsizey) * nx];

            /* Update the running sum */
            runsum += colsum[x + hsizex];

            npix += npixy;

            out[x + y * nx] = (OUT_TYPE)(runsum/(ACCU_TYPE)npix);
        }

#ifdef ALLOW_RECIPROCAL
        rnpix = (ACCU_TYPE)1.0/(ACCU_TYPE)npix;
#endif

        for (; x < nx-hsizex; x ++) {

            /* Update the new colsum */
            colsum[x + hsizex] += in[x + hsizex + (y + hsizey) * nx];


            /* Update the running sum */
            runsum -= colsum[x - hsizex - 1];
            runsum += colsum[x + hsizex];

            out[x + y * nx] = (OUT_TYPE)(RUNSUM_MEAN);
        }

        for (; x < nx; x++) {

           /* Update the running sum */
            runsum -= colsum[x - hsizex - 1];

            npix -= npixy;

            out[x + y * nx] = (OUT_TYPE)(runsum/(ACCU_TYPE)npix);
        }
    }

    /* Loop over the central rows */
    for (;y < ny-hsizey; y++) {

        ACCU_TYPE runsum = (ACCU_TYPE)0;

#ifdef ALLOW_RECIPROCAL
        ACCU_TYPE rnpix;
#endif
        npix = (hsizex)*npixy;

        for (x = 0; x < 1; x++) {

            /* Update the first 1 + hsizex column sums */
            for (i = 0; i < 1 + hsizex; i ++) {
              colsum[i] -= in[i + (y - hsizey - 1) * nx];
              colsum[i] += in[i + (y + hsizey) * nx];
              runsum += colsum[i];
            }

            npix += npixy;

            out[x + y * nx] = (OUT_TYPE)(runsum/(ACCU_TYPE)npix);
        }

        for (; x < 1 + hsizex; x++) {

            /* Update the new colsum */
            colsum[x + hsizex] -= in[x + hsizex + (y - hsizey - 1) * nx];
            colsum[x + hsizex] += in[x + hsizex + (y + hsizey) * nx];

            /* Update the running sum */
            runsum += colsum[x + hsizex];

            npix += npixy;

            out[x + y * nx] = (OUT_TYPE)(runsum/(ACCU_TYPE)npix);
        }

#ifdef ALLOW_RECIPROCAL
        rnpix = (ACCU_TYPE)1.0/(ACCU_TYPE)npix;
#endif
     
        for (; x < nx-hsizex; x++) {

            /* Update the new colsum */
            colsum[x + hsizex] -= in[x + hsizex + (y - hsizey - 1) * nx];
            colsum[x + hsizex] += in[x + hsizex + (y + hsizey) * nx];


            /* Update the running sum */
            runsum -= colsum[x - hsizex - 1];
            runsum += colsum[x + hsizex];

            out[x + y * nx] = (OUT_TYPE)(RUNSUM_MEAN);
        }

        for (; x < nx; x++) {

           /* Update the running sum */
            runsum -= colsum[x - hsizex - 1];

            npix -= npixy;

            out[x + y * nx] = (OUT_TYPE)(runsum/(ACCU_TYPE)npix);
        }
    }

    /* Loop over all the last rows, that cannot be covered by the kernel */
    for (;y < ny; y++) {

        ACCU_TYPE runsum = (ACCU_TYPE)0;

#ifdef ALLOW_RECIPROCAL
        ACCU_TYPE rnpix;
#endif

        npixy --;
        npix = (hsizex)*npixy;

        for (x = 0; x < 1; x++) {

            /* Update the first 1 + hsizex column sums */
            for (i = 0; i < 1 + hsizex; i ++) {
              colsum[i] -= in[i + (y - hsizey - 1) * nx];
              runsum += colsum[i];
            }

            npix += npixy;

            out[x + y * nx] = (OUT_TYPE)(runsum/(ACCU_TYPE)npix);
        }

        for (; x < 1 + hsizex; x++) {

            /* Update the new colsum */
            colsum[x + hsizex] -= in[x + hsizex + (y - hsizey - 1) * nx];

            /* Update the running sum */
            runsum += colsum[x + hsizex];

            npix += npixy;

            out[x + y * nx] = (OUT_TYPE)(runsum/(ACCU_TYPE)npix);
        }

#ifdef ALLOW_RECIPROCAL
        rnpix = (ACCU_TYPE)1.0/(ACCU_TYPE)npix;
#endif
    
        for (; x < nx-hsizex; x++) {

            /* Update the new colsum */
            colsum[x + hsizex] -= in[x + hsizex + (y - hsizey - 1) * nx];


            /* Update the running sum */
            runsum -= colsum[x - hsizex - 1];
            runsum += colsum[x + hsizex];

            out[x + y * nx] = (OUT_TYPE)(RUNSUM_MEAN);
        }

        for (; x < nx; x++) {

           /* Update the running sum */
            runsum -= colsum[x - hsizex - 1];

            npix -= npixy;

            out[x + y * nx] = (OUT_TYPE)(runsum/(ACCU_TYPE)npix);
        }
    }

    cpl_free(colsum);

    return;

}

/*----------------------------------------------------------------------------*/
/**
   @internal
   @ingroup cpl_image
   @brief  Average filter with bad pixels
   @param  vout  The pixel buffer to hold the filtered result
   @param  pbpm  *pbpm is the output bpm, or NULL if still to be created
   @param  vin   The input pixel buffer to be filtered
   @param  bpm   The input bad pixel map, or NULL if none
   @param  pmask The window
   @param  nx    image width
   @param  ny    image height
   @param  rx    filtering half-size in x
   @param  ry    filtering half-size in y
   @return void
   @note FIXME: Modified from cpl_filter_average
   
*/
/*----------------------------------------------------------------------------*/
static void
ADDTYPE_TWO(cpl_filter_average_bpm)(void * vout, cpl_binary ** pbpm,
                                    const void * vin, const cpl_binary * bpm,
                                    cpl_size nx, cpl_size ny,
                                    cpl_size hsizex, cpl_size hsizey,
                                    cpl_border_mode border_mode)
{

    OUT_TYPE * out = (OUT_TYPE*)vout;
    const IN_TYPE  * in  = (const IN_TYPE*)vin;
    ACCU_TYPE * colsum;
    int * colpix;
    cpl_size npix;
    cpl_size x, y, i;

    assert(out != NULL);
    assert(pbpm != NULL);
    assert(in != NULL);
    assert(bpm != NULL);

    assert( hsizex   >= 0 );
    assert( hsizey   >= 0 );
    assert( 2*hsizex <  nx ); /* Caller does not (need to) support this */
    assert( 2*hsizey <  ny ); /* Caller does not (need to) support this */

    /* FIXME: Support more modes ? */
    assert( border_mode == CPL_BORDER_FILTER );

    /* Initialize the column sums */
    colsum = cpl_calloc((size_t)nx, sizeof(*colsum));
    /* Initialize the counter of column elements (variable with bad pixels) */
    colpix = cpl_calloc((size_t)nx, sizeof(*colsum));
    for (y=0; y < hsizey; y++) {
        for (i=0; i < nx; i++) {
            if (!bpm[i + y * nx]) {
                colsum[i] += in[i + y * nx];
                colpix[i]++;
            }
        }
    }

    /* Loop over all the first rows, that cannot be covered by the kernel */
    for (y = 0; y < 1 + hsizey; y++) {

        ACCU_TYPE runsum = (ACCU_TYPE)0;

#ifdef ALLOW_RECIPROCAL
        ACCU_TYPE rnpix = 0.0; /* Prevent (false) uninit warning */
#endif

        npix = 0;

        for (x = 0; x < 1; x ++) {

            /* Update the first 1 + hsizex column sums */
            for (i = 0; i < 1 + hsizex; i ++) {
                if (!bpm[i + (y + hsizey) * nx]) {
                    colsum[i] += in[i + (y + hsizey) * nx];
                    runsum += colsum[i];
                    colpix[i]++;
                    npix += colpix[i];
                }
            }

            if (npix) {
                out[x + y * nx] = (OUT_TYPE)(runsum/(ACCU_TYPE)npix);
            } else {
                /* Flag as bad - and set to zero */
                if (*pbpm == NULL) *pbpm
                    = (cpl_binary*)cpl_calloc((size_t)nx * (size_t)ny,
                                              sizeof(cpl_binary));
                (*pbpm)[x + y * nx] = CPL_BINARY_1;
                out[x + y * nx] = (OUT_TYPE)0;
            }
        }

        for (; x < 1 + hsizex; x ++) {

            if (!bpm[x + hsizex + (y + hsizey) * nx]) {
                /* Update the new colsum */
                colsum[x + hsizex] += in[x + hsizex + (y + hsizey) * nx];

                /* Update the running sum */
                runsum += colsum[x + hsizex];

                colpix[x + hsizex]++;
                npix += colpix[x + hsizex];

            }
            if (npix) {
                out[x + y * nx] = (OUT_TYPE)(runsum/(ACCU_TYPE)npix);
            } else {
                /* Flag as bad - and set to zero */
                if (*pbpm == NULL) *pbpm
                    = (cpl_binary*)cpl_calloc((size_t)nx * (size_t)ny,
                                              sizeof(cpl_binary));
                (*pbpm)[x + y * nx] = CPL_BINARY_1;
                out[x + y * nx] = (OUT_TYPE)0;
            }
        }

#ifdef ALLOW_RECIPROCAL
        rnpix = (ACCU_TYPE)1.0/(ACCU_TYPE)npix;
#endif

        for (; x < nx-hsizex; x ++) {

            if (!bpm[x + hsizex + (y + hsizey) * nx]) {
                /* Update the new colsum */
                colsum[x + hsizex] += in[x + hsizex + (y + hsizey) * nx];
                colpix[x + hsizex]++;
            }
            if (colsum[x + hsizex] != colpix[x - hsizex - 1]) {
                npix += colpix[x + hsizex] - colpix[x - hsizex - 1];
#ifdef ALLOW_RECIPROCAL
                if (npix) rnpix = (ACCU_TYPE)1.0/(ACCU_TYPE)npix;
#endif
            }

            /* Update the running sum */
            runsum -= colsum[x - hsizex - 1];
            runsum += colsum[x + hsizex];

            if (npix) {
                out[x + y * nx] = (OUT_TYPE)(RUNSUM_MEAN);
            } else {
                /* Flag as bad - and set to zero */
                if (*pbpm == NULL) *pbpm
                    = (cpl_binary*)cpl_calloc((size_t)nx * (size_t)ny,
                                              sizeof(cpl_binary));
                (*pbpm)[x + y * nx] = CPL_BINARY_1;
                out[x + y * nx] = (OUT_TYPE)0;
            }
        }

        for (; x < nx; x++) {

           /* Update the running sum */
            runsum -= colsum[x - hsizex - 1];

            npix -= colpix[x - hsizex - 1];

            if (npix) {
                out[x + y * nx] = (OUT_TYPE)(runsum/(ACCU_TYPE)npix);
            } else {
                /* Flag as bad - and set to zero */
                if (*pbpm == NULL) *pbpm
                    = (cpl_binary*)cpl_calloc((size_t)nx * (size_t)ny,
                                              sizeof(cpl_binary));
                (*pbpm)[x + y * nx] = CPL_BINARY_1;
                out[x + y * nx] = (OUT_TYPE)0;
            }
        }
    }

    /* Loop over the central rows */
    for (;y < ny-hsizey; y++) {

        ACCU_TYPE runsum = (ACCU_TYPE)0;

#ifdef ALLOW_RECIPROCAL
        ACCU_TYPE rnpix = (ACCU_TYPE)0; /* Prevent (false) uninit warning */
#endif
        npix = 0;

        for (x = 0; x < 1; x++) {

            /* Update the first 1 + hsizex column sums */
            for (i = 0; i < 1 + hsizex; i ++) {
                if (!bpm[i + (y - hsizey - 1) * nx]) {
                    colsum[i] -= in[i + (y - hsizey - 1) * nx];
                    colpix[i]--;
                }
                if (!bpm[i + (y + hsizey) * nx]) {
                    colsum[i] += in[i + (y + hsizey) * nx];
                    colpix[i]++;
                }
                runsum += colsum[i];
                npix += colpix[i];
            }

            if (npix) {
                out[x + y * nx] = (OUT_TYPE)(runsum/(ACCU_TYPE)npix);
            } else {
                /* Flag as bad - and set to zero */
                if (*pbpm == NULL) *pbpm
                    = (cpl_binary*)cpl_calloc((size_t)nx * (size_t)ny,
                                              sizeof(cpl_binary));
                (*pbpm)[x + y * nx] = CPL_BINARY_1;
                out[x + y * nx] = (OUT_TYPE)0;
            }
        }

        for (; x < 1 + hsizex; x++) {

            /* Update the new colsum */
            if (!bpm[x + hsizex + (y - hsizey - 1) * nx]) {
                colsum[x + hsizex] -= in[x + hsizex + (y - hsizey - 1) * nx];
                colpix[x + hsizex]--;
            }
            if (!bpm[x + hsizex + (y + hsizey) * nx]) {
                colsum[x + hsizex] += in[x + hsizex + (y + hsizey) * nx];
                colpix[x + hsizex]++;
            }

            /* Update the running sum */
            runsum += colsum[x + hsizex];
            npix += colpix[x + hsizex];

            if (npix) {
                out[x + y * nx] = (OUT_TYPE)(runsum/(ACCU_TYPE)npix);
            } else {
                /* Flag as bad - and set to zero */
                if (*pbpm == NULL) *pbpm
                    = (cpl_binary*)cpl_calloc((size_t)nx * (size_t)ny,
                                              sizeof(cpl_binary));
                (*pbpm)[x + y * nx] = CPL_BINARY_1;
                out[x + y * nx] = (OUT_TYPE)0;
            }
        }

#ifdef ALLOW_RECIPROCAL
        if (npix > (ACCU_TYPE)0.0)
            rnpix = (ACCU_TYPE)1.0/(ACCU_TYPE)npix;
#endif
     
        for (; x < nx-hsizex; x++) {

            /* Update the new colsum */
            if (!bpm[x + hsizex + (y - hsizey - 1) * nx]) {
                colsum[x + hsizex] -= in[x + hsizex + (y - hsizey - 1) * nx];
                colpix[x + hsizex]--;
            }
            if (!bpm[x + hsizex + (y + hsizey) * nx]) {
                colsum[x + hsizex] += in[x + hsizex + (y + hsizey) * nx];
                colpix[x + hsizex]++;
            }

            /* Update the running sum */
            runsum -= colsum[x - hsizex - 1];
            runsum += colsum[x + hsizex];

            if (colsum[x + hsizex] != colpix[x - hsizex - 1]) {
                npix += colpix[x + hsizex] - colpix[x - hsizex - 1];
#ifdef ALLOW_RECIPROCAL
                if (npix) rnpix = (ACCU_TYPE)1.0/(ACCU_TYPE)npix;
#endif
            }

            if (npix) {
                out[x + y * nx] = (OUT_TYPE)(RUNSUM_MEAN);
            } else {
                /* Flag as bad - and set to zero */
                if (*pbpm == NULL) *pbpm
                    = (cpl_binary*)cpl_calloc((size_t)nx * (size_t)ny,
                                              sizeof(cpl_binary));
                (*pbpm)[x + y * nx] = CPL_BINARY_1;
                out[x + y * nx] = (OUT_TYPE)0;
            }
        }

        for (; x < nx; x++) {

           /* Update the running sum */
            runsum -= colsum[x - hsizex - 1];
            npix   -= colpix[x - hsizex - 1];

            if (npix) {
                out[x + y * nx] = (OUT_TYPE)(runsum/(ACCU_TYPE)npix);
            } else {
                /* Flag as bad - and set to zero */
                if (*pbpm == NULL) *pbpm
                    = (cpl_binary*)cpl_calloc((size_t)nx * (size_t)ny,
                                              sizeof(cpl_binary));
                (*pbpm)[x + y * nx] = CPL_BINARY_1;
                out[x + y * nx] = (OUT_TYPE)0;
            }
        }
    }

    /* Loop over all the last rows, that cannot be covered by the kernel */
    for (;y < ny; y++) {

        ACCU_TYPE runsum = (ACCU_TYPE)0;

#ifdef ALLOW_RECIPROCAL
        ACCU_TYPE rnpix = (ACCU_TYPE)0; /* Prevent (false) uninit warning */
#endif

        npix = (ACCU_TYPE)0;

        for (x = 0; x < 1; x++) {

            /* Update the first 1 + hsizex column sums */
            for (i = 0; i < 1 + hsizex; i ++) {
                if (!bpm[i + (y - hsizey - 1) * nx]) {
                    colsum[i] -= in[i + (y - hsizey - 1) * nx];
                    colpix[i]--;
                }
              runsum += colsum[i];
              npix += colpix[i];
            }

            if (npix) {
                out[x + y * nx] = (OUT_TYPE)(runsum/(ACCU_TYPE)npix);
            } else {
                /* Flag as bad - and set to zero */
                if (*pbpm == NULL) *pbpm
                    = (cpl_binary*)cpl_calloc((size_t)nx * (size_t)ny,
                                              sizeof(cpl_binary));
                (*pbpm)[x + y * nx] = CPL_BINARY_1;
                out[x + y * nx] = (OUT_TYPE)0;
            }
        }

        for (; x < 1 + hsizex; x++) {

            /* Update the new colsum */
                if (!bpm[x + hsizex + (y - hsizey - 1) * nx]) {
                    colsum[x + hsizex] -= in[x + hsizex + (y - hsizey - 1) * nx];
                    colpix[x + hsizex]--;
                }

            /* Update the running sum */
            runsum += colsum[x + hsizex];
            npix   += colpix[x + hsizex];

            if (npix) {
                out[x + y * nx] = (OUT_TYPE)(runsum/(ACCU_TYPE)npix);
            } else {
                /* Flag as bad - and set to zero */
                if (*pbpm == NULL) *pbpm
                    = (cpl_binary*)cpl_calloc((size_t)nx * (size_t)ny,
                                              sizeof(cpl_binary));
                (*pbpm)[x + y * nx] = CPL_BINARY_1;
                out[x + y * nx] = (OUT_TYPE)0;
            }
        }

#ifdef ALLOW_RECIPROCAL
        if (npix) rnpix = (ACCU_TYPE)1.0/(ACCU_TYPE)npix;
#endif
    
        for (; x < nx-hsizex; x++) {

            /* Update the new colsum */
            if (!bpm[x + hsizex + (y - hsizey - 1) * nx]) {
                colsum[x + hsizex] -= in[x + hsizex + (y - hsizey - 1) * nx];
                colpix[x + hsizex]--;
            }


            /* Update the running sum */
            runsum -= colsum[x - hsizex - 1];
            runsum += colsum[x + hsizex];

            if (colsum[x + hsizex] != colpix[x - hsizex - 1]) {
                npix += colpix[x + hsizex] - colpix[x - hsizex - 1];
#ifdef ALLOW_RECIPROCAL
                if (npix) rnpix = (ACCU_TYPE)1.0/(ACCU_TYPE)npix;
#endif
            }

            if (npix) {
                out[x + y * nx] = (OUT_TYPE)(RUNSUM_MEAN);
            } else {
                /* Flag as bad - and set to zero */
                if (*pbpm == NULL) *pbpm
                    = (cpl_binary*)cpl_calloc((size_t)nx * (size_t)ny,
                                              sizeof(cpl_binary));
                (*pbpm)[x + y * nx] = CPL_BINARY_1;
                out[x + y * nx] = (OUT_TYPE)0;
            }
        }

        for (; x < nx; x++) {

           /* Update the running sum */
            runsum -= colsum[x - hsizex - 1];
            npix   -= colpix[x - hsizex - 1];

            if (npix) {
                out[x + y * nx] = (OUT_TYPE)(runsum/(ACCU_TYPE)npix);
            } else {
                /* Flag as bad - and set to zero */
                if (*pbpm == NULL) *pbpm
                    = (cpl_binary*)cpl_calloc((size_t)nx * (size_t)ny,
                                              sizeof(cpl_binary));
                (*pbpm)[x + y * nx] = CPL_BINARY_1;
                out[x + y * nx] = (OUT_TYPE)0;
            }
        }
    }

    cpl_free(colsum);
    cpl_free(colpix);

    return;

}


/*----------------------------------------------------------------------------*/
/**
   @internal
   @ingroup cpl_image
   @brief  Linear filter with bad pixels
   @param  vout    The pixel buffer to hold the filtered result
   @param  ppbpm   *ppbpm is the bpm, or NULL if still to be created
   @param  vin     The input pixel buffer to be filtered
   @param  bpm     The input bad pixel map, or NULL if none
   @param  pkernel The kernel
   @param  nx      image width
   @param  ny      image height
   @param  dodiv   CPL_TRUE for division by the kernel absolute weights sum
   @param  rx      filtering half-size in x
   @param  ry      filtering half-size in y
   @return void
   @see cpl_filter_average_slow
   @note FIXME: Modified from cpl_filter_average_slow
   
*/
/*----------------------------------------------------------------------------*/
static cpl_error_code
ADDTYPE_TWO(cpl_filter_linear_slow)(void * vout, cpl_binary ** ppbpm,
                                    const void * vin,
                                    const cpl_binary * bpm,
                                    const double * pkernel,
                                    cpl_size nx, cpl_size ny,
                                    cpl_boolean dodiv,
                                    cpl_size rx, cpl_size ry,
                                    cpl_border_mode mode)
{

    if (mode == CPL_BORDER_FILTER) {
        OUT_TYPE * out = (OUT_TYPE*)vout;
        const IN_TYPE  * in  = (const IN_TYPE*)vin;
        cpl_size   x, y;

        for (y = 0; y < ny; y++) {
            const size_t y_1 = CX_MAX(0,    y - ry);
            const size_t y2  = CX_MIN(ny-1, y + ry);
            for (x = 0; x < nx; x++) {
                const size_t     x1 = CX_MAX(0,    x - rx);
                const size_t     x2 = CX_MIN(nx-1, x + rx);
                const IN_TYPE    * inj  = in + y_1 * nx;
                /* Point to last (bottom) kernel row for first (bottom)
                   image row */
                /* As a difference from cpl_filter_average_slow,
                   element (i,j) in a mask buffer corresponds to
                   element (i, n-1-j) in a matrix buffer. Thus a
                   matrix buffer has to be accessed in reverse row
                   order */
                const double     * pkernelj = pkernel
                    + (rx - x) + (ry + y - y_1) * (2 * rx + 1);
                double             sum    = 0.0;
                double             weight = 0.0;
                /* True iff the filtered pixel has contributing input pixels
                   (ignoring zero-valued weigths) */
                cpl_boolean isok = bpm == NULL;
                size_t i, j;

                if (bpm == NULL) {
                    for (j = y_1; j < 1 + y2; j++, inj += nx,
                             pkernelj -= 2 * rx + 1) {
                        for (i = x1; i < 1 + x2; i++) {
                            sum += pkernelj[i] * (double)inj[i];
                            if (dodiv) weight += fabs(pkernelj[i]);
                        }
                    }
                } else {
                    const cpl_binary * bpmj = bpm + y_1 * nx;

                    for (j = y_1; j < 1 + y2; j++, inj += nx, bpmj += nx,
                             pkernelj -= 2 * rx + 1) {
                        for (i = x1; i < 1 + x2; i++) {
                            if (!bpmj[i]) {
                                sum += pkernelj[i] * (double)inj[i];
                                if (dodiv) weight += fabs(pkernelj[i]);
                                else isok = CPL_TRUE;
                            }
                        }
                    }
                }
                if (dodiv ? weight > 0.0 : isok) {
                    if (dodiv) sum /= weight;
                } else {
                    /* Flag as bad - and set to zero */
                    if (*ppbpm == NULL) {
                        *ppbpm = cpl_calloc((size_t)nx * (size_t)ny,
                                            sizeof(cpl_binary));
                    }
                    (*ppbpm)[x + y * nx] = CPL_BINARY_1;
                }
                out[x + y * nx] = (OUT_TYPE)sum;
            }
        }
    } else {
        /* FIXME: Support more modes ? */
        return cpl_error_set_(CPL_ERROR_UNSUPPORTED_MODE);
    }

    return CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
   @internal
   @ingroup cpl_image
   @brief  Morpho filter with bad pixels
   @param  vout    The pixel buffer to hold the filtered result
   @param  ppbpm   *ppbpm is the bpm, or NULL if still to be created
   @param  vin     The input pixel buffer to be filtered
   @param  bpm     The input bad pixel map, or NULL if none
   @param  pkernel The kernel
   @param  nx      image width
   @param  ny      image height
   @param  dodiv   CPL_TRUE for division by the kernel absolute weights sum
   @param  rx      filtering half-size in x
   @param  ry      filtering half-size in y
   @return void
   @see cpl_filter_average_slow
   @note FIXME: Modified from cpl_filter_linear_slow + cpl_filter_median_slow
   
*/
/*----------------------------------------------------------------------------*/
static cpl_error_code
ADDTYPE_TWO(cpl_filter_morpho_slow)(void * vout, cpl_binary ** ppbpm,
                                    const void * vin,
                                    const cpl_binary * bpm,
                                    const double * pkernel,
                                    cpl_size nx, cpl_size ny,
                                    cpl_boolean dodiv,
                                    cpl_size rx, cpl_size ry,
                                    cpl_border_mode mode)
{

    if (mode == CPL_BORDER_FILTER) {
        OUT_TYPE * out = (OUT_TYPE*)vout;
        const IN_TYPE  * in  = (const IN_TYPE*)vin;
        IN_TYPE window[(2 * rx + 1) * (2 * ry + 1)];
        cpl_size   x, y;

        for (y = 0; y < ny; y++) {
            const size_t y_1 = CX_MAX(0,    y - ry);
            const size_t y2  = CX_MIN(ny-1, y + ry);
            for (x = 0; x < nx; x++) {
                const size_t     x1 = CX_MAX(0,    x - rx);
                const size_t     x2 = CX_MIN(nx-1, x + rx);
                const IN_TYPE    * inj  = in + y_1 * nx;
                double             sum    = 0.0;
                double             weight = 0.0;
                size_t i, j;
                size_t k = 0;

                /* FIXME: In the sorted array, only one row has to be updated
                   for each new pixel, see the fast median algorithm */

                if (bpm == NULL) {
                    for (j = y_1; j < 1 + y2; j++, inj += nx) {
                        for (i = x1; i < 1 + x2; i++) {
                            window[k] = inj[i];
                            if (dodiv) weight += fabs(pkernel[k]);
                            k++;
                        }
                    }
                } else {
                    const cpl_binary * bpmj = bpm + y_1 * nx;

                    for (j = y_1; j < 1 + y2; j++, inj += nx, bpmj += nx) {
                        for (i = x1; i < 1 + x2; i++) {
                            if (!bpmj[i]) {
                                window[k] = inj[i];
                                if (dodiv) weight += fabs(pkernel[k]);
                                k++;
                            }
                        }
                    }
                }
                if (dodiv ? weight > 0.0 : k > 0) {
                    /* Sort array with standard sorting routine */
                    /* assert(k > 0); */
                    ADDTYPE(cpl_tools_sort)(window, k);
                    for (; k--;) {
                        sum += pkernel[k] * (double)window[k];
                    }
                    if (dodiv) sum /= weight;
                } else {
                    /* Flag as bad - and set to zero */
                    if (*ppbpm == NULL) {
                        *ppbpm = cpl_calloc((size_t)nx * (size_t)ny,
                                            sizeof(cpl_binary));
                    }
                    (*ppbpm)[x + y * nx] = CPL_BINARY_1;
                }
                out[x + y * nx] = (OUT_TYPE)sum;
            }
        }
    } else {
        /* FIXME: Support more modes ? */
        return cpl_error_set_(CPL_ERROR_UNSUPPORTED_MODE);
    }

    return CPL_ERROR_NONE;
}

/* @endcond
 */

#undef ACCU_TYPE
#undef ACCU_FLOATTYPE
#undef IN_TYPE
#undef OUT_TYPE
#undef ACCU_TYPE_IS_INT
#undef ALLOW_RECIPROCAL
#undef RUNSUM_MEAN

