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

#ifndef CPL_TEST_H
#define CPL_TEST_H

/*-----------------------------------------------------------------------------
                                    Includes
 -----------------------------------------------------------------------------*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cpl_macros.h>
#include <cpl_msg.h>
#include <cpl_type.h>

#include <cpl_array.h>
#include <cpl_imagelist.h>
#include <cpl_polynomial.h>
#include <cpl_errorstate.h>

#include <errno.h>

/*-----------------------------------------------------------------------------
                                    Defines
 -----------------------------------------------------------------------------*/

#ifndef CPL_TEST_FITS
#define CPL_TEST_FITS "CPL_TEST_FITS"
#endif

CPL_BEGIN_DECLS

/*----------------------------------------------------------------------------*/
/**
   @hideinitializer
   @ingroup cpl_test
   @brief Initialize CPL + CPL messaging + unit test
   @param REPORT  The email address for the error message e.g. PACKAGE_BUGREPORT
   @param LEVEL   The default messaging level, e.g. CPL_MSG_WARNING
   @return void
   @see cpl_init()
   @note  This macro should be used at the beginning of main() of a unit test
   instead of cpl_init() and before any other CPL function call.
 */
/*----------------------------------------------------------------------------*/
#define cpl_test_init(REPORT, LEVEL) \
  cpl_test_init_macro(__FILE__, REPORT, LEVEL)

/*----------------------------------------------------------------------------*/
/**
   @hideinitializer
   @ingroup cpl_test
   @brief  Evaluate an expression and increment an internal counter if zero
   @param  bool   The expression to evaluate, side-effects are allowed
   @note   A zero value of the expression is a failure, other values are not
   @return void
   @see cpl_test_init()
   @note  This macro should be used for unit tests

  Example of usage:
   @code

   cpl_test(myfunc()); // myfunc() is expected to return non-zero
   
   @endcode

 */
/*----------------------------------------------------------------------------*/
#define cpl_test(bool) do {                                             \
        const int cpl_test_errno = errno;                               \
        const cpl_flops cpl_test_flops = cpl_test_get_flops();          \
        const double cpl_test_time = cpl_test_get_walltime();           \
        cpl_errorstate cpl_test_state = cpl_errorstate_get();           \
        cpl_test_macro(cpl_test_errno, cpl_test_time, cpl_test_flops,   \
                       cpl_test_state, (bool) ? 1 : 0, CPL_TRUE, #bool, \
                       cpl_func, __FILE__, __LINE__);                   \
    } while (0)

/*----------------------------------------------------------------------------*/
/**
   @hideinitializer
   @ingroup cpl_test
   @brief  Evaluate an expression and increment an internal counter if non-zero
   @param  zero   The numerical expression to evaluate, side-effects are allowed
   @note   A zero value of the expression is a success, other values are not
   @return void
   @see cpl_test()
   @note  This macro should be used for unit tests

  Example of usage:
   @code

   cpl_test_zero(myfunc()); // myfunc() is expected to return zero
   
   @endcode

 */
/*----------------------------------------------------------------------------*/
#define cpl_test_zero(zero) do {                                        \
        const int cpl_test_errno = errno;                               \
        const cpl_flops cpl_test_flops = cpl_test_get_flops();          \
        const double cpl_test_time = cpl_test_get_walltime();           \
        cpl_errorstate cpl_test_state = cpl_errorstate_get();           \
        cpl_test_macro(cpl_test_errno, cpl_test_time, cpl_test_flops,   \
                       cpl_test_state, (zero), CPL_FALSE,               \
                       #zero, cpl_func, __FILE__, __LINE__);            \
    } while (0)

/*----------------------------------------------------------------------------*/
/**
  @hideinitializer
  @ingroup cpl_test
  @brief  Test if a pointer is NULL and update an internal counter on failure
  @param  pointer    The NULL-pointer to check, side-effects are allowed

  @see cpl_test()
  @return void

  Example of usage:
   @code

   cpl_test_null(pointer); // pointer is expected to be NULL
   
   @endcode

 */
/*----------------------------------------------------------------------------*/
#define cpl_test_null(pointer) do {                                     \
        const int cpl_test_errno = errno;                               \
        const cpl_flops cpl_test_flops = cpl_test_get_flops();          \
        const double cpl_test_time = cpl_test_get_walltime();           \
        cpl_errorstate cpl_test_state = cpl_errorstate_get();           \
        cpl_test_null_macro(cpl_test_errno, cpl_test_time,              \
                            cpl_test_flops, cpl_test_state, pointer,    \
                            #pointer,  cpl_func, __FILE__, __LINE__);   \
    } while (0)

/*----------------------------------------------------------------------------*/
/**
  @hideinitializer
  @ingroup cpl_test
  @brief  Test if a pointer is non-NULL
  @param  pointer    The pointer to check, side-effects are allowed

  @see cpl_test_nonnull()
  @return void

  Example of usage:
   @code

   cpl_test_nonnull(pointer); // pointer is expected to be non-NULL
   
   @endcode

 */
/*----------------------------------------------------------------------------*/
#define cpl_test_nonnull(pointer) do {                                  \
        const int cpl_test_errno = errno;                               \
        const cpl_flops cpl_test_flops = cpl_test_get_flops();          \
        const double cpl_test_time = cpl_test_get_walltime();           \
        cpl_errorstate cpl_test_state = cpl_errorstate_get();           \
        cpl_test_nonnull_macro(cpl_test_errno, cpl_test_time,           \
                               cpl_test_flops, cpl_test_state,          \
                               pointer, #pointer, cpl_func,             \
                               __FILE__, __LINE__);                     \
    } while (0)


