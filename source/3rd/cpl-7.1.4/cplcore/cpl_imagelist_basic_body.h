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
#if defined CPL_CLASS && CPL_CLASS == CPL_CLASS_DOUBLE
#define CPL_TYPE double
#define CPL_TYPE_T CPL_TYPE_DOUBLE
#define CPL_IMAGE_GET_DATA cpl_image_get_data_double
#define CPL_IMAGE_GET_DATA_CONST cpl_image_get_data_double_const
#define CPL_IMAGE_WRAP cpl_image_wrap_double
#define CPL_IMAGE_GET_MEDIAN cpl_tools_get_median_double
#define CPL_TOOLS_GET_KTH cpl_tools_get_kth_double

#elif defined CPL_CLASS && CPL_CLASS == CPL_CLASS_FLOAT
#define CPL_TYPE float
#define CPL_TYPE_T CPL_TYPE_FLOAT
#define CPL_IMAGE_GET_DATA cpl_image_get_data_float
#define CPL_IMAGE_GET_DATA_CONST cpl_image_get_data_float_const
#define CPL_IMAGE_WRAP cpl_image_wrap_float
#define CPL_IMAGE_GET_MEDIAN cpl_tools_get_median_float
#define CPL_TOOLS_GET_KTH cpl_tools_get_kth_float

#elif defined CPL_CLASS && CPL_CLASS == CPL_CLASS_INT
#define CPL_TYPE int
#define CPL_TYPE_T CPL_TYPE_INT
#define CPL_IMAGE_GET_DATA cpl_image_get_data_int
#define CPL_IMAGE_GET_DATA_CONST cpl_image_get_data_int_const
#define CPL_IMAGE_WRAP cpl_image_wrap_int
#define CPL_IMAGE_GET_MEDIAN cpl_tools_get_median_int
#define CPL_TOOLS_GET_KTH cpl_tools_get_kth_int

#else
#undef CPL_TYPE
#undef CPL_TYPE_T
#undef CPL_IMAGE_GET_DATA
#undef CPL_IMAGE_GET_DATA_CONST
#undef CPL_IMAGE_WRAP
#undef CPL_IMAGE_GET_MEDIAN
#undef CPL_TOOLS_GET_KTH
#endif

