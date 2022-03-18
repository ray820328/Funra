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

#define CPL_LVMQ_DISABLE_SANITY_CHECK

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include "cpl_vector_fit_impl.h"

#include "cpl_image_io_impl.h"
#include "cpl_image_bpm_impl.h"
#include "cpl_vector_impl.h"
#include "cpl_mask_impl.h"
#include "cpl_matrix_impl.h"
#include "cpl_polynomial_impl.h"
#include "cpl_math_const.h"

#include <errno.h>
#include <math.h>
/* Needed for memchr() */
#include <string.h>

/*----------------------------------------------------------------------------*/
/**
 * @defgroup cpl_fit High-level functions for non-linear fitting
 *
 * This module provides a routine for non-linear fitting.
 *
 * @par Synopsis:
 * @code
 *   #include "cpl_fit.h"
 * @endcode
 */
/*----------------------------------------------------------------------------*/
/**@{*/

/*-----------------------------------------------------------------------------
                                   Defines
 -----------------------------------------------------------------------------*/

/* Used in cpl_fit_imagelist_polynomial_find_block_size()
   - it only needs to be corrected if it about 10 times too small,
   or a few times too big */
#ifndef L2_CACHE_BYTES
#ifdef CPL_CPU_CACHE
#define L2_CACHE_BYTES CPL_CPU_CACHE
#else
#define L2_CACHE_BYTES 262144
#endif
#endif

/*-----------------------------------------------------------------------------
                        Private function prototypes
 -----------------------------------------------------------------------------*/

static
cpl_error_code cpl_fit_imagelist_polynomial_window_(cpl_imagelist       *,
                                                    const cpl_vector    *,
                                                    const cpl_imagelist *,
                                                    cpl_size, cpl_size,
                                                    cpl_size, cpl_boolean,
                                                    cpl_image           *)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(1,2,3)))
#endif
    ;

static cpl_error_code cpl_fit_imagelist_polynomial_bpm(cpl_imagelist *,
                                                       const cpl_mask *,
                                                       const cpl_vector *,
                                                       const cpl_imagelist *,
                                                       cpl_size, cpl_size,
                                                       cpl_size, cpl_image *);

static cpl_error_code cpl_fit_imagelist_polynomial_one(cpl_imagelist *,
                                                       cpl_polynomial *,
                                                       double *,
                                                       double *,
                                                       cpl_size, cpl_size,
                                                       const cpl_vector *,
                                                       const cpl_imagelist *,
                                                       cpl_size, cpl_size,
                                                       cpl_size, cpl_image *);

static cpl_error_code cpl_fit_imagelist_polynomial_double(cpl_imagelist *,
                                                          const cpl_matrix *,
                                                          const cpl_matrix *,
                                                          const cpl_vector *,
                                                          const cpl_imagelist *,
                                                          cpl_size, cpl_size,
                                                          const cpl_vector *,
                                                          double, cpl_image *);

static cpl_error_code cpl_fit_imagelist_polynomial_float(cpl_imagelist *,
                                                         const cpl_matrix *,
                                                         const cpl_matrix *,
                                                         const cpl_vector *,
                                                         const cpl_imagelist *,
                                                         cpl_size, cpl_size,
                                                         const cpl_vector *,
                                                         double, cpl_image *);

static cpl_error_code cpl_fit_imagelist_polynomial_int(cpl_imagelist *,
                                                       const cpl_matrix *,
                                                       const cpl_matrix *,
                                                       const cpl_vector *,
                                                       const cpl_imagelist *,
                                                       cpl_size, cpl_size,
                                                       const cpl_vector *,
                                                       double, cpl_image *);

static void cpl_fit_imagelist_residual_double(cpl_image *, cpl_size, cpl_size,
                                              const cpl_vector *,
                                              const cpl_vector *,
                                              const cpl_matrix *,
                                              const cpl_matrix *);

static void cpl_fit_imagelist_residual_float(cpl_image *, cpl_size, cpl_size,
                                             const cpl_vector *,
                                             const cpl_vector *,
                                             const cpl_matrix *,
                                             const cpl_matrix *);

static void cpl_fit_imagelist_residual_int(cpl_image *, cpl_size, cpl_size,
                                           const cpl_vector *,
                                           const cpl_vector *,
                                           const cpl_matrix *,
                                           const cpl_matrix *);

static void cpl_fit_imagelist_fill_double(cpl_imagelist *, cpl_size, cpl_size,
                                          const cpl_matrix *);
 
static void cpl_fit_imagelist_fill_float(cpl_imagelist *, cpl_size, cpl_size,
                                         const cpl_matrix *);
 
static void cpl_fit_imagelist_fill_int(cpl_imagelist *, cpl_size, cpl_size,
                                       const cpl_matrix *);

static cpl_size cpl_fit_imagelist_polynomial_find_block_size(cpl_size,
                                                             cpl_size,
                                                             cpl_boolean,
                                                             cpl_type,
                                                             cpl_type,
                                                             cpl_type);

static int bigauss(const double[], const double[], double *) CPL_ATTR_NONNULL;
static int bigauss_derivative(const double[], const double[], double[])
    CPL_ATTR_NONNULL;


/*-----------------------------------------------------------------------------
                              Function code
 -----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/**
   @brief   Fit a function to a set of data
   @param   x        N x D matrix of the positions to fit.
                     Each matrix row is a D-dimensional position.
   @param   sigma_x  Uncertainty (one sigma, gaussian errors assumed)
                     assosiated with @em x. Taking into account the
             uncertainty of the independent variable is currently
             unsupported, and this parameter must therefore be set
             to NULL.
   @param   y        The N values to fit.
   @param   sigma_y  Vector of size N containing the uncertainties of
                     the y-values. If this parameter is NULL, constant
             uncertainties are assumed.
   @param   a        Vector containing M fit parameters. Must contain
                     a guess solution on input and contains the best
             fit parameters on output.
   @param   ia       Array of size M defining which fit parameters participate
                     in the fit (non-zero) and which fit parameters are held
             constant (zero). At least one element must be non-zero.
             Alternatively, pass NULL to fit all parameters.
   @param   f        Function that evaluates the fit function
                     at the position specified by the first argument (an array of
             size D) using the fit parameters specified by the second
             argument (an array of size M). The result must be output
             using the third parameter, and the function must return zero
             iff the evaluation succeded.
   @param   dfda     Function that evaluates the first order partial
                     derivatives of the fit function with respect to the fit
             parameters at the position specified by the first argument
             (an array of size D) using the parameters specified by the
             second argument (an array of size M). The result must
             be output using the third parameter (array of size M), and
             the function must return zero iff the evaluation succeded.
   @param relative_tolerance
                     The algorithm converges by definition if the relative
                     decrease in chi squared is less than @em tolerance
                     @em tolerance_count times in a row. Recommended default:
                     CPL_FIT_LVMQ_TOLERANCE
   @param tolerance_count
                     The algorithm converges by definition if the relative
                     decrease in chi squared is less than @em tolerance
                     @em tolerance_count times in a row. Recommended default:
                     CPL_FIT_LVMQ_COUNT
   @param max_iterations
                     If this number of iterations is reached without convergence,
                     the algorithm diverges, by definition. Recommended default:
                     CPL_FIT_LVMQ_MAXITER
   @param mse        If non-NULL, the mean squared error of the best fit is
                     computed.
   @param red_chisq  If non-NULL, the reduced chi square of the best fit is
                     computed. This requires @em sigma_y to be specified.
   @param covariance If non-NULL, the formal covariance matrix of the best
                     fit parameters is computed (or NULL on error). On success
             the diagonal terms of the covariance matrix are guaranteed
             to be positive. However, terms that involve a constant
             parameter (as defined by the input array @em ia) are
             always set to zero. Computation of the covariacne matrix
             requires @em sigma_y to be specified.


   @return  CPL_ERROR_NONE iff OK.

   This function makes a minimum chi squared fit of the specified function
   to the specified data set using a Levenberg-Marquardt algorithm.

   Possible #_cpl_error_code_ set in this function:
   - CPL_ERROR_NULL_INPUT if an input pointer other than @em sigma_x, @em
     sigma_y, @em mse, @em red_chisq or @em covariance is NULL.
   - CPL_ERROR_ILLEGAL_INPUT if an input matrix/vector is empty, if @em ia
     contains only zero values, if any of @em relative_tolerance,
     @em tolerance_count or max_iterations @em is non-positive, if N <= M
     and @em red_chisq is non-NULL, if any element of @em sigma_x or @em sigma_y
     is non-positive, or if evaluation of the fit function or its derivative
     failed.
   - CPL_ERROR_INCOMPATIBLE_INPUT if the dimensions of the input
     vectors/matrices do not match, or if chi square or covariance computation
     is requested and @em sigma_y is NULL.
   - CPL_ERROR_ILLEGAL_OUTPUT if memory allocation failed.
   - CPL_ERROR_CONTINUE if the Levenberg-Marquardt algorithm failed to converge.
   - CPL_ERROR_SINGULAR_MATRIX if the covariance matrix could not be computed.

*/
/*----------------------------------------------------------------------------*/

