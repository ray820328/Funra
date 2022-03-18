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

#define _XOPEN_SOURCE 500 // snprintf
#include <float.h>
#include <math.h>

#include <cpl_test.h>
#include <cpl_tools.h>
#include <cpl_memory.h>
#include <cpl_matrix.h>
#include <cpl_matrix_impl.h>
#include <cpl_error_impl.h>
#include <cpl_errorstate.h>

/*-----------------------------------------------------------------------------
                              Defines
 -----------------------------------------------------------------------------*/

/* 
 * Test for functions returning a generic pointer to data.
 *
 * r = variable to store returned pointer to data - expected non-NULL
 * f = function call
 * m = message
 */

#define test_data(r,f,m)                        \
    do {                                        \
        cpl_msg_info("", "%s", m);              \
        r = (f);                                \
        cpl_test_error(CPL_ERROR_NONE);         \
        cpl_test_nonnull(r);                    \
    } while (0)

/*
 * Test for functions returning 0 on success.
 *
 * f = function call
 * m = message
 */

#define test(f, m)                              \
    do {                                        \
        cpl_msg_info("", "%s", m);              \
        cpl_test_zero(f);                       \
    } while (0)


/*
 * Test for expected failure in functions returning 0 on success.
 *
 * f = function call
 * m = message
 */

#define test_failure(f,m)                       \
    do {                                        \
        cpl_msg_info("", "%s", m);              \
        cpl_test(f);                            \
    } while (0)

/*
 * Test for functions returning an expected integer value.
 *
 * e = expected value
 * f = function call
 * m = message
 */

#define test_ivalue(e,f,m)                      \
     do {                                       \
         cpl_msg_info("", "%s", m);             \
         cpl_test_eq(e, f);                     \
     } while (0)

/*
 * Test for functions returning an expected floating point value.
 *
 * e = expected value
 * t = tolerance on expected value
 * f = function call
 */
  
#define test_fvalue_(e,t,f)                     \
     cpl_test_abs(e, f, t)

/*
 * Test for functions returning an expected floating point value.
 *
 * e = expected value
 * t = tolerance on expected value
 * f = function call
 * m = message
 */
  
#define test_fvalue(e,t,f,m)                    \
     do {                                       \
         cpl_msg_info("", "%s", m);             \
         test_fvalue_(e,t,f);                   \
     } while (0)


/*-----------------------------------------------------------------------------
                        Private function prototypes
 -----------------------------------------------------------------------------*/

static void cpl_matrix_test_banded(int, int);

static cpl_error_code cpl_matrix_fill_illcond(cpl_matrix *, int);
static cpl_error_code cpl_matrix_fill_test(cpl_matrix *, cpl_size, cpl_size);
static double cpl_matrix_get_2norm_1(const cpl_matrix *);

static void cpl_matrix_product_transpose_bench(int, int);
static void cpl_matrix_solve_normal_bench(int, int);
static void cpl_matrix_product_bilinear_test(cpl_size, cpl_size, cpl_boolean);
static cpl_matrix * cpl_matrix_product_bilinear_brute(const cpl_matrix *,
                                                      const cpl_matrix *);
static cpl_error_code cpl_matrix_shift_brute(cpl_matrix *, cpl_size, cpl_size);

/*-----------------------------------------------------------------------------
                              main 
 -----------------------------------------------------------------------------*/

