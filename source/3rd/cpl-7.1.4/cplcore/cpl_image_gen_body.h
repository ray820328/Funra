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
#define CPL_TYPE_IS_COMPLEX FALSE

#elif CPL_CLASS == CPL_CLASS_FLOAT
#define CPL_TYPE float
#define CPL_TYPE_T CPL_TYPE_FLOAT
#define CPL_TYPE_IS_COMPLEX FALSE

#elif CPL_CLASS == CPL_CLASS_INT
#define CPL_TYPE int
#define CPL_TYPE_T CPL_TYPE_INT
#define CPL_TYPE_IS_COMPLEX FALSE

#elif CPL_CLASS == CPL_CLASS_DOUBLE_COMPLEX
#define CPL_TYPE double complex
#define CPL_TYPE_T CPL_TYPE_DOUBLE_COMPLEX
#define CPL_TYPE_IS_COMPLEX TRUE

#elif CPL_CLASS == CPL_CLASS_FLOAT_COMPLEX
#define CPL_TYPE float complex
#define CPL_TYPE_T CPL_TYPE_FLOAT_COMPLEX
#define CPL_TYPE_IS_COMPLEX TRUE

#else
#undef CPL_TYPE
#undef CPL_TYPE_T
#undef CPL_TYPE_IS_COMPLEX
#endif

#if CPL_OPERATION == CPL_IMAGE_GEN_NOISE_UNIFORM && !CPL_TYPE_IS_COMPLEX

    case CPL_TYPE_T:
    {
        CPL_TYPE    *   pi;
        pi = (CPL_TYPE*)ima->pixels;
        
        /* Compute the pixels values */
        for (j=0; j<ima->ny; j++)
            for (i=0; i<ima->nx; i++)
                pi[i+j*ima->nx] = 
                    (CPL_TYPE)(min_pix + cpl_drand()*(max_pix-min_pix));
        break;
    }

#elif CPL_OPERATION == CPL_IMAGE_GEN_NOISE_UNIFORM && CPL_TYPE_IS_COMPLEX

    case CPL_TYPE_T:
    {
        CPL_TYPE    *   pi;
        pi = (CPL_TYPE*)ima->pixels;
        
        /* Compute the pixels values */
        for (j=0; j<ima->ny; j++)
            for (i=0; i<ima->nx; i++)
                pi[i+j*ima->nx] = (CPL_TYPE)
                    (
#ifdef CPL_I
                     /* The complex image will only have complex values
                        when those are supported */
                     (min_pix + cpl_drand() * (max_pix - min_pix)) * CPL_I +
#endif
                     (min_pix + cpl_drand() * (max_pix - min_pix)));
        break;
    }

#elif CPL_OPERATION == CPL_IMAGE_GEN_GAUSSIAN
    
    case CPL_TYPE_T:
    {
        CPL_TYPE    *   pi;
        pi = (CPL_TYPE*)ima->pixels;
        
        /* Compute the pixels values */
        for (j=0; j<ima->ny; j++) {
            for (i=0; i<ima->nx; i++) {
                x = (double)(i+1) - (double)xcen;
                y = (double)(j+1) - (double)ycen;
                pi[i+j*ima->nx]=(CPL_TYPE)cpl_gaussian_2d(x,y,norm,sig_x,sig_y);
            }
        }
        break;
    }
        
#elif CPL_OPERATION == CPL_IMAGE_GEN_POLYNOMIAL
    
    case CPL_TYPE_T:
    {
        CPL_TYPE    *   pi;
        pi = (CPL_TYPE*)ima->pixels;
    
        /* Compute the pixels values */
        x = cpl_vector_new(2);
        xdata = cpl_vector_get_data(x);
        for (j=0; j<ima->ny; j++) {
            for (i=0; i<ima->nx; i++) {
                xdata[0] = (double)(startx + i * stepx);
                xdata[1] = (double)(starty + j * stepy);
                pi[i+j*ima->nx] = (CPL_TYPE)cpl_polynomial_eval(poly, x);
            }
        }
        cpl_vector_delete(x);
        break;
    }

#endif

#undef CPL_TYPE
#undef CPL_TYPE_T
#undef CPL_TYPE_IS_COMPLEX
