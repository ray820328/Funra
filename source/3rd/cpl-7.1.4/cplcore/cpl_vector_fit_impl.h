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

#ifndef CPL_VECTOR_FIT_IMPL_H
#define CPL_VECTOR_FIT_IMPL_H

/*
 * FIXME: The code in this file is a copy of the cpl_fit module and has to
 *        stay here until cpl_vector_fit_gaussian() is moved to the cpl_fit
 *        module, which can be done only before the release of the next major
 *        version, in order not to break the library hierarchy!
 *
 *        When the code in this file is finally moved to the cpl_fit module,
 *        this file has to be removed!
 */

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include "cpl_fit.h"

#include "cpl_vector_impl.h"
#include "cpl_matrix_impl.h"
#include "cpl_memory.h"
#include "cpl_error_impl.h"
#include "cpl_errorstate.h"
#include "cpl_tools.h"
#include "cpl_math_const.h"

#include <string.h>
#include <math.h>
#include <assert.h>

/*-----------------------------------------------------------------------------
                                New types
 -----------------------------------------------------------------------------*/

/* Various temporary, typically small data objects (for a full 2D gaussian fit
   616 bytes, i.e. less than the default value of CPL_IFALLOC_SZ = 1024) */
typedef struct cpl_fit_work {
    cpl_ifalloc * pmybuf;     /* Typically stack (else heap) allocated buffer */
    cpl_size      Mfit;       /* The number of non-constant fit parameters */
    cpl_size      Mallfree;   /* First free parameter after last frozen.
                                 - often only the first params are frozen, so
                                 this helps speed up looping through the top */
    double      * a_da;       /* (Output) Candidate position in parameter space.
                                 This buffer is not passed to get_candidate()
                                 via this struct, it is just allocated as part
                                 of this buffer. */
    double      * part;       /* The partial derivatives */
    cpl_matrix  * alpha;      /* Alpha in L-M algorithm. In get_candidate() it
                                 is overwritten by its Cholesky decomposition */
    double      * dalpha;     /* The elements of alpha */
    cpl_matrix  * beta;       /* Beta in L-M algorithm */
    double      * dbeta;      /* The elements of beta */
    double      * dalphabeta; /* Pointer to Alpha + Beta  */
    size_t        salphabeta; /* Number of elements in Alpha + Beta  */
    cpl_matrix  * alphabeta;  /* Alpha and beta as a single matrix */
    int         * ia;         /* which fit parameters participate
                                 in the fit (non-zero) */
} cpl_fit_work;

/*-----------------------------------------------------------------------------
                        Private function prototypes
 -----------------------------------------------------------------------------*/

inline static void cpl_fit_work_set(cpl_fit_work *, cpl_ifalloc *,
                                    size_t, size_t, size_t);
inline static void cpl_fit_work_delete(cpl_fit_work *);

inline static int cpl_fit_is_sane(const double [], cpl_size,
                                  const cpl_array *,
                                  const cpl_array *)
    CPL_ATTR_NONNULL CPL_ATTR_PURE;

inline static int
get_candidate(const double *, const int [],
          cpl_size, cpl_size, cpl_size,
          double,
          int    (*)(const double [], const double [], double *),
          int (*)(const double [], const double [], double []),
          const double *,
          const double *,
          const double *,
          double *,
          double *,
          double *,
          cpl_fit_work *)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(1,2,7,8,9,10,12,13,14,15)))
#endif
    ;

inline static double
get_chisq(cpl_size, cpl_size,
      int (*)(const double [], const double [], double *),
      const double *,
      const double *,
      const double *,
      const double *)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(3,4,5,6)))
#endif
    ;