int main(void)
{

    const int   nreps = 1; /* This is currently enough to expose the trouble */
    int         nelem = 70;
    int         subnelem = 10;
    int         size;

    double     a2390[] = {2.125581475848425e-314, 3.952525166729972e-323,
                          -0.000000000000000e+00, -2.393462054246040e-162,
                          3.952525166729972e-323, 1.677069691870074e-311,
                          -0.000000000000000e+00, -6.726400100717507e-161,
                          -0.000000000000000e+00, -0.000000000000000e+00,
                          0.000000000000000e+00, 1.172701769974922e-171,
                          -2.393462054246040e-162, -6.726400100717507e-161,
                          1.172701769974922e-171, 6.189171527656329e+10};
    double     b2390[] = {-1.709710615916477e-161,
                          -4.805142076113790e-160,
                          8.376377601213216e-171,
                          4.962260007044318e+01};

    double     a3097[] = {22.3366126198544243663945962908,
                          -16.2991659056583451103961124318,
                          0.0747823788545301237906670621669,
                          0.0237870531554605392499102123338,

                          -16.2991659056583451103961124318,
                          17.2102745748494783128990093246,
                          -0.124262796786316020991591813072,
                          0.00881594777810015828301004603418,

                          0.0747823788545301237906670621669,
                          -0.124262796786316020991591813072,
                          0.00370155728670603376487258096006,
                          0.00719839528504809342962511564679,

                          0.0237870531554605392499102123338,
                          0.00881594777810015828301004603418,
                          0.00719839528504809342962511564679,
                          0.0220958028150731733418865587737};

    double     b3097[] = {1, 2, 3, 4};


    double     *dArray;
    double     *subdArray;
    double      value;
    cpl_size    i, j, k, l = 0; /* FIXME: What is l used for ? */
    cpl_size    r, c;

    cpl_matrix  *matrix;
    cpl_matrix  *copia;
    cpl_matrix  *copy;
    cpl_matrix  *product;

    cpl_matrix * xtrue; /* The true solution */
    cpl_matrix * xsolv; /* The solution computed by the cpl_matrix solver */
    FILE       * stream;
    const double * data;
    cpl_boolean  do_bench;
    cpl_boolean  did_fail;
    cpl_error_code code;
    const cpl_matrix * null; /* This one is expected to be NULL */


    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    do_bench = cpl_msg_get_level() <= CPL_MSG_INFO ? CPL_TRUE : CPL_FALSE;

  /*
  *  Testing begins here
  */

    stream = cpl_msg_get_level() > CPL_MSG_INFO
        ? fopen("/dev/null", "a") : stdout;

    cpl_test_nonnull( stream );

    cpl_matrix_test_banded(10, 1);
    cpl_matrix_test_banded(10, 2);
    if (do_bench) {
        cpl_matrix_test_banded(400, 10);
        cpl_matrix_test_banded(1000, 2);
    }
    for (i = 1; i <= 11; i++) {
        for (j = 1; j <= 11; j++) {
            cpl_matrix_product_bilinear_test(i, j, CPL_FALSE);
        }
    }

    if (do_bench) {
        cpl_matrix_product_bilinear_test(100, 500, CPL_TRUE);
        cpl_matrix_product_bilinear_test(500, 100, CPL_TRUE);
        cpl_matrix_product_bilinear_test(500, 500, CPL_TRUE);
    }

    cpl_matrix_product_transpose_bench(128, do_bench ? 100 : 1);

    /* Test cpl_matrix_get_stdev() (for DFS05126) */

    (void)cpl_matrix_get_stdev(NULL);

    cpl_test_error(CPL_ERROR_NULL_INPUT);

    matrix = cpl_matrix_wrap(4, 4, a3097);

    value = cpl_matrix_get_stdev(matrix);

    cpl_test_leq(0.0, value); /* FIXME: use better test...*/

    (void)cpl_matrix_unwrap(matrix);

    /* cpl_matrix_decomp_lu(): Verify detection of a matrix being singular
       due to its last pivot being zero. (Related to DFS 3097) */

    matrix = cpl_matrix_wrap(4, 4, a3097);
    cpl_test_nonnull(matrix);

    copia = cpl_matrix_wrap(4, 1, b3097);
    cpl_test_nonnull(copia);

    xsolv = cpl_matrix_solve(matrix, copia);

    if (cpl_error_get_code() == CPL_ERROR_SINGULAR_MATRIX) {

        cpl_test_error(CPL_ERROR_SINGULAR_MATRIX);
        cpl_test_null(xsolv);

        cpl_msg_info(cpl_func, "cpl_matrix_solve() classified a near-"
                     "singular matrix as singular. This is not a bug, "
                     "but it indicates that cpl_matrix_solve() is less "
                     "accurate on this computer. This loss of accuracy may "
                     "be caused by compiling CPL with optimization");
        cpl_test_abs(cpl_matrix_get_determinant(matrix), 0.0, 0.0);

    } else {

        cpl_test_nonnull(xsolv);

        test_fvalue(0.0, DBL_EPSILON, cpl_matrix_get_determinant(matrix),
                    "Compute determinant of an ill-conditioned matrix");

    }


    cpl_matrix_delete(xsolv);
    cpl_matrix_unwrap(copia);
    cpl_matrix_unwrap(matrix);


    /* Test for DFS 2390 */

    matrix = cpl_matrix_wrap(4, 4, a2390);
    cpl_test_nonnull(matrix);
    copia = cpl_matrix_wrap(4, 1, b2390);
    cpl_test_nonnull(copia);

    xsolv = cpl_matrix_solve(matrix, copia);

    if (cpl_error_get_code() == CPL_ERROR_SINGULAR_MATRIX) {
        cpl_test_null(xsolv);
        cpl_test_error(CPL_ERROR_SINGULAR_MATRIX);
        cpl_msg_warning(cpl_func, "cpl_matrix_solve() classified a near-"
                        "singular matrix as singular. This is not a bug, "
                        "but it indicates that cpl_matrix_solve() is less "
                        "accurate on this computer");

        xsolv = cpl_matrix_duplicate(copia);
    } else {
        cpl_matrix_dump(xsolv, stream);

        value = cpl_matrix_get_mean(xsolv);

        cpl_test_error(CPL_ERROR_NONE);
        cpl_test( value == value); /* Check for NaNs */


    }

    cpl_matrix_unwrap(copia);

    /* Test illegal call of cpl_matrix_solve_lu() */

    cpl_test_eq(cpl_matrix_solve_lu(matrix, xsolv, NULL),
                CPL_ERROR_DIVISION_BY_ZERO);
    cpl_test_error(CPL_ERROR_DIVISION_BY_ZERO);

    cpl_matrix_delete(xsolv);
    cpl_matrix_unwrap(matrix);

  test_data(matrix, cpl_matrix_new(7, 10), "Creating the test matrix... ");

  cpl_matrix_delete(matrix);

  r = 7;
  c = 10;
  test_data(matrix, cpl_matrix_new(r, c), "Creating another test matrix... ");

  cpl_matrix_fill(matrix, 13.565656565);

  for (i = 0; i < r; i++) {
      for (j = 0; j < c; j++) {
          cpl_msg_info(cpl_func, 
                  "Check element %" CPL_SIZE_FORMAT ", %"
                  CPL_SIZE_FORMAT " of constant matrix... ", i, j);
          test_fvalue_(13.565656565, .000001, cpl_matrix_get(matrix, i, j));
      }
  }

  cpl_matrix_delete(matrix);

/*
  test_data(matrix, cpl_matrix_new_identity(3),
                    "Creating identity matrix... ");
*/

  matrix = cpl_matrix_new(3, 3);
  for (i = 0; i < 3; i++)
      cpl_matrix_set(matrix, i, i, 1);

  test_ivalue(1, cpl_matrix_is_identity(matrix, -1.), 
                    "Checking if matrix is identity... ");

  cpl_matrix_delete(matrix);

  dArray = cpl_malloc((size_t)nelem * sizeof(double));

  value = 9.00;
  for (i = 0; i < nelem; i++) {
      dArray[i] = value;
      value += .01;
  }

  subdArray = cpl_malloc((size_t)subnelem * sizeof(double));

  value = 1.00; 
  for (i = 0; i < subnelem; i++) {
      subdArray[i] = value;
      value += .01;
  }

/*
  test_data(matrix, cpl_matrix_new_diagonal(dArray, 5),
                    "Creating diagonal matrix... ");
*/

  matrix = cpl_matrix_new(5, 5);
  for (i = 0; i < 5; i++)
      cpl_matrix_set(matrix, i, i, dArray[i]);

  test_ivalue(1, cpl_matrix_is_diagonal(matrix, -1.), 
                    "Checking if matrix is diagonal... ");

  cpl_matrix_delete(matrix);

  test_data(matrix, cpl_matrix_wrap(7, 10, dArray), 
                    "Creating the final test matrix... ");

  test_fvalue(9.57, .001, cpl_matrix_get(matrix, 5, 7), 
                    "Reading a matrix element... ");

/*
  test_failure(cpl_matrix_set(matrix, 10, 10, 1.00), 
                    "Writing a matrix element out of range... ");
*/

  test(cpl_matrix_set(matrix, 6, 5, 1.00), 
                    "Writing a matrix element... ");

  test_fvalue(1.00, .001, cpl_matrix_get(matrix, 6, 5), 
                    "Reading just written matrix element... ");

  test_data(copia, cpl_matrix_duplicate(matrix), "Copy a matrix... ");

  test(cpl_matrix_subtract(copia, matrix), "Subtract original from copy... ");

  test_ivalue(1, cpl_matrix_is_zero(copia, -1), "Check if result is zero... ");

  cpl_matrix_delete(copia);

  test_data(copia, cpl_matrix_extract(matrix, 2, 3, 1, 1, 4, 3), 
                    "Extracting submatrix... ");

  test_fvalue(9.44, .001, cpl_matrix_get(copia, 2, 1), 
                    "Reading a submatrix element... ");

  cpl_matrix_delete(copia);

  test_data(copia, cpl_matrix_extract(matrix, 4, 4, 1, 1, 1000, 1000),
                    "Extracting submatrix (beyond upper boundaries)... ");

  test_fvalue(9.55, .001, cpl_matrix_get(copia, 1, 1), 
                    "Reading new submatrix element... ");

  cpl_matrix_delete(copia);

  test_data(copia, cpl_matrix_extract(matrix, 0, 0, 3, 3, 1000, 1000),
                    "Extracting submatrix (step 3, beyond boundaries)... ");

  test_fvalue(9.33, .001, cpl_matrix_get(copia, 1, 1), 
                    "Reading new submatrix element... ");

  cpl_matrix_delete(copia);

  /* Test error handling for invalid input: */
  null = cpl_matrix_extract_row(NULL, 1);
  cpl_test_error(CPL_ERROR_NULL_INPUT);
  cpl_test_null(null);
  null = cpl_matrix_extract_row(matrix, 9999999);
  cpl_test_error(CPL_ERROR_ACCESS_OUT_OF_RANGE);
  cpl_test_null(null);
  null = cpl_matrix_extract_row(matrix, -1);
  cpl_test_error(CPL_ERROR_ACCESS_OUT_OF_RANGE);
  cpl_test_null(null);
  null = cpl_matrix_extract_column(NULL, 1);
  cpl_test_error(CPL_ERROR_NULL_INPUT);
  cpl_test_null(null);
  null = cpl_matrix_extract_column(matrix, 9999999);
  cpl_test_error(CPL_ERROR_ACCESS_OUT_OF_RANGE);
  cpl_test_null(null);
  null = cpl_matrix_extract_column(matrix, -1);
  cpl_test_error(CPL_ERROR_ACCESS_OUT_OF_RANGE);
  cpl_test_null(null);
  test_data(copia, cpl_matrix_extract_row(matrix, 2), "Copy one row... ");

  c = cpl_matrix_get_ncol(matrix);

  for (i = 0; i < c; i++) {
      cpl_msg_info(cpl_func, 
              "Check element %" CPL_SIZE_FORMAT " of copied row... ", i);
      test_fvalue_(cpl_matrix_get(matrix, 2, i), 0.00001,
                   cpl_matrix_get(copia, 0, i));
  }

  cpl_matrix_delete(copia);

#ifdef CPL_MATRIX_TEST_EXTRA
  test(cpl_matrix_copy_row_remove(matrix, subdArray, 1), "Write one row... ");

  for (i = 0; i < c; i++) {
      cpl_msg_info(cpl_func, "Check element %" CPL_SIZE_FORMAT
              " of written row... ", i);
      test_fvalue(subdArray[i], 0.00001, cpl_matrix_get(matrix, 1, i));
  }

  test_data(copia, cpl_matrix_extract_column(matrix, 3), "Copy one column... ");

  r = cpl_matrix_get_nrow(matrix);

  for (i = 0; i < r; i++) {
      cpl_msg_info(cpl_func, "Check element %" CPL_SIZE_FORMAT
              " of copied column... ", i);
      test_fvalue(cpl_matrix_get(matrix, i, 3), 0.00001,
                  cpl_matrix_get(copia, i, 0));
  }

  cpl_matrix_delete(copia);

  test(cpl_matrix_copy_column_remove(matrix, subdArray, 1),
       "Write one column... ");

  for (i = 0; i < r; i++) {
      cpl_msg_info(cpl_func, "Check element %" CPL_SIZE_FORMAT
              " of written column... ", i);
      test_fvalue(subdArray[i], 0.00001, cpl_matrix_get(matrix, i, 1));
  }

#endif

  value = 9.00;
  for (i = 0; i < nelem; i++) {
      dArray[i] = value;
      value += .01;
  }

  test_data(copia, cpl_matrix_extract_diagonal(matrix, 0), 
                    "Copy main diagonal... ");

  r = cpl_matrix_get_nrow(matrix);

  for (i = 0; i < r; i++) {
      cpl_msg_info(cpl_func, "Check element %" CPL_SIZE_FORMAT
              " of copied diagonal... ", i);
      test_fvalue_(cpl_matrix_get(matrix, i, i), 0.00001,
                   cpl_matrix_get(copia, i, 0));
  }

  cpl_matrix_delete(copia);

  test_data(copia, cpl_matrix_extract_diagonal(matrix, 2), 
                    "Copy third diagonal... ");

  for (i = 0; i < r; i++) {
      cpl_msg_info(cpl_func, "Check element %" CPL_SIZE_FORMAT
              " of copied third diagonal... ", i);
      test_fvalue_(cpl_matrix_get(matrix, i , i + 2), 0.00001,
                   cpl_matrix_get(copia, i, 0));
  }

  cpl_matrix_delete(copia);

#ifdef CPL_MATRIX_TEST_EXTRA

  test(cpl_matrix_copy_diagonal_remove(matrix, subdArray, 0), 
                    "Write main diagonal... ");

  for (i = 0; i < r; i++) {
      cpl_msg_info(cpl_func, 
              "Check element %" CPL_SIZE_FORMAT " of written "
              "diagonal... ", i);;
      test_fvalue(subdArray[i], 0.00001, cpl_matrix_get(matrix, i, i));
  }

  test(cpl_matrix_copy_diagonal_remove(matrix, subdArray, 2), 
                    "Write third diagonal... ");

  for (i = 0; i < r; i++) {
      cpl_msg_info(cpl_func, 
              "Check element %" CPL_SIZE_FORMAT " of written "
              "diagonal... ", i);
      test_fvalue(subdArray[i], 0.00001, cpl_matrix_get(matrix, i, i + 2));
  }

#endif

  value = 9.00;
  for (i = 0; i < nelem; i++) {
      dArray[i] = value;
      value += .01;
  }

  cpl_matrix_unwrap(matrix);

  test_data(matrix, cpl_matrix_wrap(10, 7, dArray), 
                    "A new test matrix... ");

  test_data(copia, cpl_matrix_extract_diagonal(matrix, 0), 
                    "Copy main diagonal (vertical)... ");

  c = cpl_matrix_get_ncol(matrix);

  for (i = 0; i < c; i++) {
      cpl_msg_info(cpl_func, 
              "Check element %" CPL_SIZE_FORMAT " of copied diagonal "
              "(vertical)... ", i);
      test_fvalue_(cpl_matrix_get(matrix, i, i), 0.00001,
                   cpl_matrix_get(copia, 0, i));
  }

  cpl_matrix_delete(copia);

  test_data(copia, cpl_matrix_extract_diagonal(matrix, 1), 
                    "Copy second diagonal (vertical)... ");

  for (i = 0; i < c; i++) {
      cpl_msg_info(cpl_func,  
              "Check element %" CPL_SIZE_FORMAT " of copied second diagonal "
              "(vertical)... ", i);
      test_fvalue_(cpl_matrix_get(matrix, i + 1, i), 0.00001,
                   cpl_matrix_get(copia, 0, i));
  }

  cpl_matrix_delete(copia);

#ifdef CPL_MATRIX_TEST_EXTRA

  test(cpl_matrix_copy_diagonal_remove(matrix, subdArray, 0),
                    "Write main diagonal (vertical)... ");

  for (i = 0; i < c; i++) {
      cpl_msg_info(cpl_func,  
              "Check element %" CPL_SIZE_FORMAT " of written diagonal "
              "(vertical)... ", i);
      test_fvalue(subdArray[i], 0.00001, cpl_matrix_get(matrix, i, i));
  }

  test(cpl_matrix_copy_diagonal_remove(matrix, subdArray, 1),
                    "Write second diagonal (vertical)... ");

  for (i = 0; i < c; i++) {
      cpl_msg_info(cpl_func,  
              "Check element %" CPL_SIZE_FORMAT " of written second diagonal "
              "(vertical)... ", i);
      test_fvalue(subdArray[i], 0.00001, cpl_matrix_get(matrix, i + 1, i));
  }

#endif

  cpl_free(subdArray);

  test(cpl_matrix_fill(matrix, 3.33), "Write same value to entire matrix... ");

  c = cpl_matrix_get_ncol(matrix);
  r = c * cpl_matrix_get_nrow(matrix);

  for (i = 0; i < r; i++) {
      cpl_msg_info(cpl_func, 
              "Check element %" CPL_SIZE_FORMAT ", %" CPL_SIZE_FORMAT
              " of matrix... ", i / c, i % c);
      test_fvalue_(3.33, 0.00001, cpl_matrix_get(matrix, i / c, i % c));
  }

  test(cpl_matrix_fill_row(matrix, 2.0, 4), "Write 2 to row 4...");

  for (i = 0; i < c; i++) {
      cpl_msg_info(cpl_func, 
              "Check element %" CPL_SIZE_FORMAT " of constant row... ", i);
      test_fvalue_(2.0, 0.00001, cpl_matrix_get(matrix, 4, i));
  }

  test(cpl_matrix_fill_column(matrix, 9.99, 2), "Write 9.99 to column 2...");

  r = cpl_matrix_get_nrow(matrix);

  for (i = 0; i < r; i++) {
      cpl_msg_info(cpl_func, "Check element %" CPL_SIZE_FORMAT
              " of constant column... ", i);
      test_fvalue_(9.99, 0.00001, cpl_matrix_get(matrix, i, 2));
  }

  test(cpl_matrix_fill_diagonal(matrix, 1.11, 0), 
       "Write 1.11 to main diagonal...");

  for (i = 0; i < c; i++) {
      cpl_msg_info(cpl_func, "Check element %" CPL_SIZE_FORMAT
              " of constant main diagonal... ", i);
      test_fvalue_(1.11, 0.00001, cpl_matrix_get(matrix, i, i));
  }

  test(cpl_matrix_fill_diagonal(matrix, 1.10, -1),
                    "Write 1.11 to second diagonal...");

  for (i = 0; i < c; i++) {
      cpl_msg_info(cpl_func, "Check element %" CPL_SIZE_FORMAT
              " of constant second diagonal... ", i);
      test_fvalue_(1.1, 0.00001, cpl_matrix_get(matrix, i + 1, i));
  }

  value = 9.00;
  for (i = 0; i < nelem; i++) {
      dArray[i] = value;
      value += .01;
  }

  cpl_matrix_unwrap(matrix);

  test_data(matrix, cpl_matrix_wrap(7, 10, dArray),
                    "One more test matrix... ");

  test_data(copia, cpl_matrix_extract(matrix, 0, 0, 1, 1, 4, 4),
                    "Copying square submatrix... ");

  r = cpl_matrix_get_nrow(copia);
  c = cpl_matrix_get_ncol(copia);

  for (i = 0; i < r; i++) {
      for (j = 0; j < c; j++) {
          cpl_msg_info(cpl_func, 
                  "Check element %" CPL_SIZE_FORMAT ", %"
                  CPL_SIZE_FORMAT " of submatrix... ", i, j);
          test_fvalue_(cpl_matrix_get(matrix, i, j), 0.00001,
                       cpl_matrix_get(copia, i, j));
      }
  }

  test(cpl_matrix_add_scalar(copia, -8.), 
                    "Subtracting constant from submatrix... ");

  for (i = 0; i < r; i++) {
      for (j = 0; j < c; j++) {
          cpl_msg_info(cpl_func,  
                  "Check element %" CPL_SIZE_FORMAT ", %" CPL_SIZE_FORMAT
                  " of modified submatrix... ", i, j);
          test_fvalue_(cpl_matrix_get(matrix, i, j) - 8., 0.00001,
                       cpl_matrix_get(copia, i, j));
      }
  }

  test(cpl_matrix_copy(matrix, copia, 2, 2), 
                    "Writing submatrix into matrix...");

  for (i = 0; i < r; i++) {
      for (j = 0; j < c; j++) {
          cpl_msg_info(cpl_func,
                  "Check element %" CPL_SIZE_FORMAT ", %" CPL_SIZE_FORMAT
                  " of modified matrix... ", i + 2, j + 2);
          test_fvalue_(cpl_matrix_get(copia, i, j), 0.00001,
                       cpl_matrix_get(matrix, i + 2, j + 2));
      }
  }

  cpl_matrix_delete(copia);

  test(cpl_matrix_fill_window(matrix, 0., 1, 1, 4, 3),
                    "Writing same value into submatrix... ");

  for (i = 1; i < 4; i++) {
      for (j = 1; j < 3; j++) {
          cpl_msg_info(cpl_func,
                  "Check element %" CPL_SIZE_FORMAT ", %" CPL_SIZE_FORMAT
                  " of constant submatrix... ", i, j);
          test_fvalue_(0.0, 0.00001, cpl_matrix_get(matrix, i, j));
      }
  }

  value = 9.00;
  for (i = 0; i < nelem; i++) {
      dArray[i] = value;
      value += .01;
  }

  copia = cpl_matrix_duplicate(matrix);
  r = cpl_matrix_get_nrow(copia);
  c = cpl_matrix_get_ncol(copia);

  test(cpl_matrix_fill_window(matrix, 0., 2, 2, 10, 10),
                    "Writing same value into submatrix (beyond borders)... ");

  for (i = 0; i < r; i++) {
      for (j = 0; j < c; j++) {
          cpl_msg_info(cpl_func,
                  "Check element %" CPL_SIZE_FORMAT ", %" CPL_SIZE_FORMAT
                  " of overwritten matrix... ", i, j);
          if (i > 1 && j > 1) {
              test_fvalue_(0.0, 0.00001, 
                           cpl_matrix_get(matrix, i, j));
          }
          else {
              test_fvalue_(cpl_matrix_get(copia, i, j), 0.00001, 
                           cpl_matrix_get(matrix, i, j));
          }
      }
  }

  cpl_matrix_delete(copia);

  value = 9.00;
  for (i = 0; i < nelem; i++) {
      dArray[i] = value;
      value += .01;
  }

  copia = cpl_matrix_duplicate(matrix);
  copy  = cpl_matrix_duplicate(matrix);
  r = cpl_matrix_get_nrow(copia);
  c = cpl_matrix_get_ncol(copia);

  data = cpl_matrix_get_data_const(matrix);
  test(cpl_matrix_shift(matrix, 1, 0), "Shift matrix elements by +1,0... ");
  code = cpl_matrix_shift_brute(copy, 1, 0);
  cpl_test_eq_error(code, CPL_ERROR_NONE);
  cpl_test_matrix_abs(matrix, copy, 0.0);

  for (i = 0; i < r; i++) {
      for (j = 0; j < c; j++) {
          cpl_msg_info(cpl_func,
                  "Check element %" CPL_SIZE_FORMAT ", %" CPL_SIZE_FORMAT
                  " of first shifted matrix... ", i, j);
          if (i > r - 2) {
              test_fvalue_(cpl_matrix_get(copia, i, j), 0.00001,
                           cpl_matrix_get(matrix, i + 1 - r, j));
          }
          else {
              test_fvalue_(cpl_matrix_get(copia, i, j), 0.00001,
                           cpl_matrix_get(matrix, i + 1, j));
          }
      }
  }

  test(cpl_matrix_shift(matrix, -1, 1), "Shift matrix elements by -1,+1... ");
  code = cpl_matrix_shift_brute(copy, -1, 1);
  cpl_test_eq_error(code, CPL_ERROR_NONE);
  cpl_test_matrix_abs(matrix, copy, 0.0);

  for (i = 0; i < r; i++) {
      for (j = 0; j < c; j++) {
          cpl_msg_info(cpl_func,
                  "Check element %" CPL_SIZE_FORMAT ", %" CPL_SIZE_FORMAT
                  " of second shifted matrix... ", i, j);
          if (j > c - 2) {
              test_fvalue_(cpl_matrix_get(copia, i, j), 0.00001,
                           cpl_matrix_get(matrix, i, j + 1 - c));
          }
          else {
              test_fvalue_(cpl_matrix_get(copia, i, j), 0.00001,
                           cpl_matrix_get(matrix, i, j + 1));
          }
      }
  }

  test(cpl_matrix_shift(matrix, 4, 3), "Shift matrix elements by +4,+3... ");
  code = cpl_matrix_shift_brute(copy, 4, 3);
  cpl_test_eq_error(code, CPL_ERROR_NONE);
  cpl_test_matrix_abs(matrix, copy, 0.0);

  for (i = 0; i < r; i++) {
      for (j = 0; j < c; j++) {
          cpl_msg_info(cpl_func,
                       "Check element %" CPL_SIZE_FORMAT ", %" CPL_SIZE_FORMAT
                       " of third shifted matrix... ", i, j);
          if (j > c - 5 && i > r - 5) {
              test_fvalue_(cpl_matrix_get(copia, i, j), 0.00001,
                           cpl_matrix_get(matrix, i + 4 - r, j + 4 - c));
          }
          else if (j > c - 5) {
              test_fvalue_(cpl_matrix_get(copia, i, j), 0.00001,
                           cpl_matrix_get(matrix, i + 4, j + 4 - c));
          }
          else if (i > r - 5) {
              test_fvalue_(cpl_matrix_get(copia, i, j), 0.00001,
                           cpl_matrix_get(matrix, i + 4 - r, j + 4));
          }
          else {
              test_fvalue_(cpl_matrix_get(copia, i, j), 0.00001,
                           cpl_matrix_get(matrix, i + 4, j + 4));
          }
      }
  }

  test(cpl_matrix_shift(matrix, -8, -8), "Shift matrix elements by -8,-8... ");
  code = cpl_matrix_shift_brute(copy, -8, -8);
  cpl_test_eq_error(code, CPL_ERROR_NONE);
  cpl_test_matrix_abs(matrix, copy, 0.0);

  for (i = 0; i < r; i++) {
      for (j = 0; j < c; j++) {
          cpl_msg_info(cpl_func,
                       "Check element %" CPL_SIZE_FORMAT ", %" CPL_SIZE_FORMAT
                       " of fourth shifted matrix... ", i, j);
          if (j < 4 && i < 4) {
              test_fvalue_(cpl_matrix_get(copia, i, j), 0.00001,
                           cpl_matrix_get(matrix, i - 4 + r, j - 4 + c));
          }
          else if (j < 4) {
              test_fvalue_(cpl_matrix_get(copia, i, j), 0.00001,
                           cpl_matrix_get(matrix, i - 4, j - 4 + c));
          }
          else if (i < 4) {
              test_fvalue_(cpl_matrix_get(copia, i, j), 0.00001,
                           cpl_matrix_get(matrix, i - 4 + r, j - 4));
          }
          else {
              test_fvalue_(cpl_matrix_get(copia, i, j), 0.00001,
                           cpl_matrix_get(matrix, i - 4, j - 4));
          }
      }
  }

  /* Shift matrix back to original */
  code = cpl_matrix_shift(matrix, 4, 4);
  cpl_test_eq_error(code, CPL_ERROR_NONE);

  code = cpl_matrix_shift_brute(copy, 4, 4);
  cpl_test_eq_error(code, CPL_ERROR_NONE);
  cpl_test_matrix_abs(matrix, copy, 0.0);

  /* No information was lost since the shifts are cyclic */
  cpl_test_matrix_abs(matrix, copia, 0.0);

  /* The pointer to the elements is also unchanged */
  cpl_test_eq_ptr(data, cpl_matrix_get_data_const(matrix));

  for (i = - 2 * cpl_matrix_get_nrow(matrix);
       i < 2 + 2 * cpl_matrix_get_nrow(matrix); i++) {
      for (j = - 2 * cpl_matrix_get_ncol(matrix);
           j < 2 + 2 * cpl_matrix_get_ncol(matrix); j++) {

          code = cpl_matrix_shift(matrix, i, 0);

          code = cpl_matrix_shift_brute(copy, i, 0);
          cpl_test_eq_error(code, CPL_ERROR_NONE);
          cpl_test_matrix_abs(matrix, copy, 0.0);


          code = cpl_matrix_shift(matrix, 0, j);

          code = cpl_matrix_shift_brute(copy, 0, j);
          cpl_test_eq_error(code, CPL_ERROR_NONE);
          cpl_test_matrix_abs(matrix, copy, 0.0);


          code = cpl_matrix_shift(matrix, -i, -j);

          code = cpl_matrix_shift_brute(copy, -i, -j);
          cpl_test_eq_error(code, CPL_ERROR_NONE);
          cpl_test_matrix_abs(matrix, copy, 0.0);

      }
  }

  cpl_matrix_delete(copia);
  cpl_matrix_delete(copy);

  code = cpl_matrix_shift(NULL, 1, 1);
  cpl_test_eq_error(code, CPL_ERROR_NULL_INPUT);

  copia = cpl_matrix_duplicate(matrix);

  test(cpl_matrix_threshold_small(matrix, 9.505), "Chop matrix... ");

  for (i = 0; i < r; i++) {
      for (j = 0; j < c; j++) {
          cpl_msg_info(cpl_func,
                  "Check element %" CPL_SIZE_FORMAT ", %" CPL_SIZE_FORMAT
                  " of chopped matrix... ", i, j);
          if (cpl_matrix_get(copia, i, j) < 9.505) {
              test_fvalue_(0.0, 0.00001,
                           cpl_matrix_get(matrix, i, j));
          }
          else {
              test_fvalue_(cpl_matrix_get(copia, i, j), 0.00001,
                           cpl_matrix_get(matrix, i, j));
          }
      }
  }

  cpl_matrix_delete(copia);
  cpl_matrix_delete(matrix);

  dArray = cpl_malloc((size_t)nelem * sizeof(double));

  value = 9.00;
  for (i = 0; i < nelem; i++) {
      dArray[i] = value;
      value += .01;
  }

  test_data(matrix, cpl_matrix_wrap(7, 10, dArray),
            "Creating one more test matrix... ");

  copia = cpl_matrix_duplicate(matrix);

  test(cpl_matrix_swap_rows(matrix, 1, 3), "Swap rows 1 and 3... ");

  for (j = 0; j < c; j++) {
      cpl_msg_info(cpl_func, "Check column %" CPL_SIZE_FORMAT
                   " of row swapped matrix... ", j);
      test_fvalue_(cpl_matrix_get(copia, 1, j), 0.00001,
                   cpl_matrix_get(matrix, 3, j));
      test_fvalue_(cpl_matrix_get(copia, 3, j), 0.00001,
                   cpl_matrix_get(matrix, 1, j));
  }

  value = 9.00;
  for (i = 0; i < nelem; i++) {
      dArray[i] = value;
      value += .01;
  }

  test(cpl_matrix_swap_columns(matrix, 1, 3), "Swap columns 1 and 3... ");

  for (i = 0; i < r; i++) {
      cpl_msg_info(cpl_func, "Check row %" CPL_SIZE_FORMAT
                   " of column swapped matrix... ", i);
      test_fvalue_(cpl_matrix_get(copia, i, 1), 0.00001,
                   cpl_matrix_get(matrix, i, 3));
      test_fvalue_(cpl_matrix_get(copia, i, 1), 0.00001,
                   cpl_matrix_get(matrix, i, 3));
  }

  cpl_matrix_delete(copia);

  value = 9.00;
  for (i = 0; i < nelem; i++) {
      dArray[i] = value;
      value += .01;
  }

  test_data(copia, cpl_matrix_new(5, 5), "Creating 5x5 square matrix... ");

  test(cpl_matrix_copy(copia, matrix, 0, 0),
                    "Writing upper left 5x5 of matrix into copy...");

  test(cpl_matrix_swap_rowcolumn(copia, 1),
                    "Swapping row 1 with column 1... ");

  for (i = 0; i < 5; i++) {
      cpl_msg_info(cpl_func, 
                   "Check row/column element %" CPL_SIZE_FORMAT
                   " of swapped matrix... ", i);
      test_fvalue_(cpl_matrix_get(matrix, i, 1), 0.00001,
                   cpl_matrix_get(copia, 1, i));
      test_fvalue_(cpl_matrix_get(matrix, i, 1), 0.00001,
                   cpl_matrix_get(copia, 1, i));
  }

  cpl_matrix_delete(copia);

  copia = cpl_matrix_duplicate(matrix);

  test(cpl_matrix_flip_rows(matrix), "Flip matrix upside down... ");

  for (i = 0; i < r; i++) {
      for (j = 0; j < c; j++) {
          cpl_msg_info(cpl_func,
                  "Check element %" CPL_SIZE_FORMAT ", %" CPL_SIZE_FORMAT
                  " of row-flipped matrix... ", i, j);
          test_fvalue_(cpl_matrix_get(copia, i, j), 0.00001,
                       cpl_matrix_get(matrix, r - i - 1, j));
      }
  }

  test(cpl_matrix_flip_columns(matrix), "Flip matrix left right... ");

  for (i = 0; i < r; i++) {
      for (j = 0; j < c; j++) {
          cpl_msg_info(cpl_func,
                  "Check element %" CPL_SIZE_FORMAT ", %" CPL_SIZE_FORMAT
                  " of row-flipped matrix... ", i, j);
          test_fvalue_(cpl_matrix_get(copia, i, j), 0.00001,
                       cpl_matrix_get(matrix, r - i - 1, c - j - 1));
      }
  }

  cpl_matrix_delete(copia);

  value = 9.00;
  for (i = 0; i < nelem; i++) {
      dArray[i] = value;
      value += .01;
  }

  test_data(copia, cpl_matrix_transpose_create(matrix),
            "Compute transpose... ");

  for (i = 0; i < r; i++) {
      for (j = 0; j < c; j++) {
          cpl_msg_info(cpl_func,
                  "Check element %" CPL_SIZE_FORMAT ", %" CPL_SIZE_FORMAT
                  " of transposed matrix... ", i, j);
          test_fvalue_(cpl_matrix_get(matrix, i, j), 0.00001,
                       cpl_matrix_get(copia, j, i));
      }
  }

  test(cpl_matrix_sort_rows(copia, 1), "Sort matrix rows... ");

  test(cpl_matrix_flip_columns(copia), "Flip matrix left right (again)... ");
  test(cpl_matrix_sort_columns(copia, 1), "Sort matrix columns... ");

  cpl_matrix_delete(copia);

  copia = cpl_matrix_duplicate(matrix);

  test(cpl_matrix_erase_rows(matrix, 2, 3), "Delete rows 2 to 4... ");

  k = 0;
  for (i = 0; i < r; i++) {
      if (i > 1 && i < 5)
          break;
      for (j = 0; j < c; j++) {
          cpl_msg_info(cpl_func,
                  "Check element %" CPL_SIZE_FORMAT ", %" CPL_SIZE_FORMAT
                  " of row-deleted matrix... ", k, j);
          test_fvalue_(cpl_matrix_get(copia, i, j), 0.00001,
                       cpl_matrix_get(matrix, k, j));
      }
      k++;
  }
  
  test(cpl_matrix_erase_columns(matrix, 2, 3), "Delete columns 2 to 4... ");

  k = 0;
  for (i = 0; i < r; i++) {
      if (i > 1 && i < 5)
          break;
      for (j = 0; j < c; j++) {
          if (j > 1 && j < 5)
              break;
          cpl_msg_info(cpl_func,
                  "Check element %" CPL_SIZE_FORMAT ", %" CPL_SIZE_FORMAT
                  " of row-deleted matrix... ", k, j);
          test_fvalue_(cpl_matrix_get(copia, i, j), 0.00001,
                       cpl_matrix_get(matrix, k, j));
          l++;
      }
      k++;
  }

  cpl_matrix_delete(copia);
  cpl_matrix_delete(matrix);

  dArray = cpl_malloc((size_t)nelem * sizeof(double));

  value = 9.00;
  for (i = 0; i < nelem; i++) {
      dArray[i] = value;
      value += .01;
  }

  test_data(matrix, cpl_matrix_wrap(7, 10, dArray), 
                    "Creating the steenth test matrix... ");

  copia = cpl_matrix_duplicate(matrix);

  test(cpl_matrix_resize(matrix, -2, -2, -2, -2), 
                         "Cut a frame around matrix... ");

  for (i = 2; i < r - 2; i++) {
      for (j = 2; j < c - 2; j++) {
          cpl_msg_info(cpl_func,
                  "Check element %" CPL_SIZE_FORMAT ", %" CPL_SIZE_FORMAT
                  " of cropped matrix... ", i - 2, j - 2);
          test_fvalue_(cpl_matrix_get(matrix, i - 2, j - 2), 0.00001,
                       cpl_matrix_get(copia, i, j));
      }
  }

  test(cpl_matrix_resize(matrix, 2, 2, 2, 2), "Add a frame around matrix... ");

  for (i = 0; i < r; i++) {
      for (j = 0; j < c; j++) {
          cpl_msg_info(cpl_func,
                  "Check element %" CPL_SIZE_FORMAT ", %" CPL_SIZE_FORMAT
                  " of expanded matrix... ", i, j);
          if (i > 1 && i < r - 2 && j > 1 && j < c - 2) {
              test_fvalue_(cpl_matrix_get(copia, i, j), 0.00001,
                           cpl_matrix_get(matrix, i, j));
          }
          else {
              test_fvalue_(0.0, 0.00001,
                           cpl_matrix_get(matrix, i, j));
          }
      }
  }

  test(cpl_matrix_resize(matrix, -2, 2, 2, -2), 
                         "Reframe a matrix... ");

  cpl_matrix_delete(copia);
  cpl_matrix_delete(matrix);

  dArray = cpl_malloc((size_t)nelem * sizeof(double));

  value = 9.00;
  for (i = 0; i < nelem; i++) {
      dArray[i] = value;
      value += .01;
  }

  test_data(matrix, cpl_matrix_wrap(7, 10, dArray),
                    "Creating matrix for merging test... ");

  test_data(copia, cpl_matrix_extract(matrix, 0, 0, 1, 1, 7, 1),
                    "Creating second matrix for merging test... ");

  product = cpl_matrix_duplicate(copia);

  test(cpl_matrix_append(copia, matrix, 0), "Horizontal merging... ");

  for (i = 0; i < r; i++) {
      for (j = 0; j < c; j++) {
          cpl_msg_info(cpl_func,
                  "Check element %" CPL_SIZE_FORMAT ", %" CPL_SIZE_FORMAT
                  " of horizontally merged matrix... ", 
                  i, j + 1);
          test_fvalue_(cpl_matrix_get(matrix, i, j), 0.00001,
                       cpl_matrix_get(copia, i, j + 1));
      }
  }

  for (i = 0; i < r; i++) {
      cpl_msg_info(cpl_func,
              "Check element %" CPL_SIZE_FORMAT ", %d"
              " of horizontally merged matrix... ",
              i, 0);
      test_fvalue_(cpl_matrix_get(product, i, 0), 0.00001,
                   cpl_matrix_get(copia, i, 0));
  }

  cpl_matrix_delete(product);
  cpl_matrix_delete(copia);

  test_data(copia, cpl_matrix_extract(matrix, 0, 0, 1, 1, 4, 10),
                    "Creating third matrix for merging test... ");

  product = cpl_matrix_duplicate(copia);

  test(cpl_matrix_append(copia, matrix, 1), "Vertical merging... ");

  for (i = 0; i < 4; i++) {
      for (j = 0; j < c; j++) {
          cpl_msg_info(cpl_func,
                  "Check element %" CPL_SIZE_FORMAT ", %" CPL_SIZE_FORMAT
                  " of vertically merged matrix... ",
                  i, j);
          test_fvalue_(cpl_matrix_get(product, i, j), 0.00001,
                       cpl_matrix_get(copia, i, j));
      }
  }

  for (i = 0; i < r; i++) {
      for (j = 0; j < c; j++) {
          cpl_msg_info(cpl_func,
                  "Check element %" CPL_SIZE_FORMAT ", %" CPL_SIZE_FORMAT
                  " of vertically merged matrix... ",
                  i + 4, j);
          test_fvalue_(cpl_matrix_get(matrix, i, j), 0.00001,
                       cpl_matrix_get(copia, i + 4, j));
      }
  }

  cpl_matrix_delete(product);
  cpl_matrix_delete(copia);

  test_data(copia, cpl_matrix_duplicate(matrix), 
            "Create matrix for add test... ");

  test(cpl_matrix_add(copia, matrix), "Adding two matrices... ");

  for (i = 0; i < r; i++) {
      for (j = 0; j < c; j++) {
          cpl_msg_info(cpl_func, 
                  "Check element %" CPL_SIZE_FORMAT ", %"
                  CPL_SIZE_FORMAT " of sum matrix... ", i, j);
          test_fvalue_(cpl_matrix_get(matrix, i, j) * 2, 0.00001,
                       cpl_matrix_get(copia, i, j));
      }
  }

  test(cpl_matrix_subtract(copia, matrix), "Subtracting two matrices... ");

  for (i = 0; i < r; i++) {
      for (j = 0; j < c; j++) {
          cpl_msg_info(cpl_func, 
                  "Check element %" CPL_SIZE_FORMAT ", %"
                  CPL_SIZE_FORMAT " of diff matrix... ", i, j);
          test_fvalue_(cpl_matrix_get(matrix, i, j), 0.00001,
                       cpl_matrix_get(copia, i, j));
      }
  }

  test(cpl_matrix_multiply(copia, matrix), "Multiply two matrices... ");

  for (i = 0; i < r; i++) {
      for (j = 0; j < c; j++) {
          cpl_msg_info(cpl_func, 
                  "Check element %" CPL_SIZE_FORMAT ", %"
                  CPL_SIZE_FORMAT " of mult matrix... ", i, j);
          test_fvalue_(cpl_matrix_get(matrix, i, j) * 
                       cpl_matrix_get(matrix, i, j), 0.00001,
                       cpl_matrix_get(copia, i, j));
      }
  }

  test(cpl_matrix_divide(copia, matrix), "Divide two matrices... ");

  for (i = 0; i < r; i++) {
      for (j = 0; j < c; j++) {
          cpl_msg_info(cpl_func, 
                  "Check element %" CPL_SIZE_FORMAT ", %"
                  CPL_SIZE_FORMAT " of ratio matrix... ", i, j);
          test_fvalue_(cpl_matrix_get(matrix, i, j), 0.00001,
                       cpl_matrix_get(copia, i, j));
      }
  }

  test(cpl_matrix_multiply_scalar(copia, 1./2.), "Divide a matrix by 2... ");

  for (i = 0; i < r; i++) {
      for (j = 0; j < c; j++) {
          cpl_msg_info(cpl_func, 
                  "Check element %" CPL_SIZE_FORMAT ", %"
                  CPL_SIZE_FORMAT " of half matrix... ", i, j);
          test_fvalue_(cpl_matrix_get(matrix, i, j) / 2, 0.00001,
                       cpl_matrix_get(copia, i, j));
      }
  }

  cpl_matrix_delete(matrix);
  cpl_matrix_delete(copia);

  test_data(matrix, cpl_matrix_new(3, 4), 
                     "Creating first matrix for product... ");

  test_data(copia, cpl_matrix_new(4, 5), 
                     "Creating second matrix for product... ");

  value = 0.0;
  for (i = 0, k = 0; i < 3; i++) {
      for (j = 0; j < 4; j++) {
          k++;
          cpl_msg_info(cpl_func, 
                  "Writing to element %" CPL_SIZE_FORMAT
                  " of the first matrix... ", k);
          cpl_test_zero(cpl_matrix_set(matrix, i, j, value));
          value++;
      }
  }

  value = -10.0;
  for (i = 0, k = 0; i < 4; i++) {
      for (j = 0; j < 5; j++) {
          k++;
          cpl_msg_info(cpl_func, 
                  "Writing to element %" CPL_SIZE_FORMAT
                  " of the second matrix... ", k);
          cpl_test_zero(cpl_matrix_set(copia, i, j, value));
          value++;
      }
  }

  test_data(product, cpl_matrix_product_create(matrix, copia),
            "Matrix product... ");

  test_fvalue(10., 0.00001, cpl_matrix_get(product, 0, 0), 
              "Check element 0, 0 of product... ");
  test_fvalue(16., 0.00001, cpl_matrix_get(product, 0, 1), 
              "Check element 0, 1 of product... ");
  test_fvalue(22., 0.00001, cpl_matrix_get(product, 0, 2), 
              "Check element 0, 2 of product... ");
  test_fvalue(28., 0.00001, cpl_matrix_get(product, 0, 3), 
              "Check element 0, 3 of product... ");
  test_fvalue(34., 0.00001, cpl_matrix_get(product, 0, 4), 
              "Check element 0, 4 of product... ");
  test_fvalue(-30., 0.00001, cpl_matrix_get(product, 1, 0), 
              "Check element 1, 0 of product... ");
  test_fvalue(-8., 0.00001, cpl_matrix_get(product, 1, 1), 
              "Check element 1, 1 of product... ");
  test_fvalue(14., 0.00001, cpl_matrix_get(product, 1, 2), 
              "Check element 1, 2 of product... ");
  test_fvalue(36., 0.00001, cpl_matrix_get(product, 1, 3), 
              "Check element 1, 3 of product... ");
  test_fvalue(58., 0.00001, cpl_matrix_get(product, 1, 4), 
              "Check element 1, 4 of product... ");
  test_fvalue(-70., 0.00001, cpl_matrix_get(product, 2, 0), 
              "Check element 2, 0 of product... ");
  test_fvalue(-32., 0.00001, cpl_matrix_get(product, 2, 1), 
              "Check element 2, 1 of product... ");
  test_fvalue(6., 0.00001, cpl_matrix_get(product, 2, 2), 
              "Check element 2, 2 of product... ");
  test_fvalue(44., 0.00001, cpl_matrix_get(product, 2, 3), 
              "Check element 2, 3 of product... ");
  test_fvalue(82., 0.00001, cpl_matrix_get(product, 2, 4), 
              "Check element 2, 4 of product... ");

  cpl_matrix_delete(copia);
  cpl_matrix_delete(product);
  cpl_matrix_delete(matrix);

  test_data(matrix, cpl_matrix_new(3, 3), "Creating coefficient matrix... ");

  cpl_matrix_set(matrix, 0, 0, 1);
  cpl_matrix_set(matrix, 0, 1, 1);
  cpl_matrix_set(matrix, 0, 2, 1);
  cpl_matrix_set(matrix, 1, 0, 1);
  cpl_matrix_set(matrix, 1, 1, -2);
  cpl_matrix_set(matrix, 1, 2, 2);
  cpl_matrix_set(matrix, 2, 0, 0);
  cpl_matrix_set(matrix, 2, 1, 1);
  cpl_matrix_set(matrix, 2, 2, -2);

  test(cpl_matrix_get_minpos(matrix, &r, &c), "Find min pos 2... ");

  test_ivalue(1, r, "Check min row pos 2...");
  test_ivalue(1, c, "Check min col pos 2...");

  test_fvalue(-2.0, 0.0001, cpl_matrix_get_min(matrix),
              "Check min value 2... ");

  test(cpl_matrix_get_maxpos(matrix, &r, &c), "Find max pos 2... ");

  test_ivalue(1, r, "Check max row pos 2...");
  test_ivalue(2, c, "Check max col pos 2...");

  test_fvalue(2.0, 0.0001, cpl_matrix_get_max(matrix), "Check max value 2... ");

  test_data(copia, cpl_matrix_new(3, 1), "Creating nonhomo matrix... ");

  cpl_matrix_set(copia, 0, 0, 0);
  cpl_matrix_set(copia, 1, 0, 4);
  cpl_matrix_set(copia, 2, 0, 2);

  cpl_matrix_dump(matrix, stream);
  test_fvalue(5.0, DBL_EPSILON, cpl_matrix_get_determinant(matrix),
              "Compute determinant of a regular matrix");
  cpl_test_error(CPL_ERROR_NONE);

  test_data(product, cpl_matrix_solve(matrix, copia), 
                         "Solving a linear system... ");

  test_fvalue(4., 0.0001, cpl_matrix_get(product, 0, 0),
              "Check linear system solution, x = 4... ");
  test_fvalue(-2., 0.0001, cpl_matrix_get(product, 1, 0),
              "Check linear system solution, y = -2... ");
  test_fvalue(-2., 0.0001, cpl_matrix_get(product, 2, 0),
              "Check linear system solution, z = -2... ");

/* */
  cpl_matrix_delete(product);
  /* cpl_matrix_dump(matrix); */
  /* cpl_matrix_dump(copia); */

  test_data(product, cpl_matrix_solve_normal(matrix, copia),
            "Least squares... ");

  /* cpl_matrix_dump(product); */
/* */

  cpl_matrix_delete(copia);
  cpl_matrix_delete(product);
  cpl_matrix_delete(matrix);

  test_data(matrix, cpl_matrix_new(1, 1), "Creating 1x1 matrix to invert... ");
  cpl_matrix_set(matrix, 0, 0, 2);

  test_fvalue(2.0, 0.0, cpl_matrix_get_determinant(matrix),
              "Compute determinant of a 1x1-matrix");
  cpl_test_error(CPL_ERROR_NONE);

  test_data(copia, cpl_matrix_invert_create(matrix), "Invert 1x1 matrix... ");

  test_data(product, cpl_matrix_product_create(matrix, copia), 
                        "Product by 1x1 inverse... ");

  test_ivalue(1, cpl_matrix_is_identity(product, -1.), 
                    "Checking if 1x1 product is identity... ");


  code = cpl_matrix_product(product, matrix, copia);
  cpl_test_eq_error(code, CPL_ERROR_NONE);

  k = cpl_matrix_is_identity(product, -1.);
  cpl_test_error(CPL_ERROR_NONE);
  cpl_test_eq(1, k);

  cpl_matrix_delete(matrix);
  cpl_matrix_delete(product);
  cpl_matrix_delete(copia);

  test_data(matrix, cpl_matrix_new(2, 2), "Creating matrix to invert... ");

  cpl_matrix_set(matrix, 0, 0, 1);
  cpl_matrix_set(matrix, 0, 1, 1);
  cpl_matrix_set(matrix, 1, 0, -1);
  cpl_matrix_set(matrix, 1, 1, 2);

  cpl_matrix_dump(matrix, stream);
  test_data(copia, cpl_matrix_invert_create(matrix), "Invert matrix... ");
  cpl_matrix_dump(copia, stream);

  test_data(product, cpl_matrix_product_create(matrix, copia), 
                        "Product by inverse... ");
  cpl_matrix_dump(product, stream);

  test_ivalue(1, cpl_matrix_is_identity(product, -1.), 
                    "Checking if product is identity... ");

  /* Try to solve a 2 X 2 singular system */
  cpl_test_zero( cpl_matrix_set_size(matrix, 2, 2) );
  cpl_test_zero( cpl_matrix_set_size(copia, 2, 1)  );

  cpl_test_zero( cpl_matrix_fill(matrix, 0.0) );

  for (i = 0; i < 2; i++) {
      cpl_test_zero( cpl_matrix_set(matrix, i, 1, 1.0) );
  }

  cpl_test_zero( cpl_matrix_fill(copia, 1.0) );

  cpl_matrix_dump(matrix, stream);
  cpl_matrix_dump(copia, stream);

  cpl_msg_info("", "Try to solve a 2 X 2 singular system");
  null =  cpl_matrix_solve(matrix, copia);
  cpl_test_error(CPL_ERROR_SINGULAR_MATRIX);
  cpl_test_null(null);

  cpl_matrix_delete(product);
  xtrue = cpl_matrix_new(1,1);

  /* Compute the determinant */
  test_fvalue(0.0, 0.0, cpl_matrix_get_determinant(matrix),
              "Compute determinant of a singular matrix");
  cpl_test_error(CPL_ERROR_NONE);


  /* Compute the determinant */
  /* This time with an existing error code */
  do {
      const cpl_error_code edummy = cpl_error_set(cpl_func, CPL_ERROR_EOL);
      cpl_errorstate prev_state = cpl_errorstate_get();
      const double dvalzero = cpl_matrix_get_determinant(matrix);
      const cpl_boolean is_equal = cpl_errorstate_is_equal(prev_state);

      cpl_test_eq_error(CPL_ERROR_EOL, edummy);
      cpl_test(is_equal);

      test_fvalue(0.0, 0.0, dvalzero,
                  "Compute determinant of a singular matrix");
  } while (0);

  /* Try to solve increasingly ill-conditioned systems */
  cpl_msg_info("", "Try to solve increasingly large systems Ax=b, "
               "with A(i,j) = 1/(2*n-(1+i+j)) and x(j) = 1");

  for (size = subnelem; 1 < nreps * nelem; size++) {
      cpl_matrix * p2;
      double error, residual;
      double x_min, x_max;
      cpl_boolean is_done;

      /* The true solution consists of all ones */
      cpl_test_zero(cpl_matrix_set_size(xtrue, size, 1));
      cpl_test_zero(cpl_matrix_fill(xtrue, 1.0));

      cpl_test_zero(cpl_matrix_fill_illcond(matrix, size));

      product = cpl_matrix_product_create(matrix, xtrue);
      cpl_test_nonnull(product);

      xsolv = cpl_matrix_solve(matrix, product);
      cpl_test_nonnull(xsolv);

      x_min = cpl_matrix_get_min(xsolv);
      x_max = cpl_matrix_get_max(xsolv);

      is_done = (x_min < 0.5 || x_max >= 2.0) ? CPL_TRUE : CPL_FALSE;

      /* Find the residual */
      p2 = cpl_matrix_product_create(matrix, xsolv);
      cpl_test_zero(cpl_matrix_subtract(p2, product));
      
      residual = cpl_matrix_get_2norm_1(p2);
      cpl_matrix_delete(p2);

      /* Find the error on the solution */
      cpl_test_zero(cpl_matrix_subtract(xsolv, xtrue));

      error = cpl_matrix_get_2norm_1(xsolv);

      cpl_msg_info("","2-norm of residual with ill-conditioned %d x %d "
                   "[DBL_EPSILON]: %g", size, size, residual/DBL_EPSILON);

      cpl_msg_info("","2-norm of error on solution with ill-conditioned "
                   "%d x %d : %g", size, size, error);

      cpl_matrix_dump(xsolv, stream);


      cpl_matrix_delete(xsolv);
      cpl_matrix_delete(product);

      if (is_done) {
          cpl_msg_info("", "Stopping at n=%d, because the most significant bit "
                       "of at least one element in the solution is wrong, "
                       "x_min=%g, x_max=%g", size, x_min, x_max);
          break;
      }
  }

  /* Badness squared: Try to solve the normal equations... */
  cpl_msg_info("", "Try to solve increasingly large systems A^TAx=A^Tb, "
               "with A(i,j) = 1/(2*n-(1+i+j)) and x(j) = 1");

  k = 0;
  did_fail = CPL_FALSE;
  for (size = 1; size < nreps * nelem; size++) {
      cpl_matrix * p2;
      double error, residual;
      double x_min, x_max;
      cpl_boolean is_done;

      /* The true solution consists of all ones */
      cpl_test_zero(cpl_matrix_set_size(xtrue, size, 1));
      cpl_test_zero(cpl_matrix_fill(xtrue, 1.0));

      cpl_test_zero(cpl_matrix_fill_illcond(matrix, size));

      product = cpl_matrix_product_create(matrix, xtrue);
      cpl_test_nonnull(product);

      xsolv = cpl_matrix_solve_normal(matrix, product);
      if (cpl_error_get_code()) {
          cpl_test_error(CPL_ERROR_SINGULAR_MATRIX);
          cpl_test_null(xsolv);
          did_fail = CPL_TRUE;
          cpl_matrix_delete(product);
          break;
      }
      cpl_test_nonnull(xsolv);

      x_min = cpl_matrix_get_min(xsolv);
      x_max = cpl_matrix_get_max(xsolv);

      is_done = (x_min < 0.5 || x_max >= 2.0) ? CPL_TRUE : CPL_FALSE;

      /* Find the residual */
      p2 = cpl_matrix_product_create(matrix, xsolv);
      cpl_test_zero(cpl_matrix_subtract(p2, product));
      
      residual = cpl_matrix_get_2norm_1(p2);
      cpl_matrix_delete(p2);

      /* Find the error on the solution */
      cpl_test_zero(cpl_matrix_subtract(xsolv, xtrue));

      error = cpl_matrix_get_2norm_1(xsolv);

      cpl_msg_info("","2-norm of residual with ill-conditioned %d x %d (normal)"
                   " [DBL_EPSILON]: %g", size, size, residual/DBL_EPSILON);

      cpl_msg_info("","2-norm of error on solution with ill-conditioned %d x %d"
                   " (normal): %g", size, size, error);

      cpl_matrix_dump(xsolv, stream);

      cpl_matrix_delete(xsolv);
      cpl_matrix_delete(product);

      if (is_done) {
          cpl_msg_info("", "Stopping at n=%d, because the most significant bit "
                       "of at least one element in the solution is wrong, "
                       "x_min=%g, x_max=%g", size, x_min, x_max);
          break;
      }
      k++;
  }

  if (did_fail) {
      /* Solving stopped prematurely. Normally, we should stop when the system
         is near-singular and so ill-conditioned that the solution has an
         element where not even the most significant bit is correct. For this
         final iteration we will allow the solver to alternatively fail (due a
         singular matrix).
         This should add support for Debian sbuild on mips64el */
      cpl_test_leq(6, k);
  }

  cpl_msg_info("", "Compute the determinant of increasingly large "
               "matrices, with A(i,j) = 1/(1+abs(i-j))");

  for (size = 1; size < nelem*nreps; size++) {

      cpl_test_zero(cpl_matrix_fill_test(matrix, size, size));

      value = cpl_matrix_get_determinant(matrix);

      cpl_msg_debug("", "Determinant of %d by %d test-matrix: %g", size, size,
                    value);

  }

  if (cpl_error_get_code()) {
      cpl_msg_info("", "Stopping at n=%d, because the determinant was "
                   "truncated to zero", size);
      cpl_test_error(CPL_ERROR_UNSPECIFIED);
  } else {
      cpl_msg_info("", "Stopping at n=%d, with a determinant of %g", size,
                   value);
  }

  cpl_msg_info("", "Compute the determinant of increasingly large ill-"
               "conditioned matrices, with A(i,j) = 1/(2*n-(1+i+j))");

  for (size = 1; !cpl_error_get_code(); size++) {

      cpl_test_zero(cpl_matrix_fill_illcond(matrix, size));

      value = cpl_matrix_get_determinant(matrix);

      cpl_msg_debug("", "Determinant of %d by %d ill-conditioned-matrix: %g",
                    size, size, value);

  }

  cpl_test_error(CPL_ERROR_UNSPECIFIED);
  cpl_msg_info("", "Stopping at n=%d, because the determinant was truncated to "
               "zero", size);

  cpl_matrix_solve_normal_bench(do_bench ? 400 : 40, do_bench ? 10 : 1);

  cpl_matrix_delete(xtrue);

  cpl_matrix_delete(matrix);
  cpl_matrix_delete(copia);

  if (stream != stdout) cpl_test_zero( fclose(stream) );

  return cpl_test_end(0);

}