cpl_error_code
cpl_fit_lvmq(const cpl_matrix *x, const cpl_matrix *sigma_x,
             const cpl_vector *y, const cpl_vector *sigma_y,
             cpl_vector *a, const int ia[],
             int    (*f)(const double x[], const double a[], double *result),
             int (*dfda)(const double x[], const double a[], double result[]),
             double relative_tolerance,
             int tolerance_count,
             int max_iterations,
             double *mse,
             double *red_chisq,
             cpl_matrix **covariance)
{

    return cpl_fit_lvmq_(x, sigma_x, y, sigma_y, a, ia, NULL, NULL, f, dfda,
                         relative_tolerance, tolerance_count,
                         max_iterations, mse, red_chisq, covariance)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief  Least-squares fit a polynomial to each pixel in a list of images
  @param  x_pos      The vector of positions to fit
  @param  values     The list of images with values to fit
  @param  llx        Lower left x position (FITS convention, 1 for leftmost)
  @param  lly        Lower left y position (FITS convention, 1 for lowest)
  @param  urx        Upper right x position (FITS convention)
  @param  ury        Upper right y position (FITS convention)
  @param  mindeg     The smallest degree with a non-zero coefficient
  @param  maxdeg     The polynomial degree of the fit, at least mindeg
  @param  is_symsamp True iff the x_pos values are symmetric around their mean
  @param  pixeltype  The (non-complex) pixel-type of the created image list
  @param  fiterror   When non-NULL, the error of the fit. Must be non-complex
  @note   values and x_pos must have the same number of elements.
  @note   The created imagelist must be deallocated with cpl_imagelist_delete().
  @note   x_pos must have at least 1 + (maxdeg - mindeg) distinct values.
  @return The image list of the fitted polynomial coefficients or NULL on error.
  @see cpl_polynomial_fit()

  For each pixel, a polynomial representing the relation value = P(x) is
  computed where:
  P(x) = x^{mindeg} * (a_0 + a_1 * x + ... + a_{nc-1} * x^{nc-1}),
  where mindeg >= 0 and maxdeg >= mindeg, and nc is the number of
  polynomial coefficients to determine, nc = 1 + (maxdeg - mindeg).

  The returned image list thus contains nc coefficient images,
  a_0, a_1, ..., a_{nc-1}.

  np is the number of sample points, i.e. the number of elements in x_pos
  and number of images in the input image list.

  If mindeg is nonzero then is_symsamp is ignored, otherwise
  is_symsamp may to be set to CPL_TRUE if and only if the values in x_pos are
  known a-priori to be symmetric around their mean, e.g. (1, 2, 4, 6, 10,
  14, 16, 18, 19), but not (1, 2, 4, 6, 10, 14, 16). Setting is_symsamp to
  CPL_TRUE while mindeg is zero eliminates certain round-off errors.
  For higher order fitting the fitting problem known as "Runge's phenomenon"
  is minimized using the socalled "Chebyshev nodes" as sampling points.
  For Chebyshev nodes is_symsamp can be set to CPL_TRUE.

  Even though it is not an error, it is hardly useful to use an image of pixel
  type integer for the fitting error. An image of pixel type float should on
  the other hand be sufficient for most fitting errors.

  The call requires the following number of FLOPs, where
  nz is the number of pixels in any one image in the imagelist:

  2 * nz * nc * (nc + np) + np * nc^2 + nc^3/3 + O(nc * (nc + np)).

  If mindeg is zero an additional nz * nc^2 FLOPs are required.

  If fiterror is non-NULL an additional 2 * nz * nc * np FLOPs are required.

  Bad pixels in the input is suported as follows:
  First all pixels are fitted ignoring any bad pixel maps in the input. If
  this succeeds then each fit, where bad pixel(s) are involved is redone.
  During this second pass all input pixels flagged as bad are ignored.
  For each pixel to be redone, the remaining good samples are passed to
  cpl_polynomial_fit(). The input is_symsamp is ignored in this second pass.
  The reduced number of samples may reduce the number of sampling points to
  equal the number of coefficients to fit. In this case the fit has another
  meaning (any non-zero residual is due to rounding errors, not a fitting
  error). If for a given fit bad pixels reduces the number of sampling points
  to less than the number of coefficients to fit, then as many coefficients are
  fit as there are sampling points. The higher order coefficients are set to
  zero and flagged as bad. If a given pixel has no good samples, then the
  resulting fit will consist of zeroes, all flagged as bad.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input const pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if mindeg is negative or maxdeg is less than mindeg
      or if llx or lly are smaller than 1 or if urx or ury is smaller than
      llx and lly respectively.
  - CPL_ERROR_ACCESS_OUT_OF_RANGE if urx or ury exceed the size of values.
  - CPL_ERROR_INCOMPATIBLE_INPUT if x_pos and values have different lengths,
      or if fiterror is non-NULL with a different size than that of values,
      or if the input images do not all have the same dimensions and pixel type.
  - CPL_ERROR_DATA_NOT_FOUND if x_pos contains less than nc values.
  - CPL_ERROR_SINGULAR_MATRIX if x_pos contains less than nc distinct values.
  - CPL_ERROR_UNSUPPORTED_MODE if the chosen pixel type is not one of
        CPL_TYPE_DOUBLE, CPL_TYPE_FLOAT, CPL_TYPE_INT.
 */
/*----------------------------------------------------------------------------*/
cpl_imagelist * 
cpl_fit_imagelist_polynomial_window(const cpl_vector    * x_pos,
                                    const cpl_imagelist * values,
                                    cpl_size              llx,
                                    cpl_size              lly,
                                    cpl_size              urx,
                                    cpl_size              ury,
                                    cpl_size              mindeg,
                                    cpl_size              maxdeg,
                                    cpl_boolean           is_symsamp,
                                    cpl_type              pixeltype,
                                    cpl_image           * fiterror)
{

    cpl_imagelist   * self;
    cpl_mask        * redo = NULL;

    const cpl_image * first = cpl_imagelist_get_const(values, 0);
    const cpl_size    mx    = cpl_image_get_size_x(first);
    const cpl_size    my    = cpl_image_get_size_y(first);

    cpl_error_code   error = CPL_ERROR_NONE;

    /* Number of unknowns to determine */
    const cpl_size    nc = 1 + maxdeg - mindeg;
    const cpl_size    np = cpl_vector_get_size(x_pos);
    const cpl_size    nx = urx - llx + 1;
    const cpl_size    ny = ury - lly + 1;

    cpl_size          i;


    cpl_ensure(x_pos  != NULL, CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(values != NULL, CPL_ERROR_NULL_INPUT, NULL);

    cpl_ensure(mindeg >= 0,      CPL_ERROR_ILLEGAL_INPUT, NULL);
    cpl_ensure(maxdeg >= mindeg, CPL_ERROR_ILLEGAL_INPUT, NULL);

    cpl_ensure(np == cpl_imagelist_get_size(values),
               CPL_ERROR_INCOMPATIBLE_INPUT, NULL);

    cpl_ensure(cpl_imagelist_is_uniform(values)==0,
               CPL_ERROR_INCOMPATIBLE_INPUT, NULL);

    cpl_ensure(pixeltype == CPL_TYPE_DOUBLE || pixeltype == CPL_TYPE_FLOAT ||
               pixeltype == CPL_TYPE_INT, CPL_ERROR_UNSUPPORTED_MODE, NULL);

    if (fiterror != NULL) {
        cpl_ensure(cpl_image_get_size_x(fiterror) == nx &&
                   cpl_image_get_size_y(fiterror) == ny,
                   CPL_ERROR_INCOMPATIBLE_INPUT, NULL);
    }

    cpl_ensure(np >= nc,   CPL_ERROR_DATA_NOT_FOUND, NULL);

    cpl_ensure(llx >= 1 && llx <= urx, CPL_ERROR_ILLEGAL_INPUT, NULL);
    cpl_ensure(lly >= 1 && lly <= ury, CPL_ERROR_ILLEGAL_INPUT, NULL);

    cpl_ensure(urx <= mx, CPL_ERROR_ACCESS_OUT_OF_RANGE, NULL);
    cpl_ensure(ury <= my, CPL_ERROR_ACCESS_OUT_OF_RANGE, NULL);


    /* The Hankel matrix may be singular in such a fashion, that the pivot
       points in its Cholesky decomposition are positive due to rounding errors.
       To ensure that such singular systems are robustly detected, the number of
       distinct sampling points is counted.
    */

    cpl_ensure(!cpl_vector_ensure_distinct(x_pos, nc),
               CPL_ERROR_SINGULAR_MATRIX, NULL);
      

    /* Allocate nc images to store the results */
    self = cpl_imagelist_new();
    for (i=0; i < nc; i++) {
        cpl_image * image = cpl_image_wrap(nx, ny, pixeltype, cpl_malloc
                                           ((size_t)nx * (size_t)ny
                                            * cpl_type_get_sizeof(pixeltype)));
 
        (void)cpl_imagelist_set(self, image, i);
    }


    /* Find the bad input pixels and create a map of bpm-interpolations */
    for (i = 0; i < np; i++) {
        const cpl_image * img  = cpl_imagelist_get_const(values, i);
        const cpl_mask  * mask = cpl_image_get_bpm_const(img);

        if (mask != NULL) {
            if (redo == NULL) {
                redo = cpl_mask_extract_(mask, llx, lly, urx, ury);
            } else if (nx == mx && ny == my) {
                /* The below extraction is not needed */
                cpl_mask_or(redo, mask);
            } else {
                cpl_mask * window = cpl_mask_extract_(mask, llx, lly, urx, ury);
                if (window != NULL) {
                    cpl_mask_or(redo, window);
                    cpl_mask_delete(window);
                }
            }
        }
    }

    if (redo == NULL || cpl_mask_get_first_window(redo, 1, 1, nx, ny,
                                                  CPL_BINARY_0) >= 0) {
        /* Some (or all) interpolations are free of bad pixels */

        error = cpl_fit_imagelist_polynomial_window_(self, x_pos, values,
                                                     llx, lly, mindeg,
                                                     is_symsamp, fiterror);
    }


    if (!error && redo != NULL
        && cpl_mask_get_first_window(redo, 1, 1, nx, ny,
                                     CPL_BINARY_1) >= 0) {
        /* Some (or all) interpolations have bad pixels */

        error = cpl_fit_imagelist_polynomial_bpm(self, redo, x_pos, values,
                                                 llx, lly, mindeg, fiterror);
    }

    cpl_mask_delete(redo);

    if (error) {
        cpl_error_set_where_();
        cpl_imagelist_delete(self);
        self = NULL;
    }

    return self;
}



/*----------------------------------------------------------------------------*/
/**
  @brief  Least-squares fit a polynomial to each pixel in a list of images
  @param  self       The polynomiums as images, first has mindeg coefficients
  @param  x_pos      The vector of positions to fit
  @param  values     The list of images with values to fit
  @param  llx        Lower left x position (FITS convention, 1 for leftmost)
  @param  lly        Lower left y position (FITS convention, 1 for lowest)
  @param  mindeg     The smallest degree with a non-zero coefficient
  @param  is_symsamp True iff the x_pos values are symmetric around their mean
  @param  fiterror   When non-NULL, the error of the fit. Must be non-complex
  @note   values and x_pos must have the same number of elements.
  @note   The created imagelist must be deallocated with cpl_imagelist_delete().
  @note   x_pos must have at least 1 + (maxdeg - mindeg) distinct values.
  @return The image list of the fitted polynomial coefficients or NULL on error.
  @see cpl_fit_imagelist_polynomial_window()
  @note Ignores bad pixel maps in input
 */
/*----------------------------------------------------------------------------*/
static cpl_error_code
cpl_fit_imagelist_polynomial_window_(cpl_imagelist       * self,
                                     const cpl_vector    * x_pos,
                                     const cpl_imagelist * values,
                                     cpl_size              llx,
                                     cpl_size              lly,
                                     cpl_size              mindeg,
                                     cpl_boolean           is_symsamp,
                                     cpl_image           * fiterror)
{

    const cpl_size    nc = cpl_imagelist_get_size(self);
    const cpl_size    np = cpl_vector_get_size_(x_pos);
    cpl_matrix      * mv;   /* The transpose of the Vandermonde matrix, V' */
    cpl_matrix      * mh;   /* Upper triangular part of SPD Hankel matrix,
                            H = V' * V */
    const cpl_boolean is_eqzero = is_symsamp && mindeg == 0;
    const cpl_vector * xhat;
    cpl_vector       * xtmp = NULL;
    const double     * dx;
    double           * dmv;
    double            xmean;
    cpl_error_code    error;
    int i, j, k;


    if (mindeg == 0) {
        /* Transform: xhat = x - mean(x) */
        xhat = xtmp = cpl_vector_transform_mean(x_pos, &xmean);
    } else {
        xhat = x_pos;
        xmean = 0.0;
    }

    dx = cpl_vector_get_data_const_(xhat);

    /* Create matrices */
    dmv = (double*)cpl_malloc((size_t)nc * (size_t)np * sizeof(*dmv));
    mv  = cpl_matrix_wrap(nc, np, dmv);

    /* Fill Vandermonde matrix */
    for (j=0; j < np; j++) {
        double f_prod = cpl_tools_ipow(dx[j], (int)mindeg);
        dmv[j] = f_prod;
        for (k=1; k < nc; k++) {
            f_prod *= dx[j];
            dmv[np * k + j] = f_prod;
        }
    }

    cpl_tools_add_flops( (cpl_flops)(np * ( nc - 1)));

    cpl_vector_delete(xtmp);

    /* Form upper triangular part of the matrix of the normal equations,
       H = V' * V.
       As in cpl_polynomial_fit_1d_create() this could be done in
       O(nc * np) flops, rather than 2 * nc^2 * np, but this is
       negligible for any practical image size and is not done since
       mv still has to be formed in order to block-optimize the formation
       of the right-hand-size */
    mh = cpl_matrix_product_normal_create(mv);

    if (is_eqzero) {

        /* Ensure that the Hankel matrix has zeros on all odd skew diagonals
           - above the (non-skew) main diagonal */

        double * dmh = cpl_matrix_get_data(mh);

        for (i = 0; i < nc; i++) {
            for (j = i + 1; j < nc; j += 2) {
                dmh[nc * i + j] = 0.0;
            }
        }
    }

    /* Do an in-place Cholesky-decomposition of H into L, such that L * L' = H.
       This is an O(nc^3) operation, while the subsequent, repeated solve using
       L is only an O(nc^2) operation.
       Further, while the Cholesky-decomposition may fail, the subsequent solve
       is robust. */
    error = cpl_matrix_decomp_chol(mh);

    if (!error) {
        const cpl_image * first = cpl_imagelist_get_const(values, 0);

        const cpl_vector * xpow = NULL;
        xtmp = NULL;

        /* Should not be able to fail at this point */

        if (mindeg == 1) {
            xpow = x_pos;
        } if (mindeg > 1) {
            const double * d_pos = cpl_vector_get_data_const_(x_pos);
            double       * ppow  = (double*)cpl_malloc((size_t)np
                                                       * sizeof(*ppow));

            xpow = xtmp = cpl_vector_wrap(np, ppow);

            for (i = 0; i < np; i++) {
                ppow[i] = cpl_tools_ipow(d_pos[i], (int)mindeg);
            }

        }

        switch (cpl_image_get_type(first)) {
        case CPL_TYPE_DOUBLE:
            error = cpl_fit_imagelist_polynomial_double(self, mh, mv, x_pos,
                                                        values, llx, lly,
                                                        xpow, -xmean, fiterror);
            break;
        case CPL_TYPE_FLOAT:
            error = cpl_fit_imagelist_polynomial_float(self, mh, mv, x_pos,
                                                       values, llx, lly,
                                                       xpow, -xmean, fiterror);
            break;
        case CPL_TYPE_INT:
            error = cpl_fit_imagelist_polynomial_int(self, mh, mv, x_pos,
                                                     values, llx, lly,
                                                     xpow, -xmean, fiterror);
            break;
        default:
            error = CPL_ERROR_UNSUPPORTED_MODE;
            break;
        }

        cpl_vector_delete(xtmp);

    }

    cpl_matrix_delete(mh);
    cpl_matrix_delete(mv);

    return error ? cpl_error_set_where_() : CPL_ERROR_NONE;

}

/*----------------------------------------------------------------------------*/
/**
  @brief  Least-squares fit a polynomial to each pixel in a list of images
  @param  x_pos      The vector of positions to fit
  @param  values     The list of images with values to fit
  @param  mindeg     The smallest degree with a non-zero coefficient
  @param  maxdeg     The polynomial degree of the fit, at least mindeg
  @param  is_symsamp True iff the x_pos values are symmetric around their mean
  @param  pixeltype  The pixel-type of the created image list
  @param  fiterror   When non-NULL, the error of the fit
  @note   values and x_pos must have the same number of elements.
  @note   The created imagelist must be deallocated with cpl_imagelist_delete().
  @note   x_pos must have at least 1 + (maxdeg - mindeg) distinct values.
  @return The image list of the fitted polynomial coefficients or NULL on error.
  @see cpl_fit_imagelist_polynomial_window()

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input const pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if mindeg is negative or maxdeg is less than mindeg.
  - CPL_ERROR_INCOMPATIBLE_INPUT if x_pos and values have different lengths,
      or if fiterror is non-NULL with a different size than that of values,
      or if the input images do not all have the same dimensions and pixel type.
  - CPL_ERROR_DATA_NOT_FOUND if x_pos contains less than nc values.
  - CPL_ERROR_SINGULAR_MATRIX if x_pos contains less than nc distinct values.
  - CPL_ERROR_UNSUPPORTED_MODE if the chosen pixel type is not one of
        CPL_TYPE_DOUBLE, CPL_TYPE_FLOAT, CPL_TYPE_INT.
 */
/*----------------------------------------------------------------------------*/
cpl_imagelist * cpl_fit_imagelist_polynomial(const cpl_vector    * x_pos,
                                             const cpl_imagelist * values,
                                             cpl_size              mindeg,
                                             cpl_size              maxdeg,
                                             cpl_boolean           is_symsamp,
                                             cpl_type              pixeltype,
                                             cpl_image           * fiterror)
{
    const cpl_image * first = cpl_imagelist_get_const(values, 0);
    const cpl_size    nx    = cpl_image_get_size_x(first);
    const cpl_size    ny    = cpl_image_get_size_y(first);

    cpl_imagelist * self
        = cpl_fit_imagelist_polynomial_window(x_pos, values, 1, 1, nx, ny,
                                              mindeg, maxdeg, is_symsamp,
                                              pixeltype, fiterror);

    /* Propagate error, if any */
    if (self == NULL) (void)cpl_error_set_where_();

    return self;
}


/*
 * Section about bivariate gaussian fitting
 */

/*
 * @internal
 * @brief   Evaluate a bivariate gaussian distribution
 *
 * @param   x             The evaluation point
 * @param   a             The parameters defining the gaussian
 * @param   result        The function value, or undefined on error
 *
 * @return  0 if okay, negative on e.g. domain error
 *
 * @note The prototype of this function is given by cpl_fit_lvmq.
 * @see cpl_fit_lvmq
 *
 * This function computes
 *
 * @code
 * a0 +  a1 * exp(-0.5/(1-a2*a2){[(x0-a3)/a5]^2 - 2*a2*(x0-a3)/a5*(x1-a4)/a6
 *    +  [(x1-a4)/a6]^2}
 * @endcode
 *
 * where 
 * @code
 *     a0 = background level
 *     a1 = max of gaussian
 *     a2 = correlation xy
 *     a3 = x position of max 
 *     a4 = y position of max 
 *     a5 = x sigma
 *     a6 = y sigma 
 * @endcode
 *
 * This function fails if a5 = 0 or if a6 = 0 or if |a2| >= 1.
 *
 */

static int
bigauss(const double x[], const double a[], double *result)
{
    if (a[5] != 0.0 && a[6] != 0.0 && 1.0 - a[2] * a[2] > 0.0) {
        const double b2 = (x[0] - a[3]) / a[5];
        const double b3 = (x[1] - a[4]) / a[6];

        *result = a[0]
            + a[1] / (CPL_MATH_2PI * a[5] * a[6] * sqrt(1.0 - a[2] * a[2]))
            * exp(-0.5 * (b2 * b2 - 2 * a[2] * b2 * b3 + b3 * b3)
                  / (1.0 - a[2] * a[2]));

        return 0;
    }

    return -1;
}

/*
 * @internal
 * @brief   Evaluate the derivatives of a gaussian
 * @param   x           The evaluation point
 * @param   a           The parameters defining the gaussian
 * @param   result      The derivatives wrt to parameters, or undefined on error
 *
 * @return  0 if okay, negative on e.g. domain error
 *
 * @note The prototype of this function is given by cpl_fit_lvmq.
 * @see cpl_fit_lvmq
 *
 * The i-th element of the returned @em result vector contains df/da[i].
 *
 * This function fails if a5 = 0 or if a6 = 0 or if |a2| >= 1.
 */

static int
bigauss_derivative(const double x[], const double a[], double result[])
{
    errno = 0;

    if (a[5] != 0.0 && a[6] != 0.0 && 1.0 - a[2] * a[2] > 0.0) {
        const double b1 = 1.0 / (1.0 - a[2] * a[2]);
        const double b2 = (x[0] - a[3]) / a[5];
        const double b3 = (x[1] - a[4]) / a[6];
        const double b0 = b2 * b2 - 2 * a[2] * b2 * b3 + b3 * b3;
        const double b4 = exp(-0.5 * b1 * b0);
        const double b5 = sqrt(1.0 - a[2] * a[2]);
        const double b6 = CPL_MATH_PI * a[5] * a[6] * b5;
        const double b7 = 0.5 * a[1] * b4 / b6;

        result[0] = 1.0;
        result[1] = 0.5 * b4 / b6;
        result[2] = b7 * b1 * ((b2 * b3 - a[2] * b0 * b1) + a[2]);
        result[3] = b7 / a[5] * b1 * (b2 - a[2] * b3);
        result[4] = b7 / a[6] * b1 * (b3 - a[2] * b2);
        result[5] =-b7 / a[5] * (b1 * b2 * (a[2] * b3 - b2) + 1);
        result[6] =-b7 / a[6] * (b1 * b3 * (a[2] * b2 - b3) + 1);

        return errno ? -1 : 0;
    }

    return -1;
}


/**
 * @brief
 *    Fit a 2D gaussian to image values.
 *
 * @param im            Input image with data values to fit.
 * @param im_err        Optional input image with statistical errors
 *                      associated to data.
 * @param xpos          X position of center of fitting domain.
 * @param ypos          Y position of center of fitting domain.
 * @param xsize         X size of fitting domain. It must be at least 3 pixels.
 * @param ysize         Y size of fitting domain. It must be at least 3 pixels.
 * @param parameters    Preallocated array for returning the values of the
 *                      best-fit gaussian parameters (the parametrisation 
 *                      of the fitted gaussian is described in the main 
 *                      documentation section, below). This array must be 
 *                      of type CPL_TYPE_DOUBLE, and it must have exactly 
 *                      7 elements. 
 *                      Generally, when passed to this function, this array 
 *                      would not be initialised (all elements are "invalid").
 *                      A first-guess for the gaussian parameters is not 
 *                      mandatory: but it is possible to specify here
 *                      a first-guess value for each parameter. First-guess 
 *                      values can also be specified just for a subset of
 *                      parameters. 
 * @param err_params    Optional preallocated array for returning the 
 *                      statistical error associated to each fitted
 *                      parameter. This array must be of type CPL_TYPE_DOUBLE, 
 *                      and it must have exactly 7 elements. This makes 
 *                      mandatory to specify @em im_err. Note that the 
 *                      returned values are the square root of the diagonal 
 *                      elements (variances) of the @em covariance matrix 
 *                      (see ahead).
 * @param fit_params    Optional array, used for flagging the parameters to 
 *                      freeze. This array must be of type CPL_TYPE_INT, and 
 *                      it must have exactly 7 elements. If an array element 
 *                      is set to 0, the corresponding parameter will be 
 *                      frozen. Any other value (including an "invalid" 
 *                      array element) would indicate a free parameter. 
 *                      If a parameter is frozen, a first-guess value 
 *                      @em must be specified at the corresponding element 
 *                      of the @em parameters array. If no array is specified 
 *                      here (NULL pointer), all parameters are free.
 * @param rms           If not NULL, returned standard deviation of fit 
 *                      residuals.
 * @param red_chisq     If not NULL, returned reduced chi-squared of fit. 
 *                      This makes mandatory to specify @em im_err.
 * @param covariance    If not NULL, a newly allocated covariance matrix 
 *                      will be returned. This makes mandatory to specify
 *                      @em im_err. On error it is not modified.
 * @param major         If not NULL, returned semi-major axis of ellipse 
 *                      at 1-sigma.
 * @param minor         If not NULL, returned semi-minor axis of ellipse 
 *                      at 1-sigma.
 * @param angle         If not NULL, returned angle between X axis and 
 *                      major axis of ellipse, counted counterclockwise
 *                      (radians).
 * @param phys_cov      If not NULL, a newly allocated 3x3 covariance matrix 
 *                      for the derived physical parameters @em major, 
 *                      @em minor, and @em angle, will be returned. This 
 *                      makes mandatory to specify @em im_err. On error
 *                      it is not modified.
 *
 * @return CPL_ERROR_NONE on successful fit.
 *
 * This function fits a 2d gaussian to pixel values within a specified 
 * region by minimizing \f$\chi^2\f$ using a Levenberg-Marquardt algorithm.
 * The gaussian model adopted here is based on the well-known cartesian form
 *
 * \f[
 * z = B + \frac{A}{2 \pi \sigma_x \sigma_y \sqrt{1-\rho^2}} 
 * \exp\left({-\frac{1}{2\left(1-\rho^2\right)}
 * \left(\left(\frac{x - \mu_x}{\sigma_x}\right)^2 
 * -2\rho\left(\frac{x - \mu_x}{\sigma_x}\right)
 * \left(\frac{y - \mu_y}{\sigma_y}\right) 
 * + \left(\frac{y - \mu_y}{\sigma_y}\right)^2\right)}\right)
 * \f]
 * 
 * where \f$B\f$ is a background level and \f$A\f$ the volume of the
 * gaussian (they both can be negative!), making 7 parameters altogether. 
 * Conventionally the parameters are indexed from 0 to 6 in the elements 
 * of the arrays @em parameters, @em err_params, @em fit_params, and of 
 * the 7x7 @em covariance matrix:
 * 
 * \f{eqnarray*}{
 * \mathrm{parameters[0]} &=& B \\
 * \mathrm{parameters[1]} &=& A \\
 * \mathrm{parameters[2]} &=& \rho \\
 * \mathrm{parameters[3]} &=& \mu_x \\
 * \mathrm{parameters[4]} &=& \mu_y \\
 * \mathrm{parameters[5]} &=& \sigma_x \\
 * \mathrm{parameters[6]} &=& \sigma_y
 * \f}
 *
 * The semi-axes \f$a, b\f$ and the orientation \f$\theta\f$ of the
 * ellipse at 1-sigma level are finally derived from the fitting
 * parameters as:
 * \f{eqnarray*}{
 * \theta &=& \frac{1}{2} \arctan \left(2 \rho \frac{\sigma_x \sigma_y}
 *                        {\sigma_x^2 - \sigma_y^2}\right) \\
 * a &=& \sigma_x \sigma_y \sqrt{2(1-\rho^2) \frac{\cos 2\theta}
 *                         {\left(\sigma_x^2 + \sigma_y^2\right) \cos 2\theta
 *                         + \sigma_y^2 - \sigma_x^2}} \\
 * b &=& \sigma_x \sigma_y \sqrt{2(1-\rho^2) \frac{\cos 2\theta}
 *                         {\left(\sigma_x^2 + \sigma_y^2\right) \cos 2\theta
 *                         - \sigma_y^2 + \sigma_x^2}}
 * \f}
 *
 * Note that \f$\theta\f$ is counted counterclockwise starting from the
 * positive direction of the \f$x\f$ axis, ranging bewteen \f$-\pi/2\f$ and 
 * \f$+\pi/2\f$ radians.
 *
 * If the correlation \f$\rho = 0\f$ and \f$\sigma_x \geq \sigma_y\f$
 * (within uncertainties) the ellipse is either a circle or its major axis 
 * is aligned with the \f$x\f$ axis, so it is conventionally set
 *
 * \f{eqnarray*}{
 * \theta &=& 0 \\
 * a &=& \sigma_x \\
 * b &=& \sigma_y 
 * \f}
 *
 * If the correlation \f$\rho = 0\f$ and \f$\sigma_x < \sigma_y\f$
 * (within uncertainties) the major axis of the ellipse
 * is aligned with the \f$y\f$ axis, so it is conventionally set
 *
 * \f{eqnarray*}{
 * \theta &=& \frac{\pi}{2} \\
 * a &=& \sigma_y \\
 * b &=& \sigma_x 
 * \f}
 * 
 * If requested, the 3x3 covariance matrix G associated to the 
 * derived physical quantities is also computed, applying the usual
 * \f[
 *          \mathrm{G} = \mathrm{J} \mathrm{C} \mathrm{J}^\mathrm{T}
 * \f]
 * where J is the Jacobian of the transformation
 * \f$
 * (B, A, \rho, \mu_x, \mu_y, \sigma_x, \sigma_y) \rightarrow (\theta, a, b)
 * \f$
 * and C is the 7x7 matrix of the gaussian parameters.
 */

cpl_error_code cpl_fit_image_gaussian(const cpl_image *im,
                                      const cpl_image *im_err,
                                      cpl_size         xpos,
                                      cpl_size         ypos,
                                      cpl_size         xsize,
                                      cpl_size         ysize,
                                      cpl_array       *parameters,
                                      cpl_array       *err_params,
                                      const cpl_array *fit_params,
                                      double          *rms,
                                      double          *red_chisq,
                                      cpl_matrix     **covariance,
                                      double          *major,
                                      double          *minor,
                                      double          *angle,
                                      cpl_matrix     **phys_cov)
{
    const cpl_size mx = cpl_image_get_size_x(im);
    const cpl_size my = cpl_image_get_size_y(im);
    /* 
     * Extraction box 
     */

    const cpl_size llx = CX_MAX(xpos - xsize/2, 1);
    const cpl_size lly = CX_MAX(ypos - ysize/2, 1);
    const cpl_size urx = CX_MIN(xpos + xsize/2, mx);
    const cpl_size ury = CX_MIN(ypos + ysize/2, my);

    const cpl_size nx = urx - llx + 1;
    const cpl_size ny = ury - lly + 1;

    int     ia[] = {1, 1, 1, 1, 1, 1, 1};
    double  background;
    double  amplitude;
    double  normalisation;
    double  correlation;
    double  xcen, ycen;
    double  xsigma, ysigma;
    cpl_size ixcen, iycen;
    cpl_size nrow;
    int      i, j;
    int     invalid;
    cpl_boolean do_wrap = CPL_FALSE;
    cpl_boolean has_bpm;
    cpl_size    nfit    = 7; /* Number of parameters to fit, up to 7 */

    cpl_image     *ubox       = NULL;
    const cpl_image *box      = NULL;
    const cpl_image *ebox     = NULL;
    cpl_image     *fbox       = NULL;
    cpl_vector    *a          = NULL;
    double        *adata;
    cpl_size       acols;
    cpl_vector    *values     = NULL;
    cpl_vector    *dvalues    = NULL;
    cpl_matrix    *positions  = NULL;
    cpl_matrix    *own_cov    = NULL;
    double        *posi_data;
    double        *val_data;
    double        *dval_data = NULL;

    cpl_array     *lowersan   = NULL;
    cpl_array     *uppersan   = NULL;

    cpl_errorstate prestate = cpl_errorstate_get();
    cpl_error_code status = CPL_ERROR_NONE;


    /*
     * Always reset physical parameters
     */

    if (major ) *major  = 0.0;
    if (minor ) *minor  = 0.0;
    if (angle ) *angle  = 0.0;


    /*
     * Check input
     */

    if (im == NULL)
        return cpl_error_set_message_(CPL_ERROR_NULL_INPUT,
                                     "Missing input data image.");

    if (xpos  < 1  ||
        ypos  < 1  ||
        xpos  > mx ||
        ypos  > my ||
        xsize > mx ||
        ysize > my)
        return cpl_error_set_message_(CPL_ERROR_ACCESS_OUT_OF_RANGE,
                                     "Fitting box extends beyond image.");

    if (xsize < 3 || ysize < 3)
        return cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT,
                                     "Fitting box is too small.");

    if (im_err)
        if (cpl_image_get_size_x(im_err) != mx || 
            cpl_image_get_size_y(im_err) != my)
            return cpl_error_set_message_(CPL_ERROR_INCOMPATIBLE_INPUT,
                                     "Input images must have same size.");

    if (parameters == NULL)
        return cpl_error_set_message_(CPL_ERROR_NULL_INPUT,
                                     "Missing input parameters array.");

    if (cpl_array_get_type(parameters) != CPL_TYPE_DOUBLE)
        return cpl_error_set_message_(CPL_ERROR_INVALID_TYPE,
                                "Parameters array should be CPL_TYPE_DOUBLE.");

    if (err_params)
        if (cpl_array_get_type(err_params) != CPL_TYPE_DOUBLE)
            return cpl_error_set_message_(CPL_ERROR_INVALID_TYPE,
                          "Parameters error array should be CPL_TYPE_DOUBLE.");

    if (fit_params)
        if (cpl_array_get_type(fit_params) != CPL_TYPE_INT)
            return cpl_error_set_message_(CPL_ERROR_INVALID_TYPE,
                          "Parameters error array should be CPL_TYPE_INT.");

    if (err_params || covariance || red_chisq || phys_cov)
        if (im_err == NULL)
            return cpl_error_set_message_(CPL_ERROR_DATA_NOT_FOUND,
                                         "Missing input parameters errors.");

    switch(cpl_image_get_type(im)) { /* List of legal types, ended by break */
    case CPL_TYPE_DOUBLE:
        do_wrap = CPL_TRUE;
    case CPL_TYPE_INT:
    case CPL_TYPE_FLOAT:
        break;
    default:
        return cpl_error_set_message_(CPL_ERROR_INVALID_TYPE,
                                 "Cannot fit a gaussian to %s values",
                                 cpl_type_get_name(cpl_image_get_type(im)));
    }

    if (im_err && cpl_image_get_type(im) != cpl_image_get_type(im_err)) {
        return cpl_error_set_message_(CPL_ERROR_TYPE_MISMATCH,
                                      "Data and error images must have "
                                      "same type, not %s and %s",
                                      cpl_type_get_name(cpl_image_get_type(im)),
                                      cpl_type_get_name(cpl_image_get_type
                                                        (im_err)));
    }

    /* 
     * Extract box and error box (if present)
     */

    if (llx == 1 && lly == 1 && urx == mx && ury == my) {
        box = im;
    } else {
        box = ubox = cpl_image_extract(im, llx, lly, urx, ury);
    }

    if (im_err) {
        if (box == im) {
            ebox = im_err;
        } else {
            ebox = fbox = cpl_image_extract(im_err, llx, lly, urx, ury);
        }
    }


    /*
     * Ensure that frozen parameters have a value (first-guess)
     */

    if (fit_params) {
        for (i = 0; i < 7; i++) {
            const int flag = cpl_array_get_int(fit_params, i, &invalid);
            if (invalid || flag)
                continue;

            ia[i] = 0;  /* Flag it as frozen */
            nfit--;

            cpl_array_get_double(parameters, i, &invalid);
            if (invalid) {
                cpl_image_delete(ubox);
                cpl_image_delete(fbox);
                return cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT,
                       "Missing value for frozen parameter %d.", i);
            }
        }

        /*
         * Ensure that not all parameters are frozen
         */

        if (nfit < 1) {
            cpl_image_delete(ubox);
            cpl_image_delete(fbox);
            return cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT,
                                          "No free parameters");
        }
    }

    /*
     * Determine first-guess for gaussian parameters. Check if
     * provided by caller - if not build own guesses...
     *
     * Also, determine lower and upper sanity bounds on the free parameters,
     * to disqualify a non-sensical candidate solution during the fitting
     *
     * 0) Background level: if not given taken as median value within 
     *    fitting domain. It can be negative...
     */

    background = cpl_array_get_double(parameters, 0, &invalid);

    if (invalid)
        background = cpl_image_get_median(box);

    /*
     * 1) Normalisation is computed later on, since it depends
     *    on the sigma and the amplitude of the gaussian. Here
     *    is just a quick estimation, to know whether there is
     *    a peak or a hole. If it is flat, leave quickly this place...
     */


    normalisation = (cpl_image_get_mean(box) - background) * (double)(nx * ny);

    if (fabs(normalisation) < FLT_EPSILON) {

        /*
         * Image is flat: return a flat gaussian, with undefined
         * position of max and sigmas...
         */

        cpl_image_delete(ubox);
        cpl_image_delete(fbox);
        cpl_array_set_double (parameters, 0, background);
        cpl_array_set_double (parameters, 1, 0.0);
        cpl_array_set_invalid(parameters, 2);
        cpl_array_set_invalid(parameters, 3);
        cpl_array_set_invalid(parameters, 4);
        cpl_array_set_invalid(parameters, 5);
        cpl_array_set_double (parameters, 6, 0.0);
        return CPL_ERROR_NONE;
    }

    lowersan = cpl_array_new(7, CPL_TYPE_DOUBLE);
    uppersan = cpl_array_new(7, CPL_TYPE_DOUBLE);

    /*
     * 2) Correlation between x and y (tilted ellipse)
     */

    correlation = cpl_array_get_double(parameters, 2, &invalid);

    if (invalid)
        correlation = 0.0;

    if (ia[2]) {
        /* Correlation has hard boundaries */
        cpl_array_set_double(lowersan, 2, -1.0);
        cpl_array_set_double(uppersan, 2,  1.0);
    }

    /*
     * 3) and 4) Position of center.
     */

    if (normalisation > 0.0) {
        cpl_image_get_maxpos(box, &ixcen, &iycen);
        amplitude = cpl_image_get_(box, ixcen, iycen) - background;
    } else {
        cpl_image_get_minpos(box, &ixcen, &iycen);
        amplitude = cpl_image_get_(box, ixcen, iycen) - background;
    }

    xcen = cpl_array_get_double(parameters, 3, &invalid) - (double)(llx - 1);
    if (invalid)
        xcen = (double)ixcen;

    ycen = cpl_array_get_double(parameters, 4, &invalid) - (double)(lly - 1);
    if (invalid)
        ycen = (double)iycen;

    /*
     * 5) and 6) Sigma: if neither sigx nor sigy are given by the caller 
     *           the estimate for both is the distance (in pixel) of one 
     *           half-max (min) point from the max (min) point (here 
     *           conventionally made along the x direction). Very rough, 
     *           of course, but accuracy is not an issue on this phase 
     *           - we just need a rough starting value. If only one sig 
     *           is given by the caller, the other one is set to the same 
     *           value.
     */


    xsigma = cpl_array_get_double(parameters, 5, &invalid);
    if (invalid) {
        ysigma = cpl_array_get_double(parameters, 6, &invalid);
        if (invalid) {
            int           xhalf = 0;
            const double  value = amplitude / 2.0 + background;
            cpl_boolean   found = CPL_FALSE;

            if (normalisation > 0.0) {
                for (i = 1; i < nx; i++) {
                    int bad;
                    const double prev = cpl_image_get(box, i,     iycen, &bad);
                    const double next = bad ? 0.0 :
                        cpl_image_get(box, i + 1, iycen, &bad);

                    if (bad) continue;

                    if (prev < value && value <= next) {
                        xhalf = i - 1;
                        found = CPL_TRUE;
                        break;
                    } else if (value < prev && next <= value) {
                        xhalf = i + 1;
                        found = CPL_TRUE;
                        break;
                    }
                }
            } else {
                for (i = 1; i < nx; i++) {
                    int bad;
                    const double prev = cpl_image_get(box, i,     iycen, &bad);
                    const double next = bad ? 0.0 :
                        cpl_image_get(box, i + 1, iycen, &bad);

                    if (bad) continue;

                    if (value < prev && next <= value) {
                        xhalf = i - 1;
                        found = CPL_TRUE;
                        break;
                    } else if (prev < value && value <= next) {
                        xhalf = i + 1;
                        found = CPL_TRUE;
                        break;
                    }
                }
            }
            if (found) {
                xsigma = ysigma = fabs(xcen - xhalf);
            } else {
                xsigma = (double)xsize;
                ysigma = (double)ysize;
            }
        } else {
            xsigma = ysigma;
        }
    } else {
        ysigma = cpl_array_get_double(parameters, 6, &invalid);
        if (invalid) {
            ysigma = xsigma;
        }
    }

    if (ia[3]) {
        /* Do not allow the center to be far outside the box limit */
        cpl_array_set_double(lowersan, 3, 1.0 - 3.0 * CPL_MAX(xsize, xsigma));
        cpl_array_set_double(uppersan, 3, (double)nx + 3.0 * CPL_MAX(xsize, xsigma));
    }
    if (ia[4]) {
        /* Do not allow the center to be far outside the box limit */
        cpl_array_set_double(lowersan, 4, 1.0 - 3.0 * CPL_MAX(ysize, ysigma));
        cpl_array_set_double(uppersan, 4, (double)ny + 3.0 * CPL_MAX(ysize, ysigma));
    }

    if (ia[5]) {
        cpl_array_set_double(lowersan, 5, 0.0); /* Sigma must remain positive */
        /* The fit has nothing to work with if the slope is outside the box */
        cpl_array_set_double(uppersan, 5,
                             5.0 * CPL_MATH_FWHM_SIG * CPL_MAX(xsize, xsigma));
    }
    if (ia[6]) {
        cpl_array_set_double(lowersan, 6, 0.0); /* Sigma must remain positive */
        /* The fit has nothing to work with if the slope is outside the box */
        cpl_array_set_double(uppersan, 6, 
                             5.0 * CPL_MATH_FWHM_SIG * CPL_MAX(ysize, ysigma));
    }

    /*
     * 1) Normalisation. If not given by the user, it is derived
     *    from the max (min) value of the data distribution.
     */

    normalisation = cpl_array_get_double(parameters, 1, &invalid);

    if (invalid) {

        /*
         * It would be possible to guess the normalisation
         * (i.e., the volume of the gaussian, i.e. the total
         * flux of the star) by simply computing the flux excess
         * above background:
         *
         * normalisation = (cpl_image_get_mean(box) - background) * nx * ny;
         *
         * However this is not a good first-guess. There is a
         * correlation between the parameters, which are not
         * really independent from each other. With our guess 
         * for the sigma, we will choose a normalisation which 
         * would actually make the surface of the gaussian cross 
         * the observed point of max (min). So the first-guess 
         * would be "in touch" with the data, and there would be 
         * higher chance of convergence.
         */

        normalisation = amplitude * CPL_MATH_2PI * xsigma * ysigma;
    }

    if (ia[1]) {
        /* No sign change for the normalisation */

        if (normalisation > 0.0) {
            cpl_array_set_double(lowersan, 1, -normalisation);
        }

        if (normalisation < 0.0) {
            cpl_array_set_double(uppersan, 1, -normalisation);
        }
    }

    if (ia[0]) {
        /* Background cannot exceed amplitude */

        if (amplitude > 0.0) {
            cpl_array_set_double(lowersan, 0, -amplitude);
            cpl_array_set_double(uppersan, 0,  amplitude);
        } else if (amplitude < 0.0) {
            cpl_array_set_double(lowersan, 0,  amplitude);
            cpl_array_set_double(uppersan, 0, -amplitude);
        }
    }

    /*
     * Matrix with image positions and their values
     */

    nrow = nx * ny; /* Number of samples, less bad pixels */

    if (cpl_image_get_bpm_const(box) != NULL &&
        !cpl_mask_is_empty(cpl_image_get_bpm_const(box))) {
        has_bpm = CPL_TRUE;
        do_wrap = CPL_FALSE;
    } else if (ebox != NULL && cpl_image_get_bpm_const(ebox) != NULL &&
               !cpl_mask_is_empty(cpl_image_get_bpm_const(ebox))) {
        has_bpm = CPL_TRUE;
        do_wrap = CPL_FALSE;
    } else {
        has_bpm = CPL_FALSE;
    }

    /* Allocate all temporary data in one go */
    acols = do_wrap ? 2 : (ebox ? 4 : 3);
    adata = (double*)cpl_malloc((7 + acols * nrow) * sizeof(double));

    posi_data = adata + 7;

    if (do_wrap) {
        CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual);
        val_data  = (double*)cpl_image_get_data_double_const(box);
        if (ebox) dval_data = (double*)cpl_image_get_data_double_const(ebox);
        CPL_DIAG_PRAGMA_POP;
    } else {
        /* Arrays are oversized by number of bad pixels if any. Don't care */
        val_data  = posi_data + 2 * nrow;
        if (ebox) dval_data = val_data + nrow;
    }

    if (has_bpm) {
        /* This is the general case, bpm and any pixel type */
        cpl_size row = 0;

        assert(!do_wrap);

        for (j = 0; j < ny; j++) {
            for (i = 0; i < nx; i++) {
                int bad;
                const double value = cpl_image_get(box, i+1, j+1, &bad);

                if (!bad) {
                    posi_data[2 * row    ] = (double)(i + 1);
                    posi_data[2 * row + 1] = (double)(j + 1);
                    val_data[row] = value;
                    if (ebox) {
                        const double evalue = cpl_image_get(ebox, i+1, j+1, &bad);
                        if (!bad) dval_data[row++] = evalue;
                    } else {
                        row++;
                    }
                }
            }
        }
        assert(row < nrow);
        nrow = row;

    } else {
        cpl_size row = 0;

        for (j = 0; j < ny; j++) {
            for (i = 0; i < nx; i++, row++) {

                posi_data[2 * row    ] = (double)(i + 1);
                posi_data[2 * row + 1] = (double)(j + 1);

                val_data[row] = cpl_image_get_(box, i+1, j+1);

                if (ebox) dval_data[row] = cpl_image_get_(ebox, i+1, j+1);
            }
        }
        assert(row == nrow);
    }

    if (nrow < nfit) {

        /*
         * Too few values for the free parameters!
         */

        cpl_array_delete(lowersan);
        cpl_array_delete(uppersan);

        cpl_image_delete(ubox);
        cpl_image_delete(fbox);
        cpl_free(adata);

        return cpl_error_set_message_(CPL_ERROR_DATA_NOT_FOUND, "%d/%d good "
                                      "pixel(s) too few for %d free parameter"
                                      "(s)", (int)nrow, (int)(nx * ny),
                                      (int)nfit);
    }

    values  = cpl_vector_wrap(nrow, val_data);
    dvalues = dval_data ? cpl_vector_wrap(nrow, dval_data) : NULL;
    positions = cpl_matrix_wrap(nrow, 2, posi_data);

    /*
     * Now prepare to fit! Whahahahahah! :-D
     *
     * 1) Vector with first-guess parameters:
     */

    a = cpl_vector_wrap(7, adata);
    cpl_vector_set_(a, 0, background);
    cpl_vector_set_(a, 1, normalisation);
    cpl_vector_set_(a, 2, correlation);
    cpl_vector_set_(a, 3, xcen);
    cpl_vector_set_(a, 4, ycen);
    cpl_vector_set_(a, 5, xsigma);
    cpl_vector_set_(a, 6, ysigma);