/*-----------------------------------------------------------------------------
                              Function codes
 -----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*
   @brief   Get new position in parameter space (L-M algorithm)
   @param   a       Current fit parameters.
   @param   ia      Non-NULL array defining with non-zero values which
                    parameters participate in the fit.
   @param   M       Number of fit parameters
   @param   N       Number of positions
   @param   D       Dimension of x-positions
   @param   lambda  Lambda in L-M algorithm.
   @param   f       Function that evaluates the fit function.
   @param   dfda    Function that evaluates the partial derivaties
                    of the fit function w.r.t. fit parameters.
   @param   x       The input positions (pointer to MxD matrix buffer).
   @param   y       The N values to fit.
   @param   sigma   A vector of size N containing the uncertainties of the
                    y-values. If NULL, a constant uncertainty equal to 1 is
            assumed.
   @param   a_da    (output) Candidate position in parameter space.
   @param   pasq2n  (output) Square of the 2-norm of a_da, ||a||^2
   @param   pbsq2n  (output) Square of the 2-norm of step, ||beta||^2
   @param   work    Work space: Alpha + Beta + partials in L-M algorithm

   @return  0 iff okay.

   This function computes a potentially better set of parameters @em a + @em da,
   where @em da solves the equation @em alpha(@em lambda) * @em da = @em beta .

   Possible #_cpl_error_code_ set in this function:
   - CPL_ERROR_ILLEGAL_INPUT if the fit function or its derivative could
   not be evaluated.
   - CPL_ERROR_SINGULAR_MATRIX if @em alpha is singular.

*/
/*----------------------------------------------------------------------------*/
inline static int
get_candidate(const double *a, const int ia[],
              cpl_size M, cpl_size N, cpl_size D,
              double lambda,
              int    (*f)(const double x[], const double a[], double *result),
              int (*dfda)(const double x[], const double a[], double result[]),
              const double *x,
              const double *y,
              const double *sigma,
              double       *a_da,
              double       *pasq2n,
              double       *pbsq2n,
              cpl_fit_work *pwork)
{
    double           *partials   = pwork->part;
    cpl_matrix       *alpha      = pwork->alpha;
    cpl_matrix       *beta       = pwork->beta;
    double           *alpha_data = pwork->dalpha;
    double           *beta_data  = pwork->dbeta;
    /* Number of non-constant fit parameters */
    const cpl_size    Mfit       = pwork->Mfit;
    const cpl_size    Mallfree   = pwork->Mallfree;
    cpl_size i, imfit;

    /* For efficiency, don't check input in this static function */

    /* Build alpha, beta:
     *
     *  alpha[i,j] = sum_{k=1,N} (sigma_k)^-2 * df/da_i * df/da_j  *
     *                           (1 + delta_ij lambda) ,
     *
     *   beta[i]   = sum_{k=1,N} (sigma_k)^-2 * ( y_k - f(x_k) ) * df/da_i
     *
     * where (i,j) loop over the non-constant parameters (0 to Mfit-1),
     * delta is Kronecker's delta, and all df/da are evaluated in x_k
     */

    /* Initialize Alpha and Beta to zero */
    (void)cpl_matrix_fill(pwork->alphabeta, 0.0);

    for (cpl_size k = 0; k < N; k++) {
        double fx_k;                     /* f(x_k), y_k - f(x_k) */
        const double sm2 = sigma == NULL /* (sigma_k)^-2 */
            ? 1.0 : 1.0 / (sigma[k] * sigma[k]);
        int retval;

        /* Evaluate f(x_k) */
        retval = f(x + k * D, a, &fx_k);
        if (retval) {
            (void)cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT, "f(%g) = %d,"
                                         " k=%d/%d", x[k * D], retval, 1+(int)k,
                                         (int)N);
            return -1;
        }

        /* Evaluate (all) df/da (x_k) */
        retval = dfda(x + k * D, a, partials);
        if (retval) {
            (void)cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT, "f'(%g) = %d,"
                                         " k=%d/%d", x[k * D], retval, 1+(int)k,
                                         (int)N);
            return -1;
        }

        /* The difference y_k - f(x_k) is scaled and added to beta */
        fx_k -= y[k];

        /* Alpha is symmetric, so compute only the lower-left part
           (allowing unit stride access) */
        imfit = 0;
        for (i = 0; i < Mallfree; i++) {
            if (ia[i] != 0) {
                cpl_size j, jmfit;
                /* Beta */
                beta_data[imfit] -= sm2 * fx_k * partials[i];

                for (j = 0, jmfit = 0; j < CPL_MIN(i, Mallfree); j++) {
                    if (ia[j] != 0) {
                        alpha_data[Mfit * imfit + jmfit++]
                            += sm2 * partials[i] * partials[j];
                    }
                }
                for (; j < i; j++) {
                    alpha_data[Mfit * imfit + jmfit++]
                        += sm2 * partials[i] * partials[j];
                }

                /* Alpha, diagonal terms */
                alpha_data[Mfit * imfit + imfit] +=
                    sm2 * partials[i] * partials[i] * (1.0 + lambda);

                imfit++;
            }
        }
        for (; i < M; i++) {
            cpl_size j, jmfit;
            /* Beta */
            beta_data[imfit] -= sm2 * fx_k * partials[i];

            for (j = 0, jmfit = 0; j < CPL_MIN(i, Mallfree); j++) {
                if (ia[j] != 0) {
                    alpha_data[Mfit * imfit + jmfit++]
                        += sm2 * partials[i] * partials[j];
                }
            }
            for (; j < i; j++) {
                alpha_data[Mfit * imfit + jmfit++]
                    += sm2 * partials[i] * partials[j];
            }

            /* Alpha, diagonal terms */
            alpha_data[Mfit * imfit + imfit] +=
                sm2 * partials[i] * partials[i] * (1.0 + lambda);

            imfit++;
        }
        assert( imfit == Mfit );
    }

    /* Because alpha is the product of J^T * J (where J is assumed to have full
       column rank, otherwise nothing works) times 1 + lambda >= 0, alpha is
       symmetric positive definite.  So solve via Cholesky decomposition. */
    /* This requires the upper-right part of alpha to be filled. */

    imfit = 0;
    for (i = 0; i < Mallfree; i++) {
        if (ia[i] != 0) {
            cpl_size j, jmfit = imfit+1;

            for (j = i + 1; j < Mallfree; j++) {
                if (ia[j] != 0) {
                    alpha_data[Mfit * imfit + jmfit] =
                        alpha_data[Mfit * jmfit + imfit];

                    jmfit++;
                }
            }
            for (; j < M; j++) {
                alpha_data[Mfit * imfit + jmfit] =
                    alpha_data[Mfit * jmfit + imfit];

                jmfit++;
            }
            assert( jmfit == Mfit );

            imfit++;
        }
    }
    for (; i < M; i++) {
        cpl_size j, jmfit = imfit+1;

        for (j = i + 1; j < M; j++) {
            alpha_data[Mfit * imfit + jmfit] =
                alpha_data[Mfit * jmfit + imfit];

            jmfit++;
        }
        assert( jmfit == Mfit );

        imfit++;
    }
    assert( imfit == Mfit );

    /* Solution of   alpha * da = beta */
    /* This will overwrite alpha with its Cholesky factorization */

    if (cpl_matrix_solve_spd(alpha, beta)) {
        (void)cpl_error_set_where_();
        return -1;
    }

    /* Create a+da vector by adding a and da */

    *pasq2n = 0.0;
    *pbsq2n = 0.0;
    imfit = 0;
    for (i = 0; i < Mallfree; i++) {
        if (ia[i] != 0) {
            *pbsq2n += beta_data[imfit] * beta_data[imfit];
            a_da[i] = a[i] + beta_data[imfit++];
        } else {
            a_da[i] = a[i];
        }
        *pasq2n += a_da[i] * a_da[i];
    }
    for (; i < M; i++) {
        *pbsq2n += beta_data[imfit] * beta_data[imfit];
        a_da[i] = a[i] + beta_data[imfit++];
        *pasq2n += a_da[i] * a_da[i];
    }
    assert( imfit == Mfit );

