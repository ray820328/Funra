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
#define CPL_TYPE_INTERPOLATE cpl_image_get_interpolated_double

#elif CPL_CLASS == CPL_CLASS_FLOAT
#define CPL_TYPE float
#define CPL_TYPE_T CPL_TYPE_FLOAT
#define CPL_TYPE_INTERPOLATE cpl_image_get_interpolated_float

#elif CPL_CLASS == CPL_CLASS_INT
#define CPL_TYPE int
#define CPL_TYPE_T CPL_TYPE_INT
#define CPL_TYPE_INTERPOLATE cpl_image_get_interpolated_int

#else
#undef CPL_TYPE
#undef CPL_TYPE_T
#define CPL_TYPE_INTERPOLATE
#endif

#if defined CPL_OPERATION && CPL_OPERATION == CPL_IMAGE_RESAMPLE_SUBSAMPLE

case CPL_TYPE_T:
{
    const CPL_TYPE *pi;
    CPL_TYPE       *po;


    out_im = cpl_image_new(new_nx, new_ny, CPL_TYPE_T);
    pi = (CPL_TYPE*)image->pixels;
    po = (CPL_TYPE*)out_im->pixels;

    for (j = 0; j < image->ny; j += ystep, pi += ystep*image->nx)
        for (i = 0; i < image->nx; i += xstep)
            *po++ = pi[i];
}
break;

#elif defined CPL_OPERATION && CPL_OPERATION == CPL_IMAGE_REBIN

case CPL_TYPE_T:
{
    const CPL_TYPE *pi;
    CPL_TYPE       *po;


    out_im = cpl_image_new(new_nx, new_ny, CPL_TYPE_T);
    pi = (CPL_TYPE*)image->pixels;
    po = (CPL_TYPE*)out_im->pixels;

    for (j = ystart - 1; j < ny; j++) {
        for (i = xstart - 1; i < nx; i++) {
            po[(i-xstart+1)/xstep + ((j-ystart+1)/ystep)*new_nx] 
            += pi[i + j*old_nx];
        }
    }
}
break;

#elif defined CPL_OPERATION && CPL_OPERATION == CPL_IMAGE_WARP

case CPL_TYPE_T:

    /* Double loop on the output image  */
    for (cpl_size j = 0; j < out->ny; j++) {
        for (cpl_size i = 0; i < out->nx; i++) {

            /* Compute the original source for this pixel   */
            const cpl_size pos = i + j * out->nx;
            const double   x   = (double)(i+1) - pdeltax[pos];
            const double   y   = (double)(j+1) - pdeltay[pos];
            double         confidence;

            /* Compute the new value */
            const double value = CPL_TYPE_INTERPOLATE
                ((const CPL_TYPE*)in->pixels, inbpm,
                 in->nx, in->ny,
                 xtabsperpix, ytabsperpix, x, y,
                 yweight, pxprof, xradius, pyprof,
                 yradius, sqyradius, sqyxratio, &confidence);

            if (confidence > 0.0)
                cpl_image_set_(out, i+1, j+1, value);
            else {
                pbad[i + j * out->nx] = CPL_BINARY_1;
                hasbad = 1;
            }
        }
    }
break;

#elif defined CPL_OPERATION && CPL_OPERATION == CPL_IMAGE_WARP_POLYNOMIAL
case CPL_TYPE_T:

/* Double loop on the output image  */
for (j=0; j < out->ny; j++) {
    pval[1] = (double)(j+1);
    for (i=0; i < out->nx; i++) {
        pval[0] = (double)(i+1);

        /* The CPL-calls here cannot fail */

        /* Compute the original source for this pixel   */

        x = cpl_polynomial_eval(poly_x, val);
        y = cpl_polynomial_eval(poly_y, val);


        value = CPL_TYPE_INTERPOLATE
            ((const CPL_TYPE*)in->pixels,
             inbpm,
             in->nx, in->ny,
             xtabsperpix,
             ytabsperpix,
             x, y,
             yweight,
             pxprof,
             xradius,
             pyprof,
             yradius,
             sqyradius,
             sqyxratio,
             &confidence);


        if (confidence > 0)
            cpl_image_set_(out, i+1, j+1, value);
        else {
            pbad[i + j * out->nx] = CPL_BINARY_1;
            hasbad = 1;
        }
    }
}

break;

#else

#ifdef ADDTYPE
#undef ADDTYPE
#endif
#define ADDTYPE(a) CONCAT2X(a,CPL_TYPE)