/*----------------------------------------------------------------------------*/
/**
  @hideinitializer
  @ingroup cpl_test
  @brief  Test and if necessary reset the CPL errorstate
  @param  errorstate The expected CPL errorstate

  @see cpl_test()
  @note After the test, the CPL errorstate is set to the provided state
  @return void

  This function is useful for verifying that a successful call to a function
  does not modify any pre-existing errors.

  Example of usage:
   @code

   const cpl_error_code error = cpl_error_set(cpl_func, CPL_ERROR_EOL);
   cpl_errorstate prestate = cpl_errorstate_get();
   const cpl_error_code ok = my_func(); // Expected to succeed

   cpl_test_errorstate(prestate); // Verify that no additional errors occurred
   cpl_test_error(CPL_ERROR_EOL); // Reset error
   cpl_test_eq(ok, CPL_ERROR_NONE); // Verify that my_func() succeeded
   
   @endcode

 */
/*----------------------------------------------------------------------------*/
#define cpl_test_errorstate(errorstate) do {                            \
        const int cpl_test_errno = errno;                               \
        const cpl_flops cpl_test_flops = cpl_test_get_flops();          \
        const double cpl_test_time = cpl_test_get_walltime();           \
        cpl_errorstate cpl_test_state = cpl_errorstate_get();           \
        cpl_test_errorstate_macro(cpl_test_errno, cpl_test_time,        \
                                  cpl_test_flops, cpl_test_state,       \
                                  errorstate, #errorstate, cpl_func,    \
                                  __FILE__, __LINE__);                  \
    } while (0)

/*----------------------------------------------------------------------------*/
/**
  @hideinitializer
  @ingroup cpl_test
  @brief  Test and reset the CPL error code
  @param  error The expected CPL error code (incl. CPL_ERROR_NONE)

  @see cpl_test()
  @note After the test, the CPL errorstate is reset
  @return void

  Example of usage:
   @code

   cpl_test( my_func(NULL) ); // my_func(NULL) is expected to return non-zero

   cpl_test_error(CPL_ERROR_NULL_INPUT); // ... and to set this error code

   // The errorstate has been reset.

   cpl_test( !my_func(p) ); // my_func(p) is expected to return zero
   
   @endcode

 */
/*----------------------------------------------------------------------------*/
#define cpl_test_error(error) do {                                      \
        const int cpl_test_errno = errno;                               \
        const cpl_flops cpl_test_flops = cpl_test_get_flops();          \
        const double cpl_test_time = cpl_test_get_walltime();           \
        cpl_errorstate cpl_test_state = cpl_errorstate_get();           \
        cpl_test_error_macro(cpl_test_errno, cpl_test_time,             \
                             cpl_test_flops, cpl_test_state, error,     \
                             #error, cpl_func, __FILE__, __LINE__);     \
    } while (0)

/*----------------------------------------------------------------------------*/
/**
  @hideinitializer
  @ingroup cpl_test
  @brief  Test if two error expressions are equal and reset the CPL error code
  @param  first            The first value in the comparison
  @param  second           The second value in the comparison
  @see    cpl_test_error
  @note   If the two CPL error expressions are equal they will also be tested
          against the current CPL error code. After the test(s), the CPL
          errorstate is reset.

  Example of usage:
   @code

   cpl_error_code error = my_func(NULL);

   // my_func(NULL) is expected to return CPL_ERROR_NULL_INPUT
   // and to set the same error code
   cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);
   
   // The errorstate has been reset.

   error = my_func(p); // Successful call
   cpl_test_eq_error(error, CPL_ERROR_NONE);
   
   @endcode

 */
/*----------------------------------------------------------------------------*/
#define cpl_test_eq_error(first, second) do {                           \
        const int cpl_test_errno = errno;                               \
        const cpl_flops cpl_test_flops = cpl_test_get_flops();          \
        const double cpl_test_time = cpl_test_get_walltime();           \
        cpl_errorstate cpl_test_state = cpl_errorstate_get();           \
        cpl_test_eq_error_macro(cpl_test_errno, cpl_test_time,          \
                                cpl_test_flops, cpl_test_state, first,  \
                                #first, second, #second, cpl_func,      \
                                __FILE__, __LINE__);                    \
    } while (0)

/*----------------------------------------------------------------------------*/
/**
  @hideinitializer
  @ingroup cpl_test
  @brief Test if two integer expressions are equal
  @param first   The first value in the comparison, side-effects are allowed
  @param second  The second value in the comparison, side-effects are allowed

  @see cpl_test()
  @return void

  Example of usage:
   @code

   cpl_test_eq(computed, expected);
   
   @endcode

  For comparison of floating point values, see cpl_test_abs() and
  cpl_test_rel().

 */
/*----------------------------------------------------------------------------*/
#define cpl_test_eq(first, second) do {                                 \
        const int cpl_test_errno = errno;                               \
        const cpl_flops cpl_test_flops = cpl_test_get_flops();          \
        const double cpl_test_time = cpl_test_get_walltime();           \
        cpl_errorstate cpl_test_state = cpl_errorstate_get();           \
        cpl_test_eq_macro(cpl_test_errno, cpl_test_time, cpl_test_flops, \
                          cpl_test_state, first, #first,                \
                          second, #second, cpl_func, __FILE__, __LINE__); \
    } while (0)