#if defined CPL_VECTOR_LVMQ_DIAG && CPL_VECTOR_LVMQ_DIAG > 2
    switch (Mfit) {
    case 1:
        cpl_msg_info(cpl_func, "LVMQ-BETA(%d<%d:lambda=%g): %g",
                     (int)Mfit, (int)M, lambda, beta_data[0]);
        break;
    case 2:
        cpl_msg_info(cpl_func, "LVMQ-BETA(%d<%d:lambda=%g): %g %g",
                     (int)Mfit, (int)M, lambda, beta_data[0], beta_data[1]);
        break;
    case 3:
        cpl_msg_info(cpl_func, "LVMQ-BETA(%d<%d:lambda=%g): %g %g %g",
                     (int)Mfit, (int)M, lambda, beta_data[0], beta_data[1], beta_data[2]);
        break;
    case 4:
        cpl_msg_info(cpl_func, "LVMQ-BETA(%d<%d:lambda=%g): %g %g %g %g",
                     (int)Mfit, (int)M, lambda, beta_data[0], beta_data[1], beta_data[2],
                     beta_data[3]);
        break;
    case 5:
        cpl_msg_info(cpl_func, "LVMQ-BETA(%d<%d:lambda=%g): %g %g %g %g %g",
                     (int)Mfit, (int)M, lambda, beta_data[0], beta_data[1], beta_data[2],
                     beta_data[3], beta_data[4]);
        break;
    case 6:
        cpl_msg_info(cpl_func, "LVMQ-BETA(%d<%d:lambda=%g): %g %g %g %g %g %g",
                     (int)Mfit, (int)M, lambda, beta_data[0], beta_data[1], beta_data[2],
                     beta_data[3], beta_data[4], beta_data[5]);
        break;
    default:
        cpl_msg_info(cpl_func, "LVMQ-BETA(%d<%d:lambda=%g): %g %g %g %g %g %g %g",
                     (int)Mfit, (int)M, lambda, beta_data[0], beta_data[1], beta_data[2],
                     beta_data[3], beta_data[4], beta_data[5], beta_data[6]);
        break;
    }
#endif

    return 0;
}

/*----------------------------------------------------------------------------*/
/*
   @brief   Compute chi square
   @param   N       Number of positions
   @param   D       Dimension of x-positions
   @param   f       Function that evaluates the fit function.
   @param   a       The fit parameters.
   @param   x       Where to evaluate the fit function (N x D matrix).
   @param   y       The N values to fit.
   @param   sigma   A vector of size N containing the uncertainties of the
                    y-values. If NULL, a constant uncertainty equal to 1 is
            assumed.

   @return  chi square, or a negative number on error.

   This function calculates chi square defined as
   sum_i (y_i - f(x_i, a))^2/sigma_i^2

   Possible #_cpl_error_code_ set in this function:
   - CPL_ERROR_ILLEGAL_INPUT if the fit function could not be evaluated
*/
/*----------------------------------------------------------------------------*/

inline static double
get_chisq(cpl_size N, cpl_size D,
      int (*f)(const double x[], const double a[], double *result),
      const double *a,
      const double *x,
      const double *y,
      const double *sigma)
{
    double chi_sq = 0.0;     /* Result */

    /* For efficiency, don't check input in this static function */

    for (cpl_size i = 0; i < N; i++) {
        double fx_i;
        /* Evaluate */
        const int retval = f(x + i * D, a, &fx_i);

        if (retval != 0) {
            (void)cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT, "f(%g) = %d, "
                                         "i=%d/%d X %d", x[i * D], retval,
                                         1+(int)i, (int)N, (int)D);
            return -1.0;
        } else {
            /* Residual in units of uncertainty */
            const double residual = sigma != NULL
                ? (fx_i - y[i]) / sigma[i]
                : (fx_i - y[i]);

            /* Accumulate */
            chi_sq += residual * residual;
        }
    }

    return chi_sq;
}


