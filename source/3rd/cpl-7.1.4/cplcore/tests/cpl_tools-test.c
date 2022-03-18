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

#include <cpl_tools.h>
#include <cpl_test.h>

#include <cpl_memory.h>

#include <string.h>

/*-----------------------------------------------------------------------------
                                   Defines
 -----------------------------------------------------------------------------*/

#define NBC   8
#define NBC2 11

/*-----------------------------------------------------------------------------
                        Private function prototypes
 -----------------------------------------------------------------------------*/

static void cpl_tools_get_kth_quickselection_bench(int, int, cpl_boolean);
static void cpl_tools_get_median_bench(int, int, cpl_boolean);
static void cpl_tools_sort_bench(int, int, cpl_boolean);

/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/
int main(void)
{

    /* Some binomial coefficients - in reverse order */
    const int   p15[NBC2]  = {646646, 497420, 319770, 170544, 74613, 26334,
                              7315, 1540, 231, 22, 1};
    const float pf15[NBC2] = {646646, 497420, 319770, 170544, 74613, 26334,
                              7315, 1540, 231, 22, 1};
    int   c15[NBC2];
    float cf15[NBC2];
    int c0[NBC] = {1, 1, 1, 1, 0, 0, 0, 0};
    cpl_boolean   do_bench;


    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    do_bench = cpl_msg_get_level() <= CPL_MSG_INFO ? CPL_TRUE : CPL_FALSE;

    cpl_test_abs( cpl_tools_ipow(1.0, 0), 1.0, 0.0);
    cpl_test_abs( cpl_tools_ipow(0.0, 0), 1.0, 0.0);
    cpl_test_abs( cpl_tools_ipow(0.0, 1), 0.0, 0.0);
    cpl_test_abs( cpl_tools_ipow(0.0, 2), 0.0, 0.0);

    cpl_test_eq_ptr(c15, memcpy(c15, p15, sizeof(p15)));

    /* Find two smallest */
    cpl_test_eq(p15[NBC2-2], cpl_tools_get_kth_int(c15, NBC2, 1));

    /* Find two largest */
    cpl_test_eq(p15[1], cpl_tools_get_kth_int(c15, NBC2, NBC2-2));
    cpl_test_eq(p15[0], c15[NBC2-1]);

    cpl_test_eq(p15[NBC2-1], c15[0]); /* Check of 2nd smallest delayed */

    cpl_test_zero(cpl_tools_get_kth_int(c0, NBC, 3));
    cpl_test_zero(c0[0]);
    cpl_test_zero(c0[1]);
    cpl_test_zero(c0[2]);
    cpl_test_zero(c0[3]);
    cpl_test_eq(c0[4], 1);
    cpl_test_eq(c0[5], 1);
    cpl_test_eq(c0[6], 1);
    cpl_test_eq(c0[7], 1);

    cpl_test_eq_ptr(c15, memcpy(c15, p15, sizeof(p15)));

    /* Find median */
    cpl_test_eq(p15[(NBC2-1)/2], cpl_tools_quickselection_int(c15, NBC2, (NBC2-1)/2));
    cpl_test_eq(p15[(NBC2-1)/2], c15[(NBC2-1)/2]);

    /* Special case: 3 element median computation */
    cpl_test_eq_ptr(c15, memcpy(c15, p15, sizeof(p15)));
    cpl_test_eq(p15[1], cpl_tools_get_median_int(c15, 3));
    cpl_test_eq(p15[0], c15[2]);
    cpl_test_eq(p15[2], c15[0]);

    /* Special case: 5 element median computation */
    cpl_test_eq_ptr(c15, memcpy(c15, p15, sizeof(p15)));
    cpl_test_eq(p15[2], cpl_tools_get_median_int(c15, 5));
    cpl_test_lt(c15[0], c15[2]);
    cpl_test_lt(c15[1], c15[2]);
    cpl_test_lt(c15[2], c15[3]);
    cpl_test_lt(c15[2], c15[4]);

    /* Special case: 6 element median computation */
    cpl_test_eq_ptr(c15, memcpy(c15, p15, sizeof(p15)));
    cpl_test_eq((p15[2]+p15[3])/2, cpl_tools_get_median_int(c15, 6));
    cpl_test_lt(c15[0], c15[2]);
    cpl_test_lt(c15[1], c15[2]);
    cpl_test_lt(c15[2], c15[3]);
    cpl_test_lt(c15[2], c15[4]);
    cpl_test_lt(c15[2], c15[5]);

    /* Special case: 7 element median computation */
    cpl_test_eq_ptr(c15, memcpy(c15, p15, sizeof(p15)));
    cpl_test_eq(p15[3], cpl_tools_get_median_int(c15, 7));
    cpl_test_lt(c15[0], c15[3]);
    cpl_test_lt(c15[1], c15[3]);
    cpl_test_lt(c15[2], c15[3]);
    cpl_test_lt(c15[3], c15[4]);
    cpl_test_lt(c15[3], c15[5]);
    cpl_test_lt(c15[3], c15[6]);

    /* Normal median case with even number: 10 element median computation */
    cpl_test_eq_ptr(c15, memcpy(c15, p15, sizeof(p15)));
    cpl_test_eq((p15[4]+p15[5])/2, cpl_tools_get_median_int(c15, 10));
    cpl_test_lt(c15[0], c15[4]);
    cpl_test_lt(c15[1], c15[4]);
    cpl_test_lt(c15[2], c15[4]);
    cpl_test_lt(c15[3], c15[4]);
    cpl_test_lt(c15[5], c15[6]);
    cpl_test_lt(c15[5], c15[7]);
    cpl_test_lt(c15[5], c15[8]);
    cpl_test_lt(c15[5], c15[9]);

    /* Normal median case with even number: 11 element median computation */
    cpl_test_eq_ptr(c15, memcpy(c15, p15, sizeof(p15)));
    cpl_test_eq(p15[5], cpl_tools_get_median_int(c15, 11));
    cpl_test_lt(c15[0], c15[5]);
    cpl_test_lt(c15[1], c15[5]);
    cpl_test_lt(c15[2], c15[5]);
    cpl_test_lt(c15[3], c15[5]);
    cpl_test_lt(c15[4], c15[5]);
    cpl_test_lt(c15[5], c15[6]);
    cpl_test_lt(c15[5], c15[7]);
    cpl_test_lt(c15[5], c15[8]);
    cpl_test_lt(c15[5], c15[9]);
    cpl_test_lt(c15[5], c15[10]);

    /* Float case */
    /* Normal median case with even number: 10 element median computation */
    cpl_test_eq_ptr(cf15, memcpy(cf15, pf15, sizeof(pf15)));
    cpl_test_eq((pf15[4]+pf15[5])/2., cpl_tools_get_median_float(cf15, 10));
    cpl_test_lt(cf15[0], cf15[4]);
    cpl_test_lt(cf15[1], cf15[4]);
    cpl_test_lt(cf15[2], cf15[4]);
    cpl_test_lt(cf15[3], cf15[4]);
    cpl_test_lt(cf15[5], cf15[6]);
    cpl_test_lt(cf15[5], cf15[7]);
    cpl_test_lt(cf15[5], cf15[8]);
    cpl_test_lt(cf15[5], cf15[9]);

    /* Float case */
    /* Normal median case with even number: 11 element median computation */
    cpl_test_eq_ptr(cf15, memcpy(cf15, pf15, sizeof(pf15)));
    cpl_test_eq(pf15[5], cpl_tools_get_median_float(cf15, 11));
    cpl_test_lt(cf15[0], cf15[5]);
    cpl_test_lt(cf15[1], cf15[5]);
    cpl_test_lt(cf15[2], cf15[5]);
    cpl_test_lt(cf15[3], cf15[5]);
    cpl_test_lt(cf15[4], cf15[5]);
    cpl_test_lt(cf15[5], cf15[6]);
    cpl_test_lt(cf15[5], cf15[7]);
    cpl_test_lt(cf15[5], cf15[8]);
    cpl_test_lt(cf15[5], cf15[9]);
    cpl_test_lt(cf15[5], cf15[10]);

    cpl_tools_get_kth_quickselection_bench(1, 5, CPL_FALSE);
    cpl_tools_get_kth_quickselection_bench(1, 9, CPL_FALSE);
    cpl_tools_get_kth_quickselection_bench(1, 31, CPL_FALSE);

    cpl_tools_get_kth_quickselection_bench(1, 5, CPL_TRUE);
    cpl_tools_get_kth_quickselection_bench(1, 9, CPL_TRUE);
    cpl_tools_get_kth_quickselection_bench(1, 31, CPL_TRUE);

    cpl_tools_get_median_bench(1, 5, CPL_FALSE);
    cpl_tools_get_median_bench(1, 30, CPL_FALSE);
    cpl_tools_get_median_bench(1, 31, CPL_FALSE);

    cpl_tools_get_median_bench(1, 5, CPL_TRUE);
    cpl_tools_get_median_bench(1, 30, CPL_TRUE);
    cpl_tools_get_median_bench(1, 31, CPL_TRUE);

    cpl_tools_sort_bench(1, 5, CPL_FALSE);
    cpl_tools_sort_bench(1, 9, CPL_FALSE);
    cpl_tools_sort_bench(1, 31, CPL_FALSE);

    cpl_tools_sort_bench(1, 5, CPL_TRUE);
    cpl_tools_sort_bench(1, 9, CPL_TRUE);
    cpl_tools_sort_bench(1, 31, CPL_TRUE);

    if (do_bench) {
        cpl_tools_sort_bench(5, 100001, CPL_FALSE);
        cpl_tools_sort_bench(5,  10001, CPL_TRUE);
        cpl_tools_get_kth_quickselection_bench(5,   10001, CPL_FALSE);
        cpl_tools_get_kth_quickselection_bench(5,   10001, CPL_TRUE);
        cpl_tools_get_kth_quickselection_bench(5,   25001, CPL_FALSE);
        cpl_tools_get_kth_quickselection_bench(5,   25001, CPL_TRUE);
        cpl_tools_get_kth_quickselection_bench(5,  200001, CPL_FALSE);
        cpl_tools_get_kth_quickselection_bench(5, 2000001, CPL_FALSE);
#ifdef CPL_TOOLS_TEST_LONG
        /* Example output (Intel E6750 @ 2.66GHz):
[ INFO  ] Benchmarked kth  5 x 200001 [s]: 484.03 <=> 311.49
[ INFO  ] Benchmarked kth  5 x 2000001 [s]: 52318.6 <=> 32385.4
         */
        cpl_tools_get_kth_quickselection_bench(5,  200001, CPL_TRUE);
        cpl_tools_get_kth_quickselection_bench(5, 2000001, CPL_TRUE);
#endif
        cpl_tools_get_median_bench(5,   25000, CPL_FALSE);
        cpl_tools_get_median_bench(5,   25000, CPL_TRUE);
        cpl_tools_get_median_bench(5,   25001, CPL_FALSE);
        cpl_tools_get_median_bench(5,   25001, CPL_TRUE);
        cpl_tools_get_median_bench(5,  200000, CPL_FALSE);
        cpl_tools_get_median_bench(5,  200001, CPL_FALSE);
        cpl_tools_get_median_bench(5, 2000000, CPL_FALSE);
        cpl_tools_get_median_bench(5, 2000001, CPL_FALSE);
    }

    return cpl_test_end(0);

}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief      Benchmark the two CPL functions
  @param m           The number of repeats
  @param n           The number of elements
  @param do_sortmost Sort all but three elements
  @return    void

 */