#ifdef CPL_LVMQ_DISABLE_SANITY_CHECK
    cpl_array_delete(lowersan);
    cpl_array_delete(uppersan);
    lowersan = uppersan = NULL;
#endif

    /*
     * The "if" below may look absurd. But we need to compute
     * a covariance matrix anyway - even if the user didn't request one -
     * as soon as input errors are available.
     */

    if (dvalues) {
#ifndef NDEBUG
        const double * dval_data2 = cpl_vector_get_data_const_(dvalues);
        assert(dval_data2 == dval_data);
#endif

        /* Defer check for non-positive errors to here so bpms can be ignored */
        for (i = 0; i < nrow; i++) {
            if (dval_data[i] <= 0.0) break;
        }

        if (i < nrow) {
            cpl_array_delete(lowersan);
            cpl_array_delete(uppersan);

            cpl_image_delete(ubox);
            cpl_image_delete(fbox);
            cpl_vector_delete(a);
            cpl_matrix_unwrap(positions);
            cpl_vector_unwrap(values);
            cpl_vector_unwrap(dvalues);

            return cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT,
                                          "Non-positive error %g found in error"
                                          " image, sample %d/%d", dval_data[i],
                                          1+(int)i, (int)nrow);
        }

        if (red_chisq != NULL) *red_chisq = 0.0;
        status = cpl_fit_lvmq_(positions, NULL,
                 values, dvalues,
                 a, ia, lowersan, uppersan, bigauss, bigauss_derivative,
                 CPL_FIT_LVMQ_TOLERANCE,
                 CPL_FIT_LVMQ_COUNT,
                 CPL_FIT_LVMQ_MAXITER,
                 rms, red_chisq, &own_cov);
    }
    else {
        status = cpl_fit_lvmq_(positions, NULL,
                 values, NULL,
                 a, ia, lowersan, uppersan, bigauss, bigauss_derivative,
                 CPL_FIT_LVMQ_TOLERANCE,
                 CPL_FIT_LVMQ_COUNT,
                 CPL_FIT_LVMQ_MAXITER,
                 rms, NULL, NULL);
    }

    cpl_array_delete(lowersan);
    cpl_array_delete(uppersan);

    if (status == CPL_ERROR_SINGULAR_MATRIX) {

        /*
         * Betting these two errors are really to be ignored...
         */

        if (phys_cov == NULL || own_cov != NULL) {
            /* Well, the bet makes sense only in case phys_cov is NULL
               or own_cov is non-NULL, because otherwise the function will
               still fail when the NULL-valued own_cov is used.
               In that case, do not reset, in order to provide a less
               meaningless error.
               The guard prevents a reset with the current unit tests on a
               Darwin 10.7.0.
            */
            status = CPL_ERROR_NONE;
            cpl_errorstate_set(prestate);
        }
    }

    cpl_image_delete(ubox);
    cpl_image_delete(fbox);
    cpl_matrix_unwrap(positions);
    cpl_vector_unwrap(values);
    cpl_vector_unwrap(dvalues);

    if (status == CPL_ERROR_NONE) {

        double  S_x, S_y, R, theta, A, B;
        double  DS_x, DS_y, DR;
        double  S_x_plus, S_y_plus, R_plus;
        double  theta_plus, A_plus, B_plus;

        if (rms)
            *rms = sqrt(*rms);

        /* 
         * The LM algorithm converged. The computation of the covariance 
         * matrix might have failed. All the above errors must be ignored 
         * because of ticket DFS06126. 
         */

        /* 
         * We could at least check whether the result makes sense at all...
         * but this would require the -std=c99 option. So we skip it.

        if (isfinite(cpl_vector_get(a, 0)) &&
            isfinite(cpl_vector_get(a, 1)) &&
            isfinite(cpl_vector_get(a, 2)) &&
            isfinite(cpl_vector_get(a, 3)) &&
            isfinite(cpl_vector_get(a, 4)) &&
            isfinite(cpl_vector_get(a, 5)) &&
            isfinite(cpl_vector_get(a, 6))) {

         End of check section, requiring -std=c99 */

            /*
             * Save best fit parameters: center of gaussian is
             * converted to input image coordinates, evaluations 
             * of sigmas are forced positive (they might be both
             * negative - it would generate the same gaussian).
             */

            cpl_array_set_double(parameters, 0, cpl_vector_get_(a, 0));
            cpl_array_set_double(parameters, 1, cpl_vector_get_(a, 1));
            cpl_array_set_double(parameters, 2, cpl_vector_get_(a, 2));
            cpl_array_set_double(parameters, 3, cpl_vector_get_(a, 3)
                                 + (double)(llx - 1));
            cpl_array_set_double(parameters, 4, cpl_vector_get_(a, 4)
                                 + (double)(lly - 1));
            cpl_array_set_double(parameters, 5, fabs(cpl_vector_get_(a, 5)));
            cpl_array_set_double(parameters, 6, fabs(cpl_vector_get_(a, 6)));


            /*
             * Get from the diagonal of the covariance matrix the variances
             * and fill the error array:
             */

            if (err_params && own_cov) {
                for (i = 0; i < 7; i++) {
                    if (ia[i])
                        cpl_array_set_double(err_params, i,
                                        sqrt(cpl_matrix_get(own_cov, i, i)));
                }
            }


            /*
             * Obtain semiaxes and rotation angle of ellipse at 1-sigma level
             */

            S_x  = cpl_array_get_double(parameters, 5, NULL);
            S_y  = cpl_array_get_double(parameters, 6, NULL);
            R    = cpl_array_get_double(parameters, 2, NULL);

            if (err_params) {
                DS_x = cpl_array_get_double(err_params, 5, NULL);
                DS_y = cpl_array_get_double(err_params, 6, NULL);
                DR   = cpl_array_get_double(err_params, 2, NULL);
            }
            else {
                DS_x = 0.0;
                DS_y = 0.0;
                DR   = 0.0;
            }


            if (fabs(R) <= DR) {
                if (S_x - S_y >= - sqrt(DS_x * DS_x + DS_y * DS_y)) {

                    /*
                     * Circular distribution, or elongated along x axis
                     * (within known uncertainties).
                     */

                    theta = 0.0;
                    A = S_x;
                    B = S_y;
                }
                else {

                    /*
                     * Distribution elongated along y axis (within known 
                     * uncertainties).
                     */

                    theta = CPL_MATH_PI_2;
                    A = S_y;
                    B = S_x;
                }
            }
            else {
                theta = 0.5 * atan2(2 * R, (S_x * S_x - S_y * S_y)/ S_x / S_y);
                A = S_x * S_y
                  * sqrt(2 * (1 - R * R) * cos(2 * theta)
                             / ((S_x * S_x + S_y * S_y) * cos(2 * theta)
                               + S_y * S_y - S_x * S_x));
                B = S_x * S_y
                  * sqrt(2 * (1 - R * R) * cos(2 * theta)
                             / ((S_x * S_x + S_y * S_y) * cos(2 * theta)
                               - S_y * S_y + S_x * S_x));
            }

            if (angle)
                *angle = theta;
            if (major)
                *major = A;
            if (minor)
                *minor = B;

            if (phys_cov) {

                /*
                 * Compute the 3x3 covariance matrix G for the derived 
                 * quantities theta, A and B:
                 *
                 *  G = J C t(J)
                 *
                 * where C is the 7x7 covariance matrix of the best fit 
                 * parameters (p0, p1, p2, p3, p4, p5, p6), J is the 3x7
                 * Jacobian of the transformation (p#) -> (theta, A, B),
                 * and t(J) is its transpose. The Jacobian is computed 
                 * numerically when the analytical approach is impervious.
                 */

                cpl_matrix    *jacobian = cpl_matrix_new(3, 7);


                /*
                 * First row: derivatives of theta.
                 */

                cpl_matrix_set(jacobian, 0, 0, 0.0);   /* d(theta)/d(p0) */
                cpl_matrix_set(jacobian, 0, 1, 0.0);   /* d(theta)/d(p1) */

                R_plus = R + DR;
                S_x_plus = S_x + DS_x;
                S_y_plus = S_y + DS_y;

                if (fabs(R_plus) <= DR) {
                    if (S_x - S_y >= - sqrt(DS_x * DS_x + DS_y * DS_y)) {
                        theta_plus = 0.0;
                    }
                    else {
                        theta_plus = CPL_MATH_PI_2;
                    }
                }
                else {
                    theta_plus = 0.5 * atan2(2 * R_plus,
                                 (S_x * S_x - S_y * S_y)/ S_x / S_y);
                }

                if (DR == 0.0)
                    cpl_matrix_set(jacobian, 0, 2, 0.0);
                else
                    cpl_matrix_set(jacobian, 0, 2, (theta_plus-theta) / DR);
                                                       /* d(theta)/d(p2) */
                cpl_matrix_set(jacobian, 0, 3, 0.0);   /* d(theta)/d(p3) */
                cpl_matrix_set(jacobian, 0, 4, 0.0);   /* d(theta)/d(p4) */

                if (fabs(R) <= DR) {
                    if (S_x_plus - S_y >= - sqrt(DS_x * DS_x + DS_y * DS_y)) {
                        theta_plus = 0.0;
                    }
                    else {
                        theta_plus = CPL_MATH_PI_2;
                    }
                }
                else {
                    theta_plus = 0.5 * atan2(2 * R,
                                 (S_x_plus * S_x_plus - S_y * S_y)
                                / S_x_plus / S_y);
                }

                if (DS_x == 0.0)
                    cpl_matrix_set(jacobian, 0, 5, 0.0);
                else
                    cpl_matrix_set(jacobian, 0, 5, (theta_plus-theta) / DS_x);
                                                       /* d(theta)/d(p5) */

                if (fabs(R) <= DR) {
                    if (S_x - S_y_plus >= - sqrt(DS_x * DS_x + DS_y * DS_y)) {
                        theta_plus = 0.0;
                    }
                    else {
                        theta_plus = CPL_MATH_PI_2;
                    }
                }
                else {
                    theta_plus = 0.5 * atan2(2 * R,
                                 (S_x * S_x - S_y_plus * S_y_plus)
                                / S_x / S_y_plus);
                }

                if (DS_y == 0.0)
                    cpl_matrix_set(jacobian, 0, 6, 0.0);
                else
                    cpl_matrix_set(jacobian, 0, 6, (theta_plus-theta) / DS_y);
                                                       /* d(theta)/d(p6) */

                /*
                 * Second row: derivatives of A.
                 */

                cpl_matrix_set(jacobian, 1, 0, 0.0);   /* d(A)/d(p0) */
                cpl_matrix_set(jacobian, 1, 1, 0.0);   /* d(A)/d(p1) */

                if (fabs(R_plus) <= DR) {
                    if (S_x - S_y >= - sqrt(DS_x * DS_x + DS_y * DS_y)) {
                        A_plus = S_x;
                    }
                    else {
                        A_plus = S_y;
                    }
                }
                else {
                    A_plus = S_x * S_y
                      * sqrt(2 * (1 - R_plus * R_plus) * cos(2 * theta)
                                 / ((S_x * S_x + S_y * S_y) * cos(2 * theta)
                                   + S_y * S_y - S_x * S_x));
                }

                if (DR == 0.0)
                    cpl_matrix_set(jacobian, 1, 2, 0.0);
                else
                    cpl_matrix_set(jacobian, 1, 2, (A_plus-A) / DR);
                                                       /* d(A)/d(p2) */
                cpl_matrix_set(jacobian, 1, 3, 0.0);   /* d(A)/d(p3) */
                cpl_matrix_set(jacobian, 1, 4, 0.0);   /* d(A)/d(p4) */

                if (fabs(R) <= DR) {
                    if (S_x_plus - S_y >= - sqrt(DS_x * DS_x + DS_y * DS_y)) {
                        A_plus = S_x_plus;
                    }
                    else {
                        A_plus = S_y;
                    }
                }
                else {
                    A = S_x_plus * S_y
                      * sqrt(2 * (1 - R * R) * cos(2 * theta)
                      / ((S_x_plus * S_x_plus + S_y * S_y) * cos(2 * theta)
                      + S_y * S_y - S_x_plus * S_x_plus));
                }

                if (DS_x == 0.0)
                    cpl_matrix_set(jacobian, 1, 5, 0.0);
                else
                    cpl_matrix_set(jacobian, 1, 5, (A_plus-A) / DS_x);
                                                       /* d(A)/d(p5) */

                if (fabs(R) <= DR) {
                    if (S_x - S_y_plus >= - sqrt(DS_x * DS_x + DS_y * DS_y)) {
                        A_plus = S_x;
                    }
                    else {
                        A_plus = S_y_plus;
                    }
                }
                else {
                    A = S_x * S_y_plus
                      * sqrt(2 * (1 - R * R) * cos(2 * theta)
                      / ((S_x * S_x + S_y_plus * S_y_plus) * cos(2 * theta)
                      + S_y_plus * S_y_plus - S_x * S_x));
                }

                if (DS_y == 0.0)
                    cpl_matrix_set(jacobian, 1, 6, 0.0);
                else
                    cpl_matrix_set(jacobian, 1, 6, (A_plus-A) / DS_y);
                                                       /* d(A)/d(p6) */

                /*
                 * Third row: derivatives of B.
                 */

                cpl_matrix_set(jacobian, 2, 0, 0.0);   /* d(B)/d(p0) */
                cpl_matrix_set(jacobian, 2, 1, 0.0);   /* d(B)/d(p1) */

                if (fabs(R_plus) <= DR) {
                    if (S_x - S_y >= - sqrt(DS_x * DS_x + DS_y * DS_y)) {
                        B_plus = S_y;
                    }
                    else {
                        B_plus = S_x;
                    }
                }
                else {
                    B_plus = S_x * S_y
                      * sqrt(2 * (1 - R_plus * R_plus) * cos(2 * theta)
                                 / ((S_x * S_x + S_y * S_y) * cos(2 * theta)
                                   - S_y * S_y + S_x * S_x));
                }

                if (DR == 0.0)
                    cpl_matrix_set(jacobian, 2, 2, 0.0);
                else
                    cpl_matrix_set(jacobian, 2, 2, (B_plus-B) / DR);
                                                       /* d(B)/d(p2) */
                cpl_matrix_set(jacobian, 2, 3, 0.0);   /* d(B)/d(p3) */
                cpl_matrix_set(jacobian, 2, 4, 0.0);   /* d(B)/d(p4) */

                if (fabs(R) <= DR) {
                    if (S_x_plus - S_y >= - sqrt(DS_x * DS_x + DS_y * DS_y)) {
                        B_plus = S_y;
                    }
                    else {
                        B_plus = S_x_plus;
                    }
                }
                else {
                    B = S_x_plus * S_y
                      * sqrt(2 * (1 - R * R) * cos(2 * theta)
                      / ((S_x_plus * S_x_plus + S_y * S_y) * cos(2 * theta)
                      - S_y * S_y + S_x_plus * S_x_plus));
                }

                if (DS_x == 0.0)
                    cpl_matrix_set(jacobian, 2, 5, 0.0);
                else
                    cpl_matrix_set(jacobian, 2, 5, (B_plus-B) / DS_x);
                                                       /* d(B)/d(p5) */

                if (fabs(R) <= DR) {
                    if (S_x - S_y_plus >= - sqrt(DS_x * DS_x + DS_y * DS_y)) {
                        B_plus = S_y_plus;
                    }
                    else {
                        B_plus = S_x;
                    }
                }
                else {
                    B = S_x * S_y_plus
                      * sqrt(2 * (1 - R * R) * cos(2 * theta)
                      / ((S_x * S_x + S_y_plus * S_y_plus) * cos(2 * theta)
                      - S_y_plus * S_y_plus + S_x * S_x));
                }

                if (DS_y == 0.0)
                    cpl_matrix_set(jacobian, 2, 6, 0.0);
                else
                    cpl_matrix_set(jacobian, 2, 6, (B_plus-B) / DS_y);
                                                       /* d(B)/d(p6) */

                /*
                 * The jacobian is complete, now transpose it and
                 * derive the covariance matrix for theta, A, and B:
                 * C_{ph} = J * C * J'
                 */

                *phys_cov = cpl_matrix_new(3, 3);
                cpl_matrix_product_bilinear(*phys_cov, own_cov, jacobian);
                cpl_matrix_delete(jacobian);
            }
        /* This closing-block bracket is the one related to the isfinite()
         * check, which would require -std=c99. 
        }
        */
    }

    cpl_vector_delete(a);

    /*
     * Note that, until CPL_ERROR_CONTINUE is ignored, the following
     * condition will never be true.
     */

    if (status) {

        /* 
         * The LM algorithm did not converge, or it converged to
         * a non-sensical result. 
         * In this case the covariance matrix will not make sense
         * so delete it 
         */

        cpl_matrix_delete(own_cov);
        return cpl_error_set_where_();
    }

    if (covariance) {
        *covariance = own_cov;
    } else {
        cpl_matrix_delete(own_cov);
    }

    return CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
  @brief    Evaluate the Gaussian in a 2D-point
  @param    self  The seven Gaussian parameters
  @param    x     The X-coordinate to evaluate
  @param    y     The Y-coordinate to evaluate
  @return   The gaussian value or zero on error
  @see cpl_fit_image_gaussian()
  @note The function should not be able to fail if the parameters come from
        a succesful call to cpl_fit_image_gaussian()

   Possible #_cpl_error_code_ set in this function:
   - CPL_ERROR_NULL_INPUT if a pointer is NULL.
   - CPL_ERROR_TYPE_MISMATCH if the array is not of type double
   - CPL_ERROR_ILLEGAL_INPUT if the array has a length different from 7
   - CPL_ERROR_ILLEGAL_OUTPUT if the (absolute value of the) radius exceeds 1
   - CPL_ERROR_DIVISION_BY_ZERO if a sigma is 0, or the radius is 1

 */
