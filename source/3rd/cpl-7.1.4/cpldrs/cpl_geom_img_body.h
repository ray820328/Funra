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

#define CPL_TYPE_ADD(a) CPL_CONCAT2X(a, CPL_TYPE)
#define CPL_TYPE_ADD_CONST(a) CPL_CONCAT3X(a, CPL_TYPE, const)

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Correlate two image windows 
  @param    correl Buffer to hold the correlations (1 + 2s_hx)(1 + 2s_hy)
  @param    im1    First input image
  @param    im2    Second input image of same size and type
  @param    x_1    X-coordinate of the subwindow center inside im1 (0 for 1st)
  @param    y_1    Y-coordinate of the subwindow center inside im1
  @param    x_2    X-coordinate of the subwindow center inside im2
  @param    y_2    Y-coordinate of the subwindow center inside im2
  @param    s_hx   Search area half-width.
  @param    s_hy   Search area half-height.
  @param    m_hx   Measurement area half-width.
  @param    m_hy   Measurement area half-height.
  @param    pk_min Optimum correlation X-offset
  @param    pl_min Optimum correlation Y-offset
  @see  cpl_geom_ima_offset_xcorr_subw()
  @note The "correlation" is actually a sum of squared differences per sampled
        number of pixels! Bad pixel maps ignored! No input validation!

 */
/*----------------------------------------------------------------------------*/
static
void CPL_TYPE_ADD(cpl_geom_ima_offset_xcorr_subw)(double          * correl,
                                                  const cpl_image * im1,
                                                  const cpl_image * im2,
                                                  cpl_size          x_1,
                                                  cpl_size          y_1,
                                                  cpl_size          x_2,
                                                  cpl_size          y_2,
                                                  cpl_size          s_hx,
                                                  cpl_size          s_hy,
                                                  cpl_size          m_hx,
                                                  cpl_size          m_hy,
                                                  cpl_size        * pk_min,
                                                  cpl_size        * pl_min)