/*----------------------------------------------------------------------------*/
static void cpl_tools_get_kth_quickselection_bench(int m, int n,
                                                   cpl_boolean do_sortmost)
{

    double tqsel = 0.0;
    double tkth  = 0.0;
    int i;

    for (i = 0; i < m; i++) {
        double * dd = (double*)cpl_malloc((size_t)n * sizeof(double));
        float  * df = (float*) cpl_malloc((size_t)n * sizeof(float));
        int    * di = (int*)   cpl_malloc((size_t)n * sizeof(int));

        double * cd = (double*)cpl_malloc((size_t)n * sizeof(double));
        float  * cf = (float*) cpl_malloc((size_t)n * sizeof(float));
        int    * ci = (int*)   cpl_malloc((size_t)n * sizeof(int));
        double sec0;
        double dmed1, dmed2;
        float  fmed1, fmed2;
        int    imed1, imed2;
        int j;


        for (j = 0; j < n; j++) {
            di[j] = rand();
        }

        if (do_sortmost) {
            cpl_tools_sort_int(di, n);

            di[n/2-1] = di[n/2];
            di[n/2]   = RAND_MAX;
            di[n/2+1] = -1;
        }

        for (j = 0; j < n; j++) {
            df[j] = (float)di[j];
            dd[j] = (double)di[j];
        }

        /* Benchmark in double precision */
        cpl_test_eq_ptr(cd, memcpy(cd, dd, n * sizeof(*dd)));
        sec0 = cpl_test_get_cputime();
        dmed1 = cpl_tools_get_kth_double(cd, n, (n&1) ? n / 2 : n / 2 - 1);
        tkth += cpl_test_get_cputime() - sec0;

        cpl_test_eq_ptr(cd, memcpy(cd, dd, n * sizeof(*dd)));
        sec0 = cpl_test_get_cputime();
        dmed2 = cpl_tools_quickselection_double(cd, n, (n-1)/2);
        tqsel += cpl_test_get_cputime() - sec0;
        cpl_test_abs(dmed1, dmed2, DBL_EPSILON);

        /* Benchmark with float */
        cpl_test_eq_ptr(cf, memcpy(cf, df, n * sizeof(*df)));
        sec0 = cpl_test_get_cputime();
        fmed1 = cpl_tools_get_kth_float(cf, n, (n&1) ? n / 2 : n / 2 - 1);
        tkth += cpl_test_get_cputime() - sec0;

        cpl_test_eq_ptr(cf, memcpy(cf, df, n * sizeof(*df)));
        sec0 = cpl_test_get_cputime();
        fmed2 = cpl_tools_quickselection_float(cf, n, (n-1)/2);
        tqsel += cpl_test_get_cputime() - sec0;
        cpl_test_abs(fmed1, fmed2, FLT_EPSILON);

        /* Benchmark with int */
        cpl_test_eq_ptr(ci, memcpy(ci, di, n * sizeof(*di)));
        sec0 = cpl_test_get_cputime();
        imed1 = cpl_tools_get_kth_int(ci, n, (n&1) ? n / 2 : n / 2 - 1);
        tkth += cpl_test_get_cputime() - sec0;

        cpl_test_eq_ptr(ci, memcpy(ci, di, n * sizeof(*di)));
        sec0 = cpl_test_get_cputime();
        imed2 = cpl_tools_quickselection_int(ci, n, (n-1)/2);
        tqsel += cpl_test_get_cputime() - sec0;
        cpl_test_eq(imed1, imed2);


        cpl_free(dd);
        cpl_free(df);
        cpl_free(di);
        cpl_free(cd);
        cpl_free(cf);
        cpl_free(ci);
    }

    cpl_msg_info(cpl_func, "Benchmarked kth <=> QuickSelect%s %d x %d [s]: "
                 "%g <=> %g", do_sortmost ? " (NS)" : "",
                 m, n, tkth, tqsel);

    return;

}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief      Benchmark the two CPL functions
  @param m           The number of repeats
  @param n           The number of elements
  @param do_sortmost Sort all but three elements
  @return    void