/*----------------------------------------------------------------------------*/
/**
  @hideinitializer
  @ingroup cpl_test
  @brief Test if two integer expressions are not equal
  @param first   The first value in the comparison, side-effects are allowed
  @param second  The second value in the comparison, side-effects are allowed

  @see cpl_test_eq()
  @return void

  Example of usage:
   @code

   cpl_test_noneq(computed, wrong);
   
   @endcode

 */
/*----------------------------------------------------------------------------*/
#define cpl_test_noneq(first, second) do {                              \
        const int cpl_test_errno = errno;                               \
        const cpl_flops cpl_test_flops = cpl_test_get_flops();          \
        const double cpl_test_time = cpl_test_get_walltime();           \
        cpl_errorstate cpl_test_state = cpl_errorstate_get();           \
        cpl_test_noneq_macro(cpl_test_errno, cpl_test_time,             \
                             cpl_test_flops, cpl_test_state, first,     \
                             #first,  second, #second, cpl_func,        \
                             __FILE__, __LINE__);                       \
    } while (0)


/*----------------------------------------------------------------------------*/
/**
  @hideinitializer
  @ingroup cpl_test
  @brief  Test if two pointer expressions are equal
  @param  first    The first value in the comparison, side-effects are allowed
  @param  second   The second value in the comparison, side-effects are allowed
  @see cpl_test_eq()
  @return void

  Example of usage:
   @code

   cpl_test_eq_ptr(computed, expected);
   
   @endcode

 */
/*----------------------------------------------------------------------------*/
#define cpl_test_eq_ptr(first, second) do {                             \
        const int cpl_test_errno = errno;                               \
        const cpl_flops cpl_test_flops = cpl_test_get_flops();          \
        const double cpl_test_time = cpl_test_get_walltime();           \
        cpl_errorstate cpl_test_state = cpl_errorstate_get();           \
        cpl_test_eq_ptr_macro(cpl_test_errno, cpl_test_time,            \
                              cpl_test_flops, cpl_test_state, first,    \
                              #first, second, #second, cpl_func,        \
                              __FILE__, __LINE__);                      \
    } while (0)

/*----------------------------------------------------------------------------*/
/**
  @hideinitializer
  @ingroup cpl_test
  @brief  Test if two pointer expressions are not equal
  @param  first    The first value in the comparison, side-effects are allowed
  @param  second   The second value in the comparison, side-effects are allowed
  @see cpl_test_eq_ptr()
  @return void

  Example of usage:
   @code

   cpl_test_noneq_ptr(computed, wrong);
   
   @endcode

*/
/*----------------------------------------------------------------------------*/
#define cpl_test_noneq_ptr(first, second) do {                          \
        const int cpl_test_errno = errno;                               \
        const cpl_flops cpl_test_flops = cpl_test_get_flops();          \
        const double cpl_test_time = cpl_test_get_walltime();           \
        cpl_errorstate cpl_test_state = cpl_errorstate_get();           \
        cpl_test_noneq_ptr_macro(cpl_test_errno, cpl_test_time,         \
                                 cpl_test_flops, cpl_test_state, first, \
                                 #first,  second, #second, cpl_func,    \
                                 __FILE__, __LINE__);                   \
    } while (0)


/*----------------------------------------------------------------------------*/
/**
  @hideinitializer
  @ingroup cpl_test
  @brief  Test if two strings are equal
  @param  first      The first string or NULL of the comparison
  @param  second     The second string or NULL of the comparison
  @note One or two NULL pointer(s) is considered a failure.

  Example of usage:
   @code

   cpl_test_eq_string(computed, expected);
   
   @endcode

 */
/*----------------------------------------------------------------------------*/
#define cpl_test_eq_string(first, second) do {                          \
        const int cpl_test_errno = errno;                               \
        const cpl_flops cpl_test_flops = cpl_test_get_flops();          \
        const double cpl_test_time = cpl_test_get_walltime();           \
        cpl_errorstate cpl_test_state = cpl_errorstate_get();           \
        cpl_test_eq_string_macro(cpl_test_errno, cpl_test_time,         \
                                 cpl_test_flops, cpl_test_state, first, \
                                 #first, second, #second, cpl_func,     \
                                 __FILE__, __LINE__);                   \
    } while (0)


/*----------------------------------------------------------------------------*/
/**
  @hideinitializer
  @ingroup cpl_test
  @brief  Test if two strings are not equal
  @param  first      The first string or NULL of the comparison
  @param  second     The second string or NULL of the comparison
  @note One or two NULL pointer(s) is considered a failure.

  Example of usage:
   @code

   cpl_test_noneq_string(computed, expected);
   
   @endcode

 */
/*----------------------------------------------------------------------------*/
#define cpl_test_noneq_string(first, second) do {                       \
        const int cpl_test_errno = errno;                               \
        const cpl_flops cpl_test_flops = cpl_test_get_flops();          \
        const double cpl_test_time = cpl_test_get_walltime();           \
        cpl_errorstate cpl_test_state = cpl_errorstate_get();           \
        cpl_test_noneq_string_macro(cpl_test_errno, cpl_test_time,      \
                                    cpl_test_flops, cpl_test_state,     \
                                    first, #first, second, #second,     \
                                    cpl_func, __FILE__, __LINE__);      \
    } while (0)


