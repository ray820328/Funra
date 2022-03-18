/*
 * This file is part of the ESO cpl package
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

#define CPL_TYPE_ADD(a) CONCAT2X(a, CPL_TYPE)
#define CPL_TYPE_ADD_CONST(a) CONCAT3X(a, CPL_TYPE, const)

/*----------------------------------------------------------------------------*/
/**
  @brief  Fit a polynomial to each pixel in a list of images
  @param  self     Preallocated imagelist to hold fitting result
  @param  ml       Lower triangular matrix L, L * L' = V' * V
  @param  mv       The transpose of the Vandermonde matrix, V'
  @param  x_pos    The vector of positions to fit (used for fiterror only)
  @param  values   The list of images with values to fit
  @param  llx      Lower left x coordinate
  @param  lly      Lower left y coordinate
  @param  xpow     The mindeg powers of x_pos (or NULL, when mindeg is zero)
  @param  xnmean   Minus the mean value of the x_pos elements (or zero)
  @param  fiterror When non-NULL, the error of the fit
  @return CPL_ERROR_NONE or the relevant CPL error code on error
  @see cpl_fit_imagelist_polynomial

 */
/*----------------------------------------------------------------------------*/
static cpl_error_code
CPL_TYPE_ADD(cpl_fit_imagelist_polynomial)(cpl_imagelist * self,
                                           const cpl_matrix * ml,
                                           const cpl_matrix * mv,
                                           const cpl_vector * x_pos,
                                           const cpl_imagelist * values,
                                           cpl_size llx, cpl_size lly,
                                           const cpl_vector * xpow,
                                           double xnmean,
                                           cpl_image * fiterror)
{

    const cpl_image * first = cpl_imagelist_get(self, 0);
    const cpl_type    pixeltype = cpl_image_get_type(first);

    const cpl_image * value = cpl_imagelist_get_const(values, 0);
    const cpl_size    innx  = cpl_image_get_size_x(value);

    const cpl_size    np = cpl_imagelist_get_size(values);
    const cpl_size    nx = cpl_image_get_size_x(first);
    const cpl_size    ny = cpl_image_get_size_y(first);
    const cpl_size    nc = cpl_imagelist_get_size(self);
    const cpl_boolean do_err = fiterror != NULL ? CPL_TRUE : CPL_FALSE;

    const cpl_type    errtype
        = do_err ? cpl_image_get_type(fiterror) : CPL_TYPE_INVALID;


    /* Total number of fits to do */
    const cpl_size    imsize = nx * ny;
    /* Process this many pixel fits in one go to improve cache usage
       - The code will be correct for any natural-number-value of ijblock,
       but a too small and a too large value will slow down the code. */
    const cpl_size    ijblock
        = cpl_fit_imagelist_polynomial_find_block_size(np, nc, do_err,
                                                       cpl_image_get_type(value),
                                                       pixeltype,
                                                       errtype);

    /* - except in that the last block may have fewer points */
    cpl_size          ijdo    = ijblock;

    const CPL_TYPE ** pi  = cpl_malloc((size_t)np * sizeof(*pi));
    double          * px  = cpl_malloc((size_t)(ijblock * nc) * sizeof(*px));
    double          * pbw = cpl_malloc((size_t)(ijblock * np) * sizeof(*pbw));
    /* The polynomial coefficients to be solved for
       - transposed, so the ijblock polynomials are stored consequtively */
    cpl_matrix      * mx  = cpl_matrix_wrap(ijblock, nc, px);
    /* The transpose of one row of values to be fitted */
    cpl_matrix      * mb  = cpl_matrix_wrap(ijblock, np, pbw); 
    cpl_size          i, k;
    cpl_size ij, ijstop;
    /* First pixel to read from - in row i */
    cpl_size ini0 = (llx - 1) + (lly - 1) * innx;
    cpl_size ini = 0; /* Avoid (false) uninit warning */
    cpl_size ii0; /* Row index of input image buffer */
    cpl_size ii = 0; /* Avoid (false) uninit warning */
    cpl_error_code error = CPL_ERROR_NONE;

    for (k=0; k < np; k++) {
        pi[k] = CPL_TYPE_ADD_CONST(cpl_image_get_data)
            (cpl_imagelist_get_const(values, k));
    }

    /* Process ijdo pixels at a time - Improves cache usage  */
    for (ij = 0, ii0 = 0; ij < imsize; ij += ijdo) {

        if (ij + ijdo > imsize) ijdo = imsize - ij;

        /* Upper limit for pixel index in this iteration */
        ijstop = ij + ijdo;
 
        /* Fill in matrices */
        for (k=0; k < np; k++) {

            ii = ii0;
            ini = ini0;
            for (i=0; i < ijdo; i++, ii++) {
                if (ii == nx) {
                    ii = 0;
                    ini += innx;
                }

                /* The fact the mb is transposed makes this operation more
                   expensive - which is OK, since it has a lower complexity
                   than the subsequent use of mb */
                pbw[np * i + k] = (double)pi[k][ini + ii];
            }
        }
        ii0 = ii;
        ini0 = ini;

        /* Form the right hand side of the normal equations, X = V' * B */
        cpl_matrix_product_transpose(mx, mb, mv);

        /* In-place solution of the normal equations, L * L' * X = V' * B */
        cpl_matrix_solve_chol_transpose(ml, mx);

        if (xnmean != 0.0) {
            /* Shift polynomials back */
            for (i=0; i < ijdo; i++) {
                cpl_polynomial_shift_double(px + i * nc, (int)nc, xnmean);
            }
        }

        /* Copy results to output image list */

        switch (pixeltype) {
            case CPL_TYPE_DOUBLE:
                cpl_fit_imagelist_fill_double(self, ij, ijstop, mx);
                break;
            case CPL_TYPE_FLOAT:
                cpl_fit_imagelist_fill_float(self, ij, ijstop, mx);
                break;
            case CPL_TYPE_INT:
                cpl_fit_imagelist_fill_int(self, ij, ijstop, mx);
                break;
            default:
                /* It is an error in CPL to reach this point */
                (void)cpl_error_set_(CPL_ERROR_UNSPECIFIED);
        }

        if (fiterror != NULL) {
            switch (cpl_image_get_type(fiterror)) {
            case CPL_TYPE_DOUBLE:
                cpl_fit_imagelist_residual_double(fiterror, ij, ijstop,
                                                     x_pos, xpow, mx, mb);
                break;
            case CPL_TYPE_FLOAT:
                cpl_fit_imagelist_residual_float(fiterror, ij, ijstop,
                                                    x_pos, xpow, mx, mb);
                break;
            case CPL_TYPE_INT:
                cpl_fit_imagelist_residual_int(fiterror, ij, ijstop,
                                                  x_pos, xpow, mx, mb);
                break;
            default:
                error = cpl_error_set_(CPL_ERROR_UNSUPPORTED_MODE);
            }
            if (error) break;
        }
    }
    cpl_matrix_delete(mx);
    cpl_matrix_delete(mb);
    cpl_free(pi);

    return error;
}



