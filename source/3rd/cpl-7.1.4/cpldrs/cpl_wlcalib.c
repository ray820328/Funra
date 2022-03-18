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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include "cpl_wlcalib_impl.h"

#include <cpl_errorstate.h>
#include <cpl_polynomial_impl.h>
#include <cpl_error_impl.h>
#include <cpl_memory.h>
#include <cpl_vector_impl.h>
#include <cpl_bivector.h>
#include <cpl_math_const.h>

#ifdef CPL_WLCALIB_DEBUG
#include <cpl_msg.h>
#endif

#include <math.h>
#include <string.h>

/*----------------------------------------------------------------------------*/
/**
 * @defgroup cpl_wlcalib  Wavelength calibration
 *
 * This module contains functions to perform 1D-wavelength calibration,
 * typically of long-slit spectroscopy data.
 *
 * @par Synopsis:
 * @code
 *   #include "cpl_wlcalib.h"
 * @endcode
 */
/*----------------------------------------------------------------------------*/
/**@{*/


/*-----------------------------------------------------------------------------
                               Defines
 -----------------------------------------------------------------------------*/

#ifndef inline
#define inline /* inline */
#endif

/*-----------------------------------------------------------------------------
                               New Types
 -----------------------------------------------------------------------------*/

struct cpl_wlcalib_slitmodel_ {
    cpl_size             cost;    /* May be incremented for cost counting */
    cpl_size             xcost;   /* Ditto (can exclude failed fills) */
    cpl_size             ulines;  /* May be set to number of lines used */

    double               wslit;  /* Slit Width */
    double               wfwhm;  /* FWHM of transfer function */
    double               lthres; /* Line truncation threshold */
    cpl_bivector       * lines;  /* Catalog of intensities, with
                                    increasing X-vector elements */

    /* Private members, filled automatically */
    cpl_vector         * linepix;  /* Catalog of line pixel positions
                                      - zero for uninitialized */
    cpl_vector         * erftmp;  /* Temporary storage for erf() values
                                      - zero for uninitialized */
};



/*-----------------------------------------------------------------------------
                             Private Function Prototypes
 -----------------------------------------------------------------------------*/
inline static double cpl_erf_antideriv(double x, double);

static
cpl_error_code cpl_wlcalib_fill_line_spectrum_model(cpl_vector *,
                                                    cpl_vector *,
                                                    cpl_vector **,
                                                    const cpl_polynomial *,
                                                    const cpl_bivector *,
                                                    double,
                                                    double,
                                                    double,
                                                    cpl_size,
                                                    cpl_boolean,
                                                    cpl_boolean,
                                                    cpl_size *);

static
cpl_error_code cpl_wlcalib_evaluate(cpl_vector *,
                                    cpl_size *,
                                    cpl_vector *,
                                    const cpl_vector *,
                                    void             *,
                                    cpl_error_code  (*)(cpl_vector *, void *,
                                                        const cpl_polynomial *),
                                    const cpl_polynomial *);

/*-----------------------------------------------------------------------------
                                   Function Prototypes
 -----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/**
  @brief    Create a new line model to be initialized
  @return   1 newly allocated cpl_wlcalib_slitmodel
  @note All elements are initialized to either zero or NULL.
  @see cpl_wlcalib_slitmodel_delete() for object deallocation.

  The model comprises these elements:
    Slit Width
    FWHM of transfer function
    Truncation threshold of the transfer function
    Catalog of lines (typically arc or sky)

  The units of the X-values of the lines is a length, it is assumed to be the
  same as that of the Y-values of the dispersion relation (e.g. meter), the
  units of slit width and the FWHM are assumed to be the same as the X-values
  of the dispersion relation (e.g. pixel), while the units of the produced
  spectrum will be that of the Y-values of the lines.

 */
/*----------------------------------------------------------------------------*/
cpl_wlcalib_slitmodel * cpl_wlcalib_slitmodel_new(void)
{

    return
        (cpl_wlcalib_slitmodel*)cpl_calloc(1, sizeof(cpl_wlcalib_slitmodel));

}


/*----------------------------------------------------------------------------*/
/**
  @brief    Free memory associated with a cpl_wlcalib_slitmodel object.
  @param    self    The cpl_wlcalib_slitmodel object or @em NULL
  @return   Nothing
  @see cpl_wlcalib_slitmodel_new()
  @note If @em self is @c NULL nothing is done and no error is set.

 */
