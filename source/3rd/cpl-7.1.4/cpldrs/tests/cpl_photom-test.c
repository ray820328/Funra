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

#include <stdio.h>

#include <cpl_plot.h>
#include <cpl_test.h>

#include "cpl_polynomial.h"
#include "cpl_bivector.h"
#include "cpl_photom.h"
#include "cpl_type.h"
#include "cpl_tools.h"
#include "cpl_memory.h"

/*-----------------------------------------------------------------------------
                                   Defines
 -----------------------------------------------------------------------------*/

#ifndef FUNCTION_SIZE
#define FUNCTION_SIZE  1024
#endif

/* A telescope main mirror could for example have a temperature of 12C */
#define TEMP_BB 285

/* Wiens displacement in frequency domain [Hz/K] */
/* FIXME: Document and export */
#define CPL_PHYS_Wien_Freq 5.879e10


/* The relevant range for black-body radiation is about [2;32[ micron */
#define WL_MIN  2e-6
#define WL_MAX 32e-6

/*-----------------------------------------------------------------------------
                        Private function prototypes
 -----------------------------------------------------------------------------*/

static void cpl_photom_units_test(void);

static void cpl_photom_fill_blackbody_test(cpl_vector *, cpl_unit,
                                           const cpl_vector *,
                                           cpl_unit, double, unsigned);

