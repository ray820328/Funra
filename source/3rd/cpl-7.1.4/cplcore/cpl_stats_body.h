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
#define CPL_ADD_FLOPS_ADD cpl_tools_add_flops

#elif CPL_CLASS == CPL_CLASS_FLOAT
#define CPL_TYPE float
#define CPL_TYPE_T CPL_TYPE_FLOAT
#define CPL_ADD_FLOPS_ADD cpl_tools_add_flops

#elif CPL_CLASS == CPL_CLASS_INT
#define CPL_TYPE int
#define CPL_TYPE_T CPL_TYPE_INT
#define CPL_ADD_FLOPS_ADD(N) /* N integer ops */

#else
#undef CPL_TYPE
#undef CPL_TYPE_T
#endif

#define CPL_TYPE_ADD(a) CPL_CONCAT2X(a, CPL_TYPE)

#if CPL_OPERATION == CPL_IMAGE_STATS_ALL

  case CPL_TYPE_T:
  {
      const CPL_TYPE * pi = (const CPL_TYPE*)image->pixels;
 
      min_pos = max_pos = firstgoodpos;
      min_pix = (double)pi[firstgoodpos];
      max_pix = (double)pi[firstgoodpos];

      for (j=llysz-1; j<urysz; j++) {
          pos = (llxsz-1)+j*image->nx;

          for (i=llxsz-1; i<urxsz; i++) {
              if (nbadpix == 0 || !badmap[pos]) {
                  const double delta = (double)pi[pos] - pix_mean;

                  pix_var  += ipix * delta * (delta / (ipix + 1.0));
                  pix_mean += delta / (ipix + 1.0);
                  ipix += 1.0;

                  if (pi[pos] < min_pix) min_pix = (double)pi[min_pos = pos];
                  if (pi[pos] > max_pix) max_pix = (double)pi[max_pos = pos];

                  pix_sum += (double)pi[pos];
                  abs_sum += fabs((double)pi[pos]);
                  dev_sum += fabs((double)pi[pos]-self->med);
                  sqr_sum += (double)pi[pos] * (double)pi[pos];

              }
              pos++;
          }
      }
      CPL_ADD_FLOPS_ADD(17 * npix);
      break;
  }

#elif CPL_OPERATION == CPL_IMAGE_STATS_VARIANCE

  case CPL_TYPE_T:
  {
      const CPL_TYPE * pi = (const CPL_TYPE*)image->pixels;

      for (j=llysz-1; j<urysz; j++) {
          pos = (llxsz-1)+j*image->nx;
          for (i=llxsz-1; i<urxsz; i++) {
              if (nbadpix == 0 || !badmap[pos]) {
                  const double delta = (double)pi[pos] - pix_mean;

                  /* The round-off on pix_mean may be different here
                     and in the all-stats block :-( */
                  pix_var  += ipix * delta * (delta / (ipix + 1.0));
                  pix_mean += delta / (ipix + 1.0);
                  ipix += 1.0;

              }
              pos++;
          }
      }
      CPL_ADD_FLOPS_ADD(7 * npix);
      break;
  }

#elif CPL_OPERATION == CPL_IMAGE_STATS_CENTROID

  case CPL_TYPE_T:
  {
      const CPL_TYPE * pi = (const CPL_TYPE*)image->pixels;
      const double min_pix_tmp = min_pix < 0.0 ? min_pix : 0.0;

      for (j=llysz-1; j<urysz; j++) {
          pos = (llxsz-1)+j*image->nx;
          for (i=llxsz-1; i<urxsz; i++) {
              if (nbadpix == 0 || !badmap[pos]) {
                  sum_xz += ((double)pi[pos]-min_pix_tmp)*(double)(i+1);
                  sum_yz += ((double)pi[pos]-min_pix_tmp)*(double)(j+1);
                  sum_z += (double)pi[pos]-min_pix_tmp;
                  sum_x  += (double)(i+1);
                  sum_y  += (double)(j+1);
              }
              pos++;
          }
      }
      if (sum_z  < 0) sum_z  = 0; /* Can only become negative due to rounding */
      if (sum_xz < 0) sum_xz = 0; /* Can only become negative due to rounding */
      if (sum_yz < 0) sum_yz = 0; /* Can only become negative due to rounding */
      CPL_ADD_FLOPS_ADD(8 * npix);
      break;
  }

#elif CPL_OPERATION == CPL_IMAGE_STATS_MINMAX

  case CPL_TYPE_T:
  {
      const CPL_TYPE * pi = (const CPL_TYPE*)image->pixels;
      /* Avoid in-loop casting */
      CPL_TYPE min_tmp = pi[firstgoodpos];
      CPL_TYPE max_tmp = pi[firstgoodpos];

      min_pos = max_pos = firstgoodpos;
      for (j=llysz-1; j<urysz; j++) {
          pos = (llxsz-1)+j*image->nx;

          for (i=llxsz-1; i<urxsz; i++) {
              if (nbadpix == 0 || !badmap[pos]) {
                 if (pi[pos] < min_tmp) min_tmp = pi[min_pos = pos];
                 if (pi[pos] > max_tmp) max_tmp = pi[max_pos = pos];
              }
              pos++;
          }
      }
      min_pix = (double)min_tmp;
      max_pix = (double)max_tmp;
      CPL_ADD_FLOPS_ADD(2 * npix);
      break;
  }