/*----------------------------------------------------------------------------*/
void cpl_wlcalib_slitmodel_delete(cpl_wlcalib_slitmodel * self)
{

    if (self != NULL) {

        cpl_bivector_delete(self->lines);
        cpl_vector_delete(self->linepix);
        cpl_vector_delete(self->erftmp);

        cpl_free(self);
    }

    return;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Increment the cost counting of a cpl_wlcalib_slitmodel object
  @param  self   The cpl_wlcalib_slitmodel object or @em NULL
  @param  value  The increment
  @return CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error
  @see cpl_wlcalib_slitmodel_new()
  @note A spectrum filler function can use this function to increment the number
    of times it has been invoked.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is @em NULL
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_wlcalib_slitmodel_add_cost(cpl_wlcalib_slitmodel * self,
                                              cpl_size value)
{

    cpl_ensure_code(self  != NULL, CPL_ERROR_NULL_INPUT);

    self->cost += value;

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Increment the exclusive cost counting of a cpl_wlcalib_slitmodel object
  @param  self   The cpl_wlcalib_slitmodel object or @em NULL
  @param  value  The increment
  @return CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error
  @see cpl_wlcalib_slitmodel_new()
  @note A spectrum filler function can use this function to increment the number
    of times it has been invoked. The increment can be omitted if the fill fails.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is @em NULL
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_wlcalib_slitmodel_add_xcost(cpl_wlcalib_slitmodel * self,
                                              cpl_size value)
{

    cpl_ensure_code(self  != NULL, CPL_ERROR_NULL_INPUT);

    self->xcost += value;

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Increment the line cost counting of a cpl_wlcalib_slitmodel object
  @param  self   The cpl_wlcalib_slitmodel object or @em NULL
  @param  value  The increment
  @return CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error
  @see cpl_wlcalib_slitmodel_new()
  @note A spectrum filler function can increment this counter with the number
  of catalog lines used to provide a cost measure.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is @em NULL
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_wlcalib_slitmodel_add_ulines(cpl_wlcalib_slitmodel * self,
                                              cpl_size value)
{

    cpl_ensure_code(self  != NULL, CPL_ERROR_NULL_INPUT);

    self->ulines += value;

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief  Set the slit width to be used by the spectrum filler
  @param  self   The cpl_wlcalib_slitmodel object
  @param  value  The (positive) width of the slit
  @return CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error
  @see cpl_wlcalib_slitmodel_new()

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is @em NULL
  - CPL_ERROR_ILLEGAL_INPUT the value is non-positive
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_wlcalib_slitmodel_set_wslit(cpl_wlcalib_slitmodel * self,
                                               double value)
{

    cpl_ensure_code(self  != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(value > 0.0,   CPL_ERROR_ILLEGAL_INPUT);

    self->wslit = value;

    return CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
  @brief  Set the FWHM of the transfer function to be used by the spectrum filler
  @param  self   The cpl_wlcalib_slitmodel object
  @param  value  The (positive) FWHM
  @return CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error
  @see cpl_wlcalib_slitmodel_new()

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is @em NULL
  - CPL_ERROR_ILLEGAL_INPUT the value is non-positive
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_wlcalib_slitmodel_set_wfwhm(cpl_wlcalib_slitmodel * self,
                                               double value)
{

    cpl_ensure_code(self  != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(value > 0.0,   CPL_ERROR_ILLEGAL_INPUT);

    self->wfwhm = value;

    return CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
  @brief  The (positive) threshold for truncating the transfer function
  @param  self   The cpl_wlcalib_slitmodel object
  @param  value  The (non-negative) truncation threshold, 5 is a good value.
  @return CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error
  @see cpl_wlcalib_slitmodel_new()
  @note The threshold should be high enough to ensure a good line profile,
        but not too high to make the spectrum generation too costly.

  The line profile is truncated at this distance [pixel] from its maximum:
  \f$x_{max} = w/2 + k * \sigma,\f$ where
  \f$w\f$ is the slit width and \f$\sigma = w_{FWHM}/(2\sqrt(2\log(2))),\f$
  where \f$w_{FWHM}\f$ is the Full Width at Half Maximum (FWHM) of the
  transfer function and \f$k\f$ is the user supplied value.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is @em NULL
  - CPL_ERROR_ILLEGAL_INPUT the value is negative
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_wlcalib_slitmodel_set_threshold(cpl_wlcalib_slitmodel * self,
                                                   double value)
{

    cpl_ensure_code(self  != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(value >= 0.0,  CPL_ERROR_ILLEGAL_INPUT);

    self->lthres = value;

    return CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
  @brief  Set the catalog of lines to be used by the spectrum filler
  @param  self    The cpl_wlcalib_slitmodel object
  @param  catalog The catalog of lines (e.g. arc lines)
  @return CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error
  @see cpl_wlcalib_slitmodel_new()
  @note The values in the X-vector must be increasing. Any previously set
  catalog is deallocated

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is @em NULL
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_wlcalib_slitmodel_set_catalog(cpl_wlcalib_slitmodel * self,
                                                 cpl_bivector * catalog)
{

    cpl_ensure_code(self    != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(catalog != NULL, CPL_ERROR_NULL_INPUT);

    cpl_bivector_delete(self->lines);

    self->lines = catalog;

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Get the slit width of the cpl_wlcalib_slitmodel object
  @param  self   The cpl_wlcalib_slitmodel object
  @return The slit width
  @see cpl_wlcalib_slitmodel_set_wslit()

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is @em NULL
 */
/*----------------------------------------------------------------------------*/
double cpl_wlcalib_slitmodel_get_wslit(const cpl_wlcalib_slitmodel * self)
{

    cpl_ensure(self != NULL, CPL_ERROR_NULL_INPUT, 0.0);

    return self->wslit;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Get the FWHM of the cpl_wlcalib_slitmodel object
  @param  self   The cpl_wlcalib_slitmodel object
  @return The FWHM
  @see cpl_wlcalib_slitmodel_set_wfwhm()

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is @em NULL
 */
/*----------------------------------------------------------------------------*/
double cpl_wlcalib_slitmodel_get_wfwhm(const cpl_wlcalib_slitmodel * self)
{

    cpl_ensure(self != NULL, CPL_ERROR_NULL_INPUT, 0.0);

    return self->wfwhm;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Get the truncating threshold of the cpl_wlcalib_slitmodel object
  @param  self   The cpl_wlcalib_slitmodel object
  @return The truncating threshold for the transfer function
  @see cpl_wlcalib_slitmodel_set_threshold()

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is @em NULL
 */
/*----------------------------------------------------------------------------*/
double cpl_wlcalib_slitmodel_get_threshold(const cpl_wlcalib_slitmodel * self)
{

    cpl_ensure(self != NULL, CPL_ERROR_NULL_INPUT, 0.0);

    return self->lthres;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Get the lines catalog of the cpl_wlcalib_slitmodel object
  @param  self   The cpl_wlcalib_slitmodel object
  @return The lines catalog
  @see cpl_wlcalib_slitmodel_set_catalog()

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is @em NULL
 */
/*----------------------------------------------------------------------------*/
cpl_bivector * cpl_wlcalib_slitmodel_get_catalog(cpl_wlcalib_slitmodel * self)
{

    cpl_ensure(self != NULL, CPL_ERROR_NULL_INPUT, NULL);

    return self->lines;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Get the lines catalog of the cpl_wlcalib_slitmodel object
  @param  self   The cpl_wlcalib_slitmodel object
  @return The lines catalog
  @see cpl_wlcalib_slitmodel_set_catalog()

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is @em NULL
 */
/*----------------------------------------------------------------------------*/
const cpl_bivector *
cpl_wlcalib_slitmodel_get_catalog_const(const cpl_wlcalib_slitmodel * self)
{

    cpl_ensure(self != NULL, CPL_ERROR_NULL_INPUT, NULL);

    return self->lines;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Remove the lines catalog from the cpl_wlcalib_slitmodel object
  @param  self   The cpl_wlcalib_slitmodel object
  @return The lines catalog
  @see cpl_wlcalib_slitmodel_set_catalog()

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is @em NULL
 */
/*----------------------------------------------------------------------------*/
cpl_bivector * cpl_wlcalib_slitmodel_unset_catalog(cpl_wlcalib_slitmodel * self)
{

    cpl_bivector * lines;

    cpl_ensure(self != NULL, CPL_ERROR_NULL_INPUT, NULL);

    lines = self->lines;
    self->lines = NULL;

    return lines;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Get the accumulated cost of the cpl_wlcalib_slitmodel object usage
  @param  self   The cpl_wlcalib_slitmodel object
  @return The accumulated cost
  @see cpl_wlcalib_slitmodel_new()

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is @em NULL
 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_wlcalib_slitmodel_get_cost(const cpl_wlcalib_slitmodel * self)
{
    cpl_ensure(self != NULL, CPL_ERROR_NULL_INPUT, 0);

    return self->cost;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Get the accumulated exclusive cost of the cpl_wlcalib_slitmodel
          object usage
  @param  self   The cpl_wlcalib_slitmodel object
  @return The accumulated exclusive cost
  @see cpl_wlcalib_slitmodel_new()

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is @em NULL
 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_wlcalib_slitmodel_get_xcost(const cpl_wlcalib_slitmodel * self)
{
    cpl_ensure(self != NULL, CPL_ERROR_NULL_INPUT, 0);

    return self->xcost;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Get the accumulated line cost of the cpl_wlcalib_slitmodel
          object usage
  @param  self   The cpl_wlcalib_slitmodel object
  @return The accumulated line cost
  @see cpl_wlcalib_slitmodel_new()

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is @em NULL
 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_wlcalib_slitmodel_get_ulines(const cpl_wlcalib_slitmodel * self)
{
    cpl_ensure(self != NULL, CPL_ERROR_NULL_INPUT, 0);

    return self->ulines;
}


/*----------------------------------------------------------------------------*/
/**
  @brief  Generate a 1D spectrum from a model and a dispersion relation
  @param  self   Vector to fill with spectrum
  @param  model  Pointer to cpl_wlcalib_slitmodel object
  @param  disp   1D-Dispersion relation, at least of degree 1
  @return CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error
  @note The model is passed as a @em void pointer so the function can be used
        with cpl_wlcalib_find_best_1d().
  @see cpl_wlcalib_find_best_1d()


  The fill a vector with a spectrum, one must first initialize the parameters
  of the model (error checks omitted for brevity):

  @code
    cpl_vector            * spectrum   = cpl_vector_new(nresolution);
    cpl_wlcalib_slitmodel * model      = cpl_wlcalib_slitmodel_new();
    cpl_bivector          * lines      = my_load_lines_catalog(filename);
    cpl_polynomial        * dispersion = my_1d_dispersion();

    cpl_wlcalib_slitmodel_set_wslit(model, 3.0);
    cpl_wlcalib_slitmodel_set_wfwhm(model, 4.0);
    cpl_wlcalib_slitmodel_set_threshold(model, 5.0);
    cpl_wlcalib_slitmodel_set_catalog(model, lines);

  @endcode

  With that the spectrum can be filled:
  @code

    cpl_wlcalib_fill_line_spectrum(spectrum, model, dispersion);

  @endcode

  Clean-up when no more spectra are needed (lines are deleted with the model):
  @code

    cpl_wlcalib_slitmodel_delete(model);
    cpl_polynomial_delete(dispersion);
    cpl_vector_delete(spectrum);

  @endcode

 Each line profile is given by the convolution of the Dirac delta function with
 a Gaussian with \f$\sigma = w_{FWHM}/(2\sqrt(2\log(2))),\f$ and a top-hat with
 the slit width as width. This continuous line profile is then integrated over
 each pixel, wherever the intensity is above the threshold set by the given
 model. For a given line the value on a given pixel requires the evaluation
 of two calls to @em erf().

 Possible #_cpl_error_code_ set by this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is @em NULL
  - CPL_ERROR_INVALID_TYPE If the input polynomial is not 1D
  - CPL_ERROR_ILLEGAL_INPUT If the input polynomial is non-increasing over
    the given input (pixel) range, or if a model parameter is non-physical
    (e.g. non-positive slit width).
  - CPL_ERROR_DATA_NOT_FOUND If no catalog lines are available in the range
    of the dispersion relation
  - CPL_ERROR_INCOMPATIBLE_INPUT If the wavelengths of two catalog lines are
    found to be in non-increasing order.
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_wlcalib_fill_line_spectrum(cpl_vector            * self,
                                              void                  * model,
                                              const cpl_polynomial  * disp)
{
    cpl_wlcalib_slitmodel * mymodel = (cpl_wlcalib_slitmodel *)model;
    const cpl_size hsize = 0; /* FIXME: remove */

    cpl_ensure_code(model != NULL, CPL_ERROR_NULL_INPUT);

    mymodel->cost++;

    if (cpl_wlcalib_fill_line_spectrum_model(self,
                                             mymodel->linepix,
                                             NULL,
                                             disp,
                                             mymodel->lines,
                                             mymodel->wslit,
                                             mymodel->wfwhm,
                                             mymodel->lthres,
                                             hsize, CPL_FALSE, CPL_FALSE,
                                             &(mymodel->ulines))) {
        return cpl_error_set_where_();
    }

    mymodel->xcost++;

    return CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
  @brief  Generate a 1D spectrum from a model and a dispersion relation
  @param  self   Vector to fill with spectrum
  @param  model  Pointer to cpl_wlcalib_slitmodel object
  @param  disp   1D-Dispersion relation, at least of degree 1
  @return CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error
  @note The spectrum is generated from 1 + the logarithm of the line intensities
  @see cpl_wlcalib_fill_line_spectrum()

 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_wlcalib_fill_logline_spectrum(cpl_vector            * self,
                                                 void                  * model,
                                                 const cpl_polynomial  * disp)
{

    cpl_wlcalib_slitmodel * mymodel = (cpl_wlcalib_slitmodel *)model;
    const cpl_size hsize = 0; /* FIXME: remove */

    cpl_ensure_code(model != NULL, CPL_ERROR_NULL_INPUT);

    mymodel->cost++;

    if (cpl_wlcalib_fill_line_spectrum_model(self,
                                             mymodel->linepix,
                                             NULL,
                                             disp,
                                             mymodel->lines,
                                             mymodel->wslit,
                                             mymodel->wfwhm,
                                             mymodel->lthres,
                                             hsize, CPL_FALSE, CPL_TRUE,
                                             &(mymodel->ulines))) {
        return cpl_error_set_where_();
    }

    mymodel->xcost++;

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief  Generate a 1D spectrum from a model and a dispersion relation
  @param  self   Vector to fill with spectrum
  @param  model  Pointer to cpl_wlcalib_slitmodel object
  @param  disp   1D-Dispersion relation, at least of degree 1
  @return CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error
  @note The generated spectrum will use an approximate line profile for speed
  @see cpl_wlcalib_fill_line_spectrum()

  The approximation preserves the position of the maximum, the symmetry and
  the flux of the line profile.

  The use of a given line in a spectrum requires the evaluation of four calls
  to @em erf().

  The fast spectrum generation can be useful when the model spectrum includes
  many catalog lines.  

 */
/*----------------------------------------------------------------------------*/
cpl_error_code
cpl_wlcalib_fill_line_spectrum_fast(cpl_vector            * self,
                                    void                  * model,
                                    const cpl_polynomial  * disp)
{

    cpl_wlcalib_slitmodel * mymodel = (cpl_wlcalib_slitmodel *)model;
    const cpl_size hsize = 0; /* FIXME: remove */

    cpl_ensure_code(model != NULL, CPL_ERROR_NULL_INPUT);

    mymodel->cost++;

    if (cpl_wlcalib_fill_line_spectrum_model(self,
                                             mymodel->linepix,
                                             &(mymodel->erftmp),
                                             disp,
                                             mymodel->lines,
                                             mymodel->wslit,
                                             mymodel->wfwhm,
                                             mymodel->lthres,
                                             hsize, CPL_TRUE, CPL_FALSE,
                                             &(mymodel->ulines))) {
        return cpl_error_set_where_();
    }

    mymodel->xcost++;

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief  Generate a 1D spectrum from a model and a dispersion relation
  @param  self   Vector to fill with spectrum
  @param  model  Pointer to cpl_wlcalib_slitmodel object
  @param  disp   1D-Dispersion relation, at least of degree 1
  @return CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error
  @note The spectrum is generated from 1 + the logarithm of the line intensities
        and an approximate line profile for speed
  @see cpl_wlcalib_fill_line_spectrum_fast()
 */
/*----------------------------------------------------------------------------*/
cpl_error_code
cpl_wlcalib_fill_logline_spectrum_fast(cpl_vector            * self,
                                       void                  * model,
                                       const cpl_polynomial  * disp)
{

    cpl_wlcalib_slitmodel * mymodel = (cpl_wlcalib_slitmodel *)model;
    const cpl_size hsize = 0; /* FIXME: remove */

    cpl_ensure_code(model != NULL, CPL_ERROR_NULL_INPUT);

    mymodel->cost++;

    if (cpl_wlcalib_fill_line_spectrum_model(self,
                                             mymodel->linepix,
                                             &(mymodel->erftmp),
                                             disp,
                                             mymodel->lines,
                                             mymodel->wslit,
                                             mymodel->wfwhm,
                                             mymodel->lthres,
                                             hsize, CPL_TRUE, CPL_TRUE,
                                             &(mymodel->ulines))) {
        return cpl_error_set_where_();
    }

    mymodel->xcost++;

    return CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
 @brief   Find the best 1D dispersion polynomial in a given search space
 @param   self        Pre-created 1D-polynomial for the result
 @param   guess       1D-polynomial with the guess, may equal self
 @param   spectrum    The vector with the observed 1D-spectrum
 @param   model       The spectrum model
 @param   filler      The function used to make the spectrum
 @param   wl_search   Search range around the anchor points, same unit as guess
 @param   nsamples    Number of samples around the anchor points
 @param   hsize       Maximum (pixel) displacement of the polynomial guess
 @param   xcmax       On success, the maximum cross-correlation
 @param   xcorrs      The vector to fill with the correlation values or NULL
 @return  CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error
 @see cpl_wlcalib_fill_line_spectrum() for the model and filler.

 Find the polynomial that maximizes the cross-correlation between an observed
 1D-spectrum and a model spectrum based on the polynomial dispersion relation. 

 Each element in the vector of wavelength search ranges is in the same unit as
 the corresponding Y-value of the dispersion relation. Each value in the vector
 is the width of a search window centered on the corresponding value in the
 guess polynomial. The length D of the search vector thus determines the
 dimensionality of the search space for the dispersion polynomial. If for
 example the search vector consists of three elements, then the three lowest
 order coefficients of the dispersion relation may be modified by the search.

 For each candidate polynomial P(x), the polynomial P(x+u), -hsize <= u <= hsize
 is also evaluated. The half-size hsize may be zero. When it is non-zero, an
 additional 2 * hsize cross-correlations are performed for each candidate
 polynomial, one for each possible shift. The maximizing polynomial among those
 shifted polynomials is kept. A well-chosen half-size can allow for the use of
 fewer number of samples around the anchor points, leading to a reduction of
 polynomials to be evaluated.

 The complexity in terms of model spectra creation is O(N^D) and in terms of
 cross-correlations O(hsize * N^D), where N is nsamples and D is the length
 of wl_search.

 xcorrs must be NULL or have a size of (at least) N^D*(1 + 2 * hsize).
 
 Possible #_cpl_error_code_ set by this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is @em NULL
  - CPL_ERROR_INVALID_TYPE If an input polynomial is not 1D
  - CPL_ERROR_ILLEGAL_INPUT If nfree is less than 2, or nsamples is less than 1,
    hsize negative  or if wl_search contains a zero search bound, or if xcorrs
    is non-NULL and too short.
  - CPL_ERROR_DATA_NOT_FOUND If no model spectra can be created using the
      supplied model and filler
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_wlcalib_find_best_1d(cpl_polynomial       * self,
                                        const cpl_polynomial * guess,
                                        const cpl_vector     * spectrum,
                                        void                 * model,
                                        cpl_error_code      (* filler)(cpl_vector *,
                                                void *, const cpl_polynomial *),
                                        const cpl_vector     * wl_search,
                                        cpl_size               nsamples,
                                        cpl_size               hsize,
                                        double               * xcmax,
                                        cpl_vector           * xcorrs)
{
    cpl_errorstate   prestate = cpl_errorstate_get();
    cpl_errorstate   errorstate = prestate; /* Fix (false) uninit warning */
    const cpl_size   spec_sz  = cpl_vector_get_size(spectrum);
    const cpl_size   nfree    = cpl_vector_get_size(wl_search);
    const cpl_size   degree   = nfree - 1;
    const cpl_size   nxc      = 1 + 2 * hsize;
    cpl_size         ntests   = 1;
    cpl_vector     * spmodel;
    cpl_vector     * vxc;
    cpl_vector     * init_pts_wl;
    cpl_vector     * init_pts_x;
    cpl_vector     * pts_wl;
    cpl_vector     * conv_kernel = NULL;    
    cpl_polynomial * cand;
    double         * xcdata = xcorrs ? cpl_vector_get_data(xcorrs) : NULL;
    const double   * pwl_search = cpl_vector_get_data_const(wl_search); 
    const double   * pinit_pts_wl;
    double         * ppts_wl;
    cpl_boolean      has_solution = CPL_FALSE;
    cpl_boolean      has_no_error = CPL_TRUE;
    cpl_size         i;

    cpl_ensure_code(xcmax     != NULL, CPL_ERROR_NULL_INPUT);
    *xcmax = -1.0; /* In case of failure */

    cpl_ensure_code(self      != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(guess     != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(spectrum  != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(model     != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(filler    != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(wl_search != NULL, CPL_ERROR_NULL_INPUT);

    cpl_ensure_code(cpl_polynomial_get_dimension(self) == 1,
                    CPL_ERROR_INVALID_TYPE);
    cpl_ensure_code(cpl_polynomial_get_dimension(guess) == 1,
                    CPL_ERROR_INVALID_TYPE);

    cpl_ensure_code(nfree     >= 2, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(nsamples  >  0, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(hsize     >= 0, CPL_ERROR_ILLEGAL_INPUT);

    if (nsamples > 1) {
        /* Search place must consist of more than one point */
        /* FIXME: The bounds should probably not be negative */
        for (i = 0; i < nfree; i++) {
            if (pwl_search[i] != 0.0) break;
        }
        cpl_ensure_code(i < nfree, CPL_ERROR_ILLEGAL_INPUT);
    }

    if (xcorrs != NULL) {
        const cpl_size ncand = nxc * (cpl_size)cpl_tools_ipow(nsamples, nfree);
        cpl_ensure_code(cpl_vector_get_size(xcorrs) >= ncand,
                        CPL_ERROR_ILLEGAL_INPUT);
    }
 
    /* Create initial test points */
    init_pts_x  = cpl_vector_new(nfree);
    init_pts_wl = cpl_vector_new(nfree);
    pts_wl      = cpl_vector_new(nfree);
    for (i = 0; i < nfree; i++) {
        const double xpos  = spec_sz * i / (double)degree;
        const double wlpos = cpl_polynomial_eval_1d(guess, xpos, NULL)
            - 0.5 * pwl_search[i];

        cpl_vector_set(init_pts_x,  i, xpos);
        cpl_vector_set(init_pts_wl, i, wlpos);

        ntests *= nsamples; /* Count number of tests */

    }

    cand = cpl_polynomial_new(1);
    spmodel = cpl_vector_new(spec_sz + 2 * hsize);
    vxc = xcdata ? cpl_vector_wrap(nxc, xcdata) : cpl_vector_new(nxc);
    pinit_pts_wl = cpl_vector_get_data_const(init_pts_wl);
    ppts_wl = cpl_vector_get_data(pts_wl);

    /* Create the polynomial candidates and estimate them */
    for (i = 0; i < ntests; i++) {
        cpl_size       idiv = i;
        cpl_size       deg;
        cpl_error_code error = CPL_ERROR_NONE;

        /* Update wavelength at one anchor point - and reset wavelengths
           to their default for any anchor point(s) at higher wavelengths */
        for (deg = degree; deg >= 0; deg--, idiv /= nsamples) {
            const cpl_size imod  = idiv % nsamples;
            const double   wlpos = pinit_pts_wl[deg]
                + imod * pwl_search[deg] / nsamples;

            if (deg < degree && wlpos >= ppts_wl[deg+1]) {
                /* The current anchor points do not form a monotonely
                   increasing sequence. This is not physical, so mark
                   the polynomial as invalid. Still need to complete loop
                   for subsequent candidates */
                /* Not an actual error, but used for flow control */
                error = CPL_ERROR_DATA_NOT_FOUND;
            }
            ppts_wl[deg] = wlpos;

            if (imod > 0) break;
        }

        if (!error) {

            has_no_error = cpl_errorstate_is_equal(prestate);

            /* Fit the candidate polynomial, init_pts_x is symmetric */
            error = cpl_polynomial_fit_1d(cand, init_pts_x, pts_wl, 0, degree,
                                          CPL_TRUE, NULL);
            if (!error && cpl_polynomial_get_degree(cand) > 0) {

                /* In spmodel input pixel 0 is at position hsize,
                   shift accordingly */
                error = cpl_polynomial_shift_1d(cand, 0, -hsize);

                if (!error) {
                    cpl_size ixcmax = -1; /* Fix (false) uninit warning */

                    /* *** Estimate *** */
                    error = cpl_wlcalib_evaluate(vxc, &ixcmax, spmodel,
                                                 spectrum, model, filler, cand);
                    if (!error) {
                        const double xci = cpl_vector_get(vxc, ixcmax);

                        if (xci > *xcmax) {
                            /* Found a better solution */
                            *xcmax = xci;
                            cpl_polynomial_shift_1d(cand, 0, ixcmax);
                            cpl_polynomial_copy(self, cand);
                            has_solution = CPL_TRUE;
                        }
                    }
                }
            }

            if (error) {
                if (has_solution) {
#ifdef CPL_WLCALIB_DEBUG
                    cpl_errorstate_dump(prestate, CPL_FALSE,
                                        cpl_errorstate_dump_one_debug);
#endif
                    cpl_errorstate_set(prestate);
                } else if (has_no_error) {
                    /* Keep error(s) from 1st failed try in case no solutions
                       are found */
                    errorstate = cpl_errorstate_get();
                } else {
                    /* Reset all subsequent errors - which may be many */
#ifdef CPL_WLCALIB_DEBUG
                    cpl_errorstate_dump(errorstate, CPL_FALSE,
                                        cpl_errorstate_dump_one_debug);
#endif
                    cpl_errorstate_set(errorstate);
                }
            }
        }

        if (xcdata != NULL) {
            if (error) (void)cpl_vector_fill(vxc, -1.0);
            xcdata += nxc;
            (void)cpl_vector_rewrap(vxc, nxc, xcdata);
        }
    }

    cpl_vector_delete(spmodel);
    xcdata ? (void)cpl_vector_unwrap(vxc) : cpl_vector_delete(vxc);
    cpl_vector_delete(conv_kernel);
    cpl_vector_delete(pts_wl);
    cpl_vector_delete(init_pts_x);
    cpl_vector_delete(init_pts_wl);
    cpl_polynomial_delete(cand);

    return has_solution ? CPL_ERROR_NONE :
        cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);
}

/**@}*/

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief   Generate a 1D spectrum from (arc) lines and a dispersion relation
  @param   self    Vector to fill with spectrum
  @param   linepix Vector to update with best guess of line pixel position
  @param   perftmp Vector to update with erf()-values in fast mode, may be NULL
  @param   disp    1D-Dispersion relation, at least of degree 1
  @param   lines   Catalog of lines, with increasing wavelengths 
  @param   wslit   Positive width of the slit
  @param   wfwhm   Positive FWHM of the transfer function
  @param   lthres  Line truncation threshold
  @param   hsize   The 1st intensity in self will be disp(1-hsize), hsize >= 0
  @param   dofast  Iff true compose profile from pairs of two integer-placed
  @param   dolog   Iff true log(1+I) is used for the (positive) intensities
  @param   pulines Iff non-NULL incremented by number of lines used, on success
  @return  CPL_ERROR_NONE on success, otherwise the relevant CPL error code
  @see cpl_wlcalib_fill_line_spectrum()

 */
/*----------------------------------------------------------------------------*/
static
cpl_error_code cpl_wlcalib_fill_line_spectrum_model(cpl_vector * self,
                                                    cpl_vector * linepix,
                                                    cpl_vector ** perftmp,
                                                    const cpl_polynomial * disp,
                                                    const cpl_bivector * lines,
                                                    double wslit,
                                                    double wfwhm,
                                                    double lthres,
                                                    cpl_size hsize,
                                                    cpl_boolean dofast,
                                                    cpl_boolean dolog,
                                                    cpl_size * pulines)
{
    cpl_errorstate     prestate;
    const double       sigma   = wfwhm * CPL_MATH_SIG_FWHM;
    const cpl_vector * xlines  = cpl_bivector_get_x_const(lines);
    const double     * dxlines = cpl_vector_get_data_const(xlines);
    const double     * dylines = cpl_bivector_get_y_data_const(lines);
    double           * plinepix
        = linepix ? cpl_vector_get_data(linepix) : NULL;
    const cpl_size     nlines  = cpl_vector_get_size(xlines);
    const cpl_size     nself   = cpl_vector_get_size(self);
    double           * dself   = cpl_vector_get_data(self);
    cpl_polynomial   * dispi;
    double           * profile = NULL;
    cpl_size           i0 = 0;
    const double       p0 = cpl_polynomial_get_coeff(disp, &i0);
    const cpl_size     degree = cpl_polynomial_get_degree(disp);
    double             wl;
    const double       xtrunc = 0.5 * wslit + lthres * sigma;
    double             xpos = (double)(1-hsize)-xtrunc;
    const double       xmax = (double)(nself-hsize)+xtrunc;
    double             xderiv;
    cpl_error_code     error = CPL_ERROR_NONE;
    cpl_size           iline;
    cpl_size           ulines = 0;

    cpl_ensure_code(self     != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(disp     != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(lines    != NULL, CPL_ERROR_NULL_INPUT);

    cpl_ensure_code(wslit  >  0.0, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(wfwhm  >  0.0, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(hsize  >= 0,   CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(xtrunc >  0.0, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(nself  > 2 * hsize, CPL_ERROR_ILLEGAL_INPUT);

    cpl_ensure_code(cpl_polynomial_get_dimension(disp) == 1,
                    CPL_ERROR_INVALID_TYPE);
    cpl_ensure_code(degree > 0, CPL_ERROR_ILLEGAL_INPUT);

    /* The smallest wavelength contributing to the spectrum. */
    wl = cpl_polynomial_eval_1d(disp, xpos, &xderiv);

    if (wl <= 0.0) return
        cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT, "Non-positive "
                               "wavelength at x=%g: P(x)=%g, P'(x)=%g",
                               xpos, wl, xderiv);

    if (xderiv <= 0.0) return
        cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT, "Non-increasing "
                               "dispersion at x=%g: P'(x)=%g, P(x)=%g, "
                               "degree=%" CPL_SIZE_FORMAT, xpos, xderiv,
                               wl, degree);

    /* Find the 1st line */
    iline = cpl_vector_find(xlines, wl);
    if (iline < 0) {
        return cpl_error_set_message_(CPL_ERROR_INCOMPATIBLE_INPUT, "Invalid "
                                      "%" CPL_SIZE_FORMAT "-entry catalog",
                                      nlines);
    }

    /* The first line must be at least at wl */
    if (dxlines[iline] < wl) iline++;

    if (iline >= nlines) return
        cpl_error_set_message_(CPL_ERROR_DATA_NOT_FOUND, "The %" CPL_SIZE_FORMAT
                               "-line catalog has only lines below "
                               "P(%g)=%g > %g, degree=%" CPL_SIZE_FORMAT,
                               nlines, xpos, wl, dxlines[nlines-1], degree);

    (void)memset(dself, 0, nself * sizeof(double));

    prestate = cpl_errorstate_get();

    if (dofast) {
        const cpl_size npix = 1+(cpl_size)xtrunc;

        if (*perftmp != NULL && cpl_vector_get_size(*perftmp) == npix &&
            cpl_vector_get(*perftmp, 0) > 0.0) {
            profile = cpl_vector_get_data(*perftmp);
        } else {

            const double yval =  0.5 / wslit;
            const double x0p  =  0.5 * wslit + 0.5;
            const double x0n  = -0.5 * wslit + 0.5;
            double       x1diff
                = cpl_erf_antideriv(x0p, sigma)
                - cpl_erf_antideriv(x0n, sigma);
            cpl_size     ipix;

            if (*perftmp == NULL) {
                *perftmp = cpl_vector_new(npix);
            } else {
                cpl_vector_set_size(*perftmp, npix);
            }
            profile = cpl_vector_get_data(*perftmp);

            profile[0] = 2.0 * yval * x1diff;

            for (ipix = 1; ipix < npix; ipix++) {
                const double x1 = (double)ipix;
                const double x1p = x1 + 0.5 * wslit + 0.5;
                const double x1n = x1 - 0.5 * wslit + 0.5;
                const double x0diff = x1diff;

                x1diff = cpl_erf_antideriv(x1p, sigma)
                    - cpl_erf_antideriv(x1n, sigma);

                profile[ipix] = yval * (x1diff - x0diff);

            }

            cpl_tools_add_flops( 33 * npix );

        }
    }

    dispi = cpl_polynomial_duplicate(disp);

    /* FIXME: A custom version of cpl_polynomial_solve_1d() which returns
       P'(xpos) can be used for the 1st NR-iteration. */
    /* Perform 1st NR-iteration in solving for P(xpos) = dxlines[iline] */
    xpos -= (wl - dxlines[iline]) / xderiv;

    /* Iterate through the lines */
    for (; !error && iline < nlines; iline++) {

        if (iline > 0 && dxlines[iline-1] >= dxlines[iline]) {
            error = cpl_error_set_message_(CPL_ERROR_INCOMPATIBLE_INPUT,
                                           "Non-increasing wavelengths in %"
                                           CPL_SIZE_FORMAT "-entry catalog: I(%"
                                           CPL_SIZE_FORMAT ") = %g >= %g = I(%"
                                           CPL_SIZE_FORMAT "+1)", nlines, iline,
                                           dxlines[iline-1], dxlines[iline],
                                           iline);
            break;
        }

        /* Lines may have a non-physical intensity (e.g. zero) to indicate some
           property of the line, e.g. unknown intensity due to blending */
        if (dylines[iline] <= 0.0) continue;

        /* Use 1st guess, if available (Use 0.0 to flag unavailable) */
        if (plinepix != NULL && plinepix[iline] > 0.0) xpos = plinepix[iline];

        if (xpos > xmax) xpos = xmax; /* FIXME: Better to limit xpos ? */

        /* Find the (sub-) pixel position of the line */
        /* Also, verify monotony of dispersion */
        error = cpl_polynomial_set_coeff(dispi, &i0, p0 - dxlines[iline]) ||
            cpl_polynomial_solve_1d_(dispi, xpos, &xpos, 1, CPL_TRUE);

        if (xpos > xmax) {
            if (error) {
                error = 0;
#ifdef CPL_WLCALIB_DEBUG
                cpl_msg_debug(cpl_func, "Stopping spectrum fill at line %d/%d "
                             "at xpos=%g > xmax=%g",
                             iline, nlines, xpos, xmax);
                cpl_errorstate_dump(prestate, CPL_FALSE,
                                    cpl_errorstate_dump_one_debug);
#endif
                cpl_errorstate_set(prestate);
            }
            break;
        } else if (error) {
            if (linepix != NULL && ulines) (void)cpl_vector_fill(linepix, 0.0);
            (void)cpl_error_set_message_(cpl_error_get_code(),
                                         "Could not find pixel-position "
                                         "of line %" CPL_SIZE_FORMAT "/%"
                                         CPL_SIZE_FORMAT " at wavelength=%g. "
                                         "xpos=%g, xmax=%g", iline, nlines,
                                         dxlines[iline], xpos, xmax);
            break;
        } else if (dofast) {
            const double frac  = fabs(xpos - floor(xpos));
#ifdef CPL_WAVECAL_FAST_FAST
            const double frac0 = 1.0 - frac; /* Weight opposite of distance */
#else
            /* Center intensity correctly */
            const double ep1pw = cpl_erf_antideriv(frac + 0.5 * wslit, sigma);
            const double en1pw = cpl_erf_antideriv(frac + 0.5 * wslit - 1.0,
                                                      sigma);
            const double ep1nw = cpl_erf_antideriv(frac - 0.5 * wslit, sigma);
            const double en1nw = cpl_erf_antideriv(frac - 0.5 * wslit - 1.0,
                                                      sigma);
            const double frac0
                = (en1nw - en1pw) / (ep1pw - en1pw - ep1nw + en1nw);

#endif
            const double frac1 = 1.0 - frac0;
            const double yval0 = frac0 * dylines[iline];
            const double yval1 = frac1 * dylines[iline];
            const cpl_size npix  = 1+(cpl_size)xtrunc;
            cpl_size     ipix;
            cpl_size     i0n    = hsize - 1 + floor(xpos);
            cpl_size     i0p    = i0n;
            cpl_size     i1n    = i0n + 1;
            cpl_size     i1p    = i1n;
            cpl_boolean  didline = CPL_FALSE;


            /* Update 1st guess for next time, if available */
            if (plinepix != NULL) plinepix[iline] =  xpos;

            if (frac0 < 0.0) {
                (void)cpl_error_set_message_(CPL_ERROR_UNSPECIFIED, "Illegal "
                                             "split at x=%g: %g + %g = 1",
                                             xpos, frac0, frac1);
#ifdef CPL_WAVEVAL_DEBUG
            } else {
                cpl_msg_warning(cpl_func,"profile split at x=%g: %g + %g = 1",
                                xpos, frac0, frac1);
#endif
            }

            for (ipix = 0; ipix < npix; ipix++, i0n--, i0p++, i1n--, i1p++) {

                if (i0n >= 0 && i0n < nself) {
                    dself[i0n] += yval0 * profile[ipix];
                    didline = CPL_TRUE;
                }
                if (i1n >= 0 && i1n < nself && ipix + 1 < npix) {
                    dself[i1n] += yval1 * profile[ipix+1];
                    didline = CPL_TRUE;
                }

                if (ipix == 0) continue;

                if (i0p >= 0 && i0p < nself) {
                    dself[i0p] += yval0 * profile[ipix];
                    didline = CPL_TRUE;
                }
                if (i1p >= 0 && i1p < nself && ipix + 1 < npix) {
                    dself[i1p] += yval1 * profile[ipix+1];
                    didline = CPL_TRUE;
                }
            }

            if (didline) ulines++;

        } else {
            const double   yval = 0.5 * dylines[iline] / wslit;
            const cpl_size ifirst = CX_MAX((cpl_size)(xpos-xtrunc+0.5),
                                           1-hsize);
            const cpl_size ilast  = CX_MIN((cpl_size)(xpos+xtrunc),
                                           nself-hsize);
            cpl_size       ipix;
            const double   x0 = (double)ifirst - xpos;
            const double   x0p = x0 + 0.5*wslit - 0.5;
            const double   x0n = x0 - 0.5*wslit - 0.5;
            double         x1diff
                = cpl_erf_antideriv(x0p, sigma)
                - cpl_erf_antideriv(x0n, sigma);

            /* Update 1st guess for next time, if available */
            if (plinepix != NULL) plinepix[iline] =  xpos;

            if (ilast >= ifirst) ulines++;

            for (ipix = ifirst; ipix < 1 + ilast; ipix++) {
                const double x1 = (double)ipix - xpos;
                const double x1p = x1 + 0.5*wslit + 0.5;
                const double x1n = x1 - 0.5*wslit + 0.5;
                const double x0diff = x1diff;

                x1diff = cpl_erf_antideriv(x1p, sigma)
                    - cpl_erf_antideriv(x1n, sigma);

                dself[ipix+hsize-1] += yval * (x1diff - x0diff);

            }
        }
    }

    cpl_polynomial_delete(dispi);

    cpl_ensure_code(!error, cpl_error_get_code());

    if (dolog) {
        int i;
        for (i = 0; i < nself; i++) {
            dself[i] = dself[i] > 0.0 ? log1p(dself[i]) : 0.0;
        }
            cpl_tools_add_flops( 2 * nself );
    }

    if (!ulines) return
        cpl_error_set_message_(CPL_ERROR_DATA_NOT_FOUND, "The %" CPL_SIZE_FORMAT
                               "-line catalog has no lines in the range "
                               "%g -> P(%g)=%g", nlines, wl, xmax,
                               cpl_polynomial_eval_1d(disp, xmax, NULL));

    cpl_tools_add_flops( ulines * (dofast ? 4 * (cpl_flops)xtrunc + 74
                                   : 70 * (cpl_flops)xtrunc + 35 ));

    if (pulines != NULL) *pulines += ulines;

    return CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    The antiderivative of erx(x/sigma/sqrt(2)) with respect to x
  @param    x      x
  @param    sigma  sigma
  @return   The antiderivative
  @note This function is even.

  12 FLOPs.

 */
/*----------------------------------------------------------------------------*/
inline static double cpl_erf_antideriv(double x, double sigma)
{
    return x * erf( x / (sigma * CPL_MATH_SQRT2))
       + 2.0 * sigma/CPL_MATH_SQRT2PI * exp(-0.5 * x * x / (sigma * sigma));
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Evaluate a dispersion relation
  @param    vxc             The 1-vector of cross-correlation(s) to be filled
  @param    pixcmax         The index in vxc with the maximum cross-correlation
  @param    spmodel         Temporary storage for the model spectrum
  @param    observed        The observed spectrum
  @param    model           The spectrum model
  @param    filler          The spectrum filler function
  @param    disp            The candidate dispersion relation
  @return CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error

 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_wlcalib_evaluate(cpl_vector           * vxc,
                                           cpl_size             * pixcmax,
                                           cpl_vector           * spmodel,
                                           const cpl_vector     * observed,
                                           void                 * model,
                                           cpl_error_code      (* filler)
                                           (cpl_vector *, void *,
                                            const cpl_polynomial *),
                                           const cpl_polynomial * disp)
{

    cpl_error_code error;

    cpl_ensure_code(vxc      != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(pixcmax  != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(spmodel  != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(observed != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(model    != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(filler   != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(disp     != NULL, CPL_ERROR_NULL_INPUT);

    error = filler(spmodel, model, disp);

    if (!error) {
        *pixcmax = cpl_vector_correlate(vxc, spmodel, observed);
        if (*pixcmax < 0) error = cpl_error_get_code();
    }

    return error ? cpl_error_set_where_() : CPL_ERROR_NONE;
}