/*----------------------------------------------------------------------------*/
/**
  @hideinitializer
  @ingroup cpl_test
  @brief  Test if a file is valid FITS using an external verification utility
  @param  fitsfile  The file to verify, NULL causes failure
  @note The external verification utility is specified with the environemt
        variable CPL_TEST_FITS, if is not set the test will pass on any
        non-NULL file.

  Example of usage:
   @code

   export CPL_TEST_FITS=/usr/local/bin/fitsverify

   @endcode

   @code

   cpl_test_fits(fitsfile);
   
   @endcode

 */
/*----------------------------------------------------------------------------*/
#define cpl_test_fits(fitsfile) do {                                    \
        const int cpl_test_errno = errno;                               \
        const cpl_flops cpl_test_flops = cpl_test_get_flops();          \
        const double cpl_test_time = cpl_test_get_walltime();           \
        cpl_errorstate cpl_test_state = cpl_errorstate_get();           \
        cpl_test_fits_macro(cpl_test_errno, cpl_test_time,              \
                            cpl_test_flops, cpl_test_state, fitsfile,   \
                            #fitsfile,  cpl_func, __FILE__, __LINE__);  \
    } while (0)

/*----------------------------------------------------------------------------*/
/**
  @hideinitializer
  @ingroup cpl_test
  @brief  Test if two CPL masks are equal
  @param  first      The first mask or NULL of the comparison
  @param  second     The second mask or NULL of the comparison
  @note One or two NULL pointer(s) is considered a failure.

  Example of usage:
   @code

   cpl_test_eq_mask(computed, expected);
   
   @endcode

 */
/*----------------------------------------------------------------------------*/
#define cpl_test_eq_mask(first, second) do {                            \
        const int cpl_test_errno = errno;                               \
        const cpl_flops cpl_test_flops = cpl_test_get_flops();          \
        const double cpl_test_time = cpl_test_get_walltime();           \
        cpl_errorstate cpl_test_state = cpl_errorstate_get();           \
        cpl_test_eq_mask_macro(cpl_test_errno, cpl_test_time,           \
                               cpl_test_flops, cpl_test_state, first,   \
                               #first,  second, #second, cpl_func,      \
                               __FILE__, __LINE__);                     \
    } while (0)

/*----------------------------------------------------------------------------*/
/**
   @hideinitializer
   @ingroup cpl_test
   @brief  Evaluate A <= B and increment an internal counter if it is not true
   @param  value     The number to test
   @param  tolerance The upper limit to compare against
   @return void
   @see cpl_test_init()
   @note  This macro should be used for unit tests

  Example of usage:
   @code

   cpl_test_leq(fabs(myfunc(&p)), DBL_EPSILON);
   cpl_test_nonnull(p);
   
   @endcode

 */
/*----------------------------------------------------------------------------*/
#define cpl_test_leq(value, tolerance) do {                             \
        const int cpl_test_errno = errno;                               \
        const cpl_flops cpl_test_flops = cpl_test_get_flops();          \
        const double cpl_test_time = cpl_test_get_walltime();           \
        cpl_errorstate cpl_test_state = cpl_errorstate_get();           \
        cpl_test_leq_macro(cpl_test_errno, cpl_test_time,               \
                           cpl_test_flops, cpl_test_state, value,       \
                           #value,  tolerance, #tolerance, cpl_func,    \
                           __FILE__, __LINE__);                         \
    } while (0)

/*----------------------------------------------------------------------------*/
/**
   @hideinitializer
   @ingroup cpl_test
   @brief  Evaluate A < B and increment an internal counter if it is not true
   @param  value     The number to test
   @param  tolerance The upper limit to compare against
   @return void
   @see cpl_test_init()
   @note  This macro should be used for unit tests

  Example of usage:
   @code

   cpl_test_lt(0.0, myfunc());
   
   @endcode

 */
/*----------------------------------------------------------------------------*/
#define cpl_test_lt(value, tolerance) do {                              \
        const int cpl_test_errno = errno;                               \
        const cpl_flops cpl_test_flops = cpl_test_get_flops();          \
        const double cpl_test_time = cpl_test_get_walltime();           \
        cpl_errorstate cpl_test_state = cpl_errorstate_get();           \
        cpl_test_lt_macro(cpl_test_errno, cpl_test_time,                \
                          cpl_test_flops, cpl_test_state, value,        \
                          #value,  tolerance, #tolerance, cpl_func,     \
                          __FILE__, __LINE__);                          \
    } while (0)

/*----------------------------------------------------------------------------*/
/**
  @hideinitializer
  @ingroup cpl_test
  @brief Test if two numerical expressions are within a given absolute tolerance
  @param first     The first value in the comparison, side-effects are allowed
  @param second    The second value in the comparison, side-effects are allowed
  @param tolerance A non-negative tolerance
  @note  If the tolerance is negative, the test will always fail
  @see   cpl_test()

  Example of usage:
   @code

   cpl_test_abs(computed, expected, DBL_EPSILON);
   
   @endcode

 */