#if CPL_OPERATION == CPL_IMLIST_BASIC_OPER

    cpl_size i;

    /* Check input image sets */
    cpl_ensure_code(in1, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(in2, CPL_ERROR_NULL_INPUT);

    /* Check image sets compatibility     */
    cpl_ensure_code( in1->ni == in2->ni, CPL_ERROR_ILLEGAL_INPUT);

    /* Loop on the planes and apply the operation */
    for (i=0; i<in1->ni; i++) {
        const cpl_error_code error = 
            CPL_OPERATOR(in1->images[i], in2->images[i]);
        cpl_ensure_code(!error, error);
    }

    return CPL_ERROR_NONE;

#elif CPL_OPERATION == CPL_IMLIST_BASIC_IMAGE_LOCAL

    cpl_size i;

    /* Check input image sets */
    cpl_ensure_code(imlist, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(img,    CPL_ERROR_NULL_INPUT);

    /* Loop on the planes and apply the operation */
    for (i=0; i<imlist->ni; i++) {
        const cpl_error_code error = CPL_OPERATOR(imlist->images[i], img);
        cpl_ensure_code(!error, error);
    }

    return CPL_ERROR_NONE;

#elif CPL_OPERATION == CPL_IMLIST_BASIC_TIME_MEDIAN
    case CPL_TYPE_T:
    {
        const size_t        nz = self->ni;
        CPL_TYPE         ** pi = cpl_malloc((size_t)nz * sizeof(CPL_TYPE*));
        CPL_TYPE          * po = (CPL_TYPE*)cpl_malloc(nx * ny * sizeof(*po));
        CPL_TYPE          * timeline = NULL;
        const cpl_binary ** pbpm = NULL;
        size_t              i, j, k;

        /* Create the output image */
        median = cpl_image_wrap(nx, ny, type, po);

        /* Find planes with a non-empty bad pixel map - if any */
        for (k = 0; k < nz; k++) {
            const cpl_mask * bpm
                = cpl_image_get_bpm_const(self->images[k]);

            pi[k] = CPL_IMAGE_GET_DATA(self->images[k]);

            if (bpm != NULL && !cpl_mask_is_empty(bpm)) {
                /* Found one */
                if (pbpm == NULL) {
                    pbpm = cpl_calloc((size_t)nz, sizeof(*pbpm));
                    /* Assume that cpl_calloc() fills with NULL */
                }
                pbpm[k] = cpl_mask_get_data_const(bpm);
            }
        }

        {
            const size_t cacheline = 64 / sizeof(CPL_TYPE);
            timeline = cpl_malloc((size_t)nz * sizeof(CPL_TYPE) * cacheline);
            if (pbpm == NULL) {
                /* For each time line */
                for (i=0; i < nx * ny - ((nx * ny) % cacheline); i+=cacheline) {
                    /* Get the pixels on the current time line  */
                    for (k=0; k < nz; k++) {
                        for (j = 0; j < cacheline; j++) {
                            CPL_TYPE * pik = pi[k];
                            timeline[k + j * nz] = pik[i + j];
                        }
                    }

                    /* Compute the median */
                    for (j = 0; j < cacheline; j++) {
                        po[i + j] = CPL_IMAGE_GET_MEDIAN(timeline + j * nz, nz);
                    }
                }
                for (; i < nx * ny; i++) {
                    /* Get the pixels on the current time line  */
                    for (k=0; k < nz; k++) {
                        timeline[k] = pi[k][i];
                    }

                    /* Compute the median */
                    po[i] = CPL_IMAGE_GET_MEDIAN(timeline, nz);
                }
            } else {
                /* For each time line */
                for (i=0; i < nx * ny - ((nx * ny) % cacheline); i+=cacheline) {
                    /* Get the pixels on the current time line  */
                    size_t igood[cacheline];
                    memset(igood, 0, sizeof(igood));
                    for (k=0; k < nz; k++) {
                        for (j = 0; j < cacheline; j++) {
                            if (pbpm[k] == NULL || !pbpm[k][i + j]) {
                                timeline[j * nz + igood[j]++] = pi[k][i + j];
                            }
                        }
                    }

                    for (j = 0; j < cacheline; j++) {
                        if (igood[j] > 0) {
                            /* Compute the median */
                            po[i + j] = CPL_IMAGE_GET_MEDIAN(timeline + j * nz, igood[j]);
                        } else {
                            po[i + j] = (CPL_TYPE)0;
                            cpl_image_reject(median, (i + j)%nx + 1, (i + j)/nx + 1);
                        }
                    }
                }
                for (; i < nx * ny; i++) {
                    /* Get the pixels on the current time line  */
                    size_t igood = 0;
                    for (k=0; k < nz; k++) {
                        if (pbpm[k] == NULL || !pbpm[k][i]) {
                            timeline[igood++] = pi[k][i];
                        }
                    }

                    if (igood > 0) {
                        /* Compute the median */
                        po[i] = CPL_IMAGE_GET_MEDIAN(timeline, igood);
                    } else {
                        po[i] = (CPL_TYPE)0;
                        cpl_image_reject(median, i%nx + 1, i/nx + 1);
                    }
                }
            }
        }

        cpl_free(pi);
        cpl_free(timeline);
        cpl_free(pbpm);
        break;
    }

#elif CPL_OPERATION == CPL_IMLIST_BASIC_TIME_MINMAX

    case CPL_TYPE_T:
    {
        /* Pointers to the ni pixel buffers */
        const CPL_TYPE ** pin = cpl_malloc(ni * sizeof(CPL_TYPE *));
        const cpl_binary ** pbpm = NULL;

        /* Output pixel buffer */
        CPL_TYPE       *  pavg
            = cpl_malloc(nx * ny * sizeof(*pavg));

        /* A single timeline */
        CPL_TYPE * ptime = cpl_malloc(ni * sizeof(*ptime));
        size_t     i, k;

        avg = CPL_IMAGE_WRAP(nx, ny, pavg);

        /* Find planes with a non-empty bad pixel map - if any */
        for (k = 0; k < ni; k++) {

            const cpl_mask * bpm
                = cpl_image_get_bpm_const(self->images[k]);

            pin[k] = CPL_IMAGE_GET_DATA_CONST(cpl_imagelist_get_const(self, k));

            if (bpm != NULL && !cpl_mask_is_empty(bpm)) {
                /* Found one */
                if (pbpm == NULL) {
                    pbpm = cpl_calloc(ni, sizeof(*pbpm));
                    /* Assume that cpl_calloc() fills with NULL */
                }
                pbpm[k] = cpl_mask_get_data_const(bpm);
            }

        }


        /* Loop on the pixels */
        for (i = 0; i < nx * ny; i++) {
            size_t igood = 0;
            size_t ilow, ihigh;
            size_t ihbad;

            /* Fill the timeline */
            for (k = 0; k < ni; k++) {
                if (pbpm == NULL || pbpm[k] == NULL || !pbpm[k][i]) {
                    ptime[igood++] = pin[k][i];
                }
            }

            ihbad = (ni - igood) / 2;

            ilow  = (size_t)nlow  > ihbad ? (size_t)nlow  - ihbad : 0;
            ihigh = (size_t)nhigh > ihbad ? (size_t)nhigh - ihbad : 0;

            if (igood > ilow + ihigh) {
                double mean = 0.0;
                const size_t iuse = igood - ilow - ihigh;

                /* Place ilow and ihigh samples at the ends */
                if (ilow  > 0) (void)CPL_TOOLS_GET_KTH(ptime, igood, ilow-1);
                if (ihigh > 0) (void)CPL_TOOLS_GET_KTH(ptime + ilow,
                                                       iuse + ihigh, iuse);

                /* Compute the average - using the recurrence relation from
                   cpl_tools_get_mean_double() */
                for (k = 0; k < iuse; k++)
                    mean += ((double)ptime[ilow + k] - mean) / (double)(k + 1);

                pavg[i] = (CPL_TYPE)mean;

                cpl_tools_add_flops(3 * iuse);
            } else {
                pavg[i] = (CPL_TYPE)0;
                cpl_image_reject(avg, i%nx + 1, i/nx + 1);
            }
        }

        cpl_free(pin);
        cpl_free(ptime);
        cpl_free(pbpm);

        break;
    }

#elif CPL_OPERATION == CPL_IMLIST_BASIC_TIME_SIGCLIP

    case CPL_TYPE_T:
    {
        /* Need at least two values for the standard deviation */
        const cpl_size notok = CX_MAX(1, (cpl_size)(keepfrac * (double)ni));
        /* The pixel buffer of the output image */
        CPL_TYPE * pclipped  = (CPL_TYPE*)cpl_malloc((size_t)nx * (size_t)ny *
                                                     sizeof(*pclipped));
        /* Temporary array for the non-clipped values */
        double   * pvals = (double*)cpl_malloc((size_t)ni * sizeof(double));
        cpl_size   i;

        clipped  = cpl_image_wrap(nx, ny, CPL_TYPE_T, (void*)pclipped);

        /* Loop on the pixels */
        for (i = 0; i < nx * ny; i++) {
            double   mean = 0.0;    /* Value to use when all pixels are bad */
            cpl_size nok  = 0;      /* Number of values used in this timeline */
            /* Need a 2nd counter, for convergence check and for contribution */
            cpl_size prevok = ni + 1; /* No convergence before 1st iteration */
            cpl_size k;
        
            /* Extract and count the non-bad values for this pixel */
            for (k = 0; k < ni; k++) {
                const cpl_image  * imgk = cpl_imagelist_get_const(self, k);
                const cpl_mask   * bpm  = cpl_image_get_bpm_const(imgk);
                const cpl_binary * bbpm = bpm ? cpl_mask_get_data_const(bpm)
                    : NULL;

                if (bbpm == NULL || !bbpm[i]) {
                    const CPL_TYPE * pimg = CPL_IMAGE_GET_DATA_CONST(imgk);
                    pvals[nok++] = (double)pimg[i];
                }
            }

            /* Will not actually enter when there are too many bad pixels */
            while (notok < nok && nok < prevok) {
                /* cpl_tools_get_variancesum_double() is trusted to return
                   a non-negative result even with rounding errors,
                   so we assume it is safe to take the sqrt() below.
                   Compute also the mean. Since it is computed prior
                   to the clipping, it is the mean-value sought
                   once the iteration has stopped. */
                const double varsum = cpl_tools_get_variancesum_double
                    (pvals, nok, &mean);

                /* When computed, the median permutes the (non-clipped)
                   values, but this has no effect on the result. */
                const double center = mode == CPL_COLLAPSE_MEAN ||
                    (mode == CPL_COLLAPSE_MEDIAN_MEAN && prevok < ni + 1)
                    ? mean : cpl_tools_get_median_double(pvals, nok);

                /* Compute the clipping thresholds */
                const double stdev = sqrt(varsum / (double) (nok-1));
                const double low_thresh  = center - kappalow  * stdev;
                const double high_thresh = center + kappahigh * stdev;

                prevok = nok;
                /* Count the initial all-ok values */
                for (nok = 0; nok < prevok; nok++) {
                    /* Use exact same clipping criterion as next loop */
                    if (!(low_thresh < pvals[nok] &&
                          pvals[nok] < high_thresh)) {
                        break;
                    }
                }

                /* All values may have been checked above, but if not
                   skip the one that cause the above loop to stop and
                   check the rest and repack+count as necessary */
                for (k = nok+1; k < prevok; k++) {
                    if (low_thresh < pvals[k] &&
                        pvals[k] < high_thresh) {
                        pvals[nok++] = pvals[k];
                    }
                }
            }

            if (prevok == ni+1) {
                /* The clipping loop was not entered at all */

                prevok = nok; /* Needed for the contribution map */

                /* The mean must be computed (without clipping) */
                if (nok == 0) { /* All pixels are bad, so result pixel is bad */
                    /* The call for the 1st bad pixel will create the bpm */
                    cpl_mask   * bpm  = cpl_image_get_bpm(clipped);
                    cpl_binary * bbpm = cpl_mask_get_data(bpm);

                    bbpm[i] = CPL_BINARY_1;
                } else {
                    mean = cpl_tools_get_mean_double(pvals, nok);
                }
            }

            /* Set the output image pixel value */
            pclipped[i] = (CPL_TYPE)mean;

            if (pcontrib) {
                /* The contributing number of values */
                pcontrib[i] = (int)prevok;
                if((cpl_size)pcontrib[i] != prevok) break;
            }

            cpl_tools_add_flops(6 + prevok);

        }

        cpl_free(pvals);

        if (i < nx * ny) {
            cpl_image_delete(clipped);
            clipped = NULL;
            (void)cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        }

        break;
    }

#elif CPL_OPERATION == CPL_IMLIST_BASIC_SWAP_AXIS 

    case CPL_TYPE_T:
    {
        CPL_TYPE       * pcur_ima;
        const CPL_TYPE * pold_ima;

        /* SWAP X <-> Z */
        if (mode == CPL_SWAP_AXIS_XZ) {
            for (i=0; i<nx; i++) {
                cur_ima=cpl_image_new(ni, ny, CPL_TYPE_T);
                pcur_ima = CPL_IMAGE_GET_DATA(cur_ima);
                for (j=0; j<ni; j++) {
                    old_ima = cpl_imagelist_get_const(ilist, j);
                    pold_ima = CPL_IMAGE_GET_DATA_CONST(old_ima);
                    for (k=0; k<ny; k++) {
                        pcur_ima[j+k*ni] = pold_ima[i+k*nx];
                    }
                }
                cpl_imagelist_set(swapped, cur_ima, i);
            }
        } else {
        /* SWAP Y <-> Z */
            for (i=0; i<ny; i++) {
                cur_ima=cpl_image_new(nx, ni, CPL_TYPE_T);
                pcur_ima = CPL_IMAGE_GET_DATA(cur_ima);
                for (j=0; j<ni; j++) {
                    old_ima = cpl_imagelist_get_const(ilist, j);
                    pold_ima = CPL_IMAGE_GET_DATA_CONST(old_ima);
                    for (k=0; k<nx; k++) {
                        pcur_ima[k+j*nx] = pold_ima[k+i*nx];
                    }
                }
                cpl_imagelist_set(swapped, cur_ima, i);
            }
        }
        break;
    }

#endif

#undef CPL_TYPE
#undef CPL_TYPE_T
#undef CPL_IMAGE_GET_DATA
#undef CPL_IMAGE_GET_DATA_CONST
#undef CPL_IMAGE_WRAP
#undef CPL_IMAGE_GET_MEDIAN
#undef CPL_TOOLS_GET_KTH