/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/
int main(void)
{
    cpl_polynomial * poly1 ;
    cpl_bivector   * fun ;
    cpl_bivector   * trans;
    FILE           * ftrans;
    const double     temp = TEMP_BB;
    cpl_size         i;
    const cpl_size   half_search = FUNCTION_SIZE/2;
    cpl_vector     * vxc;
    cpl_error_code   error;
    double         * wlf;


    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    /* Insert tests below */

    cpl_msg_info("", CPL_STRINGIFY(TEMP_BB) "K Black-Body radiation peaks at "
                 "[m]: %g", CPL_PHYS_Wien/temp);
    cpl_msg_info("", CPL_STRINGIFY(TEMP_BB) "K Black-Body radiation peaks at "
                 "[Hz]: %g", CPL_PHYS_Wien_Freq * temp);

    cpl_photom_units_test();

    poly1 = cpl_polynomial_new(1);
    cpl_test_nonnull( poly1 );

    i = 0;
    error = cpl_polynomial_set_coeff(poly1, &i, WL_MIN);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    i++;
    error = cpl_polynomial_set_coeff(poly1, &i, (WL_MAX-WL_MIN)/FUNCTION_SIZE);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    fun = cpl_bivector_new(FUNCTION_SIZE);
    cpl_test_nonnull( fun );

    error = cpl_vector_fill_polynomial(cpl_bivector_get_x(fun),
                                       poly1, 0, 1);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_photom_fill_blackbody_test(cpl_bivector_get_y(fun),
                                   CPL_UNIT_PHOTONRADIANCE,
                                   cpl_bivector_get_x(fun),
                                   CPL_UNIT_LENGTH, temp, __LINE__);

    ftrans = fopen("planck1.txt", "w");
    cpl_test_nonnull( ftrans );
    cpl_bivector_dump(fun, ftrans);
    cpl_test_zero( fclose(ftrans) );

    cpl_photom_fill_blackbody_test(cpl_bivector_get_y(fun),
                                   CPL_UNIT_ENERGYRADIANCE,
                                   cpl_bivector_get_x_const(fun),
                                   CPL_UNIT_LENGTH, temp, __LINE__);

    trans = cpl_bivector_new(FUNCTION_SIZE);
    cpl_test_nonnull( trans );

    error = cpl_vector_copy(cpl_bivector_get_x(trans),
                            cpl_bivector_get_x_const(fun));
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    cpl_photom_fill_blackbody_test(cpl_bivector_get_y(trans),
                                   CPL_UNIT_LESS,
                                   cpl_bivector_get_x_const(trans),
                                   CPL_UNIT_LENGTH, temp, __LINE__);

    vxc = cpl_vector_new(2*half_search + 1);

    cpl_test_eq( cpl_vector_correlate(vxc, cpl_bivector_get_y_const(trans),
                                      cpl_bivector_get_y_const(fun)),
                 half_search );
    cpl_test_error(CPL_ERROR_NONE);

    cpl_test_abs(cpl_vector_get(vxc, half_search), 1.0,
                 FUNCTION_SIZE * DBL_EPSILON);

    ftrans = fopen("planck2.txt", "w");
    cpl_test_nonnull( ftrans );
    cpl_bivector_dump(trans, ftrans);
    cpl_test_zero( fclose(ftrans) );

    if (cpl_msg_get_level() <= CPL_MSG_DEBUG) {
        const char * options[] = {"t '" CPL_STRINGIFY(TEMP_BB) "K black body "
                                  "radiance' w points;",
                                  "t '" CPL_STRINGIFY(TEMP_BB) "K black body "
                                  "radiance peak';"};
        cpl_bivector *pair[] = {cpl_bivector_new(1), trans};

        error = cpl_vector_set(cpl_bivector_get_x(pair[0]), 0,
                               CPL_PHYS_Wien/temp);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        cpl_photom_fill_blackbody_test(cpl_bivector_get_y(pair[0]),
                                       CPL_UNIT_LESS,
                                       cpl_bivector_get_x_const(pair[0]),
                                       CPL_UNIT_LENGTH, temp, __LINE__);

        error = cpl_plot_bivectors("set xlabel 'Wavelength [m]';"
                                   "set ylabel 'Unit less';",
                                   options, "", (const cpl_bivector**)pair, 2);
        cpl_test_eq_error(error, CPL_ERROR_NONE);
        cpl_bivector_delete(pair[0]);
    }

    /* Fill also in frequency mode */
    wlf = cpl_bivector_get_x_data(fun);
    for (i = 0; i < FUNCTION_SIZE; i++) {
        wlf[i] = CPL_PHYS_C / wlf[i];
    }

    cpl_photom_fill_blackbody_test(cpl_bivector_get_y(fun),
                                   CPL_UNIT_PHOTONRADIANCE,
                                   cpl_bivector_get_x(fun),
                                   CPL_UNIT_FREQUENCY, temp, __LINE__);

    cpl_photom_fill_blackbody_test(cpl_bivector_get_y(fun),
                                   CPL_UNIT_ENERGYRADIANCE,
                                   cpl_bivector_get_x_const(fun),
                                   CPL_UNIT_FREQUENCY, temp, __LINE__);

    if (cpl_msg_get_level() <= CPL_MSG_DEBUG) {
        const char * options[] = {"t '" CPL_STRINGIFY(TEMP_BB) "K black body "
                                  "radiance' w points;",
                                  "t '" CPL_STRINGIFY(TEMP_BB) "K black body "
                                  "radiance peak';"};
        cpl_bivector *pair[] = {cpl_bivector_new(1), fun};

        error = cpl_vector_set(cpl_bivector_get_x(pair[0]), 0,
                               CPL_PHYS_Wien_Freq * temp);
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        cpl_photom_fill_blackbody_test(cpl_bivector_get_y(pair[0]),
                                       CPL_UNIT_ENERGYRADIANCE,
                                       cpl_bivector_get_x_const(pair[0]),
                                       CPL_UNIT_FREQUENCY, temp, __LINE__);

        error = cpl_plot_bivectors("set xlabel 'Frequency [hz]';"
                                   "set ylabel 'Energy Radiance';",
                                   options, "", (const cpl_bivector**)pair, 2);
        cpl_test_eq_error(error, CPL_ERROR_NONE);
        cpl_bivector_delete(pair[0]);
    }


    /* Error testing */
    error = cpl_photom_fill_blackbody(NULL,
                                      CPL_UNIT_LESS,
                                      cpl_bivector_get_x_const(trans),
                                      CPL_UNIT_LENGTH, temp);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

    error = cpl_photom_fill_blackbody(cpl_bivector_get_y(trans),
                                      CPL_UNIT_LESS,
                                      NULL,
                                      CPL_UNIT_LENGTH, temp);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

    error = cpl_photom_fill_blackbody(vxc,
                                      CPL_UNIT_LESS,
                                      cpl_bivector_get_x_const(trans),
                                      CPL_UNIT_LENGTH, temp);
    cpl_test_eq_error(error, CPL_ERROR_INCOMPATIBLE_INPUT);

    error = cpl_photom_fill_blackbody(cpl_bivector_get_y(trans),
                                      CPL_UNIT_LESS,
                                      cpl_bivector_get_x_const(trans),
                                      CPL_UNIT_LENGTH, 0.0);
    cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

    error = cpl_photom_fill_blackbody(cpl_bivector_get_y(trans),
                                      CPL_UNIT_LESS,
                                      cpl_bivector_get_x_const(trans),
                                      CPL_UNIT_FREQUENCY, temp);
    cpl_test_eq_error(error, CPL_ERROR_UNSUPPORTED_MODE);

    error = cpl_photom_fill_blackbody(cpl_bivector_get_y(trans),
                                      CPL_UNIT_LESS,
                                      cpl_bivector_get_x_const(trans),
                                      CPL_UNIT_FREQUENCY, temp);
    cpl_test_eq_error(error, CPL_ERROR_UNSUPPORTED_MODE);

    error = cpl_photom_fill_blackbody(cpl_bivector_get_y(trans),
                                      CPL_UNIT_LESS,
                                      cpl_bivector_get_x_const(trans),
                                      CPL_UNIT_LESS, temp);
    cpl_test_eq_error(error, CPL_ERROR_UNSUPPORTED_MODE);

    error = cpl_photom_fill_blackbody(cpl_bivector_get_y(trans),
                                      CPL_UNIT_LENGTH,
                                      cpl_bivector_get_x_const(trans),
                                      CPL_UNIT_ENERGYRADIANCE, temp);
    cpl_test_eq_error(error, CPL_ERROR_UNSUPPORTED_MODE);

    error = cpl_vector_set(cpl_bivector_get_x(trans), 0, 0.0);
    cpl_test_error(CPL_ERROR_NONE);
    error = cpl_photom_fill_blackbody(cpl_bivector_get_y(trans),
                                      CPL_UNIT_LESS,
                                      cpl_bivector_get_x_const(trans),
                                      CPL_UNIT_LENGTH, temp);
    cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);

    cpl_vector_delete(vxc);
    cpl_polynomial_delete(poly1);
    cpl_bivector_delete(fun);
    cpl_bivector_delete(trans);

    return cpl_test_end(0);
}


