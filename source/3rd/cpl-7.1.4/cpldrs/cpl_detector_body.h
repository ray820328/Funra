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


#define CPL_TYPE_ADD(a) CONCAT2X(a, CPL_NAME_TYPE)

static cpl_error_code
CPL_TYPE_ADD(cpl_detector_interpolate_rejected)(CPL_TYPE * pi,
                                                const cpl_binary * bpm,
                                                cpl_size nx, cpl_size ny,
                                                cpl_binary * doit)
    CPL_ATTR_NONNULL;

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Interpolate any bad pixels in an image and clean the bad pixel map
  @param pi   The pixel buffer
  @param bpm  The bad pixel map buffer
  @param nx   X-size of image and bad pixel map
  @param ny   Y-size of image and bad pixel map
  @param doit The first bad pixel to clean
  @return   The #_cpl_error_code_ or CPL_ERROR_NONE
  @see cpl_detector_interpolate_rejected
  @note No input checking in this internal function!
 
  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_DATA_NOT_FOUND if all pixels are bad
 */
/*----------------------------------------------------------------------------*/
static cpl_error_code
CPL_TYPE_ADD(cpl_detector_interpolate_rejected)(CPL_TYPE * pi,
                                                const cpl_binary * bpm,
                                                cpl_size nx, cpl_size ny,
                                                cpl_binary * doit)
{

    /* Need a copy of the bpm, to not use as interpolation source
       pixels that have been interpolated in the same pass */
    cpl_binary       * bpmcopy = cpl_malloc((size_t)(nx * ny) * sizeof(*bpm));
    cpl_mask         * copy = cpl_mask_wrap(nx, ny, bpmcopy);
    const cpl_binary * prev = doit;
    cpl_boolean        ok; /* All pixels may be bad */

    (void)memset(bpmcopy, CPL_BINARY_0, (size_t)(prev - bpm));

    do {
        cpl_binary * found = doit;

        /* assert( *doit ); */

        /* No more bad pixels between previous and current bad pixel */
        (void)memset(bpmcopy + (prev - bpm), CPL_BINARY_0,
                     (size_t)(doit - prev));
        /* Bad pixels may have been cleaned in remaining buffer as well */
        (void)memcpy(bpmcopy + (doit - bpm), doit,
                (size_t)(nx * ny - (doit - bpm)) * sizeof(*bpm));
        prev = doit;
        doit = NULL;

        ok = CPL_FALSE;

        do {
            /* Found bad pixel at (i,j) */
            const cpl_size ij = found - bpm;
            const cpl_size j = ij / nx;
            const cpl_size i = ij - j * nx;

            /* Try to interpolate the bad pixel  */
            cpl_size npix = 0;
            CPL_SUM_TYPE sumpix = 0.0;

            if (i > 0) { /* The three pixels to the left */
                if (bpmcopy[ij - 1] == CPL_BINARY_0) {
                    sumpix += (CPL_SUM_TYPE)pi[ij - 1];
                    npix++;
                }
                if (j > 0 && bpmcopy[ij - 1 - nx] == CPL_BINARY_0) {
                    sumpix += (CPL_SUM_TYPE)pi[ij - 1 - nx];
                    npix++;
                }
                if (j + 1 < ny && bpmcopy[ij - 1 + nx] == CPL_BINARY_0) {
                    sumpix += (CPL_SUM_TYPE)pi[ij - 1 + nx];
                    npix++;
                }
            }

            /* The two pixels below and above */
            if (j > 0 && bpmcopy[ij - nx] == CPL_BINARY_0) {
                sumpix += (CPL_SUM_TYPE)pi[ij - nx];
                npix++;
            }
            if (j + 1 < ny && bpmcopy[ij + nx] == CPL_BINARY_0) {
                sumpix += (CPL_SUM_TYPE)pi[ij  + nx];
                npix++;
            }

            if (i + 1 < nx) { /* The three pixels to the right */
                if (bpmcopy[ij + 1] == CPL_BINARY_0) {
                    sumpix += (CPL_SUM_TYPE)pi[ij + 1];
                    npix++;
                }
                if (j > 0 && bpmcopy[ij + 1 - nx] == CPL_BINARY_0) {
                    sumpix += (CPL_SUM_TYPE)pi[ij + 1 - nx];
                    npix++;
                }
                if (j + 1 < ny && bpmcopy[ij + 1 + nx] == CPL_BINARY_0) {
                    sumpix += (CPL_SUM_TYPE)pi[ij + 1 + nx];
                    npix++;
                }
            }
                       
            if (npix > 0) {
                /* Found source pixel(s) for interpolation */
#ifdef CPL_DO_ROUND
                /* For integers round halfway cases away from zero */
                sumpix += sumpix < 0.0 ? -0.5 : 0.5;
#endif
                pi[ij] = (CPL_TYPE)(sumpix / (CPL_SUM_TYPE)npix);
                *found = CPL_BINARY_0;
                ok = CPL_TRUE;
            } else if (doit == NULL) {
                doit = found; /* First bad pixel to redo */
            }

            found = memchr(found+1, CPL_BINARY_1,
                           (size_t)(nx * ny - ij - 1) * sizeof(*bpm));

        } while (found);
    } while (doit && ok);

    cpl_mask_delete(copy);

    return ok ? CPL_ERROR_NONE : cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);

}

#undef CPL_TYPE_ADD