/*----------------------------------------------------------------------------*/
/**
  @brief  Fill a row in the imagelist with the contents of the matrix
  @param  self     Preallocated imagelist
  @param  jj       The index of the image row to fill
  @param  mx       The transpose of the computed fitting coefficients
  @return void

  The fact the mx is transposed makes this operation more expensive
  - which is OK, since it has a lower complexity than the other use of mx

 */
/*----------------------------------------------------------------------------*/
static void CPL_TYPE_ADD(cpl_fit_imagelist_fill)(cpl_imagelist * self,
                                                 cpl_size ij, cpl_size ijstop,
                                                 const cpl_matrix * mx)
{

    const cpl_size ijdo = ijstop - ij;
    const cpl_size nc = cpl_matrix_get_ncol(mx);
    const double * px = cpl_matrix_get_data_const(mx);
    cpl_size       i, k;


    assert(cpl_imagelist_get_size(self) == nc);

    for (k=0; k < nc; k++) {
        CPL_TYPE * pdest = ij
            + CPL_TYPE_ADD(cpl_image_get_data)(cpl_imagelist_get(self, k));
        for (i=0; i < ijdo; i++) {
#ifdef CPL_TYPE_INT_ROUND
            /* Round off to nearest integer */
            pdest[i] = CPL_TYPE_INT_ROUND(px[nc * i + k]);
#else
            pdest[i] = (CPL_TYPE)px[nc * i + k];
#endif
        }
    }
}

/*----------------------------------------------------------------------------*/
/**
  @brief  Compute the residual of a polynomial fit to an imagelist
  @param  self     Preallocated image to hold residual
  @param  jj       The index of the image row to compute
  @param  x_pos    The vector of positions to fit (used for fiterror only)
  @param  xpow     The mindeg powers of x_pos (or NULL, when mindeg is zero)
  @param  mx       The transpose of the computed fitting coefficients
  @param  mb       The transpose of the values to be fitted
  @return void
  @see cpl_fit_imagelist_polynomial()

  The call requires (ijstop - ij) * (np * (2 * nc + 1) + 1) FLOPs.

 */
/*----------------------------------------------------------------------------*/
static void CPL_TYPE_ADD(cpl_fit_imagelist_residual)(cpl_image * self,
                                                     cpl_size ij,
                                                     cpl_size ijstop,
                                                     const cpl_vector * x_pos,
                                                     const cpl_vector * xpow,
                                                     const cpl_matrix * mx,
                                                     const cpl_matrix * mb)
{
    double         err, sq_err;

    const cpl_size nc = cpl_matrix_get_ncol(mx);
    const cpl_size np = cpl_vector_get_size(x_pos);

    /* pself points to 1st element in jj'th row */
    CPL_TYPE     * pself = CPL_TYPE_ADD(cpl_image_get_data)(self);

    const double * pp = cpl_vector_get_data_const(x_pos);

    /* If mindeg == 0 xpow is NULL (since no multiplication is needed) */
    const double * pm = xpow != NULL ? cpl_vector_get_data_const(xpow) : NULL;
    const double * px = cpl_matrix_get_data_const(mx);
    const double * pb = cpl_matrix_get_data_const(mb);

    cpl_size i, j, k;


    for (i=ij; i < ijstop; i++, pb += np, px += nc) {
        sq_err = 0.0;
        for (j=0; j < np; j++) {

            /* Evaluate the polynomial using Horners scheme, see
               cpl_polynomial_eval_1d() */
            k = nc;
            err = px[--k];

            while (k) err = pp[j] * err + px[--k];

            /* Multiply by x^mindeg - wheen needed */
            /* Substract expected value to compute the residual */
            if (pm != NULL) {
                err = err * pm[j] - pb[j];
            } else {
                err -= pb[j];
            }

            sq_err += err * err;
        }
#ifdef CPL_TYPE_INT_ROUND
        /* Round off to nearest integer */
        pself[i] = CPL_TYPE_INT_ROUND(sq_err/(double)np);
#else
        pself[i] = (CPL_TYPE)(sq_err/(double)np);
#endif
    }

    cpl_tools_add_flops((cpl_flops)((ijstop - ij) * (np * (2 * nc + 1) + 1)));

}

#undef CPL_TYPE_ADD