/*----------------------------------------------------------------------------*/
/*
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
   @param   lowersan For each free parameter, a lower sanity bound may be
                     provided. If a candidate parameter exceeds the sanity bound
                     the candidate is discarded. May be NULL for no sanity bound
   @param   uppersan The matching upper sanity bound, or NULL. The lower and
                     upper must either both be present or both be absent
   @param   f        Function that evaluates the fit function
                     at the position specified by the first argument (an array
             of size D) using the fit parameters specified by the second
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
                     If this number of iterations is reached without
                     convergence, the algorithm diverges, by definition.
                     Recommended default: CPL_FIT_LVMQ_MAXITER
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
inline static cpl_error_code
cpl_fit_lvmq_(const cpl_matrix *x, const cpl_matrix *sigma_x,
         const cpl_vector *y, const cpl_vector *sigma_y,
         cpl_vector *a, const int ia[],
         const cpl_array *lowersan, const cpl_array *uppersan,
         int    (*f)(const double x[], const double a[], double *result),
         int (*dfda)(const double x[], const double a[], double result[]),
         double relative_tolerance,
         int tolerance_count,
         int max_iterations,
         double *mse,
         double *red_chisq,
         cpl_matrix **covariance)
{
    const double *x_data     = NULL; /* Pointer to input data                 */
    const double *y_data     = NULL; /* Pointer to input data                 */
    const double *sigma_data = NULL; /* Pointer to input data                 */
    cpl_size N    = 0;               /* Number of data points                 */
    cpl_size D    = 0;               /* Dimension of x-points                 */
    cpl_size M = 0;                  /* Number of fit parameters              */
    cpl_size Mfit = 0;               /* Number of non-constant fit
                                        parameters                            */

    double lambda    = 0.0;          /* Lambda in L-M algorithm               */
    double MAXLAMBDA = 10e40;        /* Parameter to control the graceful exit
                    if steepest descent unexpectedly fails */
    double chi_sq    = 0.0;          /* Current  chi^2                        */
    int count        = 0;            /* Number of successive small improvements
                    in chi^2 */
    int iterations   = 0;

    cpl_matrix *alpha  = NULL;       /* The MxM ~curvature matrix used in L-M */
    cpl_ifalloc  mybuf;              /* Buffer for work space */
    cpl_fit_work work;               /* Mx1 matrix = -.5 grad(chi^2)          */
    double *a_data      = NULL;      /* Parameters, a                         */
    double *a_da        = NULL;      /* Candidate position a+da               */
    double *a_keep      = NULL;      /* Points to the a_da to keep            */
    double *part        = NULL;      /* The partial derivatives df/da         */
    cpl_boolean fit_all = CPL_FALSE; /* True iff all parameters are fitted    */
    const int *ia_local = ia;        /* non-NULL version of ia                */
    const cpl_boolean check_sanity = lowersan != NULL ? CPL_TRUE : CPL_FALSE;
    double asq2n        = 0.0;
    double bsq2n        = 0.0;
#ifdef CPL_VECTOR_LVMQ_DIAG
    double chi_sq_diff  = 0.0;
    double betamax      = 0.0;