/*----------------------------------------------------------------------------*/
double cpl_gaussian_eval_2d(const cpl_array * self, double x, double y)
{
    cpl_errorstate prestate = cpl_errorstate_get();
    const double B    = cpl_array_get_double(self, 0, NULL);
    const double A    = cpl_array_get_double(self, 1, NULL);
    const double R    = cpl_array_get_double(self, 2, NULL);
    const double M_x  = cpl_array_get_double(self, 3, NULL);
    const double M_y  = cpl_array_get_double(self, 4, NULL);
    const double S_x  = cpl_array_get_double(self, 5, NULL);
    const double S_y  = cpl_array_get_double(self, 6, NULL);

    double value = 0.0;

    if (!cpl_errorstate_is_equal(prestate)) {
        (void)cpl_error_set_where_();
    } else if (cpl_array_get_size(self) != 7) {
        (void)cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
    } else if (fabs(R) < 1.0 && S_x != 0.0 && S_y != 0.0) {
        const double x_n  = (x - M_x) / S_x;
        const double y_n  = (y - M_y) / S_y;

        value = B + A / (CPL_MATH_2PI * S_x * S_y * sqrt(1 - R * R)) *
            exp(-0.5 / (1 - R * R) * ( x_n * x_n + y_n * y_n
                                       - 2.0 * R * x_n * y_n));
    } else if (fabs(R) > 1.0) {
        (void)cpl_error_set_message_(CPL_ERROR_ILLEGAL_OUTPUT,
                                     "fabs(R=%g) > 1", R);
    } else {
        (void)cpl_error_set_message_(CPL_ERROR_DIVISION_BY_ZERO,
                                     "R=%g. Sigma=(%g, %g)", R, S_x, S_y);
    }

    return value;
}

