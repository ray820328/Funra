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

/*
 * This unit test performs a more extensive test of the median function as
 * recommended by ticket PIPE-4739.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <cpl_tools.h>
#include <cpl_test.h>
#include <cpl_memory.h>

/*-----------------------------------------------------------------------------
                                   Defines
 -----------------------------------------------------------------------------*/

/* The maximum number of data elements for which all possible permutations will
 * be tested. */
#define MAX_ARRAY_ALL_PERMUTATIONS     8

/* The maximum number of data elements for which a random sampling of
 * permutations will be tested. */
#define MAX_ARRAY_RANDOM_PERMUTATIONS  26

/* The maximum number of permutations to generate per call of the
 * cpl_test_random_permutations function. */
#define MAX_PERMUTATIONS               10000

/*-----------------------------------------------------------------------------
                        Private function prototypes
 -----------------------------------------------------------------------------*/

static int cpl_test_all_permutations(double* data, cpl_size n,
                          int (*check_permutation)(double* data, cpl_size n));

static int cpl_test_random_permutations(double* data, cpl_size n,
                          int (*check_permutation)(double* data, cpl_size n));

static void cpl_print_failed_permutation(double* data, cpl_size n);
static int cpl_check_median_mid(double* data, cpl_size n);
static int cpl_check_median_floor(double* data, cpl_size n);
static int cpl_check_median_ceil(double* data, cpl_size n);

/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/

