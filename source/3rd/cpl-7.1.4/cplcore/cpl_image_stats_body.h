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

/* Type dependent macros */
#if CPL_CLASS == CPL_CLASS_DOUBLE
#define CPL_TYPE double
#define CPL_TYPE_T CPL_TYPE_DOUBLE

#elif CPL_CLASS == CPL_CLASS_FLOAT
#define CPL_TYPE float
#define CPL_TYPE_T CPL_TYPE_FLOAT

#elif CPL_CLASS == CPL_CLASS_INT
#define CPL_TYPE int
#define CPL_TYPE_T CPL_TYPE_INT

#else
#undef CPL_TYPE
#undef CPL_TYPE_T
#endif

#define CPL_TYPE_ADD(a) CPL_CONCAT2X(a, CPL_TYPE)

#if CPL_OPERATION == CPL_IMAGE_STATS_MEDIAN_STAT

  case CPL_TYPE_T:
  {
      const CPL_TYPE * pi = (const CPL_TYPE*)image->pixels;
      int              npix = image->nx * image->ny;
      int              i;

      *sigma = 0.0;

      /*   Could be done in two FLOPs instead of three
       *    - but this would lead to more complicated code...
       */

      if (image->bpm != NULL) {
          /* Need to check bad pixel buffer */
          const cpl_binary * badmap = cpl_mask_get_data_const(image->bpm);
          int                nbad = 0;

          for (i = 0; i < npix; i++) {
              if (badmap[i]) {
                  nbad++;
              } else {
                  *sigma += fabs((double)pi[i]-median_val);
              }
          }

          /* Subtract the number of bad pixels */
          npix -= nbad;

          /* assert( npix > 0 ); */

      } else {
          for (i = 0; i < npix; i++) {
              *sigma += fabs((double)pi[i]-median_val);
          }
      }

      *sigma /= (double) npix;

      cpl_tools_add_flops( 3 * npix + 1 );

      break;
  }


#endif

#undef CPL_TYPE
#undef CPL_TYPE_T

#undef CPL_TYPE_ADD