*/
/*----------------------------------------------------------------------------*/
static void cpl_tools_get_median_bench(int m, int n,
                                                   cpl_boolean do_sortmost)
{

    double tmedian  = 0.0;
    int i;

    for (i = 0; i < m; i++) {
        double * dd = (double*)cpl_malloc((size_t)n * sizeof(double));
        float  * df = (float*) cpl_malloc((size_t)n * sizeof(float));
        int    * di = (int*)   cpl_malloc((size_t)n * sizeof(int));

        double * cd = (double*)cpl_malloc((size_t)n * sizeof(double));
        float  * cf = (float*) cpl_malloc((size_t)n * sizeof(float));
        int    * ci = (int*)   cpl_malloc((size_t)n * sizeof(int));
        double sec0;
        int j;


        for (j = 0; j < n; j++) {
            di[j] = rand();
        }

        if (do_sortmost) {
            cpl_tools_sort_int(di, n);

            di[n/2-1] = di[n/2];
            di[n/2]   = RAND_MAX;
            di[n/2+1] = -1;
        }

        for (j = 0; j < n; j++) {
            df[j] = (float)di[j];
            dd[j] = (double)di[j];
        }

        /* Benchmark in double precision */
        cpl_test_eq_ptr(cd, memcpy(cd, dd, n * sizeof(*dd)));
        sec0 = cpl_test_get_cputime();
        cpl_tools_get_median_double(cd, n);
        tmedian += cpl_test_get_cputime() - sec0;

        /* Benchmark with float */
        cpl_test_eq_ptr(cf, memcpy(cf, df, n * sizeof(*df)));
        sec0 = cpl_test_get_cputime();
        cpl_tools_get_median_float(cf, n);
        tmedian += cpl_test_get_cputime() - sec0;

        /* Benchmark with int */
        cpl_test_eq_ptr(ci, memcpy(ci, di, n * sizeof(*di)));
        sec0 = cpl_test_get_cputime();
        cpl_tools_get_median_int(ci, n);
        tmedian += cpl_test_get_cputime() - sec0;


        cpl_free(dd);
        cpl_free(df);
        cpl_free(di);
        cpl_free(cd);
        cpl_free(cf);
        cpl_free(ci);
    }

    cpl_msg_info(cpl_func, "Benchmarked median%s %d x %d [s]: "
                 "%g", do_sortmost ? " (NS)" : "",
                 m, n, tmedian);

    return;

}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief     Benchmark the CPL sort function against qsort()
  @param m           The number of repeats
  @param n           The number of elements
  @param do_sortmost Sort all but three elements
  @return    void
  @see qsort

 */