/**@}*/

/*----------------------------------------------------------------------------*/
/**
  @brief   Determine the number of pixels that can be processed within the L2
  @param   np      The number of samplint points
  @param   nc      The number of coefficients to determine
  @param   ip      The pixel-type of the input image list
  @param   op      The pixel-type of the output image list
  @param   ep      The pixel-type of the fitting error
  @param   do_err  CPL_TRUE iff fiterror is to be computed
  @return  The number of pixels, or 1 if the cache is too small

  The speed of cpl_fit_imagelist_polynomial() is only reduced significantly
  if the estimated size of the L2-cache is off by about an order of magnitude or
  more, especially if the actual cache size is much smaller than assumed here.

 */
/*----------------------------------------------------------------------------*/
static cpl_size cpl_fit_imagelist_polynomial_find_block_size(cpl_size np,
                                                             cpl_size nc,
                                                             cpl_boolean do_err,
                                                             cpl_type ip,
                                                             cpl_type op,
                                                             cpl_type ep)
{

    cpl_size blocksize;

    /* The storage [bytes] needed for mv and mh */
    cpl_size c0 = (nc * nc + nc * np) * (cpl_size)sizeof(double);
    /* Storage per pixel needed for mx, mb and the input and output images */
    cpl_size c1 =  np * (cpl_size)cpl_type_get_sizeof(ip)
        + nc * (cpl_size)cpl_type_get_sizeof(op)
        + (nc + np) * (cpl_size)sizeof(double);

    if (do_err) {
        /* The storage [bytes] needed for xpos and xpow */
        c0 += (2 * np) * (cpl_size)sizeof(double);

        /* Storage per pixel needed for fitting error */
        c1 += (cpl_size)cpl_type_get_sizeof(ep);
    }

    /* In principle the number of pixels that can be processed within the
       L2 cache would be (L2_CACHE_BYTES - c0) / c1.
       Apparently, the effective size
       of the cache is observed to be about four times smaller */

    blocksize = ((L2_CACHE_BYTES)/4 - 10 * c0 - 1024) / c1;

    return blocksize > 0 ? blocksize : 1;

}