/*----------------------------------------------------------------------------*/
#define cpl_test_abs(first, second, tolerance) do {                     \
        const int cpl_test_errno = errno;                               \
        const cpl_flops cpl_test_flops = cpl_test_get_flops();          \
        const double cpl_test_time = cpl_test_get_walltime();           \
        cpl_errorstate cpl_test_state = cpl_errorstate_get();           \
        cpl_test_abs_macro(cpl_test_errno, cpl_test_time,               \
                           cpl_test_flops, cpl_test_state, first,       \
                           #first, second, #second, tolerance,          \
                           #tolerance, cpl_func, __FILE__, __LINE__);   \
    } while (0)

/*----------------------------------------------------------------------------*/
/**
  @hideinitializer
  @ingroup cpl_test
  @brief Test if two complex expressions are within a given absolute tolerance
  @param first     The first value in the comparison, side-effects are allowed
  @param second    The second value in the comparison, side-effects are allowed
  @param tolerance A non-negative tolerance
  @note  If the tolerance is negative, the test will always fail
  @see   cpl_test()

  Example of usage:
   @code

   cpl_test_abs_complex(computed, expected, DBL_EPSILON);
   
   @endcode

 */
/*----------------------------------------------------------------------------*/
#define cpl_test_abs_complex(first, second, tolerance) do {             \
        const int cpl_test_errno = errno;                               \
        const cpl_flops cpl_test_flops = cpl_test_get_flops();          \
        const double cpl_test_time = cpl_test_get_walltime();           \
        cpl_errorstate cpl_test_state = cpl_errorstate_get();           \
        cpl_test_abs_complex_macro(cpl_test_errno, cpl_test_time,       \
                           cpl_test_flops, cpl_test_state, first,       \
                           #first, second, #second, tolerance,          \
                           #tolerance, cpl_func, __FILE__, __LINE__);   \
    } while (0)

/*----------------------------------------------------------------------------*/
/**
  @hideinitializer
  @ingroup cpl_test
  @brief  Test if two vectors are identical within a given (absolute) tolerance
  @param  first       The first vector in the comparison
  @param  second      The second vector of identical size in the comparison
  @param  tolerance   A non-negative tolerance
  @return void
  @see cpl_test_abs()
  @note The test will fail if one or both the vectors are NULL

 */
/*----------------------------------------------------------------------------*/
#define cpl_test_vector_abs(first, second, tolerance) do {              \
        const int cpl_test_errno = errno;                               \
        const cpl_flops cpl_test_flops = cpl_test_get_flops();          \
        const double cpl_test_time = cpl_test_get_walltime();           \
        cpl_errorstate cpl_test_state = cpl_errorstate_get();           \
        cpl_test_vector_abs_macro(cpl_test_errno, cpl_test_time,        \
                                  cpl_test_flops, cpl_test_state,       \
                                  first, #first,  second, #second,      \
                                  tolerance, #tolerance,  cpl_func,     \
                                  __FILE__, __LINE__);                  \
    } while (0)

/*----------------------------------------------------------------------------*/
/**
  @hideinitializer
  @ingroup cpl_test
  @brief  Test if two matrices are identical within a given (absolute) tolerance
  @param  first       The first matrix in the comparison
  @param  second      The second matrix of identical size in the comparison
  @param  tolerance   A non-negative tolerance
  @return void
  @see cpl_test_abs()
  @note The test will fail if one or both the matrices are NULL

 */
/*----------------------------------------------------------------------------*/
#define cpl_test_matrix_abs(first, second, tolerance) do {              \
        const int cpl_test_errno = errno;                               \
        const cpl_flops cpl_test_flops = cpl_test_get_flops();          \
        const double cpl_test_time = cpl_test_get_walltime();           \
        cpl_errorstate cpl_test_state = cpl_errorstate_get();           \
        cpl_test_matrix_abs_macro(cpl_test_errno, cpl_test_time,        \
                                  cpl_test_flops, cpl_test_state,       \
                                  first, #first,  second, #second,      \
                                  tolerance, #tolerance,  cpl_func,     \
                                  __FILE__, __LINE__);                  \
    } while (0)

/*----------------------------------------------------------------------------*/
/**
  @hideinitializer
  @ingroup cpl_test
  @brief  Test if two numerical arrays are identical within a given (absolute)
          tolerance
  @param  first       The first array in the comparison
  @param  second      The second array of identical size in the comparison
  @param  tolerance   A non-negative tolerance
  @return void
  @see cpl_test_abs()
  @note The test will fail if one or both the arrays are NULL or of a
        non-numerical type

 */
/*----------------------------------------------------------------------------*/
#define cpl_test_array_abs(first, second, tolerance) do {               \
        const int cpl_test_errno = errno;                               \
        const cpl_flops cpl_test_flops = cpl_test_get_flops();          \
        const double cpl_test_time = cpl_test_get_walltime();           \
        cpl_errorstate cpl_test_state = cpl_errorstate_get();           \
        cpl_test_array_abs_macro(cpl_test_errno, cpl_test_time,         \
                                 cpl_test_flops, cpl_test_state, first, \
                                 #first,  second, #second, tolerance,   \
                                 #tolerance,  cpl_func, __FILE__, __LINE__); \
    } while (0)


/*----------------------------------------------------------------------------*/
/**
  @hideinitializer
  @ingroup cpl_test
  @brief  Test if two images are identical within a given (absolute) tolerance
  @param  first       The first image in the comparison
  @param  second      The second image of identical size in the comparison
  @param  tolerance   A non-negative tolerance
  @return void
  @see cpl_test_abs()
  @note The test will fail if one or both the images are NULL

 */