#endif

    cpl_boolean didconverge = CPL_FALSE;

    /* If covariance computation is requested, then either
     * return the covariance matrix or return NULL.
     */
    if (covariance != NULL) *covariance = NULL;

    /* Validate input */
    cpl_ensure_code(x       != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(sigma_x == NULL, CPL_ERROR_UNSUPPORTED_MODE);
    cpl_ensure_code(y       != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(a       != NULL, CPL_ERROR_NULL_INPUT);
    /* ia may be NULL */
    cpl_ensure_code(f       != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(dfda    != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(relative_tolerance > 0, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(tolerance_count    > 0, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(max_iterations     > 0, CPL_ERROR_ILLEGAL_INPUT);

    /* Chi^2 and covariance computations require sigmas to be known */
    cpl_ensure_code( sigma_y != NULL ||
                     (red_chisq == NULL && covariance == NULL),
                     CPL_ERROR_INCOMPATIBLE_INPUT);

    D = cpl_matrix_get_ncol(x);
    N = cpl_matrix_get_nrow(x);
    M = cpl_vector_get_size_(a);
    cpl_ensure_code(N > 0, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(D > 0, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(M > 0, CPL_ERROR_ILLEGAL_INPUT);

    if (check_sanity) {
        cpl_ensure_code(uppersan != NULL, CPL_ERROR_NULL_INPUT);
        cpl_ensure_code(cpl_array_get_size(lowersan) == M,
                        CPL_ERROR_INCOMPATIBLE_INPUT);
        cpl_ensure_code(cpl_array_get_size(uppersan) == M,
                        CPL_ERROR_INCOMPATIBLE_INPUT);
        cpl_ensure_code(cpl_array_get_type(lowersan) == CPL_TYPE_DOUBLE,
                        CPL_ERROR_ILLEGAL_INPUT);
        cpl_ensure_code(cpl_array_get_type(uppersan) == CPL_TYPE_DOUBLE,
                        CPL_ERROR_ILLEGAL_INPUT);
    } else {
        cpl_ensure_code(uppersan == NULL, CPL_ERROR_NULL_INPUT);
    }

    cpl_ensure_code( cpl_vector_get_size_(y) == N,
             CPL_ERROR_INCOMPATIBLE_INPUT);

    x_data = cpl_matrix_get_data_const(x);
    y_data = cpl_vector_get_data_const_(y);
    a_data = cpl_vector_get_data_(a);

    if (sigma_y != NULL) {
        if (cpl_vector_get_size_(sigma_y) != N) {
            return cpl_error_set_message_(CPL_ERROR_INCOMPATIBLE_INPUT,
                                          "Y-sigma length = %d <=> %d",
                                          (int)cpl_vector_get_size(sigma_y),
                                          (int)N);
        } else if (cpl_vector_get_min(sigma_y) <= 0.0) {
            const cpl_size minpos = cpl_vector_get_minpos(sigma_y);
            return cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT, "Y-sigma "
                                          "minimum = %g <= 0.0 at %d/%d",
                                          cpl_vector_get(sigma_y, minpos),
                                          1+(int)cpl_vector_get_size(sigma_y),
                                          (int)N);
        }
        sigma_data = cpl_vector_get_data_const_(sigma_y);
    }

    /* Count non-constant fit parameters, copy ia */
    if (ia != NULL) {
        cpl_size Mallfree = 0;
        for (cpl_size i = 0; i < M; i++) {
            if (ia[i] != 0)
                Mfit++;
            else
                Mallfree = i + 1;
        }

        if (Mfit == 0) {
            return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        }

        /* Create alpha, beta, a_da, part  work space */
        cpl_fit_work_set(&work, &mybuf, (size_t)Mfit, (size_t)M, 0);

        fit_all = Mfit == M ? CPL_TRUE : CPL_FALSE;

        if (fit_all) assert(Mallfree == 0);

        work.Mallfree = Mallfree;

    } else {
        /* All parameters participate */
        const size_t sz = (size_t)M * sizeof(*ia_local);

        Mfit = M;

        /* Create alpha, beta, a_da, part + ia_local work space */
        cpl_fit_work_set(&work, &mybuf, (size_t)Mfit, (size_t)M, sz);

        assert(sz > 0); /* Avoid gcc-warning on zero-length memset() */
        ia_local = memset(work.ia, 1, sz);

        fit_all = CPL_TRUE;
        work.Mallfree = 0;
    }

    /* To compute reduced chi^2, we need N > Mfit */
    if (! ( red_chisq == NULL || N > Mfit ) )
    {
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
    }

    alpha = work.alpha;
    assert(alpha != NULL);

    a_da = work.a_da;
    assert(a_da != NULL);

    /* a_keep and a_da always point to a_data and work.a_da.
       When an improved solution is found, the pointers are swapped.
       If at convergence a_data points to the wrong one, the array is copied */
    a_keep = a_data;

    part = work.part;
    assert(part != NULL);

    /* Initialize loop variables */
    lambda = 0.001;
    count = 0;
    iterations = 0;
    if( (chi_sq = get_chisq(N, D, f, a_data, x_data, y_data, sigma_data)) < 0)
    {
        cpl_fit_work_delete(&work);
        return cpl_error_set_where_();
    }

    for (cpl_size i = 0; i < M; i++) {
        asq2n += a_data[i] * a_data[i];
    }

    /* Iterate until chi^2 didn't improve significantly many
       times in a row (where 'many' is defined by tolerance_count).
       Each iteration moves towards Newton's algorithm.
       The iterations counter is a protection against an infinte loop */
    for (; iterations < max_iterations; iterations++, lambda /= 10.0) {

        /* In each iteration lambda increases, or chi^2 decreases or
           count increases. Because chi^2 is bounded from below
           (and lambda and count from above), the loop will terminate */

        cpl_errorstate prevstate = cpl_errorstate_get();
        double * a_swap;
        double chi_sq_candidate = 0.0;
#if defined CPL_VECTOR_LVMQ_DIAG && CPL_VECTOR_LVMQ_DIAG > 1
        const double chi_sq_diff_prev = chi_sq_diff;
#endif
        double chi_sq_cand_diff;
        double asq2n_cand = 0.0;
        double bsq2n_cand = 0.0;
        const double bsq2n_prev = bsq2n;
        int canderror;
        int retries = 0; /* Counter of candidate retries (zero on 1st try) */
        /* The limit on number of retries is really provided by maxlambda
           nevertheless provide a hard limit */
        int ntries = CPL_FIT_LVMQ_MAXITER;
        cpl_boolean has_cand = CPL_FALSE;

        /* Get candidate position in parameter space = a+da,
         * where  alpha * da = beta .
         * Increase lambda until alpha is non-singular
         */

        /* Try to get a valid candidate a limited number of times
           - each retry after failure moves towards steepest descent */
        for (; retries < ntries && lambda < MAXLAMBDA; lambda *= 9.0, retries++) {
            /* Reset only error before (re)trying, so also on exhausting the
               tries errors are propagated */
            cpl_errorstate_set(prevstate);
            canderror = get_candidate(a_keep, ia_local,
                                      M, N, D,
                                      lambda, f, dfda,
                                      x_data, y_data, sigma_data,
                                      a_da, &asq2n_cand, &bsq2n_cand, &work);

            if (!canderror && cpl_errorstate_is_equal(prevstate)) {


                if (check_sanity &&
                    cpl_fit_is_sane(a_da, M, lowersan, uppersan)) {
#if defined CPL_VECTOR_LVMQ_DIAG && CPL_VECTOR_LVMQ_DIAG > 2
                    cpl_msg_warning(cpl_func, "LVMQ-ILLC(N=%d:%d<%d:%d<%d): "
                                    "lambda=%g, ||a||^2 = %g + %g = ||beta||^2 < "
                                    "||beta_prev||^2 = %g/%g (BetaMax=%g), "
                                    "chi_sq(%g) delta: %g (retries=%d)", (int)N,
                                    iterations, max_iterations, count,
                                    tolerance_count, lambda,
                                    asq2n, bsq2n, bsq2n_prev,
                                    relative_tolerance,
                                    didconverge ? 1.0 : (iterations ?
                                                         bsq2n / bsq2n_prev : 1.0),
                                    chi_sq, chi_sq_diff, (int)retries);
#endif
                    continue;
                }

                /* Get chi^2(a+da) */
                chi_sq_candidate = get_chisq(N, D, f, a_da, x_data, y_data,
                                             sigma_data);
                /* Check for invalid candidate, including NaN */
                if (cpl_errorstate_is_equal(prevstate)) {
                    if (chi_sq_candidate < 0.0 ||
                        chi_sq_candidate != chi_sq_candidate) {
                        continue; /* This candidate was no good */
                    }

                    /* Candidate is valid, its reduction of chi^2 is
                       negative when the solution is improved */
                    chi_sq_cand_diff = chi_sq_candidate - chi_sq;

                    if (chi_sq_cand_diff <= sqrt(DBL_EPSILON)) {
                        has_cand = CPL_TRUE;
                        break; /* This candidate reduces chi^2 */
                    }
                }
            }
        }

        if (!has_cand) {
            if (didconverge && !cpl_errorstate_is_equal(prevstate)) {
                /* Our solution is already good enough, so ignore last error */
                cpl_errorstate_set(prevstate);
            }
            break; /* The iteration diverged */
        }

        assert(cpl_errorstate_is_equal(prevstate));

        /* chi^2 improved, update a and chi^2 */
        a_swap = a_keep;
        a_keep = a_da;
        a_da   = a_swap;

        asq2n = asq2n_cand;
        bsq2n = bsq2n_cand;

        /* Convergence is when the change in the step-length in the parameter
           domain decreases and when reductions in chi^2 plateau, over several
           successive iterations. So:
           Count the number of successive improvements in chi^2 of
           less than 0.01 (default) relative
           - requiring that lambda does not increase much (i.e. at most 2 retries),
           - the step length should reduce or stay unchanged (up to a factor 2),
             unless it is effectively zero (unless we are just starting
             the convergence, i.e. count is 0) */
        if ((chi_sq < sqrt(DBL_EPSILON) ||
             -chi_sq_cand_diff < chi_sq * relative_tolerance) &&
            (count == 0 || (retries < 3 && 
                            (bsq2n < 2.0 * bsq2n_prev ||
                             bsq2n < asq2n * (double)(2<<Mfit) * sqrt(DBL_EPSILON))))) {
#if defined CPL_VECTOR_LVMQ_DIAG && CPL_VECTOR_LVMQ_DIAG > 1
            if (count > 0 &&
                !(bsq2n < asq2n * (double)(2<<Mfit) * sqrt(DBL_EPSILON)) &&
                bsq2n_prev > 0.0 && bsq2n > bsq2n_prev * betamax)
                betamax = bsq2n / bsq2n_prev;
#endif
            count++;
            if (count >= tolerance_count) {
                didconverge = CPL_TRUE;
            }
        } else {
            /* Chi^2 improved, by a significant amount or in some
               non-converging manner, so
               reset counter */
            count = 0;
        }

#if defined CPL_VECTOR_LVMQ_DIAG && CPL_VECTOR_LVMQ_DIAG > 1
        chi_sq_diff = chi_sq_cand_diff;
#endif
        chi_sq = chi_sq_candidate;

#if defined CPL_VECTOR_LVMQ_DIAG && CPL_VECTOR_LVMQ_DIAG > 1
        cpl_msg_info(cpl_func, "LVMQ(%d<%d:%d<%d:%c): lambda=%g, ||a||^2 = %g +"
                     " %g = ||beta||^2 < ||beta_prev||^2 = %g/%g (BetaMax=%g), "
                     "chi_sq(%g) delta: %g, conv: %g < %g, ratio: %g (retries=%d)",
                     iterations, max_iterations, count, tolerance_count,
                     didconverge ? 'T' : 'F', lambda,
                     asq2n, bsq2n, bsq2n_prev,
                     relative_tolerance,
                     didconverge ? 1.0 : (iterations ? bsq2n / bsq2n_prev : 1.0),
                     chi_sq, chi_sq_diff,
                     -chi_sq_cand_diff, chi_sq * relative_tolerance,
                     chi_sq_diff/chi_sq_diff_prev, retries);
#endif
        if (!didconverge) continue;

        /* Convergence ensured */

        /* Stop when the step length is at 1 bit/parameter on average
           + 1/2 machine precision */
        if (bsq2n < asq2n * (double)(2<<Mfit) * sqrt(DBL_EPSILON)) break;
        /* Stop when the step length increases */
        if (bsq2n >= bsq2n_prev) break;
    }

    if (a_keep != a_data) {
        assert(a_da == a_data);

        /* Update so both a_data and a_da may be used below */
        a_da = a_keep;

        /* - also need to copy solution from candidate to actual output */
        a_keep = memcpy(a_data, a_da, (size_t)M * sizeof(*a_data));
    }

    /* Set error if we didn't converge */
    if (!didconverge || !(lambda < MAXLAMBDA && iterations < max_iterations)) {
        /* Divergence or get_candidate()/get_chisq() failed in some way */
        const cpl_error_code code =
            cpl_error_get_code() == CPL_ERROR_SINGULAR_MATRIX
            ? CPL_ERROR_CONTINUE : cpl_error_get_code();

        cpl_fit_work_delete(&work);
        return cpl_error_set_message_(code ? code : CPL_ERROR_CONTINUE,
                                      "Iteration %d. lambda = %g. ||beta||^2 = "
                                      "%g", (int)iterations, lambda, bsq2n);
    }

#ifdef CPL_VECTOR_LVMQ_DIAG
        cpl_msg_info(cpl_func, "LVMQ-OK(%d<%d:%d<%d): lambda=%g, ||a||^2 = %g +"
                     " %g = ||beta||^2 (tol=%g) (BETAMAX=%g), "
                     "chi_sq(%g) delta: %g, conv: %g",
                     iterations, max_iterations, count, tolerance_count, lambda,
                     asq2n, bsq2n, relative_tolerance, betamax,
                     chi_sq, chi_sq_diff, chi_sq * relative_tolerance);
#endif

    /* Compute mse if requested */
    if (mse != NULL) {
        *mse = 0.0;

        for (cpl_size i = 0; i < N; i++) {
            double fx_i, residual;

            /* Evaluate f(x_i) at the best fit parameters */
            const int retval = f(x_data + i * D, a_data, &fx_i);

            if (retval) {
                cpl_fit_work_delete(&work);
                return cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT, "f(%g) "
                                              "= %d, k=%d/%d", x_data[i * D],
                                              retval, 1+(int)i,
                                              (int)N);
            }

            residual = y_data[i] - fx_i;
            *mse += residual * residual;
        }
        *mse /= (double)N;
    }

    /* Compute reduced chi^2 if requested */
    if (red_chisq != NULL) {
        /* We already know the optimal chi^2 (and that N > Mfit)*/
        *red_chisq = chi_sq / (double)(N-Mfit);
    }

    /* Compute covariance matrix if requested
     * cov = alpha(lambda=0)^-1
     */
    if (covariance != NULL) {
        cpl_matrix *cov = fit_all
            ? cpl_matrix_wrap(M,    M,    cpl_malloc(M * M * sizeof(double)))
            : cpl_matrix_wrap(Mfit, Mfit, cpl_malloc(Mfit * Mfit
                                                     * sizeof(double)));

        *covariance = NULL;

        if (get_candidate(a_data, ia_local,
                         M, N, D, 0.0, f, dfda,
                         x_data, y_data, sigma_data,
                         a_da, &asq2n, &bsq2n, &work) != 0) {
            cpl_fit_work_delete(&work);
            cpl_matrix_delete(cov);
            return cpl_error_set_where_();
        }

        /* Alpha is already its own Cholesky decomposition */
        /* So to create its inverse, solve with the identity matrix as
           right hand sides. */
        if (cpl_matrix_fill_identity(cov) ||
            cpl_matrix_solve_chol(work.alpha, cov)) {
            cpl_fit_work_delete(&work);
            cpl_matrix_delete(cov);
            return cpl_error_set_where_();
        }

        /* Make sure that variances are positive */
        for (cpl_size i = 0; i < Mfit; i++) {
            if ( !(cpl_matrix_get(cov, i, i) > 0.0) ) {
                cpl_fit_work_delete(&work);
                cpl_matrix_delete(cov);
                return cpl_error_set_(CPL_ERROR_SINGULAR_MATRIX);
            }
        }

        if (fit_all) {
            *covariance = cov;
        } else {
            /* Expand covariance matrix from Mfit x Mfit
               to M x M. Set rows/columns corresponding to fixed
               parameters to zero */

            cpl_size jmfit = 0;

            *covariance = cpl_matrix_new(M, M);

            for (cpl_size j = 0; j < M; j++) {
                if (ia_local[j] != 0) {
                    cpl_size imfit = 0;

                    for (cpl_size i = 0; i < M; i++)
                        if (ia_local[i] != 0) {
                            cpl_matrix_set(*covariance, i, j,
                                           cpl_matrix_get(cov, imfit, jmfit));
                            imfit++;
                        }

                    assert( imfit == Mfit );

                    jmfit++;
                }
            }
            assert( jmfit == Mfit );
            cpl_matrix_delete(cov);
        }
    }

    cpl_fit_work_delete(&work);

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/*
   @brief  Create a new work space
   @@param self   Pointer to work space to set
   @@param pmybuf Pointer to buffer for work space
   @param  mfit   Number of free elements to make space for
   @param  mtot   Total number of free elements to make space for
   @param  iasz   Bytes for ia, or zero for NULL
   @return void
   @see cpl_fit_lvmq_(), get_candidate()
*/
/*----------------------------------------------------------------------------*/
inline static void cpl_fit_work_set(cpl_fit_work * self,
                                    cpl_ifalloc * pmybuf,
                                    size_t mfit,
                                    size_t mtot,
                                    size_t iasz)
{
    const size_t size = mfit * (sizeof(double) * (1 + mfit) + sizeof(int))
        + 2 * mtot * sizeof(double) + mtot * sizeof(int) + iasz;

    double * start;

#if defined CPL_VECTOR_LVMQ_DIAG && CPL_VECTOR_LVMQ_DIAG > 1
    cpl_msg_warning(cpl_func, "LVMQ(%zu<=%zu): %zu <=> %u (%zu)", mfit, mtot,
                    size, CPL_IFALLOC_SZ, iasz);
#endif

    self->pmybuf     = pmybuf;

    self->Mfit       = mfit;

    cpl_ifalloc_set(self->pmybuf, size);

    start = cpl_ifalloc_get(self->pmybuf);

    self->a_da       = start;

    start           += mtot;

    self->part       = start;

    start           += mtot;

    /* Alpha and Beta contiguous so single memset() can reset them */
    self->dalphabeta = start;

    self->dbeta      = self->dalphabeta;

    self->beta       = cpl_matrix_wrap(mfit, 1, self->dbeta);

    start           += mfit;

    self->dalpha     = start;

    self->alpha      = cpl_matrix_wrap(mfit, mfit, self->dalpha);

    self->salphabeta = (mfit * mfit + mfit) * sizeof(double);

    self->alphabeta  = cpl_matrix_wrap(mfit, mfit + 1, self->dalphabeta);

    start           += mfit * mfit;

    self->ia         = iasz != 0 ? (void*)start : NULL;
}

/*----------------------------------------------------------------------------*/
/*
   @brief  Delete a work space
   @@param self   Pointer to work space to delete
   @return void
   @see cpl_fit_work_set()
*/
/*----------------------------------------------------------------------------*/
static void cpl_fit_work_delete(cpl_fit_work * self)
{
    cpl_ifalloc_free(self->pmybuf);
    (void)cpl_matrix_unwrap(self->beta);
    (void)cpl_matrix_unwrap(self->alpha);
    (void)cpl_matrix_unwrap(self->alphabeta);
}

/*----------------------------------------------------------------------------*/
/*
   @brief  Determine if the free parameters are within the provided bounds
   @param  self      Parameter array to check
   @param  lowersan  Parameter lower sanity bounds
   @param  uppersan  Parameter upper sanity bounds
   @return Zero if check is passed
*/
/*----------------------------------------------------------------------------*/
static int cpl_fit_is_sane(const double self[],
                           cpl_size M,
                           const cpl_array * lowersan,
                           const cpl_array * uppersan)
{
    int inv[2];
    cpl_boolean is_sane = CPL_TRUE;
 
    for (cpl_size i = 0; i < M; i++) {
        const double lower = cpl_array_get_double(lowersan, i, inv);
        const double upper = cpl_array_get_double(uppersan, i, inv + 1);

        if (!inv[0] && self[i] < lower) {
            is_sane = CPL_FALSE;
#if defined CPL_VECTOR_LVMQ_DIAG && CPL_VECTOR_LVMQ_DIAG > 2
            cpl_msg_warning(cpl_func, "LVMQ-Lower sanity check failed (%d<%d): "
                            "%g < %g < %g (%c)", (int)i, (int)M, lower, self[i],
                            upper, inv[1] ? 'I' : 'V');
#if CPL_VECTOR_LVMQ_DIAG > 3
            cpl_array_dump(lowersan,0, M, stderr);
            cpl_array_dump(uppersan,0, M, stderr);
#endif
#else
            break;
#endif
        }
        if (!inv[1] && self[i] > upper) {
            is_sane = CPL_FALSE;
#if defined CPL_VECTOR_LVMQ_DIAG && CPL_VECTOR_LVMQ_DIAG > 2
            cpl_msg_warning(cpl_func, "LVMQ-Upper sanity check failed (%d<%d): "
                            "%g (%c) < %g < %g", (int)i, (int)M, lower,
                            inv[0] ? 'I' : 'V', self[i], upper);
#if CPL_VECTOR_LVMQ_DIAG > 3
            cpl_array_dump(lowersan,0, M, stderr);
            cpl_array_dump(uppersan,0, M, stderr);
#endif
#else
            break;
#endif
        }
    }

    return is_sane ? 0 : 1;
}

#endif /* CPL_VECTOR_FIT_IMPL_H */