/**@}*/

/*-----------------------------------------------------------------------------
                              Function codes
 -----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Generate an ill-conditioned N X N matrix 
  @param    self   The matrix to be modified (including resizing).
  @param    size   The positive size 
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error
  @see http://csep1.phy.ornl.gov/bf/node17.html

  The condition number of this matrix increases with n, starting with cond=1,
  for size = 1.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if the input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if size is negative or zero
 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_matrix_fill_illcond(cpl_matrix * self, int size)
{

    int i,j;

    cpl_ensure_code(self,     CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(size > 0, CPL_ERROR_ILLEGAL_INPUT);

    cpl_ensure_code(!cpl_matrix_set_size(self, size, size),
                    cpl_error_get_code());

    for (i = 0; i<size; i++) {
        for (j = 0; j<size; j++) {
            /*
               The 'usual' definition of this increasingly ill-conditioned
               matrix is
               A(i,j) = 1/(1+i+j)
               - but to expose direct solvers without pivoting, we use
               A(i,j) = 1/(2*size - (1+i+j))
            */
            const double value = 1.0/(double)(2*size-(i+j+1));
            cpl_ensure_code(!cpl_matrix_set(self, i, j, value),
                            cpl_error_get_code());
        }
    }

    return CPL_ERROR_NONE;

}



