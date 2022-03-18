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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/*-----------------------------------------------------------------------------
                                Includes
 -----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>

#include <cpl_memory.h>
#include <cpl_imagelist.h>
#include <cpl_image_io_impl.h>
#include <cpl_stats.h>

#include <cpl_tools.h>
#include <cpl_vector.h>
#include <cpl_error_impl.h>
#include <cpl_errorstate.h>

#include "cpl_apertures.h"
#include "cpl_geom_img.h"

/*----------------------------------------------------------------------------*/
/**
 * @defgroup cpl_geom_img   High level functions for geometric transformations
 *
 * This module contains functions to compute the shift-and-add operation
 * on an image list.
 *
 * @par Synopsis:
 * @code
 *   #include "cpl_geom_img.h"
 * @endcode
 */
/*----------------------------------------------------------------------------*/
/**@{*/

/*-----------------------------------------------------------------------------
                            Defines
 -----------------------------------------------------------------------------*/

#ifndef inline
#define inline /* inline */
#endif

#define CPL_SORT(a, i, j, tmp) do { \
        if (a[i] < a[j]) { tmp = a[i]; a[i] = a[j]; a[j] = tmp; }} while (0)

/*-----------------------------------------------------------------------------
                            Static Function Prototypes
 -----------------------------------------------------------------------------*/

static void cpl_geom_img_get_min_max_double(double *, cpl_size, cpl_size,
                                            cpl_size) CPL_ATTR_NONNULL;

static double cpl_geom_ima_offset_xcorr(const cpl_image *, const cpl_image *,
                                        const cpl_bivector *,
                                        cpl_size, cpl_size, cpl_size, cpl_size,
                                        double *, double *) CPL_ATTR_NONNULL;

static double cpl_geom_ima_offset_xcorr_subw(double *, const cpl_image *,
                                             const cpl_image *,
                                             cpl_size, cpl_size,
                                             cpl_size, cpl_size,
                                             cpl_size, cpl_size,
                                             cpl_size, cpl_size, double *,
                                             double *) CPL_ATTR_NONNULL;

static
void cpl_geom_ima_offset_xcorr_subw_double(double *,
                                           const cpl_image *,
                                           const cpl_image *,
                                           cpl_size, cpl_size,
                                           cpl_size, cpl_size,
                                           cpl_size, cpl_size,
                                           cpl_size, cpl_size,
                                           cpl_size *, cpl_size *)
    CPL_ATTR_NONNULL;

static
void cpl_geom_ima_offset_xcorr_subw_float(double *,
                                          const cpl_image *,
                                          const cpl_image *,
                                          cpl_size, cpl_size,
                                          cpl_size, cpl_size,
                                          cpl_size, cpl_size,
                                          cpl_size, cpl_size,
                                          cpl_size *, cpl_size *)
    CPL_ATTR_NONNULL;

static void cpl_geom_img_offset_saa_double(cpl_image *, cpl_image *,
                                           cpl_size, cpl_size, double, double,
                                           const cpl_imagelist *, cpl_size,
                                           const cpl_bivector *,
                                           const cpl_vector *,
                                           cpl_size) CPL_ATTR_NONNULL;

static void cpl_geom_img_offset_saa_float(cpl_image *, cpl_image *,
                                          cpl_size, cpl_size, double, double,
                                          const cpl_imagelist *, cpl_size,
                                          const cpl_bivector *,
                                          const cpl_vector *,
                                          cpl_size) CPL_ATTR_NONNULL;

static void cpl_geom_img_offset_saa_all_double(cpl_image *,
                                               cpl_image *,
                                               double, double,
                                               const cpl_imagelist *,
                                               const cpl_bivector *,
                                               const cpl_vector *,
                                               cpl_size) CPL_ATTR_NONNULL;

static void cpl_geom_img_offset_saa_all_float(cpl_image *,
                                              cpl_image *,
                                              double, double,
                                              const cpl_imagelist *,
                                              const cpl_bivector *,
                                              const cpl_vector *,
                                              cpl_size) CPL_ATTR_NONNULL;

static cpl_imagelist * cpl_imagelist_wrap_all_but_first(cpl_imagelist *)
    CPL_ATTR_ALLOC;

static cpl_bivector * cpl_bivector_wrap_all_but_first(cpl_bivector *)
    CPL_ATTR_ALLOC;
static void cpl_bivector_unwrap(cpl_bivector *);


/*-----------------------------------------------------------------------------
                            Function codes
 -----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the offsets by correlating the images
  @param    ilist           Input image list
  @param    estimates       First-guess estimation of the offsets
  @param    anchors         List of anchor points
  @param    s_hx            Half-width of search area
  @param    s_hy            Half-height of search area
  @param    m_hx            Half-width of measurement area
  @param    m_hy            Half-height of measurement area
  @param    correl          List of cross-correlation quality factors
  @return   List of offsets or NULL on error

  The matching is performed using a 2d cross-correlation, using a minimal 
  squared differences criterion. One measurement is performed per input anchor 
  point, and the median offset is returned together with a measure of 
  similarity for each plane.
 
  The images in the input list must only differ from a shift. In order
  from the correlation to work, they must have the same level (check the
  average values of your input images if the correlation does not work).

  The supported types are CPL_TYPE_DOUBLE and CPL_TYPE_FLOAT.
  The bad pixel maps are ignored by this function.
  
  The ith offset (offsx, offsy) in the returned offsets is the one that have 
  to be used to shift the ith image to align it on the reference image (the 
  first one).
  
  If not NULL, the returned cpl_bivector must be deallocated with
  cpl_bivector_delete().
  
  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if (one of) the input pointer(s) is NULL
  - CPL_ERROR_ILLEGAL_INPUT if ilist is not valid
 */