int main(void)
{
    cpl_size i, n;
    double data[MAX_ARRAY_RANDOM_PERMUTATIONS];

    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    srand(1);  /* Fix the seed for repeatable results. */

    /* Test special case with data array with only one element. */
    data[0] = 1.5;
    cpl_test_abs(cpl_tools_get_median_double(data, 1), 1.5, DBL_EPSILON);
    cpl_test_error(CPL_ERROR_NONE);

    /* Here we perform 3 tests on all possible permutations for data arrays of
     * size 2 to MAX_ARRAY_ALL_PERMUTATIONS. For each test the data array is
     * prepared with elements from one of 3 corresponding sets. These are given
     * by:
     *   (1)  {0, 1, ... n-1}.
     *   (2)  {floor(n/2), 1, 2, ... n-1}
     *   (3)  {floor(n/2)+1, 1, 2, ... n-1}  (only if n is odd)
     * The expected median for each of the data sets is given by:
     *   (1)  (n-1)/2
     *   (2)  floor(n/2)
     *   (3)  ceil(n/2)   (only valid if n is odd)
     */
    for (n = 2; n <= MAX_ARRAY_ALL_PERMUTATIONS; ++n) {
        for (i = 0; i < n; ++i) {
            data[i] = i;
        }
        if (cpl_test_all_permutations(data, n, cpl_check_median_mid)
            != EXIT_SUCCESS)
        {
            cpl_print_failed_permutation(data, n);
            break;
        }

        for (i = 0; i < n; ++i) {
            data[i] = i;
        }
        data[0] = n/2;
        if (cpl_test_all_permutations(data, n, cpl_check_median_floor)
            != EXIT_SUCCESS)
        {
            cpl_print_failed_permutation(data, n);
            break;
        }

        if (n % 2) {  /* If odd */
            for (i = 0; i < n; ++i) {
                data[i] = i;
            }
            data[0] = n/2+1;
            if (cpl_test_all_permutations(data, n, cpl_check_median_ceil)
                != EXIT_SUCCESS)
            {
                cpl_print_failed_permutation(data, n);
                break;
            }
        }
    }

    /* Run the 3 tests as before but generating random permutations for data
     * array sizes from MAX_ARRAY_ALL_PERMUTATIONS + 1
     * to MAX_ARRAY_RANDOM_PERMUTATIONS. */
    for (n = MAX_ARRAY_ALL_PERMUTATIONS+1; n <= MAX_ARRAY_RANDOM_PERMUTATIONS;
         ++n)
    {
        for (i = 0; i < n; ++i) {
            data[i] = i;
        }
        if (cpl_test_random_permutations(data, n, cpl_check_median_mid)
            != EXIT_SUCCESS)
        {
            cpl_print_failed_permutation(data, n);
            break;
        }

        for (i = 0; i < n; ++i) {
            data[i] = i;
        }
        data[0] = n/2;
        if (cpl_test_random_permutations(data, n, cpl_check_median_floor)
            != EXIT_SUCCESS)
        {
            cpl_print_failed_permutation(data, n);
            break;
        }

        if (n % 2) {  /* If odd */
            for (i = 0; i < n; ++i) {
                data[i] = i;
            }
            data[0] = n/2+1;
            if (cpl_test_random_permutations(data, n, cpl_check_median_ceil)
                != EXIT_SUCCESS)
            {
                cpl_print_failed_permutation(data, n);
                break;
            }
        }
    }

    return cpl_test_end(0);
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief   Prints out the data for a failed permutation as error messages.
 */
/*----------------------------------------------------------------------------*/
static void cpl_print_failed_permutation(double* data, cpl_size n)
{
    cpl_size i;
    cpl_msg_error(cpl_func, "Failed test on permutation with %"CPL_SIZE_FORMAT
                  " elements:", n);
    cpl_msg_error(cpl_func, "Item\tValue");
    for (i = 0; i < n; ++i) {
        cpl_msg_error(cpl_func, "%"CPL_SIZE_FORMAT"\t%f", i+1, data[i]);
    }
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief   Checks if median is computed correctly as (n-1)/2.
 */
/*----------------------------------------------------------------------------*/
static int cpl_check_median_mid(double* data, cpl_size n)
{
    cpl_test_abs(cpl_tools_get_median_double(data, n), 0.5*(n-1),
                 DBL_EPSILON);
    cpl_test_error(CPL_ERROR_NONE);
    return cpl_test_get_failed() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief   Checks if median is computed correctly as floor(n/2).
 */
/*----------------------------------------------------------------------------*/
static int cpl_check_median_floor(double* data, cpl_size n)
{
    cpl_test_abs(cpl_tools_get_median_double(data, n), floor(0.5*n),
                 DBL_EPSILON);
    cpl_test_error(CPL_ERROR_NONE);
    return cpl_test_get_failed() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief   Checks if median is computed correctly as ceil(n/2).
 */
/*----------------------------------------------------------------------------*/
static int cpl_check_median_ceil(double* data, cpl_size n)
{
    cpl_test_abs(cpl_tools_get_median_double(data, n), ceil(0.5*n),
                 DBL_EPSILON);
    cpl_test_error(CPL_ERROR_NONE);
    return cpl_test_get_failed() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief   Generates and tests all permutations of a data array.

  @param[in,out] data  Array of data elements to permute. This array will be
      modified as the function generates the different permutations. If the
      function exited with @c EXIT_FAILURE then this will contain the actual
      permutation that @c check_permutation failed on.
  @param[in] n   The number of elements in the @c data array.
  @param[in] check_permutation  The function that will be applied to every
      permutation. It will be given a copy of the permuted data array (thus it
      can modify this buffer if need be) and should return @c EXIT_SUCCESS if
      processing can continue and @c EXIT_FAILURE if a test failed and no more
      permutations should be tested.
  @return @c EXIT_SUCCESS if all permutations were processed by
      @c check_permutation. Otherwise @c EXIT_FAILURE if @c check_permutation
      also returned @c EXIT_FAILURE at any point in time.

  The aim of this function is to generate all possible permutations of a given
  data array and apply the @c check_permutation() function to each one. The
  @c check_permutation function can perform any checks that it need to on the
  supplied permutation. If @c check_permutation returns @c EXIT_FAILURE then no
  more permutations are generated and this top level function returns
  immediately.

  This function is using the iterative implementation [1] of Heap's permutation
  generation algorithm [2], to generate all permutations of the data array.

  [1] http://permute.tchs.info/ScalablePermutations.html
  [2] Heap, B. R. (1963). "Permutations by Interchanges". The Computer Journal
      6 (3): 293-4
 */
/*----------------------------------------------------------------------------*/
static int cpl_test_all_permutations(double* data, cpl_size n,
                          int (*check_permutation)(double* data, cpl_size n))
{
    cpl_size  i;
    cpl_size* p = cpl_malloc(sizeof(cpl_size)*n);
    double* datacopy = cpl_malloc(sizeof(double)*n);
    cpl_test_assert(p != NULL);
    cpl_test_assert(datacopy != NULL);

    for (i = 0; i < n; ++i) {
      p[i] = i+1;
    }
    memcpy(datacopy, data, sizeof(double)*n);
    if (check_permutation(datacopy, n) == EXIT_FAILURE) return EXIT_FAILURE;
    for (i = 0; i < n-1; ) {
        const cpl_size j = i+1;
        --p[i];
        if (j % 2) {
            /* swap position p[i] and j for odd j. */
            double tmp = data[p[i]];
            data[p[i]] = data[j];
            data[j] = tmp;
        } else {
            /* swap position 0 and j for even j. */
            double tmp = data[0];
            data[0] = data[j];
            data[j] = tmp;
        }
        memcpy(datacopy, data, sizeof(double)*n);
        if (check_permutation(datacopy, n) == EXIT_FAILURE) return EXIT_FAILURE;
        for (i = 0; p[i] == 0; ++i) {
            p[i] = i+1;
        }
    }

    cpl_free(datacopy);
    cpl_free(p);
    return EXIT_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief   Generates and tests a set of random permutations of a data array.

  @param[in,out] data  Array of data elements to permute. This array will be
      modified as the function generates the different permutations. If the
      function exited with @c EXIT_FAILURE then this will contain the actual
      permutation that @c check_permutation failed on.
  @param[in] n   The number of elements in the @c data array.
  @param[in] check_permutation  The function that will be applied to every
      permutation. It will be given a copy of the permuted data array (thus it
      can modify this buffer if need be) and should return @c EXIT_SUCCESS if
      processing can continue and @c EXIT_FAILURE if a test failed and no more
      permutations should be tested.
  @return @c EXIT_SUCCESS if all generated permutations were processed by
      @c check_permutation. Otherwise @c EXIT_FAILURE if @c check_permutation
      also returned @c EXIT_FAILURE at any point in time.
 */
/*----------------------------------------------------------------------------*/
static int cpl_test_random_permutations(double* data, cpl_size n,
                          int (*check_permutation)(double* data, cpl_size n))
{
    cpl_size count = 0;
    double* original = cpl_malloc(sizeof(double)*n);
    double* datacopy = cpl_malloc(sizeof(double)*n);
    cpl_test_assert(original != NULL);
    cpl_test_assert(datacopy != NULL);

    /* Store a copy of the original data. */
    memcpy(original, data, sizeof(double)*n);

    while (count < MAX_PERMUTATIONS) {
        for (cpl_size max_trans = 0; max_trans <= n;
             max_trans = (max_trans+1)*2-1) {
                /* Reset the data array and permute. */
                memcpy(data, original, sizeof(double)*n);
                for (cpl_size i = 0; i < max_trans; ++i) {
                    const int    y   = rand() % n;
                    const double tmp = data[y];
                    int x;
                    while ((x = rand() % n) == y);  /* Make sure x != y. */
                    data[y] = data[x];
                    data[x] = tmp;
            }

            /* Now run the check method. */
            memcpy(datacopy, data, sizeof(double)*n);
            if (check_permutation(datacopy, n) == EXIT_FAILURE) {
                return EXIT_FAILURE;
            }
            ++count;
        }
    }

    cpl_free(datacopy);
    cpl_free(original);
    return EXIT_SUCCESS;
}
