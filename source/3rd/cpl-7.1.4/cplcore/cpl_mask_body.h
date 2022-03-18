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


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Apply the erosion/dilation filter
  @param  self    The output binary 2D array to hold the filtered result
  @param  other   The input  binary 2D array to filter
  @param  kernel  The input  binary 2D kernel - rows padded with 0 to fit word
  @param  nx      The X-size of the input array
  @param  ny      The Y-size of the input array
  @param  hx      The X-half-size of the kernel
  @param  hy      The Y-half-size of the kernel
  @return void
  @note No error checking in this internal function!

 */
/*----------------------------------------------------------------------------*/
static void
APPENDSIZE(OPERATION, HX, HY)(cpl_binary * self, const cpl_binary * other,
                              const cpl_binary * kernel, cpl_size nx,
                              cpl_size ny, size_t hx, size_t hy,
                              cpl_border_mode border)
{
    if (border == CPL_BORDER_NOP || border == CPL_BORDER_ZERO ||
        border == CPL_BORDER_COPY) {
        const size_t mx = 2 * hx + 1;
        /* kernel rows are padded to a multiple of four bytes so each
           instruction can process four bytes. This will lead to either a 1 or
           3 bytes out of bounds access. So process the last element(s) of the
           last row(s) one byte at a time. */
        const size_t   mxe   = 1 + (mx | CPL_MASK_PAD);
        const size_t   mxout = mxe - mx; /* 1 or 3 out-of-bounds elements */
        /* Number of out-of-bounds rows */
        const cpl_size myout = 1 + (cpl_size)(mxout-1)/nx;
        /* Last row w/o out-of-bounds rows */
        const cpl_size jstop = ny - (cpl_size)hy - myout;
        cpl_size i, j;

#ifdef GENERAL_CASE
        const cpl_size mxew  = mxe/CPL_MASK_WORD;
#else
        const size_t * kernelw = (const size_t *)kernel;
        hy = HY; /* Help compiler to optimize */
#if     HX == 2
#if     CPL_MASK_WORD == 4
        assert( hx <= 3);
        assert( hx >= 2 );
#else
        assert( hx <= 7);
        assert( hx >= 4 );
#endif
#else
#if     HX == 1
#if     CPL_MASK_WORD == 4
        assert( hx <= 1);
#else
        assert( hx <= 3);
#endif
#endif
#endif
#endif

        /* Handle border for first hy rows and first hx elements of next row */
        if (border == CPL_BORDER_ZERO) {
            (void)memset(self, CPL_BINARY_0, hx + hy * (size_t)nx);
        } else if (border == CPL_BORDER_COPY) {
            (void)memcpy(self, other, hx + hy * (size_t)nx);
        }

        self  += hy * nx;  /* self-col now indexed from -hy to hy */
        other -= hx;       /* other-row now indexed from hx */

        for (j = (cpl_size)hy; j < ny-(cpl_size)hy;
             j++, self += nx, other += nx) {
            /* The last row(s) can only do a multiple of 4 elements */
            const cpl_size istop = j < jstop ? nx - (cpl_size)hx
                : nx - (cpl_size)mxe - (cpl_size)hx;

            if (j > (cpl_size)hy) {
                /* Do also last hx border elements of previous row */
                if (border == CPL_BORDER_ZERO) {
                    (void)memset(self - hx, CPL_BINARY_0, 2 * hx);
                } else if (border == CPL_BORDER_COPY) {
                    (void)memcpy(self - hx, other + hy * (size_t)nx,
                                 2 * hx);
                }
            }

            for (i = (cpl_size)hx; i < istop; i++) {
                CPL_MASK_FILTER_WORD;
            }

            /* The last elements one byte at a time :-( */
            for (; i < nx - (cpl_size)hx; i++) {
                const cpl_binary * otheri  = other + i;
                const cpl_binary * kernelk = kernel;
                size_t k, l;

                for (k = 0; k < 1 + 2 * hy; k++, otheri += nx, kernelk += mxe) {
                    for (l = 0; l < 1 + 2 * hx; l++) {
                        if (OPERATE_PIXEL(otheri[l]) && kernelk[l]) break;
                    }
                    if (l < 1 + 2 * hx) break;
                }
                self[i] = k < 1 + 2 * hy ? VALUE_TRUE : VALUE_FALSE;
            }
        }

        /* Do also last hx border elements of previous row */
        if (border == CPL_BORDER_ZERO) {
            (void)memset(self - hx, CPL_BINARY_0, hx + hy * (size_t)nx);
        } else if (border == CPL_BORDER_COPY) {
            (void)memcpy(self - hx, other + hy * (size_t)nx,
                         hx + hy * (size_t)nx);
        }
    }
}
