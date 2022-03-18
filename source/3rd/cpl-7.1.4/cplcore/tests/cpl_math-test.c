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

#include <math.h>
#include <float.h>

#include "cpl_math_const.h"
#include "cpl_test.h"

/*-----------------------------------------------------------------------------
                                  Defines
 -----------------------------------------------------------------------------*/

/* Required precision in multiples of the machine precision */
#ifndef CPL_MATH_PRECISION
#define CPL_MATH_PRECISION 1.0
#endif

/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/
int main(void)
{
    const double prec = CPL_MATH_PRECISION * DBL_EPSILON;

    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    /* Insert tests below */

    /* Expressions with just a subtraction must be accurate to 1 unit
       of machine precision, expressions also involving division must be
       accurate to 2 units of machine precision */

    /* pi */
    cpl_test_rel(atan2(0.0, -1.0), CPL_MATH_PI, prec);

    /* 2 pi */
    cpl_test_rel(2.0 * atan2(0.0, -1.0), CPL_MATH_2PI, prec);

    /* pi/2 */
    cpl_test_rel(atan2(1.0, 0.0), CPL_MATH_PI_2, prec);

    /* pi/4 */
    cpl_test_rel(atan2(1.0, 1.0), CPL_MATH_PI_4, prec);

    /* 1/pi */
    cpl_test_rel(1.0/atan2(0.0, -1.0), CPL_MATH_1_PI, 2.0 * prec);

    /* 2/pi */
    cpl_test_rel(1.0/atan2(1.0, 0.0), CPL_MATH_2_PI, 2.0 * prec);

    /* 4/pi */
    cpl_test_rel(1.0/atan2(1.0, 1.0), CPL_MATH_4_PI, 2.0 * prec);

    /* sqrt(2pi) */
    cpl_test_rel(sqrt(2.0 * atan2(0.0, -1.0)), CPL_MATH_SQRT2PI, 2.0 * prec);

    /* 2 / sqrt(pi) */
    cpl_test_rel(1.0 / sqrt(atan2(1.0, 1.0)), CPL_MATH_2_SQRTPI, 2.0 * prec);

    /* sqrt(2) */
    cpl_test_rel(sqrt(2.0), CPL_MATH_SQRT2, prec);

    /* sqrt(3) */
    cpl_test_rel(sqrt(3.0), CPL_MATH_SQRT3, prec);

    /* sqrt(1/2) */
    cpl_test_rel(sqrt(0.5), CPL_MATH_SQRT1_2, prec);

    /* exp(1) */
    cpl_test_rel(exp(1.0), CPL_MATH_E, prec);

    /* log(2) */
    cpl_test_rel(log(2.0), CPL_MATH_LN2, prec);

    /* log(10) */
    cpl_test_rel(log(10.0), CPL_MATH_LN10, prec);

    /* log2(e) */
    cpl_test_rel(1.0/log(2.0), CPL_MATH_LOG2E, 2.0 * prec);

    /* log10(e) */
    cpl_test_rel(log10(exp(1.0)), CPL_MATH_LOG10E, prec);

    /* 180/pi */
    cpl_test_rel(45.0 / atan2(1.0, 1.0), CPL_MATH_DEG_RAD, 2.0 * prec);

    /* pi/180 */
    cpl_test_rel(CPL_MATH_RAD_DEG * CPL_MATH_DEG_RAD, 1.0, prec);

    /* 2.0*sqrt(2.0*log(2.0)) */
    cpl_test_rel(2.0*sqrt(2.0*log(2.0)), CPL_MATH_FWHM_SIG, prec);

    /* 0.5/sqrt(2.0*log(2.0)) */
    cpl_test_rel(0.5/sqrt(2.0*log(2.0)), CPL_MATH_SIG_FWHM, 2.0 * prec);

    cpl_test_zero(CPL_MIN(0, 1));
    cpl_test_zero(CPL_MAX(0, -1));

    cpl_test_zero(CPL_MIN(0.0, 1.0));
    cpl_test_zero(CPL_MAX(0.0, -1.0));

    return cpl_test_end(0);
}
