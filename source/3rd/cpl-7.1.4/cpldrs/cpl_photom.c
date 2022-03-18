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

#include "cpl_photom.h"

#include "cpl_error_impl.h"
#include "cpl_tools.h"
#include "cpl_math_const.h"

#include <stdlib.h>
#include <math.h>
#include <assert.h>

/*----------------------------------------------------------------------------*/
/**
 * @defgroup cpl_photom High-level functions that are photometry related 
 *
 * @par Synopsis:
 * @code
 *   #include "cpl_photom.h"
 * @endcode
 */
/*----------------------------------------------------------------------------*/
/**@{*/

/*-----------------------------------------------------------------------------
                              Function codes
 -----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/**
  @brief    The Planck radiance from a black-body
  @param    spectrum    Preallocated, the computed radiance
  @param    out_unit    CPL_UNIT_PHOTONRADIANCE, CPL_UNIT_ENERGYRADIANCE or
                        CPL_UNIT_LESS
  @param    evalpoints  The evaluation points (wavelengths or frequencies)
  @param    in_unit     CPL_UNIT_LENGTH or CPL_UNIT_FREQUENCY
  @param    temp        The black body temperature [K]
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_

  The Planck black-body radiance can be computed in 5 different ways:
  As a radiance of either energy [J*radian/s/m^3] or photons [radian/s/m^3],
  and in terms of either wavelength [m] or frequency [1/s]. The fifth way is
  as a unit-less radiance in terms of wavelength, in which case the area under
  the planck curve is 1.

  The dimension of the spectrum (energy or photons or unit-less, CPL_UNIT_LESS)
  is controlled by out_unit, and the dimension of the input (length or
  frequency) is controlled by in_unit.

  evalpoints and spectrum must be of equal, positive length.

  The input wavelengths/frequencies and the temperature must be positive.

  The four different radiance formulas are:
  Rph1(l,T) = 2pi c/l^4/(exp(hc/klT)-1)
  Rph2(f,T) = 2pi f^2/c^2/(exp(hf/kT)-1)
  Re1(l,T) = 2pi hc^2/l^5/(exp(hc/klT)-1) = Rph1(l,T) * hc/l
  Re2(f,T) = 2pi hf^3/c^2/(exp(hf/kT)-1)  = Rph2(f,T) * hf
  R1(l,T)  = 15h^5c^5/(pi^4k^5l^5T^5/(exp(hc/klT)-1)
           = Rph1(l,T) * h^4c^3/(2pi^5k^5T^5)
  
  where l is the wavelength, f is the frequency, T is the temperature,
  h is the Planck constant, k is the Boltzmann constant and
  c is the speed of light in vacuum.

  When the radiance is computed in terms of wavelength, the radiance peaks
  at l_max = CPL_PHYS_Wien/temp. When the radiance is unit-less this maximum,
  R1(l_max,T), is approximately 3.2648. R1(l,T) integrated over l from 0 to
  infinity is 1.

  A unit-less black-body radiance in terms of frequency may be added later,
  until then it is an error to combine CPL_UNIT_LESS and CPL_UNIT_FREQUENCY.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INCOMPATIBLE_INPUT if the size of evalpoints is different from
    the size of spectrum
  - CPL_ERROR_UNSUPPORTED_MODE if in_unit and out_unit are not as requested
  - CPL_ERROR_ILLEGAL_INPUT if temp or a wavelength is non-positive
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_photom_fill_blackbody(cpl_vector * spectrum,
                                         cpl_unit out_unit,
                                         const cpl_vector * evalpoints,
                                         cpl_unit in_unit,
                                         double temp)
{

    double         walpha = CPL_MATH_2PI;
    double         wbeta  = CPL_PHYS_H / (CPL_PHYS_K * temp);
    double       * sp     = cpl_vector_get_data( spectrum );
    const double * wlf    = cpl_vector_get_data_const( evalpoints );
    const cpl_size n      = cpl_vector_get_size( evalpoints );
    int            ipow;
    cpl_size       i;


    cpl_ensure_code(spectrum   != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(evalpoints != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(n == cpl_vector_get_size( spectrum ),
                    CPL_ERROR_INCOMPATIBLE_INPUT);
    cpl_ensure_code(temp > 0.0,         CPL_ERROR_ILLEGAL_INPUT);

    assert( wlf != NULL );
    assert( sp  != NULL );

    switch (in_unit) {
        case CPL_UNIT_LENGTH: {
            switch (out_unit) {
            case CPL_UNIT_LESS: {
                walpha *= 7.5 * cpl_tools_ipow(CPL_PHYS_H * CPL_PHYS_C
                                               /(CPL_MATH_PI*CPL_PHYS_K * temp),
                                               5);
                ipow    = 5;
                break;
            }
            case CPL_UNIT_PHOTONRADIANCE: {
                walpha *= CPL_PHYS_C;
                ipow    = 4;
                break;
            }
            case CPL_UNIT_ENERGYRADIANCE: {
                walpha *= CPL_PHYS_H * CPL_PHYS_C * CPL_PHYS_C;
                ipow    = 5;
                break;
            }
            default:
                return cpl_error_set_(CPL_ERROR_UNSUPPORTED_MODE);
            }

            wbeta  *= CPL_PHYS_C;

            for (i=0; i < n; i++) {
                if (wlf[i] <= 0.0)
                    return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
                sp[i] = walpha / (cpl_tools_ipow(wlf[i], ipow)
                                  * (expm1(wbeta / wlf[i])));
            }

            break;
        }

        case CPL_UNIT_FREQUENCY: {
            switch (out_unit) {
            case CPL_UNIT_PHOTONRADIANCE: {
                walpha /= CPL_PHYS_C * CPL_PHYS_C;
                ipow    = 2;
                break;
            }
            case CPL_UNIT_ENERGYRADIANCE: {
                walpha *= CPL_PHYS_H / (CPL_PHYS_C * CPL_PHYS_C);
                ipow    = 3;
                break;
            }
            default:
                return cpl_error_set_(CPL_ERROR_UNSUPPORTED_MODE);
            }

            for (i=0; i < n; i++) {
                if (wlf[i] <= 0.0)
                    return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
                sp[i] = walpha * cpl_tools_ipow(wlf[i], ipow)
                    / (expm1(wbeta * wlf[i]));
            }

            break;
        }
        default:
            return cpl_error_set_(CPL_ERROR_UNSUPPORTED_MODE);
    }

    return CPL_ERROR_NONE;

}
/**@}*/