{
    double           sqsum_min = DBL_MAX;
    const cpl_size   nx = cpl_image_get_size_x(im1);
    const cpl_size   ny = cpl_image_get_size_y(im1);
    /* Point to the center corellation position */
    const CPL_TYPE * pi1 = CPL_TYPE_ADD_CONST(cpl_image_get_data)(im1)
        + x_1 + y_1 * nx;
    const CPL_TYPE * pi2 = CPL_TYPE_ADD_CONST(cpl_image_get_data)(im2)
        + x_2 + y_2 * nx;
    double         * pxc = correl + s_hx; /* Offset for indexing with k */
    const cpl_size   s_hx_min = -CX_MIN(s_hx, CX_MIN(x_1, x_2));
    const cpl_size   s_hy_min = -CX_MIN(s_hy, CX_MIN(y_1, y_2));
    const cpl_size   s_hx_max =  CX_MIN(s_hx, nx-1-CX_MAX(x_1, x_2));
    const cpl_size   s_hy_max =  CX_MIN(s_hy, ny-1-CX_MAX(y_1, y_2));
    cpl_size         k, l;


    /* Outside the image(s) */
    for (l = -s_hy; l < s_hy_min; l++, pxc += 1 + 2 * s_hx) {
        for (k = -s_hx; k <= s_hx; k++) {
            pxc[k] = -1.0;
        }
    }

    /* Compute the sums of squared differences and keep the minimum one */
    for (; l <= s_hy_max; l++, pxc += 1 + 2 * s_hx) {
        /* Outside the image(s) */
        for (k = -s_hx; k < s_hx_min; k++) {
            pxc[k] = -1.0;
        }
        for (; k <= s_hx_max; k++) {
            const cpl_size m_hx_min = -CX_MIN(m_hx, CX_MIN(x_1 + k, x_2));
            const cpl_size m_hy_min = -CX_MIN(m_hy, CX_MIN(y_1 + l, y_2));
            const cpl_size m_hx_max =  CX_MIN(m_hx, nx-1-CX_MAX(x_1 + k, x_2));
            const cpl_size m_hy_max =  CX_MIN(m_hy, ny-1-CX_MAX(y_1 + l, y_2));
            const cpl_size npix = (1+m_hx_max-m_hx_min) * (1+m_hy_max-m_hy_min);

            if (npix < m_hx + m_hy) {
                /* FIXME: Arbitrary lower limit on sampled pixels */
                pxc[k] = -1.0;
            } else {
                double           sqsum    = 0.0;
                const double     rnpix = 1.0 / (double)npix;
                const CPL_TYPE * pos1 = pi1 + k + (l + m_hy_min) * nx;
                const CPL_TYPE * pos2 = pi2     + (    m_hy_min) * nx;
                cpl_size         i, j;

                /* One sum of squared differences */
                for (j = m_hy_min; j <= m_hy_max; j++, pos1 += nx, pos2 += nx) {
                    for (i = m_hx_min; i <= m_hx_max; i++) {
                        const double diff = (double)pos1[i]-(double)pos2[i];
                        sqsum += diff * diff;
                    }
                }

                sqsum *= rnpix;
                pxc[k] = sqsum;

                /* Keep track of the minimum */
                if (sqsum < sqsum_min) {
                    *pl_min = l;
                    *pk_min = k;
                    sqsum_min = sqsum;
                }

                cpl_tools_add_flops(1 + 3 * npix);
            }
        }
        /* Outside the image(s) */
        for (; k <= s_hx; k++) {
            pxc[k] = -1.0;
        }
    }

    /* Outside the image(s) */
    for (; l <= s_hy; l++, pxc += 1 + 2 * s_hx) {
        for (k = -s_hx; k <= s_hx; k++) {
            pxc[k] = -1.0;
        }
    }

    return;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Shift and add an images list to a single image 
  @param    final      Combined image, assumed to be zero
  @param    contrib    Contribution map, assumed to be zero
  @param    rmin       Number of minimum value pixels to reject in stacking
  @param    rmax       Number of maximum value pixels to reject in stacking
  @param    start_x    Offset X-position for 1st pixel in 1st image
  @param    start_y    Offset Y-position for 1st pixel in 1st image
  @param    ilist      Input image list
  @param    firstbpm   Index of 1st plane with a non-empty bpm (or list size)
  @param    offs       List of offsets in x and y
  @param    xyprofile  The sampling profile (weight)
  @param    tabsperpix Number of profile-samples per pixel
  @see cpl_geom_img_offset_saa()

 */
/*----------------------------------------------------------------------------*/
static void CPL_TYPE_ADD(cpl_geom_img_offset_saa)(cpl_image * final,
                                                  cpl_image * contrib,
                                                  cpl_size rmin, cpl_size rmax,
                                                  double start_x,
                                                  double start_y,
                                                  const cpl_imagelist * ilist,
                                                  cpl_size firstbpm,
                                                  const cpl_bivector * offs,
                                                  const cpl_vector * xyprofile,
                                                  cpl_size tabsperpix)

{

    const cpl_size      nima  = cpl_imagelist_get_size(ilist);
    const cpl_image *   img1  = cpl_imagelist_get_const(ilist, 0);
    const cpl_size      sizex = cpl_image_get_size_x(img1);
    const cpl_size      sizey = cpl_image_get_size_y(img1);
    const cpl_size      nx    = cpl_image_get_size_x(final);
    const cpl_size      ny    = cpl_image_get_size_y(final);
    const cpl_size      rtot = rmin + rmax;
    CPL_TYPE          * po = CPL_TYPE_ADD(cpl_image_get_data)(final);
    const double      * offs_x = cpl_bivector_get_x_data_const(offs);
    const double      * offs_y = cpl_bivector_get_y_data_const(offs);
    double            * acc = (double*)cpl_malloc((size_t)nx * (size_t)nima
                                                  * sizeof(*acc));
    int               * pcontrib = cpl_image_get_data_int(contrib);
    const CPL_TYPE   ** ppi = cpl_malloc((size_t)nima * sizeof(CPL_TYPE*));
    const cpl_binary ** ppibpm = NULL;
    int               * ncontrib = (int*)cpl_calloc(sizeof(int), (size_t)nx);
    cpl_size          * offset_ij = cpl_malloc(2 * (size_t)nima
                                               * sizeof(*offset_ij));
    double            * rsc = (double*)cpl_malloc((size_t)nima * 8
                                                  * sizeof(*rsc));
    cpl_boolean       * isint = (cpl_boolean*)cpl_calloc((size_t)nima,
                                                         sizeof(*isint));
    cpl_size            j, p;
    cpl_flops           myflop_count = 0;

    for (p=0; p < nima; p++) {

        const double      offset_x = start_x - offs_x[p];
        const double      offset_y = start_y - offs_y[p];
        const double      suboff_x = offset_x - floor(offset_x);
        const double      suboff_y = offset_y - floor(offset_y);

        /* Which tabulated value index shall be used? */
        const cpl_size    tabx = (int)(0.5 + suboff_x * (double)(tabsperpix));
        const cpl_size    taby = (int)(0.5 + suboff_y * (double)(tabsperpix));


        offset_ij[0 + p * 2] = (cpl_size)floor(offset_x);
        offset_ij[1 + p * 2] = (cpl_size)floor(offset_y);

        ppi[p] = CPL_TYPE_ADD_CONST(cpl_image_get_data)
            (cpl_imagelist_get_const(ilist, p));
        ppi[p] += offset_ij[0 + p * 2];

        if (tabx == 0 && taby == 0) {
            isint[p] = CPL_TRUE;
        } else {

            const double      * interp_kernel
                = cpl_vector_get_data_const(xyprofile);

            const double      sumrs
                = (  interp_kernel[    tabsperpix + tabx]
                     + interp_kernel[                 tabx]
                     + interp_kernel[    tabsperpix - tabx]
                     + interp_kernel[2 * tabsperpix - tabx])
                * (  interp_kernel[    tabsperpix + taby]
                     + interp_kernel[                 taby]
                     + interp_kernel[    tabsperpix - taby]
                     + interp_kernel[2 * tabsperpix - taby]);


            /* Compute resampling coefficients */
            /* rsc[0..3] in x, rsc[4..7] in y  */
            /* Also divide the y-rsc with the sum of the rsc */

            rsc[0 + p * 8] = interp_kernel[    tabsperpix + tabx];
            rsc[1 + p * 8] = interp_kernel[                 tabx];
            rsc[2 + p * 8] = interp_kernel[    tabsperpix - tabx];
            rsc[3 + p * 8] = interp_kernel[2 * tabsperpix - tabx];
            rsc[4 + p * 8] = interp_kernel[    tabsperpix + taby] / sumrs;
            rsc[5 + p * 8] = interp_kernel[                 taby] / sumrs;
            rsc[6 + p * 8] = interp_kernel[    tabsperpix - taby] / sumrs;
            rsc[7 + p * 8] = interp_kernel[2 * tabsperpix - taby] / sumrs;

        }
    }

    /* Get the masks if there are bad pixels  */
    if (firstbpm < nima) {
        ppibpm = (const cpl_binary **)cpl_malloc((size_t)nima
                                                 * sizeof(cpl_binary*));

        for (p = 0; p < firstbpm; p++) {
            ppibpm[p] = NULL;
        }

        for (; p < nima; p++) {
            const cpl_mask * bpm
                = cpl_image_get_bpm_const(cpl_imagelist_get_const(ilist, p));

            if (p == firstbpm || (bpm != NULL && !cpl_mask_is_empty(bpm))) {
                ppibpm[p] = cpl_mask_get_data_const(bpm);
                ppibpm[p] += offset_ij[0 + p * 2];
            } else {
                ppibpm[p] = NULL;
            }
        }
    }

    /* Triple loop */
    for (j=0; j < ny; j++) {
        cpl_flops myintp_count = 0;
        cpl_size  i;

        for (p=0; p < nima; p++) {

            const cpl_size py = j + offset_ij[1 + p * 2];

            /* If original pixel is present in the current plane */
            if (py > 1 && py < (sizey-2)) {

                const CPL_TYPE   * pi    = ppi[p] + py * sizex;
                const cpl_binary * pibpm = NULL;
                
                /* First offset source location used */
                const cpl_size i0 = CX_MAX(0, 2 - offset_ij[0 + p * 2]);
                /* First offset source location not used */
                const cpl_size i1 = CX_MIN(sizex - 2 - offset_ij[0 + p * 2],
                                           nx);


                if(ppibpm != NULL && ppibpm[p] != NULL)
                    pibpm = ppibpm[p] + py * sizex;

                if (isint[p]) {
                    /* The shift is purely integer, so no interpolation done */
                    /* When the whole list has integer shifts, the speed-up is
                       a factor between 3 and 4 on the unit tests */
                
                    for (i = i0; i < i1; i++) {
        
                        /* assert( px == i + offset_ij[0 + p * 2] ); */

                        /* Check if the pixel used
                         * is a bad pixel. If true, this image is not used 
                         * for this pixel */
                        if (pibpm == NULL || !pibpm[i]) {

                            /* Collect pixel and update number of
                               contributing pixels */
                            /* acc[] is accessed with stride nima here, in order
                               to achieve stride one access below */
                            acc[ncontrib[i] + i * nima] = pi[i];
                            ncontrib[i]++;
                        }
                    }
                } else {
                
                    for (i = i0; i < i1; i++) {
        
                        /* assert( px == i + offset_ij[0 + p * 2] ); */

                        /* Check if any of the pixels used for the interpolation
                         * is a bad pixel. If true, this image is not used 
                         * for this pixel */
                        /* An earlier check required the pixel to be
                           bad and the resampling coefficient to be non-zero.
                           This was dropped to support bad pixels w. NaNs */ 
                        if(pibpm == NULL ||
                           /* One read for all 4 pixels in the bpm row */
                           (!*((const uint32_t*)(pibpm + i - 1 - sizex    )) &&
                            !*((const uint32_t*)(pibpm + i - 1            )) &&
                            !*((const uint32_t*)(pibpm + i - 1 + sizex    )) &&
                            !*((const uint32_t*)(pibpm + i - 1 + sizex * 2)))) {

                            /* Compute interpolated pixel and update number of
                               contributing pixels */
                            /* acc[] is accessed with stride nima here, in order
                               to achieve stride one access below */
                            acc[ncontrib[i] + i * nima]
                                = rsc[4 + 8*p] *
                                (rsc[0 + 8*p] * pi[i - 1 - sizex] +
                                 rsc[1 + 8*p] * pi[i     - sizex] +
                                 rsc[2 + 8*p] * pi[i + 1 - sizex] +
                                 rsc[3 + 8*p] * pi[i + 2 - sizex] )
                                + rsc[5 + 8*p] *
                                (rsc[0 + 8*p] * pi[i - 1] +
                                 rsc[1 + 8*p] * pi[i    ] +
                                 rsc[2 + 8*p] * pi[i + 1] +
                                 rsc[3 + 8*p] * pi[i + 2] )
                                + rsc[6 + 8*p] *
                                (rsc[0 + 8*p] * pi[i - 1 + sizex] +
                                 rsc[1 + 8*p] * pi[i     + sizex] +
                                 rsc[2 + 8*p] * pi[i + 1 + sizex] +
                                 rsc[3 + 8*p] * pi[i + 2 + sizex] )
                                + rsc[7 + 8*p] *
                                (rsc[0 + 8*p] * pi[i - 1 + sizex * 2] +
                                 rsc[1 + 8*p] * pi[i     + sizex * 2] +
                                 rsc[2 + 8*p] * pi[i + 1 + sizex * 2] +
                                 rsc[3 + 8*p] * pi[i + 2 + sizex * 2]);

                            ncontrib[i]++;
                            myintp_count++;
                        }
                    }
                }
            }
        }
        for (i=0; i < nx; i++) {
            if (ncontrib[i] > rtot) {
                double finpix = 0.0;
                double * acci = acc + i * nima;

                /* Place rmin and rmax samples at the ends */
#ifdef CPL_GEOM_USE_KTH
                /* Equivalent, but slower in unit tests */
                if (rmin > 0)
                    (void)cpl_tools_get_kth_double(acci, ncontrib[i], rmin-1);
                if (rmax > 0)
                    (void)cpl_tools_get_kth_double(acci + rmin,
                                                   ncontrib[i] - rmin,
                                                   ncontrib[i] - rtot);
#else
                cpl_geom_img_get_min_max_double(acci, ncontrib[i],
                                                rmin, rmax);
#endif

                for (p=rmin; p<(ncontrib[i]-rmax); p++) 
                    finpix += acci[p];
                finpix /= (double)(ncontrib[i]-rtot);

                po[i+j*nx] = (CPL_TYPE)finpix;
                pcontrib[i+j*nx] = ncontrib[i] - rtot;
                myflop_count += 1 + ncontrib[i] - rtot;
            } else {
                cpl_image_reject(final, i+1, j+1);
            }
            ncontrib[i] = 0;
        }
        myflop_count += 35 * myintp_count;
    }
    cpl_tools_add_flops(myflop_count + 15 * nima);

    cpl_free(offset_ij);
    cpl_free(rsc);
    cpl_free(acc);
    cpl_free(ncontrib);
    cpl_free((void*)ppi);
    cpl_free((void*)ppibpm);
    cpl_free((void*)isint);

    return;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Shift and add an images list to a single image 
  @param    final      Combined image, assumed to be zero
  @param    contrib    Contribution map, assumed to be zero
  @param    start_x    Offset X-position for 1st pixel in 1st image
  @param    start_y    Offset Y-position for 1st pixel in 1st image
  @param    ilist      Input image list
  @param    offs       List of offsets in x and y
  @param    xyprofile  The sampling profile (weight)
  @param    tabsperpix Number of profile-samples per pixel
  @see cpl_geom_img_offset_saa()

 */
/*----------------------------------------------------------------------------*/
static
void CPL_TYPE_ADD(cpl_geom_img_offset_saa_all)(cpl_image * final,
                                               cpl_image * contrib,
                                               double start_x,
                                               double start_y,
                                               const cpl_imagelist * ilist,
                                               const cpl_bivector * offs,
                                               const cpl_vector * xyprofile,
                                               cpl_size tabsperpix)

{

    const cpl_size     nima  = cpl_imagelist_get_size(ilist);
    const cpl_image  * img1  = cpl_imagelist_get_const(ilist, 0);
    const cpl_size     sizex = cpl_image_get_size_x(img1);
    const cpl_size     sizey = cpl_image_get_size_y(img1);
    const cpl_size     nx    = cpl_image_get_size_x(final);
    const cpl_size     ny    = cpl_image_get_size_y(final);

    CPL_TYPE       * po = CPL_TYPE_ADD(cpl_image_get_data)(final);
    const double   * offs_x = cpl_bivector_get_x_data_const(offs);
    const double   * offs_y = cpl_bivector_get_y_data_const(offs);
    const double   * interp_kernel = cpl_vector_get_data_const(xyprofile);
    /* The contribution map. */
    int            * pcontrib = cpl_image_get_data_int(contrib);
    /* A more compact contribution map yields a small performance gain */
    unsigned char       * pcb = cpl_calloc(1, nx * ny);
    const unsigned char * pbjr = pcb; /* Pointer for j'th row of pcb */
     /* The resampling coefficients, 4 in X and 4 in Y.
        No speed-up by declaring as CPL_TYPE.
        No speed-up by forming the 16 products (saves four multiplications,
        but introduces eight reads). */
    double           rsc[8];
    cpl_flops        myintp_count = 0;
    cpl_flops        myaddp_count = 0;
    cpl_flops        myflop_count = nx * ny + 4 * nima;
    cpl_size         i, j, p;

    /* Triple loop */
    for (p = 0; p < nima; p++) {
        const cpl_image * imgp  = cpl_imagelist_get_const(ilist, p);
        const CPL_TYPE  * pij   = CPL_TYPE_ADD_CONST(cpl_image_get_data)(imgp);
        const double      offset_x = start_x - offs_x[p];
        const double      offset_y = start_y - offs_y[p];
        const double      suboff_x = offset_x - floor(offset_x);
        const double      suboff_y = offset_y - floor(offset_y);
        const cpl_size    offset_i = (int)floor(offset_x);
        const cpl_size    offset_j = (int)floor(offset_y);
        /* Which tabulated value index shall be used? */
        const cpl_size    tabx = (int)(0.5 + suboff_x * (double)(tabsperpix));
        const cpl_size    taby = (int)(0.5 + suboff_y * (double)(tabsperpix));

        CPL_TYPE * poj = po;
        int      * pcj = pcontrib;
        unsigned char * pbj = pcb; /* Pointer for j'th row of pcb */

        if (tabx == 0 && taby == 0) {
            /* No sub-pixel shift, add input image */

            /* In this context, the pixels are numbered from 0. */
            /* First source location used */
            const int px0 = CX_MAX(0, offset_i);
            const int py0 = CX_MAX(0, offset_j);
            /* First source location not used */
            const int px1 = CX_MIN(sizex, nx + offset_i);
            const int py1 = CX_MIN(sizey, ny + offset_j);

            int px, py;

            /* Offset the pointers so a single variable suffices for the
               indexing. */
            poj += (py0 - offset_j) * nx - offset_i;
            pcj += (py0 - offset_j) * nx - offset_i;
            pbj += (py0 - offset_j) * nx - offset_i;
            pij += py0 * sizex;

            myaddp_count += (py1 - py0) * (px1 - px0);
            for (py = py0; py < py1; py++, poj += nx, pcj += nx, pbj += nx,
                     pij += sizex) {

                for (px = px0; px < px1; px++) {

                    poj[px] += pij[px];

                    if (++pbj[px] == 0) pcj[px] += 256;
                }
            }
        } else {

            const double      sumrs
                = (  interp_kernel[    tabsperpix + tabx]
                     + interp_kernel[                 tabx]
                     + interp_kernel[    tabsperpix - tabx]
                     + interp_kernel[2 * tabsperpix - tabx])
                * (  interp_kernel[    tabsperpix + taby]
                     + interp_kernel[                 taby]
                     + interp_kernel[    tabsperpix - taby]
                     + interp_kernel[2 * tabsperpix - taby]);

            /* Interpolate values from any source pixel at least 2 pixels away
               from the source boundary */
            /* In this context, the pixels are numbered from 0. */

            /* First source location used */
            const int px0 = CX_MAX(2, offset_i);
            const int py0 = CX_MAX(2, offset_j);
            /* First source location not used */
            const int px1 = CX_MIN(sizex - 2, nx + offset_i);
            const int py1 = CX_MIN(sizey - 2, ny + offset_j);

            int px, py;

            /* Number of interpolated pixels */
            myintp_count += 11 + 36 * (py1 - py0) * (px1 - px0);

            /* Compute resampling coefficients */
            /* rsc[0..3] in x, rsc[4..7] in y  */
            /* Also divide the y-rsc with the sum of the rsc */

            rsc[0] = interp_kernel[    tabsperpix + tabx];
            rsc[1] = interp_kernel[                 tabx];
            rsc[2] = interp_kernel[    tabsperpix - tabx];
            rsc[3] = interp_kernel[2 * tabsperpix - tabx];
            rsc[4] = interp_kernel[    tabsperpix + taby] / sumrs;
            rsc[5] = interp_kernel[                 taby] / sumrs;
            rsc[6] = interp_kernel[    tabsperpix - taby] / sumrs;
            rsc[7] = interp_kernel[2 * tabsperpix - taby] / sumrs;

            /* Interpolate from input image */
            /* Offset the pointers so a single variable suffices for the
               indexing. */
            poj += (py0 - offset_j) * nx - offset_i;
            pcj += (py0 - offset_j) * nx - offset_i;
            pbj += (py0 - offset_j) * nx - offset_i;
            pij += py0 * sizex;

            for (py = py0; py < py1; py++, poj += nx, pcj += nx, pbj += nx, pij += sizex) {

                for (px = px0; px < px1; px++) {

                    /* Compute interpolated pixel now */
                    poj[px] += 
                        rsc[4] * (  rsc[0] * pij[px - 1 - sizex] +
                                    rsc[1] * pij[px     - sizex] +
                                    rsc[2] * pij[px + 1 - sizex] +
                                    rsc[3] * pij[px + 2 - sizex] ) +
                        rsc[5] * (  rsc[0] * pij[px - 1] +
                                    rsc[1] * pij[px    ] +
                                    rsc[2] * pij[px + 1] +
                                    rsc[3] * pij[px + 2] ) +
                        rsc[6] * (  rsc[0] * pij[px - 1 + sizex] +
                                    rsc[1] * pij[px     + sizex] +
                                    rsc[2] * pij[px + 1 + sizex] +
                                    rsc[3] * pij[px + 2 + sizex] ) +
                        rsc[7] * (  rsc[0] * pij[px - 1 + sizex * 2] +
                                    rsc[1] * pij[px     + sizex * 2] +
                                    rsc[2] * pij[px + 1 + sizex * 2] +
                                    rsc[3] * pij[px + 2 + sizex * 2] );

                    if (++pbj[px] == 0) pcj[px] += 256;
                }
            }
        }
    }

    for (j = 0; j < ny; j++, po += nx, pcontrib += nx, pbjr += nx) {
        for (i = 0; i < nx; i++) {
            pcontrib[i] += (int)pbjr[i];
            if (pcontrib[i] > 0) {
                po[i] /= (CPL_TYPE)pcontrib[i];
            } else {
                cpl_image_reject(final, i+1, j+1);
                myflop_count--;
            }
        }
    }

    cpl_tools_add_flops(myflop_count + myintp_count + myaddp_count);
    cpl_free(pcb);

    return;
}


#undef CPL_TYPE_ADD
#undef CPL_TYPE_ADD_CONST