/*----------------------------------------------------------------------------*/
#define cpl_test_image_abs(first, second, tolerance) do {               \
        const int cpl_test_errno = errno;                               \
        const cpl_flops cpl_test_flops = cpl_test_get_flops();          \
        const double cpl_test_time = cpl_test_get_walltime();           \
        cpl_errorstate cpl_test_state = cpl_errorstate_get();           \
        cpl_test_image_abs_macro(cpl_test_errno, cpl_test_time,         \
                                 cpl_test_flops, cpl_test_state, first, \
                                 #first,  second, #second, tolerance,   \
                                 #tolerance,  cpl_func, __FILE__, __LINE__); \
    } while (0)


/*----------------------------------------------------------------------------*/
/**
  @hideinitializer
  @ingroup cpl_test
  @brief  Test if two images are identical within a given (relative) tolerance
  @param  first       The first image in the comparison
  @param  second      The second image of identical size in the comparison
  @param  tolerance   A non-negative tolerance
  @return void
  @see cpl_test_rel()
  @note The test will fail if one or both the images are NULL

  For each pixel position the two values x, y must pass the test:
  |x - y| <= tol * min(|x|, |y|).
  This definition is chosen since it is commutative and meaningful also for
  zero-valued pixels.

 */
/*----------------------------------------------------------------------------*/
#define cpl_test_image_rel(first, second, tolerance) do {               \
        const int cpl_test_errno = errno;                               \
        const cpl_flops cpl_test_flops = cpl_test_get_flops();          \
        const double cpl_test_time = cpl_test_get_walltime();           \
        cpl_errorstate cpl_test_state = cpl_errorstate_get();           \
        cpl_test_image_rel_macro(cpl_test_errno, cpl_test_time,         \
                                 cpl_test_flops, cpl_test_state, first, \
                                 #first,  second, #second, tolerance,   \
                                 #tolerance,  cpl_func, __FILE__, __LINE__); \
    } while (0)


/*----------------------------------------------------------------------------*/
/**
  @hideinitializer
  @ingroup cpl_test
  @brief  Test if two imagelists are identical within a given (absolute)
          tolerance
  @param  first       The first imagelist in the comparison
  @param  second      The second imagelist of identical size in the comparison
  @param  tolerance   A non-negative tolerance
  @return void
  @see cpl_test_image_abs()
  @note The test will fail if one or both the imagelists are NULL

 */
/*----------------------------------------------------------------------------*/
#define cpl_test_imagelist_abs(first, second, tolerance) do {           \
        const int cpl_test_errno = errno;                               \
        const cpl_flops cpl_test_flops = cpl_test_get_flops();          \
        const double cpl_test_time = cpl_test_get_walltime();           \
        cpl_errorstate cpl_test_state = cpl_errorstate_get();           \
        cpl_test_imagelist_abs_macro(cpl_test_errno, cpl_test_time,     \
                                     cpl_test_flops, cpl_test_state,    \
                                     first, #first, second, #second,    \
                                     tolerance, #tolerance, cpl_func,   \
                                     __FILE__, __LINE__);               \
    } while (0)


/*----------------------------------------------------------------------------*/
/**
  @hideinitializer
  @ingroup cpl_test
  @brief  Test if two polynomials are identical within a given (absolute)
          tolerance
  @param  first       The first polynomial in the comparison
  @param  second      The second polynomial in the comparison
  @param  tolerance   A non-negative tolerance
  @return void
  @see cpl_test_abs(), cpl_polynomial_compare()
  @note The test will fail if one or both the polynomials are NULL

 */
/*----------------------------------------------------------------------------*/
#define cpl_test_polynomial_abs(first, second, tolerance) do {          \
        const int cpl_test_errno = errno;                               \
        const cpl_flops cpl_test_flops = cpl_test_get_flops();          \
        const double cpl_test_time = cpl_test_get_walltime();           \
        cpl_errorstate cpl_test_state = cpl_errorstate_get();           \
        cpl_test_polynomial_abs_macro(cpl_test_errno, cpl_test_time,    \
                                      cpl_test_flops, cpl_test_state,   \
                                      first, #first, second, #second,   \
                                      tolerance,  #tolerance, cpl_func, \
                                      __FILE__, __LINE__);              \
    } while (0)


/*----------------------------------------------------------------------------*/
/**
  @hideinitializer
  @ingroup cpl_test
  @brief Test if two numerical expressions are within a given relative tolerance
  @param first     The first value in the comparison, side-effects are allowed
  @param second    The second value in the comparison, side-effects are allowed
  @param tolerance A non-negative tolerance
  @note  If the tolerance is negative or if one but not both of the two values
         is zero, the test will always fail. If both values are zero, the test
         will succeed for any non-negative tolerance. The test is commutative
         in the two values.
  @see   cpl_test()

  The test is carried out by comparing the absolute value of the difference 
  abs (@em first - @em second)
  to the product of the tolerance and the minimum of the absolute value of
  the two values,
  tolerance * min(abs(@em first), abs(@em second))
  (The test is implemented like this to avoid division with a number that may
  be zero.

  Example of usage:
   @code

   cpl_test_rel(computed, expected, 0.001);
   
   @endcode

 */
