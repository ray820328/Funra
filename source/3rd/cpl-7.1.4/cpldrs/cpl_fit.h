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

#ifndef CPL_FIT_H
#define CPL_FIT_H

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include <cpl_vector.h>
#include <cpl_matrix.h>
#include <cpl_array.h>
#include <cpl_imagelist.h>
#include <cpl_error.h>

CPL_BEGIN_DECLS

/*-----------------------------------------------------------------------------
                                   Defines
 -----------------------------------------------------------------------------*/
/*
 * Default parameter values for
 * Levenberg-Marquardt fitting
 */

#ifndef CPL_FIT_LVMQ_TOLERANCE
#define CPL_FIT_LVMQ_TOLERANCE 0.01
#endif
/* Should be << 1.
   (A relative decrease in chi squared
   << 1 is not statistically significant
   so there is no point in converging
   to machine precision.) */

#ifndef CPL_FIT_LVMQ_COUNT
#define CPL_FIT_LVMQ_COUNT     5
#endif
/* Should be somewhat greater than 1 */

#ifndef CPL_FIT_LVMQ_MAXITER
#define CPL_FIT_LVMQ_MAXITER   1000
#endif
/* Should be >> 1 */

/*-----------------------------------------------------------------------------
                            Function prototypes
 -----------------------------------------------------------------------------*/

cpl_error_code cpl_fit_lvmq(const cpl_matrix*, const cpl_matrix*,
                            const cpl_vector*, const cpl_vector*,
                            cpl_vector*, const int[],
                            int    (*)(const double[],
                                       const double[], double*),
                            int (*)(const double[],
                                    const double[], double[]),
                            double, int, int, double*, double*, cpl_matrix**);


cpl_imagelist* cpl_fit_imagelist_polynomial(const cpl_vector*,    
                                            const cpl_imagelist*, 
                                            cpl_size, cpl_size, cpl_boolean,
                                            cpl_type, cpl_image*)
    CPL_ATTR_ALLOC;

cpl_imagelist* cpl_fit_imagelist_polynomial_window(const cpl_vector*,    
                                                   const cpl_imagelist*, 
                                                   cpl_size, cpl_size,
                                                   cpl_size, cpl_size,
                                                   cpl_size, cpl_size,
                                                   cpl_boolean,
                                                   cpl_type, cpl_image*)
    CPL_ATTR_ALLOC;

cpl_error_code cpl_fit_image_gaussian(const cpl_image*, const cpl_image*, 
                                      cpl_size,         cpl_size,
                                      cpl_size,         cpl_size,
                                      cpl_array*,       cpl_array*,
                                      const cpl_array*, double*,
                                      double*,          cpl_matrix**,
                                      double*,          double*,
                                      double*,          cpl_matrix**);

double cpl_gaussian_eval_2d(const cpl_array *, double, double);

CPL_END_DECLS

#endif
/* end of cpl_fit.h */