/*----------------------------------------------------------------------------*/
cpl_bivector * cpl_geom_img_offset_fine(
        const cpl_imagelist *   ilist,
        const cpl_bivector  *   estimates,
        const cpl_bivector  *   anchors,
        cpl_size                s_hx,
        cpl_size                s_hy,
        cpl_size                m_hx,
        cpl_size                m_hy,
        cpl_vector          *   correl)
{
    cpl_errorstate    prevstate = cpl_errorstate_get();
    const cpl_size    nima = cpl_imagelist_get_size(ilist);
    const cpl_image * img1 = cpl_imagelist_get_const(ilist, 0);
    const cpl_type    type = cpl_image_get_type(img1);
    const cpl_size    nx = cpl_image_get_size_x(img1);
    const cpl_size    ny = cpl_image_get_size_y(img1);
    cpl_image       * med1;
    cpl_image       * medi;
    cpl_mask        * kernel;
    double          * correl_data;
    const double    * anchors_x_data = cpl_bivector_get_x_data_const(anchors);
    const double    * anchors_y_data = cpl_bivector_get_y_data_const(anchors);
    const double    * estim_x_data = cpl_bivector_get_x_data_const(estimates);
    const double    * estim_y_data = cpl_bivector_get_y_data_const(estimates);
    cpl_bivector    * offsets;
    double          * offsets_x_data;
    double          * offsets_y_data;
    double            offsx, offsy;
    cpl_size          i;
    
    /* Check entries */
    cpl_ensure(ilist,     CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(estimates, CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(anchors,   CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(correl,    CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(cpl_imagelist_is_uniform(ilist)==0, CPL_ERROR_ILLEGAL_INPUT,
            NULL);

    for (i = 0; i < cpl_bivector_get_size(anchors); i++) {
        cpl_ensure(anchors_x_data[i] >= 0.0, CPL_ERROR_ILLEGAL_INPUT, NULL);
        cpl_ensure(anchors_x_data[i] < nx,   CPL_ERROR_ILLEGAL_INPUT, NULL);
        cpl_ensure(anchors_y_data[i] >= 0.0, CPL_ERROR_ILLEGAL_INPUT, NULL);
        cpl_ensure(anchors_y_data[i] < ny,   CPL_ERROR_ILLEGAL_INPUT, NULL);
    }

    /* Create the kernel for the filtering operations */
    kernel = cpl_mask_new(3, 3);
    cpl_mask_not(kernel);
    
    /* Filter the first image */
    medi = cpl_image_new(nx, ny, type);
    med1 = cpl_image_new(nx, ny, type);
    cpl_image_filter_mask(med1, img1, kernel, CPL_FILTER_MEDIAN,
                          CPL_BORDER_FILTER);
    
    /* Create the offsets container */
    offsets = cpl_bivector_new(nima);
    offsets_x_data = cpl_bivector_get_x_data(offsets);
    offsets_y_data = cpl_bivector_get_y_data(offsets);
    correl_data = cpl_vector_get_data(correl);

    offsets_x_data[0] = 0.0;
    offsets_y_data[0] = 0.0;
    correl_data[0] = 0.0;
    for (i=1; i < nima; i++) {

        /* Filter the current image */
        cpl_image_filter_mask(medi, cpl_imagelist_get_const(ilist, i), kernel,
                              CPL_FILTER_MEDIAN, CPL_BORDER_FILTER);

        /* Set the estimates */
        offsx = estim_x_data[i];
        offsy = estim_y_data[i];
        
        /* Perform cross-correlation */
        correl_data[i] = cpl_geom_ima_offset_xcorr(med1, medi, anchors, 
                                                   s_hx, s_hy, m_hx, m_hy,
                                                   &offsx, &offsy);
        cpl_errorstate_set(prevstate);

        if (correl_data[i] < 0.0) {
            /* Nothing was found. */
            /* declare the offset as invalid: null offsets and dist=-1 */
            offsets_x_data[i] = 0.0;
            offsets_y_data[i] = 0.0;
        } else {
            /* Something was found. */
            /* One standard failure case is when the returned offset is */
            /* located on the border of the search zone. Identify such */
            /* cases and flag the frame as not registrable by setting */
            /* the offset vector to nil and the distance to -1. */
            if ((fabs(fabs(estim_x_data[i]-offsx)-(double)s_hx)<1) ||
                (fabs(fabs(estim_y_data[i]-offsy)-(double)s_hy)<1)) {
                offsets_x_data[i] = 0.0;
                offsets_y_data[i] = 0.0;
                correl_data[i] = -1.0;
            } else {
                /* The returned offset is correct */
                offsets_x_data[i] = offsx;
                offsets_y_data[i] = offsy;
            }
        }
        if (correl_data[i] < 0.0) {
            cpl_msg_debug(cpl_func, "Frame %" CPL_SIZE_FORMAT "/%"
                          CPL_SIZE_FORMAT " does not correlate in window "
                          " 1 + 2%" CPL_SIZE_FORMAT " X 1 + 2%" CPL_SIZE_FORMAT
                          " sampling 1 + 2%" CPL_SIZE_FORMAT " X 1 + 2%"
                          CPL_SIZE_FORMAT, 1 + i, nima, s_hx, s_hy, m_hx, m_hy);
        }
    }
    
    cpl_mask_delete(kernel);
    cpl_image_delete(med1);
    cpl_image_delete(medi);

    return offsets;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Images list recombination
  @param    self        Input imagelist - with refining images may be erased
  @param    offs        List of offsets in x and y
  @param    refine      Iff non-zero, the offsets will be refined
  @param    aperts      List of correlation apertures or NULL if unknown
  @param    sigmas      Positive, decreasing sigmas to apply
  @param    pisigma     Index of the sigma that was used or undefined on error
  @param    s_hx        Search area half-width.
  @param    s_hy        Search area half-height.
  @param    m_hx        Measurement area half-width.
  @param    m_hy        Measurement area half-height.
  @param    min_rej     number of low values to reject in stacking
  @param    max_rej     number of high values to reject in stacking
  @param    union_flag  Combination mode (CPL_GEOM_UNION or CPL_GEOM_INTERSECT)
  @return   Pointer to newly allocated images array, or NULL on error.
  @see cpl_geom_img_offset_saa()
  @see cpl_apertures_extract()
  @see cpl_geom_img_offset_fine()

  With offset refinement enabled:
    This function detects cross correlation points in the first image (if not 
    provided by the user), use them to refine the provided offsets with a cross 
    correlation method, and then apply the shift and add to recombine the images 
    together. Non-correlating images are removed from self.

  Without offset refinement self is not modified.
  
  The supported types are CPL_TYPE_DOUBLE, CPL_TYPE_FLOAT.
 
  The number of provided offsets shall be equal to the number of input images.
  The ith offset (offs_x, offs_y) is the offset that has to be used to shift 
  the ith image to align it on the first one.

  sigmas may be NULL if offset refinement is disabled or if aperts is non-NULL.
  
  On success the returned image array contains 2 images:
  - the combined image
  - the contribution map
  
  The returned cpl_image array must be deallocated like this:
  @code
    if (array != NULL) {
        cpl_image_delete(array[0]);
        cpl_image_delete(array[1]);
        cpl_free(array);
    }
  @endcode
  
  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if self or offs is NULL, or if sigmas is NULL with
    refinement enabled and aperts NULL.
  - CPL_ERROR_ILLEGAL_INPUT if self is not uniform
  - CPL_ERROR_INCOMPATIBLE_INPUT if self and offs have different sizes
  - CPL_ERROR_DATA_NOT_FOUND if the shift and add of the images fails
 */
/*----------------------------------------------------------------------------*/
cpl_image ** cpl_geom_img_offset_combine(const cpl_imagelist* self,
                                         const cpl_bivector * offs,
                                         int                  refine,
                                         const cpl_bivector * aperts,
                                         const cpl_vector   * sigmas,
                                         cpl_size           * pisigma,
                                         cpl_size             s_hx,
                                         cpl_size             s_hy,
                                         cpl_size             m_hx,
                                         cpl_size             m_hy,
                                         cpl_size             min_rej,
                                         cpl_size             max_rej,
                                         cpl_geom_combine     union_flag)
{
    const cpl_imagelist* uselist  = self;
    cpl_imagelist      * modlist  = NULL;
    const cpl_bivector * use_offs = offs;
    cpl_bivector       * offs_fine = NULL;
    const cpl_size       nima = cpl_imagelist_get_size(self);
    cpl_image         ** combined;


    /* Check inputs */
    cpl_ensure(self != NULL, CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(cpl_imagelist_is_uniform(self) == 0, CPL_ERROR_ILLEGAL_INPUT,
            NULL);
    cpl_ensure(offs != NULL, CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(cpl_bivector_get_size(offs) == nima, CPL_ERROR_INCOMPATIBLE_INPUT,
               NULL);

    if (refine != 0 && nima > 1) {
        const cpl_bivector * use_aperts = aperts; /* May be NULL */
        cpl_bivector       * anchor = NULL;

        if (use_aperts == NULL) {
            /* No apertures are provided, look for a cross correlation point */
            double        * anchor_x;
            double        * anchor_y;
            cpl_apertures * apertures
                = cpl_apertures_extract(cpl_imagelist_get_const(self, 0),
                                                              sigmas, pisigma);

            if (apertures != NULL) {

                /* Found some, just pick a single point */
                anchor = cpl_bivector_new(1);
                anchor_x = cpl_bivector_get_x_data(anchor);
                anchor_y = cpl_bivector_get_y_data(anchor);

                /* ... of the largest object */
                cpl_apertures_sort_by_npix(apertures);

                anchor_x[0] = cpl_apertures_get_pos_x(apertures, 1);
                anchor_y[0] = cpl_apertures_get_pos_y(apertures, 1);
                cpl_apertures_delete(apertures);

                use_aperts = anchor;
            }
        }

        if (use_aperts != NULL) {
            /* Refine the offsets */
            cpl_vector * correl = cpl_vector_new(nima);

            offs_fine = cpl_geom_img_offset_fine(self, offs, use_aperts,
                                                 s_hx, s_hy, m_hx, m_hy, correl);
            cpl_bivector_delete(anchor);

            if (offs_fine != NULL) {
                const double * correl_data = cpl_vector_get_data_const(correl);
                double       * offs_fine_x = cpl_bivector_get_x_data(offs_fine);
                double       * offs_fine_y = cpl_bivector_get_y_data(offs_fine);
                cpl_size       ngood = 0;
                cpl_size       i;

                /* Create a modified list, with the correlating images */
                modlist = cpl_imagelist_new();

                /* Erase the non-correlating offsets and count the good ones */
                for (i = 0; i < nima; i++) {
                    if (correl_data[i] > -0.5) {
                        offs_fine_x[ngood] = offs_fine_x[i];
                        offs_fine_y[ngood] = offs_fine_y[i];
                        /* modlist will _not_ be modified */
                        cpl_imagelist_set(modlist, (cpl_image*)
                                          cpl_imagelist_get_const(self, i),
                                          ngood);
                        ngood++;
                    }
                }

                if (ngood > 0) {
                    if (ngood < nima) {
                        cpl_vector_set_size(cpl_bivector_get_x(offs_fine),
                                            ngood);
                        cpl_vector_set_size(cpl_bivector_get_y(offs_fine),
                                            ngood);

                        uselist = modlist;

                        assert( cpl_bivector_get_size(offs_fine) == ngood );
                        assert( cpl_imagelist_get_size(uselist)  == ngood );
                    }

                    use_offs = offs_fine;
                }
            }
            cpl_vector_delete(correl);
        }
    }

    /* Shift & add */
    combined = cpl_geom_img_offset_saa(uselist, use_offs, CPL_KERNEL_DEFAULT,
                                       min_rej, max_rej, union_flag,
                                       NULL, NULL);
    cpl_bivector_delete(offs_fine);
    if (modlist != NULL) {
        cpl_size i = cpl_imagelist_get_size(modlist);

        while (i--) {
            (void)cpl_imagelist_unset(modlist, i);
        }
        cpl_imagelist_delete(modlist);
    }

    if (combined == NULL) {
        (void)cpl_error_set_message_(CPL_ERROR_DATA_NOT_FOUND, "Could not "
                                     "shift and add %" CPL_SIZE_FORMAT
                                     " images", nima);
    }

    return combined;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Shift and add an images list to a single image 
  @param    ilist       Input image list
  @param    offs        List of offsets in x and y
  @param    kernel      Interpolation kernel to use for resampling
  @param    rejmin      Number of minimum value pixels to reject in stacking
  @param    rejmax      Number of maximum value pixels to reject in stacking
  @param    union_flag  Combination mode, CPL_GEOM_UNION, CPL_GEOM_INTERSECT or
                        CPL_GEOM_FIRST
  @param    ppos_x      If non-NULL, *ppos_x is the X-position of the first
                        image in the combined image
  @param    ppos_y      If non-NULL, *ppos_y is the Y- position of the first
                        image in the combined image
  @return   Pointer to newly allocated images array, or NULL on error.

  The supported types are CPL_TYPE_DOUBLE, CPL_TYPE_FLOAT.
 
  The number of provided offsets shall be equal to the number of input images.
  The ith offset (offs_x, offs_y) is the offset that has to be used to shift 
  the ith image to align it on the first one.
  
  Provide the name of the kernel you want to generate. Supported kernel
  types are:
  - CPL_KERNEL_DEFAULT: default kernel, currently CPL_KERNEL_TANH
  - CPL_KERNEL_TANH: Hyperbolic tangent
  - CPL_KERNEL_SINC: Sinus cardinal
  - CPL_KERNEL_SINC2: Square sinus cardinal
  - CPL_KERNEL_LANCZOS: Lanczos2 kernel
  - CPL_KERNEL_HAMMING: Hamming kernel
  - CPL_KERNEL_HANN: Hann kernel
  - CPL_KERNEL_NEAREST: Nearest neighbor kernel (1 when dist < 0.5, else 0)

  If the number of input images is lower or equal to 3, the rejection 
  parameters are ignored.
  If the number of input images is lower or equal to 2*(rejmin+rejmax), the 
  rejection parameters are ignored.
  
  On success the returned image array contains 2 images:
  - the combined image
  - the contribution map

  Pixels with a zero in the contribution map are flagged as bad in the
  combined image.
  
  If not NULL, the returned cpl_image array arr must be deallocated like:
  @code
  if (arr[0] != NULL) cpl_image_delete(arr[0]);
  if (arr[1] != NULL) cpl_image_delete(arr[1]);
  cpl_free(arr);
  @endcode

  If the call is successful, (*ppos_x, *ppos_y) is the pixel coordinate in
  the created output image-pair where the lowermost-leftmost pixel of the
  first input image is located. So with CPL_GEOM_FIRST this will always be
  (1, 1).
  
  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if (one of) the input pointer(s) is NULL
  - CPL_ERROR_ILLEGAL_INPUT if ilist is invalid or if rejmin or rejmax is
    negative
  - CPL_ERROR_INCOMPATIBLE_INPUT if ilist and offs have different sizes
  - CPL_ERROR_ILLEGAL_OUTPUT if the CPL_GEOM_INTERSECT method is used with
      non-overlapping images.
  - CPL_ERROR_INVALID_TYPE if the passed image list type is not supported
  - CPL_ERROR_UNSUPPORTED_MODE if the union_flag is not one of the supported
    combination modes, which are @em CPL_GEOM_INTERSECT, @em CPL_GEOM_UNION,
    @em CPL_GEOM_FIRST.
 */
/*----------------------------------------------------------------------------*/
cpl_image ** cpl_geom_img_offset_saa(const cpl_imagelist * ilist,
                                     const cpl_bivector  * offs,
                                     cpl_kernel            kernel,
                                     cpl_size              rejmin,
                                     cpl_size              rejmax,
                                     cpl_geom_combine      union_flag,
                                     double              * ppos_x,
                                     double              * ppos_y)
{
    cpl_imagelist    * copy = (cpl_imagelist *)ilist; /* Not modified */
    cpl_bivector     * offscopy = (cpl_bivector *)offs; /* Not modified */
    const cpl_size     nima  = cpl_imagelist_get_size(ilist);
    const cpl_image  * img1  = cpl_imagelist_get_const(ilist, 0);
    const cpl_size     sizex = cpl_image_get_size_x(img1);
    const cpl_size     sizey = cpl_image_get_size_y(img1);
    const cpl_type     type  = cpl_image_get_type(img1);
    const cpl_vector * voffsx;
    const cpl_vector * voffsy;
    /* Test rejection parameters */
    const cpl_boolean  do_rej = nima > 3 && nima > 2*(rejmin+rejmax)
        ? CPL_TRUE : CPL_FALSE;
    const cpl_size     rmin = do_rej ? rejmin : 0;
    const cpl_size     rmax = do_rej ? rejmax : 0;
    const cpl_size     rtot = rmin + rmax;
    const cpl_size     tabsperpix = CPL_KERNEL_TABSPERPIX;
    cpl_vector       * xyprofile;
    cpl_size           nx, ny;
    double             start_x, start_y;
    cpl_image        * final;
    cpl_image        * contrib;
    cpl_image       ** out_arr;
    cpl_size           i_ima;
    cpl_size           firstbpm;


    cpl_ensure(ilist != NULL, CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(offs  != NULL, CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(cpl_imagelist_is_uniform(ilist) == 0, CPL_ERROR_ILLEGAL_INPUT,
               NULL);
    cpl_ensure(cpl_bivector_get_size(offs) == nima, CPL_ERROR_INCOMPATIBLE_INPUT,
               NULL);
    cpl_ensure(rejmin>=0, CPL_ERROR_ILLEGAL_INPUT, NULL);
    cpl_ensure(rejmax>=0, CPL_ERROR_ILLEGAL_INPUT, NULL);
    
   
    /* Compute output image size for union/intersection */
    voffsx = cpl_bivector_get_x_const(offs);
    voffsy = cpl_bivector_get_y_const(offs);

    if (union_flag == CPL_GEOM_UNION) {
        /* Union image */

        /* For this mode, the rejection arguments are also used
           to reduce the size of the resulting image */

        cpl_vector   * ox_tmp  = cpl_vector_duplicate(voffsx);
        cpl_vector   * oy_tmp  = cpl_vector_duplicate(voffsy);
        const double * ox_data = cpl_vector_get_data_const(ox_tmp);
        const double * oy_data = cpl_vector_get_data_const(oy_tmp);
        double         ox_max, oy_max;


        cpl_vector_sort(ox_tmp, 1);
        cpl_vector_sort(oy_tmp, 1);

        start_x = ox_data[rtot];
        start_y = oy_data[rtot];

        if (ppos_x != NULL) *ppos_x = 1.0 - start_x;
        if (ppos_y != NULL) *ppos_y = 1.0 - start_y;

        ox_max = ox_data[nima - rtot - 1];
        oy_max = oy_data[nima - rtot - 1];

        /* Round down */
        nx = (cpl_size)((double)sizex + ox_max - start_x);
        ny = (cpl_size)((double)sizey + oy_max - start_y);

        cpl_vector_delete(ox_tmp);
        cpl_vector_delete(oy_tmp);
    } else if (union_flag == CPL_GEOM_INTERSECT) {
        /* Intersection image */
        const double ox_min = cpl_vector_get_min(voffsx);
        const double oy_min = cpl_vector_get_min(voffsy);

        start_x = cpl_vector_get_max(voffsx);
        start_y = cpl_vector_get_max(voffsy);

        if (ppos_x != NULL) *ppos_x = start_x;
        if (ppos_y != NULL) *ppos_y = start_y;

        /* Round down */
        nx = (cpl_size)((double)sizex - start_x + ox_min);
        ny = (cpl_size)((double)sizey - start_y + oy_min);

        cpl_ensure(nx > 0, CPL_ERROR_ILLEGAL_OUTPUT, NULL);
        cpl_ensure(ny > 0, CPL_ERROR_ILLEGAL_OUTPUT, NULL);

    } else if (union_flag == CPL_GEOM_FIRST) {
        /* First image as reference */
        nx = sizex;
        ny = sizey;
        start_x = 0.0;
        start_y = 0.0;

        if (ppos_x != NULL) *ppos_x = 1.0;
        if (ppos_y != NULL) *ppos_y = 1.0;

        if (rtot == 0 && nima > 1) {
            /* Create shallow copies, without the 1st element */

            copy = cpl_imagelist_wrap_all_but_first(copy);
            offscopy = cpl_bivector_wrap_all_but_first(offscopy);

            /* copy and offscopy are NOT modified below */
        }

    } else {
        /* union_flag is not one of the three supported modes */
        (void)cpl_error_set_(CPL_ERROR_UNSUPPORTED_MODE);
        return NULL;
    }

    xyprofile = cpl_vector_new(CPL_KERNEL_DEF_SAMPLES);

    /* The resampling profile is for a resampling routine with a hard-coded
       radius of 2.0 */
    if (cpl_vector_fill_kernel_profile(xyprofile, kernel, 2.0)) {
        cpl_vector_delete(xyprofile);
        if (copy != ilist) cpl_imagelist_unwrap(copy);
        if (offscopy != offs) cpl_bivector_unwrap(offscopy);
        (void)cpl_error_set_where_();
        return NULL;
    }

    if (copy == ilist) {
        /* Create output image, initialized to zero */
        final = cpl_image_new(nx, ny, type);

        /* Create output contribution image, initialized to zero */
        contrib = cpl_image_new(nx, ny, CPL_TYPE_INT);
    } else {
        /* Create output image, duplicated from 1st image */
        final = cpl_image_duplicate(img1);

        if (cpl_image_get_bpm_const(final) != NULL) { /* Check for bpm */
            cpl_mask * mask1 = cpl_image_get_bpm(final);

            cpl_mask_not(mask1);
            contrib = cpl_image_new_from_mask(mask1);
            cpl_image_fill_rejected(final, 0.0);
            cpl_image_accept_all(final);
        } else {
            /* Create an integer image filled with 1's */
            int * dcontrib = cpl_malloc((size_t)(nx * ny) * sizeof(*dcontrib));

            contrib = cpl_image_wrap_int(nx, ny, dcontrib);
            (void)cpl_image_fill_int(contrib, 1);
        }
    }

    /* Find first plane with a non-empty bad pixel map - if any */
    firstbpm = nima;
    for(i_ima=0; i_ima < nima; i_ima++)
    {
        const cpl_mask * bpm
            = cpl_image_get_bpm_const(cpl_imagelist_get_const(ilist, i_ima));

        if(bpm != NULL && !cpl_mask_is_empty(bpm)) {
            firstbpm = i_ima;
            break;
        }
    }

    /* Switch on the data type */
    if (type == CPL_TYPE_DOUBLE) {
        if (rtot > 0 || firstbpm < nima) {
            cpl_geom_img_offset_saa_double(final, contrib, rmin, rmax,
                                           start_x, start_y,
                                           ilist, firstbpm, offs,
                                           xyprofile, tabsperpix);
        } else {
            cpl_geom_img_offset_saa_all_double(final, contrib,
                                               start_x, start_y,
                                               copy, offscopy,
                                               xyprofile, tabsperpix);
        }
    } else if (type == CPL_TYPE_FLOAT) {
        if (rtot > 0 || firstbpm < nima) {
            cpl_geom_img_offset_saa_float(final, contrib, rmin, rmax,
                                          start_x, start_y,
                                          ilist, firstbpm, offs,
                                          xyprofile, tabsperpix);
        } else {
            cpl_geom_img_offset_saa_all_float(final, contrib,
                                              start_x, start_y,
                                              copy, offscopy,
                                              xyprofile, tabsperpix);
        }
    } else {
            cpl_vector_delete(xyprofile);
            cpl_image_delete(final);
            cpl_image_delete(contrib);
            if (copy != ilist) cpl_imagelist_unwrap(copy);
            if (offscopy != offs) cpl_bivector_unwrap(offscopy);
            (void)cpl_error_set_(CPL_ERROR_INVALID_TYPE);
            return NULL;
    }

    cpl_vector_delete(xyprofile);

    out_arr = cpl_malloc(2*sizeof(cpl_image *));
    out_arr[0] = final;
    out_arr[1] = contrib;
    if (copy != ilist) cpl_imagelist_unwrap(copy);
    if (offscopy != offs) cpl_bivector_unwrap(offscopy);

    return out_arr;
}
  
/**@}*/

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Compute the median offset between 2 images
  @param    im1         First input image
  @param    im2         Second input image
  @param    anchors     List of cross-correlation points in the first image
  @param    s_hx        Search area half-width.
  @param    s_hy        Search area half-height.
  @param    m_hx        Measurement area half-width.
  @param    m_hy        Measurement area half-height.
  @param    offs_x      X offset estimate - updated with the computed offset.
  @param    offs_y      Y offset estimate - updated with the computed offset.
  @return   The minimal squared difference factor or -1.0 on error
  @note No input validation!
 
  This function compares two images using a 2d cross-correlation. The input 
  images are expected to be more or less the same, up to a shift in x and y. 
 
  (offs_x, offs_y) has to be provided by the user. It is used as a first 
  estimate, and updated with the computed subpixel precision offsets.
  The input/returned offsets (offs_x, offs_y) is the a priori offset that has 
  to be used to shift im2 to align it on im1.
 
  The computed offset has a subpixel precision.
  
  The cross-correlation is carried out for each anchor point given in the list.
  It is Ok to provide only one anchor point.
 
  For each point, a cross-correlation criterion will be computed on a grid of 
  2m_hx+1 by 2m_hy+1, on the search area defined by 2s_hx+1 by 2s_hy+1. The 
  total number of pixel operations is quite high: number of anchor points 
  times number of search area pixels times number of measurement area pixels.
  
  The supported types are CPL_TYPE_DOUBLE and CPL_TYPE_FLOAT and the two 
  input images have to be of the same type.
  The bad pixel maps are ignored by this function.

  Possible #_cpl_error_code_ set in this function:
   - CPL_ERROR_ILLEGAL_INPUT
   - CPL_ERROR_INVALID_TYPE
 */
/*----------------------------------------------------------------------------*/
static double cpl_geom_ima_offset_xcorr(const cpl_image    * im1,
                                        const cpl_image    * im2,
                                        const cpl_bivector * anchors,
                                        cpl_size             s_hx,
                                        cpl_size             s_hy,
                                        cpl_size             m_hx,
                                        cpl_size             m_hy,
                                        double             * offs_x,
                                        double             * offs_y)
{
    const cpl_size nx = cpl_image_get_size_x(im1);
    const cpl_size ny = cpl_image_get_size_y(im1);
    const cpl_size nanchors = cpl_bivector_get_size(anchors);
    const double * anchor_x = cpl_bivector_get_x_data_const(anchors);
    const double * anchor_y = cpl_bivector_get_y_data_const(anchors);
    cpl_bivector * delta    = cpl_bivector_new(nanchors);
    double       * delta_x  = cpl_bivector_get_x_data(delta);
    double       * delta_y  = cpl_bivector_get_y_data(delta);
    cpl_vector   * correl   = cpl_vector_new(nanchors);
    double       * correl_data = cpl_vector_get_data(correl);
    /* Temporary work array */
    double       * corr_tmp
        = cpl_malloc((size_t)(2*s_hx+1) * (size_t)(2*s_hy+1) * sizeof(double));
    double         best_xcorr = -1.0; /* Assume failure */
    cpl_size       nvalid = 0;
    cpl_size       i;


    /* Loop on all correlating points */
    for (i = 0; i < nanchors; i++) {
        const int x_1 = (int)(anchor_x[i]);
        const int y_1 = (int)(anchor_y[i]);
        const int x_2 = x_1 - (int)(*offs_x);
        const int y_2 = y_1 - (int)(*offs_y);

        correl_data[nvalid] = 0 <= x_2 && x_2 < nx && 0 <= y_2 && y_2 < ny
            ? cpl_geom_ima_offset_xcorr_subw(corr_tmp, im1, im2, x_1, y_1,
                                             x_2, y_2, s_hx, s_hy, m_hx, m_hy,
                                             delta_x+nvalid, delta_y+nvalid)
            : -1.0;

        if (correl_data[nvalid] >= 0.0) nvalid++;
    }

    cpl_free(corr_tmp);

    if (nvalid > 0) {  /* There are valid points */
        cpl_size best_rank = 0;

        if (nvalid > 1) {
            double median_dx, median_dy;
            double min_sqdist;

            if (nvalid < nanchors) {

                /* Resize for the median computation
                   - invalidates the data pointers! */
                cpl_vector_set_size(cpl_bivector_get_x(delta), nvalid);
                cpl_vector_set_size(cpl_bivector_get_y(delta), nvalid);

                delta_x = cpl_bivector_get_x_data(delta);
                delta_y = cpl_bivector_get_y_data(delta);

            }
   
            /* Compute a median offset from the correlations */
            median_dx = cpl_vector_get_median_const(cpl_bivector_get_x(delta));
            median_dy = cpl_vector_get_median_const(cpl_bivector_get_y(delta));

            /* Find the offset measurement closest to this median */
            min_sqdist = (delta_x[0]-median_dx) * (delta_x[0]-median_dx)
                + (delta_y[0]-median_dy) * (delta_y[0]-median_dy);

            for (i = 1; i < nvalid; i++) {
                const double sqdist
                    = (delta_x[i]-median_dx) * (delta_x[i]-median_dx)
                    + (delta_y[i]-median_dy) * (delta_y[i]-median_dy);

                if (sqdist < min_sqdist) {
                    min_sqdist = sqdist;
                    best_rank = i;
                }
            }
        }

        *offs_x = (int)(*offs_x) + delta_x[best_rank];
        *offs_y = (int)(*offs_y) + delta_y[best_rank];
        best_xcorr = correl_data[best_rank];

    }

    cpl_bivector_delete(delta);
    cpl_vector_delete(correl);

    return best_xcorr;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Correlate two images subwindow 
  @param    correl      Work-array for correlation
  @param    im1         First input image
  @param    im2         Second input image
  @param    im1_posx    X-coordinate of the subwindow center in im1
  @param    im1_posy    Y-coordinate of the subwindow center in im1
  @param    im2_posx    X-coordinate of the subwindow center in im2
  @param    im2_posy    Y-coordinate of the subwindow center in im2
  @param    s_hx        Search area half-width.
  @param    s_hy        Search area half-height.
  @param    m_hx        Measurement area half-width.
  @param    m_hy        Measurement area half-height.
  @param    offs_x      Returned apodised position in x.
  @param    offs_y      Returned apodised position in y.
  @return   The minimal squared difference or -1.0 on error
  @note No input validation!
 
  The computed offset has subpixel precision.
 
  The double returned by the function measures the lowest squared difference 
  between the two input images subwindows (in the search area and over the 
  measurement area as requested by the caller). It is often a good indicator 
  of how well the cross-correlation performed.

  (offs_x, offs_y) has to be subtracted to (im2_posx, im2_posy) to retrieve 
  the exact position of the point in im2 corresponding to the point at position
  (im1_posx, im1_posy).
  
  The supported types are CPL_TYPE_DOUBLE and CPL_TYPE_FLOAT and the two 
  input images have to be of the same type.
  The bad pixel maps are ignored by this function.
  
  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT
  - CPL_ERROR_ILLEGAL_INPUT
  - CPL_ERROR_INVALID_TYPE
 */
/*----------------------------------------------------------------------------*/
static double cpl_geom_ima_offset_xcorr_subw(double          * correl,
                                             const cpl_image * im1,
                                             const cpl_image * im2,
                                             cpl_size          im1_posx,
                                             cpl_size          im1_posy,
                                             cpl_size          im2_posx,
                                             cpl_size          im2_posy,
                                             cpl_size          s_hx,
                                             cpl_size          s_hy,
                                             cpl_size          m_hx,
                                             cpl_size          m_hy,
                                             double          * offs_x,
                                             double          * offs_y)
{
    const cpl_type type = cpl_image_get_type(im1);
    cpl_size       k_min = 0;
    cpl_size       l_min = 0;
    double         inc_x, inc_y;
    cpl_size       pos_min;
    double         best_correl;
    
    /* Switch on the data type */
    if (type == CPL_TYPE_DOUBLE) {
        cpl_geom_ima_offset_xcorr_subw_double(correl,
                                              im1,
                                              im2,
                                              im1_posx,
                                              im1_posy,
                                              im2_posx,
                                              im2_posy,
                                              s_hx,
                                              s_hy,
                                              m_hx,
                                              m_hy,
                                              &k_min, &l_min);
    } else if (type == CPL_TYPE_FLOAT) {
        cpl_geom_ima_offset_xcorr_subw_float(correl,
                                              im1,
                                              im2,
                                              im1_posx,
                                              im1_posy,
                                              im2_posx,
                                              im2_posy,
                                              s_hx,
                                              s_hy,
                                              m_hx,
                                              m_hy,
                                              &k_min, &l_min);
    } else {
        (void)cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        return -1.0;
    }

    /* Store results (pixel precision) */
    pos_min = s_hx + k_min + (s_hy + l_min) * (1 + 2 * s_hx);
    best_correl = correl[pos_min];

    /* Compute inc_x and inc_y (sub-pixel precision) */
    if ((k_min == -s_hx) || (k_min == s_hx)) inc_x = 0.0;
    else inc_x = 0.5*((correl[pos_min-1]-correl[pos_min+1])/
            (correl[pos_min-1]-(2.0*correl[pos_min])+correl[pos_min+1]));

    if ((l_min == -s_hy) || (l_min == s_hy)) inc_y = 0.0;
    else inc_y = 0.5*((correl[pos_min-(2*s_hx+1)]-correl[pos_min+(2*s_hx+1)])/
            (correl[pos_min-(2*s_hx+1)]-(2.0*correl[pos_min])+
             correl[pos_min+(2*s_hx+1)]));

    /* Return the sub-pixel precision offsets */
    *offs_x = (double)k_min + inc_x;
    *offs_y = (double)l_min + inc_y;

    return best_correl;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Use bubble-sort to find the rmin smallest and rmax largest values
  @param    a   the double array
  @param    n   the array size
  @param    rmin  The 1st rmin values are the smallest
  @param    rmax  The last rmax values are the largest
  @note No error checking!

  The number of floating-point comparisons (possibly followed by a swap) is
  n * r - r * (r + 1) / 2, where r = rmin + rmax, which is better than
  sorting as long as r is small (compared to log(n)).

 */
/*----------------------------------------------------------------------------*/
inline static void cpl_geom_img_get_min_max_double(double * a,
                                                   cpl_size n,
                                                   cpl_size rmin,
                                                   cpl_size rmax)
{

    double tmp;
    const cpl_size jeq = CX_MIN(rmin, rmax);
    cpl_size i, j;


    /* Gain some locality */
    for (j = 0; j < jeq; j++) {
        /* Bubble one minimum value into place */
        for (i = n-j-1; i > j; i--) {
            CPL_SORT(a, i, i-1, tmp);
        }
        /* Bubble one maximum value into place */
        for (i += 2; i < n - j; i++) {
            CPL_SORT(a, i, i-1, tmp);
        }
    }

    /* Will enter at most one of the below two loops */

    /* Bubble one minimum value into place */
    for (; j < rmin; j++) {
        for (i = n-rmax-1; i > j; i--) {
            CPL_SORT(a, i, i-1, tmp);
        }
    }

    /* Bubble one maximum value into place */
    for (; j < rmax; j++) {
        for (i = rmin+1; i < n - j; i++) {
            CPL_SORT(a, i, i-1, tmp);
        }
    }

    cpl_tools_add_flops( n * (rmin + rmax)
                         - ((rmin + rmax) * (rmin + rmax + 1)) / 2);
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Shallow copy of all but one image in a list
  @param    other  The source list
  @return The created image list on success

 */
/*----------------------------------------------------------------------------*/
static cpl_imagelist * cpl_imagelist_wrap_all_but_first(cpl_imagelist * other)
{

    cpl_imagelist * self;
    const cpl_size n = cpl_imagelist_get_size(other);
    cpl_size i;


    cpl_ensure(other != NULL, CPL_ERROR_NULL_INPUT, NULL);

    self = cpl_imagelist_new();

    for (i=1; i < n; i++) {
        cpl_image * copy = cpl_imagelist_get(other, i);
        (void)cpl_imagelist_set(self, copy, i-1);
    }

    return self;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Shallow copy of all but the 1st element in a bivector
  @param    self   The source bivector
  @return The created bivector on success

 */
/*----------------------------------------------------------------------------*/
static cpl_bivector * cpl_bivector_wrap_all_but_first(cpl_bivector * self)
{

    const cpl_size n = cpl_bivector_get_size(self);
    cpl_vector   * xvec;
    cpl_vector   * yvec;


    cpl_ensure(self != NULL, CPL_ERROR_NULL_INPUT,    NULL);
    cpl_ensure(n    >  1,    CPL_ERROR_ILLEGAL_INPUT, NULL);

    xvec = cpl_vector_wrap(n-1, 1 + cpl_bivector_get_x_data(self));
    yvec = cpl_vector_wrap(n-1, 1 + cpl_bivector_get_y_data(self));

    return cpl_bivector_wrap_vectors(xvec, yvec);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Shallow delete of all but one image in a list
  @param    self   The destination list
  @return void

 */
/*----------------------------------------------------------------------------*/
static void cpl_bivector_unwrap(cpl_bivector * self)
{
    if (self != NULL) {
        (void)cpl_vector_unwrap(cpl_bivector_get_x(self));
        (void)cpl_vector_unwrap(cpl_bivector_get_y(self));
        cpl_bivector_unwrap_vectors(self);
    }
}

/* Define the C-type dependent functions */

#define CPL_TYPE double
#include "cpl_geom_img_body.h"
#undef CPL_TYPE

#define CPL_TYPE float
#include "cpl_geom_img_body.h"
#undef CPL_TYPE