/*----------------------------------------------------------------------------*/
/**
  @brief    Interpolate a pixel 
  @see cpl_image_get_interpolated()

  For efficiency reasons the caller must ensure that all arguments are valid.
  Specifically, this function must be called with an image of the correct pixel
  type.

*/
/*----------------------------------------------------------------------------*/
inline static double
ADDTYPE(cpl_image_get_interpolated)(const CPL_TYPE * pixel,
                                    const cpl_binary * bpm,
                                    cpl_size nx,
                                    cpl_size ny,
                                    double xtabsperpix,
                                    double ytabsperpix,
                                    double xpos, double ypos,
                                    double * yweight,
                                    const double * pxprof,
                                    double xradius,
                                    const double * pyprof,
                                    double yradius,
                                    double sqyradius,
                                    double sqyxratio,
                                    double * pconfid)
{
    double sum   = 0.0;
    double wsum  = 0.0;
    double wconf = 0.0;
    double wabs  = 0.0;

    /* These image positions can become negative */
    const intptr_t xfirst = (intptr_t)ceil (xpos - xradius);
    const intptr_t xlast  = (intptr_t)floor(xpos + xradius);
    const intptr_t yfirst = (intptr_t)ceil (ypos - yradius);
    const intptr_t ylast  = (intptr_t)floor(ypos + yradius);
#ifdef CPL_ADD_FLOPS
    cpl_flops skipped = 0;
#endif

#if 0
    assert( nx > 0 );
    assert( ny > 0 );
    assert( xradius > 0 );
    assert( yradius > 0 );
#endif

    if (xfirst > xlast || yfirst > ylast) {
        /* For sufficiently small radii no pixels can be reached */
        *pconfid = 0;
        return 0.0;
    }

    /* Generate weights for all points in the Y-direction */
    for (intptr_t y = yfirst; y <= ylast; y++)
        yweight[(size_t)(y-yfirst)] = pyprof[(size_t)(fabs((double)y - ypos)
                                                      * ytabsperpix+0.5)];

    /* Iterate through all the points in the X-direction */
    for (intptr_t x = xfirst; x <= xlast; x++) {
        const double xdel = (double)x - xpos;
        const double sqrest = sqyradius - xdel*xdel*sqyxratio;
        const double xweight = pxprof[(size_t)(fabs(xdel) * xtabsperpix+0.5)];

        /* Iterate through all the points in the Y-direction */
        for (intptr_t y = yfirst; y <= ylast; y++) {
            const double ydel = (double)y - ypos;
            double value, weight;

            if (ydel*ydel > sqrest) {
#ifdef CPL_ADD_FLOPS
                skipped += 8;
#endif
                continue;
            }
            /* (x,y) is within the ellipse with semi-major and -minor
               (xradius, yradius) at (xpos, ypos) */

            /* The pixel weight is the product of the two 1D-profiles */
            weight = xweight * yweight[(size_t)(y-yfirst)];

            /* Sum of all absolute weights in the inclusion area */
            wconf += fabs(weight);

            if (x < 1 || y < 1 || x > (intptr_t)nx || y > (intptr_t)ny) {
#ifdef CPL_ADD_FLOPS
                skipped += 5;
#endif
                continue;
            }

            if (bpm != NULL &&
                bpm[(size_t)((x-1) + (y-1) * nx)] != CPL_BINARY_0) {
#ifdef CPL_ADD_FLOPS
                skipped += 5;
#endif
                continue;
            }

            value = pixel[(size_t)((x-1) + (y-1) * nx)];

            /* Sum of weigths actually used */
            wsum += weight;
            /* Sum of absolute weigths actually used */
            wabs += fabs(weight);
            /* Weighted pixel sum */
            sum += value * weight;

        }
    }

    /* The confidence is the ratio of the absolute weights used over the sum
       of all absolute weights inside the inclusion area */
    *pconfid = wconf > 0 ? wabs / wconf : 0;

#ifdef CPL_ADD_FLOPS
    cpl_tools_add_flops( 3 * ( ylast - yfirst + 1)
                       + 6 * ( xlast - xfirst + 1)
                       + 11* ( ylast - yfirst + 1)
                           * ( xlast - xfirst + 1) - skipped );
#endif

    return wsum > 0.0 ? sum / wsum : 0.0;
}

#endif

#undef CPL_TYPE
#undef CPL_TYPE_T
#undef CPL_TYPE_INTERPOLATE
