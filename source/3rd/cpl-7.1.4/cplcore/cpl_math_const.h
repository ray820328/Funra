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

#ifndef CPL_MATH_CONST_H
#define CPL_MATH_CONST_H

#include <cpl_macros.h>

CPL_BEGIN_DECLS

/**
 * @defgroup cpl_math Fundamental math functionality
 *
 * This module provides fundamental math constants.
 *
 * Source: On-Line Encyclopedia of Integer Sequences (OEIS)
 *
 * pi:      http://www.research.att.com/~njas/sequences/A000796
 *
 * e:       http://www.research.att.com/~njas/sequences/A001113
 *
 * ln(2):   http://www.research.att.com/~njas/sequences/A002162
 *
 * ln(10):  http://www.research.att.com/~njas/sequences/A002392
 *
 * sqrt(2): http://www.research.att.com/~njas/sequences/A002193
 *
 * sqrt(3): http://www.research.att.com/~njas/sequences/A002194
 *
 * The derived constants have been computed with the
 * GNU Multiple-Precision Library v. 4.2.2.
 *
 * The constants are listed with a precision that allows a one-line definition.
 *
 * @par Synopsis:
 * @code
 *   #include <cpl_math_const.h>
 * @endcode
 */

/**@{*/

/*-----------------------------------------------------------------------------
                                Defines
 -----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/**
 * @brief  The base of the exponential function
 * @see On-Line Encyclopedia of Integer Sequences (OEIS),
 *      http://www.research.att.com/~njas/sequences/A001113
 */
/*----------------------------------------------------------------------------*/
#define CPL_MATH_E        2.7182818284590452353602874713526624977572470936999595

/*----------------------------------------------------------------------------*/
/**
 * @brief  The ratio of a circles circumference to its diameter
 * @see On-Line Encyclopedia of Integer Sequences (OEIS),
 *      http://www.research.att.com/~njas/sequences/A000796
 */
/*----------------------------------------------------------------------------*/
#define CPL_MATH_PI       3.1415926535897932384626433832795028841971693993751058

/*----------------------------------------------------------------------------*/
/**
 * @brief  The natural logarithm of 2
 * @see On-Line Encyclopedia of Integer Sequences (OEIS),
 *      http://www.research.att.com/~njas/sequences/A002162
 */
/*----------------------------------------------------------------------------*/
#define CPL_MATH_LN2      0.6931471805599453094172321214581765680755001343602553

/*----------------------------------------------------------------------------*/
/**
 * @brief  The natural logarithm of 10
 * @see On-Line Encyclopedia of Integer Sequences (OEIS),
 *      http://www.research.att.com/~njas/sequences/A002392
 */
/*----------------------------------------------------------------------------*/
#define CPL_MATH_LN10     2.3025850929940456840179914546843642076011014886287730

/*----------------------------------------------------------------------------*/
/**
  @brief 2 pi
  @see CPL_MATH_PI
  @note Derived from a fundamental constant
 */
/*----------------------------------------------------------------------------*/
#define CPL_MATH_2PI      6.2831853071795864769252867665590057683943387987502116

/*----------------------------------------------------------------------------*/
/**
  @brief pi/2
  @see CPL_MATH_PI
  @note Derived from a fundamental constant
 */
/*----------------------------------------------------------------------------*/
#define CPL_MATH_PI_2     1.5707963267948966192313216916397514420985846996875529

/*----------------------------------------------------------------------------*/
/**
  @brief pi/4
  @see CPL_MATH_PI
  @note Derived from a fundamental constant
 */
/*----------------------------------------------------------------------------*/
#define CPL_MATH_PI_4     0.7853981633974483096156608458198757210492923498437765

/*----------------------------------------------------------------------------*/
/**
  @brief 1/pi
  @see CPL_MATH_PI
  @note Derived from a fundamental constant
 */
/*----------------------------------------------------------------------------*/
#define CPL_MATH_1_PI     0.3183098861837906715377675267450287240689192914809129

/*----------------------------------------------------------------------------*/
/**
  @brief 2/pi
  @see CPL_MATH_PI
  @note Derived from a fundamental constant
 */
/*----------------------------------------------------------------------------*/
#define CPL_MATH_2_PI     0.6366197723675813430755350534900574481378385829618258

/*----------------------------------------------------------------------------*/
/**
  @brief 4/pi
  @see CPL_MATH_PI
  @note Derived from a fundamental constant
 */
/*----------------------------------------------------------------------------*/
#define CPL_MATH_4_PI     1.2732395447351626861510701069801148962756771659236516

/*----------------------------------------------------------------------------*/
/**
  @brief sqrt(2pi)
  @see CPL_MATH_PI
  @note Derived from a fundamental constant
 */
/*----------------------------------------------------------------------------*/
#define CPL_MATH_SQRT2PI  2.5066282746310005024157652848110452530069867406099383