/*----------------------------------------------------------------------------*/
#define cpl_test_rel(first, second, tolerance) do {                     \
        const int cpl_test_errno = errno;                               \
        const cpl_flops cpl_test_flops = cpl_test_get_flops();          \
        const double cpl_test_time = cpl_test_get_walltime();           \
        cpl_errorstate cpl_test_state = cpl_errorstate_get();           \
        cpl_test_rel_macro(cpl_test_errno, cpl_test_time,               \
                           cpl_test_flops, cpl_test_state, first,       \
                           #first, second, #second, tolerance,          \
                           #tolerance, cpl_func, __FILE__, __LINE__);   \
    } while (0)


/*----------------------------------------------------------------------------*/
/**
  @hideinitializer
  @ingroup cpl_test
  @brief  Test if the memory system is empty
  @see    cpl_memory_is_empty()
  @deprecated Called by cpl_test_end()
 */
/*----------------------------------------------------------------------------*/
#define cpl_test_memory_is_empty() do {                                 \
        const int cpl_test_errno = errno;                               \
        const cpl_flops cpl_test_flops = cpl_test_get_flops();          \
        const double cpl_test_time = cpl_test_get_walltime();           \
        cpl_errorstate cpl_test_state = cpl_errorstate_get();           \
        cpl_test_memory_is_empty_macro(cpl_test_errno, cpl_test_time,   \
                                       cpl_test_flops, cpl_test_state,  \
                                       cpl_func, __FILE__, __LINE__);   \
    } while (0)

/*----------------------------------------------------------------------------*/
/**
   @hideinitializer
   @ingroup cpl_test
   @brief  Evaluate an expression and terminate the process if it fails
   @param  bool   The (boolean) expression to evaluate, side-effects are allowed
   @note   A zero value of the expression is a failure, other values are not
   @return void
   @see cpl_test()
   @note  This macro should be used for unit tests that cannot continue after
          a failure.

  Example of usage:
    @code

    int main (void)
    {

        cpl_test_init(CPL_MSG_WARNING);

        cpl_test(myfunc(&p));
        cpl_test_assert(p != NULL);
        cpl_test(*p);

        return cpl_test_end(0);
    }
   
    @endcode
 */
/*----------------------------------------------------------------------------*/
#define cpl_test_assert(bool) do {                                      \
        const int cpl_test_errno = errno;                               \
        const cpl_flops cpl_test_flops = cpl_test_get_flops();          \
        const double cpl_test_time = cpl_test_get_walltime();           \
        cpl_errorstate cpl_test_state = cpl_errorstate_get();           \
        /* Evaluate bool just once */                                   \
        const cpl_boolean cpl_test_ok = (bool) ? CPL_TRUE : CPL_FALSE;  \
        cpl_test_macro(cpl_test_errno, cpl_test_time, cpl_test_flops,   \
                       cpl_test_state, cpl_test_ok, CPL_TRUE, #bool,    \
                       cpl_func, __FILE__, __LINE__);                   \
        if (cpl_test_ok == CPL_FALSE) exit(cpl_test_end(0));            \
    } while (0)


/* Deprecated. Do not use */
#define cpl_assert cpl_test_assert

#ifndef cpl_error_margin
/* Deprecated. Do not use */
#define cpl_error_margin 2.0
#endif

typedef unsigned long long int cpl_flops;

/*-----------------------------------------------------------------------------
                            Function prototypes
 -----------------------------------------------------------------------------*/

void cpl_test_init_macro(const char *, const char *,
                         cpl_msg_severity);
int cpl_test_end(cpl_size);
void cpl_test_macro(int, double, cpl_flops, cpl_errorstate, cpl_size,
                    cpl_boolean, const char *, const char *,
                    const char *, unsigned) CPL_ATTR_NONNULL;

void cpl_test_errorstate_macro(int, double, cpl_flops, cpl_errorstate,
                               cpl_errorstate, const char *, const char *,
                               const char *, unsigned) CPL_ATTR_NONNULL;

void cpl_test_error_macro(int, double, cpl_flops, cpl_errorstate,
                          cpl_error_code, const char *, const char *,
                          const char *, unsigned) CPL_ATTR_NONNULL;

void cpl_test_eq_error_macro(int, double, cpl_flops, cpl_errorstate,
                             cpl_error_code, const char *, cpl_error_code,
                             const char *, const char *,
                             const char *, unsigned) CPL_ATTR_NONNULL;

void cpl_test_null_macro(int, double, cpl_flops, cpl_errorstate, const void *,
                         const char *, const char *, const char *, unsigned)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(6, 7, 8)))
#endif
    ;
void cpl_test_nonnull_macro(int, double, cpl_flops, cpl_errorstate,
                            const void *, const char *, const char *,
                            const char *, unsigned)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(6, 7, 8)))
#endif
    ;
void cpl_test_eq_macro(int, double, cpl_flops, cpl_errorstate, cpl_size,
                       const char *, cpl_size, const char *, const char *,
                       const char *, unsigned) CPL_ATTR_NONNULL;

void cpl_test_noneq_macro(int, double, cpl_flops, cpl_errorstate, cpl_size,
                          const char *, cpl_size, const char *, const char *,
                          const char *, unsigned) CPL_ATTR_NONNULL;

void cpl_test_eq_ptr_macro(int, double, cpl_flops, cpl_errorstate, const void *,
                           const char *, const void *, const char *,
                           const char *, const char *, unsigned)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(6, 8, 9, 10)))
#endif
    ;