#elif CPL_OPERATION == CPL_IMAGE_STATS_FLUX

  case CPL_TYPE_T:
  {
      const CPL_TYPE * pi = (const CPL_TYPE*)image->pixels;

      for (j=llysz-1; j<urysz; j++) {
          pos = (llxsz-1)+j*image->nx;

          for (i=llxsz-1; i<urxsz; i++) {
              if (nbadpix == 0 || !badmap[pos]) {
                  pix_sum += (double)pi[pos];
                  abs_sum += fabs((double)pi[pos]);
                  sqr_sum += (double)pi[pos] * (double)pi[pos];
              }
              pos++;
          }
      }

      CPL_ADD_FLOPS_ADD(5 * npix);
      break;
  }

#elif CPL_OPERATION == CPL_IMAGE_STATS_MEDIAN

    case CPL_TYPE_T:
    {
        CPL_TYPE * copybuft = (CPL_TYPE *)cpl_ifalloc_get(&copybuf);
        if (nbadpix == 0) {

            /* All pixels are good */

            /* Cannot fail here */
            (void)cpl_tools_copy_window((void*)copybuft,
                                        image->pixels,
                                        sizeof(CPL_TYPE),
                                        image->nx, image->ny,
                                        llxsz, llysz, urxsz, urysz);

        } else {
            /* Point to first pixel in first row to read */
            const CPL_TYPE   * pi = (const CPL_TYPE*)image->pixels
                + (llysz-1)*image->nx;

            /* - ditto for bad pixel map */
            const cpl_binary * pbpm = badmap + (llysz-1)*image->nx;
            cpl_size ngood = 0;

            for (j = llysz - 1; j < urysz; j++, pi += image->nx, pbpm += image->nx) {
                for (i = llxsz - 1; i < urxsz; i++) {
                    /* Take only good pixels */
                    if (pbpm[i] == CPL_BINARY_0) {
                        copybuft[ngood++] = pi[i];
                    }
                }
            }
            /* assert( ngood == npix ) */
        }

        /* Compute the median */
        self->med = CPL_TYPE_ADD(cpl_tools_get_median)(copybuft, npix);

        break;
    }

#elif CPL_OPERATION == CPL_IMAGE_STATS_MEDIAN_DEV

  case CPL_TYPE_T:
  {

      /*   Could be done in two FLOPs instead of three
       *    - but this would lead to more complicated code...
       */

      if (nbadpix != 0) {
          /* Need to check bad pixel buffer */

            /* Point to first pixel in first row to read */
            const CPL_TYPE   * pi = (const CPL_TYPE*)image->pixels
                + (llysz-1)*image->nx;

            /* - ditto for bad pixel map */
            const cpl_binary * pbpm = badmap + (llysz-1)*image->nx;

            for (j = llysz - 1; j < urysz; j++, pi += image->nx, pbpm += image->nx) {
                for (i = llxsz - 1; i < urxsz; i++) {
                    /* Take only good pixels */
                    if (pbpm[i] == CPL_BINARY_0) {
                        dev_sum += fabs((double)pi[i]-self->med);
                    }
                }
            }
      } else {
            /* Point to first pixel in first row to read */
            const CPL_TYPE   * pi = (const CPL_TYPE*)image->pixels
                + (llysz-1)*image->nx;

            for (j = llysz - 1; j < urysz; j++, pi += image->nx) {
                for (i = llxsz - 1; i < urxsz; i++) {
                    dev_sum += fabs((double)pi[i]-self->med);
                }
            }
      }

      cpl_tools_add_flops( 3 * npix + 1 );

      break;
  }

#elif CPL_OPERATION == CPL_IMAGE_STATS_MAD

  case CPL_TYPE_T:
  {
      CPL_TYPE * copybuft = (CPL_TYPE *)cpl_ifalloc_get(&copybuf);
      cpl_size   ngood   = 0;


      if (nbadpix != 0) {
          /* Need to check bad pixel buffer */

            /* Point to first pixel in first row to read */
            const CPL_TYPE   * pi = (const CPL_TYPE*)image->pixels
                + (llysz-1)*image->nx;

            /* - ditto for bad pixel map */
            const cpl_binary * pbpm = badmap + (llysz-1)*image->nx;

            for (j = llysz - 1; j < urysz; j++, pi += image->nx, pbpm += image->nx) {
                for (i = llxsz - 1; i < urxsz; i++) {
                    /* Take only good pixels */
                    if (pbpm[i] == CPL_BINARY_0) {
                        copybuft[ngood++] = fabs((double)pi[i]-self->med);
                    }
                }
            }
      } else {
            /* Point to first pixel in first row to read */
            const CPL_TYPE   * pi = (const CPL_TYPE*)image->pixels
                + (llysz-1)*image->nx;

            for (j = llysz - 1; j < urysz; j++, pi += image->nx) {
                for (i = llxsz - 1; i < urxsz; i++) {
                    copybuft[ngood++] = fabs((double)pi[i]-self->med);
                }
            }
      }
      /* assert( ngood == npix ) */

      cpl_tools_add_flops( 2 * npix );

      /* Compute the median */
      self->mad = CPL_TYPE_ADD(cpl_tools_get_median)(copybuft, npix);

      break;
  }

#endif

#undef CPL_TYPE
#undef CPL_TYPE_T
#undef CPL_ADD_FLOPS_ADD

#undef CPL_TYPE_ADD