/*----------------------------------------------------------------------------*/
/**
  @brief 2/sqrt(pi)
  @see CPL_MATH_PI
  @note Derived from a fundamental constant
 */
/*----------------------------------------------------------------------------*/
#define CPL_MATH_2_SQRTPI 1.1283791670955125738961589031215451716881012586579977

/*----------------------------------------------------------------------------*/
/**
 * @brief  The square root of 2
 * @see On-Line Encyclopedia of Integer Sequences (OEIS),
 *      http://www.research.att.com/~njas/sequences/A002193
 */
/*----------------------------------------------------------------------------*/
#define CPL_MATH_SQRT2    1.4142135623730950488016887242096980785696718753769481

/*----------------------------------------------------------------------------*/
/**
 * @brief  The square root of 3
 * @see On-Line Encyclopedia of Integer Sequences (OEIS),
 *      http://www.research.att.com/~njas/sequences/A002194
 */
/*----------------------------------------------------------------------------*/
#define CPL_MATH_SQRT3    1.7320508075688772935274463415058723669428052538103806

/*----------------------------------------------------------------------------*/
/**
  @brief sqrt(1/2)
  @see CPL_MATH_SQRT2
  @note Derived from a fundamental constant
 */
/*----------------------------------------------------------------------------*/
#define CPL_MATH_SQRT1_2  0.7071067811865475244008443621048490392848359376884740

/*----------------------------------------------------------------------------*/
/**
  @brief log2(e)
  @see CPL_MATH_LN2
  @note Derived from a fundamental constant
 */
/*----------------------------------------------------------------------------*/
#define CPL_MATH_LOG2E    1.4426950408889634073599246810018921374266459541529859

/*----------------------------------------------------------------------------*/
/**
  @brief log10(e)
  @see CPL_MATH_LN10
  @note Derived from a fundamental constant
 */
/*----------------------------------------------------------------------------*/
#define CPL_MATH_LOG10E   0.4342944819032518276511289189166050822943970058036666

/*----------------------------------------------------------------------------*/
/**
  @brief 180/pi
  @see CPL_MATH_PI
  @note Derived from a fundamental constant
 */
/*----------------------------------------------------------------------------*/
#define CPL_MATH_DEG_RAD  57.295779513082320876798154814105170332405472466564322

/*----------------------------------------------------------------------------*/
/**
  @brief pi/180
  @see CPL_MATH_PI
  @note Derived from a fundamental constant
 */
/*----------------------------------------------------------------------------*/
#define CPL_MATH_RAD_DEG  0.0174532925199432957692369076848861271344287188854173

/*----------------------------------------------------------------------------*/
/**
  @brief FWHM per Sigma, 2.0*sqrt(2.0*log(2.0))
  @see CPL_MATH_LN2
  @note Derived from a fundamental constant
 */
/*----------------------------------------------------------------------------*/
#define CPL_MATH_FWHM_SIG 2.3548200450309493820231386529193992754947713787716411

/*----------------------------------------------------------------------------*/
/**
  @brief Sigma per FWHM, 0.5/sqrt(2.0*log(2.0))
  @see CPL_MATH_LN2
  @note Derived from a fundamental constant
 */
/*----------------------------------------------------------------------------*/
#define CPL_MATH_SIG_FWHM 0.4246609001440095213607514170514448098575705468921770

/*----------------------------------------------------------------------------*/
/**
  @brief Standard deviation per Median Absolute Deviation for Gaussian data
  @see cpl_image_get_mad_window()

  For a Gaussian distribution the Median Absolute Deviation (MAD) is a robust
  and consistent estimate of the Standard Deviation (STD) in the sense that the
  STD is approximately K * MAD, where K is a constant equal to approximately
  1.4826.

 */
/*----------------------------------------------------------------------------*/
#define CPL_MATH_STD_MAD 1.4826


/*----------------------------------------------------------------------------*/
/**
  @hideinitializer
  @brief  Return the minimum of two values
  @param  first   The first expression in the comparison
  @param  second  The second expression in the comparison
  @return The minimum of the two values
  @note If the first argument is the smallest then it is evaluated twice
        otherwise the second argument is evaluated twice

 */
/*----------------------------------------------------------------------------*/
#define CPL_MIN(first, second)                                          \
  (((first) < (second)) ? (first) : (second)) /* Two evaluations of minimum */

/*----------------------------------------------------------------------------*/
/**
  @hideinitializer
  @brief  Return the maximum of two values
  @param  first   The first expression in the comparison
  @param  second  The second expression in the comparison
  @return The maximum of the two values
  @see CPL_MIN()

 */
/*----------------------------------------------------------------------------*/
#define CPL_MAX(first, second)                  \
  (((first) > (second)) ? (first) : (second)) /* Two evaluations of maximum */

/**@}*/

CPL_END_DECLS

#endif