/*----------------------------------------------------------------------------*/
/**
  @brief    Test the CPL units
  @return   void
 */
/*----------------------------------------------------------------------------*/
static void cpl_photom_units_test(void)
{

    const cpl_unit units[] = {     CPL_UNIT_LESS,
                                   CPL_UNIT_RADIAN,
                                   CPL_UNIT_LENGTH,
                                   CPL_UNIT_TIME,
                                   CPL_UNIT_PERLENGTH,
                                   CPL_UNIT_FREQUENCY,
                                   CPL_UNIT_MASS,
                                   CPL_UNIT_ACCELERATION,
                                   CPL_UNIT_FORCE,
                                   CPL_UNIT_ENERGY,
                                   CPL_UNIT_PHOTONRADIANCE,
                                   CPL_UNIT_ENERGYRADIANCE};

    const unsigned nunits = sizeof(units)/sizeof(units[0]);
    unsigned i, j;

    /* No-unit must be the identity element with respect to multiplication */
    cpl_test_eq(CPL_UNIT_LESS, 1);

    /* Verify uniqueness of units */
    for (i = 0; i < nunits; i++) {
        for (j = 0; j < nunits; j++) {
            if (i == j) continue;
            cpl_test_noneq(units[i], units[j]);
        }
    }

    /* Reciprocal units do _not_ cancel out :-( */
    cpl_test_noneq(CPL_UNIT_TIME * CPL_UNIT_FREQUENCY, CPL_UNIT_LESS);
    cpl_test_noneq(CPL_UNIT_LENGTH * CPL_UNIT_PERLENGTH, CPL_UNIT_LESS);

    /* Derived quantities */
    cpl_test_eq(CPL_UNIT_ACCELERATION,
                CPL_UNIT_LENGTH * CPL_UNIT_FREQUENCY * CPL_UNIT_FREQUENCY);

    cpl_test_eq(CPL_UNIT_FORCE, CPL_UNIT_MASS * CPL_UNIT_ACCELERATION);

    cpl_test_eq(CPL_UNIT_ENERGY, CPL_UNIT_LENGTH * CPL_UNIT_FORCE);

    cpl_test_eq(CPL_UNIT_PHOTONRADIANCE, CPL_UNIT_RADIAN * CPL_UNIT_FREQUENCY
                * CPL_UNIT_PERLENGTH * CPL_UNIT_PERLENGTH * CPL_UNIT_PERLENGTH);
    cpl_test_eq(CPL_UNIT_ENERGYRADIANCE, CPL_UNIT_ENERGY * CPL_UNIT_PHOTONRADIANCE);

    return;

}