/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Generate a test N X N matrix 
  @param  self  The matrix to be modified (including resizing).
  @param  nr    The number of matrix rows
  @param  nc    The number of matrix columns
  @return CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if the input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if size is negative or zero
 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_matrix_fill_test(cpl_matrix * self, cpl_size nr,
                                           cpl_size nc)
{

    cpl_size i,j;

    cpl_ensure_code(self,   CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(nr > 0, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(nc > 0, CPL_ERROR_ILLEGAL_INPUT);

    cpl_ensure_code(!cpl_matrix_set_size(self, nr, nc), cpl_error_get_code());

    for (i = 0; i < nr; i++) {
        for (j = 0; j < nc; j++) {
            const double value = 1.0/(double)(1+labs(i-j));
            cpl_ensure_code(!cpl_matrix_set(self, i, j, value),
                            cpl_error_get_code());
        }
    }

    return CPL_ERROR_NONE;

}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Compute the 2-norm of matrix of size N x 1
  @param    self   The matrix
  @return   The 2-norm, or a negative number on error

  FIXME: To be general this function should compute the 2-norm for each column
  of A - and be called cpl_matrix_get_2norm_column().

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if the input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if size is negative or zero
 */
/*----------------------------------------------------------------------------*/
static double cpl_matrix_get_2norm_1(const cpl_matrix * self)
{
    double norm = 0;
    int i;

    cpl_ensure(self,     CPL_ERROR_NULL_INPUT, -1.0);
    cpl_ensure(cpl_matrix_get_ncol(self) == 1, CPL_ERROR_ILLEGAL_INPUT, -2.0);

    for (i = 0; i < cpl_matrix_get_nrow(self); i++) {
        const double value = cpl_matrix_get(self, i, 0);
        norm += value * value;
    }

    return sqrt(norm);

}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief   Benchmark the CPL function
  @param nsize  The size of the CPL object
  @param n      The number of repeats
  @return   void
 */
/*----------------------------------------------------------------------------*/
static void cpl_matrix_solve_normal_bench(int nsize, int n)
{

    cpl_matrix * xtrue  = cpl_matrix_new(1,1); /* The true solution */
    cpl_matrix * matrix = cpl_matrix_new(1,1); /* The system to solve */
    cpl_matrix * product; /* The Right Hand Side */
    size_t bytes;
    cpl_flops flops0;
    double secs;
    int i;

    /* The true solution consists of all ones */
    cpl_test_zero(cpl_matrix_set_size(xtrue, nsize, 1));
    cpl_test_zero(cpl_matrix_fill(xtrue, 1.0));

    cpl_test_zero(cpl_matrix_fill_test(matrix, nsize, nsize));

    product = cpl_matrix_product_create(matrix, xtrue);
    cpl_test_nonnull(product);

    bytes = (size_t)n * cpl_test_get_bytes_matrix(matrix);

    flops0 = cpl_tools_get_flops();
    secs = cpl_test_get_cputime();

    for (i = 0; i < n; i++) {

        cpl_matrix * xsolv = cpl_matrix_solve_normal(matrix, product);

        cpl_test_nonnull(xsolv);

        cpl_matrix_delete(xsolv);
    }

    secs = cpl_test_get_cputime() - secs;
    flops0 = cpl_tools_get_flops() - flops0;

    cpl_msg_info(cpl_func,"Speed while solving with size %d X %d in %g secs "
                 "[Mflop/s]: %g (%g)", nsize, nsize, secs/(double)i,
                 (double)flops0/secs/1e6, (double)flops0);
    if (secs > 0.0) {
        cpl_msg_info(cpl_func,"Processing rate [MB/s]: %g",
                     1e-6 * (double)bytes / secs);
    }


    cpl_matrix_delete(xtrue);
    cpl_matrix_delete(product);
    cpl_matrix_delete(matrix);

}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Benchmark the CPL function
  @param nsize The size of the CPL object
  @param n     The number of repeats
  @return   void
 */
/*----------------------------------------------------------------------------*/
static void cpl_matrix_product_transpose_bench(int nsize, int n)
{

    double       secs;
    cpl_matrix * in  = cpl_matrix_new(nsize, nsize);
    cpl_matrix * out = cpl_matrix_new(nsize, nsize);
    const size_t bytes = (size_t)n * cpl_test_get_bytes_matrix(in);
    int i;

    cpl_matrix_fill(in, 1.0);

    secs  = cpl_test_get_cputime();

    for (i = 0; i < n; i++) {
        const cpl_error_code error = cpl_matrix_product_transpose(out, in, in);
        cpl_test_eq_error(error, CPL_ERROR_NONE);
    }

    secs = cpl_test_get_cputime() - secs;

    cpl_msg_info(cpl_func,"Time spent transposing %d %dx%d matrices "
                 "[s]: %g", n, nsize, nsize, secs);
    if (secs > 0.0) {
        cpl_msg_info(cpl_func,"Processing rate [MB/s]: %g",
                     1e-6 * (double)bytes / secs);
    }

    cpl_matrix_delete(out);
    cpl_matrix_delete(in);
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief   Create a matrix with the product of B * A * B'
  @param   ma    The matrix A, of size M x M
  @param   mb    The matrix B, of size N x M
  @return  The product matrix, or NULL on error
  @see     cpl_matrix_product_bilinear(), cpl_matrix_product_create()
  @note Computed using two product

  
*/
/*----------------------------------------------------------------------------*/
static cpl_matrix * cpl_matrix_product_bilinear_brute(const cpl_matrix * A,
                                                      const cpl_matrix * B)
{

#if 1
    cpl_matrix         * C     = cpl_matrix_new(cpl_matrix_get_nrow(B),
                                                cpl_matrix_get_ncol(B));
    const cpl_error_code error = cpl_matrix_product_transpose(C, A, B);
    cpl_matrix * self = cpl_matrix_product_create(B, C);

    cpl_ensure(!error, cpl_error_get_code(), NULL);
   
#else
    /* An even less efficient method */
    cpl_matrix * Bt   = cpl_matrix_transpose_create(B);
    cpl_matrix * C    = cpl_matrix_product_create(A, Bt);
    cpl_matrix * self = cpl_matrix_product_create(B, C);

    cpl_matrix_delete(Bt);
#endif
    cpl_matrix_delete(C);

    return self;

}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief   Shift a matrix in the manner of cpl_matrix_shift()
  @param matrix    Pointer to matrix to be modified.
  @param rshift    Shift in the vertical direction.
  @param cshift    Shift in the horizontal direction.
  @return @c CPL_ERROR_NONE on success.
  @see     cpl_matrix_shift()

  
*/
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_matrix_shift_brute(cpl_matrix *matrix,
                                             cpl_size rshift,
                                             cpl_size cshift)
{
    cpl_error_code error = CPL_ERROR_NONE;
    const cpl_size nr = cpl_matrix_get_nrow(matrix);
    const cpl_size nc = cpl_matrix_get_ncol(matrix);

    if (matrix == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    rshift = rshift % nr;
    cshift = cshift % nc;

    if (rshift != 0 || cshift != 0) {
        cpl_matrix  *copy = cpl_matrix_duplicate(matrix);

        /* Should not be able to fail now */

        if (rshift < 0)
            rshift += nr;

        if (cshift < 0)
            cshift += nc;

        /*
         * This is not very efficient, but for the moment it may suffice...
         */

        error = cpl_matrix_copy(matrix, copy, rshift, cshift);

        if (!error && rshift) error =
            cpl_matrix_copy(matrix, copy, rshift - nr, cshift);

        if (!error && cshift) error =
            cpl_matrix_copy(matrix, copy, rshift, cshift - nc);

        if (!error && rshift && cshift) error = 
            cpl_matrix_copy(matrix, copy, rshift - nr,
                            cshift - nc);

        cpl_matrix_delete(copy);
    }

    return cpl_error_set_(error); /* Propagate error, if any */
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Test the CPL function
  @param nr       The number of matrix rows
  @param nc       The number of matrix columns
  @param do_bench Whether or not benching is enabled
  @return   void
 */
/*----------------------------------------------------------------------------*/
static
void cpl_matrix_product_bilinear_test(cpl_size nr, cpl_size nc,
                                      cpl_boolean do_bench)
{

    cpl_matrix   * self   = cpl_matrix_new(nr, nr);
    cpl_matrix   * jacobi = cpl_matrix_new(nr, nc);
    cpl_matrix   * matrix = cpl_matrix_new(nc, nc);
    cpl_matrix   * brute;
    cpl_error_code error;
    double         tfunc, tbrute, t0;
    cpl_flops      ffunc, fbrute, f0;


    error = cpl_matrix_fill_test(jacobi, nr, nc);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_matrix_fill_test(matrix, nc, nc);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    f0 = cpl_tools_get_flops();
    t0 = cpl_test_get_cputime();
    error = cpl_matrix_product_bilinear(self, matrix, jacobi);
    tfunc = cpl_test_get_cputime() - t0;
    ffunc = cpl_tools_get_flops() - f0;
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    f0 = cpl_tools_get_flops();
    t0 = cpl_test_get_cputime();
    brute = cpl_matrix_product_bilinear_brute(matrix, jacobi);
    tbrute = cpl_test_get_cputime() - t0;
    fbrute = cpl_tools_get_flops() - f0;

    /* Same number of FP operations */
    cpl_test_eq(ffunc, fbrute);

    cpl_test_matrix_abs(self, brute, 250.0 * DBL_EPSILON);

    cpl_matrix_delete(self);
    cpl_matrix_delete(jacobi);
    cpl_matrix_delete(matrix);
    cpl_matrix_delete(brute);

    if (do_bench) {
        cpl_msg_info(cpl_func,"Time spent on B * A * B', B: %" CPL_SIZE_FORMAT
                     "x%" CPL_SIZE_FORMAT " [s]: %g <=> %g (brute force)",
                     nr, nc, tfunc, tbrute);
        if (tfunc > 0.0 && tbrute > 0.0) {
            cpl_msg_info(cpl_func,"Speed while computing B * A * B', B: %"
                         CPL_SIZE_FORMAT "x%" CPL_SIZE_FORMAT " [Mflop/s]: "
                         "%g <=> %g (brute force)", nr, nc,
                         (double)ffunc/tfunc/1e6, (double)fbrute/tbrute/1e6);
        }
    }
}



/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    LU factor a banded matrix
  @param nc       The number of matrix columns/rows
  @param np       The bandedness
  @return   void
 */
/*----------------------------------------------------------------------------*/
static void cpl_matrix_test_banded(int nc, int np)
{
    cpl_matrix * A = cpl_matrix_new(nc, nc);
    cpl_array  * p = cpl_array_new(nc, CPL_TYPE_INT);
    int i;
    cpl_error_code error;
    double         tfunc, t0;
    cpl_flops      ffunc, f0;

    for (i = -np; i <= np; i++) {
        error = cpl_matrix_fill_diagonal(A, 1.0, i);
        cpl_test_eq_error(error, CPL_ERROR_NONE);
    }

    if(cpl_msg_get_level() <= CPL_MSG_DEBUG)
        cpl_matrix_dump(A, stderr);

    f0 = cpl_tools_get_flops();
    t0 = cpl_test_get_cputime();
    error = cpl_matrix_decomp_lu(A, p, &i);
    tfunc = cpl_test_get_cputime() - t0;
    ffunc = cpl_tools_get_flops() - f0;
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    if(cpl_msg_get_level() <= CPL_MSG_DEBUG)
        cpl_array_dump(p, 0, nc, stderr);

    cpl_matrix_delete(A);
    cpl_array_delete(p);

    cpl_msg_info(cpl_func,"Time spent on LU-decomposition: %d X %d [s]: %g",
                 nc, nc, tfunc);
    if (tfunc > 0.0) {
        cpl_msg_info(cpl_func,"Speed while computing LU-decomposition: %d "
                     "X %d [Mflop/s]: %g", nc, nc, (double)ffunc/tfunc/1e6);
    }
}
