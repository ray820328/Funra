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

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include "cpl_wlcalib_impl.h"

#include <cpl_test.h>
#include <cpl_plot.h>
#include <cpl_tools.h>

#include <stdlib.h>

/*-----------------------------------------------------------------------------
                                   Defines
 -----------------------------------------------------------------------------*/

#ifndef CPL_WLCALIB_SPC_SIZE
#define CPL_WLCALIB_SPC_SIZE 100
#endif

#ifndef CPL_WLCALIB_WSLIT
#define CPL_WLCALIB_WSLIT 3.0
#endif

#ifndef CPL_WLCALIB_WFWHM
#define CPL_WLCALIB_WFWHM 4.0
#endif

#ifndef CPL_WLCALIB_LTHRESH
#define CPL_WLCALIB_LTHRESH 5.0
#endif


#ifndef CPL_WLCALIB_SHIFT
#define CPL_WLCALIB_SHIFT 4
#endif


#ifndef CPL_WLCALIB_NOISE
#define CPL_WLCALIB_NOISE 0.95
#endif


/*-----------------------------------------------------------------------------
                        Private function prototypes
 -----------------------------------------------------------------------------*/

static void cpl_wlcalib_test_one(void);

/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/
int main(void)
{
    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    cpl_wlcalib_test_one();

    return cpl_test_end(0);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief      Test the exported functions
  @return    void

 */
/*----------------------------------------------------------------------------*/
static void cpl_wlcalib_test_one(void)
{
    const double linepos[] = {0.2 * CPL_WLCALIB_SPC_SIZE,
                              0.24 * CPL_WLCALIB_SPC_SIZE,
                              0.5 * CPL_WLCALIB_SPC_SIZE,
                              0.75 * CPL_WLCALIB_SPC_SIZE,
                              0.82 * CPL_WLCALIB_SPC_SIZE};
    const double lineval[] = {1.0, 1.0, 2.0, 3.0, 4.0};

    /* The true dispersion relation */
    const double coeffs[] = {1.0,
                             2.0 / CPL_WLCALIB_SPC_SIZE,
                             3.0 /(CPL_WLCALIB_SPC_SIZE*CPL_WLCALIB_SPC_SIZE)};
    const cpl_size ncoeffs = (cpl_size)(sizeof(coeffs)/sizeof(coeffs[0]));
    const cpl_size nlines  = (cpl_size)(sizeof(linepos)/sizeof(linepos[0]));
    cpl_polynomial * dtrue = cpl_polynomial_new(1);
    cpl_polynomial * dcand = cpl_polynomial_new(1);
    cpl_polynomial * dfind = cpl_polynomial_new(1);
    cpl_bivector   * lines = cpl_bivector_new(nlines);
    cpl_vector     * observed = cpl_vector_new(CPL_WLCALIB_SPC_SIZE);
    cpl_vector     * obsfast  = cpl_vector_new(CPL_WLCALIB_SPC_SIZE);
    cpl_vector     * wl_search = cpl_vector_new(ncoeffs);
    cpl_vector     * wl_sfine  = cpl_vector_new(ncoeffs);
    cpl_vector     * xcorrs;
    double           xcmax = -1.0;
    double           tn0, tn1, tf0, tf1;
    cpl_size         cost, xcost;
    cpl_size         ntests = 1;
    const cpl_size   nsamples = 25;
    const cpl_size   hsize = 2 + abs(CPL_WLCALIB_SHIFT);
    cpl_size         nxc = 1 + 2 * hsize;
    cpl_wlcalib_slitmodel * model = cpl_wlcalib_slitmodel_new();

    FILE           * stream = cpl_msg_get_level() > CPL_MSG_INFO
        ? fopen("/dev/null", "a") : stdout;

    cpl_error_code error;
    cpl_size i;

    cpl_test_eq(nlines, (cpl_size)(sizeof(lineval)/sizeof(lineval[0])));

    cpl_test_nonnull(model);

    for (i = 0; i < ncoeffs; i++) {
        error = cpl_polynomial_set_coeff(dtrue, &i, coeffs[i]);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_polynomial_set_coeff(dcand, &i, coeffs[i]
                                         * CPL_WLCALIB_NOISE);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        ntests *= nsamples;

    }
    nxc *= ntests;
    cpl_test_eq(cpl_polynomial_get_degree(dtrue), ncoeffs - 1);

    error = cpl_wlcalib_slitmodel_set_wslit(model, 3.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_wlcalib_slitmodel_set_wfwhm(model, 4.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_wlcalib_slitmodel_set_threshold(model, 5.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    for (i = 0; i < nlines; i++) {
        const double wl = cpl_polynomial_eval_1d(dtrue, linepos[i], NULL);

        cpl_test_error(CPL_ERROR_NONE);

        error = cpl_vector_set(cpl_bivector_get_x(lines), i, wl);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_vector_set(cpl_bivector_get_y(lines), i, lineval[i]);
        cpl_test_eq_error(error, CPL_ERROR_NONE);
    }

    error = cpl_wlcalib_slitmodel_set_catalog(model, lines);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    /* Create an observed spectrum from the true dispersion relation and
       the line catalog */
    error = cpl_wlcalib_fill_logline_spectrum_fast(obsfast, model, dtrue);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_wlcalib_fill_logline_spectrum(observed, model, dtrue);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    /* FIXME: Ridiculously high tolerance needed with valgrind */
    cpl_test_vector_abs(observed, obsfast, 0.1);

    error = cpl_wlcalib_fill_line_spectrum_fast(obsfast, model, dtrue);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_wlcalib_fill_line_spectrum(observed, model, dtrue);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    /* FIXME: Ridiculously high tolerance needed with valgrind */
    cpl_test_vector_abs(observed, obsfast, 0.2);

    if (cpl_msg_get_level() <= CPL_MSG_INFO) {
        const cpl_vector * dual[] = {NULL, observed, obsfast};
        cpl_vector * fdiff = cpl_vector_duplicate(observed);

        error = cpl_plot_vectors("", "", "", (const cpl_vector**)dual, 3);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_vector_subtract(fdiff, obsfast);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_plot_vector("", "", "", fdiff);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        cpl_vector_delete(fdiff);
    }

    error = cpl_vector_fill(wl_search, 0.20);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cost = cpl_wlcalib_slitmodel_get_cost(model);
    xcost = cpl_wlcalib_slitmodel_get_xcost(model);

    /* No failed fills yet */
    cpl_test_eq(cost, xcost);

    /* Test 1. Failure test: 1st guess is non-monotone */
    xcmax = 0.0;
    i = ncoeffs - 1;
    error = cpl_polynomial_copy(dfind, dtrue);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_polynomial_set_coeff(dfind, &i, -coeffs[i]);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_wlcalib_find_best_1d(dfind, dfind, observed, model,
                                     cpl_wlcalib_fill_line_spectrum,
                                     wl_search, nsamples, 0, &xcmax, NULL);
    cpl_test_eq_error(error, CPL_ERROR_DATA_NOT_FOUND);

    cpl_test_abs(xcmax, -1.0, 0.0);
    /* All fills failed */
    cpl_test_eq(xcost, cpl_wlcalib_slitmodel_get_xcost(model));
    cost = cpl_wlcalib_slitmodel_get_cost(model) - cost;
    cpl_test_leq(cost, ntests);

    /* Test 2. "Dummy" test: 1st guess is the correct solution */
    cost = cpl_wlcalib_slitmodel_get_cost(model);
    xcost = cpl_wlcalib_slitmodel_get_xcost(model);
    error = cpl_wlcalib_find_best_1d(dfind, dtrue, observed, model,
                                     cpl_wlcalib_fill_line_spectrum,
                                     wl_search, 2*nsamples, 0, &xcmax, NULL);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_msg_info(cpl_func, "Perfect 1st guess, XC: %g", xcmax);

    cpl_test_leq(xcmax, 1.0);
    cpl_test_leq(1.0 - FLT_EPSILON, xcmax);

    cpl_test_polynomial_abs(dfind, dtrue, FLT_EPSILON);

    error = cpl_polynomial_subtract(dfind, dfind, dtrue);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    error = cpl_polynomial_dump(dfind, stream);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cost = cpl_wlcalib_slitmodel_get_cost(model) - cost;
    xcost = cpl_wlcalib_slitmodel_get_xcost(model) - xcost;

    /* No failed fills during this test */
    cpl_test_eq(cost, xcost);
    cpl_test_eq(cost, ntests * (cpl_size)cpl_tools_ipow(2.0, ncoeffs));

    /* Test 3. 1st guess is an alteration of the correct solution */
    xcorrs = cpl_vector_new(nxc);
    cost = cpl_wlcalib_slitmodel_get_cost(model);
    xcost = cpl_wlcalib_slitmodel_get_xcost(model);
    tn0 = cpl_test_get_cputime();
    error = cpl_wlcalib_find_best_1d(dfind, dcand, observed, model,
                                     cpl_wlcalib_fill_line_spectrum, wl_search,
                                     nsamples, hsize, &xcmax, xcorrs);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    tn1 = cpl_test_get_cputime() - tn0;
    cpl_msg_info(cpl_func, "Altered 1st guess, XC: %g (%gsecs)", xcmax, tn1);

    cpl_test_abs(xcmax, cpl_vector_get_max(xcorrs), 0.0);
    cpl_test_leq(xcmax, 1.0);
    cpl_test_leq(0.98, xcmax);

    cpl_test_polynomial_abs(dfind, dtrue, 0.01);

    error = cpl_polynomial_subtract(dfind, dfind, dtrue);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_polynomial_dump(dfind, stream);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cost = cpl_wlcalib_slitmodel_get_cost(model) - cost;
    xcost = cpl_wlcalib_slitmodel_get_xcost(model) - xcost;

    /* No failed fills during this test */
    cpl_test_eq(xcost, cost);
    cpl_test_eq(cost, ntests);

    /* Test 4. Same, but try with fast spectrum generation */
    cost = cpl_wlcalib_slitmodel_get_cost(model);
    xcost = cpl_wlcalib_slitmodel_get_xcost(model);
    tf0 = cpl_test_get_cputime();
    error = cpl_wlcalib_find_best_1d(dfind, dcand, observed, model,
                                     cpl_wlcalib_fill_line_spectrum_fast,
                                     wl_search, nsamples, hsize, &xcmax,
                                     xcorrs);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    tf1 = cpl_test_get_cputime() - tf0;

    cpl_msg_info(cpl_func, "Fast spectrum generation, XC: %g (%gsecs)", xcmax,
                 tf1);

    cpl_test_abs(xcmax, cpl_vector_get_max(xcorrs), 0.0);
    cpl_test_leq(xcmax, 1.0);
    cpl_test_leq(0.98, xcmax);

    cpl_test_polynomial_abs(dfind, dtrue, 0.01);

    error = cpl_polynomial_subtract(dfind, dfind, dtrue);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_polynomial_dump(dfind, stream);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cost = cpl_wlcalib_slitmodel_get_cost(model) - cost;
    xcost = cpl_wlcalib_slitmodel_get_xcost(model) - xcost;

    /* No failed fills during this test */
    cpl_test_eq(xcost, cost);
    cpl_test_eq(cost, ntests);

    /* Test 5. Try the find using a shift of the true polynomial */

    /*  For a shifted polynomial we can do with a much smaller search range */
    error = cpl_vector_fill(wl_search, 0.1);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_polynomial_copy(dfind, dtrue);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_polynomial_shift_1d(dfind, 0, CPL_WLCALIB_SHIFT);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cost = cpl_wlcalib_slitmodel_get_cost(model);
    xcost = cpl_wlcalib_slitmodel_get_xcost(model);
    tn0 = cpl_test_get_cputime();
    error = cpl_wlcalib_find_best_1d(dfind, dfind, observed, model,
                                     cpl_wlcalib_fill_line_spectrum, wl_search,
                                     nsamples, hsize, &xcmax, xcorrs);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    tn1 = cpl_test_get_cputime() - tn0;

    cpl_msg_info(cpl_func, "Shifted " CPL_STRINGIFY(CPL_WLCALIB_SHIFT)
                 " pixel(s), XC: %g (%gsecs)", xcmax, tn1);

    cpl_test_abs(xcmax, cpl_vector_get_max(xcorrs), 0.0);
    cpl_test_leq(xcmax, 1.0);
    cpl_test_leq(0.999, xcmax);

    cpl_test_polynomial_abs(dfind, dtrue, 0.01);

    error = cpl_polynomial_subtract(dcand, dfind, dtrue);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_polynomial_dump(dcand, stream);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cost = cpl_wlcalib_slitmodel_get_cost(model) - cost;
    xcost = cpl_wlcalib_slitmodel_get_xcost(model) - xcost;

    /* No failed fills during this test */
    cpl_test_eq(xcost, cost);
    cpl_test_eq(cost, ntests);


    /* Test 6. Try to refine the solution */
    error = cpl_vector_fill(wl_sfine, 0.01);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_vector_fill(xcorrs, -1.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cost = cpl_wlcalib_slitmodel_get_cost(model);
    xcost = cpl_wlcalib_slitmodel_get_xcost(model);
    tn0 = cpl_test_get_cputime();
    error = cpl_wlcalib_find_best_1d(dfind, dfind, observed, model,
                                     cpl_wlcalib_fill_line_spectrum, wl_sfine,
                                     nsamples, 0, &xcmax, xcorrs);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    tn1 = cpl_test_get_cputime() - tn0;

    cpl_msg_info(cpl_func, "Refined search, XC: %g (%gsecs)", xcmax, tn1);

    cpl_test_abs(xcmax, cpl_vector_get_max(xcorrs), 0.0);
    cpl_test_leq(xcmax, 1.0);
    cpl_test_leq(0.9999, xcmax);

    cpl_test_polynomial_abs(dfind, dtrue, 0.005);

    error = cpl_polynomial_subtract(dcand, dfind, dtrue);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_polynomial_dump(dcand, stream);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cost = cpl_wlcalib_slitmodel_get_cost(model) - cost;
    xcost = cpl_wlcalib_slitmodel_get_xcost(model) - xcost;

    /* No failed fills during this test */
    cpl_test_eq(xcost, cost);
    cpl_test_eq(cost, ntests);


    /* Test 7. Same shift, but try with fast spectrum generation */
    error = cpl_polynomial_copy(dfind, dtrue);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_polynomial_shift_1d(dfind, 0, CPL_WLCALIB_SHIFT);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cost = cpl_wlcalib_slitmodel_get_cost(model);
    xcost = cpl_wlcalib_slitmodel_get_xcost(model);
    tf0 = cpl_test_get_cputime();
    error = cpl_wlcalib_find_best_1d(dfind, dfind, observed, model,
                                     cpl_wlcalib_fill_line_spectrum_fast,
                                     wl_search, nsamples, hsize, &xcmax, xcorrs);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    tf1 = cpl_test_get_cputime() - tf0;

    cpl_msg_info(cpl_func, "Shifted " CPL_STRINGIFY(CPL_WLCALIB_SHIFT)
                 " pixel(s) (fast), XC: %g (%gsecs)", xcmax, tf1);

    cpl_test_abs(xcmax, cpl_vector_get_max(xcorrs), 0.0);
    cpl_test_leq(xcmax, 1.0);
    cpl_test_leq(0.999, xcmax);

    cpl_test_polynomial_abs(dfind, dtrue, 0.01);

    error = cpl_polynomial_subtract(dcand, dfind, dtrue);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_polynomial_dump(dcand, stream);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cost = cpl_wlcalib_slitmodel_get_cost(model) - cost;
    xcost = cpl_wlcalib_slitmodel_get_xcost(model) - xcost;

    /* No failed fills during this test */
    cpl_test_eq(xcost, cost);
    cpl_test_eq(cost, ntests);

    /* Test 8. Try to refine the solution */
    error = cpl_vector_fill(xcorrs, -1.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cost = cpl_wlcalib_slitmodel_get_cost(model);
    xcost = cpl_wlcalib_slitmodel_get_xcost(model);
    tn0 = cpl_test_get_cputime();
    error = cpl_wlcalib_find_best_1d(dfind, dfind, observed, model,
                                     cpl_wlcalib_fill_line_spectrum_fast,
                                     wl_sfine, nsamples, 0, &xcmax, xcorrs);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    tn1 = cpl_test_get_cputime() - tn0;

    cpl_msg_info(cpl_func, "Refined fast search, XC: %g (%gsecs)", xcmax, tn1);

    cpl_test_abs(xcmax, cpl_vector_get_max(xcorrs), 0.0);
    cpl_test_leq(xcmax, 1.0);
    cpl_test_leq(0.9999, xcmax);

    cpl_test_polynomial_abs(dfind, dtrue, 0.001);

    error = cpl_polynomial_subtract(dcand, dfind, dtrue);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_polynomial_dump(dcand, stream);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cost = cpl_wlcalib_slitmodel_get_cost(model) - cost;
    xcost = cpl_wlcalib_slitmodel_get_xcost(model) - xcost;

    /* No failed fills during this test */
    cpl_test_eq(xcost, cost);
    cpl_test_eq(cost, ntests);

    /* Test 9. Revert the order of the wavelengths in the catalog */

    error = cpl_bivector_sort(lines, lines, CPL_SORT_DESCENDING, CPL_SORT_BY_X);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_vector_fill(xcorrs, -1.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    error = cpl_wlcalib_find_best_1d(dfind, dfind, observed, model,
                                     cpl_wlcalib_fill_line_spectrum_fast,
                                     wl_sfine, nsamples, 0, &xcmax, xcorrs);
    cpl_test_eq_error(error, CPL_ERROR_DATA_NOT_FOUND);

    cpl_test_abs(xcmax, cpl_vector_get_max(xcorrs), 0.0);
    cpl_test_leq(xcmax, -1.0);

    if (stream != stdout) cpl_test_zero( fclose(stream) );

    cpl_wlcalib_slitmodel_delete(model);
    cpl_polynomial_delete(dtrue);
    cpl_polynomial_delete(dfind);
    cpl_polynomial_delete(dcand);
    cpl_vector_delete(observed);
    cpl_vector_delete(obsfast);
    cpl_vector_delete(wl_search);
    cpl_vector_delete(wl_sfine);
    cpl_vector_delete(xcorrs);

}