/*----------------------------------------------------------------------------*/
/**
   @internal
   @brief    Fill a spectrum using valid input and verify it
   @param    spectrum    Preallocated, the computed radiance
   @param    out_unit    CPL_UNIT_PHOTONRADIANCE, CPL_UNIT_ENERGYRADIANCE or
   CPL_UNIT_LESS
   @param    evalpoints  The evaluation points (wavelengths or frequencies)
   @param    in_unit     CPL_UNIT_LENGTH or CPL_UNIT_FREQUENCY
   @param    temp        The black body temperature [K]
   @param    line        __LINE__
   @return   void
   @see cpl_photom_fill_blackbody()

   evalpoints must be strictly monotone, increasing for wavelengths,
   decreasing for frequencies.

*/
/*----------------------------------------------------------------------------*/
static void cpl_photom_fill_blackbody_test(cpl_vector * spectrum,
                                           cpl_unit out_unit,
                                           const cpl_vector * evalpoints,
                                           cpl_unit in_unit,
                                           double temp, unsigned line)
{

    const cpl_size       fsize = cpl_vector_get_size(spectrum);
    const cpl_error_code error = cpl_photom_fill_blackbody(spectrum, out_unit,
                                                           evalpoints, in_unit,
                                                           temp);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    if (error != CPL_ERROR_NONE) {
        cpl_msg_error(cpl_func, "Failure from line %u", line);
    } else if (out_unit == CPL_UNIT_ENERGYRADIANCE) {
        /* FIXME: Test also CPL_UNIT_PHOTONRADIANCE */

        const double * sp = cpl_vector_get_data_const(spectrum);
        const double * wl = cpl_vector_get_data_const(evalpoints);
        cpl_size       i;

        if (in_unit == CPL_UNIT_LENGTH) {
            const double xtpt = CPL_PHYS_Wien/temp; /* wavelength */

            cpl_test_lt(0.0, sp[0]);
            for (i = 1; i < fsize; i++) {

                /* Wavelengths must increase */
                cpl_test_lt(wl[i-1], wl[i]);

                if (wl[i] < xtpt) {
                    /* Must be monotonely increasing */
                    cpl_test_lt(sp[i-1], sp[i]);
                } else if (i < fsize - 1) {
                    /* Must be monotonely decreasing */
                    cpl_test_lt(sp[i+1], sp[i]);
                }
            }
        } else if (in_unit == CPL_UNIT_FREQUENCY) {

            const double xtpt = temp * CPL_PHYS_Wien_Freq; /* Frequency */

            cpl_test_lt(0.0, sp[0]);
            for (i = 1; i < fsize; i++) {

                /* Frequencies must increase */
                cpl_test_lt(wl[i], wl[i-1]);

                if (xtpt < wl[i]) {
                    /* Must be monotonely increasing */
                    cpl_test_lt(sp[i-1], sp[i]);
                } else if (i < fsize - 1) {
                    /* Must be monotonely decreasing */
                    cpl_test_lt(sp[i+1], sp[i]);
                }
            }
        }
    }

    return;
}