void cpl_test_noneq_ptr_macro(int, double, cpl_flops, cpl_errorstate,
                              const void *, const char *, const void *,
                              const char *, const char *, const char *,
                              unsigned)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(6, 8, 9, 10)))
#endif
    ;

void cpl_test_eq_string_macro(int, double, cpl_flops, cpl_errorstate,
                              const char *, const char *, const char *,
                              const char *, const char *, const char *,
                              unsigned)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(6, 8, 9, 10)))
#endif
    ;

void cpl_test_noneq_string_macro(int, double, cpl_flops, cpl_errorstate,
                                 const char *, const char *, const char *,
                                 const char *, const char *, const char *,
                                 unsigned)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(6, 8, 9, 10)))
#endif
    ;

void cpl_test_fits_macro(int, double, cpl_flops, cpl_errorstate, const char *,
                         const char *, const char *, const char *, unsigned)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(6, 7, 8)))
#endif
    ;

void cpl_test_eq_mask_macro(int, double, cpl_flops, cpl_errorstate,
                            const cpl_mask *, const char *, const cpl_mask *,
                            const char *, const char *, const char *, unsigned)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(6, 8, 9, 10)))
#endif
    ;

void cpl_test_leq_macro(int, double, cpl_flops, cpl_errorstate, double,
                        const char *, double, const char *, const char *,
                        const char *, unsigned) CPL_ATTR_NONNULL;

void cpl_test_lt_macro(int, double, cpl_flops, cpl_errorstate, double,
                       const char *, double, const char *, const char *,
                       const char *, unsigned) CPL_ATTR_NONNULL;

void cpl_test_abs_macro(int, double, cpl_flops, cpl_errorstate, double,
                        const char *, double, const char *, double,
                        const char *, const char *, const char *,
                        unsigned) CPL_ATTR_NONNULL;

void cpl_test_abs_complex_macro(int, double, cpl_flops, cpl_errorstate, 
                                _Complex double, const char *, 
                                _Complex double, const char *, double,
                                const char *, const char *, const char *,
                                unsigned) CPL_ATTR_NONNULL;

void cpl_test_rel_macro(int, double, cpl_flops, cpl_errorstate, double,
                        const char *, double, const char *, double,
                        const char *, const char *, const char *,
                        unsigned) CPL_ATTR_NONNULL;

void cpl_test_vector_abs_macro(int, double, cpl_flops, cpl_errorstate,
                               const cpl_vector *, const char *,
                               const cpl_vector *, const char *,
                               double, const char *, const char *,
                               const char *, unsigned)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(6, 8, 10, 11, 12)))
#endif
    ;

void cpl_test_matrix_abs_macro(int, double, cpl_flops, cpl_errorstate,
                               const cpl_matrix *, const char *,
                               const cpl_matrix *, const char *,
                               double, const char *, const char *,
                               const char *, unsigned)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(6, 8, 10, 11, 12)))
#endif
    ;

void cpl_test_array_abs_macro(int, double, cpl_flops, cpl_errorstate,
                              const cpl_array *, const char *,
                              const cpl_array *, const char *, double,
                              const char *, const char *, const char *,
                              unsigned)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(6, 8, 10, 11, 12)))
#endif
    ;

void cpl_test_image_abs_macro(int, double, cpl_flops, cpl_errorstate,
                              const cpl_image *, const char *,
                              const cpl_image *, const char *, double,
                              const char *, const char *, const char *,
                              unsigned)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(6, 8, 10, 11, 12)))
#endif
    ;

void cpl_test_image_rel_macro(int, double, cpl_flops, cpl_errorstate,
                              const cpl_image *, const char *,
                              const cpl_image *, const char *, double,
                              const char *, const char *, const char *,
                              unsigned)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(6, 8, 10, 11, 12)))
#endif
    ;

void cpl_test_imagelist_abs_macro(int, double, cpl_flops, cpl_errorstate,
                                  const cpl_imagelist *,
                                  const char *,
                                  const cpl_imagelist *,
                                  const char *,
                                  double, const char *,
                                  const char *, const char *,
                                  unsigned )
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(6, 8, 10, 11, 12)))
#endif
    ;

void cpl_test_polynomial_abs_macro(int, double, cpl_flops, cpl_errorstate,
                                   const cpl_polynomial *,
                                   const char *, const cpl_polynomial *,
                                   const char *, double, const char *,
                                   const char *, const char *, unsigned)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(6, 8, 10, 11, 12)))
#endif
    ;

void cpl_test_memory_is_empty_macro(int, double, cpl_flops, cpl_errorstate,
                                    const char *, const char *, unsigned)
    CPL_ATTR_NONNULL;

double cpl_test_get_cputime(void);
double cpl_test_get_walltime(void);

size_t cpl_test_get_bytes_vector(const cpl_vector *) CPL_ATTR_PURE;
size_t cpl_test_get_bytes_matrix(const cpl_matrix *) CPL_ATTR_PURE;
size_t cpl_test_get_bytes_image(const cpl_image *) CPL_ATTR_PURE;
size_t cpl_test_get_bytes_imagelist(const cpl_imagelist *) CPL_ATTR_PURE;

cpl_flops cpl_test_get_flops(void) CPL_ATTR_PURE;

cpl_size cpl_test_get_tested(void) CPL_ATTR_PURE;
cpl_size cpl_test_get_failed(void) CPL_ATTR_PURE;

CPL_END_DECLS

#endif