/*----------------------------------------------------------------------------*/
/**
  @brief  Least-squares fit a polynomial to each pixel in a list of images - bpm
  @param  self     The polynomiums as images, first has mindeg coefficients
  @param  redo     The map of pixels to redo, has same dimension as self
  @param  x_pos    The vector of positions to fit
  @param  values   The list of images with values to fit
  @param  llx      Lower left x coordinate
  @param  lly      Lower left y coordinate
  @param  mindeg   The smallest degree with a non-zero coefficient
  @param  fiterror When non-NULL, the error of the fit
  @note   values and mv must have the same number of elements.
  @return CPL_ERROR_NONE or the relevant CPL error code on error
  @see cpl_fit_imagelist_polynomial_window()

 */
/*----------------------------------------------------------------------------*/
static
cpl_error_code cpl_fit_imagelist_polynomial_bpm(cpl_imagelist       * self,
                                                const cpl_mask      * redo,
                                                const cpl_vector    * x_pos,
                                                const cpl_imagelist * values,
                                                cpl_size              llx,
                                                cpl_size              lly,
                                                cpl_size              mindeg,
                                                cpl_image           * fiterror)

{

    const cpl_image * first = cpl_imagelist_get_const(self, 0);
    const cpl_size    nx    = cpl_image_get_size_x(first);
    const cpl_size    ny    = cpl_image_get_size_y(first);
    const cpl_size    np    = cpl_imagelist_get_size(values);
    cpl_error_code    error = CPL_ERROR_NONE;

    const cpl_binary * bpm   = cpl_mask_get_data_const(redo);
    const cpl_binary * found = bpm - 1; /* Prepare iteration */
    cpl_size           todo  = nx * ny; /* Number of pixels to search */
    cpl_polynomial   * fit1d = cpl_polynomial_new(1);
    double           * xgood = cpl_malloc((size_t)np * sizeof(*xgood));
    double           * ygood = cpl_malloc((size_t)np * sizeof(*ygood));

    while (!error && (found = memchr(found + 1, CPL_BINARY_1,
                                     (size_t)todo * sizeof(*bpm)))) {
        /* Found an interpolation to redo */
        const cpl_size ij = found - bpm;
        const cpl_size j = ij / nx;
        const cpl_size i = ij - j * nx;


        error = cpl_fit_imagelist_polynomial_one(self, fit1d, xgood, ygood,
                                                 i, j, x_pos, values, llx,
                                                 lly, mindeg, fiterror);

        /* Update number of pixels to search. Can never become negative */
        todo = nx * ny - ij - 1;
        /* This is invariant (true before and for each loop iteration) */
        /* assert( found + todo + 1 == bpm + nx * ny); */
    }

    cpl_polynomial_delete(fit1d);
    cpl_free(xgood);
    cpl_free(ygood);

    /* Propagate error, if any */
    return error ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
  @brief  Least-squares fit a polynomial to one pixel in a list of images - bpm
  @param  self     The polynomiums as images, first has mindeg coefficients
  @param  fit1d    Temporary 1D-polynomial used internally
  @param  xgood    Temporary array for the usable sampling points
  @param  ygood    Temporary array for the usable values
  @param  i        The X-position (0 for first) of the pixel to (re)interpolate
  @param  j        The Y-position (0 for first) of the pixel to (re)interpolate
  @param  mv       The Vandermonde matrix of the sample positions
  @param  values   The list of images with values to fit
  @param  llx      Lower left x coordinate
  @param  lly      Lower left y coordinate
  @param  mindeg   The smallest degree with a non-zero coefficient
  @param  fiterror When non-NULL, the error of the fit
  @note   values and mv must have the same number of elements.
  @return CPL_ERROR_NONE or the relevant CPL error code on error
  @see cpl_fit_imagelist_polynomial_window()

 */
/*----------------------------------------------------------------------------*/
static
cpl_error_code cpl_fit_imagelist_polynomial_one(cpl_imagelist       * self,
                                                cpl_polynomial      * fit1d,
                                                double              * xgood,
                                                double              * ygood,
                                                cpl_size              i,
                                                cpl_size              j,
                                                const cpl_vector    * x_pos,
                                                const cpl_imagelist * values,
                                                cpl_size              llx,
                                                cpl_size              lly,
                                                cpl_size              mindeg,
                                                cpl_image           * fiterror)

{

    const cpl_image * first = cpl_imagelist_get_const(self, 0);
    const cpl_size    nx    = cpl_image_get_size_x(first);
    const cpl_size    ny    = cpl_image_get_size_y(first);
    const cpl_size    np    = cpl_imagelist_get_size(values);
    const cpl_size    nc    = cpl_imagelist_get_size(self);
    const int         imindeg = (int)mindeg;
    cpl_size          k;
    cpl_error_code    error = CPL_ERROR_NONE;
    cpl_size          igood = 0;

    cpl_ensure_code(self   != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(x_pos  != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(values != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(llx    >  0,    CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(lly    >  0,    CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(mindeg >= 0,    CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code((cpl_size)imindeg == mindeg, CPL_ERROR_ILLEGAL_INPUT);

    if (fiterror != NULL) {
        cpl_ensure_code(cpl_image_get_size_x(fiterror) == nx &&
                        cpl_image_get_size_y(fiterror) == ny,
                        CPL_ERROR_INCOMPATIBLE_INPUT);
    }

    for (k = 0; k < np; k++) {
        const cpl_image * img = cpl_imagelist_get_const(values, k);
        int is_rejected = 0;
        const double value = cpl_image_get(img, llx + i, lly + j, &is_rejected);

        if (!is_rejected) {
            xgood[igood] = cpl_vector_get(x_pos, k);
            ygood[igood] = value;
            igood++;
        }
    }

    if (igood == 0) {
        /* No samples available for this pixel */
        /* Bad pixels are set to zero. */
        for (k = 0; k < nc; k++) {
            cpl_image * img = cpl_imagelist_get(self, k);
            cpl_image_set_   (img, 1 + i, 1 + j, 0.0);
            cpl_image_reject_(img, 1 + i, 1 + j);
        }
        if (fiterror != NULL) {
            cpl_image_set_(fiterror, 1 + i, 1 + j, 0.0);
        }

    } else {
        cpl_vector * vxgood = cpl_vector_wrap(igood, xgood);
        cpl_vector * vygood = cpl_vector_wrap(igood, ygood);
        cpl_vector * vxcopy = cpl_vector_duplicate(vxgood);
        /* If there is a shortage of usable samples the number of coefficients
           to fit is reduced to the number of distinct sample positions */
        cpl_size ndistinct;
        const cpl_error_code err2 = cpl_vector_count_distinct(vxcopy,
                                                              &ndistinct);
        const cpl_size ncfit  = CX_MIN(nc, ndistinct);
        const cpl_size degree = ncfit + imindeg - 1;
        /* Do error estimate only if the system is overdetermined */
        const cpl_boolean do_err = fiterror != NULL && (igood > nc);
        double mse = 0.0;

        /* assert(igood < np); */

        error = err2 ? err2 : cpl_polynomial_fit_1d(fit1d, vxgood, vygood,
                                                    imindeg, degree, CPL_FALSE,
                                                    do_err ? &mse : NULL);

        cpl_vector_delete(vxcopy);
        (void)cpl_vector_unwrap(vxgood);
        (void)cpl_vector_unwrap(vygood);

        for (k = 0; k < ncfit; k++) {
            cpl_image * img = cpl_imagelist_get(self, k);
            const cpl_size kk = k + mindeg;
            const double value = cpl_polynomial_get_coeff(fit1d, &kk);
            cpl_image_set_(img, 1 + i, 1 + j,
                           cpl_image_get_type(img) == CPL_TYPE_INT
                           ? floor(0.5 + value) : value);
            cpl_image_accept_(img, 1 + i, 1 + j);
        }
        /* Higher order terms that could not be fitted are rejected,
           and set to zero. The zero-value(s) means that the lower-degree
           polynomial is still usable */
        for (; k < nc; k++) {
            cpl_image * img = cpl_imagelist_get(self, k);
            cpl_image_set_   (img, 1 + i, 1 + j, 0.0);
            cpl_image_reject_(img, 1 + i, 1 + j);
        }

        if (fiterror != NULL) {
            /* In the non-bpm case, the error is set to zero for a
               non-overdetermined system. */
            cpl_image_set_(fiterror, 1 + i, 1 + j, 
                           cpl_image_get_type(fiterror) == CPL_TYPE_INT
                           ? floor(0.5 + mse) : mse);
            cpl_image_accept_(fiterror, 1 + i, 1 + j);

        }
    }

    return error ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/* Define the C-type dependent functions */

/* These two macros are needed for support of the different pixel types */

#define CONCAT(a,b) a ## _ ## b
#define CONCAT2X(a,b) CONCAT(a,b)
#define CONCAT3X(a,b,c) CONCAT2X(CONCAT2X(a,b),c)

#define CPL_TYPE double
#include "cpl_fit_body.h"
#undef CPL_TYPE

#define CPL_TYPE float
#include "cpl_fit_body.h"
#undef CPL_TYPE

#define CPL_TYPE_INT_ROUND(A) ((int)floor(0.5 + (A)))
#define CPL_TYPE int
#include "cpl_fit_body.h"
#undef CPL_TYPE
#undef CPL_TYPE_INT_ROUND