/*----------------------------------------------------------------------------*/
static void cpl_tools_sort_bench(int m, int n, cpl_boolean do_sortmost)
{

    double tqs = 0.0;
    double tcpl  = 0.0;
    int i;

    for (i = 0; i < m; i++) {
        double * dd = (double*)cpl_malloc((size_t)n * sizeof(double));
        float  * df = (float*) cpl_malloc((size_t)n * sizeof(float));
        int    * di = (int*)   cpl_malloc((size_t)n * sizeof(int));

        double * cd = (double*)cpl_malloc((size_t)n * sizeof(double));
        float  * cf = (float*) cpl_malloc((size_t)n * sizeof(float));
        int    * ci = (int*)   cpl_malloc((size_t)n * sizeof(int));
        double sec0;
        double dmed1, dmed2;
        float  fmed1, fmed2;
        int    imed1, imed2;
        int j;


        for (j = 0; j < n; j++) {
            di[j] = rand();
        }

        if (do_sortmost) {
            cpl_tools_sort_int(di, n);

            di[n/2-1] = di[n/2];
            di[n/2]   = RAND_MAX;
            di[n/2+1] = -1;
        }

        for (j = 0; j < n; j++) {
            df[j] = (float)di[j];
            dd[j] = (double)di[j];
        }

        /* Benchmark in double precision */
        cpl_test_eq_ptr(cd, memcpy(cd, dd, n * sizeof(*dd)));
        sec0 = cpl_test_get_cputime();
        cpl_tools_sort_double(cd, n);
        tcpl += cpl_test_get_cputime() - sec0;

        dmed1 = cd[n/2];

        cpl_test_eq_ptr(cd, memcpy(cd, dd, n * sizeof(*dd)));
        sec0 = cpl_test_get_cputime();
        cpl_tools_sort_ascn_double(cd, n);
        tqs += cpl_test_get_cputime() - sec0;

        dmed2 = cd[n/2];
        cpl_test_abs(dmed1, dmed2, DBL_EPSILON);


        /* Benchmark with float */
        cpl_test_eq_ptr(cf, memcpy(cf, df, n * sizeof(*df)));
        sec0 = cpl_test_get_cputime();
        cpl_tools_sort_float(cf, n);
        tcpl += cpl_test_get_cputime() - sec0;

        fmed1 = cf[n/2];

        cpl_test_eq_ptr(cf, memcpy(cf, df, n * sizeof(*df)));
        sec0 = cpl_test_get_cputime();
        cpl_tools_sort_ascn_float(cf, n);
        tqs += cpl_test_get_cputime() - sec0;

        fmed2 = cf[n/2];
        cpl_test_abs(fmed1, fmed2, DBL_EPSILON);

        /* Benchmark with int */
        cpl_test_eq_ptr(ci, memcpy(ci, di, n * sizeof(*di)));
        sec0 = cpl_test_get_cputime();
        cpl_tools_sort_int(ci, n);
        tcpl += cpl_test_get_cputime() - sec0;

        imed1 = ci[n/2];

        cpl_test_eq_ptr(ci, memcpy(ci, di, n * sizeof(*di)));
        sec0 = cpl_test_get_cputime();
        cpl_tools_sort_ascn_int(ci, n);
        tqs += cpl_test_get_cputime() - sec0;

        imed2 = ci[n/2];
        cpl_test_eq(imed1, imed2);

        cpl_free(dd);
        cpl_free(df);
        cpl_free(di);
        cpl_free(cd);
        cpl_free(cf);
        cpl_free(ci);
    }

    cpl_msg_info(cpl_func, "Benchmarked sort %d x %d [s]: %g <=> %g", m, n,
                 tcpl, tqs);

    return;

}
